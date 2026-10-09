#!/bin/sh
# The mouse and the joysticks as a side's input in a battle (input_mouse
# and the joysticks' part of input_side0/1, port/src/input.c; joy_setup
# and joy_read, port/src/joy.c) compared with the original's.
#
#     sh port/tests/options_test.sh dumps [TAG]   the original's run (build/play/TAG/)
#     sh port/tests/options_test.sh [TAG]         the port's, and the comparison
#     sh port/tests/options_test.sh map [TAG]     the port begun with -map
#     sh port/tests/options_test.sh own [TAG]     the port on its own clock
#
# TAG is m48_mouse, m48_joy, m48_joy0 (a stick there from the program's
# start), m48_ftext (the texts of F5 to F8, with the video memory) or
# m48_joyown (taps of the sticks, for `own`); without one all that the
# way of running has are run.
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
# below).  m48_ftext is dumped 0.3 s after each key too, and with the
# video memory, which play_cmp.py compares where both have it.
#
# With `map` (m48_joy, m48_joy0) the port does not go through its menu:
# it begins with -map 48 and gets the same log of the clock, whose calls
# of the timer's routine it then makes from the program's start on, the
# sticks' script going by their times.  Compared with the same dumps of
# the original (build/play/TAG_map/ has links to them); the menu's
# range of the mouse's pointer, which -map never sets, is told apart
# (options_cmp.py).
#
# With `own` (m48_joyown) the port plays from its menu on on its own
# clock, the sticks' script by that clock's time.  Its passes are not
# the original's then, so the two are compared time by time: at each of
# the times above the original's first pass begun at or after it against
# the port's (build/play/TAG_own/: rounds.txt, rNNNN.ram; play_cmp.py
# `at`, which counts what counts passes apart, and options_cmp.py `own`).
set -e
# awk reads the clock's times as numbers only with a decimal point
export LC_ALL=C
cd "$(dirname "$0")/.."
GAME=${DOSKIT_GAME:-../game}
MODE=
case $1 in
dumps|map|own) MODE=$1; shift ;;
esac
case $MODE in
dumps) TAGS=${1:-m48_mouse m48_joy m48_joy0 m48_ftext m48_joyown} ;;
map) TAGS=${1:-m48_joy m48_joy0} ;;
own) TAGS=${1:-m48_joyown} ;;
*) TAGS=${1:-m48_mouse m48_joy m48_joy0 m48_ftext} ;;
esac
dump_times() {
    {
        awk '/^[0-9]/ { print $1 + 1 }' "tests/keys/$1.keys"
        [ "$1" = m48_ftext ] && awk '/^[0-9]/ { print $1 + 0.3 }' "tests/keys/$1.keys"
        for f in "tests/keys/$1.mice" "tests/keys/$1.joy"; do
            [ -f "$f" ] && awk '/^[0-9]/ && $1 > 0 { print $1 + 0.5 }' "$f"
        done
    } | sort -n
}
# the original's first pass begun at or after each of a tag's times
dump_passes() {
    for t in $(dump_times "$1"); do
        awk -v t="$t" '$2 == "P" && $1 >= t { print $3; exit }' "../build/play/$1/clock.txt"
    done | uniq
}
if [ "$MODE" = dumps ]; then
    for T in $TAGS; do
        D=../build/play/$T
        UNTIL=64
        [ "$T" = m48_joy0 ] && UNTIL=40
        [ "$T" = m48_ftext ] || [ "$T" = m48_joyown ] && UNTIL=50
        V=
        [ "$T" = m48_ftext ] && V=1
        rm -rf "$D"
        WALK=code STATE=$D/state VRAM=$V python3 tests/play_dumps.py "$T.keys" "$UNTIL"
        WALK=code STATE=$D/state VRAM=$V PASSES="$(dump_passes "$T" | tr '\n' ' ')" python3 tests/play_dumps.py "$T.keys" "$UNTIL"
    done
    exit 0
