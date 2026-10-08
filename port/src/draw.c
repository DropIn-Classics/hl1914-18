/* draw.c - drawing a battle's views: HL.EXE's view_draw (S0E56:0D44)
 * with the routines it draws a field's ground (S290A:0006) and a unit
 * (S2931:000E) with, on the runtime's VGA (doskit/runtime/vga.h) and the
 * program's memory.  port/README.md says how this was compared.
 */
#include "hl.h"
#include "vga.h"

#define S_DATA (HL_DATA + HL_BASE)
#define S_GAME (HL_S331F + HL_BASE)
#define DW(o) frw(S_DATA, (o))

/* a byte at seg:off, from the VGA's window at A000h or from memory, as
 * the CPU reads and writes it */
uint8_t vrb(uint16_t seg, uint16_t off)
{
    uint32_t a = lin(seg, off);

    return a >= 0xA0000 && a < 0xB0000 ? vga_read((uint16_t)(a - 0xA0000)) : mem[a];
}

void vwb(uint16_t seg, uint16_t off, uint8_t v)
{
    uint32_t a = lin(seg, off);

    if (a >= 0xA0000 && a < 0xB0000)
        vga_write((uint16_t)(a - 0xA0000), v);
    else
        mem[a] = v;
}

/* the place of pixel x, y on the page drawn: x / 4 and the line's offset
 * from the table at DATA:00F8 */
static uint16_t screen_at(uint16_t x, uint16_t y)
{
    return (uint16_t)((x >> 2) + DW((uint16_t)(0x00F8 + 2 * y)));
}

/* S290A:0006: a field's ground, 16 by 24 pixels, from video memory at
 * src to x, y, in write mode 1 (the latches carry all four planes): the
 * hexagon row by row (what to skip in the source and on the screen
 * before each row's bytes), then the pixels of its slanted edges plane by
 * plane, each latched from the source and written with one map mask */
static void ground_blit(uint16_t x, uint16_t y, Far src)
{
    static const uint8_t rows[24][3] = {
        {2, 0x02, 2}, {4, 0x4E, 2}, {4, 0x4E, 2}, {4, 0x4E, 2}, {4, 0x4E, 2},
        {3, 0x4D, 4}, {2, 0x4C, 4}, {2, 0x4C, 4}, {2, 0x4C, 4}, {2, 0x4C, 4},
        {2, 0x4C, 4}, {1, 0x4B, 6}, {0, 0x4A, 6}, {1, 0x4B, 4}, {2, 0x4C, 4},
        {2, 0x4C, 4}, {2, 0x4C, 4}, {2, 0x4C, 4}, {2, 0x4C, 4}, {3, 0x4D, 2},
        {4, 0x4E, 2}, {4, 0x4E, 2}, {4, 0x4E, 2}, {4, 0x4E, 2}};
    /* map mask, then pairs of source and screen offsets */
    static const struct { uint8_t mask, n; uint16_t at[6][2]; } edges[6] = {
        {0x1, 6, {{0x0A, 0x054}, {0x10, 0x0A4}, {0x82, 0x694}, {0x88, 0x6E4}, {0x2F, 0x235}, {0x65, 0x505}}},
        {0x3, 4, {{0x16, 0x0F4}, {0x7C, 0x644}, {0x35, 0x285}, {0x5F, 0x4B5}}},
        {0x7, 6, {{0x1C, 0x144}, {0x76, 0x5F4}, {0x3B, 0x2D5}, {0x59, 0x465}, {0x41, 0x325}, {0x53, 0x415}}},
        {0x8, 6, {{0x07, 0x051}, {0x0D, 0x0A1}, {0x7F, 0x691}, {0x85, 0x6E1}, {0x2A, 0x230}, {0x60, 0x500}}},
        {0xC, 4, {{0x13, 0x0F1}, {0x79, 0x641}, {0x30, 0x280}, {0x5A, 0x4B0}}},
        {0xE, 6, {{0x19, 0x141}, {0x73, 0x5F1}, {0x36, 0x2D0}, {0x54, 0x460}, {0x3C, 0x320}, {0x4E, 0x410}}}};
    uint16_t es = NW(screen_seg, 0), di0 = screen_at(x, y), di = di0, si = src.off;
    int r, k;

    vga_outw(0x3C4, 0x0F02);
    vga_outw(0x3CE, 0x4105);
    for (r = 0; r < 24; r++) {
        si = (uint16_t)(si + rows[r][0]);
        di = (uint16_t)(di + rows[r][1]);
        for (k = 0; k < rows[r][2]; k++, si++, di++)
            vwb(es, di, vrb(src.seg, si));
    }
    for (r = 0; r < 6; r++) {
        vga_outw(0x3C4, (uint16_t)(edges[r].mask << 8 | 2));
        for (k = 0; k < edges[r].n; k++)
            vwb(es, (uint16_t)(di0 + edges[r].at[k][1]),
                vrb(src.seg, (uint16_t)(src.off + edges[r].at[k][0])));
    }
    vga_outw(0x3CE, 0x4005);
    vga_outw(0x3C4, 0x0F02);
}

/* S2931:000E: a unit's picture, 24 by 24 pixels of 4 bits from byte 12h
 * of src on (plane 0's rows, then plane 1's ...; two pixels a byte, the
 * high half first), each made colour base + the 4 bits; byte 8 of src is
 * the transparent one, a byte of two of them skipped whole */
static void unit_blit(uint16_t x, uint16_t y, uint8_t base, Far src)
{
    uint16_t es = NW(screen_seg, 0), di0 = screen_at(x, y), si = (uint16_t)(src.off + 0x12);
    uint8_t clear = pb(src, 8), both = (uint8_t)(clear << 4 | clear), skip = (uint8_t)(clear + base);
    uint8_t plane;
    int r, b;

    for (plane = 1; plane != 0x10; plane = (uint8_t)(plane << 1)) {
        uint16_t di = di0;
        vga_outw(0x3C4, (uint16_t)(plane << 8 | 2));
        for (r = 0; r < 0x18; r++, di = (uint16_t)(di + 0x50))
            for (b = 0; b < 3; b++) {
                uint8_t c = frb(src.seg, si++), hi, lo;
                if (c == both)
                    continue;
                hi = (uint8_t)((c >> 4) + base);
                lo = (uint8_t)((c & 0x0F) + base);
                if (hi != skip)
                    vwb(es, (uint16_t)(di + 2 * b), hi);
                if (lo != skip)
                    vwb(es, (uint16_t)(di + 2 * b + 1), lo);
            }
    }
    vga_outw(0x3C4, 0x0F02);
    /* its two variables in its own segment: the plane mask, and byte 8 */
    fwb(HL_S2931 + HL_BASE, 0x00F8, 0x10);
    fwb(HL_S2931 + HL_BASE, 0x00F9, clear);
}

