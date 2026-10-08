#!/bin/sh
# Builds port/tests/computer_test.c and runs it on pairs of dumps of the
# computer's player's routines (see there).  Without arguments: the calls
# in build/computer/computer_test.args and those of the planning phases
# (assess_test.args, queue_test.args, plan_test.args), which
# port/tests/computer_dumps.py writes.
set -e
cd "$(dirname "$0")/.."
CC=${CC:-cc}
RT=../doskit/runtime
mkdir -p build
$CC -O2 -Wall -Wextra -I$RT -Isrc -o build/computer_test tests/computer_test.c src/computer.c src/plan.c src/side.c src/units.c src/range.c src/battle.c src/briefing.c tests/standalone.c src/pictures.c src/sound.c src/screen.c src/draw.c src/fightpics.c src/map.c src/files.c src/names.c src/fight.c src/move.c src/mem.c $RT/rmem.c $RT/vga.c $RT/sha256.c $RT/sys.c
if [ $# -gt 0 ]; then
    exec build/computer_test "$@"
fi
A=../build/computer
LIST="$A/computer_test.args $A/assess_test.args $A/queue_test.args $A/plan_test.args"
for f in $A/turn_test.args $A/turns_test.args $A/late_test.args; do
    [ -f "$f" ] && LIST="$LIST $f"
done
# shellcheck disable=SC2086
exec build/computer_test -f $LIST
