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
   ReadingDataSectors = 0x36,
   ReadingAudioData = 0x34,
   Unknown = 0x30,
   SeekSecurityRing1 = 0xB2,
   SeekSecurityRing2 = 0xB6
};

struct CdState
{
   u8 current_operation;
   u8 q_subcode;
   u8 track_number;
   u8 index_field;
   u8 minutes;
   u8 seconds;
   u8 frame;
   u8 absolute_minutes;
   u8 absolute_seconds;
   u8 absolute_frame;
};

void make_status_data(struct CdState *state, u8* data);

int num_execs = 0;

int output_enabled = 0;

int get_bit_from_status(u8 * data, int bit_num)
{
   u8 byte_num = bit_num / 8;
   u8 bit_within_byte = bit_num % 8;
   u8 bit = (data[byte_num] & (1 << (7 - bit_within_byte))) != 0;
   return bit;
}

int serial_counter = 0;

enum CommunicationState
{
   NoTransfer,
   Started,
   SendingFirstByte,
   ByteFinished,
   SendingByte,
   Running
}comm_state = NoTransfer;

struct CdState state = { 0 };
u8 state_data[13] = { 0 };

s32 cd_command_exec(struct CdDriveContext * drive)
{
   //assert output enable

   //0x46 is
   //0b01000110

   if (comm_state == Started)
   {
      state.current_operation = Idle;
      make_status_data(&state, state_data);
      comm_state = SendingFirstByte;
      sh1_set_output_enable();
      serial_counter = 0;
   }
   else if (comm_state == SendingFirstByte)
   {
      int bit = 0;
      bit = get_bit_from_status(state_data, serial_counter++);

      sh1_serial_recieve_bit(bit, 0);

      if (serial_counter == 8)
      {
         sh1_set_start(0);
         comm_state = ByteFinished;
      }
   }
   else if (comm_state == ByteFinished)//after a byte has been sent
   {
      //trigger next byte
      sh1_set_output_enable();
      comm_state = SendingByte;
   }
   else if (comm_state == SendingByte)
   {
      int bit = 0;
      bit = get_bit_from_status(state_data, serial_counter++);

      sh1_serial_recieve_bit(bit, 0);

      if (serial_counter % 8 == 0 && (serial_counter == 8 * 13))
      {
         comm_state = NoTransfer;
      }
      else if (serial_counter % 8 == 0)
         comm_state = ByteFinished;
   }

   if (comm_state == NoTransfer && (sh1_cxt.onchip.sci[0].scr & 0x30) == 0x30)
   {
      comm_state = Started;
      sh1_set_start(1);
   }

   num_execs++;


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

   return 1;
}

//cycles are the serial baud rate, 50000 bits per second
void cd_drive_exec(struct CdDriveContext * drive, s32 cycles)
{
   s32 cycles_temp = drive->cycles_remainder - cycles;
   while (cycles_temp < 0)
   {
      int cycles = cd_command_exec(drive);
      cycles_temp += cycles;
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