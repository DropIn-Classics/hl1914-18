/* fightpics.c - a fight shown as pictures: HL.EXE's fight_pictures
 * (S2532:0008) with its parts, fight_frames (S2532:0CAA) and the calls of
 * rand_next in it (S2532:17D1, 1970), and the blitters they draw with
 * (S29C1:001F, S2AE4:000C, S2AF3:000A, S297D:000C, S2B76:0004,
 * S294A:000A, S292E:000A) on the runtime's VGA (doskit/runtime/vga.h).
 * Its variables are in S3AEE.  The sounds (S2F39) are left out; what is
 * loaded for them is not.  port/README.md says how this was compared.
 */
#include <stdio.h>
#include "hl.h"
#include "vga.h"

#define S_DATA (HL_DATA + HL_BASE)
#define S_FP (HL_S3AEE + HL_BASE)
#define S_SPR (HL_S29C1 + HL_BASE)      /* S29C1:001F's variables, in its own segment */
#define DW(o) frw(S_DATA, (o))
#define FB(o) frb(S_FP, (uint16_t)(o))
#define FW(o) frw(S_FP, (uint16_t)(o))
#define SFB(o, v) fwb(S_FP, (uint16_t)(o), (uint8_t)(v))
#define SFW(o, v) fww(S_FP, (uint16_t)(o), (uint16_t)(v))
#define FFAR(o) far_at(S_FP, (uint16_t)(o))
#define CW(o) frw(S_SPR, (uint16_t)(o))
#define SCW(o, v) fww(S_SPR, (uint16_t)(o), (uint16_t)(v))

void (*pics_hook)(int at);

static void set_far(uint16_t seg, uint16_t off, Far p)
{
    fww(seg, off, p.off);
    fww(seg, (uint16_t)(off + 2), p.seg);
}
#define SFFAR(o, p) set_far(S_FP, (uint16_t)(o), (p))

static Far far_of(uint16_t seg, uint16_t off)
{
    Far p;

    p.seg = seg;
    p.off = off;
    return p;
}
#define FP(o) far_of(S_FP, (uint16_t)(o))

/* ptr_add with a signed word (CWD before it) */
static Far far_adds(Far p, int16_t n)
{
    return far_add(p, (uint32_t)(int32_t)n);
}

/* clock_seed(0) (S2C95:000E): rand_words made of the BIOS's count of
 * ticks */
void clock_seed_bios(void)
{
    fww(S_DATA, 0x0AF3, frw(0x40, 0x6C) ^ frw(0x40, 0x6E));
    fww(S_DATA, 0x0AF1, frw(0x40, 0x6D) | frw(0x40, 0x6F));
}

/* ---- the blitters */

/* S2955:0002: the place of pixel x, y on the page drawn (the row table at
 * DATA:00F8) and its plane's map mask, 11h rotated left by x & 3 */
static uint16_t pixel_at(uint16_t x, uint16_t y, uint8_t *mask)
{
    *mask = (uint8_t)(0x11 << (x & 3));
    return (uint16_t)(DW((uint16_t)(0x00F8 + (uint16_t)(y + y))) + (x >> 2));
}

/* S292E:000A: the pixel DATA:00DC, 00DE in the colour DATA:00DA */
void pixel_put(void)
{
    uint8_t m;
    uint16_t di = pixel_at(DW(0x00DC), DW(0x00DE), &m);

    vga_outb(0x3C4, 2);
    vga_outb(0x3C5, m);
    vwb(NW(screen_seg, 0), di, (uint8_t)DW(0x00DA));
}

/* S2AE4:000C: what lies under a picture kept, `w` pixels wide (w / 4 + 2
 * bytes of each plane through the latches) and `h` rows from the screen
 * at src: at `save`, or for a segment 0 there at the end of the page's
 * list DATA:0322 (which then moves on).  A header (the screen's offset,
 * h, the bytes), the rows, a trailer (offset, h, bytes, where the rows
 * begin).  With DATA:0374 only the trailer, with 0 for the rows. */
void under_save(uint16_t w, uint16_t h, Far src, Far save)
{
    Far to = save.seg ? save : far_at(S_DATA, 0x0322);
    uint16_t ax = (uint16_t)((w >> 2) + 2), dx = (uint16_t)(DW(0x02F0) - ax), di = to.off, si = src.off, start, n;

    vga_outw(0x3C4, 0x0F02);
    vga_outw(0x3CE, 0x4005);
    if (DW(0x0374)) {
        to = far_at(S_DATA, 0x0322);
        di = to.off;
        vwb(to.seg, di, (uint8_t)si), vwb(to.seg, (uint16_t)(di + 1), (uint8_t)(si >> 8));
        vwb(to.seg, (uint16_t)(di + 2), (uint8_t)h), vwb(to.seg, (uint16_t)(di + 3), (uint8_t)(h >> 8));
        vwb(to.seg, (uint16_t)(di + 4), (uint8_t)ax), vwb(to.seg, (uint16_t)(di + 5), (uint8_t)(ax >> 8));
        vwb(to.seg, (uint16_t)(di + 6), 0), vwb(to.seg, (uint16_t)(di + 7), 0);
        di = (uint16_t)(di + 8);
    } else {
        vwb(to.seg, di, (uint8_t)si), vwb(to.seg, (uint16_t)(di + 1), (uint8_t)(si >> 8));
        vwb(to.seg, (uint16_t)(di + 2), (uint8_t)h), vwb(to.seg, (uint16_t)(di + 3), (uint8_t)(h >> 8));
        vwb(to.seg, (uint16_t)(di + 4), (uint8_t)ax), vwb(to.seg, (uint16_t)(di + 5), (uint8_t)(ax >> 8));
        di = (uint16_t)(di + 6);
        vga_outw(0x3C4, 0x0F02);
        vga_outw(0x3CE, 0x4105);
        start = di;
        n = h;
        do {
            uint16_t k;
            for (k = 0; k < ax; k++)
                vwb(to.seg, di++, vrb(src.seg, si++));
            si = (uint16_t)(si + dx);
        } while (--n);
        vga_outw(0x3CE, 0x4005);
        vwb(to.seg, di, (uint8_t)src.off), vwb(to.seg, (uint16_t)(di + 1), (uint8_t)(src.off >> 8));
        vwb(to.seg, (uint16_t)(di + 2), (uint8_t)h), vwb(to.seg, (uint16_t)(di + 3), (uint8_t)(h >> 8));
        vwb(to.seg, (uint16_t)(di + 4), (uint8_t)ax), vwb(to.seg, (uint16_t)(di + 5), (uint8_t)(ax >> 8));
        vwb(to.seg, (uint16_t)(di + 6), (uint8_t)start), vwb(to.seg, (uint16_t)(di + 7), (uint8_t)(start >> 8));
        di = (uint16_t)(di + 8);
    }
    if (!save.seg)
        fww(S_DATA, 0x0322, di);
}

