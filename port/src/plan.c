/* plan.c - what the computer's player of HL.EXE decides by: the phases
 * of computer_turn before the steps (port/src/computer.c), translated
 * over the program's memory.  So far computer_assess (S1CC6:0A35) and
 * what it calls.  src/HL.hints says what each state is for;
 * port/README.md how this was compared with the original.
 */
#include "hl.h"

#define VIEW_SIZE 0x31u     /* a record of `views` */
#define SIDE_SIZE 0x17u     /* a record of `sides` */
#define SITE_SIZE 0x1Cu     /* a record of `carriers` and the sites' tables */
#define PHASE_SIZE 0x0Cu    /* a record of `computer_phase` */
#define PLAN_SIZE 9u        /* a record of `unit_plans` */
#define AIM_SIZE 6u         /* an entry of a side's list of aims */

#define UNIT(u, o) (A_units + (u) * UNIT_SIZE + (o))
#define UB(u, o) frb(S_units, (uint16_t)UNIT(u, o))
#define UW(u, o) frw(S_units, (uint16_t)UNIT(u, o))
#define KIND(k, o) (A_unit_dat + (k) * KIND_SIZE + (o))
#define KB(k, o) frb(S_unit_dat, (uint16_t)KIND(k, o))
#define KW(k, o) frw(S_unit_dat, (uint16_t)KIND(k, o))

static void pww(Far p, uint16_t o, uint16_t v)
{
    fww(p.seg, (uint16_t)(p.off + o), v);
}

static Far at(uint16_t seg, uint16_t off)
{
    Far p;
    p.seg = seg;
    p.off = off;
    return p;
}

/* the side's table of the units' plans */
static Far plans_of(uint16_t side)
{
    return NB(views, side * VIEW_SIZE + 0x16) == 2 ? at(S_unit_plans2, A_unit_plans2)
                                                   : at(S_unit_plans, A_unit_plans);
}

static Far map_of(uint16_t side)
{
    return side ? far_at(S_map2_ptr, A_map2_ptr) : far_at(S_map_ptr, A_map_ptr);
}

static Far phase_of(uint16_t side)
{
    return at(S_computer_phase, (uint16_t)(A_computer_phase + side * PHASE_SIZE));
}

/* the carrier a unit's byte 0Ah names, as the program reaches it (the
 * byte is signed) */
static Far carrier_of(uint8_t unit)
{
    return at(S_carriers, (uint16_t)(A_carriers + (int8_t)UB(unit, 0x0A) * (int16_t)SITE_SIZE));
}

/* unit_worth (S1FD8:00E8): three words for a unit, from the kind's bytes
 * 0Bh, 0Ch and 0Ah, the unit's count, its unit_exp and the kind's byte 2 */
static void unit_worth(uint8_t unit, Far out)
{
    static const uint8_t from[3] = { 0x0B, 0x0C, 0x0A };
    uint8_t kind = UB(unit, 8);
    uint32_t count = (uint32_t)(UB(unit, KW(kind, 0x0F) & 4 ? 3 : 2) >> 1) + 1;
    uint32_t exp = (uint32_t)UB(unit, 1) + 1, part = (uint32_t)(KB(kind, 2) >> 3);
    int i;

    for (i = 0; i < 3; i++) {
        uint32_t v = (uint32_t)KB(kind, from[i]) * count * exp * part;

        pww(out, (uint16_t)(2 * i), (uint16_t)((int32_t)v >> 5));
    }
}

/* unit_of_kind (S1FD8:02B2): the side's first unit whose kind has a bit
 * of the mask in word 2 of kind_worth; FFh for none */
static uint8_t unit_of_kind(uint16_t mask, uint16_t side)
{
    uint16_t u;

    for (u = 0; u <= 0xF0; u++) {
        uint16_t fl = UW(u, 4);

        if (same_side(fl, side) && !(fl & 0x8000) && NW(kind_worth, UB(u, 8) * 6 + 2) & mask)
            return (uint8_t)u;
    }
    return 0xFF;
}

/* unit_reaches (S1FD8:0368): 1 when there is a way for the unit from its
 * place to the field over the fields its kind can enter (mode 1: none
 * with another side's unit; mode 2: none with an own unit but itself) */
