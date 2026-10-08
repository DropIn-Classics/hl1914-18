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
#define BLAST_SIZE 7u       /* a record of `blasts` */
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
Far far_add(Far p, uint32_t n);
Far file_load(Far to, Far name, Far work);
int32_t file_size(Far name);
int32_t file_load_size(Far to, Far name, Far work);
int32_t lib_load(Far name, Far to, Far work, Far rec, uint8_t order);
uint8_t *host_load(Far name, size_t *size);
extern const char *hl_save_dir;     /* where the port keeps what the program writes */
int host_save(Far name, const uint8_t *data, size_t size);
void far_copy(Far from, Far to, uint32_t n);
int16_t lib_pick(Far list, int16_t count, Far table, int16_t words, Far to, Far buf);
Far ground_store(Far pic);
int grounds_load(Far place, Far work);

/* pictures.c */
int lbm_show(Far place, Far name, Far work);
int32_t tpwm_unpack(Far from, Far to);
int lib_read(Far *place, Far file, Far names, Far ptrs, Far work);
int32_t sfx_load(Far place, Far name, Far work);

/* sound.c */
extern void (*snd_opl)(uint8_t reg, uint8_t value);
extern void (*snd_played)(uint16_t ch, uint16_t n, uint16_t vol, uint16_t times, int16_t r);
uint16_t snd_init(uint16_t mask);
void snd_end(void);
int16_t sfx_play(uint16_t ch, uint16_t n, uint16_t vol, uint16_t times, uint16_t step);
int16_t dig_play(uint16_t ch, Far p, uint16_t vol, uint16_t times, uint16_t step);
int16_t adl_play(uint16_t ch, Far p, uint16_t vol, uint16_t times);
int16_t adl_load(uint16_t ch, Far p, uint16_t times);
int16_t adl_start(uint16_t ch, uint16_t vol);
void adl_stop(uint16_t ch);
int16_t chan_stop(uint16_t ch);
void snd_pass(void);
void dsp_rate(uint16_t rate);
void dig_irq(void);
void adl_tick(void);
int16_t music_start(Far tune, uint16_t fade, uint16_t vol);
int16_t music_fade(uint16_t speed);
int16_t music_volume(uint16_t vol);
void fade_tick(void);
int snd_irq(void);
void snd_timer(int32_t *adl, int32_t *fade, uint32_t *ran);
/* the fight's sounds: pics_play (S2532:211E) and its callers' sites in
 * fight_frames, the channels stopped at fight_pictures' end,
 * attacks_run's effect for a unit destroyed without pictures; and the
 * two that do them from pics_hook's and orders_hook's places (below)
 * while fightpics.c and orders.c do not call them (main.c sets them) */
extern void (*snd_fight)(const char *what, uint16_t a, uint16_t b, uint16_t c, uint16_t d);
void pics_play(uint16_t slot, uint16_t vol, uint16_t times);
void pics_volley_sound(uint16_t side);
void pics_begin_sound(uint16_t side);
void pics_side_sound(uint16_t side, uint8_t n);
void pics_sounds_stop(void);
void fight_blast_sound(void);
void fight_pics_sounds(int at);
void fight_orders_sounds(int at, int n);
extern void (*snd_waited)(double s);
extern int (*snd_wait)(void);
extern void (*snd_at)(int place);
extern void (*snd_music)(int out, uint16_t a, uint16_t b, int16_t r);
void snd_clock(double t);
/* the audio's clock (sound.c says how) */
extern uint64_t (*snd_samples)(const double *t);
extern void (*snd_irq_at)(uint64_t sample, uint32_t next);
extern int snd_rate;
extern uint64_t snd_stamp;
extern int snd_stamp_irq;
int pictures_load(Far *place, Far work, Far pal);
int libs_start(Far *place);

/* screen.c */
void screen_open(uint16_t w, uint16_t h, uint32_t size, uint16_t pages);
void pages_set(void);
void screen_clear(void);
void page_flip(void);
void page_copy(void);
void draw_on_shown(void);
void draw_on_drawn(void);
void pal_level(uint8_t level, Far pal);
void fade_in(void);
void fade_out(void);
void fade_grey(Far pal);
void retrace_wait(void);
void pal_blend(Far from, Far to, uint16_t first, uint16_t last, int16_t steps, int16_t step);
void fade_back(Far pal);
/* called at each wait of retrace_wait and at each pal_level's end (kind
 * SCREEN_RETRACE or SCREEN_PAL_LEVEL), for showing the picture then;
 * NULL for none */
