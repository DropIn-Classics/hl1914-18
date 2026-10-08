#!/usr/bin/env python3
"""The pokes of a key script, for the runner or for the port's program.

    python3 port/tests/play_pokes.py TAG [port]

port/tests/keys/TAG.pokes (optional) changes memory when the battle's
first pass begins, for a state no short script reaches by play: a line
"TARGET HEX", TARGET a name of the hints with +N (hex) or without, HEX
the bytes, # a comment.  Prints the options for doskit's run.py
(-poke S066A:0EE9#1 TARGET HEX each), or with `port` those of the port's
program (-poke LINEAR HEX, the address in the memory image as loaded at
0076h, from port/src/gen/names.h).  Nothing when there is no such file.
No game data is kept in this file.
"""
import os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
tag = sys.argv[1]
port = len(sys.argv) > 2 and sys.argv[2] == 'port'
path = os.path.join(HERE, 'keys', tag + '.pokes')
out = []
if os.path.exists(path):
    seg, name = {}, {}
    for line in open(os.path.join(os.path.dirname(HERE), 'src', 'gen', 'names.h')):
        m = re.match(r'#define HL_(\w+) 0x([0-9A-F]+)u', line)
        if m:
            seg[m.group(1)] = int(m.group(2), 16)
        m = re.match(r'\s*X\((\w+), (\w+), 0x([0-9A-F]+)\)', line)
        if m:
            name[m.group(2)] = ((seg[m.group(1)] + 0x76) << 4) + int(m.group(3), 16)
    for line in open(path):
        f = line.split('#')[0].split()
        if len(f) < 2:
            continue
        if port:
            n, _, o = f[0].partition('+')
            out += ['-poke', '%X' % (name[n] + int(o or '0', 16)), ''.join(f[1:])]
        else:
            out += ['-poke', 'S066A:0EE9#1', f[0], ''.join(f[1:])]
print(' '.join(out))
