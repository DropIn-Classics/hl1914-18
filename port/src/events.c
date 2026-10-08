/* events.c - the pictures of events (HL.EXE's S1A39) that game_loop shows
 * behind a battle's loop: D0h, a human side has won; CDh, the other side
 * was left without units; CEh and CFh, a new best score (battle_score).
 * Each is a few files loaded behind `place` (S1A39:034C), streams of
 * frames drawn into the VGA's planes (S2982:0002) and fades.  The other
 * kinds (a site taken, the newspapers of the maps for one player) are not
 * translated.  The music the pictures start and fade (music_start,
 * music_fade) is left out; their sounds go to sound.c.
 *
 * The waits and the loading go to event_hook, which keeps the time
 * (play.c): the port's own clock, or the original's log.
 */
#include <stdio.h>
#include "hl.h"
#include "vga.h"

#define S_DATA (HL_DATA + HL_BASE)
#define S_EV (HL_S3812 + HL_BASE)       /* the pictures' table of files */

int (*event_hook)(int at, int n);
/* a test's: the frame of the picture shown, from 0; -1 before its first
 * and outside a picture */
int event_frame = -1;

static int hook(int at, int n)
{
    return event_hook ? event_hook(at, n) : 0;
}

static void pww(Far p, uint16_t o, uint16_t v)
{
    fww(p.seg, (uint16_t)(p.off + o), v);
}

static Far scr(void)
{
    return far_at(S_DATA, 0x0092);
}

static Far ev(uint16_t off)
{
    Far p;

    p.seg = S_EV;
    p.off = off;
    return p;
}

static int is_end(Far p)
{
    return p.seg == 0xFFFF && p.off == 0xFFFF;
}

/* S1A39:034C: the n files of the table at S3812:tab (24h bytes each: the
 * name, its kind at 14h, the flags at 16h, the place it went to at 1Ch,
 * a library's record at 20h) loaded one behind the other from `place`:
 * kind 1 lbm_show (at an even place), 2 file_load_size, 3 file_load, 4
 * lib_load, 5 sfx_load.  With flag 1 the next goes behind it.  The
 * flags 2 and 4 (the file's size, S2C24:000C, instead of what the load
 * returned) are not in the tables of these pictures: left out.  Returns
 * the bytes loaded, or below 0 for a file that could not be. */
static int32_t res_load(Far place, Far work, uint16_t tab, int n)
{
    int32_t total = 0;
    int i;

    for (i = 0; i < n; i++, tab = (uint16_t)(tab + 0x24)) {
        Far e = ev(tab);
        uint16_t flags = pw(e, 0x16);
        int32_t size = 0;

        fww(S_EV, (uint16_t)(tab + 0x1C), place.off);
        fww(S_EV, (uint16_t)(tab + 0x1E), place.seg);
        switch (pw(e, 0x14)) {
        case 1:
            if (place.off & 1)
                place = far_add(place, 1);
            if (!lbm_show(place, e, work))
                return -1;
            break;
        case 2:
            size = file_load_size(place, e, work);
            if (size == -1)
                return (int32_t)0xFFFFFFFE;
            break;
        case 3:
            {
                Far f = file_load(place, e, work);

                if (!(f.seg | f.off))
                    return (int32_t)0xFFFFFFFE;
            }
            break;
        case 4:
            size = lib_load(e, place, work, pfar(e, 0x20), 0);
            if (size == -1)
                return (int32_t)0xFFFFFFFC;
            break;
        case 5:
            size = sfx_load(place, e, work);
            if (size < 0)
                return (int32_t)0xFFFFFFFB;
            break;
        default:
            fprintf(stderr, "event_picture: a file of kind %u, not translated\n", (unsigned)pw(e, 0x14));
            break;
        }
        if (flags & 1) {
            place = far_add(place, (uint32_t)size);
            total += size;
        }
    }
    return total;
}

/* the place a table's entry was loaded to (its word 1Ch) */
static Far loaded(uint16_t tab)
{
    return far_at(S_EV, (uint16_t)(tab + 0x1C));
}

