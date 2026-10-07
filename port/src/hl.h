/* hl.h - HL.EXE's memory by the names of the hints, and what is translated.
 *
 * The translated routines work on the program's own memory image
 * (doskit/runtime/rmem.h), loaded where dosrun loads it, so their results
 * can be compared byte for byte with the original's.
 */
#ifndef HL_H
#define HL_H

#include "rmem.h"
#include "gen/names.h"

/* where dosrun loads HL.EXE when it is started by itself (its -ram dumps
 * have the image at 0076h) */
#define HL_PSP 0x0066u
#define HL_BASE (HL_PSP + 0x10u)
/* the paragraphs the program's block has there (its MCB in the dumps) */
#define HL_PARAS 0x3F80u

/* S_name: the segment a name is in, as loaded; A_name: its offset */
enum {
#define X(seg, name, off) S_##name = HL_##seg + HL_BASE, A_##name = off,
    HL_NAMES(X)
#undef X
    HL_NAMES_END
};

/* byte and word at a name plus an offset */
#define NB(name, o) frb(S_##name, (uint16_t)(A_##name + (o)))
#define NW(name, o) frw(S_##name, (uint16_t)(A_##name + (o)))
#define SETB(name, o, v) fwb(S_##name, (uint16_t)(A_##name + (o)), (uint8_t)(v))
#define SETW(name, o, v) fww(S_##name, (uint16_t)(A_##name + (o)), (uint16_t)(v))

/* a far pointer as the program keeps it: offset, then segment */
typedef struct { uint16_t seg, off; } Far;

static inline Far far_at(uint16_t seg, uint16_t off)
{
    Far p;
    p.off = frw(seg, off);
    p.seg = frw(seg, (uint16_t)(off + 2));
    return p;
}
static inline uint8_t pb(Far p, uint16_t o) { return frb(p.seg, (uint16_t)(p.off + o)); }
static inline uint16_t pw(Far p, uint16_t o) { return frw(p.seg, (uint16_t)(p.off + o)); }
static inline void pwb(Far p, uint16_t o, uint8_t v) { fwb(p.seg, (uint16_t)(p.off + o), v); }
static inline Far pfar(Far p, uint16_t o) { return far_at(p.seg, (uint16_t)(p.off + o)); }

#define UNIT_SIZE 0x1Au     /* a record of `units` */
#define KIND_SIZE 0x45u     /* a record of `unit_dat` */

/* fight.c */
void field_neighbours(uint16_t field);
int same_side(uint16_t flags, uint16_t other);
uint8_t fight_around(uint8_t unit, uint8_t other, Far map, uint16_t side);
void fight_percent(uint8_t attacker, uint8_t target, Far map, uint16_t side);
int16_t field_distance(int16_t a1, int16_t a2, int16_t b1, int16_t b2);
void fight_losses(void);

/* range.c */
void buf_neighbours(int16_t col, int16_t row);
void field_bits_clear(uint8_t mask);
void move_costs(Far buf, uint8_t unit, uint16_t side, uint16_t mask, Far map);
int16_t move_range(int16_t field, Far buf, uint16_t unit, int16_t range,
                   uint16_t side, uint16_t mask, Far map);
uint8_t attack_range(Far buf, int16_t field, int16_t range, uint16_t side,
                     uint16_t unit, uint16_t mask, Far map);

int16_t path_find(Far buf, uint16_t unit, int16_t from, int16_t to, uint16_t side, Far map);

/* move.c */
uint8_t field_dir(int16_t from, int16_t to);
int16_t row_count(uint16_t side, Far rec, uint16_t used);
int16_t row_load(Far rec, uint16_t side);
void row_move(uint16_t side, Far from, Far to);
void carrier_moved(uint16_t unit, uint16_t side);
uint16_t site_find(uint16_t field, Far table);
void timer_set(uint16_t kind, int16_t passes, uint16_t value);
void unit_mark(uint8_t unit, uint16_t bits, uint16_t side, uint16_t set);
uint16_t move_target(uint16_t field, uint16_t side, Far map);
uint16_t move_arrive(uint16_t side, Far map);
uint16_t move_step(void);
uint16_t move_start(Far view, uint16_t side);
void attack_unmark(Far view, uint16_t side);
uint16_t attack_order(Far view, Far map, uint16_t side);
void move_done(Far view, Far map, uint16_t side, uint16_t flags);
uint16_t move_aim(Far view, Far map, uint16_t side);

