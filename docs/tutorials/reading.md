# Reading a drawing

A DWG file is a list of objects, each known by a handle: layers and linetypes, block records, dictionaries, and the entities that are drawn. `Load` reads them all. The table records, their control objects, the entities and the named objects come back typed. Any other object comes back as an `Unknown`, with the data every object shares read and its own data kept as bytes.

## Load {#load}

`Load` takes the bytes of a file or a stream; `LoadFile` takes a path.

```cpp
const auto loaded = DwgIO::LoadFile("foundation-r2000.dwg");
if (!loaded)
{
  std::println("{}", loaded.error().What);
  return 1;
}
const DwgIO::Drawing &drawing = loaded->Drawing;
std::println("{}, code page {}", DwgIO::AcadverOf(drawing.Release), drawing.Codepage);
std::println("{} classes, {} objects, {} preview images", drawing.Classes.size(),
             drawing.Objects.size(), drawing.Preview.size());
```

```text [Output]
AC1015, code page 30
17 classes, 219 objects, 2 preview images
```

## The objects {#objects}

`Drawing::Objects` holds every object in file order. `TypeOf` names an object's type: the DXF name of a built-in type, such as `LINE`, `LAYER` or `DICTIONARY`, or the DXF name of its class for a type the drawing declares, such as `VISUALSTYLE` or `SCALE`. `BaseOf` gives what every object carries: its handle, its owner, its extension dictionary, its reactors and its extended data.

```cpp
std::map<std::string_view, int> counts;
for (const DwgIO::Object &object : drawing.Objects)
{
  counts[DwgIO::TypeOf(object)] += 1;
}
for (const auto &[type, count] : counts)
{
  if (count > 10)
  {
    std::println("{:>3} {}", count, type);
  }
}
```

```text [Output]
 55 DICTIONARY
 18 DICTIONARYVAR
 17 SCALE
 24 VISUALSTYLE
 49 XRECORD
```

Owners tie the objects together: a layer is owned by the layer control object, a dictionary by its parent dictionary, an entity by the block record it is drawn in. The header names the roots: `Header::DictionaryNamedObjects` is the root dictionary and `Header::BlockRecordModelSpace` is model space.

## Entities {#entities}