enum { SCREEN_RETRACE, SCREEN_PAL_LEVEL };
extern void (*screen_hook)(int kind);

/* draw.c */
void view_draw(uint16_t field, uint16_t side);
void view_dim(uint16_t side);
void game_text_at(uint16_t x, uint16_t y, uint16_t n, uint8_t base);
void num_draw(int16_t value, uint16_t x, uint16_t y);
void marks_draw(uint16_t field, uint16_t side);
uint8_t vrb(uint16_t seg, uint16_t off);
void vwb(uint16_t seg, uint16_t off, uint8_t v);
void rect_fill(void);
void text_draw(uint16_t x, uint16_t y, uint16_t gap, uint16_t line, uint8_t base, Far text, uint16_t *len);
void flag_draw(uint16_t x, uint16_t y, uint16_t c1, uint16_t c2, uint16_t c3, int standing);
void pic_draw(uint16_t x, uint16_t y, Far src, uint8_t base);
void pic_draw_save(uint16_t x, uint16_t y, Far src, Far save, uint8_t base);
void pic_unpack_draw(uint16_t x, uint16_t y, Far src, uint8_t base, Far work);
/* a site's or carrier's screen (state 2): the screen, the build list's
 * places, the place under the cursor and the kind under it shown (each
 * also sets the view's word 1Eh as the original does there) */
void site_screen(uint16_t side, Far site);
void build_rows(uint16_t side, uint16_t first);
void site_info(Far view, uint16_t side);
void build_info(Far view, uint16_t side);
/* the field menu's information: on a unit, on a field without one;
 * info_hook is called at their entry (0) and return (1), `which` 0 for the
 * unit's and 1 for the battle's */
extern void (*info_hook)(int at, int which);
int unit_info(uint16_t side, uint16_t field, uint8_t unit, Far map);
void battle_info(uint16_t side);

/* fightpics.c: fight_pictures' arguments in the order of its stack
 * (S2474's fight_show passes them), and `sp` the stack pointer at its
 * entry (where lib_read's list goes, as on the original's stack) */
typedef struct {
    uint8_t near0, standing;
    uint16_t show, winter, kind0, kind1, ground0, ground1, count0, count1, after0, after1;
    Far none, place, work;
    uint16_t sp;
} FightPics;
/* where pics_hook is called: instead of clock_seed(0), after the loading
 * (before the key is cleared), in each frame before the key is tested,
 * at each level of the fade, at its end */
enum { PICS_SEED, PICS_LOADED, PICS_FRAME, PICS_LEVEL, PICS_END };
extern void (*pics_hook)(int at);
void clock_seed_bios(void);
void sprite_draw(int16_t x, int16_t y, Far pic, Far save, uint16_t base, int16_t sx, int16_t sy);
void latch_copy(uint16_t w, uint16_t h, Far src, Far dst);
void unders_restore(void);
void under_save(uint16_t w, uint16_t h, Far src, Far save);
int fight_pictures(const FightPics *a);

/* events.c: the pictures of events behind a battle (event_picture's
 * kinds CDh to D0h); event_hook keeps their time (play.c): at
 * inputs_release (EV_RELEASE), at a frame's beginning (EV_FRAME), for n
 * retraces (EV_RETRACES; 0 returned on the original's clock, where a
 * frame's wait ends with its log), after a fade_in or fade_out (EV_FADE)
 * and a pal_level (EV_LEVEL), and for the loading of the files n
 * (EV_BUSY with an EV_LOAD_) */
enum { EV_RELEASE, EV_FRAME, EV_RETRACES, EV_FADE, EV_LEVEL, EV_BUSY };
enum { EV_LOAD_WON, EV_LOAD_WIPED, EV_LOAD_MEDAL, EV_LOAD_MEDAL2, EV_LOAD_MEDAL3 };
extern int (*event_hook)(int at, int n);
extern int event_frame;
int16_t event_picture(Far place, Far work, uint16_t kind, uint16_t arg);

/* mem.c */
Far mem_alloc(uint32_t size);

/* battle.c */
/* the far pointers game_loop keeps on its stack: its block's free part
 * as the battle's setup found it ([BP-8]), the buffer of 320h bytes at
 * its beginning ([BP-0Ch]), where the map's pictures are loaded
 * ([BP-10h]), the end of the sides' two lists ([BP-18h]), the work buffer
 * of the loads ([BP-20h]) and the free end ([BP-4]) */
