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

/* S10E1:05D1 (move_start): the button again on the field move_aim took.
 * The unit in word 1Eh of the view becomes move_rec's walker; 1 is
 * returned when its word 11h is the cursor's field (the view's word 0),
 * else 12h, with 8 more for 40h in its unit_flags.  Only with 1: the unit
 * goes into the side's list of movers (the far list at 11h of its record
 * of `sides`, its count at byte 15h), its word 11h becomes its place on
 * the side's map, path_len goes down by 2, move_rec's bytes 2 and 3 are
 * FFh (byte 2 the place's byte 1Bh of the record at view+24h for a view
 * in step 3 of a site), game_state's word 4 gets 80h and 100h, a timer of
 * kind 4 is set, and site_taken becomes 0. */
uint16_t move_start(Far view, uint16_t side)
{
    uint8_t no = (uint8_t)pw(view, 0x1E);
    Far unit = unit_at(no);
    uint16_t r = pw(unit, 0x11) == pw(view, 0) ? 1 : 0x12, s = (uint16_t)(A_sides + side * 0x17);
    Far list;

    if (pw(unit, 4) & 0x40)
        r |= 8;
    SETB(move_rec, MR_UNIT, no);
    if (!(r & 1))
        return r;
    list = far_at(S_sides, (uint16_t)(s + 0x11));
    fwb(list.seg, (uint16_t)(list.off + frb(S_sides, (uint16_t)(s + 0x15))), no);
    fwb(S_sides, (uint16_t)(s + 0x15), (uint8_t)(frb(S_sides, (uint16_t)(s + 0x15)) + 1));
    pww(unit, 0x11, pw(unit, (uint16_t)(0x0B + side * 2)));
    SETW(path_len, 0, NW(path_len, 0) - 2);
    SETB(move_rec, MR_UNDER2, 0xFF);
    SETB(move_rec, MR_UNDER, 0xFF);
    if (pb(view, 0x19) == 3) {
        Far rec = far_at(view.seg, (uint16_t)(view.off + 0x24));
        SETB(move_rec, MR_UNDER, pb(rec, 0x1B));
    }
    SETW(game_state, 4, NW(game_state, 4) | 0x80);
    timer_set(4, 0, 0);
    SETW(game_state, 4, NW(game_state, 4) | 0x100);
    SETW(site_taken, 0, 0);
    return r;
}

/* S0AE0:02E3 (attack_unmark): after an attack was ordered or given up,
 * the side's marks of the fields that can be attacked (4 in field_bits,
 * 8 for the second side) are cleared and the view's unit (word 1Eh) loses
 * 100h in unit_flags (unit_mark); the view is drawn again (left out). */
void attack_unmark(Far view, uint16_t side)
{
    field_bits_clear(side ? 8 : 4);
    unit_mark((uint8_t)pb(view, 0x1E), 0x100, side, 0);
}

/* S0AE0:0008 (attack_order): the button on the cursor's field (the view's
 * word 0) while the view's unit (word 1Eh) chooses what to attack, on map
 * `map`.  Only a field marked for the side in field_bits (4 or 8 under 55h
 * or AAh) takes an order, else nothing (text 45h).  A unit with 4000h in
 * word 6 gets 0Dh on a field whose ground is amok_dat's byte 18h (while
 * fewer than 0Ah depots were built; text 46h, else 47h and nothing) and
 * 0Bh on others; one with 2000h gets 11h (text 6Eh); any other unit
 * attacks a unit on the field (9, its byte 14h 2, onto the side's list of
 * movers; text 49h), with no unit there nothing (45h).  With an order the
 * unit is marked 200h (unit_mark) and gets the field in word 11h, and
 * game_state's word 4 gets 100h with site_taken 0.  After the text
 * (show_message) game_state's word 4 always gets 8 (10h for the second
 * side).  Returns 1 when an order was given. */
