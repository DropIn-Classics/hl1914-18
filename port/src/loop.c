/* loop.c - a pass of HL.EXE's game_loop (S066A:0EE9 to 45D7): both sides'
 * parts by the state of their views, the round's end and the timers,
 * translated over the program's memory without what draws, sounds and
 * waits.  What the original reads from the keyboard handler comes in as a
 * SideIn, which port/tests/loop_dumps.py logs from the original; the
 * computer's side is played by computer_turn (port/src/plan.c), or with
 * loop_replay set by what the original's was logged to give.  The port's
 * own program (port/src/play.c) sets loop_hook and has the input in the
 * program's memory, where its own translation of the interrupts
 * (port/src/input.c) leaves it.
 * port/README.md says how this was compared with the original.
 */
#include "hl.h"

#define SIDE_SIZE 0x17u     /* a record of `sides` */
#define VIEW_SIZE 0x31u     /* a record of `views` */
#define SITE_SIZE 0x1Cu     /* a record of `carriers` and the like */
#define S_DATA (HL_DATA + HL_BASE)
#define S_COMP (HL_S39B6 + HL_BASE)

/* game_loop's locals that live from pass to pass: the key ([BP-3Bh]) and
 * whether the texts 52h/3Fh of a waiting side were shown ([BP-28h]) */
static uint8_t key;
static uint16_t told;
static const char *missing;

/* the record of input the computer's part passes to view_step
 * ([BP-36h] of game_loop, on its stack: 3EA4h:0FB4h in the dumps) */
#define FAKE_SEG 0x3EA4u
#define FAKE_OFF 0x0FB4u

static Far view_of(uint16_t side)
{
    Far v;
    v.seg = S_views;
    v.off = (uint16_t)(A_views + side * VIEW_SIZE);
    return v;
}

static void pww(Far p, uint16_t o, uint16_t v)
{
    fww(p.seg, (uint16_t)(p.off + o), v);
}

static uint16_t gs(uint16_t o) { return NW(game_state, o); }
static void gs_or(uint16_t o, uint16_t v) { SETW(game_state, o, gs(o) | v); }
static void gs_and(uint16_t o, uint16_t v) { SETW(game_state, o, gs(o) & v); }

static uint16_t side_word(uint16_t side) { return NW(sides, side * SIDE_SIZE); }

/* a test's option: the computer's side not played by computer_turn but
 * replayed from SideIn (word, text, set80, set8), as before computer_turn
 * was translated */
int loop_replay;

/* the program's own play: called at the places of a pass where the
 * original reads what its interrupts left meanwhile or where its clock
 * matters (LOOP_PART and the others, hl.h).  With it set a human's input
 * record is the memory's and not written from SideIn; the routine may
 * change the SideIn of the pass under way. */
int loop_abort;
#ifndef PORT_VERSION  /* dev-only debug keys: absent from release builds */
int loop_cheat;
#endif
void (*loop_hook)(int at, uint16_t side);

static Far no_save(void)
{
    Far p = { 0, 0 };

    return p;
}

/* where a side's cursor on the small map keeps what it covers: the far
 * pointer at byte 5 of its record of `sides` (a place in video memory) */
static Far side_save(uint16_t side)
{
    Far p;

    p.off = NW(sides, side * SIDE_SIZE + 5);
    p.seg = NW(sides, side * SIDE_SIZE + 7);
    return p;
}

/* a side's view, its marks and its cursor drawn */
static void view_redraw(uint16_t side)
{
    view_draw(NW(views, side * VIEW_SIZE + 2), side);
    marks_draw(NW(views, side * VIEW_SIZE + 2), side);
    cursor_draw(side, no_save());
}

static uint16_t phase_word(uint16_t side)
{
    return frw(S_COMP, (uint16_t)(A_computer_phase + side * 0x0C));
}

/* the computer's turn in a pass (S066A:1020 to 1190): once, or with 80h
 * in game_state's word 2 again and again while it thinks (the phases 20h
 * and 40h) until a human's button or a key stops it; that comes in as the
 * number of calls the original made (in->turns, below 0 not known: until
 * the thinking is through).  Returns the side's input word; the text is
 * shown. */
static uint16_t computer_part(uint16_t side, const SideIn *in)
{
    if (gs(2) & 0x80) {
        Far r0 = far_at(S_DATA, 0x00A6), r1 = far_at(S_DATA, 0x00B6);
        int n = 0;

        /* the original also clears word 4 behind DATA:00BE (the keys) */
        pwb(r0, 4, 0);
        pwb(r1, 4, 0);
        if (loop_hook) {
            Far k = far_at(S_DATA, 0x00BE);

            pww(k, 4, 0);
        }
        draw_on_shown();
        show_message(0x64, side);
        for (;;) {
            computer_turn(side);
            n++;
            if (loop_hook)
                loop_hook(LOOP_THINK, side);
            if (!(phase_word(side) & 0x60))
                break;
            if (in->turns >= 0 && n >= in->turns)
                break;
        }
        if (in->turns >= 0 && n != in->turns)
            missing = "the computer's thinking through after another number of calls than the original's";
        show_message(0xFFFF, 0);
        show_message(0xFFFF, 1);
        gs_and(0, 0xFFF3);
        draw_on_drawn();
        gs_and(2, 0xFF7F);
    } else
        computer_turn(side);
    show_message(NW(computer_out, side * 4 + 2), side);
    /* S066A:118E: while it still thinks, the bar of how far it got */
    if ((phase_word(side) & 0x60) && NB(views, side * VIEW_SIZE + 0x16) == 1)
        bar_draw((uint16_t)((side & 1 ? 0xA0 : 0) + 0x17), 0xC3, frb(HL_S331F + HL_BASE, 0x4C85), frb(HL_S331F + HL_BASE, 0x4C87),
                 1, (int16_t)(computer_progress(side) << 1), 0xC8);
    return NW(computer_out, side * 4);
}

/* the human side's input record (S066A:0F0C to 0FC3): DATA:00A6 or 00B6
 * by the side, exchanged with 2 clear in game_state's word 2 for the maps
 * 18h to 2Fh */
static Far input_rec(uint16_t side)
{
    int b = side & 1;

    if (!(gs(2) & 2)) {
        int16_t map = (int16_t)NW(map_number, 0);

        if (map > 0x17 && map < 0x30)
            b = !b;
    }
    return far_at(S_DATA, b ? 0x00B6 : 0x00A6);
}

/* a place's x and y in a site's or carrier's list (S066A:2777 and alike) */
static void place_xy(Far view, uint16_t side)
{
    int16_t row = (int16_t)pw(view, 0x0E);

    pww(view, 0x10, (uint16_t)(row * 0x11 + 0x0D + (side ? 0xA0 : 0)));
    pww(view, 0x12, (uint16_t)(0x8B - (row % 2) * 0x0C));
}

