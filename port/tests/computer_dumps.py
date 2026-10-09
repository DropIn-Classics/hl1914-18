#!/usr/bin/env python3
"""Makes the dumps port/tests/computer_test.sh compares the port's
computer_input and computer_step with (HL.EXE's S1E4B:0001 and S1E4B:012E).

    python3 port/tests/computer_dumps.py [-until T] [-early T] [stage2]
    python3 port/tests/computer_dumps.py [-until T] phases [stage2] [assess] [queue] [plan]

Four battles are played on the runner by generated input, two with the
computer's second side (the maps of the codes 0 and 1) and two with the
computer's first side (those of the codes 24 and 25); the code is read
from the installed game's CODES.DAT.  The first of each pair gives the
calls of computer_input and the steps of the kinds 1 to 3, the second
those of the kinds 4, 6 and 7 (the computer builds there).  Stage 1 logs every call of the two routines up to T
emulated seconds (default 1500); stage 2 (alone with `stage2`, from the
logs there are, keeping the dumps there are) runs each battle again up
to a chosen call and dumps memory behind the routine's entry code and
before its return.  Chosen:
calls of computer_input spread over those before the early time (default
150 s), and those that returned 0 with the call behind each; of
computer_step three calls of each state of each kind of step (the state's
entry is logged), spread over those before twice the early time or else
the first there are, and three ends of a list.  Everything goes to build/computer/ (ignored): the key and
mouse files, the logs, the dumps and computer_test.args.  About an hour
on twelve cores.

With `phases` the same four battles give the calls of the planning phases
instead (PLANS: computer_assess, computer_queue, computer_plan; all, or
those named): stage 1 logs each routine's entry, its exits and the places
that tell one call from another (the entries of its jump tables and its
helpers', read from the player's HL.EXE, and a few branches), into
NAME_plan0.log and NAME_plan1.log (the runner logs 64 places a run);
stage 2 dumps early calls of every set of places a run went through (the
first whenever it is, more of the first 400 s of computer_queue and 150 s of computer_plan: a dump costs a run up to
its time)
and writes assess_test.args, queue_test.args and plan_test.args, which
port/tests/computer_test.sh takes as well.

    python3 port/tests/computer_dumps.py [-until T] turn [stage2]
    python3 port/tests/computer_dumps.py late [-per N]

With `turn` four battles are played in which the human only ends its
halves (play_quiet; TURNS: the code and the halves taken) and every call
of computer_turn (S1C74:000B) is logged up to T (default 500 s) with what
it called.  Stage 2 dumps three single calls of each kind (what was
called, the kind of step) at entry and exit (turn_test.args, kind
`turn:1`) and cuts the halves named into stretches the port can run
without game_loop: the thinking between two steps, a pass that queues or
steps with the passes that only play its script, and passes waiting for
the round's end.  Each stretch starts from the original's memory at its
first call and is compared with that at the next stretch's first call
(turns_test.args, kind `turns:N`).  With `late` the paths that only come
up late or on other maps are dumped from quiet battles (LATE: the code,
the time, and the places a call has to go through): stage 1 and 2 in
one, late_test.args.  Three of them are battles changed by a poke
(POKED), for paths no battle reaches by itself.  No game data is kept in this file.
"""
import os, random, re, struct, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
sys.path.insert(0, os.path.join(ROOT, 'doskit', 'tools'))
import datfiles
from kit import game_dir

D = 'build/computer'
# name: code index, seed, the kinds of step dumped from it (0: a list's
# end), whether computer_input's calls are dumped
RUNS = {'second': (0, 7000, (0, 1, 2, 3), True), 'first': (24, 7024, (0, 1, 2, 3), True),
        'second_b': (1, 7001, (4, 6, 7), False), 'first_b': (25, 7025, (4, 6, 7), False)}
