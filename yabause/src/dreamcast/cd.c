/*  Copyright 2006, 2008 Lawrence Sebald
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

/*  The code written by Dan Potter is actually licensed under a new-BSD license
    and is taken from KallistiOS. */

#include <dc/cdrom.h>
#include <kos/thread.h>

#include "cdbase.h"
#include "debug.h"

/* These are defined in cd.s */
extern int __gdc_change_data_type(void *param);
extern int DCCDGetStatus(void);
extern int DCCDDeInit(void);

/* And these are still here (for now anyway) */
int DCCDInit(const char *);
int DCCDDeInit(void);
s32 DCCDReadTOC(u32 *);
int DCCDReadSectorFAD(u32, void *);

CDInterface ArchCD = {
    CDCORE_ARCH,
    "Dreamcast CD Drive",
    DCCDInit,
    DCCDDeInit,
    DCCDGetStatus,
    DCCDReadTOC,
    DCCDReadSectorFAD
};

/* This is mostly taken from the KallistiOS cdrom_reinit() function */
int DCCDInit(const char * cdrom_name)   {
    int	r = -1;
    uint32 params[4];
    int	timeout;

    /* Try a few times; it might be busy. If it's still busy
        after this loop then it's probably really dead. */
    timeout = 10*1000/20;	/* 10 second timeout */
    while(timeout > 0)  {
        r = cdrom_exec_cmd(CMD_INIT, NULL);
        if(r == 0)
            break;
        if(r == ERR_NO_DISC)    {
            return 0;
        }
        else if(r == ERR_SYS)   {
            return -1;
        }

        /* Still trying.. sleep a bit and check again */
        thd_sleep(20);
        timeout--;
    }
    if(timeout <= 0)    {
        return -1;
    }

    /* Check disc type and set parameters */
    params[0] = 0;              /* 0 = set, 1 = get */
    params[1] = 4096;           /* Magic value for RAW reads */
    params[2] = 0x0400;         /* ditto */
    params[3] = 2352;           /* sector size (not really on RAW though?) */
    if(__gdc_change_data_type(params) < 0)    {
        return -1;
    }

    return 0;
}

s32 DCCDReadTOC(u32 * TOC)  {
    CDROM_TOC dctoc;
    uint32 i;
    int last = 0, rv;

    memset(&dctoc, 0, sizeof(CDROM_TOC));

    rv = cdrom_read_toc(&dctoc, 0);

    if(rv == ERR_DISC_CHG || rv == ERR_NO_DISC)  {
        rv = DCCDInit(NULL);

        if(rv != 0) {
            return 0;
        }
        else    {
            rv = cdrom_read_toc(&dctoc, 0);
            if(rv != ERR_OK)    {
                return 0;
            }
        }
    }

    for(i = 0; i < 99; i++)	{
        TOC[i] = dctoc.entry[i];
        if(dctoc.entry[i] == 0xFFFFFFFF && last == 0)    {
            last = i;
        }
    }

    TOC[99] = dctoc.first;
    TOC[100] = dctoc.last;
    TOC[101] = dctoc.leadout_sector;

    return 0xCC * 2;
}

int DCCDReadSectorFAD(u32 FAD, void *buffer)    {
    int err = cdrom_read_sectors(buffer, FAD, 1);

    if(err == ERR_NO_DISC)  {
        return 0;
    }
    else if(err == ERR_DISC_CHG)    {
        err = DCCDInit(NULL);
        if(err != 0)
            return 0;

        cdrom_read_sectors(buffer, FAD, 1);
    }

    return 1;
}
