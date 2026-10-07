/* side.c - what a side does inside a site or a carrier in its attack half
 * and that draws nothing: HL.EXE's build_list (S131D:14D4) and, from
 * game_loop, the repairing (S066A:3156 to 32A9) and the building of a
 * unit (S066A:387A to 3A42).  The texts the original
 * then shows are returned, not shown.  port/README.md says how this was
 * compared with the original.
 */
#include "hl.h"

static Far unit_at(uint8_t unit)
{
    Far p;
    p.seg = S_units;
    p.off = (uint16_t)(A_units + unit * UNIT_SIZE);
    return p;
}

static void pww(Far p, uint16_t o, uint16_t v)
{
    fww(p.seg, (uint16_t)(p.off + o), v);
}

#define KIND(k, o) (A_unit_dat + (k) * KIND_SIZE + (o))

/* S131D:14D4 (build_list): build_kinds gets the kinds side `side` may
 * build with `points`, build_count their number: none without a free unit
 * record; else each of the 33h kinds whose price (byte 3Fh) is at most the
 * points, without 2 in its word 0Fh, with 1000h in word 0Dh only while a
 * record of `carriers` is free, and not the kinds below 0Ah for the second
 * side nor those above 23h for the first */
void build_list(uint16_t points, uint16_t side)
{
    unsigned k;

    for (k = 0; k < 0x33; k++)
        SETB(build_kinds, k, 0xFF);
    SETB(build_count, 0, 0);
    if (unit_find(0x8000) == 0xFF)
        return;
    for (k = 0; k < 0x33; k++) {
        int ok = frb(S_unit_dat, (uint16_t)KIND(k, 0x3F)) <= points
                 && !(frw(S_unit_dat, (uint16_t)KIND(k, 0x0F)) & 2);
        if ((frw(S_unit_dat, (uint16_t)KIND(k, 0x0D)) & 0x1000) && (int16_t)carrier_find(0x8000) < 0)
            ok = 0;
        if (k < 0x0A && side == 1)
            ok = 0;
        if (k > 0x23 && side == 0)
            ok = 0;
        if (ok) {
            SETB(build_kinds, NB(build_count, 0), k);
            SETB(build_count, 0, NB(build_count, 0) + 1);
        }
    }
}

/* S066A:3156 to 32A9: the unit on the cursor's place of the site or
 * carrier at view+24h (row side*7, place in byte 0Eh of the view) is
 * repaired.  It costs 0Fh of side_points, 32h for a unit with 4000h in
 * word 6 (text 4Ah when the side has less); a unit whose count already is
 * its kind's byte 3 gets text 50h (one with 4000h only while its byte 0Ah
 * is 2).  Else the points are paid, build_list made anew, 0A00h set in
 * unit_flags, the count made full, byte 0Ah 2 with 4000h: text 51h.
 * What follows in the original (the text shown, S331F:4CA8 and the view's
 * steps) is the caller's. */
uint16_t repair(Far view, uint16_t side)
{
    Far rec = far_at(view.seg, (uint16_t)(view.off + 0x24));
    uint8_t no = frb(rec.seg, (uint16_t)(rec.off + side * 7 + pw(view, 0x0E)));
    Far unit = unit_at(no);
    uint16_t cost = pw(unit, 6) & 0x4000 ? 0x32 : 0x0F;
    uint8_t full = frb(S_unit_dat, (uint16_t)KIND(pb(unit, 8), 3));

    if (cost > NW(side_points, side * 2))
        return 0x4A;
    if (pw(unit, 6) & 0x4000 ? pb(unit, 0x0A) == 2 && full == pb(unit, 2) : full == pb(unit, 2))
        return 0x50;
    SETW(side_points, side * 2, NW(side_points, side * 2) - cost);
    build_list(NW(side_points, side * 2), side);
    pww(unit, 4, pw(unit, 4) | 0x0A00);
    pwb(unit, 2, full);
    if (pw(unit, 6) & 0x4000)
        pwb(unit, 0x0A, 2);
    return 0x51;
}

/* S066A:387A to 3A42: the kind at build_kinds[word 0Eh + word 20h of the
 * view] (the list's cursor and its first shown entry) is built on the free
 * place in word 0Ah of the view of the site at view+24h: a free unit
 * record (unit_find(8000h); the list is empty without one) is made by
 * unit_add on the site's field for the site's side (bit 0 of its word
 * 19h), gets 4600h in unit_flags, its two records of 9 bytes at
 * S39B6:09AB and 0132 are emptied, it goes into the side's row of the
 * site, unit_total and the site side's byte 2 of `sides` go up, the price
 * (the kind's byte 3Fh) is taken from side `side`'s points, build_list is
 * made anew, 100h is set in game_state's word 4 and its word 6 becomes 0,
 * and the view goes to step 0Dh. */
void build_unit(Far view, uint16_t side)
{
    Far site = far_at(view.seg, (uint16_t)(view.off + 0x24));
    uint16_t owner = pw(site, 0x19) & 1, i;
    uint8_t no = unit_find(0x8000);
    uint8_t kind = NB(build_kinds, pw(view, 0x0E) + pw(view, 0x20));
    static const uint16_t rec[2] = { 0x09AB, 0x0132 };

    unit_add(no, kind, pw(site, 0x0E), owner);
    SETW(unit_flags, no * UNIT_SIZE, NW(unit_flags, no * UNIT_SIZE) | 0x4600);
    for (i = 0; i < 2; i++) {
        uint16_t s = HL_S39B6 + HL_BASE, o = (uint16_t)(rec[i] + no * 9);
        fww(s, (uint16_t)(o + 4), 0xFFFF);
        fwb(s, (uint16_t)(o + 8), 0);
        fww(s, o, 0);
        fww(s, (uint16_t)(o + 2), 0);
        fww(s, (uint16_t)(o + 6), 0);
    }
    pwb(site, (uint16_t)(side * 7 + pw(view, 0x0A)), no);
    SETB(unit_total, 0, NB(unit_total, 0) + 1);
    SETB(sides, owner * 0x17 + 2, NB(sides, owner * 0x17 + 2) + 1);
    kind = NB(build_kinds, pw(view, 0x0E) + pw(view, 0x20));
    SETW(side_points, side * 2, NW(side_points, side * 2) - frb(S_unit_dat, (uint16_t)KIND(kind, 0x3F)));
    build_list(NW(side_points, side * 2), side);
    SETW(game_state, 4, NW(game_state, 4) | 0x100);
    SETW(game_state, 6, 0);
    pwb(view, 0x18, 0x0D);
}
