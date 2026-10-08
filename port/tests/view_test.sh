#!/bin/sh
# Builds port/tests/view_test.c and runs it on pairs of dumps of
# the routines view_step calls (see there).  Without arguments: the calls
# in tests/view_test.args (made by tests/view_dumps.sh).
set -e
cd "$(dirname "$0")/.."
CC=${CC:-cc}
RT=../doskit/runtime
mkdir -p build
$CC -O2 -Wall -Wextra -I$RT -Isrc -o build/view_test tests/view_test.c src/side.c src/units.c src/range.c src/battle.c src/briefing.c tests/standalone.c src/pictures.c src/screen.c src/draw.c src/fightpics.c src/map.c src/files.c src/names.c src/fight.c src/move.c src/mem.c src/plan.c src/computer.c $RT/rmem.c $RT/vga.c $RT/sha256.c $RT/sys.c
if [ $# -gt 0 ]; then
    exec build/view_test "$@"
fi
# shellcheck disable=SC2046
exec build/view_test $(cat tests/view_test.args)
