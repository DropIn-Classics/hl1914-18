#!/usr/bin/env python3
"""The port's rendered AdLib sound beside the original's as the runner's
chip played it, tune by tune (port/tests/oplwav_test.sh calls it).

    python3 port/tests/oplwav_cmp.py WALK

The original's: build/oplwav/WALK.orig and build/oplwav/WALK/orig.wav
(oplwav_dumps.py; oplwav_ref fit has shown the file to be that list's
sound).  The port's: build/oplwav/WALK.port.at, .opl and .wav (its
headless program with DK_WAV, -oplat and -opllog on the same keys;
soundout_ref has shown the file to be that list's sound).

A tune is what is written from the end of a music_start that started
one to the next music_fade.  The port's tunes are paired in order with
the original's that begin with the same registers (the first tick's).
The two programs' calls do not fall on the same moments (the game's
side, not the sound's), so a tune is laid on the other at its first
tick, and the count of ticks from there is what the writes are compared
by.  For each pair:

- the writes the timer's routine made, by OPL voice: adl_tick's must
  be the same register and value at the same tick; fade_tick's (a
  fade's steps count from the game's call of it) the same registers and
  values in the same order, told how many ticks apart.  Judged in a
  tune during which the game's side wrote no register in either
  program: every voice, up to the shorter one's end.  In the others
  told only: an effect is started at another tick (the game's side),
  takes voices from the tune and gives them back at another tick, and
  a tune started without a fade begins at the levels the voices' own
  volumes of the tune before give;
- the time: the port's ticks come at the timer's periods summed
  (judged, to the sample).  The original's fall behind: the timer's
  interrupt is taken once when it has been kept waiting, the others due
  meanwhile are lost.  Judged: its second tick is late by what the
  first one (the instruments' registers, a tenth of a millisecond each)
  ran beyond a period, within a period.  Told: how far behind it is at
  the end and at how many ticks it fell behind for good, how many of
  them behind a tick of the driver that ran longer than a period (the
  others: the game's side or another routine kept the interrupt
  waiting, on the runner's machine);
- the sound: over the ticks before the first write that differs, in
  windows of WINDOW ticks, the files' levels (root mean square).  The
  files cannot be compared sample for sample: they are at 49716 and
  44100 Hz, the original's writes of a tick are spread over it where
  the port's fall on one sample, and its ticks come at other moments.
  So two more files are rendered from the original's list
  (oplwav_ref render): the same writes at the same moments on a chip
  at 44100 Hz, and that with each tick's writes on the sample of its
  first, which is the port's way; what is left between that file and
  the port's are the ticks' moments.  Told: each step's change of
  level.  Judged: the rate's step within RATE_DB in every window, the
  port's level against the original's within MEAN_DB on average and
  LEVEL_DB in every window (windows where the original is below QUIET
  are left out, and must be below it in the port too).
"""
import math, os, subprocess, sys
from collections import Counter

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
REF = os.path.join(ROOT, 'port', 'build', 'oplwav_ref')
PIT_HZ = 1193182
WINDOW = 350            # ticks, half a second at the menu's 700 a second
RATE_DB = 0.2
MEAN_DB = 0.5
LEVEL_DB = 3.0
QUIET = 50            # of 32768

SLOT = [0, 1, 2, 0, 1, 2, None, None, 3, 4, 5, 3, 4, 5, None, None, 6, 7, 8, 6, 7, 8]


def voice(reg):
    """the OPL voice a register belongs to, or 'chip'"""
    if 0xA0 <= reg <= 0xA8 or 0xB0 <= reg <= 0xB8 or 0xC0 <= reg <= 0xC8:
        return reg & 15
    if 0x20 <= reg <= 0x95 or 0xE0 <= reg <= 0xF5:
        off = reg & 31
        if off < len(SLOT) and SLOT[off] is not None:
            return SLOT[off]
    return 'chip'


