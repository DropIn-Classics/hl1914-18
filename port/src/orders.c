/* orders.c - what a round's end does with the orders of the side that
 * moved: HL.EXE's attacks_run from S02E6:0E9C to 19B8, the loop over the
 * units by their byte 13h, translated over the program's memory for the
 * orders 2, 3, 4, 5, 0Bh, 0Dh and 11h.  What the original draws, plays and
 * waits for in between (the music, the views, the picture of a site
 * taken, the libraries loaded again) is left out, as in move_step.  port/README.md
 * says how this was compared with the original.
 */
#include "hl.h"

#define SITE_SIZE 0x1Cu     /* a record of `carriers` and the like */
#define SIDE_SIZE 0x17u     /* a record of `sides` */

/* addresses without a name in the hints yet (PENDING LEAD NAME; the names
 * are proposals): S331F:4CAA, a word order 4 sets to 1 (site_taken,
 * presumably read by the round's end that follows; not checked), and
 * S3303:0021, a byte set to 1 before the picture of a site taken
 * (take_picture; what reads it was not looked at) */
#define S_site_taken (HL_S331F + HL_BASE)
#define A_site_taken 0x4CAAu
#define S_take_picture (HL_S3303 + HL_BASE)
#define A_take_picture 0x0021u

/* S331F:0049, 004D, 0051, 0055: the column and row steps (four signed
 * bytes each) from a field to the four fields of a depot, for an even and
 * an odd column (foot_steps; PENDING LEAD NAME) */
#define S_foot_steps (HL_S331F + HL_BASE)
#define A_foot_cols_even 0x0049u
#define A_foot_rows_even 0x004Du
#define A_foot_cols_odd 0x0051u
#define A_foot_rows_odd 0x0055u
/* S331F:4C92 and 4C97: the grounds a prepared field and a depot's three
 * other fields get (inside amok_dat, its bytes 18h and 1Dh) */
#define A_prepared (0x4C92u - 0x4C7Au)
#define A_depot (0x4C97u - 0x4C7Au)
/* S331F:4CAE: the count of the depots built, the next record of
 * sites_100 (depot_count; PENDING LEAD NAME) */
#define S_depot_count (HL_S331F + HL_BASE)
#define A_depot_count 0x4CAEu

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

/* the site's record an order 4 or 5 names: the table by 4 (sites_40) and
 * 8 (sites_400) in the unit's word 17h, else sites_100, by its byte 15h */
static Far order_site(Far unit)
{
    uint16_t t = pw(unit, 0x17) & 4 ? A_sites_40 : pw(unit, 0x17) & 8 ? A_sites_400 : A_sites_100;
    Far p;
    p.seg = S_carriers;
    p.off = (uint16_t)(t + pb(unit, 0x15) * SITE_SIZE);
    return p;
}

/* S0E56:1A97 (footprint; PENDING LEAD NAME): the four fields of a depot
 * on `field` as map offsets into `neighbours`: the field, then by the
 * steps at S331F:0049 (an odd column: 0051).  No test of the map's edge. */
static void footprint(uint16_t field)
{
    int16_t w = (int16_t)NW(map_width, 0), f = (int16_t)((int16_t)field >> 1);
    int16_t col = (int16_t)(f % w), row = (int16_t)(f / w);
    uint16_t cols = col & 1 ? A_foot_cols_odd : A_foot_cols_even;
    uint16_t rows = col & 1 ? A_foot_rows_odd : A_foot_rows_even;
    int i;

    for (i = 0; i < 4; i++) {
        int16_t c = (int16_t)(col + (int8_t)frb(S_foot_steps, (uint16_t)(cols + i)));
        int16_t r = (int16_t)(row + (int8_t)frb(S_foot_steps, (uint16_t)(rows + i)));
        SETW(neighbours, 2 * i, (uint16_t)(c * 2 + w * 2 * r));
    }
}

/* S331F:0059 and 00C5 (aim_steps and aim_grounds; PENDING LEAD NAME):
 * by the column's parity (36h bytes each), the aim's column (12h) and row
 * (6) less the unit's plus 1, three pairs of signed column and row steps
 * from the unit's field, and at 00C5 with the same index the ground each
 * of the three fields gets (only the first byte of each pair is read) */
