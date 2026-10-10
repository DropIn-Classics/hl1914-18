#!/usr/bin/env python3
"""Compares the port's walk through the menu with the original's
(port/tests/menu_test.sh calls it).

    python3 port/tests/menu_cmp.py WALK

In build/menu/WALK/: acts.txt and exit.ram are the original's
(menu_dumps.py), port.log and port.ram the port's (-menulog, -menudump).
When each screen outside the menu's loop came is set side by side too
(the runner's log.txt and the lines "ask N TIME" of port.log; the times
are printed, a different number of screens counts as different).
Compared:

  * what came in, in order (1 chosen, 2 the other page, 5 another item,
    0 the menu's return), and how far the times are apart at most (the
    port draws nothing, so its passes are not the original's: the times
    are told, not judged);
  * memory at the menu's last return: the game's state (S331F; but for
    the count of a battle's passes, the dword at S331F:4CB7, which is
    told apart: the port's passes are shorter, it draws less), the
    menu's own segment (S37FF: the libraries' records, the choices kept,
    the code), the mouse's record (DATA:0414 to 045B), and the block of
    445C0h bytes game_loop allocates from 40h bytes behind its beginning
    for 40000h bytes (the first 40h hold what start_setup's part that is
    not translated leaves in the first file): the menu's three pictures,
    CODES.DAT, the six libraries, the tunes and sounds and the menu's
    data with the items' rectangles and choices.

  * video memory at the passes menu_dumps.py kept the original's at: the
    page drawn and the page shown (with the mouse's pointer); and at
    each screen outside the menu's loop (askN.vram, the port's -askvram:
    the saved game's question, text_input's beginning and end, the best
    scores shown), and the memory there (askN.ram, portaskN.ram).

  * with a battle played to its end between (end57, live57): the memory
    and video memory at the end's waits and the pictures' frames
    (eN, fNNNN against the port's -evdump port_eN ...; the timers'
    passes in game_state, bytes 19h to 63h, told apart with the count
    of passes, since the port's battle on its own clock takes another
    number of passes), and the files the original wrote (written/)
    against the port's (port_saves/).  A best scores' file is compared
    with the port's deliberate deviation applied to the original's: a
    name it wrote (one not in the walk's seed/) ends at its first byte
    that is not a capital, zeros behind it (the original keeps
    text_input's cursor `]` there; menu.c's winner_name).

  * a pass's two pages exchanged are told apart: text_input shows the
    other page every pass, and the port's passes on its own clock are
    not the original's, so after a code or a name typed the page with
    the pointer can be the other one (an odd number of passes apart).
    Then each page is compared with the other program's other page, and
    nothing may differ.

Exit status 0 when the order is the same and no byte differs.

    python3 port/tests/menu_cmp.py --vram A B

compares two files of video memory that way (menu_test.sh's `live`):
exit status 0 when the pages are the same or the same exchanged.
"""
import os, re, sys


def pages_differ(v, w):
    """the bytes that differ in the two pages (200 lines of 80 bytes a
    plane at A000h and A400h, byte 4 * offset + plane), page for page
    and with the pages exchanged, and behind them"""
    d = [i >> 2 for i in range(len(v)) if v[i] != w[i]]
    pages = len([o for o in d if o < 0x8000 and o % 0x4000 < 200 * 80])
    n = 200 * 80 * 4
    other = sum(1 for i in range(n) if v[i] != w[0x10000 + i]) + sum(1 for i in range(n) if v[0x10000 + i] != w[i])
    return pages, other, len(d) - pages


if len(sys.argv) > 1 and sys.argv[1] == '--vram':
    pages, other, _ = pages_differ(open(sys.argv[2], 'rb').read(), open(sys.argv[3], 'rb').read())
    print('the same' if not pages else 'the two pages exchanged, the same otherwise' if not other else 'DIFFERENT')
    sys.exit(1 if pages and other else 0)

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
walk = sys.argv[1]
out = os.path.join(ROOT, 'build', 'menu', walk)
LOAD = 0x76             # where the runner and the port load HL.EXE
BLOCK = 0x40A60         # game_loop's block in both (its MCB follows the program's)


def acts(name):
    rows = []
    for line in open(os.path.join(out, name)):
        f = line.split()
        if len(f) >= 2 and f[0] != 'ask':
            rows.append((float(f[0]), int(f[1])))
    return rows


def asks():
    """when each screen outside the menu's loop came: the original's
    (the runner's log, the addresses of menu_dumps.py's ASKS) and the
    port's (the lines "ask N TIME" of its -menulog)"""
    x = [float(m.group(1)) for m in (re.match(r'log HL\.EXE\+167A:(?:1929|1B5A|334E|3604|0AAE|1C7C) t=(\S+) hit=\d+ regs', l)
                                     for l in open(os.path.join(out, 'log.txt'))) if m]
    y = [float(f[2]) for f in (l.split() for l in open(os.path.join(out, 'port.log'))) if len(f) == 3 and f[0] == 'ask']
    # a log made before menu_dumps.py logged all six addresses
    whole = all(any(l.startswith('log HL.EXE+167A:%s lin=' % a) for l in open(os.path.join(out, 'log.txt')))
                for a in ('1929', '1B5A', '334E', '3604', '0AAE', '1C7C'))
    return x, y, whole


