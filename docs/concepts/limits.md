# Limits

What dwgio is safe to share between threads, what it costs, what it does not do, and how it treats a damaged file.

## Threads {#threads}

dwgio holds no state of its own between calls but two: the store of names that every `Symbol` points into, which a mutex guards, and the code page tables, built once the first time a thread needs one and only read after. A `Drawing` is plain data, with no cache inside and nothing a const function changes. So the rules are those of the standard containers:

- Separate drawings can be loaded, edited, checked and saved on separate threads at the same time.
- One drawing can be read from several threads at once: `View`, `Find`, `Audit::Check` and `Save` take it as `const` and change nothing in it. `Save` makes its own copy where the target release needs the drawing changed.
- Anything that changes a drawing needs it to itself: `Edit`, `Audit::Repair`, and a change you make to its objects or header. No other thread may read it meanwhile, and a pointer the view gave before is [invalid afterwards](/guide/objects/#handles).

## Memory and speed {#performance}

A loaded drawing takes about 480 bytes per object, and a save to another release holds at most one copy of it besides yours. [Quality](/concepts/quality/#performance) gives the times and the peaks for a drawing of 100,000 entities: a load in a few hundredths of a second, a save in a few tenths.

- Each distinct name read or given, a layer's or a class's, joins the store of names once and stays there until the program ends. A program that loads many drawings keeps the names of all of them, which is small beside the drawings.
- `Find` walks `Drawing::Objects` until it meets the handle, and each `View` function given the drawing indexes it again. To look up many handles, build an `Index` of the drawing once and pass it to `Find` and `View`, as long as nothing is added or erased meanwhile; an `Edit::Editor` keeps its own and finds through it.
- The functions of `Edit` walk the objects as well, for the handle seed, the block and the names, so adding entities one at a time through them takes time growing with the square of their number: 100,000 lines added to a new drawing take 14 to 23 seconds, and 10,000 inserts each with an attribute take 20 to 25 seconds. An `Edit::Editor` makes the same additions through an index it keeps, each in a time that does not grow with the drawing: the 100,000 lines take about a hundredth of a second, and the inserts a few thousandths. `dwgio_measure --build` measures both. [Many entities](/tutorials/creating/#many) shows it.
- `Edit::Remove` and `Edit::Rename` walk the drawing a few times each, and an editor indexes the drawing again after one.

## What dwgio does not do {#not}

- It draws nothing: no rendering, no plotting, no preview made. The preview a file holds is kept unchanged, in `Drawing::Preview`.
- It has no geometry kernel. An ACIS body is kept as SAT or SAB data and converted between the releases' forms, but dwgio computes no volume, area, mesh or intersection: see [ACIS bodies and surfaces](/entities/others/#acis).
- It reads and writes DWG only. DXF is neither read nor written.
- It reads R14 to R2018. A file of an earlier release, R13 included, or of a release after R2018 fails to load, as does a file encrypted with a password; [Errors and issues](/guide/errors/#failure) lists every cause of a failed load.
- It types the table records, the entities and the named objects AutoCAD writes most. Every other object loads as an `Unknown`, its header read and its own data kept as bytes: FIELD, MPOLYGON, proxy entities, the dynamic block objects, the associative network, the solid histories, the view styles, an R2018 multiline attribute. See [the object model](/guide/objects/#unknown), [fields](/entities/text/#fields) and [MPOLYGON](/entities/hatches/#mpolygon).
- OLE2FRAME is typed from the Open Design Specification for .dwg files alone: no sample drawing holds one, and the reading is untested against AutoCAD's files. See [OLE frames](/entities/others/#ole).
- `MEASUREMENT`, imperial or metric, is not a header variable in a DWG file: it is a block of its own, kept as bytes in `Drawing::Kept`. [Units](/tutorials/coordinates/#units) reads it.
- An R14 drawing's paper space viewports keep their view in `ACAD`'s `MVIEW` extended data, as R14 does; their R2000 fields stay at their defaults until a save to R2000 or later reads the view into them. See [R14](/tutorials/spaces/#r14).
- An extended data record with no groups cannot be written, since a size of zero ends the extended data: a save leaves it out and notes it.
- A save to another release converts what both releases hold and drops what the target lacks. [Compatibility](/concepts/compatibility/) lists it step by step.

## Damaged files {#damaged}

A load notes what it can recover from and carries on: a CRC mismatch, a wrong sentinel, a block past the end of the file, bytes no block claims, a missing object map, after which it walks the objects. It fails only where it cannot go on, for the causes [Errors and issues](/guide/errors/#failure) lists. Every size, count and page number read from a file is checked before anything is allocated for it, and the reader is fuzzed, as [Quality](/concepts/quality/#gates) describes.

A drawing loaded with issues saves unchanged, damaged objects included. Run [`Audit::Check`](/tutorials/auditing/) before saving one: it finds what AutoCAD would complain about, which the reader does not look for. [Errors and issues](/guide/errors/) shows a damaged load and how to report what it says.
