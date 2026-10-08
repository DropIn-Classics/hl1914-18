/* sound.c - the sound's routines the game calls (S2F39) and of its driver
 * (S30EF) what they reach, translated over the program's memory: the
 * start, an effect played, stopped and started in a pass, and the timer's
 * routine that reads the AdLib channels' events.  src/HL.hints has the
 * records and the events' meaning.
 *
 * No sound comes out.  What the original sends to the AdLib (ports 388h
 * and 389h) goes to snd_opl when it is set, a register and its value in
 * the original's order, without the waits between them: the probe's and
 * the silencing at the start and the end (adl_detect, adl_remove), the
 * voices' registers from the tune's and the effects' events at each tick
 * of the timer, and what the routines the game calls write themselves.
 * port/tests/opl_test.sh compares that stream with the original's, write
 * for write.  The Sound
 * Blaster's part is the driver's memory only: dig_mix makes the bytes the
 * card would play in the driver's two buffers, moves the digital channels
 * on, begins one again that is played more than once and takes one off at
 * its end; dig_kick (from snd_pass) and dig_irq (the card's interrupt)
 * call it as the original does.  Nothing is sent to a card: the bytes
 * stay in the buffers, and the card's interrupt, which the original gets
 * when a buffer has been played, is made by snd_clock that long after a
 * buffer was started (its length at the rate dsp_rate set), on the port's
 * own clock only.
 *
 * The music: music_start, music_fade, music_volume and the fade's timer
 * routine fade_tick, with the tune's events read by the timer's routine.
 * The timer itself (S27FE: pit_irq and its tables of routines) is not
 * translated over the memory: snd_irq is one interrupt of it as far as
 * the sound goes, with pit_irq's count for the sound's two routines kept
 * here (the fade's routine when its count has run out, then adl_tick),
 * and snd_clock calls it at the period the tune set.  music_fade's wait for
 * the fade's end runs those interrupts itself and tells snd_waited how
 * long that was.
 *
 * The audio's clock: when snd_samples is set (main.c: an audio device is
 * there), snd_clock makes the timer's interrupts by the samples played
 * instead of by the port's clock, and every register handed to snd_opl
 * has its sample in snd_stamp (below).  The card's interrupt stays on the
 * port's clock.
 *
 * The port is the game with both devices found (snd_devices 3).
 */
#include <stdio.h>
#include "hl.h"

#define DRV (HL_S30EF + HL_BASE)
#define D(o) frw(DRV, (uint16_t)(o))
#define DSET(o, v) fww(DRV, (uint16_t)(o), (uint16_t)(v))
#define DB(o) frb(DRV, (uint16_t)(o))
#define DSETB(o, v) fwb(DRV, (uint16_t)(o), (uint8_t)(v))

/* a channel's record and its fields (from the record's beginning) */
#define REC(ch) (8 + (ch) * 0x2A)
#define R_TIMES 0x04
#define R_START 0x06
#define R_LEN 0x0A
#define R_KIND 0x0C
#define R_RATE 0x0E
#define R_AT 0x10
#define R_HALF 0x14
#define R_18 0x18
#define R_1A 0x1A
#define R_VOL 0x1C
#define R_DUE 0x1E
#define R_VOICES 0x22
#define R_FIRST 0x24
#define R_STEP 0x26
#define R_FLAGS 0x28

/* a voice's record (14h bytes) in the three copies */
#define VOICE 0x026C
#define KEPT 0x0320
#define V_VOL 0x0C
#define V_OWN_VOL 0x0E
#define V_CHANNEL 0x10
#define V_NOTE 0x12

#define OPS 0x10E9          /* the voices' operator numbers */
#define MUSIC 0x0C          /* the music's channel */

void (*snd_opl)(uint8_t reg, uint8_t value);
/* a test's: each sfx_play, with its volume, how many times and its result */
void (*snd_played)(uint16_t ch, uint16_t n, uint16_t vol, uint16_t times, int16_t r);
/* a test's: each music_start (out 0; a the fade, b the volume) and
 * music_fade (out 1; a the speed) at its end, and music_fade where it
 * begins (out 2; a the speed) */
void (*snd_music)(int out, uint16_t a, uint16_t b, int16_t r);

static double tick_at = -1;

/* The audio's clock.  snd_samples: the samples played so far at snd_rate
 * a second; with the port's clock's time when snd_clock asks (a device
 * without a clock of its own is moved on to it), NULL when only the count
 * is wanted.  Unset, the interrupts run on the port's clock as before.
 * An interrupt is due at the sample the timer's periods since the clock
 * began come to (PIT counts * snd_rate / 1193182, counted whole so that
 * the fraction is carried), and snd_clock runs those due by the samples
 * played.  snd_stamp is the sample a register handed to snd_opl belongs
 * to: the interrupt's own for what it writes (snd_stamp_irq 1), else the
 * count of samples snd_clock saw last, or the last interrupt's when
 * music_fade ran the interrupts ahead of it.  Whoever plays adds a lead.
 * snd_irq_at (a test's): behind each interrupt made so, its sample and
 * the timer's period to the next. */
uint64_t (*snd_samples)(const double *t);
void (*snd_irq_at)(uint64_t sample, uint32_t next);
int snd_rate = 44100;
uint64_t snd_stamp;
int snd_stamp_irq;
static int irq_begun;           /* irq_base and irq_pit count */
static uint64_t irq_base;       /* the samples played when the clock began */
static uint64_t irq_pit;        /* the timer's counts from there to the next interrupt */
static uint64_t irq_last;       /* the last interrupt's sample */

/* the card: the rate dsp_rate set (bytes a second), the clock's time
 * snd_clock was last called with, and when the buffer started last has
 * been played (below 0: none is being played) */
static uint16_t dig_rate = 8000;
static double clock_now = -1, dig_at = -1;

/* the period the timer calls adl_tick with: pit_add's 1843h until a tune
 * or an effect sets its own (pit_period), which is kept at adl_period */
static uint16_t tick_period(void)
{
    return NW(adl_period, 0) ? NW(adl_period, 0) : 0x1843;
}

/* the fade: its count of steps so far, how many it has, the volume it
 * began with and the one it ends with; the period of its timer routine */
#define FADE_COUNT 0x023A
#define FADE_STEPS 0x023C
#define FADE_FROM 0x023E
#define FADE_TO 0x0240
#define FADE_PERIOD 0x1D1EBL

/* how long music_fade waited for its fade, in seconds (the caller's
 * clock goes on by it); NULL: nobody counts it */
void (*snd_waited)(double s);

/* a test's: called where it matters on which side of an instruction the
 * timer's interrupt came in the original: 2 before fade_start's pit_add
 * has given the fade's routine to the timer (the instruction that makes
 * it the table's last, S27FE:07DF; an interrupt up to there does not
 * count for the fade), 3 before music_start copies the count of ticks */
void (*snd_at)(int place);

/* music_fade's wait when something else than this file makes the timer's
 * interrupts (an audio clock; the test's replay of the original's): it
 * comes back when one more has been run, or with 0 when it has none, and
 * music_fade then runs them itself as without it */
int (*snd_wait)(void);

/* The timer's counts for the sound's two routines, as pit_irq keeps them
 * for each routine it has (S27FE:06D2; the periods at DATA:0D1F, what is
 * left at DATA:0D5F): at each interrupt it takes off what is left the
 * period the timer was set to one interrupt before (DATA:0E3F, here
 * pit_ran: the period is noted before the routines run, so one a routine
 * changes is counted from the interrupt after the next), calls a routine
 * whose count is at or below 0 and adds its period again.  adl_tick's
 * count stays below 0 so (FFFFF087h after the start, FFFFDEEDh with the
 * menu's tune, seen in the original's memory), and it is called at every
 * interrupt.  The timer runs with the shortest period of its routines:
 * the BIOS's own (10000h), input_tick's (3FFFh; input.c, on play.c's own
 * clock in the port) and the sound's. */