/* one field at x, y of the side's half: its ground by the table of far
 * pointers S331F:0153 points to, then its unit (below F1h) by the one
 * S331F:013F points to, picture byte 0Fh + side of its record plus 6
 * times byte 8, in colours 30h when bit 0 of its word 4 is set, else
 * 20h; a unit with 2 in word 6 that is not on the side's side
 * (same_side) is not drawn (as view_draw, field_draw and marks_erase do
 * it, each written out in the original) */
static void field_put(uint16_t x, uint16_t y, Far map, uint16_t at, uint16_t side)
{
    uint8_t unit;
    uint16_t rec, pic;

    ground_blit(x, y, pfar(far_add(far_at(S_GAME, 0x0153), 4u * pb(map, at)), 0));
    unit = pb(map, (uint16_t)(at + 1));
    if (unit > 0xF0)
        return;
    rec = (uint16_t)(A_units + unit * UNIT_SIZE);
    if ((frw(S_units, (uint16_t)(rec + 6)) & 2) && !same_side(frw(S_units, (uint16_t)(rec + 4)), side))
        return;
    pic = (uint16_t)(frb(S_units, (uint16_t)(rec + 0x0F + side)) + frb(S_units, (uint16_t)(rec + 8)) * 6);
    unit_blit(x, y, frw(S_units, (uint16_t)(rec + 4)) & 1 ? 0x30 : 0x20,
              pfar(far_add(far_at(S_GAME, 0x013F), 4u * pic), 0));
}

static Far side_map(uint16_t side)
{
    return side ? far_at(S_map2_ptr, A_map2_ptr) : far_at(S_map_ptr, A_map_ptr);
}

/* S0E56:0D44 (view_draw): the 9 by 7 fields of a side's view from
 * `field` (a byte offset in the map, two bytes a field: ground, unit)
 * on (field_put): the second side's half of the screen lies A0h to the
 * right and takes its map from map2_ptr.  Odd columns lie 0Ch lower. */
void view_draw(uint16_t field, uint16_t side)
{
    Far map = side_map(side);
    uint16_t row_bytes = (uint16_t)(NW(map_width, 0) << 1), x = side ? 0xA0 : 0, col, row;

    for (col = 0; col < 9; col++, x = (uint16_t)(x + 0x10)) {
        uint16_t y = col & 1 ? 0x0C : 0, at = (uint16_t)(field + 2 * col);

        for (row = 0; row < 7; row++, y = (uint16_t)(y + 0x18), at = (uint16_t)(at + row_bytes))
            field_put(x, y, map, at, side);
    }
}

/* S0E56:1217 (marks_erase), its drawing: the fields of the side's view
 * from `field` on (as view_draw's) that have `mask` in field_bits drawn
 * again by field_put, so without their marks.  The caller (move.c)
 * clears the mask after it as the original does. */
void marks_redraw(uint16_t field, uint16_t side, uint8_t mask)
{
    Far map = side_map(side);
    int16_t f = (int16_t)field >> 1, w = (int16_t)NW(map_width, 0);
    uint16_t at0 = (uint16_t)(f % w + (f / w << 6)), row_bytes = (uint16_t)(w << 1);
    uint16_t x = side ? 0xA0 : 0, col, row;

    for (col = 0; col < 9; col++, x = (uint16_t)(x + 0x10)) {
        uint16_t y = col & 1 ? 0x0C : 0, at = (uint16_t)(field + 2 * col), bit = (uint16_t)(at0 + col);

        for (row = 0; row < 7; row++, y = (uint16_t)(y + 0x18), at = (uint16_t)(at + row_bytes), bit = (uint16_t)(bit + 0x40))
            if (NB(field_bits, bit) & mask)
                field_put(x, y, map, at, side);
    }
}

/* S2A1C:0004: the pixels of a field's mark at x, y: for each plane a list
 * of offsets from the field's place (words in its own segment from 008Eh
 * on, each plane's ended by a negative word); a pixel between lo and hi
 * is moved on by `add`.  Keeps the plane mask and the plane at 008Ch and
 * 008Dh.  Its last OUT goes to the port DX holds then, hi and lo (as the
 * original does; the runtime ignores that port), so the map mask stays
 * at plane 3. */
static void mark_blit(uint16_t x, uint16_t y, uint8_t lo, uint8_t hi, uint8_t add)
{
    uint16_t cs = HL_S2A1C + HL_BASE, es = NW(screen_seg, 0), di = screen_at(x, y), si = 0x8E;
    uint8_t mask = 1, plane = 0;

    do {
        int16_t off;
        vga_outw(0x3C4, (uint16_t)(mask << 8 | 2));
        vga_outb(0x3CE, 4);
        vga_outb(0x3CF, plane);
        while ((off = (int16_t)frw(cs, si)) >= 0) {
            uint16_t at = (uint16_t)(di + off);
            uint8_t v = vrb(es, at);
            si = (uint16_t)(si + 2);
            if (v >= lo && v <= hi)
                vwb(es, at, (uint8_t)(vrb(es, at) + add));
        }
        si = (uint16_t)(si + 2);
        plane++;
        mask = (uint8_t)(mask << 1);
    } while (mask != 0x10);
    fwb(cs, 0x008C, mask);
    fwb(cs, 0x008D, plane);
    vga_outw((uint16_t)(hi << 8 | lo), 0x0F02);
}

/* S0E56:24ED: a field's mark at x, y by its bits of field_bits (the
 * side's: 55h or AAh) through the table of three bytes (lo, hi, add) at
 * S330B:0008; the index is made of the bits as the original folds them
 * (bits 0 and 2 to 0, 4 to 1, 6 to 2, after or-ing each pair), 8 more
 * when byte 1Bh of the view is 0 or 4.  An entry with add FFh draws
 * nothing. */
