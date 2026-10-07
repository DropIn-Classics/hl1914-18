/* side.c - what a side does inside a site or a carrier in its attack half
 * and that draws nothing: HL.EXE's build_list (S131D:14D4) and the
 * repairing in game_loop (S066A:3156 to 32A9).  The texts the original
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
