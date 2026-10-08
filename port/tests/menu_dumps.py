#!/usr/bin/env python3
"""Makes what port/tests/menu_test.sh compares the port's menu
(port/src/menu.c) with.

    python3 port/tests/menu_dumps.py [WALK...]

HL.EXE is run on the runner with keys that walk its menu (no mouse: the
pointer stays where the menu puts it) and stopped where game_loop has the
menu's return (S066A:0390), the N-th time for a walk that goes through
the menu N times.  The walks (default: all):

    code57   the CODE row, map 57's code typed (read from the game's
             CODES.DAT), START (pressed twice, see code57 below)
    start0   START with map 0 and the country it begins with taken
    start24  the same with the other country (left): map 24
    pages    GAME SETUP and each of its rows chosen once, the other page
             and back (Esc twice), CONTROLS with two rows chosen, MISC,
             the best scores shown and left, then as code57
    exit     Esc (the exit's page), EXIT TO DOS: the menu returns and
             the program ends (S066A:4739); a second run is stopped where
             the end calls its last routine (input_remove, S27FE:0060)
             and its memory kept too (end.ram)
    noload   MISC, LOAD and the digit 5 with no saved game there: the
             text that it was not found, left by the button's key; LOAD
             again, a letter, the backspace and the button's key (none of
             them is taken), Esc (the question left); LOAD and the digit
             0, not found again; then the main menu and as code57
    back     as code57, then in the battle Esc and the key that confirms
             (the battle is over, the menu again), START once more: the
             menu's second return
    end57    as code57, then a battle played to its end (m57_win's keys
             and pokes, see BATTLES below), the best score's name, the
             scores, START once more: the menu's second return
    live57   the same as the headless window played it from the menu on
             (keys/menu57_win.keys, its -keylog)

The runner's files written (a best score) go to WALK/state/, emptied
before each run; those of the first run are kept as WALK/written/.

Written into build/menu/WALK/ (ignored):

    keys.txt   the walk as a key script for the port's program (-keys)
    log.txt    the runner's log: S167A:130B (each pass of the menu's loop,
               AX what came in) and S066A:0390
    acts.txt   a line for each pass in which something came in: its time
               and what (1 chosen, 2 the other page, 5 another item)
    start.txt  when the menu's first pass began (the port's -start)
    exit.ram   memory at the menu's last return
    shotT.vram video memory at the beginning of the first pass of the
               menu's loop at or after second T (shots.txt: the T's)
    askN.vram  video memory at the N-th screen outside the menu's loop,
               as they come: the saved game's question drawn (S167A:1929,
               where key_remove is called before its loop) or the text
               that the file was not found (S167A:1B5A, before its wait);
               text_input's beginning and end (S167A:334E, its call of
               key_remove, and 3604, of key_install: the code typed, the
               winner's name); the best scores shown (S167A:0AAE behind
               a battle, 1C7C by the menu's item); askN.ram the memory
               there; asks.txt: how many
    eN, fNNNN  (.ram, .vram) with a battle between: memory and video
               memory at the N-th entry of wait_press (the end's texts
               shown) and at the frames 1, 11, 21 ... of the pictures
               behind the loop, as play_dumps.py has them; ends.txt
               their names; pokes.txt the pokes for the port's program;
               battle.keys the battle's keys for the runner

No game data is kept in this file.
"""
import os, re, shutil, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
KIT = os.path.join(ROOT, 'doskit', 'tools', 'run.py')
sys.path.insert(0, os.path.join(ROOT, 'tools'))
import tpwmfiles

GAME = os.environ.get('DOSKIT_GAME') or os.path.join(ROOT, 'game')


def code(m, t):
    """the keys of map m's code from t on, two a second"""
    rec = tpwmfiles.unpack(open(os.path.join(GAME, 'CODES.DAT'), 'rb').read())[m * 10:][:5]
    return [(t + i * 0.5, chr(b - 0x11 + ord('a'))) for i, b in enumerate(rec)]


def code57(t):
    # START takes the second press: the button's key was let go while the
    # code was typed, when the keys were the BIOS's, so the program still
    # has it as held until it is pressed and let go once more
    return [(t, 'down'), (t + 1, 'space')] + code(57, t + 2) + [(t + 5, 'enter'), (t + 6, 'up'), (t + 7, 'space'),
                                                                 (t + 9, 'space')]


