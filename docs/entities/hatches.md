# Hatches

A HATCH fills an area with a pattern, a solid color or a gradient. The area is a set of boundary paths, each a polyline or a loop of lines, arcs, elliptic arcs and splines. The examples read `curves-r2000.dwg`, which holds five hatches: ANSI31 inside a lightweight polyline with an arc, a solid fill inside three lines, a user-defined crosshatch inside an ellipse, ANSI33 inside a closed spline, not associative, and ANSI37 inside a square with a circle as an island.

## The fill {#fill}

| Member | Holds |
| :--- | :--- |
| `Name` | The pattern's name, such as `ANSI31`, or `SOLID` for a solid fill; some AutoCAD versions write `_SOLID` and `_U` |
| `Solid` | A solid fill: no pattern follows |
| `Kind` | 0 user-defined, 1 predefined in `acad.pat`, 2 custom |
| `Angle`, `Scale` | The pattern's angle in radians and its scale, or a user-defined pattern's spacing |
| `Doubled` | A user-defined pattern drawn twice, crossed at right angles; AutoCAD also sets it on some predefined patterns, such as ANSI33 and ANSI37 here, whose `PatternLines` already hold every line |
| `PatternLines` | The pattern's lines, already turned and scaled |
| `Style` | Which islands are filled: 0 every other one, 1 the outermost area only, 2 all of it |
| `Associative` | The hatch follows its boundary entities when they change |
| `Gradient` | From R2004, a gradient fill when `Gradient::Shown` is not 0 |

```cpp
const auto curves = DwgIO::LoadFile("curves-r2000.dwg");
if (!curves)
{
  return 1;
}
const DwgIO::Drawing &drawing = curves->Drawing;
for (const DwgIO::Object &object : drawing.Objects)
{
  if (const auto *hatch = DwgIO::GetIf<DwgIO::Entities::Hatch>(&object))
  {
    const std::string fill = hatch->Solid ? std::string("solid")
                                          : std::format("kind {}, angle {:.0f}, scale {}{}, {} lines", hatch->Kind,
                                                        hatch->Angle * 360 / DwgIO::Turn, hatch->Scale,
                                                        hatch->Kind == 0 && hatch->Doubled ? ", doubled" : "",
                                                        hatch->PatternLines.size());
    std::println("HATCH {} {}: {}, style {}, {} paths{}", DwgIO::HexOf(hatch->Base.Self), hatch->Name, fill,
                 hatch->Style, hatch->Paths.size(), hatch->Associative ? ", associative" : "");
  }
}
```

```text [Output]
HATCH 38F ANSI31: kind 1, angle 0, scale 2, 1 lines, style 1, 1 paths, associative
HATCH 394 _SOLID: solid, style 1, 1 paths, associative
HATCH 396 _U: kind 0, angle 45, scale 2, doubled, 2 lines, style 1, 1 paths, associative
HATCH 398 ANSI33: kind 1, angle 30, scale 2, 2 lines, style 1, 1 paths
HATCH 39B ANSI37: kind 1, angle 0, scale 1, 2 lines, style 1, 2 paths, associative
```

## The pattern's lines {#stripes}

A pattern is a set of line families, each a `PatternLine`: lines at `Angle`, one of them through (`X`, `Y`), the next one (`Run`, `Rise`) further on, and so on across the hatch. `Dashes` gives the dash and gap lengths along each line, a dash positive and a gap negative; a line with no dashes is continuous. The file holds them turned by the hatch's `Angle` and scaled by its `Scale`, ready to draw: there is no need to read `acad.pat`.

```cpp
const auto &ansi33 = DwgIO::Get<DwgIO::Entities::Hatch>(*DwgIO::Find(drawing, DwgIO::ParseHandle("398").value()));
for (const DwgIO::Entities::PatternLine &stripe : ansi33.PatternLines)
{
  std::string dashes;
  for (const double dash : stripe.Dashes)
  {
    dashes += std::format(" {}", dash);
  }
  std::println("lines at {:.0f} degrees through ({:.4f}, {:.4f}), {:.4f} apart, dashes{}",
               stripe.Angle * 360 / DwgIO::Turn, stripe.X, stripe.Y, std::hypot(stripe.Run, stripe.Rise),
               dashes.empty() ? " none" : dashes);
}
```

```text [Output]
lines at 75 degrees through (0.0000, 0.0000), 12.7000 apart, dashes none
lines at 75 degrees through (7.7771, 4.4901), 12.7000 apart, dashes 6.35 -3.175
```

ANSI33 is two families at 45 degrees, one continuous and one dashed, 6.35 apart in `acadiso.pat`: at scale 2 they are 12.7 apart, and the hatch's 30 degrees turned them to 75.

## Boundary paths {#paths}

Each `Path` is one loop. `Flags` 1 marks an external loop, 16 an outermost one, 2 a polyline path, 4 a loop AutoCAD derived by picking a point inside, and 8 a loop around a text. A polyline path holds `Vertices`, the others `Edges`, each one of four kinds:

