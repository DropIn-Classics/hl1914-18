# File-analysis tools

The TPWM, palette, IFF and library tools were imported from `bisle` at the
user's request after testing them on installed History Line files. Four
additional experimental inspectors are described below. See
`../PROVENANCE.md` for the source scope and `../docs/HANDOFF.md` for results
and limitations. They require only Python 3 and the doskit checkout.

Run from the project root; without file arguments they discover inputs in
`game/` (or `$DOSKIT_GAME`):

    python tools/tpwmfiles.py
    python tools/palfiles.py
    python tools/ifffiles.py
    python tools/libfiles.py

Each tool checks that parsed contents write back identically. TPWM checks
packed items; pictures and libraries check their unpacked contents.
Unknown fields, chunks, entry kinds and tails may be retained unchanged.
Picture discovery covers `.IFF` and `.LBM`; other IFF container formats
are outside the verified picture set.

For local exports, keep all outputs in `build/`:

    python tools/tpwmfiles.py --out build/unpacked
    python tools/palfiles.py --png build/palettes
    python tools/ifffiles.py --png build/pictures
    python tools/libfiles.py game/INTRO/CHR14.LIB --png build/sprites

Library previews default to `game/00.PAL` and colour base zero. Use
`--pal FILE` and `--base N` to choose explicitly. Palette scaling and
sprite drawing have not been compared with History Line's original
execution. PNG export is an inspection aid.

On Windows consoles with limited encodings, set `PYTHONIOENCODING=utf-8`
before running the tools, since library entry names can contain characters
the console cannot encode. `png.py` is the shared PNG-writing helper.

`datfiles.py`, `mapfiles.py`, `sndfiles.py`, and `txtfiles.py` are
experimental History Line inspection tools. They report or reject layouts
outside their observed coverage; a successful round trip does not establish
that the game loads or interprets a format the same way. See
`docs/HANDOFF.md` and `PROVENANCE.md` for probe results and source scope.
These tools are not part of the port.

`savfiles.py FILE [--list]` splits a saved game (`0N.DAT`, in the runner's
state folder after a save) into its header and 24 blocks and checks that it
writes back identically. The layout is read from `HL.EXE`'s own `save_game`
(see `docs/HANDOFF.md`); what most blocks hold is not known yet.
