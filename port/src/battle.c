/* battle.c - the first parts of HL.EXE's game_loop (S066A:000D), the
 * program's main: what it sets up once at the start, and how it shares
 * out its block of memory and loads a battle's files.  Translated over
 * the program's memory as far as it draws nothing and makes no sound; the
 * calls left out are named where they stand.  port/README.md says how
 * this was compared with the original.
 */
#include "hl.h"

#define S_DATA (HL_DATA + HL_BASE)
#define S_GAME (HL_S331F + HL_BASE)     /* the segment of the game's state */
#define S_MAIN (HL_S3303 + HL_BASE)     /* game_loop's own DS */
#define S_OPT (HL_S37FF + HL_BASE)

GameMem game_mem;

static void set_far(uint16_t seg, uint16_t off, Far p)
{
    fww(seg, off, p.off);
    fww(seg, (uint16_t)(off + 2), p.seg);
}

static Far game_text(uint16_t off)
{
    Far p;

    p.seg = S_GAME;
    p.off = off;
    return p;
}

/* one of the two files loaded at the start (S066A:0190, S066A:023F): to
 * the block's free end, the work buffer 7530h bytes behind it; the place
 * goes to `keep` in the game's state and to `rec` in the record behind
 * DATA:0092, and the free end moves on by the file's length */
static int start_file(int16_t ext, uint16_t keep, uint16_t rec)
{
    Far name = file_name(1, 0, game_text(0x023E), ext);
    Far scr;
    int32_t size = file_load_size(game_mem.p, name, far_add(game_mem.p, 0x7530));


    fww(S_MAIN, 0x0024, (uint16_t)size);
    fww(S_MAIN, 0x0026, (uint16_t)((uint32_t)size >> 16));
    if (size == 0)
        return 2;
    set_far(S_GAME, keep, game_mem.p);
    scr = far_at(S_DATA, 0x0092);
    fww(scr.seg, (uint16_t)(scr.off + rec), game_mem.p.off);
    fww(scr.seg, (uint16_t)(scr.off + rec + 2), game_mem.p.seg);
    game_mem.p = far_add(game_mem.p, (uint32_t)size);
    game_mem.block = far_add(game_mem.block, (uint32_t)size);
    return 0;
}

/* game_loop from its entry to S066A:0349, before the menu: the program's
 * one block of memory and the two files that stay at its beginning.
 * Returns 0, or what the original hands its error exit (S0CB6:1842): 1 no
 * memory, 2 a file.
 * Left out: the command line's /a and /b and the sound's start
 * (S2F39:000B, S30EF:1C2D, S2F39:1058; the port is the game with both
 * devices found), S27FE:0013, S27FE:056C, S1571:0426 and S1571:0443 (not read), and of
 * S1650:0003 all but what it keeps of the keys: the far pointer to the
 * table of keys at S331F:0292, which game_loop compares the key of a pass
 * with, and twelve of its entries at S331F:02C7 (it also loads a file and
 * shows something; read no further). */
int start_setup(void)
{
    Far p;
    int r;

    fww(S_GAME, 0x4CA4, 0);
    fww(S_GAME, 0x4CA6, 0);
    p = far_at(S_DATA, 0x0096);
    fww(p.seg, p.off, 1);
    fww(p.seg, (uint16_t)(p.off + 2), 0);
    fww(S_MAIN, 0x0022, 0);
    game_mem.block = mem_alloc(0x445C0);
    if (!(game_mem.block.seg | game_mem.block.off))
        return 1;
    game_mem.p = game_mem.block;
    /* S066A:0131 to 0182: the screen; the error exit for a non-zero
     * return of screen_open is left out (the port's returns nothing) */
    screen_open(0x140, 0xC8, 0x2518, 1);
    p = far_at(S_DATA, 0x0092);
    fww(p.seg, p.off, 0);
    screen_clear();
    page_flip();
    screen_clear();
    if ((r = start_file(3, 0x17AA, 0x290)) != 0 || (r = start_file(5, 0x17A6, 0x294)) != 0)
        return r;
    {
        /* S1650:00F8 to 0220 */
        static const uint8_t entry[12] = { 8, 8, 9, 0x0B, 0x0A, 6, 0x1A, 0x1E, 0x1C, 0x1B, 0x1D, 1 };
        uint16_t i;

        fww(S_GAME, 0x02D5, S_GAME);
        fww(S_GAME, 0x02D3, 0x0292);
        for (i = 0; i < 12; i++)
            fwb(S_GAME, (uint16_t)(0x02C7 + i), frb(S_GAME, (uint16_t)(0x0292 + entry[i])));
    }
    fww(S_GAME, 0x4D4A, 0);
    SETW(sides, 0, 0);
    fww(S_GAME, 0x4D31, 0);
    fww(S_GAME, 0x4D2F, 0);
    SETW(map_number, 0, 0);
    return 0;
}

