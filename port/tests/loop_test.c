/* loop_test.c - passes of game_loop (port/src/loop.c) against the
 * original.
 *
 *     loop_test [-replay] START.ram INPUTS.txt N DUMP.ram [N DUMP.ram ...]
 *
 * START.ram is memory at the beginning of the original's first pass,
 * INPUTS.txt what each side's part read from outside the translated code
 * (port/tests/loop_dumps.py writes both), and each DUMP.ram memory at the
 * beginning of pass N.  The port runs the passes from START.ram with those
 * inputs and is compared at each N: the game's state (S3306 up to the
 * computer's player's segments S39B6 to S3AEB, and S3AEC up to DATA), the
 * two maps and the sides' work buffers, and what the computer's player
 * keeps and leaves (its segments, S331F:17F2 to 181B, move_args,
 * S3AEC:0000 to 0007, rand_words), which the port's computer_turn writes too; the
 * rest of memory but the stack's segment (pictures, the input records,
 * the interrupts' data) is counted apart.  With -replay the computer's
 * side is not played by computer_turn but replayed from INPUTS.txt (its
 * input word, its text, 80h in game_state's word 2, 8 in its side's
 * word), and what the computer's player keeps is only counted.
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
 * writing; and rand_words (DATA:0AF1, 8 bytes), from which computer_queue
 * draws what to build; 2 else, and name_buf (S3310:00B9, 40h bytes here), the name of
 * the last file opened, and S3AEE, the fight pictures' data, which the
 * pictures the port does not draw change */
static int region(uint32_t a)
{
    uint32_t scratch = ((uint32_t)S_units << 4) + A_free_depth;
    uint32_t args = ((uint32_t)S_move_args << 4) + A_move_args;
    uint32_t name = ((uint32_t)S_name_buf << 4) + A_name_buf;
    uint32_t rnd = ((uint32_t)S_rand_words << 4) + A_rand_words;
    /* S3AEE, the data of the fight's pictures (only S2532, which
     * fight_show calls, uses it) */
    uint32_t pics = (uint32_t)(HL_S3AEE + HL_BASE) << 4, pics_end = (uint32_t)(HL_S3B5D + HL_BASE) << 4;
    /* S3812, the data of S1A39, which shows the picture of a site taken
     * (it writes nothing else of the program's) */
    uint32_t film = (uint32_t)(HL_S3812 + HL_BASE) << 4, film_end = (uint32_t)(HL_S39B6 + HL_BASE) << 4;

    uint32_t s3306 = (uint32_t)(HL_S3306 + HL_BASE) << 4, comp = (uint32_t)(HL_S39B6 + HL_BASE) << 4;
    uint32_t comp_end = ((uint32_t)(HL_S3AEC + HL_BASE) << 4) + 8, data = (uint32_t)(HL_DATA + HL_BASE) << 4;
    uint32_t bytes = NW(map_bytes, 0);
    Far m0 = far_at(S_map_ptr, A_map_ptr), m1 = far_at(S_map2_ptr, A_map2_ptr);
    Far w0 = far_at(S_sides, A_sides + 0x0D), w1 = far_at(S_sides, A_sides + 0x17 + 0x0D);

    if ((a >= comp && a < comp_end) || (a >= scratch && a < scratch + 0x2A) || (a >= args && a < args + 0x12)
        || (a >= rnd && a < rnd + 8))
        return 1;
    if (a >= name && a < name + 0x40)
        return 2;
    if ((a >= pics && a < pics_end) || (a >= film && a < film_end))
        return 2;
    if (a >= s3306 && a < data)
        return 0;
    if ((a >= far_lin(m0) && a < far_lin(m0) + bytes) || (a >= far_lin(m1) && a < far_lin(m1) + bytes))
        return 0;
    if ((a >= far_lin(w0) && a < far_lin(w0) + 0x1104) || (a >= far_lin(w1) && a < far_lin(w1) + 0x1104))
        return 0;
    return 2;
}

/* the fight's pictures, which run here once pictures_load can load them,
 * draw from rand_words (volley_end) and play every volley, while those of
 * the original's run did not draw (loop_dumps.py refuses a run in which
 * rand_next is called by other code than rand_range): words 4 and 6 of
 * rand_words, which clock_seed leaves alone, are given back at their end;
 * words 0 and 2 come from the log after attacks_run (loop.c) */
static uint16_t kept[2];
static int keeping;