/* S297D:000C: `h` rows of `w` bytes copied through the latches from
 * src to dst, both `pitch` apart */
void latch_copy(uint16_t w, uint16_t h, Far src, Far dst)
{
    uint16_t dx = (uint16_t)(DW(0x02F0) - w), si = src.off, di = dst.off, k;

    vga_outw(0x3C4, 0x0F02);
    vga_outw(0x3CE, 0x4105);
    do {
        for (k = 0; k < w; k++)
            vwb(dst.seg, di++, vrb(src.seg, si++));
        si = (uint16_t)(si + dx);
        di = (uint16_t)(di + dx);
    } while (--h);
    vga_outw(0x3CE, 0x4005);
}

static uint16_t vrw(uint16_t seg, uint16_t off)
{
    return (uint16_t)(vrb(seg, off) | vrb(seg, (uint16_t)(off + 1)) << 8);
}

/* S2AF3:000A: the page's list DATA:0322 walked back from its end to its
 * beginning (DATA:0326), each kept piece put back on the page drawn
 * through the latches (a piece without rows from the segment DATA:0374 by
 * S297D:000C); the list is empty then.  The screen's pitch is kept at
 * S2AF3:00BA.  With DATA:04B9 the original first calls S2D0D:01DD (not
 * read; not met). */
void unders_restore(void)
{
    uint16_t pitch = DW(0x02F0), es = NW(screen_seg, 0), start = DW(0x0326), seg = DW(0x0324), bp = DW(0x0322);

    vga_outw(0x3C4, 0x0F02);
    vga_outw(0x3CE, 0x4105);
    if (DW(0x04B9))
        fprintf(stderr, "unders_restore: DATA:04B9 set, S2D0D:01DD left out\n");
    fww(HL_S2AF3 + HL_BASE, 0x00BA, pitch);
    while ((int16_t)start < (int16_t)bp) {
        uint16_t si = vrw(seg, (uint16_t)(bp - 2)), ax = vrw(seg, (uint16_t)(bp - 4));
        uint16_t bx = vrw(seg, (uint16_t)(bp - 6)), di = vrw(seg, (uint16_t)(bp - 8));

        if ((int16_t)ax > (int16_t)pitch || (int16_t)bx > 0xC7)
            break;
        if (!si) {
            /* S297D:000C leaves write mode 0 set for the pieces after */
            latch_copy(ax, bx, far_of(DW(0x0374), di), far_of(es, di));
            bp = (uint16_t)(bp - 8);
        } else {
            uint16_t s = si;
            do {
                uint16_t k;
                for (k = 0; k < ax; k++)
                    vwb(es, di++, vrb(seg, s++));
                di = (uint16_t)(di + pitch - ax);
            } while (--bx);
            bp = (uint16_t)(si - 6);
        }
    }
    fww(S_DATA, 0x0322, start);
    vga_outw(0x3CE, 0x4005);
}

/* S2B76:0004: `h` rows of `w` bytes of each plane of the screen at src
 * copied to `to` (plane after plane; with `spread` interleaved, a byte of
 * each plane in turn); returns where it ended, normalized */
static Far screen_keep(Far src, Far to, uint16_t w, uint16_t h, uint16_t spread)
{
    uint16_t pitch = DW(0x02F0), si = src.off, di = to.off, plane;

    for (plane = 0; plane < 4; plane++) {
        uint16_t s = si, d = di, r, k;

        vga_outw(0x3CE, (uint16_t)(plane << 8 | 4));
        for (r = h; r; r--) {
            uint16_t row = s;
            for (k = 0; k < w; k++) {
                vwb(to.seg, d, vrb(src.seg, s++));
                d = (uint16_t)(d + (spread ? 4 : 1));
            }
            s = (uint16_t)(row + pitch);
        }
        if (spread)
            di++;
        else
            di = d;
    }
    to.off = di;
    return ptr_norm(to);
}

/* S294A:000A: the reverse, from `from` back to the screen at dst */
static void screen_put(Far from, Far dst, uint16_t w, uint16_t h, uint16_t spread)
{
    uint16_t pitch = DW(0x02F0), si = from.off, plane;
    uint8_t m = 0x11;

    for (plane = 0; plane < 4; plane++) {
        uint16_t d = dst.off, s = si, r, k;

        vga_outw(0x3C4, (uint16_t)(m << 8 | 2));
        for (r = h; r; r--) {
            uint16_t row = d;
            for (k = 0; k < w; k++) {
                vwb(dst.seg, d++, vrb(from.seg, s));
                s = (uint16_t)(s + (spread ? 4 : 1));
            }
            d = (uint16_t)(row + pitch);
        }
        si = spread ? (uint16_t)(si + 1) : s;
        m = (uint8_t)(m << 1 | m >> 7);
    }
}

/* S29C1:02D5: x, y, w, h cut to DATA:00EC-00F2; S29C1:000A FFFFh when
 * something was cut, and x and y FFFFh when nothing is left */
static void sprite_cut(int16_t *x, int16_t *y, int16_t *w, int16_t *h)
{
    int16_t l = (int16_t)DW(0x00EC), t = (int16_t)DW(0x00EE), r = (int16_t)DW(0x00F0), b = (int16_t)DW(0x00F2), e;

    SCW(0x0A, 0);
    if (*x < l) {
        SCW(0x0A, 0xFFFF);
        *w = (int16_t)(*w - (l - *x));
        if (*w <= 0)
            goto none;
        *x = l;
    } else if (*x > r)
        goto none;
    if (*y < t) {
        SCW(0x0A, 0xFFFF);
        *h = (int16_t)(*h - (t - *y));
        if (*h <= 0)
            goto none;
        *y = t;
    } else if (*y > b)
        goto none;
    e = (int16_t)(*h + *y);
    if (e > b) {
        SCW(0x0A, 0xFFFF);
        *h = (int16_t)(*h - (e - b));
        if (*h <= 0)
            goto none;
    }
    e = (int16_t)(*w + *x);
    if (e > r) {
        SCW(0x0A, 0xFFFF);
        *w = (int16_t)(*w - (e - r));
        if (*w <= 0)
            goto none;
    }
    return;
none:
    SCW(0x0A, 0xFFFF);
    *x = *y = -1;
}

