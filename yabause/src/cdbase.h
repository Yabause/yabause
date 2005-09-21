/*  Copyright 2004-2005 Theo Berkau
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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef CDBASE_H
#define CDBASE_H

#include <stdio.h>
#include "core.h"

#define CDCORE_DEFAULT -1
#define CDCORE_DUMMY    0
#define CDCORE_ISO      1
#define CDCORE_ARCH     2

typedef struct
{
        int id;
        const char *Name;
        int (*Init)(const char *);
        int (*DeInit)();
        int (*GetStatus)();
        s32 (*ReadTOC)(u32 *TOC);
        int (*ReadSectorFAD)(u32 FAD, void *buffer);
} CDInterface;

int DummyCDInit(const char *);
int DummyCDDeInit();
int DummyCDGetStatus();
s32 DummyCDReadTOC(u32 *);
int DummyCDReadSectorFAD(u32, void *);

extern CDInterface DummyCD;

int ISOCDInit(const char *);
int ISOCDDeInit();
int ISOCDGetStatus();
s32 ISOCDReadTOC(u32 *);
int ISOCDReadSectorFAD(u32, void *);

extern CDInterface ISOCD;

extern CDInterface ArchCD;

#endif
