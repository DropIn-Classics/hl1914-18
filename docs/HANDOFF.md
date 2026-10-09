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

On the branch `mac/sound` (2026-10-08) the sound's routines are read whole
and named, and the effects' part is translated without sound output
(`port/src/sound.c`; the bullet "The sound" at the end of "`HL.EXE` stage
1"). It also found a fault in the runner's REP STOS that every earlier
comparison ran with (said there); it is fixed in doskit's master
(`22292c0`, the submodule points at it). On the branch `mac/rerun`
(2026-10-08) the sound's comparisons and the menu's, saving's, the F
keys', the briefing's, the information's, the loop's and m8_site's play
test were rerun with the rebuilt runner: the same as before but for
`snd_devices`, now 3 in the runner as in the port (the bullet "The
sound", "Rerun with the fixed runner"). The other comparisons' dumps
are still the old runner's until their owners rerun them.

On the branch `mac/music` (2026-10-08) the music's control is translated
(`music_start`, `music_fade`, `music_volume`, `fade_tick`; still no
sound output) and called where `game_loop`, `attacks_run`, the menu and
the new units' screens call it; 297 calls compared with the original,
none different (the bullet "The sound", "The music's control").

On the branch `mac/fightsound` (2026-10-08) the fight's sound triggers
are translated (`pics_play` and its three sites in `fight_frames`, the
channels stopped at the pictures' end, `attacks_run`'s effect for a unit
destroyed without pictures; `port/src/sound.c`, still no sound output)
and compared with the original (the bullet "The sound", "The fight's
sounds"). `fightpics.c` and `orders.c` were not to be touched, so the
triggers are driven from their hooks by `main.c`; the lines their owners
can put in instead are listed there.

On the branch `mac/sfx` (2026-10-08) the ways of `sfx_play` not reached
before are compared with the original (a failing call, an effect played
more than once or endless, an 8SVX effect: reached in the picture
`pic_wiped` with pokes, since the game's own tables play every effect
once) and the digital mixer's memory side is translated (`dig_mix`,
`dig_irq`, `dig_kick`; still no sound output), so the port's digital
channels end and repeat as the original's (the bullet "The sound", "The
effects' ways and the mixer").

On the branch `mac/opl` (2026-10-08) the AdLib's register stream is
compared with the original's write for write (three walks, 16,705
registers, none different), with what that needed translated: the
probe's and the end's writes (`adl_detect`, `adl_remove`, `snd_end`)
and the timer's count for the fade and `adl_tick` as `pit_irq` keeps
it. The registers go to an OPL2 of the runtime that nobody plays yet;
still no sound output, nothing listened to (the bullet "The sound",
"The AdLib's output", which also says what the audio's clock needs).

On the branch `mac/soundout` (2026-10-08) the AdLib is played on the
audio's clock: the writes go through doskit's queue with their samples
(`oplq.c`), the timer's interrupts are counted by the samples played,
and headless a tune is rendered to a WAVE file on the kit's virtual
audio device (`DK_WAV`). Two runs give the same file byte for byte and
every write is at its sample in it (`port/tests/soundout_test.sh`).
**Nothing was listened to**, the window's program was not run, the
Sound Blaster's bytes are still not played (the bullet "The sound",
"The AdLib played").

On the branch `mac/oplwav` (2026-10-09) the rendered sound is set beside
the original's as the runner's chip played it (`-oplwav`), tune by tune
(`port/tests/oplwav_test.sh`): the same registers at the same ticks
where the game's side does not write, the levels within 2 dB, the
original behind in time by the timer interrupts it loses; and the 70
registers a battle writes at another place with a device are the
millisecond of the virtual device's clock, no offset. Nothing was
changed in what is played and **nothing was listened to** (the bullet
"The sound", "The AdLib beside the original's").

On the branch `mac/scores` (2026-10-09) the best scores' screen of a map
that has scores and a winner's name corrected with the backspace are
compared with the original, in the headless window too: game state,
video memory and the scores' file the same, with what the port's own
clock makes different told apart or met by the keys' times (the bullet
"The best scores with scores there and the name corrected"). Nothing of
the port changed. Not run: the SDL window by hand, the sounds.