#define S_aim_tables (HL_S331F + HL_BASE)
#define A_aim_steps 0x0059u
#define A_aim_grounds 0x00C5u

/* S0E56:1B57 (aim_grounds_set; PENDING LEAD NAME): order 11h, a unit with
 * 2000h in its word 6.  Three fields around the unit's place, chosen by
 * where the aim lies, get new grounds when their ground has 8000h in its
 * word 0: those of the table at 00C5 while the aim's ground is below A5h,
 * else 51h.  No test of the map's edge, as in the original. */
static void aim_grounds_set(uint16_t place, uint16_t aim, Far map)
{
    int16_t w = (int16_t)NW(map_width, 0);
    int16_t p = (int16_t)((int16_t)place >> 1), q = (int16_t)((int16_t)aim >> 1);
    int16_t col = (int16_t)(p % w), row = (int16_t)(p / w);
    int16_t dc = (int16_t)(q % w - col + 1), dr = (int16_t)(q / w - row + 1);
    int undo = pb(map, (uint16_t)(q * 2)) >= 0xA5;
    int i;

    for (i = 0; i < 3; i++) {
        uint16_t at = (uint16_t)((col % 2) * 0x36 + dc * 0x12 + dr * 6 + i * 2);
        int16_t c = (int16_t)(col + (int8_t)frb(S_aim_tables, (uint16_t)(A_aim_steps + at)));
        int16_t r = (int16_t)(row + (int8_t)frb(S_aim_tables, (uint16_t)(A_aim_steps + at + 1)));
        uint16_t f = (uint16_t)(c * 2 + w * 2 * r);

        if (NW(ww1gnds_dat, pb(map, f) * 6) & 0x8000)
            pwb(map, f, undo ? 0x51 : frb(S_aim_tables, (uint16_t)(A_aim_grounds + at)));
    }
}

/* order 0Bh (S02E6:17C3): a field is prepared for a depot when its four
 * fields still have 8000h in their ground's word 0 and no unit on side
 * m's map; they get the grounds at S331F:4C92 and the unit's byte 0Ah
 * (what it has left to build with) goes down by 1 */
static void order_prepare(Far unit, uint16_t m, Far map)
{
    Far own = far_at(S_map_ptr, m ? A_map2_ptr : A_map_ptr);
    int i, free = 0;

    footprint(pw(unit, 0x11));
    for (i = 0; i < 4; i++) {
        uint16_t at = NW(neighbours, 2 * i);
        if (NW(ww1gnds_dat, pb(own, at) * 6) & 0x8000 && pb(own, (uint16_t)(at + 1)) > 0xF0)
            free++;
    }
    if (free != 4)
        return;
    for (i = 0; i < 4; i++)
        pwb(map, NW(neighbours, 2 * i), NB(amok_dat, A_prepared + i));
    pwb(unit, 0x0A, (uint8_t)(pb(unit, 0x0A) - 1));
}

/* order 0Dh (S02E6:189E): side a builds a depot on the unit's aim, the
 * next record of sites_100, with no stock; its fields get the grounds at
 * S331F:4C97 and the first that of a depot of side a (amok_dat byte 3 or
 * 4); the side's count of them goes up and the unit's byte 0Ah down */
static void order_build(Far unit, uint16_t a, Far map)
{
    uint8_t n = frb(S_depot_count, A_depot_count);
    Far rec;
    int i;

    fwb(S_depot_count, A_depot_count, (uint8_t)(n + 1));
    site_add_100(n, pw(unit, 0x11), a);
    SETB(sides, a * SIDE_SIZE + 4, NB(sides, a * SIDE_SIZE + 4) + 1);
    rec.seg = S_sites_100;
    rec.off = (uint16_t)(A_sites_100 + n * SITE_SIZE);
    pwb(rec, 0x17, 0);
    pwb(rec, 0x16, 0);
    footprint(pw(unit, 0x11));
    for (i = 1; i < 4; i++)
        pwb(map, NW(neighbours, 2 * i), NB(amok_dat, A_depot + i - 1));
    pwb(map, NW(neighbours, 0), NB(amok_dat, a ? 4 : 3));
    pwb(unit, 0x0A, (uint8_t)(pb(unit, 0x0A) - 1));
}

