/* orders.c - what a round's end does with the orders of the side that
 * moved: HL.EXE's attacks_run from S02E6:0E9C to 19B8, the loop over the
 * units by their byte 13h, for the orders 1 to 5, 0Bh, 0Dh and 11h, and
 * the rest of attacks_run around it: the fights, the freeing of the units
 * they leave marked, the round's end; translated over the program's
 * memory.  What the original draws, plays and waits for in between (the
 * music, the views, the fight's pictures and a site taken's, the libraries
 * loaded again) is left out, as in move_step.  port/README.md says how
 * this was compared with the original.
 */
#include "hl.h"

#define SITE_SIZE 0x1Cu     /* a record of `carriers` and the like */
#define SIDE_SIZE 0x17u     /* a record of `sides` */
#define VIEW_SIZE 0x31u     /* a record of `views` */
#define S_GAME (HL_S331F + HL_BASE)     /* the segment of the game's state */

/* S331F:4CAA site_taken (a word order 4 sets to 1) and S3303:0021
 * take_picture (set to 1 before the picture of a site taken): names.h */

/* S331F:0049, 004D, 0051, 0055: the column and row steps (four signed
 * bytes each) from a field to the four fields of a depot, for an even and
 * an odd column (foot_steps in the hints) */
#define S_foot_steps (HL_S331F + HL_BASE)
#define A_foot_cols_even 0x0049u
#define A_foot_rows_even 0x004Du
#define A_foot_cols_odd 0x0051u
#define A_foot_rows_odd 0x0055u
/* S331F:4C92 and 4C97: the grounds a prepared field and a depot's three
 * other fields get (inside amok_dat, its bytes 18h and 1Dh) */
#define A_prepared (0x4C92u - 0x4C7Au)
#define A_depot (0x4C97u - 0x4C7Au)
/* S331F:4CAE depot_count, the next record of sites_100: names.h */

void (*orders_hook)(int at, int n);
int orders_fought;      /* a fight was shown in this attacks_run ([BP-0Eh]) */

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

/* S0E56:1A97 (footprint in the hints): the four fields of a depot
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

/* S331F:0059 and 00C5 (aim_steps and aim_grounds in the hints):
 * by the column's parity (36h bytes each), the aim's column (12h) and row
 * (6) less the unit's plus 1, three pairs of signed column and row steps
 * from the unit's field, and at 00C5 with the same index the ground each
 * of the three fields gets (only the first byte of each pair is read) */
#define S_aim_tables (HL_S331F + HL_BASE)
#define A_aim_steps 0x0059u
#define A_aim_grounds 0x00C5u

/* S0E56:1B57 (aim_grounds_set in the hints): order 11h, a unit with
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

/* S131D:1426 (view_cover(side)): the 9 by 7 fields of the side's view as
 * view_dim places them, each drawn over with picture 0Ch of the list at
 * S331F:0167 in colours DDh on.  A drawing routine; here until draw.c
 * has it. */
