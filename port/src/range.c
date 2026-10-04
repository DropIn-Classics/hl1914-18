/* range.c - where a unit can move and whom it can attack: HL.EXE's
 * move_costs (S0B15:0A01), move_range (S0B15:0371), attack_range
 * (S0B15:062F) and buf_neighbours (S0B15:005B), translated over the
 * program's memory.  They work in a buffer of one byte a field, 64 fields
 * a row.  The original's check of the stack at each entry is left out.
 * port/README.md says how this was compared with the original.
 */
#include "hl.h"

#define BUF_BYTES 0x1104u
#define CLOSED 0xB0u        /* a field the unit cannot enter */

/* where move_range keeps its nine argument words */
#define S_move_args (HL_S3306 + HL_BASE)
#define A_move_args 0x000Au

/* S0B15:005B with S0B15:025F: the places in the buffer of the six fields
 * around a field into `neighbours`, clockwise from the one above; FFFFh
 * beyond the map's edge. */
void buf_neighbours(int16_t col, int16_t row)
{
    int16_t w = (int16_t)NW(map_width, 0), h = (int16_t)NW(map_height, 0);
    uint16_t s, n[6];
    int i;

    if (w <= col)
        col = (int16_t)(w - 1);
    if (col < 0)
        col = 0;
    if (h <= row)
        row = (int16_t)(h - 1);
    if (row < 0)
        row = 0;
    s = (uint16_t)(col + (row << 6));
    n[0] = (uint16_t)(s - 0x40);
    n[3] = (uint16_t)(s + 0x40);
    if (col & 1) {
        n[1] = (uint16_t)(s + 1);
        n[5] = (uint16_t)(s - 1);
        n[4] = (uint16_t)(s + 0x3F);
        n[2] = (uint16_t)(s + 0x41);
        if (row <= 0)
            n[0] = 0xFFFF;
        else if (h - 1 <= row)
            n[3] = n[2] = n[4] = 0xFFFF;
        if (col <= 0)
            n[5] = n[4] = 0xFFFF;
        else if (w - 1 <= col)
            n[1] = n[2] = 0xFFFF;
    } else {
        n[2] = (uint16_t)(s + 1);
        n[4] = (uint16_t)(s - 1);
        n[5] = (uint16_t)(s - 0x41);
        n[1] = (uint16_t)(s - 0x3F);
        if (row <= 0)
            n[0] = n[5] = n[1] = 0xFFFF;
        else if (h - 1 <= row)
            n[3] = 0xFFFF;
        if (col <= 0)
            n[4] = n[5] = 0xFFFF;
        else if (w - 1 <= col)
            n[1] = n[2] = 0xFFFF;
    }
    for (i = 0; i < 6; i++)
        SETW(neighbours, 2 * i, n[i]);
}

/* the fields around (col, row) that are not closed get `mark` */
static void mark_around(Far buf, int16_t col, int16_t row, uint8_t mark)
{
    int i;

    buf_neighbours(col, row);
    for (i = 0; i < 6; i++) {
        int16_t n = (int16_t)NW(neighbours, 2 * i);
        if (n >= 0 && pb(buf, (uint16_t)n) != CLOSED)
            pwb(buf, (uint16_t)n, mark);
    }
}

