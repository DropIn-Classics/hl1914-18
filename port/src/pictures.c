/* pictures.c - the battle screen's pictures: HL.EXE's pictures_load
 * (S02E6:2B87) with what it calls that the port did not have yet:
 * lbm_show (S2B06:0000), which draws an IFF picture into the VGA's
 * memory (doskit/runtime/vga.h), lib_read (S2C01:0006), tpwm_unpack
 * (S2CA0:0008) and sfx_load (S2F39:00FD), the last as far as memory goes.
 * port/README.md says how this was compared with the original.
 */
#include <stdlib.h>
#include <string.h>
#include "hl.h"
#include "vga.h"

#define S_PIC (HL_S2B06 + HL_BASE)    /* lbm_show's variables */
#define S_LIBR (HL_S2C01 + HL_BASE)   /* lib_read's buffer at 0230h */
#define S_UNP (HL_S2CA0 + HL_BASE)    /* tpwm_unpack's variables */

/* a byte at seg:off as the CPU sees it: the A000h window is video memory */
static uint8_t scr_rb(uint16_t seg, uint16_t off)
{
    uint32_t a = lin(seg, off);
    return a >= 0xA0000 && a < 0xB0000 ? vga_read((uint16_t)(a - 0xA0000)) : mem[a];
}

static void scr_wb(uint16_t seg, uint16_t off, uint8_t v)
{
    uint32_t a = lin(seg, off);
    if (a >= 0xA0000 && a < 0xB0000)
        vga_write((uint16_t)(a - 0xA0000), v);
    else
        mem[a] = v;
}

static uint16_t swap16(uint16_t v)
{
    return (uint16_t)(v << 8 | v >> 8);
}

/* S2B06:014E: from es:*di a word at a time (REPNE SCASW, 1388h words)
 * for `a` followed by `b`; 1 and *di at `a`, or 0 */
static int chunk_find(uint16_t es, uint16_t *di, uint16_t a, uint16_t b)
{
    uint16_t cx = 0x1388, d = *di;

    for (;;) {
        int hit = 0;
        while (cx) {
            cx--;
            hit = frw(es, d) == a;
            d += 2;
            if (hit)
                break;
        }
        if (!cx)
            return 0;
        if (frw(es, d) == b) {
            *di = (uint16_t)(d - 2);
            return 1;
        }
    }
}

/* the four pixels a byte: the map mask turned on, the address on after
 * the fourth */
#define MASK_NEXT(al, di) do { \
        int c_ = (al) >> 7; \
        (al) = (uint8_t)((al) << 1 | c_); \
        if (c_) (di)++; \
    } while (0)

/* S2B06:01AC: a PBM's lines, ByteRun1 */
static void draw_pbm(uint16_t es, Far body, uint16_t w, uint16_t h, uint16_t pitch)
{
    uint16_t si = (uint16_t)(body.off + 8), ds = body.seg, row = (uint16_t)-pitch, y;

    vga_outb(0x3C4, 2);
    for (y = 0; y < h; y++) {
        uint16_t cx = w, di;
        uint8_t al = 0x11;
        row = (uint16_t)(row + pitch);
        di = row;
        for (;;) {
            uint8_t bl = frb(ds, si++), n, v = 0;
            int run = bl & 0x80;
            n = run ? (uint8_t)(-bl + 1) : (uint8_t)(bl + 1);
            if (run)
                v = frb(ds, si++);
            for (; n; n--) {
                vga_outb(0x3C5, al);
                scr_wb(es, di, run ? v : frb(ds, si++));
                MASK_NEXT(al, di);
                if (!--cx)
                    goto next;
            }
        }
next:;
    }
}

/* S2B06:0232: an ILBM's lines, each plane's bits ORed in as the plane
 * bit at S2B06:03F2 */
