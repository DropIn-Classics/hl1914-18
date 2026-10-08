/* oplwav_ref.c - the two helpers of oplwav_test.sh that need the chip
 * (doskit/runtime/opl.c) or every sample of a WAVE file.
 *
 *     oplwav_ref fit ORIG.wav WALK.orig
 *
 * The runner's -oplwav file against the log of the same run
 * (oplwav_dumps.py): a second chip at the file's rate gets every logged
 * write at the sample of its logged moment, and every sample must be the
 * file's.  The log has the moment in whole microseconds, so a write
 * within half a microsecond of a sample's edge may belong to either of
 * two samples: both are tried over the next FIT_AHEAD samples (with
 * both ways again for each such write among them), and the one that
 * gives the file's is taken (the earlier when both do; told how many).
 * Says "fit ok" with the numbers or the first sample that differs.
 *
 *     oplwav_ref render HZ FRAMES LIST OUT.wav
 *
 * A chip at HZ samples a second that gets each write of LIST (a line
 * "FRAME REGISTER VALUE", the last two hex) at its frame: FRAMES frames,
 * mono.
 *
 *     oplwav_ref level FILE.wav WINDOWS
 *
 * For each line "FROM TO" of WINDOWS (frames; the first channel is
 * read): a line with the frames' root mean square.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "opl.h"

#define FIT_AHEAD 4096

static long rate, frames;
static int channels;
static int16_t *pcm;            /* the first channel */

static int wav_read(const char *path)
{
    FILE *f = fopen(path, "rb");
    unsigned char h[44];
    long i, bytes;
    int16_t *all;

    if (!f || fread(h, 1, 44, f) != 44 || memcmp(h, "RIFF", 4) || memcmp(h + 36, "data", 4)
        || h[34] != 16) {
        fprintf(stderr, "%s: not a 16-bit WAVE file with a 44-byte header\n", path);
        return 0;
    }
    channels = h[22];
    rate = (long)h[24] | (long)h[25] << 8 | (long)h[26] << 16 | (long)h[27] << 24;
    fseek(f, 0, SEEK_END);
    bytes = ftell(f) - 44;
    fseek(f, 44, SEEK_SET);
    frames = bytes / (2 * channels);
    all = malloc((size_t)bytes + 2);
    pcm = malloc((size_t)frames * 2 + 2);
    if (!all || !pcm || fread(all, 2, (size_t)(frames * channels), f) != (size_t)(frames * channels))
        return 0;
    for (i = 0; i < frames; i++)
        pcm[i] = all[i * channels];
    free(all);
    fclose(f);
    return 1;
}

/* the chip's samples from `at` up to `to` against the file's: how many
 * differ (first: the first such, when asked) */
static long run(OPL *o, long at, long to, long *first)
{
    int16_t buf[512];
    long bad = 0;

    if (to > frames)
        to = frames;
    while (at < to) {
        int i, n = to - at > 512 ? 512 : (int)(to - at);

        opl_render(o, buf, n);
        for (i = 0; i < n; i++)
            if (buf[i] != pcm[at + i]) {
                if (!bad && first)
                    *first = at + i;
                bad++;
            }
        at += n;
    }
    return bad;
}

typedef struct { long long us; int reg, val; } Write;
static Write *w;
static long nw;

static long sample_of(long long tenths)
{
    return (long)(tenths * rate / 10000000);
}

/* whether the writes from k on, with the chip at sample `at`, can give
 * the file's samples up to `end` */
static int ahead(long k, OPL chip, long at, long end)
{
    for (; k < nw; k++) {
        long lo = sample_of(w[k].us * 10 - 5), hi = sample_of(w[k].us * 10 + 5);

        if (lo < at)
            lo = at;
        if (hi < lo)
            hi = lo;
        if (lo >= end)
            break;
        if (run(&chip, at, lo, NULL))
            return 0;
        at = lo;
        if (hi > lo && hi < end) {
            OPL b = chip;

            if (!run(&b, lo, hi, NULL)) {
                opl_write(&b, w[k].reg, (uint8_t)w[k].val);
                if (ahead(k + 1, b, hi, end))
                    return 1;
            }
        }
        opl_write(&chip, w[k].reg, (uint8_t)w[k].val);
    }
    return at >= end || !run(&chip, at, end, NULL);
}

