# YabaSanshiro libretro

## How to build (on odroid XU3/XU4)
From the root project directory, run `make -C yabause/src/libretro generate-files`, then `make -C yabause/src/libretro platform=odroid`.

Library will be built at `yabause/src/libretro/yabasanshiro_libretro.so`.

You'll need retroarch (or another libretro frontend) to run this library.

## How to run games
Command line looks like this :

`retroarch -L path/to/library path/to/iso/or/rom`

## Where do you copy the bioses
Bioses go into the retroarch system directory, saturn bios is expected to be named `saturn_bios.bin`.

## Currently known issues
- The core crashes when you switch from fullscreen to windowed on the fly.
- It's still a bit rough around the edge and i only tested it on ORA Retropie with a XU4
