# Other entities

The other kinds dwgio types: inserts, filled faces, shapes, viewports, images and wipeouts, underlays, OLE frames, tables, the ACIS bodies and surfaces, meshes, lights and section planes, and what an entity of a kind dwgio does not type gives you. The examples read `entities-r2000.dwg` for the inserts, faces and shape; `rasters-r2000.dwg`, which holds two images of `dot.png` and two wipeouts; `cells-r2004.dwg` and `cells-r2018.dwg`, the same filled table in two releases; `solids-r2000.dwg` and `solids-r2018.dwg`, which hold a box with four holes, a cone, a torus and a cylinder; and `kinds-r2000.dwg`, which holds one of each kind R2007 and later added, from a mesh to the three underlays.

## Inserts {#inserts}

An `Insert` places a block: [Blocks and attributes](/tutorials/blocks/) reads one, its block's entities and its attributes. With `Columns` and `Rows` set it is a MINSERT, `TypeOf` names it so, and it places the block as a grid: column c and row r land at `Position` plus c × `ColumnSpacing` along the insert's X axis and r × `RowSpacing` along its Y axis, both turned by `Rotation`.

```cpp
const auto entities = DwgIO::LoadFile("entities-r2000.dwg");
if (!entities)
{
  return 1;
}
const DwgIO::Drawing &drawing = entities->Drawing;
for (const DwgIO::Object &object : drawing.Objects)
{
  const auto *insert = DwgIO::GetIf<DwgIO::Entities::Insert>(&object);
  if (insert == nullptr || !insert->Columns)
  {
    continue;
  }
  const double cos = std::cos(insert->Rotation);
  const double sin = std::sin(insert->Rotation);
  std::string copies;
  for (int row = 0; row < *insert->Rows; ++row)
  {
    for (int column = 0; column < *insert->Columns; ++column)
    {
      const double x = column * insert->ColumnSpacing;
      const double y = row * insert->RowSpacing;
      copies += std::format(" ({}, {})", insert->Position.X + (x * cos) - (y * sin),
                            insert->Position.Y + (x * sin) + (y * cos));
    }
  }
  std::println("{} of {}: {} columns {} apart, {} rows {} apart:{}", DwgIO::TypeOf(object),
               DwgIO::Get<DwgIO::Tables::BlockRecord>(*DwgIO::Find(drawing, insert->Block)).Record.Name,
               *insert->Columns, insert->ColumnSpacing, *insert->Rows, insert->RowSpacing, copies);
}
```

```text [Output]
MINSERT of Dot: 2 columns 10 apart, 3 rows 15 apart: (300, 0) (310, 0) (300, 15) (310, 15) (300, 30) (310, 30)
```

## Solids, traces and 3D faces {#faces}

A `Solid`, a `Trace` and a `Face3d` keep four `Corners`; a triangle repeats its third corner as the fourth. A solid and a trace are flat and filled, in their object coordinate system, all at the Z of the first corner; a SOLID is not a 3D solid, which is `Solid3d`. Their corners go across, not around: the outline runs through the first, second, fourth and third. A 3D face's corners are in world coordinates and go around, and `Face3d::Flags` hides its edges: 1 the first, 2 the second, 4 the third, 8 the fourth.

```cpp
const auto outline = [](const DwgIO::Corners &corners, bool across) -> std::string
{
  std::string out;
  for (const int at : across ? std::array{0, 1, 3, 2} : std::array{0, 1, 2, 3})
  {
    out += std::format(" ({}, {})", corners.at(at).X, corners.at(at).Y);
  }
  return out;
};
for (const DwgIO::Object &object : drawing.Objects)
{
  if (const auto *solid = DwgIO::GetIf<DwgIO::Entities::Solid>(&object))
  {
    std::println("SOLID:{}", outline(solid->Corners, true));
  }
  if (const auto *trace = DwgIO::GetIf<DwgIO::Entities::Trace>(&object))
  {
    std::println("TRACE:{}", outline(trace->Corners, true));
  }
  if (const auto *face = DwgIO::GetIf<DwgIO::Entities::Face3d>(&object))
  {
    std::println("3DFACE:{}, hidden edges {}", outline(face->Corners, false), face->Flags);
  }
}
```

