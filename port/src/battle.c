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

/* ptr_move, ptr_add (CODE:04CB, CODE:0553): a far pointer moved on by `n` bytes, its offset
 * below 10h afterwards */
static Far far_add(Far p, uint32_t n)
{
    uint32_t off = (uint32_t)p.off + (uint16_t)n;
    Far r;

    r.seg = (uint16_t)(p.seg + (off >> 16 << 12) + (uint16_t)((n >> 16) << 12) + ((uint16_t)off >> 4));
    r.off = (uint16_t)(off & 0x0F);
    return r;
}

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
 * devices found), the screen's (S298E:000D, S28D9:000C, S2974:0002),
 * S27FE:0013, S27FE:056C, S1571:0426, S1571:0443 and S1650:0003 (not
 * read). */
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
    p = far_at(S_DATA, 0x0092);
    fww(p.seg, p.off, 0);
    if ((r = start_file(3, 0x17AA, 0x290)) != 0 || (r = start_file(5, 0x17A6, 0x294)) != 0)
        return r;
    fww(S_GAME, 0x4D4A, 0);
    SETW(sides, 0, 0);
    fww(S_GAME, 0x4D31, 0);
    fww(S_GAME, 0x4D2F, 0);
    SETW(map_number, 0, 0);
    return 0;
}

/* game_loop from S066A:0417 to S066A:0938: shares the block out (the
 * buffer save_game uses, the two 82h blocks, the two maps, a place for a
 * file of texts, each side's lists) and loads the battle's files: the
 * texts, AMOK.DAT, UNIT.DAT, WW1GNDS.DAT, then the map whose number is at
 * map_number and its .SHP file.  Returns 0, or 2 for a file that cannot
 * be loaded (the original's error exit).
 * Left out: S27FE:056C, S2A10:0006, S2974:0002, and S02E6:323E, which
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