static uint16_t swapb(uint16_t v)
{
    return (uint16_t)(v << 8 | v >> 8);
}

/* S29C1:001F: picture `pic` (pixels at 1Ah, a row of word 12h, word 14h
 * rows, byte 0Ch transparent) centred on x, y, scaled by 100h / sx and
 * 100h / sy (negative sx mirrored, negative sy upside down), `base`
 * added to its colours, drawn column by column from its right and bottom;
 * with `save` not 0:0 what it covers kept first (S2AE4:000C), cut to
 * DATA:00EC-00F2 when DATA:00E8 is set.  Its variables are kept in its
 * own segment as the original keeps them. */
void sprite_draw(int16_t x, int16_t y, Far pic, Far save, uint16_t base, int16_t sx, int16_t sy)
{
    uint16_t es = NW(screen_seg, 0), h = pw(pic, 0x14), w = pw(pic, 0x12), sw, sh, cx, di, si, mirror = 0;
    uint8_t trans = pb(pic, 0x0C), mask;
    int16_t ax, bx;

    SCW(0x06, DW(0x02F0));
    fwb(S_SPR, 0x1E, (uint8_t)DW(0x02F0));
    SCW(0x0A, DW(0x00E8));
    SCW(0x08, base);
    if (sx < 0) {
        sx = (int16_t)-sx;
        mirror = 0xFFFF;
    }
    SCW(0x1C, 0xFFFF);
    if (sy < 0) {
        SCW(0x06, (uint16_t)-CW(0x06));
        sy = (int16_t)-sy;
        SCW(0x1C, 0xFFFF);
    }
    sh = (uint16_t)(((uint32_t)h << 16) / (uint16_t)sy >> 8 & 0xFF);
    y = (int16_t)(y - (sh >> 1));
    sw = (uint16_t)(((uint32_t)w << 16) / (uint16_t)sx >> 8 & 0xFF);
    x = (int16_t)(x - (sw >> 1));
    ax = x;
    bx = y;
    if (!mirror)
        ax = (int16_t)(ax + sw);
    if (!(CW(0x06) & 0x8000))
        bx = (int16_t)(bx + sh);
    SCW(0x14, ax);
    SCW(0x16, bx);
    if (save.seg | save.off) {
        int16_t cw = (int16_t)(sw + 2), ch = (int16_t)(sh + 2);

        ax = (int16_t)(x - 1);
        bx = (int16_t)(y - 1);
        if (CW(0x0A)) {
            sprite_cut(&ax, &bx, &cw, &ch);
            ch++;
            if (ax < 0)
                goto done;
        }
        di = pixel_at((uint16_t)ax, (uint16_t)bx, &mask);
        under_save((uint16_t)cw, (uint16_t)ch, far_of(es, di), save);
        ax = (int16_t)CW(0x14);
        bx = (int16_t)CW(0x16);
    }
    di = pixel_at((uint16_t)ax, (uint16_t)bx, &mask);
    si = (uint16_t)(pic.off + 0x1A);
    cx = swapb((uint16_t)(w - 1));
    if (!CW(0x0A)) {
        for (;;) {
            uint16_t rcx = swapb((uint16_t)(h - 1)), d = di, s = (uint16_t)(si + (cx >> 8));

            vga_outw(0x3C4, (uint16_t)(mask << 8 | 2));
            for (;;) {
                uint8_t c = frb(pic.seg, (uint16_t)(s + (uint8_t)w * (rcx >> 8)));
                if (c != trans)
                    vwb(es, d, (uint8_t)(c + frb(S_SPR, 0x08)));
                d = (uint16_t)(d - CW(0x06));
                if (rcx < (uint16_t)sy)
                    break;
                rcx = (uint16_t)(rcx - sy);
            }
            if (!mirror) {
                uint8_t cf = mask & 1;
                mask = (uint8_t)(mask >> 1 | mask << 7);
                di = (uint16_t)(di - cf);
            } else {
                uint8_t cf = mask >> 7;
                mask = (uint8_t)(mask << 1 | mask >> 7);
                di = (uint16_t)(di + cf);
            }
            if (cx < (uint16_t)sx)
                break;
            cx = (uint16_t)(cx - sx);
        }
        goto done;
    }
    SCW(0x0C, DW(0x00EC));
    SCW(0x0E, DW(0x00F0));
    SCW(0x10, DW(0x00EE));
    SCW(0x12, DW(0x00F2));
    SCW(0x18, CW(0x14));
    for (;;) {
        int16_t col;

        vga_outw(0x3C4, (uint16_t)(mask << 8 | 2));
        SCW(0x1A, CW(0x16));
        col = (int16_t)CW(0x18);
        if (col < (int16_t)CW(0x0C)) {
            if (!mirror)
                break;
        } else if (col > (int16_t)CW(0x0E)) {
            if (mirror)
                break;
        } else {
            uint16_t rcx = swapb((uint16_t)(h - 1)), s = (uint16_t)(si + (cx >> 8));

            for (;;) {
                int16_t row = (int16_t)CW(0x1A);
                if (row >= (int16_t)CW(0x10) && row <= (int16_t)CW(0x12)) {
                    uint16_t d = (uint16_t)((CW(0x18) >> 2) + (uint8_t)row * frb(S_SPR, 0x1E));
                    uint8_t c = frb(pic.seg, (uint16_t)(s + (uint8_t)w * (rcx >> 8)));
                    if (c != trans)
                        vwb(es, d, (uint8_t)(c + frb(S_SPR, 0x08)));
                }
                SCW(0x1A, CW(0x1A) + CW(0x1C));
                if (rcx < (uint16_t)sy)
                    break;
                rcx = (uint16_t)(rcx - sy);
            }
        }
        if (!mirror) {
            uint8_t cf = mask & 1;
            mask = (uint8_t)(mask >> 1 | mask << 7);
            di = (uint16_t)(di - cf);
            SCW(0x18, CW(0x18) - 1);
        } else {
            uint8_t cf = mask >> 7;
            mask = (uint8_t)(mask << 1 | mask >> 7);
            di = (uint16_t)(di + cf);
            SCW(0x18, CW(0x18) + 1);
        }
        if (cx < (uint16_t)sx)
            break;
        cx = (uint16_t)(cx - sx);
    }
done:
    vga_outw(0x3C4, 0x0F02);
}

