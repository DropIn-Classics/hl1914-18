/* computer.c - the computer's player of HL.EXE (computer_turn,
 * S1C74:000B, and what it calls), translated over the program's memory.
 * So far the end of its chain: the steps of a side's list that are made
 * into a script of input codes, and the script it plays the side with.
 * port/README.md says how this was compared with the original.
 */
#include "hl.h"

#define SCRIPT_SIZE 0x20u   /* a record of `computer_script` */
#define STEPS_SIZE 0x49u    /* a record of `computer_steps` */
#define VIEW_SIZE 0x31u     /* a record of `views` */
#define SIDE_SIZE 0x17u     /* a record of `sides` */

static Far script_of(uint16_t side)
{
    Far s;
    s.seg = S_computer_script;
    s.off = (uint16_t)(A_computer_script + side * SCRIPT_SIZE);
    return s;
}

/* a code at the script's place, and the script's end behind it */
static void script_add(Far s, uint8_t code)
{
    uint8_t at = pb(s, 0x1E);

    pwb(s, 0x1E, (uint8_t)(at + 1));
    pwb(s, at, code);
    pwb(s, pb(s, 0x1E), 0xFF);
}

/* the codes of one move from column c0, row r0 towards c1, r1 (left 3 or
 * right 4, then up 1 or down 2), as cursor_scroll and cursor_step end */
static void script_towards(Far s, int16_t c0, int16_t r0, int16_t c1, int16_t r1)
{
    uint8_t h = c0 > c1 ? 3 : c0 < c1 ? 4 : 0, v = r0 > r1 ? 1 : r0 < r1 ? 2 : 0;
    uint8_t at;

    if (h) {
        at = pb(s, 0x1E);
        pwb(s, 0x1E, (uint8_t)(at + 1));
        pwb(s, at, h);
    }
    if (v) {
        at = pb(s, 0x1E);
        pwb(s, 0x1E, (uint8_t)(at + 1));
        pwb(s, at, v);
    }
    pwb(s, pb(s, 0x1E), 0xFF);
}

/* cursor_scroll (S1E4B:1464): one move of the side's view towards the one
 * that has the field at column, row some five columns and four rows in
 * (an even column and row, kept inside the map).  Returns 1 when the view
 * is there (words 0Ch and 0Eh of the view), or the field is off the map. */
static uint16_t cursor_scroll(uint16_t side, int16_t col, int16_t row)
{
    int16_t w = (int16_t)NW(map_width, 0), h = (int16_t)NW(map_height, 0);
    int16_t vc, vr;

    if (w <= col || h <= row)
        return 1;
    vc = (int16_t)NW(views, side * VIEW_SIZE + 0x0C);
    vr = (int16_t)NW(views, side * VIEW_SIZE + 0x0E);
    col = (int16_t)(col - 5);
    if (col < 0)
        col = 0;
    else if ((int16_t)(w - 10) < col)
        col = (int16_t)(w - 10);
    row = (int16_t)(row - 4);
    if (row < 0)
        row = 0;
    else if ((int16_t)(h - 8) < row)
        row = (int16_t)(h - 8);
    if (col & 1)
        col--;
    if (row & 1)
        row--;
    if (vc == col && vr == row)
        return 1;
    script_towards(script_of(side), vc, vr, col, row);
    return 0;
}

/* cursor_step (S1E4B:15E8): one move of the cursor (a map offset) towards
 * the field.  Returns 1 when it is there, or the field is off the map or
 * in its last column or row. */
static uint16_t cursor_step(uint16_t side, int16_t cursor, int16_t field)
{
    int16_t w = (int16_t)NW(map_width, 0), c0, r0, c1, r1;

    if (field < 0 || (int16_t)NW(map_bytes, 0) < field)
        return 1;
    cursor >>= 1;
    c0 = (int16_t)(cursor % w);
    r0 = (int16_t)(cursor / w);
    field >>= 1;
    c1 = (int16_t)(field % w);
    r1 = (int16_t)(field / w);
    if ((int16_t)(w - 1) <= c1 || (int16_t)(NW(map_height, 0) - 1) <= r1)
        return 1;
    if (c0 == c1 && r0 == r1)
        return 1;
    script_towards(script_of(side), c0, r0, c1, r1);
    return 0;
}

