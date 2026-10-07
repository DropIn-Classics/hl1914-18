/* side_test.c - build_list and repair (port/src/side.c) against the
 * original.
 *
 *     side_test build SS:SP ENTRY.ram EXIT.ram [...]
 *     side_test repair SS:BP SI ENTRY.ram EXIT.ram [...]
 *     side_test unit SS:BP SI ENTRY.ram EXIT.ram [...]
 *     side_test out SS:BP SI ENTRY.ram EXIT.ram [...]
 *
 * build: a call of S131D:14D4, its arguments (points, side) read from the
 * stack at SS:SP of the entry dump, the exit dump at its RETF (S131D:15FA).
 * repair: game_loop at S066A:3156 and at 32A9, the view's far pointer at
 * SS:BP-3Ah and the side in SI; the text the port returns is compared with
 * the original's word at SS:BP-46h.  unit: the building of a unit,
 * game_loop at S066A:387A and at 3A42, view and side as for repair.
 * out: the moving out of a unit, game_loop at S066A:2F52 and 3146.  Memory after the port's routine is
 * compared with the exit dump in the game's state (from segment S3306 up
 * to DATA); differences elsewhere but the stack's segment are counted
 * apart (the maps lie there; so do bytes the interrupts change).  port/tests/side_test.sh builds this.
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
    int i = 1, bad = 0, calls = 0;

    while (i < argc) {
        int out_ = !strcmp(argv[i], "out"), unit = !strcmp(argv[i], "unit");
        int rep = out_ || unit || !strcmp(argv[i], "repair"), n = rep ? 5 : 4;
        unsigned ss, sp, si = 0, d = 0;
        uint32_t a, lo = (uint32_t)(HL_S3306 + HL_BASE) << 4, hi = (uint32_t)(HL_DATA + HL_BASE) << 4;
        const char *in, *out;

        if ((!rep && strcmp(argv[i], "build")) || i + n > argc || sscanf(argv[i + 1], "%x:%x", &ss, &sp) != 2
            || (rep && sscanf(argv[i + 2], "%x", &si) != 1)) {
            fprintf(stderr, "usage: side_test build SS:SP IN OUT | repair|unit|out SS:BP SI IN OUT ...\n");
            return 2;
        }
        in = argv[i + n - 2];
        out = argv[i + n - 1];
        memset(mem, 0, MEM_SIZE);
        if (!load(in, mem) || !load(out, leave))
            return 2;
        if (out_) {
            move_out(far_at((uint16_t)ss, (uint16_t)(sp - 0x3A)), (uint16_t)si);
            printf("out %s: side %u", out, si);
        } else if (unit) {
            build_unit(far_at((uint16_t)ss, (uint16_t)(sp - 0x3A)), (uint16_t)si);
            printf("unit %s: side %u, unit_total %u", out, si, NB(unit_total, 0));
        } else if (rep) {
            uint16_t got = repair(far_at((uint16_t)ss, (uint16_t)(sp - 0x3A)), (uint16_t)si);
            uint16_t want = (uint16_t)(leave[(ss << 4) + (uint16_t)(sp - 0x46)]
                                       | leave[(ss << 4) + (uint16_t)(sp - 0x45)] << 8);
            printf("repair %s: side %u, text %02Xh, original %02Xh", out, si, got, want);
            if (got != want)
                d++;
        } else {
            uint16_t points = frw((uint16_t)ss, (uint16_t)(sp + 4)), side = frw((uint16_t)ss, (uint16_t)(sp + 6));
            build_list(points, side);
            printf("build %s: points %u, side %u, %u kinds", out, points, side, NB(build_count, 0));
        }
        for (a = lo; a < hi; a++)
            if (mem[a] != leave[a]) {
                if (d < 8)
                    printf("\n  %05X: port %02X, original %02X", (unsigned)a, mem[a], leave[a]);
                d++;
            }
        {
            /* elsewhere (the maps among it), apart from the stack */
            unsigned e = 0;
            uint32_t st = (uint32_t)ss << 4;
            for (a = 0; a < RAM; a++)
                if ((a < lo || a >= hi) && (a < st || a >= st + 0x10000u) && mem[a] != leave[a]) {
                    if (e < 4)
                        printf("\n  elsewhere %05X: port %02X, original %02X", (unsigned)a, mem[a], leave[a]);
                    e++;
                }
            printf("; %u different, elsewhere %u", d, e);
        }
        printf("\n");
        if (d)
            bad = 1;
        calls++;
        i += n;
    }
    printf("%d calls, %s\n", calls, bad ? "DIFFERENT" : "all the same");
    return bad;
}
