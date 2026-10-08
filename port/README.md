# port: Battle Isle Historyline 1914-1918 in C

A native compatibility implementation requiring an installed copy of the
original game, on doskit's runtime (doskit/runtime). The game's data is
read from the player's copy at run time; none of it is in this
repository.

## State

Started 2026-10-02 from doskit's template: finds or unpacks the game's
files and shows a text screen. Nothing of the game is translated yet.

2026-10-03: follows doskit `a4b8166`'s template. When a newer release is
found, U on the setup screen downloads, verifies and installs it on
Windows and Linux (`update_install`), and opens the release page on
macOS; `dist/README.txt` says so. Built on macOS without a warning; the
update itself was not run, and no Windows or Linux build was made.

2026-10-04: the first translated routines, in `src/fight.c` over the
program's memory (`src/hl.h`, the names from `src/gen/names.h`, which
`doskit/tools/symmap.py` writes from `src/HL.hints`): `fight_percent` and
`fight_losses` with `field_neighbours`, `same_side`, `fight_around` and
`field_distance`. They are compiled into the program but nothing calls
them yet: the program still only finds the game's files.

Then `src/units.c`: `unit_destroy`, `unit_free` and `site_free`, and the
routines that make units and sites: `unit_add`, `carrier_find`,
`carrier_add`, `site_add_400`, `site_add_100`, `site_add_40`. `src/map.c`
builds a battle's state from the two files of a map once they are in
memory: `map_reset` and `map_setup` (the two parts of `map_load` around
its loading of the file), `map_copy`, `shp_reset` and `shp_setup` (the same
of `shp_load`). `src/files.c` has `file_load`, which reads a file of the
installed game into the program's memory and unpacks a packed one, and
`ptr_norm`; its allocation when no place is given, the message and the
further tries when a file cannot be opened are not translated. With it
`map_load` and `shp_load` are whole (`src/map.c`). `src/names.c` has
`file_name`, which puts a file's name together from the program's tables
of folders and endings, and `num_text` (a number as decimal text);
`src/mem.c` has `mem_alloc` on the runtime's DOS blocks.

Then the program's own start (`src/battle.c`, `src/main.c`): it loads
`HL.EXE` from the game's folder into its memory where dosrun loads it and
runs `start_setup` and `battle_setup`, the two parts of the original's
main routine that take its block of memory, load the two files that stay
at its beginning, share the block out and load a battle's files into its
units, sites and maps (`file_size` and `file_load_size` in `src/files.c`
came with it). What draws, sounds or belongs to the menu is left out, so
the game itself does not run: the program says which map it set up and
waits for Esc. `-map N` chooses the map (0 without), `-ram FILE` writes
the memory as dosrun's `-ram` does and ends the program; both are aids
for this work, not for players. The fight and the taking off of units are
still called by nothing.

Then `battle_begin` (`src/battle.c`), the rest of the main routine's setup
up to where its loop begins: the computer's file of a single player's map
(`MAP\NN.COM`), the tables and counts of a battle's beginning, the reset
of the computer's player, and each side's view put on its starting field.
Left out were the screen shown before a battle (since translated: the
new units' screens, below), the libraries of pictures, the loading of a
saved game, what draws the views and the music. The
menu is not translated: `menu_result` sets what it leaves when a map is
chosen and nothing else is changed (from the map's record in `CODES.DAT`:
a map for two players has two human sides, a map for one the computer as
its second side below map 24 and as its first from there on).

Then `src/range.c`: `move_range`, which marks where a unit can move, with
`move_costs` (what entering each field costs it) and `buf_neighbours`,
and `attack_range`, which marks whom it can attack, and `path_find`, the
way of a unit to a field it can reach. Nothing calls them yet.

Then `src/move.c`, how a move is carried out: `move_step`, which takes
the walking unit one field on along its way, `move_target`, which says
what a move to a field would be (a free field, into a carrier, into or
onto a site) or which text refuses it, and `move_arrive`, which ends the
walk and gives the unit its order; with them `field_dir`, `row_count`,
`row_load`, `row_move`, `carrier_moved`, `site_find`, `timer_set` and
`unit_mark`. What the original draws on the way and the text it shows at
the end are left out (`move_step` returns the text's number). Nothing
calls them yet.

Then `src/loop.c`, a pass of the original's main loop (`game_loop`,
`S066A:0EE9` to `45D7`) for both sides: the input word of each side,
`view_step`, the cursor's moves and the view's scrolling, `field_menu`,
the small map, what is on a field, going into a site or carrier and what
is done there (moving out, repairing, building), the end of a half and
the round's end, the battle's end, the timers (`timer_fire`) and the
count of passes. With it `attacks_run` (`src/orders.c`) has its frame
around the orders: the round's end that makes the two sides' records and
maps the same and turns the halves round (`rows_sync`,
`units_drop_empty`, the mending of units, who is left). Left out: what
draws, sounds and waits, and the keys F2 to F9, Esc and saving.
`attacks_run` now carries the fights out too (`fights_run`: `fight_rec`
filled, `fight_percent` and `fight_losses`, destroyed units, losses of a
carrier's load (`carrier_losses`), experience, the blasts counted down
when the fights are not pictures), frees the units marked 4 with
`order_undo` (`src/move.c`), and order 1 (`order_capture`); the
pictures shown meanwhile and loaded again after them (`pictures_load`,
`S02E6:2B87`) are left out. The computer's side of a pass is played by
`computer_turn` (`src/plan.c`); what the keyboard handler gives a pass
comes in from outside (`SideIn`), as do two things of the clock's: how
long a human's key lets the computer think on in one pass, and the
number the fight's pictures start the random numbers with.

Then the program plays a battle by itself, without drawing it
(`src/play.c`, `src/input.c`): after its own setup it runs the passes
with a key script as the human's input (`-keys FILE`, the format of
`tests/keys`), through translations of the original's keyboard interrupt
and of the routine its timer calls, which fill the sides' input records
(`key_irq`, `input_tick`; the joysticks and the mouse are left out). The
port's own clock calls that routine 72.8 times a second, lets a pass
last as many calls as the original waits for at its end and a round's
end as long as the original's: the retraces and scan lines its waits
and changes of colours count (`fade_grey`, `fade_back`, `fade_in`,
`fade_out`, `fight_panel`'s marks, `wait_input`, the taken site's
waits), and between them the times the runner took to compute, load
and draw, measured in runs (the `T_` numbers of `src/play.c`; they are
that machine's, not the game's, and stand in until the port draws).
The waits clear the button and the key as the original's do, a key
ends a fight's pictures, and the random numbers are started from the
BIOS's count of ticks, which runs with the clock from the time of day
or from `-seed N`, at the setup and when a fight's pictures begin.
Everything else takes no time. `-nopics` plays with the fights not
shown as pictures, `-rounds FILE` writes when each round's end began
and ended. `-clock FILE`
runs the same by the times of a log of the original instead
(`tests/play_dumps.py`), for the comparison below; `-seeds FILE` keeps
the port's own clock and takes from such a log only what the fights'
pictures did to the random numbers (the number each started them with
and its calls of `rand_next`). `-passes N` ends
after N passes, `-until T` at T seconds of the own clock, `-start T` is the key script's time at the first pass,
`-dump N FILE` writes the memory at the beginning of pass N. The play
stops with a line saying why at the battle's end and at what is not
translated (the keys F2 to F9, Esc, saving). The start now also sets the
far pointer to the table of keys and the twelve keys beside it
(`start_setup`, from `S1650:0003`), and `move_step` shows the text of a
walk's end. These options are aids for this work, not for players: the
window still only says which map was set up.

