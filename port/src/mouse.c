/* mouse.c - HL.EXE's routines for the mouse (S2BCD to S2BFD), which keep
 * the record behind DATA:00C2 (DATA:0414) through the mouse's driver
 * (interrupt 33h), translated over the program's memory.  The menu reads
 * that record and moves the pointer itself, so its state is part of what
 * the menu leaves.
 *
 * The driver is not the game's: here it is a pointer that only the
 * program moves (no mouse of the player's yet), with what a driver has
 * after its reset on a screen of 320 by 200 as the runner's has it
 * (doskit/tools/run/mouse.c: the middle of 640 by 200, three buttons,
 * speeds 50, 50 and 64), since that is what the runs of the original
 * this is compared with had.
 *
 * Left out: the pointer's picture (S2904:000E puts back what was under
 * it, S2967:0004 draws it, both on the page shown).  The record's words
 * that say whether it is drawn and where (0428, 042A, 041C) are kept as
 * the original keeps them.  port/README.md says how this was compared.
 */
#include "hl.h"

#define S_DATA (HL_DATA + HL_BASE)

static uint16_t dw(uint16_t o) { return frw(S_DATA, o); }
static void sw(uint16_t o, uint16_t v) { fww(S_DATA, o, v); }

/* the driver */
static struct {
    uint16_t x, y, min_x, max_x, min_y, max_y;
    uint16_t speed_x, speed_y, threshold, buttons;
} drv;

static uint16_t clamp(uint16_t v, uint16_t lo, uint16_t hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

static void drv_reset(void)
{
    drv.min_x = drv.min_y = 0;
    drv.max_x = 639;
    drv.max_y = 199;
    drv.x = 319;
    drv.y = 99;
    drv.speed_x = drv.speed_y = 50;
    drv.threshold = 64;
    drv.buttons = 0;
}

/* S2BE7:000A (mouse_read): the pointer's place to words 10h and 12h of
 * the record and its three buttons to words 30h, 34h and 32h (FFFFh
 * held).  With word 6 (the pointer is drawn) and a place other than the
 * one it was drawn at last (words 14h, 16h): the old picture taken off
 * when word 8 says there is one, the new drawn when word 44h is set
 * (mouse_show), word 8 FFFFh then, and the place kept.  Returns 0, FFFFh
 * without a mouse. */
uint16_t mouse_read(void)
{
    if (!dw(0x0414))
        return 0xFFFF;
    sw(0x0424, drv.x);
    sw(0x0426, drv.y);
    sw(0x0444, drv.buttons & 1 ? 0xFFFF : 0);
    sw(0x0448, drv.buttons & 2 ? 0xFFFF : 0);
    sw(0x0446, drv.buttons & 4 ? 0xFFFF : 0);
    if (!dw(0x041A) || (dw(0x0428) == drv.x && dw(0x042A) == drv.y))
        return 0;
    sw(0x041C, 0);
    if (dw(0x0458))
        sw(0x041C, 0xFFFF);
    sw(0x0428, dw(0x0424));
    sw(0x042A, dw(0x0426));
    return 0;
}

/* S2BCD:0008 (mouse_open): the driver's reset; with a mouse the record's
 * word 0 FFFFh, its number of buttons to word 2Eh, `mode` to word 2 (1:
 * the timer's routine reads the mouse on every call), `rec` to word 4
 * (the offset of the side's input record the mouse plays, 0 none), `draw`
 * to word 6, the driver's speeds to words 3Ch to 40h, then mouse_read.
 * Returns 1 when it is open already, FFFFh without a mouse. */
uint16_t mouse_open(uint16_t mode, uint16_t rec, uint16_t draw)
{
    if (dw(0x0414))
        return 1;
    drv_reset();
    sw(0x0442, 3);
    sw(0x0414, 0xFFFF);
    sw(0x0428, 0xFFFF);
    sw(0x042A, 0xFFFF);
    sw(0x0416, mode);
    sw(0x0418, rec);
    sw(0x041A, draw);
    sw(0x0450, drv.speed_x);
    sw(0x0452, drv.speed_y);
    sw(0x0454, drv.threshold);
    sw(0x0458, 0xFFFF);
    return mouse_read();
}

/* S2BD4:000A (mouse_close): the pointer's picture taken off, the driver's
 * speeds set back, words 0 and 8 of the record 0 */
uint16_t mouse_close(void)
{
    if (!dw(0x0414))
        return 0xFFFF;
    drv.speed_x = dw(0x0450);
    drv.speed_y = dw(0x0452);
    drv.threshold = dw(0x0454);
    sw(0x0414, 0);
    sw(0x041C, 0);
    return 0;
}

/* S2BDB:0006 (mouse_range): where the pointer can go */
uint16_t mouse_range(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    if (!dw(0x0414))
        return 0xFFFF;
    sw(0x043A, x0);
    sw(0x043C, x1);
    drv.min_x = x0 < x1 ? x0 : x1;
    drv.max_x = x0 < x1 ? x1 : x0;
    drv.x = clamp(drv.x, drv.min_x, drv.max_x);
    sw(0x043E, y0);
    sw(0x0440, y1);
    drv.min_y = y0 < y1 ? y0 : y1;
    drv.max_y = y0 < y1 ? y1 : y0;
    drv.y = clamp(drv.y, drv.min_y, drv.max_y);
    return 0;
}

/* S2BDF:000E (mouse_speed): the driver's steps a pixel (a, b) and its
 * threshold (e), and four more words of the record (what reads them was
 * not looked at; S27FE:03D4 presumably) */
uint16_t mouse_speed(uint16_t a, uint16_t b, uint16_t c, uint16_t d, uint16_t e, uint16_t f, uint16_t g)
{
    if (!dw(0x0414))
        return 0xFFFF;
    sw(0x042C, a);
    sw(0x042E, b);
    sw(0x0438, e);
    sw(0x0430, c);
    sw(0x0432, d);
    sw(0x0434, f);
    sw(0x0420, f);
    sw(0x0436, g);
    return 0;
}

/* S2BE5:000C (mouse_to): the pointer put somewhere */
void mouse_to(uint16_t x, uint16_t y)
{
    drv.x = clamp(x, drv.min_x, drv.max_x);
    drv.y = clamp(y, drv.min_y, drv.max_y);
}

/* S2BF9:0008 and S2BFD:000A (mouse_hide, mouse_show): the place drawn at
 * forgotten, word 44h 0 or FFFFh, mouse_read; mouse_hide then says no
 * picture is on the screen */
uint16_t mouse_hide(void)
{
    if (!dw(0x0414))
        return 0xFFFF;
    sw(0x0428, 0xFFFF);
    sw(0x042A, 0xFFFF);
    sw(0x0458, 0);
    mouse_read();
    sw(0x041C, 0);
    return 0;
}

uint16_t mouse_show(void)
{
    if (!dw(0x0414))
        return 0xFFFF;
    sw(0x0428, 0xFFFF);
    sw(0x042A, 0xFFFF);
    sw(0x0458, 0xFFFF);
    mouse_read();
    return 0;
}