WALKS = {
    # name: (keys, times through the menu, until, the seconds whose first
    # pass's video memory is kept)
    'code57': (code57(10), 1, 24, [9, 18]),
    'start0': ([(10, 'space'), (13, 'space')], 1, 22, [9]),
    'start24': ([(10, 'space'), (13, 'left'), (14.5, 'space')], 1, 24, []),
    'pages': ([(10, 'down'), (11, 'down'), (12, 'space'), (14, 'space'), (15, 'down'), (16, 'space'),
               (17, 'down'), (18, 'space'), (19, 'down'), (20, 'space'), (21, 'down'), (22, 'space'),
               (23, 'down'), (24, 'space'), (26, 'esc'), (28, 'esc'), (30, 'down'), (31, 'down'),
               (32, 'down'), (33, 'space'), (35, 'space'), (36, 'down'), (37, 'space'), (39, 'esc'),
               (41, 'down'), (42, 'down'), (43, 'down'), (44, 'down'), (45, 'space'), (47, 'space'),
               (50, 'space'), (53, 'esc')] + code57(55), 1, 70, [9, 13, 25, 27, 34, 38, 46, 63]),
    'back': (code57(10) + [(30, 'esc'), (32, 'z'), (50, 'space')], 2, 60, [45]),
    'exit': ([(10, 'esc'), (12, 'space')], 1, 20, [11]),
    'noload': ([(10, 'down'), (11, 'down'), (12, 'down'), (13, 'down'), (14, 'space'), (16, 'down'), (17, 'space'),
                (19, '5'), (22, 'space'), (25, 'space'), (26, 'a'), (27, 'backspace'), (28, 'space'), (30, 'esc'),
                (33, 'space'), (35, '0'), (38, 'space'), (40, 'esc')] + code57(42), 1, 58, [15, 23.5, 31.5, 39.5, 41]),
    # (no pass kept after the code is typed here: text_input shows the other page every pass, so which
    # of the two has the pointer afterwards depends on how many passes the typing took)
    'end57': (code57(10) + [(80, 'a'), (81, 'enter'), (90, 'space'), (94, 'space')], 2, 100, [92]),
    'live57': ([], 2, 100, [92]),
}
# a walk with a battle played to its end: a key script (port/tests/keys/
# KEYS.keys, its times moved by the seconds given; a key codeN is the N-th
# letter of map M's code, from CODES.DAT) and the pokes of POKES.pokes,
# as play_dumps.py gives them: (KEYS, seconds, POKES, M)
#   end57    as code57, then m57_win's battle (the second side's units
#            poked to none: the first round's end ends it, picture CDh),
#            the best score's name ("A", Enter), the best scores left,
#            START once more: the menu's second return
#   live57   the same as the window played it (menu57_win.keys, what its
#            -keylog wrote of end57 from the menu on)
BATTLES = {'end57': ('m57_win', -8, 'm57_win', 57), 'live57': ('menu57_win', 0, 'm57_win', 57)}
# the end's waits (wait_press) and the frames of the pictures behind the
# loop, as play_dumps.py has them
PRESS = 'S0CB6:0DA9'
# the screens outside the menu's loop (askN, see above)
ASKS = ('S167A:1929', 'S167A:1B5A', 'S167A:334E', 'S167A:3604', 'S167A:0AAE', 'S167A:1C7C')
FRAMES = ('S1A39:0BE6', 'S1A39:0EF6', 'S1A39:10AF', 'S1A39:1359')