static int fit(const char *wav, const char *list)
{
    FILE *f = fopen(list, "r");
    char line[128], kind[16];
    long cap = 0, k, at = 0, first = -1, bad = 0, either = 0, both = 0, later = 0, neither = 0;
    OPL chip;

    if (!f || !wav_read(wav)) {
        fprintf(stderr, "cannot read %s or %s\n", wav, list);
        return 1;
    }
    while (fgets(line, sizeof line, f)) {
        long long us;
        unsigned reg, val;

        if (sscanf(line, "%lld %15s %x %x", &us, kind, &reg, &val) != 4 || strcmp(kind, "w"))
            continue;
        if (nw == cap)
            w = realloc(w, (size_t)(cap = cap ? cap * 2 : 4096) * sizeof *w);
        w[nw].us = us;
        w[nw].reg = (int)reg;
        w[nw++].val = (int)val;
    }
    fclose(f);
    opl_init(&chip, rate);
    for (k = 0; k < nw; k++) {
        long lo = sample_of(w[k].us * 10 - 5), hi = sample_of(w[k].us * 10 + 5);

        if (lo < at)
            lo = at;
        if (hi < lo)
            hi = lo;
        if (lo >= frames)
            break;
        bad += run(&chip, at, lo, bad ? NULL : &first);
        at = lo;
        if (hi > lo && hi < frames) {
            OPL a = chip, b = chip;
            long end = lo + FIT_AHEAD > frames ? frames : lo + FIT_AHEAD;
            int ok_a, ok_b;

            either++;
            opl_write(&a, w[k].reg, (uint8_t)w[k].val);
            ok_a = ahead(k + 1, a, lo, end);
            ok_b = !run(&b, lo, hi, NULL);
            if (ok_b) {
                opl_write(&b, w[k].reg, (uint8_t)w[k].val);
                ok_b = ahead(k + 1, b, hi, end);
            }
            both += ok_a && ok_b;
            neither += !ok_a && !ok_b;
            if (ok_b && !ok_a) {
                later++;
                bad += run(&chip, lo, hi, bad ? NULL : &first);
                at = hi;
            }
        }
        opl_write(&chip, w[k].reg, (uint8_t)w[k].val);
    }
    bad += run(&chip, at, frames, bad ? NULL : &first);
    if (bad) {
        printf("fit FAILED: %ld of %ld samples are not the file's, the first %ld (%.6f s); %ld writes near a "
               "sample's edge, %ld fit neither way\n", bad, frames, first, (double)first / (double)rate, either,
               neither);
        return 1;
    }
    printf("fit ok: %ld samples at %ld Hz, every one the chip's with the %ld logged writes at their samples "
           "(%ld near a sample's edge: %ld of them belong to the later sample, %ld give the file either way)\n",
           frames, rate, nw, either, later, both);
    return 0;
}

/* the list's writes ("FRAME REG VALUE" a line, the frames not going
 * back) on a chip at `hz`, `n` frames, mono */
static int render(long hz, long n, const char *list, const char *out)
{
    FILE *f = fopen(list, "r"), *g = fopen(out, "wb");
    unsigned char h[44];
    int16_t buf[512];
    long at = 0, frame, bytes = n * 2;
    unsigned reg, val;
    int more;
    OPL chip;

    if (!f || !g) {
        fprintf(stderr, "cannot read %s or write %s\n", list, out);
        return 1;
    }
    memset(h, 0, sizeof h);
    memcpy(h, "RIFF", 4);
    memcpy(h + 8, "WAVEfmt ", 8);
    memcpy(h + 36, "data", 4);
    h[16] = 16; h[20] = 1; h[22] = 1; h[32] = 2; h[34] = 16;
#define PUT32(p, v) ((p)[0] = (unsigned char)(v), (p)[1] = (unsigned char)((v) >> 8), \
                     (p)[2] = (unsigned char)((v) >> 16), (p)[3] = (unsigned char)((v) >> 24))
    PUT32(h + 4, 36 + bytes); PUT32(h + 24, hz); PUT32(h + 28, hz * 2); PUT32(h + 40, bytes);
    fwrite(h, 1, 44, g);
    opl_init(&chip, hz);
    do {
        more = fscanf(f, "%ld %x %x", &frame, &reg, &val) == 3;
        if (!more || frame > n)
            frame = n;
        while (at < frame) {
            int k = frame - at > 512 ? 512 : (int)(frame - at);

            opl_render(&chip, buf, k);
            fwrite(buf, 2, (size_t)k, g);
            at += k;
        }
        if (more)
            opl_write(&chip, (int)reg, (uint8_t)val);
    } while (more && at < n);
    fclose(f);
    return fclose(g) != 0;
}

static int level(const char *wav, const char *windows)
{
    FILE *f = fopen(windows, "r");
    long from, to;

    if (!f || !wav_read(wav)) {
        fprintf(stderr, "cannot read %s or %s\n", wav, windows);
        return 1;
    }
    while (fscanf(f, "%ld %ld", &from, &to) == 2) {
        double sum = 0;
        long i;

        if (to > frames)
            to = frames;
        for (i = from; i < to; i++)
            sum += (double)pcm[i] * (double)pcm[i];
        printf("%.3f\n", to > from ? sqrt(sum / (double)(to - from)) : 0.0);
    }
    fclose(f);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc == 4 && !strcmp(argv[1], "fit"))
        return fit(argv[2], argv[3]);
    if (argc == 4 && !strcmp(argv[1], "level"))
        return level(argv[2], argv[3]);
    if (argc == 6 && !strcmp(argv[1], "render"))
        return render(atol(argv[2]), atol(argv[3]), argv[4], argv[5]);
    fprintf(stderr, "usage: oplwav_ref fit ORIG.wav WALK.orig | oplwav_ref level FILE.wav WINDOWS |\n"
                    "       oplwav_ref render HZ FRAMES LIST OUT.wav\n");
    return 2;
}
