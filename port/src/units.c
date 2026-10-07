/* units.c - units and sites made and taken off: HL.EXE's unit_add
 * (S15C5:0005), carrier_find (S15C5:01F7), the three site_add routines,
 * carrier_add (S15C5:04AE), unit_destroy (S02E6:208E), unit_free
 * (S15C5:0594) and site_free (S15C5:06EA), translated over the program's
 * memory.  The original's check of the stack at each entry is
 * left out.  port/README.md says how this was compared with the original.
 */
#include "hl.h"

#define SITE_SIZE 0x1Cu     /* a record of `carriers` and the like */
#define SIDE_SIZE 0x17u     /* a record of `sides` */
#define BLAST_SIZE 7u       /* a record of `blasts` */

/* one half of unit_destroy: notes the unit's field in `blasts` at `at` and
 * writes `put` as the field's unit */
static void blast(uint8_t unit, uint8_t put, uint16_t at, uint8_t time, uint16_t side, Far map)
{
    uint16_t field = (uint16_t)(NW(unit_pos, unit * UNIT_SIZE + 2 * side) - 1);

    SETW(blasts, at, field);
    SETB(blasts, at + 2, time);
    SETW(blasts, at + 3, pb(map, field));
    pwb(map, (uint16_t)(field + 1), put);
    SETW(blasts, at + 5, put);
    SETB(unit_marks, unit * UNIT_SIZE, NB(unit_marks, unit * UNIT_SIZE) | 4);
}

/* S02E6:208E: a unit with nothing left.  Its field gets `put` as its unit
 * byte and is noted in the record `slot` of `blasts`; the unit is marked
 * (4 in unit_marks) for unit_free.  The other half of a two-part unit
 * likewise with `put2`, in the following record and one tick later. */
void unit_destroy(uint8_t unit, uint8_t put, uint8_t put2, int8_t slot, uint16_t side, Far map)
{
    uint16_t at = (uint16_t)(slot * (int)BLAST_SIZE);
    uint16_t flags;

    blast(unit, put, at, NB(blast_time, 0), side, map);
    flags = NW(unit_flags, unit * UNIT_SIZE);
    if (!(flags & 0x40))
        return;
    unit = (uint8_t)(flags & 0x80 ? unit + 1 : unit - 1);
    blast(unit, put2, (uint16_t)(at + BLAST_SIZE), (uint8_t)(NB(blast_time, 0) + 1), side, map);
}

/* S15C5:06EA: gives up a 1Ch-byte record and the units of its row `row` */
void site_free(Far site, uint16_t row)
{
    uint16_t side = row, what;
    uint8_t i;

    for (i = 0; i < 7; i++) {
        uint16_t place = (uint16_t)(row * 7 + i);
        if (pb(site, place) <= 0xF0) {
            unit_free(pb(site, place), row);
            pwb(site, place, 0xFF);
        }
    }
    what = pw(site, 0x19);
    if (what & 1)
        side = 1;
    else if (!(what & 2))
        side = 0;
    if (what & 0x10) {
        SETB(sides, side * SIDE_SIZE + 4, NB(sides, side * SIDE_SIZE + 4) - 1);
        SETB(site_totals, 1, NB(site_totals, 1) - 1);
    } else if (what & 8) {
        SETB(sides, side * SIDE_SIZE + 3, NB(sides, side * SIDE_SIZE + 3) - 1);
        SETB(site_totals, 0, NB(site_totals, 0) - 1);
    } else if (what & 0x20)
        SETB(site_totals, 2, NB(site_totals, 2) - 1);
    pwb(site, 0x19, 0x00);
    pwb(site, 0x1A, 0x80);
}

/* S15C5:0594: makes a unit's record (and its other half's) unused, with
 * what a carrier holds, and lowers the counts of units. */
