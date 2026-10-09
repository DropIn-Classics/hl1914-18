"""make_m60.py ROUND... > m60_x.keys - a key script for map 60 that
plays m60_fights.keys' walk (its keys before 1454.5 s: the two cavalries
next to each other, the first side's at (46,19), the second's at (46,18))
and then ROUNDs of its own, one every 19.1 s from 1454.5 s, each with
F1 at 12.1 s and 12.9 s into it.  A ROUND is:

  A       the first side attacks: m60_fights' round at 1454.5 s
  B       the second side attacks: its round at 1473.6 s
  N       neither: both end their halves (A's last part)
  SxT,... make_two.py's steps, T seconds into the round: u d L R a tap
          of 0.06 s, a the button, m (button and up), E (button and
          left) with the direction let go 0.03 s after the button

A and B leave the attacker's cursor on the other unit; the next A or B
puts it back with a tap (6.6 s into the round for the other side's,
1 s before it for its own), as m60_fights does.  Behind the last round
the end's two texts are answered with Space at the two offsets of
`end=T1,T2` (seconds after the round that would follow; default 7,13).
A side attacks only every other round (in its attack half: the first
side in A's rounds, the second in B's), so a side attacking twice needs
an odd number of rounds between (docs/HANDOFF.md).

m60_first.keys is `A B A N A B end=5,10`, m60_both.keys
`A B A R4 R5 N R7 R8 R7 R10 R11 end=6.4,12.4` with the steps port/README.md lists.
No game data is kept in this file.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ev = [(float(t), k) for t, k in (l.split() for l in open(os.path.join(HERE, 'm60_fights.keys')) if l.strip())]
A = [(t - 1454.5, k) for t, k in ev if 1454.5 <= t < 1473.6]
B = [(t - 1473.6, k) for t, k in ev if 1473.6 <= t < 1492.7]
core = lambda blk: [(t, k) for t, k in blk if not 6.5 < t < 6.7]    # without the other side's tap
BUTTON = {'1': 'space', '2': 'lalt'}
DIRS = {'1': {'L': 'left', 'R': 'right', 'u': 'up', 'd': 'down'},
        '2': {'L': 'x', 'R': 'v', 'u': 'd', 'd': 'c'}}

r = [e for e in ev if e[0] < 1454.5]
t0 = 1454.5
end = (7, 13)
args = [a for a in sys.argv[1:] if not a.startswith('end=')]
for a in sys.argv[1:]:
    if a.startswith('end='):
        end = tuple(float(x) for x in a[4:].split(','))
c1 = c2 = False         # a side's cursor left on the other unit
for i, c in enumerate(args):
    if c == 'A' and i == 0:
        r += [(t0 + t, k) for t, k in A]
        c1 = True
    elif c == 'A':
        if c1:
            r += [(t0 - 1.0, 'down+'), (t0 - 0.94, 'down-')]
        r += [(t0 + t, k) for t, k in core(A)]
        if c2:
            r += [(t0 + 6.6, 'd+'), (t0 + 6.66, 'd-')]
        c1, c2 = True, False
    elif c == 'B':
        if c2:
            r += [(t0 - 1.0, 'd+'), (t0 - 0.94, 'd-')]
        r += [(t0 + t, k) for t, k in core(B)]
        if c1:
            r += [(t0 + 6.6, 'down+'), (t0 + 6.66, 'down-')]
        c1, c2 = False, True
    elif c == 'N':
        r += [(t0 + t, k) for t, k in A if t >= 7.0]
    else:
        for it in c.split(','):
            s, x, t = it[0], it[1], t0 + float(it[2:])
            if x in 'udLR':
                r += [(t, DIRS[s][x] + '+'), (t + .06, DIRS[s][x] + '-')]
            elif x in 'mE':
                d = DIRS[s]['u' if x == 'm' else 'L']
                r += [(t, BUTTON[s] + '+'), (t + .3, d + '+'), (t + .7, BUTTON[s] + '-'), (t + .73, d + '-')]
            elif x == 'a':
                r += [(t, BUTTON[s])]
        r += [(t0 + 12.1, 'f1'), (t0 + 12.9, 'f1')]
    t0 += 19.1
r += [(t0 + end[0], 'space'), (t0 + end[1], 'space')]
r.sort()
for t, k in r:
    print('%.2f %s' % (t, k))
