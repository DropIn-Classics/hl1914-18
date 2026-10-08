#!/usr/bin/env python3
"""Makes what port/tests/music_test.sh compares the port's music control
(port/src/sound.c: music_start, music_fade, music_volume, fade_tick) with.

    python3 port/tests/music_dumps.py [menu] [brief] [battle]

HL.EXE is run on the runner and stopped at the entry and at the end of
calls of the music's routines, its memory kept each time:

    menu     the menu walked by the keys of menu_dumps.py's walk `pages`
             (build/menu/pages/keys.txt: run menu_dumps.py pages first)
    brief    the new units' screens of map 4 (build/briefing/m4/keys.txt:
             run briefing_dumps.py m4 first) and the battle's beginning
             behind them
    battle   map 8 with port/tests/keys/m8_site.keys as play_dumps.py runs
             it

Kept in each: every call of music_start and music_fade, the first 40 of
fade_tick (the fade's timer routine) and of music_volume, and the first
eight calls of the timer's routine adl_tick after each of the first three
calls of music_start.

A first run logs the calls (build/music/WALK/log.txt), the others, eight
at a time, each keep one dump.  Written: build/music/WALK/*.ram and
build/music/WALK.args, the argument list of port/tests/sound_test.c.

No game data is kept in this file.
"""
import os, re, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
KIT = os.path.join(ROOT, 'doskit', 'tools', 'run.py')

# kind: (entry, end)
CALLS = {
    'mstart': ('S2F39:0F87', 'S2F39:1053'),
    'mfade': ('S2F39:1058', 'S2F39:1122'),
    'mvol': ('S2F39:1127', 'S2F39:11A7'),
    'ftick': ('S30EF:12D9', 'S30EF:130E'),
    'tick': ('S30EF:192D', 'S30EF:1A2A'),
}


def keys_pre(path, hint, until):
    if not os.path.exists(path):
        raise SystemExit('no %s: python3 port/tests/%s' % (path, hint))
    pre = ['-key', '3', 'enter', '-key', '6', 'enter']
    for line in open(path):
        t, k = line.split()
        pre += ['-key', t, k]
    return pre + ['-until', str(until)]


def battle_pre():
    # as play_dumps.py starts a battle on a map without its own code
    pre = ['-mouse', '21', '150,73,1', '-mouse', '21.2', '150,73,0', '-key', '22', 'r',
           '-key', '22.5', 'a', '-key', '23', 't', '-key', '23.5', 'i', '-key', '24', 'o',
           '-key', '25', 'enter', '-mouse', '27', '150,48,1', '-mouse', '27.2', '150,48,0']
    pre += ['-keys', os.path.join(HERE, 'keys', 'm8_site.keys'), '-until', '400']
    pre += subprocess.run(['python3', os.path.join(HERE, 'play_pokes.py'), 'm8_site'],
                          capture_output=True, text=True).stdout.split()
    return pre


WALKS = {
    'menu': lambda: keys_pre(os.path.join(ROOT, 'build', 'menu', 'pages', 'keys.txt'), 'menu_dumps.py pages', 66),
    'brief': lambda: keys_pre(os.path.join(ROOT, 'build', 'briefing', 'm4', 'keys.txt'), 'briefing_dumps.py m4', 60),
    'battle': battle_pre,
}


def run(out, args):
    r = subprocess.run(['python3', KIT] + args + ['HL.EXE'], capture_output=True, text=True, cwd=out)
    return r.stdout + r.stderr


def hits(log):
    """kind -> for entry and end the list of (time, registers) by hit"""
    at = {}
    for kind, (a, b) in CALLS.items():
        for which, addr in ((0, a), (1, b)):
            at[addr] = (kind, which)
    found = {k: ([], []) for k in CALLS}
    for m in re.finditer(r'log HL\.EXE\+(\w+):(\w+) t=([\d.]+) hit=(\d+) regs (.*)', log):
        key = 'S%s:%s' % (m.group(1), m.group(2))
        if key in at:
            kind, which = at[key]
            found[kind][which].append((float(m.group(3)), dict(f.split('=') for f in m.group(5).split() if '=' in f)))
    return found


def choose(found):
    """the calls kept: (kind, hit number from 1)"""
    want = [('mstart', n) for n in range(1, len(found['mstart'][0]) + 1)]
    want += [('mfade', n) for n in range(1, len(found['mfade'][0]) + 1)]
    want += [('ftick', n) for n in range(1, min(40, len(found['ftick'][0])) + 1)]
    want += [('mvol', n) for n in range(1, min(40, len(found['mvol'][0])) + 1)]
    some = set()
    for t, _ in found['mstart'][1][:3]:
        some.update([i for i, (tt, _) in enumerate(found['tick'][0]) if tt > t][:8])
    want += [('tick', i + 1) for i in sorted(some)]
    return want


def main():
    for walk in sys.argv[1:] or sorted(WALKS):
        out = os.path.join(ROOT, 'build', 'music', walk)
        os.makedirs(out, exist_ok=True)
        pre = WALKS[walk]()
        logs = []
        for a, b in CALLS.values():
            logs += ['-log', a, '-log', b]
        log = run(out, pre + logs)
        open(os.path.join(out, 'log.txt'), 'w').write(log)
        found = hits(log)
        want = choose(found)
        jobs, lines = [], []
        for kind, n in want:
            a, b = CALLS[kind]
            if n > len(found[kind][1]):
                continue        # the run ended inside the call
            regs, end = found[kind][0][n - 1][1], found[kind][1][n - 1][1]
            e = os.path.join(out, '%s%d_in.ram' % (kind, n))
            x = os.path.join(out, '%s%d_out.ram' % (kind, n))
            jobs.append(pre + ['-break', '%s#%d' % (a, n), '-ram', e])
            jobs.append(pre + ['-break', '%s#%d' % (b, n), '-ram', x])
            lines.append('%s %s:%s %s %s %s' % (kind, regs['SS'], regs['SP'], end['AX'], e, x))
        with ThreadPoolExecutor(8) as pool:
            list(pool.map(lambda a: run(out, a), jobs))
        with open(os.path.join(ROOT, 'build', 'music', walk + '.args'), 'w') as f:
            f.write('\n'.join(lines) + '\n')
        print('%s: %d calls kept (%s)' % (walk, len(lines), ', '.join(
            '%d of %d %s' % (sum(1 for k, _ in want if k == kind), len(found[kind][0]), kind) for kind in CALLS)))


main()
