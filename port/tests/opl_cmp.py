#!/usr/bin/env python3
"""Compares what the port's program writes to the AdLib on a walk with
what the original wrote (port/tests/opl_test.sh calls it).

    python3 port/tests/opl_cmp.py WALK [...]

The original's: build/opl/WALK.ev (port/tests/opl_dumps.py).  The
port's: build/opl/port_WALK.opl, written by its program with -opllog on
the same keys.

The two programs do not run on one clock: the port's timer interrupts
come from its own clock of passes (snd_clock), so a call of the game's
side falls on another tick than in the original, and an effect that
takes voices from the tune changes what the tune writes from then on.
The streams as wholes are therefore not the same, and this comparison is
told, not judged (it always ends with status 0); port/tests/opl_test.c
is the one that judges, with the original's calls and interrupts.

Told for each tune (from the end of a music_start to the end of the next
music_fade or the run's end): how many registers each program wrote, and
how many of the original's writes the port made with the same register
and value at the same count of ticks behind the tune's start.
"""
import os, sys
from collections import Counter

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))


def original(path):
    """the tunes: for each the writes as (ticks behind the start, register, value)"""
    tunes, cur, ticks, call, t0 = [], None, 0, None, 0
    for line in open(path):
        w = line.split()
        if w[0] in ('tick', 'lock'):
            ticks += 1
        elif w[0] in ('mstart', 'mfade'):
            call = w[0]
        elif w[0] == 'ret':
            if call == 'mstart' and w[1] == '0001':
                cur, t0 = [], ticks
                tunes.append(cur)
            elif call == 'mfade':
                cur = None
            call = None
        elif w[0] == 'w' and cur is not None:
            cur.append((ticks - t0, w[1], w[2]))
    return tunes


def port(path):
    tunes, cur, t0 = [], None, 0
    for line in open(path):
        w = line.split()
        if w[0] == 'start':
            cur, t0 = [], int(w[2])
            tunes.append(cur)
        elif w[0] == 'fade':
            cur = None
        elif cur is not None:
            cur.append((int(w[1]) - t0, w[2], w[3]))
    return tunes


def main():
    for walk in sys.argv[1:]:
        o = original(os.path.join(ROOT, 'build', 'opl', walk + '.ev'))
        p = port(os.path.join(ROOT, 'build', 'opl', 'port_%s.opl' % walk))
        print('%s: the original started %d tunes, the port %d' % (walk, len(o), len(p)))
        for i, (a, b) in enumerate(zip(o, p)):
            # the tunes may be cut at different lengths: compare what both played
            end = min(max([x[0] for x in a] or [0]), max([x[0] for x in b] or [0]))
            a = [x for x in a if x[0] <= end]
            b = [x for x in b if x[0] <= end]
            same = sum((Counter(a) & Counter(b)).values())
            print('  tune %d, %d ticks: the original %d writes, the port %d, %d the same at the same tick'
                  % (i + 1, end, len(a), len(b), same))


main()
