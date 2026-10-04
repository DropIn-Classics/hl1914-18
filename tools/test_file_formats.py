"""Synthetic regression tests: no game assets or original data fixtures."""
from pathlib import Path
import struct
import tempfile
import unittest
import wave

import datfiles
import ifffiles
import mapfiles
import savfiles
import sndfiles
import txtfiles


class TextTests(unittest.TestCase):
    def test_offsets_rebuilt_after_edit(self):
        original = txtfiles.write_texts([b'alpha', b'', b'omega'])
        texts = txtfiles.read_texts(original)
        texts[0] += b' longer'
        changed = txtfiles.write_texts(texts)
        self.assertEqual(txtfiles.read_texts(changed), texts)
        self.assertEqual(struct.unpack_from('<H', changed, 2)[0], 6 + len(texts[0]) + 1)

    def test_bad_offsets_and_missing_terminator(self):
        for data in (struct.pack('<2H', 4, 3) + b'x\0',
                     struct.pack('<H', 2) + b'unterminated'):
            with self.assertRaises(ValueError):
                txtfiles.read_texts(data)

    def test_crlf_empty_lines_preserved(self):
        lines = [b'first', b'', b'last', b'']
        self.assertEqual(txtfiles.read_lines(txtfiles.write_lines(lines)), lines)
        with self.assertRaises(ValueError):
            txtfiles.read_lines(b'bad\n')


class TableTests(unittest.TestCase):
    def test_unit_fields_rebuilt(self):
        record = {name: b'\0' * 17 if fmt == '17s' else 0
                  for name, _, fmt in datfiles.UNIT_FIELDS}
        record['b1A'] = 19
        record['text1'] = b'new label'.ljust(17, b'\0')
        record['b44'] = 31
        raw = datfiles.write_units([record])
        self.assertEqual(len(raw), 69)
        self.assertEqual(datfiles.read_units(raw), [record])
        with self.assertRaises(ValueError):
            datfiles.read_units(raw[:-1])


class MapTests(unittest.TestCase):
    def test_dimensions_and_squares(self):
        m = dict(reserved=(3, 7), w=2, h=1, squares=[(4, 9), (5, 10)])
        self.assertEqual(mapfiles.read_fin(mapfiles.write_fin(m)), m)
        m['w'] = 3
        with self.assertRaises(ValueError):
            mapfiles.write_fin(m)

    def test_shp_regenerates_count_and_fields(self):
        s = dict(availability=[0] * 51, records=[tuple(range(12))])
        raw = mapfiles.write_shp(s)
        self.assertEqual(raw[51], 1)
        self.assertEqual(mapfiles.read_shp(raw), s)
        with self.assertRaises(ValueError):
            mapfiles.read_shp(raw + b'\0')
        s['records'].append(tuple(reversed(range(12))))
        self.assertEqual(mapfiles.write_shp(s)[51], 2)


class AudioTests(unittest.TestCase):
    def sample(self):
        header = struct.pack('>IIIHBBI', 4, 0, 0, 8000, 1, 0, 65536)
        blob = ifffiles.write_form('8SVX', [('VHDR', header, None),
                                          ('BODY', bytes((0, 127, 128, 255)), None)])
        return sndfiles.read_sample(blob)

    def test_adlx_lengths_rebuilt_and_checked(self):
        song = dict(header=tuple(range(10)), track=b'synthetic events')
        blob = sndfiles.write_adlx(song)
        self.assertEqual(sndfiles.read_adlx(blob), song)
        self.assertEqual(struct.unpack_from('>I', blob, 4)[0], len(blob) - 8)
        self.assertEqual(struct.unpack_from('<I', blob, 38)[0], len(blob) - 8)
        with self.assertRaises(ValueError):
            sndfiles.read_adlx(blob[:-1])

    def test_bank_rebuilds_directory_after_edit(self):
        entries = [dict(name=b'first\0\0\0', kind='ADLX',
                        content=dict(header=(0,) * 10, track=b'odd'), pad=43),
                   dict(name=b'sample\0\0', kind='8SVX', content=self.sample(), pad=None)]
        blob = sndfiles.write_bank(entries)
        parsed = sndfiles.read_bank(blob)
        self.assertEqual(sndfiles.write_bank(parsed), blob)
        parsed[0]['content']['track'] += b'longer'
        rebuilt = sndfiles.write_bank(parsed)
        self.assertEqual(sndfiles.read_bank(rebuilt)[1]['content']['samples'], bytes((0, 127, 128, 255)))
        damaged = bytearray(blob)
        directory = struct.unpack_from('<I', blob)[0]
        struct.pack_into('<I', damaged, directory + 8, 5)
        with self.assertRaises(ValueError):
            sndfiles.read_bank(damaged)

    def test_sample_edits_and_wav_signed_conversion(self):
        sample = self.sample()
        sample['rate'] = 11025
        self.assertEqual(sndfiles.read_sample(sndfiles.write_sample(sample))['rate'], 11025)
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'sample.wav'
            sndfiles.to_wav(path, sample)
            with wave.open(str(path), 'rb') as wav:
                self.assertEqual((wav.getnchannels(), wav.getsampwidth(), wav.getframerate()), (1, 1, 11025))
                self.assertEqual(wav.readframes(4), bytes((128, 255, 0, 127)))
        sample['compression'] = 1
        with tempfile.TemporaryDirectory() as folder:
            with self.assertRaises(ValueError):
                sndfiles.to_wav(Path(folder) / 'bad.wav', sample)



class SaveFiles(unittest.TestCase):
    def test_split_and_written_back(self):
        blob = bytes((i * 7 + 3) & 255 for i in range(savfiles.SIZE))
        head, blocks = savfiles.read_save(blob)
        self.assertEqual(len(blocks), 24)
        self.assertEqual(head['w0b'], blob[11] | blob[12] << 8)
        self.assertEqual(blocks[0], blob[0x1B:0x1B + 0x1806])
        self.assertEqual(savfiles.write_save(head, blocks), blob)
        with self.assertRaises(ValueError):
            savfiles.read_save(blob[:-1])
        blocks[3] = blocks[3][:-1]
        with self.assertRaises(ValueError):
            savfiles.write_save(head, blocks)

    def test_units(self):
        self.assertEqual(sum(n for _, n, _ in savfiles.BLOCKS[:savfiles.UNITS]) + savfiles.HEADER, 0x523A)
        self.assertEqual(savfiles.BLOCKS[savfiles.UNITS][1], savfiles.UNIT_COUNT * savfiles.UNIT_REC)
        empty = bytes(5) + b'\x80' + bytes(savfiles.UNIT_REC - 6)
        used = bytes(range(1, savfiles.UNIT_REC + 1))
        blocks = [bytes(n) for _, n, _ in savfiles.BLOCKS]
        blocks[savfiles.UNITS] = empty * 2 + used + empty * (savfiles.UNIT_COUNT - 3)
        self.assertEqual(savfiles.units(blocks), [(2, used)])


if __name__ == '__main__':
    unittest.main()
