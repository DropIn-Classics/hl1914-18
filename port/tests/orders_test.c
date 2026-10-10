/* orders_test.c - the orders a round's end carries out (orders_run)
 * against the original.
 *
 *     orders_test [-vin IN] [-vram PREFIX] SS:BP DI ENTRY.ram EXIT.ram [...]
 *
 * Each set of four is one pass of attacks_run's loop over the units in
 * HL.EXE (S02E6:0E9C to 19B8): SS and BP of attacks_run's frame and DI
 * (the other side) at the loop's first entry, dosrun's -ram dump there and
 * the one at S02E6:19B8.  The side that moved is read from [BP-2], its map
 * from [BP-6].  Memory after orders_run is compared with the second dump
 * in the game's state (from segment S3306 up to DATA, the two maps and the
 * random numbers' four words, rand_words at DATA:0AF1).  A
 * byte there counts as different when orders_run wrote it and the
 * original has another value; a byte only the original wrote is counted
 * apart and listed, since the original also draws, plays music and loads
 * the libraries again when a site is taken, which the port leaves out.
 * What differs elsewhere is listed.  Whether a fight came before in
 * this attacks_run (the port's orders_fought) is [BP-0Eh] of the frame.
 * With -vram the video memory (planes interleaved, as dosrun's -vram) and
 * the DAC (as its -dac) are written where a site taken (order 4) waits
 * wait_frames(1Eh) (PREFIX_wait.vram, .dac; the original's S02E6:12F0)
 * and behind its fade (PREFIX_fade; 1343).  -vin loads IN.vram and
 * IN.dac (dosrun's -vram and -dac at the loop's entry) into the VGA
 * first, in the unchained 256-colour mode the game sets.  port/tests/orders_test.sh
 * builds this.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hl.h"
#include "vga.h"

#define RAM 0xA0000u
#define MAP_BLOCK 0x1806u

static uint8_t entry[RAM], leave[RAM];
static const char *vram_prefix;

static void dac_dump(const char *path)
{
    FILE *f = fopen(path, "wb");
    int i;

    if (!f)
        return;
    vga_outb(0x3C7, 0);
    for (i = 0; i < 0x300; i++)
        fputc(vga_inb(0x3C9), f);
    fclose(f);
}

static void vram_dump(const char *what)
{
    char path[512];

    snprintf(path, sizeof path, "%s_%s.vram", vram_prefix, what);
    vga_write_planes(path);
    snprintf(path, sizeof path, "%s_%s.dac", vram_prefix, what);
    dac_dump(path);
}

/* the original's video memory and colours at the loop's entry */
static int vram_load(const char *in)
{
    char path[512];
    FILE *f;
    long a;
    int c;

    vga_set_mode(0x13);
    vga_outw(0x3C4, 0x0604);    /* chain-4 off */
    vga_outw(0x3D4, 0x0014);
    vga_outw(0x3D4, 0xE317);
    snprintf(path, sizeof path, "%s.vram", in);
    if (!(f = fopen(path, "rb")))
        return 0;
    for (a = 0; a < 4L * 0x10000; a++) {
        if ((c = fgetc(f)) == EOF)
            break;
        vga_outw(0x3C4, (uint16_t)((1 << (a & 3)) << 8 | 2));
        vga_write((uint16_t)(a >> 2), (uint8_t)c);
    }
    fclose(f);
    vga_outw(0x3C4, 0x0F02);
    snprintf(path, sizeof path, "%s.dac", in);
    if (!(f = fopen(path, "rb")))
        return 0;
    vga_outb(0x3C8, 0);
    for (a = 0; a < 0x300 && (c = fgetc(f)) != EOF; a++)
        vga_outb(0x3C9, (uint8_t)c);
    fclose(f);
    return 1;
}

/* order 4's places: its wait after the view is drawn, its fade (fade_in
 * or the wait behind fade_back) */
static void take_hook(int at, int n)
{
    if (at == ORDERS_WAIT && n == 0x1F)
        vram_dump("wait");
    else if (at == ORDERS_FADE || (at == ORDERS_WAIT && n == 0x60))
        vram_dump("fade");
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

int main(int argc, char **argv)
{
    int i, bad = 0, calls = 0;
    const char *vin = NULL;

    if (argc > 2 && !strcmp(argv[1], "-vin")) {
        vin = argv[2];
        argv += 2;
        argc -= 2;
    }
    if (argc > 2 && !strcmp(argv[1], "-vram")) {
        vram_prefix = argv[2];
        orders_hook = take_hook;
        argv += 2;
        argc -= 2;
    }
    hl_game_dir = getenv("DOSKIT_GAME") ? getenv("DOSKIT_GAME") : "../game";
    if (argc < 5 || (argc - 1) % 4) {
        fprintf(stderr, "usage: orders_test [-vin IN] [-vram PREFIX] SS:BP DI ENTRY.ram EXIT.ram ...\n");
        return 2;
    }
    for (i = 1; i < argc; i += 4) {
        unsigned ss, bp, di, d_state = 0, d_apart = 0, d_else = 0, changed = 0, u;
        uint32_t a, lo_state, hi_state, rand_lo, zone[2];
        uint16_t m;
        uint8_t result = 0xFF;
        Far map, p;
        int k;

        if (sscanf(argv[i], "%x:%x", &ss, &bp) != 2 || sscanf(argv[i + 1], "%x", &di) != 1
            || !load(argv[i + 2], entry) || !load(argv[i + 3], leave))
            return 2;
        memset(mem, 0, MEM_SIZE);
        memcpy(mem, entry, RAM);
        if (vin && !vram_load(vin))
            return 2;
        m = frw((uint16_t)ss, (uint16_t)(bp - 2));
        map = far_at((uint16_t)ss, (uint16_t)(bp - 6));
        orders_fought = frb((uint16_t)ss, (uint16_t)(bp - 0x0E)) != 0;
        /* attacks_run's far arguments, as game_loop passes them: the work
         * buffer [BP+6] (game_loop's [BP-18h]), fade_back's buffer
         * [BP+12h] ([BP-0Ch]), the libraries' place [BP+16h] ([BP-14h]) */
        game_mem.work = far_at((uint16_t)ss, (uint16_t)(bp + 6));
        game_mem.save_buf = far_at((uint16_t)ss, (uint16_t)(bp + 0x12));
        game_mem.libs = far_at((uint16_t)ss, (uint16_t)(bp + 0x16));
        printf("%s: side %u, orders", argv[i + 3], m);
        for (u = 0; u <= 0xF0; u++)
            if (!(NW(unit_flags, u * UNIT_SIZE) & 0x8000) && NB(units, u * UNIT_SIZE + 0x13))
                printf(" %02X:%X", u, NB(units, u * UNIT_SIZE + 0x13));
        orders_run(m, (uint16_t)di, map, &result);
        if (result != 0xFF)
            printf("; result %u", result);
        lo_state = (uint32_t)(HL_S3306 + HL_BASE) << 4;
        hi_state = (uint32_t)(HL_DATA + HL_BASE) << 4;
        rand_lo = lin(S_rand_words, A_rand_words);
        p = far_at(S_map_ptr, A_map_ptr);
        zone[0] = lin(p.seg, p.off);
        p = far_at(S_map2_ptr, A_map2_ptr);
        zone[1] = lin(p.seg, p.off);
        for (a = 0; a < RAM; a++) {
            int state = (a >= lo_state && a < hi_state) || (a >= rand_lo && a < rand_lo + 8);
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
        if (d_state)
            bad++;
        calls++;
    }
    printf("%d calls, %d different\n", calls, bad);
    return bad != 0;
}
