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
 *   least one here), and a round's end as long as the original's: the
 *   retraces and scan lines its waits and changes of the screen's colours
 *   count (read in HL.EXE), and for what it computes, loads and draws
 *   between them the times measured in runs of it on the runner (the
 *   T_ numbers below; they are that machine's, not the game's).  The
 *   waits that clear the button and the key do so here too, and the
 *   fight's pictures start the random numbers from the BIOS's count of
 *   ticks, which runs with the clock from a value given (play_bios).
 *   Everything else in a pass takes no time.
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
 *       TIME Y          S167A:2FEF, 30B2: inputs_release (event_picture's
 *                       beginning: a taken site's picture, the pictures
 *                       behind the loop) has cleared the key and both
 *                       records' buttons
 *       TIME E          S0CB6:0DE4, 0E30, 0E51: wait_press (S0CB6:0DA9, the
 *                       battle's end) has cleared the first record's
 *                       button and the key
 *       TIME L          S066A:45E8, the loop is left
 *       TIME B          S063F:0009, battle_score is called behind the loop
 *       TIME M          S066A:4728, what follows the loop is done
 *   The port's own keys, timer routine and passes are run in that order.
 *   S, R, W, X and Y count from attacks_run's call to its last such line in
 *   the side's part (the pictures and waits of a round's end, which the
 *   port leaves out; rand_next elsewhere is the port's own computer's).
 *   A place the port comes to that the log does not have in the side's
 *   part, or the other way round, is counted in play_misses.
 *
 * Between the two (play_seeds): the port's own clock, and of such a log
 * only what the fights' pictures did to the random numbers, fight by
 * fight in their order: the number clock_seed set (S) and the calls of
 * rand_next after it (R).  It shows what of a difference between the two
 * plays comes from the random numbers alone.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hl.h"
#include "frame.h"
#include "platform.h"

#define S_DATA (HL_DATA + HL_BASE)
#define TICK (0x3FFF / 1193182.0)       /* the timer routine's period, s */
#define BIOS_TICK (65536 / 1193182.0)   /* the BIOS's */
#define RETRACE 0.014268                /* the screen's vertical retraces */
#define PAL_LEVEL (RETRACE * 256 / 449) /* pal_level: 256 of a retrace's 449 scan lines */

/* The runner's times for what the original computes, loads and draws in a
 * round's end, seconds, measured in logged runs (docs/HANDOFF.md; m8_d5r
 * with and without fight pictures, m0_hq): not counted by the game. */
#define T_VIEW 0.59         /* the view drawn before the first fight's panel, a fight's after pictures, a taken site's wait */
#define T_NEXT 0.10         /* a later fight without pictures: to its panel */
#define T_PANEL 0.10        /* fight_panel before its first fade or mark */
#define T_MARK 0.017        /* a mark of the panel drawn (it misses one retrace) */
#define T_PIC_LOAD 0.533    /* fight_pictures up to the key's clearing */
#define T_PIC_FRAME 0.0182  /* a frame of fight_frames (749 in 13.58 and 13.71 s) */
#define T_PIC_LEVEL 0.0088  /* a step of the colours going down after them */
#define T_PIC_BACK 0.69     /* pictures_load and the view after a fight's pictures */
#define T_PICS_END 0.83     /* after the last fight with pictures, to game_loop's fade_in */
#define T_ROUND_END 0.31    /* attacks_run's end to the round's, without pictures or with a site */
#define T_SITE_FRAMES 1.84  /* site_picture's frames (their number is in its stream) */
#define T_SITE_BACK 3.04    /* after it: S2F39:1058 and the battle's pictures loaded again */
#define T_SETUP 2.75        /* the menu's return (S066A:0390) to the loop's first pass (back walk: 20.18 to 22.93 s) */

typedef struct { double t; uint8_t code[2]; int n; long order; } KeyEv;
typedef struct { double t; char kind; int a, b; uint32_t v; } ClockLine;

