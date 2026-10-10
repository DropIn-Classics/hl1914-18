#!/usr/bin/env python3
"""Makes what port/tests/oplwav_test.sh sets beside the port's rendered
AdLib sound: the original's, as the runner's chip played it.

    python3 port/tests/oplwav_dumps.py [menu] [brief] [battle] [medal]

HL.EXE is run once on each walk of opl_dumps.py (the same keys) with
-oplwav (build/oplwav/WALK/orig.wav: the runner's OPL2, doskit's
runtime/opl.c, 49716 Hz mono, each register taking effect at the sample
of its moment) and a log of every OUT to the ports 388h and 389h, the
timer's interrupt, adl_tick's entry, the place it only reaches with the
lock clear and its end, fade_tick, and the ends of music_start and the
entry and end of music_fade.

Written: build/oplwav/WALK.orig, a line an event, the runner's time in
whole microseconds (as its log prints it) first:

    T w RR VV I     register RR gets VV (hex); I 1 inside adl_tick or
                    fade_tick
    T i             the timer's interrupt (pit_irq) begins
    T t             adl_tick begins
    T f             adl_tick is past the lock
    T e             adl_tick ends
    T ft | T fe     fade_tick begins, ends
    T start | T nostart   music_start ends with 1 (a tune plays), else
    T fade | T faded      music_fade begins, ends

No game data is kept in this file.
"""
import os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import opl_dumps as od

ROOT = od.ROOT
MARK = {od.PIT: 'i', od.TICK: 't', od.TICK_FREE: 'f', od.TICK_END: 'e', od.FTICK: 'ft', od.FTICK_END: 'fe',
        od.CALLS['mfade'][0]: 'fade', od.CALLS['mfade'][1]: 'faded'}
MSTART_END = od.CALLS['mstart'][1]


def main():
    for walk in sys.argv[1:] or ['menu', 'brief']:
        out = os.path.join(ROOT, 'build', 'oplwav', walk)
        os.makedirs(out, exist_ok=True)
        args = od.WALKS[walk]() + ['-oplwav', os.path.join(out, 'orig.wav')]
        for a in [x for p in od.OUTS for x in p] + list(MARK) + [MSTART_END]:
            args += ['-log', 'S' + a]
        log = od.run(out, args)
        open(os.path.join(out, 'log.txt'), 'w').write(log)
        reg_at = {a for a, b in od.OUTS}
        val_at = {b for a, b in od.OUTS}
        ev, latched, inside, n = [], None, 0, 0
        for m in re.finditer(r'log HL\.EXE\+(\w{4}:\w{4}) t=(\d+)\.(\d{6}) hit=\d+ regs CS:IP=\S+ AX=(\w{4})', log):
            a, t, ax = m.group(1), int(m.group(2)) * 1000000 + int(m.group(3)), int(m.group(4), 16)
            if a in reg_at:
                latched = ax & 255
            elif a in val_at:
                ev.append('%d w %02X %02X %d' % (t, latched, ax & 255, inside > 0))
                n += 1
            elif a == MSTART_END:
                ev.append('%d %s' % (t, 'start' if ax == 1 else 'nostart'))
            else:
                k = MARK[a]
                inside += (k in ('t', 'ft')) - (k in ('e', 'fe'))
                ev.append('%d %s' % (t, k))
        with open(os.path.join(ROOT, 'build', 'oplwav', walk + '.orig'), 'w') as f:
            f.write('\n'.join(ev) + '\n')
        stop = re.search(r'^stop .*', log, re.M)
        print('%s: %d registers, %d tunes started; %s' % (
            walk, n, sum(1 for e in ev if e.endswith(' start')), stop.group(0) if stop else 'no stop line'))


main()
