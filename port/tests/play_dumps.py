#!/usr/bin/env python3
"""Makes what port/tests/play_test.sh compares the port's own play of a
battle (port/src/play.c) with.

    python3 port/tests/play_dumps.py [KEYS [UNTIL]]

HL.EXE is run on the runner with a key script of port/tests/keys/
(default m8_site.keys, map 8 chosen in the menu as loop_dumps.py does; a
script whose name starts with m0 plays map 0, one with mNN_, two digits,
map NN, whose code for the menu is read from the game's CODES.DAT; the
maps from 48 on are for two human sides) up to UNTIL emulated seconds
(default 400).  One run logs, with their times:

    27FE:008C  the routine the timer calls (72.8 times a second)
    066A:0EE9  a pass begins
    066A:0FC3  a side's part begins (SI the side)
    066A:1003  the key taken in it (AL)
    0CB6:05F1  side_input is called
    0CB6:0658  side_input has found the record's keys (CX button, DX
               directions)
    0CB6:0750  view_step is called
    0CB6:0799, 08DA, 0AF5, 0C7C  view_step's tests of the input record's
               byte 8 (ZF), 0CB6:084D, 09C5, 0BDA those of byte 5
    066A:10D6  computer_turn has returned in the loop that runs while the
               computer thinks
    27FE:04F4  the keyboard's interrupt
    2C95:0024, 004A  clock_seed sets rand_words, to its argument (AX, DX)
               or to the BIOS's count of ticks (DX, CX)
    2C9A:0008  rand_next
    0CB6:0075, 00C1, 00E2  the routine that waits (S0CB6:0036) has cleared
               the first input record's button and the key
    2532:0137  the fight's pictures have cleared the key
    2532:156D  fight_frames tests the key at a frame's end (a G line)
    02E6:000F  attacks_run
    167A:2FEF, 30B2  inputs_release (the beginning of event_picture) has
               cleared the key and both input records' buttons
    066A:42FC  the battle's end (state 7) is about to show its texts
    10AF:009F  show_message has its text's number (AX; DI 0 for the
               first side, A0h for the second)
    0CB6:0DA9  wait_press is entered (the battle's end's texts shown)
    0CB6:0DE4, 0E30, 0E51  wait_press (S0CB6:0DA9, the battle's end) has
               cleared the first input record's button and the key
    066A:45E8  the loop is left (the battle is over)
    1A39:0001  event_picture (behind the loop: DI the picture and SI the
               side at game_loop's first call, SI the picture at its
               second, DI the argument at battle_score's)
    063F:0009  battle_score; 063F:0277, 027E  it chooses picture CEh, CFh
    1A39:0BE6, 0EF6, 10AF, 1359  a frame of a picture behind the loop
               begins (port/src/events.c)
    1A39:198C  a frame of a site's picture (site_picture, attacks_run's
               order 4) begins
    066A:4728  what follows the loop is done

and writes into build/play/TAG/ (ignored):

    clock.txt   the original's clock for the port's program (-clock; the
                lines are described in port/src/play.c)
    inputs.txt  a line a side's part: pass, side, key, and for a human
                side the input word side_input found (hex; - for the
                computer's) and what view_step's first tests of the
                record's bytes 5 and 8 found (0 or 255, -1 not tested), as
                the port's -inputs writes them (it has the bytes whenever
                view_step is called)
    pNNNN.ram   memory at the beginning of chosen passes (the first, 50,
                200, 400, those attacks_run ran in and the ones after
                them, the last but one, and PASSES in the environment,
                numbers apart by spaces); existing dumps are kept.  When
                the loop was left, one more with the number after the last
                pass's: memory at S066A:45E8, which the port's program
                has after its last pass; and when S066A:4728 was reached
                one with the number after that: memory there, behind
                what follows the loop.  At a battle's end also eN.ram at
                the N-th entry of wait_press (the texts shown) and
                fNNNN.ram at the frames 1, 11, 21 ... of the pictures
                behind the loop, counted over all of them (the port's
                -evdump); and gFNNN.ram at the frames 1, 11, 21 ... of
                the F-th fight's pictures (at S2532:156D, the port's
                -evdump too); and sNNNN.ram at the frames 1, 11, 21 ...
                of the sites' pictures, counted over all of them (the
                port's -evdump too)
    passes.txt  their numbers
    ends.txt    a line for each time the battle's end showed its texts
                and waited: pass, side, the first side's text and the
                second's (hex), as the port's -ends writes them; and
                "after KIND ARG" for each picture shown behind the loop

With NOPICS=1 in the environment the fights are not shown as pictures
(no_fight_pics, S37FF:0115, set at the first pass as loop_dumps.py does)
and the files go to build/play/TAG_nopics/.

With VRAM=1 in the environment each pNNNN.ram gets a pNNNN.vram beside
it, the video memory at the same place (play_cmp.py compares it).

With WALK=code or WALK=loadN in the environment the menu is walked by
keys and not by the mouse: the map's code typed and START, or MISC, LOAD
and the digit N (a saved game, which STATE's folder must hold).  keys.txt
is then the walk's keys and the script's as one key script for the port's
program, which begins at its menu, and start.txt when the menu's first
pass began (its -start); S167A:130B, a pass of the menu's loop, is logged
for it.

With STATE=DIR in the environment the files the original writes in the
run (a saved game) go to DIR instead of the runner's build/run/state.

A file port/tests/keys/TAG.pokes changes memory when the first pass
begins (port/tests/play_pokes.py), for a state no short script reaches.
A file port/tests/keys/TAG.mice is the mouse's script for the runner's
-mice ("T X,Y,B" a line, as the port's -micelog writes it).
A file port/tests/keys/TAG.joy is the joysticks' script for the runner's
-joys ("T N,X,Y,B" or "T N,off" a line; the port's -joys takes the same).

No game data is kept in this file.
"""
import os, re, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
KIT = os.path.join(ROOT, 'doskit', 'tools', 'run.py')

