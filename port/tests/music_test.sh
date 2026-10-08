#!/bin/sh
# The port's music control (port/src/sound.c: music_start, music_fade,
# music_volume, fade_tick, and the timer's routine while a tune plays)
# against the original.
#
#     python3 port/tests/menu_dumps.py pages
#     python3 port/tests/briefing_dumps.py m4
#     python3 port/tests/play_dumps.py m8_site      (the battle's clock)
#     python3 port/tests/music_dumps.py [menu] [brief] [battle]
#     sh port/tests/music_test.sh
#
# First port/tests/sound_test.c on the lists music_dumps.py wrote
# (build/music/WALK.args): each call from the original's memory at its
# entry, the driver's variables and the result compared with its end.
# Then the port's program goes the three walks and which calls of
# music_start and music_fade it makes is compared with the original's
# (port/tests/music_cmp.py).
set -e
cd "$(dirname "$0")/.."
CC=${CC:-cc}
RT=../doskit/runtime
GAME=${DOSKIT_GAME:-../game}
M=../build/music
mkdir -p build
$CC -O2 -Wall -Wextra -I$RT -Isrc -o build/sound_test tests/sound_test.c src/sound.c src/files.c src/names.c $RT/rmem.c $RT/sha256.c $RT/sys.c
bad=0
for w in menu brief battle; do
    [ -f "$M/$w.args" ] || { echo "no $M/$w.args: python3 port/tests/music_dumps.py $w"; exit 1; }
    build/sound_test $(cat "$M/$w.args") > "$M/$w.out" || { grep -v ' S30EF 0 different' "$M/$w.out"; bad=1; }
    echo "$w: $(tail -1 "$M/$w.out")"
done
sh build.sh
D=../build/menu/pages
build/history-line-headless -game "$GAME" -keys "$D/keys.txt" -start "$(cat "$D/start.txt")" -seed 0 \
    -until 66 -muslog "$M/port_menu.mus" -saves "$M" > "$M/port_menu.txt" 2>&1 || true
D=../build/briefing/m4
build/history-line-headless -game "$GAME" -keys "$D/keys.txt" -start "$(cat "$D/start.txt")" -seed 0 \
    -until 60 -passes 1 -muslog "$M/port_brief.mus" -saves "$M" > "$M/port_brief.txt" 2>&1 || true
C=../build/play/m8_site/clock.txt
[ -f "$C" ] || { echo "no $C: python3 port/tests/play_dumps.py m8_site"; exit 1; }
build/history-line-headless -game "$GAME" -map 8 -keys tests/keys/m8_site.keys -clock "$C" \
    -muslog "$M/port_battle.mus" -saves "$M" > "$M/port_battle.txt" 2>&1 || true
python3 tests/music_cmp.py || bad=1
exit $bad
