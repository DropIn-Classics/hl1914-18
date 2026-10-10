/* seq.c - the intro and the outro: the launcher's INTRO.EXE (before
 * HL.EXE) and EXTRO.EXE (after the last battle is won), each a program of
 * its own whose module (INTRO S033D, EXTRO S0340) is translated here over
 * HL.EXE's memory.  The libraries both link are HL.EXE's (their routines
 * match HL.EXE's byte for byte but for relocated words: docs/HANDOFF.md),
 * so the port's translations of them are called: the screen's (page_flip,
 * page_copy, fade_in, pal_level ...), the streams' frames (vdif), the
 * files' loading, the sound's and the scrolled text's of the newspapers
 * (events.c).  The module's own data (INTRO S0E07, EXTRO S0D22: the files'
 * names, the scenes' records, the tables of the frames) is read from the
 * player's INTRO.EXE / EXTRO.EXE into a block of DOS memory, so that its
 * offsets are the original's DS offsets.  The programs' files are found in
 * their own folders (the original runs each with its folder as the current
 * one), the fonts' '..\' names in the game's.
 *
 * The time: the port's clock (play.c's play_tick, play_retraces,
 * play_fade); the timer's routine counts DATA:0308 (the screen's record's
 * word 22Eh), which the waits wait by.  seq_hook is told each page shown
 * (page_flip, or the module's own pan, which shows the other page) for
 * the tests' dumps.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hl.h"
#include "vga.h"
#include "sys.h"

#define S_DATA (HL_DATA + HL_BASE)

void (*seq_hook)(int at, int n);
int seq_flips;

static uint16_t ds;             /* the module's data, as loaded */
static int ended;               /* Esc or the play stopped: the program's end (S033D:17E0) */

static Far scr(void)
{
    return far_at(S_DATA, 0x0092);
}

static void sw(Far p, uint16_t o, uint16_t v)
{
    fww(p.seg, (uint16_t)(p.off + o), v);
}

static void sfar(Far p, uint16_t o, Far v)
{
    sw(p, o, v.off);
    sw(p, (uint16_t)(o + 2), v.seg);
}

static Far d(uint16_t off)
{
    Far p;

    p.seg = ds;
    p.off = off;
    return p;
}

static uint8_t db(uint16_t off) { return frb(ds, off); }

static int is_end(Far p)
{
    return p.seg == 0xFFFF && p.off == 0xFFFF;
}

/* ---- the module's data from the player's program ---- */

/* the program's data segment `seg` (paragraphs from the image's start),
 * `len` bytes, into a block of its own; 0 when the program is not the one
 * the hints describe (its size) */
static int data_load(const char *exe, uint32_t want, uint16_t seg, uint16_t len)
{
    Far name;
    size_t n, i;
    uint8_t *f;
    uint16_t hdr, at;

    at = dos_alloc((uint16_t)((len >> 4) + 1));
    if (!at)
        return 0;
    for (i = 0; exe[i]; i++)
        fwb(at, (uint16_t)i, (uint8_t)exe[i]);
    fwb(at, (uint16_t)i, 0);
    name.seg = at;
    name.off = 0;
    f = host_load(name, &n);
    if (!f || n != want) {
        free(f);
        dos_free(at);
        return 0;
    }
    hdr = (uint16_t)((f[8] | f[9] << 8) * 16);
    for (i = 0; i < len; i++)
        fwb(at, (uint16_t)i, f[hdr + (uint32_t)seg * 16 + i]);
    free(f);
    ds = at;
    return 1;
}

/* ---- the time and the keys ---- */

static void shown(void)
{
    seq_flips++;
    if (seq_hook)
        seq_hook(0, seq_flips);
}

/* page_flip waits for a retrace's beginning */
static void flip(void)
{
    page_flip();
    play_retraces(1);
    shown();
}

static void fade(int in)
{
    if (in)
        fade_in();
    else
        fade_out();
    play_fade();
}

/* S2965:0000 (INTRO S0965:0000): until the next retrace begins */
static void retrace(void)
{
    retrace_wait();
    play_retraces(1);
}

/* S033D:18A3: a key in the BIOS's buffer taken (getch); -1 for Esc, 0 for
 * any other or none.  The port's keys reach the BIOS's buffer only through
 * the timer's routine (input_tick takes it into DATA:045C, the flag at
 * 0460): the key is read there.  In the original the timer's routine takes
 * a key too (S02AF:00A0, int 16h) when it comes before the module asks */
static int key(void)
{
    if (play_stopped()) {
        ended = 1;
        return -1;
    }
    if (!frw(S_DATA, 0x0460))
        return 0;
    fww(S_DATA, 0x0460, 0);
    return frb(S_DATA, 0x045C) == 0x01 ? -1 : 0;
}

/* S033D:1856 (wait(n) in the hints; renamed here, POSIX has a wait()): the timer's count from 0 up to n; -1 for Esc */
static int16_t timer_wait(int16_t n)
{
    int16_t k;

    sw(scr(), 0x22E, 0);
    while ((int16_t)pw(scr(), 0x22E) < n) {
        k = (int16_t)key();
        if (k)
            return k;
        play_tick();
    }
    return 0;
}

/* a wait whose Esc ends the program (the module's calls of 17E0) */
#define WAIT(n) do { if (timer_wait(n) < 0) { ended = 1; goto out; } } while (0)

/* S2F39:1A0B with the tune playing: its beats so far */
static void beat_wait(uint16_t beat)
{
    while (music_beat() < beat && !play_stopped())
        play_tick();
}

/* ---- the original's time where the port computes in none ----
 *
 * The runner's original (6 million instructions a second) takes time to
 * load and unpack a file, to draw a frame of a stream, to copy a page and
 * to move a text on, which the port's clock is told here (play_busy), so
 * that its waits (which begin after that work) end when the original's
 * do.  Measured against the original's flips and pans (port/README.md):
 * a file 0.05 s and 5.5 us a byte of it (packed), a stream's frame 0.8 us a
 * byte of it, a page copied 5 ms; a text's frame depends on its window. */
#define T_FILE 0.05
#define T_FILE_BYTE 5.5e-6
#define T_FRAME_BYTE 0.8e-6
#define T_COPY 0.005
#define T_TEXT_LANDS 0.0166     /* INTRO's countries' text, 108h by 2Ah */
#define T_TEXT_WIDE 0.041       /* EXTRO's texts of F0h by 78h */
#define T_TEXT_STAFF 0.0233       /* EXTRO's credits, 80h by 78h */

