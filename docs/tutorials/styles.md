# Layers, colors and styles

How an entity looks comes from its `Common` and from the records it names: its layer, its linetype, its text or dimension style. Many entities say only "by layer" or "by block", and what they show is found by following those links. The examples read `tables-r2004.dwg`, whose tables AutoCAD filled with layers in every state, five linetypes, two text styles and a dimension style.

## What an entity names {#common}

| Member of `Common` | Holds |
| :--- | :--- |
| `Layer` | The layer's handle |
| `Color` | The color: by layer, by block, a palette index or, from R2004, a true color |
| `LinetypeOrigin`, `Linetype` | An `Origin`: `ByLayer`, `ByBlock`, `Continuous`, or `Record`, the record `Linetype` names |
| `LinetypeScale` | The entity's own linetype scale, applied with the header's `Ltscale` |
| `Lineweight` | From R2000, a `Lineweight`: a standard width such as `Mm025`, `ByLayer`, `ByBlock` or `Default`; empty before R2000 |
| `PlotstyleOrigin`, `Plotstyle` | From R2000, the plot style: `ByLayer`, `ByBlock` or `Record`, the record `Plotstyle` names |
| `MaterialOrigin`, `Material` | From R2007, the material, given the same way |
| `Invisible` | Not drawn at all |

An R14 file has no such origins: it sets a bit for a ByLayer linetype and names the record of any other. dwgio reads the first as `ByLayer` and the other as `Record` with the record, which may be the `ByBlock` one (`Header::LTypeByBlock`). R14 keeps an entity's lineweight in its extended data, so `Lineweight` is empty.

## Layers {#layers}

A `Tables::Layer` holds its color, linetype, lineweight and states. `Frozen`, `On` and `Locked` are separate members, as AutoCAD keeps them: a frozen layer is neither drawn nor regenerated, a layer that is off is not drawn, and a locked one is drawn but cannot be edited. `FrozenNew` freezes it in new viewports, and `Plot` false leaves it out of plots. A viewport can also freeze a layer for itself alone, in `Viewport::Frozen`.

```cpp
const auto loaded = DwgIO::LoadFile("tables-r2004.dwg");
if (!loaded)
{
  return 1;
}
DwgIO::Drawing drawing = loaded->Drawing;
const auto recordName = [&](DwgIO::Handle record) -> std::string
{
  return DwgIO::Visit(
      [](const auto &kind) -> std::string
      {
        if constexpr (requires { kind.Record.Name; })
        {
          return kind.Record.Name;
        }
        else
        {
          return "?";
        }
      },
      *DwgIO::Find(drawing, record));
};
const auto lineweightText = [](DwgIO::Lineweight lineweight) -> std::string
{
  const std::optional<double> millimeters = DwgIO::MillimetersOf(lineweight);
  return millimeters ? std::format("{:.2f} mm", *millimeters) : "default";
};
for (const DwgIO::Tables::Layer *layer : DwgIO::View::Records<DwgIO::Tables::Layer>(drawing))
{
  std::println("{:<6} color {:<2} {:<10} {}{}{}{}{}", layer->Record.Name, layer->Color.Index,
               recordName(layer->Linetype), lineweightText(layer->Lineweight), layer->Frozen ? " frozen" : "",
               layer->On ? "" : " off", layer->Locked ? " locked" : "", layer->Plot ? "" : " not plotted");
}
```

```text [Output]
0      color 7  Continuous default
Walls  color 1  Continuous 0.50 mm
Doors  color 30 HIDDEN     default off locked not plotted
Hidden color 7  DASHED     default frozen
```

A `Lineweight` names one of AutoCAD's 24 standard widths, and `MillimetersOf` gives it. `Default` is AutoCAD's `LWDEFAULT`, a setting of the program rather than of the drawing: 0.25 mm unless changed.

## Colors {#colors}

A `Color` holds:

