"""make_m8.py STEP... > m8_site.keys - the key script of a map-8 run
(port/tests/side_dumps.sh) for the first side, from where its view starts
(one row below its headquarters), or steps appended to another script
(m8_d5r.keys: m8_d5.keys and its steps, see side_dumps.sh).  Each STEP is a letter and a time:
  eT  end the half (the button and left on a plain field, F1 twice)
  ET  the same without the F1s
  fT  F1 alone (a round's end waits for a key once the computer is done)
  gT  go from the start field to record 1 of sites_400 (4 up, 4 right,
      one step a second: map offset B2h, one of the record's fields)
  bT  4 left, 4 down
  sT  the button and left: into the site under the cursor, or in a site's
      place menu the build list (on a free place) and, in it, building
  rT  the button and down: repair the unit on the place
  oT  the button and up: move the unit on the place out
  lT  the button and right: leave the place menu / the site
  dT, uT, LT, RT  one step down, up, left, right
The times were found from screenshots and hits of S131D:1745 and of the
compared blocks, not computed; the computer's turn decides when a half
begins."""
import sys

ev = []


def tap(t, d):
    ev.extend([(t, d + '+'), (t + .08, d + '-')])


def gesture(t, d):        # the button held, a direction, both let go
    ev.extend([(t, 'space+'), (t + .3, d + '+'), (t + .7, 'space-'), (t + .9, d + '-')])


for arg in sys.argv[1:]:
    c, t = arg[0], float(arg[1:])
    if c in 'eE':
        gesture(t, 'left')
        if c == 'e':
            ev.extend([(t + 2, 'f1'), (t + 2.8, 'f1')])
    elif c == 'f':
        ev.append((t, 'f1'))
    elif c in 'gb':
        for d in (['up'] * 4 + ['right'] * 4 if c == 'g' else ['left'] * 4 + ['down'] * 4):
            tap(t, d)
            t += 1
    elif c == 'a':
        ev.append((t, 'space'))
    elif c in 'srolm':
        gesture(t, {'s': 'left', 'r': 'down', 'o': 'up', 'm': 'up', 'l': 'right'}[c])
    else:
        tap(t, {'d': 'down', 'u': 'up', 'L': 'left', 'R': 'right'}[c])
for t, k in sorted(ev):
    print('%.2f %s' % (t, k))
