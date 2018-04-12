/*  Copyright 2006 Guillaume Duhamel
    Copyright 2006 Fabien Coulon
    Copyright 2005 Joost Peters

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/
#define TEST_KRONOS_INTERPRETER
#include <assert.h>
#include <unistd.h>
#include <sys/time.h>

#include "platform.h"

#include "../yabause.h"
#include "../gameinfo.h"
#include "../yui.h"
#include "../peripheral.h"
#include "../sh2core.h"
#include "../sh2int.h"
#ifdef TEST_KRONOS_INTERPRETER
#include "../sh2_kronos/sh2int_kronos.h"
#endif
#ifdef HAVE_LIBGL
#include "../vidogl.h"
#include "../ygl.h"
#endif
#include "../vidsoft.h"
#include "../cs0.h"
#include "../cs2.h"
#include "../cdbase.h"
#include "../scsp.h"
#include "../sndsdl.h"
#include "../sndal.h"
#include "../persdljoy.h"
#ifdef ARCH_IS_LINUX
#include "../perlinuxjoy.h"
#endif
#include "../debug.h"
#include "../m68kcore.h"
#include "../m68kc68k.h"
#include "../vdp1.h"
#include "../vdp2.h"
#include "../cdbase.h"
#include "../peripheral.h"


M68K_struct * M68KCoreList[] = {
&M68KDummy,
NULL
};

SH2Interface_struct *SH2CoreList[] = {
#ifdef TEST_KRONOS_INTERPRETER
&SH2KronosInterpreter,
#endif
&SH2Interpreter,
NULL
};

PerInterface_struct *PERCoreList[] = {
&PERDummy,
};

CDInterface *CDCoreList[] = {
&DummyCD,
};

SoundInterface_struct *SNDCoreList[] = {
&SNDDummy,
};

VideoInterface_struct *VIDCoreList[] = {
&VIDDummy,
};

#ifdef YAB_PORT_OSD
#include "nanovg/nanovg_osdcore.h"
OSD_struct *OSDCoreList[] = {
&OSDNnovg,
NULL
};
#endif

static int fullscreen = 0;
static int lowres_mode = 0;

static int initDone = 0;

static char biospath[256] = "\0";
static char cdpath[256] = "\0";

yabauseinit_struct yinit;

void YuiErrorMsg(const char * string) {
}

int YuiRevokeOGLOnThisThread(){
  return 0;
}

int YuiUseOGLOnThisThread(){
  return 0;
}

void YuiSwapBuffers(void) {
}

void YuiInit() {
	yinit.m68kcoretype = 0;
	yinit.percoretype = 0;
	yinit.sh2coretype = 0;
#ifdef TEST_KRONOS_INTERPRETER
	yinit.sh2coretype = 8;
#endif
        yinit.vidcoretype = VIDCORE_SOFT;
	yinit.sndcoretype = 0;
	yinit.cdcoretype = CDCORE_DEFAULT;
	yinit.carttype = 0;
	yinit.regionid = REGION_EUROPE;
	yinit.biospath = NULL;
	yinit.cdpath = NULL;
	yinit.buppath = NULL;
	yinit.mpegpath = NULL;
	yinit.cartpath = NULL;
	yinit.osdcoretype = OSDCORE_DEFAULT;
	yinit.skip_load = 0;
	yinit.usethreads = 0;
}

void initEmulation() {
   if (initDone == 0) {
   YuiInit();
   if (YabauseSh2Init(&yinit) != 0) {
    printf("YabauseSh2Init error \n\r");
    return;
  }
  enableCache();
  initDone = 1;
  }
}