/* ---- fight_pictures' parts */

static Far rec92(void)
{
    return far_at(S_DATA, 0x0092);
}

/* the page drawn, as fight_frames reads it: word 27Ah of the record
 * DATA:0092 points to (screen_seg when that is DATA:00DA) */
static uint16_t page_seg(void)
{
    Far r = rec92();

    return frw(r.seg, (uint16_t)(r.off + 0x27A));
}

static void rec_set(uint16_t o, uint16_t v)
{
    Far r = rec92();

    fww(r.seg, (uint16_t)(r.off + o), v);
}

static Far palette(void)
{
    return far_at(S_palette_ptr, A_palette_ptr);
}

/* CODE:2C39, strncpy */
static void str_ncopy(Far to, Far from, uint16_t n)
{
    uint16_t i = 0;

    for (; i < n && pb(from, i); i++)
        pwb(to, i, pb(from, i));
    for (; i < n; i++)
        pwb(to, i, 0);
}

/* CODE:2BAA, strcpy, and CODE:2B66, strcat */
static void str_copy(Far to, Far from)
{
    uint16_t i = 0;

    do
        pwb(to, i, pb(from, i));
    while (pb(from, i++));
}

static void str_cat(Far to, Far from)
{
    uint16_t i = 0;

    while (pb(to, i))
        i++;
    to.off = (uint16_t)(to.off + i);
    str_copy(to, from);
}

/* S2532:01C2: who is still standing on either side (0688h, 068Fh: 8 for
 * each of the count, else 4) and the shown counts (04D4h, 04DBh) */
static void pics_counts(void)
{
    uint16_t i;

    for (i = 0; i < 7; i++) {
        if (i <= FW(0x063D)) {
            SFB(0x0688 + i, 8);
            SFB(0x04D4 + i, 1);
        } else {
            SFB(0x0688 + i, 4);
            SFB(0x04D4 + i, 0);
        }
        if (i <= FW(0x063B)) {
            SFB(0x068F + i, 8);
            SFB(0x04DB + i, 1);
        } else {
            SFB(0x068F + i, 4);
            SFB(0x04DB + i, 0);
        }
    }
    SFW(0x067E, 7);
    SFB(0x04DB, 0x64);
    SFB(0x04D4, 0x64);
}

/* S2532:0258: the seven far words of a side at 05CAh: 0:1 for the kinds
 * 18h to 1Bh and 20h to 24h (nothing kept under them), else 0:0 */
static void pics_kind(uint16_t side, int16_t kind)
{
    uint16_t lo = ((kind > 0x17 && kind < 0x1C) || (kind > 0x1F && kind < 0x25)) ? 1 : 0, i;

    for (i = 0; i < 7; i++) {
        SFW(0x05CA + side * 0x1C + i * 4, lo);
        SFW(0x05CC + side * 0x1C + i * 4, 0);
    }
}

/* S2532:02D6: the cut rectangle the whole screen, cutting off, the page
 * cleared */
static void pics_clear(void)
{
    rec_set(0x0E, 0);
    rec_set(0x12, 0);
    rec_set(0x16, 0x13F);
    rec_set(0x14, 0);
    rec_set(0x18, 0xC7);
    rec_set(0x00, 0);
    screen_clear();
}

/* lib_read as the original calls it: the place given, which it does not
 * move on itself; whether it failed */
static int pics_read(Far place, Far file, Far names, Far ptrs, Far work)
{
    return lib_read(&place, file, names, ptrs, work) == -1;
}

/* S2532:0357: the fight's file (S3AEE:0061) loaded to the free place
 * (069Dh), the names of its two units' records and of the two grounds'
 * copied out, and the three libraries' first pictures read (06A1h: where
 * each begins, 06A5h and 06A9h: the two lists in it named by its bytes 0
 * and 2).  `tmp` is the original's [BP-28h] on its stack. */
static int pics_load(Far tmp)
{
    Far p, u0, u1, g0, g1;
    int32_t n;
    uint16_t i;

    SFFAR(0x069D, FFAR(0x062E));
    p = FFAR(0x069D);
    n = file_load_size(p, FP(0x0061), FFAR(0x062A));
    SFW(0x064D, (uint16_t)n);
    SFW(0x064F, (uint16_t)((uint32_t)n >> 16));
    if (n == -1)
        return 2;
    SFFAR(0x062E, far_add(FFAR(0x062E), (uint32_t)n));
    u0 = far_add(p, pb(p, 0));
    u0.off = (uint16_t)(u0.off + FW(0x0645) * 0x14);
    u1 = far_add(p, pb(p, 0));
    u1.off = (uint16_t)(u1.off + FW(0x0643) * 0x14);
    g0 = far_add(p, pw(p, 4));
    g0.off = (uint16_t)(g0.off + FW(0x0641) * 0x18);
    g1 = far_add(p, pw(p, 4));
    g1.off = (uint16_t)(g1.off + FW(0x063F) * 0x18);
    str_ncopy(FP(0x007C), u0, 8);
    str_ncopy(FP(0x0085), u1, 8);
    u0.off = (uint16_t)(u0.off + 0x0A);
    str_ncopy(FP(0x00B5), u0, 4);
    u1.off = (uint16_t)(u1.off + 0x10);
    str_ncopy(FP(0x00BF), u1, 4);
    u0.off = (uint16_t)(u0.off - 0x0A);
    u1.off = (uint16_t)(u1.off - 0x10);
    SFB(0x00B5, FB(0x00C0) == 'C' || FB(0x00C0) == 'Y' ? 'F' : 'V');
    SFB(0x00BF, FB(0x00B6) == 'C' || FB(0x00B6) == 'Y' ? 'B' : 'H');
    str_ncopy(FP(0x009A), g0, 8);
    g1.off = (uint16_t)(g1.off + 0x0C);
    str_ncopy(FP(0x00A3), g1, 8);
    g1.off = (uint16_t)(g1.off - 0x0C);
    SFW(0x008E, pw(g0, 8));
    SFW(0x0090, pw(g0, 0x0A));
    SFW(0x0092, pw(g1, 0x14));
    SFW(0x0094, pw(g1, 0x16));
    SFW(0x006D, pw(u0, 8));
    SFW(0x006F, pw(u1, 0x0E));
    for (i = 0; i < 3; i++) {
        Far at = FFAR(0x062E), to = at;

        SFW(0x066C, i);
        SFFAR(0x06A1 + i * 0x0C, at);
        if (lib_read(&to, FP(0x012F), FP(0x00B5 + i * 0x0A), tmp, FFAR(0x0622)) == -1)
            return 2;
        SFFAR(0x06A5 + i * 0x0C, far_add(at, pb(at, 0)));
        SFFAR(0x06A9 + i * 0x0C, far_add(at, pb(far_add(at, 2), 0)));
        SFFAR(0x062E, to);
    }
    SFW(0x066C, 3);
    return 0;
}

