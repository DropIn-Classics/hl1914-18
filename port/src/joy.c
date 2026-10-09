/* joy.c - HL.EXE's routines for the joysticks (S2B9B and S2BAD), which
 * keep the words at DATA:0384 to 03B7 through the game port (201h),
 * translated over the program's memory.  input.c's timer routine reads
 * the sticks by joy_read and turns what it measured into a side's
 * directions and button.
 *
 * The game port is not the game's: a write starts its four one-shots,
 * whose bits (0 and 1 the first stick's X and Y, 2 and 3 the second's)
 * read 1 until each has run out, the longer the further right or down
 * the stick is; bits 4 to 7 are the buttons, 0 while held.  The game
 * measures an axis by counting how often it can read the port until the
 * bit falls, so what it finds depends on how fast the machine is.  Here
 * the machine is the runner's (doskit/tools/run: every instruction a
 * sixth of a microsecond, the default of its -ips), since that is what
 * the runs of the original this is compared with had: the port's clock
 * (port_clock) counts the original's instructions from the write on, as
 * they are read in HL.EXE.  A stick is given as the runner's -joy gives
 * it: the microseconds its one-shots stay high (joy_stick), from a script
 * (joy_script, the runner's -joys format).  No stick is there without
 * one: the window's program has none yet (a controller is keys there,
 * doskit's pad.h).  port/README.md says how this was compared.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hl.h"

#define S_DATA (HL_DATA + HL_BASE)
#define IPUS 6                  /* the machine's instructions a microsecond */
#define RETRACE_US 14268        /* a vertical retrace of the screen */

static uint16_t dw(uint16_t o) { return frw(S_DATA, o); }
static void sw(uint16_t o, uint16_t v) { fww(S_DATA, o, v); }

/* the game port */
static struct {
    long axis[4];       /* a one-shot's time in instructions, below 0 without its stick */
    uint8_t buttons;    /* bits 0 to 3, set while held */
    long clock;         /* instructions since the write, below 0 before the first */
} port = { { -1, -1, -1, -1 }, 0, -1 };

void joy_stick(int stick, int x_us, int y_us, int buttons)
{
    stick = stick ? 2 : 0;
    port.axis[stick] = x_us < 0 ? -1 : (long)x_us * IPUS;
    port.axis[stick + 1] = x_us < 0 ? -1 : (long)y_us * IPUS;
    port.buttons = (uint8_t)((port.buttons & ~(3 << stick)) | (x_us < 0 ? 0 : (buttons & 3) << stick));
}

static void port_write(void)
{
    port.clock = 0;
}

/* the port read `n` instructions after the read or write before */
static uint8_t port_read(long n)
{
    uint8_t r = (uint8_t)(~port.buttons << 4);
    int i;

    if (port.clock >= 0)
        port.clock += n;
    for (i = 0; i < 4; i++)
        if (port.axis[i] < 0 || (port.clock >= 0 && port.clock < port.axis[i]))
            r |= (uint8_t)(1 << i);
    return r;
}

/* S2BAD:0006 (joy_read): the sticks' buttons and axes.  The port is
 * written, then read: a button of the first stick held (bits 4, 5 clear)
 * gives FFFFh in DATA:038C, else 0; the second's (bits 6, 7) at a second
 * read in 0392.  Then, with interrupts off, the port is read up to as
 * many times as the word at 03B6 says (the largest range; 10000h times
 * when that is 0) until every axis of the sticks that are there (the
 * bits of the byte at 03B4) has fallen; one axis is taken a read, the
 * lowest first, and such a read is not counted.  An axis's word is the
 * count at which it fell (03B6 for one that did not, or of a stick not
 * there): the first stick's X and Y at 0388 and 038A, the second's at
 * 038E and 0390.
 * The numbers passed to port_read are the instructions the original
 * runs between its reads. */
