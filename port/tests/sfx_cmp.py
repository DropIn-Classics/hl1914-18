#!/usr/bin/env python3
"""Compares which effects the port's program plays in the picture of a
side left without units with the original's (port/tests/sfx_test.sh
calls it).

    python3 port/tests/sfx_cmp.py VARIANT ...

The original's: build/sfx/VARIANT/log.txt (every call of sfx_play from
the battle's first pass on, with its result) and calls.txt (the
picture's calls with their arguments), from port/tests/sfx_dumps.py.
The port's: build/sfx/port_VARIANT.snd (m57_win.keys played on the
original's clock) and own_VARIANT.snd (on the port's own clock), written
with -sndlog: time, channel, number, result, pass, volume, times, the
picture's frame.

Exit status 0 when in each the calls are as many as the original's with
the same results in the same order, and the picture's calls come in the
same frames of it with the same channel, number, volume and count of
times.
"""
import os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
out = os.path.join(ROOT, 'build', 'sfx')


def original(variant):
    """(times and results of all calls from the first pass on, the
    picture's calls: channel, number, volume, times, result)"""
    log = open(os.path.join(out, variant, 'log.txt')).read()
    first = float(re.search(r'log HL\.EXE\+066A:0EE9 t=([\d.]+)', log).group(1))
    entries = [float(t) for t in re.findall(r'log HL\.EXE\+2F39:06E3 t=([\d.]+) hit=', log)]
    ends = [int(ax, 16) for ax in re.findall(r'log HL\.EXE\+2F39:0965 t=[\d.]+ hit=\d+ regs .*? AX=(\w+)', log)]
    every = [(t, ax - 0x10000 if ax & 0x8000 else ax) for t, ax in zip(entries, ends) if t >= first]
    picture = []
    for line in open(os.path.join(out, variant, 'calls.txt')):
        f = line.split('  ')[0].split()
        picture.append(tuple(int(v) for v in f[0:6]))
    return every, picture


def ported(name):
    rows = []
    for f in (l.split() for l in open(os.path.join(out, name))):
        rows.append((float(f[0]), int(f[7]), int(f[1]), int(f[2]), int(f[5]), int(f[6]), int(f[3])))
    return rows


bad = 0
for variant in sys.argv[1:]:
    every, picture = original(variant)
    for clock, name in (("the original's clock", 'port_%s.snd'), ("the port's own clock", 'own_%s.snd')):
        port = ported(name % variant)
        if len(port) != len(every) or [r for _, r in every] != [p[6] for p in port]:
            print('%s, %s: %d calls in the original, %d in the port, or their results differ: %s against %s'
                  % (variant, clock, len(every), len(port), [r for _, r in every], [p[6] for p in port]))
            bad = 1
            continue
        theirs = port[len(port) - len(picture):]
        wrong = [(i + 1, a, b[1:]) for i, (a, b) in enumerate(zip(picture, theirs)) if a != b[1:]]
        if wrong:
            print('%s, %s: the picture\'s calls differ (call, original, port; frame, channel, number, volume, times, '
                  'result): %s' % (variant, clock, wrong[:4]))
            bad = 1
            continue
        print('%s, %s: %d calls of sfx_play, the results the same in order; the picture\'s %d the same in '
              'frame, channel, number, volume, times and result (%d endless, %d more than once, %d failed)'
              % (variant, clock, len(every), len(picture), sum(1 for p in picture if p[4] == 0),
                 sum(1 for p in picture if p[4] > 1), sum(1 for p in picture if p[5] < 0)))
sys.exit(bad)
