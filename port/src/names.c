/* names.c - texts the program makes: HL.EXE's file_name (S1571:02CD) and
 * num_text (S2892:0001), translated over the program's memory.
 * port/README.md says how this was compared with the original.
 */
#include "hl.h"

/* the C library's strcpy and strlen as the program has them (CODE:2BAA,
 * CODE:2BD8), on far strings */
static void str_copy(Far to, Far from)
{
    uint16_t i = 0;
    uint8_t c;

    do {
        c = pb(from, i);
        pwb(to, i++, c);
    } while (c);
}

static uint16_t str_len(Far s)
{
    uint16_t n = 0;

    while (pb(s, n))
        n++;
    return n;
}

/* S1571:02CD: puts a file's name together in `name_buf`: with `where` 2
 * the text at `name_prefix` first, then the folder `dir` (a 6-byte entry
 * of `name_dirs`; below 0: none), `name` (0:0: none) and the ending `ext`
 * (a 5-byte entry of `name_exts`; below 0: none).  Returns the buffer. */
Far file_name(uint16_t where, int16_t dir, Far name, int16_t ext)
{
    Far buf, at, part;
    int8_t n = 0;

    buf.seg = S_name_buf;
    buf.off = A_name_buf;
    at = buf;
    part.seg = S_name_buf;
#define ADD(p) do { \
        at.off = (uint16_t)(buf.off + n); \
        str_copy(at, (p)); \
        n = (int8_t)(n + str_len(p)); \
    } while (0)
    if (where == 2) {
        part.off = A_name_prefix;
        ADD(part);
    }
    if (dir >= 0) {
        part.off = (uint16_t)(A_name_dirs + dir * 6);
        ADD(part);
    }
    if (name.seg | name.off)
        ADD(name);
    if (ext >= 0) {
        part.off = (uint16_t)(A_name_exts + ext * 5);
        ADD(part);
    }
#undef ADD
    return buf;
}

/* S2892:0001: `value` (its amount, without a sign) as decimal digits at
 * `to`, `width` places at most 10: the lowest `width` digits when there
 * are more; with 4 in `flags` filled up from the left, with spaces when
 * `flags` has 1, else with zeros.  Returns `to`. */
Far num_text(int32_t value, Far to, int16_t width, uint8_t flags)
{
    uint8_t digits[12], pad = flags & 1 ? 0x20 : 0x30;
    int8_t n = 0;
    uint32_t v = (uint32_t)value;
    uint16_t o = 0;
    int16_t i;

    if (value < 0)
        v = 0u - v;
    if (width > 10)
        width = 10;
    do {
        digits[n++] = (uint8_t)('0' + (int32_t)v % 10);
        v = (uint32_t)((int32_t)v / 10);
    } while (v);
    if (n > width)
        flags &= 0xFB;
    if (flags & 4)
        for (i = 0; width - n > i; i++)
            pwb(to, o++, pad);
    if (n > width)
        n = (int8_t)width;
    while (width-- != 0)
        if (n-- > 0)
            pwb(to, o++, digits[n]);
    pwb(to, o, 0);
    return to;
}
