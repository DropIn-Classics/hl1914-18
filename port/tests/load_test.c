/* load_test.c - file_load against the original.
 *
 *     load_test GAMEDIR  SS:SP AX DX ENTRY.ram EXIT.ram [...]
 *
 * Each set of five is one call in HL.EXE: SS:SP (hex) at file_load's
 * entry, AX and DX at its RETF, and dosrun's -ram dumps at both.  The
 * arguments are read from the stack of the first dump; after the port's
 * file_load, which reads the file from GAMEDIR, the returned pointer and
 * all memory are compared with the second.  Differences in the stack below
 * the entry's SP and in the bytes `beside` names are only counted; a call with no place given (the
 * original then allocates) is skipped.  port/tests/load_test.sh builds
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

/* Bytes that change while the original's file_load runs and that it does
 * not write itself (read in its code): the BIOS clock, two bytes of S30EF
 * and some of DATA; presumably the timer's and the sound's interrupt
 * handlers write them (not checked).  They are counted apart. */
static int beside(uint32_t a)
{
    uint32_t data = (uint32_t)(HL_DATA + HL_BASE) << 4, snd = (uint32_t)(HL_S30EF + HL_BASE) << 4;

    if (a >= 0x46C && a < 0x470)
        return 1;
    if (a == snd + 0x22E || a == snd + 0x22F)
        return 1;
    if (a < data)
        return 0;
    a -= data;
    return a == 0x308 || a == 0x3C9 || (a >= 0x3D4 && a < 0x3E8) || (a >= 0x400 && a < 0x414)
        || (a >= 0x424 && a < 0x427) || a == 0xB19 || (a >= 0xD5F && a < 0xD63);
}

int main(int argc, char **argv)
{
    int i, bad = 0, calls = 0, skipped = 0;

    if (argc < 7 || (argc - 2) % 5) {
        fprintf(stderr, "usage: load_test GAMEDIR SS:SP AX DX ENTRY.ram EXIT.ram ...\n");
        return 2;
    }
    hl_game_dir = argv[1];
    for (i = 2; i < argc; i += 5) {
        unsigned ss, sp, ax, dx, d_stack = 0, d_else = 0, d_beside = 0, changed = 0, o;
        uint32_t a, lo_stack, hi_stack;
        Far to, name, work, got;
        char text[80];

        if (sscanf(argv[i], "%x:%x", &ss, &sp) != 2 || sscanf(argv[i + 1], "%x", &ax) != 1
            || sscanf(argv[i + 2], "%x", &dx) != 1
            || !load(argv[i + 3], entry) || !load(argv[i + 4], leave))
            return 2;
        memset(mem, 0, MEM_SIZE);
        memcpy(mem, entry, RAM);
        to = far_at((uint16_t)ss, (uint16_t)(sp + 4));
        name = far_at((uint16_t)ss, (uint16_t)(sp + 8));
        work = far_at((uint16_t)ss, (uint16_t)(sp + 12));
        for (o = 0; o < sizeof text - 1 && pb(name, (uint16_t)o); o++)
            text[o] = (char)pb(name, (uint16_t)o);
        text[o] = 0;
        if ((to.seg == 0 && to.off == 0) || (to.seg == 0xFFFF && to.off == 0xFFFF)) {
            printf("%s: no place given, skipped\n", text);
            skipped++;
            continue;
        }
        got = file_load(to, name, work);
        lo_stack = lin((uint16_t)ss, 0);
        hi_stack = lin((uint16_t)ss, (uint16_t)(sp + 4));
        printf("%s%s:", text, work.seg | work.off ? "" : " (no work buffer)");
        for (a = 0; a < RAM; a++) {
            changed += mem[a] != entry[a];
            if (mem[a] == leave[a])
                continue;
            if (a >= lo_stack && a < hi_stack)
                d_stack++;
            else if (beside(a))
                d_beside++;
            else if (d_else++ < 8)
                printf(" %05X", (unsigned)a);
        }
        if (got.off != ax || got.seg != dx)
            printf(" returns %04X:%04X, the original %04X:%04X;", got.seg, got.off, dx, ax);
        printf(" different %u, beside %u, stack %u; bytes the port's routine changed %u\n",
               d_else, d_beside, d_stack, changed);
        if (d_else || got.off != ax || got.seg != dx)
            bad++;
        calls++;
    }
    printf("%d calls, %d different, %d skipped\n", calls, bad, skipped);
    return bad != 0;
}