On the branch `mac/options` (2026-10-09) the two input paths the F keys
switch are compared with the original: the mouse as a side's directions
and button (`input_mouse`, which was translated on `debian/window2` but
never set beside the original; F7 off and on, F8) and the joysticks,
now translated whole (`joy_setup`, `joy_read`, the sticks' part of
`input_side0`/`input_side1`; `port/src/joy.c`, `port/src/input.c`). For
the sticks the runner got a game port with scripted sticks (`-joy`,
`-joys`; doskit's branch `doskit/gameport`, `d1672bf`, which the
submodule points at until it is merged into doskit's master; selftest
ok on macOS, not run on Linux or Windows). Three runs of map 48, game
state, inputs and DATA 0380 to 045C the same at every pass compared (the
bullet "The mouse and the joysticks as a side's input"). `loop.c` was
not to be touched, so its `joy_setup` is still the one without a game
port and the real one is started from the timer's routine; the line its
owner can put in instead is in that bullet. Not run: the window, a real
stick or controller (the window's program has no stick at all).

On the branch `mac/options2` (2026-10-09) the four things that bullet
left open are run and set beside the original: a stick in the menu (two
walks, the same), `-map` with a clock's log and a sticks' script (the
same but the pointer's range the menu sets), the sticks on the port's
own clock (taps: the same, the counts of gaps a step apart at 5 of 35
times) and the texts of F5 to F8 as video memory (no byte of the two
pages different). The port changed in two small places for it
(`joy.c`'s script in the menu before a battle on the original's clock,
`main.c`'s pokes made when the first pass begins); the bullet "The
joysticks' leftovers" says what was learned and what is still not run.

On the branch `mac/options3` (2026-10-09) the two gaps those notes left
are run and set beside the original: the other buttons at the saved
game's question (a stick's second, both at once, the second stick's;
the mouse's first, second, both, and the pointer moved alone: two new
walks of the menu, the same) and a stick moved in the menu and held
while the battle is set up, the battle played on the original's clock
(`m48_joymenu`: the same). Nothing of the port's source changed; the
tests only. Found by the way: the port's menu has its first pass 0.84 s
behind its `-start` (the bullet "The joysticks' two gaps").

On the branch `debian/window13` (2026-10-09) the two clock gaps the
window's notes left are run and set beside the original: a battle on
the original's clock inside a menu walk (`m57_menu`; the menu behind it
got no keys, now fixed) and the pictures D0h and CFh on the port's own
clock (`m57_won`, `m57_medal`, by pokes; 1.11 s behind D0h were
missing, now counted). The bullet "A battle's end on both clocks" says
what was learned; `sh port/tests/endclock_test.sh` runs it.

On the branch `mac/options4` (2026-10-09) the three small remainders of
those notes are run and set beside the original: sticks held longer
than a tap on the port's own clock (`m48_joyhold`: the same input words
in the same order, three of twelve holds read for one pass more or
less, the game's state the same at every time compared), the mouse's
third button (in a battle, `m48_mouse3`, and at the saved game's
question and in the menu, the walk `mouse3`: it does nothing by itself
in either program) and why the tenth pass of `m48_joymenu` reads 8
(the mouse's pointer, where the driver's reset left it; `m48_joyptr`
moves it and the pass reads that). Nothing of the port's source
changed; the tests only (the bullet "The joysticks' three remainders").

On the branch `mac/options5` (2026-10-09) the two gaps the input
options still had are run and set beside the original: the texts of F5
to F8 on the second side's line (`m24_ftext`: map 24, the computer's
first side; all nine texts through `show_message` with DI A0h, no byte
of the two pages different at 23 passes) and a button held from before
the saved game's question (the walk `heldask`: the question is drawn
only when the button is let go, 0.15 s behind it, in both programs).
Of the port's source only `menu.c`'s log for the tests changed (a line
"ask N TIME" for each screen outside the menu's loop); the bullet "The
input options' last two gaps".

On the branch `mac/thinkbar` (2026-10-09) the bar of the computer's
thinking is read whole and set beside the original. The task said it
was not translated; it was, since `debian/window` (`23fa0e9`:
`computer_progress` in `plan.c`, `bar_draw` in `draw.c`, the call in
`loop.c`'s `computer_part`), but only `bar_draw` had been compared, with
the value taken from the original's stack, and `S1C74:0354` had been
read to its first loop. Now 13 calls of three maps, both sides: what
`computer_progress` returns, memory at the call and behind the bar,
video memory and the DAC, nothing different. Of the port's source
nothing changed; the tests and the hints' names only (the bullet "The
pass with the computer's own turn", its last item).

On the branch `debian/window14` (2026-10-09) the battle's end is reached
by play alone on the port's own clock, no pokes: `m54_hq` (the second
side takes the first's headquarters: D0h, CFh) and `m57_hq` (the first
side takes the second's: D0h, CEh), both run on the original and the
same at every round's end and every tenth frame. The bullet "The end by
play on the own clock" says how the scripts were found;
`sh port/tests/endclock_test.sh` runs them.

On the branch `debian/screen` (2026-10-08) the `doskit` submodule points
at `953c83b` on doskit's branch `debian/vga-start` (its master `5e2524b`
merged in, plus the VGA's and the BIOS data area's starting state), until
that is merged into doskit's master.

The `doskit` submodule was before at `2f6fe29` (fast-forwarded from `a4b8166`,
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
    shows texts and a picture (read to its first screen only then; read
    whole and translated on 2026-10-08, "The new units' screens").
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
    `323E` and `lib_pick` (`S0C66:0414`), a library or a list name that
    is missing.
    2026-10-08: `pictures_load` is read whole, translated
    (`port/src/pictures.c`, called by `battle_begin`, which keeps the free
    end at `S3303:0028`) and compared. What it does, in order, is in its
    comment in src/HL.hints; learned on the way:
    - `S2B06:0000` is `lbm_show`, an IFF loader that draws straight into
      video memory at `screen_seg` (`DATA:0354`, A000h) unchained, a line
      every `screen_pitch` (`DATA:02F0`, 50h) bytes: PBM pictures as
      ByteRun1 pixels, ILBM ones bit by bit ORed into the planes, one
      larger than 168h x F0h unpacked as bytes. It finds its chunks by a
      word-aligned search (BODY from the segment's offset 0) and keeps
      what it found in its own code segment (`S2B06:03DC` to `03F4`). A
      run past a line's end is cut. The battle's picture is `GAME.IFF`, a
      320x200 PBM; ILBM and the large path were not met.
    - The picture is loaded at the free end without moving it, so the
      libraries after it lie over it. The palette file (`00.PAL` by the
      number at `S37FF:0112`) goes to the 320h bytes' buffer of
      `game_loop` (`[BP-0Ch]`) and its 300h bytes on to `palette`
      (`DATA:04E3`, also where `lbm_show` puts the CMAP).
    - `lib_read` (`S2C01:0006`) reads named entries of a library and
      unpacks them through `tpwm_unpack` (`S2CA0:0008`, the packer's codes
      from memory); `pictures_load` reads `LIB\BIGUNIT.LIB`'s entries of
      `bigunit_list` (on both maps one, `UNI96HIM`) to `bigunit_pics`, its
      pointers to the block `bigunit_ptrs`, whose bytes from 10h are the
      work buffer; the place moves on by F3Ch, not by what was read.
    - The far pointers at `S331F:179A`, `179E` and `17A2` (music,
      `UMAPDA?2`/`UMAPDA?A.LIB`, `UMAPDATA.DAT`) are two bytes apart where
      four are written, so each overwrites half of the next one written
      before it (as the original does; what reads them was not looked at).
    - `sfx_load` (`S2F39:00FD`) loads `MNU.AFX` and keeps its place and
      length at `S30EF:0242`/`0246`; nothing of the sound is started here.
    Compared (port/README.md): its first call on map 0 and on map 8 and
    the second on map 0 (by `attacks_run`, after a fight's picture): the
    memory, the video memory and the free end returned the same byte for
    byte. Not compared: an ILBM or a picture larger than the screen, a
    file that cannot be loaded, the -1 returns, `S2CC5:0004` (called by
    `lbm_show` for a place of 0:0; not read).
    The drawing a battle screen needs, in the order proposed for
    translating (from the calls of `game_loop`'s setup and loop as read
    so far; none of them stepped, the order of the unread ones a guess):
    1. `pictures_load` (`S02E6:2B87`) whole: the screen's picture through
       `S2B06:0000`, the palette to `DATA:00CE` (300h bytes, `far_copy`),
       `lib_read` (`S2C01:0006`), the libraries exp, bum, newshop. Done
       2026-10-08 (above).
    2. The screen's set-up at the start (`S298E:000D`, `S28D9:000C`,
       `S2974:0002`, left out of `start_setup`). Done 2026-10-08 as far as
       a function: `screen_open`, `pages_set` (`S2A10:0006`),
       `screen_clear`, `page_flip` are read, named and translated
       (`port/src/screen.c`); `screen_open`'s call on map 0 is the same as
       the original's in memory, video memory and the VGA's registers
       (port/README.md). Mode X 320x200 with two pages: A400h drawn,
       A000h shown at the start, page_flip swaps them. Since 2026-10-08
       called by the port's start (`start_setup`: `screen_open`, then
       `screen_clear`, `page_flip`, `screen_clear` as at `S066A:014D`)
       and `battle_setup` calls `pages_set` and `page_flip` where the
       original does (after it sets word 1Ch of the record behind
       `DATA:0092` to FFFFh). For
       that doskit's runtime now starts as DOS leaves the machine
       (doskit branch `debian/vga-start`): the VGA's registers are text
       mode 3's, so the retrace status runs before the first mode set,
       and `rm_load_exe` fills the BIOS's data area as the runner does
       (the CRTC's port at 40:63 was 0 before). Between `screen_open`
       and `S066A:0349` the original calls `page_flip` 73 times on map 0
       (`run.py -log S2974:0002`), the port twice: the other 71 are in
       routines the port leaves out at the start (which ones was not
       looked at), so the page words at `DATA:0322`-`0331` and
       `0354`-`0357` differ after `start_setup` (one swap) until
       `battle_setup`'s `pages_set` sets them anew. Not met: the 360-wide modes (`S298E:02B4`/`02CC`
       tables), `S2B51:002A` (with `DATA:0463`/`0465` FFFFh).
    3. `view_draw` (`S0E56:0D44`) and `marks_draw` (`0F36`) for each side,
       then `show_message(-1, side)` (`S10AF:000F`). `view_draw` done
       2026-10-08 with `ground_blit` (`S290A:0006`, write mode 1 from the
       grounds kept in video memory) and `unit_blit` (`S2931:000E`), in
       `port/src/draw.c`; compared on its first two calls on map 0 and
       on map 8 (port/README.md). `marks_draw` done too, with
       `S0E56:24ED` and `mark_blit` (`S2A1C:0004`), compared on seven
       calls. `show_message(-1, side)` done as `message_clear` with
       `S28E5:0004` and `S28FA:0000` (it fills the status line). The
       text part done 2026-10-08 too: `show_message` is whole in
       `port/src/draw.c` (side.c's state-only one and `message_clear`
       gone), with `text_draw` (`S2B45:0006`) and the 'P' way of
       `S2A6B:0008` (`S2A76:0002`, 4-bit pictures plane by plane, and
       its cutting `S2A76:0139`); the game's font (DATA:036A, map at
       036E) is of kind 'P'. Compared on seven calls with texts 33h, 4Fh,
       61h, 63h on map 0 and map 8, all side 1 (port/README.md). Not met:
       a side-0 text (none in the first 1,000 calls of m0_hq or m8_d5r
       looked at), DATA:00E8 not 0 (the cutting), a '|' or 0Dh or a gap
       character in a text, the 'X' and other kinds and negative y of
       `S2A6B:0008` (these are the sides' pointers' too, step 5). `battle_begin` now calls the five as the
       original does at `S066A:0DE8`; with that the begin part is the
       same as the original's in memory on map 0 and map 8, and its video
       memory at the end is the original's right after the second
       `show_message` (port/README.md). After that the original changes
       62,890 bytes of the drawn page (4000h-7E7Fh) in the routines of
       step 4 (`S2974:0002`, `S28DC:000A`, `S28E0:0004`). Seen: `mark_blit`
       ends with `OUT DX,AX` while DX holds its hi:lo bytes, not 3C4h, so
       the map mask stays on plane 3 after it.
    4. `S2974:0002`, `S28DC:000A`, `S28E0:0004` at the loop's beginning.
       Done 2026-10-08: they are `page_flip` (the drawn page shown),
       `page_copy` (`S28DC:000A`, new: the shown page copied to the drawn
       one through the latches, `page_size` bytes a plane, so both pages
       hold the views) and `fade_in` (64 calls of `pal_level` up to level
       FCh with `DATA:04E3`). `page_copy`, `pal_level`, `fade_in` and
       `fade_out` translated in `port/src/screen.c`; `battle_begin` calls
       the three as at `S066A:0E95`. With that the port's video memory
       at the end of `battle_begin` is the original's at `S066A:0EE6` on
       map 0 and map 8 (`begin` compares it now, port/README.md). The
       three were compared together at that one point, not each at its
       own call; the DAC's colours after `fade_in` were not compared (the
       dumps hold no DAC). `battle_setup`'s video memory is compared
       whole since `grounds_load` (step 6). `start_setup`'s is not: the routines it
       leaves out draw over both pages (about 48,850 bytes differ).
    5. In the loop: `field_draw` (`S0E56:145A`), `marks_erase` (`1217`),
       the sides' pointers (`S2A6B:0008`, `S2A6F:0002`), the bar of
       `S1571:01F2`, `unit_panel` (`S10AF:0131`), the fades
       (`fade_grey`, `fade_back`).
       `field_draw(side, field, flag)` and `marks_erase`'s drawing done
       2026-10-08 (`port/src/draw.c`: `field_put` is the one field's
       ground and unit that `view_draw`, `field_draw` and `marks_erase`
       each write out; the third argument of `field_draw` is not read).
       `field_draw` passes `S331F:4D92` (the first view) to the mark's
       table for both sides. Called where the original calls it:
       `move_step` (5 calls: the field left and the one reached, and the
       other half's), `move_done` (its end, `S10E1:0283`, always draws the
       view, its marks and the unit's fields as they were at the entry),
       `move_arrive` (kinds 2 and 5; kind 5 passes the place, field + 1,
       not the field), `timer_fire` kind 5 (the field of the timer's word
       64h + 2n, the side by bit 1 of the second view's byte 16h), the
       hover of `game_loop` (`S066A:13FB`: the aim's field drawn with 4
       or 8 set in `field_bits` for the drawing only) and `move_out`
       (`S066A:30D8`: view, marks and the cursor's field). `marks_erase`
       now has its field and side and is called with them by `move_aim`
       and `field_menu`. Left out: the call in `attacks_run`'s blast loop
       (`S02E6:0BD9`, with the two maps swapped and the view's state 0
       for the drawing; it belongs with the blasts' pictures, step 6).
       Compared (port/README.md) on five `field_draw` and three
       `marks_erase` calls. All 79 `field_draw` calls of m0_hq to 120 s
       reached the RETF at `1725` (none was outside its view).
       `unit_panel` (`S10AF:0131`) done 2026-10-08 with the flags
       (`S2532:1A8F`, `1BC3`), the number (`S1571:0134`) and the 'U' way
       of `S2A6B:0008` (`S2AAF:0000`, uncut only) for the experience
       pictures (the table at `S331F:01F3`; all six of kind 'U'); called by
       the loop's hover (`S066A:148B`, only after the checks that let the
       aim be shown pass). Not called yet: its four calls in `attacks_run`
       (`S02E6:07C7`, `0AE6`), in the fight's drawing with the maps
       swapped and pages flipped, left out with it. Compared on nine
       calls, none with experience above 0 or a unit with bit 0 of word 4
       (so the 'U' blitter and the standing flag are not checked).
    6. Last the fight: `fight_panel` (`S131D:178D`), the fight's pictures
       (`S2532`), and `S02E6:323E` with `lib_pick` for the parts' list.
       `S02E6:323E` done 2026-10-08 as `grounds_load` with `lib_pick` and
       `ground_store` (`S28F5:0006`), in `port/src/pictures.c`, called by `battle_setup` after `page_flip` as the
       original does. It loads partS/partW (sorted) to the free end, the
       map's file and the file named at `S331F:0264` behind it, and
       stores in video memory from AC58:0 on, 90h bytes each, every
       ground whose number is the low byte of a word of those two files
       (presumably the grounds the map uses). Learned: `lib_pick` writes
       the places into its last argument (the block's beginning, the
       work buffer) and then copies the whole list to `S331F:02E7`, so
       the entries of grounds not stored keep what the work buffer held
       from the loading; the free end does not move on, the side lists
       come over the files. With it `battle_setup` is the same as the
       original's in memory and in its whole video memory on map 0 and
       map 8 (port/README.md). Not met: a file that cannot be loaded, the
       call after `load_game`, a winter map (partW).
       `fight_panel` done 2026-10-08 (`port/src/draw.c`) with `big_unit`
       (`S131D:0C50`), `panel_whole` (`1F0D`), `pic_draw` (`S2A6B:0008`
       but its 'X' way), `pic_half` (`S2A95:000E`: a 'P' picture at -y
       is drawn at half size; the large units of places 0 and 3 are) and
       `pic_unpack_draw` (`S2A6F:0002`); the panel's picture is packed
       (TPWM) and unpacked at the free end. Compared on the first fight of
       m8_d5r with pictures and without (a poke of `no_fight_pics`): the
       same in memory and video memory but for the sound's segment, the
       timer count's high byte and the key's record (its waits and sounds
       are left out). `attacks_run` now draws where the original does:
       with the maps and the units' bytes 0Fh/10h exchanged, every field
       marked (80h for side m 1, else 40h) but the two units' fields,
       `view_draw` and `marks_draw` of side m, the two `unit_panel`s,
       the pages, word 1Ch of the screen's record 0 around the panel
       (`pages_set`), `fight_panel`, after a fight's pictures
       `pictures_load` and `libs_start` (to the places the battle's
       beginning had: game_loop's [BP-1Ch] and [BP-14h], now kept as
       `game_mem.pics` and `libs`, checked against an entry dump of
       `attacks_run`), without pictures the panels again and each
       blast's field `field_draw`n with the maps exchanged and the view's
       byte 17h 0; at its end (`S02E6:19B8`) the whole `pictures_load`
       instead of its one digit, which loads `rnd` once more, so the
       `rnd` record's list now is the original's after a fight (it
       differed in `m8_site` from pass 4543 when only the libraries
       after the pictures were loaded). Not translated: the fight's
       pictures (`S2532`, its `rand_next` calls with them), `S2AF3:000A`
       (after the panels without pictures), `S02E6:225C` (a blast below
       `blast_time`), the panel's sounds (`S131D:1FF0`). Not met: a kind
       with 40h in word 4 on the panel, a fight of the first side's
       half (x 0), the blasts' drawing (no unit destroyed without
       pictures in the runs compared). `attacks_run` as a whole was not
       compared in video memory; its state through `orders_test`,
       `loop_test` and `play_test.sh m8_site` (the same).
       `S2532` done 2026-10-08 (`port/src/fightpics.c`, names in
       the hints from `fight_pictures` on, port/README.md "A fight's
       pictures"): read whole with its blitters and translated, the same
       as the original on m8_d5r's two fights with pictures in memory
       (`rand_words` too) and video memory. Learned: the pictures are
       driven by the fight's file (`S3AEE:0061`, its tables at its words
       0, 2, 4, 6, 8, 0Ah): each side's list of figure records (22h
       bytes: flags, figure, first and last frame, x/y and scale at both
       ends, the picture or a sequence of them); the frame loop moves them
       by `long_mul`/`long_div`, and a volley's last frame (flag 10h)
       calls `volley_end`, which shares the other side's losses still to
       show among the volleys left and draws `rand_next` for it
       (`figures_fall` then marks the nearest standing figures by x). So
       the number of calls depends only on the file, the counts before
       and after, and the key that may end the frames early, not on the
       drawing. Drawing: `sprite_draw` keeps what each figure covers in a
       list per page in video memory (`DATA:0322`, `under_save`),
       `page_flip` swaps the lists and `unders_restore` puts the other
       page's back after the flip. The 'X' way of `pic_draw`
       (`S2957:0002`), `S02E6:225C`, the bar (`S1571:01F2`) and
       `fade_grey`/`fade_back` (252 lines each; they change the DAC and
       write a grey palette behind `S3303:0028`) are still to do; so are
       the pictures' sounds (`S2F39`). `attacks_run` calls it where
       the original does, and on the port's own clock with only the log's
       seeds `play_test.sh m8_d5r seeds` keeps `rand_words` the same to
       round 4 (the last dumped): of the list "What still has to come from
       a log" item 5 (R) is no longer needed; the seeds (S) and the key
       that may end the frames (its time) still are.
       The rest done 2026-10-08 (branch `debian/window`, port/README.md
       "The last drawing of a battle"): `fade_grey`, `fade_back` and
       `pal_blend` (`S2A62:0008`; the fades end one step short of the
       palette, step 1Fh of 20h), the bar (`S1571:01F2`, the computer's
       progress, `S1C74:0354`), the blasts' pictures (`S02E6:225C`) and the
       'X' way (`S2957:0002`, met in no run of the 22 key scripts), all
       compared at their calls with the DAC (the runner's new `-dac`;
       `-vgastate` prints only 32 colours). Learned on the way: the
       palette of `fade_in`/`fade_out` lies at `DATA:04E3` itself (the
       port had read a pointer there, and its screen stayed black: no
       comparison had the DAC); a cursor is taken off by drawing its field
       again (`S0E56:1013`, at `S066A:1EE2` before `view_cursor`), each
       pass's end draws both cursors and flips and copies the pages
       (`S066A:441F`); pic_draw's fourth argument is a save place, not
       colours: with it the 'P' blitter keeps what lies under the picture
       (`under_save`), used for the cursor in the small map (bit 2 of a
       side's word in `sides`, put back by `S2904:000E`; not translated).
       The battle now runs in the window (`-map N`, `play_live` in
       `port/src/play.c`).
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
    - The computer as the first side in the port's passes (2026-10-09,
      Windows, branch `win/computer`): the maps 24 and 30 (side 0 the
      computer's, side 1 a human who only ends its halves,
      `port/tests/keys/m24_quiet.keys`, `m30_quiet.keys`, after
      `play_quiet` of `computer_dumps.py`), 400 s each, run by
      `loop_test.sh` and `play_test.sh` against the original: the same at
      every pass compared but `play_test.sh m30_quiet`, which did not run
      (port/README.md: on the original's clock the port comes to no pass
      on map 30; left to `play.c`'s owner). Nothing of `computer_turn` or
      `loop.c` needed a change for the first side. Map 30's battle begins
      at 56.6 s, so its script begins at 70 s, map 24's at 52 s.
      - What was wrong was the test: `loop_test` starts from a dump and
        never set `game_mem`, so `pictures_load` after a fight failed at
        its first load (its palette's buffer 0:0) and returned 2 before
        it wrote the music's digit (`S331F:0284`, by the ratio of the
        sides' unit counts). With the computer first the digit changed
        ('2' to '3' after side 0 built three units); on the maps 8 and 0
        it did not, so the earlier runs agreed by chance. On this machine
        `m0_carry` differed on `master` for the same reason (not checked
        whether also on the others). `loop_test` now takes the far
        pointers from `game_loop`'s frame in the dump (SS 3EA4h, BP
        0FEAh) and the game's files from `../game` or `$DOSKIT_GAME`.
      - With that the fight's pictures run in `loop_test` too, and play
        every volley drawing from `rand_words` (`volley_end`), while the
        original's in those runs called no `rand_next` (`loop_dumps.py`
        refuses a run that does; why the original's did not was not
        looked at: presumably the pictures end early or no losses are
        left to share out). The test gives words 4 and 6 back at the
        pictures' end (`pics_hook`), as `play.c` does.
      - Not done: both sides the computer's. The menu has no item for it
        that was found (word 2 of `game_state` 4); a poke of both sides'
        word 0 was not tried. The computer's thinking bar
        (`S1C74:0354`) is still not translated. (Wrong: it was
        translated on `debian/window`; read whole and compared on
        `mac/thinkbar`, the bullet "The pass with the computer's own
        turn".) (Both sides: done since, the next bullet.)
    - Both sides the computer's (2026-10-09, Windows, branch
      `win/computer2`). The menu has it: GAME SETUP (third row of the
      main page) shows player one and two in its second and third rows
      (HUMAN, COMPUTER; items 7 and 8), and with both COMPUTER the menu
      leaves 2 in word 0 of both sides and 4 in word 2 of `game_state`
      (seen in a dump after START). Its first row, EXT FIGHT ON/OFF, is
      `no_fight_pics` (OFF gave 1 at `S37FF:0115`). A click counts for
      the item the pointer was on in the pass before: without moving the
      pointer onto a row first, the click toggled the row the pointer came
      from (seen: GAME SETUP's own row on the main page, the third).
      - `loop_dumps.py` `mNN_both`: after the code, GAME SETUP, fights
        OFF, the third row to COMPUTER, EXIT, START (only for maps from
        18h on, where the code leaves the second side human; below that
        not checked). With the fights' pictures on, the original's run
        called `rand_next` outside `rand_range` (map 24 pass 3721, map 30
        8639: presumably the pictures, not looked into), which the test
        refuses, so they are off.
      - Map 30 stops on its NEW UNITS pages with both sides the
        computer's until a key comes (two spaces, 42 and 47 s; the
        battle begins at 49.6 s); map 24 needs none (39.0 s).
      - Compared with `loop_test`, 600 s each: map 24 7,760 passes, 10
        round ends (`attacks_run`), the same at the 22 passes compared
        (9 round ends and the pass after each); map 30 8,704 passes, 3
        round ends, the same at the 10 compared (all three and the pass
        after). A round's end needs no key here: `loop.c` already takes
        both sides' word 0 at the round's end, nothing in the port
        changed. Not done: `play_test.sh` (the port's whole program on
        the original's clock; `play_dumps.py` has no `both` path), the
        fights' pictures on, maps below 18h.
      - On Windows without cc: the `.sh` tests ran under Git Bash with
        `CC` set to a small wrapper that hands the gcc-style options to
        MSVC's cl (vcvars64, `/MT`, `advapi32.lib shell32.lib
        winhttp.lib`); `loop_dumps.py` now writes `loop_test.args` with
        `/` and LF, which `sh` needs.
      - The queue (8) and run (2) phases compared in the pass
        (2026-10-09, Windows, branch `win/computer3`): `loop_dumps.py`
        with `STEPS=N` picks passes in which `computer_queue` gave 1
        and in which `computer_step` gave the list's end (a second run,
        `turns.txt`). On maps 24 and 30 (both sides the computer's,
        600 s) a step is queued about every 50 to 80 passes; 12 such
        passes and the ones after each were the same on each map
        (port/README.md). The cc wrapper was rewritten for this run in
        ignored `build/ccw/cc` (same idea as above). Next: the script
        play (phase 1, `computer_input` alone while a script plays):
        choose passes inside a played script, with the codes 81h, 84h
        and the diagonals still never seen. (Done, the next item.)
      - The script play (phase 1) compared (2026-10-09, Windows, branch
        `win/computer4`): `loop_dumps.py` with `SCRIPT=N` picks passes
        in which a side only played a code of its script (no
        `computer_step` in its part), N per code; `loop_test` now sets
        the computer's played word beside the original's every pass.
        Maps 24 and 30, both sides the computer's, 600 s: the words the
        same on every pass, the state the same at every dump
        (port/README.md). Learned: what a script plays alone is only
        the codes 1, 2 (up, down) and the gestures 80h, 82h, 83h; the
        codes 3 to 5 never alone (the words 4, 8, 10h are seen,
        presumably from passes with a step; not checked), and 81h (button and
        down), 84h (button twice) and the diagonals 6 to 9 were never
        played in these runs (no word 12h or 5, 6, 9, 0Ah in
        `inputs.txt`). Next: a map or state where they come up (not
        searched), and both sides in `play_test.sh`.
      - The paths no battle reached (2026-10-09, Windows, branch
        `win/computer5`): each is reached and compared or shown not to
        be reachable. Why no battle reaches them, read from memory at
        a battle's start on all 48 maps for one player (`kind_worth`
        and `unit_dat` there; scratch `build/c5/`) and the 24 maps for
        two with both sides the computer's for 1200 s (`plan_approach`,
        `computer_plan`'s state 4, `plan_to_site` and the step of kind 5
        never; the step of kind 6 on ten of them):
        - `plan_approach` (the plan 7): `computer_plan`'s state 4 gives
          the plan 7 to as many units as byte 4 of the site taker's
          `kind_worth` says. The only kind that takes sites (1 in word
          2) and has 20h there on these maps is 12h on map 22, and its
          byte 4 is 0: state 4 runs and gives no plan. (On the maps
          for two players kind 1 has 21h and 2 in byte 4, but no unit
          of kind 1 stood on map 57, the one looked at.) Reached with
          byte 4 poked to 2 (`S39B6:0070`) on map 22.
        - `computer_plan`'s state 6 and `plan_to_site` need a unit with
          4 in its word 6. Word 6 is only written by `unit_add`, from
          word 11h of the kind in `unit_dat`, and no kind 0 to 32h has 4
          there (no write to `S331F:18A2` in the listing, only tests;
          writes through other pointers not looked for beyond
          `unit_add`). Reached by pokes: state 6 on map 0 with 4 set
          in word 6 of unit 1Ch, of no side; `plan_to_site` on map 11
          with 4 set in word 6 of unit 24h, which sits in the
          computer's carrier 41h at its assessment at 918 s, and that
          carrier's plan 9 made 0 (with the plan 9 and a way to a site
          the carrier is skipped before the test).
        - `computer_plan`'s state 2 is unreachable: state 1 only picks
          an aim whose byte 5 is the kind at hand (1 to 5) and goes to
          state 2 for a kind other than 1 to 4; the aims are written by
          `computer_assess` (kinds 1, 3, 4) and state 3 (kind 2) only,
          and the count goes back to 0 at state 0Ch.
        - The step of kind 5 is unreachable: the nine calls of
          `step_add` put the kinds 1, 2, 6, 4, 2, 7, 3, 1, 1 into
          `step_draft` (the byte stored before each call, read in the
          listing), and nothing else writes `computer_steps` but its
          emptying. `cursor_script`, the handler's body, runs inside
          the other steps and is compared there.
        - `step_site_menu` (kind 6) was compared on single calls before
          (`late`, map 26); now also in the passes of map 26
          (`m26_quiet`, 17 calls at 290 s). Its state 6 is the table's
          seventh entry, never reached: state 5 sets byte 47h to 0.
        - `computer_dumps.py late` takes pokes now (`POKED`, run.py's
          `-poke` with every run of the battle, and a time from which
          calls are taken). Learned: a dump at the poke's own address
          (same hit) already holds the poked bytes.
      - The map black after a round end (2026-10-09, Windows, branch
        `win/blackmap`): played live with the fights' pictures on, a
        round end whose fights showed no pictures but took a site left
        both views black, the panels right. `fade_grey` (or `fade_out`)
        ran at the round end's start (S066A:3CC5); order 4
        (`order_take`, S02E6:11B1) set `site_taken` and `take_picture`,
        so `game_loop` left out its own `fade_back` / `fade_in`, the
        original's order 4 having shown the site and brought the
        colours back itself; the port's order 4 does only the data, so
        nothing did. With pictures shown their end brings the colours
        back (why the tests never saw it). Seen with temporary logging
        of the DAC's colours 0 to 40h (removed): `m0_carry` with
        `-nopics` at its 4th round end, `m8_site` and `m0_hq` with the
        pictures on, each a grey fade with a site taken and no
        pictures. The fix in `state_done` (loop.c) remembers which fade
        ran, `fight_pictures` sets `pics_shown` once its pictures are on
        the screen, and with no pictures shown the counterpart comes
        after the redraw. Still missing: the original's presentation of
        the site taken in order 4 (view drawn, flip, waits, its fade);
        the port shows nothing there. A dark fade with `take_picture` and
        no pictures shown was not met in a run (the branch is written,
        not seen). (Done, the next item; the fallback is gone again.)
      - The site taken's presentation (2026-10-09, Windows, branch
        `win/sitepic`): order 4 draws and fades as the original
        (`order_take`, port/README.md), `event_picture` CAh/CBh is
        `site_picture` (events.c `pic_site`, using `vdif` and `res_load`
        as the other pictures do). Learned:
        - `view_cover` is called with [BP-2], the side that takes the
          site (the hints said the loser): the taker's half is covered
          and the picture drawn over it, the other half shows the other
          side's view on the site. Its picture is the far pointer 30h
          bytes into the list S331F:0167 points to (picture 0Ch, the
          hints said 30h). `view_cover` was in orders.c at first;
          it is in draw.c since branch `debian/draw-tidy`, unchanged.
        - `site_picture(x, y, place, work)`: x 0 for CAh, A0h for CBh,
          y 0; the second of its three files is not used, the third is
          music (`music_start(it, 0, FFh)`), the frames are drawn in the
          game's colours (no palette taken), then `wait_frames(C8h)` and
          `music_fade(14h)`. The music is played in the port (the other
          event pictures still leave theirs out).
        - Times in the original (m0_s4, round 4, run's seconds): order
          4 at 128.036, `event_picture` 130.452, `site_picture` 130.623,
          its files loaded 130.957, 18 frames to 132.493,
          `wait_frames(C8h)` to 135.361, the round's end 139.430. The
          1.84 s once measured for the frames were the loading and the
          frames together.
        - The original's frames of a site picture can be dumped at
          `S1A39:198C` (each frame's start, `play_dumps.py` `sNNNN`).
        - Found, not looked into: `S331F:1838` (`fight_rec` + 1Ch) is
          FFFFh in the port from the start of a round end (before
          `attacks_run`) and 0 again at the next pass; the original has
          0 there during the site's picture.
        Next: the player's live check of the picture; a site taken after
        a fight (the `fade_in` path) and with the fights' pictures off
        are written, not run.
      - Music slower during fades (the player's report, live window):
      - The dark fade of a site taken (2026-10-09, Windows, branch
        `win/darkfade`). Order 4 brings the colours back with `fade_in`
        (S02E6:1318) when the fights' pictures are on and a fight came
        before in the same `attacks_run` ([BP-0Eh], S02E6:08F5), else
        with `fade_back` (131F); with the pictures off `fade_back`
        (1335) only without a fight before, nothing after one
        (`src/HL.hints` at S02E6:1305). `order_take` had all three
        already; no key script reached the first or the third. Found:
        - In the original with both sides the computer's (`m24_both`,
          `m30_both`, the `mNN_both` setup of `loop_dumps.py`): m30
          with the pictures off takes a site right after a fight at
          1450.355 s (11B1 to 1343 in 0.512 s, no fade). With the
          pictures on m30 takes none up to 1800 s; with `no_fight_pics`
          poked to 0 at `attacks_run`'s 7th call (`S02E6:000F#7`) it
          takes one after a fight: the fight's end 08F5 1654.305,
          11B1 1654.318, `wait_frames(1Eh)` 1654.390 to 1654.830,
          `fade_in` to 1343 at 1655.352. m24 with pictures: the grey
          path (11B1 173.351, 12E7 173.930, 1343 175.754).
        - After a fight the view takes 0.073 s to draw (11B1 to 12E7),
          not T_VIEW's 0.58: the port's clock (play.c) counted nothing
          there; now `T_TAKE_VIEW` 0.085 s (08F5 to 12E7). The port's
          `fade_in` is 0.5207 s against 0.5223.
        - Compared with `orders_test` from the original's dumps at the
          orders loop's entry (S02E6:0E9C, memory, video memory, DAC)
          and its exit (19B8): video memory and the DAC at 12F0 and
          1343 are the same on the dark path (m30) and the grey path
          (m24). orders_test had no `game_mem` (its `fade_back` read
          address 0) and no `fought`; it takes them from attacks_run's
          frame now. Still different on both paths: 4 bytes,
          `lib_pal`+10h/11h and `lib_rnd`+10h/11h (S331F), written by
          `libs_start` after the picture; not looked into.
        Not checked: the dark path on the port's own clock in a full
        play (no key script for one person reaches it; the both-sides
        computer runs cannot be played by the port with pictures, they
        draw from rand_words), the pictures-off path after a fight
        compared in video memory (seen in the original only), a live run.
        The scan of every key script for a site taken after a fight
        was stopped for lack of memory after the m0 scripts and
        m8_site/m8_long (all without a fight before).
        not measured live (a window run with `-keys` does not run in
        real time; it needs a player). Read from the code: in the live
        window `screen_wait` (play.c) shows a fade's steps at real time
        but does not move the port's clock on, so `snd_clock` makes no
        music interrupts during the fade; `own_fade` / `own_retraces`
        make them afterwards at once. The audio thread
        (plat_win32.c) waits on waveOut's event and only shares a
        short critical section, so starving it is not the likely
        cause (presumably; not checked). Done on `win/fademusic`
        (2026-10-09): `screen_wait` calls `snd_clock(now)` after each
        picture; with the window's device the interrupts go by the
        samples played, so the tune keeps its pace during the wait.
        Only sound routines run there. Not heard yet (the player's
        check); headless suites unaffected (the hook is live only).
      - The fight panel's buildup (2026-10-09, Windows, branch
        `win/fightpanel`). The player saw in DOSBox that before a
        fight's pictures the original builds the units' figures up
        step by step on the panel, the port all at once. Read from
        `fight_panel`'s listing (`S131D:178D` to `1F0C`): the drawing
        of the port was already the original's (six steps, each the
        figure i of both units at the step's place of `S331F:02B9`,
        then `page_flip`, `page_copy`, `wait_frames(6)` at `1C9A`;
        `wait_frames(0Ch)` at `1CB2`, then with pictures
        `wait_input(96h)` and `fade_out`, without them six steps back,
        `wait_frames(6)` at `1EE7` each, and `wait_input(46h)`). The
        cause was the port's clock: `round_hook` (play.c) counted all
        of the panel's waits at `ORDERS_FIGHT`, before `fight_panel`
        drew anything, so the live window waited first and then drew
        the six steps without time between them. Measured in the
        original (m8_d5r's first fight, with pictures, the runner's
        breaks): entry t=132.364, `fade_in` over +0.624, step 1 at
        +0.630, then one step each 0.1141 s (8 retraces), the 0Ch wait
        at +1.298, `wait_input` at +1.484.
        For Debian's review, the hunks in its files (a one-time
        exception the lead granted; nothing else in them changed):
        `port/src/draw.c`, inside `fight_panel` only: after `fade_in`
        and after `fade_out` `orders_hook(ORDERS_FADE, 0)`; after each
        step's `page_copy` (both loops) `orders_hook(ORDERS_MARK, 0)`
        and `orders_hook(ORDERS_WAIT, 7)`; after the first loop
        `ORDERS_WAIT, 13`; `ORDERS_INPUT, 0x96` before `fade_out` and
        `ORDERS_INPUT, 0x46` at the end without pictures; its comment
        says so. `port/src/play.c`, `round_hook` only: `ORDERS_FIGHT`
        keeps `T_VIEW`/`T_NEXT` and `T_PANEL` and nothing of the
        panel; new cases `ORDERS_MARK` (`own_busy(T_MARK)`, the step's
        drawing) and `ORDERS_INPUT` (`own_wait_input(n)`); the unused
        `i` gone. Outside them: `hl.h` names the two new events;
        `orders.c` sends `ORDERS_WAIT, 0x1F` after the panels again of
        a fight without pictures (`S02E6:0AE6`, the original's
        `wait_frames(1Eh)` behind `unders_restore`), which
        `round_hook` used to count before the panel; the test programs
        that link draw.c without orders.c (computer, load, move, side,
        view) link `tests/orders_hook.c`, which holds the hook and, with
        `LOAD_TEST_STEPS`, writes the video memory at each step. The
        sum of the times is unchanged; only where they fall moved.
      - The newspapers (2026-10-09, Windows, branch `win/papers`). The
        player saw in DOSBox newspapers before and after the battles
        that the port never showed. `event_picture`'s kinds 0 to 2Fh
        are them: `S1A39:059D` (`newspaper`) twice, pages 2n and 2n +
        1 (n the kind, or the kind less 18h from 18h on: the maps 24 to
        47 share the newspapers of 0 to 23). `battle_after` already
        passed these kinds (the map's number with one side the
        computer's and a map below 30h; `event_picture` printed "not
        translated"). Read whole and translated (`src/events.c`;
        `src/HL.hints` names it all):
        - A page's record of six bytes at `S3812:05A0`: the digits of
          its files' names (text `TXT\aabb.TXT`, pictures
          `TXT\aabb.DAT`, background `TXT\Hcc.LBM`), flags (4: the
          pictures), a picture of `event_picture` before it (pages 14,
          21, 44: C9h, CCh, C8h; maps 7/31, 10/34, 22/46), the tune
          (`TXT\ZWTXT1.ADL` and on). Fonts `TIMES_8`, `TIMES_10`,
          `TIMES_12` (`.NEW` a "PLIB" library of the characters'
          pictures, `.CHR` their widths), palette `TXT\ZWTXT.PAL`.
        - The text is set by the routines of `S2D47` (nothing else
          calls them) a line at a time into a buffer of four planes,
          two pixels a byte (the glyphs and the planes' output in
          `S2F11:00E9` and `01F4`), 30h rows below a window of 118h by
          B4h at 14h, 0Ah, and scrolled up a row a frame of 8 timer
          counts; what lies under the window is kept and put back each
          frame (screen_keep, screen_put; events.c has its own copy of
          the two, which fightpics.c keeps static). Its codes: `|` a
          hyphenation point, `^Fn` font, `^Ac` alignment (L, R, C, B
          both sides), `^Nn` empty rows, `^Rn` a number for newspaper
          (9100 to 9102 where the next picture goes, 9200 + k picture
          k of the library, rising from the window's bottom, 9999 the
          end). The key of entry 0 of the table at `S331F:02D3` ends a
          page (presumably Esc; not checked), that of entry 1 waits for
          a key (presumably a pause key; not checked; the port's own
          clock takes it as `wait_press`, `EV_PAUSE`).
        - Times (m0_paper, map 0 left by Esc and `z` at once):
          `event_picture` at 50.051 s, page 0's `S1A39:059D` 50.223,
          its first frame 51.781, 994 frames of 0.1255 s to 160.971,
          its end 161.082 (`S1A39:0A45`), page 1 at 164.439 (first
          frame 165.926, 833 frames, end 257.457), `S066A:4728` at
          260.820. The port's own clock counts 1.01 s for a page's
          loading (`EV_LOAD_PAPER`) and 2.11 s after its
          `music_fade(14h)` (`EV_PAPER_END`; presumably the fade's
          waiting, not checked what in it takes the time).
        - Compared (`tests/keys/m0_paper.keys`; `play_dumps.py` now
          dumps `nNNNN` at every tenth frame of the newspapers and of
          their pictures, the port's `-evdump` the same): all 183
          dumped frames of both pages have both pages of video memory
          the same as the original's, and the window's record
          (`S3812:0D14`) and the fonts (`S3B76`) the same. Video memory
          differs only behind the pages (offsets 7E84h on), which the
          original has filled since the battle's first pass and the
          port has not (not looked into). The BIOS's ticks: the port's
          first frame 12 ticks (0.66 s) early, within 6 to 12 through
          both pages; the 0.66 s lie before `event_picture` (the
          battle left by Esc; not looked into).
        - The pictures before three pages (`pic_zeppelin` C9h,
          `pic_ship` CCh, `pic_richt` C8h; sprites of 1Ch-byte records
          in S3812 over streams, `sprite_draw` keeping what they cover)
          compared the same way: `m31_paper` (map 31, page 14: C9h's
          53 + 50 frames, then 1,150 of the newspaper), `m22_paper`
          (map 22, page 44: C8h's 140 + 80, then 1,098), `m34_paper`
          (map 34, page 21: page 20's 617 frames, CCh's 130, then
          page 21's): all 126, 132 and 148 dumped frames have both
          pages of video memory the same. Their frames start at
          `S1A39:1BED`/`1D0F`, `1FCF`/`221B`, `169A`; in two loops the
          original sets the count after `unders_restore`, so the
          port's `EV_PAPER` comes before it there (where the dump is).
          Their loading on the port's own clock: CCh 0.66 s (measured
          against page 20's last frame in the same run), C9h 1.04 s
          and C8h 0.55 s (estimates: the first frame's lead less the
          12 ticks m0_paper has before `event_picture`, the same
          battle's end by Esc in both). The port's frames of C9h and
          C8h are 2 to 3 ticks shorter over their 100 to 220 frames
          (presumably the original's `page_copy` and stream decoding in
          each frame, which the port does not count).
        - `play_dumps.py` reads a script named `m0...` as map 0, so a
          script for map 7 must not be `m07_...`: map 31 shares its
          newspaper (`m31_paper`). Map 10's code (and map 34's) START
          shows screens of new units first, which wait for a press; map
          34's script presses space at 42 and 44 s (the port's play
          passes over keys before its start). The runner's `-shot T
          FILE` showed it.
        - Not done: the newspapers' music is started and faded
          (`music_start`, `music_fade`) as the site's is, not listened
          to; the pause key not run; the original's clock (`-clock`):
          the newspapers' frames have no log line, so there the port
          runs them without waiting (not run).
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
      Read whole and compared 2026-10-09 (Mac, branch `mac/thinkbar`;
      `computer_progress`, `bar_draw`, `rect_fill` and `line_fill` are
      names in `src/HL.hints` now):
      - It writes its frame of 12h bytes and nothing else. The table of
        plans is `unit_plans2` for the side's `attack_pick` 2, else
        `unit_plans`; counted are the units 0 to F0h that are not a
        second half (40h without 80h in word 4), have none of C000h in
        word 4, not 20h in word 6, none of 380h in the kind's word 2 of
        `kind_worth` and are the side's by `same_side`; of those the
        ones with byte 8 of their plan 0 are taken off: 64h less their
        number * 100h / (count * 100h / 64h), kept to 0 to 63h, 0 with
        no unit counted. The translation in `plan.c` was there and reads
        the same line for line; nothing of it needed a change.
      - `game_loop` is its one caller (`S066A:11C1`), after the pass's
        turn or thinking loop and its text, while the phase has
        20h or 40h and the side's `attack_pick` is 1. It keeps the value
        doubled at `[BP-42h]` and the side's x (0 or A0h) at `[BP-44h]`
        and calls `bar_draw(x + 17h, C3h, the colour at S331F:4C85, the
        back at S331F:4C87, 1, value * 2, C8h)`. `bar_draw` halves both,
        so the bar is the value in pixels of 64h.
      - The rectangles have both ends: the back is 65h pixels on two
        rows (y C3h and C4h), 202 bytes of video memory changed at
        every call, the value 0 too (presumably because the page drawn
        is the other one each pass: the dumps have them at two
        places; not looked at further).
      - In the runs the value rises while the side thinks: `m8_d5r`
        (300 s) 487 calls, 0 to 45h in the first round; `m0_hq` (300 s)
        768 calls, 0 to 55h; `m24_quiet` (400 s, the computer first)
        754 calls, 0 to 5Dh (`run.py -log S1C74:0519`).
      - Compared (`port/tests/thinkbar_dumps.py`, `load_test.sh` with
        `think` and `progress`; port/README.md "The bar of the
        computer's thinking"): 13 calls, each from the call of
        `computer_progress` to behind `bar_draw` and the routine alone,
        nothing different.
      Not verified: the line in `loop.c` itself was not run by the test
      (`computer_part` is static and `loop.c` was not to be touched; the
      test has the same call, and the line reads the same as the
      original's); the bar in the window by eye; a side whose
      `attack_pick` is 2 (the routine then takes `unit_plans2`, but
      `game_loop` calls it only with 1, so that way was met in no run);
      the two ways out of range (below 0, above 63h), reached in none of
      the 2,009 calls logged.
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
      word at `DATA:0418` names; both were not translated then (the mouse
      rests in the middle in the scripted runs and gives nothing; both
      are translated and compared since `mac/options`, the bullet "The
      mouse and the joysticks as a side's input").
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
    Since done (the two bullets below): a fight and a site taken take
    their time on the port's own clock. Not done: the retrace's period is the runner's
    for the game's screen mode, not read from the game; `pit_del` and
    `pit_period`'s callers; `input_mouse` and `crit_error` were read
    only.
  - What a round's end with fights or a site taken lasts (2026-10-08,
    Mac; read in `HL.EXE` and counted in three logged runs of the
    original: `m8_d5r` with and without fight pictures, `m0_hq`). Named
    in `src/HL.hints`: `pal_level`, `fade_in`, `fade_out`,
    `inputs_release`, `event_picture`, `site_picture`, `fight_pictures`,
    `fight_frames`; `site_taken`, `take_picture` and `fight_panel` have
    what was read added.
    - `S28E0:0004` and `002B` are the screen coming up from dark and
      going dark: 64 calls of `pal_level` (`S2AFE:000C`), which sets the
      256 colours at a level and waits a scan line a colour with
      interrupts off; 0.52 s each in the runner. They read and write
      nothing of the game's state.
    - `site_taken` at a round's end does not say that a site was taken:
      game_loop sets it to whether the side whose attacks are run has
      attackers. Without them the screen goes grey and back
      (`fade_grey`, `fade_back`, 96 retraces each); with them and fight
      pictures on it goes dark before `attacks_run` (`fade_out`) and
      comes back after it (`fade_in`, unless `take_picture` is set);
      with pictures off nothing is faded.
    - A fight with pictures, in order (retraces counted; the times
      between are the runner's for computing and drawing, measured):
      0.59 s after `attacks_run` begins (the view drawn on the
      attacker's field; 0.10 s only for the second fight of the run
      without pictures) `fight_panel`: 0.10 s, `fade_in`, six marks on
      the map with `wait_frames(6)` each (8 retraces a step with the
      drawing), `wait_frames(0Ch)`, `wait_input(96h)` (51 waits of 4
      retraces, 2.91 s, when nothing is pressed; it clears the first
      side's button and the key at each), `fade_out`; then
      `fight_pictures`: `clock_seed(0)` at once, 0.53 s of loading, the
      key cleared, 749 frames (13.6 to 13.7 s; a key ends them at once,
      a side's button does not), the colours down (0.56 s);
      `pictures_load` and the view again (0.69 s). After the last fight
      1.52 s to game_loop's `fade_in`. Two fights: 43.75 s from the
      part's beginning to the next (`m8_d5r`, pass 2265).
    - A fight without pictures: the same panel without the fades, the
      six marks forward, `wait_frames(0Ch)`, six back, `wait_input(46h)`
      (24 waits of 4 retraces), the two units' panels and
      `wait_frames(1Eh)`; with a unit destroyed the blasts' turns
      (`S02E6:0B61`: `wait_frames(2)` a turn; its wait on the pass's
      count of ticks never waits, the count is not set back there;
      not seen in a run), then `wait_input(50h)` (27 waits). Two
      fights: 11.04 s (`m8_d5r` with `no_fight_pics` poked, pass 2265).
    - A site taken (order 4, `S02E6:11B1` to `17C3`): the view on the
      site and its mark (0.59 s), `wait_frames(1Eh)`, then `fade_in`
      when a fight came before in this `attacks_run`, else `fade_back`
      (with pictures off `fade_back` only without a fight before);
      `take_picture` 1; `event_picture` with CAh or CBh:
      `inputs_release` (the key and both sides' buttons cleared, 6
      retraces at least twice, longer while something is held), then
      `site_picture`: its frames (1.84 s in the run; the number is in
      the picture's stream, not read), `wait_frames(C8h)`; then 3.04 s
      (`S2F39:1058(14h)` and the battle's pictures loaded again,
      `S02E6:3484`; not read which of the two takes it), the site's
      records changed, `wait_frames(32h)`. 11.37 s from `attacks_run` to
      game_loop's end of the round in `m0_hq`, pass 2431.
  - The port's own clock in a round's end, and the comparison round by
    round (2026-10-08, Mac; `port/src/play.c`, `orders_hook` in
    `port/src/orders.c`, `sh port/tests/play_test.sh TAG rounds`;
    port/README.md has the numbers).
    - The clock now waits what the bullet above lists: counted where the
      game counts (retraces, scan lines, `wait_input`'s steps with their
      clearing of the button and the key, `inputs_release`), and the
      runner's measured times where the original computes, loads or
      draws (the `T_` numbers in `play.c`). Those are not the game's: a
      port that draws will take its own time there, and they should go
      when it does. With them a round's end lasts within 0.1 s of the
      original's in every run looked at (22 to 52 s with pictures, 10 to
      35 s without, 11.4 s with a site taken).
    - Pass by pass the port's own play cannot be laid beside the
      original's past the first round's end even without taps: its
      passes are shorter while the computer plays (one call of the
      timer's routine against the original's drawing), so the computer's
      half is over sooner and the script's next F1 ends the round at
      another second (`m8_wait`: round 2 at 80 s in the port and 100 s
      in the original). The battle is the same nevertheless: compared
      at the n-th round's end of each, the game's state and the
      computer's are the same but for what counts passes, in `m8_d5r`,
      `m8_d5r_nopics`, `m8_site` and `m8_wait` up to and past their
      first rounds with fights (rounds 3, 3, 3 and 4).
    - Where they part after that (`m8_wait` round 5, `m8_site` round 4):
      presumably the random numbers. The seed a fight's pictures set is
      the BIOS's count of ticks at that moment, which the port's clock
      does not hit to the tick, and the pictures' own calls of
      `rand_next` (`S2532:17D1`) are not made; `computer_queue` draws
      the kinds to build from them. Not followed to the first byte.
    - A script for such comparisons should have no taps (`m8_wait.keys`:
      `make_m8.py x45:400`); `m0_hq` and the scripts with cursor steps
      leave the original before their first fight on the own clock.
    Not reached on the own clock by any comparison: a site taken
    compared by state (`m0_hq` has one, but the port's play had left the
    original before; its length agrees), a site taken after a fight, the
    blasts' turns of a fight without pictures, a fight's pictures ended
    by a key, a wait ended by the button, a battle's end. A human
    second side with a key script for both sides followed (the bullet
    "A human second side" below).
    Not read: `S2AF3:000A`, `S2F39`'s routines called here, the stream of
    the site's picture, `S2532` beyond its frame loop's frame count and
    end, `inputs_release` with its second argument set. Not seen in a
    run: a site taken after a fight, the blasts' turns, a fight's
    pictures ended by a key.
  - Where the rounds part (2026-10-08, Mac; `port/src/play.c`'s
    `-seeds` and `-until`, `sh port/tests/play_test.sh TAG rounds` and
    `TAG seeds`; port/README.md has the results). Followed to the first
    byte in the map-8 runs; there are four causes and no fifth was
    found:
    1. The seed's tick. `m8_wait`: after round 3 (two fights with
       pictures) `rand_words` has 0008h, 08E2h in the original and
       0007h, 0774h in the port: the BIOS's count when the last fight's
       pictures began, 2274 against 1908 ticks. The port's round 3 ends
       at 90.05 s of the script and the original's at 110.03 s, because
       the computer's half before it is over sooner in the port (its
       passes draw nothing) and an earlier F1 of the script ends the
       round. Nothing reads the numbers until pass 3277 of the original
       (between rounds 4 and 5), where `computer_queue` draws three
       kinds to build; from there `side_points`, two units' records,
       `unit_marks` and `computer_steps` differ (ten bytes at round 5).
       Even where a round ends at the same second the tick is another:
       `m8_site`'s round 3 begins at 150.01 s and 150.04 s, the seeds
       are 0BBCh and 0BB8h. Why the port cannot find it: (a) when the
       round ends depends on how long the computer's passes last, which
       is the original's drawing; (b) the moment within a round's end is
       the runner's measured times (the `T_` numbers), right to 0.1 s
       where a tick is 0.055 s; (c) the original's count runs slow: it
       is counted by the game's own timer interrupt, which shares the
       counts out, and loses interrupts (some 7 ticks in 135 s in
       `m8_site` by the seeds' times; presumably while `pal_level`
       waits with interrupts off, not followed); (d) the port's count
       starts at the first pass with the setup's number, the original's
       setup was 0.55 s before its first pass. On a player's machine the
       seed is the time of day, so no number is the right one; only a
       comparison needs the original's.
    2. The pictures' calls of `rand_next`. `m8_d5r`, round 3: two calls
       after each of the two seeds (4.6 s and 4.9 s after the key's
       clearing);
       none in `m8_wait`'s 14 fights and `m8_site`'s 7. Read at
       `S2532:17D1` (called from the frame loop just before
       `S2532:0F8D` when a figure's script has 10h at this frame, side
       in `S3AEE:066C`): the
       losses still to show (`S3AEE:063B` less `0637` for the one side,
       `063D` less `0639` for the other) are shared out among the
       volleys left (`0628`, `0626`, set from `063D` and `063B` before
       the frames): with one volley left all at once, no
       call; with more losses than volleys `rand_next` modulo the
       losses; else one falls when `rand_next`'s low byte is above 80h;
       no call when nothing is left to show. `S2532:1970` then marks up
       to that many figures that stand (2 in byte 0 of the figure's
       22h-byte record, 8 in the side's seven bytes at `S3AEE:0688`), the
       nearest first, and returns how many. So the calls depend on the
       figures' scripts frame by frame (in the picture's data, not
       read) and stop when a key ends the pictures. The port can make
       them once the fight's frames are translated (`fight_frames` with
       the figures; part of the drawing, not begun here), not from the
       game's state alone; a key during the pictures stays a matter of
       time. Not checked: how often `S2532:17D1` is called in a fight
       (not logged).
    3. A tap's sampling (known, "The port's own clock" above): in
       `m8_site` the up of 182.00 s is read by two passes of the
       original (2787, 2788) and one of the port (3141), the cursor is a
       field off (the first view's word 0Ah, 2 and 1), and the unit
       moved in at 185 s takes another place of the site (`sites_400`
       record 0, bytes 1Dh, 1Eh and 24h, 25h). The port cannot find
       that: which sample has a tap of 0.08 s is the original's time.
    4. The script's keys meeting another state. `m8_wait_nopics` from
       round 6 on (no seeds there: no pictures): four bytes of the first
       view only (its field, word 0, 64h and 60h, and words 0Ch and 10h:
       the view stands one column of two fields further left in the
       port). The script ends the half every 10 s with the button and
       left, left let go 0.2 s after the button. In the original each of
       these between rounds 5 and 6 comes while the view does not read
       its record (the lines of `inputs.txt` have no test of bytes 5 and
       8 there); in the port the computer is done sooner, the view is
       back in its plain state, and the left without the button is read
       by `view_step` (`own_inputs.txt`, passes 4283 to 4285 and twice
       more) and moves the view. The same reason as 1 (a): how long the
       computer's passes last. Nothing of the battle differs by it in
       that run to round 8.
    Shown by taking the first two from the original's log on the port's
    own clock (`-seeds`: each fight's seed and its number of calls):
    then `rand_words` and everything else is the same in all six rounds
    of `m8_wait` and all four of `m8_d5r`, and in `m8_site` only the
    tap's five bytes remain to round 6. One byte differed once more
    (`m8_wait`, round 5, the first dump): bit 4 of `game_state`'s word 0,
    the first side's text still shown in the port; the timer of kind 1
    takes it off and timers count passes, so it is now counted with the
    passes'.
    Changed in the test: `rounds` plays to the log's last second, not
    its number of passes, and needs no `own` run before it. Not done:
    `fight_frames` translated; the tick's loss followed; a script whose
    rounds end at the same second in both (F1 only when both computers
    are done), which would take cause (a) away but not the others.
  - A human second side (2026-10-08, Mac; `port/tests/keys/make_two.py`,
    `m48_two.keys`, `m48_steps.keys`, `m48_move.keys`;
    `port/tests/play_dumps.py` and `play_test.sh` play the map a
    script's name begins with, `mNN_`; port/README.md has the counts).
    - How the original gets there: the maps 48 to 71 are for two
      players (byte 6 of the record in `CODES.DAT` is 2), and the menu's
      CODE row with such a map's five letters and START gives the battle
      with both sides human, no other choice needed. `play_dumps.py`
      reads the letters from the player's `CODES.DAT` when it runs; none
      is in the repository.
    - The second side's keys, from its table (`DATA:0B2E`, read in a
      dump): X left, V right, D or F up, C down, the left Alt or the
      left Ctrl the button. Its view is the right one.
    - What was seen on map 48 (screenshots and the log): the round's end
      comes with the first F1 of a pair once both sides have ended
      their halves with the button and left (24 rounds in the 400 s of
      `m48_two`, one every 15 s). The second side's first half is its attack half: the
      button and up on its unit gives "NO ASSAULT TARGETS!", and its
      button and left there shows "QUIT WITH JOYSTICK BUTTON" in every
      round of `m48_two` (what state that is was not read; the round
      ends nevertheless). In its second round the same keys move the
      unit: the button and up on unit 19h, a step right, the button
      twice, "UNIT HAS BEEN MOVED"; the order is on the second map from
      then and the unit on its new field after the round's end (map
      offsets 2B3h and 2B5h, passes 930 and 931 of `m48_move`).
    - The port needed no change for it: `battle.c` already makes both
      sides human for such a map, `input.c` fills both records, and
      `loop.c` runs `view_step` for either. On the original's clock all
      three scripts are the same at every pass compared, inputs
      included. On its own clock `m48_two` (no steps of the cursor) is
      the same in all 24 rounds; `m48_steps` and `m48_move` leave the
      original at the first key whose length is sampled (cause 3
      above): in `m48_move` the up of the move's gesture, let go 0.2 s
      after the button, is read alone by two passes of the original
      (672, 673) and four of the port (674 to 677), the step right by
      one and two; the port's aim is another field, its view stays in
      another state and no later round ends. The original's passes are
      longer there (it draws the marks of the move). There is no
      length of a key that is sampled the same whenever it falls.
    - Why `m48_move`'s round 2 never ends on the own clock (2026-10-08,
      Mac): not the port's doing, the script's. With the up read alone
      in four passes the aim goes three fields up (the first of the four
      passes does not test the record's byte 8) and with the step right
      onto a field where a unit of the side stands; the two presses of
      the button there give "NO TRANSPORTER" each instead of the move,
      and the view stays where it chooses the aim (state 0 with 16h in
      byte 18h, the fields it can reach still marked). There the
      script's button and left does not end the half, so the first
      side's view shows "WAITING FOR MOVEMENT-MODE" from then on and F1
      ends no round; every later gesture of the script meets the same
      state. Shown with the original: `m48_aim.keys` is `m48_move` with
      the up let go 0.3 s after the button and the step right held
      0.12 s (`make_two.py`'s new `/H`: `2m70/0.3 2R73/0.12`), which the
      original samples as the port's clock samples `m48_move` (up alone
      in the passes 672 to 675, tested in the last three; right in 713
      and 714). The original then stalls the same way (one round's end
      in 120 s, screenshots at 76 s to 99 s), the port on the original's
      clock is the same as it at every pass compared, and the port's own
      play of `m48_move` is the same as the original's of `m48_aim` at
      72, 76, 84.9, 99.9 and 119.9 s but for what counts passes
      (`play_test.sh m48_aim at m48_move T...`, new). Lengths between
      stall too (up let go 0.33 and 0.36 s after: five passes); with
      0.4 s (six passes) the rounds go on (four ends in 120 s; where the
      unit went was not looked at). Not looked at: which key leaves the
      choosing of an aim without a move (the script has none), and what
      "NO TRANSPORTER" is in the code (presumably `move_target`'s
      refusal of a field with a unit that carries nothing).
    - A fight, a site, a repair and a build with two human sides
      (2026-10-08, Mac; `m48_build.keys`, 285 s, seven rounds; the
      script's steps were found with probe runs of the original that
      printed both views every half second, not computed). What it
      does, as the original's memory at the rounds' ends shows it:
      round 1 the second side's unit 0Eh (kind 0Bh, on (9,10); fields
      as column and row, the map offset halved) attacks unit 0Dh on
      (14,8) from five fields away (its count 6 to 5, the attacker's
      byte 1 to 1) and the first side moves its unit 15h from (15,13)
      to (13,12); round 3 that unit goes on to (11,12); round 4 it
      attacks unit 14h on (10,13) beside it while the second side
      orders the same unit to (10,12): the fight comes first (both 6 to
      5, both byte 1 to 1), then the move; round 6 unit 14h moves onto
      the field (9,11) of `sites_400` record 0 and is in both rows'
      first place after the round's end (4000h in `unit_flags`); round
      7 the second side enters the site, repairs the unit (count 5 to
      6, `side_points` 36h to 27h) and builds on the third place
      (`unit_add` once: unit 2Dh of kind 12h, the list's first, price
      23h; points 27h to 4; the side's count 20 to 21). Each fight is
      shown as pictures.
    - Learned on the way. Where a unit can go is read best from
      `field_bits` in a dump (1 and 2 by the side), not from the
      screen: a press on a field without the mark is refused without a
      text (the view waits in step 17h for four passes and is back in
      16h), which looks like a lost key. The kind 15h (the screen says
      cavalry) reached two fields here, the kind 12h one or two by the
      ground; the kind 0Dh (a bunker) has no move ("NO ASSAULT
      TARGETS!" and no marks; the kind 0Ch's marks on the screen were
      few, not read from a dump). A unit beside
      one of the other side can still move away. The button and left
      on a site's field goes into the site in the move half too (state
      2, step 0Ah); the button and right inside leaves it (step 0Bh
      with 2 in byte 19h, then state 0 on the field beside), which a
      script needs before it can end that half on a plain field. With
      less than the cheapest kind's price the button and left on a free
      place opens no list (`build_count` 0: 18 points against 35), so a
      script has to wait for the rounds that bring them: the second
      side got 18 after each even round here, the first 10 after each
      odd one (not looked at what gives them). The cursor's steps are
      one field while the view stands and two when it scrolls.
    - The port needed no change: on the original's clock `m48_build`
      is the same at every pass compared (port/README.md). On its own
      clock it leaves the original before the first round's end (the
      cursor's steps are sampled otherwise, cause 3 above) and no round
      ends.
    - A battle's end with two human sides (2026-10-08, Mac;
      `m54_end.keys`, map 54, 14 rounds, 320 s). Chosen because a unit
      of the second side stands five fields from the first side's
      headquarters there (`sites_40` record 0 on (14,16)); on no map
      for two players is one nearer. The headquarters' field can be
      entered from three fields only, (14,15), (13,15) and (15,15):
      the other three around it and (14,18) are grounds of a class the
      kind 12h cannot enter (bit 80h of the ground's byte 2, its cost
      255), and on each of the three a unit of the first side stands at
      the start. So the script walks the second side's unit 4Ch round
      the building in its move halves, (14,21) to (14,20), (14,19),
      (15,18), (16,18), (16,16), (15,15), a field or two a round (12 in
      its byte 0 against costs of 5 to 9), while the first side moves
      its unit 36h from (15,15) to (15,14) in round 9, and in round 14
      onto (14,16). The kind 12h may enter the other side's grounds
      because it has 1 in word 6 (`move_range`'s mask FFFFh).
    - What the end does, in the original: `attacks_run` of pass 3853
      (281.39 s) carries order 4 out (`event_picture`, the grey screen,
      the site's picture), the unit is in the first place of both rows
      of the record and has 4000h, the record's word 19h 4 to 5;
      both views go to state 7 with 10h and 0Fh in word 22h, the texts
      are "YOUR HQ HAS BEEN CAPTURED !!" and "VICTORY! HQ OCCUPIED",
      4000h and 10h are set in `game_state`'s word 0 and 2 cleared.
      The first side's part of that pass then waits for a press
      (`S0CB6:0DA9`): nothing more ran for 19 s until the script's
      button at 300 s, then the second side's part of the same pass
      began and waited the same way for the next press (305 s), and the
      loop was left at `S066A:45E8` (305.07 s). So each side's part
      wants its own press at a battle's end.
    - The port needed no change for the game's state: on the original's
      clock the same at every pass compared, the last pass and the
      memory at the loop's exit included (`play_dumps.py` now logs
      `S066A:45E8` and dumps there as the pass after the last, which
      the port's `-dump` writes after its last pass). On its own clock
      `m54_end` is the same to round 3 and then parts (a step's
      sampling).
    - The end's texts and its wait (2026-10-08, Mac, later; state 7
      read again, `S066A:4198` to `4333`, and `S0CB6:0DA9`, now
      `wait_press`). The numbers of the note on state 7 above are right:
      the side whose view has 0Fh gets text 3Ch and the other 3Dh, 11h
      gives 41h and the other 42h, anything else 42h for both; the
      first view is tested first. Both texts are shown by every part
      that finds its view in state 7 (so twice, the views being both in
      it), then `page_flip`, `page_copy` and `wait_press`: while the
      first side's button is held or the key's word is 1 both are
      cleared and three frames waited, then both cleared again and
      again every three frames until one comes. Only the first side's
      button and the keyboard end it, not the second side's button as
      such (on the keyboard that is a key; with a joystick it would not
      be one; not tried). The port's `state_end` does the same now
      (`loop_end_texts`, `LOOP_END`; `play.c` waits on its own clock
      and takes the clearings from the original's log as `E` lines,
      `S0CB6:0DE4`, `0E30`, `0E51`). `play_dumps.py` logs the texts
      of each state 7 (`S066A:42FC`, then `show_message`'s number at
      `S10AF:009F`) into `ends.txt`, the port's `-ends` writes the
      same, `play_cmp.py` compares: in `m54_end` 3Dh for the first
      side and 3Ch for the second, in both parts of pass 3853, the same
      in the port; 214 clearings in the two waits. Not verified: the
      wait on the port's own clock (no script reaches a battle's end
      there), the texts 41h and 42h in a run, the texts as pixels.
    - What follows the loop (2026-10-08, Mac; `S066A:45E8` to `4728`
      and `S063F:0009`, now `battle_score`, with `S167A:3610`,
      `scores_load`, read; `m54_end` run on to 350 s). All of the
      program is `game_loop`: the menu is its call of `S167A:0005` at
      `S066A:0349`, and behind the loop it goes back there unless the
      last map was won. In order: with 40h in `game_state`
      `S2F39:1058(4)` (presumably the music's end; not read),
      `fade_out`, `S2AF3:000A`; with 10h (a human has won)
      `event_picture` with D0h, or CDh when 2000h is set (a side left
      without units), and the side whose view has 0Fh (the first, else
      the second); `battle_score`; with one side the computer's and a
      map below 30h `event_picture` with the map's number as its kind
      (the number before `battle_score` moved it on when 6000h is set;
      presumably the newspaper; not seen here); with 20h (the last map
      won) the result 7Bh, 100h set and 1 and 2 cleared; 6030h
      cleared; then with 1 still set back to `S066A:0349` (1 set
      again, 10h cleared, `inputs_release`, the menu), else the
      program's end with the result. `battle_score` does nothing in
      round 0; it loads `CODES.DAT` again, keeps the map's number in
      word 0Fh of `game_state` (the pass's least length, no longer
      needed), adds 1F4h to `score` (`S331F:4D2F`, `defence_sum`'s
      result of the last round's end) with 2000h, and with 10h moves
      `map_number` on by byte 7 of the map's record and sets 20h when
      the record's byte 8 has 1; without 10h the score becomes 0. A
      score above 0 is compared with the lowest of the map's four best
      (`scores_load`: the file of `file_name(2, 1, two digits, 0Bh)`,
      zeros when it is not there, never above 7EF4h): when higher,
      `rand_next`'s low three bits below 3 choose one of three
      pictures of kind CEh (the first view has 0Fh or 11h) or CFh.
      That the menu then asks for the name is the menu's doing (its
      screen "PLEASE ENTER YOUR NAME !" at 350 s), not read.
    - Seen in `m54_end`: the loop left at 305.07 s, picture D0h for
      side 1 from 306.09 s to 321.63 s (men with raised arms before a
      building), `battle_score` with the score 0AE3h, one `rand_next`,
      picture CFh with 2 to 345.32 s, `S066A:4728`, `0349`, the menu
      at 345.49 s. `map_number` was 36h and is 37h there (byte 7 of
      the record is 1).
    - The port: `battle_after` and `battle_score` (`port/src/battle.c`)
      do this but for the pictures, the sound and the colours; each
      picture is told to `battle_hook`. `play.c` calls it once the
      battle is over (`play_after`; on the original's clock the log's
      lines `B`, `battle_score`'s call, and `M`, `S066A:4728`, bound
      it), `-ends` gets "after KIND ARGUMENT" a picture, and
      `play_dumps.py` dumps the original's memory at `S066A:4728` as
      the pass after the one after the last. `m54_end`: the pictures
      D0h with 1 and CFh with 2 in both, the game's state and the
      computer's (with `rand_words`) the same there. The files lie in
      the sides' work buffers (`CODES.DAT` in the second side's, the
      scores behind it, the first side's as `file_load`'s work), which
      are compared. Not translated: the menu, so the port's play ends
      here; the pictures and their time. Not seen in a run: CDh, CEh,
      a map for one player's picture, the last map (7Bh), a score not
      above the lowest, a file of scores that is there.
    - The other ends, on map 57 (2026-10-08, Mac; `m57_site.keys`,
      `m57_last.keys` with `m57_last.pokes`; found with probe runs of
      the original as before). Map 57 is the smallest for two players:
      16 by 16, four units a side (a kind 0Fh, a cavalry 15h, a bunker
      0Dh of three, and a kind 00h or 2Eh), a headquarters and a
      factory each (`sites_400` records 0 and 1). The second side's
      cavalry (unit 6) walks north in its move halves, (6,11) to (5,7),
      (5,5), then:
      - `m57_site` (215 s, ten rounds' ends): on to (3,4) and onto the
        first side's factory (2,3). `attacks_run` of pass 2102
        (167.39 s) takes it: the unit in the first place of both rows
        of record 0 with 4000h, the record's byte 18h 0 to 5 and 19h 8
        to 9, no end of the battle, two more rounds played. The
        script steps the second side's cursor off the factory before it
        ends its next half (the button and left on the site would go
        in).
      - `m57_last` (270 s): the pokes leave the second side with that
        cavalry alone and one figure of it (the counts of its units 4,
        5 and 7 set to 0 and of 6 to 1 when the first pass begins;
        `units_drop_empty` takes the three off at the first round's
        end; `unit_total` and the side's count stay 8 and 4, since
        nothing counted them down: the poke's doing). The cavalry goes
        to (5,3) and attacks the first side's bunker on (5,2) in round
        4: shown as pictures, the bunker loses nothing, the cavalry its
        one figure (`unit_destroy` for the attacker, the bunker's byte 1
        0 to 2, `unit_total` 8 to 7). `attacks_run` then finds the
        second side without a unit: both views in state 7, word 22h 11h
        for the first and 12h for the second, texts 41h and 42h shown
        in both parts of pass 1935, 2000h and 10h in `game_state`. One
        press of 0.15 s (195.00 s) ended both waits here, the loop was
        left at 195.10 s: the button is still held when the second
        `wait_press` has cleared it, and the timer's routine sets it
        again (in `m54_end` the second wait took the second press; not
        looked at what differed). Behind the loop picture CDh with
        side 1 (the test there is for 0Fh in the first view, which has
        11h, so the winner's side is not the one passed; the original's
        doing), `battle_score` at 213.88 s: the score A0h and 1F4h,
        294h, `map_number` 39h to 3Ah, one `rand_next`, picture CEh
        with 1, `S066A:4728` at 237.53 s.
      - The port on the original's clock: both the same at every pass
        compared, the inputs included, the end's texts, the pictures
        behind the loop and the memory at `S066A:4728` (port/README.md).
        It needed one thing more of the clock: `inputs_release`, with
        which `event_picture` begins, clears the key and both buttons
        (the second F1 of the script's pair was still kept when the
        factory's picture began; without the clearing the port's next
        part read it). `play_dumps.py` logs that as `Y` lines
        (`S167A:2FEF`, `30B2`). `inputs_release` is entered with its
        first argument as the flag "one was set", so with 1 it clears
        at least twice; this corrects the hints' comment only in how it
        comes about.
      - On the port's own clock both are the same to round 3 and then
        part (the steps' sampling, as in the other scripts with cursor
        steps); the battle's end is not reached there.
    - The pictures behind the loop and the end in the window
      (2026-10-08, branch `debian/window5`; `event_picture`'s kinds CDh
      to D0h read whole, hints `res_load`, `first_frame`, `pic_won`,
      `pic_medal`, `pic_wiped`, `vdif_frame`; translated in
      `port/src/events.c`). What was learned:
      - game_loop gives the pictures behind the loop (and
        `battle_score`) its `[BP-10h]` as the work buffer, which is
        where the map's pictures were loaded (`game_mem.gfx`), 1E4h
        before the end of the sides' lists the port had used; CODES.DAT
        and the scores went there in the original too. `battle_score`
        has moved its own on by 2710h, 3E8h and 32h (2B2Ah) when it
        calls CEh or CFh. Found by the pictures' files lying 1E4h and
        2B2Ah apart in the two memories; the screens were the same.
      - The pictures are streams of "VDIF" frames drawn straight into
        the VGA's planes, timed by word 22Eh of the screen's record
        (the timer's routine counts it; set to 0 at each frame's
        beginning): D0h 46h frames of 0Ch counts, CDh 82h of 6 (9 from
        frame 30h on), CEh/CFh 4Bh of 6 and a second stream to its end,
        then a still picture held 12Ch retraces. The last 20 frames of
        D0h and of CEh/CFh's first part blend the colours to black
        (`pal_blend` towards the stream's palette, which was cleared
        when it became the game's). CDh plays its sounds at frames of
        a table (`sfx_play`, `snd_pass`; S3303:0022 chooses the table
        and the channels, not looked into what it is).
      - The runner's times (from inputs_release's end to the first
        frame): D0h 1.24 s, CDh 1.295 s, CEh 0.88 s, its second stream
        2.44 s after the first's last frame, its still picture 9.96 s
        from the last frame to `S066A:4728`; the loop's end to
        inputs_release 1.125 s, of which the port counts 0.70 s
        (`fade_out`, inputs_release; the 0.42 s more lie in music_fade(4)
        and unders_restore, not checked which).
      - The window reaches a battle's end only with a script that
        survives the port's own clock: `m54_end` and `m57_last` part
        after round 3 there (`m57_last` with its 0.08 s taps made
        0.15 s keeps all seven rounds' ends, but the cavalry stands
        elsewhere then, and its fight's outcome follows the clock when
        its pictures begin; `m54_end`'s passes run shorter in the port
        after the second side's move order in round 4, so an F1 is
        read in the other side's part; why, below).
      - Why `m54_end` parts on the port's own clock (2026-10-09,
        `debian/window11`): at 114.0 s, the F1 that ends round 4 in the
        original's pass 1350, the port is in pass 1360 and its state
        already differs (`play_test.sh m54_end at m54_end 114.0`:
        `unit_marks`, `field_bits`, `sides`, `attack_pick`, the
        computer's). The second side's held cursor keys at 99 s are read
        in another number of parts (the record's word 10h in 6 parts and
        1 in 2 in the original, 5 and 4 in the port), because the port's
        passes lie at another phase to the keys. The lead of 10 passes
        by 114 s, measured with `-until` against the clock's file:
        - the round's ends without fights: 2.81 to 2.84 s in the
          original from the pass to the next part, 2.73 s in the port
          (the `T_` numbers come from map 8; `attacks_run` to the next
          part 1.44 to 1.46 s against 1.37): about 5 passes;
        - passes whose work overran four ticks in the original (from 40
          to 114 s: 1,183 passes of 4 ticks, one of 5, four of 6, among
          them 630 to 633): 9 ticks, about 2 passes;
        - ticks lost by the timer: from 31.34 s on the PIT runs with
          reload 1549 (770.29 a second, the period the sound's
          `pit_add` asks for; `pit_irq` takes 060Dh off every client's
          count, `S27FE:06EB` logged), input_tick's 3FFFh then come
          10 or 11 interrupts apart (0.0130 and 0.0143 s, 57.5 % 11, as
          16383/1549 says), but from 40 to 110 s there are 5,090 calls
          of input_tick (72.71 a second) against 5,098 of the PIT's
          count: about 85 interrupts lost, presumably while one was
          still pending (not checked where; it follows the runner's
          speed, as a real machine's would follow its own): about 2
          passes.
        The PIT is reprogrammed only four times in the run (0.16, 0.51,
        18.57, 31.34 s, `S27FE:075A`), so the reprogramming is not a
        cause. Not fixed: the port does not draw and does not run the
        sound's timer, and any phase shift between the passes and the
        script's held keys changes their count, so no constant makes
        it the same; a script for the own clock needs keys held for
        whole passes away from round ends (or `-keylog` from the
        window). D0h, CEh and CFh on the own clock stay unreached for
        that reason, not because of the pictures.
        `m57_win` (a poke leaves the second side without units, so the
        first round's end ends the battle) is the same on both clocks,
        in the window and behind the loop (port/README.md).
    Not reached with two human sides: a carrier, the keys F2 to F9, a
    unit destroyed by play alone (the one destroyed had its count poked
    to 1) or as a fight's target, a battle ended by fights alone, a
    depot (`sites_100`) taken, a taken site with units inside, the
    menu behind the loop, moving out of a site, a repair or a
    build refused, a map from
    24 to 47 (where `rec_of` exchanges the two records; those maps
    have a computer's side), the joysticks and the mouse, and what
    `m48_steps` did beyond moving the cursor and opening what its
    button opens (its units' records did not change; not looked at
    which states its views went through).
  - A battle ended by fights alone (2026-10-09, Windows; `m60_fights`,
    port/README.md). Map 60 (64 by 32) is the only map with one unit a
    side: a cavalry (kind 15h) each, at (5,2) and (59,26); its other 73
    units have 4006h in `unit_flags` (in sites, nobody's) and are
    counted for neither side. The two cavalries were walked together
    over 78 halves (side 1 alone for the last ones) and fought in
    the last five rounds: five fights, each costing both one figure but the
    last (the first side 1 to 0, the second 2 to 1), and the first
    side is the one left without units: both views in state 7 with
    word 22h 12h for the first and 11h for the second, texts 42h and
    41h in both parts of pass 22903, pictures CDh with 1 and CFh with
    2 behind the loop. No poke. So the count at `S02E6:1EFC` (hints
    comment) is reached by play, and the side that loses can be the
    first; 15h (both without units) was not reached.
    Learned on the way:
    - The keys' timing. `side_input` steps the cursor only on a pass
      where the directions changed or the view's repeat counter (byte
      2Ch) ran out, and the counter is set from the bytes at 28h, which
      are 1, 2, 1 here: a direction held over more than one pass
      (about 55 ms on the original) can step twice, and how often
      depends on the clock. The gesture of make_two.py let the direction
      go 0.2 s after the button, which stepped the cursor once or twice
      more after the menu. With taps of 0.06 s and the direction let go
      0.03 s after the button (`SxT/H`) a script played the same on the
      port's own clock and on the original's (checked on the runner to
      half 7 before the whole run), so the script was found on the
      port's own clock, about a second a try, and run on the original
      once. While the button is held every pass counts as a change
      (word 2Eh keeps 10h with the directions, the test compares
      without it), so the menu's pointer follows the direction steadily.
      A tap of 0.05 s was lost once on the port's own clock (taps fall
      between its calls of the timer's routine there).
    - A cavalry (classes 32h in byte 5 of its kind) cannot enter a
      ground whose byte 2 in `ww1gnds_dat` has none of those bits
      (25h, 2Bh and 2Ch on map 60, a band from (39,18) east); the way
      round was found with that rule and the ground's byte 3 as cost
      (`move_costs`), not by `move_range` itself.
    - A press of the button on a field that is not marked does not take
      back a move aim (the unit keeps 100h); the gesture was only sent
      when a field nearer was marked.
    - The end of a half (button and left) on a site's field goes into
      the site (state 2); button and right leaves it.
    - `play_dumps.py` listed only dumps with four digits in
      `passes.txt`, so in a run of more than 9,999 passes the later
      ones (the loop's exit among them) were dumped but not compared;
      fixed with this change.
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
  - `order_undo` reached, order 1 unreachable (2026-10-08, Windows;
    `port/tests/keys/m48_undo.keys` and `.pokes`, port/README.md).
    - A unit is freed with `order_undo` when the other side's fight
      destroys it in the half it was ordered to move: the walk ends
      within the half (`move_arrive` sets 1 in `unit_marks`), the fight
      at the round's end hits it on its old field on the attacker's map
      (`unit_destroy` sets 4), and the freeing loop sees both. With two
      human sides that is scriptable: `m48_build`'s round 4 already has
      the first side attacking unit 14h while the second orders it to
      move; a poke of its `unit_count` to 1 makes the fight destroy it.
      Seen in the original with a breakpoint at `S10E1:1514` (one hit,
      196.7 s, AX the unit); the port is the same at every pass compared.
    - A breakpoint log stopped at 190 s missed the call although
      `attacks_run` began at 181.6 s: the fight's pictures take seconds
      before the freeing loop runs. Let such a run go on well past the
      round's end.
    - Order 1: nothing can write it. `move_rec`'s address (180Fh) is
      taken only in `move_target` and `move_arrive`, and `move_target`
      stores only 3, 2, 4 and 5 in its byte 0Ch; no instruction names
      its bytes directly. Byte 13h of a unit is otherwise written from a
      variable only by `S0AE0` (values 9, 0Bh, 0Dh, 11h, of its own) and
      a saved game's load, and the map's units are made by `unit_add`.
      Not followed: the word writes at offset 12h of records (ten sites,
      presumably other records; the ones looked at are views and the
      like) and block copies into `S331F`. So `order_capture` and
      `move_arrive`'s kind 1 stay translated from reading only.
    - Not reached: `order_undo` for kinds 2, 4, 5 (`m48_build` round 6
      gives unit 14h kind 5, a fight on it there would reach it) and for
      a freed carrier's units.
    - On Windows `play_test.sh` cannot run `sh build.sh` (no `cc`): the
      port was built with `build.bat` and the script run with that line
      and its `cd` taken out (a copy outside the repository).
  - A depot taken (2026-10-08, Windows; port/README.md, `m0_s4`).
    - Where one can be taken: `sites_100` records read from the first
      pass of all 24 maps for two players (a dump each by a scratch
      script, menu as `play_dumps.py` walks it). Every depot owned by
      a side is closed in by its owner: on map 48 (7,8), the second
      side's, lies in a pocket west of a river whose only open side is
      the second side's ground and units (the first side's cavalry 0Dh
      walked (14,8), (11,7), (10,7), (12,9) in probe runs and could go
      no nearer); on map 54 (21,17), the first side's, has its three
      open neighbours held by units 3Eh, 42h, 43h; map 56's (29,1) is
      reached only through the second side's row of units. Only the
      main field of a depot (the one whose ground has 100h) can be
      entered; the other three are closed to the kinds tried. A
      field_bits byte is at row * 40h + column, not by the map's width.
    - So the taking was watched on map 0 with the existing `m0_s4`
      script in the play harness: both depots there are nobody's (19h
      12h); the computer takes record 1 (pass 1787), the first side
      record 0 (pass 2875); the port is the same at every pass
      compared. Order 4 was translated before (`order_take`); what it
      calls besides was read now: `S131D:1426` is `view_cover` (draws
      only), `S2F39:1058` `music_fade` (the sound driver; with music on
      it also makes attacks_run start the music again at its end).
      Both are left out of the port, being drawing and sound.
    - Not reached: a depot of the other side taken (the count byte 4
      of the loser's `sides` record going down, `!(19h & 2)`), a depot
      taken with units inside. A poke of a depot's owner would need its
      ground and the sides' counts too (not tried).
    - Not resolved: one input part of `m0_s4` (pass 1059, byte 8 of the
      first side's record after Space was let go inside the part
      before) differs although the state stays the same; port/README.md.
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
- The menu (2026-10-08, Mac; `S167A:0005`, now `menu_run`, read whole but
  for its drawing routines; the names and what each does are in
  src/HL.hints under "The menu"). Learned:
  - It is driven by `MNUDATA.DAT`: 10h texts of 20h bytes, items of D8h
    bytes (rectangle, texts of 14h bytes, kind in byte 72h, number of
    choices 73h, the choice 74h, what it does or where it goes 76h) and
    pages of 34h bytes (places, the items' numbers from 2Ch, their count
    at 32h). Page 1 is the main menu (START, CODE, GAME SETUP, CONTROLS,
    MISC), page 0 the exit's; Esc or the mouse's second button changes
    between page 1 and 0 from anywhere. Keys alone walk it: a side's up
    and down, its button (space for the first side), Esc; the code and a
    winner's name are typed with the BIOS's keys (`key_remove`).
  - A pass of its loop lasts five calls of the timer's routine (0.0687 s
    in the runs) when nothing is drawn.
  - The first press of the button after the code was typed does nothing:
    the button's key was let go while the keys were the BIOS's, so the
    program's table still has it as held until it is pressed and let go
    once more (seen: START took the second press; the port does the
    same).
  - What it leaves for the battle is listed in the hints (`menu_run`):
    which sides are the computer's and word 2 of `game_state` from items
    7 and 8, the level from item 0Ah, 8 in word 2 from item 9, the
    palette's number from item 6, `no_fight_pics` from item 0Bh,
    `pics_winter` from the map's record, the highest of the map's best
    scores at `S331F:4D2B`, the mouse's side from item 0Eh. This
    replaces the guess "8 from another item of the menu" above.
  - The mouse's routines (`mouse_open` to `mouse_show`, `S2BCD` to
    `S2BFD`) keep the record at `DATA:0414`; with a mouse the menu puts
    the pointer on the chosen item itself and takes the item under the
    pointer as chosen in every pass without input.
  - The port: `port/src/menu.c` (the loads, the loop, the code, the
    country, the scores, a winner's name, a saved game's number, what it
    leaves), `port/src/mouse.c` (the mouse's record over a driver that
    only the program moves), the BIOS's keys and `key_install`/
    `key_remove` in `input.c`, and the question whether to leave a
    battle in `loop.c` (Esc, then the key of entry 18h of the table of
    keys: 2 cleared and 1 set in `game_state`, no score). The port's
    program begins at the menu when it gets a key script and no `-map`,
    and goes round as `game_loop` does: menu, battle, what follows it,
    menu.
  - Compared (`python3 port/tests/menu_dumps.py`, `sh
    port/tests/menu_test.sh`; port/README.md): five walks by keys, each
    with what came in the same in both and no byte different at the
    menu's return in the game's state, the menu's segment, the mouse's
    record and 40000h bytes of game_loop's block: the code of map 57;
    START on map 0 with either country (maps 0 and 24); all pages with
    every row of GAME SETUP and two of CONTROLS chosen and the best
    scores shown; and menu, battle, Esc and the confirming key, menu,
    START again (the second return compared; the passes' count of the
    battle between is 130 in the original and 190 in the port, which
    draws less).
  - The drawing (2026-10-08, later the same day): `pic_part`
    (`S2A55:0006`), `item_line`, `page_items`, `item_mark`, `page_title`,
    `score_line`, the texts and the mouse's pointer (`pointer_draw`,
    `pointer_erase`, on the page shown) are read, named and translated.
    All of the menu's pictures are of the 8-bit kind (`U`), the flags
    packed (`I`). The menu draws with the screen's cut on (word 0Eh of
    the screen's record, the whole screen as rectangle); the port's
    `pic_draw` has only the way without the cut, so `menu.c` draws a
    picture that lies inside the screen with the word off and stops at
    one that does not (none met). Video memory compared at 13 passes of
    the five walks (the main menu, GAME SETUP after its rows were
    chosen, the exit's page, CONTROLS, MISC, the main menu with a typed
    code, the menu after a battle): no byte different in the 200 lines of
    either page, the pointer included. Behind the pages 810 bytes differ
    (from `A7E8:0004` on: what the original's start before the menu
    left, which the port leaves out).
  - Not reached or not done: no sound or music; the screens that are
    not passes of the menu's loop were not compared as video memory (the
    country's with the flags, the best scores, the code while it is
    typed); a winner's name and the file of scores written (`file_save`;
    translated, run in no walk: no script on the port's own clock wins
    a battle yet), a saved game's number (translated, not run then; since
    run and loading translated, see "Saving and loading" below), EXIT TO DOS
    (translated, not run), the program's end behind it (`S066A:4739`; both run and compared since,
    see "The program's end and the briefing" below),
    the menu without a mouse (not translated), a click or a moved mouse
    (the port has no mouse of the player's yet), the country left by
    Esc, a code that is not one, the backspace, a picture cut by the
    screen's edge (`S2AAF:0107`), the order of equal scores in
    `scores_show` where the original reads bytes of its stack not yet
    set, the briefing before a battle on a map for one player
    (`S274D:0003`, left out then; translated since, see "The new units'
    screens" below),
    and the times: the port's menu takes the same passes but no time for
    drawing or loading, so a key comes up to 0.54 s apart in the two.
- The program in the window (2026-10-08, branch `debian/window2`;
  port/README.md "the program in the window"). Learned: after the menu
  the original's first `input_mouse` (`S27FE:03D4`, the mouse given to a
  side's record) sees the pointer the driver's reset put at 319 and
  gives "right" with `mouse_center` (`S27FE:04D0`, at 20.19 s in the
  walk `back`); the setup that follows takes 2.75 s and clears the
  sides' records, so the battle never sees it. The mouse's speeds in a
  battle are `mouse_speed(0Ah, 0Ah, 0Ah, 0Ah, FA0h, 28h, 28h)` at
  `S066A:03B1` (and by the options' item `S37FF:0113`, 32h, 0Ah or 3,
  at `S066A:197A`); the menu's are 10h/20h with steps of 1F4h
  (`S167A:067E`). The small map is drawn by `S131D:007D` and
  `S0E56:09AD` with `S151F:000D` (translated, port/README.md "the small
  map"); UMAP's pictures are 8-bit ones drawn with the screen's cut
  (`S2AAF:0107`).
- What a player meets in the window (2026-10-08, `debian/window2`,
  headless build scripted by `DK_KEYS`/`DK_MOUSEMOVE`, map 8 and from
  the menu): works: the menu by keys and mouse, START, the battle,
  the cursor by keys and by the mouse (a motion of 60 counts up moved
  it a field), the field menu's move (range marked, way kept), the
  small map (right; drawn, frame moved without copies, the view on
  leaving), the end of a half (left: "F1: MODE CHANGE" as in the
  original's screen). Drew nothing then (translated since, see "The
  field menu's information" below): the field menu's information
  (down): on a unit `S131D:0483` (389 lines; `S290A:0006`,
  `S131D:0148`, `big_unit`), on a field without one the statistics
  `S131D:0EE5` (488 lines; `S1571:0134`, `game_text_at`, `view_dim`,
  `S2B45:0006`), seen in the runner's screen of map 8. Not tried: an
  attack by the player, a site's menus (building, repairing, moving out)
  in the window, the second side's keys, holding a key, the battle's
  end in the window (the player's attack, the second side's keys and
  held keys since: "Both sides, held keys and the player's attack in
  the window" below). port/tests' `orders_test`, `move_test`,
  `side_test`, `view_test`, `computer_test`, `load_test` and
  `income_test` do not link since the merge of save.c (save.c needs
  loop.c, which needs more); not fixed here.
- Both sides, held keys and the player's attack in the window
  (2026-10-08, `debian/window3`, Linux, headless window; port/README.md
  has the counts). Learned:
  - The window needed nothing for the second side: its keys (the left
    Alt, X, V, D, C) come through the same keyboard table, `input.c`
    fills its record, and its view and map are the ones `-keys` plays
    have used since "A human second side". A held key needs nothing of
    the window either: the platform passes no auto-repeat (`plat_sdl.c`
    drops it), and the game repeats by itself from the record's counts
    (held 1.0 s: eight steps of the first side's cursor, the first at
    once and then about every 0.11 to 0.17 s; the second side's held C
    1.0 s four, D 0.4 s three).
  - What was wrong: the window showed its pictures ahead of the game's
    clock where a routine waits for retraces that the clock counts only
    after it (`fade_grey` at a round's end: 60h retraces, 1.37 s, counted
    by `LOOP_ROUND`'s `own_retraces(0x60)`), and a key went to the
    keyboard's interrupt at once. All keys of that time reached the
    same call of the timer's routine, so a tap was lost (press and
    release at one call). Now the window's keys are kept with their
    picture's time and handed over by the clock (`live_key`), as a key
    script's are; a play in the window is a key script (`-keylog`), and
    the original can be run with it. Presumably the same holds for the
    mouse, which still goes to the driver at once (not changed, not
    tried in a fade).
  - Map 8's first side could not attack in the half tried (units 28,
    30, 32, 33 beside the computer's 27 and 29 each gave "NO ASSAULT
    TARGETS!"; their kinds were not looked at), so the attack was made
    on map 48 as in `m48_build`: the cavalry (unit 15h) moved twice to
    the field beside unit 14h and attacked in its next attack half.
    The first side's first half on map 48 is its attack half too (the
    button and up on the cavalry then gives no targets).
  - `menu_attack` draws the marks (`marks_draw` at `S0CB6:1629` and the
    two places alike before it); with them the view goes to its darker
    colours (+90h) while the target is chosen. The port had left this
    drawing out, which only the video memory showed.
  - For the comparisons: with a German locale awk compares the clock's
    times as text (`play_test.sh` sets `LC_ALL=C` now); the video memory
    behind the two pages differs from the first pass on by 1,442 bytes
    on map 48 (1,914 after the fight; not looked into).
  Not done: the SDL window by hand; an attack by the second side, a
  site's menus and the battle's end in the window; replaying a
  `-keylog` that begins at the menu.
- A site's screens and the mouse in the window (2026-10-08,
  `debian/window4`, Linux, headless window; port/README.md has the
  counts). Learned:
  - State 2 draws through `site_screen` (`S131D:0A1C`), `site_units`
    (`S131D:087A`) and `build_rows` (`S131D:0B07`), named in the hints,
    and the blocks `S066A:296E` to `2DB8` (a place shown) and `3677` to
    `382F` (a kind shown); the port had their state and none of their
    drawing, which only the video memory showed (47,864 bytes).
  - A `-map` battle of the port had no mouse: only the menu's return
    opened it. The original's `S37FF:0114` chooses the record the mouse
    plays (the first side's by default; F7 twice reopens it for that
    byte, a way to give it the second side, not tried).
  - The mouse gives a direction when the pointer is the step (28h) away
    from the middle; with the button held `mouse_center` does not put it
    back, so the direction lasts until the button is let go, and none
    is held after it, unlike the keys' gestures (the left still held
    0.2 s after the button steps a site's cursor back: why `m8_site`
    needs its R steps and the mouse not).
  - The window's motions go to the driver by the clock and `-micelog`
    writes positions the runner's `-mice` replays: the original played
    the window's mouse the same in every part.
  Not done: the SDL window by hand, a damaged unit repaired and a
  carrier's screen by the mouse, a scrolled build list.
- A carrier's screen by the mouse, a repair and a scrolled build list in
  the window (2026-10-08, `debian/window7`, Linux, headless window;
  port/README.md has the scripts and counts). Learned:
  - The port needed no change: `m0_cmouse`, `m8_rmouse_nopics` (text
    51h, the builder repaired in the depot) and `m8_scroll` are the same
    as the original at every pass compared, both video pages included,
    and the window the same as `-keys`.
  - A scrolled list needs more than seven kinds: map 8's factory with 48
    points has three, so `m8_scroll.pokes` gives the first side 2000
    points (a poke, not play). The list scrolls when the cursor is on
    row 6 and steps down (word 20h of the view up by one).
  - The window's play of `m8_d5r`'s times with fight pictures did not
    reach the repair; without pictures (`-nopics`, `NOPICS=1`) it did.
    Why was not looked into. `DK_SHOTS` frame numbers computed from the
    time are not the clock's time once a round's end has passed.
  Not done: the SDL window by hand, `rounds` for the three scripts.
- The second side's attack in the window (2026-10-08, `debian/window6`,
  Linux, headless window; port/README.md has the counts). Learned:
  - The port needed no change: a human second side attacks through the
    same routines as the first; `m48_sattack` (its medium artillery
    against the first side's cavalry two fields up, fought with
    pictures at round 1's end) is the same as the original on its
    clock, on the port's own (`rounds`) and between window and `-keys`.
  - In the attack's choosing the cursor is the fist; the target is the
    lit field. A gesture whose direction is held after the button
    (`make_two.py`'s default 0.2 s) steps the aim once more in the
    window but not in the original (sampled by other passes); let go
    with the button (`/0`) both read it alike. Shots of a window play:
    `DK_SHOTS` at picture 38 + (t - start) / 0.014268, which runs ahead
    of the clock after a round's end.
  Not done: the fight's pictures frame by frame against the original,
  the computer as a second side attacking in the window, the SDL window
  by hand.
- Menu, battle, menu through a battle's end, and a `-keylog` from the
  menu replayed (2026-10-08, `debian/window8`, Linux, headless window;
  port/README.md has the counts). Learned:
  - The port needed no change. The walks `end57` and `live57` of
    `menu_dumps.py` (map 57's code, START, `m57_win`'s battle 8 s
    earlier with its pokes, the end) are the same as the original at
    the menu's two returns, both end waits, the frames 1 to 121 of CDh
    and the menu after the battle, both video pages included, and the
    best scores' file both write (`MAP\04.HI`) is the same.
  - After a battle won from the menu the original shows CDh, then
    asks for the best score's name ("PLEASE ENTER YOUR NAME !", about
    76 s in the walk), shows the best scores and waits for a key, and
    is at the menu again (91.9 s); START then takes one press. With
    `-map` this part was never reached (the play ends at `S066A:4728`).
  - The runner's written files must be emptied before every run of a
    walk: a best score left from an earlier run is read by the next and
    adds a second entry. `menu_dumps.py` now gives each walk its own
    `-state` folder, emptied before each run (the older walks rerun with
    it, see port/README.md).
  - In the window from the menu the pictures are N times 0.014268 s
    from 0 on (`live_keys.py KEYS 0 0`), the menu's first among them.
  - The port's battle on its own clock took one pass more than the
    original's (348 against 347): game_state's count and the timer's
    due pass at its byte 28h (`S331F:4CCC`) differ by it and are told
    apart as `play_cmp.py` does.
  - The committed `menu57_win.keys` has `code1` to `code5` for the
    map's code (game data, from `CODES.DAT` at run time).
  Not done: the name's and the best scores' screens as video memory
  (they are not passes of the menu's loop), the battle on the
  original's clock inside a menu walk, the SDL window by hand, the
  sounds.
- The HISCORE screen and the name's and best scores' screens
  (2026-10-08, `debian/window9`, Linux; port/README.md has the counts).
  Learned:
  - The port needed no change. The screens are caught where the
    original calls something once per screen: text_input's key_remove
    (S167A:334E) and key_install (3604), and behind scores_show (0AAE
    after a battle, 1C7C for HISCORE). text_input also serves the map's
    code, so every walk that types a code has two such screens; they
    compare the same too (page parity included).
  - Memory and video memory there are the same as the original's in all
    six walks that have such screens, the window's (`live57`) too.
  - `menu_test.sh live` is meaningful only for a walk whose key script a
    `-keylog` wrote (`live57`); for the others the keylog differs.
  Not done: HISCORE with scores present, the backspace in the name, the
  SDL window by hand, the sounds.
- The best scores with scores there and the name corrected (2026-10-09,
  `mac/scores`, Mac, headless window; port/README.md has the counts).
  Learned:
  - The port needed no change. Four new walks of `menu_dumps.py`:
    `name57` (the winner's name with the backspace, then MISC's HISCORE
    for the map just won), `twice57` (begun with the scores' file
    `name57`'s run wrote: HISCORE for map 57 with that score, the battle
    won again, a second name) and the two as the window played them
    (`livename57`, `livetwice57`).
  - A name shorter than five letters keeps the cursor: text_input writes
    the `]` into the text behind the letters while it is shown, and the
    five bytes behind the name's first are copied into the scores
    whatever they are. With Enter pressed while the cursor shows, the
    file has the `]` as the name's next letter and the best scores show
    it (`name57`: four letters and `]`, in the original's file and the
    port's). With the cursor away it has a 0 there (`twice57`, three
    letters). A name of five letters was not run this way.
  - The cursor changes every 17th pass of text_input (the count above
    0Fh), and a pass is a retrace in the original (0.014268 s between
    most of the 784 passes logged at `S167A:35C6`; a pass that takes a
    key is longer). The port's passes on its own clock are others: the
    code's typing took 272 passes in the original and 280 in the port,
    and the port asks for the name earlier (76.26 s against 77.16 s in
    `name57`). So whether the `]` is in the name, and which of the two
    pages has the pointer afterwards, depends on both counts. `name57`'s
    Enter is at 84.825 s for that: 535 passes of the name's typing in
    the original and 601 in the port, 31 and 35 changes of the cursor,
    and 807 and 881 passes with the code's, both odd. The page's side
    of it holds for a few milliseconds only (at 84.820 the original had
    806). `twice57`'s Enter at 91 s: 333 and 402 passes, 19 and 23
    changes; 605 and 682 with the code's, so its pages are exchanged in
    the menu afterwards. `menu_cmp.py` now tells that apart: each page
    is then compared with the other program's other page.
    The port's counts were read with a print put into text_input for
    the measuring and taken out again (not committed); `name57`'s were
    counted before origin/master aea5a2e and 4dfe673 were merged, and
    not again afterwards (the comparisons were).
  - The backspace with nothing typed changes nothing, a sixth letter is
    not taken; the text and the screen at text_input's end are the same
    as the original's after both.
  - A second score equal to the first (the same battle won twice, A0h)
    goes into the file's second place; the file and the screen are the
    same in both. Which of the two the screen shows first was not
    looked at.
  - A matter of `play.c`'s own clock, not changed here: with the
    battle's keys of `m57_win` moved by 9.00 s (START 9 s later than
    `end57`'s) the port's second wait at the battle's end was over
    0.057 s after it began, the script's Space still being held from the
    first, where the original waited for the next Space 5 s later. Of 16
    shifts tried in the port 6 waited as the original (9 s less 0.06,
    plus 0.03 to 0.05, 0.08, 0.10). Presumably it is whether the timer's
    routine had filled the side's button again when the wait began; the
    original's code was not read for it. In the original only 9.00 and
    9.04 were run, both waited. `twice57` has 9.04.
  - With the same count of passes in both (`twice57`: 349) the first
    view's repeat counter (`S331F:4DBE`) still differs by 1 after the
    battle; `menu_cmp.py` now tells it apart for every walk with a
    battle played to its end, not only where the counts differ.
  - `menu_test.sh live` is meaningful for `livename57` and `livetwice57`
    (their scripts are the window's `-keylog`), not for `name57` and
    `twice57`.
  Not done: four scores with the lowest replaced, a score not above the
  lowest (no name asked for), a score outside 0 to 7EF4h in a file, the
  screens between the name's keys (only text_input's beginning and end
  are dumped), the SDL window by hand, the sounds; run on macOS only.
- The computer's attack as the second side in the window (2026-10-09,
  `debian/window10`, Linux, headless window; port/README.md has the
  counts). Learned:
  - The port's play needed no change: `m8_cattack` (map 8, the first side
    only ends its halves) is the same as the original at every pass, at
    every tenth frame of both fights shown with pictures (memory and both
    video pages), on its own clock round by round, and between window
    and `-keys`.
  - On the original's clock the port had started the fights' pictures
    with the BIOS's count, not the original's seed, and given
    `rand_words` back at their end; only dumps inside the pictures show
    it. `play_test.sh` now passes `-seeds` there too.
  - `S2532:156D` is the place in fight_frames where a frame ends and the
    key is tested; `play_dumps.py` dumps there (`gFNNN`). In this script
    the original's two fights with pictures (both in pass 1850) end at a
    key after 149 and 151 frames (which key not checked).
  Not done: a fight's pictures to their end on the original's clock, the
  SDL window by hand, the sounds.
- The window's leftovers (2026-10-09, `debian/window12`, Linux, headless
  window; port/README.md has the counts). The gaps the window notes
  left, each run and compared or explained:
  - The repair in `m8_mouse` (the unit not damaged): the original shows
    text 50h for the first side at 322.74 s (`show_message`'s AX in the
    log), so the kind's full count was already reached (HL.hints at
    `side_points`); the port's state was the same there already.
  - The mouse's mickeys below a pixel: each `-micelog` line is where the
    motion put the driver, its kept mickeys counted in, and `-mice` puts
    the driver there at the same time, so the positions the game reads
    are the window's at every time; only the driver's own rest differs,
    which is not the game's memory (comment in `play.c`; the `live` runs
    of `m8_mouse`, `m0_cmouse`, `m8_scroll` are the same throughout).
  - The bytes "behind the pages" (1,442 in every comparison): they lie
    at fixed places off the screen (7E84-7FF8, A398-A50D, C4E0-C57F)
    and are in the original's video memory from about 1 to 2 s after
    HL.EXE starts, the publisher's logo screen (`BB.IFF`, `HLLOGO.IFF`),
    which the port does not show; the menu walks have them too (810).
    No later drawing reads them (the pages are the same everywhere).
  - The 11,220 to 12,947 bytes behind the pages after fights with
    pictures on the original's clock: the port showed all 749 frames of
    each fight, the original's key ended them at 149 and 151
    (`m8_cattack`, the F1s at 120.006 and 129.994 s). The clock's file
    had no line for a fight's frame, so on the original's clock no key
    reached the frames. `play_dumps.py` now writes a `G` line for each
    `S2532:156D`, and `play.c` (`clock_frame`) runs the log's lines up
    to a frame's G line at that frame (timer, keys, the waits' and
    pictures' clearing of the key) and leaves S and R for the round's
    end as before. The W lines of the waits between two fights had to
    go with them: left for the round's end they cleared the F1 the
    second fight had left for the next part. Logs from before
    `debian/window10` have no `S2532:156D`, so no G lines; for them the
    frames take no time as before (`have_g`). Behind the pages 632
    bytes are left (the logo's that the fights do not cover).
  - `m8_d5r`/`m8_rmouse` with fight pictures do not reach the repair in
    the original either (run here: no text 51h; its last texts 36h, 33h,
    34h at 322 to 339 s): the times were written for fights without
    pictures. Not a matter of the window.
  - `m8_rmouse`'s fight 2 frames: word 4 of `rand_words` (DATA:0AF5)
    differs in the port on the original's clock. `seed_set` writes words
    0 and 2 only; words 4 and 6 are what fight 1 left, and the port gives
    back at a fight's end the words kept before it (for the round's
    replay of the S and R lines). The pictures and every pass are the
    same; not changed.
  - `rounds` from round 4 on (`m8_site`, `m8_mouse`): with the original's
    seeds (`seeds`) `rand_words` first differ in round 3's pass after
    the fights with pictures: on the port's clock a key ends the
    pictures at another frame (`m8_cattack` on its own clock: frame 215
    against 149 and 151), so rand_next is called another number of
    times. The same cause as `m54_end`'s, a phase of the port's passes
    against the script's keys.
  - D0h, CEh and CFh on the port's own clock: still not reached.
    `m54_own.keys` (`m54_end`'s with every direction let go together
    with the button, as `debian/window6` found both read alike) does not
    win on the port's clock either (6,128 passes, no end), nor does the
    window's play of `m54_end` (6,565 passes). The cause is the one
    `debian/window11` found; a script for it needs keys that no phase
    changes, not written. The original was not run with `m54_own`.
  Not done: the SDL window by hand, the sounds, G lines for the older
  logs (their runs would have to be made again).
- A battle's end on both clocks (2026-10-09, `debian/window13`, Linux;
  port/README.md has the counts; `port/tests/endclock_test.sh`).
  Learned:
  - Do the end pictures need a clock of their own? No: on the port's
    own clock all kinds go through the same `ev_hook` (frames counted by
    the timer's calls into word 22Eh, retraces, fades, per-kind load
    times), and D0h and CFh reached by pokes (`m57_won`, `m57_medal`:
    the battle ended with its first pass, a round and a score poked) are
    the same as the original's at every tenth frame, pages included.
    Their time was not: behind D0h's last frame the original takes 2.74 s
    to battle_score (2.725 in `m54_end`), the port counted 1.63 s, so
    CFh began 20 BIOS ticks early. The difference is presumably D0h's
    `music_fade(0Ah)` (left out, like the `music_fade(4)` that
    `T_AFTER` stands for; not checked what in it takes the time). Counted
    now (`EV_WON_END`, 1.11 s): every tenth frame within 2 ticks, the
    end at `S066A:4728` within 1.
  - game_loop tests whether the battle goes on before a pass begins:
    poked at `S066A:0EE9` (pass 1) with 2 cleared, the original plays
    that pass and leaves after it (its log: P, P, L). The port tested
    after its pokes and left before pass 1; `play_game` now tests the
    state from before them. Inferred from the log, the test itself not
    read in the original.
  - The battle on the original's clock inside a menu walk was played
    the same (`m57_menu`: every pass, both waits, the 13 frames of CDh),
    but the port's menu behind it never got a key: `play_begin` clears
    `in_menu`, and with a log the keys come by its K lines only, so the
    name's screen waited for ever; the own clock also still stood at the
    menu's end (22.9 s). `play_after` now goes on with the own clock from
    the log's time at `S066A:4728` and its next timer call, the menu's
    keys by their times. A log made to 100 s has the original's second
    battle in it (START at 94 s, the battle at 97.9 s) and its dumps are
    of that one: `m57_menu` runs to 95 s. One battle a clock's file: a
    second battle in the same run would replay the log from where it
    stands (not supported, not needed by any test).
  - The scores' file of `m57_menu` differs in the name's second byte
    (the cursor `]` in the original's, 0 in the port's): text_input's
    passes after the battle are the port's own, as in `name57`'s notes;
    not changed. `end57` (all on the own clock) has it the same.
  - `play_cmp.py rounds` takes the setup's bytes from the first pass
    when no round's end was dumped (a battle over before one); before,
    the bytes `-map` leaves unwritten (the menu's, `S37FF`) counted as
    differences at the frames.
  Not reached: CEh on the own clock (needs 0Fh or 11h in the first
  view's word 22h), D0h and CFh on the own clock by play without pokes
  (the phase matter of `debian/window11`). Not done: the SDL window by
  hand, the sounds. (All three reached since, next bullet.)
- The end by play on the own clock (2026-10-09, `debian/window14`,
  Linux; port/README.md has the scripts and counts). Learned:
  - A key script survives both clocks when every step of a cursor is a
    tap of 0.06 s and every gesture lets its direction go 0.03 s after
    the button (as `m60_fights` found), and when each tap lies a second
    or more from a gesture. A tap of 0.05 s 0.8 s after a gesture was
    lost on the original in a first try (round 8 of `m54_hq`; spaced
    to 1.3 s and 0.06 s it was read). With 0.03 s after the button the
    gesture's direction does not step the cursor at all; with
    `m54_end`'s 0.2 s it stepped once more on most move menus, which
    `m54_hq` gives as a tap of its own (`2u70`, `2u132`, `2u163`,
    `1u187`, `2u205`, `2u236`). Found by printing each side's cursor
    (view word 0) and state (byte 1Bh) every third pass of the port on
    the original's clock (`-clock` of `m54_end`) and on its own, and
    making the own clock's sequence the same.
  - While a move aim is chosen (view state 4) one step of the cursor can
    move it two fields: seen as one input word (0200h, down) at one pass
    and the field two rows further at the next. Not read why
    (presumably the cursor skips fields it may not go to, or the view
    scrolls by two); the same on both clocks, so a script finds its way
    by trying (on map 57 `d` and then `u` went one field down).
  - `field_bits` is 64 bytes a row whatever the map's width: during a
    move aim bit 1 marks the fields the unit may go to (40h its own),
    which shows a unit's range without trying.
  - Map 57's second headquarters (7,11) has the second side's units on
    (6,11) and (8,11) beside it and can be entered from (7,10) only; the
    first side's cavalry came there in six move halves ((8,1) to (9,4),
    (9,7), (7,7), (7,8), (7,9), (7,10)) after the second side moved its
    unit from (7,10) to (7,9) and on to (6,9). Its range shrinks near
    the second side's units (one field a move from (7,7) on); not read
    why.
  - The pictures' time from their first frame: every tenth frame within
    3 BIOS ticks of the original, `S066A:4728` within 3. The battle
    before them drifts (the first frame 7 or 8 ticks later in the port
    over 15 rounds: the passes' phase and the timer of
    `debian/window11`), so `endclock_test.sh` now counts the pictures
    from their first frame and prints the battle's drift apart.
  Not reached: 15h (both sides without units), CDh with CEh (a first
  side that wins by fights), a score not above the map's best on the own
  clock. Not done: the SDL window by hand, the sounds. (All three
  reached since, next bullet.)
- The battle's three end edges left (2026-10-09, `debian/window15`,
  Linux; port/README.md has the scripts and counts). Learned:
  - A fight's losses draw no random number (`fight_losses` seeds
    rand_state from the record and draws nothing): they follow from the
    two counts, the experience (byte 1, one more after each fight both
    were in) and the grounds' bonus (class: byte 5 of the ground's
    record in `ww1gnds_dat`, the bonus in `ground_bonus`). Rounds where
    neither side attacks change nothing of them. So which fights a
    script gets was worked out beforehand: `fight_losses`' sums (as
    port/src/fight.c has them) over counts, experience and ground
    classes, checked against m60_fights' five fights (6/6 to 0/1, all
    on class 8) and then by the port.
  - On map 60 with both cavalries on class 8 the fights go 6/6, 5/5,
    4/4, 3/3 and then the attacker loses 1 and the target 2: no order
    of attackers reaches 1/1 there (all 112 orders of 5 to 7 fights
    tried on the port end 2/1 or 1/2, then 1/0 or 0/1). An attacker
    on class 1 against a target on class 8 at 3/3 (experience 3) and
    2/2 (experience 4) takes one from each; at 1/1 (experience 5) a
    fight on class 8 against 8 destroys both. (44,19) is class 1 and
    touches (45,18) (class 8); (45,19) is class 8.
  - A side's half is a move half and an attack half by turns: the field
    menu's "up" (`field_menu`, view byte 16h bit 0) is the move aim in
    one and the attack aim (target marked 4 or 8 in `field_bits`, state
    0) in the other. On map 60 after the walk the first side attacks in
    the odd rounds from 1454.5 s and moves in the even ones, the second
    the other way round; a move aim in an attack half marks no field.
    So a side attacking twice needs an odd number of rounds between.
  - Next to the other side's unit the second side's cavalry may move one
    field (only (46,19) and then (45,18) marked), the first side's many
    (its range around (46,19) reached (44,19)); not read why.
  - The second side's end of a half: a Space given just as the end's
    texts appear (5.75 s after them in one try) ended both waits at
    once (the second 0.06 s after the first); 7 s and 13 s after the
    texts each ended one.
  - 15h (both without units): no picture behind the loop at all (no
    10h, so neither D0h nor CDh; battle_score gives no score), the texts
    42h for both sides.
  - A score not above the best: battle_score compares the score with
    the lowest of the four as signed dwords and goes on only when it is
    above (the `JBE` before `S063F:025D`, read in build/HL.ASM): equal is not
    above. The file is `MAP\57.HI` for map 57 (`num_text` of the map's
    number, as the port names it `MAP_57.HI`; `m57_menu`'s `04.HI` is
    not map 57's, not followed). With four scores 620, 520, 420 and 320
    (m57_hq's own) neither draws the random number nor shows CEh.
  - Time of CDh then CEh on the own clock (`m60_first`): by the BIOS's
    count the two agree from CDh's first frame to its 121st (235 ticks
    each), then the port is 7 ticks later at CEh's first frame and
    stays so. By the runner's own clock that stretch (CDh's last frames,
    `wait_frames(0x64)`, `fade(0)`, battle_score, CEh's loading) is
    4.207 s in the original and 4.273 s in the port, about a tick
    apart; but the original's timer routine was called 286 times in it
    (68 a second, 70.6 over CDh's first 121 frames, 72.3 after), and
    its BIOS count goes with those calls. The port's retraces and fades
    are timed by the screen's rate, its timer at 72.8 a second. Not
    read why the runner's timer runs slow there (a doskit matter, not
    changed here); presumably the same lies in D0h's tail, which
    `EV_WON_END` (1.11 s, "presumably music_fade") makes up for, not
    checked. `endclock_test.sh` allows 7 ticks for `m60_first`.
  - `play_dumps.py` keeps an existing `log.txt`: a run redone into a
    folder whose dumps alone were removed reported the old run's end.
    Remove the folder.
  Not done: the SDL window by hand, the sounds. Not looked at: the two
  map-60 differences on the original's clock, which are `m60_fights`'
  (pass 1's setup bytes, pass 4748's bytes 5 and 8).
- The field menu's information (2026-10-08, Mac; `unit_info`,
  `unit_values`, `info_rect`, `battle_info` in src/HL.hints with what
  each does; `port/src/draw.c`). Learned:
  - Both routines only draw: they change nothing of the game's state,
    only the screen's record (colour and rectangle) and, for the unit's
    large picture, the buffers `big_unit` reads into. All their helpers
    were translated already (`pic_unpack_draw`, `ground_blit`,
    `big_unit`, `unit_panel`, `num_draw`, `game_text_at`, `view_dim`,
    `cursor_field`, `rect_fill`, `latch_copy`).
  - The unit's screen names more of the kind's record by the texts it
    puts beside the bytes, as the new units' screens do: 0Ah, 0Bh, 0Ch
    the weapons against air, ground and water (the order of the three
    lines on the screen is AIR, GROUND, WATER; the new units' screen
    prints 0Ah, 0Ch, 0Bh beside AIR, WATER, GROUND, so the same bytes),
    8 less 1 and 9 less 1 the ranges (9's only where the kind's word 6
    has 4 or 8), 2 armour, 1 speed, 41h weight, 3 the group's size. Not
    checked against the fight's code.
  - `battle_info`'s numbers: `game_state`'s word 11h beside ROUND, words
    87h and 8Bh beside BEST and CURRENT, bytes 8, 0Ah, 9 in the column
    MAP; bytes 2, 4, 3 of a side's record of `sides` beside UNITS, STORES
    and FACTORY UNITS; `side_points` and `side_income` beside RESOURCES;
    the side's bytes 15h and 16h of `sides` beside MOVE and LIMIT (FFh:
    "no restriction"). With 8 in `game_state`'s word 2 a side does not
    see the other's resources.
  - Leaving the information (`S066A:248E`) draws the view and its marks
    again; the port had that part without the two calls, so the page
    shown kept the information until something else drew. Found by the
    video memory at the next call, not by the memory.
  - The original's clock goes on inside the two routines (5 bytes of
    DATA at each return); the port's play on the original's clock has no
    place for that, and nothing in the game's state depends on it here.
  - Compared on map 48, both sides (`sh port/tests/info_test.sh`,
    port/README.md "Checked"). Not reached: listed there (a unit of two
    parts first of all).
- Saving and loading, read whole and translated (2026-10-08, Mac;
  `save_game`, `load_game`, `save_blocks` in src/HL.hints with what each
  does; `port/src/save.c`). Learned:
  - `save_game` asks for the number in a loop of its own that only reads
    byte 0 of the key's record (scancodes 2 to 0Bh are 1 to 9 and 0; the
    key of entry 0 leaves with text 5Ah and the view still dark), so the
    timer's routine is all that runs meanwhile. The header is built in
    the first side's work buffer (the far pointer at `sides` + 0Dh), and
    `save_blocks` writes its list of 24 blocks over it there: after a
    saving that buffer holds the list (C0h bytes).
  - The seventh block (320h bytes) is the buffer `game_loop` hands to
    `pictures_load` for the colours: the saved game holds the palette.
    The list has `S331F:4C9E` (the map's three words, 6 bytes) between
    the two views and `field_bits`; the list in the bullet "The saved
    game's layout" above leaves it out, `tools/savfiles.py` has it.
  - The two blocks of 1806h bytes are the maps with what lies behind
    them in `game_loop`'s block: beyond `map_bytes` they hold what the
    menu's files left there. So a saved game is only the same in two
    programs that went through the same menu; the port started with
    `-map` writes other bytes there (3,111 and 4,998 of them in the
    first try) and no map's code in the header.
  - `load_game` is called after the whole setup of the map the menu
    stood at, reads the blocks over it, and `game_loop` then loads the
    grounds and the pictures again. It clears the music's and effects'
    bits (40h, 80h of `game_state`) when this machine has neither (10h,
    20h of word 2 before the load). Its result is ignored: a number
    without a file gives the menu's map from its beginning (the menu
    looks for the file before it sets 200h, so that is not reached from
    there).
  - `S1571:0426`, called around the file's work, is empty.
  - For the comparison the port's program had to go through its menu and
    then play on the original's clock; the timer's routine fills the
    sides' records on every fourth call, so which of the four it is at
    (`DATA:0B19`) must be the original's at the battle's first pass
    (`play_begin` sets it from the log's calls; the menu on the port's
    own clock leaves another, seen as a button read three passes early).
  - Compared (`sh port/tests/save_test.sh`, port/README.md "Checked"):
    the file byte for byte, the passes around the saving and a loaded
    battle through a round's end. Not compared or reached: listed there.
- The menu's loading, read again and compared further (2026-10-08, Mac,
  branch `mac/menuload`). The task began from "the menu's own loading is
  not translated"; it was: `saved_game` in `port/src/menu.c` and
  `load_game` came with `mac/save-keys` and `m48_load` compared them at
  11 passes. What this branch adds is the reading of the branch
  instruction by instruction (src/HL.hints, under `menu_run` and
  `load_game`), the cases nobody had run, and the comparison at every
  pass. Learned:
  - The question's loop (`S167A:192E`) has no wait and reads the BIOS's
    keys only: a digit is the number, the key of entry 0 (Esc) leaves, a
    letter, the backspace and a side's key for its button are passed
    over (the program's keyboard interrupt is out, so no button comes
    from a key; a joystick's or the mouse's would leave it: not run).
  - SAVED GAME NOT FOUND stays until a key or button comes
    (`wait_frames(32h)`, then `inputs_release(1, 1)`), not for a fixed
    time; then the page the question was asked from again.
  - The menu only asks whether the file can be opened. What it holds is
    `load_game`'s matter, and `game_loop` ignores its result, so a
    damaged file never gives a text: a wrong second byte or an empty
    file gives the menu's map from its beginning (with the bytes read
    lying in the first side's work buffer); ten bytes of a saved game
    give that map with the header's fields set from what the buffer
    held (the first side stood at "AWAITING COMMAND" in the run); a file
    cut inside a block gives everything before the cut (cut at 7000h,
    inside the kinds of unit, map 48's battle went on and its round was
    carried out).
  - `text_input` shows the other page every pass, so which of the two
    pages has the pointer after a code was typed depends on how many
    passes the typing took: a walk on the port's own clock can come out
    on the other one (688 bytes "different" in the first try; that pass
    is not kept in the walk `noload`).
  - Compared (port/README.md, "Checked"): the walk `noload` of
    `menu_test.sh`, and in `save_test.sh` every pass of `m48_load` from
    1 to 232 (the round's end is in 229) and the four damaged files.
    Not done: listed there.
- The keys F2 to F9 translated and compared (2026-10-08, Mac;
  `setting_keys` in `port/src/loop.c`, `sh port/tests/fkeys_test.sh`).
  What the reading of 2026-10-04 above guessed is confirmed or put
  right: F7 and F8 are the mouse (off and on; its speed, `S37FF:0113`,
  through `mouse_speed` with 32h, 0Ah or 3); F5 and F6 switch
  `DATA:03B8` and `03BA` (a side's joystick wanted) and call `joy_setup`
  (`S2B9B:000A`), which looks for the sticks anew, so without a game port
  the text says "on" at every press; F4 has no 200h test and so runs in
  both sides' parts of every pass while its key is the pass's; F6's text
  always stays by timer 2. F9: `no_fight_pics` set gives text 72h and
  clears it and `S37FF:0130`; else `0130` set gives 70h and sets
  `no_fight_pics`; else 71h and `0130` set. The mouse turned on again by
  F7 gives the first side a direction once in the original (the driver's
  pointer after its reset against the middle, presumably; `input_mouse`,
  `S27FE:03D4`, is not translated). Put right on `mac/options`:
  `input_mouse` was in the port since `debian/window2`, and with it the
  port gives that direction too; and both sides' joysticks are wanted
  from the start (`DATA:03B8`, `03BA` FFFFh), so the first F5 or F6
  switches one off (the next bullet).
- The mouse and the joysticks as a side's input (2026-10-09, Mac, branch
  `mac/options`; `port/src/joy.c`, `port/src/input.c`,
  `sh port/tests/options_test.sh`; port/README.md has the comparison).
  Read in `HL.EXE` and seen in runs:
  - `joy_read` (`S2BAD:0006`, named now) writes the game port (201h) and
    reads it: a button of the first stick held (bits 4, 5 clear) gives
    FFFFh in `DATA:038C`, one of the second's (bits 6, 7) in `0392`.
    Then, interrupts off, it reads the port up to as many times as
    `03B6` says until each axis bit of the sticks that are there (the
    byte `03B4`) has fallen, one axis a read, the lowest bit first; an
    axis's word is the count at which it fell: `0388`, `038A` the first
    stick's X and Y, `038E`, `0390` the second's; `03B6` itself for an
    axis that did not fall. So the numbers are the machine's: on the
    runner at 6 million instructions a second a one-shot of 570
    microseconds counts 2A5h to 2A8h, depending on which axes fell
    before it (a read that takes an axis is not counted and is longer).
  - `joy_setup` (`S2B9B:000A`) clears which sticks are there (`0384`,
    `0386`), `03B4` and the ranges, makes `03B6` FFFFh, writes the port
    and reads it after two retraces: a stick whose two axis bits are 0
    is there. It is measured by `joy_read` where it stands, and that is
    its middle: the range is twice it (`0398`, `039A`; `03A8`, `03AA`),
    the edges the middle less and plus half of it (`039C` to `03A2`,
    `03AC` to `03B2`: at or below the first is left or up, at or above
    the second right or down). So a stick held away while F5 or F6 is
    pressed has its middle there (run: the second stick measured at its
    left, edges 3Ah and AEh, its true middle then reads as right). The
    second stick is measured after two more retraces. `03B6` becomes the
    largest range, 0 without a stick. The edges are not cleared.
  - The timer's routine calls `joy_read` on every fourth call while one
    of the two sticks is wanted (`03B8`, `03BA`), whether one is there
    or not. Both words are FFFFh from the program's start: the sticks
    are wanted until F5 or F6 says otherwise, and `input_install` calls
    `joy_setup` once, so a stick plugged in at the start plays at once
    (run `m48_joy0`), one plugged in later only after an F5 or F6.
  - `input_side0`/`input_side1` with the side's stick wanted and there:
    its directions by the edges (each begins the opposite one's count of
    holds again) and its button; only when the stick gives nothing at
    all are the side's keys and the mouse asked. A stick taken away
    while it is there reads `03B6` in both axes: right and down for ever.
  - The timer's routine goes on while `joy_setup` waits for its
    retraces. In the second stick's wait that stick is already said to
    be there, with the edges of the time before and `03B6` FFFFh: held
    left, it gave its side "left" once in the original (pass 652 of
    `m48_joy`). The port's `joy_measure` waits by the timer's calls for
    that reason.
  - The counts of holds and gaps (`DATA:03D2`, `03FE`) and the timer
    routine's call of four run on from the menu into the battle. The
    port's menu takes another time, so they stand elsewhere at the
    first pass (4 gaps apart in these walks); given the original's
    there, they stay the same through the battle. A second press's
    byte 5 depends on them, so on the port's own clock a double press
    can come out otherwise than in the original: not looked at further.
  - A line of the sticks' script whose time falls into a read of the
    original (0.6 ms long with a stick in the middle) changes the stick
    in the middle of that read; the port changes it between two calls
    (seen with "62.00 1,off"; the script avoids it).
  - `loop.c` was not to be touched: its `joy_setup` (no game port:
    clears the words) still runs at F5 and F6, and `input_tick` starts
    `joy_measure(1, now)` at its next call when a wanted word has
    changed. For the owner of `loop.c`: delete its static `joy_setup`
    and call `joy_measure(1, play_now())` there instead (or give
    `joy_measure` the pass's time); then `input_tick`'s test of the
    wanted words can go, and only its call at the first tick (for
    `input_install`'s) stays.
  - Not done: the window (no stick there: `plat_sdl.c` gives a
    controller as keys; a stick would call `joy_stick` with one-shot
    times made from its axes), a real game port's speed (the counts are
    the runner's machine's, as the T_ numbers of `play.c`), `joy_read`
    with `03B6` 0 and a stick there (10000h reads). (The texts of F5 to
    F8 as video memory and the sticks in the menu: the next bullet.)
- The joysticks' leftovers (2026-10-09, Mac, branch `mac/options2`;
  `sh port/tests/options_test.sh` with its new ways `map` and `own`,
  the walks `joy57` and `joyload` of `menu_dumps.py`/`menu_test.sh`;
  port/README.md has the counts). Seen in runs of the original and of
  the port beside it:
  - The menu takes a stick as it takes the first side's keys: a
    direction moves to the next item (what comes in is 5, as for a
    key), either button chooses (1; the second button chose LOAD in
    `joyload`). START takes the stick's first press, where the walks
    by keys need a second after a code was typed: the button's key was
    let go while the keys were the BIOS's and the program still had it
    as held, and a stick's button is no key. START with no code typed
    leads to the countries; left there and the button gave map 24 as
    the key does.
  - The saved game's question (`S167A:192E`) is left by the stick's
    first button (the passes of the menu's loop go on 0.4 s after it,
    the MISC page again), as the reading of `mac/menuload` expected;
    right and left do nothing there. The second button at the question
    was not run here (it came while the menu was back and chose LOAD);
    it is in the next bullet.
  - `text_input`'s cursor (the `]`) comes and goes by the passes, so
    what the page holds when Enter ends it is a matter of the clock:
    with Enter at 15.0 s the original's page had it and the port's on
    its own clock did not, at 15.1 both the same. The original's state
    with Enter from 15.0 to 15.6 in tenths: at 15.0, 15.3, 15.4 and
    15.5 one, at 15.1, 15.2 and 15.6 the other (which of them has the
    cursor was read at 15.0 only: the original's, by the byte 5Dh of
    the block).
  - `-map` with a clock's log: the port makes the log's calls of the
    timer's routine from the program's start, so the sticks' script,
    which counts those calls, is at the right time in the battle with
    nothing done for it (`joy_battle(0)`). What a `-map` start cannot
    have is what the original's menu left: the pointer's range
    (`DATA:043A` up to 0442; the menu's `mouse_range(8, 4, 12Ah, C0h)`
    holds on through the battle in the original) and the counts of
    holds and gaps, which the test pokes. A `-poke` of the port was
    made before those calls; it is made when the first pass begins
    now, as the original's runs get theirs (`S066A:0EE9#1`). Nothing
    else of the comparisons with pokes changed by it (`m57_win`,
    `m57_last` and the menu's walks with a battle rerun; `m48_undo`
    and `m8_scroll` have no dumps of the original on this machine: not
    rerun).
  - The port's own clock: a stick's tap of 0.056 s is read once by a
    side's part in both programs wherever it fell in this script (18
    of 18), as the keys' taps of 0.06 s; the timer's routine reads the
    sticks every 0.0549 s, so a shorter one can be missed. The counts
    of gaps (the second word of each of a record's five pairs, 0 to 10
    round) are the only bytes of DATA 0380 up to 045C that depend on
    when in such a period a pass begins.
  - The texts: F5 65h (off) and 66h, F6 67h and 68h, F7 6Ah (off) and
    69h, F8 6Ch, 6Dh, 6Bh, all through `show_message` with DI 0 (the
    first side's line; lines 189 to 193 of the page), still shown a
    second later. F6 says 68h with no second stick there, as read
    before. The port's page is the original's byte for byte.
  - `joy.c`: with a clock's file the sticks' script and the setup's
    wait did not move while the port's menu ran (the clock's calls
    begin with the battle), so a stick there from the start was found
    at the battle's first call only; they go by the port's own clock
    there now. Nothing compared reaches that: not verified beyond the
    runs' being the same as before.
  - Not done: listed in port/README.md at the same point; first of all
    a stick in the window (`plat_sdl.c`), which is the only way a
    player's stick reaches these routines.