uint16_t attack_order(Far view, Far map, uint16_t side)
{
    uint8_t no = (uint8_t)pw(view, 0x1E), order = 0;
    Far unit = unit_at(no);
    uint16_t field = pw(view, 0), given = 0, taken = 0xFF, text = 0x45;
    int16_t p = (int16_t)field >> 1, w = (int16_t)NW(map_width, 0);
    uint8_t bits;

    if (pw(unit, 6) & 0x4000)
        order = 0x0C;
    else if (pw(unit, 6) & 0x2000)
        order = 0x10;
    else if (frb(map.seg, (uint16_t)(map.off + field + 1)) <= 0xF0)
        order = 9;
    bits = (uint8_t)(NB(field_bits, (p % w) + (p / w) * 0x40) & (side ? 0xAA : 0x55));
    if (!(bits & 0x0C))
        order = 0;
    if (order == 0x0C) {
        if (frb(map.seg, (uint16_t)(map.off + field)) == frb(S_amok_dat, A_amok_dat + 0x18)) {
            if (NB(depot_count, 0) < 0x0A)
                order = 0x0D;
            else {
                order = 0;
                text = 0x47;
            }
        } else
            order = 0x0B;
    }
    if (order) {
        unit_mark(no, 0x200, side, 1);
        pww(unit, 0x11, field);
        if (order == 9) {
            uint16_t s = (uint16_t)(A_sides + side * 0x17);
            Far list = far_at(S_sides, (uint16_t)(s + 0x11));
            pwb(unit, 0x14, 2);
            pwb(unit, 0x13, 9);
            fwb(list.seg, (uint16_t)(list.off + frb(S_sides, (uint16_t)(s + 0x15))), no);
            fwb(S_sides, (uint16_t)(s + 0x15), (uint8_t)(frb(S_sides, (uint16_t)(s + 0x15)) + 1));
        } else
            pwb(unit, 0x13, order == 0x10 ? 0x11 : order);
        text = order == 9 ? 0x49 : order == 0x10 ? 0x6E : 0x46;
        taken = 0;
        given = 1;
    }
    show_message(text, side);
    SETW(game_state, 4, NW(game_state, 4) | (side ? 0x10 : 8));
    if (taken != 0xFF) {
        SETW(game_state, 4, NW(game_state, 4) | 0x100);
        SETW(site_taken, 0, taken);
    }
    return given;
}

/* S10E1:000F (move_done): the end of choosing a move for the view's unit
 * (word 1Eh) on map `map`: the side's marks 1 (2 for the second side) are
 * cleared in field_bits.  With 10h in `flags` the move is given up: 100h
 * leaves the unit's flags (and its other half's: the next unit with 80h,
 * else the one before), the marks 40h (80h) are cleared, the view goes to
 * step 19h of state 0 with pointer 5, game_state's word 4 gets 100h and
 * site_taken becomes 1.  A view in step 3 (a unit moving out of a site or
 * carrier) is taken back from view_copy and goes to step 9 of state 2;
 * with 10h the unit goes back into its place, its place on the side's map
 * becomes the record's field again: F4h is written into the map at the
 * old place, and on a carrier the record's byte 1Bh at the new one.  The
 * drawing is left out. */
void move_done(Far view, Far map, uint16_t side, uint16_t flags)
{
    uint8_t no = (uint8_t)pw(view, 0x1E);
    Far unit = unit_at(no);
    uint16_t pos = (uint16_t)(0x0B + side * 2);

    field_bits_clear(side ? 2 : 1);
    if (flags & 0x10) {
        pww(unit, 4, pw(unit, 4) & 0xFEFF);
        if (pw(unit, 4) & 0x40) {
            Far half = unit_at((uint8_t)(pw(unit, 4) & 0x80 ? no + 1 : no - 1));
            pww(half, 4, pw(half, 4) & 0xFEFF);
        }
        field_bits_clear(side ? 0x80 : 0x40);
        pwb(view, 0x17, 0);
        pwb(view, 0x18, 0x19);
        pwb(view, 0x1B, 5);
        SETW(game_state, 4, NW(game_state, 4) | 0x100);
        SETW(site_taken, 0, 1);
    }
    if (pb(view, 0x19) != 3)
        return;
    if (flags & 0x10) {
        Far rec = far_at(view.seg, (uint16_t)(view.off + 0x24));
        fwb(map.seg, (uint16_t)(map.off + pw(unit, pos)), 0xF4);
        pww(unit, pos, pw(rec, 0x0E));
        if ((rec.off | rec.seg) && (pw(rec, 0x19) & 0x20))
            fwb(map.seg, (uint16_t)(map.off + pw(unit, pos)), pb(rec, 0x1B));
    }
    view_copy(view, 1);
    if (flags & 0x10) {
        Far rec = far_at(view.seg, (uint16_t)(view.off + 0x24));
        pwb(rec, (uint16_t)(side * 7 + pw(view, 0x0E)), no);
    }
    pwb(view, 0x17, 2);
    pwb(view, 0x18, 9);
    pwb(view, 0x19, 0);
    pwb(view, 0x1B, 5);
    pww(view, 0x0C, pw(view, 4));
    pww(view, 0x0E, pw(view, 6));
}

/* S0E56:1217 (marks_erase): the view's fields marked with `mask` drawn
 * without the marks (left out), then the mask cleared in field_bits */
static void marks_erase(uint8_t mask)
{
    field_bits_clear(mask);
}

/* S10E1:02EB (move_aim): the button on the cursor's field (the view's
 * word 0) while the view's unit (word 1Eh) chooses where to move, on map
 * `map`.  Refused (12h): a field not marked 1 or 2 for the side, the
 * unit's own field, a ground with 4000h in its word 0 for a unit with 10h
 * in unit_flags, one move_target gives a text for (shown; game_state's
 * word 4 gets 8 or 10h), and one path_find finds no way to (text 48h, the
 * marks 40h or 80h cleared).  Else the marks 1 (2) are cleared but for
 * the way's fields (path_len of them at 1F40h of the side's work buffer),
 * the field goes to the unit's word 11h and 1 is returned.  8 is added for
 * a unit with 40h in unit_flags.  The unit is move_rec's walker either
 * way.  The drawing is left out. */
