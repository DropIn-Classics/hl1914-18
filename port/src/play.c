/* play.c - the port's program plays a battle by itself: the passes of
 * game_loop (loop.c) from the port's own setup (battle.c), the human's
 * input from a key script through the port's translation of the
 * interrupts (input.c), the computer's side by computer_turn.
 *
 * The key script is the runner's (doskit/tools/run, -keys; the scripts of
 * port/tests/keys): "T KEY" a line, T in seconds, KEY a name or a hex
 * scancode (E0xx an extended key), with + or - the key down or up only,
 * without it tapped (up 0.15 s later); # begins a comment.
 *
 * What the original's pass reads depends on when it reads it, and its
 * passes last as long as its drawing does, which the port does not do.
 * So there are two clocks:
 *
 * - the port's own: the timer's routine (input_tick) is called every
 *   3FFFh of the timer's 1,193,182 counts a second, a pass lasts as many
 *   calls as game_loop waits for at its end (game_state's word 0Fh, at
 *   least one here), and a round's end as long as its two changes of
 *   the screen's colours count retraces (60h each, without a site taken);
 *   everything else takes no time, the fights' pictures and waits
 *   included.  The BIOS's count of ticks, from which the random numbers
 *   are started at the setup and where the fight's pictures would be,
 *   runs with it from a value given (play_bios).
 * - the original's, from a log of a run of it (port/tests/play_dumps.py):
 *   a line for each call of the timer's routine and for each place where
 *   a pass reads what the interrupts left, with its time, and the numbers
 *   clock_seed started the random numbers with.  The lines:
 *       TIME T          S27FE:008C, the timer's routine
 *       TIME P N SIDE   S066A:0FC3, a side's part of pass N begins
 *       TIME I          S0CB6:05F1, side_input
 *       TIME V          S0CB6:0750, view_step
 *       TIME C          S066A:10D6, computer_turn has returned in the loop
 *                       that runs while the computer thinks
 *       TIME A          S02E6:000F, attacks_run is called
 *       TIME K          S27FE:04F4, the keyboard's interrupt (the runner
 *                       brings a key up to a millisecond after its time)
 *   and what the port does not do itself, to be done at that place:
 *       TIME S HEX      clock_seed has set rand_words (word 2 above word 0)
 *       TIME R          S2C9A:0008, rand_next
 *       TIME W          S0CB6:0075, 00C1, 00E2: the routine that waits
 *                       (S0CB6:0036) has cleared the first record's
 *                       button and the key
 *       TIME X          S2532:0137: the fight's pictures have cleared the
 *                       key
 *   The port's own keys, timer routine and passes are run in that order.
 *   S, R, W and X count from attacks_run's call to its last such line in
 *   the side's part (the pictures and waits of a round's end, which the
 *   port leaves out; rand_next elsewhere is the port's own computer's).
 *   A place the port comes to that the log does not have in the side's
 *   part, or the other way round, is counted in play_misses.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hl.h"

#define S_DATA (HL_DATA + HL_BASE)
#define TICK (0x3FFF / 1193182.0)       /* the timer routine's period, s */
#define BIOS_TICK (65536 / 1193182.0)   /* the BIOS's */
#define RETRACE 0.014268                /* the screen's vertical retraces */

typedef struct { double t; uint8_t code[2]; int n; long order; } KeyEv;
typedef struct { double t; char kind; int a, b; uint32_t v; } ClockLine;

static KeyEv *keys;
static long keys_n, keys_at;
static int key_byte;            /* the next byte of keys[keys_at], by K lines */
static int by_k, in_battle;     /* the log has K lines; its first P is passed */
static ClockLine *clk;
static long clk_n, clk_at;
static int have_clock;
static FILE *inputs;

static SideIn in[2];
static int seen[2][4];          /* for -inputs: key, word, bytes 5 and 8 (-1 none) */
static int thinks;              /* calls of computer_turn in the thinking loop */
static uint16_t aim_kept;       /* fight_rec's word 1Ch over attacks_run */
static double now, began;       /* the port's own clock, s */
static double tick_at;          /* when the timer's routine is called next */
static uint32_t bios0;          /* the BIOS's count when it began */
static const char *stopped;     /* what the timer's routine met */

