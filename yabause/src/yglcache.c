/*  Copyright 2005-2006 Guillaume Duhamel
 *  Copyright 2005-2006 Theo Berkau
 *  Copyright 2011-2015 Shinya Miyamoto(devmiyax)

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
#include "yui.h"
#include "vidshared.h"



static u32 YglgetHash(u64 addr)
{
  return ((addr>>4) & HASHSIZE);
}

static YglCacheHash * YglgetNewCash(YglTextureManager * tm) {

  YglCacheHash * rtn;

  if (tm->CashLink_index >= HASHSIZE * 2){
    printf("not enough cash");
    return NULL;
  }
  rtn = &tm->CashLink[tm->CashLink_index];
  tm->CashLink_index++;
  return rtn;
}

int YglIsCached(YglTextureManager * tm, u64 addr, YglCache * c) {

  u32 hashkey;
  hashkey = YglgetHash(addr);  /* get hash */

  if (tm->HashTable[hashkey] == NULL) {  /* Empty Hash */
    return 0;        /* Not Found */
  }
  else {  /* needs liner search */
	  YglCacheHash *at = tm->HashTable[hashkey];
    while (at != NULL) {
      if (at->addr == addr) {  /* Find! */
        c->x = at->x;
        c->y = at->y;
        return 1;
      }
      at = at->next;
    }
    return 0;  /* Not found */
  }

  return 1;
}

//////////////////////////////////////////////////////////////////////////////

void YglCacheAdd(YglTextureManager * tm, u64 addr, YglCache * c) {

  u32 hashkey;
  YglCacheHash *add;
  hashkey = YglgetHash(addr);

  if (tm->HashTable[hashkey] == NULL){
	add = YglgetNewCash(tm);
    add->addr = addr;
    add->x = c->x;
    add->y = c->y;
    add->next = NULL;
	tm->HashTable[hashkey] = add;
  }
  else{
	  YglCacheHash *at = tm->HashTable[hashkey];
    while (at != NULL) {
      if (at->addr == addr  ) {
        at->addr = addr;
        at->x = c->x;
        at->y = c->y;
        return;
      }
      at = at->next;
    }

	add = YglgetNewCash(tm);
    add->addr = addr;
    add->x = c->x;
    add->y = c->y;
	add->next = tm->HashTable[hashkey];
	tm->HashTable[hashkey] = add;
  }

}

//////////////////////////////////////////////////////////////////////////////

void YglCacheReset(YglTextureManager * tm) {
	memset(tm->HashTable, 0, sizeof(tm->HashTable));
	tm->CashLink_index = 0;
}

//////////////////////////////////////////////////////////////////////////////