static void file_time(Far name)
{
    int32_t n = file_size(name);

    play_busy(T_FILE + (n > 0 ? n : 0) * T_FILE_BYTE);
}

static int32_t load_size(Far place, Far name, Far work)
{
    int32_t r = file_load_size(place, name, work);

    file_time(name);
    return r;
}

static int lbm_load(Far place, Far name, Far work)
{
    int r = lbm_show(place, name, work);

    file_time(name);
    return r;
}

static int32_t font_get(Far name, Far chr, int16_t i, uint16_t gap, Far place, Far work)
{
    int32_t r = font_load(name, chr, i, gap, place, work);

    file_time(name);
    file_time(chr);
    return r;
}

static int32_t text_get(Far name, Far w, Far place, Far work)
{
    int32_t r = paper_text(name, w, place, work);

    file_time(name);
    return r;
}

static int32_t sfx_get(Far place, Far name, Far work)
{
    int32_t r = sfx_load(place, name, work);

    file_time(name);
    return r;
}

/* music_fade waits for the fade's end in the original: the port's clock
 * goes on by that time (as menu.c's) */
static void fade_waited(double s)
{
    double to = play_now() + s, was;

    do {
        was = play_now();
        play_tick();
    } while (play_now() < to && play_now() > was);
}

static void tune_fade(uint16_t speed)
{
    snd_waited = fade_waited;
    music_fade(speed);
    snd_waited = NULL;
}

static void copy(void)
{
    page_copy();
    play_busy(T_COPY);
}

/* a stream's frame (vdif); seq_hook is told its bytes (at 1) */
static Far stream_frame(Far a, uint16_t x, uint16_t y)
{
    Far b = vdif(a, x, y);

    if (!is_end(b))
        play_busy(T_FRAME_BYTE * (double)((uint32_t)b.seg * 16 + b.off - ((uint32_t)a.seg * 16 + a.off)));
    if (seq_hook && !is_end(b))
        seq_hook(1, (int)((uint32_t)b.seg * 16 + b.off - ((uint32_t)a.seg * 16 + a.off)));
    return b;
}

/* ---- the module's routines ---- */

/* S033D:18D4 (scene_load(rec, place, work)): the stream named at rec
 * loaded to place (its place at rec + 20h), then the palette named at rec
 * + 10h, if one is, behind it (rec + 24h); the bytes of both, -1 for an
 * error */
static int32_t scene_load(Far rec, Far place, Far work)
{
    int32_t a, b = 0;

    a = load_size(place, rec, work);
    if (a == -1)
        return -1;
    sfar(rec, 0x20, place);
    place = far_add(place, (uint32_t)a);
    if (pb(rec, 0x10)) {
        Far name = rec;

        name.off = (uint16_t)(name.off + 0x10);
        b = load_size(place, name, work);
        if (b == -1)
            return -1;
        sfar(rec, 0x24, place);
    }
    return a + b;
}

/* S033D:19A5 (anim(x, y, rec, n, speed)): the stream's palette, then at
 * most n frames from its first at x, y, each shown and copied to the page
 * drawn, `speed` counts each; -1 for Esc (its callers do not look) */
static int16_t anim(uint16_t x, uint16_t y, Far rec, int16_t n, int16_t speed)
{
    Far a;

    pal_level(0xFF, pfar(rec, 0x24));
    play_level();
    a = pfar(rec, 0x20);
    while (n > 0 && !is_end(a)) {
        sw(scr(), 0x22E, 0);
        a = stream_frame(a, x, y);
        flip();
        copy();
        if (timer_wait(speed))
            return -1;
        n--;
    }
    return 0;
}

/* S033D:1A47 (pan(x)): from the end of a retrace on the page shown is the
 * screen's word 27Ah's (the pages' segments 27Ah and 27Ch change places)
 * x pixels on: the CRTC's start address x / 4 into it, the pixel panning
 * (x & 3) * 2.  With the record's word 1Ch 0 the far words 24Ch and 254h,
 * 248h and 250h change places too.  Then a wait for the next retrace */
static void pan(uint16_t x)
{
    Far r = scr();
    uint8_t px = (uint8_t)((x & 3) << 1);
    uint16_t at = (uint16_t)((x >> 2) + (uint16_t)(pw(r, 0x27A) << 4)), t;
    uint16_t port = frw(S_DATA, 0x0AFD);

    /* S2966:000C: until a retrace has ended */
    if (!(vga_inb(port) & 8))
        while (!(vga_inb(port) & 8))
            ;
    while (vga_inb(port) & 8)
        ;
    play_retraces(1);
    vga_inb(0x3DA);
    vga_outb(0x3C0, 0x13);
    vga_outb(0x3C0, px);
    vga_outb(0x3C0, 0x20);
    vga_outb(0x3D4, 0x0C);
    vga_outb(0x3D5, (uint8_t)(at >> 8));
    vga_outb(0x3D4, 0x0D);
    vga_outb(0x3D5, (uint8_t)at);
    t = pw(r, 0x27A);
    sw(r, 0x27A, pw(r, 0x27C));
    sw(r, 0x27C, t);
    if (!pw(r, 0x1C)) {
        Far a = pfar(r, 0x24C), b = pfar(r, 0x248);

        sfar(r, 0x24C, pfar(r, 0x254));
        sfar(r, 0x254, a);
        sfar(r, 0x248, pfar(r, 0x250));
        sfar(r, 0x250, b);
    }
    shown();
    retrace();
}

/* S033D:1BD5 (lib(name, place, work, rec)): a library loaded to place:
 * its first dword the offset of its table, 12 bytes an entry (a name of
 * 8, the picture's offset from the library's beginning; 0 ends it), the
 * table made far pointers in its place (rec's far word 0Eh; the count at
 * 12h, the library's size at 0Ah), 12 bytes behind the file cleared.
 * Returns the bytes kept (the table's 8 a picture shorter), -1 for an
 * error.  The last argument (0) is not used. */