unsigned long play_passes, play_misses;

static const struct { const char *name; uint16_t code; } names[] = {
    { "esc", 0x01 }, { "1", 0x02 }, { "2", 0x03 }, { "3", 0x04 }, { "4", 0x05 }, { "5", 0x06 },
    { "6", 0x07 }, { "7", 0x08 }, { "8", 0x09 }, { "9", 0x0A }, { "0", 0x0B }, { "backspace", 0x0E },
    { "tab", 0x0F }, { "q", 0x10 }, { "w", 0x11 }, { "e", 0x12 }, { "r", 0x13 }, { "t", 0x14 },
    { "y", 0x15 }, { "u", 0x16 }, { "i", 0x17 }, { "o", 0x18 }, { "p", 0x19 }, { "enter", 0x1C },
    { "lctrl", 0x1D }, { "a", 0x1E }, { "s", 0x1F }, { "d", 0x20 }, { "f", 0x21 }, { "g", 0x22 },
    { "h", 0x23 }, { "j", 0x24 }, { "k", 0x25 }, { "l", 0x26 }, { "lshift", 0x2A }, { "z", 0x2C },
    { "x", 0x2D }, { "c", 0x2E }, { "v", 0x2F }, { "b", 0x30 }, { "n", 0x31 }, { "m", 0x32 },
    { "rshift", 0x36 }, { "lalt", 0x38 }, { "space", 0x39 }, { "f1", 0x3B }, { "f2", 0x3C },
    { "f3", 0x3D }, { "f4", 0x3E }, { "f5", 0x3F }, { "f6", 0x40 }, { "f7", 0x41 }, { "f8", 0x42 },
    { "f9", 0x43 }, { "f10", 0x44 }, { "f11", 0x57 }, { "f12", 0x58 }, { "rctrl", 0xE01D },
    { "ralt", 0xE038 }, { "up", 0xE048 }, { "down", 0xE050 }, { "left", 0xE04B }, { "right", 0xE04D },
    { NULL, 0 }
};

static int key_code(const char *name, uint16_t *code)
{
    char *end;
    unsigned long v;
    int i;

    for (i = 0; names[i].name; i++)
        if (!strcmp(names[i].name, name)) {
            *code = names[i].code;
            return 1;
        }
    v = strtoul(name, &end, 16);
    if (end == name || *end || v > 0xFFFF)
        return 0;
    *code = (uint16_t)v;
    return 1;
}

static int key_add(double t, uint16_t code, int down)
{
    KeyEv *k;

    if (!(keys_n & 255)) {
        k = realloc(keys, (size_t)(keys_n + 256) * sizeof *keys);
        if (!k)
            return 0;
        keys = k;
    }
    k = &keys[keys_n];
    k->t = t;
    k->order = keys_n++;
    k->n = 0;
    if (code & 0xE000)
        k->code[k->n++] = 0xE0;
    k->code[k->n++] = (uint8_t)((code & 0x7F) | (down ? 0 : 0x80));
    return 1;
}

static int key_cmp(const void *a, const void *b)
{
    const KeyEv *x = a, *y = b;

    if (x->t != y->t)
        return x->t < y->t ? -1 : 1;
    return x->order < y->order ? -1 : 1;
}

const char *play_keys(const char *path)
{
    FILE *f = fopen(path, "r");
    char line[200], name[40];
    double t;

    if (!f)
        return "the key script cannot be opened";
    while (fgets(line, sizeof line, f)) {
        size_t n;
        uint16_t code;
        int down = -1;

        line[strcspn(line, "#\r\n")] = 0;
        if (sscanf(line, "%lf %39s", &t, name) != 2)
            continue;
        n = strlen(name);
        if (n > 1 && (name[n - 1] == '+' || name[n - 1] == '-')) {
            down = name[n - 1] == '+';
            name[n - 1] = 0;
        }
        if (!key_code(name, &code)) {
            fclose(f);
            return "the key script names a key that is not known";
        }
        if (!(down < 0 ? key_add(t, code, 1) && key_add(t + 0.15, code, 0) : key_add(t, code, down))) {
            fclose(f);
            return "no memory for the key script";
        }
    }
    fclose(f);
    qsort(keys, (size_t)keys_n, sizeof *keys, key_cmp);
    return NULL;
}

