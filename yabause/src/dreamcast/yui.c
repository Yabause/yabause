/*  Copyright 2003 Guillaume Duhamel
    Copyright 2004-2006 Lawrence Sebald

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <arch/arch.h>

#include "yui.h"
#include "peripheral.h"
#include "../cs0.h"
#include "dreamcast/perdc.h"
#include "dreamcast/viddc.h"

SH2Interface_struct *SH2CoreList[] = {
&SH2Interpreter,
NULL
};

PerInterface_struct *PERCoreList[] = {
&PERDC,
NULL
};

CDInterface *CDCoreList[] = {
&ArchCD,
&DummyCD,
NULL
};

SoundInterface_struct *SNDCoreList[] = {
&SNDDummy,
NULL
};

VideoInterface_struct *VIDCoreList[] = {
&VIDDummy,
&VIDDC,
NULL
};

const char *bios = "/pc/saturn.bin";

int YuiInit(void)   {
    yabauseinit_struct yinit;

    yinit.percoretype = PERCORE_DC;
    yinit.sh2coretype = SH2CORE_DEFAULT;
    yinit.vidcoretype = VIDCORE_DC;
    yinit.sndcoretype = SNDCORE_DUMMY;
    yinit.cdcoretype = CDCORE_ARCH;
    yinit.carttype = CART_NONE;
    yinit.regionid = REGION_AUTODETECT;
    yinit.biospath = bios;
    yinit.cdpath = NULL;
    yinit.buppath = NULL;
    yinit.mpegpath = NULL;
    yinit.cartpath = NULL;

    if(YabauseInit(&yinit) != 0)
      return -1;

    for(;;) {
      if(PERCore->HandleEvents() != 0)
         return -1;
    }

    return 0;
}

void YuiErrorMsg(const char *error_text)    {
    fprintf(stderr, "Error: %s\n", error_text);
    arch_exit();
}

/* stub for now... just to make things compile again... */
void YuiVideoResize(unsigned int w, unsigned int h, int isfullscreen)   {
}

int main(int argc, char *argv[])    {
    printf("...\n");
    YuiInit();

    return 0;
}
