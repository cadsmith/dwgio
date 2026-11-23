# Contributing to dwgio

Bug reports, drawings that dwgio reads or writes wrong, and pull requests are welcome. Report a security problem
privately, as [SECURITY.md](SECURITY.md) explains, and follow the [code of conduct](CODE_OF_CONDUCT.md).

## Reporting a bug

Open an issue with the release of the drawing, the program that wrote it, what dwgio printed (the `Failure` or the
`Issues`) and, when you can share it, the drawing itself or a smaller one that shows the problem. A drawing written by
AutoCAD is the most useful: dwgio is checked against AutoCAD's files, and its fixtures are all written by AutoCAD.

## Building and testing

dwgio needs CMake 3.28 and a C++23 compiler; [Installing dwgio](docs/guide/install.md#requirements) lists the
compilers. Configure, build and test with the presets:

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

[tests/README.md](tests/README.md) explains how the tests, the fixtures and the DXF twins are organised, how to run
one file or one case, and the `DWGIO_ACAD_VERIFY` and `DWGIO_ACAD` variables that run AutoCAD's own audit where
AutoCAD is installed. [tools/README.md](tools/README.md) describes the scripts that make the fixtures.

## Checks

A change must pass the checks CI runs: the tests on Linux, macOS, Windows and FreeBSD, static and shared, and the format and
test name checks.

```sh
python3 -I tools/format/style.py --check
python3 -I tools/tests/names.py
```

`python3 -I tools/format/style.py` formats the C++ files in place. A change to the library must also keep the gates
[Quality](docs/concepts/quality.md#gates) lists: 100% coverage (`coverage` preset), no clang-tidy finding (`tidy`),
clean sanitizers (`sanitize`), the package test (`release`) and the fuzz targets (`fuzz`, see
[tests/fuzz/README.md](tests/fuzz/README.md)).

A change to the public interface updates the snapshot the `dwgio_api` test compares the headers with:

```sh
cmake --build --preset debug --target dwgio_api_update
```

A change to the documentation under `docs/` runs its examples, which needs the `release` build of the library:

```sh
cmake --preset release
cmake --build --preset release --target dwgio
cd site && npm install && npm run examples
```

## Code style

- [C++ guidelines](docs/cpp.md): naming, file layout and what a comment may say.
- [CMake guidelines](docs/cmake.md): presets, targets and options.
- [Commit guidelines](docs/commit.md): Conventional Commits, with the directory as scope, such as
  `fix(source/releases): keep a table style's flags below R2010`.

Keep a pull request to one subject, with its tests, and describe what it changes and how it was checked.
