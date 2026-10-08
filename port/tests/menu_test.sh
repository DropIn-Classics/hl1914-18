#!/bin/sh
# Builds the port's program and lets it begin at the menu (port/src/menu.c)
# with the key scripts of the walks port/tests/menu_dumps.py ran the
# original with (build/menu/WALK/, ignored), on the port's own clock from
# the second the original's first pass of the menu began, and compares
# what came in, the memory at the menu's last return and the video memory
# at the passes the original's was kept at (port/tests/menu_cmp.py).
#
#     python3 port/tests/menu_dumps.py [WALK...]
#     sh port/tests/menu_test.sh [WALK...]
#
# The walk `back` goes through the menu, into a battle, out of it by Esc
# and the confirming key, and through the menu again: the memory compared
# is that of the second return.
set -e
cd "$(dirname "$0")/.."
GAME=${DOSKIT_GAME:-../game}
sh build.sh
[ $# -gt 0 ] || set -- back code57 pages start0 start24
r=0
for w in "$@"; do
    D=../build/menu/$w
    [ -f "$D/exit.ram" ] || { echo "no dump of the original: python3 port/tests/menu_dumps.py $w"; exit 1; }
    n=$(grep -c ' 0$' "$D/acts.txt")
    until=$(awk '{ t = $1 } END { print t + 3 }' "$D/acts.txt")
    rm -f "$D/port.ram" "$D/port.log" "$D"/port*.vram
    V=
    for t in $(cat "$D/shots.txt" 2>/dev/null); do V="$V -menuvram $t $D/port$t.vram"; done
    build/history-line-headless -game "$GAME" -keys "$D/keys.txt" -start "$(cat "$D/start.txt")" -seed 0 \
        -until "$until" -menulog "$D/port.log" -menudump "$n" "$D/port.ram" $V -saves "$D" > "$D/port.txt" 2>&1 || true
    tail -1 "$D/port.txt"
    python3 tests/menu_cmp.py "$w" || r=1
done
exit $r