const char *play_clock(const char *path)
{
    FILE *f = fopen(path, "r");
    char line[100];

    if (!f)
        return "the clock's file cannot be opened";
    while (fgets(line, sizeof line, f)) {
        ClockLine c, *p;
        unsigned long v = 0;
        char kind;

        memset(&c, 0, sizeof c);
        if (sscanf(line, "%lf %c", &c.t, &kind) != 2)
            continue;
        c.kind = kind;
        if (kind == 'P' && sscanf(line, "%*f %*c %d %d", &c.a, &c.b) != 2)
            continue;
        if (kind == 'S' && sscanf(line, "%*f %*c %lx", &v) != 1)
            continue;
        c.v = (uint32_t)v;
        if (kind == 'K')
            by_k = 1;
        if (!(clk_n & 4095)) {
            p = realloc(clk, (size_t)(clk_n + 4096) * sizeof *clk);
            if (!p) {
                fclose(f);
                return "no memory for the clock's file";
            }
            clk = p;
        }
        clk[clk_n++] = c;
    }
    fclose(f);
    have_clock = 1;
    return NULL;
}

/* a file that gets a line for each side's part: pass, side, the key taken
 * and for a human the input record's bits as side_input finds them (hex)
 * and its bytes 5 and 8 when view_step is called (-1 when not), to compare
 * with what a log of the original shows */
void play_inputs(const char *path)
{
    inputs = fopen(path, "w");
}

/* the BIOS's count of ticks (0040:006C), from which clock_seed(0) starts
 * the random numbers */
void play_bios(uint32_t ticks)
{
    bios0 = ticks;
    fww(0x40, 0x6C, (uint16_t)ticks);
    fww(0x40, 0x6E, (uint16_t)(ticks >> 16));
}

static void seed_set(uint32_t v)
{
    SETW(rand_words, 0, v);
    SETW(rand_words, 2, v >> 16);
}

/* clock_seed(0), S2C95:000E */
static uint32_t clock_seed0(void)
{
    uint16_t hi = frw(0x40, 0x6C) ^ frw(0x40, 0x6E), lo = frw(0x40, 0x6D) | frw(0x40, 0x6F);

    return (uint32_t)hi << 16 | lo;
}

/* the keys of the script up to time t, each byte an interrupt */
static void keys_to(double t)
{
    for (; keys_at < keys_n && keys[keys_at].t <= t; keys_at++) {
        int i;

        for (i = 0; i < keys[keys_at].n; i++)
            key_irq(keys[keys_at].code[i]);
    }
}

static void tick(double t)
{
    const char *why;

    if (!by_k)
        keys_to(t);
    why = input_tick();
    if (why && !stopped)
        stopped = why;
}

/* the port's own clock moved on to time t: the calls of the timer's
 * routine that are due by then, and the BIOS's count with them */
static void own_to(double t)
{
    while (tick_at <= t) {
        uint32_t b;

        now = tick_at;
        tick_at += TICK;
        b = bios0 + (uint32_t)((now - began) / BIOS_TICK);
        fww(0x40, 0x6C, (uint16_t)b);
        fww(0x40, 0x6E, (uint16_t)(b >> 16));
        tick(now);
    }
    now = t;
}

/* one call of the timer's routine */
static void own_tick(void)
{
    own_to(tick_at);
}

/* n waits for the screen's next vertical retrace to begin (S2945:000A),
 * which the original's pictures count their steps by: 70.086 a second in
 * the game's screen mode (the runner's 0.014268 s) */
static void own_retraces(int n)
{
    long k = (long)(now / RETRACE + 1e-6);

    while (n-- > 0)
        own_to((double)++k * RETRACE);
}

/* the input record of a side as game_loop chooses it (S066A:0F0C to 0FC3) */
static Far rec_of(uint16_t side)
{
    int b = side & 1;

    if (!(NW(game_state, 2) & 2) && (int16_t)NW(map_number, 0) > 0x17 && (int16_t)NW(map_number, 0) < 0x30)
        b = !b;
    return far_at(S_DATA, b ? 0x00B6 : 0x00A6);
}

