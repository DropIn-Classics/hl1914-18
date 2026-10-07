#!/bin/sh
# Builds port/tests/income_test.c and runs it on pairs of dumps of
# S131D:1745 (see there).  Without arguments: the calls in every
# build/n2/income_test*.args (map 8, the key scripts d4.2 and d5).
set -e
cd "$(dirname "$0")/.."
CC=${CC:-cc}
RT=../doskit/runtime
mkdir -p build
$CC -O2 -Wall -Wextra -I$RT -Isrc -o build/income_test tests/income_test.c src/orders.c src/move.c src/units.c src/battle.c src/fight.c src/map.c src/files.c src/mem.c src/names.c $RT/rmem.c $RT/sha256.c $RT/sys.c
if [ $# -gt 0 ]; then
    exec build/income_test "$@"
fi
# shellcheck disable=SC2046
exec build/income_test $(cat ../build/n2/income_test*.args)
