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
    name57   as end57 with the name corrected: a letter and the backspace,
             the backspace with nothing typed, five letters, a sixth (not
             taken), the backspace twice, a letter, Enter; then MISC and
             the best scores of the map just won (one score, three
             EMPTY), left, START: the menu's second return
    name5x   as name57 with a name of five letters typed straight (QWERT)
             and Enter at the same time: the name is closed properly
    twice57  begun with the best scores' file name57's run wrote (SEEDS
             below; name57 must have been made first): map 57's code,
             MISC, the best scores (the one there), START, the battle
             won again (the same score a second time), a name with one
             backspace, the two scores shown, START
    livename57, livetwice57   those two as the headless window played
             them (keys/menu57_name.keys, menu57_twice.keys, its -keylog)
    joy57    as code57 with a joystick for the keys' directions and
             button (keys/menu_joy57.joy, the runner's -joys; the stick
             is there from the program's start): down, its first button
             (the CODE row), the code typed, up, the button (START, which
             takes this first press: the stick's button is no key)
    joyload  a joystick alone: down four times and the button (MISC),
             down and the button (LOAD), the first button at the saved
             game's question (it leaves it), the second button (LOAD
             again), left and right at the question (nothing), the first
             button (left again); Esc; the button (START), left (the
             other country) and the button: map 24
    joyask   two joysticks (keys/menu_joyask.joy): the first to MISC and
             LOAD as in joyload; at the saved game's question its second
             button; LOAD again and both its buttons at once; LOAD again
             and the second stick's second button; Esc by the key (the
             main menu), then as code57
    mouseask the keys to MISC and LOAD, at the saved game's question the
             mouse (keys/menu_mouseask.mice): its first button; LOAD
             again (the key pressed twice, see code57) and its second
             button; LOAD again, the pointer moved with no button and
             both buttons; Esc (the main menu), then as code57
    mouse3   the mouse's third button (keys/menu_mouse3.mice): the keys
             to MISC and LOAD; at the saved game's question the third
             button alone (the question stays), then the first (left);
             LOAD again, the third held and the second beside it (left
             at the second); the third in the menu with LOAD the item
             (nothing); Esc, then as code57 a tenth later (the cursor
             of text_input, see joy57 below)
    heldask  a button held from before the saved game's question
             (keys/menu_heldask.joy, .mice): the first stick to MISC and
             LOAD, its first button on LOAD held for three seconds with
             Esc by the key a second before it is let go (the question
             is drawn only when the button is let go, and the Esc is
             not taken), the button again (left); the mouse's first
             button on LOAD held for three seconds (the same), its
             second (left); Esc (the main menu), then as code57 a tenth
             later

A walk WALK with a file port/tests/keys/menu_WALK.joy gets that sticks'
script (the runner's -joys; menu_test.sh gives it to the port's -joys),
one with a file menu_WALK.mice that mouse's script (-mice in both).

