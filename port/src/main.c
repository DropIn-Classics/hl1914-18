/* main.c - Battle Isle Historyline 1914-1918: a native compatibility implementation requiring an
 * installed copy of the original game.
 *
 *     history-line [-game DIR | -gog FILE|FOLDER|SETUP.exe] [-map N] [-ram FILE]
 *                  [-keys FILE] [-passes N] [-until T] [-start T] [-seed N]
 *                  [-clock FILE] [-seeds FILE] [-dump N FILE]... [-inputs FILE]
 *                  [-ends FILE] [-evdump PREFIX] [-keylog FILE] [-poke ADDR HEX]... [-briefdump DIR]
 *                  [-infodump DIR]
 *
 * DIR is the game's unpacked files: -game, else $HISTORY_LINE_GAME, else the first
 * folder `game` holding HL14-18.EXE beside the program, in the current
 * directory or in the data folder (sys_find_game).  When there is none,
 * the installed GOG release's image is unpacked into the data folder's
 * `game`, or, installed as a folder, that folder copied there (cdimage.h);
 * not installed, GOG's Windows installer (setup_*.exe) lying about is
 * unpacked instead (inno.h).  -gog names the image, the folder or the
 * installer instead of looking for it.  Found by itself, the release is
 * copied only when the player agrees, asked in the kit's dialog about the
 * game's files (launcher.h: "Copy the files" or "Quit"), which also shows
 * the copy's progress and says what to do when nothing was found; the
 * headless build shows it only in a run scripted with keys (DK_KEYS,
 * plat_null.c) and copies without asking otherwise.
 *
 * A release build (PORT_VERSION and PORT_UPDATE_URL defined) asks once
 * whether it may look for newer releases and shows one it found
 * (update.h, docs/RELEASE.md point 7).
 *
 * The game itself does not run yet.  The program loads HL.EXE from the
 * game's folder into its memory (rmem.h, where dosrun loads it), does
 * what the original does at its start and for a battle as far as that is
 * translated (battle.c: its block of memory, the files of map N, 0
 * without -map, and from them the battle's units and sites), says what
 * it found and waits for Esc.  -ram writes that memory to FILE as
 * dosrun's -ram does (for doskit/tools/memcmp.py) and ends the program.
 *
 * In the window, without -keys, -passes, -clock or -ram, the program is
 * played there at the original's pace with the player's keys and mouse
 * (play.c, play_live): from the menu on as game_loop goes round (menu,
 * battle, menu), or with -map N that battle alone; until the program
 * ends, the window is closed or something comes that is not translated,
 * and it says so on stderr.
 *
 * With -keys, -passes or -clock the program then plays the battle by
 * itself without drawing it (play.c): the passes of the original's loop,
 * the human's keys from the key script FILE (the runner's format, the
 * first pass at T seconds of its time, 0 without -start), the computer's
 * side by its own turn, until the battle is over, N passes are played or
 * something comes that is not translated; it says how far it came and
 * ends.  -seed is the BIOS's count of ticks at the setup, from which the
 * random numbers start (without it the time of day's); -clock runs the
 * passes by the times of a log of the original (port/tests/play_dumps.py)
 * instead of the port's own clock; -seeds keeps the port's own clock and
 * takes from such a log only what the fights' pictures did to the random
 * numbers; -until ends the play at T seconds of the own clock; -dump writes the memory as -ram does at
 * the beginning of pass N (the video memory when FILE ends in .vram), -inputs what each side's part read, -ends the
 * texts a battle's end showed each time it waited for a press, -evdump
 * the memory and video memory at those waits and at every tenth frame of
 * the pictures behind the loop (PREFIXeN, PREFIXfNNNN; play.c), -keylog
 * (in the window) the player's keys as a key script that -keys plays the
 * same (play.c, play_keylog), -poke
 * writes the bytes HEX at the linear address ADDR (hex) of the memory
 * image before the first pass (port/tests/play_pokes.py), -infodump the
 * memory and video memory of the field menu's information into DIR
 * (draw.c, port/tests/info_test.sh), -briefdump
 * the memory and video memory of the new units' screens into DIR
 * (briefing.c, port/tests/briefing_test.sh).  All of them are aids for
 * this work, not for players.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "cdimage.h"
#include "hl.h"
#include "inno.h"
#include "launcher.h"
#include "opl.h"
#include "oplq.h"
#include "platform.h"
#include "sys.h"
#include "textmode.h"
#include "update.h"
#include "vga.h"

#ifndef PORT_VERSION
#define PORT_VERSION ""
#endif
#ifndef PORT_UPDATE_URL
#define PORT_UPDATE_URL ""
#endif

static const GogRelease release = {
    /* GOG's folder name */
    "Battle Isle Historyline 1914-1918",
    /* the CD image's path in that folder ("": game.gog) */
    "",
    /* the Mac release's image, its path in /Applications ("": none) */
    "",
    /* GOG's product ID, the number in goggame-ID.info ("": not known) */
    "1207661063",
    /* on the CD: images of other games are passed over */
    "HL14-18.EXE",
};

