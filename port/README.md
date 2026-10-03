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

(what was compared with the original, where and how)
