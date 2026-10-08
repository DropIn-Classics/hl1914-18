/* screen.c - the screen's set-up: HL.EXE's screen_open (S298E:000D) with
 * pages_set (S2A10:0006), screen_clear (S28D9:000C) and page_flip
 * (S2974:0002), on the runtime's VGA (doskit/runtime/vga.h) and the
 * program's memory.  port/README.md says how this was compared.
 */
#include "hl.h"
#include "vga.h"

#define S_DATA (HL_DATA + HL_BASE)
#define DW(o) frw(S_DATA, (o))
#define SET_DW(o, v) fww(S_DATA, (o), (uint16_t)(v))

/* INT 10h AH=0: the mode set, and the BIOS's byte of the mode (40:49) */
static void bios_mode(uint8_t mode)
{
    vga_set_mode(mode);
    mem[0x449] = mode;
}

/* S2945:000A (retrace_wait): until the next vertical retrace begins */
static void retrace_wait(void)
{
    uint16_t port = DW(0x0AFD);

    if (vga_inb(port) & 8)
        while (vga_inb(port) & 8)
            ;
    while (!(vga_inb(port) & 8))
        ;
}

/* S298E:02E4: all four planes of A000:0 cleared, 7FFFh words */
static void vram_clear(void)
{
    uint16_t o;

    vga_outw(0x3C4, 0x0F02);
    for (o = 0; o < 0xFFFE; o++)
        vga_write(o, 0);
}

/* S298E:030C: the DAC's 256 colours made black */
static void dac_black(void)
{
    int i;

    vga_outb(0x3C8, 0);
    for (i = 0; i < 0x300; i++)
        vga_outb(0x3C9, 0);
}

/* S298E:02A0: the offsets of the lines, F0h words at DATA:00F8 */
static void line_table(void)
{
    uint16_t i, pitch = NW(screen_pitch, 0);

    for (i = 0; i < 0xF0; i++)
        SET_DW(0x00F8 + 2 * i, i * pitch);
}

/* S298E:007C: the mode by the height (the width is compared with DATA's
 * segment, left in AX: never 140h): 200 lines mode 13h made unchained
 * (S298E:01F9); else a mode of 360 by 240 or 480 lines from the CRTC
 * tables at S298E:02B4 / 02CC (not met; not translated) */
static int mode_set(uint16_t h)
{
    uint8_t v;

    if (h != 0xC8)
        return 0;
    retrace_wait();
    bios_mode(0x13);
    dac_black();
    vram_clear();
    vga_outb(0x3C4, 4);
    v = vga_inb(0x3C5);
    vga_outb(0x3C5, v & 0xF7);
    vga_outb(0x3D4, 0x11);
    v = vga_inb(0x3D5);
    vga_outb(0x3D5, v & 0x7F);
    vga_outw(0x3D4, 0xC317);
    vga_outw(0x3D4, 0x0014);
    SET_DW(0x02E4, 0x140);
    SET_DW(0x02E6, 0xC8);
    SET_DW(0x02E8, 8);
    SET_DW(0x02EA, 7);
    SET_DW(0x02EE, 0x28);
    SETW(screen_pitch, 0, 0x50);
    SET_DW(0x0360, 1);
    SET_DW(0x02F2, 0x3E80);
    SET_DW(0x02F4, 0);
    SET_DW(0x02F6, 0xFA00);
    SET_DW(0x02F8, 0);
    line_table();
    return 1;
}

/* S2A10:0006 (pages_set): the segments of the screen's pages: drawn
 * (screen_seg) and shown (DATA:0356), and further ones at DATA:0322 to
 * 0330, 035C to 0368 (what they are was not looked at) */
void pages_set(void)
{
    uint16_t bx, cx, dx = 0xAC58, ax;

    if ((int16_t)DW(0x02E6) >= 0xF0) {
        bx = 0xA547;
        cx = 0xAA8E;
    } else {
        bx = 0xA400;
        cx = 0xA7E8;
    }
    if (!DW(0x034E)) {
        ax = 4;
        SET_DW(0x0326, ax);
        SET_DW(0x032E, ax);
        SET_DW(0x0322, ax);
        SET_DW(0x032A, ax);
        ax = cx;
        SET_DW(0x0328, ax);
        SET_DW(0x0330, ax);
        SET_DW(0x0324, ax);
        SET_DW(0x032C, ax);
        SETW(screen_seg, 0, 0xA000);
        SET_DW(0x0356, 0xA000);
    } else {
        SET_DW(0x0328, cx);
        SET_DW(0x0324, cx);
        SET_DW(0x0330, cx);
        SET_DW(0x032C, cx);
        ax = 4;
        SET_DW(0x0326, ax);
        SET_DW(0x0322, ax);
        if (!DW(0x00F6))
            ax = DW(0x0342);
        SET_DW(0x032E, ax);
        SET_DW(0x032A, ax);
        SETW(screen_seg, 0, bx);
        SET_DW(0x0356, 0xA000);
    }
    SET_DW(0x035C, 0);
    SET_DW(0x035E, dx);
    SET_DW(0x0366, 0);
    SET_DW(0x0368, dx - 0x0A);
}

/* S28D9:000C (screen_clear): the page being drawn, DATA:02F2 bytes of all
 * four planes, made the colour DATA:00DA */