/* what earlier versions wrote beside the program (sys_data_migrate) */
static const char *const old_files[] = { "game", NULL };

static uint8_t pixels[TM_WIDTH * TM_HEIGHT];
static uint32_t palette[256];
static void show(void);

/* the lines about newer releases, from row y: the question, once, or
 * the setting and a release found; key the scancode read (-1 none) */
static int updates(int y, int key)
{
    const uint8_t attr = TM_ATTR(TM_LIGHTGREY, TM_BLUE), hi = TM_ATTR(TM_YELLOW, TM_BLUE);
    char line[80];
    UpdateInfo u;
    int consent = update_consent();
    int failed = 0;

    if (!*PORT_VERSION || !*PORT_UPDATE_URL)
        return 0;
    if (consent < 0) {
        if (key == 0x15 || key == 0x31)                 /* Y, N */
            update_set_consent(key == 0x15);
        tm_text(3, y, "Look for new versions of this port on GitHub, once a day?  Y / N", hi);
        return 0;
    }
    if (key == 0x3C)                                    /* F2 */
        update_set_consent(!consent);
    update_start(PORT_VERSION, PORT_UPDATE_URL);
    snprintf(line, sizeof line, "F2: look for new versions: %s", update_consent() ? "on" : "off");
    tm_text(3, y, line, attr);
    if (update_poll(&u)) {
        if (key == 0x16) {                              /* U: explicit install choice */
#ifdef __APPLE__
            update_open(u.page);
#else
            int action;
            tm_text(3, y + 1, "Downloading and checking the update...", hi);
            show();
            action = update_install(&u);
            if (action == 1)
                return 1;
            failed = action == 0;
#endif
        }
#ifdef __APPLE__
        snprintf(line, sizeof line, "%s is out (this is %s).  U opens its page.", u.version,
                 PORT_VERSION);
#else
        if (!failed)
            snprintf(line, sizeof line, "%s is ready (this is %s).  U installs it; any key later.",
                     u.version, PORT_VERSION);
#endif
        if (failed)
            tm_text(3, y + 1, "Update failed; this version is still installed.", hi);
        else
            tm_text(3, y + 1, line, hi);
        snprintf(line, sizeof line, "%.74s", u.notes);
        line[strcspn(line, "\n")] = 0;                /* the notes' first line */
        tm_text(3, y + 2, line, attr);
    }
    return 0;
}

static void show(void)
{
    tm_render(pixels, palette);
    plat_present(pixels, TM_WIDTH, TM_HEIGHT, palette);
}

static const LauncherApp app = { "Battle Isle Historyline 1914-1918", "history-line", PORT_VERSION };

/* the game's files: found, or from the GOG release (its CD image
 * unpacked, its installed folder copied, or its Windows installer
 * unpacked) once the player agreed in the kit's dialog about the game's
 * files (launcher.h); 1 if there, 0 after saying why not.  Not asked when
 * -gog named the release; headless the dialog is shown only when keys are
 * scripted. */
static int get_game(const char *given, const char *gog, char *out, size_t n)
{
    char from[SYS_PATH], data[SYS_PATH], err[256];
    int dialog = plat_has_window() || getenv("DK_KEYS"), r;
    int (*progress)(void *, const char *, long, long) = dialog ? launcher_copy_progress : NULL;
    LauncherCopy copy = { &app, LAUNCHER_GAME, 0, 0 };

    if (sys_find_game(given, "HISTORY_LINE_GAME", "HL14-18.EXE", out, n))
        return 1;
    if (gog && !given)
        snprintf(from, sizeof from, "%s", gog);
    else if (given || (!gog_find(&release, from, sizeof from) &&
                       !gog_find_folder(&release, from, sizeof from) &&
                       !inno_find(&release, from, sizeof from))) {
        if (dialog)
            launcher_no_game(&app, NULL);
        else
            plat_message("The game's files were not found. This program needs an installed "
                         "copy of Battle Isle Historyline 1914-1918 (the GOG release), or -game with its folder.");
        return 0;
    }
    sys_data_dir(data, sizeof data);
    sys_join(out, n, data, "game");
    if (dialog && !gog && !launcher_offer_copy(&app, LAUNCHER_GAME, from, out))
        return 0;
    if (sys_is_dir(from))
        r = gog_copy(from, out, "HL14-18.EXE", progress, &copy, err, sizeof err);
    else if (inno_is_setup(from))
        r = inno_unpack(from, out, "HL14-18.EXE", progress, &copy, err, sizeof err);
    else
        r = cd_unpack(from, out, "HL14-18.EXE", progress, &copy, err, sizeof err);
    if (r == 0)
        return 1;
    if (copy.closed)
        return 0;
    if (dialog)
        launcher_copy_failed(&app, from, err);
    else
        plat_message(err);
    return 0;
}

