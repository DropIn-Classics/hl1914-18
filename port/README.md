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

## Build and run

    sh port/build.sh          # macOS, Linux (SDL2 for the window)
    port\build.bat            # Windows (MSVC)
    port/build/history-line -game game

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
