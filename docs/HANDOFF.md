# Handoff

State of 2026-10-02: the repository was made from doskit's template.
`HL14-18.EXE` now rebuilds identically; its indirect code is not yet
fully classified.

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
- Not checked: the meaning and runtime reachability of the remaining gaps
  and all of `C0015`; obtain a run that enters them or identify a verified
  pointer/table entry next.

## Next

1. Finish the `HL14-18.EXE` call graph: find a verified entry into `C0015`
   and use targeted runner coverage for the remaining gaps, adding only
   verified pointer/table hints.
2. Classify `HL.EXE` indirect code with `gaps.py`, runner coverage and
   verified pointer/table hints, then begin stage 2 names and records.
3. Analyse `INTRO/INTRO.EXE` and `EXTRO/EXTRO.EXE` separately; transfer
   hints only when this game's own instruction streams demonstrate a
   match.
