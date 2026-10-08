#!/usr/bin/env python3
"""Makes what port/tests/sfx_test.sh compares sfx_play's ways through the
port's sound routines (port/src/sound.c) with: an 8SVX effect, an effect
played more than once or endless, a call that fails.

    python3 port/tests/sfx_dumps.py [VARIANT ...]

The game's own calls with other than "once" and with digital effects
are those of the pictures of events (S1A39).  Of them the port has
pic_wiped (CDh, the other side left without units; port/src/events.c),
which port/tests/keys/m57_win.keys reaches on map 57 (with its pokes):
at the frames of a table at S3812:03C4 (six bytes an entry: the frame,
how many times, the effect's number, the volume; a second table 7Eh
further on when S3303:0022 is set) it calls sfx_play and snd_pass, on
the channels 0 to 6 in turn, or 8 to 0Ah with S3303:0022.  The game's
tables play every effect once, so the other ways are reached by pokes
when the battle's first pass begins (not the game's own doing):

    dig    the game's own: 16 digital effects (S3303:0022 is 0 with the
           runner's Sound Blaster found)
    digx   the same with entries of the table changed: endless (times
           0) twice, three times, twice, and a number beyond the file
    adl    S3303:0022 set to 1: the AdLib's file and table, channels 8
           to 0Ah
    adlx   that with entries of its table changed in the same way
    chan   dig, and the seventh call's channel made 0Ch on the stack at
           sfx_play's entry
    voices adl, and adl_used made 9 at the third call's entry (adl_load
           then has no voice free)
    lock   adl, and adl_used made 9 at the snd_pass behind the third
           call (adl_start then needs more voices than are free)

The last three are for the memory's comparison only; the first four the
port's program plays too (port/tests/sfx_test.sh).

HL.EXE is run on the runner as play_dumps.py runs m57_win.keys.  A first
run logs (build/sfx/VARIANT/log.txt), with their times: the passes
(066A:0EE9), pic_wiped (1A39:1262) and its frames (1A39:1359), the entry
and the end of sfx_play, dig_play, adl_play, adl_load, adl_start,
chan_stop, adl_stop, snd_pass and adl_tick, and in effect_events an
effect's end (30EF:18B3) and its beginning again (30EF:18DA).  From it
build/sfx/VARIANT/calls.txt, a line for each sfx_play of the picture as
the port's -sndlog has them: "FRAME CHANNEL NUMBER VOLUME TIMES RESULT"
(FRAME the picture's frame from 0, as pic_wiped counts), and behind it
what the call reached: "dig R", "adl R load R" and for the snd_pass
behind it "start CHANNEL R" for each adl_start that did not return
FFFFh.  Then memory is kept at the entry and the end of every sfx_play
of the picture, of the snd_pass behind each and of calls of adl_tick
(the first twelve behind each of the first three effects, those in
which an effect ended or began again and the one before each of them),
and in the variants with digital effects of the card's interrupts
(dig_irq; a second log, irq.txt: the first three behind each of the
picture's calls, without those dig_kick makes inside snd_pass), for
port/tests/sound_test.c, whose argument list is build/sfx/VARIANT.args.

build/sfx/VARIANT.pokes has the first pass's pokes as the port's program
takes them (-poke LINEAR HEX), m57_win's own included.

No game data is kept in this file.
"""
import os, re, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
KIT = os.path.join(ROOT, 'doskit', 'tools', 'run.py')
KEYS = 'm57_win'
BASE = 0x76         # the program's segments in memory: the hints' + 76h

TABLE = 0x03C4      # S3812: pic_wiped's table of sounds; +7Eh with S3303:0022
# an entry's change: (entry, "times" or "number", value)
CHANGES = [(1, 'times', 0), (2, 'times', 3), (3, 'number', 0xFF), (5, 'times', 2), (8, 'times', 0)]
# variant: (S3303:0022, the table's changes, a poke at one call of the
# picture: (which call from 1, at its sfx_play or at the snd_pass behind
# it, what))
VARIANTS = {
    'dig': (0, [], None),
    'digx': (0, CHANGES, None),
    'adl': (1, [], None),
    'adlx': (1, CHANGES, None),
    'chan': (0, [], (7, 'play', 'channel')),
    'voices': (1, [], (3, 'play', 'used')),
    'lock': (1, [], (3, 'pass', 'used')),
}
# the variants the port's program can play too (table pokes only)
PLAYED = ('dig', 'digx', 'adl', 'adlx')

