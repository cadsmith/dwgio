# Editing a drawing

A loaded drawing is plain data: change a field of an object and the next save writes it. `Edit` adds and removes objects with every link that ties them to the rest of the drawing; a change in place is yours to keep consistent. The examples edit `entities-r2000.dwg`, which AutoCAD drew with a block `Tag` holding two attribute definitions, an insert of it with one attribute, a layer `Walls` and a group `PAIR` of a line and a circle, then a drawing made with `Create`.

## Change an entity {#change}

`Find` gives an object by handle, and `Get` gives it as its kind, both mutable on a mutable drawing. Move the line of the group 10 units right, onto the layer `Walls`, and color it red:

```cpp
auto loaded = DwgIO::LoadFile("entities-r2000.dwg");
if (!loaded)
{
  return 1;
}
DwgIO::Drawing &drawing = loaded->Drawing;
const DwgIO::Handle walls = DwgIO::View::Record<DwgIO::Tables::Layer>(drawing, "Walls")->Record.Base.Self;

const DwgIO::Handle lineHandle = DwgIO::ParseHandle("2D3").value();
auto &line = DwgIO::Get<DwgIO::Entities::Line>(*DwgIO::Find(drawing, lineHandle));
line.Start.X += 10;
line.End.X += 10;
line.Common.Layer = walls;
line.Common.Color.Index = 1;
std::println("LINE {} from ({}, {}) to ({}, {}) on layer {}", DwgIO::HexOf(line.Base.Self), line.Start.X,
             line.Start.Y, line.End.X, line.End.Y, DwgIO::HexOf(line.Common.Layer));
```

```text [Output]
LINE 2D3 from (10, 0) to (110, 50) on layer 2CF
```

The geometry, the color, the linetype scale and every other value can change freely. The handles are another matter, since other objects find this one through them, and it finds them:

- `Base::Self` is how every other object names it: leave it.
- `Base::Owner`, `Common::Previous` and `Common::Next` place it in its block's chain. Moving an entity to another block is a removal and an addition, as [below](#copy).
- `Common::Layer`, `Common::Linetype`, a text's style or an insert's block must name a record of that kind that exists. `View::Record` and `View::Block` find them by name.
- `Base::Reactors` names the objects that watch it, such as the group, and `Base::Dictionary` the extension dictionary it owns: leave them.