keys = sys.argv[1] if len(sys.argv) > 1 else 'm8_site.keys'
until = sys.argv[2] if len(sys.argv) > 2 else '400'
keypath = os.path.join(HERE, 'keys', keys)
if keys.startswith('m0'):
    pre = ['-mouse', '22', '150,48,1', '-mouse', '22.2', '150,48,0',
           '-mouse', '31', '230,80,1', '-mouse', '31.2', '230,80,0']
elif re.match(r'm\d\d_', keys):
    # the menu's CODE row, the map's five letters (the first five bytes of
    # its ten-byte record in the unpacked CODES.DAT, a letter the byte less
    # 11h), Enter, START
    sys.path.insert(0, os.path.join(ROOT, 'tools'))
    import tpwmfiles
    game = os.environ.get('DOSKIT_GAME') or os.path.join(ROOT, 'game')
    rec = tpwmfiles.unpack(open(os.path.join(game, 'CODES.DAT'), 'rb').read())[int(keys[1:3]) * 10:][:5]
    pre = ['-mouse', '21', '150,73,1', '-mouse', '21.2', '150,73,0']
    for i, b in enumerate(rec):
        pre += ['-key', str(22 + i * 0.5), chr(b - 0x11 + ord('a'))]
    pre += ['-key', '25', 'enter', '-mouse', '27', '150,48,1', '-mouse', '27.2', '150,48,0']
else:
    pre = ['-mouse', '21', '150,73,1', '-mouse', '21.2', '150,73,0', '-key', '22', 'r',
           '-key', '22.5', 'a', '-key', '23', 't', '-key', '23.5', 'i', '-key', '24', 'o',
           '-key', '25', 'enter', '-mouse', '27', '150,48,1', '-mouse', '27.2', '150,48,0']
walk = os.environ.get('WALK')
menu_keys = None
if walk:
    # the menu walked by keys instead (the port's menu has no mouse of the
    # player's), as port/tests/menu_dumps.py does: `code` the CODE row,
    # the map's code typed, START (which takes the second press); `loadN`
    # MISC, LOAD and the digit N
    if walk == 'code':
        menu_keys = [(10, 'down'), (11, 'space')]
        menu_keys += [(12 + i * 0.5, chr(b - 0x11 + ord('a'))) for i, b in enumerate(rec)]
        menu_keys += [(15, 'enter'), (16, 'up'), (17, 'space'), (19, 'space')]
    else:
        menu_keys = [(10, 'down'), (11, 'down'), (12, 'down'), (13, 'down'), (14, 'space'),
                     (16, 'down'), (17, 'space'), (19, walk[4:])]
    pre = ['-key', '3', 'enter', '-key', '6', 'enter']
    for t, k in menu_keys:
        pre += ['-key', str(t), k]
pre += ['-keys', keypath, '-until', until]
# the mouse's script beside the key script (the window's -micelog), if any
micepath = os.path.splitext(keypath)[0] + '.mice'
if os.path.exists(micepath):
    pre += ['-mice', micepath]
joypath = os.path.splitext(keypath)[0] + '.joy'
if os.path.exists(joypath):
    pre += ['-joys', joypath]
if os.environ.get('STATE'):
    # the folder the original's files written in the run go to (a saved
    # game), instead of the runner's build/run/state
    os.makedirs(os.environ['STATE'], exist_ok=True)
    pre += ['-state', os.path.abspath(os.environ['STATE'])]
tag = os.path.splitext(keys)[0]
pre += subprocess.run(['python3', os.path.join(HERE, 'play_pokes.py'), tag],
                      capture_output=True, text=True).stdout.split()
