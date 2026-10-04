/* units.c - taking units off: HL.EXE's unit_destroy (S02E6:208E),
 * unit_free (S15C5:0594) and site_free (S15C5:06EA), translated over the
 * program's memory.  The original's check of the stack at each entry is
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
