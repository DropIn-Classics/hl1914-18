#!/bin/sh
# Builds the port's program and lets it play a battle by itself from its
# own setup (port/src/play.c), with the key script port/tests/keys/TAG.keys
# (default m8_site; a tag beginning with m0 is map 0, else map 8) and the
# original's clock, and compares its memory with the original's at the
# passes port/tests/play_dumps.py dumped (build/play/TAG/, ignored).
#
#     sh port/tests/play_test.sh [TAG [own|rounds|seeds]]
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
# A tag ending in _nopics (dumps made with NOPICS=1) plays the same key
# script with the fights not shown as pictures (-nopics); one beginning
# with m0 is map 0, with mNN_ map NN, else map 8.
set -e
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
sh build.sh
if [ "$2" = at ]; then
    START=$(awk '$2 == "P" { print $1; exit }' "$D/clock.txt")
    SEED=$(awk '$2 == "S" { v = $3 } $2 == "P" { print substr(v, 1, length(v) - 4); exit }' "$D/clock.txt")
    RUN="build/history-line-headless -game $GAME -map $MAP -keys tests/keys/$3.keys -start $START -seed 0x$SEED $NOPICS"
    shift 3
    rm -f "$D"/r[0-9][0-9][0-9][0-9].ram
    : > "$D/rounds.txt"
    i=1
    for t in "$@"; do
        o=$(awk -v t="$t" '$2 == "P" && $1 >= t { print $3; exit }' "$D/clock.txt")
        [ -f "$D/p$(printf %04d "$o").ram" ] || { echo "no dump of the original's pass $o ($t s): PASSES=$o python3 port/tests/play_dumps.py $KEYS.keys"; exit 1; }
        # shellcheck disable=SC2086
        p=$($RUN -until "$t" 2>&1 | awk '/passes played/ { print $1 + 1 }')
        # shellcheck disable=SC2086
        $RUN -until "$t" -dump 1 "$D/r0001.ram" -dump "$p" "$D/r$(printf %04d "$p").ram" > /dev/null 2>&1
        echo "$i $o $p" >> "$D/rounds.txt"
        i=$((i + 1))
    done
    exec python3 tests/play_cmp.py "$TAG" at
fi
if [ "$2" = rounds ] || [ "$2" = seeds ]; then
    START=$(awk '$2 == "P" { print $1; exit }' "$D/clock.txt")
    SEED=$(awk '$2 == "S" { v = $3 } $2 == "P" { print substr(v, 1, length(v) - 4); exit }' "$D/clock.txt")
    UNTIL=$(awk 'END { print $1 }' "$D/clock.txt")
    SEEDS=
    [ "$2" = rounds ] || SEEDS="-seeds $D/clock.txt"
    RUN="build/history-line-headless -game $GAME -map $MAP -keys tests/keys/$KEYS.keys -start $START -seed 0x$SEED -until $UNTIL $NOPICS $SEEDS"
    # shellcheck disable=SC2086
    $RUN -rounds "$D/own_rounds.txt"
    rm -f "$D"/r[0-9][0-9][0-9][0-9].ram
    DUMPS="-dump 1 $D/r0001.ram"
    : > "$D/rounds.txt"
    i=1
    for o in $(awk '$2 == "A" { print p } $2 == "P" { p = $3 }' "$D/clock.txt"); do
        p=$(awk -v i=$i 'NR == i { print $1 }' "$D/own_rounds.txt")
        [ -n "$p" ] || break
        for k in 0 1; do
            [ -f "$D/p$(printf %04d $((o + k))).ram" ] || continue
            DUMPS="$DUMPS -dump $((p + k)) $D/r$(printf %04d $((p + k))).ram"
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
rm -f "$D"/$Q[0-9][0-9][0-9][0-9].ram
DUMPS=
for p in $(cat "$D/passes.txt"); do
    DUMPS="$DUMPS -dump $p $D/$Q$(printf %04d "$p").ram"
done
# shellcheck disable=SC2086
build/history-line-headless -game "$GAME" -map $MAP -keys tests/keys/"$KEYS".keys $CLOCK $NOPICS \
    -inputs "$IN" -ends "$D/${Q}_ends.txt" $DUMPS
exec python3 tests/play_cmp.py "$TAG" $2
