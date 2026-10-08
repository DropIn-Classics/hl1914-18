#!/usr/bin/env python3
"""Makes what port/tests/fsound_test.sh compares the fight's sound
triggers of the port (port/src/sound.c: pics_play and what calls it, the
channels stopped at the pictures' end, attacks_run's effect) with.

    python3 port/tests/fsound_dumps.py [TAG ...]

TAG is a key script of port/tests/keys/ for map 8 (default m8_site and
m8_d5r), with _nopics behind it the same script with the fights not
shown as pictures (no_fight_pics set at the first pass, as
play_dumps.py's NOPICS=1 does).  HL.EXE is run on the runner as
play_dumps.py runs it.  A first run logs, with their times:

    066A:0EE9  a pass begins
    2532:0CAA  fight_frames
    2532:0D1D  a frame of it begins
    2532:211E  pics_play, 2532:2140 it has its slot (SI) and volume (DI),
               2532:21AE its end
    2532:14B8  a side without figures is met in a frame (AX the side);
               the chan_stop that follows at once is that site's
    2532:0187, 01A8  the channels stopped at the pictures' end
    2F39:09C9, 0A6C  chan_stop and its end
    2F39:0177, 068B  dig_play and its end
    2F39:0690  adl_play
    2F39:096A, 09C5  snd_pass and its end
    2F39:06E3, 0965  sfx_play and its end
    02E6:0B2B  attacks_run: a unit was destroyed in a fight without
               pictures (its effect follows with effects on)

and from it build/fsound/TAG/calls.txt, the triggers in order as the
port's -fxlog writes them: "fight N FRAMES" (the N-th fight with
pictures and how many frames it had: a key may end them), "play FRAME
SLOT VOLUME TIMES", "side FRAME SIDE" (the channel stopped for a side
without figures), "end CHANNEL" (the pictures' end) and "blast"
(attacks_run's effect); FRAME counts fight_frames' frames from 1 in each
fight.  Then memory is kept at the entry and the end of a choice of
them: pics_play (the first six of each fight, every tenth and every one
that goes on to dig_play), the side's chan_stop with the snd_pass behind
it, the first and the last chan_stop of each pictures' end and
attacks_run's sfx_play with its snd_pass, for port/tests/sound_test.c,
whose argument list is build/fsound/TAG.args.  TIMES is read from the
stack of the entry's dump where one was kept, else it is "-".

No game data is kept in this file.
"""
import os, re, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
KIT = os.path.join(ROOT, 'doskit', 'tools', 'run.py')

LOGS = ('066A:0EE9', '2532:0CAA', '2532:0D1D', '2532:211E', '2532:2140', '2532:21AE', '2532:14B8',
        '2532:0187', '2532:01A8', '2F39:09C9', '2F39:0A6C', '2F39:0177', '2F39:068B', '2F39:0690',
        '2F39:096A', '2F39:09C5', '2F39:06E3', '2F39:0965', '02E6:0B2B')
# kind: (entry, end) for sound_test.c
CALLS = {
    'pics': ('2532:211E', '2532:21AE'),
    'stop': ('2F39:09C9', '2F39:0A6C'),
    'pass': ('2F39:096A', '2F39:09C5'),
    'play': ('2F39:06E3', '2F39:0965'),
}


def prefix(tag):
    keys = tag[:-7] if tag.endswith('_nopics') else tag
    # as play_dumps.py starts a battle on map 8
    pre = ['-mouse', '21', '150,73,1', '-mouse', '21.2', '150,73,0', '-key', '22', 'r',
           '-key', '22.5', 'a', '-key', '23', 't', '-key', '23.5', 'i', '-key', '24', 'o',
           '-key', '25', 'enter', '-mouse', '27', '150,48,1', '-mouse', '27.2', '150,48,0']
    pre += ['-keys', os.path.join(HERE, 'keys', keys + '.keys'), '-until', os.environ.get('UNTIL', '400')]
    pre += subprocess.run(['python3', os.path.join(HERE, 'play_pokes.py'), keys],
                          capture_output=True, text=True).stdout.split()
    if keys != tag:
        pre += ['-poke', 'S066A:0EE9#1', 'S37FF:0115', '01']
    return pre


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


