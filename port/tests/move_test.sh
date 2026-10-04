#!/bin/sh
# Builds port/tests/move_test.c and runs it on pairs of dumps of calls of
# move_target and move_step (see there).  Without arguments: the calls
# build/n2/move2.py dumped (build/n2/move_test.args).
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
exec build/move_test $(cat ../build/n2/move_test.args)