#define INPUT_PERIOD 0x3FFFu
static int32_t adl_left, fade_left;
static uint32_t pit_ran = INPUT_PERIOD;

/* the period the timer is set to (DATA:0E47) */
static uint32_t pit_least(void)
{
    return tick_period() < INPUT_PERIOD ? tick_period() : INPUT_PERIOD;
}

/* a test's: the three above */
void snd_timer(int32_t *adl, int32_t *fade, uint32_t *ran)
{
    *adl = adl_left;
    *fade = fade_left;
    *ran = pit_ran;
}

static void adl_voices_free(uint16_t ch);

/* S30EF:11E6 (opl_write), without its waits */
static void opl(uint8_t reg, uint8_t value)
{
    if (snd_opl)
        snd_opl(reg, value);
}

static uint16_t used(void) { return NW(adl_used, 0); }

/* the music's voice `v` is an effect's for now: what the three routines
 * below test, with the least channel that counts as the music's */
static int taken(uint8_t v, int music_only)
{
    uint16_t ch = NW(snd_channel, 0);

    if (music_only ? ch != MUSIC : ch < MUSIC)
        return 0;
    return (used() & 0x0F) && v >= (uint8_t)(9 - (uint8_t)used());
}

/* S30EF:1205 (voice_level) */
static void voice_level(uint16_t voice, uint16_t level)
{
    uint8_t v = (uint8_t)voice, l = (uint8_t)level, own, op, reg;
    uint16_t at;

    if (v >= 9)
        return;
    at = (uint16_t)(v * 0x14);
    if ((NW(music_on, 0) & 1) && taken(v, 0)) {
        DSETB(KEPT + V_VOL + at, l);
        return;
    }
    DSETB(VOICE + V_VOL + at, l);
    own = DB(VOICE + V_OWN_VOL + at);
    op = DB(OPS + v);
    reg = DB(VOICE + 6 + at);
    opl((uint8_t)(op + 0x43),
        (uint8_t)((0x3F - (uint8_t)((uint8_t)((0x3F - (reg & 0x3F)) * own >> 8) * l >> 8)) | (reg & 0xC0)));
    if (D(VOICE + 0x0A + at) & 1) {
        reg = DB(VOICE + 1 + at);
        opl((uint8_t)(op + 0x40),
            (uint8_t)((0x3F - (uint8_t)((uint8_t)((0x3F - (reg & 0x3F)) * own >> 8) * l >> 8)) | (reg & 0xC0)));
    }
}

/* S30EF:14A5 (voice_off) */
static void voice_off(uint16_t voice)
{
    uint8_t v = (uint8_t)voice, b;

    if (v >= 9)
        return;
    b = DB(VOICE + V_NOTE + 1 + v * 0x14) & 0xDF;
    DSETB(VOICE + V_NOTE + 1 + v * 0x14, b);
    opl((uint8_t)(0xB0 + v), b);
}

/* S30EF:14F7 (voice_note) */
static void voice_note(uint16_t voice, uint16_t note)
{
    uint8_t v = (uint8_t)voice;

    if (v >= 9)
        return;
    DSET(VOICE + V_NOTE + v * 0x14, note);
    opl((uint8_t)(0xA0 + v), (uint8_t)note);
    opl((uint8_t)(0xB0 + v), (uint8_t)(note >> 8));
}

/* S30EF:136B (voice_instrument): 0Ch bytes at `p` */
static void voice_instrument(uint16_t voice, Far p)
{
    uint8_t v = (uint8_t)voice, op, b[12];
    uint16_t at, i;

    if (v >= 9)
        return;
    at = (uint16_t)(v * 0x14);
    for (i = 0; i < 12; i++)
        b[i] = pb(p, i);
    if (taken(v, 1)) {
        for (i = 0; i < 12; i++)
            DSETB(KEPT + at + i, b[i]);
        return;
    }
    for (i = 0; i < 12; i++)
        DSETB(VOICE + at + i, b[i]);
    op = DB(OPS + v);
    opl((uint8_t)(op + 0x20), b[0]);
    opl((uint8_t)(op + 0x40), (uint8_t)(b[10] & 1 ? b[1] | 0x3F : b[1]));
    opl((uint8_t)(op + 0x60), b[2]);
    opl((uint8_t)(op + 0x80), b[3]);
    opl((uint8_t)(op + 0xE0), b[4]);
    op = (uint8_t)(op + 3);
    opl((uint8_t)(op + 0x20), b[5]);
    opl((uint8_t)(op + 0x40), b[6] | 0x3F);
    opl((uint8_t)(op + 0x60), b[7]);
    opl((uint8_t)(op + 0x80), b[8]);
    opl(0x01, 0x20);
    opl((uint8_t)(op + 0xE0), b[9]);
    opl((uint8_t)(0xC0 + v), b[10]);
    DSET(VOICE + V_CHANNEL + at, NW(snd_channel, 0));
    voice_level(v, D(VOICE + V_VOL + at));
}

/* S30EF:1565 (voice_volume) */
static void voice_volume(uint16_t voice, uint16_t vol)
{
    uint8_t v = (uint8_t)voice;
    uint16_t at;

    if (v >= 9)
        return;
    at = (uint16_t)(v * 0x14);
    if (taken(v, 0)) {
        DSETB(KEPT + V_OWN_VOL + at, vol);
        return;
    }
    DSETB(VOICE + V_OWN_VOL + at, vol);
    voice_level(v, D(VOICE + V_VOL + at));
}

static Far drv_far(uint16_t off)
{
    Far p;

    p.seg = DRV;
    p.off = off;
    return p;
}

/* S2F39:14B9 (adl_voices_free) */
static void adl_voices_free(uint16_t ch)
{
    uint16_t r = (uint16_t)REC(ch), n = D(r + R_VOICES), c, w;
    int16_t low = (int16_t)(9 - used()), top = (int16_t)(low + n), s;

    SETW(adl_used, 0, used() - n);
    for (c = 8; c < MUSIC; c++)
        if (c != ch && (D(REC(c) + R_FLAGS) & 4) && D(REC(c) + R_FIRST) < D(r + R_FIRST))
            DSET(REC(c) + R_FIRST, D(REC(c) + R_FIRST) + n);
    if (D(r + R_FIRST) > (uint16_t)low)
        for (s = (int16_t)(D(r + R_FIRST) + n - 1); s >= top; s--) {
            uint16_t to = (uint16_t)(VOICE + s * 0x14), from = (uint16_t)(VOICE + (s - (int16_t)n) * 0x14), i;

            for (i = 0; i < 12; i++)
                DSETB(to + i, DB(from + i));
            DSET(to + V_VOL, D(from + V_VOL));
            DSET(to + V_OWN_VOL, D(from + V_OWN_VOL));
            DSET(to + V_NOTE, D(from + V_NOTE));
            DSET(to + V_CHANNEL, D(from + V_CHANNEL));
            SETW(snd_channel, 0, D(to + V_CHANNEL));
            w = D(to + V_NOTE);
            voice_off((uint16_t)s);
            voice_instrument((uint16_t)s, drv_far(to));
            voice_note((uint16_t)s, w);
        }
    for (s = low; s < top; s++) {
        uint16_t at = (uint16_t)(s * 0x14);

        if (D(REC(MUSIC) + R_VOICES) > (uint16_t)s) {
            DSET(VOICE + V_VOL + at, NW(music_volume_now, 0));
            DSET(VOICE + V_OWN_VOL + at, D(KEPT + V_OWN_VOL + at));
            SETW(snd_channel, 0, MUSIC);
            DSET(VOICE + V_CHANNEL + at, MUSIC);
            voice_off((uint16_t)s);
            voice_instrument((uint16_t)s, drv_far((uint16_t)(KEPT + at)));
            voice_note((uint16_t)s, D(KEPT + V_NOTE + at));
        } else {
            voice_off((uint16_t)s);
            voice_level((uint16_t)s, 0);
        }
    }
}

