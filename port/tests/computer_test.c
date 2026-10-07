/* computer_test.c - the computer's player (port/src/computer.c) against
 * the original.
 *
 *     computer_test input SIDE SS:BP ENTRY.ram EXIT.ram [...]
 *
 * Each set of five is one call of computer_input in HL.EXE: the side, SS
 * and BP of the routine's frame, dosrun's -ram dump at S1E4B:0023 (behind
 * the routine's entry code) and the one at S1E4B:011D (before its result
 * is loaded from [BP-8]).  The result and the memory after the port's
 * routine are compared with the second dump; the routine's own frame (the
 * 0Ah bytes below BP) is left out.  port/tests/computer_test.sh builds
 * this.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hl.h"

#define RAM 0xA0000u

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
        fprintf(stderr, "usage: computer_test input SIDE SS:BP ENTRY.ram EXIT.ram ...\n");
        return 2;
    }
    for (i = 1; i < argc; i += 5) {
        unsigned side, ss, bp, diff = 0, changed = 0;
        uint32_t a, frame;
        uint16_t got, want;

        if (strcmp(argv[i], "input") || sscanf(argv[i + 1], "%u", &side) != 1
            || sscanf(argv[i + 2], "%x:%x", &ss, &bp) != 2
            || !load(argv[i + 3], entry) || !load(argv[i + 4], leave))
            return 2;
        memset(mem, 0, MEM_SIZE);
        memcpy(mem, entry, RAM);
        got = computer_input((uint16_t)side);
        frame = lin((uint16_t)ss, (uint16_t)(bp - 0x0A));
        want = (uint16_t)(leave[frame + 2] | leave[frame + 3] << 8);
        printf("%s: side %u, input %04X, result %u", argv[i + 4], side,
               NW(computer_out, side * 4), got);
        for (a = 0; a < RAM; a++) {
            changed += mem[a] != entry[a];
            if (mem[a] == leave[a] || (a >= frame && a < frame + 0x0A))
                continue;
            if (diff++ < 8)
                printf(" %05X", (unsigned)a);
        }
        if (got != want)
            printf("; the original's result %u", want);
        printf("; different %u; bytes the port changed %u\n", diff, changed);
        if (diff || got != want)
            bad++;
        calls++;
    }
    printf("%d calls, %d different\n", calls, bad);
    return bad != 0;
}
