/* plan.c - what the computer's player of HL.EXE decides by: the phases
 * of computer_turn before the steps (port/src/computer.c), translated
 * over the program's memory: computer_assess (S1CC6:0A35), computer_plan
 * (S2138:000C) and computer_queue (S1CC6:0000) with what they call, and
 * computer_turn itself.  src/HL.hints says what each state is for;
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

/* ---- computer_queue */

/* rand_next (S2C9A:0008): the next number of the four words of
 * rand_words; rand_range (S0CB6:0713): one from lo to hi of it */
int16_t rand_next(void)
{
    uint16_t w0 = NW(rand_words, 0), x = (uint16_t)(w0 ^ NW(rand_words, 2));
    uint16_t w6 = NW(rand_words, 6);

    SETW(rand_words, 4, w0);
    SETW(rand_words, 6, (uint16_t)((w6 & 0x7FFF) | (x & 1) << 15));
    SETW(rand_words, 0, (uint16_t)((w6 & 0x8000) | x >> 1));
    SETW(rand_words, 2, w0);
    return (int16_t)w0;
}

static int16_t rand_range(int16_t lo, int16_t hi)
{
    int16_t v = rand_next();

    if (v < 0)
        v = (int16_t)-v;
    return (int16_t)(v % (int16_t)(hi - lo + 1) + lo);
}

/* the step computer_queue and its helpers fill before step_add (the
 * bytes a kind of step does not use stay what they were) */
static Far step_new(uint8_t kind)
{
    Far step = at(S_step_draft, A_step_draft);

    pwb(step, 0, kind);
    return step;
}

/* step_add (S1E4B:038F): the step's 7 bytes to the end of the side's
 * list, and the list's end behind it; nothing with ten steps there */
void step_add(uint16_t side, Far step)
{
    Far steps = at(S_computer_steps, (uint16_t)(A_computer_steps + side * 0x49));
    uint8_t n = pb(steps, 0x46), next;

    if (n >= 0x0A)
        return;
    if (!n) {
        pwb(steps, 0x46, 1);
        next = 1;
    } else {
        pwb(steps, 0x46, (uint8_t)(n + 1));
        next = pb(steps, 0x46);
    }
    pwb(steps, (uint16_t)(n * 7), pb(step, 0));
    pww(steps, (uint16_t)(n * 7 + 1), pw(step, 1));
    pww(steps, (uint16_t)(n * 7 + 3), pw(step, 3));
    pww(steps, (uint16_t)(n * 7 + 5), pw(step, 5));
    pwb(steps, (uint16_t)(next * 7), 0xFF);
}

/* plan_units_out (S202F:04D9): every unit in the side's rows of a table
 * of sites, without 24h in its word 6, gets the plan 5 (to leave) with
 * the worth 7, in unit_plans */
static void plan_units_out(Far table, uint16_t side, uint8_t count)
{
    uint8_t i, n;

    for (i = 0; i < count; i++) {
        uint16_t w19 = pw(table, (uint16_t)(i * SITE_SIZE + 0x19));

        if (w19 & 0x8002 || !same_side(w19, side))
            continue;
        for (n = 0; n < 7; n++) {
            uint8_t in = pb(table, (uint16_t)(i * SITE_SIZE + side * 7 + n));

            if (in > 0xF0 || UW(in, 6) & 0x24)
                continue;
            SETB(unit_plans, in * PLAN_SIZE + 8, 5);
            SETW(unit_plans, in * PLAN_SIZE + 4, 7);
        }
    }
}

/* plan_repairs (S202F:05CB): while the side has more than 0Eh points,
 * every such unit whose unit_count is below its kind's byte 3 gets the
 * plan 6 (a repair) with the worth 7, in unit_plans2 */
static void plan_repairs(Far table, uint16_t side, uint8_t count)
{
    uint8_t i, n;

    for (i = 0; i < count; i++) {
        uint16_t w19 = pw(table, (uint16_t)(i * SITE_SIZE + 0x19));

        if (w19 & 0x8002 || !same_side(w19, side) || (int16_t)NW(side_points, side * 2) <= 0x0E)
            continue;
        for (n = 0; n < 7; n++) {
            uint8_t in = pb(table, (uint16_t)(i * SITE_SIZE + side * 7 + n));

            if (in > 0xF0 || UW(in, 6) & 0x24 || UB(in, 2) >= KB(UB(in, 8), 3))
                continue;
            SETB(unit_plans2, in * PLAN_SIZE + 8, 6);
            SETW(unit_plans2, in * PLAN_SIZE + 4, 7);
        }
    }
}

/* the first half of a unit of two parts for its second */
static uint8_t unit_first(uint8_t unit)
{
    return (UW(unit, 4) & 0x40 && !(UW(unit, 4) & 0x80)) ? (uint8_t)(unit - 1) : unit;
}

static uint16_t bits_index(int16_t field)
{
    int16_t w = (int16_t)NW(map_width, 0), h = (int16_t)(field >> 1);

    return (uint16_t)(h % w + ((h / w) << 6));
}

static uint8_t dir_wrap(uint8_t d)              /* S02E6:2802 */
{
    while (d > 5)
        d = (uint8_t)(d - 6);
    return d;
}

/* plan_approach (S202F:000A), the plan 7: the unit goes next to the unit
 * its plan names (word 0), a part a call by byte 7 of computer_phase: 0
 * waits while 80h is set in game_state's word 4, 1 move_range, 2 takes
 * the first field around the target, in the order of approach_order from
 * the way the target faces, that the unit can go to and that is free (not
 * a ground with 4000h in its word 0 for a unit with 10h in unit_flags):
 * a step of kind 1 there, and with a unit of another side around that
 * field that the kind can attack (its word 6 and the unit's unit_flags
 * share one of the bits 3Ch) the plan 2 against that field in unit_plans2,
 * with the worth of that unit as a target; without such a field the plan
 * becomes 1, to the target's field.  Returns 1 from the state 2. */
static uint16_t plan_approach(uint8_t unit, uint16_t side)
{
    Far phase = phase_of(side), plan = plans_of(side), map = map_of(side);
    Far buf = far_at(S_sides, (uint16_t)(A_sides + side * SIDE_SIZE + 0x0D));
    uint8_t bit = side ? 2 : 1, i, dir, d = 0;
    int16_t place;

    unit = unit_first(unit);
    plan.off = (uint16_t)(plan.off + unit * PLAN_SIZE);
    switch (pb(phase, 7)) {
    case 0:
        if (!(NW(game_state, 4) & 0x80))
            pwb(phase, 7, 1);
        return 0;
    case 1:
        move_range((int16_t)(UW(unit, 0x0B + side * 2) - 1), buf, unit, UB(unit, 0), side,
                   UW(unit, 4) & 1 ? 6 : 3, map);
        pwb(phase, 7, 2);
        return 0;
    case 2:
        break;
    default:
        return 0;
    }
    place = (int16_t)(NW(unit_pos, pw(plan, 0) * UNIT_SIZE + side * 2) - 1);
    dir = frb(S_units, (uint16_t)(UNIT(pw(plan, 0), 0x0F) + side));
    field_neighbours((uint16_t)place);
    for (i = 0; i < 6; i++) {
        int16_t f;
        uint8_t ok;

        d = dir_wrap((uint8_t)(dir + NB(approach_order, i)));
        f = (int16_t)NW(neighbours, d * 2);
        if (f == -1 || !(NB(field_bits, bits_index(f)) & bit))
            continue;
        ok = pb(map, (uint16_t)(f + 1)) > 0xF0;
        if (UW(unit, 4) & 0x10 && NW(ww1gnds_dat, pb(map, (uint16_t)f) * 6) & 0x4000)
            ok = 0;
        if (ok)
            break;
    }
    if (i < 6) {
        Far step = step_new(1);

        pwb(step, 1, unit);
        pww(step, 3, NW(neighbours, d * 2));
        step_add(side, step);
        pwb(plan, 8, 0);
        pww(plan, 4, 0xFFFF);
        pww(plan, 2, 1);
        field_neighbours(NW(neighbours, d * 2));
        for (i = 0; i < 6; i++) {
            uint16_t f, fl;
            uint8_t on;

            d = dir_wrap((uint8_t)(dir + NB(approach_order, i)));
            f = NW(neighbours, d * 2);
            if (f == 0xFFFF)
                continue;
            on = pb(map, (uint16_t)(f + 1));
            if (on > 0xF0)
                continue;
            fl = UW(on, 4);
            if (same_side(side, fl) || !(KW(UB(unit, 8), 6) & 0x3C & fl))
                continue;
            SETB(unit_plans2, unit * PLAN_SIZE + 8, 2);
            SETW(unit_plans2, unit * PLAN_SIZE, f);
            SETW(unit_plans2, unit * PLAN_SIZE + 4, pw(plans_of(side), (uint16_t)(on * PLAN_SIZE + 6)));
        }
    } else {
        pwb(plan, 8, 1);
        pww(plan, 0, (uint16_t)place);
    }
    field_bits_clear(bit);
    pwb(phase, 7, 0);
    return 1;
}

