# The object model

A `Drawing` is a DWG file as values: its release, its header variables, its classes and every object it holds, in file order. Each object is one of the kinds dwgio types, such as a layer, a line or a dictionary, or an `Unknown` that keeps the bytes of a kind it does not. Objects point to each other by handle, and each has an owner.

## The drawing {#drawing}

| Member | Holds |
| :--- | :--- |
| `Release` | The release the file was in, which a save writes unless told another |
| `Codepage`, `Maintenance` | From the file header: the code page of the text before R2007, and the maintenance version |
| `Header` | The header variables, one member each, such as `Insunits`, `Handseed` or `BlockRecordModelSpace` |
| `Classes` | The classes the file declares, which number the object types from 500 up |
| `Objects` | Every object, in file order |
| `Preview` | The preview images |
| `SummaryInfo`, `FileDependencies` | The drawing properties and the file dependencies, when the file has them |
| `Order`, `Kept` | The sections of the file in their order, and those dwgio keeps as bytes |

```cpp
auto loaded = DwgIO::LoadFile("foundation-r2000.dwg");
if (!loaded)
{
  std::println("{}", loaded.error().What);
  return 1;
}
DwgIO::Drawing &drawing = loaded->Drawing;
std::println("{}: {} classes, {} objects, next handle {}", DwgIO::NameOf(drawing.Release), drawing.Classes.size(),
             drawing.Objects.size(), DwgIO::HexOf(drawing.Header.Handseed));
```

```text [Output]
R2000: 17 classes, 219 objects, next handle 417
```

