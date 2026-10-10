#!/bin/sh
# Saving and loading a battle (port/src/save.c) compared with the
# original's.
#
#     sh port/tests/save_test.sh dumps     the original's runs (build/play/)
#     sh port/tests/save_test.sh           the port's, and the comparison
#
# Two runs of each program, both through the menu by keys:
#
#   m48_save  the code of map 48 typed, START; both sides end their half,
#             then D and 3 save the battle as number 3, and later D and
#             Esc leave the saving (port/tests/keys/m48_save.keys)
#   m48_save2 as m48_save, then ten seconds later D and 4 save it again
#             as number 4 (port/tests/keys/m48_save2.keys); both files
#             compared, and the window's play (DK_KEYS, live_keys.py) of
#             the same keys with four seconds before each digit must
#             write both and go on (the port once ended the battle at
#             the second save's question there, docs/HANDOFF.md)
#   m48_load  MISC, LOAD, 3: that battle loaded and played on: the second
#             side goes back into its half and ends it again, the first
#             F1 carries the round out (port/tests/keys/m48_load.keys)
#
#   load_bad, load_cut, load_head, load_none
#             MISC, LOAD and a digit with a damaged file there, made
#             from m48_save's saved game: 00.DAT with 6 as its second
#             byte, 09.DAT its first 7000h bytes, 01.DAT its first ten,
#             02.DAT empty; then the F1 pair (port/tests/keys/load_*.keys).
#             The original starts a battle from each (docs/HANDOFF.md);
#             the port deliberately refuses them (save_valid, save.c):
#             it must show no battle and wait in the menu.  Only the
#             port's run is checked; the original's dumps stay as the
#             record of what the original does.
#
# The original's (play_dumps.py with WALK and STATE) write the saved game
# into build/play/m48_save/state/ and load it from a copy of that; the
# port's program walks its own menu on its own clock and plays the battle
# on the original's (-clock), writes its saved game into
# build/play/m48_save/saves/ and loads its own file from a copy.  Compared:
# the two files byte for byte, and memory at the passes dumped
# (play_cmp.py: before and after the saving, the first passes of the
# loaded battle and later ones).  Of m48_load every pass from the first
# to the third behind the round's end is dumped and compared.
set -e
cd "$(dirname "$0")/.."
B=../build/play
GAME=${DOSKIT_GAME:-../game}
N=03.DAT
# damaged TAG FROM DIR: the damaged file of that run, made from the saved
# game FROM, in DIR; prints the digit its name has
damaged() {
    mkdir -p "$3"
    case $1 in
    load_bad) { printf '\000\006'; tail -c +3 "$2"; } > "$3/00.DAT"; echo 0 ;;
    load_cut) head -c 28672 "$2" > "$3/09.DAT"; echo 9 ;;
    load_head) head -c 10 "$2" > "$3/01.DAT"; echo 1 ;;
    load_none) : > "$3/02.DAT"; echo 2 ;;
    esac
}
BAD="load_bad load_cut load_head load_none"
if [ "$1" = dumps ]; then
    rm -rf $B/m48_save $B/m48_load
    WALK=code STATE=$B/m48_save/state python3 tests/play_dumps.py m48_save.keys 64
    # the passes around the two times the saving is asked for
    P=$(awk '$2 == "P" && $4 == 0 { if (p && $1 - p > 1) printf "%d %d %d %d ", n, $3, $3 + 1, $3 + 6; p = $1; n = $3 }' $B/m48_save/clock.txt)
    WALK=code STATE=$B/m48_save/state PASSES="$P" python3 tests/play_dumps.py m48_save.keys 64
    [ -f $B/m48_save/state/$N ] || { echo "the original wrote no $N"; exit 1; }
    rm -rf $B/m48_save2
    WALK=code STATE=$B/m48_save2/state python3 tests/play_dumps.py m48_save2.keys 72
    mkdir -p $B/m48_load/state
    cp $B/m48_save/state/$N $B/m48_load/state/
    WALK=load3 STATE=$B/m48_load/state PASSES="2 3 10 100" python3 tests/play_dumps.py m48_load.keys 60
    # every pass through the round's end (the first pass attacks_run ran in) and three more
    E=$(awk '$2 == "P" { p = $3 } $2 == "A" { print p + 3; exit }' $B/m48_load/clock.txt)
    WALK=load3 STATE=$B/m48_load/state PASSES="$(seq 1 "$E" | tr '\n' ' ')" python3 tests/play_dumps.py m48_load.keys 60
    for T in $BAD; do
        rm -rf $B/$T
        d=$(damaged $T $B/m48_save/state/$N $B/$T/state)
        WALK=load$d STATE=$B/$T/state PASSES="2 3 10 100" python3 tests/play_dumps.py $T.keys 60
    done
    exit 0
fi
sh build.sh
r=0
for T in m48_save m48_save2 m48_load $BAD; do
    D=$B/$T
    [ -f "$D/passes.txt" ] || { echo "no dumps of the original: sh port/tests/save_test.sh dumps"; exit 1; }
    rm -rf "$D/saves"
    mkdir -p "$D/saves"
    case $T in
    m48_save | m48_save2) ;;
    m48_load) cp $B/m48_save/saves/$N "$D/saves/" ;;
    *) damaged $T $B/m48_save/saves/$N "$D/saves" > /dev/null ;;
    esac
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
    case $T in
    load_*)
        # deviation (win/origfixes): the port refuses a damaged file as
        # SAVED GAME NOT FOUND and stays in the menu; the original's
        # dumps are kept to show what it does instead (no comparison)
        if grep -q "0 times through the menu, 0 battles.*the menu waits for input" "$D/port.txt"; then
            echo "$T: refused, no battle begun"
        else
            echo "$T: the damaged file was not refused"
            r=1
        fi ;;
    *) python3 tests/play_cmp.py $T || r=1 ;;
    esac
    case $T in m48_save) F=$N ;; m48_save2) F="03.DAT 04.DAT" ;; *) F= ;; esac
    for f in $F; do
        if cmp "$D/state/$f" "$D/saves/$f"; then
            echo "the saved game $f: $(wc -c < "$D/saves/$f" | tr -d ' ') bytes, the same in both"
        else
            r=1
        fi
    done
    if [ $T = m48_save2 ]; then
        # the window's play, the digits four seconds after their D
        sed -e 's/^52.00 3/54.00 3/' -e 's/^62.00 d/60.00 d/' "$D/keys.txt" > "$D/keys_slow.txt"
        rm -rf "$D/live"
        mkdir -p "$D/live"
        DK_FRAMES=5600 DK_KEYS=$(python3 tests/live_keys.py "$D/keys_slow.txt" 0 0) \
            build/history-line-headless -game "$GAME" -saves "$D/live" > "$D/live.txt" 2>&1 || true
        if [ -f "$D/live/03.DAT" ] && [ -f "$D/live/04.DAT" ] && ! grep -q "no key came" "$D/live.txt"; then
            echo "the window's play: both saved, the battle went on"
        else
            echo "the window's play: $(tail -1 "$D/live.txt")"
            r=1
        fi
    fi
done
exit $r