/* cursor_script (S1E4B:128E): takes the side's cursor to the field, a part
 * a call, by byte 48h of the side's steps: 0 chooses (4 at once when the
 * field is less than five columns and rows away or byte 18h of the view
 * is set), 1 the code 83h, 2 cursor_scroll until the view is there, 3 the
 * button, 4 cursor_step until the cursor is there, 5 done: returns 1. */
static uint16_t cursor_script(uint16_t side, int16_t cursor, int16_t field, Far steps)
{
    int16_t w = (int16_t)NW(map_width, 0), c0, r0, c1, r1;
    Far s = script_of(side);

    if (field < 0 || (int16_t)NW(map_bytes, 0) < field)
        return 1;
    c0 = (int16_t)((cursor >> 1) % w);
    r0 = (int16_t)((cursor >> 1) / w);
    c1 = (int16_t)((field >> 1) % w);
    r1 = (int16_t)((field >> 1) / w);
    switch (pb(steps, 0x48)) {
    case 0: {
        int16_t dc = (int16_t)(c0 - c1), dr = (int16_t)(r0 - r1);

        if (dc < 0)
            dc = (int16_t)-dc;
        if (dr < 0)
            dr = (int16_t)-dr;
        if (NB(views, side * VIEW_SIZE + 0x18) || (dc < 5 && dr < 5))
            pwb(steps, 0x48, 4);
        else
            pwb(steps, 0x48, (uint8_t)(pb(steps, 0x48) + 1));
        return 0;
    }
    case 1:
        script_add(s, 0x83);
        break;
    case 2:
        if (!cursor_scroll(side, c1, r1))
            return 0;
        break;
    case 3:
        script_add(s, 5);
        break;
    case 4:
        if (!cursor_step(side, cursor, field))
            return 0;
        break;
    default:
        pwb(steps, 0x48, 0);
        return 1;
    }
    pwb(steps, 0x48, (uint8_t)(pb(steps, 0x48) + 1));
    return 0;
}

/* where a unit is for the side, as a map offset */
static int16_t unit_field(uint8_t unit, uint16_t side)
{
    return (int16_t)(NW(unit_pos, unit * UNIT_SIZE + side * 2) - 1);
}

/* a field's place in field_bits (64 fields a row) */
static uint16_t bits_at(int16_t field)
{
    int16_t w = (int16_t)NW(map_width, 0), f = (int16_t)(field >> 1);

    return (uint16_t)(f % w + ((f / w) << 6));
}

static void side_refused(uint16_t side)
{
    SETW(sides, side * SIDE_SIZE, NW(sides, side * SIDE_SIZE) | 8);
}

#define NEXT(steps) pwb(steps, 0x47, (uint8_t)(pb(steps, 0x47) + 1))

/* step_move (S1E4B:0484), a step of kind 1: the unit moves to the field
 * `aim`, by byte 47h of the side's steps: 0 waits while 80h is set in
 * game_state's word 4, 1 the cursor to the unit, 2 the code 80h (button
 * and up), 3 the cursor towards the aim, 4 move_target for the cursor's
 * field (8 in the side's word 0 when the field is not marked for the side
 * or move_target refuses) and the button, 5 nothing, 6 the button, 7 done:
 * returns 1. */
static uint16_t step_move(uint16_t side, int16_t cursor, uint8_t unit, int16_t aim, Far steps)
{
    Far s = script_of(side);

    switch (pb(steps, 0x47)) {
    case 0:
        if (!(NW(game_state, 4) & 0x80))
            NEXT(steps);
        pwb(steps, 0x48, 0);
        return 0;
    case 1:
        if (!cursor_script(side, cursor, unit_field(unit, side), steps))
            return 0;
        break;
    case 2:
        script_add(s, 0x80);
        break;
    case 3:
        if (!cursor_step(side, cursor, aim))
            return 0;
        break;
    case 4: {
        Far map = far_at(S_map_ptr, side ? A_map2_ptr : A_map_ptr);

        SETB(move_rec, 0, unit);
        if (!(NB(field_bits, bits_at(cursor)) & (side ? 2 : 1)))
            side_refused(side);
        else if (move_target((uint16_t)cursor, side, map))
            side_refused(side);
        script_add(s, 5);
        break;
    }
    case 5:
        break;
    case 6:
        script_add(s, 5);
        break;
    default:
        pwb(steps, 0x47, 0);
        return 1;
    }
    NEXT(steps);
    return 0;
}

