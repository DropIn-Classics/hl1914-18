# File-analysis tools

These offline tools were imported from `bisle` at the user's request after
testing them on the installed History Line files. See `../PROVENANCE.md`
for the explicit scope and `../docs/HANDOFF.md` for results and limitations.
They require only Python 3 and the existing doskit checkout.

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

The map, table, text and sound tools were not imported: their probes had
rejections or incomplete coverage. These tools are not part of the port.