def original(path):
    """the ticks as [begin, end] (s) and the tunes: each a list of writes
    (time, register, value, who made it: 0 the game's side, 1 adl_tick,
    2 fade_tick, ticks so far)"""
    ticks, tunes, cur, inside = [], [], None, 0
    for line in open(path):
        w = line.split()
        t = int(w[0]) / 1e6
        if w[1] == 't':
            ticks.append([t, t])
        elif w[1] == 'e':
            ticks[-1][1] = t
        if w[1] in ('t', 'ft'):
            inside = 1 if w[1] == 't' else 2
        elif w[1] in ('e', 'fe'):
            inside = 0
        elif w[1] == 'start':
            cur = []
            tunes.append(cur)
        elif w[1] == 'fade':
            cur = None
        elif w[1] == 'w' and cur is not None:
            cur.append((t, int(w[2], 16), int(w[3], 16), inside, len(ticks)))
    return ticks, tunes


def port(at, opl):
    """the rate, and the tunes as the original's; a write's time is its
    frame in the file (the lead in it) / rate"""
    lines = open(at).read().split('\n')
    rate = int(lines[0].split()[1])
    queued = [l.split() for l in lines if l.startswith('w ')]
    period = {}             # an interrupt's frame -> the timer's period behind it
    lead = int(lines[0].split()[3])
    for l in lines:
        if l.startswith('i '):
            a = l.split()
            period[int(a[1]) + lead] = int(a[2])
    tunes, cur, k = [], None, 0
    for line in open(opl):
        w = line.split()
        if w[0] == 'start':
            cur = []
            tunes.append(cur)
        elif w[0] in ('fade', 'fading'):
            cur = None
        else:
            q = queued[k]
            k += 1
            if (int(q[2], 16), int(q[3], 16)) != (int(w[2], 16), int(w[3], 16)):
                raise SystemExit('oplwav_cmp FAILED: the port\'s two lists differ at write %d' % k)
            if cur is not None:
                cur.append((int(q[1]) / rate, int(q[2], 16), int(q[3], 16), int(q[4]), int(w[1])))
    return rate, period, tunes


def levels(wav, windows):
    path = os.path.join(ROOT, 'build', 'oplwav', 'windows.txt')
    with open(path, 'w') as f:
        for a, b in windows:
            f.write('%d %d\n' % (a, b))
    out = subprocess.run([REF, 'level', wav, path], capture_output=True, text=True).stdout.split('\n')
    return [float(l) for l in out if l]


