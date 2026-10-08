# Handoff

Last full rebuild check (2026-10-08): all four programs rebuilt
identically (`HL.EXE` 71,762 instructions with the seven of the
interrupt 24h handler `crit_error`, INTRO 18,055, EXTRO 15,678,
the launcher 1,663). Everything
that runs of the game executed is decoded, including whole battles on all
72 maps, saving and loading. Stage 2 is under way in `HL.EXE`: the saved
game's layout, the unit records and the tables beside them, how `map_load`
builds a battle's state, the two maps, and the routine that takes a fight's
losses off (`fight_losses`) are named and described below under "`HL.EXE`
stage 1" (the bullets dated 2026-10-04); the fight's arithmetic is read and
agrees with 39 fights of six runs. The port has its first translated
routines (`port/src/fight.c`: `fight_percent`, `fight_losses`), the same as
the original on those 39 fights, and since then the taking off and making
of units and sites and the building of a battle's state from a map's files
(`port/src/units.c`, `port/src/map.c`). The port's program now loads `HL.EXE`
into its memory and sets a battle up by itself as far as that is translated
(`port/src/battle.c`; the bullets "The port's own start" and "The rest of a
battle's setup" below: the setup is translated up to the loop's beginning,
but for pictures and sound), and plays the battle on from there without
drawing it, with a key script as the human's input (the bullet "The
port's program plays a battle by itself"). Of the loop, the views' states, a round's end
and what a unit reaches are read (the last bullets under "The loop of
`game_loop`, second reading"), and the routines that mark where a unit can
move, whom it can attack and its way are translated (`port/src/range.c`),
as is the carrying out of a move, field by field (`port/src/move.c`).
The computer-turn helpers and income routine have now been read and named
in `src/HL.hints`, but are not translated or fully checked; see the
2026-10-07 notes below. `master` is one local commit ahead of
`origin/master`, with additional uncommitted changes in `src/HL.hints` and
`docs/HANDOFF.md`.

The `doskit` submodule is now at `2f6fe29` (fast-forwarded from `a4b8166`,
eight commits). That update adds first support for LE images (`disasm.py`,
`build.py`; this game has none), an assembler option
`regreg_form=rm_reg` that puts the destination of a register ALU/MOV
instruction in r/m (see the docstring of `disasm.py`; not tried on this
project's `raw` lines), 32-bit offsets in the runner's
named addresses, and in the runner fixed US country information, an empty
DBCS table and unused interrupt vectors an extender can find. Nothing of the
port's template changed. Checked on macOS: `doskit/tests/selftest.py` passed
(`selftest ok`), `check.py` says `all ok` (the four programs rebuild
identically with the same numbers as before), `sh port/build.sh` built, and
`tools/test_file_formats.py`, `port/tests/fight_test.sh` (39 fights, 0
different) and `port/tests/units_test.sh` (24 calls, 0 different) passed.

The update before it, to `a4b8166` (from `f4dc33c`), added consent-based package self-updates to the runtime
(`update.c`: on Windows and Linux the setup screen's U downloads, verifies
and installs a newer release; on macOS it opens the release page), a
`pad_in_use()` query, GOG Linux shell installations in `goglist.py`, and a
`disasm.py` check that refuses a name given at two addresses. The template's
changes to `port/src/main.c` and `port/dist/README.txt` were applied to this
port; `port/src/main.c` again differs from the template only in the filled-in
placeholders. Checked on macOS, with `game/` unpacked by `inno.py -x game`
from GOG's Windows installer (521 files): `doskit/tests/selftest.py` passed
(`selftest ok`), `check.py` says `all ok` (all four programs rebuild
identically, EXTRO's carried block is up to date), `sh port/build.sh` built
both programs without a warning, and `tools/test_file_formats.py` passed
its nine synthetic tests. Not checked: the self-update was not run, and the
Windows and Linux builds of the port were not made.

The earlier update to `f4dc33c` (from `95ba89e`) added `run.py -base LIN` for translating loader image addresses;
`doskit/tests/selftest.py` passed (`selftest ok`), as did this project's
`check.py` (`all ok`) and the nine synthetic tests in
`tools/test_file_formats.py`. Its earlier `xfer.py` retains a source
comment only when the hint directly below it is carried, preventing
game-specific source comments from leaking into a sibling program's carried
block. `doskit/tests/selftest.py` and this project's `check.py` passed after
the update. The runner's tested `-criterr AH ERR` option invokes a guest's
INT 24 handler for a matching INT 21 call; an `AL=3` (Fail) return becomes
`ERR` in AX with CF set. `-requirements` prints a compact end-of-run report
of the devices and VGA memory interface used, plus unknown ports and
unsupported CPU, BIOS and DOS requests; its own `REQ.EXE` test exercises
each reported category.

## Start here (next session)

- AGENTS.md and PROVENANCE.md have the rules: read them first.
- `game/` holds the game's unpacked files (`doskit/tools/isox.py` for a
  CD image); it is not in the repository.

## The game's files

| Folder / file | What |
|---|---|
| `HL.EXE` | Main game program. |
| `HL14-18.EXE` | Launcher program; the port's game-file marker. |
| `INTRO/INTRO.EXE` | Intro program. |
| `EXTRO/EXTRO.EXE` | Outro program. |
| `MAP/` | Map data (`.COM`, `.FIN`, `.SHP`); the `.COM` files are data, not DOS programs. |
| `LIB/`, `TXT/`, `FX/`, `SEQ/` | Game data, text, effects and sequence data. |
| `INTRO/`, `EXTRO/`, `ZWI/` | Intro, outro and intermediate-scene data, alongside their programs. |
| Top level (`.LBM`, `.IFF`, `.PAL`, `.DAT`, `.LIB`, `.ADL`, `.SFX`) | Shared pictures, palettes, records, graphics libraries and audio data. |

The headless runner starts `HL.EXE` for two emulated seconds without an
emulation error (2026-10-02).

## `HL.EXE` stage 1

- MZ program, entry `0000:0000`, stack `3E2E:0080`, with 146 relocation
  target frames and 5,837 relocations. Its entry explicitly loads DS with
  `3CFF`, which is the `DATA` segment.
- The program has a Borland TLINK 5.0 header and no tail after its MZ
  image. `linker tlink 50 header=original` and `relocorder original` are
  required.
- `src/HL.hints` rebuilds the program `IDENTICAL` (51,809 instructions,
  4,748 labels and 70 `raw` instructions from `build.py` output).
- A five-second deterministic run reached seven initial gaps; recursive
  descent from their recorded first instructions exposed two more reached
  gaps in the same coverage. The nine verified `code` entries rebuild
  identically as 53,218 instructions and 4,938 labels; `build.py` required
  nine additional `raw` encodings (79 total).
- Several compiler segments begin within their relocation frame. Four
  far calls cross a segment boundary; the preceding segment's `size=` and
  the following segment's `start=` preserve each relocation's original
  source frame.
- A ten-second no-input runner coverage run reached the former CODE gap at
  `CODE:114F` (71 instruction starts; first at that address). A breakpoint
  there stopped at `t=0.000231`, with `CS:IP=0076:114F`; it is recorded as
  a verified `code` entry.
- A 30-second no-input coverage run also entered the relocation-frame
  55 former-DATA relocation-frame segments. Their first observed instruction
  starts are recorded as `code` entries; the segments remain `DATA`, because
  changing their linker class moves them before DATA and changes the MZ
  image. The largest reached groups were `S167A` (2,121 instruction starts),
  `S30EF` (988), `S2F39` (612), `S27FE` (411), `S066A` (247), `S1650`
  (216), `S1571` (211) and `S2B06` (206). Three newly reached instructions
  require `raw` hints because their branch targets cannot be represented by
  the current assembler (`S0E56:0006`, `S2947:0002`, `S2BD4:0008`).
- With those entries recorded, a fresh ten-second no-input coverage run has
  34 CODE gaps (5,810 of 11,872 bytes) and reaches none of them. The former
  `114F-126C` gap is reduced to `123C-126C` (237 bytes decoded from the new
  entry). `CODE:205C` is a DOS AH=09 text string and `CODE:2ACE` is an
  argument, not code-entry evidence. The word at `DATA:12CA` was initially
  dismissed as an unrelocated value; a later cross-reference below shows it
  is an indirect near-code target.
- Four 20-second timed menu-key sequences (Escape/Enter, Enter/Space,
  Down/Enter and function keys) likewise reached none of those gaps. They
  changed the run state and hashes, but each coverage file still reports the
  same 34 gaps and 5,810 bytes; further menu-key variants are not a useful
  source of new code entries.
- A 120-second run with staged Enter, Space, direction and Escape inputs
  reached the map screen (confirmed by screenshots), but its coverage still
  entered none of the then-unclassified CODE gaps. Static tracing did find a
  computed parser dispatch at `CODE:0A15`: after limiting the index to
  `0..17h`, it jumps through 24 near offsets at `CODE:0DC8`. All 24 table
  values point into the parser's otherwise unclassified handler region. The
  `words CODE:0DC8 18 CODE` hint seeds those entries; the identical rebuild
  now decodes 53,795 instructions and 5,072 labels (82 `raw` instructions).
  At that stage `gaps.py` reported 31 gaps and 4,877 bytes, down from 34 and
  5,810.
- `CODE:1B32` loads ES from `CODE:0284` (the DATA segment) and jumps indirectly
  through `ES:12CA`; the original initialized word at `DATA:12CA` is `123C`,
  which lies in the remaining CODE gap. `words DATA:12CA 1 CODE` records this
  target. The build remains IDENTICAL and `gaps.py` now reports 32 gaps / 4,844
  bytes: 33 bytes at `CODE:123C` decode before the next unknown area. This
  instruction was not reached in the 120-second coverage run, so its dynamic
  value and reachability remain unverified.
- The adjacent words at `DATA:12CC`, `12CE` and `12D0` each contain `1241`.
  A temporary `words DATA:12CC 3 CODE` hint rebuilds IDENTICAL and decodes
  `CODE:1241` as `MOV DX,1283H`, which rejoins at the already decoded
  `CODE:1244`. The current hints therefore record all four near-code targets
  at `DATA:12CA..12D0`. These indirect paths were not observed in a runner
  session. With the three additional table entries, `gaps.py` reports 32 gaps
  / 4,836 bytes.
- `CODE:031D` and `CODE:0334..0336` call through far pointers at `DATA:0FD4`,
  `0FD8` and `0FDC`; all three initially target `CODE:02D1`. `CODE:2A70`
  writes `2E2E` to the offset half of the first slot, so that slot is mutable.
  `words DATA:0FD4 3 CODE stride=4` plus `code CODE:02D1` rebuilds IDENTICAL
  and decodes the target. `gaps.py` now reports 32 gaps / 4,828 bytes. The
  indirect calls were not reached in a runner session; the first slot's
  runtime target may differ from its initialized value.
- The gap xref at `S29C1:010E` resolves to a near pointer to `CODE:0379`.
  `words S29C1:010E 1 CODE` rebuilds IDENTICAL and decodes 21 bytes through
  `CODE:038E`, where it rejoins known code. `gaps.py` now reports 32 gaps /
  4,807 bytes. This pointer path was not observed in a runner session.
- `S066A:0D15` writes `07AB` to `[BX+6A]` and `CODE` to the adjacent
  `[BX+6C]`; nearby fields use the same segment:offset layout. The temporary
  `ptr S066A:0D15 CODE` hint rebuilt IDENTICAL and decoded 24 bytes from
  `CODE:07AB`, moving that gap start to `07C3`. `CODE:2A70` also writes
  `2E2E` and `CODE` to the offset and segment halves of the far-call pointer
  at `DATA:0FD4/0FD6`. With `code CODE:2E2E`, the combined hints rebuild
  IDENTICAL (57,458 instructions, 5,407 labels, 83 raw encodings) and decode
  through `CODE:2E49`; the next gap is `2E4A-2E60` (22 bytes). `gaps.py`
  reports 32 gaps / 4,755 bytes. The instruction at `CODE:2E48` jumps to
  `2E65`, beyond CODE's declared extent `2E60`; `raw CODE:2E48` preserves the
  instruction without classifying that target. These dynamically selected
  paths have not been reached in a runner session.
- Temporary entries at `CODE:03D9`, `03E0`, `03E8` and `03F0` rebuild
  IDENTICAL and reduce the gap report to 28 / 4,792 bytes. Each begins
  `POP CX; PUSH CS; PUSH CX`; its following body selects CX=0, 1, 2 or 3
  before joining at `CODE:03F6`. Existing callers target the adjacent entries
  `03DC`, `03E3` and `03EB`; no incoming call or pointer to the four gap
  starts was found. Keep these as unverified candidates, not code hints.
- Further word-xref checks did not establish targets at `CODE:04A8`, `050C`,
  `057F` or `05E2`. `S2BAD:008F` is the `A8 04` opcode/immediate of `TEST
  AL,4`; `S10E1:13B1` contains the displacement/immediate bytes for
  `MOV BYTE PTR ES:[BX+0CH],5`. The other `050C` and `057F` occurrences are
  within routine bodies, not a pointer table. `S2D47:072C`, one source for
  `05E2`, lies in a DB-encoded stream and remains ambiguous. No hints were
  added from these numeric matches.
- A further static pass found 12 bounded indexed indirect-jump tables in
  relocation-frame segments (`S02E6`, `S0CB6`, `S10E1`, `S1CC6`, `S1E4B`,
  `S2138` and `S2D47`). Their bounds and `JMP WORD PTR CS:[BX+table]` forms
  identify 92 near-code targets. Ten tables are now recorded with their
  targets: `S0CB6:0437`, `S02E6:206C`, `S10E1:0F78`, `S10E1:17CF`,
  `S1CC6:0A23`, `S1E4B:0381`, and `S2D47:0CBC`, `0CA6`, `0CEC`, `0CC4`.
  Each was added with an identical rebuild. The current build decodes 57,418
  instructions and 5,400 labels (82 `raw` instructions); `gaps.py` reports
  32 CODE gaps / 4,844 bytes. Two tables remain from this set. The six-entry
  table at `S1CC6:1845` changed the image size by 16 bytes and was removed.
  The 13-entry `S2138:2CDD` table also changed the image size by 16 bytes,
  even with `num` hints preserving its four direct `DS:0006` operands (below
  DATA's `start=8`); it was removed pending segment-boundary analysis. The
  separate dispatch at `S1A39:02B3` needs investigation: seeding its target
  `S1A39:01CE` produced an assembler phase error.
- Input that reaches gameplay (2026-10-03, macOS): Enter at 3 and 6 s gives
  the main menu by 8 s; further Enters (9, 12, 16 s) choose START and the
  country, and the map is shown by 20 s. The menus follow the mouse
  (`-mouse X,Y,B`, X 0..319 and Y 0..199 as on the screen): the main menu's
  rows are at Y 48, 73, 97, 122 and 147, EXIT at about 290,185. GAME SETUP's
  default is player one HUMAN, player two COMPUTER; CONTROLS shows
  `MOUSE SIDE A`. On the map, random mouse clicks, drags and both buttons
  drive player one far deeper into the game than keys alone: the game
  reaches `WAITING FOR ATTACK-MODE` and other screens.
- `gaps.py` lists gaps of CODE-class segments only, and nearly all of this
  program's code is in the DATA-class relocation frames. Executed code the
  hints miss was therefore found by comparing the `-cover` files directly
  with the decoded instruction starts of every segment (a scratch script
  over `disasm.generate`, in ignored `build/hl-cov/`; a `gaps.py` option for
  this belongs in doskit). 18 coverage runs were joined: idle 60 s, the Enter
  sequence, four 300-second runs with pseudo-random keys on the map, four
  300-second runs with pseudo-random mouse and keys on the map, and eight
  runs of up to 180 s that click a main-menu row and then at random. They
  executed 34,602 distinct instruction starts. Each first start outside
  decoded code is now a `code` entry (added in rounds, since decoding one
  entry exposes the next); all 34,602 are decoded.
- A `words` hint whose target segment is DATA class makes labels but does
  not seed code. The targets of the recorded jump tables that no run reached
  are therefore `code` entries of their own (nine in `S1A39`, three in
  `S2138`).
- The two tables that changed the image size by 16 bytes end in the next
  relocation frame: `S1CC6:1845` (six words, bound `CMP BX,5`) by one byte
  and `S2138:2CDD` (13 words, bound `CMP BX,0CH`) by seven. `size=1851` /
  `start=1` and `size=2CF7` / `start=7` keep them with their readers, and
  both are recorded. `S1A39:02B3` is a nine-word table (bound `CMP BX,8`),
  recorded likewise; the former phase error does not occur any more.
- State: IDENTICAL with 68,740 instructions, 6,581 labels, 345 `code`
  entries and 198 `raw` instructions. 98 of the `raw` lines are in `S2138`,
  the first of them a direct operand below DATA's `start=8`
  (`MOV WORD PTR [6],1`); presumably the others are alike (not checked one by
  one), and a `num` hint or the right `ASSUME` would replace them. Others are
  jumps into the preceding frame and `PUSH` immediates in the long form.
- Of the 249,888 bytes before DATA, 64,336 are not decoded as instructions.
  41,192 of those are 23 frames with no instruction at all (the largest
  `S331F` 19,952, `S3812` 6,720, `S3B76` 6,272, `S39B6` 4,880, `S3AEE`
  1,776), presumably data; that leaves about 23,000 bytes in frames with
  code, the largest `CODE` 4,749, `S30EF` 4,686, `S1E4B` 2,812, `S2138`
  1,903, `S2F39` 1,472, `S202F` 1,315, `S2D47` 1,137 and `S1CC6` 1,089.
  `gaps.py` still reports 32 CODE gaps (4,749 bytes); no run reached one.
- The work above was merged with four commits made on another machine in
  the meantime (the `S29C1:010E` pointer, the `S066A:0D15` and `CODE:2E2E`
  entries; their paragraphs are in this section with the numbers of their
  own line of work). The merged hints rebuild IDENTICAL with 68,762
  instructions, 6,586 labels and 198 instructions as DB.
- Random input is used up: four further 600-second runs with other seeds
  raised the executed starts to 36,038 and reached nothing undecoded.
- How the game is driven, found by runs with screenshots (2026-10-03, macOS;
  scratch scripts in ignored `build/n2/`):
  - Main menu, CODE row (150,73): a click, five letter keys and Enter set
    the map; START (150,48) then goes to that map without the country
    choice. The 72 codes are the first five bytes of each ten-byte record of
    the unpacked `CODES.DAT`, a letter being the byte minus 11h plus `A`
    (the first record gives the code the menu shows at the start).
  - GAME SETUP: its third row (150,97) switches player two between COMPUTER
    and HUMAN (seen on the screenshot); EXIT (290,185) returns.
  - On the map F1 shows `F1 : MODE CHANGE` and a second F1 confirms it (Enter
    or Space cancels): both sides get `NEW MODE`, the computer shows
    `THINKING TIME` and moves its units. A single F1 changes nothing, which
    is why the earlier random runs stayed in round 0.
  - F2 music, F3 effects, F4 `HQ FREQUENCY`, F5 and F6 joystick 1 and 2, F9
    the fight setting; F7, F8 and F10 showed no message on the map.
  - Esc asks `QUIT PROGRAM ? (Y)`; the key that confirms is the one at
    scancode 2Ch (the runner's `z`: Y on a German keyboard), not `y`. The
    battle then ends in the newspaper sequence, which scrolls by itself and
    which Enter does not skip.
- 144 further runs of 300 s, one for each code with player two computer and
  one with player two human, pseudo-random mouse and keys (without the F1
  pair), and the quit run executed 43,859 distinct starts with the earlier
  ones' 36,038. The 790 outside decoded code are covered by 40 new `code`
  entries (five more `raw`); none lay on bytes a run changed (two `-ram`
  dumps compared). `HL.EXE` rebuilds IDENTICAL with 69,628 instructions,
  6,674 labels and 203 instructions as DB; 62,129 of the 249,888 bytes
  before DATA are not decoded.
- 215 further runs, one per code each: 600 s with the F1 pair at random
  times (player two computer), 400 s with it and rarer keys (function keys,
  digits, letters; player two human), and 400 s with those and one quit
  (Esc, scancode 2Ch) into the newspaper. Their screenshots show rounds
  going on, the computer's moves, the fight pictures and unit information.
  With all earlier coverage 56,852 distinct starts ran; those outside decoded
  code are covered by 39 more `code` entries and 41 more `raw`. `HL.EXE`
  rebuilds IDENTICAL with 70,890 instructions, 6,780 labels and 244
  instructions as DB; 58,670 of the 249,888 bytes before DATA are not
  decoded (the five frames without any instruction make up 39,600 of them:
  `S331F`, `S3812`, `S3B76`, `S39B6`, `S3AEE`).
- A runner fault, for doskit: one of those runs (`build/n2/e7.keys` and
  `.mice`, between 200 and 300 s) never ends. The host's stack shows
  `dos_int21` in `mcb_free`: `mcb_coalesce` walks the chain of memory blocks
  until a `Z` block and loops for ever when a block's signature is neither
  `M` nor `Z`. So the guest overwrote a block header (whether by a fault of
  the game under random input or of the emulation is not checked). The
  runner should stop with a message there, as DOS halts with a memory
  allocation error; not changed here (a kit change with its test).
- MISC, LOAD asks for a saved game's number 0-9 and says `SAVED GAME NOT
  FOUND` for 1; HISCORE shows four `EMPTY` lines for the map. No run wrote
  a file into its state folder, so nothing was saved. Keys are compared
  with a table of scancodes at `S331F:0292` (pointer at `S331F:02D3`); read
  from a run's memory it holds, besides Esc, Space, the arrows and F1-F10,
  the letters S, E, N, D, X, C, V, F and the key at 2Ch. No decoded
  instruction reads the S or E entry through that pointer; which of them
  saves, and where, is not known. The routine at `S10AF:000F` shows a text
  of `GAME.TXT` by its number in one side's status line (its callers pass
  3Eh for the quit question, 52h for the F1 line, 64h for the thinking
  time, as seen on the screen).
- Segment bounds (2026-10-04). Most modules of `HL.EXE` begin inside their
  frame, as in the launcher: a routine's tail lay in the next frame as DB
  (`file_close` ended in `S2B87`, the create path of `file_open` in
  `S2C95`), and 60 of the observed `code` entries were such tails at offsets
  0 to 5 of a frame. Following METHOD.md, each segment whose lowest
  far-referenced offset (far CALL/JMP or relocated far pointer in the
  generated source) is 1 to 15 now has that as `start=`, and the segment
  before it the matching `size=`: 93 segments, among them `CODE size=2E6F`
  (which is why `CODE:2E48`'s jump to `2E65` needed a `raw`). Segments that
  already had a `start=` were left as they were. The 60 entries below a
  start were removed, the `raw` lines made anew from `build.py`'s output
  (196 instead of 244). `HL.EXE` rebuilds IDENTICAL with 70,990
  instructions and 6,751 labels, and every start the coverage files hold
  (57,838) is decoded. Not checked one by one: that each `start=` is the
  module's true first byte; a far-referenced offset is an upper bound, and a
  module whose first routine is only called near begins lower (its bytes
  are then decoded as the tail of the segment before, with the same bytes).
- Stage 2 has begun with seven names in `src/HL.hints`, each with a comment
  saying what it rests on: `file_open`, `file_close`, `file_size`,
  `lib_read`, `file_load`, `mem_alloc` (one module each; their INT 21h calls
  were seen in a `-dos` trace up to the map, the arguments are read from
  the code) and `show_message`.
- Saving and loading (2026-10-04). The save key is D (entry 1Ah of the key
  table), compared at `S066A:3FA7` in `game_loop` (`S066A:000D`); that
  branch is reached only while the other side's state byte is 5 and the key
  is not F1, which is the state a left click on the map brings up (the
  `F1 : MODE CHANGE` line). In a run a click, D and `3` showed `SAVE STATE
  OF PLAY`, `CHOOSE POSITION 0-9`, and wrote `03.DAT` (the layer's root);
  a second run on that state folder with MISC, LOAD and `3` continued the
  battle with the same line shown. Both runs executed nothing outside
  decoded code. Named: `game_loop`, `save_game` (`S1273:0005`), `load_game`
  (`S1273:0399`, called at the start of `game_loop` when bit 200h of
  `S331F:4CA4` is set; that it reads the file is read from the code). The
  save file's layout is not looked at yet.
- 30 runs of 1800 s with a mode change every few seconds reached rounds 4
  and 5 only (the computer's thinking takes most of the time) and the
  overview map; their 222 starts outside decoded code are entries now.
  `HL.EXE` rebuilds IDENTICAL with 71,250 instructions, 6,773 labels and
  196 instructions as DB; 59,448 distinct starts ran in the runs of
  `build/n2/`. dosrun takes at most 4,096 key and 4,096 mouse events
  (`MAXEV`), which bounds how long a scripted run can be busy.
- Files opened from the start to the first map, in order (`-dos` trace, all
  through `file_open`): `LIB\NEWCHAR6.LIB` and `.DAT`, `BB.DAT`, `BB.IFF`,
  `HLLOGO.IFF` (3.5 s), then at 6.7 s `CODES.DAT`, `MNUTIT.IFF`,
  `MNUSCORE.IFF`, `MNUFAHN.IFF`, `MNUCHR18/14/8.LIB`, `MNUFAHN.LIB`,
  `MNUICON.LIB`, `MNUMISC.LIB`, `MNU.ADL`, `MNUHI.ADL`, `MNU.AFX`,
  `MNUDATA.DAT`; at START `MAP\00.HI`, `S.LIB`, `S.DAT`, `MAP\00.FIN`,
  `MAP\STATMAP.FIN`, `GAME.TXT`, `AMOK.DAT`, `UNIT.DAT`, `WW1GNDS.DAT`,
  `MAP\00.SHP`, `MAP\00.COM`, `NEWUNIT.DAT`, a `.LIB` and `.DAT` whose name
  has only the extension (presumably a prefix that is empty for this map;
  not checked), `GAME.IFF`, `00.PAL`, `LIB\BIGUNIT.DAT` and `.LIB`,
  `LIB\UMAPDATA.DAT`, `LIB\UMAPDASA.LIB`, `ONGAME2.ADL`. `file_write`
  (`S2BC5:0008`) is named as well.
- Jump tables, second pass (2026-10-04), read from the generated source:
  20 more bounded tables of compiled switches (`CMP BX,n`, `JBE`/`JA`,
  `SHL BX,1`, `JMP WORD PTR CS:[BX+table]`) in `S1A39`, `S1CC6`, `S1E4B`,
  `S202F`, `S2138` and `S2407`; the targets of a switch by value at
  `S2D47:0CDA` (four values at `0CD2` before them); and two 16-entry
  dispatches on a byte's upper four bits at `S30EF:173F` and `S30EF:190D`.
  Nearly all their targets were decoded already; six were not and are
  `code` entries. `HL.EXE` rebuilds IDENTICAL with 71,755 instructions,
  6,814 labels and 197 instructions as DB. INTRO and EXTRO had no table
  hints at all; the same forms give nine tables each (the parser dispatch
  through `CODE:0D0E`, a switch by seven values, the two 16-entry
  dispatches, five in the module that is `S2D47` in `HL.EXE`). INTRO
  rebuilds IDENTICAL with 18,055 instructions, EXTRO with 15,678 (four of
  its tables carried by `xfer.py`, five in its own part). No table was seen
  taken entry by entry in a run.
- The saved game's layout (2026-10-04), read from `save_game` and the
  routine at `S1273:0662` that lists what it writes: a header of 1Bh bytes
  (0, 7, the bytes at `S331F:1892` and `S331F:189B`, six bytes from
  `S37FF:0129` which in the file a run wrote are the map's code and a zero,
  `S37FF:012F`, the word at `S331F:0008`, then of each side's record at
  `S331F:4D33` and `S331F:4D4A` its word 0 and bytes 2, 3, 4, 15h, 16h),
  then 24 blocks copied from memory, not packed. The block lengths add up
  to exactly the 35,411 bytes of that file. `tools/savfiles.py` splits a
  file by this layout and writes it back identically (checked on that one
  file; a synthetic test is in `tools/test_file_formats.py`), and its
  `BLOCKS` table is the list: two blocks of 1806h bytes behind the far
  pointers at `S331F:17B6` and `17B2`, a block of F2h bytes behind each
  side record's pointer, 82h bytes behind `S331F:1893` and `1897`, 320h
  bytes of a buffer, 8Fh bytes of state from `S331F:4CA4`, a 31h-byte
  record for each side (`S331F:4D92`, `4DC3`), and tables at `S331F:3B5A`,
  `3A0E`, `38F6`, `38BE`, `3116`, `189C`, `09BD`, `S39B6:0132`, `09AB`,
  `12FD`, `126B`, `1224` and `S331F:02B5`. This list is the game's own
  statement of its state and the place to start naming records. The table
  at `S331F:09BD` is 33h records of 45h bytes (see the records below), 3,519 bytes: the size of the
  top-level `UNIT.DAT`, which `datfiles.py` rejects as records of 68. A
  `-ram` dump after 300 s of play confirms it: 3,385 of the unpacked file's
  3,519 bytes are at `S331F:09BD` (the game changes some while it runs),
  and the unpacked `WW1GNDS.DAT` (1,050 bytes) and `AMOK.DAT` (36 bytes)
  lie unchanged at `S331F:05A3` and `S331F:4C7A`; the three are named
  `unit_dat`, `ww1gnds_dat` and `amok_dat`. `NEWUNIT.DAT` (9,807 bytes
  unpacked) was not found in that dump as a whole. What the other blocks
  hold is not known.
- The two blocks of 1806h bytes are the map. The unpacked `MAP\00.FIN` is
  four header bytes (two big-endian words, 16 and 24 for map 00;
  presumably width and height) and two bytes a field, 768 bytes; in the
  file saved at the start of map 00 both blocks begin with those 768
  bytes, 741 of them equal. 1806h is room for 3,072 fields and six bytes.
  The 27 bytes that differ are all second bytes of a field, the same in
  both copies: where the file has another value than FFh (28 of its 384
  fields), memory has a number from 0 to 27. Presumably the second byte
  is what stands on the field, a kind in the file and an index in memory;
  that the tables of 1Ch (28) records in the list above belong to it is a
  guess from the number alone. Beyond the first 768 bytes the two copies
  differ (the rest of the 1806h bytes is not empty; not looked at). In map 00
  the file's 28 values are even (20 to 44) in the upper part of the map and
  odd (21 to 45) in the lower, the two sides' ends: presumably the value is
  a unit's kind times two plus its side (not checked against `unit_dat`).
  The first records of the 1Ch-record tables, taken by the index, showed no
  plain relation to kind or position. Pieces of `00.SHP` and `00.COM` were not found in any block
  as they are in the files.
- The saved game's records (2026-10-04, later the same day; from the one
  file saved at the start of map 00, `tools/savfiles.py --list`). Where
  `save_blocks` multiplies two numbers, the variable is the record's length
  and the constant the count, not the other way round as first written
  down: the blocks are periodic that way and not the other.
  - `S331F:189C`: the units, F1h (241) records of 1Ah bytes, 30 in use;
    an unused one is zero but for 80h in byte 5. The map's 28 fields with
    a unit hold the record's number in their second byte, and the words at
    0Bh and 0Dh of the record are the offset of that byte in the map block
    (the map's fields begin at the block's offset 0). Byte 8 is the map
    file's value halved (the kind), byte 4 is 4 for the 14 units of the
    upper end, 5 for the 14 of the lower: the file's value is kind times
    two plus side, as guessed. Two more records (byte 4 is 6) point to a
    field's first byte, where the file has 15. Bytes 0, 2, 5 to 7 and 9
    are the same for units of a kind in this file; what they mean is not
    known.
  - `S331F:3116`, `3A0E`, `38F6`, `38BE`: 70, 10, 10 and 2 records of 1Ch
    bytes of one layout: 14 bytes (FFh, or in `38F6` a few unit numbers),
    four words that are map offsets, five bytes, and a unit's number or
    FFh. The three records in use at `3116` name the three units of kind
    22, and byte 0Ah of those units is the record's number. The two used
    records of `38F6` have the two byte-4-is-6 units' offsets and three
    fields beside each. Presumably these are what carries or holds units
    (not checked by a run).
  - `S331F:09BD` (`unit_dat`): 33h (51) records of 45h (69) bytes. The file
    differs from the unpacked `UNIT.DAT` only in bytes 0Fh, 3Dh and 3Eh of
    the records.
  - `S39B6:0132`, `S39B6:09AB`: 241 records of 9 bytes, one a unit; in the
    first the 14 records of side two's units differ from the rest.
    `S39B6:12FD`, `126B`, `1224` and `S331F:02B5` are two records each
    (0Ch, 49h, 20h and 2 bytes), presumably one a side.
  - `S331F:4C9E`: three words, the map's length in bytes (300h here), its
    width and its height (16 and 24). Over the map's 768 bytes the two map blocks are the
    same in this file; after them the first has bytes left from elsewhere
    and the second only the values 6, 8, 9, 10 and 11 (not known).
- Names from the unit records (2026-10-04): `unit_count`, `unit_flags`,
  `unit_kind`, `unit_pos` (the fields at 2, 4, 8 and 0Bh, as the code
  addresses them: record number times 1Ah plus the field's address),
  `carriers` (`S331F:3116`), `map_ptr`, `map2_ptr`, `map_bytes`. Read in the
  code, not seen in a run: `unit_flags` 8000h marks an unused record
  (`S15C5:0594` writes it, nine places test it); the 14 bytes at the start
  of a carrier's record are two rows of seven unit numbers, and
  `S02E6:2556` walks them, subtracts a value from each unit's `unit_count`
  and, where the count is not above it, sets 4 in the unit's byte 14h and
  empties the place. Presumably that is damage to what a carrier holds.
  Much of the code reaches the records through a far pointer (`[BX+4]`),
  so the names do not show every use. Next there: the routines in `S02E6`,
  `S1CC6` and `S0E56` that loop over the table.
- How a battle's state is built (2026-10-04, read in `map_load`,
  `S0E56:0329`, and the routines it calls; not stepped in a run, but the
  saved file agrees). `map_load` marks all records unused, loads the map
  file, sets `map_width`, `map_height`, `map_bytes` and `map_ptr`, and walks
  the fields. A field's first byte indexes `ww1gnds_dat` (6-byte records):
  bits 2 and 1 of the record's word 0 give a side, and 400h, 100h or 40h
  add a record to the table at `S331F:3A0E`, `38F6` or `38BE`
  (`site_add_400`, `site_add_100`, `site_add_40`; what the three are in the
  game is not known, byte 18h of their record starts as 0Fh, 0Ah and 14h).
  A field's second byte below F4h is a unit: `unit_add(number, kind, map
  offset, flag bits)` copies bytes 0, 3, 4, 3Dh and the words 0Dh and 11h
  of the kind's record in `unit_dat` into the unit (so `unit_count` starts
  as the kind's byte 3 and `unit_flags` as its word 0Dh), and a kind with
  1000h in that word gets a record of `carriers` (`carrier_add`). A kind
  with 40h and 80h there makes two units, the second one row down with
  the next kind. `map_load` also sets bytes 3Dh and 3Eh of every kind to 1
  (the bytes in which the saved table differs from `UNIT.DAT`) and counts
  in the side records (17h bytes each at `S331F:4D33`, bytes 2, 3 and 4:
  units and the first two kinds of table record; they are the bytes the
  saved game's header repeats). `unit_free` is `S15C5:0594`. Next: the
  loops over the units in `S02E6`, `S1CC6`, `S1FD8` and `S2138`, and a run
  with `-watch` on `unit_count` to see a fight.
- The second map (2026-10-04, from `build/n2/b7.ram`, a `-ram` dump after
  300 s of random play on a map of 32 by 24 with 107 units). Three units
  had 0200h in `unit_flags` and in word 0Dh another map offset than in
  `unit_pos` (two rows up); the map behind `map_ptr` had the unit's number
  at `unit_pos` and FFh at the other offset, the map behind `map2_ptr` the
  reverse, and the two maps differed in those six bytes only. Presumably
  the second map is the map as it will be once the orders are carried out,
  and word 0Dh where a unit is ordered to; that the orders
  are carried out later was not watched. Those units also had their
  `unit_pos` in word 11h, 3 in byte 13h and 1 in byte 14h; one unit had
  0100h in `unit_flags` and a field's first-byte offset in word 11h (not
  known). Byte 9 (`unit_serial`) counts the units of a kind and side as 1,
  3, 5 and so on. No unit had lost any of its `unit_count` in that dump
  or in the one of map 00: 300 s of random play do not bring a fight, so
  what a fight changes is still to be watched.
- Units after fights (2026-10-04). Six of the long runs were played again
  to 2400 s with a `-ram` dump at the end (`build/n2/fight.sh`, the dumps
  read with `build/n2/units.py`). In five of them units have a
  `unit_count` below their kind's byte 3 (the start value, 6 or 3): 1 to
  5 are left, in one dump of 19 units of 82. So `unit_count` is how many
  of a unit are left, as the code suggested. Byte 1 of those units is 1
  to 4, and a few units with a full count have it too: presumably it
  rises with fights (named `unit_exp`; what raises it was not watched).
  On map 00 `unit_total` was 29 after 30 at the start and the side
  records' byte 2 said 13 and 16 after 14 and 14: units are lost and
  gained.
- The fight (2026-10-04). `-watch unit_count+680` (unit 64) in the long
  run `w3` played again to 2400 s: the byte is set to 6 by `unit_add`
  (at `S15C5:0074`, t=17.9, which confirms the reading of `map_load`) and
  then lowered by one five times between t=534 and t=2395, always by the
  last store of the routine at `S2487:000A`, named `fight_losses`. Its
  end was read: it gets a record with far pointers to two units and to
  their kinds' records, bounds two losses by the units' counts and
  subtracts them; for a kind with 4 in its word 0Fh the result goes to
  the unit's byte 3 and `unit_count` becomes 0 when that is 0. Not read:
  the 900 lines before, which work the losses out, who calls it, and
  where a unit with nothing left is taken off the map (`unit_free`,
  presumably). That is the next step; `-break fight_losses` in the same
  run gives the caller and the record.
- `fight_losses`, first reading (2026-10-04; read once from the generated
  source, nothing of it checked in a run, so all of it is to be confirmed
  before the port uses it). The record it works on is the far pointer at
  `S3AEC:0008`: far pointers to the two units at 0 and 4, to their kinds
  at 0Ah and 0Eh, to two more records at 12h and 16h whose byte 5 indexes
  a table of byte pairs at `S3AED:0000` (presumably the ground of the two
  fields and what it adds to attack and defence), the units' map offsets
  at 1Ah and 1Ch, two bytes at 30h and 31h used as per cent added to the
  first unit's two values, a far routine at 6Ah called with a word made
  from the record's first eight bytes and the dword at 3Ah (presumably
  the random number). Each unit's count is taken from `unit_count` (or
  byte 3 for a kind with 4 in word 0Fh), at least 1 and at most 6. Which
  byte of the attacker's kind is its strength depends on the other unit's
  `unit_flags`: byte 0Ah if 10h is set, 0Ch if 8, else 0Bh (presumably
  three classes of unit, the bits 4, 8 and 10h); the kind's byte 8 or 9
  is the reach, and beyond one field the strength is lowered by distance
  (`S0E56:241F` is called with row and column of both; presumably the
  distance in fields). Byte 2 of a kind is used as the other value
  (presumably defence), `unit_exp` plus one enters as a factor. One
  unit does not hit back in four cases tested at the start (the other
  kind's word 6 has 80h, or 40h with the record's word 2Eh set, or word
  2Eh is 0, or the kinds' class bits do not meet). The exact arithmetic
  (the calls to the long multiply and divide at `CODE:03A1` and `03DC`)
  was not worked through.
- `fight_losses` entered (2026-10-04): `-break fight_losses` in the long
  run `w3` stops first at t=313.4 (`build/n2/h3.ram` is the dump there).
  The return address on the stack is `S2474:00A9`, so the caller is in
  `S2474`. The record's pointers are, as read: units 20 and 10, their
  kinds 12 and 13 in `unit_dat`, two 6-byte records of `ww1gnds_dat`
  (numbers 167 and 83) at 12h and 16h, so those are the grounds; the map
  offsets at 1Ah and 1Ch are 1AAh and B6h, word 2Eh is 0, and the far
  routine at 6Ah is `CODE:07AB`. Not done: reading `S2474` and
  `CODE:07AB`, and working the losses out by hand from this dump to
  compare with what the routine stores.
- The fight confirmed (2026-10-04). `build/n2/fights.sh` played the long
  runs `w0` to `w5` again to 2400 s (5, 10, 9, 35, 2 and 25 fights) and
  dumped memory at `fight_losses` and at its end (`S2487:0AB1`) for the
  first eight fights of each; `build/n2/fightcalc.py` works the losses out
  from the first dump as written here and compares with the second: all 39
  the same. The first unit of the record (A) is the attacker, the second
  (B) the target. All values are long, divisions go towards zero.
  - Count of each: `unit_count` (byte 3 for a kind with 4 in word 0Fh), 0
    taken as 1, above 6 as 6.
  - Attack of each: the byte of its own kind chosen by the other unit's
    `unit_flags`: 0Ah with 10h, else 0Ch with 8, else 0Bh. A's reach is its
    kind's byte 8 in the first case, else byte 9; with a reach above 1, A's
    attack is lowered by attack x (distance - 1) x 3 / (reach x 4), the
    distance from `field_distance`. B's attack is not lowered. Defence of
    each is its kind's byte 2.
  - A's attack is raised by the record's byte 30h per cent, B's defence by
    byte 31h per cent (value plus the upper word of (value x 65536 / 100)
    x per cent). This corrects the first reading above, which gave both to
    the first unit. The two bytes are written by `fight_percent`
    (`S02E6:2635`), which `attacks_run` calls before it fills the record;
    the bullet "The per cent bytes" below has how.
  - Strength of a value v for a unit with count n and e = `unit_exp` + 1 on
    a ground with bonus g: n x v + n x v x (e x 256 / 6 + 1) / 256 +
    g x (v / 2). g is a signed byte of the pairs at `S3AED:0000`, indexed
    by byte 5 of the field's record in `ww1gnds_dat`: the first of the pair
    for an attack, the second for a defence. For B's defence the middle
    term is divided by 512, not 256.
  - Loss of B = (A's attack strength - B's defence strength) / (B's defence
    x (8 - e of A) / 2), loss of A the same with the units exchanged. The e
    in this divisor is bounded: for A below 1 it is 2 and above 7 it is 6,
    for B up to 1 it is 2 and from 7 on it is 6 (the two tests differ, as
    the code has them).
  - Bounds, in this order: a loss of 0 or less becomes 1 when the other's
    defence strength / its count + its defence / 2 is below the attack
    strength; a loss of 7 to 11 becomes 6; a loss is at most twice the
    count of the unit that causes it, at least 0 and at most the count of
    the unit that takes it.
  - A takes no loss at all (the four cases of the first reading) when B's
    kind has 80h in word 6, when the record's word 2Eh is 0, when it is not
    0 and B's kind has 40h in word 6, or when B's kind's word 6 (bits 3Ch)
    and A's kind's word 0Dh share no bit. Word 2Eh is set by `attacks_run`
    when the target is one of the attacker's six neighbouring fields: only
    a neighbour hits back.
  - No random number enters: `fight_losses` only calls `rand_seed`
    (`CODE:07AB`) with a word made of the two unit pointers and the dword
    at `S331F:4CB7`; presumably the pictures of the fight draw from it.
  - Not met in the 39 fights, so read only: a kind with 4 in word 0Fh, a
    loss of 12 or more before the bounds, a divisor of 0 (the divide
    routine would then divide by zero).
- Around the fight (2026-10-04; read in `attacks_run`, `S02E6:000F`, not
  stepped). Each side's record (17h bytes at `S331F:4D33`) has a far
  pointer at 11h to a list of its attacking units and their number in byte
  15h; presumably that list is the F2h block of the saved game. For each,
  the target field is the unit's word 11h (`unit_aim`) and the target the
  unit on it; the record at `S331F:181C` (`fight_rec`) is filled, and with
  `S37FF:0115` 0 `fight_show` (`S2474:0005`) is called, which calls
  `fight_losses` and then `S2532:0008` with the counts before and after;
  otherwise `S131D:178D` is called with 0 and calls `fight_losses` itself
  (presumably the F9 setting; not tried). A unit whose count is 0 then goes
  to `unit_destroy` (`S02E6:208E`): its field's unit byte becomes FFh at
  once, the field is noted in one of four 7-byte records at `S331F:4C5E`
  with a count-down from the byte at `S331F:4C82` (presumably the
  explosion), and 4 is set in the unit's byte 14h (`unit_marks`). A
  carrier (1000h) that lost something passes the loss to `S02E6:2556`.
  `unit_exp` of a unit rises by 1 when the other lost something (unless
  the other unit's word 6 has 20h) and by 1 more when the other has
  nothing left, up to 6. At the end `unit_free` is called for every unit
  with 4 in `unit_marks`, and for the units a freed carrier holds.
  `unit_pos` is two words, one for each of the two maps (the code adds the
  side times two); the word at 0Dh is therefore the unit's place in the
  second map rather than "where it is ordered to" as written above.
- The per cent bytes (2026-10-04; `fight_percent`, `S02E6:2635`, read and
  then worked out from the 39 dumps by `build/n2/percalc.py` and compared
  with bytes 30h and 31h of the record: all 39 the same). This replaces
  the loose reading above, which had the zero case wrong.
  - `field_neighbours` (`S0B15:0162`) writes the map offsets of the six
    fields around a field to `neighbours` (`S331F:17FB`), clockwise from the
    one above, FFFFh for one beyond the edge; a field in an odd column lies
    half a field lower.
  - Both bytes are 0 when the attacker stands on none of the six fields
    around the target: only a fight between neighbours has them.
  - `fight_around(unit, other)` (`S02E6:2834`) marks each field around
    `other` that holds a unit which is on `unit`'s side (`same_side`,
    `S0CB6:0205`, bits 0 and 1 of `unit_flags`) and could attack `other`:
    its kind's word 6 has one of `other`'s `unit_flags` bits 3Ch (or bit 2,
    if `other`'s own word 6 has bit 2) and not 40h. It returns which of the
    six fields holds `unit`.
  - Byte 30h (the attack): for each of the other five fields around the
    target that is marked for the attacker's side, the byte of
    `around_percent` (`S331F:0009`) for its place counted round from the
    attacker (1 to 5) is added. So every further unit that stands at the
    target and could attack it helps, by its place: the encirclement.
  - Byte 31h (the defence): 50h for each of the two fields that touch both
    the attacker and the target, if marked for the target's side (a unit of
    the target's side there that could attack the attacker); at most 96h.
  - Which map: `attacks_run` begins by reading `attack_pick`
    (`S331F:4DA8`): 2 gives side index 0 and the map behind `map_ptr`, any
    other value index 1 and `map2_ptr`. So each side has its map and its
    word of `unit_pos`; this corrects "the map as it will be" above. All 39
    fights had index 1 (the byte 1, DI 1 at `fight_losses`, the attacker
    of the second side): the random play of the first side never attacked,
    and the dumps could not show a wrong pick (both maps and both words of
    `unit_pos` the same in them). Not met: byte 31h with both fields
    held (the bound of 96h), a field beyond the edge next to a fight (the
    first loop then reads the map's byte 0 instead of skipping; read only).
- The port's first routines (2026-10-04). `port/src/hl.h` gives the names
  of `src/HL.hints` as `S_name` (segment as loaded, image at 0076h as in
  dosrun's dumps of `HL.EXE` started by itself, so PSP 0066h) and `A_name`
  (offset), from `port/src/gen/names.h` (`symmap.py`; check.py keeps it up
  to date, so a new name in the hints means running symmap.py again before
  the commit). `port/src/fight.c` translates `fight_percent`,
  `fight_around`, `field_neighbours`, `same_side`, `field_distance` and
  `fight_losses`; `port/tests/fight_test.sh` runs them on the 39 pairs of
  dumps: all the same (port/README.md, "Checked", has what differs in the
  rest of memory and what the fights do not cover). Names added for it:
  `units`, `fight_rec_ptr`, `ground_bonus`, `rand_state`. A test of this
  kind (the original's memory at a routine's entry and end, the port's
  routine run on the first) works for every routine that draws nothing and
  waits for nothing; the game's loading and its loop come later.
- `attacks_run` before and after the fight (2026-10-04, read only, not
  stepped). After filling `fight_rec` it exchanges `map_ptr` and
  `map2_ptr`, fills the 1104h bytes at `S331F:3B5A` (a block of the saved
  game) with 80h or 40h (by the side) and writes 0 at the two units'
  places (both halves of a two-part unit), calls three routines of
  `S0E56` (`177C`, `0D44`, `0F36`; presumably they draw the map) and
  exchanges the pointers back; so at `fight_losses` the pointers are as
  before, as the test takes them. The index into `3B5A` is column plus
  row times 64, one byte a field (1104h is 66 times 66, which does not fit
  rows of 64; not understood); presumably what of the map is to be drawn
  or seen (not checked). Later
  in the routine the pointers are exchanged around `S0E56:145A` in the
  same way. Between and after: messages (`S10AF:0131`), `S2974:0002`,
  `S28DC:000A`, `S2A10:0006`, none of them read. This is why
  `attacks_run` is not translated yet: nearly all it calls besides the
  fight draws or waits.
- `unit_destroy`, `unit_free`, `site_free` (2026-10-04), read and translated
  (`port/src/units.c`), 12 calls each of the first two compared with the
  original (`port/tests/units_test.sh`, dumps by `build/n2/destroy2.py`):
  the same. The six long runs hold 14 calls of each.
  - `unit_destroy(unit, byte, byte for the other half, slot, side index,
    map)`: notes the unit's field in record `slot` of `blasts`
    (`S331F:4C5E`, 7 bytes: map offset, a count-down from `blast_time`
    `S331F:4C82`, the field's first byte and the new unit byte as words),
    writes the byte as the field's unit and sets 4 in `unit_marks`; for a
    unit of two parts (40h) the other half (the next unit with 80h set,
    else the one before) the same in the next record, its count-down one
    higher.
  - `unit_free(unit, row)`: counts its own depth in `free_depth`
    (`S331F:17F2`) and returns above 5 or for a record already unused
    (8000h), in both cases without lowering the depth again (as read; not
    met). For a unit of two parts both records become unused and 2 is
    taken off instead of 1; a carrier (1000h) gives up its record through
    `site_free`; `unit_total` and byte 2 of the side's record (`sides`,
    `S331F:4D33`; side by bits 0 and 1 of `unit_flags`, none for bit 1)
    are lowered, `unit_flags` becomes 8000h.
  - `site_free(record, row)` (`S15C5:06EA`): frees the seven units of the
    record's row `row` (0 or 1; `unit_free` hands its own second argument
    on) and writes FFh there; then by word 19h of the record (side bits 0
    and 1 as in `unit_flags`): with 10h byte 4 of the side's record and
    the second byte of `site_totals` (`S331F:4CAD`) are lowered, with 8
    byte 3 and the first, with 20h the third; word 19h becomes 8000h. So
    word 19h of a 1Ch-byte record is its flags, and the three bits are
    presumably the three tables filled from the ground (which is which is
    not checked).
  - Not met in the 12 calls: a unit of two parts, a carrier (so
    `site_free` is read only), the early returns.
  - `build/n2/d0.log` to `d5.log` of an earlier session were overwritten
    by this session's first logging script (scratch files; what they held
    is not known to matter).
- Making a battle's state, translated (2026-10-04; `port/src/units.c`,
  `port/src/map.c`, compared with the original by
  `port/tests/units_test.sh`: 60 calls of `unit_add`, 43 of the three
  `site_add` routines, and the parts of `map_load` and `shp_load` behind
  their `file_load` on six maps, all the same; port/README.md has what is
  not covered).
  - `shp_load` (`S0E56:0079`) is called after `map_load` and reads the
    map's `.SHP` file: a byte for each of the 33h kinds (bit 0 sets or
    clears 2 in the kind's word 0Fh of `unit_dat`), a count, then 12-byte
    records: the units' flag bits (the side), which table (0 `sites_40`, 1
    `sites_400`, 2 `sites_100`, else `carriers`), the record's number, a
    byte for the record's bytes 16h and 17h, one for byte 18h, and seven
    kinds (above F0h: none). Each unit is made by `unit_add` on the
    record's first field with 4000h in `unit_flags` (presumably "stands
    inside") and is written into the same place of both rows of the
    record. So the 14 bytes at the start of a 1Ch-byte record are what
    stands inside a site or carrier; why there are two rows is not known.
    This is where the units beyond those on the map's fields come from
    (the six runs log 30 to 84 calls of `unit_add`, of which `map_load`
    makes 23 to 66).
  - The three tables filled from the ground are named `sites_400`
    (`S331F:3A0E`), `sites_100` (`38F6`) and `sites_40` (`38BE`). Their
    flags (word 19h) get 8, 10h and 4, a carrier's 20h, which settles what
    `site_free` counts down: `site_totals` byte 0 and the sides' byte 3
    are `sites_400`, byte 1 and the sides' byte 4 `sites_100`, byte 2 the
    carriers. `map_load` does not set `site_totals` byte 2 to 0 (read; the
    comparison begins behind that part). Bytes 16h and 17h of a record
    start as 1Eh, 14h and 32h (for a carrier its kind's byte 40h) and are
    then set from the `.SHP` record; byte 18h as 0Fh, 0Ah, 14h and 0.
  - The four map offsets of a record: `sites_400` the field, the one two
    columns to its left and the two between them; `sites_100` the field,
    the one below and the two beside that; `sites_40` and a carrier the
    one field four times. So the first two are things of four fields.
  - `sites_40` has one record for each side, the side being the record's
    number; `map_load` calls `site_add_40` for a field of no side (2) too,
    which then writes the place of the first record of `sites_100` (read;
    whether a map has such a field was not looked at).
  - `unit_add` takes the kind's serial number for the side from the kind's
    byte 3Dh or 3Eh and adds 2 there (which is why `map_load` sets both to
    1, and why the saved table differs from `UNIT.DAT` in them); bytes 0Fh
    and 10h of a unit start as 3 for side 0 and 0 otherwise.
  - `build/n2/a0.log` to `a5.log` were written anew by this session's
    `add1.py`; whether an earlier session had files of those names is not
    known (`a6.log` to `a9.log`, empty, are older).
- `file_load` translated (2026-10-04; `port/src/files.c`, compared by
  `port/tests/load_test.sh` with the original on all 47 calls of run `w0`
  up to its first map: the returned pointer and all memory the same, but
  for bytes the original's routine does not write itself, see
  port/README.md). Its arguments are a place, a name and a work buffer of
  1000h bytes; the packed format is in the routine's comment in
  `src/HL.hints` (read from `HL.EXE`'s own unpacker; the offline
  `tools/tpwmfiles.py` was not consulted for it). 44 of the 47 files were
  packed, one not (`AMOK.DAT`), and
  `MAP\00.HI` (asked for twice) does not exist: the routine returns 0.
  Every call gave a place and a work buffer, so the allocating path and
  the path without a work buffer are read only. The names the game asks
  for include `LIB\partS.LIB`, `LIB\unit.LIB`, `LIB\cursor.LIB`,
  `LIB\exp.LIB`, `LIB\bum.LIB` and `LIB\newshop.LIB` with their `.DAT`:
  the "`.LIB` and `.DAT` whose name has only the extension" of the list of
  opened files above was a name cut short in that trace's reading, not an
  empty prefix. `ptr_norm` (`S2C9E:0002`) is named. `file_open` uses the
  string behind the name when `DATA:0378` is above 0 and calls a hook
  around the opening when `DATA:04B9` is set (neither met nor read
  further).
- `map_load` and `shp_load` whole, `file_name`, `num_text` (2026-10-04;
  `port/src/map.c`, `port/src/names.c`, compared by
  `port/tests/load_test.sh`: 47 calls the same, port/README.md has the
  limits). A battle is set up by the first part of `game_loop` itself
  (`S066A:000D`; about a thousand lines of the generated source before
  the loop, with calls for pictures, sound and memory between the loads;
  most of it not read). Read there, around `S066A:07C6` to `0938`:
  - `map_load(name, place, work buffer)` gets `map_ptr` itself as the
    place, normalises it and afterwards sets `map_ptr` four bytes behind
    it (the file's header), so `map_ptr` moves on by four with every map
    loaded in one start of the program (read; a second battle in one run
    was not looked at). `shp_load` gets the far pointer at `sides`+9
    (`S331F:4D3C`) as its place: the `.SHP` file is loaded where side 0's
    list is (what that list is, is not known).
  - The same routine loads `UNIT.DAT` to `unit_dat` and `WW1GNDS.DAT` to
    `ww1gnds_dat` directly with `file_load`, and calls `S0CB6:1842` with 2
    when a load fails (presumably the error exit; not read). After
    `map_load` it sets the word at `S331F:0008` to 1 for a map wider than
    32 or higher than 40, else 2 (one of the words the saved game's header
    repeats).
  - `file_name(where, folder, name, ending)` (`S1571:02CD`) makes every
    file's name at `name_buf` (`S3310:00B9`) from `name_dirs` (6-byte
    entries; 0 and 1 were met, `LIB\` and `MAP\`) and `name_exts` (5-byte
    entries); the map's number comes from `num_text` (`S2892:0001`, the
    map's number in two places). `where` 2 puts `name_prefix`
    (`S3310:007D`) in front; it was met for `MAP\00.HI` with an empty
    prefix (presumably the folder for files the game writes; not checked).
  - `S288D:0009` normalises a far pointer like `ptr_norm` (not named; the
    port uses `ptr_norm` for it).
- `game_loop` is the program's main (2026-10-04, its first 400 lines
  read, nothing of it stepped): its arguments are the command line's
  count and words; `/a` and `/b` (either case) choose 1 or 2 instead of 3
  for the call of `S2F39:000B` (presumably the sound or picture device;
  not read). It then asks `mem_alloc` for 445C0h bytes (the one
  allocation of a start up to the first map), sets up a 320 by 200 screen
  (`S298E:000D` with 140h, C8h, 4), loads two files into that block
  through `S2B81:0004` and keeps their places at `S331F:17AA` and `17A6`,
  and goes into the menu (`S167A`) before the loads of a battle described
  above. `S0CB6:1842` is called with 1, 2 or 3 after a failed allocation,
  load or screen setup (presumably the exit with a message). `mem_alloc`
  is translated (`port/src/mem.c`, on the runtime's `dos_alloc`; the one
  call compared, the same).
- `/a` and `/b` (2026-10-04, read, not tried in a run): the value they
  choose is a mask of the sound devices `S2F39:000B` tries, 3 without a
  switch. Bit 0 (`/a` alone) is the detection at `S30EF:1A7D`, which reads
  port 388h and makes the timer test: the AdLib. Bit 1 (`/b` alone) is
  `S30EF:2050`, which hooks an interrupt vector, unmasks its IRQ and sends
  D1h: presumably the Sound Blaster (no port number of the card looked
  for). The devices found are kept as the same mask in `S30EF:025E`, and
  `S3303:0022` is 1 when the second was not found (tested in some twenty
  places). A switch therefore only takes a device away.
- Decided with the user (2026-10-04): the port behaves as the game does
  without a switch and with both devices found (mask 3 in `S30EF:025E`, 0
  in `S3303:0022`): music and digital effects, always. It does not offer
  `/a` or `/b`.
- The port's own start (2026-10-04; `port/src/battle.c`, `port/src/main.c`).
  Two parts of `game_loop` are translated, without what draws or sounds:
  - `start_setup`, the entry to `S066A:0349` (before the menu): the block
    of 445C0h bytes, then two files loaded to its beginning with
    `file_load_size` (`S2B81:0004`: `file_load`, then `file_size` of the
    name), the work buffer 7530h bytes behind the place; the names are
    `file_name(1, 0, S331F:023E, 3)` and the same with ending 5 (the
    `LIB\NEWCHAR6` pair of the trace above). Their places go to
    `S331F:17AA` and `17A6` and to 290h and 294h of the record behind
    `DATA:0092`, and the block's free part begins behind them. `map_number`
    (`S331F:4CBB`) and word 0 of both sides' records are set to 0.
  - `battle_setup`, `S066A:0417` to `0938`: the free part is shared out
    with `ptr_move` (`CODE:04CB`) in this order: 320h bytes (the buffer
    `save_game` writes), 82h behind `S331F:1893`, 82h behind `1897`, 1806h
    for `map_ptr`, 1806h for `map2_ptr`, BB8h behind `S331F:17BA` (the
    file `file_name(1, -1, S331F:024C, 6)` is loaded there; `GAME.TXT` by
    the trace), then `S02E6:323E` is called with the free end and the
    block's beginning as work buffer (it loads the map's pictures there,
    the `S`/`W` libraries; not translated, only its beginning read), and
    over that same place come F2h for the second side's list of attackers
    (`sides`+17h+11h), F2h for the first side's, 2710h for each side at
    0Dh of its record (the first is the work buffer of all loads that
    follow), 3E8h for each at 9 (the first takes the `.SHP` file). Each
    side's pointer at 5 is a fixed place in the screen's memory
    (`A7E8:0004`, `A7D0:0300`; what for is not known). Then `AMOK.DAT`,
    `UNIT.DAT` and the grounds' file are loaded to their tables, `map_load`
    and `shp_load` called. `S331F:0262` (the last letter of the grounds'
    file's name) and `S331F:0149` (of the pictures' name) are `W` when
    `S37FF:012F` is not 0, else `S`: presumably summer and winter (not
    checked). With 400h in `S331F:4CA4` a file with ending 9 is loaded to
    `S39B6:0000` next (the bullet "The rest of a battle's setup" below).
  - Compared (`port/tests/load_test.sh`, kinds `start` and `setup`, dumps
    by `build/n2/start1.py` and `setup1.py`): `start_setup` on the one
    start, `battle_setup` on the six runs' maps; no byte the port's
    routines write differs from the original, and the pointers they keep
    are those of `game_loop`'s frame. The original's routines left out
    write some 15,000 and 110,000 bytes more (pictures in the block, the
    table of far pointers at `S331F:02E9` to `05A2`, the screen's records).
  - `port/build/history-line-headless -game game -map N -ram FILE` does
    both from `HL.EXE` loaded by `rm_load_exe` (PSP 0066h, 3F80h
    paragraphs as in dosrun) and writes its memory. For map 0 against the
    original's dump at `S066A:0938` of run `w0`: the block lies at the same
    address (`40A6:0002` behind the two files), and of the 40,448 bytes
    from `S331F:0000` to `DATA` 750 differ: `S331F:0149` to `0157` and
    `02C7` to `05A2` (the pictures' name and tables), three bytes of
    `4CA4`, three in the sides' records (`4D49` to `4D60`), 36 in `4E12`
    to `4E7E`, six in `4F1E` to `4F2D` and one at `9DF6`: what the menu and
    the routines left out set (not looked at one by one). Units, sites,
    both tables of kinds and grounds and the sides' counts are the same.
    Next: `S02E6:323E` and the calls after `0938` (`S274D:0003`,
    `S02E6:3484`, `S02E6:2B87`, `S0B15:000B`, `S0E56:000B`,
    `S02E6:29D4`, `S1C74:01B4`, `S0CB6:0CDA`), and what the menu leaves in
    `4CA4` and the sides' records.
- The rest of a battle's setup (2026-10-04; `game_loop` from `S066A:0938`
  to `S066A:0EE6`, where it jumps into its loop; read, and the part that
  draws nothing translated as `battle_begin` in `port/src/battle.c`).
  - What the menu leaves (read in `S167A`, around `S167A:23F0` and
    `S167A:3130`; the values seen in the dumps of maps 0 to 5): byte 6 of
    the map's ten-byte record in `CODES.DAT` is 2 for a map for two
    players, which makes both sides human and clears 400h in `game_state`
    (`S331F:4CA4`); any other map sets 400h and makes one side the
    computer's, the second below map 18h, else the first. A side of the
    computer has 2 in word 0 of its record in `sides`. Word 2 of
    `game_state` gets 1 for one computer side, 2 for none, 4 for both, and
    8 from another item of the menu (not looked at which). Byte 16h of both
    sides' records is 4, 8 or 10h by an item with three choices, FFh
    without (presumably a level; it multiplies `defence_sum`). In the six
    runs: `game_state` 0401h, its word 2 1, the second side 2, the bytes
    FFh.
  - With 400h the file of the map with ending 9 is loaded to `S39B6:0000`:
    `MAP\NN.COM`, so the `.COM` files are the computer's player's data for
    a single player's map (48 of the 72 maps have one). On map 0 it changed
    bytes up to `S39B6:131B`, so it lies over the tables of `S39B6` the
    saved game holds (what it holds was not looked at).
  - `S274D:0003(map, free end, work buffer)` is called unless a saved game
    is to be loaded: nothing for a map above 2Fh, else it loads files and
    shows texts and a picture (read to its first screen only; presumably
    the briefing). Left out of the port.
  - `S02E6:3484` and `S02E6:2B87` load libraries of pictures to the free
    end through `S0C66:0009` (records at `S331F:0131`, `0159`, `01A9`,
    `01E5`, `022D`; each record's dword at 0Ah is the length the free end
    moves on by) and return the new free end, which `game_loop` keeps at
    `S3303:0028`. `2B87` also loads a file named by the number at
    `S37FF:0112` (presumably a palette, handed to `S0CB6:187F` with 300h)
    and makes two blocks of 1194h bytes (`S331F:02D7`, `02DF`). Left out.
    2026-10-08: `S0C66:0009` is `lib_load`, `3484` is `libs_start`
    (`LIB\pal.LIB` at the record `lib_pal`, sorted by `LIB\pal.DAT`
    through `lib_order`, `S0C66:024E`; then `LIB\rnd.LIB` at `lib_rnd`).
    A library's first dword is the offset of a table of 12-byte entries
    (eight bytes of name, the picture's offset in the file) ended by an
    offset of 0; `lib_load` makes it a list of normalised far pointers in
    place and fills the record (14h bytes: name, length at 0Ah, list at
    0Eh, count at 12h); the src/HL.hints comments have the rest. Records
    seen in a start: `S331F:0131` pal, `0145` partS or partW (`323E`,
    sorted too), `0159` rnd, `01E5` exp, `01A9` bum, `01F9` newshop
    (`pictures_load`, which loads rnd a second time), and the menu's six
    `MNU*.LIB` at `S37FF:0008` to `006C`. Both are translated
    (`port/src/files.c`, `battle.c`; `battle_begin` now calls
    `libs_start`) and `lib_load` compared byte for byte on all 13 calls
    of a start on map 0 and on map 8 (port/README.md). Not reached:
    `pictures_load` (its picture `S2B06:0000`, the palette, `lib_read`,
    the blocks at `02D7`/`02DF`, sound), `323E` and `lib_pick`
    (`S0C66:0414`), a library or a list name that is missing.
  - State, all translated: `field_bits_clear(FFh)` empties `field_bits`
    (`S331F:3B5A`); `view_tables` fills `row_y`, `row_y_odd` and `col_x` (a
    field is 10h wide and 18h high on the screen, odd columns 0Ch lower);
    the words at `S331F:02B5` and `02B7` are 0; `clock_seed(0)` keeps a
    value made of the BIOS's tick count at `DATA:0AF1` (who reads it is
    not known; `rand_state` is another place). In `game_state`: word 4, word
    11h and dword 13h 0, byte 86h 0, bytes 0Dh and 0Eh the map's width less
    10 and height less 8, halved (presumably how far a view can go), word
    0Fh 4, dword 8Bh `defence_sum()`, and 15 entries each of dwords at 28h,
    bytes at 19h and words at 64h cleared. Both sides' count of attackers
    (byte 15h) 0, bits 0 and 2 of their word 0 cleared, `computer_reset`
    for a computer's side, `view_reset` for both `views` (`S331F:4D92`,
    31h bytes a side), then `view_to_field` puts each side's view on the
    field one row below the first field of its record of `sites_40`. So
    `sites_40` is where a side's view begins: presumably its headquarters
    (not checked on the screen). The view is 10 columns by 8 rows and the
    second side's cursor lies A0h further right: each side has half the
    screen. `attack_pick` (byte 16h of a view) becomes 1 for the first side
    and 2 for the second. Bytes 0 to 5 and 8 of the records behind
    `DATA:00A6` and `DATA:00B6` are cleared (presumably the two sides'
    input; byte 8 is FFh again at the end), byte 2 of the four `blasts`
    becomes FFh, `fight_rec` gets the far pointer at `S331F:013F` (set by
    the libraries' loading) at 3Eh and seven routines' addresses at 5Ah to
    72h (`S30EF:1E8B`, `S2AF3:000A`, `S2A6B:0008`, `S2A6F:0002`,
    `rand_seed`, `S0CB6:0713`, `CODE:0288`), `game_state` gets C0h, its
    word 2 30h, `free_depth` and the word behind it 0.
  - With 200h in `game_state` the saved game is loaded here (`load_game`,
    then `S02E6:323E` and `2B87` again, the bit cleared). Not translated.
  - Then the views are drawn (`S0E56:0D44` and `S0E56:0F36` for each side
    with word 2 of its view, `show_message(-1, side)`), bits 4 and 8 of
    `game_state` cleared, the record behind `DATA:00BE` emptied (word 4,
    bytes 0 and 2), `S2974:0002`, `S28DC:000A`, `S28E0:0004`, and with 40h
    in `game_state` `S2F39:0F87` with the far pointer at `S331F:179A`
    (presumably the music); byte 0Ch of `game_state` 0, bit 2 set, and the
    loop begins at `S066A:45D7`.
  - Compared (`port/tests/load_test.sh`, kind `begin`; the original's dumps
    at `0938` and at `0EE6` of the six runs, `build/n2/begin1.py`): no byte
    `battle_begin` writes differs. Counted apart: the seed (the clock), the
    two byte 8 named above, and what the loading routines keep, since the
    original loads more files after the `.COM`. In the game's state
    (`S331F` to `DATA`) the original alone writes the libraries' records
    and three letters of names (`S331F:013B` to `0284`), `02D7` to `02E6`,
    three far pointers at `179A` and `fight_rec`'s 3Eh.
  - The port's own run on map 0 against the original's dump at `0EE6`: 803
    of 40,448 bytes from `S331F:0000` to `DATA` differ, all in those
    places, in `S331F:02C7` to `05A2` (the pictures' tables), in records
    of `S37FF` (`0012` to `007E`, `011E`, and the map's code at `0129`,
    which the menu writes) and one byte at `S3CFE:0006`. `game_state`, the
    sides' records, the views and the computer's segment are the same.
  - `menu_result(map)` in the port sets what the menu leaves for a map with
    nothing else changed, reading byte 6 of the map's record in `CODES.DAT`
    itself (loaded to the block's free part for that; the name is at
    `S37FF:009E`). Two runs of the original to `S066A:0938`
    (`build/n2/mn30`, `mn60`) confirm the reading: map 30 has `game_state`
    0401h, word 2 1 and the first side 2; map 60 `game_state` 0001h, word 2
    2 and no side 2. The port's run gives the same for maps 0, 30 and 60,
    and sets up maps 48 to 71, which have no `.COM` file.
- The loop of `game_loop`, first reading (2026-10-04; `S066A:0EE9` to
  `S066A:45D7`, some 4,700 lines of the generated source of which the
  first 900 were read; nothing stepped, nothing translated).
  - Shape: while 2 is set in `game_state`, a pass serves both sides one
    after the other (byte 0Ch of `game_state` is the side, 0 then 1; the
    pass begins at `S066A:0EE9`, a side at `0F0C`, the pass ends at
    `43A9`). When the bit is cleared the battle is over: `S066A:45E8`
    stops the music and calls `S1A39:0001` and `S063F:0009` (presumably the
    end's screens and the high score; not read), and 20h in `game_state`
    makes the routine return 7Bh, the status the launcher starts the outro
    for.
  - A side's input: its record behind `DATA:00A6` or `DATA:00B6` (which
    side gets which depends on 2 in `game_state`'s word 2 and on the map's
    number, 18h to 2Fh exchanged), its view and its record of `sides`. A
    side of the computer (2 in its word 0) calls `S1C74:000B(side)` instead
    and shows the text whose number is at `S39B6:1317`; a human's input
    comes from `S0CB6:05F1(input record, view)` as bits in DI: 1, 2, 4, 8
    the directions (up, down, left, right, as the cursor's moves use them)
    and 10h presumably the button. The side's map (`map_ptr` for side 0,
    `map2_ptr` for side 1) is kept at `S331F:17AE`.
  - Computer turn and income routines (2026-10-07; read in generated
    source, with runtime evidence noted below). `computer_turn` at
    `S1C74:000B` advances a phase word in the side's 0Ch-byte record at
    `S39B6:12FD`: assess (20h), plan (40h), queue (8), run queued steps
    (2), then play the input-key script (1). A run saw side 1 pass through
    20h, 40h, 8 and 2 and show texts 4Eh, 4Fh and 60h–63h (6,392 hits,
    all for side 1); a computer-controlled side 0 was not observed. The
    phase helpers and step handlers were read but not stepped. Their broad
    roles and the meaning of steps 4 and 6 remain partly guesses; details
    and addresses are named in `src/HL.hints` under `computer_turn`.
    `income_add` at `S131D:1745` adds `side_income` to the side's points;
    `game_loop` calls it at the end-of-round path after `attacks_run`.
    This call path was read in generated source, but no run reached the
    round end for an income observation. `side_income` sums byte 16h plus
    the side index over the `sites_100` and `sites_400` records, and that
    byte in the side's `sites_40` record. Treat the ownership bits and
    whether unused records contribute as unverified until exercised.
  - Byte 17h of a view is the state of that side's hand, 0 to 7 (`0` the
    cursor moves freely: `S0CB6:0750` and then the column and row of the
    view change by the bits, an odd column's neighbours half a field off).
    With no direction and state 0 the unit under the cursor is shown
    (`S10AF:0131(unit, side)`), and for a unit of the side that has an aim
    (`unit_aim`) with 2 in byte 16h of the view the aim's field is marked
    in `field_bits` (4 for side 0, 8 for side 1) while `S0E56:145A` draws:
    so those two bits of `field_bits` are a mark for the drawing.
  - Keys are compared with the entries of the table behind `S331F:02D3`:
    entry 0 (Esc) asks to quit (texts 3Eh and 3Fh) and waits; entry 18h
    confirms: 2 is cleared and 1 set in `game_state` and the dword at
    `game_state`+8Bh (`defence_sum`'s) becomes 0; entry 1 or a button
    cancels. Entries 0Dh to 14h are F2 to F9: music (40h in `game_state`,
    texts 55h/56h), effects (80h, 57h/58h), entry 0Fh (F4) shows text 59h
    and the two bytes at `S331F:0006` and `0007`, F5 and F6 switch words
    34h and 36h of the record behind `DATA:009A` (the joysticks, texts 65h
    to 68h), F7 and F8 work on the record behind `DATA:00C2` (presumably
    the mouse and its speed in three steps, `S37FF:0113`; texts 69h to
    6Dh), F9 steps through three settings of `S37FF:0115` and `S37FF:0130`
    (texts 70h to 72h; `0115` is what `attacks_run` tests before
    `fight_show`). 200h in `game_state`'s word 4 keeps a key from
    repeating.
- The loop of `game_loop`, second reading (2026-10-04; `S066A:1F39` to
  the loop's end at `S066A:45D7`, some 3,400 lines; read, nothing stepped,
  nothing translated, none of the routines it calls read but the two
  named). What a side does in a pass is chosen by byte 17h of its view,
  with bytes 18h and 19h as steps inside it; byte 1Bh is the picture of
  the side's pointer (5 the plain one; the table of pictures is behind
  the far pointer at `S331F:0167`), words 10h and 12h where it is drawn.
  Where the states are entered from state 0 is in the part read before
  (`S066A:0F0C` to `1F39`) and was not followed.
  - State 1 only calls `S0CB6:0EAA(view, side, input bits)` (not read;
    presumably a unit's order, since nothing else here moves one that
    stands on the map).
  - State 3, the whole map small. Entered with 4 in byte 18h: the map is
    drawn in the middle of the side's half of the screen
    (`S0E56:09AD(side, x, y, map_ptr)`), a field 2 pixels wide and high
    when the word at `S331F:0008` is 1 and 4 when it is 2 (so that word,
    which `battle_setup` sets by the map's size, is the shift of this
    picture); the cursor becomes the view's corner (bytes 14h and 15h of
    the view are the corner's column and row halved), 4 is set in word 0
    of the side's record and byte 18h becomes 5. After that the
    directions move the corner two fields at a time, within the map less
    the view's 10 by 8, and the button goes back to state 0 with the
    view's first field (word 2) set from the corner and the view drawn
    again. `S2904:000E` with the side's far pointer at 5 of its record is
    called after each move (presumably it brings back what was under the
    frame; that pointer is the place in the screen's memory named above).
  - State 4, what is on the cursor's field. With 0 in byte 18h: a unit
    there (the second byte of the side's map up to F0h) is shown by
    `S131D:0483(side, map offset, unit, map)` and byte 18h becomes 6; the
    second half of a unit of two parts (40h without 80h in `unit_flags`)
    shows the first. A unit with 2 in its word 6 is shown only to its own
    side (`same_side`). Without a unit, or with such a foreign one,
    `S131D:0EE5(side)` is called and byte 18h becomes 7 (what it shows
    was not looked at). The button goes back to state 0.
  - State 6 goes into what stands on the cursor's field and becomes state
    2: by the ground's word 0 in `ww1gnds_dat`, 200h the record of
    `sites_400` with that field, 80h that of `sites_100` (both found by
    `S0E56:1726(map offset, table)`), 40h the record of `sites_40` of the
    side the ground belongs to; without those bits the record of
    `carriers` named in byte 0Ah of the unit on the field (of the unit
    before it with 2000h in `unit_flags`). The far pointer to the record
    is kept at 24h of the view, and word 22h is 1 for a site and 2 for a
    carrier. (The three bits are not the ones `map_load` adds the records
    by, 400h, 100h and 40h; not looked at why.)
  - State 2, inside a site or a carrier. The record's first row of seven
    unit numbers belongs to the first side and the second to the second
    (the row is the side times 7); the cursor stands on one of the seven
    places, 11h pixels apart, odd ones 0Ch higher. Steps in byte 18h:
    - 9 draws the inside (`S131D:0A1C(side, record)`), puts the cursor on
      the first place that holds a unit (the fourth without one) and, for
      a site with 2 in byte 16h of the view, shows the side's
      `side_points` (`S131D:14D4`); then 0Ah.
    - 0Ah: the directions step through the places; the place's unit is
      shown (`S131D:0C50(2, kind, side)`; text 2 for an empty place, 8
      and 9 for a unit with 400h and 800h in `unit_flags`). A site shows
      text 25h with byte 16h (the first side) or 17h (the second) of its
      record and the side's `side_points` (0 for a site with 2 in the
      record's word 19h, so that bit presumably says the site is
      nobody's, and bit 0 that it is the second side's); a carrier shows
      the sum of byte 41h of the kinds in it and byte 40h of its own
      kind (presumably load and room). The button goes to 0Bh.
    - 0Bh, what can be done with the place, kept as bits in word 1Ch of
      the view and chosen with a direction while the button is held, done
      when it is let go: right (always) leaves, to step 8. Up moves the
      unit out, for a unit of the side without 100h, 200h and 2 in
      `unit_flags`, with byte 16h of the view not 2 and 80h clear in
      `game_state`'s word 4; but when the side's count of attackers (byte
      15h of its record) has reached byte 16h of the record, a human
      gets text 53h instead. So that byte, 4, 8, 10h or FFh from the menu,
      is presumably the number of units a side may send in a round. Down
      repairs, with 2 in byte 16h of the view and without 4 in the unit's
      word 6, in a site only. Left builds, on an empty place of a site
      with 8 in its word 19h (what `site_add_400` gives) that is the
      side's, with 2 in byte 16h of the view and `build_count` above 0.
      - Moving out: the place is emptied, the unit's place on the side's
        map (word 0Bh, or 0Dh for the second side) becomes the record's
        field (word 0Eh; one field less with 20h in the record's flags,
        a carrier), `S0B15:0371` is called with the field, the side's
        work buffer, the unit and masks made of the unit's side and its
        word 6 (presumably it marks where the unit can go), the unit is
        put on the map, gets 100h in `unit_flags`, its field 40h or 80h
        in `field_bits` by the side, and the view goes to state 0 with
        16h in byte 18h and 3 in 19h (the choosing of an aim, in the
        part not followed).
      - Repairing costs 0Fh of `side_points`, 32h for a unit with 4000h
        in word 6: text 4Ah when the side has less, 50h when
        `unit_count` already is byte 3 of the kind (the kind's full
        count), else `unit_count` becomes that, `unit_flags` gets 0A00h
        and text 51h is shown. (A unit with 4000h in word 6 also gets 2
        in byte 0Ah and counts as whole only with that.)
    - 0Ch, the list of kinds to build: `build_kinds`, `build_count` of
      them, seven shown (`S131D:0B07`), with text 7 and the kind's byte
      3Fh, its price. The button held with left builds: `S15C5:01A7`
      with 8000h gives a free unit's number, `unit_add` makes it on the
      site's field for the site's side (bit 0 of its word 19h), it gets
      4600h in `unit_flags`, its two records of 9 bytes in the
      computer's segment (`S39B6:09AB` and `0132`) are emptied, it is put
      in the place, `unit_total` and the side's count (byte 2 of its
      record) go up and the price is taken from `side_points`. This is
      the second caller of `unit_add` the list below named. The price is
      not compared with what the side has here (not looked at whether
      the list only offers what it can pay).
    - 8 leaves: state 0, the cursor back on its field, the view drawn.
  - State 5, the side has ended its round. The button takes that back
    (text 34h). While the other side's view is not in state 5, text 32h
    or 33h is shown (33h with 2 in byte 16h of the view), and a human
    who plays against the computer sets `game_state`'s word 0Fh to 0 and
    clears 800h (and sets 80h in word 2 when the computer's record at
    `S39B6:12FD` has 60h). With both in state 5 the round is carried out when
    both sides are the computer's or the key of entry 0Ch of the table of
    keys is pressed: `attacks_run`, then both views put back on their
    fields and drawn, `S131D:1745` with the side `attack_pick` names (the
    second with 2 clear there) and 4 set in `S39B6:12FD` and `1309`. `attacks_run` returns a side or a negative value: with a side
    both views go to state 7, word 22h of that side's view becomes 0Fh
    and the other's 10h. What takes the views out of state 5 after a
    round was not found here (presumably `S131D:1745`). The key of entry
    1Ah saves in this state: `save_game`, whose result is the number of
    the text shown.
  - State 7, the battle's end: word 22h of the two views chooses the
    texts (0Fh: 3Ch for that side and 3Dh for the other, and 4000h in
    `game_state`; 11h: 41h and 42h, and 2000h; else 42h for both). 10h in
    `game_state` is set for two humans, and with one side the computer's
    when the human's view has 0Fh or 11h; it is cleared again when
    neither 4000h nor 2000h is set. Then `S0CB6:0DA9` (presumably waits)
    and 2 is cleared in `game_state`, which ends the loop. Behind the
    loop 10h chooses the call with picture CDh or D0h (by 2000h) and the
    side whose view has 0Fh. Which of 0Fh and 11h is which end was not
    looked at (who writes 11h was not found in this part).
  - The end of a side's part: with 100h in `game_state`'s word 4 and
    effects on (80h in `game_state`) `S2F39:06E3` plays the effect whose
    number is word 6 of `game_state`, with 9 for the second side and 0Bh
    for the first as its first argument; byte 0Ch goes on to the next side.
  - The end of a pass (`S066A:43A9`): 8 and 10h in `game_state`'s word 4
    (a text was shown to the first or second side) become timers of kind
    1 and 2 over 19h passes (`timer_set`), 40h calls `S10E1:072C`; both
    sides' pointers are drawn (`S2A6B:0008`, not in state 4), then
    `S2F39:096A`, `S2974:0002`, `S28DC:000A`. Dword 13h of `game_state`
    counts the passes, and each of the 15 timers that is due is carried
    out by `timer_fire`. The pass then waits until the count at 22Eh of
    the record behind `DATA:0092` has reached word 0Fh of `game_state`:
    4, the least length of a pass (presumably in ticks; who counts and
    who sets it back to 0 was not looked at).
  - Where the states are entered (2026-10-04; `view_step`, `S0CB6:0750`,
    and `field_menu`, `S0CB6:0EAA`, both read, nothing stepped, their
    callees not read). `game_loop` itself never leaves state 0 for
    another; it calls `view_step` in state 0 and `field_menu` in state 1.
    - `field_menu(view, side, input bits)`, state 1, the choice on the
      cursor's field. On entry (byte 18h 1) it makes word 1Ch of the view,
      what may be chosen: 2, 4 and 8 always; 200h unless 80h is set in
      `game_state`'s word 4; for a unit of the side without 2 and 200h in
      `unit_flags`, 10h with 1 in byte 16h of the view (and 80h clear in
      word 4) and else 1; 400h for a unit with 1000h or 2000h in
      `unit_flags` and for a ground with 80h, 200h or 40h in its word 0
      (with 8 in `game_state`'s word 2 only for a unit of the side, or a
      ground with 1 for the second side and 4 for the first). While the
      button is held a direction picks the pointer (byte 1Bh): up 4 (with
      10h) or 0 (with 1), but text 53h and nothing when the side's count
      of attackers has reached byte 16h of its record; down 2; left 0Ah
      (with 400h) or else 9 (with 200h); right 3; no direction 1. When
      the button is let go the pointer says what is done:
      - 4, moving: `S0B15:0371(the unit's field, the side's work buffer,
        unit, byte 0 of the unit, side, mask, map)` with the first half
        of a unit of two parts; a result above 1 goes to state 0 with 16h
        in byte 18h and the unit in word 1Eh, else text 54h. (The mask is
        FFFFh with 1 in the unit's word 6, else 6 or 3 by the unit's
        side. In the moving out of a site the fourth argument is 4 or 8;
        so presumably how far the unit goes, and byte 0 of a unit what it
        has left. Not checked.)
      - 0, attacking, three kinds of unit: with 4000h in word 6, text 4Ch
        when byte 0Ah is 0, else `S0E56:18A3(field, map)` and, if that
        gives something, the ten entries of `neighbours` marked in
        `field_bits` (4 or 8 by the side), else text 4Bh; with 2000h in
        word 6, the six fields of `field_neighbours` whose ground has
        8000h marked, text 6Fh for none; else `S0B15:062F` once for each
        of the kind's bytes 9 and 8 that is above 1 (with the kind's word
        6 less 10h, and less 2Ch), text 4Dh when both find nothing. With
        marks the view goes to state 0 with 13h in byte 18h and pointer 0.
      - 2 goes to state 4, 0Ah to state 6, 9 to state 5, 3 to state 3
        with 4 in byte 18h (the pointer 0Bh or 6 by the word at
        `S331F:0008`), anything else back to state 0.
      So byte 16h of a view is the side's part of the round: with 1 it
      moves units, with 2 it attacks, repairs and builds (its being
      `attack_pick` fits). `attacks_run` exchanges it between the views
      ("A round's end" below).
    - `view_step(view, input record, input bits, far pointer, side)`,
      state 0, by byte 18h: 0 a pressed button goes to state 1; 19h waits
      for byte 8 of the input record and then is 0; 16h, a unit that is to
      move: the button calls `S10E1:02EB(view, pointer, side)`, then 3,
      where the button calls `S10E1:05D1(view, side)` and
      `S10E1:000F(view, pointer, side, result)` and ends with 19h
      (presumably the aim and then its confirming; not read); 13h, a unit
      that is to attack: `S0AE0:0008(view, pointer, side)`, then
      `S0AE0:02E3(view, side)` and 19h. Each of the three has a second
      step (17h, 18h, 1Ah) entered with 400h in `game_state`'s word 4 and
      a timer of kind 7 over 4 passes: a second press within them (byte 5
      of the input record, or 8 in word 0 of the side's record) gives the
      move up (`S10E1:000F` with 10h) or the attack (effect 1), and when
      the timer has run out the step goes back (18h calls `S0B15:0371`
      again with nine words kept at `S3306:000A`). Byte 30h of the view
      is set when the button was let go before, so that a held button
      does not count twice.
  - A round's end (2026-10-04; the end of `attacks_run`, `S02E6:19B8` to
    `205B`, read, nothing stepped). This is where byte 16h of the views
    changes: no key does it. Nothing in `S066A` writes the byte but the
    setup (1 for the first view, 2 for the second), and here the two are
    exchanged. So a round has two halves: in each, one side moves and the
    other attacks, repairs and builds, both at once, and `attacks_run`
    after both have ended (state 5) carries the attacks out and turns the
    parts round. A is the side whose attacks were carried out (the one
    with 2 in byte 16h), M the other. After the fights:
    - with 1000h in `game_state`, `S02E6:2B87` is called with the
      routine's own six argument words, then `S2974:0002` and
      `S28DC:000A`, and 1000h is cleared (not read);
    - `rows_sync` (`S02E6:23F0`) for `carriers` (46h records),
      `sites_100` and `sites_400` (0Ah each) and `sites_40` (2): M's
      records get M's row copied to A's, all others A's row to M's. So
      the two rows of seven are not the two sides' units but each side's
      own state of the record, as each side has its own map; this
      corrects "the first row belongs to the first side" under state 2
      above (the code there takes the row of the side that looks);
    - every unit byte of A's map becomes FFh; each unit of M's side gets
      its `unit_pos` and its byte 0Fh for A set from those for M (so M's
      moves become true for A); `units_drop_empty` (`S02E6:2B28`) gives
      8000h to every unit with `unit_count` 0; then each unit without
      8000h and 4000h is written into A's map at its word 0Bh (the first
      word of `unit_pos` whichever side A is; not understood, after the
      step before the two words are the same only for M's units), and
      `map_copy` copies A's map to M's;
    - both views: bytes 17h, 18h and 19h 0 and the pointer 5 (this is
      what takes them out of state 5), byte 16h exchanged; byte 15h of
      both `sides` records (the count of attackers) 0; texts 35h and 36h
      (35h for the first side when its part is now 1, else for the
      second); 18h set in `game_state`'s word 4; `round_count`
      (`S331F:4CB5`, word 11h of `game_state`) goes up when the first
      view's part is 1 again, so once in two calls; `field_bits_clear`
      with FFh;
    - each unit: 0F00h cleared in `unit_flags`; with 1 in `unit_marks`
      and 1 in its kind's word 0Fh (and not 4000h in the flags)
      `S0E56:1DDB(unit, 200h, its side, 1)` and 200h set in the flags
      (not read; presumably a unit that has moved and may not act in the
      next half); with 8000h in its word 6 its byte 3 rises by 1 up to
      the kind's byte 4 when the kind has 4 in word 0Fh, else
      `unit_count` by 1 up to the kind's byte 3 (a unit that mends by
      itself), and for a carrier (1000h) of that sort the same for each
      unit in the row of seven of its record; then `unit_marks`,
      `unit_aim` and bytes 13h, 15h and 16h become 0;
    - who is left: the units without 8000h and without 20h and 4 in word
      6 are counted by side. A side with none left ends the battle: both
      views go to state 7, the loser's word 22h 12h and the other's 11h,
      15h for both when neither has one. So 11h in state 7 is a victory
      by the other side having no unit left (texts 41h and 42h), and 0Fh
      the one `attacks_run`'s result gives;
    - `defence_sum`'s result is kept at `S331F:4D2F`, and with 40h in
      `game_state` and a local flag set `S2F39:0F87` is called
      (presumably music; not read). The result is the byte at [BP-7].
    The F1 pair of the runs is then presumably what ends a side's half
    (pointer 9, state 5) and the key of entry 0Ch; not looked at.
  - What a unit reaches (2026-10-04; `move_costs` `S0B15:0A01`,
    `move_range` `S0B15:0371`, `attack_range` `S0B15:062F` and
    `buf_neighbours` `S0B15:005B`, read, nothing stepped). All three work
    in a buffer of 1104h bytes, one a field, 64 fields a row (as
    `field_bits` is).
    - `move_costs` writes what entering each field costs the unit as a
      negative byte, B0h for a field it cannot enter. The ground's
      6-byte record in `ww1gnds_dat`: byte 2 says which kinds may enter
      (a bit of the kind's byte 5 must be in it), byte 3 is the cost,
      byte 4 the cost for a unit with 10h in `unit_flags`; a unit with
      10h in its word 6 pays 2 everywhere it may enter. The mask
      `field_menu` gives (6 or 3 by the unit's side, none with 1 in the
      unit's word 6) closes the grounds whose word 0 has one of its bits
      (presumably the other side's sites). A field with a unit of the
      other side, or one with 2 in `unit_flags`, is closed, and the six
      fields around a unit of the other side are marked (9Ah or 9Bh). A
      field with an own unit keeps its cost, with a carrier of the own
      side (1000h or 2000h) it costs 2; a kind with 40h in word 0Dh
      cannot enter any field that holds a unit.
    - `move_range` puts the range (its fourth argument, byte 0 of the
      unit in `field_menu`) on the unit's field and spreads it over the
      neighbours, each getting what is left less its cost; a field
      beside a unit of the other side gets 0, so a unit can go next to
      an enemy but not past it. What was reached gets 1 or 2 in
      `field_bits` by the side. So byte 0 of a unit is what it may spend
      on a move, in the grounds' costs (this confirms the guess above
      as far as reading does).
    - `attack_range` marks with 4 or 8 the units of the other side less
      than the range away that the mask allows (the bits 3Ch of
      `unit_flags`, as in `fight_around`; 2 for a unit with 2 in its
      word 6), without the six fields around the unit when the mask has
      40h. `field_menu` calls it with the kind's bytes 9 and 8 as ranges;
      so those are presumably the ranges of the kind's two weapons, one
      more than the fields they carry.
    Not looked at: whether the bits 3Ch and the grounds' byte 2 are
    land, water and air, which the two costs suggest.
  - How an order is given (2026-10-04; `move_aim` `S10E1:02EB`,
    `move_start` `S10E1:05D1`, `move_done` `S10E1:000F`, `attack_order`
    `S0AE0:0008`, `S0AE0:02E3` and `unit_mark` `S0E56:1DDB`, read, nothing
    stepped; these are what `view_step` calls).
    - A move takes two presses. The first on a marked field (`move_aim`)
      finds the way: `S10E1:0F82(field, side, map)` may refuse the field
      with a text, `S0B15:0E52(work buffer, unit, from, to, side, map)`
      gives 0 when there is a way (text 48h when not) and leaves the
      way's fields as words at 1F40h in the side's work buffer (far
      pointer at 0Dh of its `sides` record), their number at
      `S331F:17F3`; only those fields stay marked, and `unit_aim` is the
      field. A unit with 10h in `unit_flags` cannot aim at a ground with
      4000h in its word 0. The second press on the same field
      (`move_start`) puts the unit on the side's list, sets 80h in
      `game_state`'s word 4 and starts a timer of kind 4 (presumably the
      unit's walk; `timer_fire` and what it calls for kind 4 were not
      read), `S331F:180F` being the unit. Neither routine was read:
      `0F82` and `0E52`.
    - An attack takes one press on a marked field (`attack_order`): the
      field goes to `unit_aim`, `unit_marks` gets 2, the unit's byte 13h
      9, and the unit is put on the side's list; the fight is at the
      round's end (`attacks_run`). The two special kinds of unit get
      other values in byte 13h and no place on the list: 0Bh or 0Dh for
      a unit with 4000h in word 6 (0Dh on the ground whose number is the
      byte at `S331F:4C92`, at most while the byte at `S331F:4CAE` is
      below 0Ah; presumably it builds something there), 11h for one with
      2000h. So byte 13h of a unit is its order. `S0AE0:02E3` afterwards
      clears the marks, takes 100h off the unit and draws the view.
    - So the list at 11h of a side's record, counted in its byte 15h and
      bounded by byte 16h (the menu's 4, 8, 10h or FFh), holds the units
      that were given an order in this half, movers and attackers alike;
      it is the number of orders a side has, not of attacks. (The names
      "attackers" above and `attack_pick` are from before this was
      known.)
    - `unit_mark` is what puts 100h (a unit being moved) and 200h (a
      unit that has its order) into `unit_flags`, with 40h/80h and
      10h/20h in `field_bits` on the unit's field; the round's end
      clears them.
  - The way (2026-10-04; `path_find`, `S0B15:0E52`, read, translated in
    `port/src/range.c` and compared on 60 calls of the six runs: the
    same; port/README.md, "Checked"). It goes over the fields
    `move_range` marked, trying them in the order of their worth: the
    distance to the end (`field_distance`) times the kind's byte 15h
    plus the ground's cost times the kind's byte 16h, so those two bytes
    of `unit_dat` say how a kind weighs the straight way against easy
    ground. It is not the cheapest way: what the way so far has cost is
    not counted. The way is left from its end back to its beginning,
    the unit's own field last; `path_len` (`S331F:17F3`) is the number of
    its fields. The six runs call it 10,237 times, 5,421 without a way
    (presumably the computer trying fields; not looked at).
  - The walk (2026-10-04; `move_step` `S10E1:072C`, `move_target`
    `S10E1:0F82`, `move_arrive` `S10E1:0AF0`, `order_undo` `S10E1:1514`
    and the small routines they call, read, nothing stepped; the comments
    in `src/HL.hints` have the details).
    - One unit walks at a time, the one in `move_rec` (`S331F:180F`).
      `move_start` starts a timer of kind 4; when it is due `timer_fire`
      sets 40h in `game_state`'s word 4 and `game_loop` calls `move_step`
      at the pass's end: the unit goes one field on along the way (from
      the list's end, `path_len` counting down), on its own side's map
      only, and the next timer runs over byte 17h of the kind's record in
      `unit_dat` passes: how fast a kind walks. Bytes 0Fh and 10h of a
      unit are the direction it faces on the first and the second side's
      map (0 up, clockwise to 5; `field_dir`). The field left gets back
      the unit byte it had (kept in `move_rec`), so a unit walks over its
      own side's units without wiping them out; the second half of a
      two-part unit follows onto the field the first half left.
    - `move_target` says what a move to a field would be, or the text
      that refuses it: 3 a free field, 2 into a carrier of the side, 5
      into a site of the side, 4 onto a site of the other side or of
      nobody. A carrier takes a unit when the unit's word 6 and the
      carrier's record's word 19h share one of the bits 0FC0h, a place
      of the seven is free and the load fits: byte 41h of a kind is what
      it weighs, byte 40h of a carrier's kind what it takes (the guess
      "load and room" under state 2 above, now read at its use). A
      carrier that goes into a carrier or site gives its own units over
      (`row_move`).
    - `move_arrive` ends the walk: the unit's byte 13h (its order) gets
      the kind, bytes 15h, 16h, 19h and word 17h what belongs to it (the
      record's number, the other unit, the place, the record's flags),
      `unit_marks` 1 and `unit_flags` 200h (4200h inside a carrier or
      site). A site of the other side is not changed here: presumably the
      round's end takes it over from byte 13h being 4 (in the part of
      `attacks_run` not read yet). Kind 1 (the mover takes the unit on
      the field into its own row) is written by nothing read so far.
    - `order_undo` takes all that back for a unit that is freed at the
      round's end (`attacks_run`).
    - Translated (`port/src/move.c`: `move_step`, `move_target`,
      `move_arrive` and what they call, with `timer_set` and `unit_mark`
      without their drawing) and compared on 89 calls of the six runs
      (`port/tests/move_test.sh`, dumps by `build/n2/move1.py` to
      `move3.py`): 41 of `move_step`, 48 of `move_target`, the same.
      The runs hold 2,012 steps, 974 of them a walk's end, and 18,607
      calls of `move_target` (most of them presumably the computer
      trying fields through `S0B15:13EC`). Every walk dumped ends on a
      free field: the kinds 1, 2, 4 and 5 of an arrival are read only,
      and `order_undo` and `row_move` were never called in the runs
      (port/README.md, "Checked"). A scripted move by the first side
      into a site and into a carrier would close that. The drawing and
      the text of a step change nothing in the game's state.
  - The orders carried out (2026-10-04; `attacks_run` from `S02E6:0E9C`
    to `19B8`, after the fights and the freeing of the destroyed and
    before "A round's end" above; read, nothing stepped). Every unit in
    use is taken by its byte 13h; A is the side whose attacks were
    carried out, M the one that moved (as above). Orders 2 and 3 need
    nothing more.
    - 4, onto a site of the other side or of nobody (M's unit; the
      record by word 17h and byte 15h as in `order_undo`): the site is
      taken. Music and pictures (`S2F39:1058`, A's view put on the site,
      `S131D:1426`, the picture CBh or CAh through `S1A39:0001`, the
      libraries loaded again by `S02E6:3484`, 1000h in `game_state`);
      then A's row of the site is copied to M's, the unit gets the first
      free place of it (with none, place 0, whose unit is freed); every
      unit standing in the site changes to M's side (its side bits, a
      new `unit_serial` from the kind's byte 3Dh or 3Eh, the sides'
      counts in byte 2, a carrier's record's side bits); the sides'
      count of the site's sort (byte 3 for `sites_400`, 4 for
      `sites_100`) goes over, the site's field gets the ground of its
      new owner from `amok_dat` (bytes 0 and 1 for `sites_400`, 3 and 4
      for `sites_100`, 6 and 7 for `sites_40`), the record's side bits
      become M and its byte 18h 5; the unit goes inside (F4h on the
      field, 4200h, `carrier_moved`). For a record of `sites_40` the
      routine's result becomes M: taking that site wins the battle,
      which is the result 0Fh of state 7 and settles that `sites_40` is
      the headquarters. So `amok_dat` holds the grounds of the three
      sorts of site by owner.
    - 5, a carrier into a site of its side: each unit in the site's row
      for M that has 4 in its word 6 is freed and its `unit_count` times
      8 added to byte 16h (17h for the second side) of the site's record.
      So a unit with 4 in word 6 is a load of goods, and those bytes are
      the site's stock (what text 25h shows). (The bound tested there,
      2710h, cannot be met by a byte sum; as the code has it.)
    - 1, the mover took a unit on its field into itself: for a unit that
      was freed meanwhile the place is emptied, else the unit changes to
      the mover's side as above. Still no writer of kind 1 found.
    - 0Bh, a unit with 4000h in word 6 (A's): `S0E56:1A97` gives four
      fields from `unit_aim` in `neighbours`; when all four have 8000h in
      their ground's word 0 and no unit, their grounds become the four
      bytes at `S331F:4C92` and the unit's byte 0Ah goes down by 1.
    - 0Dh, the same sort of unit: a new record of `sites_100` is made on
      `unit_aim` for A (`site_add_100`; its number the count at
      `S331F:4CAE`, which is why `attack_order` bounds it by 0Ah), with
      stock 0, the grounds of the three other fields from `S331F:4C97`
      on and of the first from `amok_dat`; byte 0Ah goes down by 1. So
      such a unit builds, and its byte 0Ah is what it has left to build
      with; `sites_100` is presumably a depot (not seen on the screen).
    - 11h, a unit with 2000h in word 6: `S0E56:1B57(its place, unit_aim,
      map)`, read (2026-10-07, Windows): from the place's and the aim's
      column and row, an index (the place's column parity times 36h, the
      aim's column less the place's plus 1 times 12h, the rows' the same
      times 6, and 0, 2, 4) chooses three column/row steps at
      `S331F:0059` and three grounds at `S331F:00C5`; each of the three
      fields whose ground has 8000h gets its ground, or 51h when the
      aim's ground is A5h or above (presumably taking the work back; that
      branch not seen run). Seen once on map 51 (unit 1F, kind 14, aim
      01FAh): 01FAh, 0228h and 022Ch of the map at `[BP-6]` went from 51h
      to A7h, AAh and ABh; side 1's map was not changed. There `[BP-2]`
      was 1 and DI 0, and `[BP-6]` was `map_ptr`, side 0's map: the map of
      the side whose orders are carried out, not that of `[BP-2]`, at
      least in this pass. Only kind 14 has 2000h in word 11h of
      `UNIT.DAT`; on map 2 such units start in a site with 2 in
      `unit_flags` (from the `.SHP` record's flag byte 02, presumably),
      and `game_loop`'s state 2 (`S066A:2E3A`) does not offer moving out
      a unit with 2.
    `S331F:4C92` to `4C99` lie inside `amok_dat` (36 bytes from
    `S331F:4C7A`; its bytes 18h to 1Fh), as does `blast_time` (byte 8):
    that file is a table of grounds and settings the program uses by
    place.
    Not translated: the part draws and waits in between; the state it
    changes could be translated apart, as was done for `move_step`.
  - The first side played by a key script (2026-10-07, Linux; scripts
    and dumps in ignored `build/n2/`, `build/n2/tools/keys.py` writes the
    key files). START and the right flag (Germany) give map 0 with side 0
    human (word 0 of its `sides` record 0, the other 2) on the left view;
    the left flag makes side 0 the computer's. Side 0 is driven by the
    keyboard: the arrows move its cursor (a tap under about 0.08 s is
    sometimes lost; past the view's edge the view scrolls and the cursor
    jumps two fields) and Space is the button. Word 0 of a view is the
    cursor's map offset (what `field_menu` reads), so a script's cursor
    can be checked in a dump. Button held, up, button let go: the move
    (pointer 4); Space on the aim and Space again: `move_aim` and
    `move_start`. Button held, left, let go on a field without 400h:
    pointer 9, state 5; then an F1 pair, sent after the other view is in
    state 5 too, makes the round's end (`attacks_run`); an F1 pair alone,
    or one sent before, changes nothing. The six long runs never did this
    for side 0.
    - Kind 5 (unit 00 into `sites_40` 0), kind 2 (unit 0D into carrier
      record 1) and kind 4 (unit 06, three moves later, onto
      `sites_100` 0 at (10,9), the one of its four fields whose ground
      has 100h; word 19h 0012h) were reached and compared
      (port/README.md, "Checked"). A unit moved into a site gets the
      site's word 0Eh as its place, without the +1 of a place on the map.
    - The loop of `attacks_run` from `0E9C` to `19B8` is translated for
      the orders 2 to 5 (`port/src/orders.c`): 2 and 3 go to `19AA`
      (nothing), 5 to `1071`, 4 to `11B1`; its jump table at
      `S02E6:206C`. Besides what the bullet above says, order 4 sets
      1000h in `game_state` and the word at `S331F:4CAA` to 1, puts the
      other side's view on the site (`view_to_field`), marks the site's
      field with 30h in `field_bits` while the view is drawn and clears
      it again, and writes 1 to `S3303:0021` before the picture. The
      side that moved is `[BP-2]` of `attacks_run`, its map `[BP-6]`,
      the other side DI.
    - Map 8 (RATIO) has a unit with 4000h in word 6 (unit 0C) and map 2
      (MOUSE) one with 2000h (unit 22, in a site at the start). The
      attack gesture with unit 0C called `S0E56:18A3` and showed text
      4Bh ("NOT POSSIBLE TO SET UP DEPOT"): no field around (18,5) was
      accepted. That routine was read only in part: presumably a field
      counts when its ground is the byte at `S331F:4C92` or its
      neighbours' grounds have 8000h and no unit (not checked). So 0Bh,
      0Dh and 11h are still not reached.
    - Building reached (2026-10-07, Linux; the rule found on the Mac:
      `S0E56:18A3` takes a field whose ground is the byte at
      `S331F:4C92`, or whose four fields (it, the one below, lower left
      and lower right) are on the map, have 8000h in their ground's word
      0 and hold no unit). Map 8, unit 0C (word 6 4300h, byte 0Ah 2)
      moved from (18,5) to (17,6) (field 0142h); its 200h was cleared at
      the half's end, so in the next half the attack gesture offered
      building: `field_bits` 4 at 1D0h only, the press on 0170h gave
      0Bh, aim 0170h, 200h, result 1, the text "DEPOT BEING SET UP" on
      the screen. The round's end (19B8 at 125.24 s): byte 0Ah 2 to 1,
      the grounds of 0170h, 0172h, 01A0h, 016Eh 15h, 18h, 16h, 17h (the
      bytes at 4C92 to 4C95). In the next build half the same press gave
      0Dh (ground 15h); the round's end (237.94 s): `S331F:4CAE` 1 to 2,
      record 1 of `sites_100` on 0170h with word 19h 10h and byte 18h
      0Ah, side 0's byte 4 0 to 1, byte 0Ah 1 to 0, the grounds 10h,
      14h, 12h, 13h (the last three from 4C97 to 4C99; 10h presumably
      `amok_dat` byte 3, not checked). Byte 13h stays 0Dh after the
      loop. Dumps (ignored `build/n2/o/`): `d3_attack_order`,
      `d4r_orders`, `d5_attack_order`, `d5r_orders`, entry and exit.
      `orders_run` does not carry out 0Bh and 0Dh yet.
  - `S0E56:1A97` and `1B57` read again and named (2026-10-07, Mac):
    `footprint` and `aim_grounds_set`, their tables `foot_steps`
    (`S331F:0049`), `aim_steps` (`0059`) and `aim_grounds` (`00C5`); the
    names the port proposed. The listing agrees with what the bullets
    above and `port/src/orders.c` say; each has only the callers named
    there (`S02E6:17C3` and `189E`; `S02E6:1986`). New from the tables:
    in the entries of `aim_steps` looked at (an even column; the aim
    above left, left below, above, below) the second of the three fields
    is the aim and the others are the aim's two neighbours to either
    side of the line from the place, so order 11h lays three fields in a
    row across the direction the unit works in, the middle one with a
    ground of its own; the entries for an aim that is no neighbour are
    all 0 (the place itself; `aim_grounds` 0 there too). The odd
    column's entries were not looked at one by one. What kind 14 builds
    there (the grounds A7h, AAh, ABh) was not seen on the screen.
    Nothing was run on the Mac for this: the dumps of `orders_test.sh`
    are not on this machine, so the port's test was only built, not run.
  - `S131D:1745` (`income_add`) is the round's income: it adds
    `S131D:1685` (`side_income`) to the side's word of `side_points`.
    `side_income` reads all ten records of `sites_100` and of `sites_400`
    with no test of use, takes those whose word 19h AND 3 is the side
    (an unused record has 8000h there, so it counts for side 0; its byte
    16h is presumably 0, not checked) and sums byte 16h plus the side
    (16h for side 0, 17h for side 1), then adds that byte of the side's
    record of `sites_40`. Translated (`port/src/orders.c`) and compared
    on four calls on map 8 (port/README.md); 24 and 57 a round there.
    On map 8 side 0 holds record 1 of `sites_400` from the start, and
    its depot (record 1 of `sites_100`) has byte 16h 0.
    Also on seven calls on map 0 (run s4), where side 0 takes record 0
    of `sites_100` and its income goes from 5 to 20. In 81 dumps of maps
    0 and 8 no unused record has byte 16h or 17h not 0
    (`port/tests/sites_scan.py`), so the unused records of side 0 add
    nothing there; other maps not looked at. Both sides' bytes 16h and
    17h were equal in every record seen (why there are two was not
    found). The dumps for this and the second side's arrivals are made
    by `port/tests/sites_dumps.sh`; kind 1 of `move_arrive` still has no
    known writer.
  - The computer's turn (2026-10-07, Mac; scratch scripts, logs and
    dumps in ignored `build/n4/`). `S1C74:000B` is `computer_turn(side)`;
    the names and what each routine does are in `src/HL.hints` (the block
    "The computer's player").
    - How it plays: through the hand a human has. `computer_queue` makes
      a list of up to ten steps (`computer_steps`, 7 bytes a step, added
      by `step_add`), `computer_step` turns the step at hand into a
      script of input codes (`computer_script`), and `computer_input`
      gives `game_loop` one word of direction and button bits a pass
      (`computer_out`, which `game_loop` reads into DI where a human's
      input from `S0CB6:05F1` would be). The codes: 0 to 9 nothing, the
      four directions, the button, the four diagonals (`script_bits`);
      80h to 83h the button held with a direction and then let go, 84h
      the button twice (`script_gestures`). So the code 80h is the
      gesture of a move the key scripts use (button, up).
    - The phases, stepped: every call of `computer_turn` and what the
      phase's routine returned was logged (`-log` at `S1C74:002C`, behind
      each of the five calls and at `01AE`; `build/n4/phases.py` sums a
      log up) in the long runs `w0` and `w1` (the computer's side 1;
      1500 s each) and in two new runs with the codes 24 and 30 of
      `build/n2/codes.txt` (`build/n4/k24`, `k30`, made by `gen.py` with
      mode FS, 1500 s), where side 0 is the computer's: so a computer's
      first side is reached by a map's code, and it goes through the
      same phases. Side 0 at the start, and side 1 from its second time
      on: `computer_assess` 0 five times and then 1, `computer_plan` 0
      some 320 to 1650 times and then 1 (15 s and 87 s of the game's
      time in `k24` and `k30`, 8 s in `w0`), `computer_queue` 0 5 to 81
      times and then 1. Side 1's first time, at the battle's start, is
      short (`computer_assess` and `computer_plan` 0 once and then 1).
      In the pass `computer_queue` returns 1,
      `computer_step` is called at once (0) and `computer_input` after it
      (1: no script yet). From there a pass is `computer_step` 0 and
      `computer_input` 1, or `computer_input` alone while a script is
      played (0, then 1 at its end). `computer_step` FFFFh (the list's
      end) leads back to `computer_queue`, which adds the next steps
      (about one unit's a time, by the counts of `step_add`: 41 and 48
      calls in 400 s). After the round's end the phase is 20h again
      (seen: the call after a `computer_queue` 0 is `computer_assess`);
      what sets it was not looked at (not `computer_turn`; `game_loop`
      sets 4 there, and `computer_reset` writes 20h).
    - The kinds of step seen in 400 s of `w0` and of `k24` (`-log` at the
      seven entries of the table at `S1E4B:0381`): kind 1 492 and 623
      calls, kind 3 4,916 and 4,333, kind 4 40 and 68 (first at 310 s and
      357 s), the kinds 2, 5, 6 and 7 none. Kind 1 is a unit's move
      (read: its handler `S1E4B:0484` goes through the cursor to the
      unit, the code 80h, the cursor to the aim, `move_target` and the
      button). Kind 3 is presumably the round's end at the headquarters,
      called every pass while the side waits (it gets word 0Eh of the
      side's record of `sites_40`). The kinds 2 and 4 to 7 are in the
      next bullet.
    - The steps, read and translated (2026-10-07, Mac; the handlers'
      names are the lead's, what each does by its byte 47h is in
      `src/HL.hints`). `computer_input`, and `computer_step` with
      `step_move` (kind 1), `step_attack` (2), `step_end_round` (3) and
      `cursor_script` (5), `cursor_scroll` and `cursor_step` are in
      `port/src/computer.c`, compared (port/README.md). Not translated:
      `step_unit_out` (4), `step_site_menu` (6), `step_build` (7); read,
      the first needs `move_targets_list`.
      - A step never acts on the game by itself but for `step_move`'s
        state 4 (`move_target` for the cursor's field, and 8 in the
        side's word 0 of `sides` when the field is not marked or the
        move is refused) and `step_attack`'s 8 there; everything else is
        codes for the hand. The cursor goes to a field in two ways
        (`cursor_script`): near (less than five columns and rows) a
        field a pass by `cursor_step`; far, the code 83h, the view
        scrolled by `cursor_scroll`, the button, then `cursor_step`.
      - `step_end_round` settles kind 3: the cursor to the side's
        headquarters (word 0Eh of its `sites_40` record), a row below
        it, the code 82h (button and left) once 80h is clear in
        `game_state`'s word 4, then it waits for 4 in the side's
        `computer_phase`, which `game_loop` sets after the round's end.
        That is what the many calls of kind 3 are.
      - Counts in 1500 s (`port/tests/computer_dumps.py`'s logs), the
        second side on the map of code 0 and the first on that of code
        24: kind 1 1,164 and 1,744 calls, kind 2 23 and 145 (so attacks
        come up, later than 400 s), kind 3 21,340 and 18,977, kind 4 83
        and 142, the kinds 5, 6 and 7 none; 97 and 135 lists ended.
      - The dump scripts are in the repository now:
        `port/tests/computer_dumps.py` makes the two runs' input (the
        same as `w0` and `k24` up to its end), the logs, the dumps and
        the test's arguments in ignored `build/computer/`. Of the
        scratch scripts in `build/n4/` only `phases.py`, which sums up
        a log of `computer_turn`, has no copy there.
    - The other steps (2026-10-07, Mac, later): `step_unit_out` (kind
      4) with `move_targets_list`, `step_site_menu` (6) and `step_build`
      (7) are translated too, with `unit_holder` and `rec_find_unit`;
      `computer_step` now carries out every kind.
      - `step_unit_out`: the cursor to the site or carrier that holds
        the unit, the code 82h (the site's list opens), the list's row
        to the unit, the code 80h, then `move_targets_list` gives every
        field the unit could go to and the first of them is taken (the
        columns from the left, each from the top; with none the site's
        own field and 8 in the side's word 0), the cursor there, the
        button twice, the code 83h. So the computer does not choose
        where a unit leaves to: it is the first field of the list.
      - `step_build`: the cursor to the `sites_400` record, 82h, the
        list's row to the last free place of the side's seven, 82h,
        down to the kind in `build_kinds`, 82h, 83h.
      - Both end by the code 83h (button and right), presumably leaving
        the site's list (not seen on the screen).
      - Which kinds come up where (24 runs of 1500 s, every third code
        from 1, scratch `build/n5/survey.py`): the 16 maps with a
        computer's side all have the kinds 1, 2 and 3; kind 4 on 12 of
        them (50 to 617 calls), kind 7 on 15 (48 to 178 calls); the
        kinds 5 and 6 on none, nor `step_attack`'s state 4 (a target
        whose field is not marked). The codes from 49 on are maps for
        two players: no call of `computer_step`. `computer_queue` adds a
        step of kind 6 at one place (the call of `step_add` at
        `S1CC6:0567`); when, was not read. Nothing adds a kind 5.
      - `computer_dumps.py` now logs the entry of every state of every
        handler (their addresses from the jump tables in the player's
        `HL.EXE`) and dumps three calls of each state, from four
        battles (the codes 0, 24 and, for the kinds 4 and 7, 1 and 25).
    - `computer_assess` read in full and translated (2026-10-07, Mac,
      branch `mac/computer-assess`; `port/src/plan.c`, compared on 75
      calls, port/README.md). What it is: the round's bookkeeping before
      the planning. Its five states (byte 5 of `computer_phase`): the
      two sides weighed against each other by `unit_worth` (word 9 of
      `computer_phase` 1, 2 or 3, presumably how careful to be) and the
      side's plans emptied; every unit of the other side given its
      worth as a target (word 6 of its record in `unit_plans`); then
      the side's list of aims (6
      bytes an entry behind the far pointer at byte 9 of the side's
      record of `sides`, counted in byte 6 of `computer_phase`): the
      other side's headquarters and every site that is not the side's
      (kind 1), the fields around the own headquarters (kind 3) and
      units with 4 in word 6 and 2 in `unit_flags` (kind 4).
      - The plans: `unit_plans` (`S39B6:09AB`) and `unit_plans2`
        (`S39B6:0132`), 9 bytes a unit; a side uses the second with 2
        in its `attack_pick`, the first otherwise. So `attack_pick` (1
        or 2) tells the two halves of a round apart for a side, and
        each half has its own table of plans; code of one half writes
        the other half's table directly in places (what a unit is to do
        in the half that follows).
      - `kind_worth` (`S39B6:0000`), 6 bytes a kind of unit: byte 0 its
        worth as a target, word 2 the bits the computer picks kinds by.
        Its bytes come from the computer's file of the map, presumably
        (`computer_reset`; not checked here).
      - Slips of the original kept as they are: the fields around a
        unit near the headquarters are taken of the halved map offset
        (state 1), and `plan_to_site` takes its distance of map offsets
        not halved.
      - Not seen in a run: a unit with the plan 9 or a carrier with a
        unit to bring back in state 0, a unit listed in state 4.
    - `computer_queue` read in full and translated (2026-10-07, Mac;
      `port/src/plan.c`, port/README.md for what was compared). It
      serves the units' plans (byte 8 of a unit's record in
      `unit_plans`: the kinds are listed in `src/HL.hints`) in two
      passes: first in the order of the units those that are 3, 4, 0Ch
      or 0Eh (what `computer_plan`'s attacks and `plan_ride` wrote),
      then the rest by their worth, highest first. A plan becomes one
      step (`step_add`), and the routine returns 1 so that the step is
      carried out before the next plan is looked at; then building,
      then the step that ends the round.
      - The two halves of a round: with 1 in the side's `attack_pick`
        the units in the side's sites get the plan to leave
        (`plan_units_out`) and nothing is built; else those that are
        not whole get the plan of a repair (`plan_repairs`) and state
        6 builds.
      - What the lead asked about. `S1CC6:07B7` is the default of state
        5's table: a plan of a kind that has no step there (3, 4, 8,
        0Ch) is given up. The call of `build_list` with the points less
        3Dh is not there but in state 6 (`S1CC6:07FC`, the building):
        the computer keeps 3Dh points back, takes a kind at random
        among those `build_list` offers that have 400h in word 2 of
        `kind_worth`, and queues a step of kind 7. The `step_add` at
        `S1CC6:0567` queues a step of kind 6 for the plan 6, which only
        `plan_repairs` writes: so the step of kind 6 (`step_site_menu`,
        the code 81h, button and down) is the computer's repair. It did
        not come up in any run: `plan_repairs` needs a unit below its
        full count inside a site of the side in the half with 2 in
        `attack_pick` and more than 0Eh points (not looked at which of
        these fails; presumably the units do not get into the sites
        while damaged, since the plans 9 never came up either).
      - A move does not go to the plan's field at once: `plan_move`
        finds the whole way (`unit_reaches`, three tries with fewer
        fields closed each time), then what the unit can reach this
        round (`move_range`, `move_targets_list`), and queues a move to
        the reachable field that is furthest along the way.
      - Random numbers: `rand_next` (`S2C9A:0008`) on the four words at
        `DATA:0AF1` that `clock_seed` sets, through `rand_range`
        (`S0CB6:0713`). The building is the first place found that
        draws from it (the fights have their own, `rand_seed`).
    - `computer_plan` read in full and translated (2026-10-07, Mac;
      `port/src/plan.c`, port/README.md for what was compared), and
      `computer_turn` with it (not compared). The whole chain from the
      assessment to the input word is in the port now. What the plan
      is, in the order of its states (byte 4 of `computer_phase`; the
      details in `src/HL.hints`):
      - Nothing is planned in the half of a round with 2 in the side's
        `attack_pick`: that half's table (`unit_plans2`) was filled
        while planning the other.
      - The aims of `computer_assess`, kind by kind and the worthiest
        first: a site to take gets the unit with the shortest way among
        those whose kind has 1 in word 2 of `kind_worth`; a field at
        the headquarters gets a unit with 40h there; a unit to go with
        (an aim the first adds for kinds with 20h) gets followers; a
        unit to fetch gets a carrier (`plan_ride`).
      - Units that are not whole go to a site of the side (the plan 1,
        or a carrier by `plan_ride`).
      - Units that fire from afar (2 in `kind_worth`) stay and get the
        attack on the worthiest unit in reach for the other half.
      - The attacks: target after target by its worth
        (`computer_assess`'s word 6), up to four units are put on
        fields around it, opposite one another where that can be, and
        attack it in the other half.
      - Whoever has no plan then goes towards the worthiest target it
        can attack, else to where the nearest unit with a plan goes,
        else home by carrier.
      - The bits of word 2 of `kind_worth` as the planning uses them: 1
        takes sites, 2 fires from afar, 1Ch matched against bits of
        the `unit_flags` of a unit to go with, 20h wants company, 40h
        holds a field, 380h a carrier (never planned as a fighter),
        400h is built by the computer. Names for them are guesses.
      - Not seen in a run: the aims of kind 2 and 4 (so no unit with
        20h in `kind_worth` took a site, and no unit was listed to
        fetch), and with them the plans 7 and 0Ah.
      - Left open: `game_loop`'s loop, which would call
        `computer_turn`, is not translated; the dumps of
        `computer_plan` are early ones (150 s), so the attacks with
        several units around a target late in a battle are thinly
        covered.
    - `computer_turn` run against the original, call by call and over
      whole halves (2026-10-07, Mac; port/README.md for the counts).
      What was learned on the way:
      - A battle in which the human only ends its halves runs about
        ten times as fast on the runner as one with random input and
        gets further into a battle; that is where the late paths were
        found. Two F1 keys alone do not end a half: the script holds
        the button, taps left on a plain field and releases, then F1
        twice, every 8 s (`play_quiet` in
        `port/tests/computer_dumps.py`).
      - `game_loop` calls `computer_turn` again and again without
        drawing while the side thinks (the phases 20h and 40h), yet a
        few bytes of the game's state still change between those
        passes that `computer_turn` did not write; so even the thinking
        is compared loosely (what the port wrote must be the same).
        What writes them was not looked at.
      - While the round's end waits (`step_end_round`, entered at
        `S1E4B:09C6`) `computer_turn` is called pass after pass and
        only the step runs; the round's end is seen at `S1E4B:09EB`.
      - Where the rarer paths come up with an idle human (the code's
        index, the first time): the plan 6 and the repair step on 26
        (251 s) and ten more maps; the plans 9 to 0Bh on 8 (627 s), 0,
        11, 32, 37; the plan 0Ch in the queue's state 3 on 8, 0, 11,
        32; `plan_ride`'s state 6 on 11 (184 s) and thirteen more;
        `step_attack`'s state 4 on 45 (191 s), 6, 8, 9, 12;
        `computer_plan`'s state 4 on 22 only (from 56 s).
      - Reached on no map (48 quiet for 1200 s, ten with random play
        for 2000 s): `computer_plan`'s states 2 and 6, `plan_approach`,
        `plan_to_site`, the plan 7 at the queue, the step of kind 5.
        State 2 only strikes out an aim whose kind is none of 1 to 4,
        presumably never the case; state 6 and `plan_to_site` need a
        unit listed to fetch (4 in word 6 of its record), which no map
        showed; why the plan 7, which state 4 gives, never reached the
        queue on the code 22 was not looked at. These stay translated
        from the listing only.
  - A side's part in a site (2026-10-07, Linux): building, repairing
    and moving out are not routines of their own but parts of
    `game_loop` (repairing `S066A:3156` to `32A9`; building sets 4600h
    in `unit_flags` after the label `S066A:3890`; moving out in state 2,
    step 0Bh). `S131D:14D4` is `build_list(points, side)`:
    `build_kinds` all FFh, `build_count` 0; nothing more without a free
    unit record (`S15C5:01A7`, `unit_find(8000h)`); else every kind whose
    byte 3Fh is at most the points and without 2 in word 0Fh, with 1000h
    in word 0Dh only while `carriers` has a free record, not the kinds
    below 0Ah for the second side nor above 23h for the first. It runs
    when a site is entered in the attack half (state 2, step 9) and after
    a repair; it also ran once at t=208 in the map-0 run s4 with no site
    entered (caller not looked for). Repairing finds the unit from the
    record at view+24h (row side*7, place in word 0Eh of the view). Both
    translated (`port/src/side.c`) and compared on few calls
    (port/README.md). For key scripts: a half is ended with the button
    and left on a plain field (pointer 9) and F1 twice; on a site's field
    the same gesture enters the site. A direction let go 0.02 s after the
    button did not take in one case (k4's move); 0.2 s works. The halves'
    times depend on the computer's turn, so a fixed script over many
    rounds drifts. The headquarters (`sites_40`, word 19h 4) cannot build
    (building needs 8 there), so building is to be scripted at a record
    of `sites_400` of the side (map 8: record 1), not done. Next:
    building and moving out, and a repair carried out (a damaged unit
    and 0Fh points).
  - Building (2026-10-07, Linux): `S066A:387A` to `3A42` builds the
    kind at `build_kinds`[word 0Eh + word 20h of the view] on the place
    in word 0Ah; the new unit's side is bit 0 of the site's word 19h, the
    price is taken from the side whose view it is. Translated
    (`build_unit`) and compared on one build on map 8 (port/README.md).
    Map-8 key script notes (`port/tests/keys/make_m8.py`): taps must be
    held 0.08 s and spaced about 1 s, else some are lost or the cursor
    jumps; right/left on row 3 step two map columns from column 13 on;
    the first side's view starts at map offset 0136h+30h (row 7, col 11);
    4 up and 4 right reach B2h, a field of `sites_400` record 1. A round's
    end waits for a key once the computer is done, so F1 is pressed every
    5 s in the windows where the first side waits; any key pressed later
    starts it instead, which shifts all later times. `build_list`'s other
    caller is `S1CC6:07B7`, an entry of the computer's nine-entry dispatch
    at `S1CC6:0A23` (it passes its DI less 3Dh as the points and SI as the
    side); not named here.
  - Repairing in the place menu is offered (100h in word 1Ch of the
    view, just before `S066A:2EA9`) only in the attack half for the unit in word 1Eh
    of the view whose word 6 has no 4; not for a carrier (word 22h 2
    clears 100h and 80h). The run site's fifth round repairs its built
    unit: 50h, as the port. Moving out (`S066A:2F52` to the view's step
    16h) calls `S0CB6:0445`, `view_to_field`, `move_range`,
    `S0E56:0D44`, `0F36` and `145A` (not read), and it is offered only
    in the move half (10h in word 1Ch) to a unit without 100h, 200h and
    2 in `unit_flags`: a unit built (4600h) cannot move out in the half
    it was built in. Translated (`move_out`) and compared on four calls
    (port/README.md): `S0CB6:0445` is `view_copy` (the view to
    `S331F:4D61`); `S0E56:0D44` (`view_draw`), `0F36` (`marks_draw`)
    and `145A` (`field_draw`) only draw, as far as read (names guessed).
    The computer's moves out go through the same block of `game_loop`
    (40 calls before the first side's in the run site), with `game_loop`'s
    frame (BP 0FEAh). `S331F:17AE` is the far pointer to the map the move
    is made on (`map_cur` in side.c, PENDING LEAD NAME). `S1CC6:07B7`
    was not named: its role beyond calling `build_list` was not read.
  - A repair carried out (51h) on map 8: the builder (unit 0Ch, kind
    16, word 6 4300h) after building the depot stands at map offset
    143h beside it with byte 0Ah 0; moved into the depot (one step down
    from where the cursor stood reaches the depot's field 170h; the
    field right of it is one of the depot's four fields, where the end
    gesture enters the site) and repaired in the following attack half: 51h, 32h
    points (72 to 22; byte 0Ah 0 to 2, 0A00h in `unit_flags`). A step
    up or down moved the cursor by one row of `map_width` in some places
    and by two in others (from row 5 down to 7 and 9 here); why was not
    looked at, so a script's cursor is checked by dumping the view's
    word 0. The button with right on a plain field shows the field's
    information (state 3).
  - Building, repairing and moving out in the loop's passes (2026-10-08,
    Linux): the port's `build_unit`, `repair` and `move_out` were run
    in whole runs of `loop_test` (`m8_site`, `m8_long`, `m0_carry`, and
    new dumps of `m8_d5r` without fight pictures, `m0_s2c`, `m0_hq`), and
    the state agreed at every pass compared; repairs with texts 4Ah, 50h
    and 51h and a move out of a carrier are among them (port/README.md).
    `m8_d5r` with fight pictures on cannot be dumped: `rand_next` is
    called by other code than `rand_range` in pass 2265 (presumably the
    pictures, `S2532`; not checked). Still not reached: a scrolled build
    list, a carrier's kind built, a human second side's build or repair,
    a repair of a damaged unit without 4000h in word 6, and
    `step_site_menu`.
  - Moving out of a carrier: none of the computer's moves out in the
    map-8 and map-0 runs left a carrier (`S066A:2FA7`, the step back
    for 20h in the record's word 19h, never hit); the run `m0_s2c.keys`
    does it for the first side (port/README.md), the same as the port.
  - The human side's input (2026-10-07, Linux): `S0CB6:05F1` is
    `side_input(input record, view)`, called by `game_loop` at
    `S066A:122D` for a side that is not the computer's (the computer's
    word comes from the other branch). The record's five bytes are, by
    the keys of the scripts, left, right, up, down and the button: 4, 8,
    1, 2 and 10h in the word (consistent with the gestures: down gives
    2, which the place menu turns into repairing, left 4, building). The
    record is cleared on each call. The directions are reported only
    when byte 2Ch of the view (a repeat counter, counted down each call)
    is 0 or they differ from word 2Eh (the last word); then 2Ch is set
    from the delays at 28h (1, 2, 1 in the map-8 dumps) by the stage in
    2Bh (0 after a change, up to 2). So a key held over more calls than
    the delay is reported again: calls 4050 and 4051 of the run
    `m8_d5r.keys` both gave 11h for one held up. This is the cursor's
    odd step: a tap held 0.08 s spans one or two passes of `game_loop`
    (about 16 a second while the computer thinks, more otherwise), so
    it moves the cursor once or twice, rows and columns alike (the
    m8 scripts saw rows 5, 7, 9 and columns 13, 15, 17); the half field
    of an odd column does not enter into it. A key script that wants
    one step holds the key for less than a pass or checks the view's
    word 0. Translated (`side_input`) and compared on seven calls.
  - `view_step`'s callees (2026-10-07, Linux): `move_start`
    (`S10E1:05D1`) is used by the computer as well (49 calls in the run
    s4, 63 in d5r up to 260 s). Translated and compared (port/README.md).
    `S0AE0:02E3` is `attack_unmark` (named, translated, compared on three
    calls). `attack_order` is translated and compared on three calls;
    `show_message` writes state too: 4 (8 for the second side) in
    `game_state`'s word 0 for a text, cleared by `timer_fire` (so the
    port has its state part). `move_done` is translated and compared on
    five calls, all with flags 1. Its giving up of a move out of a site
    or carrier (flags 10h, view step 3) writes F4h and the record's byte
    1Bh into the map at the unit's place. (This said first that the write
    went through the units' segment: a misreading, `MOV ES,DX` follows
    `MOV BX,AX`; the comparison of `view_step` showed it.) `move_aim` is translated
    and compared on five calls (all finding a way); `S0E56:1217` is
    `marks_erase` (draws, then clears the mask). `field_menu` is
    translated and compared on 23 calls (port/README.md). It has two
    details the 2026-10-04 reading did not say: the two calls of
    `same_side` take their arguments in opposite orders (`(side, flags)`
    for the unit's own part, `(flags, side)` for a carrier), and for a
    unit with 4000h in word 6 the attack's fields come from
    `S0E56:18A3`, now `depot_places`: of ten fields around the unit (the
    steps at `S331F:0021`, `depot_steps`), those whose ground is
    `amok_dat`'s byte 18h or whose four fields (`foot_steps`) all lie on
    the map with a ground with 8000h and no unit. So the builder's
    "attack" is the choice of where a depot goes (not compared: no run
    had one). Releases are found with `run.py -log S0CB6:106C` beside
    the entry: the call before each hit is one with the button let go.
    `view_step` is translated and compared on 15 calls; its steps are as
    the 2026-10-04 reading says. Its result is only AL (AH is left as it
    was). The refused aim (move_aim's 12h has 10h) is what leads to step
    17h: a second press within the timer's four passes gives the move
    up, otherwise the view goes back to 16h for another aim. Steps 18h
    and 1Ah were not reached in the six key scripts.
  - A pass of `game_loop` (2026-10-07, Linux): `S066A:0EE9` to `45D7`
    read again for what changes the state and translated
    (`port/src/loop.c`), with `attacks_run`'s frame around the orders
    (`S02E6:000F` to `0122` and `0D48` to `0E9C`, `19B8` to `205B`,
    `rows_sync`, `units_drop_empty`; `port/src/orders.c`). Compared over
    the first round of `m8_site` (port/README.md). Learned on the way:
    - The key of a side's part is read at `S066A:0FE6` to `1003`, before
      `1006`, into `[BP-3Bh]`, which only the pass's beginning and the
      round's end clear: a key read in the first side's part is still
      there in the second's. In `m8_site` the F1 pair's first F1 is read
      in pass 291 by side 0 and runs the round's end; the second is read
      by side 1 in the same pass and does nothing there.
    - Byte 8 of a human's input record is written FFh by the timer's
      interrupt (`S27FE:0326`) whenever the record's button (byte 4) is
      not held: so it means "the button is up", and `view_step` clears it
      when it has used it. Byte 5 was not traced (presumably a press, as
      the 2026-10-04 reading says).
    - The computer's turn (`S1C74:000B`) calls `move_range`,
      `attack_range`, `field_neighbours` and `move_target` while it
      plans (logged in `m8_site` between the computer's `S066A:0FC3` and
      `1243` from pass 292 on: 15, 6, 186 and some 360 calls), so the
      way (`S331F:17F2` on), `neighbours`, `move_rec` and `move_args` hold
      what its planning left; `S3AEC:0002` is written by code in `S2138`
      (`computer_queue`). `computer_step` sets 8 in its side's word of
      `sides` (`S1E4B:0610`, `063C`, `0886`, `0C78`), which `view_step`
      takes as a second press.
    - The computer's side used state 3 (the small map, step 5) in 124
      passes of the first round; why was not looked at.
    - `LIB\BIGUNIT.LIB` is loaded during play (from 83 s in `m8_site`,
      several times), presumably to draw a unit's picture; it changes
      `name_buf`.
    - In `m8_site` `attacks_run` runs in passes 291 (48.41 s) and 1939
      (111.43 s) without fights; its third call (150.53 s) has the first
      fights (`fight_percent` at 151.04 s).
    Not translated in the pass: the keys F2 to F9, Esc and saving, and
    what draws, sounds and waits (the fights, order 1, `order_undo` and
    `S02E6:2B87` followed, see the next bullet); the computer's player beyond what comes in as input
    (it followed too: "The pass with the computer's own turn" below).
  - The pass with the computer's own turn (2026-10-08, Mac):
    `port/src/loop.c` calls `computer_turn` for a computer's side
    instead of replaying the original's log; 400 s of `m8_site` are the
    same as the original at 29 passes, the computer's state included
    (port/README.md has the counts). Learned on the way:
    - The computer's part of a pass (`S066A:1020` to `1190`): one call
      of `computer_turn`, the side's word of `computer_out` as the
      input and its second word as the text. With 80h in `game_state`'s
      word 2 it first clears byte 4 of both input records and word 4
      behind `DATA:00BE`, shows the text 64h and then calls
      `computer_turn` again and again inside the one pass while 20h or
      40h is in the side's `computer_phase`, until one of those three
      (a human's button, a key) is set; then both texts are taken off,
      0Ch cleared in `game_state`'s word 0 and the 80h cleared. State 5
      of a human's side sets the 80h when the other side is the
      computer's and has 20h or 40h in its phase (`S066A:40EA` to
      `414E`): so a human who has ended the half lets the computer
      think through at once. In `m8_site` that is passes 671, 3088 and
      5017 with 60, 238 and 336 calls, none ended by a key.
    - What ends such a loop early comes from the interrupts, so the
      number of calls is an input of the pass beside the keys (the
      test stops at the original's number).
    - The random numbers are a second input: `clock_seed` with 0 takes
      the BIOS's count of ticks, and besides the battle's setup the
      fight's pictures call it (`S2532:0008`; once a fight, twice in
      the round's end of pass 2615 and five times in that of 4543). The
      only reader seen is `computer_queue`'s state 6, which draws the
      kinds to build (`rand_range`: five calls in pass 4394, three in
      6454). With the seed left out the port drew another third kind
      in pass 4394; with it everything is the same.
      For the port's program this means: seed from its own clock where
      the fight's pictures would be, or the same battle is not played
      differently for that. `S2532` has calls of `rand_next` of its own
      (`S2532:183B` on) that did not come in the run; if they do, the
      numbers the computer draws depend on the pictures.
    - What the planning leaves between passes and HANDOFF had listed
      as counted apart is now the same byte for byte at every pass
      compared: `move_args`, `S331F:17F2` to `181B`, `S3AEC:0000` to
      `0007`, and the computer's segments `S39B6` to `S3AEB`
      themselves; `rand_words` was added to them.
    - `S1C74:0354`, called after the turn while the computer thinks
      and the side's `attack_pick` is 1, counts the side's units and
      plans for a bar drawn by `S1571:01F2`; presumably it writes
      nothing of the state (read to the first loop only, not checked
      to its end; the comparisons show no difference from it).
    Open: a loop ended early by a key, a computer first side in the
    pass (the single calls of `computer_turn` covered side 0, the pass
    did not); the pass is now run by the program too (the next bullet).
  - The port's program plays a battle by itself (2026-10-08, Mac;
    `port/src/play.c`, `port/src/input.c`, the options `-keys`, `-clock`,
    `-seed`, `-passes`, `-dump` of the headless program). From its own
    setup (`battle.c`) it runs the passes of `loop.c` with a key script
    as the human's input; started so and run by the original's clock it
    has the same game's state and computer's state as the original at
    every pass compared on three key scripts (`m8_site` 400 s, `m0_hq`
    and `m8_d5r` 300 s; port/README.md has the counts). Learned on the
    way:
    - The setup's differences. Of the game's state (`S3306` up to
      `DATA`) the port's own setup differs from the original's first pass
      only in what the routines left out fill: the records of the
      libraries of pictures and their far pointers (`S331F:013B` to
      `020B`, `02D7` to `05A2`, `179A` to `17A5`, `185A`; `S37FF:0012` to
      `007E`), a letter in three names (`0272`, `027B`, `0284`: `W` in
      the original at map 8's first pass, `T` in the port; who writes it
      was not looked at), the map's number as text (`177D`),
      `S37FF:011E` and the code typed in the menu (`0129`), and
      `S3CFE:0006`: 772 bytes on map 8. None of them is read by a pass
      as far as translated, but one was missing and is now set:
      `S1650:0003` (called once at the start; it loads a file and shows
      something, not read further) puts the far pointer to the table of
      keys `S331F:0292` at `S331F:02D3` and twelve of the table's entries
      at `02C7` (entries 8, 8, 9, 0Bh, 0Ah, 6, 1Ah, 1Eh, 1Ch, 1Bh, 1Dh,
      1). Without the pointer no key of a pass is known, and the F1 pair
      did not end the round (first seen as a difference in pass 292 of
      `m8_site`). Outside the state some 187,000 bytes differ at the
      first pass (pictures, the screen's and the sound's data, the
      interrupts').
    - The keyboard (`S27FE:04F4`, the interrupt 9 the program sets; it
      passes nothing on to the BIOS): the byte read goes to `DATA:0B2B`,
      and the key's bit in the 16 bytes at `DATA:0B1B` (80h shifted right
      by the code's low three bits, in the byte of its upper four) is
      cleared for a key down and set for one up.
    - The timer (`S27FE:06D2`, interrupt 8) shares its counts out among
      routines with their periods (`S27FE:0783` adds one); in the battle
      there are four: the BIOS's old interrupt (10000h), `S27FE:008C`
      (3FFFh: 72.8 times a second), and two of `S30EF` (`S30EF:192D`
      with 06A9h and `S30EF:12D9` with 1D1EBh; presumably the sound's).
    - `S27FE:008C`: counts the word at `DATA:0308` (22Eh of the record
      behind `DATA:0092`; a pass sets it to 0 at its beginning and waits at
      its end until it is at least `game_state`'s word 0Fh: so a pass
      lasts at least four calls, 55 ms, and need not wait with the word
      0, which a human's side that has ended its half sets while the
      computer plays); a key down in `DATA:0B2B` goes to the record
      behind `DATA:00BE` (`DATA:045C`: byte 0 the scancode, word 4 FFFFh
      until `game_loop` has read it) and `0B2B` becomes 0; and on every
      fourth call (`DATA:0B19`) the two sides' records are filled
      (`S27FE:0175` and `020E`): what the side's table of keys (the word
      at `DATA:0B2C` or `0B2E`; seven bytes a key, the scancode and what
      it gives left, right, up, down and the button) finds held goes
      into the five bytes at `DATA:03BC` or `03E8`, the directions from
      there into bytes 0 to 3 of the record (`DATA:03C8`, `03F4`: what
      `DATA:00A6` and `00B6` point to), FFh into byte 4 for the button
      and into byte 8 whenever it is up; byte 5 becomes FFh for a second
      press within 19h such calls (bytes 6 and 7 keep how far that has
      come; the counts of holds and gaps are five pairs of words at
      `DATA:03D2` and `03FE`). The first side's keys are the arrows and
      the keypad's corners with Space and Enter as the button, the
      second's X, V, D or F, C with the left Alt and Ctrl. With a
      joystick switched on (`DATA:0384`, `0386`) the directions come from
      it instead, and the mouse (`S27FE:03D4`) adds to the record the
      word at `DATA:0418` names; both are not translated (the mouse rests
      in the middle in the scripted runs and gives nothing).
    - `S0CB6:0036(n)` is the routine that waits: it waits for the first
      side's button and the key to be gone, then up to n in steps of
      three (`S0CB6:0001`), clearing byte 4 of the first record and the
      key's word 4 at each step, until one of them comes; it returns 1
      for the key of entry 0 or when the time ran out. The fight's
      pictures clear the key too (`S2532:0131`). So a key pressed during
      a round's end with fights is gone: in `m8_d5r` the second F1 of the
      pair in pass 2265 never reaches the second side's part (in
      `m8_site`'s pass 291, without fights, it does).
    - `S2532` does call `rand_next` itself in a fight's pictures after
      `clock_seed` (11 calls in `m8_d5r`, none in `m8_site`): the numbers
      the computer draws later depend on the pictures.
    - `move_step` shows the text `move_arrive` returns
      (`show_message`, `S10E1:0A68`), which sets the side's bit in
      `game_state`'s word 0; the port's left it out (seen in `m8_d5r`,
      pass 431, where the first side's unit arrives; the timers then
      took other places). Added.
    - What still has to come from a log of the original for the same
      battle to be played (`port/tests/play_dumps.py` writes it as the
      clock's file): (1) the times of the timer's routine, of the
      keyboard's interrupt and of the places where a pass reads what
      they left (a side's part, `side_input`, `view_step`, each turn of
      the thinking loop, `attacks_run`), because the original's passes
      last as long as its drawing and the port draws nothing; with the
      times the port finds the input words, the keys, bytes 5 and 8 and
      the number of calls in the thinking loop by itself (they were
      inputs of `loop_test`); (2) the numbers `clock_seed` starts the
      random numbers with (the BIOS's count of ticks at the setup and in
      each fight's pictures) and the calls of `rand_next` by the
      pictures; (3) where the waits and pictures of a round's end clear
      the button and the key. No game state is taken from the original.
    - On the port's own clock (a pass lasts as many calls of the timer's
      routine as the original waits for, at least one; nothing else took
      time then) the same keys gave the same state up to pass 291 of
      `m8_site` and another cursor field by pass 400; the next bullet
      says why and what was done.
    Not done: nothing is drawn or sounded, the keys F2 to F9, Esc and
    saving stop the play ("not translated"), no battle was played to its
    end, the mouse and joysticks, a thinking loop ended by a key (none of
    the three scripts has one), a computer first side.
  - The port's own clock, and what still comes from the original's log
    (2026-10-08, Mac; `port/src/play.c`, `sh port/tests/play_test.sh TAG
    own`). `S27FE`'s routines, `S0CB6:0036` (`wait_input`), `S1650:0003`
    (`start_setup`), the two waits for the screen and the two fades are
    named in `src/HL.hints` now, with what was read of each.
    - Why pass 400 differed: a round's end takes time in the original
      and took none in the port. `game_loop` calls `fade_grey`
      (`S0E56:21D8`) before `attacks_run` and `fade_back` (`S0E56:1F91`)
      after it, each 60h steps with a wait for the screen's vertical
      retrace (`retrace_wait`, `S2945:000A`): 192 retraces, 2.74 s at the
      runner's 0.014268 s a retrace. In `m8_site`'s pass 291 the log has
      96 calls of `retrace_wait` before `attacks_run` and 96 after it,
      and the pass lasts 2.81 s. The script's second F1 (47.8 s) comes in
      that time and is read by the second side in the same pass, where
      it does nothing; in the port the pass was over at once and the F1
      came to the first side in pass 306 (what it did there was not
      followed; by pass 400 the cursor's field differed).
    - Done: the port's own clock is a line of time on which the timer's
      routine is due every 3FFFh counts and a retrace every 0.014268 s;
      the round's end waits its 60h retraces before and after
      `attacks_run` when no site was taken (`site_taken` 0). With that
      the second F1 is read as in the original and pass 400 of `m8_site`
      is the same.
    - How far it goes now (the game's state and the computer's, at the
      passes dumped): `m8_site` the same at 50, 200, 291, 292, 296, 386,
      400 to 404; at 387 five bytes differ for one pass (the up of 55.0 s
      is read in pass 386 by the original and in 388 by the port), and
      from 405 on some bytes differ at every pass dumped (presumably
      the cursor's field: the ups below; the bytes were not looked at).
      `m8_d5r` the same at 50 and 200 and one or two bytes different at
      400 and on, `m0_hq` the same at 50 and different at 200 (the parts
      first read something else in pass 101).
    - Why it cannot go further without drawing: `input_tick` samples the
      keys on every fourth call (54.9 ms) and a pass lasts four calls, so
      a tap of 0.08 s (5.8 calls) is found in one sample or in two by
      where it falls between them, and `side_input` reports a direction
      held over two calls twice. In `m8_site` the four ups from 55 s move
      the cursor 1, 2, 2 and 1 fields in the original and 1, 1, 1 and 2
      in the port. Where the samples fall is the original's time: its
      passes last longer than four calls whenever it draws more (by
      their times about 2,800 of `m8_site`'s 6,581 passes last four
      calls, 2,700 one call, the passes without a wait, and the rest up
      to 14), the round's end 0.07 s more than its retraces, and its
      timer's routine comes late (28,998 calls in 400 s: 13.79 ms apart
      instead of 13.73, most between 0.8 and 1.2 periods, single ones
      from 0.1 to 3.1). A key script
      written by the original's seconds therefore fits the port's own
      clock only until the first tap that falls near a sample.
    - What still comes from the log for the same battle (`-clock`; the
      test with it is unchanged and the same over all of the three runs:
      6,582, 4,574 and 4,428 passes), and why each:
      1. The times of `input_tick` (T). They are the emulator's and the
         sound's: `pit_irq` shares the timer with two routines of `S30EF`
         whose periods set the timer, and calls come late or are lost
         with it.
      2. The times of the keyboard's interrupt (K). The runner brings a
         key at the end of a batch of instructions, up to a millisecond
         after its time, and a logged place cuts batches (`main.c` of
         the runner); the order against T decides which sample has it.
      3. The times of the places where a pass reads what the interrupts
         left (P, I, V, C, A). They are the time the original takes to
         compute and draw; the port draws nothing.
      4. The numbers `clock_seed` sets (S). They are the BIOS's count of
         ticks, which `pit_bios` counts with the period 10000h beside
         `input_tick`'s 3FFFh. Tried: the count from the number of T
         lines before the seed, `floor(n * 3FFFh / 10000h + x)`. One x
         fits all eight seeds of `m8_site` (0.055 to 0.066) and all six
         of `m8_d5r` (0.152 to 0.304), but not both runs: the fraction
         moves with the other routines of the timer (presumably when the
         sound's are added or their periods change; not followed). So a
         derived seed can be one tick off, which is another number.
      5. The calls of `rand_next` by the fight's pictures (R). They come
         from `S2532:17D1`, which shares the losses still to show out
         among the volleys left (one call when more are left than
         volleys, or a draw against 80h otherwise) and takes off what
         `S2532:1970` says fell; that depends on the picture's figures,
         which are not read.
      6. Where the waits and pictures of a round's end clear the button
         and the key (W, X). `wait_input` is read (it clears both every
         three frames until one comes or its count runs out), but when
         it runs is the time of the fight's show around it (`S131D:1CAE`
         before the pictures; some 10 s a fight between two seeds),
         which the port does not have.
      Nothing was cut from this list: each was looked at and is a time
      of the original's drawing, of the emulator's interrupts or of the
      sound, or needs the fight's pictures.
    Not done: on the port's own clock a fight still takes no time (its
    marks, `wait_input(96h)`, the pictures), nor does the round's end
    with a site taken (`S28E0:002B` and `0004`, 64 steps of
    `S2AFE:000C` each, not read); the retrace's period is the runner's
    for the game's screen mode, not read from the game; `pit_del` and
    `pit_period`'s callers; `input_mouse` and `crit_error` were read
    only.
  - The fights of `attacks_run` (2026-10-07, Linux; `S02E6:0122` to
    `0D48`, `0D63` to `0E9C`, order 1 at `0ED9`, `order_undo`, read and
    translated, `port/src/orders.c` and `move.c`). Learned on the way:
    - Each attacker of side a's list: the first half of a two-part unit
      (40h without 80h is the second, the first is the one before) fights
      the unit byte of the map at its `unit_aim`; nothing when that is
      above F0h. `fight_rec` gets the far pointer game_loop passes first
      (the end of the sides' lists) at 52h, and 1000h is set in
      `game_state` before the first fight, so `pictures_load`
      (`S02E6:2B87`, now named) always runs at the end of a round with
      fights.
    - With fight pictures (`no_fight_pics`, `S37FF:0115`, 0)
      `fight_panel` (`S131D:178D`) and `fight_show` draw, the latter
      calling `fight_losses` and keeping the counts before and after at
      `S3AEC:000C` to `000F`; without them `fight_panel` calls
      `fight_losses` itself and `attacks_run` counts the blasts down on
      the map and puts the fields back.
    - After the losses: a unit left with nothing is destroyed
      (`unit_destroy`, slot 0 for the attacker, 2 for the target); a
      carrier that lost passes the loss on to each unit of its row
      (`carrier_losses`, `S02E6:2556`; one with no more than the loss is
      marked 4 and leaves the row); each side's experience (byte 1)
      rises, at most to 6, by one for the other unit destroyed and one
      for a loss it took that is not of a kind with 20h in word 6.
    - Then every unit marked 4 is freed (`unit_free` for side m), after
      `order_undo` when it has 1 in `unit_marks`, and for a carrier so
      are the units of side a's row of its record.
    - Order 1 (the mover took the unit in its byte 16h into its carrier's
      row): that unit goes over to side m like the units of a site taken
      (counts of the sides, serial number of its kind); if it was freed
      meanwhile only its place in the row becomes FFh.
    - `order_undo` (`S10E1:1514`) by byte 13h: 1 puts the taken unit back
      on the field with its flags (word 17h), 2 and 5 empty the place in
      the carrier's or site's row, 3 and 4 the unit's field (both halves);
      byte 14h loses 1 and word 4 4200h, byte 13h becomes 0.
    - `unit_panel` (`S10AF:0131`) and `pictures_load` change nothing the
      port keeps (read); the pictures' data in `S3AEE` and `S3812` does
      change and is counted apart in the comparison (port/README.md).
  - The fights' rarer paths (2026-10-08, Linux; port/README.md): two
    key scripts that play long: `m8_long` (map 8, the first side only
    waits from 375 s to 900 s) and `m0_carry` (map 0, a unit in the first
    side's carrier, waiting to 700 s), and `m8_long` again with fight
    pictures off (a poke of `no_fight_pics`). Reached and compared: units
    destroyed and freed, the blasts without pictures, `carrier_losses`.
    Learned on the way:
    - A long wait is scripted by ending the half every 10 s and F1 5 s
      later (`make_m8.py`'s `xT:U`); gestures in the computer's half
      changed nothing seen in these runs. It has to start with the cursor on a
      plain field and no menu open: after `m8_site`'s move out at 398 s
      the gestures did nothing and the game stood.
    - `pictures_load` writes `S331F:0284`, a digit of the file name at
      `S331F:027E` (presumably music or pictures by who leads; what it
      loads was not read), from the sides' unit counts (byte 2 of
      `sides`): (first+1)*100h/(second+1), the two swapped when only the
      first side has 2 in its word 0; '1' above 140h, '3' below C0h, else
      '2'.
    - `move_step` (`S10E1:0A6A`) passes `move_arrive`'s text to
      `show_message` for the moving side before it sets 8 or 10h in
      `game_state`'s word 4. While 4 (8) is in word 0, `hover` sets no
      timer, so the timers' slots differ without it (seen as a free
      slot's due pass at the passes 400 and 428 of `m0_carry`).
    - In `m0_carry` `carrier_losses` ran from 281 s on (seven times),
      but no freed carrier had a unit in its row, so the freeing of the
      units aboard did not run (whether the carrier itself was destroyed
      was not looked at). `order_undo` (a unit
      ordered to move and destroyed before its order) and order 1 came
      in no run; the hints say no code read writes kind 1 to
      `move_rec`.
    - A waiting script must not be matched by its own `pgrep -f`
      (a wait loop on `loop_dumps.py` stood for hours): wait on the job.
  - Next there: the rest of the computer's turn (`computer_step` and its
    handlers first, then `computer_assess`, `computer_plan`,
    `computer_queue`); a scripted run of the first side that moves into a
    site and a carrier and takes a site, to compare `move_arrive`'s other
    kinds and this part, and the income with a side that holds a depot.
    The human-input routine `S0CB6:05F1` has been read, but its port
    behavior is not implemented.

  - The three routines are translated (`port/src/range.c`) and compared
    with 108 calls of the original from the six long runs
    (`port/tests/range_test.sh`, dumps by `build/n2/range2.py`,
    `range3.py`, `range4.py`): 60 of `move_range`, 48 of `attack_range`,
    results and state the same. The runs hold 2,912 and 619 calls, nearly
    all the computer's: every call dumped is the second side's, with the
    masks 0, 6 and FFFFh for moves (0 is not one `field_menu` gives; who
    passes it was not looked at, presumably the computer's player). Not
    covered: the first side, a unit with 10h in `unit_flags` or word 6, a
    kind with 40h in word 0Dh, 2 in `attack_range`'s mask (port/README.md,
    "Checked"). A scripted move and attack by the first side would close
    the first gap. `field_bits` is the block at `S331F:3B5A` named
    earlier, so that block of the saved game is the marks of where units
    can move and whom they can attack, with `attacks_run`'s 80h and 40h.
- Decided with the user (2026-10-04): the hints of the sibling project's
  programs (`../bisle/src`, among them a data disk of the same year whose
  packer is the same) are not used here, not even to carry names where
  instruction streams match. PROVENANCE.md stays as it is; the exception
  for the offline file tools is the only one. Do not propose it again.
- Battles that end (2026-10-04). Of 20 runs of three emulated hours (a
  click and the F1 pair every 15 s after ten busy minutes; an hour of host
  time each) 19 finished; one hung in the
  runner's `mcb_coalesce` as described above (`build/n2/w8.keys`). Six of
  the first nine show the main menu at the end, so their battle was over
  before; whether by a victory or a defeat was not looked at. The 19
  executed 55,828 distinct starts and nothing outside decoded code:
  coverage by play has stopped finding code.
- Not checked: which of the remaining undecoded bytes are code (about
  18,000 in frames with code, the largest `S30EF` 4,680 and `CODE` 4,469).
  What is left needs reading: pointers and tables in those frames, and
  whether the launcher's status 7Bh (the last battle won) can be reached.

## `HL14-18.EXE` stage 1

- MZ program, entry `0000:000B`, stack `0296:0100`; its relocation
  table names 25 segment frames plus the stack frame.
- `src/HL14-18.hints` has the segment map derived from that table.
  `build.py` reaches 446 instructions and 65 labels and reports
  `IDENTICAL`.
- Eight instructions are `raw` because the current assembler cannot
  express them; their addresses are copied from `build.py` output.
- `C0015` starts one byte into its frame. Without `start=1`, later
  segments are paragraph-aligned incorrectly and the rebuilt image is
  one paragraph too long.
- The MZ header is Borland TLINK 5.0 (`linker tlink 50 header=original`)
  and its original relocation order is retained.
- A two-second deterministic runner trace loads `INTRO/INTRO.EXE` and
  reaches `C0170:0000`; that entry is recorded as a `code` hint. The
  rebuilt program remains identical (451 instructions, 66 labels).
- Relocated segment words following direct far CALL/JMP opcodes identify
  17 additional code entries. Adding them and the three required `raw`
  encodings expands the identical rebuild to 1,464 instructions and 191
  labels (13 `raw` instructions).
- A fresh two-second launcher coverage run again loads `INTRO/INTRO.EXE`,
  but reaches none of the remaining `gaps.py` gaps. The large `C0015`
  frame, in particular, has no verified entry yet; do not classify it from
  the code-like bytes alone.
- The three relocation sites whose values select `C0015` are segment loads,
  not direct CALL/JMP targets. `gaps.py` finds no other convincing reference
  to its only gap start. A run with Escape at five seconds exits the intro
  and starts `HL.EXE`, but reaches no launcher gap before then.
- Three DOS vector installations give verified `C0015` entries: the
  `INT 21h/AH=25h` calls at `C0015:004A`, `C0015:059A` and `C0015:0688`
  install the INT 24, INT 09 and INT 08 handlers at `C0015:055C`,
  `C0015:04F4` and `C0015:06D2`, respectively. These entries are recorded
  as `code` hints. The normal launcher path with Escape at five seconds did
  not execute the INT 24 handler before starting `HL.EXE` (zero breakpoint
  hits through 30 seconds); it also reached no C0015 gaps.
- Targeted idle, early-key and late-key launcher runs likewise gave zero
  hits at all three vector handlers and no C0015 coverage. In the late-key
  run, after `HL.EXE` loaded at `t=5.109051`, INT 08 and INT 09 pointed to
  `2B30:06D2` and `2B30:04F4`; those are offsets in `HL.EXE`, not launcher
  coverage.
- A six-second launcher input sequence (A, F1, Enter, Space, Escape, Up,
  Down and Tab) again gave zero hits at the launcher INT 09, INT 24 and INT
  08 handlers, and no C0015 gap ran. It loaded `INTRO/INTRO.EXE` at
  `t=0.000007` and, after Escape, `HL.EXE` at `t=3.111522`; by the end INT 08
  and INT 09 belonged to the loaded main program/BIOS. A two-second idle
  interrupt trace likewise showed the launcher immediately EXECing the
  intro, before its C0015 installation sites or handlers ran. The runner
  currently reports failed DOS open/EXEC operations as INT 21h error returns.
  Doskit now has the tested `-criterr AH ERR` runner option: it makes the next
  matching INT 21h call enter the guest's INT 24 vector, and an `AL=3` (Fail)
  return gives its caller `ERR` in AX with CF set. The launcher has not yet
  installed its C0015 INT 24 vector in an observed run, so this option has not
  yet produced launcher-handler coverage. These experiments add no C0015
  entries.
- Segment-boundary review (2026-10-03) found 15 tracked routines whose
  continuations had been left in the following relocation frame as data,
  including split instructions and backward branches. Explicit `size=` and
  `start=` now keep those tails with their original CS frame. The former
  `code C0170:0000` alias is covered by the continuation of `C016F:0004`
  instead. None of the extended regions contains a relocation site.
  CS-relative variables at `C00B2:0059-005A` and `C012C:0064-0069` stay
  with their routines as well; this avoids four otherwise required `raw`
  encodings caused by labels outside the segment.
  The final build reports `IDENTICAL` with 1,663 instructions, 236 labels
  and 13 `raw` instructions; no additional `raw` hints are needed.
  Fresh gap classification against both the EXEC-error coverage and the
  six-second normal coverage reports 33 gaps, 3,249 bytes, and zero reached
  gaps. The retained CS-relative variables account for two of these gaps;
  a gap is not by itself evidence of code.
- A targeted EXEC-error run (`run.py -until 1 -key 0.2 enter -criterr 4B 2
  -dos -log CODE:010C -cover build/HL14-18-collector-criterr.cover
  HL14-18.EXE`) reaches `CODE:010C` at `t=0.000010`, with AX=`007E` and
  CF set. Enter dismisses the prompt and the launcher exits with code zero
  at `t=0.200112`. The four formerly reported reached gaps are routine
  tails in the preceding frames, not new indirect entries. No DOS vector
  installation occurs before this first EXEC: the injected error uses the
  runner's default vector, so this is not evidence that `C0015:055C` ran.
- The `-criterr 4B 2` experiment above predates doskit `f4dc33c`; that
  version accepts only critical error codes 13h..1Fh. Repeating the bounded
  run with `-criterr 4B 15`, without stopping at a breakpoint, exits normally
  at `t=0.200112`: `CODE:010C` logs once at `t=0.000010` with AX=`007E`,
  while `CODE:00E9` logs zero times. Gap classification against its fresh
  coverage still reports the two CODE gaps (`0000-000B`, `00E9-00F0`) and
  three C0015 gaps (`0005-04F4`, `056C-06D2`, `0783-08F0`) as unreached.
  This valid critical-error injection adds no new launcher entry evidence.
- The same valid `ERR=15h` injection on the observed CHDIR call
  (`-criterr 3B 15`, call at CODE:0035) exits normally at `t=0.200112`.
  Breakpoints and logs at the three C0015 vector-installation sites and all
  three handlers each report zero hits; coverage again marks all three
  C0015 gaps unreached. This earlier DOS error path also gives no new entry.
- A six-second normal run with Escape at three seconds loads the intro at
  `t=0.000007`; it exits at `t=3.111517` and the launcher EXECs the main
  game at `t=3.111522`. The launcher does not reach the child-status query
  at `CODE:009A` before that run ends. Thus the later outro branch has not
  been verified by this run.
- A bounded reference audit (2026-10-03) found no decoded immediate far
  CALL/JMP or recognized relocated far pointer targeting C0015. Its three
  segment relocations are at C0015:0027, C0015:062E and C0015:0684;
  these do not establish callers of the vector-installation routines.
  The timer's indirect far call at C0015:073C addresses DATA:0E25 plus
  SI after loading DATA into DS; its table targets remain unverified.
- The CS-relative word-table loop at C00F4:00A9 advances SI twice and
  exits on a zero word. Starting at C00F4:00ED, its first zero is at
  C00F4:00F5, so the table ends at 00F7 across the next relocation frame.
  C00F4 size=F7 and C0103 start=7 retain the whole table with its reader;
  no relocation site lies in that table. A dptr records the table address
  loaded at C00F4:00A4. Another dptr records C0173:0311's DATA pointer:
  DS is loaded at 030B/030E and LES DI,[BX] consumes it at 0326.
  The rebuild remains IDENTICAL (1,663 instructions, 238 labels, 13 raw);
  gap classification against the existing six-second normal coverage still
  reports 33 gaps, 3,249 bytes, and zero reached gaps.
  These are static observations; neither table's contents or targets were
  newly verified by execution.
- Not checked: runtime reachability of the remaining launcher gaps and the
  three vector handlers under the events that invoke them; obtain targeted
  runs or identify further verified pointer/table entries next.

### Launcher size and dispatch map (2026-10-03)

- `HL14-18.EXE` is 11,872 bytes (10,848-byte MZ image). Its initial `CODE`
  segment spans only 0155h bytes and decodes to 162 instructions. The
  executable has 25 code relocation frames in total; the identical rebuild
  decodes 1,663 instructions, with DATA spanning 0E80h bytes and STACK 0100h.
  The size is distributed across linked support modules and data, not just
  the dispatcher.
- The dispatcher at CODE:000B calls C016F:0004, which uses DOS AH=4Ah to
  resize the launcher's memory block, then stores its stack and PSP. It
  changes to the INTRO directory and executes INTRO.EXE at CODE:0054. After
  return it changes to `..` and executes HL.EXE at CODE:008F. It checks the
  child's DOS return status with AH=4Dh; only AX=007Bh proceeds to the
  EXTRO directory (if its CHDIR succeeds) and executes EXTRO.EXE at
  CODE:00D5. Other statuses go to the common exit. Runs have observed the
  first two EXECs; the 007Bh branch and third EXEC have not been reached.
- The DOS failure routine at CODE:010C calls display helpers and waits for a
  key. Its two error messages and the three executable names are stored in
  DATA. Instruction evidence shows BIOS video calls in C0108/C012C, INT 33h
  mouse calls in C015D, and game-port reads at 201h in C013F. C0015 contains
  timer setup and interrupt handlers, but most of its 08F0h span remains
  unclassified; low-level I/O or helper code in other modules is not yet
  named. Keep the remaining gaps unclassified until an entry or data role is
  established.
- Static tracing of the outro condition reaches into the main game's exit
  path. HL CODE:013A calls the existing `code S066A:000D` entry and passes its
  returned AX through CODE:L034A and L02D9. When L02D9's `[BP+6]` and SI
  checks allow termination, it passes `[BP+4]` to L0159, which exits through
  DOS AH=4Ch with that byte in AL. In the decoded S066A routine, a test of
  `S331F4CA4 & 20h` conditionally stores 007Bh in its local return value;
  S063F:0198 sets that flag bit when a referenced record byte at offset 8
  has bit 0 set. The existing 30-second no-input `HL-30.cover` does not
  reach S063F:0198, S066A:46F2, or the S066A return sequence. The user
  suspects this flag/status is set on a game win, which fits the launcher's
  EXTRO condition; that meaning and an actual exit with status 007Bh are
  not runtime-verified. Do not keep probing ordinary launcher starts for
  this branch; treat the static return path as understood and the win
  trigger as unverified.

### Intro and outro first collection (2026-10-03)

- `INTRO/INTRO.EXE` is a 80,032-byte MZ file with a 5,632-byte header, 1,124
  relocations, entry `0000:0010`, and stack `1222:0080`. Its linker marker is
  consistent with TLINK 5.0. The relocation table has 68 site frames, all
  also present among the 71 target frames; `0968`, `0E07` and `0F6C` are
  target-only. `src/INTRO.hints` now records the full layout and rebuilds
  IDENTICAL: 16,176 instructions, 1,921 labels, and 47 `raw` instructions.
  Relocation overhang establishes `S094E start=5`, `S0B1B start=2` and
  `S0B1F start=3`; far pointers with target offset zero establish several
  other segment bases. DATA-class treatment of target-only frames `0E07` and
  `0F6C` remains a working classification.
- A deterministic two-second runner session loaded INTRO at PSP `0077`,
  completed 12,000,001 instructions over 80 frames without an emulation error,
  and opened `INTRO.ADL`, `BANG.ANI`, `BANG.PAL`, `BANG.TXT`, `CHR14.LIB`, and
  `BANG.SFX`. Coverage marked 28 gaps reached; their first instruction
  addresses are now `code` hints. CODE and several relocation frames still
  have unclassified gaps; the two-second run does not settle all code/data
  roles.
- `EXTRO/EXTRO.EXE` is a 75,136-byte MZ file with a 5,120-byte header, 1,042
  relocations, entry `0000:0010`, and stack `1110:0080`. Its temporary
  segment map is now in `src/EXTRO.hints` and rebuilds `IDENTICAL`: 14,715
  instructions, 1,870 labels and, after explicit raw hints, 57 `raw`
  instructions. Site extents establish `S04AD start=3`, `S0592 start=B` and
  `S068A start=6`; the rebuilt stack is again `1110:0080` and the relocation
  sets and order match. Frames `0D22` and `0E5B` remain CODE-class because
  their roles have not been established.
- A normal EXTRO runner session exits with code 0 after 2,370 instructions.
  With the exact launcher tail, a 30-second run reached 180,000,001
  instructions and marked its runtime-seeded gaps. Remaining code-class
  gaps total 5,367 bytes and none ran in that coverage session. A clean
  `xfer.py` alignment from INTRO mapped 15,395 of 23,576 / 27,996
  instructions, 141 data addresses and 37 segments; 55 code/raw hints
  transferred and 20 did not map. This is partial instruction-stream
  similarity, not whole-program identity.
- Whole-run coverage (2026-10-03, macOS). An idle launcher run plays the
  whole intro: `INTRO.EXE` loads at `0333` and `HL.EXE` is loaded over it at
  `0332` at `t=397.794602`. Coverage for INTRO must come from a run stopped
  before that (`-until 397.79`): a `-cover` file has only addresses, and
  both `gaps.py --cover` and the comparison used here take every address
  above the first load, so a longer run counts `HL.EXE`'s instructions as
  INTRO's (the first attempt did; it produced entries in the middle of
  instructions and in the data frame `S0E07`, and was thrown away). The
  clean run executed 10,638 instruction starts; the 825 outside decoded code
  are now covered by `code` entries. INTRO rebuilds IDENTICAL with 17,173
  instructions, 2,082 labels and 51 `raw` instructions.
- EXTRO with the launcher's command tail runs to its own end: exit code 0
  at `t=561.691638` (3,370,149,828 instructions), 8,531 instruction starts.
  After `xfer.py` carried INTRO's new hints (107 carried, 30 not mapped),
  43 starts were outside decoded code; they are `code` entries in EXTRO's
  own part of the file, above the carried block. EXTRO rebuilds IDENTICAL
  with 14,819 instructions, 1,892 labels and 60 `raw` instructions.
- A check against run-time memory guards this method: a `-ram` dump is
  compared with the file image (relocated words left out), and a start whose
  bytes changed is not taken as the file's code. No start was dropped for
  that in INTRO, EXTRO or `HL.EXE`. The only decoded instruction over
  changed bytes was at INTRO `CODE:0294` and `HL.EXE` `CODE:0284`, each a
  CS-relative variable the entry code writes. A `stop` at the call before
  it (`CODE:0291` / `CODE:0281`) and a `code` entry at the `PUSH BP` after it
  (`CODE:0298` / `CODE:0288`) make the word data; `xfer.py` carried the pair
  to EXTRO. That the call does not return and that a routine begins there
  are presumptions: no run executed either address. After this the programs
  rebuild IDENTICAL with 68,739 (`HL.EXE`), 17,172 (INTRO) and 14,818
  (EXTRO) instructions.
- Runs with input (2026-10-03, macOS). INTRO started by itself and left
  with Esc at 5, 40, 125, 200 and 300 s (some with Enter, Space and mouse
  buttons before) exits with code 0 a little after the key; the five runs
  executed 35 starts outside decoded code, now three `code` entries
  (`CODE:02CC`, `CODE:2AB2`, `S02AF:0001`; INTRO IDENTICAL with 17,207
  instructions). `xfer.py` carried them (111 carried, 31 not mapped). EXTRO
  with the launcher's tail and Esc at 5 s, or Space at 150 and Esc at 155 s,
  ends shortly after the Esc; with Esc at 60, 300 or 400 s it ran to its own
  end at about 561.7 s (why those were not taken is not checked). The five
  EXTRO runs executed nothing outside decoded code.
- Segment bounds (2026-10-04), as for `HL.EXE` above: 48 segments each of
  INTRO and EXTRO get `start=` from their lowest far-referenced offset and
  the segment before the matching `size=`; the three segments of each that
  already had a `start=` stay. `code` entries below a start are removed,
  the `raw` lines made anew, `xfer.py` run again (84 hints carried, 7 not
  mapped). INTRO rebuilds IDENTICAL with 17,288 instructions (26 as DB,
  before 51), EXTRO with 14,909 (26 as DB, before 60); every start of their
  coverage files is decoded (10,972 and 8,588). The same caution holds: a
  far-referenced offset is an upper bound of a module's start.
- `src/INTRO.hints` had 71 lines ending in CRLF among LF lines (the
  repository has `* -text`, so nothing converts them). Commit `bebc4c4`
  rewrote the file with LF throughout, which is why its segment lines show
  as changed there; no hint changed on those lines.
- Commands and files used for this collection are local: generated sources,
  rebuilds and coverage are under ignored `build/`. Stage-1 `src/INTRO.hints`
  and `src/EXTRO.hints` contain no game bytes.

### EXTRO command-tail launch probe (2026-10-03)

- The launcher's EXTRO EXEC uses `BX=0008`, `DX=002F` and a parameter block
  whose command-tail pointer is `CS:0000`. The launcher image encodes a
  nine-character DOS command tail there. A direct EXTRO run with that same
  tail took the long path: it opened/read
  `EXTRO1.ADL`, `EISENBAH.LBM`, `EISEN0.PAL`, `EISENBAH.PAL`, `EISEN2.PAL` and
  `warend.txt`, then remained active until `-until 40` (240,000,001 total
  runner instructions). Direct runs without a tail exited after 2,370
  instructions; `/?` also exited early. Passing the matching tail as one
  argument or as two arguments produced identical open timing and coverage.
- A launcher run that skipped HL.EXE but retained the launcher-created
  EXTRO EXEC context also ran through the 30-second limit and opened the same
  assets. Thus HL execution and its persistent file state are not required
  for this behavior. Runs stopped at the time limit, so EXTRO's natural end
  was not observed. The matching command tail explains the immediate direct
  exit; which characters EXTRO checks has not yet been isolated.
- The exact launcher tail is required for the observed long path: a direct
  run with only its leading `0` or with a different identifier exits early
  (2,509 and 2,721 instructions respectively) without attempting file
  opens. The identity-specific check has not been located in the disassembly.
- EXTRO reads and tokenizes the child PSP command tail starting at
  `CODE:1DC5` (`MOV ES,[D007B]`) and `CODE:1DC9` (`MOV SI,80H`). Breakpoints
  at both named addresses hit in the exact-tail run; ES points to PSP segment
  `0067` at `1DC9`. The exact tail and a mismatched identifier both reach the
  argument dispatcher; their execution diverges after its indirect dispatch.
  The specific comparison and branch have not yet been assigned a verified
  EXTRO code address.
- All probe logs and coverage are in ignored `build/extro-probe/`. The DOS
  trace is very large because EXTRO polls DOS during its active loop; avoid
  long `-dos` runs when coverage or targeted breakpoints are sufficient.

## Next

1. For `HL14-18.EXE`, keep the remaining C0015 gaps and vector handlers
   unclassified until an incoming call or table target is evidenced. Do not
   repeat ordinary launcher runs for these gaps. The dispatcher and
   conditional EXTRO path are statically mapped; the likely win trigger for
   status 007Bh remains unverified.
2. `HL.EXE`: 71,755 instructions decode and play finds no more code. Go on
   with stage 2 where it stands (the bullets of 2026-10-04 above):
   - the fight is confirmed (`build/n2/fights.sh`, `fightcalc.py`); left
     of it: a fight of a kind with 4 in word 0Fh, `S131D:178D` and the
     pictures in `S2532`. The per cent bytes are confirmed too
     (`fight_percent`, `build/n2/percalc.py`), and both are translated in the port
     (`port/src/fight.c`, `port/tests/fight_test.sh`). Next in the port:
     `unit_destroy` and `unit_free` (`port/src/units.c`,
     `port/tests/units_test.sh`) are done, and so is the making of a
     battle's state from a map's two files (`port/src/map.c`); the loading
     itself is translated too (`file_load`, `port/src/files.c`). `map_load`
     and `shp_load` are whole, with `file_name` and `num_text`
     (`port/src/names.c`). Next there: the first part of `game_loop`, which
     sets a battle up (it calls them; which files go where, and what else
     it prepares), call by call and translated as far as it draws nothing, so that the port
     builds a battle's state from the game's folder by itself;
     `mem_alloc` is translated on the runtime's DOS blocks. The
     port's own start is done up to the loop's beginning at `S066A:0EE6`
     (`port/src/battle.c`, the bullets "The port's own start" and "The rest
     of a battle's setup"). The loop is read from `S066A:1F39` to its end
     (the bullet "The loop of `game_loop`, second reading": the states of
     a view, the other caller of `unit_add`, which builds a unit in a
     site, the end of a round and of a battle). Next: its part from
     the scripted run named under "The orders carried out" (the walk is
     read and translated, `port/src/move.c`, and what a round's end does
     with the orders is read, as is the
     way, `path_find`; `view_step`, `field_menu`, the routines that
     take an order, `move_aim`, `move_start`, `attack_order`, the round's end in
     `attacks_run` and `move_range`, `move_costs` and `attack_range` are
     read, the three translated in `port/src/range.c` and compared). The
     computer-turn and income routines have since been read and named;
     next translate them and compare against runs, then translate what of
     a side's part changes state and draws
     nothing (building, repairing, moving out), each with a comparison
     as for the setup; the libraries' loading (`lib_load`) is translated
     and compared (2026-10-08), `pictures_load` is next toward drawing. `attacks_run` around them waits for
     the drawing routines; a fight the first side starts is still to be
     seen (a scripted attack by the player), as are a carrier's and a
     two-part unit's end;
   - what the three tables filled from the ground are (`site_add_400`,
     `site_add_100`, `site_add_40`) and what byte 18h of their record
     counts; what the units' bytes 0, 3, 0Fh, 10h and word 6 mean, and
     the other fields of a kind's record in `unit_dat`;
   - how orders are carried out (the second map, `unit_flags` 0200h and
     0100h), then the loops over the units in `S02E6`, `S1CC6`, `S1FD8`
     and the computer player's in `S2138`, and the routines of
     `game_loop`;
   - the saved game's blocks not yet looked at (the two F2h and two 82h
     blocks, `save_game`'s buffer, `S331F:4CA4`, `3B5A`, the `S39B6`
     tables).
   For doskit: the runner's endless loop in `mcb_coalesce`, the limit of
   4,096 input events, and an option of `gaps.py` for coverage of
   DATA-class segments. A wait loop on a task's output file must not test
   the last line: the harness appends its own (one such loop ran for seven
   hours on 2026-10-04).
3. Continue stage 2 for `INTRO/INTRO.EXE` and `EXTRO/EXTRO.EXE`: everything
   their full idle runs execute is decoded now, so name routines and
   classify what is left from tables and pointers (and from runs with keys:
   neither program was given input). Keep the transferred INTRO
   hints only where this game's instruction streams match.

## bisle file-tool compatibility probe (2026-10-03)

At the user's request, the unchanged file-tool CLIs from sibling `bisle`
revision `a88573b7cef03de0521d65518a0a307a6fb45359` were run as black boxes
on this project's installed History Line data. Their implementation source
was not read or copied. This is a tool-compatibility experiment, not evidence
for History Line's runtime interpretation and not permission to derive
History Line code from Battle Isle's game-specific parsers.

Results from explicit input paths (success means the tool reported
`written back identical`, rather than merely exiting with status zero):

| Tool | Result on History Line files |
|---|---|
| `tpwmfiles.py` | All 489 packed files found in a 544-file recursive scan round-trip identically; zero failures. |
| `ifffiles.py` | All 66 recursively found `.IFF` / `.LBM` files round-trip identically. |
| `palfiles.py` | All 33 `.PAL` files round-trip identically. |
| `libfiles.py` | All 21 `.LIB` files round-trip identically. `NEWCHAR6.LIB` needs UTF-8 stdout on this Windows host; the first failure was an output-encoding error. |
| `txtfiles.py` | `CHAR6.DAT` and `TITEL.TXT` round-trip identically; `GAME.TXT` is rejected with `text 0 does not end`. Other text layouts are unverified. |
| `datfiles.py` | Top-level `CODES.DAT` and `AMOK.DAT`, and `LIB/UNIT.DAT`, round-trip identically. Top-level `UNIT.DAT` is rejected with `3519 bytes, not records of 68`; `LIB/GROUND.DAT` with `271792 bytes, not records of 6`. The original top-level directory also lacks the expected `GROUND.DAT`. |
| `mapfiles.py` | Direct use on `game/MAP` stops at the missing top-level `GROUND.DAT`. With the actual `LIB/GROUND.DAT` and `LIB/UNIT.DAT` copied unchanged into a temporary root beside a copy of `MAP`, 73 maps are tested: `STATMAP` round-trips identically, while all 72 numbered maps fail on trailing `.SHP` data (36 to 228 bytes after the recognized records). No alternative ground file was substituted. |
| `sndfiles.py` | Directory invocation finds zero files: exit zero is vacuous, and sound compatibility is unverified. |

PNG export also succeeds for `AHHIMMEL.LBM` (one image), `00.PAL`
(one image), and `INTRO/CHR14.LIB` with explicit `--pal game/00.PAL`
(47 images); these exports were not visually or semantically verified.

Reproduction: pass each file recognized as packed to
`python ../bisle/tools/tpwmfiles.py FILE`; run
`python ../bisle/tools/ifffiles.py FILE` (likewise
`palfiles.py` and `libfiles.py`) for every matching file recursively under
`game/`; run `txtfiles.py game`, `datfiles.py game`, `datfiles.py game/LIB`,
`sndfiles.py game`, and `mapfiles.py game/MAP`. Set
`PYTHONIOENCODING=utf-8` for the Windows console. For the additional map
experiment, place unchanged copies of `game/LIB/GROUND.DAT`,
`game/LIB/UNIT.DAT`, and `game/MAP/` under
`build/file-tools-probe/staged-game/`, then pass that root's `MAP` directory.
The local command logs and input/exit metadata are in
`build/file-tools-probe/`; no game bytes or generated assets are tracked.

Not verified: writer internals (whether any fields pass through unchanged),
rendered-image correctness, audio playback, interpretation of map suffixes,
and any of these formats against History Line's original loading routines.
The successful tools can help inspect local assets immediately; a port
parser still needs independent evidence from History Line itself. Prioritize
its `.SHP` reader and the distinct table/text layouts rather than assuming
all corresponding Battle Isle formats match.

### Adoption requested after the probe

The user then explicitly requested adopting the working tools. The four
fully successful tool families (`tpwmfiles.py`, `palfiles.py`,
`ifffiles.py`, `libfiles.py`) and their `png.py` dependency now live under
`tools/`. `PROVENANCE.md` records this scoped exception and source revision;
these are imported offline analysis tools, not independent History Line
implementations or runtime code. No partial table/text tools or unverified
sound/map tools were included in that initial adoption.

Library PNG output now defaults to the game's top-level `00.PAL` and
colour base zero; Battle Isle's library-specific colour bases were removed.
Descriptions no longer present Battle Isle executable observations as
History Line evidence. Parsing/packing is otherwise retained. Source
inspection confirms that unknown library entries, entry tails and unknown
IFF chunks can pass through unchanged; successful round trips therefore
prove less than complete format reconstruction. See `tools/README.md` for
directory checks and local export commands.

The first imported picture tool's directory scan also selected 12
`FX/*.SFX` IFF containers, all rejected with `no BMHD`. Its automatic
discovery now covers only the tested `.IFF` / `.LBM` picture families;
explicit other inputs still report their parser errors. The library
directory scan additionally checks top-level `BB.DAT`, which round-trips
identically along with the 21 `.LIB` files.

Verification after adoption: all four directory commands in
`tools/README.md` exit zero: TPWM 489 files, PAL 33, IFF 66, LIB 22;
each reports zero failures. The TPWM packer comparison reports `isle 5`,
`moon 421`, and `moon but the last item 63`; the last category is not a
complete recompression match despite an identical parsed-item round trip.
The three single-file PNG commands for `00.PAL`, `AHHIMMEL.LBM`, and
`INTRO/CHR14.LIB` also exit zero, the library using its new default
palette without `--pal`. Logs are `build/file-tools-probe/adopted-*.log`.
No visual or original-runtime comparisons were made.

Four additional experimental inspectors are now present:
`datfiles.py`, `mapfiles.py`, `txtfiles.py`, and `sndfiles.py`. Their probe
results and limits are listed above. The DAT, map and text tools adapt
offline parsing techniques; the sound tool is project-specific. These are
not port parsers, and none has been checked against History Line's original
loading routines. `tools/test_file_formats.py` uses synthetic data only.
