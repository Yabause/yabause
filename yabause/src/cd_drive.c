/*  Copyright 2014-2016 James Laird-Wah
    Copyright 2004-2006, 2013 Theo Berkau

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

/*! \file cd_drive.c
    \brief CD drive onboard microcontroller HLE
*/

#include "core.h"
#include "cd_drive.h"
#include "sh7034.h"
#include "assert.h"
#include "memory.h"
#include "debug.h"
#include <stdarg.h>

struct CdDriveContext cdd_cxt;

enum CdStatusOperations
{
   Idle = 0x46,
   Stopped = 0x12,
   Seeking = 0x22,
   LidOpen = 0x80,
   NoDisc = 0x83,
   ReadingDataSectors = 0x36,
   ReadingAudioData = 0x34,
   Unknown = 0x30,
   SeekSecurityRing1 = 0xB2,
   SeekSecurityRing2 = 0xB6

};


void make_status_data(struct CdState *state, u8* data);


enum CommunicationState
{
   NoTransfer,
   Reset,
   Started,
   SendingFirstByte,
   ByteFinished,
   FirstByteFinished,
   SendingByte,
   SendingByteFinished,
   Running,
   NewTransfer,
   WaitToOe,
   WaitToOeFirstByte,
}comm_state = NoTransfer;

void the_log(const char * format, ...)
{
   return;
   static int started = 0;
   static FILE* fp = NULL;
   va_list l;

   if (!started)
   {
      fp = fopen("C:/yabause/THE_LOG.txt", "w");

      if (!fp)
      {
         return;
      }
      started = 1;
   }

   va_start(l, format);
   vfprintf(fp, format, l);
   va_end(l);
}

u8 cd_drive_get_serial_bit()
{
   u8 bit = 1 << (7 - cdd_cxt.bit_counter);
   return (cdd_cxt.state_data[cdd_cxt.byte_counter] & bit) != 0;
}

void cd_drive_set_serial_bit(u8 bit)
{
   cdd_cxt.received_data[cdd_cxt.byte_counter] |= bit << cdd_cxt.bit_counter;

   cdd_cxt.bit_counter++;
   if (cdd_cxt.bit_counter == 8)
   {
      cdd_cxt.byte_counter++;
      cdd_cxt.bit_counter = 0;

      if (comm_state == SendingFirstByte)
         comm_state = WaitToOeFirstByte;
      else if (comm_state == SendingByte)
         comm_state = WaitToOe;   
   }
}


extern u8 transfer_buffer[13];
int cd_command_exec()
{
   if (comm_state == Reset)
   {
      comm_state = NoTransfer;
      return 1710000;
   }
   if (comm_state == NoTransfer && (sh1_cxt.onchip.sci[0].scr & 0x30) == 0x30)
   {

      cdd_cxt.state.current_operation = Idle;
      make_status_data(&cdd_cxt.state, cdd_cxt.state_data);

      cdd_cxt.bit_counter = 0;
      cdd_cxt.byte_counter = 0;
      comm_state = SendingFirstByte;

      sh1_set_start(1);
      sh1_set_output_enable();
      the_log("start = 1 oe set \n");
   }

   //it is required to wait to assert output enable
   //otherwise some sort of race condition occurs
   //and breaks the transfer
   if (comm_state == WaitToOeFirstByte)
   {
      sh1_set_output_enable();
      sh1_set_start(0);
      comm_state = SendingByte;
      the_log("start = 0 oe set \n");
      return 40;//approximate
   }
   else if (comm_state == WaitToOe)
   {
      sh1_set_output_enable();
      the_log("oe set \n");
      comm_state = SendingByte;
      if (cdd_cxt.byte_counter == 13)
      {
         comm_state = NoTransfer;
      }

      return 40;
   }

   return 1;

   cdd_cxt.num_execs++;


#if 0
   int command = 0;
   switch (command)
   {
   case 0x0:
      //nop
      break;
   case 0x2:
      //seek
      break;
   case 0x3:
      //read toc
      break;
   case 0x4:
      //stop disc
      break;
   case 0x6:
      //read data at lba
      break;
   case 0x8:
      //pause
      break;
   case 0x9:
      //seek
      break;
   case 0xa:
      //scan forward
      break;
   case 0xb:
      //scan backwards
      break;
   }
#endif
}

void cd_drive_exec(struct CdDriveContext * drive, s32 cycles)
{
   s32 cycles_temp = drive->cycles_remainder - cycles;
   while (cycles_temp < 0)
   {
      int cycles_exec = cd_command_exec(drive);
      cycles_temp += cycles_exec;
   }
   drive->cycles_remainder = cycles_temp;
}

void make_status_data(struct CdState *state, u8* data)
{
   u8 parity = 0;
   int i = 0;
   data[0] = state->current_operation;
   data[1] = state->q_subcode;
   data[2] = state->track_number;
   data[3] = state->index_field;
   data[4] = state->minutes;
   data[5] = state->seconds;
   data[6] = state->frame;
   data[7] = 0x04; //or zero?
   data[8] = state->absolute_minutes;
   data[9] = state->absolute_seconds;
   data[10] = state->absolute_frame;


   for (i = 0; i < 11; i++)
      parity += data[i];
   data[11] = ~parity;
   data[12] = 0;
}

void cdd_reset()
{
   memset(&cdd_cxt, 0, sizeof(struct CdDriveContext));
   comm_state = Reset;
}