- `Index`: 256 by layer, 0 by block, 1 to 255 an entry of AutoCAD's palette, where 1 to 7 are red, yellow, green, cyan, blue, magenta and white.
- `Rgb`, from R2004, the true color as the file stores it: red, green and blue in the low three bytes, red the highest, and in the top byte its method, 0xC2 for a true color. When it holds a value it is the color, whatever `Index` says. `MethodOf` gives the method as a `ColorMethod`, and `RgbOf` the color as 0xRRGGBB.
- `Names`, from R2004, the color's name and the color book it comes from, when it has them.
- `Transparency`, from R2004, an entity's transparency: in the top byte 1 for by block or 2 for a value, and in the low byte that value, from 0 for clear to 255 for opaque. None means by layer. `TransparencyMethodOf` gives the method and `AlphaOf` the value.

The drawing has no true color. Add a layer with one, and a line on it that is half transparent:

```cpp
DwgIO::Tables::Layer exterior;
exterior.Record.Name = "Exterior";
exterior.Color = DwgIO::Color{.Index = 0, .Rgb = static_cast<int>(0xC2B4643CU)};
exterior.Color.Names = DwgIO::ColorNames{.Name = "Orange", .Book = "Office"};
exterior.Linetype = drawing.Header.LTypeContinuous;
const auto layerHandle = DwgIO::Edit::Add(drawing, exterior);
DwgIO::Entities::Line wall;
wall.End = DwgIO::Vector{.X = 4, .Y = 0, .Z = 0};
wall.Common.Color.Transparency = 0x0200007F;
if (!layerHandle)
{
  return 1;
}
wall.Common.Layer = *layerHandle;
if (!DwgIO::Edit::Add(drawing, wall) || !DwgIO::SaveFile(drawing, "exterior.dwg"))
{
  return 1;
}
const auto describe = [](const DwgIO::Color &color) -> std::string
{
  if (const std::optional<std::uint32_t> rgb = DwgIO::RgbOf(color))
  {
    return std::format("rgb({}, {}, {}){}", (*rgb >> 16U) & 0xFFU, (*rgb >> 8U) & 0xFFU, *rgb & 0xFFU,
                       color.Names && (*color.Names)->Name ? " named " + *(*color.Names)->Name : "");
  }
  switch (DwgIO::MethodOf(color))
  {
  case DwgIO::ColorMethod::ByLayer:
    return "by layer";
  case DwgIO::ColorMethod::ByBlock:
    return "by block";
  default:
    return std::format("index {}", color.Index);
  }
};
const auto reloaded = DwgIO::LoadFile("exterior.dwg");
if (!reloaded)
{
  return 1;
}
const auto *layer = DwgIO::View::Record<DwgIO::Tables::Layer>(reloaded->Drawing, "exterior");
std::println("layer {}: {}", layer->Record.Name, describe(layer->Color));
for (const DwgIO::Object *entity :
     DwgIO::View::Entities(reloaded->Drawing, *DwgIO::View::ModelSpace(reloaded->Drawing)))
{
  if (const auto *line = DwgIO::GetIf<DwgIO::Entities::Line>(entity))
  {
    const DwgIO::Color &color = line->Common.Color;
    std::println("line: {}, transparency by alpha {}, {:.0f}% clear", describe(color),
                 DwgIO::TransparencyMethodOf(color) == DwgIO::TransparencyMethod::Alpha,
                 100 - (DwgIO::AlphaOf(color).value_or(0) * 100 / 255.0));
  }
}
```

```text [Output]
layer Exterior: rgb(180, 100, 60) named Orange
line: by layer, transparency by alpha true, 50% clear
```

AutoCAD writes the index 0 next to a layer's true color, which is why `Rgb` is tested first. A release before R2004 has no true color: a save to one keeps an index in its place.

## Resolving by layer and by block {#resolve}

To know the color, linetype and lineweight an entity is drawn with, follow where each comes from:

- By layer takes the layer's. An entity on layer 0 inside a block takes the layer of the insert that places it, as AutoCAD draws it.
- By block takes what the insert that places the block is drawn with. Outside any block, by block draws white (7), with Continuous, at the default lineweight.
- Anything else is the entity's own.

