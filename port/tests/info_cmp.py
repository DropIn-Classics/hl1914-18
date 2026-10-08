#!/usr/bin/env python3
"""Compares the port's field menu's information (unit_info, battle_info;
port/src/draw.c) with the original's (port/tests/info_test.sh calls it).

    python3 port/tests/info_cmp.py TAG

In build/play/TAG/: kinds.txt, infoN_entry.ram, infoN_end.ram and
infoN_end.vram are the original's (info_dumps.py), the same names with
port_ before them the port's (-infodump).  Compared for each call:

  * memory at the routine's entry and at its return: the game's state
    (S331F), the computer's (S39B6), the screen's record (DATA:00DA to
    00F3: the colour and rectangle the routines leave) and game_loop's
    block from 40h behind its beginning for 40000h bytes (the pictures
    read for the unit's large picture go there);
  * the bytes of the whole image that differ at the return and did not at
    the entry, by segment (told, not judged);
  * video memory at the return, and at the beginning of a pass a second
    later while the information is shown: the page drawn and the page
    shown, 200 lines each.

Exit status 0 when the calls are the same in number and kind and no byte
of the regions and the pages differs.
"""
import os, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
out = os.path.join(ROOT, 'build', 'play', sys.argv[1])
LOAD = 0x76             # where the runner and the port load HL.EXE
BLOCK = 0x40A60         # game_loop's block in both

segs = sorted((int(f[2], 16), f[1]) for f in (l.split() for l in open(os.path.join(ROOT, 'src', 'HL.hints')))
              if len(f) > 2 and f[0] == 'segment')


def read(name):
    p = os.path.join(out, name)
    return open(p, 'rb').read() if os.path.exists(p) else None


def regions():
    r = []
    for i, (frame, name) in enumerate(segs):
        if name in ('S331F', 'S39B6'):
            r.append((name, (frame + LOAD) * 16, (segs[i + 1][0] + LOAD) * 16))
        if name == 'DATA':
            r.append(('the screen', (frame + LOAD) * 16 + 0xDA, (frame + LOAD) * 16 + 0xF4))
    r.append(('the block', BLOCK + 0x40, BLOCK + 0x40000))
    return r


def memory(what, x, y):
    bad = 0
    for name, lo, hi in regions():
        d = [i for i in range(lo, hi) if x[i] != y[i]]
        print('    %-6s %-10s %6d bytes, %d differ%s' % (what, name, hi - lo, len(d),
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


kinds = (read('kinds.txt') or b'').decode().split()
port = (read('port_kinds.txt') or b'').decode().split()
bad = 0
print('%s: the original\'s calls: %s; the port\'s: %s' % (sys.argv[1], ' '.join(kinds) or 'none', ' '.join(port) or 'none'))
if kinds != port:
    bad += 1
for n, kind in enumerate(kinds, 1):
    e0, e1, v = read('info%d_entry.ram' % n), read('info%d_end.ram' % n), read('info%d_end.vram' % n)
    p0, p1, w = read('port_info%d_entry.ram' % n), read('port_info%d_end.ram' % n), read('port_info%d_end.vram' % n)
    print('  call %d (%s):' % (n, kind))
    if p0 is None or p1 is None or w is None:
        print('    the port wrote none')
        bad += 1
        continue
    bad += memory('entry', e0, p0)
    bad += memory('return', e1, p1)
    new = {}
    for i in range(min(len(e1), len(p1))):
        if e1[i] != p1[i] and e0[i] == p0[i]:
            new[seg_of(i)] = new.get(seg_of(i), 0) + 1
    print('    differing at the return and not at the entry: %s' %
          (', '.join('%s %d' % kv for kv in sorted(new.items())) or 'nothing'))
    # byte 4 * offset + plane: the two pages at A000h and A400h
    d = [i >> 2 for i in range(len(v)) if v[i] != w[i]]
    k = len([o for o in d if o < 0x8000 and o % 0x4000 < 200 * 80])
    print('    video memory at the return: %d bytes differ in the two pages, %d behind them' % (k, len(d) - k))
    bad += bool(k)
    v, w = read('shown%d.vram' % n), read('port_shown%d.vram' % n)
    if v is None:
        continue
    if w is None:
        print('    video memory a second later: the port wrote none')
        bad += 1
        continue
    d = [i >> 2 for i in range(len(v)) if v[i] != w[i]]
    k = len([o for o in d if o < 0x8000 and o % 0x4000 < 200 * 80])
    print('    video memory a second later: %d bytes differ in the two pages, %d behind them' % (k, len(d) - k))
    bad += bool(k)
sys.exit(1 if bad else 0)
