/*  Copyright 2004 Theo Berkau

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

#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/cdrom.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

int hCDROM;

int CDInit(char *cdrom_name) {
	if ((hCDROM = open(cdrom_name, O_RDONLY | O_NONBLOCK)) == -1) {
		return -1;
	}
	fprintf(stderr, "CDInit (%s) OK\n", cdrom_name);
	return 0;
}

int CDDeInit() {
	close(hCDROM);

	fprintf(stderr, "CDDeInit OK\n");

	return 0;
}


bool CDIsCDPresent()
{
	int ret = ioctl(hCDROM, CDROM_DRIVE_STATUS, CDSL_CURRENT);
	return (ret == CDS_DISC_OK);
}

long CDReadToc(unsigned long *TOC)
{
   bool success;
   struct cdrom_tochdr ctTOC;
   struct cdrom_tocentry ctTOCent;
   int i, j;
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
      if (ioctl(hCDROM, CDROMREADTOCENTRY, &ctTOCent) == -1)
	      printf("graaaaaaaa\n");
      if (ctTOCent.cdte_addr.lba == 0) add150 = 150;
      TOC[0] = ((ctTOCent.cdte_ctrl << 28) |
	          (ctTOCent.cdte_adr << 24) |
	           ctTOCent.cdte_addr.lba + add150);

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

int CDGetStatus(void) {
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
}

bool CDReadSectorFAD(unsigned long FAD, void *buffer) {
	unsigned long dwBytesReturned;
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
			return false;
		}
		memcpy(buffer, position.bigbuf, 2352);

		return true;
	}

	return false;
}