| Edge | Holds |
| :--- | :--- |
| `Edges::Line` | `Start` and `End` |
| `Edges::Arc` | `Center`, `Radius`, `Start` and `End` angles in radians, and `CounterClockwise` when it runs counterclockwise |
| `Edges::Ellipse` | `Center`, `Major`, the end of the major axis from the center, `Ratio`, `Start` and `End` in radians, and `CounterClockwise` |
| `Edges::Spline` | `Degree`, `Rational`, `Periodic`, `Knots` and `ControlPoints`, each a `Node` with its weight; from R2010, `FitPoints` and the end tangents `StartTangent` and `EndTangent` |

```cpp
for (const DwgIO::Object &object : drawing.Objects)
{
  const auto *hatch = DwgIO::GetIf<DwgIO::Entities::Hatch>(&object);
  if (hatch == nullptr)
  {
    continue;
  }
  for (const DwgIO::Entities::Path &path : hatch->Paths)
  {
    std::string edges;
    for (const DwgIO::Entities::Edges::Edge &edge : path.Edges)
    {
      if (const auto *line = std::get_if<DwgIO::Entities::Edges::Line>(&edge))
      {
        edges += std::format(" line ({}, {})-({}, {})", line->Start.X, line->Start.Y, line->End.X, line->End.Y);
      }
      else if (const auto *arc = std::get_if<DwgIO::Entities::Edges::Arc>(&edge))
      {
        edges += std::format(" arc about ({}, {}) radius {}, {:.0f} to {:.0f} degrees", arc->Center.X, arc->Center.Y,
                             arc->Radius, arc->Start * 360 / DwgIO::Turn, arc->End * 360 / DwgIO::Turn);
      }
      else if (const auto *ellipse = std::get_if<DwgIO::Entities::Edges::Ellipse>(&edge))
      {
        edges += std::format(" ellipse about ({}, {}) major ({}, {}) ratio {}", ellipse->Center.X,
                             ellipse->Center.Y, ellipse->Major.X, ellipse->Major.Y, ellipse->Ratio);
      }
      else
      {
        const auto &spline = std::get<DwgIO::Entities::Edges::Spline>(edge);
        edges += std::format(" spline of degree {}, {} poles, {} knots", spline.Degree, spline.ControlPoints.size(),
                             spline.Knots.size());
      }
    }
    std::println("{} flags {}:{}", DwgIO::HexOf(hatch->Base.Self), path.Flags, edges);
  }
}
```

```text [Output]
38F flags 1: line (0, -80)-(10, -80) arc about (10, -75) radius 5, 270 to 450 degrees line (10, -70)-(0, -70) line (0, -70)-(0, -80)
394 flags 1: line (20, -80)-(30, -80) line (30, -80)-(25, -70) line (25, -70)-(20, -80)
396 flags 1: ellipse about (45, -75) major (5, 0) ratio 0.6
398 flags 1: spline of degree 3, 7 poles, 11 knots
39B flags 1: line (80, -90)-(90, -90) line (90, -90)-(90, -80) line (90, -80)-(80, -80) line (80, -80)-(80, -90)
39B flags 16: arc about (85, -85) radius 1.5, 0 to 360 degrees
```

The paths are in the hatch's object coordinate system: in the plane of `Normal`, at the height `Elevation`. Convert them to the world as [Coordinates and units](/tutorials/coordinates/#arbitrary) shows. AutoCAD made every path here an edge path, even from the lightweight polyline of 38F, whose bulged side became an arc edge from 270 to 450 degrees: angles past a full turn are kept as written.

## Polyline paths {#polyline}

