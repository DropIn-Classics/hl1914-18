/* briefing.c - the new units' screens before a battle (S274D:0003), which
 * game_loop calls at S066A:09AB with the map's number, the free end and
 * the work buffer unless a saved game is to be loaded.  Translated over
 * the program's memory: its files go where the original's go (from the
 * free end on, which it does not move), its texts through the buffer of
 * 40h bytes it has on its stack.
 *
 * NEWUNIT.DAT has a word for each of the 24 maps of a country (a map from
 * 18h on is the map 18h below): 0, or where the map's list begins: a
 * month, a year, then words up to a 0: 1 or 2 is a side's screen (its
 * flag between NEW UNITS and the month and year), any other where a new
 * unit's record is: the kind, the colours' base of its side, and lines of
 * text up to an empty one.  Each screen is shown, lit in 33 steps, waits
 * for a key or a button (inputs_release) and goes dark again.
 *
 * Left out: the music (S2F39:0F87 and 1058; its file is loaded).  The
 * time is the port's own clock (play.c).  port/README.md says how this
 * was compared with the original.
 */
#include <stdio.h>
#include "hl.h"
#include "platform.h"

#define S_DATA (HL_DATA + HL_BASE)
#define S_GAME (HL_S331F + HL_BASE)
#define S_OPT (HL_S37FF + HL_BASE)
#define S_TEXT (HL_S3B5D + HL_BASE)     /* the routine's own DS: its names and texts */
/* the routine's buffer [BP-40h] on the program's stack: SS:SP is
 * 3EA4:0F8A at its entry in the runs (game_loop's frame is the same in
 * all of them) */
#define S_STACK (HL_STACK + HL_BASE)
#define A_BUF (0x0F88u - 0x40u)

void (*briefing_hook)(int at, int n);
int briefing_player;

static Far text(uint16_t off)
{
    Far p;

    p.seg = S_TEXT;
    p.off = off;
    return p;
}

static Far buf(void)
{
    Far p;

    p.seg = S_STACK;
    p.off = A_BUF;
    return p;
}

/* CODE:2BAA, the compiler's strcpy to the buffer */
static void buf_copy(Far from)
{
    uint16_t i = 0;

    do
        fwb(S_STACK, (uint16_t)(A_BUF + i), pb(from, i));
    while (pb(from, i++));
}

static uint16_t buf_put(uint16_t at, uint8_t c)
{
    fwb(S_STACK, (uint16_t)(A_BUF + at), c);
    return (uint16_t)(at + 1);
}

static uint16_t buf_num(uint16_t at, uint16_t v)
{
    uint8_t d[5];
    int n = 0;

    do
        d[n++] = (uint8_t)('0' + v % 10);
    while ((v /= 10) != 0);
    while (n > 0)
        at = buf_put(at, d[--n]);
    return at;
}

/* CODE:2B15, the compiler's sprintf to the buffer, as far as the
 * routine's formats need it: %s with a far text, %u and %d with a word */
static void buf_format(Far fmt, Far str, uint16_t a, uint16_t b)
{
    uint16_t i, at = 0, k, arg[2], n = 0;
    uint8_t c;

    arg[0] = a;
    arg[1] = b;
    for (i = 0; (c = pb(fmt, i)) != 0; i++) {
        if (c != '%') {
            at = buf_put(at, c);
            continue;
        }
        c = pb(fmt, ++i);
        if (c == 's') {
            for (k = 0; pb(str, k); k++)
                at = buf_put(at, pb(str, k));
        } else if (n < 2) {
            uint16_t v = arg[n++];

            if (c == 'd' && (int16_t)v < 0) {
                at = buf_put(at, '-');
                v = (uint16_t)-(int16_t)v;
            }
            at = buf_num(at, v);
        }
    }
    buf_put(at, 0);
}

/* CODE:2488, the compiler's itoa to the buffer with radix 10 */
static void buf_itoa(uint16_t v)
{
    buf_put(buf_num(0, v), 0);
}

static uint8_t kind_byte(uint16_t kind, uint16_t o)
{
    return NB(unit_dat, kind * KIND_SIZE + o);
}

/* a text of the unit's screen: text_draw with no gap, 6 lines a row */
static void put(uint16_t x, uint16_t y, uint8_t colour, Far t)
{
    text_draw(x, y, 0, 6, colour, t, NULL);
}

/* S274D:0142: a side's screen.  MNUFAHN.IFF shown, MNUCHR14.LIB loaded
 * with the menu's record of it (S37FF:001C), NEW UNITS and the month and
 * year in that font, the side's flag of LIB\FAHNEN.DAT, and the side's
 * tune loaded to the 898h bytes at the routine's beginning */