/* What the menu (S167A, not translated) leaves when a map is chosen and
 * nothing else is changed: the map's number; from byte 6 of the map's
 * record in CODES.DAT whether it is a map for two players (2: both sides
 * human) or for one (400h in game_state, and one side the computer's: the
 * second below map 18h, else the first); in word 2 of game_state 1 for
 * one side of the computer and 2 for none; FFh in byte 16h of both sides'
 * records; from byte 8 of the map's record FFh at S37FF:012F when its
 * bit of value 2 is set, else 0 (the menu's store at S167A:25C1; the
 * grounds' and the pictures' files are chosen by it, W or S).  With the 1
 * game_loop sets in game_state before the menu (S066A:0349).  The file is
 * loaded to the block's free part only to be read here (the menu has it
 * elsewhere).  Returns 0, or 2 when the file cannot be loaded. */
int menu_result(uint16_t map)
{
    Far name, codes;
    uint16_t two;

    name.seg = S_OPT;
    name.off = 0x009E;
    codes = file_load(game_mem.p, file_name(1, -1, name, 5), far_add(game_mem.p, 0x1000));
    if (!(codes.seg | codes.off))
        return 2;
    two = pb(codes, (uint16_t)(map * 10 + 6)) == 2;
    SETW(map_number, 0, map);
    SETW(game_state, 0, (NW(game_state, 0) & 0xFBFF) | 1 | (two ? 0 : 0x400));
    SETW(sides, 0, (NW(sides, 0) & 0xFFFD) | (!two && map >= 0x18 ? 2 : 0));
    SETW(sides, 0x17, (NW(sides, 0x17) & 0xFFFD) | (!two && map < 0x18 ? 2 : 0));
    SETW(game_state, 2, (NW(game_state, 2) & 0xFFF0) | (two ? 2 : 1));
    SETB(sides, 0x16, 0xFF);
    SETB(sides, 0x17 + 0x16, 0xFF);
    fwb(S_OPT, 0x012F, pb(codes, (uint16_t)(map * 10 + 8)) & 2 ? 0xFF : 0);
    return 0;
}

void (*battle_hook)(uint16_t picture, uint16_t arg);

/* S167A:3610 (scores_load): the map's four best scores, dwords at buf,
 * and their names of six bytes from buf + 10h on: zeros and four times
 * the name given, then the map's file of scores loaded over them (it
 * need not be there: the word behind DATA:0096 is 0 meanwhile) */
static void scores_load(Far buf, Far work, int16_t map, Far name)
{
    Far err = far_at(S_DATA, 0x0096);
    uint16_t i, j;

    for (i = 0; i < 4; i++) {
        fww(buf.seg, (uint16_t)(buf.off + 4 * i), 0);
        fww(buf.seg, (uint16_t)(buf.off + 4 * i + 2), 0);
        for (j = 0; j < 6; j++)
            pwb(buf, (uint16_t)(0x10 + 6 * i + j), pb(name, j));
    }
    fww(err.seg, err.off, 0);
    file_load(buf, file_name(2, 1, num_text(map, game_text(0x177C), 2, 4), 0x0B), work);
    fww(err.seg, err.off, 1);
}

