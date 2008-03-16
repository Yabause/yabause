/*  Copyright 2008 Theo Berkau

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
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <gccore.h>
#include <sdcard.h>
#include "../cs0.h"
#include "../m68kcore.h"
#include "../peripheral.h"
#include "../vidsoft.h"
#include "../vdp2.h"
#include "../yui.h"

static u32 *xfb[2] = { NULL, NULL };
int fbsel = 0;
static GXRModeObj *rmode = NULL;
volatile int done=0;
volatile int resetemu=0;
int running=0;

SH2Interface_struct *SH2CoreList[] = {
&SH2Interpreter,
&SH2DebugInterpreter,
NULL
};

PerInterface_struct *PERCoreList[] = {
&PERDummy,
NULL
};

CDInterface *CDCoreList[] = {
&DummyCD,
&ISOCD,
NULL
};

SoundInterface_struct *SNDCoreList[] = {
&SNDDummy,
NULL
};

VideoInterface_struct *VIDCoreList[] = {
&VIDDummy,
&VIDSoft,
NULL
};

M68K_struct *M68KCoreList[] = {
&M68KDummy,
&M68KC68K,
NULL
};

char biosfilename[512]="\0";

extern int vdp2width, vdp2height;

void reset()
{
   resetemu=1;
}

void powerdown()
{
   done = 1;
}

int main(int argc, char **argv)
{
   yabauseinit_struct yinit;
   int ret;

   VIDEO_Init();
   PAD_Init();
   SDCARD_Init();

   SYS_SetResetCallback(reset);
   SYS_SetPowerCallback(powerdown);
	
   switch(VIDEO_GetCurrentTvMode()) 
   {
      case VI_NTSC:
         rmode = &TVNtsc480IntDf;
	 break;
      case VI_PAL:
  	 rmode = &TVPal528IntDf;
 	 break;
      case VI_MPAL:
	 rmode = &TVMpal480IntDf;
	 break;
      default:
	 rmode = &TVNtsc480IntDf;
	 break;
   }

   // Allocate two buffers(may not be necessary)
   xfb[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
   xfb[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

   console_init(xfb[fbsel],20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	
   VIDEO_Configure(rmode);
   VIDEO_ClearFrameBuffer (rmode, xfb[0], COLOR_BLACK);
   VIDEO_ClearFrameBuffer (rmode, xfb[1], COLOR_BLACK);
   VIDEO_SetNextFramebuffer(xfb[0]);
   VIDEO_SetBlack(FALSE);
   VIDEO_Flush();
   VIDEO_WaitVSync();
   if(rmode->viTVMode&VI_NON_INTERLACE) 
      VIDEO_WaitVSync();

   memset(&yinit, 0, sizeof(yabauseinit_struct));
   yinit.percoretype = PERCORE_DUMMY;
   yinit.sh2coretype = SH2CORE_INTERPRETER;
   yinit.vidcoretype = VIDCORE_SOFT;
   yinit.sndcoretype = SNDCORE_DUMMY;
   yinit.cdcoretype = CDCORE_DUMMY;
   yinit.m68kcoretype = M68KCORE_C68K;
   yinit.carttype = CART_NONE;
   yinit.regionid = REGION_AUTODETECT;
   if (strcmp(biosfilename, "") == 0)
      yinit.biospath = NULL;
   else
      yinit.biospath = biosfilename;
   yinit.cdpath = NULL;
   yinit.buppath = NULL;
   yinit.mpegpath = NULL;
   yinit.cartpath = NULL;
   yinit.netlinksetting = NULL;
   yinit.flags = VIDEOFORMATTYPE_NTSC;

   if ((ret = YabauseInit(&yinit)) != 0)
   {
      sd_file *fp;
      s32 size;
      s32 i;
      u32 data;
      unsigned char *buf;


      // If it's failing, it's only because of the missing bios/cd
      while ((fp = SDCARD_OpenFile("dev0:\\bios.bin", "rb")) == NULL)
      {         
         printf("Couldn't find bios...trying again in 5 seconds...\n");
         for(i = 0; i < (60 * 5); i++)
            VIDEO_WaitVSync();
      }

      printf("Found bios\n");
      printf("Loading bios to rom area...\n");
      size = SDCARD_GetFileSize(fp);
      SDCARD_SeekFile(fp, 0, SDCARD_SEEK_SET);
      VIDEO_WaitVSync();

      buf = malloc(size);
      SDCARD_ReadFile(fp, buf, size);

      for (i = 0; i < size / sizeof(data); i++)
         T2WriteByte(BiosRom, i, buf[i]);

      free(buf);

      SDCARD_CloseFile(fp);

      YabauseResetNoLoad();
      printf("done.\n");
   }

   EnableAutoFrameSkip();

   console_init(xfb[fbsel],20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);

   while(!done)
   {
      if (PERCore->HandleEvents() != 0)
         return -1;
      if (resetemu)
      {
         YabauseReset();
         resetemu = 0;
      }
   }

   exit(0);
   return 0;
}

void YuiErrorMsg(const char *string)
{
   if (strncmp(string, "Master SH2 invalid opcode", 25) == 0)
   {
      if (!running)
         return;
      running = 0;
   }

   printf("%s\n", string);
}

void YuiSwapBuffers()
{
   int i, j;
   u32 *curfb;
   u32 *buf;

   fbsel ^= 1;
   curfb = xfb[fbsel];
   buf = dispbuffer;

   for (j = 0; j < vdp2height; j++)
   {
      for (i = 0; i < (vdp2width / 2); i++)
      {
         // This isn't pretty
         int y1, cb1, cr1, y2, cb2, cr2, cb, cr;
         u8 r, g, b;
      
         r = buf[0] >> 24;
         g = buf[0] >> 16;
         b = buf[0] >> 8;
         buf++;
        
         y1 = (299 * r + 587 * g + 114 * b) / 1000;
         cb1 = (-16874 * r - 33126 * g + 50000 * b + 12800000) / 100000;
         cr1 = (50000 * r - 41869 * g - 8131 * b + 12800000) / 100000;

         r = buf[0] >> 24;
         g = buf[0] >> 16;
         b = buf[0] >> 8;
         buf++;
 
         y2 = (299 * r + 587 * g + 114 * b) / 1000;
         cb2 = (-16874 * r - 33126 * g + 50000 * b + 12800000) / 100000;
         cr2 = (50000 * r - 41869 * g - 8131 * b + 12800000) / 100000;
 
         cb = (cb1 + cb2) >> 1;
         cr = (cr1 + cr2) >> 1;

         curfb[0] = (y1 << 24) | (cb << 16) | (y2 << 8) | cr;
         curfb++;
      }
      curfb += (640 - vdp2width) / 2;
   }
   
   VIDEO_SetNextFramebuffer (xfb[fbsel]);
   VIDEO_Flush ();
}
