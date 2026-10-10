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
(`key_irq`, `input_tick`; the joysticks and the mouse were left out
then: the mouse came with the window, the joysticks on 2026-10-09, see
below). The
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
music and F3 the effects off and on (the bits of `game_state`; F2 calls
`music_fade(4)` or `music_start` since the branch `mac/music`, see the
music's paragraph below; no sound comes out), F4 a text with two numbers, F5 and F6 a side's joystick (no game
port in the port: the word is switched and no stick found), F7 the mouse
off and on, F8 its speed in three steps, F9 the three settings of the
fights' pictures. The play no longer stops at them.

2026-10-09 (`mac/options`), the joysticks (`src/joy.c`, `src/input.c`):
`joy_read` (`S2BAD:0006`: the sticks' buttons, and each axis as the
number of reads of the game port until its bit falls), `joy_setup`
(`S2B9B:000A`: which sticks are there, each one's middle where it
stands, its range and edges; `joy_measure` in the port) and the sticks'
part of the sides' input records (`S27FE:01AC`, `0232`: directions by
the edges and the button, before the keys and the mouse, which are asked
only when the stick gives nothing). The timer's routine reads the sticks
on every fourth call while one is wanted, as the original. The game port
is the port's own, made like the runner's: a stick is the microseconds
its two one-shots stay high after a write (`joy_stick`), and the reads
are counted on a clock of the original's instructions at the runner's 6
million a second, so a stick measures what it does in a run of the
original there. **This is a changeable choice of machine, not the
game's**: a faster or slower PC counts other numbers for the same stick.
Sticks come from a script only (`-joys FILE`, lines `T N,X,Y,B` or
`T N,off` as the runner's `-joys`, in the order of their times; they go
by the port's own clock, in a battle on the original's clock by that
log's calls of the timer's routine); the window's program has no stick (a
controller is keys there), so there F5 and F6 still find none. In the
menu a stick's directions and buttons do what the first side's keys do
(compared 2026-10-09 on `mac/options2`, see "Checked").
Different from the original, because `src/loop.c` was not to be changed
on this branch: F5 and F6 still call `loop.c`'s own `joy_setup` (the one
for a machine without a game port), and the real one is started by the
timer's routine at its next call, when it sees that a stick's wanted
word has changed; it then waits through the original's retraces by the
calls' times, to within a call. `input_tick` no longer returns "a
joystick" as not translated.

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
each screen's video memory into DIR (for the test). The music's calls
(a side's tune started with a fade of 5, the tune before and the last
one faded out in 5) are made since the branch `mac/music`; no sound
comes out. The time is the port's own
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

2026-10-08 (`debian/window7`): a carrier's screen by the mouse, a real
repair and a scrolled build list in the window. The port needed no
change. Three scripts, each played in the headless window (the drafts
from `make_m8.py` with `-moves`; `.keys` and `.mice` are what the window
wrote by `-keylog` and `-micelog`), compared with the original's run of
the same files (`play_dumps.py`, `VRAM=1`, `PASSES` around the gestures):
- `m0_cmouse` (map 0, 120 s): `m0_s2.keys` and `make_m8.py -moves
  m0_cmouse.moves d52 E55 f60 f65 E72 f80 f85 Mu98 Ms99 Mo104`: the
  first side enters the loaded carrier by the mouse (its screen: loading
  4, capacity 4) and moves the unit out. `PASSES="1182 1200 1216 1243
  1289 1307 1352"`.
- `m8_rmouse_nopics` (map 8, 350 s, fights without pictures):
  `m8_d5.keys` and `D5R` of `side_dumps.sh` with `Md333 Ms335 Mr339`:
  the builder (unit 0Ch) enters the depot and is repaired by the mouse;
  the original shows text 51h at 339.73 s. With fight pictures the
  window's play of `m8_d5r`'s times does not reach the repair (screens
  at the frame of 340 s show round 2; not looked into, the frame numbers
  of `DK_SHOTS` are not the clock's time across a round's end).
  `PASSES="5765 5809 5854 5881 5915 5984"`.
- `m8_scroll` (map 8, 240 s): `m8_scroll.pokes` sets the first side's
  points to 2000 at the first pass (not the game's own doing), so the
  factory's build list has more kinds than its seven places; `m8_site`'s
  steps to `s189`, then `d191` to `d198` (eight taps: the cursor to row 6
  and the list scrolled), `Md199 Md200`, `u201`, `s203` (infantry built
  from the scrolled list, 2048 to 2013 points), `l206 d208 E209` and F1s.
  `PASSES="2922 2940 3048 3084 3089 3113 3228 3328"`. Whether the second
  mouse step scrolled once more was not checked (the pointer stayed at
  160,164 in the `.mice`).
For each, after the merge of origin/master a9ef47d: `play_test.sh TAG`
the inputs, the game's state and the computer's the same at every pass
compared, both pages of video memory the same at all of them (behind the
pages 1,442 bytes differ, 11,544 to 12,841 in `m8_scroll` after its
fights, not looked into); `live`: the window's play and the `-keys`/`-mice`
play the same in all parts (3,328, 13,880, 10,068) and at every dump.
Not done: `rounds` for the three, the SDL window by hand, a repair with
fight pictures on, `m8_mouse` and the older scripts not rerun after the
merge.

2026-10-08 (`debian/window5`): a battle's end in the window. The
pictures behind the loop are translated (`src/events.c`, HL.EXE's
`event_picture`, `S1A39:0001`, for its kinds CDh to D0h): D0h a human
side has won (`pic_won`), CDh the other side was left without units
(`pic_wiped`, with its sounds by `sfx_play`), CEh and CFh a new best
score (`pic_medal`: two streams, a third, and a picture of a library on
a background), with what they share: the loading of their files from
the tables in S3812 (`res_load`, `S1A39:034C`), the frames of the
streams drawn into the VGA's planes (`vdif_frame`, `S2982:0002`), a
first frame faded in (`first_frame`, `S1A39:02C5`) and the colours
going down at the end (`pal_blend`, now public in `src/screen.c`, as
is `retrace_wait`). `battle_after` now does the `fade_out` and
`unders_restore` before them and shows each picture (`battle_hook`
calls `event_picture` in `play.c` with the work buffer its caller has:
game_loop's `[BP-10h]`, which is `game_mem.gfx`, not `lists_end` as the
port had it, and battle_score's own moved on by 2B2Ah for CEh and CFh;
`battle_score` loads `CODES.DAT` there too). Left out: the music
(`music_start`, `music_fade`), the other kinds (a site's, the
newspapers of the maps for one player), the flags 2 and 4 of
`res_load` (not in these tables). The time: on the original's clock
`play_dumps.py` logs each frame's beginning (`F`: `S1A39:0BE6`, `0EF6`,
`10AF`, `1359`) and wait_press's entry (`Q`, `S0CB6:0DA9`) and the port
runs the log up to them; on its own clock the frames count the timer's
calls as the original does, and the loading takes what the runner took
(`t_ev_load` in `play.c`: D0h 0.70 s, CDh 0.76, CEh/CFh 0.34, 0.34,
0.46 and 2.07 behind `music_fade(14h)`, and 0.42 s before inputs_release; measured in `m54_end`,
`m57_last` and `m57_win`). `-evdump PREFIX` writes the memory and video
memory at each end's wait (`PREFIXeN`) and at the frames 1, 11, 21 ...
of the pictures (`PREFIXfNNNN`); `play_dumps.py` dumps the original's
there (`eN`, `fNNNN`), `play_cmp.py` compares them in each mode, and
`play_test.sh` gives `-evdump` to the port in all of them (`live`: the
window's against the `-keys` play's). The port's `-dump` behind what
follows the loop now writes video memory for a `.vram` name (it wrote
memory there).
`m57_win` (map 57, 84 s, with `tests/keys/m57_win.pokes`: the second
side's four units get count 0 when the first pass begins, a poke, not
the game's own doing) is what the headless window wrote (`-keylog`)
playing both sides' ends of their first halves and two presses: the
first round's end drops the second side's units, both views go to
state 7 (texts 41h and 42h), each side's part waits for its press
(52.7 to 60.0 s and to 65.1 s), the loop is left, picture CDh with 1,
`battle_score` (no new best score here: no medal), as in the original
with the same file:
- `play_test.sh m57_win`: the inputs, the game's state and the
  computer's the same at every pass compared, and at both end waits and
  the frames 1 to 121 of CDh both pages of video memory the same;
- `rounds` (the port's own clock, the window's): the same at both
  rounds' dumps, both waits and all 13 frames compared, pages included;
- `live`: the window's play and the `-keys` play the same in every part and
  all 36 files (dumps, waits, frames).
On the original's clock also (each rerun after the merge of origin/master
64773b5): `m57_last` (CDh with 1 and CEh with 1: the same at all 26
frames compared, both waits, the passes and `S066A:4728`, pages
included; the pictures' files in memory the same too once the work
buffers were right), `m54_end` (D0h with 1 and CFh with 2: the same at
every pass, both waits and all 20 frames, pages included);
`m48_attack`, `m8_site`, `m8_mouse`, `loop_test.sh` and
`menu_test.sh` (all six walks) still the same. What still differs
"elsewhere" at the waits and frames is the sound driver's segment
(S30EF, the music not translated) and the interrupt vectors.
Not done or not checked: the SDL window by hand; D0h, CEh and CFh on
the port's own clock (no script reaches them there: `m54_end` and
`m57_last` part from the original after round 3, the cursor's keys
sampled otherwise, and `m57_last`'s fight's outcome follows the
clock; their loading times are taken from one run each; why the passes'
phase moves, docs/HANDOFF.md "Why `m54_end` parts on the port's own
clock": shorter round's ends, passes that overran and lost timer
interrupts in the original, not fixed); a battle's end
in the window from the menu (with `-map` the play ends behind the
pictures; from the menu the menu follows, not tried with this script);
the sounds of CDh as sound; a medal with `which` 1 or 2.