/* step_attack (S1E4B:06DF), a step of kind 2: the unit attacks the unit
 * `target`: 0 the cursor to the unit, 1 the code 80h, 2 the cursor towards
 * the target's field, 3 the button, and on to 5 when the field is marked
 * for the side's attack in field_bits (4, the second side 8), else 8 in
 * the side's word 0; 4 the button; 5 done: returns 1. */
static uint16_t step_attack(uint16_t side, int16_t cursor, uint8_t unit, uint8_t target, Far steps)
{
    Far s = script_of(side);

    switch (pb(steps, 0x47)) {
    case 0:
        if (!cursor_script(side, cursor, unit_field(unit, side), steps))
            return 0;
        break;
    case 1:
        script_add(s, 0x80);
        break;
    case 2:
        if (!cursor_step(side, cursor, unit_field(target, side)))
            return 0;
        break;
    case 3:
        script_add(s, 5);
        if (NB(field_bits, bits_at(unit_field(target, side))) & (side ? 8 : 4)) {
            pwb(steps, 0x47, 5);
            return 0;
        }
        NEXT(steps);
        side_refused(side);
        return 0;
    case 4:
        script_add(s, 5);
        break;
    default:
        pwb(steps, 0x47, 0);
        return 1;
    }
    NEXT(steps);
    return 0;
}

/* step_end_round (S1E4B:08ED), a step of kind 3: the side ends its round:
 * 0 the cursor to the field (the side's headquarters), 1 the cursor
 * towards the field a row below, 2 the code 82h (button and left) once 80h
 * is clear in game_state's word 4, 3 waits for 4 in the side's
 * computer_phase (game_loop sets it after the round's end) and clears it,
 * 4 done: returns 1. */
static uint16_t step_end_round(uint16_t side, int16_t cursor, int16_t field, Far steps)
{
    switch (pb(steps, 0x47)) {
    case 0:
        if (!cursor_script(side, cursor, field, steps))
            return 0;
        break;
    case 1:
        if (!cursor_step(side, cursor, (int16_t)(field + NW(map_width, 0) * 2)))
            return 0;
        break;
    case 2:
        if (NW(game_state, 4) & 0x80)
            return 0;
        script_add(script_of(side), 0x82);
        break;
    case 3:
        if (!(NW(computer_phase, side * 0x0C) & 4))
            return 0;
        SETW(computer_phase, side * 0x0C, NW(computer_phase, side * 0x0C) & 0xFFFB);
        break;
    default:
        pwb(steps, 0x47, 0);
        return 1;
    }
    NEXT(steps);
    return 0;
}

/* move_targets_list (S0B15:13EC): the fields marked for the side in
 * field_bits (1, the second side 2) that move_target takes for the unit,
 * as map offsets at `buf`, column by column; their number in path_len (at
 * most 316h). */
void move_targets_list(Far buf, uint16_t side, uint8_t unit)
{
    Far map = far_at(S_map_ptr, side ? A_map2_ptr : A_map_ptr);
    uint8_t mask = side ? 2 : 1;
    int16_t col, row;

    SETW(path_len, 0, 0);
    SETB(move_rec, 0, unit);
    for (col = 0; col < (int16_t)NW(map_width, 0); col++)
        for (row = 0; row < (int16_t)NW(map_height, 0); row++) {
            uint16_t f;

            if (!(NB(field_bits, (uint16_t)(col + (row << 6))) & mask))
                continue;
            f = (uint16_t)((NW(map_width, 0) * row + col) << 1);
            if (move_target(f, side, map))
                continue;
            fww(buf.seg, (uint16_t)(buf.off + NW(path_len, 0) * 2), f);
            SETW(path_len, 0, NW(path_len, 0) + 1);
            if ((int16_t)NW(path_len, 0) >= 0x316)
                return;
        }
}

