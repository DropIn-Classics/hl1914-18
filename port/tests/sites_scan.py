"""sites_scan.py DUMP.ram [...] - the records of sites_40, sites_100 and
sites_400 (S331F:38BE, 38F6, 3A0E; 1Ch bytes each) in dosrun -ram dumps of
HL.EXE: for each record its word 19h and bytes 16h and 17h, the bytes
side_income (S131D:1685) adds.  Records with 8000h in word 19h (unused)
whose byte 16h or 17h is not 0 are marked UNUSED+.  The segment is that of
the port's layout (PSP at 0066h, port/src/hl.h)."""
import struct, sys

SEG = 0x331F + 0x0066 + 0x10
TABLES = (('sites_40', 0x38BE, 2), ('sites_100', 0x38F6, 10), ('sites_400', 0x3A0E, 10))

bad = 0
for path in sys.argv[1:]:
    m = open(path, 'rb').read()
    print(path)
    for name, off, n in TABLES:
        for r in range(n):
            a = SEG * 16 + off + r * 0x1C
            w19 = struct.unpack_from('<H', m, a + 0x19)[0]
            b16, b17 = m[a + 0x16], m[a + 0x17]
            flag = ''
            if w19 & 0x8000 and (b16 or b17):
                flag = ' UNUSED+'
                bad += 1
            if w19 & 0x8000 and not flag:
                continue
            print('  %-9s %2d  19h=%04X  16h=%02X 17h=%02X%s' % (name, r, w19, b16, b17, flag))
print('unused records with byte 16h or 17h not 0: %d' % bad)
