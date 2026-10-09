#!/bin/sh
# A battle's end on both clocks: the battle played on the original's clock
# inside a walk of the menu, and the pictures D0h and CFh on the port's
# own clock (port/src/play.c, port/src/events.c).
#
#     sh port/tests/endclock_test.sh dumps     the original's runs (build/play/TAG/)
#     sh port/tests/endclock_test.sh           the port's, and the comparison
#
#   m57_menu   map 57's code typed in the menu by keys, START, m57_win's
#              battle (its keys 8 s earlier, its pokes), the best score's
#              name ("A", Enter), the best scores left; the port walks
#              its menu on its own clock and plays the battle on the
#              original's (-clock), then walks the menu on on its own
#              clock from the original's time at S066A:4728.  Compared:
#              every pass dumped, both waits and the frames of CDh
#              (play_cmp.py), and the scores' file both write.
#   m57_won    no keys; a poke ends the battle with its first pass and
#              picture D0h follows (m57_won.pokes)
#   m57_medal  the same with a round and a score poked, so that a best
#              score follows: D0h and CFh (m57_medal.pokes)
#   m54_hq     no pokes: m54_end's battle by play, its keys made for both
#              clocks (make_two.py, steps of 0.06 s, gestures of 0.03 s):
#              the second side takes the first's headquarters in round
#              14, D0h and CFh
#   m57_hq     no pokes: the first side's cavalry walks into the second
#              side's headquarters in round 15, D0h and CEh
#   m57_low    m57_hq's keys with a map's scores file in place whose
#              lowest score is the battle's own (320): battle_score finds
#              the score not above the best and shows no CEh (D0h only)
#   m60_first  m60_fights' walk, then the two cavalries fight six times,
#              the first side attacking in the third and fourth (an idle
#              round between, as a side attacks every other round): the
#              second side is left without units, CDh with 1 and CEh
#   m60_both   m60_fights' walk, three fights where they stand (3/3),
#              then the first side's cavalry attacks from (44,19), the
#              ground whose class lets it take a figure more, twice
#              (2/2, 1/1) and from (45,19) once more: both destroyed in
#              the same fight, 15h for both views, no picture behind
#              the loop.  Both map-60 scripts without fight pictures
#              (NOPICS=1, the tags _nopics) as m60_fights was.
#              All seven played by play_test.sh on the original's clock and on
#              the port's own (`rounds`: memory and video memory at every
#              tenth frame), and the BIOS's count of ticks at each such
#              frame and at S066A:4728 set beside the original's, counted
#              from the first pass: the pictures' time on the own clock.
set -e
# the map 57 scores file of m57_low: four scores, the lowest 320 (what
# m57_hq's battle scores), and four names; made here, not game data
hi() {
    python3 -c "import struct, sys; open(sys.argv[1], 'wb').write(b''.join(struct.pack('<i', s) for s in (620, 520, 420, 320)) + b''.join(n.ljust(6, b'\\0') for n in (b'LOWA', b'LOWB', b'LOWC', b'LOWD')))" "$1"
}
# awk reads the clock's times as numbers only with a decimal point
export LC_ALL=C
cd "$(dirname "$0")/.."
B=../build/play
GAME=${DOSKIT_GAME:-../game}
if [ "$1" = dumps ]; then
    rm -rf $B/m57_menu $B/m57_won $B/m57_medal
    # to 95 s: the original's second START (94 s) begins a battle at 97.9 s
    WALK=code STATE=$B/m57_menu/state VRAM=1 python3 tests/play_dumps.py m57_menu.keys 95
    VRAM=1 python3 tests/play_dumps.py m57_won.keys 70
    VRAM=1 python3 tests/play_dumps.py m57_medal.keys 90
    rm -rf $B/m54_hq $B/m57_hq
    VRAM=1 python3 tests/play_dumps.py m54_hq.keys 350
    VRAM=1 python3 tests/play_dumps.py m57_hq.keys 360
    rm -rf $B/m57_low $B/m60_first_nopics $B/m60_both_nopics
    mkdir -p $B/m57_low/state/MAP
    hi $B/m57_low/state/MAP/57.HI
    STATE=$B/m57_low/state VRAM=1 python3 tests/play_dumps.py m57_low.keys 360
    NOPICS=1 VRAM=1 python3 tests/play_dumps.py m60_first.keys 1640
    NOPICS=1 VRAM=1 python3 tests/play_dumps.py m60_both.keys 1700
    exit 0
fi
sh build.sh
r=0
D=$B/m57_menu
[ -f "$D/passes.txt" ] || { echo "no dumps of the original: sh port/tests/endclock_test.sh dumps"; exit 1; }
rm -rf "$D/saves"
mkdir -p "$D/saves"
rm -f "$D"/q[0-9][0-9][0-9][0-9].* "$D"/q[ef][0-9]*.*
DUMPS=
for p in $(cat "$D/passes.txt"); do
    DUMPS="$DUMPS -dump $p $D/q$(printf %04d "$p").ram"
    [ -f "$D/p$(printf %04d "$p").vram" ] && DUMPS="$DUMPS -dump $p $D/q$(printf %04d "$p").vram"