/* S2982:0002: a frame of a stream ("VDIF") drawn into the page drawn, at
 * x (a multiple of 4) and y: for each of the four planes from x's on,
 * runs of bytes to copy (a byte below 80h, the count) or to fill (80h +
 * count - 2, then the byte), FFh and a word an offset from the place (the
 * plane's end at 3E80h or more).  The place's offset is kept at
 * S2982:00A2.  Returns the next frame, or FFFF:FFFF when `src` is none. */
static Far vdif(Far src, uint16_t x, uint16_t y)
{
    uint16_t es = NW(screen_seg, 0), di, di0, si = src.off, w;
    uint8_t mask, planes, b;
    Far r;

    x &= 0xFFFC;
    di0 = di = (uint16_t)(y * 0x50 + (x >> 2));
    mask = (uint8_t)(1 << (x & 3));
    vga_outw(0x3C4, (uint16_t)(mask << 8 | 2));
    fww(HL_S2982 + HL_BASE, 0x00A2, di0);
    if (frw(src.seg, si) != 0x4456 || frw(src.seg, (uint16_t)(si + 2)) != 0x4649) {
        r.seg = r.off = 0xFFFF;
        return r;
    }
    si = (uint16_t)(si + 4);
    mask = 1;
    for (planes = 4; planes; planes--) {
        vga_outw(0x3C4, (uint16_t)(mask << 8 | 2));
        for (;;) {
            b = frb(src.seg, si++);
            if (b == 0xFF) {
                w = frw(src.seg, si);
                si = (uint16_t)(si + 2);
                di = w;
                if (di >= 0x3E80)
                    break;
                di = (uint16_t)(di + di0);
                b = frb(src.seg, si++);
            }
            if (b & 0x80) {
                uint16_t n = (uint16_t)((b & 0x7F) + 2);
                uint8_t v = frb(src.seg, si++);

                while (n--)
                    vwb(es, di++, v);
            } else
                while (b--)
                    vwb(es, di++, frb(src.seg, si++));
        }
        mask = (uint8_t)(mask << 1);
        if (mask & 0x10) {
            mask = 1;
            di++;
        }
    }
    r.seg = (uint16_t)(src.seg + (si >> 4));
    r.off = (uint16_t)(si & 0x0F);
    return r;
}

static void flip(void)
{
    page_flip();
    hook(EV_RETRACES, 1);
}

/* a frame's beginning: the timer's count (word 22Eh of the screen's
 * record) set to 0 */
static void frame_begin(void)
{
    hook(EV_FRAME, 0);
    event_frame++;
    pww(scr(), 0x22E, 0);
}

/* until the timer's routine has counted n since frame_begin, a retrace at
 * a time (on the original's clock the hook says no: its log's next frame
 * brings the calls) */
static void frame_end(int16_t n)
{
    while ((int16_t)pw(scr(), 0x22E) < n) {
        retrace_wait();
        if (!hook(EV_RETRACES, 1))
            break;
    }
}

static void wait_frames(int n)
{
    hook(EV_RETRACES, n + 1);
}

static void fade(int in)
{
    if (in)
        fade_in();
    else
        fade_out();
    hook(EV_FADE, 0);
}

/* the palette made the stream's (pal) and pal cleared: the fade at the
 * end goes from it to black */
static void pal_take(Far pal)
{
    Far p = far_at(S_DATA, A_palette_ptr);
    uint16_t i;

    for (i = 0; i < 0x300; i++) {
        pwb(p, i, pb(pal, i));
        pwb(pal, i, 0);
    }
}

/* a stream's last 20 frames go down to black, a step each */
static void pal_step(Far pal, int16_t *step)
{
    retrace_wait();
    hook(EV_RETRACES, 1);
    pal_blend(far_at(S_DATA, A_palette_ptr), pal, 0, 0xFF, 0x14, (*step)++);
}

/* S1A39:02C5: the palette pal (300h bytes) made the game's, the screen
 * cleared, the stream's first frame drawn on both pages and faded in */
static Far first_frame(Far a, Far pal)
{
    far_copy(pal, far_at(S_DATA, A_palette_ptr), 0x300);
    pww(scr(), 0, 0);
    screen_clear();
    a = vdif(a, 0, 0);
    flip();
    page_copy();
    fade(1);
    return a;
}

