#!/usr/bin/env python3
"""History Line saved games (`0N.DAT`, written by HL.EXE's save_game).

    savfiles.py FILE [...] [--list]

The layout is read from this game's own program (src/HL.hints: save_game,
S1273:0005, and the routine at S1273:0662 that lists what it writes): a
header of 1Bh bytes, then 24 blocks of fixed length copied from memory, not
packed.  The lengths add up to the size of a file the game wrote in a run
(35,411 bytes).  What the blocks hold is known only as far as the notes
below say; the tool splits a file, writes it back and checks that the bytes
are the same.  It is an inspection aid, not part of the port.
"""
import argparse
from pathlib import Path
import struct
import sys

HEADER = 0x1B
# (where save_game takes the block from, bytes, note).  An address in
# brackets is a far pointer variable: the block is what it points to.
BLOCKS = [
    ('[S331F:17B6]', 0x1806, ''),
    ('[S331F:17B2]', 0x1806, ''),
    ('[S331F:4D44]', 0xF2, "side one (the pointer is in the side's record at S331F:4D33)"),
    ('[S331F:4D5B]', 0xF2, 'side two (record at S331F:4D4A)'),
    ('[S331F:1893]', 0x82, ''),
    ('[S331F:1897]', 0x82, ''),
    ("save_game's buffer", 0x320, ''),
    ('S331F:4CA4', 0x8F, 'state words (the flag words 4CA4 and 4CA8 are in it)'),
    ('S331F:4D92', 0x31, 'side one, 31h-byte record'),
    ('S331F:4DC3', 0x31, 'side two, 31h-byte record'),
    ('S331F:4C9E', 6, ''),
    ('S331F:3B5A', 0x1104, ''),
    ('S331F:3A0E', 0x1C * 0x0A, '1Ch records of 0Ah bytes'),
    ('S331F:38F6', 0x1C * 0x0A, '1Ch records of 0Ah bytes'),
    ('S331F:38BE', 0x1C * 2, '1Ch words'),
    ('S331F:3116', 0x1C * 0x46, '1Ch records of 46h bytes'),
    ('S331F:189C', 0x1A * 0xF1, '1Ah records of F1h bytes'),
    ('S331F:09BD', 0x45 * 0x33, '45h records of 33h bytes (the size of the top-level UNIT.DAT)'),
    ('S39B6:0132', 9 * 0xF1, '9 records of F1h bytes'),
    ('S39B6:09AB', 9 * 0xF1, '9 records of F1h bytes'),
    ('S39B6:12FD', 0x0C * 2, '0Ch words'),
    ('S39B6:126B', 0x49 * 2, '49h words'),
    ('S39B6:1224', 0x20 * 2, '20h words'),
    ('S331F:02B5', 2 * 2, 'two words'),
]
SIZE = HEADER + sum(n for _, n, _ in BLOCKS)


def read_save(data):
    """The header's fields and the blocks of a saved game."""
    if len(data) != SIZE:
        raise ValueError(f'{len(data)} bytes, a saved game has {SIZE}')
    h = data[:HEADER]
    head = {
        'b00': h[0], 'b01': h[1],             # written as 0 and 7
        'b02': h[2], 'b03': h[3],             # S331F:1892, S331F:189B
        'six': bytes(h[4:10]),                # S37FF:0129..012E
        'b0a': h[10],                         # S37FF:012F
        'w0b': struct.unpack_from('<H', h, 11)[0],   # S331F:0008
        # of each side's record (S331F:4D33, 4D4A): its word 0 and bytes 2, 3, 4, 15h, 16h
        'side1': bytes(h[13:20]), 'side2': bytes(h[20:27]),
    }
    blocks, pos = [], HEADER
    for _, n, _ in BLOCKS:
        blocks.append(bytes(data[pos:pos + n])); pos += n
    return head, blocks


def write_save(head, blocks):
    if len(blocks) != len(BLOCKS) or any(len(b) != n for b, (_, n, _) in zip(blocks, BLOCKS)):
        raise ValueError('a saved game has 24 blocks of fixed length')
    if len(head['six']) != 6 or len(head['side1']) != 7 or len(head['side2']) != 7:
        raise ValueError('header field of the wrong length')
    h = bytes((head['b00'], head['b01'], head['b02'], head['b03'])) + head['six']
    h += bytes((head['b0a'],)) + struct.pack('<H', head['w0b']) + head['side1'] + head['side2']
    return h + b''.join(blocks)


def main():
    ap = argparse.ArgumentParser(description='History Line saved-game inspection.')
    ap.add_argument('files', nargs='+')
    ap.add_argument('--list', action='store_true', help='print the header and the blocks')
    a = ap.parse_args()
    bad = 0
    for f in a.files:
        data = Path(f).read_bytes()
        try:
            head, blocks = read_save(data)
        except ValueError as e:
            print(f'{f}: {e}'); bad += 1; continue
        same = write_save(head, blocks) == data
        bad += not same
        print(f'{f}: header and {len(blocks)} blocks; ' + ('written back identical' if same else 'NOT identical'))
        if a.list:
            print('  header:', {k: (v.hex() if isinstance(v, bytes) else v) for k, v in head.items()})
            pos = HEADER
            for (src, n, note), b in zip(BLOCKS, blocks):
                print(f'  {pos:5X} {n:5X} {src:20} {sum(1 for x in b if x):5} bytes not zero  {note}')
                pos += n
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
