/* move.c - how a move is carried out: HL.EXE's move_step (S10E1:072C),
 * move_target (S10E1:0F82), move_arrive (S10E1:0AF0) and what they call
 * (field_dir, row_count, row_move, row_load, carrier_moved, site_find,
 * timer_set, unit_mark), translated over the program's memory.  What the
 * original draws on the way (S0E56:145A for a field, show_message for the
 * text at the end) is left out, as is its check of the stack at each
 * entry.  port/README.md says how this was compared with the original.
 */
#include "hl.h"

#define SITE_SIZE 0x1Cu     /* a record of `carriers` and the like */
#define SIDE_SIZE 0x17u     /* a record of `sides` */

/* move_rec, the move under way */
enum {
    MR_UNIT = 0,            /* the unit that walks */
    MR_OTHER = 1,           /* the unit on the aim's field */
    MR_UNDER = 2,           /* the unit byte of the field it stands on */
    MR_UNDER2 = 3,          /* the same for the second half */
    MR_PLACE = 4,           /* a place in a row of seven */
    MR_SITE = 5,            /* far pointer to the site's or carrier's record */
    MR_SITE_NO = 9,
    MR_AIM = 0x0A,          /* the aim's field */
    MR_KIND = 0x0C          /* what the arrival is, 1 to 5 */
};

static void pww(Far p, uint16_t o, uint16_t v)
{
    fww(p.seg, (uint16_t)(p.off + o), v);
}

static Far unit_at(uint8_t unit)
{
    Far p;
    p.seg = S_units;
    p.off = (uint16_t)(A_units + unit * UNIT_SIZE);
    return p;
}

static Far kind_of(Far unit)
{
    Far p;
    p.seg = S_unit_dat;
    p.off = (uint16_t)(A_unit_dat + pb(unit, 8) * KIND_SIZE);
    return p;
}

static Far site_at(uint16_t table, int16_t n)
{
    Far p;
    p.seg = S_carriers;
    p.off = (uint16_t)(table + n * (int)SITE_SIZE);
    return p;
}

/* the record of `carriers` a unit's byte 0Ah names */
static Far carrier_of(Far unit)
{
    return site_at(A_carriers, (int8_t)pb(unit, 0x0A));
}

/* S10E1:144C: the direction of a neighbouring field, 0 above and on
 * clockwise to 5; both are map offsets */
uint8_t field_dir(int16_t from, int16_t to)
{
    int16_t w = (int16_t)NW(map_width, 0);
    int16_t col, row, col2, row2;

    from >>= 1;
    to >>= 1;
    col = (int16_t)(from % w);
    row = (int16_t)(from / w);
    col2 = (int16_t)(to % w);
    row2 = (int16_t)(to / w);
    if (col2 == col)
        return row2 < row ? 0 : 3;
    if (col & 1) {
        if (col2 < col)
            return row2 > row ? 4 : 5;
        return row2 > row ? 2 : 1;
    }
    if (col2 < col)
        return row2 < row ? 5 : 4;
    return row2 < row ? 1 : 2;
}

/* S10E1:17D9: the places of the side's row that hold a unit (used) or
 * are free */
int16_t row_count(uint16_t side, Far rec, uint16_t used)
{
    int16_t n = 0;
    uint8_t i;

    for (i = 0; i < 7; i++) {
        uint8_t u = pb(rec, (uint16_t)(side * 7 + i));
        if (used ? u <= 0xF0 : u > 0xF0)
            n++;
    }
    return n;
}

/* S131D:15FB: what the units of the side's row weigh (byte 41h of their
 * kinds) */
int16_t row_load(Far rec, uint16_t side)
{
    int16_t sum = 0;
    uint8_t i;

    for (i = 0; i < 7; i++) {
        uint8_t u = pb(rec, (uint16_t)(side * 7 + i));
        if (u <= 0xF0)
            sum = (int16_t)(sum + pb(kind_of(unit_at(u)), 0x41));
    }
    return sum;
}

