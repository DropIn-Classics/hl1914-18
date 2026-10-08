#!/bin/sh
# The port's AdLib output rendered headless: the register writes through
# the runtime's queue (doskit/runtime/oplq.c) on plat_null.c's virtual
# audio device, the timer's interrupts counted by the samples played
# (port/src/sound.c, port/src/main.c).  Nothing is listened to.
#
#     sh port/tests/soundout_test.sh
#
# Two walks, each run twice with DK_WAV: the first 120 s of map 8 with
# tests/keys/m8_site.keys, and the menu's walk when menu_dumps.py pages
# has written its keys (build/menu/pages).  For each:
# - the two runs' WAVE files and lists (-oplat) are the same byte for byte;
# - no write was late or early in the queue (the program's last line);
# - tests/soundout_ref.c: the interrupts' samples are the timer's
#   periods summed, an interrupt's writes are at its sample plus the
#   lead, and every frame of the file is the one of a second chip that
#   got each write exactly at its sample;
# - tests/soundout_cmp.py: the writes' samples beside the port's own
#   clock (-opllog);
# - told, not judged: whether a run without a device (the interrupts on
#   the port's clock of passes) writes the same registers in the same
#   order.  It need not: the device's clock goes in whole milliseconds,
#   so an interrupt can fall on the other side of a call of the game's.
set -e
cd "$(dirname "$0")/.."
CC=${CC:-cc}
RT=../doskit/runtime
GAME=${DOSKIT_GAME:-../game}
O=../build/soundout
mkdir -p build "$O"
sh build.sh
$CC -O2 -Wall -Wextra -I$RT -o build/soundout_ref tests/soundout_ref.c $RT/opl.c -lm
bad=0
walk() {
    w=$1
    shift
    for r in a b; do
        DK_WAV="$O/$w.$r.wav" build/history-line-headless -game "$GAME" "$@" -oplat "$O/$w.$r.at" \
            -opllog "$O/$w.$r.opl" -saves "$O" > "$O/$w.$r.txt" 2>&1 || true
    done
    DK_WAV= DK_AUDIO= build/history-line-headless -game "$GAME" "$@" -opllog "$O/$w.none.opl" -saves "$O" \
        > "$O/$w.none.txt" 2>&1 || true
    echo "$w:"
    grep '^Audio:' "$O/$w.a.txt" || { echo "no audio device in the run ($O/$w.a.txt)"; bad=1; return; }
    grep -q '^Audio:.* 0 late, 0 early' "$O/$w.a.txt" || { echo "late or early writes"; bad=1; }
    if cmp -s "$O/$w.a.wav" "$O/$w.b.wav" && cmp -s "$O/$w.a.at" "$O/$w.b.at"; then
        echo "two runs: the same WAVE file ($(wc -c < "$O/$w.a.wav" | tr -d ' ') bytes) and the same list"
    else
        echo "two runs differ"
        bad=1
    fi
    build/soundout_ref "$O/$w.a.at" "$O/$w.a.wav" || bad=1
    python3 tests/soundout_cmp.py "$O/$w.a.at" "$O/$w.a.opl" || bad=1
    python3 - "$O/$w.a.opl" "$O/$w.none.opl" <<'PY'
import sys
a, b = ([l.split()[2:4] for l in open(p) if l[0].isdigit()] for p in sys.argv[1:3])
same = sum(1 for x, y in zip(a, b) if x == y)
print(f'without a device: {len(b)} registers, ' + ('the same in the same order' if a == b else
      f'{same} the same at the same place' + (', the same registers in another order' if sorted(a) == sorted(b) else '')))
PY
}
walk m8 -map 8 -keys tests/keys/m8_site.keys -seed 0 -until 120
D=../build/menu/pages
if [ -f "$D/keys.txt" ]; then
    walk menu -keys "$D/keys.txt" -start "$(cat "$D/start.txt")" -seed 0 -until 66
else
    echo "no $D/keys.txt (python3 port/tests/menu_dumps.py pages): the menu's walk left out"
fi
[ $bad = 0 ] || { echo "the rendered sound is not as expected ($O)"; exit 1; }
echo "soundout: all as expected"
