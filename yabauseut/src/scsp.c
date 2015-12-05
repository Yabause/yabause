/*  Copyright 2013 Theo Berkau

    This file is part of YabauseUT

    YabauseUT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    YabauseUT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with YabauseUT; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <iapetus.h>
#include "tests.h"
#include "scsp.h"
#include "main.h"
#include <stdio.h>

#define SCSPREG_TIMERA  (*(volatile u16 *)0x25B00418)
#define SCSPREG_TIMERB  (*(volatile u16 *)0x25B0041A)
#define SCSPREG_TIMERC  (*(volatile u16 *)0x25B0041C)

#define SCSPREG_SCIEB   (*(volatile u16 *)0x25B0041E)
#define SCSPREG_SCIPD   (*(volatile u16 *)0x25B00420)
#define SCSPREG_SCIRE   (*(volatile u16 *)0x25B00422)

#define SCSPREG_MCIEB   (*(volatile u16 *)0x25B0042A)
#define SCSPREG_MCIPD   (*(volatile u16 *)0x25B0042C)
#define SCSPREG_MCIRE   (*(volatile u16 *)0x25B0042E)

volatile u32 hblank_counter=0;

int tinc=0;

void scsp_interactive_test();

//////////////////////////////////////////////////////////////////////////////

void scsp_test()
{
   int choice;

   menu_item_struct vdp1_menu[] = {
//   { "Individual slots(Mute sound first)" , &scspslottest, },
   { "SCSP timing" , &scsp_timing_test, },
   { "Misc" , &scsp_misc_test, },
//   { "Interactive" , &scspinteractivetest, },
   { "\0", NULL }
   };

   for (;;)
   {
      choice = gui_do_menu(vdp1_menu, &test_disp_font, 0, 0, "SCSP Tests", MTYPE_CENTER, -1);
      if (choice == -1)
         break;
   }   
}

//////////////////////////////////////////////////////////////////////////////

void scsp_minimal_init()
{
   // Put system in minimalized state
   interrupt_set_level_mask(0xF);

   init_iapetus(RES_320x224);
   vdp_rbg0_init(&test_disp_settings);
   vdp_set_default_palette();

   // Turn off sound cpu
   smpc_issue_command(0x07);

   // Display On
   vdp_disp_on();
}

//////////////////////////////////////////////////////////////////////////////

void scsp_minimal_slot_init(int slotnum)
{
/*
   void *slot=(void *)(0x25B00000 + (slotnum << 5));

   // Setup slot
   *(volatile u32 *)(slot+0x00) = 0x10300000 | (info->ssctl << 23) | 0; // key off, normal loop, start address
   *(volatile u16 *)(slot+0x04) = 0x0000; // LSA
   *(volatile u16 *)(slot+0x06) = 0x00FF; // LEA  
   *(volatile u16 *)(slot+0x08) = (info->d2r << 11) | (info->d1r << 6) | (info->eghold << 5) | info->ar; // Normal D2R, D1R, EGHOLD, AR
   *(volatile u16 *)(slot+0x0A) = (info->lpslnk << 14) | (info->krs << 10) | (info->dl << 5) | info->rr; // Normal LPSLNK, KRS, DL, RR
   *(volatile u16 *)(slot+0x0C) = (info->stwinh << 9) | (info->sdir << 8) | info->tl; // stwinh, sdir, tl    
   *(volatile u16 *)(slot+0x0E) = (info->mdl << 12) | (info->mdxsl << 6) | info->mdysl; // modulation stuff
   *(volatile u16 *)(slot+0x10) = (info->oct << 11) | info->fns; // oct/fns
   *(volatile u16 *)(slot+0x12) = (info->lfof << 10) | (info->plfows << 8) | (info->plfos << 5) | (info->alfows << 3) | info->alfos; // lfo
   *(volatile u16 *)(slot+0x14) = (info->isel << 3) | info->imxl; // isel/imxl
   *(volatile u16 *)(slot+0x16) = (info->disdl << 13) | (info->dipan << 8) | (info->efsdl << 5) | info->efpan; // 

   // Time to key on
   *(volatile u8 *)(slot+0x00) = 0x18 | (info->ssctl >> 1);
*/
}

//////////////////////////////////////////////////////////////////////////////

