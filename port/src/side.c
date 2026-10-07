/* side.c - what a side does inside a site or a carrier in its attack half
 * and that draws nothing: HL.EXE's build_list (S131D:14D4) and, from
 * game_loop, the repairing (S066A:3156 to 32A9) and the building of a
 * unit (S066A:387A to 3A42) and the moving out of a unit (S066A:2F52 to
 * 3146), and the view's states 1 and 0 (field_menu, S0CB6:0EAA, and
 * view_step, S0CB6:0750).  The texts the original
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

/* S331F:17AE map_cur, the map moves are made on: names.h */

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

/* S0CB6:0445 (view_copy): the view's bytes 0 to 27h to the copy at
 * S331F:4D61 (back from it with `back`) */
void view_copy(Far view, int back)
{
    uint16_t i, s = S_views, o = 0x4D61;

    for (i = 0; i < 0x28; i++)
        if (back)
            pwb(view, i, frb(s, (uint16_t)(o + i)));
        else
            fwb(s, (uint16_t)(o + i), pb(view, i));
}

/* S066A:2F52 to 3146: the unit on the cursor's place of the site or
 * carrier at view+24h leaves it, in the move half: the place becomes FFh,
 * the unit's place on side `side`'s map (word 0Bh, 0Dh for the second
 * side) the record's field (word 0Eh, one less with 20h in its word 19h,
 * a carrier) plus 1, the view is kept (view_copy) and put on the field,
 * move_range marks where the unit can go (range 4 with 10h in its word 6,
 * else 8; mask 6 with 1 in unit_flags, else 3, with 18h more for 4 in word
 * 6) in the side's work buffer on the map at S331F:17AE, the unit is put
 * on that map, gets 100h in unit_flags and its field 40h (80h for the
 * second side) in field_bits, and the view goes to step 16h of state 0
 * with the unit in word 1Eh and pointer 4.  The drawing in between
 * (S0E56:0D44, 0F36, 145A) is left out. */
void move_out(Far view, uint16_t side)
{
    Far site = far_at(view.seg, (uint16_t)(view.off + 0x24));
    uint16_t slot = (uint16_t)(side * 7 + pw(view, 0x0E));
    uint8_t no = pb(site, slot);
    Far unit = unit_at(no), map = far_at(S_map_cur, A_map_cur), buf;
    uint16_t field = pw(site, 0x0E), mask, w = NW(map_width, 0);
    int16_t p;

    if (pw(site, 0x19) & 0x20)
        field--;
    pwb(site, slot, 0xFF);
    pww(unit, (uint16_t)(0x0B + side * 2), (uint16_t)(field + 1));
    view_copy(view, 0);
    view_to_field(side, field);
    mask = pw(unit, 4) & 1 ? 6 : 3;
    if (pw(unit, 6) & 4)
        mask |= 0x18;
    buf = far_at(S_sides, (uint16_t)(A_sides + side * 0x17 + 0x0D));
    move_range((int16_t)field, buf, no, pw(unit, 6) & 0x10 ? 4 : 8, side, mask, map);
    fwb(map.seg, (uint16_t)(map.off + field + 1), no);
    p = (int16_t)pw(unit, (uint16_t)(0x0B + side * 2)) >> 1;
    pww(unit, 4, pw(unit, 4) | 0x100);
    SETB(field_bits, (p % (int16_t)w) + (p / (int16_t)w) * 0x40,
         NB(field_bits, (p % (int16_t)w) + (p / (int16_t)w) * 0x40) | (side ? 0x80 : 0x40));
    pwb(view, 0x17, 0);
    pwb(view, 0x18, 0x16);
    pwb(view, 0x19, 3);
    pww(view, 0x1E, no);
    pwb(view, 0x1B, 4);
}

/* S0CB6:05F1 (side_input): the input word of a human side from its input
 * record `rec` (bytes 0 to 4: left, right, up, down, the button, as the
 * key scripts' keys show; each cleared here) and its view: 10h for the
 * button, 4 left or else 8 right, 1 up or else 2 down.  The directions count only when the view's
 * repeat counter (byte 2Ch) has run out or they changed from the last
 * word (word 2Eh); then the counter is set from the delays at 28h by the
 * stage in byte 2Bh, which goes up to 2 (byte 2Dh holds it back first),
 * and a change of the directions starts again at stage 0. */
