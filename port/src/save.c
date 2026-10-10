/* save.c - a battle saved and loaded: HL.EXE's save_game (S1273:0005),
 * load_game (S1273:0399) and save_blocks (S1273:0662), translated over the
 * program's memory.
 *
 * The file is a header of 1Bh bytes and 24 blocks copied from memory as
 * they are (save_blocks lists them); its name is the number's two digits
 * with the extension file_name's 5 gives.  The original writes it into
 * its own folder; the port keeps it where it keeps what the program
 * writes (hl_save_dir, files.c) and looks there first when it loads.
 * port/README.md says how this was compared with the original.
 */
#include <stdlib.h>
#include <string.h>
#include "hl.h"

#define S_DATA (HL_DATA + HL_BASE)
#define S_GAME (HL_S331F + HL_BASE)
#define S_OPT (HL_S37FF + HL_BASE)
#define S_COMP (HL_S39B6 + HL_BASE)
#define SIDE_SIZE 0x17u     /* a record of `sides` */
#define HEAD 0x1Bu          /* the file's header */
#define BLOCKS 0x18u        /* the blocks behind it */

static void pww(Far p, uint16_t o, uint16_t v)
{
    fww(p.seg, (uint16_t)(p.off + o), v);
}

/* S1273:0662 (save_blocks): the list of what a saved game holds behind
 * its header, eight bytes a block (far pointer, dword length), written
 * at `list`: the two maps, each side's list of attackers, the two blocks
 * of 82h bytes, the 320h bytes at `buf` (the colours), game_state, the
 * two views, the map's three words, field_bits, the three tables of
 * sites, the carriers, the units, unit_dat, and of the computer's player
 * two tables of nine bytes a unit and three of a record a side, then the
 * two words at S331F:02B5 */
static const uint16_t first_len[7] = {0x1806, 0x1806, 0xF2, 0xF2, 0x82, 0x82, 0x320};
static const struct { uint8_t comp; uint16_t off; uint32_t len; } fixed[] = {
    {0, 0x4CA4, 0x8F}, {0, 0x4D92, 0x31}, {0, 0x4DC3, 0x31}, {0, 0x4C9E, 6}, {0, 0x3B5A, 0x1104},
    {0, 0x3A0E, 0x1C * 0x0A}, {0, 0x38F6, 0x1C * 0x0A}, {0, 0x38BE, 0x1C * 2}, {0, 0x3116, 0x1C * 0x46},
    {0, 0x189C, 0x1A * 0xF1}, {0, 0x09BD, 0x45 * 0x33}, {1, 0x0132, 9 * 0xF1}, {1, 0x09AB, 9 * 0xF1},
    {1, 0x12FD, 0x0C * 2}, {1, 0x126B, 0x49 * 2}, {1, 0x1224, 0x20 * 2}, {0, 0x02B5, 2 * 2}};

static void save_blocks(Far list, Far buf)
{
    Far first[7];
    uint16_t i, o = 0;

    first[0] = far_at(S_map_ptr, A_map_ptr);
    first[1] = far_at(S_map2_ptr, A_map2_ptr);
    first[2] = far_at(S_sides, A_sides + 0x11);
    first[3] = far_at(S_sides, A_sides + SIDE_SIZE + 0x11);
    first[4] = far_at(S_GAME, 0x1893);
    first[5] = far_at(S_GAME, 0x1897);
    first[6] = buf;
    for (i = 0; i < 7; i++, o = (uint16_t)(o + 8)) {
        pww(list, o, first[i].off);
        pww(list, (uint16_t)(o + 2), first[i].seg);
        pww(list, (uint16_t)(o + 4), first_len[i]);
        pww(list, (uint16_t)(o + 6), 0);
    }
    for (i = 0; i < sizeof fixed / sizeof fixed[0]; i++, o = (uint16_t)(o + 8)) {
        pww(list, o, fixed[i].off);
        pww(list, (uint16_t)(o + 2), fixed[i].comp ? S_COMP : S_GAME);
        pww(list, (uint16_t)(o + 4), (uint16_t)fixed[i].len);
        pww(list, (uint16_t)(o + 6), (uint16_t)(fixed[i].len >> 16));
    }
}

