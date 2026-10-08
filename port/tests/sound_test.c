/* sound_test.c - the sound's routines (port/src/sound.c) against the
 * original.
 *
 *     sound_test (init|play|dig|pass|stop|tick|mstart|mfade|mvol|ftick) SS:SP AX ENTRY.ram EXIT.ram [...]
 *
 * Each set of five is one call in HL.EXE: which routine (snd_init,
 * sfx_play, dig_play,
 * snd_pass, chan_stop, adl_tick, music_start, music_fade, music_volume,
 * fade_tick), SS:SP at its entry, AX at its
 * end (hex), dosrun's -ram dump at the entry and the one at the routine's
 * end (port/tests/sound_dumps.py makes them and the argument list).  The
 * arguments are read from the stack of the first dump; after the port's
 * routine the sound's segment (S30EF, the driver's variables up to its
 * code) and what it returned are compared with the second.
 *
 * The timer's interrupt comes in the original while a routine runs:
 * snd_ticks then counts on in between.  The port's count is set to the
 * original's after the call, and a record's due tick (set from the count)
 * may be lower by as many ticks as came in; such calls are counted.
 * Outside S30EF only the count of differing bytes is told (the stack,
 * the timer's and the keyboard's own counts when an interrupt came in).
 *
 * init: the words the port leaves to the machine are listed, not judged
 * (see snd_init in sound.c); its result is judged.
 *
 * The music's four (port/tests/music_dumps.py makes their dumps): a tune
 * plays, so a tick that comes in also reads the tune's events.  When the
 * count of ticks differs after the port's call, the call is made again
 * with that many of the timer's interrupts (snd_irq) before it, and if
 * that is not the original's memory either, behind it; which was needed
 * is told.  music_fade waits for its fade while the timer runs: the port
 * runs the interrupts itself, and how many each took is told (the
 * original's count at its end has those that came in behind the wait
 * too, while it silences the voices: 20 registers written with
 * opl_write's waits, presumably what takes that long).
 * The number the timer gave the fade's routine (S30EF:1118, pit_add's;
 * the port does not have the timer's tables) is taken from the original
 * and not judged.  fade_tick has no result.  music_start's copy of the
 * count of ticks (S30EF:0232) may be lower by the ticks that came in.
 * port/tests/sound_test.sh builds this.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hl.h"

#define RAM 0xA0000u
#define DRV_END 0x111Fu     /* the driver's code begins here */

static uint8_t entry[RAM], leave[RAM];
static unsigned opl_n;