static int32_t lib(Far name, Far place, Far work, Far rec)
{
    int32_t size = load_size(place, name, work), v;
    uint32_t off;
    Far p, t;
    int i;

    if (size == -1)
        return -1;
    off = (uint32_t)pb(place, 0) | (uint32_t)pb(place, 1) << 8 | (uint32_t)pb(place, 2) << 16
          | (uint32_t)pb(place, 3) << 24;
    for (i = 0; i < 12; i++)
        pwb(far_add(far_add(place, (uint32_t)size), (uint32_t)i), 0, 0);
    p = far_add(place, off);
    t = p;
    sw(rec, 0x12, 0);
    sfar(rec, 0x0E, t);
    sw(rec, 0x0A, (uint16_t)size);
    sw(rec, 0x0C, (uint16_t)((uint32_t)size >> 16));
    do {
        Far e;

        p = far_add(p, 8);
        v = (int32_t)((uint32_t)pb(p, 0) | (uint32_t)pb(far_add(p, 1), 0) << 8 | (uint32_t)pb(far_add(p, 2), 0) << 16
                      | (uint32_t)pb(far_add(p, 3), 0) << 24);
        p = far_add(p, 4);
        e = far_add(place, (uint32_t)v);
        sfar(t, 0, e);
        t = far_add(t, 4);
        sw(rec, 0x12, (uint16_t)(pw(rec, 0x12) + 1));
    } while (v);
    sw(rec, 0x12, (uint16_t)(pw(rec, 0x12) - 1));
    return size - (int32_t)(int16_t)pw(rec, 0x12) * 12 + (int32_t)(int16_t)pw(rec, 0x12) * 8;
}

/* a character's picture number: 0-9 the digits, 10 on the letters (small
 * and capital the same), 24h on the eleven of the table at `chars`
 * (S0E07:0850), whose rises are at chars + 0Bh; -1 for one not there */
static int glyph_of(uint8_t c, uint16_t chars, int8_t *rise)
{
    int k;

    *rise = 0;
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'z')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'Z')
        return c - 'A' + 10;
    for (k = 0; k < 0x0B; k++)
        if (db((uint16_t)(chars + k)) == c) {
            *rise = (int8_t)db((uint16_t)(chars + 0x0B + k));
            return 0x24 + k;
        }
    return -1;
}

/* S033D:205D (text_width(text, pics, gap)): the pictures' widths (byte
 * 0Eh of each) and the gap after each */
static int16_t text_width(Far text, Far pics, int16_t gap, uint16_t chars)
{
    int16_t w = 0;
    int8_t rise;
    uint8_t c;

    while ((c = pb(text, 0)) != 0) {
        int g = glyph_of(c, chars, &rise);

        text.off++;
        if (g >= 0)
            w = (int16_t)(w + pb(pfar(pics, (uint16_t)(g * 4)), 0x0E) + gap);
    }
    return w;
}

/* S033D:1EE8 (text_draw(x, y, text, pics, save, base, gap)) */
static void text_at(int16_t x, int16_t y, Far text, Far pics, uint16_t base, int16_t gap, uint16_t chars)
{
    int8_t rise;
    uint8_t c;

    while ((c = pb(text, 0)) != 0) {
        int g = glyph_of(c, chars, &rise);

        text.off++;
        if (g >= 0) {
            Far pic = pfar(pics, (uint16_t)(g * 4));

            pic_draw((uint16_t)x, (uint16_t)(y + rise), pic, (uint8_t)base);
            x = (int16_t)(x + pb(pic, 0x0E) + gap);
        }
    }
}

/* S033D:2007 (text_mid(y, text, pics, save, base, gap)): in the middle of
 * the screen, around x A0h */
static void text_mid(int16_t y, Far text, Far pics, uint16_t base, int16_t gap, uint16_t chars)
{
    int16_t w = text_width(text, pics, gap, chars);

    text_at((int16_t)(0xA0 - (w >> 1)), y, text, pics, base, gap, chars);
}

/* S2A27:0004 (INTRO S0A27:0004, pic_part(pic, pw, ph, sx, sy, w, h, dx,
 * dy)): w by h of a picture of pw by ph kept plane after plane (as
 * screen_keep leaves it) from sx, sy to dx, dy of the page drawn, plane by
 * plane from the pixel's own, the place from the table of lines at
 * DATA:00F8; as menu.c's, which has the menu's pw and ph fixed */
static void pic_part(Far pic, uint16_t pwid, uint16_t ph, int16_t sx, int16_t sy, int16_t w, int16_t h, int16_t dx,
                     int16_t dy)
{
    uint16_t rowb = (uint16_t)(pwid >> 2), plane = (uint16_t)(rowb * ph), es = NW(screen_seg, 0);
    uint16_t pitch = NW(screen_pitch, 0), k, r, c;

    for (k = 0; k < 4; k++, sx++, dx++) {
        uint16_t n = (uint16_t)(((uint16_t)w >> 2) + (k < ((uint16_t)w & 3)));
        Far src = far_add(pic, (uint16_t)(plane * ((uint16_t)sx & 3)));
        uint16_t di = (uint16_t)(frw(S_DATA, (uint16_t)(0x00F8 + 2 * dy)) + ((uint16_t)dx >> 2));

        src = far_add(src, (uint16_t)(sy * rowb + ((uint16_t)sx >> 2)));
        vga_outw(0x3C4, (uint16_t)((uint8_t)(0x11 << (dx & 3)) << 8 | 2));
        for (r = 0; r < (uint16_t)h; r++) {
            for (c = 0; c < n; c++)
                vwb(es, (uint16_t)(di + c), pb(src, c));
            src = far_add(src, rowb);
            di = (uint16_t)(di + pitch);
        }
    }
}

/* S2963:0002 (INTRO S09A4:000C): colour c to the DAC */
static void dac_set(uint8_t c, uint8_t r, uint8_t g, uint8_t b)
{
    vga_outb(0x3C8, c);
    vga_outb(0x3C9, r);
    vga_outb(0x3C9, g);
    vga_outb(0x3C9, b);
}

/* S033D:2149 (blend(p1, p2, ranges, list, n, steps)): `steps` steps from
 * the palette p1 to p2 of the n ranges of colours the list names (their
 * first and last in the table `ranges`, two bytes each), a retrace after
 * each range */
