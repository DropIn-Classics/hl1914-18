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
static uint8_t vrb(uint16_t seg, uint16_t off)
{
    uint32_t a = lin(seg, off);

    return a >= 0xA0000 && a < 0xB0000 ? vga_read((uint16_t)(a - 0xA0000)) : mem[a];
}

static void vwb(uint16_t seg, uint16_t off, uint8_t v)
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

/* S0E56:0D44 (view_draw): the 9 by 7 fields of a side's view from
 * `field` (a byte offset in the map, two bytes a field: ground, unit)
 * on: the second side's half of the screen lies A0h to the right and
 * takes its map from map2_ptr.  Each field's ground by the table of far
 * pointers S331F:0153 points to, then its unit (below F1h) by the one
 * S331F:013F points to, picture byte 0Fh + side of its record plus 6 times byte 8,
 * in colours 30h when bit 0 of its word 4 is set, else 20h; a unit with
 * 2 in word 6 that is not on the side's side (same_side) is not drawn.
 * Odd columns lie 0Ch lower. */
void view_draw(uint16_t field, uint16_t side)
{
    Far map = side ? far_at(S_map2_ptr, A_map2_ptr) : far_at(S_map_ptr, A_map_ptr);
    uint16_t row_bytes = (uint16_t)(NW(map_width, 0) << 1), x = side ? 0xA0 : 0, col, row;

    for (col = 0; col < 9; col++, x = (uint16_t)(x + 0x10)) {
        uint16_t y = col & 1 ? 0x0C : 0, at = (uint16_t)(field + 2 * col);

        for (row = 0; row < 7; row++, y = (uint16_t)(y + 0x18), at = (uint16_t)(at + row_bytes)) {
            uint8_t unit;
            uint16_t rec, pic;

            ground_blit(x, y, pfar(far_add(far_at(S_GAME, 0x0153), 4u * pb(map, at)), 0));
            unit = pb(map, (uint16_t)(at + 1));
            if (unit > 0xF0)
                continue;
            rec = (uint16_t)(A_units + unit * UNIT_SIZE);
            if ((frw(S_units, (uint16_t)(rec + 6)) & 2) && !same_side(frw(S_units, (uint16_t)(rec + 4)), side))
                continue;
            pic = (uint16_t)(frb(S_units, (uint16_t)(rec + 0x0F + side)) + frb(S_units, (uint16_t)(rec + 8)) * 6);
            unit_blit(x, y, frw(S_units, (uint16_t)(rec + 4)) & 1 ? 0x30 : 0x20,
                      pfar(far_add(far_at(S_GAME, 0x013F), 4u * pic), 0));
        }
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
static void rect_fill(void)
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

/* S10AF:000F (show_message) with the text -1: the side's status line
 * (x 5 to 94h, y BCh to C4h, the second side's A0h to the right) filled
 * with colour D7h through the record DATA:0092 points to (colour, x0, y0,
 * x1, y1).  A text's drawing (other numbers) is not translated. */
void message_clear(uint16_t side)
{
    Far r = far_at(S_DATA, 0x0092);
    uint16_t x = side ? 0xA0 : 0;

    fww(r.seg, (uint16_t)(r.off + 2), (uint16_t)(x + 5));
    fww(r.seg, (uint16_t)(r.off + 4), 0xBC);
    fww(r.seg, (uint16_t)(r.off + 6), (uint16_t)(x + 0x94));
    fww(r.seg, (uint16_t)(r.off + 8), 0xC4);
    fww(r.seg, r.off, 0xD7);
    rect_fill();
}
