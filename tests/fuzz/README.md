# Fuzzing

Eight libFuzzer targets, built by the `fuzz` preset with ASan and UBSan. The
preset needs LLVM's clang, which has libFuzzer; Apple Clang does not.

```sh
PATH=/opt/homebrew/opt/llvm/bin:$PATH cmake --preset fuzz
PATH=/opt/homebrew/opt/llvm/bin:$PATH cmake --build --preset fuzz
build/fuzz/bin/dwgio_fuzz_load -max_total_time=600 tests/fuzz/corpus/load
```

| Target | Input | Corpus |
| :--- | :--- | :--- |
| `load` | A drawing, loaded and saved to its own release | `corpus/load` |
| `cross` | Byte 0 the release to save to, then a drawing | `build/fuzz/corpus/cross` |
| `bits` | Bits read as every bit code in turn | none |
| `lz` | An R2004 page, decompressed and recompressed | none |
| `lz21` | An R2007 page, decompressed and recompressed | none |
| `acis` | Byte 0 the format and release, then an ACIS body | `corpus/acis` |
| `store` | A data store, R2013 and later, read and written back | `corpus/store` |
| `recast` | Bytes 0 to 2 the class, the releases and a split, then an untyped object's bits; the same bytes as a summary and file dependencies | `corpus/recast` |

`corpus/load` holds whole drawings of each release and, as `*-head.dwg`, the
first bytes of drawings, which reach the file header and the section locators
quickly. The `fuzz` preset writes the `cross` corpus from it: each drawing
behind the byte of R14, R2007 and R2018. A target without a corpus starts from
nothing. Pass a second, empty directory first to keep what a run finds out of
the corpus:

```sh
mkdir -p /tmp/cross
build/fuzz/bin/dwgio_fuzz_cross /tmp/cross build/fuzz/corpus/cross
```

An input that crashes a target goes in `found/`, its name ending in `-load` or
`-cross` when one of those targets found it, and with no such ending when the
`lz21` target did. `tests/dwgio/fuzz.cpp` runs each file there as its target
does, and a test of the code that failed pins the cause.
