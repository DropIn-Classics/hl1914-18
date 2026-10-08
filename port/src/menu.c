/* menu.c - HL.EXE's menu (S167A:0005, menu_run), which game_loop calls at
 * S066A:0349 before every battle: its loads, the walk through its pages
 * and items (they are records of MNUDATA.DAT), the map's code typed, the
 * choice of the country, the best scores and a winner's name, and what it
 * leaves for the battle when it returns.  Translated over the program's
 * memory; the player's input is what the interrupts leave (input.c,
 * mouse.c), the time the port's own clock (play.c).
 *
 * Drawn as the original draws it, but for the mouse's pointer (mouse.c).
 * Left out: the sounds and the music (S2F39:06E3, 096A, 0F87, 1058), and
 * the branch for a machine without a mouse (the port's has one, mouse.c).
 * port/README.md says how this was compared with the original.
 */
#include <stdio.h>
#include <stdlib.h>
#include "hl.h"
#include "vga.h"

#define S_DATA (HL_DATA + HL_BASE)
#define S_GAME (HL_S331F + HL_BASE)
#define S_OPT (HL_S37FF + HL_BASE)      /* the menu's own DS */

#define ITEM 0xD8u      /* an item's record in MNUDATA.DAT */
#define PAGE 0x34u      /* a page's */

/* what menu_run keeps on its stack */
static struct {
    Far font18, font14, font8, icon, misc, fahn;    /* the libraries' lists */
    Far pages, items, data, texts;
    int16_t px, py;         /* where the pointer goes */
    Far scores, work, pic[3], codes, music, music_hi, p;
    int16_t ncodes;
    int16_t page, item, idx, hl, run, snd;
    Far itp;                /* the item's record the choice is read from */
    uint16_t held[2], mheld;
} m;

const char *menu_stopped;
static FILE *menu_log;

void menu_log_to(const char *path)
{
    menu_log = fopen(path, "w");
}

/* video memory written to a file at the first pass of the menu's loop
 * that begins `at` seconds or later (up to eight) */
static struct { double at; const char *file; } shots[8];
static int shots_n;

void menu_shot(double at, const char *file)
{
    if (shots_n < 8) {
        shots[shots_n].at = at;
        shots[shots_n++].file = file;
    }
}

static Far opt(uint16_t off)
{
    Far p;

    p.seg = S_OPT;
    p.off = off;
    return p;
}

static void pww(Far p, uint16_t o, uint16_t v) { fww(p.seg, (uint16_t)(p.off + o), v); }
static Far key_rec(void) { return far_at(S_DATA, 0x00BE); }
static Far side_rec(int s) { return far_at(S_DATA, s ? 0x00B6 : 0x00A6); }
static Far mouse_rec(void) { return far_at(S_DATA, 0x00C2); }
static Far screen_rec(void) { return far_at(S_DATA, 0x0092); }
static Far page_at(int16_t n) { return far_add(m.pages, (uint32_t)((int32_t)n * (int32_t)PAGE)); }
static Far item_at(int16_t n) { return far_add(m.items, (uint32_t)((int32_t)n * (int32_t)ITEM)); }
static int16_t page_item(Far pg, int16_t i) { return (int8_t)pb(pg, (uint16_t)(0x2C + i)); }
static int16_t page_count(Far pg) { return (int8_t)pb(pg, 0x32); }
static uint8_t key0(uint16_t n) { return pb(far_at(S_GAME, 0x02D3), n); }

/* the port's clock: one call of the timer's routine; the menu stops when
 * it waits and the key script has nothing left */
static void tick(void)
{
    static int idle;

    idle = play_idle() ? idle + 1 : 0;
    if (idle > 400 && !menu_stopped)
        menu_stopped = "the menu waits for input the key script does not have";
    play_tick();
}

static void wait_frames(int n)
{
    play_retraces(n + 1);
}

/* page_flip waits for a retrace's beginning */
static void flip(void)
{
    page_flip();
    play_retraces(1);
}

/* the sounds and the music: left out */
static void sound(int16_t n) { (void)n; }
static void music_start(Far tune) { (void)tune; }
static void music_stop(void) { }

/* ---- what draws ---- */

/* S2A55:0006 (pic_part): a part of one of the three pictures kept in
 * memory (four planes of 50h bytes a line, as screen_keep left them) to
 * the page drawn: plane by plane from the pixel's own, each as many bytes
 * a line as that plane has of the width (S2949:0002), the place from the
 * table of lines (S2955:0002).  The map mask stays on the last plane. */
static void pic_part(Far pic, int16_t sx, int16_t sy, int16_t w, int16_t h, int16_t dx, int16_t dy)
{
    uint16_t rowb = 0x140 >> 2, plane = (uint16_t)(rowb * 0xC8), es = NW(screen_seg, 0), pitch = NW(screen_pitch, 0);
    uint16_t k, r, c;

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
static void pic_whole(Far pic) { pic_part(pic, 0, 0, 0x140, 0xC8, 0, 0); }

/* The menu draws with the screen's cut on (word 0Eh of the screen's
 * record, DATA:00E8, with the whole screen as its rectangle), and the
 * port's pic_draw has only the way without it (the cut one, S2AAF:0107,
 * is not translated).  A picture that lies inside the rectangle is the
 * same either way, so it is drawn with the word off; one that does not is
 * not drawn and the menu stops. */
static void cut_off(int on)
{
    static uint16_t kept;

    if (on) {
        kept = frw(S_DATA, 0x00E8);
        fww(S_DATA, 0x00E8, 0);
    } else {
        fww(S_DATA, 0x00E8, kept);
        /* pic_blit_8 keeps the word in its own segment */
        fww((uint16_t)(HL_S2AAF + HL_BASE), 0x035A, kept);
    }
}

static int uncut;       /* text_mid_uncut draws */

static void menu_pic(int16_t x, int16_t y, Far pic, uint16_t base)
{
    int16_t x0 = (int16_t)(x + pw(pic, 0x0A)), y0 = (int16_t)(y + pw(pic, 0x0C));

    if (uncut) {
        pic_draw((uint16_t)x, (uint16_t)y, pic, (uint8_t)base);
        return;
    }
    if (x0 < (int16_t)frw(S_DATA, 0x00EC) || y0 < (int16_t)frw(S_DATA, 0x00EE)
        || x0 + (int16_t)pw(pic, 0x0E) - 1 > (int16_t)frw(S_DATA, 0x00F0)
        || y0 + (int16_t)pw(pic, 0x10) - 1 > (int16_t)frw(S_DATA, 0x00F2)) {
        if (!menu_stopped)
            menu_stopped = "a picture cut by the screen's edge (S2AAF:0107)";
        return;
    }
    cut_off(1);
    pic_draw((uint16_t)x, (uint16_t)y, pic, (uint8_t)base);
    cut_off(0);
}

/* S167A:28E0 (text_width): the width of a text in a font, `gap` more a
 * character.  A character's picture is the font's entry for it: 0 to 9
 * the digits, 0Ah to 23h the letters (small ones as capitals), 24h on the
 * eleven characters at S37FF:00FC; others are passed over.  Its width is
 * byte 0Eh of the picture. */
static int font_index(uint8_t c)
{
    int i;

    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'z')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'Z')
        return c - 'A' + 10;
    for (i = 0; i < 0x0B; i++)
        if (frb(S_OPT, (uint16_t)(0x00FC + i)) == c)
            return 0x24 + i;
    return -1;
}

