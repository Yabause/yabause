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

#ifndef CD_DRIVE_H
#define CD_DRIVE_H

#include "core.h"
#include "cdbase.h"

struct CdState
{
   u8 current_operation;//0
   u8 q_subcode;//1
   u8 track_number;//2
   u8 index_field;//3
   u8 minutes;//4
   u8 seconds;//5
   u8 frame;//6
   //7 zero
   u8 absolute_minutes;//8
   u8 absolute_seconds;//9
   u8 absolute_frame;//10
   //11 parity
   //12 zero
};

struct CdDriveContext
{
   s32 cycles_remainder;

   int num_execs;
   int output_enabled;
   int bit_counter;
   int byte_counter;

   struct CdState state;
   u8 state_data[13];
   u8 received_data[13];
   int received_data_counter;
   u8 post_seek_state;

   CDInterfaceToc10 toc[103*3], tracks[100];
   int toc_entry;
   int num_toc_entries, num_tracks;

   u32 disc_fad;
   u32 target_fad;

   int seek_time;
   int speed;
};

extern struct CdDriveContext cdd_cxt;


void cd_drive_exec(struct CdDriveContext * drive, s32 cycles);
u8 cd_drive_get_serial_bit();
void cd_drive_set_serial_bit(u8 bit);
void cd_drive_start_transfer();
void cdd_reset();
#endif