static void effect(uint16_t e)
{
    if (e != 0xFF) {
        gs_or(4, 0x100);
        SETW(site_taken, 0, e);
    }
}

/* S066A:12A4 to 14BA: with no direction in state 0, the unit on the
 * cursor's field: the mark of its aim (a timer of kind 5; the mark in
 * field_bits is only there while field_draw draws it), its panel in
 * the status line (unit_panel) and a timer of kind 1 or 2 */
static void hover(Far view, uint16_t side)
{
    Far map = far_at(S_map_cur, A_map_cur);
    uint8_t u = pb(map, (uint16_t)(pw(view, 0) + 1));
    uint16_t ok = 1, f;

    if (u > 0xF0)
        return;
    if (NW(units, u * UNIT_SIZE + 6) & 4)
        ok = 0;
    if (NW(units, u * UNIT_SIZE + 6) & 2) {
        f = NW(unit_flags, u * UNIT_SIZE);
        if (side & 1 ? !(f & 1) : f & 1)
            ok = 0;
    }
    if (gs(0) & (side ? 8 : 4))
        ok = 0;
    if (!ok)
        return;
    f = NW(unit_flags, u * UNIT_SIZE);
    if (f & 0x40 && !(f & 0x80))
        u--;
    {
        uint16_t aim = NW(unit_aim, u * UNIT_SIZE);

        if (pb(view, 0x16) & 2 && aim && same_side(side, NW(unit_flags, u * UNIT_SIZE))) {
            int16_t p = (int16_t)aim >> 1, w = (int16_t)NW(map_width, 0);
            uint16_t bit = (uint16_t)(p % w + (p / w << 6));
            uint8_t was = NB(field_bits, bit);

            timer_set(5, 1, aim);
            SETB(field_bits, bit, was | (side ? 8 : 4));
            field_draw(side, aim);
            SETB(field_bits, bit, was);
        }
    }
    unit_panel(u, side);
    timer_set(side ? 2 : 1, 5, 0);
}

/* S066A:1C78 to 1F39: the cursor moved by view_step's input bits, the
 * view scrolled at its edges (2 in game_state's word 4 asks for drawing) */
static void cursor_move(Far view, uint16_t side, uint16_t di)
{
    int16_t w4 = (int16_t)(NW(map_width, 0) << 2);
    int16_t col = (int16_t)pw(view, 0x0C), row = (int16_t)pw(view, 0x0E);
    uint8_t moved = 0;      /* [BP-41h]: 1 up, 2 down, 4 left, 8 right */

    if (di & 4) {
        if (col & 1) {
            if (di & 2)
                moved |= 2, row++;
        } else if (di & 1)
            moved |= 1, row--;
        moved |= 4;
        col--;
    } else if (di & 8) {
        if (col & 1) {
            if (di & 2)
                moved |= 2, row++;
        } else if (di & 1)
            moved |= 1, row--;
        moved |= 8;
        col++;
    } else if (di & 1)
        moved |= 1, row--;
    else if (di & 2)
        moved |= 2, row++;
    if (row < 1) {
        if ((int8_t)pb(view, 0x15) > 0) {
            gs_or(4, 2);
            pww(view, 2, (uint16_t)(pw(view, 2) - w4));
            pwb(view, 0x15, (uint8_t)(pb(view, 0x15) - 1));
            row++;
        }
        if (row < 0) {
            moved &= 0xFE;
            row = 0;
        }
    } else if (row >= 6) {
        if (pb(view, 0x15) < NB(game_state, 0x0E)) {
            gs_or(4, 2);
            pww(view, 2, (uint16_t)(pw(view, 2) + w4));
            pwb(view, 0x15, (uint8_t)(pb(view, 0x15) + 1));
            row--;
        }
        if (row > 6) {
            moved &= 0xFD;
            row = 6;
        }
    }
    if (col < 1) {
        if ((int8_t)pb(view, 0x14) > 0) {
            gs_or(4, 2);
            pww(view, 2, (uint16_t)(pw(view, 2) - 4));
            pwb(view, 0x14, (uint8_t)(pb(view, 0x14) - 1));
            col++;
        }
        if (col < 0) {
            moved &= 0xFB;
            col = 0;
        }
    } else if (col >= 8) {
        if (pb(view, 0x14) < NB(game_state, 0x0D)) {
            gs_or(4, 2);
            pww(view, 2, (uint16_t)(pw(view, 2) + 4));
            pwb(view, 0x14, (uint8_t)(pb(view, 0x14) + 1));
            col--;
        }
        if (col > 8) {
            moved &= 0xF7;
            col = 8;
        }
    }
    pww(view, 0x0C, (uint16_t)col);
    pww(view, 0x0E, (uint16_t)row);
    /* S066A:1E97: the cursor's step sounds, with the effects on */
    if (moved && gs(0) & 0x80)
        sfx_play(side ? 9 : 0x0B, side ? 5 : 4, 0xDC, 1, 0x200);
    cursor_field(view, side);       /* S066A:1EE2 */
    view_cursor(side);
    /* the view scrolled: drawn again (S066A:1F02) */
    if (gs(4) & 2) {
        view_draw(pw(view, 2), side);
        marks_draw(pw(view, 2), side);
    }
    gs_and(4, 0xFFFD);
}

/* back to state 0 from a state that showed something (S066A:2118, 248E):
 * step 19h, the plain pointer, the cursor where it was */
static void back_to_map(Far view, uint16_t side, Far rec)
{
    pwb(view, 0x17, 0);
    pwb(view, 0x18, 0x19);
    pwb(view, 0x1B, 5);
    pww(view, 0x0C, pw(view, 4));
    pww(view, 0x0E, pw(view, 6));
    view_cursor(side);
    pwb(rec, 8, 0);
}

