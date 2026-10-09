/* load_test.c - file_load, map_load, shp_load, file_name, num_text,
 * mem_alloc, battle_setup and battle_begin against the original.
 *
 *     load_test GAMEDIR  (load|lib|pict|scr|view|marks|msg|map|shp|name|num|alloc|start|setup|begin) SS:SP AX DX ENTRY.ram EXIT.ram [...]
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
 * there written back to the frame before the comparison.  A byte the
 * port's routine left as it was is counted apart when it differs, and so
 * is the word file_load keeps of what lay behind a file (for `start` all
 * that the loading routines keep for themselves: the routines left out
 * there load further files).  `begin` is the part from
 * S066A:0938 to S066A:0EE6 (battle_begin), SS:BP likewise, the work buffer
 * and the palette's buffer read from the frame, the video memory drawn
 * from the entry's and compared (for `setup` too, whole); begin_apart says what else is counted
 * apart there.  LOAD_TEST_VRAM names a file for the
 * port's video memory at the end (as -vram writes it).
 * `lib` is one call of lib_load (S0C66:0009) with its five arguments.
 * `pict` is one of pictures_load (S02E6:2B87); beside each dump the
 * video memory dosrun's -vram wrote (the same name ending .vram) is
 * loaded into the VGA (mode 13h made unchained) and compared at the end.
 * `view` is one of view_draw (S0E56:0D44), `marks` one of marks_draw
 * (S0E56:0F36), `msg` one of show_message(text, side) (S10AF:000F, its
 * RETF at 009B for -1, at 0130 for a text), `fdraw` one of field_draw
 * (S0E56:145A, RETF at 1725), `merase` one of marks_erase (S0E56:1217,
 * RETF at 1459), `panel` one of unit_panel (S10AF:0131, RETF at
 * S10E1:000E), `fpanel` one of fight_panel (S131D:178D, RETF at 1CD7
 * with pictures, 1F0C without; it returns 0, AX compared), the video
 * memory likewise, `fpics` one of fight_pictures (S2532:0008, RETF at
 * 01C1; AX not compared, the sound's segment, DATA:0309 and the key's
 * record counted apart as for `fpanel`), the video memory likewise,
 * AX and DX not compared (it returns nothing); begin_apart's bytes are
 * counted beside (byte 8 of the input records changed while it ran).
 * `fgrey` and `fback` are fade_grey (S0E56:21D8, RETF at 241E) and
 * fade_back (S0E56:1F91, RETF at 21D7), `bar` the bar of S1571:01F2 (its
 * RETF taken at the caller, S066A:1212; its seven argument words, two
 * of which it halves, counted as the stack), `progress`
 * computer_progress (S1C74:0354, RETF at 0519; AX compared, no video
 * memory), `think` game_loop's part that draws the bar of the computer's
 * thinking (S066A:11C1, the call of computer_progress, to S066A:1212
 * behind bar_draw: SS:SP at that call), `blast` S02E6:225C (RETF at
 * the caller, S02E6:0BC2) and `pic` pic_draw (S2A6B:0008), each with the
 * video memory likewise; for the fades the keyboard interrupt's bytes
 * counted apart (key_apart).  Beside every dump a .dac (dumps.py with
 * DAC set, the runner's -dac) is loaded into the VGA's DAC at the entry
 * and compared at the exit.
 * `scr` is one of screen_open (S298E:000D), the video memory likewise;
 * it returns nothing, so AX is not compared.
 * port/tests/load_test.sh builds this.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hl.h"
#include "vga.h"

#define RAM 0xA0000u

static uint8_t entry[RAM], leave[RAM];

#define VRAM 0x40000u
static uint8_t vleave[VRAM], vtmp[VRAM];

/* a dump's video memory beside it (.ram made .vram): 1, 0 if none */
static int load_vram(const char *ram, uint8_t *to)
{
    char path[1024];
    size_t n = strlen(ram);
    FILE *f;

    if (n < 4 || n + 2 > sizeof path)
        return 0;
    memcpy(path, ram, n - 4);
    strcpy(path + n - 4, ".vram");
    if (!(f = fopen(path, "rb")))
        return 0;
    n = fread(to, 1, VRAM, f);
    fclose(f);
    return n == VRAM;
}