/* what game_loop does with the mouse when the menu has returned
 * (S066A:0390 on): the mouse opened for the side's record the options'
 * byte 114h names, its pointer shown, the battle's speeds, word 46h of the
 * record behind DATA:00C2 FFFFh, and byte 130h of the options 0 */
static void menu_mouse(void)
{
    uint16_t data = (uint16_t)(HL_DATA + HL_BASE), opt = (uint16_t)(HL_S37FF + HL_BASE);
    Far ms;

    mouse_open(0, frw(data, frb(opt, 0x0114) > 0 ? 0x00B6 : 0x00A6), 0);
    mouse_show();
    mouse_speed(0x0A, 0x0A, 0x0A, 0x0A, 0xFA0, 0x28, 0x28);
    ms = far_at(data, 0x00C2);
    fww(ms.seg, (uint16_t)(ms.off + 0x46), 0xFFFF);
    fwb(opt, 0x0130, 0);
}

/* HL.EXE into the program's memory and the translated parts of its
 * start and of a battle's setup for map `map`; a line about the result
 * in `line`.  0 if it failed. */
static int start_game(const char *game, int map, uint32_t bios, char *line, size_t n)
{
    char exe[SYS_PATH], err[256];
    int r;

    if (!sys_find(game, HL_EXE, exe, sizeof exe)) {
        snprintf(line, n, "%s was not found.", HL_EXE);
        return 0;
    }
    if (rm_load_exe(exe, HL_SIZE, HL_SHA256, HL_PSP, HL_PARAS, err, sizeof err) != 0) {
        snprintf(line, n, "%.*s", (int)n - 1, err);
        return 0;
    }
    hl_game_dir = game;
    play_bios(bios);
    r = start_setup();
    key_install();      /* S066A:0344, after what start_setup translates */
    if (r == 0 && map < 0) {    /* the menu chooses the map (menu_game) */
        snprintf(line, n, "The program's start is done.");
        return 1;
    }
    if (r == 0)
        r = menu_result((uint16_t)map);
    if (r == 0) {
        /* as from the menu; the setup's 2.75 s are not played here, in
         * which the original's first input_mouse (S27FE:03D4) finds the
         * pointer the driver's reset left at 319 a direction away and puts
         * it to the middle (its direction cleared by the setup) */
        menu_mouse();
        mouse_to(0xA0, 0x64);
        r = battle_setup();
    }
    if (r == 0)
        r = battle_begin();
    if (r != 0) {
        snprintf(line, n, r == 1 ? "No memory for the game." : "A file of the game could not be loaded.");
        return 0;
    }
    snprintf(line, n, "Map %d: %u by %u fields, %u units.", map, NW(map_width, 0), NW(map_height, 0),
             NB(unit_total, 0));
    return 1;
}

/* the BIOS's count of ticks for the time of day */
static uint32_t bios_now(void)
{
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    if (!tm)
        return 0;
    return (uint32_t)((tm->tm_hour * 3600L + tm->tm_min * 60L + tm->tm_sec) * (1193182.0 / 65536.0));
}

#define DUMPS 512
static struct { unsigned long pass; const char *file; } dumps[DUMPS];
static int dumps_n;
static int nopics;      /* -nopics: the fights are not shown as pictures */

/* -sndlog FILE: each call of sfx_play, its time on the port's clock, the
 * channel, the effect's number, what it returned, the battle's count
 * of passes, the volume, how many times and the frame of the picture
 * behind the loop it came in, from 0 (event_frame, events.c; -1: not in
 * one; for the tests). */
static FILE *snd_log;

static void snd_logged(uint16_t ch, uint16_t n, uint16_t vol, uint16_t times, int16_t r)
{
    fprintf(snd_log, "%.3f %u %u %d %lu %u %u %d\n", play_now(), ch, n, r,
            (unsigned long)((uint32_t)NW(game_state, 0x15) << 16 | NW(game_state, 0x13)), vol, times, event_frame);
    fflush(snd_log);
}