/* S063F:0009 (battle_score), behind game_loop's loop: the map's number
 * kept in game_state's word 0Fh, 1F4h more score for a side left without
 * units (2000h), the next map and whether this was the last (20h) when a
 * human has won (10h), else no score; a score above the lowest of the
 * map's four best draws a random number, which in three of eight cases
 * chooses a picture (battle_hook; not shown here) */
static void battle_score(Far work)
{
    Far codes = far_add(work, 0x2710), best = far_add(work, 0x2710 + 0x3E8), name;
    int32_t score, low = 0x7EF4;
    uint16_t i, map, t;

    if (NW(round_count, 0) < 1)
        return;
    name.seg = S_OPT;
    name.off = 0x009E;
    codes = file_load(codes, file_name(1, -1, name, 5), work);
    if (!(codes.seg | codes.off))
        return;
    map = NW(map_number, 0);
    SETW(game_state, 0x0F, map);
    score = (int32_t)(NW(score, 0) | (uint32_t)NW(score, 2) << 16);
    if (NW(game_state, 0) & 0x2000)
        score += 0x1F4;
    if (NW(game_state, 0) & 0x10) {
        SETW(map_number, 0, map + pb(far_add(codes, (uint32_t)((int32_t)(int16_t)map * 10)), 7));
        if (pb(far_add(codes, (uint32_t)((int32_t)(int16_t)map * 10)), 8) & 1)
            SETW(game_state, 0, NW(game_state, 0) | 0x20);
    } else
        score = 0;
    SETW(score, 0, score);
    SETW(score, 2, (uint32_t)score >> 16);
    if (score <= 0)
        return;
    name.seg = S_MAIN;
    name.off = 0;
    scores_load(best, work, (int16_t)map, name);
    for (i = 0; i < 4; i++) {
        int32_t v = (int32_t)(pw(best, (uint16_t)(4 * i)) | (uint32_t)pw(best, (uint16_t)(4 * i + 2)) << 16);

        if (v < low)
            low = v;
    }
    if (score <= low)
        return;
    t = NW(views, 0x22);
    i = (uint16_t)rand_next() & 7;
    if (i < 3 && battle_hook)
        battle_hook(t == 0x0F || t == 0x11 ? 0xCE : 0xCF, i);
}

/* game_loop behind its loop, S066A:45E8 to 4728: with 10h in game_state
 * (a human has won) the picture D0h, or CDh when a side was left without
 * units (2000h), for the side whose view has 0Fh in word 22h (the first,
 * else the second); battle_score; with one side the computer's below map
 * 30h the picture whose number is the map's (the one before it when the
 * battle was won, 6000h, and battle_score went on to the next; the map's
 * own after the last, 20h); after the last map 100h is set and 1 and 2
 * cleared, which ends the program with 7Bh; 6030h cleared.  Returns that
 * 7Bh or 0.  The pictures are not shown (battle_hook is told each); left
 * out too: the music's end (S2F39:1058), fade_out and S2AF3:000A. */
uint16_t battle_after(void)
{
    uint16_t result = 0, gs = NW(game_state, 0);

    if (gs & 0x10 && battle_hook)
        battle_hook(gs & 0x2000 ? 0xCD : 0xD0, NW(views, 0x22) == 0x0F ? 0 : 1);
    battle_score(game_mem.lists_end);
    gs = NW(game_state, 0);
    if (!(NW(game_state, 2) & 2) && (int16_t)NW(map_number, 0) < 0x30 && battle_hook)
        battle_hook(gs & 0x6000 && !(gs & 0x20) ? NW(map_number, 0) - 1 : NW(map_number, 0), 0);
    if (gs & 0x20) {
        result = 0x7B;
        gs = (gs | 0x100) & 0xFFFC;
    }
    SETW(game_state, 0, gs & 0x9FCF);
    return result;
}

