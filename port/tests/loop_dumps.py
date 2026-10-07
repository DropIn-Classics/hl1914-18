#!/usr/bin/env python3
"""Makes what port/tests/loop_test.sh compares the port's game_loop pass
(port/src/loop.c) with.

    python3 port/tests/loop_dumps.py [KEYS [UNTIL]]

HL.EXE is run on the runner with a key script of port/tests/keys/
(default m8_site.keys, map 8 chosen as dumps.py does with MAP8; a script
whose name starts with m0 plays map 0) up to UNTIL emulated seconds
(default 145, before the first fight of m8_site, at 150 s).  One run logs
what a pass reads from outside the translated code, by registers at:

    066A:0EE9  a pass begins
    066A:0FC3  a side's part begins (SI the side)
    066A:1003  the key read in it (AL)
    0CB6:0658  side_input has found the record's keys (CX button, DX
               directions)
    066A:1243  the side's input word (DI; used for the computer's side)
    066A:118B, 10AF:009F, 066A:1190  the text the computer's part shows
               (show_message's AX at 009F between the call and its return)
    0CB6:0799, 08DA, 0AF5, 0C7C  view_step's tests of the input record's
               byte 8 (ZF), 0CB6:084D, 09C5, 0BDA those of byte 5
    066A:4147  80h set in game_state's word 2 by the computer's phase
    1E4B:0610, 063C, 0886, 0C78  computer_step sets 8 in its side's word 0
               of `sides` (BX the side times 17h)
    02E6:000F  attacks_run (to choose passes)

and writes build/loop/inputs.txt, a line a side's part: pass, side, key,
word, text, rec5, rec8, set80, set8 (hex; -1 not read).  Further runs dump memory
at the first pass (start.ram) and at the beginning of chosen passes
(pNNNN.ram, the state after pass NNNN-1), and loop_test.args lists them.
Existing dumps are kept; PASSES in the environment (numbers apart by
spaces) adds passes to dump.  Everything goes to build/loop/ (ignored); no
game data is kept in this file.
"""
import os, re, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
KIT = os.path.join(ROOT, 'doskit', 'tools', 'run.py')
OUT = os.path.join(ROOT, 'build', 'loop')

keys = sys.argv[1] if len(sys.argv) > 1 else 'm8_site.keys'
until = sys.argv[2] if len(sys.argv) > 2 else '145'
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
out = os.path.join(OUT, tag)
os.makedirs(out, exist_ok=True)

R8 = ('0CB6:0799', '0CB6:08DA', '0CB6:0AF5', '0CB6:0C7C')
R5 = ('0CB6:084D', '0CB6:09C5', '0CB6:0BDA')
SET8 = ('1E4B:0610', '1E4B:063C', '1E4B:0886', '1E4B:0C78')
LOGS = ('066A:0EE9', '066A:0FC3', '066A:1003', '0CB6:0658', '066A:1243', '066A:118B',
        '10AF:009F', '066A:1190', '066A:4147', '02E6:000F') + R8 + R5 + SET8


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

rows = []          # [pass, side, key, word, text, rec5, rec8, set80, human, set8]
row = None
passes = 0
fights = []        # passes attacks_run ran in
in_text = False
for line in open(log):
    m = re.match(r'log HL\.EXE\+(\w{4}:\w{4}) t=\S+ hit=\d+ regs (.*)', line)
    if not m:
        continue
    at, regs = m.group(1), dict(re.findall(r'([A-Z]+)=([0-9A-F]+)', m.group(2)))
    reg = lambda r: int(regs[r], 16)
    if at == '066A:0EE9':
        passes += 1
    elif at == '066A:0FC3':
        row = [passes, reg('SI'), 0, 0, 0xFFFF, -1, -1, 0, False, 0]
        rows.append(row)
    elif row is None:
        continue
    elif at == '066A:1003':
        row[2] = reg('AX') & 0xFF
    elif at == '0CB6:0658':
        row[3] = (reg('CX') & 0x10) | (reg('DX') & 0x0F)
        row[8] = True
    elif at == '066A:1243':
        if not row[8]:
            row[3] = reg('DI')      # the computer's word (a human's is the record's)
    elif at == '066A:118B':
        in_text = True
    elif at == '10AF:009F' and in_text:
        row[4] = reg('AX')
        in_text = False
    elif at == '066A:1190':
        in_text = False
    elif (at in R8 or at in R5) and row[8]:
        i = 6 if at in R8 else 5
        if row[i] == -1:
            row[i] = 0 if reg('F') & 0x40 else 0xFF
    elif at == '066A:4147':
        row[7] = 1
    elif at in SET8:
        if reg('BX') // 0x17 != row[1]:
            sys.exit('pass %d: 8 set in the word of side %d in side %d\'s part' % (passes, reg('BX') // 0x17, row[1]))
        row[9] = 1
    elif at == '02E6:000F':
        fights.append(passes)

# the computer's side: its word is DI; a human's is what side_input saw
with open(os.path.join(out, 'inputs.txt'), 'w') as f:
    for r in rows:
        f.write('%d %d %X %X %X %d %d %d %d\n' % (tuple(r[:8]) + (r[9],)))

extra = [int(x) for x in os.environ.get('PASSES', '').split()]
chosen = sorted(set([1, 50, 200, 400] + [p + 1 for p in fights] + [p for p in fights]
                    + [passes - 1] + extra))
chosen = [p for p in chosen if 1 <= p < passes]
have = sorted(int(f[1:5]) for f in os.listdir(out) if re.match(r'p\d{4}\.ram$', f))
lines = []
for p in chosen:
    name = 'start.ram' if p == 1 else 'p%04d.ram' % p
    path = os.path.join(out, name)
    if not os.path.exists(path):
        run(['-break', 'S066A:0EE9#%d' % p, '-ram', path])
for p in sorted(set(chosen + have)):
    if p > 1:
        path = os.path.join(out, 'p%04d.ram' % p)
        lines.append('%d %s' % (p, os.path.relpath(path, os.path.join(ROOT, 'port'))))
with open(os.path.join(out, 'loop_test.args'), 'w') as f:
    f.write('%s %s\n' % (os.path.relpath(os.path.join(out, 'start.ram'), os.path.join(ROOT, 'port')),
                         os.path.relpath(os.path.join(out, 'inputs.txt'), os.path.join(ROOT, 'port'))))
    f.write('\n'.join(lines) + '\n')
print('%d passes logged, attacks_run in passes %s; dumps at %s' % (passes, fights, chosen))
