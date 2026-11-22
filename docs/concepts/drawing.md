# The drawing

A `Drawing` follows the file: what dwgio types is typed, anything else is kept as bytes, and the order of the file's blocks is kept so a save can write everything back in place.

## Blocks {#blocks}

An R14 or R2000 file is a header followed by blocks: the auxiliary header, the preview images, the header variables, the classes, a four-byte pad, the objects, the object map, the second header and the measurement. From R2004 the file is paged and the blocks are named sections, such as `AcDb:SummaryInfo` or `AcDb:RevHistory`. `Drawing::Order` lists the blocks by name in file order; bytes no block claims are kept under the name `unknown` and noted as an issue. Where the order puts the map before the objects, or the second header before the blocks it points to, a save moves them after and notes the move.

| Block | Where it lives |
| :--- | :--- |
| `header` | `Drawing::Header`, one member per variable of the Open Design Specification for .dwg files |
| `classes` | `Drawing::Classes` |
| `objects` | `Drawing::Objects`, each an `Object` |
| `preview` | `Drawing::Preview`, one `PreviewImage` per record |
| `map` | Computed on save from the objects |
| `second` | `Drawing::Kept`; its addresses, records and handles are computed on save from the header |
| `AcDb:SummaryInfo` | `Drawing::SummaryInfo`; in R14 and R2000, the `DWGPROPS` record among the objects |
| `AcDb:FileDepList` | `Drawing::FileDependencies` |
| `AcDb:AppInfo` | Not kept: every save writes a new one |
| `auxiliary`, `pad`, `measurement`, `unknown`, and other sections | `Drawing::Kept`, bytes as read |

`Release`, `Codepage` and `Maintenance` come from the file header: the file's release, the code page of its text before R2007, and the maintenance version AutoCAD stamped it with. A save to the drawing's own release stamps its `Maintenance` again; a save to another release, and a drawing from `Create`, take the one AutoCAD 2026 stamps that release with, which `MaintenanceOf(release)` gives: 10 in R14, 15 in R2000, 104, 50, 226 and 125 from R2004 to R2013, 0 in R2018.

## Objects {#objects}

`Object` is a variant of the kinds dwgio types, with `Unknown` first for every other kind. Each kind is held in a `Boxed` of its own, so an object takes the size of its kind rather than of the largest. `Holds`, `Get`, `GetIf` and `Visit` work like `std::holds_alternative`, `std::get`, `std::get_if` and `std::visit`, and give the kind rather than its box.

