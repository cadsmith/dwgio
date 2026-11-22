# Coordinates and units

dwgio gives every point as the file stores it, with no conversion. Some entities store world coordinates, others coordinates in a plane of their own, set by their extrusion. The table below says which is which; `Geometry::ToWorld` converts a point of an entity's plane to the world, and the header says what unit the drawing is in.

## World and object coordinates {#ocs}

The world coordinate system (WCS) is the drawing's fixed frame. A planar entity, such as a circle or a text, stores its points in its object coordinate system (OCS) instead: a frame whose Z axis is the entity's `Extrusion`, also called its normal. With the default extrusion, `Up`, the two are the same, which is why most 2D drawings never show the difference.

| Kind | Stored in world coordinates | Stored in object coordinates |
| :--- | :--- | :--- |
| `Line`, `Point` | `Start`, `End`, `Position` | |
| `Face3d`, `Ray`, `XLine`, `Ellipse`, `Spline`, `MLine`, `Leader`, `MText`, `Tolerance` | their points and axes | |
| `Polyline`, `Vertex` | a 3D polyline's and a mesh's vertices | a 2D polyline's vertices |
| `Circle`, `Arc` | | `Center`, its Z the elevation |
| `Text`, `AttDef`, `Attrib` | | `Lettering.Position` and `Lettering.Alignment` |
| `Insert` | | `Position` |
| `LwPolyline` | | each `LwPolyline::Vertex::Position`, at the height `Elevation` |
| `Solid`, `Trace`, `Shape` | | their corners and position |
| `Hatch` | | its paths and seeds, at the height `Elevation`, in the plane of `Normal` |
| `Dimension` | `Measure::DefinitionPoint` and the points of its `Form` but one | `Measure::TextMiddle`, `Measure::CloneInsertion` and an angular form's `ArcPoint`, in the plane of `Measure::Normal` |

A `Line` and a `Point` have an extrusion too, but their points stay in world coordinates; the extrusion sets the direction of their thickness. The comment on each member in the reference says which system it is in.

## From object to world coordinates {#arbitrary}

The OCS of a normal N follows the arbitrary axis algorithm of the DXF reference. When N is close to the world Z axis, its X and Y both smaller than 1/64, the OCS X axis is the world Y axis crossed with N; otherwise it is the world Z axis crossed with N. The Y axis is N crossed with the X axis. A point (x, y, z) of the OCS is then x times the X axis, plus y times the Y axis, plus z times N. `Geometry::ToWorld` does this, and `Geometry::ToObject` takes a world point back.
A circle whose extrusion is the world X axis lies in the world YZ plane. Its OCS X axis is the world Y axis, its Y axis the world Z axis, so the center (2, 3, 5) of its OCS is (5, 2, 3) in the world, and the point at angle 0, one radius along the OCS X axis, is one radius along the world Y axis.

```cpp
DwgIO::Drawing drawing = DwgIO::Create();
DwgIO::Entities::Circle sideCircle;
sideCircle.Center = DwgIO::Vector{.X = 2, .Y = 3, .Z = 5};
sideCircle.Radius = 1.5;
sideCircle.Extrusion = DwgIO::Vector{.X = 1, .Y = 0, .Z = 0};
if (!DwgIO::Edit::Add(drawing, sideCircle) || !DwgIO::SaveFile(drawing, "circle.dwg"))
{
  return 1;
}
const auto loaded = DwgIO::LoadFile("circle.dwg");
if (!loaded)
{
  return 1;
}
const auto roundOff = [](double value) -> double { return (std::round(value * 1e9) / 1e9) + 0.0; };
for (const DwgIO::Object &object : loaded->Drawing.Objects)
{
  if (const auto *circle = DwgIO::GetIf<DwgIO::Entities::Circle>(&object))
  {
    const DwgIO::Vector center = DwgIO::Geometry::ToWorld(circle->Center, circle->Extrusion);
    const DwgIO::Vector onRim{.X = circle->Center.X + circle->Radius, .Y = circle->Center.Y, .Z = circle->Center.Z};
    const DwgIO::Vector rim = DwgIO::Geometry::ToWorld(onRim, circle->Extrusion);
    std::println("center ({}, {}, {}), rim at angle 0 ({}, {}, {})", roundOff(center.X), roundOff(center.Y),
                 roundOff(center.Z), roundOff(rim.X), roundOff(rim.Y), roundOff(rim.Z));
  }
}
```