/* S2F39:1754 (adl_stop) */
void adl_stop(uint16_t ch)
{
    uint16_t lock = NW(snd_lock, 0), r = (uint16_t)REC(ch);

    if (!(NW(snd_devices, 0) & 1))
        return;
    if ((int16_t)ch < 8 || (int16_t)ch >= MUSIC || !(D(r + R_FLAGS) & 4) || !(D(r + R_FLAGS) & 1))
        return;
    SETW(snd_lock, 0, 1);
    DSET(r + R_FLAGS, 0);
    DSET(r + R_AT, D(r + R_START));
    DSET(r + R_AT + 2, D(r + R_START + 2));
    adl_voices_free(ch);
    SETW(snd_lock, 0, lock);
}

/* S2F39:09C9 (chan_stop) */
int16_t chan_stop(uint16_t ch)
{
    if (ch > MUSIC)
        return -1;
    if (ch >= 8) {
        adl_stop(ch);
        return 1;
    }
    if (NW(snd_devices, 0) & 2) {
        uint16_t bit = (uint16_t)(1u << ch);

        if (!(NW(dig_mask, 0) & bit))
            return -1;
        DSET(REC(ch) + R_TIMES, 0);
        /* the original waits here while snd_busy is set (the mixer) */
        if (NW(dig_mask, 0) & bit) {
            SETW(dig_mask, 0, NW(dig_mask, 0) & ~bit);
            SETW(dig_count, 0, NW(dig_count, 0) - 1);
        }
    }
    return 1;
}

static uint32_t flin(Far p) { return (uint32_t)p.seg * 16 + p.off; }

/* the next byte of a huge pointer (ptr_post by 1) */
static uint8_t next(Far *p)
{
    uint8_t b = pb(*p, 0);

    *p = far_add(*p, 1);
    return b;
}

static int tag(Far *p, const char *t)
{
    return next(p) == t[0] && next(p) == t[1] && next(p) == t[2] && next(p) == t[3];
}

static uint32_t be32(Far *p)
{
    uint32_t v = 0;
    int i;

    for (i = 0; i < 4; i++)
        v = v << 8 | next(p);
    return v;
}

/* S2F39:0177 (dig_play) */
int16_t dig_play(uint16_t ch, Far p, uint16_t vol, uint16_t times, uint16_t step)
{
    Far start = p, end;
    uint32_t size;
    uint16_t rate, kind, r = (uint16_t)REC(ch);

    if (!(NW(snd_devices, 0) & 2) || ch >= 8)
        return -1;
    if (!tag(&p, "FORM"))
        return -1;
    size = be32(&p);
    end = far_add(start, size);
    while (!tag(&p, "VHDR"))
        if (flin(end) < flin(p))
            return -1;
    rate = (uint16_t)(pb(far_add(p, 0x10), 0) << 8 | pb(far_add(p, 0x11), 0));
    kind = pb(far_add(p, 0x13), 0) ? 3 : 2;
    while (!tag(&p, "BODY"))
        if (flin(end) < flin(p))
            return -1;
    size = be32(&p);
    if (size > 0xFA00u)
        return -1;
    chan_stop(ch);
    if (vol > 0xFF)
        vol = 0xFF;
    size &= 0xFFFE;
    DSET(r + R_FLAGS, 0);
    if (!times)
        DSET(r + R_FLAGS, 2);
    else
        DSET(r + R_TIMES, times);
    DSET(r + R_VOL, vol);
    DSET(r + R_START, p.off);
    DSET(r + R_START + 2, p.seg);
    DSET(r + R_AT + 2, p.seg);
    DSET(r + R_AT, 0);
    DSET(r + R_LEN, size);
    DSET(r + R_KIND, kind);
    DSET(r + R_18, kind & 1);
    DSET(r + R_STEP, step);
    DSET(r + R_HALF, size >> 1);
    DSET(r + R_1A, kind & 2 ? 0x7F : 0);
    DSET(r + R_RATE, rate);
    /* the original waits here while snd_busy is set (the mixer) */
    SETW(dig_count, 0, NW(dig_count, 0) + 1);
    SETW(dig_mask, 0, NW(dig_mask, 0) | 1u << ch);
    return 1;
}

static int tag_at(Far p, uint16_t o, const char *t)
{
    return pb(p, o) == t[0] && pb(p, (uint16_t)(o + 1)) == t[1] && pb(p, (uint16_t)(o + 2)) == t[2]
        && pb(p, (uint16_t)(o + 3)) == t[3];
}

/* be_long's lower word of the dword at p + o */
static uint16_t be_low(Far p, uint16_t o)
{
    return (uint16_t)(pb(p, (uint16_t)(o + 2)) << 8 | pb(p, (uint16_t)(o + 3)));
}

/* S2F39:0AA8 (adl_load) */
int16_t adl_load(uint16_t ch, Far p, uint16_t times)
{
    Far form, head, map, track;
    uint16_t r = (uint16_t)REC(ch);

    if (!(NW(snd_devices, 0) & 1) || (int16_t)ch > MUSIC || (int16_t)ch < 8)
        return -1;
    form = p;
    p.off = (uint16_t)(p.off + 8);
    if (!tag_at(form, 0, "ADLX"))
        return -1;
    head = p;
    p.off = (uint16_t)(p.off + 8);
    if (!tag_at(head, 0, "AHDR"))
        return -1;
    p.off = (uint16_t)(p.off + be_low(head, 4));
    map = p;
    p.off = (uint16_t)(p.off + 8);
    if (!tag_at(map, 0, "IMAP"))
        return -1;
    p.off = (uint16_t)(p.off + be_low(map, 4));
    track = p;
    p.off = (uint16_t)(p.off + 8);
    if (!tag_at(track, 0, "TRCK"))
        return -1;
    if (ch != MUSIC && pb(head, 0x0A) > (uint16_t)(9 - used()))
        return -1;
    if (ch == MUSIC) {
        DSET(0x025C, map.seg);
        DSET(0x025A, map.off + 8);
        DSET(0x022C, pw(head, 0x0E));
    }
    DSET(r + R_STEP, 0x100);
    DSET(r + R_LEN, be_low(form, 4) + 0xFFDE);
    DSET(r + R_VOICES, pb(head, 0x0A));
    DSET(r + R_AT, p.off);
    DSET(r + R_AT + 2, p.seg);
    DSET(r + R_START, p.off);
    DSET(r + R_START + 2, p.seg);
    DSET(r + R_FLAGS, 4);
    if (!times)
        DSET(r + R_FLAGS, 4 | 2);
    else
        DSET(r + R_TIMES, times);
    return 1;
}

/* S2F39:0690 (adl_play) */
int16_t adl_play(uint16_t ch, Far p, uint16_t vol, uint16_t times)
{
    if (ch < 8)
        return -1;
    adl_stop(ch);
    if (!adl_load(ch, p, times))    /* its FFFFh passes too */
        return -1;
    DSET(REC(ch) + R_VOL, vol);
    return 1;
}

