#!/bin/sh
# Builds port/tests/side_test.c and runs it on pairs of dumps of
# build_list and the repairing (see there).  Without arguments: the calls
# in tests/side_test.args (made by tests/side_dumps.sh).
set -e
cd "$(dirname "$0")/.."
CC=${CC:-cc}
RT=../doskit/runtime
mkdir -p build
$CC -O2 -Wall -Wextra -I$RT -Isrc -o build/side_test tests/side_test.c src/side.c src/units.c src/range.c src/battle.c src/briefing.c tests/standalone.c tests/orders_hook.c src/pictures.c src/sound.c src/screen.c src/draw.c src/fightpics.c src/map.c src/files.c src/names.c src/fight.c src/move.c src/mem.c src/plan.c src/computer.c $RT/rmem.c $RT/vga.c $RT/sha256.c $RT/sys.c
if [ $# -gt 0 ]; then
    exec build/side_test "$@"
fi
# shellcheck disable=SC2046
exec build/side_test $(cat tests/side_test.args)