```text [Output]
center (5, 2, 3), rim at angle 0 (5, 3.5, 3)
```

An arc or a circle that AutoCAD mirrored often has the extrusion (0, 0, -1). Its OCS X axis is then the world's -X: the point (2, 3, 5) is (-2, 3, -5) in the world, and its angles run clockwise as seen from above.

```cpp
const DwgIO::Vector mirrored =
    DwgIO::Geometry::ToWorld(DwgIO::Vector{.X = 2, .Y = 3, .Z = 5}, DwgIO::Vector{.X = 0, .Y = 0, .Z = -1});
std::println("({}, {}, {})", roundOff(mirrored.X), roundOff(mirrored.Y), roundOff(mirrored.Z));
```

```text [Output]
(-2, 3, -5)
```

`entities-r2000.dwg` holds a circle AutoCAD drew at (0, 0, -120) in a UCS turned 90 degrees about the world X axis. That UCS's Z axis is the world's -Y, so the center is (0, 120, 0) in the world. The file stores the circle in its OCS: the extrusion is that Z axis, and the center is (0, 0, -120) again.

```cpp
const auto entities = DwgIO::LoadFile("entities-r2000.dwg");
if (!entities)
{
  return 1;
}
const auto &ucsCircle =
    DwgIO::Get<DwgIO::Entities::Circle>(*DwgIO::Find(entities->Drawing, DwgIO::ParseHandle("2FC").value()));
const DwgIO::Vector worldCenter = DwgIO::Geometry::ToWorld(ucsCircle.Center, ucsCircle.Extrusion);
const DwgIO::Vector &center = ucsCircle.Center;
const DwgIO::Vector &normal = ucsCircle.Extrusion;
std::println("stored ({}, {}, {}) along ({}, {}, {}), in the world ({}, {}, {})", center.X, center.Y, center.Z,
             roundOff(normal.X), roundOff(normal.Y), roundOff(normal.Z), roundOff(worldCenter.X),
             roundOff(worldCenter.Y), roundOff(worldCenter.Z));
```

```text [Output]
stored (0, 0, -120) along (0, -1, 0), in the world (0, 120, 0)
```

The same function places an insert's `Position`, a text's position and a lightweight polyline's vertices. For a point of a block, `Geometry::TransformOf` gives the whole transform of an insert, OCS included, as [Blocks and attributes](/tutorials/blocks/#entities) shows.

## Elevation and thickness {#thickness}

An entity in its OCS stands at a height along its normal, its elevation. A circle, an arc and a text keep it as the Z of their point; a lightweight polyline and a hatch keep it in `Elevation`, and a 2D polyline in `Polyline::Elevation`, its vertices' own Z left as AutoCAD wrote it. `Thickness` extrudes the entity along its normal by that height: a thick circle draws a cylinder's wall. Most entities have a thickness of 0.

```cpp
const auto curves = DwgIO::LoadFile("curves-r2000.dwg");
if (!curves)
{
  return 1;
}
for (const DwgIO::Object &object : curves->Drawing.Objects)
{
  if (const auto *polyline = DwgIO::GetIf<DwgIO::Entities::Polyline>(&object); polyline && polyline->Thickness != 0)
  {
    std::println("{} {}: elevation {}, thickness {}", DwgIO::TypeOf(object), DwgIO::HexOf(polyline->Base.Self),
                 polyline->Elevation, polyline->Thickness);
  }
  if (const auto *polyline = DwgIO::GetIf<DwgIO::Entities::LwPolyline>(&object); polyline && polyline->Thickness != 0)
  {
    std::println("{} {}: elevation {}, thickness {}, first vertex at Z {}", DwgIO::TypeOf(object),
                 DwgIO::HexOf(polyline->Base.Self), polyline->Elevation, polyline->Thickness,
                 polyline->Vertices.front().Position.Z);
  }
}
```