/* state 3, the whole map small (S066A:1F53 to 22FD) */
static void state_map(Far view, uint16_t side, uint16_t di, Far rec)
{
    uint16_t sh = frw(S_game_state, 0x0008);   /* S331F:0008 */

    if (pb(view, 0x18) == 4) {
        uint16_t w = NW(map_width, 0), h = NW(map_height, 0);

        pww(view, 0x10, side & 1 ? 0xA0 : 0);
        if (sh == 2) {
            pww(view, 0x10, (uint16_t)(pw(view, 0x10) + 0x50 - (w * 2 + 2)));
            pww(view, 4, pw(view, 0x10));
            pww(view, 0x12, (uint16_t)(0x64 - (h * 2 + 2)));
        } else {
            pww(view, 0x10, (uint16_t)(pw(view, 0x10) + 0x50 - (w + 2)));
            pww(view, 4, pw(view, 0x10));
            pww(view, 0x12, (uint16_t)(0x64 - (h + 2)));
        }
        pww(view, 6, pw(view, 0x12));
        pww(view, 8, pw(view, 0x0C));
        pww(view, 0x0A, pw(view, 0x0E));
        pww(view, 0x0C, (uint16_t)((int8_t)pb(view, 0x14) * 2));
        pww(view, 0x0E, (uint16_t)((int8_t)pb(view, 0x15) * 2));
        pww(view, 0x10, (uint16_t)(pw(view, 0x10) + (pw(view, 0x0C) << sh)));
        pww(view, 0x12, (uint16_t)(pw(view, 0x12) + (pw(view, 0x0E) << sh)));
        small_map_back(side);
        small_map_draw(side, pw(view, 4), pw(view, 6));
        SETW(sides, side * SIDE_SIZE, side_word(side) | 4);
        gs_or(4, 0x100);
        SETW(site_taken, 0, 0);
        pwb(view, 0x2A, 1);
        pwb(view, 0x29, 1);
        pwb(view, 0x28, 1);
        pwb(view, 0x18, 5);
    } else if (di & 0x10) {
        pwb(view, 0x17, 0);
        pwb(view, 0x18, 0x19);
        pwb(view, 0x1B, 5);
        pwb(view, 0x14, (uint8_t)((int16_t)pw(view, 0x0C) >> 1));
        pwb(view, 0x15, (uint8_t)((int16_t)pw(view, 0x0E) >> 1));
        pww(view, 2, (uint16_t)(pw(view, 0x0C) * 2 + pw(view, 0x0E) * NW(map_width, 0) * 2));
        pointer_erase(side_save(side));
        SETW(sides, side * SIDE_SIZE, side_word(side) & 0xFFFB);
        view_draw(pw(view, 2), side);
        marks_draw(pw(view, 2), side);
        pww(view, 4, pw(view, 8));
        pww(view, 0x0C, pw(view, 8));
        pww(view, 6, pw(view, 0x0A));
        pww(view, 0x0E, pw(view, 0x0A));
        view_cursor(side);
        pwb(view, 0x28, 1);
        pwb(view, 0x29, 2);
        pwb(view, 0x2A, 1);
        pwb(rec, 8, 0);
    } else {
        int16_t step = (int16_t)(sh << 2);

        if (di & 1) {
            if ((int16_t)pw(view, 0x0E) > 0) {
                pww(view, 0x0E, (uint16_t)(pw(view, 0x0E) - 2));
                pww(view, 0x12, (uint16_t)(pw(view, 0x12) - step));
            }
        } else if (di & 2) {
            if ((int16_t)pw(view, 0x0E) < (int16_t)NW(map_height, 0) - 8) {
                pww(view, 0x0E, (uint16_t)(pw(view, 0x0E) + 2));
                pww(view, 0x12, (uint16_t)(pw(view, 0x12) + step));
            }
        }
        if (di & 4) {
            if ((int16_t)pw(view, 0x0C) > 0) {
                pww(view, 0x0C, (uint16_t)(pw(view, 0x0C) - 2));
                pww(view, 0x10, (uint16_t)(pw(view, 0x10) - step));
            }
        } else if (di & 8) {
            if ((int16_t)pw(view, 0x0C) < (int16_t)NW(map_width, 0) - 10) {
                pww(view, 0x0C, (uint16_t)(pw(view, 0x0C) + 2));
                pww(view, 0x10, (uint16_t)(pw(view, 0x10) + step));
            }
        }
        pointer_erase(side_save(side));
    }
}

/* state 4, what is on the cursor's field (S066A:2300 to 24FF) */
static void state_field(Far view, uint16_t side, uint16_t di, Far rec)
{
    if (pb(view, 0x18) == 0) {
        Far map = far_at(S_map_cur, A_map_cur);
        uint8_t u = pb(map, (uint16_t)(pw(view, 0) + 1));

        if (u <= 0xF0) {
            int ok = 1;

            if (NW(units, u * UNIT_SIZE + 6) & 2 && !same_side(NW(unit_flags, u * UNIT_SIZE), side)) {
                ok = 0;
                u = 0xFF;
            }
            if (ok) {
                uint16_t f;

                pwb(view, 0x18, 6);
                pwb(view, 0x1B, 1);
                pww(view, 4, pw(view, 0x0C));
                pww(view, 6, pw(view, 0x0E));
                pww(view, 0x10, side & 1 ? 0x75 + 0xA0 : 0x75);
                pww(view, 0x12, 0x87);
                f = NW(unit_flags, u * UNIT_SIZE);
                if (f & 0x40 && !(f & 0x80))
                    u--;
                pww(view, 0x1E, u);
                unit_info(side, pw(view, 0), u, map);
                u = 0;
            }
        }
        if (u) {
            pwb(view, 0x18, 7);
            pwb(view, 0x1B, 1);
            pww(view, 4, pw(view, 0x0C));
            pww(view, 6, pw(view, 0x0E));
            battle_info(side);
        }
        gs_or(4, 0x100);
        SETW(site_taken, 0, 0);
    }
    if (di & 0x10) {
        back_to_map(view, side, rec);
        /* S066A:24DB: the view over what was shown, and its marks */
        view_draw(pw(view, 2), side);
        marks_draw(pw(view, 2), side);
    }
}

/* state 6, into what stands on the cursor's field (S066A:2502 to 26EB) */
static void state_enter(Far view, uint16_t side)
{
    Far map = far_at(S_map_cur, A_map_cur);
    uint16_t g = frw(S_ww1gnds_dat, (uint16_t)(A_ww1gnds_dat + pb(map, pw(view, 0)) * 6));
    uint16_t u = pb(map, (uint16_t)(pw(view, 0) + 1)), n;
    Far table;

    table.seg = S_carriers;
    pwb(view, 0x17, 2);
    pwb(view, 0x18, 9);
    if (g & 0x200 || g & 0x80) {
        table.off = g & 0x200 ? A_sites_400 : A_sites_100;
        n = site_find(pw(view, 0), table);
        pww(view, 0x1E, n);
        table.off = (uint16_t)(table.off + n * SITE_SIZE);
        pww(view, 0x22, 1);
    } else if (g & 0x40) {
        pww(view, 0x22, 1);
        n = same_side(side, g) ? side : (side ? 0 : 1);
        pww(view, 0x1E, n);
        table.off = (uint16_t)(A_sites_40 + n * SITE_SIZE);
    } else {
        if (NW(unit_flags, u * UNIT_SIZE) & 0x2000)
            u--;
        n = (uint16_t)(int16_t)(int8_t)NB(units, u * UNIT_SIZE + 0x0A);
        pww(view, 0x1E, n);
        table.off = (uint16_t)(A_carriers + n * SITE_SIZE);
        pww(view, 0x22, 2);
    }
    pww(view, 0x24, table.off);
    pww(view, 0x26, table.seg);
}