void scsp_slot_test()
{
   scsp_minimal_init();
   unregister_all_tests();
   register_test(&scu_interrupt_test, "Key On/Off");
   register_test(&scsp_timer_a_test, "8/16-bit/Noise samples");
   register_test(&scsp_timer_a_test, "Source bit");
   register_test(&scsp_timer_a_test, "Looping");
   register_test(&scsp_timer_a_test, "Octaves"); // not sure I can easily test this
   register_test(&scsp_timer_a_test, "Attack rate"); // not sure I can easily test this
   register_test(&scsp_timer_a_test, "Decay rate"); // not sure I can easily test this
   register_test(&scsp_timer_a_test, "Decay level"); // not sure I can easily test this
   register_test(&scsp_timer_a_test, "Release rate"); // not sure I can easily test this
   register_test(&scsp_timer_a_test, "Total level");
   // modulation test here
   // LFO tests here
   // direct pan/sdl tests here
   do_tests("SCSP Timer tests", 0, 0);
}

//////////////////////////////////////////////////////////////////////////////

void scsp_timing_test()
{
   scsp_minimal_init();
   unregister_all_tests();
   register_test(&scu_interrupt_test, "Sound Request Interrupt");
   register_test(&scsp_timer_a_test, "Timer A");
   register_test(&scsp_timer_b_test, "Timer B");
   register_test(&scsp_timer_c_test, "Timer C");
/*
   register_test(&ScspTimerTimingTest0, "Timer timing w/1 sample inc");
   register_test(&ScspTimerTimingTest1, "Timer timing w/2 sample inc");
   register_test(&ScspTimerTimingTest2, "Timer timing w/4 sample inc");
   register_test(&ScspTimerTimingTest3, "Timer timing w/8 sample inc");
   register_test(&ScspTimerTimingTest4, "Timer timing w/16 sample inc");
   register_test(&ScspTimerTimingTest5, "Timer timing w/32 sample inc");
   register_test(&ScspTimerTimingTest6, "Timer timing w/64 sample inc");
   register_test(&ScspTimerTimingTest7, "Timer timing w/128 sample inc");
*/
   do_tests("SCSP Timer tests", 0, 0);
}

//////////////////////////////////////////////////////////////////////////////

void scsp_misc_test()
{
   scsp_minimal_init();
   unregister_all_tests();
   register_test(&scsp_int_on_timer_enable_test, "Int on Timer enable behaviour");
   register_test(&scsp_int_on_timer_reset_test, "Int on Timer reset behaviour");
   register_test(&scsp_int_dup_test, "No second Int after Timer done");
//   register_test(&scsp_mcipd_test, "MCIPD bit cleared after Timer Int");
   register_test(&scsp_scipd_test, "Timers start after SCIRE write");
   register_test(&scsp_scipd_test, "Timers start after MCIRE write");

   // DMA tests here

   // MSLC/CA tests here

   do_tests("SCSP Interrupt tests", 0, 0);
}

//////////////////////////////////////////////////////////////////////////////

int scsp_counter;

void scsp_interrupt()
{
   scsp_counter++;
}

//////////////////////////////////////////////////////////////////////////////