/* game_loop from S066A:0417 to S066A:0938: shares the block out (the
 * buffer save_game uses, the two 82h blocks, the two maps, a place for a
 * file of texts, each side's lists) and loads the battle's files: the
 * texts, AMOK.DAT, UNIT.DAT, WW1GNDS.DAT, then the map whose number is at
 * map_number and its .SHP file.  Returns 0, or 2 for a file that cannot
 * be loaded (the original's error exit).
 * Left out: S27FE:056C (key_install) and S02E6:323E, which
 * loads the map's pictures to the block's free end (the lists then lie
 * over them). */
int battle_setup(void)
{
    Far scr = far_at(S_DATA, 0x0092), p = game_mem.block, name, num;
    uint16_t i;

    fww(scr.seg, (uint16_t)(scr.off + 0x0E), 0);
    fww(scr.seg, (uint16_t)(scr.off + 0x12), 0);
    fww(scr.seg, (uint16_t)(scr.off + 0x14), 0);
    fww(scr.seg, (uint16_t)(scr.off + 0x16), 0x140);
    fww(scr.seg, (uint16_t)(scr.off + 0x18), 0xB3);
    for (i = 8; i < 0x0C; i++)
        fwb(S_GAME, (uint16_t)(0x4CA4 + i), 0);
    game_mem.work = p;
    game_mem.save_buf = p;
    p = far_add(p, 0x320);
    set_far(S_GAME, 0x1893, p);
    p = far_add(p, 0x82);
    set_far(S_GAME, 0x1897, p);
    p = far_add(p, 0x82);
    set_far(S_map_ptr, A_map_ptr, p);
    p = far_add(p, 0x1806);
    set_far(S_map2_ptr, A_map2_ptr, p);
    p = far_add(p, 0x1806);
    set_far(S_GAME, 0x17BA, p);
    p = far_add(p, 0xBB8);
    game_mem.gfx = p;
    fww(scr.seg, (uint16_t)(scr.off + 0x1C), 0xFFFF);
    pages_set();
    page_flip();
    /* each side's record: the list of its attackers (11h), two blocks of
     * 2710h (0Dh; the first is the work buffer of the loads below) and
     * 3E8h bytes (9; side 0's takes the .SHP file) */
    set_far(S_sides, A_sides + 0x17 + 0x11, p);
    p = far_add(p, 0xF2);
    set_far(S_sides, A_sides + 0x11, p);
    p = far_add(p, 0xF2);
    game_mem.lists_end = p;
    game_mem.work = p;
    set_far(S_sides, A_sides + 0x0D, p);
    p = far_add(p, 0x2710);
    set_far(S_sides, A_sides + 0x17 + 0x0D, p);
    p = far_add(p, 0x2710);
    set_far(S_sides, A_sides + 9, p);
    p = far_add(p, 0x3E8);
    set_far(S_sides, A_sides + 0x17 + 9, p);
    p = far_add(p, 0x3E8);
    game_mem.p = p;
    /* two places in the screen's memory behind the picture */
    SETW(sides, 5, 0x0004);
    SETW(sides, 7, 0xA7E8);
    SETW(sides, 0x17 + 5, 0x0300);
    SETW(sides, 0x17 + 7, 0xA7D0);

    name = file_name(1, -1, game_text(0x024C), 6);
    p = file_load(far_at(S_GAME, 0x17BA), name, game_mem.work);
    if (!(p.seg | p.off))
        return 2;
    name = file_name(1, -1, game_text(0x0247), 5);
    p = file_load(game_text(A_amok_dat), name, game_mem.work);
    if (!(p.seg | p.off))
        return 2;
    name = file_name(1, -1, game_text(0x0257), 5);
    p = file_load(game_text(A_unit_dat), name, game_mem.work);
    if (!(p.seg | p.off))
        return 2;
    /* the last letter of the grounds' file's name */
    fwb(S_GAME, 0x0262, frb(S_OPT, 0x012F) ? 0x57 : 0x53);
    name = file_name(1, -1, game_text(0x025C), 5);
    p = file_load(game_text(A_ww1gnds_dat), name, game_mem.work);
    if (!(p.seg | p.off))
        return 2;
    num = num_text((int16_t)NW(map_number, 0), game_text(0x177C), 2, 4);
    name = file_name(1, 1, num, 0);
    if (map_load(name, far_at(S_map_ptr, A_map_ptr), game_mem.work))
        return 2;
    fww(S_GAME, 0x0008, (int16_t)NW(map_width, 0) > 0x20 || (int16_t)NW(map_height, 0) > 0x28 ? 1 : 2);
    name = file_name(1, 1, num, 2);
    if (shp_load(name, far_at(S_sides, A_sides + 9), game_mem.work))
        return 2;
    return 0;
}

