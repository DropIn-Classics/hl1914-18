#!/usr/bin/env python3
"""Compares the fight's sound triggers of the port with the original's
(port/tests/fsound_test.sh calls it).

    python3 port/tests/fsound_cmp.py TAG ...

The original's: build/fsound/TAG/calls.txt (port/tests/fsound_dumps.py).
The port's: build/fsound/port_TAG.fx, written by its program with -fxlog
on the same key script and the original's clock (build/play/TAG/clock.txt,
play_dumps.py).  Both are lists of

    fight N [FRAMES]            the N-th fight shown as pictures
    play FRAME SLOT VOLUME TIMES   pics_play
    side FRAME SIDE             the channel stopped for a side without figures
    end CHANNEL                 a channel stopped at the pictures' end
    blast                       attacks_run's effect, a unit destroyed
                                without pictures

Fight by fight the port's triggers must be the original's, in order.  A
key ends the original's frames early; on the original's clock the port
plays all of a fight's frames (play.c), so its triggers behind the
original's last frame are counted and not compared.  The original's
TIMES is known only where the call's entry was dumped ("-" elsewhere).
Fights the port did not reach (its clock's file ends before the
original's run) are counted; of attacks_run's effect the calls both
have are compared.  Exit status 0 when nothing compared differs.
"""
import os, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
out = os.path.join(ROOT, 'build', 'fsound')


def parts(path):
    """the fights: [frames or None, [lines]], and the lines outside them"""
    fights, outside = [], []
    for line in open(path):
        f = line.split()
        if not f:
            continue
        if f[0] == 'fight':
            fights.append([int(f[2]) if len(f) > 2 else None, []])
        elif f[0].startswith('blast'):
            outside.append(line.strip())
        elif fights:
            fights[-1][1].append(f)
    return fights, outside


def same(a, b):
    return len(a) == len(b) and all(x == y or x == '-' for x, y in zip(a, b))


bad = 0
for tag in sys.argv[1:]:
    o, p = os.path.join(out, tag, 'calls.txt'), os.path.join(out, 'port_%s.fx' % tag)
    if not os.path.exists(o) or not os.path.exists(p):
        print('%s: no %s' % (tag, p if os.path.exists(o) else o))
        bad = 1
        continue
    (of, ob), (pf, pb) = parts(o), parts(p)
    n = known = beyond = wrong = 0
    kinds = {}
    for k, ((frames, a), (_, b)) in enumerate(zip(of, pf)):
        late = [x for x in b if x[0] in ('play', 'side') and int(x[1]) > frames]
        b = [x for x in b if x not in late]
        beyond += len(late)
        if len(a) != len(b) or not all(same(x, y) for x, y in zip(a, b)):
            wrong += 1
            first = [(x, y) for x, y in zip(a, b) if not same(x, y)][:1]
            print('%s: fight %d differs: %d triggers in the original, %d in the port; the first: %s'
                  % (tag, k + 1, len(a), len(b), first or 'the count'))
            continue
        n += len(a)
        known += sum(1 for x in a if x[0] == 'play' and x[4] != '-')
        for x in a:
            key = x[0] if x[0] != 'play' else 'slot %s' % x[2]
            kinds[key] = kinds.get(key, 0) + 1
    if ob[:len(pb)] != pb[:len(ob)]:
        wrong += 1
        print('%s: attacks_run\'s effect differs: original %s, port %s' % (tag, ob, pb))
    bad |= wrong != 0
    print('%s: %d fights with pictures in the original, %d in the port, %d compared and %d of them different; '
          '%d triggers the same in order (%s), the times known in %d; %d of the port\'s behind the original\'s last '
          'frame not compared; attacks_run\'s effect %d times in the original, %d in the port'
          % (tag, len(of), len(pf), min(len(of), len(pf)), wrong, n,
             ', '.join('%d %s' % (v, k) for k, v in sorted(kinds.items())) or 'none', known, beyond, len(ob), len(pb)))
sys.exit(bad)