/* The AdLib: what sound.c hands to snd_opl, a register and its value in
 * the original's order, goes to an OPL2 of the runtime (opl.h).
 *
 * With an audio device (plat_audio_start: the window's, or plat_null.c's
 * virtual one with DK_AUDIO=1 or DK_WAV=FILE) the chip is played: each
 * write goes into the runtime's queue (oplq.h) with the sample sound.c
 * says it belongs to (snd_stamp) plus AUDIO_LEAD, the device's callback
 * renders through the queue (audio_fill), and sound.c makes the timer's
 * interrupts by the samples played (snd_samples = audio_samples).  A
 * write the game's side makes is stamped with the samples played at that
 * moment when they are further.  The Sound Blaster's bytes are not
 * played.  Without a device, as before: the chip gets the writes at
 * once, nobody asks it for samples, and the interrupts run on the port's
 * clock of passes (the tests' way).
 *
 * The virtual device has no clock of its own (plat_null.c: time passes
 * in plat_sleep_ms only), and a scripted play (-keys and the like) does
 * not sleep: audio_samples then moves it on to the port's clock, in
 * whole milliseconds, each time snd_clock asks.  Where the port's clock
 * begins anew or goes back, the device's goes on from where it was.  A
 * live play (play_live) sleeps a picture at a time itself.
 *
 * -oplat FILE (with a device): a line "rate R lead L", then "w S REG
 * VALUE I" for each write (S the sample it is queued for, the lead in it;
 * hex; I 1 when an interrupt wrote it) and behind each interrupt "i S N"
 * (its sample without the lead, the timer's period to the next) and
 * "c T" where the virtual device's clock is set by the port's (T the
 * port's time at the device's sample 0), for port/tests/soundout_test.sh.  At the end a line to stderr with the
 * samples played and the queue's counts.
 *
 * -opllog FILE: each write, the time on the port's clock, the
 * driver's count of ticks, the register and the value (hex); and a line
 * "start" or "fade" with the time and the count at the end of each
 * music_start and music_fade (for port/tests/opl_cmp.py). */
static OPL adlib;
static FILE *opl_log;

#define AUDIO_RATE 44100
#define AUDIO_LEAD (2 * 14 * AUDIO_RATE / 1000)     /* two pictures */
static OPLQ adlib_q;
static int audio_on, audio_moved, audio_live;
static FILE *opl_at;
static long audio_writes;
static uint64_t audio_ms;       /* the virtual device's time, moved on here */
static double audio_off, audio_t;

static void audio_fill(int16_t *out, int frames, void *user)
{
    static int16_t mono[1024];
    int i, n;

    (void)user;
    for (; frames > 0; frames -= n, out += 2 * n) {
        n = frames > 1024 ? 1024 : frames;
        oplq_render(&adlib_q, mono, n);
        for (i = 0; i < n; i++)
            out[2 * i] = out[2 * i + 1] = mono[i];
    }
}

/* sound.c's snd_samples */
static uint64_t audio_samples(const double *t)
{
    uint64_t n;

    if (t && !plat_has_window() && !audio_live) {
        uint64_t to;

        if (!audio_moved || *t < audio_t) {
            audio_off = *t - (double)audio_ms / 1000;
            if (opl_at)
                fprintf(opl_at, "c %.6f\n", audio_off);
        }
        audio_moved = 1;
        audio_t = *t;
        to = (uint64_t)((*t - audio_off) * 1000);
        if (to > audio_ms) {
            plat_sleep_ms((int)(to - audio_ms));
            audio_ms = to;
        }
    }
    plat_audio_lock();
    n = oplq_played(&adlib_q);
    plat_audio_unlock();
    return n;
}

static void audio_irq(uint64_t sample, uint32_t next)
{
    fprintf(opl_at, "i %llu %lu\n", (unsigned long long)sample, (unsigned long)next);
}

static void audio_start(void)
{
    if (!plat_audio_start(AUDIO_RATE, audio_fill, NULL))
        return;
    plat_audio_lock();
    opl_init(&adlib, AUDIO_RATE);
    oplq_init(&adlib_q, &adlib);
    plat_audio_unlock();
    audio_on = 1;
    snd_rate = AUDIO_RATE;
    snd_samples = audio_samples;
    if (opl_at) {
        fprintf(opl_at, "rate %d lead %d\n", AUDIO_RATE, AUDIO_LEAD);
        snd_irq_at = audio_irq;
    }
}

static void audio_end(void)
{
    if (!audio_on)
        return;
    plat_audio_lock();
    fprintf(stderr, "Audio: %llu samples played, %ld registers, %ld late, %ld early, %d waiting.\n",
            (unsigned long long)oplq_played(&adlib_q), audio_writes, adlib_q.late, adlib_q.early,
            oplq_pending(&adlib_q));
    plat_audio_unlock();
    if (opl_at)
        fclose(opl_at);
}

static void adlib_write(uint8_t reg, uint8_t value)
{
    if (audio_on) {
        uint64_t at = snd_stamp;

        plat_audio_lock();
        if (!snd_stamp_irq && oplq_played(&adlib_q) > at)
            at = oplq_played(&adlib_q);
        at += AUDIO_LEAD;
        oplq_write(&adlib_q, at, reg, value);
        plat_audio_unlock();
        audio_writes++;
        if (opl_at)
            fprintf(opl_at, "w %llu %02X %02X %d\n", (unsigned long long)at, reg, value, snd_stamp_irq);
    } else
        opl_write(&adlib, reg, value);
    if (opl_log)
        fprintf(opl_log, "%.6f %lu %02X %02X\n", play_now(),
                (unsigned long)((uint32_t)NW(snd_ticks, 2) << 16 | NW(snd_ticks, 0)), reg, value);
}

/* -muslog FILE: each call of music_start ("start", the fade's steps, the
 * volume) and music_fade ("fade", the speed) at its end: the time on the
 * port's clock, what it returned and the battle's count of passes (for
 * the test) */
