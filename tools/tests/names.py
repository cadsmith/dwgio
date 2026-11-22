#!/usr/bin/env python3
"""Checks the names of the test cases under tests/dwgio.

    python3 -I tools/tests/names.py

A name opens with the path of its file under tests/dwgio, without .cpp, then a colon: the cases of
tests/dwgio/releases/acis.cpp are "releases/acis: ...". Names are unique, hold no comma, which
doctest's -tc filter would split on, and run to 100 characters at most. A file that queues drawings
for AutoCAD's AUDIT closes with the case that runs it. Each breach is listed and the exit status is 1.
"""

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2] / "tests" / "dwgio"
CASE = re.compile(r'TEST_CASE\(\s*((?:"(?:[^"\\]|\\.)*"\s*)+)')
PIECE = re.compile(r'"((?:[^"\\]|\\.)*)"')
LIMIT = 100


def names(text):
    for match in CASE.finditer(text):
        line = text.count("\n", 0, match.start()) + 1
        yield line, "".join(PIECE.findall(match.group(1)))


def main():
    breaches = []
    seen = {}
    for path in sorted(ROOT.rglob("*.cpp")):
        unit = path.relative_to(ROOT).with_suffix("").as_posix()
        text = path.read_text(encoding="utf-8")
        where = path.relative_to(ROOT.parents[1]).as_posix()
        for line, name in names(text):
            spot = f"{where}:{line}"
            if not name.startswith(unit + ": "):
                breaches.append(f"{spot}: does not open with '{unit}: ': {name}")
            if "," in name:
                breaches.append(f"{spot}: holds a comma: {name}")
            if len(name) > LIMIT:
                breaches.append(f"{spot}: {len(name)} characters: {name}")
            if name in seen:
                breaches.append(f"{spot}: same name as {seen[name]}: {name}")
            seen.setdefault(name, spot)
        if "QueueAudit(" in text or "CheckSaveAs(" in text:
            last = list(names(text))[-1:]
            if "CheckQueuedAudits();" not in text or not last or "AUDIT" not in last[0][1]:
                breaches.append(f"{where}: queues drawings for AUDIT but does not close with the case that runs it")
    for breach in breaches:
        print(breach)
    return 1 if breaches else 0


if __name__ == "__main__":
    sys.exit(main())
