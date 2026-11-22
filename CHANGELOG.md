# Changelog

All notable changes to dwgio are documented in this file. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project follows
[Semantic Versioning](https://semver.org/spec/v2.0.0.html); before 1.0 a minor version may change the interface.

## [Unreleased]

## [0.1.0] - 2026-11-22

The first release.

### Added

- `Load` and `LoadFile` read DWG files of AutoCAD R14, R2000, R2004, R2007, R2010, R2013 and R2018 into a
  `Drawing`, with the issues found while reading. A damaged file loads as far as it can.
- `Save` and `SaveFile` write a drawing to any of those releases. A drawing saved to its own release gives back the
  same bytes in R14 and R2000 and the same sections, once decompressed, from R2004; a save to another release
  converts what both releases hold and notes what the target cannot hold.
- Typed table records, entities and named objects, among them polylines, hatches, dimensions, multileaders, tables,
  images, underlays, meshes, ACIS solids and surfaces, dictionaries, layouts, styles and materials. Every other object
  is kept with its handles and bytes, and crosses to another release where its layout allows.
- `Create` makes a new drawing that AutoCAD opens; `Edit` and `Edit::Editor` add entities, attributes, vertices,
  blocks and table records, rename records and remove objects with what they own.
- `View` reads a drawing through its tables, blocks, layouts and dictionaries, and `Index` finds objects by handle in
  constant time.
- `Audit::Check` finds what AutoCAD would refuse or repair, and `Audit::Repair` fixes it.
- `Geometry` converts object coordinates to world coordinates and places a block's entities through an insert.
- A CMake package with a static or shared library, and no dependency beyond the C++23 standard library.

[Limits](docs/concepts/limits.md) lists what this release does not do.

[Unreleased]: https://github.com/cadsmith/dwgio/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/cadsmith/dwgio/releases/tag/v0.1.0
