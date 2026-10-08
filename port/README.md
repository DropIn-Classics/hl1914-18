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
Left out are the screen shown before a battle, the libraries of pictures,
the loading of a saved game, what draws the views and the music. The
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

## Build and run

    sh port/build.sh          # macOS, Linux (SDL2 for the window)
    port\build.bat            # Windows (MSVC)
    port/build/history-line -game game
    port/build/history-line-headless -game game -map 5 -ram build/map5.ram
    port/build/history-line-headless -game game -map 8 -keys port/tests/keys/m8_site.keys -start 31.1 -seed 563 -passes 2000 -ram build/played.ram

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
  Not reached: a fight between two human sides, a site, building,
  repairing, the function keys, a battle's end, other maps for two
  players.
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
  pass. Still not reached: `order_undo`, order 1, and the units aboard a
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
