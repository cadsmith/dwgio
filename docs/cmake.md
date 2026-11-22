# CMake Guidelines

## Rules

- No `#` comments are allowed in any CMake file.
- Lines must not exceed 80 columns.
- The library target is `dwgio`, its alias `dwgio::dwgio`; other targets use
  the `dwgio_` prefix.
- Libraries must be placed in `lib/` and executables in `bin/`.
- All targets must require C++23 (`cxx_std_23`).
- Configuration, building, and testing must use CMake presets.
- Build presets run 6 jobs.

## Presets

| Name | Type | Purpose |
| :--- | :--- | :--- |
| `debug` | `Debug` | Development with symbols and assertions |
| `release` | `Release` | Optimized build |
| `coverage` | `Debug` | Instrumented build for LLVM coverage |
| `sanitize` | `Debug` | ASan and UBSan instrumented build |
| `tidy` | `Debug` | The library built through clang-tidy |
| `fuzz` | `RelWithDebInfo` | libFuzzer targets, needs LLVM clang on the path |

## Options

| Option | Default | Effect |
| :--- | :--- | :--- |
| `DWGIO_INSTALL` | top level | Generates the install rules and the package config; off when dwgio is added with `add_subdirectory` or `FetchContent` |
| `DWGIO_ENABLE_CCACHE` | `ON` | Compiles through ccache when it is installed and dwgio is the top-level project |
| `DWGIO_ENABLE_COVERAGE` | `OFF` | Instruments for LLVM coverage and adds the `coverage` target, Clang only, which needs `llvm-profdata` and `llvm-cov` |
| `DWGIO_ENABLE_SANITIZE` | `OFF` | Instruments for ASan and UBSan, GCC or Clang |
| `DWGIO_ENABLE_TIDY` | `OFF` | Runs clang-tidy, `DWGIO_CLANG_TIDY` or the first on the path, on the library as it builds; the tests and tools are not checked |
| `DWGIO_ENABLE_FUZZ` | `OFF` | Builds the libFuzzer targets, LLVM Clang only |

The warning flags and the `bin/` and `lib/` output directories apply only when
dwgio is the top-level project. MSVC always compiles with `/utf-8`,
`/Zc:__cplusplus` and `/bigobj`.

## Targets

| Target | Alias | Type | Description |
| :--- | :--- | :--- | :--- |
| `dwgio` | `dwgio::dwgio` | Static or shared, as `BUILD_SHARED_LIBS` says | DWG format: bits, fields, the file, its objects, load and save |
| `dwgio_objects` | - | Object | The library's compiled sources, shared by `dwgio` and `dwgio_internal` |
| `dwgio_internal` | - | Static or interface | The library with its internal headers, for the tests and fuzz targets; a static archive of the same objects when `dwgio` is shared |
| `dwgio_tests` | - | Executable | Doctest test suite; CTest runs it once per test file as `dwgio/<path>`, the files with AutoCAD cases labelled `acad` |
| `dwgio_names` | - | Test | Checks the test case names with `tools/tests/names.py` |
| `dwgio_headers` | - | Object | Each public header compiled on its own, twice |
| `dwgio_api` | - | Test | Compares the public headers with `tests/api/snapshot.txt` |
| `dwgio_api_update` | - | Custom | Takes a new snapshot of the public interface |
| `coverage` | - | Custom | Runs the tests and gates lines, functions, regions and branches at 100% |
| `dwgio_package` | - | Test, Release configuration | Installs the library and builds a program against its package config |
| `dwgio_measure` | - | Executable | Times loading, saving and building drawings |
| `dwgio_bare` | - | Executable | Writes a drawing without its tables' content, for `tools/acad/resave.sh` |
| `dwgio_fuzz_<name>` | - | Executable, fuzz | One libFuzzer target each, see `tests/fuzz/README.md` |

## Commands

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
ctest --preset debug -R dwgio/save/
./build/debug/bin/dwgio_tests -tc="version*"
./build/debug/bin/dwgio_tests --source-file="*dwgio?save?own.cpp"

cmake --preset coverage
cmake --build --preset coverage --target coverage

cmake --preset tidy
cmake --build --preset tidy
```