static int16_t width_of(const uint8_t *text, Far font, int16_t gap)
{
    int16_t w = 0;

    for (; *text; text++) {
        int n = font_index(*text);

        if (n >= 0)
            w = (int16_t)(w + pb(pfar(font, (uint16_t)(n * 4)), 0x0E) + gap);
    }
    return w;
}

/* a text of the program's memory, for the two routines here */
static const uint8_t *host_text(Far text, uint8_t *buf, uint16_t n)
{
    uint16_t i;

    for (i = 0; i + 1 < n && (buf[i] = pb(text, i)) != 0; i++)
        ;
    buf[i] = 0;
    return buf;
}

static int16_t text_width(Far text, Far font, int16_t gap)
{
    uint8_t buf[80];

    return width_of(host_text(text, buf, sizeof buf), font, gap);
}

/* S167A:27B4 (text_at): each character's picture by pic_draw, then on by
 * its width and `gap`; one of the eleven is moved down by its signed
 * byte at S37FF:0107.  The dword of colours the original passes on is 0
 * in all of the menu's calls. */
static void draw_text(int16_t x, int16_t y, const uint8_t *text, Far font, uint16_t base, int16_t gap)
{
    for (; *text; text++) {
        int n = font_index(*text);
        int16_t at = y;
        Far pic;

        if (n < 0)
            continue;
        if (n >= 0x24)
            at = (int16_t)(at + (int8_t)frb(S_OPT, (uint16_t)(0x0107 + n - 0x24)));
        pic = pfar(font, (uint16_t)(n * 4));
        menu_pic(x, at, pic, base);
        x = (int16_t)(x + pb(pic, 0x0E) + gap);
    }
}

static void text_at(int16_t x, int16_t y, Far text, Far font, uint32_t colours, uint16_t base, int16_t gap)
{
    uint8_t buf[80];

    (void)colours;
    draw_text(x, y, host_text(text, buf, sizeof buf), font, base, gap);
}

/* S167A:274C (text_mid): at 160 less half its width */
static void text_mid(int16_t y, Far text, Far font, uint32_t colours, uint16_t base, int16_t gap)
{
    text_at((int16_t)(0xA0 - (text_width(text, font, gap) >> 1)), y, text, font, colours, base, gap);
}

/* text_mid for a caller that draws with the screen's cut off (the new
 * units' screens, briefing.c): the pictures go to pic_draw as they are */
void text_mid_uncut(int16_t y, Far text, Far font, uint16_t base, int16_t gap)
{
    uncut = 1;
    text_mid(y, text, font, 0, base, gap);
    uncut = 0;
}

/* a map's code in brackets in the middle of line 6 (the winner's name
 * and the best scores) */
static void code_mid(int16_t map)
{
    uint8_t head[8];
    int i;

    head[0] = 0x5B;
    for (i = 1; i < 6; i++)
        head[i] = (uint8_t)(pb(far_add(m.codes, (uint32_t)((int32_t)map * 10)), (uint16_t)(i - 1)) + 0x30);
    head[6] = 0x5D;
    head[7] = 0;
    draw_text((int16_t)(0xA0 - (width_of(head, m.font18, 0) >> 1)), 6, head, m.font18, 0, 0);
}

/* S167A:2A6B (item_line): an item of a page: the icon of its choice
 * (byte 6Ch on, a picture of MNUICON.LIB) and 26h right and 2 down its
 * choice's text in the large font; nothing for an item with byte 77h */
static void item_line(int16_t x, int16_t y, int16_t item)
{
    Far it = item_at(item);
    int16_t ch = (int8_t)pb(it, 0x74);

    if (pb(it, 0x77))
        return;
    menu_pic(x, y, pfar(m.icon, (uint16_t)(pb(it, (uint16_t)(0x6C + ch)) * 4)), 0);
    text_at((int16_t)(x + 0x26), (int16_t)(y + 2), far_add(it, (uint32_t)(8 + ch * 0x14)), m.font18, 0, 0, 0);
}

/* S167A:2B63 (item_mark): what the chosen item is for, at the page's
 * words 8 and 0Ah: the second picture of MNUMISC.LIB and on it the
 * item's three lines (78h, 98h, B8h) in the small font */
static void item_mark(Far pg, int16_t item)
{
    Far it = item_at(item);
    int16_t x = (int16_t)pw(pg, 8), y = (int16_t)pw(pg, 0x0A);

    menu_pic(x, y, pfar(m.misc, 4), 0);
    text_at((int16_t)(x + 9), (int16_t)(y + 3), far_add(it, 0x78), m.font8, 0, 0, 1);
    text_at((int16_t)(x + 5), (int16_t)(y + 0x0D), far_add(it, 0x98), m.font8, 0, 0, 1);
    text_at((int16_t)(x + 5), (int16_t)(y + 0x16), far_add(it, 0xB8), m.font8, 0, 0, 1);
}

/* S167A:29E2 (page_items): the page's items, 18h lines apart from its
 * words 0 and 2 */
static void page_items(Far pg, int16_t page)
{
    int16_t i, y = (int16_t)pw(pg, 2);

    (void)page;
    for (i = 0; page_count(pg) > i; i++, y = (int16_t)(y + 0x18))
        item_line((int16_t)pw(pg, 0), y, page_item(pg, i));
}

/* S167A:2CAF (page_title): the page's name (0Ch) in the large font with
 * the colours from 70h on, 12h right and 5 down of its words 4 and 6 */
static void page_frame(Far pg, int16_t page)
{
    (void)page;
    text_at((int16_t)(pw(pg, 4) + 0x12), (int16_t)(pw(pg, 6) + 5), far_add(pg, 0x0C), m.font18, 0, 0x70, 0);
}

static void pic_put(int16_t x, int16_t y, Far pic)
{
    menu_pic(x, y, pic, 0);
}

/* S167A:3A0B (score_line): a score as five digits in the middle font, 3
 * apart */
static void score_line(int16_t x, int16_t y, uint32_t score)
{
    uint8_t d[6];
    int i;

    for (i = 4; i >= 0; i--) {
        d[i] = (uint8_t)('0' + (int32_t)score % 10);
        score = (uint32_t)((int32_t)score / 10);
    }
    d[5] = 0;
    draw_text(x, y, d, m.font14, 0, 3);
}