static KeyEv *keys;
static long keys_n, keys_at;
static int key_byte;            /* the next byte of keys[keys_at], by K lines */
static int by_k, in_battle;     /* the log has K lines; its first P is passed */
static int in_menu;             /* before a battle (play_start): the keys come by their times */
static ClockLine *clk;
static long clk_n, clk_at;
static int have_clock;
static FILE *inputs, *rounds, *ends;
typedef struct { uint32_t v; int r; } Fight;
static Fight *fights;           /* play_seeds: a fight's pictures' seed and calls of rand_next */
static long fights_n, fights_at;

static SideIn in[2];
static int seen[2][4];          /* for -inputs: key, word, bytes 5 and 8 (-1 none) */
static int thinks;              /* calls of computer_turn in the thinking loop */
static uint16_t aim_kept;       /* fight_rec's word 1Ch over attacks_run */
static double now, began;       /* the port's own clock, s */
static double part_at, round_at; /* when the part began, attacks_run was called */
static double tick_at;          /* when the timer's routine is called next */
static uint32_t bios0;          /* the BIOS's count when it began */
static const char *stopped;     /* what the timer's routine met */
static int live;                /* play_live: the window's time, keys and pictures */
static double pic_at;           /* live: when the next picture is shown, s */
static double shown_to;         /* live: the time screen_wait's waits came to, s */

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

/* the fights' pictures of a log of the original (the clock's file), for a
 * play on the port's own clock: the S lines after the first pass began,
 * each with the number of R lines up to the next S or the next side's
 * part */
