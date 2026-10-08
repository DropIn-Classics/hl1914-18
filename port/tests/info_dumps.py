#!/usr/bin/env python3
"""Makes what port/tests/info_test.sh compares the port's field menu's
information (unit_info and battle_info, port/src/draw.c; the original's
S131D:0483 and S131D:0EE5) with.

    python3 port/tests/info_dumps.py [KEYS [UNTIL]]

First port/tests/play_dumps.py with WALK=code (the menu walked by keys,
the map's code typed) for the key script of port/tests/keys/ (default
m48_info.keys, 64 seconds): the original's clock, its inputs and its
memory at chosen passes in build/play/TAG/.  Then the same run again
with the two routines' entries and the places game_loop has their
returns at (S066A:2434, S066A:2473) logged, and once more for each call,
stopped at its entry and at its return.  Written into build/play/TAG/:

    kinds.txt        a line a call in the run's order: `unit` or `battle`
    infoN_entry.ram  memory at the N-th call's entry
    infoN_end.ram, infoN_end.vram   memory and video memory at its return
    shownN.vram      video memory at the beginning of the first pass a
                     second or more after the N-th call (shown.txt: the
                     passes' numbers)

No game data is kept in this file.
"""
import os, re, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
KIT = os.path.join(ROOT, 'doskit', 'tools', 'run.py')
sys.path.insert(0, os.path.join(ROOT, 'tools'))
import tpwmfiles

keys = sys.argv[1] if len(sys.argv) > 1 else 'm48_info.keys'
until = sys.argv[2] if len(sys.argv) > 2 else '64'
tag = os.path.splitext(keys)[0]
out = os.path.join(ROOT, 'build', 'play', tag)
env = dict(os.environ, WALK='code')
subprocess.run(['python3', os.path.join(HERE, 'play_dumps.py'), keys, until], env=env, check=True)

# the menu's keys as play_dumps.py's WALK=code has them
game = os.environ.get('DOSKIT_GAME') or os.path.join(ROOT, 'game')
rec = tpwmfiles.unpack(open(os.path.join(game, 'CODES.DAT'), 'rb').read())[int(keys[1:3]) * 10:][:5]
menu = [(10, 'down'), (11, 'space')] + [(12 + i * 0.5, chr(b - 0x11 + ord('a'))) for i, b in enumerate(rec)]
menu += [(15, 'enter'), (16, 'up'), (17, 'space'), (19, 'space')]
pre = ['-key', '3', 'enter', '-key', '6', 'enter']
for t, k in menu:
    pre += ['-key', str(t), k]
pre += ['-keys', os.path.join(HERE, 'keys', keys), '-until', until]

ENTRY = {'unit': 'S131D:0483', 'battle': 'S131D:0EE5'}
BACK = {'unit': 'S066A:2434', 'battle': 'S066A:2473'}


def run(args):
    r = subprocess.run(['python3', KIT] + pre + args + ['HL.EXE'], capture_output=True, text=True, cwd=out)
    return r.stdout + r.stderr


log = run(['-log', ENTRY['unit'], '-log', ENTRY['battle']])
calls = []
for line in log.split('\n'):
    m = re.match(r'log HL\.EXE\+131D:(\w{4}) t=(\S+) hit=(\d+)', line)
    if m:
        calls.append(('unit' if m.group(1) == '0483' else 'battle', int(m.group(3)), m.group(2)))
open(os.path.join(out, 'kinds.txt'), 'w').write(''.join('%s\n' % c[0] for c in calls))
for old in os.listdir(out):
    if re.match(r'(info|shown)\d+', old):
        os.remove(os.path.join(out, old))
shown = []
for n, (kind, hit, t) in enumerate(calls, 1):
    run(['-break', '%s#%d' % (ENTRY[kind], hit), '-ram', os.path.join(out, 'info%d_entry.ram' % n)])
    run(['-break', '%s#%d' % (BACK[kind], hit), '-ram', os.path.join(out, 'info%d_end.ram' % n),
         '-vram', os.path.join(out, 'info%d_end.vram' % n)])
    # the screen a second later: video memory where the first pass at or
    # after that time begins (S066A:0EE9)
    p = next((int(f[2]) for f in (l.split() for l in open(os.path.join(out, 'clock.txt')))
              if len(f) > 2 and f[1] == 'P' and float(f[0]) >= float(t) + 1), None)
    if p:
        run(['-break', 'S066A:0EE9#%d' % p, '-vram', os.path.join(out, 'shown%d.vram' % n)])
        shown.append(p)
    print('call %d: %s at %s s; shown at pass %s' % (n, kind, t, p))
open(os.path.join(out, 'shown.txt'), 'w').write(' '.join(str(p) for p in shown) + '\n')
