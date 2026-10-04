/* map.c - a battle's state from its map: the part of HL.EXE's map_load
 * (S0E56:0329) behind the loading of the file, and map_copy (S0E56:1D9B),
 * translated over the program's memory.  port/README.md says how this was
 * compared with the original.
 */
#include "hl.h"

#define SIDE_SIZE 0x17u     /* a record of `sides` */
#define SITE_SIZE 0x1Cu     /* a record of `carriers` and the like */
#define GROUND_SIZE 6u      /* a record of `ww1gnds_dat` */

static void inc(uint16_t seg, uint16_t off, uint8_t by)
{
    fwb(seg, off, (uint8_t)(frb(seg, off) + by));
}

/* S0E56:1D9B: copies the map's bytes */
void map_copy(Far from, Far to)
{
    int16_t i;

    for (i = 0; (int16_t)NW(map_bytes, 0) > i; i++)
        pwb(to, (uint16_t)i, pb(from, (uint16_t)i));
}

/* map_load before it loads the file: the counts and the kinds' serial
 * numbers start anew.  The third byte of site_totals (the carriers) is
 * not set to 0 here. */
void map_reset(void)
{
    uint8_t k;

    SETB(sides, SIDE_SIZE + 2, 0);
    SETB(sides, SIDE_SIZE + 4, 0);
    SETB(sides, SIDE_SIZE + 3, 0);
    SETB(sides, 2, 0);
    SETB(sides, 4, 0);
    SETB(sides, 3, 0);
    SETB(site_totals, 1, 0);
    SETB(site_totals, 0, 0);
    SETB(unit_total, 0, 0);
    for (k = 0; k < 0x33; k++) {
        SETB(unit_dat, k * KIND_SIZE + 0x3D, 1);
        SETB(unit_dat, k * KIND_SIZE + 0x3E, 1);
    }
}

/* one or two units of a kind on the field whose unit byte is at `at`;
 * `side` is 0 or 1 and the second half of a two-part unit lies a row up
 * for side 0, a row down for side 1 */
static void units_put(uint8_t kind, uint16_t at, uint16_t side)
{
    uint16_t k = (uint16_t)(kind * KIND_SIZE + 0x0D);
    uint8_t n;

    if (NW(unit_dat, k) & 0x40) {
        uint16_t row;
        if (!(NW(unit_dat, k) & 0x80))
            return;
        row = (uint16_t)(NW(map_width, 0) << 1);
        n = NB(unit_total, 0);
        SETB(unit_total, 0, n + 1);
        unit_add(n, kind, at, side);
        n = NB(unit_total, 0);
        SETB(unit_total, 0, n + 1);
        unit_add(n, (uint8_t)(kind + 1), (uint16_t)(side ? at + row : at - row), side);
        inc(S_sides, (uint16_t)(A_sides + side * SIDE_SIZE + 2), 2);
    } else if (NW(unit_dat, k) & 2) {
        n = NB(unit_total, 0);
        SETB(unit_total, 0, n + 1);
        unit_add(n, kind, at, 2);
    } else {
        n = NB(unit_total, 0);
        SETB(unit_total, 0, n + 1);
        unit_add(n, kind, at, side);
        inc(S_sides, (uint16_t)(A_sides + side * SIDE_SIZE + 2), 1);
    }
}

/* map_load from S0E56:0411 on: `file` is the unpacked map file in memory
 * (four header bytes, then two bytes a field).  Sets the map's size and
 * map_ptr, marks all records unused, makes the sites and units the fields
 * name, writes each unit's number into its field and copies the map to
 * the second one. */