uint16_t side_input(Far rec, Far view)
{
    uint16_t cx = 0, dx = 0, i;
    int fresh = 0;

    if (pb(rec, 4))
        cx |= 0x10;
    if (pb(rec, 0))
        dx |= 4;
    else if (pb(rec, 1))
        dx |= 8;
    if (pb(rec, 2))
        dx |= 1;
    else if (pb(rec, 3))
        dx |= 2;
    for (i = 0; i < 5; i++)
        pwb(rec, (uint16_t)(4 - i), 0);
    if (pb(view, 0x2C)) {
        pwb(view, 0x2C, (uint8_t)(pb(view, 0x2C) - 1));
        if (pw(view, 0x2E) != dx)
            fresh = 1;
    } else
        fresh = 1;
    if (fresh) {
        if (pw(view, 0x2E) != dx) {
            pwb(view, 0x2D, 0);
            pwb(view, 0x2B, 0);
        }
        cx |= dx;
        pwb(view, 0x2C, pb(view, (uint16_t)(0x28 + pb(view, 0x2B))));
        pww(view, 0x2E, cx);
        if (pb(view, 0x2D))
            pwb(view, 0x2D, (uint8_t)(pb(view, 0x2D) - 1));
        else if (pb(view, 0x2B) < 2)
            pwb(view, 0x2B, (uint8_t)(pb(view, 0x2B) + 1));
        else
            pwb(view, 0x2B, 2);
    }
    return cx;
}

/* S10AF:000F (show_message): the text with number `text` in the side's
 * message line; the drawing is left out.  What stays in the game's state:
 * game_state's word 0 gets 4 (8 for the second side) for a text, not for
 * FFFFh (which only takes the line off), until timer_fire clears it. */
void show_message(uint16_t text, uint16_t side)
{
    if (text != 0xFFFF)
        SETW(game_state, 0, NW(game_state, 0) | (side ? 8 : 4));
}

/* S0E56:18A3 (depot_places): the ten fields at depot_steps from `field`
 * (a map offset) into `neighbours`, each kept when its ground is
 * amok_dat's byte 18h or its four fields of foot_steps lie on the map
 * with a ground with 8000h and no unit, else FFFFh; the number kept */
static uint16_t depot_places(uint16_t field, Far map)
{
    int16_t w = (int16_t)NW(map_width, 0), p = (int16_t)field >> 1;
    int16_t col = p % w, row = p / w, i, j;
    uint16_t kept = 0, steps = (uint16_t)(A_depot_steps + (col & 1 ? 0x14 : 0));

    for (i = 0; i < 10; i++) {
        int16_t c = (int16_t)(col + (int8_t)frb(S_depot_steps, (uint16_t)(steps + i)));
        int16_t r = (int16_t)(row + (int8_t)frb(S_depot_steps, (uint16_t)(steps + 10 + i)));
        uint16_t place = (uint16_t)(c * 2 + w * 2 * r), foot = (uint16_t)(A_foot_steps + (c & 1 ? 8 : 0));
        uint8_t n = 0;

        SETW(neighbours, 2 * i, place);
        if (frb(map.seg, (uint16_t)(map.off + place)) == NB(amok_dat, 0x18))
            n = 4;
        else
            for (j = 0; j < 4; j++) {
                int16_t cc = (int16_t)(c + (int8_t)frb(S_foot_steps, (uint16_t)(foot + j)));
                int16_t rr = (int16_t)(r + (int8_t)frb(S_foot_steps, (uint16_t)(foot + 4 + j)));
                uint16_t q;
                if (cc < 0 || cc >= w || rr < 0 || (int16_t)NW(map_height, 0) <= rr)
                    continue;
                q = (uint16_t)(cc * 2 + w * 2 * rr);
                if ((NW(ww1gnds_dat, frb(map.seg, (uint16_t)(map.off + q)) * 6) & 0x8000)
                    && frb(map.seg, (uint16_t)(map.off + q + 1)) > 0xF0)
                    n++;
            }
        if (n != 4)
            SETW(neighbours, 2 * i, 0xFFFF);
        else
            kept++;
    }
    return kept;
}

