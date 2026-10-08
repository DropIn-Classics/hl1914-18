#!/usr/bin/env python3
"""Makes what port/tests/opl_test.sh compares the port's AdLib output
(port/src/sound.c: what goes to snd_opl) with: the original's stream of
register writes and the calls and timer interrupts that made it.

    python3 port/tests/opl_dumps.py [-keep] [menu] [brief] [battle]

HL.EXE is run on the runner once with a log (build/opl/WALK/log.txt) of

    every OUT to the ports 388h and 389h (four places in the driver:
        opl_write, voice_off, and voice_note's two), AL at each: the
        register, then its value
    the timer's interrupt (pit_irq) and in it the fade's routine
        fade_tick and adl_tick, with the place adl_tick only reaches when
        the lock is clear
    the entry and the end of the routines the game calls: snd_init,
        snd_end, sfx_play, dig_play, snd_pass, chan_stop, music_start,
        music_fade (music_volume too; it is only called from inside)
    where adl_start reads the count of ticks for an effect's or a tune's
        first event
    where pit_add has made its routine the timer's last and where
        music_start copies the count of ticks (on which side of them an
        interrupt came)
    sfx_load (it sets where the effects' file is, which the port's test
        then takes from the next dump) and the Sound Blaster's interrupt
        dig_irq where it is not dig_kick's own call of it

    menu     the menu walked by the keys of menu_dumps.py's walk `pages`
             (build/menu/pages/keys.txt: run menu_dumps.py pages first)
    brief    the new units' screens of map 4 (build/briefing/m4/keys.txt:
             run briefing_dumps.py m4 first) and the battle's beginning
    battle   map 8 with port/tests/keys/m8_site.keys as play_dumps.py
             runs it, its first 180 seconds

Then one run for each call that has arguments, stopped at its entry with
the memory kept (the arguments are on the stack there, and the driver's
variables are compared with the port's there), and one for every 4000th
tick outside a call and for the last one.  With -keep the dumps that are
there already are not made again (the run is the same each time; for a
change of this script that only changes the list).

Written: build/opl/WALK/*.ram and build/opl/WALK.ev, the list
port/tests/opl_test.c replays, a line an event in the original's order:

    init|play|dig|stop|mstart|mfade SS:SP FILE   a call begins (FILE its
                             entry's memory)
    pass | end               snd_pass, snd_end begins
    ret AX                   the call ends
    w RR VV                  register RR gets VV
    irq                      a timer interrupt begins
    ftick | tick | lock      fade_tick, adl_tick, adl_tick with the lock
                             set (it only counts)
    iend                     the interrupt's routines are done
    due NNNN                 adl_start has read the count of ticks
    sfxload                  sfx_load was called
    at 2 | at 3              the game's side is where pit_add has made a
                             routine the timer's last (fade_start's; the
                             start's two are in the list too), at
                             music_start's copy of the count of ticks
    dirq                     the Sound Blaster's interrupt (dig_irq)
    check FILE               the memory before this tick

and build/opl/WALK.times, how far the writes and the ticks are apart in
the runner (for docs/HANDOFF.md's notes on the audio's clock).

No game data is kept in this file.
"""
import os, re, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
KIT = os.path.join(ROOT, 'doskit', 'tools', 'run.py')

# the OUT to 388h and the one to 389h of each place that writes a register
OUTS = [('30EF:11ED', '30EF:11FA'), ('30EF:14DB', '30EF:14EA'),
        ('30EF:1527', '30EF:1534'), ('30EF:1549', '30EF:1556')]
PIT = '27FE:06D2'
TICK, TICK_FREE, TICK_END = '30EF:192D', '30EF:1948', '30EF:1A2A'
FTICK, FTICK_END = '30EF:12D9', '30EF:130E'
DUE = '2F39:0F02'
SFX_LOAD = '2F39:00FD'
AT = {'27FE:07E3': 2, '2F39:1036': 3}
DIG_IRQ, DIG_KICK, DIG_KICK_END = '30EF:1DF8', '30EF:1E8B', '30EF:1EE8'
# kind: (entry, end); those with arguments get an entry dump
CALLS = {
    'init': ('2F39:000B', '2F39:008D'),
    'end': ('2F39:0092', '2F39:00FA'),
    'play': ('2F39:06E3', '2F39:0965'),
    'dig': ('2F39:0177', '2F39:068B'),
    'pass': ('2F39:096A', '2F39:09C5'),
    'stop': ('2F39:09C9', '2F39:0A6C'),
    'mstart': ('2F39:0F87', '2F39:1053'),
    'mfade': ('2F39:1058', '2F39:1122'),
    'mvol': ('2F39:1127', '2F39:11A7'),
}
NO_ARGS = ('pass', 'end')
EVERY = 4000