static int16_t sfx_play_(uint16_t ch, uint16_t n, uint16_t vol, uint16_t times, uint16_t step);

/* S2F39:06E3 (sfx_play): effect n of the file at sfx_data */
int16_t sfx_play(uint16_t ch, uint16_t n, uint16_t vol, uint16_t times, uint16_t step)
{
    int16_t r = sfx_play_(ch, n, vol, times, step);

    if (snd_played)
        snd_played(ch, n, vol, times, r);
    return r;
}

static int16_t sfx_play_(uint16_t ch, uint16_t n, uint16_t vol, uint16_t times, uint16_t step)
{
    Far file = far_at(S_sfx_data, A_sfx_data), e, p;
    uint32_t size = (uint32_t)NW(sfx_size, 2) << 16 | NW(sfx_size, 0);

    if (ch >= MUSIC)
        return -1;
    e = far_add(file, (uint32_t)pw(file, 2) << 16 | pw(file, 0));
    e = far_add(far_add(e, (uint16_t)(n * 0x0C)), 8);
    if (flin(far_add(file, size)) < flin(e))
        return -1;
    p = far_add(file, (uint32_t)pw(e, 2) << 16 | pw(e, 0));
    if (pb(p, 0) == 'F' && pb(far_add(p, 1), 0) == 'O' && pb(far_add(p, 2), 0) == 'R' && pb(far_add(p, 3), 0) == 'M'
        && pb(far_add(p, 8), 0) == '8' && pb(far_add(p, 9), 0) == 'S' && pb(far_add(p, 10), 0) == 'V'
        && pb(far_add(p, 11), 0) == 'X')
        return dig_play(ch, p, vol, times, step);
    if (pb(p, 0) == 'A' && pb(far_add(p, 1), 0) == 'D' && pb(far_add(p, 2), 0) == 'L' && pb(far_add(p, 3), 0) == 'X'
        && pb(far_add(p, 8), 0) == 'A' && pb(far_add(p, 9), 0) == 'H' && pb(far_add(p, 10), 0) == 'D'
        && pb(far_add(p, 11), 0) == 'R')
        return adl_play(ch, p, vol, times);
    return -1;
}

/* S2F39:0CFB (adl_start) */
int16_t adl_start(uint16_t ch, uint16_t vol)
{
    uint16_t lock = NW(snd_lock, 0), r = (uint16_t)REC(ch), v;
    uint32_t t;

    if (!(NW(snd_devices, 0) & 1))
        return -1;
    SETW(snd_channel, 0, ch);
    if (vol & 0xFF00)
        vol = 0xFF;
    if ((int16_t)ch > MUSIC || (int16_t)ch < 8 || !(D(r + R_FLAGS) & 4) || (D(r + R_FLAGS) & 1))
        return -1;
    SETW(snd_lock, 0, 1);
    DSET(r + R_VOL, vol);
    if ((int16_t)ch < MUSIC) {
        adl_stop(ch);
        SETW(adl_used, 0, used() + D(r + R_VOICES));
        if (used() > 9) {
            SETW(adl_used, 0, used() - D(r + R_VOICES));
            return -1;              /* snd_lock stays 1, as in the original */
        }
        DSET(r + R_FIRST, 9 - used());
    }
    for (v = D(r + R_FIRST); (uint16_t)(D(r + R_FIRST) + D(r + R_VOICES)) > v; v++) {
        uint16_t at = (uint16_t)(v * 0x14), i;

        if (D(REC(MUSIC) + R_VOICES) > v) {
            DSET(KEPT + V_NOTE + at, D(VOICE + V_NOTE + at));
            for (i = 0; i < 12; i++)
                DSETB(KEPT + at + i, DB(VOICE + at + i));
            DSET(KEPT + V_VOL + at, NW(music_volume_now, 0));
            DSET(KEPT + V_OWN_VOL + at, D(VOICE + V_OWN_VOL + at));
        }
        voice_level(v, 0);
        voice_off(v);
        DSET(VOICE + V_VOL + at, vol);
    }
    DSET(r + R_FLAGS, D(r + R_FLAGS) | 1);
    t = ((uint32_t)NW(snd_ticks, 2) << 16 | NW(snd_ticks, 0)) + 3;
    DSET(r + R_DUE, (uint16_t)t);
    DSET(r + R_DUE + 2, (uint16_t)(t >> 16));
    SETW(snd_lock, 0, lock);
    return 1;
}

/* S30EF:1C2D (dsp_rate): the rate the card plays the buffers' bytes at
 * (the original sends the DSP 256 - 1000000 / rate and keeps nothing) */
void dsp_rate(uint16_t rate)
{
    if (NW(snd_devices, 0) & 2)
        dig_rate = rate;
}

/* S30EF:1CED (dig_mix): `len` bytes at S30EF:`buf`, a pair of them a
 * step.  For each pair every playing channel's two bytes at its count of
 * steps times its step / 100h, signed, times half its volume, the upper
 * byte of that summed; each sum divided by the count of channels, plus
 * 7Fh.  A channel past its length gives nothing to that pair and begins
 * again (endless, or times left) or is taken off; when the last one is
 * taken off the routine ends at once, the pair not written.  Returns
 * `len` less the pairs not made (as the original: bytes less pairs). */
static uint16_t dig_mix(uint16_t buf, uint16_t len)
{
    uint16_t pairs = (uint16_t)(len >> 1), ch;

    if (!NW(dig_count, 0))
        return 0;
    SETW(snd_busy, 0, 1);
    DSET(0x10BB, pairs);
    DSET(0x10C3, 0);
    DSET(0x10C5, 0);
    for (;;) {
        for (ch = 0; ch < 8; ch++) {
            uint16_t r = (uint16_t)REC(ch), at;
            int8_t vol;
            int16_t a, b;

            if (!(DB(A_dig_mask) & 1u << ch))
                continue;
            at = (uint16_t)(((uint32_t)D(r + R_AT) * D(r + R_STEP)) >> 8);
            if (at >= D(r + R_LEN)) {
                if (!(D(r + R_FLAGS) & 2)) {
                    int16_t left = (int16_t)(D(r + R_TIMES) - 1);

                    DSET(r + R_TIMES, left);
                    if (left <= 0) {
                        int16_t n = (int16_t)(NW(dig_count, 0) - 1);

                        DSETB(A_dig_mask, DB(A_dig_mask) ^ 1u << ch);
                        SETW(dig_count, 0, n);
                        if (n <= 0)
                            goto done;
                        continue;
                    }
                }
                DSET(r + R_AT, 0);
                continue;
            }
            at = (uint16_t)(at + D(r + R_START));
            vol = (int8_t)(DB(r + R_VOL) >> 1);
            a = (int8_t)((int16_t)((int8_t)frb(D(r + R_START + 2), at) * vol) >> 8);
            b = (int8_t)((int16_t)((int8_t)frb(D(r + R_START + 2), (uint16_t)(at + 1)) * vol) >> 8);
            DSET(0x10C3, D(0x10C3) + a);
            DSET(0x10C5, D(0x10C5) + b);
            DSET(r + R_AT, D(r + R_AT) + 1);
        }
        DSETB(buf, (int16_t)D(0x10C3) / (int16_t)NW(dig_count, 0) + 0x7F);
        DSETB(buf + 1, (int16_t)D(0x10C5) / (int16_t)NW(dig_count, 0) + 0x7F);
        buf = (uint16_t)(buf + 2);
        pairs = (uint16_t)(D(0x10BB) - 1);
        DSET(0x10BB, pairs);
        if ((int16_t)pairs <= 0)
            break;
        DSET(0x10C3, 0);
        DSET(0x10C5, 0);
    }
done:
    SETW(snd_busy, 0, 0);
    return (uint16_t)(len - D(0x10BB));
}