void unit_free(uint8_t unit, uint16_t row)
{
    uint16_t flags;
    uint8_t n = 1;

    SETB(free_depth, 0, NB(free_depth, 0) + 1);
    /* both early returns leave free_depth raised, as the original does */
    if ((int8_t)NB(free_depth, 0) > 5)
        return;
    flags = NW(unit_flags, unit * UNIT_SIZE);
    if (flags & 0x8000)
        return;
    if (flags & 0x40) {
        uint8_t other;
        if (flags & 0x80)
            other = (uint8_t)(unit + 1);
        else
            other = unit--;
        n = 2;
        SETW(unit_flags, other * UNIT_SIZE, 0x8000);
    }
    flags = NW(unit_flags, unit * UNIT_SIZE);
    if (flags & 0x1000) {
        Far site;
        site.seg = S_carriers;
        site.off = (uint16_t)(A_carriers + (int8_t)NB(units, unit * UNIT_SIZE + 0x0A) * (int)SITE_SIZE);
        site_free(site, row);
    }
    SETB(unit_total, 0, NB(unit_total, 0) - n);
    flags = NW(unit_flags, unit * UNIT_SIZE);
    if (flags & 1)
        SETB(sides, SIDE_SIZE + 2, NB(sides, SIDE_SIZE + 2) - n);
    else if (!(flags & 2))
        SETB(sides, 2, NB(sides, 2) - n);
    SETW(unit_flags, unit * UNIT_SIZE, 0x8000);
    SETB(free_depth, 0, NB(free_depth, 0) - 1);
}

/* the 14 places of a 1Ch-byte record, all empty */
static void site_empty(Far site)
{
    uint16_t i;

    for (i = 0; i < 14; i++)
        pwb(site, i, 0xFF);
}

static void pww(Far p, uint16_t o, uint16_t v)
{
    pwb(p, o, (uint8_t)v);
    pwb(p, (uint16_t)(o + 1), (uint8_t)(v >> 8));
}

static Far site_at(uint16_t seg, uint16_t table, uint16_t n)
{
    Far p;

    p.seg = seg;
    p.off = (uint16_t)(table + n * SITE_SIZE);
    return p;
}

/* S15C5:01A7: the first of the units 0 to F0h whose unit_flags have a bit
 * of `mask` (FFh for none); with 8000h a free record */
uint8_t unit_find(uint16_t mask)
{
    unsigned u;

    for (u = 0; u <= 0xF0; u++)
        if (NW(unit_flags, u * UNIT_SIZE) & mask)
            return (uint8_t)u;
    return 0xFF;
}

/* S15C5:01F7: the first of the 70 records of `carriers` whose flags have a
 * bit of `mask`; FFFFh when there is none */
uint16_t carrier_find(uint16_t mask)
{
    uint8_t i;

    for (i = 0; i < 0x46; i++)
        if (NW(carriers, i * SITE_SIZE + 0x19) & mask)
            return i;
    return 0xFFFF;
}

/* S15C5:024A: record `n` of `sites_400` for the field at map offset
 * `field`; its four fields are the field, the one two columns to the left
 * and the two between them */
void site_add_400(uint16_t n, uint16_t field, uint16_t flags)
{
    Far s = site_at(S_sites_400, A_sites_400, n);
    uint16_t row = (uint16_t)(NW(map_width, 0) << 1);
    int odd = ((int16_t)field >> 1) & 1;

    pww(s, 0x0E, field);
    pww(s, 0x10, (uint16_t)(field - 4));
    pww(s, 0x12, (uint16_t)(odd ? field - 2 : field - 2 - row));
    pww(s, 0x14, (uint16_t)(odd ? field + row - 2 : field - 2));
    pww(s, 0x19, flags | 8);
    pwb(s, 0x17, 0x1E);
    pwb(s, 0x16, 0x1E);
    pwb(s, 0x1B, 0xFF);
    pwb(s, 0x18, 0x0F);
    site_empty(s);
}

/* S15C5:032C: record `n` of `sites_100`; its four fields are the field,
 * the one below and the two beside that */
void site_add_100(uint16_t n, uint16_t field, uint16_t flags)
{
    Far s = site_at(S_sites_100, A_sites_100, n);
    uint16_t row = (uint16_t)(NW(map_width, 0) << 1);
    int odd = ((int16_t)field >> 1) & 1;

    pww(s, 0x0E, field);
    pww(s, 0x10, (uint16_t)(odd ? field + row - 2 : field - 2));
    pww(s, 0x12, (uint16_t)(field + row));
    pww(s, 0x14, (uint16_t)(odd ? field + row + 2 : field + 2));
    pww(s, 0x19, flags | 0x10);
    pwb(s, 0x17, 0x14);
    pwb(s, 0x16, 0x14);
    pwb(s, 0x1B, 0xFF);
    pwb(s, 0x18, 0x0A);
    site_empty(s);
}