# computer_step: entry, exits (behind the last write of each), and the
# seven kinds' entries of its table
STEP_IN, STEP_OUT, STEP_END = '014F', '035B', '0372'
KINDS = {'01A3': 1, '01E2': 2, '0222': 3, '0265': 4, '029D': 5, '02D4': 6, '030B': 7}
# the handlers' tables of their states' entries (the kind, the table's
# offset in S1E4B, its entries); the offsets are read from the player's
# HL.EXE (states())
TABLES = {1: (0x06CF, 8), 2: (0x08E1, 6), 3: (0x0A1C, 5), 4: (0x0D6E, 13), 6: (0x0FAA, 7), 7: (0x127E, 8)}
S1E4B = 0x1E4B
# how many calls of each state of a kind of step a run gives, and of the
# lists' ends
PER_STATE, ENDS = 3, 3
INPUT_SPREAD, INPUT_SCRIPT = 14, 12
# the planning phases: the routine's entry behind the frame's code, its
# exits (a place and the result there; None: the result is AX, at the
# place computer_turn gets back to), jump tables whose entries tell calls
# apart (segment, offset, entries; the helpers' too) and further places,
# how many calls of a set of places a run gives and of the routine at most,
# every which call of a set is taken, and up to which time (a set's first
# call is dumped whenever it is)
PLANS = {
    'assess': ('1CC6:0A54', (('1CC6:1835', 0), ('1CC6:182B', 1)), (('1CC6', 0x1845, 6),),
               ('1CC6:0BFF', '1CC6:0CF6', '1CC6:0D71', '1CC6:1027', '1CC6:11B8'), 3, 40, 1, 1500.0),
    'queue': ('1CC6:0022', (('1C74:010F', None),),
              (('1CC6', 0x0A23, 9), ('1CC6', 0x0A0B, 12), ('1CC6', 0x09F1, 13), ('202F', 0x0B97, 8)),
              ('202F:0105', '202F:011D', '202F:0186', '202F:02C9', '202F:0482', '1CC6:08AA', '1CC6:092A'),
              3, 90, 5, 400.0),
    'plan': ('2138:002B', (('1C74:00CE', None),),
             (('2138', 0x2CDD, 13), ('2138', 0x2CCB, 5), ('2138', 0x2CC1, 5),
              ('2138', 0x2CB5, 6), ('2138', 0x2CA9, 6), ('2138', 0x2C9F, 5), ('2138', 0x2C87, 12),
              ('2138', 0x2C71, 11)), (), 2, 120, 5, 150.0),
    # computer_plan again, told apart by plan_ride's states (the kind `late`)
    'ride': ('2138:002B', (('1C74:00CE', None),), (('2407', 0x06C5, 8), ('2138', 0x2CDD, 13)),
             ('2407:0695',), 2, 120, 5, 150.0),
}
# the runner logs 64 places a run: which routines share a run's log
PLAN_LOGS = (('assess', 'queue'), ('plan',), ('ride',))
# the kind `late`: quiet battles in which paths come up that the others
# never reach (found by logging these places in a quiet battle on each of
# the 48 maps for one player).  name: code index, how long, and for a
# routine the places wanted (a call that went through one is dumped) or
# for `step` the kinds of step and states; a place with a second address
# is taken only behind the first time that address was reached in the
# routine named with it
LATE = {
    # a repair: the plan 6 and the step of the kind 6
    'first_q26': (26, 300, {'step': [(6, n) for n in range(7)], 'queue': ['1CC6:054F']}),
    # a carrier sent for a unit (the plan 0Dh, plan_ride's state 6), the
    # plans 9 to 0Bh and 0Ch, plan_ride's states 1 to 4
    'second_q11': (11, 800, {'queue': [('1CC6:0623', 'ride', '2407:05F3'), '1CC6:0780', '1CC6:02F7'],
                             'ride': ['2407:05F3', '2407:00E2', '2407:014D', '2407:019A', '2407:02AA']}),
    # computer_assess's state 0 with a unit that has the plan 9
    'second_q8': (8, 760, {'assess': ['1CC6:0BFF'], 'queue': ['1CC6:0780', '1CC6:02F7']}),
    # step_attack's state 4
    'first_q45': (45, 260, {'step': [(2, 4)]}),
    # computer_plan's state 4, an aim of the kind 2
    'second_q22': (22, 120, {'plan': ['2138:05D6']}),
    # the paths no battle reaches by itself, reached by a poke (POKED):
    # plan_approach (the plan 7 at computer_queue's state 5)
    'second_p22': (22, 140, {'queue': ['1CC6:0680']}),
    # computer_plan's state 6, an aim of the kind 4 (a unit to fetch)
    'second_p0': (0, 70, {'plan': ['2138:0DCD']}),
    # computer_assess's state 0 with plan_to_site
    'second_p11': (11, 925, {'assess': ['1CC6:0AD6']}),
}
# Pokes for LATE's battles (as run.py takes them: where, what, the bytes)
# and the time from which their calls are taken.  No battle gives these
# paths: on the 48 maps for one player no kind that takes sites (1 in
# word 2 of kind_worth) has both 20h there and followers (byte 4), so no
# plan 7 is made, and no kind of unit has 4 in its word 6 (word 11h of
# unit_dat), so nothing is listed to fetch nor carried there.
# - map 22: byte 4 of kind 12h's kind_worth (that kind has 20h and 1)
#   from 0 to 2;
# - map 0: 4 set in word 6 of unit 1Ch, of no side (2 in unit_flags), 340h
#   at the start;
# - map 11: 4 set in word 6 of unit 24h (341h), in carrier 41h of the
#   computer's side at its assessment at 918 s, and the carrier's plan
#   (9, a repair, which skips the test) made 0.
POKED = {
    'second_p22': ([('S066A:0EE9#1', 'S39B6:0070', '02')], 0.0),
    'second_p0': ([('S066A:0EE9#1', 'S331F:1B7A', '4403')], 0.0),
    'second_p11': ([('S1CC6:0A54#43', 'S331F:1C4A', '4503'),
                    ('S1CC6:0A54#43', 'S39B6:0BFC', '00')], 918.0),
}