/* libs and pics: the free end before libs_start and before pictures_load
 * at the battle's beginning (game_loop's [BP-14h] and [BP-1Ch]) */
typedef struct { Far block, save_buf, gfx, lists_end, work, p, libs, pics; } GameMem;
extern GameMem game_mem;
int start_setup(void);
int menu_result(uint16_t map);
void scores_load(Far buf, Far work, int16_t map, Far name);
int battle_setup(void);
/* called by battle_after for each picture the original shows there
 * (event_picture's kind, its last argument and the work buffer its
 * caller passes; the picture's place is 2710h bytes behind it) */
extern void (*battle_hook)(uint16_t picture, uint16_t arg, Far work);
#define AFTER_FADE 0xFFFF       /* battle_hook's picture: fade_out is done */
#define AFTER_SCORE 0xFFFE      /* battle_score begins */
uint16_t battle_after(void);
int battle_begin(void);
void view_to_field(uint16_t side, uint16_t field);
void view_cursor(uint16_t side);
uint32_t defence_sum(void);

/* save.c */
uint16_t save_game(Far buf);
int16_t load_game(Far buf);

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
int16_t rand_next(void);
uint16_t computer_assess(uint16_t side);
void step_add(uint16_t side, Far step);
uint16_t computer_queue(uint16_t side);
uint16_t computer_plan(uint16_t side);
void computer_turn(uint16_t side);
uint16_t computer_progress(uint16_t side);

/* side.c */
void build_list(uint16_t points, uint16_t side);
uint16_t repair(Far view, uint16_t side);
void build_unit(Far view, uint16_t side);
void view_copy(Far view, int back);
void move_out(Far view, uint16_t side);
uint16_t side_input(Far rec, Far view);
void show_message(uint16_t text, uint16_t side);
void field_draw(uint16_t side, uint16_t field);
void cursor_draw(uint16_t side, Far save);
void pixel_put(void);
void small_map_back(uint16_t side);
void small_map_draw(uint16_t side, uint16_t x, uint16_t y);
void cursor_field(Far view, uint16_t side);
void bar_draw(uint16_t x, uint16_t y, uint16_t colour, uint16_t back, uint16_t h, int16_t value, int16_t width);
void blast_draw(uint16_t n, Far map, uint16_t view, uint16_t side);
void unit_panel(uint8_t no, uint16_t side);
int fight_panel(Far rec, uint16_t pictures);
void marks_redraw(uint16_t field, uint16_t side, uint8_t mask);
void marks_erase(uint16_t field, uint16_t side, uint8_t mask);
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
 * steps do, computer_step).  Of the computer's side only `turns` is used,
 * the number of calls of computer_turn in the loop at S066A:10CD (-1 not
 * known), unless loop_replay is set: then its word, text, set80 and set8
 * stand in for computer_turn.  seeded: clock_seed was called in the part
 * (the fight's pictures do, with the BIOS's count of ticks), and `seed`
 * is what it left in rand_words (word 2 above word 0) */
typedef struct {
    uint8_t key;
    uint16_t word, text;
    int16_t rec5, rec8;
    int set80, set8;
    int turns;
    int seeded;
    uint32_t seed;
} SideIn;
extern int loop_replay;
/* where loop_hook is called in a pass: a side's part begins (before its
 * key is taken from SideIn); a human's input record is about to be read
 * by side_input; view_step is about to be called; computer_turn has
 * returned in the loop that runs while the computer thinks (SideIn's
 * `turns` set to the calls so far ends it); attacks_run is about to be
 * called; it has returned (SideIn's `seeded` and `seed` are read next);
 * the battle's end has shown its texts (loop_end_texts, the first side's
 * and the second's) and waits for a press (wait_press) */
enum { LOOP_PART, LOOP_INPUT, LOOP_STEP, LOOP_THINK, LOOP_ROUND, LOOP_FOUGHT, LOOP_END, LOOP_QUIT, LOOP_SAVE };
/* LOOP_QUIT: the pass waits for the answer to the question whether to
 * leave the battle; the hook lets the timer's routine run on and sets
 * loop_abort when no answer can come */
/* LOOP_SAVE: save_game waits for the number's key or the key of entry 0,
 * the timer's routine going on; loop_abort when none can come */