for walk in sys.argv[1:] or sorted(WALKS):
    keys, times, until, shots = WALKS[walk]
    out = os.path.join(ROOT, 'build', 'menu', walk)
    os.makedirs(out, exist_ok=True)
    battle, pokes = [], []
    if walk in BATTLES:
        script, dt, tag, m = BATTLES[walk]
        letters = [k for _, k in code(m, 0)]
        for line in open(os.path.join(HERE, 'keys', script + '.keys')):
            f = line.split('#')[0].split()
            if len(f) == 2:
                k = re.sub(r'^code(\d)', lambda x: letters[int(x.group(1)) - 1], f[1])
                battle.append(('%.6f' % (float(f[0]) + dt), k))
        pokes = subprocess.run(['python3', os.path.join(HERE, 'play_pokes.py'), tag],
                               capture_output=True, text=True).stdout.split()
        with open(os.path.join(out, 'pokes.txt'), 'w') as f:
            f.write(subprocess.run(['python3', os.path.join(HERE, 'play_pokes.py'), tag, 'port'],
                                   capture_output=True, text=True).stdout)
    with open(os.path.join(out, 'keys.txt'), 'w') as f:
        for t, k in sorted(keys + battle, key=lambda x: float(x[0])):
            f.write('%s %s\n' % (t, k))
    with open(os.path.join(out, 'battle.keys'), 'w') as f:
        for t, k in battle:
            f.write('%s %s\n' % (t, k))
    # what the original writes (a best score) goes to a folder of its own,
    # emptied before each run, so that every run begins alike
    state = os.path.join(out, 'state')
    os.makedirs(state, exist_ok=True)

    def run(args):
        for d, _, names in os.walk(state):
            for name in names:
                os.remove(os.path.join(d, name))
        return subprocess.run(['python3', KIT] + args + ['HL.EXE'], capture_output=True, text=True, cwd=out)

    a = ['-key', '3', 'enter', '-key', '6', 'enter', '-state', state] + pokes
    if battle:
        a += ['-keys', os.path.join(out, 'battle.keys')]
    for t, k in keys:
        a += ['-key', str(t), k]
    a += ['-log', 'S167A:130B', '-log', 'S066A:0390']
    for l in ASKS:
        a += ['-log', l]
    if battle:
        for l in (PRESS,) + FRAMES:
            a += ['-log', l]
    a += ['-until', str(until), '-break', 'S066A:0390#%d' % times, '-ram', os.path.join(out, 'exit.ram')]
    r = run(a)
    # what that run wrote is kept as written/ (the later runs stop earlier)
    written = os.path.join(out, 'written')
    for d, _, names in os.walk(written):
        for name in names:
            os.remove(os.path.join(d, name))
    for d, _, names in os.walk(state):
        for name in names:
            to = os.path.join(written, os.path.relpath(d, state))
            os.makedirs(to, exist_ok=True)
            shutil.copyfile(os.path.join(d, name), os.path.join(to, name))
    open(os.path.join(out, 'log.txt'), 'w').write(r.stdout + r.stderr)
    acts, start, exits = [], None, []
    left = list(shots)      # the pass of the loop each shot's second falls to
    at = []
    asks = []               # (address, hit) of each screen of the saved game's question
    ends = []               # (address, hit, file) of the end's waits and every tenth frame
    waits = frames = 0
    for line in (r.stdout + r.stderr).split('\n'):
        m = re.match(r'log HL\.EXE\+(\w{4}:\w{4}) t=(\S+) hit=(\d+) regs', line)
        if m and 'S' + m.group(1) == PRESS:
            waits += 1
            ends.append((PRESS, waits, 'e%d' % waits))
            continue
        if m and 'S' + m.group(1) in FRAMES:
            if frames % 10 == 0:
                ends.append(('S' + m.group(1), int(m.group(3)), 'f%04d' % (frames + 1)))
            frames += 1
            continue
        m = re.match(r'log HL\.EXE\+(\w{4}:\w{4}) t=(\S+) hit=(\d+) regs .*?AX=([0-9A-F]{4})', line)
        if not m:
            continue
        if 'S' + m.group(1) in ASKS:
            asks.append((m.group(1), int(m.group(3))))
            continue
        if m.group(1) == '167A:130B' and left and float(m.group(2)) >= left[0]:
            at.append((left.pop(0), int(m.group(3))))
        m = re.match(r'log HL\.EXE\+(\w{4}:\w{4}) t=(\S+) hit=\d+ regs .*?AX=([0-9A-F]{4})', line)
        if m.group(1) == '066A:0390':
            exits.append(m.group(2))
            acts.append('%s 0' % m.group(2))
            continue
        if start is None or (acts and acts[-1].endswith(' 0')):
            if start is None:
                start = m.group(2)
        if int(m.group(3), 16):
            acts.append('%s %d' % (m.group(2), int(m.group(3), 16)))
    open(os.path.join(out, 'acts.txt'), 'w').write('\n'.join(acts) + '\n')
    open(os.path.join(out, 'start.txt'), 'w').write('%s\n' % start)
    # video memory where that pass begins (S167A:0D3C), a run each
    for t, n in at:
        run(a[:a.index('-log')] + ['-until', str(until), '-break', 'S167A:0D3C#%d' % n,
                                   '-vram', os.path.join(out, 'shot%s.vram' % t)])
    for i, (ad, n) in enumerate(asks):
        run(a[:a.index('-log')] + ['-until', str(until), '-break', 'S%s#%d' % (ad, n),
                                   '-ram', os.path.join(out, 'ask%d.ram' % (i + 1)),
                                   '-vram', os.path.join(out, 'ask%d.vram' % (i + 1))])
    open(os.path.join(out, 'asks.txt'), 'w').write('%d\n' % len(asks))
    # memory and video memory at the end's waits and frames (eN, fNNNN as
    # play_dumps.py names them)
    for ad, n, name in ends:
        run(a[:a.index('-log')] + ['-until', str(until), '-break', '%s#%d' % (ad, n),
                                   '-ram', os.path.join(out, name + '.ram'), '-vram', os.path.join(out, name + '.vram')])
    open(os.path.join(out, 'ends.txt'), 'w').write(' '.join(name for _, _, name in ends) + '\n')
    if walk == 'exit':
        run(a[:a.index('-log')] + ['-until', str(until), '-break', 'S27FE:0060', '-ram', os.path.join(out, 'end.ram')])
    open(os.path.join(out, 'shots.txt'), 'w').write(' '.join(str(t) for t, n in at) + '\n')
    print('%s: first pass at %s s, %d things came in, the menu returned at %s' %
          (walk, start, len([x for x in acts if not x.endswith(' 0')]), ', '.join(exits) or 'no time'))