static void blend(Far p1, Far p2, uint16_t ranges, Far list, uint8_t n, int16_t steps)
{
    int16_t di;
    uint16_t si;

    for (di = 0; di < steps; di++)
        for (si = 0; si < n; si++) {
            uint16_t c = (uint16_t)(pb(list, si) * 2);

            pal_blend(p1, p2, db((uint16_t)(ranges + c)), db((uint16_t)(ranges + c + 1)), steps, di);
            retrace();
        }
}

/* S033D:21BD (lands(from, to, k, speed)): the countries' record k (32
 * bytes at S0E07:05B0: its steps, the count of countries, their numbers
 * from 2, the step each begins at from 0Ch, its blend's steps from 16h):
 * at each step the countries that begin there blended from `from` to `to`
 * (or back, as the country's record of 0Eh bytes at S0E07:04A6 says: its
 * byte 0, the count of ranges and their numbers in the table at
 * S0E07:048E behind), then `speed` counts waited; Esc ends the program */
static void lands(Far from, Far to, uint16_t k, int16_t speed)
{
    uint16_t rec = (uint16_t)(0x05B0 + k * 0x20), di, si;

    for (di = 0; db(rec) > di; di++) {
        sw(scr(), 0x22E, 0);
        for (si = 0; db((uint16_t)(rec + 1)) > si; si++) {
            uint16_t cr;

            if (db((uint16_t)(rec + 0x0C + si)) != di)
                continue;
            cr = (uint16_t)(0x04A6 + db((uint16_t)(rec + 2 + si)) * 0x0E);
            if (db(cr))
                blend(from, to, 0x048E, d((uint16_t)(cr + 2)), db((uint16_t)(cr + 1)), db((uint16_t)(rec + 0x16 + si)));
            else
                blend(to, from, 0x048E, d((uint16_t)(cr + 2)), db((uint16_t)(cr + 1)), db((uint16_t)(rec + 0x16 + si)));
        }
        if (timer_wait(speed) < 0) {
            ended = 1;
            return;
        }
    }
}

static void pal_take(Far pal)
{
    far_copy(pal, far_at(S_DATA, A_palette_ptr), 0x300);
}

/* the screen's record's rectangle (words 2 to 8) filled with its colour
 * (word 0), rect_fill */
static void rect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    Far r = scr();

    sw(r, 2, x0);
    sw(r, 4, y0);
    sw(r, 6, x1);
    sw(r, 8, y1);
    rect_fill();
}

/* the screen's start: the record's pages and the screen opened (S033D:00E6
 * to 01FF and EXTRO's alike) */
static void screen_start(void)
{
    Far r = scr(), aa = { 0xAA00, 0 };

    sw(r, 0x1C, 0xFFFF);
    screen_open(0x140, 0xC8, 0x1000, 1);
    sfar(r, 0x24C, aa);
    sfar(r, 0x248, aa);
    sfar(r, 0x254, aa);
    sfar(r, 0x250, aa);
    sw(r, 0x27A, 0xA000);
    sw(r, 0x27C, 0xA400);
    sw(r, 0, 0);
    screen_clear();
}

/* S033D:17E0 (the end): tune_fade(5), the screen's mode from before
 * (int 10h; left out: the next program sets its own), the blocks of
 * mem_alloc freed, snd_end, the timer and interrupt 24h given back
 * (input_remove; the port's clock has no interrupts to give back) */
static void seq_end(void)
{
    uint16_t k, blocks = frw(S_DATA, 0x0467);

    tune_fade(5);
    for (k = 0; k < blocks; k++)
        dos_free(frw(S_DATA, (uint16_t)(0x0469 + 2 * k)));
    fww(S_DATA, 0x0467, 0);
    snd_end();
    if (ds)
        dos_free(ds);
    ds = 0;
}

/* the module's mem_alloc(size): where DOS has less room for it than the
 * original (the port keeps HL.EXE loaded under it), the largest block it
 * has */
static Far big_alloc(uint32_t size)
{
    Far p = mem_alloc(size);
    uint16_t n;

    if (p.seg)
        return p;
    fww(S_DATA, 0x0B17, 0);
    for (n = (uint16_t)(size >> 4); n > 0x1000; n = (uint16_t)(n - 0x100)) {
        p = mem_alloc((uint32_t)n << 4);
        if (p.seg)
            return p;
    }
    return p;
}

static const char *fail;        /* a message of the end with an error */

#define CHECK(x, why) do { if ((x) == -1) { fail = (why); goto out; } } while (0)

/* ---- INTRO.EXE (S033D:000C, its main) ---- */

#define INTRO_SIZE 80032u
#define INTRO_DATA 0x0E07u      /* S0E07 */
#define INTRO_LEN 0x1650u

/* S033D:000C: the scenes of the flags 3F7h (the program's argument, "?"
 * aside, never changes them; 8 is not among them), one after the other:
 * 1 BANG (the title, the shot, its palettes and sounds by the frame's
 * number), 10h BUCH1A, 20h BUCH1B, 2 AUTO (the car over MORD.LBM, its
 * colours 20h to 2Fh from CARPAL.DAT), 4 the picture kept scrolled and
 * MORD, 40h BUCH2A, 80h BUCH2B and its palette blended to BUCH2B.PAL,
 * 200h the countries (LAENDER.LBM, the text buch3.txt scrolled, the
 * countries' colours blended by its codes), 100h BUCH3; then the music
 * faded out.  Only Esc ends it before; any other key is taken and
 * nothing done.  Returns 0, or -1 when a file could not be loaded (the
 * message on stderr) */