if os.environ.get('NOPICS'):
    # the fights without pictures: no_fight_pics is set when the first pass
    # begins (a poke, not the game's own way of setting it)
    pre += ['-poke', 'S066A:0EE9#1', 'S37FF:0115', '01']
    tag += '_nopics'
out = os.path.join(ROOT, 'build', 'play', tag)
os.makedirs(out, exist_ok=True)
if menu_keys:
    with open(os.path.join(out, 'keys.txt'), 'w') as f:
        for t, k in menu_keys:
            f.write('%s %s\n' % (t, k))
        f.write(open(keypath).read())

R8 = ('0CB6:0799', '0CB6:08DA', '0CB6:0AF5', '0CB6:0C7C')
R5 = ('0CB6:084D', '0CB6:09C5', '0CB6:0BDA')
WAITS = ('0CB6:0075', '0CB6:00C1', '0CB6:00E2')
PRESS = ('0CB6:0DE4', '0CB6:0E30', '0CB6:0E51')
FRAMES = ('1A39:0BE6', '1A39:0EF6', '1A39:10AF', '1A39:1359')
SITE = '1A39:198C'      # a frame of site_picture
RELEASE = ('167A:2FEF', '167A:30B2')
LOGS = ('27FE:008C', '066A:0EE9', '066A:0FC3', '066A:1003', '0CB6:05F1', '0CB6:0658', '0CB6:0750',
        '066A:10D6', '2C95:0024', '2C95:004A', '02E6:000F', '27FE:04F4', '2C9A:0008',
        '2532:0137', '2532:156D', '066A:45E8', '066A:42FC', '10AF:009F',
        '1A39:0001', '063F:0009', '063F:0277', '063F:027E', '066A:4728', '0CB6:0DA9') + R8 + R5 + WAITS + PRESS + RELEASE + FRAMES + (SITE,)
if menu_keys:
    LOGS += ('167A:130B',)      # a pass of the menu's loop


def run(args):
    r = subprocess.run(['python3', KIT] + pre + args + ['HL.EXE'],
                       capture_output=True, text=True, cwd=out)
    return r.stdout + r.stderr


log = os.path.join(out, 'log.txt')
if not os.path.exists(log):
    a = []
    for l in LOGS:
        a += ['-log', 'S' + l]
    open(log, 'w').write(run(a))

if menu_keys:
    for line in open(log):
        m = re.match(r'log HL\.EXE\+167A:130B t=(\S+)', line)
        if m:
            open(os.path.join(out, 'start.txt'), 'w').write(m.group(1) + '\n')
            break

clock = []
rows = []           # [pass, side, key, word or None, byte 5, byte 8]
row = None
passes = 0
rounds = []         # passes attacks_run ran in
ends = []           # [pass, side, the two sides' texts] of each state 7
end = None
left = False        # the loop was left
after = False       # S066A:4728 was reached
scored = False      # battle_score was called
pic = None          # the picture battle_score chose
waits = 0           # wait_press's entries
frames = []         # the pictures' frames: (address, its hit there)
local = {}
fights = []         # the fights' pictures: the frames each has run
sites = 0           # the sites' pictures' frames
for line in open(log):
    m = re.match(r'log HL\.EXE\+(\w{4}:\w{4}) t=(\S+) hit=\d+ regs (.*)', line)
    if not m:
        continue
    at, t, regs = m.group(1), m.group(2), dict(re.findall(r'([A-Z]+)=([0-9A-F]+)', m.group(3)))
    reg = lambda r: int(regs[r], 16)
    if at == '27FE:008C':
        clock.append('%s T' % t)
    elif at == '066A:0EE9':
        passes += 1
    elif at == '066A:45E8':
        left = True
        clock.append('%s L' % t)
    elif at == '066A:4728':
        after = True
        clock.append('%s M' % t)
    elif at == '063F:0009':
        scored = True
        clock.append('%s B' % t)
    elif at in ('063F:0277', '063F:027E'):
        pic = 0xCE if at == '063F:0277' else 0xCF
    elif at == '1A39:0001':
        if left and not after:
            if pic is not None:
                ends.append(['after', '%X' % pic, '%X' % reg('DI')])
                pic = None
            elif scored:
                ends.append(['after', '%X' % reg('SI'), '0'])
            else:
                ends.append(['after', '%X' % reg('DI'), '%X' % reg('SI')])
    elif at == '066A:42FC':
        end = [passes, row[1], None, None]
        ends.append(end)
    elif at == '10AF:009F':
        if end is not None:
            end[3 if reg('DI') else 2] = reg('AX')
            if None not in end:
                end = None
    elif at == '0CB6:0DA9':
        waits += 1
        clock.append('%s Q' % t)
    elif at in FRAMES:
        local[at] = local.get(at, 0) + 1
        frames.append((at, local[at]))
        clock.append('%s F' % t)
    elif at == SITE:
        sites += 1
    elif at in RELEASE:
        clock.append('%s Y' % t)
    elif at in PRESS:
        clock.append('%s E' % t)
    elif at == '2C95:0024':
        clock.append('%s S %X' % (t, reg('DX') << 16 | reg('AX')))
    elif at == '2C95:004A':
        clock.append('%s S %X' % (t, reg('CX') << 16 | reg('DX')))
    elif at == '066A:0FC3':
        row = [passes, reg('SI'), 0, None, -1, -1]
        rows.append(row)
        clock.append('%s P %d %d' % (t, passes, reg('SI')))
    elif at == '27FE:04F4':
        clock.append('%s K' % t)
    elif at == '2C9A:0008':
        clock.append('%s R' % t)
    elif at in WAITS:
        clock.append('%s W' % t)
    elif at == '2532:0137':
        clock.append('%s X' % t)
        fights.append(0)
    elif at == '2532:156D':
        fights[-1] += 1
        clock.append('%s G' % t)
    elif row is None:
        continue
    elif at == '066A:1003':
        row[2] = reg('AX') & 0xFF
    elif at == '0CB6:05F1':
        clock.append('%s I' % t)
    elif at == '0CB6:0658':
        row[3] = (reg('CX') & 0x10) | (reg('DX') & 0x0F)
    elif at == '0CB6:0750':
        clock.append('%s V' % t)
    elif at == '066A:10D6':
        clock.append('%s C' % t)
    elif (at in R8 or at in R5) and row[3] is not None:
        i = 5 if at in R8 else 4
        if row[i] == -1:
            row[i] = 0 if reg('F') & 0x40 else 255
    elif at == '02E6:000F':
        rounds.append(passes)
        clock.append('%s A' % t)

