#!/usr/bin/env python3
"""Makes the dumps port/tests/computer_test.sh compares the port's
computer_input and computer_step with (HL.EXE's S1E4B:0001 and S1E4B:012E).

    python3 port/tests/computer_dumps.py [-until T] [-early T] [stage2]

Four battles are played on the runner by generated input, two with the
computer's second side (the maps of the codes 0 and 1) and two with the
computer's first side (those of the codes 24 and 25); the code is read
from the installed game's CODES.DAT.  The first of each pair gives the
calls of computer_input and the steps of the kinds 1 to 3, the second
those of the kinds 4, 6 and 7 (the computer builds there).  Stage 1 logs every call of the two routines up to T
emulated seconds (default 1500); stage 2 (alone with `stage2`, from the
logs there are, keeping the dumps there are) runs each battle again up
to a chosen call and dumps memory behind the routine's entry code and
before its return.  Chosen:
calls of computer_input spread over those before the early time (default
150 s), and those that returned 0 with the call behind each; of
computer_step three calls of each state of each kind of step (the state's
entry is logged), spread over those before twice the early time or else
the first there are, and three ends of a list.  Everything goes to build/computer/ (ignored): the key and
mouse files, the logs, the dumps and computer_test.args.  About an hour
on twelve cores.  No game data is kept in this file.
"""
import os, random, re, struct, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
sys.path.insert(0, os.path.join(ROOT, 'doskit', 'tools'))
import datfiles
from kit import game_dir

D = 'build/computer'
# name: code index, seed, the kinds of step dumped from it (0: a list's
# end), whether computer_input's calls are dumped
RUNS = {'second': (0, 7000, (0, 1, 2, 3), True), 'first': (24, 7024, (0, 1, 2, 3), True),
        'second_b': (1, 7001, (4, 6, 7), False), 'first_b': (25, 7025, (4, 6, 7), False)}
# computer_step: entry, exits (behind the last write of each), and the
# seven kinds' entries of its table
STEP_IN, STEP_OUT, STEP_END = '014F', '035B', '0372'
KINDS = {'01A3': 1, '01E2': 2, '0222': 3, '0265': 4, '029D': 5, '02D4': 6, '030B': 7}
# the handlers' tables of their states' entries (the kind, the table's
# offset in S1E4B, its entries); the offsets are read from the player's
# HL.EXE (states())
TABLES = {1: (0x06CF, 8), 2: (0x08E1, 6), 3: (0x0A1C, 5), 4: (0x0D6E, 13), 6: (0x0FAA, 7), 7: (0x127E, 8)}
S1E4B = 0x1E4B
# how many calls of each state of a kind of step a run gives, and of the
# lists' ends
PER_STATE, ENDS = 3, 3
INPUT_SPREAD, INPUT_SCRIPT = 14, 12


def code(index):
    """a map's code as the menu takes it: five letters of the record"""
    data = datfiles.unpacked(os.path.join(game_dir(), 'CODES.DAT'))
    return ''.join(chr(b - 0x11 + ord('a')) for b in data[index * 10:index * 10 + 5])


def states():
    """{offset in S1E4B: (kind, state)} of the handlers' states, from the
    jump tables in the program's file"""
    exe = open(os.path.join(game_dir(), 'HL.EXE'), 'rb').read()
    base = struct.unpack_from('<H', exe, 8)[0] * 16 + S1E4B * 16
    out = {}
    for kind, (table, n) in TABLES.items():
        for state, off in enumerate(struct.unpack_from('<%dH' % n, exe, base + table)):
            out['%04X' % off] = (kind, state)
    return out