/* S30EF:1DF8 (dig_irq), the card's interrupt: the buffer that waits is
 * started when it has 28h bytes or more (its header: the length at +0,
 * the DMA's page and address at +2 and +4, its place at +6; the first at
 * S30EF:049B, the second 40Ah further on; byte 0499h says which waits)
 * and the other one is mixed anew; else the interrupt is masked.  The
 * transfer itself (dma_start) is the time until the next interrupt. */
void dig_irq(void)
{
    uint16_t h = DB(0x0499) & 1 ? 0x049B : 0x08A5, other = DB(0x0499) & 1 ? 0x08A5 : 0x049B, len;

    if (DB(0x0496) & 1)
        return;
    DSETB(0x0496, 1);
    len = D(h);
    if (len >= 0x28) {
        DSETB(0x0499, DB(0x0499) ^ 1);
        dig_at = clock_now < 0 ? -1 : clock_now + (double)len / dig_rate;
        DSET(h, 0);
        DSET(other, dig_mix(D(other + 6), 0x400));
    } else
        dig_at = -1;
    DSETB(0x0496, 0);
}

/* S30EF:1E8B (dig_kick): with digital channels playing the card halted,
 * the first buffer mixed and made the one that waits, the interrupt */
static void dig_kick(void)
{
    if (!(NW(snd_devices, 0) & 2) || (int16_t)NW(dig_count, 0) <= 0)
        return;
    DSET(0x049B, dig_mix(D(0x04A1), 0x400));
    DSETB(0x0499, 1);
    dig_irq();
}

/* S2F39:096A (snd_pass) */
void snd_pass(void)
{
    uint16_t ch;

    if ((NW(snd_devices, 0) & 2) && NW(dig_count, 0))
        dig_kick();
    if (NW(snd_devices, 0) & 1)
        for (ch = 8; ch < MUSIC; ch++)
            adl_start(ch, D(REC(ch) + R_VOL));
}

/* the timer's period set by a tune (pit_period) */
static void period_set(uint16_t period)
{
    SETW(adl_period, 0, period);
}

/* S30EF:15C7 (music_events): the music's events at `p` up to a wait */
static void music_events(Far p)
{
    uint16_t r = (uint16_t)REC(MUSIC);

    for (;;) {
        uint8_t b = pb(p, 0), v = b & 0x0F;
        uint32_t t;

        p.off++;
        switch (b >> 4) {
        case 0:
            t = ((uint32_t)D(r + R_DUE + 2) << 16 | D(r + R_DUE)) + (uint16_t)((uint16_t)(v << 8 | pb(p, 0)) << 7);
            p.off++;
            DSET(r + R_DUE, (uint16_t)t);
            DSET(r + R_DUE + 2, (uint16_t)(t >> 16));
            DSET(r + R_AT, p.off);
            return;
        case 1:
            if (v < 9) {
                uint16_t w = pw(p, 0);

                p.off = (uint16_t)(p.off + 2);
                if (v >= (uint8_t)(9 - (uint8_t)used()))
                    DSET(KEPT + V_NOTE + v * 0x14, w);
                else
                    voice_note(v, w);
            }
            break;
        case 2:
            if (v < 9) {
                if (v >= (uint8_t)(9 - (uint8_t)used()))
                    DSET(KEPT + V_NOTE + v * 0x14, D(KEPT + V_NOTE + v * 0x14) & 0xFFDF);
                else
                    voice_off(v);
            }
            break;
        case 3:
            if (v < 9)
                voice_volume(v, pb(p, 0));
            p.off++;
            break;
        case 4:
            if (v < 9) {
                voice_instrument(v, p);
                p.off = (uint16_t)(p.off + 0x0B);
            }
            break;
        case 5:
            break;
        case 6:
            period_set((uint16_t)(pb(p, 0) << 8 | pb(p, 1)));
            p.off = (uint16_t)(p.off + 2);
            break;
        case 7:
            if (b == 0x77) {
                if (!(D(r + R_FLAGS) & 2)) {
                    int16_t left = (int16_t)(D(r + R_TIMES) - 1);

                    DSET(r + R_TIMES, left);
                    if (left <= 0) {
                        DSET(r + R_FLAGS, D(r + R_FLAGS) & 0xFFFE);
                        return;
                    }
                }
                DSET(r + R_AT + 2, D(r + R_START + 2));
                DSET(r + R_AT, D(r + R_START));
                DSET(r + R_DUE, NW(snd_ticks, 0));
                DSET(0x0232, NW(snd_ticks, 0));
                DSET(r + R_DUE + 2, NW(snd_ticks, 2));
                DSET(0x0234, NW(snd_ticks, 2));
                return;
            } else {
                uint8_t n = pb(p, 0);

                p.off++;
                p.off = (uint16_t)(p.off + (v ? n : 2));
            }
            break;
        default:
            t = ((uint32_t)D(r + R_DUE + 2) << 16 | D(r + R_DUE)) + (b & 0x7F);
            DSET(r + R_DUE, (uint16_t)t);
            DSET(r + R_DUE + 2, (uint16_t)(t >> 16));
            DSET(r + R_AT, p.off);
            return;
        }
    }
}

/* S30EF:175F (effect_events): the events of the effect whose record is at
 * `r` */
static void effect_events(uint16_t r, Far p)
{
    for (;;) {
        uint8_t b = pb(p, 0), v = (uint8_t)((b & 0x0F) + DB(r + R_FIRST));
        uint32_t t, d;

        p.off++;
        switch (b >> 4) {
        case 0:
            d = (uint32_t)(uint16_t)((uint16_t)((b & 0x0F) << 8 | pb(p, 0)) << 7) * D(r + R_STEP);
            p.off++;
            t = ((uint32_t)D(r + R_DUE + 2) << 16 | D(r + R_DUE)) + (d >> 8);
            DSET(r + R_DUE, (uint16_t)t);
            DSET(r + R_DUE + 2, (uint16_t)(t >> 16));
            DSET(r + R_AT, p.off);
            return;
        case 1:
            if (v < 9) {
                voice_note(v, pw(p, 0));
                p.off = (uint16_t)(p.off + 2);
            }
            break;
        case 2:
            if (v < 9)
                voice_off(v);
            break;
        case 3:
            if (v < 9)
                voice_volume(v, pw(p, 0));
            p.off++;
            break;
        case 4:
            if (v < 9) {
                DSET(VOICE + V_CHANNEL + v * 0x14, NW(snd_channel, 0));
                voice_instrument(v, p);
                p.off = (uint16_t)(p.off + 0x0B);
            }
            break;
        case 5:
            break;
        case 6:
            if (!(NW(music_on, 0) & 1) && !NW(adl_period, 0))
                period_set((uint16_t)(pb(p, 0) << 8 | pb(p, 1)));
            else if (NW(adl_period, 0))     /* the original divides by 0 otherwise */
                DSET(r + R_STEP, (uint16_t)(((uint32_t)pb(p, 0) << 16 | (uint32_t)pb(p, 1) << 8) / NW(adl_period, 0)));
            p.off = (uint16_t)(p.off + 2);
            break;
        case 7:
            if (b == 0x77) {
                if (!(D(r + R_FLAGS) & 2)) {
                    int16_t left = (int16_t)(D(r + R_TIMES) - 1);

                    DSET(r + R_TIMES, left);
                    if (left <= 0) {
                        adl_stop(NW(snd_channel, 0));
                        return;
                    }
                }
                DSET(r + R_AT + 2, D(r + R_START + 2));
                DSET(r + R_AT, D(r + R_START));
                return;
            } else {
                uint8_t n = pb(p, 0);

                p.off++;
                p.off = (uint16_t)(p.off + ((b & 0x0F) ? n : 2));
            }
            break;
        default:
            d = (uint32_t)(b & 0x7F) * D(r + R_STEP);
            t = ((uint32_t)D(r + R_DUE + 2) << 16 | D(r + R_DUE)) + (uint16_t)(d >> 8);
            DSET(r + R_DUE, (uint16_t)t);
            DSET(r + R_DUE + 2, (uint16_t)(t >> 16));
            DSET(r + R_AT, p.off);
            return;
        }
    }
}