/* rec_find_unit (S1E4B:1827): the number of the first of the table's
 * `count` records in use (neither 2 nor 8000h in word 19h) with the unit
 * in the side's row; FFFFh for none */
static uint16_t rec_find_unit(uint16_t side, uint8_t unit, uint16_t table, uint8_t count)
{
    uint8_t r, i;

    for (r = 0; r < count; r++) {
        uint16_t rec = (uint16_t)(table + r * 0x1C);

        if (frw(S_sites_40, (uint16_t)(rec + 0x19)) & 0x8002)
            continue;
        for (i = 0; i < 7; i++)
            if (frb(S_sites_40, (uint16_t)(rec + side * 7 + i)) == unit)
                return r;
    }
    return 0xFFFF;
}

/* unit_holder (S1E4B:1734): the record that has the unit in the side's
 * row: of sites_400, sites_100, carriers or sites_40; FFFFh:FFFFh for
 * none */
static Far unit_holder(uint16_t side, uint8_t unit)
{
    static const struct { uint16_t table; uint8_t count; } T[4] = {
        { A_sites_400, 0x0A }, { A_sites_100, 0x0A }, { A_carriers, 0x46 }, { A_sites_40, 2 } };
    Far p;
    int i;

    for (i = 0; i < 4; i++) {
        uint16_t r = rec_find_unit(side, unit, T[i].table, T[i].count);

        if (r != 0xFFFF) {
            p.seg = S_sites_40;
            p.off = (uint16_t)(T[i].table + r * 0x1C);
            return p;
        }
    }
    p.seg = p.off = 0xFFFF;
    return p;
}

/* the far pointers the steps keep between calls, behind holder_ptr:
 * step_site_menu's at S3AE8:0023, step_build's at S3AE8:0027 */
static void keep_ptr(uint16_t o, Far p)
{
    SETW(holder_ptr, o, p.off);
    SETW(holder_ptr, o + 2, p.seg);
}

/* one move of the view's row (its word 0Eh) towards `place` of a site's
 * list: the code 2 (down) or 1 (up); 1 when it is there */
static int row_script(uint16_t side, int16_t place)
{
    int16_t at = (int16_t)NW(views, side * VIEW_SIZE + 0x0E);

    if (place > at)
        script_add(script_of(side), 2);
    else if (place < at)
        script_add(script_of(side), 1);
    else
        return 1;
    return 0;
}

/* the last of the seven places of the side's row in `rec` that holds
 * `what` (a unit, or FFh: a free place); 0 for none */
static int16_t row_place(Far rec, uint16_t side, uint8_t what)
{
    int16_t place = 0;
    uint8_t i;

    for (i = 0; i < 7; i++)
        if (pb(rec, (uint16_t)(side * 7 + i)) == what)
            place = i;
    return place;
}

/* step_unit_out (S1E4B:0A26), a step of kind 4: the unit leaves the site
 * or carrier it is in: 0 finds that (none: done), 1 the cursor to its
 * field, 2 the code 82h, 3 the list's row to the unit, 4 waits while 80h
 * is set in game_state's word 4, 5 the code 80h, 6 nothing, 7 the fields
 * the unit can go to (move_targets_list into the side's buffer): the
 * first becomes step_target_field, with none the holder's own field and 8
 * in the side's word 0; 8 the cursor towards it, 9 and 0Ah the button,
 * 0Bh the code 83h and done: returns 1. */
