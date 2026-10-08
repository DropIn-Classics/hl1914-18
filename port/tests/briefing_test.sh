#!/bin/sh
# Builds the port's program and lets it go through the menu and the new
# units' screens before a battle (port/src/briefing.c) with the key scripts
# of the walks port/tests/briefing_dumps.py ran the original with
# (build/briefing/WALK/, ignored), on the port's own clock, and compares
# the memory at the routine's entry and return and the video memory of
# each screen (port/tests/briefing_cmp.py).
#
#     python3 port/tests/briefing_dumps.py [WALK...]
#     sh port/tests/briefing_test.sh [WALK...]
set -e
cd "$(dirname "$0")/.."
GAME=${DOSKIT_GAME:-../game}
sh build.sh
[ $# -gt 0 ] || set -- m0 m12 m28 m4
r=0
for w in "$@"; do
    D=../build/briefing/$w
    [ -f "$D/end.ram" ] || { echo "no dump of the original: python3 port/tests/briefing_dumps.py $w"; exit 1; }
    rm -f "$D"/port_*.ram "$D"/port_*.vram
    build/history-line-headless -game "$GAME" -keys "$D/keys.txt" -start "$(cat "$D/start.txt")" -seed 0 \
        -until 60 -passes 1 -briefdump "$D" -saves "$D" > "$D/port.txt" 2>&1 || true
    tail -1 "$D/port.txt"
    python3 tests/briefing_cmp.py "$w" || r=1
done
exit $r