uint16_t move_aim(Far view, Far map, uint16_t side)
{
    uint8_t no = (uint8_t)pw(view, 0x1E), mark = side ? 2 : 1;
    Far unit = unit_at(no);
    uint16_t field = pw(view, 0), r = 1, pos = (uint16_t)(0x0B + side * 2);
    int16_t p = (int16_t)field >> 1, w = (int16_t)NW(map_width, 0);
    Far buf = far_at(S_sides, (uint16_t)(A_sides + side * 0x17 + 0x0D));

    if (!(NB(field_bits, (p % w) + (p / w) * 0x40) & (side ? 0xAA : 0x55) & 3))
        r = 0x12;
    if (pw(unit, pos) == field + 1)
        r = 0x12;
    if ((pw(unit, 4) & 0x10)
        && (frw(S_ww1gnds_dat, (uint16_t)(A_ww1gnds_dat + frb(map.seg, (uint16_t)(map.off + field)) * 6)) & 0x4000))
        r = 0x12;
    SETB(move_rec, MR_UNIT, no);
    if (r & 1) {
        uint16_t text = move_target(field, side, map);
        if (text) {
            show_message(text, side);
            SETW(game_state, 4, NW(game_state, 4) | (side ? 0x10 : 8));
            r = 0x12;
        }
    }
    if (pw(unit, 4) & 0x40)
        r |= 8;
    if (!(r & 1))
        return r;
    if (path_find(buf, no, (int16_t)(pw(unit, pos) - 1), (int16_t)field, side, map)) {
        field_bits_clear(side ? 0x80 : 0x40);
        show_message(0x48, side);
        SETW(game_state, 4, NW(game_state, 4) | (side ? 0x10 : 8));
        return 0x12;
    }
    {
        int16_t n = (int16_t)NW(path_len, 0);
        marks_erase(mark);
        field_bits_clear(mark);
        while (n-- > 0) {
            int16_t q = (int16_t)frw(buf.seg, (uint16_t)(buf.off + 0x1F40 + 2 * n)) >> 1;
            SETB(field_bits, (q % w) + (q / w) * 0x40, NB(field_bits, (q % w) + (q / w) * 0x40) | mark);
        }
    }
    pww(unit, 0x11, field);
    return r;
}

/* the unit's order taken back (byte 14h loses 1, word 4 4200h) */
static void undo_marks(Far unit)
{
    pwb(unit, 0x14, (uint8_t)(pb(unit, 0x14) & 0xFE));
    pww(unit, 4, pw(unit, 4) & 0xBDFF);
}

/* S10E1:1514 (order_undo): the arrival a unit's byte 13h records (as
 * move_arrive wrote it) taken back on `side`'s map, for a unit attacks_run
 * frees: 1 puts the taken unit (byte 16h) back on the unit's field with its
 * flags (word 17h) and empties its place in the carrier's row; 2 empties
 * the unit's place in the carrier's row (record by byte 15h); 3 and 4 empty
 * the unit's field (and its other half's); 5 empties its place in the
 * site's row (sites_40 by 4, sites_400 by 8 in word 17h, else sites_100).
 * Byte 13h becomes 0. */
void order_undo(uint8_t no, Far map, uint16_t side)
{
    Far unit = unit_at(no);
    uint16_t pos = pw(unit, (uint16_t)(0x0B + 2 * side));

    switch (pb(unit, 0x13)) {
    case 1: {
        uint8_t o = pb(unit, 0x16);

        pwb(map, pos, o);
        SETW(unit_pos, o * UNIT_SIZE + 2 * side, pos);
        SETW(unit_flags, o * UNIT_SIZE, pw(unit, 0x17));
        SETB(unit_marks, o * UNIT_SIZE, 0);
        pwb(carrier_of(unit), (uint16_t)(side * 7 + pb(unit, 0x19)), 0xFF);
        undo_marks(unit);
        break;
    }
    case 2:
        pwb(site_at(A_carriers, pb(unit, 0x15)), (uint16_t)(side * 7 + pb(unit, 0x19)), 0xFF);
        undo_marks(unit);
        break;
    case 3:
    case 4:
        pwb(map, pos, 0xFF);
        if (pw(unit, 4) & 0x40) {
            uint8_t h = (uint8_t)(pw(unit, 4) & 0x80 ? no + 1 : no - 1);

            pwb(map, NW(unit_pos, h * UNIT_SIZE + 2 * side), 0xFF);
            undo_marks(unit_at(h));
        }
        undo_marks(unit);
        break;
    case 5: {
        uint16_t t = pw(unit, 0x17) & 4 ? A_sites_40 : pw(unit, 0x17) & 8 ? A_sites_400 : A_sites_100;

        pwb(site_at(t, pb(unit, 0x15)), (uint16_t)(side * 7 + pb(unit, 0x19)), 0xFF);
        undo_marks(unit);
        break;
    }
    default:
        break;
    }
    pwb(unit, 0x13, 0);
}
