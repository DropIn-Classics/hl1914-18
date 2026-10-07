/* orders_test.c - the orders a round's end carries out (orders_run)
 * against the original.
 *
 *     orders_test SS:BP DI ENTRY.ram EXIT.ram [...]
 *
 * Each set of four is one pass of attacks_run's loop over the units in
 * HL.EXE (S02E6:0E9C to 19B8): SS and BP of attacks_run's frame and DI
 * (the other side) at the loop's first entry, dosrun's -ram dump there and
 * the one at S02E6:19B8.  The side that moved is read from [BP-2], its map
 * from [BP-6].  Memory after orders_run is compared with the second dump
 * in the game's state (from segment S3306 up to DATA and the two maps).  A
 * byte there counts as different when orders_run wrote it and the
 * original has another value; a byte only the original wrote is counted
 * apart and listed, since the original also draws, plays music and loads
 * the libraries again when a site is taken, which the port leaves out.
 * What differs elsewhere is listed.  port/tests/orders_test.sh builds
 * this.
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
        fprintf(stderr, "usage: orders_test SS:BP DI ENTRY.ram EXIT.ram ...\n");
        return 2;
    }
    for (i = 1; i < argc; i += 4) {
        unsigned ss, bp, di, d_state = 0, d_apart = 0, d_else = 0, changed = 0, u;
        uint32_t a, lo_state, hi_state, zone[2];
        uint16_t m;
        uint8_t result = 0xFF;
        Far map, p;
        int k, left;

        if (sscanf(argv[i], "%x:%x", &ss, &bp) != 2 || sscanf(argv[i + 1], "%x", &di) != 1
            || !load(argv[i + 2], entry) || !load(argv[i + 3], leave))
            return 2;
        memset(mem, 0, MEM_SIZE);
        memcpy(mem, entry, RAM);
        m = frw((uint16_t)ss, (uint16_t)(bp - 2));
        map = far_at((uint16_t)ss, (uint16_t)(bp - 6));
        printf("%s: side %u, orders", argv[i + 3], m);
        for (u = 0; u <= 0xF0; u++)
            if (!(NW(unit_flags, u * UNIT_SIZE) & 0x8000) && NB(units, u * UNIT_SIZE + 0x13))
                printf(" %02X:%X", u, NB(units, u * UNIT_SIZE + 0x13));
        left = orders_run(m, (uint16_t)di, map, &result);
        if (left)
            printf("; %d not carried out by the port", left);
        if (result != 0xFF)
            printf("; result %u", result);
        lo_state = (uint32_t)(HL_S3306 + HL_BASE) << 4;
        hi_state = (uint32_t)(HL_DATA + HL_BASE) << 4;
        p = far_at(S_map_ptr, A_map_ptr);
        zone[0] = lin(p.seg, p.off);
        p = far_at(S_map2_ptr, A_map2_ptr);
        zone[1] = lin(p.seg, p.off);
        for (a = 0; a < RAM; a++) {
            int state = a >= lo_state && a < hi_state;
            changed += mem[a] != entry[a];
            if (mem[a] == leave[a])
                continue;
            for (k = 0; k < 2; k++)
                if (a >= zone[k] && a < zone[k] + MAP_BLOCK)
                    state = 1;
            if (state && mem[a] != entry[a]) {
                if (d_state++ < 8)
                    printf(" %05X", (unsigned)a);
            } else if (state) {
                if (d_apart++ < 12)
                    printf(" [%05X]", (unsigned)a);
            } else if (d_else++ < 8)
                printf(" (%05X)", (unsigned)a);
        }
        printf("; state %u, only the original's %u, elsewhere %u; bytes orders_run changed %u\n",
               d_state, d_apart, d_else, changed);
        if (d_state || left)
            bad++;
        calls++;
    }
    printf("%d calls, %d different\n", calls, bad);
    return bad != 0;
}
