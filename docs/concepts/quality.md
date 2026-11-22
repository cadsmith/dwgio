# Quality

dwgio is tested against files AutoCAD wrote, and every change must pass the gates below.

## Fixtures {#fixtures}

The files under `tests/fixtures/acad` were written by AutoCAD 2026 running the scripts under `tools/acad`; none is edited by hand or written by another program. Each script draws its subject, then saves it to each release from R2018 down to R14, but for a few saved to some releases only, and some also as DXF; [tests/fixtures/README.md](../../tests/fixtures/README.md) lists them. The tests compare what dwgio reads with the DXF twin: the handles, the layers, the header variables and the fields of every typed kind.

## What is verified {#verified}

- Every fixture loads with no issue in each of the seven releases and saves back to its own release: byte for byte in R14 and R2000, and from R2004 section for section once decompressed, but for the application information that names the program that saved the file. The object map and the second header's addresses are computed, not copied.
- Every fixture saves to every other release and loads back clean. `tools/acad/verify.py` opens dwgio's saves in AutoCAD's console and runs its audit, which finds no errors.
- AutoCAD's 121 sample drawings in ten languages (dynamic block libraries, title blocks, templates) load with no issue and save back to their release unchanged. The sixteen English ones save to every other release, load back clean, pass AutoCAD's audit, and keep what AutoCAD's own save to that release keeps. The samples belong to Autodesk and are not in the repository: their tests run where AutoCAD is installed and are skipped elsewhere.

## Gates {#gates}

| Gate | Preset | What it requires |
| :--- | :--- | :--- |
| Tests | `debug` | Every test passes; the public interface matches `tests/api/snapshot.txt` and every public header compiles alone |
| Coverage | `coverage` | 100% of regions, functions, lines and branches under `include/` and `source/` |
| clang-tidy | `tidy` | Every check on, every finding an error |
| Sanitizers | `sanitize` | The tests pass under ASan and UBSan |
| Package | `release` | dwgio installs, and a project outside the tree finds it with `find_package(dwgio)` and saves a drawing |
| Fuzzing | `fuzz` | Eight libFuzzer targets: a load and its save to the same release (`load`) and to another (`cross`), the bit codes (`bits`), both decompressors (`lz`, `lz21`), ACIS bodies (`acis`), the data store (`store`), and untyped objects carried across releases with the summary and the file dependencies (`recast`). Each input a target finds is added under `tests/fuzz/found` with a test that pins it |

## Reading damaged files {#damaged}

Every size, count and page number read from a file is checked before anything is allocated for it, and the fuzz targets above feed the reader damaged input. [Damaged files](/concepts/limits/#damaged) says what a load recovers from and when it fails.

## Performance {#performance}

`tools/measure` times a load and a save to each release. `tools/measure/large.scr` makes the test drawing: 100,000 entities (lines on twenty layers, circles, arcs, texts, polylines, and inserts of a block), saved by AutoCAD to each release. On an Apple M5, in the `release` preset, in seconds:

| | R14 | R2000 | R2004 | R2007 | R2010 | R2013 | R2018 |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| File | 5.7 MB | 5.2 MB | 1.6 MB | 2.1 MB | 2.0 MB | 1.6 MB | 1.6 MB |
| Load | 0.03 | 0.03 | 0.04 | 0.04 | 0.04 | 0.04 | 0.04 |
| Save to its release | 0.17 | 0.16 | 0.21 | 0.37 | 0.23 | 0.22 | 0.22 |
| Save to R14 | | 0.24 | 0.24 | 0.25 | 0.25 | 0.23 | 0.23 |
| Save to R2018 | 0.31 | 0.29 | 0.28 | 0.29 | 0.29 | 0.26 | |

A loaded drawing of 100,000 objects takes about 48 MB. Each object holds its kind in a box of its own, so it takes 16 bytes plus the size of its kind (360 bytes for a LINE, of which 200 are the common entity data) plus what it owns. A load peaks near 70 MB. A save to the drawing's release adds about 10 MB; a save to another release adds at most one copy of the drawing, shared by every step that changes it, so this drawing peaks near 145 MB. Every step of a save is linear in the number of objects: objects are found by handle through one index per pass.