/* S1A39:0A7F (D0h): a side has won.  Two streams (the second the side's,
 * from its beginning again when it ends; the first from frame 15h on) for
 * 46h frames of 0Ch counts, the last 20 of them going down to black */
static int16_t pic_won(Far place, Far work, uint16_t side)
{
    Far a, pal, loop0, loop;
    int16_t di, step = 0;

    if (res_load(place, work, 0x0042, 5) < 0)
        return -1;
    a = loaded(0x0042);
    pal = loaded(0x0066);
    loop0 = loop = loaded(side ? 0x00AE : 0x008A);
    hook(EV_BUSY, EV_LOAD_WON);
    pal_take(pal);
    pww(scr(), 0, 0);
    screen_clear();
    a = vdif(a, 0, 0);
    loop = vdif(loop, 0, 0);
    flip();
    page_copy();
    fade(1);
    /* music_start(the fifth file, 0, FFh): left out */
    for (di = 0; di < 0x46; di++) {
        frame_begin();
        loop = vdif(loop, 0, 0);
        if (is_end(loop))
            loop = vdif(loop = loop0, 0, 0);
        if (di > 0x14 && !is_end(a))
            a = vdif(a, 0, 0);
        if (di > 0x32)
            pal_step(pal, &step);
        flip();
        page_copy();
        frame_end(0x0C);
    }
    pal_level(0xFF, pal);
    hook(EV_LEVEL, 0);
    /* music_fade(0Ah): left out */
    wait_frames(0x64);
    return 0;
}

/* S1A39:0D1A (CEh with f 0, CFh with 1): a new best score, the medal
 * picture `which` (0 to 2) of the side f: its tune loaded first, two
 * streams as in pic_won (4Bh frames of 6 counts, the first from frame 25h
 * on), a third shown to its end, then a picture of the side's library on
 * a background, held for 12Ch frames */
static int16_t pic_medal(Far place, Far work, uint16_t which, uint16_t f)
{
    Far tune, a, pal, loop0, loop;
    int32_t size;
    int16_t si, step = 0;
    uint16_t k;

    size = file_load_size(place, ev(f ? 0x0103 : 0x00F6), work);
    if (size == -1)
        return -1;
    tune = place;
    (void)tune;
    place = far_add(place, (uint32_t)size);
    if (res_load(place, work, 0x0110, 5) < 0)
        return -1;
    a = loaded(0x0110);
    loop0 = loop = loaded(f ? 0x017C : 0x0134);
    pal = loaded(f ? 0x01A0 : 0x0158);
    hook(EV_BUSY, EV_LOAD_MEDAL);
    pal_take(pal);
    pww(scr(), 0, 0);
    screen_clear();
    a = vdif(a, 0, 0);
    loop = vdif(loop, 0, 0);
    flip();
    page_copy();
    fade(1);
    /* music_start(tune, 14h, FFh): left out */
    for (si = 0; si < 0x4B; si++) {
        frame_begin();
        loop = vdif(loop, 0, 0);
        if (is_end(loop))
            loop = vdif(loop = loop0, 0, 0);
        if (si > 0x24 && !is_end(a))
            a = vdif(a, 0, 0);
        if (si > 0x37)
            pal_step(pal, &step);
        flip();
        page_copy();
        frame_end(6);
    }
    pal_level(0xFF, pal);
    hook(EV_LEVEL, 0);
    wait_frames(0x64);

    k = f ? 2 : 0;
    if (res_load(place, work, (uint16_t)(0x01C4 + k * 0x24), 2) < 0)
        return -1;
    hook(EV_BUSY, EV_LOAD_MEDAL2);
    a = first_frame(loaded((uint16_t)(0x01C4 + k * 0x24)), loaded((uint16_t)(0x01E8 + k * 0x24)));
    while (!is_end(a)) {
        frame_begin();
        a = vdif(a, 0, 0);
        flip();
        page_copy();
        frame_end(6);
    }
    wait_frames(0x64);
    fade(0);

    k = f ? 3 : 0;
    if (f > 2)
        f = 2;
    if (res_load(place, work, (uint16_t)(0x0280 + k * 0x24), 3) < 0)
        return -1;
    hook(EV_BUSY, EV_LOAD_MEDAL3);
    pal = loaded((uint16_t)(0x02A4 + k * 0x24));
    far_copy(pal, far_at(S_DATA, A_palette_ptr), 0x300);
    pic_draw(frw(S_EV, (uint16_t)(0x0268 + f * 6 + which * 2)), frw(S_EV, (uint16_t)(0x0274 + f * 6 + which * 2)),
             pfar(far_at(S_EV, 0x0262), (uint16_t)(which * 4)), 0);
    flip();
    fade(1);
    wait_frames(0x12C);
    fade(0);
    /* music_fade(14h): left out */
    return 0;
}