/* S0B15:0A01: what entering each field costs the unit, as a negative byte */
void move_costs(Far buf, uint8_t unit, uint16_t side, uint16_t mask, Far map)
{
    int16_t w = (int16_t)NW(map_width, 0), h = (int16_t)NW(map_height, 0);
    uint16_t w2 = (uint16_t)(NW(map_width, 0) << 1);
    uint16_t kind = (uint16_t)(A_unit_dat + NB(unit_kind, unit * UNIT_SIZE) * KIND_SIZE);
    uint16_t kind_d = frw(S_unit_dat, (uint16_t)(kind + 0x0D));
    uint8_t classes = frb(S_unit_dat, (uint16_t)(kind + 5));
    int16_t col, row;
    uint16_t i;
    int mode;

    for (i = 0; i < BUF_BYTES; i++)
        pwb(buf, i, 0xFF);
    mode = NW(unit_flags, unit * UNIT_SIZE) & 0x10 ? 0 : 2;
    if (NW(units, unit * UNIT_SIZE + 6) & 0x10)
        mode = 1;

    for (row = 0; row < h; row++)
        for (col = 0; col < w; col++) {
            uint16_t at = (uint16_t)((row << 6) + col);
            uint16_t g = (uint16_t)(A_ww1gnds_dat + pb(map, (uint16_t)(row * w2 + col * 2)) * 6);
            uint8_t v = CLOSED;

            if (frb(S_ww1gnds_dat, (uint16_t)(g + 2)) & classes)
                v = mode == 0 ? (uint8_t)-frb(S_ww1gnds_dat, (uint16_t)(g + 4))
                  : mode == 1 ? 0xFE
                  : (uint8_t)-frb(S_ww1gnds_dat, (uint16_t)(g + 3));
            if (mask != 0xFFFF && frw(S_ww1gnds_dat, g) & mask)
                v = CLOSED;
            pwb(buf, at, v);
        }

    for (row = 0; row < h; row++)
        for (col = 0; col < w; col++) {
            uint16_t at = (uint16_t)((row << 6) + col);
            uint8_t u = pb(map, (uint16_t)(row * w2 + col * 2 + 1));
            uint16_t flags;

            if (u > 0xF0)
                continue;
            flags = NW(unit_flags, u * UNIT_SIZE);
            if (flags & 2)
                pwb(buf, at, CLOSED);
            else if (flags & 1) {
                if (side != 1) {
                    pwb(buf, at, CLOSED);
                    mark_around(buf, col, row, 0x9A);
                } else if (flags & 0x3000)
                    pwb(buf, at, 0xFE);
            } else {
                if (side != 0) {
                    pwb(buf, at, CLOSED);
                    mark_around(buf, col, row, 0x9B);
                } else if (flags & 0x3000)
                    pwb(buf, at, 0xFE);
            }
            if (kind_d & 0x40 && u != unit)
                pwb(buf, at, CLOSED);
            /* (the original has a further part here that a variable it
             * has just set to 0 keeps from running) */
        }
}

/* S0B15:0371: marks in field_bits where the unit can move; returns the
 * number of fields marked.  `unit` is the word on the stack, of which the
 * routine uses the low byte. */
int16_t move_range(int16_t field, Far buf, uint16_t unit, int16_t range,
                   uint16_t side, uint16_t mask, Far map)
{
    int16_t w = (int16_t)NW(map_width, 0), h = (int16_t)NW(map_height, 0);
    uint8_t marker = side != 0 ? 0x9B : 0x9A, bit;
    int16_t col0, col1, row0, row1, col, row, level, count = 0;
    uint16_t at, i;

    fww(S_move_args, A_move_args, (uint16_t)field);
    fww(S_move_args, A_move_args + 2, buf.off);
    fww(S_move_args, A_move_args + 4, buf.seg);
    fww(S_move_args, A_move_args + 6, unit);
    fww(S_move_args, A_move_args + 8, (uint16_t)range);
    fww(S_move_args, A_move_args + 0x0A, side);
    fww(S_move_args, A_move_args + 0x0C, mask);
    fww(S_move_args, A_move_args + 0x0E, map.off);
    fww(S_move_args, A_move_args + 0x10, map.seg);

    field = (int16_t)(field >> 1);
    move_costs(buf, (uint8_t)unit, side, mask, map);

    col0 = (int16_t)(field % w - (range >> 1));
    if (col0 < 0)
        col0 = 0;
    row0 = (int16_t)(field / w - (range >> 1));
    if (row0 < 0)
        row0 = 0;
    col1 = (int16_t)(col0 + range);
    if (w < col1)
        col1 = w;
    row1 = (int16_t)(row0 + range);
    if (h < row1)
        row1 = h;

    at = (uint16_t)(field % w + ((field / w) << 6));
    pwb(buf, at, (uint8_t)range);
    for (level = (int8_t)pb(buf, at); level > 0; level--)
        for (row = row0; row < row1; row++)
            for (col = col0; col < col1; col++) {
                uint16_t here = (uint16_t)(row * 0x40 + col);

                if ((int8_t)pb(buf, here) != level)
                    continue;
                buf_neighbours(col, row);
                for (i = 0; i < 6; i++) {
                    int16_t n = (int16_t)NW(neighbours, 2 * i);
                    int8_t b;

                    if (n < 0)
                        continue;
                    b = (int8_t)pb(buf, (uint16_t)n);
                    if (b >= 0)
                        continue;
                    if ((uint8_t)b == marker)
                        pwb(buf, (uint16_t)n, 0);
                    else if (b + (int8_t)pb(buf, here) >= 0)
                        pwb(buf, (uint16_t)n, (uint8_t)(b + pb(buf, here)));
                }
            }

    bit = side == 1 ? 2 : 1;
    for (i = 0; i < BUF_BYTES; i++)
        if ((int8_t)pb(buf, i) >= 0) {
            count++;
            SETB(field_bits, i, NB(field_bits, i) | bit);
        }
    return count;
}