const char *play_seeds(const char *path)
{
    FILE *f = fopen(path, "r");
    char line[100];
    int battle = 0, open = 0;

    if (!f)
        return "the seeds' file cannot be opened";
    while (fgets(line, sizeof line, f)) {
        unsigned long v = 0;
        double t;
        char kind;

        if (sscanf(line, "%lf %c", &t, &kind) != 2)
            continue;
        if (kind == 'P') {
            battle = 1;
            open = 0;
        } else if (kind == 'S' && battle && sscanf(line, "%*f %*c %lx", &v) == 1) {
            Fight *p;

            if (!(fights_n & 255)) {
                p = realloc(fights, (size_t)(fights_n + 256) * sizeof *fights);
                if (!p) {
                    fclose(f);
                    return "no memory for the seeds' file";
                }
                fights = p;
            }
            fights[fights_n].v = (uint32_t)v;
            fights[fights_n++].r = 0;
            open = 1;
        } else if (kind == 'R' && open)
            fights[fights_n - 1].r++;
    }
    fclose(f);
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

/* live: the player's keys.  Each byte the window hands over comes into
 * the keys as a key script's would, at the time of the picture it came
 * with (key_time, rounded as -keylog writes it), and reaches the keyboard's
 * interrupt when the clock gets there (keys_to, at a call of the timer's
 * routine).  The pictures are shown at the window's pace, at times also
 * ahead of the clock (screen_wait: fade_grey's 60h retraces are counted
 * after it), and a key then would otherwise reach the program before its
 * time, a tap's press and release at one call (and the tap lost).
 * -keylog writes them as a key script: played with -keys and the same
 * -start and -seed it is the window's play again. */
static FILE *keylog;
static int keylog_e0;
static double key_time;         /* live: the time of the picture being shown */

void play_keylog(const char *path)
{
    keylog = fopen(path, "w");
}

static void live_key(uint8_t code)
{
    double t = floor(key_time * 1e6 + 0.5) / 1e6;
    KeyEv *k;

    if (!(keys_n & 255)) {
        k = realloc(keys, (size_t)(keys_n + 256) * sizeof *keys);
        if (!k)
            return;
        keys = k;
    }
    k = &keys[keys_n];
    k->t = t;
    k->order = keys_n++;
    k->n = 1;
    k->code[0] = code;
    if (keylog) {
        uint16_t c = (uint16_t)((keylog_e0 ? 0xE000 : 0) | (code & 0x7F));
        int i;

        if (code == 0xE0)
            keylog_e0 = 1;
        else {
            for (i = 0; names[i].name && names[i].code != c; i++)
                ;
            if (names[i].name)
                fprintf(keylog, "%.6f %s%c\n", t, names[i].name, code & 0x80 ? '-' : '+');
            else
                fprintf(keylog, "%.6f %X%c\n", t, (unsigned)c, code & 0x80 ? '-' : '+');
            fflush(keylog);
            keylog_e0 = 0;
        }
    }
}

/* the mouse, as the keys: a script of -mice (the runner's format, "T X,Y,B"
 * a line: the mouse put at the driver's X, Y with the buttons B at time T)
 * is handed to the driver when the clock reaches T (mice_to, at a call of
 * the timer's routine); in the window each motion the platform reports
 * is kept with the time of the picture it came with and given to the
 * driver (mouse_motion) when the clock gets there, and -micelog writes
 * where that put the mouse as such a line (when it moved it or changed
 * its buttons), so that -mice and the runner's -mice play the window's
 * mouse again.  A motion's mickeys not yet a pixel are not in the line:
 * the driver keeps them, -mice does not (presumably never a difference
 * where the battle's pointer goes to the middle after each direction;
 * not checked). */
typedef struct { double t; int rel, x, y, b; } MouseEv;

static MouseEv *mice;
static long mice_n, mice_at;
static FILE *micelog;

void play_micelog(const char *path)
{
    micelog = fopen(path, "w");
}

static void mouse_add(double t, int rel, int x, int y, int b)
{
    MouseEv *m;

    if (!(mice_n & 255)) {
        m = realloc(mice, (size_t)(mice_n + 256) * sizeof *mice);
        if (!m)
            return;
        mice = m;
    }
    m = &mice[mice_n++];
    m->t = t;
    m->rel = rel;
    m->x = x;
    m->y = y;
    m->b = b;
}

const char *play_mice(const char *path)
{
    FILE *f = fopen(path, "r");
    char line[100];

    if (!f)
        return "the mouse's script cannot be opened";
    while (fgets(line, sizeof line, f)) {
        char *c = strchr(line, '#');
        double t;
        int x, y, b;

        if (c)
            *c = 0;
        if (sscanf(line, "%lf %d,%d,%d", &t, &x, &y, &b) == 4)
            mouse_add(t, 0, x, y, b);
    }
    fclose(f);
    return NULL;
}

static void mice_to(double t)
{
    for (; mice_at < mice_n && mice[mice_at].t <= t; mice_at++) {
        const MouseEv *m = &mice[mice_at];

        if (m->rel) {
            uint16_t x0, y0, x, y;
            int b0, b;

            mouse_where(&x0, &y0, &b0);
            mouse_motion(m->x, m->y, m->b);
            mouse_where(&x, &y, &b);
            /* a motion that left the driver as it was is none */
            if (micelog && (x != x0 || y != y0 || b != b0)) {
                fprintf(micelog, "%.6f %u,%u,%d\n", m->t, x, y, b);
                fflush(micelog);
            }
        } else
            mouse_set((uint16_t)m->x, (uint16_t)m->y, m->b);
    }
}

/* a file that gets a line for each round's end on the port's own clock:
 * the pass, when its part began, when attacks_run was called and when the
 * round's end was over (seconds of the key script's time) */
void play_rounds(const char *path)
{
    rounds = fopen(path, "w");
}

/* a file that gets a line for each time the battle's end shows its texts
 * and waits (state 7): the pass, the side whose part it is, the first
 * side's text and the second's (hex), and on the port's own clock when the
 * wait began and ended */
void play_ends(const char *path)
{
    ends = fopen(path, "w");
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

    if (!by_k || in_menu)
        keys_to(t);
    mice_to(t);
    why = input_tick();
    if (why && !stopped)
        stopped = why;
}

/* live: one picture shown at the window's pace (frame_wait, which hands
 * the player's keys to the keyboard's interrupt), and what the mouse did
 * meanwhile given to its driver, as the driver's interrupt would */
static void live_picture(void)
{
    int dx, dy, b;

    key_time = pic_at;
    if (!frame_wait() && !stopped)
        stopped = "the window was closed";
    if (plat_mouse_motion(&dx, &dy, &b))
        mouse_add(floor(key_time * 1e6 + 0.5) / 1e6, 1, dx, dy, b);
}

/* live: a wait of the screen's routines (screen_hook): a retrace_wait or
 * a pal_level.  The pictures due within it are shown now, so that a
 * fade's steps and what is drawn between retraces are seen; the clock's
 * own account of the wait (own_fade, own_retraces) then finds them shown
 * and does not wait again */
static void screen_wait(int kind)
{
    if (shown_to < now)
        shown_to = now;
    shown_to += kind == SCREEN_PAL_LEVEL ? PAL_LEVEL : RETRACE;
    for (; pic_at <= shown_to; pic_at += RETRACE)
        live_picture();
}

/* the port's own clock moved on to time t: the calls of the timer's
 * routine that are due by then, and the BIOS's count with them */
static void own_to(double t)
{
    while (tick_at <= t) {
        uint32_t b;

        /* live: the pictures due before the call, each at its time of
         * the window's clock (frame_wait: the player's keys go to the
         * keyboard's interrupt meanwhile) */
        for (; live && pic_at <= tick_at; pic_at += RETRACE)
            live_picture();

        now = tick_at;
        tick_at += TICK;
        b = bios0 + (uint32_t)((now - began) / BIOS_TICK);
        fww(0x40, 0x6C, (uint16_t)b);
        fww(0x40, 0x6E, (uint16_t)(b >> 16));
        tick(now);
        snd_clock(now);
    }
    now = t;
    snd_clock(now);
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

/* what the original computes or draws meanwhile */
static void own_busy(double s)
{
    own_to(now + s);
}

/* fade_in, fade_out (S28E0:0004, 002B): 64 calls of pal_level */
static void own_fade(void)
{
    own_to(now + 64 * PAL_LEVEL);
}

static int pressed(void)
{
    return pb(far_at(S_DATA, 0x00A6), 4) || frw(S_DATA, (uint16_t)(frw(S_DATA, 0x00BE) + 4));
}

static void press_clear(void)
{
    Far k = far_at(S_DATA, 0x00BE);

    pwb(far_at(S_DATA, 0x00A6), 4, 0);
    fww(k.seg, (uint16_t)(k.off + 4), 0);
}

/* wait_input(n), S0CB6:0036: while the first side's button is held or the
 * key's word is 1 both are cleared and three frames waited; then again,
 * n counted down by 3 until it is below 0, as long as neither comes */
static void own_wait_input(int n)
{
    Far k = far_at(S_DATA, 0x00BE);

    while (pb(far_at(S_DATA, 0x00A6), 4) || pw(k, 4) == 1) {
        press_clear();
        own_retraces(4);
    }
    press_clear();
    while (!pressed()) {
        press_clear();
        own_retraces(4);
        n -= 3;
        if (n < 0)
            break;
    }
}

/* wait_press, S0CB6:0DA9: as wait_input's beginning, then both cleared
 * and three frames waited until the first side's button or a key comes.
 * A key script that has no key left would wait for ever: the play stops */
static void own_wait_press(void)
{
    Far k = far_at(S_DATA, 0x00BE);
    int idle = 0;

    while (pb(far_at(S_DATA, 0x00A6), 4) || pw(k, 4) == 1) {
        press_clear();
        own_retraces(4);
    }
    press_clear();
    while (!pressed()) {
        if (!live && keys_at >= keys_n && mice_at >= mice_n && ++idle > 16) {
            if (!stopped)
                stopped = "the battle's end waits for a press the key script does not have";
            break;
        }
        press_clear();
        own_retraces(4);
    }
}

/* fight_pictures' times on the port's own clock (pics_hook): its seed
 * (clock_seed(0); with play_seeds the original's number instead; the
 * calls of rand_next are its own, volley_end), its loading, its frames
 * and its fade */
static void pics_clock(int at)
{
    static uint16_t kept[4];
    int i;

    switch (at) {
    case PICS_SEED:
        if (have_clock) {
            /* the log's S and R come after attacks_run and the frames
             * take no time here (no key ends them): rand_words are
             * given back at the end */
            for (i = 0; i < 4; i++)
                kept[i] = NW(rand_words, 2 * i);
            seed_set(clock_seed0());
        } else if (fights_at < fights_n)
            seed_set(fights[fights_at++].v);
        else {
            if (fights_n)
                play_misses++;
            seed_set(clock_seed0());
        }
        break;
    case PICS_LOADED:
        if (!have_clock)
            own_busy(T_PIC_LOAD);
        break;
    case PICS_FRAME:
        if (!have_clock)
            own_busy(T_PIC_FRAME);
        break;
    case PICS_LEVEL:
        if (!have_clock)
            own_busy(T_PIC_LEVEL);
        break;
    case PICS_END:
        if (have_clock)
            for (i = 0; i < 4; i++)
                SETW(rand_words, 2 * i, kept[i]);
        break;
    }
}

/* attacks_run's waits and pictures (orders_hook), on the port's own clock */
static void round_hook(int at, int n)
{
    static int first;
    int i;

    if (have_clock)
        return;
    switch (at) {
    case ORDERS_FIGHT:
        own_busy(first || n ? T_VIEW : T_NEXT);
        first = 0;
        /* fight_panel, S131D:178D */
        own_busy(T_PANEL);
        if (n)
            own_fade();
        for (i = 0; i < 6; i++) {
            own_busy(T_MARK);
            own_retraces(7);
        }
        own_retraces(13);
        if (n) {
            own_wait_input(0x96);
            own_fade();
            /* then fight_pictures, through pics_hook */
        } else {
            for (i = 0; i < 6; i++) {
                own_busy(T_MARK);
                own_retraces(7);
            }
            own_wait_input(0x46);
            own_retraces(31);       /* S02E6:0AE6, wait_frames(1Eh) */
        }
        break;
    case ORDERS_BLAST:
        own_retraces(3);            /* S02E6:0D24, wait_frames(2) */
        break;
    case ORDERS_FOUGHT:
        if (n)
            own_busy(T_PIC_BACK);
        else
            own_wait_input(0x50);   /* S02E6:0D3C */
        break;
    case ORDERS_TAKEN:
        /* order 4, S02E6:1233 to 17C3 */
        if (first)
            own_busy(T_VIEW);
        first = 0;
        own_retraces(31);
        if (!NB(no_fight_pics, 0)) {
            if (n)
                own_fade();
            else
                own_retraces(0x60);     /* fade_back */
        } else if (!n)
            own_retraces(0x60);
        /* event_picture: inputs_release(1, 0), S167A:2F83 */
        press_clear();
        pwb(far_at(S_DATA, 0x00B6), 4, 0);
        own_retraces(6);
        do {
            i = pressed() || pb(far_at(S_DATA, 0x00B6), 4);
            press_clear();
            pwb(far_at(S_DATA, 0x00B6), 4, 0);
            own_retraces(6);
        } while (i);
        /* site_picture, S1A39:1906 */
        own_busy(T_SITE_FRAMES);
        own_retraces(201);
        own_busy(T_SITE_BACK);
        own_retraces(51);           /* S02E6:17B4, wait_frames(32h) */
        break;
    case -1:
        first = 1;
        break;
    default:
        break;
    }
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
        case 'Y':
            pwb(far_at(S_DATA, 0x00B6), 4, 0);
            pwb(far_at(S_DATA, 0x00A6), 4, 0);
            fww(k.seg, (uint16_t)(k.off + 4), 0);
            break;
        case 'E':
            pwb(far_at(S_DATA, 0x00A6), 4, 0);
            fww(k.seg, (uint16_t)(k.off + 4), 0);
            break;
        case 'L':
        case 'B':
        case 'M':
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
        part_at = now;
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
        round_hook(-1, 0);
        if (have_clock)
            clock_to('A', 0, 0);
        else if (!NW(site_taken, 0))
            /* S0E56:21D8, the screen fades to grey: 20h and 40h steps, a
             * retrace each */
            own_retraces(0x60);
        else if (!NB(no_fight_pics, 0))
            own_fade();     /* S066A:3D17, fade_out */
        round_at = now;
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
                if (strchr("SRWXY", clk[i].kind)) {
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
            if (!NW(site_taken, 0))
                /* S0E56:1F91, the colours come back: 40h and 20h steps */
                own_retraces(0x60);
            else if (pictures) {
                own_busy(T_PICS_END);
                if (!NB(take_picture, 0))
                    own_fade();     /* S066A:3EA9, fade_in */
            } else
                own_busy(T_ROUND_END);
            if (rounds) {
                fprintf(rounds, "%lu %.4f %.4f %.4f\n", play_passes + 1, part_at, round_at, now);
                fflush(rounds);
            }
        }
        break;
    }
    case LOOP_QUIT: {
        /* the port's own clock only: the original's log has no lines
         * for this wait */
        static int idle;

        idle = keys_at >= keys_n && mice_at >= mice_n ? idle + 1 : 0;
        if (have_clock || idle > 400)
            loop_abort = 1;
        else
            own_tick();
        break;
    }
    case LOOP_SAVE: {
        /* save_game waits for its key: on the original's clock the log's
         * next call of the timer's routine or key, a line a turn (the
         * wait has no line of its own); on the port's own a call of the
         * timer's routine */
        static int idle;

        if (have_clock) {
            if (clk_at < clk_n && (clk[clk_at].kind == 'T' || clk[clk_at].kind == 'K')) {
                clock_run(clk_at + 1, 0);
                clk_at++;
            } else
                loop_abort = 1;
        } else {
            idle = keys_at >= keys_n && mice_at >= mice_n ? idle + 1 : 0;
            if (idle > 400)
                loop_abort = 1;
            else
                own_tick();
        }
        break;
    }
    case LOOP_END: {
        double t = now;

        if (have_clock) {
            /* wait_press's clearings, in the order of the original's
             * timer's routine and keys; the press that ends the wait
             * comes after the last */
            long i, last = -1;

            for (i = clk_at; i < clk_n && clk[i].kind != 'P' && clk[i].kind != 'L'; i++)
                if (clk[i].kind == 'E')
                    last = i;
            if (last >= 0) {
                clock_run(last + 1, 0);
                clk_at = last + 1;
            } else
                play_misses++;
        } else
            own_wait_press();
        if (ends) {
            fprintf(ends, "%lu %u %X %X", play_passes + 1, (unsigned)side, (unsigned)loop_end_texts[0],
                    (unsigned)loop_end_texts[1]);
            if (!have_clock)
                fprintf(ends, " %.4f %.4f", t, now);
            fputc('\n', ends);
            fflush(ends);
        }
        break;
    }
    default:
        break;
    }
}

