/* opl_test.c - the port's AdLib output (port/src/sound.c: what goes to
 * snd_opl) against the original's stream of register writes.
 *
 *     opl_test [-v] WALK.ev
 *
 * WALK.ev is port/tests/opl_dumps.py's list of one run of HL.EXE on the
 * runner (its lines are described there): the calls of the sound's
 * routines by the game, the timer's interrupts with fade_tick and
 * adl_tick, and every register written, in the original's order, with
 * the memory at the entry of each call that has arguments and before
 * some ticks.
 *
 * The test makes the same calls and interrupts in that order on one
 * memory, from the first call's entry (snd_init) to the list's end: an
 * interrupt is one of the port's (snd_irq), which must then run the
 * routines the list has for it, fade_tick or adl_tick or both, so the
 * port's count of the timer's periods is compared too (and at each dump
 * with the counts in the original's timer table).  Each
 * register the port's routines hand to snd_opl must be the list's
 * next: the same register, the same value.  At each dump the driver's
 * variables (S30EF up to its code, and snd_started, snd_ended) are
 * compared with the port's; then the dump's memory is taken, because the
 * game loads files and tunes in between that the routines read (and so a
 * difference does not go on into what follows; every one is counted).
 * A call's result is compared with AX at its end.
 *
 * Where the interrupt came in while a routine of the game's side ran, the
 * list says between which two register writes.  The port's routine is
 * called once, so the test makes the interrupt from inside snd_opl: at
 * once behind the write before it when the lock is clear (or when the
 * tick found it set and only counted), else before the next write, or
 * behind the call when no write follows; one that came before the call's
 * first write is made before the call.  A tick that found the lock set
 * and came behind adl_start's reading of the count of ticks is made
 * behind that write, never before the reading.  Two places are marked
 * in the list and told by the port's routines (snd_at, sound.c), because
 * the side an interrupt came on changes what follows: where pit_add has
 * made fade_start's routine the timer's last (an interrupt before it does
 * not count for the fade's period) and music_start's copy of the count
 * of ticks.  music_fade's wait takes
 * its interrupts from the list (snd_wait, sound.c), one at a time while
 * the fade is on.  How many interrupts were placed each way is told.
 *
 * Not judged in the driver's variables, the machine's that the port does
 * not keep: see ignored() below.
 * port/tests/opl_test.sh builds this.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hl.h"

#define RAM 0xA0000u
#define DRV_END 0x111Fu     /* the driver's code begins here */
#define DRV ((uint32_t)(HL_S30EF + HL_BASE) * 16)

static uint8_t dump[RAM];
static char **ev;
static int n_ev, cur, verbose;
static int in_irq, in_call, eager_ok, started;
static const char *call_name = "";
static int call_line;

static unsigned long n_w, n_w_bad, n_w_missing, n_w_extra, n_calls, n_ret_bad, n_checks, n_check_bad, n_check_bytes;
static unsigned long n_irq, n_irq_before, n_irq_eager, n_irq_lazy, n_irq_behind, n_irq_wait, n_irq_at, n_lock_forced, n_tick_locked,
    n_wait_bad, n_loose;
static int shown, sfx_loaded;
static unsigned long n_timer, n_timer_fade, n_timer_bad;

static int is(int i, const char *w)
{
    size_t n = strlen(w);

    return i < n_ev && !strncmp(ev[i], w, n) && (ev[i][n] == ' ' || !ev[i][n]);
}

static void say(const char *what)
{
    if (shown++ < 40 || verbose)
        printf("  line %d (%s from line %d): %s\n", cur + 1, call_name, call_line, what);
}

/* the driver's bytes the port does not keep as the original: the timer's
 * interrupt's old vector and the counts delay_measure took by it
 * (10BF to 10C2, 111B to 111E), the numbers pit_add gave the two timer
 * routines (1116 to 1119), the Sound Blaster's interrupt's old vector
 * (0489 to 048C).  The place and the length of the effects' file (0242
 * to 0249) are sfx_load's, which is not called here: not judged at the
 * first dump behind a call of it. */