Each entity kind is a struct: an `Entities::Line` holds two points, an `Entities::Circle` holds a center and a radius, an `Entities::Text` holds a `Lettering`, an `Entities::LwPolyline` holds its vertices, and an `Entities::Dimension` holds a `Measure` and the form of its kind. The [Entities](/entities/curves/) pages say what each kind holds and how to read it. Each opens with its `Base` and its `Common`: the layer, color, linetype scale, visibility, which linetype it draws with, from R2000 the lineweight and plot style, and the links to its neighbours. An entity in model or paper space has its owner filled in from the header, and `Common::Space` says which space. [Coordinates and units](/tutorials/coordinates/#ocs) says which points are in world coordinates and which in the entity's own, and [Layers, colors and styles](/tutorials/styles/#resolve) resolves the color, linetype and lineweight it is drawn with.

An `Object` holds its kind in a box of its own, so it takes only that kind's size. `GetIf` gives an object as a kind, or null when it is another kind or there is no object. `Get`, `Holds` and `Visit` work like `std::get`, `std::holds_alternative` and `std::visit`, and pass the kind itself, not the box.

`View::Entities` gives a block's entities in drawing order; `View::Block` finds a block by name, model space included. [Model space and layouts](/tutorials/spaces/) reads the spaces and their viewports, and [Blocks and attributes](/tutorials/blocks/) the inserts and their attributes.

```cpp
const DwgIO::Tables::BlockRecord *model = DwgIO::View::ModelSpace(drawing);
for (const DwgIO::Object *entity : DwgIO::View::Entities(drawing, *model))
{
  if (const auto *line = DwgIO::GetIf<DwgIO::Entities::Line>(entity))
  {
    std::println("line {} on layer {} from ({}, {}) to ({}, {})", DwgIO::HexOf(line->Base.Self),
                 DwgIO::HexOf(line->Common.Layer), line->Start.X, line->Start.Y, line->End.X, line->End.Y);
  }
}
```

```text [Output]
line 2CC on layer 10 from (0, 0) to (100, 50)
```

An entity of a kind dwgio does not type is an `Unknown` with `Entity` set, its `Common` read, and its bytes kept unchanged from the object's size to its CRC. `CommonOf` gives the common data of any entity, typed or not, and null for an object that is not an entity. A save to the same release writes those bytes back unchanged.

## Tables {#tables}

The table records are typed: a `Tables::Layer` holds its states, color, lineweight and linetype, a `Tables::LType` holds its dashes, and a `Tables::DimStyle` holds one member per DIM variable. Styles, views, coordinate systems, viewports, applications and block records are typed the same way. Each opens with a `Tables::Record` holding its name and handles. Each table's control object lists its records: `View::Records` gives them in that order, and `View::Record` finds one by name, ignoring case. [Layers, colors and styles](/tutorials/styles/#layers) reads the layers, linetypes and styles further.

```cpp
for (const DwgIO::Tables::Layer *layer : DwgIO::View::Records<DwgIO::Tables::Layer>(drawing))
{
  std::println("layer {} color {} linetype {} {}", layer->Record.Name, layer->Color.Index,
               DwgIO::HexOf(layer->Linetype), layer->On ? "on" : "off");
}
```

```text [Output]
layer 0 color 7 linetype 16 on
```

## Named objects {#objects-named}

The named objects are typed too: an `Objects::Dictionary` holds its entries, each a name and a handle, an `Objects::XRecord` holds an application's DXF groups, and an `Objects::Layout` holds a tab with its `Plot` settings. Groups, multiline styles, scales, sort tables, placeholders and variables are typed the same way. `View::Root` gives the root dictionary, which `Header::DictionaryNamedObjects` names; `View::Dictionary` gives one of its dictionaries by name, `View::Entry` an entry of any dictionary, and `View::Layouts` the layouts in tab order, which [Model space and layouts](/tutorials/spaces/#layouts) reads further.

```cpp
std::println("{} entries in the root, {} in ACAD_LAYOUT", DwgIO::View::Root(drawing)->Entries.size(),
             DwgIO::View::Dictionary(drawing, "ACAD_LAYOUT")->Entries.size());
for (const DwgIO::Objects::Layout *layout : DwgIO::View::Layouts(drawing))
{
  std::println("{} tab {} on block {}", layout->Name, layout->Order, DwgIO::HexOf(layout->Block));
}
```

```text [Output]
18 entries in the root, 3 in ACAD_LAYOUT
Model tab 0 on block 1F
Layout1 tab 1 on block D2
Layout2 tab 2 on block D6
```

## Extended data {#xdata}

Extended data comes as `Xdata` records, one per application. Each names its APPID record by handle and holds its groups under their DXF codes: 1000 for text, 1070 for a short, 1005 for a handle.

```cpp
int records = 0;
std::string first;
for (const DwgIO::Object &object : drawing.Objects)
{
  for (const DwgIO::Xdata &record : DwgIO::BaseOf(object).Extended)
  {
    ++records;
    if (const auto *text = std::get_if<std::string>(&record.Tags.front().Value); text && first.empty())
    {
      first = std::format("{} from {}: {}", DwgIO::TypeOf(object), DwgIO::HexOf(record.AppId), *text);
    }
  }
}
std::println("{} records, the first with text: {}", records, first);
```

```text [Output]
36 records, the first with text: VISUALSTYLE from 12: AcDbSavedByObjectVersion
```

## Issues {#issues}

A file AutoCAD wrote loads with no issues; a damaged one loads as far as it can. [Errors and issues](/guide/errors/#issues) lists what a load notes and shows it on a damaged file.

```cpp
for (const DwgIO::Issue &issue : loaded->Issues)
{
  std::println("byte {}, object {}: {}", issue.Offset.value_or(0), DwgIO::HexOf(issue.Subject), issue.What);
}
```