- The joysticks' two gaps (2026-10-09, Mac, branch `mac/options3`; the
  walks `joyask` and `mouseask` of `menu_dumps.py`/`menu_test.sh`, the
  tag `m48_joymenu` of `options_test.sh`; port/README.md has the
  counts). Nothing of the port's source changed. Seen in runs of the
  original and of the port beside it:
  - The saved game's question (`S167A:192E`) is left by every button
    tried: the first stick's second button, both its buttons at once,
    the second stick's second button (the second side's byte 4; both
    sticks there from the program's start), the mouse's first button,
    its second, both at once. Each time the passes of the menu's loop
    go on 0.3 to 0.4 s after the press, on the MISC page with LOAD
    still the item (the stick's first button or the key then asked
    again). The pointer moved at the question with no button held does
    not leave it (the question stood from 26.0 to 30.3 s with the move
    at 29.0 and the buttons at 30.0). So the reading of `mac/menuload`
    holds: a side's button or one of the mouse's two words. Not tried:
    the mouse's third button, a button held from before the question.
  - A `-mice` line puts the pointer where it says, also in the menu, and
    there the item under the pointer becomes the chosen one: the first
    try let the button go at the question's own place (290, 181) when
    the menu was back, and the key after it chose another item in the
    original. The walk clicks at 98, 73 instead, where the menu puts
    the pointer on the MISC page with LOAD chosen (the mouse's record,
    `DATA:0424`, in a dump of the original at such a pass).
  - A stick moved in the menu before a battle on the original's clock
    (`m48_joymenu`: down and up in the menu at 9.2 and 9.6 s, which the
    original's menu takes as two changes of the item; held left from
    20.0 s, behind START while the battle is set up, to 23.6, into the
    battle; then down, the button, right and up at once, F5 off and
    on). The battle's first nine passes read "left" (4) for the first
    side in both; the tenth reads 8 in both (not looked into;
    presumably the mouse's direction when the stick gives nothing
    again). Nothing in the setup between the menu and the first pass
    was seen to ask the stick: the first pass begins at 23.13 s as in
    `m48_joy0`, whose stick is at rest there.
  - Found by the way, not changed: the port's menu has its first pass
    0.84 s behind its `-start` (`play_start` sets the clock when
    `menu_run` begins; the fade and `wait_frames(5)` follow, and what
    came in is cleared before the loop), while the tests give as
    `-start` the time of the original's first pass (its `menu_run`
    began 1.36 s before that pass in one run: `S167A:0005` at 6.69 s,
    the first `S167A:130B` at 8.05). So a key or a stick in the first
    0.84 s of the original's menu is taken there and not by the port:
    a stick's down at 8.6 s was (the original at 8.62 s, the port
    nothing; tried in the port: taken from 8.7 on, at 8.91 s), and the
    walk then went to map 0. Every walk's first input is at 9.2 s or
    later, so no comparison so far reaches it. It is the tests' way of
    starting, not the player's menu; whoever wants inputs that early
    gives the port `-start` 0.84 s earlier, which moves every time of
    the walks compared so far (not tried).
