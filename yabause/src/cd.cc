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

// Generic cd.cc file for those too lazy or busy to implement a proper one

int CDInit(char *cdrom_name)
{
   // Initialization function. cdrom_name can be whatever you want it to be.
   // Obviously with some ports(e.g. the dreamcast port) you probably won't
   // even use it.
   return 0;
}

int CDDeInit()
{
   // Cleanup function. Enough said.
   return 0;
}

bool CDIsCDPresent()
{
   // This is where it checks to see if a disc is present. It should be
   // pretty straight forward. Eventually I may change this so it returns
   // the general drive status.
   return false;
}

long CDReadToc(unsigned long *TOC)
{
   // The format of TOC is as follows:
   // TOC[0] - TOC[98] are meant for tracks 1-99. Each entry has the following
   // format:
   // bits 0 - 23: track FAD address
   // bits 24 - 27: track addr
   // bits 28 - 31: track ctrl
   //
   // Any Unused tracks should be set to 0xFFFFFFFF
   //
   // TOC[99] uses the following format:
   // bits 16 - 23: first track's number
   // bits 24 - 27: first track's addr
   // bits 28 - 31: first track's ctrl
   //
   // TOC[100] uses the following format:
   // bits 16 - 23: last track's number
   // bits 24 - 27: last track's addr
   // bits 28 - 31: last track's ctrl
   //
   // TOC[101] uses the following format:
   // bits 0 - 23: leadout FAD address
   // bits 24 - 27: leadout's addr
   // bits 28 - 31: leadout's ctrl
   //
   // Special Note: To convert from LBA to FAD, add 150.

   return 0;
}

unsigned long CDReadSector(unsigned long lba, unsigned long size, void *buffer)
{
   // lba is obviously LBA, as opposed to FAD.
   // You should return the amount of data you read. Which under normally
   // circumstances should be equal to size. Obviously if there's a read
   // error or something you'd return 0.

   return size;
}

