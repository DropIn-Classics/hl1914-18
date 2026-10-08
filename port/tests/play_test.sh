#!/bin/sh
# Builds the port's program and lets it play a battle by itself from its
# own setup (port/src/play.c), with the key script port/tests/keys/TAG.keys
# (default m8_site; a tag beginning with m0 is map 0, else map 8) and the
# original's clock, and compares its memory with the original's at the
# passes port/tests/play_dumps.py dumped (build/play/TAG/, ignored).
#
#     sh port/tests/play_test.sh [TAG [own|rounds|seeds|live]]
#     sh port/tests/play_test.sh TAG at KEYS T...
#
# With `own` the port plays on its own clock instead: of the original's
# log it takes only when the first pass began and the BIOS's count of
# ticks the setup started the random numbers with (the time of day).  Its
# passes then are not the original's (it draws nothing), so this says up
# to which pass compared the two still agree and does not fail later.
# With `rounds` it plays on its own clock too, up to the second the
# original's log ends at, and is compared round by round instead of pass
# by pass: its memory at the pass of its n-th round's end and at the one
# after against the original's at the passes of its n-th (rNNNN.ram,
# rounds.txt), because the port's passes are shorter and a round's end
# comes in another pass.  What counts passes is then counted apart
# (play_cmp.py).  The port plays twice for it: once to find the passes
# its rounds end in (own_rounds.txt), once to dump them.
# With `seeds` the same, but the port takes from the original's log what
# its fights' pictures did to the random numbers (-seeds: the number each
# started them with and its calls of rand_next), and the random numbers
# are compared too: what still differs then has another cause.
# With `at KEYS T...` the port plays another key script, tests/keys/KEYS.keys,
# on its own clock and is compared with the original's run of TAG at the
# first pass each begins at or after T seconds (the original's needs a dump:
# PASSES in play_dumps.py's environment, the test names the passes).  For a
# script TAG that gives the original the keys as the port's clock sampled
# those of KEYS: the two plays should then be the same but for the passes'.
# With `live` the port plays the key script twice on its own clock: once
# in the window (play_live, headless: the keys given to the window's
# pictures at their times, tests/live_keys.py) and once by -keys, and
# says whether the two read the same in every part and have the same
# memory and video memory at the passes of their rounds' ends and after
# them.  A script with a mouse has tests/keys/TAG.mice (the window's
# -micelog), which every -keys play here and the original's run take,
# and TAG.moves, the motions the window was given (DK_MOUSEMOVE).  A script the window wrote (-keylog) must be the same; the
# comparison with the original is then `rounds` or `at`.
# A tag ending in _nopics (dumps made with NOPICS=1) plays the same key
# script with the fights not shown as pictures (-nopics); one beginning
# with m0 is map 0, with mNN_ map NN, else map 8.
set -e
# awk reads the clock's times as numbers only with a decimal point
export LC_ALL=C
cd "$(dirname "$0")/.."
TAG=${1:-m8_site}
D=../build/play/$TAG
GAME=${DOSKIT_GAME:-../game}
case $TAG in
m0*) MAP=0 ;;
m[0-9][0-9]_*) MAP=${TAG%%_*} MAP=${MAP#m} ;;
*) MAP=8 ;;
esac
KEYS=${TAG%_nopics}
NOPICS=
[ "$KEYS" = "$TAG" ] || NOPICS=-nopics
# the script's pokes (tests/keys/TAG.pokes, play_pokes.py), if it has any
NOPICS="$NOPICS $(python3 tests/play_pokes.py "$KEYS" port)"
# the script's mouse (tests/keys/TAG.mice, the window's -micelog), if it has
# one; the window's play in `live` gets the motions of TAG.moves instead
MICE=
[ -f "tests/keys/$KEYS.mice" ] && MICE="-mice tests/keys/$KEYS.mice"
sh build.sh
# what the port's program writes (a saved game, a map's scores) stays here
mkdir -p "$D/saves"
NOPICS="$NOPICS -saves $D/saves"
if [ "$2" = at ]; then
    START=$(awk '$2 == "P" { print $1; exit }' "$D/clock.txt")
    SEED=$(awk '$2 == "S" { v = $3 } $2 == "P" { print substr(v, 1, length(v) - 4); exit }' "$D/clock.txt")
    MICE=
    [ -f "tests/keys/$3.mice" ] && MICE="-mice tests/keys/$3.mice"
    RUN="build/history-line-headless -game $GAME -map $MAP -keys tests/keys/$3.keys -start $START -seed 0x$SEED $NOPICS $MICE"
    shift 3
    rm -f "$D"/r[0-9][0-9][0-9][0-9].ram "$D"/r[0-9][0-9][0-9][0-9].vram
    : > "$D/rounds.txt"
    i=1
    for t in "$@"; do
        o=$(awk -v t="$t" '$2 == "P" && $1 >= t { print $3; exit }' "$D/clock.txt")
        [ -f "$D/p$(printf %04d "$o").ram" ] || { echo "no dump of the original's pass $o ($t s): PASSES=$o python3 port/tests/play_dumps.py $KEYS.keys"; exit 1; }
        # shellcheck disable=SC2086
        p=$($RUN -until "$t" 2>&1 | awk '/passes played/ { print $1 + 1 }')
        V=
        [ -f "$D/p$(printf %04d "$o").vram" ] && V="-dump $p $D/r$(printf %04d "$p").vram"
        # shellcheck disable=SC2086
        $RUN -until "$t" -dump 1 "$D/r0001.ram" -dump "$p" "$D/r$(printf %04d "$p").ram" $V > /dev/null 2>&1
        echo "$i $o $p" >> "$D/rounds.txt"
        i=$((i + 1))
    done
    exec python3 tests/play_cmp.py "$TAG" at
