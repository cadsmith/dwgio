# Creating a drawing

`Create` returns the drawing AutoCAD starts with: its tables, the two spaces, the layouts and the named objects, with every handle and root set. `Edit` adds to it, or to any loaded drawing, and keeps every link consistent. Each addition returns the handle of what it added, or a `Failure` saying why it was refused.

## Create {#create}

```cpp
DwgIO::Drawing drawing = DwgIO::Create();
std::println("{} objects, {} classes, next handle {}", drawing.Objects.size(), drawing.Classes.size(),
             DwgIO::HexOf(drawing.Header.Handseed));
for (const DwgIO::Objects::Layout *layout : DwgIO::View::Layouts(drawing))
{
  std::println("{} on block {}", layout->Name, DwgIO::HexOf(layout->Block));
}
```

```text [Output]
35 objects, 3 classes, next handle 2B
Model on block 1F
Layout1 on block 23
```

## A layer and a block {#tables}

A table record is added only under a name no other record of its table has, ignoring case. A block is made with its BLOCK and ENDBLK, and entities are added to it by name.

```cpp
DwgIO::Tables::Layer walls;
walls.Record.Name = "Walls";
walls.Color.Index = 1;
walls.Linetype = drawing.Header.LTypeContinuous;
const auto layer = DwgIO::Edit::Add(drawing, walls);
const auto block = DwgIO::Edit::AddBlock(drawing, "Chair");
DwgIO::Entities::Circle circle;
circle.Radius = 0.25;
const auto circleHandle = DwgIO::Edit::Add(drawing, circle, "Chair");
if (!layer || !block || !circleHandle)
{
  return 1;
}
std::println("layer {}, block {} holding {}", DwgIO::HexOf(*layer), DwgIO::HexOf(*block), DwgIO::HexOf(*circleHandle));
std::println("{}", DwgIO::Edit::Add(drawing, walls).error().What);
```

```text [Output]
layer 2B, block 2C holding 2F
layer Walls exists
```

## Entities in a space {#entities}

An entity added to a space gets a handle, an owner, its space, the current layer when it names none, and a place at the end of the block's chain, which `View::Entities` follows.

```cpp
DwgIO::Entities::Line wall;
wall.Common.Layer = *layer;
wall.End = DwgIO::Vector{.X = 10, .Y = 0, .Z = 0};
DwgIO::Entities::Insert chairInsert;
chairInsert.Block = *block;
chairInsert.Position = DwgIO::Vector{.X = 5, .Y = 1, .Z = 0};
if (!DwgIO::Edit::Add(drawing, wall) || !DwgIO::Edit::Add(drawing, chairInsert, DwgIO::Space::Model))
{
  return 1;
}
const DwgIO::Tables::BlockRecord *model = DwgIO::View::ModelSpace(drawing);
for (const DwgIO::Object *entity : DwgIO::View::Entities(drawing, *model))
{
  std::println("{} {} on layer {}", DwgIO::TypeOf(*entity), DwgIO::HexOf(DwgIO::BaseOf(*entity).Self),
               DwgIO::HexOf(DwgIO::CommonOf(*entity)->Layer));
}
```

```text [Output]
LINE 30 on layer 2B
INSERT 31 on layer 10
```

## Save and load back {#save}

The drawing saves like any other and loads back unchanged. AutoCAD opens it, and its audit finds nothing.

```cpp
if (const auto saved = DwgIO::SaveFile(drawing, "chairs.dwg"); !saved)
{
  std::println("{}", saved.error().What);
  return 1;
}
const auto reloaded = DwgIO::LoadFile("chairs.dwg");
std::println("{} objects back, {} issues", reloaded->Drawing.Objects.size(), reloaded->Issues.size());
std::println("{} on {}", DwgIO::View::Record<DwgIO::Tables::Layer>(reloaded->Drawing, "walls")->Record.Name,
             DwgIO::View::Block(reloaded->Drawing, "chair")->Record.Name);
```

```text [Output]
42 objects back, 0 issues
Walls on Chair
```

## Many entities {#many}

Each function of `Edit` walks the drawing to find the handle seed and what it looks up. A single edit needs that walk, but a long run of edits repeats it at each step, so entities added one at a time take time growing with the square of their number. To build a large drawing, make the edits through an `Edit::Editor`. It walks the drawing once, keeps an index of its handles, its tables and their records' names current through each edit it makes, so each addition takes the same time however large the drawing grows. Its functions are those of `Edit`, refusals included, without the drawing as first argument.

```cpp
DwgIO::Drawing large = DwgIO::Create(DwgIO::Version::R2018);
DwgIO::Edit::Editor editor{large};
const auto tag = editor.AddBlock("Tag");
DwgIO::Entities::AttDef definition;
definition.Tag = "NUMBER";
definition.Lettering.Height = 0.25;
if (!tag || !editor.Add(definition, "Tag"))
{
  return 1;
}
for (int at = 0; at < 10'000; ++at)
{
  DwgIO::Entities::Line line;
  line.Start = DwgIO::Vector{.X = static_cast<double>(at), .Y = 0, .Z = 0};
  line.End = DwgIO::Vector{.X = static_cast<double>(at), .Y = 1, .Z = 0};
  if (!editor.Add(line))
  {
    return 1;
  }
}
for (int at = 0; at < 1'000; ++at)
{
  DwgIO::Entities::Insert insert;
  insert.Block = *tag;
  insert.Position = DwgIO::Vector{.X = static_cast<double>(at), .Y = -1, .Z = 0};
  const auto insertHandle = editor.Add(insert);
  DwgIO::Entities::Attrib number;
  number.Tag = "NUMBER";
  number.Lettering.Value = std::to_string(at);
  number.Lettering.Height = 0.25;
  if (!insertHandle || !editor.AddAttribute(*insertHandle, number))
  {
    return 1;
  }
}
std::println("{} objects, next handle {}", large.Objects.size(), DwgIO::HexOf(large.Header.Handseed));
if (!DwgIO::SaveFile(large, "numbered.dwg"))
{
  return 1;
}
```

```text [Output]
13038 objects, next handle 32F7
```

The drawing must outlive the editor and change only through it while it is used: an object appended to `Drawing::Objects` or a record renamed behind its back needs a new editor. `Editor::Remove` removes as `Edit::Remove` does, walking the drawing, and indexes it again afterwards. [Memory and speed](/concepts/limits/#performance) gives the times with and without an editor.