static void draw_ilbm(uint16_t es, Far body, uint16_t w, uint16_t h, uint16_t planes, uint16_t pitch)
{
    uint16_t si = (uint16_t)(body.off + 8), ds = body.seg, row = (uint16_t)-pitch, y, p;

    vga_outb(0x3C4, 2);
    for (y = 0; y < h; y++) {
        uint8_t bit = 0x80;
        row = (uint16_t)(row + pitch);
        for (p = (uint16_t)(planes + 1);; ) {
            uint16_t cx = w, di = row;
            bit = (uint8_t)(bit << 1 | bit >> 7);
            fwb(S_PIC, 0x03F2, bit);
            if (!--p)
                break;
            for (;;) {
                uint8_t bl = frb(ds, si++), n, v = 0;
                int run = bl & 0x80;
                n = run ? (uint8_t)(-bl + 1) : (uint8_t)(bl + 1);
                if (run)
                    v = frb(ds, si++);
                for (; n; n--) {
                    uint8_t ah = run ? v : frb(ds, si++), al = 0x11;
                    int k;
                    for (k = 0; k < 8; k++) {
                        vga_outb(0x3C5, al);
                        if (ah & 0x80)
                            scr_wb(es, di, (uint8_t)(scr_rb(es, di) | bit));
                        ah = (uint8_t)(ah << 1);
                        MASK_NEXT(al, di);
                        if (!--cx)
                            goto plane_end;
                    }
                }
            }
plane_end:;
        }
    }
}

/* S2B06:02E9: a picture larger than the screen, unpacked to es:0 as bytes,
 * (width + 7) / 8 a line */
static void draw_big(uint16_t es, Far body, uint16_t w, uint16_t h)
{
    uint32_t from = lin(body.seg, (uint16_t)(body.off + 8)), to = lin(es, 0);
    int16_t line = (int16_t)((uint16_t)(w + 7) >> 3), done = 0, y = 0;

    do {
        int8_t c = (int8_t)mem[from++];
        uint16_t n = (uint16_t)(c >= 0 ? c + 1 : -c + 1), k;
        uint8_t v = c < 0 ? mem[from++] : 0;
        for (k = 0; k < n; k++) {
            uint8_t b = c >= 0 ? mem[from++] : v;
            if (to >= 0xA0000 && to < 0xB0000)
                vga_write((uint16_t)(to - 0xA0000), b);
            else
                mem[to] = b;
            to++;
        }
        done = (int16_t)(done + n);
        if (done >= line) {
            done = 0;
            y++;
        }
    } while (y < (int16_t)h);
}

/* S2B06:0000 (lbm_show): the IFF picture `name` loaded to `place` and
 * drawn at screen_seg:0.  1, or 0 when the file cannot be loaded (the
 * original's FFFF:FFFF and 0:0). */
int lbm_show(Far place, Far name, Far work)
{
    uint16_t pitch = NW(screen_pitch, 0), es, di, w, h, comp, planes;
    uint32_t p = ((uint32_t)place.seg << 16 | place.off) + 1;
    Far at, bmhd, cmap;

    fww(S_PIC, 0x03F3, pitch);
    work = ptr_norm(work);
    name = ptr_norm(name);
    place.seg = (uint16_t)(p >> 16);
    place.off = (uint16_t)(p & 0xFFFE);
    at = file_load(place, name, work);
    /* (with place 0:0 the original calls S2CC5:0004 at its end; not read) */
    if (!at.seg)
        return 0;
    es = at.seg;
    di = at.off;
    fww(S_PIC, 0x03DC, 0xFFFF);
    if (!chunk_find(es, &di, 0x4250, 0x204D)) {          /* "PBM " */
        di = at.off;
        if (!chunk_find(es, &di, 0x4C49, 0x4D42))        /* "ILBM" */
            return 0;
        fww(S_PIC, 0x03DC, 0);
    }
    if (!chunk_find(es, &di, 0x4D42, 0x4448))            /* "BMHD" */
        return 0;
    fww(S_PIC, 0x03DE, di);
    fww(S_PIC, 0x03E0, es);
    if (!chunk_find(es, &di, 0x4D43, 0x5041))            /* "CMAP" */
        return 0;
    fww(S_PIC, 0x03E8, es);
    fww(S_PIC, 0x03E6, di);
    di = 0;
    if (!chunk_find(es, &di, 0x4F42, 0x5944))            /* "BODY" */
        return 0;
    fww(S_PIC, 0x03E4, es);
    fww(S_PIC, 0x03E2, di);
    /* S2B06:0164: the header */
    bmhd = far_at(S_PIC, 0x03DE);
    w = swap16(pw(bmhd, 8));
    h = swap16(pw(bmhd, 0x0A));
    comp = (uint16_t)(int16_t)(int8_t)pb(bmhd, 0x12);
    planes = (uint16_t)(int16_t)(int8_t)pb(bmhd, 0x10);
    fww(S_PIC, 0x03EA, w);
    fww(S_PIC, 0x03EC, h);
    fww(S_PIC, 0x03EE, comp);
    fww(S_PIC, 0x03F0, planes);
    /* S2B06:0190: the colours to `palette` */
    cmap = far_at(S_PIC, 0x03E6);
    {
        uint16_t n = swap16(pw(cmap, 6)), i;
        for (i = 0; i < n; i++)
            SETB(palette, i, pb(cmap, (uint16_t)(8 + i)));
    }
    es = NW(screen_seg, 0);
    if ((int16_t)w > 0x168 && (int16_t)h > 0xF0) {
        if (comp == 1)
            draw_big(es, far_at(S_PIC, 0x03E2), w, h);
    } else if (frw(S_PIC, 0x03DC)) {
        if (comp == 1)
            draw_pbm(es, far_at(S_PIC, 0x03E2), w, h, pitch);
    } else if (comp == 1)
        draw_ilbm(es, far_at(S_PIC, 0x03E2), w, h, planes, pitch);
    return 1;
}