A `Drawing` is a plain value: a copy copies every object, and `==` compares every member. [The drawing](/concepts/drawing/#blocks) shows where each member comes from in the file.

## Objects {#objects}

`Object` is a `std::variant` with one alternative per kind: `Boxed<Unknown>` first, then `Boxed<Tables::Layer>`, `Boxed<Entities::Line>`, `Boxed<Objects::Dictionary>` and so on. A `Boxed` keeps its kind on the heap and copies and compares it as a value, so each object takes the room of its own kind rather than of the largest. A kind converts to an `Object`, and a default `Object` is an empty `Unknown`.

Four functions read an object as its kind, seeing through the box:

| Function | Gives |
| :--- | :--- |
| `Holds<KindT>(object)` | Whether the object is of that kind, as `std::holds_alternative` |
| `Get<KindT>(object)` | The kind, as `std::get`; throws `std::bad_variant_access` for another kind |
| `GetIf<KindT>(pointer)` | A pointer to the kind, or null for another kind or a null pointer, as `std::get_if` |
| `Visit(function, object)` | What the function returns for the kind, as `std::visit` |

```cpp
for (const DwgIO::Object &object : drawing.Objects)
{
  if (DwgIO::Holds<DwgIO::Entities::Circle>(object))
  {
    const DwgIO::Entities::Circle &circle = DwgIO::Get<DwgIO::Entities::Circle>(object);
    std::println("circle at ({}, {}), radius {}", circle.Center.X, circle.Center.Y, circle.Radius);
  }
  if (const auto *arc = DwgIO::GetIf<DwgIO::Entities::Arc>(&object))
  {
    std::println("arc at ({}, {}), radius {}, from {} to {} radians", arc->Center.X, arc->Center.Y, arc->Radius,
                 arc->Start, arc->End);
  }
}
```

```text [Output]
circle at (50, 50), radius 25
arc at (10, 0), radius 10, from 0 to 3.141592653589793 radians
```

`Visit` calls a function with the kind itself, so a generic lambda can ask what the kind has. Every table record opens with a `Record`, and every entity has a `Common`; an `Unknown` has a `Common` too, filled in only when it is an entity, so it is tested first.

```cpp
std::map<std::string_view, int> counts;
for (const DwgIO::Object &object : drawing.Objects)
{
  const std::string_view category = DwgIO::Visit(
      [](const auto &kind) -> std::string_view
      {
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(kind)>, DwgIO::Unknown>)
        {
          return "untyped";
        }
        else if constexpr (requires { kind.Record.Name; })
        {
          return "table records";
        }
        else if constexpr (requires { kind.Common; })
        {
          return "entities";
        }
        else
        {
          return "other objects";
        }
      },
      object);
  counts[category] += 1;
}
for (const auto &[category, count] : counts)
{
  std::println("{:>3} {}", count, category);
}
```

```text [Output]
 10 entities
186 other objects
 21 table records
  2 untyped
```

The other objects are the table controls, the dictionaries and the other named objects.

## What every object has {#base}

Three functions read any object without knowing its kind:

- `TypeOf` names its type as DXF does: `LINE`, `LAYER`, `DICTIONARY`, or the DXF name of its class, such as `SCALE`.
- `BaseOf` gives its `Base`: its handle `Self`, its `Owner`, its extension `Dictionary`, its `Reactors` and its `Extended` data.
- `CommonOf` gives an entity's `Common`, typed or not: its layer, color, linetype, lineweight, the space it is in. It is null for an object that is not an entity.

Read four objects by their handles, with `Find` and `ParseHandle` from the [next section](#handles): the root dictionary, layer 0, a line, and an object dwgio does not type.

```cpp
for (const char *hex : {"C", "10", "2CC", "21A"})
{
  const DwgIO::Object &object = *DwgIO::Find(drawing, DwgIO::ParseHandle(hex).value());
  const DwgIO::Base &base = DwgIO::BaseOf(object);
  const DwgIO::Common *common = DwgIO::CommonOf(object);
  std::println("{} {}: owner {}, reactors {}, {}", DwgIO::TypeOf(object), DwgIO::HexOf(base.Self),
               DwgIO::HexOf(base.Owner), base.Reactors.size(),
               common == nullptr ? "not an entity" : "on layer " + DwgIO::HexOf(common->Layer));
}
```

```text [Output]
DICTIONARY C: owner 0, reactors 0, not an entity
LAYER 10: owner 2, reactors 0, not an entity
LINE 2CC: owner 1F, reactors 0, on layer 10
ACDBSECTIONVIEWSTYLE 21A: owner 219, reactors 1, not an entity
```

## Handles {#handles}

A `Handle` is the number an object is known by and that other objects point to it with; the null handle, zero, names nothing and tests false. `HexOf` writes a handle as DXF does and `ParseHandle` reads one back. `Find` gives the object a handle names, or null, and `GetIf` takes that pointer directly, so a lookup and a test of the kind read as one step.

```cpp
const DwgIO::Object *found = DwgIO::Find(drawing, DwgIO::ParseHandle("2CC").value());
const auto *line = DwgIO::GetIf<DwgIO::Entities::Line>(found);
const auto *layer = DwgIO::GetIf<DwgIO::Tables::Layer>(DwgIO::Find(drawing, line->Common.Layer));
std::println("{} {} is on layer {}", DwgIO::TypeOf(*found), DwgIO::HexOf(line->Base.Self), layer->Record.Name);
```

```text [Output]
LINE 2CC is on layer 0
```

`Find` on a drawing you may change gives a pointer you may change through. It looks through `Objects` from the start each time; to look up many handles, build an `Index` once: it finds each handle in constant time, and every `View` function takes it in place of the drawing.

```cpp
const DwgIO::Index index{drawing};
const auto *circle = DwgIO::GetIf<DwgIO::Entities::Circle>(DwgIO::Find(index, DwgIO::Handle{.Value = 0x2CD}));
std::println("radius {}, {} entities in model space", circle->Radius,
             DwgIO::View::Entities(index, *DwgIO::View::ModelSpace(index)).size());
```

```text [Output]
radius 25, 4 entities in model space
```

A pointer into `Objects`, from `Find`, from `View` or from an `Index`, stays valid until the vector changes: an object added or erased, or an `Edit` call that adds or removes. The same holds for an `Index` itself, which must be built again after such a change.

## Owners {#owners}

Every object but the roots (the table controls and the root dictionary) has an owner in `Base::Owner`:

- a table record is owned by its table's control object, such as `LAYER_CONTROL` for a layer;
- a dictionary or an entry of one is owned by the dictionary it is in, up to the root, which `Header::DictionaryNamedObjects` names;
- an extension dictionary is owned by the object it extends, whose `Base::Dictionary` names it;
- an entity is owned by the block record it is drawn in: model space, a paper space, or a block.

The header names the block records of the two spaces, `BlockRecordModelSpace` and `BlockRecordPaperSpace`. A block's entities lie anywhere in `Objects`; the block record says their order, and `View::Entities` follows it. The `BLOCK` and `ENDBLK` that frame a block are owned by its record too, but are not among its entities.

```cpp
const DwgIO::Handle model = drawing.Header.BlockRecordModelSpace;
std::print("owned by model space:");
for (const DwgIO::Object &object : drawing.Objects)
{
  if (DwgIO::BaseOf(object).Owner == model)
  {
    std::print(" {}", DwgIO::TypeOf(object));
  }
}
std::print("\ndrawn in model space:");
const auto &record = DwgIO::Get<DwgIO::Tables::BlockRecord>(*DwgIO::Find(drawing, model));
for (const DwgIO::Object *entity : DwgIO::View::Entities(drawing, record))
{
  std::print(" {}", DwgIO::TypeOf(*entity));
}
std::println("");
```

```text [Output]
owned by model space: LINE CIRCLE ARC LWPOLYLINE ENDBLK DICTIONARY BLOCK
drawn in model space: LINE CIRCLE ARC LWPOLYLINE
```

The `DICTIONARY` is the model space record's extension dictionary.

`Common::Space` says how the file places an entity: 2 in model space, 1 in the paper space the header names, 0 anywhere else, a block or another layout, where the file stores the owner. For the first two the file leaves the owner out, and `Load` fills it in from the header, so `Owner` is set for every entity. To add an entity with its owner, space and place in the order set, use [`Edit::Add`](/tutorials/creating/#entities) rather than pushing it onto `Objects`.

## Unknown objects {#unknown}

An object of a kind dwgio does not type loads as an `Unknown`. It holds its type code and, for a type of 500 and up, its class's DXF name; whether it is an entity; its `Base`, and its `Common` when it is an entity; its `Bytes` as the file stored them; and `Body`, the bits of its own data after the header.

```cpp
for (const DwgIO::Object &object : drawing.Objects)
{
  if (const auto *unknown = DwgIO::GetIf<DwgIO::Unknown>(&object))
  {
    std::println("{} {}: type {}, {} bytes, owned by {}", DwgIO::TypeOf(object), DwgIO::HexOf(unknown->Base.Self),
                 unknown->Type, unknown->Bytes.size(), DwgIO::HexOf(unknown->Base.Owner));
  }
}
```

```text [Output]
ACDBSECTIONVIEWSTYLE 21A: type 515, 288 bytes, owned by 219
ACDBDETAILVIEWSTYLE 21C: type 516, 145 bytes, owned by 21B
```

With an `Unknown` you can:

- read its type, its handles, its owner, its reactors, its extended data and, for an entity, its layer and the other members of its `Common`;
- keep it: a save to its own release writes its original bytes, and a save to another release writes its header in that release's form followed by its own bits, where AutoCAD writes its class the same way in both releases; otherwise the save leaves it out and notes an issue;
- remove it with `Edit::Remove`, which takes what it owns with it and drops it from the dictionary that names it.

You cannot read or change its own data. To change its `Base` or `Common`, clear its `Bytes` as well: a save to its own release then writes its header again from them, followed by its own bits. `Edit::Remove` does this for you when it drops a link to what it removes. An unknown object without `Body` keeps its bytes, and any change to its header is lost.

Remove the section view style. Its extension dictionary and the record in it are removed with it, three objects in total, and the dictionary that held it is left empty:

```cpp
const DwgIO::Handle style = DwgIO::ParseHandle("21A").value();
const DwgIO::Handle owner = DwgIO::BaseOf(*DwgIO::Find(drawing, style)).Owner;
if (const auto removed = DwgIO::Edit::Remove(drawing, style); !removed)
{
  std::println("{}", removed.error().What);
  return 1;
}
std::println("{} objects, {} entries left in {}", drawing.Objects.size(),
             DwgIO::Get<DwgIO::Objects::Dictionary>(*DwgIO::Find(drawing, owner)).Entries.size(), DwgIO::HexOf(owner));
if (const auto saved = DwgIO::SaveFile(drawing, "styles.dwg"); !saved)
{
  std::println("{}", saved.error().What);
  return 1;
}
```

```text [Output]
216 objects, 0 entries left in 219
```