static void field_mark(uint16_t x, uint16_t y, uint16_t bits, Far view)
{
    uint16_t s = (uint16_t)((bits | (int16_t)bits >> 1) & 0x55), i;
    uint16_t t = HL_S330B + HL_BASE;

    s = (uint16_t)((s & 1) | (s & 4) >> 2 | (s & 0x10) >> 3 | (s & 0x40) >> 4);
    if (pb(view, 0x1B) == 4 || pb(view, 0x1B) == 0)
        s |= 8;
    i = (uint16_t)(s * 3);
    if (frb(t, (uint16_t)(0x0A + i)) >= 0xFF)
        return;
    mark_blit(x, y, frb(t, (uint16_t)(0x08 + i)), frb(t, (uint16_t)(0x09 + i)), frb(t, (uint16_t)(0x0A + i)));
}

/* S0E56:0F36 (marks_draw): the marks of the 9 by 7 fields of a side's
 * view from `field` (a byte offset in the map, as view_draw's) by
 * field_bits, 40h fields a row there; the side's bits 55h for the first,
 * AAh for the second, whose half lies A0h to the right. */
void marks_draw(uint16_t field, uint16_t side)
{
    int16_t f = (int16_t)field >> 1, w = (int16_t)NW(map_width, 0);
    uint16_t at0 = (uint16_t)(f % w + (f / w << 6)), x = side ? 0xA0 : 0, col, row;
    uint8_t mine = side ? 0xAA : 0x55;
    Far view;

    view.seg = S_views;
    view.off = (uint16_t)(A_views + side * 0x31);
    for (col = 0; col < 9; col++, x = (uint16_t)(x + 0x10)) {
        uint16_t y = col & 1 ? 0x0C : 0, at = (uint16_t)(at0 + col);

        for (row = 0; row < 7; row++, y = (uint16_t)(y + 0x18), at = (uint16_t)(at + 0x40))
            field_mark(x, y, NB(field_bits, at) & mine, view);
    }
}

/* S0E56:145A (field_draw): the field at `field` (a map offset) drawn
 * again in the side's half when its view is in state 0, 1, 7 or 8 (byte
 * 17h) and the field lies in its 9 by 7 fields from the view's word 2:
 * field_put, then its mark by the side's bits of field_bits (55h or
 * AAh), the table chosen by byte 1Bh of the first side's view (the
 * original passes S331F:4D92 for both sides). */
void field_draw(uint16_t side, uint16_t field)
{
    Far view, view0;
    int16_t w = (int16_t)NW(map_width, 0), f = (int16_t)field >> 1, v;
    int16_t col = f % w, row = f / w, vcol, vrow;
    uint16_t x, y, bit;
    uint8_t st, mine = side ? 0xAA : 0x55, b;

    view.seg = view0.seg = S_views;
    view.off = (uint16_t)(A_views + side * 0x31);
    view0.off = A_views;
    v = (int16_t)pw(view, 2) >> 1;
    vcol = v % w;
    vrow = v / w;
    st = pb(view, 0x17);
    if (st != 0 && st != 1 && st != 8 && st != 7)
        return;
    if (col < vcol || vcol + 9 <= col || row < vrow || vrow + 7 <= row)
        return;
    x = (uint16_t)(((col - vcol) << 4) + (side ? 0xA0 : 0));
    y = (uint16_t)(((col - vcol) & 1 ? 0x0C : 0) + (row - vrow) * 0x18);
    field_put(x, y, side_map(side), field, side);
    bit = (uint16_t)(col + (row << 6));
    b = NB(field_bits, bit);
    if (b & mine)
        field_mark(x, y, (uint16_t)(b & mine), view0);
}

/* S28FA:0000: a line of the page drawn in the colour DATA:00DA from x
 * DATA:00DC to DATA:00E0 (made the smaller first, kept so) at y DATA:00DE;
 * the masks of a group's first and last pixels and the pixels left in the
 * first group from its own segment (bytes at 00A6 and 00AA, words at
 * 009E) */
static void line_fill(void)
{
    uint16_t cs = HL_S28FA + HL_BASE, es = NW(screen_seg, 0), y = DW(0x00DE), x0 = DW(0x00DC), di;
    uint8_t c = (uint8_t)DW(0x00DA), m;
    int16_t n;

    if ((int16_t)x0 > (int16_t)DW(0x00E0)) {
        fww(S_DATA, 0x00DC, DW(0x00E0));
        fww(S_DATA, 0x00E0, x0);
        x0 = DW(0x00DC);
    }
    di = screen_at(x0, y);
    n = (int16_t)(DW(0x00E0) - x0 + 1);
    m = frb(cs, (uint16_t)(0xA6 + (x0 & 3)));
    n = (int16_t)(n - (int16_t)frw(cs, (uint16_t)(0x9E + 2 * (x0 & 3))));
    if (n <= 0) {
        vga_outw(0x3C4, (uint16_t)((m & frb(cs, (uint16_t)(0xAA + (DW(0x00E0) & 3)))) << 8 | 2));
        vwb(es, di, c);
        return;
    }
    vga_outw(0x3C4, (uint16_t)(m << 8 | 2));
    vwb(es, di++, c);
    vga_outw(0x3C4, 0x0F02);
    if (n >> 2) {
        int16_t k;
        for (k = 0; k < n >> 2; k++)
            vwb(es, di++, c);
        if (!(n & 3))
            return;
    }
    vga_outw(0x3C4, (uint16_t)(frb(cs, (uint16_t)(0xAA + (DW(0x00E0) & 3))) << 8 | 2));
    vwb(es, di, c);
}

/* S28E5:0004: the lines from y DATA:00DE to DATA:00E2 (made the smaller
 * first), DATA:00DE counted on past the last */
void rect_fill(void)
{
    uint16_t n = (uint16_t)(DW(0x00E2) - DW(0x00DE));

    if (DW(0x00E2) < DW(0x00DE)) {
        uint16_t t = DW(0x00DE);
        n = (uint16_t)-n;
        fww(S_DATA, 0x00DE, DW(0x00E2));
        fww(S_DATA, 0x00E2, t);
    }
    for (n++; n; n--) {
        line_fill();
        fww(S_DATA, 0x00DE, (uint16_t)(DW(0x00DE) + 1));
    }
}