/* ---- the menu's routines ---- */

/* the key's word and both sides' buttons and the mouse's two cleared */
static int inputs_clear(void)
{
    Far k = key_rec(), ms = mouse_rec();
    int any = pw(k, 4) || pb(side_rec(0), 4) || pb(side_rec(1), 4) || pw(ms, 0x30) || pw(ms, 0x34);

    pww(k, 4, 0);
    pwb(side_rec(0), 4, 0);
    pwb(side_rec(1), 4, 0);
    pww(ms, 0x30, 0);
    pww(ms, 0x34, 0);
    return any;
}

/* S167A:2F83 (inputs_release(1, wait)): the key, the buttons and the
 * mouse's cleared and five frames waited, again as long as one of them
 * was set since (at least twice); with `wait` then until one is set */
void inputs_release(int wait)
{
    Far k = key_rec(), ms = mouse_rec();

    inputs_clear();
    wait_frames(5);
    for (;;) {
        int any = inputs_clear();

        wait_frames(5);
        if (!any)
            break;
    }
    if (wait)
        while (!(pw(k, 4) || pb(side_rec(0), 4) || pb(side_rec(1), 4) || pw(ms, 0x30) || pw(ms, 0x34))
               && !menu_stopped)
            tick();
}

/* S167A:3B32: the middle of the page's item `idx` is where the pointer
 * goes */
static void pointer_aim(int16_t page, int16_t idx)
{
    Far it = item_at(page_item(page_at(page), idx));

    m.px = (int16_t)(((int16_t)(pw(it, 4) - pw(it, 0)) >> 1) + pw(it, 0));
    m.py = (int16_t)(((int16_t)(pw(it, 6) - pw(it, 2)) >> 1) + pw(it, 2));
}

/* S167A:3A9C with a mouse: its pointer shown and put there */
static void pointer_to(int16_t x, int16_t y)
{
    mouse_show();
    mouse_to((uint16_t)x, (uint16_t)y);
}

/* S167A:2D37 (page_draw): the page with its item `item` shown as chosen,
 * and the pointer on the item of place `idx` */
static void page_draw(int16_t page, int16_t item, int16_t idx)
{
    Far pg = page_at(page);

    mouse_hide();
    item_mark(pg, item);
    page_items(pg, page);
    page_frame(pg, page);
    flip();
    page_copy();
    pointer_aim(page, idx);
    pointer_to(m.px, m.py);
}

/* S167A:2E30 (page_rects): the rectangles of a page's items but the last,
 * words 0 to 6 of their records: at the page's place (its words 0 and 2),
 * 18h lines apart, 15h high and as wide as the widest of the item's texts
 * (byte 73h of them, 14h bytes each from byte 8 on) in the large font
 * and 27h more.  The mouse's pointer chooses by them. */
static void page_rects(int16_t page)
{
    Far pg = page_at(page);
    int16_t x = (int16_t)pw(pg, 0), y = (int16_t)pw(pg, 2), i, k;

    for (i = 0; page_count(pg) - 1 > i; i++) {
        Far it = item_at(page_item(pg, i));
        int16_t w = 0;

        for (k = 0; (int8_t)pb(it, 0x73) > k; k++) {
            int16_t tw = text_width(far_add(it, (uint32_t)(8 + k * 0x14)), m.font18, 0);

            if (tw > w)
                w = tw;
        }
        pww(it, 0, (uint16_t)x);
        pww(it, 2, (uint16_t)y);
        pww(it, 4, (uint16_t)(x + w + 0x27));
        pww(it, 6, (uint16_t)(y + 0x15));
        y = (int16_t)(y + 0x18);
    }
}

/* S167A:3128 (menu_code_set): the code of map `map` (the first five
 * bytes of its ten-byte record in CODES.DAT, 30h added to each) into
 * item 1's text from byte 0Fh on, closed by `]`, and to S37FF:0129; and
 * by byte 6 of the record who plays, as items 7 and 8 show it: 2, both
 * sides human and 400h cleared in game_state; else 400h set and one side
 * the computer's, the second with map_number below 18h, else the first */
static void menu_code_set(int16_t map)
{
    Far rec = far_add(m.codes, (uint32_t)((int32_t)map * 10)), it = item_at(1);
    Far it7 = item_at(7), it8 = item_at(8);
    uint16_t i;

    for (i = 0; i < 5; i++) {
        uint8_t c = (uint8_t)(pb(rec, i) + 0x30);

        pwb(it, (uint16_t)(0x0F + i), c);
        fwb(S_OPT, (uint16_t)(0x0129 + i), c);
    }
    pwb(it, 0x0F + 5, 0x5D);
    pwb(it, 0x10 + 5, 0);
    fwb(S_OPT, 0x012E, 0);
    if (pb(rec, 6) == 2) {
        pwb(it7, 0x74, 0);
        pwb(it8, 0x74, 0);
        SETW(game_state, 0, NW(game_state, 0) & 0xFBFF);
        return;
    }
    if ((int16_t)NW(map_number, 0) < 0x18) {
        pwb(it7, 0x74, 0);
        pwb(it8, 0x74, 1);
    } else {
        pwb(it7, 0x74, 1);
        pwb(it8, 0x74, 0);
    }
    SETW(game_state, 0, NW(game_state, 0) | 0x400);
}

/* S167A:325C (text_input): the player types at `text` + `start`, at most
 * `max` letters (capitals), the cursor a `]` that comes and goes every
 * 10h passes; the key of entry 2 of the table of keys takes the last
 * letter off, Enter or the key of entry 0 ends it.  The keys are the
 * BIOS's meanwhile (key_remove).  A pass lasts as long as its drawing
 * and page_flip. */
