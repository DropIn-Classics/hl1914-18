/* events.c - the pictures of events (HL.EXE's S1A39) that game_loop shows
 * behind a battle's loop: D0h, a human side has won; CDh, the other side
 * was left without units; CEh and CFh, a new best score (battle_score).
 * Each is a few files loaded behind `place` (S1A39:034C), streams of
 * frames drawn into the VGA's planes (S2982:0002) and fades; CAh and CBh,
 * a site taken (attacks_run's order 4), over the taker's half of the
 * screen.  The kinds 0 to 2Fh are the newspapers of the maps for one
 * player (S1A39:059D, two pages each, their text set and scrolled by
 * S2D47's routines, paper_*), three of whose pages begin with the
 * pictures C8h, C9h or CCh (sprites over a stream).  The music the
 * pictures start and fade (music_start, music_fade) is left out but for
 * the site's and the newspapers'; their sounds go to sound.c.
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

/* S1A39:1906 (site_picture(x, y, place, work); CAh with x 0, CBh with
 * A0h, y 0): its three files loaded (the table at S3812:086C), the third
 * started as music (music_start(it, 0, FFh)), then the frames of the
 * first stream at x, y in the game's colours, 6 counts each, until it
 * ends; wait_frames(C8h) and music_fade(14h).  The second file is not
 * used here. */
