/* load_test.c - file_load, map_load, shp_load, file_name, num_text,
 * mem_alloc, battle_setup and battle_begin against the original.
 *
 *     load_test GAMEDIR  (load|lib|map|shp|name|num|alloc|start|setup|begin) SS:SP AX DX ENTRY.ram EXIT.ram [...]
 *
 * Each set of six is one call in HL.EXE: the routine, SS:SP (hex) at its
 * entry, AX and DX at its RETF, and dosrun's -ram dumps at both.  The
 * arguments are read from the stack of the first dump; after the port's
 * routine, which reads files from GAMEDIR, the returned value and
 * all memory are compared with the second.  Differences in the stack below
 * the entry's SP, in the routine's own arguments and in the bytes `beside`
 * names are only counted; a call with no place given (the
 * original then allocates) is skipped.  `start` is game_loop from its
 * entry to S066A:0349 (start_setup), `setup` is the part of game_loop
 * from S066A:0417 to S066A:0938 (battle_setup): SS:BP instead of SS:SP,
 * the block's place read from game_loop's frame, the pointers it keeps
 * there written back to the frame before the comparison.  The original
 * calls routines there that the port leaves out (they load the map's
 * pictures into the block, with its beginning as their work buffer), so a
 * byte the port's routine left as it was is counted apart when it
 * differs, and so is the word file_load keeps of what lay behind a file
 * (for `start` all that the loading routines keep for themselves: the
 * routines left out there load further files).  `begin` is the part from
 * S066A:0938 to S066A:0EE6 (battle_begin), SS:BP likewise, the work buffer
 * read from the frame; begin_apart says what else is counted apart there.
 * `lib` is one call of lib_load (S0C66:0009) with its five arguments.
 * port/tests/load_test.sh builds this.
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
    return a == 0x308 || (a >= 0x3C6 && a < 0x3CA) || (a >= 0x3D4 && a < 0x3E8) || (a >= 0x400 && a < 0x414)
        || (a >= 0x424 && a < 0x427) || a == 0xB19 || (a >= 0xD5F && a < 0xD63);
}

/* What the loading routines keep for themselves: file_load's variables,
 * file_size's eight bytes and name_buf.  Between game_loop's entry and
 * the menu the original loads further files in routines the port leaves
 * out, so these hold the last of those there. */
static int scratch(uint32_t a)
{
    uint32_t load = ((uint32_t)(HL_S2C45 + HL_BASE) << 4) + 0x486, size = ((uint32_t)(HL_S2B8F + HL_BASE) << 4) + 0xC1;
    uint32_t name = ((uint32_t)S_name_buf << 4) + A_name_buf, open = (uint32_t)(HL_S2C8E + HL_BASE) << 4;

    return (a >= load && a < load + 0x10) || (a >= size && a < size + 8) || (a >= name && a < name + 0x40)
        || (a >= open && a < open + 6);
}

/* In `begin`, besides the loading routines' own and the work buffer (the
 * original loads further files after the port's one): the value clock_seed
 * makes of the BIOS's clock, and byte 8 of the two records behind DATA:00A6
 * and DATA:00B6, which the setup clears and which is FFh again at its end
 * (presumably an interrupt handler's; not checked). */
static int begin_apart(uint32_t a)
{
    uint32_t data = (uint32_t)(HL_DATA + HL_BASE) << 4;

    return (a >= data + 0xAF1 && a < data + 0xAF5) || a == data + 0x3D0 || a == data + 0x3FC;
}

