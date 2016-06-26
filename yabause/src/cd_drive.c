/*  Copyright 2014-2016 James Laird-Wah
    Copyright 2004-2006, 2013, 2016 Theo Berkau

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
#include "cs2.h"
#include "ygr.h"
#include <stdarg.h>
#include "tsunami/yab_tsunami.h"

static INLINE u8 num2bcd(u8 num);
static INLINE void fad2msf_bcd(s32 fad, u8 *msf);

#define WANT_RX_TRACE
#ifdef WANT_RX_TRACE
#define RXTRACE(...) cd_trace_log(__VA_ARGS__)
#else
#define RXTRACE(...)
#endif

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

//When reading sectors SH1 expects irq7 delta timing to be within a margin of -/+ 28 ticks. Adjust as required
#define TIME_READSECTOR 8730

//1 second / 75 sectors == 13333.333...
//2299 time for transferring 13 bytes, start signal etc
#define TIME_AUDIO_SECTOR (13333 - 2299)

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
s32 toc_10_get_track(s32 fad);
void state_set_msf_info(struct CdState *state, s32 track_fad, s32 disc_fad);
static INLINE u32 msf_bcd2fad(u8 min, u8 sec, u8 frame);
s32 get_track_fad(int track_num, s32 fad, int * index);
s32 get_track_start_fad(int track_num);
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

static INLINE void fad2msf(s32 fad, u8 *msf) {
   msf[0] = fad / (75 * 60);
   fad -= msf[0] * (75 * 60);
   msf[1] = fad / 75;
   fad -= msf[1] * 75;
   msf[2] = fad;
}

static INLINE u8 num2bcd(u8 num) {
   return ((num / 10) << 4) | (num % 10);
}

static INLINE void fad2msf_bcd(s32 fad, u8 *msf) {
   fad2msf(fad, msf);
   msf[0] = num2bcd(msf[0]);
   msf[1] = num2bcd(msf[1]);
   msf[2] = num2bcd(msf[2]);
}

static INLINE u8 bcd2num(u8 bcd) {
   return (bcd >> 4) * 10 + (bcd & 0xf);
}

static INLINE u32 msf_bcd2fad(u8 min, u8 sec, u8 frame) {
   u32 fad = 0;
   fad += bcd2num(min);
   fad *= 60;
   fad += bcd2num(sec);
   fad *= 75;
   fad += bcd2num(frame);
   return fad;
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

void do_toc()
{
   int toc_entry;
   cdd_cxt.state_data[0] = cdd_cxt.state.current_operation = ReadToc;
   comm_state = NoTransfer;
   //fill cdd_cxt.state_data with toc info

   toc_entry = cdd_cxt.toc_entry++;
   memcpy(cdd_cxt.state_data + 1, &cdd_cxt.toc[toc_entry], 10);

   set_checksum(cdd_cxt.state_data);

   if (cdd_cxt.toc_entry >= cdd_cxt.num_toc_entries)
   {
      cdd_cxt.state.current_operation = Idle;
      make_status_data(&cdd_cxt.state, cdd_cxt.state_data);
   }
}
void update_seek_status();

void update_status_info()
{
   int index = 0;
   s32 track_num = 0;
   s32 track_fad = 0;
   s32 track_start_fad = 0;
   if (cdd_cxt.disc_fad >= get_track_start_fad(-1)) {
       track_fad = cdd_cxt.disc_fad;
   } else {
       track_num = toc_10_get_track(cdd_cxt.disc_fad);
       track_fad = get_track_fad(track_num, cdd_cxt.disc_fad + 4, &index);
       track_start_fad = get_track_start_fad(track_num);
       track_fad = cdd_cxt.disc_fad - track_start_fad;
   }

   if (track_fad < 0)
      track_fad = -track_fad;
   else
      index = 1;

   state_set_msf_info(&cdd_cxt.state, track_fad, cdd_cxt.disc_fad);

   if (cdd_cxt.disc_fad >= get_track_start_fad(-1)) {   // leadout
        cdd_cxt.state.q_subcode = 1;
        cdd_cxt.state.index_field = 1;
        cdd_cxt.state.track_number = 0xaa;
   } else {
       cdd_cxt.state.q_subcode = cdd_cxt.tracks[track_num - 1].ctrladr;
       cdd_cxt.state.index_field = index;
       cdd_cxt.state.track_number = num2bcd(track_num);
   }
}

static u8 ror(u8 in) 
{
   return (in>>1) | (in<<(7));
}

static void make_ring_data(u8 *buf) 
{
   u32 i,j;
   u16 lfsr = 1;
   u8 a;
   for (i=12; i<2352; i++) 
   {
      a = (i & 1) ? 0x59 : 0xa8;
      for (j=0; j<8; j++) 
      {
         u32 x = a;
         u32 var2 = lfsr & 1;
         a ^= var2;
         a = ror(a);

         x = lfsr;
         x >>= 1;
         var2 = lfsr;
         x ^= var2;
         lfsr |= x << 15;
         lfsr >>= 1;
      }
      buf[i-12] = a;
   }
}

void do_dataread()
{
   struct Dmac *dmac=&sh1_cxt.onchip.dmac;
#if 1

   if ((dmac->channel[0].chcr & 1) && 
      !(dmac->channel[0].chcr & 2)) 
   {
      u8 buf[2448];		
      u32 dest;
      int i;

      CDLOG("running DMA to %X(FAD: %d)\n", dmac->channel[0].dar, cdd_cxt.disc_fad);

      if (cdd_cxt.disc_fad < 150)
      {
         memset(buf, 0, sizeof(buf));
         fad2msf_bcd(cdd_cxt.disc_fad, buf+12);
         buf[15] = 1;
      }
      else if (cdd_cxt.disc_fad >= get_track_start_fad(-1))
      {
         u8 *subbuf=buf+12;
         // fills all 2352 bytes
         make_ring_data(subbuf);

         fad2msf_bcd(cdd_cxt.disc_fad, subbuf);
         subbuf[3] = 2;	// Mode 2, Form 2
         // 8 byte subheader (unknown purpose)
         subbuf[4] = 0; subbuf[5] = 0; subbuf[6] = 28; subbuf[7] = 0;
         subbuf[8] = 0; subbuf[9] = 0; subbuf[10] = 28; subbuf[11] = 0;

         // 4 byte error code at end
         subbuf[2352-4] = 0;
         subbuf[2352-3] = 0;
         subbuf[2352-2] = 0;
         subbuf[2352-1] = 0;
      }
      else
         Cs2Area->cdi->ReadSectorFAD(cdd_cxt.disc_fad, buf);

      printf("sector head:");
      for (i=12; i<16; i++)
          printf(" %02X", buf[i]);
      printf("\n");

      if (dmac->channel[0].dar >> 24 != 9) 
      {
         CDLOG("DMA0 error: dest is not DRAM\n");
      }
      if (dmac->channel[0].tcr*2 > 2340) 
      {
         CDLOG("DMA0 error: count too big\n");
      }
      dest = dmac->channel[0].dar & 0x7FFFF;

      for (i = 0; i < dmac->channel[0].tcr*2; i+=2)
         T2WriteWord(SH1Dram, dest+i, (buf[12+i] << 8) | buf[13+i]);

      dmac->channel[0].chcr |= 2;		
      if (dmac->channel[0].chcr & 4)
         SH2SendInterrupt(SH1, 72, (sh1_cxt.onchip.intc.iprc >> 12) & 0xf);
   }
  cdd_cxt.disc_fad++;

  if (cdd_cxt.disc_fad >= 150 && cdd_cxt.disc_fad < get_track_start_fad(-1))
     Cs2Area->cdi->ReadAheadFAD(cdd_cxt.disc_fad);

#else
   if (!(dmac->channel[0].chcr & 2))
   {
     // do_dma(0);
      if (dmac->channel[0].chcr & 4)
         SH2SendInterrupt(SH1, 72, (sh1_cxt.onchip.intc.iprc >> 12) & 0xf);
   }
#endif

   ygr_cd_irq(0x10);
}

void make_ring_status()
{
   u32 fad = cdd_cxt.disc_fad + 4;
   u8 msf_buf[3] = { 0 };
   fad2msf_bcd(cdd_cxt.disc_fad, msf_buf);

   cdd_cxt.state_data[0] = SeekSecurityRing2;
   cdd_cxt.state_data[1] = 0x44;
   cdd_cxt.state_data[2] = 0xf1;
   cdd_cxt.state_data[3] = fad >> 16;
   cdd_cxt.state_data[4] = fad >> 8;
   cdd_cxt.state_data[5] = fad;
   cdd_cxt.state_data[6] = 0x09;
   cdd_cxt.state_data[7] = 0x09;
   cdd_cxt.state_data[8] = 0x09;
   cdd_cxt.state_data[9] = 0x09;
   cdd_cxt.state_data[10] = msf_buf[2];

   set_checksum(cdd_cxt.state_data);
}
void ScspReceiveCDDA(const u8 *sector);

int continue_command()
{
   if (cdd_cxt.state.current_operation == Idle)
   {
      comm_state = NoTransfer;
      cdd_cxt.disc_fad++;
      
      //drive head moves back when fad is too high
      if (cdd_cxt.disc_fad > cdd_cxt.target_fad + 5)
         cdd_cxt.disc_fad = cdd_cxt.target_fad;

      update_status_info();
      make_status_data(&cdd_cxt.state, cdd_cxt.state_data);
      return TIME_PERIODIC/cdd_cxt.speed;
   }
   else if (cdd_cxt.state.current_operation == ReadingDataSectors ||
            cdd_cxt.state.current_operation == ReadingAudioData)
   {
      int is_audio = 1;

      if (cdd_cxt.disc_fad < 150)
         is_audio = 0;

      if (cdd_cxt.disc_fad >= get_track_start_fad(-1))
         is_audio = 0;

      if (cdd_cxt.state.current_operation != ReadingAudioData)
         is_audio = 0;

      comm_state = NoTransfer;

      if (is_audio)
      {
         u8 buf[2448];		
         Cs2Area->cdi->ReadSectorFAD(cdd_cxt.disc_fad, buf);
         ScspReceiveCDDA(buf);
         cdd_cxt.disc_fad++;
         Cs2Area->cdi->ReadAheadFAD(cdd_cxt.disc_fad);
      }
      else
         do_dataread();

      update_status_info();
      cdd_cxt.state.current_operation = (cdd_cxt.state.q_subcode & 0x40) ? ReadingDataSectors : ReadingAudioData;
      make_status_data(&cdd_cxt.state, cdd_cxt.state_data);

      if (is_audio)
         return TIME_AUDIO_SECTOR;
      else
         return TIME_READSECTOR / cdd_cxt.speed;
   }
   else if (cdd_cxt.state.current_operation == Stopped)
   {
      comm_state = NoTransfer;
      return TIME_PERIODIC / cdd_cxt.speed;
   }
   else if (cdd_cxt.state.current_operation == ReadToc)
   {
      do_toc();
      return TIME_READING;
   }
   else if (cdd_cxt.state.current_operation == Seeking || 
      cdd_cxt.state.current_operation == SeekSecurityRing2)
   {
      
      cdd_cxt.seek_time++;
      update_seek_status();

      if (cdd_cxt.seek_time > 9)
      {
         //seek completed, change for next status
         cdd_cxt.state.current_operation = cdd_cxt.post_seek_state;
      }
      comm_state = NoTransfer;
      return TIME_READING;
   }
   else
   {
      comm_state = NoTransfer;
      return TIME_PERIODIC / cdd_cxt.speed;
   }
}

u32 get_fad_from_command(u8 * buf)
{
   u32 fad = buf[1];
   fad <<= 8;
   fad |= buf[2];
   fad <<= 8;
   fad |= buf[3];

   return fad;
}


s32 get_track_start_fad(int track_num)
{
   s32 fad;
   if (track_num == -1) // leadout
      track_num = cdd_cxt.num_tracks;
   else                 // normal track (1-based)
      track_num--;
   fad = msf_bcd2fad(cdd_cxt.tracks[track_num].pmin, cdd_cxt.tracks[track_num].psec, cdd_cxt.tracks[track_num].pframe);
   return fad;
}

s32 get_track_fad(int track_num, s32 fad, int * index)
{
   s32 track_start_fad = get_track_start_fad(track_num);
   s32 track_fad = fad - track_start_fad;

   if (track_fad < 0)
      *index = 1;

   return track_fad;
}

void update_seek_status()
{
   update_status_info();
   cdd_cxt.state.current_operation = Seeking;

   make_status_data(&cdd_cxt.state, cdd_cxt.state_data);
   comm_state = NoTransfer;
}

void do_seek_common(u8 post_seek_state)
{
   s32 fad = get_fad_from_command(cdd_cxt.received_data);
   cdd_cxt.disc_fad = fad - 4;
   cdd_cxt.target_fad = cdd_cxt.disc_fad;
   cdd_cxt.seek_time = 0;
   cdd_cxt.post_seek_state = post_seek_state;
}

void do_seek()
{
   do_seek_common(Idle);
   update_seek_status();
}

int do_command()
{
   int command = cdd_cxt.received_data[0];

   if (cdd_cxt.received_data[10] == 1)
      cdd_cxt.speed = 1;
   else
      cdd_cxt.speed = 2;

   switch (command)
   {
   case 0x0:
      //nop
      return continue_command();
      break;
   case 0x2:
      //seeking ring
      CDLOG("seek ring\n");
      cdd_cxt.state.current_operation = SeekSecurityRing2;
      do_seek_common(Idle);
      make_ring_status();
      comm_state = NoTransfer;
      return TIME_READING / cdd_cxt.speed;
      break;
   case 0x3:
   {
      int i, track_num, max_track = 0;
      CDInterfaceToc10 toc[102];
      CDLOG("read toc\n");
      cdd_cxt.toc_entry = 0;
      cdd_cxt.num_toc_entries = Cs2Area->cdi->ReadTOC10(toc);

      // Read and sort the track entries, so you get one entry for each track + leadout, in starting FAD order.
      for (i = 0; i < cdd_cxt.num_toc_entries; i++)
      {
         CDInterfaceToc10 *entry = &cdd_cxt.toc[i*3];
         entry->ctrladr = toc[i].ctrladr;
         entry->tno = toc[i].tno;
         if(toc[i].point > 0x99)
            entry->point = toc[i].point;
         else
            entry->point = toc[i].point = num2bcd(toc[i].point);
         entry->min = num2bcd(toc[i].min);
         entry->sec = num2bcd(toc[i].sec);
         entry->frame = num2bcd(i*3);
         entry->zero = 0;
         entry->pmin = num2bcd(toc[i].pmin);
         entry->psec = num2bcd(toc[i].psec);
         entry->pframe = num2bcd(toc[i].pframe);
         memcpy(&cdd_cxt.toc[i*3+1], entry, sizeof(*entry));
         cdd_cxt.toc[i*3+1].frame = num2bcd(i*3+1);
         memcpy(&cdd_cxt.toc[i*3+2], entry, sizeof(*entry));
         cdd_cxt.toc[i*3+2].frame = num2bcd(i*3+2);

         if (entry->point <= 0x99) 
         {
            track_num = bcd2num(entry->point);
            if (track_num > max_track)
               max_track = track_num;
            memcpy(&cdd_cxt.tracks[track_num-1], entry, sizeof(*entry));
         }
      }
      cdd_cxt.num_toc_entries *= 3;
      for (i = 0; i < cdd_cxt.num_toc_entries; i++)
      {
         if (cdd_cxt.toc[i].point == 0xa2) {  // leadout
            memcpy(&cdd_cxt.tracks[max_track], &cdd_cxt.toc[i], sizeof(cdd_cxt.toc[i]));
            break;
         }
      }
      cdd_cxt.num_tracks = max_track;
      do_toc();

      return TIME_READING;
      break;
   }
   case 0x4:
      //stop disc
      CDLOG("stop disc\n");
      cdd_cxt.state.current_operation = Stopped;
      make_status_data(&cdd_cxt.state, cdd_cxt.state_data);
      comm_state = NoTransfer;
      return TIME_PERIODIC / cdd_cxt.speed;
      break;
   case 0x5:
   //prevent scan from crashing the drive for now
   case 0xa:
   case 0xb:
      CDLOG("unknown command 5\n");
      //just idle for now
      cdd_cxt.state.current_operation = Idle;
      make_status_data(&cdd_cxt.state, cdd_cxt.state_data);
      comm_state = NoTransfer;
      return TIME_PERIODIC / cdd_cxt.speed;
      break;
   case 0x6:
   {
      do_seek_common(ReadingDataSectors);
      update_seek_status();

      comm_state = NoTransfer;
      return TIME_READSECTOR / cdd_cxt.speed;
   }
   case 0x8:
      //pause
      CDLOG("pause\n");
      cdd_cxt.state.current_operation = Idle;
      make_status_data(&cdd_cxt.state, cdd_cxt.state_data);
      comm_state = NoTransfer;
      return TIME_PERIODIC / cdd_cxt.speed;
      break;
   case 0x9:
      //seek
   {
      CDLOG("seek\n");
      do_seek();
      return TIME_READING;
      break;
   }
#if 0
   case 0xa:
      //scan forward
      CDLOG("scan forward\n");
      break;
   case 0xb:
      //scan backwards
      CDLOG("scan backwards\n");
      break;
#endif
   }

   assert(0);

   return TIME_PERIODIC;
}


char* get_status_string(int status)
{
   u32 track_fad = msf_bcd2fad(cdd_cxt.state_data[4], cdd_cxt.state_data[5], cdd_cxt.state_data[6]);
   u32 abs_fad = msf_bcd2fad(cdd_cxt.state_data[8], cdd_cxt.state_data[9], cdd_cxt.state_data[10]);

   static char str[256] = { 0 };

   switch (status)
   {
   case 0x46:
      sprintf(str, "%s %d %d", "Idle", track_fad, abs_fad);
      return str;
   case 0x12:
      return "Stopped";
   case 0x22:
      sprintf(str, "%s %d %d", "Seeking", track_fad, abs_fad);
      return str;
   case 0x36:
      sprintf(str, "%s %d %d", "Reading Data Sectors", track_fad, abs_fad);
      return str;
   case 0x34:
      sprintf(str, "%s %d %d", "Reading Audio Data", track_fad, abs_fad);
      return str;
   default:
      return "";
   }
   return "";
}

char * get_command_string(int command)
{
   u32 fad = get_fad_from_command(cdd_cxt.received_data);

   static char str[256] = { 0 };

   switch (command)
   {
   case 0x0:
      return "";
   case 0x2:
      return "Seeking Ring";
   case 0x3:
      return "Read TOC";
   case 0x4:
      return "Stop Disc";
   case 0x6:
      sprintf(str, "%s %d", "Read", fad);
      return str;
   case 0x8:
      return "Pause";
   case 0x9:
      sprintf(str, "%s %d", "Seek", fad);
      return str;
   default:
      return "";
      break;
   }
   return "";
}

void do_cd_logging()
{
   char str[1024] = { 0 };
   int i;

   for (i = 0; i < 12; i++)
      sprintf(str + strlen(str), "%02X ", cdd_cxt.received_data[i]);

   sprintf(str + strlen(str), " || ");

   for (i = 0; i < 12; i++)
      sprintf(str + strlen(str), "%02X ", cdd_cxt.state_data[i]);

   sprintf(str + strlen(str), " %s ||  %s", get_command_string(cdd_cxt.received_data[0]), get_status_string(cdd_cxt.state_data[0]));

   CDLOG("%s\n", str);
}

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
#ifdef CDDEBUG
      //debug logging
      static u8 previous_state = 0;

      if (cdd_cxt.received_data[11] != 0xff && cdd_cxt.received_data[0])
      {
         char str[1024] = { 0 }; 
         int i;

         for (i = 0; i < 13; i++)
            sprintf(str + strlen(str), "%02X ", cdd_cxt.received_data[i]);

         CDLOG("CMD: %s\n", str);
      }

      if (cdd_cxt.state_data[0] != previous_state)
      {
         char str[1024] = { 0 };
         int i;

         previous_state = cdd_cxt.state_data[0];

         for (i = 0; i < 13; i++)
            sprintf(str + strlen(str), "%02X ", cdd_cxt.state_data[i]);

         CDLOG("STA: %s\n", str);
      }
#endif

#ifdef DO_LOGGING
      do_cd_logging();
#endif

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
      int cycles_exec = cd_command_exec();
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

s32 toc_10_get_track(s32 fad)
{
   int i = 0;
   if (!cdd_cxt.num_toc_entries)
      return 1;

   for (i = 0; i < cdd_cxt.num_toc_entries; i++)
   {
      s32 track_start = get_track_start_fad(i+1);
      s32 track_end  = get_track_start_fad(i+2);

      if (fad >= track_start && fad < track_end)
         return (i + 1);

      if (i == 0 && fad < track_start)
         return 1;//lead in
   }

   return 0;
}

void state_set_msf_info(struct CdState *state, s32 track_fad, s32 abs_fad)
{
   u8 msf_buf[3] = { 0 };
   fad2msf_bcd(track_fad, msf_buf);
   state->minutes = msf_buf[0];
   state->seconds = msf_buf[1];
   state->frame = msf_buf[2];
   fad2msf_bcd(abs_fad, msf_buf);
   state->absolute_minutes = msf_buf[0];
   state->absolute_seconds = msf_buf[1];
   state->absolute_frame = msf_buf[2];
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

   if (data[0] && data[0] != 0x46) {
         RXTRACE("STA: ");

         for (i = 0; i < 13; i++)
            RXTRACE(" %02X", data[i]);
         RXTRACE("\n");
   }
}

void cdd_reset()
{
   memset(&cdd_cxt, 0, sizeof(struct CdDriveContext));
   comm_state = Reset;
}