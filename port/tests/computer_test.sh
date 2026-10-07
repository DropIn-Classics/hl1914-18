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
$CC -O2 -Wall -Wextra -I$RT -Isrc -o build/computer_test tests/computer_test.c src/computer.c src/plan.c src/side.c src/units.c src/range.c src/battle.c src/map.c src/files.c src/names.c src/fight.c src/move.c src/mem.c $RT/rmem.c $RT/sha256.c $RT/sys.c
if [ $# -gt 0 ]; then
    exec build/computer_test "$@"
fi
# shellcheck disable=SC2046
exec build/computer_test $(cat ../build/computer/computer_test.args ../build/computer/assess_test.args ../build/computer/queue_test.args ../build/computer/plan_test.args)