void screen_clear(void)
{
    uint16_t seg = NW(screen_seg, 0), n = DW(0x02F2) >> 1, o;
    uint8_t c = (uint8_t)DW(0x00DA);
    uint32_t a = lin(seg, 0);

    vga_outw(0x3C4, 0x0F02);
    for (o = 0; o < (uint16_t)(2 * n); o++) {
        uint32_t at = a + o;
        if (at >= 0xA0000 && at < 0xB0000)
            vga_write((uint16_t)(at - 0xA0000), c);
        else
            mem[at] = c;
    }
}

static void swap_dw(uint16_t a, uint16_t b)
{
    uint16_t t = DW(a);

    SET_DW(a, DW(b));
    SET_DW(b, t);
}

/* S2974:0002 (page_flip): the page drawn is shown from the next retrace on
 * (the CRTC's start address) and the other one is drawn to.  With
 * DATA:0463 and 0465 both FFFFh it first calls S2B51:002A (it makes the
 * header of a PBM picture of the screen, presumably to save it; not
 * translated, not met).  Without DATA:00F6 the four words at DATA:0322 to
 * 0328 change places with those at 032A to 0330. */
void page_flip(void)
{
    uint16_t crtc = DW(0x0AF9), bx;

    if (!DW(0x00F6)) {
        swap_dw(0x0328, 0x0330);
        swap_dw(0x0326, 0x032E);
        swap_dw(0x0324, 0x032C);
        swap_dw(0x0322, 0x032A);
    }
    bx = DW(0x0356);
    SET_DW(0x0356, NW(screen_seg, 0));
    SETW(screen_seg, 0, bx);
    bx = (uint16_t)(DW(0x0356) << 4);
    while (vga_inb((uint16_t)(crtc + 6)) & 8)
        ;
    vga_outw(crtc, (uint16_t)((bx & 0xFF00) | 0x0C));
    vga_outw(crtc, (uint16_t)((bx << 8) | 0x0D));
    while (!(vga_inb((uint16_t)(crtc + 6)) & 8))
        ;
}

/* S298E:000D (screen_open): the mode for w by h (only h counts), the
 * pages, both cleared.  `size` goes to DATA:0342 (pages_set's further
 * pages use it), `pages` to DATA:034E. */
void screen_open(uint16_t w, uint16_t h, uint32_t size, uint16_t pages)
{
    (void)w;
    /* INT 10h AH=0Fh: the BIOS's mode and columns */
    SET_DW(0x0346, mem[0x44A] << 8 | mem[0x449]);
    vram_clear();
    SET_DW(0x0AF9, frw(0x40, 0x63));
    SET_DW(0x0AFB, DW(0x0AF9) + 1);
    SET_DW(0x0AFD, DW(0x0AF9) + 6);
    mode_set(h);
    SET_DW(0x0342, (uint16_t)size);
    SET_DW(0x0344, (uint16_t)(size >> 16));
    SET_DW(0x034E, pages);
    pages_set();
    SET_DW(0x00DA, 0);
    screen_clear();
    page_flip();
    screen_clear();
}

/* S28DC:000A (page_copy): the page shown copied to the page drawn,
 * page_size bytes in each of the four planes through the latches (write
 * mode 1, all planes on); write mode 0 again after it, the map mask left
 * on all four planes */
void page_copy(void)
{
    uint16_t to = (uint16_t)((NW(screen_seg, 0) - 0xA000u) << 4);
    uint16_t from = (uint16_t)((NW(screen_shown, 0) - 0xA000u) << 4);
    uint16_t n = NW(page_size, 0), i;

    vga_outw(0x3C4, 0x0F02);
    vga_outw(0x3CE, 0x4105);
    for (i = 0; i < n; i++)
        vga_write((uint16_t)(to + i), vga_read((uint16_t)(from + i)));
    vga_outw(0x3CE, 0x4005);
}

/* S2AFE:000C (pal_level): the 256 colours of the far palette pal (300h
 * bytes) times level / 100h, a quarter of that to the DAC; after each
 * colour's first two bytes a wait for the next time the display is off
 * (bit 0 of 3DAh clear, then set; interrupts off in the original) */
void pal_level(uint8_t level, Far pal)
{
    uint16_t si = pal.off;
    int i, k;

    vga_outb(0x3C8, 0);
    for (i = 0; i < 0x100; i++) {
        for (k = 0; k < 3; k++) {
            if (k == 2) {
                while (vga_inb(0x3DA) & 1)
                    ;
                while (!(vga_inb(0x3DA) & 1))
                    ;
            }
            vga_outb(0x3C9, (uint8_t)((frb(pal.seg, si++) * level >> 8) >> 2));
        }
    }
}

/* S28E0:0004 (fade_in): pal_level at the levels 0, 4, ... FCh with the
 * palette at DATA:04E3 */
void fade_in(void)
{
    unsigned l;

    for (l = 0; l < 0x100; l += 4)
        pal_level((uint8_t)l, far_at(S_DATA, 0x04E3));
}

/* S28E0:002B (fade_out): the levels FFh, FBh, ... 3 */
void fade_out(void)
{
    int l;

    for (l = 0xFF; l >= 0; l -= 4)
        pal_level((uint8_t)l, far_at(S_DATA, 0x04E3));
}
