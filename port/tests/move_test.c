/* move_test.c - move_target and move_step against the original.
 *
 *     move_test (target|step) SS:SP AX ENTRY.ram EXIT.ram [...]
 *
 * Each set of five is one call in HL.EXE: which routine, SS:SP (hex) at its
 * entry, AX (hex) at its end, dosrun's -ram dump at the entry and the one
 * at the routine's end (before the registers are taken back from the
 * stack).  move_target's arguments are read from the stack of the first
 * dump and its result compared with AX; move_step has neither (the text
 * the port's routine returns is printed).  Memory after the port's routine
 * is compared with the second dump in the game's state (from segment S3306
 * up to DATA, the two maps and the two sides' work buffers, 2580h bytes of
 * each).  A byte there counts as different when the port's routine wrote it
 * and the original has another value; a byte only the original wrote is
 * counted apart and listed, since move_step's original also draws and
 * shows a text, which the port leaves out.  What differs elsewhere is
 * listed (the stack below the entry's SP is only counted).
 * port/tests/move_test.sh builds this.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hl.h"

#define RAM 0xA0000u
#define MAP_BLOCK 0x1806u
#define BUF_BYTES 0x2580u      /* with the way path_find leaves at 1F40h */

static uint8_t entry[RAM], leave[RAM];

static int load(const char *path, uint8_t *to)
{
    FILE *f = fopen(path, "rb");
    size_t n;
    if (!f) {
        fprintf(stderr, "%s: cannot be opened\n", path);
        return 0;
    }
    n = fread(to, 1, RAM, f);
    fclose(f);
    if (n != RAM) {
        fprintf(stderr, "%s: not a dump of %u bytes\n", path, RAM);
        return 0;
    }
    return 1;
}

int main(int argc, char **argv)
{
    int i, bad = 0, calls = 0;

    if (argc < 6 || (argc - 1) % 5) {
        fprintf(stderr, "usage: move_test (target|step) SS:SP AX ENTRY.ram EXIT.ram ...\n");
        return 2;
    }
    for (i = 1; i < argc; i += 5) {
        unsigned ss, sp, ax, got, want, d_stack = 0, d_state = 0, d_apart = 0, d_else = 0, changed = 0;
        uint32_t a, lo_stack, hi_stack, lo_state, hi_state, zone[4];
        uint16_t arg, s;
        Far p;
        int k, step;

        if (sscanf(argv[i + 1], "%x:%x", &ss, &sp) != 2 || sscanf(argv[i + 2], "%x", &ax) != 1
            || !load(argv[i + 3], entry) || !load(argv[i + 4], leave))
            return 2;
        memset(mem, 0, MEM_SIZE);
        memcpy(mem, entry, RAM);
        s = (uint16_t)ss;
        arg = (uint16_t)(sp + 4);       /* behind the far return address */
        step = !strcmp(argv[i], "step");
        if (step) {
            got = move_step();
            printf("%s %s: unit %u, kind %u, text %Xh", argv[i], argv[i + 4],
                   NB(move_rec, 0), NB(move_rec, 0x0C), got);
        } else if (!strcmp(argv[i], "target")) {
            got = move_target(frw(s, arg), frw(s, (uint16_t)(arg + 2)), far_at(s, (uint16_t)(arg + 4)));
            want = ax;
            printf("%s %s: kind %u, result %Xh", argv[i], argv[i + 4], NB(move_rec, 0x0C), got);
            if (got != want)
                printf(" (the original's %Xh)", want);
        } else {
            fprintf(stderr, "%s: target or step\n", argv[i]);
            return 2;
        }
        lo_stack = lin(s, 0);
        hi_stack = lin(s, (uint16_t)(sp + 4));
        lo_state = (uint32_t)(HL_S3306 + HL_BASE) << 4;
        hi_state = (uint32_t)(HL_DATA + HL_BASE) << 4;
        p = far_at(S_map_ptr, A_map_ptr);
        zone[0] = lin(p.seg, p.off);
        p = far_at(S_map2_ptr, A_map2_ptr);
        zone[1] = lin(p.seg, p.off);
        p = far_at(S_sides, A_sides + 0x0D);
        zone[2] = lin(p.seg, p.off);
        p = far_at(S_sides, A_sides + 0x17 + 0x0D);
        zone[3] = lin(p.seg, p.off);
        for (a = 0; a < RAM; a++) {
            int state = a >= lo_state && a < hi_state;
            changed += mem[a] != entry[a];
            if (mem[a] == leave[a])
                continue;
            for (k = 0; k < 4; k++)
                if (a >= zone[k] && a < zone[k] + (k < 2 ? MAP_BLOCK : BUF_BYTES))
                    state = 1;
            if (state && mem[a] != entry[a]) {
                if (d_state++ < 8)
                    printf(" %05X", (unsigned)a);
            } else if (state) {
                if (d_apart++ < 12)
                    printf(" [%05X]", (unsigned)a);
            } else if (a >= lo_stack && a < hi_stack)
                d_stack++;
            else if (d_else++ < 8)
                printf(" (%05X)", (unsigned)a);
        }
        printf("; state %u, only the original's %u, elsewhere %u, stack %u; bytes the port's routine changed %u\n",
               d_state, d_apart, d_else, d_stack, changed);
        if (d_state || (!step && got != want))
            bad++;
        calls++;
    }
    printf("%d calls, %d different\n", calls, bad);
    return bad != 0;
}
