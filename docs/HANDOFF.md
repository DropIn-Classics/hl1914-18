# Handoff

State of 2026-10-04: all four programs rebuild identically (`HL.EXE` 71,755
instructions, INTRO 18,055, EXTRO 15,678, the launcher 1,663). Everything
that runs of the game executed is decoded, including whole battles on all
72 maps, saving and loading. Stage 2 is under way in `HL.EXE`: the saved
game's layout, the unit records and the tables beside them, how `map_load`
builds a battle's state, the two maps, and the routine that takes a fight's
losses off (`fight_losses`) are named and described below under "`HL.EXE`
stage 1" (the bullets dated 2026-10-04); the fight's arithmetic is read and
agrees with 39 fights of six runs. The port has its first translated
routines (`port/src/fight.c`: `fight_percent`, `fight_losses`), the same as
the original on those 39 fights; nothing calls them yet. `master` was pushed at `a6e0117`; the commits after it are local.

The `doskit` submodule is now at `a4b8166` (fast-forwarded from `f4dc33c`).
That update adds consent-based package self-updates to the runtime
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
     `attacks_run` around them, `unit_destroy`, `unit_free`; a fight
     the first side starts is still to be seen (a scripted attack by the
     player);
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