/* plan_move (S202F:071E), the plans that take a unit to a field (word 0
 * of its plan), a part a call by byte 7 of computer_phase: 0 waits while
 * 80h is set in game_state's word 4; 1, 2 and 3 unit_reaches with the
 * modes 3, 2 and 0, the first that finds a way goes on to 4; 4 copies the
 * way (from its end) to the side's buffer of aims and its length to
 * way_len; 5 move_range (the mask FFFFh with 1 in the unit's word 6); 6
 * move_targets_list; 7 takes the first field of the way, from its end,
 * that is among those the unit can go to this round - other than the aim
 * itself not a ground with 540h in its word 0 nor a field with a unit
 * that has 1000h in unit_flags - and adds a step of kind 1 there.
 * Returns 1 from the state 7, with or without a step. */
static uint16_t plan_move(uint8_t unit, uint16_t side)
{
    Far phase = phase_of(side), plan = plans_of(side), map = map_of(side);
    Far buf = far_at(S_sides, (uint16_t)(A_sides + side * SIDE_SIZE + 0x0D));
    Far list = far_at(S_sides, (uint16_t)(A_sides + side * SIDE_SIZE + 9));
    uint8_t bit = side ? 2 : 1, state;
    int16_t n, i;

    unit = unit_first(unit);
    plan.off = (uint16_t)(plan.off + unit * PLAN_SIZE);
    state = pb(phase, 7);
    switch (state) {
    case 0:
        if (!(NW(game_state, 4) & 0x80))
            pwb(phase, 7, 1);
        return 0;
    case 1:
    case 2:
        if (unit_reaches((int16_t)pw(plan, 0), unit, side, state == 1 ? 3 : 2))
            pwb(phase, 7, 4);
        else
            pwb(phase, 7, (uint8_t)(state + 1));
        return 0;
    case 3:
        unit_reaches((int16_t)pw(plan, 0), unit, side, 0);
        break;
    case 4:
        SETW(way_len, 0, NW(path_len, 0));
        for (n = 0; (int16_t)NW(path_len, 0) > n; n++)
            pww(list, (uint16_t)(n * 2), pw(buf, (uint16_t)(0x1F40 + n * 2)));
        break;
    case 5:
        move_range((int16_t)(UW(unit, 0x0B + side * 2) - 1), buf, unit, UB(unit, 0), side,
                   UW(unit, 6) & 1 ? 0xFFFF : UW(unit, 4) & 1 ? 6 : 3, map);
        break;
    case 6:
        move_targets_list(buf, side, unit);
        field_bits_clear(bit);
        break;
    case 7:
        pwb(phase, 7, 0);
        for (n = 0; n < (int16_t)NW(way_len, 0); n++)
            for (i = 0; (int16_t)NW(path_len, 0) > i; i++) {
                uint16_t f = pw(buf, (uint16_t)(i * 2));
                uint8_t ok = 1;

                if (f != pw(list, (uint16_t)(n * 2)))
                    continue;
                if (pw(plan, 0) != f) {
                    uint8_t on = pb(map, (uint16_t)(f + 1));

                    if (NW(ww1gnds_dat, pb(map, f) * 6) & 0x540)
                        ok = 0;
                    if (on <= 0xF0 && UW(on, 4) & 0x1000)
                        ok = 0;
                }
                if (ok) {
                    Far step = step_new(1);

                    pwb(step, 1, unit);
                    pww(step, 3, f);
                    step_add(side, step);
                    pwb(phase, 7, 0);
                    return 1;
                }
            }
        return 1;
    default:
        return 0;
    }
    pwb(phase, 7, (uint8_t)(state + 1));
    return 0;
}

/* a plan carried out or given up: no kind, no worth, done */
static void plan_done(Far plan, int worth)
{
    pwb(plan, 8, 0);
    if (worth)
        pww(plan, 4, 0xFFFF);
    pww(plan, 2, 1);
}

/* the plans 2 and 0Eh: a step of kind 2 against the unit on the plan's
 * field, when there is one */
static uint16_t queue_attack(uint16_t side, uint16_t u, Far plan)
{
    uint8_t on = pb(map_of(side), (uint16_t)(pw(plan, 0) + 1));
    Far step;

    plan_done(plan, 1);
    if (UW(on, 4) & 0x8000 || on > 0xF0)
        return 0;
    step = step_new(2);
    pwb(step, 1, (uint8_t)u);
    pwb(step, 3, on);
    step_add(side, step);
    return 1;
}

/* computer_queue (S1CC6:0000), the phase 8 of computer_turn: makes the
 * units' plans into steps, by byte 3 of the side's computer_phase.
 * Returns 1 when steps were added that are to be carried out before it
 * goes on, else 0:
 * 0  the plans of the side's units become open (word 2 FFFFh);
 * 1  the units in the side's sites get plans: to leave with 1 in the
 *    side's attack_pick, else a repair where one is needed;
 * 2  the next unit of the side with a plan, from word 9 on (none: 4);
 * 3  its plan when that is 3 (a step of kind 1 to the plan's field), 4
 *    (given up), 0Ch (plan_move) or 0Eh (an attack); back to 2, with the
 *    next unit when it is another plan;
 * 4  the open plan with the highest worth of the side's units without
 *    8200h in unit_flags (none: 6);
 * 5  that plan: 1 and 0Dh plan_move, 2 an attack, 5 a step of kind 4, 6
 *    one of kind 6, 7 plan_approach, 9 to 0Bh plan_move with the plan
 *    kept, any other given up; back to 4;
 * 6  not with 1 in attack_pick: for every free place of the side's
 *    records of sites_400, while the side has more than 3Dh points and
 *    fewer than ten were chosen, build_list with what is above 3Dh and a
 *    kind drawn from build_kinds that has 400h in word 2 of kind_worth
 *    (a hundred tries): a step of kind 7, its price taken off the count;
 * 7  a step of kind 3 (the round's end);
 * 8  the phase becomes 20h (computer_assess) again. */
