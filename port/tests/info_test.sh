#!/bin/sh
# The field menu's information on a unit and on a field without one
# (unit_info and battle_info, port/src/draw.c) compared with the
# original's.
#
#     python3 port/tests/info_dumps.py     the original's runs (build/play/m48_info/)
#     sh port/tests/info_test.sh           the port's, and the comparison
#
# Both programs go through the menu by keys to map 48 and get the keys of
# port/tests/keys/m48_info.keys; the port's program plays the battle on
# the original's clock (as port/tests/save_test.sh).  Compared: memory at
# the passes play_dumps.py chose (play_cmp.py), and for each call of the
# two routines the memory at its entry and return and the video memory at
# its return, and the video memory of a pass a second later, while the
# information is shown (info_cmp.py).
set -e
cd "$(dirname "$0")/.."
T=${1:-m48_info}
D=../build/play/$T
GAME=${DOSKIT_GAME:-../game}
sh build.sh
[ -f "$D/kinds.txt" ] || { echo "no dumps of the original: python3 port/tests/info_dumps.py"; exit 1; }
rm -rf "$D/saves"
mkdir -p "$D/saves"
rm -f "$D"/q[0-9][0-9][0-9][0-9].ram "$D"/port_info* "$D"/port_shown* "$D"/port_kinds.txt
DUMPS=
for p in $(cat "$D/passes.txt"); do
    DUMPS="$DUMPS -dump $p $D/q$(printf %04d "$p").ram"
done
n=0
for p in $(cat "$D/shown.txt"); do
    n=$((n + 1))
    DUMPS="$DUMPS -dump $p $D/port_shown$n.vram"
done
# shellcheck disable=SC2086
build/history-line-headless -game "$GAME" -keys "$D/keys.txt" -start "$(cat "$D/start.txt")" -seed 0 \
    -clock "$D/clock.txt" -saves "$D/saves" -inputs "$D/port_inputs.txt" -ends "$D/q_ends.txt" \
    -infodump "$D" $DUMPS > "$D/port.txt" 2>&1 || true
tail -1 "$D/port.txt"
r=0
python3 tests/play_cmp.py $T || r=1
python3 tests/info_cmp.py $T || r=1
exit $r