def code(index):
    """a map's code as the menu takes it: five letters of the record"""
    data = datfiles.unpacked(os.path.join(game_dir(), 'CODES.DAT'))
    return ''.join(chr(b - 0x11 + ord('a')) for b in data[index * 10:index * 10 + 5])


def states():
    """{offset in S1E4B: (kind, state)} of the handlers' states, from the
    jump tables in the program's file"""
    exe = open(os.path.join(game_dir(), 'HL.EXE'), 'rb').read()
    base = struct.unpack_from('<H', exe, 8)[0] * 16 + S1E4B * 16
    out = {}
    for kind, (table, n) in TABLES.items():
        for state, off in enumerate(struct.unpack_from('<%dH' % n, exe, base + table)):
            out['%04X' % off] = (kind, state)
    return out


def play(name, index, seed, end):
    """key and mouse files: the main menu, the code, START, then clicks
    and drags anywhere and the key pair that ends the human's round (as
    the scratch generator of the long runs made them, its mode FS)"""
    r = random.Random(seed)
    keys, mice = ['3 enter', '6 enter'], []

    def click(t, x, y, b=1):
        mice.extend([f'{t:.2f} {x},{y},0', f'{t+0.5:.2f} {x},{y},{b}', f'{t+0.7:.2f} {x},{y},0'])
    t = 8.5
    click(t, 150, 73); t += 1.5
    for ch in code(index):
        keys.append(f'{t:.2f} {ch}'); t += 0.6
    keys.append(f'{t:.2f} enter'); t += 2
    click(t, 150, 48); t += 3
    for d in (0, 3, 7):
        keys.append(f'{t + d:.2f} enter')
    t += 14
    x, y = 80, 100
    nf1 = t + 8
    while t < end:
        c = r.random()
        if t > 600:
            x, y = r.randrange(2, 318), r.randrange(2, 198)
            mice += [f'{t:.2f} {x},{y},0', f'{t+0.3:.2f} {x},{y},1', f'{t+0.5:.2f} {x},{y},0']
            keys += [f'{t+2:.2f} f1', f'{t+3.5:.2f} f1']; t += 15
            continue
        if t >= nf1:
            keys += [f'{t:.2f} f1', f'{t+1.2:.2f} f1']; t += 2.5; nf1 = t + r.choice([1, 2, 4])
            continue
        if c < 0.35:
            x, y = r.randrange(2, 318), r.randrange(2, 198)
            b = r.choice([1, 1, 1, 2, 3])
            mice += [f'{t:.2f} {x},{y},0', f'{t+0.3:.2f} {x},{y},{b}', f'{t+0.5:.2f} {x},{y},0']; t += 0.9
        elif c < 0.6:
            b = r.choice([1, 1, 2])
            dx, dy = r.choice([(-24, 0), (24, 0), (0, -16), (0, 16), (-24, -16), (24, 16)])
            mice.append(f'{t:.2f} {x},{y},{b}')
            for k in range(1, 4):
                x = min(317, max(2, x + dx)); y = min(197, max(2, y + dy))
                mice.append(f'{t+0.15*k:.2f} {x},{y},{b}')
            mice.append(f'{t+0.7:.2f} {x},{y},0'); t += 1.0
        elif c < 0.8:
            x = min(317, max(2, x + r.choice([-24, 24, 0, 0]))); y = min(197, max(2, y + r.choice([-16, 16, 0])))
            mice.append(f'{t:.2f} {x},{y},0'); t += 0.3
        else:
            keys.append(f'{t:.2f} ' + r.choice(['up', 'down', 'left', 'right', 'space', 'enter', 'space', 'enter', 'f1', 'f5', 'tab', 'a', 'm', '1'])); t += 0.4
    open('%s/%s.keys' % (D, name), 'w').write('\n'.join(keys) + '\n')
    open('%s/%s.mice' % (D, name), 'w').write('\n'.join(mice) + '\n')