uint16_t computer_queue(uint16_t side)
{
    Far phase = phase_of(side), plans = plans_of(side), plan, step;
    uint16_t u;
    uint8_t one = NB(views, side * VIEW_SIZE + 0x16) == 1;

#define SITES(f, t, n) f(at(S_##t, A_##t), side, n)
    switch (pb(phase, 3)) {
    case 0:
        for (u = 0; u <= 0xF0; u++)
            if (!(UW(u, 4) & 0x8000) && same_side(UW(u, 4), side))
                pww(plans, (uint16_t)(u * PLAN_SIZE + 2), 0xFFFF);
        pwb(phase, 7, 0);
        pww(phase, 9, 0);
        break;
    case 1:
        if (one) {
            SITES(plan_units_out, sites_100, 10);
            SITES(plan_units_out, sites_400, 10);
            SITES(plan_units_out, sites_40, 2);
        } else {
            SITES(plan_repairs, sites_100, 10);
            SITES(plan_repairs, sites_400, 10);
            SITES(plan_repairs, sites_40, 2);
        }
        break;
    case 2:
        for (u = pb(phase, 9); u <= 0xF0; u++)
            if (!(UW(u, 4) & 0x8000) && same_side(UW(u, 4), side)
                && pb(plans, (uint16_t)(u * PLAN_SIZE + 8))) {
                pww(phase, 9, u);
                break;
            }
        if (u > 0xF0) {
            pwb(phase, 3, 4);
            return 0;
        }
        break;
    case 3:
        u = pw(phase, 9);
        plan = at(plans.seg, (uint16_t)(plans.off + u * PLAN_SIZE));
        switch (pb(plan, 8)) {
        case 3:
            step = step_new(1);
            pwb(step, 1, (uint8_t)u);
            pww(step, 3, pw(plan, 0));
            step_add(side, step);
            plan_done(plan, 0);
            pwb(phase, 3, 2);
            return 1;
        case 4:
            plan_done(plan, 0);
            pwb(phase, 3, 2);
            return 0;
        case 0x0C:
            if (!plan_move((uint8_t)u, side))
                return 0;
            plan_done(plan, 1);
            pwb(phase, 3, 2);
            return 1;
        case 0x0E:
            pwb(phase, 3, 2);
            return queue_attack(side, u, plan);
        default:
            pwb(phase, 3, 2);
            pww(phase, 9, (uint16_t)(pw(phase, 9) + 1));
            return 0;
        }
    case 4: {
        int16_t best = -1;

        pww(phase, 9, 0xFFFF);
        for (u = 0; u <= 0xF0; u++) {
            plan = at(plans.seg, (uint16_t)(plans.off + u * PLAN_SIZE));
            if ((int16_t)pw(plan, 4) > best && !(UW(u, 4) & 0x8200) && same_side(UW(u, 4), side)
                && pb(plan, 8) && (int16_t)pw(plan, 2) < 0) {
                best = (int16_t)pw(plan, 4);
                pww(phase, 9, u);
            }
        }
        if ((int16_t)pw(phase, 9) < 0) {
            pwb(phase, 3, 6);
            return 0;
        }
        break;
    }
    case 5:
        u = pw(phase, 9);
        plan = at(plans.seg, (uint16_t)(plans.off + u * PLAN_SIZE));
        switch (pb(plan, 8)) {
        case 1:
        case 0x0D:
            if (!plan_move((uint8_t)u, side))
                return 0;
            plan_done(plan, 1);
            pwb(phase, 3, 4);
            return 1;
        case 2:
            pwb(phase, 3, 4);
            return queue_attack(side, u, plan);
        case 5:
        case 6:
            step = step_new(pb(plan, 8) == 5 ? 4 : 6);
            pwb(step, 1, (uint8_t)u);
            step_add(side, step);
            plan_done(plan, 1);
            pwb(phase, 3, 4);
            return 1;
        case 7:
            if (!plan_approach((uint8_t)u, side))
                return 0;
            pwb(phase, 3, 4);
            return 1;
        case 9:
        case 0x0A:
        case 0x0B:
            if (!plan_move((uint8_t)u, side))
                return 0;
            pww(plan, 2, 1);
            pwb(phase, 3, 4);
            return 1;
        default:
            plan_done(plan, 1);
            pwb(phase, 3, 4);
            return 0;
        }
    case 6: {
        uint8_t chosen = 0, i, n, t;

        if (one)
            break;
        for (i = 0; i < 10; i++) {
            Far rec = at(S_sites_400, (uint16_t)(A_sites_400 + i * SITE_SIZE));
            int16_t points = (int16_t)NW(side_points, side * 2);

            if (pw(rec, 0x19) & 0x8002 || !same_side(pw(rec, 0x19), side) || points <= 0x3D)
                continue;
            for (n = 0; n < 7; n++) {
                uint8_t kind = 0xFF;

                if (pb(rec, (uint16_t)(side * 7 + n)) <= 0xF0)
                    continue;
                if ((int16_t)(points - 0x3D) <= 0)
                    break;
                build_list((uint16_t)(points - 0x3D), side);
                if (!NB(build_count, 0) || chosen >= 0x0A)
                    break;
                for (t = 0; t < 0x64; t++) {
                    kind = NB(build_kinds, rand_range(0, (int16_t)(NB(build_count, 0) - 1)));
                    if (NW(kind_worth, kind * 6 + 2) & 0x400)
                        break;
                    kind = 0xFF;
                }
                if (kind == 0xFF)
                    continue;
                points = (int16_t)(points - KB(kind, 0x3F));
                chosen++;
                step = step_new(7);
                pwb(step, 1, kind);
                pwb(step, 3, i);
                step_add(side, step);
            }
        }
        break;
    }
    case 7:
        step_add(side, step_new(3));
        pwb(phase, 3, 8);
        return 1;
    default:
        pww(phase, 0, (uint16_t)((pw(phase, 0) & 0xFFF7) | 0x20));
        pwb(phase, 3, 0);
        return 0;
    }
#undef SITES
    pwb(phase, 3, (uint8_t)(pb(phase, 3) + 1));
    return 0;
}

/* ---- computer_plan */

#define KWORTH(u) NW(kind_worth, UB(u, 8) * 6 + 2)
#define HALF2(fl) ((fl) & 0x40 && !((fl) & 0x80))   /* the second half of a unit of two */
#define POS(u) ((int16_t)(NW(unit_pos, (u) * UNIT_SIZE + side * 2) - 1))
#define P_W(u, o) pw(plans, (uint16_t)((u) * PLAN_SIZE + (o)))
#define P_B(u, o) pb(plans, (uint16_t)((u) * PLAN_SIZE + (o)))
#define SP_W(u, o, v) pww(plans, (uint16_t)((u) * PLAN_SIZE + (o)), (uint16_t)(v))
#define SP_B(u, o, v) pwb(plans, (uint16_t)((u) * PLAN_SIZE + (o)), (uint8_t)(v))

/* the other half's plan of a unit emptied */
static void plan2_clear(uint16_t u)
{
    SETB(unit_plans2, u * PLAN_SIZE + 8, 0);
    SETW(unit_plans2, u * PLAN_SIZE + 4, 0xFFFF);
}

/* field_distance of two map offsets */
static int16_t fields_apart(int16_t a, int16_t b)
{
    int16_t w = (int16_t)NW(map_width, 0);

    a >>= 1;
    b >>= 1;
    return field_distance((int16_t)(a % w), (int16_t)(a / w), (int16_t)(b % w), (int16_t)(b / w));
}

/* unit_outmatched (S1FD8:0002): FFFFh when the unit's count is more than
 * two below the other's and its unit_exp below 3, else 0 */
static int16_t unit_outmatched(uint8_t unit, uint8_t other)
{
    int16_t a = UB(unit, KW(UB(unit, 8), 0x0F) & 4 ? 3 : 2);
    int16_t b = UB(other, KW(UB(other, 8), 0x0F) & 4 ? 3 : 2);

    return (int16_t)(a - b) < -2 && UB(unit, 1) < 3 ? -1 : 0;
}