int main(int argc, char **argv)
{
    int i, bad = 0, calls = 0, skipped = 0;

    if (argc < 8 || (argc - 2) % 6) {
        fprintf(stderr, "usage: load_test GAMEDIR (load|lib|map|shp|name|num|alloc|start|setup|begin) SS:SP AX DX ENTRY.ram EXIT.ram ...\n");
        return 2;
    }
    hl_game_dir = argv[1];
    for (i = 2; i < argc; i += 6) {
        const char *what = argv[i++];
        int wide = 1;
        unsigned ss, sp, ax, dx, d_stack = 0, d_else = 0, d_beside = 0, d_left = 0, changed = 0, o;
        uint32_t a, lo_stack, hi_stack, kept = ((uint32_t)(HL_S2C45 + HL_BASE) << 4) + 0x494;
        int left_out = 0;
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
        text[0] = 0;
        if (!strcmp(what, "name")) {
            got = file_name(frw((uint16_t)ss, (uint16_t)(sp + 4)), (int16_t)frw((uint16_t)ss, (uint16_t)(sp + 6)),
                            name, (int16_t)frw((uint16_t)ss, (uint16_t)(sp + 12)));
            name = got;
        } else if (!strcmp(what, "num")) {
            got = num_text((int32_t)(frw((uint16_t)ss, (uint16_t)(sp + 4))
                                     | (uint32_t)frw((uint16_t)ss, (uint16_t)(sp + 6)) << 16),
                           name, (int16_t)frw((uint16_t)ss, (uint16_t)(sp + 12)),
                           frb((uint16_t)ss, (uint16_t)(sp + 14)));
        } else if (!strcmp(what, "alloc")) {
            /* the runtime's DOS blocks begin behind the program's own */
            rm_psp = HL_PSP;
            rm_arena = (uint16_t)(HL_PSP + frw(HL_PSP - 1, 3));
            got = mem_alloc(frw((uint16_t)ss, (uint16_t)(sp + 4))
                            | (uint32_t)frw((uint16_t)ss, (uint16_t)(sp + 6)) << 16);
            name.seg = name.off = 0;
            snprintf(text, sizeof text, "%04X:%04X", got.seg, got.off);
        } else if (!strcmp(what, "start")) {
            /* game_loop's entry to S066A:0349 (start_setup): SS:SP at the
             * entry, the frame is two bytes below */
            uint16_t bp = (uint16_t)(sp - 2);
            rm_psp = HL_PSP;
            rm_arena = (uint16_t)(HL_PSP + frw(HL_PSP - 1, 3));
            got.off = (uint16_t)start_setup();
            got.seg = 0;
            wide = 0;
            name.seg = name.off = 0;
            fww((uint16_t)ss, (uint16_t)(bp - 4), game_mem.p.off);
            fww((uint16_t)ss, (uint16_t)(bp - 2), game_mem.p.seg);
            fww((uint16_t)ss, (uint16_t)(bp - 8), game_mem.block.off);
            fww((uint16_t)ss, (uint16_t)(bp - 6), game_mem.block.seg);
            left_out = 2;
            snprintf(text, sizeof text, "%04X:%04X", game_mem.block.seg, game_mem.block.off);
        } else if (!strcmp(what, "setup")) {
            uint16_t bp = (uint16_t)sp;
            game_mem.block = far_at((uint16_t)ss, (uint16_t)(bp - 8));
            got.off = (uint16_t)battle_setup();
            got.seg = 0;
            wide = 0;
            name.seg = name.off = 0;
#define KEEP(o, p) (fww((uint16_t)ss, (uint16_t)(bp - (o)), (p).off), fww((uint16_t)ss, (uint16_t)(bp - (o) + 2), (p).seg))
            KEEP(0x04, game_mem.p);
            KEEP(0x0C, game_mem.save_buf);
            KEEP(0x10, game_mem.gfx);
            KEEP(0x18, game_mem.lists_end);
            KEEP(0x20, game_mem.work);
#undef KEEP
            left_out = 1;
            snprintf(text, sizeof text, "%04X:%04X", game_mem.block.seg, game_mem.block.off);
        } else if (!strcmp(what, "begin")) {
            uint16_t bp = (uint16_t)sp;
            game_mem.work = far_at((uint16_t)ss, (uint16_t)(bp - 0x20));
            game_mem.p = far_at((uint16_t)ss, (uint16_t)(bp - 4));
            got.off = (uint16_t)battle_begin();
            got.seg = 0;
            wide = 0;
            name.seg = name.off = 0;
            left_out = 3;
        } else if (!strcmp(what, "lib")) {
            /* lib_load(name, place, work, record, order) */
            int32_t r = lib_load(to, name, work, far_at((uint16_t)ss, (uint16_t)(sp + 16)),
                                 frb((uint16_t)ss, (uint16_t)(sp + 20)));
            name = to;
            got.off = (uint16_t)r;
            got.seg = (uint16_t)((uint32_t)r >> 16);
        } else if (!strcmp(what, "map") || !strcmp(what, "shp")) {
            name = to;
            got.off = what[0] == 'm' ? map_load(to, far_at((uint16_t)ss, (uint16_t)(sp + 8)), work)
                                     : shp_load(to, far_at((uint16_t)ss, (uint16_t)(sp + 8)), work);
            got.seg = 0;
            wide = 0;
        } else if ((to.seg == 0 && to.off == 0) || (to.seg == 0xFFFF && to.off == 0xFFFF)) {
            printf("load: no place given, skipped\n");
            skipped++;
            i--;
            continue;
        } else
            got = file_load(to, name, work);
        if (name.seg | name.off) {
            for (o = 0; o < sizeof text - 1 && pb(name, (uint16_t)o); o++)
                text[o] = (char)pb(name, (uint16_t)o);
            text[o] = 0;
        }
        i--;
        if (getenv("LOAD_TEST_OUT"))    /* the port's memory, for memcmp.py */
            rm_write(getenv("LOAD_TEST_OUT"));
        lo_stack = lin((uint16_t)ss, 0);
        /* with the routine's own arguments, which the original changes
         * (a pointer moved along, a width counted down): 12 bytes, 10 of
         * file_name */
        hi_stack = lin((uint16_t)ss, (uint16_t)(sp + 4 + (strcmp(what, "name") ? 12 : 10)));
        printf("%s %s:", what, text);
        for (a = 0; a < RAM; a++) {
            changed += mem[a] != entry[a];
            if (mem[a] == leave[a])
                continue;
            if (a >= lo_stack && a < hi_stack)
                d_stack++;
            else if (beside(a))
                d_beside++;
            else if (left_out && (mem[a] == entry[a] || a == kept || a == kept + 1))
                d_left++;
            else if (left_out >= 2 && scratch(a))
                d_left++;
            else if (left_out == 3 && (begin_apart(a) || (a >= lin(game_mem.work.seg, game_mem.work.off)
                                                          && a < lin(game_mem.work.seg, game_mem.work.off) + 0x1000)))
                d_left++;
            else if (d_else++ < 8)
                printf(" %05X", (unsigned)a);
        }
        if (got.off != ax || (wide && got.seg != dx))
            printf(" returns %04X:%04X, the original %04X:%04X;", got.seg, got.off, dx, ax);
        if (left_out)
            printf(" written by the original only %u,", d_left);
        printf(" different %u, beside %u, stack %u; bytes the port's routine changed %u\n",
               d_else, d_beside, d_stack, changed);
        if (d_else || got.off != ax || (wide && got.seg != dx))
            bad++;
        calls++;
    }
    printf("%d calls, %d different, %d skipped\n", calls, bad, skipped);
    return bad != 0;
}
