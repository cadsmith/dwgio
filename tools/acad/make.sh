#!/bin/sh
# Makes the fixtures of one script: make.sh <name>, for tools/acad/<name>.scr. The script runs in
# AutoCAD's console (console.sh: macOS, AutoCAD 2026) in $TMPDIR/dwgio-acad, and the files it saves,
# <name>-*.dwg and <name>-*.dxf, move to tests/fixtures/acad. When <name>-saves.scr exists, it then
# runs on the <name>-r2018.dxf the first wrote, for what the console can only set in a DXF.
set -eu
here=$(cd "$(dirname "$0")" && pwd)
fixtures=$(cd "$here/../../tests/fixtures/acad" && pwd)
work=${TMPDIR:-/tmp}/dwgio-acad
name=$1
mkdir -p "$work"
rm -f "$work/$name"-*.dwg "$work/$name"-*.dxf
cp "$here/$name.scr" "$here"/*.png "$work/"
python3 "$here/locked.py" sh "$here/console.sh" "$work/$name.scr" < /dev/null
if [ -e "$here/$name-saves.scr" ]; then
  cp "$here/$name-saves.scr" "$work/"
  python3 "$here/locked.py" sh "$here/console.sh" "$work/$name-saves.scr" "$work/$name-r2018.dxf" < /dev/null
  rm -f "$work/$name"-*.dxf
fi
for made in "$work/$name"-*.dwg "$work/$name"-*.dxf; do
  if [ -e "$made" ]; then
    mv "$made" "$fixtures/"
  fi
done
