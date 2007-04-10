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

#ifndef CHEAT_H
#define CHEAT_H

#include "core.h"

enum
{
   CHEATTYPE_NONE=0,
   CHEATTYPE_ENABLE,
   CHEATTYPE_BYTEWRITE,
   CHEATTYPE_WORDWRITE,
   CHEATTYPE_LONGWRITE
};

typedef struct
{
   int type;
   u32 addr;
   u32 val;
} cheatlist_struct;

int CheatInit(void);
void CheatDeInit(void);
int CheatAddCode(int type, u32 addr, u32 val);
int CheatAddARCode(const char *code);
int CheatRemoveCode(int type, u32 addr, u32 val);
int CheatRemoveARCode(const char *code);
void CheatDoPatches(void);

#endif