/* field_pinched (S202F:0C79): 1 for a field with more than three units of
 * the other side around it, or with two of them on opposite sides;
 * `neighbours` is left as it was */
static uint16_t field_pinched(uint16_t field, uint16_t side)
{
    Far map = map_of(side);
    uint16_t other = side ? 0 : 1, keep[6], pinched = 0;
    int16_t n = 0, i;

    for (i = 0; i < 6; i++)
        keep[i] = NW(neighbours, i * 2);
    field_neighbours(field);
    for (i = 0; i < 6; i++) {
        uint16_t f = NW(neighbours, i * 2);
        uint8_t on;

        SETW(neighbours, i * 2, 0);
        if (f == 0xFFFF)
            continue;
        on = pb(map, (uint16_t)(f + 1));
        if (on <= 0xF0 && same_side(other, UW(on, 4))) {
            SETW(neighbours, i * 2, 1);
            n++;
        }
    }
    if (n > 3)
        pinched = 1;
    else
        for (i = 0; i < 6; i++)
            if (NW(neighbours, i * 2) && NW(neighbours, dir_wrap((uint8_t)(i + 3)) * 2))
                pinched = 1;
    for (i = 0; i < 6; i++)
        SETW(neighbours, i * 2, keep[i]);
    return pinched;
}

/* unit_chosen (S202F:0E3C): is the unit one of the six chosen for the
 * fields around the target (bytes 28h to 2Dh of the side's work buffer) */
static uint8_t unit_chosen(uint8_t unit, uint16_t side)
{
    Far buf = far_at(S_sides, (uint16_t)(A_sides + side * SIDE_SIZE + 0x0D));
    uint8_t i;

    for (i = 0; i < 6; i++)
        if (pb(buf, (uint16_t)(0x28 + i)) == unit)
            return 1;
    return 0;
}

/* plan_ride (S2407:0007): a carrier for the unit, a part a call by byte 8
 * of computer_phase; returns 1 when it is through, with or without one.
 * 0: a unit with 4 in its word 6 goes on at 5; 1 move_range, 2
 * move_targets_list; 3 a field the unit can go to with a unit whose plan
 * is `kind` already: the unit gets the plan 0Ch there and that unit's
 * worth goes up by `worth`; 4 one with a unit without a plan: the same,
 * and that unit gets the plan `kind` to `field`; 5 the next carrier of the
 * side (1000h in unit_flags, 380h in word 2 of its kind_worth) from
 * ride_carrier on whose plan is worth less, is not 9, 0Ah, 0Bh or 0Dh,
 * and whose record of carriers shares one of the bits FC0h in word 19h
 * with the unit's word 6; 6 with a way for it to the unit it gets the
 * plan 0Dh there, else 5 again with the next. */
static uint16_t plan_ride(uint16_t side, uint8_t unit, int16_t worth, uint8_t kind, uint16_t field)
{
    Far phase = phase_of(side), plans = plans_of(side), map = map_of(side);
    Far buf = far_at(S_sides, (uint16_t)(A_sides + side * SIDE_SIZE + 0x0D));
    uint16_t u;
    int16_t i;

    switch (pb(phase, 8)) {
    case 0:
        SETW(ride_carrier, 0, 0);
        pwb(phase, 8, UW(unit, 6) & 4 ? 5 : 1);
        return 0;
    case 1:
        move_range(POS(unit), buf, unit, UB(unit, 0), side, 0, map);
        break;
    case 2:
        move_targets_list(buf, side, unit);
        field_bits_clear(side ? 2 : 1);
        break;
    case 3:
    case 4:
        for (i = 0; (int16_t)NW(path_len, 0) > i; i++) {
            uint16_t f = pw(buf, (uint16_t)(i * 2));
            uint8_t on = pb(map, (uint16_t)(f + 1));

            if (on > 0xF0 || P_B(on, 8) != (pb(phase, 8) == 3 ? kind : 0))
                continue;
            SP_W(unit, 4, worth);
            SP_W(unit, 0, f);
            SP_B(unit, 8, 0x0C);
            plan2_clear(unit);
            if (pb(phase, 8) == 3)
                SP_W(on, 4, P_W(on, 4) + worth);
            else {
                SP_W(on, 4, worth);
                SP_W(on, 0, field);
                SP_B(on, 8, kind);
                plan2_clear(on);
            }
            pwb(phase, 8, 0);
            return 1;
        }
        break;
    case 5: {
        int16_t best = 0x3E8;

        for (u = (uint8_t)NW(ride_carrier, 0); u <= 0xF0; u++) {
            uint16_t fl = UW(u, 4);
            uint8_t k;
            int16_t d;

            if (fl & 0xC000 || !(fl & 0x1000) || (int16_t)P_W(u, 4) >= worth
                || !same_side(side, fl))
                continue;
            k = P_B(u, 8);
            if (!(KWORTH(u) & 0x380) || k == 9 || k == 0x0A || k == 0x0D || k == 0x0B)
                continue;
            if (!(UW(unit, 6) & pw(carrier_of((uint8_t)u), 0x19) & 0xFC0))
                continue;
            d = fields_apart(POS(unit), POS(u));
            if (d < best) {
                best = d;
                SETW(ride_carrier, 0, u);
                break;
            }
        }
        pwb(phase, 8, u <= 0xF0 ? 6 : 7);
        return 0;
    }
    case 6: {
        int16_t at_ = POS(unit);

        u = NW(ride_carrier, 0);
        if (unit_reaches(at_, (uint8_t)u, side, 0)) {
            SP_W(u, 4, worth);
            SP_W(u, 0, at_);
            SP_B(u, 8, 0x0D);
            plan2_clear(u);
            pwb(phase, 8, 7);
        } else {
            pwb(phase, 8, 5);
            SETW(ride_carrier, 0, u + 1);
        }
        return 0;
    }
    default:
        pwb(phase, 8, 0);
        return 1;
    }
    pwb(phase, 8, (uint8_t)(pb(phase, 8) + 1));
    return 0;
}

/* computer_plan (S2138:000C), the phase 40h of computer_turn: gives the
 * side's units their plans, a part a call, by the bytes 4 (the state)
 * and 7 (the step inside it) of the side's computer_phase.  Returns 1
 * from the state 0Ch, else 0.  src/HL.hints says what each state does. */
