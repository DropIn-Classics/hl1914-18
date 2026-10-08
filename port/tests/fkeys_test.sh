#!/bin/sh
# The keys F2 to F9 in a battle (setting_keys, port/src/loop.c) compared
# with the original's.
#
#     sh port/tests/fkeys_test.sh dumps     the original's run (build/play/m48_fkeys/)
#     sh port/tests/fkeys_test.sh           the port's, and the comparison
#
# Both programs go through the menu by keys to map 48 and get the keys of
# port/tests/keys/m48_fkeys.keys; the port's program plays the battle on
# the original's clock (as port/tests/save_test.sh).  Memory is dumped a
# second after each key (and at play_dumps.py's own passes) and compared:
# the game's state (play_cmp.py; the bits of game_state, the mouse's
# speed and the fights' setting are in it) and of DATA the joysticks'
# words (0380 to 03BC) and the mouse's record (0414 to 045C) but for the
# pointer's place in it (0424 to 0428: the original's is the runner's
# mouse, the port's the one its menu left; they differ from the first
# pass on), by range_cmp.py.
set -e
cd "$(dirname "$0")/.."
T=m48_fkeys
D=../build/play/$T
GAME=${DOSKIT_GAME:-../game}
if [ "$1" = dumps ]; then
    rm -rf $D
    WALK=code STATE=$D/state python3 tests/play_dumps.py $T.keys 64
    P=
    for t in $(awk '{ print $1 + 1 }' tests/keys/$T.keys 2>/dev/null | grep -v '^1$'); do
        P="$P $(awk -v t="$t" '$2 == "P" && $1 >= t { print $3; exit }' $D/clock.txt)"
    done
    WALK=code STATE=$D/state PASSES="$P" python3 tests/play_dumps.py $T.keys 64
    exit 0
fi
sh build.sh
[ -f "$D/passes.txt" ] || { echo "no dumps of the original: sh port/tests/fkeys_test.sh dumps"; exit 1; }
rm -rf "$D/saves"
mkdir -p "$D/saves"
rm -f "$D"/q[0-9][0-9][0-9][0-9].ram
DUMPS=
for p in $(cat "$D/passes.txt"); do
    DUMPS="$DUMPS -dump $p $D/q$(printf %04d "$p").ram"
done
# shellcheck disable=SC2086
build/history-line-headless -game "$GAME" -keys "$D/keys.txt" -start "$(cat "$D/start.txt")" -seed 0 \
    -clock "$D/clock.txt" -saves "$D/saves" -inputs "$D/port_inputs.txt" -ends "$D/q_ends.txt" $DUMPS \
    > "$D/port.txt" 2>&1 || true
tail -1 "$D/port.txt"
r=0
python3 tests/play_cmp.py $T || r=1
python3 tests/range_cmp.py $T 0380 03BC 0414 0424 0428 045C || r=1
exit $r