void joy_read(void)
{
    uint16_t max = dw(0x03B6), cx = max, at[4] = { 0, 0, 0, 0 };
    uint8_t ah, al;
    long n;

    port_write();
    al = port_read(7);
    sw(0x038C, ~al & 0x30 ? 0xFFFF : 0);
    n = ~al & 0x30 ? 5 : 6;
    al = port_read(n);
    sw(0x0392, ~al & 0xC0 ? 0xFFFF : 0);
    n = ~al & 0xC0 ? 4 : 5;
    ah = frb(S_DATA, 0x03B4);
    if (ah) {
        ah = (uint8_t)~ah;
        n += 5;
        for (;;) {
            al = (uint8_t)((port_read(n) | ah) ^ 0xFF);
            if (!al) {
                n = 5;
                if (!--cx)
                    break;
                continue;
            }
            if (al & 1) {
                at[0] = cx;
                ah |= 1;
                n = 10;
            } else if (al & 2) {
                at[1] = cx;
                ah |= 2;
                n = 12;
            } else if (al & 4) {
                at[2] = cx;
                ah |= 4;
                n = 14;
            } else {
                at[3] = cx;
                ah |= 8;
                n = 14;
            }
            if (ah == 0xFF)
                break;
        }
    }
    sw(0x0390, (uint16_t)(max - at[3]));
    sw(0x038E, (uint16_t)(max - at[2]));
    sw(0x0388, (uint16_t)(max - at[0]));
    sw(0x038A, (uint16_t)(max - at[1]));
}

/* a stick's middle `o` (two words, X and Y, as joy_read left them) to
 * its range at `range` (twice the middle) and to the four words at
 * `edge`: left of the first and right of the second is a direction
 * across, the third and fourth the same up and down (the middle less and
 * plus half of it) */
static void stick_ranges(uint16_t o, uint16_t range, uint16_t edge)
{
    int i;

    sw(range, (uint16_t)(dw(o) << 1));
    sw((uint16_t)(range + 2), (uint16_t)(dw((uint16_t)(o + 2)) << 1));
    for (i = 0; i < 2; i++) {
        uint16_t v = dw((uint16_t)(o + 2 * i)), half = v >> 1, lo = (uint16_t)(v - half);

        sw((uint16_t)(edge + 4 * i), lo);
        sw((uint16_t)(edge + 4 * i + 2), (uint16_t)(lo + half + half));
    }
}

/* S2B9B:000A (joy_setup; joy_measure here as long as loop.c has its own
 * joy_setup, the one for a machine without a game port): the joysticks
 * looked for and measured anew.
 * The words that say which are there (DATA:0384, 0386), their bits
 * (03B4) and their ranges are cleared and the largest range (03B6) made
 * FFFFh; the game port is written and read after two retraces: a stick
 * whose two axes' bits are 0 then is there (FFFFh in its word, its bits
 * into 03B4), is measured where it stands (joy_read) and gets its range
 * and edges from that as its middle (stick_ranges).  The second stick is
 * measured after two more retraces, and the first with it once more.
 * 03B6 becomes the largest of the four ranges, 0 without a stick.
 * Called at the program's start (input_install) and by F5 and F6.
 *
 * The timer's routine goes on during the retraces, and reads the sticks
 * with what stands in these words then: a second stick already said to
 * be there, with the edges of the time before and 03B6 FFFFh, gives its
 * side a direction if it is held away from its old middle (seen in the
 * original, m48_joy at 59 s).  So the routine is in three steps here,
 * setup_step going on by the clock at the calls of the timer's routine
 * (joy_tick): `wait` 0 does it all at once. */
static int setup_stage;         /* 1 the first two retraces, 2 the second stick's */
static double setup_due;
static uint8_t setup_al;

static void setup_end(void)
{
    static const uint16_t ranges[] = { 0x039A, 0x03A8, 0x03AA };
    uint16_t max = dw(0x0398);
    unsigned i;

    for (i = 0; i < sizeof ranges / sizeof ranges[0]; i++)
        if (max < dw(ranges[i]))
            max = dw(ranges[i]);
    sw(0x03B6, max);
    setup_stage = 0;
}

static void setup_step(void)
{
    if (setup_stage == 1) {
        setup_al = port_read(2L * RETRACE_US * IPUS);
        if (!(setup_al & 3)) {
            sw(0x0384, 0xFFFF);
            fwb(S_DATA, 0x03B4, frb(S_DATA, 0x03B4) | 3);
            joy_read();
            stick_ranges(0x0388, 0x0398, 0x039C);
        }
        if (setup_al & 0x0C) {
            setup_end();
            return;
        }
        sw(0x0386, 0xFFFF);
        fwb(S_DATA, 0x03B4, frb(S_DATA, 0x03B4) | 0x0C);
        setup_stage = 2;
        setup_due += 2 * RETRACE_US * 1e-6;
    } else {
        joy_read();
        stick_ranges(0x038E, 0x03A8, 0x03AC);
        setup_end();
    }
}

