#!/usr/bin/env python3
"""Compares the port's own play of a battle with the original's.

    python3 port/tests/play_cmp.py [TAG [own|rounds|seeds]]

In build/play/TAG/ (default m8_site): pNNNN.ram is the original's memory
at the beginning of pass NNNN and inputs.txt what each side's part read
(port/tests/play_dumps.py), qNNNN.ram and port_inputs.txt the same of
the port's program started from its own setup (port/tests/play_test.sh).
With `own` the port's files are those of a play on its own clock
(oNNNN.ram, own_inputs.txt): its passes are not the original's then, so
the result is up to which pass the two agree, and where the parts first
read something else.

With `rounds` (play_test.sh) the port's play on its own clock is compared
round by round: rounds.txt has a line for each dump, the round's number,
the original's pass and the port's (rNNNN.ram), the pass of the n-th
round's end of each and the one after.  The two are then at different
passes, so what counts passes is counted apart as "the passes'": the
count of passes in game_state (dword 13h) and its copy in fight_rec
(3Ah), the timers' kinds and the passes they are due at (19h to 63h),
byte 2Ch of both views (counted down a pass), the bits 4 and 8 of
game_state's word 0 (a side's text is shown; the timers of kinds 1 and 2
take them off) and rand_words (the seed is
the BIOS's count of ticks when a fight's pictures begin, and those
pictures' own calls of rand_next are not made).  With `seeds` the port
took both from the original's log (play.c, -seeds) and rand_words is
compared with the computer's.

Compared as port/tests/loop_test.c does: the game's state (S3306 up to the
computer's player's segments S39B6 to S3AEB, and S3AEC:0008 up to DATA),
the two maps and the sides' work buffers; what the computer's player
keeps and leaves (its segments, S331F:17F2 to 181B, move_args, S3AEC:0000
to 0007, rand_words); the rest of memory but the stack's segment is
counted apart, and with it fight_rec's two far pointers of grounds'
pictures.  The differences at the first dump are the setup's: they
are listed once, and at a later pass a byte counts as the setup's while
both programs still have there what they had at the first dump.
"""
import os, re, struct, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
tag = sys.argv[1] if len(sys.argv) > 1 else 'm8_site'
own = len(sys.argv) > 2 and sys.argv[2] == 'own'
by_rounds = len(sys.argv) > 2 and sys.argv[2] in ('rounds', 'seeds')
seeds = len(sys.argv) > 2 and sys.argv[2] == 'seeds'
out = os.path.join(ROOT, 'build', 'play', tag)
BASE = 0x76
RAM = 0xA0000

seg, name = {}, {}
for line in open(os.path.join(ROOT, 'port', 'src', 'gen', 'names.h')):
    m = re.match(r'#define HL_(\w+) 0x([0-9A-F]+)u', line)
    if m:
        seg[m.group(1)] = int(m.group(2), 16)
    m = re.match(r'\s*X\((\w+), (\w+), 0x([0-9A-F]+)\)', line)
    if m:
        name[m.group(2)] = ((seg[m.group(1)] + BASE) << 4) + int(m.group(3), 16)
lin = lambda s: (seg[s] + BASE) << 4
order = sorted((v, k) for k, v in seg.items())


def where(a):
    s = None
    for v, k in order:
        if (v + BASE) << 4 <= a:
            s = (v, k)
    return '%s:%04X' % (s[1], a - ((s[0] + BASE) << 4)) if s else '%05X' % a


