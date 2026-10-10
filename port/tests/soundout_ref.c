/* soundout_ref.c - the WAVE file the port's program rendered (DK_WAV, on
 * plat_null.c's virtual audio device) against the list of what it queued
 * (-oplat): run by soundout_test.sh.
 *
 *     soundout_ref RUN.at RUN.wav
 *
 * Checked:
 * - the interrupts' samples: the first one's, then each at the timer's
 *   periods summed since (PIT counts * rate / 1193182, the whole sum
 *   divided, so the fraction is carried), none twice;
 * - a write an interrupt made is queued for that interrupt's sample plus
 *   the lead; any other not before the lead (told: how many stamps go
 *   back, which the queue takes as the stamp before);
 * - the file: 16 bits, two channels, the list's rate, both channels the
 *   same, and every frame the one of a second chip (doskit/runtime/opl.c)
 *   that got each write exactly at its sample, rendered from write to
 *   write (other pieces than the device's 1024);
 * - silence up to the sample of the first write that sets a voice's key
 *   on, sound within ONSET samples from there.
 * Says "soundout ok" with the numbers, or the first thing that differs.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "opl.h"

#define PIT_HZ 1193182u
#define ONSET 4410

static int fail(const char *what, long long a, long long b)
{
    printf("soundout FAILED: %s: %lld, %lld\n", what, a, b);
    return 1;
}

static unsigned le(const unsigned char *p, int n)
{
    unsigned v = 0;

    while (n-- > 0)
        v = v << 8 | p[n];
    return v;
}

typedef struct {
    unsigned long long at;
    int reg, v, irq;
} Write;

int main(int argc, char **argv)
{
    static OPL chip;
    static int16_t piece[4096];
    FILE *f;
    unsigned char head[44], *data;
    char line[100];
    Write *w = NULL;
    long nw = 0, cap = 0, i, first_irq_write = 0, back = 0, behind = 0, irqs = 0, irq_writes = 0;
    long periods[8], period_n[8];
    int nperiods = 0, rate = 0, lead = -1, begun = 0;
    unsigned long long base = 0, pit = 0, at = 0, frames, eff = 0, key_on = 0, sound = 0;
    int have_key = 0;

    if (argc != 3) {
        fprintf(stderr, "usage: soundout_ref RUN.at RUN.wav\n");
        return 2;
    }
    f = fopen(argv[1], "r");
    if (!f || !fgets(line, sizeof line, f) || sscanf(line, "rate %d lead %d", &rate, &lead) != 2)
        return fail("the list's first line", 0, 0);
    while (fgets(line, sizeof line, f)) {
        unsigned long long s;
        unsigned a, b;
        int irq;
        unsigned long next;

        if (sscanf(line, "w %llu %x %x %d", &s, &a, &b, &irq) == 4) {
            if (nw == cap) {
                cap = cap ? cap * 2 : 4096;
                w = realloc(w, (size_t)cap * sizeof *w);
                if (!w)
                    return fail("memory", 0, 0);
            }
            w[nw].at = s;
            w[nw].reg = (int)a;
            w[nw].v = (int)b;
            w[nw].irq = irq;
            if (s < (unsigned long long)lead)
                return fail("a write before the lead", nw, (long long)s);
            nw++;
        } else if (sscanf(line, "i %llu %lu", &s, &next) == 2) {
            int p;

            /* the interrupt's sample by the periods */
            if (!begun) {
                begun = 1;
                base = s;
            } else if (s != base + pit * (unsigned long long)rate / PIT_HZ)
                return fail("an interrupt's sample is not the periods' sum", irqs,
                            (long long)s - (long long)(base + pit * (unsigned long long)rate / PIT_HZ));
            pit += next;
            for (p = 0; p < nperiods && periods[p] != (long)next; p++)
                ;
            if (p == nperiods && nperiods < 8) {
                periods[nperiods] = (long)next;
                period_n[nperiods++] = 0;
            }
            if (p < 8)
                period_n[p]++;
            /* what it wrote: the writes since the interrupt before */
            for (i = first_irq_write; i < nw; i++)
                if (w[i].irq) {
                    if (w[i].at != s + (unsigned long long)lead)
                        return fail("an interrupt's write is not at its sample plus the lead", i,
                                    (long long)w[i].at - (long long)s);
                    irq_writes++;
                }
            first_irq_write = nw;
            irqs++;
        } else if (line[0] != 'c')     /* the clock's lines are soundout_cmp.py's */
            return fail("a line of the list", nw, irqs);
    }
    fclose(f);
    for (i = first_irq_write; i < nw; i++)
        if (w[i].irq)
            return fail("an interrupt's write without its interrupt", i, 0);

    f = fopen(argv[2], "rb");
    if (!f || fread(head, 1, 44, f) != 44 || memcmp(head, "RIFF", 4) || memcmp(head + 8, "WAVEfmt ", 8)
        || memcmp(head + 36, "data", 4))
        return fail("the WAVE file's header", 0, 0);
    if (le(head + 20, 2) != 1 || le(head + 22, 2) != 2 || le(head + 34, 2) != 16 || (int)le(head + 24, 4) != rate)
        return fail("the WAVE file's format", le(head + 22, 2), le(head + 24, 4));
    frames = le(head + 40, 4) / 4;
    data = malloc((size_t)frames * 4 + 1);
    if (!data || fread(data, 4, (size_t)frames, f) != frames || fgetc(f) != EOF)
        return fail("the WAVE file's length", (long long)frames, 0);
    fclose(f);

    /* the second chip, each write at its sample */
    opl_init(&chip, rate);
    for (i = 0; i <= nw; i++) {
        unsigned long long to = frames;

        if (i < nw) {
            if (w[i].at < eff)
                back++;                 /* the queue: at the stamp before */
            else
                eff = w[i].at;
            to = eff < frames ? eff : frames;
            if (eff >= frames)
                behind++;
        }
        while (at < to) {
            int n = to - at > 4096 ? 4096 : (int)(to - at), k;

            opl_render(&chip, piece, n);
            for (k = 0; k < n; k++) {
                const unsigned char *p = data + (at + (unsigned)k) * 4;
                int16_t l = (int16_t)le(p, 2), r = (int16_t)le(p + 2, 2);
                /* main.c's audio_fill: the chip's sample twice over */
                int c = 2 * piece[k];

                piece[k] = (int16_t)(c > 32767 ? 32767 : c < -32768 ? -32768 : c);

                if (l != r)
                    return fail("the two channels differ at a frame", (long long)at + k, l - r);
                if (l != piece[k])
                    return fail("a frame is not the chip's with the writes at their samples",
                                (long long)at + k, l - piece[k]);
                if (l && !sound)
                    sound = at + (unsigned)k + 1;
            }
            at += (unsigned)n;
        }
        if (i < nw && eff < frames) {
            if (!have_key && w[i].reg >= 0xB0 && w[i].reg <= 0xB8 && (w[i].v & 0x20)) {
                have_key = 1;
                key_on = eff;
            }
            opl_write(&chip, w[i].reg, (uint8_t)w[i].v);
        }
    }
    if (!have_key || !sound)
        return fail("no note or no sound", have_key, (long long)sound);
    sound--;
    if (sound < key_on)
        return fail("sound before the first note's sample", (long long)sound, (long long)key_on);
    if (sound >= key_on + ONSET)
        return fail("no sound behind the first note's sample", (long long)sound, (long long)key_on);

    printf("soundout ok: %llu frames at %d Hz, %ld writes (%ld by %ld interrupts, %ld stamps back, "
           "%ld behind the file's end), lead %d, the first interrupt at %llu, "
           "the first note at %llu, sound from %llu\n",
           frames, rate, nw, irq_writes, irqs, back, behind, lead, base, key_on, sound);
    printf("the timer's periods:");
    for (i = 0; i < nperiods; i++)
        printf(" %lXh %ld times", (unsigned long)periods[i], period_n[i]);
    printf("\n");
    return 0;
}
