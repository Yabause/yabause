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
   return 0;
}

int CDDeInit()
{
   return 0;
}

bool CDIsCDPresent()
{
   return false;
}

long CDReadToc(unsigned long *TOC)
{
   return 0;
}

unsigned long CDReadSector(unsigned long lba, unsigned long size, void *buffer)
{
   return 0;
}

