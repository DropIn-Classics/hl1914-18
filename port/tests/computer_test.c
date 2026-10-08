/* computer_test.c - the computer's player (port/src/computer.c) against
 * the original.
 *
 *     computer_test KIND SIDE SS:BP ENTRY.ram EXIT.ram [...]
 *     computer_test -f FILE [...]      the same list, read from files
 *
 * Each set of five is one call of a routine in HL.EXE: the side, SS and
 * BP of the routine's frame, and dosrun's -ram dumps behind the routine's
 * entry code and before its return.  KIND:
 *   input    computer_input, dumps at S1E4B:0023 and S1E4B:011D (before
 *            its result is loaded from [BP-8], where it is read here);
 *   step     computer_step, dumps at S1E4B:014F and S1E4B:035B (result 0);
 *   stepend  the same with the exit S1E4B:0372 (result FFFFh);
 *   assess   computer_assess, dumps at S1CC6:0A54 and S1CC6:1835 (result
 *            0); assessend with the exit S1CC6:182B (result 1);
 *   queue    computer_queue, dumps at S1CC6:0022 and where computer_turn
 *            gets back to, S1C74:010F (result 0); queueend the same
 *            with the result 1;
 *   plan     computer_plan, dumps at S2138:002B and S1C74:00CE; planend.
 *            For these three the side is read from the frame as well
 *            and must be SIDE.
 *   turn:N   computer_turn, N calls of it one after the other from the
 *            dump at S1C74:002C of the first, compared with the dump at
 *            S1C74:01AE of the last (the same place N calls on); it has
 *            no result.  N above 1 is the tight loop game_loop calls it
 *            in while the side thinks.
 *   turns:N  the same for a stretch of passes with game_loop's work
 *            between them, which the port does not do: a byte of the
 *            game's state the port did not write is not counted as
 *            different but as game_loop's.  The second dump is that of
 *            the next call's entry (S1C74:002C), so that a half's
 *            stretches follow one another without a gap.
 * The result and the memory after the port's routine are compared with
 * the second dump; the stack below the routine's BP is left out (the 0Ah
 * bytes of computer_input's frame, 400h bytes for computer_step and what
 * it calls).  A call is different when a byte the port wrote has another
 * value in the second dump, or a byte of the game's state (from segment
 * S3306 up to DATA) has that the port did not write; a byte elsewhere
 * that only the original wrote (an interrupt's, presumably) is counted
 * apart and listed.  A call of computer_step whose step the port does not carry
 * out is counted apart and not compared.  port/tests/computer_test.sh
 * builds this; port/tests/computer_dumps.py makes the dumps.
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

/* the words of the files named, as an argument list */
static char **words(int n, char **files, int *count)
{
    char **list = NULL, word[512];
    int k, have = 1, room = 0;

    for (k = 0; k < n; k++) {
        FILE *f = fopen(files[k], "r");
        if (!f) {
            fprintf(stderr, "%s: cannot be opened\n", files[k]);
            exit(2);
        }
        while (fscanf(f, "%511s", word) == 1) {
            if (have + 1 >= room) {
                room = room ? room * 2 : 4096;
                list = realloc(list, (size_t)room * sizeof *list);
            }
            list[have] = malloc(strlen(word) + 1);
            strcpy(list[have++], word);
        }
        fclose(f);
    }
    *count = have;
    return list;
}