def keys_pre(path, hint, until):
    if not os.path.exists(path):
        raise SystemExit('no %s: python3 port/tests/%s' % (path, hint))
    pre = ['-key', '3', 'enter', '-key', '6', 'enter']
    for line in open(path):
        t, k = line.split()
        pre += ['-key', t, k]
    return pre + ['-until', str(until)]


def battle_pre():
    # as play_dumps.py starts a battle on a map without its own code
    pre = ['-mouse', '21', '150,73,1', '-mouse', '21.2', '150,73,0', '-key', '22', 'r',
           '-key', '22.5', 'a', '-key', '23', 't', '-key', '23.5', 'i', '-key', '24', 'o',
           '-key', '25', 'enter', '-mouse', '27', '150,48,1', '-mouse', '27.2', '150,48,0']
    pre += ['-keys', os.path.join(HERE, 'keys', 'm8_site.keys'), '-until', '180']
    pre += subprocess.run(['python3', os.path.join(HERE, 'play_pokes.py'), 'm8_site'],
                          capture_output=True, text=True).stdout.split()
    return pre


WALKS = {
    'menu': lambda: keys_pre(os.path.join(ROOT, 'build', 'menu', 'pages', 'keys.txt'), 'menu_dumps.py pages', 66),
    'brief': lambda: keys_pre(os.path.join(ROOT, 'build', 'briefing', 'm4', 'keys.txt'), 'briefing_dumps.py m4', 60),
    'battle': battle_pre,
}


def run(out, args):
    r = subprocess.run(['python3', KIT] + args + ['HL.EXE'], capture_output=True, text=True, cwd=out)
    return r.stdout + r.stderr


def read(log):
    """the log's lines in order: (address, time, hit, registers)"""
    rows = []
    for m in re.finditer(r'log HL\.EXE\+(\w{4}:\w{4}) t=([\d.]+) hit=(\d+) regs (.*)', log):
        rows.append((m.group(1), float(m.group(2)), int(m.group(3)),
                     dict(f.split('=') for f in m.group(4).split() if '=' in f)))
    return rows


def events(rows, out):
    """the list for opl_test.c, the dumps to make as (address, hit, file),
    and the times"""
    entry = {a: k for k, (a, b) in CALLS.items()}
    end = {b: k for k, (a, b) in CALLS.items()}
    reg_at = {a: 1 for a, b in OUTS}
    val_at = {b: 1 for a, b in OUTS}
    ev, dumps = [], []
    stack = []              # the calls open, the first the game's own
    irq = False             # the timer's interrupt's routines are running
    inside = None           # 'tick' or 'ftick' while one of the two runs
    reg = {False: None, True: None}     # the register chosen, outside and inside the interrupt
    latched = None                      # what port 388h got last
    clobbered = 0
    kick = False
    ticks = last_check = 0
    free = False
    gaps = {'reg to value': [], 'value to next register': [], 'tick to tick': []}
    last_val = last_tick = None

    def close():
        nonlocal irq
        if irq and not inside:
            ev.append('iend')
            irq = False

    for a, t, hit, r in rows:
        if a == PIT:
            close()
            ev.append('irq')
            irq = True
            continue
        if a == SFX_LOAD:
            close()
            ev.append('sfxload')
            continue
        if a == DIG_KICK:
            kick = True
            continue
        if a == DIG_KICK_END:
            kick = False
            continue
        if a == DIG_IRQ:
            if not kick:
                if not inside:
                    close()
                ev.append('dirq')
            continue
        if a in reg_at:
            if not inside:
                close()
            latched = reg[bool(inside)] = int(r['AX'], 16) & 0xFF
            if last_val is not None:
                gaps['value to next register'].append(t - last_val[0])
            last_val = None
            reg_t = t
            continue
        if a in val_at:
            # an interrupt between a register's number and its value comes
            # before the write; had it written registers itself, the value
            # went to the one it chose last
            if not inside:
                close()
            if latched != reg[bool(inside)]:
                clobbered += 1
            ev.append('w %02X %02X' % (latched, int(r['AX'], 16) & 0xFF))
            gaps['reg to value'].append(t - reg_t)
            last_val = (t,)
            continue
        if a == FTICK:
            inside = 'ftick'
            ev.append('ftick')
            continue
        if a == FTICK_END:
            inside = None
            continue
        if a == TICK:
            ticks += 1
            if last_tick is not None:
                gaps['tick to tick'].append(t - last_tick)
            last_tick = t
            inside = 'tick'
            free = False
            at = len(ev)
            ev.append('lock')
            # a dump before a tick the game's own code is not inside of
            # and whose interrupt had no fade_tick before it
            if not stack and ev[at - 1] == 'irq' and ticks - last_check >= EVERY:
                f = os.path.join(out, 'tick%d.ram' % hit)
                dumps.append((TICK, hit, f))
                ev.insert(at, 'check ' + f)
                last_check = ticks
            continue
        if a == TICK_FREE:
            ev[-1] = 'tick'
            continue
        if a == TICK_END:
            inside = None
            continue
        if inside:
            continue        # music_volume from fade_tick
        if a in AT:
            if stack:
                close()
                ev.append('at %d' % AT[a])
            continue
        if a == DUE:
            close()
            ev.append('due %s' % r['CX'])
            continue
        if a in entry:
            close()
            k = entry[a]
            if not stack:
                if k in NO_ARGS:
                    ev.append(k)
                else:
                    f = os.path.join(out, '%s%d.ram' % (k, hit))
                    dumps.append((a, hit, f))
                    ev.append('%s %s:%s %s' % (k, r['SS'], r['SP'], f))
            stack.append(k)
            continue
        if a in end:
            if not stack or stack[-1] != end[a]:
                raise SystemExit('the log has the end of %s at t=%f without its entry' % (end[a], t))
            stack.pop()
            if not stack:
                close()
                ev.append('ret %s' % r['AX'])
            continue
    close()
    # cut behind the last whole interrupt outside a call, with a dump there
    cut = None
    depth = 0
    for i, e in enumerate(ev):
        w = e.split()[0]
        if w in CALLS:
            depth += 1
        elif w == 'ret':
            depth -= 1
        elif w == 'irq' and not depth and i + 1 < len(ev) and ev[i + 1] in ('tick', 'lock'):
            cut = i
    n = sum(1 for e in ev[:cut] if e in ('tick', 'lock'))
    f = os.path.join(out, 'last.ram')
    dumps.append((TICK, n + 1, f))
    ev = ev[:cut] + ['check ' + f]
    return ev, dumps, gaps, clobbered