void joy_measure(int wait, double now)
{
    static const uint16_t cleared[] = { 0x0384, 0x0386, 0x0394, 0x0396, 0x03A4, 0x03A6, 0x0398, 0x039A, 0x03A8, 0x03AA };
    unsigned i;

    for (i = 0; i < sizeof cleared / sizeof cleared[0]; i++)
        sw(cleared[i], 0);
    fwb(S_DATA, 0x03B4, 0);
    sw(0x03B6, 0xFFFF);
    port_write();
    setup_stage = 1;
    /* the key was taken up to a call before this one and the first wait
     * lasts one to two retraces: a retrace from now is the middle */
    setup_due = now + RETRACE_US * 1e-6;
    while (!wait && setup_stage)
        setup_step();
}

/* The sticks' script, "T N,X,Y,B" a line (stick N, 0 or 1, from T
 * seconds on with one-shots of X and Y microseconds and the buttons B;
 * "T N,off" takes it away; # begins a comment), the runner's -joys.  A
 * line of time 0 holds from the program's start.  The others come by the
 * calls of the timer's routine (joy_tick, from input_tick): on the
 * port's own clock by its time (the menu's too when a battle behind it
 * is played on the original's); with the original's clock by the times
 * of that log's calls, which the port's calls in a battle are one for
 * one (play.c; joy_clock reads them, joy_battle says the battle's first
 * is next: the log's first, or after a menu on the port's own clock the
 * first behind the log's first pass). */
typedef struct { double t; int stick, x, y, b; } JoyEv;

static JoyEv *evs;
static long evs_n, evs_at;
static double *calls;
static long calls_n, calls_pass = -1, calls_at = -1;

const char *joy_script(const char *path)
{
    FILE *f = fopen(path, "r");
    char line[100];

    if (!f)
        return "the joysticks' script cannot be opened";
    while (fgets(line, sizeof line, f)) {
        char *c = strchr(line, '#'), word[8];
        JoyEv e;

        if (c)
            *c = 0;
        if (sscanf(line, "%lf %d,%d,%d,%d", &e.t, &e.stick, &e.x, &e.y, &e.b) != 5) {
            if (sscanf(line, "%lf %d,%7s", &e.t, &e.stick, word) != 3 || strcmp(word, "off"))
                continue;
            e.x = e.y = -1;
            e.b = 0;
        }
        if (!(evs_n & 255)) {
            JoyEv *m = realloc(evs, (size_t)(evs_n + 256) * sizeof *evs);

            if (!m)
                break;
            evs = m;
        }
        evs[evs_n++] = e;
    }
    fclose(f);
    for (; evs_at < evs_n && evs[evs_at].t <= 0; evs_at++)
        joy_stick(evs[evs_at].stick, evs[evs_at].x, evs[evs_at].y, evs[evs_at].b);
    return NULL;
}

const char *joy_clock(const char *path)
{
    FILE *f = fopen(path, "r");
    char line[100], kind;
    double t;

    if (!f)
        return "the clock's file cannot be opened";
    while (fgets(line, sizeof line, f)) {
        if (sscanf(line, "%lf %c", &t, &kind) != 2)
            continue;
        if (kind == 'P' && calls_pass < 0)
            calls_pass = calls_n;
        if (kind != 'T')
            continue;
        if (!(calls_n & 4095)) {
            double *m = realloc(calls, (size_t)(calls_n + 4096) * sizeof *calls);

            if (!m)
                break;
            calls = m;
        }
        calls[calls_n++] = t;
    }
    fclose(f);
    return NULL;
}

void joy_battle(int menu)
{
    if (calls)
        calls_at = menu && calls_pass >= 0 ? calls_pass : 0;
}

/* at a call of the timer's routine: the script's lines due, then what
 * of a joy_measure under way is due.  Returns the clock's time. */
double joy_tick(void)
{
    static double t;

    if (!calls || calls_at < 0)
        t = play_now();
    else if (calls_at < calls_n)
        t = calls[calls_at++];
    else
        t += 0x3FFF / 1193182.0;
    for (; evs_at < evs_n && evs[evs_at].t <= t; evs_at++)
        joy_stick(evs[evs_at].stick, evs[evs_at].x, evs[evs_at].y, evs[evs_at].b);
    while (setup_stage && setup_due <= t)
        setup_step();
    return t;
}