done
# shellcheck disable=SC2086
build/history-line-headless -game "$GAME" -keys "$D/keys.txt" -start "$(cat "$D/start.txt")" -seed 0 \
    -clock "$D/clock.txt" -saves "$D/saves" -inputs "$D/port_inputs.txt" -ends "$D/q_ends.txt" -evdump "$D/q" \
    $(python3 tests/play_pokes.py m57_menu port) $DUMPS -until 95 > "$D/port.txt" 2>&1 || true
tail -1 "$D/port.txt"
python3 tests/play_cmp.py m57_menu || r=1
if cmp -s "$D/state/MAP/04.HI" "$D/saves/MAP_04.HI"; then
    echo "m57_menu: the scores' file the same"
else
    # the name's cursor: text_input writes `]` behind the letters every
    # 17th pass, and the menu's passes are the port's own (HANDOFF)
    echo "m57_menu: the scores' file differs (the name's cursor, see docs/HANDOFF.md)"
fi
mkdir -p $B/m57_low/saves $B/m57_low/tt
hi $B/m57_low/saves/MAP_57.HI
hi $B/m57_low/tt/MAP_57.HI
for T in m57_won m57_medal m54_hq m57_hq m57_low m60_first_nopics m60_both_nopics; do
    if ! sh tests/play_test.sh $T > "$B/$T/test.txt" 2>&1; then
        # map 60's walk is m60_fights': its one input part that differs
        # (pass 4748, the second side's bytes 5 and 8, port/README.md) and
        # nothing else is accepted
        case $T in
        m60_*) grep -q '^inputs: .* 1 different' "$B/$T/test.txt" &&
               grep -q 'original "4748 1 0 10 -1 0", port "4748 1 0 10 0 255"' "$B/$T/test.txt" &&
               ! grep -Eq "state [1-9][0-9]* different|computer's [1-9]|DIFFERENT\)|, DIFFERENT" "$B/$T/test.txt" || r=1 ;;
        *) r=1 ;;
        esac
    fi
    echo "$T: $(grep -v '^$' "$B/$T/test.txt" | tail -1)"
    sh tests/play_test.sh $T rounds > "$B/$T/rounds_test.txt" 2>&1 || r=1
    echo "$T rounds: video memory at $(grep -c "^frame.*video memory 0 bytes different" "$B/$T/rounds_test.txt") of $(grep -c '^frame' "$B/$T/rounds_test.txt") frames the same"
    # the BIOS's ticks from the first pass to each tenth frame and to
    # S066A:4728 (two passes after the last), the original's and the port's
    START=$(awk '$2 == "P" { print $1; exit }' "$B/$T/clock.txt")
    SEED=$(awk '$2 == "S" { v = $3 } $2 == "P" { print substr(v, 1, length(v) - 4); exit }' "$B/$T/clock.txt")
    mkdir -p "$B/$T/tt"
    M=${T%%_*} M=${M#m}
    K=${T%_nopics} NP=
    [ "$K" = "$T" ] || NP=-nopics
    [ -f "$B/$T/f0001.ram" ] || { echo "  no picture behind the loop"; continue; }
    RUN="build/history-line-headless -game $GAME -map $M -keys tests/keys/$K.keys $NP -start $START -seed 0x$SEED $(python3 tests/play_pokes.py $K port) -saves $B/$T/tt"
    # shellcheck disable=SC2086
    N=$($RUN 2>&1 | awk '/passes played/ { print $1 + 2 }')
    # shellcheck disable=SC2086
    $RUN -dump 1 "$B/$T/tt/a.ram" -dump "$N" "$B/$T/tt/c.ram" > /dev/null 2>&1 || true
    LAST=$(tr ' ' '\n' < "$B/$T/passes.txt" | sort -n | tail -1)
    # m60_first: from CDh's frame 121 to CEh's first the waits go by
    # retraces, and the runner's timer ran slow there (286 calls in 4.21 s
    # of its clock): its BIOS count falls 7 ticks behind, the port's
    # stretch is 0.066 s longer by the runner's clock (docs/HANDOFF.md)
    TOL=3
    [ "$T" = m60_first_nopics ] && TOL=7
    python3 - "$B/$T" "$LAST" "$TOL" <<'EOF' || r=1
import os, struct, sys
d = sys.argv[1]
def ticks(f):
    return struct.unpack('<I', open(os.path.join(d, f), 'rb').read()[0x46C:0x470])[0]
a0, b0 = ticks('p0001.ram'), ticks('r0001.ram')
# the battle's own drift (the passes' phase, docs/HANDOFF.md) apart: the
# pictures counted from their first frame
a1, b1 = ticks('f0001.ram') - a0, ticks('rf0001.ram') - b0
worst = 0
for n in range(1, 1000, 10):
    if not os.path.exists(os.path.join(d, 'f%04d.ram' % n)):
        break
    worst = max(worst, abs((ticks('rf%04d.ram' % n) - b0 - b1) - (ticks('f%04d.ram' % n) - a0 - a1)))
end = (ticks('tt/c.ram') - ticks('tt/a.ram') - b1) - (ticks('p%04d.ram' % int(sys.argv[2])) - a0 - a1)
print('  the first frame %+d ticks from the first pass; from it the frames at most %d ticks apart, S066A:4728 %+d' % (b1 - a1, worst, end))
tol = int(sys.argv[3])
sys.exit(0 if worst <= tol and abs(end) <= tol else 1)
EOF
done
exit $r
