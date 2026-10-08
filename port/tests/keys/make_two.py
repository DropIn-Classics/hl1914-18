"""make_two.py ARG... > m48_two.keys - the key script of a run with two
human sides (a map for two players, 48 to 71; port/tests/play_dumps.py
plays the map a script's name begins with, mNN_).  Each ARG is one of:

  T:U     from T to U seconds, every 15 s: the first side ends its half
          (Space and left, as make_m8.py's E), 1.5 s later the second
          side does the same with its own keys (the left Alt and X), F1 at
          5 s and at 5.8 s
  seed=N  with T:U: the second side also uses its other keys in the ten
          seconds between, one action a second chosen by a fixed sequence
          from N (not random between runs): a step of its cursor (X left,
          V right, D up, C down, held 0.2 s) or its button with a
          direction (the left Alt and one of the four), and the first
          side a step of its own (the arrows) every third second.  What
          these do on the map was not planned.
  SxT     a step of side S (1 or 2) at T seconds, x as in make_m8.py:
          u d L R a step of the cursor (held 0.08 s), a the button,
          m the button and up (the move), s and left, r and down, l and
          right, E the half ended (the button and left), f F1 (S ignored)
  SxT/H   the same with another length, H seconds: a step held H (not
          0.08), a gesture's direction let go H after the button (not
          0.2); for scripts that give the original what another clock
          sampled

m48_two.keys is `45:400`, m48_steps.keys `45:400 seed=7`, m48_move.keys,
m48_aim.keys, m48_build.keys, m54_end.keys, m57_site.keys and m57_last.keys what port/README.md says.  The second side's keys are those of its table
in HL.EXE (docs/HANDOFF.md, "The keyboard").  No game data is kept in
this file.
"""
import sys

ev = []
BUTTON = {'1': 'space', '2': 'lalt'}
DIRS = {'1': {'L': 'left', 'R': 'right', 'u': 'up', 'd': 'down'},
        '2': {'L': 'x', 'R': 'v', 'u': 'd', 'd': 'c'}}


def gesture(t, button, d, after=.2):  # the button held, a direction, both let go
    ev.extend([(t, button + '+'), (t + .3, d + '+'), (t + .7, button + '-'), (t + .7 + after, d + '-')])


def tap(t, d, held=.2):
    ev.extend([(t, d + '+'), (t + held, d + '-')])


n = 0


def pick(k):
    global n
    n = (n * 1103515245 + 12345) & 0x7FFFFFFF
    return (n >> 16) % k


args = sys.argv[1:]
seed = None
for arg in args:
    if arg.startswith('seed='):
        seed = n = int(arg[5:])
for arg in args:
    if arg.startswith('seed='):
        continue
    if ':' in arg:
        t, u = (float(x) for x in arg.split(':'))
        while t < u:
            gesture(t, 'space', 'left')
            gesture(t + 1.5, 'lalt', 'x')
            ev.extend([(t + 5, 'f1'), (t + 5.8, 'f1')])
            if seed is not None:
                for i in range(8):
                    s = t + 6.5 + i
                    if pick(3):
                        tap(s, 'xvdc'[pick(4)])
                    else:
                        gesture(s, 'lalt', 'xvdc'[pick(4)])
                    if i % 3 == 0:
                        tap(s + .5, ('left', 'right', 'up', 'down')[pick(4)])
            t += 15
        continue
    arg, _, held = arg.partition('/')
    side, c, t = arg[0], arg[1], float(arg[2:])
    d = DIRS[side]
    if c in d:
        tap(t, d[c], float(held or .08))
    elif c == 'a':
        ev.append((t, BUTTON[side]))
    elif c == 'f':
        ev.append((t, 'f1'))
    else:
        gesture(t, BUTTON[side], d[{'m': 'u', 's': 'L', 'E': 'L', 'r': 'd', 'l': 'R'}[c]], float(held or .2))
for t, k in sorted(ev):
    print('%.2f %s' % (t, k))
