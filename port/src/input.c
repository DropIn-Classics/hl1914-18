/* input.c - what HL.EXE's interrupts leave for game_loop to read: the
 * keyboard's (S27FE:04F4) and the routine the timer calls 72.8 times a
 * second (S27FE:008C, a period of 3FFFh of the timer's counts), which
 * keeps the last key pressed and on every fourth call fills the two
 * sides' input records from the keys held, the joysticks and the mouse.
 * Translated over the program's memory; the joysticks' own routines are
 * in joy.c.  port/README.md says how this was compared with the
 * original.
 */
#include "hl.h"

#define S_DATA (HL_DATA + HL_BASE)

static uint8_t db(uint16_t o) { return frb(S_DATA, o); }
static uint16_t dw(uint16_t o) { return frw(S_DATA, o); }
static void sb(uint16_t o, uint8_t v) { fwb(S_DATA, o, v); }
static void sw(uint16_t o, uint16_t v) { fww(S_DATA, o, v); }

#define S_KEYS (HL_S27FE + HL_BASE)
#define A_INSTALLED 0x0012u     /* FFh while interrupt 9 is the program's */

/* The BIOS's keys while the program's keyboard interrupt is taken out
 * (key_remove; the menu's typing): not the game's but the machine's part,
 * so written here and not read from anything: a key pressed becomes a
 * word, the scancode above its character (letters small, digits, Enter,
 * Escape, the backspace and the space; 0 for any other), which the
 * timer's routine fetches with interrupt 16h.  No shift, no repeat of a
 * held key. */
static uint16_t bios_keys[16];
static int bios_n, bios_e0;

static void bios_key(uint8_t code)
{
    static const char row[] = "\0\0331234567890\0\0\b\0qwertyuiop\0\0\r\0asdfghjkl\0\0\0\0\0zxcvbnm";
    uint8_t ch = 0;

    if (code == 0xE0) {
        bios_e0 = 1;
        return;
    }
    if (!(code & 0x80) && bios_n < 16) {
        if (!bios_e0 && code < sizeof row - 1)
            ch = (uint8_t)row[code];
        if (!bios_e0 && code == 0x39)
            ch = ' ';
        bios_keys[bios_n++] = (uint16_t)(code << 8 | ch);
    }
    bios_e0 = 0;
}

/* S27FE:056C and 05AB (key_install, key_remove): interrupt 9 to the
 * program's routine and back to the one before; the flag at S27FE:0012
 * says which.  Left out: the vector kept at S27FE:000A.  key_remove also
 * clears the byte the program's routine keeps (DATA:0B2B). */
void key_install(void)
{
    fwb(S_KEYS, A_INSTALLED, 0xFF);
}

void key_remove(void)
{
    if (!frb(S_KEYS, A_INSTALLED))
        return;
    fwb(S_KEYS, A_INSTALLED, 0);
    sb(0x0B2B, 0);
}

/* S27FE:04F4, the keyboard's interrupt: the byte read from the keyboard
 * is kept at DATA:0B2B, and the key's bit in the table at DATA:0B1B is
 * cleared for a key pressed and set for one let go (80h in the byte).
 * While it is taken out the byte goes to the BIOS's keys instead. */
