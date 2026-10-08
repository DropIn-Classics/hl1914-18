#!/bin/sh
# The port's rendered AdLib sound beside the original's as the runner's
# chip played it (-oplwav), and what the 70 registers are that a battle
# writes at another place with an audio device than without.  Nothing is
# listened to.
#
#     python3 port/tests/menu_dumps.py pages
#     python3 port/tests/briefing_dumps.py m4
#     python3 port/tests/oplwav_dumps.py [menu] [brief] [battle]
#     sh port/tests/oplwav_test.sh
#
# For each walk oplwav_dumps.py has run (build/oplwav/WALK.orig):
# - tests/oplwav_ref.c fit: the runner's file is the sound of its log's
#   writes at their moments, sample for sample;
# - the port's headless program goes the walk with DK_WAV, -oplat and
#   -opllog; tests/soundout_ref.c: its file is the sound of its list;
# - tests/oplwav_cmp.py: the two tune by tune (the writes by ticks, the
#   ticks' times, the levels).
# Then the first 120 s of map 8 (tests/keys/m8_site.keys) without a
# device, with one, and without one but the interrupts counted as with
# one (-oplcount) on clocks of several steps:
# - judged: counted at 44100 Hz in steps of a millisecond, the log is the
#   device's line for line (the count is what the device does);
# - judged: counted in steps of 100 microseconds, of one, and at the
#   timer's own rate without steps, the registers are in the order of the
#   run without a device (so the other places are the millisecond's:
#   the device's clock is up to a millisecond behind the port's, and a
#   tick due in it comes behind a call of the game's side, not before);
# - judged: where the device's log differs from the one without, its
#   count of ticks is the same or one less, never more;
# - told: how many registers are at another place, and how many lines'
#   counts of ticks differ in each run.
set -e
cd "$(dirname "$0")/.."
CC=${CC:-cc}
RT=../doskit/runtime
GAME=${DOSKIT_GAME:-../game}
O=../build/oplwav
mkdir -p build "$O"
sh build.sh
$CC -O2 -Wall -Wextra -I$RT -o build/oplwav_ref tests/oplwav_ref.c $RT/opl.c -lm
$CC -O2 -Wall -Wextra -I$RT -o build/soundout_ref tests/soundout_ref.c $RT/opl.c -lm
bad=0
walk() {
    w=$1
    shift
    if [ ! -f "$O/$w.orig" ]; then
        echo "no $O/$w.orig (python3 port/tests/oplwav_dumps.py $w): the walk left out"
        return
    fi
    build/oplwav_ref fit "$O/$w/orig.wav" "$O/$w.orig" || bad=1
    DK_WAV="$O/$w.port.wav" build/history-line-headless -game "$GAME" "$@" -oplat "$O/$w.port.at" \
        -opllog "$O/$w.port.opl" -saves "$O" > "$O/$w.port.txt" 2>&1 || true
    grep '^Audio:' "$O/$w.port.txt" || { echo "no audio device in the run ($O/$w.port.txt)"; bad=1; return; }
    build/soundout_ref "$O/$w.port.at" "$O/$w.port.wav" || bad=1
    python3 tests/oplwav_cmp.py "$w" || bad=1
}
D=../build/menu/pages
[ -f "$D/keys.txt" ] && walk menu -keys "$D/keys.txt" -start "$(cat "$D/start.txt")" -seed 0 -until 66
D=../build/briefing/m4
[ -f "$D/keys.txt" ] && walk brief -keys "$D/keys.txt" -start "$(cat "$D/start.txt")" -seed 0 -until 60 -passes 1
walk battle -map 8 -keys tests/keys/m8_site.keys -seed 0 -until 120

m8() {
    n=$1
    shift
    build/history-line-headless -game "$GAME" -map 8 -keys tests/keys/m8_site.keys -seed 0 -until 120 \
        -saves "$O" -opllog "$O/m8.$n.opl" "$@" > "$O/m8.$n.txt" 2>&1 || true
}
DK_WAV= DK_AUDIO= m8 none
DK_WAV="$O/m8.dev.wav" m8 dev
DK_WAV= DK_AUDIO= m8 c44100_1000 -oplcount 44100 1000
DK_WAV= DK_AUDIO= m8 c44100_100 -oplcount 44100 100
DK_WAV= DK_AUDIO= m8 c44100_1 -oplcount 44100 1
DK_WAV= DK_AUDIO= m8 cpit_0 -oplcount 1193182 0
python3 - "$O" <<'PY' || bad=1
import sys
def rd(n):
    return [l.split() for l in open('%s/m8.%s.opl' % (sys.argv[1], n)) if l[0].isdigit()]
none, dev = rd('none'), rd('dev')
bad = 0
moved = [i for i, (x, y) in enumerate(zip(dev, none)) if x[2:4] != y[2:4]]
ticks = sorted({int(x[1]) - int(y[1]) for x, y in zip(dev, none) if x != y})
print('map 8, %d registers: with the device %d at another place than without; where the lines differ, the '
      'device\'s count of ticks less the other\'s is %s' % (len(none), len(moved), ticks))
if len(dev) != len(none) or sorted(x[2:4] for x in dev) != sorted(x[2:4] for x in none) or [t for t in ticks if t not in (0, -1)]:
    print('FAILED: other registers, or the device\'s ticks ahead')
    bad = 1
if rd('c44100_1000') != dev:
    print('FAILED: counted at 44100 Hz in steps of 1000 microseconds is not the device\'s log')
    bad = 1
else:
    print('counted at 44100 Hz in steps of 1000 microseconds: the device\'s log line for line')
for n, what in (('c44100_100', '44100 Hz in steps of 100 microseconds'), ('c44100_1', '44100 Hz in steps of 1 microsecond'),
                ('cpit_0', '1193182 Hz without steps')):
    x = rd(n)
    m = sum(1 for p, q in zip(x, none) if p[2:4] != q[2:4])
    print('counted at %s: %d registers at another place than without a device, %d lines with another count of '
          'ticks or time' % (what, m, sum(1 for p, q in zip(x, none) if p != q)))
    if m or len(x) != len(none):
        print('FAILED: the order is not the one without a device')
        bad = 1
sys.exit(bad)
PY
[ $bad = 0 ] || { echo "the comparison with the original's sound is not as expected ($O)"; exit 1; }
echo "oplwav: all as expected"
