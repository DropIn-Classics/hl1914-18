"""make_hq.py aT|mT ... > m0_hq.keys - the key script of a map-0 run (port/tests/
side_dumps.sh): the first side moves a unit into its headquarters
(record 0 of sites_40) as m0_k4.keys does, and in each of its attack
halves enters the headquarters, tries to repair the unit there, leaves and
ends the half; in its move halves it only ends the half.  A half is ended
with the button and left on a plain field (field_menu's pointer 9) and
F1 twice.  The times were found from screenshots and the hits of
S131D:1745 (each round's end), not computed."""
import sys

ev = []


def at(t, *keys):
    for i, k in enumerate(keys):
        ev.append((t + i * 0.3 if k.endswith('+') else t + i * 0.3 + 0.02, k))


def gesture(t, d):        # the button held, a direction, both let go
    ev.extend([(t, 'space+'), (t + .3, d + '+'), (t + .7, 'space-'), (t + .9, d + '-')])


def tap(t, d):
    ev.extend([(t, d + '+'), (t + .04, d + '-')])


def end(t):               # on a plain field: pointer 9, then F1 twice
    gesture(t, 'left')
    ev.extend([(t + 2, 'f1'), (t + 2.8, 'f1')])


# the first move half: k4's move of the unit above the headquarters into it
tap(42, 'up'); tap(43, 'up'); gesture(44, 'up'); tap(46, 'down'); tap(46.5, 'down')
ev.extend([(47.5, 'space'), (49, 'space')])
tap(54, 'down'); end(55)
for arg in sys.argv[1:]:        # aT: an attack half at T, mT: a move half,
    # rT: the button and down alone
    t = float(arg[1:])
    if arg[0] == 'r':           # rT: only the repairing gesture
        gesture(t, 'down')
        continue
    if arg[0] == 'm':
        end(t)
        continue
    tap(t, 'up')
    gesture(t + 1, 'left')        # into the headquarters
    gesture(t + 4, 'down')        # repair the unit on the cursor's place
    gesture(t + 7, 'right')       # leave
    tap(t + 9, 'down')
    end(t + 10)
for t, k in sorted(ev):
    print('%.2f %s' % (t, k))
