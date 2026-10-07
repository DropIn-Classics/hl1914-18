/* side_test.c - build_list and repair (port/src/side.c) against the
 * original.
 *
 *     side_test build SS:SP ENTRY.ram EXIT.ram [...]
 *     side_test repair SS:BP SI ENTRY.ram EXIT.ram [...]
 *
 * build: a call of S131D:14D4, its arguments (points, side) read from the
 * stack at SS:SP of the entry dump, the exit dump at its RETF (S131D:15FA).
 * repair: game_loop at S066A:3156 and at 32A9, the view's far pointer at
 * SS:BP-3Ah and the side in SI; the text the port returns is compared with
 * the original's word at SS:BP-46h.  Memory after the port's routine is
 * compared with the exit dump in the game's state (from segment S3306 up
 * to DATA).  port/tests/side_test.sh builds this.
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
        int rep = !strcmp(argv[i], "repair"), n = rep ? 5 : 4;
        unsigned ss, sp, si = 0, d = 0;
        uint32_t a, lo = (uint32_t)(HL_S3306 + HL_BASE) << 4, hi = (uint32_t)(HL_DATA + HL_BASE) << 4;
        const char *in, *out;

        if ((!rep && strcmp(argv[i], "build")) || i + n > argc || sscanf(argv[i + 1], "%x:%x", &ss, &sp) != 2
            || (rep && sscanf(argv[i + 2], "%x", &si) != 1)) {
            fprintf(stderr, "usage: side_test build SS:SP IN OUT | repair SS:BP SI IN OUT ...\n");
            return 2;
        }
        in = argv[i + n - 2];
        out = argv[i + n - 1];
        memset(mem, 0, MEM_SIZE);
        if (!load(in, mem) || !load(out, leave))
            return 2;
        if (rep) {
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
        printf("; %u different\n", d);
        if (d)
            bad = 1;
        calls++;
        i += n;
    }
    printf("%d calls, %s\n", calls, bad ? "DIFFERENT" : "all the same");
    return bad;
}