def regions(mem):
    """address ranges of the kinds 0 (the game's state) and 1 (the
    computer's); what is in neither is 2"""
    far = lambda a: struct.unpack_from('<HH', mem, a)
    def ptr(a):
        off, s = far(a)
        return (s << 4) + off
    nbytes = struct.unpack_from('<H', mem, name['map_bytes'])[0]
    comp = [(lin('S39B6'), lin('S3AEC') + 8), (name['free_depth'], name['free_depth'] + 0x2A),
            (name['move_args'], name['move_args'] + 0x12), (name['rand_words'], name['rand_words'] + 8)]
    # elsewhere: the last file's name, the data of the fight's and the taken
    # site's pictures, and the far pointers of the two grounds' pictures a
    # fight puts into fight_rec (1Eh, 22h) from the table behind S331F:0153,
    # which the libraries of pictures fill that the port does not load
    other = [(name['name_buf'], name['name_buf'] + 0x40), (lin('S3AEE'), lin('S3B5D')),
             (lin('S3812'), lin('S39B6')), (name['fight_rec'] + 0x1E, name['fight_rec'] + 0x26)]
    state = [(lin('S3306'), lin('DATA'))]
    for p in ('map_ptr', 'map2_ptr'):
        state.append((ptr(name[p]), ptr(name[p]) + nbytes))
    for o in (0x0D, 0x17 + 0x0D):
        state.append((ptr(name['sides'] + o), ptr(name['sides'] + o) + 0x1104))
    kind = bytearray(b'\x02') * RAM
    for lo, hi in state:
        kind[lo:hi] = b'\x00' * (hi - lo)
    for lo, hi in other:
        kind[lo:hi] = b'\x02' * (hi - lo)
    for lo, hi in comp:
        kind[lo:hi] = b'\x01' * (hi - lo)
    if by_rounds:
        gs = name['game_state']
        for lo, hi in ((gs + 0x13, gs + 0x17), (gs + 0x19, gs + 0x64), (name['fight_rec'] + 0x3A, name['fight_rec'] + 0x3E),
                       (name['views'] + 0x2C, name['views'] + 0x2D), (name['views'] + 0x31 + 0x2C, name['views'] + 0x31 + 0x2D),
                       (name['rand_words'], name['rand_words'] + (0 if seeds else 8))):
            kind[lo:hi] = b'\x04' * (hi - lo)
    st = 0x3EA4 << 4            # the stack's segment in the dumps
    kind[st:min(st + 0x10000, RAM)] = b'\x03' * (min(st + 0x10000, RAM) - st)
    return kind


def ranges(addrs):
    r = []
    for a in addrs:
        if r and a - r[-1][1] <= 8:
            r[-1][1] = a
            r[-1][2] += 1
        else:
            r.append([a, a, 1])
    return r


if by_rounds:
    first = None
    agree = None
    differs = False
    for line in open(os.path.join(out, 'rounds.txt')).read().split('\n')[:-1]:
        n, po, pp = (int(x) for x in line.split())
        rpath = os.path.join(out, 'r%04d.ram' % pp)
        if not os.path.exists(rpath):
            print('round %d: the port did not come so far' % n)
            continue
        if first is None:
            first = (open(os.path.join(out, 'p0001.ram'), 'rb').read(), open(os.path.join(out, 'r0001.ram'), 'rb').read())
        o = open(os.path.join(out, 'p%04d.ram' % po), 'rb').read()
        q = open(rpath, 'rb').read()
        kind = regions(q)
        gs = name['game_state']
        if not (o[gs] ^ q[gs]) & 0xF3:
            kind[gs] = 4        # a side's text still shown or not
        d = [i for i in range(RAM) if o[i] != q[i] and kind[i] != 3]
        new = [i for i in d if not (o[i] == first[0][i] and q[i] == first[1][i])]
        c = [sum(kind[i] == k for i in new) for k in (0, 1, 2, 4)]
        print('round %d, pass %d of the original and %d of the port: the game\'s state %d different, the '
              'computer\'s %d, the passes\' %d, elsewhere %d' % (n, po, pp, c[0], c[1], c[3], c[2]))
        for lo, hi, cnt in ranges([i for i in new if kind[i] < 2])[:12]:
            print('  %s to %s: %d (%s)' % (where(lo), where(hi).split(':')[1], cnt,
                                           'the computer\'s' if kind[lo] == 1 else 'the game\'s state'))
        if c[0] or c[1]:
            differs = True
        elif not differs:
            agree = (n, po, pp)
    print('round by round on the port\'s own clock%s: the game\'s state and the computer\'s the same but for the '
          'passes\' up to %s' % (' with the original\'s seeds' if seeds else '', 'round %d (pass %d of the original, %d of the port)' % agree if agree else 'no round'))
    sys.exit(0)

bad = 0
# what each side's part read
a = open(os.path.join(out, 'inputs.txt')).read().split('\n')[:-1]
b = open(os.path.join(out, 'own_inputs.txt' if own else 'port_inputs.txt')).read().split('\n')[:-1]
n = min(len(a), len(b))