uint16_t computer_plan(uint16_t side)
{
    Far phase = phase_of(side), plans = plans_of(side), map = map_of(side);
    Far aims = far_at(S_sides, (uint16_t)(A_sides + side * SIDE_SIZE + 9));
    Far buf = far_at(S_sides, (uint16_t)(A_sides + side * SIDE_SIZE + 0x0D));
    uint16_t other = side ? 0 : 1, u;
    uint8_t step = pb(phase, 7), i;
    int16_t best, pick;

#define A_W(n, o) pw(aims, (uint16_t)((n) * AIM_SIZE + (o)))
#define A_B(n, o) pb(aims, (uint16_t)((n) * AIM_SIZE + (o)))
#define AIM NW(plan_aim, 0)
#define AIM_WORTH ((int16_t)A_W(AIM, 0))
#define CUR NB(plan_unit, 0)
#define CUR_NEXT SETB(plan_unit, 0, NB(plan_unit, 0) + 1)
#define TARGET NB(plan_target, 0)
#define STATE(v) pwb(phase, 4, (uint8_t)(v))
#define STEP(v) pwb(phase, 7, (uint8_t)(v))
#define AIM_DONE (STEP(0), STATE(1), pwb(aims, (uint16_t)(AIM * AIM_SIZE + 5), 0))
/* the plan 1 to a field for the unit at hand, worth 0Ah */
#define FOLLOW(field) (SP_W(CUR, 4, 0x0A), SP_W(CUR, 0, field), SP_B(CUR, 8, 1), \
                       plan2_clear(CUR), CUR_NEXT, STEP(1))
    switch (pb(phase, 4)) {
    case 0:
        if (NB(views, side * VIEW_SIZE + 0x16) == 2)
            STATE(0x0C);
        else {
            STATE(1);
            SETW(plan_aim_kind, 0, 1);
        }
        return 0;
    case 1:                     /* the aim worth most of the kind at hand */
        if (!pb(phase, 6) || (int16_t)NW(plan_aim_kind, 0) >= 6) {
            STATE(7);
            return 0;
        }
        best = 0;
        SETW(plan_aim, 0, 0xFFFF);
        for (pick = 0; (int16_t)pb(phase, 6) > pick; pick++)
            if ((int16_t)A_W(pick, 0) > best && A_B(pick, 5) == NW(plan_aim_kind, 0)) {
                best = (int16_t)A_W(pick, 0);
                SETW(plan_aim, 0, pick);
            }
        if ((int16_t)AIM >= 0) {
            uint8_t k = A_B(AIM, 5);

            STATE(k >= 1 && k <= 4 ? k + 2 : 2);
        } else
            SETW(plan_aim_kind, 0, NW(plan_aim_kind, 0) + 1);
        return 0;
    case 2:
        STATE(1);
        pwb(aims, (uint16_t)(AIM * AIM_SIZE + 5), 0);
        return 0;
    case 3:                     /* an aim of kind 1: a site to take */
        switch (step) {
        case 0:
            SETB(plan_unit, 0, 0);
            STEP(1);
            return 0;
        case 1:
            for (; CUR <= 0xF0; CUR_NEXT)
                if (KWORTH(CUR) & 1 && !(UW(CUR, 4) & 0xC040) && same_side(UW(CUR, 4), side)
                    && (int16_t)P_W(CUR, 4) < AIM_WORTH)
                    break;
            STEP(CUR <= 0xF0 ? 2 : 3);
            return 0;
        case 2:
            SP_W(CUR, 2, unit_reaches((int16_t)A_W(AIM, 2), CUR, side, 0) ? NW(path_len, 0) : 0x3E8);
            STEP(1);
            CUR_NEXT;
            return 0;
        case 3:
            best = 0x3E8;
            pick = -1;
            for (u = 0; u <= 0xF0; u++)
                if (KWORTH(u) & 1 && !(UW(u, 4) & 0xC040) && same_side(UW(u, 4), side)
                    && (int16_t)P_W(u, 4) < AIM_WORTH && (int16_t)P_W(u, 2) < best
                    && (int16_t)P_W(u, 4) < (int16_t)(A_W(AIM, 0) - P_W(u, 2))) {
                    best = (int16_t)P_W(u, 2);
                    pick = (int16_t)u;
                }
            if (pick >= 0) {
                uint16_t worth = (uint16_t)(A_W(AIM, 0) - P_W(pick, 2));

                SP_W(pick, 4, worth);
                SP_W(pick, 0, A_W(AIM, 2));
                SP_B(pick, 8, 1);
                plan2_clear((uint16_t)pick);
                if (KWORTH(pick) & 0x20)
                    aim_add(phase, aims, (uint16_t)(worth - 10), (uint16_t)pick, 2);
            }
            STEP(4);
            return 0;
        default:
            AIM_DONE;
            return 0;
        }
    case 4:                     /* an aim of kind 2: a unit to go with */
        switch (step) {
        case 0:
            SETB(plan_unit, 0, 0);
            STEP(1);
            return 0;
        case 1:
            for (; CUR <= 0xF0; CUR_NEXT) {
                uint16_t fl = UW(CUR, 4);

                if (HALF2(fl))
                    continue;
                if (!(fl & 0xC000) && same_side(fl, side) && (int16_t)P_W(CUR, 4) < AIM_WORTH
                    && KWORTH(CUR) & UW(A_W(AIM, 2), 4) & 0x1C)
                    break;
            }
            STEP(CUR <= 0xF0 ? 2 : 3);
            return 0;
        case 2:
            SP_W(CUR, 2, unit_reaches(POS(A_W(AIM, 2)), CUR, side, 0) ? NW(path_len, 0) : 0x3E8);
            STEP(1);
            CUR_NEXT;
            return 0;
        case 3: {
            uint8_t whom = pb(aims, (uint16_t)(AIM * AIM_SIZE + 2));
            uint8_t n = NB(kind_worth, UB(whom, 8) * 6 + 4);

            for (i = 0; i < n; i++) {
                best = 0x3E8;
                pick = -1;
                for (u = 0; u <= 0xF0; u++) {
                    uint16_t fl = UW(u, 4);

                    if (HALF2(fl) || fl & 0xC000 || !same_side(fl, side)
                        || (int16_t)P_W(u, 4) >= AIM_WORTH
                        || !(KWORTH(u) & UW(whom, 4) & 0x1C) || (int16_t)P_W(u, 2) >= best)
                        continue;
                    best = (int16_t)P_W(u, 2);
                    pick = (int16_t)u;
                }
                if (pick < 0)
                    continue;
                best = (int16_t)(AIM_WORTH / 3);
                SP_W(pick, 4, A_W(AIM, 0));
                pww(aims, (uint16_t)(AIM * AIM_SIZE), (uint16_t)(A_W(AIM, 0) - best));
                SP_W(pick, 0, whom);
                SP_B(pick, 8, 7);
                plan2_clear((uint16_t)pick);
            }
            STEP(4);
            return 0;
        }
        default:
            AIM_DONE;
            return 0;
        }
    case 5:                     /* an aim of kind 3: a field to hold */
        switch (step) {
        case 0:
            SETB(plan_unit, 0, 0);
            STEP(1);
            return 0;
        case 1: {
            uint16_t f = A_W(AIM, 2);
            uint8_t on = pb(map, (uint16_t)(f + 1));

            if (on <= 0xF0 && same_side(side, UW(on, 4)) && (int16_t)P_W(on, 4) < AIM_WORTH
                && KWORTH(on) & 0x40) {
                SP_W(on, 4, A_W(AIM, 0));
                SP_W(on, 0, f);
                SP_B(on, 8, 4);
                plan2_clear(on);
                STEP(5);
            } else
                STEP(2);
            return 0;
        }
        case 2:
            for (; CUR <= 0xF0; CUR_NEXT) {
                uint16_t fl = UW(CUR, 4);

                if (HALF2(fl))
                    continue;
                if (!(fl & 0xC000) && same_side(fl, side) && (int16_t)P_W(CUR, 4) < AIM_WORTH
                    && KWORTH(CUR) & 0x40)
                    break;
            }
            STEP(CUR <= 0xF0 ? 3 : 4);
            return 0;
        case 3:
            SP_W(CUR, 2, unit_reaches((int16_t)A_W(AIM, 2), CUR, side, 0) ? NW(path_len, 0) : 0x3E8);
            STEP(2);
            CUR_NEXT;
            return 0;
        case 4:
            best = 0x3E8;
            pick = -1;
            for (u = 0; u <= 0xF0; u++) {
                uint16_t fl = UW(u, 4);

                if (HALF2(fl) || fl & 0xC000 || !same_side(fl, side)
                    || (int16_t)P_W(u, 4) >= AIM_WORTH || !(KWORTH(u) & 0x40)
                    || (int16_t)P_W(u, 2) >= best)
                    continue;
                best = (int16_t)P_W(u, 2);
                pick = (int16_t)u;
            }
            if (pick >= 0) {
                SP_W(pick, 4, A_W(AIM, 0));
                SP_W(pick, 0, A_W(AIM, 2));
                SP_B(pick, 8, 1);
                plan2_clear((uint16_t)pick);
            }
            STEP(5);
            return 0;
        default:
            AIM_DONE;
            return 0;
        }
    case 6: {                   /* an aim of kind 4: a unit to fetch */
        uint16_t whom = A_W(AIM, 2);

        if (plan_ride(side, (uint8_t)whom, AIM_WORTH, 0x0A,
                      NW(unit_pos, whom * UNIT_SIZE + side * 2))) {
            STATE(1);
            STEP(0);
            pwb(aims, (uint16_t)(AIM * AIM_SIZE + 5), 0);
        }
        return 0;
    }
    case 7:                     /* the units that are not whole: to a site */
        switch (step) {
        case 0:
            SETB(plan_unit, 0, 0);
            STEP(1);
            return 0;
        case 1:
            for (; CUR <= 0xF0; CUR_NEXT)
                if (!(UW(CUR, 4) & 0xC048) && same_side(UW(CUR, 4), side)
                    && UB(CUR, 2) < KB(UB(CUR, 8), 3))
                    break;
            STEP(CUR <= 0xF0 ? 2 : 5);
            return 0;
        case 2:
            SETW(path_len, 0, 0);
            sites_list(at(S_sites_40, A_sites_40), side, 2);
            sites_list(at(S_sites_400, A_sites_400), side, 10);
            sites_list(at(S_sites_100, A_sites_100), side, 10);
            if (!NW(path_len, 0)) {
                STEP(1);
                CUR_NEXT;
            } else {
                SETW(plan_sites_left, 0, NW(path_len, 0));
                STEP(3);
            }
            return 0;
        case 3: {
            int16_t worth, f;
            int more;

            if ((int16_t)NW(plan_sites_left, 0) <= 0)
                return 0;
            SETW(plan_sites_left, 0, NW(plan_sites_left, 0) - 1);
            f = (int16_t)pw(aims, (uint16_t)(NW(plan_sites_left, 0) * 2));
            worth = UB(CUR, 2) < 5 && UB(CUR, 1) > 1 ? (int16_t)(0x46 + UB(CUR, 1) * 0x0A) : 0x0F;
            more = 1;           /* the site is further than a round's way */
            if (unit_reaches(f, CUR, side, 1)
                && (int16_t)NW(path_len, 0) <= (int16_t)KB(UB(CUR, 8), 0)) {
                more = 0;
                worth = (int16_t)(worth << 1);
            }
            if ((int16_t)P_W(CUR, 4) >= worth) {
                STEP(1);
                CUR_NEXT;
            } else if (more) {
                SETW(plan_worth, 0, worth);
                SETW(plan_field, 0, f);
                STEP(4);
            } else {
                SP_W(CUR, 4, worth);
                SP_W(CUR, 0, f);
                SP_B(CUR, 8, 1);
                plan2_clear(CUR);
                STEP(1);
                CUR_NEXT;
            }
            return 0;
        }
        case 4:
            if (KWORTH(CUR) & 0x380
                || plan_ride(side, CUR, (int16_t)NW(plan_worth, 0), 9, NW(plan_field, 0))) {
                STEP(1);
                CUR_NEXT;
            }
            return 0;
        default:
            STEP(0);
            STATE(8);
            return 0;
        }
    case 8:                     /* what can attack each unit of the other side */
        for (u = 0; u <= 0xF0; u++) {
            uint16_t fl = UW(u, 4), by = 0;
            uint8_t kind = UB(u, 8);

            if (HALF2(fl) || !same_side(other, fl) || fl & 0xC000)
                continue;
            if (KW(kind, 6) & 0xC0)
                by |= 1;
            if (!KB(kind, 0x0A))
                by |= 2;
            if (!KB(kind, 0x0C))
                by |= 4;
            if (!KB(kind, 0x0B))
                by |= 8;
            SP_W(u, 2, by);
        }
        STATE(9);
        return 0;
    case 9:                     /* the units that fire from afar */
        switch (step) {
        case 0:
            SETB(plan_unit, 0, 0);
            STEP(1);
            return 0;
        case 1:
            for (u = CUR; u <= 0xF0; u++) {
                uint16_t fl = UW(u, 4);

                if (!HALF2(fl) && same_side(side, fl) && !(fl & 0xC200) && KWORTH(u) & 2) {
                    SETB(plan_unit, 0, u);
                    break;
                }
            }
            STEP(u <= 0xF0 ? 2 : 4);
            return 0;
        case 2: {
            uint8_t kind = UB(CUR, 8), bit = side ? 8 : 4;
            int16_t f = POS(CUR);

            field_bits_clear(bit);
            if (KB(kind, 9) > 1)
                attack_range(buf, f, KB(kind, 9), side, CUR, KW(kind, 6) & 0xFFEF, map);
            if (KB(kind, 8) > 1)
                attack_range(buf, f, KB(kind, 8), side, CUR, KW(kind, 6) & 0xFFD3, map);
            STEP(3);
            return 0;
        }
        case 3: {
            uint8_t bit = side ? 8 : 4;
            int16_t col, row, at_ = -1;

            best = -1;
            for (col = 0; (int16_t)NW(map_width, 0) > col; col++)
                for (row = 0; (int16_t)NW(map_height, 0) > row; row++) {
                    uint16_t f;
                    uint8_t on;

                    if (!(NB(field_bits, (uint16_t)(col + (row << 6))) & bit))
                        continue;
                    f = (uint16_t)((NW(map_width, 0) * row + col) << 1);
                    on = pb(map, (uint16_t)(f + 1));
                    if ((int16_t)P_W(on, 6) > best) {
                        at_ = (int16_t)f;
                        best = (int16_t)P_W(on, 6);
                    }
                }
            if (at_ >= 0) {
                SP_B(CUR, 8, 4);
                SP_W(CUR, 0, POS(CUR));
                SP_W(CUR, 4, 0x64);
                SETB(unit_plans2, CUR * PLAN_SIZE + 8, 0x0E);
                SETW(unit_plans2, CUR * PLAN_SIZE, at_);
                SETW(unit_plans2, CUR * PLAN_SIZE + 4, 0x64);
            }
            field_bits_clear(bit);
            CUR_NEXT;
            STEP(1);
            return 0;
        }
        default:
            STEP(0);
            STATE(0x0A);
            return 0;
        }
    case 0x0A: {                /* the targets, each with units around it */
        Far fields = at(buf.seg, (uint16_t)(buf.off + 0x14));
        Far chosen = at(buf.seg, (uint16_t)(buf.off + 0x28));

/* the units that could go to the field in direction d: aims + d * 14h,
 * their number at aims + 78h + d; the field's state, word d of buf: 0
 * open, 1 taken, FFFFh not to be used */
#define CAND(d, n) pb(aims, (uint16_t)((d) * 0x14 + (n)))
#define CANDS(d) pb(aims, (uint16_t)(0x78 + (d)))
#define SET_CANDS(d, v) pwb(aims, (uint16_t)(0x78 + (d)), (uint8_t)(v))
#define FIELD(d) pw(buf, (uint16_t)((d) * 2))
#define SET_FIELD(d, v) pww(buf, (uint16_t)((d) * 2), (uint16_t)(v))
#define COUNT ((int16_t)NW(plan_count, 0))
#define TAKE(d, who) (pwb(chosen, (d), (who)), SET_FIELD(d, 1), SET_CANDS(d, 0))
        switch (step) {
        case 0:
            SETB(plan_target, 0, 0);
            STEP(1);
            return 0;
        case 1:
            best = -1;
            SETB(plan_unit, 0, 0);
            for (u = 0; u <= 0xF0; u++) {
                uint16_t fl = UW(u, 4);

                if (HALF2(fl) || !same_side(other, fl) || fl & 0xC002
                    || (int16_t)P_W(u, 6) <= best || (int16_t)P_W(u, 2) < 0)
                    continue;
                best = (int16_t)P_W(u, 6);
                SETB(plan_target, 0, u);
            }
            if (best >= 0) {
                STEP(2);
                for (i = 0; i < 6; i++)
                    SET_CANDS(i, 0);
            } else
                STEP(0x0B);
            return 0;
        case 2:
            STEP(3);
            return 0;
        case 3:                 /* the next unit that could attack it */
            for (u = CUR; u <= 0xF0; u++) {
                uint16_t fl = UW(u, 4);
                uint8_t kind = UB(u, 8);

                if (HALF2(fl) || unit_outmatched((uint8_t)u, TARGET) < 0)
                    continue;
                if (!same_side(side, fl) || fl & 0xC000
                    || (int16_t)P_W(u, 4) >= (int16_t)P_W(TARGET, 6))
                    continue;
                if (fields_apart(POS(u), POS(TARGET)) >= (int16_t)(KB(kind, 0) >> 1))
                    continue;
                if (!(KW(kind, 6) & UW(TARGET, 4) & 0x3C) || KW(kind, 6) & 0x40)
                    continue;
                SETB(plan_unit, 0, u);
                break;
            }
            STEP(u <= 0xF0 ? 4 : 6);
            return 0;
        case 4:
            move_range(POS(CUR), buf, CUR, UB(CUR, 0), side,
                       UW(CUR, 6) & 1 ? 0xFFFF : UW(CUR, 4) & 1 ? 6 : 3, map);
            STEP(5);
            return 0;
        case 5: {               /* the fields around the target it could take */
            uint8_t bit = side ? 2 : 1;

            field_neighbours((uint16_t)POS(TARGET));
            for (i = 0; i < 6; i++) {
                int16_t f = (int16_t)NW(neighbours, i * 2);
                uint16_t ground;

                if (CANDS(i) >= 0x14 || f < 0 || field_pinched((uint16_t)f, side))
                    continue;
                if (POS(CUR) != f) {
                    if (!(NB(field_bits, bits_index(f)) & bit))
                        continue;
                    ground = NW(ww1gnds_dat, pb(map, (uint16_t)f) * 6);
                    if (pb(map, (uint16_t)(f + 1)) <= 0xF0 || field_pinched((uint16_t)f, side)
                        || ground & 0x540 || (UW(CUR, 4) & 0x10 && ground & 0x4000))
                        continue;
                    for (u = 0; u <= 0xF0; u++)
                        if (P_W(u, 0) == (uint16_t)f)
                            break;
                    if (u <= 0xF0)
                        continue;
                }
                pwb(aims, (uint16_t)(i * 0x14 + CANDS(i)), CUR);
                SET_CANDS(i, CANDS(i) + 1);
            }
            field_bits_clear(bit);
            CUR_NEXT;
            STEP(2);
            return 0;
        }
        case 6:                 /* which of the six fields are open */
            SETW(plan_count, 0, 6);
            field_neighbours((uint16_t)POS(TARGET));
            for (i = 0; i < 6; i++) {
                int16_t f = (int16_t)NW(neighbours, i * 2);

                pwb(chosen, i, 0xFF);
                if (f < 0) {
                    SET_FIELD(i, 0xFFFF);
                    SETW(plan_count, 0, COUNT - 1);
                    continue;
                }
                pww(fields, (uint16_t)(i * 2), (uint16_t)f);
                SET_FIELD(i, 0);
                if (field_pinched((uint16_t)f, side)) {
                    SET_FIELD(i, 0xFFFF);
                    SETW(plan_count, 0, COUNT - 1);
                    continue;
                }
                for (u = 0; u <= 0xF0; u++)
                    if (!(UW(u, 4) & 0xC000) && same_side(UW(u, 4), side) && P_B(u, 8)
                        && P_W(u, 0) == (uint16_t)f) {
                        SET_FIELD(i, 1);
                        SETW(plan_count, 0, COUNT - 1);
                        break;
                    }
            }
            if (COUNT > 0) {
                STEP(7);
                SETW(plan_count, 0, 0);
            } else
                STEP(0x0A);
            return 0;
        case 7:                 /* the units that fit the target, at most four */
            for (i = 0; i < 6; i++) {
                uint8_t n;

                if (FIELD(i))
                    continue;
                for (n = 0; CANDS(i) > n && COUNT < 4; n++) {
                    uint8_t who = CAND(i, n), ok = !unit_chosen(who, side);
                    uint16_t by, fl;

                    if ((int16_t)P_W(TARGET, 6) <= (int16_t)P_W(who, 4) || !ok)
                        continue;
                    by = P_W(TARGET, 2);
                    fl = UW(who, 4);
                    if (!(by & 1 || (by & 2 && fl & 0x10) || (by & 8 && fl & 4) || (by & 4 && fl & 8)))
                        continue;
                    TAKE(i, who);
                    SETW(plan_count, 0, COUNT + 1);
                }
            }
            STEP(8);
            return 0;
        case 8: {               /* more of them: opposite, between, any */
            uint8_t n;

            for (i = 0; i < 6 && COUNT < 4; i++) {
                uint8_t o = dir_wrap((uint8_t)(i + 3));

                if (FIELD(i) == 1 && FIELD(o) == 0) {
                    for (n = 0; CANDS(o) > n; n++) {
                        uint8_t who = CAND(o, n);

                        if (unit_chosen(who, side))
                            continue;
                        pwb(chosen, o, who);
                        SET_FIELD(o, 1);
                        n = CANDS(o);
                        SET_CANDS(o, 0);
                        SETW(plan_count, 0, COUNT + 1);
                    }
                } else if (FIELD(i) == 0 && FIELD(o) == 0 && CANDS(i) && CANDS(o)) {
                    uint8_t a = 0xFF, b = 0xFF;

                    for (n = 0; CANDS(i) > n; n++)
                        if (!unit_chosen(CAND(i, n), side))
                            a = CAND(i, n);
                    for (n = 0; CANDS(o) > n; n++)
                        if (!unit_chosen(CAND(o, n), side) && CAND(o, n) != a)
                            b = CAND(o, n);
                    if (a == 0xFF || b == 0xFF)
                        continue;
                    pwb(chosen, i, a);
                    pwb(chosen, o, b);
                    SET_FIELD(i, 1);
                    SET_FIELD(o, 1);
                    SET_CANDS(i, 0);
                    SET_CANDS(o, 0);
                    SETW(plan_count, 0, COUNT + 2);
                }
            }
            for (i = 0; i < 6; i++) {
                if (FIELD(i) || FIELD(dir_wrap((uint8_t)(i + 1))) != 1
                    || FIELD(dir_wrap((uint8_t)(i + 5))) != 1 || COUNT >= 4)
                    continue;
                for (n = 0; CANDS(i) > n; n++)
                    if (!unit_chosen(CAND(i, n), side)) {
                        TAKE(i, CAND(i, n));
                        SETW(plan_count, 0, COUNT + 1);
                    }
            }
            for (i = 0; i < 6; i++) {
                if (COUNT >= 4)
                    continue;
                for (n = 0; CANDS(i) > n; n++)
                    if (!unit_chosen(CAND(i, n), side)) {
                        TAKE(i, CAND(i, n));
                        SETW(plan_count, 0, COUNT + 1);
                    }
            }
            STEP(9);
            return 0;
        }
        case 9:                 /* their plans: there, and then the attack */
            for (i = 0; i < 6; i++) {
                uint8_t who = pb(chosen, i);
                uint16_t f = pw(fields, (uint16_t)(i * 2));

                if (who == 0xFF)
                    continue;
                SP_B(who, 8, (uint16_t)POS(who) == f ? 4 : 3);
                SP_W(who, 0, f);
                SP_W(who, 4, P_W(TARGET, 6));
                SETB(unit_plans2, who * PLAN_SIZE + 8, 2);
                SETW(unit_plans2, who * PLAN_SIZE, POS(TARGET));
                SETW(unit_plans2, who * PLAN_SIZE + 4, P_W(TARGET, 6));
            }
            STEP(0x0A);
            return 0;
        case 0x0A:
            SP_W(TARGET, 2, 0xFFFF);
            SETB(plan_target, 0, TARGET + 1);
            STEP(1);
            return 0;
        default:
            STEP(0);
            STATE(0x0B);
            return 0;
        }
    }
    case 0x0B:                  /* the units still without a plan */
        switch (step) {
        case 0:
            SETB(plan_unit, 0, 0);
            STEP(1);
            for (u = 0; u <= 0xF0; u++)
                SP_W(u, 2, 0x3E8);
            return 0;
        case 1:
            for (; CUR <= 0xF0; CUR_NEXT) {
                if (HALF2(UW(CUR, 4)))
                    continue;
                if (!(UW(CUR, 4) & 0xC000) && !(UW(CUR, 6) & 0x20) && !(KWORTH(CUR) & 0x380)
                    && same_side(UW(CUR, 4), side) && !P_B(CUR, 8))
                    break;
            }
            STEP(CUR <= 0xF0 ? 2 : 0x0A);
            return 0;
        case 2:                 /* how far every other unit is, or will be */
            for (u = 0; u <= 0xF0; u++) {
                if (UW(u, 4) & 0xC002 || UW(u, 6) & 0x20 || u == CUR)
                    continue;
                SP_W(u, 2, fields_apart(POS(CUR), same_side(UW(u, 4), side) ? (int16_t)P_W(u, 0)
                                                                            : POS(u)));
            }
            STEP(5);
            return 0;
        case 5: {               /* the target worth most it can attack */
            uint16_t can = KW(UB(CUR, 8), 6) & 0x3C;

            best = -1;
            SETB(plan_other, 0, 0xFF);
            for (u = 0; u <= 0xF0; u++) {
                uint16_t fl = UW(u, 4);

                if (fl & 0xC000 || !same_side(fl, other) || (int16_t)P_W(u, 6) <= best
                    || (int16_t)P_W(u, 2) >= 0x3E8 || !(can & fl))
                    continue;
                best = (int16_t)P_W(u, 6);
                SETB(plan_other, 0, u);
            }
            STEP(NB(plan_other, 0) <= 0xF0 ? 6 : 7);
            return 0;
        }
        case 6: {
            int16_t f = POS(NB(plan_other, 0));

            if (unit_reaches(f, CUR, side, 0))
                FOLLOW(f);
            else {
                SP_W(NB(plan_other, 0), 2, 0x3E8);
                STEP(5);
            }
            return 0;
        }
        case 7:                 /* else the nearest unit of the side with a plan */
            best = 0x3E8;
            SETB(plan_other, 0, 0xFF);
            for (u = 0; u <= 0xF0; u++) {
                uint16_t fl = UW(u, 4);
                uint8_t k = P_B(u, 8);

                if (fl & 0xC000 || KWORTH(u) & 0x380 || !same_side(fl, side)
                    || (int16_t)P_W(u, 4) <= 0x0A || (int16_t)P_W(u, 2) >= best
                    || !k || k == 8 || k == 0x0C || k == 6)
                    continue;
                best = (int16_t)P_W(u, 2);
                SETB(plan_other, 0, u);
            }
            STEP(NB(plan_other, 0) <= 0xF0 ? 8 : 9);
            return 0;
        case 8: {
            uint16_t f = P_W(NB(plan_other, 0), 0);

            if (unit_reaches((int16_t)f, CUR, side, 0))
                FOLLOW(f);
            else {
                SP_W(NB(plan_other, 0), 2, 0x3E8);
                STEP(7);
            }
            return 0;
        }
        case 9:                 /* else a carrier home */
            if (KWORTH(CUR) & 0x380
                || plan_ride(side, CUR, 0x46, 0x0B, NW(sites_40, side * SITE_SIZE + 0x0E))) {
                STEP(1);
                CUR_NEXT;
            }
            return 0;
        default:
            STEP(0);
            STATE(0x0C);
            return 0;
        }
    default:
        STATE(0);
        pwb(phase, 6, 0);
        return 1;
    }
}

