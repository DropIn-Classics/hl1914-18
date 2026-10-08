#!/usr/bin/env python3
"""Makes what port/tests/briefing_test.sh compares the port's new units'
screens (port/src/briefing.c, the original's S274D:0003) with.

    python3 port/tests/briefing_dumps.py [WALK...]

HL.EXE is run on the runner with keys that type a map's code in the menu
(read from the game's CODES.DAT) and press START, then a key every five
seconds for the screens shown before the battle.  The walks (default:
all):

    m4     map 4: both sides have a new unit (four screens)
    m12    map 12: the second side has two (five screens)
    m28    map 28, which is map 4 for the other country (the same four)
    m0     map 0, which has none: the routine returns at once

Written into build/briefing/WALK/ (ignored):

    keys.txt      the walk as a key script for the port's program (-keys)
    log.txt       the runner's log: the menu's return (S066A:0390), the
                  routine's entry (S274D:0003), each screen lit and about
                  to wait (S274D:0AC2), the routine's return (S066A:09BD)
    start.txt     when the menu's first pass began (the port's -start)
    screens.txt   how many screens were shown
    entry.ram     memory at the routine's entry
    end.ram, end.vram     memory and video memory at its return
    screenN.vram  video memory when screen N is lit, before it waits

No game data is kept in this file.
"""
import os, re, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
KIT = os.path.join(ROOT, 'doskit', 'tools', 'run.py')
sys.path.insert(0, os.path.join(ROOT, 'tools'))
import tpwmfiles

GAME = os.environ.get('DOSKIT_GAME') or os.path.join(ROOT, 'game')


def code(m, t):
    """the keys of map m's code from t on, two a second"""
    rec = tpwmfiles.unpack(open(os.path.join(GAME, 'CODES.DAT'), 'rb').read())[m * 10:][:5]
    return [(t + i * 0.5, chr(b - 0x11 + ord('a'))) for i, b in enumerate(rec)]


def walk_keys(m):
    # as menu_dumps.py's code57: START takes the second press
    return [(10, 'down'), (11, 'space')] + code(m, 12) + [(15, 'enter'), (16, 'up'), (17, 'space'), (19, 'space')] \
        + [(26 + 5 * i, 'space') for i in range(6)]


WALKS = {
    'm4': (walk_keys(4), 60),
    'm12': (walk_keys(12), 60),
    'm28': (walk_keys(28), 60),
    'm0': ([(10, 'space'), (13, 'space')], 22),
}


def run(out, a, extra):
    r = subprocess.run(['python3', KIT] + a + extra + ['HL.EXE'], capture_output=True, text=True, cwd=out)
    return r.stdout + r.stderr


for walk in sys.argv[1:] or sorted(WALKS):
    keys, until = WALKS[walk]
    out = os.path.join(ROOT, 'build', 'briefing', walk)
    os.makedirs(out, exist_ok=True)
    with open(os.path.join(out, 'keys.txt'), 'w') as f:
        for t, k in keys:
            f.write('%s %s\n' % (t, k))
    a = ['-key', '3', 'enter', '-key', '6', 'enter']
    for t, k in keys:
        a += ['-key', str(t), k]
    a += ['-until', str(until)]
    log = run(out, a, ['-log', 'S167A:130B', '-log', 'S066A:0390', '-log', 'S274D:0003', '-log', 'S274D:0AC2',
                       '-break', 'S066A:09BD', '-ram', os.path.join(out, 'end.ram'),
                       '-vram', os.path.join(out, 'end.vram')])
    open(os.path.join(out, 'log.txt'), 'w').write(log)
    start, screens, times = None, 0, {}
    for line in log.split('\n'):
        m = re.match(r'log HL\.EXE\+(\w{4}:\w{4}) t=(\S+) hit=(\d+)', line)
        if not m:
            continue
        if m.group(1) == '167A:130B' and start is None:
            start = m.group(2)
        if m.group(1) == '274D:0AC2':
            screens = int(m.group(3))
        times.setdefault(m.group(1), []).append(m.group(2))
    open(os.path.join(out, 'start.txt'), 'w').write('%s\n' % start)
    open(os.path.join(out, 'screens.txt'), 'w').write('%d\n' % screens)
    run(out, a, ['-break', 'S274D:0003', '-ram', os.path.join(out, 'entry.ram')])
    for n in range(1, screens + 1):
        run(out, a, ['-break', 'S274D:0AC2#%d' % n, '-vram', os.path.join(out, 'screen%d.vram' % n)])
    print('%s: the menu returned at %s, the routine entered at %s, %d screens lit at %s' %
          (walk, ', '.join(times.get('066A:0390', [])), ', '.join(times.get('274D:0003', [])), screens,
           ', '.join(times.get('274D:0AC2', [])) or 'no time'))