/* a field (a map offset) marked in field_bits with `bits` */
static void field_mark(uint16_t field, uint8_t bits)
{
    int16_t p = (int16_t)field >> 1, w = (int16_t)NW(map_width, 0);
    uint16_t i = (uint16_t)((p % w) + (p / w) * 0x40);

    SETB(field_bits, i, NB(field_bits, i) | bits);
}

/* what field_menu does when it shows a refusal: the text, and 8 (10h for
 * the second side) in game_state's word 4 */
static void menu_refuse(uint16_t text, uint16_t side)
{
    SETW(game_state, 4, NW(game_state, 4) | (side ? 0x10 : 8));
    show_message(text, side);
}

/* the view goes to the unit's attack: step 13h of state 0, pointer 0, the
 * unit in word 1Eh and marked 100h (marks_draw, drawing, left out) */
static void menu_attack(Far view, uint16_t no, uint16_t side)
{
    pwb(view, 0x17, 0);
    pwb(view, 0x18, 0x13);
    pww(view, 0x1E, no);
    pwb(view, 0x1B, 0);
    unit_mark((uint8_t)no, 0x100, side, 1);
}

/* S0CB6:0EAA (field_menu): state 1 of a view, the choice on the cursor's
 * field (word 0) of the side's map.  On entry (step 1) word 1Ch gets what
 * may be chosen and the step becomes 2: 2, 4 and 8; 200h without 80h in
 * game_state's word 4; on a unit of the side (same_side) without 2 and
 * 200h in unit_flags 10h in the move half (byte 16h 1, and not 80h) or
 * else 1; 400h for a carrier (1000h or 2000h; with 8 in game_state's word
 * 2 only for one with same_side the other way round and without 2) or a
 * ground with 80h, 200h or 40h (with 8, only one with 1 in its word 0 for
 * the second side, 4 for the first).  While the button (10h in `bits`) is
 * held: up picks pointer 4 (with 10h) or 0 (with 1) but, when the side's
 * attackers (byte 15h of `sides`) have reached byte 16h and its word 0
 * has no 2, 5 with text 53h; down 2, left 0Ah (400h) or 9 (200h), right
 * 3, none 1.  When it is let go the pointer acts:
 * 4: move_range from the unit's place (the first half of a unit of two)
 * with its byte 0 as range; above 1 marks the unit (unit_mark 100h) and
 * goes to step 16h of state 0, else marks_erase and text 54h back to step
 * 0 with pointer 5; game_state's word 4 gets 100h and site_taken 0 or 1.
 * 2: state 4.  0: an attack (step 0 of state 0, pointer 5, unless marks
 * are found, then menu_attack): a unit with 4000h in word 6 by
 * depot_places (text 4Ch when its byte 0Ah is not above 0, 4Bh for none),
 * one with 2000h by the six neighbours whose ground has 8000h (6Fh), else
 * attack_range with the kind's bytes 9 and 8 (4Dh); 100h and site_taken
 * as for 4.  0Ah: state 6, 9: state 5, 3: state 3 with step 4 and pointer
 * 0Bh (6 unless the word at S331F:0008 is 1); anything else step 0 of
 * state 0 with pointer 5.  The drawing is left out. */
