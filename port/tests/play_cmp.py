#!/usr/bin/env python3
"""Compares the port's own play of a battle with the original's.

    python3 port/tests/play_cmp.py [TAG [own]]

In build/play/TAG/ (default m8_site): pNNNN.ram is the original's memory
at the beginning of pass NNNN and inputs.txt what each side's part read
(port/tests/play_dumps.py), qNNNN.ram and port_inputs.txt the same of
the port's program started from its own setup (port/tests/play_test.sh).
With `own` the port's files are those of a play on its own clock
(oNNNN.ram, own_inputs.txt): its passes are not the original's then, so
the result is up to which pass the two agree, and where the parts first
read something else.

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
if own:
    print('on the port\'s own clock: the game\'s state and the computer\'s the same up to pass %s of those '
          'compared%s' % (agree, '; the parts first read something else in pass %s' % a[diff[0]].split()[0]
                          if diff else ''))
    sys.exit(0)
print('DIFFERENT' if bad else 'the inputs, the game\'s state and the computer\'s the same at every pass compared')
sys.exit(bad)