static uint16_t unit_reaches(int16_t field, uint8_t unit, uint16_t side, uint8_t mode)
{
    Far map = map_of(side);
    uint8_t bit = side ? 2 : 1, kind = UB(unit, 8);
    int16_t w = (int16_t)NW(map_width, 0), f;
    uint16_t found;

    for (f = 0; (int16_t)NW(map_bytes, 0) > f; f = (int16_t)(f + 2)) {
        uint8_t on = pb(map, (uint16_t)(f + 1)), ok = 1;

        if (!(NB(ww1gnds_dat, pb(map, (uint16_t)f) * 6 + 2) & KB(kind, 5)))
            ok = 0;
        else if (on <= 0xF0) {
            uint16_t fl = UW(on, 4);

            if (mode & 1 && !same_side(side, fl))
                ok = 0;
            if (same_side(fl, side)) {
                if (mode & 2) {
                    if (NW(unit_pos, unit * UNIT_SIZE + side * 2) != (uint16_t)f)
                        ok = 0;
                } else if (fl & 0x1000)
                    ok = 1;
            }
        }
        if (ok) {
            int16_t h = (int16_t)(f >> 1);
            uint16_t i = (uint16_t)(h % w + ((h / w) << 6));

            SETB(field_bits, i, NB(field_bits, i) | bit);
        }
    }
    path_find(far_at(S_sides, (uint16_t)(A_sides + side * SIDE_SIZE + 0x0D)), unit,
              (int16_t)(NW(unit_pos, unit * UNIT_SIZE + side * 2) - 1), field, side, map);
    found = NW(path_len, 0) != 0;
    field_bits_clear(bit);
    return found;
}

/* sites_list (S202F:0BA7): the fields of the side's records of a table
 * of sites into the side's buffer, counted in path_len, while the side
 * has more than 0Eh points */
static void sites_list(Far table, uint16_t side, uint8_t count)
{
    Far list = far_at(S_sides, (uint16_t)(A_sides + side * SIDE_SIZE + 9));
    uint8_t i;

    for (i = 0; i < count; i++) {
        uint16_t w19 = pw(table, (uint16_t)(i * SITE_SIZE + 0x19));

        if (w19 & 0x8002 || !same_side(w19, side) || (int16_t)NW(side_points, side * 2) <= 0x0E)
            continue;
        pww(list, (uint16_t)(NW(path_len, 0) * 2), pw(table, (uint16_t)(i * SITE_SIZE + 0x0E)));
        SETW(path_len, 0, NW(path_len, 0) + 1);
    }
}

/* plan_to_site (S202F:0EAA): the unit's plan becomes kind 0Ah, to the
 * nearest record of sites_400 of the side it has a way to, else to the
 * side's headquarters */
static void plan_to_site(uint16_t side, uint8_t unit)
{
    Far plan = plans_of(side);
    int16_t w = (int16_t)NW(map_width, 0), best = 0x3E8, pick = -1, field;
    uint8_t i;

    plan.off = (uint16_t)(plan.off + unit * PLAN_SIZE);
    for (i = 0; i < 10; i++) {
        uint16_t w19 = NW(sites_400, i * SITE_SIZE + 0x19);
        int16_t at_, d;

        if (w19 & 0x8000 || !same_side(w19, side))
            continue;
        field = (int16_t)NW(sites_400, i * SITE_SIZE + 0x0E);
        if (!unit_reaches(field, unit, side, 1))
            continue;
        at_ = (int16_t)(NW(unit_pos, unit * UNIT_SIZE + side * 2) - 1);
        d = field_distance((int16_t)(field % w), (int16_t)(field / w),
                           (int16_t)(at_ % w), (int16_t)(at_ / w));
        if (d < best) {
            best = d;
            pick = i;
        }
    }
    field = (int16_t)(pick >= 0 ? NW(sites_400, pick * SITE_SIZE + 0x0E)
                                : NW(sites_40, side * SITE_SIZE + 0x0E));
    pwb(plan, 8, 0x0A);
    pww(plan, 4, 0x190);
    pww(plan, 0, (uint16_t)field);
    SETB(unit_plans2, unit * PLAN_SIZE + 8, 0);
    SETW(unit_plans2, unit * PLAN_SIZE + 4, 0xFFFF);
}

/* an entry at the end of the side's list of aims */
static void aim_add(Far phase, Far list, uint16_t worth, uint16_t what, uint8_t kind)
{
    uint16_t o = (uint16_t)(pb(phase, 6) * AIM_SIZE);

    pww(list, o, worth);
    pwb(list, (uint16_t)(o + 4), kind);
    pwb(list, (uint16_t)(o + 5), kind);
    pww(list, (uint16_t)(o + 2), what);
    pwb(phase, 6, (uint8_t)(pb(phase, 6) + 1));
}

/* the records of a table of sites that are in use and not the side's,
 * as aims of kind 1 (computer_assess's state 2) */
