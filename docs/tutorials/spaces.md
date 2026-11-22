# Model space and layouts

A drawing is drawn in model space, at full size, and laid out on sheets in paper space: one sheet per layout tab, each with viewports that show a part of model space at a scale. Each space is a block record that owns its entities. The examples read `curves-r2000.dwg`, which AutoCAD drew with curves, dimensions and leaders in model space and a layout of five viewports, and `curves-r14.dwg`, the same saved as R14.

## The spaces {#spaces}

The header names two block records: `BlockRecordModelSpace`, named `*Model_Space`, and `BlockRecordPaperSpace`, the paper space of the layout last active. Every other layout has a paper space block record of its own, and every one of them is named `*Paper_Space` in the file. Reach a layout's space through the layout, whose `Block` names its block record.

```cpp
const auto loaded = DwgIO::LoadFile("curves-r2000.dwg");
if (!loaded)
{
  return 1;
}
const DwgIO::Drawing &drawing = loaded->Drawing;
for (const DwgIO::Objects::Layout *layout : DwgIO::View::Layouts(drawing))
{
  const auto &space = DwgIO::Get<DwgIO::Tables::BlockRecord>(*DwgIO::Find(drawing, layout->Block));
  std::println("{:<8} tab {}: {} {}, {} entities{}", layout->Name, layout->Order, space.Record.Name,
               DwgIO::HexOf(layout->Block), DwgIO::View::Entities(drawing, space).size(),
               layout->Block == drawing.Header.BlockRecordPaperSpace ? ", the header's paper space" : "");
}
std::println("View::Block finds {}", DwgIO::HexOf(DwgIO::View::Block(drawing, "*Paper_Space")->Record.Base.Self));
```

```text [Output]
Model    tab 0: *Model_Space 1F, 52 entities
Layout1  tab 1: *Paper_Space D2, 7 entities, the header's paper space
Layout2  tab 2: *Paper_Space D6, 0 entities
View::Block finds D2
```

`View::Block(drawing, "*Paper_Space")` gives the header's paper space, which the block table keeps apart from its entries. The other layouts' records bear the same name, so reach them through their layout.

## The entities of a space {#entities}

`View::Entities` gives a space's entities in the order they are drawn: the record's list from R2004, the chain from `First` through each entity's `Next` up to R2000. Where the chain is broken, it gives every entity the record owns, in file order. The order is that of the block, not of a draw order table (`Objects::SortEntsTable`), which can change what is drawn on top.

Each entity says in `Common::Space` how the file places it: 2 in model space, 1 in the header's paper space, 0 in any other block, including another layout's paper space. `Load` fills in `Base::Owner` for every entity, so test the owner, not `Space`, to know which layout an entity is on.

```cpp
std::map<std::string, int> counts;
for (const DwgIO::Object &object : drawing.Objects)
{
  if (const DwgIO::Common *common = DwgIO::CommonOf(object); common != nullptr && common->Space != DwgIO::Space::Owner)
  {
    const std::string_view space = common->Space == DwgIO::Space::Model ? "model" : "paper";
    counts[std::format("{} in {}", space, DwgIO::HexOf(DwgIO::BaseOf(object).Owner))] += 1;
  }
}
for (const auto &[where, count] : counts)
{
  std::println("{} {}", count, where);
}
```

```text [Output]
54 model in 1F
9 paper in D2
```

The counts include each space's BLOCK and ENDBLK, which `View::Entities` leaves out. Layout2's space has `Space` 0 throughout, as does every entity owned by another entity, such as a polyline's vertices or an insert's attributes.

## Layouts {#layouts}

An `Objects::Layout` is a tab: its `Name`, its place `Order` (0 for Model), the block record of its space, the viewport last active in it, its limits and extents in paper units, and in `Plot` its page setup: the device, the paper size and margins in millimeters, what is plotted and at what scale. `View::Layouts` gives them in tab order and `View::Layout` finds one by name, ignoring case.

```cpp
const DwgIO::Objects::Layout *sheet = DwgIO::View::Layout(drawing, "layout1");
std::println("{}: limits ({}, {}) to ({}, {}), {} on {}, {} by {} mm", sheet->Name, sheet->Limmin.X, sheet->Limmin.Y,
             sheet->Limmax.X, sheet->Limmax.Y, sheet->Plot.Size, sheet->Plot.Device, sheet->Plot.Width,
             sheet->Plot.Height);
```

```text [Output]
Layout1: limits (-20, -7.5) to (277, 202.5), ISO_A4_(210.00_x_297.00_MM) on none_device, 210 by 297 mm
```

From R2000 a space's block record names its layout back in `BlockRecord::Layout`.

## Viewports {#viewports}

A paper space viewport is an `Entities::Viewport` owned by its layout's block record. On the sheet it is a rectangle: `Center`, `Width` and `Height` in paper space units. What it shows is a view of model space: `ViewCenter` is the view's center and `ViewHeight` its height, both in model space units; `Target` and `Direction` give the line of sight in world coordinates; `Twist` turns the view, in radians. The scale of the viewport is its `Height` over its `ViewHeight`.