/* a dump's DAC beside it (.ram made .dac, dumps.py with DAC set: 256
 * colours of three 6-bit bytes): 1, 0 if none */
static int load_dac(const char *ram, uint8_t *to)
{
    char path[1024];
    size_t n = strlen(ram);
    FILE *f;

    if (n < 4 || n + 1 > sizeof path)
        return 0;
    memcpy(path, ram, n - 4);
    strcpy(path + n - 4, ".dac");
    if (!(f = fopen(path, "rb")))
        return 0;
    n = fread(to, 1, 768, f);
    fclose(f);
    return n == 768;
}

static uint8_t dac_tmp[768];

/* the DAC beside an entry's dump into the VGA */
static void dac_in(const char *ram)
{
    int k;

    if (!load_dac(ram, dac_tmp))
        return;
    vga_outb(0x3C8, 0);
    for (k = 0; k < 768; k++)
        vga_outb(0x3C9, dac_tmp[k]);
}

/* the colours of the VGA's DAC that differ from those beside the exit's
 * dump; -1 if there is none */
static int dac_diff(const char *ram)
{
    int k, d = 0;

    if (!load_dac(ram, dac_tmp))
        return -1;
    vga_outb(0x3C7, 0);
    for (k = 0; k < 768; k++)
        if (vga_inb(0x3C9) != dac_tmp[k])
            d++;
    return d;
}

/* the video memory beside an entry's dump into the VGA, unchained */
static void vram_in(const char *ram)
{
    unsigned o, pl;

    if (!load_vram(ram, vtmp))
        return;
    vga_set_mode(0x13);
    dac_in(ram);
    vga_outb(0x3C4, 4);
    vga_outb(0x3C5, 0x06);
    for (pl = 0; pl < 4; pl++) {
        vga_outb(0x3C4, 2);
        vga_outb(0x3C5, (uint8_t)(1 << pl));
        for (o = 0; o < 0x10000; o++)
            vga_write((uint16_t)o, vtmp[4 * o + pl]);
    }
    vga_outb(0x3C4, 2);
    vga_outb(0x3C5, 0x0F);
}

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
/* in `fpanel` what the panel's waits and sounds write, which the port
 * leaves to its clock (play.c) or out: the sound's segment S30EF (the
 * steps' sounds, S131D:1FF0), the timer count's high byte DATA:0309, and
 * the key's record behind DATA:00BE (DATA:045C to 0461: wait_input
 * clears its word 4, the keyboard's interrupt writes it) */
static int sound_apart(uint32_t a)
{
    uint32_t snd = (uint32_t)(HL_S30EF + HL_BASE) << 4, data = (uint32_t)(HL_DATA + HL_BASE) << 4;

    return (a >= snd && a < snd + 0x1400) || a == data + 0x0309 || (a >= data + 0x045C && a < data + 0x0462);
}

/* in `fgrey` and `fback` (1.4 s each) what the keyboard's interrupt
 * writes meanwhile: the table of keys held and the last byte
 * (DATA:0B1B to 0B2B, key_irq) */
static int key_apart(uint32_t a)
{
    uint32_t data = (uint32_t)(HL_DATA + HL_BASE) << 4;

    return a >= data + 0x0B1B && a <= data + 0x0B2B;
}

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

/* In `start`, besides: the pages' words (DATA:0322 to 0331, 0354 to
 * 0357), which page_flip swaps: the original flips 73 times before
 * S066A:0349 on map 0, the port twice (screen_open's and game_loop's);
 * the other 71 are in routines the port leaves out there.  battle_setup's
 * pages_set sets them anew. */
