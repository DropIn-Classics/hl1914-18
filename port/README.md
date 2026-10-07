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

## Build and run

    sh port/build.sh          # macOS, Linux (SDL2 for the window)
    port\build.bat            # Windows (MSVC)
    port/build/history-line -game game
    port/build/history-line-headless -game game -map 5 -ram build/map5.ram

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
