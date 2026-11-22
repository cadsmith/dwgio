# Compatibility

`Save` writes a drawing of any release as any other from R14 to R2018. What both releases store, it writes in the target's form; what the target has no place for, it drops. The sections below list, for each step between releases, what a save converts, what it drops with an issue, and what it drops silently.

## How a save crosses releases {#crossing}

A save to another release crosses every step between the two: an R2018 drawing saved as R2000 passes through the steps below R2018, below R2013, below R2010, below R2007 and below R2004, in that order.

A step does one of three things to each value:

- Converted: the target holds the same thing another way, and nothing is lost. The drawing loads back from the saved file with the same values.
- Dropped and noted: the drawing held a value the target cannot, such as a true color in R2000. `Saved::Issues` gets one `Issue` for it, with the object's handle in `Subject`. Its text names the release that lacks the value, which is the step's, not always the target: a save to R14 notes `transparency of LINE 3FA not kept by R2000`.
- Dropped without a note: what AutoCAD's own save to the target drops silently, such as the header variables of a later release, or an entity's material and visual styles.

The comments in the headers under `include/dwgio` name the release that added each field. Whatever the step, AutoCAD opens the saved file and its audit finds nothing, as [Quality](/concepts/quality/#verified) says.

[Saving a drawing](/tutorials/saving/#older) saves a drawing to R14 and counts its issues.

## Saving to a newer release {#newer}

A save to a newer release keeps what the typed objects hold, but for the few values listed after the table, and the untyped objects [left out at any step](#unknown). What the newer release adds takes AutoCAD's defaults, the header's from `DefaultsOf`, and what it stores another way is converted:

| Step | Converted |
| :--- | :--- |
| R14 to R2000 | A LEADER's arrowhead kind into the R2000 field AutoCAD moves it to. An MTEXT's line spacing from `ACAD`'s `MTEXTBEGIN` extended data. An entity's lineweight from `ACAD` extended data into its code, and its ByLayer, ByBlock or Continuous linetype from the record's handle into a flag. A paper space viewport's view from `ACAD`'s `MVIEW` extended data into its fields. The dimension arrow blocks from names to block records. `DIMUNIT` into `DIMLUNIT` and `DIMFRAC`, `DIMFIT` into `DIMATFIT` and `DIMTMOVE`, in the header and each dimension style. The names R14 cut or put in upper case, from the records of the original names R14 keeps. |
| R2000 to R2004 | An MTEXT's background from `ACAD`'s `ACAD_MTEXT_BBRT` extended data, which is removed with its `MTEXTBEGIN`. Each break point of a BREAKDATA without the null handle R2000 holds for it, with the bit AutoCAD sets. A block record's entities, an insert's attributes and a polyline's vertices from chains into lists. ACIS bodies from SAT into SAB. The summary from the `DWGPROPS` record into its section, which the record leaves. The sections AutoCAD requires of a paged file are made, with an application information naming dwgio. |
| R2004 to R2007 | An MTEXT's defined height (`Defined`) from `ACAD`'s `ACAD_MTEXT_DEFINED_HEIGHT` extended data, which stays, as AutoCAD leaves it. Text from the code page into UTF-16, a linetype's dash texts included. A table's values into R2007's form. The texts of the untyped dynamic block objects, DIMASSOC and, up to R2010, the associative network into their string stream. A 3DSOLID's history from the `ACAD_SOLID_HIST` groups of its `ACAD_XREC_ROUNDTRIP` record, which are removed; the record and its dictionary are removed with them when they hold nothing else. The saved-by-version extended data of the history nodes of extrusions and sweeps taken away. |
| R2007 to R2010 | A multileader's and its style's version from `ACAD_MLEADERVER` extended data into a field, and from its `ACAD_XREC_ROUNDTRIP` record what its leader lines hold of their own (the `Leaderline` groups) and its text's direction and attachments above, below and beside it (the `ROUNDTRIP_MLCONTEXT` and `ACAD_ROUNDTRIP_2010_MLEADER` groups), each leader's dogleg the length the record gives, 0 as AutoCAD writes it. A vertically attached text's content base point moved back to the middle of the text, by the landing gap and half the text's width along the text and, when every leader comes from above, 1.2 text heights down it; the width is inferred from where the text's location lies: the whole text width for right alignment, half for centered, and 0 for left alignment, whose location is its left edge, where AutoCAD measures it with the font. A table style's flags into the long R2010 keeps them in. A table from its grid into its content, a table style from its three cells into cell styles; the table's DATATABLE and the style's cell map and record taken away. A visual style's flags from its round-trip record. |
| R2010 to R2013 | Each break point of a BREAKDATA with the second bit AutoCAD writes. The ACIS bodies into a data store made for them, and an ACIS entity's GUID from `ACAD_OBJECT_CHANGE_GUID` extended data. A SURFACE into a NURBSURFACE. A spline's or helix's flags and knot parameter set as AutoCAD sets them. A visual style's properties beyond R2010's from its round-trip record, which is removed. |
| R2013 to R2018 | An MTEXT's columns from `ACAD`'s `ACAD_MTEXT_COLUMN_INFO` extended data, which is removed, its width that of all its columns; the MTEXTs AutoCAD split further columns into joined to the first and taken away. A table style saved from below R2010 gets the flag 8 AutoCAD sets. A detail or section view style's display name and flags from its round-trip record. The data store's bodies kept unchanged. |

A few values exist only in the older release and are dropped by a save to a newer one, as AutoCAD's own save drops them:

- A LEADER's `Gap`, `Dimasz`, `Color` and the unnamed fields of R14 above R14, and its annotation's `Height` and `Width` above R2007. AutoCAD derives them again from the dimension style and the annotation at each save to those releases, as dwgio does when saving to an older release. No note.
- A multileader's arrowheads list (`Arrowheads`) above R2007, noted. AutoCAD drops it at every save, its own release's included; it keeps a leader line's own arrowhead in the line, below R2010 in the round-trip record.
- A table style's suppressed title and header rows above R2007, noted.
- A vertically attached multileader's doglegs above R2007: AutoCAD's save below R2010 gives them no length and keeps 0 in the record, and its save to a newer release takes the 0. The overrides the save below R2010 marks (the attachments beside the text and the dogleg's length) stay marked. No note.
- A dimension arrow name that names no block, noted as `DIMBLK DOT of R14 not written as R2000`.

## Below R2018 {#r2013}

Saving an R2018 drawing as R2013 or older.

| What | How |
| :--- | :--- |
| Header | R2018's variables dropped, no note. The auxiliary header made in R2013's form. |
| MTEXT | Its columns written into `ACAD` extended data, `ACAD_MTEXT_COLUMN_INFO_BEGIN` to `_END`, its width a column's. AutoCAD's save also writes the text of each column the text fills past the first in an MTEXT of its own; dwgio, which does not lay text out, keeps the text whole in the first and notes it: `3 columns of MTEXT 2CE written as one MTEXT, where AutoCAD's save writes one per column`. AutoCAD reads such a text back into the same columns. `Annotative`, `Version`, `Defaulted` and `Application` dropped, no note. |
| Table style | The flag 8 AutoCAD's save to R2018 sets cleared. |
| ATTDEF, ATTRIB | `Type` dropped, no note. |
| Multiline style | Each element's linetype from its record to its index among the linetype table's entries; a record the table lacks becomes ByLayer, noted. |
| Detail and section view styles | Their display name and flags moved into an XRECORD under `ACAD_XREC_ROUNDTRIP` in their extension dictionary, made where they have none. |
| Data store | Its ACIS bodies rewritten as R2013 writes them; one that cannot be is kept unchanged, noted. |
| Light, visual style | `ACAD`'s `AcDbSavedByObjectVersion` extended data added, as AutoCAD's files below R2018 have it. |

## Below R2013 {#r2010}

| What | How |
| :--- | :--- |
| Header | `REQUIREDVERSIONS` and R2013's dictionary dropped, no note. |
| Data store | Each ACIS body moved into its entity, as SAB; the entity's data store bit cleared. Anything else in the store, such as a layout's data, left out with one issue: `data store left out, the data of 2 objects in it lost`. |
| ACIS entities | A NURBSURFACE written as a SURFACE, its second stream dropped. Its GUID kept in `ACAD_OBJECT_CHANGE_GUID` extended data, the application registered where the drawing lacks it. |
| Spline, helix | `Options` and `Parameter` dropped, no note. |
| Layer | `Unknown1` dropped, no note. |
| Visual style | The properties beyond R2010's 28 moved into an XRECORD under `ACAD_XREC_ROUNDTRIP` in its extension dictionary. |
| Multileader, its style, table | R2013's fields (`MLeader::ExtendToText`, the style's `Unknown1`, the table's `Unknown3`) dropped, no note. |
| BREAKDATA | Converted: each break point rid of the second bit R2013 adds. |

## Below R2010 {#r2007}

| What | How |
| :--- | :--- |
| Header, dimension styles | `DIMTXTDIRECTION`, `DIMALTMZF`, `DIMALTMZS`, `DIMMZF` and `DIMMZS` dropped, no note. |
| Every entity | Its visual styles (`FullStyle`, `FaceStyle`, `EdgeStyle`) dropped, no note. |
| Multileader and its style | The version moved into `ACAD_MLEADERVER` extended data, the application registered where the drawing lacks it. What the multileader holds of R2010 moved into an XRECORD under `ACAD_XREC_ROUNDTRIP` in its extension dictionary, made where it has none, as AutoCAD's save writes it: for a text attached vertically, its direction and its context's attachments in `ROUNDTRIP_MLCONTEXT` groups with each leader's index; what each leader line holds of its own (type, color, linetype, lineweight, arrow size, arrowhead) in a `Leaderline` group; the multileader's own direction, attachments and dogleg length in `ACAD_ROUNDTRIP_2010_MLEADER` groups, and for any text its attachments above and below that are not at its center. A vertically attached text's attachments beside it are derived as AutoCAD's save derives them (left 4, or 5 under an overlined bottom; right 0, or 6 when the top is overlined and every leader comes from above), its doglegs given no length and those values marked overridden. The style's text direction and attachments above and below dropped, no note. A vertically attached text's content base point moved as AutoCAD's save moves it, to the text's left edge less the landing gap and, when every leader comes from above, 1.2 text heights up the text; the half width is inferred from where the location of a text aligned left or right lies, and 0 for a centered text, whose location is its middle, where AutoCAD measures it with the font. |
| Table, table style | The table written as a grid made from its content; the style's cell styles as its three cells, with a cell style map made under the handle seed, its flags in group 71. |
| Image, wipeout | An inverted clip (`Raster::Outside`) dropped, noted. |
| LEADER | Its annotation's `Height` and `Width`, which AutoCAD derives at each save: an MTEXT annotation's extents, else 0. |
| LWPOLYLINE, VERTEX | The segment and vertex `Id`s dropped, no note. |
| Hatch | A spline edge's fit points and end tangents dropped, no note. |
| ATTDEF, ATTRIB, dimension, visual style | The `Revision` and `Version` fields dropped, no note. |
| Visual style | Its properties' flags moved into its round-trip record, with the older layout's forms: a lighting quality of 3 as 2, a line width as a whole number. |
| Untyped object context data | Converted: the version they hold. |

## Below R2007 {#r2004}

| What | How |
| :--- | :--- |
| Text | From UTF-16 into the drawing's code page. A character the code page lacks is written as AutoCAD writes it, `\U+` and four hex digits, or `?` beyond the basic plane. |
| Linetype | Its dash texts laid out in the 256 bytes R2004 has for them; a text that does not fit is left out, noted. |
| Header | R2007's variables (camera, lofting, sun and location, the current material, the interference and drag visual styles, `DIMLTYPE`, `DIMFXL`, `DIMJOGANG`, `DIMTFILL`, `DIMARCSYM` and others) dropped, no note. |
| Dimension styles | The same R2007 variables dropped, no note. |
| Every entity | Its material and shadow dropped, no note. |
| Layer, layout | The material and the visual style dropped, no note. |
| Page setup | Its visual style dropped, noted. |
| View, viewport record, viewport | Lights, brightness, contrast, background, visual style and sun dropped, no note. |
| ATTDEF, ATTRIB | `Locked` dropped, no note. |
| MTEXT | `Defined` written into `ACAD`'s `ACAD_MTEXT_DEFINED_HEIGHT` extended data. |
| Dimension | `FlipFirst` and `FlipSecond` dropped, no note. |
| Material | R2007's fields dropped, no note. |
| 3DSOLID | Its history kept in `ACAD_SOLID_HIST` groups of an XRECORD under `ACAD_XREC_ROUNDTRIP` in its extension dictionary, made where it has none, as AutoCAD's save writes it: the history, the solid's R2007 long and its own handle. |
| Extruded and swept surfaces, the history nodes of extrusions and sweeps | `ACAD`'s saved-by-version extended data added, as AutoCAD writes it. |
| Table | A DATATABLE made from its values, and a record of its style's cell types; each cell's value written as text. |
| Untyped dynamic block objects, DIMASSOC, the associative network | Converted: their texts move from the string stream into their data, and the release the dynamic block objects say they were saved in. The network only from below R2013. |

## Below R2004 {#r2000}

| What | How |
| :--- | :--- |
| File | Written as a locator file: no pages, no compression. |
| Summary | Written into a `DWGPROPS` XRECORD in the named objects dictionary, made under the handle seed where it has none. |
| File dependencies | Left out, noted: `file dependencies left out, a locator file has no place for them`. |
| Application information and its history, revision history, free space | Left out, no note. |
| Macros (`AcDb:VBAProject`), security and any other section dwgio does not type | Left out, noted: `section AcDb:VBAProject left out, a locator file has no place for it`. |
| Block records, inserts, polylines | Their lists written as chains, `First`, `Last` and each entity's links. |
| Every entity | A true color, its names, book and reference dropped, noted, the index kept. A transparency dropped, noted. |
| Dimension styles | Their colors reduced to the index, a true color noted. |
| Light | A true color dropped, noted, the index kept. |
| MTEXT | Its background written into `ACAD`'s `ACAD_MTEXT_BBRT` extended data, and its line spacing into `MTEXTBEGIN`, as AutoCAD's save writes them. |
| Hatch | Its gradient dropped, noted. |
| Viewport | Its shade plot dropped, noted. |
| Layout | Its shade plot, plot view and viewports dropped, each noted. |
| Page setup | Its plot view written as a name instead of a VIEW record, noted when the table lacks it; its shade plot dropped, noted. |
| Header | R2004's variables dropped, no note. |
| ACIS entities | Bodies written as SAT. |
| Untyped shape history nodes | Converted: their color. |
| BREAKDATA | Converted: each break point given the null handle R2000 holds for it, and rid of the bit R2004 adds. |

## Below R2000 {#r14}

Saving as R14.

| What | How |
| :--- | :--- |
| Every entity | Its lineweight into `ACAD` extended data (`RTLwPs`), noted when the drawing has no `ACAD` application. Its ByLayer, ByBlock or Continuous linetype as the record's handle. Its plot style dropped, noted. |
| Layer | Its lineweight dropped, noted; its plot style dropped, no note. |
| Paper space viewport | Its view written into `ACAD`'s `MVIEW` extended data. A clip entity dropped, noted; its own UCS dropped, no note. |
| Dimension styles and header | The arrow blocks by name. `DIMLUNIT` and `DIMFRAC` into `DIMUNIT`, `DIMATFIT` and `DIMTMOVE` into `DIMFIT`. `DIMLWD`, `DIMLWE` and the other R2000 variables dropped, no note. |
| Header | R2000's variables (`INSUNITS`, `HYPERLINKBASE`, the plot style mode, the orthographic UCS settings, the GUIDs and others) dropped, no note. |
| Block record | Its layout, list of inserts, description and preview dropped, no note. |
| Dimension | Its measurement fields (`Attachment`, `LineSpacingStyle`, `LineSpacingFactor`, `Measured`) dropped, no note. |
| LEADER | Its R14 fields derived as AutoCAD's save derives them: `Gap` and `Dimasz` from `DIMGAP` and `DIMASZ` times `DIMSCALE` (0 taken as 1), `Color` from `DIMCLRD` (ByBlock as the leader's own color, ByBlock again as 7), `Arrowhead` 8 for an arrow drawn by a block, each from the leader's dimension style under its `DSTYLE` overrides. The arrow block's name written first in the `DSTYLE` overrides (`1070` 5, the name upper case without AutoCAD's underscore, empty for the closed filled arrow), since R14 draws the leader's arrow by `DIMBLK`; the `ACAD` record then follows any other application's. |
| MTEXT | Its line spacing kept in `MTEXTBEGIN` extended data, as below R2004. |
| OLE frame | Its mode dropped, no note. |
| Dictionaries | Entry names in upper case, every character but a letter, a digit, `$`, `-`, `_` and `*` as `_`, a name longer than 31 characters cut to 26; the names changed kept in an `EXTNAMES` record, as AutoCAD's R14 save keeps them. Table records keep their names. |
| LWPOLYLINE, HATCH, XRECORD | Stored under the class R14 keeps them under, declared where the drawing lacks it. |
| Auxiliary header | Left out, no note. |

## Left out at any step {#unknown}

An `Unknown` crosses a step where AutoCAD writes its class the same way on both sides, or where dwgio converts what differs, as the rows above say for the dynamic block objects, DIMASSOC, object context data, shape history nodes, view styles, break data and the associative network below R2013. A constraint group, which lists its dependencies in another order at each save, crosses between R2013 and R2018. Any other class whose bits change at a step is left out and noted, in a save to a newer release as well as an older one:

- the associative network (actions, dependencies, parameters, action bodies, variables and the network itself) across R2013, where AutoCAD moves the action bodies' parameters into the actions, all of it or none of it, each object noted as left out with its associative network; and across R2007 a network holding a constraint group, a geometry dependency or a dependency body, whose texts lie among data whose layout is not known: a constraint group's nodes, each of a class of its own, a geometry dependency's persistent subentity;
- a solid history's evaluation graph with everything it owns, in any step that leaves one of them out, as for a node of a class not found in AutoCAD's files; the solid's `History` is then cleared and noted;
- the constraint parameters, block properties tables and user parameters across R2007;
- a table's content or geometry holding what dwgio does not read, across R2007 and R2018.

The [DWG format notes](https://github.com/cadsmith/dwgio/blob/main/docs/dwg.md#untyped-objects-across-releases) list where each class's bits change. Two typed kinds are left out the same way: an ACIS entity whose body cannot be written as the target, and a mesh whose overrides dwgio keeps as bits. A pointer of R2007 and later to an object left out, such as a layer's material or a view's visual style, is cleared and noted.

The sections dwgio does not type, such as the macros (`AcDb:VBAProject`) and the security, are written back in the drawing's own release and left out, noted, in any other. The signature is left out of every save, its own release included, and noted: it signs bytes a save does not keep. The application information is never kept: every save to a paged release makes a new one, naming dwgio.