/* state 2, inside a site or a carrier (S066A:26EB to 3BC9) */
static void state_inside(Far view, uint16_t side, uint16_t di, Far rec)
{
    Far site = far_at(view.seg, (uint16_t)(view.off + 0x24));
    uint16_t row0 = (uint16_t)(side * 7), e;

    if (pb(view, 0x18) == 9) {
        uint8_t p;

        pwb(view, 0x1B, 5);
        pww(view, 4, pw(view, 0x0C));
        pww(view, 6, pw(view, 0x0E));
        pww(view, 0x0C, 0);
        pww(view, 0x0E, 3);
        for (p = 0; p < 7; p++)
            if (pb(site, (uint16_t)(row0 + p)) != 0xFF) {
                pww(view, 0x0E, p);
                break;
            }
        place_xy(view, side);
        site_screen(side, site);
        if (pw(view, 0x22) == 1 && pb(view, 0x16) & 2) {
            uint16_t pts = NW(side_points, side * 2);

            if (pts > 0x2710)
                SETW(side_points, side * 2, 0x2710);
            build_list(pts, side);
        }
        gs_or(4, 0x100);
        SETW(site_taken, 0, 0);
        pwb(view, 0x18, 0x0A);
        pwb(view, 0x1A, 1);
    }
    if (pb(view, 0x18) == 0x0A) {
        e = 0xFF;
        if (di & 0x10) {
            pwb(view, 0x18, 0x0B);
            pwb(view, 0x19, 1);
            pwb(view, 0x1A, 1);
        } else {
            if (di & 1 || di & 4) {
                if ((int16_t)pw(view, 0x0E) > 0) {
                    pww(view, 0x0E, (uint16_t)(pw(view, 0x0E) - 1));
                    place_xy(view, side);
                    pwb(view, 0x1A, 1);
                    e = side ? 5 : 4;
                }
            } else if (di & 2 || di & 8) {
                if ((int16_t)pw(view, 0x0E) < 6) {
                    pww(view, 0x0E, (uint16_t)(pw(view, 0x0E) + 1));
                    place_xy(view, side);
                    pwb(view, 0x1A, 1);
                    e = side ? 5 : 4;
                }
            }
            if (pb(view, 0x1A) == 1) {
                /* the place shown (S066A:296E), its unit into word 1Eh */
                site_info(view, side);
                pwb(view, 0x1A, 0);
            }
            effect(e);
        }
    }
    if (pb(view, 0x18) == 0x0B) {
        if (pb(view, 0x19) == 1) {
            pww(view, 0x1C, 2);
            pww(view, 0x1E, pb(site, (uint16_t)(row0 + pw(view, 0x0E))));
            if ((int16_t)pw(view, 0x1E) <= 0xF0) {
                uint16_t f = NW(unit_flags, pw(view, 0x1E) * UNIT_SIZE);

                if (!(f & 0x100) && !(f & 0x200) && same_side(f, side) && !(f & 2)) {
                    if (pb(view, 0x16) & 2) {
                        if (!(NW(units, pw(view, 0x1E) * UNIT_SIZE + 6) & 4))
                            pww(view, 0x1C, pw(view, 0x1C) | 0x100);
                    } else if (!(gs(4) & 0x80))
                        pww(view, 0x1C, pw(view, 0x1C) | 0x10);
                }
            } else {
                uint16_t r = pw(site, 0x19);

                if (r & 8 && !(r & 2) && same_side(side, r) && NB(build_count, 0) && pb(view, 0x16) & 2)
                    pww(view, 0x1C, pw(view, 0x1C) | 0x80);
            }
            if (pw(view, 0x22) == 2)
                pww(view, 0x1C, pw(view, 0x1C) & 0xFE7F);
            pwb(view, 0x19, 2);
        }
        if (pb(view, 0x19) == 2) {
            e = 0xFF;
            if (di & 0x10) {
                pwb(view, 0x1B, 5);
                if (di & 8 && pw(view, 0x1C) & 2)
                    pwb(view, 0x1B, 1);
                else if (di & 2 && pw(view, 0x1C) & 0x100)
                    pwb(view, 0x1B, 8);
                else if (di & 4 && pw(view, 0x1C) & 0x80)
                    pwb(view, 0x1B, 7);
                else if (NB(sides, side * SIDE_SIZE + 0x15) >= NB(sides, side * SIDE_SIZE + 0x16)
                         && !(side_word(side) & 2)) {
                    show_message(0x53, side);
                    gs_or(4, side ? 0x10 : 8);
                } else if (di & 1 && pw(view, 0x1C) & 0x10)
                    pwb(view, 0x1B, 4);
            } else if (pb(view, 0x1B) == 4) {
                move_out(view, side);
                e = 0;
            } else if (pb(view, 0x1B) == 8) {
                uint16_t text = repair(view, side);

                e = text == 0x51 ? 0 : 1;
                show_message(text, side);
                gs_or(4, side ? 0x10 : 8);
                pwb(view, 0x18, 0x0A);
                pwb(view, 0x19, 0);
                pwb(view, 0x1B, 5);
                pwb(view, 0x1A, 1);
            } else if (pb(view, 0x1B) == 7) {
                pwb(view, 0x18, 0x0C);
                pwb(view, 0x19, 1);
                e = 0;
            } else if (pb(view, 0x1B) == 1)
                pwb(view, 0x18, 8);
            else {
                pwb(view, 0x18, 0x0A);
                pwb(view, 0x19, 0);
            }
            effect(e);
        }
    }
    if (pb(view, 0x18) == 0x0C) {
        if (pb(view, 0x19) == 1) {
            pww(view, 0x20, 0);
            pww(view, 0x0A, pw(view, 0x0E));
            pww(view, 8, pw(view, 0x0C));
            pww(view, 0x0E, 0);
            pwb(view, 0x1B, 5);
            pww(view, 0x10, side & 1 ? 0x0D + 0xA0 : 0x0D);
            pww(view, 0x12, 0x8B);
            build_rows(side, pw(view, 0x20));
            pwb(view, 0x1A, 2);
            pwb(view, 0x19, 4);
        }
        if (pb(view, 0x19) == 4) {
            e = 0xFF;
            if (di & 0x10)
                pwb(view, 0x19, 2);
            else {
                int16_t row = (int16_t)pw(view, 0x0E), first = (int16_t)pw(view, 0x20);
                int16_t count = NB(build_count, 0);

                if (di & 1 || di & 4) {
                    if (row > 0) {
                        pww(view, 0x0E, (uint16_t)(row - 1));
                        place_xy(view, side);
                        pwb(view, 0x1A, 2);
                        e = side ? 5 : 4;
                    } else if (first > 0) {
                        pww(view, 0x20, (uint16_t)(first - 1));
                        pwb(view, 0x1A, 2);
                        e = side ? 5 : 4;
                    }
                } else if (di & 2 || di & 8) {
                    if (row < 6) {
                        if (count - 1 > row) {
                            pww(view, 0x0E, (uint16_t)(row + 1));
                            place_xy(view, side);
                            pwb(view, 0x1A, 2);
                            e = side ? 5 : 4;
                        }
                    } else if (count - 7 > first) {
                        pww(view, 0x20, (uint16_t)(first + 1));
                        pwb(view, 0x1A, 2);
                        e = side ? 5 : 4;
                    }
                }
                if (pb(view, 0x1A) == 2) {
                    /* the kind shown (S066A:3677), into word 1Eh */
                    build_info(view, side);
                    pwb(view, 0x1A, 0);
                }
                effect(e);
            }
        }
        if (pb(view, 0x19) == 2) {
            if (!(di & 0x10)) {
                if (pb(view, 0x1B) == 7)
                    build_unit(view, side);
                else if (pb(view, 0x1B) == 1)
                    pwb(view, 0x18, 0x0D);
                else
                    pwb(view, 0x19, 4);
            } else {
                if (di & 4)
                    pwb(view, 0x1B, 7);
                else if (di & 8)
                    pwb(view, 0x1B, 1);
                else
                    pwb(view, 0x1B, 5);
                build_rows(side, pw(view, 0x20));
            }
        }
    }
    if (pb(view, 0x18) == 0x0D) {
        pww(view, 0x0C, pw(view, 8));
        pww(view, 0x0E, pw(view, 0x0A));
        place_xy(view, side);
        pwb(view, 0x1B, 5);
        pwb(view, 0x18, 0x0A);
        pwb(view, 0x19, 0);
        pwb(view, 0x1A, 1);
    }
    if (pb(view, 0x18) == 8) {
        pwb(view, 0x17, 0);
        pwb(view, 0x18, 0x19);
        pwb(view, 0x19, 0);
        pww(view, 0x26, 0);
        pww(view, 0x24, 0);
        pwb(view, 0x1B, 5);
        pww(view, 0x0C, pw(view, 4));
        pww(view, 0x0E, pw(view, 6));
        view_cursor(side);
        show_message(0xFFFF, side);
        gs_and(4, side ? 0xFFEF : 0xFFF7);
        pwb(rec, 8, 0);
        view_draw(pw(view, 2), side);
        marks_draw(pw(view, 2), side);
    }
}

