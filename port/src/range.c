/* range.c - where a unit can move and whom it can attack: HL.EXE's
 * move_costs (S0B15:0A01), move_range (S0B15:0371), attack_range
 * (S0B15:062F), buf_neighbours (S0B15:005B) and path_find (S0B15:0E52),
 * translated over the program's memory.  They work in a buffer of one byte a field, 64 fields
 * a row.  The original's check of the stack at each entry is left out.
 * port/README.md says how this was compared with the original.
 */
#include "hl.h"

#define BUF_BYTES 0x1104u
#define CLOSED 0xB0u        /* a field the unit cannot enter */


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

/* ---- the way */

#define NODE 10u            /* a field met: column, row, the field it was
                             * reached from, its worth, the one before and
                             * the one after it in the order of worth */
#define NODES 0x316
#define PATH 0x1F40u        /* the way's fields, in the same buffer */

/* S0B15:000B: clears the bits of `mask` in every byte of field_bits */
void field_bits_clear(uint8_t mask)
{
    uint16_t i;
    for (i = 0; i < BUF_BYTES; i++)
        SETB(field_bits, i, NB(field_bits, i) & (uint8_t)~mask);
}

/* S0B15:0E52: the way from `from` to `to` over the fields move_range marked
 * for the side.  Returns 0 and leaves the way, from its end back to its
 * beginning, as map offsets at 1F40h of the buffer and their number in
 * path_len; FFFFh and no way when there is none or the buffer is full.
 * The fields are tried in the order of their worth: the distance to the
 * end times the kind's byte 15h plus the ground's cost times its byte 16h. */
int16_t path_find(Far buf, uint16_t unit, int16_t from, int16_t to, uint16_t side, Far map)
{
    int16_t w = (int16_t)NW(map_width, 0);
    uint8_t reach = side != 0 ? 2 : 1, seen = side != 0 ? 0x80 : 0x40;
    uint16_t kind = (uint16_t)(A_unit_dat + NB(unit_kind, (uint16_t)(unit * UNIT_SIZE)) * KIND_SIZE);
    int16_t by_dist = (int8_t)frb(S_unit_dat, (uint16_t)(kind + 0x15));
    int16_t by_cost = (int8_t)frb(S_unit_dat, (uint16_t)(kind + 0x16));
    uint8_t to_col = (uint8_t)((to >> 1) % w), to_row = (uint8_t)((to >> 1) / w);
    int16_t at, next = 2;
    int i;
#define NB_(n, o) pb(buf, (uint16_t)((n) * NODE + (o)))
#define NW_(n, o) pw(buf, (uint16_t)((n) * NODE + (o)))
#define SB_(n, o, v) pwb(buf, (uint16_t)((n) * NODE + (o)), (uint8_t)(v))
#define SW_(n, o, v) fww(buf.seg, (uint16_t)(buf.off + (n) * NODE + (o)), (uint16_t)(v))

    SETW(path_len, 0, 0);
    SW_(0, 8, 1);
    SB_(1, 0, (from >> 1) % w);
    SB_(1, 1, (from >> 1) / w);
    SW_(1, 2, 1);
    SW_(1, 4, 0);
    SW_(1, 6, 0);
    SW_(1, 8, 2);
    SB_(2, 0, 0);               /* the list's end */
    SB_(2, 1, 0);
    SW_(2, 4, 0x7530);
    SW_(2, 6, 1);
    SW_(2, 8, 0);
    next++;
    at = 0;
    do {
        at = (int16_t)NW_(at, 8);
        if (at == 0)
            goto none;
        if (NB_(at, 0) == to_col && NB_(at, 1) == to_row) {
            fww(buf.seg, (uint16_t)(buf.off + PATH),
                (uint16_t)((int8_t)to_row * (w << 1) + ((int8_t)to_col << 1)));
            SETW(path_len, 0, NW(path_len, 0) + 1);
            while (at > 1) {
                at = (int16_t)NW_(at, 2);
                fww(buf.seg, (uint16_t)(buf.off + PATH + (NW(path_len, 0) << 1)),
                    (uint16_t)((int8_t)NB_(at, 1) * (w << 1) + ((int8_t)NB_(at, 0) << 1)));
                SETW(path_len, 0, NW(path_len, 0) + 1);
                if ((int16_t)NW(path_len, 0) > NODES)
                    goto none;
            }
            at = 0;
        } else if (at != 2) {
            buf_neighbours((int8_t)NB_(at, 0), (int8_t)NB_(at, 1));
            for (i = 0; i < 6; i++) {
                int16_t n = (int16_t)NW(neighbours, 2 * i), dist, worth, cost, k;
                uint8_t bits, col, row;

                if (n < 0)
                    continue;
                bits = NB(field_bits, (uint16_t)n);
                if (!(bits & reach) || bits & seen)
                    continue;
                col = (uint8_t)(n % 0x40);
                row = (uint8_t)(n / 0x40);
                dist = (int8_t)field_distance((int8_t)col, (int8_t)row, (int8_t)to_col, (int8_t)to_row);
                cost = (int8_t)frb(S_ww1gnds_dat, (uint16_t)(A_ww1gnds_dat
                    + pb(map, (uint16_t)((int8_t)row * (w << 1) + ((int8_t)col << 1))) * 6
                    + (NW(unit_flags, (uint16_t)(unit * UNIT_SIZE)) & 0x10 ? 4 : 3)));
                worth = dist == 0 ? 0 : (int16_t)(dist * by_dist + cost * by_cost);
                k = (int16_t)NW_(at, 8);
                while ((int16_t)NW_(k, 4) <= worth)
                    k = (int16_t)NW_(k, 8);
                SB_(next, 0, col);
                SB_(next, 1, row);
                SW_(next, 4, worth);
                SW_(next, 2, at);
                SW_(next, 6, NW_(k, 6));
                SW_(next, 8, k);
                SW_(NW_(k, 6), 8, next);
                SW_(k, 6, next);
                SETB(field_bits, (uint16_t)n, NB(field_bits, (uint16_t)n) | seen);
                next++;
                if (next > NODES)
                    goto none;
            }
        }
    } while (at != 0);
    field_bits_clear(seen);
    return 0;
none:
    field_bits_clear(seen);
    SETW(path_len, 0, 0);
    return -1;
#undef NB_
#undef NW_
#undef SB_
#undef SW_
}