#define S_COMP (HL_S39B6 + HL_BASE)     /* the computer's player's segment */
#define VIEW_SIZE 0x31u

/* defence_sum (S02E6:29D4) */
uint32_t defence_sum(void)
{
    uint8_t level = NB(sides, 0x16);
    int32_t sum = 0;
    uint16_t u;

    for (u = 0; u <= 0xF0; u++) {
        uint16_t flags = NW(unit_flags, u * UNIT_SIZE);

        if (flags & 0x8000 || ((flags & 0x40) && !(flags & 0x80)))
            continue;
        if (NW(units, u * UNIT_SIZE + 6) & 4)
            continue;
        sum += NB(unit_dat, NB(unit_kind, u * UNIT_SIZE) * KIND_SIZE + 2);
    }
    if (NW(game_state, 2) & 8)
        sum += 0x64;
    if (level == 4)
        sum *= 4;
    else if (level == 8)
        sum *= 3;
    else if (level == 0x10)
        sum *= 2;
    return sum > 0x7EF4 ? 0x7EF4 : (uint32_t)sum;
}

/* computer_reset (S1C74:01B4) */
static void computer_reset(uint16_t side)
{
    uint16_t r = (uint16_t)(0x12FD + side * 0x0C), u;

    fww(S_COMP, (uint16_t)(0x1315 + side * 4), 0);
    fww(S_COMP, (uint16_t)(0x1317 + side * 4), 0x4E);
    fww(S_COMP, r, 0x20);
    for (u = 2; u < 9; u++)
        fwb(S_COMP, (uint16_t)(r + u), 0);
    fww(S_COMP, (uint16_t)(r + 9), 0);
    fwb(S_COMP, (uint16_t)(r + 0x0B), 0);
    for (u = 0; u <= 0xF0; u++) {
        uint16_t t;

        for (t = 0x0132; t <= 0x09AB; t += 0x09AB - 0x0132) {
            uint16_t o = (uint16_t)(t + u * 9);

            fwb(S_COMP, (uint16_t)(o + 8), 0);
            fww(S_COMP, (uint16_t)(o + 4), 0xFFFF);
            fww(S_COMP, (uint16_t)(o + 2), 0);
            fww(S_COMP, (uint16_t)(o + 6), 0);
        }
    }
    fwb(S_COMP, (uint16_t)(0x1224 + side * 0x20), 0xFF);
    fwb(S_COMP, (uint16_t)(0x1242 + side * 0x20), 0);
    fwb(S_COMP, (uint16_t)(0x1243 + side * 0x20), 0);
    fwb(S_COMP, (uint16_t)(0x126B + side * 0x49), 0xFF);
    fwb(S_COMP, (uint16_t)(0x12B1 + side * 0x49), 0);
    fwb(S_COMP, (uint16_t)(0x12B2 + side * 0x49), 0);
    fwb(S_COMP, (uint16_t)(0x12B3 + side * 0x49), 0);
}

