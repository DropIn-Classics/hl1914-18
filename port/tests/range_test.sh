#!/bin/sh
# Builds port/tests/range_test.c and runs it on pairs of dumps of calls of
# move_range, attack_range and path_find (see there).  Without arguments: the calls
# build/n2/range2.py to range5.py dumped (build/n2/range_test.args).
set -e
cd "$(dirname "$0")/.."
CC=${CC:-cc}
RT=../doskit/runtime
mkdir -p build
$CC -O2 -Wall -Wextra -I$RT -Isrc -o build/range_test tests/range_test.c src/range.c src/fight.c $RT/rmem.c $RT/sha256.c $RT/sys.c
if [ $# -gt 0 ]; then
    exec build/range_test "$@"
fi
# shellcheck disable=SC2046
exec build/range_test $(cat ../build/n2/range_test.args)
