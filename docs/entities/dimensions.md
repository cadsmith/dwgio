# Dimensions and leaders

A dimension measures and is drawn through an anonymous block that holds its lines, arrows and text. dwgio gives its definition points, the measurement AutoCAD saved, its style and the overrides it keeps in extended data. LEADER, TOLERANCE and MULTILEADER follow the dimension kinds. The examples read `curves-r2000.dwg`, which holds a linear, a rotated, an aligned, two angular, a radius, a diameter and two ordinate dimensions, three leaders and a tolerance; `dimensions-r2000.dwg`, which holds an arc length dimension and a jogged radius dimension; and `mleaders-r2018.dwg`, which holds four multileaders, one of them with a block.

## The kinds {#forms}

Every `Dimension` has a `Measure`, what all kinds share, and a `Form`, a variant holding the kind and the points it adds. `TypeOf` names the kind. The arc length and jogged radius dimensions are kinds of their own, `ArcDimension` and `LargeRadialDimension`.

| `TypeOf` | `Form` | What `Measure::DefinitionPoint` marks | The kind's points |
| :--- | :--- | :--- | :--- |
| `DIMENSION_LINEAR` | `Forms::Linear` | Where the dimension line meets the second extension line | `Start`, `End`: the extension lines' origins; `Angle`: the direction measured along |
| `DIMENSION_ALIGNED` | `Forms::Aligned` | The same | `Start`, `End` |
| `DIMENSION_ANG3PT` | `Forms::Angular3Point` | A point on the dimension arc | `Vertex`, and `Start` and `End` on each side |
| `DIMENSION_ANG2LN` | `Forms::Angular2Line` | The end of the second line | `FirstStart`, `FirstEnd`: the first line; `SecondStart`: the start of the second; `ArcPoint`: a point on the arc |
| `DIMENSION_RADIUS` | `Forms::Radial` | The center | `Chord`: the point on the curve |
| `DIMENSION_DIAMETER` | `Forms::Diameter` | The point on the curve across from `Chord` | `Chord`: the point on the curve |
| `DIMENSION_ORDINATE` | `Forms::Ordinate` | The UCS origin it measures from | `Feature`, and the end of the `Leader` |
| `ARC_DIMENSION` | | A point on the dimension arc | `Start`, `End`, `Center` of the arc |
| `LARGE_RADIAL_DIMENSION` | | The curve's center | `Chord` on the curve, `Center` it is drawn from, `JogPoint` the jog |

`Measure::Measured` is the value AutoCAD last computed, in drawing units or radians; a dimension dwgio makes holds -1 until AutoCAD measures it. The definition points give it back:

```cpp
const auto curves = DwgIO::LoadFile("curves-r2000.dwg");
if (!curves)
{
  return 1;
}
const DwgIO::Drawing &drawing = curves->Drawing;
const auto distance = [](DwgIO::Vector a, DwgIO::Vector b) -> double
{ return std::hypot(b.X - a.X, b.Y - a.Y, b.Z - a.Z); };
const auto between = [](DwgIO::Vector one, DwgIO::Vector other) -> double
{
  const double dot = (one.X * other.X) + (one.Y * other.Y) + (one.Z * other.Z);
  const double cross = std::hypot((one.Y * other.Z) - (one.Z * other.Y), (one.Z * other.X) - (one.X * other.Z),
                                  (one.X * other.Y) - (one.Y * other.X));
  return std::atan2(cross, dot);
};
const auto minus = [](DwgIO::Vector a, DwgIO::Vector b) -> DwgIO::Vector
{ return DwgIO::Vector{.X = a.X - b.X, .Y = a.Y - b.Y, .Z = a.Z - b.Z}; };
namespace Forms = DwgIO::Entities::Forms;
for (const DwgIO::Object &object : drawing.Objects)
{
  const auto *dimension = DwgIO::GetIf<DwgIO::Entities::Dimension>(&object);
  if (dimension == nullptr)
  {
    continue;
  }
  const DwgIO::Entities::Measure &measure = dimension->Measure;
  const Forms::Form &form = dimension->Form;
  double value = 0;
  bool angle = false;
  if (const auto *linear = std::get_if<Forms::Linear>(&form))
  {
    const DwgIO::Vector span = minus(linear->End, linear->Start);
    value = std::abs((span.X * std::cos(linear->Angle)) + (span.Y * std::sin(linear->Angle)));
  }
  else if (const auto *aligned = std::get_if<Forms::Aligned>(&form))
  {
    value = distance(aligned->Start, aligned->End);
  }
  else if (const auto *points = std::get_if<Forms::Angular3Point>(&form))
  {
    value = between(minus(points->Start, points->Vertex), minus(points->End, points->Vertex));
    angle = true;
  }
  else if (const auto *lines = std::get_if<Forms::Angular2Line>(&form))
  {
    value = between(minus(lines->FirstEnd, lines->FirstStart), minus(measure.DefinitionPoint, lines->SecondStart));
    angle = true;
  }
  else if (const auto *radial = std::get_if<Forms::Radial>(&form))
  {
    value = distance(measure.DefinitionPoint, radial->Chord);
  }
  else if (const auto *diameter = std::get_if<Forms::Diameter>(&form))
  {
    value = distance(measure.DefinitionPoint, diameter->Chord);
  }
  else if (const auto *ordinate = std::get_if<Forms::Ordinate>(&form))
  {
    const bool along = (measure.Flags & 64) != 0;
    value = std::abs(along ? ordinate->Feature.X - measure.DefinitionPoint.X : ordinate->Feature.Y - measure.DefinitionPoint.Y);
  }
  const double scale = angle ? 360 / DwgIO::Turn : 1;
  std::println("{} {}: {:.4f} from its points, {:.4f} saved{}", DwgIO::TypeOf(object),
               DwgIO::HexOf(dimension->Base.Self), value * scale, measure.Measured * scale, angle ? " (degrees)" : "");
}
```