/* view_reset (S0CB6:0CDA) */
static void view_reset(uint16_t side)
{
    uint16_t v = (uint16_t)(A_views + side * VIEW_SIZE), o;

    for (o = 0x17; o < 0x1B; o++)
        fwb(S_views, (uint16_t)(v + o), 0);
    fwb(S_views, (uint16_t)(v + 0x1B), 5);
    for (o = 0x1C; o < 0x28; o += 2)
        fww(S_views, (uint16_t)(v + o), 0);
    for (o = 4; o < 0x0C; o += 2)
        fww(S_views, (uint16_t)(v + o), 0);
    fwb(S_views, (uint16_t)(v + 0x30), 0);
    fww(S_views, (uint16_t)(v + 0x2E), 0);
    fwb(S_views, (uint16_t)(v + 0x2C), 0);
    fwb(S_views, (uint16_t)(v + 0x2D), 0);
    fwb(S_views, (uint16_t)(v + 0x2B), 0);
    fwb(S_views, (uint16_t)(v + 0x28), 1);
    fwb(S_views, (uint16_t)(v + 0x29), 2);
    fwb(S_views, (uint16_t)(v + 0x2A), 1);
}

/* view_cursor (S0CB6:0154) */
void view_cursor(uint16_t side)
{
    uint16_t v = (uint16_t)(A_views + side * VIEW_SIZE);
    uint16_t col = frw(S_views, (uint16_t)(v + 0x0C)), row = frw(S_views, (uint16_t)(v + 0x0E));
    uint16_t x = NW(col_x, col * 2);

    fww(S_views, (uint16_t)(v + 0x12), col & 1 ? NW(row_y_odd, row * 2) : NW(row_y, row * 2));
    fww(S_views, (uint16_t)(v + 0x10), (uint16_t)(side ? x + 0xA0 : x));
    fww(S_views, v, (uint16_t)(frw(S_views, (uint16_t)(v + 2)) + col * 2 + row * (NW(map_width, 0) * 2)));
}

/* view_to_field (S0E56:177C) */
void view_to_field(uint16_t side, uint16_t field)
{
    uint16_t v = (uint16_t)(A_views + side * VIEW_SIZE);
    int16_t w = (int16_t)NW(map_width, 0), f = (int16_t)((int16_t)field >> 1);
    int16_t col = (int16_t)(f % w), row = (int16_t)(f / w);
    int16_t left = (int16_t)(col - 4), top = (int16_t)(row - 4);
    uint8_t most;

    if (left & 1)
        left--;
    if (top & 1)
        top--;
    if (left < 0)
        left = 0;
    if (top < 0)
        top = 0;
    most = (uint8_t)(w - 10);
    if (most < left)
        left = most;
    most = (uint8_t)(NB(map_height, 0) - 8);
    if (most < top)
        top = most;
    col = (int16_t)(col - left);
    row = (int16_t)(row - top);
    if (col < 0)
        col = 0;
    if (row < 0)
        row = 0;
    fww(S_views, (uint16_t)(v + 2), (uint16_t)(top * w * 2 + left * 2));
    fww(S_views, (uint16_t)(v + 0x0C), (uint16_t)col);
    fww(S_views, (uint16_t)(v + 0x0E), (uint16_t)row);
    fwb(S_views, (uint16_t)(v + 0x15), (uint8_t)(top >> 1));
    fwb(S_views, (uint16_t)(v + 0x14), (uint8_t)(left >> 1));
    view_cursor(side);
}

/* a far routine's address in a record */
static void set_code(Far rec, uint16_t o, uint16_t seg, uint16_t off)
{
    fww(rec.seg, (uint16_t)(rec.off + o), off);
    fww(rec.seg, (uint16_t)(rec.off + o + 2), (uint16_t)(seg + HL_BASE));
}

/* S02E6:3484 (libs_start): the libraries of the records at S331F:0131
 * (sorted by its list) and S331F:0159 loaded one behind the other to the
 * free end, which moves on by each file's length.  Returns 0, or 2 for a
 * file that cannot be loaded (the original's error exit). */