void field_menu(Far view, uint16_t side, uint16_t bits)
{
    Far map = side ? far_at(S_map2_ptr, A_map2_ptr) : far_at(S_map_ptr, A_map_ptr);
    uint16_t field = pw(view, 0), s = (uint16_t)(A_sides + side * 0x17);
    uint8_t ground = frb(map.seg, (uint16_t)(map.off + field));
    uint16_t no = frb(map.seg, (uint16_t)(map.off + field + 1));
    Far buf = far_at(S_sides, (uint16_t)(s + 0x0D));

    if (pb(view, 0x18) == 1) {
        pww(view, 0x1C, 0x0E);
        if (!(NW(game_state, 4) & 0x80))
            pww(view, 0x1C, pw(view, 0x1C) | 0x200);
        if (no <= 0xF0) {
            uint16_t f = NW(unit_flags, no * UNIT_SIZE);
            if (same_side(side, f) && !(f & 2) && !(f & 0x200)) {
                if (pb(view, 0x16) & 1) {
                    if (!(NW(game_state, 4) & 0x80))
                        pww(view, 0x1C, pw(view, 0x1C) | 0x10);
                } else
                    pww(view, 0x1C, pw(view, 0x1C) | 1);
            }
            if ((f & 0x1000) || (f & 0x2000)) {
                if (!(NW(game_state, 2) & 8) || (same_side(f, side) && !(f & 2)))
                    pww(view, 0x1C, pw(view, 0x1C) | 0x400);
            }
        } else {
            uint16_t g = NW(ww1gnds_dat, ground * 6);
            if ((g & 0x80) || (g & 0x200) || (g & 0x40)) {
                if (!(NW(game_state, 2) & 8) || (g & (side & 1 ? 1 : 4)))
                    pww(view, 0x1C, pw(view, 0x1C) | 0x400);
            }
        }
        pwb(view, 0x18, 2);
    }
    if (bits & 0x10) {
        uint16_t can = pw(view, 0x1C);
        if (bits & 1) {
            if (can & 0x10)
                pwb(view, 0x1B, 4);
            else if (can & 1)
                pwb(view, 0x1B, 0);
            if (frb(S_sides, (uint16_t)(s + 0x15)) >= frb(S_sides, (uint16_t)(s + 0x16))
                && !(frw(S_sides, s) & 2)) {
                pwb(view, 0x1B, 5);
                show_message(0x53, side);
                SETW(game_state, 4, NW(game_state, 4) | (side ? 0x10 : 8));
            }
        } else if ((bits & 2) && (can & 4))
            pwb(view, 0x1B, 2);
        else if (bits & 4) {
            if (can & 0x400)
                pwb(view, 0x1B, 0x0A);
            else if (can & 0x200)
                pwb(view, 0x1B, 9);
        } else if ((bits & 8) && (can & 8))
            pwb(view, 0x1B, 3);
        else
            pwb(view, 0x1B, 1);
        return;
    }
    switch (pb(view, 0x1B)) {
    case 4: {
        uint16_t mover = no, mask, taken;
        Far unit = unit_at((uint8_t)mover);
        if ((pw(unit, 4) & 0x40) && !(pw(unit, 4) & 0x80))
            unit = unit_at((uint8_t)--mover);
        mask = pw(unit, 6) & 1 ? 0xFFFF : pw(unit, 4) & 1 ? 6 : 3;
        if (move_range((int16_t)(pw(unit, (uint16_t)(0x0B + side * 2)) - 1), buf, mover, pb(unit, 0),
                       side, mask, map) > 1) {
            unit_mark((uint8_t)no, 0x100, side, 1);
            taken = 0;
            pwb(view, 0x17, 0);
            pwb(view, 0x18, 0x16);
            pww(view, 0x1E, mover);
        } else {
            field_bits_clear(side ? 2 : 1); /* marks_erase, then again */
            menu_refuse(0x54, side);
            taken = 1;
            pwb(view, 0x17, 0);
            pwb(view, 0x18, 0);
            pwb(view, 0x1B, 5);
        }
        SETW(game_state, 4, NW(game_state, 4) | 0x100);
        SETW(site_taken, 0, taken);
        break;
    }
    case 2:
        pwb(view, 0x17, 4);
        pwb(view, 0x18, 0);
        pwb(view, 0x1B, 5);
        break;
    case 0: {
        Far unit = unit_at((uint8_t)no);
        uint16_t taken = 1, place = (uint16_t)(pw(unit, (uint16_t)(0x0B + side * 2)) - 1), i;
        uint8_t mark = side ? 8 : 4;
        pwb(view, 0x17, 0);
        pwb(view, 0x18, 0);
        pwb(view, 0x1B, 5);
        if (pw(unit, 6) & 0x4000) {
            if ((int8_t)pb(unit, 0x0A) <= 0)
                menu_refuse(0x4C, side);
            else if (depot_places(place, map)) {
                for (i = 0; i < 10; i++)
                    if ((int16_t)NW(neighbours, 2 * i) >= 0)
                        field_mark(NW(neighbours, 2 * i), mark);
                menu_attack(view, no, side);
                taken = 0;
            } else
                menu_refuse(0x4B, side);
        } else if (pw(unit, 6) & 0x2000) {
            int16_t n = 0;
            field_neighbours(place);
            for (i = 0; i < 6; i++) {
                int16_t q = (int16_t)NW(neighbours, 2 * i);
                if (q < 0 || !(NW(ww1gnds_dat, frb(map.seg, (uint16_t)(map.off + q)) * 6) & 0x8000))
                    continue;
                n++;
                field_mark((uint16_t)q, mark);
            }
            if (n > 0) {
                menu_attack(view, no, side);
                taken = 0;
            } else
                menu_refuse(0x6F, side);
        } else {
            uint8_t found = 0, kind = pb(unit, 8), r;
            uint16_t w6 = frw(S_unit_dat, (uint16_t)KIND(kind, 6));
            r = frb(S_unit_dat, (uint16_t)KIND(kind, 9));
            if (r > 1)
                found = (uint8_t)(found + attack_range(buf, (int16_t)field, r, side, no, w6 & 0xFFEF, map));
            r = frb(S_unit_dat, (uint16_t)KIND(kind, 8));
            if (r > 1)
                found = (uint8_t)(found + attack_range(buf, (int16_t)field, r, side, no, w6 & 0xFFD3, map));
            if (found) {
                menu_attack(view, no, side);
                taken = 0;
            } else
                menu_refuse(0x4D, side);
        }
        SETW(game_state, 4, NW(game_state, 4) | 0x100);
        SETW(site_taken, 0, taken);
        break;
    }
    case 0x0A:
        pwb(view, 0x17, 6);
        pwb(view, 0x18, 0);
        break;
    case 9:
        pwb(view, 0x17, 5);
        pwb(view, 0x18, 0);
        break;
    case 3:
        pwb(view, 0x17, 3);
        pwb(view, 0x18, 4);
        pwb(view, 0x1B, frw(HL_S331F + HL_BASE, 8) == 1 ? 0x0B : 6);
        break;
    default:
        pwb(view, 0x17, 0);
        pwb(view, 0x18, 0);
        pwb(view, 0x1B, 5);
    }
}