def play_quiet(name, index, end):
    """key and mouse files of a human who only ends each half: the menu
    and the code as play() has them, then every eight seconds the gesture
    that ends a half and the key pair that lets the round end"""
    keys, mice = ['3 enter', '6 enter'], []

    def click(t, x, y, b=1):
        mice.extend([f'{t:.2f} {x},{y},0', f'{t+0.5:.2f} {x},{y},{b}', f'{t+0.7:.2f} {x},{y},0'])
    t = 8.5
    click(t, 150, 73); t += 1.5
    for ch in code(index):
        keys.append(f'{t:.2f} {ch}'); t += 0.6
    keys.append(f'{t:.2f} enter'); t += 2
    click(t, 150, 48); t += 3
    for d in (0, 3, 7):
        keys.append(f'{t + d:.2f} enter')
    t += 22
    for k in range(2):      # off the headquarters, where the view begins
        keys += [f'{t:.2f} down+', f'{t+0.08:.2f} down-']; t += 1
    while t < end:          # the button and left on a plain field, F1 twice
        keys += [f'{t:.2f} space+', f'{t+0.3:.2f} left+', f'{t+0.7:.2f} space-', f'{t+0.9:.2f} left-',
                 f'{t+2:.2f} f1', f'{t+2.8:.2f} f1']; t += 8
    open('%s/%s.keys' % (D, name), 'w').write('\n'.join(keys) + '\n')
    open('%s/%s.mice' % (D, name), 'w').write('\n'.join(mice) + '\n')


def run(name, out, until, opts):
    st = '%s/st_%s' % (D, out)
    subprocess.run(['rm', '-rf', st])
    cmd = [sys.executable, 'doskit/tools/run.py', '-state', st, '-until', str(until),
           '-keys', '%s/%s.keys' % (D, name), '-mice', '%s/%s.mice' % (D, name)] + opts
    for at, target, data in POKED.get(name, ([], 0))[0]:
        cmd += ['-poke', at, target, data]
    cmd += ['HL.EXE']
    with open('%s/%s.log' % (D, out), 'w') as f:
        subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT, timeout=7200)
    subprocess.run(['rm', '-rf', st])


def stage1(name, until):
    opts = []
    for a in ['0023', '0120', STEP_IN, STEP_OUT, STEP_END] + list(KINDS) + list(states()):
        opts += ['-log', 'S1E4B:' + a]
    run(name, name, until, opts)


def calls(name):
    """(time, side, SS:BP, entry, exit, result) of computer_input's calls,
    and (time, side, SS:BP, entry, exit, kind, state) of computer_step's
    (kind 0: the list's end; state: of the kind's handler, None for the
    kind 5)"""
    inputs, steps, cur, step, st = [], [], None, None, states()
    for line in open('%s/%s.log' % (D, name)):
        m = re.match(r'log HL\.EXE\+1E4B:(\w+) t=(\S+) hit=(\d+) regs .*?AX=(\w+) .* SI=(\w+) DI=(\w+) BP=(\w+) .* SS=(\w+)', line)
        if not m:
            continue
        a, t, hit, ax, si, di, bp, ss = m.groups()
        t = float(t)
        if a == '0023':
            cur = (t, int(di, 16), '%s:%s' % (ss, bp), 'S1E4B:0023#' + hit, 'S1E4B:011D#' + hit)
        elif a == '0120' and cur:
            inputs.append(cur + (int(ax, 16),))
            cur = None
        elif a == STEP_IN:
            step = [t, int(si, 16), '%s:%s' % (ss, bp), 'S1E4B:%s#%s' % (a, hit), None, 0, None]
        elif a in KINDS and step:
            step[5] = KINDS[a]
        elif a in st and step and st[a][0] == step[5] and step[6] is None:
            step[6] = st[a][1]
        elif a in (STEP_OUT, STEP_END) and step:
            step[4] = 'S1E4B:%s#%s' % (a, hit)
            steps.append(tuple(step))
            step = None
    return inputs, steps


