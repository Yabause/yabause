# Kronos libretro

## How to build
From the root project directory, use `make -C libretro generate-files` to generate a few needed files, then `make -C libretro` to generate the library.

## How to run
You'll need retroarch (or another libretro frontend).
Command line look like this :

`retroarch -L kronos_libretro.so path/to/your/game/iso`

You can also use full path to the kronos_libretro library, however relative path seems to crash at the moment
