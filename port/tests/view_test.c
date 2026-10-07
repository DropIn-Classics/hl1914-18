/* view_test.c - the routines view_step calls (port/src/move.c and the
 * like) against the original.
 *
 *     view_test ROUTINE SS:SP AX ENTRY.ram EXIT.ram [...]
 *
 * Each set is one call dumped by port/tests/dumps.py at the routine's
 * entry and its RETF: SS:SP at the entry (the arguments are read from the
 * stack there) and the original's result AX (from dumps.log).  ROUTINE is
 * step (view_step, S0CB6:0750, its result in AL only), menu (field_menu, S0CB6:0EAA, nothing returned), aim (move_aim, S10E1:02EB), start (move_start, S10E1:05D1), done (move_done, S10E1:000F, nothing
 * returned), order (attack_order, S0AE0:0008) or unmark (attack_unmark, S0AE0:02E3,
 * which returns nothing: give - as AX).  Memory after the port's routine is
 * compared with the exit dump in the game's state (from segment S3306 up
 * to DATA); differences elsewhere but the stack's segment are counted
 * apart (the maps lie there; so do bytes the interrupts change).
 * port/tests/view_test.sh builds this.
 */
#include <stdio.h>
#include <string.h>
#include "hl.h"

#define RAM 0xA0000u

static uint8_t leave[RAM];

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
        fprintf(stderr, "usage: view_test ROUTINE SS:SP AX ENTRY.ram EXIT.ram ...\n");
        return 2;
    }
    for (i = 1; i < argc; i += 5) {
        unsigned ss, sp, ax = 0, d = 0, e = 0;
        int noax = !strcmp(argv[i + 2], "-");
        uint16_t got = 0;
        uint32_t a, lo = (uint32_t)(HL_S3306 + HL_BASE) << 4, hi = (uint32_t)(HL_DATA + HL_BASE) << 4, st;
        const char *r = argv[i];
#define ARG(n) frw((uint16_t)ss, (uint16_t)(sp + 4 + 2 * (n)))
#define FARG(n) far_at((uint16_t)ss, (uint16_t)(sp + 4 + 2 * (n)))

        if (sscanf(argv[i + 1], "%x:%x", &ss, &sp) != 2 || (!noax && sscanf(argv[i + 2], "%x", &ax) != 1))
            return 2;
        memset(mem, 0, MEM_SIZE);
        if (!load(argv[i + 3], mem) || !load(argv[i + 4], leave))
            return 2;
        if (!strcmp(r, "aim"))
            got = move_aim(FARG(0), FARG(2), ARG(4));
        else if (!strcmp(r, "start"))
            got = move_start(FARG(0), ARG(2));
        else if (!strcmp(r, "order"))
            got = attack_order(FARG(0), FARG(2), ARG(4));
        else if (!strcmp(r, "done"))
            move_done(FARG(0), FARG(2), ARG(4), ARG(5));
        else if (!strcmp(r, "step")) {
            got = view_step(FARG(0), FARG(2), ARG(4), FARG(5), ARG(7));
            ax &= 0xFF;
        } else if (!strcmp(r, "menu"))
            field_menu(FARG(0), ARG(2), ARG(3));
        else if (!strcmp(r, "unmark"))
            attack_unmark(FARG(0), ARG(2));
        else {
            fprintf(stderr, "%s: not a routine of this test\n", r);
            return 2;
        }
        if (noax)
            printf("%s %s", r, argv[i + 4]);
        else {
            printf("%s %s: %04Xh, original %04Xh", r, argv[i + 4], got, ax);
            if (got != ax)
                d++;
        }
        st = (uint32_t)ss << 4;
        for (a = 0; a < RAM; a++) {
            if (mem[a] == leave[a] || (a >= st && a < st + 0x10000u))
                continue;
            if (a >= lo && a < hi) {
                if (d < 8)
                    printf("\n  %05X: port %02X, original %02X", (unsigned)a, mem[a], leave[a]);
                d++;
            } else {
                if (e < 4)
                    printf("\n  elsewhere %05X: port %02X, original %02X", (unsigned)a, mem[a], leave[a]);
                e++;
            }
        }
        printf("; %u different, elsewhere %u\n", d, e);
        if (d)
            bad = 1;
        calls++;
    }
    printf("%d calls, %s\n", calls, bad ? "DIFFERENT" : "all the same");
    return bad;
}