static int ignored(unsigned off)
{
    return (off >= 0x10BF && off < 0x10C3) || (off >= 0x111B && off < 0x111F) || (off >= 0x1116 && off < 0x111A)
        || (off >= 0x0489 && off < 0x048D);
}

static void check(const char *path)
{
    FILE *f = fopen(path, "rb");
    unsigned off, n = 0;
    uint32_t a;

    if (!f || fread(dump, 1, RAM, f) != RAM) {
        fprintf(stderr, "%s: not a dump of %u bytes\n", path, RAM);
        exit(2);
    }
    fclose(f);
    if (started) {
        char line[160];

        n_checks++;
        for (off = 0; off < DRV_END; off++)
            if (mem[DRV + off] != dump[DRV + off] && !ignored(off) && !(sfx_loaded && off >= 0x242 && off < 0x24A)) {
                if (n++ < 8 || verbose) {
                    snprintf(line, sizeof line, "S30EF:%04X original %02X port %02X", off, dump[DRV + off], mem[DRV + off]);
                    say(line);
                }
            }
        for (a = (uint32_t)S_snd_started * 16 + A_snd_started; a < (uint32_t)S_snd_started * 16 + A_snd_started + 4; a++)
            if (mem[a] != dump[a]) {
                n++;
                snprintf(line, sizeof line, "snd_started/snd_ended +%u original %02X port %02X",
                         (unsigned)(a - ((uint32_t)S_snd_started * 16 + A_snd_started)), dump[a], mem[a]);
                say(line);
            }
        if (n) {
            n_check_bad++;
            n_check_bytes += n;
            snprintf(line, sizeof line, "%u bytes of the driver differ at %s", n, path);
            say(line);
        }
    } else
        memset(mem, 0, MEM_SIZE);
    memcpy(mem, dump, RAM);
    started = 1;
    sfx_loaded = 0;
}

static void missing(void)
{
    while (is(cur, "w")) {
        char line[80];

        n_w_missing++;
        snprintf(line, sizeof line, "the original wrote %s, the port nothing", ev[cur] + 2);
        say(line);
        cur++;
    }
}

static int irq_at(int i) { return is(i, "irq") || is(i, "dirq"); }

/* the timer's counts for the sound's routines in the dump read last
 * (DATA:0E3F the period the timer ran with; a routine's number with the
 * timer, S30EF:1116 adl_tick's and 1118 the fade's, gives its place in
 * the table through DATA:0DBF, what is left of its period is at
 * DATA:0D5F and 0D7F there) against the port's (snd_timer, sound.c) */
static void timer_check(void)
{
    const uint8_t *d = dump + (uint32_t)(HL_DATA + HL_BASE) * 16, *drv = dump + DRV;
    int32_t adl, fade, o;
    uint32_t ran, o_ran = d[0xE3F] | d[0xE40] << 8 | (uint32_t)d[0xE41] << 16 | (uint32_t)d[0xE42] << 24;
    unsigned slot;
    char line[120];

    if (!drv[0x111A])
        return;
    snd_timer(&adl, &fade, &ran);
    n_timer++;
    slot = d[0xDBF + (drv[0x1116] | drv[0x1117] << 8)];
    o = (int32_t)(d[0xD5F + slot] | d[0xD60 + slot] << 8 | (uint32_t)d[0xD7F + slot] << 16 | (uint32_t)d[0xD80 + slot] << 24);
    if (o != adl || o_ran != ran) {
        n_timer_bad++;
        snprintf(line, sizeof line, "the timer: adl_tick's count %ld, the original's %ld; it ran with %lX, the original %lX",
                 (long)adl, (long)o, (unsigned long)ran, (unsigned long)o_ran);
        say(line);
    }
    if (drv[0x24C] & 1) {
        slot = d[0xDBF + (drv[0x1118] | drv[0x1119] << 8)];
        o = (int32_t)(d[0xD5F + slot] | d[0xD60 + slot] << 8 | (uint32_t)d[0xD7F + slot] << 16
                      | (uint32_t)d[0xD80 + slot] << 24);
        n_timer_fade++;
        if (o != fade) {
            n_timer_bad++;
            snprintf(line, sizeof line, "the timer: the fade's count %ld, the original's %ld", (long)fade, (long)o);
            say(line);
        }
    }
}

