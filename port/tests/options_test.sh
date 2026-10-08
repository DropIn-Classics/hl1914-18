#!/bin/sh
# The mouse and the joysticks as a side's input in a battle (input_mouse
# and the joysticks' part of input_side0/1, port/src/input.c; joy_setup
# and joy_read, port/src/joy.c) compared with the original's.
#
#     sh port/tests/options_test.sh dumps [TAG]   the original's run (build/play/TAG/)
#     sh port/tests/options_test.sh [TAG]         the port's, and the comparison
#
# TAG is m48_mouse, m48_joy or m48_joy0 (a stick there from the program's
# start); without one all three are run.
# Both programs go through the menu by keys to map 48 and get the keys of
# port/tests/keys/TAG.keys with the mouse of TAG.mice and the sticks of
# TAG.joy beside it (the runner's -mice and -joys, the port's -mice and
# -joys); the port's program plays the battle on the original's clock (as
# port/tests/fkeys_test.sh).  Memory is dumped a second after each key
# and half a second after each of the mouse's and the sticks' lines (and
# at play_dumps.py's own passes) and compared: the game's state
# (play_cmp.py) and of DATA the joysticks' words, the sides' input
# records and the mouse's record (options_cmp.py).  The input records'
# counts are given to the port once, from the original's first dump (see
# below).
set -e
cd "$(dirname "$0")/.."
GAME=${DOSKIT_GAME:-../game}
MODE=
if [ "$1" = dumps ]; then
    MODE=dumps
    shift
fi
TAGS=${1:-m48_mouse m48_joy m48_joy0}
dump_times() {
    {
        awk '/^[0-9]/ { print $1 + 1 }' "tests/keys/$1.keys"
        for f in "tests/keys/$1.mice" "tests/keys/$1.joy"; do
            [ -f "$f" ] && awk '/^[0-9]/ { print $1 + 0.5 }' "$f"
        done
    } | sort -n
}
if [ "$MODE" = dumps ]; then
    for T in $TAGS; do
        D=../build/play/$T
        UNTIL=64
        [ "$T" = m48_joy0 ] && UNTIL=40
        rm -rf "$D"
        WALK=code STATE=$D/state python3 tests/play_dumps.py "$T.keys" "$UNTIL"
        P=
        for t in $(dump_times "$T"); do
            P="$P $(awk -v t="$t" '$2 == "P" && $1 >= t { print $3; exit }' "$D/clock.txt")"
        done
        WALK=code STATE=$D/state PASSES="$P" python3 tests/play_dumps.py "$T.keys" "$UNTIL"
    done
    exit 0
fi
sh build.sh
r=0
for T in $TAGS; do
    D=../build/play/$T
    [ -f "$D/passes.txt" ] || { echo "no dumps of the original: sh port/tests/options_test.sh dumps $T"; exit 1; }
    rm -rf "$D/saves"
    mkdir -p "$D/saves"
    rm -f "$D"/q[0-9][0-9][0-9][0-9].ram
    ARGS=
    for p in $(cat "$D/passes.txt"); do
        ARGS="$ARGS -dump $p $D/q$(printf %04d "$p").ram"
    done
    # the sides' input records with their counts of holds and gaps (DATA:03BC
    # up to 0414) as the original has them when the first pass begins: the
    # counts run on from the menu, whose time is not the original's in the
    # port (as the timer routine's call of four, which play.c takes from
    # the clock's log)
    ARGS="$ARGS -poke $(python3 tests/options_cmp.py "$T" poke)"
    [ -f "tests/keys/$T.mice" ] && ARGS="$ARGS -mice tests/keys/$T.mice"
    [ -f "tests/keys/$T.joy" ] && ARGS="$ARGS -joys tests/keys/$T.joy"
    # shellcheck disable=SC2086
    build/history-line-headless -game "$GAME" -keys "$D/keys.txt" -start "$(cat "$D/start.txt")" -seed 0 \
        -clock "$D/clock.txt" -saves "$D/saves" -inputs "$D/port_inputs.txt" -ends "$D/q_ends.txt" $ARGS \
        > "$D/port.txt" 2>&1 || true
    tail -1 "$D/port.txt"
    python3 tests/play_cmp.py "$T" || r=1
    python3 tests/options_cmp.py "$T" || r=1
done
exit $r
