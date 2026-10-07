/* computer.c - the computer's player of HL.EXE (computer_turn,
 * S1C74:000B, and what it calls), translated over the program's memory.
 * So far only the end of its chain: the script of input codes it plays a
 * side with.  port/README.md says how this was compared with the
 * original.
 */
#include "hl.h"

#define SCRIPT_SIZE 0x20u   /* a record of `computer_script` */

/* computer_input (S1E4B:0001): the side's next word of input, into
 * computer_out, from the code at the script's place (byte 1Eh): a code
 * below 80h is one word of script_bits, a higher one a sequence of
 * script_gestures, one word a call (byte 1Fh the place in it).  Returns 1
 * when the script is at its end after that, and empties it. */
uint16_t computer_input(uint16_t side)
{
    Far s;
    uint8_t c;
    uint16_t done = 0;

    s.seg = S_computer_script;
    s.off = (uint16_t)(A_computer_script + side * SCRIPT_SIZE);
    SETW(computer_out, side * 4, 0);
    c = pb(s, pb(s, 0x1E));
    if (c != 0xFF && c & 0x80) {
        uint8_t g = NB(script_gestures, (c & 0x7F) * 3 + pb(s, 0x1F));

        if (g != 0xFF) {
            SETW(computer_out, side * 4, g);
            pwb(s, 0x1F, (uint8_t)(pb(s, 0x1F) + 1));
        } else {
            pwb(s, 0x1F, 0);
            pwb(s, 0x1E, (uint8_t)(pb(s, 0x1E) + 1));
        }
    } else if (c != 0xFF) {
        SETW(computer_out, side * 4, NB(script_bits, c));
        pwb(s, 0x1E, (uint8_t)(pb(s, 0x1E) + 1));
    }
    if (pb(s, pb(s, 0x1E)) == 0xFF) {
        pwb(s, 0, 0xFF);
        pwb(s, 0x1E, 0);
        pwb(s, 0x1F, 0);
        done = 1;
    }
    return done;
}
