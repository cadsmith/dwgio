# Fixtures

Every file under `acad` was written by AutoCAD 2026 for Mac, from a script of `tools/acad` or the saves
`resave.sh` makes: no file is edited by hand.

## Names

| File | What it is |
| :--- | :--- |
| `<script>-r14.dwg` to `<script>-r2018.dwg` | The script's drawing saved by AutoCAD to that release |
| `<script>-r2000.dxf` and other `-r<year>.dxf` | The DXF twin: the same drawing saved as DXF, which the tests compare the DWG with |
| `<stem>-as-r<year>.dwg` | AutoCAD's own save of the fixture `<stem>.dwg` to another release, from `tools/acad/resaves.txt` |
| `<stem>-bare-as-r<year>.dwg` | The same from the fixture without its tables' content, written by `dwgio_bare` |

## Scripts

Each script draws its subject, then saves it to every release from R2018 down to R14, unless the
table says otherwise.

| Script | Draws | DXF twins |
| :--- | :--- | :--- |
| `foundation.scr` | A line, a circle, an arc and a closed polyline | R2000 |
| `empty.scr` | Nothing: the console's new drawing, which `Create` is compared with | R2000 |
| `tables.scr` | Every table: five linetypes from acadiso.lin, two complex; three layers with colors, linetypes, a lineweight and the frozen, off, locked and no-plot states; two text styles; a named view; a UCS; a two-viewport configuration; an application; a dimension style with its `_Dot` arrow block; and a block of a line and a circle (`CHAIR`) inserted three times | R2000 |
| `entities.scr` | One of each simple entity: a line with extended data and a group reactor, a point, a thick circle, an arc, two ellipses, a ray, an xline, a 3D face with an invisible edge, a solid, a trace, a shape, three texts in three alignments, an mtext, a block of two attribute definitions inserted with a value, a block inserted as a grid, entities with their own layer, color, linetype and lineweight, a circle under a rotated UCS and an invisible line | R2000 |
| `curves.scr` | Four 2D polylines (widths and an arc, fitted, splined, thick and raised), two 3D polylines, two polygon meshes, a polyface mesh, six lightweight polylines, six splines by fit and by control points, five hatches, nine dimensions of the seven kinds, four leaders, a tolerance, two multilines, and a layout with three floating viewports, two clipped and one with a frozen layer and its own UCS | R2000 |
| `objects.scr` | The named objects: a dictionary holding an xrecord with a group of every kind, an empty dictionary and a nested one, a hard-owner dictionary, a named and an unnamed group, three scales added and one deleted, a draw order, a dimension association variable, and a renamed layout with two viewports | R2000 |
| `kinds.scr` | The kinds the others lack, after dxfio's script of the same name: a box and a sphere, a region, a helix, a mesh, a planar surface, extruded, revolved, lofted and swept surfaces procedural and as NURBS, a multileader, a section plane, point, spot and distant lights, PDF, DWF and DGN underlays with their definitions, and a body made by `entmake` from the box's ACIS data | R2000 to R2018 |
| `rasters.scr` | `dot.png`, a four-pixel image, attached twice, and two wipeouts | R2000 to R2018 |
| `table.scr` | A table of three columns and four rows | R2000 to R2018 |
| `cells.scr` | The same table with its first ten cells given a text through `entmod`, the merged title among them | R2000 to R2018 |
| `rowheights.scr` | A table of three columns and five rows, fifteen cells with text and the fourth row 20 high; saved as R2018 and R2004 only | none |
| `dimensions.scr` | An arc length and a jogged radius dimension, and a page setup made with `entmakex` | R2000 to R2018 |
| `mleaders.scr` | Two multileader styles, one with a block of a circle and an attribute definition, one with vertical text, a frame and other values; a multileader with two leaders using the block, one of the vertical style, and one with manual breaks in its leader line and dogleg | R2000 to R2018 |
| `attachments.scr` | Multileader styles with the text attached over, under, beside and around the leader line, and a multileader of each, one with its own line overrides | none |
| `dimbreaks.scr` | Two linear dimensions and a multileader broken by `DIMBREAK`, automatically and manually | none |
| `vertical.scr` | Multileaders whose text attaches above or below its leaders: centered, aligned left with a frame and aligned right, each with a leader from below and one from above, one with leaders from both, two rotated, and one holding a block; saved as R2018 and R2007 only | none |
| `leaders.scr` | Leaders of each color kind with dimension variable overrides and a block arrow, and multileaders whose leader lines carry their own color, linetype and arrow | none |
| `columns.scr` | MTEXTs with static and dynamic columns, one without columns, and texts made by `entmake` with exact spacing, a background fill and a defined height | R2013, R2018 |
| `histories.scr` | Solids with their history kept (`SOLIDHIST 1`): a box with a hole and a fillet, a wedge, a pyramid, a sphere, a cone, a torus and extrusions | none |
| `solids.scr` | A box less four cylinders, a cone, a torus and a cylinder: reversed edges, an apex and a torus, which `kinds` lacks | none |
| `material.scr` | A material with every channel set: ambient, diffuse and specular colors, a map on each of the six channels with its blend, projection, tiling and transform, gloss, opacity, refraction and the R2007 fields | R2000 to R2018 |
| `visualstyles.scr` | Two visual styles departing from the defaults in every kind of property, and the detail and section view styles given a value of their own in every field | R2000 to R2018 |
| `many.scr` | 1200 lines and 600 circles, so that the object map runs to several sections | none |
| `transparent.scr` | Lines with true colors and transparency, for `tests/dwgio/color.cpp`; saved as R2018 only | none |
| `variables.scr` | The new drawing as an R2018 DXF with `$REALWORLDSCALE` 0 and `$SHADOWPLANELOCATION` -5, which the console cannot set; `variables-saves.scr` then saves that DXF as R2018 and R2007. The DXF is not kept. | none |