/* S1A39:1262 (CDh): the other side has no unit left.  Its sounds loaded
 * first (the file named in the table at S3812:0002 by S3303:0022), then
 * one stream from frame 15h on for 82h frames of 6 counts (9 from frame
 * 30h on), the sounds started at the frames of the table at S3812:03C4 */
static int16_t pic_wiped(Far place, Far work)
{
    uint16_t snd = frw(HL_S3303 + HL_BASE, 0x0022), si = snd ? 8 : 0, k = 0;
    int16_t di, wait = 6;
    int32_t size;
    Far a;

    size = sfx_load(place, ev((uint16_t)((snd << 5) + 2)), work);
    if (size < 0)
        return -1;
    place = far_add(place, (uint32_t)size);
    if (res_load(place, work, 0x0358, 3) < 0)
        return -1;
    hook(EV_BUSY, EV_LOAD_WIPED);
    a = first_frame(loaded(0x0358), loaded(0x037C));
    for (di = 0; di < 0x82; di++) {
        Far e = ev((uint16_t)(snd * 0x7E + k * 6 + 0x03C4));

        frame_begin();
        if ((int16_t)pw(e, 0) == di) {
            if (snd) {
                if (si > 2)
                    si = 0;
                sfx_play((uint16_t)(si + 8), pb(e, 4), pb(e, 5), pw(e, 2), 0x200);
            } else {
                if (si > 6)
                    si = 0;
                sfx_play(si, pb(e, 4), pb(e, 5), pw(e, 2), 0x200);
            }
            snd_pass();
            k++;
            si++;
        }
        if (di > 0x14 && !is_end(a))
            a = vdif(a, 0, 0);
        flip();
        page_copy();
        if (di == 0x30)
            wait = 9;
        frame_end(wait);
    }
    wait_frames(0x64);
    fade(0);
    return 0;
}

/* S1A39:0001 (event_picture(place, work, kind, arg)): but for a site's
 * (CAh, CBh) the screen's record made the whole screen (its words 0Eh to
 * 1Ch kept and put back after it, pages_set and page_flip each time),
 * inputs_release(1, 0) (the hook), then the picture of the kind */
int16_t event_picture(Far place, Far work, uint16_t kind, uint16_t arg)
{
    Far r = scr();
    uint16_t keep[6];
    static const uint16_t at[6] = { 0x12, 0x16, 0x14, 0x18, 0x1C, 0x0E };
    int16_t res = -1;
    int i, whole = kind != 0xCA && kind != 0xCB;

    for (i = 0; i < 6; i++)
        keep[i] = pw(r, at[i]);
    if (whole) {
        pww(r, 0x0E, 1);
        pww(r, 0x12, 0);
        pww(r, 0x14, 0);
        pww(r, 0x16, 0x13F);
        pww(r, 0x18, 0xC7);
        pww(r, 0x1C, 0);
        pages_set();
        flip();
    }
    hook(EV_RELEASE, 0);
    event_frame = -1;
    switch (kind) {
    case 0xCD:
        res = pic_wiped(place, work);
        break;
    case 0xCE:
    case 0xCF:
        res = pic_medal(place, work, arg, kind == 0xCF);
        break;
    case 0xD0:
        res = pic_won(place, work, arg);
        break;
    default:
        fprintf(stderr, "event_picture: kind %X not translated\n", (unsigned)kind);
        break;
    }
    event_frame = -1;
    if (whole) {
        for (i = 0; i < 6; i++)
            pww(r, at[i], keep[i]);
        pages_set();
        flip();
    }
    return res;
}
