#!/usr/bin/env python3
"""Compares the joysticks' words and the mouse's record in the dumps of a
play (port/tests/options_test.sh), which play_cmp.py counts as
"elsewhere" (DATA is not in the game's state).

    python3 port/tests/options_cmp.py TAG
    python3 port/tests/options_cmp.py TAG poke   the port's -poke arguments
                                                 for the input records of
                                                 the original's first dump

In build/play/TAG/: pNNNN.ram the original's memory and qNNNN.ram the
port's at the passes of passes.txt.  Compared at each: DATA:0380 up to
03BC (the joysticks: which are there, what was measured, the ranges, the
words F5 and F6 switch), the two sides' input records with what the
timer's routine keeps for them (03BC up to 0414) and the mouse's record
(0414 up to 045C).  The pointer's place in that record and where it was
drawn last (0424 up to 042C) are compared only from the first pass after
the mouse's script has put the pointer somewhere (the first line of
port/tests/keys/TAG.mice): before that the original's is the runner's
mouse and the port's the one its menu left.  Exit status 0 when no byte
differs.
"""
import os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
tag = sys.argv[1]
out = os.path.join(ROOT, 'build', 'play', tag)
data = None
for line in open(os.path.join(ROOT, 'port', 'src', 'gen', 'names.h')):
    m = re.match(r'#define HL_DATA 0x([0-9A-F]+)u', line)
    if m:
        data = (int(m.group(1), 16) + 0x76) << 4

if len(sys.argv) > 2 and sys.argv[2] == 'poke':
    # the port's -poke for the input records of the original's first dump
    x = open(os.path.join(out, 'p0001.ram'), 'rb').read()
    print('%X %s' % (data + 0x03BC, x[data + 0x03BC:data + 0x0414].hex()))
    sys.exit(0)

# the first pass at which the mouse's place is the script's in both
placed = None
mice = os.path.join(HERE, 'keys', tag + '.mice')
if os.path.exists(mice):
    first = min(float(l.split()[0]) for l in open(mice) if l[:1].isdigit())
    for line in open(os.path.join(out, 'clock.txt')):
        f = line.split()
        if len(f) > 2 and f[1] == 'P' and float(f[0]) >= first + 0.1:
            placed = int(f[2])
            break

bad = n = 0
for p in open(os.path.join(out, 'passes.txt')).read().split():
    q = os.path.join(out, 'q%04d.ram' % int(p))
    if not os.path.exists(q):
        print('pass %s: the port has no dump' % p)
        bad += 1
        continue
    x = open(os.path.join(out, 'p%04d.ram' % int(p)), 'rb').read()
    y = open(q, 'rb').read()
    d = [o for o in range(0x0380, 0x045C)
         if x[data + o] != y[data + o] and not (0x0424 <= o < 0x042C and (placed is None or int(p) < placed))]
    n += 1
    if d:
        bad += 1
        print('pass %s: %d bytes of DATA differ: %s' % (p, len(d), ' '.join('%04X' % o for o in d[:24])))
print('%s: DATA 0380 to 045C (the pointer\'s place from pass %s) at %d passes: %s'
      % (tag, placed, n, 'different at %d' % bad if bad else 'the same at every one'))
sys.exit(1 if bad else 0)