/* S15C5:040D: the record of `sites_40` of side `n`, on one field; map_load
 * also calls it with 2 for a field of no side, which is the place of the
 * first record of `sites_100` */
void site_add_40(uint16_t field, uint16_t n)
{
    Far s = site_at(S_sites_40, A_sites_40, n);

    pww(s, 0x14, field);
    pww(s, 0x12, field);
    pww(s, 0x10, field);
    pww(s, 0x0E, field);
    pwb(s, 0x17, 0x32);
    pwb(s, 0x16, 0x32);
    pww(s, 0x19, n | 4);
    pwb(s, 0x1B, 0xFF);
    site_empty(s);
    pwb(s, 0x18, 0x14);
}

/* S15C5:04AE: record `n` of `carriers` for the unit `unit` of kind `kind`
 * on `field` */
void carrier_add(int16_t n, uint16_t field, uint16_t kind, uint16_t flags, uint16_t unit)
{
    Far s = site_at(S_carriers, A_carriers, (uint16_t)n);
    uint8_t room = NB(unit_dat, kind * KIND_SIZE + 0x40);
    uint16_t own;

    pww(s, 0x14, field);
    pww(s, 0x12, field);
    pww(s, 0x10, field);
    pww(s, 0x0E, field);
    pwb(s, 0x17, room);
    pwb(s, 0x16, room);
    pww(s, 0x19, flags | 0x20);
    pwb(s, 0x1B, (uint8_t)unit);
    own = NW(unit_dat, NB(unit_kind, (uint16_t)(unit * UNIT_SIZE)) * KIND_SIZE + 0x13);
    pww(s, 0x19, pw(s, 0x19) | own);
    site_empty(s);
    pwb(s, 0x18, 0);
}

/* S15C5:0005: fills the record of unit `unit` from its kind's record in
 * `unit_dat`; `flags` (the side, in map_load) is added to unit_flags.  A
 * carrier's kind (1000h) gets the first unused record of `carriers`. */
void unit_add(uint8_t unit, uint8_t kind, uint16_t field, uint16_t flags)
{
    uint16_t u = (uint16_t)(unit * UNIT_SIZE), k = (uint16_t)(kind * KIND_SIZE);
    uint8_t first;

    SETB(units, u + 8, kind);
    SETB(units, u, NB(unit_dat, k));
    SETB(units, u + 2, NB(unit_dat, k + 3));
    SETB(units, u + 3, NB(unit_dat, k + 4));
    SETW(units, u + 6, NW(unit_dat, k + 0x11));
    SETW(units, u + 4, NW(unit_dat, k + 0x0D) | flags);
    if (!(flags & 2)) {
        /* the kind's serial number for this side, counted on by two */
        uint16_t serial = (uint16_t)(k + flags + 0x3D);
        SETB(units, u + 9, NB(unit_dat, serial));
        SETB(unit_dat, serial, NB(unit_dat, serial) + 2);
    }
    SETB(units, u + 0x13, 0);
    SETB(units, u + 0x16, 0);
    SETB(units, u + 0x19, 0);
    SETB(units, u + 0x15, 0);
    SETB(units, u + 0x14, 0);
    SETB(units, u + 1, 0);
    SETW(units, u + 0x17, 0);
    SETW(units, u + 0x11, 0);
    first = flags ? 0 : 3;
    SETB(units, u + 0x10, first);
    SETB(units, u + 0x0F, first);
    SETW(units, u + 0x0D, field);
    SETW(units, u + 0x0B, field);
    if (NW(unit_dat, k + 0x0D) & 0x1000) {
        SETB(units, u + 0x0A, carrier_find(0x8000));
        carrier_add((int8_t)NB(units, u + 0x0A), field, kind, flags, unit);
        SETB(site_totals, 2, NB(site_totals, 2) + 1);
    } else if (NW(unit_dat, k + 0x11) & 0x4000)
        SETB(units, u + 0x0A, 2);
    else
        SETB(units, u + 0x0A, 0xFF);
}
