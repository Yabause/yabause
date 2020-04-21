[![Travis CI Build Status](https://travis-ci.org/FCare/Kronos.svg?branch=extui-align)](https://travis-ci.org/github/FCare/Kronos)


Latest windows build can be found here:
http://tradu-france.com/index.php?page=fullstory&id=676

To build on Linux:
- Clone the current repository in DIR

For standalone version:
	- Execute cmake $DIR/yabause
	- Then execute make -j8
	you will need SDL2 and qt5 dev packages.

For retroarch core:
	- mkdir build_retro; cd build_retro;
	- make -j4 -C ../yabause/src/libretro/ generate-files
	- make -j4 -C ../yabause/src/libretro/
	- then execute retroarch -L ../yabause/src/libretro/kronos_libretro.so path_to_your_iso

To generate a changelog, add in your commits the [ChangeLog] tag. Changelog will be extracted like this
  git shortlog --grep=Changelog --since "01 Jan 2020"