/* computer_turn (S1C74:000B): one pass of a computer's side, in place of
 * reading the side's input.  The phase word of computer_phase chooses:
 * 20h computer_assess, 40h computer_plan, 8 computer_queue (one a pass;
 * the next phase when it returns not 0), then in the same pass 2
 * computer_step and 1 computer_input.  The number of the side's text
 * goes to computer_out's second word. */
void computer_turn(uint16_t side)
{
    Far phase = phase_of(side);
    int left = 0;

#define TEXT(t) SETW(computer_out, side * 4 + 2, t)
#define PHASE(off, on) pww(phase, 0, (uint16_t)((pw(phase, 0) & (uint16_t)~(off)) | (on)))
#define LIST_AT_0 SETB(computer_steps, side * 0x49 + 0x46, 0)
    TEXT(0x4E);
    if (pw(phase, 0) & 0x20) {
        TEXT(0x61);
        if (computer_assess(side)) {
            PHASE(0x20, 0x40);
            LIST_AT_0;
        }
    } else if (pw(phase, 0) & 0x40) {
        if (pb(phase, 0x0B)) {
            TEXT(0x62);
            pwb(phase, 0x0B, 0);
        } else {
            TEXT(0x63);
            pwb(phase, 0x0B, 1);
        }
        if (computer_plan(side)) {
            PHASE(0x40, 8);
            LIST_AT_0;
        }
    } else if (pw(phase, 0) & 8) {
        TEXT(0x60);
        if (computer_queue(side)) {
            PHASE(8, 2);
            LIST_AT_0;
        }
    }
    if (pw(phase, 0) & 2) {
        TEXT(0x4F);
        if (computer_step(side, &left))
            PHASE(0, 8);
        else {
            PHASE(0, 1);
            SETB(computer_script, side * 0x20 + 0x1E, 0);
        }
        PHASE(2, 0);
    }
    if (pw(phase, 0) & 1) {
        TEXT(0x4F);
        if (computer_input(side))
            PHASE(1, 2);
    }
#undef TEXT
#undef PHASE
#undef LIST_AT_0
}

