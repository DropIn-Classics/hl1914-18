#!/bin/sh
# Builds port/tests/units_test.c and runs it with the arguments given (see
# there); build/n2/destroy2.sh (a scratch script) makes the dumps and the
# argument list build/n2/units_test.args.
set -e
cd "$(dirname "$0")/.."
CC=${CC:-cc}
RT=../doskit/runtime
mkdir -p build
$CC -O2 -Wall -Wextra -I$RT -Isrc -o build/units_test tests/units_test.c src/units.c $RT/rmem.c $RT/sha256.c $RT/sys.c
if [ $# -eq 0 ]; then
    set -- $(cat ../build/n2/units_test.args)
fi
exec build/units_test "$@"
