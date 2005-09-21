/*  Copyright 2003 Guillaume Duhamel
    Copyright 2004-2005 Lawrence Sebald

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

#include "yui.h"
#include "peripheral.h"
#include "../cs0.h"
#include "dreamcast/perdc.h"

SH2Interface_struct *SH2CoreList[] = {
&SH2Interpreter,
NULL
};

PerInterface_struct *PERCoreList[] = {
&PERDC,
NULL
};

CDInterface *CDCoreList[] = {
&DummyCD,
NULL
};

SoundInterface_struct *SNDCoreList[] = {
&SNDDummy,
NULL
};

VideoInterface_struct *VIDCoreList[] = {
&VIDDummy,
NULL
};

int stop;

const char *bios = "/pc/saturn.bin";
const char *iso_or_cd = 0;
const char *backup_ram = "/ram/backup.ram";
int cdcore = CDCORE_DEFAULT;

void YuiSetBiosFilename(const char *biosfilename) {
        bios = biosfilename;
}

void YuiSetIsoFilename(const char *isofilename) {
	//cdcore = CDCORE_ISO;
	//iso_or_cd = isofilename;
}

void YuiSetCdromFilename(const char *cdromfilename) {
	//cdcore = CDCORE_ARCH;
	//iso_or_cd = cdromfilename;
}

void YuiHideShow(void) {
}

void YuiQuit(void) {
   stop = 1;
}

int YuiInit(void) {
   yabauseinit_struct yinit;

   stop = 0;

   yinit.percoretype = PERCORE_DC;
   yinit.sh2coretype = SH2CORE_DEFAULT;
   yinit.vidcoretype = 0;
   yinit.sndcoretype = 0;
   yinit.cdcoretype = cdcore;
   yinit.carttype = CART_NONE;
   yinit.regionid = REGION_AUTODETECT;
   yinit.biospath = bios;
   yinit.cdpath = iso_or_cd;
   yinit.buppath = backup_ram;
   yinit.mpegpath = NULL;
   yinit.cartpath = NULL;

   if (YabauseInit(&yinit) != 0)
      return -1;

   while (!stop)
   {
      if (PERCore->HandleEvents() != 0)
         return -1;
   }

   return 0;
}

void YuiErrorMsg(const char *error_text, SH2_struct *sh2opcodes) {
   fprintf(stderr, "Error: %s\n", error_text);

   fprintf(stderr, "R0 = %08X\tR12 = %08X\n", sh2opcodes->regs.R[0], sh2opcodes->regs.R[12]);
   fprintf(stderr, "R1 = %08X\tR13 = %08X\n", sh2opcodes->regs.R[1], sh2opcodes->regs.R[13]);
   fprintf(stderr, "R2 = %08X\tR14 = %08X\n", sh2opcodes->regs.R[2], sh2opcodes->regs.R[14]);
   fprintf(stderr, "R3 = %08X\tR15 = %08X\n", sh2opcodes->regs.R[3], sh2opcodes->regs.R[15]);
   fprintf(stderr, "R4 = %08X\tSR = %08X\n", sh2opcodes->regs.R[4], sh2opcodes->regs.SR.all);
   fprintf(stderr, "R5 = %08X\tGBR = %08X\n", sh2opcodes->regs.R[5], sh2opcodes->regs.GBR);
   fprintf(stderr, "R6 = %08X\tVBR = %08X\n", sh2opcodes->regs.R[6], sh2opcodes->regs.VBR);
   fprintf(stderr, "R7 = %08X\tMACH = %08X\n", sh2opcodes->regs.R[7], sh2opcodes->regs.MACH);
   fprintf(stderr, "R8 = %08X\tMACL = %08X\n", sh2opcodes->regs.R[8], sh2opcodes->regs.MACL);
   fprintf(stderr, "R9 = %08X\tPR = %08X\n", sh2opcodes->regs.R[9], sh2opcodes->regs.PR);
   fprintf(stderr, "R10 = %08X\tPC = %08X\n", sh2opcodes->regs.R[10], sh2opcodes->regs.PC);
   fprintf(stderr, "R11 = %08X\n", sh2opcodes->regs.R[11]);
   stop = 1;
}

