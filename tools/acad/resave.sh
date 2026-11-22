#!/bin/sh
# AutoCAD's own save of a fixture to other releases: tests/fixtures/acad/<stem>.dwg saved as
# <stem>-as-r<year>.dwg for each year given, or for each line of resaves.txt with no argument.
# macOS and AutoCAD 2026, through console.sh; the saves land in tests/fixtures/acad. A stem ending in
# -bare is the fixture without its tables' content, which dwgio_bare writes first: DWGIO_BARE names
# the program, build/debug/bin/dwgio_bare by default.
set -eu
here=$(cd "$(dirname "$0")" && pwd)
fixtures=$(cd "$here/../../tests/fixtures/acad" && pwd)
work=${TMPDIR:-/tmp}/dwgio-acad
mkdir -p "$work"

resave() {
  stem=$1
  shift
  script="$work/resave-$stem.scr"
  {
    printf 'FILEDIA 0\nCMDECHO 0\n'
    for year in "$@"; do
      if [ "$year" = 14 ]; then release=R14; suffix=r14; else release=$year; suffix=r$year; fi
      printf '_.SAVEAS _%s "%s-as-%s.dwg"\n' "$release" "$stem" "$suffix"
    done
    printf '_.QUIT _Y\n'
  } > "$script"
  drawing="$fixtures/$stem.dwg"
  case $stem in
    *-bare)
      drawing="$work/$stem.dwg"
      "${DWGIO_BARE:-$here/../../build/debug/bin/dwgio_bare}" "$fixtures/${stem%-bare}.dwg" "$drawing"
      ;;
  esac
  python3 "$here/locked.py" sh "$here/console.sh" "$script" "$drawing" < /dev/null
  for year in "$@"; do
    mv "$work/$stem-as-r$year.dwg" "$fixtures/"
  done
}

if [ $# -eq 0 ]; then
  while read -r line; do
    [ -n "$line" ] && resave $line
  done < "$here/resaves.txt"
else
  resave "$@"
fi
