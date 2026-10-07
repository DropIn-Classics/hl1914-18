#!/bin/sh
# Builds port/tests/orders_test.c and runs it on pairs of dumps of
# attacks_run's loop over the orders (see there).  Without arguments: the
# passes in build/n2/orders_test.args (the first side's scripted moves).
set -e
cd "$(dirname "$0")/.."
CC=${CC:-cc}
RT=../doskit/runtime
mkdir -p build
$CC -O2 -Wall -Wextra -I$RT -Isrc -o build/orders_test tests/orders_test.c src/orders.c src/move.c src/units.c src/battle.c src/fight.c src/map.c src/files.c src/mem.c src/names.c $RT/rmem.c $RT/sha256.c $RT/sys.c
if [ $# -gt 0 ]; then
    exec build/orders_test "$@"
fi
# shellcheck disable=SC2046
exec build/orders_test $(cat ../build/n2/orders_test.args)
