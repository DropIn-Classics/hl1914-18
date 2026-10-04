/* fight_test.c - fight_percent and fight_losses against the original.
 *
 *     fight_test [-out DIR] ENTRY.ram EXIT.ram [ENTRY.ram EXIT.ram ...]
 *
 * Each pair is two of dosrun's -ram dumps of one fight in HL.EXE: at
 * -break fight_losses and at the routine's end (S2487:0AB1).  From the
 * first, fight_percent must give the record's per cent bytes again and
 * fight_losses the units, the fight record and the random seed of the
 * second.  With -out the memory after the port's fight_losses is written
 * to DIR/NAME (NAME the exit dump's), for doskit/tools/memcmp.py.
 * port/tests/fight_test.sh builds and runs this.
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

/* how many bytes of mem differ from `with` in [from, from + n) */
static unsigned differ(const uint8_t *with, uint32_t from, uint32_t n)
{
    unsigned d = 0;
    uint32_t i;
    for (i = from; i < from + n; i++)
        d += mem[i] != with[i];
    return d;
}

int main(int argc, char **argv)
{
    const char *out = NULL;
    int i = 1, bad = 0, fights = 0;

    if (argc > 2 && !strcmp(argv[1], "-out")) {
        out = argv[2];
        i = 3;
    }
    if (i >= argc || (argc - i) % 2) {
        fprintf(stderr, "usage: fight_test [-out DIR] ENTRY.ram EXIT.ram ...\n");
        return 2;
    }
    for (; i < argc; i += 2) {
        uint32_t units = lin(S_units, A_units), rec = lin(S_fight_rec, A_fight_rec);
        uint32_t seed = lin(S_rand_state, A_rand_state);
        unsigned d_pc, d_units, d_rec, d_seed, d_all;
        uint8_t a, b;
        Far p;

        if (!load(argv[i], entry) || !load(argv[i + 1], leave))
            return 2;
        memset(mem, 0, MEM_SIZE);
        memcpy(mem, entry, RAM);
        p = far_at(S_fight_rec_ptr, A_fight_rec_ptr);
        if (lin(p.seg, p.off) != rec) {
            fprintf(stderr, "%s: the fight's record is not fight_rec\n", argv[i]);
            return 2;
        }
        a = NB(fight_rec, 8);
        b = NB(fight_rec, 9);
        /* the map and side index as attacks_run picks them */
        if (NB(attack_pick, 0) == 2)
            fight_percent(a, b, far_at(S_map_ptr, A_map_ptr), 0);
        else
            fight_percent(a, b, far_at(S_map2_ptr, A_map2_ptr), 1);
        d_pc = differ(entry, rec, 0x80);
        fight_losses();
        d_units = differ(leave, units, 0xF1 * UNIT_SIZE);
        d_rec = differ(leave, rec, 0x80);
        d_seed = differ(leave, seed, 4);
        d_all = differ(leave, 0, RAM);
        printf("%s units %u %u per cent %u %u: record after fight_percent %u, "
               "after fight_losses units %u record %u seed %u (all memory %u)\n",
               argv[i + 1], a, b, NB(fight_rec, 0x30), NB(fight_rec, 0x31),
               d_pc, d_units, d_rec, d_seed, d_all);
        if (d_pc || d_units || d_rec || d_seed)
            bad++;
        fights++;
        if (out) {
            char path[1024];
            const char *name = strrchr(argv[i + 1], '/');
            snprintf(path, sizeof path, "%s/%s", out, name ? name + 1 : argv[i + 1]);
            if (rm_write(path) != 0) {
                fprintf(stderr, "%s: cannot be written\n", path);
                return 2;
            }
        }
    }
    printf("%d fights, %d different\n", fights, bad);
    return bad != 0;
}
