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
#include "tsunami/yab_tsunami.h"

//oe rising edge to falling edge
//26 usec
#define TIME_OE 26

//serial clock rising edge of the final bit in a transmission to falling edge of start signal
//13992 usec
#define TIME_PERIODIC 13992

//start  falling edge to rising edge
//187 usec
#define TIME_START 187

//start falling edge to rising edge (slow just after power on)
//3203 usec

//poweron stable signal to first start falling edge (reset time)
//451448 usec
#define TIME_POWER_ON 451448

//time from first start falling edge to first transmission
#define TIME_WAITING 416509

//from falling edge to rising edge of serial clock signal for 1 byte
#define TIME_BYTE 150

//"when disc is reading transactions are ~6600us apart"
#define TIME_READING 6600

struct CdDriveContext cdd_cxt;

enum CdStatusOperations
{
   ReadToc = 0x04,
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
void set_checksum(u8 * data);

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
   WaitToRxio
}comm_state = NoTransfer;

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
      tsunami_log_value("CMD", cdd_cxt.received_data[cdd_cxt.byte_counter], 8);

      cdd_cxt.byte_counter++;
      cdd_cxt.bit_counter = 0;

      sh1_set_output_enable_rising_edge();

      if (comm_state == SendingFirstByte)
         comm_state = WaitToOeFirstByte;
      else if (comm_state == SendingByte)
         comm_state = WaitToOe;

      if (cdd_cxt.byte_counter == 13)
         comm_state = WaitToRxio;
   }
}

int do_command()
{
   int command = cdd_cxt.received_data[0];
   switch (command)
   {
   case 0x0:
      //nop
      comm_state = NoTransfer;
      return TIME_PERIODIC;
      break;
   case 0x2:
      //seeking ring
      cdd_cxt.state.current_operation = SeekSecurityRing2;
      break;
   case 0x3:
   {
      int i = 0;
      //read toc
      cdd_cxt.state_data[0] = cdd_cxt.state.current_operation = ReadToc;
      comm_state = NoTransfer;
      //fill cdd_cxt.state_data with toc info

      cdd_cxt.state_data[1] = 0x41;
      cdd_cxt.state_data[2] = 0x00;
      cdd_cxt.state_data[3] = 0xA0;
      cdd_cxt.state_data[4] = 0x00;
      cdd_cxt.state_data[5] = 0x02;
      cdd_cxt.state_data[6] = 0x00;
      cdd_cxt.state_data[7] = 0x00;
      cdd_cxt.state_data[8] = 0x01;
      cdd_cxt.state_data[9] = 0x00;
      cdd_cxt.state_data[10] = 0x00;

      set_checksum(cdd_cxt.state_data);
      return TIME_READING;
      break;
   }
   case 0x4:
      //stop disc
      cdd_cxt.state.current_operation = Stopped;
      break;
   case 0x6:
      //read data at lba
      cdd_cxt.state.current_operation = ReadingDataSectors;//what about audio data?
      break;
   case 0x8:
      //pause
      cdd_cxt.state.current_operation = Idle;
      break;
   case 0x9:
      //seek
      cdd_cxt.state.current_operation = Seeking;
      break;
   case 0xa:
      //scan forward
      break;
   case 0xb:
      //scan backwards
      break;
   }

   return TIME_PERIODIC;
}


extern u8 transfer_buffer[13];
int cd_command_exec()
{
   if (comm_state == Reset)
   {
      cdd_cxt.state.current_operation = Idle;
      make_status_data(&cdd_cxt.state, cdd_cxt.state_data);
      comm_state = NoTransfer;
      return TIME_POWER_ON + TIME_WAITING;
   }
   else if (
      comm_state == SendingFirstByte || 
      comm_state == SendingByte)
   {
      return TIME_BYTE;
   }
   else if (
      comm_state == NoTransfer)
   {
      cdd_cxt.bit_counter = 0;
      cdd_cxt.byte_counter = 0;
      comm_state = SendingFirstByte;

      memset(&cdd_cxt.received_data, 0, sizeof(u8) * 13);

      sh1_set_start(1);
      sh1_set_output_enable_falling_edge();

      return TIME_START;
   }
   //it is required to wait to assert output enable
   //otherwise some sort of race condition occurs
   //and breaks the transfer
   else if (comm_state == WaitToOeFirstByte)
   {
      sh1_set_output_enable_falling_edge();
      sh1_set_start(0);
      comm_state = SendingByte;
      return TIME_OE;
   }
   else if (comm_state == WaitToOe)
   {
      sh1_set_output_enable_falling_edge();
      comm_state = SendingByte;

      return TIME_OE;
   }
   else if (comm_state == WaitToRxio)
   {
      //handle the command
      return do_command();
   }

   assert(0);

   return 1;

   cdd_cxt.num_execs++;
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

void set_checksum(u8 * data)
{
   u8 parity = 0;
   int i = 0;
   for (i = 0; i < 11; i++)
      parity += data[i];
   data[11] = ~parity;

   data[12] = 0;
}

void make_status_data(struct CdState *state, u8* data)
{
   
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

   set_checksum(data); 
}

void cdd_reset()
{
   memset(&cdd_cxt, 0, sizeof(struct CdDriveContext));
   comm_state = Reset;
}