/* S2532:0755: the figures' pictures of the three libraries (S3AEE:0036,
 * named by the list at 06A5h), their places put in the table at 06D1h,
 * 12h a library; how many at 0682h */
static int pics_figures(void)
{
    uint16_t i;

    for (i = 0; i < 3; i++) {
        Far list = FFAR(0x062A), to = FFAR(0x062E), tab = FFAR(0x06D1);

        SFW(0x066C, i);
        if (lib_read(&to, FP(0x0036), FFAR(0x06A5 + i * 0x0C), list, FFAR(0x0622)) == -1)
            return 2;
        SFFAR(0x062E, to);
        SFW(0x0680, 0);
        while (pw(list, 0) | pw(list, 2)) {
            uint16_t k = (uint16_t)((i * 0x12 + FW(0x0680)) * 4);

            SFW(0x0680, FW(0x0680) + 1);
            fww(tab.seg, (uint16_t)(tab.off + k), pw(list, 0));
            fww(tab.seg, (uint16_t)(tab.off + k + 2), pw(list, 2));
            list = far_add(list, 4);
        }
        SFW(0x0682 + i * 2, FW(0x0680));
    }
    SFW(0x066C, 3);
    return 0;
}

/* S2532:1CF9: a side's mark n (7 pixels apart, the second side's 105h
 * further right) in colour c: a box 5 to 0Bh, y 22h to 25h, and the pixel
 * at 5, 21h */
static void pics_mark(uint16_t side, uint16_t n, uint16_t c)
{
    uint16_t x = (uint16_t)(n * 7 + side * 0x105);

    rec_set(0, c);
    rec_set(4, 0x22);
    rec_set(8, 0x25);
    rec_set(2, (uint16_t)(x + 5));
    rec_set(6, (uint16_t)(x + 0x0B));
    rect_fill();
    rec_set(4, 0x21);
    rec_set(2, (uint16_t)(x + 5));
    pixel_put();
}

/* the count of a side (a digit at S3AEE:010F) at x, y 21h */
static void pics_count(uint16_t n, uint16_t x)
{
    SFB(0x010F, n + '0');
    text_draw(x, 0x21, 0, 6, 1, FP(0x010F), NULL);
}

/* S2532:0877: the start: the ground's picture (S3AEE:0002 + 0Dh each:
 * both units of kind 7, and the winter's two further on), unless both are
 * of kind 7 the grounds' pictures of the two (colour 32h in winter), the
 * frames of S3AEE:0073, the flags, the units' pictures, the marks and
 * the counts; then cutting on, the page shown and copied */
static int pics_start(void)
{
    Far none = {0, 0}, pal = palette();
    uint16_t di, i;
    int both7 = FW(0x063F) == 7 && FW(0x0641) == 7;

    SFW(0x0140, 0);
    pal_level(0, pal);
    di = both7 ? 1 : 0;
    if (FW(0x0647))
        di = (uint16_t)(di + 2);
    lbm_show(FFAR(0x062E), FP(0x0002 + di * 0x0D), FFAR(0x062A));
    if (!both7) {
        if (FW(0x0647))
            SFB(0x00AA, 0x32);
        if (pics_read(FFAR(0x062E), FP(0x0052), FP(0x00A3), FFAR(0x062A), FFAR(0x0622)))
            return 2;
        pic_draw(FW(0x0092), FW(0x0094), FFAR(0x062E), 0);
        if (FW(0x0647))
            SFB(0x00A1, 0x32);
        if (pics_read(FFAR(0x062E), FP(0x0052), FP(0x009A), FFAR(0x062A), FFAR(0x0622)))
            return 2;
        pic_draw(FW(0x008E), (uint16_t)(FW(0x0090) + 1), FFAR(0x062E), 0);
    }
    if (pics_read(FFAR(0x062E), FP(0x0036), FP(0x0073), FFAR(0x062A), FFAR(0x0622)))
        return 2;
    sprite_draw(0x1D, 0x14, FFAR(0x062E), none, 0, 0x100, 0x100);
    sprite_draw(0x122, 0x14, FFAR(0x062E), none, 0, 0x100, 0x100);
    if (!FB(0x0636)) {
        flag_draw(0x2B, 5, 0, 3, 0x11, 0);
        flag_draw(0x130, 5, 5, 3, 0x11, 1);
    } else {
        flag_draw(0x130, 5, 0, 3, 0x11, 0);
        flag_draw(0x2B, 5, 5, 3, 0x11, 1);
    }
    if (pics_read(FFAR(0x062E), FP(0x0044), FP(0x007C), FFAR(0x062A), FFAR(0x0622)))
        return 2;
    sprite_draw(0x1D, 0x10, FFAR(0x062E), none, 0, 0x100, 0x100);
    if (pics_read(FFAR(0x062E), FP(0x0044), FP(0x0085), FFAR(0x062A), FFAR(0x0622)))
        return 2;
    sprite_draw(0x122, 0x10, FFAR(0x062E), none, 0, 0x100, 0x100);
    for (i = 0; i < FW(0x063D); i++)
        pics_mark(0, i, 0x1F);
    for (i = 0; i < FW(0x063B); i++)
        pics_mark(1, i, 0x1F);
    pics_count(FW(0x063D), 0x33);
    pics_count(FW(0x063B), 0x138);
    rec_set(0x0E, 1);
    page_flip();
    page_copy();
    return 0;
}