/* S10E1:184E: the units of the side's row of `from` go to the free places
 * of `to` */
void row_move(uint16_t side, Far from, Far to)
{
    uint8_t i, c;

    for (i = 0; i < 7; i++) {
        uint8_t u = pb(from, (uint16_t)(side * 7 + i));
        if (u > 0xF0)
            continue;
        for (c = 0; c < 7; c++) {
            if (pb(to, (uint16_t)(side * 7 + c)) <= 0xF0)
                continue;
            pwb(to, (uint16_t)(side * 7 + c), u);
            SETW(unit_pos, u * UNIT_SIZE + 2 * side, pw(to, 0x0E));
            pwb(from, (uint16_t)(side * 7 + i), 0xFF);
            break;
        }
    }
}

/* S15C5:0804: a carrier's place for the side goes to the units in its row
 * and to its record */
void carrier_moved(uint16_t unit, uint16_t side)
{
    Far rec = carrier_of(unit_at((uint8_t)unit));
    uint16_t pos = NW(unit_pos, unit * UNIT_SIZE + 2 * side);
    uint8_t i;

    for (i = 0; i < 7; i++) {
        uint8_t u = pb(rec, (uint16_t)(side * 7 + i));
        if (u <= 0xF0)
            SETW(unit_pos, u * UNIT_SIZE + 2 * side, pos);
    }
    pww(rec, 0x0E, pos);
}

/* S0E56:1726: the first of the table's ten records with the field among
 * its four, FFFFh for none */
uint16_t site_find(uint16_t field, Far table)
{
    uint16_t n, k;

    for (n = 0; n < 10; n++, table.off = (uint16_t)(table.off + SITE_SIZE))
        for (k = 0; k < 4; k++)
            if (pw(table, (uint16_t)(0x0E + 2 * k)) == field)
                return n;
    return 0xFFFF;
}

/* S0CB6:0272: the first free of game_state's 15 timers gets the kind, the
 * pass it is due at and a value; nothing when all are taken */
void timer_set(uint16_t kind, int16_t passes, uint16_t value)
{
    uint16_t i, k;

    for (i = 0; i < 15; i++) {
        if (NB(game_state, 0x19 + i))
            continue;
        if (kind == 1 || kind == 2)
            for (k = 0; k < 15; k++)
                if (NB(game_state, 0x19 + k) == kind)
                    SETB(game_state, 0x19 + k, 0);
        SETB(game_state, 0x19 + i, kind);
        SETW(game_state, 0x64 + 2 * i, value);
        {
            uint32_t due = (uint32_t)NW(game_state, 0x13) | (uint32_t)NW(game_state, 0x15) << 16;
            due += (uint32_t)(int32_t)passes;
            SETW(game_state, 0x28 + 4 * i, due);
            SETW(game_state, 0x2A + 4 * i, due >> 16);
        }
        return;
    }
}

/* one half of unit_mark */
static void mark_half(uint8_t unit, uint16_t bits, uint8_t mask, uint16_t side, uint16_t set)
{
    int16_t w = (int16_t)NW(map_width, 0);
    int16_t f = (int16_t)((int16_t)(NW(unit_pos, unit * UNIT_SIZE + 2 * side) - 1) >> 1);
    uint16_t at = (uint16_t)(f % w + ((f / w) << 6));
    uint16_t flags = NW(unit_flags, unit * UNIT_SIZE);

    if (set) {
        SETW(unit_flags, unit * UNIT_SIZE, flags | bits);
        SETB(field_bits, at, NB(field_bits, at) | mask);
    } else {
        SETW(unit_flags, unit * UNIT_SIZE, flags & ~bits);
        SETB(field_bits, at, NB(field_bits, at) & ~mask);
    }
}

/* S0E56:1DDB: sets or clears the bits in unit_flags and the mark that
 * belongs to them on the unit's field in field_bits (40h or 80h for 100h,
 * 10h or 20h for 200h, by the side), for both halves of a two-part unit.
 * The original draws the fields. */