fi
if [ "$2" = live ]; then
    START=$(awk '$2 == "P" { print $1; exit }' "$D/clock.txt")
    SEED=$(awk '$2 == "S" { v = $3 } $2 == "P" { print substr(v, 1, length(v) - 4); exit }' "$D/clock.txt")
    UNTIL=$(awk 'END { print $1 }' "$D/clock.txt")
    ARGS="-game $GAME -map $MAP -start $START -seed 0x$SEED $NOPICS"
    # shellcheck disable=SC2086
    build/history-line-headless $ARGS $MICE -keys tests/keys/$KEYS.keys -until "$UNTIL" -rounds "$D/own_rounds.txt" > /dev/null 2>&1
    rm -f "$D"/[kw][0-9][0-9][0-9][0-9].ram "$D"/[kw][0-9][0-9][0-9][0-9].vram "$D"/[kw][ef][0-9]*.ram "$D"/[kw][ef][0-9]*.vram
    K="-evdump $D/k" W="-evdump $D/w"
    for p in 1 $(awk '{ print $1, $1 + 1 }' "$D/own_rounds.txt"); do
        f=$(printf %04d "$p")
        K="$K -dump $p $D/k$f.ram -dump $p $D/k$f.vram"
        W="$W -dump $p $D/w$f.ram -dump $p $D/w$f.vram"
    done
    # shellcheck disable=SC2086
    build/history-line-headless $ARGS $MICE -keys tests/keys/$KEYS.keys -until "$UNTIL" -inputs "$D/keys_inputs.txt" $K
    # shellcheck disable=SC2086
    MOVES=
    [ -f "tests/keys/$KEYS.moves" ] && MOVES=$(python3 tests/live_keys.py --moves "tests/keys/$KEYS.moves" "$START")
    DK_KEYS=$(python3 tests/live_keys.py "tests/keys/$KEYS.keys" "$START") DK_MOUSEMOVE=$MOVES \
    DK_FRAMES=$(awk -v s="$START" -v u="$UNTIL" 'BEGIN { print int(38 + (u - s) / 0.014268) }') \
        build/history-line-headless $ARGS -inputs "$D/live_inputs.txt" $W
    n=$(wc -l < "$D/live_inputs.txt")
    [ "$(wc -l < "$D/keys_inputs.txt")" -lt "$n" ] && n=$(wc -l < "$D/keys_inputs.txt")
    bad=0
    if head -n "$n" "$D/live_inputs.txt" | cmp -s - "$D/keys_inputs.txt" 2>/dev/null ||
       head -n "$n" "$D/keys_inputs.txt" | cmp -s - "$D/live_inputs.txt"; then
        echo "inputs: the first $n parts the same"
    else
        echo "inputs: DIFFERENT"; bad=1
    fi
    for f in "$D"/k[0-9][0-9][0-9][0-9].* "$D"/k[ef][0-9]*.*; do
        [ -f "$f" ] || continue
        g=$D/w${f##*/k}
        if [ ! -f "$g" ]; then
            echo "${f##*/}: the window did not come so far"
        elif cmp -s "$f" "$g"; then
            echo "${f##*/}: the same"
        else
            echo "${f##*/}: DIFFERENT"; bad=1
        fi
    done
    exit $bad
fi
if [ "$2" = rounds ] || [ "$2" = seeds ]; then
    START=$(awk '$2 == "P" { print $1; exit }' "$D/clock.txt")
    SEED=$(awk '$2 == "S" { v = $3 } $2 == "P" { print substr(v, 1, length(v) - 4); exit }' "$D/clock.txt")
    UNTIL=$(awk 'END { print $1 }' "$D/clock.txt")
    SEEDS=
    [ "$2" = rounds ] || SEEDS="-seeds $D/clock.txt"
    RUN="build/history-line-headless -game $GAME -map $MAP -keys tests/keys/$KEYS.keys -start $START -seed 0x$SEED -until $UNTIL $NOPICS $MICE $SEEDS"
    # shellcheck disable=SC2086
    $RUN -rounds "$D/own_rounds.txt"
    rm -f "$D"/r[0-9][0-9][0-9][0-9].ram "$D"/r[0-9][0-9][0-9][0-9].vram "$D"/r[ef][0-9]*.ram "$D"/r[ef][0-9]*.vram
    DUMPS="-dump 1 $D/r0001.ram -evdump $D/r"
    : > "$D/rounds.txt"
    i=1
    for o in $(awk '$2 == "A" { print p } $2 == "P" { p = $3 }' "$D/clock.txt"); do
        p=$(awk -v i=$i 'NR == i { print $1 }' "$D/own_rounds.txt")
        [ -n "$p" ] || break
        for k in 0 1; do
            [ -f "$D/p$(printf %04d $((o + k))).ram" ] || continue
            DUMPS="$DUMPS -dump $((p + k)) $D/r$(printf %04d $((p + k))).ram"
            [ -f "$D/p$(printf %04d $((o + k))).vram" ] && DUMPS="$DUMPS -dump $((p + k)) $D/r$(printf %04d $((p + k))).vram"
            echo "$i $((o + k)) $((p + k))" >> "$D/rounds.txt"
        done
        i=$((i + 1))
    done
    # shellcheck disable=SC2086
    $RUN $DUMPS
    exec python3 tests/play_cmp.py "$TAG" "$2"
fi
if [ "$2" = own ]; then
    Q=o
    START=$(awk '$2 == "P" { print $1; exit }' "$D/clock.txt")
    SEED=$(awk '$2 == "S" { v = $3 } $2 == "P" { print substr(v, 1, length(v) - 4); exit }' "$D/clock.txt")
    CLOCK="-start $START -seed 0x$SEED -passes $(awk '$2 == "P" { n = $3 } END { print n - 1 }' "$D/clock.txt")"
    IN=$D/own_inputs.txt
    CLOCK="$CLOCK -rounds $D/own_rounds.txt"
else
    Q=q
    CLOCK="-clock $D/clock.txt"
    IN=$D/port_inputs.txt
fi
rm -f "$D"/$Q[0-9][0-9][0-9][0-9].ram "$D"/$Q[0-9][0-9][0-9][0-9].vram "$D"/${Q}[ef][0-9]*.ram "$D"/${Q}[ef][0-9]*.vram
DUMPS=
for p in $(cat "$D/passes.txt"); do
    DUMPS="$DUMPS -dump $p $D/$Q$(printf %04d "$p").ram"
    # the video memory too where the original's was dumped (VRAM=1)
    [ -f "$D/p$(printf %04d "$p").vram" ] && DUMPS="$DUMPS -dump $p $D/$Q$(printf %04d "$p").vram"
done
# shellcheck disable=SC2086
build/history-line-headless -game "$GAME" -map $MAP -keys tests/keys/"$KEYS".keys $CLOCK $NOPICS $MICE \
    -inputs "$IN" -ends "$D/${Q}_ends.txt" -evdump "$D/$Q" $DUMPS
exec python3 tests/play_cmp.py "$TAG" $2
