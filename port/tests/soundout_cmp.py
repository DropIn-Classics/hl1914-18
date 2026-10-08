#!/usr/bin/env python3
"""The samples the port's program queued its AdLib writes for (-oplat)
beside the port's own clock at each write (-opllog), for
soundout_test.sh.

    python3 port/tests/soundout_cmp.py RUN.at RUN.opl

The two lists have the same writes in the same order.  A line "c T" of
the first says the port's time at the virtual device's sample 0, from
where it stands on.  A write's sample less the lead must not be more
than 20 ms behind the clock's time (the interrupts are made in one go
for the samples played since the clock was last asked: a call of the
port's timer routine and a millisecond at most) and not more than 3.5 s
ahead (music_fade makes a fade's interrupts at once, 20 steps at most).
The writes before the first such line (the start's, made before the
clock runs) are counted apart.
"""
import sys

at_file, opl_file = sys.argv[1], sys.argv[2]
lines = open(at_file).read().split('\n')
head = lines[0].split()
rate, lead = int(head[1]), int(head[3])
opl = [l.split() for l in open(opl_file) if l[0].isdigit()]
off = None
clocks = before = on = ahead = k = 0
worst_behind = worst_ahead = 0.0
for l in lines[1:]:
    a = l.split()
    if l.startswith('c '):
        off = float(a[1])
        clocks += 1
    if not l.startswith('w '):
        continue
    if k >= len(opl):
        raise SystemExit(f'soundout_cmp FAILED: more writes queued than the {len(opl)} logged')
    o = opl[k]
    if (int(a[2], 16), int(a[3], 16)) != (int(o[2], 16), int(o[3], 16)):
        raise SystemExit(f'soundout_cmp FAILED: write {k} is {a[2]}={a[3]} queued, {o[2]}={o[3]} logged')
    k += 1
    if off is None:
        before += 1
        continue
    t = float(o[0])
    d = (int(a[1]) - lead) / rate - (t - off)
    if d < -0.020 or d > 3.5:
        raise SystemExit(f'soundout_cmp FAILED: write {k - 1} at {t:.6f} s is queued {d * 1000:.1f} ms from the clock')
    worst_behind = min(worst_behind, d)
    worst_ahead = max(worst_ahead, d)
    if d > 0.001:
        ahead += 1
    else:
        on += 1
if k != len(opl):
    raise SystemExit(f'soundout_cmp FAILED: {k} writes queued, {len(opl)} logged')
print(f'soundout_cmp ok: {k} writes, the clock set {clocks} times, {before} writes before it, {on} within '
      f'{-worst_behind * 1000:.1f} ms behind the clock, {ahead} ahead ({worst_ahead * 1000:.0f} ms at most)')