void map_setup(Far file)
{
    Far map;
    int16_t bytes, i;
    uint8_t n;

    SETW(map_width, 0, pb(file, 1));
    SETW(map_height, 0, pb(file, 3));
    map.seg = file.seg;
    map.off = (uint16_t)(file.off + 4);
    SETW(map_ptr, 2, map.seg);
    SETW(map_ptr, 0, map.off);
    bytes = (int16_t)((uint16_t)(NW(map_width, 0) * NW(map_height, 0)) << 1);
    SETW(map_bytes, 0, (uint16_t)bytes);
    for (n = 0; n <= 0xF0; n++)
        SETW(unit_flags, n * UNIT_SIZE, 0x8000);
    for (n = 0; n < 0x0A; n++)
        SETW(sites_400, n * SITE_SIZE + 0x19, 0x8000);
    for (n = 0; n < 0x0A; n++)
        SETW(sites_100, n * SITE_SIZE + 0x19, 0x8000);
    for (n = 0; n < 0x46; n++)
        SETW(carriers, n * SITE_SIZE + 0x19, 0x8000);
    for (i = 1; i < bytes; i += 2) {
        uint16_t ground = NW(ww1gnds_dat, pb(map, (uint16_t)(i - 1)) * GROUND_SIZE);
        uint16_t side = ground & 2 ? 2 : ground & 1 ? 1 : 0;
        uint8_t unit, kind;

        if (ground & 0x400) {
            n = NB(site_totals, 0);
            SETB(site_totals, 0, n + 1);
            site_add_400(n, (uint16_t)(i - 1), side);
            if (side != 2)
                inc(S_sides, (uint16_t)(A_sides + side * SIDE_SIZE + 3), 1);
        } else if (ground & 0x100) {
            n = NB(site_totals, 1);
            SETB(site_totals, 1, n + 1);
            site_add_100(n, (uint16_t)(i - 1), side);
            if (side != 2)
                inc(S_sides, (uint16_t)(A_sides + side * SIDE_SIZE + 4), 1);
        } else if (ground & 0x40)
            site_add_40((uint16_t)(i - 1), side);
        unit = pb(map, (uint16_t)i);
        if (unit >= 0xF4) {
            pwb(map, (uint16_t)i, 0xFF);
            continue;
        }
        kind = (uint8_t)(unit >> 1);
        if (kind >= 0x33 || NB(unit_total, 0) > 0xF0)
            continue;
        units_put(kind, (uint16_t)i, unit & 1);
    }
    for (i = 0; i <= 0xF0; i++)
        if (!(NW(unit_flags, i * UNIT_SIZE) & 0x8000))
            pwb(map, NW(unit_pos, i * UNIT_SIZE), (uint8_t)i);
    map_copy(map, far_at(S_map2_ptr, A_map2_ptr));
}

/* shp_load (S0E56:0079) before it loads the file: bytes 16h and 17h of
 * the ten records of `sites_100` and of `sites_400` become 0 */
void shp_reset(void)
{
    uint16_t n;

    for (n = 0; n < 0x0A; n++) {
        SETB(sites_100, n * SITE_SIZE + 0x17, 0);
        SETB(sites_100, n * SITE_SIZE + 0x16, 0);
    }
    for (n = 0; n < 0x0A; n++) {
        SETB(sites_400, n * SITE_SIZE + 0x17, 0);
        SETB(sites_400, n * SITE_SIZE + 0x16, 0);
    }
}

/* shp_load from S0E56:013A on: `file` is the map's unpacked .SHP file in
 * memory.  A byte for each of the 33h kinds (bit 0 sets or clears 2 in the
 * kind's word 0Fh), a count, then for each record: flags of its units,
 * which table (0 `sites_40`, 1 `sites_400`, 2 `sites_100`, else
 * `carriers`), the record's number, a byte for the record's bytes 16h and
 * 17h, one for byte 18h, and seven kinds (above F0h: none).  Each unit is
 * made on the record's field with 4000h in unit_flags and stands in the
 * same place of both rows of the record. */
void shp_setup(Far file)
{
    uint16_t p = 0;
    int16_t k, count, r, i;

    for (k = 0; k < 0x33; k++) {
        uint16_t w = NW(unit_dat, k * KIND_SIZE + 0x0F);
        SETW(unit_dat, k * KIND_SIZE + 0x0F, pb(file, p++) & 1 ? w | 2 : w & 0xFFFD);
    }
    count = pb(file, p++);
    if (count == 0)
        return;
    for (r = 0; count > r; r++) {
        uint8_t flags = pb(file, p), table = pb(file, (uint16_t)(p + 1));
        uint8_t rec = pb(file, (uint16_t)(p + 2));
        Far s;

        s.seg = S_carriers;     /* all four tables are in this segment */
        s.off = (uint16_t)((table == 0 ? A_sites_40 : table == 1 ? A_sites_400
                            : table == 2 ? A_sites_100 : A_carriers) + rec * SITE_SIZE);
        pwb(s, 0x17, pb(file, (uint16_t)(p + 3)));
        pwb(s, 0x16, pb(file, (uint16_t)(p + 3)));
        pwb(s, 0x18, pb(file, (uint16_t)(p + 4)));
        p = (uint16_t)(p + 5);
        for (i = 0; i < 7; i++, p++) {
            uint8_t n;

            if (pb(file, p) > 0xF0)
                continue;
            n = NB(unit_total, 0);
            SETB(unit_total, 0, n + 1);
            unit_add(n, pb(file, p), pw(s, 0x0E), flags);
            SETW(unit_flags, n * UNIT_SIZE, NW(unit_flags, n * UNIT_SIZE) | 0x4000);
            if (!(flags & 1) && !(flags & 2))
                inc(S_sides, (uint16_t)(A_sides + 2), 1);
            else if (flags & 1)
                inc(S_sides, (uint16_t)(A_sides + SIDE_SIZE + 2), 1);
            pwb(s, (uint16_t)(7 + i), n);
            pwb(s, (uint16_t)i, n);
        }
    }
}
