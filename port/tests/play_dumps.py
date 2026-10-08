#!/usr/bin/env python3
"""Makes what port/tests/play_test.sh compares the port's own play of a
battle (port/src/play.c) with.

    python3 port/tests/play_dumps.py [KEYS [UNTIL]]

HL.EXE is run on the runner with a key script of port/tests/keys/
(default m8_site.keys, map 8 chosen in the menu as loop_dumps.py does; a
script whose name starts with m0 plays map 0) up to UNTIL emulated seconds
(default 400).  One run logs, with their times:

    27FE:008C  the routine the timer calls (72.8 times a second)
    066A:0EE9  a pass begins
    066A:0FC3  a side's part begins (SI the side)
    066A:1003  the key taken in it (AL)
    0CB6:05F1  side_input is called
    0CB6:0658  side_input has found the record's keys (CX button, DX
               directions)
    0CB6:0750  view_step is called
    0CB6:0799, 08DA, 0AF5, 0C7C  view_step's tests of the input record's
               byte 8 (ZF), 0CB6:084D, 09C5, 0BDA those of byte 5
    066A:10D6  computer_turn has returned in the loop that runs while the
               computer thinks
    27FE:04F4  the keyboard's interrupt
    2C95:0024, 004A  clock_seed sets rand_words, to its argument (AX, DX)
               or to the BIOS's count of ticks (DX, CX)
    2C9A:0008  rand_next
    0CB6:0075, 00C1, 00E2  the routine that waits (S0CB6:0036) has cleared
               the first input record's button and the key
    2532:0137  the fight's pictures have cleared the key
    02E6:000F  attacks_run

and writes into build/play/TAG/ (ignored):

    clock.txt   the original's clock for the port's program (-clock; the
                lines are described in port/src/play.c)
    inputs.txt  a line a side's part: pass, side, key, and for a human
                side the input word side_input found (hex; - for the
                computer's) and what view_step's first tests of the
                record's bytes 5 and 8 found (0 or 255, -1 not tested), as
                the port's -inputs writes them (it has the bytes whenever
                view_step is called)
    pNNNN.ram   memory at the beginning of chosen passes (the first, 50,
                200, 400, those attacks_run ran in and the ones after
                them, the last but one, and PASSES in the environment,
                numbers apart by spaces); existing dumps are kept
    passes.txt  their numbers

With NOPICS=1 in the environment the fights are not shown as pictures
(no_fight_pics, S37FF:0115, set at the first pass as loop_dumps.py does)
and the files go to build/play/TAG_nopics/.

No game data is kept in this file.
"""
import os, re, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
KIT = os.path.join(ROOT, 'doskit', 'tools', 'run.py')

keys = sys.argv[1] if len(sys.argv) > 1 else 'm8_site.keys'
until = sys.argv[2] if len(sys.argv) > 2 else '400'
keypath = os.path.join(HERE, 'keys', keys)
if keys.startswith('m0'):
    pre = ['-mouse', '22', '150,48,1', '-mouse', '22.2', '150,48,0',
           '-mouse', '31', '230,80,1', '-mouse', '31.2', '230,80,0']
else:
    pre = ['-mouse', '21', '150,73,1', '-mouse', '21.2', '150,73,0', '-key', '22', 'r',
           '-key', '22.5', 'a', '-key', '23', 't', '-key', '23.5', 'i', '-key', '24', 'o',
           '-key', '25', 'enter', '-mouse', '27', '150,48,1', '-mouse', '27.2', '150,48,0']
pre += ['-keys', keypath, '-until', until]
tag = os.path.splitext(keys)[0]
if os.environ.get('NOPICS'):
    # the fights without pictures: no_fight_pics is set when the first pass
    # begins (a poke, not the game's own way of setting it)
    pre += ['-poke', 'S066A:0EE9#1', 'S37FF:0115', '01']
    tag += '_nopics'
out = os.path.join(ROOT, 'build', 'play', tag)
os.makedirs(out, exist_ok=True)

