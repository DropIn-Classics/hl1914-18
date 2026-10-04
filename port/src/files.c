/* files.c - reading the game's files: HL.EXE's file_load (S2C45:0002) with
 * its unpacking and ptr_norm (S2C9E:0002), translated over the program's
 * memory; the file itself comes from the installed game's folder.
 * port/README.md says how this was compared with the original.
 *
 * file_size (S2B8F:000C) and the routine at S2B81:0004 that loads a file
 * and returns its length are translated from reading only.
 *
 * Not translated: the allocation when no place is given (mem_alloc), the
 * four tries with a message when a file cannot be opened, and the hook
 * around the opening that DATA:04B9 switches on.
 */
#include <stdlib.h>
#include <string.h>
#include "hl.h"
#include "sys.h"

const char *hl_game_dir = ".";

/* file_load's own variables, in its code segment */
#define S_LOAD (HL_S2C45 + HL_BASE)
#define A_HEAD 0x0486u      /* the file's first eight bytes */
#define A_LEFT 0x048Au      /* in them: the unpacked length, counted down */
#define A_POS 0x048Eu       /* place in the work buffer */
#define A_END 0x0490u       /* far pointer behind the unpacked data */
#define A_KEPT 0x0494u      /* the word that was there */

#define CHUNK 0x1000u

/* the file a DOS name in the program's memory means, looked up without
 * regard to case below the game's folder */
static uint8_t *host_load(Far name, size_t *size)
{
    char path[1024], next[1024], part[64];
    uint16_t o = 0;
    size_t n = 0;
    uint8_t c;

    /* with DATA:0378 above 0 the name is the string behind the first */
    if ((int16_t)frw(HL_DATA + HL_BASE, 0x0378) > 0)
        while (pb(name, o++))
            ;
    strncpy(path, hl_game_dir, sizeof path - 1);
    path[sizeof path - 1] = 0;
    for (;;) {
        c = pb(name, o++);
        if (c && c != '\\' && c != '/') {
            if (n + 1 < sizeof part)
                part[n++] = (char)c;
            continue;
        }
        part[n] = 0;
        if (n && strcmp(part, ".")) {
            if (!sys_find(path, part, next, sizeof next))
                return NULL;
            strcpy(path, next);
        }
        n = 0;
        if (!c)
            break;
    }
    return sys_load(path, size);
}

/* S2C9E:0002: a far pointer with its offset below 10h */
Far ptr_norm(Far p)
{
    Far r;

    r.seg = (uint16_t)(p.seg + (p.off >> 4));
    r.off = p.off & 0x0F;
    return r;
}

/* what file_load returns: the place, its offset never 0 */
static Far load_result(Far at)
{
    if (at.off == 0) {
        at.off = 0x10;
        at.seg--;
    }
    return at;
}

/* S2C45:0002: loads the file `name` to `to`.  A file that begins with the
 * packer's signature is unpacked when a work buffer of 1000h bytes is
 * given (`work` not 0:0), else it is loaded as it is, like any other
 * file.  Returns the place with its offset made 1 to 10h, or 0:0. */
Far file_load(Far to, Far name, Far work)
{
    Far none = { 0, 0 }, at;
    size_t size, in = 0;
    uint8_t *file = host_load(name, &size);
    uint32_t out, left, end;
    uint16_t pos, i;
    unsigned ctl = 0, bits = 0;

    if (!file)
        return none;
    for (i = 0; i < 8 && i < size; i++)
        fwb(S_LOAD, (uint16_t)(A_HEAD + i), file[i]);
    if ((to.seg == 0 && to.off == 0) || (to.seg == 0xFFFF && to.off == 0xFFFF)) {
        free(file);     /* mem_alloc is not translated yet */
        return none;
    }
    at = ptr_norm(to);
    if (!(work.seg | work.off) || frw(S_LOAD, A_HEAD) != 0x5054 || frw(S_LOAD, A_HEAD + 2) != 0x4D57) {
        /* as it is: the original reads up to the segment's end, then
         * 8000h bytes at a time */
        memcpy(mem + lin(to.seg, to.off), file, size);
        free(file);
        return load_result(at);
    }
    in = 8;
    left = frw(S_LOAD, A_LEFT) | (uint32_t)frw(S_LOAD, A_LEFT + 2) << 16;
    out = lin(at.seg, at.off);
    /* the word behind the data is kept: one byte more than the length is
     * written */
    {
        uint16_t lo = (uint16_t)left, hi = (uint16_t)(left >> 16);
        uint16_t seg = (uint16_t)((uint16_t)(hi >> 4 | hi << 12) + (lo >> 4) + at.seg);
        uint16_t off = (uint16_t)((lo & 0x0F) + at.off);
        end = lin(seg, off);
        fww(S_LOAD, A_KEPT, frw(seg, off));
        fww(S_LOAD, A_END + 2, seg);
        fww(S_LOAD, A_END, off);
    }
    pos = CHUNK + 1;
#define NEXT(v) do { \
        if (pos >= CHUNK) { \
            size_t n = size - in < CHUNK ? size - in : CHUNK; \
            memcpy(mem + lin(work.seg, work.off), file + in, n); \
            in += n; \
            pos = 0; \
        } \
        (v) = mem[lin(work.seg, work.off) + pos]; \
        pos++; \
    } while (0)
#define PUT(b) do { \
        mem[out++] = (uint8_t)(b); \
        if (left-- == 0) \
            goto done; \
    } while (0)
    for (;;) {
        if (!bits) {
            NEXT(ctl);
            bits = 8;
        }
        bits--;
        if (!(ctl & 0x80)) {
            uint8_t b;
            ctl <<= 1;
            NEXT(b);
            PUT(b);
        } else {
            uint8_t b1, b2, n;
            uint32_t from;
            ctl <<= 1;
            NEXT(b1);
            NEXT(b2);
            from = out - (((unsigned)(b1 >> 4) << 8) | b2);
            for (n = (uint8_t)((b1 & 0x0F) + 3); n; n--)
                PUT(mem[from++]);
        }
    }
done:
#undef NEXT
#undef PUT
    fww(S_LOAD, A_LEFT, (uint16_t)left);
    fww(S_LOAD, A_LEFT + 2, (uint16_t)(left >> 16));
    fww(S_LOAD, A_POS, pos);
    mem[end] = (uint8_t)frw(S_LOAD, A_KEPT);
    mem[end + 1] = (uint8_t)(frw(S_LOAD, A_KEPT) >> 8);
    free(file);
    return load_result(at);
}

/* S2B8F:000C: a file's length, of a packed file the unpacked length its
 * header gives; -1 when it cannot be opened.  The file's first eight
 * bytes are left at S2B8F:00C1. */
int32_t file_size(Far name)
{
    size_t size, i;
    uint8_t *file = host_load(name, &size);
    uint16_t seg = HL_S2B8F + HL_BASE;

    if (!file)
        return -1;
    for (i = 0; i < 8 && i < size; i++)
        fwb(seg, (uint16_t)(0x00C1 + i), file[i]);
    free(file);
    if (frw(seg, 0x00C1) == 0x5054 && frw(seg, 0x00C3) == 0x4D57)
        return (int32_t)(frw(seg, 0x00C5) | (uint32_t)frw(seg, 0x00C7) << 16);
    return (int32_t)size;
}

/* S2B81:0004: file_load, then the file's length (file_size) whatever the
 * load returned */
int32_t file_load_size(Far to, Far name, Far work)
{
    file_load(to, name, work);
    return file_size(name);
}