static void text_input(int16_t x, int16_t y, Far text, int16_t max, int16_t start, Far pic)
{
    Far k = key_rec();
    int16_t on = 1, count = 0, si = start, snd = -1;

    pww(k, 4, 0);
    pwb(k, 2, 0);
    pwb(k, 0, 0);
    pic_part(pic, x, (int16_t)(y - 2), text_width(text, m.font18, 0), 0x18, x, (int16_t)(y - 2));
    pwb(text, (uint16_t)si, 0);
    pwb(text, (uint16_t)(si + 1), 0x5D);
    text_at(x, y, text, m.font18, 0, 0, 0);
    key_remove();
    while (!menu_stopped) {
        if (pw(k, 4)) {
            uint8_t ch = pb(k, 2), scan = pb(k, 0);

            if (ch >= 'a' && ch <= 'z')
                ch = (uint8_t)(ch - 0x20);
            pww(k, 4, 0);
            pwb(k, 2, 0);
            pwb(k, 0, 0);
            if (ch >= 'A' && ch <= 'Z' && start + max > si) {
                pwb(text, (uint16_t)si, ch);
                if (on) {
                    pwb(text, (uint16_t)(si + 1), 0x5D);
                    pwb(text, (uint16_t)(si + 2), 0);
                } else
                    pwb(text, (uint16_t)(si + 1), 0);
                snd = 2;
                si++;
            } else if (ch == 0x0D || key0(0) == scan) {
                snd = 0;
                break;
            } else if (key0(2) == scan && si > start) {
                pic_part(pic, x, (int16_t)(y - 2), text_width(text, m.font18, 0), 0x18, x, (int16_t)(y - 2));
                si--;
                if (on) {
                    pwb(text, (uint16_t)si, 0x5D);
                    pwb(text, (uint16_t)(si + 1), 0);
                } else
                    pwb(text, (uint16_t)si, 0);
                snd = 2;
            }
            text_at(x, y, text, m.font18, 0, 0, 0);
        }
        if (count > 0x0F) {
            if (on) {
                pwb(text, (uint16_t)si, 0x5D);
                pwb(text, (uint16_t)(si + 1), 0);
                on = 0;
            } else {
                on = 1;
                pic_part(pic, x, (int16_t)(y - 2), text_width(text, m.font18, 0), 0x18, x, (int16_t)(y - 2));
                pwb(text, (uint16_t)si, 0);
            }
            text_at(x, y, text, m.font18, 0, 0, 0);
            count = 0;
        } else
            count++;
        if (snd >= 0) {
            sound(snd);
            snd = -1;
        }
        flip();
        page_copy();
        tick();
    }
    if (snd >= 0)
        sound(snd);
    key_install();
}

/* S167A:36F9 (scores_show): the map's four best scores with their names,
 * the best first.  The scores in the buffer are first brought into 0 to
 * 7EF4h, which stays. */
static void scores_show(Far buf, int16_t map)
{
    int8_t order[4];
    int32_t last = 0x7EF4;
    int n, i, k;

    for (i = 0; i < 4; i++) {
        int32_t v = (int32_t)(pw(buf, (uint16_t)(4 * i)) | (uint32_t)pw(buf, (uint16_t)(4 * i + 2)) << 16);

        if (v < 0)
            v = 0;
        if (v > 0x7EF4)
            v = 0x7EF4;
        pww(buf, (uint16_t)(4 * i), (uint16_t)v);
        pww(buf, (uint16_t)(4 * i + 2), (uint16_t)((uint32_t)v >> 16));
    }
    /* the next lower each time; one that equals the last and is not yet
     * taken counts as lower */
    for (n = 0; n < 4; n++) {
        int32_t best = -1;
        int8_t which = 0;

        for (i = 0; i < 4; i++) {
            int32_t v = (int32_t)(pw(buf, (uint16_t)(4 * i)) | (uint32_t)pw(buf, (uint16_t)(4 * i + 2)) << 16);

            if (v > best && v < last) {
                best = v;
                which = (int8_t)i;
            } else if (v > best && v == last) {
                int free_one = 1;

                for (k = 0; k < 4; k++)
                    if (k < n && order[k] == i)
                        free_one = 0;
                if (free_one) {
                    best = v;
                    which = (int8_t)i;
                }
            }
        }
        order[n] = which;
        last = (int32_t)(pw(buf, (uint16_t)(4 * which)) | (uint32_t)pw(buf, (uint16_t)(4 * which + 2)) << 16);
    }
    code_mid((int16_t)(int8_t)map);
    text_mid(0xAE, far_add(m.texts, 0xA0), m.font18, 0, 0, 0);
    for (n = 0; n < 4; n++) {
        text_at(0xAF, (int16_t)(n * 0x18 + 0x38), far_add(buf, (uint32_t)(0x10 + order[n] * 6)), m.font14, 0, 0, 3);
        score_line(0x41, (int16_t)(n * 0x18 + 0x38),
                   pw(buf, (uint16_t)(4 * order[n])) | (uint32_t)pw(buf, (uint16_t)(4 * order[n] + 2)) << 16);
    }
    flip();
    page_copy();
}

/* S2B76:0004 (screen_keep, as menu_run calls it): the page drawn (50h
 * bytes of C8h lines, plane by plane) into memory */
static void page_grab(Far to)
{
    uint16_t from = (uint16_t)((NW(screen_seg, 0) - 0xA000u) << 4), pitch = NW(screen_pitch, 0);
    uint32_t at = lin(to.seg, to.off);
    uint16_t plane, row, col;

    for (plane = 0; plane < 4; plane++) {
        vga_outw(0x3CE, (uint16_t)(plane << 8 | 4));
        for (row = 0; row < 0xC8; row++)
            for (col = 0; col < 0x50; col++)
                mem[at++] = vga_read((uint16_t)(from + row * pitch + col));
    }
}

/* a library loaded at the free end, which moves on by its length (the
 * dword at 0Ah of its record); its list of pictures (0Eh) is returned.
 * 0 if it cannot be loaded. */
static int lib(uint16_t name, uint16_t rec, Far *list)
{
    if (lib_load(file_name(1, -1, opt(name), 3), m.p, m.work, opt(rec), 0) == -1)
        return 0;
    *list = far_at(S_OPT, (uint16_t)(rec + 0x0E));
    m.p = far_add(m.p, frw(S_OPT, (uint16_t)(rec + 0x0A)) | (uint32_t)frw(S_OPT, (uint16_t)(rec + 0x0C)) << 16);
    return 1;
}

static int32_t score_now(void)
{
    return (int32_t)(NW(score, 0) | (uint32_t)NW(score, 2) << 16);
}

static int32_t score_at(Far buf, int i)
{
    return (int32_t)(pw(buf, (uint16_t)(4 * i)) | (uint32_t)pw(buf, (uint16_t)(4 * i + 2)) << 16);
}

/* S167A:078E to 0B20: after a battle with a score above the lowest of
 * its map's four best (the map's number is in word 0Fh of game_state,
 * battle_score's) the winner's name is asked for, put in that place with
 * the score, and the file written; then the four are shown until a key
 * or a button comes */