def plan_marks(what):
    """the places of a routine that are logged to tell its calls apart"""
    exe = open(os.path.join(game_dir(), 'HL.EXE'), 'rb').read()
    head = struct.unpack_from('<H', exe, 8)[0] * 16
    out = list(PLANS[what][3])
    for seg, table, n in PLANS[what][2]:
        out += ['%s:%04X' % (seg, o) for o in struct.unpack_from('<%dH' % n, exe, head + int(seg, 16) * 16 + table)]
    return sorted(set(out))


def plan_stage1(name, until, group):
    whats, places = PLAN_LOGS[group], set()
    for what in whats:
        places |= set([PLANS[what][0]] + [a for a, _ in PLANS[what][1]] + plan_marks(what))
    opts = []
    for a in sorted(places):
        opts += ['-log', 'S' + a]
    run(name, '%s_plan%d' % (name, group), until, opts)


def plan_calls(name, whats):
    """{routine: [(time, SS:BP, entry, exit, result, the places gone
    through)]} of the calls of the planning routines in a run's log"""
    entries = {PLANS[w][0]: w for w in whats}
    marks = {w: set(plan_marks(w)) for w in whats}
    out, cur, lines = {w: [] for w in whats}, None, []
    for group, of in enumerate(PLAN_LOGS):
        if set(of) & set(whats):
            lines += list(open('%s/%s_plan%d.log' % (D, name, group))) + ['end']
    for line in lines:
        if line == 'end':
            cur = None
        m = re.match(r'log HL\.EXE\+(\w+:\w+) t=(\S+) hit=(\d+) regs .*?AX=(\w+) .* BP=(\w+) .* SS=(\w+)', line)
        if not m:
            continue
        a, t, hit, ax, bp, ss = m.groups()
        if a in entries:
            cur = [entries[a], float(t), '%s:%s' % (ss, bp), 'S%s#%s' % (a, hit), set()]
            continue
        if not cur:
            continue
        ends = dict(PLANS[cur[0]][1])
        if a in ends:
            result = int(ax, 16) if ends[a] is None else ends[a]
            out[cur[0]].append((cur[1], cur[2], cur[3], 'S%s#%s' % (a, hit), result, tuple(sorted(cur[4]))))
            cur = None
        elif a in marks[cur[0]]:
            cur[4].add(a)
    return out


def plan_main(args, until):
    whats = [w for w in PLANS if w in args] or [w for w in PLANS if w != 'ride']
    if 'stage2' not in args:
        for name, run_ in RUNS.items():
            if not os.path.exists('%s/%s.keys' % (D, name)):
                play(name, run_[0], run_[1], until)
        with ThreadPoolExecutor(len(RUNS) * len(PLAN_LOGS)) as ex:
            list(ex.map(lambda j: plan_stage1(j[0], until, j[1]),
                        [(n, g) for n in RUNS for g in range(2)]))
    jobs, keep = [], 'stage2' in args
    lines = {w: [] for w in whats}
    for name in RUNS:
        found = plan_calls(name, whats)
        for what in whats:
            by = {}
            for c in found[what]:
                by.setdefault((c[4], c[5]), []).append(c)
            print('%s: %s, %d sets of places, calls by result and places: %s' % (name, what, len(by), {
                '%d/%s' % (k[0], '+'.join(a.split(':')[1] for a in k[1])): len(v) for k, v in sorted(by.items())}))
            per, most, nth, early = PLANS[what][4:8]
            chosen = []
            for n in range(per):        # the first of every set, then a later one, ...
                chosen += [of[n * nth] for k, of in sorted(by.items())
                           if len(of) > n * nth and (n == 0 or of[n * nth][0] <= early)]
            for t, ssbp, a_in, a_out, result, _ in sorted(chosen[:most]):
                base = '%s_%s_%s' % (name, what, a_in.split('#')[1])
                for way, a in (('e', a_in), ('x', a_out)):
                    ram = '%s/%s%s.ram' % (D, base, way)
                    if not (keep and os.path.exists(ram)):
                        jobs.append((name, base + way, int(t) + 5, ['-break', a, '-ram', ram]))
                lines[what].append('%s%s %d %s ../%s/%se.ram ../%s/%sx.ram' % (
                    what, 'end' if result else '', 0 if name.startswith('first') else 1,
                    ssbp, D, base, D, base))
    with ThreadPoolExecutor(int(os.environ.get('DUMP_JOBS', '10'))) as ex:
        list(ex.map(lambda j: run(*j), jobs))
    for what in whats:
        open('%s/%s_test.args' % (D, what), 'w').write('\n'.join(lines[what]) + '\n')
        print('computer_dumps: %d calls in %s/%s_test.args' % (len(lines[what]), D, what))