static int due(uint16_t r)
{
    uint32_t now = (uint32_t)NW(snd_ticks, 2) << 16 | NW(snd_ticks, 0);

    return now >= ((uint32_t)D(r + R_DUE + 2) << 16 | D(r + R_DUE));
}

/* S30EF:192D (adl_tick), the timer's routine.  Left out: music_restore
 * (S30EF:0250 set, which nothing found sets) */
void adl_tick(void)
{
    uint32_t t = ((uint32_t)NW(snd_ticks, 2) << 16 | NW(snd_ticks, 0)) + 1;
    uint16_t r;

    SETW(snd_ticks, 0, (uint16_t)t);
    SETW(snd_ticks, 2, (uint16_t)(t >> 16));
    if (NW(snd_lock, 0) & 1)
        return;
    SETW(snd_lock, 0, 1);
    if ((NW(music_on, 0) & 1) && due((uint16_t)REC(MUSIC))) {
        DSET(0x10E3, REC(MUSIC));
        SETW(snd_channel, 0, MUSIC);
        music_events(far_at(DRV, (uint16_t)(REC(MUSIC) + R_AT)));
    }
    if (used() & 0x0F) {
        SETW(snd_channel, 0, 8);
        for (r = (uint16_t)REC(8); r < REC(MUSIC); r = (uint16_t)(r + 0x2A)) {
            if ((DB(r + R_FLAGS) & 5) == 5 && due(r)) {
                DSET(0x10E3, r);
                effect_events(r, far_at(DRV, (uint16_t)(r + R_AT)));
            }
            SETW(snd_channel, 0, NW(snd_channel, 0) + 1);
        }
    }
    SETW(snd_lock, 0, 0);
}

/* S30EF:1325 (fade_stop) and S30EF:1342 (fade_start): the fade's routine
 * taken from the timer and given to it (pit_del, pit_add with the period
 * 1D1EBh; its number, which the original keeps at S30EF:1118, is not
 * kept) */
static void fade_stop(void)
{
    if (NW(fade_on, 0) & 1)
        SETW(fade_on, 0, 0);
}

static void fade_start(void)
{
    if (NW(fade_on, 0) & 1)
        return;
    if (snd_at)
        snd_at(2);
    fade_left = FADE_PERIOD;
    SETW(fade_on, 0, 1);
}

/* S2F39:1127 (music_volume) */
int16_t music_volume(uint16_t vol)
{
    uint16_t ch = NW(snd_channel, 0), v;

    if (!(NW(snd_devices, 0) & 1) || !NW(music_on, 0))
        return -1;
    SETW(music_volume_now, 0, vol);
    SETW(snd_channel, 0, MUSIC);
    for (v = 0; D(REC(MUSIC) + R_VOICES) > v; v++)
        voice_level(v, vol);
    SETW(snd_channel, 0, ch);
    return 1;
}

/* S30EF:12D9 (fade_tick), the fade's timer routine: the volume of step
 * `count` of `steps` between the two volumes; one call more ends it */
void fade_tick(void)
{
    uint16_t n = (uint16_t)(D(FADE_COUNT) + 1), steps = D(FADE_STEPS), from = D(FADE_FROM);

    DSET(FADE_COUNT, n);
    if (n > steps) {
        SETW(fade_on, 0, 0);
        return;
    }
    music_volume((uint16_t)(from + (int16_t)((int32_t)(int16_t)(D(FADE_TO) - from) * (int16_t)n / (int16_t)steps)));
}

/* S30EF:1A2B (opl_silence) */
static void opl_silence(void)
{
    uint16_t v;

    opl(0x01, 0x00);
    opl(0x01, 0x20);
    for (v = 0; v < 9; v++) {
        uint8_t op = DB(OPS + v);

        opl((uint8_t)(op + 0x40), 0x3F);
        opl((uint8_t)(op + 0x43), 0x3F);
    }
}

/* S2F39:0F87 (music_start): the tune at `tune` from its beginning,
 * endless, at volume `vol` at once or from 0 in `fade` steps of the
 * fade's timer */
static int16_t music_start_(Far tune, uint16_t fade, uint16_t vol);

int16_t music_start(Far tune, uint16_t fade, uint16_t vol)
{
    int16_t r = music_start_(tune, fade, vol);

    if (snd_music)
        snd_music(0, fade, vol, r);
    return r;
}

static int16_t music_start_(Far tune, uint16_t fade, uint16_t vol)
{
    if (!(NW(snd_devices, 0) & 1))
        return -1;
    fade_stop();
    if (adl_load(MUSIC, tune, 0) < 0)
        return -1;
    SETW(music_on, 0, 1);
    if (!fade) {
        adl_start(MUSIC, vol);
        music_volume(vol);
    } else {
        adl_start(MUSIC, 0);
        DSET(FADE_FROM, 0);
        DSET(FADE_TO, vol);
        DSET(FADE_COUNT, 0);
        DSET(FADE_STEPS, fade);
        fade_start();
    }
    SETW(music_volume_now, 0, vol);
    if (snd_at)
        snd_at(3);
    DSET(0x0234, NW(snd_ticks, 2));
    DSET(0x0232, NW(snd_ticks, 0));
    return 1;
}

/* one interrupt of the timer (S27FE:06D2, pit_irq) as far as the sound
 * goes: the counts of the two routines, then the fade's routine first (it
 * was given to the timer last and pit_irq calls from the last one down)
 * and adl_tick, each when its count has run out.  Returns which ran: 1
 * adl_tick, 2 fade_tick. */
int snd_irq(void)
{
    int adl = 0, fade = 0;

    if (!DB(0x111A))            /* adl_detect has not given adl_tick to the timer */
        return 0;
    adl_left -= (int32_t)pit_ran;
    if (adl_left <= 0) {
        adl = 1;
        adl_left += tick_period();
    }
    if (NW(fade_on, 0) & 1) {
        fade_left -= (int32_t)pit_ran;
        if (fade_left <= 0) {
            fade = 2;
            fade_left += FADE_PERIOD;
        }
    }
    pit_ran = pit_least();
    if (fade)
        fade_tick();
    if (adl)
        adl_tick();
    return adl | fade;
}

/* the sample the next interrupt is due at */
static uint64_t irq_due(void)
{
    if (!irq_begun) {
        irq_begun = 1;
        irq_base = snd_samples(NULL);
        if (irq_base < irq_last)
            irq_base = irq_last;
        irq_pit = 0;
    }
    return irq_base + irq_pit * (uint64_t)snd_rate / 1193182u;
}

/* the next interrupt on the audio's clock, at its sample */
static void irq_audio(void)
{
    uint64_t was = snd_stamp;

    irq_last = snd_stamp = irq_due();
    snd_stamp_irq = 1;
    snd_irq();
    snd_stamp_irq = 0;
    irq_pit += pit_least();
    if (snd_irq_at)
        snd_irq_at(irq_last, pit_least());
    snd_stamp = was > irq_last ? was : irq_last;
}

