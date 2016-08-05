/*  Copyright 2004-2005 Theo Berkau
    Copyright 2004-2006 Guillaume Duhamel
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

#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/cdrom.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#ifdef LINUX_CDROM_H_IS_BROKEN
#include <limits.h>
#endif

#include "cdbase.h"
#include "debug.h"

static int LinuxCDInit(const char *);
static void LinuxCDDeInit(void);
static s32 LinuxCDReadTOC(u32 *);
static s32 LinuxCDReadTOC10(CDInterfaceToc10 *);
static int LinuxCDGetStatus(void);
static int LinuxCDReadSectorFAD(u32, void *);
static void LinuxCDReadAheadFAD(u32);

CDInterface ArchCD = {
	CDCORE_ARCH,
	"Linux CD Drive",
	LinuxCDInit,
	LinuxCDDeInit,
	LinuxCDGetStatus,
	LinuxCDReadTOC,
	LinuxCDReadTOC10,
	LinuxCDReadSectorFAD,
	LinuxCDReadAheadFAD,
};

static int hCDROM;

static int LinuxCDInit(const char * cdrom_name) {
	if ((hCDROM = open(cdrom_name, O_RDONLY | O_NONBLOCK)) == -1) {
		return -1;
	}
	LOG("CDInit (%s) OK\n", cdrom_name);
	return 0;
}

static void LinuxCDDeInit(void) {
	if (hCDROM != -1) {
		close(hCDROM);
	}

	LOG("CDDeInit OK\n");
}


static s32 LinuxCDReadTOC(u32 * TOC)
{
   struct cdrom_tochdr ctTOC;
   struct cdrom_tocentry ctTOCent;
   int i;
   int add150 = 0;

   ctTOCent.cdte_format = CDROM_LBA;

   if (hCDROM != -1)
   {
      memset(TOC, 0xFF, 0xCC * 2);
      memset(&ctTOC, 0xFF, sizeof(struct cdrom_tochdr));

      if (ioctl(hCDROM, CDROMREADTOCHDR, &ctTOC) == -1) {
	return 0;
      }

      ctTOCent.cdte_track = ctTOC.cdth_trk0;
      ioctl(hCDROM, CDROMREADTOCENTRY, &ctTOCent);
      if (ctTOCent.cdte_addr.lba == 0) add150 = 150;
      TOC[0] = ((ctTOCent.cdte_ctrl << 28) |
	          (ctTOCent.cdte_adr << 24) |
	           (ctTOCent.cdte_addr.lba + add150));

      // convert TOC to saturn format
      for (i = ctTOC.cdth_trk0 + 1; i <= ctTOC.cdth_trk1; i++)
      {      
	      ctTOCent.cdte_track = i;
	      ioctl(hCDROM, CDROMREADTOCENTRY, &ctTOCent);
	      TOC[i - 1] = (ctTOCent.cdte_ctrl << 28) |
                  (ctTOCent.cdte_adr << 24) |
		  (ctTOCent.cdte_addr.lba + add150);
      }

      // Do First, Last, and Lead out sections here

      ctTOCent.cdte_track = ctTOC.cdth_trk0;
      ioctl(hCDROM, CDROMREADTOCENTRY, &ctTOCent);
      TOC[99] = (ctTOCent.cdte_ctrl << 28) |
                (ctTOCent.cdte_adr << 24) |
                (ctTOC.cdth_trk0 << 16);

      ctTOCent.cdte_track = ctTOC.cdth_trk1;
      ioctl(hCDROM, CDROMREADTOCENTRY, &ctTOCent);
      TOC[100] = (ctTOCent.cdte_ctrl << 28) |
                 (ctTOCent.cdte_adr << 24) |
                 (ctTOC.cdth_trk1 << 16);

      ctTOCent.cdte_track = CDROM_LEADOUT;
      ioctl(hCDROM, CDROMREADTOCENTRY, &ctTOCent);
      TOC[101] = (ctTOCent.cdte_ctrl << 28) |
                 (ctTOCent.cdte_adr << 24) |
		 (ctTOCent.cdte_addr.lba + add150);

      return (0xCC * 2);
   }

   return 0;
}

static s32 LinuxCDReadTOC10(CDInterfaceToc10 *TOC)
{
   if (hCDROM != -1)
   {
      struct cdrom_tochdr ctTOC;
      struct cdrom_tocentry ctTOCent;
      u8 i, num_toc = 0;

      memset(&ctTOC, 0xFF, sizeof(struct cdrom_tochdr));

      if (ioctl(hCDROM, CDROMREADTOCHDR, &ctTOC) == -1)
         return 0;

      ctTOCent.cdte_format = CDROM_MSF;
      for(i = ctTOC.cdth_trk0;i <= ctTOC.cdth_trk1;i++)
      {
         ctTOCent.cdte_track = i;
         if (ioctl(hCDROM, CDROMREADTOCENTRY, &ctTOCent) == -1)
	    return 0;

         TOC[num_toc].ctrladr = (ctTOCent.cdte_ctrl << 4 | ctTOCent.cdte_adr);
         TOC[num_toc].tno = 0;
         TOC[num_toc].point = i;
         TOC[num_toc].min = num_toc + 150;
         TOC[num_toc].sec = num_toc + 150;
         TOC[num_toc].frame = num_toc + 150;
         TOC[num_toc].zero = 0;
         TOC[num_toc].pmin = ctTOCent.cdte_addr.msf.minute;
         TOC[num_toc].psec = ctTOCent.cdte_addr.msf.second;
         TOC[num_toc].pframe = ctTOCent.cdte_addr.msf.frame;

         num_toc++;
      }

      // A0
      ctTOCent.cdte_track = ctTOC.cdth_trk0;
      if (ioctl(hCDROM, CDROMREADTOCENTRY, &ctTOCent) == -1)
         return 0;
      TOC[num_toc].ctrladr = (ctTOCent.cdte_ctrl << 4 | ctTOCent.cdte_adr);
      TOC[num_toc].tno = 0;
      TOC[num_toc].point = 0xA0;
      TOC[num_toc].min = num_toc + 150;
      TOC[num_toc].sec = num_toc + 150;
      TOC[num_toc].frame = num_toc + 150;
      TOC[num_toc].zero = 0;
      TOC[num_toc].pmin = ctTOC.cdth_trk0;
      TOC[num_toc].psec = 0;
      TOC[num_toc].pframe = 0;

      num_toc++;

      // A1
      ctTOCent.cdte_track = ctTOC.cdth_trk1;
      if (ioctl(hCDROM, CDROMREADTOCENTRY, &ctTOCent) == -1)
         return 0;
      TOC[num_toc].ctrladr = (ctTOCent.cdte_ctrl << 4 | ctTOCent.cdte_adr);
      TOC[num_toc].tno = 0;
      TOC[num_toc].point = 0xA1;
      TOC[num_toc].min = num_toc + 150;
      TOC[num_toc].sec = num_toc + 150;
      TOC[num_toc].frame = num_toc + 150;
      TOC[num_toc].zero = 0;
      TOC[num_toc].pmin = ctTOC.cdth_trk1;
      TOC[num_toc].psec = 0;
      TOC[num_toc].pframe = 0;

      num_toc++;

      // A2
      ctTOCent.cdte_track = CDROM_LEADOUT;
      if (ioctl(hCDROM, CDROMREADTOCENTRY, &ctTOCent) == -1)
         return 0;
      TOC[num_toc].ctrladr = (ctTOCent.cdte_ctrl << 4 | ctTOCent.cdte_adr);
      TOC[num_toc].tno = 0;
      TOC[num_toc].point = 0xA2;
      TOC[num_toc].min = num_toc + 150;
      TOC[num_toc].sec = num_toc + 150;
      TOC[num_toc].frame = num_toc + 150;
      TOC[num_toc].zero = 0;
      TOC[num_toc].pmin = ctTOCent.cdte_addr.msf.minute;
      TOC[num_toc].psec = ctTOCent.cdte_addr.msf.second;
      TOC[num_toc].pframe = ctTOCent.cdte_addr.msf.frame;

      num_toc++;

      return num_toc;
   }

   return 0;
}

static int LinuxCDGetStatus(void) {
	// 0 - CD Present, disc spinning
	// 1 - CD Present, disc not spinning
	// 2 - CD not present
	// 3 - Tray open
	// see ../windows/cd.cc for more info

	int ret = ioctl(hCDROM, CDROM_DRIVE_STATUS, CDSL_CURRENT);
	switch(ret) {
		case CDS_DISC_OK:
			return 0;
		case CDS_NO_DISC:
			return 2;
		case CDS_TRAY_OPEN:
			return 3;
	}

	// guess it's ok to say there's no disc here...
	return 2;
}

static int LinuxCDReadSectorFAD(u32 FAD, void *buffer) {
	union {
		struct cdrom_msf msf;
		char bigbuf[2352];
	} position;

	if (hCDROM != -1)
	{
		position.msf.cdmsf_min0 = FAD / 4500;
		position.msf.cdmsf_sec0 = (FAD % 4500) / 75;
		position.msf.cdmsf_frame0 = FAD % 75;

		if (ioctl(hCDROM, CDROMREADRAW, &position) == -1) {
			return 0;
		}
		
		memcpy(buffer, position.bigbuf, 2352);

		return 1;
	}

	return 0;
}

static void LinuxCDReadAheadFAD(UNUSED u32 FAD)
{
	// No-op
}