LOGS = ('066A:0EE9', '1A39:1262', '1A39:1359', '2F39:06E3', '2F39:0965', '2F39:0177', '2F39:068B',
        '2F39:0690', '2F39:06DF', '2F39:0AA8', '2F39:0CF6', '2F39:0CFB', '2F39:0F27', '2F39:09C9',
        '2F39:0A6C', '2F39:1754', '2F39:1809', '2F39:096A', '2F39:09C5', '30EF:192D', '30EF:1A2A',
        '30EF:18B3', '30EF:18DA')
# kind: (entry, end) for sound_test.c
CALLS = {
    'play': ('2F39:06E3', '2F39:0965'),
    'pass': ('2F39:096A', '2F39:09C5'),
    'tick': ('30EF:192D', '30EF:1A2A'),
    'irq': ('30EF:1DF8', '30EF:1E70'),
}
# a second log for the variants with digital effects: the card's
# interrupts (dig_irq and its IRET behind the work) among the passes
IRQ_LOGS = ('1A39:1262', '2F39:06E3', '2F39:096A', '2F39:09C5', '30EF:1DF8', '30EF:1E70')


def irqs_of(rows):
    """the card's interrupts worth a dump, in the picture and outside
    snd_pass (dig_kick calls dig_irq itself: the pass has those): the
    first three behind each of the picture's calls: (entry's hit, end's
    hit), and both addresses' registers by hit"""
    want, by = [], {}
    inside = in_pass = False
    left, entry = 0, None
    for at, t, hit, regs in rows:
        by[(at, hit)] = regs
        if at == '1A39:1262':
            inside = True
        elif not inside:
            continue
        elif at == '2F39:06E3':
            left = 3
        elif at == '2F39:096A':
            in_pass = True
        elif at == '2F39:09C5':
            in_pass = False
        elif at == '30EF:1DF8':
            entry = None if in_pass or not left else hit
        elif at == '30EF:1E70' and entry is not None:
            want.append((entry, hit))
            left -= 1
            entry = None
    return want, by


def pokes(variant):
    """the variant's pokes: (segment of the hints, offset, bytes)"""
    adl, changes, _ = VARIANTS[variant]
    out = []
    if adl:
        out.append((0x3303, 0x0022, bytes([1, 0])))
    for k, what, v in changes:
        at = TABLE + adl * 0x7E + k * 6
        out.append((0x3812, at + 2, v.to_bytes(2, 'little')) if what == 'times' else (0x3812, at + 4, bytes([v])))
    return out


def prefix(variant):
    # as play_dumps.py starts a battle of a map by its code
    sys.path.insert(0, os.path.join(ROOT, 'tools'))
    import tpwmfiles
    game = os.environ.get('DOSKIT_GAME') or os.path.join(ROOT, 'game')
    rec = tpwmfiles.unpack(open(os.path.join(game, 'CODES.DAT'), 'rb').read())[int(KEYS[1:3]) * 10:][:5]
    pre = ['-mouse', '21', '150,73,1', '-mouse', '21.2', '150,73,0']
    for i, b in enumerate(rec):
        pre += ['-key', str(22 + i * 0.5), chr(b - 0x11 + ord('a'))]
    pre += ['-key', '25', 'enter', '-mouse', '27', '150,48,1', '-mouse', '27.2', '150,48,0']
    pre += ['-keys', os.path.join(HERE, 'keys', KEYS + '.keys'), '-until', os.environ.get('UNTIL', '84')]
    pre += subprocess.run(['python3', os.path.join(HERE, 'play_pokes.py'), KEYS],
                          capture_output=True, text=True).stdout.split()
    for seg, off, data in pokes(variant):
        pre += ['-poke', 'S066A:0EE9#1', 'S%04X:%04X' % (seg, off), data.hex()]
    return pre


def port_pokes(variant):
    own = subprocess.run(['python3', os.path.join(HERE, 'play_pokes.py'), KEYS, 'port'],
                         capture_output=True, text=True).stdout.split()
    for seg, off, data in pokes(variant):
        own += ['-poke', '%X' % (((seg + BASE) << 4) + off), data.hex()]
    return own


