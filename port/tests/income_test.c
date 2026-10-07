/* income_test.c - a round's income (income_add) against the original.
 *
 *     income_test SIDE ENTRY.ram EXIT.ram [...]
 *
 * Each set of three is one call of S131D:1745 in HL.EXE: its argument and
 * dosrun's -ram dumps at its entry and at its RETF (S131D:178C).  Memory
 * after income_add is compared with the second dump in the game's state
 * (from segment S3306 up to DATA); the side's points before and after are
 * printed.  port/tests/income_test.sh builds this.
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
    int i, bad = 0;

    if (argc < 4 || (argc - 1) % 3) {
        fprintf(stderr, "usage: income_test SIDE ENTRY.ram EXIT.ram ...\n");
        return 2;
    }
    for (i = 1; i < argc; i += 3) {
        unsigned side, d = 0;
        uint16_t before;
        uint32_t a, lo = (uint32_t)(HL_S3306 + HL_BASE) << 4, hi = (uint32_t)(HL_DATA + HL_BASE) << 4;

        if (sscanf(argv[i], "%x", &side) != 1 || side > 1)
            return 2;
        memset(mem, 0, MEM_SIZE);
        if (!load(argv[i + 1], mem) || !load(argv[i + 2], leave))
            return 2;
        before = NW(side_points, side * 2);
        income_add((uint16_t)side);
        for (a = lo; a < hi; a++)
            if (mem[a] != leave[a]) {
                if (d < 8)
                    printf("  %05X: port %02X, original %02X\n", (unsigned)a, mem[a], leave[a]);
                d++;
            }
        printf("%s: side %u, points %u to %u, %u different\n", argv[i + 2], side, before,
               NW(side_points, side * 2), d);
        if (d)
            bad = 1;
    }
    printf(bad ? "DIFFERENT\n" : "all the same\n");
    return bad;
}