/* S2CA0:0008 (tpwm_unpack): the packer's data at `from` unpacked to `to`;
 * the length, or -1 without the signature */
int32_t tpwm_unpack(Far from, Far to)
{
    uint32_t s, d, len, left;
    int is_short;
    uint16_t kept = 0;
    Far end = { 0, 0 };

    from = ptr_norm(from);
    to = ptr_norm(to);
    if (pw(from, 0) != 0x5054 || pw(from, 2) != 0x4D57)
        return -1;
    len = pw(from, 4) | (uint32_t)pw(from, 6) << 16;
    fww(S_UNP, 0x01C5, (uint16_t)len);
    fww(S_UNP, 0x01C7, (uint16_t)(len >> 16));
    s = lin(from.seg, from.off) + 8;
    d = lin(to.seg, to.off);
    is_short = !(len >> 16) && (uint32_t)to.off + len < 0x10000;
    if (is_short) {
        end.seg = to.seg;
        end.off = (uint16_t)(to.off + len);
        kept = pw(end, 0);
        fww(S_UNP, 0x01CB, end.seg);
        fww(S_UNP, 0x01C9, end.off);
        fww(S_UNP, 0x01CD, kept);
    } else {
        fww(S_UNP, 0x01C1, (uint16_t)len);
        fww(S_UNP, 0x01C3, (uint16_t)(len >> 16));
    }
    /* both write one byte more than the length */
    left = len + 1;
    for (;;) {
        uint8_t ctl = mem[s++];
        int k;
        for (k = 0; k < 8; k++, ctl = (uint8_t)(ctl << 1)) {
            if (!(ctl & 0x80)) {
                mem[d++] = mem[s++];
                if (!--left)
                    goto done;
            } else {
                uint8_t b1 = mem[s++], b2 = mem[s++], n = (uint8_t)((b1 & 0x0F) + 3);
                uint32_t f = d - ((uint32_t)(b1 >> 4) << 8 | b2);
                for (; n; n--) {
                    mem[d++] = mem[f++];
                    if (!--left)
                        goto done;
                }
            }
        }
    }
done:
    if (is_short) {
        fww(end.seg, end.off, kept);
    } else {
        fww(S_UNP, 0x01C1, 0xFFFF);
        fww(S_UNP, 0x01C3, 0xFFFF);
    }
    return (int32_t)len;
}

/* lib_read's DOS reads: up to `n` bytes at *pos to S2C01:0230 */
static size_t lr_read(const uint8_t *file, size_t size, size_t *pos, size_t n)
{
    size_t k = *pos < size ? (size - *pos < n ? size - *pos : n) : 0, i;

    for (i = 0; i < k; i++)
        fwb(S_LIBR, (uint16_t)(0x0230 + i), file[*pos + i]);
    *pos += k;
    return k;
}

static uint32_t lr_dword(uint16_t o)
{
    return frw(S_LIBR, o) | (uint32_t)frw(S_LIBR, (uint16_t)(o + 2)) << 16;
}

