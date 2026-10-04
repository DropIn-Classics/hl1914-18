/* mem.c - memory the program asks DOS for: HL.EXE's mem_alloc
 * (S2CBE:000E), translated over the program's memory with the runtime's
 * DOS blocks (rmem.h).  port/README.md says how this was compared with
 * the original.  The hook that DATA:04B9 switches on is not translated.
 */
#include "hl.h"

#define S_DATA (HL_DATA + HL_BASE)

/* S2CBE:000E: a block for `size` bytes (size / 16 + 1 paragraphs).  Its
 * segment is noted in the table at DATA:0469 (their number at DATA:0467).
 * Returns the block at offset 0, or 0:0 with 1 in DATA:0B17 when DOS has
 * no room. */
Far mem_alloc(uint32_t size)
{
    Far p = { 0, 0 };
    uint16_t n;

    p.seg = dos_alloc((uint16_t)((uint16_t)(size >> 4) + 1));
    if (!p.seg) {
        fww(S_DATA, 0x0B17, 1);
        return p;
    }
    n = frw(S_DATA, 0x0467);
    fww(S_DATA, 0x0467, (uint16_t)(n + 1));
    fww(S_DATA, (uint16_t)((uint16_t)(n << 1) + 0x0469), p.seg);
    return p;
}