- The joysticks' three remainders (2026-10-09, Mac, branch
  `mac/options4`; the tags `m48_joyhold`, `m48_mouse3`, `m48_joyptr` of
  `options_test.sh`, the walk `mouse3` of `menu_dumps.py`/
  `menu_test.sh`; port/README.md has the counts). Nothing of the port's
  source changed. Seen in runs of the original and of the port beside
  it:
  - The tenth pass of `m48_joymenu` is the mouse's. `game_loop` opens
    the mouse again behind the menu and the driver's reset leaves the
    pointer at 319, 99 (`DATA:0424`, 0426 in the dump of pass 1, in
    both programs), right of the middle (160, 100) by more than the
    step. A side's part asks the keys and the mouse only when its stick
    gives nothing, so while the stick was held left the pointer stayed
    there (still 319, 99 at pass 9); the first call of the timer's
    routine after the stick's letting go at 23.60 s asked the mouse,
    which gave "right" (8) once and went to the middle (160, 100 in the
    dump of pass 10, which began at 23.62 s; the five bytes found,
    `DATA:03BC`, have "right" there and nothing at pass 11). Checked by
    moving the pointer: `m48_joyptr` is the same walk with the pointer
    put at 160, 40 at 23.30 s, while the stick is held: pass 9 has it
    there, pass 10 reads "up" (1) and has the middle again, in both
    programs. With a stick at rest from the start (`m48_joy0`) that
    "right" is given before the first pass and never read by a side's
    part (the pointer is in the middle in its first dump; what clears
    the record between was not looked for).
  - The mouse's third button is word 32h of the mouse's record
    (`DATA:0446`, FFFFh while held; `mouse_read`) and nothing else:
    `input_mouse` and `mouse_center` ask the words of the first two
    only. In a battle (`m48_mouse3`) the third alone gives nothing; a
    move with it held gives its direction once and the pointer goes
    back to the middle, as with no button; with the first it is the
    side's button, and the first let go with the third held is not the
    case of the second held (no ten calls without the mouse: a move
    0.1 s later gave its direction at once); with the second it is the
    second alone (a move gives its direction pass after pass, the
    pointer not put back); all three give nothing, as the first and
    second together. F8 with the third held and a move behind it as
    without. At the saved game's question the third button held for
    0.4 s leaves it standing (the question stood from 17.03 to 21.33 s
    with the third at 19.0 and the first at 21.0; a second time from
    23.10 to 26.84 with the third from 26.0 and the second beside it
    from 26.5), and in the menu, on the MISC page with the pointer on
    LOAD, it chooses nothing (nothing came in between 23.10 and the
    Esc at 30.05 s). The port's question does not say when it was
    left; that the third button did not leave it there either follows
    from what came in (the first button at 21.0 on LOAD would have
    chosen it again) and from the screens outside the loop, the same
    in number and bytes.
  - Sticks held longer than a tap on the port's own clock
    (`m48_joyhold`: holds of 0.3, 0.5 and 1 s). The timer's routine
    reads the sticks every 0.0549 s and a pass takes as long, so a hold
    is read by one pass more or less by where in such a period it
    began: of the twelve holds nine were read by the same number of
    passes in both programs and three by one more or less (the first
    stick's up of 1 s 18 passes in the original and 19 in the port,
    its left with the button of 0.5 s 10 and 9, the second stick's
    button of 1 s 19 and 18). The input words are the same in the same
    order, and the game's state was the same at each of the 13 times
    compared, all with the sticks at rest (why the pass more of "up"
    moved nothing was not looked into; presumably the map's edge). It
    need not be: a first script, not kept, had the second stick's left
    of 0.5 s read by 9 passes in the original and 10 in the port, and
    from there on 4 bytes of the game's state differed (`S331F:4DC3`,
    4DCF to 4DD5; presumably that side's cursor one field further, not
    checked). The four holds of 0.5 s begun a quarter of a period apart
    were all read by 9 passes in both, so the original's own one more
    or less was not shown by them; it is taken from the 18 and 19 of
    the two programs' holds of 1 s.
  - What else differs in DATA 0380 up to 045C on the own clock with
    holds, all of it the counts': the counts of gaps stand further
    apart than a step (2 from the first pass here, with two sticks
    there from the start; they run on from the menu, and a count
    stands still while its direction is held, so a hold read once more
    moves it by one); and a count of holds keeps what its hold left
    until its count of gaps has gone round, when both become 0, so half
    a second after a hold one program can still have it and the other
    not (seen for the first side's left and the second side's button,
    with that side's record's byte 7 beside it, which is cleared with
    the button's count). Byte 5 of a record (a second press within 19h
    calls) depends on those, as said before; a second press was not
    run here either.
  - Not done: a second press on the own clock, a button held from
    before the saved game's question, the third button in the window
    (`plat_sdl.c`; not looked at whether it passes one on), a real
    mouse or stick; macOS only.