/* S2F39:1058 (music_fade): the effects stopped; the music off at once
 * with speed 0, else faded out in `speed` steps, waited for.  The
 * original waits in a loop while the timer's interrupts come; here they
 * are run one after the other, so the sound's clock is ahead of the
 * caller's by that time afterwards (snd_clock then calls nothing until
 * it has caught up) and snd_waited is told.  On the audio's clock the
 * same: the fade's interrupts are made at once, each with its sample,
 * ahead of the samples played */
static int16_t music_fade_(uint16_t speed);

int16_t music_fade(uint16_t speed)
{
    int16_t r;

    if (snd_music)
        snd_music(2, speed, 0, 0);
    r = music_fade_(speed);
    if (snd_music)
        snd_music(1, speed, 0, r);
    return r;
}

static int16_t music_fade_(uint16_t speed)
{
    uint16_t ch;
    double s = 0;

    if (!(NW(snd_devices, 0) & 1))
        return -1;
    for (ch = 8; ch < MUSIC; ch++)
        adl_stop(ch);
    if (!NW(music_on, 0))
        return -1;
    fade_stop();
    if (!speed) {
        SETW(music_on, 0, 0);
        return 1;
    }
    DSET(FADE_TO, 0);
    DSET(FADE_FROM, NW(music_volume_now, 0));
    DSET(FADE_COUNT, 0);
    DSET(FADE_STEPS, speed);
    fade_start();
    while (NW(fade_on, 0)) {
        if (snd_wait && snd_wait())
            continue;
        s += pit_least() / 1193182.0;
        if (snd_samples)
            irq_audio();
        else
            snd_irq();
    }
    if (tick_at >= 0)
        tick_at += s;
    SETW(music_on, 0, 0);
    fade_stop();
    SETW(snd_channel, 0, 0);
    opl_silence();
    if (snd_waited)
        snd_waited(s);
    return 1;
}

/* the port's clock at t seconds: the timer's interrupts due by then.  The
 * sound's clock begins anew when it is far from t (another clock began);
 * it may be ahead by a fade's wait (music_fade) */
void snd_clock(double t)
{
    /* the card's interrupts: each when the buffer started has been played */
    if (clock_now > t + 4)
        dig_at = -1;
    clock_now = t;
    while (dig_at >= 0 && dig_at <= t) {
        clock_now = dig_at;
        dig_irq();
    }
    clock_now = t;
    if (snd_samples) {
        uint64_t played = snd_samples(&t);

        if (NW(snd_devices, 0) & 1)
            while (irq_due() <= played)
                irq_audio();
        snd_stamp = played > irq_last ? played : irq_last;
        return;
    }
    if (!(NW(snd_devices, 0) & 1))
        return;
    if (tick_at < 0 || tick_at > t + 4)
        tick_at = t;
    while (tick_at <= t) {
        snd_irq();
        tick_at += pit_least() / 1193182.0;
    }
}

/* S30EF:1A7D (adl_detect): nothing when the AdLib is marked found
 * already; else the chip's two timers tried (the registers 1 to 4; the
 * original reads the status at port 388h before and behind timer 1's run
 * and gives up when the flags are not clear and then set: here the chip
 * is there), the voices silenced, the waveforms allowed, and adl_tick
 * given to the timer with the period 1843h (pit_install, which does
 * nothing when input_install has set the timer up before, and pit_add:
 * here the routine's count and the sound's clock begun anew) */
static int adl_detect(void)
{
    if (DB(0x111A))
        return 1;
    DSETB(0x111A, 1);
    opl(0x01, 0x00);
    opl(0x04, 0x60);
    opl(0x04, 0x80);
    opl(0x02, 0xFF);
    opl(0x04, 0x21);
    opl(0x04, 0x60);
    opl(0x04, 0x80);
    opl_silence();
    opl(0x01, 0x20);
    tick_at = -1;
    irq_begun = 0;
    adl_left = 0x1843;
    fade_left = 0;
    return 1;
}

/* S30EF:1A59 (adl_remove): adl_tick taken from the timer and the timer
 * given back (pit_del, pit_remove: the machine's), the mark cleared, the
 * voices silenced */
static void adl_remove(void)
{
    DSETB(0x111A, 0);
    opl_silence();
}

/* S2F39:0092 (snd_end): nothing when it ran before and no device is
 * left; else the lock set (and left set), the fade's routine off the
 * timer, the devices removed (dig_remove is the machine's only: the
 * DSP's speaker off, its interrupt masked and its vector put back, the
 * DSP reset), snd_devices 0.  The port's game_loop does not call it at
 * its end yet (loop.c); snd_init does when it is called again. */
void snd_end(void)
{
    if (NW(snd_ended, 0) && !NW(snd_devices, 0))
        return;
    SETW(snd_lock, 0, 1);
    fade_stop();
    if (NW(snd_devices, 0) & 2)
        dig_at = -1;
    if (NW(snd_devices, 0) & 1)
        adl_remove();
    SETW(snd_devices, 0, 0);
    SETW(snd_ended, 0, 1);
}

/* S2F39:000B (snd_init) with both devices found: snd_end first when it
 * ran before and devices are left, drv_clear, what
 * dig_buffers works out, the card where the runner has it (port 220h,
 * interrupt 0Fh, its bit 80h of the interrupts' mask), the AdLib
 * (adl_detect).  Left out, the machine's: the interrupts' vectors and
 * their old values, delay_measure's counts, the timer's numbers (pit_add)
 * and the frames waited. */
uint16_t snd_init(uint16_t mask)
{
    uint16_t found = 0, i;
    uint32_t a;

    if (NW(snd_started, 0) && NW(snd_devices, 0))
        snd_end();
    for (i = 0; i < 0x488; i++)
        DSETB(i, 0);
    if (mask & 2) {
        /* dig_buffers: a buffer that would cross a DMA page goes to 0CB7h */
        a = (uint32_t)DRV * 16 + 0x04A3;
        if ((a & 0xFFFF) + 0x400 > 0xFFFF)
            a = (uint32_t)DRV * 16 + 0x0CB7, DSET(0x04A1, 0x0CB7);
        else
            DSET(0x04A1, 0x04A3);
        DSETB(0x049D, a >> 16);
        DSET(0x049F, a);
        a = (uint32_t)DRV * 16 + 0x08AD;
        if ((a & 0xFFFF) + 0x400 > 0xFFFF)
            a = (uint32_t)DRV * 16 + 0x0CB7, DSET(0x08AB, 0x0CB7);
        else
            DSET(0x08AB, 0x08AD);
        DSETB(0x08A7, a >> 16);
        DSET(0x08A9, a);
        SETW(dig_port, 0, 0x220);
        SETW(dig_int, 0, 0x0F);
        DSETB(0x0488, 0x80);
        DSETB(0x0495, 1);       /* dig_find's interrupt came */
        dig_rate = 8000;        /* snd_init's dsp_rate(1F40h) */
        dig_at = clock_now = -1;
        found |= 2;
    }
    if ((mask & 1) && adl_detect())
        found |= 1;
    SETW(snd_devices, 0, found);
    SETW(snd_started, 0, 1);
    return found;
}

/* ---- the fight's sounds: what the fight's pictures (S2532) and
 * attacks_run (S02E6) ask of the routines above.  The pictures keep six
 * sounds, three a side, in the slots 0 to 2 and 4 to 6 (pics_sounds
 * loads them, fightpics.c): 0 and 4 what a side's standing figures sound
 * like, 1 and 5 a figure that begins, 2 and 6 a volley's end.  A slot is
 * its digital channel; without the Sound Blaster's part (S3303:0022 set)
 * its AdLib channel is the word at S3AEE:011Fh. */