static void winner_name(void)
{
    int16_t map = (int16_t)NW(game_state, 0x0F);
    int32_t low = 0x7EF4;
    int which = 0, i;

    scores_load(m.scores, m.work, map, m.texts);
    for (i = 0; i < 4; i++)
        if (score_at(m.scores, i) < low) {
            which = i;
            low = score_at(m.scores, i);
        }
    if (!(score_now() > low))
        return;
    pic_whole(m.pic[1]);
    code_mid(map);
    text_mid(0x3C, far_add(m.texts, 0x20), m.font14, 0, 0, 2);
    text_mid(0x55, far_add(m.texts, 0x40), m.font14, 0, 0, 2);
    text_mid(0x6E, far_add(m.texts, 0x60), m.font14, 0, 0, 2);
    flip();
    page_copy();
    fade_in();
    play_fade();
    music_start(m.music_hi);
    text_input(0x64, 0xAE, far_add(m.texts, 0x80), 5, 1, m.pic[1]);
    for (i = 0; i < 5; i++)
        pwb(m.scores, (uint16_t)(0x10 + which * 6 + i), pb(m.texts, (uint16_t)(0x81 + i)));
    pww(m.scores, (uint16_t)(4 * which), NW(score, 0));
    pww(m.scores, (uint16_t)(4 * which + 2), NW(score, 2));
    /* S2BB9:000A: the 28h bytes written to the file whose name the last
     * file_name left (scores_load's: the map's file of scores) */
    {
        Far name;

        name.seg = S_name_buf;
        name.off = A_name_buf;
        host_save(name, mem + lin(m.scores.seg, m.scores.off), 0x28);
    }
    pic_whole(m.pic[1]);
    scores_show(m.scores, (int16_t)(int8_t)map);
    inputs_release(1);
    mouse_hide();
    fade_out();
    play_fade();
    pic_whole(m.pic[0]);
    flip();
    page_copy();
    inputs_release(0);
    music_stop();
}

/* S167A:1D43 to 2278: START with map 0 asks for the country: left and
 * right change it, a side's button or a click on a flag takes it, the
 * key of entry 0 or the mouse's other button goes back.  The first
 * country means map 18h, where the first side is the computer's.
 * Returns whether one was taken. */
static int country(void)
{
    Far k = key_rec(), ms = mouse_rec(), scr = screen_rec();
    int16_t si = 1, redraw = 0, stay = 1, chosen = 0;
    int s;

    mouse_hide();
    pic_whole(m.pic[2]);
    text_mid(0x0A, far_add(m.texts, 0x1A0), m.font18, 0, 0, 0);
    text_mid(0xAD, far_add(m.texts, (uint32_t)(0x1C0 + si * 0x20)), m.font18, 0, 0, 0);
    /* the two flags, pictures 0 and 1 of MNUFAHN.LIB, unpacked to the free
     * end */
    cut_off(1);
    pic_unpack_draw(0x0B, 0x33, pfar(m.fahn, 0), 0, m.p);
    pic_unpack_draw(0xA1, 0x33, pfar(m.fahn, 4), 0, m.p);
    cut_off(0);
    flip();
    page_copy();
    pointer_to((int16_t)frw(S_OPT, (uint16_t)(0x00F8 + si * 2)), 0x58);
    inputs_release(0);
    m.snd = -1;
    while (stay && !menu_stopped) {
        pww(scr, 0x22E, 0);
        if (pw(ms, 0x30) && (int16_t)pw(ms, 0x12) >= 0x33 && (int16_t)pw(ms, 0x12) <= 0x92) {
            if ((int16_t)pw(ms, 0x10) >= 0x0B && (int16_t)pw(ms, 0x10) <= 0x9A) {
                chosen = 1;
                si = 0;
            }
            if ((int16_t)pw(ms, 0x10) >= 0xA4 && (int16_t)pw(ms, 0x10) <= 0x133) {
                chosen = 1;
                si = 1;
            }
            if (chosen) {
                stay = 0;
                redraw = 1;
                m.snd = 0;
            }
        }
        if (pw(ms, 0x34)) {
            chosen = 0;
            m.snd = 1;
            stay = 0;
        }
        if (pw(k, 4)) {
            if (pb(k, 0) == key0(0)) {
                chosen = 0;
                m.snd = 1;
                stay = 0;
            }
            pww(k, 4, 0);
            pwb(k, 0, 0);
        }
        for (s = 0; s < 2; s++) {
            Far rec = side_rec(s);

            if (pb(rec, 0)) {
                if (!(m.held[s] & 1)) {
                    si--;
                    m.snd = 2;
                    redraw = 1;
                    if (si < 0)
                        si = 1;
                    m.held[s] |= 1;
                }
                pwb(rec, 0, 0);
            } else
                m.held[s] &= 0xFFFE;
            if (pb(rec, 1)) {
                if (!(m.held[s] & 2)) {
                    si++;
                    m.snd = 2;
                    redraw = 1;
                    if (si > 1)
                        si = 0;
                    m.held[s] |= 2;
                }
                pwb(rec, 1, 0);
            } else
                m.held[s] &= 0xFFFD;
            if (pb(rec, 4)) {
                redraw = 0;
                chosen = 1;
                stay = 0;
                m.snd = 0;
                break;
            }
            while ((int16_t)pw(scr, 0x22E) < 5 && !menu_stopped)
                tick();
        }
        if (m.snd >= 0) {
            sound(m.snd);
            m.snd = -1;
        }
        if (redraw) {
            redraw = 0;
            mouse_hide();
            pic_part(m.pic[2], 0, 0xA0, 0x140, 0x23, 0, 0xA0);
            text_mid(0xAD, far_add(m.texts, (uint32_t)(0x1C0 + si * 0x20)), m.font18, 0, 0, 0);
            flip();
            page_copy();
            pointer_to((int16_t)frw(S_OPT, (uint16_t)(0x00F8 + si * 2)), 0x58);
        }
    }
    if (!chosen) {
        pic_whole(m.pic[0]);
        page_draw(m.page, m.item, m.idx);
        inputs_release(0);
        return 0;
    }
    if (si == 0) {
        SETW(map_number, 0, 0x18);
        menu_code_set(0x18);
        pwb(item_at(7), 0x74, 1);
        pwb(item_at(8), 0x74, 0);
    }
    mouse_hide();
    wait_frames(0x64);
    return 1;
}

/* S167A:1780 to 1BED: a saved game: its number is the digit typed (the
 * BIOS's keys; the key of entry 0, a button or the mouse leave it); when
 * the file is there 200h is set in game_state, which makes game_loop
 * load it, and the menu ends */