def rendered(walk, rate, together):
    """the original's list on a chip at `rate`, with each tick's writes on
    the sample of its first when asked: the file's name"""
    d = os.path.join(ROOT, 'build', 'oplwav')
    name = os.path.join(d, '%s.orig%d%s' % (walk, rate, 't' if together else ''))
    out, first, t = [], None, 0
    for line in open(os.path.join(d, walk + '.orig')):
        w = line.split()
        t = int(w[0])
        if w[1] in ('t', 'ft'):
            first = None
        elif w[1] == 'w':
            at = t
            if together and w[4] == '1':
                first = t if first is None else first
                at = first
            out.append('%d %s %s' % (at * rate // 1000000, w[2], w[3]))
    with open(name + '.lst', 'w') as f:
        f.write('\n'.join(out) + '\n')
    subprocess.run([REF, 'render', str(rate), str(t * rate // 1000000), name + '.lst', name + '.wav'], check=True)
    return name + '.wav'


def step(a, b, quiet):
    db = [20 * math.log10(y / x) for x, y, q in zip(a, b, quiet) if not q]
    return sum(db) / len(db), min(db), max(db)


def main():
    walk = sys.argv[1]
    d = os.path.join(ROOT, 'build', 'oplwav')
    ticks, otunes = original(os.path.join(d, walk + '.orig'))
    rate, period, ptunes = port(os.path.join(d, walk + '.port.at'), os.path.join(d, walk + '.port.opl'))
    owav, pwav = os.path.join(d, walk, 'orig.wav'), os.path.join(d, walk + '.port.wav')
    rwav, twav = rendered(walk, rate, False), rendered(walk, rate, True)
    bad = 0
    pairs, k = [], 0
    def head(tune):
        # the first tick's registers, the operators' levels (40h to 55h:
        # the tune's volume, the caller's) by number only
        w = [x for x in tune if x[3]]
        return [(x[1], None if 0x40 <= x[1] <= 0x55 else x[2]) for x in w if x[4] == w[0][4]]

    for j, b in enumerate(ptunes):
        for i in range(k, len(otunes)):
            if head(b) and head(otunes[i]) == head(b):
                pairs.append((i, j))
                k = i + 1
                break
    print('%s: the original started %d tunes, the port %d; %d pairs begin with the same registers'
          % (walk, len(otunes), len(ptunes), len(pairs)))
    if not pairs:
        raise SystemExit('oplwav_cmp FAILED: no tune of the port begins as one of the original')
    for i, j in pairs:
        a, b = otunes[i], ptunes[j]
        if next(x[3] for x in a if x[3]) != 1:
            print('  tune %d (the port\'s %d): fade_tick writes before the first tick, not compared' % (i + 1, j + 1))
            continue
        a0 = next(x[4] for x in a if x[3])
        b0 = next(x[4] for x in b if x[3])
        end = min(a[-1][4] - a0, b[-1][4] - b0)
        a = [x for x in a if 0 <= x[4] - a0 <= end]
        b = [x for x in b if 0 <= x[4] - b0 <= end]
        print('  tune %d (the port\'s %d), %d ticks (%.1f s) in both:' % (i + 1, j + 1, end, b[-1][0] - b[0][0]))
        # --- the writes, by voice
        called = any(not x[3] for x in a + b)
        first = None
        told, fades, shifts = [], 0, []
        for v in list(range(9)) + ['chip']:
            wa = [(x[4] - a0, x[1], x[2]) for x in a if x[3] == 1 and voice(x[1]) == v]
            fa = [(x[4] - a0, x[1], x[2]) for x in a if x[3] == 2 and voice(x[1]) == v]
            wb = [(x[4] - b0, x[1], x[2]) for x in b if x[3] and voice(x[1]) == v]
            same = sum((Counter(wa) & Counter(wb)).values())
            # what the port wrote beyond adl_tick's: the fade's steps,
            # which count from the game's call and may be at another tick
            fb = sorted((Counter(wb) - Counter(wa)).elements())
            lost = sorted((Counter(wa) - Counter(wb)).elements())
            faded = [x[1:] for x in sorted(fa)] == [x[1:] for x in fb]
            if faded:
                fades += len(fa)
                shifts += [y[0] - x[0] for x, y in zip(sorted(fa), fb)]
            diff = lost + ([] if faded else fb + fa)
            if diff and (first is None or min(diff)[0] < first):
                first = min(diff)[0]
            told.append('%s %d/%d/%d' % (v, len(wa) + len(fa), len(wb), same + (len(fa) if faded else 0)))
            if diff and not called:
                print('    FAILED: voice %s is not written the same (the first difference at tick %d)'
                      % (v, min(diff)[0]))
                bad = 1
        print('    the timer\'s writes by voice, original/port/the same%s: ' % (' (not judged: the game\'s side wrote too)' if called else '')
              + ', '.join(told))
        print('    of those the same, %d are fade_tick\'s, in the port %s ticks from the original\'s; the game\'s '
              'side: %d writes in the original, %d in the port; the first write that differs is at tick %s'
              % (fades, '%+d to %+d' % (min(shifts), max(shifts)) if shifts else 'no',
                 sum(1 for x in a if not x[3]), sum(1 for x in b if not x[3]), 'none' if first is None else first))
        # --- the time
        at_o = {}           # tick -> when the original's began, from the tune's first
        for n in range(end + 1):
            at_o[n] = ticks[a0 - 1 + n][0] - ticks[a0 - 1][0]
        adl = {(x[4] - a0, x[1], x[2]) for x in a if x[3] == 1}
        t0p = next(x[0] for x in b if x[3])
        at_p = {}           # tick -> the port's frame time, for the ticks adl_tick wrote in
        for x in b:
            if x[3] and (x[4] - b0, x[1], x[2]) in adl:
                at_p.setdefault(x[4] - b0, x[0] - t0p)
        ns = sorted(at_p)
        per = period.get(round(t0p * rate))
        if per is None:
            raise SystemExit('oplwav_cmp FAILED: no interrupt of the port at the tune\'s first tick')
        p = per / PIT_HZ
        lag = {n: at_o[n] - at_p[n] for n in ns}
        # the port's ticks are the periods summed
        off = max(abs(at_p[n] - n * p) for n in ns)
        # where the original falls behind for good: the least it is
        # behind at any later tick grows by more than half a period
        behind = [at_o[n] - n * p for n in range(end + 1)]
        least = behind[:]
        for n in range(end - 1, -1, -1):
            least[n] = min(least[n], least[n + 1])
        steps = [n for n in range(1, end + 1) if least[n] - least[n - 1] > p / 2]
        long_ = sum(1 for n in steps if ticks[a0 - 2 + n][1] - ticks[a0 - 2 + n][0] > p)
        tick0 = ticks[a0 - 1][1] - ticks[a0 - 1][0]
        after = least[1] if end else 0.0
        print('    the time: the port\'s ticks are %.3f ms apart (the period %04Xh; at most %.3f ms off the '
              'periods summed). The original\'s first tick runs %.1f ms, and its second comes %.1f ms later '
              'than a period after the first; at the end it is %.1f ms behind the port (%.1f periods), fallen '
              'behind for good at %d ticks, %d of them behind a tick that ran longer than a period'
              % (p * 1000, per, off * 1000, tick0 * 1000, after * 1000, lag[ns[-1]] * 1000, lag[ns[-1]] / p,
                 len(steps), long_))
        if off > 1.5 / rate or (end and abs(after - max(tick0 - p, 0)) > p):
            print('    FAILED: the port\'s ticks are not at the periods summed, or the original\'s second tick is '
                  'not late by what the first ran')
            bad = 1
        # --- the sound, before the first write that differs
        upto = end if first is None else first - 1
        wins = [(n, n + WINDOW) for n in range(1, upto - WINDOW, WINDOW)]
        if not wins:
            print('    the sound: fewer than %d ticks before the first write that differs, not compared' % WINDOW)
            continue
        t0o = ticks[a0 - 1][0]
        ow = [(t0o + at_o[x], t0o + at_o[y]) for x, y in wins]
        lo = levels(owav, [(int(x * 49716), int(y * 49716)) for x, y in ow])
        lr = levels(rwav, [(int(x * rate), int(y * rate)) for x, y in ow])
        lt = levels(twav, [(int(x * rate), int(y * rate)) for x, y in ow])
        lp = levels(pwav, [(round((t0p + x * p) * rate), round((t0p + y * p) * rate)) for x, y in wins])
        quiet = [x < QUIET for x in lo]
        if any(q != (x < QUIET) for q, x in zip(quiet, lp)):
            print('    FAILED: a window is silent in one file only')
            bad = 1
            continue
        if all(quiet):
            print('    the sound: all %d windows silent in both' % len(wins))
            continue
        sr, st, sm, sp = step(lo, lr, quiet), step(lr, lt, quiet), step(lt, lp, quiet), step(lo, lp, quiet)
        print('    the sound over the first %d ticks, %d windows of %d (%d silent in both and left out), the '
              'change of level on average and its range: the original\'s stream on a chip at %d Hz %+.2f dB '
              '(%+.2f to %+.2f); then each tick\'s writes on one sample %+.2f dB (%+.2f to %+.2f); from that to '
              'the port\'s file %+.2f dB (%+.2f to %+.2f); the port\'s file against the original\'s %+.2f dB '
              '(%+.2f to %+.2f)' % ((wins[-1][1], len(wins), WINDOW, sum(quiet), rate) + sr + st + sm + sp))
        if max(abs(x) for x in sr[1:]) > RATE_DB or abs(sp[0]) > MEAN_DB or max(abs(x) for x in sp[1:]) > LEVEL_DB:
            print('    FAILED: the levels are further apart than %.1f dB for the rate, %.1f dB on average or %.1f '
                  'dB in a window' % (RATE_DB, MEAN_DB, LEVEL_DB))
            bad = 1
    if bad:
        raise SystemExit('oplwav_cmp FAILED: %s' % walk)
    print('oplwav_cmp ok: %s' % walk)


main()