/* the key of entry n of the table of keys behind S331F:02D3 */
static uint8_t key_entry(uint16_t n)
{
    Far t = far_at(S_game_state, 0x02D3);
    return pb(t, n);
}

/* attacks_run's music, which orders.c leaves out (the calls before
 * S02E6:0102, 1233 and 205B): with the music on (40h in game_state) it is faded out before the
 * fights, when the side whose orders are carried out has any, and again
 * at each site taken (order 4; nothing is on then if it was faded
 * before), and started anew at attacks_run's end when one of these came.
 * Done from here around the call: before it, at orders_hook's
 * ORDERS_TAKEN (the hook that was set is called behind it) and after it;
 * between these places and the original's nothing reads the sound's
 * state. */
static void (*round_kept)(int at, int n);
static uint8_t round_faded;

static void round_fade(void)
{
    if (gs(0) & 0x40) {
        music_fade(4);
        round_faded++;
    }
}

static void round_music(int at, int n)
{
    if (at == ORDERS_TAKEN)
        round_fade();
    if (round_kept)
        round_kept(at, n);
}

/* state 5, the side has ended its half (S066A:3BC9 to 418B) */
static void state_done(Far view, uint16_t side, uint16_t di, const SideIn *in)
{
    Far other = view_of(side & 1 ? 0 : 1);

    if (di & 0x10) {
        pwb(view, 0x17, 0);
        pwb(view, 0x18, 0);
        pwb(view, 0x19, 0);
        pwb(view, 0x1B, 5);
        gs_or(4, side ? 0x10 : 8);
        show_message(0x34, side);
        SETW(game_state, 0x0F, 4);
        told = 0;
    } else if (pb(other, 0x17) == 5) {
        SETW(game_state, 0x0F, 4);
        if ((side_word(0) & 2 && side_word(1) & 2) || (key && key == key_entry(0x0C))) {
            uint16_t f0 = NW(views, 0), f1 = NW(views, VIEW_SIZE);
            uint16_t a = NB(attack_pick, 0) & 2 ? 0 : 1;
            uint8_t res;

            key = 0;
            /* S066A:3CC5: the screen goes grey, or with a fight's
             * pictures dark */
            if (!NB(sides, a * SIDE_SIZE + 0x15)) {
                fade_grey(game_mem.save_buf);
                SETW(site_taken, 0, 0);
            } else {
                SETW(site_taken, 0, 1);
                if (!NB(no_fight_pics, 0))
                    fade_out();
            }
            SETB(take_picture, 0, 0);
            if (loop_hook)
                loop_hook(LOOP_ROUND, side);
            round_faded = 0;
            if (NB(sides, (NB(attack_pick, 0) == 2 ? 0 : 1) * SIDE_SIZE + 0x15))
                round_fade();
            round_kept = orders_hook;
            orders_hook = round_music;
            res = attacks_run();
            orders_hook = round_kept;
            if (gs(0) & 0x40 && round_faded)
                music_start(far_at(S_music_data, A_music_data), 0, 0xFF);
            if (loop_hook)
                loop_hook(LOOP_FOUGHT, side);
            /* the fight's pictures (S2532, not translated) start the
             * random numbers anew from the BIOS's count of ticks, once a
             * fight; the last of them comes in */
            if (in->seeded) {
                SETW(rand_words, 0, in->seed);
                SETW(rand_words, 2, in->seed >> 16);
            }
            /* S066A:3D5E to 3EBE: both views, their marks and cursors
             * drawn and shown, and the colours back */
            view_to_field(0, f0);
            view_redraw(0);
            view_to_field(1, f1);
            view_redraw(1);
            page_flip();
            page_copy();
            if (NW(site_taken, 0)) {
                if (!NB(no_fight_pics, 0) && !NB(take_picture, 0))
                    fade_in();
            } else
                fade_back(game_mem.save_buf);
            if (!(res & 0x80)) {
                Far won = view_of(res), lost = view_of(res & 1 ? 0 : 1);

                SETB(views, VIEW_SIZE + 0x17, 7);
                SETB(views, 0x17, 7);
                pww(won, 0x22, 0x0F);
                pww(lost, 0x22, 0x10);
            }
            income_add(NB(attack_pick, 0) & 2 ? 0 : 1);
            fww(S_COMP, A_computer_phase, (uint16_t)(frw(S_COMP, A_computer_phase) | 4));
            fww(S_COMP, (uint16_t)(A_computer_phase + 0x0C), (uint16_t)(frw(S_COMP, (uint16_t)(A_computer_phase + 0x0C)) | 4));
            told = 0;
        } else if (key && key == key_entry(0x1A)) {
            /* S066A:3FBD: the first side's view drawn, the battle saved
             * (save_game, the 320h bytes of the colours with it), the
             * text it returns shown for the first side; 200h in word 4
             * keeps the key from being taken again for four passes; the view
             * drawn once more, the pages flipped and the key cleared */
            uint16_t text;
            Far rec;

            view_draw(NW(views, 2), 0);
            marks_draw(NW(views, 2), 0);
            text = save_game(game_mem.save_buf);
            if (text == 0xFFFF)
                missing = "the saved game's number: no key came";
            else {
                show_message(text, 0);
                timer_set(1, 0x19, 0);
                gs_or(4, 0x200);
                timer_set(6, 4, 0);
                view_draw(NW(views, 2), 0);
                marks_draw(NW(views, 2), 0);
                page_flip();
                page_copy();
                key = 0;
                rec = far_at(S_DATA, 0x00BE);
                pwb(rec, 2, 0);
                pwb(rec, 0, 0);
                pww(rec, 4, 0);
            }
        }
        else if (!told) {
            show_message(0x52, 0);
            show_message(0x3F, 1);
            told = 1;
        }
    } else {
        uint16_t o = side ? 0 : 1;

        show_message(pb(view, 0x16) == 2 ? 0x33 : 0x32, side);
        /* 80h in game_state's word 2 when the computer's player of the
         * other side is in its phase 20h or 40h (S066A:40EA to 414E);
         * replayed, the original's decision comes in with the input */
        if (!loop_replay) {
            if (!(side_word(side) & 2) && side_word(o) & 2 && phase_word(o) & 0x60)
                gs_or(2, 0x80);
        } else if (in->set80) {
            if (side_word(side) & 2 || !(side_word(o) & 2))
                missing = "80h in game_state's word 2 where the port finds no reason";
            gs_or(2, 0x80);
        }
        if (!(side_word(side) & 2) && !(gs(2) & 2)) {
            SETW(game_state, 0x0F, 0);
            gs_and(0, 0xF7FF);
        }
    }
}