/* the log's lines from where the clock stands up to line `to`: the
 * timer's routine, the keys, and with `round` what the pictures and waits
 * of a round's end did; else a place of the pass among them is one the
 * port did not come to */
static void clock_run(long to, int round)
{
    Far k = far_at(S_DATA, 0x00BE);
    long i;

    for (i = clk_at; i < to; i++)
        switch (clk[i].kind) {
        case 'T':
            tick(clk[i].t);
            break;
        case 'K':
            if (in_battle && keys_at < keys_n) {
                key_irq(keys[keys_at].code[key_byte++]);
                if (key_byte >= keys[keys_at].n) {
                    keys_at++;
                    key_byte = 0;
                }
            }
            break;
        case 'S':
            seed_set(clk[i].v);
            break;
        case 'R':
            if (round)
                rand_next();
            break;
        case 'W':
            pwb(far_at(S_DATA, 0x00A6), 4, 0);
            fww(k.seg, (uint16_t)(k.off + 4), 0);
            break;
        case 'X':
            fww(k.seg, (uint16_t)(k.off + 4), 0);
            break;
        default:
            play_misses++;
        }
}

/* the original's clock run on to the next line of the kind in the side's
 * part under way (for P: to the next part's beginning, which should be
 * that of `pass` and `side`); 0 when the log has none there */
static int clock_to(char kind, int pass, int side)
{
    long i, at = -1;

    for (i = clk_at; i < clk_n; i++) {
        if (clk[i].kind == kind) {
            at = i;
            break;
        }
        if (clk[i].kind == 'P')
            break;
    }
    if (at < 0) {
        play_misses++;
        return 0;
    }
    if (kind == 'P' && (clk[at].a != pass || clk[at].b != side))
        play_misses++;
    clock_run(at, 0);
    if (!by_k)
        keys_to(clk[at].t);
    if (kind == 'P')
        in_battle = 1;
    clk_at = at + 1;
    return 1;
}

static void hook(int at, uint16_t side)
{
    SideIn *s = &in[side];
    Far k = far_at(S_DATA, 0x00BE);

    switch (at) {
    case LOOP_PART:
        memset(s, 0, sizeof *s);
        s->text = 0xFFFF;
        s->rec5 = s->rec8 = -1;
        s->turns = -1;
        thinks = 0;
        if (have_clock)
            clock_to('P', (int)play_passes + 1, side);
        /* S066A:0FD0: the key the timer's routine kept */
        if (pw(k, 4)) {
            fww(k.seg, (uint16_t)(k.off + 4), 0);
            s->key = pb(k, 0);
        }
        seen[side][0] = s->key;
        seen[side][1] = seen[side][2] = seen[side][3] = -1;
        break;
    case LOOP_INPUT: {
        Far r = rec_of(side);

        if (have_clock)
            clock_to('I', 0, 0);
        seen[side][1] = (pb(r, 4) ? 0x10 : 0) | (pb(r, 0) ? 4 : pb(r, 1) ? 8 : 0) | (pb(r, 2) ? 1 : pb(r, 3) ? 2 : 0);
        break;
    }
    case LOOP_STEP:
        if (have_clock)
            clock_to('V', 0, 0);
        if (!(NW(sides, side * 0x17) & 2)) {
            seen[side][2] = pb(rec_of(side), 5);
            seen[side][3] = pb(rec_of(side), 8);
        }
        break;
    case LOOP_THINK:
        thinks++;
        if (have_clock)
            clock_to('C', 0, 0);
        /* S066A:10EA to 1120: a human's button or a key ends the loop */
        if (pb(far_at(S_DATA, 0x00A6), 4) || pb(far_at(S_DATA, 0x00B6), 4) || pw(k, 4))
            s->turns = thinks;
        break;
    case LOOP_ROUND:
        if (have_clock)
            clock_to('A', 0, 0);
        else if (!NW(site_taken, 0))
            /* S0E56:21D8, the screen fades to grey: 20h and 40h steps, a
             * retrace each */
            own_retraces(0x60);
        /* a fight writes its target's field here, never FFFFh */
        aim_kept = NW(fight_rec, 0x1C);
        SETW(fight_rec, 0x1C, 0xFFFF);
        break;
    case LOOP_FOUGHT: {
        int fought = NW(fight_rec, 0x1C) != 0xFFFF, pictures;

        if (!fought)
            SETW(fight_rec, 0x1C, aim_kept);
        pictures = fought && !NB(no_fight_pics, 0);
        if (have_clock) {
            /* what the original's pictures and waits did, in the order
             * of its timer's routine and keys */
            long i, last = -1;
            int seeds = 0;

            for (i = clk_at; i < clk_n && clk[i].kind != 'P'; i++)
                if (strchr("SRWX", clk[i].kind)) {
                    last = i;
                    seeds |= clk[i].kind == 'S';
                }
            if (last >= 0) {
                clock_run(last + 1, 1);
                clk_at = last + 1;
            }
            if (seeds != pictures)
                play_misses++;
        } else {
            if (pictures) {
                /* the fight's pictures (S2532:0008) start the random
                 * numbers anew with clock_seed(0) and take the keys
                 * pressed meanwhile */
                s->seeded = 1;
                s->seed = clock_seed0();
                pwb(far_at(S_DATA, 0x00A6), 4, 0);
                fww(k.seg, (uint16_t)(k.off + 4), 0);
            }
            /* S0E56:1F91, the colours come back: 40h and 20h steps */
            if (!NW(site_taken, 0))
                own_retraces(0x60);
        }
        break;
    }
    default:
        break;
    }
}