int main(int argc, char **argv)
{
    int i, bad = 0, calls = 0, skipped = 0;

    if (argc > 2 && !strcmp(argv[1], "-f"))
        argv = words(argc - 2, argv + 2, &argc);

    if (argc < 6 || (argc - 1) % 5) {
        fprintf(stderr, "usage: computer_test KIND SIDE SS:BP ENTRY.ram EXIT.ram ...\n");
        return 2;
    }
    for (i = 1; i < argc; i += 5) {
        unsigned side, ss, bp, diff = 0, apart = 0, changed = 0, loop = 0, turns = 0;
        int loose = 0;
        uint32_t a, frame, below;
        uint32_t lo_state = (uint32_t)(HL_S3306 + HL_BASE) << 4;
        uint32_t hi_state = (uint32_t)(HL_DATA + HL_BASE) << 4;
        uint16_t got, want;
        int input = !strcmp(argv[i], "input"), left = 0;
        const char *names[3] = { "assess", "queue", "plan" };
        int phase = -1, k;

        for (k = 0; k < 3; k++)
            if (!strcmp(argv[i], names[k])
                || (!strncmp(argv[i], names[k], strlen(names[k]))
                    && !strcmp(argv[i] + strlen(names[k]), "end")))
                phase = k;
        if (sscanf(argv[i], "turn:%u", &turns) != 1 && sscanf(argv[i], "turns:%u", &turns) == 1)
            loose = 1;
        if ((!input && phase < 0 && !turns && strcmp(argv[i], "step") && strcmp(argv[i], "stepend"))
            || sscanf(argv[i + 1], "%u", &side) != 1
            || sscanf(argv[i + 2], "%x:%x", &ss, &bp) != 2
            || !load(argv[i + 3], entry) || !load(argv[i + 4], leave))
            return 2;
        memset(mem, 0, MEM_SIZE);
        memcpy(mem, entry, RAM);
        below = input ? 0x0A : turns ? 0x440 : 0x400;
        frame = lin((uint16_t)ss, (uint16_t)(bp - below));
        if (input) {
            got = computer_input((uint16_t)side);
            want = (uint16_t)(leave[frame + 2] | leave[frame + 3] << 8);
            printf("%s: side %u, input %04X, result %u", argv[i + 4], side,
                   NW(computer_out, side * 4), got);
        } else if (turns) {
            unsigned n;

            if (frw((uint16_t)ss, (uint16_t)(bp + 6)) != side) {
                fprintf(stderr, "%s: the frame's side is not %u\n", argv[i + 3], side);
                return 2;
            }
            printf("%s: side %u, computer_turn %u times from phase %04X", argv[i + 4], side, turns,
                   NW(computer_phase, side * 0x0C));
            for (n = 0; n < turns; n++)
                computer_turn((uint16_t)side);
            got = want = 0;
            printf(" to %04X, input %04X, text %02X", NW(computer_phase, side * 0x0C),
                   NW(computer_out, side * 4), NW(computer_out, side * 4 + 2));
        } else if (phase >= 0) {
            static const uint8_t state_at[3] = { 5, 3, 4 };
            uint8_t state = NB(computer_phase, side * 0x0C + state_at[phase]);
            uint8_t inner = NB(computer_phase, side * 0x0C + 7);
            uint8_t ride = NB(computer_phase, side * 0x0C + 8);

            /* computer_queue: the unit at hand and its kind of plan */
            uint16_t unit = NW(computer_phase, side * 0x0C + 9);
            uint8_t plan = unit > 0xF0 ? 0
                           : NB(views, side * 0x31 + 0x16) == 2 ? NB(unit_plans2, unit * 9 + 8)
                                                                : NB(unit_plans, unit * 9 + 8);

            if (frw((uint16_t)ss, (uint16_t)(bp + 6)) != side) {
                fprintf(stderr, "%s: the frame's side is not %u\n", argv[i + 3], side);
                return 2;
            }
            got = phase == 0 ? computer_assess((uint16_t)side)
                  : phase == 1 ? computer_queue((uint16_t)side) : computer_plan((uint16_t)side);
            want = argv[i][strlen(names[phase])] ? 1 : 0;
            printf("%s: side %u, computer_%s state %u/%u/%u", argv[i + 4], side,
                   names[phase], state, inner, ride);
            if (phase == 1 && unit <= 0xF0)
                printf(", unit %02X plan %X", unit, plan);
            printf(", result %u", got);
        } else {
            uint8_t at = NB(computer_steps, side * 0x49 + 0x46);
            uint8_t kind = NB(computer_steps, side * 0x49 + at * 7);
            uint8_t s47 = NB(computer_steps, side * 0x49 + 0x47);
            uint8_t s48 = NB(computer_steps, side * 0x49 + 0x48);

            got = computer_step((uint16_t)side, &left);
            want = argv[i][4] ? 0xFFFF : 0;
            printf("%s: side %u, step %u kind %02X at %u/%u, result %04X", argv[i + 4], side,
                   at, kind, s47, s48, got);
            if (left) {
                printf("; not carried out by the port\n");
                skipped++;
                continue;
            }
        }
        for (a = 0; a < RAM; a++) {
            changed += mem[a] != entry[a];
            if (mem[a] == leave[a] || (a >= frame && a < frame + below))
                continue;
            if (loose && mem[a] == entry[a] && a >= lo_state && a < hi_state)
                loop++;
            else if (mem[a] != entry[a] || (a >= lo_state && a < hi_state)) {
                if (diff++ < 8)
                    printf(" %05X", (unsigned)a);
            } else if (apart++ < 8)
                printf(" [%05X]", (unsigned)a);
        }
        if (got != want)
            printf("; the original's result %04X", want);
        printf("; different %u, only the original's elsewhere %u", diff, apart);
        if (loose)
            printf(", game_loop's in the state %u", loop);
        printf("; bytes the port changed %u\n", changed);
        if (diff || got != want)
            bad++;
        calls++;
    }
    printf("%d calls, %d different", calls, bad);
    if (skipped)
        printf(", %d of steps not translated", skipped);
    printf("\n");
    return bad != 0;
}
