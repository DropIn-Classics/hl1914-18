#!/usr/bin/env python3
"""History Line text tables, CRLF text files and small font inspection.

    txtfiles.py [FILE ...] [--list] [--png DIR] [--encoding cp437]

GAME.TXT uses 16-bit offsets, unlike bisle's terminated text groups.
The small-font and title readers are adapted from bisle; see PROVENANCE.md.
Font PNGs are previews, not verified copies of the original rendering.
"""
import argparse
import os
from pathlib import Path
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'doskit', 'tools'))
from kit import game_dir
import png
import tpwmfiles


def unpacked(path):
    data = Path(path).read_bytes()
    return tpwmfiles.unpack(data) if data[:4] == tpwmfiles.MAGIC else data


def read_texts(data):
    if len(data) < 2:
        raise ValueError('text offset table too short')
    first = struct.unpack_from('<H', data)[0]
    if first < 2 or first % 2 or first >= len(data):
        raise ValueError('invalid text offset-table size')
    offsets = struct.unpack_from('<%dH' % (first // 2), data)
    if any(a >= b for a, b in zip(offsets, offsets[1:])) or offsets[-1] >= len(data):
        raise ValueError('text offsets out of order or outside file')
    texts = []
    for start, end in zip(offsets, offsets[1:] + (len(data),)):
        span = data[start:end]
        if not span.endswith(b'\0') or b'\0' in span[:-1]:
            raise ValueError('text span does not contain one terminated string')
        texts.append(span[:-1])
    return texts


def write_texts(texts):
    if not texts or any(b'\0' in text for text in texts):
        raise ValueError('empty table or embedded text terminator')
    offsets = []
    at = 2 * len(texts)
    for text in texts:
        offsets.append(at)
        at += len(text) + 1
    if at > 65536:
        raise ValueError('text table exceeds 16-bit offsets')
    return struct.pack('<%dH' % len(offsets), *offsets) + b''.join(text + b'\0' for text in texts)


def read_font(data):
    if not data or len(data) % 12:
        raise ValueError('font length is not six 16-bit rows per glyph')
    return [struct.unpack_from('<6H', data, at) for at in range(0, len(data), 12)]


def write_font(font):
    return b''.join(struct.pack('<6H', *rows) for rows in font)


def read_title(data):
    lines, at = [], 0
    while at < len(data) and data[at] != 2:
        end = data.find(b'\0', at)
        if end < 0:
            raise ValueError('title line has no terminator')
        lines.append(data[at:end])
        at = end + 1
    if at == len(data):
        raise ValueError('title has no end marker')
    return lines, data[at:]


def write_title(title):
    lines, tail = title
    return b''.join(line + b'\0' for line in lines) + tail


def read_lines(data):
    if b'\0' in data or any(c < 32 and c not in (9, 10, 13) for c in data):
        raise ValueError('not plain text: unsupported control bytes')
    if not data.endswith(b'\r\n'):
        raise ValueError('plain text does not end in CRLF')
    lines = data[:-2].split(b'\r\n')
    if any(b'\r' in line or b'\n' in line for line in lines):
        raise ValueError('mixed line endings')
    return lines


def write_lines(lines):
    return b'\r\n'.join(lines) + b'\r\n'


def font_sheet(path, font):
    width, height = 16 * 14, ((len(font) + 15) // 16) * 8
    pixels = bytearray(width * height)
    for n, glyph in enumerate(font):
        for y, word in enumerate(glyph):
            bits = ((word & 255) << 4) | (word >> 12)
            for x in range(12):
                if bits & (0x800 >> x):
                    pixels[((n // 16) * 8 + y + 1) * width + (n % 16) * 14 + x + 1] = 1
    png.write_indexed(path, width, height, pixels,
                      [(0, 0, 0), (255, 255, 255)] + [(0, 0, 0)] * 254)


def text_files(root):
    for p in sorted(Path(root).rglob('*')):
        if p.is_file() and (p.suffix.upper() == '.TXT' or p.name.upper() == 'CHAR6.DAT'):
            # Installer, manuals and the unclassified INTRO/BANG.TXT are
            # outside these verified text layouts.
            rel = p.relative_to(root)
            if p.name.upper() in ('GAME.TXT', 'TITEL.TXT', 'CHAR6.DAT') or (
                    rel.parts[0].upper() in ('TXT', 'INTRO', 'EXTRO')
                    and p.name.upper() != 'BANG.TXT'):
                yield p


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('files', nargs='*')
    ap.add_argument('--list', action='store_true', help='print decoded texts; encoding is a preview assumption')
    ap.add_argument('--encoding', default='cp437')
    ap.add_argument('--png', help='write small-font preview in a build directory')
    a = ap.parse_args()
    root = Path(game_dir())
    files = [Path(p) for p in a.files] if a.files else list(text_files(root))
    bad = 0
    for p in files:
        try:
            data = unpacked(p)
            name = p.name.upper()
            if name == 'CHAR6.DAT':
                parsed = read_font(data)
                same = write_font(parsed) == data
                detail = '%d glyphs' % len(parsed)
                if a.png:
                    os.makedirs(a.png, exist_ok=True)
                    font_sheet(os.path.join(a.png, 'CHAR6.png'), parsed)
                texts = []
            elif name == 'GAME.TXT':
                texts = read_texts(data)
                same = write_texts(texts) == data
                detail = '%d offset-table strings' % len(texts)
            elif name == 'TITEL.TXT':
                parsed = read_title(data)
                same = write_title(parsed) == data
                detail = '%d encoded title lines; tail preserved' % len(parsed[0])
                texts = []
            else:
                texts = read_lines(data)
                same = write_lines(texts) == data
                detail = '%d CRLF lines' % len(texts)
            bad += not same
            print('%s: %s; %s' % (p, detail, 'written back identical' if same else 'WRITTEN BACK OTHERWISE'))
            if a.list:
                for n, text in enumerate(texts):
                    print('  %d %s' % (n, text.decode(a.encoding, errors='replace')))
        except (OSError, ValueError, struct.error) as e:
            bad += 1
            print('%s: %s' % (p, e))
    if not files:
        bad += 1
    print('%d files, %d not read or not written back' % (len(files), bad))
    return int(bool(bad))


if __name__ == '__main__':
    sys.exit(main())