def main():
    keep = '-keep' in sys.argv
    for walk in [a for a in sys.argv[1:] if a != '-keep'] or ['menu', 'brief']:
        out = os.path.join(ROOT, 'build', 'opl', walk)
        os.makedirs(out, exist_ok=True)
        pre = WALKS[walk]()
        logs = []
        for a in [x for p in OUTS for x in p] + [PIT, TICK, TICK_FREE, TICK_END, FTICK, FTICK_END, DUE] \
                + [SFX_LOAD, DIG_IRQ, DIG_KICK, DIG_KICK_END] + list(AT) + [x for p in CALLS.values() for x in p]:
            logs += ['-log', 'S' + a]
        log = run(out, pre + logs)
        open(os.path.join(out, 'log.txt'), 'w').write(log)
        rows = read(log)
        ev, dumps, gaps, clobbered = events(rows, out)
        todo = [d for d in dumps if not (keep and os.path.exists(d[2]))]
        with ThreadPoolExecutor(8) as pool:
            list(pool.map(lambda d: run(out, pre + ['-break', 'S%s#%d' % (d[0], d[1]), '-ram', d[2]]), todo))
        missing = [d[2] for d in dumps if not os.path.exists(d[2])]
        if missing:
            raise SystemExit('%s: %d dumps were not written, the first %s' % (walk, len(missing), missing[0]))
        with open(os.path.join(ROOT, 'build', 'opl', walk + '.ev'), 'w') as f:
            f.write('\n'.join(ev) + '\n')
        with open(os.path.join(ROOT, 'build', 'opl', walk + '.times'), 'w') as f:
            for k, v in gaps.items():
                if v:
                    v = sorted(v)
                    f.write('%s: %d, least %.6f s, median %.6f s, most %.6f s\n'
                            % (k, len(v), v[0], v[len(v) // 2], v[-1]))
        count = lambda w: sum(1 for e in ev if e.split()[0] == w)
        print('%s: %d writes, %d ticks (%d with the lock set), %d of the fade, %d calls (%s), %d dumps' % (
            walk, count('w'), count('tick') + count('lock'), count('lock'), count('ftick'), count('ret'),
            ', '.join('%d %s' % (count(k), k) for k in CALLS if count(k)), len(dumps)))
        print('%s: %d values went to another register than their routine chose (an interrupt in between wrote)'
              % (walk, clobbered))


main()