No script makes an OLE frame, which AutoCAD for the Mac cannot insert. `tools/measure/large.scr` makes
the benchmark drawing of `dwgio_measure`, not a fixture.

`tests/dwgio/releases/samples.cpp` also reads the sample drawings inside the AutoCAD application, which
are not in this repository.

## Resaves

AutoCAD's own save of a fixture to another release shows what AutoCAD does when it converts: the
values an older release lacks, a table rebuilt from its content. `tools/acad/resaves.txt` lists each
fixture and the releases it is saved to:

| Fixture | Saved as |
| :--- | :--- |
| `kinds-r2000` | R2018, R2013, R2010, R2007, R2004 |
| `kinds-r2004` | R2018, R2013, R2010, R2007 |
| `kinds-r2007` | R2018, R2013, R2010 |
| `cells-r2004` | R2010, R2000 |
| `cells-r2007`, `cells-r2013` | R2018 |
| `cells-r2018` | R2007, R2004 |
| `cells-r2004-bare`, `cells-r2007-bare` | R2018 |
| `table-r2000` | R2007 |
| `table-r2004` | R2018 |
| `table-r2018` | R2000 |
| `rowheights-r2004` | R2018 |
| `rowheights-r2018` | R2004 |
| `histories-r2004` | R2018, R2007 |
| `leaders-r14` | R2000 |
| `leaders-r2007` | R2010 |
| `columns-r14` | R2018 |
| `attachments-r2007` | R2010 |
| `vertical-r2007` | R2018 |

A `-bare` input is the fixture with each table's extension dictionary and everything under it taken
away, so that AutoCAD's save shows the content it makes from a table's grid alone. `dwgio_bare`, built
from `tools/fixtures/bare.cpp`, writes it; `resave.sh` runs it first.

## Making them again

On a Mac with AutoCAD 2026 (`DWGIO_ACAD` names another installation):

```sh
sh tools/acad/make.sh foundation
cmake --build --preset debug --target dwgio_bare
sh tools/acad/resave.sh
sh tools/acad/resave.sh kinds-r2000 2018 2013
```

`make.sh` runs a script in AutoCAD's console in `$TMPDIR/dwgio-acad` and moves what it saved into
`tests/fixtures/acad`. `resave.sh` with no argument makes every resave of `resaves.txt`, or one fixture
to the releases given. Both run one console at a time under `tools/acad/locked.py`.