```text [Output]
DIMENSION_LINEAR 39C: 10.0000 from its points, 10.0000 saved
DIMENSION_LINEAR 3AB: 6.1603 from its points, 6.1603 saved
DIMENSION_ALIGNED 3BA: 11.1803 from its points, 11.1803 saved
DIMENSION_ANG3PT 3C7: 90.0000 from its points, 90.0000 saved (degrees)
DIMENSION_ANG2LN 3D9: 90.0000 from its points, 90.0000 saved (degrees)
DIMENSION_RADIUS 3E8: 5.0000 from its points, 5.0000 saved
DIMENSION_DIAMETER 3F3: 10.0000 from its points, 10.0000 saved
DIMENSION_ORDINATE 3FF: 120.0000 from its points, 120.0000 saved
DIMENSION_ORDINATE 408: 20.0000 from its points, 20.0000 saved
```

An ordinate dimension measures along X when `Measure::Flags` has 64, along Y otherwise. A two-line angular dimension's lines make two angles; `ArcPoint`, the point on the arc, says which one it measures.

## Coordinate systems {#coordinates}

Every definition point is in world coordinates except three, which are in the dimension's object coordinate system, set by `Measure::Normal`, their Z the elevation: `Measure::TextMiddle`, the middle of the text; `Measure::CloneInsertion`, where AutoCAD inserts the next dimension of a baseline or continued series; and an angular form's `ArcPoint`. With the default normal the two systems are the same. [Coordinates and units](/tutorials/coordinates/#arbitrary) converts the others.

## The block {#block}

`Block` is the anonymous block record, named `*D`, that holds what AutoCAD draws: lines, arrowheads as solids or inserts, the text as an MTEXT, and points on the layer `Defpoints` at the definition points. The block is placed with no offset, at `Measure::Scale` and turned by `Measure::BlockRotation`, both the identity for a dimension AutoCAD has not moved, so its entities are where the drawing shows them. To draw a dimension, draw its block; dwgio does not draw one from its points. [Blocks and attributes](/tutorials/blocks/#anonymous) finds every dimension's block.

```cpp
const auto &rotated = DwgIO::Get<DwgIO::Entities::Dimension>(*DwgIO::Find(drawing, DwgIO::ParseHandle("3AB").value()));
const auto &record = DwgIO::Get<DwgIO::Tables::BlockRecord>(*DwgIO::Find(drawing, rotated.Block));
std::map<std::string_view, int> kinds;
std::string shown;
for (const DwgIO::Object *entity : DwgIO::View::Entities(drawing, record))
{
  ++kinds[DwgIO::TypeOf(*entity)];
  if (const auto *mtext = DwgIO::GetIf<DwgIO::Entities::MText>(entity))
  {
    shown = mtext->Value;
  }
}
std::string counted;
for (const auto &[kind, count] : kinds)
{
  counted += std::format(" {} {}", count, kind);
}
std::println("text \"{}\", shown \"{}\"; block {}:{}", rotated.Measure.Text, shown, record.Record.Name, counted);
```

```text [Output]
text "Width <>", shown "Width 6,16"; block *D: 5 LINE 1 MTEXT 3 POINT 2 SOLID
```

`Measure::Text` is the text the user gave: empty shows the measurement, and `<>` stands for it, as here. The block's MTEXT holds what the drawing shows, the measurement formatted as the style says: 2 decimals with a comma in `ISO-25`.

## Style and overrides {#style}

`Style` names the `Tables::DimStyle`, which holds one member per DIM variable, such as `Dimtxt`, `Dimdec` or `Dimjogang`. A dimension that overrides some of them keeps the overrides in its extended data under the application `ACAD`: the text `DSTYLE`, then between braces each variable's DXF group code as a 1070 group, followed by its value. AutoCAD also writes some overrides under applications of their own, such as `ACAD_DSTYLE_DIMRADIAL_EXTENSION`.

```cpp
const auto overrides = [](const DwgIO::Base &base) -> std::string
{
  std::string found;
  for (const DwgIO::Xdata &record : base.Extended)
  {
    const std::vector<DwgIO::Tag> &groups = record.Tags;
    const auto *first = groups.empty() ? nullptr : std::get_if<std::string>(&groups.front().Value);
    if (first == nullptr || *first != "DSTYLE")
    {
      continue;
    }
    for (std::size_t at = 2; at + 1 < groups.size(); at += 2)
    {
      const std::int64_t code = std::get<std::int64_t>(groups[at].Value);
      const DwgIO::Content &value = groups[at + 1].Value;
      if (const auto *number = std::get_if<std::int64_t>(&value))
      {
        found += std::format(" {}={}", code, *number);
      }
      else if (const auto *real = std::get_if<double>(&value))
      {
        found += std::format(" {}={:.4f}", code, *real);
      }
      else if (const auto *text = std::get_if<std::string>(&value))
      {
        found += std::format(" {}=\"{}\"", code, *text);
      }
      else if (const auto *handle = std::get_if<DwgIO::Handle>(&value))
      {
        found += std::format(" {}={}", code, DwgIO::HexOf(*handle));
      }
    }
  }
  return found.empty() ? " none" : found;
};
const auto measures = DwgIO::LoadFile("dimensions-r2000.dwg");
if (!measures)
{
  return 1;
}
for (const DwgIO::Drawing *one : {&drawing, &measures->Drawing})
{
  for (const DwgIO::Object &object : one->Objects)
  {
    const DwgIO::Handle style = DwgIO::Visit(
        [](const auto &kind) -> DwgIO::Handle
        {
          if constexpr (requires { kind.Measure; })
          {
            return kind.Style;
          }
          return {};
        },
        object);
    if (style)
    {
      std::println("{} {} in {}: overrides{}", DwgIO::TypeOf(object), DwgIO::HexOf(DwgIO::BaseOf(object).Self),
                   DwgIO::Get<DwgIO::Tables::DimStyle>(*DwgIO::Find(*one, style)).Record.Name,
                   overrides(DwgIO::BaseOf(object)));
    }
  }
}
```

```text [Output]
DIMENSION_LINEAR 39C in ISO-25: overrides none
DIMENSION_LINEAR 3AB in ISO-25: overrides none
DIMENSION_ALIGNED 3BA in ISO-25: overrides none
DIMENSION_ANG3PT 3C7 in ISO-25: overrides none
DIMENSION_ANG2LN 3D9 in ISO-25: overrides none
DIMENSION_RADIUS 3E8 in ISO-25: overrides 288=1
DIMENSION_DIAMETER 3F3 in ISO-25: overrides 288=1
DIMENSION_ORDINATE 3FF in ISO-25: overrides none
DIMENSION_ORDINATE 408 in ISO-25: overrides none
ARC_DIMENSION 2CD in ISO-25: overrides none
LARGE_RADIAL_DIMENSION 2ED in ISO-25: overrides 50=0.7854
```

Group 288 is `DIMUPT`, on when the text was placed with the cursor, and group 50 `DIMJOGANG`, the jog angle, here 45 degrees in radians. The DXF reference's DIMSTYLE table lists every variable's group code.

## Arc length and jogged radius {#arc}

An `ArcDimension` measures along an arc: `Center` and the arc's ends `Start` and `End`, its angles `StartAngle` and `EndAngle` in radians, and `Partial` when it measures part of the arc. A `LargeRadialDimension` is a radius dimension for an arc whose center is off the sheet: the line runs from `Center`, a stand-in for the curve's center, through the jog at `JogPoint` to `Chord` on the curve.

```cpp
for (const DwgIO::Object &object : measures->Drawing.Objects)
{
  if (const auto *arc = DwgIO::GetIf<DwgIO::Entities::ArcDimension>(&object))
  {
    std::println("ARC_DIMENSION: center ({}, {}), from {:.0f} to {:.0f} degrees, measured {:.4f}", arc->Center.X,
                 arc->Center.Y, arc->StartAngle * 360 / DwgIO::Turn, arc->EndAngle * 360 / DwgIO::Turn, arc->Measure.Measured);
  }
  if (const auto *jogged = DwgIO::GetIf<DwgIO::Entities::LargeRadialDimension>(&object))
  {
    const auto round = [](double value) -> double { return std::round(value) + 0.0; };
    std::println("LARGE_RADIAL_DIMENSION: center ({}, {}), drawn from ({}, {}), measured {}",
                 round(jogged->Measure.DefinitionPoint.X), round(jogged->Measure.DefinitionPoint.Y), round(jogged->Center.X),
                 round(jogged->Center.Y), jogged->Measure.Measured);
  }
}
```

```text [Output]
ARC_DIMENSION: center (10, 0), from 0 to 180 degrees, measured 31.4159
LARGE_RADIAL_DIMENSION: center (50, 0), drawn from (70, 20), measured 10
```

## Leaders {#leader}

A `Leader` is a line with an arrow from `Points`, in world coordinates, to its annotation. `Annotation` says what it was made with: 0 an MTEXT, 1 a TOLERANCE, 2 an INSERT, 3 nothing; `Note` is that entity's handle. `Path` 1 draws a spline through the points, `Arrow` says the first point has an arrowhead, and `Style` names the dimension style it takes its arrow and sizes from.

```cpp
for (const DwgIO::Object &object : drawing.Objects)
{
  if (const auto *leader = DwgIO::GetIf<DwgIO::Entities::Leader>(&object))
  {
    std::string points;
    for (const DwgIO::Vector &point : leader->Points)
    {
      points += std::format(" ({}, {})", point.X, point.Y);
    }
    const DwgIO::Object *note = DwgIO::Find(drawing, leader->Note);
    std::println("LEADER {}:{}, {}, annotated by {}", DwgIO::HexOf(leader->Base.Self), points,
                 leader->Path == 1 ? "a spline" : "straight", note == nullptr ? "nothing" : DwgIO::TypeOf(*note));
  }
}
```

```text [Output]
LEADER 411: (0, -40) (10, -45) (15, -45), straight, annotated by MTEXT
LEADER 413: (20, -40) (25, -48) (30, -45), a spline, annotated by nothing
LEADER 414: (40, -40) (45, -45), straight, annotated by nothing
LEADER 41C: (80, -40) (85, -45), straight, annotated by INSERT
```

## Tolerances {#tolerance}

A `Tolerance` is a feature control frame at `Position`, along `Direction`, in world coordinates. `Text` holds its boxes in AutoCAD's codes: `%%v` separates two boxes, `{\Fgdt;j}` writes a `j` in the `gdt` font, which draws the position symbol, `n` the diameter sign and `m` the circled M of maximum material condition; a second row follows `^J`.

```cpp
for (const DwgIO::Object &object : drawing.Objects)
{
  if (const auto *frame = DwgIO::GetIf<DwgIO::Entities::Tolerance>(&object))
  {
    std::println("TOLERANCE at ({}, {}): {}", frame->Position.X, frame->Position.Y, frame->Text);
  }
}
```

```text [Output]
TOLERANCE at (60, -40): {\Fgdt;j}%%v{\Fgdt;n}0.1{\Fgdt;m}
```

## Multileaders {#multileader}

A `MultiLeader` keeps all it holds in `MLeader`, a `MultiLeaders::MLeader`. `MLeader::ContentType` says what the leaders point from: 1 a block, 2 an MTEXT, 3 a tolerance. Where things lie is in its context data, `MLeader::Context`: the text in `Text`, with MTEXT's formatting codes, at `TextLocation`; or the block `BlockContent`, with the values of its attributes in `MLeader::BlockAttributes`. Each leader is a `LeaderRoot`: the point `LastPoint` where its lines end and its landing starts, the landing's `DoglegVector` and `DoglegLength`, and its lines, each a `LeaderLine` whose `Vertices` run from the arrowhead toward `LastPoint` and whose `Breaks` are the breaks DIMBREAK made. All points are in world coordinates. `MLeader::Style` names the MLEADERSTYLE, an entry of the `ACAD_MLEADERSTYLE` dictionary, and `MLeader::Overrides` says which of its properties the multileader overrides.

```cpp
const auto mleaders = DwgIO::LoadFile("mleaders-r2018.dwg");
if (!mleaders)
{
  return 1;
}
const DwgIO::Drawing &callouts = mleaders->Drawing;
const DwgIO::Objects::Dictionary *styles = DwgIO::View::Dictionary(callouts, "ACAD_MLEADERSTYLE");
for (const DwgIO::Object &object : callouts.Objects)
{
  const auto *mleader = DwgIO::GetIf<DwgIO::Entities::MultiLeader>(&object);
  if (mleader == nullptr)
  {
    continue;
  }
  const DwgIO::Entities::MultiLeaders::MLeader &callout = mleader->MLeader;
  const DwgIO::Entities::MultiLeaders::ContextData &context = callout.Context;
  const auto style = std::ranges::find(styles->Entries, callout.Style, &DwgIO::Objects::Entry::Object);
  std::string content = std::format("text \"{}\"", context.Text);
  if (callout.ContentType == 1)
  {
    const auto &block = DwgIO::Get<DwgIO::Tables::BlockRecord>(*DwgIO::Find(callouts, context.BlockContent));
    content = std::format("block {}", block.Record.Name);
    for (const DwgIO::Entities::MultiLeaders::BlockAttribute &label : callout.BlockAttributes)
    {
      const auto &definition = DwgIO::Get<DwgIO::Entities::AttDef>(*DwgIO::Find(callouts, label.Definition));
      content += std::format(", {} = {}", definition.Tag, label.Text);
    }
  }
  std::println("MULTILEADER {} in {}: {}", DwgIO::HexOf(mleader->Base.Self), style->Name, content);
  for (const DwgIO::Entities::MultiLeaders::LeaderRoot &branch : context.Leaders)
  {
    for (const DwgIO::Entities::MultiLeaders::LeaderLine &stem : branch.Lines)
    {
      std::string points;
      for (const DwgIO::Vector &point : stem.Vertices)
      {
        points += std::format(" ({}, {})", point.X, point.Y);
      }
      std::println("  line{} to ({:.4f}, {}), landing {} along ({}, {}), {} breaks", points, branch.LastPoint.X,
                   branch.LastPoint.Y, branch.DoglegLength, branch.DoglegVector.X, branch.DoglegVector.Y, stem.Breaks.size());
    }
  }
}
```

```text [Output]
MULTILEADER 2DB in Upright: text "up"
  line (100, 0) (105, 5) to (110.0000, 10), landing 6 along (1, 0), 0 breaks
MULTILEADER 2DE in Standard: text "broken"
  line (150, 0) to (160.0000, 10), landing 8 along (1, 0), 1 breaks
MULTILEADER 2D3 in Standard: text "two"
  line (0, 0) to (10.0000, 10), landing 8 along (1, 0), 0 breaks
  line (30, 0) to (38.3902, 10), landing 8 along (-1, 0), 0 breaks
MULTILEADER 2D7 in Blocky: block Bubble, NUM = 7
  line (50, 0) to (60.0000, 10), landing 8 along (1, 0), 0 breaks
```

The first multileader, in the style `Upright`, has a line of two points. `broken` has one break in its line, from (154, 4) to (155, 5). `two` has two leaders, the second added by MLEADEREDIT, landing on either side of its text. The block `Bubble` of the last one holds an attribute definition `NUM`, given 7 here.

## Across releases {#releases}

- `Measure::Attachment`, `LineSpacingStyle`, `LineSpacingFactor` and `Measured` are from R2000; R14 keeps no measurement, and `Measured` reads -1 there. `FlipFirst` and `FlipSecond`, which flip the arrowheads, are from R2007, `Measure::Version` from R2010.
- A leader keeps `Gap`, `Dimasz`, `Arrowhead` and its color up to R14, and its text annotation's `Height` and `Width` up to R2007, all of them derived from its dimension style and its annotation, which a save to those releases derives again, as AutoCAD does. A tolerance keeps its text height and gap up to R14.
- The arc length and jogged radius dimensions, and the multileader, are stored under their class, in every release, R14 included. A multileader's `MLeader::Version` is a field from R2010, and is kept before in extended data of the application `ACAD_MLEADERVER`. Its lines' own colors, linetypes and arrowheads are fields from R2010; before, AutoCAD keeps them in an XRECORD of the multileader's extension dictionary, which a save across R2010 makes and reads.
- AutoCAD's saves of one drawing need not agree on what a multileader computed: in `mleaders-r2000.dwg`, the R2000 save of the drawing `mleaders-r2018.dwg` comes from, the `Upright` multileader has a landing of length 0 where the R2018 save has 6.