def whole(path):
    return os.path.exists(path) and os.path.getsize(path) == 0xA0000


def run(out, args):
    r = subprocess.run(['python3', KIT] + args + ['HL.EXE'], capture_output=True, text=True, cwd=out)
    return r.stdout + r.stderr


def read(log):
    """the log's lines: (address, time, hit, registers)"""
    rows = []
    for m in re.finditer(r'log HL\.EXE\+(\w{4}:\w{4}) t=([\d.]+) hit=(\d+) regs (.*)', log):
        regs = dict(re.findall(r'([A-Z]+)=([0-9A-F]+)', m.group(4)))
        rows.append((m.group(1), float(m.group(2)), int(m.group(3)), regs))
    return rows


def logged(out, pre, name):
    path = os.path.join(out, name)
    if not os.path.exists(path):
        a = []
        for l in LOGS:
            a += ['-log', 'S' + l]
        open(path, 'w').write(run(out, pre + a))
    return read(open(path).read())


def signed(regs):
    ax = int(regs['AX'], 16)
    return ax - 0x10000 if ax & 0x8000 else ax


def calls_of(rows):
    """pic_wiped's calls of sfx_play in order: a dict each; and the calls
    worth a dump: (kind, hit)"""
    calls, want = [], []
    frame, inside, cur, in_pass, in_tick = -1, False, None, False, None
    ticks_after, prev_tick = 0, None
    for at, t, hit, regs in rows:
        if at == '1A39:1262':
            inside = True
        elif at == '1A39:1359':
            frame += 1
        elif not inside:
            continue
        elif at == '2F39:06E3':
            cur = {'frame': frame, 'hit': hit, 'regs': regs, 'reached': [], 't': t}
            calls.append(cur)
            want.append(('play', hit))
            if len(calls) <= 3:
                ticks_after = 12
        elif at == '2F39:0965' and cur is not None:
            cur['r'] = signed(regs)
        elif at == '2F39:068B' and cur is not None and 'r' not in cur:
            cur['reached'].append('dig %d' % signed(regs))
        elif at == '2F39:0CF6' and cur is not None and 'r' not in cur:
            cur['load'] = signed(regs)
        elif at == '2F39:06DF' and cur is not None and 'r' not in cur:
            cur['reached'].append('adl %d load %d' % (signed(regs), cur.get('load', 0)))
        elif at == '2F39:096A':
            in_pass = True
            if cur is not None and 'r' in cur and 'pass' not in cur:
                cur['pass'] = hit
                want.append(('pass', hit))
                in_pass = cur
        elif at == '2F39:0CFB' and isinstance(in_pass, dict):
            in_pass['start_ch'] = int(regs['SI'], 16)      # snd_pass's loop variable
        elif at == '2F39:0F27' and isinstance(in_pass, dict) and signed(regs) != -1:
            in_pass['reached'].append('start %s %d' % (in_pass.get('start_ch', '?'), signed(regs)))
        elif at == '2F39:09C5':
            in_pass = False
        elif at == '30EF:192D':
            in_tick = hit
            if ticks_after:
                ticks_after -= 1
                want.append(('tick', hit))
        elif at in ('30EF:18B3', '30EF:18DA') and in_tick:
            want.append(('tick', in_tick))
            if prev_tick:
                want.append(('tick', prev_tick))
            if cur is not None:
                calls[-1].setdefault('events', []).append('again' if at == '30EF:18DA' else 'end')
        elif at == '30EF:1A2A':
            prev_tick, in_tick = in_tick, None
    seen, uniq = set(), []
    for w in want:
        if w not in seen:
            seen.add(w)
            uniq.append(w)
    return calls, uniq


def stack_args(path, regs, n=5):
    with open(path, 'rb') as f:
        f.seek(int(regs['SS'], 16) * 16 + int(regs['SP'], 16) + 4)
        b = f.read(2 * n)
    return [int.from_bytes(b[i:i + 2], 'little') for i in range(0, 2 * n, 2)]