Add a block `Mark` whose line is by block in all three and whose circle is by layer on layer 0, then insert it twice: on layer Walls, and on layer 0 in green with the DASHED linetype at 0.18 mm.

```cpp
DwgIO::Entities::Line blockLine;
blockLine.End = DwgIO::Vector{.X = 1, .Y = 0, .Z = 0};
blockLine.Common.Color.Index = 0;
blockLine.Common.LinetypeOrigin = DwgIO::Origin::ByBlock;
blockLine.Common.Lineweight = DwgIO::Lineweight::ByBlock;
DwgIO::Entities::Circle blockCircle;
blockCircle.Radius = 0.5;
const auto layerZero = DwgIO::View::Record<DwgIO::Tables::Layer>(drawing, "0")->Record.Base.Self;
const auto walls = DwgIO::View::Record<DwgIO::Tables::Layer>(drawing, "Walls")->Record.Base.Self;
const auto dashed = DwgIO::View::Record<DwgIO::Tables::LType>(drawing, "DASHED")->Record.Base.Self;
const auto mark = DwgIO::Edit::AddBlock(drawing, "Mark");
DwgIO::Entities::Insert wallsInsert;
wallsInsert.Common.Layer = walls;
DwgIO::Entities::Insert greenInsert;
greenInsert.Common.Layer = layerZero;
greenInsert.Common.Color.Index = 3;
greenInsert.Common.LinetypeOrigin = DwgIO::Origin::Record;
greenInsert.Common.Linetype = dashed;
greenInsert.Common.Lineweight = DwgIO::Lineweight::Mm018;
if (!mark || !DwgIO::Edit::Add(drawing, blockLine, "Mark") || !DwgIO::Edit::Add(drawing, blockCircle, "Mark"))
{
  return 1;
}
wallsInsert.Block = *mark;
greenInsert.Block = *mark;
if (!DwgIO::Edit::Add(drawing, wallsInsert) || !DwgIO::Edit::Add(drawing, greenInsert))
{
  return 1;
}
```

`AppearanceOf` resolves an entity's `Common` against what the insert around it resolved to, or nothing at the top. An `Index` of the drawing lets it find the layers without walking the drawing each time.

```cpp
const DwgIO::Index index{drawing};
const auto &block = DwgIO::Get<DwgIO::Tables::BlockRecord>(*index.Find(*mark));
for (const DwgIO::Object *entity : DwgIO::View::Entities(index, *DwgIO::View::ModelSpace(index)))
{
  const auto *insert = DwgIO::GetIf<DwgIO::Entities::Insert>(entity);
  if (insert == nullptr || insert->Block != *mark)
  {
    continue;
  }
  const DwgIO::Appearance insertAppearance = DwgIO::AppearanceOf(index, insert->Common, nullptr);
  for (const DwgIO::Object *part : DwgIO::View::Entities(index, block))
  {
    const DwgIO::Appearance appearance = DwgIO::AppearanceOf(index, *DwgIO::CommonOf(*part), &insertAppearance);
    std::println("{} in INSERT on {}: layer {}, {}, {}, {}", DwgIO::TypeOf(*part), recordName(insert->Common.Layer),
                 recordName(appearance.Layer), describe(appearance.Color), recordName(appearance.Linetype),
                 lineweightText(appearance.Lineweight));
  }
}
```

```text [Output]
LINE in INSERT on Walls: layer Walls, index 1, Continuous, 0.50 mm
CIRCLE in INSERT on Walls: layer Walls, index 1, Continuous, 0.50 mm
LINE in INSERT on 0: layer 0, index 3, DASHED, 0.18 mm
CIRCLE in INSERT on 0: layer 0, index 7, Continuous, default
```

For a block inside a block, resolve the inner insert with the outer one's appearance and pass the result down. The same flags and the same rule hold for the plot style and, from R2007, the material. Whether the entity shows at all follows the layer resolved the same way: an entity on layer 0 in a block placed on a frozen layer is not drawn.