extern int loop_abort;
extern void (*loop_hook)(int at, uint16_t side);
extern uint16_t loop_end_texts[2];
/* where orders_hook is called in attacks_run, for the time the original
 * spends there showing what the port does not draw: a fight is about to
 * be worked out (n: its pictures are on); a turn of the blasts' loop of a
 * fight without pictures; a fight is over (n: pictures); a site is taken
 * (n: a fight came before in this attacks_run) */
enum { ORDERS_FIGHT, ORDERS_BLAST, ORDERS_FOUGHT, ORDERS_TAKEN };
extern void (*orders_hook)(int at, int n);
void timer_fire(uint16_t kind, uint16_t timer);
const char *loop_pass(const SideIn in[2]);
void loop_reset(void);

/* input.c */
void key_irq(uint8_t code);
void key_install(void);
void key_remove(void);
const char *input_tick(void);

/* joy.c: the joysticks' routines, and the game port they read: a stick
 * given by the times of its one-shots (joy_stick: microseconds, x below 0
 * no stick), from a script by the clock (joy_script; joy_clock and
 * joy_battle for the original's clock, joy_tick at each call of the
 * timer's routine, which also carries a joy_measure with `wait` on
 * through its retraces) */
void joy_measure(int wait, double now);
void joy_read(void);
void joy_stick(int stick, int x_us, int y_us, int buttons);
const char *joy_script(const char *path);
const char *joy_clock(const char *path);
void joy_battle(int menu);
double joy_tick(void);

/* mouse.c */
uint16_t mouse_read(void);
uint16_t mouse_open(uint16_t mode, uint16_t rec, uint16_t draw);
uint16_t mouse_close(void);
uint16_t mouse_range(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
uint16_t mouse_speed(uint16_t a, uint16_t b, uint16_t c, uint16_t d, uint16_t e, uint16_t f, uint16_t g);
void mouse_to(uint16_t x, uint16_t y);
uint16_t mouse_hide(void);
uint16_t mouse_show(void);
void mouse_motion(int dx, int dy, int buttons);
void mouse_ratio(uint16_t x, uint16_t y);
void mouse_set(uint16_t x, uint16_t y, int buttons);
void mouse_where(uint16_t *x, uint16_t *y, int *buttons);
void pointer_erase(Far save);

/* menu.c */
/* the menu before a battle (S167A:0005), played on the port's own clock
 * (play_start); 0, or FFFFh (a file could not be loaded).  menu_stopped
 * is set when it could not go on (what it met).  menu_log_to: a file that
 * gets a line for each thing that came in: time, what (1 chosen, 2 the
 * other page, 5 another item), the page, the item's place and number;
 * and "TIME 0" when the menu has returned */
extern const char *menu_stopped;
void menu_log_to(const char *path);
void menu_shot(double at, const char *file);
void menu_ask_shot(int n, const char *file);
void inputs_release(int wait);
void text_mid_uncut(int16_t y, Far text, Far font, uint16_t base, int16_t gap);
int menu_run(Far place);

/* briefing.c */
/* the new units' screens before a battle (S274D:0003); 0, or 1 when a
 * file could not be loaded.  briefing_hook is called at its entry (0), for
 * each screen when it is lit and about to wait (1, n from 1 on) and at its
 * return (2) */
extern void (*briefing_hook)(int at, int n);
/* set when a player is there (the window): the screens wait for a key as
 * long as it takes */
extern int briefing_player;
uint16_t briefing(uint16_t map, Far place, Far work);

/* play.c */
/* the port's program plays a battle by itself: the human's keys from a
 * key script, the time from its own clock or from the original's
 * (port/tests/play_dumps.py).  Each returns NULL or what went wrong. */
const char *play_keys(const char *path);
const char *play_clock(const char *path);
const char *play_seeds(const char *path);
double play_now(void);
void play_start(double start);
void play_tick(void);
void play_retraces(int n);
void play_fade(void);
void play_setup(void);
int play_idle(void);
const char *play_stopped(void);
void play_inputs(const char *path);
void play_keylog(const char *path);
void play_micelog(const char *path);
const char *play_mice(const char *path);
void play_rounds(const char *path);
void play_ends(const char *path);
void play_evdump(const char *prefix);
void play_bios(uint32_t ticks);
void play_begin(double start);
void play_live(void);
void play_sync(void);
uint16_t play_after(void);
const char *play_pass(void);
extern unsigned long play_passes, play_misses;

/* names.c */
Far file_name(uint16_t where, int16_t dir, Far name, int16_t ext);
Far num_text(int32_t value, Far to, int16_t width, uint8_t flags);

#endif
