# Handoff

State of 2026-10-03: the repository was made from doskit's template.
`HL14-18.EXE` now rebuilds identically; its indirect code is not yet
fully classified.

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
- Not checked: which of the remaining undecoded bytes are code. The runs
  never ended a game by a victory, saved or loaded; those are the next
  sources of coverage, then bounded tables and pointers in the frames named
  above. Which key saves is not known (the texts say `ONLY IN MAP MODE`,
  `CHOOSE POSITION 0-9`, `TO SAVE PRESS SPACE BAR`).

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
2. `HL.EXE`: 69,628 instructions decode; about 21,000 bytes in frames with
   code remain. Get coverage of what no run did yet (a won or lost game,
   save and load, whole rounds with the F1 pair), compare it with the decoded
   starts of all segments as described above, then begin stage 2 names and
   records.
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