/* S2A76:0139: a picture at x, y, `bytes` (two pixels each) a row of a
 * plane, `h` rows, cut to the rectangle DATA:00EC-00F2 (left, top,
 * right, bottom); the rows cut off above and below kept in its own
 * segment (F8h and FAh, in source bytes); 0 when nothing is left, with
 * x, y and h as cut */
static int pic_clip(uint16_t *x, uint16_t *y, uint16_t bytes, uint16_t *h)
{
    uint16_t cs = HL_S2A76 + HL_BASE;
    int16_t ax = (int16_t)*x, bx = (int16_t)*y, cx = (int16_t)bytes, dx = (int16_t)*h, di;

    if (ax < (int16_t)DW(0x00EC)) {
        cx = (int16_t)(cx - ((int16_t)DW(0x00EC) - ax));
        if (cx <= 0)
            return 0;
        ax = (int16_t)DW(0x00EC);
    } else if (ax > (int16_t)DW(0x00F0))
        return 0;
    if (bx < (int16_t)DW(0x00EE)) {
        dx = (int16_t)(dx - ((int16_t)DW(0x00EE) - bx));
        if (dx <= 0)
            return 0;
        bx = (int16_t)DW(0x00EE);
    } else if (bx > (int16_t)DW(0x00F2))
        return 0;
    di = (int16_t)(dx + bx);
    if (di > (int16_t)DW(0x00F2)) {
        dx = (int16_t)(dx - (di - (int16_t)DW(0x00F2)));
        if (!dx)
            return 0;
    }
    di = (int16_t)(cx + ax);
    if (di > (int16_t)DW(0x00F0)) {
        cx = (int16_t)(cx - (di - (int16_t)DW(0x00F0)));
        if (!cx)
            return 0;
    }
    fww(cs, 0x01FA, (uint16_t)((*h - dx) * frw(cs, 0x01F0)));
    fww(cs, 0x01F8, (uint16_t)((bx - *y) * frw(cs, 0x01F0)));
    *x = (uint16_t)ax;
    *y = (uint16_t)bx;
    *h = (uint16_t)dx;
    return 1;
}

/* S2A76:0002, reached through S2A6B:0008 for a picture of kind 'P'
 * (byte 9) and y not negative: pixels of 4 bits from byte 12h on, plane
 * by plane (each a plane's rows of (width + 7) / 8 bytes, the high half
 * first, the two pixels four apart on the screen), each made `base` +
 * its 4 bits, byte 8 the transparent one; placed at x, y moved by the
 * words at 0Ah and 0Ch, width at 0Eh, height at 10h.  Cut to DATA:00EC
 * when DATA:00E8 is not 0.  Its variables in its own segment (E6h-FCh)
 * as the original leaves them.  The colour table of S2AE4:000C (the two
 * words 0 in all calls translated) is not. */
static void pic_blit(uint16_t x, uint16_t y, Far src, uint8_t base)
{
    uint16_t cs = HL_S2A76 + HL_BASE, es = NW(screen_seg, 0), bytes, w, h, di, si;
    uint8_t mask, skip = (uint8_t)(pb(src, 8) + base);
    int plane, r, k;

    fww(cs, 0x01FA, 0);
    fww(cs, 0x01F8, 0);
    x = (uint16_t)(x + pw(src, 0x0A));
    y = (uint16_t)(y + pw(src, 0x0C));
    bytes = (uint16_t)(((pw(src, 0x0E) + 7) & 0xFFFC) >> 3);
    fww(cs, 0x01F0, bytes);
    fww(cs, 0x01F2, bytes);
    fww(cs, 0x01F6, (uint16_t)(NW(screen_pitch, 0) - 2 * bytes));
    w = (uint16_t)(2 * bytes);
    h = pw(src, 0x10);
    if (DW(0x00E8) && !pic_clip(&x, &y, w, &h)) {
        vga_outw(0x3C4, 0x0F02);
        return;
    }
    fww(cs, 0x01EA, w);
    fww(cs, 0x01E8, h);
    /* S2955:0002: the place, and the plane mask 11h rotated by x & 3 */
    di = screen_at(x, y);
    mask = (uint8_t)(0x11 << (x & 3));
    fwb(cs, 0x01FC, mask);
    si = (uint16_t)(src.off + 0x12 + frw(cs, 0x01F8));
    for (plane = 0; plane < 4; plane++) {
        uint16_t d = di;
        vga_outw(0x3C4, (uint16_t)(frb(cs, 0x01FC) << 8 | 2));
        for (r = h; r; r--) {
            fww(cs, 0x01E6, (uint16_t)r);
            for (k = 0; k < (int)bytes; k++, d = (uint16_t)(d + 2)) {
                uint8_t c = frb(src.seg, si++), hi = (uint8_t)((c >> 4) + base), lo = (uint8_t)((c & 0x0F) + base);
                if (hi != skip)
                    vwb(es, d, hi);
                if (lo != skip)
                    vwb(es, (uint16_t)(d + 1), lo);
            }
            d = (uint16_t)(d + frw(cs, 0x01F6));
            si = (uint16_t)(si + frw(cs, 0x01F4));
        }
        fww(cs, 0x01E6, 0);
        mask = frb(cs, 0x01FC);
        fwb(cs, 0x01FC, (uint8_t)(mask << 1 | mask >> 7));
        di = (uint16_t)(di + (mask >> 7));
        si = (uint16_t)(si + frw(cs, 0x01FA));
    }
    vga_outw(0x3C4, 0x0F02);
}

/* S2B45:0006: the text `text` (host bytes, ended by 0) from x, y on with
 * the font `font` and the table `map` (a word a character: the place of
 * its picture in the font, FFFFh for a gap half the last picture's width;
 * 0:0 for either takes DATA:036A / 036E), each picture drawn in colours
 * `base` on and followed by `gap` pixels; '|' and 0Dh begin a line
 * `line` lower at the first x.  Only pictures of kind 'P' at y not
 * negative are drawn (the other ways of S2A6B:0008 are not translated;
 * the game's font is 'P'). */