/* S0B15:062F: marks in field_bits the units of the other side the unit can
 * attack with a weapon of this range; returns their number. */
uint8_t attack_range(Far buf, int16_t field, int16_t range, uint16_t side,
                     uint16_t unit, uint16_t mask, Far map)
{
    int16_t w = (int16_t)NW(map_width, 0), h = (int16_t)NW(map_height, 0);
    uint16_t w2 = (uint16_t)(NW(map_width, 0) << 1);
    int16_t col0, col1, row0, row1, col, row, level, pos;
    uint16_t at, i;
    uint8_t bit, count = 0;

    for (i = 0; i < BUF_BYTES; i++)
        pwb(buf, i, 0xFF);
    field = (int16_t)(field >> 1);
    col0 = (int16_t)(field % w - range);
    if (col0 < 0)
        col0 = 0;
    row0 = (int16_t)(field / w - range);
    if (row0 < 0)
        row0 = 0;
    col1 = (int16_t)(col0 + (range << 1) + 1);
    if (col1 > w)
        col1 = w;
    row1 = (int16_t)(row0 + (range << 1) + 1);
    if (h < row1)
        row1 = h;

    at = (uint16_t)(field % w + ((field / w) << 6));
    pwb(buf, at, (uint8_t)(pb(buf, at) + range));
    for (level = (int8_t)pb(buf, at); level > 0; level--)
        for (row = row0; row < row1; row++)
            for (col = col0; col < col1; col++) {
                uint16_t here = (uint16_t)(row * 0x40 + col);

                if ((int8_t)pb(buf, here) != level)
                    continue;
                buf_neighbours(col, row);
                for (i = 0; i < 6; i++) {
                    int16_t n = (int16_t)NW(neighbours, 2 * i);
                    int8_t b;

                    if (n < 0)
                        continue;
                    b = (int8_t)pb(buf, (uint16_t)n);
                    if (b < 0 && b + (int8_t)pb(buf, here) >= 0)
                        pwb(buf, (uint16_t)n, (uint8_t)(b + pb(buf, here)));
                }
            }

    /* a unit with 40h in the mask does not reach the fields around it */
    pos = (int16_t)((int16_t)(NW(unit_pos, (uint16_t)(unit * UNIT_SIZE + (side << 1))) - 1) >> 1);
    if (mask & 0x40) {
        buf_neighbours((int16_t)(pos % w), (int16_t)(pos / w));
        for (i = 0; i < 6; i++) {
            int16_t n = (int16_t)NW(neighbours, 2 * i);
            if (n >= 0)
                pwb(buf, (uint16_t)n, 0xFF);
        }
    }

    for (row = 0; row < h; row++)
        for (col = 0; col < w; col++) {
            uint8_t u;
            uint16_t flags, six;
            int other;

            at = (uint16_t)(row * 0x40 + col);
            if ((int8_t)pb(buf, at) < 0)
                continue;
            pwb(buf, at, 0xFF);
            u = pb(map, (uint16_t)(row * w2 + col * 2 + 1));
            if (u > 0xF0)
                continue;
            flags = NW(unit_flags, u * UNIT_SIZE);
            other = flags & 1 ? side != 1 : side != 0;
            if (flags & 2)
                other = 0;
            if (!other)
                continue;
            if (flags & mask & 0x3C)
                pwb(buf, at, 1);
            six = NW(units, u * UNIT_SIZE + 6);
            if (six & 2 && six & mask & 2)
                pwb(buf, at, 1);
        }

    bit = side & 1 ? 8 : 4;
    for (i = 0; i < BUF_BYTES; i++)
        if ((int8_t)pb(buf, i) > 0) {
            SETB(field_bits, i, NB(field_bits, i) | bit);
            count++;
        }
    return count;
}