/* Deviation from the original (win/origfixes): whether a saved game's
 * file is whole, that is as long as save_game writes it (the header and
 * every block of save_blocks' list) and beginning with 0 and 7.  The
 * original's load_game tests only the two bytes, in its buffer and not
 * in what was read, so an empty or cut file starts a battle of the
 * menu's setup mixed with what the file held, without a word; the port's
 * menu (saved_game) asks this first and shows SAVED GAME NOT FOUND for a
 * damaged file as for a missing one.  A whole file is loaded as before. */
int save_valid(const uint8_t *file, size_t size)
{
    size_t whole = HEAD;
    unsigned i;

    for (i = 0; i < 7; i++)
        whole += first_len[i];
    for (i = 0; i < sizeof fixed / sizeof fixed[0]; i++)
        whole += fixed[i].len;
    return size == whole && file[0] == 0 && file[1] == 7;
}

/* block i of the list: where it is in the memory image and how long */
static uint32_t block(Far list, uint16_t i, uint32_t *len)
{
    Far p = pfar(list, (uint16_t)(8 * i));

    *len = pw(list, (uint16_t)(8 * i + 4)) | (uint32_t)pw(list, (uint16_t)(8 * i + 6)) << 16;
    return lin(p.seg, p.off);
}

/* the saved game's name: the number's two digits (num_text into num_buf)
 * in the program's folder with the extension of file_name's 5 */
static Far save_name(int32_t n)
{
    Far num;

    num.seg = S_num_buf;
    num.off = A_num_buf;
    return file_name(2, -1, num_text(n, num, 2, 4), 5);
}

/* S1273:0005 (save_game): the first side's view is made dark (view_dim)
 * and the texts 1Eh, 1Dh and 1Ch are shown on it in the colours of
 * amok_dat's byte 0Dh; then it waits, the timer's routine going on
 * (LOOP_SAVE), for a key of the scancodes 2 to 0Bh, the digits 1 to 9
 * and 0, which is the game's number, or for the key of entry 0 of the
 * table of keys, which leaves with text 5Ah and the view as it is.  The
 * header is put together in the first side's work buffer (the far
 * pointer at sides + 0Dh), written, and the list of blocks (save_blocks)
 * written over it there and the blocks after it.  The view is drawn
 * again.  Returns the text to show: 5Bh, 5Ch when the file could not be
 * made, 5Ah; FFFFh when no key can come (loop_abort).
 * The original makes the file before it puts the header together and
 * leaves the buffer alone when it cannot; so does this, which writes the
 * file whole. */
uint16_t save_game(Far buf)
{
    Far k = far_at(S_DATA, 0x00BE), keys = far_at(S_GAME, 0x02D3), head, name;
    uint8_t col = NB(amok_dat, 0x0D), n = 0, h[HEAD];
    uint16_t text = 0x5B, i, s;
    uint32_t size = HEAD, len, at;
    uint8_t *file;
    int wait = 1;

    view_dim(0);
    game_text_at(0x24, 0x24, 0x1E, col);
    game_text_at(0x20, 0x42, 0x1D, col);
    game_text_at(0x2E, 0x88, 0x1C, col);
    page_flip();
    page_copy();
    loop_abort = 0;
    while (wait) {
        n = pb(k, 0);
        if (n >= 2 && n <= 0x0A) {
            n--;
            wait = 0;
        } else if (n == 0x0B) {
            n = 0;
            wait = 0;
        }
        if (pb(k, 0) == pb(keys, 0))
            return 0x5A;
        if (wait) {
            if (loop_hook)
                loop_hook(LOOP_SAVE, 0);
            if (!loop_hook || loop_abort)
                return 0xFFFF;
        }
    }
    pwb(k, 0, 0);
    head = far_at(S_sides, A_sides + 0x0D);
    name = save_name(n);
    memset(h, 0, sizeof h);
    h[1] = 7;
    h[2] = frb(S_GAME, 0x1892);
    h[3] = frb(S_GAME, 0x189B);
    for (i = 0; i < 6; i++)
        h[4 + i] = frb(S_OPT, (uint16_t)(0x0129 + i));
    h[0x0A] = NB(pics_winter, 0);
    h[0x0B] = frb(S_GAME, 0x0008);
    h[0x0C] = frb(S_GAME, 0x0009);
    for (s = 0; s < 2; s++) {
        uint16_t r = (uint16_t)(s * SIDE_SIZE);
        uint8_t *q = h + 0x0D + 7 * s;

        q[0] = NB(sides, r);
        q[1] = NB(sides, r + 1);
        q[2] = NB(sides, r + 2);
        q[3] = NB(sides, r + 3);
        q[4] = NB(sides, r + 4);
        q[5] = NB(sides, r + 0x15);
        q[6] = NB(sides, r + 0x16);
    }
    /* the list where the original builds it; what was there is put back
     * if the file cannot be written */
    {
        uint8_t kept[8 * BLOCKS];

        memcpy(kept, mem + lin(head.seg, head.off), sizeof kept);
        save_blocks(head, buf);
        for (i = 0; i < BLOCKS; i++) {
            block(head, i, &len);
            size += len;
        }
        file = malloc(size);
        if (file) {
            /* none of the blocks is that buffer */
            memcpy(file, h, HEAD);
            size = HEAD;
            for (i = 0; i < BLOCKS; i++) {
                at = block(head, i, &len);
                memcpy(file + size, mem + at, len);
                size += len;
            }
        }
        if (!file || !host_save(name, file, size)) {
            memcpy(mem + lin(head.seg, head.off), kept, sizeof kept);
            text = 0x5C;
        }
        free(file);
    }
    view_draw(NW(views, 2), 0);
    marks_draw(NW(views, 2), 0);
    page_flip();
    page_copy();
    return text;
}

