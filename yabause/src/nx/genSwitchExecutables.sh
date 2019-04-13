#!/bin/sh

set -o xtrace

mv yabasanshiro yabasanshiro.elf
elf2nro yabasanshiro.elf ../../yabasanshiro.nro
elf2nso yabasanshiro.elf ../../yabasanshiro.nso

#mkdir -p exefs
#cp yabasanshiro.nso ./exefs/main
#build_pfs0 exefs yabasanshiro.pfs0