- The input options' last two gaps (2026-10-09, Mac, branch
  `mac/options5`; the tag `m24_ftext` of `options_test.sh`, the walk
  `heldask` of `menu_dumps.py`/`menu_test.sh`; port/README.md has the
  counts). Seen in runs of the original and of the port beside it:
  - The texts of F5 to F8 with the computer as the first side (map 24,
    reached by its code; a first stick there and at rest, no second):
    the nine texts 65h, 66h, 67h, 68h, 6Ah, 69h, 6Ch, 6Dh, 6Bh all come
    through `show_message` (`S10AF:009F`) with DI A0h, the second
    side's line, and the port's two pages are the original's byte for
    byte 0.3 s and a second after each key. `setting_keys` had that
    already (the line by bit 1 of the first side's word); nothing
    changed. The key is taken in the first side's part of the pass,
    the computer's, at eight of the nine presses and in the second
    side's at one (pass 378, the second F8), in both programs alike;
    why that one falls to the second side's part was not looked into
    (presumably where in the pass the key came).
  - A button held from before the saved game's question. The first
    stick's first button pressed on LOAD at 17.0 s and held to 20.0:
    LOAD is chosen at once (what came in at 17.05 s), but the question
    (`S167A:1929`) is drawn at 20.15 s, 0.15 s behind the button's
    letting go: the `inputs_release` before it waits five frames again
    as long as something was set since. A key in that wait is not
    taken: Esc at 19.0 s, while the button was held, did not leave the
    question, which stood until the button's next press at 22.0 (the
    menu's passes go on at 22.47 s). The mouse's first button pressed
    on LOAD at 24.0 and held to 27.0: the same, the question at
    27.14 s, left by the second button at 29.0. The port: 20.16 and
    27.11 s, what came in the same and in the same order, the memory
    and the two pages at both questions the same.
  - The port's `-menulog` has a line "ask N TIME" for each screen
    outside the menu's loop now (`ask_screen`, `menu.c`), and
    `menu_cmp.py` sets those times beside the original's (the runner's
    log). Found by it in the walks with a battle between (`end57`,
    `name57`, `twice57` and the window's three): `text_input` for the
    winner's name begins 0.91 to 0.98 s earlier in the port than in
    the original (76.25 against 77.16 s in `end57`), the screens
    behind it at the same times again. Not looked into (presumably
    the time of what is shown behind the battle on the port's own
    clock; whether it is what `debian/window13` found behind D0h was
    not checked). In the walks without a battle the times are at most
    0.06 s apart.
  - Not done: a key held from before the question (the key scripts
    have presses only), the second stick's button or the mouse's
    second held, the texts on the second side's line with a second
    stick there or on map 30, both in the window; macOS only.
- The program's end and the briefing (2026-10-08, Mac).
  - `S066A:4739`, reached when 1 is clear in `game_state` behind the
    menu or a battle (EXIT TO DOS sets 100h and clears 1 and 2):
    `mouse_hide`, `key_remove`, `S2B7F:000E` (the BIOS's mode of
    `bios_mode_old` set again when it is not 0), `S2CC9:0002` (each
    block of `mem_alloc`'s list at `DATA:0469` freed, INT 21h AH=49h, the
    count at `DATA:0467` 0), `S2F39:0092` (the sound's end; not read
    further), `input_remove`; `game_loop` returns its word [BP-26h] (7Bh
    after the last battle won). Translated but for the machine's part
    and compared (`menu_test.sh exit`, port/README.md).
  - The mouse's pointer: `game_loop` calls `mouse_open` again each time
    the menu returns; the runner's driver then has the pointer in the
    middle (A0h in word 10h of the record), the port's driver keeps the
    place the menu left. Seen as 3 bytes at the program's end and in
    every battle from its first pass (`DATA:0424` to `0426`). For
    whoever gives the port the player's mouse (`port/src/mouse.c`).
  - The briefing (`S274D:0003`): read whole, named and translated since,
    see "The new units' screens" below. What was known before: called
    with the map's number and two far pointers unless a saved game is
    loaded. Its outline from the generated source: it loads
    a file and a picture (`file_load_size`, `lbm_show`), libraries
    (`lib_load`, `lib_read`), starts and stops music (`S2F39:0F87`,
    `1058`), draws many texts (`text_draw`, with `L2488` and `L2B15`
    before them, presumably a number put into a text and texts joined;
    not read), steps the colours (`pal_level`) and calls
    `inputs_release`. One run only timed it: START on map 0 with the
    first country, no key after the country's (13 s): the routine was
    entered at 16.66 s and the battle's first pass began at 18.42 s, and
    the screen at 19 s was the battle's. So on map 0 it did not wait for
    a key there; whether it showed anything, and on which maps it
    waits, was not known then.
- The new units' screens (2026-10-08, Mac; `briefing` in src/HL.hints
  with all of it, `port/src/briefing.c`). What `S274D:0003` is: not a
  briefing of the battle but the units that are new on this map.
  - `NEWUNIT.DAT` (packed; `file_size` gives the unpacked length, which
    is what the free end moves on by) has a word for each of the 24 maps
    of a country, 0 or where the map's list is: a month, a year, and
    words up to a 0, each a screen: 1 or 2 a side's (the flag, NEW UNITS,
    the month and year), any other the place of a new unit's record
    (kind, the colours' base of its side, lines of text). A map from 18h
    on takes the entry of the map 18h below, so both countries see the
    same screens; a map above 2Fh (two players) has none. In the
    player's file maps 0 to 2, 7 to 9, 13, 18, 22 and 23 have none: that
    is why the run on map 0 did not wait.
  - Each screen waits in `inputs_release(1, 1)` for a key or a button,
    with no time limit; between two screens the original takes about
    1.6 s in the runner (the files and the two fades of 33 and 32 calls
    of `pal_level`).
  - The routine draws with the screen's cut off (`DATA:00E8` is 0 at
    its entry), unlike the menu before it, and uses the menu's record of
    MNUCHR14.LIB (`S37FF:001C`) and `lib_pal`'s record (`S331F:0131`)
    for its own loads of those libraries; `libs_start` loads the second
    again right after it.
  - Its buffer of 40h bytes is on the stack (`3EA4:0F48` in the runs):
    the names for `lib_read` are built there, so the port writes them to
    the same place.
  - The unit's screen prints bytes 1, 2, 3, 8 to 0Ch and 41h of the
    kind's record in `unit_dat` beside the words SPEED, ARMOUR, MAX GROUP
    SIZE, WEAPONS and SIZE/RADIUS (AIR, WATER, GROUND) and WEIGHT: names
    for those bytes from the game itself. That the bytes are used so in
    the fight's code was not looked at.
  - The compiler's `sprintf`, `strcpy`, `strncpy`, `itoa` and the huge
    pointer's post-increment (`ptr_post`) are named; of the first four
    only the beginnings were read.
  - Compared on maps 0, 4, 12 and 28 (`sh port/tests/briefing_test.sh`,
    port/README.md "Checked"): memory at the entry and the return and
    every screen's video memory the same. Not done or not known: listed
    there; and the two routines of the sound it calls (`S2F39:1058`,
    `S2F39:0F87`) are still not read.
  - The standalone test programs of port/tests that link `battle.c`
    had not linked since `battle_begin` calls `load_game` (and
    `loop_test` since `loop.c` calls the mouse's routines); they link
    again with `tests/standalone.c`. Five of them need dumps of the
    original that were scratch files and are gone (`build/n2/o/`): they
    were not run.
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
- The sound (2026-10-08, Mac, branch `mac/sound`). `S2F39` (the routines
  the game calls) and the driver's code in `S30EF` (`111F` to `2135`) are
  read whole from the generated source and named in `src/HL.hints` (the
  block "The sound" there has the channel and voice records, every
  routine and the events' meaning; this is the outline).
  - Two devices, the bits of `snd_devices` (`S30EF:025E`): 1 the AdLib,
    2 the Sound Blaster. 13 channels: 0 to 7 digital (IFF 8SVX, mixed by
    the driver into the card's buffer), 8 to 0Bh AdLib effects (ADLX), 0Ch
    the music (ADLX). The nine OPL voices are shared: the tune has the
    first ones (its AHDR's byte 0Ah says how many), an effect takes voices
    from the top down (`adl_used` counts them) and the tune's state for a
    voice taken is kept in a second copy and put back when the effect
    ends.
  - `sfx_play(channel, n, volume, times, step)` (`S2F39:06E3`) plays
    effect n of the file `sfx_load` loaded: an 8SVX form goes to
    `dig_play`, an ADLX one to `adl_play`, which only loads it;
    `snd_pass` (`S2F39:096A`, once a pass of every loop that sounds)
    starts it (`adl_start`). The timer's routine `adl_tick` (through
    `pit_add`, period 1843h until a tune or effect sets another: the
    menu's tune 06A9h, 700 a second) reads the events of the tune and of
    each playing effect and ends an effect at its event 77h (`adl_stop`).
  - Who calls `sfx_play`: `game_loop` (the cursor's step: effect 4 for the
    first side on channel 0Bh, 5 for the second on 9; 100h in
    `game_state`'s word 4 at the end of a side's part: the effect whose
    number is `site_taken`, same channels; both only with effects on, 80h
    in `game_state`), `panel_sound` (4 a figure drawn on the fight's
    panel, 5 one taken off; channel 8), the menu and its text input
    (channel 8, volume FFh), `attacks_run`, `event_picture` and
    `site_picture` (not looked at further). The fight's pictures call
    `dig_play` themselves (`pics_play`) and `chan_stop`. Volume DCh, once,
    step 200h everywhere in `game_loop` and the panel.
  - The music: `music_start(far ADLX, fade, volume)` (`S2F39:0F87`),
    `music_fade(speed)` (`S2F39:1058`), `music_volume` (`S2F39:1127`); a
    fade is a second timer routine (`fade_tick`, period 1D1EBh) that
    steps the volume and takes itself away; `music_fade` with a speed
    waits in a loop until it has. `game_loop` calls `dsp_rate(10000)`
    right after `snd_init` (which had set 8000).
  - Not decoded and without a caller found (bytes in the source):
    `S2F39:0F2C`, `11AC`, `149B`, `180E`, `1A0B`, `1A30` (what they
    presumably are is in the hints); `music_restore` (`S2F39:18FE`) is
    decoded but only reached through a flag nothing found sets.
  - Quirks read, kept in the port: `adl_play` takes `adl_load`'s FFFFh
    for success; `adl_start` leaves the lock (`snd_lock`) set when an
    effect needs more voices than are free, after which `adl_tick` does
    nothing until something clears it (not seen in a run); `dig_kick`
    halts the card and mixes anew in every pass while digital channels
    play (read, not listened to).
  - **A fault of the runner found** (doskit `tools/run/cpu.c`, `strop`,
    there since the kit's first commit): its fast path for REP MOVS/STOS
    added the count to SI for REP STOS too. `drv_clear` (REP STOSB of 488h
    bytes) therefore handed `snd_init` SI = 0488h, and `snd_devices` was
    048Bh in every run of that runner instead of 3. With the two lines
    changed in a copy of the runner (SI only for MOVS) `snd_init` returns
    3 and the run's memory hash differs. The game only tests bits 0 and 1
    and against 0, so nothing else was seen to change; but any routine
    that keeps a register variable in SI over a REP STOS of 16 or more
    elements ran differently in the runner than on a PC, and the port was
    compared against that.
    **Fixed in doskit** (2026-10-08, `903f6cc` on its branch `doskit/rep-stos`, made
    from doskit's master `6108ae9`; not merged into doskit's master, and
    this project's submodule pointer is not moved to it): the block path
    advances SI for REP MOVS only, with the test `tests/strreg` there
    (STRREG.EXE in the selftest, which failed before the change). With
    that runner a fresh run (`run.py -until 8 -dump snd_devices 2
    HL.EXE`) has `snd_devices` 3, and `check.py` says `all ok` (the
    tools' output is the same; only what the runner computes changed).
    Not done then: the comparisons of the port against the runner were not
    rerun with it, so every memory hash and baseline kept under `build/`
    is still the old runner's, and the notes that give 048Bh as the
    runner's value (`src/HL.hints` at `snd_init`, port/README.md at the
    sound's comparison) describe the runner before this fix.
    **Rerun with the fixed runner** (2026-10-08, branch `mac/rerun`,
    macOS; doskit's master `22292c0` has the fix and the submodule points
    at it, `dosrun` rebuilt; `run.py -until 8 -dump snd_devices 2 HL.EXE`
    gives 3). The original's dumps of `sound_test.sh` (304 calls),
    `menu_test.sh`, `save_test.sh`, `fkeys_test.sh`, `briefing_test.sh`,
    `info_test.sh`, `loop_test.sh` and `play_test.sh m8_site` were made
    anew and the tests run again; port/README.md ("Checked", the sound's
    entry) has each result against the one before. In short: nothing
    differs that did not before; `snd_init` returns 3 in both programs;
    the tests that count the setup's differences count 2 fewer, the two
    bytes of `snd_devices`. Dump against dump of the same place, the fix
    changed in the original's whole memory only `snd_devices`, the word
    of `game_loop`'s frame that keeps `snd_init`'s result (`[BP-2Ah]`,
    tested for bit 1 right after the call) and at `snd_init`'s end two
    words below the stack pointer; the inputs logged are the same. So in
    these runs no routine was seen to have run differently for a register
    kept in SI over a REP STOS; that says nothing of code these runs do
    not reach. The notes that gave 048Bh as the runner's value are
    corrected (`src/HL.hints` at `snd_init`, port/README.md), and the two
    tests that allowed for it no longer do (`sound_test.c` judges
    `snd_init`'s result, `load_test.c` compares `snd_devices` in
    `start`). The old runner's dumps of these tests are kept in
    `build/old-runner/` (ignored) and can go when no one needs them.
    Not rerun here: every comparison whose dumps lie in `build/n2` to
    `build/n6` and `build/computer` (but `load_test.sh`'s one `start`
    call) and the play tests of other key scripts; their dumps are still
    the old runner's.
  - The port (`port/src/sound.c`, port/README.md): `snd_init`,
    `sfx_play`, `dig_play`, `adl_play`, `adl_load`, `adl_start`,
    `adl_stop`, `adl_voices_free`, `chan_stop`, `snd_pass`, `adl_tick`
    with `music_events` and `effect_events`, and the voices' routines of
    the driver, over the program's memory; called where `game_loop`, the
    fight's panel, the menu and the start call them. No sound comes out.
    Compared call by call with the original (304 calls, the driver's
    variables the same) and by which effects the menu and a battle play
    (`port/tests/sound_dumps.py`, `sound_test.sh`; port/README.md
    "Checked" has the numbers and what was not reached). `site_taken`
    (`S331F:4CAA`) is, besides order 4's mark, the number of the effect
    the end of a side's part plays when 100h is in word 4 (0 and 1 were
    seen for certain).
  - What sound output would need (not done):
    - AdLib: the registers are already handed over in the original's
      order (`snd_opl(register, value)`, a hook, unset): an OPL2 (the
      runtime has `opl.c`) fed from it, mixed into the platform's audio
      callback. The original waits between a register's number and its
      value (`delay`); a synthesizer does not need that. The timer's
      routine must run at the tune's rate on the audio's clock, not on
      the port's own clock of passes as now (`snd_clock`), or the tune's
      speed follows the drawing.
    - Sound Blaster: `dig_mix` translated (two bytes a step for each
      playing channel at count * step / 100h, signed, times half the
      volume, summed, divided by the count of channels, plus 7Fh;
      unsigned 8 bit mono) and called for 400h bytes whenever the output
      has played a buffer, at the rate `dsp_rate` set (10000 a second
      from `game_loop`; the DSP's time constant 256 - 1000000/rate, so
      really 1000000/100 = 10000), resampled to the platform's. The
      original plays two buffers in turn by DMA channel 1 and the card's
      interrupt (found among 7, 5, 3, 2; port 210h to 260h); none of that
      is needed. The channels' end (`dig_count`, `dig_mask`) comes from
      the mixer, so until it exists a digital channel in the port stays
      "playing" until `chan_stop` or the next `dig_play`. (Since
      `mac/sfx` the mixer's memory side exists, with `dig_irq` and
      `dig_kick`, and the channels end; the bullet "The effects' ways
      and the mixer" below says what is left for output.)
    - The music: its control is translated (the next bullet). For
      sound it needs the AdLib's part above and the timer's interrupts
      (`snd_irq`) on the audio's clock; `music_fade`'s wait then has to
      wait for that clock instead of running the interrupts itself, and
      `opl_silence` at its end is what makes the tune stop sounding
      (with speed 0 the original only clears `music_on` and leaves the
      voices as they are).
  - The music's control (2026-10-08, Mac, branch `mac/music`).
    `music_start`, `music_fade`, `music_volume`, `fade_tick`,
    `fade_start`, `fade_stop` and `opl_silence` are translated in
    `port/src/sound.c` over the program's memory and compared
    (`port/tests/music_dumps.py`, `music_test.sh`, `music_cmp.py`;
    port/README.md "Checked" has the numbers). What the runs showed:
    - The fade's words in the driver: `S30EF:023A` the steps so far,
      `023C` how many, `023E` the volume it begins with, `0240` the one
      it ends with; `1118` the number `pit_add` gave its routine.
      `fade_tick` sets the volume from + (to - from) * count / steps and
      at the call after the last step takes itself off the timer and
      clears `fade_on`: a fade of n steps is n + 1 calls, a tenth of a
      second apart (1D1EBh timer counts).
    - `pit_irq` calls `fade_tick` before `adl_tick` in the same
      interrupt (the routine given to the timer last is called first),
      and each when its own count has run out: with the menu's tune
      (period 06A9h) 70 calls of `adl_tick` lie between two of
      `fade_tick`. The port has no timer tables; `snd_irq` in `sound.c`
      is one interrupt for the two routines, and the count it gives for
      a fade's wait is the original's in all 11 waits compared (350 for
      4 steps with the menu's and the battle's tune, 280 with the menu's
      other one, 504 and 555 for 5 steps, 3,116 for 20). It assumes
      `adl_tick` has the shortest period of the timer's routines (it had
      in every run) and takes a new period at once, where `pit_irq`
      still counts the interrupt under way with the old one (not seen
      to matter).
    - `music_fade` returns 1 to 3 ticks after its wait is over
      (presumably `opl_silence`'s 20 registers with `opl_write`'s
      waits; not measured). `music_start` copies the count of ticks to
      `S30EF:0232`/`0234` (what reads it was not looked for).
    - Who calls them, by the return addresses in the runs:
      `game_loop` `music_fade(4)` at its start (`S066A:0341` behind the
      call; nothing plays then, FFFFh), `music_start(music_data, 0,
      FFh)` before its loop (`S066A:0ECD`), F2 and the loop's end
      (read, not run); `attacks_run` `music_fade(4)` before its fights
      when the side has orders (`S02E6:00FC`) and at a site taken
      (`S02E6:122D`), `music_start` at its end when one of them came
      (`S02E6:2058`); the menu `music_start(tune, 4, FFh)` and
      `music_fade(4)` (`S167A`); the new units' screens with 5
      (`S274D`); a taken site's picture its own tune and a fade of 20
      (`S1A39:1987`, `19FE`). `S1A39`'s other calls (fades of 10 and 20,
      tunes with fades of 0 and 20) were not run; the port does not
      have them.
    - The port calls `attacks_run`'s three from `loop.c` around its
      call of `attacks_run` (before it, at `orders_hook`'s
      `ORDERS_TAKEN`, after it), because `orders.c` leaves them out and
      was not to be touched on this branch. Whoever owns `orders.c` may
      move them inside; then the wrapper in `loop.c` (`round_fade`,
      `round_music`) goes.
    - The time of a fade's wait: the menu and the new units' screens
      move the port's clock on by it (`snd_waited`); with that the
      menu's times are within 0.07 s of the original's in
      `menu_test.sh`'s seven walks, where they were half a second
      apart. In a battle the wait does not move the port's clock: what
      `play.c` counts for `attacks_run` was measured on runs with the
      music on, presumably with the fade's half second in it (not
      checked), and F2's wait is not counted at all. On the original's
      clock (`-clock`) `snd_clock` is not called in a battle, so a tune
      there only goes on inside a fade's wait.
    - `menu_cmp.py` tells the views' repeat counters (byte 2Ch of a
      view's record) apart when the counts of passes differ: in the walk
      `back` the port's battle before the menu now has 131 passes where
      the original has 130 (140 before the fade's wait was counted), and
      the counters, which go down by the pass, are one apart.
    - Not done: `snd_end` (read, the lock, `fade_stop`, the devices'
      removal; `game_loop`'s end calls it), `S1A39`'s calls, sound
      output.
  - The fight's sounds (2026-10-08, Mac, branch `mac/fightsound`).
    What was left of the sound's triggers in the fight: `panel_sound`
    (`S131D:1FF0`) was already translated and called by `fight_panel`
    (`port/src/draw.c`, since `mac/sound`); the notes that still listed
    it as left out were older. New in `port/src/sound.c`:
    - `pics_play(slot, volume, times)` (`S2532:211E`): the sound
      `pics_sound` loaded for the slot (far at `S3AEE:04B4` + 4 * slot;
      none: nothing happens), by `dig_play(slot, sound, volume, times,
      step)` with the step at `S3AEE:04A4` + 2 * slot, or with
      `S3303:0022` set by `adl_play` on the channel at `S3AEE:011F` + 2
      * slot; then `snd_pass`. A slot is its digital channel: 0 to 2 the
      first side's, 4 to 6 the second's.
    - Its three sites in `fight_frames`, each a routine of its own in
      the port: `pics_volley_sound(side)` behind `volley_end` (slot 4 *
      side + 2, volume FFh, once), `pics_begin_sound(side)` at the first
      frame of a figure with 20h in its record's flags (slot 4 * side +
      1, FFh, once), `pics_side_sound(side, n)` when the byte at
      `S3AEE:04D4` + 7 * side changes: n figures of the side stand, slot
      4 * side, volume (28h * n + 0Fh) >> side, 3E8h times (so the
      sound of a side grows fainter as its figures fall, and the second
      side's has half the volume); with 64h (the side has no figures)
      `chan_stop(side)` and `snd_pass` instead. That `chan_stop` gets
      the side's number, not its slot: for the second side it stops
      channel 1, the first side's slot 1, and leaves slot 4 playing
      (read; no run met a side without figures, so not seen).
    - `pics_sounds_stop` at `fight_pictures`' end: `chan_stop` for the
      channels 0 to 7 (8 to 0Bh with `S3303:0022`).
    - `fight_blast_sound` (`S02E6:0B2B`): in a fight without pictures
      that destroyed a unit, behind the panels and `wait_frames(1Eh)`
      and before the blasts' loop, with effects on `sfx_play(8, 5, DCh,
      1, 200h)` and `snd_pass`. It is the only call of the sound's
      routines in `attacks_run` besides the music's three.
    - How they are called. `fightpics.c` and `orders.c` belong to other
      branches and were not touched, so nothing calls the routines from
      inside the frames yet. Instead `main.c` puts itself before
      `play.c`'s `pics_hook` and `orders_hook` (behind `play_begin`,
      which sets them) and calls `fight_pics_sounds(at)` and
      `fight_orders_sounds(at, n)`. The second is exact: the blasts'
      loop only runs when a unit was destroyed, and the effect comes at
      its first turn's hook. The first finds a frame's sounds behind
      the frame (`PICS_FRAME`) from what the frame left in memory,
      going through the figures' records again with the marks of the
      second side as they were at the frame's beginning and the bytes
      at 04D4h as they were; it says in `sound.c` where that would not
      be the original (a volley in the third list; a record whose last
      frame was moved in the same frame with a negative offset at
      006Dh: neither seen). The sounds of one frame come in the
      original's order, but behind the frame's work instead of inside
      it; they do not read what the frame writes besides the above.
      **For the owners** of `fightpics.c` and `orders.c`, when they
      want the calls inside: in `fight_frames` `pics_volley_sound(side)`
      behind `volley_end()`, `pics_begin_sound(side)` where `fl & 0x20`
      and `frame == start`, `pics_side_sound(side, 0x64)` and
      `pics_side_sound(side, n)` where the byte at 04D4h + 7 * side is
      set; `pics_sounds_stop()` where `fight_pictures` sets 0680h at
      its end; `fight_blast_sound()` before `blasts_run` in
      `fights_run`'s part without pictures. Then `fight_pics_sounds`,
      `fight_orders_sounds` and `main.c`'s `fight_sounds_wire` go.
    - Compared (`port/tests/fsound_dumps.py`, `fsound_test.sh`,
      `fsound_cmp.py`; port/README.md "Checked" has the numbers): which
      trigger comes in which frame with which slot and volume, fight by
      fight, the same in all nine fights the port reached (m8_site's
      seven, m8_d5r's first two, which run all 749 frames), and
      `attacks_run`'s effect in m8_wait without pictures (twice); the
      driver's variables after `pics_play`, the stops and the effect the
      same in 142 calls but for what the mixer writes.
    - What the mixer does there, seen in the dumps: behind a `dig_play`
      `snd_pass` runs `dig_kick`, which fills both buffers at once
      (`S30EF:04A3`, `08AD`, 400h bytes each), moves the channel's count
      of steps on by 400h, leaves `dig_mix`'s sums at `S30EF:10C3` and
      `10C5` and flips `dig_irq`'s flag at `0499`; a channel played
      3E8h times has its count at +4 go down each time it begins again.
      The port has none of that, so its digital channels keep the place
      and the count `dig_play` gave them, and a sound played once stays
      in `dig_mask` and `dig_count` after the original's has ended (not
      seen to differ in the calls compared; at the pictures' end all
      eight are stopped in both).
    - Not verified: the 64h way and the AdLib's way of `pics_play`
      (read only); `pics_play`'s count of times where the call's entry
      was not dumped (known in 51 of m8_site's 56 calls and in 37 of
      the 51 in m8_d5r's two fights, all as read: 3E8h for the slots 0
      and 4, 1 for the others); the
      port's triggers behind the frame a key ended the original's
      pictures at (on the original's clock the port plays all frames);
      m8_d5r's fights 3 to 7 (its clock's file ends before them); the
      slots 5 and 6 (no trigger came for them: the second side's figures
      neither began with 20h nor ended a volley in these fights).
      Nothing was listened to. Built on macOS only (no new source file).
    - Left for sound output: everything under "What sound output would
      need" above; for the fight's pictures in particular `dig_mix`,
      since all their sounds are digital and a channel's end comes from
      the mixer.
  - The effects' ways and the mixer (2026-10-08, Mac, branch `mac/sfx`).
    What `mac/sound` had listed as not reached of `sfx_play`: a call
    that fails, an effect played more than once or endless, an 8SVX
    effect. `sfx_play`, `dig_play`, `adl_play`, `adl_load`, `adl_start`
    and the events' end (77h) were already translated as read; they are
    now compared on those ways and none needed a change. New in
    `port/src/sound.c`: the digital mixer's memory side (`dig_mix`,
    `dig_irq`, `dig_kick`, `dsp_rate`), because a digital effect's
    repeating and ending is the mixer's doing and `snd_pass` behind an
    8SVX effect could not be the original's without it.
    - Where the game calls `sfx_play` with other than "once" or with a
      digital effect: only in the pictures of events (`S1A39`). Its 20
      call sites, all with a constant step of 200h: `attacks_run`
      (`S02E6:0B2B`), `game_loop` (two), `panel_sound`, the menu (four),
      all ADLX on the channels 8 to 0Bh, once, and all called by the
      port since `mac/sound` and `mac/fightsound`; `pic_wiped` (CDh,
      two: `S3303:0022` chooses the file, the table and the channels),
      called by `port/src/events.c` since `debian/window5`; and ten in
      three pictures the port does not have: `S1A39:149D` (CCh in
      `event_picture`'s table), `S1A39:1A11` (C9h) and `S1A39:1DD5`
      (C8h). Read at the call sites only: CCh starts effect 5 of the
      same file 258h times (volume AAh with `S3303:0022`, else 1Eh)
      before its first frame, plays others from a table at `S3812:0764`
      (30h bytes for each value of `S3303:0022`) and ends with
      `chan_stop` for the channels 0 to 0Bh; C9h plays effect 2 FFh and
      C8h times, C8h effect 1 twice on channel 9 and effect 2 C8h times
      on channel 0Bh or 7; both end with `chan_stop`. No call of
      `event_picture` that passes C8h, C9h or CCh as a constant was
      found in the generated source (its four callers pass CAh/CBh,
      CEh/CFh, CDh/D0h and a map's number); whether anything reaches
      them was not looked for in a run. So in what runs, every effect
      is played once; the other ways are in the routines, not in the
      game's own calls seen.
    - `pic_wiped`'s table (`S3812:03C4`, six bytes an entry: the frame,
      how many times, the number, the volume) has 19 entries for the
      digital file, its end's included, and the AdLib's entries follow
      at once; but with `S3303:0022` set the routine looks 7Eh bytes
      further on, which is 21 entries: two past the AdLib's first, so
      its entries of the frames 1 and 5 are never read and the
      picture's first sound comes at frame 10.
      The picture has 82h frames, so the entries of the frames 130 and
      135 never play: 16 calls digital, 14 AdLib.
    - Reached by pokes, since the game's tables only play once
      (`port/tests/sfx_dumps.py`, on `m57_win.keys`, which ends a
      battle at the first round's end with picture CDh): entries of
      the table changed when the first pass begins (times 0, 3, 2, a
      number of FFh), `S3303:0022` set to 1 for the AdLib's way with
      the Sound Blaster still found, one call's channel made 0Ch on
      the stack, and `adl_used` made 9 at one call's entry and at one
      `snd_pass`. Not the game's own doing; what the routines do with
      such arguments is the original's.
    - What the runs showed (all the same in the port):
      - A number beyond the file and a channel of 0Ch give FFFFh and
        change nothing of the driver's variables. The picture goes on
        to its next channel all the same.
      - A digital effect: `dig_play` stops the channel first (FFFFh
        from `chan_stop` when its last effect has ended by itself, 1
        when it still played: an endless one always does). With times
        0 the record's flags are 2 and its count at +4 is left as it
        was.
      - `adl_load` with no voice free returns FFFFh, `adl_play` takes
        that for success, so `sfx_play` returns 1 and nothing plays
        (the record keeps the flags `adl_stop` left). `adl_start`
        with more voices needed than free takes its count back and
        leaves `snd_lock` 1: seen for the first time, and from then on
        every later call found it 1 (`adl_start` and `adl_stop` put
        back what they found), so `adl_tick` only counts its ticks and
        no effect ends any more; later effects still load and start
        while voices are free. Nothing in the picture clears it.
      - An AdLib effect played three times or twice begins again at
        its event 77h (`S30EF:18DA`) and is stopped by the last one
        (`adl_stop` from the timer's routine); an endless one goes on
        until the next `sfx_play` on its channel stops it.
    - The mixer, read whole and translated without output:
      - `dig_mix` (`S30EF:1CED`; DX the buffer, BX its length): for
        each pair of bytes every channel in `dig_mask` (its low byte,
        channels 0 to 7): the place is count * step >> 8 (the count of
        steps at +10h, the step at +26h); at or past the length
        (+0Ah) the channel gives nothing to this pair and either
        begins again (count 0: endless, or its times at +4 counted
        down and still above 0) or is taken off (`dig_mask`,
        `dig_count`); when that was the last channel the routine
        ends at once without writing the pair. Else the two bytes at
        the place, each signed, times half the volume (+1Ch, a byte),
        the product's upper byte added to a sum of its own
        (`S30EF:10C3`, `10C5`), the count moved on by one. Then each
        sum divided by `dig_count` (signed), plus 7Fh, one byte each.
        The pairs left are counted at `S30EF:10BB`. It returns the
        length in bytes less the pairs not made (bytes less pairs, as
        it is written). `snd_busy` is 1 while it works. So step 200h
        plays the file's bytes one for one, two a step.
      - `dig_irq` (`S30EF:1DF8`, the card's interrupt; nothing while
        byte `0496` is set): of the two buffers (headers at
        `S30EF:049B` and `08A5`: the length, the DMA's page and
        address, the buffer's place) the one that waits (byte `0499`
        bit 0: the first) is started when it has 28h bytes or more
        (`dma_start`), `0499` flipped, its length cleared and the
        other one mixed anew with 400h; with less the interrupt is
        masked and nothing more comes until the next `dig_kick`.
      - `dig_kick` (`S30EF:1E8B`, from `snd_pass` while `dig_count` is
        not 0): the DSP halted (D0h), the first buffer mixed, `0499`
        1, the interrupt unmasked and `dig_irq` called: so both
        buffers are mixed in the pass, 800h bytes ahead, and what was
        mixed before and not yet played is thrown away while the
        channels' counts have already moved past it. An effect of up
        to 800h bytes at step 200h is therefore at its end, and off
        `dig_mask`, when the `snd_pass` that began it returns (seen:
        the picture's short effects), and each further `snd_pass`
        skips up to 800h bytes of every channel still playing (read
        from the code and the counts; not listened to).
      - `dsp_rate` keeps nothing in memory; `game_loop` calls it with
        2710h at `S066A:00EB` whatever `snd_init` found.
    - In the port: `snd_pass` calls `dig_kick` as the original does;
      `dig_irq` is public. No bytes leave the buffers. The card's
      interrupt is made by `snd_clock`, the buffer's length / the rate
      (`dsp_rate`'s: 8000 from `snd_init`, 10000 from `battle.c` where
      `game_loop` sets it) after the buffer was started, so on the
      port's own clock digital channels move on and end between the
      passes; on the original's clock (`-clock`) `snd_clock` is not
      called in a battle and only `snd_pass` moves them. That timing
      is the port's own (the original's interrupt comes from the DMA's
      end; not compared in time).
    - Compared (`port/tests/sfx_dumps.py`, `sfx_test.sh`, `sfx_cmp.py`;
      port/README.md "Checked" has the numbers): memory at the entry
      and the end of every `sfx_play` of the picture, of the
      `snd_pass` behind each, of calls of `adl_tick` (those in which
      an effect ended or began again among them) and of `dig_irq`, in
      seven variants: 610 calls, the driver's variables the same in
      all, the mixer's buffers included. The port's program plays the
      picture's effects in the same frames with the same channel,
      number, volume, times and result in the four variants it can
      play, on the original's clock and on its own. With the mixer in,
      the earlier tests no longer leave its bytes out and are still
      without a difference (`sound_test.sh` 304 calls, `fsound_test.sh`
      142, `music_test.sh` 297).
    - That overtakes what the bullet above says of the mixer ("The
      port has none of that"): the port's digital channels now end
      and count down as the original's in the calls compared.
    - Not verified: nothing was listened to (no output); the card's
      interrupt in time (when the original's comes against
      `snd_clock`'s); `dig_irq` while byte `0496` is set and its way
      with less than 28h bytes at the dumps' places was not looked
      for among the 94 calls; a step other than 200h; a digital
      effect of the kind 3 (the VHDR's byte 13h set); an effect that
      is neither 8SVX nor ADLX (FFFFh, read); `adl_load` with a file
      whose chunks are not in order; the pictures C8h, C9h and CCh;
      the fights' pictures on the port's own clock against the
      original's memory (their channels now move with `snd_clock`).
      Built and run on macOS only (no new source file).
    - What sound output still needs (not done), in place of the
      Sound Blaster's part under "What sound output would need" above:
      the bytes are there. Where `dig_irq` starts a buffer
      (`dma_start` in the original) its bytes (the header's length, at
      the header's place in `S30EF`; unsigned, 8 bit, mono, at
      `dsp_rate`'s rate) go to a hook as the AdLib's registers go to
      `snd_opl`; the platform's mixer resamples them to its rate and
      adds the OPL2's and the CD's. `dig_kick` must drop what is
      queued and not yet played (the DSP's halt), or the port sounds
      longer than the original. The next `dig_irq` must come when
      the device has played the buffer, counted in samples of the
      audio's clock and carried out on the game's thread (the
      routines write the program's memory), not from `snd_clock`'s
      own count as now; with the original's clock (`-clock`) there is
      none. Whether the skipping of `dig_kick` is audible, and
      whether a faster machine than the runner's would have sounded
      the same, was not looked into.
  - The AdLib's output (2026-10-08, Mac, branch `mac/opl`). The
    routines translated on `mac/sound` and `mac/music` already handed
    every register to a hook in the original's order (`snd_opl`), but
    that stream had only been counted in the tests, never compared, and
    the start's and the end's writes were left out. Now it is compared
    write for write, and what the comparison needed is translated.
    - Where the original writes the chip (ports 388h and 389h): four
      places of the driver and no other in the program. `opl_write`
      (`S30EF:11E6`, BH the register, BL the value; `delay(1)` between
      the two and `delay(4)` behind), `voice_off` (its own two OUTs,
      `delay(2)` behind) and `voice_note`'s two pairs (A0h + voice,
      `delay(3)`; B0h + voice, `delay(2)`), the last with the interrupts
      off from the first OUT to the fourth. `opl_write` and `voice_off`
      leave them on.
    - New in `port/src/sound.c`:
      - `adl_detect` (`S30EF:1A7D`), called by `snd_init`: nothing
        when the AdLib is marked found (byte `S30EF:111A`); else the
        chip's timer test, the registers 01=00, 04=60, 04=80, 02=FF,
        04=21, 04=60, 04=80 (the original reads the status at 388h
        after the third and the fifth and gives up unless its bits E0h
        are clear and then set; the port takes the chip as there),
        `opl_silence` (01=00, 01=20, then 3Fh to the registers 40h and
        43h + operator of the nine voices: 22 writes), 01=20, and
        `adl_tick` given to the timer with the period 1843h.
      - `adl_remove` (`S30EF:1A59`: the mark cleared, `opl_silence`;
        the timer's part is the machine's) and `snd_end`
        (`S2F39:0092`: the lock set and left set, `fade_stop`, the
        devices removed, `snd_devices` 0, `snd_ended` 1). `snd_init`
        calls `snd_end` first when it ran before and devices are left,
        as the original's code does (read). `game_loop`'s end calls `snd_end` too
        (`S066A`, before `input_remove`); the port's `loop.c` does not
        yet (not to be touched on this branch): its owner adds
        `snd_end()` there.
      - The timer's count for the sound's two routines, as `pit_irq`
        keeps it (`snd_irq`; the next point), in place of the count
        `mac/music` had made up for the fade.
      - Two hooks for whoever makes the interrupts instead of
        `snd_clock`: `snd_wait` (music_fade's wait asks it for the
        next interrupt instead of running them itself) and, for the
        test only, `snd_at` (the routine is at a place where the side
        an interrupt comes on matters).
      - `snd_init` sets byte `S30EF:0495` (the Sound Blaster's probe
        interrupt came), which the comparison showed missing.
    - `pit_irq`'s count, read and now seen in the timer's table in the
      dumps (DATA:0D1D the last routine's index, 0D1F/0D3F the periods,
      0D5F/0D7F what is left, 0DBF a routine's number to its place,
      0E3F the period the timer ran with, 0E47 the shortest, 0E4B the
      one the timer is set to). The table has the BIOS's routine
      (10000h), `input_tick` (3FFFh), `adl_tick` and, while a fade
      runs, `fade_tick` (1D1EBh). At each interrupt every routine's
      count goes down by the period *noted at the interrupt before*
      (0E3F is set from 0E47 before the routines are called), a
      routine at or below 0 is called and gets its period added. So:
      - `adl_tick`'s count never gets above 0 (FFFFF087h after the
        start: 1843h less `input_tick`'s 3FFFh plus 1843h; FFFFDEEDh
        once the menu's tune has set 06A9h) and it is called at every
        interrupt, as long as its period is the table's shortest (a
        tune's period above `input_tick`'s 3FFFh: read, not seen).
      - A period a tune sets in its first tick (event 6) counts for the
        fade only from the interrupt after the next. A fade begun with
        the menu's first tune therefore has its first step at the 60th
        interrupt behind `fade_start` (4 at 1843h, then 56 at 06A9h),
        not at the 63rd as the port had it; the later steps are 70
        apart in both.
      - An interrupt that comes while `pit_add` is still at work for
        `fade_start` (before `S27FE:07DF` makes the routine the
        table's last) does not count for the fade. Seen once (the
        menu's third tune): the fade's steps then come two ticks later
        than with the interrupt a few instructions on.
    - What the runs showed besides:
      - The tune's first tick writes 120 registers (the voices'
        instruments) and takes 11.9 ms in the runner,
        with the interrupt's end (the EOI) behind it: the timer's
        periods that run out meanwhile give no interrupt, at most one
        is caught up (the next tick then follows 0.05 ms behind). In
        the menu's walk 46 ticks took longer than a period, in the new
        units' 16. A tick without events takes 5 microseconds.
      - `delay`'s unit is about 20 microseconds in the runner
        (`delay_measure` counts a loop over one BIOS tick): 21 from a
        register's number to its value, 77 from a value to the next
        number in `opl_write`, so about 0.1 ms a register and 2.1 ms
        for `opl_silence`; that is why `music_fade` returns 1 to 3
        ticks behind its wait ("presumably" above: now measured).
      - An interrupt can come between a register's number and its
        value in `opl_write` and `voice_off`. Seen three times (once
        in the menu's walk, twice in the new units'), each with the
        lock set, so the tick did nothing and the value went where it
        belonged. Not seen, but possible by the code: `music_volume`
        from the game's side (`voice_level`, no lock) cut by a tick
        that writes, whose value would go to the register the tick
        chose last. `opl_dumps.py` counts such writes: 0 in all walks.
    - Compared (`port/tests/opl_dumps.py`, `opl_test.c`,
      `opl_test.sh`; port/README.md "Checked" has the numbers). One
      run of the original with a log of every OUT to the two ports,
      every timer interrupt with the routines it ran, and the entry
      and end of every call of the sound's routines by the game; a
      dump at the entry of each call that has arguments and before
      every 4000th tick. The test makes the same calls and interrupts
      again in that order on one memory, from `snd_init`'s entry to
      the walk's end, an interrupt by the port's `snd_irq`; every
      register the port writes must be the original's next, the
      interrupt must run the routines the original's ran, and at each
      dump the driver's variables and the timer's counts must be the
      original's. Three walks, none with a difference: the menu
      (66 s: 3,303 registers, 41,570 ticks, 30 steps of a fade, 84
      calls, 56 dumps), the new units' screens of map 4 and the
      battle's beginning (60 s: 1,959 registers, 42,982 ticks, 326
      calls, 30 dumps) and the first 180 s of map 8 with
      `m8_site.keys` (11,443 registers, 115,956 ticks, 3,128 calls
      with 323 effects, 16 `chan_stop`, 2 `dig_play`, three tunes
      started and three faded out; 376 dumps). 958 to 1,446
      interrupts a walk came while a routine of the game's side ran;
      the test's header says how it places them. A copy of `sound.c`
      with one bit of a note changed gave 564 different registers in
      the menu's walk, so the test does see the stream. The earlier
      tests are as before with the new `snd_init` and `snd_irq`
      (`sound_test.sh` 304 calls, `music_test.sh`, `sfx_test.sh`,
      `fsound_test.sh`: none different).
    - In the port's program: `main.c` sets `snd_opl` to a routine
      that feeds an OPL2 of the runtime (`doskit/runtime/opl.c`, now
      in both build scripts) and with `-opllog FILE` writes each
      register with the port's time and the count of ticks. Nobody
      asks the chip for samples: **no sound comes out, and nothing was
      listened to.** The music and the effects reach it through the
      routines' own calls, which were wired before (`start_setup`,
      `game_loop`, the menu, the panel, the pictures, `loop.c`'s
      wrappers), so nothing else had to be wired. On its own clock the
      program does not write the original's stream as a whole: its
      calls fall on other ticks (`opl_cmp.py` tells how much agrees,
      tune by tune; not judged).
    - Not verified: nothing was listened to; `snd_end` at
      `game_loop`'s end and inside a second `snd_init`, with
      `adl_remove` (translated as read: no walk reaches either, the
      original runs `snd_init` once in each); the
      chip's status as `adl_detect` reads it (the port has no chip
      to ask) and its way out when the test fails (`S30EF:1B28`, not
      read to its end); `music_restore`'s path in `adl_tick` (left
      out as before); an effect's event 6 (its own speed against the
      tune's) and the 7xh events other than 77h, if the walks' files
      have none (not looked for); a tune's period above 3FFFh;
      `pit_least`'s assumption that `input_tick` is always in the
      timer's table (after a second `snd_init` `adl_remove`'s
      `pit_remove` and `adl_detect`'s `pit_install` may leave the
      table without it: read, not looked at in a dump); the timing of
      the port's own clock against the original's interrupts, as
      before. Built and run on macOS only; `port/build.bat` got
      `opl.c` added unbuilt.
    - **What the audio's clock needs** (not done; in place of "AdLib"
      and "The music" under "What sound output would need" above).
      The runtime has what makes the sound: `opl.c` (`opl_write`,
      `opl_render`, mono, any rate; it says itself that timers are the
      port's business), an audio callback for a buffer of frames
      (`platform.h`, `plat_audio_start`), a lock, a clock in
      microseconds. It has nothing that says *when*, and that is the
      whole of what is missing:
      1. The timer's interrupts counted in samples played, not in
         passes. An interrupt is due every N / 1193182 s, N the
         period `pit_least` gives (set anew at each interrupt:
         06A9h is 63.02 samples at 44.1 kHz, so the fraction has to
         be carried). Either the audio callback cuts its buffer at
         those moments and makes the interrupt there, or the game's
         thread makes the interrupts and stamps what they write.
      2. If the callback makes them (the original's shape: the audio
         thread is the interrupt): `snd_irq` writes the program's
         memory, which the game's thread reads and writes too, so
         every routine of the sound called from the game's side must
         hold `plat_audio_lock`; the driver's own lock (`snd_lock`)
         stays what it is, the tick that finds it set only counts.
         `music_fade` then waits through `snd_wait` (sleeping while
         the callback runs the fade), and a call's effect is heard
         from the next buffer on.
      3. If the game's thread makes them (the memory stays on one
         thread, and the headless comparisons stay as they are):
         doskit needs a queue of register writes with the sample they
         belong to, which the callback empties into `opl_write` as
         its rendering gets there (not in the kit; generic, with a
         test there), and the count of samples played so far to run
         `snd_clock` by. The writes are stamped with the interrupt's
         own time plus a lead, and the game's thread must call
         `snd_clock` in every wait (`frame_set_tick` does once a
         picture, 14 ms), or the tune stops while the program loads
         a file; the lead (two pictures or so) is the delay of every
         sound.
      4. Headless there is no audio (`plat_null.c`'s
         `plat_audio_start` returns 0), so the tests would go on with
         the clock of passes while the window uses another one. A
         virtual audio clock in `plat_null.c` (the callback asked for
         the samples of the time that has passed, the samples
         optionally written to a file) would let both run the same
         way and make the output comparable with the runner's
         `-oplwav`: also the kit's.
      5. What to do about the time the original spends inside a tick
         (0.1 ms a register, up to 11.9 ms, with timer periods lost):
         a port on the sample clock writes a tick's registers at one
         sample and loses none, so a tune runs ahead of the
         original's by about 10 ms at each start. To keep it, the
         clock would have to move the next interrupt by 98
         microseconds a register written. A decision, not a need of
         the chip's (`opl.c` takes writes at once).
      6. The Sound Blaster's bytes on the same clock (the bullet
         above): a source of unsigned 8 bit mono at `dsp_rate`'s
         rate resampled into the callback, `dig_irq` when its buffer
         has been played, the queue dropped at `dig_kick`. The kit
         has no such source besides `modplay.c` and `cdaudio.c`.
      Of the two ways the third point's is the one recommended here:
      it needs two small generic pieces in doskit (the stamped
      queue, the headless clock) and no lock in the game's code.
  - The AdLib played (2026-10-08, Mac, branch `mac/soundout`). The
    third point's way above, with the two pieces doskit has since
    `9aa2c77` (`oplq.c`; `plat_null.c`'s virtual device, `DK_AUDIO`,
    `DK_WAV`). port/README.md has what the program does and the
    numbers ("State", "Checked"); here what was learned and decided.
    - Where the pieces are. `sound.c` counts: `snd_samples` (a hook;
      unset, everything is as before, which is how every earlier test
      still runs) gives the samples played, `snd_clock` makes the
      interrupts due by them (`irq_due`: the periods' sum in PIT
      counts times the rate / 1193182, divided whole) and leaves the
      sample of what is being written in `snd_stamp`. `main.c` plays:
      the device, the queue, the lead (`AUDIO_LEAD`, 1234 samples),
      the callback. `play.c` was not touched: it calls `snd_clock`
      with the port's time as before, at every call of its timer
      routine (13.7 ms) and at the end of each wait.
    - The scripted headless play never sleeps, and the virtual device
      only moves in `plat_sleep_ms`. So `main.c`'s `snd_samples` moves
      the device to the port's clock (the time `snd_clock` was given)
      before it answers, in whole milliseconds since that is what the
      platform sleeps in; where the port's clock goes back (a new
      clock begins) the device goes on from where it stands. In a
      live play the pictures' waits move it and `main.c` does not.
      Both ways were rendered twice with the same result.
    - The lead is two pictures. In the scripted runs the interrupts
      are made at most 14.5 ms behind their samples (one call of the
      timer routine and the millisecond), so nothing was late with 28
      ms; with a real device the callback's buffer adds to that and
      the lead may have to grow (not tried: the count of late writes
      at the program's end says).
    - `music_fade` was left making its interrupts at once (no
      `snd_wait`): each has its own sample ahead of the samples
      played, the queue keeps the writes (the menu's walk has 223
      such writes in all its fades, far below the queue's 4096 at a
      time), and the caller's
      clock goes on by the wait through `snd_waited` as before, after
      which the samples have caught up. A caller without `snd_waited`
      leaves the sound's clock ahead by the wait; the next tune's
      first writes then come at the fade's end and its ticks follow
      once the samples are there (by the code; no walk has it).
    - A write of the game's side is stamped with the samples played
      at that moment, or with the last interrupt's sample when a fade
      ran ahead. The start's writes (`adl_detect` in `snd_init`,
      before any clock) are at sample 0 plus the lead.
    - With the device the interrupts fall at other moments between
      the game's calls than on the clock of passes (the millisecond),
      so the register stream of a battle is not write for write the
      one without a device: in 120 s of map 8 the same 10,231
      registers, 70 of them at another place. The menu's walk gave
      the same order. Neither is the original's order of calls and
      ticks anyway (`opl_cmp.py`); `opl_test.sh` is what compares the
      routines with the original, and it does not use a device.
    - Decided, as METHOD.md says for the kit: the time the original
      spends inside a tick is not modelled, a tick's registers fall on
      one sample (point 5 above).
    - Not verified: **nothing was listened to**; the window's program
      with a real audio device and its thread (built, not run); the
      rendering against one of the original's stream (the runner's
      `-oplwav` on the same walk would give a file to set beside it:
      the tunes' starts differ in time, so tune by tune); the chip's
      level against the digital effects', which are not played; a
      queue overrun (`early`) and late writes, which no run had;
      Windows and Linux builds.
    - Left for output: the Sound Blaster's bytes (point 6 above and
      the bullet "The effects' ways and the mixer"), mixed into
      `main.c`'s callback beside the queue's samples, with `dig_irq`
      on the samples played instead of `snd_clock`'s time; then
      listening.

  - The AdLib beside the original's (2026-10-09, Mac, branch
    `mac/oplwav`). port/README.md has the tests and the numbers
    ("Checked"); here what was learned.
    - The runner's `-oplwav` file can be had back from its log sample
      for sample (`oplwav_ref fit`), although the log's moments are
      whole microseconds: a write near a sample's edge is tried on both
      samples, looking ahead through the next writes. So the list of a
      run and its file say the same, on both sides, and the comparison
      can be made on the lists and the levels.
    - A tune's writes are compared by ticks from its first tick, not by
      time and not from `music_start`'s end: the port's count of ticks
      (`-opllog`) and the count of the original's `adl_tick` calls agree
      there. `fade_tick`'s writes are logged under the count before its
      interrupt's tick in both; its steps count from the game's call,
      so they can be a tick or two off where the game's side called at
      another moment.
    - The original loses timer interrupts, the port none. In a tune's
      first tick `adl_tick` writes all instruments (116 to 120
      registers, `opl_write`'s delays: 11.6 to 11.9 ms at 06A9h or
      0854h); the interrupt is taken once when it ends, so the tune is
      about 7 periods (10 ms) behind the timer from its second tick on. That
      is the driver's own doing and presumably the same on any machine
      (the delays are measured, `delay_measure`; not checked). In the
      fights the original falls further behind, by 98 periods in 119 s
      of map 8, behind ticks that wrote nothing: something keeps the
      interrupt waiting more than two periods there (the game's side
      or another routine; not looked for, and it is the runner's
      machine). Not modelled in the port, as decided on `mac/soundout`;
      to model the first, `irq_audio` would have to count a tick's
      registers.
    - A tick's registers on one sample change the sound measurably: up
      to about 1 dB in half a second against the original's own stream
      spread over the tick (voices keyed in one tick start in step in
      the port, a tenth of a millisecond apart and more in the
      original). The chip's rate does not (0.07 dB). Whether it is
      heard was not tried.
    - `-map N` is a start the original does not have: the driver is
      untouched when the battle's tune starts without a fade, and
      `voice_level` then works with the voices' own volumes 0 (bytes
      `VOICE + 0Eh`), so the tune's voices are at 3Fh until their first
      volume event. In the original's memory at that `music_start` the
      nine bytes are what the menu's tune left (seen in the dump of
      `opl_dumps.py battle`). Through the menu the port does the same
      (the walk `brief`). Left as it is; a direct start that sounds
      like the game's would have to set those bytes.
    - The 70 registers: the device's clock goes in milliseconds
      (`plat_sleep_ms`), the port's does not; `-oplcount` counts the
      interrupts as the device does on a clock of any step and shows
      the order without a device from 100 microseconds down. A tick's
      writes have their own sample either way; what moves is on which
      side of a call of the game's the tick runs.
    - Not verified: **nothing was listened to**; no sample-for-sample
      comparison of the two files; the battle's sound (its tune differs
      from the first tick, see `-map`); what holds the interrupt in the
      fights; a real machine and a real chip.

  - The Sound Blaster played, and the drums (2026-10-09, Debian, branch
    `debian/sound`, in Mac's place; Mac reviews). port/README.md
    ("State") says what the program does.
    - Point 6 of "What the audio's clock needs" is done in the port:
      `snd_dig` behind `dig_irq`'s start of a buffer and at
      `dig_kick`'s halt, a queue in `main.c` added in the callback. The
      kit still has no source for such bytes (the queue is written in
      `main.c`, port-specific glue, not a copy of a kit piece). A
      generic, tested "PCM queue at a rate, resampled into the callback"
      in doskit would replace it; a gap for the lead, not built here.
    - Map 8 with `m8_site.keys`, 180 s, with `DK_WAV`: 96,256 bytes of
      the card (9.6 s at 10000 a second), 9 halts. The intro (`-intro`,
      `-until 120`): 161,356 bytes, 48 halts. Against master's program
      on the same run the WAVE files differ in 577,743 samples, all in
      the seconds 8 to 23 (the BANG scene, before the tune starts at its
      frame 87h). Not listened to.
    - The intro's effects: with the Sound Blaster found, `S033D` loads
      BANG.SFX (the name at `S0E07:0260`) and plays it on the channels 0
      to 7 by the table at `S0E07:036E`; without it BANG.AFX on 8 to 0Bh
      (`seq.c`, as translated before; not compared with the original's
      run here). EXTRO has no effects (its strings name only the two
      .ADL files, and its folder has no .SFX or .AFX).
    - The drums. The tunes do not use the chip's rhythm mode (no write to
      BDh in `soundout_test.sh`'s two walks), so the drums are melodic
      voices. The noisy ones presumably use the highest feedback (C?h =
      0Eh, feedback 7, set on the voices 3, 5, 6 and 7 in those walks;
      which instrument is a drum was not checked). Findings:
      - The suites cannot see the difference from DOSBox: the runner's
        `-oplwav` renders with the same `doskit/runtime/opl.c`
        (`tools/run/build.sh`), so oplwav and soundout compare one
        synthesizer with itself. The register stream was already judged
        the original's write for write in the tunes the game side does
        not touch (port/README.md "Checked", `mac/oplwav`). The 35
        differing writes are in the battle's tune from `-map 8` only
        (the direct start's levels and three notes), not in a game
        entered through the menu, so they cannot make the drums of
        every tune duller. Not changed.
      - `opl.c`'s feedback noise depends on the rate it renders at. A
        voice alone, feedback 7, at 44100 Hz makes 19,867 zero
        crossings a second, at 49716 Hz (the chip's) 22,155: 11 % more,
        the ratio of the rates (feedback 5 and 6 alike; a scratch
        program on `opl_init`/`opl_write`/`opl_render`, not kept). A
        chip makes its noise at 49716 Hz whatever the card's output, so
        at the port's 44100 every noisy instrument is about 11 % lower
        in pitch: presumably the duller snares. Also by its own header
        `opl.c` is no circuit model (a float sine, no log-sin tables,
        no 9-bit envelope steps; the attack's curve and the carrier's
        modulation depth are choices), which changes the noise's colour
        too; not measured.
      - For the lead (kit gap, not built): `opl.c` should step at the
        chip's 49716 Hz and resample to the device's rate (or a
        chip-exact core taken into `third_party/` under a permissive
        licence), with a test against a reference. Then the port's
        sound and the runner's `-oplwav` would show it. Not done here:
        doskit is read-only on this branch.
  - `opl.c` against DOSBox, and the drums (2026-10-09, Debian, branch
    `debian/oplmodel`). Negative result: neither the rate nor the model's
    three guesses make the drums duller. No kit change was made.
    - The rate (the last entry's kit gap). Tried and dropped, not
      committed: `opl.c` stepping at 49716 Hz and resampling linearly to
      44100. The "11 % fewer zero crossings" were mostly bandwidth: a
      44100 Hz render has nothing above 22 kHz. Through the same 5 kHz
      low pass, a feedback-7 voice changes sign 5062 times a second at
      44100 (the old code) and 5054 at 49716. Power per octave from
      250 Hz to 22 kHz: the old 44100 render is as close to the 49716
      one as the resampled one, which is 2-4 dB lower above 4 kHz
      (linear interpolation). The rate does not lower the noise's pitch.
    - The reference: DOSBox 0.74-3 (Debian's package, unpacked in a
      scratch folder, `oplemu=default`, `oplrate`/`rate` 49716, SB2,
      recorded with SDL's `disk` audio driver), used as a black box
      only. No DOSBox source was read or used; nothing of it is in the
      repository. It played a scratch `.COM` (not kept) that replays the
      menu walk's register writes (`build/oplwav/menu.orig` from
      `oplwav_dumps.py menu`, its first 47 s: 2171 writes, the first
      tune) at their times, by polling the PIT's channel 0. The same
      writes went through `opl.c` at 49716 Hz. The two recordings were
      aligned by frame energy (offset 0.13 s), 46.9 s compared. Also
      each channel alone (the others' key bits cleared). Channels 1 and
      7 are silent in that stretch.
    - Whole tune, power per octave relative to the total, `opl.c` minus
      DOSBox, 62 Hz .. 16 kHz: -0.3 -0.1 0.0 0.3 -0.3 -0.1 -0.2 -0.3
      -1.5 dB. Each channel's level against the mix within 0.4 dB of
      DOSBox's, except channel 6 at -1.2 dB. Above 1 kHz the mix is
      0.45 dB rms off. Small differences sit in quiet voices: channel 3
      (-17 dB of the mix) is 23-27 dB short from 4 kHz up, channel 0
      6-12 dB at 8-16 kHz.
    - Zero crossings: `opl.c`'s output has a DC offset (the mix's mean
      +437, DOSBox's +928 at a 2.1 times higher level), so crossings
      are counted about the mean. Whole tune: 0.94 of DOSBox's.
    - The guesses one by one; numbers are the rms of the band
      differences from 1 kHz up (lower is closer), whole tune first:
      - Modulation depth (the modulator's peak moves the carrier by 4
        cycles; tried 2, 4, 6, 8, 12, 16). 4 fits best on channels 0,
        2 and 5 (channel 5: 1.28 at 4, 4.09 at 6, 6.21 at 2). No depth
        closes channel 3's gap. Leave it.
      - Feedback scaling (tried 0.5x, 1x, 2x the data sheet's table).
        Mix 1.01 / 0.45 / 0.63; channel 6, the feedback-7 voice, 1.72 /
        0.46 / 1.78. As it is fits best. Leave it.
      - The float sine against the chip's resolutions. Tried 10-bit
        phase, integer operator outputs (1/4096) and envelope
        attenuation in steps of 0.1875 dB. Each is a choice, not taken
        from a data sheet. Mix 0.45 -> 0.24. Phase helps channel 0 (6.03
        -> 2.13), the envelope steps channel 3 (19.85 -> 9.33),
        channel 8 gets worse (1.85 -> 2.37). Coarser steps fit
        channel 3 better and channels 5 and 8 worse: a fitted
        parameter, not a mechanism. In the mix it is a fraction of a dB
        (not listened to); not taken into the kit.
    - So against DOSBox, `opl.c` plays this tune within half a dB in
      every octave. If the drums sound weaker than in DOSBox, the cause
      is presumably outside the synthesizer: the port's output chain
      (levels, the SB mix, filtering), the 35 differing writes of the
      battle tune, or which tune and voice were heard. Not checked:
      the brief and battle tunes, the rhythm mode (no tune here uses
      it), DOSBox's other `oplemu` settings, and no listening test was
      done.
  - The snare (2026-10-09, Debian, branch `debian/snare`). The user
    heard the snare drums clearly weaker in the port than in the
    original. Negative result: from the register stream to the output
    stage, nothing in the port loses the snare against two reference
    emulators by more than they differ from each other. No code change.
    - Which voice. The battle's tune (`oplwav_dumps.py battle`, the
      tune from 31.1 to 150.5 s of `build/oplwav/battle.orig`, no write
      to BDh: no rhythm mode) has two noisy voices at feedback 7:
      channel 5 (161 key-ons; 20h-F5h modulator/carrier 30/10 00/D1
      A8/D6 4C/4F waves 2/3, C0h 0Eh, block 2, F-number 432) and
      channel 3 (92). Channel 5 is the snare: it has the envelope of
      channel 6 (a feedback-0 sine at the same note, the kick, 114
      key-ons), plays the off-beats and the 16th rolls (73.74, 73.89,
      74.04, 74.19 s) and always comes with channel 4. 58 of its hits
      are retriggers: key-off and key-on in the same tick.
    - The registers. The port's `-map 8` run (`-opllog`, 120 s) gives
      channel 5 the same 11 instrument bytes at every key-on (165 in
      120 s against the original's 161; ticks, not values), and every
      other voice too. So the 35 differing writes of `mac/oplwav` (the
      direct start's first tick and three notes) never reach a key-on:
      not a suspect for the snare.
    - The references, black boxes only (renders, no source): DOSBox
      0.74-3 (`oplemu=default`) and DOSBox-X 2025.02.01 (Debian's
      packages unpacked under `build/`, `oplemu=nuked`, both `oplrate`
      49716, SB2), each playing the tune's writes (31.0 to 150.5 s,
      10,206 writes; the writes before 31.0 s dropped but 01h, 08h and
      BDh's) from the replay `.COM` of the last entry, once whole and
      once with every other voice's key bit cleared; `opl.c` the same
      list at 49716 Hz.
    - Level. Each voice against the mix (rms, DOSBox): `opl.c` within
      0.5 dB on all nine; the snare -18.30 dB against -17.82 (DOSBox)
      and -17.78 (Nuked). Hit by hit (each of the 161 aligned on its
      own; DOSBox's replay drifts 135 ms over the tune), the first 15 ms
      after the mix's gain: peak +0.06 dB, rms -0.17 dB (-0.85 to
      +0.99), 15-90 ms +0.42 dB.
    - Spectrum (the snare alone, per octave against its own total,
      `opl.c` minus Nuked, 31 Hz .. 16 kHz): +1.6 +4.9 +0.8 +0.8 +0.7
      -0.2 -1.1 -1.1 -1.6 -1.2 dB; rms from 125 Hz 1.01 dB (DOSBox
      1.55). DOSBox minus Nuked differ by up to 2.2 dB in a band. So
      `opl.c`'s snare is 1-1.6 dB darker above 2 kHz and has more at
      62 Hz, where the window's 38 Hz DC blocker takes most of it.
      Tried on a scratch copy: feedback 0.5x/2x and modulation depth
      2/8 are worse; the chip's 10-bit phase (the last entry's guess)
      brings it to 0.60 (Nuked) and 1.11 (DOSBox), the 62 Hz excess to
      +0.5. A small gain inside the references' spread, and the last
      entry found that guess worse on other voices: not taken.
    - The chain after the chip: the voice volume, `music_volume` and the
      fade are register writes (the same as the original's, above); the
      Sound Blaster's bytes are only added (`dig_add`); `audiofx` at 0
      dB is flat but for its DC blocker, which moves the snare against
      the mix by less than 0.7 dB (rms, 0 to 38 Hz tried); the volume is
      one gain; the chip renders at 44100 (the last entry: the rate
      does not change the noise); SDL converts to the device's rate.
    - Told, not judged: the port's whole output is 6.25 dB below
      DOSBox 0.74's and 3.71 dB below DOSBox-X's (the references
      disagree on the level by 2.5 dB). Since the card's bytes go in at
      full scale in both, the music is that much quieter against the
      effects than in those emulators. Not snare-specific.
    - For the user's ear: `build/snare/listen/` has the tune and the
      snare alone from the three (`opl.c` raised by 6.25 dB to DOSBox's
      level), 49716 Hz mono. Not kept in the repository.
    - Not checked: what the user heard the original on (emulator,
      `oplemu`, card) and the port's window build by ear; a real
      OPL2; the menu and briefing tunes' drums against Nuked.

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

### Intro and outro translated (2026-10-09, Windows, branch `win/intro`)

- INTRO's module is `S033D` (data `S0E07`), EXTRO's `S0340` (data
  `S0D22`); everything else is HL.EXE's libraries. Matching each far
  call's first bytes (relocated words masked) against HL.EXE found every
  library routine they call; INTRO's data offsets in the driver are
  HL's + 0Ah (S071F:0236 = S30EF:022C), its DATA's near the end + 0Ch
  (D0AF1 = HL D0AFD). Named in src/INTRO.hints and src/EXTRO.hints.
- The flag word: INTRO 3F7h always (its argument only prints a credit
  for "?"; bit 8 never set), EXTRO 1Fh, and EXTRO needs its second
  argument "HL14-18" (strupr'd), which explains the earlier tail probe.
- Keys: the modules' wait takes a key with kbhit/getch (DOS 0Bh), only
  Esc counts; the timer routine (S02AF:00A0 / S27FE:00A0) takes BIOS
  keys with int 16h too, so an Esc that comes while the module computes
  is lost. Runner: INTRO Esc at 5, 20.05, 30, 45, 200 s ends it ~0.7 s
  later, at 20.00 and 70 s nothing; EXTRO at 100 s nothing. This is why
  the earlier EXTRO runs with Esc at 60/300/400 s ran to the end.
- S2F39:11AC is `music_seek(from, to)` and S2F39:1A0B `music_beat`;
  music_restore is reached (INTRO's countries, EXTRO's two tunes).
- The runner's original computes a lot between waits (its waits count
  from 0 after the work): see port/README.md for the measured costs;
  EXTRO's text rows cost 41 ms each, which is where its 160 s over the
  bare waits come from.
- The launcher seam, for the lead (not built): HL14-18.EXE runs INTRO
  (CODE:0054) before HL.EXE, and EXTRO (CODE:00D5, tail "0 HL14-18")
  only when HL.EXE exits with 7Bh (CODE:009A). In the port: call
  `intro_play()` after HL.EXE's image is loaded (the libraries' data is
  in it) and before `start_setup`, then load HL.EXE's image again, so
  HL starts on fresh memory as a new process would (main.c's `seq_game`
  shows the call); `extro_play()` where `menu_game` sees "the last
  battle is won" (`play_after` returned 7Bh). Memory: with HL.EXE loaded
  DOS has 6000h paragraphs, less than INTRO's 72038h bytes; `big_alloc`
  takes the largest block, enough for every scene (THEEND's 365 KB
  stream fits).
- The seam is built (win/seqhook): `start_game` plays INTRO on HL.EXE's
  image and loads it again before `start_setup`; `menu_game` plays EXTRO
  where it sees the last battle won. Only with a window and no `-map`;
  `-nointro` drops the intro. `play_start` clears `stopped`, so a window
  closed in the intro is looked at before the menu's `play_start`; the
  outro gets `play_start(play_now())` because the battle's clock mode
  is still on there. Not run live here: whether a key from the intro
  reaches the menu, and the outro after a real campaign's end.
- Comparison tooling used (build/seq, not committed): logs of
  `S09B5:000C`/`S033D:1A47` (INTRO) and `S0514:000A` (EXTRO), the port's
  `-seqlog`, breakpoints as `INTRO.EXE+09B5:000C#N` (run.py passes
  `S09B5:000C#N` through untranslated: a linear address that never
  hits), `-vram`/`-dac` against `-seqdump`.

## Next

- Intro/outro: the sound of both
  checked against the original's OPL stream; the player's live check.

0. The sound (the bullet "The sound" above): the runner's REP STOS is
   fixed and the sound's, the menu's and the battle's comparisons named
   there are rerun; left of that are the comparisons with dumps in
   `build/n2` to `build/n6` and `build/computer`, which their owners
   rerun. The music's control is done (`mac/music`; the bullet
   "The music's control"), and so are the fight's sound triggers
   (`mac/fightsound`; the bullet "The fight's sounds"), driven from the
   hooks. Then their calls put inside `orders.c` and `fightpics.c` by
   the owners (the list is in that bullet), the music's three calls of
   `attacks_run` moved from `loop.c` into `orders.c` by its owner,
   `snd_end()` at the end of `game_loop` in `loop.c` by its owner
   (translated on `mac/opl`, not called),
   `S1A39`'s screens with their tunes (the pictures of `events.c`
   leave `music_start` and `music_fade` out but for the site's and the
   newspapers'; the pictures C8h, C9h and CCh with their effects are
   translated on `win/papers`, called by the newspapers), and the
   output itself (the OPL2 is fed since `mac/opl` and
   its stream compared, and played on the audio's clock since
   `mac/soundout`, rendered headless and not listened to: the bullet
   "The AdLib played"; set beside the runner's `-oplwav` on
   `mac/oplwav`: "The AdLib beside the original's"; then
   the mixer's bytes, which `dig_mix` now makes in the driver's buffers
   (`mac/sfx`; the bullet "The effects' ways and the mixer" says what
   output still needs) with the timer's interrupts (`snd_irq`) and the
   card's (`dig_irq`) on the audio's clock).
1. For `HL14-18.EXE`, keep the remaining C0015 gaps and vector handlers
   unclassified until an incoming call or table target is evidenced. Do not
   repeat ordinary launcher runs for these gaps. The dispatcher and
   conditional EXTRO path are statically mapped; the likely win trigger for
   status 007Bh remains unverified.
2. `HL.EXE`: 71,755 instructions decode and play finds no more code. Go on
   with stage 2 where it stands (the bullets of 2026-10-04 above):
   - the computer's player (`win/computer`: the first side compared on
     the maps 24 and 30; `win/computer2`: both sides, `loop_test` on the
     same maps): next both sides in `play_test.sh`; the paths no map
     reached are compared with pokes or shown unreachable
     (`win/computer5`);
   - a battle's end in the window (`debian/window5`): the pictures CDh
     to D0h are translated and compared; next there the music they
      start (the sound's owner); `event_picture`'s other kinds (C8h,
      C9h, CCh, the newspapers of the maps 0 to 2Fh, `S1A39:059D`) are
      translated on `win/papers` (the bullet "The newspapers"); the
      end on the port's own clock by play is done (`debian/window14`,
      its three edges 15h, CEh after a win by fights and a score not
      above the best on `debian/window15`,
     `m54_hq`, `m57_hq`) (the end in the window from the menu is
     done, `debian/window8`);
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
     and compared (2026-10-08), so is `pictures_load` (2026-10-08); next
     toward drawing is the screen's set-up at the start (the list under
     "The drawing a battle screen needs"). `attacks_run` around them waits for
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