uint16_t loop_end_texts[2] = { 0xFFFF, 0xFFFF };

/* state 7, the battle's end (S066A:418B to 4333), run in each side's part
 * whose view is in it: word 22h of the two views chooses the texts (0Fh,
 * the side has taken the other's headquarters: 3Ch for it and 3Dh for the
 * other; 11h, the other side has no unit left: 41h for it and 42h for the
 * other; else 42h for both), both are shown, the page is flipped and
 * copied, and wait_press (S0CB6:0DA9) waits for the first side's button
 * or a key (LOOP_END) */
static void state_end(uint16_t side)
{
    uint16_t t0 = NW(views, 0x22), t1 = NW(views, VIEW_SIZE + 0x22);

    if (t0 == 0x0F || t1 == 0x0F)
        gs_or(0, 0x4000);
    else if (t0 == 0x11 || t1 == 0x11)
        gs_or(0, 0x2000);
    if (t0 == 0x0F)
        t0 = 0x3C, t1 = 0x3D;
    else if (t1 == 0x0F)
        t1 = 0x3C, t0 = 0x3D;
    else if (t0 == 0x11)
        t0 = 0x41, t1 = 0x42;
    else if (t1 == 0x11)
        t1 = 0x41, t0 = 0x42;
    else
        t0 = t1 = 0x42;
    if (gs(2) & 2)
        gs_or(0, 0x10);
    else if (gs(2) & 1) {
        uint16_t t = side_word(0) & 2 ? NW(views, VIEW_SIZE + 0x22) : NW(views, 0x22);

        if (t == 0x0F || t == 0x11) {
            if (side_word(1) & 2 && (int16_t)NW(map_number, 0) < 0x18)
                gs_or(0, 0x10);
            if (side_word(0) & 2 && (int16_t)NW(map_number, 0) > 0x17)
                gs_or(0, 0x10);
        }
    }
    if (!(gs(0) & 0x6000))
        gs_and(0, 0xFFEF);
    show_message(t0, 0);
    show_message(t1, 1);
    loop_end_texts[0] = t0;
    loop_end_texts[1] = t1;
    page_flip();
    page_copy();
    if (loop_hook)
        loop_hook(LOOP_END, side);
    gs_and(0, 0xFFFD);
}

/* S2B9B:000A (joy_setup): the joysticks looked for and measured anew:
 * the words that say which are there (DATA:0384, 0386) and their ranges
 * cleared, the game port (201h) written and read after two retraces; a
 * stick whose two axes' bits are 0 then is there and measured
 * (S2BAD:0006).  The port has no game port: it reads FFh as the runner's
 * does, no stick is there, and the largest range (DATA:03B6) is 0. */
static void joy_setup(void)
{
    static const uint16_t cleared[] = { 0x0384, 0x0386, 0x0394, 0x0396, 0x03A4, 0x03A6, 0x0398, 0x039A, 0x03A8, 0x03AA };
    unsigned i;

    for (i = 0; i < sizeof cleared / sizeof cleared[0]; i++)
        fww(S_DATA, cleared[i], 0);
    fwb(S_DATA, 0x03B4, 0);
    fww(S_DATA, 0x03B6, 0);
}

/* S066A:14BA to 1B17, in each side's part: the keys of entries 0Dh to 14h
 * of the table of keys (F2 to F9).  The text goes to the first side's
 * line, to the second's when the first side is the computer's, and stays
 * 19h passes (timer_set); but for F4 200h in game_state's word 4 keeps
 * the keys from being taken again for ten passes (timer 6).
 *   0Fh (F4)  text 59h and the two bytes at S331F:0007 and 0006 as
 *             numbers behind it
 *   0Dh (F2)  the music off (text 56h) or on (55h): 40h in game_state;
 *             only with 10h in its word 2 (there is music): music_fade(4)
 *             or music_start with the battle's tune (sound.c)
 *   0Eh (F3)  the effects off (58h) or on (57h): 80h; only with 20h in
 *             word 2
 *   10h, 11h (F5, F6)  the first and the second side's joystick off (65h,
 *             67h) or on (66h, 68h): words 34h and 36h of the record of
 *             DATA:009A (DATA:03B8, 03BA), then joy_setup.  F6's text
 *             always stays by timer 2
 *   12h (F7)  the mouse off (mouse_hide, mouse_close, 6Ah) or on (69h):
 *             opened for the first side's input record, with S37FF:0114
 *             above 0 for the second's, shown, the middle speed
 *   13h (F8)  with a mouse its speed, S37FF:0113 0 to 2 (6Bh, 6Ch, 6Dh)
 *   14h (F9)  the fights' pictures: no_fight_pics set gives 72h and both
 *             it and S37FF:0130 cleared; else with 0130 set 70h and
 *             no_fight_pics set; else 71h and 0130 set */