/* S1C74:0354: how far the side's computer has got with its units, 0 to
 * 63h, for the bar game_loop draws.  Counted are the units (0 to F0h) on
 * the side's side but the second halves of two-part ones, those with
 * C000h in word 4, 20h in word 6 or 380h in the kind's word of
 * kind_worth; done are those whose byte 8 in the side's table of plans is
 * 0.  64h less done * 100h / (count * 100h / 64h), 0 to 63h; 0 when none
 * is counted. */
uint16_t computer_progress(uint16_t side)
{
    Far plans = plans_of(side);
    int32_t n = 0, done = 0, p;
    unsigned u;

    for (u = 0; u <= 0xF0; u++) {
        uint16_t w4 = UW(u, 4);

        if ((w4 & 0x40) && !(w4 & 0x80))
            continue;
        if ((w4 & 0xC000) || (UW(u, 6) & 0x20) || (NW(kind_worth, UB(u, 8) * 6 + 2) & 0x380))
            continue;
        if (!same_side(w4, side))
            continue;
        n++;
        if (!pb(plans, (uint16_t)(u * PLAN_SIZE + 8)))
            done++;
    }
    if (n <= 0)
        return 0;
    p = 0x64 - (done << 8) / ((n << 8) / 0x64);
    if (p < 0)
        return 0;
    return (uint16_t)(p > 0x63 ? 0x63 : p);
}