## debian/draw-tidy (2026-10-09)

- `view_cover` (`S131D:1426`) moved from orders.c to draw.c beside
  `view_dim`, its body unchanged, declared in hl.h.
- win/fightpanel's `fight_panel` waits were read against the listing:
  `fade_in` at 1AC8 only with pictures, `wait_frames(6)` after each
  step's `page_flip`/`page_copy` in both loops, `wait_frames(0Ch)` after
  the first loop on both paths, then `wait_input(96h)` and `fade_out`
  with pictures, `wait_input(46h)` at the end without. The port's
  `orders_hook` calls stand at those places (wait_frames(n) sent as n+1
  retraces, as elsewhere in round_hook); nothing was changed. Not
  checked: that `T_PANEL` and `T_MARK` match the original's times
  (taken over from win/fightpanel as they are).

## win/launcher: the setup screen (2026-10-09, Windows)

- The port now runs doskit's launcher before the game in the window
  (`setup_run` in main.c): pages Game (intro/outro), Sound (volume,
  headphone mix), Display (full screen); settings in
  `history-line.cfg` in the data folder. Only pages and items are the
  port's; nothing of launcher.c was copied or overridden, and the kit
  needed no change.
- The original has no setup or config of its own to compare with: the
  game folder holds no settings file of the game (only GOG's
  `goglog.ini`, `unins000.ini`), HL14-18.EXE takes no options. Side and
  difficulty are the game's menu's, in its memory; offering defaults for
  them would mean writing that memory before the menu, left out.
