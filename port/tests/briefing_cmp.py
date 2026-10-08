#!/usr/bin/env python3
"""Compares the port's new units' screens (port/src/briefing.c) with the
original's (port/tests/briefing_test.sh calls it).

    python3 port/tests/briefing_cmp.py WALK

In build/briefing/WALK/: entry.ram, end.ram, end.vram and screenN.vram
are the original's (briefing_dumps.py), the same names with port_ before
them the port's (-briefdump).  Compared:

  * memory at the routine's entry and at its return: the game's state
    (S331F; but for the count of a battle's passes, the dword at
    S331F:4CB7, which no battle has reset yet), the menu's segment
    (S37FF, with the record of MNUCHR14.LIB the routine loads anew), the
    palette (DATA:04E3, 300h bytes) and the block game_loop allocates
    from 40h bytes behind its beginning for 40000h bytes, where the
    routine's files go;
  * the bytes of the whole image that differ at the return and did not
    at the entry, by segment (told, not judged: the stack, the timer's
    and the keyboard's counts, what the sound's routines keep);
  * video memory with each screen lit and at the return: the page drawn
    and the page shown, 200 lines each.

Exit status 0 when no byte of the regions and the pages differs and the
port showed as many screens as the original.
"""
import os, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
walk = sys.argv[1]
out = os.path.join(ROOT, 'build', 'briefing', walk)
LOAD = 0x76             # where the runner and the port load HL.EXE
BLOCK = 0x40A60         # game_loop's block in both (its MCB follows the program's)

# the hints' lines `segment NAME FRAME ...`
segs = sorted((int(f[2], 16), f[1]) for f in (l.split() for l in open(os.path.join(ROOT, 'src', 'HL.hints')))
              if len(f) > 2 and f[0] == 'segment')


def read(name):
    p = os.path.join(out, name)
    return open(p, 'rb').read() if os.path.exists(p) else None


def regions():
    r = []
    for i, (frame, name) in enumerate(segs):
        if name in ('S331F', 'S37FF'):
            r.append((name, (frame + LOAD) * 16, (segs[i + 1][0] + LOAD) * 16))
        if name == 'DATA':
            r.append(('the palette', (frame + LOAD) * 16 + 0x4E3, (frame + LOAD) * 16 + 0x7E3))
    r.append(('the block', BLOCK + 0x40, BLOCK + 0x40000))
    return r


def memory(what, x, y):
    """the regions of two dumps compared; how many of them differ"""
    bad = 0
    print('  memory at the routine\'s %s:' % what)
    for name, lo, hi in regions():
        d = [i for i in range(lo, hi) if x[i] != y[i]]
        if name == 'S331F':
            d = [i for i in d if not 0x4CB7 <= i - lo < 0x4CBB]
        print('  %-11s %6d bytes, %d differ%s' % (name, hi - lo, len(d),
              (': first at +%Xh (%02X, port %02X)' % (d[0] - lo, x[d[0]], y[d[0]])) if d else ''))
        bad += bool(d)
    return bad


def seg_of(a):
    name = '(below the program)'
    for frame, n in segs:
        if (frame + LOAD) * 16 <= a:
            name = n
    if a >= BLOCK:
        name = 'the block' if a < BLOCK + 0x445C0 else '(behind the block)'
    return name


def pages(what, v, w):
    """video memory (byte 4 * offset + plane): the two pages' 200 lines of
    80 bytes a plane at A000h and A400h"""
    if w is None:
        print('  video memory %s: the port wrote none' % what)
        return 1
    d = [i >> 2 for i in range(len(v)) if v[i] != w[i]]
    n = len([o for o in d if o < 0x8000 and o % 0x4000 < 200 * 80])
    print('  video memory %-16s %d bytes differ in the two pages, %d behind them' % (what + ':', n, len(d) - n))
    return 1 if n else 0


bad = 0
print('%s:' % walk)
e0, e1, p0, p1 = read('entry.ram'), read('end.ram'), read('port_entry.ram'), read('port_end.ram')
if p0 is None or p1 is None:
    print('  the port wrote no memory')
    sys.exit(1)
bad += memory('entry', e0, p0)
bad += memory('return', e1, p1)
new = {}
for i in range(min(len(e1), len(p1))):
    if e1[i] != p1[i] and e0[i] == p0[i]:
        new.setdefault(seg_of(i), []).append(i)
print('  differing at the return and not at the entry: %s' %
      (', '.join('%s %d' % (k, len(v)) for k, v in sorted(new.items())) or 'nothing'))
n = int(open(os.path.join(out, 'screens.txt')).read())
for k in range(1, n + 1):
    bad += pages('with screen %d lit' % k, read('screen%d.vram' % k), read('port_screen%d.vram' % k))
if os.path.exists(os.path.join(out, 'port_screen%d.vram' % (n + 1))):
    print('  the port showed more screens than the original\'s %d' % n)
    bad += 1
bad += pages('at the return', read('end.vram'), read('port_end.vram'))
sys.exit(1 if bad else 0)