with open(os.path.join(out, 'clock.txt'), 'w') as f:
    f.write('\n'.join(clock) + '\n')
with open(os.path.join(out, 'inputs.txt'), 'w') as f:
    for p, s, k, w, r5, r8 in rows:
        f.write('%d %d %X %s\n' % (p, s, k, '-' if w is None else '%X %d %d' % (w, r5, r8)))

with open(os.path.join(out, 'ends.txt'), 'w') as f:
    for e in ends:
        if e[0] == 'after':
            f.write(' '.join(e) + '\n')
        else:
            f.write('%d %d %s %s\n' % (e[0], e[1], *('-' if x is None else '%X' % x for x in e[2:])))

extra = [int(x) for x in os.environ.get('PASSES', '').split()]
chosen = sorted(set([1, 50, 200, 400] + rounds + [p + 1 for p in rounds] + [passes - 1] + extra))
chosen = [p for p in chosen if 1 <= p < passes + left]


def dump(p):
    path = os.path.join(out, 'p%04d.ram' % p)
    vram = os.environ.get('VRAM') and not os.path.exists(path[:-4] + '.vram')
    if not os.path.exists(path) or vram:
        run(['-break', 'S066A:0EE9#%d' % p, '-ram', path] + (['-vram', path[:-4] + '.vram'] if vram else []))


with ThreadPoolExecutor(8) as pool:
    list(pool.map(dump, chosen))
# the battle's end: the memory where the loop is left, and behind what
# follows it; the end's waits and every tenth frame of the pictures
ends_at = []
if left:
    ends_at.append(('S066A:45E8', 'p%04d' % (passes + 1)))
if after:
    ends_at.append(('S066A:4728', 'p%04d' % (passes + 2)))
ends_at += [('S0CB6:0DA9#%d' % (n + 1), 'e%d' % (n + 1)) for n in range(waits)]
ends_at += [('S%s#%d' % f, 'f%04d' % (n + 1)) for n, f in enumerate(frames) if n % 10 == 0]
ends_at += [('S%s#%d' % (SITE, n), 's%04d' % n) for n in range(1, sites + 1, 10)]
ends_at += [('S2532:156D#%d' % (sum(fights[:i]) + k), 'g%d%03d' % (i + 1, k))
            for i, n in enumerate(fights) for k in range(1, n + 1, 10)]


def dump_at(a):
    where, name = a
    path = os.path.join(out, name + '.ram')
    if not os.path.exists(path):
        run(['-break', where, '-ram', path] + (['-vram', path[:-4] + '.vram'] if os.environ.get('VRAM') else []))


with ThreadPoolExecutor(8) as pool:
    list(pool.map(dump_at, ends_at))
have = sorted(int(f[1:-4]) for f in os.listdir(out) if re.match(r'p\d{4,}\.ram$', f))
with open(os.path.join(out, 'passes.txt'), 'w') as f:
    f.write(' '.join(str(p) for p in have) + '\n')
print('%d passes logged, attacks_run in passes %s; dumps at %s' % (passes, rounds, have))