/* units.c */
void unit_destroy(uint8_t unit, uint8_t put, uint8_t put2, int8_t slot, uint16_t side, Far map);
void site_free(Far site, uint16_t row);
void unit_free(uint8_t unit, uint16_t row);
uint16_t carrier_find(uint16_t mask);
uint8_t unit_find(uint16_t mask);
void site_add_400(uint16_t n, uint16_t field, uint16_t flags);
void site_add_100(uint16_t n, uint16_t field, uint16_t flags);
void site_add_40(uint16_t field, uint16_t side);
void carrier_add(int16_t n, uint16_t field, uint16_t kind, uint16_t flags, uint16_t unit);
void unit_add(uint8_t unit, uint8_t kind, uint16_t field, uint16_t flags);

/* map.c */
void map_copy(Far from, Far to);
void map_reset(void);
void map_setup(Far file);
void shp_reset(void);
void shp_setup(Far file);
uint16_t map_load(Far name, Far buf, Far work);
uint16_t shp_load(Far name, Far buf, Far work);

/* files.c */
extern const char *hl_game_dir;     /* the installed game's folder */
Far ptr_norm(Far p);
Far file_load(Far to, Far name, Far work);
int32_t file_size(Far name);
int32_t file_load_size(Far to, Far name, Far work);

/* mem.c */
Far mem_alloc(uint32_t size);

/* battle.c */
/* the far pointers game_loop keeps on its stack: its block's free part
 * as the battle's setup found it ([BP-8]), the buffer of 320h bytes at
 * its beginning ([BP-0Ch]), where the map's pictures are loaded
 * ([BP-10h]), the end of the sides' two lists ([BP-18h]), the work buffer
 * of the loads ([BP-20h]) and the free end ([BP-4]) */
typedef struct { Far block, save_buf, gfx, lists_end, work, p; } GameMem;
extern GameMem game_mem;
int start_setup(void);
int menu_result(uint16_t map);
int battle_setup(void);
int battle_begin(void);
void view_to_field(uint16_t side, uint16_t field);
void view_cursor(uint16_t side);
uint32_t defence_sum(void);

/* orders.c */
void orders_run(uint16_t m, uint16_t a, Far map, uint8_t *result);
void order_undo(uint8_t unit, Far map, uint16_t side);
void income_add(uint16_t side);
uint8_t attacks_run(void);

/* computer.c */
uint16_t computer_input(uint16_t side);
uint16_t computer_step(uint16_t side, int *left);
void move_targets_list(Far buf, uint16_t side, uint8_t unit);

/* plan.c */
uint16_t computer_assess(uint16_t side);
void step_add(uint16_t side, Far step);
uint16_t computer_queue(uint16_t side);
uint16_t computer_plan(uint16_t side);
void computer_turn(uint16_t side);

/* side.c */
void build_list(uint16_t points, uint16_t side);
uint16_t repair(Far view, uint16_t side);
void build_unit(Far view, uint16_t side);
void view_copy(Far view, int back);
void move_out(Far view, uint16_t side);
uint16_t side_input(Far rec, Far view);
void show_message(uint16_t text, uint16_t side);
void field_menu(Far view, uint16_t side, uint16_t bits);
uint8_t view_step(Far view, Far rec, uint16_t bits, Far map, uint16_t side);

/* loop.c */
/* what a pass of game_loop reads from outside the translated code, for
 * one side: the key read in its part (0 none); a human's directions and
 * button as side_input finds them in the input record (1 up, 2 down,
 * 4 left, 8 right, 10h the button), the computer's input word; the text
 * the computer's part shows (FFFFh none); a human's input record bytes 5
 * and 8 as view_step reads them (-1 not read); whether the original set
 * 80h in game_state's word 2 in state 5 (by the computer's phase); and
 * whether the computer's turn set 8 in the side's word 0 of `sides` (its
 * steps do, computer_step, which the port's pass does not run) */
typedef struct {
    uint8_t key;
    uint16_t word, text;
    int16_t rec5, rec8;
    int set80, set8;
} SideIn;
void timer_fire(uint16_t kind, uint16_t timer);
const char *loop_pass(const SideIn in[2]);
void loop_reset(void);

/* names.c */
Far file_name(uint16_t where, int16_t dir, Far name, int16_t ext);
Far num_text(int32_t value, Far to, int16_t width, uint8_t flags);

#endif
