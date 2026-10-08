#!/usr/bin/env python3
"""Compares which effects the port's menu and a battle's passes play
with the original's (port/tests/sound_test.sh calls it).

    python3 port/tests/sound_cmp.py

The original's: build/sound/menu/log.txt (the calls of sfx_play, in
order) and the stack words of each call's entry dump (the channel and the
effect's number), from port/tests/sound_dumps.py menu.  The port's:
build/sound/port_menu.snd, written by its program with -sndlog on the
same keys (build/menu/pages/keys.txt): time, channel, number, result.

With build/sound/port_battle.snd (the port's battle on map 8 with
m8_site.keys on the original's clock) the battle's calls are compared
too, see below.

Exit status 0 when both play the same effects on the same channels in
the same order with the same results; how far the times are apart at
most is told, not judged (the port's passes are not the original's).
"""
import os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
out = os.path.join(ROOT, 'build', 'sound')

def original(walk):
    """the walk's calls of sfx_play in order: (time, hit, channel, number,
    result), channel and number None where no entry dump was kept"""
    log = open(os.path.join(out, walk, 'log.txt')).read()
    entries = re.findall(r'log HL\.EXE\+2F39:06E3 t=([\d.]+) hit=(\d+) regs (.*)', log)
    ends = re.findall(r'log HL\.EXE\+2F39:0965 t=[\d.]+ hit=\d+ regs (.*)', log)
    calls = []
    for (t, n, regs), end in zip(entries, ends):
        r = dict(f.split('=') for f in regs.split() if '=' in f)
        ax = int(dict(f.split('=') for f in end.split() if '=' in f)['AX'], 16)
        ch = num = None
        path = os.path.join(out, walk, 'play%s_in.ram' % n)
        if os.path.exists(path):
            ram = open(path, 'rb').read()
            at = int(r['SS'], 16) * 16 + int(r['SP'], 16) + 4
            ch, num = ram[at] | ram[at + 1] << 8, ram[at + 2] | ram[at + 3] << 8
        calls.append((float(t), int(n), ch, num, ax - 0x10000 if ax & 0x8000 else ax))
    return calls


def ported(name):
    return [(float(f[0]), int(f[1]), int(f[2]), int(f[3])) for f in (l.split() for l in open(os.path.join(out, name)))]


bad = 0
orig, port = original('menu'), ported('port_menu.snd')
if [x[2:] for x in orig] != [x[1:] for x in port]:
    print('the menu\'s effects differ: original %s, port %s' % ([x[2:] for x in orig], [x[1:] for x in port]))
    bad = 1
else:
    far = max([abs(a[0] - b[0]) for a, b in zip(orig, port)] or [0])
    print('the menu: %d effects, the same channel, number and result in the same order; times at most %.2f s apart'
          % (len(orig), far))

# The battle (map 8, m8_site.keys): the port played it on the original's
# clock (build/play/m8_site/clock.txt, play_dumps.py), so its calls are
# the original's from the battle's first pass on, one for one.  The
# original's channel and number are known for the calls whose entry was
# kept; the results for all.
if os.path.exists(os.path.join(out, 'port_battle.snd')):
    clock = os.path.join(ROOT, 'build', 'play', 'm8_site', 'clock.txt')
    first = min(float(l.split()[0]) for l in open(clock) if l.split()[1] == 'P')
    orig = [x for x in original('battle') if x[0] >= first]
    port = ported('port_battle.snd')
    wrong = [(a, b) for a, b in zip(orig, port) if a[4] != b[3] or (a[2] is not None and a[2:4] != b[1:3])]
    known = sum(1 for a in orig if a[2] is not None)
    if wrong or len(port) < len(orig):
        print('the battle\'s effects differ: %d calls in the original, %d in the port; the first: %s'
              % (len(orig), len(port), wrong[:3]))
        bad = 1
    else:
        print('the battle: %d calls of sfx_play in the original, %d in the port (it plays to the clock\'s end); '
              'the results the same in order, channel and number the same in the %d calls kept'
              % (len(orig), len(port), known))
sys.exit(bad)