# computer_turn: its entry behind the frame's code, its end, and behind
# its five calls (the routine's letter; the result is AX there)
TURN_IN, TURN_OUT = '1C74:002C', '1C74:01AE'
TURN_CALLS = {'1C74:0064': 'A', '1C74:00CE': 'P', '1C74:010F': 'Q', '1C74:014E': 'S', '1C74:0199': 'I'}
# step_end_round's state that waits for the round's end (4 in
# computer_phase), and where it sees it
ROUND_WAIT, ROUND_SEEN = '1E4B:09C6', '1E4B:09EB'
# the quiet battles of the kind `turn`: name: code index, the halves
# whose stretches are dumped
TURNS = {'quiet_second': (0, (0, 1, 12)), 'quiet_first': (24, (0, 1, 15)),
         'quiet_second_b': (1, ()), 'quiet_first_b': (25, (1, 3))}


def turn_passes(name):
    """the calls of computer_turn in a quiet run's log: (time, hit,
    SS:BP, what it called: a string of the routines' letters with their
    results, the kind of step, whether the round's end was seen, whether
    it was waited for)"""
    out, cur = [], None
    for line in open('%s/%s_turn.log' % (D, name)):
        m = re.match(r'log HL\.EXE\+(\w+:\w+) t=(\S+) hit=(\d+) regs .*?AX=(\w+) .* BP=(\w+) .* SS=(\w+)', line)
        if not m:
            continue
        a, t, hit, ax, bp, ss = m.groups()
        if a == TURN_IN:
            cur = [float(t), int(hit), '%s:%s' % (ss, bp), '', 0, False, False]
        elif not cur:
            continue
        elif a in TURN_CALLS:
            cur[3] += TURN_CALLS[a] + ('0' if int(ax, 16) == 0 else '1' if TURN_CALLS[a] != 'S' else 'F')
        elif a[:5] == '1E4B:' and a[5:] in KINDS:
            cur[4] = KINDS[a[5:]]
        elif a == ROUND_SEEN:
            cur[5] = True
        elif a == ROUND_WAIT:
            cur[6] = True
        elif a == TURN_OUT:
            out.append(tuple(cur))
            cur = None
    return out


def turn_halves(passes):
    """the passes of each of the side's halves: a half begins with the
    first call of computer_assess behind a round's end (or the first of
    all)"""
    halves, ended = [], True
    for p in passes:
        if ended and p[3].startswith('A'):
            halves.append([])
            ended = False
        if p[5]:
            ended = True
        if halves:
            halves[-1].append(p)
    return halves


def turn_segments(half):
    """a half cut into stretches of passes the port can run without what
    game_loop does between them: the thinking (the passes of
    computer_assess and computer_plan, which game_loop calls in a tight
    loop), a pass that calls computer_queue or computer_step with the
    passes of computer_input alone that follow it (the script played),
    and the passes that wait for the round's end"""
    segs = []
    for p in half:
        what = p[3]
        last = segs[-1] if segs else None
        if last and what[0] in 'AP' and last[-1][3] in ('A0', 'A1', 'P0'):
            last.append(p)
        elif last and what[0] == 'I' and last[-1][3] != 'I1' and last[-1][3][0] not in 'AP':
            last.append(p)
        elif (last and what == 'S0I1' and p[6] and not p[5]
              and last[-1][3] == what and last[-1][6] and not last[-1][5]):
            last.append(p)
        else:
            segs.append([p])
    return segs