static int libs_start(void)
{
    static const uint16_t recs[2] = { A_lib_pal, A_lib_rnd };
    int i;

    for (i = 0; i < 2; i++) {
        Far rec = game_text(recs[i]);

        if (lib_load(file_name(1, 0, rec, 3), game_mem.p, game_mem.work, rec, (uint8_t)(i == 0)) == -1)
            return 2;
        game_mem.p = far_add(game_mem.p, pw(rec, 0x0A) | (uint32_t)pw(rec, 0x0C) << 16);
    }
    return 0;
}

/* game_loop from S066A:0938 to S066A:0EE6, where it goes into its loop:
 * the computer's file of a single player's map, the tables and counts of
 * a battle's beginning, each side's view put on the field of its record
 * of sites_40, fight_rec's routines.  Returns 0, or 2 for a file that
 * cannot be loaded.
 * Left out: S274D:0003 (not with a saved game; shows something before the
 * battle of a map up to 2Fh, not read to its end), the loading of a saved
 * game (200h in game_state) and the music's start (S2F39:0F87). */
int battle_begin(void)
{
    Far st = game_text(A_game_state), rec, p;
    uint16_t i;

    if (NW(game_state, 0) & 0x400) {
        Far num = num_text((int16_t)NW(map_number, 0), game_text(0x177C), 2, 4);
        Far to;

        to.seg = S_COMP;
        to.off = 0;
        p = file_load(to, file_name(1, 1, num, 9), game_mem.work);
        if (!(p.seg | p.off))
            return 2;
    }
    if (libs_start())
        return 2;
    /* pictures_load(free end, work buffer, the 320h bytes' buffer for the
     * palette); game_loop keeps the free end it returns at S3303:0028,
     * FFFF:FFFF too */
    if (pictures_load(&game_mem.p, game_mem.work, game_mem.save_buf) == 2)
        return 2;
    fww(S_MAIN, 0x0028, game_mem.p.off);
    fww(S_MAIN, 0x002A, game_mem.p.seg);
    /* field_bits_clear(FFh) */
    for (i = 0; i < 0x1104; i++)
        SETB(field_bits, i, 0);
    /* view_tables */
    for (i = 0; i < 8; i++) {
        SETW(row_y, i * 2, i * 0x18);
        SETW(row_y_odd, i * 2, i * 0x18 + 0x0C);
    }
    for (i = 0; i < 10; i++)
        SETW(col_x, i * 2, i * 0x10);
    fww(S_GAME, 0x02B7, 0);
    fww(S_GAME, 0x02B5, 0);
    /* clock_seed(0) */
    fww(S_DATA, 0x0AF3, frw(0x40, 0x6C) ^ frw(0x40, 0x6E));
    fww(S_DATA, 0x0AF1, frw(0x40, 0x6D) | frw(0x40, 0x6F));
    fww(st.seg, (uint16_t)(st.off + 0x15), 0);
    fww(st.seg, (uint16_t)(st.off + 0x13), 0);
    fww(st.seg, (uint16_t)(st.off + 0x11), 0);
    fww(st.seg, (uint16_t)(st.off + 4), 0);
    pwb(st, 0x86, 0);
    pwb(st, 0x0D, (uint8_t)((int16_t)(NW(map_width, 0) - 10) >> 1));
    pwb(st, 0x0E, (uint8_t)((int16_t)(NW(map_height, 0) - 8) >> 1));
    fww(st.seg, (uint16_t)(st.off + 0x0F), 4);
    {
        uint32_t sum = defence_sum();

        fww(st.seg, (uint16_t)(st.off + 0x8B), (uint16_t)sum);
        fww(st.seg, (uint16_t)(st.off + 0x8D), (uint16_t)(sum >> 16));
    }
    for (i = 0; i < 0x0F; i++) {
        fww(st.seg, (uint16_t)(st.off + 0x2A + i * 4), 0);
        fww(st.seg, (uint16_t)(st.off + 0x28 + i * 4), 0);
        pwb(st, (uint16_t)(0x19 + i), 0);
        fww(st.seg, (uint16_t)(st.off + 0x64 + i * 2), 0);
    }
    /* no attackers yet; bits 0 and 2 of the sides' word 0 */
    SETB(sides, 0x17 + 0x15, 0);
    SETB(sides, 0x15, 0);
    SETW(sides, 0, NW(sides, 0) & 0xFFFA);
    SETW(sides, 0x17, NW(sides, 0x17) & 0xFFFA);
    if (NW(sides, 0) & 2)
        computer_reset(0);
    if (NW(sides, 0x17) & 2)
        computer_reset(1);
    view_reset(0);
    view_reset(1);
    /* each side's view on the field below the one of its record of
     * sites_40 */
    view_to_field(0, (uint16_t)(NW(sites_40, 0x0E) + NW(map_width, 0) * 2));
    view_to_field(1, (uint16_t)(NW(sites_40, 0x1C + 0x0E) + NW(map_width, 0) * 2));
    SETB(views, 0x16, 1);
    SETB(views, VIEW_SIZE + 0x16, 2);
    fwb(S_GAME, 0x189B, 0);
    fwb(S_GAME, 0x1892, 0);
    for (i = 0xA6; i <= 0xB6; i += 0x10) {
        rec = far_at(S_DATA, i);
        pwb(rec, 4, 0);
        pwb(rec, 8, 0);
        pwb(rec, 5, 0);
        pwb(rec, 3, 0);
        pwb(rec, 2, 0);
        pwb(rec, 1, 0);
        pwb(rec, 0, 0);
    }
    for (i = 0; i < 4; i++)
        SETB(blasts, i * 7 + 2, 0xFF);
    /* fight_rec: a far pointer of a record of pictures and the routines
     * the fight calls */
    rec = game_text(A_fight_rec);
    fww(rec.seg, (uint16_t)(rec.off + 0x40), frw(S_GAME, 0x0141));
    fww(rec.seg, (uint16_t)(rec.off + 0x3E), frw(S_GAME, 0x013F));
    set_code(rec, 0x5A, HL_S30EF, 0x1E8B);
    set_code(rec, 0x5E, HL_S2AF3, 0x000A);
    set_code(rec, 0x62, HL_S2A6B, 0x0008);
    set_code(rec, 0x66, HL_S2A6F, 0x0002);
    set_code(rec, 0x6A, HL_CODE, A_rand_seed);
    set_code(rec, 0x6E, HL_S0CB6, 0x0713);
    set_code(rec, 0x72, HL_CODE, 0x0288);
    fww(rec.seg, (uint16_t)(rec.off + 0x34), 0);
    fww(rec.seg, (uint16_t)(rec.off + 0x32), 0);
    SETW(game_state, 2, NW(game_state, 2) | 0x30);
    SETW(game_state, 0, NW(game_state, 0) | 0xC0);
    fww(S_GAME, 0x17F3, 0);
    SETB(free_depth, 0, 0);
    /* S066A:0DE8: both views drawn from word 2 of each side's view, their
     * marks, the status lines cleared */
    for (i = 0; i < 2; i++) {
        view_draw(NW(views, i * 0x31 + 2), i);
        marks_draw(NW(views, i * 0x31 + 2), i);
    }
    show_message(0xFFFF, 0);
    show_message(0xFFFF, 1);
    SETW(game_state, 0, NW(game_state, 0) & 0xFFF3);
    rec = far_at(S_DATA, 0xBE);
    fww(rec.seg, (uint16_t)(rec.off + 4), 0);
    pwb(rec, 0, 0);
    pwb(rec, 2, 0);
    /* S066A:0E95: the drawn page shown, copied to the other one, the
     * colours up from dark */
    page_flip();
    page_copy();
    fade_in();
    SETB(game_state, 0x0C, 0);
    SETW(game_state, 0, NW(game_state, 0) | 2);
    return 0;
}