static void aims_sites(Far phase, Far list, uint16_t side, Far table, int income)
{
    uint8_t i, n;

    for (i = 0; i < 10; i++) {
        Far rec = at(table.seg, (uint16_t)(table.off + i * SITE_SIZE));
        uint16_t worth;

        if (pw(rec, 0x19) & 0x8000 || same_side(pw(rec, 0x19), side))
            continue;
        worth = (uint16_t)(pb(rec, 0x18) * 0x6E);
        for (n = 0; n < 7; n++)
            if (pb(rec, (uint16_t)(side * 7 + n)) <= 0xF0)
                worth = (uint16_t)(worth + 0x14);
        if (income && pb(rec, (uint16_t)(0x16 + side)) >= 0x0A)
            worth = (uint16_t)(worth + pb(rec, (uint16_t)(0x16 + side)) / 10 * 0x14);
        aim_add(phase, list, worth, pw(rec, 0x0E), 1);
    }
}

/* computer_assess (S1CC6:0A35), the phase 20h of computer_turn: one of
 * its states a call, by byte 5 of the side's computer_phase.  0 weighs
 * the two sides (word 9 of computer_phase: 1, 2 or 3) and empties the
 * plans of the side's units, 1 gives the other side's units their worth
 * as targets (word 6 of their plans), 2 to 4 make the side's list of
 * aims (sites to take, fields around the headquarters, units), 5 ends:
 * returns 1 there, else 0. */
