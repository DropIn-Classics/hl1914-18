#!/usr/bin/env python3
"""Compares the joysticks' words and the mouse's record in the dumps of a
play (port/tests/options_test.sh), which play_cmp.py counts as
"elsewhere" (DATA is not in the game's state).

    python3 port/tests/options_cmp.py TAG
    python3 port/tests/options_cmp.py TAG poke   the port's -poke arguments
                                                 for the input records of
                                                 the original's first dump
    python3 port/tests/options_cmp.py TAG_map map
    python3 port/tests/options_cmp.py TAG_own own

In build/play/TAG/: pNNNN.ram the original's memory and qNNNN.ram the
port's at the passes of passes.txt.  Compared at each: DATA:0380 up to
03BC (the joysticks: which are there, what was measured, the ranges, the
words F5 and F6 switch), the two sides' input records with what the
timer's routine keeps for them (03BC up to 0414) and the mouse's record
(0414 up to 045C).  The pointer's place in that record and where it was
drawn last (0424 up to 042C) are compared only from the first pass after
the mouse's script has put the pointer somewhere (the first line of
port/tests/keys/TAG.mice): before that the original's is the runner's
mouse and the port's the one its menu left.  With `map` (the port begun
with -map, options_test.sh map) the range the menu gives the mouse's
pointer (043A up to 0442) is told apart; with `own` (the port on its own
clock) the dumps are those of rounds.txt, the original's pass and the
port's of each time, and the sides' counts of gaps are told apart where
they are one step apart (they go round with every fourth call of the
timer's routine, and the two passes do not begin at the same moment) or
further (they run on from the menu, whose time is not the original's in
the port, and a count stands still while its direction is held, so a
hold read once more or less moves it); and what each side's parts read
is listed hold by hold for both programs, the input word and for how
many passes in a row (a count of holds that one of the two still has
from the last hold and the other has at 0 already is told apart too, with
bytes 6 and 7 of the record when it is the button's): the words must be the same in the same order, the
passes are counted apart where they differ (a hold is read every 0.0549
s, once more or less by where in such a period it began).
Exit status 0 when no byte differs.
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
mice = os.path.join(HERE, 'keys', re.sub(r'_(map|own)$', '', tag) + '.mice')
if os.path.exists(mice):
    first = min(float(l.split()[0]) for l in open(mice) if l[:1].isdigit())
    for line in open(os.path.join(out, 'clock.txt')):
        f = line.split()
        if len(f) > 2 and f[1] == 'P' and float(f[0]) >= first + 0.1:
            placed = int(f[2])
            break

mode = sys.argv[2] if len(sys.argv) > 2 else ''
# what is told apart: with `map` the range of the mouse's pointer (043A up
# to 0442), which the menu sets and a port begun with -map never does
skip = range(0x043A, 0x0442) if mode == 'map' else ()
if mode == 'own':
    # the port on its own clock: the original's pass and the port's of
    # each time (rounds.txt, options_test.sh)
    pairs = [(int(o), 'r%04d.ram' % int(q)) for _, o, q in
             (l.split() for l in open(os.path.join(out, 'rounds.txt')))]
else:
    pairs = [(int(p), 'q%04d.ram' % int(p)) for p in open(os.path.join(out, 'passes.txt')).read().split()]
bad = n = steps = far = left = 0
GAPS = [b + 4 * i for b in (0x03D4, 0x0400) for i in range(5)]
for p, port in pairs:
    q = os.path.join(out, port)
    if not os.path.exists(q):
        print('pass %s: the port has no dump' % p)
        bad += 1
        continue
    x = open(os.path.join(out, 'p%04d.ram' % int(p)), 'rb').read()
    y = open(q, 'rb').read()
    d = [o for o in range(0x0380, 0x045C)
         if x[data + o] != y[data + o] and o not in skip
         and not (0x0424 <= o < 0x042C and (placed is None or int(p) < placed))]
    if mode == 'own':
        # the ten counts of gaps (03D4, 0400 and every fourth byte behind
        # them) go round 0 to 10 with every fourth call of the timer's
        # routine: a step apart when the two passes begin on either side
        # of such a call
        g = [o for o in d if o in GAPS and (x[data + o] - y[data + o]) % 11 in (1, 10)]
        if g:
            steps += 1
        # further apart: their phase from the menu, and the holds read
        # once more or less
        if [o for o in d if o in GAPS and o not in g]:
            far += 1
        d = [o for o in d if o not in GAPS]
        # a count of holds stays as its hold left it until its count of
        # gaps has gone round, and then both are 0: left over in one of
        # the two and 0 already in the other by that count's phase; and
        # with the button's so, bytes 6 and 7 of the side's record (how
        # far a second press has come, cleared with it)
        h = [o for o in d if o + 2 in GAPS and (x[data + o:data + o + 2] == b'\0\0' or y[data + o:data + o + 2] == b'\0\0')]
        for count, rec in ((0x03E2, 0x03C8), (0x040E, 0x03F4)):
            if count in h:
                h += [o for o in d if o in (rec + 6, rec + 7)]
        if h:
            left += 1
            d = [o for o in d if o not in h]
    n += 1
    if d:
        bad += 1
        print('pass %s%s: %d bytes of DATA differ: %s' % (p, ' (the port\'s %s)' % int(port[1:5]) if mode == 'own' else '',
                                                         len(d), ' '.join('%04X' % o for o in d[:24])))
print('%s: DATA 0380 to 045C (the pointer\'s place from pass %s%s) at %d passes: %s%s'
      % (tag, placed, ', the pointer\'s range told apart' if skip else '', n,
         'different at %d' % bad if bad else 'the same at every one',
         '; the counts of gaps a step apart at %d of them, further at %d, a count of holds left over in one at %d'
         % (steps, far, left) if mode == 'own' else ''))
if mode == 'own':
    # what each side read, hold by hold: [first pass, input word, passes]
    def holds(name):
        runs, last = {'0': [], '1': []}, {}
        for line in open(os.path.join(out, name)):
            f = line.split()
            if last.get(f[1]) == f[3]:
                runs[f[1]][-1][2] += 1
            else:
                runs[f[1]].append([int(f[0]), f[3], 1])
                last[f[1]] = f[3]
        return {s: [r for r in v if r[1] not in ('0', '-')] for s, v in runs.items()}
    a, b = holds('inputs.txt'), holds('own_inputs.txt')
    for s in '01':
        words = [r[1] for r in a[s]] == [r[1] for r in b[s]]
        apart = [(x, y) for x, y in zip(a[s], b[s]) if x[2] != y[2]]
        print('side %s: %d holds read by the original, %d by the port, the input words %s; the passes of %d differ'
              % (s, len(a[s]), len(b[s]), 'the same in the same order' if words else 'DIFFERENT', len(apart)))
        print('  input word x passes, the original: ' + ' '.join('%sx%d' % (r[1], r[2]) for r in a[s]))
        print('  input word x passes, the port:     ' + ' '.join('%sx%d' % (r[1], r[2]) for r in b[s]))
        if not words or any(abs(x[2] - y[2]) > 1 for x, y in apart):
            bad += 1
sys.exit(1 if bad else 0)