R8 = ('0CB6:0799', '0CB6:08DA', '0CB6:0AF5', '0CB6:0C7C')
R5 = ('0CB6:084D', '0CB6:09C5', '0CB6:0BDA')
WAITS = ('0CB6:0075', '0CB6:00C1', '0CB6:00E2')
LOGS = ('27FE:008C', '066A:0EE9', '066A:0FC3', '066A:1003', '0CB6:05F1', '0CB6:0658', '0CB6:0750',
        '066A:10D6', '2C95:0024', '2C95:004A', '02E6:000F', '27FE:04F4', '2C9A:0008',
        '2532:0137') + R8 + R5 + WAITS


def run(args):
    r = subprocess.run(['python3', KIT] + pre + args + ['HL.EXE'],
                       capture_output=True, text=True, cwd=out)
    return r.stdout + r.stderr


log = os.path.join(out, 'log.txt')
if not os.path.exists(log):
    a = []
    for l in LOGS:
        a += ['-log', 'S' + l]
    open(log, 'w').write(run(a))

clock = []
rows = []           # [pass, side, key, word or None, byte 5, byte 8]
row = None
passes = 0
rounds = []         # passes attacks_run ran in
for line in open(log):
    m = re.match(r'log HL\.EXE\+(\w{4}:\w{4}) t=(\S+) hit=\d+ regs (.*)', line)
    if not m:
        continue
    at, t, regs = m.group(1), m.group(2), dict(re.findall(r'([A-Z]+)=([0-9A-F]+)', m.group(3)))
    reg = lambda r: int(regs[r], 16)
    if at == '27FE:008C':
        clock.append('%s T' % t)
    elif at == '066A:0EE9':
        passes += 1
    elif at == '2C95:0024':
        clock.append('%s S %X' % (t, reg('DX') << 16 | reg('AX')))
    elif at == '2C95:004A':
        clock.append('%s S %X' % (t, reg('CX') << 16 | reg('DX')))
    elif at == '066A:0FC3':
        row = [passes, reg('SI'), 0, None, -1, -1]
        rows.append(row)
        clock.append('%s P %d %d' % (t, passes, reg('SI')))
    elif at == '27FE:04F4':
        clock.append('%s K' % t)
    elif at == '2C9A:0008':
        clock.append('%s R' % t)
    elif at in WAITS:
        clock.append('%s W' % t)
    elif at == '2532:0137':
        clock.append('%s X' % t)
    elif row is None:
        continue
    elif at == '066A:1003':
        row[2] = reg('AX') & 0xFF
    elif at == '0CB6:05F1':
        clock.append('%s I' % t)
    elif at == '0CB6:0658':
        row[3] = (reg('CX') & 0x10) | (reg('DX') & 0x0F)
    elif at == '0CB6:0750':
        clock.append('%s V' % t)
    elif at == '066A:10D6':
        clock.append('%s C' % t)
    elif (at in R8 or at in R5) and row[3] is not None:
        i = 5 if at in R8 else 4
        if row[i] == -1:
            row[i] = 0 if reg('F') & 0x40 else 255
    elif at == '02E6:000F':
        rounds.append(passes)
        clock.append('%s A' % t)

with open(os.path.join(out, 'clock.txt'), 'w') as f:
    f.write('\n'.join(clock) + '\n')
with open(os.path.join(out, 'inputs.txt'), 'w') as f:
    for p, s, k, w, r5, r8 in rows:
        f.write('%d %d %X %s\n' % (p, s, k, '-' if w is None else '%X %d %d' % (w, r5, r8)))

extra = [int(x) for x in os.environ.get('PASSES', '').split()]
chosen = sorted(set([1, 50, 200, 400] + rounds + [p + 1 for p in rounds] + [passes - 1] + extra))
chosen = [p for p in chosen if 1 <= p < passes]


def dump(p):
    path = os.path.join(out, 'p%04d.ram' % p)
    if not os.path.exists(path):
        run(['-break', 'S066A:0EE9#%d' % p, '-ram', path])


with ThreadPoolExecutor(8) as pool:
    list(pool.map(dump, chosen))
have = sorted(int(f[1:5]) for f in os.listdir(out) if re.match(r'p\d{4}\.ram$', f))
with open(os.path.join(out, 'passes.txt'), 'w') as f:
    f.write(' '.join(str(p) for p in have) + '\n')
print('%d passes logged, attacks_run in passes %s; dumps at %s' % (passes, rounds, have))