def triggers(rows):
    """the triggers in order, and the calls worth a dump: (kind, hit)"""
    calls, want = [], []
    frame = fight = per_fight = ended = 0
    for i, (at, t, hit, regs) in enumerate(rows):
        nxt = rows[i + 1] if i + 1 < len(rows) else None
        if at == '2532:0CAA':
            frame, fight, per_fight, ended = 0, fight + 1, 0, 0
            calls.append(['fight', fight, 0])
        elif at == '2532:0D1D':
            frame += 1
            [c for c in calls if c[0] == 'fight'][-1][2] = frame
        elif at == '2532:2140':
            calls[-1] += [frame, int(regs['SI'], 16), int(regs['DI'], 16)]
        elif at == '2532:211E':
            per_fight += 1
            calls.append(['play', hit])
            digs = [r for r in rows[i + 1:i + 4] if r[0] in ('2F39:0177', '2532:21AE')]
            if per_fight <= 6 or per_fight % 10 == 0 or (digs and digs[0][0] == '2F39:0177'):
                want.append(('pics', hit))
        elif at == '2532:14B8' and nxt and nxt[0] == '2F39:09C9':
            calls.append(['side', frame, int(regs['AX'], 16)])
            want.append(('stop', nxt[2]))
            later = [r for r in rows[i + 2:i + 6] if r[0] == '2F39:096A']
            if later:
                want.append(('pass', later[0][2]))
        elif at in ('2532:0187', '2532:01A8') and nxt and nxt[0] == '2F39:09C9':
            calls.append(['end', ended + (8 if at == '2532:01A8' else 0)])
            if ended in (0, 7):
                want.append(('stop', nxt[2]))
            ended += 1
        elif at == '02E6:0B2B':
            calls.append(['blast' if nxt and nxt[0] == '2F39:06E3' else 'blast, effects off'])
            if nxt and nxt[0] == '2F39:06E3':
                want.append(('play', nxt[2]))
                later = [r for r in rows[i + 2:i + 6] if r[0] == '2F39:096A']
                if later:
                    want.append(('pass', later[0][2]))
    return calls, want


def main():
    for tag in sys.argv[1:] or ['m8_site', 'm8_d5r']:
        out = os.path.join(ROOT, 'build', 'fsound', tag)
        os.makedirs(out, exist_ok=True)
        pre = prefix(tag)
        path = os.path.join(out, 'log.txt')
        if not os.path.exists(path):
            a = []
            for l in LOGS:
                a += ['-log', 'S' + l]
            open(path, 'w').write(run(out, pre + a))
        if os.environ.get('LOG_ONLY'):
            continue
        rows = read(open(path).read())
        calls, want = triggers(rows)
        first = [t for at, t, _, _ in rows if at == '066A:0EE9']
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
        with ThreadPoolExecutor(8) as pool:
            list(pool.map(lambda a: run(out, a), jobs))
        with open(os.path.join(ROOT, 'build', 'fsound', tag + '.args'), 'w') as f:
            f.write('\n'.join(lines) + ('\n' if lines else ''))
        # pics_play's third argument from the entry's stack
        for c in calls:
            if c[0] != 'play':
                continue
            e, regs = os.path.join(out, 'pics%d_in.ram' % c[1]), by[('2532:211E', c[1])]
            times = '-'
            if whole(e):
                with open(e, 'rb') as f:
                    f.seek(int(regs['SS'], 16) * 16 + int(regs['SP'], 16) + 8)
                    times = int.from_bytes(f.read(2), 'little')
            c[1:] = c[2:] + [times]
        with open(os.path.join(out, 'calls.txt'), 'w') as f:
            for c in calls:
                f.write(' '.join(str(v) for v in c) + '\n')
        print('%s: the first pass at %s s, %d triggers in %d fights with pictures, %d calls kept'
              % (tag, first[0] if first else '-', sum(1 for c in calls if c[0] != 'fight'),
                 sum(1 for c in calls if c[0] == 'fight'), len(lines)))


main()