uint16_t computer_assess(uint16_t side)
{
    Far phase = phase_of(side), plans = plans_of(side), map = map_of(side);
    Far list = far_at(S_sides, (uint16_t)(A_sides + side * SIDE_SIZE + 9));
    uint16_t other = side ? 0 : 1, u;
    int16_t w = (int16_t)NW(map_width, 0);

#define PLAN(u, o) (uint16_t)((u) * PLAN_SIZE + (o))
    switch (pb(phase, 5)) {
    case 0: {
        int32_t own = 0, theirs = 0;

        pwb(phase, 7, 0);
        if (NB(views, side * VIEW_SIZE + 0x16) == 2) {
            pwb(phase, 5, 5);
            return 0;
        }
        for (u = 0; u <= 0xF0; u++) {
            uint16_t fl = UW(u, 4);
            int32_t sum;
            uint8_t i, found;

            if (fl & 0x8002)
                continue;
            unit_worth((uint8_t)u, at(S_unit_worth_out, A_unit_worth_out));
            sum = (int32_t)(int16_t)NW(unit_worth_out, 4) + (int16_t)NW(unit_worth_out, 0)
                  + (int16_t)NW(unit_worth_out, 2);
            if (!same_side(side, fl)) {
                pww(plans, PLAN(u, 6), 0xFFFF);
                pww(plans, PLAN(u, 2), 0xFFFF);
                theirs += sum;
                continue;
            }
            own += sum;
            if (pb(plans, PLAN(u, 8)) == 0x0B)
                continue;
            if (pb(plans, PLAN(u, 8)) == 9) {
                int16_t n;

                found = 0;
                SETW(path_len, 0, 0);
                sites_list(at(S_sites_40, A_sites_40), side, 2);
                sites_list(at(S_sites_400, A_sites_400), side, 10);
                sites_list(at(S_sites_100, A_sites_100), side, 10);
                for (n = (int16_t)NW(path_len, 0); n > 0;) {
                    n--;
                    if (unit_reaches((int16_t)pw(list, (uint16_t)(n * 2)), (uint8_t)u, side, 0)) {
                        found = 1;
                        break;
                    }
                }
                if (found)
                    continue;
            }
            if (UW(u, 4) & 0x1000) {
                Far rec = carrier_of((uint8_t)u);

                found = 0;
                for (i = 0; i < 7; i++) {
                    uint8_t in = pb(rec, (uint16_t)(side * 7 + i));

                    if (in <= 0xF0 && UW(in, 6) & 4)
                        found = 1;
                }
                if (found) {
                    plan_to_site(side, (uint8_t)u);
                    continue;
                }
            }
            pww(plans, PLAN(u, 4), 0xFFFF);
            pwb(plans, PLAN(u, 8), 0);
            pww(plans, PLAN(u, 0), 0);
            SETW(unit_plans2, PLAN(u, 4), 0xFFFF);
            SETB(unit_plans2, PLAN(u, 8), 0);
            SETW(unit_plans2, PLAN(u, 0), 0);
        }
        if ((int32_t)((uint32_t)own << 1) <= theirs)
            pww(phase, 9, 3);
        else if (own < theirs)
            pww(phase, 9, 2);
        else
            pww(phase, 9, 1);
        break;
    }
    case 1: {
        int16_t hq = (int16_t)((int16_t)NW(sites_40, side * SITE_SIZE + 0x0E) >> 1);
        int16_t hq_col = (int16_t)(hq % w), hq_row = (int16_t)(hq / w);

        for (u = 0; u <= 0xF0; u++) {
            uint16_t fl = UW(u, 4);
            uint8_t kind = UB(u, 8), i;
            int16_t worth;

            if ((fl & 0x40 && !(fl & 0x80)) || !same_side(other, fl) || fl & 0x8000)
                continue;
            worth = (int8_t)NB(kind_worth, kind * 6);
            if (KW(kind, 6) & 0xC0)
                worth = (int16_t)(worth + 0x32);
            if (!(KW(kind, 0x11) & 0x20)) {
                if (!KB(kind, 0x0A))
                    worth = (int16_t)(worth + 0x19);
                if (!KB(kind, 0x0C))
                    worth = (int16_t)(worth + 0x19);
                if (!KB(kind, 0x0B))
                    worth = (int16_t)(worth + 0x19);
            }
            if (fl & 0x1000) {
                Far rec = carrier_of((uint8_t)u);

                for (i = 0; i < 7; i++) {
                    uint8_t in = pb(rec, (uint16_t)(side * 7 + i));

                    if (in <= 0xF0)
                        worth = (int16_t)(worth + (int8_t)NB(kind_worth, UB(in, 8) * 6));
                }
            }
            worth = (int16_t)(worth + (6 - UB(u, 1)) * 0x0A);
            if (NW(kind_worth, kind * 6 + 2) & 1) {
                int16_t f = (int16_t)((int16_t)(NW(unit_pos, u * UNIT_SIZE + side * 2) - 1) >> 1);
                int16_t range;

                if (fl & 0x4000)
                    range = KB(UB(pb(carrier_of((uint8_t)u), 0x1B), 8), 0);
                else
                    range = KB(kind, 0);
                if (field_distance(hq_col, hq_row, (int16_t)(f % w), (int16_t)(f / w)) < range) {
                    worth = (int16_t)(worth + 0x12C);
                    pww(phase, 9, (uint16_t)(pw(phase, 9) + 2));
                    field_neighbours((uint16_t)f);
                    for (i = 0; i < 6; i++) {
                        int16_t nb = (int16_t)NW(neighbours, i * 2);
                        uint8_t on;

                        if (nb < 0)
                            continue;
                        on = pb(map, (uint16_t)(nb + 1));
                        if (on <= 0xF0 && same_side(other, UW(on, 4)))
                            pww(plans, PLAN(on, 6), (uint16_t)(pw(plans, PLAN(on, 6)) + 0x12C));
                    }
                }
            }
            pww(plans, PLAN(u, 6), (uint16_t)(pw(plans, PLAN(u, 6)) + worth));
        }
        break;
    }
    case 2:
        if (unit_of_kind(1, side) <= 0xF0) {
            aim_add(phase, list, (uint16_t)(NB(sites_40, other * SITE_SIZE + 0x18) * 0x0A),
                    NW(sites_40, other * SITE_SIZE + 0x0E), 1);
            aims_sites(phase, list, side, at(S_sites_400, A_sites_400), 1);
            aims_sites(phase, list, side, at(S_sites_100, A_sites_100), 0);
        }
        break;
    case 3: {
        uint8_t n, i = 0;

        if (unit_of_kind(1, other) > 0xF0)
            break;
        field_neighbours(NW(sites_40, side * SITE_SIZE + 0x0E));
        if ((int16_t)pw(phase, 9) > 6)
            pww(phase, 9, 6);
        if (NB(sides, side * SIDE_SIZE + 2) < 0x0A && (int16_t)pw(phase, 9) > 1)
            pww(phase, 9, 1);
        for (n = pb(phase, 9); n > 0; n--, i++)
            aim_add(phase, list, (uint16_t)(0x78 + n * 0x1E),
                    NW(neighbours, (int8_t)NB(guard_order, i) * 2), 3);
        break;
    }
    case 4:
        if (unit_of_kind(0x280, side) > 0xF0)
            break;
        for (u = 0; u <= 0xF0; u++)
            if (!(UW(u, 4) & 0x8000) && UW(u, 6) & 4 && UW(u, 4) & 2)
                aim_add(phase, list, 0xC8, u, 4);
        break;
    default:
        pwb(phase, 5, 0);
        return 1;
    }
#undef PLAN
    pwb(phase, 5, (uint8_t)(pb(phase, 5) + 1));
    return 0;
}
