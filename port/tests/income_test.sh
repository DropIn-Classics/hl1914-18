#!/bin/sh
# Builds port/tests/income_test.c and runs it on pairs of dumps of
# S131D:1745 (see there).  Without arguments: the calls in
# tests/income_test.args (made by tests/sites_dumps.sh).
set -e
cd "$(dirname "$0")/.."
CC=${CC:-cc}
RT=../doskit/runtime
mkdir -p build
$CC -O2 -Wall -Wextra -I$RT -Isrc -o build/income_test tests/income_test.c src/orders.c src/side.c src/units.c src/range.c src/battle.c src/pictures.c src/screen.c src/draw.c src/map.c src/files.c src/names.c src/fight.c src/move.c src/mem.c $RT/rmem.c $RT/vga.c $RT/sha256.c $RT/sys.c
if [ $# -gt 0 ]; then
    exec build/income_test "$@"
fi
# shellcheck disable=SC2046
exec build/income_test $(cat tests/income_test.args)