/* S1273:0399 (load_game): the saved game whose number is game_state's
 * word 6 (the menu's) read back: the header into the first side's work
 * buffer and from there to where save_game took it from, then each block
 * of save_blocks' list to its place.  A file that is not there or does
 * not begin with 0 and 7 changes nothing and gives -1.  The music's and
 * the effects' bits of game_state (40h, 80h) are cleared when bit 10h or
 * 20h of its word 2 was not set before the load.  Returns 0.
 * A file that ends early leaves the rest as it is, as the original's
 * reads do. */
int16_t load_game(Far buf)
{
    Far head = far_at(S_sides, A_sides + 0x0D), name;
    uint32_t pos = HEAD, len, at;
    uint16_t i, s, before;
    size_t size;
    uint8_t *file;

    name = save_name((int32_t)(int16_t)NW(site_taken, 0));
    file = host_load(name, &size);
    if (!file)
        return -1;
    memcpy(mem + lin(head.seg, head.off), file, size < HEAD ? size : HEAD);
    if (pb(head, 0) != 0 || pb(head, 1) != 7) {
        free(file);
        return -1;
    }
    fwb(S_GAME, 0x1892, pb(head, 2));
    fwb(S_GAME, 0x189B, pb(head, 3));
    SETB(pics_winter, 0, pb(head, 0x0A));
    for (i = 0; i < 6; i++)
        fwb(S_OPT, (uint16_t)(0x0129 + i), pb(head, (uint16_t)(4 + i)));
    fww(S_GAME, 0x0008, pw(head, 0x0B));
    for (s = 0; s < 2; s++) {
        uint16_t r = (uint16_t)(s * SIDE_SIZE), q = (uint16_t)(0x0D + 7 * s);

        SETW(sides, r, pw(head, q));
        SETB(sides, r + 2, pb(head, (uint16_t)(q + 2)));
        SETB(sides, r + 3, pb(head, (uint16_t)(q + 3)));
        SETB(sides, r + 4, pb(head, (uint16_t)(q + 4)));
        SETB(sides, r + 0x15, pb(head, (uint16_t)(q + 5)));
        SETB(sides, r + 0x16, pb(head, (uint16_t)(q + 6)));
    }
    before = NW(game_state, 2);
    save_blocks(head, buf);
    for (i = 0; i < BLOCKS; i++) {
        at = block(head, i, &len);
        if (pos < size)
            memcpy(mem + at, file + pos, size - pos < len ? size - pos : len);
        pos += len;
    }
    free(file);
    if (!(before & 0x10))
        SETW(game_state, 0, NW(game_state, 0) & 0xFFBF);
    if (!(before & 0x20))
        SETW(game_state, 0, NW(game_state, 0) & 0xFF7F);
    return 0;
}