## Linetypes {#linetypes}

A `Tables::LType` holds its `Description`, the `Length` of its pattern and its `Dashes`. A dash's `Length` is positive for a dash, 0 for a dot and negative for a space. A complex linetype draws a shape or a text in some of them: `Flags` has 4 for a shape, numbered `Shape` in the shape file `Style` names, or 2 for a text, `Text` in the text style `Style` names; `Along`, `Across`, `Scale` and `Rotation` place it.

```cpp
for (const DwgIO::Tables::LType *linetype : DwgIO::View::Records<DwgIO::Tables::LType>(drawing))
{
  std::string pattern;
  for (const DwgIO::Tables::Dash &dash : linetype->Dashes)
  {
    pattern += std::format(" {:g}", dash.Length);
    if ((dash.Flags & 4) != 0)
    {
      pattern += std::format(" [shape {}]", dash.Shape);
    }
    if ((dash.Flags & 2) != 0)
    {
      pattern += std::format(" [\"{}\" in {}]", dash.Text, recordName(dash.Style));
    }
  }
  std::println("{:<10} {:g}:{}", linetype->Record.Name, linetype->Length, pattern);
}
```

```text [Output]
Continuous 0:
CENTER     50.8: 31.75 -6.35 6.35 -6.35
DASHED     19.05: 12.7 -6.35
HIDDEN     9.525: 6.35 -3.175
FENCELINE1 36.83: 6.35 -2.54 [shape 133] -2.54 25.4
GAS_LINE   24.13: 12.7 -5.08 ["GAS" in Standard] -6.35
ByBlock    0:
ByLayer    0:
```

The ByBlock and ByLayer records have no pattern: they stand for the flags 1 and 0. Lengths are in drawing units, scaled by the header's `Ltscale` and the entity's `LinetypeScale`.

## Text and dimension styles {#styles}

A text names its `Tables::Style` in `Lettering.Style`, an MTEXT in `Style`, and a dimension its `Tables::DimStyle` in `Style`; the header's `Textstyle` and `Dimstyle` are the current ones. A text style holds a font file, a fixed `Height` or 0 when each text gives its own, a `Width` factor and an `Oblique` angle in radians. A style with `Shape` set is a shape file, such as the nameless one a complex linetype loads. A dimension style has one member per DIM variable, named as AutoCAD names it.

```cpp
for (const DwgIO::Tables::Style *style : DwgIO::View::Records<DwgIO::Tables::Style>(drawing))
{
  std::println("style \"{}\": {}{}, height {}, width {}, oblique {:.0f} degrees", style->Record.Name, style->Font,
               style->Shape ? " (shapes)" : "", style->Height, style->Width, style->Oblique * 360 / DwgIO::Turn);
}
const auto *metric = DwgIO::View::Record<DwgIO::Tables::DimStyle>(drawing, "Metric");
std::println("dimension style {}: DIMSCALE {}, DIMTXT {}, DIMASZ {}, DIMDEC {}, text in {}", metric->Record.Name,
             metric->Dimscale, metric->Dimtxt, metric->Dimasz, metric->Dimdec, recordName(metric->Dimtxsty));
```

```text [Output]
style "Standard": Arial.ttf, height 0, width 1, oblique 0 degrees
style "Annotative": Arial.ttf, height 0, width 1, oblique 0 degrees
style "": ltypeshp.shx (shapes), height 0, width 1, oblique 0 degrees
style "Notes": Arial.ttf, height 0, width 0.8, oblique 15 degrees
style "Mono": txt.shx, height 2.5, width 1, oblique 0 degrees
dimension style Metric: DIMSCALE 2, DIMTXT 3.5, DIMASZ 2.5, DIMDEC 2, text in Standard
```

A dimension's own overrides of its style are in its extended data, under the application `ACAD`: the text `DSTYLE`, then in braces each variable's DXF group code as a 1070 group, followed by its value.
