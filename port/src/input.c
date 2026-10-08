/* input.c - what HL.EXE's interrupts leave for game_loop to read: the
 * keyboard's (S27FE:04F4) and the routine the timer calls 72.8 times a
 * second (S27FE:008C, a period of 3FFFh of the timer's counts), which
 * keeps the last key pressed and on every fourth call fills the two
 * sides' input records from the keys held.  Translated over the program's
 * memory; the joysticks and the mouse are left out (the places are named
 * where they stand).  port/README.md says how this was compared with the
 * original.
 */
#include "hl.h"

#define S_DATA (HL_DATA + HL_BASE)

static uint8_t db(uint16_t o) { return frb(S_DATA, o); }
static uint16_t dw(uint16_t o) { return frw(S_DATA, o); }
static void sb(uint16_t o, uint8_t v) { fwb(S_DATA, o, v); }
static void sw(uint16_t o, uint16_t v) { fww(S_DATA, o, v); }

/* S27FE:04F4, the keyboard's interrupt: the byte read from the keyboard
 * is kept at DATA:0B2B, and the key's bit in the table at DATA:0B1B is
 * cleared for a key pressed and set for one let go (80h in the byte) */
void key_irq(uint8_t code)
{
    uint16_t at = (uint16_t)(0x0B1B + ((code & 0x7F) >> 3));
    uint8_t bit = (uint8_t)(0x80 >> (code & 7));

    sb(0x0B2B, code);
    sb(at, (uint8_t)((db(at) & ~bit) | ((code & 0x80) >> (code & 7))));
}

/* a hold or a gap counted at `c` (S27FE:03B3): the gap, and after ten of
 * them both counts begin again */
static void count_gap(uint16_t c)
{
    sw((uint16_t)(c + 2), (uint16_t)(dw((uint16_t)(c + 2)) + 1));
    if ((int16_t)dw((uint16_t)(c + 2)) > 0x0A) {
        sw(c, 0);
        sw((uint16_t)(c + 2), 0);
    }
}

/* S27FE:0175 and 020E, one side's input record `rec` (DATA:03C8 or 03F4,
 * what DATA:00A6 and 00B6 point to) from the keys held: `cur` are the five
 * bytes found now (left, right, up, down, the button), `prev` those of
 * the call before, `cnt` five pairs of words, how many calls each was
 * held and how many not.  The side's table of keys (seven bytes a key:
 * the scancode and what it gives each of the five) is the one `table`
 * points to.
 * The record: a direction found is put into bytes 0 to 3 (side_input
 * clears them), the button into byte 4; byte 8 becomes FFh whenever the
 * button is up; byte 5 becomes FFh for a second press of the button begun
 * within 19h calls (bytes 6 and 7 keep how far that has come).
 * Returns NULL, or what is not translated: a joystick (a side's word at
 * DATA:0384 or 0386 with the word at 03B8 or 03BA; F5 and F6 switch
 * them).  Left out: the mouse (S27FE:03D4, called for the record the
 * word at DATA:0418 names; at rest in the middle it gives nothing). */
