[![Travis CI Build Status](https://travis-ci.org/Yabause/yabause.svg?branch=master)](https://travis-ci.org/Yabause/yabause)
[![Appveyor Build status](https://ci.appveyor.com/api/projects/status/n35d1obw5deo1dsl/branch/master?svg=true)](https://ci.appveyor.com/project/Guillaumito/yabause)
[![CircleCI Build Status](https://circleci.com/gh/Yabause/yabause/tree/master.svg?style=shield&circle-token=c3153fb8a4e9d5a8801604ce5cac566c5ea16774)](https://circleci.com/gh/Yabause/yabause)
[![Coverity Scan Status](https://scan.coverity.com/projects/6271/badge.svg)](https://scan.coverity.com/projects/6271)


[![Join the chat at https://gitter.im/uoyabause/Lobby](https://badges.gitter.im/uoyabause/Lobby.svg)]
(https://gitter.im/uoyabause/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)


Latest windows build can be found here:
http://tradu-france.com/index.php?page=fullstory&id=676

To build on Linux:
- Clone the current repository in DIR
- Execute cmake $DIR/yabause
- Then execute make -j8
you will need SDL2 and qt5 dev packages.

To generate a changelog, add in your commits the [ChangeLog] tag. Changelog will be extracted like this 
  git shortlog --grep=Changelog --since "01 Jan 2020"
