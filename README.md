# dwgio

[![CI](https://github.com/cadsmith/dwgio/actions/workflows/ci.yml/badge.svg)](https://github.com/cadsmith/dwgio/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](docs/guide/install.md#requirements)

dwgio is a C++23 library that reads and writes AutoCAD DWG files.

- It reads and writes every release from R14 to R2018: R14, R2000, R2004, R2007, R2010, R2013 and R2018.
- A drawing saved to its own release gives back the same bytes in R14 and R2000, and the same sections, once
  decompressed, from R2004.
- A drawing saved to another release is converted as AutoCAD's own save converts it, and passes AutoCAD's audit.
- It depends on nothing but the C++23 standard library, and is MIT licensed.

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

Documentation: [getting started](docs/guide/start.md), the [tutorials](docs/tutorials/reading.md), the
[entities](docs/entities/curves.md) and the [concepts](docs/concepts/drawing.md). The site under `site/` renders
them with the reference generated from the headers.

## Status

Version 0.1.0. The interface may still change before 1.0. A command line tool that converts between DWG and DXF is
planned.

| Supported | Not supported |
| :--- | :--- |
| Reading and writing R14, R2000, R2004, R2007, R2010, R2013 and R2018 | DXF |
| A save to the drawing's own release: the same bytes in R14 and R2000, the same sections from R2004 | R13 and earlier, and releases after R2018 |
| A save to any other release, with what the target cannot hold noted as an issue | Files encrypted with a password |
| Typed table records, entities and named objects; any other object kept with its handles and bytes | Rendering, plotting and geometry computation |
| `Create`, `Edit`, `View` and `Audit`: make, change, walk and repair a drawing | The content of FIELD, MPOLYGON and proxy objects, which is kept as bytes |

[Limits](docs/concepts/limits.md) gives the details, and [Compatibility](docs/concepts/compatibility.md) what a
save to another release converts and drops.

## Use in a project

```cmake
include(FetchContent)
FetchContent_Declare(dwgio
  GIT_REPOSITORY https://github.com/cadsmith/dwgio.git
  GIT_TAG v0.1.0
)
FetchContent_MakeAvailable(dwgio)

target_link_libraries(app PRIVATE dwgio::dwgio)
```

`add_subdirectory` and `find_package` work too. [Installing dwgio](docs/guide/install.md) gives the compilers it
needs and how to build it as a shared library.

## Build and test

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

[CONTRIBUTING.md](CONTRIBUTING.md) lists the other presets and the checks a change must pass, and
[tests/README.md](tests/README.md) how the tests and fixtures are organised.

## Layout

| Directory | Holds |
| :--- | :--- |
| `include/dwgio` | The public headers, namespace `DwgIO`; `dwgio/dwgio.hpp` includes all of them |
| `source` | The implementation: bit codes, field layouts, the file's sections, objects, header variables and classes |
| `tests` | Unit tests, the drawings AutoCAD wrote for them, the interface snapshot and the fuzz targets |
| `tools` | The AutoCAD scripts that make the fixtures, `dwgio_bare`, the code page generator, the formatter, the test name check and a timing tool |
| `vendor` | Where doctest is fetched from |
| `docs` | The documentation pages, the [DWG format notes](docs/dwg.md) and the contributor guidelines |
| `site` | The documentation site that renders `docs` |

## Acknowledgements

The format follows the *Open Design Specification for .dwg files* (Open Design Alliance), checked against the files
AutoCAD 2026 writes. AutoCAD and DWG are trademarks of Autodesk, Inc.; dwgio is not affiliated with or endorsed by
Autodesk or the Open Design Alliance.

The site bundles the Inter and JetBrains Mono fonts under the SIL Open Font License and GitHub's Octicons mark under
the MIT License; the tests use doctest, under the MIT License.

## License

[MIT](LICENSE)
