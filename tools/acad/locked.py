#!/usr/bin/env python3
"""Runs a command while holding the lock that keeps one AutoCAD console at a time, as the consoles
share one isolated profile: `python3 tools/acad/locked.py sh tools/acad/console.sh script.scr`."""

import contextlib
import fcntl
import subprocess
import sys
import tempfile
from pathlib import Path

LOCK = Path(tempfile.gettempdir()) / "dwgio-acad.lock"


@contextlib.contextmanager
def held():
    """Holds the lock for the block's length."""
    with open(LOCK, "w", encoding="ascii") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        yield


if __name__ == "__main__":
    with held():
        sys.exit(subprocess.run(sys.argv[1:], check=False).returncode)