2026-10-08 (`debian/window6`): the second side's attack in the window.
Nothing of the port needed changing: the second side's attack goes
through the same `menu_attack`, `attack_order` and `attacks_run` as the
first side's. `m48_sattack.keys` (map 48, 180 s) is what the headless
window wrote (`-keylog`) playing `make_two.py` with `2R45 2R46 2R47 2R48
2u50 2u51 2u52 2u53 2u54 2d55 2L56 2L57 2m59/0 2R62 2R63 2R64 2R65 2L66
2u67 2u68 2d69 2a71 2a73 1E93 2E95 2f99 2f99.8 105:180` (`m48_build`'s
first round, the gesture's up let go with the button): in its first
(attack) half the second side's medium artillery is chosen, the attack's
view darkened, the aim stepped onto the first side's cavalry two fields
up and the attack ordered (71 to 73 s); the first round's end fights it
with its pictures (attacks_run at pass 1235, the pictures' key cleared
at 105.5 s), then five more rounds are ended by both sides. With
`m48_build`'s `2m59` (up let go 0.2 s after the button) the window and
the original read the trailing up by other numbers of passes (the aim
one field apart, the same cause as `m48_move`'s), and no script with
that length is the same in both; let go with the button it is.
The original's run of the same file (`play_dumps.py m48_sattack.keys
180`, `VRAM=1`, `PASSES="536 652 724 754 1234"`: the artillery chosen,
the attack's view, the aim stepped, the order, the pass before the
fight):
- `play_test.sh m48_sattack`: the inputs, the game's state and the
  computer's the same at every pass compared (26 dumps, the attack's
  screens and all six rounds' ends and the passes after them); both
  pages of video memory the same at all of them (5,655 bytes differ
  behind the pages, as in `m48_attack`, not looked into);
- `rounds` (the port's own clock, the window's): the game's state and
  the computer's the same but for the passes' in all six rounds (the
  port's round 1 ends at pass 1239, the original's at 1235), the pages
  the same;
- `live`: the window's play and the `-keys` play the same in all 4,574
  parts and all 26 dumps, memory and video memory.
Not done or not checked: the fight's pictures frame by frame in video
memory against the original (only the views before and after them),
the computer as second side attacking in the window (map 48 has two
human sides; the computer's attacks were compared behind the window,
`computer_test.sh`), the SDL window by hand.

2026-10-08 (`debian/window8`): menu, battle, menu through a battle's
end, and a `-keylog` from the menu replayed. Nothing of the port needed
changing. `menu_dumps.py` has two walks with a battle played to its end
between (`BATTLES`: a key script, its times moved, its pokes): `end57`
types map 57's code, START, plays `m57_win`'s keys 8 s earlier with its
pokes (the first round's end leaves the second side without units, CDh
with 1), types the best score's name (A, Enter), leaves the best scores
and takes START again (the menu's second return, 95.17 s); `live57` is
the same as the headless window played it from the menu on:
`tests/keys/menu57_win.keys` is what its `-keylog` wrote, with the map's
code as `code1` to `code5` (filled in from `CODES.DAT` at run time).
For both the original's run now also gives memory and video memory at
the end's two waits and the frames 1, 11 ... 121 of CDh, and each walk
has its own `-state` folder emptied before every run (the port's
`port_saves/` likewise), whose files of the first run are compared with
the port's. `sh port/tests/menu_test.sh end57 live57`: what came in the
same in both (7 things, times at most 0.06 s apart); no byte different
at the menu's two returns in the regions compared, at both waits and all
13 frames (memory as `menu_cmp.py` has it, both video pages), and in the
menu after the battle (92 s); `MAP\04.HI` the same. Told apart: the
battle's count of passes (347 in the original, 348 on the port's own
clock) and with it the timer's due pass at game_state+28h.
`sh port/tests/menu_test.sh live live57`: the window plays the walk's
script, its `-keylog` is the script again byte for byte, and its memory
at the second return, the menu after the battle and all 15 end dumps,
video memory included, are the same as the `-keys` play's.
The seven older walks rerun with their own `-state` folders
(`menu_dumps.py` and `menu_test.sh back code57 exit noload pages start0
start24`): all the same as before (`back`'s battle between now counts 130
passes in the original and 131 in the port).
Not done or not checked: the name's and the best scores' screens as
video memory, the battle on the original's clock inside a menu walk,
the SDL window by hand, the sounds.

2026-10-08 (`debian/window9`): the HISCORE screen, the winner's name and
the best scores after a battle, compared as game state and video memory.
Nothing the port does changed: `menu.c` only dumps at more screens
outside the menu's loop (`ask_screen`, `-askvram N FILE`, which now also
writes the memory as FILE with `.ram` for `.vram`). The screens, as they
come, at the original's addresses: the saved game's question and its
not-found text (S167A:1929, 1B5A, as before), text_input's beginning and
end (its calls of key_remove, S167A:334E, and of key_install, 3604: the
map's code typed and the winner's name), and the best scores drawn
(behind scores_show: S167A:0AAE after a battle, 1C7C for the menu's
HISCORE item). `menu_dumps.py` keeps the original's memory and video
memory there (askN.ram, askN.vram), `menu_cmp.py` compares both. `sh
port/tests/menu_test.sh back code57 end57 exit noload pages start0
start24` and `sh port/tests/menu_test.sh live live57`: all 24 screens
(back 2, code57 2, end57 5, live57 5, noload 7, pages 3, the HISCORE
screen among `pages`' and the name's prompt, the name typed and the best
scores among `end57`'s) no byte different in the regions compared and in
both video pages; in `end57` and `live57` the battle's count of passes
(348, port 349) and with it the timers' due passes told apart, as
before. The window's five screens of `live57` are the same as the
`-keys` play's, its `-keylog` the script again. The other walks' results
as before.
Not done or not checked: the HISCORE screen with a map that has scores
(the walk's map 0 shows four `EMPTY`), a name typed with the backspace,
the battle on the original's clock inside a menu walk, the SDL window by
hand, the sounds. `menu_test.sh live` for walks whose scripts no
`-keylog` wrote (all but `live57`) reports the keylog NOT the script and
some memory DIFFERENT; that was not looked into (it was never claimed).

2026-10-09 (`mac/scores`): the best scores of a map that has scores and
a winner's name corrected with the backspace, compared as game state and
video memory, in the headless window too. Nothing the port does changed.
Four walks more in `menu_dumps.py`, all with `m57_win`'s battle and its
pokes: `name57` types the name as a letter, the backspace, the backspace
with nothing left, five letters, a sixth (not taken), the backspace
twice, a letter, Enter (four letters stay), leaves the scores, goes to
MISC and shows the best scores of the map just won (one score, three
`EMPTY`); `twice57` begins with the scores' file `name57`'s run of the
original wrote (`SEEDS`: copied into the runner's `-state` folder before
every run and into the port's `-saves` folder by `menu_test.sh`; so
`name57` is dumped first), types map 57's code, shows the best scores
(the one there), wins the battle again and types a second name with one
letter taken off; `livename57` and `livetwice57` are the two as the
headless window played them (`tests/keys/menu57_name.keys`,
`menu57_twice.keys`: its `-keylog`, the code's letters as `code1` to
`code5`). The battle's end is not dumped again for them (`NOENDS`;
`end57` and `live57` have it). Each walk has six screens outside the
menu's loop (text_input's beginning and end for the code and the name,
the best scores after the battle and by the menu's item).
`sh port/tests/menu_test.sh name57 twice57` and `sh
port/tests/menu_test.sh live livename57 livetwice57`, after origin/master
4dfe673 was merged:
- what came in the same in all four (14 things in the two `name57`, 12 in
  the two `twice57`; times at most 0.06 s apart);
- memory at the menu's second return and at all 24 screens: no byte
  different in the regions compared, with the battle's count of passes
  (348, port 349) and the timers' due passes told apart in the two
  `name57`, and the first view's repeat counter (1 byte) in the two
  `twice57`, where the count is the same;
- video memory: both pages the same at all 24 screens and at the menu's
  passes kept (4 in each `name57`, 3 in each `twice57`), but for the pass
  at 102 s of the two `twice57`, where the two pages are exchanged and
  the same then (told apart, see below);
- the scores' file (`MAP\04.HI`) the same in all four: one score with
  the corrected name, then two;
- the window: its `-keylog` is the script again for both, and its memory
  and video memory are the same as the `-keys` play's at all 81 dumps.
The key times are chosen: whether the cursor's `]` gets into a short
name and which page has the pointer afterwards depend on the count of
text_input's passes, which is not the same on the port's own clock
(docs/HANDOFF.md, "The best scores with scores there and the name
corrected"). `menu_cmp.py` tells two things more apart for that: a
pass's two pages exchanged (each must then be the same as the other
program's other page; `--vram A B` does it for two files, used by
`menu_test.sh live`), and the views' repeat counters for every walk with
a battle played to its end. The earlier walks rerun with it (`back
code57 end57 exit noload pages start0 start24`, `live live57`; `end57`
and `live57` dumped again here): all the same as before.
Not done or not checked: four scores with the lowest replaced, a score
not above the lowest, a score outside 0 to 7EF4h in a file, the screens
between the name's keys, a name of five letters ended with the cursor
shown, the SDL window by hand, the sounds. Run on macOS only.
`menu_test.sh live name57 twice57` reports the keylog NOT the script, as
for every walk no `-keylog` wrote.

2026-10-09 (`debian/window10`): the computer's attack as the second side
in the window, with the fights' pictures compared frame by frame.
`m8_cattack.keys` (map 8, 200 s; the computer is the second side) is what
the headless window wrote (`-keylog`) playing `make_m8.py x45:200`: the
first side only ends its halves and presses F1, so every attack is the
computer's. In the original's run of it (`play_dumps.py m8_cattack.keys
200`, `VRAM=1`) attacks_run runs in passes 345, 1710, 1850 and 3172; the
two fights of pass 1850's attacks_run are shown with pictures (from
116.1 s and 126.1 s; why the other rounds' are not was not looked into),
and the original ends each at a key of the script after 149 and 151 of
749 frames (which key was not checked). New for the comparison: `play_dumps.py`
also dumps at every tenth frame of each fight's pictures (`gFNNN`, at
`S2532:156D`, where fight_frames tests the key at a frame's end), the
port's `-evdump` at the same place (`pics_hook`'s `PICS_FRAME`),
`play_cmp.py` compares them with the end's screens; and on the
original's clock `play_test.sh` now gives the port `-seeds`, so the
pictures start with the original's number (its S lines) and not the
BIOS's count: before, `rand_words` differed in the frames (given back at
the pictures' end, so the passes never showed it).
- `play_test.sh m8_cattack`: the inputs (7,148 parts), the game's state and
  the computer's the same at every pass compared (13), both pages the
  same at all of them, and at all 31 fight frames (1 to 141 and 1 to 151)
  memory and both pages the same;
- `rounds`: the same but for the passes' up to round 4, the last, pages
  included; `seeds`: the same up to round 3, then `rand_words` differ (on
  the port's own clock no key ends the fights' pictures, so they make
  other calls of rand_next than the original's cut ones);
- `live`: the window's play and the `-keys` play the same in the first
  7,066 parts, at every round's dump and at all 207 fight frames of their
  seven fights (memory and video memory); the dump after the last pass
  (3535) is not reached by the window, which closes a pass earlier.
  Since `debian/window11` the window is given pictures up to the end of
  the last round's end begun before the log's end and a second more
  (that round's end ran to 212.35 s, the log to 199.99 s): rerun, the
  same in all 7,068 parts and all 436 files, 3534 and 3535 included.
  `m8_site live` differs in its inputs with and without the change (177
  files, its script was not written by the window).
- After the merge of origin/master b371682, `m8_site`, `m48_sattack` and
  `m57_win` rerun on the original's clock with `-seeds`: still the same at
  every pass.
Not done or not checked: the frames of a fight on the original's clock
after a key ended the original's (the port's run goes on to 749 frames;
their dumps have no counterpart), a fight's pictures to their own end on
the original's clock, the SDL window by hand, the sounds.

2026-10-09 (`debian/window12`): the window's leftovers. A fight's
pictures on the original's clock now end where the original's key ended
them. The clock's file had no line for a fight's frame, so the port
showed all 749 frames of each fight where the original's F1 ended them
at 149 and 151 (`m8_cattack`); only the video memory behind the pages
showed it (11,220 to 12,947 bytes after the fights). `play_dumps.py`
writes a `G` line for each frame's end (`S2532:156D`), and `play.c`'s
`clock_frame` runs the log up to a frame's G line at that frame (the
timer's routine, the keys, the clearings of the key by the waits and
pictures, W/X/Y/E) and takes those lines out; S and R stay for the
round's end as before. A log without G lines (all made before
`debian/window12` but `m8_cattack`'s and `m8_rmouse`'s) plays as before.
- `play_test.sh m8_cattack` (clock.txt made again from its log): the
  inputs (7,148 parts), the game's state and the computer's the same at
  every pass, both pages the same at every pass and fight frame; behind
  the pages 632 bytes after the fights (before: 11,220). With the W lines
  left for the round's end, as at first, pass 1850's second side lost
  the F1 (3Bh) the second fight left: they are run with the frames.
- `m8_rmouse` (fight pictures on; the original run here, `VRAM=1`,
  `PASSES="5765 5809 5854 5881 5915 5984"`, 350 s): the inputs (9,730
  parts), the game's state and the computer's the same at every pass.
  In the 17 dumps of fight 2's frames `rand_words`' word 4 (DATA:0AF5)
  differs: `seed_set` writes words 0 and 2, the others are
  what fight 1 left, and the port gives the words back at a fight's end
  for the round's replay; the same with the G lines taken out. The
  pass before the last (4864) has 96 bytes different on the page (rows
  30 and 31, presumably the pointer; not checked), the same without the
  G lines. The original does not reach the repair with pictures on
  either (no text 51h): the script's times are `m8_d5r_nopics`'s.
- `m8_site`, `m48_attack`, `m54_end` (older logs, no G lines): still the
  same at every pass; `m8_cattack live`: the window and `-keys` the same
  in all parts and files.
Explained, not changed: the 1,442 bytes behind the pages in every
comparison are the logo screen's (`BB.IFF`, `HLLOGO.IFF`, in the
original's video memory 1 to 2 s after HL.EXE starts; the port does not
show it); the repair of `m8_mouse` is text 50h (the full count already);
the mickeys below a pixel (`play.c`'s comment); `rounds` from round 4
(`m8_site`): `seeds` shows `rand_words` differ first after round 3's
fights with pictures, which a key ends at another frame on the port's
clock. Not reached: D0h, CEh, CFh on the port's own clock;
`m54_own.keys` (`m54_end`'s with each direction let go with the
button) does not win there either; the original was not run with it.
Not done: the SDL window by hand, the sounds, the older logs made again
with G lines.

2026-10-09 (`debian/window15`): the battle's three end edges left by
`debian/window14`, by play, set beside the original on both clocks
(`sh port/tests/endclock_test.sh dumps` runs the original, without it
the port; why each script does what it does in docs/HANDOFF.md, "The
battle's three end edges left"):
- `m57_low` (map 57, 360 s): `m57_hq`'s keys with a scores file for
  map 57 in place (`MAP\57.HI` in the original's state folder,
  `MAP_57.HI` in the port's saves; four scores 620, 520, 420, 320, the
  lowest the battle's own, written by the test, not game data).
  battle_score finds the score not above the best: D0h with 0 and no
  CEh on both clocks (`m57_hq` has CEh with 2).
- `m60_first` (map 60, 1,640 s, fights not shown as pictures as
  `m60_fights`): `tests/keys/make_m60.py A B A N A B end=5,10`, the first
  side attacking in fights 3 and 4: 6/6 to 2/1, then 1/0. Texts 41h and
  42h, CDh with 1 and CEh with 2 on both clocks.
- `m60_both` (map 60, 1,700 s, the same): `make_m60.py A B A R4 R5 N R7
  R8 R7 R10 R11 end=6.4,12.4` with
    R4  1d-1,1m0,1L2,1L2.6,1a3.6,1a5.1,2E7.1,1E8.6   (to (44,19))
    R5  2m0,2L2,2a3.6,2a5.1,1E7.1,2E8.6              (to (45,18))
    R7  1m0,1R2,1u2.6,1a3.6,1a5.1,2E7.1,1E8.6        (attack)
    R8  1L2,1d2.6,2E7.1,1E8.6                        (cursor back)
    R10 1L0,1d0.6,1m2,1R4,1a5.6,1a7.1,2E9.1,1E10.6   (to (45,19))
    R11 1m0,1u2,1a3.6,1a5.1,2E7.1,1E8.6              (attack)
  three fights where the walk left them (3/3), two from the class 1
  field (44,19) (2/2, 1/1), the last from (45,19): both cavalries
  destroyed in the same fight, 15h, texts 42h for both sides, no
  picture behind the loop, on both clocks.
- Compared: on the original's clock (`play_test.sh`) the inputs, the
  game's state and the computer's the same at every pass compared
  (`m57_low` 3,615 passes; `m60_first` 23,199, `m60_both` 24,622,
  about 196 and 192 places each), both end waits and every tenth
  picture frame with video memory. On the map-60 runs what differs is
  `m60_fights`' (its walk is theirs): pass 1's setup bytes and the
  second side's bytes 5 and 8 in pass 4748; `endclock_test.sh` accepts
  exactly that. On the port's own clock (`rounds`) the same at every
  round's end and the pass after it (15, 87 and 92 rounds), at both
  end waits and at every tenth picture frame (7 of 7 and 26 of 26,
  both pages). Time by the BIOS's count from the first frame: `m57_low`
  within 1 tick, `S066A:4728` +0; `m60_first` within 7, +6, all of it
  between CDh's frame 121 and CEh's first, where the original's runner
  called the timer routine 68 times a second (docs/HANDOFF.md); by the
  runner's clock that stretch is 0.066 s longer in the port.
  `endclock_test.sh` allows 7 ticks for `m60_first`, 3 for the others.
- `endclock_test.sh` plays the three new tags too (the `_nopics` ones
  with `-nopics` on the own clock as well) and skips the pictures' time
  when there is no picture.
- Nothing of the port's source changed. Not done: the SDL window by
  hand, the sounds. Not run here after the change: the other
  `play_test.sh` scripts (no source changed). The original's runs were made
  before origin/master was merged in (7c35db0, a fast-forward, which
  changed six files of port/src), the port's comparisons all after it.

2026-10-09 (`debian/window14`): the battle's end by play on the port's
own clock, no pokes, set beside the original's (`sh
port/tests/endclock_test.sh dumps` runs the original, without it the
port; the original's runs with the runner rebuilt at doskit as this
branch has it). Both scripts made by `tests/keys/make_two.py`, every
cursor step a tap of 0.06 s, every gesture with its direction let go
0.03 s after the button (docs/HANDOFF.md, "The end by play on the own
clock"):
- `m54_hq` (map 54, 350 s), `m54_end`'s battle with a step of its own
  where `m54_end`'s longer gestures stepped the cursor:
    1E45/0.03 2E47/0.03 2f51 2f51.8 2R56/0.06 2R57/0.06 2R58/0.06
    2R59/0.06 2R60/0.06 2R61/0.06 2L62/0.06 2u63/0.06 2u64/0.06
    2u65/0.06 2u66/0.06 2m68/0.03 2u70/0.06 2d71/0.06 2a73 2a75
    2E77/0.03 1E79/0.03 2f83 2f83.8 1E88/0.03 2E90/0.03 2f94 2f94.8
    2m99/0.03 2u102/0.06 2a104 2a106 2E108/0.03 1E110/0.03 2f114
    2f114.8 1E119/0.03 2E121/0.03 2f125 2f125.8 2m130/0.03
    2u132/0.06 2R133/0.06 2a135 2a137 2E139/0.03 1E141/0.03 2f145
    2f145.8 1E150/0.03 2E152/0.03 2f156 2f156.8 2m161/0.03
    2u163/0.06 2R164/0.06 2d164.9/0.06 2a166 2a168 2E170/0.03
    1E172/0.03 2f176 2f176.8 1R181/0.06 1u182/0.06 1u183/0.06
    1m185/0.03 1u187/0.06 1a188 1a190 1E192/0.03 2E194/0.03 2f198
    2f198.8 2m203/0.03 2u205/0.06 2u206/0.06 2a208 2a210 2E212/0.03
    1E214/0.03 2f218 2f218.8 1E223/0.03 2E225/0.03 2f229 2f229.8
    2m234/0.03 2u236/0.06 2L237/0.06 2a239 2a241 2E243/0.03
    1E245/0.03 2f249 2f249.8 1E254/0.03 2E256/0.03 2f260 2f260.8
    2m265/0.03 2d268/0.06 2L268.9/0.06 2u269.8/0.06 2a270 2a272
    2E274/0.03 1E276/0.03 2f280 2f280.8 1a300  1a305 1a305
  The second side takes the first side's headquarters in round 14, the
  texts 3Dh and 3Ch, pictures D0h with 1 and CFh with 2 on both clocks.
- `m57_hq` (map 57, 360 s): the second side moves its unit off (7,10),
  the first side's cavalry walks from (8,1) into the second side's
  headquarters (7,11) in round 15:
    1E45 2E46.5 2f50 2f50.8 2u54/0.06 2u55/0.06 2m57/0.03 2u59/0.06
    2a63 2a65 2E68/0.03 1E70/0.03 2f74 2f74.8 1R79/0.06 1u80/0.06
    1m82/0.03 1R84/0.06 1d85/0.06 1d86/0.06 1d87/0.06 1a89 1a91
    1E93/0.03 2E95/0.03 2f98 2f98.8 2m100/0.03 2L101.5/0.06 2a102.5
    2a103.5 1E103/0.03 2E104.5/0.03 2f108 2f108.8 1m113/0.03
    1d115/0.06 1d116/0.06 1a120 1a122 1E124/0.03 2E126/0.03 2f129
    2f129.8 1E134/0.03 2E135.5/0.03 2f139 2f139.8 1m144/0.03
    1L146/0.06 1L147/0.06 1a149 1a150.5 1E152/0.03 2E153.5/0.03
    2f157 2f157.8 2E163/0.03 1E164.5/0.03 2f168 2f168.8 1m173/0.03
    1d175/0.06 1u176/0.06 1a177 1a178.5 1E180/0.03 2E181.5/0.03
    2f185 2f185.8 1E190/0.03 2E191.5/0.03 2f195 2f195.8 1m200/0.03
    1d202/0.06 1a205 1a206.5 1E208/0.03 2E209.5/0.03 2f213 2f213.8
    2E218/0.03 1E219.5/0.03 2f223 2f223.8 1m228/0.03 1d230/0.06
    1u231/0.06 1a233 1a234.5 1E236/0.03 2E237.5/0.03 2f241 2f241.8
    2E246/0.03 1E247.5/0.03 2f251 2f251.8 1m256/0.03 1d258/0.06
    1a261 1a262.5 1E264/0.03 2E265.5/0.03 2f269 2f269.8 1a285 1a290
  Texts 3Ch and 3Dh, pictures D0h with 0 and CEh with 2 on both clocks.
- Compared (`play_test.sh TAG` on the original's clock, `rounds` on the
  port's own): the inputs, the game's state and the computer's the same
  at every pass compared on the original's clock; on the own clock the
  same at every round's end and the pass after it (14 and 15 rounds;
  the passes' counts apart, the port's passes being shorter: 3,896
  against 3,854, 3,654 against 3,616) and at every tenth frame of the
  pictures, both pages of video memory included (20 of 20 frames each).
  Time, by the BIOS's count of ticks: the first frame comes 8 and 7
  ticks later in the port (the battle's drift over its rounds, the
  matter of `debian/window11`); from the first frame on every tenth
  frame lies within 3 ticks of the original's and `S066A:4728` within
  3 (`m54_hq` +3, `m57_hq` -1). `endclock_test.sh` now measures the
  pictures from their first frame (for `m57_won` and `m57_medal` too:
  within 1 and 2 ticks, `S066A:4728` +0 and +2) and the battle's drift
  apart.
- Nothing of the port's source changed; the tests and the key scripts.
Not reached: 15h, CDh with CEh, a score not above the best on the own
clock. Not done: the SDL window by hand, the sounds. Not run here after
the change: the other `play_test.sh` scripts (no source changed).

2026-10-09 (`debian/window13`): a battle's end on both clocks, the two
gaps the window's notes left. `sh port/tests/endclock_test.sh dumps`
runs the original three times, `sh port/tests/endclock_test.sh` the
port and compares.
- The battle on the original's clock inside a menu walk (`m57_menu`:
  map 57's code typed, START, `m57_win`'s keys 8 s earlier and its pokes,
  the best score's name, the best scores left; the original to 95 s, its
  second START at 94 s begins a battle at 97.9 s). The port's menu
  after such a battle got no keys: `play_begin` had cleared `in_menu`
  and the keys came only with the log's K lines, so the name's screen
  waited for ever; and the port's own clock still stood where the menu
  had been left (22.9 s). `play_after` now hands the clock back: the
  menu goes on on the port's own clock from the log's time at
  `S066A:4728` (75.78 s) and at its timer's next call, its keys by their
  times. Result: the inputs (696 parts), the game's state and the
  computer's the same at every pass (1, 50, 200, 347 to 350), both waits
  and all 13 frames of CDh, both pages included; the menu then asks for
  the name, shows the scores and leaves at the second START (95.11 s).
  The scores' file differs in the name's second byte: the original's has
  the cursor `]` behind the A, the port's a 0. text_input writes it
  every 17th of its passes, which are the port's own here; the known
  matter of `name57` (docs/HANDOFF.md), not changed.
- D0h and CFh on the port's own clock (`m57_won`, `m57_medal`: no keys,
  pokes in `tests/keys/*.pokes` end the battle with its first pass,
  `m57_medal`'s with a round and a score so that a best score follows;
  not the game's own doing). The port tested whether the battle goes on
  after its pokes and so left before pass 1, where the original (poked
  at `S066A:0EE9`) played pass 1 and left after it: `main.c`'s
  `play_game` now tests the state from before its pokes. On both clocks
  the game's state and the computer's are the same at every pass and
  every tenth frame (7 of D0h, 20 of D0h and CFh), both pages included
  (on the own clock but for the setup's bytes that `-map` leaves
  unwritten; `play_cmp.py` now takes those from the first pass when no
  round's end was dumped). The time on the own clock, by the BIOS's
  count of ticks from the first pass: CFh began 20 ticks (1.1 s) early,
  because behind D0h's last frame the original takes 2.74 s to
  battle_score (2.725 s in `m54_end`) and the port counted 1.63 s; the
  missing part is presumably `music_fade(0Ah)`, which the port leaves
  out (`EV_WON_END`, 1.11 s, counted now; what in it takes the time not
  checked). Now every tenth frame lies within 2 ticks of the original's
  and `S066A:4728` within 1 (`m57_won` 301 against 302, `m57_medal` 733
  against 732).
- Rerun after the change: `play_test.sh` `m57_win` (and `rounds`),
  `m57_last`, `m54_end`, `m8_site`, `m8_cattack`, `loop_test.sh`,
  `menu_test.sh end57 live57`, `save_test.sh`, `fkeys_test.sh`,
  `options_test.sh m48_joymenu` (their originals' dumps made again): the
  same as before.
Not reached: CEh on the own clock (the first view's word 22h is neither
0Fh nor 11h in these pokes; its frames are CFh's code with another
side), D0h or CFh on the own clock by play rather than pokes. Not done:
the SDL window by hand, the sounds.

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
Not translated then: the music's start, fade and volume (`music_start`,
`music_fade`, `music_volume`, `fade_tick`; done since, the next
paragraph), the
digital mixer and the card (`dig_mix`, `dig_irq`, `dig_kick`), `snd_end`,
`dsp_rate`, and the calls of `sfx_play`, `dig_play` and `chan_stop` by
`attacks_run`, `event_picture`, `site_picture` and the fight's pictures
(`src/orders.c`, `src/fightpics.c`: still left out there).
docs/HANDOFF.md ("The sound") says what sound output would need.

2026-10-08 (macOS, branch `mac/fightsound`): the fight's sounds, in
`src/sound.c`, still without any sound coming out. `pics_play` (the
sound loaded for one of a fight's six slots, by `dig_play` or
`adl_play`, then `snd_pass`) with the three places `fight_frames` calls
it from as routines (`pics_volley_sound`, `pics_begin_sound`,
`pics_side_sound`), `pics_sounds_stop` (the channels stopped at the
pictures' end) and `fight_blast_sound` (`attacks_run`'s effect 5 when a
fight without pictures destroyed a unit). The fight's panel already had
its sounds (`panel_sound`, `src/draw.c`). `src/fightpics.c` and
`src/orders.c` do not call them yet: `src/main.c` sets itself before
`play.c`'s `pics_hook` and `orders_hook` (`fight_sounds_wire`) and
`fight_pics_sounds` works a frame's sounds out behind the frame from
the figures' records; docs/HANDOFF.md ("The fight's sounds") says what
that assumes and which calls go inside once the two files' owners put
them there. `-fxlog FILE` writes each trigger (for the test). Left for
sound output, besides what is said there: the fight's pictures play
digital sounds only, so they need the mixer (`dig_mix`), which also
ends a channel; until then a sound played once stays marked as playing
until the pictures' end stops all channels.

2026-10-08 (macOS, branch `mac/music`): the music's control, in
`src/sound.c` over the program's memory, still without any sound coming
out. `music_start(tune, fade, volume)` (the tune loaded and started on
channel 0Ch, at the volume at once or from 0 in `fade` steps),
`music_fade(speed)` (the effects stopped; the music off at once with
speed 0, else faded to 0 in `speed` steps, waited for, then the voices
silenced, `opl_silence`), `music_volume` and `fade_tick`, the fade's
timer routine (period 1D1EBh timer counts, a tenth of a second), with
`fade_start` and `fade_stop`. The timer itself (`S27FE`: `pit_irq` and
its tables) is not translated: `snd_irq` is one of its interrupts as far
as the sound goes, `fade_tick` first when its period has run out, then
`adl_tick`; `snd_clock` calls it at the period the tune set, which is now
read from the driver's `adl_period` (`S30EF:0000`; 1843h while it is 0)
and no longer kept beside the memory. `music_fade`'s wait runs the
interrupts itself until the fade is over (speed + 1 calls of
`fade_tick`: 350 interrupts for speed 4 with the menu's tune) and tells
`snd_waited` how long that was; the sound's clock is ahead of the
caller's by that time and `snd_clock` calls nothing until it has caught
up.
Called where the original calls them: `start_setup` (`music_fade(4)`
before the keyboard's interrupt is set; nothing plays yet),
`battle_begin`'s end (the battle's tune, no fade, volume FFh, with 40h
in `game_state`), F2 in a battle (`setting_keys`, `src/loop.c`),
`battle_after` (the fade at the loop's end), the menu's four starts
(fade 4, volume FFh) and four fades (4) in `src/menu.c`, the new units'
screens (`src/briefing.c`: fade 5), and `attacks_run`'s three places.
Those are made from `src/loop.c` around its call of `attacks_run`,
because `src/orders.c` leaves them out: the fade before the fights when
the side has orders, the fade at a site taken through `orders_hook`'s
`ORDERS_TAKEN` (the hook set before is called behind it), and the start
at the end when one of them came; nothing between those places and the
original's reads the sound's state. In the menu and the new units'
screens the fade's wait moves the port's clock on (`snd_waited`); in a
battle it does not (the times of `attacks_run` on the port's own clock
are `src/play.c`'s and were measured with the fade in them, presumably;
not checked), and F2's half second is not counted there either.
`-muslog FILE` writes each call of `music_start` and `music_fade` at
its end (time, which, the numbers, the result, the battle's pass).
Not translated: `snd_end`, the calls of the music's routines by `S1A39`
(a taken site's picture, the end's screens: fades of 10 and 20 steps
and their own tunes), the timer's tables (so the number the timer gives
the fade's routine, `S30EF:1118`, stays 0), and as before the digital
mixer and `dsp_rate`. On the original's clock (a battle played with
`-clock`) `snd_clock` is not called, so a tune started there does not go
on but in a fade's wait.

2026-10-08 (macOS, branch `mac/sfx`): the effects' other ways and the
digital mixer's memory side, in `src/sound.c`, still without any sound
coming out. `sfx_play` and what it reaches were already there as read;
their ways not reached before (a call that fails, an effect played more
than once or endless, an 8SVX effect) are now compared and needed no
change. New: `dig_mix` (each pair of bytes of a buffer from the playing
digital channels' bytes, their volumes and their count; a channel at
its end begun again or taken off `dig_mask` and `dig_count`), `dig_irq`
(the card's interrupt: the buffer that waits started, the other mixed
anew), `dig_kick` (the first buffer mixed and the interrupt, from
`snd_pass` while digital channels play, as in the original) and
`dsp_rate` (the rate, kept beside the memory; `start_setup` calls it
with 10000 where `game_loop` does). The bytes stay in the driver's two
buffers in the program's memory. The card's interrupt, which the
original gets when a buffer has been played, comes from `snd_clock` the
buffer's length / the rate after it was started, on the port's own
clock only; with `-clock` only `snd_pass` moves the digital channels.
So a digital effect now ends and counts its times down as in the
original at the calls compared, where before it stayed marked as
playing until `chan_stop` or the next effect on its channel.
Called where the game calls `sfx_play`: nothing new was needed. Of the
original's 20 call sites the port has the ten that lie in what is
translated (`attacks_run`'s, `game_loop`'s two, `panel_sound`, the
menu's four, `pic_wiped`'s two in `src/events.c`); the other ten are in
the pictures C8h, C9h and CCh of `event_picture`, which the port does
not have (docs/HANDOFF.md, "The effects' ways and the mixer", has what
they play and that no caller passing these kinds was found).
`-sndlog` now writes the volume, the count of times and the picture's
frame (`event_frame`, counted in `src/events.c` for the test) behind
each call's line.
Not done: sound output (docs/HANDOFF.md says what it still needs: a
hook for the buffers' bytes where `dig_irq` starts one, the queue
dropped at `dig_kick`, the interrupt from the audio's clock), `snd_end`,
the three pictures.

2026-10-08 (macOS, branch `mac/opl`): the AdLib's output, in
`src/sound.c`, still without any sound coming out. The routines already
handed each register and its value to `snd_opl` in the original's
order; added are the writes of the start and the end (`adl_detect` in
`snd_init`: the chip's timer test, the voices silenced, the waveforms
allowed; `adl_remove` and `snd_end`, which `snd_init` calls when it is
called again; `game_loop`'s end in `src/loop.c` does not call
`snd_end` yet) and the timer's count as `pit_irq` keeps it for
`adl_tick` and the fade's routine (`snd_irq`, which now returns which
of the two it ran): a period a tune sets counts from the interrupt
after the next, so a fade begun with a tune steps three ticks earlier
than the port had it. `music_fade`'s wait asks `snd_wait` for the next
interrupt when that is set (for an audio clock, and the test).
`src/main.c` sets `snd_opl` to a routine that feeds an OPL2 of the
runtime (`doskit/runtime/opl.c`, added to both build scripts); nobody
asks it for samples, because the interrupts still run on the port's
clock of passes. `-opllog FILE` writes each register (the port's time,
the driver's count of ticks, register, value) and a line at the end of
each `music_start` and `music_fade`.
Not done: the audio's clock and with it any sound (docs/HANDOFF.md,
"The AdLib's output", lists what it needs, part of it in doskit);
`snd_end` called by the loop's end. Not verified: see "Checked".

2026-10-08 (macOS, branch `mac/soundout`): the AdLib is played, on the
audio's clock. **Nothing was listened to**: what is known of the sound
is that it is rendered, the same in every run, with each write at its
sample ("Checked").
- `src/main.c` asks the platform for an audio device after the game's
  files are found (`plat_audio_start`, 44100 Hz). With one, what
  `snd_opl` gets goes into the runtime's queue (`doskit/runtime/oplq.c`,
  added to both build scripts) with its sample plus a lead of 1234
  samples (28 ms, two pictures), and the device's callback renders the
  chip through the queue, the same on both channels. Without one (the
  headless program unless `DK_AUDIO=1` or `DK_WAV=FILE` is set, so every
  earlier test) everything is as before: the chip gets the writes at
  once, nobody asks it for samples, the interrupts run on the port's
  clock of passes.
- The mix (`audio_fill`, `src/main.c`): the chip's samples are doubled
  (+6.02 dB, clipped) before the Sound Blaster's bytes are added. The
  battle tune's mix was 6.27 dB below DOSBox 0.74's (sb16, `oplmode`
  auto, `oplemu` default, 44100 Hz) with the card at full scale in
  both (docs/HANDOFF.md, the live gap). `tests/soundout_ref.c` renders
  the chip with the same doubling, `tests/oplwav_cmp.py` takes the
  port's file at 1/2 (`PORT_GAIN`); both pass.
- `src/sound.c`: with a device (`snd_samples` set) `snd_clock` makes the
  timer's interrupts by the samples played, not by the time it is
  given. An interrupt is due at the sample the timer's periods since
  the start come to (`pit_least` at each interrupt, PIT counts times
  44100 / 1193182 with the sum divided whole, so 06A9h's 63.02 samples
  carry their fraction), and what it writes has that sample
  (`snd_stamp`); a write the game's side makes has the samples played
  at that moment. `music_fade` still makes its fade's interrupts at
  once, each at its own sample ahead of the samples played, and the
  caller's clock goes on by the wait as before (`snd_waited`); the
  clock makes none until the samples have caught up.
- The headless device has no clock of its own (`plat_null.c`: time
  passes in `plat_sleep_ms`), and a scripted play does not sleep, so
  `main.c` moves it on to the port's clock whenever `snd_clock` asks,
  in whole milliseconds. A live play (the window; headless with
  `DK_KEYS`) sleeps a picture at a time itself and is left alone.
- `-oplat FILE` lists what was queued (each write with its sample, each
  interrupt with its sample and the period to the next); the program's
  last line to stderr says the samples played and how many writes the
  queue had late, early and still waiting.
Not done: the Sound Blaster's bytes are not played (the digital
effects stay silent, their interrupt on the port's clock); the time the
original spends inside a tick is not modelled (a tick's registers fall
on one sample); the tune is not compared with a rendering of the
original's stream (the runner's `-oplwav`). Not verified: see "Checked".

2026-10-09 (macOS, branch `mac/oplwav`): the rendered AdLib sound set
beside the original's (the runner's `-oplwav`), and the 70 registers a
battle writes at another place with a device judged ("Checked" has the
numbers). No change to what the program plays. **Still nothing was
listened to.**
- `src/main.c`: `-oplcount RATE STEP`, a test's: without a device the
  timer's interrupts are counted as with one, by samples at RATE a
  second on a clock moved in steps of STEP microseconds (0: none);
  44100 and 1000 is what the virtual device does. `-opllog` has a line
  `fading` where a `music_fade` begins (`snd_music` is called there
  with 2), so a tune's writes can be told from its fade's.
- Found, not changed:
  - the 70 registers are the millisecond of the virtual device's clock,
    no offset;
  - the original's tunes fall behind the port's: 10 ms in a tune's
    first tick (it writes the instruments' 116 to 120 registers at
    about a tenth of a millisecond each and runs 11.6 to 11.9 ms; the
    timer's interrupts due meanwhile are lost but one), and on the
    runner 98 periods (140 ms) in 119 s of a battle with its fights.
    The port's ticks are the periods summed and lose none;
  - a tick's writes on one sample (the port's way) move the level of
    half a second of a tune by up to about 1 dB against the original's
    spread over the tick; the chip's rate (49716 or 44100 Hz) by 0.07
    dB at most;
  - `-map N` starts the battle's tune with the driver untouched: its
    voices begin at the level 3Fh (silent) until each voice's own
    volume event, where the original, which always has played the
    menu's tune before, begins at the levels the voices' own volumes
    left by that tune give. Through the menu the port's start is the
    original's (the walk `brief`, tune 4).
Not done: nothing listened to; a tick's time not modelled (the lost
interrupts, the writes' spread); the direct start's volumes; the Sound
Blaster's bytes. Not verified: see "Checked".

2026-10-09 (Windows, branch `win/intro`): the intro and the outro, the
launcher's INTRO.EXE and EXTRO.EXE (`src/seq.c`). Their own modules
(INTRO `S033D`, EXTRO `S0340`) are translated over HL.EXE's memory; the
libraries they link are HL.EXE's (matched routine by routine on their
bytes), so the port's translations are called: page_flip, page_copy,
fade_in/out, pal_level, pal_blend, rect_fill, `vdif` (the streams),
lbm_show, file_load_size, sfx_load/sfx_play, music_start/fade, and the
newspapers' scrolled text (`font_load`, `paper_*`, now exported from
`src/events.c` with `vdif`, `planes_keep`, `planes_put`). New in
`src/sound.c`: `music_beat` (S2F39:1A0B, the beats a tune has played,
which INTRO waits for), `music_seek` (S2F39:11AC: the tune's voices
worked out up to a beat, a loop between two beats set) and
`music_restore` (S2F39:18FE, which `adl_tick` now calls at the loop's
end). The modules' data (file names, scene records, the frames' tables
of palettes, sounds and the countries' colours) is read at run time
from the player's INTRO.EXE / EXTRO.EXE (their size checked) into a DOS
block, so its offsets are the original's. The files are found in
INTRO\ and EXTRO\ (`host_load` now goes up for `..\`, the fonts).
INTRO: BANG (title text from CHR14.LIB, the shot with its palettes and
sounds by frame number, the tune from frame 87h), BUCH1A, BUCH1B, AUTO
(the module's own CRTC panning, the car's colours from CARPAL.DAT),
the kept picture scrolled and MORD, BUCH2A, BUCH2B blended to its
second palette, the countries (LAENDER.LBM, buch3.txt scrolled, the
countries blended by the text's codes, the tune looping beats 6Fh to
EAh), BUCH3. EXTRO: the war's end, the treaty with EURO.ANI, the
crater, THEEND.ANI, the credits (loops 107h-162h and 1-16Bh). Only Esc
ends either early; other keys are taken and ignored, as in the
originals. The port's clock counts what the runner's original spends
computing (a file 0.05 s + 5.5 us a packed byte, a stream frame 0.8 us
a byte, page_copy 5 ms, a text row 16.6 ms (INTRO), 41 ms (EXTRO's
F0h-wide windows), 23.3 ms (credits); the pan's wait for a retrace's
end) and music_fade's wait. A test aid, not the launcher: `-intro` /
`-extro` play one by itself and end the program (in the window live),
`-seqlog` prints each page shown, `-seqdump N FILE` writes memory,
`.vram` or `.dac` at page N.
In the window, from the menu on, the program plays them where the
launcher HL14-18.EXE does: INTRO.EXE before HL.EXE (its CODE:0054; Esc
ends it, as INTRO's own Esc does, and the game starts whatever it
returns), on HL.EXE's loaded image, which is then loaded again so the
game starts on fresh memory; EXTRO.EXE only when HL.EXE would end with
7Bh, the last battle won (its CODE:009A, CODE:00D5), after which the
program ends. `-nointro` leaves the intro out (the launcher has no such
choice; the player can also press Esc). Scripted runs (`-keys`,
`DK_KEYS`, headless) and `-map` play neither, so their times stay.
Checked: the build, `check.py`, `-extro` headless (4530 pages, to
561.78 s, as before). Not checked: the intro and outro in the live
game path (left to the player's run), a key held over from the intro
into the menu.
Checked against the runner's originals (INTRO.EXE alone, EXTRO.EXE
with `0 HL14-18`): the same number and order of pages shown (INTRO
1831 flips and pans, EXTRO 4530 flips, the screen_open's own one aside);
at 15 pages of each spread over all scenes (INTRO 2 ... 1800, EXTRO 2
... 4500; breakpoints `INTRO.EXE+09B5:000C#N` / `+033D:1A47#N`,
`EXTRO.EXE+0514:000A#N`) both video pages and the DAC are byte for
byte the same. Times: INTRO ends at 370.6 s (original 368.1), within
-0.8 to +2.6 s of the original at every page; EXTRO 561.8 s (561.7),
within -0.5 to +1.2 s. Esc at 20 s ends the port's intro at 20.61 s.
Not checked: the sound by ear and against the original's OPL stream
(music_seek/restore are read from the code, not compared); the live
window (the player checks); the Sound Blaster's effects (sfx through
the port's driver memory only). Not done: the launcher's wiring (the
intro before the menu, the outro after a 7Bh end; docs/HANDOFF.md).
Differs: the original's Esc is lost when its timer routine reads the
key first (seen at 20.00 and 70 s); the port always takes it. Where
DOS has less room than the original's mem_alloc asks (HL.EXE stays
loaded under it in the port) the largest block is taken.

2026-10-09 (Debian, branch `debian/sound`): the Sound Blaster's effects
are played. `sound.c`'s `dig_irq` hands each buffer it starts to a hook
(`snd_dig`: the buffer's bytes, unsigned 8 bit mono, and `dsp_rate`'s
rate), and `dig_kick` calls it with NULL where the original halts the
DSP. `main.c` queues the buffers one behind the other, reads them in
the audio callback with a linear step to 44100 Hz and adds them to the
chip's samples. A queue that has run empty begins AUDIO_LEAD samples
behind the samples played, as the AdLib's writes do, and a halt drops
only what lies beyond the game's moment (the lead ahead of the samples
played), so the buffers mixed anew follow on without a gap. The card's
interrupt stays on the port's clock (`snd_clock`), as before. That
covers the fights' pictures (`pics_play`, `dig_play`) and the intro's
BANG scene (BANG.SFX on the channels 0 to 7, `seq.c`); EXTRO plays no
effects (its folder has no .SFX or .AFX, its program names only
`EXTRO1.ADL` and `CREDITS1.ADL`, and `extro_play` calls no `sfx_play`).
Without a device nothing changes (the hook is unset). With one the
program's last lines to stderr count the card's bytes and halts. Not
verified: by ear; the level of the card's bytes against the chip's (an
effect peaks at about 5,500 where the music peaks at about 1,500, and
how a card and an AdLib compared on a PC was not measured); the
interrupt's time against the device's (the queue only joins the
buffers, so a port clock running ahead lets it grow, one running
behind leaves gaps); the window's program with a real device; Windows.
`soundout_ref` checks a WAVE file against its list of AdLib writes, so
a walk with digital effects in it would not pass that check. The walks
`soundout_test.sh` makes have none (map 8's first fight pictures come
after its 120 s), so it is not extended yet.

2026-10-10 (Debian, branch `debian/pictunes`): the tunes of the
pictures D0h (`music_start(0, FFh)` behind its `fade_in`,
`music_fade(0Ah)` behind `pal_level`) and CEh/CFh (`music_start(14h,
FFh)`, `music_fade(14h)` at the end) are played where the original
calls them (`src/events.c`); CDh has none. Checked by the new walk
`medal` of `oplwav_dumps.py` / `oplwav_test.sh` (map 57's end,
`m57_medal`): the calls' times within 0.07 s of the original's but
CFh's fade, 2.07 s late (its wait counted in `EV_LOAD_MEDAL3` before
it; open, docs/HANDOFF.md); CFh's tune every write the same at the same
tick, D0h's but its first tick on three voices. `oplwav_cmp` fails on
that walk for those first ticks and a level window (the original's
interrupt 72 ms behind): told, not judged. Not listened to; CEh not run.

2026-10-10 (Windows, branch `win/cffade`): CFh's fade on time. The
2.53 s counted at `EV_LOAD_MEDAL3` is split as the original spends it:
0.46 s at the third part's loading, 2.07 s at a new `EV_MEDAL_END`
behind `music_fade(14h)` (the call `pic_medal` ends with, S1A39:0D1A's
listing; its wait for the fade). Checked on the `medal` walk with
`-muslog` against the original's log (`oplwav_dumps.py medal`): CFh's
`music_fade` at 69.093 s against 69.095 (before: 71.162); the other
calls unchanged (33.294, 44.881, 48.490). Not listened to; CEh not run.
## Build and run

    sh port/build.sh          # macOS, Linux (SDL2 for the window)
    port\build.bat            # Windows (MSVC)
    port/build/history-line -game game
    port/build/history-line-headless -game game -map 5 -ram build/map5.ram
    port/build/history-line -game game -intro       # or -extro, in the window
    port/build/history-line-headless -game game -map 8 -keys port/tests/keys/m8_site.keys -start 31.1 -seed 563 -passes 2000 -ram build/played.ram
    port/build/history-line-headless -game game -keys build/menu/back/keys.txt -start 8.05 -until 54 -menulog build/menu.log
    port/build/history-line-headless -game game -keys build/play/m48_save/keys.txt -start 8.05 -clock build/play/m48_save/clock.txt -saves build/saves
    DK_WAV=build/m8.wav port/build/history-line-headless -game game -map 8 -keys port/tests/keys/m8_site.keys -seed 0 -until 120 -oplat build/m8.at

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
is the players' README in each package, with the game's keys (from the
setup's key tables in `src/main.c` and the battle keys above) and the
setup screen's; beside it `uninstall.cmd` (Windows), `uninstall.sh`
(Linux) and, as `uninstall.command`, the Mac's, made from doskit's
template (2026-10-10, branch `win/rc1`). The workflow is doskit's
template with the slug filled in, so a tag `vX.Y` gives the three
packages, each with README.txt, its uninstall script and the licences,
and `latest.json`; `PORT_UPDATE_URL` names
`DropIn-Classics/hl1914-18`, the Mac app is
`io.github.dropin-classics.history-line`. `uninstall.sh` was run under
Git Bash on Windows with `DK_DATA_DIR` set to a scratch folder: without
a terminal it removed nothing, `--yes` only `game`, `--yes --all` the
folder. Not run: `uninstall.cmd` with its questions, the `.command` on a
Mac, the workflow itself (it runs at the next push). seq.c's timer
wait (S033D:1856) is `timer_wait`, not `wait`: macOS's <stdlib.h>
declares POSIX wait() and clang refused the static one (branch
`win/macfix`); checked only with the Windows build, not on a Mac.

(which packages were started from a download, on which systems)

## Country screen, Windows rc2 (2026-10-10)

Investigated a corrupted German flag in the rc2 Windows package (not
in a dev build): not reproduced; gcc with UBSan/ASan and -O0 against -O3
through the walks `start0` and `start24` gave no difference and no UB in
the flag path (docs/HANDOFF.md, debian/flagcorr). `src/joy.c`'s
port_read no longer shifts a negative int (UBSan; same value).

## Checked

- DEBUG keys, the port's own (2026-10-10, Windows, branch `win/cheat`),
  in the window's live play only: F12 wins the battle on the spot, F11
  wins it as if it were the map before the campaign's last, so that the
  menu then offers the last map (23 for the maps 0 to 23, 47 for 24 to
  47; CODES.DAT's byte 8 bit 1, which makes `battle_score` set 20h and
  the program end with 7Bh, the outro). The keys never reach the game
  nor a `-keylog`, and a scripted run (-keys, -clock, -passes) never
  comes there (play.c's `live_key`); one pressed outside a battle is
  dropped when the next begins. loop.c's `cheat_win` does what a won
  fight does (both views to state 7, the human's word 22h 0Fh, the
  other's 10h); the end's texts, the pictures, the newspaper and the
  score follow as after a real win. As in the original, `battle_score`
  does nothing in round 0, so the map moves on only when a round has
  ended: press F11 or F12 after the first round. For the outro: F11 in
  a campaign battle after round 0, then on the last map F12 after round
  0. Checked headless (DK_KEYS, map 22): F11 ends the battle with the
  win's picture; with `round_count` poked to 1 (`-poke 38605 0100`)
  `-ends` has the pictures D0h and 16h (map 22's newspaper, chosen as
  the number before the one `battle_score` moved on to: 23). The menu
  then offering map 23 and the outro by eye in the window are left to
  the player. Dev builds only (2026-10-10, branch `win/nocheat`): the
  keys, `loop_cheat` and `cheat_win` sit in `#ifndef PORT_VERSION`, so a
  release build (build.bat/build.sh define PORT_VERSION only for a
  tagged or versioned build) has none and passes F11 and F12 on to the
  game like any other key.
- The newspapers' scroll (2026-10-10, Windows, branch `win/cheat`):
  its timing is the original's. A row a frame; a frame's count reset at
  its start, then retraces waited until the timer's routine has counted
  8 (TICK 3FFFh/1193182 s), as `S2D47`'s caller in `S1A39` does: frames
  of 7 or 8 retraces (100 or 114 ms), 0.11017 s on average by the
  port's clock against the original's 0.10985 s measured over m0_paper's
  994 frames (the 6 to 12 BIOS ticks noted with the newspapers). The
  uneven 7/8 retraces are the original's own; the window shows the
  70 Hz pictures as they come (doskit's frame.c, not synchronized with
  a 60 Hz display), which can add to the unevenness on such a screen;
  that is the kit's and is not changed here. Not changed: nothing in
  the newspaper code.

- The F1 prompt (2026-10-10, Windows, branch `win/f1prompt`). It is
  shown once when both sides have ended their halves (loop.c's `told`,
  the original's `[BP-28h]`); a later text in that line (F2 to F9's)
  leaves the line empty when its time is over, in the original as well.
  Compared on map 8 with the original's screenshots and the port's
  `-keys` video memory: without input the prompt stays until F1; after
  F2 it does not come back, in both. No change to the code.

- Two saves in a row (2026-10-10, Windows, branch `win/savecrash`).
  The window's play ended the battle at the second save's question: the
  wait for the digit (play.c, LOOP_SAVE) and the quit question's
  (LOOP_QUIT) gave up after 400 timer calls without input, counted
  across waits and live as well; now live they end only when the window
  is closed (`own_idle_tick`). `save_test.sh` has a run `m48_save2`:
  D 3, ten seconds later D 4, both files byte for byte the original's
  and memory compared on its clock, and the headless window's play of
  the same keys with four seconds before each digit, which must write
  both files and go on (before the fix it stopped at 61.53 s with
  03.DAT only).

- The snare in the battle's tune (2026-10-09, Debian, branch
  `debian/snare`; docs/HANDOFF.md has the numbers). The snare is
  channel 5, a melodic voice at feedback 7 (no rhythm mode). The
  port's `-map 8` run gives it the same instrument bytes as the
  original at every key-on. Rendered alone and in the mix through
  `opl.c`, DOSBox 0.74-3 and DOSBox-X's Nuked OPL (black boxes, the
  same register list): its level against the mix within 0.5 dB, each
  hit's attack within 1 dB, its spectrum 1-1.6 dB darker above 2 kHz
  than Nuked (the two references differ by up to 2.2 dB in a band).
  The window's output stage (DC blocker, volume, the card's bytes
  added) changes it against the mix by less than 0.7 dB. The port's
  whole output is 6.25 dB (DOSBox) or 3.71 dB (DOSBox-X) quieter.
  No change made. Not checked: by ear, the user's own setup of the
  original, a real chip.
- The AdLib beside the original's (2026-10-09, macOS, branch
  `mac/oplwav`): `python3 port/tests/oplwav_dumps.py menu brief battle`
  runs the original on the walks of `opl_dumps.py` with `-oplwav`
  (49716 Hz mono) and a log of every register, timer interrupt and
  tick (`build/oplwav/WALK.orig`, `WALK/orig.wav`); `sh
  port/tests/oplwav_test.sh` then, for each walk:
  - `port/tests/oplwav_ref.c fit`: the runner's file is the sound of
    its log, sample for sample: a second chip that gets each logged
    write at the sample of its logged moment gives every sample of the
    file (menu 3,281,256 samples and 3,303 writes; brief 2,982,960 and
    1,959; battle 8,948,880 and 11,443). The log's moments are whole
    microseconds, so a write within half a microsecond of a sample's
    edge is tried on both samples (152, 88 and 581 writes; 31, 27 and
    79 belong to the later one);
  - the port's headless program on the same keys with `DK_WAV`, and
    `soundout_ref`: its file is the sound of its list, as before;
  - `port/tests/oplwav_cmp.py`, tune by tune (a tune laid on the other
    at its first tick, the writes compared by the count of ticks from
    there):
    - the writes. In the four tunes during which the game's side wrote
      no register (menu 2: 1,470 ticks; brief 2, 3 and 4: 6,720, 8,505
      and the first tick) every write the timer's routine made is the
      same register and value at the same tick, on all voices
      (`adl_tick`'s; `fade_tick`'s 32, 30 and 25 at the same ticks
      too): judged. In the others told: menu 1 and 3 and brief 1 have
      the voices 0 to 7 the same throughout (1,373, 527 and 450
      writes) and voice 8, the effects', at other ticks (the game's
      side calls at other moments; menu 3's fade steps 2 ticks
      earlier). The battle's tune (83,370 ticks, 119.1 s; the port's
      from `-map 8`, the original's entered through the menu): of the
      3,629 writes to the voices 0 to 7 all but 35 the same at the same
      tick; the 35 are one level each in the first tick (the direct
      start, "State") and notes at three ticks (29191, 38221, 46831;
      presumably where an effect gives voices back, not checked);
    - the time. The port's ticks are the timer's periods summed, to the
      sample (judged; 06A9h, 0854h, 058Dh, 050Ah and 060Dh in the
      tunes). The original's second tick is late by what its first ran
      beyond a period (judged, within a period: the first runs 11.9,
      11.9, 11.9, 11.9, 8.5, 5.7 and 11.6 ms, the second is 10.5, 10.1,
      10.5, 10.5, 7.3, 4.6 and 10.0 ms late). At the tunes' ends the
      original is behind by 16.2, 10.1 and 12.0 ms (menu), 10.5, 7.3
      and 4.7 ms (brief) and 140.1 ms, 98.0 periods (battle; fallen
      behind for good at 86 ticks, 3 of them behind a tick that ran
      longer than a period): told;
    - the sound, over the ticks before the first write that differs, in
      windows of 350 ticks, by the level (root mean square), with two
      files rendered between (the original's list on a chip at 44100
      Hz; that with each tick's writes on the sample of its first).
      Judged: the rate's step within 0.2 dB (it is -0.05 to +0.07 dB
      over all windows), the port's file against the original's within
      0.5 dB on average and 3 dB in a window: menu 1 +0.04 dB (+0.01 to
      +0.08, 2 windows), menu 2 +0.08 (-0.01 to +0.14, 4), menu 3 +0.08
      (-0.08 to +0.43, 4), brief 1 +0.04 (2), brief 2 +0.33 (-0.01 to
      +0.82, 19, 3 silent in both), brief 3 -0.00 (-1.71 to +1.97, 24,
      3 silent). The step "each tick's writes on one sample" alone is
      up to +1.15 dB in a window, the ticks' moments up to +1.93 dB.
      The battle's tune differs at its first tick, so its sound is not
      compared.
  Then the 70 registers (the first 120 s of map 8, 10,231 registers):
  with the device they are at another place than without, and where
  the logs differ the device's count of ticks is the same or one less,
  never more. Counted without a device as the device does (`-oplcount
  44100 1000`) the log is the device's line for line; counted in steps
  of 100 microseconds, of 1, or at the timer's rate without steps, no
  register is at another place than without a device (74, 25 and 23
  lines differ in the count of ticks logged or the time). So it is the
  clock's millisecond: the virtual device is up to a millisecond
  behind the port's clock, and a tick due in that millisecond comes
  behind a call of the game's side, not before. No offset. A real
  device's buffer is coarser than that.
  By hand: in the battle the original falls behind where the fights
  play (0 periods in each 8 s without a write of the game's side, 27,
  20, 15, 14 and 8 in the 8 s stretches with 306, 218, 194, 172 and 80
  such writes), 82 of the 86 times behind a tick that wrote nothing.
  The earlier tests after the change: `opl_test.sh`,
  `soundout_test.sh`, `music_test.sh`, `sound_test.sh` as before;
  `check.py` all ok.
  Not checked: **nothing was listened to**; the files are not compared
  sample for sample (two rates, other moments), only by level, and the
  levels' limits are chosen, not derived; what keeps the timer's
  interrupt waiting in the fights (the game's side or another routine;
  the runner's machine at 6 million instructions a second, not a real
  one); the battle's sound; a tune entered as the original enters the
  battle; the kit's chip against a real one (`opl.h` says it is not
  the chip's exact output: both files are that chip's); Windows and
  Linux.
- The AdLib rendered headless (2026-10-08, macOS, branch
  `mac/soundout`; `src/sound.c`, `src/main.c`): `sh
  port/tests/soundout_test.sh` runs the headless program twice with
  `DK_WAV` (doskit's virtual audio device) and `-oplat` over two walks,
  the first 120 s of map 8 with `m8_site.keys` (`-seed 0`) and the
  menu's walk of `menu_dumps.py pages` (59.9 s), and for each
  - compares the two runs' WAVE files and lists byte for byte: the
    same (map 8: 5,293,411 frames, 21,173,688 bytes; the menu:
    2,640,002 frames, 10,560,052 bytes);
  - reads the queue's counts: 0 writes late, 0 early in both (the menu
    ends with 27 writes still waiting, queued for samples behind the
    run's end);
  - `port/tests/soundout_ref.c`: every interrupt's sample is the
    timer's periods summed (map 8: 83,995 interrupts, 2 at 1843h and
    83,993 at 06A9h; the menu: 41,077, 135 at 1843h, 39,102 at 06A9h
    and 1,840 at 0854h), every write an interrupt made is queued for
    that sample plus the lead (9,041 of 10,231 writes; 3,075 of 3,320),
    no stamp goes back, and every frame of the file, on both channels,
    is the frame of a second chip that got each write exactly at its
    sample, rendered from write to write. The file is silent up to the
    sample of the first write that sets a key on and sounds from the
    next (1693 and 1694; 32224 and 32225);
  - `port/tests/soundout_cmp.py`: each write's sample beside the port's
    own clock at the write (`-opllog`): never more than 14.5 ms behind
    it (the interrupts are made in one go for the samples since the
    clock was last asked); 223 of the menu's are ahead, by 499 ms at
    most (the fades', made at once), none of map 8's;
  - tells whether a run without a device writes the same registers:
    the menu's 3,320 the same in the same order; map 8's 10,231 the
    same registers, 10,161 of them at the same place (the device's
    clock goes in milliseconds, so an interrupt can fall on the other
    side of a call of the game's; not judged).
  A list with one write's sample or one interrupt's sample changed by
  1 fails `soundout_ref` (tried by hand, three kinds), so it does see
  a sample. A live headless play (`DK_KEYS`, 2,500 pictures of the
  menu, 35.7 s) was rendered twice by hand: the same file, 0 late, 0
  early, `soundout_ref` ok. The earlier tests, which run without a
  device, are as before (`opl_test.sh`, `sound_test.sh` 304 calls,
  `music_test.sh`, `sfx_test.sh`, `fsound_test.sh`: none different;
  `fight_test.sh` 39 fights, `units_test.sh` 139 calls; doskit's
  `selftest.py` ok, `check.py` all ok).
  Not checked: **nothing was listened to**, so not whether it sounds
  like the game (levels, the chip's rendering, the lead); the window's
  program was built but not run, so the real device with its own
  thread (the lock, late writes, the pace) is untried; the tune
  against the original's (the writes are the original's by
  `opl_test.sh`, their times are the port's); the Sound Blaster's
  part; Windows and Linux builds (`build.bat` got `oplq.c` unbuilt).
- The AdLib's register stream (2026-10-08, macOS, branch `mac/opl`;
  `src/sound.c`): `python3 port/tests/opl_dumps.py menu brief battle`
  runs the original with a log of every OUT to the ports 388h and 389h,
  every timer interrupt and the routines it ran, and the entry and end
  of every call of the sound's routines, and keeps the memory at the
  entry of each call with arguments and before every 4000th tick
  (`build/opl/WALK.ev`; the battle's dumps take a quarter of an hour).
  `sh port/tests/opl_test.sh` replays each list in the port
  (`port/tests/opl_test.c`): the same calls and interrupts in the same
  order on one memory from `snd_init`'s entry on, an interrupt by the
  port's `snd_irq`. Result: menu 3,303 registers the same, 0 different,
  0 missing, 0 too many, 84 calls, the driver's variables the same at 56
  dumps and the timer's counts at 55; brief 1,959 registers, 326 calls,
  30 dumps; battle (180 s of map 8 with `m8_site.keys`) 11,443
  registers, 3,128 calls, 376 dumps; every interrupt ran the routines
  the original's ran (30, 34 and 15 steps of a fade among them). Not
  judged in the driver: the interrupts' old vectors, `delay_measure`'s
  counts and the numbers `pit_add` gave (the machine's). In 1 and 17
  cases (menu, battle) a tick that found the lock set in the original
  came where the port's routine no longer held it and was made to count
  only. The script then runs the port's program through the menu's and
  the new units' walks with `-opllog` and tells how much of the
  original's stream it writes at the same tick of each tune
  (`port/tests/opl_cmp.py`; not judged, and far from all: the program's
  calls fall on other ticks than the original's, see the numbers it
  prints).
  Not checked or not reached: no sound was made or listened to;
  `snd_end`, `adl_remove` and `snd_init` called a second time (no walk
  has them); the chip's status in `adl_detect`; the battle beyond 180 s;
  the pictures of events; an interrupt that writes between a register's
  number and its value (0 in the walks); Windows and Linux builds
  (`build.bat` got `opl.c` unbuilt).
- The effects' ways and the mixer (2026-10-08, macOS, branch `mac/sfx`;
  `src/sound.c`): `python3 port/tests/sfx_dumps.py` runs the original on
  map 57 with `m57_win.keys` (a battle that ends at the first round's
  end with picture CDh, `pic_wiped`, which plays its effects by
  `sfx_play` at frames of a table) in seven variants and keeps memory
  at the entry and the end of every `sfx_play` of the picture, of the
  `snd_pass` behind each, of calls of the timer's routine `adl_tick`
  (the first twelve behind each of the first three effects, those in
  which an effect ended or began again and the one before each) and, in
  two, of the card's interrupt `dig_irq` (the first three behind each
  call, without those `dig_kick` makes inside `snd_pass`). The game's
  own tables play every effect once, so the other ways come from pokes
  (not the game's own doing): `dig` the game's own, 16 digital (8SVX)
  effects on the channels 0 to 6; `digx` with entries of the table
  changed when the first pass begins (two endless, one three times, one
  twice, one with a number beyond the file); `adl` with `S3303:0022`
  set, the AdLib's file and table, 14 effects on the channels 8 to 0Ah;
  `adlx` that with the same changes; `chan` one call's channel made
  0Ch on the stack; `voices` `adl_used` made 9 at one call's entry
  (`adl_load` finds no voice free); `lock` `adl_used` made 9 at one
  `snd_pass` (`adl_start` needs more voices than are free and leaves
  `snd_lock` set). `sh port/tests/sfx_test.sh` then compares two
  things.
  Memory: the port's routine gets the entry's memory and arguments
  (`port/tests/sound_test.c`, `irq` a new kind) and the driver's
  variables (`S30EF:0000` to `111E`) and the result are compared with
  the end's: 610 calls, none different (dig 114: 16 `sfx_play`, 16
  `snd_pass`, 36 `adl_tick`, 46 `dig_irq`; digx 116 with 48 `dig_irq`;
  adl 84 with 56 `adl_tick`; adlx 86 with 58; chan 68; voices 78; lock
  64), in 126 of them with the timer's interrupt in between as before.
  The mixer's bytes are judged now: both buffers of 400h bytes with
  their headers, `dig_mix`'s sums and count, the digital channels'
  counts of steps and of times, `dig_mask` and `dig_count` are the
  original's after every `snd_pass` and `dig_irq` compared. Among the
  calls: the two failing ones (FFFFh, nothing changed), `adl_load`'s
  FFFFh that `adl_play` takes for success, `adl_start`'s failure with
  the lock left 1 and every later call under that lock (no effect ends
  any more in the original either), effects beginning again at their
  end and stopped at the last, endless ones stopped by the next effect
  on their channel (`chan_stop` 1 for a digital one, `adl_stop` for an
  AdLib one).
  Which effect comes when: the port's program plays `m57_win.keys` with
  the variant's pokes (`dig`, `digx`, `adl`, `adlx`; the three others
  poke at one call, which the program cannot) on the original's clock
  and on its own (`-sndlog`), and `port/tests/sfx_cmp.py` compares
  every call of `sfx_play` from the first pass on: 16, 16, 14 and 14
  calls, all in the picture, the same in the picture's frame, channel,
  number, volume, count of times and result, on both clocks.
  Run again with the mixer in, after origin/master `a6c857a` was
  merged: `sound_test.sh` (304 calls, none different, the menu's 38
  effects and the battle's 1,185 against 1,186 calls as before),
  `fsound_test.sh` (m8_site 57 calls, m8_d5r 81, m8_wait_nopics 4, none
  different; the triggers as before) and `music_test.sh` (297, none
  different): `sound_test.c` no longer leaves the mixer's bytes out, so
  the 32 calls of the fight's pictures that go on to `dig_play` are now
  the original's in the buffers too. `play_test.sh` m57_win (the
  original's clock and `rounds`), m8_site and m8_site `own`: exit
  status 0, the inputs, the game's state and the computer's the same
  at every pass compared; "elsewhere" was not compared with its counts
  before the change (at m57_win's frames 101 to 121 of the picture: 826
  to 1,733 bytes on the original's clock, where the port has no card's
  interrupts, and 20 to 28 on the port's own).
  Not checked or not reached: nothing was listened to; when the
  original's card interrupt comes against `snd_clock`'s (its timing is
  the port's own); `dig_irq` with less than 28h bytes waiting or while
  it is already at work (not looked for among the 94 calls); a step
  other than 200h; an 8SVX form of the kind 3; an effect that is
  neither 8SVX nor ADLX; the pictures C8h, C9h and CCh and their
  effects (not in the port; no run reached them); `orders_test.sh`,
  `move_test.sh`, `range_test.sh`, `load_test.sh`, `menu_test.sh`,
  `loop_test.sh` and the tests with dumps in `build/n2` to `build/n6`
  and `build/computer` (not run again); the Windows and Linux builds
  (no new source file; not built).
- The fight's sound triggers (2026-10-08, macOS, branch
  `mac/fightsound`; `src/sound.c`, called from `src/main.c`'s hooks):
  `python3 port/tests/fsound_dumps.py` runs the original on map 8 with
  `m8_site.keys`, `m8_d5r.keys` and `m8_wait.keys` with the fights not
  shown as pictures, logs `pics_play` (its slot and volume), the frames
  of `fight_frames`, the channels stopped and `attacks_run`'s effect,
  and keeps memory at the entry and the end of a choice of the calls.
  `sh port/tests/fsound_test.sh` then compares two things.
  Memory: the port's routine gets the entry's memory and arguments
  (`port/tests/sound_test.c`, `pics_play` a new kind) and the driver's
  variables (`S30EF:0000` to `111E`) are compared with the end's: 142
  calls, none different (m8_site 43 of `pics_play` and 14 of
  `chan_stop` at the pictures' ends; m8_d5r 67 and 14; m8_wait without
  pictures the effect's two `sfx_play`, effect 5 on channel 8, and the
  `snd_pass` behind each). In the 32 calls that go on to `dig_play`
  (4 and 28) the original's `snd_pass` runs the mixer, which the port
  does not have: its buffers with their headers (`S30EF:0496` to
  `10B6`), `dig_mix`'s sums (`10C3` to `10C6`) and of the digital
  channels the count of steps (+10h) and how often they are still
  played (+4) are counted there and not judged, about 2,000 bytes a
  call; `sound_test.c` does this now wherever digital channels play at
  the original's end, and `sound_test.sh` (304 calls) and
  `music_test.sh` (297) are as before with it.
  Which trigger comes when: the port's program plays the same key
  script on the original's clock (`-fxlog`) and
  `port/tests/fsound_cmp.py` compares fight by fight the frame, slot,
  volume and count of times of every `pics_play` and the channels
  stopped at the end. m8_site: seven fights, 112 triggers the same in
  order (17 of slot 0, 8 of slot 1, 31 of slot 4, 56 stops). m8_d5r:
  the port's clock file reaches the first two of the original's seven
  fights, both with all 749 frames: 67 triggers the same (2 of slot 0,
  12 of slot 1, 24 of slot 2, 13 of slot 4, 16 stops). m8_wait without
  pictures: `attacks_run`'s effect twice in both. The count of times is
  the original's where the call's entry was dumped (51 of 56 and 37 of
  51).
  Not compared: in m8_site a key ends the original's pictures after
  149 to 272 frames while the port on the original's clock plays all
  749, so 120 of the port's triggers lie behind the original's last
  frame; m8_d5r's fights 3 to 7. Not reached by any run: a side without
  figures (`pics_side_sound`'s 64h, `chan_stop` with the side's
  number), the AdLib's way of `pics_play` (`S3303:0022` set), the slots
  5 and 6. `play_test.sh` m8_site, m8_d5r and m8_wait_nopics were run
  again with the triggers in: the inputs, the game's state and the
  computer's the same at every pass compared, as before. Built and run
  on macOS only; the Windows and Linux builds were not made (no new
  source file). No sound comes out, nothing was listened to.
- The music's control (2026-10-08, macOS, branch `mac/music`;
  `src/sound.c`): `python3 port/tests/music_dumps.py` stops the original
  at the entry and the end of calls and keeps its memory, in three
  walks: the menu (the keys of `menu_dumps.py`'s walk `pages`), the new
  units' screens of map 4 up to the battle's beginning
  (`briefing_dumps.py`'s `m4`) and the battle of map 8 with
  `m8_site.keys`. Kept: every call of `music_start` (3, 4 and 6) and of
  `music_fade` (4, 5 and 6), the calls of `fade_tick` (30, 34, and 40 of
  the battle's 46) and of `music_volume` (24, 29, and 40 of 45), and the
  first eight calls of `adl_tick` after each of the first three starts
  (24 in each). `sh port/tests/music_test.sh` gives the port's routine
  the entry's memory and arguments (`port/tests/sound_test.c`, which
  has the four routines as new kinds) and compares the driver's
  variables (`S30EF:0000` to `111E`) and the result with the end's: 297
  calls (85, 96, 116), none different. Among them: starts with a fade of
  4 (the menu), of 5 (the screens) and without one (the battle, from
  `game_loop` and from `attacks_run`'s end); fades of 4 and 5 steps and
  one of 20 (a taken site's picture, `S1A39`), each with the tune's
  events read all through the wait (11 waits), and four fades with no
  music on (FFFFh). The wait of a fade is the same count of the timer's
  interrupts in both: 350 for 4 steps with the menu's tune and with the
  battle's, 280 with the menu's other tune, 504 and 555 for 5 steps,
  3,116 for 20; the original's count of ticks at the routine's end is 1
  to 3 higher (those come in behind the wait, while it writes
  `opl_silence`'s registers; the test runs that many interrupts behind
  the port's call, after which all of the driver's variables agree).
  In 24 calls the timer's interrupt came in while the original's routine
  ran: there the test makes the call again with that many interrupts
  before it or behind it and says which gave the original's memory (all
  did one way or the other, or with the count of ticks alone set).
  Not judged: `S30EF:1118` (the fade's number with the timer, taken from
  the original), `music_start`'s copy of the count of ticks
  (`S30EF:0232`) by the ticks that came in, and memory outside `S30EF`,
  which is only counted (the stack and the timer's own tables in `DATA`,
  which the port does not keep).
  The calls the port's program makes (`-muslog`,
  `port/tests/music_cmp.py`): in the menu's walk the original's seven
  (fade 4 with nothing playing, then start 4 255 and fade 4 three times
  over) in the same order with the same results, their ends at most
  0.79 s apart (the first start, which the port makes 0.79 s after the
  time its clock begins with, the original's first pass of the menu; the
  others 0.04 s or less); through the new units' screens the original's
  nine, ends at most 1.74 s apart (the battle's start behind the
  screens, for which the port counts no time; the first screen's two
  calls 1.06 s, not looked at; the others within 0.21 s); in the battle on the original's clock the seven from
  `game_loop`'s start to the last `attacks_run`'s end (start 0 255, then
  fade 4 and start 0 255 three times: twice the fade before the fights,
  once the one at a site taken), in the passes 2614, 4542 and 6214 of
  the port's count where the original's clock has 2615, 4543 and 6215
  (one apart each time; not looked at). Two calls of the original there
  come from `S1A39` (a taken site's picture: its own tune started, faded
  out in 20 steps), which the port does not have; the test leaves them
  out by their return address.
  The other tests, each before the change (origin/master `64773b5` in a
  second working tree) and after it, output against output:
  `sound_test.sh` (304 calls), `fight_test.sh` and `units_test.sh` the
  same line for line. `menu_test.sh`: what came in is the same; the
  times, which were up to 0.54 s apart, are now at most 0.07 s apart in
  all seven walks (the half second was `music_fade`'s wait, which the
  port did not have); in the walk `back` the port now counts 131 passes
  in the battle before where the original has 130 (before: 140), and
  the two views' repeat counters (byte 2Ch of a view's record, counted
  down by the pass in `side_input`) are 0 where the original's are 1:
  `menu_cmp.py` now tells those two bytes apart when the counts of
  passes differ, as it does the count itself (before the change the
  bytes agreed by chance, ten passes apart). `briefing_test.sh`: the
  same verdicts; of what differs at the routine's return and not at its
  entry the driver's segment has 11 to 13 bytes instead of 78, `DATA` 0
  instead of 12 on one map and 9 instead of 1 on two (not looked at).
  `fkeys_test.sh`, `save_test.sh`, `info_test.sh`: "the setup's
  differences" 1,148 instead of 1,396 at pass 1; "elsewhere" at the
  later passes between 114 lower and 6 higher (the higher ones not
  looked at; the tune's place and due tick are candidates, the port's
  clock of ticks not being the original's). `loop_test.sh` (m8_site):
  "elsewhere" the same or up to 110 lower. `play_test.sh m8_site`, with
  `own` and with `rounds`: the game's and the computer's counts the same
  at every pass, "elsewhere" up to 100 lower and once 1 higher on the
  original's clock, 28 to 140 lower on the port's own. All with exit
  status 0 before and after (`menu_test.sh` after the change to
  `menu_cmp.py`; without it its walk `back` fails on the two counters).
  Not checked or not reached: `music_fade` with speed 0, `music_start`
  with a file that is not a tune (FFFFh) or while a fade runs, a fade
  begun while effects have voices of the tune (the fades compared
  stopped the effects first, as `music_fade` does), `music_volume`
  called from outside `fade_tick` and `music_start`, F2's two calls and
  `battle_after`'s in a run (the calls are made where the original's
  are; no key script pressed F2 or ended a battle here), a fade of
  `attacks_run` on the port's own clock against the original's times,
  `snd_clock`'s timing against the original's interrupts as before, the
  registers' values sent to `snd_opl` (only counted), `orders_test.sh`,
  `move_test.sh`, `range_test.sh`, `load_test.sh` and the tests with
  dumps in `build/n2` to `build/n6` and `build/computer` (not run), and
  the Windows and Linux builds (no new source file; not built).
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
  Not compared: the texts of F2 to F4 and F9 as video memory (F4's
  numbers among them; those of F5 to F8: "The joysticks' leftovers"
  below), the sound (F2 stops and starts the music in the original), F2 and F3
  on a machine without music or effects (10h, 20h of word 2 clear), the
  keys while 200h is set (pressed again within ten passes), a computer's
  first side (the text then goes to the second side's line), F6 pressed
  twice, a joystick that is there, F8 without a mouse. (F7 twice, F6
  twice and sticks that are there: the next point.)

- The mouse and the joysticks as a side's input (2026-10-09, macOS,
  `mac/options`): `sh port/tests/options_test.sh dumps` and
  `sh port/tests/options_test.sh`, made as the F keys' test (both
  programs through the menu by keys to map 48, two human sides, the
  port's battle on the original's clock; memory a second after each key
  and half a second after each line of the mouse's or the sticks'
  script). The runner needs doskit's `-joy`/`-joys` (branch
  `doskit/gameport`). Three runs:
  - `m48_mouse` (`tests/keys/m48_mouse.keys` and `.mice`): F7 off, the
    mouse moved while off, F7 on (the driver's reset leaves the pointer
    at 319: "right" once, in both), then the pointer put the step (40)
    right of the middle, one short of it, left, down, up, right and
    down at once, the first button, the second alone, both, the first
    let go with the second held and a move in the ten calls after it,
    F8 three times with a move after each, a move with the first button
    held, F7 off and on with a button held. 36 passes compared.
  - `m48_joy` (`.keys`, `.joy`): no stick at the start; the first stick
    plugged in and found by F5 (off) and F5 (on); its four directions,
    two at once, each button, the first twice in a row, a place just
    inside and just outside the left edge; the second stick found by F6
    twice, its directions and button, both sticks at once; a key while
    the stick rests; F5 off (the stick gives nothing), F6 off; the first
    stick taken away and F5 on; F6 on while the second stick is held
    left (its middle measured there), back, left, and taken away while
    wanted (right and down for ever). 51 passes compared.
  - `m48_joy0`: a stick there from the program's start (found by the
    setup at the start, in the first dump), used at once, F5 off and on,
    F6 with no second stick. 19 passes compared.

  In all three: every side's part read the same input (1,484, 1,484 and
  614 parts, 0 different; in the first two the mouse and the first
  stick gave each of the four directions and the button), the game's state the same at every pass
  compared, and of DATA 0380 up to 045C (the joysticks' words, both
  sides' input records with their counts, the mouse's record) no byte
  different (`tests/options_cmp.py`), with two things given:
  - the input records and their counts of holds and gaps (03BC up to
    0414) are set once, before the first pass, to what the original has
    there (`-poke`): they run on from the menu, which takes another
    time in the port (without this the gaps' counts stand 4 apart from
    the first pass on and nothing else differs; the timer routine's
    call of four is taken from the clock's log by `play.c` for the same
    reason);
  - the pointer's place in the mouse's record (0424 up to 042C) is
    compared only from the mouse script's first line on (in `m48_joy`
    and `m48_joy0` not at all), as in the F keys' test.

  Found on the way: the runner's first game port compared times as
  sums of fractions and so read an axis one count off where the
  one-shot ended exactly on a read (two of the first run's words); it
  counts instructions now. A script line at 62.00 fell into a read of
  the original and changed the stick in the middle of it (moved to
  62.10). The second stick's side read "left" once during `joy_setup`'s
  wait in the original; a setup done at once did not (1 part and 1 byte
  of state different), the one that waits does. The F keys' test
  (`fkeys_test.sh`, F5 and F6 without a stick) caught that this wait
  needs the clock's times also without a sticks' script; rerun with
  the other comparisons after the change (`check.py`, `fkeys_test.sh`,
  `play_test.sh`, `menu_test.sh`, `save_test.sh`, `loop_test.sh`,
  `info_test.sh`, `briefing_test.sh`, `fight_test.sh`, `units_test.sh`:
  as before; their dumps of the original were not made anew).
  Not compared: the window and a real stick or controller, a machine of
  another speed, `joy_read` with a
  stick whose range is 0, a stick changed in the middle of a read, the
  exact call at which the port's `joy_measure` ends against the
  original's `joy_setup` (to within a call; seen the same in the runs'
  dumps, none of which falls into a setup). (The menu with a stick, the
  texts of F5 to F8 as video memory, the port's own clock and `-map`
  with a clock's log, listed here before: the next point.)

- The joysticks' leftovers (2026-10-09, macOS, `mac/options2`): four
  things the point above had not run, each set beside the original.
  Changed in the port for them: `src/joy.c` lets the sticks' script go
  by the port's own clock in the menu also when the battle behind it is
  played on the original's (before, a script's lines and the setup's
  wait stood still until the battle's first call of the timer's
  routine), and `src/main.c` makes its `-poke`s when the first pass
  begins, behind what the clock's log has before it (they were made
  before; the same for a port that comes from its menu or plays on its
  own clock).
  - A stick in the menu (`python3 port/tests/menu_dumps.py joy57
    joyload`, `sh port/tests/menu_test.sh joy57 joyload`; the sticks'
    scripts `tests/keys/menu_joy57.joy`, `menu_joyload.joy`, taps of
    0.2 s, the menu on the port's own clock as in every walk). `joy57`:
    down, the first button on the CODE row, the code typed by the keys,
    up, the button on START. `joyload`: down four times and the button
    (MISC), down and the button (LOAD), the first button at the saved
    game's question, the second button (LOAD again), right and left at
    the question, the first button, Esc by the key, then the button
    (START), left (the other country) and the button. What came in is
    the same in both and in the same order (5 and 11 things, the times
    at most 0.05 s apart), no byte differs in the memory at the menu's
    return and at the two screens outside the loop of each walk, nor in
    the two pages of video memory at the passes kept (3 and 5) and at
    those screens. `joy57`'s Enter is at 15.1 s: at 15.0 the last pass
    of `text_input` had the cursor (the `]`) in the original and not in
    the port (504 bytes of the two pages and one of the block), which
    is the clock's and not the stick's: it comes and goes by the passes
    (the port tried with Enter from 15.00 to 15.70, the original from
    15.10 to 15.60: the same at 15.1, 15.3 and 15.6, not at 15.2, 15.4,
    15.5).
  - `-map` with a clock's log and a sticks' script (`sh
    port/tests/options_test.sh map`: `m48_joy` and `m48_joy0` again,
    the port begun with `-map 48`, compared with the same dumps of the
    original). The port then makes the log's calls of the timer's
    routine from the program's start on and the script goes by their
    times. 1,484 and 614 parts read the same input, the game's state
    the same at every pass compared (51 and 19; the setup's 41 bytes in
    `S37FF:0012` to `012D` are those of every `-map` start), and of
    DATA 0380 up to 045C no byte different but the four words of the
    pointer's range (043A up to 0442: the menu's 8 to 12Ah and 4 to
    C0h, which a port begun with `-map` never sets; told apart by
    `options_cmp.py`). The input records are given once as in the
    point above; with that poke made before the log's first calls, as
    `main.c` did, the counts of gaps differed in `m48_joy` at every
    pass (10 bytes), without it 3 bytes.
  - The port's own clock (`sh port/tests/options_test.sh dumps
    m48_joyown`, `sh port/tests/options_test.sh own`): the port from
    its menu on with nothing of the original's log but when the menu's
    first pass began, the sticks' script by its own time. The script
    (`tests/keys/m48_joyown.keys`, `.joy`) has taps of 0.056 s only, a
    little more than the 0.0549 s between two reads of the sticks: the
    first stick (there from the start) left, right, up, down, right and
    down at once, the second found by F6 twice and moved the same, both
    at once, F5 off (a tap gives nothing) and on, each side's button.
    The 18 parts that read something read the same in the same order
    in both (the original's passes 54 to 416, the port's 58 to 423),
    and at the 35 times compared (the first pass of each at or after
    half a second behind each line of the script and a second behind
    each key) the game's state is the same (`play_cmp.py` `at`: 0
    bytes, with 2 to 5 of the passes' counted apart) and DATA 0380 up
    to 045C too, but that at 5 of them the ten counts of gaps are one
    step apart (they go round 0 to 10 with every fourth call of the
    timer's routine, and the two passes do not begin at the same
    moment; told apart by `options_cmp.py`). No poke was needed.
  - The texts of F5 to F8 as video memory (`sh
    port/tests/options_test.sh dumps m48_ftext`, `sh
    port/tests/options_test.sh m48_ftext`; on the original's clock, a
    first stick there and at rest, no second): F5 twice, F6 twice, F7
    twice, F8 three times, video memory 0.3 s and a second after each.
    The original shows the texts 65h, 66h, 67h, 68h, 6Ah, 69h, 6Ch,
    6Dh, 6Bh in that order on the first side's line (the lines 189 to
    193 of the page change at each key and stay to the second dump).
    At the 22 passes with video memory no byte of the two pages differs
    (810 behind them, as in every comparison), 978 parts read the same,
    the game's state and DATA 0380 up to 045C the same at every pass.
  Run again after the changes, as before: `options_test.sh` (all
  three ways), `fkeys_test.sh`, `menu_test.sh` (15 walks), `save_test.sh`,
  `info_test.sh`, `briefing_test.sh`, `loop_test.sh` and `play_test.sh`
  with `m57_last`, `m57_win` (pokes), `m48_two`, `m8_site`, `m54_end`.
  Not compared (a stick moved in the menu before a battle played on
  the original's clock, the second button at the saved game's question
  and the mouse's button there, listed here before: the next point): a
  stick at the country's choice other than left, the name's
  and the code's `text_input` with a stick's button, the scripts with
  pokes whose dumps of the original are not on this machine
  (`m48_undo`, `m8_scroll`) after `main.c`'s change, a key or a stick
  held longer than a tap on the own clock (known to be sampled by the
  passes' length), the texts of F5 to F8 on the second side's line (a
  computer's first side), the window, a real stick; macOS only.

- The joysticks' two gaps (2026-10-09, macOS, `mac/options3`): the
  first two things of the list above, each set beside the original.
  Nothing of the port's source changed; `tests/menu_dumps.py` and
  `tests/menu_test.sh` give a walk its mouse's script too
  (`tests/keys/menu_WALK.mice`, `-mice` in both programs), and
  `tests/options_test.sh` has the tag `m48_joymenu`.
  - The buttons at the saved game's question (`python3
    port/tests/menu_dumps.py joyask mouseask`, `sh
    port/tests/menu_test.sh joyask mouseask`; the menu on the port's
    own clock as in every walk). `joyask` (two sticks there from the
    start, `tests/keys/menu_joyask.joy`): the first stick to MISC and
    LOAD, at the question its second button; LOAD again and both its
    buttons at once; LOAD again and the second stick's second button;
    Esc by the key, then the code of map 57 and START by keys.
    `mouseask` (`tests/keys/menu_mouseask.mice`): the keys to MISC and
    LOAD, at the question the mouse's first button; LOAD again and its
    second; LOAD again, the pointer moved with no button (the question
    stays), then both buttons; Esc, the code, START. In the original
    each of the six presses leaves the question, and so in the port:
    what came in is the same in both and in the same order (15 things
    each, the times at most 0.06 and 0.05 s apart), no byte differs in
    the memory at the menu's return and at the five screens outside
    the loop of each walk (the three questions, `text_input`'s
    beginning and end), nor in the two pages of video memory at the
    five passes kept of each and at those screens.
  - A stick moved in the menu before a battle on the original's clock
    (`sh port/tests/options_test.sh dumps m48_joymenu`, `sh
    port/tests/options_test.sh m48_joymenu`; `tests/keys/
    m48_joymenu.joy`, `.keys`): the stick there from the start, down
    and up in the menu before the walk's keys, held left from 20.0 to
    23.6 s (behind START into the battle, whose first pass is at
    23.13 s), then down, the button, right and up at once, F5 twice.
    614 parts read the same input (the first side's "left" in passes
    1 to 9 among them), the game's state and the computer's the same
    at each of the 13 passes compared, DATA 0380 up to 045C the same at
    every one. The input records' counts are given once as in the
    points above.
  Not compared: an input in the first 0.84 s of the menu (the port's
  first pass is that much behind its `-start`, which the tests set to
  the original's first pass; docs/HANDOFF.md, "The joysticks' two
  gaps": a stick's down at 8.6 s was taken by the original and not by
  the port, so the script has it at 9.2), a stick held across START
  itself (a stick that gives something keeps its side's keys from
  being asked, and START here is a key), the mouse's third button and
  a button held from before the question, the walks in the window
  (`menu_test.sh live`), the other tests after this branch (the port's
  source is unchanged; `menu_test.sh joyload joy57` and
  `options_test.sh m48_joy0` were run again, the same); macOS only.

- The joysticks' three remainders (2026-10-09, macOS, `mac/options4`):
  what the two points above still listed, each set beside the original.
  Nothing of the port's source changed; `tests/options_test.sh` has the
  tags `m48_mouse3`, `m48_joyptr` and (for `own`) `m48_joyhold`, and
  dumps passes 9 to 11 of `m48_joymenu` too; `tests/menu_dumps.py` and
  `tests/menu_test.sh` have the walk `mouse3`; `tests/options_cmp.py`
  `own` lists what each side read hold by hold and tells the counts
  apart as said below.
  - The mouse's third button in a battle (`sh port/tests/
    options_test.sh dumps m48_mouse3`, `sh port/tests/options_test.sh
    m48_mouse3`; `tests/keys/m48_mouse3.mice`, `.keys`; on the
    original's clock): the third alone, a move with it held, with the
    first and the first let go beside it, with the second and a move,
    all three, pressed and let go beside the first held, the first let
    go with the second and third held, held over F8. 978 parts read
    the same input, the game's state the same at each of the 32 passes
    compared, DATA 0380 up to 045C the same at every one (the mouse's
    record with the third button's word, 0446, among them; the
    pointer's place from the script's first line on). In both the
    third button changes that word and nothing else.
  - The third button at the saved game's question and in the menu
    (`python3 port/tests/menu_dumps.py mouse3`, `sh port/tests/
    menu_test.sh mouse3`; `tests/keys/menu_mouse3.mice`; the menu on
    the port's own clock as in every walk): at the question the third
    alone (it stays), the first (left); LOAD again, the third held and
    the second beside it (left at the second); in the menu the third
    with the pointer on LOAD (nothing); Esc, the code, START. 14
    things came in, the same in both and in the same order (the times
    at most 0.05 s apart), no byte differs in the memory at the menu's
    return and at the four screens outside the loop, nor in the two
    pages of video memory at the five passes kept and at those
    screens. The code's Enter is at 37.1 s: at 37.0 the original's
    last pass of `text_input` had the cursor and the port's not (504
    bytes of the two pages, one of the block), as in `joy57`.
  - The tenth pass of `m48_joymenu` (`sh port/tests/options_test.sh
    dumps m48_joymenu m48_joyptr` one tag at a time, `sh
    port/tests/options_test.sh m48_joymenu`, `... m48_joyptr`): with
    the passes 9, 10 and 11 dumped the pointer is at 319, 99 up to pass
    9 and in the middle from pass 10 on in both programs, the first
    side's five bytes "right" at pass 10: the mouse, asked for the
    first time when the stick gives nothing. `m48_joyptr` puts the
    pointer above the middle while the stick is held
    (`tests/keys/m48_joyptr.mice`): pass 10 reads "up" in both. 614 and
    250 parts read the same input, the game's state the same at each
    of the 16 and 12 passes compared, DATA 0380 up to 045C the same at
    every one.
  - Sticks held longer than a tap on the port's own clock (`sh
    port/tests/options_test.sh dumps m48_joyhold`, `sh
    port/tests/options_test.sh own m48_joyhold`; `tests/keys/
    m48_joyhold.joy`, `.keys`): both sticks there from the start; the
    first left 0.3 s, right and left four times 0.5 s, up and down 1 s,
    its button 1 s, left with the button 0.5 s; the second left 0.5 s,
    down 1 s, its button 1 s. Compared at the first pass of each at or
    after half a second behind each line that puts a stick back to
    rest and a second behind the key (13 times). The twelve holds give
    the same input words in the same order in both; nine are read by
    the same number of passes, three by one more or less (up 18 and
    19, left with the button 10 and 9, the second button 19 and 18:
    the original's and the port's). The game's state is the same at
    all 13 times (`play_cmp.py` `at`: 0 bytes, with the passes' counted
    apart) and DATA 0380 up to 045C too, with told apart by
    `options_cmp.py`: the ten counts of gaps (a step apart at 10 of
    the times, further at all 13) and a count of holds that one
    program still has from the last hold and the other has at 0 (at 2
    times, once with byte 7 of the second side's record). A hold read
    once more can move the cursor a field further (a first script did
    so for the second side; docs/HANDOFF.md, "The joysticks' three
    remainders"): the play then differs as two plays of the original
    begun 0.03 s apart would, which was not shown with the original
    alone.
  Run again on the tree with `origin/master` (`2f94d5c`) merged:
  `options_test.sh` (all three ways, every tag), `menu_test.sh` (18
  walks), `fkeys_test.sh`: as before; `check.py`. Not run again: the
  other tests. Not compared: a second press of a button on the own
  clock, a button held from before the saved game's question, the
  third button and the sticks in the window, a real mouse or stick;
  macOS only.

- The input options' last two gaps (2026-10-09, macOS, `mac/options5`):
  what the points above still listed of the F keys and the question,
  each set beside the original. Of the port's source only the tests'
  log changed: `src/menu.c` writes a line "ask N TIME" into its
  `-menulog` at each screen outside the menu's loop, and
  `tests/menu_cmp.py` prints those times beside the original's (a
  different number of screens counts as different; for dumps of the
  original made before all six addresses were logged it says that the
  times are not compared). `tests/options_test.sh` has the tag
  `m24_ftext`, `tests/menu_dumps.py` and `tests/menu_test.sh` the walk
  `heldask`.
  - The texts of F5 to F8 on the second side's line (`sh
    port/tests/options_test.sh dumps m24_ftext`, `sh
    port/tests/options_test.sh m24_ftext`; `tests/keys/m24_ftext.keys`,
    `.joy`; map 24 by its code, the computer's first side, the battle
    on the original's clock; a first stick there and at rest, no
    second): F5 twice, F6 twice, F7 twice, F8 three times, video memory
    0.3 s and a second after each. The original shows the texts 65h,
    66h, 67h, 68h, 6Ah, 69h, 6Ch, 6Dh, 6Bh in that order with DI A0h
    (the runner's log of `show_message`): the second side's line. At
    the 23 passes with video memory no byte of the two pages differs
    (810 behind them at 17 passes, 577 at 6), 960 parts read the same
    input, the game's state and the computer's the same at every pass,
    DATA 0380 up to 045C too (the input records' counts given once as
    in the points above).
  - A button held from before the saved game's question (`python3
    port/tests/menu_dumps.py heldask`, `sh port/tests/menu_test.sh
    heldask`; `tests/keys/menu_heldask.joy`, `.mice`; the menu on the
    port's own clock as in every walk): the first stick to MISC and
    LOAD, its first button on LOAD from 17.0 to 20.0 s with Esc by the
    key at 19.0, the button again at 22.0; the mouse's first button on
    LOAD from 24.0 to 27.0, its second at 29.0; Esc, the code, START.
    In the original the question is drawn at 20.15 and 27.14 s, when
    the button has been let go, and the Esc during the hold is not
    taken; in the port at 20.16 and 27.11 s. 14 things came in, the
    same in both and in the same order (the times at most 0.06 s
    apart), no byte differs in the memory at the menu's return and at
    the four screens outside the loop (the two questions,
    `text_input`'s beginning and end), nor in the two pages of video
    memory at the four passes kept and at those screens.
  Run again on the tree with `origin/master` (`5484df5`) merged:
  `menu_test.sh` (19 walks; the times of the screens printed for 12,
  not compared for `back`, `code57`, `noload`, `pages`, whose dumps of
  the original are older; `exit`, `start0`, `start24` have no such
  screen), `options_test.sh` (all three ways, every
  tag), `fkeys_test.sh`: as before. Seen in the new times: in the six
  walks with a battle between the winner's name is asked 0.91 to 0.98 s
  earlier in the port than in the original (docs/HANDOFF.md, "The
  input options' last two gaps"; not looked into). Not run again: the
  other tests, `menu_test.sh live`. Not compared: a key held from
  before the question, the second stick's button or the mouse's second
  held there, the texts with a second stick there, the window; macOS
  only.

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
    map loaded with the menu at a summer one is still open); (the
    question left by the mouse's buttons and a joystick's: "The
    joysticks' leftovers" and "The joysticks' two gaps", the walks
    `joyload`, `joyask`, `mouseask`); a file that
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
  out (`battle_hook` is told each picture; since `debian/window5` the
  pictures CDh to D0h are shown, `src/events.c`). The program's play calls it
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
- A battle ended by fights alone (2026-10-09, Windows; docs/HANDOFF.md,
  "A battle ended by fights alone"): `m60_fights.keys` (map 60, the two
  sides' one cavalry each, 1,640 s, no pokes; played with the fights
  not shown as pictures, `NOPICS=1`, as the tag `m60_fights_nopics`):
  the cavalries walk together in 78 halves and fight in the last five
  rounds; the first side's is destroyed in the fifth fight and
  `attacks_run`'s count of units left (the end of `attacks_run` in
  `src/orders.c`, `S02E6:1EFC`) ends the battle with 12h for the first
  view and 11h for the second. The script was found on the port's own
  clock (taps of 0.06 s, the gestures' direction let go 0.03 s after
  the button; why in docs/HANDOFF.md) and then logged on the original
  (`NOPICS=1 python3 port/tests/play_dumps.py m60_fights.keys 1640`,
  the runner of doskit 22292c0, the same source as 9aa2c77's). On the
  original's clock (`play_test.sh m60_fights_nopics`, merged to
  aea5a2e): 22,903 passes, 45,806 parts read the same but one, the end
  shown in both parts of pass 22903 with texts 42h and 41h by both,
  behind the loop pictures CDh with 1 and CFh with 2 by both, 178
  passes compared (1, 50, 200, 400, the 86 halves' ends and the
  passes after them, the last, the loop's exit, `S066A:4728`) and the
  end's waits and the pictures' frames: the game's state and the
  computer's the same at every one. Different: pass 1's setup bytes
  (41 in `S37FF:0012` to `012D`, as in `m0_s4`), and in pass 4748 the
  second side's `view_step` finds byte 5 and 8 of its record -1 and 0
  in the original, 0 and FFh in the port (the button tap at 329.90 s;
  the same sort as `m0_s4`'s pass 1059, not followed).
  `play_dumps.py` now lists dumps of passes from 10000 on in
  `passes.txt` (before, they were made but not compared).
  Not reached: 15h, both sides without units; a fight shown as
  pictures that ends a battle. Not checked: the port's own clock
  against the original beyond what the script needed (the steps were
  the same at half 7 and at the end's texts).
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
  counting only; not translated). (It was translated on `debian/window`
  and is compared since `mac/thinkbar`: "The bar of the computer's
  thinking" below.)
- The computer as the first side (2026-10-09, Windows; branch
  `win/computer`): `tests/keys/m24_quiet.keys` and `m30_quiet.keys`
  play the maps 24 and 30, where side 0 is the computer's, with a human
  second side that only ends its halves. `tests/loop_dumps.py` now takes
  `mNN_` scripts as `tests/play_dumps.py` does (the map's code typed in
  the menu). 400 s of each, `tests/loop_test.sh $(cat
  ../build/loop/TAG/loop_test.args)`: the game's state and the
  computer's the same at every pass compared, 33 passes of 6,119 on map
  24 (14 with `attacks_run`) and 15 of 5,065 on map 30 (5). Rerun the
  same way: `m8_site` and `m0_carry`, the same at every pass.
  `tests/play_test.sh m24_quiet`: the inputs, the game's state and the
  computer's the same at every pass compared; `m30_quiet` did not run:
  on the original's clock the port's program comes to no pass (`-until
  57` to `200` all stopped by a timeout of 40 s, nothing written), while on
  its own clock it plays (1,382 passes in 60 s). Not looked into: it
  is `play.c`'s (map 30's first pass begins at 56.6 s, map 24's at 31 s;
  the script's keys begin at 70 s, after it).
  `tests/loop_test.c` was changed for it: it sets `game_mem` from
  `game_loop`'s frame in the first dump and gives the game's folder
  (`../game`, `$DOSKIT_GAME`), without which `pictures_load` after a
  fight failed and left the music's digit (`S331F:0284`) unwritten; on
  map 24 the digit changed and the test said DIFFERENT. The fight's
  pictures now run in the test and draw from `rand_words`, which the
  original's runs did not do: words 4 and 6 are given back at their end
  (docs/HANDOFF.md). Not run with `-replay`.
- Both sides the computer's (2026-10-09, Windows; branch
  `win/computer2`): `tests/loop_dumps.py` with a script named
  `mNN_both` sets it in the menu's GAME SETUP (player two to COMPUTER,
  the fights' pictures OFF) before START; `tests/keys/m24_both.keys`
  (no key) and `m30_both.keys` (two keys through map 30's NEW UNITS
  pages). 600 s each, `tests/loop_test.sh $(cat
  ../build/loop/TAG/loop_test.args)`: the game's state and the
  computer's the same at every pass compared, 22 passes of 7,760 on
  map 24 (the ends of 9 of its 10 rounds and the pass after each) and
  10 of 8,704 on map 30 (all 3 round ends). The port needed no change.
  Rerun after merging master (972da43): `m24_quiet`, `m30_quiet`,
  `m8_site`, `m0_carry` the same. Built with the cc wrapper for MSVC
  under Git Bash (docs/HANDOFF.md). Not covered: `play_test.sh` with
  both sides the computer's, the fights' pictures on, maps below 18h.
- The queue and run phases in the pass (2026-10-09, Windows; branch
  `win/computer3`): `STEPS=6 python3 tests/loop_dumps.py m24_both.keys
  600` (and `m30_both`) logs in a second run where `computer_turn` gets
  back from `computer_queue` (`S1C74:010F`) and `computer_step`
  (`014E`) and dumps six passes in which a step was queued (the queue's
  1; its run begins in the same pass), six in which the list ran out
  (the step's not 0, back to the queue), and the pass after each.
  Map 24: 163 and 161 such passes in 600 s, map 30: 108 and 108.
  `loop_test` compared 46 passes on map 24 and 34 on map 30, each
  played by the port from the first pass: the game's state and the
  computer's (with it `computer_steps`, `computer_script` and the
  phases) 0 bytes different at every one. The port needed no change.
  Rerun after merging master (bd2232d). Not covered: the script play
  (phase 1) on its own (it runs in the same passes but was not chosen
  for), `play_test.sh`, the fights' pictures on, other maps; which
  side queued in a chosen pass was not recorded.
- The script play (phase 1) in the pass (2026-10-09, Windows; branch
  `win/computer4`): `SCRIPT=3 python3 tests/loop_dumps.py m24_both.keys
  600` (and `m30_both`) logs in another run (`script.txt`) the code
  `computer_input` reads (`S1E4B:0061`, AL) and whether `computer_step`
  ran in the side's part (`S1C74:014E`), and dumps three passes for
  each code a side only played (no step in its part), each with the
  pass after. Played alone: map 24 the codes 1 (153 passes), 2 (161),
  80h (274), 82h (55), 83h (138); map 30 1 (154), 2 (165), 80h (192),
  82h (12), 83h (116). `loop_test` now also sets the computer's input
  word (`computer_out`) after every pass beside the word the
  original's pass took (DI at `S066A:1243`, `inputs.txt`): 0 of 15,516
  side-passes different on map 24 and 0 of 17,404 on map 30; the game's
  state and the computer's 0 bytes different at all 75 and 58 passes
  compared (with the earlier STEPS and fights' passes). The port needed
  no change. Not covered: the codes 3 to 9, 81h and 84h played alone
  (3 to 5 never alone, presumably only in passes with a step, not
  checked; 81h, 84h and the diagonals
  never, in these two runs: the words seen are 1, 2, 4, 8, 10h, 11h,
  14h, 18h); `play_test.sh`, the fights' pictures on, other maps. The
  word check counts a pass where the original did not reach `S066A:1243`
  as word 0 (not told apart). Built with the cc wrapper (it does not
  fail on a compile error: a stale `build/loop_test.exe` then runs, so
  it is removed before building).
- The paths no battle reached (2026-10-09, Windows; branch
  `win/computer5`): `python3 tests/computer_dumps.py late second_p22
  second_p0 second_p11` plays three quiet battles changed by a poke
  (`POKED` there says which bytes and why) and dumps the calls;
  `sh tests/computer_test.sh -f ../build/computer/late_test.args`: 9
  calls, 0 different. `plan_approach` in `computer_queue`'s state 5 (6
  calls, its states 0 to 2, two units, map 22 with byte 4 of kind 12h's
  `kind_worth` 2); `computer_plan`'s state 6 (2 calls, `plan_ride` in
  its states 0 and 5, map 0 with 4 in word 6 of a unit of no side);
  `computer_assess`'s state 0 with `plan_to_site` (1 call, map 11 at
  918 s, 4 in word 6 of the unit in the computer's carrier 41h and that
  carrier's plan 0; the carrier got the plan 0Ah, worth 190h). The
  repair step (kind 6) in the passes: `tests/keys/m26_quiet.keys` (map
  26, the computer the first side, as `m30_quiet`), `PASSES="4004 4010
  4020 4027 4040" STEPS=3 python3 tests/loop_dumps.py m26_quiet.keys
  400`, then `tests/loop_test.sh`: the game's state and the computer's
  the same at all 36 passes compared, 0 of 5,343 input words
  different; the 17 calls of `step_site_menu` fall in the passes 4004
  to 4026. Not reachable, so not compared: `computer_plan`'s state 2,
  the step of kind 5 and `step_site_menu`'s state 6
  (docs/HANDOFF.md). The port needed no change. Not covered: these
  paths in an unpoked battle (none gives them), which branch of
  `plan_approach`'s state 2 the calls took (not looked at), `plan_ride`'s states 1 to 4 for a unit to
  fetch, `play_test.sh` on `m26_quiet`; whether the repair's text was
  shown the same was not looked at beyond the bytes.
- The colours back after a round end without pictures (2026-10-09,
  Windows; branch `win/blackmap`): `state_done` (loop.c) remembers the
  fade it made (grey or dark) and, when no fight's pictures were shown
  (`pics_shown`, set in `fight_pictures`), after the redraw makes the
  counterpart (`fade_back`, `fade_in`) that order 4's own presentation
  would have made (not translated). Checked with temporary DAC logging:
  `m0_carry -nopics` (4th round end), `m8_site`, `m0_hq` (pictures on)
  dark before, the game's colours after. The pictures-shown path is
  unchanged: base and fixed headless builds give the same `-rounds`,
  RAM and VRAM at passes 3000 and 6000 on `m8_site`, `m0_carry`,
  `m0_hq`; `loop_test.sh` on `m8_site`, `m0_carry`, `m24_both`,
  `m30_both` the same; `play_test.sh` `m57_site`, `m8_long` the same,
  `m60_fights_nopics`, `m0_s4` DIFFERENT as on origin/master. Not
  covered: the dark fade with no pictures (never met), a live run (the
  player checks), the site taken's own picture (still not shown).
- The site taken's presentation (2026-10-09, Windows; branch
  `win/sitepic`): order 4 (`order_take`, `S02E6:11B1` to `1373`) now
  draws what the original draws: the other side's view on the site
  (`view_to_field`, `view_draw`, the site's field marked with 30h while
  `marks_draw` draws), the taker's view covered (`view_cover`,
  `S131D:1426`, in draw.c since `debian/draw-tidy`), `page_flip`,
  `page_copy`, `wait_frames(1Eh)`, `fade_in` after a fight in this
  `attacks_run` else `fade_back` (with the fights' pictures off
  `fade_back` only without a fight), `take_picture` 1, `event_picture`
  CAh or CBh at the work buffer plus 1064h and `libs_start` again.
  `event_picture` CAh/CBh is `site_picture` (`S1A39:1906`, events.c
  `pic_site`): three files (the table at `S3812:086C`), the third
  started as music, the first stream's frames over the taker's half at
  x 0 or A0h, 6 timer counts each, `wait_frames(C8h)`,
  `music_fade(14h)`. The colours that `state_done` brought back for it
  since `win/blackmap` (`pics_shown`) are gone again: order 4 does it.
  The port's own clock (play.c) now waits where order 4 waits
  (`ORDERS_WAIT`, `ORDERS_FADE`, `ORDERS_TAKEN_BACK`; the picture's
  frames, waits and `inputs_release` through `site_hook`), and the
  site's files take 0.333 s (`EV_LOAD_SITE`, measured in the
  original's log: `S1A39:1906` to `195B`) instead of the 1.84 s that
  stood for the frames. Compared on `m0_s4` (two depots taken, rounds
  4 and 7): `play_dumps.py` dumps the original at every tenth frame of
  the sites' pictures (`S1A39:198C`, `sNNNN`) and `-evdump` the port
  there; frames 1, 11, 21 and 31 (CBh over the right half for the
  computer's site in round 4, CAh over the left for the first side's
  in round 7) the same in both pages of
  video memory on both clocks (1442 bytes behind the pages differ, the
  same at every frame). At those frames `S331F:1838` (`fight_rec` + 1Ch)
  is FFFFh in the port and 0 in the original; the port has FFFFh there
  from the round end's start, before `attacks_run`, and 0 again at the
  next pass: not caused by this change, not looked into. Round end with
  the site taken, `attacks_run` to the round's end: original 11.42 and
  11.39 s, port 11.35 and 11.34 (11.34 before). `play_test.sh m0_s4`
  DIFFERENT as before (the one input part of pass 1059, on master too;
  the differences "elsewhere" after the site taken down from 204 to
  123); `rounds` the same up to round 7 as before; `m57_site` both
  clocks and `loop_test.sh` (`m8_site`, a site taken in pass 6215) the
  same. `orders_test.sh` and `loop_test.sh` link events.c now;
  `orders_test.sh` and `fight_test.sh` were not run (their dumps,
  `build/n2`, are not on this machine). Not checked: a live run in the
  window (the player checks), a site taken after a fight (`fade_in`
  path) or with the fights' pictures off, the site's music as sound.
- The bar of the computer's thinking (2026-10-09, Mac; branch
- The site taken's dark fade (2026-10-09, Windows; branch
  `win/darkfade`): order 4 after a fight with the fights' pictures on
  brings the colours back with `fade_in` (S02E6:1318) instead of
  `fade_back`; `order_take` had it, no run had reached it. Reached in
  the original with both sides the computer's on map 30 and
  `no_fight_pics` poked to 0 at `attacks_run`'s 7th call (a fight,
  then a site taken in the same unit's turn, 1654.3 s). `orders_test`
  now takes `orders_fought` ([BP-0Eh], was a static of orders.c) and
  `game_mem`'s libraries, save buffer and work buffer from
  attacks_run's frame, with `-vin IN` loads the original's video
  memory and DAC at the loop's entry and with `-vram PREFIX` writes
  the port's at order 4's `wait_frames(1Eh)` and behind its fade.
  Compared with the original's (dosrun -vram/-dac at S02E6:12F0 and
  1343): the same in all 256 KB and 768 bytes at both places, for the
  dark path (m30) and the grey path (m24_both with pictures, the
  `fade_back` DAC differed before only because the test had no
  `game_mem`). The game's state behind the loop differs in 4 bytes on
  both paths (`lib_pal`+10h/11h, `lib_rnd`+10h/11h, from `libs_start`;
  not looked into). Times: after a fight the original draws the view
  in 0.073 s; play.c's clock now counts `T_TAKE_VIEW` (0.085 s, from
  the fight's end) there instead of nothing; `fade_in` 0.5207 s
  against 0.5223. Not checked: a full play of the port reaching it
  (own clock, play_test), the pictures-off case after a fight in video
  memory, a live run.
- The site taken's 4 bytes and the random stream (2026-10-10, Windows;
  branch `win/librnd`): the bytes were the segments of the library
  lists' far pointers in `lib_pal` and `lib_rnd`, wrong only in
  `orders_test`, which had attacks_run's [BP+6] (work buffer) and
  [BP+16h] (the libraries' place) the wrong way round; fixed there.
  `order_take` drew the site's picture at the work buffer plus 1064h,
  the original at the libraries' place plus 1064h; fixed (orders.c).
  orders_test also compares `rand_words` now: 0 bytes different in the
  game's state on the dark and the grey path, video memory and DAC
  still the original's at both places. loop_test lists the passes in
  which a site was taken and takes up to 30,000 passes; on
  `m30_both_take` (m30_both.keys to 1500 s, pictures off; dumps in
  build/loop, the key file not committed) the site taken in pass 19869
  happens in the port too and the game's state, the computer's and
  `rand_words` are the same at every dumped pass up to 20463, the
  computer's input word in all 40,924 side-passes. Not checked: a take
  with the fights' pictures on in loop_test, the passes between the
  dumps, a live run.
  `mac/thinkbar`): `computer_progress` (`S1C74:0354`, `src/plan.c`) and
  the bar `computer_part` draws with it (`src/loop.c`, `bar_draw` in
  `src/draw.c`) were there since "The last drawing of a battle", where
  `bar_draw` alone was compared with the value from the original's
  stack. Now set beside the original where `game_loop` calls them:
  `tests/thinkbar_dumps.py` makes the dumps (`build/thinkbar/`, 52 runs)
  and `thinkbar_test.args`, and `sh tests/load_test.sh $(cat
  ../build/thinkbar/thinkbar_test.args)` compares two modes new in
  `tests/load_test.c`: `progress`, the routine from its entry to its
  RETF (`S1C74:0519`) with AX, and `think`, `game_loop` from the call of
  `computer_progress` (`S066A:11C1`) to behind `bar_draw`
  (`S066A:1212`) with the video memory and the DAC, the two words
  `game_loop` keeps in its frame written as the original does. 13 calls
  in each mode: `m8_d5r` 1, 86, 188 and 487 and `m0_hq` 1, 64, 329 and
  768 (the computer the second side, the bar at x B7h; values 0, 36h,
  45h, 2Bh and 0, 17h, 55h, 4Bh), `m24_quiet` 1, 31, 279, 611 and 751
  (the computer first, x 17h; 0, 5, 5Dh, 5Dh, 4Bh). 26 calls, 0
  different: the same value, no byte of memory different but the stack
  below the call (35 to 67 bytes) and at most 5 of the timer's
  (`beside`), no byte of video memory (the original changes 202 at each
  call) and none of the DAC. `computer_progress` changed no byte, the
  `think` part 4 to 7. Nothing in `src/` changed. Not verified: the line
  in `loop.c` itself (the test has a copy of the call, since
  `computer_part` is static and `loop.c` was not to be touched); the
  window, by eye; `attack_pick` 2 and the two ways out of the range 0 to
  63h, which no call reached; other dumps of `load_test.sh` were not
  rerun (`build/n2/` has none on this machine).
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

2026-10-09 (Windows, branch `win/fademusic`): the music goes on through
the screen's waits in the window. The live `screen_wait` (`src/play.c`)
shows a fade's steps and a retrace wait's pictures at real time while
the port's clock stands still; it now calls `snd_clock` after each
picture, which with the window's audio device runs the timer's sound
interrupts (`adl_tick`, `fade_tick`, the card's) that the samples
played have made due, as the original's timer interrupts its fades.
Before, none ran during the wait and they came all at once after it,
so the tune slowed in a fade and hurried behind it (the player's
report, compared with DOSBox by ear). Only the sound's routines run
there, never the game's (no `tick`, keys or mouse). Checked: the build,
`oplwav_test.sh` and `soundout_test.sh` as expected (the screen hook is
set by `play_live` only, so the headless runs these compare are not
touched); `opl_test.sh` reports `WALK.out` different and its dumps
missing in this worktree, the same without the change. Not checked:
the window by ear (left to the player); timing measured live.

2026-10-09 (Windows, branch `win/fightpanel`): a fight's panel builds
its figures up step by step at the original's pace. `fight_panel`
(`src/draw.c`) sends its waits to `orders_hook` where the original
waits (`ORDERS_FADE` after `fade_in` and `fade_out`, `ORDERS_MARK` and
`ORDERS_WAIT` 7 after each step's page, `ORDERS_WAIT` 13 after the
steps, `ORDERS_INPUT` 96h or 46h), and `round_hook` (`src/play.c`)
counts only the time before the panel at `ORDERS_FIGHT`; before, it
counted all the panel's waits there, so the window drew the six steps
at once after them. `orders.c` sends the `wait_frames(1Eh)` after the
panels of a fight without pictures (`S02E6:0AE6`) where it falls.
Checked: m8_d5r's first fight with pictures, the original dumped by
the runner at fight_panel's entry, its RETF and each step's
`wait_frames(6)` (`S131D:1C9A`, hits 1 to 6); `load_test.sh fpanel`
with `LOAD_TEST_STEPS` (`tests/orders_hook.c`) writes the port's video
memory at each step: all six the same as the original's, and the call
the same as before (video memory 0 different). The original's times
there: `fade_in` over 0.624 s after the entry, a step each 0.1141 s
(8 retraces), the 0Ch wait 1.298 s, `wait_input` 1.484 s; the port's
own clock gives a step `T_MARK` and 7 retraces, as it counted them
before. `loop_test.sh` the same; computer, move, side and view test
programs build. Not checked: the steps without pictures against the
original (the same code, its six steps back), the port's own-clock
times at the steps (no original run of m8_d5r's log on this machine
to compare `rounds` with), `orders_test.sh` and `fight_test.sh`
(their dumps are not here), the live window (the player checks).

2026-10-09 (Windows, branch `win/papers`): the newspapers. After a
battle on a map below 30h against the computer the port shows the
map's newspaper as the original does (`src/events.c`: `event_picture`'s
kinds 0 to 2Fh, HL.EXE's `S1A39:059D` twice, two pages): the three
fonts, the background, palette, tune, text and pictures of a page
loaded as the original loads them (`font_load`, `paper_text`,
`lbm_show`, `file_load_size`, `lib_load`), the text set a line at a
time into a buffer of four planes, two pixels a byte, and scrolled up
through a window a row a frame of 8 timer counts, with the pictures
the text names rising from below (HL.EXE's `S2D47`, `paper_*`, and
`S2F11`'s two drawing routines, `glyph_put` and `plane_put`), the
tune started and faded. The three pictures some pages begin with are
translated too (`pic_ship` CCh, `pic_zeppelin` C9h, `pic_richt` C8h:
sprites by `sprite_draw` over streams, their sounds). The port's own
clock counts the original's times measured in `m0_paper`: a page's
loading 1.01 s (`EV_LOAD_PAPER`), its end after `music_fade(14h)` 2.11 s
(`EV_PAPER_END`); the frames count the timer's calls. A frame of
these has no line in the original's log (`EV_PAPER`): on the original's
clock the port runs them without waiting. `-evdump PREFIX` writes
memory and video memory at the frames 1, 11, 21 ... of the newspapers
and their pictures (`PREFIXnNNNN`); `play_dumps.py` dumps the
original's there (`S1A39:08F9`, and `169A`, `1BED`, `1D0F`, `1FCF`,
`221B` for the pictures).
Checked: `tests/keys/m0_paper.keys` (map 0, the battle left at once by
Esc and `z`): the original's run and the port's
(`build/history-line-headless -map 0 -keys tests/keys/m0_paper.keys
-start 36.496061 -seed 0x28B -until 270 -evdump DIR/q`, the start and
seed from the original's `clock.txt`) both show 1,827 frames; at all
183 dumped both pages of video memory are the same, as are the window's
record (`S3812:0D14`) and the fonts (`S3B76`); behind the pages
(offsets 7E84h on) the original has what it put there before the
battle's first pass, the port nothing. The BIOS's ticks at those
frames: the port 6 to 12 ticks early throughout (12 at the first; the
difference lies before `event_picture`). The three pictures the same
way, each with the newspaper around it: `tests/keys/m31_paper.keys`
(C9h), `m22_paper.keys` (C8h), `m34_paper.keys` (CCh, before page 21):
all 126, 132 and 148 dumped frames have both pages the same. Their
loading counts CCh 0.66 s (measured), C9h 1.04 s and C8h 0.55 s
(estimated, docs/HANDOFF.md); with them the port stays 4 to 16 ticks
early at all dumped frames of the four runs. `check.py` all ok; the
window and headless programs build (MSVC), `orders_test` builds with
the cc wrapper.
Not checked: the music by ear; the pause key; the original's clock
(`-clock`) through a newspaper; the live window (the player checks).

2026-10-09 (win/launcher): the setup screen. In the window, before the
game, `main.c` (`setup_run`) shows doskit's launcher (`launcher.h`,
doskit/docs/LAUNCHER.md) with the port's pages only: the menu ("Start
the game", then the pages Game, Sound, Display; Quit is the kit's), Game
(Intro and Outro: Play or Skip, the launcher's INTRO.EXE and EXTRO.EXE;
`-nointro` still skips the intro), Sound (Volume 0 to 10, Headphone mix)
and Display (Full screen). The values are kept in `history-line.cfg` in
the data folder. Volume and headphone go through `audiofx.h` at the end
of `audio_fill`, and in play the keypad's + and - change the volume, *
mutes, shown with `hud.h` (doskit/docs/PLAYER-SETTINGS.md); `hud.c` is
now built. The original has no setup program or settings file of its
own (HL14-18.EXE takes no options, the sound is the AdLib only), so
there is nothing of it to compare; its side and difficulty are chosen
in the game's own menu and are not offered here (they live in the
program's memory). Headless and scripted runs never show the setup
screen and leave the sound as before (`fx_on` 0). Built with MSVC (both
programs), the window started; `check.py` all ok. Not checked: the
screen by eye, the settings file's round trip, the volume keys, the
headphone mix and full screen in the window (the player checks), and
whether the game itself reads the keypad's + - * (frame.c keeps them
from it once the hud is set). Not offered: the player's keys and the
controller's buttons (the game's own keys are not yet written down).

2026-10-09 (win/titlesetup): the title screens, a flatter setup, the
player's keys and a controller.
- HL.EXE's own logo screens at its start (the end of S1650:0003,
  `start_setup`): BB.DAT and BB.IFF, the publisher's animated logo
  (S28A5:000C, `title_show`: 43h frames by a table in HL.EXE's image,
  read from there), then HLLOGO.IFF faded in; each waits at most 1F4h of
  the timer's counts for a key, a stick's button or a click
  (S0CB6:18F1, `wait_span`), then fades out. `src/title.c`, called from
  `start_setup` through the hook `start_title` (the standalone tests that
  link battle.c need none of it). Shown in the window when the setup's
  "Intro and title screen" is Play (the same choice as the intro, which
  comes before it, as HL14-18.EXE runs INTRO.EXE before HL.EXE), and
  headless with `-title`; never in the comparisons. The port did not
  show them before (the "bytes behind the pages" above are theirs).
  Compared on the runner (`run.py -log S0CB6:18F1 -log fade_out`): the
  original's two waits last 6.877 and 6.881 s without a key, the port's
  first 6.870 s (headless, `-title`, a temporary log); Space at 4.0 s
  ends the original's first wait at 4.013 s, the port's at 4.009 s; in
  both a key during the animation is lost (the wait clears the count
  first). Not compared: the pictures byte for byte (no video memory
  dump of the logo was taken), the port's second wait (only start to
  end), and the window by eye.
- The setup's menu: Full screen is now a choice in the menu itself (one
  setting is all that group had; doskit/docs/LAUNCHER.md); the Display
  page is gone. The Game page's first item is "Intro and title screen"
  (the settings file's name stays `intro`).
- Keyboard page: the game's keys, read from HL.EXE (comment in
  `main.c`): the first side's arrows and Space (the keypad's 7 9 1 3 and
  Enter stay as they are), the second side's D, C, X, V and Ctrl (F and
  Alt stay), and F1 (confirm the mode change), F2 (music), F3 (sounds),
  F9 (fights' pictures; their meaning is the manual's, page 51). An
  `LI_KEY` each (`key_...`), the game's key the default; the player's key
  goes to the game as the game's through `frame_set_keymap`, the game's
  key keeps working. Controller page: an `LI_CHOICE` per button
  (`pad_<button>`) of nothing, those actions (as the player's key), Esc,
  Enter, Space, Y, N; the defaults D-pad = the first side's arrows, A
  fire, X F1, B and Back Esc, Start Enter; set by `pad_set_keys` when the
  game is started. All of it in `main.c` with the kit's parts; nothing
  of input.c, joy.c or menu.c changed. Not checked: any of it in the
  window (the player checks: the pages, a remapped key and a button in
  play, the settings file's round trip), and whether a key mapped onto
  an E0 key (the arrows) reaches the game's table of held keys as the
  game's own arrow does (frame.c writes the E0 byte; the game's INT 9
  translation passes it to `key_irq`, which takes `code & 7Fh`).

2026-10-09 (win/titlefix): two faults of the title screens the player
found.
- The publisher's logo: its sparkle was drawn shifted and left a trail
  of itself. Two things of the original's were missing. S28A5:000C
  (`title_show`) sets the screen's cut (DATA:00E8) to FFFFh at its
  beginning and end; without it the frames above the top (y FFFDh - 1)
  went the uncut way and were drawn from a wrapped place. And BB.DAT's
  pictures are of kind 'U', whose blitter S2AAF:0000 keeps what it
  covers (`under_save` of the cut rectangle, in the uncut way and in
  the cut way at S2AAF:0279) for `unders_restore`; the port's `pic_blit_u`
  takes no save place, so nothing was put back. `src/title.c` now sets
  the cut and does that save itself (`title_save`, the cut of
  S2AAF:0107) before each frame. Compared: the original under the
  runner (`run.py -until 1.8 -shotevery 0.1 HL.EXE`) against the port
  headless (`-title`, DK_SHOTS every second picture from 30 to 130):
  the original's shots at 1.2, 1.4, 1.5, 1.6 and 1.7 s (the sparkle at
  the top right corner and the logo after it) each have a
  pixel-identical picture among the port's, and the trail is gone. The
  original's shots at 0.7 to 1.1 and 1.3 s (the sparkle on its way
  along the edges) fell between the port's sampled pictures and were
  not matched; not compared by eye in the window.
- A key that ended a wait (Space or Enter) started a map: not the
  game's doing but the port's clock. The window's keys are kept as a
  key script by their times (`live_key`), and `menu_game` began the
  menu's clock again at `-start` (0), so every key of the intro's and
  the title's time came a second time, to the menu, Space and Enter as
  its fire. After the title screens the menu now goes on on their
  clock (`play_start(play_now())`, as the extro already did), so those
  keys are passed over. In the original the key is eaten by the wait
  itself and by `inputs_release(1, 0)` before the menu (S066A:0349),
  which the port does too. Checked headless (`-title -menulog`, DK_KEYS):
  Space at both waits, Enter at both, Space during HLLOGO's fade-in and
  then at its wait: no act of the menu, it stays at the main menu;
  before the fix the first two went into a battle. A Space in the menu
  after the title is still the menu's (act 1). With `-title` and a
  `-start` in a scripted run the menu now begins where the title ended,
  not at `-start`.

2026-10-10 (`debian/border`): the cursor's border inside a site or
carrier on a place with no function. The port draws what the original
draws: the border (cursor picture 5) on every place, empty or not, and
on a gesture with nothing on offer; no change. `m8_border.keys`
(`make_m8.py e45 g55 s64 d66 d67 d68 d69 d70 d71 u73 u74 r76 o79 L82
l85`: the factory entered, its seven empty places walked, button and
down, button and up on an empty place, leave), `play_dumps.py`
`VRAM=1`, `PASSES="560 580 590 610 640 680 720 768 772 780 816 822 830
870 900"`: `play_test.sh m8_border` the inputs, the game's state and the
computer's the same at every pass compared, both pages of video memory
the same at all of them. Not checked: the same by the mouse, a carrier
in this script, the window by eye.

## Bugs of the original the port copies (win/origreview, 2026-10-10)

A review of HL.EXE found bugs the port reproduces on purpose, since it
follows the original: a battle won before the first round's end files
its best score under map 4 and does not advance the map (`battle.c`
`battle_score`). The intro's lost Esc and the two below are the ones
the port does not copy. The ranked list with repros and fix proposals
is in docs/HANDOFF.md.

## Deliberate deviations from the original (win/origfixes, 2026-10-10)

Two bugs of the original are fixed; each is marked "deviation
(win/origfixes)" in the code.

- A damaged saved game. The original's menu only asks whether the file
  can be opened, and its `load_game` (S1273:0399) tests the header's 0
  and 7 in its buffer, not in what was read: an empty, cut or wrong file
  starts a battle of the menu's map mixed with what the file held, with
  no word. The port's menu (`menu.c` `saved_game`) asks `save_valid`
  (`save.c`) first: a file that is not exactly as long as `save_game`
  writes it (1Bh bytes of header and the 24 blocks, 35,411 bytes) or
  does not begin with 0 and 7 is refused with SAVED GAME NOT FOUND, the
  menu's text for a missing file, and the menu stays. A whole file is
  loaded as before.
- The best score's name. `text_input` writes its cursor `]` into the
  name behind the letters, and the original copies the five bytes as
  they are, so Enter while the cursor shows kept a shorter name with a
  `]` (walk `name57`: QWEU]). The port's `winner_name` (`menu.c`) copies
  the name up to its first byte that is not a capital and zeros behind
  it (the manual's names are letters). A name of five letters is the
  same as before (walk `name5x`). `text_input` itself is unchanged.

Checked (Windows, MSVC headless):
- `save_test.sh`: m48_save, m48_save2 (and the window's play), m48_load
  as before: the files byte for byte and every pass compared the same.
  `load_bad`, `load_cut`, `load_head`, `load_none` (wrong second byte,
  cut at 7000h, ten bytes, empty) now must leave the port in the menu
  with no battle begun; all four do. The original's dumps of them are
  kept, no longer compared.
- `menu_test.sh name57 name5x twice57` and `live livename57
  livetwice57`: `menu_cmp.py` compares a best scores' file with the
  deviation applied to the original's (a name the walk wrote ended at
  its first non-capital). name57 (QWEU]), twice57 (NEW], in this run's
  dumps), livename57, livetwice57: the port's file is the original's
  with the one `]` taken off; name5x (QWERT): the same file unchanged.
  Memory and video memory at every compared point: no byte differs.
  The `live` part's window-against-`-keys` memory compares (port.ram,
  port_e*, port_f*) say DIFFERENT on this machine with and without the
  change (checked on master's code for livename57): not this change's.
  The other menu walks were not run (no dumps on this machine; they do
  not reach the name or a saved game that exists).

Not checked: the refusal's screen against a picture (only that no
battle begins; the text is the menu's own path for a missing file); a
file of the right length with damaged blocks (it is loaded, as the
original would); the window by hand (the player checks).

2026-10-10 (win/volsave): the keypad's + - * in play are kept. Until now
`launcher_save` ran only when the setup screen closed, so a volume or
mute changed in play was lost with the process. `setup_run` now keeps the
path of `history-line.cfg` in a static and `hud_control` writes the
settings after each change, so the setup screen and the keys agree after
a restart (mute was saved too at first; withdrawn, see below).
Built with MSVC, `check.py` all ok. Not checked: the round trip in the
window (the player checks); no test, since the hud's keys only act in the
window and the tests run headless.

2026-10-10 (win/volfix): corrected. The mute item is gone again;
volume 0 is silent and mute is not a setting. Only + and - write
`history-line.cfg`; * mutes until the process ends, as doskit's player
settings prescribe. Built with MSVC, `check.py` all ok; the window by hand
not checked.