A line, a point and a 3D face are in world coordinates; most planar entities are in their own, set by `Extrusion`. [Coordinates and units](/tutorials/coordinates/#ocs) says which, so you know what a move means for each.

An attribute is edited the same way. The insert's attribute holds Alice; set it to Room 101. An ATTRIB holds its own position, not one relative to its insert: an insert you move leaves its attributes where they were until you move them by the same amount.

```cpp
auto &nameAttribute = DwgIO::Get<DwgIO::Entities::Attrib>(*DwgIO::Find(drawing, DwgIO::ParseHandle("2F1").value()));
nameAttribute.Lettering.Value = "Room 101";
```

## Add objects {#add}

`Edit::Add` appends an entity to a space or a block, or a record to its table; `Edit::AddBlock` makes a block. Each gives the new object its handle, its owner and its place in every list that names it, and returns the handle. [Creating a drawing](/tutorials/creating/) shows each one.

```cpp
DwgIO::Entities::Circle column;
column.Center = DwgIO::Vector{.X = 30, .Y = 0, .Z = 0};
column.Radius = 0.5;
column.Common.Layer = walls;
const auto circleHandle = DwgIO::Edit::Add(drawing, column);
if (!circleHandle)
{
  std::println("{}", circleHandle.error().What);
  return 1;
}
std::println("CIRCLE {}, last of {} in model space", DwgIO::HexOf(*circleHandle),
             DwgIO::View::Entities(drawing, *DwgIO::View::ModelSpace(drawing)).size());
```

```text [Output]
CIRCLE 449, last of 24 in model space
```

## Attach attributes {#attributes}

`Edit::AddAttribute` appends an ATTRIB to an insert, after the ones it has, and keeps the SEQEND that closes them. The block `Tag` defines two attributes: `SIZE`, constant, which stays in the block and no insert repeats, and `NAME`, which each insert fills in. A new insert of it gets an ATTRIB for each definition that is not constant (flag 2), at the definition's place moved by the insert's position.

```cpp
const DwgIO::Tables::BlockRecord &tag = *DwgIO::View::Block(drawing, "Tag");
DwgIO::Entities::Insert newInsert;
newInsert.Block = tag.Record.Base.Self;
newInsert.Position = DwgIO::Vector{.X = 220, .Y = 0, .Z = 0};
std::vector<DwgIO::Entities::Attrib> newAttributes;
for (const DwgIO::Object *entity : DwgIO::View::Entities(drawing, tag))
{
  if (const auto *definition = DwgIO::GetIf<DwgIO::Entities::AttDef>(entity);
      definition != nullptr && (definition->Flags & 2) == 0)
  {
    DwgIO::Entities::Attrib value;
    value.Tag = definition->Tag;
    value.Flags = definition->Flags;
    value.Lettering = definition->Lettering;
    value.Lettering.Value = "Room 102";
    value.Lettering.Position.X += newInsert.Position.X;
    newAttributes.push_back(value);
  }
}
const auto newInsertHandle = DwgIO::Edit::Add(drawing, newInsert);
if (!newInsertHandle)
{
  return 1;
}
for (const DwgIO::Entities::Attrib &value : newAttributes)
{
  if (!DwgIO::Edit::AddAttribute(drawing, *newInsertHandle, value))
  {
    return 1;
  }
}
for (const DwgIO::Object &object : drawing.Objects)
{
  if (const auto *attribute = DwgIO::GetIf<DwgIO::Entities::Attrib>(&object))
  {
    std::println("{} {} = {} on INSERT {}", DwgIO::HexOf(attribute->Base.Self), attribute->Tag,
                 attribute->Lettering.Value, DwgIO::HexOf(attribute->Base.Owner));
  }
}
```

```text [Output]
2F1 NAME = Room 101 on INSERT 2EF
44B NAME = Room 102 on INSERT 44A
```

The definitions are gathered before the first addition because an addition changes `Drawing::Objects`, which [invalidates](/guide/objects/#handles) every pointer the view gave, `tag` and `definition` included. Adding the position is right for an insert at scale 1 and no rotation, as this one is; [Blocks and attributes](/tutorials/blocks/#insert) places an attribute under any scale and rotation.

## Copy an entity {#copy}

A copy of an object is a new object: it needs a handle of its own, an owner and a place in a chain, which `Edit::Add` gives it. The copy is made from the original, so it starts with the original's handle, which the addition refuses:

```cpp
const DwgIO::Handle insertHandle = DwgIO::ParseHandle("2EF").value();
const DwgIO::Object original = *DwgIO::Find(drawing, lineHandle);
std::println("{}", DwgIO::Edit::Add(drawing, original).error().What);
```

```text [Output]
handle 2D3 exists
```

Clear the handle, and with it what belongs to the original alone: the reactors, such as the group, which does not list the copy, and the extension dictionary, which only the original owns. `Edit::Add` sets the owner, the space and the links to the neighbours itself. A copy can be added to another block; this one is added to `Tag`.

```cpp
DwgIO::Object copy = original;
DwgIO::BaseOf(copy).Self = {};
DwgIO::BaseOf(copy).Reactors.clear();
DwgIO::BaseOf(copy).Dictionary = {};
DwgIO::Get<DwgIO::Entities::Line>(copy).End.Y = 5;
const auto copyHandle = DwgIO::Edit::Add(drawing, copy, "Tag");
if (!copyHandle)
{
  return 1;
}
std::string contents;
for (const DwgIO::Object *entity : DwgIO::View::Entities(drawing, *DwgIO::View::Block(drawing, "Tag")))
{
  contents += std::format("{}{} {}", contents.empty() ? "" : ", ", DwgIO::TypeOf(*entity),
                      DwgIO::HexOf(DwgIO::BaseOf(*entity).Self));
}
std::println("Tag holds {}", contents);
```

```text [Output]
Tag holds LINE 2EB, ATTDEF 2EC, ATTDEF 2ED, LINE 44D
```

An object that owns others is copied with new copies of what it owns, as AutoCAD's COPY does. For an insert, clear its attribute links as well, add it, then add a copy of each attribute to it:

```cpp
DwgIO::Object insertCopy = *DwgIO::Find(drawing, insertHandle);
auto &insert = DwgIO::Get<DwgIO::Entities::Insert>(insertCopy);
insert.Base.Self = {};
insert.Base.Reactors.clear();
insert.Base.Dictionary = {};
insert.Follows = false;
insert.First = {};
insert.Last = {};
insert.SeqEnd = {};
insert.Owned.clear();
insert.Position.Y += 10;
const auto insertCopyHandle = DwgIO::Edit::Add(drawing, insertCopy);
if (!insertCopyHandle)
{
  return 1;
}
std::vector<DwgIO::Entities::Attrib> attributes;
for (const DwgIO::Object &object : drawing.Objects)
{
  if (const auto *attribute = DwgIO::GetIf<DwgIO::Entities::Attrib>(&object);
      attribute != nullptr && attribute->Base.Owner == insertHandle)
  {
    attributes.push_back(*attribute);
  }
}
for (DwgIO::Entities::Attrib attribute : attributes)
{
  attribute.Base.Self = {};
  attribute.Base.Reactors.clear();
  attribute.Base.Dictionary = {};
  attribute.Lettering.Position.Y += 10;
  if (!DwgIO::Edit::AddAttribute(drawing, *insertCopyHandle, attribute))
  {
    return 1;
  }
}
const auto &insertInDrawing = DwgIO::Get<DwgIO::Entities::Insert>(*DwgIO::Find(drawing, *insertCopyHandle));
std::println("INSERT {} with ATTRIB {}; {} inserts of Tag", DwgIO::HexOf(*insertCopyHandle),
             DwgIO::HexOf(insertInDrawing.First), DwgIO::View::Block(drawing, "Tag")->Inserts.size());
```

```text [Output]
INSERT 44E with ATTRIB 44F; 3 inserts of Tag
```

The attributes are copied out first, as the definitions were above. A POLYLINE copies the same way: add it with `First`, `Last`, `SeqEnd` and `Vertices` cleared, then each VERTEX with `Edit::AddVertex`, which chains or lists it as the release does, closes the sequence with a SEQEND, and gives a vertex without type bits its polyline's. A LWPOLYLINE holds its points itself and copies like a line.

```cpp
DwgIO::Drawing newDrawing = DwgIO::Create();
DwgIO::Entities::Polyline polyline;
polyline.Flags = std::to_underlying(DwgIO::Entities::PolylineFlags::Polyline3d);
const auto polylineHandle = DwgIO::Edit::Add(newDrawing, polyline);
if (!polylineHandle)
{
  return 1;
}
for (const double z : {0.0, 1.0, 3.0})
{
  DwgIO::Entities::Vertex vertex;
  vertex.Position = DwgIO::Vector{.X = z, .Y = 0, .Z = z};
  if (!DwgIO::Edit::AddVertex(newDrawing, *polylineHandle, vertex))
  {
    return 1;
  }
}
const auto &polylineInDrawing = DwgIO::Get<DwgIO::Entities::Polyline>(*DwgIO::Find(newDrawing, *polylineHandle));
const auto vertices = DwgIO::View::Vertices(newDrawing, polylineInDrawing);
std::println("{} vertices, the last 3D: {}", vertices.size(),
             DwgIO::Has(vertices.back()->Flags, DwgIO::Entities::VertexFlags::Polyline3d));
```

```text [Output]
3 vertices, the last 3D: true
```

## Rename a record {#rename}

`Edit::Rename` renames a table record or a block, and a block's BLOCK with it. Everything else names a record by its handle, so an entity on a renamed layer stays on it. The drawing's own records, such as layer 0 or a space, an anonymous block, a record from an external reference, a name with a character AutoCAD refuses and a name the table already has, in any case, are refused.

```cpp
DwgIO::Tables::Layer doors;
doors.Record.Name = "Doors";
doors.Linetype = newDrawing.Header.LTypeContinuous;
const auto layer = DwgIO::Edit::Add(newDrawing, doors);
if (!layer)
{
  return 1;
}
std::println("{}", DwgIO::Edit::Rename(newDrawing, *layer, "Openings") ? "renamed" : "refused");
std::println("{}", DwgIO::Edit::Rename(newDrawing, *layer, "0").error().What);
```

```text [Output]
renamed
layer 0 exists
```

## Remove objects {#remove}

`Edit::Remove` takes an object out with everything it owns, as AutoCAD's ERASE and PURGE do, and drops it from every list that names it. The line leaves the group `PAIR` with it:

```cpp
const auto groups = [&drawing]() -> std::size_t
{
  const DwgIO::Object *pair = DwgIO::View::Entry(drawing, *DwgIO::View::Dictionary(drawing, "ACAD_GROUP"), "PAIR");
  return DwgIO::Get<DwgIO::Objects::Group>(*pair).Members.size();
};
std::println("{} objects, {} in the group", drawing.Objects.size(), groups());
if (const auto removed = DwgIO::Edit::Remove(drawing, lineHandle); !removed)
{
  std::println("{}", removed.error().What);
  return 1;
}
std::println("{} objects, {} in the group", drawing.Objects.size(), groups());
```

```text [Output]
267 objects, 2 in the group
266 objects, 1 in the group
```

A removal erases from `Drawing::Objects`, so a pointer or reference taken into it before, such as `line` above, is invalid afterwards; find the object again by its handle.

What a removal deletes with an object:

| You remove | It takes |
| :--- | :--- |
| an entity | its extension dictionary; for an insert, its attributes and SEQEND; for a polyline, its vertices and SEQEND |
| a block | its BLOCK and ENDBLK and every entity in it |
| a table record | its extension dictionary, and its place in its control's list |
| a dictionary or named object | what it owns, and its entry in the dictionary that names it |

Groups, draw orders, a block's list of inserts, a layout's list of viewports and the reactors of every object drop what is removed. A dimension association bound to a removed entity is removed too, and a hatch made from one loses its boundary and is no longer associative.

A removal is refused, and the drawing left unchanged, when the object is one the drawing needs or one that can only be removed with its owner, or when something left behind still names it. `What` says which:

```cpp
const auto tryRemove = [&drawing](std::string_view handle) -> void
{
  const auto removed = DwgIO::Edit::Remove(drawing, DwgIO::ParseHandle(handle).value());
  std::println("{}: {}", handle, removed ? "removed" : removed.error().What);
};
tryRemove("2F1");
tryRemove("2E9");
tryRemove("2CF");
tryRemove("10");
tryRemove("1F");
tryRemove("999");
```

```text [Output]
2F1: ATTRIB 2F1 belongs to INSERT 2EF
2E9: block Tag in use
2CF: layer Walls in use
10: layer 0 required
1F: block *Model_Space required
999: handle 999 missing
```

Remove the inserts first, then the block. With the three inserts removed, nothing names `Tag`, and it is removed with its BLOCK, ENDBLK and the four entities in it:

```cpp
const std::size_t before = drawing.Objects.size();
if (!DwgIO::Edit::Remove(drawing, insertHandle) || !DwgIO::Edit::Remove(drawing, *newInsertHandle) ||
    !DwgIO::Edit::Remove(drawing, *insertCopyHandle))
{
  return 1;
}
const std::size_t inserts = before - drawing.Objects.size();
tryRemove("2E9");
std::println("{} objects for the inserts, {} for the block", inserts, before - inserts - drawing.Objects.size());
```

```text [Output]
2E9: removed
9 objects for the inserts, 7 for the block
```

The refusals list the records AutoCAD will not let go of: a table's control object, the root dictionary, the two spaces and the layouts' blocks, layer `0`, the linetypes ByBlock, ByLayer and Continuous, the Standard text and dimension styles, the application `ACAD`, the viewport `*Active`. A block also stays while a dimension style or the header names it as an arrow.

## Edit an unknown object {#unknown}

A change to an `Unknown`'s `Base` or `Common` reaches the file only once its `Bytes` are cleared, as [Unknown objects](/guide/objects/#unknown) explains. Add extended data to the section view style of `foundation-r2000.dwg`, which dwgio does not type, once with the bytes kept and once with them cleared:

```cpp
auto foundation = DwgIO::LoadFile("foundation-r2000.dwg");
if (!foundation)
{
  return 1;
}
const DwgIO::Handle style = DwgIO::ParseHandle("21A").value();
const auto recordsAfterSave = [&foundation, style](bool cleared) -> std::size_t
{
  DwgIO::Drawing changed = foundation->Drawing;
  auto &unknown = DwgIO::Get<DwgIO::Unknown>(*DwgIO::Find(changed, style));
  const DwgIO::Handle acad = DwgIO::View::Record<DwgIO::Tables::AppId>(changed, "ACAD")->Record.Base.Self;
  unknown.Base.Extended.push_back(
      DwgIO::Xdata{.Application = "ACAD", .AppId = acad, .Tags = {{.Code = 1000, .Value = std::string{"checked"}}}});
  if (cleared)
  {
    unknown.Bytes.clear();
  }
  const auto reloaded = DwgIO::Load(DwgIO::Save(changed)->Bytes);
  return DwgIO::BaseOf(*DwgIO::Find(reloaded->Drawing, style)).Extended.size();
};
std::println("bytes kept: {} records; bytes cleared: {} records", recordsAfterSave(false), recordsAfterSave(true));
```

```text [Output]
bytes kept: 0 records; bytes cleared: 1 records
```

`Edit::Remove` clears the bytes itself when it drops a link from an unknown object to what it removes.

## A new drawing {#made}

A drawing from `Create` edits the same way. Move a line to another layer; the first layer is then unused and can be removed:

```cpp
DwgIO::Drawing layersDrawing = DwgIO::Create();
DwgIO::Tables::Layer temporaryLayer;
temporaryLayer.Record.Name = "Temporary";
temporaryLayer.Linetype = layersDrawing.Header.LTypeContinuous;
DwgIO::Tables::Layer wallLayer;
wallLayer.Record.Name = "Walls";
wallLayer.Linetype = layersDrawing.Header.LTypeContinuous;
const auto temporaryHandle = DwgIO::Edit::Add(layersDrawing, temporaryLayer);
const auto wallLayerHandle = DwgIO::Edit::Add(layersDrawing, wallLayer);
DwgIO::Entities::Line wall;
wall.Common.Layer = temporaryHandle.value_or(DwgIO::Handle{});
wall.End = DwgIO::Vector{.X = 4, .Y = 3, .Z = 0};
const auto wallHandle = DwgIO::Edit::Add(layersDrawing, wall);
if (!temporaryHandle || !wallLayerHandle || !wallHandle)
{
  return 1;
}
std::println("{}", DwgIO::Edit::Remove(layersDrawing, *temporaryHandle).error().What);
DwgIO::CommonOf(*DwgIO::Find(layersDrawing, *wallHandle))->Layer = *wallLayerHandle;
if (!DwgIO::Edit::Remove(layersDrawing, *temporaryHandle))
{
  return 1;
}
std::print("layers");
for (const DwgIO::Tables::Layer *layer : DwgIO::View::Records<DwgIO::Tables::Layer>(layersDrawing))
{
  std::print(" {}", layer->Record.Name);
}
std::println("");
```

```text [Output]
layer Temporary in use
layers 0 Walls
```

## Check and save {#save}

`Edit` keeps the links it touches, but a change in place is not checked: a layer handle that names nothing is only found by `Audit::Check`, which [Auditing a drawing](/tutorials/auditing/) covers. Check both drawings before saving them.

```cpp
for (const auto &[file, edited] : {std::pair{"edited.dwg", &drawing}, std::pair{"layers.dwg", &layersDrawing}})
{
  const DwgIO::Audit::Report report = DwgIO::Audit::Check(*edited);
  std::println("{}: {} findings", file, report.Findings.size());
  if (const auto saved = DwgIO::SaveFile(*edited, file); !saved || !report.IsClean())
  {
    return 1;
  }
}
```

```text [Output]
edited.dwg: 0 findings
layers.dwg: 0 findings
```