/* S2532:1E66: a figure's sound `slot` named in its record (at 0, or at
 * 0Ah with S3303:0022 set; a blank: none) loaded behind the others; where
 * at 04B4h, its word 14h at 04A4h */
static int pics_sound(uint16_t slot, Far rec)
{
    int sb = frw(HL_S3303 + HL_BASE, 0x0022) != 0;
    int32_t n;

    if (pb(rec, sb ? 0x0A : 0) == ' ') {
        SFW(0x04B4 + slot * 4, 0);
        SFW(0x04B6 + slot * 4, 0);
        return 0;
    }
    SFFAR(0x04B4 + slot * 4, FFAR(0x062E));
    SFW(0x04A4 + slot * 2, pw(rec, 0x14));
    str_copy(FP(0x04E2), FP(0x0111));
    if (!sb) {
        str_cat(FP(0x04E2), rec);
        str_cat(FP(0x04E2), FP(0x0115));
    } else {
        rec.off = (uint16_t)(rec.off + 0x0A);
        str_cat(FP(0x04E2), rec);
        str_cat(FP(0x04E2), FP(0x011A));
    }
    n = file_load_size(FFAR(0x062E), FP(0x04E2), FFAR(0x062A));
    SFW(0x064D, (uint16_t)n);
    SFW(0x064F, (uint16_t)((uint32_t)n >> 16));
    if (!n)
        return 2;
    SFFAR(0x062E, far_add(FFAR(0x062E), (uint32_t)(n + 1) & 0xFFFFFFFEu));
    return 0;
}

/* S2532:1FD6: the six sounds of the two units' kinds (the fight's file's
 * tables at its words 8 and 0Ah, 16h bytes a sound) */
static int pics_sounds(void)
{
    Far p = FFAR(0x069D), a = far_add(p, pw(p, 8)), b = a, names = far_add(p, pw(p, 0x0A));
    uint16_t i;

    a.off = (uint16_t)(a.off + FW(0x0645) * 4);
    b.off = (uint16_t)(b.off + FW(0x0643) * 4);
    for (i = 0; i < 3; i++) {
        Far r = names;
        r.off = (uint16_t)(r.off + pb(a, i) * 0x16);
        if (pics_sound(i, r))
            return 2;
    }
    for (i = 0; i < 3; i++) {
        Far r = names;
        r.off = (uint16_t)(r.off + pb(b, i) * 0x16);
        if (pics_sound((uint16_t)(4 + i), r))
            return 2;
    }
    return 0;
}

/* S2532:1970: up to `count` of the other side's figures that still stand
 * (byte 8 at 0688h) and can fall (bit 2 of their record), each time the
 * one nearest in x to the figure shot (word 0Ch), marked 2 (fallen); how
 * many */
static uint16_t figures_fall(uint16_t count, uint16_t other)
{
    Far shot = FFAR(0x06CD);
    uint16_t n = 0, k, x = pw(shot, 0x0C);

    for (k = 1; k < 7 && n < count; k++) {
        Far p = FFAR(0x06A9 + other * 0x0C);
        uint16_t best = 0xFA00, who = 0;

        for (; !(pb(p, 0) & 0x80); p.off = (uint16_t)(p.off + 0x22)) {
            if ((pb(p, 0) & 2) && FB(0x0688 + other * 7 + pb(p, 1)) == 8) {
                int16_t d = (int16_t)(pw(p, 0x0C) - x);
                uint16_t a = (uint16_t)(d < 0 ? -d : d);
                if (a < best) {
                    best = a;
                    who = pb(p, 1);
                }
            }
        }
        if (who) {
            SFB(0x0688 + other * 7 + who, 2);
            n++;
        }
    }
    return n;
}

/* S2532:17D1: a volley of side 066Ch ends: the other side's losses still
 * to show (its count less the count after) shared out among the volleys
 * left (0628h for side 0's, 0626h for side 1's): all of them at the last
 * volley or when one is left, else a number below them (rand_next) when
 * more are left than volleys, else one in a draw against 80h
 * (rand_next); then 013Eh 2 (the marks redrawn) */
static void volley_end(void)
{
    uint16_t side = FW(0x066C), other = (uint16_t)((side + 1) & 1);
    uint16_t cnt = side ? 0x063D : 0x063B, after = side ? 0x0639 : 0x0637, left = side ? 0x0626 : 0x0628, si;

    si = (uint16_t)(FW(cnt) - FW(after));
    if (si) {
        if (FW(left) == 1)
            SFW(cnt, FW(cnt) - figures_fall(si, other));
        else if (si > FW(left)) {
            uint16_t r = (uint16_t)rand_next();
            SFW(cnt, FW(cnt) - figures_fall((uint16_t)(r % si), other));
        } else if (((uint16_t)rand_next() & 0xFF) > 0x80)
            SFW(cnt, FW(cnt) - figures_fall(1, other));
    }
    SFW(left, FW(left) - 1);
    si = (uint16_t)(FW(cnt) - FW(after));
    if (!FW(left) && si)
        SFW(cnt, FW(cnt) - figures_fall(si, other));
    SFW(0x013E, 2);
}

/* long_mul and long_div of the figure's way: from + (to - from) * t / n */
static int16_t along(uint16_t store, int16_t from, int16_t to, int16_t t, int16_t n)
{
    int32_t v = (int32_t)(int16_t)(to - from) * t;

    SFW(store, (uint16_t)v);
    SFW(store + 2, (uint16_t)((uint32_t)v >> 16));
    if (!n) {
        fprintf(stderr, "fight_frames: a figure's way of 0 frames (long_div by 0, not followed)\n");
        return from;
    }
    return (int16_t)(v / n + from);
}

/* the 16-byte entry k of the frame's list at S3AEE:0144 */
#define L(k, o) (0x0144 + (k) * 0x10 + (o))

/* fight_frames (S2532:0CAA): see the hints.  Returns 1 when the key ended
 * the pictures. */