static FILE *mus_log;

static void mus_logged(int out, uint16_t a, uint16_t b, int16_t r)
{
    if (opl_log)
        fprintf(opl_log, "%s %.6f %lu\n", out ? "fade" : "start", play_now(),
                (unsigned long)((uint32_t)NW(snd_ticks, 2) << 16 | NW(snd_ticks, 0)));
    if (!mus_log)
        return;
    fprintf(mus_log, "%.3f %s %u %u %d %lu\n", play_now(), out ? "fade" : "start", a, b, r,
            (unsigned long)((uint32_t)NW(game_state, 0x15) << 16 | NW(game_state, 0x13)));
    fflush(mus_log);
}

/* The fight's sounds (sound.c) from the places fightpics.c and orders.c
 * call their hooks at, before what play.c has set there; play_begin sets
 * the hooks anew, so this is done behind it. */
static void (*pics_kept)(int at);
static void (*orders_kept)(int at, int n);
/* -fxlog FILE: each of the fight's sound triggers (sound.c, snd_fight)
 * as port/tests/fsound_dumps.py writes the original's, and "fight N"
 * when the N-th fight's pictures are loaded */
static FILE *fx_log;
static int fx_fights;

static void fx_logged(const char *what, uint16_t a, uint16_t b, uint16_t c, uint16_t d)
{
    if (!strcmp(what, "play"))
        fprintf(fx_log, "play %u %u %u %u\n", a, b, c, d);
    else if (!strcmp(what, "side"))
        fprintf(fx_log, "side %u %u\n", a, b);
    else if (!strcmp(what, "end"))
        fprintf(fx_log, "end %u\n", a);
    else
        fprintf(fx_log, "%s\n", what);
    fflush(fx_log);
}

static void pics_sounded(int at)
{
    if (at == PICS_LOADED && fx_log) {
        fprintf(fx_log, "fight %d\n", ++fx_fights);
        fflush(fx_log);
    }
    fight_pics_sounds(at);
    if (pics_kept)
        pics_kept(at);
    else if (at == PICS_SEED)
        clock_seed_bios();
}

static void orders_sounded(int at, int n)
{
    fight_orders_sounds(at, n);
    if (orders_kept)
        orders_kept(at, n);
}

static void fight_sounds_wire(void)
{
    if (pics_hook != pics_sounded) {
        pics_kept = pics_hook;
        pics_hook = pics_sounded;
    }
    if (orders_hook != orders_sounded) {
        orders_kept = orders_hook;
        orders_hook = orders_sounded;
    }
}
static struct { unsigned long at; const char *hex; } pokes[16];
static int pokes_n;
static struct { unsigned long pass; const char *file; } menu_dumps[8];
static int menu_dumps_n;
static int dumps_ok;    /* play_game wrote its dumps */

/* -infodump DIR: the memory at the entry and the return of each call of
 * the field menu's information (unit_info, battle_info; draw.c) and the
 * video memory at the return, as infoN_entry.ram, infoN_end.ram and
 * infoN_end.vram in DIR, N counted from 1; kinds.txt gets a line a call,
 * `unit` or `battle` */
static const char *info_dir;

static void info_dump(int at, int which)
{
    static int n;
    char path[SYS_PATH], name[40];

    if (!at) {
        FILE *f;

        n++;
        sys_join(path, sizeof path, info_dir, "port_kinds.txt");
        f = fopen(path, n == 1 ? "w" : "a");
        if (f) {
            fprintf(f, "%s\n", which ? "battle" : "unit");
            fclose(f);
        }
    }
    snprintf(name, sizeof name, "port_info%d_%s.ram", n, at ? "end" : "entry");
    sys_join(path, sizeof path, info_dir, name);
    rm_write(path);
    if (at) {
        snprintf(name, sizeof name, "port_info%d_end.vram", n);
        sys_join(path, sizeof path, info_dir, name);
        vga_write_planes(path);
    }
}

/* -briefdump DIR: the memory at the entry of the new units' screens
 * (briefing.c) and at their return, and the video memory of each screen
 * lit and at the return, as files in DIR */
static const char *brief_dir;

static void brief_dump(int at, int n)
{
    char path[SYS_PATH], name[32];

    if (at == 1) {
        snprintf(name, sizeof name, "port_screen%d.vram", n);
        sys_join(path, sizeof path, brief_dir, name);
        vga_write_planes(path);
        return;
    }
    sys_join(path, sizeof path, brief_dir, at ? "port_end.ram" : "port_entry.ram");
    rm_write(path);
    if (at) {
        sys_join(path, sizeof path, brief_dir, "port_end.vram");
        vga_write_planes(path);
    }
}

/* a -dump's file: the memory, or the video memory for a name that ends
 * in .vram */
