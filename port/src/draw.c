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