The runner's files written (a best score) go to WALK/state/, emptied
before each run (and for a walk of SEEDS filled with the other walk's
written/); those of the first run are kept as WALK/written/.  WALK/seed/
has the files the port's program is to begin with (named as its -saves
folder names them).

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
    'name57': (code57(10) + [(78, 'b'), (78.5, 'backspace'), (79, 'backspace'), (79.5, 'q'), (80, 'w'), (80.5, 'e'),
                             (81, 'r'), (81.5, 't'), (82, 'y'), (82.5, 'backspace'), (83, 'backspace'), (83.5, 'u'),
                             (84.825, 'enter'), (91, 'space'), (95, 'down'), (96, 'down'), (97, 'down'), (98, 'down'),
                             (99, 'space'), (101, 'space'), (104, 'space'), (106, 'esc'), (108, 'space')],
               2, 116, [93, 100, 105, 107]),
    # name57 with a name of five letters and no backspace: the name is closed by text_input's
    # 0, no `]` (the port's deviation that takes the cursor off a shorter name changes nothing here)
    'name5x': (code57(10) + [(79.5, 'q'), (80, 'w'), (80.5, 'e'), (81, 'r'), (81.5, 't'),
                             (84.825, 'enter'), (91, 'space'), (95, 'down'), (96, 'down'), (97, 'down'), (98, 'down'),
                             (99, 'space'), (101, 'space'), (104, 'space'), (106, 'esc'), (108, 'space')],
               2, 116, [93, 100, 105, 107]),
    # (name57's Enter at 84.705: text_input shows the other page every pass and the port's passes are not
    # the original's, so which page has the pointer afterwards depends on both counts; with Enter in these
    # 10 ms both are odd, 799 and 873 with the code's; twice57's first START takes the second press as
    # code57's)
    'twice57': ([(10, 'down'), (11, 'space')] + code(57, 12) +
                [(15, 'enter'), (16, 'down'), (17, 'down'), (18, 'down'), (19, 'space'), (20, 'space'), (21.5, 'space'),
                 (24, 'space'), (26, 'esc'), (28, 'space'), (88, 'n'), (88.5, 'o'), (89, 'backspace'), (89.5, 'e'), (90, 'w'),
                 (91, 'enter'), (100, 'space'), (104, 'space')], 2, 112, [20.7, 27, 102]),
    'livename57': ([], 2, 116, [93, 100, 105, 107]),
    'livetwice57': ([], 2, 112, [20.7, 27, 102]),
    # (joy57's Enter at 15.1: text_input's cursor, the `]`, comes and goes by its passes, and the port's
    # passes are not the original's; at 15.0 the original's last pass had it and the port's did not, as
    # it is at 15.2, 15.4 and 15.5; at 15.1, 15.3 and 15.6 both have the same)
    'joy57': (code(57, 12) + [(15.1, 'enter')], 1, 24, [9, 10.6, 16.6]),
    'joyload': ([(27, 'esc')], 1, 40, [9, 15, 20, 26, 28]),
    'joyask': ([(30, 'esc')] + code57(32), 1, 48, [9, 20, 24, 28.5, 31]),
    'mouseask': ([(10, 'down'), (11, 'down'), (12, 'down'), (13, 'down'), (14, 'space'), (16, 'down'), (17, 'space'),
                  (21, 'space'), (22, 'space'), (26, 'space'), (27, 'space'), (33, 'esc')] + code57(35),
                 1, 51, [9, 20, 25, 31.5, 34]),
    'mouse3': ([(10, 'down'), (11, 'down'), (12, 'down'), (13, 'down'), (14, 'space'), (16, 'down'), (17, 'space'),
                (23, 'space'), (24, 'space'), (30, 'esc')] + code57(32.1), 1, 48, [9, 22.5, 27.5, 29, 31]),
    'heldask': ([(19, 'esc'), (31, 'esc')] + code57(33.1), 1, 49, [9, 23, 30, 32]),
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
#   name57, twice57   m57_win's battle too (twice57's a second later:
#            its START comes 9 s after end57's)
BATTLES = {'end57': ('m57_win', -8, 'm57_win', 57), 'live57': ('menu57_win', 0, 'm57_win', 57),
           'name57': ('m57_win', -8, 'm57_win', 57), 'name5x': ('m57_win', -8, 'm57_win', 57), 'twice57': ('m57_win', 1.04, 'm57_win', 57),
           'livename57': ('menu57_name', 0, 'm57_win', 57), 'livetwice57': ('menu57_twice', 0, 'm57_win', 57)}
# walks that begin with the files another walk's first run wrote
SEEDS = {'twice57': 'name57', 'livetwice57': 'name57'}
# walks whose battle's end is not dumped again (end57 and live57 have it)
NOENDS = ('name57', 'name5x', 'twice57', 'livename57', 'livetwice57')
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

    seed = os.path.join(ROOT, 'build', 'menu', SEEDS[walk], 'written') if walk in SEEDS else None
    if seed and not os.path.isdir(seed):
        sys.exit('%s begins with what %s wrote: python3 port/tests/menu_dumps.py %s' % (walk, SEEDS[walk], SEEDS[walk]))
    # the same files for the port's program, as its -saves folder names
    # them (a folder's separator is _)
    shutil.rmtree(os.path.join(out, 'seed'), ignore_errors=True)
    os.makedirs(os.path.join(out, 'seed'))
    for d, _, names in os.walk(seed or ''):
        for name in names:
            rel = os.path.relpath(os.path.join(d, name), seed)
            shutil.copyfile(os.path.join(d, name), os.path.join(out, 'seed', rel.replace(os.sep, '_')))

    def run(args):
        for d, _, names in os.walk(state):
            for name in names:
                os.remove(os.path.join(d, name))
        for d, _, names in os.walk(seed or ''):
            for name in names:
                to = os.path.join(state, os.path.relpath(d, seed))
                os.makedirs(to, exist_ok=True)
                shutil.copyfile(os.path.join(d, name), os.path.join(to, name))
        return subprocess.run(['python3', KIT] + args + ['HL.EXE'], capture_output=True, text=True, cwd=out)

    a = ['-key', '3', 'enter', '-key', '6', 'enter', '-state', state] + pokes
    # the walk's joysticks, if it has a script of them
    if os.path.exists(os.path.join(HERE, 'keys', 'menu_%s.joy' % walk)):
        a += ['-joys', os.path.join(HERE, 'keys', 'menu_%s.joy' % walk)]
    if os.path.exists(os.path.join(HERE, 'keys', 'menu_%s.mice' % walk)):
        a += ['-mice', os.path.join(HERE, 'keys', 'menu_%s.mice' % walk)]
    if battle:
        a += ['-keys', os.path.join(out, 'battle.keys')]
    for t, k in keys:
        a += ['-key', str(t), k]
    a += ['-log', 'S167A:130B', '-log', 'S066A:0390']
    for l in ASKS:
        a += ['-log', l]
    if battle and walk not in NOENDS:
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