static int dump_write(const char *file)
{
    size_t n = strlen(file);

    if (n > 5 && !strcmp(file + n - 5, ".vram"))
        return vga_write_planes(file);
    return rm_write(file);
}

/* the battle played from the setup on (play.c), at most `passes` passes
 * (0: no limit but a million); a line about how far it came in `line`.
 * 0 if a dump could not be written. */
static int play_game(unsigned long passes, double start, double until, int clocked, char *line, size_t n)
{
    const char *why = NULL;
    int i, ok = 1;

    if (!passes)
        passes = 1000000;
    play_begin(start);
    fight_sounds_wire();
    dumps_ok = 1;
    if (nopics)
        SETB(no_fight_pics, 0, 1);
    for (i = 0; i < pokes_n; i++) {
        unsigned long a = pokes[i].at;
        const char *h = pokes[i].hex;
        unsigned v;

        for (; h[0] && h[1] && sscanf(h, "%2x", &v) == 1; h += 2, a++)
            fwb((uint16_t)(a >> 4), (uint16_t)(a & 15), (uint8_t)v);
    }
    for (;;) {
        play_sync();
        for (i = 0; i < dumps_n; i++)
            if (dumps[i].pass == play_passes + 1 && dump_write(dumps[i].file) != 0)
                ok = 0;
        if (play_passes >= passes || (until > 0 && play_now() >= until))
            break;
        if (!(NW(game_state, 0) & 2)) {
            /* what follows the loop; a dump of the pass after the one
             * after the last is the memory then (S066A:4728) */
            uint16_t r = play_after();

            for (i = 0; i < dumps_n; i++)
                if (dumps[i].pass == play_passes + 2 && dump_write(dumps[i].file) != 0)
                    ok = 0;
            why = r ? "the battle is over, and it was the last (7Bh)" : "the battle is over";
            break;
        }
        why = play_pass();
        if (why)
            break;
    }
    dumps_ok = ok;
    snprintf(line, n, "%lu passes played%s%s.", play_passes, why ? ", then: " : "", why ? why : "");
    if (clocked)    /* with -seeds: fights the log had no seed for */
        snprintf(line + strlen(line), n - strlen(line), " %lu places where the clock's file did not fit.",
                 play_misses);
    return ok;
}

/* The program from the menu on, as game_loop goes round from S066A:0349:
 * the menu (menu.c) with the key script's keys on the port's own clock
 * from `start` seconds on, what game_loop does when it returns
 * (S066A:0390), the battle's setup and the battle (play_game), what
 * follows it, and the menu again; until the menu ends the program, the
 * key script has nothing left for it, `until` is reached or a battle
 * stops before its end.  With a clock's file (`clocked`) the battles are
 * played on the original's clock, the menu still on the port's own.
 * A -menudump N FILE is the memory when the menu
 * has returned the N-th time.  A line about how far it came in `line`;
 * 0 if something failed. */
static int menu_game(unsigned long passes, double start, double until, int clocked, char *line, size_t n)
{
    uint16_t data = (uint16_t)(HL_DATA + HL_BASE);
    int menus = 0, battles = 0, i, r, ok = 1;
    const char *why = NULL;
    char sub[200];

    play_start(start);
    sub[0] = 0;
    for (;;) {
        SETW(game_state, 0, (NW(game_state, 0) | 1) & 0xFFEF);
        inputs_release(0);
        mouse_close();
        r = menu_run(game_mem.block);
        if (r != 0) {
            snprintf(line, n, "A file of the menu could not be loaded.");
            return 0;
        }
        if (menu_stopped) {
            why = play_stopped() ? play_stopped() : menu_stopped;
            break;
        }
        menus++;
        for (i = 0; i < menu_dumps_n; i++)
            if (menu_dumps[i].pass == (unsigned long)menus && rm_write(menu_dumps[i].file) != 0)
                ok = 0;
        menu_mouse();
        if (NW(game_state, 0) & 0x100) {
            /* S066A:4739, the program's end: the mouse's pointer taken
             * off, the keyboard's interrupt given back, the blocks of
             * mem_alloc freed (their count at DATA:0467).  Left out, the
             * machine's part: the screen's mode from before the program
             * (S2B7F:000E), the sound's end (S2F39:0092), interrupt 24h
             * and the timer given back (input_remove) */
            uint16_t k, blocks = frw(data, 0x0467);

            mouse_hide();
            key_remove();
            for (k = 0; k < blocks; k++)
                dos_free(frw(data, (uint16_t)(0x0469 + 2 * k)));
            fww(data, 0x0467, 0);
            why = "the menu ended the program";
            break;
        }
        if (until > 0 && play_now() >= until) {
            why = "the time given is over";
            break;
        }
        play_setup();
        r = battle_setup();
        if (r == 0)
            r = battle_begin();
        if (r != 0) {
            snprintf(line, n, r == 1 ? "No memory for the game." : "A file of the game could not be loaded.");
            return 0;
        }
        battles++;
        fprintf(stderr, "Menu %d left at %.2f s for map %u.\n", menus, play_now(), NW(map_number, 0));
        play_game(passes, -1, until, clocked, sub, sizeof sub);
        ok = ok && dumps_ok;
        if (NW(game_state, 0) & 2) {
            why = "the battle did not end";
            break;
        }
        if (!(NW(game_state, 0) & 1)) {
            why = "the last battle is won";
            break;
        }
    }
    snprintf(line, n, "%d times through the menu, %d battles, to %.2f s; then: %s.%s%s", menus, battles, play_now(),
             why, sub[0] ? " The last battle: " : "", sub);
    return ok;
}