def turn_main(args, until):
    """the kind `turn`: stage 1 logs every call of computer_turn of the
    quiet battles (all, or those named) and what it called; stage 2 dumps
    three single calls of every kind of pass (turn_test.args), and the
    stretches of the halves TURNS names, one behind the other
    (turns_test.args)"""
    names = [n for n in TURNS if n in args] or list(TURNS)
    if 'stage2' not in args:
        def one(name):
            play_quiet(name, TURNS[name][0], until)
            opts = []
            for a in [TURN_IN, TURN_OUT, ROUND_WAIT, ROUND_SEEN] + list(TURN_CALLS) + ['1E4B:' + k for k in KINDS]:
                opts += ['-log', 'S' + a]
            run(name, name + '_turn', until, opts)
        with ThreadPoolExecutor(len(names)) as ex:
            list(ex.map(one, names))
    jobs, keep, single, chain = {}, 'stage2' in args, [], []

    def want(name, first, last, strict):
        # a single call ends at its end, a stretch at the entry of the
        # call behind it (the dump the next stretch begins with)
        base = '%s_turn_%d' % (name, first[1])
        e, x = '%s/%se.ram' % (D, base), '%s/%s_%dx.ram' % (D, base, last[1])
        end = (x, TURN_OUT, last[1], last[0])
        if not strict:
            x = '%s/%s_turn_%de.ram' % (D, name, last[1] + 1)
            end = (x, TURN_IN, last[1] + 1, last[0])
        for ram, a, hit, t in ((e, TURN_IN, first[1], first[0]), end):
            if not (keep and os.path.exists(ram)):
                jobs[ram] = (name, os.path.basename(ram)[:-4], int(t) + 5, ['-break', 'S%s#%d' % (a, hit), '-ram', ram])
        return '%s:%d %d %s ../%s ../%s' % ('turn' if strict else 'turns', last[1] - first[1] + 1,
                                           0 if 'first' in name else 1, first[2], e, x)

    for name in names:
        passes = turn_passes(name)
        halves = turn_halves(passes)
        by = {}
        for p in passes:
            by.setdefault((p[3], p[4]), []).append(p)
        print('%s: %d calls of computer_turn in %d halves; by what was called and the kind of step: %s' % (
            name, len(passes), len(halves), {'%s/%d' % k: len(v) for k, v in sorted(by.items())}))
        for k, of in sorted(by.items()):
            for p in of[:3]:
                single.append(want(name, p, p, True))
        for n, half in enumerate(halves):
            if n not in TURNS[name][1]:
                continue
            segs = turn_segments(half)
            print('%s: half %d, %.0f s to %.0f s, %d calls in %d stretches' % (
                name, n, half[0][0], half[-1][0], len(half), len(segs)))
            for seg in segs:
                if seg[-1] is not passes[-1]:
                    chain.append(want(name, seg[0], seg[-1], False))
    with ThreadPoolExecutor(int(os.environ.get('DUMP_JOBS', '10'))) as ex:
        list(ex.map(lambda j: run(*j), jobs.values()))
    for what, lines in (('turn', single), ('turns', chain)):
        open('%s/%s_test.args' % (D, what), 'w').write('\n'.join(lines) + '\n')
        print('computer_dumps: %d lines in %s/%s_test.args' % (len(lines), D, what))


def late_main(args):
    """the kind `late`: stage 1 plays and logs LATE's battles (all, or
    those named), stage 2 dumps the calls wanted into late_test.args"""
    names = [n for n in LATE if n in args] or list(LATE)
    per = int(args[args.index('-per') + 1]) if '-per' in args else 2
    if 'stage2' not in args:
        jobs = []
        for name in names:
            index, until, wanted = LATE[name]
            play_quiet(name, index, until)
            groups = set(g for g, of in enumerate(PLAN_LOGS) if set(of) & set(wanted))
            groups |= set(g for g, of in enumerate(PLAN_LOGS) for w in wanted if w != 'step'
                          for t in wanted[w] if isinstance(t, tuple) and t[1] in of)
            jobs += [(plan_stage1, name, until, g) for g in sorted(groups)]
            if 'step' in wanted:
                jobs.append((stage1, name, until))
        with ThreadPoolExecutor(int(os.environ.get('DUMP_JOBS', '10'))) as ex:
            list(ex.map(lambda j: j[0](*j[1:]), jobs))
    jobs, keep, lines = {}, 'stage2' in args, []

    def want(name, kind, base, t, ssbp, a_in, a_out):
        for way, a in (('e', a_in), ('x', a_out)):
            ram = '%s/%s%s.ram' % (D, base, way)
            if not (keep and os.path.exists(ram)):
                jobs[ram] = (name, base + way, int(t) + 5, ['-break', a, '-ram', ram])
        lines.append('%s %d %s ../%s/%se.ram ../%s/%sx.ram' % (
            kind, 0 if name.startswith('first') else 1, ssbp, D, base, D, base))

    for name in names:
        index, until, wanted = LATE[name]
        for what, targets in wanted.items():
            if what == 'step':
                _, steps = calls(name)
                for kind, state in targets:
                    of = [c for c in steps if c[5] == kind and c[6] == state]
                    print('%s: %d calls of computer_step of the kind %d in state %s' % (name, len(of), kind, state))
                    for c in of[:per + 1]:
                        want(name, 'step', '%s_step_%s' % (name, c[3].split('#')[1]), c[0], c[2], c[3], c[4])
                continue
            found = plan_calls(name, [what])[what]
            for target in targets:
                mark, since = target, POKED.get(name, ([], 0.0))[1]
                if isinstance(target, tuple):
                    mark, other, place = target
                    since = min([c[0] for c in plan_calls(name, [other])[other] if place in c[5]] or [1e9])
                by = {}
                for c in found:
                    if mark in c[5] and c[0] >= since:
                        by.setdefault((c[4], c[5]), []).append(c)
                print('%s: %s through %s, %d calls in %d sets of places' % (
                    name, what, mark, sum(len(v) for v in by.values()), len(by)))
                chosen = []
                for n in range(per):
                    chosen += [of[n] for k, of in sorted(by.items()) if len(of) > n]
                for t, ssbp, a_in, a_out, result, _ in sorted(set(chosen[:12])):
                    kind = ('plan' if what == 'ride' else what) + ('end' if result else '')
                    want(name, kind, '%s_%s_%s' % (name, what, a_in.split('#')[1]), t, ssbp, a_in, a_out)
    with ThreadPoolExecutor(int(os.environ.get('DUMP_JOBS', '10'))) as ex:
        list(ex.map(lambda j: run(*j), jobs.values()))
    lines = list(dict.fromkeys(lines))
    open('%s/late_test.args' % D, 'w').write('\n'.join(lines) + '\n')
    print('computer_dumps: %d calls in %s/late_test.args' % (len(lines), D))