static const char *side_rec(uint16_t cur, uint16_t prev, uint16_t cnt, uint16_t rec, uint16_t table,
                            uint16_t on, uint16_t stick)
{
    uint16_t t, i;

    for (i = 0; i < 5; i++)
        sb((uint16_t)(prev + i), db((uint16_t)(cur + i)));
    for (i = 0; i < 6; i++)
        sb((uint16_t)(cur + i), 0);
    if (dw(on) && dw(stick))
        return "a joystick (S27FE:01AC)";
    for (t = dw(table); db(t); t = (uint16_t)(t + 7)) {
        uint8_t code = db(t);

        if (db((uint16_t)(0x0B1B + ((code & 0x7F) >> 3))) & (0x80 >> (code & 7)))
            continue;
        for (i = 0; i < 5; i++)
            sb((uint16_t)(cur + i), db((uint16_t)(cur + i)) | db((uint16_t)(t + 1 + i)));
    }
    /* a direction begins the opposite one's count of holds again */
    if (db(cur))
        sw((uint16_t)(cnt + 4), 0);
    if (db((uint16_t)(cur + 1)))
        sw(cnt, 0);
    if (db((uint16_t)(cur + 2)))
        sw((uint16_t)(cnt + 0x0C), 0);
    if (db((uint16_t)(cur + 3)))
        sw((uint16_t)(cnt + 8), 0);
    if (!db((uint16_t)(cur + 4)))
        sb((uint16_t)(rec + 8), 0xFF);
    if (dw(cur) | dw((uint16_t)(cur + 2))) {
        sw(rec, dw(cur));
        sw((uint16_t)(rec + 2), dw((uint16_t)(cur + 2)));
    }
    if (db((uint16_t)(cur + 4))) {
        sb((uint16_t)(rec + 4), 0xFF);
        if ((int16_t)dw((uint16_t)(cnt + 0x10)) < 0x19) {
            if (!db((uint16_t)(rec + 7))) {
                sb((uint16_t)(rec + 7), 0xFF);
                sb((uint16_t)(rec + 6), 0);
            }
            if (db((uint16_t)(rec + 6))) {
                sb((uint16_t)(rec + 5), 0xFF);
                sb((uint16_t)(rec + 7), 0);
                sb((uint16_t)(rec + 6), 0);
            }
        } else {
            sb((uint16_t)(rec + 7), 0);
            sb((uint16_t)(rec + 6), 0);
        }
    } else if (dw((uint16_t)(cnt + 0x10)))
        sb((uint16_t)(rec + 6), 0xFF);
    else
        sb((uint16_t)(rec + 7), 0);
    for (i = 0; i < 5; i++) {
        uint16_t c = (uint16_t)(cnt + 4 * i);

        if (db((uint16_t)(cur + i)) && db((uint16_t)(prev + i)))
            sw(c, (uint16_t)(dw(c) + 1));
        else
            count_gap(c);
    }
    return NULL;
}

/* S27FE:008C, called by the timer's interrupt (S27FE:06D2 shares the
 * timer out among four routines; this one has the period 3FFFh): the
 * count game_loop waits by at a pass's end (DATA:0308), the last key
 * pressed into the record behind DATA:00BE (DATA:045C: byte 0 the
 * scancode, word 4 FFFFh until game_loop has read it), and on every fourth
 * call the two sides' records.  Returns NULL, or what is not translated.
 * Left out: the BIOS's keys (INT 16h; the program's own keyboard
 * interrupt passes none on), S2D0D:0306 with the word at DATA:04B9, the
 * joysticks' and the mouse's reading (S2BAD:0006, S2BE7:000A). */
const char *input_tick(void)
{
    const char *why;
    uint8_t code = db(0x0B2B);

    sw(0x0308, (uint16_t)(dw(0x0308) + 1));
    if (code && !(code & 0x80)) {
        sb(0x045D, db(0x045C));
        sb(0x045F, db(0x045E));
        sb(0x045C, code);
        sb(0x045E, 0);
        sw(0x0460, 0xFFFF);
        sb(0x0B2B, 0);
    }
    if (db(0x045C) == db(0x0462)) {
        sw(0x0463, 0xFFFF);
        sw(0x0460, 0);
        sb(0x045C, 0);
    }
    sw(0x0B19, (uint16_t)(dw(0x0B19) + 1));
    if ((int16_t)dw(0x0B19) < 4)
        return NULL;
    why = side_rec(0x03BC, 0x03C2, 0x03D2, 0x03C8, 0x0B2C, 0x03B8, 0x0384);
    if (!why)
        why = side_rec(0x03E8, 0x03EE, 0x03FE, 0x03F4, 0x0B2E, 0x03BA, 0x0386);
    sw(0x0B19, 0);
    return why;
}