int main(int argc, char **argv)
{
    const char *given = NULL, *gog = NULL, *ram = NULL, *keys = NULL, *clock_file = NULL, *why = NULL;
    const char *seeds_file = NULL;
    char game[SYS_PATH], line[300];
    int i, map = -1, ok, play = 0, seeded = 0;
    unsigned long passes = 0, seed = 0;
    double start = 0, until = 0;

    opl_init(&adlib, 49716);    /* the chip's own rate until something plays it */
    snd_opl = adlib_write;
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-game") && i + 1 < argc)
            given = argv[++i];
        else if (!strcmp(argv[i], "-gog") && i + 1 < argc)
            gog = argv[++i];
        else if (!strcmp(argv[i], "-map") && i + 1 < argc)
            map = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-ram") && i + 1 < argc)
            ram = argv[++i];
        else if (!strcmp(argv[i], "-keys") && i + 1 < argc)
            keys = argv[++i], play = 1;
        else if (!strcmp(argv[i], "-clock") && i + 1 < argc)
            clock_file = argv[++i], play = 1;
        else if (!strcmp(argv[i], "-passes") && i + 1 < argc)
            passes = strtoul(argv[++i], NULL, 0), play = 1;
        else if (!strcmp(argv[i], "-seeds") && i + 1 < argc)
            seeds_file = argv[++i];
        else if (!strcmp(argv[i], "-until") && i + 1 < argc)
            until = atof(argv[++i]), play = 1;
        else if (!strcmp(argv[i], "-start") && i + 1 < argc)
            start = atof(argv[++i]);
        else if (!strcmp(argv[i], "-seed") && i + 1 < argc)
            seed = strtoul(argv[++i], NULL, 0), seeded = 1;
        else if (!strcmp(argv[i], "-inputs") && i + 1 < argc)
            play_inputs(argv[++i]);
        else if (!strcmp(argv[i], "-keylog") && i + 1 < argc)
            play_keylog(argv[++i]);
        else if (!strcmp(argv[i], "-micelog") && i + 1 < argc)
            play_micelog(argv[++i]);
        else if (!strcmp(argv[i], "-mice") && i + 1 < argc) {
            const char *why = play_mice(argv[++i]);

            if (why) {
                fprintf(stderr, "%s\n", why);
                return 1;
            }
        }
        else if (!strcmp(argv[i], "-rounds") && i + 1 < argc)
            play_rounds(argv[++i]);
        else if (!strcmp(argv[i], "-ends") && i + 1 < argc)
            play_ends(argv[++i]);
        else if (!strcmp(argv[i], "-evdump") && i + 1 < argc)
            play_evdump(argv[++i]);
        else if (!strcmp(argv[i], "-nopics"))
            nopics = 1;
        else if (!strcmp(argv[i], "-saves") && i + 1 < argc)
            hl_save_dir = argv[++i];
        else if (!strcmp(argv[i], "-infodump") && i + 1 < argc) {
            info_dir = argv[++i];
            info_hook = info_dump;
        }
        else if (!strcmp(argv[i], "-briefdump") && i + 1 < argc) {
            brief_dir = argv[++i];
            briefing_hook = brief_dump;
        }
        else if (!strcmp(argv[i], "-menulog") && i + 1 < argc)
            menu_log_to(argv[++i]);
        else if (!strcmp(argv[i], "-sndlog") && i + 1 < argc) {
            snd_log = fopen(argv[++i], "w");
            if (snd_log)
                snd_played = snd_logged;
        }
        else if (!strcmp(argv[i], "-fxlog") && i + 1 < argc) {
            fx_log = fopen(argv[++i], "w");
            if (fx_log)
                snd_fight = fx_logged;
        }
        else if (!strcmp(argv[i], "-opllog") && i + 1 < argc) {
            opl_log = fopen(argv[++i], "w");
            if (opl_log)
                snd_music = mus_logged;
        }
        else if (!strcmp(argv[i], "-oplat") && i + 1 < argc)
            opl_at = fopen(argv[++i], "w");
        else if (!strcmp(argv[i], "-muslog") && i + 1 < argc) {
            mus_log = fopen(argv[++i], "w");
            if (mus_log)
                snd_music = mus_logged;
        }
        else if (!strcmp(argv[i], "-menuvram") && i + 2 < argc) {
            double at = atof(argv[++i]);

            menu_shot(at, argv[++i]);
        }
        else if (!strcmp(argv[i], "-askvram") && i + 2 < argc) {
            int k = atoi(argv[++i]);

            menu_ask_shot(k, argv[++i]);
        }
        else if (!strcmp(argv[i], "-menudump") && i + 2 < argc && menu_dumps_n < 8) {
            menu_dumps[menu_dumps_n].pass = strtoul(argv[++i], NULL, 0);
            menu_dumps[menu_dumps_n++].file = argv[++i];
        }
        else if (!strcmp(argv[i], "-poke") && i + 2 < argc && pokes_n < 16) {
            pokes[pokes_n].at = strtoul(argv[++i], NULL, 16);
            pokes[pokes_n++].hex = argv[++i];
        }
        else if (!strcmp(argv[i], "-dump") && i + 2 < argc && dumps_n < DUMPS) {
            dumps[dumps_n].pass = strtoul(argv[++i], NULL, 0);
            dumps[dumps_n++].file = argv[++i];
        } else {
            fprintf(stderr, "usage: history-line [-game DIR | -gog FILE|FOLDER] [-map N] [-ram FILE]\n"
                            "       [-keys FILE] [-passes N] [-until T] [-start T] [-seed N] [-clock FILE] [-seeds FILE]\n"
                            "       [-nopics] [-rounds FILE] [-inputs FILE] [-ends FILE] [-keylog FILE] [-poke ADDR HEX]... [-dump N FILE]...\n"
                            "       [-oplat FILE] [-saves DIR] [-menulog FILE] [-menudump N FILE]... [-menuvram T FILE]... [-askvram N FILE]...\n"
                            "With -keys and no -map the program begins at the menu (at -start seconds).\n");
            return 2;
        }
    }
    if (keys)
        why = play_keys(keys);
    if (!why && clock_file)
        why = play_clock(clock_file);
    if (!why && seeds_file)
        why = play_seeds(seeds_file);
    if (why) {
        fprintf(stderr, "%s\n", why);
        return 2;
    }
    sys_set_app("Battle Isle Historyline 1914-1918", "history-line");
    sys_data_migrate(old_files);
    if (!plat_init("Battle Isle Historyline 1914-1918"))
        return 1;
    if (!get_game(given, gog, game, sizeof game)) {
        plat_shutdown();
        return 1;
    }
    audio_start();
    if (!hl_save_dir) {
        static char saves[SYS_PATH];

        sys_data_dir(saves, sizeof saves);
        hl_save_dir = saves;
    }
    /* in the window without -keys, -passes, -clock or -ram the program is
     * played there (play_live): from the menu on, or with -map the battle
     * alone; headless too when DK_KEYS or DK_SHOTS script it (plat_null.c) */
    if (!play && !ram && (plat_has_window() || getenv("DK_KEYS") || getenv("DK_SHOTS"))) {
        play_live();
        audio_live = 1;
        briefing_player = 1;
        play = 1;
    }
    if (!play && map < 0)
        map = 0;
    ok = start_game(game, map, seeded ? (uint32_t)seed : bios_now(), line, sizeof line);
    if (ok && play && map < 0)
        ok = menu_game(passes, start, until, clock_file != NULL || seeds_file != NULL, line, sizeof line);
    else if (ok && play) {
        fprintf(stderr, "%s\n", line);
        ok = play_game(passes, start, until, clock_file != NULL || seeds_file != NULL, line, sizeof line);
    }
    if (ram || play) {
        if (ok && ram && rm_write(ram) != 0)
            ok = 0;
        fprintf(stderr, "%s\n", line);
        audio_end();
        plat_shutdown();
        return !ok;
    }
    tm_clear(' ', TM_ATTR(TM_LIGHTGREY, TM_BLUE));
    tm_frame(1, 1, 78, 5, TM_ATTR(TM_WHITE, TM_BLUE));
    tm_text(3, 2, "Battle Isle Historyline 1914-1918", TM_ATTR(TM_YELLOW, TM_BLUE));
    tm_text(3, 3, "The game's files:", TM_ATTR(TM_LIGHTGREY, TM_BLUE));
    tm_text(3, 4, game, TM_ATTR(TM_WHITE, TM_BLUE));
    tm_text(3, 7, line, TM_ATTR(TM_WHITE, TM_BLUE));
    tm_text(3, 8, "The game itself does not run yet.  Esc ends the program.", TM_ATTR(TM_LIGHTGREY, TM_BLUE));
    while (plat_pump()) {
        int b, key = -1;
        while ((b = plat_read_scancode()) >= 0)
            if (!(b & 0x80))
                key = b;
        if (key == 0x01)
            break;
        tm_fill(0, 10, TM_WIDTH, 3, ' ', TM_ATTR(TM_LIGHTGREY, TM_BLUE));
        if (updates(10, key))
            break;
        show();
        plat_sleep_ms(15);
    }
    plat_shutdown();
    return 0;
}