/* after the port's setup (battle_begin): the first pass begins at `start`
 * seconds of the key script's time (the port's own clock; the original's
 * has its own times) */
void play_begin(double start)
{
    loop_hook = hook;
    loop_reset();
    now = began = start;
    tick_at = start + TICK;
    play_passes = play_misses = 0;
    keys_at = clk_at = 0;
    key_byte = in_battle = 0;
    if (have_clock) {
        /* the number the original's setup started the random numbers with */
        long i;

        for (i = 0; i < clk_n && clk[i].kind != 'P'; i++)
            if (clk[i].kind == 'S')
                seed_set(clk[i].v);
    }
    stopped = NULL;
    /* the keys before the battle are the menu's */
    while (!have_clock && keys_at < keys_n && keys[keys_at].t < start)
        keys_at++;
}

/* the original's clock brought up to the next pass's beginning (the
 * timer's routine and the keys since the last place of the pass before),
 * so that memory written now is that of the pass's beginning */
void play_sync(void)
{
    long i;

    if (!have_clock)
        return;
    for (i = clk_at; i < clk_n && clk[i].kind != 'P'; i++)
        ;
    if (i < clk_n) {
        clock_run(i, 0);
        clk_at = i;
    }
}

/* one pass; NULL, or why the battle does not go on */
const char *play_pass(void)
{
    const char *why;

    if (have_clock) {
        long i;

        int parts = 0;

        for (i = clk_at; i < clk_n && parts < 2; i++)
            parts += clk[i].kind == 'P';
        if (parts < 2)
            return "the clock's file ends here";
    }
    /* S066A:0EE9: the count the pass's end waits by */
    fww(S_DATA, 0x0308, 0);
    why = loop_pass(in);
    if (!why)
        why = stopped;
    if (why)
        return why;
    if (!have_clock) {
        uint16_t n = NW(game_state, 0x0F);

        do
            own_tick();
        while (!stopped && frw(S_DATA, 0x0308) < n);
        if (stopped)
            return stopped;
    }
    play_passes++;
    if (inputs) {
        int s;

        for (s = 0; s < 2; s++)
            if (seen[s][1] < 0)
                fprintf(inputs, "%lu %d %X -\n", play_passes, s, seen[s][0]);
            else
                fprintf(inputs, "%lu %d %X %X %d %d\n", play_passes, s, seen[s][0], seen[s][1], seen[s][2], seen[s][3]);
        fflush(inputs);
    }
    return NULL;
}