a, b = acts('acts.txt'), acts('port.log')
bad = 0
if [k for _, k in a] != [k for _, k in b]:
    bad += 1
    print('%s: what came in differs: original %s, port %s' % (walk, [k for _, k in a], [k for _, k in b]))
else:
    far = max([abs(x[0] - y[0]) for x, y in zip(a, b)] or [0])
    print('%s: %d things came in, the same in both; times at most %.2f s apart' % (walk, len(a), far))
x, y, whole = asks()
if len(x) != len(y) and not whole:
    print('  when the screens outside the loop came: not compared (the original\'s log is older than their addresses in menu_dumps.py)')
elif len(x) != len(y):
    bad += 1
    print('  the screens outside the loop: %d in the original, %d in the port' % (len(x), len(y)))
elif x:
    print('  the screens outside the loop came at %s s, in the port at %s (at most %.2f s apart)' %
          (' '.join('%.2f' % t for t in x), ' '.join('%.2f' % t for t in y), max(abs(p - q) for p, q in zip(x, y))))

def memory(what, x, y):
    """the regions of two dumps compared; how many of them differ"""
    bad = 0
    print('  %s:' % what)
    # the hints' lines `segment NAME FRAME ...`
    segs = sorted((int(f[2], 16), f[1]) for f in (l.split() for l in open(os.path.join(ROOT, 'src', 'HL.hints')))
                  if len(f) > 2 and f[0] == 'segment')
    regions = []
    for i, (frame, name) in enumerate(segs):
        if name in ('S331F', 'S37FF'):
            regions.append((name, (frame + LOAD) * 16, (segs[i + 1][0] + LOAD) * 16))
        if name == 'DATA':
            regions.append(('the mouse', (frame + LOAD) * 16 + 0x414, (frame + LOAD) * 16 + 0x45C))
            regions.append(('blocks', (frame + LOAD) * 16 + 0x467, (frame + LOAD) * 16 + 0x469))
    regions.append(('the block', BLOCK + 0x40, BLOCK + 0x40000))
    for name, lo, hi in regions:
        d = [i for i in range(lo, hi) if x[i] != y[i]]
        count = []
        if name == 'S331F':
            count = [i for i in d if 0x4CB7 <= i - lo < 0x4CBB]
            d = [i for i in d if i not in count]
            if count:
                at = lo + 0x4CB7
                print('  the passes counted in the battle before: %d, port %d' %
                      (int.from_bytes(x[at:at + 4], 'little'), int.from_bytes(y[at:at + 4], 'little')))
                # and the timers' kinds and the passes they are due at
                # (game_state, S331F:4CA4, its bytes 19h to 63h), as
                # play_cmp.py counts them apart
                due = [i for i in d if 0x4CA4 + 0x19 <= i - lo < 0x4CA4 + 0x64]
                d = [i for i in d if i not in due]
                if due:
                    print('  the timers\' passes differ (%d bytes), told apart' % len(due))
        if name == 'S331F' and (count or os.path.exists(os.path.join(out, 'pokes.txt'))):
            # with a battle before (another count of passes, or a walk
            # with a battle played to its end), the views' repeat
            # counters (byte 2Ch of the two records of 31h bytes from
            # 4D92h), which side_input counts down a pass at a time
            # while a key is held: the port's passes on its own clock
            # are not the original's, also where their count is the
            # same (twice57)
            held = [i for i in d if i - lo in (0x4D92 + 0x2C, 0x4D92 + 0x31 + 0x2C)]
            d = [i for i in d if i not in held]
            if held:
                print('  the views\' repeat counters, counted down by the pass, differ (%d bytes), told apart'
                      % len(held))
        if name == 'the mouse' and what.endswith('end'):
            # the pointer's place (words 10h and 12h): game_loop opens the
            # mouse again behind the menu, the original's driver then has
            # the pointer in the middle, the port's where the menu left it
            place = [i for i in d if 0x10 <= i - lo < 0x14]
            d = [i for i in d if i not in place]
            if place:
                print('  the pointer\'s place after mouse_open differs (%d bytes), told apart' % len(place))
        print('  %-10s %6d bytes, %d differ%s' % (name, hi - lo, len(d),
              (': first at +%Xh (%02X, port %02X)' % (d[0] - lo, x[d[0]], y[d[0]])) if d else ''))
        bad += bool(d)
    return bad


bad += memory('memory at the menu\'s last return', open(os.path.join(out, 'exit.ram'), 'rb').read(),
              open(os.path.join(out, 'port.ram'), 'rb').read())