static int start_apart(uint32_t a)
{
    uint32_t data = (uint32_t)(HL_DATA + HL_BASE) << 4;

    return (a >= data + 0x322 && a < data + 0x332) || (a >= data + 0x354 && a < data + 0x358);
}

/* In `scr`, besides: bytes of the timer's tables (what is left of a
 * period, DATA:0D5F to 0D9E, and the words at DATA:0E3F and 0E4B), which
 * change while the original waits for the retraces; presumably its
 * interrupt handler writes them (not checked). */
static int scr_apart(uint32_t a)
{
    uint32_t data = (uint32_t)(HL_DATA + HL_BASE) << 4;

    return (a >= data + 0xD5F && a < data + 0xD9F) || a == data + 0xE3F || a == data + 0xE40
        || a == data + 0xE4B || a == data + 0xE4C;
}

int main(int argc, char **argv)
{
    int i, bad = 0, calls = 0, skipped = 0;

    if (argc < 8 || (argc - 2) % 6) {
        fprintf(stderr, "usage: load_test GAMEDIR (load|lib|pict|scr|view|marks|msg|map|shp|name|num|alloc|start|setup|begin) SS:SP AX DX ENTRY.ram EXIT.ram ...\n");
        return 2;
    }
    hl_game_dir = argv[1];
    for (i = 2; i < argc; i += 6) {
        const char *what = argv[i++];
        int wide = 1;
        unsigned ss, sp, ax, dx, d_stack = 0, d_else = 0, d_beside = 0, d_left = 0, changed = 0, o;
        uint32_t a, lo_stack, hi_stack, kept = ((uint32_t)(HL_S2C45 + HL_BASE) << 4) + 0x494;
        int left_out = 0, r_pict = 99;
        Far to, name, work, got = { 0, 0 };
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
            /* the video memory compared but for the grounds at AC58:0,
             * which S02E6:323E (left out) puts there */
            vram_in(argv[i + 3]);
            got.off = (uint16_t)battle_setup();
            r_pict = 0;
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
            game_mem.save_buf = far_at((uint16_t)ss, (uint16_t)(bp - 0x0C));
            /* the video memory drawn from the entry's and compared */
            vram_in(argv[i + 3]);
            got.off = (uint16_t)battle_begin();
            r_pict = 0;
            got.seg = 0;
            wide = 0;
            name.seg = name.off = 0;
            left_out = 3;
        } else if (!strcmp(what, "pict")) {
            /* pictures_load(place, work, palette buffer) */
            Far place = to;
            vram_in(argv[i + 3]);
            r_pict = pictures_load(&place, name, work);
            got = place;
            name.seg = name.off = 0;
            snprintf(text, sizeof text, "%d", r_pict);
        } else if (!strcmp(what, "msg")) {
            /* show_message(text, side): video memory likewise */
            vram_in(argv[i + 3]);
            show_message(frw((uint16_t)ss, (uint16_t)(sp + 4)), frw((uint16_t)ss, (uint16_t)(sp + 6)));
            r_pict = 0;
            got.off = (uint16_t)ax;
            got.seg = (uint16_t)dx;
            name.seg = name.off = 0;
        } else if (!strcmp(what, "fdraw")) {
            /* field_draw(side, field, flag): video memory likewise */
            vram_in(argv[i + 3]);
            field_draw(frw((uint16_t)ss, (uint16_t)(sp + 4)), frw((uint16_t)ss, (uint16_t)(sp + 6)));
            r_pict = 0;
            got.off = (uint16_t)ax;
            got.seg = (uint16_t)dx;
            name.seg = name.off = 0;
        } else if (!strcmp(what, "panel")) {
            /* unit_panel(unit, side): video memory likewise */
            vram_in(argv[i + 3]);
            unit_panel(frb((uint16_t)ss, (uint16_t)(sp + 4)), frw((uint16_t)ss, (uint16_t)(sp + 6)));
            r_pict = 0;
            got.off = (uint16_t)ax;
            got.seg = (uint16_t)dx;
            name.seg = name.off = 0;
        } else if (!strcmp(what, "fpanel")) {
            /* fight_panel(far record, pictures): video memory likewise */
            vram_in(argv[i + 3]);
            got.off = (uint16_t)fight_panel(far_at((uint16_t)ss, (uint16_t)(sp + 4)), frw((uint16_t)ss, (uint16_t)(sp + 8)));
            got.seg = (uint16_t)dx;
            r_pict = 0;
            name.seg = name.off = 0;
        } else if (!strcmp(what, "fpics")) {
            /* fight_pictures(its 18 words of arguments): video memory
             * likewise */
            FightPics a;
            uint16_t s0 = (uint16_t)ss, p0 = (uint16_t)(sp + 4);

            vram_in(argv[i + 3]);
            a.near0 = frb(s0, p0);
            a.standing = frb(s0, (uint16_t)(p0 + 2));
            a.show = frw(s0, (uint16_t)(p0 + 4));
            a.winter = frw(s0, (uint16_t)(p0 + 6));
            a.kind0 = frw(s0, (uint16_t)(p0 + 8));
            a.kind1 = frw(s0, (uint16_t)(p0 + 10));
            a.ground0 = frw(s0, (uint16_t)(p0 + 12));
            a.ground1 = frw(s0, (uint16_t)(p0 + 14));
            a.count0 = frw(s0, (uint16_t)(p0 + 16));
            a.count1 = frw(s0, (uint16_t)(p0 + 18));
            a.after0 = frw(s0, (uint16_t)(p0 + 20));
            a.after1 = frw(s0, (uint16_t)(p0 + 22));
            a.none = far_at(s0, (uint16_t)(p0 + 24));
            a.place = far_at(s0, (uint16_t)(p0 + 28));
            a.work = far_at(s0, (uint16_t)(p0 + 32));
            a.sp = (uint16_t)sp;
            /* AX is what the sound's routine left: not compared, but
             * the port's 2 for the original's error exit */
            got.off = (uint16_t)fight_pictures(&a);
            if (!got.off)
                got.off = (uint16_t)ax;
            got.seg = (uint16_t)dx;
            r_pict = 0;
            name.seg = name.off = 0;
        } else if (!strcmp(what, "fgrey") || !strcmp(what, "fback")) {
            /* fade_grey or fade_back(far palette): the video memory and
             * the DAC likewise */
            vram_in(argv[i + 3]);
            (what[1] == 'g' ? fade_grey : fade_back)(far_at((uint16_t)ss, (uint16_t)(sp + 4)));
            r_pict = 0;
            got.off = (uint16_t)ax;
            got.seg = (uint16_t)dx;
            name.seg = name.off = 0;
        } else if (!strcmp(what, "bar")) {
            /* bar(x, y, colour, back, h, value, width), its RETF taken
             * at the caller (S066A:1212): video memory likewise */
            uint16_t p0 = (uint16_t)(sp + 4);

            vram_in(argv[i + 3]);
            bar_draw(frw((uint16_t)ss, p0), frw((uint16_t)ss, (uint16_t)(p0 + 2)), frw((uint16_t)ss, (uint16_t)(p0 + 4)),
                     frw((uint16_t)ss, (uint16_t)(p0 + 6)), frw((uint16_t)ss, (uint16_t)(p0 + 8)),
                     (int16_t)frw((uint16_t)ss, (uint16_t)(p0 + 10)), (int16_t)frw((uint16_t)ss, (uint16_t)(p0 + 12)));
            r_pict = 0;
            got.off = (uint16_t)ax;
            got.seg = (uint16_t)dx;
            name.seg = name.off = 0;
        } else if (!strcmp(what, "progress")) {
            /* computer_progress(side) (S1C74:0354, RETF at 0519): AX */
            uint16_t side = frw((uint16_t)ss, (uint16_t)(sp + 4));

            got.off = computer_progress(side);
            wide = 0;
            name.seg = name.off = 0;
            snprintf(text, sizeof text, "side %u: %02X", side, got.off);
        } else if (!strcmp(what, "think")) {
            /* game_loop's bar of the computer's thinking, from the call
             * of computer_progress (S066A:11C1, the side pushed) to
             * behind bar_draw (S066A:1212), as loop.c's computer_part
             * has it; game_loop keeps the doubled value and the side's
             * x in its frame ([BP-42h], [BP-44h], BP 54h above the SP
             * here): video memory likewise */
            uint16_t side = frw((uint16_t)ss, (uint16_t)sp), bp = (uint16_t)(sp + 0x54);
            uint16_t x = side & 1 ? 0xA0 : 0, v;

            vram_in(argv[i + 3]);
            v = (uint16_t)(computer_progress(side) << 1);
            fww((uint16_t)ss, (uint16_t)(bp - 0x42), v);
            fww((uint16_t)ss, (uint16_t)(bp - 0x44), x);
            bar_draw((uint16_t)(x + 0x17), 0xC3, frb(HL_S331F + HL_BASE, 0x4C85), frb(HL_S331F + HL_BASE, 0x4C87),
                     1, (int16_t)v, 0xC8);
            r_pict = 0;
            got.off = (uint16_t)ax;
            got.seg = (uint16_t)dx;
            name.seg = name.off = 0;
            snprintf(text, sizeof text, "side %u: %02X", side, v >> 1);
        } else if (!strcmp(what, "blast")) {
            /* S02E6:225C(n, far map, view, side), its RETF taken at the
             * caller (S02E6:0BC2): video memory likewise */
            uint16_t p0 = (uint16_t)(sp + 4);

            vram_in(argv[i + 3]);
            blast_draw(frw((uint16_t)ss, p0), far_at((uint16_t)ss, (uint16_t)(p0 + 2)), frw((uint16_t)ss, (uint16_t)(p0 + 6)),
                       frw((uint16_t)ss, (uint16_t)(p0 + 8)));
            r_pict = 0;
            got.off = (uint16_t)ax;
            got.seg = (uint16_t)dx;
            name.seg = name.off = 0;
        } else if (!strcmp(what, "pic")) {
            /* pic_draw(x, y, far picture, dword colours, base) entered at
             * S2A6B:0008 or at the blitter it jumps to (the same frame):
             * video memory likewise */
            uint16_t p0 = (uint16_t)(sp + 4);

            vram_in(argv[i + 3]);
            pic_draw(frw((uint16_t)ss, p0), frw((uint16_t)ss, (uint16_t)(p0 + 2)), far_at((uint16_t)ss, (uint16_t)(p0 + 4)),
                     frb((uint16_t)ss, (uint16_t)(p0 + 12)));
            r_pict = 0;
            got.off = (uint16_t)ax;
            got.seg = (uint16_t)dx;
            name.seg = name.off = 0;
        } else if (!strcmp(what, "merase")) {
            /* marks_erase(field, side, mask): video memory likewise */
            vram_in(argv[i + 3]);
            marks_erase(frw((uint16_t)ss, (uint16_t)(sp + 4)), frw((uint16_t)ss, (uint16_t)(sp + 6)),
                        frb((uint16_t)ss, (uint16_t)(sp + 8)));
            r_pict = 0;
            got.off = (uint16_t)ax;
            got.seg = (uint16_t)dx;
            name.seg = name.off = 0;
        } else if (!strcmp(what, "view") || !strcmp(what, "marks")) {
            /* view_draw or marks_draw(field, side): video memory likewise */
            vram_in(argv[i + 3]);
            (what[0] == 'v' ? view_draw : marks_draw)(frw((uint16_t)ss, (uint16_t)(sp + 4)), frw((uint16_t)ss, (uint16_t)(sp + 6)));
            r_pict = 0;
            got.off = (uint16_t)ax;
            got.seg = (uint16_t)dx;
            name.seg = name.off = 0;
        } else if (!strcmp(what, "scr")) {
            /* screen_open(w, h, n, dword size, pages); it returns nothing:
             * AX and DX are what it leaves */
            vram_in(argv[i + 3]);
            screen_open(frw((uint16_t)ss, (uint16_t)(sp + 4)), frw((uint16_t)ss, (uint16_t)(sp + 6)),
                        frw((uint16_t)ss, (uint16_t)(sp + 10)) | (uint32_t)frw((uint16_t)ss, (uint16_t)(sp + 12)) << 16,
                        frw((uint16_t)ss, (uint16_t)(sp + 14)));
            r_pict = 0;
            got.off = (uint16_t)ax;
            wide = 0;
            name.seg = name.off = 0;
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
        if (getenv("LOAD_TEST_VRAM"))   /* and its video memory */
            vga_write_planes(getenv("LOAD_TEST_VRAM"));
        lo_stack = lin((uint16_t)ss, 0);
        /* with the routine's own arguments, which the original changes
         * (a pointer moved along, a width counted down): 12 bytes, 10 of
         * file_name */
        hi_stack = lin((uint16_t)ss, (uint16_t)(sp + 4 + (!strcmp(what, "name") ? 10 : !strcmp(what, "bar") ? 14 : 12)));
        printf("%s %s:", what, text);
        for (a = 0; a < RAM; a++) {
            changed += mem[a] != entry[a];
            if (mem[a] == leave[a])
                continue;
            if (a >= lo_stack && a < hi_stack)
                d_stack++;
            else if (beside(a) || (!strcmp(what, "scr") && scr_apart(a))
                     || ((!strcmp(what, "fpanel") || !strcmp(what, "fpics") || !strcmp(what, "fgrey") || !strcmp(what, "fback")) && sound_apart(a))
                     || ((!strcmp(what, "fgrey") || !strcmp(what, "fback")) && key_apart(a))
                     || ((!strcmp(what, "view") || !strcmp(what, "marks") || !strcmp(what, "msg")
                          || !strcmp(what, "fdraw") || !strcmp(what, "merase") || !strcmp(what, "panel") || !strcmp(what, "fpanel") || !strcmp(what, "fpics")
                          || !strcmp(what, "fgrey") || !strcmp(what, "fback") || !strcmp(what, "bar") || !strcmp(what, "blast")
                          || !strcmp(what, "pic") || !strcmp(what, "progress") || !strcmp(what, "think"))
                         && begin_apart(a)))
                d_beside++;
            else if (left_out && (mem[a] == entry[a] || a == kept || a == kept + 1))
                d_left++;
            else if (left_out == 2 && start_apart(a))
                d_left++;
            else if (left_out >= 2 && scratch(a))
                d_left++;
            else if (left_out == 3 && (begin_apart(a) || (a >= lin(game_mem.work.seg, game_mem.work.off)
                                                          && a < lin(game_mem.work.seg, game_mem.work.off) + 0x1000)))
                d_left++;
            else if (d_else++ < 8)
                printf(" %05X", (unsigned)a);
        }
        if (r_pict != 99 && load_vram(argv[i + 5], vleave)) {
            unsigned vd = 0, va = 0, pl, o2;
            for (o2 = 0; o2 < 0x10000; o2++)
                for (pl = 0; pl < 4; pl++)
                    if (vga_plane_byte((int)pl, (uint16_t)o2) == vleave[4 * o2 + pl])
                        continue;
                    else if (vd++ < 4)
                        printf(" vram %X.%u", o2, pl);
            if (va)
                printf(" video memory written by the original only %u,", va);
            printf(" video memory different %u,", vd);
            d_else += vd;
        }
        {
            int dd = dac_diff(argv[i + 5]);

            if (dd >= 0) {
                printf(" DAC bytes different %d,", dd);
                d_else += (unsigned)dd;
            }
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