static uint16_t step_unit_out(uint16_t side, int16_t cursor, uint8_t unit, Far steps)
{
    Far s = script_of(side), holder = far_at(S_holder_ptr, A_holder_ptr);

    switch (pb(steps, 0x47)) {
    case 0:
        holder = unit_holder(side, unit);
        keep_ptr(0, holder);
        if (holder.seg == 0xFFFF && holder.off == 0xFFFF) {
            pwb(steps, 0x47, 0);
            return 1;
        }
        break;
    case 1:
        if (!cursor_script(side, cursor, (int16_t)pw(holder, 0x0E), steps))
            return 0;
        break;
    case 2:
        script_add(s, 0x82);
        break;
    case 3:
        if (!row_script(side, row_place(holder, side, unit)))
            return 0;
        break;
    case 4:
        if (NW(game_state, 4) & 0x80)
            return 0;
        break;
    case 5:
        script_add(s, 0x80);
        break;
    case 6:
        break;
    case 7: {
        Far buf = far_at(S_sides, (uint16_t)(A_sides + side * SIDE_SIZE + 9));

        move_targets_list(buf, side, unit);
        if (NW(path_len, 0) == 0) {
            SETW(step_target_field, 0, pw(holder, 0x0E));
            side_refused(side);
        } else
            SETW(step_target_field, 0, pw(buf, 0));
        SETW(path_len, 0, 0);
        break;
    }
    case 8:
        if (!cursor_step(side, cursor, (int16_t)NW(step_target_field, 0)))
            return 0;
        break;
    case 9:
    case 0x0A:
        script_add(s, 5);
        break;
    case 0x0B:
        script_add(s, 0x83);
        pwb(steps, 0x47, 0);
        return 1;
    default:
        pwb(steps, 0x47, 0);
        return 1;
    }
    NEXT(steps);
    return 0;
}

/* step_site_menu (S1E4B:0D88), a step of kind 6: as step_unit_out up to
 * the list's row of the unit in its site or carrier, then the code 81h
 * (button and down), the code 83h and done */
static uint16_t step_site_menu(uint16_t side, int16_t cursor, uint8_t unit, Far steps)
{
    Far s = script_of(side), holder = far_at(S_holder_ptr, (uint16_t)(A_holder_ptr + 6));

    switch (pb(steps, 0x47)) {
    case 0:
        holder = unit_holder(side, unit);
        keep_ptr(6, holder);
        if (holder.seg == 0xFFFF && holder.off == 0xFFFF) {
            pwb(steps, 0x47, 0);
            return 1;
        }
        break;
    case 1:
        if (!cursor_script(side, cursor, (int16_t)pw(holder, 0x0E), steps))
            return 0;
        break;
    case 2:
        script_add(s, 0x82);
        break;
    case 3:
        if (!row_script(side, row_place(holder, side, unit)))
            return 0;
        break;
    case 4:
        script_add(s, 0x81);
        break;
    case 5:
        script_add(s, 0x83);
        pwb(steps, 0x47, 0);
        return 1;
    default:
        pwb(steps, 0x47, 0);
        return 1;
    }
    NEXT(steps);
    return 0;
}

/* step_build (S1E4B:0FB8), a step of kind 7: a unit of the kind is built
 * in the record `site` of sites_400: 0 the cursor to its field, 1 the
 * code 82h, 2 the list's row to the last free place of the side's row, 3
 * the code 82h, 4 the code 2 (down) until the view's row plus its word
 * 20h reaches the kind's place in build_kinds, 5 the code 82h, 6 the code
 * 83h, 7 done */
static uint16_t step_build(uint16_t side, int16_t cursor, uint8_t kind, uint8_t site, Far steps)
{
    Far s = script_of(side), holder;

    switch (pb(steps, 0x47)) {
    case 0:
        holder.seg = S_sites_400;
        holder.off = (uint16_t)(A_sites_400 + site * 0x1C);
        keep_ptr(0x0A, holder);
        if (!cursor_script(side, cursor, (int16_t)pw(holder, 0x0E), steps))
            return 0;
        break;
    case 1:
    case 3:
    case 5:
        script_add(s, 0x82);
        break;
    case 2:
        holder = far_at(S_holder_ptr, (uint16_t)(A_holder_ptr + 0x0A));
        if (!row_script(side, row_place(holder, side, 0xFF)))
            return 0;
        break;
    case 4: {
        int16_t place = 0;
        uint8_t i;

        for (i = 0; i < NB(build_count, 0); i++)
            if (NB(build_kinds, i) == kind)
                place = i;
        if (place > (int16_t)(NW(views, side * VIEW_SIZE + 0x0E) + NW(views, side * VIEW_SIZE + 0x20)))
            script_add(s, 2);
        else
            break;
        return 0;
    }
    case 6:
        script_add(s, 0x83);
        break;
    default:
        pwb(steps, 0x47, 0);
        return 1;
    }
    NEXT(steps);
    return 0;
}

