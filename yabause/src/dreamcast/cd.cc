/*  Copyright 2004 Lawrence Sebald
    Copyright 2004 Theo Berkau
    Copyright 2000 Dan Potter

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

#include <dc/cdrom.h>
#include <stdio.h>
#include <string.h>

extern "C"	{
	int __yaba_gdc_get_drv_stat(void *param);
	int __yaba_gdc_change_data_type(void *param);
}

int CDInit(char *cdrom_name)	{
	return 0;
}

int CDDeInit()	{
	/* Make sure everything is as KOS expects it.... */
	cdrom_reinit();
	return 0;
}

bool CDIsCDPresent()	{
	int status = 0;
	int err = cdrom_get_status(&status, NULL);
	
	if(err != ERR_OK)	{
		return false;
	}
	
	if(status == CD_STATUS_OPEN || status == CD_STATUS_NO_DISC)	{
		return false;
	}
	
	return true;
}

long CDReadToc(unsigned long *TOC)	{
	CDROM_TOC dctoc;
	uint32 i;
	
	memset(TOC, 0xFF, 0xCC * 2);
	cdrom_read_toc(&dctoc, 0);
	
	printf("cd:\t TOC info: First Track = %d, Last Track = %d\n", dctoc.entry[0], dctoc.last);
	
	for(i = 0; i < 99; i++)	{
		TOC[i] = dctoc.entry[i] + 150;
		printf("TOC[i] = %d\n", TOC[i]);
	}
	
	/* This *should* be right, but I'm not quite sure... */
	TOC[99] = (dctoc.entry[0] & 0xFF000000) | dctoc.first << 16;
	TOC[100] = (dctoc.entry[dctoc.last - 1] & 0xFF000000) | dctoc.last << 16;
	TOC[101] = dctoc.entry[dctoc.last] + 150 /* leadout_sector */; /* not sure on this one... need some real testing.... */
	
	return 0xCC * 2;
}

unsigned long CDReadSector(unsigned long lba, unsigned long size, void *buffer)	{
#if 0
	/* WARNING! Its probably not technically safe to do this without locking the cdrom mutex and all first, but
	   I have no access to said mutex, so, we'll just see what happens ;) */
	/* Borrowed from KOS again :) (Isn't low level access fun!) */
	uint32 params[4];
	int cdxa;
	int rv;
	
	__yaba_gdc_get_drv_stat(params);
	cdxa = params[1] == 32;
	params[0] = 0;				/* 0 = set, 1 = get */
	params[1] = 8192;			/* ? */
	params[2] = cdxa ? 2048 : 1024;		/* CD-XA mode 1/2 */
	params[3] = size;			/* sector size */
	if (__yaba_gdc_change_data_type(params) < 0) { return 0; }
	/* This ends the code borrowed from KOS */
		
	cdrom_read_sectors(buffer, lba, 1);
#endif
	cdrom_read_sectors(buffer, lba - 150, 1);
	
	return size;	
}