int intro_play(void)
{
    uint32_t flags = 0x3F7;
    uint16_t snd, digital, i;
    int32_t size;
    Far buf, work, tune, place0, p, rec, a = { 0xFFFF, 0xFFFF }, pal = { 0, 0 }, pal2 = { 0, 0 }, txt, pics, w;
    int16_t frame, pi, si, ch, speed, r = 0, step;
    const char *dir = hl_game_dir;
    char sub[SYS_PATH];

    if (!sys_find(dir, "INTRO", sub, sizeof sub)) {
        fprintf(stderr, "intro: the folder INTRO was not found\n");
        return -1;
    }
    hl_game_dir = sub;
    ended = 0;
    fail = NULL;
    seq_flips = 0;
    if (!data_load("INTRO.EXE", INTRO_SIZE, INTRO_DATA, INTRO_LEN)) {
        hl_game_dir = dir;
        fprintf(stderr, "intro: INTRO.EXE was not found or is not the one the port knows\n");
        return -1;
    }
    snd = snd_init(3);
    digital = !(snd & 2) ? 1 : 0;     /* S0E07:1593: 0 with the Sound Blaster */
    dsp_rate(10000);
    buf = big_alloc(0x72038);
    if (!buf.seg) {
        fail = "No Memory";
        goto out;
    }
    work = buf;
    tune = far_add(buf, 0x1388);
    place0 = far_add(tune, 0x1B58);
    p = place0;
    screen_start();
    flip();
    copy();
    fade(1);
    CHECK(load_size(tune, d(0x0256), work), "INTRO.ADL");

    if (flags & 1) {
        rec = d(0x0078);
        size = scene_load(rec, p, work);
        CHECK(size, "BANG");
        p = far_add(p, (uint32_t)size);
        size = load_size(p, d(0x01E0), work);
        CHECK(size, "BANGPAL.DAT");
        sfar(rec, 0x24, p);
        p = far_add(p, (uint32_t)size);
        size = load_size(p, d(0x0213), work);
        CHECK(size, "BANG.TXT");
        txt = p;
        p = far_add(p, (uint32_t)size);
        size = lib(d(0x024C), p, work, d(0x01CC));
        CHECK(size, "CHR14.LIB");
        p = far_add(p, (uint32_t)size);
        CHECK(sfx_get(p, d((uint16_t)(0x0260 + digital * 0x0E)), work) < 0 ? -1 : 0, "BANG.SFX");
        pal_take(pfar(rec, 0x24));
        pics = pfar(d(0x01CC), 0x0E);
        for (i = 0; i < 3; i++)
            text_mid((int16_t)(0x50 + i * 0x14), far_add(txt, (uint32_t)i * 0x3C), pics, 0xB0, 1, 0x0850);
        flip();
        copy();
        fade(1);
        WAIT(0x12C);
        fade(0);
        a = stream_frame(pfar(rec, 0x20), 0, 0);
        pal_take(pfar(rec, 0x24));
        flip();
        copy();
        fade(1);
        frame = pi = si = ch = 0;
        speed = 5;
        pal_level(0xFF, pfar(rec, 0x24));
        play_level();
        r = 0;
        while (!is_end(a)) {
            a = stream_frame(a, 0, 0);
            if (frame > 0xC1)
                speed = 2;
            flip();
            copy();
            if (pi < 0x2C && db((uint16_t)(0x0422 + pi * 2)) == frame) {
                pal = far_add(pfar(rec, 0x24), (uint32_t)db((uint16_t)(0x0423 + pi * 2)) * 0x300);
                pal_level(0xFF, pal);
                play_level();
                pi++;
            }
            if (digital) {
                if (db((uint16_t)(0x03FE + si * 3)) == frame) {
                    if (ch > 3)
                        ch = 0;
                    sfx_play((uint16_t)(ch + 8), db((uint16_t)(0x03FF + si * 3)), 0xFF, 1, 0x200);
                    snd_pass();
                    si++;
                    ch++;
                }
            } else if (db((uint16_t)(0x036E + si * 3)) == frame) {
                if (ch > 7)
                    ch = 0;
                sfx_play((uint16_t)ch, db((uint16_t)(0x036F + si * 3)), db((uint16_t)(0x0370 + si * 3)), 1, 0x200);
                snd_pass();
                si++;
                ch++;
            }
            if (frame == 0x87)
                music_start(tune, 0x14, 0xFF);
            r = timer_wait(speed);
            if (r < 0) {
                ended = 1;
                goto out;
            }
            if (r > 0)
                break;
            frame++;
        }
        pal_take(pal);
        if (!r)
            WAIT(0x46);
        fade(0);
        sw(scr(), 0, 0);
        screen_clear();
        flip();
        copy();
        pal_take(pfar(rec, 0x24));
        for (i = 0; i < 4; i++)
            text_mid((int16_t)(0x3C + i * 0x14), far_add(txt, 0xB4 + (uint32_t)i * 0x3C), pics, 0xB0, 1, 0x0850);
        flip();
        copy();
        if (snd & 3)
            beat_wait(0x10);
        fade(1);
        WAIT(0x258);
        fade(0);
    }

    if (flags & 0x10) {
        Far r0 = scr();

        pan(0x280);
        sw(r0, 0x0E, 1);
        sw(r0, 0x12, 0x140);
        sw(r0, 0x14, 0);
        sw(r0, 0x16, 0x280);
        sw(r0, 0x18, 0xC8);
        sw(r0, 0, 0);
        rect(0, 0xC8, 0x13F, 0xD2);
        flip();
        rect(0, 0xC8, 0x13F, 0xD2);
        rec = d(0x00C8);
        p = place0;
        size = scene_load(rec, p, work);
        CHECK(size, "BUCH1A");
        p = far_add(p, (uint32_t)size);
        a = stream_frame(pfar(rec, 0x20), 0, 0);
        pal_take(pfar(rec, 0x24));
        flip();
        fade(1);
        WAIT(0x64);
        anim(0, 0, rec, 0x3E7, 5);
        WAIT(0x190);
    }

    if (flags & 0x20) {
        rec = d(0x00F0);
        p = place0;
        size = scene_load(rec, p, work);
        CHECK(size, "BUCH1B");
        p = far_add(p, (uint32_t)size);
        pal_level(0xFF, pfar(rec, 0x24));
        play_level();
        anim(0, 0, rec, 0x3E7, 5);
        if (snd & 3)
            beat_wait(0x36);
    }

    if (flags & 2) {
        if (!(flags & 0x10)) {
            sw(scr(), 0, 0);
            rect(0, 0xC8, 0x13F, 0xD2);
            flip();
            rect(0, 0xC8, 0x13F, 0xD2);
        }
        p = place0;
        if (!lbm_load(p, d(0x021C), work)) {
            fail = "MORD.LBM";
            goto out;
        }
        {
            Far v = { pw(scr(), 0x27A), 0 };

            planes_keep(v, p, 0x50, 0xC8);
        }
        p = far_add(p, 0xFDE8);
        if (!lbm_load(p, d(0x0225), work)) {
            fail = "AUTO.LBM";
            goto out;
        }
        rec = d(0x00A0);
        size = scene_load(rec, p, work);
        CHECK(size, "AUTO");
        p = far_add(p, (uint32_t)size);
        size = load_size(p, d(0x01EC), work);
        CHECK(size, "CARPAL.DAT");
        pal2 = p;
        p = far_add(p, (uint32_t)size);
        pal_level(0xFF, pfar(rec, 0x24));
        play_level();
    }
    a = stream_frame(pfar(d(0x00A0), 0x20), 0, 0);
    if (flags & 2) {
        frame = 0;
        si = 0;
        while (!is_end(a)) {
            unders_restore();
            a = stream_frame(a, 0, 0);
            pan(0x280);
            copy();
            retrace();
            if (si < 0x0A && db((uint16_t)(0x047A + si * 2)) == frame) {
                uint16_t di = 0x60, c;

                for (c = 0x20; c < 0x30; c++, di = (uint16_t)(di + 3))
                    dac_set((uint8_t)c, (uint8_t)(pb(far_add(pal2, di), 0) >> 2),
                            (uint8_t)(pb(far_add(pal2, (uint32_t)di + 1), 0) >> 2),
                            (uint8_t)(pb(far_add(pal2, (uint32_t)di + 2), 0) >> 2));
                pal2 = far_add(pal2, 0x300);
                si++;
            }
            r = timer_wait(5);
            if (r < 0) {
                ended = 1;
                goto out;
            }
            if (r > 0)
                break;
            frame++;
        }
        if (snd & 3)
            beat_wait(0x56);
    }

    if (flags & 4) {
        int16_t di, s = 2;

        r = 0;
        pan(0x280);
        copy();
        for (di = 0x27E; di > 0x13E; di = (int16_t)(di - s)) {
            sw(scr(), 0x16, (uint16_t)(pw(scr(), 0x16) - s));
            unders_restore();
            pic_part(place0, 0x140, 0xC8, di, 0, s, 0xC7, di, 0);
            pan((uint16_t)di);
            pic_part(place0, 0x140, 0xC8, di, 0, s, 0xC7, di, 0);
        }
        if (!r)
            WAIT(0x46);
        rec = d(0x0118);
        p = place0;
        size = scene_load(rec, p, work);
        CHECK(size, "MORD");
        p = far_add(p, (uint32_t)size);
        {
            Far name = rec;

            name.off = (uint16_t)(name.off + 0x10);
            size = load_size(p, name, work);
            CHECK(size, "AUTO.PAL");
        }
        sfar(rec, 0x24, p);
        p = far_add(p, (uint32_t)size);
        anim(0, 0, rec, 0x3E7, 5);
        WAIT(0x64);
    }

    if (flags & 0x40) {
        rec = d(0x0140);
        p = place0;
        size = scene_load(rec, p, work);
        CHECK(size, "BUCH2A");
        p = far_add(p, (uint32_t)size);
        a = stream_frame(pfar(rec, 0x20), 0, 0);
        a = stream_frame(a, 0, 0);
        anim(0, 0, rec, 0x3E7, 5);
    }

    if (flags & 0x80) {
        int16_t di;

        rec = d(0x0168);
        p = place0;
        size = scene_load(rec, p, work);
        CHECK(size, "BUCH2B");
        p = far_add(p, (uint32_t)size);
        size = load_size(p, d(0x01F7), work);
        CHECK(size, "BUCH2B.PAL");
        pal2 = p;
        p = far_add(p, (uint32_t)size);
        pal_level(0xFF, pfar(rec, 0x24));
        play_level();
        a = stream_frame(pfar(rec, 0x20), 0, 0);
        anim(0, 0, rec, 0x3E7, 5);
        WAIT(0x64);
        for (di = 0; di < 0x40; di++) {
            retrace();
            pal_blend(pfar(rec, 0x24), pal2, 0, 0xFF, 0x40, di);
        }
    }

    if (flags & 0x200) {
        Far base, red;

        p = place0;
        size = load_size(p, d(0x0202), work);
        CHECK(size, "BASE.PAL");
        base = p;
        p = far_add(p, (uint32_t)size);
        size = load_size(p, d(0x020B), work);
        CHECK(size, "RED.PAL");
        red = p;
        p = far_add(p, (uint32_t)size);
        if (!lbm_load(p, d(0x022E), work)) {
            fail = "LAENDER.LBM";
            goto out;
        }
        flip();
        pal_level(0xFF, base);
        play_level();
        copy();
        WAIT(0x64);
        if (snd & 3)
            music_seek(0x6F, 0xEA);
        for (i = 0; i < 3; i++) {
            size = font_get(d((uint16_t)(i * 0x14)), d((uint16_t)(0x3C + i * 0x14)), (int16_t)i, 1, p, work);
            if (size < 0) {
                fail = "No font !";
                goto out;
            }
            p = far_add(p, (uint32_t)size);
        }
        w = d(0x0866);
        size = text_get(d(0x15F5), w, p, work);
        if (size < 0) {
            fail = "No text !";
            goto out;
        }
        p = far_add(p, (uint32_t)size);
        /* S2D47:0D65, the buffer's size for these, is called and its
         * result not used */
        if (paper_init(w, p, 0x108, 0x2A, 0x32, 0x32, 0x9C, 0x60)) {
            fail = "No scroll text !";
            goto out;
        }
        if (paper_step(w, 0, 0)) {
            fail = "scroll error !";
            goto out;
        }
        flip();
        if (paper_step(w, 0, 0)) {
            fail = "scroll error !";
            goto out;
        }
        sw(scr(), 0x22E, 0);
        for (;;) {
            paper_restore(w);
            step = paper_step(w, 0, 1);
            play_busy(T_TEXT_LANDS);
            if (step < 0) {
                paper_restore(w);
                fail = "scroll error !";
                goto out;
            }
            if (step == 0x64)
                timer_wait(0x12C);    /* its Esc is not looked at */
            else if (step == 0x3E7)
                break;
            else if (step > 0 && step < 0x64)
                lands(base, red, (uint16_t)(step - 1), 8);
            if (ended)
                goto out;
            flip();
            r = timer_wait(6);
            if (r < 0) {
                paper_restore(w);
                ended = 1;
                goto out;
            }
            if (r > 0)
                break;
        }
        paper_restore(w);
    }

    if (flags & 0x100) {
        rec = d(0x0190);
        p = place0;
        size = scene_load(rec, p, work);
        CHECK(size, "BUCH3");
        p = far_add(p, (uint32_t)size);
        a = stream_frame(pfar(rec, 0x20), 0, 0);
        anim(0, 0, rec, 0x3E7, 5);
        WAIT(0x64);
        pal_take(pfar(rec, 0x24));
        fade(0);
    }
    tune_fade(0x1E);
out:
    if (fail)
        fprintf(stderr, "intro: %s\n", fail);
    seq_end();
    hl_game_dir = dir;
    return fail ? -1 : 0;
}

