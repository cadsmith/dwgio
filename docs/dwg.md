# DWG format notes

dwgio reads and writes AutoCAD DWG files of every release from R14 to R2018:
R14, R2000, R2004, R2007, R2010, R2013 and R2018 (`AC1014`, `AC1015`,
`AC1018`, `AC1021`, `AC1024`, `AC1027` and `AC1032`). These notes cover what
it reads and keeps, and the format as AutoCAD writes it, where it differs
from the *Open Design Specification for .dwg files* (Open Design Alliance).
What a save to another release converts and drops is in
[Compatibility](concepts/compatibility.md), and what dwgio does not do in
[Limits](concepts/limits.md).

- [What is kept](#what-is-kept)
- [The format as read](#the-format-as-read): the file, the paged file, the
  coded file of R2007, the object map, objects, entities and named objects
- [Across releases](#across-r2000-and-r2004): what each kind stores
  differently from one release to the next
- [Fixtures and checks](#fixtures-and-checks), and what AutoCAD accepts
- [Differences from AutoCAD's saves](#differences-from-autocads-saves)

## Use

[Getting started](guide/start.md) introduces the interface. In short:

```cpp
#include "dwgio/dwgio.hpp"

#include <print>

auto main() -> int
{
  const auto loaded = DwgIO::LoadFile("plan.dwg");
  if (!loaded)
  {
    std::println("cannot load plan.dwg: {}", loaded.error().What);
    return 1;
  }
  for (const DwgIO::Object &object : loaded->Drawing.Objects)
  {
    std::println("{} {}", DwgIO::HexOf(DwgIO::BaseOf(object).Self), DwgIO::TypeOf(object));
  }
  DwgIO::SaveOptions options;
  options.Release = DwgIO::Version::R2018;
  const auto saved = DwgIO::SaveFile(loaded->Drawing, "plan-r2018.dwg", options);
  if (!saved)
  {
    std::println("cannot save plan-r2018.dwg: {}", saved.error().What);
    return 1;
  }
}
```

## What is kept

A drawing read and written back to the same release gives the same bytes in
R14 and R2000, and from R2004 the same sections once decompressed, but for
the application information.

- The table records are typed under `Tables`: layers, linetypes, text
  styles, views, coordinate systems, viewports, applications, dimension
  styles and block records, with each table's control object.
- The entities are typed under `Entities`: LINE, POINT, CIRCLE, ARC,
  ELLIPSE, RAY, XLINE, 3DFACE, SOLID, TRACE, SHAPE, TEXT, ATTDEF, ATTRIB,
  MTEXT, BLOCK, ENDBLK, SEQEND, INSERT and MINSERT; POLYLINE in its four
  forms with its VERTEX kinds, LWPOLYLINE, SPLINE, HATCH, DIMENSION in its
  seven forms, LEADER, TOLERANCE, MLINE and VIEWPORT; MULTILEADER,
  ARC_DIMENSION, LARGE_RADIAL_DIMENSION, IMAGE, WIPEOUT, the three
  underlays, OLE2FRAME, ACAD_TABLE, HELIX, MESH, LIGHT and SECTIONOBJECT.
- The ACIS entities are typed under `Entities`: REGION, 3DSOLID, BODY and
  the surfaces SURFACE, PLANESURFACE, NURBSURFACE, EXTRUDEDSURFACE,
  LOFTEDSURFACE, REVOLVEDSURFACE and SWEPTSURFACE, with their SAT or SAB
  body, wires, silhouettes and each surface's own data. From R2013 their
  bodies live in the data store, kept as bytes, which a save to another
  release reads and writes (see ACIS bodies across releases).
- The named objects are typed under `Objects`: DICTIONARY and its form with
  a default, XRECORD, LAYOUT, PLOTSETTINGS, GROUP, MLINESTYLE, SCALE,
  SORTENTSTABLE, ACDBPLACEHOLDER, DICTIONARYVAR, MLEADERSTYLE, TABLESTYLE,
  DATATABLE, TABLECONTENT, TABLEGEOMETRY, CELLSTYLEMAP, MATERIAL,
  VISUALSTYLE, and the image and underlay objects; the viewport entity
  header is under `Tables`. The detail and section view styles stay
  untyped and cross every release by their layout (see Visual and view
  styles across releases).
- Every typed kind writes back bit for bit from its fields.
- From R2004 a block record lists the entities it owns, an insert its
  attributes and a polyline its vertices, where R2000 chains them through
  `First`, `Last` and each entity's links. A dimension style's colors are
  true colors; a viewport's shade plot, an MTEXT's background, a hatch's
  gradient and a layout's shade plot, plot view and viewports are typed.
- Every other object is an `Unknown`: its type, its class, whether it is an
  entity, the handles and extended data every object has, the common entity
  data when it is one, and its bytes from its size through its CRC, written
  back unchanged. Its own bits are also kept apart from its header: the
  data after the common data, the remainder of the handle stream without the
  padding to the byte, and from R2007 the string stream. A save to another
  release writes them after a new header.
- The header variables of chapter 9 of the specification, R13 to R2018, are
  typed one member each and write back bit for bit; the bits after the last
  item are kept.
- The classes of chapter 10 are typed and write back bit for bit.
- The preview images are kept by record.
- The object map is computed on save from the objects. The second header is
  kept in `Drawing::Kept`, and a save computes its addresses, records and
  handles from the header.
- The summary is typed as `Drawing::SummaryInfo`, from `AcDb:SummaryInfo` or,
  in R14 and R2000, from the `DWGPROPS` record of the named objects
  dictionary; the file dependencies as `Drawing::FileDependencies`, from
  `AcDb:FileDepList`. Each is typed only when it writes back to the bytes it
  was read from, and kept as bytes otherwise.
- The auxiliary header, the pad before the objects, the measurement, and any
  bytes no block claims are kept unchanged, by name, in `Drawing::Kept`;
  `Drawing::Order` keeps the blocks' order. The sections of a paged file
  dwgio does not type (`AcDb:AppInfoHistory`, `AcDb:RevHistory`,
  `AcDb:ObjFreeSpace`, the macros and the security) are kept the same way
  and written back in place. The signature is never kept. A save as R2004
  of a drawing that lacks them makes the three sections AutoCAD requires.
- The application information names the program that saved the file. It is
  not kept: every save writes a new one naming dwgio, as AutoCAD's saves name
  AutoCAD, so a file dwgio wrote never claims an Autodesk application saved
  it last.

## The format as read

The reference is the *Open Design Specification for .dwg files*, version
5.4.1, called the specification below. Where the files AutoCAD 2026 writes
disagree with it, the notes below follow the files.

### The file

An R13 to R2000 file opens with its release in six characters, five zero
bytes, the maintenance version, a byte of 1, the offset of the preview
images, two more bytes, the code page number, and the locator records: a
count, then for each a number, an offset and a size. A CRC and a sentinel
close it. Record 0 locates the header variables, 1 the classes, 2 the object
map, 4 the measurement and, from R2000, 5 the auxiliary header.

AutoCAD writes the blocks in this order: the auxiliary header, the preview,
the header variables, the classes, four bytes of pad, the objects, the
object map, the second header, the measurement.

### The paged file

An R2004 file opens with the same six characters and code page, a byte of 3
for its flag, the maintenance version and the offset of the preview images.
At 0x80 lies a record of 0x6C bytes XORed with a sequence the generator
`seed = seed * 0x343FD + 0x269EC3` makes from 1, taking each seed's third
byte. The record holds the id `AcFssFcAJMB`, the page size 0x7400, the
count and address of the pages, the numbers of the page map and the section
map, and a CRC32 over the record with its own field zeroed. The bytes from
0xEC to 0x100 are the sequence's own; a copy of the record closes the file.

Every section's data is cut into pages of 0x7400 bytes, each compressed on
its own with the LZ77 of chapter 4.7 and stored under a header of eight
longs XORed with `0x4164536B ^ address`: a type, the section, the compressed
size, the page's extent, the page's start in the section, and two checksums
of chapter 4.2, the data's and the header's seeded with it. The page map and
the section map are system pages: a header of a type, the sizes and a
checksum, the compressed map, a trailer and the sequence's bytes to the next
0x20. AutoCAD finds sections in the map by name. The summary, the
application information, its history and the preview are stored
uncompressed, in pages of their own sizes, the preview rounded up to a
multiple of 0x1000.

Beyond the specification:

- A literal length byte of 0x0F means 18 bytes. The compressor's window
  carries across a section's pages; the decompressor's output is zero-filled
  to the page size, as AutoCAD's own pages may end short.
- AutoCAD compresses every page whole, including the zeros after the
  section's last byte, and refuses a page that decompresses short. Its
  decoder also refuses a stream that does not end in `0x11` followed by two
  zero bytes counted in the compressed size, one whose last thirteen bytes
  are not literals, a copy shorter than four bytes, and the opcodes `0x18`
  to `0x1F`, which it never writes: a far copy of ten bytes or more takes the
  long form. A copy may follow a copy, and copies may overlap and reach back
  one byte, as the specification allows.
- AutoCAD verifies both checksums of every data page and the compressed
  sizes the section map repeats, so no page can be swapped alone. The
  section map lists the nameless section first and the others by descending
  id; the record counts the system pages among its pages.
- The objects section opens with the long `0x0DCA`; the object map's offsets
  count from the section's start. The classes section is the locator file's
  frame with eight zero bytes after it, its preamble the highest class
  number, two zero bytes and a true bit. A class record from R2004 counts its
  instances and names the library version that wrote it, 22.42 for
  AutoCAD's core classes. The `AcDb:Template` section is the four bytes of
  the measurement block; `AcDb:AuxHeader` is the auxiliary header of chapter
  27 to the byte.
- AutoCAD refuses a file without `AcDb:RevHistory` (four longs, 0 0 1 0),
  `AcDb:ObjFreeSpace` (two longs of 0xFFFF0000, the update time, a zero, and
  the four 64-bit values the specification lists) or `AcDb:AuxHeader`, and
  opens one without the summary, the application information, the preview
  or the template.

### The coded file of R2007

R2007 opens with the first 0x80 bytes of R2004, in the clear, and codes
everything after them against damage with the Reed-Solomon codes of chapter
5.13: (255, 239) over the field of polynomial 0x169 for the file header and
the system pages, (255, 251) over 0x11D for the data pages. A codeword's
bytes in file order are the message's ascending powers, the generator's roots
are alpha^1 onwards, and the codewords are interleaved byte by byte.

The file header at 0x80 is 0x400 bytes: three codewords holding, as often as
they fit, a check sequence, its CRC, the compressed fields' CRC and length,
and the 0x110 bytes of fields compressed; a copy closes the file. The fields
place the page map at the start of the pages, 0x480, with its copy after
it, and name the section map's page. Both maps are system pages, their data
repeated as often as the page holds it. The section map gives each section
its size, page size, name hash, encoding and pages, each page with its
start, its sizes, the checksum of chapter 5.4.1 and a 64-bit CRC of chapter
5.12; a nameless section closes it.

Beyond the specification:

- The compression of chapter 5.10 copies a literal run in chunks of 32 whose
  blocks of two and three bytes are reversed and whose blocks of sixteen
  have their halves swapped. A first opcode of 0x2X is a reference of
  nothing whose last byte gives the first run's length.
- The 64-bit CRCs take each eight bytes as its pairs 6, 4, 2 and 0, as the
  page checksum does; the mirrored one's polynomial is 0x95AC9329AC4BC9B5.
  The maps' CRCs are mirrored, the file header's normal.
- AutoCAD stores the summary, the application information, its history and
  the preview with encoding 1: the bytes plain at the page's start, followed
  by a parity dwgio does not reproduce. It reads them as well with encoding
  4, which dwgio writes for every section.
- AutoCAD checks each section's name hash, whose low half is the sum of the
  name's characters plus its length and one. dwgio writes the hashes chapter
  5.6 lists, and leaves out, with an issue, a section it has none for.
  AutoCAD refuses an uncompressed section map, an uncompressed file header
  whose negative length is not 64 bits, and a file whose encoded seeds it
  did not make; dwgio writes the seeds AutoCAD's files carry. It reads
  neither the check data at 0x3D8 nor the random padding, which dwgio leaves
  zero.

### The object map

The map of chapter 23 is cut into sections, each a big-endian size, the
handle and offset differences, and a CRC. AutoCAD counts each section's
differences from zero, where the specification resets the last handle and
offset only once for the whole map. It closes a section once it reaches
2032 bytes, including the entry that reached them, so a section may hold a
few bytes more. A map of a few hundred objects fits in one section; a map of
several sections read the specification's way has most of its later entries
pointing into the middle of other objects.

### Bits and CRC

Everything but the file header and the object map is bit-coded, most
significant bit first, as chapter 2 describes: `B`, `BB`, `3B`, `BS`, `BL`,
`BLL`, `BD`, `MC`, `MS`, `H`, `DD`, `BT`, `BE`, `CMC`, `ENC`, `OT`, `T`,
`TU` and `TV`. The CRC is the reflected polynomial `0xA001`, seeded with
`0xC0C1`; the specification's seed of zero with a per-block magic gives the
same number. The object map stores its sizes and CRCs big-endian.

- AutoCAD ends every non-empty text with a NUL counted in its length, and
  pads the classes, each object and the second header to the byte with one
  bits. The reader drops the NUL and the writer puts it back.
- A modular char's last byte keeps bit `0x40` for the sign, so a positive
  value whose last seven bits reach it takes one more byte.
- The code page of an extended data string is stored big-endian.
- A `BLL`'s length is three plain bits, not the `3B` triplet the
  specification points to: R2013's REQUIREDVERSIONS of 0 takes three bits.

### Objects

Each object is a size, a type, for R2000 the bit position of its handles,
its handle, its extended data, its data, its handles, padding and a CRC over
the size and the data. An entity adds the common entity data of chapter
20.4.1 before its own. The handles follow chapter 20.4.2: the owner when the
mode says so, the reactors, the extension dictionary, then for an entity the
layer, the linetype and, from R2000, the plot style, with the links to the
entities before and after. R13 and R14 store the layer before those links,
R2000 after them.

- A reference with code 6, 8, 0xA or 0xC is an offset from the object's own
  handle. AutoCAD writes an owner as an offset only while the distance is
  smaller than the handle itself, and absolute otherwise.
- A kind AutoCAD stores under a class is written under the class of its name
  in the drawing's CLASSES. Before R2004 this includes LAYOUT and
  ACDBPLACEHOLDER, though chapter 20.3 gives them a fixed type; from R2004
  AutoCAD stores those two under the types 82 and 80, the classes still
  declared, and so does dwgio.
- From R2004 the common data carries, after the reactor count, a bit set
  when no extension dictionary handle follows. An entity keeps no links to
  its neighbours, and its color is an ENC whose flags say whether a true
  color, a reference to an AcDbColor object after the layer, and a
  transparency follow.
- A CMC's long carries the color method in its top byte: `0xC3` with the
  index in the low byte, `0xC0` by layer, `0xC1` by block, `0xC2` a true
  color. The index before it is written 0.

### Table objects

A table record opens with its name, the 64 bit of the DXF 70 group, the xref
index and the xdep bit; its handle stream begins, after the common handles,
with the external reference block. Where AutoCAD's files differ from the
specification:

- A layer's "on" bit, in R14's bit and R2000's values short alike, is set
  when the layer is off. No handle follows the linetype.
- A style stores the shape file bit before the vertical bit.
- A dash's flags are DXF's own, 2 a text and 4 a shape, with 8 for an upright
  text; the specification swaps 2 and 4. A plain dash's scale is 1.
- The linetype control lists ByBlock then ByLayer after its entries, as hard
  owners. The dimension style control counts extra hard pointers with a byte
  from R2000. The view control may list null entries.
- A viewport stores its view width where the specification reads an aspect
  ratio.
- Every paper space block record is named `*Paper_Space`; AutoCAD's DXF
  numbers the later ones.
- An R14 file keeps table names in upper case and names the built-in dot
  arrow `DOT`; it has no DIMDSEP, DIMADEC, DIMFRAC or DIMLUNIT.
- From R2004 a block record stores a count of its entities after its data
  and that many hard owners after its BLOCK and ENDBLK, and no `First` and
  `Last`. A dimension style's three colors are CMC. R2004 keeps no viewport
  entity headers and no table of them, and the header has no handle for one.

### Entities

An entity's own data follows the common data of chapter 20.4.1; its handles
follow the common ones. Beyond the specification:

- The owner is written only when the mode is 0, as a reference relative to
  the entity's own handle: code 6 or 8 for the handle one above or below it,
  0xA or 0xC with the distance otherwise, 4 for null. The links to the
  entities before and after are written the same way, and left out when
  they are the handles next to the entity's own; the reader fills them in.
- R2000's LINE packs its end against its start, and its 3DFACE each corner
  against the one before, each with a bit saying the Z are zero. A 3DFACE
  with no invisible edge leaves its flags out behind another bit.
- R2000's TEXT, ATTDEF and ATTRIB open with a data flags byte, a bit for
  each value left out at its default: the elevation, the alignment point
  when it is the insertion point, the oblique angle, the rotation, the
  width, the generation and the two alignments. AutoCAD sets each bit
  exactly when the value is at its default.
- R2000's INSERT keeps its scale behind two bits: 3 for unit scale, 1 for X
  at one with Y and Z as deltas, 2 for one value shared, 0 for all three.
  Its block is a hard pointer, its first and last attribute absolute soft
  pointers, its SEQEND a hard owner. A MINSERT is the same object under type
  8 with its grid.
- An R14 file names the ByBlock and Continuous linetypes of an entity through
  their records where R2000 uses the flags, and keeps an entity's own
  lineweight in `ACAD` extended data: `RTLwPs` with a 1070 of 128 plus the
  lineweight code. A save to the other release translates both.
- An R14 paper space viewport stores only its center, width and height. Its
  view is in `ACAD` extended data: `MVIEW`, a brace, the version 16, the
  target and direction as points, the twist, height, center, lens and clips,
  the view mode, circle zoom, fast zoom (1 in AutoCAD's files), UCS icon,
  snap, grid, snap style and isopair as shorts, the snap angle, base and
  spacing and the grid spacing, whether hidden lines are plotted, the frozen
  layers as 1003 handles in a brace of their own, and the closing brace.
  AutoCAD refuses an R14 file whose viewport lacks it. A save to R14 makes
  it from R2000's fields as AutoCAD does, and notes a clip entity R14 cannot
  keep. A save from R14 reads it into the fields, with the grid shown beyond
  the limits and adaptive as AutoCAD's R2000 viewports have it, and drops
  it.
- AutoCAD gives a SEQEND a null plot style of its own. The BLOCK and ENDBLK
  of each layout after the first are owned by their block record through
  mode 0, though DXF flags them with 67 as paper space.
- A 2D VERTEX keeps whatever Z AutoCAD had: for a raised polyline the first
  holds the elevation and the next ones zero; the elevation is the
  polyline's. Equal non-zero widths are written as one negative value and no
  end width. A 3D POLYLINE stores two bytes, the first 1 or 2 for a
  quadratic or cubic spline fit, the second 1 when closed; a polyface mesh
  stores no flags.
- An R14 file keeps LWPOLYLINE and HATCH under a class whose DXF name is the
  kind's; R2000 under the fixed types 77 and 78. The reader matches a class
  by its name as it matches a fixed type by its code.
- An LWPOLYLINE's flag says what follows: 1 an extrusion, 2 a thickness, 4 a
  constant width, 8 an elevation, 16 a bulge per vertex, 32 a width pair per
  vertex, 256 PLINEGEN, 512 closed. AutoCAD writes the bulges when any is
  not zero and the width pairs when any vertex has one. It stores a constant
  width of zero behind its bit for some polylines and not for others, where
  DXF writes group 43 as zero either way.
- A SPLINE by fit points stores its tolerance, its two tangents and the fit
  points, and nothing of the knots and control points. One by control points
  stores the closed, periodic and rational bits, the two tolerances, the
  knots, and the control points with a weight each when rational.
- AutoCAD stores every HATCH boundary as edges, a polyline's as lines and
  arcs, and writes no polyline path. A derived boundary's pixel size follows
  the pattern when any path's flags have 4.
- A DIMENSION's first flag byte carries 8 in every file AutoCAD writes,
  beside the two bits the specification names. The elevation stored once is
  the Z of the text's middle, of the clone point and of a two-line angular
  dimension's arc point. An R14 save draws the dimension blocks again, so
  their entities have no R2000 twin by handle.
- A VIEWPORT's clip entity is a hard pointer, where the specification says
  soft. A LEADER of an R14 file carries an extended data record its R2000
  twin lacks.
- From R2004 an INSERT stores a count of its attributes and that many hard
  owners (soft pointers for a MINSERT), under the attribute bit only, and a
  POLYLINE a count of its vertices, as hard owners for every form where the
  specification says soft pointers for the mesh and face forms, each before
  the SEQEND. A VIEWPORT's frozen layers are references of code 4 and its
  shade plot a `BS`. An MTEXT stores its background flags and, when bit 1 or
  16 is set, the scale as a `BD`, the color and the transparency. A HATCH
  stores the gradient block of chapter 20.4.75 before its elevation: for a
  plain hatch, an unused linear fill from blue to yellow with tint 1. In a
  dimension's regenerated block AutoCAD gives each entity a ByBlock
  transparency.

### Named objects

- A dictionary keeps one hard owner flag for all its entries, not one per
  entry as DXF's 350 and 360 do. R14 keeps that flag in the byte the
  specification calls unknown, and a dictionary stored under its "with
  default" class carries the cloning flag in R14 too. R14 keeps dictionary
  keys in upper case and fewer entries in the round-trip dictionaries.
- An xrecord's data is DXF groups coded by their group number, each kind as
  DXF types the code: a text is an RS count, an RC code page and the
  characters; a point three doubles under one group (10 to 18 and 210 to
  239, where 38 and 39 are single doubles); a handle eight little-endian
  bytes in the data; a 160 group eight bytes of integer. The 1000 range
  follows the extended data's kinds, as R14's dimension style overrides use
  it. What does not parse stays as bytes after the groups that did. A chunk
  over 255 bytes or a text over 65535 is cut on write and noted.
- An R2000 layout stores no plot view handle and no viewport list; after the
  common handles come its block, its last active viewport and the two UCS
  records. From R2004 its plot settings end with the three shade plot
  shorts (AutoCAD's plain values are 0, 2 and 300), its viewport count after
  the extents is a `BL` where the specification says `RL`, the plot view
  comes as a hard pointer after the extension dictionary, and the viewports
  as soft pointers last. AutoCAD makes a layout's extension dictionary, its
  variables and many xrecords again at each save, under new handles.
- A viewport entity header's viewport is a soft pointer. One header per
  drawing stands at the chain's root with no viewport and its flag clear.
- A sort table's keys are handles in the data, its entities soft pointers in
  the handle stream after the block.

### The second header

After the file header's sentinel come a size, the header's own location as a
`BL`, six version characters, a hundred bits of constants, one record per
locator entry, a `BS` count of handle records, one bit padding to the byte,
a CRC over everything from the size, and eight bytes of junk, in R2000 as in
R14. The handles are the handle seed, the control objects of the tables, the
viewport entity header control, and the named objects, multiline style and
group dictionaries.

### Across R2000 and R2004

A save from one release to the other translates what the two store
differently.

| What | R2000 to R2004 | R2004 to R2000 |
| :--- | :--- | :--- |
| A block record's, insert's and polyline's owned objects | the list built by walking the chain from `First` through each `Next` to `Last`; a cut chain gives what it reaches, and the audit lists the entities it misses | `First` and `Last` from the list's ends, each member's links from its neighbours; the lists cleared |
| An entity's color | kept | a true color, name, book or reference dropped and noted, the index kept; a transparency dropped and noted |
| MTEXT background, HATCH gradient, VIEWPORT and LAYOUT shade plot, LAYOUT plot view and viewports | the kind's defaults, which are what AutoCAD writes | dropped and noted when set |
| A dimension style's colors | kept | reduced to their index, noted when a true color |
| The viewport entity header table | the header's handle cleared; a record kept from an R2000 file stays, unnamed | none to carry; AutoCAD opens the file without one |
| The header's R2004 variables | `DefaultsOf(R2004)` | dropped without a note |
| The sections | the three AutoCAD requires and the application information made; the summary written from the `DWGPROPS` record, which is removed | left out, the macros, the file dependencies and any other but the application information and its history noted; the summary written into the `DWGPROPS` record; the auxiliary header and the measurement carried |
| An `Unknown` | carried where its class crosses, else left out and noted | the same |

### R2010 to R2018

R2010, R2013 and R2018 keep the paged file of R2004: the record at 0x80
decrypts with the same sequence, and the page map, the section map and the
compression are unchanged. R2013 and R2018 add `AcDb:AcDsPrototype_1b`, the
data store. AutoCAD 2026 stamps R2010 files with maintenance 226, R2013 with
125 and R2018 with 0, and R2018 with zeros where the others have `0x21 0xFF`
at 0x12 and 0x16. It gives the summary a page of 0x100 from R2010, and the
preview pages of 0x800 in every paged release.

What changes is inside the sections, following the specification's R2007
and R2010 chapters except where noted:

- An object is its size as an MS, the size in bits of its handle stream as
  a UMC, the bytes the MS counts, then the CRC over all of them; the MS does
  not count the UMC. The handles start that many bits before the end. The
  bit before them says whether a string stream lies before it. When it
  does, the short before the bit is its size in bits, or with 0x8000 its low
  fifteen bits under another short's high ones, and the stream ends where
  those shorts begin. Every TV of the object is in that stream, as UTF-16
  with a short count, a CMC's names among them; extended data keeps its
  strings in place.
- The header and the classes keep their frame with a second RL after the
  size, from R2010 when the maintenance is above 3 and always from R2018,
  inside the CRC. Their body opens with an RL, the bit at which the handles
  start, counted from the body's first bit. The classes' strings and the
  header's strings and handles lie as in an object, except the header's
  handle seed, which stays in the data. MENU is still in the header (`.`),
  against the specification, and the four R14 shorts that ended it are
  gone. Ten bytes follow the header's handles in AutoCAD's files, kept as
  its padding.
- From R2013 an object and an entity carry a bit, after the one that says no
  dictionary follows, for data in the data store. From R2007 an entity's
  common data has the material flags and the shadow flags, a material and,
  when the shadow flags are 3, a handle AutoCAD writes null; from R2010
  three bits for the visual styles it names. The proxy graphics' size is a
  BLL from R2010.
- Table records hold no xref index from R2007, only the 64 flag and the xdep
  bit.
- The kinds gain what chapter 20 lists, and more it does not:
  - R2013's layer gains a null handle after its linetype;
  - R2007's block record gains its units, whether it explodes and its
    scaling;
  - the attribute gains a version byte from R2010, its type from R2018 and a
    lock bit from R2007; the definition gains a second version byte;
  - the dimension gains a version byte from R2010 and three bits from R2007;
  - the 2D vertex and, by flag 1024, the lightweight polyline gain their
    identifiers from R2010;
  - the viewport entity, the view and the viewport record gain their
    lighting and visual style from R2007, and the view its live section;
  - the dimension style gains the variables the header gained;
  - the linetype gains a strings area of 512 bytes of UTF-16, present only
    when a dash has text;
  - the layout gains its visual style;
  - MTEXT gains the height of its rectangle from R2007, and from R2018 a
    frame that repeats its attachment, direction, position, rectangle and
    extents (version 4 in AutoCAD's files), then its columns;
  - R2013's spline gains its flags, whose first bit says it is by fit points
    where the scenario no longer does, and its knot parameter;
  - a hatch's spline edge gains its fit points, and tangents with them, from
    R2010;
  - an xrecord's text becomes UTF-16;
  - R2018's multiline style names its linetypes by handle.

  The leader drops its text box from R2010, though the text keeps it.

### Across R2004 and R2010 to R2018

| What | To a newer release | To an older release |
| :--- | :--- | :--- |
| The header's R2007 to R2013 variables | `DefaultsOf` the release, the visual style dictionary found among the root's entries | dropped without a note; the four R14 shorts from `DefaultsOf` |
| A linetype's strings area | 512 bytes of UTF-16 when a dash has text, none otherwise, each text laid out again | 256 bytes in the code page; a text the area cannot hold is left out and noted |
| A multiline style's linetypes, by index before R2018 and by record from it | the record at that index among the linetype table's entries, ByLayer's for 32767, ByBlock's for 32766 | the record's index the same way; one the table lacks becomes ByLayer and is noted |
| A spline saved to R2013 or later | AutoCAD's flags and knot parameter: 9 and the chord by fit points, 0 and 15 by control points | dropped |
| The other fields of the kinds | the kind's defaults, which are AutoCAD's | dropped |
| A pointer of R2007 and later to an object left out, the header's and a view's live section among them | cleared and noted, an entity's material then its layer's: AutoCAD refuses a view naming a missing visual style | the same |
| The auxiliary header, the free space and the template | made in the release's form | the same; as R2000 or R14 the template and the auxiliary header are carried where their form is the release's, made again where not |
| The summary and the file dependencies | written in the release's form | the same; as R2000 or R14 the summary is written into the `DWGPROPS` record and the file dependencies are left out and noted |
| The application information and its history | the information made naming dwgio, the history carried | the same; as R2000 or R14 left out |
| Any other section dwgio keeps, such as the macros | left out and noted | the same; as R2000 or R14 too |
| The data store | made from the ACIS bodies in a save to R2013, kept from R2013 to R2018 | its bodies taken into their entities below R2013, its R2018 bodies written as R2013's; anything else it holds left out below R2013, each object's store bit cleared and the loss noted |

### ACIS bodies and surfaces

REGION, 3DSOLID and BODY (types 37 to 39) and the surfaces, stored under
their classes in every release, R14 included, share the AcDbModelerGeometry
data of chapter 20.4.41, typed as `Entities::Geometry`. Where the
specification leaves it open:

- An ACIS stream is an empty bit and, when clear, a bit the specification
  leaves unknown (1 with SAT, 0 with SAB in AutoCAD's files), the version as
  a `BS`, and the data. Version 1 is SAT in blocks of at most 4096
  characters, each a `BL` size and its bytes, `0x21` to `0x7E` mapped to
  `0x9F` minus themselves, a block of size 0 last; tabs stay tabs. Version 2
  is SAB, not byte aligned, from `ACIS BinaryFile` to the tokens
  `End-of-ACIS-data`, or `End-of-ASM-data` from ACIS 21800, which dwgio
  looks for at each byte. R14 and R2000 write SAT (ACIS 1.06 and 4.00),
  R2004 to R2010 SAB.
- The wireframe follows the body: a bit, then a point behind a bit of its
  own, the isoline count as a `BL`, and only behind the "isolines present"
  bit the wire count and the wires. The silhouette count follows, read
  outside the wireframe's bit, which AutoCAD's files always set. Then come a
  second empty bit, which the specification says AutoCAD sets, and from
  R2007 a `BL` of 0. An R2000 save AutoCAD makes of an R2013 NURBS surface
  clears that bit and writes a second SAT stream there, a body per isoline;
  dwgio keeps it as `Geometry::Second`.
- From R2013 the object holds no first body. The wireframe comes first, then
  the second stream behind its empty bit, the `BL`, a bit for the body being
  in the data store (DXF's 290), the GUID as a `BL`, two `BS` and eight
  bytes (DXF's group 2, kept as its text), and a second `BL` of 0. AutoCAD
  puts every body in `AcDb:AcDsPrototype_1b`, a NURBS surface's isoline
  stream inline as SAB too. A BODY may have the entity's data store bit set
  and a null GUID.
- A 3DSOLID ends with its history, an ACSH_HISTORY_CLASS object, in the
  handles from R2007 (DXF's 350), a hard owner.
- A surface adds the two isoline counts as `BS` (71 and 72). A planar
  surface and a SURFACE, AutoCAD's name for a NURBS surface up to R2010,
  stop there; R2013's NURBSURFACE adds a `BS` (170), a bit (290) and four
  points (10 to 13). The procedural surfaces keep what DXF writes after the
  isolines, in another order:
  - an extruded surface: the sweep options (the six reals 42, 43, 44, 45, 48
    and 49, the solid bit, the alignment 70 and 71 as `BS`, the bits 292, 293
    and 294, the reference vector 11, the bits 295 and 296, the matrices 46
    and 47 as sixteen `BD`), the sweep vector 10, the matrix 40 and its
    profile;
  - a swept surface: the options, the matrices 40 and 41, its profile and
    path;
  - a revolved surface: the four reals 43 to 46, the bits 290 and 291, the
    axis point and direction, the angles 40 and 41, the matrix 42 and its
    profile;
  - a lofted surface: the matrix 40, the counts of its sections and guides,
    a bit for a path, the four reals 41 to 44, the eight bits 290 to 297, the
    normal 70, then the sections, the guides and the path.

  A profile is the entity's type code as a `BL` (DXF's first 90, 19 for a
  LINE), its size in bits as a `BL` (the second 90) and the bits, which
  DXF's 310 writes with one byte more. These tails are the same bits in
  every release.

What AutoCAD 2026 accepts, and so what a save writes:

- It refuses SAB below R2004 and reads SAT in any release. It drops a body
  written in the object from R2013, and its audit erases a 3DSOLID with
  none. A save converts the body to the release's own ACIS and moves it into
  or out of the data store (see ACIS bodies across releases).
- Before R2007 it gives an extruded and a swept surface ACAD extended data,
  `AcDbSavedByObjectVersion` and a short of 0, and refuses one of R2004
  without it: a save below R2007 adds it, a save from it drops it.
- A NURBS surface is a SURFACE below R2013 and a NURBSURFACE from it, and a
  save renames it accordingly; a solid's history is cleared when the history
  is left out.
- A save to R14 or R2000 that leaves out an object carries its neighbours'
  links, and a block record's, polyline's or insert's ends, past it: AutoCAD
  refuses a chain that runs into an object the file lacks.

In DXF, `Geometry` maps to these groups: the version to group 70 (1 in every
DXF of R2000 to R2010, which converts SAB to SAT), the SAT text to groups 1
and 3 line by line, each encrypted the same way, a tab written as a space and
a caret as `^ `; from R2013 `Stored` and `Guid` to 290 and 2, and the body in
the data store to ACDSDATA. The second header line of the SAT holds the
save's date, which DXF leaves out.

### ACIS bodies across releases

AutoCAD writes a body in the ACIS of the release it saves: 1.06 as SAT in
R14, 4.00 as SAT in R2000, then SAB of 20800 in R2004, 21200 in R2007, 21500
in R2010, 21800 in R2013 and 22300 in R2018. dwgio reads SAT or SAB into one
model (`source/acis`), each record a list of values in the order of every
release's fields, and writes it as any of the seven.

SAB holds the same records as SAT, each value behind a byte that gives its
kind: 0x04 a long, 0x06 a double, 0x07 text behind a byte of its length
(0x08 behind two), 0x0A and 0x0B a logical true and false, 0x0C a pointer,
0x0D a word or a record name's last part, 0x0E a part a hyphen follows, 0x0F
and 0x10 the braces of a subtype, 0x11 the end of a record, 0x13 a position
and 0x14 a direction of three doubles, 0x15 an enumeration and 0x17 a 64-bit
integer. Its head is `ACIS BinaryFile`, or `ASM BinaryFile4` from 22300,
four longs (the release, 0, 2 and the flags: 0, 26, 24, 12 and 4 from 20800
to 22300), the product, modeler and date as text, and the three tolerances
as doubles. It ends with the record `End-of-ACIS-data`, `End-of-ASM-data`
from 21800, and no 0x11.

SAT's head is a line of the release, the record count, 1 and 0, padded to
twenty characters, and from 4.00 a line of the three texts, each its length
and itself, and a line of the tolerances. Every value is followed by a
space, a double is written with nineteen significant digits, a record ends
with `#` on its line, and a subtype breaks its lists onto lines of their own
behind a tab. SAT names the logicals and enumerations SAB numbers: `forward`
and `reversed`, `single` and `double`, `out` and `in`, `forward_v`,
`no_rotate`, `no_reflect`, `no_shear`, `I` for an infinite bound and `F`
before a finite one's double, the closures `open`, `closed` and `periodic`,
the singularity `none`, and an attribute's actions `copy`, `keep_all` and
`ignore`. ACIS 1.06 writes a sense, a transform's three logicals and an
attribute's actions as numbers, but a face's as words, and has no text
lengths before its first record.

The 22 records found in AutoCAD's files change between releases as follows:

| Change | Release | Saved to a newer release, AutoCAD gives |
| :--- | :--- | :--- |
| A record's history long after its attribute, and for an entity but a transform and the header record a pointer | 20800 | -1 and -1 |
| The header record `asmheader`, first, with AutoCAD's modeler version: every pointer moves by one | 20800 | `231.0.0.65535` |
| An edge's parameters at its start and end vertices, and its convexity | 20800 | the parameters found from the curve; `unknown` |
| A spline subtype's approximation, `full` | 20800 | 0 |
| An attribute's copy action | 20800 | `copy` |
| The subtypes renamed: `exactcur`, `exppc` and `exactsur` to `exact_int_cur`, `exp_par_cur` and `exact_spl_sur` | 21200 | |
| A vertex's long, 0 when it starts its edge and 1 when it ends it | 21200 | 2 |
| A spline surface's version, after its name, and its extension at the end | 21200 | 20900 and 0 |
| An intcurve's fourth long after its nulls | 21200 | 0 |
| An intcurve's version and its two extensions | 21500 | 20900, 0 and 0 |
| A spline surface's logical after its six longs | 21500 | false |
| A coedge's long before its parameter curve | 21800 | 0 |
| A shell's wire pointer; a cone's parameter scale, the length of its major axis; the tails of a spline surface (six longs and four bounds) and an intcurve (two bounds, three longs, two bounds) | 4.00 | -1; the length; zeros, `F 1 F 0` |

A save to an older release drops what the release lacks, as AutoCAD's own
saves do. A save to a newer release gives what the table says, plus two
things for a procedural surface, whose body AutoCAD rebuilds on reading:
each vertex says which end of its edge it is, and an edge along a seam, with
its two coedges on one face, is `tangent`. AutoCAD finds an edge's
parameters as dwgio does:

- along a straight line, the distance from its root in lengths of its
  direction;
- around an ellipse, the angle from its major axis, from a half turn back to
  just short of a half turn on, the end past the start, and a whole turn
  past it when the edge closes;
- along a B-spline, its first and last knots when the edge runs between its
  end points or closes at the first;
- 1 to 0 for an edge without a curve, such as a cone's apex.

A body already in the release's ACIS crosses unchanged, and AutoCAD keeps an
R2013 body of 21800 in an R2018 file. The date in a converted body is the
one it was read with; from R14, which has none, the time of the save.

The data store of chapter 24 (`source/store`) holds a body under the schema
`AcDb3DSolid_ASM_Data` by the entity's handle. AutoCAD's first save lays it
out as dwgio writes it:

- a head of 0x80 bytes: `jard`, 0x80, 8, 2, 0, the revision 1, the segment
  index's offset, 0, the segment count, the indexes of the schema index, the
  data index and the search segment, 0 and the size;
- then the segments, each a header of 0x30 bytes ending in eight bytes of
  0x55, padded with 0x70 to a multiple of 0x80:
  - the segment index: each segment's offset and size;
  - the data index: each record's segment, the offset of its header and its
    schema;
  - one data segment of every record: a header of twenty bytes each (20, 1,
    the handle and the data's offset), 0x62 to sixteen bytes, then each
    record's size and data, or past 0x40000 bytes 0xBB106BB1 and a
    reference to the blob segments of its pages;
  - the schema index and schema data, the same in every first save;
  - a `blob01` segment per page of at most 0xFFFB0 bytes, whose header gives
    1 and 0x30 for its size;
  - the search segment, whose second schema lists every record and each
    handle with its record's index.

AutoCAD ignores a record the search segment lacks. The data segment's
records, the schema index's names and the search data are where the
specification has them; a schema index lists its names after its two
tables, at its system data offset. AutoCAD's own store also holds the
drawing's thumbnail, which dwgio does not make.

Crossing R2013 changes the entities too, as AutoCAD's saves do:

- Below R2013 AutoCAD keeps a body's GUID in extended data of
  `ACAD_OBJECT_CHANGE_GUID`: a short of 1, the GUID's long, two shorts and
  eight bytes as shorts, with the application registered. In a save to a
  newer release, it takes the GUID back and removes the extended data. An
  entity says its body is in the store (group 290) only with a GUID, and a
  NURBS surface never: AutoCAD refuses a file whose entity says so without
  one.
- Below R2013 AutoCAD keeps a NURBS surface's own data in a record under
  `ACAD_XREC_ROUNDTRIP` in its extension dictionary (`AcadNurbSurfaceData`,
  `AcadModlrDecomposedType` and a checksum), and removes it in a save to a
  newer release. AutoCAD refuses an R2013 file whose NURBS surface still has
  it, so a save removes it; dwgio does not make it in a save to an older
  release.
- The second stream of a NURBS surface, its isolines in SAB, is dropped both
  ways, and the entity's wires are kept; AutoCAD remakes them on opening.
- AutoCAD makes a new GUID for a procedural surface in a save to a newer
  release; dwgio takes the one kept below R2013.

### Code pages

Text before R2007 is in the code page the file header numbers: 28 to 30 and
32 to 36 for ANSI 1250 to 1257, 37 for 874, 38 to 42 for the East Asian 932,
936, 949, 950 and 1361 (two bytes a character), and 44 for 1258. A number
the tables lack reads through ANSI 1252 with an issue.

Below R2007, AutoCAD writes a character the code page lacks as `\U+` and the
four upper-case hex digits of its code point. A Japanese layer name in an
ANSI 1252 drawing saved as R2004 becomes `\U+30BF\U+30A4\U+30C8\U+30EB
\U+30D6\U+30ED\U+30C3\U+30AF`, and AutoCAD's audit renames a layer named
with question marks. dwgio writes the same. Beyond the basic plane AutoCAD
keeps four digits of the code point and loses the others; dwgio writes a
question mark.

AutoCAD reads the escape back as the character, so its R2007 and R2018 saves
of such a drawing hold the original names. dwgio reads the escape AutoCAD
writes (`\U+`, four upper-case hex digits, a character the code page lacks)
as that character, so a save to R2007 or later writes the character.
AutoCAD also reads any other escape of four hex digits as the character, in
lower case or of a character the page holds (`\U+0041` reads `A`, `\u+30bf`
the katakana). dwgio keeps those as text, so the drawing writes back to its
own bytes: AutoCAD's dimensions write `\U+00B0` for a degree sign ANSI 1252
holds, in every release, and MTEXT and TEXT show the escape as the
character.

### Dimensions and page setups

- ARC_DIMENSION and LARGE_RADIAL_DIMENSION are stored under their classes in
  every release, R14 included, as entities with proxy flags 1025;
  PLOTSETTINGS under its class with proxy flags 0. Each writes back bit for
  bit from its fields in every release.
- Both dimensions open with the dimension data of chapter 20.4.22, as a
  DIMENSION does, the 8 of the first flag byte included. DXF's group 70
  gives the kind in its low four bits, 5 for an arc length and 9 for a
  jogged radius, which `Measure::Flags` leaves out as it does a
  DIMENSION's.
- An ARC_DIMENSION follows chapter 20.4.19: the definition point, which lies
  on the dimension arc, the arc's two ends and center, whether part of the
  arc is measured as a bit, the two angles of that part, whether there is a
  leader as a bit, the leader's two points, then the style and the block as
  hard pointers. DXF writes the two bits as the shorts 70 and 71.
- A LARGE_RADIAL_DIMENSION stores, after the center that is its definition
  point, the chord point on the curve, the jog's angle as a `BD` the
  specification calls unknown, the center the dimension is drawn from, and
  the jog point. The specification numbers them 13, 40, 14 and 15; AutoCAD's
  DXF writes the chord point as 15, the center drawn from as 13 and the jog
  as 14.
- An R14 file gives both dimensions proxy graphics of R14's own form, other
  bytes than R2000's; from R2000 they are DXF's 310 groups.
- A PLOTSETTINGS holds the plot settings a LAYOUT opens with, as chapter
  20.4.84 gives them: the view by name up to R2000 and the three shade plot
  shorts from R2004, then a hard pointer to the plot view from R2004 and a
  soft pointer to the visual style its shade plot takes from R2007 (DXF's
  333), the two a LAYOUT's plot settings also carry.
- An ACDBDICTIONARYWDFLT is an `Objects::Dictionary` whose `Default` holds a
  value: the entries, the hard owner flag and the cloning flag of a
  dictionary, then the default entry (group 340), a hard pointer after the
  entries.
- Saved to another release, the two dimensions cross as a DIMENSION does. A
  page setup's plot view becomes the VIEW record of its name in a save to
  R2004, and the name of its record in a save to R2000, noted when the table
  has none. Its shade plot is dropped in a save to R2000 and its visual style
  in a save to R2004, each noted when set. A visual style left out of the save
  is cleared and noted.

### Rasters, underlays and OLE frames

- AutoCAD stores the images, the wipeouts, the underlays and every object
  that belongs to them under their classes in every release, R14 included:
  the image classes under the application `ISM`, the wipeout's and its
  variables' under the long `WipeOut|Product Desc: ...` names, quotes and
  runs of spaces as AutoCAD writes them, the underlays' under `ObjectDBX
  Classes`. A save that must declare one declares it so.
- An IMAGE is chapter 20.4.80 as written: the version, the three vectors,
  the size as 2RD, the display flags, the clipping bit, the brightness,
  contrast and fade bytes, from R2010 the clip mode bit, and the boundary:
  its kind as a BS, then for a rectangle (1) its two corners as 2RD with no
  count, for a polygon (2) a BL count and the vertices. The definition is a
  hard pointer, the reactor a hard owner. DXF's group 91 counts a polygon's
  first vertex again at its end, which the file leaves out: a wipeout's
  rectangle is four vertices in the file and five in DXF.
- A WIPEOUT has the image's layout whole: a size of 1 by 1, the clipping bit
  set, a polygon boundary, and null definition and reactor.
- An IMAGEDEF is chapter 20.4.81, and an IMAGEDEF_REACTOR holds only its
  version, the image it serves being its owner, which DXF writes again as
  the subclass's
  330. A RASTERVARIABLES is chapter 20.4.91; a WIPEOUTVARIABLES holds its
  frame as one BS. The definition's reactors are its dictionary and the
  images' reactors.
- An underlay of any of the three kinds is the extrusion as 3BD, not BE, the
  position as 3BD, the rotation as a BD in radians before the scale, the
  scale as 3BD, the flags, contrast and fade as RCs, and the clip as a BL
  count of 2RD vertices; the definition is a hard pointer in the handles.
  AutoCAD keeps a clipping rectangle as its top left and bottom right
  corners, whatever two corners it was given.
- A PDF, DWF or DGN definition is two texts, the file and the part shown,
  owned by the ACAD_PDFDEFINITIONS, ACAD_DWFDEFINITIONS or
  ACAD_DGNDEFINITIONS dictionary. R14 keys the entry by the file's name in
  upper case with its dot an underscore, `LEVE_PDF`.
- An OLE2FRAME is typed from chapter 20.4.88 alone, under its fixed type 74:
  the version, from R2000 the mode, a BL count of data bytes, and from R2000
  a byte the specification leaves unnamed. DXF's description and corners lie
  inside the data. AutoCAD 2026 for Mac cannot insert one, so no AutoCAD file
  has confirmed the layout; frames with made-up data round-trip in every
  release and pass AutoCAD's audit.
- Across releases an image's inverted clip, which R2010 added, is cleared
  and noted below it, and an OLE frame's mode and trailing byte are dropped
  below R2000. A definition or reactor left out of a save is cleared from
  its image or underlay and noted. The audit leaves out an image or an
  underlay whose definition is not of its kind, as AutoCAD shows nothing
  without it, and clears an image's reactor that names no IMAGEDEF_REACTOR.

### Multileaders

MULTILEADER and MLEADERSTYLE are typed with the DXF reference's names:
`MultiLeaders::MLeader` with its `ContextData`, its leaders `LeaderRoot`,
their lines `LeaderLine` and breaks `LineBreak` and `DoglegBreak`, and the
style's `Leader`, `Text` and `Block`. AutoCAD stores both under their
classes in every release, R14 included, and writes its two default styles,
Standard and Annotative, into every file. Chapters 20.4.48, 20.4.86 and
20.4.87 hold, with these additions:

- The context data follows the multileader's version with no object context
  header: a `BL` count of leaders and each leader, then the content. Its
  text ends with a bit after the word break that DXF leaves out, 0 in every
  file; its block bit is stored only when the content is not text. A line's
  breaks are a `BL` count of segments, each its index, a `BL` count and that
  many start and end pairs, one DXF group 90 per pair.
- Up to R2007 a multileader stores two counts after its annotative bit, the
  arrowheads and the block's attribute values; from R2010 only the values,
  each line holding its own arrowhead. Every top attachment, the
  multileader's, its context's and its style's, comes before the bottom one.
- A style stores its annotative bit before the bit DXF's 295 gives; the
  files set the first for the Annotative style, which DXF writes with 296
  set. From R2013 a bit follows the attachments, DXF's 298.
- R2010 stores the version of both in their data, 2. Earlier releases keep
  it in `ACAD_MLEADERVER` extended data, a single 1070, which also says a
  style is of the new format that stores its text's angle and flush bit.
  The file names the application by its record alone, so the record is
  recognized by its shape. A save across R2010 moves the version between the
  two; a save to R2010 or later drops the arrowheads with a note.
- AutoCAD's R2018 samples register no `ACAD_MLEADERVER` application, and
  AutoCAD refuses a multileader style saved below R2010 without its version,
  as the old format. AutoCAD's own save registers the application under a
  new handle; so does a save of dwgio, under the handle seed, before it
  writes the objects.
- The audit checks the style, linetypes, text styles, block and arrowheads a
  multileader names, and those its style names, when they are not null. A
  missing style falls back to Standard, a missing arrowhead or style block
  to none, and a multileader of a missing block is left out.

### Tables

`ACAD_TABLE`, `TABLESTYLE` and `DATATABLE` are typed as `Entities::AcadTable`,
`Objects::TableStyle` and `Objects::DataTable`. AutoCAD stores the three under
their classes in every release, R14 included; a DWG names the data table
`DATATABLE` even where the R2000 DXF says `ACDBDATATABLE`, and from R2007
AutoCAD keeps none. `TABLECONTENT`, `TABLEGEOMETRY` and `CELLSTYLEMAP` are
typed as `Objects::TableContent`, `Objects::TableGeometry` and
`Objects::CellStyleMap`; from R2010 a table holds its content itself as
`Objects::TableContents::Data`. Beyond chapters 20.4.96 to 20.4.103:

- A data table is a short version, the column and row counts as `BL`, the
  name, and per column its type, its name and a cell per row: 1 a `BL`, 2 a
  `BD`, 6 a hard owner in the handle stream (the XRECORDs the table owns).
  The bits are the same from R14 to R2004. A column of another type keeps
  the object `Unknown`.
- A table style's colors are true colors (`CMC` as R2004 stores it) in every
  release, R14 included. Up to R2007 it is the specification's 20.4.101.1.
  From R2010 it is a version byte, the description, two longs (0 and 101,
  the number the user's next style takes), a null hard owner, the cell style
  of the table followed by its number 4, class 2 and name `Table`, then a
  count and for each row style its number, its cell style, its number again,
  its class and name, in the order title, header, data. A cell style's
  colors are `CMC`; a null pointer is a hard one in a cell style and a soft
  one in a cell's own content format.
- A table up to R2007 follows 20.4.96.1 with its extrusion a `3BD`, no owned
  count unless it has attributes, and its block and style as hard pointers
  before one hard pointer per cell. A cell's text is stored before R2007
  only. A cell given a text carries override bit 0x40000, which stores
  nothing. From R2007 a cell's value of type 4 is a `BL` byte count and the
  text in UTF-16 with its NUL, in the data, not a `TV`.
- From R2010 the soft pointer the specification leaves unknown is the block,
  a hard pointer, and the table style comes last in the handle stream. The
  R2010 bit is 1 and the R2013 long 0. Each cell stores its extents, a long
  of 3, or 7 from R2018, its width and height, and a pointer, soft when null
  and no geometry follows. The short before the direction is 6 or 22.
- The file names the anonymous block `*T`, which DXF numbers `*T1`.
- AutoCAD keeps beside every table, in every release R14 included, an
  XRECORD under `ACAD_XREC_ROUNDTRIP` in its extension dictionary: the
  marker `ACAD_ROUNDTRIP_2008_TABLE_ENTITY` (102), its TABLECONTENT as a hard
  owner (360), 70 2, 90 1, a point of zeros (10, 20, 30), 90 0, 90 5 and its
  TABLEGEOMETRY (361). Below R2007 the record continues with
  `ACAD_ROUNDTRIP_PRE2007_TABLE`, the row and column counts (90, 91), and
  `ACAD_ROUNDTRIP_PRE2007_TABLECELL` with the DATATABLE (360). The data
  table has a row per cell, row by row, and four columns: the cell's
  checksum, two longs of 0, and an XRECORD of the cell's value as R2007
  keeps it (93 the flags, 90 the type, 1 a string, 94, 300 and 302), 7 and 0
  for a cell with no value, the flags with 4 for a string.
- A TABLECONTENT is the content a table holds from R2010, the same in every
  release but for its text: before R2007 a value has no flags, unit, format
  or text, and a string is a `BL` byte count and the code page with its NUL.
  A content's own format names no text style with a hard pointer, where the
  table's names it with a soft one. Each cell carries the custom data item
  `ACAD_ROUNDTRIP_2008_CELL_CHECKSUM`, a double: the sum of each UTF-16 unit
  of the cell's text times its place counted from 1 ("M8" 189, "Parts"
  1655), 0 for a cell with no content; a cell a merged range covers holds
  none and 0. It has no extents: they are the TABLEGEOMETRY's, a count of
  rows, of columns and of cells, then per cell a long (the content's bits, 3
  or 7), its width and height, a soft pointer and its contents' geometry,
  each as a content's.
- Below R2010 a table style keeps its cell styles in a CELLSTYLEMAP under
  `ACAD_ROUNDTRIP_2008_TABLESTYLE_CELLSTYLEMAP` in its extension dictionary,
  each style followed by its number, class and name, exactly R2010's title,
  header and data styles; from R2010 the dictionary stays, empty. Below
  R2007 the dictionary also holds, under `ACAD_XREC_ROUNDTRIP`, an XRECORD of
  `ACAD_ROUNDTRIP_PRE2007_TABLESTYLE` and the data type, unit and format of
  each of its three cells (90, 91, 1; 92, 93, 2; 94, 95, 3).
- R14 names a dictionary entry in upper case, cut to 26 characters, and keeps
  the names it changed in an XRECORD of `EXTNAMES` under the dictionary's own
  extension dictionary: the map's entry is `ACAD_ROUNDTRIP_2008_TABLES` (see
  R14's names).

#### Tables across releases

AutoCAD reads a table from its TABLECONTENT and TABLEGEOMETRY, not from the
table itself. Its saves to another release hold the TABLECONTENT's content,
with the cells' checksums in their custom data and the TABLEGEOMETRY's
extents, whether it writes that content into the table (from R2010) or a
grid made of it (before). It carries the two objects unchanged, except for
the form of their values across R2007. It ignores a table's grid when they
are there: a grid whose text was changed in the file is saved to a newer
release with the content's text. A save of dwgio to another release
(`releases/table/convert.cpp` and `releases/table/content.cpp`) does the
following:

| What | How |
| :--- | :--- |
| A table read | the content of its TABLECONTENT, each cell given its TABLEGEOMETRY's extents. Read from a grid, a cell with no content gets an empty one as AutoCAD gives it (from R2007 a value of flags 3; before, an empty string of flags 2, or a general empty string where the content's value is empty, the extents set to zero but for the content's center), and a cell whose grid text no longer matches its checksum takes the grid's text, where AutoCAD keeps the content's |
| A table with no content or geometry | its content made from its grid as AutoCAD makes it (the first row the title, the second the header, a value's format setting its type, a merged range from each span), or kept from R2010, and the record, content and geometry made from it |
| The table written from R2010 | that content; from a grid, the flags without 16, the R2010 bit 1 and R2013's long 0, one part for every row; no table style named; R2010 drops bit 4 of a cell's extents |
| The table written before R2010 | a grid of that content: the widths and heights, a merged range's span and the cells it covers marked, each cell's first value (one of flags 3 where it has none), from R2007 as the value and before as its text, the text override 0x40000 on a string or a value with text; saved from R2010, the flags with 16 for a table that flows down |
| The TABLECONTENT and TABLEGEOMETRY | carried; across R2007 a string's bytes change form, and the flags, unit, format and text R2007 added are dropped in a save to an older release and left empty in a save to a newer release, as AutoCAD writes them |
| Below R2007 | the DATATABLE made from the content, with the record's second part; in a save to a newer release, both are removed |
| A table style | its two forms across R2010 as above; in a save to a newer release, with the cell styles of its CELLSTYLEMAP where it has one, user styles included; the CELLSTYLEMAP made in a save from R2010 to an older release and removed in a save to a newer release, its entry with it; the record of its cells' types made in a save to below R2007 and removed in a save to a newer release; saved from below R2010 to R2018, its long before the cell styles is 8 |

The result matches AutoCAD's own save to that release, handle for handle
once numbered in the order reached from the table, except for the table's
proxy graphics, which AutoCAD draws again, and, for saves from or to R14, the
name of the map's entry above. A table saved from R2007 to R2018 keeps the
text of every cell no merged range covers; a cell a range covers loses it,
as in AutoCAD's own save. A table holding what dwgio does not read (a block
cell, an override other than 0x40000, custom data of another type, a linked
cell, a field or break data) stays `Unknown`, and is left out of a save to
another release with a note.

### Helices, meshes, lights, sections, materials and visual styles

The specification does not describe HELIX, MESH, LIGHT, SECTIONOBJECT,
MATERIAL and VISUALSTYLE. AutoCAD keeps them under their six classes in
every release from R14. Each is typed in every release and writes back bit
for bit; a mesh's overrides alone are kept as bits.

- A HELIX is a SPLINE's data as chapter 20.4.40 lays it out, R2013's flags
  and knot parameter included, then BL major (33) and minor (377) versions,
  the axis's base, the start point and the axis as 3BD, the radius, the
  turns and the height of a turn as BD, a bit for the right hand, and an RC
  for what an edit keeps.
- A MESH is a BS version (2), a blend bit, a BL smoothing level, a BL count
  of vertices and their 3BD, a BL count of face numbers and the numbers as
  BL, a BL count of edges and two BL indices each, a BL count of creases and
  their BD, and a BL count of overridden parts, whose data is not known.
- A LIGHT is a BL version (1), the name, a BL kind, a bit for on, a CMC
  color, a bit for the plotted glyph, the intensity, the position and the
  target as 3BD, a BL fading, a bit for limits, the start and end distances,
  the hotspot and falloff angles in radians where DXF gives degrees, a bit
  for shadows, a BL shadow kind, a BS map size and an RC softness. R2000's
  DXF packs the color in a 90 group; the file keeps the CMC.
- A SECTIONOBJECT is a BL state and BL flags, the name, the vertical as 3BD,
  the top and bottom as BD, a BS transparency, the indicator's color, a BL
  count of vertices and their 3BD, the same for the back line, and its
  SECTION_SETTINGS as a hard owner. The color is a CMC as R2004 stores it,
  its long, in every release, R14 and R2000 included; R2000's DXF leaves it
  out.
- A MATERIAL is the name and description, then three shades (ambient,
  diffuse and specular) and six maps (diffuse, specular, reflection,
  opacity, bump and refraction), in the order: ambient, diffuse, the diffuse
  map, the specular shade and map, the gloss, the reflection map, the
  opacity and its map, the bump map, the index of refraction and its map. A
  shade is an RC method, a BD factor and, only when the method is 1, a BL
  color. A map is a BD blend, RC projection, tiling and automatic bits, 16
  BD of its transform row by row, an RC source and, only when the source is
  1, the file. From R2007 the translucence, the self illumination and the
  reflectivity as BD, the model and the channels as BL, and the mode as BS
  follow. DXF leaves out what is at its default.
- A VISUALSTYLE is the name and a BS kind, then its properties in one of
  three layouts. Up to R2007 they are 29 values, a 30th BD in R2007,
  followed by the internal bit, with no flags: `71`, `72`, `73` as BS, `40`
  and `41` as BD, `63` a CMC, `90` BL, `74` BS, `91` BL, `64` and `65` CMC,
  `75` BS, `42` BD, `92` BL, `66` CMC, `43` BD, `76` to `78` BS, `67` CMC,
  `79` BS, `170` an RC, `171` BS, `290` a bit, `174` and `175` BS, `93` and
  `44` BL and `173` BS, named by their DXF groups. DXF's 62 and 45 are not
  in the file except for R2007's last real, and an R2000 file's Realistic
  keeps 2 where DXF says 72 is 3. From R2010 a BS version (2 in every file,
  though R2013's DXF says 3) and the internal bit come after the kind. R2010
  has 28 properties in DXF's order, each followed by its flags as a BS;
  R2013 and R2018 have 58, the first 28 the same, typed as DXF's 90, 40, 62,
  290 and 1 say. A color of no method, 257 to R2000, is the long
  `0xC8000000` from R2004.
- AutoCAD writes ACAD's extended data `AcDbSavedByObjectVersion` and a 1070
  of 0 on every light and visual style below R2018. It refuses an R2000 file
  whose light lacks it, and an R2004, R2000 or R14 file whose visual style
  lacks it. An older AutoCAD wrote none in the R2007 files AutoCAD ships,
  such as the break line symbol, which AutoCAD opens as they are; a save
  below R2018 adds it wherever it is missing.

| What | To a newer release | To an older release |
| :--- | :--- | :--- |
| A helix's spline flags and knot parameter at R2013 | AutoCAD's, as a spline's | dropped |
| A light's color across R2004 | 257 as no method | a true color dropped and noted, the index kept |
| A material's R2007 fields | the defaults of AutoCAD's materials | dropped |
| A visual style | its properties in the release's layout, those it lacks read from its record (see below), AutoCAD's defaults where there is none | its layout, the other properties in a record as AutoCAD's save makes it, its true colors the nearest index below R2004 |
| A light's or visual style's marker | dropped in a save to R2018 | added in a save from it |
| A mesh with overrides | left out and noted | the same |
| A section's settings, an Unknown | carried: AutoCAD writes them the same in every release | the same |

The audit checks that an entity's own material and its visual styles, a
layer's material, and the visual style of a view, a viewport record, a
viewport entity and a layout name objects of their kind, and clears them
where they do not, the entity's material then its layer's.

### Visual and view styles across releases

AutoCAD keeps every property of R2013's visual style in memory, and the
display name and flags of R2018's view styles. A save to a release whose
layout lacks some writes them into an XRECORD under `ACAD_XREC_ROUNDTRIP` in
the style's extension dictionary, a hard owner of it, both made under new
handles. A save to a release that holds them reads them back and takes the
record away, and the dictionary with it when it holds nothing else. dwgio
does the same before it writes (`releases/roundtrip.cpp`), so a style
crosses every release both ways.

- R2010's 28 properties are R2013's first 28 in the same order. The older
  layout's are R2010's by DXF code; R2010 drops the short 174, which, like
  R2007's last real 45, is 0 in every file AutoCAD writes.
- The older layout writes the face opacity (40) and specular amount (41)
  negative where the face modifiers (90) leave bit 1, or bit 2, off; the
  line width (44) as a whole number, cut towards zero (7.6 as 7, -2.5 as
  -2); and a lighting quality (72) of 3 as 2. Below R2004 a color of no
  method is the index 257 and a true color the palette's nearest index.
- The record of R2010 holds, for each of the 30 properties beyond R2010's,
  the marker `RTVSPost2010Prop<n>` (group 102) and the value: a flag as a
  280, a whole number as a 90, a real as a 140, a text as a 1, or for a color
  the markers `RTVSPost2010Prop<n>ColorIndex` and
  `RTVSPost2010Prop<n>ColorRGB`, each followed by a 90: the index (the
  nearest one for a true color, 257 for none) and the true color's red,
  green and blue, or -16777216 for one that is not. Then comes
  `RTVSPost2010PropOp<n>` with the flags as a 70. Below R2010 the record
  continues with `RTVSPropertyOp<n>` and a 70 for each of R2013's 58
  properties, their flags, and five more, which AutoCAD sets to 1 for a
  style it read from a file and to 0 for one it made itself; dwgio keeps
  what a record held.
- A style without a record, as an older AutoCAD wrote the R2007 samples,
  takes AutoCAD's defaults: the flags 1, and for the 30 properties beyond
  R2010's: false, true, true, then six false, 50, 0.0, 1.0, 0, black, 50, 3,
  blue, false, 50, 50, 50, false, 50, by layer, 1.0, 2, `strokes_ogs.tif`,
  false, 1.0 and 1.0, the 2D wireframe's first and third the other way, and
  the flags of the by-layer color and the 1.0 after it 0.
- AutoCAD's palette holds red, yellow, green, cyan, blue, magenta, white,
  grey 128 and grey 192 as its indices 1 to 9, then for each of 24 hues 15
  degrees apart ten shades, full at 255, 204, 153, 127 and 76, each followed
  by its half-saturated one, then six greys from 51 to 255. The nearest
  index is the one at the least distance in red, green and blue, the first
  of those equally near; AutoCAD names true colors the same way.
- A view style opens with the model documentation part: a BS version, the
  description as a text, a bit, and from R2018 the display name as a text
  and the flags as a BL. Then a detail view style holds `BS BL CMC BD T BD
  RC CMC BD BL CMC CMC BD BL BD BL T BL CMC BL CMC RC`, and a section view
  style `BS BL CMC BD CMC BD T BD BL CMC BL CMC BD BD CMC BD BL BD BL T CMC
  CMC T BD BL B B BL BD BL BD`, a BL count and that many BD angles; the
  handles follow in the handle stream, the same in every release. The fields
  lie in another order than DXF's: the identifier's offset and placement
  before the arrow's color in a detail view style, the identifier's and the
  cutting plane's last values at the end in a section view style. AutoCAD
  writes the bit as 0 below R2018 and keeps it nowhere. A view style saved
  to a newer release without a record takes as its display name the name its
  dictionary lists it under, and no flags.

A value a release cannot hold is not restored by a save to a newer release,
in AutoCAD's saves as in dwgio's: a true color below R2004 stays its index,
a lighting quality of 3 below R2010 stays 2 and a line width its whole
number, and a view style's R2018 bit stays 0.

### Untyped objects across releases

An `Unknown` keeps its own bits apart from its header: the data after the
common data, the remainder of the handle stream, and from R2007 the string
stream. A save to its own release writes its bytes. A save to another
release writes the header in that release's form, as for a typed kind (the
type, the handle sizes, the string stream's frame from R2007, the data store
bit from R2013, an entity's common data), then the own bits, when AutoCAD
writes the class the same way in both releases, or differently in ways a
layout of its fields can convert. Otherwise the object is left out and the
save notes it.

A class whose own bits are the same in two releases in every object AutoCAD
writes has the same layout in both, and AutoCAD converts the same way in either
direction. The table is `Lineages` in `source/releases/untyped.cpp`.

| Classes | Their bits change at | Across those |
| :--- | :--- | :--- |
| ACAD_EVALUATION_GRAPH, ACDB_BLOCKREPRESENTATION_DATA, ACDB_DYNAMICBLOCKPROXYNODE, ACDB_DYNAMICBLOCKPURGEPREVENTER_VERSION, ACSH_HISTORY_CLASS, SECTION_MANAGER, SECTION_SETTINGS | nowhere | carried |
| The block elements of dynamic blocks: BLOCKGRIPLOCATIONCOMPONENT and the 26 BLOCK* parameters, grips and actions found in AutoCAD's samples; DIMASSOC | R2007 | converted by their layouts |
| ACDB_BLKREFOBJECTCONTEXTDATA_CLASS, ACDB_MTEXTATTRIBUTEOBJECTCONTEXTDATA_CLASS | R2010 | converted: the version |
| The shape history nodes: ACSH_BOOLEAN_CLASS, ACSH_BOX_CLASS, ACSH_CONE_CLASS, ACSH_CYLINDER_CLASS, ACSH_EXTRUSION_CLASS, ACSH_LOFT_CLASS, ACSH_PYRAMID_CLASS, ACSH_REVOLVE_CLASS, ACSH_SPHERE_CLASS, ACSH_SWEEP_CLASS, ACSH_TORUS_CLASS, ACSH_WEDGE_CLASS | R2004 | converted: the color |
| TABLECONTENT that holds what dwgio does not read; CELLSTYLEMAP | R2007; R2007 and R2010, gone from R2010 | left out |
| TABLEGEOMETRY | R2018 | left out |
| The constraint parameters, BLOCKUSERPARAMETER, BLOCKPROPERTIESTABLE and its grip, BLOCKPARAMDEPENDENCYBODY, ASSOCDIMDEPENDENCYBODY | R2007 | left out |
| ACDBASSOCVALUEDEPENDENCY | R2007 | converted by its layout |
| ACDBASSOCVARIABLE, ACDBASSOCEDGEACTIONPARAM, ACDBASSOCPATHACTIONPARAM and the surface action bodies | R2007 and R2013 | converted by their layouts across R2007 below R2013; left out across R2013 |
| ACDBASSOCGEOMDEPENDENCY, ACDBASSOC2DCONSTRAINTGROUP | R2007 and R2013 | left out |
| ACDBASSOCNETWORK, ACDBASSOCACTION, ACDBASSOCDEPENDENCY, ACDBASSOCPERSSUBENTMANAGER, ACDBPERSSUBENTMANAGER | R2013 | left out |
| ACDBDETAILVIEWSTYLE, ACDBSECTIONVIEWSTYLE | R2004, R2007 and R2018 | converted by their layouts, R2018's display name and flags moved to and from a record |
| BREAKDATA | R2004 and R2013 | converted by its layout |

About the classes converted:

- A layout is the list of the fields of chapter 2 in the class's data; each
  is copied unchanged, except those a release changes. A field read wrong
  does not reach the data's end, or leaves strings over, and the object is
  then left out, so a wrong layout cannot pass silently.
- A text is in the data as a T up to R2004, its count including the NUL that
  ends it, and in the string stream as a TU from R2007. AutoCAD writes no
  field for an empty description or label of a value set, though DXF lists
  one.
- Every block element opens with an evaluation expression: a BL of -1, the
  BL pair 33 and 377, its value as a BS type and what it says (-9999 none,
  40 a BD, 70 a BS), and its identifier; then its name, then a BL pair for
  the release it was saved in, 25 and 104 up to R2004 and 33 and 377 from
  R2007, and a BL. A grip adds two BL, a 3BD, a B and a BL. A parameter adds
  two B, then, for one point, a 3BD, two lists of BL and text and a BL; for
  two points it adds two 3BD, four such lists, four BL and a BS. An action
  adds a 3BD, the count of the handles it acts on and a list of BL. Each
  class's own fields follow as listed.
- The object context data hold the version chapter 20.4.89 gives as a BS
  first: 3 up to R2007, 4 from R2010. AutoCAD may compute two doubles of an
  MTEXT attribute's context again at an R2010 save; dwgio carries the values
  read.
- A shape history node opens with an evaluation expression and two BL, then
  holds a 4 by 4 matrix of sixteen BD, then a CMC, whose index alone up to
  R2000 becomes from R2004 the index 0, the long of its method and a byte of
  flags; what its shape holds follows, alike in every release. Below R2007
  the nodes of extrusions and sweeps carry ACAD's AcDbSavedByObjectVersion
  extended data, as the surfaces do, and AutoCAD refuses a file whose node
  lacks it.
- A 3DSOLID refers to its history from R2007. Below it AutoCAD keeps the
  reference in the solid's ACAD_XREC_ROUNDTRIP record: 102
  ACAD_SOLID_HIST, 360 the history, 102 ACAD_SOLID_HIST1, 90 the solid's
  R2007 long, 102 ACAD_SOLID_HIST2, 90 the solid's own handle.
- BREAKDATA: a BS, a handle to the object broken, a BL count, then for each
  break point a BL, from R2004 a bit AutoCAD sets and from R2013 a second it
  clears, two BS, a BL, two 3BD, a handle, up to R2000 a null hard owner
  handle more, and a BL count of BL triples each with a handle of the object
  that breaks.
- The associative network: a dependency opens with BL, BL, four B, BL, B,
  BL; a value dependency adds a BL, its name as text and its value as an
  evaluation expression's (a BS type, then a BD for 40, a BS for 70, a BL
  for 90). An action opens up to R2010 with four BL and a count of B, from
  R2013 with four BL more, which hold the action body's parameters AutoCAD
  moves into the action; a variable adds a BL, four texts (name,
  expression, evaluator, description), its value, two B and a count of BL.
  An edge action parameter is a BL, a name, three BL, a B, a BL and two 3BD;
  a path action parameter a BL, a name, two BL, a count and a BL. A surface
  action body holds its value parameters (each a BL, a name, a BL and a
  count of values) among BL, B and BS fields, a revolved one a B more.
- A constraint group lists its dependencies in no fixed order, which
  changes from one save to the next; its nodes follow, each its class's
  name as a text and its own data, so its texts cannot be found without the
  layout of every node class. A geometry dependency names the class of its
  persistent subentity as a text up to R2010 and otherwise from R2013.
- DIMASSOC: a BL mask of the points associated, a B, an RC, then for each
  point the name of its class as text, an RC for the object snap, a BL count
  of BS and BL pairs, two BD, a 3BD and a B.
- AutoCAD makes CELLSTYLEMAP, TABLECONTENT and TABLEGEOMETRY again at each
  save of a drawing it made, under new handles; compared by place, their bits
  are the same within the releases the table gives. All three are typed; the
  rows apply to an untyped one only (see Tables across releases).
- AutoCAD refuses a file holding part of an associative network (actions,
  dependencies and the network without their parameters and bodies), and
  its audit mends an evaluation graph whose nodes are gone (`Incorrect
  object node id`). So the network, the objects whose class starts with
  ACDBASSOC or ends with DEPENDENCYBODY, crosses whole or not at all. A
  dynamic block's evaluation graph crosses with everything it owns or not at
  all, as does a solid's history with its graph: AutoCAD refuses a file
  whose history owns a graph it lacks, and opens a solid without history.
  Each object removed with them is noted as left out with its network or graph.

### The file's own sections across releases

AutoCAD's saves, and so dwgio's, treat the file's sections as follows:

| Section | R14 and R2000 | R2004 to R2018 |
| :--- | :--- | :--- |
| The summary | an XRECORD named `DWGPROPS` in the named objects dictionary, made from the summary; read into the summary on opening and removed in a save to a newer release | `AcDb:SummaryInfo`, its texts in the code page up to R2004 and in UTF-16 from R2007 |
| The application information | none | made again at each save, in one form in every release |
| Its history | none | carried, in one form in every release |
| The file dependencies | none: left out and noted | carried in the release's form, though AutoCAD's console writes none |
| The macros (`AcDb:VBAProject`) | none: left out and noted | kept in the drawing's own release, left out and noted in another |
| The signature | none | left out of every save and noted |
| The security | none | kept in the drawing's own release, left out and noted in another |

- The summary is eight texts, each a short count of its characters followed
  by them and a NUL: the title, the subject, the author, the keywords, the
  comments, the login of whoever saved it last, the revision and the
  hyperlink base. Then come the editing time and the two dates, each a long
  of days and one of milliseconds, the dates as the header's TDCREATE and
  TDUPDATE; a short count of the custom properties, each a name and a value;
  and two zero longs. Up to R2004 the texts are in the code page; from R2007
  in UTF-16, the count then of UTF-16 units, as AutoCAD 2007's R2007 samples
  hold them.
- R14 and R2000 keep it in the `DWGPROPS` record: a 1 of `DWGPROPS COOKIE`,
  the texts as 2, 3, 4, 6, 7, 8 and 9 (comments before keywords), the first
  ten custom properties as 300 to 309, each `name=value` (`=` for none), the
  editing time and the dates as Julian days 40, 41 and 42, the hyperlink
  base as a 1 and the count of properties as a 90, each property past the
  tenth a 300 after it. AutoCAD reads all of them back, leaving out a
  property with no name, and the record is gone from the dictionary once the
  drawing is open. It writes the dates from the header, to the second in the
  record, the saver as the login, and the hyperlink base empty, its own
  being the header's HYPERLINKBASE. At each save AutoCAD sets the saver, the
  time of the change and the editing time. A drawing of R2004 and later may
  hold a record of that name, which AutoCAD keeps as any XRECORD and
  replaces in a save to an older release.
- The application information is a long of 3, the text `AppInfoDataList`, a
  long count of entries, and each entry's GUID and text in UTF-16. AutoCAD
  2026 writes the same bytes in every release. It gives the product's entry,
  and the comment that the file is a Trusted DWG last saved by an Autodesk
  application only when the drawing it opened was last saved by AutoCAD: a
  drawing whose information names dwgio it saves with the product's entry
  alone, and its history then without the comment. dwgio writes the
  product's entry alone, naming dwgio and its version, into every save of a
  paged release.
- The history opens with 32 bytes AutoCAD always writes the same, then the
  same layout: the version that made the drawing, the comment, the saver,
  the date, the product and its build, and the product's information.
  AutoCAD writes it again at each save; dwgio carries it, in one form in every
  release.
- The file dependencies are a long count of features, each a long count of
  bytes and the text with no NUL, then a long count of files, each four texts
  counted the same way (the full name, the path found, and the fingerprint
  and version GUIDs of an external reference), the feature's index, the time
  stamp and the size as longs, a short for whether it affects the graphics,
  and a long count of references. AutoCAD 2007's R2007 samples hold the
  texts in UTF-16; dwgio writes the code page's bytes below R2007, as the
  specification says. AutoCAD 2026's console writes none in any save, and
  opens and audits clean every release dwgio writes one into.
- AutoCAD opens a drawing with a VBA project or a signature section, both
  unknown to its console, and leaves them out of its saves to every release,
  its own included. The signature signs bytes no save keeps. dwgio keeps the
  macros and the security in the drawing's own release, where its saves give
  the file back.
- AutoCAD refuses a file whose template section is empty, though its own
  R2004 saves may describe one of four bytes and no pages, which reads as
  none: dwgio makes a new template there.

### R14's names

AutoCAD's R14 save names every dictionary's entries in upper case, turns
every character but a letter, a digit, `$`, `-`, `_` and `*` into an
underscore (one for a character of several bytes), and cuts a name of more
than 31 characters to 26. Each dictionary whose names it changed gets, in
its extension dictionary (made where it has none, a hard owner, no
reactors), an XRECORD under `ACAD_XREC_ROUNDTRIP` holding a 102 of
`EXTNAMES` and, for each name changed in the entries' order, a 1 of the name
it holds and a 2 of the name it was. It does the same for each table record,
the `EXTNAMES` group first in the record's round-trip XRECORD, before what
else R14 keeps there of it. Two dictionaries keep their names: the plot
style names, a dictionary with a default, and the one a dynamic block keeps
under `ACDBBLOCKTABLERECORDDATA` (its `Edit`). AutoCAD's R2000 and R2018
saves of an R14 file name every entry and record as the records say, and
keep no record of names, nor an extension dictionary left empty.

dwgio does the same with the dictionaries both ways: every dictionary's
record of names in its R14 saves matches AutoCAD's group for group, except
the root's, where AutoCAD names its own round-trip record of the auxiliary
header, which dwgio does not make. In a save to a newer release, dwgio reads
the records of the table records too, a block record's BLOCK taking its
name. In a save to an older release, it leaves the table records their
names, which AutoCAD's R14 opens and audits clean.

## Fixtures and checks

The fixtures under `tests/fixtures/acad` were written by AutoCAD 2026 from
the scripts in `tools/acad`, most saved to every release from R2018 to R14,
some also as DXF. [tests/fixtures/README.md](../tests/fixtures/README.md)
lists them and what each holds.

The tests check that:

- every object of each fixture reads under the handle its map gives it, and
  every fixture loads with no issue;
- every typed field agrees with the DXF twin, handles compared through the
  twin's names;
- each fixture saves to its own release byte for byte, or for a paged file
  every section byte for byte once decompressed (R2007 except its preview);
- each fixture saved to every other release loads back clean and holds what
  AutoCAD's own save to that release holds: every typed entity and object,
  every untyped object whose class crosses with AutoCAD's bits, each ACIS
  body byte for byte but for the date, each visual and view style with its
  properties, flags and record, and each table with its content and
  geometry;
- AutoCAD's sixteen English samples, where AutoCAD is installed, save to
  every other release with the untyped objects, styles and summary AutoCAD's
  own save keeps, and pass AutoCAD's audit; AutoCAD's own saves of them to
  each older release, saved to each newer one, keep every untyped object
  AutoCAD's save to the newer keeps; the Spanish dynamic block libraries keep
  their constraints' associative network across R2018.

`tools/acad/verify.py` opens a DWG in AutoCAD's console and runs `_.AUDIT
_Y`; it finds no errors in dwgio's saves of the fixtures and samples to
every release. The samples in the other languages, saved to every release
below both their own and R2010, also audit clean, a name in characters the
code page lacks written as an escape below R2007 (see Code pages). AutoCAD's
own save of its Japanese title block to R14 does not finish.

### What AutoCAD accepts

AutoCAD 2026, given drawings broken one way at a time:

- refuses a drawing whose handle seed is below its handles, one missing a
  table, a required record or the root dictionary, one with an unlisted or
  unnamed record or a name repeated in a table, an entity on a missing
  layer, linetype or text style, an insert of a missing block, a block chain
  that loops, or extended data of a missing application;
- fixes in its AUDIT a color out of range and a record listed twice in its
  table;
- silently keeps one of two objects under one handle;
- opens without change a drawing missing a named dictionary, with a dangling
  entry, missing a block's BLOCK, ENDBLK or layout, with a layout of a
  missing block, a wrong space code, an entity owned by nothing, an open or
  broken sequence, an orphan vertex, or a lineweight it does not know;
- refuses a paged file without a revision history, an object free space or
  an auxiliary header, or whose compressed pages depart from its own encoder
  in the ways listed under The paged file;
- silently opens a paged file whose block lists an entity it does not own,
  misses an entity or names one twice, and an R2000 file without a viewport
  entity header table;
- refuses an R2018 MTEXT whose frame has version 0 and a file whose view
  names a missing visual style, and repairs an MTEXT that names no text
  style.

`Audit` grades its findings by these.

## Differences from AutoCAD's saves

[Limits](concepts/limits.md) lists what dwgio does not do. At the level of
the format, its saves also differ from AutoCAD's in these ways:

- Text read below R2007 keeps as text an escape of four hex digits AutoCAD
  never writes, in lower case or of a character the code page holds, where
  AutoCAD reads it as the character: a save to R2007 or later writes the
  escape, which MTEXT and TEXT show as the character. AutoCAD's `\M+`
  escapes of East Asian characters are not read. A character beyond the
  basic plane becomes a question mark below R2007.
- The history of the application information is carried unchanged, where
  AutoCAD adds its own save to it. The file dependencies are carried as
  read, where AutoCAD's console writes none. A macro project or a security
  section is kept in the drawing's own release, where AutoCAD's console
  leaves the macros out. A drawing with no summary gets none, where AutoCAD
  writes one at every save.
- R2007's sections of encoding 1 are written with encoding 4, and a section
  whose name hash dwgio does not know is left out of an R2007 save and
  noted. The preview of an R2007 save gives its images' addresses as if its
  page were plain, which it is not.
- An ACIS body holding a record or value not seen in AutoCAD's files, such as
  another surface or curve, an attribute of another kind, a parameter curve
  on another surface or a spline of another subtype, cannot be converted to
  another ACIS release: its entity is left out of a save to a release of
  other ACIS, and noted. So is one saved from R2000 or R14 to a newer
  release whose edge parameters the rules above do not find, along a
  reversed edge or an intcurve's middle. The store's thumbnail and a NURBS
  surface's round-trip record are not made, and a procedural surface saved
  to a newer release keeps its GUID where AutoCAD makes a new one. A solid
  whose history holds a node of a class dwgio does not carry loses its
  history with its evaluation graph.
- A table keeps its proxy graphics as read, where AutoCAD draws them again
  in the release written. A table whose grid was changed since its content
  was stored is saved to a newer release with the grid's text, where
  AutoCAD's save takes the content's. A table holding a block cell, a field,
  a link, break data or an override other than the text's stays untyped and
  is left out of a save to another release.
- A dimension arrow block named in an R14 header that no block record bears
  stays null. A table record keeps its name in a save to R14, where
  AutoCAD's writes it in upper case with a record of the name it was; two
  dictionary entries whose names R14 holds alike stay alike. AutoCAD's other
  R14 round-trip records, of the header, the auxiliary header and what a
  table record holds beyond R14, are neither made nor read: they cross as
  any XRECORD.