/* order 5 (S02E6:1071): a carrier in a site of its side leaves its goods
 * there: each unit of the side's row with 4 in its word 6 is freed and its
 * unit_count times 8 goes to the site's stock (byte 16h, 17h for the
 * second side) */
static void order_into_site(Far unit, uint16_t m)
{
    Far rec;
    uint8_t i;

    if (!(pw(unit, 4) & 0x1000))
        return;
    rec = order_site(unit);
    for (i = 0; i < 7; i++) {
        uint8_t u = pb(rec, (uint16_t)(m * 7 + i)), add;

        if (u > 0xF0 || !(NW(units, u * UNIT_SIZE + 6) & 4))
            continue;
        add = (uint8_t)(NB(unit_count, u * UNIT_SIZE) << 3);
        if (pb(rec, (uint16_t)(0x16 + m)) + add > 0x2710)    /* as the original has it */
            pwb(rec, (uint16_t)(0x16 + m), 0x10);
        else
            pwb(rec, (uint16_t)(0x16 + m), (uint8_t)(pb(rec, (uint16_t)(0x16 + m)) + add));
        unit_free(u, m);
        pwb(rec, (uint16_t)(m * 7 + i), 0xFF);
    }
}

/* order 4 (S02E6:11B1): side m takes the site; a is the other side */
static void order_take(Far unit, uint8_t no, uint16_t m, uint16_t a, Far map, uint8_t *result)
{
    Far rec = order_site(unit);
    uint16_t field = pw(rec, 0x0E), o = m & 1 ? 0 : 1, at;
    int16_t w = (int16_t)NW(map_width, 0), f;
    int i;

    SETW(game_state, 0, NW(game_state, 0) | 0x1000);
    fww(S_site_taken, A_site_taken, 1);
    view_to_field(a, field);
    /* the site's field is marked with 30h while the view is drawn and
     * the mark taken off again */
    f = (int16_t)((int16_t)field >> 1);
    at = (uint16_t)(f % w + ((f / w) << 6));
    SETB(field_bits, at, NB(field_bits, at) & 0xCF);
    fwb(S_take_picture, A_take_picture, 1);

    for (i = 0; i < 7; i++)
        pwb(rec, (uint16_t)(m * 7 + i), pb(rec, (uint16_t)(a * 7 + i)));
    for (i = 0; i < 7; i++)
        if (pb(rec, (uint16_t)(m * 7 + i)) > 0xF0)
            break;
    if (i == 7) {
        pwb(unit, 0x19, 0);
        unit_free(pb(rec, (uint16_t)(m * 7)), m);
    } else
        pwb(unit, 0x19, (uint8_t)i);

    /* every unit standing in the site goes over to side m */
    for (i = 0; i <= 0xF0; i++) {
        uint16_t flags = NW(unit_flags, i * UNIT_SIZE);
        uint16_t kind = (uint16_t)(A_unit_dat + NB(unit_kind, i * UNIT_SIZE) * KIND_SIZE + m + 0x3D);

        if (NW(unit_pos, i * UNIT_SIZE + 2 * m) != field || flags & 0x8000)
            continue;
        if (!(flags & 2))
            SETB(sides, o * SIDE_SIZE + 2, NB(sides, o * SIDE_SIZE + 2) - 1);
        SETW(unit_flags, i * UNIT_SIZE, (flags & 0xFFFC) | m);
        SETB(unit_serial, i * UNIT_SIZE, frb(S_unit_dat, kind));
        fwb(S_unit_dat, kind, (uint8_t)(frb(S_unit_dat, kind) + 2));
        SETB(sides, m * SIDE_SIZE + 2, NB(sides, m * SIDE_SIZE + 2) + 1);
        if (NW(unit_flags, i * UNIT_SIZE) & 0x1000) {
            uint16_t c = (uint16_t)(A_carriers + NB(units, i * UNIT_SIZE + 0x0A) * SITE_SIZE + 0x19);
            fww(S_carriers, c, (uint16_t)((frw(S_carriers, c) & 0xFFFC) | m));
        }
    }

    /* the count of the site's sort goes over and its field gets the
     * ground of the new owner (amok_dat) */
    if (pw(rec, 0x19) & 8) {
        if (!(pw(rec, 0x19) & 2))
            SETB(sides, o * SIDE_SIZE + 3, NB(sides, o * SIDE_SIZE + 3) - 1);
        SETB(sides, m * SIDE_SIZE + 3, NB(sides, m * SIDE_SIZE + 3) + 1);
        pwb(map, field, NB(amok_dat, m & 1 ? 1 : 0));
    } else if (pw(rec, 0x19) & 0x10) {
        if (!(pw(rec, 0x19) & 2))
            SETB(sides, o * SIDE_SIZE + 4, NB(sides, o * SIDE_SIZE + 4) - 1);
        SETB(sides, m * SIDE_SIZE + 4, NB(sides, m * SIDE_SIZE + 4) + 1);
        pwb(map, field, NB(amok_dat, m & 1 ? 4 : 3));
    } else if (pw(rec, 0x19) & 4) {
        *result = (uint8_t)m;       /* the headquarters: the battle is won */
        pwb(map, field, NB(amok_dat, m & 1 ? 7 : 6));
    }
    pww(rec, 0x19, (uint16_t)((pw(rec, 0x19) & 0xFFFC) | m));
    pwb(rec, 0x18, 5);

    /* the unit goes inside */
    pwb(rec, (uint16_t)(m * 7 + pb(unit, 0x19)), no);
    pwb(map, pw(unit, (uint16_t)(0x0B + 2 * m)), 0xF4);
    pww(unit, (uint16_t)(0x0B + 2 * m), field);
    pww(unit, 4, (uint16_t)((pw(unit, 4) & 0xFEFF) | 0x4200));
    if (pw(unit, 4) & 0x1000)
        carrier_moved(no, m);
}

