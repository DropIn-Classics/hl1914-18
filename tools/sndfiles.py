#!/usr/bin/env python3
"""History Line ADLX music containers, audio banks and IFF 8SVX samples.

    sndfiles.py [FILE ...] [--wav DIR] [--out DIR]

Layouts are observed in this game's own files. bisle's MIDI parser and
sound-player implementation are not applicable and are not imported.
ADLX headers and track payloads are retained as explicitly unparsed data;
this tool does not emulate the music player or infer track duration.
"""
import argparse
import os
from pathlib import Path
import struct
import sys
import wave

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'doskit', 'tools'))
from kit import game_dir
import ifffiles
import tpwmfiles


def unpacked(path):
    data = Path(path).read_bytes()
    return tpwmfiles.unpack(data) if data[:4] == tpwmfiles.MAGIC else data


def read_adlx(data):
    if len(data) < 42 or data[:4] != b'ADLX':
        raise ValueError('not an ADLX container')
    size = struct.unpack_from('>I', data, 4)[0]
    if size != len(data) - 8:
        raise ValueError('ADLX outer length mismatch')
    if data[8:12] != b'AHDR' or struct.unpack_from('>I', data, 12)[0] != 10:
        raise ValueError('unsupported ADLX header layout')
    if data[26:30] != b'IMAP' or struct.unpack_from('>I', data, 30)[0] != 0:
        raise ValueError('unsupported ADLX instrument-map layout')
    if data[34:38] != b'TRCK' or struct.unpack_from('<I', data, 38)[0] != size:
        raise ValueError('ADLX TRCK length does not match outer payload length')
    return dict(header=tuple(data[16:26]), track=data[42:])


def write_adlx(song):
    if len(song['header']) != 10:
        raise ValueError('ADLX header must have ten bytes')
    size = 34 + len(song['track'])
    return (b'ADLX' + struct.pack('>I', size) + b'AHDR' + struct.pack('>I', 10) +
            bytes(song['header']) + b'IMAP' + struct.pack('>I', 0) +
            b'TRCK' + struct.pack('<I', size) + song['track'])


def read_sample(data):
    kind, chunks = ifffiles.parse_form(data)
    if kind != '8SVX':
        raise ValueError('IFF container is not 8SVX')
    headers = [body for name, body, _ in chunks if name == 'VHDR']
    bodies = [body for name, body, _ in chunks if name == 'BODY']
    if len(headers) != 1 or len(headers[0]) != 20 or len(bodies) != 1:
        raise ValueError('8SVX needs one twenty-byte VHDR and one BODY')
    values = struct.unpack('>IIIHBBI', headers[0])
    return dict(chunks=chunks, one_shot=values[0], repeat=values[1],
                cycle=values[2], rate=values[3], octave=values[4],
                compression=values[5], volume=values[6], samples=bodies[0])


def write_sample(sample):
    header = struct.pack('>IIIHBBI', sample['one_shot'], sample['repeat'],
                         sample['cycle'], sample['rate'], sample['octave'],
                         sample['compression'], sample['volume'])
    chunks = [(name, header if name == 'VHDR' else sample['samples'] if name == 'BODY' else body, pad)
              for name, body, pad in sample['chunks']]
    return ifffiles.write_form('8SVX', chunks)


def container_length(data, at):
    if data[at:at + 4] not in (b'ADLX', b'FORM') or at + 8 > len(data):
        raise ValueError('unknown bank container at offset %d' % at)
    return 8 + struct.unpack_from('>I', data, at + 4)[0]


def read_bank(data):
    if len(data) < 4:
        raise ValueError('audio bank too short')
    directory = struct.unpack_from('<I', data)[0]
    if not 4 <= directory <= len(data) or (len(data) - directory) % 12:
        raise ValueError('invalid audio-bank directory')
    entries = []
    at = 4
    for pos in range(directory, len(data), 12):
        name = data[pos:pos + 8]
        offset = struct.unpack_from('<I', data, pos + 8)[0]
        if offset != at:
            raise ValueError('audio directory offsets do not match contiguous containers')
        size = container_length(data, at)
        end = at + size
        if end + (size & 1) > directory:
            raise ValueError('audio container extends into directory')
        blob = data[at:end]
        kind, content = read_container(blob)
        pad = data[end] if size & 1 else None
        entries.append(dict(name=name, kind=kind, content=content, pad=pad))
        at = end + (size & 1)
    if not entries or at != directory:
        raise ValueError('audio bank has unreferenced bytes before directory')
    return entries


