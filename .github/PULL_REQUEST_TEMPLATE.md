## What this changes

## How it was checked

- [ ] `ctest --preset debug` passes, with tests for the change
- [ ] `python3 -I tools/format/style.py --check` and `python3 -I tools/tests/names.py` pass
- [ ] `tests/api/snapshot.txt` updated if the public interface changed
- [ ] `npm run examples` in `site/` run if a page under `docs/` changed
- [ ] Commits follow [the commit guidelines](../docs/commit.md)