static void setting_keys(void)
{
    uint16_t ts = side_word(0) & 2 ? 1 : 0, timer = ts ? 2 : 1, text, opt = S_no_fight_pics;
    Far joy = far_at(S_DATA, 0x009A), ms = far_at(S_DATA, 0x00C2);

    if (key == key_entry(0x0F)) {
        show_message(0x59, ts);
        num_draw(frb(S_game_state, 0x0007), 0x63, 0xBD);
        num_draw(frb(S_game_state, 0x0006), 0x79, 0xBD);
        timer_set(timer, 0x19, 0);
        return;
    }
    if (key == key_entry(0x0D)) {
        if (!(gs(2) & 0x10) || gs(4) & 0x200)
            return;
        if (gs(0) & 0x40) {
            music_fade(4);
            text = 0x56;
            gs_and(0, 0xFFBF);
        } else {
            music_start(far_at(S_music_data, A_music_data), 0, 0xFF);
            text = 0x55;
            gs_or(0, 0x40);
        }
    } else if (key == key_entry(0x0E)) {
        if (!(gs(2) & 0x20) || gs(4) & 0x200)
            return;
        if (gs(0) & 0x80) {
            text = 0x58;
            gs_and(0, 0xFF7F);
        } else {
            text = 0x57;
            gs_or(0, 0x80);
        }
    } else if (key == key_entry(0x10) || key == key_entry(0x11)) {
        uint16_t second = key != key_entry(0x10), o = second ? 0x36 : 0x34;

        if (gs(4) & 0x200)
            return;
        if (pw(joy, o)) {
            pww(joy, o, 0);
            text = second ? 0x67 : 0x65;
        } else {
            pww(joy, o, 0xFFFF);
            text = second ? 0x68 : 0x66;
        }
        joy_setup();
        if (second)
            timer = 2;
    } else if (key == key_entry(0x12)) {
        if (gs(4) & 0x200)
            return;
        if (pw(ms, 0)) {
            mouse_hide();
            mouse_close();
            text = 0x6A;
        } else {
            mouse_open(0, frw(S_DATA, frb(opt, 0x0114) > 0 ? 0x00B6 : 0x00A6), 0);
            mouse_show();
            mouse_speed(0x0A, 0x0A, 0x0A, 0x0A, 0xFA0, 0x28, 0x28);
            ms = far_at(S_DATA, 0x00C2);
            pww(ms, 0x46, 0xFFFF);
            text = 0x69;
        }
    } else if (key == key_entry(0x13)) {
        uint8_t n;

        if (gs(4) & 0x200 || !pw(ms, 0))
            return;
        n = (uint8_t)(frb(opt, 0x0113) + 1);
        if (n > 2)
            n = 0;
        fwb(opt, 0x0113, n);
        if (n == 0) {
            mouse_speed(0x32, 0x32, 0x32, 0x32, 0xFA0, 0x28, 0x28);
            text = 0x6B;
        } else if (n == 1) {
            mouse_speed(0x0A, 0x0A, 0x0A, 0x0A, 0xFA0, 0x28, 0x28);
            text = 0x6C;
        } else {
            mouse_speed(3, 3, 3, 3, 0xFA0, 0x28, 0x28);
            text = 0x6D;
        }
    } else if (key == key_entry(0x14)) {
        if (gs(4) & 0x200)
            return;
        if (NB(no_fight_pics, 0)) {
            text = 0x72;
            SETB(no_fight_pics, 0, 0);
            fwb(opt, 0x0130, 0);
        } else if (frb(opt, 0x0130)) {
            text = 0x70;
            SETB(no_fight_pics, 0, 1);
        } else {
            text = 0x71;
            fwb(opt, 0x0130, 1);
        }
    } else
        return;
    show_message(text, ts);
    timer_set(timer, 0x19, 0);
    gs_or(4, 0x200);
    timer_set(6, 0x0A, 0);
}

/* DEBUG, the port's own (loop_cheat; never in a scripted run): the human
 * `side` wins the battle as a won fight makes it (S066A:3D5E on: both
 * views to state 7, the winner's word 22h 0Fh, the loser's 10h), so that
 * state_end, the pictures, battle_score and the next map follow as after
 * a real win.  With 2, below map 30h, the map's number is first made the
 * one before the campaign's last: CODES.DAT's records have bit 1 of byte
 * 8 (battle_score's 20h) on maps 23 (17h) and 47 (2Fh) alone, the ends of
 * the maps 0 to 23 and 24 to 47; battle_score then moves on to that last
 * map, which the menu offers next.  Dev-only, absent from a release build
 * (PORT_VERSION defined). */
#ifndef PORT_VERSION
static void cheat_win(uint16_t side)
{
    uint16_t map = NW(map_number, 0);

    if (loop_cheat == 2 && (int16_t)map < 0x30)
        SETW(map_number, 0, map < 0x18 ? 0x16 : 0x2E);
    loop_cheat = 0;
    SETB(views, VIEW_SIZE + 0x17, 7);
    SETB(views, 0x17, 7);
    pww(view_of(side), 0x22, 0x0F);
    pww(view_of(side ? 0 : 1), 0x22, 0x10);
}
#endif