```text [Output]
3DFACE: (0, 0) (10, 0) (10, 10) (0, 10), hidden edges 4
SOLID: (20, 0) (30, 0) (30, 10) (20, 10)
TRACE: (0, -10) (50, -10) (50, -8) (0, -8)
```

## Shapes {#shapes}

A `Shape` draws one shape of a shape file, by its `Number`; DXF names it instead, and the name is in the file, not the drawing. `Style` is the text style record that loaded the file, one with `Shape` set. `Position` is in the shape's object coordinate system, `Size` its height, and `Rotation`, `Width` and `Oblique` place it as they do a text.

```cpp
for (const DwgIO::Object &object : drawing.Objects)
{
  if (const auto *shape = DwgIO::GetIf<DwgIO::Entities::Shape>(&object))
  {
    const auto &file = DwgIO::Get<DwgIO::Tables::Style>(*DwgIO::Find(drawing, shape->Style));
    std::println("SHAPE {} of {} at ({}, {}), size {}", shape->Number, file.Font, shape->Position.X,
                 shape->Position.Y, shape->Size);
  }
}
```

```text [Output]
SHAPE 133 of ltypeshp.shx at (70, 70), size 2
```

## Viewports {#viewports}

A `Viewport` is a window of a layout onto model space. [Model space and layouts](/tutorials/spaces/#viewports) reads where it lies on the sheet, what it shows and at what scale, and how R14 keeps it.

## Images and wipeouts {#images}

An `Image` and a `Wipeout` share a `Raster`. `Origin` is the outer corner of the bottom-left pixel; `UVector` is one pixel's step along the bottom edge and `VVector` one pixel's step up the left edge, all in world coordinates; `Size` counts the pixels. The picture covers `Size.X` times `UVector` by `Size.Y` times `VVector`. An image's `Definition` is an `Objects::ImageDef`, which names the file, its size in pixels and, in `Pixel`, one pixel's size in its `Units`; the drawing keeps the file's path, not its pixels.

A raster's clip boundary, `Corners`, is in pixels: `ClipShape` 1 gives two opposite corners, 2 a polygon. Its X runs along the bottom edge from the center of the first pixel, its Y down from the center of the top row, so the corner (x, y) lies at `Origin` plus (x + 0.5) `UVector` plus (`Size.Y` - 0.5 - y) `VVector`. A wipeout is an image of one pixel with no definition, always clipped by its polygon.

```cpp
const auto rasters = DwgIO::LoadFile("rasters-r2000.dwg");
if (!rasters)
{
  return 1;
}
const auto inWorld = [](const DwgIO::Entities::Raster &raster, DwgIO::Vector pixel) -> DwgIO::Vector
{
  const double x = pixel.X + 0.5;
  const double y = raster.Size.Y - 0.5 - pixel.Y;
  return DwgIO::Vector{.X = raster.Origin.X + (x * raster.UVector.X) + (y * raster.VVector.X),
                       .Y = raster.Origin.Y + (x * raster.UVector.Y) + (y * raster.VVector.Y),
                       .Z = raster.Origin.Z + (x * raster.UVector.Z) + (y * raster.VVector.Z)};
};
for (const DwgIO::Object &object : rasters->Drawing.Objects)
{
  if (const auto *image = DwgIO::GetIf<DwgIO::Entities::Image>(&object))
  {
    const DwgIO::Entities::Raster &raster = image->Raster;
    const auto &file = DwgIO::Get<DwgIO::Objects::ImageDef>(*DwgIO::Find(rasters->Drawing, raster.Definition));
    const double wide = raster.Size.X * std::hypot(raster.UVector.X, raster.UVector.Y);
    const double turned = std::atan2(raster.UVector.Y, raster.UVector.X) * 360 / DwgIO::Turn;
    std::println("IMAGE of {}, {} by {} pixels: at ({}, {}), {:.4f} wide, turned {:.0f} degrees", file.File,
                 file.Size.X, file.Size.Y, raster.Origin.X, raster.Origin.Y, wide, turned);
  }
  if (const auto *wipeout = DwgIO::GetIf<DwgIO::Entities::Wipeout>(&object))
  {
    std::string corners;
    for (const DwgIO::Entities::Pixel &corner : wipeout->Raster.Corners)
    {
      const DwgIO::Vector at = inWorld(wipeout->Raster, corner.Position);
      corners += std::format(" ({}, {})", at.X, at.Y);
    }
    std::println("WIPEOUT over{}", corners);
  }
}
```

```text [Output]
IMAGE of dot.png, 4 by 4 pixels: at (0, 0), 10.0000 wide, turned 0 degrees
IMAGE of dot.png, 4 by 4 pixels: at (20, 0), 5.0000 wide, turned 30 degrees
WIPEOUT over (40, 0) (50, 0) (50, 10) (40, 10)
WIPEOUT over (60, 0) (70, 0) (65, 10)
```

The second wipeout was drawn through (60, 0), (70, 0) and (65, 10): its stored corners are (-0.5, 0.5), (0.5, 0.5) and (0, -0.5), with Y running down. An `ImageDefReactor` links each image back to its definition, and `RasterVariables` and `WipeoutVariables`, in the named objects dictionary, say whether frames are drawn around images and wipeouts.

## Underlays {#underlays}

A `PdfUnderlay`, a `DwfUnderlay` and a `DgnUnderlay` show another file under the drawing. Each holds its `Data`, an `UnderlayData`: `Position`, `Scale` and `Rotation` in world coordinates, `Flags`, `Contrast` and `Fade`, and the `Clip` boundary in the underlay's own coordinates. `Definition` names a `PdfDefinition`, `DwfDefinition` or `DgnDefinition`, with the `File` and the page, sheet or model, `Name`. dwgio reads neither the PDF nor the others.

```cpp
const auto kinds = DwgIO::LoadFile("kinds-r2000.dwg");
if (!kinds)
{
  return 1;
}
const DwgIO::Drawing &more = kinds->Drawing;
for (const DwgIO::Object &object : more.Objects)
{
  if (const auto *pdf = DwgIO::GetIf<DwgIO::Entities::PdfUnderlay>(&object))
  {
    const auto &file = DwgIO::Get<DwgIO::Objects::PdfDefinition>(*DwgIO::Find(more, pdf->Data.Definition));
    std::println("PDFUNDERLAY of {}, page {}, at ({}, {})", file.File, file.Name, pdf->Data.Position.X,
                 pdf->Data.Position.Y);
  }
  if (const auto *dgn = DwgIO::GetIf<DwgIO::Entities::DgnUnderlay>(&object))
  {
    const auto &file = DwgIO::Get<DwgIO::Objects::DgnDefinition>(*DwgIO::Find(more, dgn->Data.Definition));
    std::println("DGNUNDERLAY of {}, model {}", file.File, file.Name);
  }
}
```

```text [Output]
PDFUNDERLAY of leve.pdf, page 1, at (300, 0)
DGNUNDERLAY of voirie.dgn, model Default
```

## OLE frames {#ole}

An `Ole2Frame` is an embedded OLE object, such as a spreadsheet: its `Version`, its `Mode` from R2000, and its `Data` as the file holds it. dwgio types it from the Open Design Specification alone: AutoCAD 2026 for Mac cannot insert one, so no sample drawing holds one and the reading is untested against AutoCAD's files. The description and the corners DXF writes are inside `Data`, which dwgio does not take apart.

## Tables {#tables}

An `AcadTable` is drawn through an anonymous block, `Block`, named `*T`, whose lines and MTEXTs show it; its cells are in the table itself. Up to R2007 it keeps them in `Widths`, `Heights` and `Cells`, row by row: a cell's text in `Cell::Text` up to R2004 and in `Cell::Value::Text` in R2007. From R2010 it keeps them in `Content`: `Columns` with their widths, `Rows` with their heights and cells, and each cell's `Items`, whose `Value::Text` is the text. `Content::Merged` lists the ranges of merged cells.

```cpp
const auto cellText = [](const DwgIO::Entities::AcadTable &table, std::size_t row, std::size_t column) -> std::string
{
  if (!table.Content.Rows.empty())
  {
    const DwgIO::Objects::TableContents::Cell &cell = table.Content.Rows.at(row).Cells.at(column);
    return cell.Items.empty() ? std::string() : cell.Items.front().Value.Text;
  }
  const DwgIO::Entities::AcadTables::Cell &cell = table.Cells.at((row * table.Widths.size()) + column);
  return cell.Text.empty() ? cell.Value.Text : cell.Text;
};
for (const char *file : {"cells-r2004.dwg", "cells-r2018.dwg"})
{
  const auto loaded = DwgIO::LoadFile(file);
  if (!loaded)
  {
    return 1;
  }
  for (const DwgIO::Object &object : loaded->Drawing.Objects)
  {
    if (const auto *table = DwgIO::GetIf<DwgIO::Entities::AcadTable>(&object))
    {
      const bool content = !table->Content.Rows.empty();
      const std::size_t rows = content ? table->Content.Rows.size() : table->Heights.size();
      const std::size_t columns = content ? table->Content.Columns.size() : table->Widths.size();
      std::println("{}: {} rows of {} columns", DwgIO::NameOf(loaded->Drawing.Release), rows, columns);
      for (std::size_t row = 0; row < 4; ++row)
      {
        std::println("  \"{}\" \"{}\" \"{}\"", cellText(*table, row, 0), cellText(*table, row, 1),
                     cellText(*table, row, 2));
      }
    }
  }
}
```

```text [Output]
R2004: 6 rows of 3 columns
  "Parts" "Name" "Size"
  "Count" "Bolt" "M8"
  "12" "Nut" "M8"
  "24" "" ""
R2018: 6 rows of 3 columns
  "Parts" "Name" "Size"
  "Count" "Bolt" "M8"
  "12" "Nut" "M8"
  "24" "" ""
```

The script that filled this table wrote ten texts into the first ten cells in order, so the title row holds three, of which only `Parts` shows: AutoCAD merges the title row's cells. A table with what dwgio does not read, such as a block cell, a field, a linked cell or a cell that overrides its style, loads as an `Unknown` and is saved back as it was.

## ACIS bodies and surfaces {#acis}

A `Region`, a `Solid3d`, a `Body` and the surfaces, `Surface`, `PlaneSurface`, `NurbSurface`, `ExtrudedSurface`, `LoftedSurface`, `RevolvedSurface` and `SweptSurface`, keep their shape as an ACIS body, in `Geometry`. dwgio gives the body as the file holds it and has no geometry kernel: it computes no volume, area, mesh or intersection, and draws nothing. What it gives:

| Member | Holds |
| :--- | :--- |
| `Geometry.Body` | The ACIS data: `Version` 1 for SAT text, decrypted, in R14 and R2000; 2 for SAB binary from R2004 to R2010; 0 for none |
| `Geometry.Wires`, `Silhouettes` | The wireframe AutoCAD draws the body with, as polylines, and its outline seen from a viewport |
| `Geometry.Stored`, `Guid` | From R2013, the body is in the data store, under that GUID |
| `Solid3d::History` | From R2007, the solid's history object, kept as an `Unknown` |
| `SurfaceOf::UIsolines`, `VIsolines`, `Data` | A surface's isolines, and what its kind adds: the profile and the sweep, revolution or loft that made it |

From R2013 AutoCAD keeps every body in the data store, a section of the file dwgio keeps as bytes, and `Body` is empty. A save to R2010 or below takes each body out of the store into its entity, in that release's ACIS. To read the bodies of a newer drawing, save it to R2000 in memory and load that: every body is then SAT text.

```cpp
const auto solids = DwgIO::LoadFile("solids-r2000.dwg");
if (!solids)
{
  return 1;
}
const auto firstLine = [](std::span<const std::byte> data) -> std::string
{
  std::string line;
  for (const std::byte one : data)
  {
    if (one == std::byte{'\n'})
    {
      break;
    }
    line.push_back(static_cast<char>(one));
  }
  return line.substr(0, line.find_last_not_of(' ') + 1);
};
for (const DwgIO::Object &object : solids->Drawing.Objects)
{
  if (const auto *solid = DwgIO::GetIf<DwgIO::Entities::Solid3d>(&object))
  {
    const DwgIO::Entities::Geometry &geometry = solid->Geometry;
    std::println("3DSOLID {}: SAT of {} bytes, \"{}\", {} wires", DwgIO::HexOf(solid->Base.Self),
                 geometry.Body.Data.size(), firstLine(geometry.Body.Data), geometry.Wires.size());
  }
}
const auto newer = DwgIO::LoadFile("solids-r2018.dwg");
if (!newer)
{
  return 1;
}
const DwgIO::Handle box = DwgIO::ParseHandle("2CE").value();
const auto &stored = DwgIO::Get<DwgIO::Entities::Solid3d>(*DwgIO::Find(newer->Drawing, box));
std::println("R2018: {} bytes in the entity, in the data store {}, {}", stored.Geometry.Body.Data.size(),
             stored.Geometry.Stored, stored.Geometry.Guid);
const auto older = DwgIO::Save(newer->Drawing, DwgIO::SaveOptions{.Release = DwgIO::Version::R2000});
const auto reread = older ? DwgIO::Load(older->Bytes) : std::unexpected(older.error());
if (!reread)
{
  return 1;
}
const auto &taken = DwgIO::Get<DwgIO::Entities::Solid3d>(*DwgIO::Find(reread->Drawing, box));
std::println("saved as R2000: version {}, \"{}\"", taken.Geometry.Body.Version, firstLine(taken.Geometry.Body.Data));
```

```text [Output]
3DSOLID 2CE: SAT of 7933 bytes, "400 188 1 0", 16 wires
3DSOLID 2E2: SAT of 1298 bytes, "400 27 1 0", 4 wires
3DSOLID 2E6: SAT of 455 bytes, "400 7 1 0", 8 wires
3DSOLID 2EA: SAT of 1530 bytes, "400 33 1 0", 4 wires
R2018: 0 bytes in the entity, in the data store true, {d81f8c98-accb-964b-8a8d-72c2540a7e47}
saved as R2000: version 1, "400 188 1 0"
```

The first line of SAT gives the ACIS release, 400 for R2000's 4.00, and how many records follow: the box with its four holes takes 188, the torus 7. The records name each face's surface: the cylinder 2EA has a `cone-surface`, which is how ACIS keeps a cylinder, and two `plane-surface`s. The R2018 body comes back from R2000's save as the same 188 records.

## Meshes, lights and section planes {#more}

A `Mesh` is a subdivision surface: its `Vertices`, and its `Faces` as one list in which each face is its vertex count followed by that many vertex indices. `Level` is how many times it is smoothed, and `Edges` and `Creases` the creased edges. A `Light` is a point (`Kind` 2), spot (3) or distant (1) light at `Position`, aimed at `Target`, with its `Intensity`, `Color` and, for a spot, its `Hotspot` and `Falloff` cones in radians. A `SectionObject` is a section plane: its line through `Vertices`, its `Vertical` direction, and `Settings`, a SECTION_SETTINGS object dwgio keeps as an `Unknown`.

```cpp
for (const DwgIO::Object &object : more.Objects)
{
  if (const auto *mesh = DwgIO::GetIf<DwgIO::Entities::Mesh>(&object))
  {
    std::map<int, int> sides;
    for (std::size_t at = 0; at < mesh->Faces.size(); at += mesh->Faces[at] + 1)
    {
      ++sides[mesh->Faces[at]];
    }
    std::string faces;
    for (const auto &[count, many] : sides)
    {
      faces += std::format(" {} of {} sides", many, count);
    }
    std::println("MESH: {} vertices,{}, smoothed {} times", mesh->Vertices.size(), faces, mesh->Level);
  }
  if (const auto *light = DwgIO::GetIf<DwgIO::Entities::Light>(&object))
  {
    static constexpr std::array<std::string_view, 3> kinds{"distant", "point", "spot"};
    std::println("LIGHT {}: {} at ({}, {}, {}), toward ({}, {}, {}), cone {:.0f} to {:.0f} degrees", light->Name,
                 kinds.at(light->Kind - 1), light->Position.X, light->Position.Y, light->Position.Z,
                 light->Target.X, light->Target.Y, light->Target.Z, light->Hotspot * 360 / DwgIO::Turn,
                 light->Falloff * 360 / DwgIO::Turn);
  }
  if (const auto *section = DwgIO::GetIf<DwgIO::Entities::SectionObject>(&object))
  {
    std::println("SECTIONOBJECT {}: from ({}, {}) to ({}, {}), settings {}", section->Name,
                 section->Vertices.front().X, section->Vertices.front().Y, section->Vertices.back().X,
                 section->Vertices.back().Y, DwgIO::TypeOf(*DwgIO::Find(more, section->Settings)));
  }
}
```

```text [Output]
MESH: 56 vertices, 54 of 4 sides, smoothed 0 times
SECTIONOBJECT Section Plane (1): from (240, 0) to (250, 0), settings SECTION_SETTINGS
LIGHT Pointlight1: point at (260, 0, 5), toward (260, 0, -5), cone 45 to 50 degrees
LIGHT Spotlight2: spot at (270, 0, 5), toward (270, 0, 0), cone 45 to 50 degrees
LIGHT Distantlight3: distant at (280, 0, 50), toward (280, 0, 0), cone 45 to 50 degrees
```

A light's cone is kept for every kind; only a spot light uses it.

## Entities dwgio does not type {#unknown}

An entity of any other kind, such as a proxy entity, an MPOLYGON or a Civil 3D object, loads as an `Unknown` with `Entity` set. `CommonOf` gives its layer, color and the other members of its `Common`, and `BaseOf` its handle, owner and extended data; its own data stays as bytes, saved back unchanged to its own release. [The object model](/guide/objects/#unknown) says what you can do with one. None of the sample drawings holds one; the loop below lists them in any drawing:

```cpp
for (const DwgIO::Object &object : more.Objects)
{
  if (const auto *unknown = DwgIO::GetIf<DwgIO::Unknown>(&object); unknown && unknown->Entity)
  {
    const DwgIO::Common &common = *DwgIO::CommonOf(object);
    std::println("{} {} on layer {}, {} bytes", DwgIO::TypeOf(object), DwgIO::HexOf(unknown->Base.Self),
                 DwgIO::Get<DwgIO::Tables::Layer>(*DwgIO::Find(more, common.Layer)).Record.Name,
                 unknown->Bytes.size());
  }
}
```

## Across releases {#releases}

- Images, wipeouts, underlays, tables, surfaces, meshes, lights and section planes are stored under their class, and AutoCAD writes them in every release, R14 included: the R14 save of `kinds-r2000.dwg` holds every kind it has, and dwgio reads them there as in later releases.
- An ACIS body is SAT in R14 and R2000, SAB from R2004 to R2010, and in the data store from R2013. A save to another release converts it.
- A table keeps its cells itself up to R2007 and in `Content` from R2010; a save converts between the two.
- A NURBS surface is a SURFACE up to R2010 and a NURBSURFACE from R2013.
- An image's `Outside`, which inverts its clip, is from R2010.
