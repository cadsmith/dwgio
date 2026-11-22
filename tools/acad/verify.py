#!/usr/bin/env python3
"""Runs AutoCAD's AUDIT on DWG files through console.sh and passes those it finds nothing wrong in.

    python3 tools/acad/verify.py <file-or-folder>... [--timeout SECONDS]

A file passes when AUDIT reports 0 errors found, 0 fixed and 0 objects erased. One console runs at a
time, under the lock of locked.py. The console sometimes aborts during startup, before it opens the
drawing; such a run is tried again rather than taken as the drawing's fault.
"""

import argparse
import os
import re
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
from locked import held  # pylint: disable=wrong-import-position

ROOT = Path(__file__).resolve().parents[2]
CONSOLE = ROOT / "tools" / "acad" / "console.sh"
ATTEMPTS = 3

SCRIPT_TEMPLATE = """FILEDIA 0
CMDECHO 0
_.AUDIT _Y
_.QUIT _Y
"""

ERRORS_RE = re.compile(r"Total errors found\s+(\d+)\s+fixed\s+(\d+)")
ERASED_RE = re.compile(r"Erased\s+(\d+)\s+objects")
DISCARDED_RE = re.compile(
    r"(Invalid or incomplete DXF input|Drawing file is not valid|drawing discarded)"
)


@dataclass
class Audit:
    """What AUDIT said of one file; the counts are None when it said nothing."""

    passed: bool
    status: str
    found: int | None = None
    fixed: int | None = None


def run_console(script: str, drawing: Path, timeout: float) -> tuple[int, str]:
    """Runs the console alone, again when it dies before it audits."""
    with held():
        for _ in range(ATTEMPTS):
            try:
                run = subprocess.run(
                    ["sh", str(CONSOLE), script, str(drawing)],
                    capture_output=True,
                    text=True,
                    timeout=timeout,
                )
            except subprocess.TimeoutExpired:
                continue
            output = run.stdout + run.stderr
            if run.returncode >= 0 or ERRORS_RE.search(output):
                return run.returncode, output
    return -1, "AutoCAD's console crashed or hung on every attempt"


def audit_dwg(drawing: Path, timeout: float) -> Audit:
    """Runs AUDIT on one DWG file."""
    if not CONSOLE.exists():
        return Audit(False, "console.sh not found")

    with tempfile.NamedTemporaryFile("w", suffix=".scr", delete=False) as file:
        file.write(SCRIPT_TEMPLATE)
        script = file.name

    try:
        returncode, output = run_console(script, drawing, timeout)
    finally:
        os.remove(script)

    if DISCARDED_RE.search(output):
        return Audit(False, "rejected")
    if returncode < 0:
        return Audit(False, "crashed")
    errors = ERRORS_RE.search(output)
    erasures = ERASED_RE.search(output)
    if not errors or not erasures:
        return Audit(False, "no summary")

    found = int(errors.group(1))
    fixed = int(errors.group(2))
    erased = int(erasures.group(1))
    passed = returncode == 0 and found == 0 and fixed == 0 and erased == 0
    return Audit(passed, "passed" if passed else "failed", found, fixed)


def main() -> int:
    parser = argparse.ArgumentParser(description="Runs AutoCAD's AUDIT on DWG files.")
    parser.add_argument("paths", nargs="+", type=Path, help="DWG files, or folders to search")
    parser.add_argument("--timeout", type=float, default=60, help="seconds per console run")
    args = parser.parse_args()

    targets: list[Path] = []
    for path in args.paths:
        if path.is_dir():
            targets.extend(sorted(path.glob("**/*.dwg")))
        elif path.is_file():
            targets.append(path)

    if not targets:
        print("No DWG files found.")
        return 1

    print(f"Verifying {len(targets)} DWG file(s) with AutoCAD...")
    print("-" * 75)
    print(f"{'File':<40} | {'Status':<8} | {'Errors':<6} | {'Fixed':<6}")
    print("-" * 75)

    failures = 0
    for target in targets:
        audit = audit_dwg(target, args.timeout)
        if not audit.passed:
            failures += 1
        name = target.name if len(target.name) <= 40 else target.name[:37] + "..."
        found = "N/A" if audit.found is None else str(audit.found)
        fixed = "N/A" if audit.fixed is None else str(audit.fixed)
        print(f"{name:<40} | {audit.status:<8} | {found:<6} | {fixed:<6}")

    print("-" * 75)
    print(f"Summary: {len(targets) - failures} passed, {failures} failed.")
    return 1 if failures > 0 else 0


if __name__ == "__main__":
    sys.exit(main())
