#!/bin/sh
# Builds port/tests/units_test.c and runs it with the arguments given (see
# there); build/n2/destroy2.py, build/n2/add1.py
# and build/n2/map1.py (scratch scripts) make the dumps and the argument
# lists build/n2/units_test.args, add_test.args and map_test.args.
set -e
cd "$(dirname "$0")/.."
CC=${CC:-cc}
RT=../doskit/runtime
mkdir -p build
$CC -O2 -Wall -Wextra -I$RT -Isrc -o build/units_test tests/units_test.c src/units.c src/map.c src/files.c src/names.c $RT/rmem.c $RT/sha256.c $RT/sys.c
if [ $# -eq 0 ]; then
    set -- $(cat ../build/n2/units_test.args ../build/n2/add_test.args ../build/n2/map_test.args)
fi
exec build/units_test "$@"
