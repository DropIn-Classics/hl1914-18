#!/bin/sh
# Builds port/tests/fight_test.c and runs it on pairs of dumps of fights
# (see there).  Without arguments: the dumps build/n2/fights.sh made
# (g<run>_e<k>.ram at fight_losses, g<run>_x<k>.ram at its end).
set -e
cd "$(dirname "$0")/.."
CC=${CC:-cc}
RT=../doskit/runtime
mkdir -p build
$CC -O2 -Wall -Wextra -I$RT -Isrc -o build/fight_test tests/fight_test.c src/fight.c $RT/rmem.c $RT/sha256.c $RT/sys.c
if [ $# -gt 0 ]; then
    exec build/fight_test "$@"
fi
set --
for e in ../build/n2/g*_e*.ram; do
    x=$(echo "$e" | sed 's/_e\([0-9]*\)\.ram$/_x\1.ram/')
    [ -f "$x" ] && set -- "$@" "$e" "$x"
done
exec build/fight_test "$@"