def same(x, y):
    """a part's line of the original and the port's: the bytes 5 and 8 of
    the record count only where the original tested them, as 0 or not"""
    x, y = x.split(), y.split()
    if len(x) != len(y) or x[:4] != y[:4]:
        return False
    return all(u == '-1' or (u == '0') == (v == '0') for u, v in zip(x[4:], y[4:]))


diff = [i for i in range(n) if not same(a[i], b[i])]
print('inputs: %d parts of the port, %d of the original, %d different' % (len(b), len(a), len(diff)))
for i in diff[:10]:
    print('  original "%s", port "%s"' % (a[i], b[i]))
if diff:
    bad = 1

passes = [int(x) for x in open(os.path.join(out, 'passes.txt')).read().split()]
first = None
agree = None        # the last pass compared before the first difference
differs = False
for p in passes:
    qpath = os.path.join(out, '%s%04d.ram' % ('o' if own else 'q', p))
    if not os.path.exists(qpath):
        print('pass %d: the port did not come so far' % p)
        bad = 1
        continue
    o = open(os.path.join(out, 'p%04d.ram' % p), 'rb').read()
    q = open(qpath, 'rb').read()
    kind = regions(q)
    d = [i for i in range(RAM) if o[i] != q[i] and kind[i] != 3]
    if first is None:
        first = (o, q)
        print('pass %d, the setup\'s differences: the game\'s state %d, the computer\'s %d, elsewhere %d' % (
            p, sum(kind[i] == 0 for i in d), sum(kind[i] == 1 for i in d), sum(kind[i] == 2 for i in d)))
        for lo, hi, c in ranges([i for i in d if kind[i] < 2]):
            print('  %s to %s: %d (%s)' % (where(lo), where(hi).split(':')[1], c,
                                           'the computer\'s' if kind[lo] == 1 else 'the game\'s state'))
        continue
    setup = [i for i in d if o[i] == first[0][i] and q[i] == first[1][i]]
    new = sorted(set(d) - set(setup))
    c = [sum(kind[i] == k for i in new) for k in (0, 1, 2)]
    print('pass %d: the game\'s state %d different, the computer\'s %d, elsewhere %d; the setup\'s still %d' % (
        p, c[0], c[1], c[2], len(setup)))
    for lo, hi, cnt in ranges([i for i in new if kind[i] < 2])[:12]:
        print('  %s to %s: %d (%s)' % (where(lo), where(hi).split(':')[1], cnt,
                                       'the computer\'s' if kind[lo] == 1 else 'the game\'s state'))
    if c[0] or c[1]:
        bad = 1
        differs = True
    elif not differs:
        agree = p
if own and os.path.exists(os.path.join(out, 'own_rounds.txt')):
    # how long each round's end lasts: the original's from its clock (the
    # part's beginning, attacks_run's call, the next part's beginning)
    orig, part, at = [], None, None
    for line in open(os.path.join(out, 'clock.txt')):
        f = line.split()
        if f[1] == 'P':
            if at is not None and int(f[2]) != part[0]:
                orig.append((part[0], part[1], at, float(f[0])))
                at = None
            if part is None or int(f[2]) != part[0]:
                part = (int(f[2]), float(f[0]))
        elif f[1] == 'A':
            at = float(f[0])
    mine = [l.split() for l in open(os.path.join(out, 'own_rounds.txt'))]
    print('the rounds\' ends in their order (pass, begun at, s to attacks_run, s after it): '
          'the original | the port')
    for n in range(max(len(orig), len(mine))):
        l = '  %5d %8.2f %5.2f %6.2f' % (orig[n][0], orig[n][1], orig[n][2] - orig[n][1],
                                        orig[n][3] - orig[n][2]) if n < len(orig) else '  %27s' % '-'
        if n < len(mine):
            pp, pb_, pa, pe = int(mine[n][0]), float(mine[n][1]), float(mine[n][2]), float(mine[n][3])
            l += ' | %5d %8.2f %5.2f %6.2f' % (pp, pb_, pa - pb_, pe - pa)
        print(l)
if own:
    print('on the port\'s own clock: the game\'s state and the computer\'s the same up to pass %s of those '
          'compared%s' % (agree, '; the parts first read something else in pass %s' % a[diff[0]].split()[0]
                          if diff else ''))
    sys.exit(0)
print('DIFFERENT' if bad else 'the inputs, the game\'s state and the computer\'s the same at every pass compared')
sys.exit(bad)
