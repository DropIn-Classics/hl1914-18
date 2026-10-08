#!/usr/bin/env python3
"""Turns a key script of port/tests/keys/ into DK_KEYS for the port's
program in the window (play_live, port/src/play.c) run headless
(doskit/runtime/plat_null.c): each key is pressed or let go at the
picture shown at its time, as a player's key comes in the window.

    python3 port/tests/live_keys.py KEYS START [FIRST]
    python3 port/tests/live_keys.py --moves MOVES START [FIRST]

START is the first pass's time (-start); FIRST the number plat_null
gives the window's first picture after the setup (default 38, the setup
of -map shows 38).  Pictures come every 0.014268 s from START on.  Keys
before START are left out (the menu's; -map has none).  Prints the
DK_KEYS value.

Which call of the timer's routine a key reaches is not the script's time
everywhere: where the window shows its pictures ahead of the game's
clock (a round's end, a fight's pictures) keys reach it early.  The
window's -keylog writes the calls they reached, a script that -keys
plays the same.
"""
import sys

RETRACE = 0.014268
CODES = {
    'esc': '01', '1': '02', '2': '03', '3': '04', '4': '05', '5': '06', '6': '07', '7': '08',
    '8': '09', '9': '0A', '0': '0B', 'backspace': '0E', 'tab': '0F', 'q': '10', 'w': '11',
    'e': '12', 'r': '13', 't': '14', 'y': '15', 'u': '16', 'i': '17', 'o': '18', 'p': '19',
    'enter': '1C', 'lctrl': '1D', 'a': '1E', 's': '1F', 'd': '20', 'f': '21', 'g': '22',
    'h': '23', 'j': '24', 'k': '25', 'l': '26', 'lshift': '2A', 'z': '2C', 'x': '2D', 'c': '2E',
    'v': '2F', 'b': '30', 'n': '31', 'm': '32', 'rshift': '36', 'lalt': '38', 'space': '39',
    'f1': '3B', 'f2': '3C', 'f3': '3D', 'f4': '3E', 'f5': '3F', 'f6': '40', 'f7': '41',
    'f8': '42', 'f9': '43', 'f10': '44', 'f11': '57', 'f12': '58', 'rctrl': 'E0-1D',
    'ralt': 'E0-38', 'up': 'E0-48', 'down': 'E0-50', 'left': 'E0-4B', 'right': 'E0-4D',
}


def code(name, down):
    c = CODES[name]
    pre, b = ('E0-', c[3:]) if c.startswith('E0-') else ('', c)
    return pre + ('%02X' % (int(b, 16) | (0 if down else 0x80)))


def main():
    if sys.argv[1] == '--moves':
        return moves(sys.argv[2:])
    path, start = sys.argv[1], float(sys.argv[2])
    first = int(sys.argv[3]) if len(sys.argv) > 3 else 38
    ev = []
    for n, line in enumerate(open(path)):
        f = line.split('#')[0].split()
        if len(f) < 2:
            continue
        t, name = float(f[0]), f[1]
        if name[-1] in '+-' and len(name) > 1:
            ev.append((t, n, code(name[:-1], name[-1] == '+')))
        else:
            ev += [(t, n, code(name, True)), (t + 0.15, n, code(name, False))]
    ev.sort()
    print(' '.join('%d:%s' % (first + round((t - start) / RETRACE), c) for t, _, c in ev if t >= start))


def moves(argv):
    """the mouse's motions of a moves file ("T DX,DY,B" a line, the
    window's counts and buttons) as DK_MOUSEMOVE, at the picture shown at
    each time"""
    path, start = argv[0], float(argv[1])
    first = int(argv[2]) if len(argv) > 2 else 38
    out = []
    for line in open(path):
        f = line.split('#')[0].split()
        if len(f) == 2 and float(f[0]) >= start:
            out.append('%d:%s' % (first + round((float(f[0]) - start) / RETRACE), f[1]))
    print(' '.join(out))


if __name__ == '__main__':
    main()