def play(name, index, seed, end):
    """key and mouse files: the main menu, the code, START, then clicks
    and drags anywhere and the key pair that ends the human's round (as
    the scratch generator of the long runs made them, its mode FS)"""
    r = random.Random(seed)
    keys, mice = ['3 enter', '6 enter'], []

    def click(t, x, y, b=1):
        mice.extend([f'{t:.2f} {x},{y},0', f'{t+0.5:.2f} {x},{y},{b}', f'{t+0.7:.2f} {x},{y},0'])
    t = 8.5
    click(t, 150, 73); t += 1.5
    for ch in code(index):
        keys.append(f'{t:.2f} {ch}'); t += 0.6
    keys.append(f'{t:.2f} enter'); t += 2
    click(t, 150, 48); t += 3
    for d in (0, 3, 7):
        keys.append(f'{t + d:.2f} enter')
    t += 14
    x, y = 80, 100
    nf1 = t + 8
    while t < end:
        c = r.random()
        if t > 600:
            x, y = r.randrange(2, 318), r.randrange(2, 198)
            mice += [f'{t:.2f} {x},{y},0', f'{t+0.3:.2f} {x},{y},1', f'{t+0.5:.2f} {x},{y},0']
            keys += [f'{t+2:.2f} f1', f'{t+3.5:.2f} f1']; t += 15
            continue
        if t >= nf1:
            keys += [f'{t:.2f} f1', f'{t+1.2:.2f} f1']; t += 2.5; nf1 = t + r.choice([1, 2, 4])
            continue
        if c < 0.35:
            x, y = r.randrange(2, 318), r.randrange(2, 198)
            b = r.choice([1, 1, 1, 2, 3])
            mice += [f'{t:.2f} {x},{y},0', f'{t+0.3:.2f} {x},{y},{b}', f'{t+0.5:.2f} {x},{y},0']; t += 0.9
        elif c < 0.6:
            b = r.choice([1, 1, 2])
            dx, dy = r.choice([(-24, 0), (24, 0), (0, -16), (0, 16), (-24, -16), (24, 16)])
            mice.append(f'{t:.2f} {x},{y},{b}')
            for k in range(1, 4):
                x = min(317, max(2, x + dx)); y = min(197, max(2, y + dy))
                mice.append(f'{t+0.15*k:.2f} {x},{y},{b}')
            mice.append(f'{t+0.7:.2f} {x},{y},0'); t += 1.0
        elif c < 0.8:
            x = min(317, max(2, x + r.choice([-24, 24, 0, 0]))); y = min(197, max(2, y + r.choice([-16, 16, 0])))
            mice.append(f'{t:.2f} {x},{y},0'); t += 0.3
        else:
            keys.append(f'{t:.2f} ' + r.choice(['up', 'down', 'left', 'right', 'space', 'enter', 'space', 'enter', 'f1', 'f5', 'tab', 'a', 'm', '1'])); t += 0.4
    open('%s/%s.keys' % (D, name), 'w').write('\n'.join(keys) + '\n')
    open('%s/%s.mice' % (D, name), 'w').write('\n'.join(mice) + '\n')


def run(name, out, until, opts):
    st = '%s/st_%s' % (D, out)
    subprocess.run(['rm', '-rf', st])
    cmd = [sys.executable, 'doskit/tools/run.py', '-state', st, '-until', str(until),
           '-keys', '%s/%s.keys' % (D, name), '-mice', '%s/%s.mice' % (D, name)] + opts + ['HL.EXE']
    with open('%s/%s.log' % (D, out), 'w') as f:
        subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT, timeout=7200)
    subprocess.run(['rm', '-rf', st])


def stage1(name, until):
    opts = []
    for a in ['0023', '0120', STEP_IN, STEP_OUT, STEP_END] + list(KINDS) + list(states()):
        opts += ['-log', 'S1E4B:' + a]
    run(name, name, until, opts)


