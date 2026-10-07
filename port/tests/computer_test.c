/* computer_test.c - the computer's player (port/src/computer.c) against
 * the original.
 *
 *     computer_test KIND SIDE SS:BP ENTRY.ram EXIT.ram [...]
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
 *            For these two the side is read from the frame as well
 *            and must be SIDE.
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

int main(int argc, char **argv)
{
    int i, bad = 0, calls = 0, skipped = 0;

    if (argc < 6 || (argc - 1) % 5) {
        fprintf(stderr, "usage: computer_test KIND SIDE SS:BP ENTRY.ram EXIT.ram ...\n");
        return 2;
    }
    for (i = 1; i < argc; i += 5) {
        unsigned side, ss, bp, diff = 0, apart = 0, changed = 0;
        uint32_t a, frame, below;
        uint32_t lo_state = (uint32_t)(HL_S3306 + HL_BASE) << 4;
        uint32_t hi_state = (uint32_t)(HL_DATA + HL_BASE) << 4;
        uint16_t got, want;
        int input = !strcmp(argv[i], "input"), left = 0;
        const char *names[2] = { "assess", "queue" };
        int phase = -1, k;

        for (k = 0; k < 2; k++)
            if (!strcmp(argv[i], names[k])
                || (!strncmp(argv[i], names[k], strlen(names[k]))
                    && !strcmp(argv[i] + strlen(names[k]), "end")))
                phase = k;
        if ((!input && phase < 0 && strcmp(argv[i], "step") && strcmp(argv[i], "stepend"))
            || sscanf(argv[i + 1], "%u", &side) != 1
            || sscanf(argv[i + 2], "%x:%x", &ss, &bp) != 2
            || !load(argv[i + 3], entry) || !load(argv[i + 4], leave))
            return 2;
        memset(mem, 0, MEM_SIZE);
        memcpy(mem, entry, RAM);
        below = input ? 0x0A : 0x400;
        frame = lin((uint16_t)ss, (uint16_t)(bp - below));
        if (input) {
            got = computer_input((uint16_t)side);
            want = (uint16_t)(leave[frame + 2] | leave[frame + 3] << 8);
            printf("%s: side %u, input %04X, result %u", argv[i + 4], side,
                   NW(computer_out, side * 4), got);
        } else if (phase >= 0) {
            static const uint8_t state_at[2] = { 5, 3 };
            uint8_t state = NB(computer_phase, side * 0x0C + state_at[phase]);
            uint8_t inner = NB(computer_phase, side * 0x0C + 7);
            uint8_t ride = NB(computer_phase, side * 0x0C + 8);

            if (frw((uint16_t)ss, (uint16_t)(bp + 6)) != side) {
                fprintf(stderr, "%s: the frame's side is not %u\n", argv[i + 3], side);
                return 2;
            }
            got = phase == 0 ? computer_assess((uint16_t)side) : computer_queue((uint16_t)side);
            want = argv[i][strlen(names[phase])] ? 1 : 0;
            printf("%s: side %u, computer_%s state %u/%u/%u, result %u", argv[i + 4], side,
                   names[phase], state, inner, ride, got);
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
            if (mem[a] != entry[a] || (a >= lo_state && a < hi_state)) {
                if (diff++ < 8)
                    printf(" %05X", (unsigned)a);
            } else if (apart++ < 8)
                printf(" [%05X]", (unsigned)a);
        }
        if (got != want)
            printf("; the original's result %04X", want);
        printf("; different %u, only the original's elsewhere %u; bytes the port changed %u\n",
               diff, apart, changed);
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
