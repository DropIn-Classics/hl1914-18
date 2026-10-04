/* fight.c - what a fight costs: HL.EXE's fight_percent (S02E6:2635) and
 * fight_losses (S2487:000A) with the routines they call, translated over
 * the program's memory.  The original's check of the stack at each entry is
 * left out.  port/README.md says how this was compared with the original.
 */
#include "hl.h"

/* ---- the long arithmetic of the compiler's helpers */

static int32_t lmul(int32_t a, int32_t b)       /* long_mul: the low 32 bits */
{
    return (int32_t)((uint32_t)a * (uint32_t)b);
}

static int32_t ldiv32(int32_t a, int32_t b)     /* long_div: towards zero */
{
    /* the original would divide by zero here; no fight seen came to it */
    if (b == 0)
        return 0;
    if (a == INT32_MIN && b == -1)
        return a;
    return a / b;
}

/* ---- fields */

/* S0B15:0162: the map offsets of the six fields around `field` into
 * `neighbours`, clockwise from the one above; FFFFh beyond the map's edge.
 * A field in an odd column lies half a field lower. */
void field_neighbours(uint16_t field)
{
    uint16_t w = NW(map_width, 0), w2 = (uint16_t)(w << 1), n[6];
    int16_t half = (int16_t)((int16_t)field >> 1);
    int16_t col = (int16_t)(half % (int16_t)w), row = (int16_t)(half / (int16_t)w);
    int16_t right = (int16_t)(w - 1), bottom = (int16_t)(NW(map_height, 0) - 1);
    int i;

    n[0] = (uint16_t)(field - w2);
    n[3] = (uint16_t)(field + w2);
    if (col & 1) {
        n[1] = (uint16_t)(field + 2);
        n[5] = (uint16_t)(field - 2);
        n[4] = (uint16_t)(field + w2 - 2);
        n[2] = (uint16_t)(field + w2 + 2);
        if (row <= 0)
            n[0] = 0xFFFF;
        else if (bottom <= row)
            n[3] = n[2] = n[4] = 0xFFFF;
        if (col <= 0)
            n[5] = n[4] = 0xFFFF;
        else if (right <= col)
            n[1] = n[2] = 0xFFFF;
    } else {
        n[2] = (uint16_t)(field + 2);
        n[4] = (uint16_t)(field - 2);
        n[1] = (uint16_t)(field - w2 + 2);
        n[5] = (uint16_t)(field - w2 - 2);
        if (row <= 0)
            n[0] = n[5] = n[1] = 0xFFFF;
        else if (bottom <= row)
            n[3] = 0xFFFF;
        if (col <= 0)
            n[4] = n[5] = 0xFFFF;
        else if (right <= col)
            n[1] = n[2] = 0xFFFF;
    }
    for (i = 0; i < 6; i++)
        SETW(neighbours, 2 * i, n[i]);
}

/* S0CB6:0205: two units' unit_flags; is the second on the first's side */
int same_side(uint16_t flags, uint16_t other)
{
    if (flags & 1)
        return (other & 1) != 0;
    if (flags & 2)
        return (other & 2) != 0;
    return !(other & 1);
}

/* S0E56:241F, the arguments in the order of its stack */
int16_t field_distance(int16_t a1, int16_t a2, int16_t b1, int16_t b2)
{
    int16_t t, d;

    if (a2 > b2) {
        t = a1, a1 = b1, b1 = t;
        t = a2, a2 = b2, b2 = t;
    }
    if (b1 >= a1) {
        a2 = (int16_t)(a2 - a1 / 2);
        b2 = (int16_t)(b2 - b1 / 2);
    } else {
        t = a1, a1 = b1, b1 = t;
        a2 = (int16_t)(a2 - a1 / 2 - a1 % 2);
        b2 = (int16_t)(b2 - b1 / 2 - b1 % 2);
    }
    d = (int16_t)(b2 - a2);
    if (d < 0)
        d = 0;
    return (int16_t)(b1 - a1 + d);
}

/* ---- the per cent bytes */

static uint16_t unit_field(uint8_t unit, uint16_t side)
{
    return (uint16_t)(NW(unit_pos, unit * UNIT_SIZE + 2 * side) - 1);
}

static uint8_t dir_wrap(uint8_t d)              /* S02E6:2802 */
{
    while (d > 5)
        d = (uint8_t)(d - 6);
    return d;
}

/* S02E6:2834: leaves 1 in `neighbours` for each field around `other` with a
 * unit on `unit`'s side that could attack `other`, 0 for the rest; returns
 * which of the six holds `unit` (as the original, whatever was in its
 * variable when none does: 0 here). */