/* the second press of a step entered with 400h in game_state's word 4
 * (byte 5 of the input record, or 8 in the side's word of `sides`, which
 * is cleared): whether it came */
static int second_press(Far rec, uint16_t side)
{
    uint16_t s = (uint16_t)(A_sides + side * 0x17);

    if (!pb(rec, 5) && !(frw(S_sides, s) & 8))
        return 0;
    fww(S_sides, s, frw(S_sides, s) & 0xFFF7);
    return 1;
}

/* a step that waits for a second press: 400h in game_state's word 4 and a
 * timer of kind 7 over 4 passes */
static void second_wait(void)
{
    SETW(game_state, 4, NW(game_state, 4) | 0x400);
    timer_set(7, 4, 0);
}

/* S0CB6:0750 (view_step): state 0 of a view (byte 17h), by its step (byte
 * 18h), with the input record `rec` (byte 8 a press, byte 5 a second
 * press), the input bits of side_input and the side's map.  A press acts
 * only when byte 30h of the view says the button was let go before (a
 * press with 30h 0 only sets it).
 * 16h: move_aim; its result goes to 30h; with 10h (a refusal) the view
 *   waits in 17h (second_wait), else goes to 3 with 30h 0.
 * 17h: with 400h a second press gives the move up (move_done with 10h);
 *   without (the timer ran out) back to 16h.
 * 3: move_start; with 10h in its result the view waits in 18h, else goes
 *   to step 19h with pointer 5 and move_done with the result.
 * 18h: with 400h a second press gives the move up; without, move_range
 *   again with the arguments it kept at S3306:000A (the drawing left out),
 *   100h in game_state's word 4, site_taken 0, back to 16h.
 * 13h: attack_order; given, step 19h with pointer 5 and attack_unmark;
 *   else the view waits in 1Ah.
 * 1Ah: with 400h a second press gives the attack up (step 19h, pointer 5,
 *   100h, site_taken 1, attack_unmark); without back to 13h.
 * 19h: a press (byte 8) is taken and the step becomes 0.
 * Else (0) the button takes the side's text away (4, 8 for the second
 * side, leaves game_state's word 0) and goes to state 1, step 1.
 * Byte 8 of the record is cleared after each of the steps 16h to 1Ah that
 * acted.  Returns 0 with the button in `bits`, else 1. */
