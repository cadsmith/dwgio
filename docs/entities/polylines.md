# Polylines

A polyline is a chain of vertices joined by straight or arc segments. LWPOLYLINE keeps its vertices inside itself; POLYLINE is followed by its VERTEX entities and a SEQEND, and also stands for 3D polylines and meshes. MLINE, a multiline, closes the page. The examples read `curves-r2000.dwg`, whose model space holds every kind of polyline AutoCAD makes: 2D polylines plain, curve-fit, spline-fit and thick, 3D polylines, two polygon meshes, a polyface mesh, lightweight polylines with widths and arcs, and two multilines.

## Lightweight polylines {#lwpolyline}

An `LwPolyline` holds its `Vertices`: the vertex's `Position` and the `Bulge` and `Widths` of the segment that starts at it. In a closed polyline, `LwPolylineFlags::Closed` in `Flags`, the last vertex's segment runs back to the first vertex. The positions are 2D points in the polyline's object coordinate system; the polyline stands at `Elevation` along its `Extrusion`, and the vertices' own Z is 0.

The width is kept one of two ways: `Width`, one constant width for every segment, or `Widths` on each segment, its start and end width. A polyline with neither has width 0.

```cpp
const auto curves = DwgIO::LoadFile("curves-r2000.dwg");
if (!curves)
{
  return 1;
}
const DwgIO::Drawing &drawing = curves->Drawing;
const DwgIO::Tables::BlockRecord *model = DwgIO::View::ModelSpace(drawing);
for (const DwgIO::Object *entity : DwgIO::View::Entities(drawing, *model))
{
  const auto *polyline = DwgIO::GetIf<DwgIO::Entities::LwPolyline>(entity);
  if (polyline == nullptr)
  {
    continue;
  }
  std::string widths;
  for (const DwgIO::Entities::LwPolyline::Vertex &segment : polyline->Vertices)
  {
    if (segment.Widths)
    {
      widths += std::format(" {}/{}", segment.Widths->Start, segment.Widths->End);
    }
  }
  widths = polyline->Width ? std::format(", width {}", *polyline->Width) : widths.empty() ? "" : ", widths" + widths;
  std::println("LWPOLYLINE {}: {} vertices, {}, elevation {}{}", DwgIO::HexOf(polyline->Base.Self),
               polyline->Vertices.size(), DwgIO::Has(polyline->Flags, DwgIO::Entities::LwPolylineFlags::Closed) ? "closed" : "open", polyline->Elevation,
               widths);
}
```

```text [Output]
LWPOLYLINE 384: 4 vertices, closed, elevation 0, widths 0.2/0.4 0.4/0.4 0/0 0/0
LWPOLYLINE 385: 3 vertices, open, elevation 0, width 0.3
LWPOLYLINE 386: 3 vertices, open, elevation 2, width 0.3
LWPOLYLINE 38C: 4 vertices, closed, elevation 0, width 0.3
LWPOLYLINE 399: 4 vertices, closed, elevation 0
```

`Flags` 128 says the linetype runs on around the vertices rather than starting again at each; AutoCAD sets it when `PLINEGEN` is on, as it was for 386. That polyline was drawn in a UCS turned about the X axis: its extrusion is the world's -Y, and its vertices convert to the world as [Coordinates and units](/tutorials/coordinates/#arbitrary) shows.

## Bulges and arcs {#bulges}

A segment's `Bulge` b is the tangent of a quarter of the angle its arc turns through, 0 for a straight segment. A positive bulge turns counterclockwise from the vertex to the next, a negative one clockwise; 1 is a half circle. For a segment of chord length c, from P1 to P2:

- the arc turns through θ = 4 atan b;
- its radius is r = c (1 + b²) / (4 |b|);
- its center is the chord's middle moved (1 - b²) / (4 b) times the chord to its left, the chord turned a quarter counterclockwise;
- its length is r |θ|.

