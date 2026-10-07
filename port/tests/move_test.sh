#!/bin/sh
# Builds port/tests/move_test.c and runs it on pairs of dumps of calls of
# move_target, move_step and move_arrive (see there).  Without arguments:
# the calls in every build/n2/move_test*.args (move_test.args: those
# build/n2/move2.py dumped; move_test_sides.args: the first side's
# scripted moves into a site, a carrier and a site of nobody).
set -e
cd "$(dirname "$0")/.."
CC=${CC:-cc}
RT=../doskit/runtime
mkdir -p build
$CC -O2 -Wall -Wextra -I$RT -Isrc -o build/move_test tests/move_test.c src/move.c src/fight.c $RT/rmem.c $RT/sha256.c $RT/sys.c
if [ $# -gt 0 ]; then
    exec build/move_test "$@"
fi
# shellcheck disable=SC2046
exec build/move_test $(cat ../build/n2/move_test*.args)