def main():
    for variant in sys.argv[1:] or list(VARIANTS):
        out = os.path.join(ROOT, 'build', 'sfx', variant)
        os.makedirs(out, exist_ok=True)
        pre = prefix(variant)
        late = VARIANTS[variant][2]
        if late:
            # a poke at one call needs its number among all calls of the
            # routine, and the stack's poke the place of its arguments: a
            # run without it first (the same up to that call)
            n, where, what = late
            calls, _ = calls_of(logged(out, pre, 'log0.txt'))
            c = calls[n - 1]
            at = 'S2F39:06E3#%d' % c['hit'] if where == 'play' else 'S2F39:096A#%d' % c['pass']
            if what == 'channel':
                lin = int(c['regs']['SS'], 16) * 16 + int(c['regs']['SP'], 16) + 4
                pre += ['-poke', at, '%X' % lin, '0C00']
            else:
                pre += ['-poke', at, 'adl_used', '0900']
        rows = logged(out, pre, 'log.txt')
        with open(os.path.join(ROOT, 'build', 'sfx', variant + '.pokes'), 'w') as f:
            f.write(' '.join(port_pokes(variant)) + '\n')
        if os.environ.get('LOG_ONLY'):
            continue
        calls, want = calls_of(rows)
        by = {}
        for at, t, hit, regs in rows:
            by[(at, hit)] = regs
        jobs, lines = [], []
        for kind, n in want:
            a, b = CALLS[kind]
            if (b, n) not in by:
                continue        # the run ended inside the call
            regs, end = by[(a, n)], by[(b, n)]
            e = os.path.join(out, '%s%d_in.ram' % (kind, n))
            x = os.path.join(out, '%s%d_out.ram' % (kind, n))
            if not whole(e):
                jobs.append(pre + ['-break', 'S%s#%d' % (a, n), '-ram', e])
            if not whole(x):
                jobs.append(pre + ['-break', 'S%s#%d' % (b, n), '-ram', x])
            lines.append('%s %s:%s %s %s %s' % (kind, regs['SS'], regs['SP'], end['AX'], e, x))
        with ThreadPoolExecutor(int(os.environ.get('JOBS', '8'))) as pool:
            list(pool.map(lambda a: run(out, a), jobs))
        if not VARIANTS[variant][0]:
            path = os.path.join(out, 'irq.txt')
            if not os.path.exists(path):
                a = []
                for l in IRQ_LOGS:
                    a += ['-log', 'S' + l]
                open(path, 'w').write(run(out, pre + a))
            pairs, iby = irqs_of(read(open(path).read()))
            jobs = []
            for n, m in pairs:
                a, b = CALLS['irq']
                regs = iby[(a, n)]
                e = os.path.join(out, 'irq%d_in.ram' % n)
                x = os.path.join(out, 'irq%d_out.ram' % n)
                if not whole(e):
                    jobs.append(pre + ['-break', 'S%s#%d' % (a, n), '-ram', e])
                if not whole(x):
                    jobs.append(pre + ['-break', 'S%s#%d' % (b, m), '-ram', x])
                # the interrupt's frame: IP, CS and the flags are on the stack, and it returns nothing
                lines.append('irq %s:%s 0 %s %s' % (regs['SS'], regs['SP'], e, x))
            with ThreadPoolExecutor(int(os.environ.get('JOBS', '8'))) as pool:
                list(pool.map(lambda a: run(out, a), jobs))
        with open(os.path.join(ROOT, 'build', 'sfx', variant + '.args'), 'w') as f:
            f.write('\n'.join(lines) + ('\n' if lines else ''))
        with open(os.path.join(out, 'calls.txt'), 'w') as f:
            for c in calls:
                e = os.path.join(out, 'play%d_in.ram' % c['hit'])
                a = stack_args(e, c['regs']) if whole(e) else ['-'] * 5
                f.write('%d %s %s %s %s %s  %s\n' % (c['frame'], a[0], a[1], a[2], a[3], c.get('r', '-'),
                                                    ', '.join(c['reached'] + c.get('events', []))))
        print('%s: %d calls of sfx_play in the picture, %d calls kept (%s)'
              % (variant, len(calls), len(lines),
                 ', '.join('%d %s' % (sum(1 for l in lines if l.startswith(k)), k) for k in CALLS)))


main()
