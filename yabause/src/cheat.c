/*  Copyright 2007 Theo Berkau

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

#include <stdlib.h>
#include "cheat.h"
#include "memory.h"

cheatlist_struct *cheatlist=NULL;
int numcheats=0;
int cheatsize;

//////////////////////////////////////////////////////////////////////////////

int CheatInit(void)
{  
   cheatsize = 10;
   if ((cheatlist = (cheatlist_struct *)malloc(sizeof(cheatlist_struct) * cheatsize)) == NULL)
      return -1;
   cheatlist[0].type = CHEATTYPE_NONE;

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void CheatDeInit(void)
{  
   if (cheatlist)
      free(cheatlist);
}

//////////////////////////////////////////////////////////////////////////////

int CheatAddCode(int type, u32 addr, u32 val)
{
   cheatlist[numcheats].type = type;
   cheatlist[numcheats].addr = addr;
   cheatlist[numcheats].val = val;
   cheatlist[numcheats].desc = NULL;
   cheatlist[numcheats].enable = 1;
   numcheats++;

   // Make sure we still have room
   if (numcheats >= cheatsize)
   {
      cheatlist = realloc(cheatlist, sizeof(cheatlist_struct) * (cheatsize * 2));
      cheatsize *= 2;
   }

   cheatlist[numcheats].type = CHEATTYPE_NONE;

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

int CheatAddARCode(const char *code)
{
   u32 addr;
   u16 val;
   sscanf(code, "%08lX %04hX", &addr, &val);
   switch (addr >> 28)
   {
      case 0x0:
         // One time word write(fix me)
         return -1;
      case 0x1:
         return CheatAddCode(CHEATTYPE_WORDWRITE, addr & 0x0FFFFFFF, val);
      case 0x3:
         return CheatAddCode(CHEATTYPE_BYTEWRITE, addr & 0x0FFFFFFF, val);
      case 0xD:
         return CheatAddCode(CHEATTYPE_ENABLE, addr & 0x0FFFFFFF, val);
      default: return -1;
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

int FindCheat(int type, u32 addr, u32 val)
{
   int i;

   for (i = 0; i < numcheats; i++)
   {
      if (cheatlist[i].type == type &&
          cheatlist[i].addr == addr &&
          cheatlist[i].val == val)
         return i;
   }

   return -1;
}

//////////////////////////////////////////////////////////////////////////////

int CheatChangeDescription(int type, u32 addr, u32 val, char *desc)
{
   int i;

   if ((i = FindCheat(type, addr, val)) == -1)
      // There is no matches, so let's bail
      return -1;

   return CheatChangeDescriptionByIndex(i, desc);
}

//////////////////////////////////////////////////////////////////////////////

int CheatChangeDescriptionByIndex(int i, char *desc)
{
   // Free old description(if existing)
   if (cheatlist[i].desc)
      free(cheatlist[i].desc);

   cheatlist[i].desc = strdup(desc);
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

int CheatRemoveCode(int type, u32 addr, u32 val)
{
   int i;

   if ((i = FindCheat(type, addr, val)) == -1)
      // There is no matches, so let's bail
      return -1;

   return CheatRemoveCodeByIndex(i);
}

//////////////////////////////////////////////////////////////////////////////

int CheatRemoveCodeByIndex(int i)
{
   // If there's a description, free the memory.
   if (cheatlist[i].desc)
      free(cheatlist[i].desc);

   // Move all entries one forward
   for (; i < numcheats-1; i++)
      memcpy(&cheatlist[i], &cheatlist[i+1], sizeof(cheatlist_struct));

   numcheats--;

   // Set the last one to type none
   cheatlist[numcheats].type = CHEATTYPE_NONE;

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

int CheatRemoveARCode(const char *code)
{
   u32 addr;
   u16 val;
   sscanf(code, "%08lX %04hX", &addr, &val);

   switch (addr >> 28)
   {
      case 0x1:
         return CheatRemoveCode(CHEATTYPE_WORDWRITE, addr & 0x0FFFFFFF, val);
      case 0x3:
         return CheatRemoveCode(CHEATTYPE_BYTEWRITE, addr & 0x0FFFFFFF, val);
      case 0xD:
         return CheatRemoveCode(CHEATTYPE_ENABLE, addr & 0x0FFFFFFF, val);
      default: return -1;
   }
}

//////////////////////////////////////////////////////////////////////////////

void CheatClearCodes(void)
{
   cheatlist[0].type = CHEATTYPE_NONE;
   numcheats = 0;
}

//////////////////////////////////////////////////////////////////////////////

void CheatEnableCode(int index)
{
   cheatlist[index].enable = 1;
}

//////////////////////////////////////////////////////////////////////////////

void CheatDisableCode(int index)
{
   cheatlist[index].enable = 0;
}

//////////////////////////////////////////////////////////////////////////////

void CheatDoPatches(void)
{
   int i;

   for (i = 0; ; i++)
   {
      switch (cheatlist[i].type)
      {
         case CHEATTYPE_NONE:
            return;
         case CHEATTYPE_ENABLE:
            if (cheatlist[i].enable == 0)
               continue;
            if (MappedMemoryReadWord(cheatlist[i].addr) != cheatlist[i].val)
               return;
            break;
         case CHEATTYPE_BYTEWRITE:
            if (cheatlist[i].enable == 0)
               continue;
            MappedMemoryWriteByte(cheatlist[i].addr, (u8)cheatlist[i].val);
            break;
         case CHEATTYPE_WORDWRITE:
            if (cheatlist[i].enable == 0)
               continue;
            MappedMemoryWriteWord(cheatlist[i].addr, (u16)cheatlist[i].val);
            break;
         case CHEATTYPE_LONGWRITE:
            if (cheatlist[i].enable == 0)
               continue;
            MappedMemoryWriteLong(cheatlist[i].addr, cheatlist[i].val);
            break;            
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

cheatlist_struct *CheatGetList(int *cheatnum)
{
   if (cheatnum == NULL)
      return NULL;

   *cheatnum = numcheats;
   return cheatlist;
}

//////////////////////////////////////////////////////////////////////////////
