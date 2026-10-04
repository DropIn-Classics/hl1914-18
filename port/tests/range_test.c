/* range_test.c - move_range and attack_range against the original.
 *
 *     range_test (move|attack) SS:SP AX ENTRY.ram EXIT.ram [...]
 *
 * Each set of five is one call in HL.EXE: which routine, SS:SP (hex) at its
 * entry, AX (hex) at its end, dosrun's -ram dump at the entry and the one
 * at the routine's end (before the registers are taken back from the
 * stack).  The routine's arguments are read from the stack of the first
 * dump; after the port's routine its result is compared with AX (AL for
 * attack_range) and memory with the second dump.  A call passes when the
 * result is the same and nothing differs in the game's state (from
 * segment S3306, where move_range keeps its arguments, up to DATA, the two
 * maps and the work buffer the routine was given); what differs elsewhere
 * is listed (the stack below the entry's SP is only counted).
 * port/tests/range_test.sh builds this.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hl.h"

#define RAM 0xA0000u
#define MAP_BLOCK 0x1806u
#define BUF_BYTES 0x1104u

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
        fprintf(stderr, "usage: range_test (move|attack) SS:SP AX ENTRY.ram EXIT.ram ...\n");
        return 2;
    }
    for (i = 1; i < argc; i += 5) {
        unsigned ss, sp, ax, got, want, d_stack = 0, d_state = 0, d_else = 0, changed = 0;
        uint32_t a, lo_stack, hi_stack, lo_state, hi_state, map1, map2, work;
        uint16_t arg, s;
        Far m1, m2, buf;

        if (sscanf(argv[i + 1], "%x:%x", &ss, &sp) != 2 || sscanf(argv[i + 2], "%x", &ax) != 1
            || !load(argv[i + 3], entry) || !load(argv[i + 4], leave))
            return 2;
        memset(mem, 0, MEM_SIZE);
        memcpy(mem, entry, RAM);
        s = (uint16_t)ss;
        arg = (uint16_t)(sp + 4);       /* behind the far return address */
#define W(o) frw(s, (uint16_t)(arg + (o)))
        if (!strcmp(argv[i], "move")) {
            buf = far_at(s, (uint16_t)(arg + 2));
            got = (uint16_t)move_range((int16_t)W(0), buf, W(6), (int16_t)W(8), W(0x0A), W(0x0C),
                                       far_at(s, (uint16_t)(arg + 0x0E)));
            want = ax;
        } else if (!strcmp(argv[i], "attack")) {
            buf = far_at(s, arg);
            got = attack_range(buf, (int16_t)W(4), (int16_t)W(6), W(8), W(0x0A), W(0x0C),
                               far_at(s, (uint16_t)(arg + 0x0E)));
            want = ax & 0xFF;
        } else {
            fprintf(stderr, "%s: move or attack\n", argv[i]);
            return 2;
        }
#undef W
        printf("%s %s: result %u", argv[i], argv[i + 4], got);
        if (got != want)
            printf(" (the original's %u)", want);
        lo_stack = lin(s, 0);
        hi_stack = lin(s, (uint16_t)(sp + 4));
        lo_state = (uint32_t)(HL_S3306 + HL_BASE) << 4;
        hi_state = (uint32_t)(HL_DATA + HL_BASE) << 4;
        m1 = far_at(S_map_ptr, A_map_ptr);
        m2 = far_at(S_map2_ptr, A_map2_ptr);
        map1 = lin(m1.seg, m1.off);
        map2 = lin(m2.seg, m2.off);
        work = lin(buf.seg, buf.off);
        for (a = 0; a < RAM; a++) {
            changed += mem[a] != entry[a];
            if (mem[a] == leave[a])
                continue;
            if ((a >= lo_state && a < hi_state) || (a >= map1 && a < map1 + MAP_BLOCK)
                || (a >= map2 && a < map2 + MAP_BLOCK) || (a >= work && a < work + BUF_BYTES)) {
                if (d_state++ < 8)
                    printf(" %05X", (unsigned)a);
            } else if (a >= lo_stack && a < hi_stack)
                d_stack++;
            else if (d_else++ < 8)
                printf(" (%05X)", (unsigned)a);
        }
        printf("; state %u, elsewhere %u, stack %u; bytes the port's routine changed %u\n",
               d_state, d_else, d_stack, changed);
        if (d_state || got != want)
            bad++;
        calls++;
    }
    printf("%d calls, %d different\n", calls, bad);
    return bad != 0;
}
