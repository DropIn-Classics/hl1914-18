#!/usr/bin/env python3
"""History Line packed-file analysis (TPWM).

    tpwmfiles.py [FILE ...] [--out DIR]

Without FILE, scan the installed game's folder for packed files. Check
parsed-item round trips and compare the inherited "isle" and "moon"
packer variants. --out writes unpacked files; use a directory in build/.
All 489 installed packed files passed the initial compatibility probe.
Imported from bisle by explicit user request; see PROVENANCE.md.
Packer names describe inherited variants, not History Line runtime evidence.
"""
import argparse
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'doskit', 'tools'))
from kit import game_dir  # noqa: E402

MAGIC = b'TPWM'
MIN_LEN, MAX_LEN, WINDOW = 3, 18, 0xFFF


def parse(data):
    """(length, items, flag bits after the last item) of a TPWM file;
    an item is a byte (int) or a copy (length, distance).  ValueError
    when it is none or ends early."""
    if data[:4] != MAGIC or len(data) < 8:
        raise ValueError('not a TPWM file')
    n = struct.unpack_from('<I', data, 4)[0]
    items = []
    done = 0
    i = 8
    spare = 0
    try:
        while done < n:
            flags = data[i]
            i += 1
            for k in range(8):
                if done >= n:
                    spare = flags & 0xFF        # the unused bits, at the top
                    break
                if flags & 0x80:
                    b1, b2 = data[i], data[i + 1]
                    i += 2
                    length, dist = (b1 & 0x0F) + MIN_LEN, (b1 & 0xF0) << 4 | b2
                    if dist == 0 or dist > done:
                        raise ValueError('copy from before the start at %d' % done)
                    items.append((length, dist))
                    done += length
                else:
                    items.append(data[i])
                    i += 1
                    done += 1
                flags = flags << 1 & 0xFF
    except IndexError:
        raise ValueError('ends after %d of %d bytes' % (done, n))
    if i != len(data):
        raise ValueError('%d bytes after the end' % (len(data) - i))
    return n, items, spare


def write(n, items, spare=0):
    """The file's bytes from parse()'s result."""
    out = bytearray(MAGIC + struct.pack('<I', n))
    for g in range(0, len(items), 8):
        group = items[g:g + 8]
        flags = 0
        body = bytearray()
        for k, it in enumerate(group):
            if isinstance(it, tuple):
                length, dist = it
                flags |= 0x80 >> k
                body += bytes(((dist >> 4) & 0xF0 | length - MIN_LEN, dist & 0xFF))
            else:
                body.append(it)
        if len(group) < 8:
            flags |= spare >> len(group)
        out.append(flags)
        out += body
    return bytes(out)


def expand(n, items):
    """The unpacked bytes."""
    out = bytearray()
    for it in items:
        if isinstance(it, tuple):
            length, dist = it
            for _ in range(length):
                if len(out) >= n:
                    break
                out.append(out[-dist])
        else:
            out.append(it)
    return bytes(out[:n])


def unpack(data):
    n, items, _ = parse(data)
    return expand(n, items)


def pack_items(raw, packer='isle', after=0):
    """The items the named packer makes of raw (see the docstring)."""
    n = len(raw)
    src = raw + bytes((after,))
    heads = {}              # 3 bytes -> their positions, oldest first
    items = []
    pos = 0
    while pos < n:
        best_len, best_dist = 0, 0
        chain = heads.get(raw[pos:pos + 3], []) if pos + MIN_LEN <= n else []
        if packer == 'isle':
            limit = min(MAX_LEN, n - pos + 1)
            order = chain                   # oldest first: farthest wins ties
        else:
            limit = min(MAX_LEN, n - pos)
            order = reversed(chain)         # newest first: nearest wins ties
        for p in order:
            dist = pos - p
            if dist > WINDOW:
                if packer == 'isle':
                    continue
                break
            k = MIN_LEN
            while k < limit and src[p + k] == src[pos + k]:
                k += 1
            if k > best_len:
                best_len, best_dist = k, dist
                if k == limit:
                    break
        best_len = min(best_len, n - pos)
        if best_len >= MIN_LEN:
            items.append((best_len, best_dist))
            step = best_len
        else:
            items.append(raw[pos])
            step = 1
        for q in range(pos, pos + step):
            if q + MIN_LEN <= n and (q > 0 or packer == 'isle'):
                lst = heads.setdefault(raw[q:q + 3], [])
                lst.append(q)
                while pos - lst[0] > WINDOW + MAX_LEN:   # out of reach for good
                    lst.pop(0)
        pos += step
    return items


def pack(raw, packer='isle', after=0):
    return write(len(raw), pack_items(raw, packer, after))


def which_packer(n, items, raw):
    """How the file's items come from raw: a short description."""
    isle = pack_items(raw, 'isle')
    if isle == items:
        return 'isle'
    if isle[:-8] == items[:-8]:         # the byte past the end matters at the end only
        for after in range(1, 256):
            if pack_items(raw, 'isle', after) == items:
                return 'isle, %02X past the end' % after
    moon = pack_items(raw, 'moon')
    if moon == items:
        return 'moon'
    if len(moon) >= len(items) - 1 and moon[:len(items) - 1] == items[:-1]:
        return 'moon but the last item'
    return 'none'


def packed_files(root):
    for d, dirs, files in os.walk(root):
        dirs.sort()
        for f in sorted(files):
            path = os.path.join(d, f)
            with open(path, 'rb') as fh:
                if fh.read(4) == MAGIC:
                    yield path


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('files', nargs='*')
    ap.add_argument('--out', help='write the unpacked files here')
    a = ap.parse_args()
    root = game_dir()
    files = a.files or list(packed_files(root))
    bad = 0
    count = {}
    for path in files:
        rel = os.path.relpath(path, root) if not a.files else path
        data = open(path, 'rb').read()
        try:
            n, items, spare = parse(data)
        except ValueError as e:
            print('%-28s %s' % (rel, e))
            bad += 1
            continue
        same = write(n, items, spare) == data
        raw = expand(n, items)
        how = which_packer(n, items, raw)
        bad += not same or how == 'none'
        key = how.split(',')[0]
        count[key] = count.get(key, 0) + 1
        print('%-28s %7d -> %7d  %s%s' % (rel, len(data), n,
              'written back identical' if same else 'WRITTEN BACK OTHERWISE',
              '; packer ' + how + (', unused flag bits %02X' % spare if spare else '')))
        if a.out:
            dst = os.path.join(a.out, os.path.relpath(path, root))
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            with open(dst, 'wb') as fh:
                fh.write(raw)
    print('%d files, %d not written back or of no known packer; %s' % (
        len(files), bad, ', '.join('%s %d' % kv for kv in sorted(count.items()))))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
