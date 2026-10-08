#!/usr/bin/env python3
"""Makes what port/tests/sound_test.sh compares the port's sound routines
(port/src/sound.c) with.

    python3 port/tests/sound_dumps.py [menu] [battle]

HL.EXE is run on the runner and stopped at the entry and at the end of
calls of the sound's routines, its memory kept each time:

    menu     the menu walked by the keys of menu_dumps.py's walk `pages`
             (build/menu/pages/keys.txt: run menu_dumps.py pages first);
             snd_init, every sfx_play, the snd_pass behind each, and
             calls of the timer's routine adl_tick: the first twelve after
             each of the first three effects began and those around an
             effect's end
    battle   map 8 with port/tests/keys/m8_site.keys as play_dumps.py runs
             it: a choice of the calls of sfx_play (the first six, then
             every 40th), the snd_pass behind each, the calls of dig_play
             from the fight's pictures and a choice of chan_stop

A first run logs the calls (build/sound/WALK/log.txt), the others, eight
at a time, each keep one dump.  Written: build/sound/WALK/*.ram and
build/sound/WALK.args, the argument list of port/tests/sound_test.c.

No game data is kept in this file.
"""
import os, re, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
KIT = os.path.join(ROOT, 'doskit', 'tools', 'run.py')

# kind: (entry, end)
CALLS = {
    'init': ('S2F39:000B', 'S2F39:008D'),
    'play': ('S2F39:06E3', 'S2F39:0965'),
    'dig': ('S2F39:0177', 'S2F39:068B'),
    'stop': ('S2F39:09C9', 'S2F39:0A6C'),
    'pass': ('S2F39:096A', 'S2F39:09C5'),
    'tick': ('S30EF:192D', 'S30EF:1A2A'),
}


def menu_pre():
    path = os.path.join(ROOT, 'build', 'menu', 'pages', 'keys.txt')
    if not os.path.exists(path):
        raise SystemExit('no %s: python3 port/tests/menu_dumps.py pages' % path)
    pre = ['-key', '3', 'enter', '-key', '6', 'enter']
    for line in open(path):
        t, k = line.split()
        pre += ['-key', t, k]
    return pre + ['-until', '66']


def battle_pre():
    # as play_dumps.py starts a battle on a map without its own code
    pre = ['-mouse', '21', '150,73,1', '-mouse', '21.2', '150,73,0', '-key', '22', 'r',
           '-key', '22.5', 'a', '-key', '23', 't', '-key', '23.5', 'i', '-key', '24', 'o',
           '-key', '25', 'enter', '-mouse', '27', '150,48,1', '-mouse', '27.2', '150,48,0']
    pre += ['-keys', os.path.join(HERE, 'keys', 'm8_site.keys'), '-until', '400']
    pre += subprocess.run(['python3', os.path.join(HERE, 'play_pokes.py'), 'm8_site'],
                          capture_output=True, text=True).stdout.split()
    return pre


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


def choose(walk, found):
    """the calls kept: (kind, hit number from 1)"""
    plays, passes, ticks = found['play'][0], found['pass'][0], found['tick'][0]
    want = []
    if walk == 'menu':
        want.append(('init', 1))
        picked = list(range(1, len(plays) + 1))
    else:
        picked = [n for n in range(1, len(plays) + 1) if n <= 6 or n % 40 == 0]
        want += [('dig', n) for n in range(1, len(found['dig'][0]) + 1)]
        want += [('stop', n) for n in range(1, len(found['stop'][0]) + 1) if n <= 2 or n % 6 == 0]
    for n in picked:
        want.append(('play', n))
        t = plays[n - 1][0]
        after = [i for i, (pt, _) in enumerate(passes) if pt > t]
        if after:
            want.append(('pass', after[0] + 1))
    if walk == 'menu':
        some = set()
        for n in picked[:3]:
            t = plays[n - 1][0]
            some.update([i for i, (tt, _) in enumerate(ticks) if tt > t][:12])
        # an effect's end: adl_stop called from the timer's routine
        for t in found.get('ends', []):
            some.update(i for i, (tt, _) in enumerate(ticks) if t - 0.004 < tt <= t)
        want += [('tick', i + 1) for i in sorted(some)]
    return want


def main():
    for walk in sys.argv[1:] or ['menu', 'battle']:
        out = os.path.join(ROOT, 'build', 'sound', walk)
        os.makedirs(out, exist_ok=True)
        pre = menu_pre() if walk == 'menu' else battle_pre()
        logs = []
        for kind, (a, b) in CALLS.items():
            if kind == 'tick' and walk != 'menu':
                continue
            logs += ['-log', a, '-log', b]
        log = run(out, pre + logs + ['-log', 'S2F39:1754'])
        open(os.path.join(out, 'log.txt'), 'w').write(log)
        found = hits(log)
        # adl_stop reached from the timer's routine: inside a tick
        ends = []
        ticks = found['tick']
        for m in re.finditer(r'log HL\.EXE\+2F39:1754 t=([\d.]+)', log):
            t = float(m.group(1))
            inside = [1 for (a, _), (b, _) in zip(ticks[0], ticks[1]) if a <= t <= b]
            if inside:
                ends.append(t)
        found['ends'] = ends
        want = choose(walk, found)
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
        with open(os.path.join(ROOT, 'build', 'sound', walk + '.args'), 'w') as f:
            f.write('\n'.join(lines) + '\n')
        print('%s: %d calls kept (%s)' % (walk, len(lines), ', '.join(
            '%d %s' % (sum(1 for k, _ in want if k == kind), kind) for kind in CALLS if any(k == kind for k, _ in want))))


main()