/* the interrupt at ev[cur] ("irq", or "dirq" the Sound Blaster's): one
 * of the port's (snd_irq), which must run the routines the list has */
static void irq(void)
{
    int want = 0, got, i, locked = 0, forced = 0, timer = 0;

    if (is(cur, "dirq")) {
        cur++;
        n_irq++;
        dig_irq();
        return;
    }
    cur++;
    n_irq++;
    for (i = cur; i < n_ev && !is(i, "iend"); i++) {
        if (is(i, "ftick"))
            want |= 2;
        else if (is(i, "tick"))
            want |= 1;
        else if (is(i, "lock"))
            want |= 1, locked = 1;
    }
    if (is(cur, "check")) {
        check(ev[cur++] + 6);
        timer = 1;
    }
    if (locked && !(NW(snd_lock, 0) & 1)) {
        /* the tick came where the port's routine does not hold the
         * lock: it only counts, as the original's did */
        SETW(snd_lock, 0, 1);
        forced = 1;
        n_lock_forced++;
    } else if (!locked && (want & 1) && (NW(snd_lock, 0) & 1)) {
        n_tick_locked++;
        say("the original's tick found the lock clear, the port's is set");
    }
    in_irq++;
    got = snd_irq();
    in_irq--;
    if (forced)
        SETW(snd_lock, 0, 0);
    while (cur < n_ev && !is(cur, "iend")) {
        if (is(cur, "w"))
            missing();
        else if (is(cur, "dirq")) {
            cur++;
            dig_irq();
        } else
            cur++;
    }
    cur++;
    if (got != want) {
        char line[100];

        n_timer_bad++;
        snprintf(line, sizeof line, "the port's interrupt ran %s%s, the original's %s%s", got & 2 ? "fade_tick " : "",
                 got & 1 ? "adl_tick" : "", want & 2 ? "fade_tick " : "", want & 1 ? "adl_tick" : "");
        say(line);
    }
    if (timer)
        timer_check();
}

/* may the interrupt at ev[i] be made now: a tick or a fade's step only
 * with the lock clear */
static int fits(int i)
{
    if (is(i, "dirq"))
        return 1;
    for (i++; i < n_ev && !is(i, "iend"); i++)
        if ((is(i, "tick") || is(i, "ftick")) && (NW(snd_lock, 0) & 1))
            return 0;
    return 1;
}

static void written(uint8_t reg, uint8_t value)
{
    unsigned r, v;
    char line[120];

    if (!in_irq)
        for (;;) {
            if (is(cur, "due") || is(cur, "at"))
                cur++;
            else if (irq_at(cur)) {
                n_irq_lazy++;
                irq();
            } else
                break;
        }
    else
        /* inside the interrupt: the list's lines that say which routine runs */
        for (;;) {
            if (is(cur, "ftick") || is(cur, "tick") || is(cur, "lock"))
                cur++;
            else if (is(cur, "dirq")) {
                cur++;
                dig_irq();
            } else
                break;
        }
    if (is(cur, "w") && sscanf(ev[cur] + 2, "%x %x", &r, &v) == 2) {
        n_w++;
        if (r != reg || v != value) {
            n_w_bad++;
            snprintf(line, sizeof line, "register %02X = %02X, the original's %02X = %02X", reg, value, r, v);
            say(line);
        }
        cur++;
    } else {
        n_w_extra++;
        snprintf(line, sizeof line, "register %02X = %02X, the original wrote none here (%s)", reg, value,
                 cur < n_ev ? ev[cur] : "the end");
        say(line);
    }
    if (!in_irq && eager_ok)
        while (irq_at(cur) && fits(cur)) {
            n_irq_eager++;
            irq();
        }
}

