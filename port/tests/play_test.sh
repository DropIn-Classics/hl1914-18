#!/bin/sh
# Builds the port's program and lets it play a battle by itself from its
# own setup (port/src/play.c), with the key script port/tests/keys/TAG.keys
# (default m8_site; a tag beginning with m0 is map 0, else map 8) and the
# original's clock, and compares its memory with the original's at the
# passes port/tests/play_dumps.py dumped (build/play/TAG/, ignored).
#
#     sh port/tests/play_test.sh [TAG]
set -e
cd "$(dirname "$0")/.."
TAG=${1:-m8_site}
D=../build/play/$TAG
GAME=${DOSKIT_GAME:-../game}
case $TAG in
m0*) MAP=0 ;;
*) MAP=8 ;;
esac
sh build.sh
rm -f "$D"/q[0-9][0-9][0-9][0-9].ram
DUMPS=
for p in $(cat "$D/passes.txt"); do
    DUMPS="$DUMPS -dump $p $D/q$(printf %04d "$p").ram"
done
# shellcheck disable=SC2086
build/history-line-headless -game "$GAME" -map $MAP -keys tests/keys/"$TAG".keys -clock "$D/clock.txt" \
    -inputs "$D/port_inputs.txt" $DUMPS
exec python3 tests/play_cmp.py "$TAG"