/* the play in the window, at the original's pace (the own clock's time
 * is the window's: frame.h), with the player's keys instead of a key
 * script's (live_key: at the times of the pictures they came with), the
 * picture the
 * port's video memory and DAC hold shown at the screen's refresh rate,
 * also at the waits of the screen's routines (screen_hook), and the
 * player's mouse to the mouse's driver (mouse_motion; the window keeps
 * the mouse, plat_mouse_grab) */
void play_live(void)
{
    live = 1;
    frame_set_keyboard(live_key);
    screen_hook = screen_wait;
    plat_mouse_grab(1);
}

/* after the port's setup (battle_begin): the first pass begins at `start`
 * seconds of the key script's time (the port's own clock; the original's
 * has its own times) */
void play_begin(double start)
{
    loop_hook = hook;
    orders_hook = round_hook;
    pics_hook = pics_clock;
    loop_reset();
    play_passes = play_misses = 0;
    clk_at = fights_at = 0;
    key_byte = in_battle = in_menu = 0;
    if (start < 0) {
        /* the clock goes on (play_start, the menu before); with the
         * original's clock for the battle the number its setup started
         * the random numbers with, as below */
        long i, calls = 0;

        for (i = 0; have_clock && i < clk_n && clk[i].kind != 'P'; i++)
            if (clk[i].kind == 'S')
                seed_set(clk[i].v);
            else if (clk[i].kind == 'T')
                calls++;
        /* and which of its four calls the timer's routine is at, which
         * fills the sides' records on every fourth (DATA:0B19): the
         * original's, not the one the port's menu left */
        if (have_clock) {
            fww(S_DATA, 0x0B19, (uint16_t)(calls & 3));
            clk_at = i;
        }
        return;
    }
    now = began = pic_at = start;
    tick_at = start + TICK;
    keys_at = 0;
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

/* the port's own clock, seconds of the key script's time */
double play_now(void)
{
    return now;
}

/* the port's own clock for what comes before a battle (the menu, menu.c):
 * it begins at `start` seconds of the key script's time, the keys before
 * that are passed over; play_begin with a start below 0 then goes on with
 * it.  play_tick is one call of the timer's routine, play_retraces n
 * waits for a retrace, play_fade a fade_in or fade_out, play_idle whether
 * the key script has no key left.  With a clock's file given too the menu
 * still runs on the port's own clock, its keys by their times, and the
 * battle behind it on the original's: the log's keys before its first
 * pass are the menu's and passed over, the key script's that are left are
 * the battle's. */
void play_start(double start)
{
    in_menu = 1;
    now = began = start;
    tick_at = start + TICK;
    keys_at = 0;
    stopped = NULL;
    while (keys_at < keys_n && keys[keys_at].t < start)
        keys_at++;
}

void play_tick(void)
{
    own_tick();
}

void play_retraces(int n)
{
    own_retraces(n);
}

void play_fade(void)
{
    own_fade();
}

/* the time the original's battle setup takes after the menu (the timer's
 * routine goes on meanwhile; the mouse put to the middle by it is gone
 * when the setup clears the sides' records) */
void play_setup(void)
{
    own_busy(T_SETUP);
}

int play_idle(void)
{
    return live ? stopped != NULL : keys_at >= keys_n && mice_at >= mice_n;
}

/* why the play cannot go on (the window was closed, the timer's routine
 * met what is not translated), or NULL */
const char *play_stopped(void)
{
    return stopped;
}

/* the original's clock brought up to the next pass's beginning, or to
 * where the loop is left (the timer's routine and the keys since the last
 * place of the pass before), so that memory written now is that of the
 * pass's beginning */
void play_sync(void)
{
    long i;

    if (!have_clock)
        return;
    for (i = clk_at; i < clk_n && clk[i].kind != 'P' && clk[i].kind != 'L'; i++)
        ;
    if (i < clk_n) {
        clock_run(i, 0);
        clk_at = i;
    }
}

static void after_hook(uint16_t picture, uint16_t arg)
{
    if (ends) {
        fprintf(ends, "after %X %X\n", (unsigned)picture, (unsigned)arg);
        fflush(ends);
    }
}

/* what follows the loop once the battle is over (battle_after); its
 * pictures go into the ends' file as "after KIND ARGUMENT".  On the
 * original's clock the log's lines up to battle_score's call come first,
 * with what the picture before it did to the random numbers, and the
 * rest up to S066A:4728 after (the calls of rand_next there are
 * battle_score's own).  The pictures take no time here. */
uint16_t play_after(void)
{
    uint16_t r;
    long i;

    battle_hook = after_hook;
    if (have_clock) {
        for (i = clk_at; i < clk_n && clk[i].kind != 'B' && clk[i].kind != 'M'; i++)
            ;
        if (i < clk_n) {
            clock_run(i, 1);
            clk_at = i;
        } else
            play_misses++;
    }
    r = battle_after();
    if (have_clock) {
        for (i = clk_at; i < clk_n && clk[i].kind != 'M'; i++)
            ;
        if (i < clk_n) {
            clock_run(i, 0);
            clk_at = i;
        } else
            play_misses++;
    }
    return r;
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