/* computer_step (S1E4B:012E): carries on the step byte 46h of the side's
 * computer_steps names, by its kind, with the map offset of the side's
 * cursor; byte 46h goes up when the step is done.  Returns 0, or FFFFh at
 * the list's end (the list is emptied).  *left stays 0: every kind is
 * translated now. */
uint16_t computer_step(uint16_t side, int *left)
{
    Far steps, step;
    int16_t cursor = (int16_t)NW(views, side * VIEW_SIZE);
    uint16_t done;

    steps.seg = S_computer_steps;
    steps.off = (uint16_t)(A_computer_steps + side * STEPS_SIZE);
    step = steps;
    step.off = (uint16_t)(step.off + pb(steps, 0x46) * 7);
    *left = 0;
    if (pb(step, 0) == 0xFF) {
        pwb(steps, 0, 0xFF);
        pwb(steps, 0x46, 0);
        return 0xFFFF;
    }
    switch (pb(step, 0)) {
    case 1:
        done = step_move(side, cursor, pb(step, 1), (int16_t)pw(step, 3), steps);
        break;
    case 2:
        done = step_attack(side, cursor, pb(step, 1), pb(step, 3), steps);
        break;
    case 3:
        done = step_end_round(side, cursor, (int16_t)NW(sites_40, side * 0x1C + 0x0E), steps);
        break;
    case 5:
        done = cursor_script(side, cursor, (int16_t)pw(step, 1), steps);
        break;
    case 4:
        done = step_unit_out(side, cursor, pb(step, 1), steps);
        break;
    case 6:
        done = step_site_menu(side, cursor, pb(step, 1), steps);
        break;
    case 7:
        done = step_build(side, cursor, pb(step, 1), pb(step, 3), steps);
        break;
    default:
        pwb(steps, 0, 0xFF);
        pwb(steps, 0x46, 0);
        return 0;
    }
    if (done)
        pwb(steps, 0x46, (uint8_t)(pb(steps, 0x46) + 1));
    return 0;
}

/* computer_input (S1E4B:0001): the side's next word of input, into
 * computer_out, from the code at the script's place (byte 1Eh): a code
 * below 80h is one word of script_bits, a higher one a sequence of
 * script_gestures, one word a call (byte 1Fh the place in it).  Returns 1
 * when the script is at its end after that, and empties it. */
uint16_t computer_input(uint16_t side)
{
    Far s = script_of(side);
    uint8_t c;
    uint16_t done = 0;

    SETW(computer_out, side * 4, 0);
    c = pb(s, pb(s, 0x1E));
    if (c != 0xFF && c & 0x80) {
        uint8_t g = NB(script_gestures, (c & 0x7F) * 3 + pb(s, 0x1F));

        if (g != 0xFF) {
            SETW(computer_out, side * 4, g);
            pwb(s, 0x1F, (uint8_t)(pb(s, 0x1F) + 1));
        } else {
            pwb(s, 0x1F, 0);
            pwb(s, 0x1E, (uint8_t)(pb(s, 0x1E) + 1));
        }
    } else if (c != 0xFF) {
        SETW(computer_out, side * 4, NB(script_bits, c));
        pwb(s, 0x1E, (uint8_t)(pb(s, 0x1E) + 1));
    }
    if (pb(s, pb(s, 0x1E)) == 0xFF) {
        pwb(s, 0, 0xFF);
        pwb(s, 0x1E, 0);
        pwb(s, 0x1F, 0);
        done = 1;
    }
    return done;
}