/* ---- EXTRO.EXE (S0340:0004, its main) ---- */

#define EXTRO_SIZE 75136u
#define EXTRO_DATA 0x0D22u      /* S0D22 */
#define EXTRO_LEN 0x1390u

/* the window w's first rows set (paper_step twice, page_flip between);
 * 0, or -1 with the message */
static int text_start(Far w)
{
    if (paper_step(w, 0, 0)) {
        fail = "scroll error !";
        return -1;
    }
    flip();
    if (paper_step(w, 0, 0)) {
        fail = "scroll error !";
        return -1;
    }
    return 0;
}

/* a row of a scrolled text: what lay under the window put back, the text
 * moved on (paper_step(w, 0, 1)); its code, or -1 with the message */
static int16_t text_row(Far w, double t)
{
    int16_t code;

    paper_restore(w);
    code = paper_step(w, 0, 1);
    play_busy(t);
    if (code < 0) {
        paper_restore(w);
        fail = "scroll error !";
    }
    return code;
}

/* the end of a text's frame: the page shown, 6 counts; 1 when the text
 * ends here (a key's return above 0; Esc ends the program) */
static int text_wait(Far w)
{
    int16_t r;

    flip();
    r = timer_wait(6);
    if (r < 0) {
        paper_restore(w);
        ended = 1;
    }
    return r != 0;
}