static int fight_frames(void)
{
    Far key = far_at(S_DATA, 0x00BE), pal = palette();
    int draw = 0;
    int16_t frame;

    SFW(0x0628, FW(0x063D));
    SFW(0x0626, FW(0x063B));
    SFW(0x0670, 0);
    for (SFW(0x0680, 1); (int16_t)FW(0x0680) < 0x2EE; SFW(0x0680, FW(0x0680) + 1)) {
        int16_t side;
        uint16_t k, si;

        frame = (int16_t)FW(0x0680);
        if (frame < 0x40) {
            SFW(0x0140, frame * 4 + 3);
            pal_level((uint8_t)FW(0x0140), pal);
        }
        k = 0;
        SFW(0x0142, 0);
        draw = !FW(0x013C);
        for (SFW(0x066C, 2); (int16_t)(side = (int16_t)FW(0x066C)) >= 0; SFW(0x066C, FW(0x066C) - 1)) {
            SFFAR(0x06CD, FFAR(0x06A9 + side * 0x0C));
            SFW(0x067E, 0);
            for (;;) {
                Far rec = FFAR(0x06CD), pic, mark;
                uint8_t fl = pb(rec, 0);
                uint16_t di = pb(rec, 1);
                int16_t start, end, x, y;

                if (fl & 0x80)
                    break;
                SFB(0x0665, fl);
                SFW(0x0668, pw(rec, 2) + FW(0x006D + side * 2));
                SFW(0x0666, pw(rec, 4) + FW(0x006D + side * 2));
                start = (int16_t)FW(0x0668);
                end = (int16_t)FW(0x0666);
                if (start > frame || end < frame || !(FB(0x0688 + side * 7 + di) & fl)
                    || ((fl & 0x40) && FB(0x013B)))
                    goto next;
                SFW(0x0674, frame - start);
                SFW(0x0676, end - start);
                x = along(0x0659, (int16_t)pw(rec, 6), (int16_t)pw(rec, 0x0C), (int16_t)FW(0x0674), (int16_t)FW(0x0676));
                y = along(0x0655, (int16_t)pw(rec, 8), (int16_t)pw(rec, 0x0E), (int16_t)FW(0x0674), (int16_t)FW(0x0676));
                SFW(0x067C, along(0x0661, (int16_t)pw(rec, 0x12), (int16_t)pw(rec, 0x16), (int16_t)FW(0x0674), (int16_t)FW(0x0676)));
                SFW(0x067A, along(0x065D, (int16_t)pw(rec, 0x14), (int16_t)pw(rec, 0x18), (int16_t)FW(0x0674), (int16_t)FW(0x0676)));
                SFW(0x0678, 0);
                if ((fl & 0x10) && end == frame)
                    volley_end();       /* then its sound, S2532:211E: left out */
                /* fl & 20h at the start: a sound, left out */
                if ((fl & 2) && (fl & 1) && end > 0x2EE)
                    fww(rec.seg, (uint16_t)(rec.off + 4), (uint16_t)(frame + 2));
                if ((fl & 1) && frame + 1 >= end) {
                    x = (int16_t)pw(rec, 0x0C);
                    y = (int16_t)pw(rec, 0x0E);
                    SFW(0x067C, pw(rec, 0x16));
                    SFW(0x067A, pw(rec, 0x18));
                    SFW(0x0678, pw(rec, 0x1C));
                    SFW(0x064B, 0);
                    SFW(0x0649, 0);
                    if (side < 2)
                        SFB(0x04D4 + side * 7 + di, 0);
                } else {
                    SFW(0x064B, 0);
                    SFW(0x0649, 1);
                }
                if (pw(rec, 0x20) == 0xFFFF) {
                    Far tab = FFAR(0x06D1);
                    pic = far_at(tab.seg, (uint16_t)(tab.off + (side * 0x12 + pw(rec, 0x1E)) * 4));
                } else {
                    Far p, tab = FFAR(0x06D1);
                    uint16_t n = pw(rec, 0x20);

                    fww(rec.seg, (uint16_t)(rec.off + 0x20), (uint16_t)(n + 1));
                    SFW(0x066A, n);
                    p = far_adds(far_adds(FFAR(0x06A1 + side * 0x0C), (int16_t)pw(rec, 0x1E)), (int16_t)(n * 2));
                    SFW(0x066A, pw(p, 0));
                    if (FW(0x066A) == 0xFFFF) {
                        fww(rec.seg, (uint16_t)(rec.off + 0x20), 0);
                        p = far_adds(FFAR(0x06A1 + side * 0x0C), (int16_t)pw(rec, 0x1E));
                        SFW(0x066A, pw(p, 0));
                    }
                    pic = far_at(tab.seg, (uint16_t)(tab.off + (side * 0x12 + FW(0x066A)) * 4));
                }
                SFFAR(0x06C5, pic);
                mark = FFAR(0x05CA + side * 0x1C + di * 4);
                if (di > 0 && di < 7 && side < 2 && !(FW(0x0649) | FW(0x064B))) {
                    uint16_t o = (uint16_t)(0x0522 + side * 0x38 + di * 8);
                    Far scr;

                    if (!(mark.seg | mark.off)) {
                        /* the figure lies still: what lies under it kept
                         * (S2B76:0004) once, put back when it fell */
                        int16_t a = (int16_t)FW(0x067C), b = (int16_t)FW(0x067A), w, h, px, py;

                        w = (int16_t)((int16_t)(pb(far_add(pic, 0x12), 0) << 8) / (int16_t)(a < 0 ? -a : a));
                        h = (int16_t)((int16_t)(pb(far_add(pic, 0x14), 0) << 8) / (int16_t)(b < 0 ? -b : b));
                        px = (int16_t)(x - (w >> 1));
                        py = (int16_t)(y - (h >> 1));
                        scr.seg = page_seg();
                        scr.off = (uint16_t)(py * 0x50 + px / 4);
                        if (py + h > 0xC7)
                            h = (int16_t)(0xC7 - py);
                        SFW(o, px);
                        SFW(o + 2, py);
                        SFW(o + 4, w);
                        SFW(o + 6, h);
                        SFFAR(0x05CA + side * 0x1C + di * 4, FFAR(0x061E));
                        SFFAR(0x061E, screen_keep(scr, FFAR(0x061E), (uint16_t)((w >> 2) + 2), (uint16_t)(h + 1), 0));
                    } else if ((uint16_t)(mark.seg + (mark.off >> 4)) > 0 || (mark.off & 0x0F) > 1) {
                        if (FB(0x0688 + side * 7 + di) == 2) {
                            int16_t px = (int16_t)FW(o), py = (int16_t)FW(o + 2);

                            scr.seg = page_seg();
                            scr.off = (uint16_t)(py * 0x50 + px / 4);
                            screen_put(mark, scr, (uint16_t)(((int16_t)FW(o + 4) >> 2) + 2), (uint16_t)(FW(o + 6) + 1), 0);
                        }
                    }
                }
                SFW(L(k, 0), x);
                SFW(L(k, 2), y);
                SFFAR(L(k, 4), FFAR(0x06C5));
                SFW(L(k, 8), FW(0x0649));
                SFW(L(k, 0x0A), FW(0x064B));
                SFW(L(k, 0x0C), FW(0x067C));
                SFW(L(k, 0x0E), FW(0x067A));
                k++;
                SFW(0x0142, FW(0x0142) + 1);
                SFW(0x0670, FW(0x0670) + 1);
next:
                SFW(0x06CD, FW(0x06CD) + 0x22);
                SFW(0x067E, FW(0x067E) + 1);
            }
            if (side < 2) {
                uint8_t n = 0;

                if ((!FW(0x063D) && side == 0) || (!FW(0x063B) && side == 1)) {
                    if (FB(0x04D4 + side * 7) != 0x64)
                        SFB(0x04D4 + side * 7, 0x64);       /* and two sounds, left out */
                } else {
                    for (SFW(0x067E, 1); FW(0x067E) < 7; SFW(0x067E, FW(0x067E) + 1))
                        if (FB(0x04D4 + side * 7 + FW(0x067E)))
                            n++;
                    if (FB(0x04D4 + side * 7) != n)
                        SFB(0x04D4 + side * 7, n);          /* and a sound, left out */
                }
            }
        }
        if (pics_hook)
            pics_hook(PICS_FRAME);
        if (pw(key, 4))
            return 1;
        if ((int16_t)FW(0x013E) > 0) {
            uint16_t n = 0;

            for (si = 1; si < 7; si++)
                if (FB(0x0688 + si) == 2)
                    pics_mark(0, n++, 0x1E);
            n = 0;
            for (si = 1; si < 7; si++)
                if (FB(0x068F + si) == 2)
                    pics_mark(1, n++, 0x1E);
            rec_set(0, 3);
            rec_set(4, 0x21);
            rec_set(8, 0x25);
            rec_set(2, 0x2F);
            rec_set(6, 0x36);
            rect_fill();
            pics_count(FW(0x063D), 0x33);
            rec_set(0, 3);
            rec_set(4, 0x21);
            rec_set(8, 0x25);
            rec_set(2, 0x134);
            rec_set(6, 0x13B);
            rect_fill();
            pics_count(FW(0x063B), 0x138);
            draw = 1;
            SFW(0x013E, FW(0x013E) - 1);
        }
        if (FW(0x013C) && !draw) {
            for (si = 0; si < FW(0x0142); si++)
                if (!(FW(L(si, 8)) | FW(L(si, 0x0A)))) {
                    draw = 1;
                    break;
                }
            if (frame % 2 > 0 && !draw)
                draw = 1;
        }
        if (draw) {
            for (si = 0; si < FW(0x0142); si++)
                sprite_draw((int16_t)FW(L(si, 0)), (int16_t)FW(L(si, 2)), FFAR(L(si, 4)),
                            far_of(FW(L(si, 0x0A)), FW(L(si, 8))), 0,
                            (int16_t)FW(L(si, 0x0C)), (int16_t)FW(L(si, 0x0E)));
            page_flip();
            unders_restore();
        }
    }
    return 0;
}

