# Handoff

State of 2026-10-02: the repository was made from doskit's template.
`HL14-18.EXE` now rebuilds identically; its indirect code is not yet
fully classified.

The `doskit` submodule is at `95ba89e`. Its `xfer.py` retains a source
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
  entry). The apparent references to `CODE:123C`, `CODE:205C` and
  `CODE:2ACE` are respectively an unrelocated word, a DOS AH=09 text string
  and an argument, so they are not code-entry evidence.
- Four 20-second timed menu-key sequences (Escape/Enter, Enter/Space,
  Down/Enter and function keys) likewise reached none of those gaps. They
  changed the run state and hashes, but each coverage file still reports the
  same 34 gaps and 5,810 bytes; further menu-key variants are not a useful
  source of new code entries.
- Not checked: the classification and reachability of the remaining indirect
  CODE gaps. The non-entry relocation frames are conservatively not treated
  as code yet; use runner coverage and verified pointer/table hints next.

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
- A six-second normal run with Escape at three seconds loads the intro at
  `t=0.000007`; it exits at `t=3.111517` and the launcher EXECs the main
  game at `t=3.111522`. The launcher does not reach the child-status query
  at `CODE:009A` before that run ends. Thus the later outro branch has not
  been verified by this run.
- Not checked: runtime reachability of the remaining launcher gaps and the
  three vector handlers under the events that invoke them; obtain targeted
  runs or identify further verified pointer/table entries next.

## Next

1. Continue the `HL14-18.EXE` call graph by identifying callers of the
   C0015 vector-installation routines and verified pointer/table entries.
   Ordinary launcher input and first-EXEC error injection do not reach
   those installations; repeat targeted vector runs only with an evidenced
   path to them. The outro path also needs a verified main-game exit.
2. Classify `HL.EXE` indirect code with `gaps.py`, runner coverage and
   verified pointer/table hints, then begin stage 2 names and records.
3. Analyse `INTRO/INTRO.EXE` and `EXTRO/EXTRO.EXE` separately; transfer
   hints only when this game's own instruction streams demonstrate a
   match.
