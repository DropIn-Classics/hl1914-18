#!/bin/sh
# sfx_play's ways through the port's sound routines (port/src/sound.c: an
# 8SVX effect, an effect played more than once or endless, a call that
# fails) against the original, in the picture of a side left without
# units (pic_wiped, port/src/events.c).
#
#     python3 port/tests/play_dumps.py m57_win.keys 84   (the original's clock)
#     python3 port/tests/sfx_dumps.py [VARIANT ...]
#     sh port/tests/sfx_test.sh [VARIANT ...]
#
# VARIANT as in sfx_dumps.py (default all of them).  For each: the calls
# kept there (build/sfx/VARIANT.args) through port/tests/sound_test.c,
# memory at each call's entry and end.  Then, for the variants the port's
# program can play (dig, digx, adl, adlx), it plays m57_win.keys with the
# variant's pokes on the original's clock and on its own with -sndlog, and
# port/tests/sfx_cmp.py compares which effects the picture plays, on which
# channel, how loud, how many times and with what result.
set -e
cd "$(dirname "$0")/.."
CC=${CC:-cc}
RT=../doskit/runtime
[ $# -gt 0 ] || set -- dig digx adl adlx chan voices lock
mkdir -p build
$CC -O2 -Wall -Wextra -I$RT -Isrc -o build/sound_test tests/sound_test.c src/sound.c src/files.c src/names.c $RT/rmem.c $RT/sha256.c $RT/sys.c
sh build.sh
F=../build/sfx
D=../build/play/m57_win
GAME=${DOSKIT_GAME:-../game}
bad=0
PLAYED=
for V in "$@"; do
    [ -f "$F/$V.args" ] || { echo "no $F/$V.args: python3 port/tests/sfx_dumps.py $V"; exit 1; }
    # shellcheck disable=SC2046
    build/sound_test $(cat "$F/$V.args") > "$F/$V.out" || { grep -v ' 0 different, elsewhere' "$F/$V.out"; bad=1; }
    echo "$V: $(tail -1 "$F/$V.out")"
    case $V in
    dig|digx|adl|adlx) ;;
    *) continue ;;
    esac
    PLAYED="$PLAYED $V"
    [ -f "$D/clock.txt" ] || { echo "no $D/clock.txt: python3 port/tests/play_dumps.py m57_win.keys 84"; exit 1; }
    START=$(awk '$2 == "P" { print $1; exit }' "$D/clock.txt")
    SEED=$(awk '$2 == "S" { v = $3 } $2 == "P" { print substr(v, 1, length(v) - 4); exit }' "$D/clock.txt")
    UNTIL=$(awk 'END { print $1 }' "$D/clock.txt")
    # shellcheck disable=SC2046
    build/history-line-headless -game "$GAME" -map 57 -keys tests/keys/m57_win.keys -clock "$D/clock.txt" \
        $(cat "$F/$V.pokes") -sndlog "$F/port_$V.snd" -saves "$F" > "$F/port_$V.txt" 2>&1 || true
    # shellcheck disable=SC2046
    build/history-line-headless -game "$GAME" -map 57 -keys tests/keys/m57_win.keys -start "$START" -seed "0x$SEED" \
        -until "$UNTIL" $(cat "$F/$V.pokes") -sndlog "$F/own_$V.snd" -saves "$F" > "$F/own_$V.txt" 2>&1 || true
done
# shellcheck disable=SC2086
[ -z "$PLAYED" ] || python3 tests/sfx_cmp.py $PLAYED || bad=1
exit $bad