/* fight_pictures (S2532:0008).  Returns 2 where the original goes to its
 * error exit (S2532:1DD3, which ends the program). */
int fight_pictures(const FightPics *a)
{
    Far pal = palette(), tmp;
    uint16_t i;

    SFB(0x013B, a->near0);
    SFB(0x0636, a->standing);
    SFW(0x013C, a->show);
    SFW(0x0647, a->winter);
    SFW(0x0645, a->kind0);
    SFW(0x0643, a->kind1);
    SFW(0x0641, a->ground0);
    SFW(0x063F, a->ground1);
    SFW(0x063D, a->count0);
    SFW(0x063B, a->count1);
    SFW(0x0639, a->after0);
    SFW(0x0637, a->after1);
    SFFAR(0x0632, a->none);
    SFFAR(0x0622, a->place);
    SFFAR(0x062E, far_add(a->place, 0x7D00));
    SFFAR(0x061E, a->place);
    SFFAR(0x062A, a->work);
    SFFAR(0x06D1, FFAR(0x062E));
    SFFAR(0x062E, far_add(FFAR(0x062E), 0xD8));
    pics_counts();
    pics_kind(0, (int16_t)FW(0x0645));
    pics_kind(1, (int16_t)FW(0x0643));
    if (pics_hook)
        pics_hook(PICS_SEED);
    else
        clock_seed_bios();
    pics_clear();
    /* lib_read's list of places on the original's stack ([BP-28h] of
     * S2532:0357, below fight_pictures' frame) */
    tmp.seg = HL_STACK + HL_BASE;
    tmp.off = (uint16_t)(a->sp - 0x32);
    if (pics_load(tmp) || pics_start() || pics_sounds() || pics_figures())
        return 2;
    page_copy();
    page_flip();
    if (pics_hook)
        pics_hook(PICS_LOADED);
    {
        Far key = far_at(S_DATA, 0x00BE);
        fww(key.seg, (uint16_t)(key.off + 4), 0);
    }
    fight_frames();
    while ((int16_t)FW(0x0140) > 0) {
        pal_level((uint8_t)FW(0x0140), pal);
        SFW(0x0140, FW(0x0140) - 4);
        if ((int16_t)FW(0x0140) < 0)
            SFW(0x0140, 0);
        if (pics_hook)
            pics_hook(PICS_LEVEL);
    }
    /* S2F39:09C9 for the sound's channels 0 to 7 (12 with S3303:0022):
     * left out, but for the count it leaves */
    i = frw(HL_S3303 + HL_BASE, 0x0022) ? 0x0C : 8;
    SFW(0x0680, i);
    if (pics_hook)
        pics_hook(PICS_END);
    return 0;
}
