/*  Copyright 2005-2006 Guillaume Duhamel
    Copyright 2005-2006 Theo Berkau
    Copyright 2011 Shinya Miyamoto
    
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

#include "ygl.h"

typedef struct
{
   u32 id;
   YglCache c;
} cache_struct;

static cache_struct *cachelist;
static int cachelistsize=0;

//////////////////////////////////////////////////////////////////////////////

void YglCacheInit() {
   // This is probably wrong, but it'll have to do for now
   if ((cachelist = (cache_struct *)malloc(0x100000 / 8 * sizeof(cache_struct))) == NULL)
      return -1;
}

//////////////////////////////////////////////////////////////////////////////

void YglCacheDeInit() {
   if (cachelist)
      free(cachelist);
}

//////////////////////////////////////////////////////////////////////////////

int YglIsCached(u32 addr, YglCache * c ) {
   int i = 0;

   for (i = 0; i < cachelistsize; i++)
   {
      if (addr == cachelist[i].id)
     {
         c->x=cachelist[i].c.x;
       c->y=cachelist[i].c.y;
       return 1;
      }
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void YglCacheAdd(u32 addr, YglCache * c) {
   cachelist[cachelistsize].id = addr;
   cachelist[cachelistsize].c.x = c->x;
   cachelist[cachelistsize].c.y = c->y;
   cachelistsize++;
}

//////////////////////////////////////////////////////////////////////////////

void YglCacheReset(void) {
   cachelistsize = 0;
}
