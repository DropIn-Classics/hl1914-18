#!/bin/sh
# Builds port/tests/sound_test.c and runs it with the arguments given (see
# there), or with the lists port/tests/sound_dumps.py wrote
# (build/sound/menu.args, build/sound/battle.args).
#
#     python3 port/tests/menu_dumps.py pages
#     python3 port/tests/sound_dumps.py [menu] [battle]
#     sh port/tests/sound_test.sh
#
# Without arguments it then builds the port's program, walks its menu with
# the keys of menu_dumps.py's walk `pages` and compares which effects it
# plays with the original's, and the same for the battle of play_test.sh
# m8_site on the original's clock (port/tests/sound_cmp.py).
set -e
cd "$(dirname "$0")/.."
CC=${CC:-cc}
RT=../doskit/runtime
mkdir -p build
$CC -O2 -Wall -Wextra -I$RT -Isrc -o build/sound_test tests/sound_test.c src/sound.c src/files.c src/names.c $RT/rmem.c $RT/sha256.c $RT/sys.c
if [ $# -eq 0 ]; then
    for f in ../build/sound/menu.args ../build/sound/battle.args; do
        [ -f "$f" ] || { echo "no $f: python3 port/tests/sound_dumps.py"; exit 1; }
    done
    build/sound_test $(cat ../build/sound/menu.args ../build/sound/battle.args) > ../build/sound/calls.out || { grep -v ' 0 different, elsewhere' ../build/sound/calls.out; exit 1; }
    tail -1 ../build/sound/calls.out
    sh build.sh
    D=../build/menu/pages
    build/history-line-headless -game "${DOSKIT_GAME:-../game}" -keys "$D/keys.txt" -start "$(cat "$D/start.txt")" -seed 0 \
        -until 66 -sndlog ../build/sound/port_menu.snd -saves ../build/sound > ../build/sound/port_menu.txt 2>&1 || true
    # the battle of port/tests/play_test.sh m8_site, on the original's clock
    C=../build/play/m8_site/clock.txt
    rm -f ../build/sound/port_battle.snd
    if [ -f "$C" ]; then
        build/history-line-headless -game "${DOSKIT_GAME:-../game}" -map 8 -keys tests/keys/m8_site.keys -clock "$C" \
            -sndlog ../build/sound/port_battle.snd -saves ../build/sound > ../build/sound/port_battle.txt 2>&1 || true
    else
        echo "no $C (python3 port/tests/play_dumps.py): the battle's effects are not compared"
    fi
    exec python3 tests/sound_cmp.py
fi
exec build/sound_test "$@"