/* snd_at (sound.c): the port's routine is at a place the list marks
 * ("at N"): the interrupts the list has before the mark are made now */
static void at(int place)
{
    char mark[8];
    int i;

    snprintf(mark, sizeof mark, "at %d", place);
    for (i = cur; i < n_ev && (is(i, "due") || irq_at(i)); i++)
        if (is(i, "irq"))
            while (i < n_ev && !is(i, "iend"))
                i++;
    if (i >= n_ev || strcmp(ev[i], mark))
        return;
    while (strcmp(ev[cur], mark)) {
        if (is(cur, "due"))
            cur++;
        else {
            n_irq_at++;
            irq();
        }
    }
    cur++;
}

/* music_fade's wait: the list's next interrupt */
static int fade_wait(void)
{
    while (is(cur, "due") || is(cur, "at"))
        cur++;
    if (irq_at(cur)) {
        n_irq_wait++;
        irq();
    } else {
        n_wait_bad++;
        say("music_fade still waits where the original's wait was over");
        SETW(fade_on, 0, 0);
    }
    if (!NW(fade_on, 0))
        eager_ok = 1;
    return 1;
}

int main(int argc, char **argv)
{
    FILE *f;
    char buf[400];
    int cap = 0, a = 1;

    if (argc > 1 && !strcmp(argv[1], "-v"))
        verbose = 1, a = 2;
    if (argc != a + 1 || !(f = fopen(argv[a], "r"))) {
        fprintf(stderr, "usage: opl_test [-v] WALK.ev\n");
        return 2;
    }
    while (fgets(buf, sizeof buf, f)) {
        buf[strcspn(buf, "\r\n")] = 0;
        if (n_ev == cap)
            ev = realloc(ev, (size_t)(cap = cap ? cap * 2 : 4096) * sizeof *ev);
        ev[n_ev] = malloc(strlen(buf) + 1);
        strcpy(ev[n_ev++], buf);
    }
    fclose(f);
    snd_opl = written;
    snd_wait = fade_wait;
    snd_at = at;
    while (cur < n_ev) {
        static const char *const kinds[] = {"init", "play", "dig", "stop", "mstart", "mfade", "pass", "end"};
        unsigned ss = 0, sp = 0, ax = 0, k;
        uint16_t s, arg;
        int16_t r = 0;
        int has_r = 1, i;
        char path[320];

        if (is(cur, "check")) {
            check(ev[cur++] + 6);
            continue;
        }
        if (is(cur, "sfxload")) {
            sfx_loaded = 1;
            cur++;
            continue;
        }
        if (irq_at(cur)) {
            if (!started) {
                fprintf(stderr, "%s: an interrupt before the first call\n", argv[a]);
                return 2;
            }
            irq();
            continue;
        }
        for (k = 0; k < sizeof kinds / sizeof *kinds; k++)
            if (is(cur, kinds[k]))
                break;
        if (k == sizeof kinds / sizeof *kinds) {
            /* a write, an end or a reading outside the calls the list knows */
            n_loose++;
            call_name = "no call";
            say(ev[cur]);
            cur++;
            continue;
        }
        call_name = kinds[k];
        call_line = cur + 1;
        if (k < 6) {
            if (sscanf(ev[cur] + strlen(kinds[k]), "%x:%x %319s", &ss, &sp, path) != 3) {
                fprintf(stderr, "%s line %d: %s\n", argv[a], cur + 1, ev[cur]);
                return 2;
            }
            i = started;
            check(path);
            if (i)
                timer_check();
        } else if (!started) {
            fprintf(stderr, "%s: the first call has no memory\n", argv[a]);
            return 2;
        }
        cur++;
        n_calls++;
        s = (uint16_t)ss;
        arg = (uint16_t)(sp + 4);       /* behind the far return address */
        /* the interrupts that came before the call's first write (a
         * fade's wait takes them itself) */
        if (k != 5)
            while (irq_at(cur)) {
                n_irq_before++;
                irq();
            }
        in_call = 1;
        eager_ok = k != 5;
        switch (k) {
        case 0:
            r = (int16_t)snd_init(frw(s, arg));
            break;
        case 1:
            r = sfx_play(frw(s, arg), frw(s, (uint16_t)(arg + 2)), frw(s, (uint16_t)(arg + 4)),
                         frw(s, (uint16_t)(arg + 6)), frw(s, (uint16_t)(arg + 8)));
            break;
        case 2:
            r = dig_play(frw(s, arg), far_at(s, (uint16_t)(arg + 2)), frw(s, (uint16_t)(arg + 6)),
                         frw(s, (uint16_t)(arg + 8)), frw(s, (uint16_t)(arg + 10)));
            break;
        case 3:
            r = chan_stop(frw(s, arg));
            break;
        case 4:
            r = music_start(far_at(s, arg), frw(s, (uint16_t)(arg + 4)), frw(s, (uint16_t)(arg + 6)));
            break;
        case 5:
            r = music_fade(frw(s, arg));
            break;
        case 6:
            snd_pass();
            has_r = 0;
            break;
        default:
            snd_end();
            has_r = 0;
            break;
        }
        in_call = 0;
        /* what the list still has of the call: interrupts behind its last
         * write, writes the port did not make */
        for (;;) {
            if (is(cur, "due") || is(cur, "at"))
                cur++;
            else if (irq_at(cur)) {
                n_irq_behind++;
                irq();
            } else if (is(cur, "w"))
                missing();
            else
                break;
        }
        if (!is(cur, "ret") || sscanf(ev[cur] + 4, "%x", &ax) != 1) {
            say("the call's end is not where the list has it");
            continue;
        }
        if (has_r && (uint16_t)r != ax) {
            char line[80];

            n_ret_bad++;
            snprintf(line, sizeof line, "returned %04X, the original %04X", (uint16_t)r, ax);
            say(line);
        }
        cur++;
        call_name = "between calls";
    }
    printf("%lu registers written the same, %lu different, %lu the port did not write, %lu only the port wrote\n",
           n_w - n_w_bad, n_w_bad, n_w_missing, n_w_extra);
    printf("%lu calls, %lu results different; the driver's variables compared at %lu dumps, %lu different (%lu bytes)\n",
           n_calls, n_ret_bad, n_checks, n_check_bad, n_check_bytes);
    printf("%lu interrupts: %lu inside a call (%lu before it, %lu behind a write, %lu before the next write, "
           "%lu behind it, %lu in a fade's wait, %lu before a place the list marks)\n",
           n_irq, n_irq_before + n_irq_eager + n_irq_lazy + n_irq_behind + n_irq_wait + n_irq_at, n_irq_before, n_irq_eager,
           n_irq_lazy, n_irq_behind, n_irq_wait, n_irq_at);
    printf("%lu ticks counted only where the port's lock was clear, %lu ticks ran against the port's lock, "
           "%lu waits not as the original's, %lu lines outside a call\n",
           n_lock_forced, n_tick_locked, n_wait_bad, n_loose);
    printf("the timer's counts for adl_tick compared at %lu dumps, for the fade at %lu; %lu differences of the timer's\n",
           n_timer, n_timer_fade, n_timer_bad);
    return n_timer_bad || n_w_bad || n_w_missing || n_w_extra || n_ret_bad || n_check_bad || n_tick_locked || n_wait_bad || n_loose;
}