static void opl_count(uint8_t reg, uint8_t value)
{
    (void)reg; (void)value;
    opl_n++;
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

static uint32_t ticks(const uint8_t *m)
{
    uint32_t a = (uint32_t)S_snd_ticks * 16 + A_snd_ticks;

    return m[a] | m[a + 1] << 8 | (uint32_t)m[a + 2] << 16 | (uint32_t)m[a + 3] << 24;
}

/* 0232h is the low word, 0234h the high one */
static uint32_t rd4(const uint8_t *m)
{
    return m[0] | m[1] << 8 | (uint32_t)m[2] << 16 | (uint32_t)m[3] << 24;
}

/* the due tick of channel record: offset within S30EF of one of its bytes? */
static int is_due(unsigned off)
{
    return off >= 8 && off < 0x22A && (off - 8) % 0x2A >= 0x1E && (off - 8) % 0x2A < 0x22;
}

/* the call of set i from the entry's memory, `pre` interrupts of the
 * timer before it and `post` behind it */
static int16_t call(char **argv, int i, unsigned ss, unsigned sp, unsigned pre, unsigned post, int *has_r)
{
    uint16_t s = (uint16_t)ss, arg = (uint16_t)(sp + 4);    /* behind the far return address */
    int16_t r = 0;
    unsigned n;

    memset(mem, 0, MEM_SIZE);
    memcpy(mem, entry, RAM);
    opl_n = 0;
    *has_r = 1;
    for (n = 0; n < pre; n++)
        snd_irq();
    if (!strcmp(argv[i], "init"))
        r = (int16_t)snd_init(frw(s, arg));
    else if (!strcmp(argv[i], "play"))
        r = sfx_play(frw(s, arg), frw(s, (uint16_t)(arg + 2)), frw(s, (uint16_t)(arg + 4)),
                     frw(s, (uint16_t)(arg + 6)), frw(s, (uint16_t)(arg + 8)));
    else if (!strcmp(argv[i], "dig"))
        r = dig_play(frw(s, arg), far_at(s, (uint16_t)(arg + 2)), frw(s, (uint16_t)(arg + 6)),
                     frw(s, (uint16_t)(arg + 8)), frw(s, (uint16_t)(arg + 10)));
    else if (!strcmp(argv[i], "stop"))
        r = chan_stop(frw(s, arg));
    else if (!strcmp(argv[i], "mstart"))
        r = music_start(far_at(s, arg), frw(s, (uint16_t)(arg + 4)), frw(s, (uint16_t)(arg + 6)));
    else if (!strcmp(argv[i], "mfade"))
        r = music_fade(frw(s, arg));
    else if (!strcmp(argv[i], "mvol"))
        r = music_volume(frw(s, arg));
    else if (!strcmp(argv[i], "pass")) {
        snd_pass();
        *has_r = 0;
    } else if (!strcmp(argv[i], "tick")) {
        adl_tick();
        *has_r = 0;
    } else if (!strcmp(argv[i], "ftick")) {
        fade_tick();
        *has_r = 0;
    } else {
        fprintf(stderr, "%s: not a routine of this test\n", argv[i]);
        exit(2);
    }
    for (n = 0; n < post; n++)
        snd_irq();
    return r;
}

/* the driver's bytes that differ from the end's dump, without the due
 * ticks when `k` ticks came in (counted in *due) */
static unsigned drv_diff(uint32_t drv, uint32_t k, unsigned *due, int show)
{
    unsigned off, n = 0;

    *due = 0;
    for (off = 0; off < DRV_END; off++) {
        if (mem[drv + off] == leave[drv + off])
            continue;
        if (k && is_due(off)) {
            ++*due;
            continue;
        }
        /* music_start's copy of the count of ticks, taken after a tick
         * came in */
        if (k && off >= 0x232 && off < 0x236 && rd4(leave + drv + 0x232) - rd4(mem + drv + 0x232) <= k)
            continue;
        n++;
        if (show < 0 || (int)n <= show)
            printf("  S30EF:%04X original %02X port %02X\n", off, leave[drv + off], mem[drv + off]);
    }
    return n;
}

int main(int argc, char **argv)
{
    int i, bad = 0, calls = 0, between = 0;

    if (argc < 6 || (argc - 1) % 5) {
        fprintf(stderr, "usage: sound_test (init|play|dig|pass|stop|tick|mstart|mfade|mvol|ftick) SS:SP AX ENTRY.ram EXIT.ram ...\n");
        return 2;
    }
    snd_opl = opl_count;
    for (i = 1; i < argc; i += 5) {
        unsigned ss, sp, ax, d_drv = 0, d_else = 0, d_stack = 0, d_due = 0, pre = 0, post = 0;
        uint32_t a, drv = (uint32_t)(HL_S30EF + HL_BASE) * 16, k, t, t0, waited = 0;
        uint16_t arg, s;
        int16_t r = 0;
        int has_r = 1, init = !strcmp(argv[i], "init"), music = argv[i][0] == 'm' || !strcmp(argv[i], "ftick");

        if (sscanf(argv[i + 1], "%x:%x", &ss, &sp) != 2 || sscanf(argv[i + 2], "%x", &ax) != 1
            || !load(argv[i + 3], entry) || !load(argv[i + 4], leave))
            return 2;
        s = (uint16_t)ss;
        arg = (uint16_t)(sp + 4);
        t0 = ticks(entry);
        r = call(argv, i, ss, sp, 0, 0, &has_r);
        waited = ticks(mem) - t0;
        calls++;
        /* the ticks that came in while the original's routine ran */
        k = ticks(leave) - ticks(mem);
        if (music && k && k <= 8) {
            /* a tune plays: the interrupts before the call, or behind it
             * (music_fade: behind it first) */
            int fade = !strcmp(argv[i], "mfade");

            r = call(argv, i, ss, sp, fade ? 0 : k, fade ? k : 0, &has_r);
            if (!drv_diff(drv, 0, &d_due, 0))
                *(fade ? &post : &pre) = k;
            else {
                r = call(argv, i, ss, sp, fade ? k : 0, fade ? 0 : k, &has_r);
                if (!drv_diff(drv, 0, &d_due, 0))
                    *(fade ? &pre : &post) = k;
                else
                    r = call(argv, i, ss, sp, 0, 0, &has_r);
            }
            if (pre || post)
                k = ticks(leave) - ticks(mem);
            between++;
        }
        if (k) {
            t = ticks(leave);
            SETW(snd_ticks, 0, (uint16_t)t);
            SETW(snd_ticks, 2, (uint16_t)(t >> 16));
            if (!music)
                between++;
        }
        if (music) {
            /* the fade's number with the timer */
            mem[drv + 0x1118] = leave[drv + 0x1118];
            mem[drv + 0x1119] = leave[drv + 0x1119];
        }
        d_drv = drv_diff(drv, k, &d_due, init ? -1 : 12);
        for (a = 0; a < RAM; a++) {
            if (mem[a] == leave[a] || (a >= drv && a < drv + DRV_END))
                continue;
            if (a >= (uint32_t)ss * 16 + sp - 0x200 && a < (uint32_t)ss * 16 + sp)
                d_stack++;
            else
                d_else++;
        }
        /* a due tick that differs: by no more than the ticks in between */
        if (d_due) {
            unsigned ch;

            for (ch = 8; ch <= 12; ch++) {
                uint32_t at = drv + 8 + ch * 0x2A + 0x1E;
                uint32_t o = leave[at] | leave[at + 1] << 8 | (uint32_t)leave[at + 2] << 16 | (uint32_t)leave[at + 3] << 24;
                uint32_t p = mem[at] | mem[at + 1] << 8 | (uint32_t)mem[at + 2] << 16 | (uint32_t)mem[at + 3] << 24;

                if (o - p > k) {
                    d_drv++;
                    printf("  channel %u due at %lu, in the port at %lu\n", ch, (unsigned long)o, (unsigned long)p);
                }
            }
        }
        printf("%s %s: ", argv[i], argv[i + 3]);
        if (!strcmp(argv[i], "play"))
            printf("effect %u on channel %u, ", frw(s, (uint16_t)(arg + 2)), frw(s, arg));
        if (!strcmp(argv[i], "mstart"))
            printf("fade %u, volume %u, ", frw(s, (uint16_t)(arg + 4)), frw(s, (uint16_t)(arg + 6)));
        if (!strcmp(argv[i], "mfade"))
            printf("speed %u, waited %lu ticks (original %lu), ", frw(s, arg), (unsigned long)waited,
                   (unsigned long)(ticks(leave) - t0));
        if (!strcmp(argv[i], "mvol"))
            printf("volume %u, ", frw(s, arg));
        if (has_r)
            printf("returned %04X (original %04X), ", (uint16_t)r, ax);
        printf("%u registers written, S30EF %u different, elsewhere %u, stack %u%s", opl_n, d_drv, d_else, d_stack,
               k ? ", ticks came in" : "");
        if (pre || post)
            printf(", %u ticks %s the call", pre + post, pre ? "before" : "behind");
        printf("\n");
        if ((!init && d_drv) || (has_r && (uint16_t)r != ax))
            bad++;
    }
    printf("%d calls, %d different; in %d the timer came in between\n", calls, bad, between);
    return bad != 0;
}