static void pics_keep(int at)
{
    if (at == PICS_SEED) {
        if (!keeping) {
            kept[0] = NW(rand_words, 4);
            kept[1] = NW(rand_words, 6);
            keeping = 1;
        }
        clock_seed_bios();
    } else if (at == PICS_END && keeping) {
        SETW(rand_words, 4, kept[0]);
        SETW(rand_words, 6, kept[1]);
        keeping = 0;
    }
}

int main(int argc, char **argv)
{
    FILE *f;
    char line[200];
    int i, bad = 0, done = 1, last = 0;
    uint32_t st;

    /* the game's files, which pictures_load reads after a fight: run from
     * port/, so ../game unless $DOSKIT_GAME names another folder */
    hl_game_dir = getenv("DOSKIT_GAME") ? getenv("DOSKIT_GAME") : "../game";
    pics_hook = pics_keep;
    if (argc > 1 && !strcmp(argv[1], "-replay")) {
        loop_replay = 1;
        argc--;
        argv++;
    }
    if (argc < 5 || (argc - 3) % 2) {
        fprintf(stderr, "usage: loop_test START.ram INPUTS.txt N DUMP.ram ...\n");
        return 2;
    }
    memset(mem, 0, MEM_SIZE);
    if (!load(argv[1], mem))
        return 2;
    st = (uint32_t)0x3EA4 << 4;         /* the stack's segment in the dumps */
    /* the far pointers game_loop keeps in its frame (BP 0FEAh there), which
     * battle.c sets in a battle the port begins itself: without them
     * pictures_load, after a fight or a site taken, fails at its first load
     * and leaves the music's digit (S331F:0284) unwritten */
    game_mem.p = far_at(0x3EA4, 0x0FEA - 0x04);
    game_mem.block = far_at(0x3EA4, 0x0FEA - 0x08);
    game_mem.save_buf = far_at(0x3EA4, 0x0FEA - 0x0C);
    game_mem.gfx = far_at(0x3EA4, 0x0FEA - 0x10);
    game_mem.libs = far_at(0x3EA4, 0x0FEA - 0x14);
    game_mem.lists_end = far_at(0x3EA4, 0x0FEA - 0x18);
    game_mem.pics = far_at(0x3EA4, 0x0FEA - 0x1C);
    game_mem.work = far_at(0x3EA4, 0x0FEA - 0x20);
    f = fopen(argv[2], "r");
    if (!f) {
        fprintf(stderr, "%s: cannot be opened\n", argv[2]);
        return 2;
    }
    while (fgets(line, sizeof line, f)) {
        int p, s, r5, r8, s80, s8, turns = -1, seeded = 0;
        unsigned k, w, t;
        unsigned long seed = 0;

        /* the columns from the tenth on (the computer's calls in its
         * loop, clock_seed) are not in inputs logged before computer_turn
         * was called by the pass */
        if (sscanf(line, "%d %d %x %x %x %d %d %d %d %d %d %lx", &p, &s, &k, &w, &t, &r5, &r8, &s80, &s8, &turns,
                   &seeded, &seed) < 9
            || p < 1 || p >= MAXP || s < 0 || s > 1)
            continue;
        in[p][s].key = (uint8_t)k;
        in[p][s].word = (uint16_t)w;
        in[p][s].text = (uint16_t)t;
        in[p][s].rec5 = (int16_t)r5;
        in[p][s].rec8 = (int16_t)r8;
        in[p][s].set80 = s80;
        in[p][s].set8 = s8;
        in[p][s].turns = turns;
        in[p][s].seeded = seeded;
        in[p][s].seed = (uint32_t)seed;
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
            if (r == 1 && !loop_replay) {
                if (d[1] < (getenv("LOOP_ALL") ? 100000u : 12u))
                    printf("\n  %05X: port %02X, original %02X (the computer's)", (unsigned)a, mem[a], want[a]);
            }
            if (r == 0 && d[0] < (getenv("LOOP_ALL") ? 100000u : 12u))
                printf("\n  %05X: port %02X, original %02X", (unsigned)a, mem[a], want[a]);
            d[r]++;
        }
        printf("\n  game's state %u different; the computer's %u, elsewhere %u\n", d[0], d[1], d[2]);
        if (d[0] || (d[1] && !loop_replay))
            bad = 1;
    }
    printf("%s\n", bad ? "DIFFERENT" : loop_replay ? "the game's state the same at every pass compared"
           : "the game's state and the computer's the same at every pass compared");
    return bad;
}