- Next for the setup screen: a Keys page and a Controller page
  (PLAYER-SETTINGS.md) need the game's key table read from the INT 9
  handler (`key_install`, S27FE:056C) first; and whether the game uses
  the keypad's + - * (taken by the volume hud) is not checked.

## win/titlesetup: title screens, keys, controller (2026-10-09, Windows)

- The title screen the player saw before the menu is HL.EXE's, not the
  menu's (S167A) and not TITEL.TXT or GAME.IFF: S1650:0003
  (`start_setup`) ends with BB.DAT/BB.IFF (S28A5:000C, `title_show`, an
  animation of 43h frames by the table at S28A5:01AD), a wait, HLLOGO.IFF
  faded in, a wait, a fade_out. The wait is S0CB6:18F1 (`wait_span(min,
  max)`, named in src/HL.hints; `wait_input` was taken by S0CB6:0036):
  any key, stick button or mouse click ends it, else 1F4h timer counts
  (6.88 s measured). Before key_install keys reach it through the BIOS
  buffer the timer routine reads (input.c). The names BB and HLLOGO are
  at S37FE:000E and 0011. Runs and numbers: the comment of S1650:0003 in
  src/HL.hints. menu.c needed no change, so the scoped exception for it
  was not used.
- S1650:0003's far argument is taken to be the port's `game_mem.p` (the
  caller loads it from S3303:0024; not checked against a run).
