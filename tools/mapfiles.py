#!/usr/bin/env python3
"""History Line map-file inspection and identical unpacked round trips.

    mapfiles.py [MAPDIR ...] [--grid NAME] [--png DIR]

Adapted from bisle's structural readers by explicit request; see
PROVENANCE.md. The installed files establish 51 availability slots and
six bytes per computer-table record. No Battle Isle gameplay checks,
sprite placement, building interpretation or runtime renderer is included.
PNG output is a schematic data grid, not the original game's map view.
"""
import argparse
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'doskit', 'tools'))
from kit import game_dir
import png
import tpwmfiles

UNIT_TYPES = 51


def unpacked(path):
    data = open(path, 'rb').read()
    return tpwmfiles.unpack(data) if data[:4] == tpwmfiles.MAGIC else data


def read_fin(data):
    if len(data) < 4:
        raise ValueError('.FIN too short')
    w, h = data[1], data[3]
    if not w or not h or len(data) != 4 + 2 * w * h:
        raise ValueError('.FIN length does not match dimensions')
    return dict(reserved=(data[0], data[2]), w=w, h=h,
                squares=[tuple(data[i:i + 2]) for i in range(4, len(data), 2)])


def write_fin(m):
    if len(m['squares']) != m['w'] * m['h']:
        raise ValueError('square count does not match dimensions')
    return bytes((m['reserved'][0], m['w'], m['reserved'][1], m['h'])) + bytes(
        v for square in m['squares'] for v in square)


def read_shp(data, types=UNIT_TYPES):
    if len(data) < types + 1:
        raise ValueError('.SHP too short')
    count = data[types]
    if len(data) != types + 1 + 12 * count:
        raise ValueError('.SHP length does not match availability slots and record count')
    # Byte offsets are established; their runtime meanings are unverified.
    return dict(availability=list(data[:types]),
                records=[tuple(data[i:i + 12]) for i in range(types + 1, len(data), 12)])


def write_shp(s):
    if any(len(r) != 12 for r in s['records']):
        raise ValueError('.SHP record must have 12 fields')
    return bytes(s['availability']) + bytes((len(s['records']),)) + bytes(
        v for record in s['records'] for v in record)


def read_com(data, types=UNIT_TYPES):
    if len(data) != types * 6:
        raise ValueError('.COM length does not match %d six-byte records' % types)
    return [tuple(data[i:i + 6]) for i in range(0, len(data), 6)]


def write_com(records):
    if any(len(r) != 6 for r in records):
        raise ValueError('.COM record must have six fields')
    return bytes(v for record in records for v in record)


def grid(m):
    return '\n'.join(' '.join('%02X%02X' % m['squares'][y * m['w'] + x]
                              for x in range(m['w'])) for y in range(m['h']))


def schematic(path, m):
    # A neutral indexed data view: first byte fills the cell, second byte
    # occupies its centre. Colours indicate byte values, not terrain types.
    size = 6
    width, height = size * m['w'], size * m['h']
    pixels = bytearray(width * height)
    for y in range(m['h']):
        for x in range(m['w']):
            a, b = m['squares'][y * m['w'] + x]
            for dy in range(size):
                for dx in range(size):
                    pixels[(size * y + dy) * width + size * x + dx] = (
                        b if 2 <= dx < 4 and 2 <= dy < 4 else a)
    colours = [(n, (n * 53) % 256, (n * 97) % 256) for n in range(256)]
    png.write_indexed(path, width, height, pixels, colours)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('dirs', nargs='*', metavar='MAPDIR')
    ap.add_argument('--grid', metavar='NAME')
    ap.add_argument('--png', help='write schematic data grids under this build directory')
    a = ap.parse_args()
    root = game_dir()
    total = bad = files_checked = 0
    for directory in a.dirs or [os.path.join(root, 'MAP')]:
        files = {name.upper(): os.path.join(directory, name) for name in os.listdir(directory)}
        maps = sorted(os.path.splitext(name)[0] for name in files if name.endswith('.FIN'))
        for name in maps:
            total += 1
            try:
                data = unpacked(files[name + '.FIN'])
                m = read_fin(data)
                same = write_fin(m) == data
                files_checked += 1
                detail = ['%dx%d' % (m['w'], m['h'])]
                for ext, read, write in (('.SHP', read_shp, write_shp),
                                         ('.COM', read_com, write_com)):
                    if name + ext in files:
                        data = unpacked(files[name + ext])
                        parsed = read(data)
                        same = (write(parsed) == data) and same
                        files_checked += 1
                        count = len(parsed['records']) if ext == '.SHP' else len(parsed)
                        detail.append('%s %d records' % (ext, count))
                if a.grid and name == a.grid.upper():
                    print(grid(m))
                if a.png:
                    os.makedirs(a.png, exist_ok=True)
                    schematic(os.path.join(a.png, name + '.png'), m)
                bad += not same
                print('%s: %s; %s' % (name, ', '.join(detail),
                      'written back identical' if same else 'WRITTEN BACK OTHERWISE'))
            except (OSError, ValueError, IndexError, struct.error) as e:
                bad += 1
                print('%s: %s' % (name, e))
    if total == 0:
        bad += 1
        print('no maps found')
    print('%d maps, %d files checked, %d not read or not written back' % (total, files_checked, bad))
    return int(bool(bad))


if __name__ == '__main__':
    sys.exit(main())
