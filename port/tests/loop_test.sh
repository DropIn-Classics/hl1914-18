#!/bin/sh
# Builds port/tests/loop_test.c and runs it on the passes
# port/tests/loop_dumps.py logged and dumped (build/loop/TAG/, ignored).
# Without arguments: m8_site's (build/loop/m8_site/loop_test.args); with
# -replay alone the same, the computer's side replayed from the log.
set -e
cd "$(dirname "$0")/.."
CC=${CC:-cc}
RT=../doskit/runtime
mkdir -p build
$CC -O2 -Wall -Wextra -I$RT -Isrc -o build/loop_test tests/loop_test.c src/loop.c src/mouse.c src/computer.c src/plan.c src/orders.c src/side.c src/units.c src/range.c src/battle.c src/briefing.c tests/standalone.c src/pictures.c src/screen.c src/draw.c src/fightpics.c src/map.c src/files.c src/names.c src/fight.c src/move.c src/mem.c $RT/rmem.c $RT/vga.c $RT/sha256.c $RT/sys.c
if [ $# -gt 1 ]; then
    exec build/loop_test "$@"
fi
# shellcheck disable=SC2046
exec build/loop_test "$@" $(cat ../build/loop/m8_site/loop_test.args)
