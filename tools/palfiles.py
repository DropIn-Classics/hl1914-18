#!/usr/bin/env python3
"""History Line palette analysis (.PAL).

    palfiles.py [FILE ...] [--png DIR]

Without FILE, scan the installed game's folder. Check parsed palette
round trips; --png writes preview swatches into a directory in build/.
All 33 installed palettes passed the initial compatibility probe.
Imported from bisle by explicit user request; see PROVENANCE.md.
The value-range classification and default preview level of 252 are
inherited conventions, unverified against History Line's DAC writes.
"""
import argparse
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, '..', 'doskit', 'tools'))
from kit import game_dir  # noqa: E402
import png  # noqa: E402
import tpwmfiles  # noqa: E402

SIZE = 768


def read(data):
    """(colours, packed, items) of a palette file's bytes: colours the
    256 (r, g, b) triples as stored."""
    packed = data[:4] == tpwmfiles.MAGIC
    parsed = None
    if packed:
        parsed = tpwmfiles.parse(data)
        data = tpwmfiles.expand(parsed[0], parsed[1])
    if len(data) != SIZE:
        raise ValueError('%d bytes, not %d' % (len(data), SIZE))
    return [tuple(data[i:i + 3]) for i in range(0, SIZE, 3)], packed, parsed


def write(colours, parsed=None):
    """The file's bytes: the colours, packed with the parsed items when
    the file was packed."""
    raw = bytes(c for colour in colours for c in colour)
    if parsed is None:
        return raw
    n, items, spare = parsed
    if tpwmfiles.expand(n, items) != raw:
        raise ValueError('the colours changed; pack them anew')
    return tpwmfiles.write(n, items, spare)


def kind(colours):
    return 8 if max(max(c) for c in colours) > 63 else 6


def dac(colours, bits, level=252):
    """preview DAC values using the inherited, runtime-unverified scaling"""
    if bits == 6:
        return colours
    return [tuple(v * level >> 10 for v in c) for c in colours]


def swatch(path, colours, bits):
    rgb = [tuple(png.dac_to_rgb(v) for v in c) for c in dac(colours, bits)]
    pixels = bytes((y // 8) * 16 + x // 8 for y in range(128) for x in range(128))
    png.write_indexed(path, 128, 128, pixels, rgb)


def pal_files(root):
    for d, dirs, files in os.walk(root):
        dirs.sort()
        for f in sorted(files):
            if f.upper().endswith('.PAL'):
                yield os.path.join(d, f)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('files', nargs='*')
    ap.add_argument('--png', help='write each palette as a PNG here')
    a = ap.parse_args()
    root = game_dir()
    files = a.files or list(pal_files(root))
    bad = 0
    for path in files:
        rel = os.path.relpath(path, root) if not a.files else path
        data = open(path, 'rb').read()
        try:
            colours, packed, parsed = read(data)
        except ValueError as e:
            print('%-24s %s' % (rel, e))
            bad += 1
            continue
        same = write(colours, parsed) == data
        bad += not same
        bits = kind(colours)
        values = [v for c in colours for v in c]
        print('%-24s %-6s %3d..%3d  %d-bit  %s' % (rel, 'packed' if packed else 'plain',
              min(values), max(values), bits,
              'written back identical' if same else 'WRITTEN BACK OTHERWISE'))
        if a.png:
            dst = os.path.join(a.png, os.path.splitext(os.path.relpath(path, root))[0] + '.png')
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            swatch(dst, colours, bits)
    print('%d files, %d not read or not written back' % (len(files), bad))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