static void text_put(uint16_t x, uint16_t y, uint16_t gap, uint16_t line, uint8_t base, const uint8_t *text, Far font, Far map)
{
    uint16_t x0 = x, last = 0;
    uint8_t c;

    if (!font.seg && !font.off)
        font = far_at(S_DATA, 0x036A);
    if (!map.seg && !map.off)
        map = far_at(S_DATA, 0x036E);
    while ((c = *text++) != 0) {
        uint16_t o = pw(map, (uint16_t)(2 * c));
        Far g;

        if (c == '|' || c == 0x0D) {
            y = (uint16_t)(y + line);
            x = x0;
            continue;
        }
        if (o == 0xFFFF) {
            x = (uint16_t)(x + (last >> 1));
            continue;
        }
        g.seg = font.seg;
        g.off = (uint16_t)(font.off + o);
        if (pb(g, 9) == 'P' && (int16_t)y >= 0)
            pic_blit(x, y, g, base);
        last = pw(g, 0x0E);
        x = (uint16_t)(x + last + gap);
    }
}

/* text_put for a text in the program's memory; its length (CODE:2BD8:
 * 0 for 0:0) in *len when len is not NULL */
void text_draw(uint16_t x, uint16_t y, uint16_t gap, uint16_t line, uint8_t base, Far text, uint16_t *len)
{
    uint8_t buf[256];
    uint16_t n = 0;
    Far none = {0, 0};

    if (text.seg || text.off)
        while (n < sizeof buf - 1 && (buf[n] = pb(text, n)) != 0)
            n++;
    buf[n] = 0;
    if (len)
        *len = n;
    else
        text_put(x, y, gap, line, base, buf, none, none);
}

/* S10AF:000F (show_message): the side's status line (x 5 to 94h, y BCh
 * to C4h, the second side's A0h to the right) filled with colour D7h
 * through the record DATA:0092 points to (colour, x0, y0, x1, y1); for a
 * text other than FFFFh then the text of that number (the table of word
 * offsets S331F:17BA points to) centred at 3Eh less 3/2 of its length,
 * y BDh, in colours 73h (75h for the second side), and game_state's word
 * 0 gets 4 (8 for the second side) until timer_fire clears it. */
void show_message(uint16_t text, uint16_t side)
{
    Far r = far_at(S_DATA, 0x0092), t;
    uint16_t x = side ? 0xA0 : 0, n = 0;

    fww(r.seg, (uint16_t)(r.off + 2), (uint16_t)(x + 5));
    fww(r.seg, (uint16_t)(r.off + 4), 0xBC);
    fww(r.seg, (uint16_t)(r.off + 6), (uint16_t)(x + 0x94));
    fww(r.seg, (uint16_t)(r.off + 8), 0xC4);
    fww(r.seg, r.off, 0xD7);
    rect_fill();
    if (text == 0xFFFF)
        return;
    t = far_at(S_GAME, 0x17BA);
    t.off = (uint16_t)(t.off + frw(t.seg, (uint16_t)(t.off + 2 * text)));
    text_draw(0, 0, 0, 0, 0, t, &n);
    text_draw((uint16_t)(x + 0x3E - ((int16_t)(n * 3) >> 1)), 0xBD, 0, 6, side ? 0x75 : 0x73, t, NULL);
    SETW(game_state, 0, NW(game_state, 0) | (side ? 8 : 4));
}

/* S2AAF:0000, reached through S2A6B:0008 for a picture of kind 'U': 8-bit
 * pixels from byte 12h on, plane by plane (the plane of x first), each
 * plane's rows as wide as S2949:0002 makes them (width / 4, one more for
 * the first width & 3 planes), byte 8 the transparent one, `base` added;
 * placed as pic_blit places its pictures.  Its variables in its own
 * segment (332h-35Ah) as the original leaves them.  The cut way
 * (DATA:00E8 not 0, S2AAF:0107 and 0279) and S2AE4:000C's colour table
 * are not translated: with DATA:00E8 set nothing is drawn. */
static void pic_blit_u(uint16_t x, uint16_t y, Far src, uint8_t base)
{
    uint16_t cs = HL_S2AAF + HL_BASE, es = NW(screen_seg, 0), w = pw(src, 0x0E), h, di, si, q, i;
    uint8_t clear = pb(src, 8), mask;
    int plane;

    fww(cs, 0x0332, NW(screen_pitch, 0));
    fww(cs, 0x035A, DW(0x00E8));
    fww(cs, 0x033E, 0x0340);
    for (q = w >> 2, i = 0; i < 4; i++)
        fww(cs, (uint16_t)(0x0340 + 2 * i), (uint16_t)(q + (i < (w & 3))));
    fwb(cs, 0x0358, clear);
    if (DW(0x00E8))
        return;
    x = (uint16_t)(x + pw(src, 0x0A));
    y = (uint16_t)(y + pw(src, 0x0C));
    h = pw(src, 0x10);
    fww(cs, 0x0338, w);
    fww(cs, 0x0336, h);
    di = screen_at(x, y);
    mask = (uint8_t)(0x11 << (x & 3));
    si = (uint16_t)(src.off + 0x12);
    for (plane = 0; plane < 4; plane++) {
        uint16_t n = frw(cs, (uint16_t)(0x0340 + 2 * plane)), r, k, d = di;

        vga_outw(0x3C4, (uint16_t)(mask << 8 | 2));
        if (n)
            for (r = 0; r < h; r++, d = (uint16_t)(d + frw(cs, 0x0332)))
                for (k = 0; k < n; k++) {
                    uint8_t c = frb(src.seg, si++);
                    if (c != clear)
                        vwb(es, (uint16_t)(d + k), (uint8_t)(c + base));
                }
        di = (uint16_t)(di + (mask >> 7));
        mask = (uint8_t)(mask << 1 | mask >> 7);
    }
    fwb(cs, 0x0359, mask);
    vga_outw(0x3C4, 0x0F02);
}

/* the rectangle x0, y0 to x1, y1 in colour c through the record DATA:0092
 * points to (rect_fill) */
static void box_fill(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t c)
{
    Far r = far_at(S_DATA, 0x0092);

    fww(r.seg, (uint16_t)(r.off + 2), x0);
    fww(r.seg, (uint16_t)(r.off + 4), y0);
    fww(r.seg, (uint16_t)(r.off + 6), x1);
    fww(r.seg, (uint16_t)(r.off + 8), y1);
    fww(r.seg, r.off, c);
    rect_fill();
}

/* S2532:1A8F and S2532:1BC3: a flag 12 by 6 pixels at x, y of three
 * stripes in colours c1, c2, c3: lying (two rows each) or standing (four
 * columns each) */
