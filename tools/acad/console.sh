#!/bin/sh
# Runs an AutoCAD script in AcCoreConsole, on a drawing when one is given: console.sh script.scr [drawing.dwg].
# macOS and AutoCAD 2026; DWGIO_ACAD names another AutoCAD application. The console runs in
# $TMPDIR/dwgio-acad, an isolated profile, and saves under relative names land there.
set -eu
app=${DWGIO_ACAD:-"/Applications/Autodesk/AutoCAD 2026/AutoCAD 2026.app"}
console="$app/Contents/Helpers/AcCoreConsole.app/Contents/MacOS/AcCoreConsole"
if [ ! -x "$console" ]; then
  echo "AcCoreConsole not found at $console; set DWGIO_ACAD to the AutoCAD application" >&2
  exit 1
fi

script=$(cd "$(dirname "$1")" && pwd)/$(basename "$1")
user=${TMPDIR:-/tmp}/dwgio-acad
mkdir -p "$user"
if [ $# -gt 1 ]; then
  drawing=$(cd "$(dirname "$2")" && pwd)/$(basename "$2")
  set -- /i "$drawing"
else
  set --
fi
cd "$user"
exec "$console" /isolate dwgio "$user" "$@" /s "$script" /l en-US