/* one side's part of a pass (S066A:0F0C to 4399) */
static void side_part(uint16_t side, const SideIn *in)
{
    Far view = view_of(side), rec;
    uint16_t di = 0;

    if (loop_hook)
        loop_hook(LOOP_PART, side);
    if (in->key)
        key = in->key;
#ifndef PORT_VERSION
    if (loop_cheat && !(side_word(side) & 2))
        cheat_win(side);
#endif
    if (side_word(side) & 2) {
        uint16_t i;

        rec.seg = FAKE_SEG;
        rec.off = FAKE_OFF;
        for (i = 0; i < 8; i++)
            pwb(rec, i, 0);
        pwb(rec, 8, 1);
        if (!loop_replay)
            di = computer_part(side, in);
        else if (gs(2) & 0x80) {
            /* the computer's turn run until it is done, the texts and
             * the keys cleared (S066A:108D to 1161) */
            Far r0 = far_at(S_DATA, 0x00A6), r1 = far_at(S_DATA, 0x00B6);

            pwb(r0, 4, 0);
            pwb(r1, 4, 0);
            show_message(0x64, side);
            show_message(0xFFFF, 0);
            show_message(0xFFFF, 1);
            gs_and(0, 0xFFF3);
            gs_and(2, 0xFF7F);
        }
        if (loop_replay) {
            if (in->set8)
                SETW(sides, side * SIDE_SIZE, side_word(side) | 8);
            di = in->word;
            show_message(in->text, side);
        }
        gs_or(4, side ? 0x10 : 8);
    } else {
        rec = input_rec(side);
        if (loop_hook)
            loop_hook(LOOP_INPUT, side);
        else {
            pwb(rec, 0, in->word & 4 ? 1 : 0);
            pwb(rec, 1, in->word & 8 ? 1 : 0);
            pwb(rec, 2, in->word & 1 ? 1 : 0);
            pwb(rec, 3, in->word & 2 ? 1 : 0);
            pwb(rec, 4, in->word & 0x10 ? 1 : 0);
            if (in->rec5 >= 0)
                pwb(rec, 5, (uint8_t)in->rec5);
            if (in->rec8 >= 0)
                pwb(rec, 8, (uint8_t)in->rec8);
        }
        di = side_input(rec, view);
    }
    if (side)
        fww(S_map_cur, A_map_cur, frw(S_map2_ptr, A_map2_ptr)),
        fww(S_map_cur, A_map_cur + 2, frw(S_map2_ptr, A_map2_ptr + 2));
    else
        fww(S_map_cur, A_map_cur, frw(S_map_ptr, A_map_ptr)),
        fww(S_map_cur, A_map_cur + 2, frw(S_map_ptr, A_map_ptr + 2));
    if (!(di & 0x0F) && pb(view, 0x17) == 0)
        hover(view, side);
    if (key && key == key_entry(0) && !(NW(game_state, 4) & 0x200)) {
        /* S066A:1B17: the key of entry 0 asks whether to leave the battle
         * (texts 3Eh and 3Fh) and waits, the timer's routine going on
         * (LOOP_QUIT): the key of entry 18h ends it (2 cleared and 1 set
         * in game_state, no score), a button or the key of entry 1 goes
         * on with the battle */
        Far k = far_at(S_DATA, 0x00BE), ra = far_at(S_DATA, 0x00A6), rb = far_at(S_DATA, 0x00B6);

        show_message(0x3E, 0);
        show_message(0x3F, 1);
        page_flip();
        page_copy();
        loop_abort = 0;
        for (;;) {
            if (key_entry(0x18) == pb(k, 0)) {
                SETW(game_state, 0, (NW(game_state, 0) & 0xFFFD) | 1);
                SETW(score, 0, 0);
                SETW(score, 2, 0);
                break;
            }
            if (pb(ra, 4) || pb(rb, 4) || key_entry(1) == pb(k, 0)) {
                timer_set(1, 5, 0);
                timer_set(2, 5, 0);
                pwb(rb, 4, 0);
                pwb(ra, 4, 0);
                break;
            }
            if (loop_hook)
                loop_hook(LOOP_QUIT, side);
            if (!loop_hook || loop_abort) {
                missing = "the question whether to leave the battle got no answer";
                break;
            }
        }
    } else if (key)
        setting_keys();
    if (pb(view, 0x17) == 0) {
        Far map = far_at(S_map_cur, A_map_cur);

        if (loop_hook)
            loop_hook(LOOP_STEP, side);
        if (view_step(view, rec, di, map, side))
            cursor_move(view, side, di);
    }
    if (pb(view, 0x17) == 1)
        field_menu(view, side, di);
    if (pb(view, 0x17) == 3)
        state_map(view, side, di, rec);
    else if (pb(view, 0x17) == 4)
        state_field(view, side, di, rec);
    else if (pb(view, 0x17) == 6)
        state_enter(view, side);
    if (pb(view, 0x17) == 2)
        state_inside(view, side, di, rec);
    if (pb(view, 0x17) == 5)
        state_done(view, side, di, in);
    if (missing)
        return;
    if (pb(view, 0x17) == 7)
        state_end(side);
    /* S066A:4338: 100h in word 4 sounds, with the effects on: the effect
     * whose number is site_taken */
    if (gs(4) & 0x100 && gs(0) & 0x80)
        sfx_play(side ? 9 : 0x0B, NW(site_taken, 0), 0xDC, 1, 0x200);
    gs_and(4, 0xFEFF);
}

/* S0CB6:035A (timer_fire): what a timer of the kind does when it is due;
 * kind 5 only draws the field kept in game_state's word 64h + 2 * timer
 * again (field_draw), for the second side when bit 1 of its view's byte
 * 16h is set, else for the first */
void timer_fire(uint16_t kind, uint16_t timer)
{
    switch (kind) {
    case 1:
        show_message(0xFFFF, 0);
        gs_and(0, 0xFFFB);
        break;
    case 2:
        show_message(0xFFFF, 1);
        gs_and(0, 0xFFF7);
        break;
    case 3:
        gs_or(4, 0x20);
        break;
    case 4:
        gs_or(4, 0x40);
        break;
    case 5:
        field_draw(frb(S_views, (uint16_t)(A_views + 0x31 + 0x16)) & 2 ? 1 : 0, gs((uint16_t)(0x64 + 2 * timer)));
        break;
    case 6:
        gs_and(4, 0xFDFF);
        break;
    case 7:
        gs_and(4, 0xFBFF);
        break;
    default:
        break;
    }
}

/* the end of a pass (S066A:43A9 to 45D7): the texts' timers, a move's
 * step, the cursors drawn and the page shown, the count of passes and the
 * timers that are due */
static void pass_end(void)
{
    uint16_t i;
    uint32_t n;

    if (gs(4) & 8) {
        timer_set(1, 0x19, 0);
        gs_and(4, 0xFFF7);
    }
    if (gs(4) & 0x10) {
        timer_set(2, 0x19, 0);
        gs_and(4, 0xFFEF);
    }
    if (gs(4) & 0x40) {
        move_step();
        gs_and(4, 0xFFBF);
    }
    /* S066A:441F to 4554: each side's cursor but in its view's state 4,
     * shown; with 4 in the side's word 0 of `sides` (the small map) what
     * it covers kept at side_save, which state_map puts back
     * (pointer_erase); then snd_pass */
    for (i = 0; i < 2; i++)
        if (NB(views, i * VIEW_SIZE + 0x17) != 4)
            cursor_draw(i, side_word(i) & 4 ? side_save(i) : no_save());
    snd_pass();
    page_flip();
    page_copy();
    n = ((uint32_t)gs(0x15) << 16 | gs(0x13)) + 1;
    SETW(game_state, 0x13, n);
    SETW(game_state, 0x15, n >> 16);
    for (i = 0; i < 15; i++) {
        uint32_t due = (uint32_t)gs((uint16_t)(0x2A + 4 * i)) << 16 | gs((uint16_t)(0x28 + 4 * i));

        if (due <= n) {
            timer_fire(NB(game_state, 0x19 + i), i);
            SETB(game_state, 0x19 + i, 0);
        }
    }
}

/* one pass of game_loop, both sides' parts and its end; NULL, or what it
 * met that is not translated (the state is left as it was then) */
const char *loop_pass(const SideIn in[2])
{
    uint16_t side;

    missing = NULL;
    key = 0;
    for (side = 0; side < 2; side++) {
        SETB(game_state, 0x0C, side);
        side_part(side, &in[side]);
        if (missing)
            return missing;
    }
    SETB(game_state, 0x0C, 2);
    pass_end();
    return NULL;
}

/* game_loop's locals from its frame in memory (BP 0FEAh in the stack
 * segment, as in the dumps) */
void loop_reset(void)
{
    key = 0;
    told = frw(FAKE_SEG, 0x0FEA - 0x28);
}