/* S0340:0004: with the launcher's argument (the second word "HL14-18";
 * without it the original ends at once: the port's caller is the
 * launcher's part) the scenes of the flags 1Fh: 1 the war's end
 * (EISENBAH.LBM kept, the text warend.txt over it; at its code 1 the
 * picture put back and its colours blended from EISEN0.PAL through
 * EISENBAH.PAL to EISEN2.PAL), 2 the treaty (VERTRAG.LBM blended to
 * VERTRAG2.PAL, vertrag.txt; at its code 1 the stream EURO), 4 the crater
 * (KRATER.LBM, krater.txt), 8 THEEND's stream, 10h the credits (staff.txt,
 * the tune CREDITS1.ADL).  EXTRO1.ADL plays from the first scene on, from
 * its beat 162h back to 107h again and again (music_seek), CREDITS1.ADL
 * from 16Bh back to 1.  Only Esc ends it before.  Returns 0, or -1 when
 * a file could not be loaded */
int extro_play(void)
{
    uint32_t flags = 0x1F;
    int32_t size;
    Far buf, work, p, place, tunes, rec, w, a;
    int16_t r, code, si;
    const char *dir = hl_game_dir;
    char sub[SYS_PATH];

    if (!sys_find(dir, "EXTRO", sub, sizeof sub)) {
        fprintf(stderr, "extro: the folder EXTRO was not found\n");
        return -1;
    }
    hl_game_dir = sub;
    ended = 0;
    fail = NULL;
    seq_flips = 0;
    if (!data_load("EXTRO.EXE", EXTRO_SIZE, EXTRO_DATA, EXTRO_LEN)) {
        hl_game_dir = dir;
        fprintf(stderr, "extro: EXTRO.EXE was not found or is not the one the port knows\n");
        return -1;
    }
    snd_init(3);
    dsp_rate(10000);
    buf = big_alloc(0x72BF0);
    if (!buf.seg) {
        fail = "No Memory";
        goto out;
    }
    work = buf;
    p = far_add(buf, 0x1388);
    screen_start();
    flip();
    copy();
    fade(1);
    for (si = 0; si < 3; si++) {
        size = font_get(d((uint16_t)(si * 0x14)), d((uint16_t)(0x3C + si * 0x14)), si, 1, p, work);
        if (size < 0) {
            fail = "No font !";
            goto out;
        }
        p = far_add(p, (uint32_t)size);
    }
    tunes = p;
    size = load_size(p, d(0x0078), work);
    CHECK(size, "EXTRO1.ADL");
    if (size & 1)
        size++;
    p = far_add(p, (uint32_t)size);
    place = p;

    if (flags & 1) {
        Far kept, pal0, pal1, pal2, v = { pw(scr(), 0x27A), 0 };

        if (!lbm_load(place, d(0x0127), work)) {
            fail = "EISENBAH.LBM";
            goto out;
        }
        planes_keep(v, place, 0x50, 0xC8);
        kept = place;
        place = far_add(place, 0xFA00);
        sw(scr(), 0, 0);
        screen_clear();
        size = load_size(place, d(0x00ED), work);
        CHECK(size, "EISEN0.PAL");
        pal0 = place;
        place = far_add(place, (uint32_t)size);
        size = load_size(place, d(0x00E0), work);
        CHECK(size, "EISENBAH.PAL");
        pal1 = place;
        place = far_add(place, (uint32_t)size);
        size = load_size(place, d(0x00F8), work);
        CHECK(size, "EISEN2.PAL");
        pal2 = place;
        place = far_add(place, (uint32_t)size);
        w = d(0x046A);
        size = text_get(d(0x11FD), w, place, work);
        if (size < 0) {
            fail = "No text !";
            goto out;
        }
        place = far_add(place, (uint32_t)size);
        if (paper_init(w, place, 0xF0, 0x78, 0x30, 0x28, 0x30, 0x50)) {
            fail = "No scroll text !";
            goto out;
        }
        pal_level(0xFF, pal0);
        play_level();
        if (text_start(w))
            goto out;
        sw(scr(), 0x22E, 0);
        music_start(tunes, 0x14, 0xFF);
        music_seek(0x107, 0x162);
        for (;;) {
            code = text_row(w, T_TEXT_WIDE);
            if (code < 0)
                goto out;
            if (code == 0x3E7)
                break;
            if (code == 1) {
                v.seg = pw(scr(), 0x27A);
                planes_put(kept, v, 0x50, 0xC8);
                if (paper_step(w, 0, 0)) {
                    fail = "scroll error !";
                    goto out;
                }
                flip();
                v.seg = pw(scr(), 0x27A);
                planes_put(kept, v, 0x50, 0xC8);
                if (paper_step(w, 0, 0)) {
                    fail = "scroll error !";
                    goto out;
                }
                for (si = 0; si < 0x40; si++) {
                    pal_blend(pal0, pal1, 0, 0x4F, 0x40, si);
                    retrace();
                }
                timer_wait(0x12C);    /* its Esc is not looked at */
                if (ended)
                    goto out;
                for (si = 0; si < 0x40; si++) {
                    retrace();
                    pal_blend(pal1, pal2, 0, 0x4F, 0x40, si);
                }
                pal_level(0xFF, pal2);
                play_level();
                pal_take(pal2);
            }
            if (text_wait(w)) {
                if (ended)
                    goto out;
                break;
            }
        }
        fade(0);
        paper_restore(w);
        flip();
        paper_restore(w);
    }

    if (flags & 2) {
        Far pa, pb2;

        place = p;
        rec = d(0x0090);
        if (!lbm_load(place, d(0x0134), work)) {
            fail = "VERTRAG.LBM";
            goto out;
        }
        size = load_size(place, d(0x010E), work);
        CHECK(size, "VERTRAG.PAL");
        pa = place;
        place = far_add(place, (uint32_t)size);
        size = load_size(place, d(0x011A), work);
        CHECK(size, "VERTRAG2.PAL");
        pb2 = place;
        place = far_add(place, (uint32_t)size);
        size = scene_load(rec, place, work);
        CHECK(size, "EURO");
        place = far_add(place, (uint32_t)size);
        w = d(0x0381);
        size = text_get(d(0x126E), w, place, work);
        if (size < 0) {
            fail = "No text !";
            goto out;
        }
        place = far_add(place, (uint32_t)size);
        if (paper_init(w, place, 0xF0, 0x78, 0x30, 0x28, 0x30, 0x30)) {
            fail = "No scroll text !";
            goto out;
        }
        flip();
        copy();
        if (paper_step(w, 0, 0) < 0) {
            fail = "scroll error !";
            goto out;
        }
        flip();
        if (paper_step(w, 0, 0) < 0) {
            fail = "scroll error !";
            goto out;
        }
        sw(scr(), 0x22E, 0);
        pal_take(pa);
        fade(1);
        WAIT(0x64);
        for (si = 0; si < 0x40; si++) {
            pal_blend(pa, pb2, 0, 0xFF, 0x40, si);
            retrace();
        }
        pal_take(pb2);
        for (;;) {
            code = text_row(w, T_TEXT_WIDE);
            if (code < 0)
                goto out;
            if (code == 0x3E7)
                break;
            if (code == 1) {
                anim(0, 0, rec, 0x3E7, 8);      /* its Esc is not looked at */
                if (ended)
                    goto out;
                copy();
                if (paper_step(w, 0, 1) < 0) {
                    fail = "scroll error !";
                    goto out;
                }
                flip();
                if (paper_step(w, 0, 1) < 0) {
                    fail = "scroll error !";
                    goto out;
                }
            }
            if (text_wait(w)) {
                if (ended)
                    goto out;
                break;
            }
        }
        paper_restore(w);
        WAIT(0xC8);
        fade(0);
    }

    if (flags & 4) {
        Far pal;

        place = p;
        if (!lbm_load(place, d(0x0140), work)) {
            fail = "KRATER.LBM";
            goto out;
        }
        size = load_size(place, d(0x0103), work);
        CHECK(size, "KRATER.PAL");
        pal = place;
        place = far_add(place, (uint32_t)size);
        w = d(0x0298);
        size = text_get(d(0x12E0), w, place, work);
        if (size < 0) {
            fail = "No text !";
            goto out;
        }
        place = far_add(place, (uint32_t)size);
        if (paper_init(w, place, 0xF0, 0x78, 0x30, 0x28, 0x30, 0x20)) {
            fail = "No scroll text !";
            goto out;
        }
        flip();
        copy();
        if (text_start(w))
            goto out;
        sw(scr(), 0x22E, 0);
        pal_take(pal);
        fade(1);
        for (;;) {
            code = text_row(w, T_TEXT_WIDE);
            if (code < 0)
                goto out;
            if (code == 0x3E7)
                break;
            if (text_wait(w)) {
                if (ended)
                    goto out;
                break;
            }
        }
        paper_restore(w);
        WAIT(0xC8);
    }

    if (flags & 8) {
        place = p;
        rec = d(0x00B8);
        size = scene_load(rec, place, work);
        CHECK(size, "THEEND");
        a = pfar(rec, 0x20);
        while (!is_end(a)) {
            a = stream_frame(a, 0, 0);
            flip();
            copy();
            r = timer_wait(8);
            if (r < 0) {
                ended = 1;
                goto out;
            }
            if (r > 0)
                break;
        }
        WAIT(0xC8);
        tune_fade(0x3C);
    }

    if (flags & 0x10) {
        size = load_size(tunes, d(0x0083), work);
        CHECK(size, "CREDITS1.ADL");
        place = far_add(tunes, (uint32_t)size);
        w = d(0x01AF);
        size = text_get(d(0x1333), w, place, work);
        if (size < 0) {
            fail = "No text !";
            goto out;
        }
        place = far_add(place, (uint32_t)size);
        if (paper_init(w, place, 0x80, 0x78, 0x30, 0xB8, 0x30, 0x20)) {
            fail = "No scroll text !";
            goto out;
        }
        flip();
        copy();
        if (text_start(w))
            goto out;
        sw(scr(), 0x22E, 0);
        music_start(tunes, 0x3C, 0xFF);
        music_seek(1, 0x16B);
        for (;;) {
            code = text_row(w, T_TEXT_STAFF);
            if (code < 0)
                goto out;
            if (code == 0x3E7)
                break;
            flip();
            r = timer_wait(6);
            if (r < 0) {
                ended = 1;
                goto out;
            }
            if (r > 0)
                break;
        }
        paper_restore(w);
        WAIT(0xC8);
        fade(0);
    }
    tune_fade(0x3C);
out:
    if (fail)
        fprintf(stderr, "extro: %s\n", fail);
    seq_end();
    hl_game_dir = dir;
    return fail ? -1 : 0;
}
