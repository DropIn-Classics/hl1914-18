#!/bin/sh
# The port's AdLib output (port/src/sound.c: the registers handed to
# snd_opl, and the timer's count that says which interrupt runs the
# fade's routine and adl_tick) against the original's.
#
#     python3 port/tests/menu_dumps.py pages
#     python3 port/tests/briefing_dumps.py m4
#     python3 port/tests/opl_dumps.py [menu] [brief] [battle]
#     sh port/tests/opl_test.sh
#
# First port/tests/opl_test.c on the lists opl_dumps.py wrote
# (build/opl/WALK.ev): the original's calls and interrupts made again in
# their order, every register written compared with the original's next
# one and the driver's variables with its memory at each dump.  The walks
# menu and brief must be there, battle is taken when it is.
# Then the port's program goes the walks menu and brief with -opllog and
# what it writes on its own clock is set beside the original's
# (port/tests/opl_cmp.py; told, not judged).
set -e
cd "$(dirname "$0")/.."
CC=${CC:-cc}
RT=../doskit/runtime
GAME=${DOSKIT_GAME:-../game}
O=../build/opl
mkdir -p build
$CC -O2 -Wall -Wextra -I$RT -Isrc -o build/opl_test tests/opl_test.c src/sound.c src/files.c src/names.c $RT/rmem.c $RT/sha256.c $RT/sys.c
bad=0
for w in menu brief battle; do
    if [ ! -f "$O/$w.ev" ]; then
        echo "no $O/$w.ev: python3 port/tests/opl_dumps.py $w"
        [ "$w" = battle ] || bad=1
        continue
    fi
    build/opl_test "$O/$w.ev" > "$O/$w.out" || bad=1
    echo "$w:"
    tail -5 "$O/$w.out"
done
[ $bad = 0 ] || { echo "the port's AdLib output is not the original's (build/opl/WALK.out)"; exit 1; }
sh build.sh
D=../build/menu/pages
build/history-line-headless -game "$GAME" -keys "$D/keys.txt" -start "$(cat "$D/start.txt")" -seed 0 \
    -until 66 -opllog "$O/port_menu.opl" -saves "$O" > "$O/port_menu.txt" 2>&1 || true
D=../build/briefing/m4
build/history-line-headless -game "$GAME" -keys "$D/keys.txt" -start "$(cat "$D/start.txt")" -seed 0 \
    -until 60 -passes 1 -opllog "$O/port_brief.opl" -saves "$O" > "$O/port_brief.txt" 2>&1 || true
python3 tests/opl_cmp.py menu brief