```cpp
struct Bend
{
  DwgIO::Vector Center;
  double Radius;
  double Start;
  double End;
  double Length;
};
const auto bend = [](DwgIO::Vector from, DwgIO::Vector to, double bulge) -> Bend
{
  const double dx = to.X - from.X;
  const double dy = to.Y - from.Y;
  const double chord = std::hypot(dx, dy);
  const double radius = chord * (1 + (bulge * bulge)) / (4 * std::abs(bulge));
  const double left = (1 - (bulge * bulge)) / (4 * bulge);
  const DwgIO::Vector center{
      .X = ((from.X + to.X) / 2) - (left * dy), .Y = ((from.Y + to.Y) / 2) + (left * dx), .Z = from.Z};
  return Bend{.Center = center,
              .Radius = radius,
              .Start = std::atan2(from.Y - center.Y, from.X - center.X),
              .End = std::atan2(to.Y - center.Y, to.X - center.X),
              .Length = radius * std::abs(4 * std::atan(bulge))};
};
const auto length = [&](const DwgIO::Entities::LwPolyline &polyline) -> double
{
  double total = 0;
  const std::size_t count = polyline.Vertices.size();
  const std::size_t segments = DwgIO::Has(polyline.Flags, DwgIO::Entities::LwPolylineFlags::Closed) ? count : count - 1;
  for (std::size_t at = 0; at < segments; ++at)
  {
    const DwgIO::Entities::LwPolyline::Vertex &from = polyline.Vertices[at];
    const DwgIO::Vector to = polyline.Vertices[(at + 1) % count].Position;
    total += from.Bulge == 0 ? std::hypot(to.X - from.Position.X, to.Y - from.Position.Y)
                             : bend(from.Position, to, from.Bulge).Length;
  }
  return total;
};
const auto &outline = DwgIO::Get<DwgIO::Entities::LwPolyline>(*DwgIO::Find(drawing, DwgIO::ParseHandle("384").value()));
const DwgIO::Entities::LwPolyline::Vertex &second = outline.Vertices[1];
const Bend arc = bend(second.Position, outline.Vertices[2].Position, second.Bulge);
std::println("bulge {:.4f} from ({}, {}): center ({:.4f}, {:.4f}), radius {:.4f}, from {:.0f} to {:.0f} degrees",
             second.Bulge, second.Position.X, second.Position.Y, arc.Center.X, arc.Center.Y, arc.Radius,
             arc.Start * 360 / DwgIO::Turn, arc.End * 360 / DwgIO::Turn);
std::println("LWPOLYLINE 384 is {:.4f} long", length(outline));
```

```text [Output]
bulge 1.0000 from (10, 80): center (10.0000, 85.0000), radius 5.0000, from -90 to 90 degrees
LWPOLYLINE 384 is 45.7080 long
```

The second segment of 384 runs from (10, 80) to (10, 90) with a bulge of 1: a half circle of radius 5 about (10, 85), counterclockwise from -90 to 90 degrees, bulging out to (15, 85). With the three straight sides of 10, the closed polyline is 30 + 5π long.

## 2D and 3D polylines {#polyline}

A `Polyline` holds what the whole line shares; its points are the `Vertex` entities that follow it. `Flags` says which kind it is, and `TypeOf` names it: `POLYLINE_2D`, `POLYLINE_3D` with 8, `POLYLINE_MESH` with 16, `POLYLINE_PFACE` with 64. Its vertices are `VERTEX_2D`, `VERTEX_3D`, `VERTEX_MESH`, `VERTEX_PFACE` and `VERTEX_PFACE_FACE`. A SEQEND, `SeqEnd`, closes them.

From R2004 the polyline lists its vertices in `Vertices`. Up to R2000 it names the first and the last in `First` and `Last`, and each vertex names the next in `Common::Next`. `View::Vertices` reads both:

```cpp
const auto vertices = [](const DwgIO::Drawing &drawing, const DwgIO::Entities::Polyline &polyline)
{
  return DwgIO::View::Vertices(drawing, polyline);
};
for (const char *file : {"curves-r2000.dwg", "curves-r2004.dwg"})
{
  const auto loaded = DwgIO::LoadFile(file);
  if (!loaded)
  {
    return 1;
  }
  const auto &plain =
      DwgIO::Get<DwgIO::Entities::Polyline>(*DwgIO::Find(loaded->Drawing, DwgIO::ParseHandle("2CF").value()));
  std::string points;
  for (const DwgIO::Entities::Vertex *vertex : vertices(loaded->Drawing, plain))
  {
    points += std::format(" ({}, {})", vertex->Position.X, vertex->Position.Y);
  }
  std::println("{}: {} listed, first {}:{}", DwgIO::NameOf(loaded->Drawing.Release), plain.Vertices.size(),
               DwgIO::HexOf(plain.First), points);
}
```