/* a far pointer's offset moved on by `n` with the carry added to its
 * segment as a word (ADD, ADC on the dword) */
static Far dword_add(Far p, uint16_t n)
{
    uint32_t v = ((uint32_t)p.seg << 16 | p.off) + n;
    Far r;

    r.seg = (uint16_t)(v >> 16);
    r.off = (uint16_t)v;
    return r;
}

/* S2C01:0006 (lib_read): the entries `names` (eight bytes each, ended by
 * an empty name) of the library `file` one behind the other to `place`,
 * their far pointers to `ptrs` and 0:0 behind them.  Returns 0 and
 * *place moved on, or -1 (the original's FFFF:FFFF). */
int lib_read(Far *place, Far file, Far names, Far ptrs, Far work)
{
    size_t size, pos = 0;
    uint8_t *data = host_load(file, &size);
    uint32_t table, start, end;
    uint16_t len;

    if (!data)
        return -1;
    lr_read(data, size, &pos, 4);
    table = lr_dword(0x0230);
    for (;;) {
        pos = table;
        if (!pb(names, 0))
            break;
        do {
            if (lr_read(data, size, &pos, 12) != 12)
                goto fail;
        } while (memcmp(mem + lin(S_LIBR, 0x0230), mem + lin(names.seg, names.off), 8));
        start = lr_dword(0x0238);
        end = lr_read(data, size, &pos, 12) == 12 ? lr_dword(0x0238) : table;
        len = (uint16_t)(end - start);
        pos = start;
        if (work.seg) {
            lr_read(data, size, &pos, 4);
            pos = start;
        }
        if (work.seg && frw(S_LIBR, 0x0230) == 0x5054 && frw(S_LIBR, 0x0232) == 0x4D57) {
            int32_t r;
            if (pos + len > size)
                goto fail;
            memcpy(mem + lin(work.seg, work.off), data + pos, len);
            pos += len;
            r = tpwm_unpack(work, *place);
            if (r == -1)
                goto fail;
            len = (uint16_t)r;
        } else {
            if (pos + len > size)
                goto fail;
            memcpy(mem + lin(place->seg, place->off), data + pos, len);
            pos += len;
        }
        fww(ptrs.seg, ptrs.off, place->off);
        fww(ptrs.seg, (uint16_t)(ptrs.off + 2), place->seg);
        ptrs = dword_add(ptrs, 4);
        *place = ptr_norm(dword_add(*place, len));
        names = ptr_norm(dword_add(names, 8));
    }
    free(data);
    fww(ptrs.seg, ptrs.off, 0);
    fww(ptrs.seg, (uint16_t)(ptrs.off + 2), 0);
    return 0;
fail:
    free(data);
    return -1;
}

/* S2F39:00FD (sfx_load): the sound effects' file loaded to `place`, kept
 * at sfx_data and sfx_size; its length, or -1 */
int32_t sfx_load(Far place, Far name, Far work)
{
    Far at = file_load(place, name, work);
    int32_t n;

    if (!(at.seg | at.off))
        return -1;
    SETW(sfx_data, 0, at.off);
    SETW(sfx_data, 2, at.seg);
    n = file_size(name);
    SETW(sfx_size, 0, (uint16_t)n);
    SETW(sfx_size, 2, (uint16_t)((uint32_t)n >> 16));
    return n;
}

static Far game_text(uint16_t off)
{
    Far p;

    p.seg = HL_S331F + HL_BASE;
    p.off = off;
    return p;
}

#define SIDE_SIZE 0x17u     /* a record of `sides` */

static void set_far(uint16_t seg, uint16_t off, Far p)
{
    fww(seg, off, p.off);
    fww(seg, (uint16_t)(off + 2), p.seg);
}
#define SET_FAR(name, p) set_far(S_##name, A_##name, p)

/* a lib_load of the record at S331F:rec, list ending 3; its length added */
static int pl_lib(uint16_t rec, Far *place, Far work)
{
    Far r = game_text(rec);

    if (lib_load(file_name(1, 0, r, 3), *place, work, r, 0) == -1)
        return -1;
    *place = far_add(*place, pw(r, 0x0A) | (uint32_t)pw(r, 0x0C) << 16);
    return 0;
}