The menu (2026-10-08, `src/menu.c`, `menu_run` from `S167A:0005`): with a
key script and no `-map` the program begins where `game_loop` calls the
menu and goes round as it does: the menu, what `game_loop` does at its
return (`S066A:0390`), the battle's setup and the battle, what follows
it, and the menu again, until the menu ends the program, the key script
has nothing left for it, or a battle stops before its end. The menu's
loads (the scores' buffer, three pictures shown and fetched back from
the page, `CODES.DAT`, six libraries, two tunes, the sounds,
`MNUDATA.DAT`), its loop over pages and items, the map's code, the
country, the best scores, a winner's name, a saved game's number and
everything it leaves for the battle are translated over the program's
memory, and it is drawn as the original draws it, with the mouse's
pointer; it makes no sound. Its input is what the interrupts leave: the keys
(`input.c`, now with the BIOS's keys for what is typed while the
program's keyboard interrupt is out) and the mouse's record
(`src/mouse.c`: the program's routines over a driver whose pointer only
the program moves; no mouse of the player's yet). Its time is the port's
own clock: a pass lasts five calls of the timer's routine, the waits and
changes of colour as long as the original's count, drawing and loading
nothing. The window does not show the menu yet (it plays it from a key
script only). `-start T` is the key script's time at which the menu begins,
`-menulog FILE` writes what came in, `-menudump N FILE` the memory at
the menu's N-th return, `-menuvram T FILE` the video memory at the first
pass of its loop at or after second T, `-askvram N FILE` the video memory
at the N-th screen of the saved game's question (the question, or the
text that the file was not found), `-saves DIR` is where a file the program writes
(a map's best scores) is kept instead of the port's own folder; the
game's folder is never written to. Esc in a battle and the key that
confirms it now leave the battle (`loop.c`).

2026-10-08, saving and loading (`src/save.c`: `save_game`, `load_game`,
`save_blocks`, with `view_dim` and `game_text_at` in `src/draw.c`). With
both sides waiting for the round's end the key of entry 1Ah of the table
of keys (D) asks for a number: a digit saves the battle under it, the key
of entry 0 (Esc) leaves. The file is the original's, byte for byte (a
header of 1Bh bytes and 24 blocks of memory), named as the original
names it (`03.DAT` for number 3) and kept where the port keeps what the
program writes (`-saves DIR`, else the port's own folder), never in the
game's folder. The menu's MISC, LOAD and a digit load it: `battle_begin`
sets the battle of the menu's map up as before, reads the saved game
over it (`load_game`) and loads the grounds and the pictures again. A
saved game is looked for in the port's folder first, then in the game's
(`host_load`), so one the original wrote into the game's folder loads
too (not tried). With `-clock FILE` and no `-map` the program now walks
its menu on its own clock and plays the battle behind it on the
original's (`play.c`). Not done: nothing of it sounds; on the port's own
clock saving takes no time (the original's two seconds in the runs were
the wait for the digit; what its drawing and the file take was not
measured).

2026-10-08: the battle in the window. In the window build `-map N`
alone (no key script, `-passes`, `-clock` or `-ram`) plays the battle
there (`play_live`, `src/play.c`): the port's own clock runs at the
window's time, each picture of the port's video memory and DAC is shown
at the screen's refresh rate (doskit's `frame_wait`, 70.086 a second in
the game's mode), and the player's keys go, byte by byte as port 60h gave
them, to the translation of the keyboard's interrupt (`key_irq`, the same
path as a key script's); the timer's routine reads them as the
original's does. The headless build does the same when `DK_KEYS` or
`DK_SHOTS` script it (doskit's plat_null.c), which is how it was checked
(below). With it the drawing the loop still lacked: each pass's end draws
both cursors and shows the page (`S066A:441F`), a cursor's move draws
its old field again (`cursor_field`, `S0E56:1013`) and the view when it
scrolled, the round's end fades (`fade_grey`, `fade_back`, `fade_out`,
`fade_in` where the original calls them) and draws both views, their
marks and cursors after `attacks_run`, the computer's thinking draws on
the shown page (`S2952:0006`, `S2953:000E`) and its bar of progress
(`S1571:01F2` with `computer_progress`, `S1C74:0354`), and the blasts'
pictures are drawn (`S02E6:225C`, `blast_draw`). `fade_in` and
`fade_out` took the far pointer stored at `DATA:04E3` for the palette,
which lies there itself: the screen stayed black (fixed). Not done: the
mouse, the sounds, the menu in the window (without `-map` the window
still shows the text screen), the pictures of events, saving.

2026-10-08 (branch `debian/window2`): the program in the window. Without
`-map` the window build (and the headless one under `DK_KEYS`/`DK_SHOTS`)
begins at the menu and goes round as `game_loop` does (menu, battle,
menu), on the window's time (`play_live`). The original's setup between
the menu's return and the loop's first pass takes 2.75 s in the runner
(walk `back`: `S066A:0390` at 20.18 s, `S066A:0EE9` at 22.93 s); the
port's clock now lets that time pass before `battle_setup`
(`play_setup`), with the timer's routine going on, as in the original.
The player's mouse: in the window the platform's mouse (held by the
window, `plat_mouse_grab`) moves the driver of `src/mouse.c` as a
driver's interrupt would (`mouse_motion`: the system's counts as
mickeys, 8 pixels per the ratio's mickeys, interrupt 33h function 0Fh,
which `mouse_speed` now sets; within the range), its buttons held as
they are. In a battle the mouse adds to a side's record as the original
does (`input_mouse`, `S27FE:03D4`, and `mouse_center`, `S27FE:04D0`,
translated in `src/input.c`): a pointer the step (record word 20h or
22h) or more from the middle (A0h, 64h) is a direction and goes back to
the middle, the left button is the side's button, its letting go with
the right one held gives ten calls without the mouse. The pictures of
the fades: at each wait of `retrace_wait` and at the end of each
`pal_level` (`screen_hook`, `src/screen.c`) the pictures due by then
are shown, so a fade's steps are seen; the clock's own account of those
waits then finds them shown. The small map's cursor: `pic_draw_save`
(`S2A6B:0008` with its save place; `pic_blit` keeps what lies under
the picture by `under_save`), the pass's end passes the side's place
(byte 5 of `sides`) with 4 in the side's word, and `state_map` puts it
back (`pointer_erase`, `S2904:000E`, now in `src/screen.c`, for the
mouse's pointer too) after a move and before leaving, where it also
draws the view and its marks again as the original does (`S066A:2118`).

Compared: `sh port/tests/menu_test.sh` (all five walks: no byte
different at the menu's return; the walk `back` has a battle with the
mouse open between, and its first `input_mouse` gives a direction in
both, cleared by the setup: before `play_setup` the port's cursor went
one field right and 4 bytes differed), `loop_test.sh` (the same at
every pass compared), `check.py`. In the headless build scripted by
`DK_KEYS` (Space twice): the menu, then map 0's battle on screen; by
`DK_MOUSEMOVE` (200 counts down, the left button): the pointer to GAME
SETUP and that page shown. Not compared: a scripted mouse walk against
the original (the runner takes `-mouse T X,Y,B` and `-mice FILE`, at
places, not motion: not done yet), the small map's cursor in video
memory, the fades' pictures against the original's timing, the feel of
the mickey ratio with a real mouse. Still not drawn: the small map
itself (`S131D:007D`, `S0E56:09AD`).

2026-10-08 (`debian/window2`): the small map. Entering state 3
(`state_map` with 4 in byte 18h) now draws it as `S066A:2400` does:
`small_map_back` (`S131D:007D`: the battle's second background picture
in the side's half and a box in colour F6h) and `small_map_draw`
(`S0E56:09AD`: four boxes `box_3d`, `S28E8:0000`, with lines
`vline_fill`, `S2940:000C`; the map by `umap_draw`, `S151F:000D`, from
UMAP's library and table with the screen's cut on; a line down its left
edge; a dot of four pixels for each unit, `unit_dot`, `S0E56:08F5`).
UMAP's pictures are of the 8-bit kind, so `pic_blit_u` now has its cut
(`pic_cut_u`, `S2AAF:0107`/`0279`, written from what it does to the
screen, not instruction by instruction; its own counts in `S2AAF` are
not kept). Compared: the headless build with `DK_KEYS` (Space held,
right, let go: the small map; down, right: the frame moved without
leaving copies; Space: the view drawn at the corner) against the
runner's `-shot` of map 8 with the same keys after the menu walk of
`play_dumps.py`: the small map's half the same but for the frame, which
stands 33 pixels further right in the original (its cursor was
elsewhere; the port's run began with `-map 8`, not from the menu), 314
of 64,000 pixels different, all of them the two frames.
2026-10-08, the keys F2 to F9 in a battle (`setting_keys` and
`joy_setup` in `src/loop.c`, `num_draw` of `src/draw.c` for F4): F2 the
music and F3 the effects off and on (the bits of `game_state`; the port
makes no sound yet, so the calls that stop and start the music are left
out), F4 a text with two numbers, F5 and F6 a side's joystick (no game
port in the port: the word is switched and no stick found), F7 the mouse
off and on, F8 its speed in three steps, F9 the three settings of the
fights' pictures. The play no longer stops at them.

2026-10-08, the program's end: when the menu ends the program (EXIT TO
DOS on the exit's page) `menu_game` in `src/main.c` does what
`game_loop` does at `S066A:4739`: the mouse's pointer off, the
keyboard's interrupt given back, `mem_alloc`'s blocks freed. Left out as
the machine's part: the screen's old mode, the sound's end, interrupt
24h and the timer. The window build does not get there yet (its menu is
not in the window).

2026-10-08, the new units' screens before a battle (`src/briefing.c`,
the original's `S274D:0003`, `briefing` in the hints): `battle_begin`
calls it where `game_loop` does, behind the computer's file and before
the libraries, unless a saved game is to be loaded (200h in
`game_state`). For a map up to 2Fh whose entry in `NEWUNIT.DAT` is not 0
it shows, one after the other, a screen for a side (its flag between
NEW UNITS and the month and year of the battle) and one for each new
unit (its large and its small picture, its name, the numbers of its
kind's record and some lines of text); each is lit in 33 steps, waits
for a key or a button and goes dark again. Maps 0 to 2 and some others
have none and the battle begins as before. Its files go into the
program's memory where the original's go. In the window (from the menu
or with `-map N`) the screens wait for the player until the window is
closed; a key script
that has no key left for a screen is waited for 400 calls of the
timer's routine, then the screens go on and a line on stderr says so.
`-briefdump DIR` writes the memory at the routine's entry and return and
each screen's video memory into DIR (for the test). Left out: the music
(the side's tune is loaded, not played). The time is the port's own
clock: the files take none, and the 33 steps of a fade are given 19
retraces (0.27 s, the original's 33 calls of `pal_level` in the runner).
The test programs that link `battle.c` by themselves get the screens
without their two texts in the menu's font and without the wait
(`tests/standalone.c`).

2026-10-08 (`debian/window2`): the range's marks drawn again where the
original does, found by comparing each routine's calls of `view_draw`,
`marks_draw`, `marks_erase` and `field_draw` with the port's:
`field_menu`'s move (pointer 4) draws the marks after `move_range`
(`S0CB6:1101`); `view_step`'s step 18h draws the view and its marks after
its `move_range` (`S0CB6:0A2B`); `move_aim` draws the marks at its end
(`S10E1:05B4`, after the marks but the way's were taken off);
`attack_unmark` draws the view and its marks (`S0AE0:02E3`). State
unchanged: `play_test.sh m8_site` and `m8_d5r` and `loop_test.sh` the
same as before. Seen in the headless window (`DK_KEYS`, map 8): the
range marked on choosing a unit's move, the way left marked after the
target. Not compared with the original's screen (the runner's key
script did not bring its cursor to the same unit; not pursued).

2026-10-08 (`debian/window3`): both sides, held keys and the player's
attack in the window, compared with the original. The window's keys
(`play_live`) now go into the keys as a key script's do (`live_key`,
`src/play.c`): each byte at the time of the picture it came with,
handed to the keyboard's interrupt when the clock reaches it (`keys_to`,
at a call of the timer's routine). Before, a byte went to the interrupt
at once, and while the window shows pictures ahead of the clock (the
round's end: `fade_grey`'s 60h retrace waits are shown as they come and
counted by the clock after it, 1.37 s) a tap's press and release reached
the same call and the tap was lost (seen: a step down at 1.2 s into a
half of map 48 did nothing). `-keylog FILE` writes the window's keys as
a key script (the times as they are kept, six places): played with
`-keys` and the same `-start` and `-seed` it is the window's play again.
`menu_attack` (`src/side.c`) draws the view with the attack's marks
(`marks_draw`, `S0CB6:1629` and the two alike before it), which the
port had left out: the original's view goes to its darker colours (+90h)
while the target is chosen.
New tests: `port/tests/live_keys.py` (a key script as `DK_KEYS` for the
headless window, each key at the picture shown at its time),
`play_test.sh TAG live` (the window's play and the `-keys` play of the
same script: every part's inputs and memory and video memory at the
rounds' ends and after them byte for byte), `play_dumps.py` with
`VRAM=1` (the video memory beside each dump; `play_cmp.py` compares the
two pages and counts what lies behind them apart, as
`briefing_cmp.py`), and `play_test.sh` sets `LC_ALL=C` (with a German
locale awk took the clock's times for text and `at` found the wrong
passes).
`m48_attack.keys` (map 48, both sides human, 170 s) was played in the
headless window and written by `-keylog`: the second side ends its halves
with its own keys (the left Alt and X), the first side's cavalry (unit
15h) moves twice and attacks unit 14h beside it (ordered at 117.2 s,
fought at the round's end, 125.5 s in both), then the first side holds
right 1.0 s (eight steps of its cursor) and the second side holds C
1.0 s (four steps, presumably to the view's edge; not checked), D 0.4 s
(three) and taps D (one). The original's run with the same file
(`play_dumps.py m48_attack.keys 170`, `VRAM=1`, `PASSES` at the attack's
screens):
- on the original's clock (`play_test.sh m48_attack`): 3,886 parts the
  same, the game's state and the computer's the same at all 28 passes
  compared, the two pages of video memory the same at all of them
  (choosing the attack, the target chosen, the order, the round of the
  fight and after); 1,442 to 1,914 bytes differ behind the pages, the
  same from the first pass on (not looked into). Before the
  `menu_attack` change the pages differed at pass 1303 (46,754 bytes:
  the view not darkened);
- on the port's own clock, which is the window's (`rounds`): the game's
  state and the computer's the same but for the passes' in all eight
  rounds, the player's attack in round 6 included, the pages the same
  at all 16 dumps; `at` 70.9, 115.3, 116.9, 73.9, 117.5, 138, 146.5 and
  148.5 s the same, at 70.4, 114.8 and 116.7 s not (the port a few passes
  ahead within a gesture: 6 to 14 passes of difference by then; half a
  second later the same);
- `play_test.sh m48_attack live`: the window's play and the `-keys` play
  the same in all 3,918 parts and all 34 dumps.
- Rerun with the runner of doskit 22292c0 (the REP STOS fix), the
  original's dumps made again: all of the above the same, to the byte
  counts.
`play_test.sh m8_d5r` and `m8_site` still the same at every pass,
`m8_d5r rounds` to round 4 as before. The menu in the headless window by
keys (map 48's code and START) still reaches the battle. Not done or not
checked: the window with SDL by a hand (only the headless platform), the
mouse in the window (still handed to the driver at once, so ahead of the
clock in a fade), a `-keylog` of a play from the menu played back with
`-keys`, an attack by the second side in the window, a fight's pictures
in video memory frame by frame against the original in the window.

2026-10-08 (`debian/window4`): a site's screens and the mouse in the
window. Inside a site or carrier (state 2) the port now draws what
`game_loop` draws there (`src/draw.c`): `site_screen` (`S131D:0A1C`: the
background, the side's row of units by `site_units`, `S131D:087A`, and
the site's kind), `site_info` (`S066A:296E` to `2DB8`: the place under
the cursor, a unit's values and large picture or the texts 8, 9 and 2,
and a site's points or a carrier's weight and room), `build_rows`
(`S131D:0B07`, the build list's places) and `build_info` (`S066A:3677`
to `382F`: the kind under the cursor, its price and picture); leaving
the site draws the view and its marks again. The state was already
translated; only the drawing was missing.
The mouse: with `-map` the port now opens the mouse as the menu's return
does (`menu_mouse`; before, a `-map` battle had no mouse at all) and puts
the pointer to the middle, where the original's first `input_mouse` in
the setup leaves it. In the window the platform's motions are kept with
their picture's time and handed to the driver by the clock (as the keys
since `debian/window3`); `-micelog FILE` writes where each moved the
driver as `T X,Y,B` lines, the runner's `-mice` format, and `-mice FILE`
plays such a file. `play_dumps.py` and `play_test.sh` take
`tests/keys/TAG.mice` when it is there, and `live` gives the window the
motions of `TAG.moves` (`live_keys.py --moves`). `make_m8.py` has mouse
steps (`M` before a step).
`m8_mouse` (map 8, 406 s) is `m8_site`'s steps with the site's gestures
made by the mouse: `make_m8.py -moves m8_mouse.moves` with
`e45 g55 s64 l67 d69 E70 f95 f100 f105 f110 E116 f150 f155 f160 f165 f170
u182 L183 Ms185 Ms189 Ms192 Ml195 d197 E198 f205 f210 f215 f220 f225 f230
f235 E245 f250 f255 f260 f265 f270 f275 f280 f285 f290 f295 f300 u312 L313
Ms315 Mr322 Ml325 d327 E328 f335 f340 f345 f350 f355 f360 f365 f370 f375
u388 L389 Ms391 Mo398 MR400 Ma401`, played in the headless window, and
`m8_mouse.keys` and `.mice` are what the window wrote. By the mouse the
first side enters the site, opens the build list and builds (unit 41h,
kind 12h, points 48 to 13), enters again and repairs that unit (offered,
answered by a text, no points taken: it was not damaged; which text was
not read), and moves it out and orders it to a field (view step 3).
(`m8_site`'s R steps were left out: the keys' left still held after
entering steps the cursor back, the mouse leaves no direction held.)
Compared with the original's run of the same files (`play_dumps.py`,
`VRAM=1`, `PASSES` at 186 to 405 s: 2858 2940 2994 3040 4788 4915 4969
6460 6570 6620 6675), rerun after the merge of origin/master 8273158:
- `play_test.sh m8_mouse`: 13,384 parts' inputs the same, the game's
  state and the computer's the same at all 28 passes, both pages of
  video memory the same at all of them, the site's screen, build list,
  built unit, repair and move out included. With the new drawing taken
  out the pages differ by 47,864 bytes at those passes;
- `live`: the window's play and the `-keys`/`-mice` play the same in
  15,532 parts and all 26 dumps;
- `rounds`: the same to round 3, then a few bytes differ, as
  `m8_site rounds` does without the mouse (not looked into).
`m8_site` and `m48_attack` still the same at every pass, pages included.
Not done or checked: the SDL window by hand, a real repair (text 51h)
and a carrier's screen by the mouse (`m0_s2c` keeps them by keys, its
pages not compared), a scrolled build list, the mouse's mickeys left
over below a pixel (`-micelog` does not keep them).

2026-10-08, the field menu's information (`unit_info`, `unit_values`,
`info_rect` and `battle_info` in `src/draw.c`, the original's
`S131D:0483`, `0148`, `0008` and `0EE5`; `state_field` of `src/loop.c`
calls them where `game_loop` does). With the button held, down and the
button let go a side sees, on a unit: its large picture, what its kind
can do (weapons and ranges against air, ground and water, armour, speed,
weight, group size) and the unit on its ground; on a field without a
unit: the battle's numbers over the darkened view (round, the map's
code, mode, the best and the current score, both sides' units, stores,
factories and resources, the moves' limit). Leaving it by the button
now draws the view and its marks again (`S066A:24DB`), which the port
had left out: the shown page kept the old picture. `latch_copy` of
`src/fightpics.c` is no longer static. With `-map N` the row LEVEL is
empty (the code is the menu's text). `-infodump DIR` writes each call's
memory and video memory into DIR, and `-dump N FILE` writes the video
memory when FILE ends in `.vram` (both for the test).

2026-10-08 (macOS, branch `mac/sound`): the sound's routines, in
`src/sound.c` over the program's memory, without any sound coming out.
`snd_init` (both devices found, `snd_devices` 3), `sfx_play` and what it
reaches (`dig_play` for an 8SVX effect: the channel's record filled;
`adl_play`, `adl_load` for an ADLX one), `snd_pass` with `adl_start`
(which takes OPL voices for an effect and keeps the tune's), `chan_stop`,
`adl_stop` with `adl_voices_free`, and the timer's routine `adl_tick`
with the events of a tune and of an effect (`music_events`,
`effect_events`) and the driver's voice routines. Called where the
original calls them: `start_setup` (`snd_init`), `game_loop` (the
cursor's step, the end of a side's part with 100h in `game_state`'s word
4, `snd_pass` at a pass's end; `src/loop.c`), the fight's panel
(`panel_sound`, `src/draw.c`) and the menu's sounds (`src/menu.c`).
`adl_tick` is called from the port's own clock (`snd_clock` in
`src/play.c`'s `own_to`) at the period the tune or effect set, so an
effect ends as in the original; on the original's clock (a battle played
with `-clock`) it is not called and an effect stays playing until the
next one on its channel. What the original writes to the AdLib's
registers goes to the hook `snd_opl` (unset). `-sndlog FILE` writes each
call of `sfx_play` (time, channel, number, result).
Not translated: the music's start, fade and volume (`music_start`,
`music_fade`, `music_volume`, `fade_tick`: read, in the hints), the
digital mixer and the card (`dig_mix`, `dig_irq`, `dig_kick`), `snd_end`,
`dsp_rate`, and the calls of `sfx_play`, `dig_play` and `chan_stop` by
`attacks_run`, `event_picture`, `site_picture` and the fight's pictures
(`src/orders.c`, `src/fightpics.c`: still left out there).
docs/HANDOFF.md ("The sound") says what sound output would need.

## Build and run

    sh port/build.sh          # macOS, Linux (SDL2 for the window)
    port\build.bat            # Windows (MSVC)
    port/build/history-line -game game
    port/build/history-line-headless -game game -map 5 -ram build/map5.ram
    port/build/history-line-headless -game game -map 8 -keys port/tests/keys/m8_site.keys -start 31.1 -seed 563 -passes 2000 -ram build/played.ram
    port/build/history-line-headless -game game -keys build/menu/back/keys.txt -start 8.05 -until 54 -menulog build/menu.log
    port/build/history-line-headless -game game -keys build/play/m48_save/keys.txt -start 8.05 -clock build/play/m48_save/clock.txt -saves build/saves

Both scripts define `PORT_VERSION` (a string) for the compiler when
there is a version: the environment's `PORT_VERSION`, else the tag of the
commit built; the workflow sets it for a tag's build. Without one it
stays undefined.
`PORT_UPDATE_URL` likewise, from the environment only: where a release
looks for newer ones (doskit/runtime/update.h); the workflow sets it to
the latest release's `latest.json`.

## Releases

`.github/workflows/build.yml` builds the packages doskit/docs/RELEASE.md
prescribes; a pushed tag `vX.Y` makes a release of them. `dist/README.txt`
is the players' README in each package: fill in the game's keys and
anything the game needs before the first release.

(which packages were started from a download, on which systems)

## Checked

- The sound's routines (2026-10-08, macOS; `src/sound.c`): `python3
  port/tests/sound_dumps.py` stops the original at the entry and the end
  of calls and keeps its memory: in the menu (the keys of
  `menu_dumps.py`'s walk `pages`) `snd_init`, all 38 calls of `sfx_play`,
  the `snd_pass` behind each and 141 calls of the timer's routine
  `adl_tick` (the first twelve after each of the first three effects
  began, and those in which an effect ended); in a battle (map 8,
  `m8_site.keys`) 35 of the 1,191 calls of `sfx_play` (the first six,
  then every 40th), the `snd_pass` behind each, the four calls of
  `dig_play` by the fight's pictures and twelve of `chan_stop`. `sh
  port/tests/sound_test.sh` gives the port's routine the entry's memory
  and arguments and compares the driver's variables (`S30EF:0000` to
  `111E`) and the result with the end's: 304 calls, none different. In 41
  the timer's interrupt came in while the original's routine ran; there
  the count of ticks is taken from the original and a record's due tick
  may be lower by the ticks that came in (none was). Memory outside
  `S30EF` is only counted (the stack, and the timer's and keyboard's
  counts when an interrupt came in). `snd_init`: the port leaves the
  interrupts' old vectors (`S30EF:0489`, `10BF`), the probe's flag
  (`0495`), `delay_measure`'s counts (`111B` to `111E`) and the timer's
  number (`1116`) 0; `snd_devices` is 3 in both and `snd_init` returns 3
  in both, and nothing else of the driver's variables differed. (Until
  doskit `22292c0` the runner had 048Bh there, its fault with REP STOS,
  docs/HANDOFF.md; the numbers here are the fixed runner's, see the rerun
  below. `sound_test.c` now judges `snd_init`'s result too: against the
  old runner's dump it fails on it, as it should.)
  Which effect plays: the port's program walked the same menu (`-sndlog`)
  and played the same 38 effects (numbers 0, 1 and 2 on channel 8) in the
  same order with the same results, at most 0.09 s apart; and the battle
  of `play_test.sh m8_site` on the original's clock: the original's 1,185
  calls from the first pass on against the port's 1,186 (one more at the
  clock's end, after the original's run was cut off; not looked at
  further), the results the same in order, channel and number the same in
  the 29 of them whose entry was kept. Once, by a log of the three call
  sites in the original (not in the test): 694 from the cursor's step,
  415 from the end of a side's part and 76 from `panel_sound`, in the
  order of the port's calls, the step's number and channel (4 on 0Bh, 5
  on 9) right in all 694.
  After the change the other comparisons were run again: `menu_test.sh`,
  `play_test.sh m8_site`, `fkeys_test.sh`, `info_test.sh`,
  `briefing_test.sh`, `loop_test.sh` (m8_site), `fight_test.sh`,
  `units_test.sh` and `load_test.sh` as before (`load_test.c` now sets
  `snd_devices` beside in `start`).
  Rerun with the fixed runner (2026-10-08, macOS, doskit `22292c0`,
  `dosrun` rebuilt; the tree at master `8ad578a`): the original's dumps
  were all made anew (`menu_dumps.py`, `play_dumps.py`, `sound_dumps.py`,
  `save_test.sh dumps`, `fkeys_test.sh dumps`, `info_dumps.py`,
  `briefing_dumps.py`, `loop_dumps.py m8_site.keys 400` with the passes
  of before), the old runner's kept beside them (`build/old-runner/`,
  ignored), and each test's output compared with its output on the old
  dumps:
  - `sound_test.sh`: 304 calls, none different, 41 with the timer in
    between, as before; `snd_init` returned 0003 in both (before: the
    original 048Bh) and lists 13 bytes the port leaves to the machine
    instead of 15, the two of `snd_devices` gone; the lines of the other
    303 calls are the same as before; the menu's 38 effects and the
    battle's 1,185 against 1,186 calls as before.
  - `menu_test.sh` (six walks), `briefing_test.sh` (four maps) and
    `loop_test.sh` (m8_site, 29 passes): the output the same line for
    line.
  - `play_test.sh m8_site` (31 passes), `save_test.sh`, `fkeys_test.sh`
    and `info_test.sh`: every line the same but the count "the setup's
    differences", lower by 2 at every pass: the two bytes of
    `snd_devices`, which the port's setup had right and the old runner
    had not. The saved game of the original is the same file under both
    runners and the same as the port's. (In `play_test.sh` the passes
    4544, 6215, 6216 and 6581 count 3 bytes fewer "elsewhere" after master `8ad578a`
    was merged; on the same new dumps the tree before the merge had the
    old counts, so that is the merged change to the port and not the
    runner. Not looked at further.)
  - `load_test.sh` on its one `start` call (`build/n2/start1.py`, a
    scratch script, run again): 0 different with `snd_devices` no longer
    set beside in `load_test.c`; against the old runner's dump the test
    now fails at that word.
  What the fix changed in the original's memory, new dump against old
  dump of the same place, all 640 KB: in the 31 passes of m8_site over
  400 s, the 30 dumps of `loop_dumps.py`, the 49 passes of the four map
  48 runs and the 8 dumps of the briefing only the two bytes of
  `snd_devices`; at the menu's return, at `game_loop`'s `S066A:0349` and
  in the first `sfx_play` dumps also the word of `game_loop`'s frame it
  keeps `snd_init`'s result in (`[BP-2Ah]`); at `snd_init`'s end two more
  words below the stack pointer (not looked at; presumably what was
  pushed of SI). The logs of what came in (`inputs.txt`) are the same;
  the clock's file of m8_site has four more lines of a kind the newer
  `play_dumps.py` logs and no other change.
  Not rerun: `fight_test.sh`, `units_test.sh` and the rest of
  `load_test.sh`, `orders_test.sh`, `move_test.sh`, `range_test.sh` and
  the others whose dumps are in `build/n2` to `build/n6` and
  `build/computer`: those dumps are still the old runner's (their owners
  rerun them); the play tests of other keys than those named; anything
  on Windows or Linux.
  Not checked or not reached: a failing call (an effect's number beyond
  the file, more voices needed than free), an effect played more than
  once or endless, an 8SVX effect through `sfx_play` (the four `dig_play`
  calls came from the fight's pictures), `chan_stop` of an AdLib channel,
  an effect taking voices the tune uses in a battle (in the menu the tune
  played during every call compared), `music_events`' end of a tune,
  `snd_clock`'s timing against the original's interrupts (the port's
  ticks are counted on its own clock, the due ticks therefore not the
  original's), the registers' values sent to `snd_opl` (only counted),
  the Windows and Linux builds (`build.bat` has the new file; not built),
  and `orders_test.sh` and `move_test.sh`, whose dumps in `build/n2` were
  not all there.
- The field menu's information (2026-10-08, macOS): `python3
  port/tests/info_dumps.py` runs the original through its menu by keys to
  map 48 (two human sides) with `tests/keys/m48_info.keys`: the first
  side asks for the information on its starting field (no unit: the
  battle's numbers), leaves it, moves onto a unit and asks again; then
  the second side the same with its keys. It keeps the memory at each
  call's entry (`S131D:0EE5`, `S131D:0483`) and at its return in
  `game_loop` (`S066A:2473`, `S066A:2434`), the video memory at the
  return, and the video memory of the first pass a second later, while
  the information is shown. `sh port/tests/info_test.sh` lets the port's
  program walk its menu and play on the original's clock (`-clock`,
  `-infodump`) and compares (`tests/play_cmp.py`, `tests/info_cmp.py`).
  The four calls are the same in order and kind (battle, unit, battle,
  unit; two in each half of the screen). At every call's entry and
  return no byte differs in the game's state (S331F), the computer's
  (S39B6), the screen's record (`DATA:00DA` to `00F3`) and `game_loop`'s
  block (40000h bytes from 40h behind its beginning), and none in the
  200 lines of either page of video memory at the four returns and at
  the four passes a second later (behind the pages the 810 bytes of the
  original's start, as in the menu's test). The second comparison first
  found 21,888 bytes of the shown page different at the second call: the
  port did not draw the view again when the information was left
  (`S066A:24DB`); fixed, see above. Told, not judged: of the whole image
  5 bytes of DATA differ at each return and did not at the entry (the
  timer's count `DATA:0308`, three bytes of the input's records,
  `DATA:0B19`: the original's clock goes on inside the routine, the
  port's does not) and 26 to 36 of the stack. `play_cmp.py`: the inputs
  (1,482 parts), the game's state and the computer's the same at every
  pass compared. The window's way was tried headless with `DK_KEYS` and
  `DK_SHOTS` (`-map 48`): the battle's numbers and the unit's screen
  drawn as in the original's pictures but for the empty LEVEL row. Run
  again after the change and the merge of origin/master b70875e:
  `briefing_test.sh`, `menu_test.sh`, `save_test.sh`, `fkeys_test.sh`,
  `play_test.sh`, `load_test.sh`, `computer_test.sh`, `loop_test.sh`.
  Not reached or not compared: a unit of two parts (40h in its word 4:
  the two grounds and halves and the copy in video memory), another
  side's unit (text 1) and a unit with 4 in its word 6 (text 11h), a
  kind without a weight (the text at `S330E:0008`), a side whose byte
  16h of `sides` is not FFh (the number instead of text 13h), 8 in
  `game_state`'s word 2 (the other side's resources hidden), a map with
  a computer's side; `big_unit`'s error exit; the window itself (only the
  headless build was run); what the pointer does in the information
  (state 6, into a site) beyond the pass a second later.

- The new units' screens (2026-10-08, macOS): `python3
  port/tests/briefing_dumps.py` runs the original through its menu by
  keys (a map's code typed, START, then a key every five seconds) and
  keeps its memory at the entry of `S274D:0003` and at its return
  (`S066A:09BD`) and the video memory of each screen lit, before it waits
  (`S274D:0AC2`), and at the return; `sh port/tests/briefing_test.sh`
  lets the port's program do the same on its own clock (`-briefdump`)
  and compares (`tests/briefing_cmp.py`). Four walks: `m4` (map 4, both
  sides have a new unit: four screens), `m12` (map 12, five screens, two
  units one after the other), `m28` (map 28, which is map 4 for the
  other country: the same four) and `m0` (map 0: none, the routine
  returns at once). In all four no byte differs at the entry or at the
  return in the game's state (S331F, but for the dword of a battle's
  passes), the menu's segment (S37FF), the palette (`DATA:04E3`, 300h
  bytes) and `game_loop`'s block from 40h behind its beginning for
  40000h bytes (where the routine's files go), and none in the 200 lines
  of either page of video memory at any of the 13 screens or at the four
  returns (behind the pages the 810 bytes of the original's start, as in
  the menu's test). Of the whole image, what differs at the return and
  did not at the entry is told: the stack (66 to 72 bytes; 23 on map
  0), 78 bytes of S30EF (`S30EF:0292` to `0397`; the original's code at
  `S30EF:1514` writes there from the first screen on, presumably the
  tune the port does not play) and `DATA:0B19` on maps 4 and 28 (which of
  its four calls the timer's routine is at: the two clocks). After the
  merge of origin/master be11493 (the menu and the player in the window)
  the comparisons were run again with the same result; told besides on
  map 12 then: 12 bytes of the input's records (`DATA:03CF` to `0410`)
  and a byte of the BIOS's count of ticks (`0:046D`), presumably the two
  clocks again (not looked at further). The port
  showed as many screens as the original and left the last at 41.3 s
  where the original returned at 41.9 s (map 4; the keys' times decide).
  The window's way was tried headless with `DK_KEYS` and `DK_SHOTS`
  (`-map 4`, a key every 300 pictures): the four screens, each until its
  key, then the battle; and with `DK_FRAMES=300` and no key (the window
  closed during the first screen): the program ends. Also run after the change:
  `menu_test.sh` (all six walks the same as before), `save_test.sh`,
  `fkeys_test.sh`, `play_test.sh`, `load_test.sh` (108 calls, 0
  different), `computer_test.sh` and `loop_test.sh`. Not compared or
  not done: the DAC's colours during the fades (the palette's memory is
  compared, the levels are not), the times, the sound; a screen left by
  a mouse's button or a joystick's (the same test of the input records,
  not tried); a file that cannot be loaded; maps other than 0, 4, 12 and
  28, so most kinds' numbers and texts (of the five kinds shown one has
  a weight, byte 41h not 0: map 12's last; the others the text for
  none); the window itself on a screen (only the headless build was
  run); `-map N` with `-clock` on a map with screens (the original's log
  has the screens' time before its first pass; not tried). `view_test`,
  `side_test`, `move_test`, `income_test` and `orders_test` link again
  (they had not since `battle_begin` calls `load_game`) but were not
  run: their dumps of the original are scratch files that are no longer
  there.

- EXIT TO DOS and the program's end (2026-10-08, macOS): a sixth walk of
  `tests/menu_dumps.py` and `tests/menu_test.sh`, `exit` (Esc, the button
  on EXIT TO DOS). What came in is the same (3 things), no byte differs
  at the menu's return, nor in video memory at the exit's page. A second
  run of the original is stopped where the end calls its last routine
  (`input_remove`, `S27FE:0060`) and compared with the port's memory at
  its end (`-ram`): the game's state, the menu's segment, the count of
  `mem_alloc`'s blocks (`DATA:0467`, 0 in both) and `game_loop`'s block
  the same; in the mouse's record 3 bytes of the pointer's place (words
  10h and 12h) differ and are told apart: `game_loop` opens the mouse
  again behind the menu, the original's driver then has the pointer in
  the middle, the port's where the menu left it (the same difference a
  battle has from its first pass on; `src/mouse.c` was not changed).
  Not compared: the screen's mode, the vectors and DOS's memory after
  the end; what `main` does behind `game_loop`'s return.

- The keys F2 to F9 (2026-10-08, macOS): `sh port/tests/fkeys_test.sh
  dumps` and `sh port/tests/fkeys_test.sh`, as the saving's test (both
  programs through the menu by keys to map 48, the port's battle on the
  original's clock), with `tests/keys/m48_fkeys.keys`: F2, F2, F3, F3,
  F4, F5, F5, F6, F8 three times, F9 three times, F7, two seconds apart.
  Memory a second after each key: 1,470 parts' inputs and the game's
  state the same at all 20 passes compared, and of DATA the joysticks'
  words (0380 to 03BC) and the mouse's record (0414 to 045C but the
  pointer's place, 0424 to 0428, which differs from the first pass on:
  the runner's mouse and the one the port's menu left) the same too
  (`tests/range_cmp.py`). Seen in a first run with F7 pressed twice and
  left out of the script: when F7 turns the mouse on again, the
  original's first side reads a direction once (input word 8 in pass 447
  of that run, where the port's had 0) and its cursor moves a field;
  that is `input_mouse` (`S27FE:03D4`), which the port does not have.
  Not compared: the texts as video memory (F4's numbers among them),
  the sound (F2 stops and starts the music in the original), F2 and F3
  on a machine without music or effects (10h, 20h of word 2 clear), the
  keys while 200h is set (pressed again within ten passes), a computer's
  first side (the text then goes to the second side's line), F6 pressed
  twice, a joystick that is there, F8 without a mouse.

- Saving and loading (2026-10-08, macOS): `sh port/tests/save_test.sh
  dumps` runs the original twice through its menu by keys
  (`tests/play_dumps.py` with `WALK` and `STATE`): `m48_save` (the code
  of map 48, START, both sides end their half, D and 3 save the battle,
  later D and Esc leave the saving) and `m48_load` on a folder with the
  file that run wrote (MISC, LOAD, 3; the second side goes back into its
  half and ends it again, the F1 pair carries the round out). `sh
  port/tests/save_test.sh` lets the port's program do the same: its own
  menu on its own clock, the battle on the original's clock
  (`-clock`), its own saved game loaded in the second run. Result: the
  port's `03.DAT` is the original's byte for byte (35,411 bytes); in
  `m48_save` 1,344 parts' inputs and the game's state the same at all 13
  passes compared (the passes before, with and after each saving among
  them: 490 to 492, 497, 600 to 602, 607); in `m48_load` 1,190 parts'
  inputs and the state the same at all 11 (the loaded battle's passes 1,
  2, 3, 10, 50, 100, 200, the round's end in 229, 230, 400, 594). Not
  compared: video memory while the number is asked for (the dark view
  and the three texts) and after; text 5Ch (the file cannot be made); a
  file that is not a saved game or ends early; a battle with the
  computer's side saved and loaded (map 48 is for two humans, so the
  computer's blocks were written and read but hold nothing in use); a
  saved game of another map than the menu's (here the menu stood at map
  0 when map 48's battle was loaded, which is such a case for the state
  but not for the libraries of pictures a map chooses, which the port
  does not compare); a game the original saved loaded by the port
  without the port having written it (the two files are the same bytes).
  The older tests still pass after the change (`menu_test.sh` all five
  walks, `play_test.sh` `m8_site`, `m48_two`, `m57_last`).
- The menu's loading, further (2026-10-08, macOS, branch
  `mac/menuload`, on origin/master fe0a39c with the runner of doskit
  22292c0; all of the original's dumps named here made anew):
  - `python3 port/tests/menu_dumps.py noload`, `sh
    port/tests/menu_test.sh noload`: MISC, LOAD and 5 with no saved game
    (not found, left by a key), LOAD again with a letter, the backspace
    and space (none taken) and Esc, LOAD and 0 (not found), then the
    main menu, map 57's code and START. 15 things came in, the same in
    both; no byte different at the menu's return (the game's state, the
    menu's segment, the mouse's record, 40000h bytes of the block); no
    byte different in the two pages at five passes of the menu's loop
    and at the five screens of the question (three times the question,
    twice the text; `-askvram` against the original stopped at
    `S167A:1929` and `S167A:1B5A`). All seven walks of `menu_test.sh`
    pass.
  - `sh port/tests/save_test.sh dumps`, `sh port/tests/save_test.sh`:
    `m48_load` now at 234 passes, every one from 1 to 232 (the round's
    end is in pass 229) and 400 and 594: 1,190 parts' inputs the same,
    the game's state and the computer's 0 bytes different at each.
    `m48_save` as before (13 passes, the file the same 35,411 bytes).
    Four damaged files made from that saved game, each loaded from the
    menu by its digit and followed by the F1 pair: `load_bad` (00.DAT,
    second byte 6; 647 passes, 9 compared), `load_cut` (09.DAT, the
    first 7000h bytes; 595 passes, 11 compared, the round's end in 174
    with it), `load_head` (01.DAT, ten bytes; 648 passes, 9 compared),
    `load_none` (02.DAT, empty; 647 passes, 9 compared): the inputs,
    the game's state and the computer's the same at every pass
    compared.
  - Not compared or not run: video memory of a loaded battle (the
    libraries of pictures a map chooses, so a saved game of a winter
    map loaded with the menu at a summer one is still open); the
    question left by a joystick's or the mouse's button; a file that
    cannot be read after it was opened (a DOS error); a file cut inside
    the first blocks (the maps, the units), where the state is half one
    map's and half another's; the digits other than 0, 1, 2, 3, 5 and
    9; a saved game found in the game's folder and not the port's; the
    window build (loading was only run in the headless program, by key
    scripts); `play_test.sh` and the other tests were not run again on
    this branch after the merge of fe0a39c (the port's change here is
    the option `-askvram` and the limit of `-dump` options, 512 for 64).

- The menu (2026-10-08, macOS): `python3 port/tests/menu_dumps.py` runs
  the original with keys that walk its menu and stops it where
  `game_loop` has the menu's return (`S066A:0390`); `sh
  port/tests/menu_test.sh` starts the port's program at the menu with the
  same keys, on its own clock from the second the original's first pass
  began, and `tests/menu_cmp.py` compares what came in (pass by pass:
  chosen, the other page, another item, the return) and the memory at
  the last return: the game's state (`S331F`), the menu's segment
  (`S37FF`), the mouse's record (`DATA:0414` to `045B`) and 40000h bytes
  of `game_loop`'s block from 40h behind its beginning (the menu's
  pictures, libraries and data with the items' rectangles and choices).
  Five walks, all the same in what came in and with no byte different:
  `code57` (the code of map 57 typed, START), `start0` and `start24`
  (START on map 0 with either country), `pages` (GAME SETUP with each
  row chosen, the exit's page and back, CONTROLS with two rows chosen,
  MISC, the best scores shown, then as `code57`: 36 things came in) and
  `back` (as `code57`, in the battle Esc and the confirming key, the
  menu again, START: the second return compared; the dword that counts
  the battle's passes at `S331F:4CB7` is 130 in the original and 190 in
  the port and is told apart). The times of what came in are at most
  0.54 s apart. Video memory is compared at 13 passes of the walks (the
  main menu, GAME SETUP, the exit's page, CONTROLS, MISC, the typed
  code, the menu after the battle): no byte different in the 200 lines
  of the page drawn and of the page shown, the mouse's pointer
  included; behind the pages 810 bytes differ, left by the original's
  start, which the port does not do. Not compared: the screens that are
  not passes of the loop (the country's, the best scores, the code
  while typed), the first 40h bytes of the block (10 bytes differ there, in
  the first file `start_setup` loads; what the original's part that is
  not translated does there was not looked at), the rest of `DATA` (197
  bytes differ at the first return: the C library's start, the sound,
  counts that run with the clock; not looked at one by one), the name of
  a winner and the file written, a saved game, the program's end from
  the menu, a click, a walk with the mouse moved. The earlier tests were
  run again after it (`play_test.sh` `m57_last`, `m57_site`, `m8_site`,
  `load_test.sh`, `fight_test.sh`, `units_test.sh`: as before).
- The program's own play (2026-10-08, macOS): `python3
  port/tests/play_dumps.py KEYS UNTIL` runs the original with a key
  script and logs its clock (the timer's routine, the keyboard's
  interrupt, the places where a pass reads the input, `clock_seed`,
  `rand_next`, the waits' and pictures' clearing of the keys) and what
  each side's part read, and dumps memory at the beginning of chosen
  passes; `sh port/tests/play_test.sh TAG` starts the port's program from
  its own setup with the same key script and that clock
  (`-keys`, `-clock`, `-dump`) and `tests/play_cmp.py` compares. Nothing
  of the game's state comes from the original. Results, each "the
  inputs, the game's state and the computer's the same at every pass
  compared", with no place where the clock's file did not fit:
  - `m8_site` (map 8, 400 s): 6,582 passes played; the 13,164 parts read
    the same key and input word, and where the original's `view_step`
    tested bytes 5 and 8 of the record the port had the same; 17 passes
    compared (1, 50, 200, 400, each pass with `attacks_run` and the one
    after it: 291, 1939, 2615, 4284, 4543, 6215, and 6581).
  - `m0_hq` (map 0, 300 s): 4,574 passes, 9,148 parts, 15 passes
    compared (rounds' ends 373, 1036, 1131, 2431, 2937).
  - `m8_d5r` (map 8, 300 s): 4,428 passes, 8,856 parts, 42 passes
    compared (rounds' ends 530, 2176, 2265, 3896, and 420 to 439 one by
    one, where a difference was looked for).
  The game's state is compared as `loop_test` does (`S3306` up to `DATA`
  but the computer's segments, the two maps, the sides' work buffers),
  the computer's likewise (its segments, `S331F:17F2` to `181B`,
  `move_args`, `S3AEC:0000` to `0007`, `rand_words`). Counted apart: the
  setup's differences (772 bytes of the game's state at map 8's first
  pass: the pictures' records and pointers, a letter of three file
  names, the map's number as text and the menu's code; listed by the
  test and in docs/HANDOFF.md), the two far pointers of grounds'
  pictures a fight puts into `fight_rec` (1Eh, 22h; the port has none),
  and the rest of memory (pictures, the interrupts' data, the stack).
  `sh port/tests/loop_test.sh` still says the same for `m8_site` with
  the changed `loop.c`, and of `move_test.sh` the 40 steps whose dumps
  are on this machine are as before with the text shown in `move_step`.
  The port's own clock (2026-10-08, macOS): `sh port/tests/play_test.sh
  TAG own` plays without the clock's file (of the log only the first
  pass's time and the BIOS's count at the setup, as `-start` and
  `-seed`) and says up to which pass dumped the state is still the
  original's. With the round's end lasting its retraces: `m8_site` the
  same at passes 50, 200, 291, 292, 296, 386 and 400 to 404 (before
  the change it left the original in pass 291), five bytes different
  at 387 and different from 405 on; `m8_d5r` the same at 50 and 200,
  one or two bytes different from 400 on; `m0_hq` the same at 50,
  different at 200. It cannot stay the same for long: the keys are
  sampled every 54.9 ms, so a tap of 0.08 s moves the cursor one field
  or two by when it falls, and that is the original's time
  (docs/HANDOFF.md, "The port's own clock", which also lists what the
  clock's file is still needed for and why). The test with the clock's
  file was run again after the change, all three scripts as above, and
  `loop_test.sh` (`m8_site`).
  A round's end with fights or a site taken on the port's own clock
  (2026-10-08, macOS). `play_test.sh TAG own` now also prints how long
  each round's end lasted in the original and in the port, and
  `play_test.sh TAG rounds` compares the two round by round: the port
  at the pass of its n-th round's end and the one after against the
  original at the passes of its n-th, because the port's passes are
  shorter (the computer's turn takes it less time) and a round's end
  comes in another pass, often at another second. What counts passes is
  then counted apart ("the passes'": the count of passes and its copy in
  `fight_rec`, the timers' kinds and due passes, byte 2Ch of the views,
  `rand_words`; 3 to 19 bytes). New runs of the original
  (`tests/play_dumps.py`, with `NOPICS=1` for fights without pictures):
  `m8_wait.keys` (the half ended every 10 s and F1 5 s later, no taps),
  `m8_wait_nopics`, `m8_d5r_nopics`. The lengths, seconds from
  `attacks_run`'s call to the round's end, original and port:
  two fights with pictures 43.25 and 43.21 (`m8_d5r`), one 22.10 and
  22.06 (`m8_site`) and 22.07 and 22.06 (`m8_wait`), several 52.05 and
  52.09 (`m8_site`; its port had left the original before) and 52.05 and
  52.08 (`m8_wait`); without pictures 11.05 and 11.01 (`m8_d5r_nopics`,
  two fights), 10.34 and 10.27, 25.34 and 25.31, 35.38 and 35.36
  (`m8_wait_nopics`); a site taken 11.39 and 11.34 (`m0_hq`, where the
  port's play had left the original long before, see below) and 11.42
  and 11.34 (`m8_wait`, round 6; not looked at whether that is a site
  taken too); a round's end without either 1.46 and 1.37. Round by round
  the game's state and the computer's are the same but for the passes':
  `m8_d5r` to round 3, the round of its two fights with pictures and the
  last the port reaches in the original's number of passes (pass 2266
  of the original, 175 s, and 2635 of the port); `m8_d5r_nopics` to
  round 3 likewise; `m8_site` to round 3, its first fight (pass 2616,
  173 s), 3 to 5 bytes different in round 4 and 14 in round 5;
  `m8_wait` to round 4 (pass 3168, 183 s; a fight in round 3), ten bytes
  different from round 5 on, a round of several fights (`side_points` of
  the second side and unit records: presumably another kind built with
  other random numbers, not followed); `m8_wait_nopics` to round 4, one
  to five bytes different after; `m0_hq` in no round (its taps are read
  otherwise from pass 101 on, as before). Pass by pass (`own`) the
  results are as before the change: `m8_site` to pass 386 and 400 to
  404. The test with the clock's file says the same as before for
  `m8_site`, `m8_d5r` and `m0_hq`, and `loop_test.sh` and
  `fight_test.sh` pass. `units_test.sh` does not link since the merge
  of the libraries' loading (`lib_load` in `src/files.c` needs
  `file_name`, which the test does not compile in); not changed here.
  Where the rounds part, and why (2026-10-08, macOS; docs/HANDOFF.md,
  "Where the rounds part"). `play_test.sh TAG rounds` now lets the port
  play to the second the original's log ends at (`-until`; before, to
  its number of passes, which the port's shorter passes are through
  sooner) and finds the passes of the port's rounds itself (two plays);
  the bits 4 and 8 of `game_state`'s word 0 (a side's text shown, taken
  off by a timer) are counted with the passes'. `play_test.sh TAG seeds`
  is the same with `-seeds`, and compares `rand_words` too. Results,
  the game's state and the computer's but for the passes':
  - `m8_wait`: `rounds` the same to round 4, `seeds` in all six rounds
    of the log (12 dumps). The first byte that differs without the
    seeds is in `rand_words` after round 3's two fights (the original's
    last seed 08E2h ticks, the port's 0774h: its round 3 ends at 90 s,
    the original's at 110 s); the computer draws from it in pass 3277
    of the original (three calls of `rand_next`) and builds other kinds.
  - `m8_d5r`: both the same in all four rounds dumped (the port now
    reaches round 4); `rand_words` the same only with `seeds`, which
    also makes the four calls of `rand_next` the two fights' pictures
    made.
  - `m8_site`: both the same to round 3; from round 4 five bytes differ
    with `seeds` too (two places of `sites_400` record 0 exchanged and
    the first view's word 0Ah): the up of 182.00 s is read in two
    passes by the original and in one by the port (the tap's sampling,
    as above). Without the seeds 14 to 15 bytes from round 5 on, with
    them the five only, to round 6.
  - `m8_d5r_nopics` the same in its four rounds and `m8_wait_nopics` to
    round 5 either way (no seeds without pictures); from round 6 to 8
    four bytes of the first view differ, its column: a left of the
    script that the original's view does not read comes in the port
    when the computer is already done.
  The port cannot find the seeds or the pictures' calls itself (why, in
  docs/HANDOFF.md); with them given, nothing else parts the plays in
  these runs but the taps and that left. The test with the clock's file says the same
  as before for `m8_site`, `m8_d5r` and `m0_hq`.
  A human second side (2026-10-08, macOS; docs/HANDOFF.md, "A human
  second side"). Map 48 is for two players; `tests/keys/make_two.py`
  writes scripts with keys for both sides (the second's: X, V, D, C and
  the left Alt), and `play_dumps.py` and `play_test.sh` take the map
  from a script's name (`mNN_`). Three runs of the original, compared
  with the port from its own setup:
  - `m48_two` (`make_two.py 45:400`: both sides end their halves every
    15 s, F1 twice; 400 s): on the original's clock 5,493 passes, 10,986
    parts read the same, 53 passes compared (24 rounds' ends), "the
    inputs, the game's state and the computer's the same at every pass
    compared"; on the port's own clock (`rounds`) the same in all 24
    rounds.
  - `m48_steps` (`45:400 seed=7`: the second side's cursor steps and
    button with each direction, eight a round from a fixed sequence, and
    steps of the first side; 400 s): on the original's clock 5,847
    passes, 11,694 parts, 37 passes compared (16 rounds' ends), the
    same at every one. On the own clock the same to round 1's end only
    (its steps are sampled otherwise).
  - `m48_move` (`2R45 2R46 2u47 2m48.5 2R52 2a54 2a56 1E58 2E60 2f64
    2f64.8 2m70 2R73 2a75 2a77 1E79 2E81 2f85 2f85.8 95:200`; 200 s):
    the second side moves a unit in its second round. On the original's
    clock 2,615 passes, 5,230 parts, 23 passes compared (9 rounds' ends,
    the order given by pass 930 and carried out by 931), the same at
    every one. On the own clock the same to round 1 (pass 601); the
    move's gesture is then read by other numbers of passes and the
    port's round 2 never ends.
  - Why it never ends (2026-10-08, macOS): the script, not the port.
    `m48_aim` (`m48_move` with `2m70/0.3 2R73/0.12`: the lengths the
    port's clock sampled, given to the original) stalls the original the
    same way: the aim lands on a unit of the own side, the button gives
    "NO TRANSPORTER", the view stays in the choosing of an aim and the
    script's later keys never end that side's half. On the original's
    clock `m48_aim` is the same at every pass compared (1,566 passes in
    120 s, 3,132 parts read the same, 11 passes compared). And
    `sh tests/play_test.sh m48_aim at m48_move 72 76 84.9 99.9 119.9`
    (new: the port plays another script on its own clock and is compared
    with the original's run of the tag at the first pass each begins at
    or after a second): the game's state and the computer's the same at
    all five, the passes' 3 to 6 bytes apart. Not checked: a time after
    120 s.
  - `m48_build` (285 s, seven rounds; `make_two.py` with
    2R45 2R46 2R47 2R48 2u50 2u51 2u52 2u53 2u54 2d55 2L56 2L57 2m59
    2R62 2R63 2R64 2R65 2L66 2u67 2u68 2d69 2a71 2a73 1L75 1L76 1L77
    1L78 1L79 1L80 1d81 1m83 1L86 1a88 1a90 1E93 2E95 2f99 2f99.8
    1E121 2E123 2f127 2f127.8 1m133 1L136 1L137 1a139 1a141 1E144
    2E146 2f150 2f150.8 1m156 1L159 1d160 1d161 1a163 2L156 2L157
    2L158 2d159 2d160 2d161 2L162 2R164 2m166 2a169 2a171 1E175 2E177
    2f181 2f181.8 1E204 2E206 2f210 2f210.8 2m216 2L219 2a221 2a223
    1E225 2E227 2l229 2R231 2E232 2f236 2f236.8 2L242 2L243 2s245
    2r248 2R251 2s253 2s256 2l259 2R261 2E262 1E264 2f268 2f268.8
    ): fights by both sides (the second side's artillery from five
    fields away in round 1, the first side's unit against the unit
    beside it in round 4, which was ordered to move in the same half),
    moves by both sides, a damaged unit moved onto the second side's
    factory (round 6), and in round 7 the site entered, the unit
    repaired and a unit built. On the original's clock 3,597 passes,
    7,194 parts read the same, 19 passes compared (the seven rounds'
    ends and the passes after them): "the inputs, the game's state and
    the computer's the same at every pass compared". On the own clock
    no round ends (the cursor's steps are sampled otherwise from the
    first seconds on).
  - `m48_undo` (2026-10-08, Windows; 240 s, seven rounds; `make_two.py`
    with `m48_build`'s keys to its round 4's end, 2f181.8, then
    `195:240`, and `m48_undo.pokes`: the second side's unit 14h left
    with one figure when the first pass begins): in round 4 the first
    side's attack destroys unit 14h, which the second side has ordered
    to a free field in the same half, so `attacks_run` frees it with
    `order_undo` (kind 3). A breakpoint at `S10E1:1514` in the original
    was hit once, at 196.7 s with AX 1A14h (unit 14h), and the port calls
    it for the same unit (marks 05h, byte 13h 3, seen with a print not
    kept). On the original's clock (after the merge of be11493) 2,860
    passes, 19 compared: the game's state and the computer's the same at
    every one, 2254 and 2255 around the call included. Three of 5,720
    parts differ in the key the part took (passes 1801, 1874, 1892, while
    both sides' keys overlap; the game's state the same); the original's
    `m48_build` run on the same Windows machine has one such part too,
    so it is not the poke's doing (not looked into). Not reached:
    `order_undo` for the kinds 2, 4 and 5 and for the units aboard a
    freed carrier. Order 1 cannot come: nothing writes kind 1
    (`src/HL.hints` at `move_arrive`; docs/HANDOFF.md).
- `m0_s4` (2026-10-08, Windows; map 0, the second side the computer's,
  260 s, the existing key script; `play_dumps.py m0_s4.keys 260`): a
  depot taken twice. The computer takes `sites_100` record 1 at the
  round's end in pass 1787 (word 19h 12h to 11h) and the first side
  record 0 in pass 2875 (12h to 10h). On the original's clock 3,796
  passes, 18 compared (1, 50, 200, 400, the seven rounds' ends and the
  passes after them, the last): the game's state and the computer's
  the same at every one, both takings included; again the same after
  the merge of 382a4bb. Two things make `play_cmp.py` say DIFFERENT
  and were not resolved: pass 1's setup differs in 43 bytes of
  `S37FF:0012` to `012D` and `S3CFE:0006` (the setup's known bytes,
  docs/HANDOFF.md, "The setup's differences"; map 0 is chosen by the
  mouse here), and one of 7,594 parts: in pass 1059 the first side's
  button word is 10h in both, but the original's `view_step` finds the
  record's byte 8 0 where the port's has FFh (Space let go at 92.450 s,
  inside the first side's part of pass 1058; the port's `input.c` sets
  byte 8 to FFh while the button is not down, so the release reaches
  the two at different ticks; not followed). The music's fade
  (`music_fade`) and `view_cover` of the taking are left out (sound,
  drawing). Not reached: a depot of the other side taken (every depot
  of the 24 maps for two players is closed in by its owner's units:
  map 48's (7,8) behind a river and the second side's units, map 54's
  (21,17) by three of the first side's, map 56's by the second side's;
  scripted probes on 48 and 54 stopped there), so the other side's
  count (byte 4 of `sides`) going down and a depot's stock changing
  hands were not run.
  - `m54_end` (map 54, 320 s, 14 rounds; `make_two.py` with
    1E45 2E47 2f51 2f51.8 2R56 2R57 2R58 2R59 2R60 2R61 2L62 2u63
    2u64 2u65 2u66 2m68 2d71 2a73 2a75 2E77 1E79 2f83 2f83.8 1E88
    2E90 2f94 2f94.8 2m99 2u102 2a104 2a106 2E108 1E110 2f114 2f114.8
    1E119 2E121 2f125 2f125.8 2m130 2R133 2a135 2a137 2E139 1E141
    2f145 2f145.8 1E150 2E152 2f156 2f156.8 2m161 2R164 2d164.9 2a166
    2a168 2E170 1E172 2f176 2f176.8 1R181 1u182 1u183 1m185 1a188
    1a190 1E192 2E194 2f198 2f198.8 2m203 2u206 2a208 2a210 2E212
    1E214 2f218 2f218.8 1E223 2E225 2f229 2f229.8 2m234 2L237 2a239
    2a241 2E243 1E245 2f249 2f249.8 1E254 2E256 2f260 2f260.8 2m265
    2d268 2L268.9 2u269.8 2a270 2a272 2E274 1E276 2f280 2f280.8 1a300
    1a305
    ): the second side walks a unit round the first side's
    headquarters in its seven move halves, the first side moves the
    unit that stands on the way, and in round 14 the headquarters is
    taken: the battle's end. Two presses after it (300 s and 305 s) let
    each side's part of the last pass go on, and the original leaves
    its loop. On the original's clock 3,853 passes, 7,706 parts read
    the same, 33 passes compared (the 14 rounds' ends and the passes
    after them, the last pass, and the memory at the loop's exit
    `S066A:45E8` against the port's after its last pass, when it says
    "the battle is over"): the same at every one. On the own clock
    (`rounds`) the same to round 3.
  The battle's end (2026-10-08, macOS, later): `state_end` shows the
  texts the original does (3Ch to the side that took the headquarters
  and 3Dh to the other, 41h and 42h when a side has no unit left, else
  42h to both), flips and copies the page and waits for a press in
  each side's part (`wait_press`, `S0CB6:0DA9`: the first side's button
  or a key). `play_dumps.py` logs the texts of each end (`ends.txt`)
  and the wait's clearings of the button and the key (`E` lines of the
  clock, `L` where the loop is left), the port's `-ends FILE` writes
  its texts, `play_cmp.py` compares them. `m54_end` again, on the
  original's clock: the end shown twice in pass 3853 with 3Dh and 3Ch
  by both, 3,853 passes, 7,706 parts read the same, 33 passes compared
  with the loop's exit, the same at every one, no place where the
  clock's file did not fit. `m48_build` and `m8_site` (clocks without
  the new lines) and `loop_test.sh` say the same as before. Not
  checked: the wait on the port's own clock (it stops the play when
  the key script has no key left), the texts 41h and 42h, the pixels.
  What follows the loop (2026-10-08, macOS): `battle_after` and
  `battle_score` in `src/battle.c` (`S066A:45E8` to `4728`,
  `S063F:0009`): the map's number kept, the score, the next map, the
  last map's mark and result 7Bh, the map's four best scores loaded and
  the random number that chooses a picture when the score is among
  them; the pictures themselves, the sound and the colours are left
  out (`battle_hook` is told each picture). The program's play calls it
  when the battle is over and says so; the menu that follows in the
  original is not translated. `play_dumps.py` logs the original's
  pictures behind the loop into `ends.txt` and dumps its memory at
  `S066A:4728` (the pass after the one after the last; a run has to
  last long enough: `m54_end.keys 350`), the port's `-dump` of that
  number is its memory after `battle_after`. `m54_end` on the
  original's clock: pictures D0h with 1 and CFh with 2 by both, the
  game's state and the computer's the same at all 34 passes compared,
  that one included (elsewhere 123,149 bytes there: the pictures).
  Not checked: any other way through it (a side left without units, a
  computer's side, the last map, a score below the map's best).
  The other ends (2026-10-08, macOS; docs/HANDOFF.md, "The other
  ends, on map 57"). Two scripts on map 57 (16 by 16, four units a
  side), compared on the original's clock from the port's own setup:
  - `m57_site` (215 s; `make_two.py` with
    1E45 2E46.5 2f50 2f50.8 2L54 2u55 2m57 2L59 2u60 2u61 2a63 2a65
    2E68 1E70 2f74 2f74.8 1E79 2E80.5 2f84 2f84.8 2m90 2u92 2u93 2a95
    2a97 2E99 1E101 2f105 2f105.8 1E110 2E111.5 2f115 2f115.8 2m121
    2L123 2L124 2a126 2a128 2E130 1E132 2f136 2f136.8 1E141 2E142.5
    2f146 2f146.8 2m152 2L154 2a156 2a158 2E160 1E162 2f166 2f166.8
    2R182 1E185 2E186.5 2f190 2f190.8 1E200 2E201.5 2f205 2f205.8
    ): the second side's cavalry walks to the first side's factory and
    takes it in pass 2102, two more rounds follow. 2,660 passes, 5,320
    parts read the same, 25 passes compared (ten rounds' ends and the
    passes after them): the same at every one.
  - `m57_last` (270 s; the same to 2f115.8, then 2m121 2u123 2a125
    2a127 2E129 1E131 2f135 2f135.8 2m140 2a143 2a145 1E148 2E150 2f154
    2f154.8 1a195 1a200; with `tests/keys/m57_last.pokes`, which
    leaves the second side one unit of one figure when the first pass
    begins: `tests/play_pokes.py` gives the runner and the port's new
    `-poke ADDR HEX` the same bytes): that unit attacks a bunker and is
    destroyed, the second side has no unit left, the battle ends.
    1,935 passes, 3,870 parts read the same; the end shown in both
    parts of pass 1935 with texts 41h and 42h by both programs; behind
    the loop pictures CDh with 1 and CEh with 1 by both; 20 passes
    compared (seven rounds' ends and the passes after them, the loop's
    exit, `S066A:4728`): the same at every one.
  The clock's file has `Y` lines now, `inputs_release`'s clearing of
  the key and both buttons when a picture of an event begins
  (`m57_site` read a kept F1 in the pass after the factory's picture
  without it); `m54_end` logged again with them is as before (34 passes
  compared). `play_test.sh TAG rounds` (the port's own clock): both
  scripts the same to round 3, then the steps are sampled otherwise.
  After the merge of origin/master (the fight's drawing) `m48_build`,
  `m8_site` and `loop_test.sh` say the same as before.
  Not reached: a unit destroyed without a poke or as a target, a
  battle ended by fights alone, a depot taken, a site taken with units
  inside, moving out, a carrier, the function keys, the other maps for
  two players, the menu behind the loop.
  Not checked: a battle played to its end; maps other than 0 and 8; a
  computer first side; a thinking loop ended
  by a key; a run with the fight pictures switched off; `move_test`'s,
  `view_test`'s, `side_test`'s, `orders_test`'s and `income_test`'s dumps
  that are only on the Linux worker's machine (not rerun here after the
  change to `move_step`); Windows and Linux builds of the new files
  (`build.bat` names them, not built).
- The fight (2026-10-04, macOS): `sh port/tests/fight_test.sh` builds
  `tests/fight_test.c` and runs it on 39 fights of the original, each a
  pair of dosrun `-ram` dumps made at `-break fight_losses` and at the
  routine's end (`S2487:0AB1`) in six long scripted runs (the scratch
  script `build/n2/fights.sh`; dumps and scripts are local, not in the
  repository). From the first dump `fight_percent` must give the two per
  cent bytes the record already holds (and leave the rest of `fight_rec`
  as it is), then `fight_losses` the unit records, `fight_rec` and
  `rand_state` of the second dump: all 39 the same. Whole memory after the
  port's routine against the original's second dump (`fight_test -out`,
  also `doskit/tools/memcmp.py`) differs only in the stack below the
  original's SP and, in 17 of the fights, in five bytes outside the fight
  (`S30EF:021E`, `DATA:0D4F` to `0D52`; one byte each at `DATA:0308` and
  `DATA:0B19` in one fight): presumably what a timer interrupt changed
  while the original ran, not checked.
  Not covered by these fights: a kind with 4 in word 0Fh (its count in
  byte 3), a loss of 12 or more before the bounds, a fight at the map's
  edge, both fields beside a fight held by the target's side, and a
  divisor of 0 (the original would stop with a division error; the port
  takes the quotient as 0). The test hands `fight_percent` the map and side
  index as `attacks_run` picks them (`attack_pick`); in all 39 fights that
  is index 1 and `map2_ptr`, the second side attacking, so the other case
  is not covered either (and in these dumps both maps and both words of
  `unit_pos` are the same, so a wrong pick would not show).
- Taking units off (2026-10-04, macOS): `sh port/tests/units_test.sh` runs
  `unit_destroy` and `unit_free` on calls of the original, each a pair of
  dosrun `-ram` dumps at the routine's entry and at its end, the arguments
  read from the stack of the first (the scratch script
  `build/n2/destroy2.py` makes the dumps from the same six runs and
  writes the test's argument list; local, not in the repository). The six
  runs have 14 calls of each; the first four of each run were dumped, 12
  of `unit_destroy` and 12 of `unit_free`: after the port's routine
  nothing differs from the original in the game's state (segment `S331F`
  up to `DATA`, and both maps). Elsewhere only the stack below the entry's
  SP differs, and in one call the same five bytes as in the fights
  (`S30EF:021E`, `DATA:0D4F` to `0D52`).
  Not covered: in all 12 calls `unit_free` changed four bytes, so no unit
  of two parts and no carrier was freed: `site_free`, the two-part paths
  of both routines and `unit_free`'s two early returns are translated from
  reading only.
- Making units and sites (2026-10-04, macOS): the same test takes `add`,
  `site400`, `site100` and `site40` for calls of `unit_add` and the three
  `site_add` routines (dumps at the entry and at the RETF, by the scratch
  script `build/n2/add1.py` from the first 60 s of the six runs, which load
  six different maps: up to ten calls of each routine a run, spread over
  the run's calls), and `map` and `shp` for `map_setup` and `shp_setup`:
  the first dump is taken at `S0E56:0411` or `S0E56:013A`, where the
  original has loaded the file, the second at the routine's RETF, and the
  file's place in memory is read from the original's stack frame
  (`build/n2/map1.py`). 60 calls of `unit_add`, 21 of `site_add_400`, 10
  of `site_add_100`, 12 of `site_add_40`, and both setups on each of the
  six maps: nothing differs in the game's state (as above; it includes the
  units, the four tables of sites, the sides' counts, `unit_dat` and both
  maps). Elsewhere up to 19 bytes differ in the two setups, at `0040:006C`
  (the BIOS clock), `S30EF:022E` and in `DATA` (`0308`, `03C6` and on,
  `0B19`, `0D5F` to `0D62`; segment and offset worked out from the dumps'
  linear addresses): presumably what the timer and sound interrupts write
  while the original's routine runs (not checked), and in `shp_setup` the
  caller's pointer argument, which the original moves along the file.
  Not covered: none of the six maps has a two-part unit or a kind with 2
  in word 0Dh on a field, so those paths of `map_setup` are from reading
  only; `map_reset`, `shp_reset` and `carrier_find`'s return of FFFFh were
  not run against the original at all (the dumps begin behind the reset
  parts).
- Loading files (2026-10-04, macOS): `sh port/tests/load_test.sh` runs
  `file_load` on calls of the original (dumps at its entry and at its
  RETFs, the returned pointer from the registers logged there; the scratch
  script `build/n2/load1.py` takes all 47 calls of one run from the start
  to the first map). The port's routine reads the same file from `game/`
  (`$DOSKIT_GAME`): in all 47 calls the returned pointer and all of memory
  are the same as the original's, the work buffer and the routine's own
  variables in its code segment included, apart from the stack below the
  entry's SP and the bytes the test's `beside` lists (the BIOS clock,
  `S30EF:022E`, and in `DATA` `0308`, `03C9`, `03D4` to `03E7`, `0400` to
  `0413`, `0424` to `0426`, `0B19`, `0D5F` to `0D62`), which `file_load`
  does not write (read in its code); presumably the interrupt handlers do
  (not checked). 44 files were packed, one was not, one that does not
  exist was asked for twice (the routine returns 0).
  Not covered: a call without a place (the original allocates; the port
  returns 0 there), a packed file without a work buffer, a file longer
  than a segment, `DATA:0378` above 0 (the name behind the name), and the
  failure to read.
- Libraries of pictures (2026-10-08, Linux): `lib_load` (`S0C66:0009`)
  with `lib_order` in `src/files.c`, and `libs_start` (`S02E6:3484`,
  `LIB\pal.LIB` sorted by `LIB\pal.DAT`, then `LIB\rnd.LIB`) called by
  `battle_begin`, which moves `game_mem.p` on by their lengths.
  `load_test.sh` takes `lib` for a call of `lib_load`. Dumps at its entry
  and at its last RETF (`S0C66:0246`) of all 13 calls of a start on map 0
  and on map 8 (`port/tests/dumps.py ... lib_load N`, keys `m0_hq` and
  `m8_site`, `build/n2/lib/`; argument list `build/n2/lib/lib_test.args`,
  made from its `dumps.log`): the six menu libraries, `partS`/`partW`
  (sorted), `pal` (sorted), `rnd` twice, `exp`, `bum`, `newshop`. All 26
  the same: the return and all memory, but for the stack and `beside`.
  Not covered: a file that cannot be loaded, a name of the list missing
  from the library (the original writes it on the screen; the port skips
  it), and `battle_begin` as a whole after the change (its `begin` dumps
  were not at hand; see docs/HANDOFF.md).
- The screen's set-up (2026-10-08, Linux): `screen_open` (`S298E:000D`,
  Mode X 320x200 for a height of 200), `pages_set`, `screen_clear` and
  `page_flip` in `src/screen.c`, called by the port's start since
  (the entry above). `load_test.sh` takes `scr` (the video
  memory beside the dumps as for `pict`; the routine returns nothing, so
  AX is not compared; the timer's tables at `DATA:0D5F` to `0D9E`,
  `0E3F` and `0E4B`, which change while the original waits for
  retraces, are counted apart). Dumps at its entry and its RETF
  (`S298E:007B`) on map 0 (`dumps.py ... screen_open 1`,
  `build/n2/scr/`, argument list `scr_test.args`): memory and all video
  memory the same; the VGA's registers after it (misc, sequencer, CRTC,
  graphics controller, read from the port by a scratch program
  `build/n2/scr/regs.c`) the same as the runner's `-vgastate` at that
  break; the DAC black in both. Not covered: the other heights' modes,
  `S2B51:002A`, `page_flip` and `pages_set` called on their own.
- A unit's panel (2026-10-08, Linux): `unit_panel` (`S10AF:0131`) with
  `flag_draw` (`S2532:1A8F`, `1BC3`), `num_draw` (`S1571:0134`) and
  `pic_blit_u` (`S2AAF:0000` without its cutting) in `src/draw.c`; the
  loop's hover calls it (`src/loop.c`). `text_draw` became `text_put`
  over host bytes (the number's digits lie on the original's stack) with
  `text_draw` reading the program's memory. `load_test.sh` takes
  `panel`. Nine calls (`build/n2/panel/`, `panel_test.args`; m0_hq 1, 20,
  28, m8_site 1, m8_d5r 30, 285, 294, 296, 300): eight the same in memory
  and video memory; m0_hq call 28 differs in DATA:03CF and 03D2 only,
  bytes of the first input record the original's interrupts changed while
  it ran (presumably, as byte 8 counted beside; not looked at further).
  Not covered: experience above 0 (the 'U' pictures), a unit with bit 0
  of word 4 (the standing flag), a unit with 2 in word 4 (no name), the
  second side's x.
- A field drawn again, and the marks taken off (2026-10-08, Linux):
  `field_draw` (`S0E56:145A`) and the drawing of `marks_erase`
  (`S0E56:1217`, `marks_redraw`) in `src/draw.c`, `marks_erase` itself in
  `src/move.c`; called by `move_step`, `move_done`, `move_arrive`,
  `move_out`, `timer_fire` (kind 5), the hover of the loop (`src/loop.c`),
  `move_aim` and `field_menu` where the original calls them
  (docs/HANDOFF.md). `load_test.sh` takes `fdraw` and `merase` and now
  links `move.c`, `range.c` and `side.c`. Dumps (`build/n2/fdraw/`,
  `fdraw_test.args`): `field_draw` calls 1, 10, 40 of m0_hq and 1, 8 of
  m8_d5r, `marks_erase` calls 1, 7 of m0_hq and 2 of m8_d5r: all eight
  the same in memory and video memory (between 0 and 1,024 bytes of
  video memory written; call 40 wrote none that differ from before).
  Not compared: the routines that call them, as a whole with the
  drawing (the state tests run as before, below); a `field_draw` that
  returns early (state other than 0, 1, 7, 8, or outside the view).
- A fight's panel (2026-10-08, Linux): `fight_panel` (`S131D:178D`)
  with `big_unit`, `panel_whole`, `pic_draw`, `pic_half` and
  `pic_unpack_draw` in `src/draw.c`, called by `attacks_run`
  (`src/orders.c`) with the view, the units' panels and the pages as the
  original draws them; after a fight's pictures and at `attacks_run`'s
  end `pictures_load` (and `libs_start`) run whole (docs/HANDOFF.md).
  `load_test.sh` takes `fpanel` (and links `fight.c`); `dumps.py` takes
  `fight_panel`, `fight_panel0` (the RETF without pictures),
  `attacks_run` and `NOPICS=1`. Dumps of m8_d5r's first fight with and
  without pictures (`build/n2/fp/`, `fp_test.args`): both the same in
  memory and video memory, counted apart the sound's segment `S30EF`,
  `DATA:0309` and the key's record `DATA:045C`-`0461` (the panel's
  sounds and waits, left out). `play_test.sh m8_site`, `orders_test.sh`,
  `loop_test.sh` and `move_test.sh` the same as before. Not covered: a
  kind with 40h in word 4, the first side's half, the blasts drawn, the
  fight's pictures, the DAC after the fades.
- A fight's pictures (2026-10-08, Linux): `fight_pictures`
  (`S2532:0008`) whole in `src/fightpics.c` with its parts
  (`pics_counts`, `pics_kind`, `pics_clear`, `pics_load`,
  `pics_start`, `pics_sounds`/`pics_sound`, `pics_figures`,
  `fight_frames`, and in it `volley_end` and `figures_fall`, which make
  the pictures' calls of `rand_next`) and the blitters they need:
  `sprite_draw` (`S29C1:001F`, with `sprite_cut`), `under_save`
  (`S2AE4:000C`), `unders_restore` (`S2AF3:000A`), `latch_copy`
  (`S297D:000C`), `screen_keep` (`S2B76:0004`), `screen_put`
  (`S294A:000A`) and `pixel_put` (`S292E:000A`). `src/draw.c` lends it
  `rect_fill`, `text_draw`, `pic_draw`, `flag_draw`, `vrb` and `vwb`. The
  sounds (`S2F39`, `pics_play`) are left out; the sounds' files are
  loaded. `load_test.sh` takes `fpics` (and links `plan.c` and
  `computer.c` for `rand_next`; so do the other tests that link
  `fightpics.c`); `dumps.py` takes `fight_pictures` and `fight_frames`.
  Dumps of m8_d5r's two fights with pictures (`build/n2/fpic/`,
  `fpics_test.args`; the first 6 against 6 with one loss, the second 6
  against 5 with one, each run to its 749th frame, no key): both the same
  in memory, `rand_words` included, and in all video memory, counted
  apart the sound's segment, `DATA:0309` and the key's record as for
  `fpanel`, and AX (what the sound's routine left). Not covered: a key
  ending the frames, `S37FF:0130` not 0 (`show`: frames drawn only when
  needed), two units of ground kind 7, a sound card (`S3303:0022`),
  `DATA:0374` set (cutting is on in the frames, `DATA:00E8` 1, so both
  of `sprite_draw`'s paths ran; how often each was not counted), a figure's way of 0 frames
  (`long_div` by 0, reported), the error exit.
  `attacks_run` now calls it as `fight_show` does (`src/orders.c`,
  `fight_show`: the arguments from `fight_rec`, `S37FF:0130`,
  `pics_winter` and the counts at `S3AEC:000C`-`000F`, at its work buffer
  plus 2710h) and `unders_restore` after the panels without pictures
  (`S02E6:0AE6`). `play.c`'s stand-in (the log's number of R lines
  replayed, a loop of frames for the time) is gone; `pics_hook`
  (`pics_clock`) gives the seed (the log's with `-seeds`, else the
  BIOS's), the loading's, each frame's and each fade level's time, and in
  clock mode gives `rand_words` back at the end (the log's S and R lines
  come after `attacks_run`; the port's frames take no time there, so no
  key ends them as the original's did in m8_site). Rerun 2026-10-08:
  `play_test.sh m8_site` and `m8_d5r` (new log and dumps here, 300 s) the
  same at every pass; `m8_d5r rounds` and `seeds` the same up to round 4,
  the last dumped, and with `seeds` (the log's seed numbers only, no
  more of its R lines) `rand_words` the same too: the pictures' calls of
  `rand_next` are the port's own now. `orders_test`, `loop_test`,
  `income_test`, `side_test`, `view_test`, `move_test` and `fpanel` the
  same as before. Not run: `m8_wait` (no log here), `m0_*`.
- The last drawing of a battle (2026-10-08, Linux): `fade_grey`,
  `fade_back` (with `pal_blend`, `S2A62:0008`) in `src/screen.c`, the bar
  (`bar_draw`, `S1571:01F2`), `blast_draw` (`S02E6:225C`) and the 'X' way
  of `pic_draw` (`pic_x`, `S2957:0002`) in `src/draw.c`. `load_test.sh`
  takes `fgrey`, `fback`, `bar`, `blast` and `pic`, and compares the DAC
  where a dump has a `.dac` beside it (`dumps.py` with `DAC=1`, the
  runner's new `-dac`). Ten calls (`build/n2/win/`, `win_test.args`):
  `fade_grey` 1 of m8_d5r and 2 of m0_hq, `fade_back` 2 of m8_d5r and 4
  of m0_hq, the bar 1 and 300 of m8_d5r and 700 of m0_hq (lengths 0, 8Ah
  and 0Eh), the blast 1, 3 and 6 of m57_last without pictures (`dumps.py`
  now starts the maps other than 0 and 8 as `play_dumps.py` does): all the
  same in memory, video memory and the DAC, counted apart the keyboard
  interrupt's bytes during the fades. The 'X' way was reached in no run
  of the 22 key scripts, with pictures or without: translated, not
  compared. Not compared either: the cursors at the pass's end and
  `cursor_field` at their calls (the state tests below are the same, and
  the picture against the original's, next entry), a blast outside the
  view, the bar with no back colour.
- The battle in a window (2026-10-08, Linux, headless build with
  `DK_SHOTS`/`DK_KEYS`): `history-line-headless -map 8` showed the battle
  at picture 700; against the original's screen at 45 s of the same map
  (runner `-shot`, no keys after the menu) 260 of 64,000 pixels differ,
  all within x 208-231, y 108-131, the computer's cursor (the two are
  not the same moment). Four taps of the right arrow (`DK_KEYS` E0-4D)
  moved the human's cursor to the right, its old fields drawn again, and
  scrolled the view at its edge (how many fields it went was not counted
  against the original). The SDL build started on a desktop session
  (`DISPLAY` refused, SDL went on by itself, presumably Wayland) played
  417 passes in about 20 s until it was stopped; the window itself was
  not seen from this shell. Not checked: what a player sees in it, the
  second side's keys, holding a key, the pace measured against a clock.
- The grounds in video memory (2026-10-08, Linux): `grounds_load`
  (`S02E6:323E`) in `src/pictures.c` with `ground_store` (`S28F5:0006`),
  and `lib_pick` (`S0C66:0414`) in `src/pictures.c` too (it was in `src/files.c` at first), called by
  `battle_setup`. `load_test.sh`'s `setup` now compares the whole video
  memory (before, AC58:0 on was counted apart). Dumps at `S066A:0417`
  and `0938` on map 0 (`build/n2/st/st_test.args`) and on map 8 (new,
  `dumps.py ... game_setup 1` with `MAP8` and `VRAM`,
  `build/n2/st/setup8_test.args`): memory and video memory the same on
  both, but for the stack and `beside`. Not covered: a winter map
  (partW), a file missing, the call after `load_game`.
- A text in a status line (2026-10-08, Linux): `show_message`
  (`S10AF:000F`) whole in `src/draw.c`, with `text_draw` (`S2B45:0006`),
  `pic_blit` (`S2A76:0002`, the 'P' way of `S2A6B:0008`) and `pic_clip`
  (`S2A76:0139`); it replaces `message_clear` and the state-only
  `show_message` of `src/side.c`, so the loop's calls now draw.
  `dumps.py` takes `show_message` (RETF at `S10AF:0130`, reached only for
  a text; the call numbers of entry and RETF counted from a run with
  `-log` on both). `load_test.sh`'s `msg` passes the text now. Seven
  calls (`build/n2/msgt/`, `msgt_test.args`): m0_hq calls 3, 4, 30, 900
  and m8_d5r calls 3 (m8_site), 700, 940, texts 61h, 61h, 4Fh, 63h, 61h,
  33h, 33h, all for side 1: the same in memory and video memory (the
  first wrote 155 bytes of video memory). The four `-1` calls of
  `msg_test.args` give the same as before. Not covered: side 0's colour
  73h with a text, the cutting (DATA:00E8 was 0 in all seven), line
  breaks and gap characters, `S2AE4:000C`'s colour table (its two words
  are 0 in these calls).
- The drawn views shown (2026-10-08, Linux): `page_copy` (`S28DC:000A`),
  `pal_level` (`S2AFE:000C`), `fade_in` and `fade_out` (`S28E0:0004`,
  `002B`) in `src/screen.c`; `battle_begin` calls `page_flip`,
  `page_copy` and `fade_in` after the views as at `S066A:0E95`.
  `load_test.sh`'s `begin` now compares the video memory as well: on
  map 0 and map 8 (`build/n2/st/begin_test.args`) the port's at its end
  is the same as the original's at `S066A:0EE6`, memory as before.
  `setup` compares it too, the grounds' area AC58:0 to the end (offsets
  C580h-F4BFh, written by `S02E6:323E`, not translated) counted as
  written by the original only (47,704 bytes on map 0), the rest the
  same. `start` does not (its left-out routines draw on both pages).
  Not compared: the DAC after `fade_in`, each of the three at its own
  call, `fade_out` (not called by the port yet). All other
  `load_test` argument sets give the same results as before.
- The views drawn at a battle's beginning (2026-10-08, Linux):
  `message_clear` (`show_message(-1, side)`, `S10AF:000F` to its RETF at
  `009B`, with `S28E5:0004` and `S28FA:0000`) in `src/draw.c`;
  `battle_begin` calls `view_draw`, `marks_draw` for each side and
  `message_clear` for both, as at `S066A:0DE8`. `load_test.sh` takes `msg`
  (as `view`): its two calls there on map 0 and map 8 (`build/n2/msg/`,
  `msg_test.args`) the same in memory and video memory. `begin` takes the
  palette's buffer from the frame now (before, `pictures_load` failed in
  it) and draws from the entry's video memory: new dumps of
  `dumps.py ... game_begin 1` on map 0 and map 8 (`build/n2/st/`,
  `begin_test.args`) are the same in memory; the port's video memory at
  the end (`LOAD_TEST_VRAM`) is the same as the original's at the RETF
  of the second `show_message` on both maps. The original's at
  `S066A:0EE6` differs from that in 62,890 bytes of the drawn page,
  changed by routines the port does not have yet (docs/HANDOFF.md). The
  headless program (`-game game -map 0` and `-map 8`, each with `-ram`)
  gets through its start; `move`, `side`, `income`, `orders`, `loop`,
  `view` and `play_test.sh m8_site` give the same results as before.
- A side's marks drawn (2026-10-08, Linux): `marks_draw` (`S0E56:0F36`)
  with `S0E56:24ED` and `mark_blit` (`S2A1C:0004`) in `src/draw.c`,
  called by `battle_begin` since (above). `load_test.sh` takes `marks` (as `view`).
  Dumps at its entry and RETF (`S0E56:1012`): calls 1 and 2 on map 0
  and map 8 (the start: no bits in `field_bits`, nothing drawn), and
  calls 20, 35, 49 of `m0_hq` (`-until 120`; call 20, side 1, changes
  1,920 bytes of video memory) (`build/n2/marks/`, argument list
  `marks_test.args`): all seven the same in memory and video memory.
  Not covered: the entries of the table with the view's byte 1Bh not 0
  or 4 were not told apart (which ones the calls met was not looked at).
- A side's view drawn (2026-10-08, Linux): `view_draw` (`S0E56:0D44`)
  with `ground_blit` (`S290A:0006`) and `unit_blit` (`S2931:000E`) in
  `src/draw.c`, called by `battle_begin` since (above). `load_test.sh` takes `view`
  (video memory as for `pict`; AX not compared; byte 8 of the two input
  records, `DATA:03D0` and `03FC`, counted beside, as for `begin`). Dumps
  at its entry and RETF (`S0E56:0F35`) of its first and second call on
  map 0 (`m0_hq`, sides 0 and 1) and on map 8 (`m8_site`), by
  `dumps.py ... view_draw N` with `VRAM` (`build/n2/view/`, argument list
  `view_test.args`): all four the same in memory and all video memory.
  Not looked at: whether a unit with bit 0 of word 4 (colours 30h) was
  in these views. Not covered: a foreign unit with 2 in word 6 (skipped).
  The Windows build (`build.bat`, `draw.c` added) was not made.
- The screen opened by the port's start (2026-10-08, Linux):
  `start_setup` calls `screen_open(320, 200, 2518h, 1)`, `screen_clear`,
  `page_flip`, `screen_clear` as `game_loop` does, `battle_setup` calls
  `pages_set` and `page_flip`. This needs doskit's runtime starting as
  DOS leaves the machine (the VGA in text mode 3's registers, the BIOS's
  data area filled; doskit branch `debian/vga-start`): without it the
  first retrace wait never ended. `load_test.sh` takes `start` and
  `setup` again with new dumps of map 0 (`dumps.py ... game_start 1` and
  `game_setup 1`, entries added to `dumps.py`; `build/n2/st/`, argument
  list `st_test.args`, SS:SP 3EA4:0FEC and SS:BP 3EA4:0FEA): both the
  same, with the page words (`DATA:0322`-`0331`, `0354`-`0357`) counted
  apart for `start` (the original flips 73 times there, 71 of them in
  routines the port leaves out; docs/HANDOFF.md). The video memory was
  not compared for these two: the left-out routines draw (the menu's
  screens at the start; in the setup presumably `S02E6:323E`, the first
  difference at C580h, the page at AC58h; not checked; 48,853 and
  47,704 bytes differ). `scr`, `pict` and the
  default arguments are the same as before; the headless program
  (`history-line-headless -game game -map 0 -ram FILE`) gets through its
  start.
- The battle screen's pictures (2026-10-08, Linux): `pictures_load`
  (`S02E6:2B87`) in `src/pictures.c` with `lbm_show` (`S2B06:0000`, the
  IFF picture drawn into the runtime's VGA memory, `runtime/vga.h`),
  `lib_read`, `tpwm_unpack` and `sfx_load` (memory only); `battle_begin`
  calls it after `libs_start` and keeps the free end at `S3303:0028`.
  `load_test.sh` takes `pict`: beside each `-ram` dump it loads the
  `-vram` dump of the same name into the VGA (mode 13h made unchained)
  and compares the video memory too. Dumps at its entry and at its RETF
  (`S02E6:3237`) by `port/tests/dumps.py ... pictures_load N` with `VRAM`
  set (`build/n2/pict/`, argument list `pict_test.args`): call 1 on map 0
  (`m0_hq`) and on map 8 (`m8_site`), call 2 on map 0 (`m0_carry`, after
  a fight's picture, 48,959 bytes of video memory changed). All three the
  same: the free end returned, all memory but the stack and `beside`, and
  all 256 KB of video memory. The other tests that link `battle.c` (income, side,
  orders, loop, view, move, load) and `play_test.sh m8_site` are the
  same as before. Not covered: ILBM pictures and the path for one larger
  than the screen (written from reading only), a file that cannot be
  loaded, the original's FFFF:FFFF returns, `S2CC5:0004`. The game's own
  window does not yet show the picture: the mode, the palette's DAC and
  the fade-in are the next routines (docs/HANDOFF.md).
- Whole loads and names (2026-10-04, macOS): the same test takes `map`,
  `shp`, `name` and `num` for calls of `map_load`, `shp_load`, `file_name`
  and `num_text` (dumps by the scratch script `build/n2/calls1.py`: the
  one `map_load` and `shp_load` of each of the six runs, 30 of run `w0`'s
  45 calls of `file_name` and its five of `num_text`). All 47 the same in
  all of memory, with the same exceptions as above and the routine's own
  arguments on the stack, which the original changes (a pointer moved
  along, a width counted down); `DATA:03C6` to `03C8` joined the bytes
  counted apart. `map_load` and `shp_load` read `MAP\00.FIN` to `05.FIN`
  and `.SHP` from the game's folder, so this compares the whole way from
  the files to a battle's units, sites and maps.
  Not covered: `num_text` was only ever called with 0, two places and
  filling (all five calls), so a real number, the cut to the lowest digits
  and a negative value are from reading only; `file_name` was met with
  nine combinations of its arguments, among them the prefix (`where` 2) on
  an empty prefix only; a map or `.SHP` file that cannot be loaded.
- Memory (2026-10-04, macOS): the same test takes `alloc` for `mem_alloc`
  (the runtime's `rm_arena` set from the size of the program's own block
  in the dump). A start up to the first map has one call, for 445C0h
  bytes: the returned segment and all memory the same, the block's and the
  following free block's headers included. One call is all the evidence;
  a second block, no room, and `file_load`'s use of it are not covered.
- The start and a battle's setup (2026-10-04, macOS): the same test takes
  `start` and `setup` for the two parts of the original's main routine
  (`start_setup`: its entry to `S066A:0349`, one start; `battle_setup`:
  `S066A:0417` to `0938`, the six runs' maps; dumps by the scratch scripts
  `build/n2/start1.py` and `setup1.py`). The original calls routines in
  between that the port leaves out (the screen, the sound, further files,
  the map's pictures), so all of memory cannot be the same; the test
  counts a byte as different when the port's routine wrote it and the
  original has another value, and apart when only the original wrote it
  (some 15,000 bytes in the start, 110,000 in a setup, most of them
  pictures in the block). Different: 0 in all seven. The pointers the
  routines keep are written to the original's stack frame for the
  comparison and are the same there. Counted apart besides: the word
  `file_load` keeps of what lay behind a file, and in `start` the loading
  routines' own variables and the name buffer (the original loads further
  files there afterwards).
  The program's own run (`history-line-headless -game game -map 0 -ram
  FILE`) against the original's dump at `S066A:0938` of the run on map 0:
  the block is at the same address, and from `S331F:0000` to `DATA` 750
  of 40,448 bytes differ, none of them in the units, the sites, the tables
  of kinds and grounds or the sides' counts (docs/HANDOFF.md lists the
  places; presumably what the menu and the routines left out set, not
  looked at one by one). Maps 5, 33 and 71 were run too and give a size
  and a number of units; they were not compared with anything.
  Not covered: `file_size` of a file that is not packed or does not exist
  (both files of the start are packed), the error returns, and a second
  battle in one start.
- A battle's beginning (2026-10-04, macOS): the same test takes `begin`
  for `battle_begin` (the main routine from `S066A:0938` to `S066A:0EE6`;
  the entry dumps are those the `setup` kind ends with, the exit dumps by
  the scratch script `build/n2/begin1.py`), counted as for `setup`. On the
  six maps no byte the port's routine writes differs from the original.
  Counted apart, besides what only the original writes (some 140,000
  bytes, most of them pictures): the loading routines' own variables and
  the work buffer (the original loads more files after the port's one),
  the four bytes `clock_seed` makes of the BIOS's clock, and byte 8 of the
  two records behind `DATA:00A6` and `DATA:00B6`, which the routine clears
  and which is FFh again at the end (presumably an interrupt handler
  writes it; not checked).
  The program's own run on map 0 against the original's dump at
  `S066A:0EE6`: 803 of the 40,448 bytes from `S331F:0000` to `DATA`
  differ, all of them in records and tables of the pictures' libraries,
  three far pointers at `S331F:179A`, the options' segment `S37FF` and one
  byte at `S3CFE:0006` (docs/HANDOFF.md has the places); the state of the
  game, the sides' records, the views and the computer's segment are the
  same.
  `menu_result`: two more runs of the original, to `S066A:0938` on maps
  30 and 60, have the values the port's run sets for those maps (the
  flags, the sides' word 0 and byte 16h, read from both dumps by hand; no
  test keeps this). Maps 48 to 71 have no file of the computer; the
  program sets up 60 and 71, compared with nothing.
  Not covered: all six runs of `begin` have the second side as the
  computer's, so `computer_reset(0)`, a map without the computer's file
  and a level other than FFh in `defence_sum` are from reading only, as is
  a view at a map's far edge beyond what these six starting fields give;
  in the program's own run the seed is made of whatever the runtime has at
  the BIOS clock's place.
- What a unit reaches (2026-10-04, macOS): `sh port/tests/range_test.sh`
  runs `move_range` and `attack_range` on calls of the original, each a
  pair of dosrun `-ram` dumps at the routine's entry and at its end
  (`S0B15:0621`, `S0B15:09F3`), the arguments read from the stack of the
  first and the result compared with AX as the run's log has it at the
  end (the scratch scripts `build/n2/range2.py`, `range3.py` and
  `range4.py` make the dumps from the six long runs, which hold 2,912
  calls of `move_range` and 619 of `attack_range`, and write the test's
  argument list; local, not in the repository). 60 calls of `move_range`
  (the first four of each run and six spread over each run) and 48 of
  `attack_range` (the first four of each run and the first four that
  found something): the result and the game's state (from segment `S3306`
  up to `DATA`, both maps and the work buffer) are the same in all 108.
  Elsewhere the stack below the entry's SP differs, the argument words
  the original changes on its stack (the port does not write them back),
  and the bytes outside the game's state seen in the fights
  (`S30EF:021E`, `DATA:0D4F` to `0D52` and a few more in `S30EF` and
  `DATA`, the BIOS's count of ticks): presumably interrupts, not checked.
  Not covered: every call dumped is the second side's (the computer's; the
  first side's random play did not get there in the calls taken), so the
  first side's marks and bits are translated from reading only; no unit
  with 10h in `unit_flags` or in its word 6 (the other two of
  `move_costs`' three costs), no kind with 40h in word 0Dh, no
  `attack_range` with 2 in its mask. 10 of the 60 moves reach only the
  unit's own field; 12 of the 48 attacks have 40h in the mask, 25 find
  one to ten units.
- The way (2026-10-04, macOS): the same test takes `path` for calls of
  `path_find` (dumps at its entry and at each of its four ends,
  `S0B15:1019`, `110B`, `13B7` and `13DE`, by the scratch script
  `build/n2/range5.py`). The six runs hold 10,237 calls, 5,421 of them
  without a way; ten of each run were dumped, eight spread over the run
  and the first two without a way: 26 with a way and 34 without, the
  result, `path_len`, the way and the records in the work buffer the same
  in all 60 (the test compares 2580h bytes of the buffer). With them the
  test runs 168 calls, 0 different.
  Not covered: which side and which units these calls are for was not
  looked at (presumably the computer's, as with `move_range`); the two
  ends at 316h records or fields were not met in the calls dumped, as far
  as the results show (they return FFFFh like a way not found, so the
  dumps do not tell them apart).
- The walk (2026-10-04, macOS): `sh port/tests/move_test.sh` runs
  `move_target` and `move_step` on calls of the original, each a pair of
  dosrun `-ram` dumps at the routine's entry and at its end (`S10E1:1444`,
  `S10E1:0AEF`; the scratch scripts `build/n2/move1.py`, `move2.py` and
  `move3.py` log the six long runs and make the dumps; local, not in the
  repository). The runs hold 2,012 calls of `move_step`, 974 of them the
  end of a walk, and 18,607 of `move_target` (14,795 with result 0, 3,797
  with 39h, 11 with 3Ah, 4 with 3Bh). Dumped: of each run three steps that
  end a walk and three that do not, eight calls of `move_target` with one
  of each result the run has, and five steps of a carrier in run `w2`
  (`carrier_moved` called): 41 of `move_step` and 48 of `move_target`.
  The result of `move_target` and the game's state (from segment `S3306`
  up to `DATA`, both maps and both sides' work buffers) are the same in
  all 89, and the original writes nothing there that the port's routines
  do not, although it draws and shows a text. Elsewhere the stack below
  the entry's SP differs and, in the steps, bytes outside the game's
  state: `S30EF:022E`, `DATA:0D5F` to `0D62` and a few more seen in the
  other tests, and `S2A95:000C` in every step that ends a walk
  (presumably the text shown; not checked).
  Not covered: every walk dumped ends on a free field (kind 3), so
  `move_arrive`'s kinds 1, 2, 4 and 5 (into a carrier or site, onto a
  site of the other side) and with them `row_move` are translated from
  reading only; of `move_target` one call gave kind 5 and none kind 2 or
  4 (its refusals 39h, 3Ah and 3Bh were met); no two-part unit walked in
  the calls dumped, as far as the bytes changed show (not looked at one
  by one); which side the walks were was not looked at (presumably the
  computer's, as with `move_range`). `timer_set` was run only for kind 4
  and `unit_mark` only setting 200h, both inside `move_step`.
  `order_undo` is not translated; the logs show no call of it nor of
  `row_move` in the six runs.
- The arrivals (2026-10-07, Linux): `move_test` takes `arrive` for calls
  of `move_arrive` (dumps at `S10E1:0AF0` and at its RETF `S10E1:0F70`),
  and `move_test.sh` reads every `build/n2/move_test*.args`. Three calls
  from runs where the first side is played by a key script (map 0, the
  German side; `build/n2/move_test_sides.args`, dumps under `build/n2/o/`,
  local): a unit into a site of its side (kind 5), into a carrier (kind
  2) and onto a site of nobody (kind 4). Result 38h and the game's state
  the same in all three; elsewhere only the stack and, in the kind 4
  call, five bytes outside the state (`DATA:0D5F` to `0D62` and one at
  linear 3187Eh, presumably interrupts as in the other tests; not
  checked). Not covered: kind 1, a two-part unit, a carrier that moves
  in with units (`row_move`), the second side.
- The second side's arrivals (2026-10-07, Linux): four calls of
  `move_arrive` from the map-0 run s4 (`port/tests/move_test_side1.args`,
  side 1, kind 3, made by `port/tests/sites_dumps.sh`): result 38h, the
  game's state the same; elsewhere the five bytes noted above. Kind 1
  still has no known writer and was not compared.
- `file_load` clears `DATA:0B17` (2026-10-07, Linux): each try of the
  original writes 0 there before it opens the file (`S2C45:0016`); the
  port now does so too. `load_test` on three calls of `file_load` from a
  run on map 0 (`build/n2/load_test_n2.args`: `MNUCHR18.LIB`,
  `LIB\partS.DAT`, `MAP\00.SHP`): the same. In this run its low byte (the
  only one watched) was 0 before each of the last 32 of its 46 writes
  (those the runner lists), so the dumps do
  not tell the change apart; which routine sets it otherwise was not looked
  at.
- The orders at a round's end (2026-10-07, Linux): `sh
  port/tests/orders_test.sh` runs `orders_run` (`src/orders.c`,
  `attacks_run` from `S02E6:0E9C` to `19B8`) on dumps at the loop's first
  entry and at `19B8`, the side that moved and its map read from
  `attacks_run`'s frame, the other side from DI. Three passes from the
  key-script runs (`build/n2/orders_test.args`): with a unit of order 5
  (not a carrier) and with one of order 2 the original changes no byte of
  the game's state and neither does the port; with order 4 (unit 06 onto
  `sites_100` 0, map 0) the port's 24 changed bytes are the original's
  (word 19h of the site 0012h to 0010h, byte 18h 28h to 05h, side 0's row
  copied from side 1's and the unit in its first free place, 1, the unit
  4204h at the site's field). 14 bytes only the original writes: three in
  the libraries' records (`S331F:0167` to `016A`) and eleven below
  `S331F` (linear 3391Dh to 33927h), presumably the libraries loaded
  again and the music, which the port leaves out (not checked one by
  one). 3 passes, 0 different.
  Not covered: order 5 of a carrier with goods (the stock and the freeing),
  a site of the other side, `sites_40` and `sites_400`, the second side's
  orders; the orders 1, 0Bh, 0Dh and 11h are not translated (`orders_run`
  counts them and the test fails a pass that has one).
- Building at a round's end (2026-10-07, Linux): `orders_run` carries out
  0Bh (`S02E6:17C3`: the four fields of a depot, from `S0E56:1A97`, get
  the grounds at `S331F:4C92` when they still have 8000h and no unit on
  the side's map, and byte 0Ah goes down by 1) and 0Dh (`S02E6:189E`: the
  next record of `sites_100`, the count at `S331F:4CAE` and the side's
  byte 4 up by 1, stock 0, the grounds at `S331F:4C97` and `amok_dat`
  byte 3 or 4). `orders_test.sh` reads every
  `build/n2/orders_test*.args`; two passes from a key-script run on map 8
  (`orders_test_build.args`, unit 0C; DI 0 there, the side that built):
  the 0Bh pass (byte 0Ah 2 to 1, grounds 15h, 18h, 16h, 17h) and the 0Dh
  pass (`sites_100` record 1 on 0170h, `4CAE` 1 to 2, side 0's byte 4 0
  to 1, byte 0Ah 1 to 0) are the same, nothing written only by the
  original; with the three earlier passes 5, 0 different.
  Not covered: a 0Bh whose fields are no longer free, the second side's
  depots, the field at a map's edge (`S0E56:1A97` does not test it).
- Order 11h at a round's end (2026-10-07, Windows): `orders_run` carries
  it out (`S02E6:1986` calls `S0E56:1B57` with the unit's place on side
  DI's map, `unit_aim` and the map at `[BP-6]`): three fields around the
  place, chosen by the aim's direction and the column's parity from the
  steps at `S331F:0059`, get the grounds at `S331F:00C5` where their
  ground has 8000h, or 51h when the aim's ground is A5h or above. One pass
  from a key-script run on map 51 (code PLATE, player two set to the
  computer; unit 1F, kind 14; `build/n2/orders_test_build11.args`,
  `0E9C#242`/`19B8#2`, DI 0): the original changes three bytes of side
  0's map (01FAh 51h to A7h, 0228h to AAh, 022Ch to ABh) and nothing in
  side 1's (a compare of the whole dump), and the port the same three,
  nothing written only by the original; with the old `orders.c` the pass
  fails (the three bytes only the original's). Built and run with MSVC
  here; the five earlier passes' dumps are not on this machine.
  Not covered: the branch for an aim's ground of A5h or above, a field
  at the map's edge (not tested by the original either), the second
  side's 11h, a ground without 8000h among the three.
- A round's income (2026-10-07, Linux): `income_add` (`src/orders.c`,
  `S131D:1745`) adds `side_income` (`S131D:1685`) to the side's word of
  `side_points`: byte 16h plus the side of each of the ten records of
  `sites_100` and of `sites_400` whose word 19h has the side in its two
  low bits (used or not, the original tests nothing else), and that byte
  of the side's record of `sites_40`. `sh port/tests/income_test.sh`
  compares four calls dumped at entry and RETF (`S131D:178C`) on map 8
  (`build/n2/income_test.args`, key scripts d4.2 and d5, `MAP8=1
  build/n2/tools/dumps.py ... income N`): side 0 0 to 24 and 24 to 48,
  side 1 0 to 57 and 57 to 114, the game's state the same. Seven more
  calls from the map-0 run s4 (`port/tests/keys/m0_s4.keys`): side 0 5 a
  round until it takes record 0 of `sites_100` (word 19h 12h to 10h,
  byte 16h 0Fh), then 20 (15 to 35); side 1 5, then 20 with record 1 of
  `sites_100`; all the same. On map 8 side 0 holds record 1 of
  `sites_400` (0Eh) from the start, and its depot (record 1 of
  `sites_100`) has byte 16h 0, so it adds nothing. The dumps are made
  again by `sh port/tests/sites_dumps.sh` (`port/tests/dumps.py`, the
  key scripts in `port/tests/keys/`; the remade s4i1 dump was the same
  byte for byte); the calls are in `port/tests/income_test.args`.
  `port/tests/sites_scan.py` lists the records of the three tables in a
  dump: in 81 dumps of maps 0 and 8 no unused record (8000h in word 19h)
  has byte 16h or 17h not 0. Not covered: other maps, a depot or site
  whose byte 16h is not 0 newly taken by the second side.
- The computer's input and steps (2026-10-07, Mac; `port/src/computer.c`,
  the end of the computer's player): `computer_input` (`S1E4B:0001`), the
  next word of direction and button bits from the side's script of input
  codes, and `computer_step` (`S1E4B:012E`), which carries on the step at
  hand of the side's list, every kind of it: `step_move`, `step_attack`,
  `step_end_round`, `step_unit_out` (with `move_targets_list`,
  `S0B15:13EC`, `unit_holder` and `rec_find_unit`), `step_site_menu`,
  `step_build`, and `cursor_script` with `cursor_scroll` and
  `cursor_step`. They write the script; `step_move` and
  `move_targets_list` call `move_target`.
  `port/tests/computer_dumps.py` plays four battles on the runner (the
  computer's second side on the maps of the codes 0 and 1, its first
  side on those of 24 and 25; the codes are read from the game's
  `CODES.DAT`), logs the calls of both routines and the entry of each
  state of each handler for 1500 s, and dumps memory behind the entry
  code and before the return of chosen calls (three of each state),
  into ignored `build/computer/`; about an hour.
  `port/tests/computer_test.sh` runs the port's routine on each entry
  dump and compares the result and the whole dump but the stack below
  the routine's frame. A byte the port wrote, or one of the game's state
  (segment `S3306` up to `DATA`), must be the same; a byte elsewhere
  that only the original wrote is listed apart.
  279 calls, 0 different, 143 of them of side 0 (the script run once
  from its beginning): 51 of `computer_input` (single codes, the
  sequences 11h, 14h and 18h, a script's end, no script) and 228 of
  `computer_step`: 48 of `step_move` (six of each of its eight states),
  24 of `step_attack` (states 0 to 3 and 5), 30 of `step_end_round`
  (all five states), 72 of `step_unit_out` (six of each state 0 to 0Bh;
  in the six of state 7 `move_targets_list` wrote the fields), 48 of
  `step_build` (six of each of its eight states) and 6 ends of a list.
  Ten calls had the same five bytes only the original wrote, one below
  `S3306` and four in `DATA`, among them all six of `step_unit_out`'s
  state 7, the longest call: a timer's interrupt, presumably (not
  looked at).
  Not covered: `step_site_menu` and a step of kind 5 (no such step in
  1500 s on 16 maps), `step_attack`'s state 4 (the same), a unit that is
  in no site at `step_unit_out`'s state 0, `move_targets_list` with no
  field, a field off the map; of `computer_input` the codes 81h and 84h
  and the diagonals (81h is what `step_site_menu` would write).
  The planning phases are in the next three entries.
- The computer's assessment (2026-10-07, Mac; `port/src/plan.c`):
  `computer_assess` (`S1CC6:0A35`), the phase 20h of `computer_turn`,
  with `unit_worth` (`S1FD8:00E8`), `unit_of_kind` (`S1FD8:02B2`),
  `unit_reaches` (`S1FD8:0368`, which calls `path_find`), `sites_list`
  (`S202F:0BA7`) and `plan_to_site` (`S202F:0EAA`). What its five states
  do is in `src/HL.hints`. `port/tests/computer_dumps.py phases` plays the
  same four battles, logs the routine's entry, its two exits and the
  entries of its states (and five branches inside them) for 1500 s, and
  dumps the first three calls of every set of places a run went through;
  `port/tests/computer_test.sh` compares them as it does the steps (its
  kinds `assess` and `assessend`, the side read from the frame as well).
  75 calls, 0 different: 36 of side 0 and 39 of side 1, six of each of
  the states 1 to 5 for each side and six and nine of state 0 (three of
  them with a carrier among the side's units). 29 of the calls had bytes
  only the original wrote outside the game's state (the same kind as
  with the steps).
  Not covered: in state 0 a unit with the plan 9 (the search with
  `sites_list` and `unit_reaches`: neither ran) and a carrier that holds
  a unit with 4 in its word 6 (`plan_to_site`: not run either); in state
  1 a unit near the headquarters only on the map of code 25 (three
  calls); in state 4 no unit was listed in any call (one byte changed),
  so that loop and whether its first test passed were not seen. All
  calls are from the first 835 s of four maps.
- The computer's queue (2026-10-07, Mac; `port/src/plan.c`):
  `computer_queue` (`S1CC6:0000`), the phase 8 of `computer_turn`, which
  makes the units' plans into steps, with `step_add` (`S1E4B:038F`),
  `plan_units_out` (`S202F:04D9`), `plan_repairs` (`S202F:05CB`),
  `plan_move` (`S202F:071E`), `plan_approach` (`S202F:000A`) and
  `rand_next` and `rand_range` (`S2C9A:0008`, `S0CB6:0713`); it calls
  `build_list`, `unit_reaches`, `move_range` and `move_targets_list`.
  Its nine states and the kinds of plan are in `src/HL.hints`.
  `computer_dumps.py phases` logs its entry, where `computer_turn` gets
  back to (the result is AX there), the entries of its three jump tables
  and of `plan_move`'s, and dumps the first call of every set of places
  a run went through and its sixth and eleventh when they are in the
  first 400 s; `computer_test.sh` compares them (kinds `queue` and
  `queueend`).
  171 calls, 0 different, 83 of them of side 0: 5 of state 0, 5 of
  state 1, 12 of state 2, 24 of state 3 (the plans 3, 4 and 0Eh, and
  units with another plan), 12 of state 4, 97 of state 5 (`plan_move`
  in each of its eight states, with and without a step at its end, its
  state 3, the third try for a way, is rare: 20 calls in all the logs;
  the plan 2;
  the plan 5), 7 of state 6, 5 of state 7 and 4 of state 8. Five of
  the calls of state 6 changed one byte (nothing to build); the two
  that built (25 and 74 bytes, the kinds drawn by `rand_range`) were
  picked by hand from the logs (the call of state 6 before the first
  step of kind 7, at 617 s and 435 s on the maps of the codes 1 and
  25) and dumped the same way; the script now logs two places inside
  the building (`S1CC6:08AA`, `092A`) so that it finds such calls
  itself, which was not run again. 66 calls had bytes only the
  original wrote outside the game's state.
  Not covered (no such call in 1500 s of the four battles): the plans 6
  (a repair, the step of kind 6), 7 (`plan_approach`: translated, never
  run against the original), 9, 0Ah, 0Bh and 0Dh, the plan 0Ch in state
  3, a list of steps that is full.
- The computer's plan (2026-10-07, Mac; `port/src/plan.c`):
  `computer_plan` (`S2138:000C`), the phase 40h of `computer_turn`, which
  gives the side's units their plans, with `plan_ride` (`S2407:0007`),
  `unit_outmatched` (`S1FD8:0002`), `field_pinched` (`S202F:0C79`) and
  `unit_chosen` (`S202F:0E3C`); it calls `unit_reaches`, `sites_list`,
  `move_range`, `move_targets_list` and `attack_range`. Its thirteen
  states and their steps are in `src/HL.hints`. `computer_dumps.py
  phases` logs its entry, where `computer_turn` gets back to and the
  entries of eight of its jump tables (the state and the step inside
  it), and dumps the first call of every state and step a run went
  through and its sixth when that is in the first 150 s;
  `computer_test.sh` compares them (kinds `plan` and `planend`).
  283 calls, 0 different, 148 of them of side 0: 4 of state 0, 8 of
  state 1, 37 of state 3, 28 of state 5, 20 of state 7, 4 of state 8,
  30 of state 9, 84 of state 0Ah (every one of its steps 0 to 0Ah), 64
  of state 0Bh and 4 of state 0Ch. 103 calls had bytes only the
  original wrote outside the game's state.
  Not covered (no such call in 1500 s of the four battles): the states
  2, 4 (an aim of kind 2) and 6 (an aim of kind 4); `plan_ride` was not
  logged by its own states: by byte 8 of `computer_phase` at the entry,
  ten of the calls went into it, six in its state 0 and four in its
  state 5; its states 1 to 4 and 6 were in no dump. Most calls are from
  the first
  150 s of a battle, where few units have plans yet.
  `computer_turn` (`S1C74:000B`) itself is translated too, a few lines
  that call the five routines by the phase word; the port's program
  does not call it yet: `game_loop`'s loop is not translated.
- `computer_turn` compared, and the paths not run before (2026-10-07,
  Mac): `computer_dumps.py turn` plays four battles in which the human
  only ends its halves (the codes 0, 1, 24 and 25; 500 s) and logs
  every call of `computer_turn` with what it called (10178, 10766,
  12681 and 11009 calls in 23, 20, 8 and 7 halves of the computer).
  `computer_test.sh` compares 275 single calls (kind `turn:1`, three of
  each combination of what was called and the kind of step, strictly:
  every byte but the stack below the frame) and eight whole halves cut
  into 1143 stretches (kind `turns:N`, 2761 calls: the halves 0, 1 and
  12 of the code 0, 0, 1 and 15 of the code 24, 1 and 3 of the code
  25). A stretch is what the port can run without `game_loop`: the
  thinking between two steps as one, a pass that queues or steps with
  the passes that only play its script, the passes waiting at the
  round's end. Each starts from the original's memory at its first call
  and ends at the next stretch's first call, so the stretches join
  without a gap; what the port wrote has to be the same, and bytes of
  the game's state that only the original wrote are counted as
  `game_loop`'s (it draws, moves and fights between the passes). None
  differs. A half is not run in one go from its first dump: what
  `game_loop` does between the passes (the moves and fights the input
  asks for) is not in the port yet.
  `computer_dumps.py late` takes 86 more calls from quiet battles on
  other maps (the codes 8, 11, 22, 26 and 45, found by logging 48 maps
  for 1200 s each): the repair step (kind 6, 8 calls, its states 0 to
  5), `computer_queue` with the plans 6, 9 to 0Bh, 0Ch and 0Dh (51
  calls), `computer_assess` where a damaged unit gets the plan 9,
  `computer_plan` in its state 4 (7 calls) and in `plan_ride`'s states
  1 to 4 and 6 (18 calls), and `step_attack`'s state 4 (1 call). None
  differs; `sh port/tests/computer_test.sh` now runs 2312 calls.
  Not covered, because no run reached them (48 maps quiet for 1200 s,
  ten maps with random play for 2000 s, the code 22 with random play
  for 1500 s): `computer_plan`'s states 2 and 6, `plan_approach`,
  `plan_to_site`, the plan 7 at `computer_queue`, the step of kind 5
  and state 6 of the repair step.
  `port/tests/computer_test.sh` now links `side.c` and what that needs:
  `move.c` calls `show_message` and `view_copy` since the last merge, so
  the script did not link on `master` (nor does
  `port/tests/move_test.sh` there; left as it is).
- A side's part in a site, first steps (2026-10-07, Linux):
  `src/side.c` has `build_list` (`S131D:14D4`, which makes `build_kinds`
  and `build_count` anew from the side's points) and the repairing that
  `game_loop` does inline (`S066A:3156` to `32A9`, `repair`, returning
  the text; the text shown, `S331F:4CA8` and the view's steps after it
  are not translated), and `src/units.c` `unit_find` (`S15C5:01A7`).
  `sh port/tests/side_test.sh` compares them on dumps made by
  `sh port/tests/side_dumps.sh` (`port/tests/side_test.args`): three
  calls of `build_list` (map 0, side 0, 5, 15 and 35 points: 0, 0 and 1
  kinds) and one repair (side 0 with 5 points: text 4Ah, nothing
  changed); the game's state the same. The run hq
  (`port/tests/keys/m0_hq.keys`, made by `make_hq.py`) moves a unit into
  the headquarters and enters it in an attack half; its later rounds do
  not go as planned (the halves' times depend on the computer's turn), so
  only its first entry and repair and one later entry are used. Not
  covered: a repair that is carried out (text 51h) or refused as full
  (50h), a unit with 4000h, the second side, a list with more than one
  kind; building and moving out are not translated.
- Building a unit (2026-10-07, Linux): `build_unit` (`src/side.c`,
  `game_loop` from `S066A:387A` to `3A42`) makes the unit `unit_find`
  gives with `unit_add` on the site's field for the site's side, sets
  4600h in `unit_flags`, empties its two 9-byte records at `S39B6:09AB`
  and `0132`, puts it in the place, raises `unit_total` and the side's
  byte 2 of `sides`, takes the kind's price, calls `build_list` and sets
  100h in `game_state`'s word 4 (word 6 0), the view to step 0Dh. The
  map-8 run site (`port/tests/keys/m8_site.keys`, from `make_m8.py`;
  the arguments are in `side_dumps.sh`) enters record 1 of `sites_400`
  in the first side's attack halves and in the third round builds the
  first kind of the list: `side_test` (`unit`) the same, `unit_total` 65
  to 66, points 48 to 13; with it two more `build_list` calls (48 points:
  3 kinds, 13: none), the same. Not covered: a list scrolled (word 20h
  not 0), the second side, a carrier's kind (1000h in word 0Dh).
- A repair of a full unit (2026-10-07, Linux): the map-8 run site goes on
  to its fifth round, enters the factory again, steps to the unit built
  there (place 2) and repairs it: `side_test` (`repair`) text 50h like
  the original, nothing changed (37 points). The unit on the cursor's
  place must exist for the menu to offer repairing (word 1Eh of the view
  FFh: no 100h in word 1Ch). Still not covered: a repair carried out
  (51h), which needs a damaged unit.
- Moving a unit out of a site (2026-10-07, Linux): `move_out`
  (`src/side.c`, `game_loop` from `S066A:2F52` to `3146`) with
  `view_copy` (`S0CB6:0445`) and the port's `view_to_field` and
  `move_range`; the three drawing calls (`S0E56:0D44`, `0F36`, `145A`)
  are left out. The map-8 run site goes on to its sixth round and moves
  the unit it built out of the factory (call 41); the block also serves
  the computer's moves out, of which calls 1, 2 and 20 (the second side)
  are dumped too. `side_test` (`out`): all four the same in the game's
  state. `side_test` now also counts differences outside it but the
  stack's segment, where the maps lie: in the four moves out about 22
  each, the timer and interrupt bytes seen before (`0046C`, `3187E`,
  `3E4AF` to `3E4B2`) and `DATA:0308` to `0410` and `DATA:0B19` to
  `0B2B`, presumably what the left-out drawing writes (not checked); no
  byte of a map differs. Not covered: a carrier (20h in the record's
  word 19h), a unit with 4 or 10h in word 6.
- A repair carried out (2026-10-07, Linux): the run `m8_d5r.keys`
  (`m8_d5.keys`, which builds the depot, and the steps of `D5R` in
  `port/tests/side_dumps.sh`) moves the builder, unit 0Ch (4000h in word
  6, byte 0Ah 0 after building), into the depot in the next move half and
  repairs it there in the attack half after (72 points): `side_test`
  (`repair`) text 51h like the original, points 72 to 22, byte 0Ah 2,
  nothing different, outside the state neither. A unit with 4000h is
  repaired at full count while its byte 0Ah is not 2, so no fight was
  needed. The move into the depot (`move_arrive` kind 5, the 63rd call
  of the run; `port/tests/move_test_m8.args`) is the same as well.
  Not covered: a repair of a unit with lost count (no unit without
  4000h in a site was damaged in the runs).
- Moving out of a carrier (2026-10-07, Linux): the map-0 run `m0_s2c.keys`
  (`m0_s2.keys`, whose unit moves into a carrier, and the steps of `C2`
  in `port/tests/side_dumps.sh`) ends both halves of the first round and
  in the second move half enters the carrier and moves the unit out
  (the record's word 19h has 20h, so the field is one less; the branch
  at `S066A:2FA7` was hit): `side_test` (`out`) the same in the game's
  state; outside it the same kinds of bytes as for the moves out of a
  site (interrupts, `DATA:0308` to `0410`, `0B19`), no byte of a map.
- A human side's input (2026-10-07, Linux): `side_input` (`src/side.c`,
  `S0CB6:05F1`) makes the input word `game_loop` takes into DI from the
  side's 5-byte input record and the view's repeat counter. `side_test`
  (`input`) compares seven calls of the run `m8_d5r.keys` taken at its
  taps of right (245.00) and of the move gesture (248.00), made by
  `side_dumps.sh` (the call numbers counted with `-log` up to those
  times): results 8, 0, 0, 10h, 10h, 11h, 11h and the record and view
  the same, also outside the game's state. `view_step` and
  `field_menu`, which the word drives, are not translated yet.
- `move_start` (2026-10-07, Linux; `src/move.c`, `S10E1:05D1`), the
  first of `view_step`'s callees: the confirmed move puts the unit into
  the side's list of movers and starts the walk's timer. `sh
  port/tests/view_test.sh` (dumps by `port/tests/view_dumps.sh`) compares
  six calls from the runs k4, s2, s4 (map 0) and d5r (map 8), the first
  side's and the computer's: result 1 and memory the same, also outside
  the game's state. Not covered: the result 12h (the cursor not on the
  field move_aim took) and a unit with 40h in `unit_flags`.
- `attack_unmark` (2026-10-07, Linux; `src/move.c`, `S0AE0:02E3`): after
  `attack_order` the side's attack marks are cleared in `field_bits`
  (`field_bits_clear`, now shared from `src/range.c`) and the view's unit
  loses 100h; the redrawing is left out. Three calls of the run d5r (the
  first side's and two of the computer's): the game's state the same;
  outside it eight bytes, the interrupt bytes and `DATA:0308`, `0B19`,
  as with the other routines that draw.
- `attack_order` (2026-10-07, Linux; `src/move.c`, `S0AE0:0008`) and the
  state part of `show_message` (`src/side.c`, `S10AF:000F`: 4 or 8 in
  `game_state`'s word 0 for a text shown, which `timer_fire` takes away;
  the drawing left out). Three calls of the run d5r (the first side's
  and two of the computer's, all orders given): the game's state the
  same, outside it the interrupt bytes and bytes around linear 2B0B0h
  that `show_message`'s drawing presumably writes (not checked). Before
  `show_message` was added the first call differed in `game_state`'s
  word 0. Not covered: a field not marked (text 45h), the builder's
  orders 0Bh, 0Dh and the depot limit (47h), order 11h.
- `move_done` (2026-10-07, Linux; `src/move.c`, `S10E1:000F`): five
  calls (the first side's in k4 and the computer's in s2c, d5r and site)
  the same in the game's state; all have the flags 1, so only the
  clearing of the side's marks is covered. Not covered: a move given up
  (10h in the flags) and a view in step 3 (a unit taken back into its
  site or carrier). (Corrected with `view_step` below: the original
  writes F4h and the record's byte 1Bh into the map; the port wrote them
  through the units' segment, which the first comparison of a move given
  up showed.)
- `move_aim` (2026-10-07, Linux; `src/move.c`, `S10E1:02EB`) with
  `marks_erase` (`S0E56:1217`, of which only its `field_bits_clear` is
  state): five calls (the first side's in k4 and d5r, the computer's in
  d5r) the same in the game's state, all with a way found (result 1).
  Not covered: the refusals (12h: a field not marked, the unit's own, a
  ground closed to the unit, a text from `move_target`, no way, 48h).
  With this all of `view_step`'s callees are in the port.

- `field_menu` (2026-10-07, Linux; `src/side.c`, `S0CB6:0EAA`) with
  `depot_places` (`S0E56:18A3`, named now: where the builder of depots
  may build; marks and the drawing left out). 23 calls of the runs k4,
  s4, hq (map 0), d5r and site (map 8), found with `run.py -log` on the
  entry and on `S0CB6:106C` (the button let go): the entry of state 1,
  a held button with up, and the releases with the pointers 4 (a move,
  `move_range` finding fields), 0 (an attack by `attack_range`), 9, 0Ah
  and 3, of both views; the game's state the same in all, outside it
  only the bytes the drawing and the interrupts change. Not covered:
  pointer 2, the refusals (texts 53h, 54h, 4Bh, 4Ch, 4Dh, 6Fh), the
  builder (`depot_places`) and a unit with 2000h in word 6, and a held
  down, left or right.
- `view_step` (2026-10-07, Linux; `src/side.c`, `S0CB6:0750`), with
  `move_args` (`S3306:000A`, now named; `move_range` keeps its arguments
  there and step 18h calls it again with them). 15 calls of the runs k4,
  s4 (map 0), d5r and site (map 8), found with `run.py -log` on its
  branches (`S0CB6:07C1`, `080F`, `0837`, `0909`, `0B24`, `0C90`): a
  press in step 0, without a press, the aim (16h) found and refused
  (12h, two calls), the move's start (3) and the step after it (19h),
  the move given up by a second press (17h, `move_done` with 10h and a
  unit out of a site, three calls) and the attack's order (13h, four
  calls): result (AL) and the game's state the same. The move given up
  first differed in one byte: `move_done` wrote F4h through the units'
  segment where the original writes into the map; fixed in `src/move.c`.
  Two refused calls of `move_aim` (d5r, its calls 15 and 16, result 12h)
  were added to the comparison; which of its tests refused them was not
  looked at. Not covered: steps 18h and 1Ah (a move whose start gave 10h,
  an attack not ordered; neither came in these runs, so `move_start`'s
  12h is still not compared) and a second press by 8 in the side's word.
- The pass of `game_loop` (2026-10-07, Linux; `src/loop.c`,
  `S066A:0EE9` to `45D7`, and `attacks_run`'s frame in `src/orders.c`)
  over the first round of the key script `m8_site` on map 8 (the first
  side human, the computer the second), from the original's memory at
  its first pass: `tests/loop_dumps.py` runs the original up to 145 s
  (2,523 passes, before the first fight at 150 s) and logs by
  registers what each side's part reads from outside the translated
  code (the key, the input record's keys as `side_input` finds them and
  its bytes 5 and 8 as `view_step` tests them, the computer's input word
  and text, 80h set in `game_state`'s word 2 by the computer's phase,
  and 8 set in its side's word by `computer_step`); `tests/loop_test.sh`
  plays those passes and compares memory at the beginning of 26 passes
  (50 to 2522, among them the two round's ends in passes 291 and 1939 and
  the passes after them) with dumps of the original: the game's state the
  same at all of them (S3306 to DATA without the computer's segments,
  the two maps, the sides' work buffers). Counted apart: the computer's
  segments and what its planning leaves between the passes (from pass
  292 on, logged: 15 calls of `move_range`, 6 of `attack_range`, 186 of
  `field_neighbours` and some 360 of `move_target` inside the computer's
  turn): `move_args`, `S331F:17F2` to `181B` (the way, `neighbours`,
  `move_rec`) and `S3AEC:0000` to `0007` (written by code in `S2138`);
  and outside the state the pictures, the input records and `name_buf`
  (the original loads `LIB\BIGUNIT.LIB` from 83 s on to draw a unit).
  The first comparisons differed by two things, both in the replay, not
  in the translated code: a key was given to the side before (it is read
  before `S066A:1006`, so a side's part is logged from `0FC3`), and 8 in
  the computer's word of `sides` (from `computer_step`) was missing.
  Reached in the compared passes (counted with an instrumented build of
  the test): states 0 (the computer's steps 3, 13h, 16h, 17h and 19h, the
  first side's 0 and 19h), 1, 2 (steps 0Ah,
  0Bh with pointers 1, 4 and 5: entering and leaving a site, a unit moved
  out by the computer), 3 (the small map, by the computer's side, 124
  passes) and 5 of both sides, and the round's end twice. Not covered:
  the fights (`attacks_run` refuses them), order 1, `S02E6:2B87`, the
  keys but F1, states 4 and 7, repairing and building inside the loop
  (they come later in `m8_site`), the computer's part with 80h in
  `game_state`'s word 2 (not set in these passes), and a human second
  side. The pass is not yet played by the port's program: it runs only
  in the test, from the original's memory.
- The fights in the pass (2026-10-07, Linux; `fights_run`,
  `carrier_losses`, `order_capture` in `src/orders.c`, `order_undo` in
  `src/move.c`): `tests/loop_dumps.py m8_site.keys 400` runs the original
  to 400 s (6,582 passes; `attacks_run` in passes 291, 1939, 2615, 4284,
  4543 and 6215) and `tests/loop_test.sh` compares 16 passes (50 to 6581,
  each round's end and the pass after it): the game's state the same at
  all of them. Seven fights were carried out in passes 2615, 4284 and
  4543 (fight pictures on: `S37FF:0115`, `no_fight_pics`, is 0), and a
  site was taken in 6215 (order 4 with `pictures_load` after it). Counted
  apart now besides what was before: `S3AEE`, the data of the fight's
  pictures (used only by `S2532`, which `fight_show` calls), and
  `S3812`, the data of `S1A39`, the picture of a site taken; both
  differed and nothing else of the program's reads them. Not reached in
  these passes (counted with an instrumented build): a unit destroyed in
  a fight, `carrier_losses`, the freeing of units marked 4, `order_undo`,
  order 1 and the blasts with fight pictures off.
- The pass with the computer's own turn (2026-10-08, Mac; `src/loop.c`
  `computer_part`, `S066A:1020` to `1190`, and the 80h of state 5 at
  `S066A:40EA` to `414E`): the computer's side is no longer replayed
  from the original's log but played by `computer_turn`, so the 400 s of
  `m8_site` run from the memory at the first pass with the human's keys
  alone and two inputs of the clock (below). `tests/loop_test.sh`
  compares 29 passes (50 to 6581: each of the six rounds' ends and the
  pass after it, the three passes in which the computer thinks in a loop
  and the pass after each, and eleven others): the game's state the same
  at all of them, and now with it, compared and no longer counted apart,
  what the computer's player keeps and leaves: its segments `S39B6` to
  `S3AEB` (the phases, steps, scripts, plans and aims), `S3AEC:0000` to
  `0007`, `S331F:17F2` to `181B` (the way, `neighbours`, `move_rec`),
  `move_args` and `rand_words`: 0 bytes different at every pass.
  `computer_turn` ran 7,211 times in those passes (counted with a
  coverage build of the test): `computer_assess` 26 calls,
  `computer_plan` 1,939, `computer_queue` 976, `computer_step` 3,544;
  634 of the calls in the three loops with 80h in `game_state`'s word 2
  (passes 671, 3088 and 5017: 60, 238 and 336 calls).
  The two inputs besides the keys, both logged by `tests/loop_dumps.py`
  (columns 10 to 12 of `inputs.txt`): the number of calls in such a loop
  (a human's button or a key ends it early; the port stops at the
  original's number and refuses a loop that is through at another), and
  what `clock_seed` leaves in `rand_words` when the fight's pictures
  call it with the BIOS's count of ticks (once a fight: twice in pass
  2615, five times in 4543; the last of a pass is given to the port
  after `attacks_run`). `rand_range` was called eight times in the run
  (five in pass 4394, three in 6454).
  The one difference found was that seed: without it the third kind
  `computer_queue` drew to build in pass 4394 was another (a byte of
  `step_draft` and two of `computer_steps` at the passes 4543 to 6216);
  nothing in the translated code was changed for it.
  `tests/loop_test.sh -replay` plays the computer's side from the log
  as before (the game's state the same at the 29 passes; the computer's
  then differs, 688 bytes at the last, since nothing writes it). Not
  covered: a loop ended early by a key (all three ran until the
  thinking was through: the same result with the numbers left out);
  `rand_next` called by other code than `rand_range` (the dumps script
  refuses such a run; `S2532` has calls that did not come); a computer
  first side, two computers; `plan_to_site`, `plan_approach` and
  `step_site_menu`, which these passes do not reach (the first two were
  reached on no map, docs/HANDOFF.md). Not verified: the bar the
  original draws while the computer thinks (`S1C74:0354`, read as
  counting only; not translated).
- The fights' rarer paths (2026-10-08, Linux; key scripts
  `tests/keys/m8_long.keys` and `m0_carry.keys`, made as
  `tests/side_dumps.sh` checks): `m8_long` is `m8_site` up to 375 s, then
  the first side only waits (the half ended every 10 s, F1 5 s later; the
  step `xT:U` of `make_m8.py`) to 900 s; `m0_carry` is `m0_s2` (a unit
  into the first side's carrier) and waiting to 700 s, while the
  computer fights the carrier. `NOPICS=1 tests/loop_dumps.py` makes the
  same run with fight pictures off: it writes 1 to `no_fight_pics`
  (`S37FF:0115`) at the first pass with `-poke`, not the way the game
  sets it (not looked for). `tests/loop_test.sh` (the computer's own
  turn, not `-replay`) on the three runs, the game's state and the
  computer's the same at every pass compared:
  `m8_long` 12,715 passes, 29 compared (13 rounds with `attacks_run`);
  `m8_long_nopics` 16,087 passes, 35 compared; `m0_carry` 8,653 passes,
  all its round's ends and the passes after them. Reached (counted with
  an instrumented build of the test, in the replay before the merge of
  the computer's turn): units destroyed in fights and freed (5 in
  `m8_long`, 12 in `m8_long_nopics`, 8 destroyed and 9 freed in
  `m0_carry`), the blasts counted down without pictures (12),
  `carrier_losses` (7, `m0_carry`). Two differences were found in
  `m0_carry` and fixed: `pictures_load` writes a digit of a file name
  (`S331F:0284`, by the ratio of the sides' unit counts; now
  `pictures_digit` in `src/orders.c`, at `S02E6:19B8`), and `move_step`
  shows the arrival's text (`show_message`, setting 4 in `game_state`'s
  word 0, which keeps `hover` from setting a timer while it is shown);
  the port had left that call out, so a free timer slot kept another due
  pass. Still not reached: `order_undo` (reached later, `m48_undo`),
  order 1 (unreachable), and the units aboard a
  freed carrier (in `m0_carry` no freed carrier had a unit in its row).
  Not checked: which of the digit's values ('1', '2', '3') the runs had
  besides the '2' and '3' of the difference.
- A side's part in the passes (2026-10-08, Linux): building, repairing
  and moving out (`build_unit`, `repair`, `move_out`, `build_list` in
  `src/side.c`, called by `src/loop.c`) were compared on single calls
  before; now also in whole runs of `tests/loop_test.sh`, the computer's
  side played by `computer_turn`. New loop dumps (`tests/loop_dumps.py`):
  `m8_d5r.keys 345` with `NOPICS=1` (with pictures on, the dumps script
  refuses the run: `rand_next` called by other code than `rand_range` in
  pass 2265), `m0_s2c.keys 110` and `m0_hq.keys 260`; each is run as
  `tests/loop_test.sh $(cat ../build/loop/TAG/loop_test.args)`. The game's
  state and the computer's the same at every pass compared in all of
  them and in `m8_site`, `m8_long` and `m0_carry`. Reached (counted with
  a coverage build of the test): `build_unit` 5 calls in `m8_site`, 8 in
  `m8_long`, 2 in `m8_d5r_nopics`; `move_out` 41, 48, 36, 4 (`m0_carry`),
  2 (`m0_hq`) and 1 in `m0_s2c`, that one out of a carrier (the field one
  less); `repair` with text 50h (`m8_site`), 51h (`m8_d5r_nopics`) and 4Ah
  (`m0_hq`). The calls were not split between the human and the computer.
  Not reached: a build list scrolled (word 20h of the view not 0), a
  carrier's kind built, a build or repair by a human second side, a
  repair of a unit with lost count and no 4000h in word 6, a move out of
  a unit with 4 or 10h in word 6, and the computer's own repair step
  (`step_site_menu`, reached on no map, docs/HANDOFF.md).
