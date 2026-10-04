/* units_test.c - unit_destroy, unit_free, unit_add and the three site_add
 * routines and the parts of map_load and shp_load behind the loading against
 * the original.
 *
 *     units_test (destroy|free|add|site400|site100|site40|map|shp) SS:SP ENTRY.ram EXIT.ram [...]
 *
 * Each set of four is one call in HL.EXE: which routine, SS:SP (hex) at its
 * entry, dosrun's -ram dump at the entry and the one at the routine's end
 * (before the registers are taken back from the stack).  The routine's arguments are read from the stack of the
 * first dump; after the port's routine memory is compared with the second.
 * A call passes when nothing differs in the game's state (from segment
 * S331F up to DATA, and the two maps); what differs elsewhere is listed
 * (the stack below the entry's SP is only counted).
 * port/tests/units_test.sh builds this.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hl.h"

#define RAM 0xA0000u
#define MAP_BLOCK 0x1806u

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

    if (argc < 5 || (argc - 1) % 4) {
        fprintf(stderr, "usage: units_test (destroy|free|add|site400|site100|site40|map|shp) SS:SP ENTRY.ram EXIT.ram ...\n");
        return 2;
    }
    for (i = 1; i < argc; i += 4) {
        unsigned ss, sp, d_stack = 0, d_state = 0, d_else = 0;
        uint32_t a, lo_stack, hi_stack, lo_state, hi_state, map1, map2;
        uint16_t arg;
        Far m1, m2;

        if (sscanf(argv[i + 1], "%x:%x", &ss, &sp) != 2
            || !load(argv[i + 2], entry) || !load(argv[i + 3], leave))
            return 2;
        memset(mem, 0, MEM_SIZE);
        memcpy(mem, entry, RAM);
        arg = (uint16_t)(sp + 4);       /* behind the far return address */
        if (!strcmp(argv[i], "destroy")) {
            Far map = far_at((uint16_t)ss, (uint16_t)(arg + 10));
            unit_destroy(frb((uint16_t)ss, arg), frb((uint16_t)ss, (uint16_t)(arg + 2)),
                         frb((uint16_t)ss, (uint16_t)(arg + 4)),
                         (int8_t)frb((uint16_t)ss, (uint16_t)(arg + 6)),
                         frw((uint16_t)ss, (uint16_t)(arg + 8)), map);
        } else if (!strcmp(argv[i], "free"))
            unit_free(frb((uint16_t)ss, arg), frw((uint16_t)ss, (uint16_t)(arg + 2)));
        else if (!strcmp(argv[i], "add"))
            unit_add(frb((uint16_t)ss, arg), frb((uint16_t)ss, (uint16_t)(arg + 2)),
                     frw((uint16_t)ss, (uint16_t)(arg + 4)), frw((uint16_t)ss, (uint16_t)(arg + 6)));
        else if (!strcmp(argv[i], "site400"))
            site_add_400(frw((uint16_t)ss, arg), frw((uint16_t)ss, (uint16_t)(arg + 2)),
                         frw((uint16_t)ss, (uint16_t)(arg + 4)));
        else if (!strcmp(argv[i], "site100"))
            site_add_100(frw((uint16_t)ss, arg), frw((uint16_t)ss, (uint16_t)(arg + 2)),
                         frw((uint16_t)ss, (uint16_t)(arg + 4)));
        else if (!strcmp(argv[i], "site40"))
            site_add_40(frw((uint16_t)ss, arg), frw((uint16_t)ss, (uint16_t)(arg + 2)));
        else if (!strcmp(argv[i], "map"))   /* SS:BP at S0E56:0411, the file loaded */
            map_setup(far_at((uint16_t)ss, (uint16_t)(sp + 0x0A)));
        else if (!strcmp(argv[i], "shp"))   /* SS:BP at S0E56:013A, the file loaded */
            shp_setup(far_at((uint16_t)ss, (uint16_t)(sp + 0x0A)));
        else {
            fprintf(stderr, "%s: destroy, free, add, site400, site100, site40, map or shp\n", argv[i]);
            return 2;
        }
        printf("%s %s", argv[i], argv[i + 3]);
        lo_stack = lin((uint16_t)ss, 0);
        hi_stack = lin((uint16_t)ss, (uint16_t)(sp + 4));
        lo_state = lin(S_units, 0);
        hi_state = (uint32_t)(HL_DATA + HL_BASE) << 4;
        m1 = far_at(S_map_ptr, A_map_ptr);
        m2 = far_at(S_map2_ptr, A_map2_ptr);
        map1 = lin(m1.seg, m1.off);
        map2 = lin(m2.seg, m2.off);
        for (a = 0; a < RAM; a++) {
            if (mem[a] == leave[a])
                continue;
            if ((a >= lo_state && a < hi_state) || (a >= map1 && a < map1 + MAP_BLOCK)
                || (a >= map2 && a < map2 + MAP_BLOCK)) {
                if (d_state++ < 8)
                    printf(" %05X", (unsigned)a);
            } else if (a >= lo_stack && a < hi_stack)
                d_stack++;
            else if (d_else++ < 8)
                printf(" (%05X)", (unsigned)a);
        }
        {
            unsigned changed = 0;
            for (a = 0; a < RAM; a++)
                changed += mem[a] != entry[a];
            printf(": state %u, elsewhere %u, stack %u; bytes the port's routine changed %u\n",
                   d_state, d_else, d_stack, changed);
        }
        if (d_state)
            bad++;
        calls++;
    }
    printf("%d calls, %d different\n", calls, bad);
    return bad != 0;
}