uint8_t view_step(Far view, Far rec, uint16_t bits, Far map, uint16_t side)
{
    uint8_t step = pb(view, 0x18), r;
    int press = (bits & 0x10) != 0;

    if (step == 0x16 || step == 3 || step == 0x13) {
        if (pb(rec, 8) && !pb(view, 0x30)) {
            pwb(view, 0x30, 1);
            return !press;
        }
        if (!press || !pb(view, 0x30))
            return !press;
    }
    switch (step) {
    case 0x16:
        r = (uint8_t)move_aim(view, map, side);
        pwb(view, 0x30, r);
        if (r & 0x10) {
            second_wait();
            pwb(view, 0x18, 0x17);
        } else {
            pwb(view, 0x18, 3);
            pwb(view, 0x30, 0);
        }
        pwb(rec, 8, 0);
        break;
    case 0x17:
    case 0x18:
        if (NW(game_state, 4) & 0x400) {
            if (second_press(rec, side)) {
                pwb(view, 0x30, 0x10);
                move_done(view, map, side, 0x10);
                pwb(view, 0x30, 0);
                pwb(rec, 5, 0);
            }
        } else if (step == 0x17)
            pwb(view, 0x18, 0x16);
        else {
            Far buf, m;
            uint16_t a = A_move_args;
            buf.off = frw(S_move_args, (uint16_t)(a + 2));
            buf.seg = frw(S_move_args, (uint16_t)(a + 4));
            m.off = frw(S_move_args, (uint16_t)(a + 0x0E));
            m.seg = frw(S_move_args, (uint16_t)(a + 0x10));
            move_range((int16_t)frw(S_move_args, a), buf, frw(S_move_args, (uint16_t)(a + 6)),
                       (int16_t)frw(S_move_args, (uint16_t)(a + 8)), frw(S_move_args, (uint16_t)(a + 0x0A)),
                       frw(S_move_args, (uint16_t)(a + 0x0C)), m);
            SETW(game_state, 4, NW(game_state, 4) | 0x100);
            SETW(site_taken, 0, 0);
            pwb(view, 0x30, 0);
            pwb(view, 0x18, 0x16);
        }
        pwb(rec, 8, 0);
        break;
    case 3:
        r = (uint8_t)move_start(view, side);
        pwb(view, 0x30, r);
        if (r & 0x10) {
            second_wait();
            pwb(rec, 5, 0);
            pwb(view, 0x18, 0x18);
        } else {
            pwb(view, 0x17, 0);
            pwb(view, 0x18, 0x19);
            pwb(view, 0x1B, 5);
            move_done(view, map, side, (uint16_t)(int8_t)r);
            pwb(view, 0x30, 0);
        }
        pwb(rec, 8, 0);
        break;
    case 0x13:
        r = (uint8_t)attack_order(view, map, side);
        pwb(view, 0x30, r);
        if (r) {
            pwb(view, 0x17, 0);
            pwb(view, 0x18, 0x19);
            pwb(view, 0x1B, 5);
            attack_unmark(view, side);
        } else {
            pwb(view, 0x18, 0x1A);
            pwb(rec, 5, 0);
            second_wait();
        }
        pwb(view, 0x30, 0);
        pwb(rec, 8, 0);
        break;
    case 0x1A:
        if (!(NW(game_state, 4) & 0x400))
            pwb(view, 0x18, 0x13);
        else if (second_press(rec, side)) {
            pwb(view, 0x17, 0);
            pwb(view, 0x18, 0x19);
            pwb(view, 0x1B, 5);
            SETW(game_state, 4, NW(game_state, 4) | 0x100);
            SETW(site_taken, 0, 1);
            pwb(view, 0x30, 0);
            pwb(rec, 5, 0);
            attack_unmark(view, side);
        }
        pwb(rec, 8, 0);
        break;
    case 0x19:
        if (pb(rec, 8)) {
            pwb(rec, 8, 0);
            pwb(view, 0x18, 0);
        }
        break;
    default:
        if (press) {
            show_message(0xFFFF, side);
            SETW(game_state, 0, NW(game_state, 0) & (side ? 0xFFF7 : 0xFFFB));
            pwb(view, 0x17, 1);
            pwb(view, 0x18, 1);
        }
    }
    return !press;
}