# the program's end (the walk `exit`): the original's memory where it
# calls input_remove, the port's at its end; with it the count of
# mem_alloc's blocks (DATA:0467)
if os.path.exists(os.path.join(out, 'end.ram')):
    if os.path.exists(os.path.join(out, 'port_end.ram')):
        bad += memory('memory at the program\'s end', open(os.path.join(out, 'end.ram'), 'rb').read(),
                      open(os.path.join(out, 'port_end.ram'), 'rb').read())
    else:
        print('  memory at the program\'s end: the port wrote none')
        bad += 1
# the memory at each screen outside the menu's loop (askN.ram; the
# port's -askvram writes portaskN.ram beside its .vram)
asks = int(open(os.path.join(out, 'asks.txt')).read()) if os.path.exists(os.path.join(out, 'asks.txt')) else 0
for k in range(1, asks + 1):
    o, p = os.path.join(out, 'ask%d.ram' % k), os.path.join(out, 'portask%d.ram' % k)
    if not os.path.exists(o):
        continue
    if not os.path.exists(p):
        print('  memory at screen %d outside the loop: the port wrote none' % k)
        bad += 1
        continue
    bad += memory('memory at screen %d outside the loop' % k, open(o, 'rb').read(), open(p, 'rb').read())
# a battle's end between (the walk end57): the memory at the end's waits
# and frames (ends.txt, eN and fNNNN; the port's -evdump port_eN ...)
ends = open(os.path.join(out, 'ends.txt')).read().split() if os.path.exists(os.path.join(out, 'ends.txt')) else []
for e in ends:
    if not os.path.exists(os.path.join(out, 'port_%s.ram' % e)):
        print('  memory at %s: the port wrote none' % e)
        bad += 1
        continue
    bad += memory('memory at %s' % e, open(os.path.join(out, e + '.ram'), 'rb').read(),
                  open(os.path.join(out, 'port_%s.ram' % e), 'rb').read())
# the files the original wrote (written/, kept from menu_dumps.py's first
# run; a best score) against the port's (port_saves/, where a
# folder's separator is _)
def name_cut(data, seed):
    """a best scores' file (28h bytes: four dwords, 10h on four names of
    six bytes) with each name not in `seed` ended at its first byte that
    is not a capital, as the port keeps it; how many bytes that changed"""
    data, cut = bytearray(data), 0
    for i in range(4):
        at = 0x10 + 6 * i
        if seed is not None and seed[at:at + 6] == data[at:at + 6]:
            continue
        end = False
        for j in range(at, at + 5):
            end = end or not 0x41 <= data[j] <= 0x5A
            if end and data[j]:
                data[j], cut = 0, cut + 1
    return bytes(data), cut


state = os.path.join(out, 'written')
for d, _, names in os.walk(state):
    for name in sorted(names):
        rel = os.path.relpath(os.path.join(d, name), state)
        pf = os.path.join(out, 'port_saves', rel.replace(os.sep, '_'))
        want, cut = open(os.path.join(d, name), 'rb').read(), 0
        if name.upper().endswith('.HI') and len(want) == 0x28:
            sf = os.path.join(out, 'seed', rel.replace(os.sep, '_'))
            want, cut = name_cut(want, open(sf, 'rb').read() if os.path.exists(sf) else None)
        same = os.path.exists(pf) and open(pf, 'rb').read() == want
        print('  the file %s written: %s%s' % (rel, 'the same' if same else 'DIFFERENT or not written by the port',
              " (with the name's cursor taken off: %d bytes, the deviation)" % cut if cut else ''))
        bad += not same
# video memory (byte 4 * offset + plane): the two pages' 200 lines of 80
# bytes a plane at A000h and A400h; what lies behind them is told apart
# (from A7E8:0004 on the original's start, whose part before the menu is
# not translated, has left what the port's has not)
shots = open(os.path.join(out, 'shots.txt')).read().split() if os.path.exists(os.path.join(out, 'shots.txt')) else []
asks = int(open(os.path.join(out, 'asks.txt')).read()) if os.path.exists(os.path.join(out, 'asks.txt')) else 0
for what, vf, pf in [('at %5s s' % t, 'shot%s.vram' % t, 'port%s.vram' % t) for t in shots] + \
        [('at screen %d outside the loop' % k, 'ask%d.vram' % k, 'portask%d.vram' % k)
         for k in range(1, asks + 1)] + \
        [('at %s' % e, e + '.vram', 'port_%s.vram' % e) for e in ends]:
    v = open(os.path.join(out, vf), 'rb').read()
    pv = os.path.join(out, pf)
    if not os.path.exists(pv):
        print('  video memory %s: the port wrote none' % what)
        bad += 1
        continue
    w = open(pv, 'rb').read()
    pages, other, behind = pages_differ(v, w)
    if pages and not other:
        print('  video memory %s: the two pages exchanged (told apart), 0 bytes differ then, %d behind them'
              % (what, behind))
        continue
    print('  video memory %s: %d bytes differ in the two pages, %d behind them' % (what, pages, behind))
    bad += bool(pages)
sys.exit(1 if bad else 0)