Every other object is an `Unknown`, which keeps its bytes; [Unknown objects](/guide/objects/#unknown) says what it holds and how a save carries it.

`TypeOf` names any object as DXF does. `BaseOf` gives the handles every object has. `CommonOf` gives an entity's common data, typed or not, and null for anything else. `Find` gives the object a handle names, or null. [The object model](/guide/objects/) shows them at work.

Every record opens with a `Tables::Record`: its `Base`, its name and its xref data. Every entity opens with its `Base` and its `Common`, and every named object opens with its `Base` alone; each is followed by its own members, each commented with its DXF group. Angles are in radians, and a point is in world or object coordinates as the file keeps it. A typed kind is written from its fields.

`Base` is what every object carries: `Self`, `Owner`, `Dictionary`, `Reactors` and `Extended`. A kind the file stores under a class rather than a fixed type, such as a layout, a scale or a variable, is matched on read by its class's DXF name and numbered on write from the drawing's classes; when the drawing declares no such class, a save leaves the object out with an issue.

`Common` is what every entity carries besides:

- the space it is drawn in, its color, linetype scale, visibility, lineweight and layer;
- which linetype, plot style and material it draws with (the layer's, the block's, Continuous, or a record of its own), set by flags and named by handles;
- the links to the entities before and after it, kept by R2000 and older files and filled in where the file leaves them implicit;
- its proxy graphics.

## Kinds {#kinds}

Every kind dwgio types, with the type the file stores it under and what it holds:

| Kind | File type | What it holds |
| :--- | :--- | :--- |
| `Tables::Control` | the ten `*_CONTROL` objects | the records of one table, which `Control::Table` names |
| `Tables::AppId` | `APPID` | an application registered for extended data |
| `Tables::Layer` | `LAYER` | the states, color, lineweight, plot style and linetype of a layer |
| `Tables::Style` | `STYLE` | a text style or a shape file |
| `Tables::LType` | `LTYPE` | a pattern of dashes, each with the shape or text it carries |
| `Tables::View` | `VIEW` | a named view and its UCS |
| `Tables::Ucs` | `UCS` | a named coordinate system |
| `Tables::VPort` | `VPORT` | a tiled viewport of model space |
| `Tables::DimStyle` | `DIMSTYLE` | one member per DIM variable |
| `Tables::BlockRecord` | `BLOCK_HEADER` | a block's entities, inserts and layout |
| `Tables::ViewportEntityHeader` | `VP_ENT_HDR` | the record a paper space viewport is listed under, before R2004 |
| `Entities::Line` | `LINE` | two points |
| `Entities::Point` | `POINT` | a point and the angle of its marker |
| `Entities::Circle`, `Entities::Arc` | `CIRCLE`, `ARC` | a center and a radius, and for the arc two angles |
| `Entities::Ellipse` | `ELLIPSE` | a center, a semi-major axis, a ratio and two parameter angles |
| `Entities::Ray`, `Entities::XLine` | `RAY`, `XLINE` | a point and a direction |
| `Entities::Face3d` | `3DFACE` | four corners and which edges are invisible |
| `Entities::Solid`, `Entities::Trace` | `SOLID`, `TRACE` | four corners at one Z |
| `Entities::Shape` | `SHAPE` | a shape of a shape file, by number, and its style |
| `Entities::Text`, `Entities::AttDef`, `Entities::Attrib` | `TEXT`, `ATTDEF`, `ATTRIB` | a `Lettering`; for the attributes a tag and flags, for the definition a prompt |
| `Entities::MText` | `MTEXT` | paragraphs in a rectangle |
| `Entities::Block`, `Entities::EndBlk`, `Entities::SeqEnd` | `BLOCK`, `ENDBLK`, `SEQEND` | what opens and closes a block's entities or an insert's attributes |
| `Entities::Insert` | `INSERT`, `MINSERT` | a block placed once or as a grid, with its attributes |
| `Entities::Polyline` | `POLYLINE_2D`, `POLYLINE_3D`, `POLYLINE_PFACE`, `POLYLINE_MESH` | a polyline or mesh whose vertices follow it, its form set by its flags |
| `Entities::Vertex` | `VERTEX_2D`, `VERTEX_3D`, `VERTEX_MESH`, `VERTEX_PFACE`, `VERTEX_PFACE_FACE` | a point of a polyline, or a face of a polyface mesh |
| `Entities::LwPolyline` | `LWPOLYLINE` | a 2D polyline holding its own `Vertices` |
| `Entities::Spline` | `SPLINE` | a NURBS curve by fit points or by knots and control points |
| `Entities::Hatch` | `HATCH` | boundary `Path`s of edges or vertices, a pattern of `PatternLine`s, and seeds |
| `Entities::Dimension` | the seven `DIMENSION_*` types | a `Measure` and a `Forms::Form`, which picks the type |
| `Entities::ArcDimension`, `Entities::LargeRadialDimension` | `ARC_DIMENSION`, `LARGE_RADIAL_DIMENSION` | an arc length or a jogged radius dimension |
| `Entities::Leader` | `LEADER` | a line to an annotation, with its arrowhead and hook line |
| `Entities::MultiLeader` | `MULTILEADER` | an `MLeader`: leader lines and a text or block content |
| `Entities::Tolerance` | `TOLERANCE` | a feature control frame |
| `Entities::MLine` | `MLINE` | parallel lines, each `Joint` carrying a `Strand` per line of the style |
| `Entities::Viewport` | `VIEWPORT` | a window on a layout and the view of model space it shows |
| `Entities::Image`, `Entities::Wipeout` | `IMAGE`, `WIPEOUT` | a raster image, or a polygon that masks what lies under it |
| `Entities::PdfUnderlay`, `Entities::DwfUnderlay`, `Entities::DgnUnderlay` | `PDFUNDERLAY`, `DWFUNDERLAY`, `DGNUNDERLAY` | a PDF, DWF or DGN file shown under the drawing |
| `Entities::Ole2Frame` | `OLE2FRAME` | an OLE object, its data kept as bytes |
| `Entities::AcadTable` | `ACAD_TABLE` | a table of cells |
| `Entities::Helix`, `Entities::Mesh`, `Entities::Light`, `Entities::SectionObject` | `HELIX`, `MESH`, `LIGHT`, `SECTIONOBJECT` | a helix, a subdivision mesh, a light, a section plane |
| `Entities::Region`, `Entities::Solid3d`, `Entities::Body` | `REGION`, `3DSOLID`, `BODY` | an ACIS body, its wires and silhouettes |
| `Entities::Surface` and the six other surface kinds | `SURFACE`, `PLANESURFACE`, `NURBSURFACE`, `EXTRUDEDSURFACE`, `LOFTEDSURFACE`, `REVOLVEDSURFACE`, `SWEPTSURFACE` | an ACIS body and the surface's own data |
| `Objects::Dictionary` | `DICTIONARY`, `ACDBDICTIONARYWDFLT` | objects found by name, with a default when it has one |
| `Objects::XRecord` | `XRECORD` | an application's DXF groups |
| `Objects::Layout` | `LAYOUT` | a tab of the drawing with its `Plot` settings, limits, extents and UCS |
| `Objects::PlotSettings` | `PLOTSETTINGS` | a named page setup |
| `Objects::Group` | `GROUP` | entities selected together |
| `Objects::MLineStyle` | `MLINESTYLE` | the lines of a multiline and its caps |
| `Objects::MLeaderStyle` | `MLEADERSTYLE` | a multileader style |
| `Objects::TableStyle`, `Objects::CellStyleMap` | `TABLESTYLE`, `CELLSTYLEMAP` | a table style and, below R2010, its cell styles |
| `Objects::TableContent`, `Objects::TableGeometry`, `Objects::DataTable` | `TABLECONTENT`, `TABLEGEOMETRY`, `DATATABLE` | a table's content, the extents of its cells, and its values below R2007 |
| `Objects::Scale` | `SCALE` | one entry of the scale list |
| `Objects::SortEntsTable` | `SORTENTSTABLE` | the draw order of a block's entities |
| `Objects::Placeholder` | `ACDBPLACEHOLDER` | the object a plot style name stands for |
| `Objects::DictionaryVar` | `DICTIONARYVAR` | a drawing variable kept as text, such as DIMASSOC |
| `Objects::ImageDef`, `Objects::ImageDefReactor`, `Objects::RasterVariables`, `Objects::WipeoutVariables` | `IMAGEDEF`, `IMAGEDEF_REACTOR`, `RASTERVARIABLES`, `WIPEOUTVARIABLES` | an image file, the link from an image to it, and the drawing's raster and wipeout settings |
| `Objects::PdfDefinition`, `Objects::DwfDefinition`, `Objects::DgnDefinition` | `PDFDEFINITION`, `DWFDEFINITION`, `DGNDEFINITION` | the file an underlay shows, and the part of it |
| `Objects::Material`, `Objects::VisualStyle` | `MATERIAL`, `VISUALSTYLE` | a material and a visual style |

## The view {#view}

`Drawing::Objects` is in file order, and handles tie the objects together; `View` reads them by structure. Its functions take the drawing or an `Index` of it and return pointers into `Objects`, or null. Given the drawing, each indexes it again; build an `Index` once to walk a large drawing. None copies the drawing, and `Records` and `Record` on a drawing that is not const give pointers to change it through. [Handles](/guide/objects/#handles) says how long a pointer stays valid. Names compare as AutoCAD compares them, ASCII letters in any case, through `View::Matches`.

| Function | Answers |
| :--- | :--- |
| `View::Control(drawing, table)` | a table's control object |
| `View::Records<RecordT>(drawing)` | a table's records in its control's order, followed by those the control keeps apart: the two spaces, ByBlock and ByLayer |
| `View::Record<RecordT>(drawing, name)` | the record of a name, among those the control keeps apart first, so `*Paper_Space` is the header's paper space |
| `View::Block(drawing, name)` | a block record by name |
| `View::ModelSpace(drawing)`, `View::PaperSpace(drawing)` | the block records of the spaces the header names |
| `View::Entities(drawing, record)` | a block's entities in drawing order; for a broken chain, what the block owns, in file order |
| `View::Attributes(drawing, insert)`, `View::Vertices(drawing, polyline)` | an insert's attributes or a polyline's vertices, listed or chained |
| `View::Layouts(drawing)`, `View::Layout(drawing, name)` | the layouts in tab order, or one by name |
| `View::Root(drawing)`, `View::Dictionary(drawing, name)`, `View::Entry(drawing, dictionary, name)` | the named objects dictionary, one of its dictionaries, an entry of any dictionary |

## Create and Edit {#edit}

`Create` returns the drawing AutoCAD makes from nothing, as far as dwgio types it:

- the header at its `DefaultsOf`;
- the ten control objects;
- the layer `0`, the three linetypes, the style Standard, the application ACAD, the dimension styles Standard and ISO-25 and the viewport `*Active`;
- the two spaces with their BLOCK and ENDBLK, and the layouts Model and Layout1;
- the named objects dictionary with its groups, layouts, multiline styles, plot settings and plot style names, and the classes those are stored under.

AutoCAD's audit finds nothing in it once saved.

`Edit` adds to a drawing and keeps every link consistent:

- `Edit::Add` of an entity appends it to a space, or to a block by name. It gives the entity a handle when it has none, its owner, its space code, the current layer when it names none, the lineweight an R2000 save writes, and a place at the end of the block's chain. An ATTDEF marks its block as holding attribute definitions, and from R2000 an INSERT is listed in its block record.
- `Edit::AddAttribute` appends an ATTRIB to an insert, in the insert's chain or list, makes the SEQEND that closes them when there is none and sets the insert's attribute flag.
- `Edit::AddBlock` makes a block record with its BLOCK and ENDBLK and lists it in the block table.
- `Edit::Add` of a table record lists it in its control, under a name no record of that table has, ignoring case.
- `Edit::MakeHandle` returns the handle seed, moved above every handle the drawing holds.
- `Edit::Editor` makes the same edits for a run of them: it indexes the drawing once and keeps the index current, so each addition takes a time that does not grow with the drawing, where each function above walks it. It is the way to build a large drawing.

An object that is not an entity, a block the drawing lacks, a name already taken, or a table the drawing does not have is refused with the reason, and the drawing is left unchanged.

`Edit::Remove` takes an object out with everything it owns, as AutoCAD's ERASE and PURGE do: an entity with its attributes or vertices and its extension dictionary, a block with its entities, a table record, a dictionary entry. Chains, lists, dictionaries, groups, draw orders and reactors close over what is removed; a dimension association bound to it is removed too, and a hatch bound to it loses its boundary. It refuses what a drawing needs, such as layer `0` or a space, and anything still in use, such as a layer an entity is on, naming the reason. [Editing a drawing](/tutorials/editing/) changes, copies and removes objects with them.

## Audit {#audit}

`Audit::Check` lists what AutoCAD would complain about in a drawing as a `Report` of `Finding`s, and leaves the drawing unchanged. `Audit::Repair` runs the same checks and fixes what they find, in place. A finding says briefly what is wrong, names the object at fault or the one that needed what is missing, and has a level:

- `Error`: AutoCAD refuses the drawing or recovers it, or an object is silently lost.
- `Warning`: AutoCAD opens the drawing without changing it.

`Report::IsClean` is true when no error is left unfixed. The checks run in order, each assuming the earlier ones hold, and are graded by how AutoCAD 2026 treats a drawing broken that way.

| Check | Finds | Repair |
| :--- | :--- | :--- |
| Handles | an object with no handle or with another's; the seed not past the highest handle | a new handle past every handle in use; the seed moved |
| Tables | a table without its control, or one the header does not name; a required record missing: layer `0`, the style Standard, the linetypes ByBlock, ByLayer and Continuous, the application ACAD, the two spaces; a header root naming no record of its kind | the control, the record, or a space with its BLOCK, ENDBLK and layout, made as `Create` makes them; the root set |
| Dictionaries | the root dictionary missing; one of the five AutoCAD keeps in it missing or not the header's; an entry or a default naming nothing | made and entered; the header set; the entry dropped |
| Listings | a control entry naming no record of its table, or listed twice; a record not listed, or owned by another; the spaces or the ByBlock and ByLayer linetypes of a control not the header's | the entry dropped; the record listed once and owned; the control set |
| Names | a record whose name another record of its table has, ignoring case; a record with no name | the later or the nameless record left out, and what named it moved to the first or to the table's default |
| References | a reference to a missing record or object, or one of the wrong kind: an entity's layer, linetype or plot style; a text's, dimension's or multiline's style; a layer's linetype or plot style; a dimension style's text style; a multileader's style, linetypes, text styles, block and arrowheads, and those of its style; an image's definition and reactor, an underlay's definition; a table's block and style, a table style's text styles; an entity's material and visual styles, a layer's material, the visual style of a view, a viewport record, a viewport entity or a layout; a solid's history; an insert or a layout of a missing block; a block record without its BLOCK, ENDBLK or, for a space, its layout | the fallback: layer `0`; ByLayer for an entity's linetype, Continuous for a layer's; no plot style or arrowhead; the Standard text, dimension, multileader or table style; the STANDARD multiline style; no block for a multileader style; a material, visual style or history cleared; an insert, layout, multileader or table of a missing block, or an image or underlay without its definition, left out; an image's stray reactor cleared; the framing made |
| Chains | an entity owned by no block record; one not in the space of its block; a block chain that does not walk what the block owns; a vertex, attribute or SEQEND owned by no sequence; a polyline or insert whose sequence is broken, lacks its SEQEND or names another; an insert whose attribute flag does not match its attributes | moved to model space; the space set; the chain or sequence rebuilt in file order; the orphan left out; the SEQEND made; the flag set |
| Classes and values | an object of a kind stored under a class the drawing does not declare; extended data of a missing application; a lineweight outside the codes a file keeps; a color out of range | the class declared as AutoCAD declares it, or an unfixed finding when no table knows it; the data dropped; ByLayer; ByLayer, or white on a layer |

Every drawing AutoCAD wrote checks clean. What AutoCAD's own files hold is not a finding: null entries in a view table, nameless viewport entity headers, repeated names among anonymous blocks and tiled viewports, and a SEQEND with its own plot style and no handle. `Repair` erases what it leaves out from `Objects`, which [invalidates](/guide/objects/#handles) pointers the view gave before it.

## Handles {#handles}

A `Handle` is the number an object is known by in its drawing, and that other objects use to point to it. Zero names no object. `HexOf` writes one as DXF does, and `ParseHandle` reads one back. The second header repeats the roots (the handle seed, the tables' control objects, the named objects dictionary and a few more); a save takes them from the header.

## Issues and failures {#issues}

An `Issue` is something the reader or writer noted and recovered from, with its byte offset and the handle of the object it concerns. A `Failure` stops a load or a save. [Errors and issues](/guide/errors/) lists what makes each fail and shows how to report them.