void flag_draw(uint16_t x, uint16_t y, uint16_t c1, uint16_t c2, uint16_t c3, int standing)
{
    if (standing) {
        box_fill(x, y, (uint16_t)(x + 3), (uint16_t)(y + 5), c1);
        box_fill((uint16_t)(x + 4), y, (uint16_t)(x + 7), (uint16_t)(y + 5), c2);
        box_fill((uint16_t)(x + 8), y, (uint16_t)(x + 0x0B), (uint16_t)(y + 5), c3);
    } else {
        box_fill(x, y, (uint16_t)(x + 0x0B), (uint16_t)(y + 1), c1);
        box_fill(x, (uint16_t)(y + 2), (uint16_t)(x + 0x0B), (uint16_t)(y + 3), c2);
        box_fill(x, (uint16_t)(y + 4), (uint16_t)(x + 0x0B), (uint16_t)(y + 5), c3);
    }
}

/* S1571:0134: `value` in decimal at x, y in the colour the record of
 * DATA:0092 holds (its word 0, the last one filled) */
static void num_draw(int16_t value, uint16_t x, uint16_t y)
{
    uint8_t rev[12], txt[12];
    int n = 0, i;
    Far none = {0, 0};

    do {
        rev[n++] = (uint8_t)('0' + value % 10);
        value = (int16_t)(value / 10);
    } while (value);
    for (i = 0; i < n; i++)
        txt[i] = rev[n - 1 - i];
    txt[n] = 0;
    text_put(x, y, 0, 6, (uint8_t)pw(far_at(S_DATA, 0x0092), 0), txt, none, none);
}

/* S10AF:0131 (unit_panel): the unit's line in the side's status line
 * (x 5, the second side's A5h): the line cleared (show_message(-1)),
 * the flag of the unit's side at x + 7Ah (standing in 3Eh, 1, 11h for a
 * unit with bit 0 of word 4, else lying in 0, 1, 11h), its count at
 * x + 6 (byte 3 when its kind has 4 in word 09CCh of its record of 45h
 * bytes, else byte 2), its experience (byte 1, at most 6) as the picture
 * of that number less 1 from the table S331F:01F3 points to at x + 0Eh,
 * y BCh (word 18h of the record of DATA:0092 set to C8h meanwhile), and
 * without 2 in its word 4 its kind's name (S331F:09E9 in the kind's
 * record) at x + 28h, y BDh in colours 73h (75h). */
void unit_panel(uint8_t no, uint16_t side)
{
    Far unit, r = far_at(S_DATA, 0x0092);
    uint16_t x = side ? 0xA5 : 5, kind;
    uint8_t exp;

    unit.seg = S_units;
    unit.off = (uint16_t)(A_units + no * UNIT_SIZE);
    kind = (uint16_t)(pb(unit, 8) * 0x45);
    show_message(0xFFFF, side);
    if (pw(unit, 4) & 1)
        flag_draw((uint16_t)(x + 0x7A), 0xBC, 0x3E, 1, 0x11, 1);
    else
        flag_draw((uint16_t)(x + 0x7A), 0xBC, 0, 1, 0x11, 0);
    num_draw(frw(S_GAME, (uint16_t)(0x09CC + kind)) & 4 ? pb(unit, 3) : pb(unit, 2), (uint16_t)(x + 6), 0xBD);
    exp = pb(unit, 1);
    if (exp > 6)
        exp = 6;
    if (exp) {
        uint16_t keep = pw(r, 0x18);
        Far pic = pfar(far_add(far_at(S_GAME, 0x01F3), 4u * (uint8_t)(exp - 1)), 0);

        fww(r.seg, (uint16_t)(r.off + 0x18), 0xC8);
        if (pb(pic, 9) == 'P')
            pic_blit((uint16_t)(x + 0x0E), 0xBC, pic, 0);
        else if (pb(pic, 9) != 'X')
            pic_blit_u((uint16_t)(x + 0x0E), 0xBC, pic, 0);
        fww(r.seg, (uint16_t)(r.off + 0x18), keep);
    }
    if (!(pw(unit, 4) & 2)) {
        Far name;
        name.seg = S_GAME;
        name.off = (uint16_t)(0x09E9 + kind);
        text_draw((uint16_t)(x + 0x28), 0xBD, 0, 6, side ? 0x75 : 0x73, name, NULL);
    }
}

/* S2A95:000E, reached through S2A6B:0008 for a picture of kind 'P' and y
 * negative (drawn at -y): the picture at half its size, uncut.  Four
 * passes, one a plane from the plane of x on: the rows of every second
 * source row of plane 0's pixels, then of plane 2's, each byte giving
 * one screen byte, the high half in the first two passes and the low
 * half in the last two; `base` added, byte 8 the transparent one.  Its
 * variables in its own segment (184h-19Eh) as the original leaves them.
 * S2AE4:000C's colour table is not translated (0 in all calls). */
static void pic_half(uint16_t x, uint16_t y, Far src, uint8_t base)
{
    uint16_t cs = HL_S2A95 + HL_BASE, es = NW(screen_seg, 0), b, h, di, si0, si, plane_bytes;
    uint8_t mask, skip = (uint8_t)(pb(src, 8) + base);
    int pass;

    fww(cs, 0x0198, 0);
    fww(cs, 0x0196, 0);
    x = (uint16_t)(x + pw(src, 0x0A));
    y = (uint16_t)(y + pw(src, 0x0C));
    fww(cs, 0x0194, NW(screen_pitch, 0));
    b = (uint16_t)((pw(src, 0x0E) + 7) & 0xFFFC);
    fww(cs, 0x0192, (uint16_t)(b >> 1));
    b >>= 3;
    fww(cs, 0x018E, b);
    fww(cs, 0x0190, b);
    h = pw(src, 0x10);
    fww(cs, 0x0188, b);
    fww(cs, 0x0186, h);
    di = screen_at(x, y);
    mask = (uint8_t)(0x11 << (x & 3));
    fwb(cs, 0x019A, mask);
    plane_bytes = (uint16_t)(b * h);
    fww(cs, 0x019B, plane_bytes);
    si0 = (uint16_t)(src.off + 0x12);
    fww(cs, 0x0196, si0);
    fww(cs, 0x019D, 0);
    si = si0;
    for (pass = 4; pass; pass--) {
        uint16_t d = di, r, k;

        vga_outw(0x3C4, (uint16_t)(frb(cs, 0x019A) << 8 | 2));
        for (r = (uint16_t)(h >> 1); r; r--) {
            uint16_t e = d;

            fww(cs, 0x0184, r);
            for (k = 0; k < b; k++, e++) {
                uint8_t c = frb(src.seg, si++);

                c = (uint8_t)((frw(cs, 0x019D) ? c & 0x0F : c >> 4) + base);
                if (c != skip)
                    vwb(es, e, c);
            }
            d = (uint16_t)(d + frw(cs, 0x0194));
            si = (uint16_t)(si + b);
        }
        fww(cs, 0x0184, 0);
        mask = frb(cs, 0x019A);
        fwb(cs, 0x019A, (uint8_t)(mask << 1 | mask >> 7));
        di = (uint16_t)(di + (mask >> 7));
        si = si0;
        if (pass == 4)
            si = (uint16_t)(si + 2 * plane_bytes);
        else if (pass == 3)
            fww(cs, 0x019D, (uint16_t)~frw(cs, 0x019D));
        else if (pass == 2)
            si = (uint16_t)(si + 2 * plane_bytes);
    }
    vga_outw(0x3C4, 0x0F02);
}