static void view_cover(uint16_t side)
{
    Far pic = pfar(far_add(far_at(S_GAME, 0x0167), 0x30), 0);
    uint16_t x = side ? 0xA0 : 0, col, row;

    for (col = 0; col < 9; col++, x = (uint16_t)(x + 0x10)) {
        uint16_t y = col & 1 ? 0x0C : 0;

        for (row = 0; row < 7; row++, y = (uint16_t)(y + 0x18))
            pic_draw(x, y, pic, 0xDD);
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
    if (orders_hook)
        orders_hook(ORDERS_TAKEN, orders_fought);
    /* S02E6:1249 to 1373: the other side's view on the site, its field
     * marked with 30h while the marks are drawn and the mark taken off
     * again, the taker's view covered, shown, wait_frames(1Eh); the
     * colours back (fade_in after a fight in this attacks_run, else
     * fade_back; with the fights' pictures off fade_back only without a
     * fight before), then the site's picture over the taker's half
     * (event_picture CAh for the first side, CBh for the second, at the
     * work buffer plus 1064h) and the battle's libraries loaded again
     * over it (libs_start, as after a fight's pictures) */
    view_to_field(a, field);
    view_draw(NW(views, a * VIEW_SIZE + 2), a);
    f = (int16_t)((int16_t)field >> 1);
    at = (uint16_t)(f % w + ((f / w) << 6));
    SETB(field_bits, at, NB(field_bits, at) | 0x30);
    marks_draw(NW(views, a * VIEW_SIZE + 2), a);
    SETB(field_bits, at, NB(field_bits, at) & 0xCF);
    view_cover(m);
    page_flip();
    page_copy();
    if (orders_hook)
        orders_hook(ORDERS_WAIT, 0x1F);
    if (!NB(no_fight_pics, 0) && orders_fought) {
        fade_in();
        if (orders_hook)
            orders_hook(ORDERS_FADE, 0);
    } else if (!NB(no_fight_pics, 0) || !orders_fought) {
        fade_back(game_mem.save_buf);
        if (orders_hook)
            orders_hook(ORDERS_WAIT, 0x60);
    }
    fwb(S_take_picture, A_take_picture, 1);
    event_picture(far_add(game_mem.work, 0x1064), game_mem.work, m ? 0xCB : 0xCA, 0);
    {
        Far p = game_mem.libs;

        libs_start(&p);
    }
    if (orders_hook)
        orders_hook(ORDERS_TAKEN_BACK, 0);

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

/* order 1 (S02E6:0ED9): the unit took the unit in its byte 16h into its
 * carrier's row.  A unit freed meanwhile only empties its place in the
 * row; else it goes over to side m: the sides' counts of units (byte 2)
 * change unless it was already m's (same_side) or of no side (2), and it
 * gets the next serial number of its kind for side m (unit_dat byte
 * 3Dh plus the side, raised by 2), as in order_take. */
static void order_capture(Far unit, uint16_t m)
{
    uint8_t o = pb(unit, 0x16);
    uint16_t other = (uint16_t)(m ? 0 : 1), flags = NW(unit_flags, o * UNIT_SIZE);
    uint16_t kind;

    if (flags & 0x8000) {
        fwb(S_carriers, (uint16_t)(A_carriers + (int8_t)pb(unit, 0x0A) * SITE_SIZE + m * 7 + pb(unit, 0x19)), 0xFF);
        return;
    }
    if (!same_side(pw(unit, 4), flags) && !(flags & 2))
        SETB(sides, other * SIDE_SIZE + 2, NB(sides, other * SIDE_SIZE + 2) - 1);
    if (!same_side(pw(unit, 4), flags) || flags & 2)
        SETB(sides, m * SIDE_SIZE + 2, NB(sides, m * SIDE_SIZE + 2) + 1);
    SETW(unit_flags, o * UNIT_SIZE, (flags & 0xFFFC) | m);
    kind = (uint16_t)(A_unit_dat + NB(unit_kind, o * UNIT_SIZE) * KIND_SIZE + m + 0x3D);
    SETB(unit_serial, o * UNIT_SIZE, frb(S_unit_dat, kind));
    fwb(S_unit_dat, kind, (uint8_t)(frb(S_unit_dat, kind) + 2));
}

/* S02E6:0E9C to 19B8: each unit in use by its order (byte 13h).  m is the
 * side that moved ([BP-2] of attacks_run), a the other (DI), map m's map
 * ([BP-6]); *result is attacks_run's result byte ([BP-7]), set to m when
 * a headquarters is taken. */
void orders_run(uint16_t m, uint16_t a, Far map, uint8_t *result)
{
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
            order_capture(unit, m);
            break;
        default:        /* 2, 3 and anything else: nothing more */
            break;
        }
    }
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

/* S02E6:23F0 (rows_sync): for each of the n records of the table without
 * 8000h and 2 in word 19h, a record of side m (same_side) has m's row of
 * seven copied to a's row, a freed unit (8000h in unit_flags) becoming FFh
 * in both; any other record has a's row copied to m's.  An empty place
 * (FFh) is looked up as a unit too, as the original does. */
static void rows_sync(uint16_t table, uint16_t a, uint16_t m, uint8_t n)
{
    uint8_t r, p;

    for (r = 0; r < n; r++) {
        Far rec;

        rec.seg = S_carriers;
        rec.off = (uint16_t)(table + r * SITE_SIZE);
        if (pw(rec, 0x19) & 0x8002)
            continue;
        for (p = 0; p < 7; p++) {
            if (same_side(m, pw(rec, 0x19))) {
                uint8_t u = pb(rec, (uint16_t)(m * 7 + p));

                if (NW(unit_flags, u * UNIT_SIZE) & 0x8000) {
                    pwb(rec, (uint16_t)(m * 7 + p), 0xFF);
                    u = 0xFF;
                }
                pwb(rec, (uint16_t)(a * 7 + p), u);
            } else
                pwb(rec, (uint16_t)(m * 7 + p), pb(rec, (uint16_t)(a * 7 + p)));
        }
    }
}

/* S02E6:2B28 (units_drop_empty): 8000h in unit_flags of every unit in use
 * whose unit_count is 0 */
static void units_drop_empty(void)
{
    uint16_t u;

    for (u = 0; u <= 0xF0; u++) {
        Far unit = unit_at((uint8_t)u);

        if (!(pw(unit, 4) & 0x8000) && pb(unit, 2) == 0)
            pww(unit, 4, pw(unit, 4) | 0x8000);
    }
}

/* the kind's bytes 3 and 4 (kind word 0Fh with 4: byte 3 of the unit up
 * to byte 4 of the kind, else unit_count up to byte 3): a unit that mends
 * by itself at a round's end */
static void unit_mend(Far unit)
{
    uint16_t k = (uint16_t)(A_unit_dat + pb(unit, 8) * KIND_SIZE);

    if (frw(S_unit_dat, (uint16_t)(k + 0x0F)) & 4) {
        if (frb(S_unit_dat, (uint16_t)(k + 4)) > pb(unit, 3))
            pwb(unit, 3, (uint8_t)(pb(unit, 3) + 1));
    } else if (frb(S_unit_dat, (uint16_t)(k + 3)) > pb(unit, 2))
        pwb(unit, 2, (uint8_t)(pb(unit, 2) + 1));
}

/* S331F:0153 (far pointer to a far pointer for each ground), 4CB3 (a
 * count of ticks attacks_run waits for) and 4CB7 (a dword fight_rec gets
 * at 3Ah; rand_seed's seed is made from it) */
#define A_ground_pics 0x0153u
#define A_fight_wait 0x4CB3u
#define A_fight_seed 0x4CB7u

static void rec_far(uint16_t o, uint16_t seg, uint16_t off)
{
    fww(S_fight_rec, (uint16_t)(A_fight_rec + o), off);
    fww(S_fight_rec, (uint16_t)(A_fight_rec + o + 2), seg);
}

/* the count a fight takes off: byte 3 for a kind with 4 in word 0Fh, else
 * unit_count */
static uint8_t unit_left(uint8_t u)
{
    uint16_t k = (uint16_t)(A_unit_dat + NB(unit_kind, u * UNIT_SIZE) * KIND_SIZE);

    return NB(units, u * UNIT_SIZE + (frw(S_unit_dat, (uint16_t)(k + 0x0F)) & 4 ? 3 : 2));
}

/* the first half of a two-part unit (40h without 80h is the second) */
static uint8_t first_half(uint8_t u)
{
    uint16_t f = NW(unit_flags, u * UNIT_SIZE);

    return (uint8_t)(f & 0x40 && !(f & 0x80) ? u - 1 : u);
}

/* fight_rec's part for one unit: the unit, its kind, its field's record
 * in ww1gnds_dat, its number (at 8 + which) and its field's ground
 * picture (1Eh, the target's 22h) */
static void rec_unit(uint16_t which, uint8_t u, uint16_t field, Far map)
{
    uint16_t g = pb(map, field);
    Far pics = far_at(S_GAME, A_ground_pics), pic;

    rec_far(which ? 4 : 0, S_units, (uint16_t)(A_units + u * UNIT_SIZE));
    rec_far(which ? 0x0E : 0x0A, S_unit_dat, (uint16_t)(A_unit_dat + NB(unit_kind, u * UNIT_SIZE) * KIND_SIZE));
    rec_far(which ? 0x16 : 0x12, S_ww1gnds_dat, (uint16_t)(A_ww1gnds_dat + g * 6));
    fwb(S_fight_rec, (uint16_t)(A_fight_rec + 8 + which), u);
    pics = ptr_norm((Far){ (uint16_t)(pics.seg + (uint16_t)(((uint32_t)g * 4) >> 4)), (uint16_t)(pics.off + ((g * 4) & 0x0F)) });
    pic = far_at(pics.seg, pics.off);
    rec_far(which ? 0x22 : 0x1E, pic.seg, pic.off);
}

/* fight_show (S2474:000E) after fight_losses: fight_pictures with what
 * fight_rec says of the two units and their fields (the units' word 4
 * bit 0 and byte 8, the fields' grounds' byte 5, whether they are
 * neighbours), S37FF:0130, pics_winter and the counts kept at S3AEC:000C
 * to 000F, at attacks_run's work buffer plus 2710h.  0F08h is the
 * original's stack pointer at fight_pictures' entry (in the dumps). */
static void fight_show(Far rec)
{
    FightPics a;
    Far u0 = pfar(rec, 0), u1 = pfar(rec, 4);

    a.near0 = pw(rec, 0x2E) == 0;
    a.standing = pw(u0, 4) & 1;
    a.show = frb(HL_S37FF + HL_BASE, 0x0130);
    a.winter = NB(pics_winter, 0);
    a.kind0 = (uint16_t)(pb(u0, 8) + 1);
    a.kind1 = (uint16_t)(pb(u1, 8) + 1);
    a.ground0 = pb(pfar(rec, 0x12), 5);
    a.ground1 = pb(pfar(rec, 0x16), 5);
    a.count0 = frb(S_fight_rec_ptr, 0x0F);
    a.count1 = frb(S_fight_rec_ptr, 0x0E);
    a.after0 = frb(S_fight_rec_ptr, 0x0D);
    a.after1 = frb(S_fight_rec_ptr, 0x0C);
    a.none.seg = a.none.off = 0;
    a.work = game_mem.work;
    a.place = far_add(game_mem.work, 0x2710);
    a.sp = 0x0F08;
    fight_pictures(&a);
}

/* S02E6:208E's blasts counted down and the fields put back (the loop at
 * S02E6:0B61, when the fights are not pictures): each turn every blast
 * whose byte 2 is not negative counts down, and one that goes below 0
 * gets its field's two bytes back and is drawn again (field_draw); a
 * page shown each turn; a blast whose time is below blast_time is drawn
 * before it counts down (S02E6:225C, blast_draw).  Left out: the waits. */
static void maps_swap(void);

static void blasts_run(Far map, uint16_t m)
{
    int active;

    do {
        uint16_t i;

        if (orders_hook)
            orders_hook(ORDERS_BLAST, 0);
        active = 0;
        for (i = 0; i < 4; i++) {
            uint16_t b = (uint16_t)(i * BLAST_SIZE);
            int8_t t = (int8_t)NB(blasts, b + 2);

            if (t < 0)
                continue;
            active++;
            if ((uint8_t)t < NB(blast_time, 0))
                blast_draw(i, map, NW(views, m * VIEW_SIZE + 2), m);
            SETB(blasts, b + 2, t - 1);
            if (t - 1 < 0) {
                uint8_t st = NB(views, m * VIEW_SIZE + 0x17);

                pwb(map, NW(blasts, b), NB(blasts, b + 3));
                pwb(map, (uint16_t)(NW(blasts, b) + 1), NB(blasts, b + 5));
                /* the field drawn again with the view's state 0 and the
                 * maps exchanged */
                SETB(views, m * VIEW_SIZE + 0x17, 0);
                maps_swap();
                field_draw(m, NW(blasts, b));
                maps_swap();
                SETB(views, m * VIEW_SIZE + 0x17, st);
            }
        }
        page_flip();
        page_copy();
    } while (active);
}

/* S02E6:2556 (carrier_losses): each unit in the row `side` of the
 * carrier's record loses `loss`; one with no more than that is marked 4
 * in unit_marks (freed after the fights) and its place emptied */
static void carrier_losses(uint16_t side, uint8_t carrier, uint8_t loss)
{
    uint16_t rec = (uint16_t)(A_carriers + (int8_t)NB(units, carrier * UNIT_SIZE + 0x0A) * SITE_SIZE);
    uint8_t i;

    for (i = 0; i < 7; i++) {
        uint8_t u = frb(S_carriers, (uint16_t)(rec + side * 7 + i));

        if (u > 0xF0)
            continue;
        if (NB(unit_count, u * UNIT_SIZE) <= loss) {
            SETB(unit_marks, u * UNIT_SIZE, NB(unit_marks, u * UNIT_SIZE) | 4);
            fwb(S_carriers, (uint16_t)(rec + side * 7 + i), 0xFF);
        } else
            SETB(unit_count, u * UNIT_SIZE, NB(unit_count, u * UNIT_SIZE) - loss);
    }
}

/* experience (byte 1) raised by n, at most 6 */
static void exp_add(uint8_t u, uint8_t n)
{
    uint16_t e = (uint16_t)(NB(units, u * UNIT_SIZE + 1) + n);

    SETB(units, u * UNIT_SIZE + 1, e > 6 ? 6 : e);
}

/* map_ptr and map2_ptr exchanged */
static void maps_swap(void)
{
    Far a = far_at(S_map_ptr, A_map_ptr), b = far_at(S_map2_ptr, A_map2_ptr);

    fww(S_map_ptr, A_map_ptr, b.off);
    fww(S_map_ptr, (uint16_t)(A_map_ptr + 2), b.seg);
    fww(S_map2_ptr, A_map2_ptr, a.off);
    fww(S_map2_ptr, (uint16_t)(A_map2_ptr + 2), a.seg);
}

/* bytes 0Fh and 10h (each side's picture) of all units exchanged */
static void pics_swap(void)
{
    uint16_t u;

    for (u = 0; u <= 0xF0; u++) {
        Far unit = unit_at((uint8_t)u);
        uint8_t b = pb(unit, 0x0F);

        pwb(unit, 0x0F, pb(unit, 0x10));
        pwb(unit, 0x10, b);
    }
}

/* the mark of the field of unit u for side a taken off, and that of its
 * other half for a two-part unit (the next unit with 80h, else the one
 * before) */
static void fight_unmark(uint8_t u, uint16_t a)
{
    int k;

    for (k = 0; k < 2; k++) {
        int16_t f = (int16_t)((int16_t)(NW(unit_pos, u * UNIT_SIZE + 2 * a) - 1) >> 1);
        int16_t w = (int16_t)NW(map_width, 0);

        SETB(field_bits, (uint16_t)((f / w) * 64 + f % w), 0);
        if (k || !(NW(units, u * UNIT_SIZE + 4) & 0x40))
            break;
        u = (uint8_t)(NW(units, u * UNIT_SIZE + 4) & 0x80 ? u + 1 : u - 1);
    }
}

/* word 1Ch of the screen's record behind DATA:0092 (pages_set reads it) */
static void screen_word_1c(uint16_t v)
{
    Far r = far_at(HL_DATA + HL_BASE, 0x0092);

    fww(r.seg, (uint16_t)(r.off + 0x1C), v);
}

/* S02E6:0122 to 0D48: side a's attacks, one a unit of its list (far
 * pointer at 11h of its `sides` record, their number in byte 15h); m is
 * the other side, map side a's.  Drawn as the original draws (the view,
 * the panels, fight_panel, the blasts); left out: what is played and
 * waited for (the port's clock, play.c, through orders_hook) and the
 * sounds. */
static void fights_run(uint16_t a, uint16_t m, Far map)
{
    Far list = far_at(S_sides, (uint16_t)(A_sides + a * SIDE_SIZE + 0x11));
    uint8_t n = 0;

    while (n < NB(sides, a * SIDE_SIZE + 0x15)) {
        uint8_t u = first_half(pb(list, n)), t, cu, ct, lu, lt, gain_u = 0, gain_t = 0;
        uint16_t aim = NW(unit_aim, pb(list, n) * UNIT_SIZE);
        uint16_t field = (uint16_t)(NW(unit_pos, u * UNIT_SIZE + 2 * a) - 1);
        uint16_t w = NW(map_width, 0), i, pictures = !NB(no_fight_pics, 0);
        int destroyed = 0;

        n++;
        t = pb(map, (uint16_t)(aim + 1));
        if (t > 0xF0)
            continue;
        t = first_half(t);
        fww(S_fight_rec, (uint16_t)(A_fight_rec + 0x2E), 0);
        field_neighbours(field);
        for (i = 0; i < 6; i++)
            if (NW(neighbours, 2 * i) == aim)
                fww(S_fight_rec, (uint16_t)(A_fight_rec + 0x2E), 1);
        fight_percent(u, t, map, a);
        rec_unit(0, u, field, map);
        cu = unit_left(u);
        rec_unit(1, t, aim, map);
        ct = unit_left(t);
        fww(S_fight_rec, (uint16_t)(A_fight_rec + 0x1A), field);
        fww(S_fight_rec, (uint16_t)(A_fight_rec + 0x1C), aim);
        fww(S_fight_rec, (uint16_t)(A_fight_rec + 0x26), (uint16_t)((int16_t)((int16_t)field >> 1) % (int16_t)w));
        fww(S_fight_rec, (uint16_t)(A_fight_rec + 0x28), (uint16_t)((int16_t)((int16_t)field >> 1) / (int16_t)w));
        fww(S_fight_rec, (uint16_t)(A_fight_rec + 0x2A), (uint16_t)((int16_t)((int16_t)aim >> 1) % (int16_t)w));
        fww(S_fight_rec, (uint16_t)(A_fight_rec + 0x2C), (uint16_t)((int16_t)((int16_t)aim >> 1) / (int16_t)w));
        rec_far(0x32, 0, 0);
        rec_far(0x3A, frw(S_GAME, A_fight_seed + 2), frw(S_GAME, A_fight_seed));
        fww(S_fight_rec, (uint16_t)(A_fight_rec + 0x38), NW(game_state, 0) & 0x80 ? 1 : 0);
        field = (uint16_t)(((int16_t)field >> 1) << 1);

        /* S02E6:04D0 to 0835: the attacker's view put on its field and
         * drawn with the maps exchanged, every field marked but the two
         * units' (both halves of a two-part one) and the units' bytes 0Fh
         * and 10h exchanged meanwhile; then the two units' panels */
        maps_swap();
        for (i = 0; i < 0x1104; i++)
            SETB(field_bits, i, m ? 0x80 : 0x40);
        fight_unmark(u, a);
        fight_unmark(t, a);
        pics_swap();
        view_to_field(m, field);
        view_draw(NW(views, m * VIEW_SIZE + 2), m);
        marks_draw(NW(views, m * VIEW_SIZE + 2), m);
        pics_swap();
        maps_swap();
        field_bits_clear(0xFF);
        unit_panel(t, m);
        unit_panel(u, a);
        page_flip();
        page_copy();
        screen_word_1c(0);
        pages_set();
        page_flip();

        /* the losses: fight_panel, and with pictures fight_show (it keeps
         * the counts before and after and draws the pictures,
         * fight_pictures), then pictures_load and libs_start again */
        fww(S_fight_rec_ptr, A_fight_rec_ptr, A_fight_rec);
        fww(S_fight_rec_ptr, (uint16_t)(A_fight_rec_ptr + 2), S_fight_rec);
        if (orders_hook)
            orders_hook(ORDERS_FIGHT, pictures);
        orders_fought = 1;     /* [BP-0Eh], S02E6:08F5 */
        if (pictures) {
            Far rec = { S_fight_rec, A_fight_rec }, p;

            fight_panel(rec, 1);
            fwb(S_fight_rec_ptr, 0x0F, NB(unit_count, u * UNIT_SIZE));
            fwb(S_fight_rec_ptr, 0x0E, NB(unit_count, t * UNIT_SIZE));
            fight_losses();
            fwb(S_fight_rec_ptr, 0x0D, NB(unit_count, u * UNIT_SIZE));
            fwb(S_fight_rec_ptr, 0x0C, NB(unit_count, t * UNIT_SIZE));
            fight_show(rec);
            p = game_mem.pics;
            pictures_load(&p, game_mem.work, game_mem.save_buf);
            page_flip();
            page_copy();
            p = game_mem.libs;
            libs_start(&p);
        } else {
            Far rec = { S_fight_rec, A_fight_rec };

            fight_panel(rec, 0);
        }
        screen_word_1c(0xFFFF);
        pages_set();
        page_flip();

        lu = unit_left(u);
        lt = unit_left(t);
        if (lu == 0) {
            unit_destroy(u, 0xFF, 0xFF, 0, a, map);
            destroyed = 1;
            gain_t++;
        }
        if (lt == 0) {
            unit_destroy(t, 0xFF, 0xFF, 2, a, map);
            destroyed = 1;
            gain_u++;
        }
        lu = (uint8_t)(cu - lu);
        if (NW(unit_flags, u * UNIT_SIZE) & 0x1000)
            carrier_losses(a, u, lu);
        lt = (uint8_t)(ct - lt);
        if (NW(unit_flags, t * UNIT_SIZE) & 0x1000)
            carrier_losses(a, t, lt);
        if (lt && !(NW(units, t * UNIT_SIZE + 6) & 0x20))
            gain_u++;
        if (lu && !(NW(units, u * UNIT_SIZE + 6) & 0x20))
            gain_t++;
        exp_add(u, gain_u);
        exp_add(t, gain_t);
        fww(S_GAME, A_fight_wait, 4);
        if (!pictures) {
            /* S02E6:0AE6: the two units' panels again, then
             * unders_restore */
            page_copy();
            unit_panel(t, m);
            unit_panel(u, a);
            page_flip();
            page_copy();
            unders_restore();
            if (orders_hook)
                orders_hook(ORDERS_WAIT, 0x1F);     /* wait_frames(1Eh) */
            if (destroyed)
                blasts_run(map, m);
        }
        if (orders_hook)
            orders_hook(ORDERS_FOUGHT, pictures);
    }
}

/* S02E6:000F (attacks_run), the round's end after both sides ended their
 * half: the attacks of side a (the one with 2 in its view's byte 16h) are
 * carried out, then the orders of the side m that moved (orders_run),
 * then each side's records and maps are made the same and the halves
 * turned round (S02E6:19B8 to 205B).  Left out: what plays music and
 * waits, and what fights_run leaves out.  Returns the result byte ([BP-7]): FFh, or the side that
 * took a headquarters. */
uint8_t attacks_run(void)
{
    uint8_t result = 0xFF;
    uint16_t a, m, u, c0 = 0, c1 = 0;
    Far map, other, v0, v1;

    orders_fought = 0;
    field_bits_clear(0xFF);
    show_message(0xFFFF, 0);
    show_message(0xFFFF, 1);
    SETW(game_state, 0, NW(game_state, 0) & 0xFFF3);
    if (NB(attack_pick, 0) == 2)
        a = 0, m = 1;
    else
        a = 1, m = 0;
    map = a ? far_at(S_map2_ptr, A_map2_ptr) : far_at(S_map_ptr, A_map_ptr);
    other = a ? far_at(S_map_ptr, A_map_ptr) : far_at(S_map2_ptr, A_map2_ptr);
    if (NB(sides, a * SIDE_SIZE + 0x15)) {
        /* S02E6:00E0: the far pointer game_loop passes first (the end of
         * the sides' lists) goes to fight_rec's 52h */
        Far ends = ptr_norm(far_at(S_sides, A_sides + 0x11));

        ends = ptr_norm((Far){ ends.seg, (uint16_t)(ends.off + 0xF2) });
        fww(S_fight_rec, (uint16_t)(A_fight_rec + 0x52), ends.off);
        fww(S_fight_rec, (uint16_t)(A_fight_rec + 0x54), ends.seg);
        SETW(game_state, 0, NW(game_state, 0) | 0x1000);
        fights_run(a, m, map);
    }
    /* S02E6:0D63: the units with 4 in unit_marks are freed, with
     * order_undo for those with 1, and so are those in the row of side a
     * of a freed carrier's record */
    for (u = 0; u <= 0xF0; u++) {
        Far unit = unit_at((uint8_t)u), rec;
        int carrier;
        uint8_t p;

        if (pw(unit, 4) & 0x8000 || !(pb(unit, 0x14) & 4))
            continue;
        carrier = pw(unit, 4) & 0x1000 ? 1 : 0;
        if (pb(unit, 0x14) & 1)
            order_undo((uint8_t)u, map, m);
        unit_free((uint8_t)u, m);
        if (!carrier)
            continue;
        rec.seg = S_carriers;
        rec.off = (uint16_t)(A_carriers + (int8_t)pb(unit, 0x0A) * SITE_SIZE);
        for (p = 0; p < 7; p++) {
            uint8_t c = pb(rec, (uint16_t)(a * 7 + p));

            if (c > 0xF0)
                continue;
            if (NB(unit_marks, c * UNIT_SIZE) & 1)
                order_undo(c, map, m);
            unit_free(c, m);
        }
    }
    show_message(0xFFFF, 0);
    show_message(0xFFFF, 1);
    orders_run(m, a, map, &result);
    /* S02E6:19B8: after a fight or a site taken the pictures are loaded
     * again (pictures_load to the battle's place for them) */
    if (NW(game_state, 0) & 0x1000) {
        Far p = game_mem.pics;

        pictures_load(&p, game_mem.work, game_mem.save_buf);
        page_flip();
        page_copy();
    }
    SETW(game_state, 0, NW(game_state, 0) & 0xEFFF);
    rows_sync(A_carriers, a, m, 0x46);
    rows_sync(A_sites_100, a, m, 0x0A);
    rows_sync(A_sites_400, a, m, 0x0A);
    rows_sync(A_sites_40, a, m, 2);
    for (u = 1; (int16_t)u < (int16_t)NW(map_bytes, 0); u += 2)
        pwb(map, u, 0xFF);
    for (u = 0; u <= 0xF0; u++) {
        Far unit = unit_at((uint8_t)u);
        uint16_t f = pw(unit, 4);

        if (f & 0x8000)
            continue;
        if (m ? f & 1 : !(f & 3)) {
            pww(unit, (uint16_t)(0x0B + 2 * a), pw(unit, (uint16_t)(0x0B + 2 * m)));
            pwb(unit, (uint16_t)(0x0F + a), pb(unit, (uint16_t)(0x0F + m)));
        }
    }
    units_drop_empty();
    for (u = 0; u <= 0xF0; u++) {
        Far unit = unit_at((uint8_t)u);

        if (!(pw(unit, 4) & 0xC000))
            pwb(map, pw(unit, 0x0B), (uint8_t)u);
    }
    map_copy(map, other);
    v0.seg = v1.seg = S_views;
    v0.off = A_views;
    v1.off = (uint16_t)(A_views + VIEW_SIZE);
    pwb(v1, 0x17, 0); pwb(v0, 0x17, 0);
    pwb(v1, 0x18, 0); pwb(v0, 0x18, 0);
    pwb(v1, 0x19, 0); pwb(v0, 0x19, 0);
    pwb(v1, 0x1B, 5); pwb(v0, 0x1B, 5);
    if (pb(v0, 0x16) == 1)
        pwb(v0, 0x16, 2), pwb(v1, 0x16, 1);
    else
        pwb(v0, 0x16, 1), pwb(v1, 0x16, 2);
    SETB(sides, SIDE_SIZE + 0x15, 0);
    SETB(sides, 0x15, 0);
    if (NB(attack_pick, 0) == 1) {
        show_message(0x35, 0);
        show_message(0x36, 1);
    } else {
        show_message(0x36, 0);
        show_message(0x35, 1);
    }
    SETW(game_state, 4, NW(game_state, 4) | 0x18);
    if (NB(attack_pick, 0) == 1)
        SETW(round_count, 0, NW(round_count, 0) + 1);
    field_bits_clear(0xFF);
    for (u = 0; u <= 0xF0; u++) {
        Far unit = unit_at((uint8_t)u);
        uint16_t k = (uint16_t)(A_unit_dat + pb(unit, 8) * KIND_SIZE);

        pww(unit, 4, pw(unit, 4) & 0xF0FF);
        if (!(pw(unit, 4) & 0x8000) && pb(unit, 0x14) & 1
            && frw(S_unit_dat, (uint16_t)(k + 0x0F)) & 1 && !(pw(unit, 4) & 0x4000)) {
            unit_mark((uint8_t)u, 0x200, pw(unit, 4) & 1 ? 1 : 0, 1);
            pww(unit, 4, pw(unit, 4) | 0x200);
        }
        if (pw(unit, 6) & 0x8000) {
            unit_mend(unit);
            if (pw(unit, 4) & 0x1000) {
                Far rec;
                uint8_t p;

                rec.seg = S_carriers;
                rec.off = (uint16_t)(A_carriers + (int8_t)pb(unit, 0x0A) * SITE_SIZE);
                for (p = 0; p < 7; p++)
                    if (pb(rec, p) <= 0xF0)
                        unit_mend(unit_at(pb(rec, p)));
            }
        }
        pwb(unit, 0x14, 0);
        pww(unit, 0x11, 0);
        pwb(unit, 0x16, 0);
        pwb(unit, 0x15, 0);
        pwb(unit, 0x13, 0);
    }
    for (u = 0; u <= 0xF0; u++) {
        Far unit = unit_at((uint8_t)u);
        uint16_t f = pw(unit, 4);

        if (f & 0x8000 || pw(unit, 6) & 0x24)
            continue;
        if (!(f & 3))
            c0++;
        else if (f & 1)
            c1++;
    }
    if (!c0 || !c1) {
        pwb(v1, 0x17, 7);
        pwb(v0, 0x17, 7);
        pww(v0, 0x22, !c0 && !c1 ? 0x15 : !c0 ? 0x12 : 0x11);
        pww(v1, 0x22, !c0 && !c1 ? 0x15 : !c0 ? 0x11 : 0x12);
    }
    {
        uint32_t sum = defence_sum();

        SETW(game_state, 0x8B, sum);
        SETW(game_state, 0x8D, sum >> 16);
    }
    return result;
}