void unit_mark(uint8_t unit, uint16_t bits, uint16_t side, uint16_t set)
{
    uint8_t mask = 0;
    uint16_t flags;

    if (bits & 0x100)
        mask = side & 1 ? 0x80 : 0x40;
    else if (bits & 0x200)
        mask = side & 1 ? 0x20 : 0x10;
    mark_half(unit, bits, mask, side, set);
    flags = NW(unit_flags, unit * UNIT_SIZE);
    if (flags & 0x40)
        mark_half((uint8_t)(flags & 0x80 ? unit + 1 : unit - 1), bits, mask, side, set);
}

/* the first free place of the side's row, 7 for none */
static uint8_t row_free(Far rec, uint16_t side)
{
    uint8_t i;

    for (i = 0; i < 7; i++)
        if (pb(rec, (uint16_t)(side * 7 + i)) > 0xF0)
            break;
    return i;
}

/* a carrier that moves in needs a free place more than it has units */
static int row_room(Far unit, Far rec, uint16_t side)
{
    if (!(pw(unit, 4) & 0x1000))
        return 1;
    return row_count(side, rec, 0) >= row_count(side, carrier_of(unit), 1) + 1;
}

static void mr_site(Far rec)
{
    SETW(move_rec, MR_SITE, rec.off);
    SETW(move_rec, MR_SITE + 2, rec.seg);
}

/* S10E1:0F82: what a move of move_rec's unit to the field would be, kept
 * in move_rec; returns 0 or the number of the text that refuses it */
uint16_t move_target(uint16_t field, uint16_t side, Far map)
{
    Far unit = unit_at(NB(move_rec, MR_UNIT));
    uint8_t on = pb(map, (uint16_t)(field + 1));
    Far rec;
    uint16_t ground, text = 0;
    int site = 0;
    uint8_t place;

    SETB(move_rec, MR_KIND, 3);
    SETW(move_rec, MR_AIM, field);
    if (on <= 0xF0) {
        Far other = unit_at(on);
        int16_t room, load;
        int fits;

        SETB(move_rec, MR_OTHER, on);
        if ((pw(other, 4) & 0x40) && !(pw(other, 4) & 0x80))
            other = unit_at((uint8_t)(on - 1));
        if (!same_side(pw(other, 4), side))
            return 0x37;
        if (!(pw(other, 4) & 0x1000))
            return 0x39;
        rec = carrier_of(other);
        mr_site(rec);
        SETB(move_rec, MR_SITE_NO, pb(other, 0x0A));
        if (!(pw(unit, 6) & pw(rec, 0x19) & 0x0FC0) || (pw(other, 4) & 0x200))
            return 0x3A;
        place = row_free(rec, side);
        if (place == 7)
            return 0x3B;
        room = pb(kind_of(other), 0x40);
        load = pb(kind_of(unit), 0x41);
        fits = 1;
        if (pw(unit, 4) & 0x1000) {
            load = (int16_t)(load + row_load(carrier_of(unit), side));
            fits = row_room(unit, rec, side);
        }
        load = (int16_t)(load + row_load(rec, side));
        if (load > room || !fits)
            return 0x3B;
        SETB(move_rec, MR_PLACE, place);
        SETB(move_rec, MR_KIND, 2);
        return 0;
    }
    ground = NW(ww1gnds_dat, pb(map, field) * 6);
    if (ground & 0x400) {
        Far t = site_at(A_sites_400, 0);
        int16_t n = (int16_t)site_find(field, t);
        rec = site_at(A_sites_400, n);
        SETB(move_rec, MR_SITE_NO, n);
        site = 1;
    } else if (ground & 0x100) {
        Far t = site_at(A_sites_100, 0);
        int16_t n = (int16_t)site_find(field, t);
        rec = site_at(A_sites_100, n);
        SETB(move_rec, MR_SITE_NO, n);
        site = 1;
    } else if (ground & 0x40) {
        rec = site_at(A_sites_40, ground & 1);
        SETB(move_rec, MR_SITE_NO, ground & 1);
        site = 1;
    }
    if (site) {
        place = row_free(rec, side);
        if (!same_side(pw(rec, 0x19), side) || (pw(rec, 0x19) & 2)) {
            SETB(move_rec, MR_KIND, 4);
            mr_site(rec);
            return 0;
        }
        if (!row_room(unit, rec, side))
            place = 7;
        if (place == 7)
            return 0x3B;
        SETB(move_rec, MR_PLACE, place);
        SETB(move_rec, MR_KIND, 5);
        mr_site(rec);
        return 0;
    }
    if (!(pb(kind_of(unit), 5) & NB(ww1gnds_dat, pb(map, field) * 6 + 2)))
        text = 0x40;
    if ((pw(unit, 4) & 0x10) && (ground & 0x4000))
        text = 0x40;
    return text;
}

