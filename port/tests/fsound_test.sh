#!/bin/sh
# The fight's sound triggers of the port (port/src/sound.c: pics_play and
# the sites that call it, the channels stopped at the pictures' end,
# attacks_run's effect for a unit destroyed without pictures) against the
# original.
#
#     python3 port/tests/play_dumps.py TAG.keys     (the original's clock)
#     python3 port/tests/fsound_dumps.py [TAG ...]
#     sh port/tests/fsound_test.sh [TAG ...]
#
# TAG as in fsound_dumps.py (default m8_site m8_d5r m8_wait_nopics).  For
# each: the calls kept there (build/fsound/TAG.args) through
# port/tests/sound_test.c, memory at each call's entry and end; then the
# port's program plays the key script on the original's clock with -fxlog
# and port/tests/fsound_cmp.py compares which triggers came, in which
# frame, with what.
set -e
cd "$(dirname "$0")/.."
CC=${CC:-cc}
RT=../doskit/runtime
[ $# -gt 0 ] || set -- m8_site m8_d5r m8_wait_nopics
mkdir -p build
$CC -O2 -Wall -Wextra -I$RT -Isrc -o build/sound_test tests/sound_test.c src/sound.c src/files.c src/names.c $RT/rmem.c $RT/sha256.c $RT/sys.c
sh build.sh
bad=0
for TAG in "$@"; do
    F=../build/fsound
    [ -f "$F/$TAG.args" ] || { echo "no $F/$TAG.args: python3 port/tests/fsound_dumps.py $TAG"; exit 1; }
    if [ -s "$F/$TAG.args" ]; then
        build/sound_test $(cat "$F/$TAG.args") > "$F/$TAG.out" || { grep -v ' 0 different, elsewhere' "$F/$TAG.out"; bad=1; }
        echo "$TAG: $(tail -1 "$F/$TAG.out")"
    fi
    C=../build/play/$TAG/clock.txt
    [ -f "$C" ] || { echo "no $C: python3 port/tests/play_dumps.py"; exit 1; }
    KEYS=${TAG%_nopics}
    NOPICS=
    [ "$KEYS" = "$TAG" ] || NOPICS=-nopics
    build/history-line-headless -game "${DOSKIT_GAME:-../game}" -map 8 -keys "tests/keys/$KEYS.keys" -clock "$C" $NOPICS \
        $(python3 tests/play_pokes.py "$KEYS" port) -fxlog "$F/port_$TAG.fx" -saves "$F" > "$F/port_$TAG.txt" 2>&1 || true
done
python3 tests/fsound_cmp.py "$@" || bad=1
exit $bad