static void saved_game(void)
{
    Far k = key_rec(), ms = mouse_rec(), err = far_at(S_DATA, 0x0096), name, num;
    uint8_t ch;
    size_t size;
    uint8_t *file;

    mouse_hide();
    pic_whole(m.pic[0]);
    text_mid(0x06, far_add(m.texts, 0xC0), m.font18, 0, 0, 0);
    text_mid(0x37, far_add(m.texts, 0xE0), m.font18, 0, 0, 0);
    text_mid(0x50, far_add(m.texts, 0x100), m.font18, 0, 0, 0);
    text_mid(0x69, far_add(m.texts, 0x120), m.font18, 0, 0, 0);
    text_mid(0x82, far_add(m.texts, 0x140), m.font18, 0, 0, 0);
    flip();
    page_copy();
    pointer_to(0x122, 0xB5);
    inputs_release(0);
    pww(k, 4, 0);
    pwb(side_rec(0), 4, 0);
    pwb(side_rec(1), 4, 0);
    pww(ms, 0x30, 0);
    pww(ms, 0x34, 0);
    key_remove();
    for (;;) {
        if (menu_stopped) {
            ch = 0xFF;
            break;
        }
        if (pb(side_rec(0), 4) || pb(side_rec(1), 4) || pw(ms, 0x30) || pw(ms, 0x34)) {
            ch = 0xFF;
            break;
        }
        if (pw(k, 4)) {
            ch = pb(k, 2);
            pww(k, 4, 0);
            if (ch >= '0' && ch <= '9') {
                SETW(game_state, 6, ch - '0');
                ch = 0;
                break;
            }
            if (pb(k, 0) == key0(0)) {
                ch = 0xFF;
                break;
            }
        }
        tick();
    }
    mouse_hide();
    key_install();
    pic_whole(m.pic[0]);
    flip();
    page_copy();
    if (ch == 0) {
        num.seg = S_num_buf;
        num.off = A_num_buf;
        name = file_name(2, -1, num_text((int32_t)(int16_t)NW(game_state, 6), num, 2, 4), 5);
        fww(err.seg, err.off, 0);
        file = host_load(name, &size);
        if (file) {
            free(file);
            SETW(game_state, 0, NW(game_state, 0) | 0x200);
            m.run = 0;
            fade_out();
            play_fade();
        } else {
            text_mid(0x50, far_add(m.texts, 0x160), m.font18, 0, 0, 0);
            text_mid(0x69, far_add(m.texts, 0x180), m.font18, 0, 0, 0);
            flip();
            page_copy();
            pointer_to(0x122, 0xB5);
            wait_frames(0x32);
            inputs_release(1);
            mouse_hide();
            pic_whole(m.pic[0]);
            flip();
            page_copy();
        }
        fww(err.seg, err.off, 1);
    }
    page_draw(m.page, m.item, m.idx);
    inputs_release(0);
}

/* the mouse's pointer in an item's rectangle */
static int in_item(Far ms, Far it)
{
    int16_t x = (int16_t)pw(ms, 0x10), y = (int16_t)pw(ms, 0x12);

    return x >= (int16_t)pw(it, 0) && x < (int16_t)pw(it, 4) && y >= (int16_t)pw(it, 2) && y < (int16_t)pw(it, 6);
}

/* the item chosen, S167A:1410 to 22A7: by byte 72h of the record at itp:
 * 4 the map's code is typed and looked for in CODES.DAT (found: the map's
 * number); 2 the item's choice (byte 74h) goes on by one, from the last
 * (byte 73h of them) to 0; 5 the page of byte 76h; 1 by byte 76h: 3 a
 * saved game, 1 the best scores, 4 START, 2 the program's end (100h set
 * and 1 and 2 cleared in game_state) */