/* S2A6B:0008 (pic_draw): a picture by its kind (byte 9): 'P' by
 * pic_blit, or at half size by pic_half for y negative; 'X' (S2957:0002)
 * is not translated and draws nothing here; any other by pic_blit_u */
void pic_draw(uint16_t x, uint16_t y, Far src, uint8_t base)
{
    if (pb(src, 9) == 'P') {
        if ((int16_t)y >= 0)
            pic_blit(x, y, src, base);
        else
            pic_half(x, (uint16_t)-(int16_t)y, src, base);
    } else if (pb(src, 9) != 'X')
        pic_blit_u(x, y, src, base);
}

/* S2A6F:0002: a packed picture unpacked to `work` (tpwm_unpack) and drawn
 * from there; one without the packer's signature drawn as it is */
static void pic_unpack_draw(uint16_t x, uint16_t y, Far src, uint8_t base, Far work)
{
    int32_t n = tpwm_unpack(src, work);

    pic_draw(x, y, (uint32_t)n >> 16 == 0xFFFF ? src : work, base);
}

static Far free_end(void)
{
    Far p;

    p.off = frw(HL_S3303 + HL_BASE, 0x0028);
    p.seg = frw(HL_S3303 + HL_BASE, 0x002A);
    return p;
}

/* S131D:0C50: a unit's large picture in place `slot` of the fight's
 * panel (its table at S331F:020D, 8 bytes a place: x, y, the name's x
 * and y; A0h further right for `right`): the kind's entry in BIGUNIT.LIB
 * (its name the eight bytes of bigunit_dat at the kind's byte 09D7h
 * times 8) read to bigunit_buf, then bigunit_pics (the ground under it)
 * and the unit drawn at y, the unit byte 09D6h of the kind's record (45h
 * bytes) lower (half that, both at half size, in the places other than
 * 1), colours 40h on, by way of the free end; the kind's name below in
 * colours 71h: at the name's x plus 1 in places 0 and 3, else centred on
 * x + 37h.  Returns 2 where the original goes to its error exit. */
static int big_unit(uint16_t slot, uint8_t kind, uint8_t right)
{
    Far name, buf = far_at(S_GAME, 0x02D7), pics = far_at(S_GAME, 0x02DB), ptrs = far_at(S_GAME, 0x02DF), kname;
    Far names, gname, to;
    int r;
    uint16_t k = (uint16_t)(kind * 0x45), t = (uint16_t)(0x020D + slot * 8), x, y, yu, tx, ty, i, n = 0;
    uint8_t list[0x10];

    names = far_add(far_at(S_GAME, 0x02E3), (uint32_t)frb(S_GAME, (uint16_t)(0x09D7 + k)) * 8);
    /* CODE:2C39 (strncpy): eight bytes of the name onto the original's
     * stack, a 0 behind them (the list's end).  The port lends them 10h
     * bytes at the free end for lib_read and gives them back. */
    for (i = 0; i < 8 && (list[i] = pb(names, i)) != 0; i++)
        ;
    for (; i < 0x10; i++)
        list[i] = 0;
    names = free_end();
    for (i = 0; i < 0x10; i++) {
        uint8_t b = pb(names, i);

        pwb(names, i, list[i]);
        list[i] = b;
    }
    gname.seg = S_GAME;
    gname.off = A_bigunit_name;
    name = file_name(1, 0, gname, 3);
    to = buf;
    r = lib_read(&to, name, names, ptrs, far_add(ptrs, 0x10));
    for (i = 0; i < 0x10; i++)
        pwb(names, i, list[i]);
    if (r == -1)
        return 2;
    x = frw(S_GAME, t);
    y = frw(S_GAME, (uint16_t)(t + 2));
    yu = y;
    tx = frw(S_GAME, (uint16_t)(t + 4));
    ty = frw(S_GAME, (uint16_t)(t + 6));
    if (right) {
        x = (uint16_t)(x + 0xA0);
        tx = (uint16_t)(tx + 0xA0);
    }
    if (slot != 1) {
        yu = (uint16_t)(yu + (frb(S_GAME, (uint16_t)(0x09D6 + k)) >> 1));
        yu = (uint16_t)-(int16_t)yu;
        y = (uint16_t)-(int16_t)y;
    } else
        yu = (uint16_t)(yu + frb(S_GAME, (uint16_t)(0x09D6 + k)));
    pic_unpack_draw(x, y, pics, 0x40, free_end());
    pic_unpack_draw(x, yu, buf, 0x40, free_end());
    kname.seg = S_GAME;
    kname.off = (uint16_t)(0x09E9 + k);
    if (slot == 0 || slot == 3)
        tx = (uint16_t)(tx + 1);
    else {
        tx = (uint16_t)(tx + 0x37);
        text_draw(0, 0, 0, 0, 0, kname, &n);
        tx = (uint16_t)(tx - ((int16_t)(n * 3) >> 1));
    }
    text_draw(tx, ty, 0, 6, 0x71, kname, NULL);
    return 0;
}

