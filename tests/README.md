# Tests

| Folder | What it holds |
| :--- | :--- |
| `dwgio` | The doctest suite, `dwgio_tests`. Its folders follow `source/`: `bits`, `fields`, `file`, `objects`, `entities`, `tables`, `releases` and so on; the files at the top test the public functions: `load.cpp`, `create.cpp`, `edit.cpp`, `view.cpp`, the `save/` and `remove/` folders. |
| `dwgio/support` | The headers the tests share: fixture loading (`fixtures.hpp`), DXF twins (`twin.hpp`, `entities.hpp`, `objects.hpp`), AutoCAD (`acad.hpp`), own-release and cross-release saves (`saves.hpp`). |
| `fixtures` | The drawings AutoCAD wrote, and how they were made: see `fixtures/README.md`. |
| `api` | The snapshot of the public interface. |
| `package` | A program built against the installed package. |
| `fuzz` | The libFuzzer targets, their corpora and the inputs they found: see `fuzz/README.md`. |

## Running them

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

CTest runs `dwgio_tests` once per test file, as `dwgio/<path>`: `dwgio/save/own` runs
`tests/dwgio/save/own.cpp`. Select by file or by case:

```sh
ctest --preset debug -R dwgio/releases/
./build/debug/bin/dwgio_tests --source-file="*dwgio?save?own.cpp"
./build/debug/bin/dwgio_tests -tc="releases/acis: *"
```

## Names

A case's name opens with its file's path under `tests/dwgio`, without `.cpp`, then a colon and what it
checks in the present tense: `releases/table/convert: a table style gets a dictionary for its map only
where it has none`. No commas, since doctest's `-tc` splits on them, and 100 characters at most.
`python3 -I tools/tests/names.py` checks the rule; CTest runs it as `dwgio_names`.

## Fixtures and DXF twins

Most tests read a drawing from `tests/fixtures/acad`, written by AutoCAD from a script in `tools/acad`.
Most scripts save their drawing to every release, `<script>-r14.dwg` to `<script>-r2018.dwg`, and many as
DXF, `<script>-r2000.dxf`: the DWG's twin; [fixtures/README.md](fixtures/README.md) lists which. A test reads the DWG with dwgio and the DXF with `support/twin.hpp`,
a plain reader of code and value pairs, and compares the two: a field dwgio reads wrong differs from
what AutoCAD wrote in the DXF. `LoadFixture` and `LoadDrawing` in `support/fixtures.hpp` load a
fixture and stop the case if it does not load.

To add a fixture, write its script, run it with `tools/acad/make.sh`, and list it in
`fixtures/README.md`.

## AutoCAD

Some cases ask AutoCAD itself, on macOS with AutoCAD 2026 installed:

- `DWGIO_ACAD_VERIFY` set in the environment runs AutoCAD's AUDIT, through `tools/acad/verify.py`, on
  what the cases saved. Each file that saves drawings for it closes with a case named "... pass
  AutoCAD's AUDIT"; without the variable that case is reported as skipped. A few cases also compare
  with AutoCAD's own save of what dwgio wrote.
- `DWGIO_ACAD` names the AutoCAD application when it is not at
  `/Applications/Autodesk/AutoCAD 2026/AutoCAD 2026.app`.
- `DWGIO_TABLE_ORACLE` names a folder of AutoCAD's saves of the `table` and `cells` fixtures from each
  release from R2000 to every other, named `<fixture>-r<from>-as-r<to>.dwg` as `tools/acad/resave.sh`
  names them; `releases/table/roundtrip.cpp` compares dwgio's saves with them, and without the variable
  that case is reported as skipped.
- `releases/samples.cpp` reads the sample drawings inside the application, which are not in the
  repository; its cases are skipped when the application is missing.

CTest labels the files with such cases `acad`: `ctest --preset debug -L acad`. AutoCAD runs one console
at a time, so these are slow.

```sh
DWGIO_ACAD_VERIFY=1 ctest --preset debug -L acad
```

## Public interface

`tests/api/snapshot.txt` is every public header with its comments stripped. The `dwgio_api` test fails
when the headers no longer match it; after an intended change, take a new snapshot and commit it:

```sh
cmake --build --preset debug --target dwgio_api_update
```

## Coverage and format

The `coverage` preset instruments the library and its target gates lines, functions, regions and
branches of `include/` and `source/` at 100%:

```sh
cmake --preset coverage
cmake --build --preset coverage --target coverage
```

`python3 -I tools/format/style.py --check` lists the C++ files whose code or comments the formatter
would change.
