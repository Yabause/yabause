/*  Copyright 2003-2005 Guillaume Duhamel
    Copyright 2005 Theo Berkau

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

/*! \file cs1.c
    \brief A-Bus CS1 emulation functions.
*/

#include <stdlib.h>
#include "cs1.h"
#include "cs0.h"

//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL Cs1ReadByte(SH2_struct *context, UNUSED u8* memory, u32 addr)
{
   addr &= 0xFFFFFF;
               
   if (addr == 0xFFFFFF)
      return CartridgeArea->cartid;

   return CartridgeArea->Cs1ReadByte(context, memory, addr);
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL Cs1ReadWord(SH2_struct *context, UNUSED u8* memory, u32 addr)
{
   addr &= 0xFFFFFF;

   if (addr == 0xFFFFFE)
      return (0xFF00 | CartridgeArea->cartid);

   return CartridgeArea->Cs1ReadWord(context, memory, addr);
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL Cs1ReadLong(SH2_struct *context, UNUSED u8* memory, u32 addr)
{
   addr &= 0xFFFFFF;

   if (addr == 0xFFFFFC)
      return (0xFF00FF00 | (CartridgeArea->cartid << 16) | CartridgeArea->cartid);

   return CartridgeArea->Cs1ReadLong(context, memory, addr);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Cs1WriteByte(SH2_struct *context, UNUSED u8* memory, u32 addr, u8 val)
{
   addr &= 0xFFFFFF;

   if (addr == 0xFFFFFF)
      return;

   CartridgeArea->Cs1WriteByte(context, memory, addr, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Cs1WriteWord(SH2_struct *context, UNUSED u8* memory, u32 addr, u16 val)
{
   addr &= 0xFFFFFF;

   if (addr == 0xFFFFFE)
      return;

   CartridgeArea->Cs1WriteWord(context, memory, addr, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Cs1WriteLong(SH2_struct *context, UNUSED u8* memory, u32 addr, u32 val)
{
   addr &= 0xFFFFFF;

   if (addr == 0xFFFFFC)
      return;

   CartridgeArea->Cs1WriteLong(context, memory, addr, val);
}

//////////////////////////////////////////////////////////////////////////////