static void choose(void)
{
    Far pg = page_at(m.page);
    int16_t i, k;

    switch (pb(m.itp, 0x72)) {
    case 4: {
        int found = 0;

        mouse_hide();
        text_input((int16_t)(pw(m.itp, 0) + 0x26), (int16_t)(pw(m.itp, 2) + 2), far_add(m.itp, 8), 5, 7, m.pic[0]);
        pww(screen_rec(), 0x22E, 0);
        for (i = 0; i < m.ncodes; i++) {
            for (k = 0; k < 5; k++)
                if (pb(far_add(m.codes, (uint32_t)((int32_t)i * 10)), (uint16_t)k)
                    != (uint16_t)(pb(m.itp, (uint16_t)(0x0F + k)) + 0xFFD0))
                    break;
            if (k == 5) {
                found = 1;
                break;
            }
        }
        if (found) {
            SETW(map_number, 0, i);
            menu_code_set(i);
            scores_load(m.scores, m.work, (int16_t)NW(map_number, 0), m.texts);
        } else
            menu_code_set((int16_t)NW(map_number, 0));
        pic_whole(m.pic[0]);
        page_draw(m.page, m.item, m.idx);
        break;
    }
    case 2:
        m.itp = item_at(m.item);
        pwb(m.itp, 0x74, (uint8_t)(pb(m.itp, 0x74) + 1));
        if (!((int8_t)pb(m.itp, 0x74) < (int8_t)pb(m.itp, 0x73)))
            pwb(m.itp, 0x74, 0);
        mouse_hide();
        pic_part(m.pic[0], (int16_t)pw(m.itp, 0), (int16_t)pw(m.itp, 2), (int16_t)(pw(m.itp, 4) - pw(m.itp, 0)),
                 (int16_t)(pw(m.itp, 6) - pw(m.itp, 2)), (int16_t)pw(m.itp, 0), (int16_t)pw(m.itp, 2));
        item_line((int16_t)pw(pg, 0), (int16_t)(pw(pg, 2) + m.idx * 0x18), page_item(pg, m.idx));
        flip();
        page_copy();
        pointer_aim(m.page, m.idx);
        pointer_to(m.px, m.py);
        m.snd = 0;
        break;
    case 5:
        m.page = (int8_t)pb(m.itp, 0x76);
        m.idx = 0;
        m.item = page_item(page_at(m.page), 0);
        m.itp = item_at(m.item);
        mouse_hide();
        pic_whole(m.pic[0]);
        page_draw(m.page, m.item, m.idx);
        m.snd = 0;
        break;
    case 1:
        switch (pb(m.itp, 0x76)) {
        case 3:
            saved_game();
            break;
        case 1:
            music_stop();
            mouse_hide();
            pic_whole(m.pic[1]);
            flip();
            page_copy();
            music_start(m.music_hi);
            scores_show(m.scores, (int16_t)(int8_t)NW(map_number, 0));
            pointer_aim(m.page, m.idx);
            pointer_to(0x9B, 0x5F);
            inputs_release(1);
            mouse_hide();
            pic_whole(m.pic[0]);
            page_draw(m.page, m.item, m.idx);
            inputs_release(0);
            music_stop();
            music_start(m.music);
            break;
        case 4:
            if (NW(map_number, 0) != 0 || country())
                m.run = 0;
            break;
        case 2:
            SETW(game_state, 0, (NW(game_state, 0) | 0x100) & 0xFFFC);
            m.run = 0;
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}

/* one pass of the menu's loop, S167A:0D3C to 23B1: what came in (the key
 * of entry 0 of the table of keys or the mouse's second button: 2; a
 * side's button or a click on an item of the page: 1; a side's up and
 * down: the item before or after, around the page's end), each once
 * while it is held; without any the item under the mouse's pointer
 * becomes the chosen one.  2 changes between pages 1 and 0.  The pass
 * lasts until the timer's routine has counted 5.  The record a choice is
 * read from (itp) is also what the two loops over the page's items go
 * through them with: at the menu's start it is item 12h's, and the first
 * pass without input puts it on the item under the pointer */
static void pass(void)
{
    Far k = key_rec(), ms = mouse_rec(), scr = screen_rec(), pg;
    int16_t act = 0, i;
    int s;

    for (s = 0; s < shots_n; s++)
        if (shots[s].file && play_now() >= shots[s].at) {
            vga_write_planes(shots[s].file);
            shots[s].file = NULL;
        }
    pww(scr, 0x22E, 0);
    if (pw(k, 4)) {
        if (pb(k, 0) == key0(0))
            act = 2;
        pww(k, 4, 0);
        pwb(k, 0, 0);
    }
    if (!act)
        for (s = 0; s < 2; s++) {
            Far rec = side_rec(s);
            static const struct { uint16_t at, bit; int16_t act; } in[3] = { { 4, 4, 1 }, { 2, 1, 3 }, { 3, 2, 4 } };

            for (i = 0; i < 3; i++)
                if (pb(rec, in[i].at)) {
                    if (!(m.held[s] & in[i].bit)) {
                        act = in[i].act;
                        m.held[s] |= in[i].bit;
                    }
                    pwb(rec, in[i].at, 0);
                } else
                    m.held[s] &= (uint16_t)~in[i].bit;
        }
    if (pw(ms, 0x34)) {
        if (!(m.mheld & 2)) {
            act = 2;
            m.mheld |= 2;
        }
        pww(ms, 0x34, 0);
    } else
        m.mheld &= 0xFFFD;
    if (pw(ms, 0x30)) {
        if (!(m.mheld & 1)) {
            pg = page_at(m.page);
            m.mheld |= 1;
            for (i = 0; page_count(pg) > i; i++) {
                m.itp = item_at(page_item(pg, i));
                if (in_item(ms, m.itp))
                    break;
            }
            if (page_count(pg) != i)
                act = 1;
        }
        pww(ms, 0x30, 0);
    } else
        m.mheld &= 0xFFFE;
    pg = page_at(m.page);
    if (act == 3 || act == 4) {
        if (act == 3) {
            if (--m.idx < 0)
                m.idx = (int16_t)(page_count(pg) - 1);
        } else if (page_count(pg) - 1 < ++m.idx)
            m.idx = 0;
        m.item = page_item(pg, m.idx);
        m.itp = item_at(m.item);
        act = 5;
        m.snd = 2;
    } else if (!act) {
        /* itp is this loop's own variable too: it is left on the item
         * under the pointer, or on the page's last */
        for (i = 0; page_count(pg) > i; i++) {
            m.itp = item_at(page_item(pg, i));
            if (in_item(ms, m.itp)) {
                if (page_item(pg, i) != m.item || !m.hl) {
                    m.item = page_item(pg, i);
                    m.itp = item_at(m.item);
                    m.idx = i;
                    m.hl = 1;
                    item_mark(pg, m.item);
                    mouse_hide();
                    flip();
                    page_copy();
                    mouse_show();
                }
                break;
            }
        }
        if (page_count(pg) == i && m.hl) {
            /* the pointer on none: nothing is shown as chosen */
            pic_put((int16_t)pw(pg, 8), (int16_t)pw(pg, 0x0A), pfar(m.misc, 4));
            mouse_hide();
            flip();
            page_copy();
            pointer_aim(m.page, m.idx);
            mouse_show();
            m.hl = 0;
        }
    }
    if (menu_log && act)
        fprintf(menu_log, "%.3f %d %d %d %d\n", play_now(), act, m.page, m.idx, m.item);
    switch (act) {
    case 1:
        choose();
        break;
    case 2:
        m.page = m.page == 1 ? 0 : 1;
        m.idx = 0;
        m.item = page_item(page_at(m.page), 0);
        m.itp = item_at(m.item);
        mouse_hide();
        pic_whole(m.pic[0]);
        page_draw(m.page, m.item, m.idx);
        m.snd = 1;
        break;
    case 5:
        m.itp = item_at(page_item(pg, m.idx));
        mouse_hide();
        item_mark(pg, m.item);
        m.hl = 1;
        flip();
        page_copy();
        pointer_aim(m.page, m.idx);
        pointer_to(m.px, m.py);
        break;
    default:
        break;
    }
    if (m.snd >= 0) {
        sound(m.snd);
        m.snd = -1;
    }
    while (pw(scr, 0x22E) < 5 && !menu_stopped)
        tick();
}

/* what the menu leaves when it ends, S167A:23BA to 2748: the choices
 * (byte 74h) of items 0 to 12h to S37FF:0116; from items 7 and 8 which
 * sides are the computer's (2 in word 0 of a side's record; word 2 of
 * game_state: 1 one, 4 both, 2 none); from item 0Ah byte 16h of both
 * sides' records (4, 8, 10h for its choices 1 to 3, else FFh); item 9: 8
 * in word 2 of game_state; item 6 to S37FF:0112 (the palette's number);
 * pics_winter from byte 8 of the map's record in CODES.DAT; item 0Bh to
 * no_fight_pics; the highest of the map's four best scores to
 * S331F:4D2B; item 0Eh to S37FF:0114: the side whose record the mouse
 * plays (mouse_open) */
static void leave(void)
{
    Far scr = screen_rec();
    int16_t i;
    int32_t best = -1;
    uint16_t w2;
    uint8_t level;

    for (i = 0; i < 0x13; i++)
        fwb(S_OPT, (uint16_t)(0x0116 + i), pb(item_at(i), 0x74));
    w2 = NW(game_state, 2) & 0xFFF8;
    if ((int8_t)pb(item_at(7), 0x74) > 0) {
        SETW(sides, 0, NW(sides, 0) | 2);
        w2 |= 1;
    } else
        SETW(sides, 0, NW(sides, 0) & 0xFFFD);
    if ((int8_t)pb(item_at(8), 0x74) > 0) {
        SETW(sides, 0x17, NW(sides, 0x17) | 2);
        w2 |= 1;
    } else
        SETW(sides, 0x17, NW(sides, 0x17) & 0xFFFD);
    if ((NW(sides, 0) & 2) && (NW(sides, 0x17) & 2))
        w2 = (w2 | 4) & 0xFFFE;
    if (!(NW(sides, 0) & 2) && !(NW(sides, 0x17) & 2))
        w2 = (w2 | 2) & 0xFFFE;
    switch ((int8_t)pb(item_at(0x0A), 0x74)) {
    case 1: level = 4; break;
    case 2: level = 8; break;
    case 3: level = 0x10; break;
    default: level = 0xFF; break;
    }
    SETB(sides, 0x17 + 0x16, level);
    SETB(sides, 0x16, level);
    if ((int8_t)pb(item_at(9), 0x74) > 0)
        w2 |= 8;
    else
        w2 &= 0xFFF7;
    SETW(game_state, 2, w2);
    fwb(S_OPT, 0x0112, pb(item_at(6), 0x74));
    SETB(pics_winter, 0,
         pb(far_add(m.codes, (uint32_t)((int32_t)(int16_t)NW(map_number, 0) * 10)), 8) & 2 ? 0xFF : 0);
    SETB(no_fight_pics, 0, pb(item_at(0x0B), 0x74));
    for (i = 0; i < 4; i++)
        if (score_at(m.scores, i) > best)
            best = score_at(m.scores, i);
    fww(S_GAME, 0x4D2B, (uint16_t)best);
    fww(S_GAME, 0x4D2D, (uint16_t)((uint32_t)best >> 16));
    pww(mouse_rec(), 0x46, 0xFFFF);
    m.itp = item_at(0x0E);
    mouse_hide();
    mouse_close();
    mouse_open(0, frw(S_DATA, (int8_t)pb(m.itp, 0x74) > 0 ? 0x00B6 : 0x00A6), 0);
    mouse_show();
    fwb(S_OPT, 0x0114, pb(m.itp, 0x74));
    if (!(NW(game_state, 0) & 0x200)) {
        fade_out();
        play_fade();
    }
    music_stop();
    mouse_hide();
    mouse_close();
    unders_restore();
    pww(scr, 0, 0);
    screen_clear();
    flip();
    unders_restore();
    screen_clear();
}

/* S167A:0005 (menu_run(far place)): the menu, from the block's free part
 * on: 32h bytes for a map's best scores, 1064h of work buffer, three
 * pictures of FA00h bytes (MNUTIT, MNUSCORE and MNUFAHN, each shown on
 * the page drawn and fetched from there), 3E8h for CODES.DAT, then the
 * six libraries, the two tunes, the sounds and MNUDATA.DAT, whose word 0
 * is passed over, 200h bytes of texts follow (20h each), then the items'
 * records from 202h and the pages' from 120Ah.  Returns 0, FFFFh when a
 * file cannot be loaded; menu_stopped says when it could not go on. */
int menu_run(Far place)
{
    Far scr = screen_rec(), k = key_rec(), ms = mouse_rec();
    int32_t size;
    int16_t i;

    m.snd = -1;
    m.page = 1;
    m.item = m.idx = m.hl = 0;
    m.run = 1;
    m.mheld = m.held[0] = m.held[1] = 0;
    pww(scr, 0x0E, 1);
    pww(scr, 0x12, 0);
    pww(scr, 0x14, 0);
    pww(scr, 0x16, 0x13F);
    pww(scr, 0x18, 0xC7);
    pww(scr, 0x1C, 0xFFFF);
    pages_set();
    page_flip();
    m.p = place;
    m.scores = m.p;
    m.p = far_add(m.p, 0x32);
    m.work = m.p;
    m.p = far_add(m.p, 0x1064);
    for (i = 0; i < 3; i++) {
        m.pic[i] = m.p;
        m.p = far_add(m.p, 0xFA00);
    }
    m.codes = m.p;
    m.p = far_add(m.p, 0x3E8);
    size = file_load_size(m.codes, file_name(1, -1, opt(0x009E), 5), m.work);
    if (size == -1)
        return -1;
    m.ncodes = (int16_t)((int16_t)size / 10);
    for (i = 0; i < 3; i++) {
        lbm_show(m.p, file_name(1, -1, opt((uint16_t)(0x0080 + i * 10)), 0x0A), m.work);
        page_grab(m.pic[i]);
    }
    if (!lib(0x00B4, 0x0008, &m.font18) || !lib(0x00BD, 0x001C, &m.font14) || !lib(0x00C6, 0x0030, &m.font8)
        || !lib(0x00AC, 0x006C, &m.fahn) || !lib(0x00CE, 0x0044, &m.icon) || !lib(0x00D6, 0x0058, &m.misc))
        return -1;
    m.music = m.p;
    size = file_load_size(m.p, file_name(1, -1, opt(0x00E6), -1), m.work);
    if (size == -1)
        return -1;
    m.p = far_add(m.p, (uint32_t)size);
    m.music_hi = m.p;
    size = file_load_size(m.p, file_name(1, -1, opt(0x00EE), -1), m.work);
    if (size == -1)
        return -1;
    m.p = far_add(m.p, (uint32_t)size);
    size = sfx_load(m.p, file_name(1, -1, opt(0x00DE), -1), m.work);
    if (size < 0)
        return -1;
    m.p = far_add(m.p, (uint32_t)size);
    size = file_load_size(m.p, file_name(1, -1, opt(0x00A4), 5), m.work);
    if (size == -1)
        return -1;
    m.data = m.p;
    m.texts = far_add(m.p, 2);
    m.items = far_add(m.p, 0x202);
    m.pages = far_add(m.p, 0x120A);
    m.px = 0x9B;
    m.py = 0x5F;
    m.p = far_add(m.p, (uint32_t)size);
    /* the pointer's picture: the first of MNUMISC.LIB */
    pww(scr, 0x288, pw(m.misc, 0));
    pww(scr, 0x28A, pw(m.misc, 2));
    mouse_close();
    mouse_open(1, 0, 1);
    mouse_hide();
    if ((int16_t)pw(ms, 0) >= 0) {
        menu_stopped = "the menu without a mouse (S167A:0775)";
        return 0;
    }
    mouse_speed(0x10, 0x20, 0x10, 0x20, 0x1F4, 0x1F4, 0x1F4);
    mouse_range(8, 4, 0x12A, 0xC0);
    if (score_now() > 0)
        winner_name();
    scores_load(m.scores, m.work, (int16_t)NW(map_number, 0), m.texts);
    m.item = page_item(page_at(m.page), m.idx);
    for (i = 0; i < 5; i++)
        page_rects(i);
    for (i = 0; i < 0x13; i++) {
        m.itp = item_at(i);
        pwb(m.itp, 0x74, frb(S_OPT, (uint16_t)(0x0116 + i)));
    }
    menu_code_set((int16_t)NW(map_number, 0));
    pic_whole(m.pic[0]);
    page_draw(m.page, m.item, m.idx);
    m.hl = 1;
    fade_in();
    play_fade();
    music_start(m.music);
    wait_frames(5);
    pww(k, 4, 0);
    pwb(k, 0, 0);
    for (i = 0; i < 2; i++) {
        pwb(side_rec(i), 2, 0);
        pwb(side_rec(i), 3, 0);
        pwb(side_rec(i), 4, 0);
    }
    pww(ms, 0x30, 0);
    pww(ms, 0x34, 0);
    while (m.run && !menu_stopped)
        pass();
    if (menu_stopped)
        return 0;
    leave();
    if (menu_log) {
        fprintf(menu_log, "%.3f 0\n", play_now());
        fflush(menu_log);
    }
    return 0;
}