void key_irq(uint8_t code)
{
    uint16_t at = (uint16_t)(0x0B1B + ((code & 0x7F) >> 3));
    uint8_t bit = (uint8_t)(0x80 >> (code & 7));

    if (!frb(S_KEYS, A_INSTALLED)) {
        bios_key(code);
        return;
    }
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

/* S27FE:04CF (mouse_center): with neither of the two buttons held the
 * pointer to the middle, A0h and 64h, in the mouse's record and the
 * driver (interrupt 33h function 4) */
static void mouse_center(void)
{
    if (dw(0x0448) || dw(0x0444))
        return;
    sw(0x0424, 0xA0);
    sw(0x0426, 0x64);
    mouse_to(0xA0, 0x64);
}

/* S27FE:03D4 (input_mouse): the mouse as a side's directions and button,
 * into the five bytes at `cur`.  While the first button is held the
 * driver's ratio is the record's words 30h and 32h (DATA:0430) and the
 * step word 22h (0436h), else words 18h and 1Ah and word 20h (0434h); at
 * its letting go the pointer goes to the middle, and with the second
 * button held then ten calls give nothing (the first of them does).  With
 * both buttons held nothing; else a pointer the step or more from the
 * middle (A0h, 64h) gives that direction, across and down, and goes back
 * to the middle; the first button held is the side's button. */
static void input_mouse(uint16_t cur)
{
    int16_t d;

    if (dw(0x0444)) {
        if (!dw(0x041E)) {
            mouse_ratio(dw(0x0430), dw(0x0432));
            sw(0x041E, 0xFFFF);
            sw(0x0420, dw(0x0436));
        }
    } else if (dw(0x041E)) {
        mouse_ratio(dw(0x042C), dw(0x042E));
        sw(0x041E, 0);
        sw(0x0420, dw(0x0434));
        mouse_center();
        if (dw(0x0448))
            sw(0x0456, 0x0A);
        return;
    } else if (dw(0x0456)) {
        sw(0x0456, (uint16_t)(dw(0x0456) - 1));
        if (dw(0x0456) != 9 && dw(0x0456))
            return;
    }
    if (dw(0x0444) & dw(0x0448))
        return;
    d = (int16_t)(dw(0x0424) - 0xA0);
    if (d > 0 && d >= (int16_t)dw(0x0420)) {
        sb((uint16_t)(cur + 1), 0xFF);
        mouse_center();
    } else if (d < 0 && -d >= (int16_t)dw(0x0420)) {
        sb(cur, 0xFF);
        mouse_center();
    }
    d = (int16_t)(dw(0x0426) - 0x64);
    if (d > 0 && d >= (int16_t)dw(0x0420)) {
        sb((uint16_t)(cur + 3), 0xFF);
        mouse_center();
    } else if (d < 0 && -d >= (int16_t)dw(0x0420)) {
        sb((uint16_t)(cur + 2), 0xFF);
        mouse_center();
    }
    if (dw(0x0444))
        sb((uint16_t)(cur + 4), 0xFF);
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
 * With the side's joystick wanted (the word at `on`, DATA:03B8 or 03BA,
 * which F5 and F6 switch) and there (`there`, 0384 or 0386) it comes
 * first (S27FE:01AC, 0232): what joy_read measured last (`stick`: X, Y
 * and the button's word, 0388 or 038E) against the stick's edges
 * (`edge`, 039C or 03AC: X at or below the first is left, at or above
 * the second right, Y with the third and fourth up and down), a
 * direction beginning the opposite one's count of holds again, and the
 * button.  Only when it gives nothing are the keys and the mouse asked.
 * The mouse adds to the record the word at DATA:0418 names
 * (input_mouse). */
static void side_rec(uint16_t cur, uint16_t prev, uint16_t cnt, uint16_t rec, uint16_t table,
                     uint16_t on, uint16_t there, uint16_t stick, uint16_t edge)
{
    uint16_t t, i;
    int keys = 1;

    for (i = 0; i < 5; i++)
        sb((uint16_t)(prev + i), db((uint16_t)(cur + i)));
    for (i = 0; i < 6; i++)
        sb((uint16_t)(cur + i), 0);
    if (dw(on) && dw(there)) {
        int16_t x = (int16_t)dw(stick), y = (int16_t)dw((uint16_t)(stick + 2));

        if (x <= (int16_t)dw(edge)) {
            sb(cur, 0xFF);
            sw((uint16_t)(cnt + 4), 0);
        } else if (x >= (int16_t)dw((uint16_t)(edge + 2))) {
            sb((uint16_t)(cur + 1), 0xFF);
            sw(cnt, 0);
        }
        if (y <= (int16_t)dw((uint16_t)(edge + 4))) {
            sb((uint16_t)(cur + 2), 0xFF);
            sw((uint16_t)(cnt + 0x0C), 0);
        } else if (y >= (int16_t)dw((uint16_t)(edge + 6))) {
            sb((uint16_t)(cur + 3), 0xFF);
            sw((uint16_t)(cnt + 8), 0);
        }
        sb((uint16_t)(cur + 4), db((uint16_t)(stick + 4)));
        keys = !(dw(cur) | dw((uint16_t)(cur + 2)) | db((uint16_t)(cur + 4)));
    }
    for (t = dw(table); keys && db(t); t = (uint16_t)(t + 7)) {
        uint8_t code = db(t);

        if (db((uint16_t)(0x0B1B + ((code & 0x7F) >> 3))) & (0x80 >> (code & 7)))
            continue;
        for (i = 0; i < 5; i++)
            sb((uint16_t)(cur + i), db((uint16_t)(cur + i)) | db((uint16_t)(t + 1 + i)));
    }
    if (keys && dw(0x0414) && dw(0x0418) == rec && dw(0x045A) == 0xFFFF)
        input_mouse(cur);
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
}

/* S27FE:008C, called by the timer's interrupt (S27FE:06D2 shares the
 * timer out among four routines; this one has the period 3FFFh): the
 * count game_loop waits by at a pass's end (DATA:0308), the last key
 * pressed into the record behind DATA:00BE (DATA:045C: byte 0 the
 * scancode, word 4 FFFFh until game_loop has read it), and on every fourth
 * call the two sides' records.  Returns NULL, or what is not translated.
 * A key the BIOS has (INT 16h; only while the program's own keyboard
 * interrupt is taken out, which passes none on) comes first: its scancode
 * to byte 0 and its character to byte 2 of that record.  The mouse is
 * read (mouse_read, S2BE7:000A) on every call when its record's word 2 is
 * 1 (the menu), else on every fourth.
 * read (mouse_read, S2BE7:000A) on every call when its record's word 2 is
 * 1 (the menu), else on every fourth; the joysticks (joy_read,
 * S2BAD:0006) on every fourth while one of the two is wanted.
 * Left out: S2D0D:0306 with the word at DATA:04B9.
 * Not the original's: joy_measure (joy_setup) is called from here, at the first call
 * (for input_install's, S27FE:0013, at the program's start) and at the
 * first after F5 or F6 has switched a stick's word (loop.c's own
 * joy_setup is the one for a machine without a game port; with
 * joy_measure() of joy.c called there instead the second is not needed).
 * The original's waits for retraces in the pass while this routine goes
 * on; joy_measure does so by the calls' times, to within a call (it does
 * not know when in the pass the key was taken, nor where the retrace
 * stood). */
const char *input_tick(void)
{
    static uint16_t wanted[2];
    static int set_up;
    double now;
    uint8_t code = db(0x0B2B);

    now = joy_tick();
    if (!set_up || wanted[0] != dw(0x03B8) || wanted[1] != dw(0x03BA)) {
        joy_measure(set_up, now);
        set_up = 1;
        wanted[0] = dw(0x03B8);
        wanted[1] = dw(0x03BA);
    }

    sw(0x0308, (uint16_t)(dw(0x0308) + 1));
    if (bios_n) {
        uint16_t k = bios_keys[0];
        int i;

        for (i = 1; i < bios_n; i++)
            bios_keys[i - 1] = bios_keys[i];
        bios_n--;
        sb(0x045D, db(0x045C));
        sb(0x045F, db(0x045E));
        sb(0x045C, (uint8_t)(k >> 8));
        sb(0x045E, (uint8_t)k);
        sw(0x0460, 0xFFFF);
    } else if (code && !(code & 0x80)) {
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
    if (dw(0x0414) && dw(0x0416) == 1)
        mouse_read();
    sw(0x0B19, (uint16_t)(dw(0x0B19) + 1));
    if ((int16_t)dw(0x0B19) < 4)
        return NULL;
    if (dw(0x03B8) | dw(0x03BA))
        joy_read();
    if (!dw(0x0416))
        mouse_read();
    side_rec(0x03BC, 0x03C2, 0x03D2, 0x03C8, 0x0B2C, 0x03B8, 0x0384, 0x0388, 0x039C);
    side_rec(0x03E8, 0x03EE, 0x03FE, 0x03F4, 0x0B2E, 0x03BA, 0x0386, 0x038E, 0x03AC);
    sw(0x0B19, 0);
    return NULL;
}