/* S02E6:0E9C to 19B8: each unit in use by its order (byte 13h).  m is the
 * side that moved ([BP-2] of attacks_run), a the other (DI), map m's map
 * ([BP-6]); *result is attacks_run's result byte ([BP-7]), set to m when
 * a headquarters is taken.  Returns the number of units with an order the
 * port does not carry out (only 1 now; read, not translated). */
int orders_run(uint16_t m, uint16_t a, Far map, uint8_t *result)
{
    int left = 0;
    uint16_t no;

    for (no = 0; no <= 0xF0; no++) {
        Far unit = unit_at((uint8_t)no);

        if (pw(unit, 4) & 0x8000)
            continue;
        switch (pb(unit, 0x13)) {
        case 4:
            order_take(unit, (uint8_t)no, m, a, map, result);
            break;
        case 5:
            order_into_site(unit, m);
            break;
        case 0x0B:
            order_prepare(unit, m, map);
            break;
        case 0x0D:
            order_build(unit, a, map);
            break;
        case 0x11:      /* S02E6:1986: the place on side a's map */
            aim_grounds_set(pw(unit, (uint16_t)(0x0B + 2 * a)), pw(unit, 0x11), map);
            break;
        case 1:
            left++;
            break;
        default:        /* 2, 3 and anything else: nothing more */
            break;
        }
    }
    return left;
}

/* S131D:1685 (side_income): byte 16h plus the side of every record of
 * sites_100 and sites_400 whose word 19h has the side in its two low bits,
 * ten each, used or not (the original has no other test), and that byte
 * of the side's record of sites_40 */
static uint16_t side_income(uint16_t side)
{
    uint16_t sum = 0, n;

    for (n = 0; n < 10; n++)
        if ((NW(sites_100, n * SITE_SIZE + 0x19) & 3) == side)
            sum = (uint16_t)(sum + NB(sites_100, n * SITE_SIZE + 0x16 + side));
    for (n = 0; n < 10; n++)
        if ((NW(sites_400, n * SITE_SIZE + 0x19) & 3) == side)
            sum = (uint16_t)(sum + NB(sites_400, n * SITE_SIZE + 0x16 + side));
    return (uint16_t)(sum + NB(sites_40, side * SITE_SIZE + 0x16 + side));
}

/* S131D:1745 (income_add): side_income added to the side's word of
 * side_points; game_loop calls it at a round's end after attacks_run */
void income_add(uint16_t side)
{
    SETW(side_points, side * 2, NW(side_points, side * 2) + side_income(side));
}
