# Blocks and attributes

A block is a block record with the entities it owns, framed by a BLOCK and an ENDBLK. An INSERT places a block: at a point, scaled and turned, with the ATTRIB values its attribute definitions ask for. The examples build a drawing with one block and one insert, then read it back as any drawing is read.

## A block with an attribute {#build}

`Edit::AddBlock` makes the record with its BLOCK and ENDBLK; `Edit::Add` adds entities to it by name. An ATTDEF is an entity of the block like any other: it holds the tag, the prompt and the default value.

```cpp
DwgIO::Drawing drawing = DwgIO::Create(DwgIO::Version::R2004);
const auto door = DwgIO::Edit::AddBlock(drawing, "Door");
DwgIO::Entities::Line doorLine;
doorLine.End = DwgIO::Vector{.X = 1, .Y = 0, .Z = 0};
DwgIO::Entities::AttDef number;
number.Tag = "NUMBER";
number.Prompt = "Door number";
number.Lettering.Value = "000";
number.Lettering.Height = 0.25;
number.Lettering.Position = DwgIO::Vector{.X = 0, .Y = -0.5, .Z = 0};
if (!door || !DwgIO::Edit::Add(drawing, doorLine, "Door") || !DwgIO::Edit::Add(drawing, number, "Door"))
{
  return 1;
}
```

Adding the ATTDEF sets `Attributes` on the record, which says the block holds attribute definitions, as the 2 bit of the BLOCK's group 70 does in DXF.

## An insert with its attribute {#insert}

The insert names the block record by handle and places the block's base point at `Position`, scaled by `Scale` and turned by `Rotation`, in radians. `Edit::Add` lists it in the block record's `Inserts`, from R2000.

```cpp
DwgIO::Entities::Insert doorInsert;
doorInsert.Block = *door;
doorInsert.Position = DwgIO::Vector{.X = 10, .Y = 5, .Z = 0};
doorInsert.Scale = DwgIO::Vector{.X = 2, .Y = 2, .Z = 2};
doorInsert.Rotation = DwgIO::Turn / 4;
const auto insertHandle = DwgIO::Edit::Add(drawing, doorInsert);
if (!insertHandle)
{
  return 1;
}
```

An ATTRIB belongs to its insert, not to a space. `Edit::AddAttribute` appends one to the insert: it gives the attribute a handle, the insert as owner, the insert's layer and the current text style when it names none, and makes the SEQEND that closes the insert's attributes. The insert's `Follows` then says ATTRIBs follow it.

```cpp
DwgIO::Entities::Attrib value;
value.Tag = number.Tag;
value.Lettering = number.Lettering;
value.Lettering.Value = "101";
value.Lettering.Height = 0.5;
value.Lettering.Position = DwgIO::Vector{.X = 11, .Y = 5, .Z = 0};
value.Lettering.Rotation = DwgIO::Turn / 4;
if (!DwgIO::Edit::AddAttribute(drawing, *insertHandle, value) || !DwgIO::SaveFile(drawing, "doors.dwg") ||
    !DwgIO::SaveFile(drawing, "doors-r2000.dwg", DwgIO::SaveOptions{.Release = DwgIO::Version::R2000}))
{
  return 1;
}
```

The attribute stands where the insert puts the definition: `(0, -0.5)` scaled by 2 is `(0, -1)`, a quarter turn takes it to `(1, 0)`, and the position adds `(10, 5)`. Its height and rotation are the definition's, scaled and turned the same way. AutoCAD draws the ATTRIB where it is stored; it does not place it again from the ATTDEF.

## Read an insert {#read}

Load the drawing and walk model space. `Insert::Block` is the handle of the block record; `Find` gives it.

```cpp
const auto loaded = DwgIO::LoadFile("doors.dwg");
if (!loaded)
{
  return 1;
}
const DwgIO::Drawing &reloaded = loaded->Drawing;
const DwgIO::Tables::BlockRecord *model = DwgIO::View::ModelSpace(reloaded);
const auto entities = DwgIO::View::Entities(reloaded, *model);
const auto found = std::ranges::find_if(entities, [](const DwgIO::Object *entity)
                                        { return DwgIO::Holds<DwgIO::Entities::Insert>(*entity); });
const auto &insert = DwgIO::Get<DwgIO::Entities::Insert>(**found);
const auto &block = DwgIO::Get<DwgIO::Tables::BlockRecord>(*DwgIO::Find(reloaded, insert.Block));
std::println("{} at ({}, {}), scale {}, rotated {} radians", block.Record.Name, insert.Position.X,
             insert.Position.Y, insert.Scale.X, insert.Rotation);
```

```text [Output]
Door at (10, 5), scale 2, rotated 1.5707963267948966 radians
```

`Insert::Columns` holds a value for a MINSERT, a grid of `Columns` by `Rows` copies spaced `ColumnSpacing` and `RowSpacing`; it is empty for a plain INSERT.

## The block's entities {#entities}

`View::Entities` gives the block's entities in the order they are drawn, the ATTDEF among them, never the BLOCK and ENDBLK. `Geometry::TransformOf` carries a point of the block to the world: its base point taken off, then scaled, turned and moved to `Position`, then out of the insert's object coordinates:

```cpp
const DwgIO::Matrix transform = DwgIO::Geometry::TransformOf(insert, block);
for (const DwgIO::Object *entity : DwgIO::View::Entities(reloaded, block))
{
  if (const auto *line = DwgIO::GetIf<DwgIO::Entities::Line>(entity))
  {
    const DwgIO::Vector end = DwgIO::Geometry::Transform(transform, line->End);
    std::println("LINE ends at ({}, {})", end.X, end.Y);
  }
  if (const auto *definition = DwgIO::GetIf<DwgIO::Entities::AttDef>(entity))
  {
    const DwgIO::Vector at = DwgIO::Geometry::Transform(transform, definition->Lettering.Position);
    std::println("ATTDEF {} asks \"{}\", default {}, placed at ({}, {})", definition->Tag, definition->Prompt,
                 definition->Lettering.Value, at.X, at.Y);
  }
}
```

```text [Output]
LINE ends at (10, 7)
ATTDEF NUMBER asks "Door number", default 000, placed at (11, 5)
```

`Position` is in the insert's object coordinate system, which its `Extrusion` sets, and the transform takes it to the world as [Coordinates and units](/tutorials/coordinates/#arbitrary) explains. A block placed inside another block nests the same way: `Geometry::Compose(outer, inner)` applies the inner insert, then the outer.

## The insert's attributes {#attributes}

From R2004 an insert lists its attributes in `Owned`. Up to R2000 it names the first and the last, `First` and `Last`, and each attribute names the next in `Common::Next`. Either way the SEQEND that closes them is `SeqEnd`. `View::Attributes` reads both:

```cpp
for (const char *file : {"doors.dwg", "doors-r2000.dwg"})
{
  const auto doors = DwgIO::LoadFile(file);
  if (!doors)
  {
    return 1;
  }
  for (const DwgIO::Object &object : doors->Drawing.Objects)
  {
    if (const auto *insert = DwgIO::GetIf<DwgIO::Entities::Insert>(&object))
    {
      for (const DwgIO::Entities::Attrib *attribute : DwgIO::View::Attributes(doors->Drawing, *insert))
      {
        std::println("{}: {} = {} at ({}, {}), {} listed, first {}", DwgIO::NameOf(doors->Drawing.Release),
                     attribute->Tag, attribute->Lettering.Value, attribute->Lettering.Position.X,
                     attribute->Lettering.Position.Y, insert->Owned.size(), DwgIO::HexOf(insert->First));
      }
    }
  }
}
```

```text [Output]
R2004: NUMBER = 101 at (11, 5), 1 listed, first 0
R2000: NUMBER = 101 at (11, 5), 0 listed, first 31
```

An insert whose block has no attribute definitions has neither, and `Follows` false. The ATTRIB keeps its own tag, so match it to the ATTDEF by tag, not by order.

## Anonymous blocks {#anonymous}

A block AutoCAD makes for its own use is anonymous: `Anonymous` is set and the name starts with `*`. A dimension is drawn through one named `*D`, a table through one named `*T`, and other generated blocks are named `*U`. The file stores the name without a number; AutoCAD's DXF numbers them `*D1`, `*D2` and so on. Several records can bear the same name, so reach them by handle, such as each dimension's `Block`. `curves-r2000.dwg` holds nine dimensions.

```cpp
const auto curves = DwgIO::LoadFile("curves-r2000.dwg");
if (!curves)
{
  return 1;
}
for (const DwgIO::Object &object : curves->Drawing.Objects)
{
  if (const auto *dimension = DwgIO::GetIf<DwgIO::Entities::Dimension>(&object))
  {
    const auto &record = DwgIO::Get<DwgIO::Tables::BlockRecord>(*DwgIO::Find(curves->Drawing, dimension->Block));
    std::println("{} drawn by {} {}, anonymous {}, {} entities", DwgIO::TypeOf(object), record.Record.Name,
                 DwgIO::HexOf(record.Record.Base.Self), record.Anonymous,
                 DwgIO::View::Entities(curves->Drawing, record).size());
  }
}
```

```text [Output]
DIMENSION_LINEAR drawn by *D 39F, anonymous true, 9 entities
DIMENSION_LINEAR drawn by *D 3AC, anonymous true, 11 entities
DIMENSION_ALIGNED drawn by *D 3BB, anonymous true, 9 entities
DIMENSION_ANG3PT drawn by *D 3C8, anonymous true, 12 entities
DIMENSION_ANG2LN drawn by *D 3DA, anonymous true, 9 entities
DIMENSION_RADIUS drawn by *D 3E9, anonymous true, 6 entities
DIMENSION_DIAMETER drawn by *D 3F4, anonymous true, 8 entities
DIMENSION_ORDINATE drawn by *D 400, anonymous true, 5 entities
DIMENSION_ORDINATE drawn by *D 409, anonymous true, 5 entities
```

`View::Block(drawing, "*D")` gives only the first of them. The blocks of the model and paper spaces are not anonymous, though their names start with `*` too.

## External references {#xrefs}

A block record with `Xref` set is an external reference: `Path` names the drawing it shows, `Overlaid` says it is an overlay, not an attachment, and from R2000 `Loaded` says whether it was loaded when the file was saved. The record owns no entities; they are in the other file, which dwgio does not open. A table record that came from an external reference, such as a layer named `Plan|Walls`, has `Dependent` set and its `Source` naming the external reference's block record.

```cpp
for (const DwgIO::Tables::BlockRecord *record : DwgIO::View::Records<DwgIO::Tables::BlockRecord>(reloaded))
{
  if (record->Xref)
  {
    std::println("{} refers to {}", record->Record.Name, record->Path);
  }
}
```