/* S10E1:0AF0: the end of a walk, by move_rec's kind: the unit gets its
 * order (byte 13h and what belongs to it) and goes into the carrier or
 * site it came to.  Returns the text the original then shows (0 for a
 * kind that is none of the five, where the original returns whatever its
 * stack holds). */
uint16_t move_arrive(uint16_t side, Far map)
{
    uint8_t no = NB(move_rec, MR_UNIT), kind = NB(move_rec, MR_KIND);
    uint8_t place = NB(move_rec, MR_PLACE), on = NB(move_rec, MR_OTHER);
    Far unit = unit_at(no), rec = far_at(S_move_rec, A_move_rec + MR_SITE);
    uint16_t pos = (uint16_t)(A_unit_pos - A_units + 2 * side);     /* in a unit's record */
    uint16_t text = 0x38;

    switch (kind) {
    case 1: {
        Far other = unit_at(on);
        pwb(rec, (uint16_t)(side * 7 + place), on);
        pww(unit, pos, (uint16_t)(NW(move_rec, MR_AIM) + 1));
        pwb(map, pw(unit, pos), no);
        carrier_moved(no, side);
        pww(unit, 0x17, pw(other, 4));
        pww(other, 4, pw(other, 4) | 0x4200);
        pww(unit, 4, (pw(unit, 4) & 0xBEFF) | 0x200);
        text = pw(other, 6) & 0x20 ? 0x43 : 0x44;
        unit_mark(no, 0x200, side, 1);
        pwb(unit, 0x13, kind);
        pwb(unit, 0x14, pb(unit, 0x14) | 1);
        pwb(unit, 0x19, place);
        pwb(unit, 0x16, on);
        break;
    }
    case 2:
        pwb(rec, (uint16_t)(side * 7 + place), no);
        pwb(map, pw(unit, pos), on);
        pww(unit, 4, (pw(unit, 4) & 0xFEFF) | 0x4200);
        if (pw(unit, 4) & 0x1000)
            row_move(side, carrier_of(unit), rec);
        pwb(unit, 0x13, kind);
        pwb(unit, 0x19, place);
        pwb(unit, 0x15, NB(move_rec, MR_SITE_NO));
        pwb(unit, 0x16, on);
        pwb(unit, 0x14, pb(unit, 0x14) | 1);
        break;
    case 3:
    case 4:
        pww(unit, 4, (pw(unit, 4) & 0xBEFF) | 0x200);
        unit_mark(no, 0x200, side, 1);
        pwb(unit, 0x13, kind);
        pwb(unit, 0x14, pb(unit, 0x14) | 1);
        if (pw(unit, 4) & 0x1000)
            carrier_moved(no, side);
        if (kind == 3) {
            pwb(unit, 0x15, 0);
            pwb(unit, 0x19, 0);
            pww(unit, 0x17, 0);
        } else {
            pww(unit, 0x17, pw(rec, 0x19));
            pwb(unit, 0x15, NB(move_rec, MR_SITE_NO));
        }
        pwb(unit, 0x16, 0);
        break;
    case 5:
        pwb(unit, 0x13, kind);
        pwb(unit, 0x14, pb(unit, 0x14) | 1);
        pwb(unit, 0x19, place);
        pwb(unit, 0x15, NB(move_rec, MR_SITE_NO));
        pww(unit, 0x17, pw(rec, 0x19));
        pwb(rec, (uint16_t)(side * 7 + place), no);
        if (pw(unit, 4) & 0x1000)
            row_move(side, carrier_of(unit), rec);
        pwb(map, pw(unit, pos), 0xF4);
        pww(unit, pos, pw(rec, 0x0E));
        pww(unit, 4, (pw(unit, 4) & 0xFEFF) | 0x4200);
        break;
    default:
        text = 0;
    }
    return text;
}