void scsp_interactive_test()
{
   u16 scieb=0;
   u16 mcieb=0;

   scsp_minimal_init();

   test_disp_font.transparent = 0;
   scsp_counter=0;
   SCSPREG_SCIEB = mcieb;
   SCSPREG_MCIEB = scieb;

   // Mask SCSP interrupt temporarily
   bios_change_scu_interrupt_mask(0xFFFFFFFF, 0x40);

   // Set SCSP interrupt function
   bios_set_scu_interrupt(0x46, scsp_interrupt);

   // Unmask SCSP interrupt
   bios_change_scu_interrupt_mask(~0x40, 0);

   for (;;)
   {
      u16 scipd;
      u16 mcipd;

      vdp_vsync();

#define Refresh(line) \
      scipd = SCSPREG_SCIPD; \
      mcipd = SCSPREG_MCIPD; \
      \
      vdp_printf(&test_disp_font, 2 * 8, line * 8, 0xF, "SCIEB = %04X", scieb); \
      vdp_printf(&test_disp_font, 2 * 8, (line+1) * 8, 0xF, "MCIEB = %04X", mcieb); \
      vdp_printf(&test_disp_font, 2 * 8, (line+2) * 8, 0xF, "SCIPD = %04X", scipd); \
      vdp_printf(&test_disp_font, 2 * 8, (line+3) * 8, 0xF, "MCIPD = %04X", mcipd); \
      vdp_printf(&test_disp_font, 2 * 8, (line+4)* 8, 0xF, "int counter = %d", scsp_counter);

      Refresh(17);

      if (per[0].but_push_once & PAD_A)
      {
         SCSPREG_TIMERA = 0x700;
         Refresh(23);
      }
      else if (per[0].but_push_once & PAD_B)
      {
         SCSPREG_TIMERA = 0x07FF;
         Refresh(23);
      }
      else if (per[0].but_push_once & PAD_C)
      {
         SCSPREG_SCIRE = 0x40;
         Refresh(23);
      }
      else if (per[0].but_push_once & PAD_X)
      {
         mcieb ^= 0x40;
         SCSPREG_MCIEB = mcieb;
         Refresh(23);
      }
      else if (per[0].but_push_once & PAD_Y)
      {
         SCSPREG_MCIRE = 0x40;
         Refresh(23);
      }
      else if (per[0].but_push_once & PAD_L)
      {
         mcieb ^= 0x20;
         SCSPREG_MCIEB = mcieb;
         Refresh(23);
      }
      else if (per[0].but_push_once & PAD_R)
      {
         if (SCSPREG_MCIPD & 0x20)
            SCSPREG_MCIRE = 0x20;
         else
            SCSPREG_MCIPD = 0x20;
         Refresh(23);
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

void scsp_int_test_func()
{
   stage_status = STAGESTAT_DONE;

   // Mask SCSP interrupts
   bios_change_scu_interrupt_mask(0xFFFFFFFF, 0x40);
}

//////////////////////////////////////////////////////////////////////////////

void scu_interrupt_test()
{
   // Disable everything temporarily
   SCSPREG_SCIEB = 0;
   SCSPREG_MCIEB = 0;

   // Mask SCSP interrupt temporarily
   bios_change_scu_interrupt_mask(0xFFFFFFFF, 0x40);

   // Set SCSP interrupt function
   bios_set_scu_interrupt(0x46, scsp_int_test_func);

   // Unmask SCSP interrupt
   bios_change_scu_interrupt_mask(~0x40, 0);

   // Enable all SCSP Main cpu interrupts
   SCSPREG_MCIEB = 0x7FF;
   SCSPREG_MCIRE = 0x7FF;
}

//////////////////////////////////////////////////////////////////////////////

void scsp_timer_pending_test(int timer_mask, int main_cpu)
{
   // Disable everything
   SCSPREG_SCIEB = 0;
   SCSPREG_MCIEB = 0;

   if (main_cpu)
   {
      // Clear any pending interrupts
      SCSPREG_SCIRE = timer_mask;

      // Make sure it isn't set prematurely
      if (SCSPREG_SCIPD & timer_mask)
      {
         stage_status = STAGESTAT_BADTIMING;
         return;
      }

      // Wait a bit while testing SCIPD
      wait_test(SCSPREG_SCIPD & timer_mask, 80000, STAGESTAT_BADINTERRUPT)
   }
   else
   {
      // Clear any pending interrupts
      SCSPREG_MCIRE = timer_mask;

      // Make sure it isn't set prematurely
      if (SCSPREG_MCIPD & timer_mask)
      {
         stage_status = STAGESTAT_BADTIMING;
         return;
      }

      // Wait a bit while testing SCIPD
      wait_test(SCSPREG_MCIPD & timer_mask, 80000, STAGESTAT_BADINTERRUPT)
   }
}

//////////////////////////////////////////////////////////////////////////////

void scsp_scipd_test()
{
   int i;
   u16 mask_list[] = { 0x0040, 0x0080, 0x0100 };

   for (i = 0; i < (sizeof(mask_list) / sizeof(u16)); i++)
   {
      scsp_timer_pending_test(mask_list[i], 0);
      if (stage_status != STAGESTAT_DONE)
         return;
   }
}

//////////////////////////////////////////////////////////////////////////////

void scsp_mcipd_test()
{
   int i;
   u16 mask_list[] = { 0x0040, 0x0080, 0x0100 };

   for (i = 0; i < (sizeof(mask_list) / sizeof(u16)); i++)
   {
      scsp_timer_pending_test(mask_list[i], 1);
      if (stage_status != STAGESTAT_DONE)
         return;
   }
}

//////////////////////////////////////////////////////////////////////////////

void scsp_timer_test(int timer_mask)
{
   // Disable everything temporarily
   SCSPREG_SCIEB = 0;
   SCSPREG_MCIEB = 0;

   // Mask SCSP interrupt temporarily
   bios_change_scu_interrupt_mask(0xFFFFFFFF, 0x40);

   // Set SCSP interrupt function
   bios_set_scu_interrupt(0x46, scsp_int_test_func);

   // Unmask SCSP interrupt
   bios_change_scu_interrupt_mask(~0x40, 0);

   stage_status = STAGESTAT_WAITINGFORINT;

   // Enable Timer interrupt
   SCSPREG_MCIRE = timer_mask;
   SCSPREG_MCIEB = timer_mask;
}

//////////////////////////////////////////////////////////////////////////////

void scsp_timer_a_test()
{
   scsp_timer_test(0x40);
}

//////////////////////////////////////////////////////////////////////////////

void scsp_timer_b_test()
{
   scsp_timer_test(0x80);
}

//////////////////////////////////////////////////////////////////////////////

void scsp_timer_c_test()
{
   scsp_timer_test(0x100);
}

//////////////////////////////////////////////////////////////////////////////

void scsp_timing_func(void)
{
   u32 hblank_temp;
   hblank_temp = hblank_counter;

//   vdp_printf(&test_disp_font, 1 * 8, 22 * 8, 0xF, "hblankcounter: %08X", hblanktemp);

   switch (tinc)
   {
      case 0:
         if ((hblank_temp & 0xFF00) < 0x100) // fix me
            stage_status = STAGESTAT_DONE;
         else
            stage_status = STAGESTAT_BADTIMING;
         break;
      case 1:
         if ((hblank_temp & 0xFF00) < 0x100) // fix me
            stage_status = STAGESTAT_DONE;
         else
            stage_status = STAGESTAT_BADTIMING;
         break;
      case 2:
         if ((hblank_temp & 0xFF00) == 0x100) // fix me
            stage_status = STAGESTAT_DONE;
         else
            stage_status = STAGESTAT_BADTIMING;
         break;
      case 3:
         if ((hblank_temp & 0xFF00) == 0x200) // fix me
            stage_status = STAGESTAT_DONE;
         else
            stage_status = STAGESTAT_BADTIMING;
         break;
      case 4:
         if ((hblank_temp & 0xFF00) == 0x500) // fix me
            stage_status = STAGESTAT_DONE;
         else
            stage_status = STAGESTAT_BADTIMING;
         break;
      case 5:
         if ((hblank_temp & 0xFF00) == 0xB00) // fix me
            stage_status = STAGESTAT_DONE;
         else
            stage_status = STAGESTAT_BADTIMING;
         break;
      case 6:
         if ((hblank_temp & 0xFF00) == 0x1600) // fix me
            stage_status = STAGESTAT_DONE;
         else
            stage_status = STAGESTAT_BADTIMING;
         break;
      case 7:
         if ((hblank_temp & 0xFF00) == 0x2D00) // fix me
            stage_status = STAGESTAT_DONE;
         else
            stage_status = STAGESTAT_BADTIMING;
      default:
         stage_status = STAGESTAT_DONE;
         break;
   }
}

//////////////////////////////////////////////////////////////////////////////

void hblank_timing_func(void)
{
   hblank_counter++;
}

//////////////////////////////////////////////////////////////////////////////

void scsp_timer_timing_test(u16 timer_setting)
{
   // Disable everything temporarily
   SCSPREG_SCIEB = 0;
   SCSPREG_MCIEB = 0;   

   tinc = timer_setting >> 8;

   interrupt_set_level_mask(0xF);

   // Mask SCSP/H-blank interrupts temporarily
   bios_change_scu_interrupt_mask(0xFFFFFFFF, 0x40 | 0x4);

   // Set SCSP interrupt function
   bios_set_scu_interrupt(0x46, scsp_timing_func);

   // Set Hblank-in interrupt function
   bios_set_scu_interrupt(0x42, hblank_timing_func);

   // Unmask SCSP/H-blank interrupts
   bios_change_scu_interrupt_mask(~(0x40 | 0x4), 0);

   hblank_counter = 0;
   vdp_vsync();

//   switch (timermask)
//   {
//      case 0x40:
         SCSPREG_TIMERA = timer_setting;
//         break;
//      case 0x80:
//         SCSPREG_TIMERB = timersetting;
//         break;
//      case 0x100:
//         SCSPREG_TIMERC = timersetting;
//         break;
//      default: break;
//   }
   interrupt_set_level_mask(0x8);

   stage_status = STAGESTAT_WAITINGFORINT;

   // Enable Timer interrupt
   SCSPREG_MCIRE = 0x0040;
   SCSPREG_MCIEB = 0x0040;
}

//////////////////////////////////////////////////////////////////////////////

void scsp_timer_timing_test0()
{
   scsp_timer_timing_test(0x0000);
}

//////////////////////////////////////////////////////////////////////////////

void scsp_timer_timing_test1()
{
   scsp_timer_timing_test(0x0100);
}

//////////////////////////////////////////////////////////////////////////////

void scsp_timer_timing_test2()
{
   scsp_timer_timing_test(0x0200);                                         
}

//////////////////////////////////////////////////////////////////////////////

void scsp_timer_timing_test3()
{
   scsp_timer_timing_test(0x0300);
}

//////////////////////////////////////////////////////////////////////////////

void scsp_timer_timing_test4()
{
   scsp_timer_timing_test(0x0400);
}

//////////////////////////////////////////////////////////////////////////////

void scsp_timer_timing_test5()
{
   scsp_timer_timing_test(0x0500);
}

//////////////////////////////////////////////////////////////////////////////

void scsp_timer_timing_test6()
{
   scsp_timer_timing_test(0x0600);
}

//////////////////////////////////////////////////////////////////////////////

void scsp_timer_timing_test7()
{
   scsp_timer_timing_test(0x0700);
}

//////////////////////////////////////////////////////////////////////////////

void int_on_timer_enable_test_func(void)
{
//   u32 hblanktemp;
//   hblanktemp = hblankcounter;

   if (hblank_counter == 0)
      stage_status = STAGESTAT_DONE;
   else
      stage_status = STAGESTAT_BADTIMING;

//   vdp_printf(&test_disp_font, 1 * 8, 23 * 8, 0xF, "hblankcounter: %08X", hblanktemp);
}

//////////////////////////////////////////////////////////////////////////////

void scsp_int_on_timer_enable_test()
{
   // Disable everything temporarily
   SCSPREG_SCIEB = 0;
   SCSPREG_MCIEB = 0;

   // Mask SCSP/H-blank interrupts temporarily
   bios_change_scu_interrupt_mask(0xFFFFFFFF, 0x40 | 0x4);

   // Set SCSP interrupt function
   bios_set_scu_interrupt(0x46, int_on_timer_enable_test_func);

   // Set Hblank-in interrupt function
   bios_set_scu_interrupt(0x42, hblank_timing_func);

   // Unmask SCSP/H-blank interrupts
   bios_change_scu_interrupt_mask(~(0x40 | 0x4), 0);

   // Clear timer
   SCSPREG_TIMERA = 0x0700; 
   SCSPREG_MCIRE = 0x40;

   // Wait until MCIPD is showing an interrupt pending
   while (!(SCSPREG_MCIPD & 0x40)) {}

   stage_status = STAGESTAT_WAITINGFORINT;

   // Enable Timer interrupt
   hblank_counter = 0;
   SCSPREG_MCIEB = 0x40;
}

//////////////////////////////////////////////////////////////////////////////

void scsp_int_on_timer_reset_test()
{
   // Disable everything temporarily
   SCSPREG_SCIEB = 0;
   SCSPREG_MCIEB = 0;

   // Mask SCSP interrupt temporarily
   bios_change_scu_interrupt_mask(0xFFFFFFFF, 0x40);

   // Set SCSP interrupt function
   bios_set_scu_interrupt(0x46, 0);

   // Unmask SCSP interrupt
   bios_change_scu_interrupt_mask(~0x40, 0);

   // Enable timer
   SCSPREG_TIMERA = 0x0700; 
   SCSPREG_MCIRE = 0x40;
   SCSPREG_MCIEB = 0x40;

   // Wait until MCIPD is showing an interrupt pending
   while (!(SCSPREG_MCIPD & 0x40)) {}

   // Set SCSP interrupt function
   bios_set_scu_interrupt(0x46, scsp_int_test_func);

   stage_status = STAGESTAT_WAITINGFORINT;

   // Reset Timer
   hblank_counter = 0;
   SCSPREG_MCIRE = 0x40;
}

//////////////////////////////////////////////////////////////////////////////

void scsp_int_dup_test()
{
   // Disable everything temporarily
   SCSPREG_SCIEB = 0;
   SCSPREG_MCIEB = 0;

   // Mask SCSP/H-blank interrupts temporarily
   bios_change_scu_interrupt_mask(0xFFFFFFFF, 0x40 | 0x4);

   // Set SCSP interrupt function
   bios_set_scu_interrupt(0x46, 0);

   // Set Hblank-in interrupt function
   bios_set_scu_interrupt(0x42, hblank_timing_func);

   // Unmask SCSP/H-blank interrupts
   bios_change_scu_interrupt_mask(~(0x40 | 0x4), 0);

   // Enable timer
   SCSPREG_TIMERA = 0; 
   SCSPREG_MCIRE = 0x40;
   SCSPREG_MCIEB = 0x40;

   // Wait until MCIPD is showing an interrupt pending
   while (!(SCSPREG_MCIPD & 0x40)) {}

   // Set SCSP interrupt function
   bios_set_scu_interrupt(0x46, scsp_int_test_func);

   // Wait around 0x2000 hblanks to make sure nothing else triggers the interrupt
   hblank_counter = 0;

   while (hblank_counter < 0x2000) {}

   if (stage_status == STAGESTAT_DONE)
      stage_status = STAGESTAT_BADINTERRUPT;
   else
      stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

u16 scsp_dsp_reg_test_pattern[8] = { 0xdead ,0xbeef,0xcafe ,0xbabe,0xba5e ,0xba11,0xf01d,0xab1e };

void scsp_dsp_do_reg_write(volatile u16* ptr, int length)
{
   int i;
   for (i = 0; i < length; i++)
   {
      ptr[i] = scsp_dsp_reg_test_pattern[i&7];
   }
}

//////////////////////////////////////////////////////////////////////////////

int scsp_dsp_check_reg(volatile u16* ptr, int length, int mask, char* name, int pos)
{
   int i;
   for (i = 0; i < length; i++)
   {
      u16 ptr_val = ptr[i];
      u16 correct_val = scsp_dsp_reg_test_pattern[i & 7] & mask;
      if (ptr_val != correct_val)
      {
         char str[64] = { 0 };
         sprintf(str, "%d %s %04X != %04X", i, name, ptr_val, correct_val);
         vdp_printf(&test_disp_font, 0 * 8, pos * 8, 40, str);
         return 1;
      }
   }
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void scsp_dsp_register_write_test()
{
   volatile u16* coef_ptr = (volatile u16 *)0x25B00700;
   volatile u16* madrs_ptr = (volatile u16 *)0x25B00780;
   volatile u16* mpro_ptr = (volatile u16 *)0x25B00800;
   int failure = 0;
   int coef_length = 64;
   int madrs_length = 32;
   int mpro_length = 128;

   scsp_dsp_do_reg_write(coef_ptr, coef_length);
   scsp_dsp_do_reg_write(madrs_ptr, madrs_length);
   scsp_dsp_do_reg_write(mpro_ptr, mpro_length);

   int i;
   for (i = 0; i < 28; i++)
   {
      vdp_printf(&test_disp_font, 0 * 8, i * 8, 0xF, "coef: %04X", coef_ptr[i]);
      vdp_printf(&test_disp_font, 12 * 8, i * 8, 0xF, "madrs: %04X", madrs_ptr[i]);
      vdp_printf(&test_disp_font, 24 * 8, i * 8, 0xF, "mpro: %04X", mpro_ptr[i]);
   }

   if (scsp_dsp_check_reg(coef_ptr, coef_length, 0xfff8, "COEF", 0))
      failure = 1;
   if (scsp_dsp_check_reg(madrs_ptr, madrs_length, 0xffff, "MADRS", 1))
      failure = 1;
   if (scsp_dsp_check_reg(mpro_ptr, mpro_length, 0xffff, "MPRO", 2))
      failure = 1;

#ifndef BUILD_AUTOMATED_TESTING
   for (;;)
   {
      vdp_vsync();

      //start to advance to the next test
      if (per[0].but_push_once & PAD_START)
      {
         int q;
         u32* dest = (u32 *)VDP2_RAM;

         //clear framebuffer
         for (q = 0; q < 0x10000; q++)
         {
            dest[q] = 0;
         }
         break;
      }
   }
#endif

   if (!failure)
   {
      stage_status = STAGESTAT_DONE;
   }
   else
   {
      stage_status = STAGESTAT_BADDATA;
   }
}

//////////////////////////////////////////////////////////////////////////////

void scsp_dsp_write_mpro(u16 instr0, u16 instr1, u16 instr2, u16 instr3, u32 addr)
{
   volatile u16* mpro_ptr = (u16 *)0x25b00800;
   addr *= 4;
   mpro_ptr[addr+0] = instr0;
   mpro_ptr[addr+1] = instr1;
   mpro_ptr[addr+2] = instr2;
   mpro_ptr[addr+3] = instr3;
}

//////////////////////////////////////////////////////////////////////////////

void scsp_dsp_clear_instr(u16 *instr0, u16 *instr1, u16 *instr2, u16 *instr3)
{
   *instr0 = 0;
   *instr1 = 0;
   *instr2 = 0;
   *instr3 = 0;
}

//////////////////////////////////////////////////////////////////////////////

void scsp_dsp_erase_mpro()
{
   volatile u16* mpro_ptr = (u16 *)0x25b00800;
   int i;

   for (i = 0; i < 128 * 4; i++)
   {
      mpro_ptr[i] = 0;
   }
}

//////////////////////////////////////////////////////////////////////////////

void scsp_dsp_clear()
{
   volatile u16* sound_ram_ptr = (u16 *)0x25a00000;
   volatile u16* coef_ptr = (u16 *)0x25b00700;
   volatile u16* mems_ptr = (u16 *)0x25b00C00;
   volatile u16* temp_ptr = (u16 *)0x25b00E00;
   int i;

   scsp_dsp_erase_mpro();

   for (i = 0; i < 27; i++)
   {
      sound_ram_ptr[i] = 0;
      mems_ptr[i] = 0;
      temp_ptr[i] = 0;
      coef_ptr[i] = 0;
   }
}

//////////////////////////////////////////////////////////////////////////////

void scsp_dsp_start_test(u16 a_val, u16 b_val, int*yval)
{

   volatile u16* sound_ram_ptr = (u16 *)0x25a00000;
   volatile u16* coef_ptr = (u16 *)0x25b00700;
   volatile u16* start_dsp = (u16 *)0x25b00BF0;

   u16 instr0 = 0;
   u16 instr1 = 0;
   u16 instr2 = 0;
   u16 instr3 = 0;

   scsp_dsp_clear();

   coef_ptr[0] = a_val;

   sound_ram_ptr[0] = b_val;

   //nop
   scsp_dsp_write_mpro(0, 0, 0, 0, 0);

   //set memval
   instr2 = 1 << 13;//set mrd
   instr2 |= 1 << 15;// set table
   instr3 = 1 << 15; //set nofl

   scsp_dsp_write_mpro(0, 0, instr2, instr3, 1);

   scsp_dsp_clear_instr(&instr0, &instr1, &instr2, &instr3);

   //load memval to inputs
   instr1 = 1 << 5;//set iwt
   //ira and iwa will be 0 and equal to each other

   scsp_dsp_write_mpro(0, instr1, 0, 0, 2);

   scsp_dsp_clear_instr(&instr0, &instr1, &instr2, &instr3);

   scsp_dsp_write_mpro(0, 0, 0, 0, 3);//nop

   //load inputs to x, load coef to y, set B to zero
   instr1 = 1 << 15;//set xsel
   instr1 |= 1 << 13;//set ysel to 1
   //coef == 0
   instr2 = 1 << 1;//zero

   scsp_dsp_write_mpro(0, instr1, instr2, 0, 4);

   scsp_dsp_clear_instr(&instr0, &instr1, &instr2, &instr3);

   //needs to be on an odd instruction
   instr2 |= 1 << 14;//set mwt
   instr3 |= 1;//set nxadr
   instr2 |= 1 << 15; //set table
   instr3 |= 1 << 15; //set nofl

   scsp_dsp_write_mpro(instr0, instr1, instr2, instr3, 5);

   //start the dsp
   start_dsp[0] = 1;

   //wait a frame
   vdp_vsync();

   vdp_printf(&test_disp_font, 0 * 8, *yval * 8, 0xF, "a: 0x%04x * b: 0x%04x = 0x%04x", a_val,b_val, sound_ram_ptr[1]);
   *yval = *yval + 1;
}

//////////////////////////////////////////////////////////////////////////////

void scsp_dsp_multiply_test()
{
   int i, j;
   int yval = 0;
   u16 test_vals[] = { 0x0100,0x0200,0x0400,0x0800,0x1000};

   for (i = 0; i < 5; i++)
   {
      for (j = 0; j < 5; j++)
      {
         scsp_dsp_start_test(test_vals[i], test_vals[j], &yval);
      }
   }

   for (;;)
   {
      vdp_vsync();

      if (per[0].but_push_once & PAD_START)
      {
         reset_system();
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

void scsp_dsp_test()
{
   scsp_minimal_init();
   unregister_all_tests();
   register_test(&scsp_dsp_register_write_test, "Register Read/Write Test");
   do_tests("SCSP DSP tests", 0, 0);
}

//////////////////////////////////////////////////////////////////////////////

u16 square_table[] =
{
   0xc000, 0xc000, 0xc000, 0xc000, 0xc000, 0xc000, 0xc000, 0xc000,
   0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000
};

#define KEYONEX 0x10000000
#define KEYONB 0x08000000

u16 fns_table[] = {
   0x0,0x3d,0x7d,0xc2,0x10a,0x157,0x1a8,0x1fe,0x25a,0x2ba,0x321,0x38d
};

//////////////////////////////////////////////////////////////////////////////

void write_note(u8 which_slot, u32 sa, u16 lsa, u16 lea, u8 oct, u16 fns, u8 pcm8b, u8 loop)
{
   volatile u16* slot_ptr = (u16 *)0x25b00000;
   volatile u32* slot_ptr_l = (u32 *)0x25b00000;

   int offset = which_slot * 0x10;

   //force a note off
   slot_ptr_l[which_slot * 0x8] = KEYONEX;

   slot_ptr_l[which_slot*0x8] = KEYONEX | KEYONB | sa | (pcm8b << 20) | (loop << 21);
   slot_ptr[offset + 2] = lsa;//lsa
   slot_ptr[offset + 3] = lea;//lea
   slot_ptr[offset + 4] = 0x001f;//d2r,d1r,ho,ar
   slot_ptr[offset + 5] = 0x001f;//ls,krs,dl,rr
   slot_ptr[offset + 6] = 0x0000;//si,sd,tl
   slot_ptr[offset + 7] = 0x0000;//mdl,mdxsl,mdysl
   slot_ptr[offset + 8] = (oct << 11) | fns;//oct,fns
   slot_ptr[offset + 9] = 0x0000;//re,lfof,plfows,plfos,alfows,alfos
   slot_ptr[offset + 10] = 0x0000;//isel,imxl
   slot_ptr[offset + 11] = 0xE000;//disdl,dipan,efsdl,efpan
}

//////////////////////////////////////////////////////////////////////////////

void scsp_test_note_on()
{
   volatile u32* slot_ptr_l = (u32 *)0x25b00000;
   volatile u16* sound_ram_ptr = (u16 *)0x25a00000;
   volatile u16* control = (u16 *)0x25b00400;
   int oct = 0xd;
   int which_note = 0;
   int major_scale[] = { 0,2,4,5,7,9,11 };
   int i;

   for (i = 0; i < 16; i++)
   {
      sound_ram_ptr[i] = square_table[i];
   }

   control[0] = 7;//master vol

   for (;;)
   {
      vdp_vsync();

      int scale_note = major_scale[which_note];
      int fns = fns_table[scale_note];

      //key on
      if (per[0].but_push_once & PAD_A)
      {
         write_note(0, 0, 0, 0x10 - 1, oct, fns, 0, 1);
         which_note++;
         if (which_note > 6)
            which_note = 0;
      }

      if (per[0].but_push_once & PAD_X)
      {
         write_note(1, 0, 0, 0x10 - 1, oct, fns, 0, 1);
         which_note++;
         if (which_note > 6)
            which_note = 0;
      }

      //key off
      if (per[0].but_push_once & PAD_B)
      {
         slot_ptr_l[0] = KEYONB;
      }

      if (per[0].but_push_once & PAD_START)
      {
         reset_system();
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

/*
void mcipd_test_func()
{
   u16 mcipd_temp=SCSPREG_MCIPD;

   vdp_printf(&test_disp_font, 1 * 8, 22 * 8, 0xF, "MCIPD: %04X", mcipd_temp);
}

//////////////////////////////////////////////////////////////////////////////

void scsp_mcipd_test()
{
   // Disable everything temporarily
   SCSPREG_SCIEB = 0;
   SCSPREG_MCIEB = 0;

   // Mask SCSP/H-blank interrupts temporarily
   bios_change_scu_interrupt_mask(0xFFFFFFFF, 0x40);

   // Set SCSP interrupt function
   bios_set_scu_interrupt(0x46, mcipd_test_func);

   // Unmask SCSP/H-blank interrupts
   bios_change_scu_interrupt_mask(~0x40, 0);

   stagestatus = STAGESTAT_WAITINGFORINT;

   // Enable timer
   SCSPREG_TIMERA = 0x0700;
   SCSPREG_MCIRE = 0x40;
   SCSPREG_MCIEB = 0x40;
}

//////////////////////////////////////////////////////////////////////////////

*/