```text [Output]
R2000: 0 listed, first 2D1: (0, 0) (10, 0) (10, 10) (0, 10)
R2004: 4 listed, first 0: (0, 0) (10, 0) (10, 10) (0, 10)
```

A 2D polyline's vertices are in its object coordinate system, set by the polyline's `Extrusion`, at the height `Polyline::Elevation`; a vertex's own Z is what AutoCAD left there, as [Coordinates and units](/tutorials/coordinates/#thickness) shows. A 3D polyline's and a mesh's vertices are in world coordinates.

A 2D vertex carries the segment that starts at it, as a lightweight polyline's does: `Bulge`, and the start and end widths in `StartWidth` and `EndWidth`. The polyline's own `StartWidth` and `EndWidth` are the widths a vertex takes when it gives none; the file keeps a pair on every vertex, so read the vertex's.

## Curve-fit and spline-fit polylines {#fitted}

PEDIT's Fit and Spline add vertices AutoCAD computed, and mark them in `Vertex::Flags`:

| `Polyline::Flags` | The vertices drawn | The vertices kept to edit from |
| :--- | :--- | :--- |
| 2, curve-fit | all of them: those AutoCAD added have 1, and every segment has a bulge | the vertices without 1 |
| 4, spline-fit | those with 8, joined by straight segments | the frame, with 16 |

`Polyline::Curve` names the spline: 5 quadratic, 6 cubic. The header's `Splframe` says whether AutoCAD shows the frame. To draw a spline-fit polyline, skip the vertices with 16.

```cpp
const auto near = [](double value) -> double { return (std::round(value * 1e4) / 1e4) + 0.0; };
const auto &fit = DwgIO::Get<DwgIO::Entities::Polyline>(*DwgIO::Find(drawing, DwgIO::ParseHandle("2D5").value()));
const std::vector<const DwgIO::Entities::Vertex *> fitted = vertices(drawing, fit);
for (std::size_t at = 0; at + 1 < fitted.size(); ++at)
{
  const DwgIO::Entities::Vertex &from = *fitted[at];
  const Bend part = bend(from.Position, fitted[at + 1]->Position, from.Bulge);
  std::println("{} ({}, {}), bulge {}: center ({}, {}), radius {}", DwgIO::Has(from.Flags, DwgIO::Entities::VertexFlags::CurveFitExtra) ? "added" : "given",
               near(from.Position.X), near(from.Position.Y), near(from.Bulge), near(part.Center.X),
               near(part.Center.Y), near(part.Radius));
}
const auto &spline = DwgIO::Get<DwgIO::Entities::Polyline>(*DwgIO::Find(drawing, DwgIO::ParseHandle("2DE").value()));
const auto all = vertices(drawing, spline);
const auto drawn = std::ranges::count_if(all, [](const auto *vertex) { return DwgIO::Has(vertex->Flags, DwgIO::Entities::VertexFlags::SplineFit); });
std::println("spline-fit, curve {}: {} vertices, {} drawn, {} in the frame", spline.Curve, all.size(), drawn,
             std::ranges::count_if(all, [](const auto *vertex) { return DwgIO::Has(vertex->Flags, DwgIO::Entities::VertexFlags::ControlPoint); }));
```

```text [Output]
given (20, 0), bulge -0.1989: center (25, 0), radius 5
added (21.4645, 3.5355), bulge -0.1989: center (25, 0), radius 5
given (25, 5), bulge -0.4142: center (25, 2.5), radius 2.5
added (27.5, 2.5), bulge 0.4142: center (30, 2.5), radius 2.5
given (30, 0), bulge 0.1989: center (30, 5), radius 5
added (33.5355, 1.4645), bulge 0.1989: center (30, 5), radius 5
spline-fit, curve 6: 13 vertices, 9 drawn, 4 in the frame
```

The third segment checks the formula: from (25, 5) to (27.5, 2.5) with a bulge of -0.4142, which is -tan 22.5°, it turns a quarter clockwise about (25, 2.5), which lies 2.5 from both ends. The spline-fit polyline 2DE keeps its 4 picked points as the frame and draws the 9 vertices AutoCAD computed.

## Meshes {#meshes}