/* S10E1:072C: the unit of move_rec goes one field on along its way.
 * Returns 0 while the walk goes on (a timer of kind 4 is set for the next
 * step), else the text of its end, which the original shows to the side
 * (FFFFh for none, the value of a kind move_arrive does not know). */
uint16_t move_step(void)
{
    uint8_t no = NB(move_rec, MR_UNIT);
    Far unit = unit_at(no);
    uint16_t side = pw(unit, 4) & 1;
    uint16_t pos = (uint16_t)(A_unit_pos - A_units + 2 * side);
    Far way = far_at(S_sides, (uint16_t)(A_sides + side * SIDE_SIZE + 0x0D));
    Far map = side ? far_at(S_map2_ptr, A_map2_ptr) : far_at(S_map_ptr, A_map_ptr);
    uint16_t next, text;

    way.off = (uint16_t)(way.off + 0x1F40);
    next = pw(way, (uint16_t)(NW(path_len, 0) << 1));
    if (NW(path_len, 0) == 0)
        move_target(next, side, map);
    if (pw(unit, 4) & 0x40) {
        Far half = unit_at((uint8_t)(no + 1));
        uint16_t old = pw(unit, pos);

        pwb(map, pw(half, pos), NB(move_rec, MR_UNDER2));
        SETB(move_rec, MR_UNDER2, NB(move_rec, MR_UNDER));
        SETB(move_rec, MR_UNDER, pb(map, (uint16_t)(next + 1)));
        pwb(map, (uint16_t)(next + 1), no);
        pwb(unit, (uint16_t)(0x0F + side), field_dir((int16_t)(pw(unit, pos) - 1), (int16_t)next));
        pww(unit, pos, (uint16_t)(next + 1));
        pwb(map, old, (uint8_t)(no + 1));
        pwb(half, (uint16_t)(0x0F + side), pb(unit, (uint16_t)(0x0F + side)));
        pww(half, pos, old);
    } else {
        pwb(map, pw(unit, pos), NB(move_rec, MR_UNDER));
        SETB(move_rec, MR_UNDER, pb(map, (uint16_t)(next + 1)));
        pwb(map, (uint16_t)(next + 1), no);
        pwb(unit, (uint16_t)(0x0F + side), field_dir((int16_t)(pw(unit, pos) - 1), (int16_t)next));
        pww(unit, pos, (uint16_t)(next + 1));
    }
    if (pw(unit, 4) & 0x1000)
        carrier_moved(no, side);
    if ((int16_t)NW(path_len, 0) > 0) {
        timer_set(4, (int8_t)pb(kind_of(unit), 0x17), 0);
        SETW(path_len, 0, NW(path_len, 0) - 1);
        return 0;
    }
    text = move_arrive(side, map);
    SETW(game_state, 4, (NW(game_state, 4) | (side ? 0x10 : 8)) & 0xFF7F);
    pww(unit, 4, pw(unit, 4) & 0xFEFF);
    if (pw(unit, 4) & 0x40) {
        uint8_t h = (uint8_t)(pw(unit, 4) & 0x80 ? no + 1 : no - 1);
        SETW(unit_flags, h * UNIT_SIZE, NW(unit_flags, h * UNIT_SIZE) & 0xFEFF);
    }
    return text ? text : 0xFFFF;
}
