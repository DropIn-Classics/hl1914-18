#!/usr/bin/env python3
"""History Line table and library-backed DAT file inspection.

    datfiles.py [FILE ...] [--list]

Adapted offline analysis, see PROVENANCE.md. Structural layouts and library
cross-checks come from this game's files; gameplay meanings are unverified.
Unclassified DAT families are reported, not accepted as opaque successes.
"""
import argparse
import os
from pathlib import Path
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'doskit', 'tools'))
from kit import game_dir
import libfiles
import tpwmfiles


def unpacked(path):
    data = Path(path).read_bytes()
    return tpwmfiles.unpack(data) if data[:4] == tpwmfiles.MAGIC else data


def read_records(data, size, fields):
    # Structural record helper adapted from bisle; field names below refer
    # to byte positions, not inherited gameplay interpretations.
    if not data or len(data) % size:
        raise ValueError('%d bytes, not records of %d' % (len(data), size))
    return [{name: struct.unpack_from(fmt, data, at + off)[0] for name, off, fmt in fields}
            for at in range(0, len(data), size)]


def write_records(records, size, fields):
    out = bytearray(size * len(records))
    for n, record in enumerate(records):
        for name, off, fmt in fields:
            struct.pack_into(fmt, out, size * n + off, record[name])
    return bytes(out)


UNIT_FIELDS = tuple(('b%02X' % off, off, 'B') for off in range(27)) + (
    ('text1', 27, '17s'), ('text2', 44, '17s')) + tuple(
    ('b%02X' % off, off, 'B') for off in range(61, 69))
GROUND_FIELDS = (('word00', 0, '<H'),) + tuple(('b%02X' % off, off, 'B') for off in range(2, 6))
CODE_FIELDS = (('encoded_text', 0, '5s'),) + tuple(('b%02X' % off, off, 'B') for off in range(5, 10))
AMOK_FIELDS = tuple(('b%02X' % off, off, 'B') for off in range(36))


def read_units(data):
    return read_records(data, 69, UNIT_FIELDS)


def write_units(records):
    return write_records(records, 69, UNIT_FIELDS)


def read_ground(data):
    return read_records(data, 6, GROUND_FIELDS)


def write_ground(records):
    return write_records(records, 6, GROUND_FIELDS)


def read_codes(data):
    return read_records(data, 10, CODE_FIELDS)


def write_codes(records):
    return write_records(records, 10, CODE_FIELDS)


def read_amok(data):
    if len(data) != 36:
        raise ValueError('AMOK table must have 36 bytes')
    return read_records(data, 36, AMOK_FIELDS)


def write_amok(records):
    if len(records) != 1:
        raise ValueError('AMOK table must have one record')
    return write_records(records, 36, AMOK_FIELDS)


def read_names(data):
    if not data or len(data) % 8:
        raise ValueError('library name table is not eight-byte records')
    return [data[at:at + 8] for at in range(0, len(data), 8)]


def write_names(names):
    if any(len(name) != 8 for name in names):
        raise ValueError('library name record must have eight bytes')
    return b''.join(names)


def read_lookup(data):
    if len(data) != 512:
        raise ValueError('glyph lookup must have 256 sixteen-bit entries')
    return list(struct.unpack('<256H', data))


def write_lookup(values):
    return struct.pack('<256H', *values)


def inspect(path, root):
    data = unpacked(path)
    name = path.name.upper()
    if path.parent.resolve() == root.resolve():
        layouts = {'UNIT.DAT': (read_units, write_units),
                   'WW1GNDS.DAT': (read_ground, write_ground),
                   'WW1GNDW.DAT': (read_ground, write_ground),
                   'CODES.DAT': (read_codes, write_codes),
                   'AMOK.DAT': (read_amok, write_amok)}
        if name in layouts:
            read, write = layouts[name]
            records = read(data)
            return write(records) == data, '%d structural records' % len(records), records
    companions = [p for p in path.parent.iterdir()
                  if p.stem.upper() == path.stem.upper() and p.suffix.upper() == '.LIB']
    if companions:
        library = unpacked(companions[0])
        entries = libfiles.read(library)
        if name == 'NEWCHAR6.DAT':
            values = read_lookup(data)
            directory = struct.unpack_from('<I', library)[0]
            offsets = {struct.unpack_from('<I', library, at + 8)[0]
                       for at in range(directory, len(library), 12)}
            if any(value != 65535 and value not in offsets for value in values):
                raise ValueError('glyph lookup refers outside companion library entries')
            return write_lookup(values) == data, '256 glyph offsets, checked against companion library', values
        names = read_names(data)
        have = {name for name, _ in entries}
        if any(name not in have for name in names):
            raise ValueError('name table contains entries absent from companion library')
        return write_names(names) == data, '%d names, checked against companion library' % len(names), names
    entries = libfiles.read(data)
    return libfiles.write(entries) == data, '%d library entries; unknown entry data preserved' % len(entries), []


def dat_files(root):
    for p in sorted(root.rglob('*')):
        if not p.is_file() or p.suffix.upper() != '.DAT':
            continue
        rel = p.relative_to(root)
        if len(rel.parts) == 1:
            if p.name.upper() in ('UNIT.DAT', 'WW1GNDS.DAT', 'WW1GNDW.DAT', 'CODES.DAT', 'AMOK.DAT', 'BB.DAT'):
                yield p
        elif rel.parts[0].upper() in ('LIB', 'TXT', 'ZWI', 'SEQ'):
            yield p


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('files', nargs='*')
    ap.add_argument('--list', action='store_true', help='print table fields; names are preview text')
    a = ap.parse_args()
    root = Path(game_dir())
    files = [Path(p) for p in a.files] if a.files else list(dat_files(root))
    bad = 0
    for path in files:
        try:
            same, detail, records = inspect(path, root)
            bad += not same
            print('%s: %s; %s' % (path, detail, 'written back identical' if same else 'WRITTEN BACK OTHERWISE'))
            if a.list:
                for n, record in enumerate(records):
                    print('  %d %r' % (n, record))
        except (OSError, ValueError, IndexError, struct.error) as e:
            bad += 1
            print('%s: unclassified or invalid: %s' % (path, e))
    if not files:
        bad += 1
    print('%d files, %d not read or not written back' % (len(files), bad))
    return int(bool(bad))


if __name__ == '__main__':
    sys.exit(main())