uint8_t fight_around(uint8_t unit, uint8_t other, Far map, uint16_t side)
{
    uint16_t classes = NW(unit_flags, other * UNIT_SIZE) & 0x3C;
    uint16_t two = NW(units, other * UNIT_SIZE + 6) & 2;
    uint8_t i, where = 0;

    field_neighbours(unit_field(other, side));
    for (i = 0; i < 6; i++) {
        uint16_t field = NW(neighbours, 2 * i);
        uint8_t u = pb(map, (uint16_t)(field + 1));
        uint16_t can;

        SETW(neighbours, 2 * i, 0);
        if (field == 0xFFFF || u > 0xF0)
            continue;
        can = frw(S_unit_dat, (uint16_t)(A_unit_dat + NB(unit_kind, u * UNIT_SIZE) * KIND_SIZE + 6));
        if ((classes & can || two & can) && !(can & 0x40)
            && same_side(NW(unit_flags, u * UNIT_SIZE), NW(unit_flags, unit * UNIT_SIZE)))
            SETW(neighbours, 2 * i, 1);
        if (u == unit)
            where = i;
    }
    return where;
}

/* S02E6:2635: bytes 30h and 31h of fight_rec, the per cent added to the
 * attacker's attack and to the target's defence. */
void fight_percent(uint8_t attacker, uint8_t target, Far map, uint16_t side)
{
    uint8_t i, n = 0, where, sum;

    field_neighbours(unit_field(target, side));
    for (i = 0; i < 6; i++) {
        uint16_t at = (uint16_t)(NW(neighbours, 2 * i) + 1);
        /* a field beyond the edge (FFFFh) is not skipped: the map's byte 0 */
        if (at != 0xFFFF && pb(map, at) != attacker)
            n++;
    }
    if (n == 6) {                       /* not neighbours */
        SETB(fight_rec, 0x30, 0);
        SETB(fight_rec, 0x31, 0);
        return;
    }

    /* the attack: the attacker's side on the other five fields around the
     * target */
    where = fight_around(attacker, target, map, side);
    sum = 0;
    for (i = 1; i < 6; i++)
        if (NW(neighbours, 2 * dir_wrap((uint8_t)(where + i))) != 0)
            sum = (uint8_t)(sum + NB(around_percent, i));
    /* (the original bounds the sum at 12Ch here, which a byte never passes) */
    SETB(fight_rec, 0x30, sum);

    /* the defence: the target's side on the two fields that touch both */
    where = fight_around(target, attacker, map, side);
    sum = 0;
    if (NW(neighbours, 2 * dir_wrap((uint8_t)(where + 1))) != 0)
        sum = (uint8_t)(sum + 0x50);
    if (NW(neighbours, 2 * dir_wrap((uint8_t)(where + 5))) != 0)
        sum = (uint8_t)(sum + 0x50);
    if (sum > 0x96)
        sum = 0x96;
    SETB(fight_rec, 0x31, sum);
}

/* ---- the losses */

static int32_t count_of(Far unit, Far kind)
{
    int32_t n = pw(kind, 0x0F) & 4 ? pb(unit, 3) : pb(unit, 2);
    return n == 0 ? 1 : n > 6 ? 6 : n;
}

/* a kind's attack against a unit with these unit_flags; *reach with it */
static int16_t attack_of(Far kind, uint16_t flags, int16_t *reach)
{
    if (flags & 0x10) {
        *reach = pb(kind, 8);
        return pb(kind, 0x0A);
    }
    *reach = pb(kind, 9);
    return pb(kind, flags & 8 ? 0x0C : 0x0B);
}

/* a 16-bit value raised by pc per cent, as the routine works it out */
static int32_t percent(uint16_t value, uint8_t pc)
{
    int32_t hi = (int32_t)((uint32_t)value << 16);
    return (int16_t)((uint32_t)(hi + lmul(pc, ldiv32(hi, 100))) >> 16);
}

/* what a unit of `count` brings to bear with a value, its experience plus
 * one and the ground's bonus */
static int32_t strength(int32_t count, int32_t value, int32_t exp1, int8_t ground, int32_t by)
{
    int32_t e = ldiv32((int32_t)((uint32_t)exp1 << 8), 6) + 1;
    return (int32_t)((uint32_t)lmul(count, value)
                     + (uint32_t)ldiv32(lmul(count, lmul(value, e)), by)
                     + (uint32_t)lmul(ground, ldiv32(value, 2)));
}

/* S2487:000A: takes a fight's losses off the two units of the record that
 * fight_rec_ptr points to (the attacker first). */