def spread(items, n):
    step = max(1, len(items) // n)
    return items[step // 2::step][:n]


def main():
    args = sys.argv[1:]
    until = int(args[args.index('-until') + 1]) if '-until' in args else 1500
    early = float(args[args.index('-early') + 1]) if '-early' in args else 150.0
    os.chdir(ROOT)
    os.makedirs(D, exist_ok=True)
    if 'phases' in args:
        return plan_main(args, until)
    if 'turn' in args:
        return turn_main(args, until)
    if 'late' in args:
        return late_main(args)
    if 'stage2' not in args:
        for name, run_ in RUNS.items():
            play(name, run_[0], run_[1], until)
        with ThreadPoolExecutor(len(RUNS)) as ex:
            list(ex.map(lambda n: stage1(n, until), RUNS))
    jobs, lines, keep = [], [], 'stage2' in args

    def want(name, kind, c):
        t, side, ssbp, a_in, a_out = c[:5]
        base = '%s_%s_%s' % (name, kind, a_in.split('#')[1])
        for way, a in (('e', a_in), ('x', a_out)):
            ram = '%s/%s%s.ram' % (D, base, way)
            if not (keep and os.path.exists(ram)):
                jobs.append((name, base + way, int(t) + 5, ['-break', a, '-ram', ram]))
        lines.append('%s %d %s ../%s/%se.ram ../%s/%sx.ram' % (kind, side, ssbp, D, base, D, base))

    for name in RUNS:
        inputs, steps = calls(name)
        first = [c for c in inputs if c[0] <= early]
        chosen = spread(first, INPUT_SPREAD)
        script = []
        for i, c in enumerate(first):
            if c[5] == 0:
                script += [c] + first[i + 1:i + 2]
        for c in spread(list(dict.fromkeys(script)), INPUT_SCRIPT):
            if c not in chosen:
                chosen.append(c)
        for c in chosen if RUNS[name][3] else []:
            want(name, 'input', c)
        count = {}
        for c in steps:
            count.setdefault((c[5], c[6]), []).append(c)
        for (kind, state), of in sorted(count.items(), key=lambda x: (x[0][0], x[0][1] or 0)):
            if kind not in RUNS[name][2]:
                continue
            n = ENDS if kind == 0 else PER_STATE
            soon = [c for c in of if c[0] <= 2 * early]
            for c in (spread(soon, n) if len(soon) >= n else of[:n]):
                want(name, 'stepend' if kind == 0 else 'step', c)
        count = {'%d/%s' % k: len(v) for k, v in sorted(count.items(), key=lambda x: (x[0][0], x[0][1] or 0))}
        print('%s: %d calls of computer_input, of computer_step by kind/state %s' % (name, len(inputs), count))
    with ThreadPoolExecutor(10) as ex:
        list(ex.map(lambda j: run(*j), jobs))
    open('%s/computer_test.args' % D, 'w').write('\n'.join(lines) + '\n')
    print('computer_dumps: %d calls in %s/computer_test.args' % (len(lines), D))


if __name__ == '__main__':
    main()
