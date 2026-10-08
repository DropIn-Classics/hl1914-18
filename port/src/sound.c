/* sound.c - the sound's routines the game calls (S2F39) and of its driver
 * (S30EF) what they reach, translated over the program's memory: the
 * start, an effect played, stopped and started in a pass, and the timer's
 * routine that reads the AdLib channels' events.  src/HL.hints has the
 * records and the events' meaning.
 *
 * No sound comes out.  What the original sends to the AdLib (ports 388h
 * and 389h) goes to snd_opl when it is set, a register and its value in
 * the original's order, without the waits between them.  The Sound
 * Blaster's part ends at the channel's record: dig_mix, dig_irq and
 * dig_kick, which make the bytes the card plays, are not translated, so a
 * digital channel stays as dig_play left it until chan_stop or the next
 * dig_play on it.
 *
 * The music: music_start, music_fade, music_volume and the fade's timer
 * routine fade_tick, with the tune's events read by the timer's routine.
 * The timer itself (S27FE: pit_irq and its tables of routines) is not
 * translated: snd_irq is one interrupt of it as far as the sound goes,
 * the fade's routine when its period has run out and then adl_tick, and
 * snd_clock calls it at the period the tune set.  music_fade's wait for
 * the fade's end runs those interrupts itself and tells snd_waited how
 * long that was.
 *
 * The port is the game with both devices found (snd_devices 3).
 */
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
void (*snd_played)(uint16_t ch, uint16_t n, int16_t r);   /* a test's: each sfx_play */
/* a test's: each music_start (out 0; a the fade, b the volume) and
 * music_fade (out 1; a the speed) at its end */
void (*snd_music)(int out, uint16_t a, uint16_t b, int16_t r);

static double tick_at = -1;

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

/* what is left of the fade's period, in the timer's counts (pit_add's
 * and pit_irq's count for the routine) */
static int32_t fade_left;

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
        snd_played(ch, n, r);
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

/* S2F39:096A (snd_pass).  Left out: dig_kick when digital channels play */
void snd_pass(void)
{
    uint16_t ch;

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
    DSET(0x0234, NW(snd_ticks, 2));
    DSET(0x0232, NW(snd_ticks, 0));
    return 1;
}

/* one interrupt of the timer (S27FE:06D2, pit_irq) as far as the sound
 * goes: the fade's routine first when its period has run out (it was
 * given to the timer last and pit_irq calls from the last one down), then
 * adl_tick, whose period the timer runs at */
void snd_irq(void)
{
    if (NW(fade_on, 0) & 1) {
        fade_left -= tick_period();
        if (fade_left <= 0) {
            fade_left += FADE_PERIOD;
            fade_tick();
        }
    }
    adl_tick();
}

/* S2F39:1058 (music_fade): the effects stopped; the music off at once
 * with speed 0, else faded out in `speed` steps, waited for.  The
 * original waits in a loop while the timer's interrupts come; here they
 * are run one after the other, so the sound's clock is ahead of the
 * caller's by that time afterwards (snd_clock then calls nothing until
 * it has caught up) and snd_waited is told */
static int16_t music_fade_(uint16_t speed);

int16_t music_fade(uint16_t speed)
{
    int16_t r = music_fade_(speed);

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
        s += tick_period() / 1193182.0;
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
    if (!(NW(snd_devices, 0) & 1))
        return;
    if (tick_at < 0 || tick_at > t + 4)
        tick_at = t;
    while (tick_at <= t) {
        snd_irq();
        tick_at += tick_period() / 1193182.0;
    }
}

/* S2F39:000B (snd_init) with both devices found: drv_clear, what
 * dig_buffers works out, the card where the runner has it (port 220h,
 * interrupt 0Fh, its bit 80h of the interrupts' mask), the AdLib marked
 * found.  Left out, the machine's: the interrupts' vectors and their old
 * values, delay_measure's counts, the timer's numbers (pit_add) and the
 * frames waited. */
uint16_t snd_init(uint16_t mask)
{
    uint16_t found = 0, i;
    uint32_t a;

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
        found |= 2;
    }
    if (mask & 1) {
        DSETB(0x111A, 1);
        tick_at = -1;
        fade_left = 0;
        found |= 1;
    }
    SETW(snd_devices, 0, found);
    SETW(snd_started, 0, 1);
    return found;
}
