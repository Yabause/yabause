# Kronos libretro

## How to build
From the root project directory, run `make -C libretro`.

Library will be built in the libretro subfolder.

You'll need retroarch (or another libretro frontend) to run this library.

## How to run Saturn iso
Command line looks like this :

`retroarch -L path/to/library path/to/iso`

## How to run ST-V MAME roms
Command line looks like this :

`retroarch -L path/to/library --subsystem stv path/to/rom`

## Where do you copy the bioses
Bioses go to the SYSTEM_DIRECTORY/kronos directory, saturn bios is expected to be named `saturn_bios.bin`, for ST-V bios you just need to copy `stvbios.zip` from MAME

## Currently known issues
- The core only works with an OpenGLES3 enabled retroarch at the moment, OpenGL3.3 profile should be coming soon.
- The core crashes when you switch from fullscreen to windowed on the fly.
- It was mostly tested under linux so expect issues on other OSes at the moment (NB : i know for sure that loading ST-V games doesn't work on windows currently).
