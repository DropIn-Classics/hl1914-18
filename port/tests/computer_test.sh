#!/bin/sh
# Builds port/tests/computer_test.c and runs it on pairs of dumps of the
# computer's player's routines (see there).  Without arguments: the calls
# in build/n4/computer_test.args.
set -e
cd "$(dirname "$0")/.."
CC=${CC:-cc}
RT=../doskit/runtime
mkdir -p build
$CC -O2 -Wall -Wextra -I$RT -Isrc -o build/computer_test tests/computer_test.c src/computer.c $RT/rmem.c $RT/sha256.c $RT/sys.c
if [ $# -gt 0 ]; then
    exec build/computer_test "$@"
fi
# shellcheck disable=SC2046
exec build/computer_test $(cat ../build/n4/computer_test.args)