#define S_FP (HL_S3AEE + HL_BASE)
#define FB(o) frb(S_FP, (uint16_t)(o))
#define FW(o) frw(S_FP, (uint16_t)(o))

/* a test's: each trigger below ("play" the frame, the slot, the volume,
 * how many times; "side" the frame and the side; "end" the channel;
 * "blast") */
void (*snd_fight)(const char *what, uint16_t a, uint16_t b, uint16_t c, uint16_t d);

static int no_digital(void) { return frw(HL_S3303 + HL_BASE, 0x0022) != 0; }

/* S2532:211E (pics_play): the sound of `slot`, if it has one.  The
 * AdLib's way is read only (S3303:0022 was 0 in every run). */
void pics_play(uint16_t slot, uint16_t vol, uint16_t times)
{
    Far p = far_at(S_FP, (uint16_t)(0x04B4 + slot * 4));

    if (snd_fight)
        snd_fight("play", FW(0x0680), slot, vol, times);
    if (!(p.seg | p.off))
        return;
    if (!no_digital())
        dig_play(slot, p, vol, times, FW(0x04A4 + slot * 2));
    else
        adl_play(FW(0x011F + slot * 2), p, vol, times);
    snd_pass();
}

/* fight_frames, the code before S2532:0F8D: a volley's last frame,
 * behind volley_end */
void pics_volley_sound(uint16_t side)
{
    pics_play((uint16_t)(side * 4 + 2), 0xFF, 1);
}

/* fight_frames, S2532:0F8D to 0FB6: the first frame of a figure with 20h in its
 * record's flags */
void pics_begin_sound(uint16_t side)
{
    pics_play((uint16_t)(side * 4 + 1), 0xFF, 1);
}

/* fight_frames, S2532:148B to 155F: the byte at S3AEE:04D4h + 7 * side
 * has changed to n.  64h (the side has no figures): chan_stop with the
 * side's number, 0 or 1, and not with its slot, 0 or 4, so for the second
 * side this stops the first side's slot 1; then snd_pass.  Else n of its
 * figures stand: slot 0 or 4 a thousand times, the volume by their
 * number, half of it for the second side.  The 64h way is read only: no
 * run met a side without figures. */
void pics_side_sound(uint16_t side, uint8_t n)
{
    if (n == 0x64) {
        if (snd_fight)
            snd_fight("side", FW(0x0680), side, 0, 0);
        chan_stop(side);
        snd_pass();
    } else
        pics_play((uint16_t)(side * 4), (uint16_t)((int16_t)(n * 0x28 + 0x0F) >> (side & 0x1F)), 0x3E8);
}

/* fight_pictures, the loops at S2532:0187 and 01A8: its end, the channels 0 to 7 stopped (8 to
 * 0Bh without the Sound Blaster's part) */
void pics_sounds_stop(void)
{
    uint16_t ch = no_digital() ? 8 : 0, end = no_digital() ? 0x0C : 8;

    for (; ch < end; ch++) {
        if (snd_fight)
            snd_fight("end", ch, 0, 0, 0);
        chan_stop(ch);
    }
}

/* attacks_run, S02E6:0B2B: a fight without pictures has destroyed a
 * unit; before the blasts' loop, with effects on */
void fight_blast_sound(void)
{
    if (snd_fight)
        snd_fight(NW(game_state, 0) & 0x80 ? "blast" : "blast, effects off", 0, 0, 0, 0);
    if (NW(game_state, 0) & 0x80) {
        sfx_play(8, 5, 0xDC, 1, 0x200);
        snd_pass();
    }
}

/* What a frame of fight_frames sounds, found from what the frame left in
 * memory: fightpics.c does not call the four routines above yet, so this
 * is called behind each frame instead (pics_hook's PICS_FRAME, main.c)
 * and goes through the figures' records as the frame did.  It needs what
 * the frame changed as it was before: the byte at 04D4h + 7 * side (a
 * figure's record with number 0 that ended clears it, which is seen
 * here) and, for the second side's figures, the marks at 068Fh as they
 * were at the frame's beginning (the first side's volleys, which come
 * later in the frame, change them; the second side's change the first
 * side's before its figures are gone through).  Not the same as the
 * original where a record of the third list (the pictures that belong to
 * no side) ends a volley, which would change the second side's marks
 * before they are read (told on stderr; not seen), or where a record's
 * last frame was moved in this frame (flags 1 and 2, an end beyond 2EEh)
 * and its side's offset at 006Dh is below -1 (not seen). */
static uint8_t pics_stood[2], pics_marks[7];

static void pics_frame_sounds(void)
{
    int16_t frame = (int16_t)FW(0x0680), side;
    uint16_t i;

    for (side = 2; side >= 0; side--) {
        Far rec = far_at(S_FP, (uint16_t)(0x06A9 + side * 0x0C));
        int cleared = 0;

        for (; !(pb(rec, 0) & 0x80); rec.off = (uint16_t)(rec.off + 0x22)) {
            uint8_t fl = pb(rec, 0), mark;
            uint16_t di = pb(rec, 1);
            int16_t start = (int16_t)(pw(rec, 2) + FW(0x006D + side * 2));
            int16_t end = (int16_t)(pw(rec, 4) + FW(0x006D + side * 2));

            mark = side == 1 && di < 7 ? pics_marks[di] : FB(0x0688 + side * 7 + di);
            if (start > frame || end < frame || !(mark & fl) || ((fl & 0x40) && FB(0x013B)))
                continue;
            if ((fl & 0x10) && end == frame) {
                if (side == 2)
                    fprintf(stderr, "pics_frame_sounds: a volley of the third list, the second side's marks not followed\n");
                pics_volley_sound((uint16_t)side);
            }
            if ((fl & 0x20) && frame == start)
                pics_begin_sound((uint16_t)side);
            if ((fl & 1) && frame + 1 >= end && side < 2 && !di)
                cleared = 1;
        }
        if (side < 2) {
            uint8_t now = FB(0x04D4 + side * 7);

            if (now != (cleared ? 0 : pics_stood[side]))
                pics_side_sound((uint16_t)side, now);
            pics_stood[side] = now;
        }
    }
    for (i = 0; i < 7; i++)
        pics_marks[i] = FB(0x068F + i);
}

/* the pictures' sounds by pics_hook's places (hl.h): what the frames
 * need kept when the loading is done, a frame's sounds, the channels
 * stopped at the end */
void fight_pics_sounds(int at)
{
    uint16_t i;

    switch (at) {
    case PICS_LOADED:
        pics_stood[0] = FB(0x04D4);
        pics_stood[1] = FB(0x04DB);
        for (i = 0; i < 7; i++)
            pics_marks[i] = FB(0x068F + i);
        break;
    case PICS_FRAME:
        pics_frame_sounds();
        break;
    case PICS_END:
        pics_sounds_stop();
        break;
    }
}

/* attacks_run's effect by orders_hook's places (hl.h): the blasts' loop
 * of a fight without pictures runs only when a unit was destroyed, and
 * the effect comes before its first turn */
void fight_orders_sounds(int at, int n)
{
    static int due;

    switch (at) {
    case ORDERS_FIGHT:
        due = !n;
        break;
    case ORDERS_BLAST:
        if (due)
            fight_blast_sound();
        due = 0;
        break;
    case ORDERS_FOUGHT:
        due = 0;
        break;
    }
}
