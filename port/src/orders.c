/* orders.c - what a round's end does with the orders of the side that
 * moved: HL.EXE's attacks_run from S02E6:0E9C to 19B8, the loop over the
 * units by their byte 13h, translated over the program's memory for the
 * orders 2, 3, 4 and 5.  What the original draws, plays and waits for in
 * between (the music, the views, the picture of a site taken, the
 * libraries loaded again) is left out, as in move_step.  port/README.md
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
 * port does not carry out (1, 0Bh, 0Dh and 11h; read, not translated). */
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
        case 1:
        case 0x0B:
        case 0x0D:
        case 0x11:
            left++;
            break;
        default:        /* 2, 3 and anything else: nothing more */
            break;
        }
    }
    return left;
}
