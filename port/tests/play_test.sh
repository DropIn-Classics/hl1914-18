#!/bin/sh
# Builds the port's program and lets it play a battle by itself from its
# own setup (port/src/play.c), with the key script port/tests/keys/TAG.keys
# (default m8_site; a tag beginning with m0 is map 0, else map 8) and the
# original's clock, and compares its memory with the original's at the
# passes port/tests/play_dumps.py dumped (build/play/TAG/, ignored).
#
#     sh port/tests/play_test.sh [TAG [own|rounds]]
#
# With `own` the port plays on its own clock instead: of the original's
# log it takes only when the first pass began and the BIOS's count of
# ticks the setup started the random numbers with (the time of day).  Its
# passes then are not the original's (it draws nothing), so this says up
# to which pass compared the two still agree and does not fail later.
# With `rounds` it plays on its own clock too and is compared round by
# round instead of pass by pass: its memory at the pass of its n-th
# round's end and at the one after against the original's at the passes
# of its n-th (rNNNN.ram, rounds.txt), because the port's passes are
# shorter and a round's end comes in another pass.  What counts passes is
# then counted apart (play_cmp.py).  Run `own` first: it writes when the
# port's rounds end.
# A tag ending in _nopics (dumps made with NOPICS=1) plays the same key
# script with the fights not shown as pictures (-nopics).
set -e
cd "$(dirname "$0")/.."
TAG=${1:-m8_site}
D=../build/play/$TAG
GAME=${DOSKIT_GAME:-../game}
case $TAG in
m0*) MAP=0 ;;
*) MAP=8 ;;
esac
KEYS=${TAG%_nopics}
NOPICS=
[ "$KEYS" = "$TAG" ] || NOPICS=-nopics
sh build.sh
if [ "$2" = rounds ]; then
    START=$(awk '$2 == "P" { print $1; exit }' "$D/clock.txt")
    SEED=$(awk '$2 == "S" { v = $3 } $2 == "P" { print substr(v, 1, length(v) - 4); exit }' "$D/clock.txt")
    N=$(awk '$2 == "P" { n = $3 } END { print n - 1 }' "$D/clock.txt")
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
    build/history-line-headless -game "$GAME" -map $MAP -keys tests/keys/"$KEYS".keys -start $START \
        -seed 0x$SEED -passes $N $NOPICS $DUMPS
    exec python3 tests/play_cmp.py "$TAG" rounds
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
    -inputs "$IN" $DUMPS
exec python3 tests/play_cmp.py "$TAG" $2