The first viewport of a layout is the sheet itself as paper space shows it; the others look into model space. `Frozen` lists the layers frozen in that viewport alone, and `Clip` the entity that clips a viewport that is not a rectangle.

```cpp
const auto &paper = DwgIO::Get<DwgIO::Tables::BlockRecord>(*DwgIO::Find(drawing, sheet->Block));
for (const DwgIO::Object *entity : DwgIO::View::Entities(drawing, paper))
{
  if (const auto *viewport = DwgIO::GetIf<DwgIO::Entities::Viewport>(entity))
  {
    std::string frozen;
    for (const DwgIO::Handle layer : viewport->Frozen)
    {
      frozen += " " + DwgIO::Get<DwgIO::Tables::Layer>(*DwgIO::Find(drawing, layer)).Record.Name;
    }
    std::println("{} at ({:g}, {:g}), {:g} by {:g}: shows ({:g}, {:g}), scale {:.3f}, twist {}{}{}",
                 DwgIO::HexOf(viewport->Base.Self), viewport->Center.X, viewport->Center.Y, viewport->Width,
                 viewport->Height, viewport->ViewCenter.X, viewport->ViewCenter.Y, viewport->Height / viewport->ViewHeight,
                 viewport->Twist, viewport->Clip ? ", clipped" : "", frozen.empty() ? "" : ", frozen:" + frozen);
  }
}
```

```text [Output]
429 at (128.5, 97.5), 392.254 by 222.18: shows (128.5, 97.5), scale 1.000, twist 0
42D at (128.5, 97.5), 205.6 by 156: shows (300.25, 255.5), scale 0.222, twist 0
431 at (55, 35), 90 by 50: shows (66.9537, 12.533), scale 0.232, twist 0
436 at (150, 35), 60 by 50: shows (66.9537, 12.533), scale 0.232, twist 0, clipped
43B at (220, 40), 40 by 40: shows (50, 50), scale 0.400, twist 0, clipped, frozen: Hidden
```

`ViewCenter` is in the view's own coordinates, which match the world's when `Direction` is the Z axis, `Target` the origin and `Twist` zero, as here. `Flags` holds the viewport's switches, such as 131072 when it is off and 16384 when its zoom is locked; the reference lists them all.

## R14 {#r14}

R14 has no layouts: one model space, one paper space, and `TILEMODE` saying which is current. Its block records name no layout, and a viewport keeps its view not in its own fields but in its extended data, under the application `ACAD`: the text `MVIEW`, then in braces the target and direction, the twist, the view height and the view center, and the frozen layers by handle. The fields dwgio reads from R2000 on stay at their defaults.

A file a later AutoCAD saved as R14 still holds its `LAYOUT` objects as named objects, so `View::Layouts` finds them, and `Layout::Block` leads to each space as in later releases.

```cpp
const auto older = DwgIO::LoadFile("curves-r14.dwg");
if (!older)
{
  return 1;
}
const DwgIO::Drawing &r14 = older->Drawing;
const DwgIO::Objects::Layout *first = DwgIO::View::Layout(r14, "Layout1");
std::println("{} on {}, layout named back: {}", first->Name,
             DwgIO::Get<DwgIO::Tables::BlockRecord>(*DwgIO::Find(r14, first->Block)).Record.Name,
             DwgIO::HexOf(DwgIO::Get<DwgIO::Tables::BlockRecord>(*DwgIO::Find(r14, first->Block)).Layout));
for (const DwgIO::Object &object : r14.Objects)
{
  const auto *viewport = DwgIO::GetIf<DwgIO::Entities::Viewport>(&object);
  if (viewport == nullptr || viewport->Base.Extended.empty())
  {
    continue;
  }
  const std::vector<DwgIO::Tag> &groups = viewport->Base.Extended.front().Tags;
  std::println("{}: {}, Extent {}, view height {:g}, center ({:g}, {:g})", DwgIO::HexOf(viewport->Base.Self),
               std::get<std::string>(groups[0].Value), viewport->ViewHeight, std::get<double>(groups[10].Value),
               std::get<double>(groups[11].Value), std::get<double>(groups[12].Value));
}
```

```text [Output]
Layout1 on *PAPER_SPACE, layout named back: 0
429: MVIEW, Extent 0, view height 222.18, center (128.5, 97.5)
42D: MVIEW, Extent 0, view height 702.086, center (300.25, 255.5)
431: MVIEW, Extent 0, view height 215.405, center (66.9537, 12.533)
436: MVIEW, Extent 0, view height 215.405, center (66.9537, 12.533)
43B: MVIEW, Extent 0, view height 100, center (50, 50)
```

The groups count from the `MVIEW` text: the opening brace, a version, the target and the direction as three groups each, then the twist, the height and the center.