void fight_losses(void)
{
    Far rec = far_at(S_fight_rec_ptr, A_fight_rec_ptr);
    Far ua = pfar(rec, 0), ub = pfar(rec, 4);
    Far ka = pfar(rec, 0x0A), kb = pfar(rec, 0x0E);
    Far ga = pfar(rec, 0x12), gb = pfar(rec, 0x16);
    uint16_t near_by = pw(rec, 0x2E), width = NW(map_width, 0);
    int quiet = 0;                      /* the attacker takes no loss */
    int32_t cnt_a, cnt_b, exp_a, exp_b, att_a, att_b, def_a, def_b;
    int32_t p, q, r, s, e_a, e_b, loss_a, loss_b;
    int16_t reach, unused;

    if (pw(kb, 6) & 0x80)
        quiet = 1;
    if (near_by == 0)
        quiet = 1;
    if (!(pw(ka, 0x0D) & (pw(kb, 6) & 0x3C)))
        quiet = 1;
    if (near_by != 0 && pw(kb, 6) & 0x40)
        quiet = 1;

    cnt_a = count_of(ua, ka);
    cnt_b = count_of(ub, kb);

    exp_b = pb(ub, 1) + 1;
    def_b = pb(kb, 2);
    att_a = attack_of(ka, pw(ub, 4), &reach);
    if (reach > 1) {
        uint16_t fa = pw(rec, 0x1A) >> 1, fb = pw(rec, 0x1C) >> 1;
        int16_t d = field_distance((int16_t)(fa / width), (int16_t)(fa % width),
                                   (int16_t)(fb / width), (int16_t)(fb % width));
        att_a -= ldiv32(lmul(3, lmul(att_a, (int16_t)(d - 1))), (int16_t)(reach << 2));
    }

    exp_a = pb(ua, 1) + 1;
    def_a = pb(ka, 2);
    att_b = attack_of(kb, pw(ua, 4), &unused);

    att_a = percent((uint16_t)att_a, pb(rec, 0x30));
    def_b = percent((uint16_t)def_b, pb(rec, 0x31));

    /* rand_seed, through the record's pointer at 6Ah: no number is drawn
     * here */
    SETW(rand_state, 0, (uint16_t)((pw(rec, 0) + pw(rec, 4)) ^ pw(rec, 0x3A)));
    SETW(rand_state, 2, 0);

    p = strength(cnt_a, att_a, exp_a, (int8_t)NB(ground_bonus, 2 * pb(ga, 5)), 0x100);
    q = strength(cnt_a, def_a, exp_a, (int8_t)NB(ground_bonus, 2 * pb(ga, 5) + 1), 0x100);
    r = strength(cnt_b, att_b, exp_b, (int8_t)NB(ground_bonus, 2 * pb(gb, 5)), 0x100);
    s = strength(cnt_b, def_b, exp_b, (int8_t)NB(ground_bonus, 2 * pb(gb, 5) + 1), 0x200);

    /* the two tests differ, as the original has them */
    e_a = exp_a < 1 ? 2 : exp_a > 7 ? 6 : exp_a;
    e_b = exp_b <= 1 ? 2 : exp_b >= 7 ? 6 : exp_b;
    loss_b = ldiv32((int32_t)((uint32_t)p - (uint32_t)s), ldiv32(lmul(def_b, 8 - e_a), 2));
    loss_a = ldiv32((int32_t)((uint32_t)r - (uint32_t)q), ldiv32(lmul(def_a, 8 - e_b), 2));

    if (loss_b <= 0 && ldiv32(s, cnt_b) + ldiv32(def_b, 2) < p)
        loss_b = 1;
    if (loss_a <= 0 && ldiv32(q, cnt_a) + ldiv32(def_a, 2) < r)
        loss_a = 1;
    if (loss_b > 6 && loss_b < 12)
        loss_b = 6;
    if (loss_a > 6 && loss_a < 12)
        loss_a = 6;
    if (loss_b > 2 * cnt_a)
        loss_b = 2 * cnt_a;
    if (loss_a > 2 * cnt_b)
        loss_a = 2 * cnt_b;
    loss_b = loss_b < 0 ? 0 : loss_b > cnt_b ? cnt_b : loss_b;
    loss_a = loss_a < 0 ? 0 : loss_a > cnt_a ? cnt_a : loss_a;

    if (!quiet) {
        uint8_t left = (uint8_t)(cnt_a - loss_a);
        if (pw(ka, 0x0F) & 4) {
            pwb(ua, 3, left);
            if (left == 0)
                pwb(ua, 2, 0);
        } else
            pwb(ua, 2, left);
    }
    {
        uint8_t left = (uint8_t)(cnt_b - loss_b);
        if (pw(kb, 0x0F) & 4) {
            pwb(ub, 3, left);
            if (left == 0)
                pwb(ub, 2, 0);
        } else
            pwb(ub, 2, left);
    }
}