def write_bank(entries):
    body, directory = bytearray(), bytearray()
    for entry in entries:
        if len(entry['name']) != 8:
            raise ValueError('bank entry name must have eight bytes')
        directory += entry['name'] + struct.pack('<I', 4 + len(body))
        blob = write_container(entry['kind'], entry['content'])
        body += blob
        if len(blob) & 1:
            body.append(entry['pad'] or 0)
    return struct.pack('<I', 4 + len(body)) + body + directory


def read_container(data):
    if data[:4] == b'ADLX':
        return 'ADLX', read_adlx(data)
    return '8SVX', read_sample(data)


def write_container(kind, parsed):
    if kind == 'ADLX':
        return write_adlx(parsed)
    if kind == '8SVX':
        return write_sample(parsed)
    raise ValueError('unsupported audio container')


def to_wav(path, sample):
    if sample['compression'] != 0 or sample['octave'] != 1 or not sample['rate']:
        raise ValueError('WAV export supports uncompressed single-octave samples only')
    # 8SVX signed eight-bit PCM becomes unsigned eight-bit WAV PCM.
    with wave.open(str(path), 'wb') as out:
        out.setnchannels(1)
        out.setsampwidth(1)
        out.setframerate(sample['rate'])
        out.writeframes(bytes((value + 128) & 255 for value in sample['samples']))


def sound_files(root):
    return sorted(p for p in root.rglob('*') if p.is_file() and p.suffix.upper() in ('.ADL', '.AFX', '.SFX'))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('files', nargs='*')
    ap.add_argument('--wav', help='write uncompressed sample WAVs into a build directory')
    ap.add_argument('--out', help='extract bank containers into a build directory')
    a = ap.parse_args()
    root = Path(game_dir())
    files = [Path(p) for p in a.files] if a.files else sound_files(root)
    bad = songs = samples = 0
    for path in files:
        try:
            data = unpacked(path)
            if data[:4] in (b'ADLX', b'FORM'):
                kind, parsed = read_container(data)
                same = write_container(kind, parsed) == data
                entries = [dict(kind=kind, content=parsed)]
            else:
                entries = read_bank(data)
                same = write_bank(entries) == data
            detail = []
            for n, entry in enumerate(entries):
                kind, parsed = entry['kind'], entry['content']
                try:
                    rel = path.resolve().relative_to(root.resolve())
                except ValueError:
                    rel = Path(path.name)
                stem = rel.parent / (rel.name + '.%03d' % n)
                if kind == 'ADLX':
                    songs += 1
                    detail.append('ADLX %d unparsed track bytes' % len(parsed['track']))
                else:
                    samples += 1
                    detail.append('8SVX %d samples at %d Hz' % (len(parsed['samples']), parsed['rate']))
                    if a.wav:
                        dst = Path(a.wav) / (str(stem) + '.wav')
                        dst.parent.mkdir(parents=True, exist_ok=True)
                        to_wav(dst, parsed)
                if a.out:
                    dst = Path(a.out) / (str(stem) + ('.ADL' if kind == 'ADLX' else '.SFX'))
                    dst.parent.mkdir(parents=True, exist_ok=True)
                    dst.write_bytes(write_container(kind, parsed))
            bad += not same
            print('%s: %s; %s' % (path, ', '.join(detail),
                  'written back identical' if same else 'WRITTEN BACK OTHERWISE'))
        except (OSError, ValueError, IndexError, struct.error) as e:
            bad += 1
            print('%s: %s' % (path, e))
    if not files:
        bad += 1
    print('%d files, %d ADLX containers, %d samples, %d not read or not written back' %
          (len(files), songs, samples, bad))
    return int(bool(bad))


if __name__ == '__main__':
    sys.exit(main())