```text [Output]
POLYLINE_2D 2ED: elevation 3, thickness 2
LWPOLYLINE 386: elevation 2, thickness 1, first vertex at Z 0
```

## Angles {#angles}

Every angle in dwgio is in radians, where DXF gives degrees: an arc's `Start` and `End`, a text's `Rotation`, an insert's `Rotation`, a viewport's `Twist`. `DwgIO::Turn` is a full turn, 2π. An angle is measured counterclockwise from the X axis of the entity's OCS, about its normal; an arc runs counterclockwise from `Start` to `End`. The header's `Angbase` and `Angdir` change only how AutoCAD shows and takes angles, not how they are stored.

```cpp
const auto foundation = DwgIO::LoadFile("foundation-r2000.dwg");
if (!foundation)
{
  return 1;
}
const auto &arc =
    DwgIO::Get<DwgIO::Entities::Arc>(*DwgIO::Find(foundation->Drawing, DwgIO::ParseHandle("2CE").value()));
std::println("arc from {} to {} degrees", arc.Start * 360 / DwgIO::Turn, arc.End * 360 / DwgIO::Turn);
```

```text [Output]
arc from 0 to 180 degrees
```

## Units {#units}

The header says what a drawing unit is and how AutoCAD shows numbers. dwgio stores the numbers unchanged and converts none of them.

| Member | Says |
| :--- | :--- |
| `Insunits` | What one drawing unit stands for, from R2000: 0 unitless, 1 inches, 2 feet, 4 millimeters, 5 centimeters, 6 meters, and so on as AutoCAD's INSUNITS lists them |
| `Lunits`, `Luprec` | How lengths are shown: 1 scientific, 2 decimal, 3 engineering, 4 architectural, 5 fractional; and with how many decimals |
| `Aunits`, `Auprec` | How angles are shown: 0 decimal degrees, 1 degrees, minutes and seconds, 2 grads, 3 radians, 4 surveyor's units; and with how many decimals |
| `Angbase`, `Angdir` | The direction of angle 0, in radians, and whether angles are shown clockwise |
| `Ltscale`, `Celtscale` | The scale of every linetype, and of new entities' linetypes |
| `Elevation`, `Thickness` | The elevation and thickness new entities take in model space |

`MEASUREMENT`, whether the drawing is imperial (0) or metric (1), is not among the header's variables in a DWG file: it is the last two bytes of a block of its own, which dwgio keeps as bytes in `Drawing::Kept` under the name `measurement`.

```cpp
for (const char *file : {"foundation-r14.dwg", "foundation-r2000.dwg"})
{
  const auto reloaded = DwgIO::LoadFile(file);
  if (!reloaded)
  {
    return 1;
  }
  const DwgIO::Header &header = reloaded->Drawing.Header;
  const auto block = std::ranges::find(reloaded->Drawing.Kept, "measurement", &DwgIO::Section::Name);
  const int measurement = block == reloaded->Drawing.Kept.end() || block->Bytes.size() < 2
                              ? -1
                              : std::to_integer<int>(block->Bytes.back()) * 256 +
                                    std::to_integer<int>(block->Bytes[block->Bytes.size() - 2]);
  std::println("{}: INSUNITS {}, LUNITS {} to {} places, AUNITS {}, ANGBASE {}, MEASUREMENT {}", file,
               header.Insunits, header.Lunits, header.Luprec, header.Aunits, header.Angbase, measurement);
}
```

```text [Output]
foundation-r14.dwg: INSUNITS 0, LUNITS 2 to 4 places, AUNITS 0, ANGBASE 0, MEASUREMENT 1
foundation-r2000.dwg: INSUNITS 4, LUNITS 2 to 4 places, AUNITS 0, ANGBASE 0, MEASUREMENT 1
```

R14 has no `INSUNITS`, so its value reads 0. A drawing from `Create` has no measurement block until it is saved; the save writes one that says metric. A block record's `Units`, from R2007, gives the units of that block in `INSUNITS`' numbering, for inserting it into a drawing of other units.