A polyline path, `Flags` with 2, holds its vertices as `Edges::Node`s: `Position`, and in `Weight` the bulge of the segment to the next vertex when `Bulged` is set. `Closed` says the last vertex joins the first. A bulge turns into an arc as [Polylines](/entities/polylines/#bulges) shows.

This hatch is built here with a polyline path whose second side bulges out in a half circle, and a gradient, which needs R2004:

```cpp
DwgIO::Drawing made = DwgIO::Create(DwgIO::Version::R2004);
DwgIO::Entities::Hatch shaded;
shaded.Name = "SOLID";
shaded.Solid = true;
DwgIO::Entities::Path outline{.Flags = 1 | 2, .Bulged = true, .Closed = true};
outline.Vertices = {
    DwgIO::Entities::Edges::Node{.Position = DwgIO::Vector{.X = 0, .Y = 0, .Z = 0}, .Weight = 0},
    DwgIO::Entities::Edges::Node{.Position = DwgIO::Vector{.X = 10, .Y = 0, .Z = 0}, .Weight = 1},
    DwgIO::Entities::Edges::Node{.Position = DwgIO::Vector{.X = 10, .Y = 10, .Z = 0}, .Weight = 0},
    DwgIO::Entities::Edges::Node{.Position = DwgIO::Vector{.X = 0, .Y = 10, .Z = 0}, .Weight = 0}};
shaded.Paths.push_back(outline);
shaded.Gradient.Shown = 1;
shaded.Gradient.Angle = DwgIO::Turn / 8;
if (!DwgIO::Edit::Add(made, shaded) || !DwgIO::SaveFile(made, "shaded.dwg"))
{
  return 1;
}
const auto shadedLoaded = DwgIO::LoadFile("shaded.dwg");
if (!shadedLoaded)
{
  return 1;
}
for (const DwgIO::Object &object : shadedLoaded->Drawing.Objects)
{
  if (const auto *hatch = DwgIO::GetIf<DwgIO::Entities::Hatch>(&object))
  {
    const DwgIO::Entities::Path &path = hatch->Paths.front();
    std::string vertices;
    for (const DwgIO::Entities::Edges::Node &node : path.Vertices)
    {
      vertices += std::format(" ({}, {}) bulge {}", node.Position.X, node.Position.Y, node.Weight);
    }
    const DwgIO::Entities::Gradient &gradient = hatch->Gradient;
    std::println("{}, closed {}:{}", hatch->Name, path.Closed, vertices);
    std::println("gradient {} at {:.0f} degrees, from {:08X} to {:08X}", gradient.Name,
                 gradient.Angle * 360 / DwgIO::Turn, static_cast<std::uint32_t>(gradient.Colors.front().Rgb),
                 static_cast<std::uint32_t>(gradient.Colors.back().Rgb));
  }
}
```

```text [Output]
SOLID, closed true: (0, 0) bulge 0 (10, 0) bulge 1 (10, 10) bulge 0 (0, 10) bulge 0
gradient LINEAR at 45 degrees, from C20000FF to C2FFFF00
```

A gradient fill is a solid hatch with `Gradient::Shown` set. `Name` is the gradient's shape, such as `LINEAR`, `CYLINDER` or `SPHERICAL`, `Angle` turns it, `Shift` moves its center, and `Colors` are its colors, each at a `Place` from 0 to 1. `Single` marks a gradient of one color, which `Tint` lightens or darkens. Every hatch from R2004 holds a gradient: one that is no gradient fill has `Shown` 0 and the values shown here, blue to yellow, which AutoCAD writes in each of the R2004 save's hatches and dwgio gives a new one. A shade's `Rgb` is stored with the color method in its top byte, 0xC2 for a true color, then red, green and blue.

## Associativity {#associative}

An associative hatch names the entities its boundary came from in each path's `Sources`, and each of them names the hatch among its `Base::Reactors`. AutoCAD redraws the hatch when one of them changes; dwgio does not. A hatch made with `HPASSOC` off, such as 398, names none. `Edit::Remove` on a boundary entity clears every path's `Sources` and the hatch's `Associative`: the hatch keeps its paths and no longer follows anything.

```cpp
const auto &square = DwgIO::Get<DwgIO::Entities::Hatch>(*DwgIO::Find(drawing, DwgIO::ParseHandle("39B").value()));
for (const DwgIO::Entities::Path &path : square.Paths)
{
  for (const DwgIO::Handle source : path.Sources)
  {
    const DwgIO::Object &boundary = *DwgIO::Find(drawing, source);
    const std::vector<DwgIO::Handle> &reactors = DwgIO::BaseOf(boundary).Reactors;
    std::println("path {} from {} {}, which names the hatch back: {}", path.Flags, DwgIO::TypeOf(boundary),
                 DwgIO::HexOf(source), std::ranges::contains(reactors, square.Base.Self));
  }
}
```

```text [Output]
path 1 from LWPOLYLINE 399, which names the hatch back: true
path 16 from CIRCLE 39A, which names the hatch back: true
```

The square is the outer path, `Flags` 1, and the circle the island, `Flags` 16. The hatch's `Style` 1 fills the outermost area only, so the circle stays empty.

## Seed points {#seeds}

`Seeds` holds the points picked inside the area to find the boundary, in the hatch's object coordinate system. A hatch made by selecting objects, as all of these were, keeps a single seed at the origin. `Pixel` is the pixel size AutoCAD found a picked boundary with, kept when a path has `Flags` 4.

## MPOLYGON {#mpolygon}

An MPOLYGON, the polygon of AutoCAD Map and Civil 3D, holds paths much as a hatch does. dwgio does not type it: it loads as an `Unknown`, with its `Common` read and its own data kept as bytes, and saves back to its own release unchanged. [The object model](/guide/objects/#unknown) says what you can do with one.

## Across releases {#releases}

- R14 stores HATCH under its class; dwgio reads it as in later releases.
- `Gradient` is from R2004; a save to R2000 drops a gradient and notes it. The hatches here also carry extended data of the applications `GradientColor1ACI` and `GradientColor2ACI`, the gradient colors AutoCAD would use, in their R2000 and R2004 saves alike.
- A spline edge's fit points and end tangents are from R2010: the spline edge of 398 has 5 fit points in the R2018 save of this drawing and none in this one.
