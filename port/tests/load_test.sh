#!/bin/sh
# Builds port/tests/load_test.c and runs it on the installed game's files
# (../game, or $DOSKIT_GAME) with the arguments given (see there);
# build/n2/load1.py and build/n2/calls1.py (scratch scripts) make the dumps
# and the argument lists build/n2/load_test*.args.
set -e
cd "$(dirname "$0")/.."
CC=${CC:-cc}
RT=../doskit/runtime
mkdir -p build
$CC -O2 -Wall -Wextra -I$RT -Isrc -o build/load_test tests/load_test.c src/files.c src/map.c src/units.c src/names.c src/mem.c $RT/rmem.c $RT/sha256.c $RT/sys.c
if [ $# -eq 0 ]; then
    set -- $(cat ../build/n2/load_test*.args)
fi
exec build/load_test "${DOSKIT_GAME:-../game}" "$@"
