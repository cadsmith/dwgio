# Lines and curves

LINE, POINT, CIRCLE, ARC, ELLIPSE, SPLINE, RAY, XLINE and HELIX: what each holds, in which coordinate system, and how to get points on it. The examples read `entities-r2000.dwg`, which holds one of most kinds AutoCAD draws, `curves-r2000.dwg`, which adds splines by fit points and by control points, and `kinds-r2000.dwg`, which holds a helix among the kinds R2007 and later added.

## Lines and points {#lines}

A `Line` holds `Start` and `End`, a `Point` its `Position`, all in world coordinates. Their `Extrusion` sets only the direction of `Thickness`. A point is drawn as the header's `Pdmode` and `Pdsize` say; its `Angle` turns the marker, in radians.

```cpp
const auto entities = DwgIO::LoadFile("entities-r2000.dwg");
if (!entities)
{
  return 1;
}
const DwgIO::Drawing &drawing = entities->Drawing;
const DwgIO::Tables::BlockRecord *model = DwgIO::View::ModelSpace(drawing);
for (const DwgIO::Object *entity : DwgIO::View::Entities(drawing, *model))
{
  if (const auto *line = DwgIO::GetIf<DwgIO::Entities::Line>(entity))
  {
    const double length = std::hypot(line->End.X - line->Start.X, line->End.Y - line->Start.Y,
                                     line->End.Z - line->Start.Z);
    std::println("LINE {} from ({}, {}) to ({}, {}), length {:.3f}", DwgIO::HexOf(line->Base.Self), line->Start.X,
                 line->Start.Y, line->End.X, line->End.Y, length);
  }
  if (const auto *point = DwgIO::GetIf<DwgIO::Entities::Point>(entity))
  {
    std::println("POINT {} at ({}, {}), drawn as PDMODE {}", DwgIO::HexOf(point->Base.Self), point->Position.X,
                 point->Position.Y, drawing.Header.Pdmode);
  }
}
```

```text [Output]
LINE 2D3 from (0, 0) to (100, 50), length 111.803
POINT 2D4 at (10, 10), drawn as PDMODE 3
LINE 2F8 from (0, -50) to (100, -50), length 100.000
LINE 2FD from (0, -100) to (10, -100), length 10.000
```

## Circles and arcs {#arcs}

