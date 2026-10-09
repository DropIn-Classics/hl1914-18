/* title.c - HL.EXE's logo screens at its start (the end of S1650:0003,
 * which battle.c's start_setup translates up to them): the publisher's
 * animated logo and the game's title picture, each until a key, a
 * button or a click, or about seven seconds.  Played on the port's clock
 * (play.c) in the window when the player's setup has the intro on, or
 * with -title; battle.c calls it through its hook start_title, so the
 * standalone tests that link battle.c need none of this.  port/README.md
 * says how it was compared with the original.
 */
#include "hl.h"

#define S_DATA (HL_DATA + HL_BASE)

static Far at(uint16_t seg, uint16_t off)
{
    Far p;

    p.seg = seg;
    p.off = off;
    return p;
}

static void pww(Far p, uint16_t o, uint16_t v) { fww(p.seg, (uint16_t)(p.off + o), v); }

/* S0CB6:18F1 (wait_span(min, max)): the counts of keys (word 4 of the
 * record DATA:00BE points to), of both sticks' buttons (byte 4 of
 * DATA:00A6's and 00B6's) and of the mouse's buttons (words 30h and 34h
 * of DATA:00C2's) cleared, then min of the timer's counts (word 22Eh of
 * the screen's record) waited, then until one of them is not 0 or more
 * than max more have gone by.  The port stops waiting too when the
 * window is closed. */
static void wait_span(int16_t min, uint16_t max)
{
    Far scr = far_at(S_DATA, 0x0092), k = far_at(S_DATA, 0x00BE), ms = far_at(S_DATA, 0x00C2);
    Far j1 = far_at(S_DATA, 0x00A6), j2 = far_at(S_DATA, 0x00B6);

    pww(k, 4, 0);
    pwb(j1, 4, 0);
    pwb(j2, 4, 0);
    pww(ms, 0x30, 0);
    pww(ms, 0x34, 0);
    pww(scr, 0x22E, 0);
    while ((int16_t)pw(scr, 0x22E) < min && !play_stopped())
        play_tick();
    pww(scr, 0x22E, 0);
    while (!pw(k, 4) && !pb(j1, 4) && !pb(j2, 4) && !pw(ms, 0x30) && !pw(ms, 0x34)
           && pw(scr, 0x22E) <= max && !play_stopped())
        play_tick();
}

/* What S2AAF:0000 (pic_draw's way for a picture of kind 'U', all of
 * BB.DAT's) does with a save place before it draws: with the screen's cut
 * on (DATA:00E8) the picture's rectangle (x, y moved by its words 0Ah and
 * 0Ch, width 0Eh, height 10h) cut by S2AAF:0107 to DATA:00EC-00F2 (left,
 * top, right, bottom), nothing when it lies outside, and what the screen
 * has there kept by under_save, which unders_restore puts back.  The
 * port's pic_blit_u (draw.c) leaves the save place out, so title.c does
 * this step itself before pic_draw_save. */
static void title_save(uint16_t x, uint16_t y, Far pic, Far save)
{
    int16_t sx = (int16_t)(x + pw(pic, 0x0A)), sy = (int16_t)(y + pw(pic, 0x0C));
    int16_t w = (int16_t)pw(pic, 0x0E), h = (int16_t)pw(pic, 0x10);
    int16_t l = (int16_t)frw(S_DATA, 0x00EC), t = (int16_t)frw(S_DATA, 0x00EE);
    int16_t r = (int16_t)frw(S_DATA, 0x00F0), b = (int16_t)frw(S_DATA, 0x00F2);
    Far scr;

    if (sx < l) {
        if ((w = (int16_t)(w - (l - sx))) <= 0)
            return;
        sx = l;
    } else if (sx > r)
        return;
    if (sy < t) {
        if ((h = (int16_t)(h - (t - sy))) <= 0)
            return;
        sy = t;
    } else if (sy > b)
        return;
    if (sy + h > b && !(h = (int16_t)(b - sy)))
        return;
    if (sx + w > r && !(w = (int16_t)(r - sx)))
        return;
    scr.seg = NW(screen_seg, 0);
    scr.off = (uint16_t)(((uint16_t)sx >> 2) + frw(S_DATA, (uint16_t)(0x00F8 + 2 * sy)));
    under_save((uint16_t)w, (uint16_t)h, scr, save);
}

/* page_flip waits for a retrace's beginning */
static void title_flip(void)
{
    page_flip();
    play_retraces(1);
}

/* S28A5:000C with a place (the way S1650:0003 calls it): the screen's
 * cut (DATA:00E8) set to FFFFh at its beginning and again at its end, so
 * that the frames above the screen's top (y FFFDh) are cut, not drawn
 * from a wrapped place; the file `name`.DAT loaded to `place`, `name`.IFF shown behind it, its colours
 * set at once (pal_level FFh of DATA:04E3), then the 43h frames of the
 * table at S28A5:01AD (6 bytes each: x, y, the picture's offset in the
 * .DAT) each drawn at x + 3, y - 1 with what it covers kept (save place
 * 0:1, the list at DATA:0322), the page flipped and what was covered put
 * back.  The original makes both names first (the .IFF's copied to its
 * stack); the port makes each when it is used.  The table is read from
 * HL.EXE's image. */
static void title_show(Far place, Far name, Far work)
{
    Far pics = place;
    int32_t size = file_load_size(place, file_name(1, -1, name, 5), work);
    uint16_t i, t, cs = HL_S28A5 + HL_BASE;

    fww(S_DATA, 0x00E8, 0xFFFF);
    place = far_add(place, (uint32_t)size);
    lbm_show(place, file_name(1, -1, name, 0x0A), work);
    title_flip();
    pal_level(0xFF, at(S_DATA, 0x04E3));
    play_level();
    page_copy();
    for (i = 0, t = 0x01AD; i < 0x43; i++, t = (uint16_t)(t + 6)) {
        uint16_t x = (uint16_t)(frw(cs, t) + 3), y = (uint16_t)(frw(cs, (uint16_t)(t + 2)) - 1);
        Far pic = ptr_norm(far_add(pics, frw(cs, (uint16_t)(t + 4))));

        title_save(x, y, pic, at(0, 1));
        pic_draw_save(x, y, pic, at(0, 1), 0);
        title_flip();
        unders_restore();
    }
    title_flip();
    unders_restore();
    fww(S_DATA, 0x00E8, 0xFFFF);
}

/* the end of S1650:0003, after the keys' table: the publisher's logo
 * (BB, the name at S37FE:000E) by title_show on the place 2710h past the
 * block's free part, at most 1F4h of the timer's counts waited (any key,
 * button or click ends it), a fade_out, HLLOGO.IFF (S37FE:0011) faded in
 * and waited for the same way, a fade_out.  The far argument S1650:0003
 * gets is taken to be game_mem.p (not checked). */
void title_screens(void)
{
    title_show(far_add(game_mem.p, 0x2710), at(HL_S37FE + HL_BASE, 0x000E), game_mem.p);
    wait_span(0, 0x1F4);
    fade_out();
    play_fade();
    unders_restore();
    title_flip();
    unders_restore();
    lbm_show(far_add(game_mem.p, 0x2710), file_name(1, -1, at(HL_S37FE + HL_BASE, 0x0011), 0x0A), game_mem.p);
    title_flip();
    fade_in();
    play_fade();
    wait_span(0, 0x1F4);
    fade_out();
    play_fade();
}

