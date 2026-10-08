#!/bin/sh
# Builds the port's program and lets it begin at the menu (port/src/menu.c)
# with the key scripts of the walks port/tests/menu_dumps.py ran the
# original with (build/menu/WALK/, ignored), on the port's own clock from
# the second the original's first pass of the menu began, and compares
# what came in, the memory at the menu's last return and the video memory
# at the passes the original's was kept at (port/tests/menu_cmp.py).
#
#     python3 port/tests/menu_dumps.py [WALK...]
#     sh port/tests/menu_test.sh [WALK...]
#
# The video memory at each screen outside the menu's loop is compared too
# (-askvram, against askN.vram): the saved game's question and the text
# that it was not found (`noload`), text_input's beginning and end (the
# code typed, the winner's name), the best scores shown (by the menu's
# item in `pages`, after the battle in `end57` and `live57`).
# The walk `exit` ends the program from the menu (EXIT TO DOS): the memory
# at the program's end is compared too (-ram, against end.ram).
# The walk `back` goes through the menu, into a battle, out of it by Esc
# and the confirming key, and through the menu again: the memory compared
# is that of the second return.
# The walk `end57` plays a battle to its end between (m57_win's keys and
# pokes, pokes.txt): the memory and video memory at the end's waits and
# frames are compared too (-evdump, against eN and fNNNN).
# With `live` first (sh port/tests/menu_test.sh live WALK...) each walk is
# also played in the window (play_live, headless: the keys given to the
# window's pictures at their times, tests/live_keys.py), from the menu on:
# what its -keylog writes must be the walk's key script again (a walk
# whose script a -keylog wrote, live57), and its
# memory and video memory where the -keys play's were dumped the same.
set -e
# awk reads the clock's times as numbers only with a decimal point
export LC_ALL=C
cd "$(dirname "$0")/.."
GAME=${DOSKIT_GAME:-../game}
sh build.sh
LIVE=
[ "$1" = live ] && { LIVE=1; shift; }
[ $# -gt 0 ] || set -- back code57 end57 exit live57 noload pages start0 start24
r=0
for w in "$@"; do
    D=../build/menu/$w
    [ -f "$D/exit.ram" ] || { echo "no dump of the original: python3 port/tests/menu_dumps.py $w"; exit 1; }
    n=$(grep -c ' 0$' "$D/acts.txt")
    until=$(awk '{ t = $1 } END { print t + 3 }' "$D/acts.txt")
    rm -f "$D/port.ram" "$D/port_end.ram" "$D/port.log" "$D"/port*.vram
    V=
    for t in $(cat "$D/shots.txt" 2>/dev/null); do V="$V -menuvram $t $D/port$t.vram"; done
    k=1
    while [ $k -le "$(cat "$D/asks.txt" 2>/dev/null || echo 0)" ]; do V="$V -askvram $k $D/portask$k.vram"; k=$((k + 1)); done
    # what the port writes (a best score) goes to a folder of its own,
    # emptied first, as the original's does
    mkdir -p "$D/port_saves"
    find "$D/port_saves" -type f -delete
    [ -f "$D/pokes.txt" ] && V="$V $(cat "$D/pokes.txt") -evdump $D/port_"
    # shellcheck disable=SC2086
    build/history-line-headless -game "$GAME" -keys "$D/keys.txt" -start "$(cat "$D/start.txt")" -seed 0 \
        -until "$until" -menulog "$D/port.log" -menudump "$n" "$D/port.ram" $V -saves "$D/port_saves" -ram "$D/port_end.ram" > "$D/port.txt" 2>&1 || true
    tail -1 "$D/port.txt"
    python3 tests/menu_cmp.py "$w" || r=1
    [ -n "$LIVE" ] || continue
    # the window: pictures every 0.014268 s from 0 on, the menu's first
    # among them, as long as the walk lasts
    mkdir -p "$D/window_saves"
    find "$D/window_saves" -type f -delete
    find "$D" -maxdepth 1 -name 'window*' -type f -delete
    W=$(echo "$V" | sed 's/port/window/g')
    # shellcheck disable=SC2086
    DK_KEYS=$(python3 tests/live_keys.py "$D/keys.txt" 0 0) DK_FRAMES=$(awk -v u="$until" 'BEGIN { print int(u / 0.014268) }') \
        build/history-line-headless -game "$GAME" -start "$(cat "$D/start.txt")" -seed 0 -keylog "$D/window.keys" \
        -menulog "$D/window.log" -menudump "$n" "$D/window.ram" $W -saves "$D/window_saves" > "$D/window.txt" 2>&1 || true
    if cmp -s "$D/window.keys" "$D/keys.txt"; then
        echo "  live: the window's -keylog is the walk's key script again"
    else
        echo "  live: the window's -keylog is NOT the walk's key script"; r=1
    fi
    for f in "$D"/port.ram "$D"/port[0-9]*.vram "$D"/portask[0-9]*.vram "$D"/port_[ef][0-9]*.*; do
        [ -f "$f" ] || continue
        g=$D/window${f##*/port}
        if [ ! -f "$g" ]; then
            echo "  live: ${f##*/}: the window did not come so far"; r=1
        elif cmp -s "$f" "$g"; then
            echo "  live: ${f##*/}: the same"
        else
            echo "  live: ${f##*/}: DIFFERENT"; r=1
        fi
    done
done
exit $r
