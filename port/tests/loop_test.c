/* loop_test.c - passes of game_loop (port/src/loop.c) against the
 * original.
 *
 *     loop_test START.ram INPUTS.txt N DUMP.ram [N DUMP.ram ...]
 *
 * START.ram is memory at the beginning of the original's first pass,
 * INPUTS.txt what each side's part read from outside the translated code
 * (port/tests/loop_dumps.py writes both), and each DUMP.ram memory at the
 * beginning of pass N.  The port runs the passes from START.ram with those
 * inputs and is compared at each N: the game's state (S3306 up to the
 * computer's player's segments S39B6 to S3AEB, and S3AEC up to DATA), the
 * two maps and the sides' work buffers; the computer's segments, which
 * the port does not keep, the results its planning leaves in S331F:17F2
 * to 1806, and the rest of memory but the stack's segment
 * (pictures, the input records, the interrupts' data) are counted apart.
 * port/tests/loop_test.sh builds this.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hl.h"

#define RAM 0xA0000u
#define MAXP 20000

static uint8_t want[RAM];
static SideIn in[MAXP][2];
static int have[MAXP];

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

static uint32_t far_lin(Far p) { return lin(p.seg, p.off); }

/* where a difference lies: 0 the game's state, 1 the computer's: its
 * segments, and what the routines its planning calls inside a pass leave
 * (logged in m8_site from pass 292 on: move_range, attack_range,
 * field_neighbours, move_target), S331F:17F2 to 181B (path_find's way,
 * neighbours, move_rec) and move_args (S3306:000A, 12h bytes); and
 * S3AEC:0000 to 0007, which only code in S2138 (computer_queue) was seen
 * writing; 2 else, and name_buf (S3310:00B9, 40h bytes here), the name of
 * the last file opened, which the pictures the port does not draw change */
static int region(uint32_t a)
{
    uint32_t scratch = ((uint32_t)S_units << 4) + A_free_depth;
    uint32_t args = ((uint32_t)S_move_args << 4) + A_move_args;
    uint32_t name = ((uint32_t)S_name_buf << 4) + A_name_buf;

    uint32_t s3306 = (uint32_t)(HL_S3306 + HL_BASE) << 4, comp = (uint32_t)(HL_S39B6 + HL_BASE) << 4;
    uint32_t comp_end = ((uint32_t)(HL_S3AEC + HL_BASE) << 4) + 8, data = (uint32_t)(HL_DATA + HL_BASE) << 4;
    uint32_t bytes = NW(map_bytes, 0);
    Far m0 = far_at(S_map_ptr, A_map_ptr), m1 = far_at(S_map2_ptr, A_map2_ptr);
    Far w0 = far_at(S_sides, A_sides + 0x0D), w1 = far_at(S_sides, A_sides + 0x17 + 0x0D);

    if ((a >= comp && a < comp_end) || (a >= scratch && a < scratch + 0x2A) || (a >= args && a < args + 0x12))
        return 1;
    if (a >= name && a < name + 0x40)
        return 2;
    if (a >= s3306 && a < data)
        return 0;
    if ((a >= far_lin(m0) && a < far_lin(m0) + bytes) || (a >= far_lin(m1) && a < far_lin(m1) + bytes))
        return 0;
    if ((a >= far_lin(w0) && a < far_lin(w0) + 0x1104) || (a >= far_lin(w1) && a < far_lin(w1) + 0x1104))
        return 0;
    return 2;
}

int main(int argc, char **argv)
{
    FILE *f;
    char line[200];
    int i, bad = 0, done = 1, last = 0;
    uint32_t st;

    if (argc < 5 || (argc - 3) % 2) {
        fprintf(stderr, "usage: loop_test START.ram INPUTS.txt N DUMP.ram ...\n");
        return 2;
    }
    memset(mem, 0, MEM_SIZE);
    if (!load(argv[1], mem))
        return 2;
    st = (uint32_t)0x3EA4 << 4;         /* the stack's segment in the dumps */
    f = fopen(argv[2], "r");
    if (!f) {
        fprintf(stderr, "%s: cannot be opened\n", argv[2]);
        return 2;
    }
    while (fgets(line, sizeof line, f)) {
        int p, s, r5, r8, s80, s8;
        unsigned k, w, t;

        if (sscanf(line, "%d %d %x %x %x %d %d %d %d", &p, &s, &k, &w, &t, &r5, &r8, &s80, &s8) != 9
            || p < 1 || p >= MAXP || s < 0 || s > 1)
            continue;
        in[p][s].key = (uint8_t)k;
        in[p][s].word = (uint16_t)w;
        in[p][s].text = (uint16_t)t;
        in[p][s].rec5 = (int16_t)r5;
        in[p][s].rec8 = (int16_t)r8;
        in[p][s].set80 = s80;
        in[p][s].set8 = s8;
        have[p] |= 1 << s;
        if (p > last)
            last = p;
    }
    fclose(f);
    loop_reset();
    for (i = 3; i < argc; i += 2) {
        int n = atoi(argv[i]);
        unsigned d[3] = { 0, 0, 0 };
        uint32_t a;

        for (; done < n; done++) {
            const char *why;

            if (done > last || have[done] != 3) {
                printf("pass %d: no input logged\n", done);
                return 2;
            }
            why = loop_pass(in[done]);
            if (why) {
                printf("pass %d: not translated: %s\n", done, why);
                return 1;
            }
        }
        if (!load(argv[i + 1], want))
            return 2;
        printf("pass %d %s:", n, argv[i + 1]);
        for (a = 0; a < RAM; a++) {
            int r;

            if (mem[a] == want[a] || (a >= st && a < st + 0x10000u))
                continue;
            r = region(a);
            if (r == 0 && d[0] < (getenv("LOOP_ALL") ? 100000u : 12u))
                printf("\n  %05X: port %02X, original %02X", (unsigned)a, mem[a], want[a]);
            d[r]++;
        }
        printf("\n  game's state %u different; the computer's %u, elsewhere %u\n", d[0], d[1], d[2]);
        if (d[0])
            bad = 1;
    }
    printf("%s\n", bad ? "DIFFERENT" : "the game's state the same at every pass compared");
    return bad;
}
