#!/usr/bin/env python3
"""Compares which calls of music_start and music_fade the port's program
makes with the original's (port/tests/music_test.sh calls it).

    python3 port/tests/music_cmp.py [menu] [brief] [battle]

The original's: build/music/WALK/log.txt (the calls in order, the result
in AX at each one's end) and the stack words of each call's entry dump
(the fade's steps and the volume, or the speed), from
port/tests/music_dumps.py.  The port's: build/music/port_WALK.mus, written
by its program with -muslog on the same keys: the time at the call's end,
start or fade, the two numbers, the result.

    menu    the whole walk from the program's start
    brief   the same, through the new units' screens to the battle's
            beginning
    battle  the port plays the battle of map 8 with m8_site.keys on the
            original's clock (build/play/m8_site/clock.txt) and has no
            menu before it: both are compared from the battle's first
            music_start (the one without a fade) on.  The original's
            calls from S1A39 (a taken site's picture, which the port does
            not have: the far return address on the stack tells) are
            counted and left out.  The port's clock does not run there;
            the pass each call was made in is told instead (the
            original's from the clock's file)

Exit status 0 when the port makes the original's calls in the same order
with the same numbers and results (it may go on after the original's run
ended); how far the times of the calls' ends are apart at most is told,
not judged.
"""
import os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
out = os.path.join(ROOT, 'build', 'music')
ADDR = {'2F39:0F87': ('start', 0), '2F39:1053': ('start', 1), '2F39:1058': ('fade', 0), '2F39:1122': ('fade', 1)}


def frame(name):
    for l in open(os.path.join(ROOT, 'src', 'HL.hints')):
        f = l.split()
        if len(f) > 2 and f[0] == 'segment' and f[1] == name:
            return int(f[2], 16)


LOAD = 0x76     # where the runner loads the program (port/src/gen/names.h, HL_BASE)
left_out = 0


def original(walk):
    """the walk's calls in order: (time of the end, kind, a, b, result)"""
    global left_out
    log = open(os.path.join(out, walk, 'log.txt')).read()
    calls, open_call = [], {}
    for m in re.finditer(r'log HL\.EXE\+(\w+:\w+) t=([\d.]+) hit=(\d+) regs (.*)', log):
        if m.group(1) not in ADDR:
            continue
        kind, end = ADDR[m.group(1)]
        r = dict(f.split('=') for f in m.group(4).split() if '=' in f)
        if not end:
            open_call[kind] = (int(m.group(3)), r)
            continue
        n, e = open_call.pop(kind)
        ax = int(r['AX'], 16)
        ram = open(os.path.join(out, walk, '%s%d_in.ram' % ('mstart' if kind == 'start' else 'mfade', n)), 'rb').read()
        at = int(e['SS'], 16) * 16 + int(e['SP'], 16) + 4
        word = lambda o: ram[at + o] | ram[at + o + 1] << 8
        if word(-2) - LOAD == frame('S1A39'):
            left_out += 1
            continue
        a, b = (word(4), word(6)) if kind == 'start' else (word(0), 0)
        calls.append((float(m.group(2)), kind, a, b, ax - 0x10000 if ax & 0x8000 else ax))
    return calls


def ported(walk):
    return [(float(f[0]), f[1], int(f[2]), int(f[3]), int(f[4]), int(f[5]))
            for f in (l.split() for l in open(os.path.join(out, 'port_%s.mus' % walk)))]


def battle_only(calls):
    for i, c in enumerate(calls):
        if c[1] == 'start' and c[2] == 0:
            return calls[i:]
    return []


bad = 0
for walk in sys.argv[1:] or ['menu', 'brief', 'battle']:
    orig, port = original(walk), ported(walk)
    if walk == 'battle':
        orig, port = battle_only(orig), battle_only(port)
    wrong = [(a, b) for a, b in zip(orig, port) if a[1:] != b[1:5]]
    if wrong or len(port) < len(orig) or not orig:
        print('%s: the music\'s calls differ: %d in the original, %d in the port; the first: %s'
              % (walk, len(orig), len(port), wrong[:2]))
        bad = 1
        continue
    # the port's clock begins with the menu; a call before that has no time
    far = max([abs(a[0] - b[0]) for a, b in zip(orig, port) if b[0] > 0] or [0])
    if walk == 'battle':
        far = 0     # the original's clock: the port's own does not run
    print('%s: %d calls of music_start and music_fade in the original (%s), %d in the port; the same in order, '
          'numbers and results; their ends at most %.2f s apart'
          % (walk, len(orig), ', '.join('%s %d %d' % c[1:4] if c[1] == 'start' else 'fade %d' % c[2] for c in orig)
             if len(orig) <= 12 else '%d start, %d fade' % (sum(c[1] == 'start' for c in orig),
                                                           sum(c[1] == 'fade' for c in orig)),
             len(port), far))
    if walk == 'battle':
        clock = [l.split() for l in open(os.path.join(ROOT, 'build', 'play', 'm8_site', 'clock.txt'))]
        passes = [(float(f[0]), int(f[2])) for f in clock if len(f) > 2 and f[1] == 'P']
        at = lambda t: max([n for pt, n in passes if pt <= t] or [0])
        print('  the passes of the calls\' ends: original %s, port %s; %d calls from S1A39 left out'
              % (' '.join(str(at(a[0])) for a in orig), ' '.join(str(b[5]) for b in port), left_out))
sys.exit(bad)