A polygon mesh, `Flags` 16, is a grid of `Rows` by `Columns` vertices (M by N in AutoCAD's terms), given row by row: the vertex at row m and column n is the m × `Columns` + n'th. `Flags` 1 closes it along M and 32 along N. A smoothed mesh has `Curve` 5, 6 or 8, and `Density` by `Depth` vertices it computed, flagged 8, before the original ones, flagged 16.

A polyface mesh, `Flags` 64, has `Rows` vertices and `Columns` faces. Its vertices come first, with `Flags` 192; then its faces, with 128 alone, whose `First` to `Fourth` count the vertices from 1. A negative index hides the edge that starts at that vertex, and `Fourth` is 0 for a triangle.

```cpp
for (const char *handle : {"306", "30E", "37B"})
{
  const DwgIO::Object &object = *DwgIO::Find(drawing, DwgIO::ParseHandle(handle).value());
  const auto &mesh = DwgIO::Get<DwgIO::Entities::Polyline>(object);
  const std::vector<const DwgIO::Entities::Vertex *> all = vertices(drawing, mesh);
  std::println("{} {}: flags {}, rows {}, columns {}, {} vertices", DwgIO::TypeOf(object), handle, mesh.Flags,
               mesh.Rows, mesh.Columns, all.size());
  for (const DwgIO::Entities::Vertex *face : all)
  {
    if (face->Flags == 128)
    {
      std::println("  face {} {} {} {}", face->First, face->Second, face->Third, face->Fourth);
    }
  }
}
```

```text [Output]
POLYLINE_MESH 306: flags 16, rows 2, columns 3, 6 vertices
POLYLINE_MESH 30E: flags 52, rows 4, columns 4, 58 vertices
POLYLINE_PFACE 37B: flags 64, rows 5, columns 2, 7 vertices
  face 1 -2 3 4
  face 1 2 5 0
```

The mesh 306 is 2 rows of 3 vertices. 30E is smoothed, `Flags` 4, and closed along N, 32: its 7 by 6 computed vertices come before its 16 own. The polyface mesh 37B has 5 vertices and 2 faces: a quadrilateral whose edge from vertex 2 is hidden, and a triangle.

## Multilines {#mlines}

An `MLine` draws the lines of its `MLINESTYLE`, which `Style` names, side by side along a path. Each `Joint` is a vertex in world coordinates with the `Direction` of the segment that starts there and the `Miter` the lines turn along. It has one `Strand` per line of the style, whose `Spans` place that line: the first is how far along the miter it lies from the vertex, the second how far along the segment from there it starts, and any more where it breaks. `Scale` multiplies the style's offsets, `Justification` puts the vertices on the top (0), the middle (1) or the bottom (2) line, and `Flags` 2 closes the multiline.

```cpp
for (const DwgIO::Object *entity : DwgIO::View::Entities(drawing, *model))
{
  if (const auto *mline = DwgIO::GetIf<DwgIO::Entities::MLine>(entity))
  {
    const DwgIO::Entities::Joint &corner = mline->Joints[1];
    std::string lines;
    for (const DwgIO::Entities::Strand &strand : corner.Strands)
    {
      const double along = strand.Spans.front();
      lines += std::format(" ({:.4f}, {:.4f})", corner.Position.X + (along * corner.Miter.X),
                           corner.Position.Y + (along * corner.Miter.Y));
    }
    std::println("MLINE {}: {} joints, scale {}, justification {}, {}; at ({}, {}) its lines pass{}",
                 DwgIO::HexOf(mline->Base.Self), mline->Joints.size(), mline->Scale, mline->Justification,
                 (mline->Flags & 2) != 0 ? "closed" : "open", corner.Position.X, corner.Position.Y, lines);
  }
}
```

```text [Output]
MLINE 41E: 3 joints, scale 2, justification 0, open; at (10, -60) its lines pass (10.0000, -60.0000) (8.0000, -62.0000)
MLINE 41F: 4 joints, scale 2, justification 1, closed; at (30, -60) its lines pass (31.0000, -59.0000) (29.0000, -61.0000)
```

## Across releases {#releases}

- LWPOLYLINE exists from R14, where it is stored under its class; dwgio reads it the same in every release.
- A polyline's vertices are chained through `First`, `Last` and `Common::Next` up to R2000, and listed in `Vertices` from R2004. A save to another release converts between the two.
- From R2010 a lightweight polyline may keep an `Id` on each segment and a 2D vertex its `Id`; both are 0 or absent before.
- `PLINETYPE` decides whether AutoCAD makes a new 2D polyline as an LWPOLYLINE or a POLYLINE, and both kinds sit in one drawing, as they do here. dwgio converts neither into the other.
