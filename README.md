[![Travis CI Build Status](https://travis-ci.org/FCare/Kronos.svg?branch=extui-align)](https://travis-ci.org/github/FCare/Kronos)


Latest windows build can be found here:
http://tradu-france.com/index.php?page=fullstory&id=676

To build on Linux:
- Clone the current repository in DIR
- Execute cmake $DIR/yabause
- Then execute make -j8
you will need SDL2 and qt5 dev packages.

To generate a changelog, add in your commits the [ChangeLog] tag. Changelog will be extracted like this 
  git shortlog --grep=Changelog --since "01 Jan 2020"