static int side_screen(uint16_t side, uint16_t month, uint16_t year, Far base, Far place, Far work)
{
    Far font, rec, p = place;
    uint16_t i;

    if (!lbm_show(place, text(0x001F), work))
        return 1;
    rec.seg = S_OPT;
    rec.off = 0x001C;
    if (lib_load(text(0x0012), place, work, rec, 0) == -1)
        return 1;
    font = far_at(S_OPT, 0x002A);
    text_mid_uncut(0x0C, text(0x0115), font, 0, 2);
    buf_format(text(0x0167), text((uint16_t)(0x009D + (int16_t)(month - 1) * 10)), year, 0);
    text_mid_uncut(0xAE, buf(), far_at(S_OPT, 0x002A), 0, 2);
    for (i = 0; i < 10; i++)
        fwb(S_STACK, (uint16_t)(A_BUF + i), 0);
    buf_copy(text(side == 1 ? 0x016F : 0x0176));
    if (lib_read(&p, text(0x002B), buf(), work, far_add(place, 0x4E20)) == -1)
        return 1;
    pic_draw(0x58, 0x37, place, 0);
    /* S2F39:1058(5): the music's end, left out */
    buf_copy(text(side == 1 ? 0x0083 : 0x0090));
    p = file_load(base, buf(), work);
    if (!(p.seg | p.off))
        return 1;
    /* S2F39:0F87(base, 5, FFh): the tune's start, left out */
    return 0;
}

/* S274D:0333: a new unit's screen.  NEWUNIT.LBM shown and 00.PAL loaded
 * as the palette; the kind's name; the frame and the kind's picture of
 * LIB\BIGUNIT.LIB (its name from LIB\BIGUNIT.DAT by the kind's byte 1Ah,
 * moved down by its byte 19h); LIB\UNIT.LIB loaded with the record at
 * S331F:0131 and the kind's small picture (the list's entry six times the
 * kind) with the colours' base of the record's second word and 20h; the
 * kind's numbers; the record's lines */
static int unit_screen(Far entry, Far place, Far work, uint16_t *result)
{
    static const struct { uint16_t x, y, name, fmt, a, b; } rows[7] = {
        /* the three of WEAPONS and SIZE/RADIUS: two of the kind's bytes,
         * the second less 1; then a byte each */
        { 0xE1, 0x27, 0x011F, 0x017D, 0x0A, 8 },
        { 0xE1, 0x2D, 0x0123, 0x0183, 0x0C, 9 },
        { 0xE1, 0x33, 0x0129, 0x0189, 0x0B, 9 },
        { 0xBB, 0x39, 0x0130, 0, 2, 0 },
        { 0xBE, 0x40, 0x0137, 0, 1, 0 },
        { 0xBC, 0x47, 0x013D, 0, 0x41, 0 },
        { 0xAE, 0x4E, 0x0144, 0, 3, 0 },
    };
    uint16_t kind = pw(entry, 0), y, i, base;
    Far p, rec, none = { 0, 0 };

    *result = 1;
    entry = far_add(entry, 2);
    if (!lbm_show(place, text(0x003A), work))
        return 1;
    p = file_load(far_at(S_palette_ptr, A_palette_ptr), text(0x0046), work);
    if (!(p.seg | p.off))
        return 1;
    rec.seg = S_unit_dat;
    rec.off = (uint16_t)(A_unit_dat + kind * KIND_SIZE + 0x2C);
    put(0x15, 0x75, 0x6F, rec);
    p = place;
    if (lib_read(&p, text(0x005D), text(0x007A), work, far_add(place, 0x2710)) == -1)
        return 1;
    pic_draw(0x19, 0x16, place, 0x40);
    p = file_load(place, text(0x004D), work);
    if (!(p.seg | p.off))
        return 1;
    /* CODE:2C39, the compiler's strncpy: the name's eight bytes */
    p = far_add(place, (uint32_t)kind_byte(kind, 0x1A) << 3);
    for (i = 0; i < 8 && pb(p, i); i++)
        fwb(S_STACK, (uint16_t)(A_BUF + i), pb(p, i));
    for (; i < 9; i++)
        fwb(S_STACK, (uint16_t)(A_BUF + i), 0);
    p = place;
    if (lib_read(&p, text(0x005D), buf(), work, far_add(place, 0x2710)) == -1)
        return 1;
    pic_draw(0x19, (uint16_t)(0x16 + kind_byte(kind, 0x19)), place, 0x40);
    rec.seg = S_lib_pal;
    rec.off = A_lib_pal;
    if (lib_load(text(0x006D), place, work, rec, 1) == -1) {
        *result = 0xFFFF;
        return 1;
    }
    base = pw(entry, 0);
    entry = far_add(entry, 2);
    p = far_add(far_at(S_GAME, 0x013F), (uint32_t)kind * 6 << 2);
    pic_draw(0x97, 0x65, pfar(p, 0), (uint8_t)(base + 0x20));
    put(0xAA, 0x27, 0x6F, text(0x0153));
    put(0xA8, 0x2D, 0x6F, text(0x015B));
    for (i = 0; i < 7; i++) {
        put(rows[i].x, rows[i].y, 0x6F, text(rows[i].name));
        if (rows[i].fmt)
            buf_format(text(rows[i].fmt), none, kind_byte(kind, rows[i].a),
                       (uint16_t)(kind_byte(kind, rows[i].b) - 1));
        else if (rows[i].a == 0x41 && !kind_byte(kind, 0x41))
            buf_copy(text(0x018F));     /* no weight */
        else
            buf_itoa(kind_byte(kind, rows[i].a));
        put(i < 3 ? 0x100 : 0xF1, rows[i].y, 0x71, buf());
    }
    /* the lines, 6 apart from 8Fh on: at least one */
    y = 0;
    do {
        put(0x14, (uint16_t)(0x8F + y), 0x6F, entry);
        y = (uint16_t)(y + 6);
        do {
            p = entry;
            entry = far_add(entry, 1);
        } while (pb(p, 0));
    } while (pb(entry, 0));
    return 0;
}

