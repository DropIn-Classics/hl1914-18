"""Indexed PNG output for the format tools (standard library only).

Imported unchanged from bisle; see PROVENANCE.md."""
import struct
import zlib


def _chunk(kind, data):
    body = kind + data
    return struct.pack('>I', len(data)) + body + struct.pack('>I', zlib.crc32(body))


def write_indexed(path, width, height, pixels, rgb, transparent=None):
    """pixels: width*height bytes of colour numbers; rgb: 256 (r, g, b)
    triples of 0..255; transparent: a colour number shown as clear."""
    if len(pixels) != width * height or len(rgb) != 256:
        raise ValueError('bad size or palette')
    raw = b''.join(b'\0' + bytes(pixels[y * width:(y + 1) * width]) for y in range(height))
    trns = b''
    if transparent is not None:
        trns = _chunk(b'tRNS', bytes(0 if i == transparent else 255 for i in range(256)))
    png = (b'\x89PNG\r\n\x1a\n' +
           _chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 3, 0, 0, 0)) +
           _chunk(b'PLTE', bytes(c for colour in rgb for c in colour)) + trns +
           _chunk(b'IDAT', zlib.compress(raw, 9)) +
           _chunk(b'IEND', b''))
    with open(path, 'wb') as f:
        f.write(png)


def dac_to_rgb(v):
    """a 6-bit DAC value as 0..255"""
    return (v * 255 + 31) // 63
