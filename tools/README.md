# Tools

| Folder | What it holds |
| :--- | :--- |
| `acad` | AutoCAD scripts that make the fixtures under `tests/fixtures/acad`, and the scripts that run AutoCAD: `console.sh` runs one script in AutoCAD's console, `locked.py` keeps one console at a time, `resave.sh` saves fixtures to other releases from `resaves.txt`, `verify.py` runs AUDIT on DWG files. macOS and AutoCAD 2026 only; see `tests/fixtures/README.md`. |
| `fixtures` | `dwgio_bare`, which writes a drawing without its tables' content, the input of the `-bare` resaves. |
| `format` | `style.py`, the formatter of the C++ sources and their comments; `--check` lists what it would change. |
| `measure` | `dwgio_measure`, which times loading, saving and building drawings, and `large.scr`, the AutoCAD script of its 100,000-entity benchmark drawing. |
| `pages` | `codepages.py`, which generates `source/encoding/pages.hpp` from Python's codecs; run it again only to add a code page. |
| `tests` | `names.py`, which checks the test case names; CTest runs it as `dwgio_names`. |

```sh
python3 -I tools/format/style.py --check
python3 -I tools/tests/names.py
python3 -I tools/pages/codepages.py source/encoding/pages.hpp
python3 -I tools/format/style.py source/encoding/pages.hpp
```

`dwgio_measure` and `dwgio_bare` build with the library's presets, into `build/<preset>/bin`.