/* a file_load_size of `name`; its length added */
static int pl_file(Far name, Far *place, Far work, Far *at)
{
    int32_t n = file_load_size(*place, name, work);

    if (n == -1)
        return -1;
    *at = *place;
    *place = far_add(*place, (uint32_t)n);
    return 0;
}

/* S02E6:2B87 (pictures_load): what the battle's screen needs, loaded to
 * *place, which moves on; `pal` is a buffer for the palette's file.
 * Returns 0; -1 where the original returns FFFF:FFFF (*place made so);
 * 2 where it goes to its error exit (S0CB6:1842 with 2). */
int pictures_load(Far *place, Far work, Far pal)
{
    uint16_t s331f = HL_S331F + HL_BASE;
    uint16_t bx, si, r;
    uint8_t digit = '2';
    Far at, rec;

    lbm_show(*place, file_name(1, -1, game_text(A_pict_name), 0x0A), work);
    at = file_load(pal, file_name(1, -1, num_text(frb(HL_S37FF + HL_BASE, 0x0112), game_text(A_num_buf), 2, 4), 4),
                   work);
    if (!(at.seg | at.off))
        return 2;
    far_copy(pal, far_at(S_palette_ptr, A_palette_ptr), 0x300);
    if (pl_lib(A_lib_rnd, place, work)) {
        place->seg = place->off = 0xFFFF;
        return -1;
    }
    if (pl_lib(0x01E5, place, work))
        return 2;
    if (pl_lib(0x01A9, place, work)) {
        place->seg = place->off = 0xFFFF;
        return -1;
    }
    if (pl_file(file_name(1, 0, game_text(A_bigunit_name), 5), place, work, &at))
        return 2;
    SET_FAR(bigunit_dat, at);
    SET_FAR(bigunit_buf, *place);
    *place = far_add(*place, 0x1194);
    SET_FAR(bigunit_ptrs, *place);
    *place = far_add(*place, 0x1194);
    {
        /* lib_read gets a copy: the place moves on by F3Ch bytes below,
         * not by what it read */
        Far ptrs = far_at(S_bigunit_ptrs, A_bigunit_ptrs), to = *place;
        if (lib_read(&to, file_name(1, 0, game_text(A_bigunit_name), 3), game_text(A_bigunit_list), ptrs,
                     far_add(ptrs, 0x10)))
            return 2;
    }
    SET_FAR(bigunit_pics, *place);
    *place = far_add(*place, 0xF3C);
    if (pl_lib(0x01F9, place, work))
        return 2;
    SETB(umap_name, 6, 'T');
    if (pl_file(file_name(1, 0, game_text(A_umap_name), 5), place, work, &at))
        return 2;
    SET_FAR(umap_dat, at);
    SETB(umap_name, 6, NB(pics_winter, 0) ? 'W' : 'S');
    SETB(umap2_name, 6, NB(pics_winter, 0) ? 'W' : 'S');
    rec = game_text(frw(s331f, 0x0008) == 1 ? A_umap2_name : A_umap_name);
    if (pl_file(file_name(1, 0, rec, 3), place, work, &at))
        return 2;
    SET_FAR(umap_lib, at);
    /* the music's digit by the ratio of the sides' unit counts */
    bx = (uint16_t)(NB(sides, 2) + 1);
    si = (uint16_t)(NB(sides, SIDE_SIZE + 2) + 1);
    if ((NW(sides, 0) & 2) != (NW(sides, SIDE_SIZE) & 2) && (NW(sides, 0) & 2)) {
        r = bx;
        bx = si;
        si = r;
    }
    r = (uint16_t)((uint16_t)(bx << 8) / si);
    if (r > 0x140)
        digit = '1';
    if (r < 0xC0)
        digit = '3';
    SETB(music_name, 6, digit);
    if (file_load_size(*place, game_text(A_music_name), work) == -1)
        return 2;
    SET_FAR(music_data, *place);
    *place = far_add(*place, 0x213A);
    {
        int32_t n = sfx_load(*place, game_text(A_sfx_name), work);
        if (n == -1)
            return 2;
        *place = far_add(*place, (uint32_t)n);
    }
    return 0;
}