- The game's keys: the two sides' held keys are tables in DATA behind
  DATA:0B2C (side 1, at 0B30) and 0B2E (side 2, at 0B93), 7 bytes an
  entry (scancode, left, right, up, down, fire): side 1 Space and Enter
  fire, the keypad 1 to 9 (4F to 49) the eight directions (the arrows
  are the same codes behind E0); side 2 Alt and Ctrl fire, C down, D and
  F up, X left, V right. The pressed keys are compared with the table at
  S331F:0292 (game_loop, S0CB6, S1273, the menu, S1A39), entries 0 to
  22h: Esc, Space, Backspace, Del, Ctrl, Tab, Enter twice, the arrows,
  F1 to F10, S, E, Z, N, D, X, C, V, F, /, then 37h, 4Ah, 4Eh (the
  keypad's *, - and +). Which of those last three entries is read was
  not checked (the player confirmed + - * are free in play; frame.c
  keeps them from the program while the hud is set). S331F:02C7 to 02D2
  (twelve entries copied by start_setup) are not read by any routine
  found by a text search of build/HL.asm for those labels.
- Remapping is in main.c only: frame.h's `frame_set_keymap` and pad.h's
  `pad_set_keys` (PLAYER-SETTINGS.md) act before the game's INT 9, so
  the seam the lead asked about does not cross menu.c, input.c or
  joy.c. joy.c's game port stays without a stick in the window; a
  controller is keys.
- Not checked: the title screens and both pages in the window (the
  player live-checks), a byte comparison of the logo's pictures.

## win/titlefix: the logo's sparkle, the title's keys (2026-10-09, Windows)

- S2AAF:0000 (pic_draw's 'U' way, the kind of all of BB.DAT's pictures)
  takes the save place like S2A76:0002: `under_save` of the rectangle it
  draws, cut by S2AAF:0107 when DATA:00E8 is set (the cut way at S2AAF:0279).
  The port's `pic_blit_u` and `pic_cut_u` (draw.c) leave it out, as the
  hint of S2A6B:0008 says; draw.c was out of this branch's reach, so
  title.c does the save itself (`title_save`). It belongs into draw.c
  (`pic_draw_save` passing the save place to `pic_blit_u`), after which
  `title_save` goes. No other caller of a 'U' picture with a save place
  was looked for.
- S28A5:000C sets DATA:00E8 (the screen's cut) to FFFFh at its start and
  end and leaves it so; the port's title did not. What DATA:00E8 was
  before and whether later code depends on it staying FFFFh was not
  checked (the port now leaves it FFFFh as the original does).
- The key leak was the port's: `live_key` keeps the window's keys as a
  timed script and `play_start` rewinds to the first key at or after its
  start. Any later `play_start` to an earlier time in the window replays
  keys the same way; menu_game after the title now continues the clock
  (main.c). Whether the menu's first pictures in the window were also
  held back before the fix (play_start set `now` back but not `pic_at`)
  was not checked.
- Not checked: the window by eye (the player live-checks), the first
  frames of the logo against the original pixel for pixel.
