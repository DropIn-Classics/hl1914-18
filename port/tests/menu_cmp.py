#!/usr/bin/env python3
"""Compares the port's walk through the menu with the original's
(port/tests/menu_test.sh calls it).

    python3 port/tests/menu_cmp.py WALK

In build/menu/WALK/: acts.txt and exit.ram are the original's
(menu_dumps.py), port.log and port.ram the port's (-menulog, -menudump).
Compared:

  * what came in, in order (1 chosen, 2 the other page, 5 another item,
    0 the menu's return), and how far the times are apart at most (the
    port draws nothing, so its passes are not the original's: the times
    are told, not judged);
  * memory at the menu's last return: the game's state (S331F; but for
    the count of a battle's passes, the dword at S331F:4CB7, which is
    told apart: the port's passes are shorter, it draws less), the
    menu's own segment (S37FF: the libraries' records, the choices kept,
    the code), the mouse's record (DATA:0414 to 045B), and the block of
    445C0h bytes game_loop allocates from 40h bytes behind its beginning
    for 40000h bytes (the first 40h hold what start_setup's part that is
    not translated leaves in the first file): the menu's three pictures,
    CODES.DAT, the six libraries, the tunes and sounds and the menu's
    data with the items' rectangles and choices.

  * video memory at the passes menu_dumps.py kept the original's at: the
    page drawn and the page shown (with the mouse's pointer); and at
    each screen of the saved game's question (askN.vram, the port's
    -askvram).

Exit status 0 when the order is the same and no byte differs.
"""
import os, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
walk = sys.argv[1]
out = os.path.join(ROOT, 'build', 'menu', walk)
LOAD = 0x76             # where the runner and the port load HL.EXE
BLOCK = 0x40A60         # game_loop's block in both (its MCB follows the program's)


def acts(name):
    rows = []
    for line in open(os.path.join(out, name)):
        f = line.split()
        if len(f) >= 2:
            rows.append((float(f[0]), int(f[1])))
    return rows


a, b = acts('acts.txt'), acts('port.log')
bad = 0
if [k for _, k in a] != [k for _, k in b]:
    bad += 1
    print('%s: what came in differs: original %s, port %s' % (walk, [k for _, k in a], [k for _, k in b]))
else:
    far = max([abs(x[0] - y[0]) for x, y in zip(a, b)] or [0])
    print('%s: %d things came in, the same in both; times at most %.2f s apart' % (walk, len(a), far))

def memory(what, x, y):
    """the regions of two dumps compared; how many of them differ"""
    bad = 0
    print('  %s:' % what)
    # the hints' lines `segment NAME FRAME ...`
    segs = sorted((int(f[2], 16), f[1]) for f in (l.split() for l in open(os.path.join(ROOT, 'src', 'HL.hints')))
                  if len(f) > 2 and f[0] == 'segment')
    regions = []
    for i, (frame, name) in enumerate(segs):
        if name in ('S331F', 'S37FF'):
            regions.append((name, (frame + LOAD) * 16, (segs[i + 1][0] + LOAD) * 16))
        if name == 'DATA':
            regions.append(('the mouse', (frame + LOAD) * 16 + 0x414, (frame + LOAD) * 16 + 0x45C))
            regions.append(('blocks', (frame + LOAD) * 16 + 0x467, (frame + LOAD) * 16 + 0x469))
    regions.append(('the block', BLOCK + 0x40, BLOCK + 0x40000))
    for name, lo, hi in regions:
        d = [i for i in range(lo, hi) if x[i] != y[i]]
        if name == 'S331F':
            count = [i for i in d if 0x4CB7 <= i - lo < 0x4CBB]
            d = [i for i in d if i not in count]
            if count:
                at = lo + 0x4CB7
                print('  the passes counted in the battle before: %d, port %d' %
                      (int.from_bytes(x[at:at + 4], 'little'), int.from_bytes(y[at:at + 4], 'little')))
        if name == 'the mouse' and what.endswith('end'):
            # the pointer's place (words 10h and 12h): game_loop opens the
            # mouse again behind the menu, the original's driver then has
            # the pointer in the middle, the port's where the menu left it
            place = [i for i in d if 0x10 <= i - lo < 0x14]
            d = [i for i in d if i not in place]
            if place:
                print('  the pointer\'s place after mouse_open differs (%d bytes), told apart' % len(place))
        print('  %-10s %6d bytes, %d differ%s' % (name, hi - lo, len(d),
              (': first at +%Xh (%02X, port %02X)' % (d[0] - lo, x[d[0]], y[d[0]])) if d else ''))
        bad += bool(d)
    return bad


bad += memory('memory at the menu\'s last return', open(os.path.join(out, 'exit.ram'), 'rb').read(),
              open(os.path.join(out, 'port.ram'), 'rb').read())
# the program's end (the walk `exit`): the original's memory where it
# calls input_remove, the port's at its end; with it the count of
# mem_alloc's blocks (DATA:0467)
if os.path.exists(os.path.join(out, 'end.ram')):
    if os.path.exists(os.path.join(out, 'port_end.ram')):
        bad += memory('memory at the program\'s end', open(os.path.join(out, 'end.ram'), 'rb').read(),
                      open(os.path.join(out, 'port_end.ram'), 'rb').read())
    else:
        print('  memory at the program\'s end: the port wrote none')
        bad += 1
# video memory (byte 4 * offset + plane): the two pages' 200 lines of 80
# bytes a plane at A000h and A400h; what lies behind them is told apart
# (from A7E8:0004 on the original's start, whose part before the menu is
# not translated, has left what the port's has not)
shots = open(os.path.join(out, 'shots.txt')).read().split() if os.path.exists(os.path.join(out, 'shots.txt')) else []
asks = int(open(os.path.join(out, 'asks.txt')).read()) if os.path.exists(os.path.join(out, 'asks.txt')) else 0
for what, vf, pf in [('at %5s s' % t, 'shot%s.vram' % t, 'port%s.vram' % t) for t in shots] + \
        [('at screen %d of the saved game\'s question' % k, 'ask%d.vram' % k, 'portask%d.vram' % k)
         for k in range(1, asks + 1)]:
    v = open(os.path.join(out, vf), 'rb').read()
    pv = os.path.join(out, pf)
    if not os.path.exists(pv):
        print('  video memory %s: the port wrote none' % what)
        bad += 1
        continue
    w = open(pv, 'rb').read()
    d = [i >> 2 for i in range(len(v)) if v[i] != w[i]]
    pages = len([o for o in d if o < 0x8000 and o % 0x4000 < 200 * 80])
    print('  video memory %s: %d bytes differ in the two pages, %d behind them' % (what, pages, len(d) - pages))
    bad += bool(pages)
sys.exit(1 if bad else 0)
