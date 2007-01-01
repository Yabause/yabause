/*  Copyright 2006 Theo Berkau

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

#include "memory.h"
#include "cs0.h"
#include "debug.h"
#include "sh2core.h"

static u8 sh2masklist[0x20] = {
0xF0, 0xE0, 0xD0, 0xC0, 0xB0, 0xA0, 0x90, 0x80,
0x80, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 
0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 
0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70
};

static u32 scumasklist[0x20] = {
0xFFFFFFFF, 0xFFFFFFFE, 0xFFFFFFFC, 0xFFFFFFF8,
0xFFFFFFF0, 0xFFFFFFE0, 0xFFFFFFC0, 0xFFFFFF80,
0xFFFFFF80, 0xFFFFFE00, 0xFFFFFE00, 0xFFFFFE00,
0xFFFFFE00, 0xFFFFFE00, 0xFFFFFE00, 0xFFFFFE00,
0xFFFFFE00, 0xFFFFFE00, 0xFFFFFE00, 0xFFFFFE00,
0xFFFFFE00, 0xFFFFFE00, 0xFFFFFE00, 0xFFFFFE00,
0xFFFFFE00, 0xFFFFFE00, 0xFFFFFE00, 0xFFFFFE00,
0xFFFFFE00, 0xFFFFFE00, 0xFFFFFE00, 0xFFFFFE00
};

u32 interruptlist[2][0x80];

void SmpcCKCHG320();
void SmpcCKCHG352();

//////////////////////////////////////////////////////////////////////////////

void BiosInit(void)
{
   int i;

   // Setup vectors
   MappedMemoryWriteLong(0x06000600, 0x002B0009); // rte, nop
   MappedMemoryWriteLong(0x06000604, 0xE0F0600C); // mov #0xF0, r0; extu.b r0, r0
   MappedMemoryWriteLong(0x06000608, 0x400E8BFE); // ldc r0, sr; bf
   MappedMemoryWriteLong(0x0600060C, 0x00090009); // nop
   MappedMemoryWriteLong(0x06000610, 0x000B0009); // rts, nop

   for (i = 0; i < 0x200; i+=4)
   {
      MappedMemoryWriteLong(0x06000000+i, 0x06000600);
      MappedMemoryWriteLong(0x06000400+i, 0x06000600);
      interruptlist[0][i >> 2] = 0x06000600;
      interruptlist[1][i >> 2] = 0x06000600;
   }

   MappedMemoryWriteLong(0x06000010, 0x06000604);
   MappedMemoryWriteLong(0x06000018, 0x06000604);
   MappedMemoryWriteLong(0x06000024, 0x06000604);
   MappedMemoryWriteLong(0x06000028, 0x06000604);
   interruptlist[0][4] = 0x06000604;
   interruptlist[0][6] = 0x06000604;
   interruptlist[0][9] = 0x06000604;
   interruptlist[0][10] = 0x06000604;

   MappedMemoryWriteLong(0x06000410, 0x06000604);
   MappedMemoryWriteLong(0x06000418, 0x06000604);
   MappedMemoryWriteLong(0x06000424, 0x06000604);
   MappedMemoryWriteLong(0x06000428, 0x06000604);
   interruptlist[1][4] = 0x06000604;
   interruptlist[1][6] = 0x06000604;
   interruptlist[1][9] = 0x06000604;
   interruptlist[1][10] = 0x06000604;

   // Scu Interrupts
   for (i = 0; i < 0x38; i+=4)
   {
      MappedMemoryWriteLong(0x06000100+i, 0x00000400+i);
      interruptlist[0][0x40+(i >> 2)] = 0x00000400+i;
   }

   for (i = 0; i < 0x40; i+=4)
   {
      MappedMemoryWriteLong(0x06000140+i, 0x00000440+i);
      interruptlist[0][0x50+(i >> 2)] = 0x00000440+i;
   }

   for (i = 0; i < 0x100; i+=4)
      MappedMemoryWriteLong(0x06000A00+i, 0x06000610);

   // Setup Bios Functions
   MappedMemoryWriteLong(0x06000210, 0x00000210);
   MappedMemoryWriteLong(0x0600026C, 0x0000026C);
   MappedMemoryWriteLong(0x06000274, 0x00000274);
   MappedMemoryWriteLong(0x06000280, 0x00000280);
   MappedMemoryWriteLong(0x06000300, 0x00000300);
   MappedMemoryWriteLong(0x06000304, 0x00000304);
   MappedMemoryWriteLong(0x06000310, 0x00000310);
   MappedMemoryWriteLong(0x06000314, 0x00000314);
   MappedMemoryWriteLong(0x06000320, 0x00000320);
   MappedMemoryWriteLong(0x06000324, 0x00000000);
   MappedMemoryWriteLong(0x06000330, 0x00000330);
   MappedMemoryWriteLong(0x06000334, 0x00000334);
   MappedMemoryWriteLong(0x06000340, 0x00000340);
   MappedMemoryWriteLong(0x06000344, 0x00000344);
   MappedMemoryWriteLong(0x06000348, 0xFFFFFFFF);
   MappedMemoryWriteLong(0x06000354, 0x00000000);
   MappedMemoryWriteLong(0x06000358, 0x00000358);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BiosSetScuInterrupt(SH2_struct * sh)
{
//   LOG("BiosSetScuInterrupt. vector = %02X, func = %08X\n", sh->regs.R[4], sh->regs.R[5]);

   if (sh->regs.R[5] == 0)
   {
      MappedMemoryWriteLong(0x06000900+(sh->regs.R[4] << 2), 0x06000610);      
      sh->cycles += 8;
   }
   else
   {
      MappedMemoryWriteLong(0x06000900+(sh->regs.R[4] << 2), sh->regs.R[5]);
      sh->cycles += 9;
   }

   sh->regs.PC = sh->regs.PR;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BiosGetScuInterrupt(SH2_struct * sh)
{
   // check me
//   LOG("BiosGetScuInterrupt\n"); 

   sh->regs.R[0] = MappedMemoryReadLong(0x06000900+(sh->regs.R[4] << 2));
   sh->cycles += 5;

   sh->regs.PC = sh->regs.PR;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BiosSetSh2Interrupt(SH2_struct * sh)
{
//   LOG("BiosSetSh2Interrupt\n");

   if (sh->regs.R[5] == 0)
   {            
      MappedMemoryWriteLong(sh->regs.VBR+(sh->regs.R[4] << 2), interruptlist[sh->isslave][sh->regs.R[4]]);
      sh->cycles += 8;
   }
   else
   {
      MappedMemoryWriteLong(sh->regs.VBR+(sh->regs.R[4] << 2), sh->regs.R[5]);
      sh->cycles += 9;
   }

   sh->regs.PC = sh->regs.PR;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BiosGetSh2Interrupt(SH2_struct * sh)
{
   // check me
//   LOG("BiosGetSh2Interrupt\n");

   sh->regs.R[0] = MappedMemoryReadLong(sh->regs.VBR+(sh->regs.R[4] << 2));
   sh->cycles += 5;

   sh->regs.PC = sh->regs.PR;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BiosSetScuInterruptMask(SH2_struct * sh)
{
   // check me
   LOG("BiosSetScuInterruptMask\n");

   MappedMemoryWriteLong(0x06000348, sh->regs.R[4]);
   MappedMemoryWriteLong(0x25FE00A0, sh->regs.R[4]); // Interrupt Mask Register

   if (!(sh->regs.R[4] & 0x8000)) // double check this
      MappedMemoryWriteLong(0x25FE00A8, 1); // A-bus Interrupt Acknowledge

   sh->cycles += 17;

   sh->regs.PC = sh->regs.PR;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BiosChangeScuInterruptMask(SH2_struct * sh)
{
   u32 newmask;

//   LOG("BiosChangeScuInterruptMask\n");

   // Read Stored Scu Interrupt Mask, AND it by R4, OR it by R5, then put it back
   newmask = (MappedMemoryReadLong(0x06000348) & sh->regs.R[4]) | sh->regs.R[5];
   MappedMemoryWriteLong(0x06000348, newmask);
   MappedMemoryWriteLong(0x25FE00A0, newmask); // Interrupt Mask Register
   MappedMemoryWriteLong(0x25FE00A4, (u32)(s16)sh->regs.R[4]); // Interrupt Status Register

   if (!(sh->regs.R[4] & 0x8000)) // double check this
      MappedMemoryWriteLong(0x25FE00A8, 1); // A-bus Interrupt Acknowledge

   sh->cycles += 20;

   sh->regs.PC = sh->regs.PR;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BiosGetSemaphore(SH2_struct * sh)
{
   u8 temp;

   // check me
   LOG("BiosGetSemaphore\n");
  
   if ((temp = MappedMemoryReadByte(0x06000B00 + sh->regs.R[4])) == 0)
      sh->regs.R[0] = 1;
   else
      sh->regs.R[0] = 0;

   temp |= 0x80;
   MappedMemoryWriteByte(0x06000B00 + sh->regs.R[4], temp);
   
   sh->cycles += 11;
   sh->regs.PC = sh->regs.PR;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BiosClearSemaphore(SH2_struct * sh)
{
   // check me
   LOG("BiosClearSemaphore\n");

   MappedMemoryWriteByte(0x06000B00 + sh->regs.R[4], 0);

   sh->cycles += 5;
   sh->regs.PC = sh->regs.PR;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BiosChangeSystemClock(SH2_struct * sh)
{
   // check me
   LOG("BiosChangeSystemClock\n");

   MappedMemoryWriteLong(0x06000324, sh->regs.R[4]);

   if (sh->regs.R[4] == 0)
      SmpcCKCHG320();
   else
      SmpcCKCHG352();
  
   sh->regs.PC = sh->regs.PR;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BiosChangeScuInterruptPriority(SH2_struct * sh)
{
   int i;

   // check me
//   LOG("BiosChangeScuInterruptPriority\n");

   for (i = 0; i < 0x20; i++)
   {
      scumasklist[i] = MappedMemoryReadLong(sh->regs.R[4]+(i << 2));
      sh2masklist[i] = (scumasklist[i] >> 16);
      if (scumasklist[i] & 0x8000)
         scumasklist[i] |= 0xFFFF0000;
      else
         scumasklist[i] &= 0x0000FFFF;
   }

   sh->cycles += 186;
   sh->regs.PC = sh->regs.PR;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BiosExecuteCDPlayer(SH2_struct * sh)
{
   LOG("BiosExecuteCDPlayer\n");

   sh->regs.PC = sh->regs.PR;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BiosPowerOnMemoryClear(SH2_struct * sh)
{
   LOG("BiosPowerOnMemoryClear\n");

   sh->regs.PC = sh->regs.PR;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BiosCheckMPEGCard(SH2_struct * sh)
{
   LOG("BiosCheckMPEGCard\n");

   sh->regs.PC = sh->regs.PR;
}

//////////////////////////////////////////////////////////////////////////////

u32 GetDeviceStats(u32 device, u32 *size, u32 *addr, u32 *blocksize)
{
   switch(device)
   {
      case 0:
         *addr = 0x00180000;
         *size = 0x8000;
         *blocksize = 0x40;
         return 0;
      case 1:
         if ((CartridgeArea->cartid & 0xF0) == 0x20)
         {
            *addr = 0x04000000;
            *size = 0x40000 << (CartridgeArea->cartid & 0x0F);
            if (CartridgeArea->cartid == 0x24)
               *blocksize = 0x400;
            else
               *blocksize = 0x200;

            return 0;
         }
         else
            return 1;
      default:
         *addr = 0;
         *size = 0;
         *blocksize = 0;
         return 1;
   }

   return 1;
}

//////////////////////////////////////////////////////////////////////////////

int CheckHeader(u32 device)
{
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

u32 GetFreeSpace(u32 device, u32 size, u32 addr, u32 blocksize)
{
   int i, i2;
   u32 usedblocks=0;

   for (i = ((2 * blocksize) << 1); i < (size << 1); i += (blocksize << 1))
   {
      // Find a block with the start of a save
      if (((s8)MappedMemoryReadByte(addr + i + 1)) < 0)
      {
         i2 = addr+i+0x45;
         usedblocks++;

         // Now figure out how many blocks this save is
         for(;;)
         {
            u16 block;
            block = (MappedMemoryReadByte(i2) << 8) | MappedMemoryReadByte(i2 + 2);
            if (block == 0)
               break;
            i2 += 4;
            if (((i2-1) & ((blocksize << 1) - 1)) == 0)
               i2 += 8;
            usedblocks++;
         }
      }
   }

   return ((size / blocksize) - 2 - usedblocks);
}

//////////////////////////////////////////////////////////////////////////////

u32 FindSave(u32 device, u32 stringaddr, u32 blockoffset, u32 size, u32 addr, u32 blocksize)
{
   u32 i;
   int findmatch = MappedMemoryReadByte(stringaddr);

   for (i = ((blockoffset * blocksize) << 1); i < (size << 1); i += (blocksize << 1))
   {
      // Find a block with the start of a save
      if (((s8)MappedMemoryReadByte(addr + i + 1)) < 0)
      {
         int i3;

         // See if string matches, or if there's no string to check, just copy
         // the data over
         for (i3 = 0; i3 < 11; i3++)
         {
            u8 data = MappedMemoryReadByte(stringaddr+i3);
                
            if (MappedMemoryReadByte(addr+i+0x9+(i3*2)) != data)
            {
               if (data == 0 && findmatch == 0)
               {
                  // There's no string to match
                  return ((i / blocksize) >> 1);
               }
               else
                  // No Match, move onto the next block
                  i3 = 12;
            }
            else
            {
               // Match
               if (i3 == 10)
                  return ((i / blocksize) >> 1);
            }
         }
      }
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void DeleteSave(u32 blockoffset)
{
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BiosBUPInit(SH2_struct * sh)
{
   LOG("BiosBUPInit. arg1 = %08X, arg2 = %08X, arg3 = %08X\n", sh->regs.R[4], sh->regs.R[5], sh->regs.R[6]);

   // Setup Function table
   MappedMemoryWriteLong(0x06000354, sh->regs.R[5]);
   MappedMemoryWriteLong(sh->regs.R[5]+0x00, 0x00000380);
   MappedMemoryWriteLong(sh->regs.R[5]+0x04, 0x00000384);
   MappedMemoryWriteLong(sh->regs.R[5]+0x08, 0x00000388);
   MappedMemoryWriteLong(sh->regs.R[5]+0x0C, 0x0000038C);
   MappedMemoryWriteLong(sh->regs.R[5]+0x10, 0x00000390);
   MappedMemoryWriteLong(sh->regs.R[5]+0x14, 0x00000394);
   MappedMemoryWriteLong(sh->regs.R[5]+0x18, 0x00000398);
   MappedMemoryWriteLong(sh->regs.R[5]+0x1C, 0x0000039C);
   MappedMemoryWriteLong(sh->regs.R[5]+0x20, 0x000003A0);
   MappedMemoryWriteLong(sh->regs.R[5]+0x24, 0x000003A4);
   MappedMemoryWriteLong(sh->regs.R[5]+0x28, 0x000003A8);
   MappedMemoryWriteLong(sh->regs.R[5]+0x2C, 0x000003AC);

   // Setup Device list

   // First Device
   MappedMemoryWriteWord(sh->regs.R[6], 1); // ID
   MappedMemoryWriteWord(sh->regs.R[6]+0x2, 1); // Number of partitions

   // Second Device
   if ((CartridgeArea->cartid & 0xF0) == 0x20)
   {
      MappedMemoryWriteWord(sh->regs.R[6]+0x4, 2); // ID
      MappedMemoryWriteWord(sh->regs.R[6]+0x6, 1); // Number of partitions
   }
   else
   {
      MappedMemoryWriteWord(sh->regs.R[6]+0x4, 0); // ID
      MappedMemoryWriteWord(sh->regs.R[6]+0x6, 0); // Number of partitions
   }

   // Third Device
   MappedMemoryWriteWord(sh->regs.R[6]+0x08, 0); // ID
   MappedMemoryWriteWord(sh->regs.R[6]+0x0A, 0); // Number of partitions

   // cycles need to be incremented

   sh->regs.PC = sh->regs.PR;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BiosBUPSelectPartition(SH2_struct * sh)
{
   LOG("BiosBUPSelectPartition. PR = %08X\n", sh->regs.PR);

   sh->regs.R[0] = 0; // returns 0 if there's no error
   sh->regs.PC = sh->regs.PR;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BiosBUPFormat(SH2_struct * sh)
{
   LOG("BiosBUPFormat. PR = %08X\n", sh->regs.PR);

   switch (sh->regs.R[4])
   {
      case 0:
         FormatBackupRam(BupRam, 0x10000);
         break;
      case 1:
         if ((CartridgeArea->cartid & 0xF0) == 0x20)
         {
            switch (CartridgeArea->cartid & 0xF)
            {
               case 1:
                  FormatBackupRam(CartridgeArea->bupram, 0x100000);
                  break;
               case 2:
                  FormatBackupRam(CartridgeArea->bupram, 0x200000);
                  break;
               case 3:
                  FormatBackupRam(CartridgeArea->bupram, 0x400000);
                  break;
               case 4:
                  FormatBackupRam(CartridgeArea->bupram, 0x800000);
                  break;
               default: break;
            }
         }
         break;
      case 2:
      default: break;
   }

   sh->regs.R[0] = 0; // returns 0 if there's no error
   sh->regs.PC = sh->regs.PR;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BiosBUPStatus(SH2_struct * sh)
{
   u32 size;
   u32 addr;
   u32 blocksize;
   u32 ret;
   u32 freeblocks=0;

   LOG("BiosBUPStatus. arg1 = %d, arg2 = %d, arg3 = %08X, PR = %08X\n", sh->regs.R[4], sh->regs.R[5], sh->regs.R[6], sh->regs.PR);

   // Fill in status variables
   ret = GetDeviceStats(sh->regs.R[4], &size, &addr, &blocksize);

   // Make sure there's a proper header, and return if there's any other errors
   if (ret == 1 || CheckHeader(sh->regs.R[4]) != 0)
   {
      // Error
      sh->regs.R[0] = ret;
      sh->regs.PC = sh->regs.PR;
      return;
   }

   freeblocks = GetFreeSpace(sh->regs.R[4], size, addr, blocksize);

   MappedMemoryWriteLong(sh->regs.R[6], size); // Size of Backup Ram (in bytes)
   MappedMemoryWriteLong(sh->regs.R[6]+0x4, size / blocksize); // Size of Backup Ram (in blocks)
   MappedMemoryWriteLong(sh->regs.R[6]+0x8, blocksize); // Size of block
   MappedMemoryWriteLong(sh->regs.R[6]+0xC, ((blocksize - 6) * freeblocks) - 30); // Free space(in bytes)
   MappedMemoryWriteLong(sh->regs.R[6]+0x10, freeblocks); // Free space(in blocks)
   MappedMemoryWriteLong(sh->regs.R[6]+0x14, freeblocks); // Not sure, but seems to be the same as Free Space(in blocks)

   // cycles need to be incremented

   sh->regs.R[0] = ret; // returns 0 if there's no error
   sh->regs.PC = sh->regs.PR;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BiosBUPWrite(SH2_struct * sh)
{
   u32 size;
   u32 addr;
   u32 blocksize;
   u32 block;
   u32 ret;
   u32 savesize;

   LOG("BiosBUPWrite. arg1 = %d, arg2 = %08X, arg3 = %08X, arg4 = %d, PR = %08X\n", sh->regs.R[4], sh->regs.R[5], sh->regs.R[6], sh->regs.R[7], sh->regs.PR);

   // Fill in status variables
   ret = GetDeviceStats(sh->regs.R[4], &size, &addr, &blocksize);

   // See if save exists already
   if ((block = FindSave(sh->regs.R[4], sh->regs.R[5], 2, size, addr, blocksize)) != 0)
   {
      // save exists

      // Should we be overwriting it?
      if (sh->regs.R[7] != 0)
      {
         // Nope, let's bail instead
         sh->regs.R[0] = 6;
         sh->regs.PC = sh->regs.PR;
         return;
      }

      // Delete old save
      DeleteSave(block);
   }

   // Let's figure out how many blocks will be needed for the save
   savesize = MappedMemoryReadLong(sh->regs.R[5]+0x1C);
   savesize += 0x1D;
   savesize = savesize / (blocksize - 6);

   // Will it blend? Err... fit
   if (savesize > GetFreeSpace(sh->regs.R[4], size, addr, blocksize))
   {
      // Nope, time to bail
      sh->regs.R[0] = 4;
      sh->regs.PC = sh->regs.PR;
      return;
   }

   // Find free blocks for the save here

   // Create save here

   sh->regs.R[0] = 0; // returns 0 if there's no error
   sh->regs.PC = sh->regs.PR;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BiosBUPRead(SH2_struct * sh)
{
   u32 size;
   u32 addr;
   u32 blocksize;
   u32 block;
   u32 blockoffset;
   u32 ret;
   u32 i;
   u32 tableaddr;

   LOG("BiosBUPRead. PR = %08X\n", sh->regs.PR);

   ret = GetDeviceStats(sh->regs.R[4], &size, &addr, &blocksize);

   if (ret == 1)
   {
      // Error
      sh->regs.R[0] = ret;
      sh->regs.PC = sh->regs.PR;
      return;
   }

   // See if save exists
   if ((block = FindSave(sh->regs.R[4], sh->regs.R[5], 2, size, addr, blocksize)) == 0)
   {
      // save doesn't exist
      sh->regs.R[0] = 5;
      sh->regs.PC = sh->regs.PR;
      return;
   }

   // Read in save here
   tableaddr = addr + (block * blocksize) + 0x45;

   sh->regs.R[0] = 0; // returns 0 if there's no error
   sh->regs.PC = sh->regs.PR;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BiosBUPDelete(SH2_struct * sh)
{
   u32 size;
   u32 addr;
   u32 blocksize;
   u32 block;
   u32 ret;

   LOG("BiosBUPDelete. PR = %08X\n", sh->regs.PR);

   // Fill in status variables
   ret = GetDeviceStats(sh->regs.R[4], &size, &addr, &blocksize);

   // See if save exists
   if ((block = FindSave(sh->regs.R[4], sh->regs.R[5], 2, size, addr, blocksize)) == 0)
   {
      // Since the save doesn't exist, let's bail with an error

      sh->regs.R[0] = 5;
      sh->regs.PC = sh->regs.PR;
      return;
   }

   DeleteSave(block);

   sh->regs.R[0] = 0; // returns 0 if there's no error
   sh->regs.PC = sh->regs.PR;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BiosBUPDirectory(SH2_struct * sh)
{
   u32 size;
   u32 addr;
   u32 blocksize;
   u32 ret;
   int i, i2=0;
   char filename[12];
   u32 blockoffset=2;

   int findmatch = MappedMemoryReadByte(sh->regs.R[5]);

   for (i = 0; i < 12; i++)
      filename[i] = MappedMemoryReadByte(sh->regs.R[5]+i);

   LOG("BiosBUPDirectory. arg1 = %d, arg2 = %s, arg3 = %08X, arg4 = %08X, PR = %08X\n", sh->regs.R[4], filename, sh->regs.R[6], sh->regs.R[7], sh->regs.PR);

   ret = GetDeviceStats(sh->regs.R[4], &size, &addr, &blocksize);

   if (ret == 1)
   {
      // Error
      sh->regs.R[0] = ret;
      sh->regs.PC = sh->regs.PR;
      return;
   }

   for (i = 0; i < sh->regs.R[6]; i++)
   {
      int i4;
      u32 datasize=0;
      u32 block = FindSave(sh->regs.R[4], sh->regs.R[5], blockoffset, size, addr, blocksize);

      if (block == 0)
         break;

      blockoffset = block+1;

      // Alright, we found a match :) Time to copy over some data
      block = addr + (blocksize * block * 2);

      // Copy over filename
      for (i4 = block+0x9; i4 < ((block+0x9) + (11 * 2)); i4+=2)
      {
         MappedMemoryWriteByte(sh->regs.R[7], MappedMemoryReadByte(i4));
         sh->regs.R[7]++;
      }
      MappedMemoryWriteByte(sh->regs.R[7], 0);
      sh->regs.R[7]++;

      // Copy over comment
      for (i4 = block+0x21; i4 < ((block+0x21) + (10 * 2)); i4+=2)
      {
         MappedMemoryWriteByte(sh->regs.R[7], MappedMemoryReadByte(i4));
         sh->regs.R[7]++;
      }

      // Copy over language
      MappedMemoryWriteByte(sh->regs.R[7], MappedMemoryReadByte(block+0x1F));
      sh->regs.R[7]++;

      MappedMemoryWriteByte(sh->regs.R[7], 0);
      sh->regs.R[7]++;

      // Copy over date
      for (i4 = block+0x35; i4 < ((block+0x35) + (4 * 2)); i4+=2)
      {
         MappedMemoryWriteByte(sh->regs.R[7], MappedMemoryReadByte(i4));
         sh->regs.R[7]++;
      }

      // Copy over data size
      for (i4 = block+0x3D; i4 < ((block+0x3D) + (4 * 2)); i4+=2)
      {
         u8 data;
         datasize <<= 8;
         data = MappedMemoryReadByte(i4);
         MappedMemoryWriteByte(sh->regs.R[7], data);
         datasize |= data;
         sh->regs.R[7]++;
      }

      // Calculate block size from the data size, and then copy it over
      MappedMemoryWriteWord(sh->regs.R[7], ((datasize + 0x1D) / (blocksize - 6)) + 1);
      sh->regs.R[7] += 4;
   }

   sh->regs.R[0] = i; // returns the number of successfully read dir entries
   sh->regs.PC = sh->regs.PR;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BiosBUPVerify(SH2_struct * sh)
{
   u32 size;
   u32 addr;
   u32 blocksize;
   u32 block;
   u32 ret;

   LOG("BiosBUPVerify. PR = %08X\n", sh->regs.PR);

   ret = GetDeviceStats(sh->regs.R[4], &size, &addr, &blocksize);

   if (ret == 1)
   {
      // Error
      sh->regs.R[0] = ret;
      sh->regs.PC = sh->regs.PR;
      return;
   }

   // See if save exists
   if ((block = FindSave(sh->regs.R[4], sh->regs.R[5], 2, size, addr, blocksize)) == 0)
   {
      // Since the save doesn't exist, let's bail with an error
      sh->regs.R[0] = 5;
      sh->regs.PC = sh->regs.PR;
      return;
   }

   // Verify Save here

   sh->regs.R[0] = 0; // returns 0 if there's no error
   sh->regs.PC = sh->regs.PR;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BiosBUPGetDate(SH2_struct * sh)
{
   LOG("BiosBUPGetDate. PR = %08X\n", sh->regs.PR);

   sh->regs.PC = sh->regs.PR;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BiosBUPSetDate(SH2_struct * sh)
{
   u32 date;
   u8 data;

   LOG("BiosBUPSetDate. PR = %08X\n", sh->regs.PR);
   // Year
   data = MappedMemoryReadByte(sh->regs.R[4]);
   date = (data / 4) * 0x5B5;
   if (data % 4)
      date += ((data % 4) * 0x16D) + 1;

   // Month
   data = MappedMemoryReadByte(sh->regs.R[4]+1);
   if (data != 1 && data < 13)
   {
      // fix me
   }

   // Day
   date += MappedMemoryReadByte(sh->regs.R[4]+2) - 1;
   date *= 0x5A0;

   // Hour
   date += (MappedMemoryReadByte(sh->regs.R[4]+3) * 0x3C);

   // Minute
   date += MappedMemoryReadByte(sh->regs.R[4]+4);

   sh->regs.R[0] = date;
   sh->regs.PC = sh->regs.PR;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BiosHandleScuInterrupt(SH2_struct * sh, int vector)
{
   // Save R0-R7, PR, GBR, and old Interrupt mask to stack
   sh->regs.R[15] -= 4;
   MappedMemoryWriteLong(sh->regs.R[15], sh->regs.R[0]);
   sh->regs.R[15] -= 4;
   MappedMemoryWriteLong(sh->regs.R[15], sh->regs.R[1]);
   sh->regs.R[15] -= 4;
   MappedMemoryWriteLong(sh->regs.R[15], sh->regs.R[2]);
   sh->regs.R[15] -= 4;
   MappedMemoryWriteLong(sh->regs.R[15], sh->regs.R[3]);
   sh->regs.R[15] -= 4;
   MappedMemoryWriteLong(sh->regs.R[15], MappedMemoryReadLong(0x06000348));
   sh->regs.R[15] -= 4;
   MappedMemoryWriteLong(sh->regs.R[15], sh->regs.R[4]);
   sh->regs.R[15] -= 4;
   MappedMemoryWriteLong(sh->regs.R[15], sh->regs.R[5]);
   sh->regs.R[15] -= 4;
   MappedMemoryWriteLong(sh->regs.R[15], sh->regs.R[6]);
   sh->regs.R[15] -= 4;
   MappedMemoryWriteLong(sh->regs.R[15], sh->regs.R[7]);
   sh->regs.R[15] -= 4;
   MappedMemoryWriteLong(sh->regs.R[15], sh->regs.PR);
   sh->regs.R[15] -= 4;
   MappedMemoryWriteLong(sh->regs.R[15], sh->regs.GBR);

   // Set SR according to vector
   sh->regs.SR.all = (u32)sh2masklist[vector - 0x40];

   // Write new Interrupt mask value
   MappedMemoryWriteLong(0x06000348, scumasklist[vector - 0x40]);
   MappedMemoryWriteLong(0x25FE00A0, scumasklist[vector - 0x40]);

   // Set PR to our Interrupt Return handler
   sh->regs.PR = 0x00000480;

   // Now execute the interrupt
   sh->regs.PC = MappedMemoryReadLong(0x06000900+(vector << 2));
//   LOG("Interrupt PC = %08X. Read from %08X\n", sh->regs.PC, 0x06000900+(vector << 2));

   sh->cycles += 33;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BiosHandleScuInterruptReturn(SH2_struct * sh)
{
   u32 oldmask;

   // Restore R0-R7, PR, GBR, and old Interrupt mask from stack
   sh->regs.GBR = MappedMemoryReadLong(sh->regs.R[15]);
   sh->regs.R[15] += 4;
   sh->regs.PR = MappedMemoryReadLong(sh->regs.R[15]);
   sh->regs.R[15] += 4;
   sh->regs.R[7] = MappedMemoryReadLong(sh->regs.R[15]);
   sh->regs.R[15] += 4;
   sh->regs.R[6] = MappedMemoryReadLong(sh->regs.R[15]);
   sh->regs.R[15] += 4;
   sh->regs.R[5] = MappedMemoryReadLong(sh->regs.R[15]);
   sh->regs.R[15] += 4;
   sh->regs.R[4] = MappedMemoryReadLong(sh->regs.R[15]);
   sh->regs.R[15] += 4;
   oldmask = MappedMemoryReadLong(sh->regs.R[15]);
   MappedMemoryWriteLong(0x06000348, oldmask);
   MappedMemoryWriteLong(0x25FE00A0, oldmask);
   sh->regs.R[15] += 4;
   sh->regs.R[3] = MappedMemoryReadLong(sh->regs.R[15]);
   sh->regs.R[15] += 4;
   sh->regs.R[2] = MappedMemoryReadLong(sh->regs.R[15]);
   sh->regs.R[15] += 4;
   sh->regs.R[1] = MappedMemoryReadLong(sh->regs.R[15]);
   sh->regs.R[15] += 4;
   sh->regs.R[0] = MappedMemoryReadLong(sh->regs.R[15]);
   sh->regs.R[15] += 4;

   // Return SR back to normal
   sh->regs.SR.all = 0xF0;

   sh->regs.PC = MappedMemoryReadLong(sh->regs.R[15]);
   sh->regs.R[15] += 4;
   sh->regs.SR.all = MappedMemoryReadLong(sh->regs.R[15]) & 0x000003F3;
   sh->regs.R[15] += 4;

   sh->cycles += 24;
}

//////////////////////////////////////////////////////////////////////////////

int FASTCALL BiosHandleFunc(SH2_struct * sh)
{
   // Let's see if it's a bios function
   switch((sh->regs.PC - 0x200) >> 2)
   {
      case 0x04: // 0x06000210
         BiosPowerOnMemoryClear(sh);
         break;
      case 0x1B: // 0x0600026C
         BiosExecuteCDPlayer(sh);
         break;
      case 0x1D: // 0x06000274
         BiosCheckMPEGCard(sh);
         break;
      case 0x20:  // 0x06000280
         BiosChangeScuInterruptPriority(sh);
         break;
      case 0x40: // 0x06000300
         BiosSetScuInterrupt(sh);
         break;
      case 0x41: // 0x06000304
         BiosGetScuInterrupt(sh);
         break;
      case 0x44: // 0x06000310
         BiosSetSh2Interrupt(sh);
         break;
      case 0x45: // 0x06000314
         BiosGetSh2Interrupt(sh);
         break;
      case 0x48: // 0x06000320
         BiosChangeSystemClock(sh);
         break;
      case 0x4C: // 0x06000330
         BiosGetSemaphore(sh);
         break;
      case 0x4D: // 0x06000334
         BiosClearSemaphore(sh);
         break;
      case 0x50: // 0x06000340
         BiosSetScuInterruptMask(sh);
         break;
      case 0x51: // 0x06000344
         BiosChangeScuInterruptMask(sh);
         break;
      case 0x56: // 0x06000358
         BiosBUPInit(sh);
         break;
      case 0x60: // 0x06000380
         break;
      case 0x61: // 0x06000384
         BiosBUPSelectPartition(sh);
         break;
      case 0x62: // 0x06000388
         BiosBUPFormat(sh);
         break;
      case 0x63: // 0x0600038C
         BiosBUPStatus(sh);
         break;
      case 0x64: // 0x06000390
         BiosBUPWrite(sh);
         break;
      case 0x65: // 0x06000394
         BiosBUPRead(sh);
         break;
      case 0x66: // 0x06000398
         BiosBUPDelete(sh);
         break;
      case 0x67: // 0x0600039C
         BiosBUPDirectory(sh);
         break;
      case 0x68: // 0x060003A0
         BiosBUPVerify(sh);
         break;
      case 0x69: // 0x060003A4
         BiosBUPGetDate(sh);
         break;
      case 0x6A: // 0x060003A8
         BiosBUPSetDate(sh);
         break;
      case 0x6B: // 0x060003AC
         break;
      case 0x80: // Interrupt Handler
      case 0x81:
      case 0x82:
      case 0x83:
      case 0x84:
      case 0x85:
      case 0x86:
      case 0x87:
      case 0x88:
      case 0x89:
      case 0x8A:
      case 0x8B:
      case 0x8C:
      case 0x8D:
      case 0x90:
      case 0x91:
      case 0x92:
      case 0x93:
      case 0x94:
      case 0x95:
      case 0x96:
      case 0x97:
      case 0x98:
      case 0x99:
      case 0x9A:
      case 0x9B:
      case 0x9C:
      case 0x9D:
      case 0x9E:
      case 0x9F:
         BiosHandleScuInterrupt(sh, (sh->regs.PC - 0x300) >> 2);
         break;
      case 0xA0: // Interrupt Handler Return
         BiosHandleScuInterruptReturn(sh);
         break;
      default:
         return 0;
   }

   return 1;
}

//////////////////////////////////////////////////////////////////////////////

