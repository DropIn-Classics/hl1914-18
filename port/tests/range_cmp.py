#!/usr/bin/env python3
"""Compares a range of DATA in the dumps of a play, which play_cmp.py
counts as "elsewhere" (it compares the game's state, and DATA is not in
it): the joysticks' words and the mouse's record that F5 to F8 change.

    python3 port/tests/range_cmp.py TAG LO HI [LO HI]...

In build/play/TAG/: pNNNN.ram the original's memory and qNNNN.ram the
port's at the passes of passes.txt; each DATA:LO up to DATA:HI (hex, HI not
included) is compared at each.  Exit status 0 when no byte differs.
"""
import os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
tag = sys.argv[1]
ranges = [(int(sys.argv[i], 16), int(sys.argv[i + 1], 16)) for i in range(2, len(sys.argv) - 1, 2)]
out = os.path.join(ROOT, 'build', 'play', tag)
data = None
for line in open(os.path.join(ROOT, 'port', 'src', 'gen', 'names.h')):
    m = re.match(r'#define HL_DATA 0x([0-9A-F]+)u', line)
    if m:
        data = (int(m.group(1), 16) + 0x76) << 4
bad = 0
for p in open(os.path.join(out, 'passes.txt')).read().split():
    x = open(os.path.join(out, 'p%04d.ram' % int(p)), 'rb').read()
    y = open(os.path.join(out, 'q%04d.ram' % int(p)), 'rb').read()
    d = [o for lo, hi in ranges for o in range(lo, hi) if x[data + o] != y[data + o]]
    if d:
        bad += 1
        print('pass %s: %d bytes of DATA differ: %s' % (p, len(d), ' '.join('%04X' % o for o in d[:16])))
print('DATA %s: %s' % (', '.join('%04X to %04X' % r for r in ranges),
                       'different at %d passes' % bad if bad else 'the same at every pass compared'))
sys.exit(1 if bad else 0)