static int16_t pic_site(Far place, Far work, uint16_t x, uint16_t y)
{
    Far a;

    if (res_load(place, work, 0x086C, 3) < 0)
        return -1;
    hook(EV_BUSY, EV_LOAD_SITE);
    a = loaded(0x086C);
    music_start(loaded(0x08B4), 0, 0xFF);
    while (!is_end(a)) {
        frame_begin();
        a = vdif(a, x, y);
        flip();
        page_copy();
        frame_end(6);
    }
    wait_frames(0xC8);
    music_fade(0x14);
    return 0;
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
    /* music_fade(0Ah): left out, its time counted */
    hook(EV_BUSY, EV_WON_END);
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

static void pfww(Far p, uint16_t o, Far v)
{
    pww(p, o, v.off);
    pww(p, (uint16_t)(o + 2), v.seg);
}

/* ---- The pictures some newspapers begin with (their record's byte 4):
 * CCh the ship (ZWI\SHIP.*), C9h the zeppelin (ZWI\ZEPP.*), C8h ZWI\RICHT.*.
 * Their frames are counted without a frame of the original's log
 * (EV_PAPER). ---- */

/* a frame's beginning: the timer's count set to 0 */
static void count_begin(void)
{
    hook(EV_PAPER, 0);
    event_frame++;
    pww(scr(), 0x22E, 0);
}

static uint16_t snd_kind(void)
{
    return frw(HL_S3303 + HL_BASE, 0x0022);
}

/* a sprite's record of 1Ch bytes in S3812: x0, y0 (0, 2), x, y (4, 6),
 * its first and last frame (8, 0Ah), the steps of x and y (bytes 0Ch,
 * 0Dh; at the beginning 0Eh, 0Fh), of its scale (10h; 14h) and the scale
 * (12h; 16h), its picture (far 18h).  S1A39:158C and 1EF9 set them up
 * with the library's first picture (pic) */
static void sprite_start(uint16_t rec, Far pic)
{
    Far s = ev(rec);

    pfww(s, 0x18, pic);
    pww(s, 0x10, pw(s, 0x14));
    pww(s, 0x12, pw(s, 0x16));
    pww(s, 4, pw(s, 0));
    pww(s, 6, pw(s, 2));
    pwb(s, 0x0C, pb(s, 0x0E));
    pwb(s, 0x0D, pb(s, 0x0F));
}

/* in the frames from its first to its last: moved on a step, its scale
 * too, and drawn centred there (sprite_draw; what it covers kept) */
static void sprite_step(uint16_t rec, int16_t k)
{
    Far s = ev(rec), keep1;

    if ((int16_t)pw(s, 8) > k || (int16_t)pw(s, 0x0A) < k)
        return;
    pww(s, 4, (uint16_t)(pw(s, 4) + (int8_t)pb(s, 0x0C)));
    pww(s, 6, (uint16_t)(pw(s, 6) + (int8_t)pb(s, 0x0D)));
    pww(s, 0x12, (uint16_t)(pw(s, 0x12) + pw(s, 0x10)));
    keep1.seg = 0;
    keep1.off = 1;
    sprite_draw((int16_t)pw(s, 4), (int16_t)pw(s, 6), pfar(s, 0x18), keep1, 0, (int16_t)pw(s, 0x12), (int16_t)pw(s, 0x12));
}

/* the sound of an entry of a table of six bytes (the frame at 0, then
 * sfx_play's times at 2, number at 4, volume at 5) on the next of the
 * channels 0 to 6 (sound kind 0) or 8 to 10 (with `first` added) */
static void entry_sound(Far e, int16_t *ch, uint16_t first)
{
    if (snd_kind()) {
        if (*ch > 2)
            *ch = 0;
        sfx_play((uint16_t)(*ch + 8 + first), pb(e, 4), pb(e, 5), pw(e, 2), 0x200);
    } else {
        if (*ch > 6)
            *ch = 0;
        sfx_play((uint16_t)(*ch + first), pb(e, 4), pb(e, 5), pw(e, 2), 0x200);
    }
    snd_pass();
}

/* the sounds' file of the sound kind (the table at S3812:0002, as
 * pic_wiped) loaded first; returns its size, below 0 for none */
static int32_t sounds_load(Far place, Far work)
{
    return sfx_load(place, ev((uint16_t)((snd_kind() << 5) + 2)), work);
}

/* S1A39:149D (CCh): the ship.  Four files from S3812:06D4 (two streams,
 * a palette, a library); six sprites (S3812:07C4) from the library's
 * first picture; the second stream's first frame, a sound (channel 0 or
 * 8, number 5), the first stream by first_frame; then 82h frames of 6
 * counts: the sounds of the table at S3812:0764 (30h bytes a sound kind)
 * at their frames on channels 1 to 7 (9 to 11), the first stream from its
 * beginning again when it ends, the second from frame 57h on to its end,
 * the sprites, page_flip, unders_restore, page_copy, page_flip.  Then the
 * channels 0 to 0Bh stopped, fade_out, unders_restore, page_flip,
 * unders_restore. */
static int16_t pic_ship(Far place, Far work)
{
    int16_t k, s = 0, ch = snd_kind() ? 8 : 0, i;
    int32_t size = sounds_load(place, work);
    Far a, b, c, pics;

    if (size < 0)
        return -1;
    place = far_add(place, (uint32_t)size);
    if (res_load(place, work, 0x06D4, 4) < 0)
        return -1;
    hook(EV_BUSY, EV_LOAD_SHIP);
    a = loaded(0x06D4);
    b = loaded(0x06F8);
    pics = far_at(S_EV, 0x06CE);
    for (i = 0; i < 6; i++)
        sprite_start((uint16_t)(0x07C4 + i * 0x1C), pfar(pics, 0));
    b = vdif(b, 0, 0);
    sfx_play((uint16_t)ch, 5, snd_kind() ? 0xAA : 0x1E, 0x258, 0x200);
    snd_pass();
    first_frame(a, loaded(0x071C));
    c = a;
    for (k = 0; k < 0x82; k++) {
        Far e = ev((uint16_t)(snd_kind() * 0x30 + s * 6 + 0x0764));

        count_begin();
        unders_restore();
        if ((int16_t)pw(e, 0) == k) {
            entry_sound(e, &ch, 1);
            s++;
            ch++;
        }
        c = vdif(c, 0, 0);
        if (is_end(c))
            c = vdif(c = a, 0, 0);
        if (!is_end(b) && k > 0x56)
            b = vdif(b, 0, 0);
        for (i = 0; i < 6; i++)
            sprite_step((uint16_t)(0x07C4 + i * 0x1C), k);
        flip();
        unders_restore();
        page_copy();
        flip();
        frame_end(6);
    }
    for (i = 0; i < 0x0C; i++)
        chan_stop((uint16_t)i);
    fade(0);
    unders_restore();
    flip();
    unders_restore();
    return 0;
}

/* S1A39:1A11 (C9h): the zeppelin.  Four files from S3812:0938 (a stream,
 * two palettes, a library); the first palette made the game's, the
 * screen cleared, the stream's first frame, the library's first picture
 * at 85h, 2Eh (sprite_draw, full size), the first frame again, page_flip,
 * page_copy, fade_in, a sound (channel 0 or 8, number 2, volume C8h),
 * wait_frames(C8h).  Then the stream to its end or frame 35h, 6 counts a
 * frame: the sounds of the table at S3812:08EC (frame, volume) at their
 * frames, the colours from frame 8 on 0Ah steps from the first palette
 * to the second, from frame 13h on 0Fh steps back (S2A62:0008, a
 * retrace_wait before each).  Then 32h frames of the picture growing
 * (scale 100h, 14h more a frame), the stream's first frame drawn again
 * behind the first; the channel stopped, fade_out, unders_restore,
 * page_flip, unders_restore. */
static int16_t pic_zeppelin(Far place, Far work)
{
    int16_t di = 0, p1 = 0, p2 = 0, ch = snd_kind() ? 8 : 0, si;
    uint16_t n = 0xC8;
    int32_t size = sounds_load(place, work);
    Far a, a0, pal, pal2, pic, keep1;

    if (size < 0)
        return -1;
    place = far_add(place, (uint32_t)size);
    if (res_load(place, work, 0x0938, 4) < 0)
        return -1;
    hook(EV_BUSY, EV_LOAD_ZEPPELIN);
    a = loaded(0x0938);
    pal = loaded(0x095C);
    pal2 = loaded(0x0980);
    pic = pfar(far_at(S_EV, 0x08E6), 0);
    keep1.seg = 0;
    keep1.off = 1;
    far_copy(pal, far_at(S_DATA, A_palette_ptr), 0x300);
    pww(scr(), 0, 0);
    screen_clear();
    a0 = a;
    a = vdif(a, 0, 0);
    sprite_draw(0x85, 0x2E, pic, keep1, 0, 0x100, 0x100);
    a = vdif(a = a0, 0, 0);
    flip();
    page_copy();
    fade(1);
    sfx_play((uint16_t)ch, 2, n, 0xFF, 0x200);
    snd_pass();
    wait_frames(0xC8);
    while (!is_end(a)) {
        count_begin();
        for (si = 0; si < 6; si++)
            if (frb(S_EV, (uint16_t)(0x08EC + si * 2)) == di) {
                n = frb(S_EV, (uint16_t)(0x08ED + si * 2));
                sfx_play((uint16_t)ch, 2, n, 0xC8, 0x200);
                snd_pass();
            }
        a = vdif(a, 0, 0);
        if (di > 7 && p1 < 0x0A) {
            retrace_wait();
            hook(EV_RETRACES, 1);
            pal_blend(pal, pal2, 0, 0xFF, 0x0A, p1);
            p1++;
        }
        if (di > 0x12 && p2 < 0x0F) {
            retrace_wait();
            hook(EV_RETRACES, 1);
            pal_blend(pal2, pal, 0, 0xFF, 0x0F, p2);
            p2++;
        }
        flip();
        page_copy();
        frame_end(6);
        if (++di == 0x35)
            break;
    }
    for (si = 0; si < 0x32; si++) {
        /* (the original sets the count after unders_restore) */
        count_begin();
        unders_restore();
        sprite_draw(0x85, 0x2E, pic, keep1, 0, (int16_t)(si * 0x14 + 0x100), (int16_t)(si * 0x14 + 0x100));
        flip();
        if (!si)
            vdif(a0, 0, 0);
        frame_end(6);
    }
    chan_stop((uint16_t)ch);
    fade(0);
    unders_restore();
    flip();
    unders_restore();
    return 0;
}

/* S1A39:1DD5 (C8h): ZWI\RICHT.*.  Four files from S3812:0AE0 (a
 * background shown by lbm_show, a library, a stream, a palette); the
 * palette made the game's, page_flip, page_copy, fade_in, a sound
 * (channel 9, number 1); 0Fh sprites (S3812:0B70) from the library's third
 * picture, the second and the third sprite with its second and first.
 * Then 8Ch frames of 3 counts: the sounds of the table at S3812:0A1C (two
 * bytes, frame and volume, 0Eh a sound kind) on channel 7 (0Bh), those of
 * the table at S3812:0A38 (54h bytes a sound kind) as pic_ship's on
 * channels 0 to 6 (8 to 10), the sprites, changed on the way: at frame 28h
 * the first's steps 1, 1 and its scale's 41h, at 46h the second's y step
 * 1 and the third's scale's 2Dh, at 6Eh the second's y step 2 (each set
 * after each sprite drawn in those frames).  Then the stream to its end,
 * 6 counts a frame; the channel stopped, wait_frames(64h), fade_out,
 * unders_restore, page_flip, unders_restore. */
static int16_t pic_richt(Far place, Far work)
{
    int16_t k, s = 0, ch = 0, di, i;
    int32_t size = sounds_load(place, work);
    Far a, pics;

    if (size < 0)
        return -1;
    place = far_add(place, (uint32_t)size);
    if (res_load(place, work, 0x0AE0, 4) < 0)
        return -1;
    hook(EV_BUSY, EV_LOAD_RICHT);
    a = loaded(0x0B28);
    far_copy(loaded(0x0B4C), far_at(S_DATA, A_palette_ptr), 0x300);
    flip();
    page_copy();
    fade(1);
    sfx_play(9, 1, 0xFF, 2, 0x200);
    snd_pass();
    pics = far_at(S_EV, 0x09D6);
    for (i = 0; i < 0x0F; i++)
        sprite_start((uint16_t)(0x0B70 + i * 0x1C), pfar(pics, 8));
    pfww(ev(0x0BA4), 0, pfar(pics, 0));
    pfww(ev(0x0B88), 0, pfar(pics, 4));
    for (k = 0; k < 0x8C; k++) {
        Far e = ev((uint16_t)(snd_kind() * 0x54 + s * 6 + 0x0A38));

        /* (the original sets the count after unders_restore) */
        count_begin();
        unders_restore();
        for (di = 0; di < 7; di++) {
            uint16_t t = (uint16_t)(snd_kind() * 0x0E + di * 2);

            if (frb(S_EV, (uint16_t)(0x0A1C + t)) == k) {
                sfx_play(snd_kind() ? 0x0B : 7, 2, frb(S_EV, (uint16_t)(0x0A1D + t)), 0xC8, 0x200);
                snd_pass();
            }
        }
        if ((int16_t)pw(e, 0) == k) {
            entry_sound(e, &ch, 0);
            s++;
            ch++;
        }
        for (di = 0; di < 0x0F; di++) {
            sprite_step((uint16_t)(0x0B70 + di * 0x1C), k);
            if (k == 0x46) {
                fww(S_EV, 0x0B9C, 0x2D);
                fwb(S_EV, 0x0B99, 1);
            }
            if (k == 0x28) {
                fww(S_EV, 0x0B80, 0x41);
                fwb(S_EV, 0x0B7D, 1);
                fwb(S_EV, 0x0B7C, 1);
            }
            if (k == 0x6E)
                fwb(S_EV, 0x0B99, 2);
        }
        frame_end(3);
        flip();
    }
    while (!is_end(a)) {
        count_begin();
        a = vdif(a, 0, 0);
        flip();
        page_copy();
        frame_end(6);
    }
    chan_stop(snd_kind() ? 0x0B : 7);
    wait_frames(0x64);
    fade(0);
    unders_restore();
    flip();
    unders_restore();
    return 0;
}

/* ---- S2D47: the newspaper's text (paper_*), set a line at a time into a
 * buffer of four planes with two pixels a byte (a pixel x in plane x & 3,
 * byte x >> 3, the high half for x & 4 clear) and scrolled up through a
 * window of the screen.  The fonts are kept in S3B76, 416h bytes each:
 * word 2 the pictures' count, 4 the far table of their offsets, 8 the far
 * library, 0Ch its size, 10h a word for each character (its picture,
 * FFFFh none), 210h a word for each (its width), 410h the widest, 412h
 * the highest, 414h the mean width (a space's), 416h the gap between
 * characters.  The window's record (S3812:0D14 here) is named at
 * paper_init. ---- */

#define S_FONT (HL_S3B76 + HL_BASE)

static void fill(Far p, uint8_t v, uint16_t n)
{
    uint16_t i;

    for (i = 0; i < n; i++)
        pwb(far_add(p, i), 0, v);
}


static Far far_off(Far p, uint16_t o)
{
    p.off = (uint16_t)(p.off + o);
    return p;
}

/* S2D47:1AB4: a "PLIB" library loaded to the place at the font's word 6
 * (rec is the font's word 2): its size at 0Ah, the table of offsets
 * (12 bytes in) at 2, the count (its word 8) at 0 */
static int16_t plib_open(Far name, Far rec, Far work)
{
    Far f = file_load(pfar(rec, 6), name, work);

    if (!(f.seg | f.off))
        return -1;
    pfww(rec, 6, f);
    if (pb(f, 0) != 'P' || pb(f, 1) != 'L' || pb(f, 2) != 'I' || pb(f, 3) != 'B')
        return -2;
    {
        int32_t n = file_size(name);

        pww(rec, 0x0A, (uint16_t)n);
        pww(rec, 0x0C, (uint16_t)((uint32_t)n >> 16));
    }
    pfww(rec, 2, far_off(f, 0x0C));
    pww(rec, 0, pw(f, 8));
    return 0;
}

/* S2D47:0038 (font_load(name, chr, i, gap, place, work)): font i's
 * library (`name`) loaded to place, then its list of characters (`chr`:
 * a line for each picture, the character and its width in digits) behind
 * it, read into the font's tables; returns the library's size (the list
 * is not kept), below 0 for an error.  Without a place it would take
 * memory of its own (mem_alloc, S2CC5:0004): not translated, the
 * newspaper always gives one. */
static int32_t font_load(Far name, Far chr, int16_t i, uint16_t gap, Far place, Far work)
{
    Far rec, p, offs, pic;
    uint16_t r, k, n, maxw = 0, maxh = 0;
    int32_t sum = 0;
    uint8_t c;

    if (i < 0 || i > 6)
        return -1;
    r = (uint16_t)(i * 0x416);
    rec.seg = S_FONT;
    rec.off = (uint16_t)(r + 2);
    fww(S_FONT, (uint16_t)(r + 8), place.off);
    fww(S_FONT, (uint16_t)(r + 0x0A), place.seg);
    if (plib_open(name, rec, work))
        return -2;
    if (place.seg | place.off)
        place = far_add(place, (uint32_t)frw(S_FONT, (uint16_t)(r + 0x0E)) << 16 | frw(S_FONT, (uint16_t)(r + 0x0C)));
    p = file_load(place, chr, work);
    if (!(p.seg | p.off))
        return -3;
    fill(far_off(rec, 0x0E), 0xFF, 0x200);
    fill(far_off(rec, 0x20E), 0, 0x200);
    for (k = 0; (int16_t)frw(S_FONT, (uint16_t)(r + 2)) > (int16_t)k; k++) {
        n = 0;
        c = pb(p, 0);
        p.off++;
        fww(S_FONT, (uint16_t)(r + 0x10 + c * 2), k);
        offs = far_at(S_FONT, (uint16_t)(r + 4));
        pic = far_at(S_FONT, (uint16_t)(r + 8));
        pic.off = (uint16_t)(pic.off + pw(offs, (uint16_t)(k * 2)));
        if (pw(pic, 2) > 0x0A)
            return -4;
        while (pb(p, 0) >= '0' && pb(p, 0) <= '9') {
            n = (uint16_t)(n * 10 + pb(p, 0) - '0');
            p.off++;
        }
        sum += (int16_t)n;
        fww(S_FONT, (uint16_t)(r + 0x210 + c * 2), n);
        if ((int16_t)n > (int16_t)maxw)
            maxw = n;
        if (pw(pic, 4) > maxh)
            maxh = pw(pic, 4);
        while (pb(p, 0) != 0x0A && pb(p, 0) != 0x0D)
            p.off++;
        while (pb(p, 0) == 0x0A || pb(p, 0) == 0x0D)
            p.off++;
    }
    fww(S_FONT, (uint16_t)(r + 0x416), gap);
    fww(S_FONT, (uint16_t)(r + 0x410), maxw);
    fww(S_FONT, (uint16_t)(r + 0x412), maxh);
    fww(S_FONT, (uint16_t)(r + 0x414), k ? (uint16_t)(sum / (int16_t)k) : 0);
    fww(S_FONT, (uint16_t)(r + 0x250), frw(S_FONT, (uint16_t)(r + 0x414)));
    return (int32_t)((uint32_t)frw(S_FONT, (uint16_t)(r + 0x0E)) << 16 | frw(S_FONT, (uint16_t)(r + 0x0C)));
}

/* S2D47:02BD (text_load(name, win, place, work)): the text loaded to
 * place; the window's far words 0 (the text) and 4 (where it is read)
 * set to it, its size at 8, the alignment (byte 0Eh) 'L' */
static int32_t paper_text(Far name, Far w, Far place, Far work)
{
    Far f = file_load(place, name, work);
    int32_t n;

    if (!(f.seg | f.off))
        return -1;
    fill(w, 0, 4);
    pfww(w, 0, f);
    pfww(w, 4, f);
    n = file_size(name);
    pww(w, 8, (uint16_t)n);
    pww(w, 0x0A, (uint16_t)((uint32_t)n >> 16));
    pwb(w, 0x0E, 'L');
    return n;
}

/* a rectangle of the window's record: x0, y0, x1, y1 */
typedef struct { uint16_t x0, y0, x1, y1; } Rect;

static Rect rect_at(Far w, uint16_t o)
{
    Rect r;

    r.x0 = pw(w, o);
    r.y0 = pw(w, (uint16_t)(o + 2));
    r.x1 = pw(w, (uint16_t)(o + 4));
    r.y1 = pw(w, (uint16_t)(o + 6));
    return r;
}

/* S2D47:1785: a rectangle of the buffer (its planes' far words at desc,
 * the bytes a row at desc + 10h) filled with v in both halves */
static void paper_clear(Far desc, Rect r, uint8_t v)
{
    uint16_t p0 = r.x0 & 3, n = (uint16_t)((r.x1 - r.x0) >> 2), pitch = pw(desc, 0x10), di, y;

    v = (uint8_t)(v | v << 4);
    /* the count is made one more when (p0 & 3) < (x0 & 3): never */
    for (di = p0; (uint16_t)(p0 + 4) > di; di++)
        for (y = r.y0; r.y1 > y; y++)
            fill(far_off(pfar(desc, (uint16_t)((di & 3) * 4)), (uint16_t)(y * pitch + (r.x0 >> 3))), v, (uint16_t)(n >> 1));
}

/* S2F11:00E9: a character's picture (word 0 the bytes of one copy of its
 * planes, 2 the bytes a row (at most 14h), 4 the rows; at 8 its four
 * planes, then the same shifted by half a byte) put into the buffer at
 * x, y: whole bytes copied where it begins in a byte's high half, the
 * first byte of each row or-ed in where it begins in the low half */
static void glyph_put(Far pic, Far desc, uint16_t x, uint16_t y, uint16_t pitch)
{
    Far pl[4];
    uint16_t off0 = (uint16_t)(y * pitch + (x >> 3)), w0 = pw(pic, 0), si = pic.off, d, k;
    uint8_t wb, h, r, skip, b = x & 3, cnt = 4;
    int shifted;

    for (k = 0; k < 4; k++)
        pl[k] = pfar(desc, (uint16_t)(k * 4));
    if (pw(pic, 2) > 0x14)
        return;
    wb = (uint8_t)pw(pic, 2);
    h = pb(pic, 4);
    skip = (uint8_t)(pitch - wb);
    shifted = (x & 4) != 0;
    si = (uint16_t)(si + 8 + (shifted ? w0 : 0));
    for (;;) {
        d = (uint16_t)(pl[b].off + off0);
        for (r = h; r; r--) {
            uint8_t n = wb;

            if (shifted) {
                fwb(pl[b].seg, d, (uint8_t)(frb(pl[b].seg, d) | frb(pic.seg, si)));
                si++;
                d++;
                n = (uint8_t)(wb - 1);
            }
            while (n--)
                fwb(pl[b].seg, d++, frb(pic.seg, si++));
            d = (uint16_t)(d + skip);
        }
        if (!--cnt)
            return;
        if (++b == 4) {
            b = 0;
            if (shifted) {
                si = (uint16_t)(si - w0);
                off0++;
            } else
                si = (uint16_t)(si + w0);
            shifted = !shifted;
        }
    }
}

/* S2D47:185C: character n of a font (its picture's offset in the table
 * at the font's word 2, from the library at its word 6) into the buffer */
static void glyph(Far font, int16_t n, Far desc, uint16_t x, uint16_t y)
{
    Far pic = pfar(font, 6);
    uint32_t o;

    if (n < 0)
        return;
    o = (uint32_t)pic.off + pw(pfar(font, 2), (uint16_t)(n * 2));
    pic.off = (uint16_t)o;
    pic.seg = (uint16_t)(pic.seg + (o >> 16));
    glyph_put(pic, desc, x, y, pw(desc, 0x10));
}

/* S2F11:01F4: n bytes of a plane of the page drawn at off, h rows, from
 * the buffer's plane (two pixels a byte; pitch its bytes a row): each half
 * not 0 written with base added (n made even) */
static void plane_put(uint16_t plane, uint16_t off, int16_t h, uint16_t n, Far src, uint8_t base, uint16_t pitch)
{
    uint16_t seg = NW(screen_seg, 0), di = off, si = src.off, c;
    uint8_t dskip, sskip;

    vga_outw(0x3C4, (uint16_t)((1 << plane) << 8 | 2));
    if (!n)
        return;
    n = (uint16_t)((n >> 1) + (n & 1));
    dskip = (uint8_t)(NW(screen_pitch, 0) - n * 2);
    sskip = (uint8_t)(pitch - n);
    while (--h >= 0) {
        for (c = n; c; c--) {
            uint8_t b = frb(src.seg, si++);

            if (b & 0xF0)
                vwb(seg, di, (uint8_t)((b >> 4) + base));
            di++;
            if (b & 0x0F)
                vwb(seg, di, (uint8_t)((b & 0x0F) + base));
            di++;
        }
        di = (uint16_t)(di + dskip);
        si = (uint16_t)(si + sskip);
    }
}

/* S2D47:1695: a rectangle of the buffer shown at x, y on the page drawn,
 * colours from base on */
static void paper_show(Far desc, uint16_t x, uint16_t y, Rect r, uint8_t base)
{
    uint16_t p0 = x & 3, off, src, di = r.x0 & 3, pitch = pw(desc, 0x10);
    int16_t si, wd = (int16_t)(r.x1 - r.x0), h = (int16_t)(r.y1 - r.y0);

    off = (uint16_t)(y * pw(scr(), 0x216) + ((int16_t)x >> 2));
    src = (uint16_t)(r.y0 * pitch + (r.x0 >> 2));
    if (wd <= 0 || h <= 0)
        return;
    for (si = (int16_t)p0; (int16_t)(p0 + 3) >= si; si++) {
        uint16_t n = (uint16_t)(wd >> 2);
        Far pl = pfar(desc, (uint16_t)((di & 3) * 4));

        if ((uint16_t)(wd & 3) > (di & 3))
            n++;
        di++;
        plane_put((uint16_t)(si & 3), off, h, n, far_off(pl, src), base, pitch);
    }
}

/* S2B76:0004 and S294A:000A (screen_keep, screen_put) as fightpics.c has
 * them, which keeps them to itself: h rows of w bytes of each plane of the
 * screen at src kept at `to` one plane after the other, and back */
static void keep(Far src, Far to, uint16_t w, uint16_t h)
{
    uint16_t pitch = NW(screen_pitch, 0), si = src.off, di = to.off, plane, r, k;

    for (plane = 0; plane < 4; plane++) {
        uint16_t s = si;

        vga_outw(0x3CE, (uint16_t)(plane << 8 | 4));
        for (r = h; r; r--) {
            uint16_t row = s;

            for (k = 0; k < w; k++)
                vwb(to.seg, di++, vrb(src.seg, s++));
            s = (uint16_t)(row + pitch);
        }
    }
}

static void put_back(Far from, Far dst, uint16_t w, uint16_t h)
{
    uint16_t pitch = NW(screen_pitch, 0), s = from.off, plane, r, k;
    uint8_t m = 0x11;

    for (plane = 0; plane < 4; plane++) {
        uint16_t d = dst.off;

        vga_outw(0x3C4, (uint16_t)(m << 8 | 2));
        for (r = h; r; r--) {
            uint16_t row = d;

            for (k = 0; k < w; k++)
                vwb(dst.seg, d++, vrb(from.seg, s++));
            d = (uint16_t)(row + pitch);
        }
        m = (uint8_t)(m << 1 | m >> 7);
    }
}

/* S2D47:0CF6: the place of the next line a line further on (the font's
 * height and the gap, word 0Ch), the cleared part's end too, kept within
 * the buffer */
static void line_next(Far w)
{
    uint16_t lh = (uint16_t)(pw(pfar(w, 0xE6), 0x410) + pw(w, 0x0C));

    pww(w, 0x88, (uint16_t)(pw(w, 0x88) + lh));
    pww(w, 0x8C, (uint16_t)(pw(w, 0x8C) + lh));
    if (pw(w, 0x8C) > pw(w, 0x84))
        pww(w, 0x8C, (uint16_t)(pw(w, 0x8C) - lh));
}

/* S2D47:0473: the next line of the text set into the buffer at the row of
 * word 88h.  The text's codes: '|' a place where a word may be broken
 * (with '-'), CR and LF a space (but at a line's beginning), "^Fn" font
 * n (0 to 6), "^Ac" the alignment c ('L', 'R', 'C', 'B' both sides), "^Nn"
 * n empty rows, "^Rn" n returned (the newspaper's: 9999 the end, 9100 to
 * 9102 where the next picture goes, 9200 on a picture).  The pictures
 * being shown (bytes 14h to 19h of the window) take their widths off the
 * line.  A line that does not fit is broken at its last space or '|'; the
 * text's end begins it again. */
static uint16_t paper_line(Far w)
{
    Far p = pfar(w, 4), font = pfar(w, 0xE6);
    uint8_t buf[0x60] = { 0 }, *line = buf + 1, c, align = pb(w, 0x0E);
    int16_t si = 0, prev2 = 0, prev = 0, brk = 0, brk2 = 0, last = 0, x = 0, state = 0, n = 0, di;
    int16_t sp = (int16_t)pw(font, 0x414), avail = (int16_t)(pw(w, 0xA8) - 8);
    uint16_t ret = 0;
    int32_t gap = 0;
    int16_t start = 0, acc;
    Rect r;

    c = pb(p, 0);
    p.off++;
    for (di = 0; di < 6; di++)
        if ((int16_t)pw(w, (uint16_t)(0x20 + di * 2)) > 0) {
            pwb(w, (uint16_t)(0x14 + di), 1);
            avail = (int16_t)(avail - (pw(pfar(w, (uint16_t)(0x50 + di * 4)), 0x0E) + sp * 2));
        }
    if ((uint16_t)(pw(w, 0x8C) - pw(w, 0x88)) <= pw(font, 0x410)) {
        uint16_t t = pw(w, 0x9E);

        pww(w, 0x88, 0);
        pww(w, 0x8C, pw(w, 0xAA));
        pww(w, 0x9E, pw(w, 0xA0));
        pww(w, 0xA0, t);
    }
    r.x0 = pw(w, 0x86);
    r.y0 = pw(w, 0x88);
    r.x1 = pw(w, 0x8A);
    r.y1 = (uint16_t)(pw(w, 0x88) + pw(font, 0x410) + pw(w, 0x0C));
    paper_clear(far_off(w, 0x68), r, 0);
    if (pw(w, 0x10) & 1) {
        if (!pw(w, 0x12))
            pww(w, 0x10, (uint16_t)(pw(w, 0x10) & ~1));
        else {
            pww(w, 0x88, (uint16_t)(pw(w, 0x88) + 1));
            pww(w, 0x8C, (uint16_t)(pw(w, 0x8C) + 1));
            if (pw(w, 0x8C) > pw(w, 0x84))
                pww(w, 0x8C, (uint16_t)(pw(w, 0x8C) - 1));
            pww(w, 0x12, (uint16_t)(pw(w, 0x12) - 1));
            return 0;
        }
    }
    while (!state) {
        uint32_t end = ((uint32_t)pw(w, 2) << 16 | pw(w, 0)) + ((uint32_t)pw(w, 0x0A) << 16 | pw(w, 8));
        int check = 1;

        if (end <= ((uint32_t)p.seg << 16 | p.off)) {
            state = 1;
            pwb(w, 0x0E, 'L');
            check = 0;
        } else if (si >= 0x50) {
            state = 2;
            check = 0;
        } else if (c == '^') {
            uint8_t c2 = pb(p, 0);

            p.off++;
            switch (c2) {
            case 'F':
                n = 0;
                while (pb(p, 0) >= '0' && pb(p, 0) <= '9') {
                    n = (int16_t)(n * 10 + pb(p, 0) - '0');
                    p.off++;
                }
                p.off--;
                if (n < 0 || n > 6)
                    n = 0;
                state = 5;
                break;
            case 'A':
                align = pb(p, 0);
                state = 6;
                break;
            case 'N':
                {
                    int16_t v = 0;

                    while (pb(p, 0) >= '0' && pb(p, 0) <= '9') {
                        v = (int16_t)(v * 10 + pb(p, 0) - '0');
                        p.off++;
                    }
                    p.off--;
                    if (v)
                        pww(w, 0x12, (uint16_t)v);
                    state = 7;
                    if (pb(w, 0x0E) == 'B') {
                        align = 'B';
                        pwb(w, 0x0E, 'L');
                    }
                }
                break;
            case 'R':
                ret = 0;
                while (pb(p, 0) >= '0' && pb(p, 0) <= '9') {
                    ret = (uint16_t)(ret * 10 + pb(p, 0) - '0');
                    p.off++;
                }
                break;
            default:
                state = 0xFF;
                break;
            }
        } else if (c == '|') {
            if (si > 0) {
                brk2 = brk;
                brk = (int16_t)(si - 1);
                prev2 = prev;
                prev = x;
            }
        } else if (c == ' ' || ((c == 0x0A || c == 0x0D) && si >= 2)) {
            while (line[si - 1] == ' ') {
                x = (int16_t)(x - (sp + (int16_t)pw(font, 0x412)));
                si--;
            }
            line[si++] = ' ';
            last = (int16_t)(si - 2);
            prev2 = prev;
            prev = x;
            x = (int16_t)(x + sp + (int16_t)pw(font, 0x412));
        } else if (c == 0x0A || c == 0x0D) {
            /* at a line's beginning: nothing */
        } else if (pw(font, (uint16_t)(0x0E + c * 2)) != 0xFFFF) {
            line[si++] = c;
            x = (int16_t)(x + sp + (int16_t)pw(font, (uint16_t)(0x20E + c * 2)));
        } else if (pw(font, 0x6A) == 0xFFFF) {
            line[si++] = ' ';
            x = (int16_t)(x + sp + (int16_t)pw(font, 0x412));
        } else {
            line[si++] = '.';
            x = (int16_t)(x + sp + (int16_t)pw(font, 0x26A));
        }
        if (check && x >= avail)
            state = 3;
        c = pb(p, 0);
        p.off++;
    }
    if (si > 0) {
        si--;
        switch (state) {
        case 1:
            pwb(w, 0x0E, 'L');
            break;
        case 2:
        case 3:
            di = 0;
            while (si > brk && si > last) {
                si--;
                di++;
            }
            x = prev;
            if (si == brk) {
                si++;
                line[si] = '-';
                di++;
                x = (int16_t)(x + sp + (int16_t)pw(font, 0x268));
                if (x >= avail) {
                    while (si > brk2 && si > last) {
                        si--;
                        di++;
                    }
                    x = prev2;
                    if (si == brk2) {
                        di++;
                        si++;
                        line[si] = '-';
                    }
                }
            }
            p.off = (uint16_t)(p.off - di);
            break;
        case 5:
        case 6:
        case 7:
            while (si >= 0 && line[si] == ' ') {
                x = (int16_t)(x - (sp + (int16_t)pw(font, (uint16_t)(0x20E + line[si] * 2))));
                si--;
            }
            break;
        default:
            break;
        }
        if (pw(w, 0x10) & 2) {
            if (pb(w, 0x0E) == 'B') {
                align = 'B';
                pwb(w, 0x0E, 'R');
                state = 6;
            }
            pww(w, 0x10, (uint16_t)(pw(w, 0x10) & ~2));
        }
        /* the widths of the pictures on the left (their word 0Eh and twice
         * the gap) */
        acc = 0;
        for (di = 0; di < 6; di++)
            if ((int16_t)pw(w, (uint16_t)(0x20 + di * 2)) > 0 && pb(w, (uint16_t)(0x1A + di)) == 'L')
                acc = (int16_t)(acc + pw(pfar(w, (uint16_t)(0x50 + di * 4)), 0x0E) + sp * 2);
        switch (pb(w, 0x0E)) {
        case 'B':
            /* both sides: the room left spread over the gaps, in 16.16
             * (the original divides by si without testing it for 0) */
            gap = si ? (int32_t)((uint32_t)(int32_t)(avail - x) << 16) / si : 0;
            gap += (int32_t)((uint32_t)sp << 16);
            start = acc;
            break;
        case 'C':
            gap = (int32_t)((uint32_t)sp << 16);
            start = (int16_t)((uint16_t)(pw(w, 0xA8) - x) >> 1);
            break;
        case 'L':
            gap = (int32_t)((uint32_t)sp << 16);
            start = acc;
            break;
        case 'R':
            gap = (int32_t)((uint32_t)sp << 16);
            start = (int16_t)(avail - x + acc);
            break;
        default:
            /* the original's are left as they were on its stack */
            break;
        }
        acc = 0;
        for (di = 0; di <= si; di++) {
            uint16_t hi = (uint16_t)((uint32_t)((int32_t)di * gap) >> 16);

            glyph(font, (int16_t)pw(font, (uint16_t)(0x0E + line[di] * 2)), far_off(w, 0x68),
                  (uint16_t)(start + acc + hi), pw(w, 0x88));
            acc = (int16_t)(acc + pw(font, (uint16_t)(0x20E + line[di] * 2)));
        }
        line_next(w);
    }
    pfww(w, 4, p);
    switch (state) {
    case 1:
        pwb(w, 0x0E, 'L');
        pfww(w, 4, pfar(w, 0));
        break;
    case 5:
        pww(w, 0xE6, (uint16_t)(n * 0x416 + 2));
        pww(w, 0xE8, S_FONT);
        break;
    case 6:
        pwb(w, 0x0E, align);
        break;
    case 7:
        pww(w, 0x10, (uint16_t)(pw(w, 0x10) | 1));
        pwb(w, 0x0E, align);
        break;
    default:
        break;
    }
    return ret;
}

/* S2D47:0DD2 (paper_init(win, buf, w, h, extra, x, y, base)): the
 * window's record made for a window of w by h at x, y (x and w multiples
 * of 8; w 8 more inside), the buffer `extra` rows higher, at buf: its
 * four planes (far words 68h, bytes a row 78h, width 7Ah, height 7Ch),
 * then two places for what lies under the window on each page (B0h and
 * C6h: four planes, bytes a row, width, height), the rectangles of the
 * buffer shown (8Eh and 96h: x0, y0, x1, y1; 9Eh and A0h which of them
 * comes first), the next line's row (88h; the cleared part 86h to 8Ch),
 * the buffer's whole (7Eh to 84h), the window's place (A2h, A4h; on the
 * screen A6h, and as far words of both pages DCh, E0h), the width A8h,
 * the rows below the window AAh, the colours' base E5h, the font E6h
 * (font 0), the line's gap 0Ch (1). */
static int16_t paper_init(Far w, Far buf, uint16_t wd, uint16_t h, uint16_t extra, uint16_t x, uint16_t y, uint8_t base)
{
    uint16_t si = wd, bpr, size, di, k;
    Far f0;
    Rect all;

    if (si & 7)
        si = (uint16_t)(si + 8 - (si & 7));
    if (x & 7)
        x = (uint16_t)(x + 8 - (x & 7));
    if ((uint16_t)(x + si) > 0x140)
        return -1;
    if (!(buf.seg | buf.off)) {
        fprintf(stderr, "paper_init: no buffer given, mem_alloc not translated here\n");
        return -1;
    }
    si = (uint16_t)(si + 8);
    pww(w, 0xE6, 2);
    pww(w, 0xE8, S_FONT);
    pww(w, 0x0C, 1);
    pww(w, 0x86, 0);
    pww(w, 0x88, h);
    pww(w, 0x8A, si);
    pww(w, 0x8C, (uint16_t)(h + extra));
    pww(w, 0xA8, si);
    pww(w, 0xAA, extra);
    pww(w, 0x7E, 0);
    pww(w, 0x80, 0);
    pww(w, 0x82, si);
    pww(w, 0x84, (uint16_t)(h + extra));
    pww(w, 0xA2, x);
    pww(w, 0xA4, y);
    pfww(w, 4, pfar(w, 0));
    pww(w, 0xA6, (uint16_t)(y * pw(scr(), 0x216) + (x >> 2)));
    for (di = 0; di < 6; di++) {
        pwb(w, (uint16_t)(0x14 + di), 0);
        pww(w, (uint16_t)(0x20 + di * 2), 0);
    }
    bpr = (uint16_t)((si >> 3) + (si & 1));
    size = (uint16_t)((h + extra) * bpr);
    pww(w, 0x78, bpr);
    pww(w, 0x7A, si);
    pww(w, 0x7C, (uint16_t)(h + extra));
    for (di = 0; di < 4; di++) {
        pfww(w, (uint16_t)(0x68 + di * 4), buf);
        fill(buf, 0, size);
        buf = far_add(buf, size);
    }
    bpr = (uint16_t)((si >> 2) + (si & 1));
    size = (uint16_t)(bpr * h);
    for (k = 0; k < 2; k++) {
        uint16_t o = (uint16_t)(k * 0x16);

        pww(w, (uint16_t)(0xC0 + o), bpr);
        pww(w, (uint16_t)(0xC2 + o), si);
        pww(w, (uint16_t)(0xC4 + o), h);
        for (di = 0; di < 4; di++) {
            pfww(w, (uint16_t)(0xB0 + o + di * 4), buf);
            buf = far_add(buf, size);
        }
        pww(w, (uint16_t)(0x8E + k * 8), 0);
        pww(w, (uint16_t)(0x90 + k * 8), 0);
        pww(w, (uint16_t)(0x92 + k * 8), si);
        pww(w, (uint16_t)(0x94 + k * 8), h);
    }
    f0.seg = pw(scr(), 0x27A);
    f0.off = pw(w, 0xA6);
    pfww(w, 0xDC, f0);
    f0.seg = pw(scr(), 0x27C);
    pfww(w, 0xE0, f0);
    pwb(w, 0xE4, 0);
    pww(w, 0x9C, 0);
    pww(w, 0x9E, 1);
    pww(w, 0xA0, 0);
    pwb(w, 0xE5, base);
    all = rect_at(w, 0x7E);
    paper_clear(far_off(w, 0x68), all, 0);
    return 0;
}

/* which page is drawn: 0 when it is the one of word DCh */
static uint16_t paper_page(Far w)
{
    return pw(scr(), 0x27A) == pw(w, 0xDE) && pw(w, 0xA6) == pw(w, 0xDC) ? 0 : 1;
}

/* S2D47:1159: what lay under the window put back on the page drawn */
static void paper_restore(Far w)
{
    uint16_t i = paper_page(w), o = (uint16_t)(i * 0x16);

    put_back(pfar(w, (uint16_t)(0xB0 + o)), pfar(w, (uint16_t)(0xDC + i * 4)), pw(w, (uint16_t)(0xC0 + o)), pw(w, (uint16_t)(0xC4 + o)));
}

/* S2D47:0356 (paper_picture(win, pic, base, align)): a picture added to
 * the first free place of six, coming up from below the window: on the
 * left, on the right (8 from the edge) or in the middle (the lines then
 * kept empty for its height and two gaps) */
static int16_t paper_picture(Far w, Far pic, uint16_t base, uint16_t align)
{
    uint16_t cx = 0;

    while (cx <= 6 && pb(w, (uint16_t)(0x14 + cx)))
        cx++;
    if (cx == 6)
        return -1;
    pww(w, (uint16_t)(0x44 + cx * 2), base);
    pww(w, (uint16_t)(0x20 + cx * 2), pw(pic, 0x10));
    pwb(w, (uint16_t)(0x1A + cx), (uint8_t)align);
    pfww(w, (uint16_t)(0x50 + cx * 4), pic);
    pww(w, (uint16_t)(0x2C + cx * 2), (uint16_t)(pw(w, 0x84) - pw(w, 0xAA)));
    switch (align) {
    case 'L':
        pww(w, (uint16_t)(0x38 + cx * 2), pw(w, 0x7E));
        break;
    case 'C':
        pww(w, (uint16_t)(0x38 + cx * 2), (uint16_t)((uint16_t)(pw(w, 0x7E) + pw(w, 0x82) - pw(pic, 0x0E)) >> 1));
        pww(w, 0x12, (uint16_t)(pw(w, 0x0C) * 2 + pw(pic, 0x10)));
        pww(w, 0x10, (uint16_t)(pw(w, 0x10) | 1));
        break;
    case 'R':
        pww(w, (uint16_t)(0x38 + cx * 2), (uint16_t)(pw(w, 0x82) - pw(pic, 0x0E) - 8));
        break;
    }
    return 0;
}

/* S2D47:1208 (paper_step(win, 0, step)): lines set until the buffer
 * reaches `step` rows below the part shown, the part moved on by step
 * (the buffer used round: two rectangles), what lies under the window
 * kept (screen_keep), the pictures drawn (cut to the window's rows, the
 * screen's record's words 14h and 18h, 0Eh FFFFh) and moved up, the
 * text over them.  Returns the last "^R" code met, 0 none. */
static int16_t paper_step(Far w, int16_t a, int16_t step)
{
    Far r = scr();
    uint16_t ret = 0, st = (uint16_t)step, i, o;
    int16_t di;

    if (a < 0 || step < 0)
        return -1;
    for (di = 0; di < 6; di++)
        if (pw(w, (uint16_t)(0x20 + di * 2)) && pb(w, (uint16_t)(0x1A + di)) == 'C')
            pww(w, 0x10, (uint16_t)(pw(w, 0x10) | 1));
    while ((uint16_t)(pw(w, (uint16_t)(0x94 + pw(w, 0xA0) * 8)) + st) > pw(w, 0x88)) {
        uint16_t v = paper_line(w);

        if (v)
            ret = v;
    }
    {
        uint16_t a0 = (uint16_t)(pw(w, 0xA0) * 8), e = (uint16_t)(pw(w, 0x9E) * 8);

        pww(w, (uint16_t)(0x94 + a0), (uint16_t)(pw(w, (uint16_t)(0x94 + a0)) + st));
        if (pw(w, (uint16_t)(0x94 + e)) < pw(w, 0x88))
            pww(w, (uint16_t)(0x90 + a0), (uint16_t)(pw(w, (uint16_t)(0x90 + a0)) + st));
        else {
            pww(w, (uint16_t)(0x90 + e), (uint16_t)(pw(w, (uint16_t)(0x90 + e)) + st));
            if (pw(w, (uint16_t)(0x90 + e)) >= pw(w, (uint16_t)(0x94 + e))) {
                pww(w, 0x8C, pw(w, 0x84));
                pww(w, (uint16_t)(0x90 + a0), (uint16_t)(pw(w, (uint16_t)(0x90 + e)) - pw(w, (uint16_t)(0x94 + e))));
                pww(w, (uint16_t)(0x90 + e), 0);
                pww(w, (uint16_t)(0x94 + e), 0);
            }
        }
    }
    i = paper_page(w);
    o = (uint16_t)(i * 0x16);
    keep(pfar(w, (uint16_t)(0xDC + i * 4)), pfar(w, (uint16_t)(0xB0 + o)), pw(w, (uint16_t)(0xC0 + o)), pw(w, (uint16_t)(0xC4 + o)));
    pwb(w, 0xE4, (uint8_t)(pb(w, 0xE4) ^ 1));
    for (di = 0; di < 6; di++) {
        uint16_t y0, y1, e0;
        Far pic;

        if (!pb(w, (uint16_t)(0x14 + di)))
            continue;
        y0 = pw(r, 0x14);
        y1 = pw(r, 0x18);
        e0 = pw(r, 0x0E);
        pww(r, 0x14, (uint16_t)(pw(w, 0x80) + pw(w, 0xA4)));
        pww(r, 0x18, (uint16_t)(pw(w, 0x84) - pw(w, 0xAA) + pw(w, 0xA4)));
        pww(r, 0x0E, 0xFFFF);
        pic = pfar(w, (uint16_t)(0x50 + di * 4));
        pic_draw((uint16_t)(pw(w, (uint16_t)(0x38 + di * 2)) + pw(w, 0xA2)),
                 (uint16_t)(pw(w, (uint16_t)(0x2C + di * 2)) + pw(w, 0xA4)), pic,
                 (uint8_t)pw(w, (uint16_t)(0x44 + di * 2)));
        pww(w, (uint16_t)(0x20 + di * 2), (uint16_t)(pw(w, (uint16_t)(0x20 + di * 2)) - st));
        pww(w, (uint16_t)(0x2C + di * 2), (uint16_t)(pw(w, (uint16_t)(0x2C + di * 2)) - st));
        if ((int16_t)pw(w, (uint16_t)(0x2C + di * 2)) <= -(int16_t)pw(pic, 0x10)) {
            pwb(w, (uint16_t)(0x14 + di), 0);
            pww(w, (uint16_t)(0x20 + di * 2), 0);
        }
        pww(r, 0x14, y0);
        pww(r, 0x18, y1);
        pww(r, 0x0E, e0);
    }
    {
        Rect first = rect_at(w, (uint16_t)(0x8E + pw(w, 0x9E) * 8)), second = rect_at(w, (uint16_t)(0x8E + pw(w, 0xA0) * 8));

        paper_show(far_off(w, 0x68), pw(w, 0xA2), pw(w, 0xA4), first, pb(w, 0xE5));
        paper_show(far_off(w, 0x68), pw(w, 0xA2), (uint16_t)(pw(w, 0xA4) + first.y1 - first.y0), second, pb(w, 0xE5));
    }
    return (int16_t)ret;
}

/* S1A39:22D3: the digits of a newspaper's record put into the names of
 * its files: bytes 0 and 1 the text's and pictures' (TXT\aabb.TXT,
 * TXT\aabb.DAT), byte 2 the background's (TXT\Hcc.LBM) */
static void paper_names(uint16_t n)
{
    uint16_t rec = (uint16_t)(0x05A0 + n * 6);
    uint8_t a = frb(S_EV, rec), b = frb(S_EV, (uint16_t)(rec + 1)), c = frb(S_EV, (uint16_t)(rec + 2));

    fwb(S_EV, 0x04D8, (uint8_t)(a / 10 + '0'));
    fwb(S_EV, 0x04D9, (uint8_t)(a % 10 + '0'));
    fwb(S_EV, 0x04E5, (uint8_t)(a / 10 + '0'));
    fwb(S_EV, 0x04E6, (uint8_t)(a % 10 + '0'));
    fwb(S_EV, 0x04DA, (uint8_t)(b / 10 + '0'));
    fwb(S_EV, 0x04DB, (uint8_t)(b % 10 + '0'));
    fwb(S_EV, 0x04E7, (uint8_t)(b / 10 + '0'));
    fwb(S_EV, 0x04E8, (uint8_t)(b % 10 + '0'));
    fwb(S_EV, 0x04F3, (uint8_t)(c / 10 + '0'));
    fwb(S_EV, 0x04F4, (uint8_t)(c % 10 + '0'));
}

/* S1A39:059D (newspaper(place, work, n)): page n of the newspapers (two
 * for each map; its record of six bytes at S3812:05A0: the names'
 * digits, flags (4: a library of pictures), a picture of event_picture
 * shown first (0 none), the tune (TXT\ZWTXT1.ADL and on, 16 bytes a
 * name)).  The three fonts (TIMES_8, TIMES_10, TIMES_12 .NEW and .CHR),
 * the background (shown, both pages), the palette TXT\ZWTXT.PAL, the
 * tune, the text and the pictures loaded; the window 118h by B4h at 14h,
 * 0Ah, colours from 10h; the first lines set (twice, page_flip between),
 * the palette made the game's, fade_in, music_start(tune, 14h, FFh).
 * Then a row a frame of 8 timer counts, the key's word 4 cleared at each
 * frame's beginning: the text's codes 9100 to 9102 choose where the next
 * picture goes (left, middle, right; 'L' for the others to 9199), 9200
 * on picture n - 9200 of the library (base 0), 9999 the end, as does the
 * key of entry 0 of the table at S331F:02D3 (presumably Esc; not
 * checked); the key of entry 1 waits for a key (inputs_release(1, 1)).
 * At the end the window's background put back, wait_frames(32h),
 * fade_out, music_fade(14h). */
static int16_t paper(Far place, Far work, uint16_t n)
{
    uint16_t rec = (uint16_t)(0x05A0 + n * 6), align = 'R', flags = frb(S_EV, (uint16_t)(rec + 3)), di;
    Far w = ev(0x0D14), pal, tune, keys;
    int32_t size;
    int16_t si;

    if (frb(S_EV, (uint16_t)(rec + 4)))
        event_picture(place, work, frb(S_EV, (uint16_t)(rec + 4)), 0);
    for (di = 0; di < 3; di++) {
        size = font_load(ev((uint16_t)(0x0528 + di * 0x14)), ev((uint16_t)(0x0564 + di * 0x14)), (int16_t)di, 1, place, work);
        if (size < 0)
            return -1;
        place = far_add(place, (uint32_t)size);
    }
    paper_names(n);
    if (!lbm_show(place, ev(0x04EE), work))
        return -1;
    flip();
    page_copy();
    pal = place;
    size = file_load_size(place, ev(0x051A), work);
    if (size == -1)
        return -1;
    place = far_add(place, (uint32_t)size);
    tune = place;
    size = file_load_size(place, ev((uint16_t)(0x04FA + frb(S_EV, (uint16_t)(rec + 5)) * 16)), work);
    if (size == -1)
        return -1;
    place = far_add(place, (uint32_t)size);
    size = paper_text(ev(0x04D4), w, place, work);
    if (size < 0)
        return -1;
    place = far_add(place, (uint32_t)size);
    if (flags & 4) {
        size = lib_load(ev(0x04E1), place, work, ev(0x04C0), 0);
        if (size == -1)
            return -1;
        place = far_add(place, (uint32_t)size);
    }
    /* S2D47:0D65, the buffer's size for these, is called and its result
     * not used */
    if (paper_init(w, place, 0x118, 0xB4, 0x30, 0x14, 0x0A, 0x10) < 0)
        return -1;
    hook(EV_BUSY, EV_LOAD_PAPER);
    if (paper_step(w, 0, 0) < 0)
        return -1;
    flip();
    if (paper_step(w, 0, 0) < 0)
        return -1;
    far_copy(pal, far_at(S_DATA, A_palette_ptr), 0x300);
    fade(1);
    music_start(tune, 0x14, 0xFF);
    keys = far_at(HL_S331F + HL_BASE, 0x02D3);
    for (;;) {
        Far k = far_at(S_DATA, 0x00BE);

        /* the frame's count set to 0 (no frame of the original's log
         * marks it) */
        hook(EV_PAPER, 0);
        event_frame++;
        pww(scr(), 0x22E, 0);
        pww(k, 4, 0);
        paper_restore(w);
        si = paper_step(w, 0, 1);
        if (si == 0x270F)
            break;
        if (si >= 0x238C && si < 0x23EF) {
            switch (si) {
            case 0x238C:
                align = 'L';
                break;
            case 0x238D:
                align = 'C';
                break;
            case 0x238E:
                align = 'R';
                break;
            default:
                align = 'L';
                break;
            }
        } else if (si >= 0x23F0 && si < 0x2453) {
            Far pics = far_at(S_EV, 0x04CE);

            paper_picture(w, pfar(pics, (uint16_t)((si - 0x23F0) * 4)), 0, align);
        }
        flip();
        frame_end(8);
        k = far_at(S_DATA, 0x00BE);
        if (pw(k, 4)) {
            if (pb(k, 0) == pb(keys, 0))
                break;
            if (pb(keys, 1) == pb(k, 0) && !hook(EV_PAUSE, 0))
                break;
        }
    }
    paper_restore(w);
    wait_frames(0x32);
    fade(0);
    music_fade(0x14);
    hook(EV_BUSY, EV_PAPER_END);
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
    case 0xCA:
    case 0xCB:
        res = pic_site(place, work, kind == 0xCB ? 0xA0 : 0, 0);
        break;
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
    case 0xC8:
        res = pic_richt(place, work);
        break;
    case 0xC9:
        res = pic_zeppelin(place, work);
        break;
    case 0xCC:
        res = pic_ship(place, work);
        break;
    default:
        /* the newspapers: the maps 0 to 17h and 18h to 2Fh share them, two
         * pages each.  The original tests its SI (not set here: its
         * caller's) for below 0 before them; taken as not */
        {
            uint16_t n = 0xFFFF;

            if ((int16_t)kind >= 0 && (int16_t)kind <= 0x17)
                n = kind;
            else if ((int16_t)kind >= 0x18 && (int16_t)kind <= 0x2F)
                n = (uint16_t)(kind - 0x18);
            res = paper(place, work, (uint16_t)(n * 2));
            res = paper(place, work, (uint16_t)(n * 2 + 1));
        }
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