A `Circle` and an `Arc` keep `Center` in their object coordinate system, its Z the elevation, and `Radius`. An arc runs counterclockwise about `Extrusion` from `Start` to `End`, both in radians; `End` may be smaller than `Start` when the arc crosses angle 0. The point at angle a is the center plus the radius along (cos a, sin a), in the same OCS: convert it as [Coordinates and units](/tutorials/coordinates/#arbitrary) shows when the extrusion is not `Up`.

```cpp
const auto onArc = [](const DwgIO::Entities::Arc &arc, double angle) -> DwgIO::Vector
{
  return DwgIO::Vector{.X = arc.Center.X + (arc.Radius * std::cos(angle)),
                       .Y = arc.Center.Y + (arc.Radius * std::sin(angle)),
                       .Z = arc.Center.Z};
};
const auto shown = [](double value) -> double { return (std::round(value * 1e9) / 1e9) + 0.0; };
for (const DwgIO::Object *entity : DwgIO::View::Entities(drawing, *model))
{
  if (const auto *circle = DwgIO::GetIf<DwgIO::Entities::Circle>(entity))
  {
    std::println("CIRCLE {} at ({}, {}, {}), radius {}, thickness {}", DwgIO::HexOf(circle->Base.Self),
                 circle->Center.X, circle->Center.Y, circle->Center.Z, circle->Radius, circle->Thickness);
  }
  if (const auto *arc = DwgIO::GetIf<DwgIO::Entities::Arc>(entity))
  {
    const double sweep = std::fmod(arc->End - arc->Start + DwgIO::Turn, DwgIO::Turn);
    const DwgIO::Vector from = onArc(*arc, arc->Start);
    const DwgIO::Vector to = onArc(*arc, arc->End);
    std::println("ARC {} from ({}, {}) to ({}, {}), {} degrees, length {:.4f}", DwgIO::HexOf(arc->Base.Self),
                 shown(from.X), shown(from.Y), shown(to.X), shown(to.Y), sweep * 360 / DwgIO::Turn,
                 arc->Radius * sweep);
  }
}
```

```text [Output]
CIRCLE 2D5 at (50, 50, 0), radius 25, thickness 2
ARC 2D6 from (20, 0) to (0, 0), 180 degrees, length 31.4159
CIRCLE 2F9 at (0, -80, 0), radius 5, thickness 0
ARC 2FA from (30, -80) to (20, -80), 180 degrees, length 15.7080
CIRCLE 2FC at (0, 0, -120), radius 4, thickness 0
```

The circle 2FC lies in a turned plane: its extrusion is the world's -Y, and its center is (0, 120, 0) in the world, as [Coordinates and units](/tutorials/coordinates/#arbitrary) works out. The circle 2D5 was drawn with `THICKNESS` at 2, so it shows as a cylinder's wall.

## Ellipses {#ellipses}

An `Ellipse` keeps its `Center` and `Axis`, the semi-major axis from the center, in world coordinates; `Extrusion` is the normal of its plane. `Ratio` is the minor axis over the major one, at most 1. `Start` and `End` are parameters, not angles: the point of parameter t is the center plus cos t times the major axis plus sin t times the minor axis, which is `Ratio` times the normal crossed with the major axis. A whole ellipse runs from 0 to `DwgIO::Turn`.

```cpp
const auto onEllipse = [](const DwgIO::Entities::Ellipse &ellipse, double parameter) -> DwgIO::Vector
{
  const DwgIO::Vector n = ellipse.Extrusion;
  const DwgIO::Vector a = ellipse.Axis;
  const DwgIO::Vector b{.X = ellipse.Ratio * ((n.Y * a.Z) - (n.Z * a.Y)),
                        .Y = ellipse.Ratio * ((n.Z * a.X) - (n.X * a.Z)),
                        .Z = ellipse.Ratio * ((n.X * a.Y) - (n.Y * a.X))};
  const double c = std::cos(parameter);
  const double s = std::sin(parameter);
  return DwgIO::Vector{.X = ellipse.Center.X + (c * a.X) + (s * b.X),
                       .Y = ellipse.Center.Y + (c * a.Y) + (s * b.Y),
                       .Z = ellipse.Center.Z + (c * a.Z) + (s * b.Z)};
};
for (const DwgIO::Object *entity : DwgIO::View::Entities(drawing, *model))
{
  if (const auto *ellipse = DwgIO::GetIf<DwgIO::Entities::Ellipse>(entity))
  {
    const DwgIO::Vector from = onEllipse(*ellipse, ellipse->Start);
    const DwgIO::Vector to = onEllipse(*ellipse, ellipse->End);
    std::println("ELLIPSE {} at ({}, {}), major ({}, {}), ratio {}, from ({}, {}) to ({}, {})",
                 DwgIO::HexOf(ellipse->Base.Self), ellipse->Center.X, ellipse->Center.Y, ellipse->Axis.X,
                 ellipse->Axis.Y, ellipse->Ratio, shown(from.X), shown(from.Y), shown(to.X), shown(to.Y));
  }
}
```

```text [Output]
ELLIPSE 2D7 at (30, 30), major (10, 0), ratio 0.5, from (40, 30) to (40, 30)
ELLIPSE 2D8 at (60, 30), major (10, 0), ratio 0.5, from (70, 30) to (60, 35)
```

The parameter of a point is its angle only on a circle. On the ellipse 2D8 the point at 45 degrees from the center, seen from the center, has the parameter atan(tan 45° / `Ratio`), about 63.4 degrees. An elliptic arc AutoCAD draws by angles stores the parameters of those angles.

## Splines {#splines}

A `Spline` is a NURBS curve, held in its `Curve`, kept in one of two ways:

- by control points: `Degree`, `Knots`, `ControlPoints`, in world coordinates, and, when `Flags` has 4, `Weights`, one per control point;
- by fit points: `FitTolerance` holds the fit tolerance, `FitPoints` the points the curve passes through, in world coordinates, and `StartTangent` and `EndTangent` the tangents at its ends, or zero vectors when AutoCAD computed them.

A spline by fit points has no knots and no control points in the file: AutoCAD computes them when it loads the drawing, and its DXF writes both. dwgio gives what the file holds, so to draw such a spline you interpolate the fit points yourself.

```cpp
const auto curves = DwgIO::LoadFile("curves-r2000.dwg");
if (!curves)
{
  return 1;
}
for (const DwgIO::Object &object : curves->Drawing.Objects)
{
  if (const auto *spline = DwgIO::GetIf<DwgIO::Entities::Spline>(&object))
  {
    if (spline->Curve.FitTolerance)
    {
      std::println("SPLINE {} by {} fit points, tolerance {}, start tangent ({:.3f}, {:.3f})",
                   DwgIO::HexOf(spline->Base.Self), spline->Curve.FitPoints.size(), *spline->Curve.FitTolerance, spline->Curve.StartTangent.X,
                   spline->Curve.StartTangent.Y);
      continue;
    }
    std::string knots;
    for (const double knot : spline->Curve.Knots)
    {
      knots += std::format(" {}", knot);
    }
    std::println("SPLINE {} of degree {} by {} control points, flags {}, knots{}, {} weights",
                 DwgIO::HexOf(spline->Base.Self), spline->Curve.Degree, spline->Curve.ControlPoints.size(), spline->Curve.Flags, knots,
                 spline->Curve.Weights.size());
  }
}
```

```text [Output]
SPLINE 387 by 4 fit points, tolerance 1e-10, start tangent (-0.010, 1.000)
SPLINE 388 by 4 fit points, tolerance 1e-10, start tangent (0.000, 0.000)
SPLINE 389 of degree 3 by 5 control points, flags 0, knots 0 0 0 0 1 2 2 2 2, 0 weights
SPLINE 38A of degree 2 by 4 control points, flags 4, knots 0 0 0 1 2 2 2, 4 weights
SPLINE 38B of degree 2 by 4 control points, flags 0, knots 2 2 2 3 4 4 4, 0 weights
SPLINE 397 by 5 fit points, tolerance 1e-10, start tangent (0.789, -0.614)
```

There are as many knots as poles plus the degree plus 1, and they need not start at 0: those of 38B run from 2 to 4. A clamped spline, such as these, repeats its first and last knots degree plus 1 times, so it starts at its first pole and ends at its last. De Boor's algorithm gives the point at any parameter between the first and the last knot; on weighted poles it gives a rational spline's points too:

```cpp
const auto onSpline = [](const DwgIO::Entities::Spline &spline, double u) -> DwgIO::Vector
{
  const int degree = spline.Curve.Degree;
  const int poles = static_cast<int>(spline.Curve.ControlPoints.size());
  const std::vector<double> &knots = spline.Curve.Knots;
  int span = degree;
  while (span + 1 < poles && u >= knots[span + 1])
  {
    ++span;
  }
  std::vector<std::array<double, 4>> points;
  for (int at = span - degree; at <= span; ++at)
  {
    const DwgIO::Vector &pole = spline.Curve.ControlPoints[at];
    const double weight = spline.Curve.Weights.empty() ? 1.0 : spline.Curve.Weights[at];
    points.push_back({pole.X * weight, pole.Y * weight, pole.Z * weight, weight});
  }
  for (int round = 1; round <= degree; ++round)
  {
    for (int at = degree; at >= round; --at)
    {
      const double low = knots[span - degree + at];
      const double share = (u - low) / (knots[span + 1 + at - round] - low);
      for (int axis = 0; axis < 4; ++axis)
      {
        points[at][axis] = ((1 - share) * points[at - 1][axis]) + (share * points[at][axis]);
      }
    }
  }
  const std::array<double, 4> &last = points[degree];
  return DwgIO::Vector{.X = last[0] / last[3], .Y = last[1] / last[3], .Z = last[2] / last[3]};
};
for (const char *handle : {"389", "38A"})
{
  const auto &spline =
      DwgIO::Get<DwgIO::Entities::Spline>(*DwgIO::Find(curves->Drawing, DwgIO::ParseHandle(handle).value()));
  std::string points;
  for (const double u : {0.0, 0.5, 1.0, 2.0})
  {
    const DwgIO::Vector point = onSpline(spline, u);
    points += std::format(" ({:.4f}, {:.4f})", point.X, point.Y);
  }
  std::println("SPLINE {} at 0, 0.5, 1 and 2:{}", handle, points);
}
```

```text [Output]
SPLINE 389 at 0, 0.5, 1 and 2: (20.0000, 100.0000) (25.9375, 103.1250) (30.0000, 102.5000) (40.0000, 100.0000)
SPLINE 38A at 0, 0.5, 1 and 2: (20.0000, 110.0000) (24.6154, 113.8462) (26.6667, 113.3333) (35.0000, 115.0000)
```

The spline 38A weighs its second pole, (25, 115), 2 and the others 1, which draws the curve toward that pole: at 1 it passes (26.6667, 113.3333), where the same spline with no weights passes (27.5, 112.5).

## Rays and construction lines {#rays}

A `Ray` starts at `Start` and runs without end along `Direction`; an `XLine` runs without end both ways through `Start`. Both are in world coordinates, and `Direction` is a unit vector.

```cpp
for (const DwgIO::Object *entity : DwgIO::View::Entities(drawing, *model))
{
  if (const auto *ray = DwgIO::GetIf<DwgIO::Entities::Ray>(entity))
  {
    std::println("RAY from ({}, {}) toward {:.0f} degrees", ray->Start.X, ray->Start.Y,
                 std::atan2(ray->Direction.Y, ray->Direction.X) * 360 / DwgIO::Turn);
  }
  if (const auto *xline = DwgIO::GetIf<DwgIO::Entities::XLine>(entity))
  {
    std::println("XLINE through ({}, {}) along ({}, {})", xline->Start.X, xline->Start.Y, xline->Direction.X,
                 xline->Direction.Y);
  }
}
```

```text [Output]
RAY from (0, 100) toward 45 degrees
XLINE through (0, 120) along (1, 0)
```

## Helices {#helices}

A `Helix` is kept twice: as the spline AutoCAD draws, in `Curve`, as a `Spline` holds its own, and as its shape, in `Data`, a `HelixData`. `HelixData::BasePoint` is a point of the axis and `Axis` its direction; `Start` is where the helix starts, so the distance from `Start` to the axis is the base radius, and `Radius` is the top radius. `Turns` counts the turns, `Rise` is the height of one, `RightHanded` says it is right-handed, turning counterclockwise as it rises along the axis, and `Constraint` which of the turns' height (0), the turns (1) or the height (2) AutoCAD keeps when the helix is edited. All points are in world coordinates.

`kinds-r2000.dwg` holds a helix drawn about (80, 0, 0), from a base radius of 5 to a top radius of 3, 10 high, in 3 turns.

```cpp
const auto kinds = DwgIO::LoadFile("kinds-r2000.dwg");
if (!kinds)
{
  return 1;
}
for (const DwgIO::Object &object : kinds->Drawing.Objects)
{
  if (const auto *helix = DwgIO::GetIf<DwgIO::Entities::Helix>(&object))
  {
    const DwgIO::Entities::HelixData &coil = helix->Data;
    const DwgIO::Vector out{
        .X = coil.Start.X - coil.BasePoint.X, .Y = coil.Start.Y - coil.BasePoint.Y, .Z = coil.Start.Z - coil.BasePoint.Z};
    const double along = (out.X * coil.Axis.X) + (out.Y * coil.Axis.Y) + (out.Z * coil.Axis.Z);
    const double base = std::sqrt((out.X * out.X) + (out.Y * out.Y) + (out.Z * out.Z) - (along * along));
    std::println("HELIX about ({}, {}, {}): base radius {}, top radius {}, {} turns of {:.4f}, height {:.0f}",
                 coil.BasePoint.X, coil.BasePoint.Y, coil.BasePoint.Z, base, coil.Radius, coil.Turns, coil.Rise,
                 coil.Turns * coil.Rise);
    std::println("drawn as a spline of degree {} with {} control points", helix->Curve.Degree,
                 helix->Curve.ControlPoints.size());
  }
}
```

```text [Output]
HELIX about (80, 0, 0): base radius 5, top radius 3, 3 turns of 3.3333, height 10
drawn as a spline of degree 3 with 54 control points
```

## Across releases {#releases}

Every kind on this page reads the same way in every release, with these differences:

- From R2013 a spline carries `Options` and `Parameter`: the R2018 save of `curves-r2000.dwg` gives the splines by fit points `Options` 9, by fit points with the knot parameter used, and those by control points `Parameter` 15, custom knots. Before R2013 both are 0.
- `Curve::Flags` keeps only the closed (1), periodic (2) and rational (4) bits of the file. The planar and linear bits, and the 1024 that marks a spline by fit points in DXF's group 70, are not in a DWG file before R2013; `FitTolerance` tells the two ways apart in every release.
- A helix is stored under its class, and AutoCAD writes it in every release, R14 and R2000 included: dwgio reads it in any of them.