fi
sh build.sh
# the original's dumps of a tag linked into the folder $2 for another way
# of running the port
links() {
    rm -rf "$2"
    mkdir -p "$2/saves"
    for f in "../build/play/$1"/p[0-9][0-9][0-9][0-9].ram "../build/play/$1"/inputs.txt \
             "../build/play/$1"/passes.txt "../build/play/$1"/clock.txt; do
        ln -s "$(cd "../build/play/$1" && pwd)/${f##*/}" "$2/"
    done
}
r=0
for T in $TAGS; do
    D=../build/play/$T
    [ -f "$D/passes.txt" ] || { echo "no dumps of the original: sh port/tests/options_test.sh dumps $T"; exit 1; }
    JOYS=
    [ -f "tests/keys/$T.joy" ] && JOYS="-joys tests/keys/$T.joy"
    if [ "$MODE" = own ]; then
        O=${D}_own
        links "$T" "$O"
        RUN="build/history-line-headless -game $GAME -keys $D/keys.txt -start $(cat "$D/start.txt") -seed 0 -saves $O/saves $JOYS"
        [ -f "tests/keys/$T.mice" ] && RUN="$RUN -mice tests/keys/$T.mice"
        ARGS="-dump 1 $O/r0001.ram"
        i=1
        for t in $(dump_times "$T"); do
            o=$(awk -v t="$t" '$2 == "P" && $1 >= t { print $3; exit }' "$D/clock.txt")
            # the port's first pass begun at or after t: the one behind
            # those it has played when its clock comes there
            # shellcheck disable=SC2086
            p=$($RUN -until "$t" 2>&1 | sed -n 's/.*The last battle: \([0-9]*\) passes played.*/\1/p')
            p=$((p + 1))
            ARGS="$ARGS -dump $p $O/r$(printf %04d "$p").ram"
            echo "$i $o $p" >> "$O/rounds.txt"
            i=$((i + 1))
        done
        # shellcheck disable=SC2086
        $RUN -until "$(awk 'END { print $1 }' "$D/clock.txt")" -inputs "$O/own_inputs.txt" $ARGS > "$O/port.txt" 2>&1 || true
        tail -1 "$O/port.txt"
        python3 tests/play_cmp.py "${T}_own" at || r=1
        python3 tests/options_cmp.py "${T}_own" own || r=1
        continue
    fi
    if [ "$MODE" = map ]; then
        M=${D}_map
        links "$T" "$M"
        ARGS=
        for p in $(cat "$D/passes.txt"); do
            ARGS="$ARGS -dump $p $M/q$(printf %04d "$p").ram"
        done
        # the input records' counts as below; a poke is made when the
        # first pass begins, behind the log's calls before it
        # shellcheck disable=SC2086
        build/history-line-headless -game "$GAME" -map 48 -keys "tests/keys/$T.keys" -clock "$D/clock.txt" \
            -seeds "$D/clock.txt" -saves "$M/saves" -inputs "$M/port_inputs.txt" -ends "$M/q_ends.txt" \
            -poke $(python3 tests/options_cmp.py "$T" poke) $JOYS $ARGS > "$M/port.txt" 2>&1 || true
        tail -1 "$M/port.txt"
        python3 tests/play_cmp.py "${T}_map" || r=1
        python3 tests/options_cmp.py "${T}_map" map || r=1
        continue
    fi
    rm -rf "$D/saves"
    mkdir -p "$D/saves"
    rm -f "$D"/q[0-9][0-9][0-9][0-9].ram "$D"/q[0-9][0-9][0-9][0-9].vram
    ARGS=
    for p in $(cat "$D/passes.txt"); do
        ARGS="$ARGS -dump $p $D/q$(printf %04d "$p").ram"
        # the video memory too where the original's was dumped
        [ -f "$D/p$(printf %04d "$p").vram" ] && ARGS="$ARGS -dump $p $D/q$(printf %04d "$p").vram"
    done
    # the sides' input records with their counts of holds and gaps (DATA:03BC
    # up to 0414) as the original has them when the first pass begins: the
    # counts run on from the menu, whose time is not the original's in the
    # port (as the timer routine's call of four, which play.c takes from
    # the clock's log)
    ARGS="$ARGS -poke $(python3 tests/options_cmp.py "$T" poke)"
    [ -f "tests/keys/$T.mice" ] && ARGS="$ARGS -mice tests/keys/$T.mice"
    # shellcheck disable=SC2086
    build/history-line-headless -game "$GAME" -keys "$D/keys.txt" -start "$(cat "$D/start.txt")" -seed 0 \
        -clock "$D/clock.txt" -saves "$D/saves" -inputs "$D/port_inputs.txt" -ends "$D/q_ends.txt" $JOYS $ARGS \
        > "$D/port.txt" 2>&1 || true
    tail -1 "$D/port.txt"
    python3 tests/play_cmp.py "$T" || r=1
    python3 tests/options_cmp.py "$T" || r=1
done
exit $r