/* the far pointer to unit picture n (the table S331F:013F points to) */
static Far unit_pic(uint16_t n)
{
    return pfar(far_add(far_at(S_GAME, 0x013F), 4u * n), 0);
}

/* S131D:1F0D: a unit of a kind with 40h in word 4 in the fight's panel
 * at x, y: the kind's picture 2 (8 for a unit without 80h in word 4: 6
 * pictures earlier) of the field's pictures, 6 further on at x + 60h,
 * y + 0Fh, then the picture itself at x + 70h, y + 1Bh, colours base on */
static void panel_whole(Far unit, uint16_t x, uint16_t y, uint8_t base)
{
    uint16_t n = (uint16_t)(pb(unit, 8) * 6 + 2);

    if (!(pw(unit, 4) & 0x80))
        n = (uint16_t)(n - 6);
    pic_draw((uint16_t)(x + 0x60), (uint16_t)(y + 0x0F), unit_pic((uint16_t)(n + 6)), base);
    pic_draw((uint16_t)(x + 0x70), (uint16_t)(y + 0x1B), unit_pic(n), base);
}

/* S131D:178D (fight_panel): the fight of the record `rec` (fight_rec: the
 * two units' far pointers at 0 and 4, their grounds' pictures at 1Eh and
 * 22h) on the panel, the first unit's side's half (x A0h further right
 * when bit 0 of its word 4 is set).  Without pictures it works the
 * losses out itself (fight_losses).  The panel's picture (picture 0 of
 * lib_newshop, unpacked at the free end), both sides' flags, two black
 * boxes, the two units' large pictures (S131D:0C50, places 0 and 3), the
 * two grounds (ground_blit at S331F:02C5/02C6, the second 4Dh lower);
 * with pictures the page shown and fade_in; then six steps of the
 * units' figures (picture byte 8 * 6 + step of S331F:013F's table at the
 * step's place of S331F:02B9, the second 4Dh lower; units of a kind with
 * 40h in word 4 whole at once, S131D:1F0D), in colours 20h or 30h by
 * the side, a step a page shown.  With pictures then fade_out; without,
 * the figures lost drawn again from the last step down in colours B0h
 * and C0h.  Left out here: the sounds of a step (S131D:1FF0) and the
 * waits (wait_frames, wait_input), which the port's clock counts
 * (play.c), and wait_input's clearing of the button and the key.
 * Returns 2 where the original goes to its error exit. */
int fight_panel(Far rec, uint16_t pictures)
{
    Far u0 = pfar(rec, 0), u1 = pfar(rec, 4);
    uint16_t f0 = pw(u0, 4), f1 = pw(u1, 4), gx = frb(S_GAME, 0x02C5), gy = frb(S_GAME, 0x02C6);
    uint8_t right = f0 & 1, x = right ? 0xA0 : 0, n0, n1, l0 = 0, l1 = 0, c0, c1;
    int i;

    fww(S_fight_rec_ptr, A_fight_rec_ptr, rec.off);
    fww(S_fight_rec_ptr, (uint16_t)(A_fight_rec_ptr + 2), rec.seg);
    n0 = pb(u0, 2);
    n1 = pb(u1, 2);
    if (!pictures) {
        fight_losses();
        l0 = pb(u0, 2);
        l1 = pb(u1, 2);
    }
    pic_unpack_draw(x, 0, pfar(far_at(S_GAME, 0x0207), 0), 0xF0, free_end());
    flag_draw((uint16_t)(x + 0x41), right ? 0x9C : 0x4E, 0, 1, 0x11, 0);
    flag_draw((uint16_t)(x + 0x41), right ? 0x4E : 0x9C, 0x3E, 1, 0x11, 1);
    box_fill((uint16_t)(x + 0x15), 0x1B, (uint16_t)(x + 0x45), 0x43, 0xFF);
    box_fill((uint16_t)(x + 0x15), 0x68, (uint16_t)(x + 0x45), 0x90, 0xFF);
    if (big_unit(0, pb(u0, 8), right) || big_unit(3, pb(u1, 8), right))
        return 2;
    ground_blit((uint16_t)(gx + x), gy, pfar(rec, 0x1E));
    ground_blit((uint16_t)(gx + x), (uint16_t)(gy + 0x4D), pfar(rec, 0x22));
    c0 = right ? 0x30 : 0x20;
    c1 = right ? 0x20 : 0x30;
    if (pictures) {
        page_flip();
        page_copy();
        fade_in();
    }
    if (f0 & 0x40)
        panel_whole(u0, x, 0, c0);
    if (f1 & 0x40)
        panel_whole(u1, x, 0x4D, c1);
    for (i = 0; i < 6; i++) {
        uint16_t px = (uint16_t)(frb(S_GAME, (uint16_t)(0x02B9 + 2 * i)) + x), py = frb(S_GAME, (uint16_t)(0x02BA + 2 * i));

        if (i < n0 && !(f0 & 0x40))
            pic_draw(px, py, unit_pic((uint16_t)(pb(u0, 8) * 6 + i)), c0);
        if (i < n1 && !(f1 & 0x40))
            pic_draw(px, (uint16_t)(py + 0x4D), unit_pic((uint16_t)(pb(u1, 8) * 6 + i)), c1);
        page_flip();
        page_copy();
    }
    if (pictures) {
        fade_out();
        return 0;
    }
    c0 = right ? 0xC0 : 0xB0;
    c1 = right ? 0xB0 : 0xC0;
    for (i = 5; i >= 0; i--) {
        uint16_t px = (uint16_t)(frb(S_GAME, (uint16_t)(0x02B9 + 2 * i)) + x), py = frb(S_GAME, (uint16_t)(0x02BA + 2 * i));

        if (f0 & 0x40) {
            if (i == 0 && !l0)
                panel_whole(u0, x, 0, c0);
        } else if (i >= l0 && i < n0)
            pic_draw(px, py, unit_pic((uint16_t)(pb(u0, 8) * 6 + i)), c0);
        if (f1 & 0x40) {
            if (i == 0 && !l1)
                panel_whole(u1, x, 0x4D, c1);
        } else if (i >= l1 && i < n1)
            pic_draw(px, (uint16_t)(py + 0x4D), unit_pic((uint16_t)(pb(u1, 8) * 6 + i)), c1);
        page_flip();
        page_copy();
    }
    return 0;
}