def calls(name):
    """(time, side, SS:BP, entry, exit, result) of computer_input's calls,
    and (time, side, SS:BP, entry, exit, kind, state) of computer_step's
    (kind 0: the list's end; state: of the kind's handler, None for the
    kind 5)"""
    inputs, steps, cur, step, st = [], [], None, None, states()
    for line in open('%s/%s.log' % (D, name)):
        m = re.match(r'log HL\.EXE\+1E4B:(\w+) t=(\S+) hit=(\d+) regs .*?AX=(\w+) .* SI=(\w+) DI=(\w+) BP=(\w+) .* SS=(\w+)', line)
        if not m:
            continue
        a, t, hit, ax, si, di, bp, ss = m.groups()
        t = float(t)
        if a == '0023':
            cur = (t, int(di, 16), '%s:%s' % (ss, bp), 'S1E4B:0023#' + hit, 'S1E4B:011D#' + hit)
        elif a == '0120' and cur:
            inputs.append(cur + (int(ax, 16),))
            cur = None
        elif a == STEP_IN:
            step = [t, int(si, 16), '%s:%s' % (ss, bp), 'S1E4B:%s#%s' % (a, hit), None, 0, None]
        elif a in KINDS and step:
            step[5] = KINDS[a]
        elif a in st and step and st[a][0] == step[5] and step[6] is None:
            step[6] = st[a][1]
        elif a in (STEP_OUT, STEP_END) and step:
            step[4] = 'S1E4B:%s#%s' % (a, hit)
            steps.append(tuple(step))
            step = None
    return inputs, steps


def spread(items, n):
    step = max(1, len(items) // n)
    return items[step // 2::step][:n]


def main():
    args = sys.argv[1:]
    until = int(args[args.index('-until') + 1]) if '-until' in args else 1500
    early = float(args[args.index('-early') + 1]) if '-early' in args else 150.0
    os.chdir(ROOT)
    os.makedirs(D, exist_ok=True)
    if 'stage2' not in args:
        for name, run_ in RUNS.items():
            play(name, run_[0], run_[1], until)
        with ThreadPoolExecutor(len(RUNS)) as ex:
            list(ex.map(lambda n: stage1(n, until), RUNS))
    jobs, lines, keep = [], [], 'stage2' in args

    def want(name, kind, c):
        t, side, ssbp, a_in, a_out = c[:5]
        base = '%s_%s_%s' % (name, kind, a_in.split('#')[1])
        for way, a in (('e', a_in), ('x', a_out)):
            ram = '%s/%s%s.ram' % (D, base, way)
            if not (keep and os.path.exists(ram)):
                jobs.append((name, base + way, int(t) + 5, ['-break', a, '-ram', ram]))
        lines.append('%s %d %s ../%s/%se.ram ../%s/%sx.ram' % (kind, side, ssbp, D, base, D, base))

    for name in RUNS:
        inputs, steps = calls(name)
        first = [c for c in inputs if c[0] <= early]
        chosen = spread(first, INPUT_SPREAD)
        script = []
        for i, c in enumerate(first):
            if c[5] == 0:
                script += [c] + first[i + 1:i + 2]
        for c in spread(list(dict.fromkeys(script)), INPUT_SCRIPT):
            if c not in chosen:
                chosen.append(c)
        for c in chosen if RUNS[name][3] else []:
            want(name, 'input', c)
        count = {}
        for c in steps:
            count.setdefault((c[5], c[6]), []).append(c)
        for (kind, state), of in sorted(count.items(), key=lambda x: (x[0][0], x[0][1] or 0)):
            if kind not in RUNS[name][2]:
                continue
            n = ENDS if kind == 0 else PER_STATE
            soon = [c for c in of if c[0] <= 2 * early]
            for c in (spread(soon, n) if len(soon) >= n else of[:n]):
                want(name, 'stepend' if kind == 0 else 'step', c)
        count = {'%d/%s' % k: len(v) for k, v in sorted(count.items(), key=lambda x: (x[0][0], x[0][1] or 0))}
        print('%s: %d calls of computer_input, of computer_step by kind/state %s' % (name, len(inputs), count))
    with ThreadPoolExecutor(10) as ex:
        list(ex.map(lambda j: run(*j), jobs))
    open('%s/computer_test.args' % D, 'w').write('\n'.join(lines) + '\n')
    print('computer_dumps: %d calls in %s/computer_test.args' % (len(lines), D))


if __name__ == '__main__':
    main()