/* what inputs_release waits for: the key's word 4, a side's button, the
 * mouse's two */
static int pressed(void)
{
    Far k = far_at(S_DATA, 0x00BE), ms = far_at(S_DATA, 0x00C2);

    return pw(k, 4) || pb(far_at(S_DATA, 0x00A6), 4) || pb(far_at(S_DATA, 0x00B6), 4) || pw(ms, 0x30)
           || pw(ms, 0x34);
}

/* inputs_release(1, 1): all of them cleared until none comes any more,
 * then the wait for one, a call of the timer's routine at a time.  A
 * player (briefing_player) is waited for until the window is closed; a
 * key script that has no key left is not waited for longer than 400
 * calls, and the screens go on as if one had come */
static void wait_press(void)
{
    static int told;
    int idle = 0;

    inputs_release(0);
    while (!pressed() && plat_pump()) {
        if (!briefing_player && play_idle() && ++idle > 400) {
            if (!told)
                fprintf(stderr, "The new units' screens wait for a key the key script does not have: gone on.\n");
            told = 1;
            break;
        }
        play_tick();
    }
}

/* the 19 retraces the 33 or 32 steps of pal_level last in the original
 * (8.1 ms each), spread over them */
static void level(int16_t l, int step)
{
    pal_level((uint8_t)l, far_at(S_palette_ptr, A_palette_ptr));
    if (step * 19 / 33 != (step + 1) * 19 / 33)
        play_retraces(1);
}

static uint16_t screens(uint16_t map, Far place, Far work)
{
    Far base = place, idx, pc;
    uint16_t month, year, e, result = 0;
    int32_t size;
    int16_t l;
    int n = 0, step;

    if (map > 0x2F)
        return 1;
    if (map > 0x17)
        map = (uint16_t)(map - 0x18);
    place = idx = far_add(place, 0x898);
    size = file_load_size(place, text(0x0006), work);
    if (!size)
        return 1;
    place = far_add(place, (uint32_t)size);
    e = pw(far_add(idx, (uint32_t)map << 1), 0);
    if (!e)
        return 0;
    pc = far_add(idx, e);
    month = pw(pc, 0);
    pc = far_add(pc, 2);
    year = pw(pc, 0);
    pc = far_add(pc, 2);
    do {
        e = pw(pc, 0);
        pc = far_add(pc, 2);
        if (e <= 2) {
            if (side_screen(e, month, year, base, place, work))
                return 1;
        } else if (unit_screen(far_add(idx, e), place, work, &result))
            return result;
        page_flip();
        play_retraces(1);
        for (l = 0, step = 0; l <= 0xF8; l = (int16_t)(l + 8))
            level(l, step++);
        level(0xFF, step);
        if (briefing_hook)
            briefing_hook(1, ++n);
        wait_press();
        for (l = 0xF8, step = 0; l >= 0; l = (int16_t)(l - 8))
            level(l, step++);
    } while (pw(pc, 0));
    /* S2F39:1058(5): the music's end, left out */
    return 0;
}

uint16_t briefing(uint16_t map, Far place, Far work)
{
    uint16_t r;

    if (briefing_hook)
        briefing_hook(0, 0);
    r = screens(map, place, work);
    if (briefing_hook)
        briefing_hook(2, 0);
    return r;
}
