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
#include "cdb.h"

void cd_write_command(cd_cmd_struct *cd_cmd);
static int audio_track=3;

#define do_cdb_tests_unexp_cr_data_error() \
   do_tests_unexp_data_error("%04X %04X %04X %04X %04X", CDB_REG_HIRQ, cd_cmd_rs.CR1, cd_cmd_rs.CR2, cd_cmd_rs.CR3, cd_cmd_rs.CR4);

//////////////////////////////////////////////////////////////////////////////

enum IAPETUS_ERR init_cdb_tests()
{
   interrupt_set_level_mask(0xF);

   init_iapetus(RES_320x224);
   vdp_rbg0_init(&test_disp_settings);
   vdp_set_default_palette();

   // Display On
   vdp_disp_on();

   if (!is_cd_present())
      return IAPETUS_ERR_CDNOTFOUND;

   if (!is_cd_auth(NULL))
   {
      cd_auth();

      if (!is_cd_auth(NULL))
         return IAPETUS_ERR_AUTH;
   }

   return IAPETUS_ERR_OK;
}

//////////////////////////////////////////////////////////////////////////////

void cdb_test()
{
   int choice;

   menu_item_struct cdb_menu[] = {
   { "CD Commands", &cd_cmd_test, },
   { "Misc CD Block" , &misc_cd_test, },
   { "\0", NULL }
   };

   for (;;)
   {
      choice = gui_do_menu(cdb_menu, &test_disp_font, 0, 0, "CD Block Tests", MTYPE_CENTER, -1);
      gui_clear_scr(&test_disp_font);
      if (choice == -1)
         break;
   }   
}

//////////////////////////////////////////////////////////////////////////////

void cd_cmd_test()
{
   init_cdb_tests();

   unregister_all_tests();
   register_test(&test_cdb_mbx, "CDB mbx");
   register_test(&test_cmd_init_cd, "Initialize CD System");
   register_test(&test_cmd_cd_status, "Get CD Status");
   register_test(&test_cmd_get_hw_info, "Get Hardware Info");
   register_test(&test_cmd_get_toc, "Get TOC");
   register_test(&test_cmd_end_data_transfer, "End Data Transfer");
   register_test(&test_cmd_get_session_info, "Get Session Info");
   register_test(&test_cmd_open_tray, "Open Tray");
   register_test(&test_cmd_play_disc, "Play Disc");
   register_test(&test_cmd_seek_disc, "Seek Disc");
   register_test(&test_cmd_scan_disc, "Scan Disc");
   register_test(&test_cmd_get_subcode_qrw, "Get Subcode Q/RW");
   register_test(&test_cmd_set_cddev_con, "Set CD Device Connection");
   register_test(&test_cmd_get_cd_dev_con, "Get CD Device Connection");
   register_test(&test_cmd_get_last_buffer, "Get Last Buffer Dest");
   register_test(&test_cmd_set_filter_range, "Set Filter Range");
   register_test(&test_cmd_get_filter_range, "Get Filter Range");
   register_test(&test_cmd_set_filter_sh_cond, "Set Filter SH Conditions");
   register_test(&test_cmd_get_filter_sh_cond, "Get Filter SH Conditions");
   register_test(&test_cmd_set_filter_mode, "Set Filter Mode");
   register_test(&test_cmd_get_filter_mode, "Get Filter Mode");
   register_test(&test_cmd_set_filter_con, "Set Filter Connection");
   register_test(&test_cmd_get_filter_con, "Get Filter Connection");
   register_test(&test_cmd_reset_selector, "Reset Selector");
   register_test(&test_cmd_get_buffer_size, "Get Buffer Size");
   register_test(&test_cmd_get_sector_number, "Get Sector Number");
   register_test(&test_cmd_calculate_actual_size, "Calculate Actual Size");
   register_test(&test_cmd_get_actual_size, "Get Actual Size");
   register_test(&test_cmd_get_sector_info, "Get Sector Info");
   register_test(&test_cmd_exec_fad_search, "Execute FAD Search");
   register_test(&test_cmd_get_fad_search_results, "Get FAD Search Results");
   //register_test(&test_cmd_set_sector_length, "Set Sector Length");
   register_test(&test_cmd_get_sector_data, "Get Sector Data");
   register_test(&test_cmd_del_sector_data, "Delete Sector Data");
   register_test(&test_cmd_get_then_del_sector_data, "Get Then Delete Sector Data");
   register_test(&test_cmd_put_sector_data, "Put Sector Data");
   register_test(&test_cmd_copy_sector_data, "Copy Sector Data");
   register_test(&test_cmd_move_sector_data, "Move Sector Data");
   register_test(&test_cmd_get_copy_error, "Get Copy Error");
   register_test(&test_cmd_change_directory, "Change Directory");
   register_test(&test_cmd_read_directory, "Read Directory");
   register_test(&test_cmd_get_file_system_scope, "Get File System Scope");
   register_test(&test_cmd_get_file_info, "Get File Info");
   register_test(&test_cmd_read_file, "Read File");
   register_test(&test_cmd_abort_file, "Abort File");
   do_tests("CD Command tests", 0, 0);
}

//////////////////////////////////////////////////////////////////////////////

int cdb_mbx_init()
{
   cd_cmd_struct cd_cmd;
   u32 old_level_mask;
   u16 hirq_temp;

   cd_cmd.CR1 = 0x0100; // Just use Get Hardware Info
   cd_cmd.CR2 = cd_cmd.CR3 = cd_cmd.CR4 = 0x0000;

   // Mask any interrupts, we don't need to be interrupted
   old_level_mask = interrupt_get_level_mask();
   interrupt_set_level_mask(0xF);

   hirq_temp = CDB_REG_HIRQ;

   // Make sure CMOK flag is set, or we can't continue
   if (!(hirq_temp & HIRQ_CMOK))
      return IAPETUS_ERR_CMOK;

   // Clear CMOK and any other user-defined flags
   CDB_REG_HIRQ = ~(HIRQ_CMOK);

   // Alright, time to execute the command
   cd_write_command(&cd_cmd);

   // Let's wait till the command operation is finished
   if (!cd_wait_hirq(HIRQ_CMOK))
      return IAPETUS_ERR_TIMEOUT;

   // return interrupts back to normal
   interrupt_set_level_mask(old_level_mask);
   return IAPETUS_ERR_OK;
}

#define MAX_MBX_TIMEOUT 0xFFFF
void test_cdb_mbx()
{
   int ret;
   u32 i, j;
   u32 cr_temp;
   int expect_timeout=0, timeout=0;

   for (i = 0; i < 9; i++)
   {
      if ((ret = cdb_mbx_init()) != IAPETUS_ERR_OK)
      {
         do_tests_error(ret, "%d", i);
         return;
      }

      switch (i)
      {
         case 0:
            cr_temp = CDB_REG_CR1;
            expect_timeout = 1;
            break;
         case 1:
            cr_temp = CDB_REG_CR2;
            expect_timeout = 1;
            break;
         case 2:
            cr_temp = CDB_REG_CR3;
            expect_timeout = 1;
            break;
         case 3:
            cr_temp = CDB_REG_CR4;
            expect_timeout = 0;
            break;
         case 4:
            cr_temp = CDB_REG_CR1;
            cr_temp = CDB_REG_CR2;
            cr_temp = CDB_REG_CR3;
            expect_timeout = 1;
            break;
         case 5:
            cr_temp = *((volatile u32 *)0x25890018);
            expect_timeout = 1;
            break;
         case 6:
            cr_temp = *((volatile u32 *)0x2589001C);
            expect_timeout = 1;
            break;
         case 7:
            cr_temp = *((volatile u32 *)0x25890020);
            expect_timeout = 1;
            break;
         case 8:
            cr_temp = *((volatile u32 *)0x25890024);
            expect_timeout = 0;
            break;
         default: break;
      }

      for (j = 0; j < MAX_MBX_TIMEOUT; j++) 
      {
         if ((CDB_REG_CR1 & 0xF000) == 0x2000)
         {
            timeout = 0;
            break;
         }
         else if (j == MAX_MBX_TIMEOUT-1)
         {
            timeout = 1;
            break;
         }
      }

      cr_temp = CDB_REG_CR4;

      if (timeout != expect_timeout)
      {
         tests_disp_iapetus_error(IAPETUS_ERR_TIMEOUT, __FILE__, __LINE__, "test %d failed", i);
         stage_status = STAGESTAT_BADTIMING;
         return;
      }
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

int cd_status_rs(cd_cmd_struct *cd_cmd_rs)
{
   cd_cmd_struct cd_cmd;
   int ret;

   cd_cmd.CR1 = cd_cmd.CR2 = cd_cmd.CR3 = cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(0, &cd_cmd, cd_cmd_rs)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return FALSE;
   }
   return TRUE;
}

//////////////////////////////////////////////////////////////////////////////

int cd_wait_status_rs(u8 status, int timeout, int wait, cd_cmd_struct *cd_cmd_rs)
{
   int i,j;
   int ret;
   char text[256];

   for (i = 0; i < timeout; i++)
   {
      vdp_vsync();
      if ((ret=cd_status_rs(cd_cmd_rs)) == TRUE)
      {
         if ((cd_cmd_rs->CR1 >> 8) == status)
         {
            for (j = 0; j < wait; j++)
               vdp_vsync();
            return ret;
         }
      }
   }

   return cd_status_rs(cd_cmd_rs);
}

//////////////////////////////////////////////////////////////////////////////

int cd_end_transfer_rs(cd_cmd_struct *cd_cmd_rs, u16 hirq_mask)
{
   int ret;
   cd_cmd_struct cd_cmd;

   cd_cmd.CR1 = 0x0600;
   cd_cmd.CR2 = cd_cmd.CR3 = cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(HIRQ_DRDY | hirq_mask, &cd_cmd, cd_cmd_rs)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return FALSE;
   }

   return TRUE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_cd_status()
{
   cd_cmd_struct cd_cmd_rs;

   if (!cd_status_rs(&cd_cmd_rs))
      return;

   // Verify that the data returned is correct
   if ((cd_cmd_rs.CR1 & 0xFF00) != (STATUS_PAUSE << 8) ||
       cd_cmd_rs.CR2 != 0x4101 ||
       (cd_cmd_rs.CR3 & 0xFF00) != 0x0100)
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_get_hw_info()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x0100;
   cd_cmd.CR2 = cd_cmd.CR3 = cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(0, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   // Verify that the data returned is correct
   if (cd_cmd_rs.CR1 != (STATUS_PAUSE << 8) ||
       (cd_cmd_rs.CR2 & 0xFD00) != 0x0000)
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_get_toc()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;
   u32 toc[102];
   u16 *ptr=(u16 *)toc;
   int i;

   cd_cmd.CR1 = 0x0200;
   cd_cmd.CR2 = cd_cmd.CR3 = cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(HIRQ_DRDY, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   // Verify that the data returned is correct
   if (cd_cmd_rs.CR2 != 0xCC)
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   if (!cd_wait_hirq(HIRQ_DRDY))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   // Read out TOC and verify
   for (i = 0; i < cd_cmd_rs.CR2; i++)
      ptr[i] = CDB_REG_DATATRNSW;      

   if (toc[0] != 0x41000096 || 
       (toc[1] & 0xFF000000) != 0x41000000 || 
       (toc[2] & 0xFF000000) != 0x01000000 ||
       toc[3] != 0xFFFFFFFF ||
       toc[99] != 0x41010000 ||
       toc[100] != 0x01030000 ||
       (toc[101] & 0xFF000000) != 0x01000000 )
   {
      do_tests_unexp_data_error("%08X %08X %08X %08X %08X %08X %08X", toc[0], toc[1], toc[2], toc[3], toc[99], toc[100], toc[101]);
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_get_session_info()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x0300;
   cd_cmd.CR2 = cd_cmd.CR3 = cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(0, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   // Verify that the data returned is correct
   if (cd_cmd_rs.CR1 != (STATUS_PAUSE << 8) ||
      cd_cmd_rs.CR2 != 0x0000 ||
      (cd_cmd_rs.CR3 & 0xFF00) != 0x0100)
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   // Test first session
   cd_cmd.CR1 = 0x0301;
   cd_cmd.CR2 = cd_cmd.CR3 = cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(0, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   // Verify that the data returned is correct
   if (cd_cmd_rs.CR1 != 0x0100 ||
      cd_cmd_rs.CR2 != 0x0000 ||
      cd_cmd_rs.CR3 != 0x0100 ||
      cd_cmd_rs.CR4 != 0x0000)
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_init_cd()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x0400;
   cd_cmd.CR2 = 0x0001; // almost instant stop
   cd_cmd.CR3 = 0x0000;
   cd_cmd.CR4 = 0x040F;

   if ((ret = cd_exec_command(0, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   // Verify that the data returned is correct
   if ((cd_cmd_rs.CR1 & 0xFF00) != (STATUS_BUSY << 8))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   if (!cd_wait_hirq(HIRQ_EFLS))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   // Wait a couple of seconds, then check status
   if (!cd_wait_status_rs(STATUS_STANDBY, 60 * 5, 60 * 2, &cd_cmd_rs))
      return;

   // Verify that the data returned is correct
   if ((cd_cmd_rs.CR1 & 0xFF00) != (STATUS_STANDBY << 8))
   {
      cd_start_drive();
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   // back to default settings
   if ((ret = cd_cdb_init(0)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   if (!cd_wait_status_rs(STATUS_PAUSE, 60 * 5, 60 * 1, &cd_cmd_rs))
      return;

   if ((cd_cmd_rs.CR1 & 0xFF00) != (STATUS_PAUSE << 8))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_open_tray()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x0500;
   cd_cmd.CR2 = cd_cmd.CR3 = cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(0, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   if (!cd_wait_hirq(HIRQ_EFLS | HIRQ_DCHG))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   if (!cd_status_rs(&cd_cmd_rs))
      return;

   // Verify that the data returned is correct
   if ((cd_cmd_rs.CR1 & 0xFF00) != (STATUS_BUSY << 8))
   {
      cd_cdb_init(0);
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   cd_cdb_init(0);

   if (!cd_wait_hirq(HIRQ_EFLS))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   if (!cd_wait_status_rs(STATUS_PAUSE, 60 * 5, 0, &cd_cmd_rs))
      return;

   if ((cd_cmd_rs.CR1 & 0xFF00) != (STATUS_PAUSE << 8))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   // Re-authenticate disc since open command resets status
   if ((ret = cd_auth()) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_end_data_transfer()
{
   cd_cmd_struct cd_cmd_rs;
   if (!cd_end_transfer_rs(&cd_cmd_rs, 0))
      return;

   if (cd_cmd_rs.CR2 != 0xCC)
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_play_disc()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   // Enable SCSP audio
   sound_external_audio_enable(7, 7);

   cd_cmd.CR1 = 0x1000;
   cd_cmd.CR2 = (audio_track << 8) | 0x01;
   cd_cmd.CR3 = 0x0000;
   cd_cmd.CR4 = (audio_track << 8) | 0x63;

   if ((ret = cd_exec_command(HIRQ_PEND|HIRQ_CSCT, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   // Wait a few seconds, then check status
   if (!cd_wait_status_rs(STATUS_PLAY, 60 * 5, 60 * 4, &cd_cmd_rs))
      return;

   if (cd_cmd_rs.CR1 != (STATUS_PLAY << 8) ||
       cd_cmd_rs.CR2 != (0x0100 | audio_track) ||
       (cd_cmd_rs.CR3 & 0xFF00) != 0x0100)
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_seek_disc()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x1100;
   cd_cmd.CR2 = 0x0101;
   cd_cmd.CR3 = cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(HIRQ_PEND|HIRQ_CSCT, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   // Wait a few seconds, then check status
   if (!cd_wait_status_rs(STATUS_PAUSE, 60 * 5, 0, &cd_cmd_rs))
      return;

   if (cd_cmd_rs.CR1 != (STATUS_PAUSE << 8) ||
      cd_cmd_rs.CR2 != 0x4101 ||
      (cd_cmd_rs.CR3 & 0xFF00) != 0x0100)
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_scan_disc()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   // Start playing
   sound_external_audio_enable(7, 7);

   cd_cmd.CR1 = 0x1000;
   cd_cmd.CR2 = (audio_track << 8) | 0x01;
   cd_cmd.CR3 = 0x0000;
   cd_cmd.CR4 = (audio_track << 8) | 0x63;

   if ((ret = cd_exec_command(HIRQ_PEND|HIRQ_CSCT, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   // Wait until it starts playing
   if (!cd_wait_status_rs(STATUS_PLAY, 60 * 5, 0, &cd_cmd_rs))
      return;

   // Forward
   cd_cmd.CR1 = 0x1200;
   cd_cmd.CR2 = cd_cmd.CR3 = cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(0, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   // Wait a few seconds, then check status
   if (!cd_wait_status_rs(STATUS_SCAN, 60 * 5, 60 * 0, &cd_cmd_rs))
      return;

   if (cd_cmd_rs.CR1 != (STATUS_SCAN << 8) ||
      cd_cmd_rs.CR2 != (0x0100 | audio_track) ||
      (cd_cmd_rs.CR3 & 0xFF00) != 0x0100)
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   // Reverse
   cd_cmd.CR1 = 0x1201;
   cd_cmd.CR2 = cd_cmd.CR3 = cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(0, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   // Wait a few seconds, then check status
   if (!cd_wait_status_rs(STATUS_SCAN, 60 * 5, 15, &cd_cmd_rs))
      return;

   if (cd_cmd_rs.CR1 != (STATUS_SCAN << 8) ||
      cd_cmd_rs.CR2 != (0x0100 | audio_track) ||
      (cd_cmd_rs.CR3 & 0xFF00) != 0x0100)
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   // Stop scanning
   if ((ret = cd_seek_fad(150)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   if (!cd_wait_status_rs(STATUS_PAUSE, 60 * 5, 0, &cd_cmd_rs))
      return;

   if (cd_cmd_rs.CR1 != (STATUS_PAUSE << 8))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_get_subcode_qrw()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret, i;
   u16 subcode[12];

   cd_cmd.CR1 = 0x2000;
   cd_cmd.CR2 = cd_cmd.CR3 = cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(HIRQ_DRDY, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   if (!cd_wait_hirq(HIRQ_DRDY))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   // Read out Subcode and verify
   for (i = 0; i < cd_cmd_rs.CR2; i++)
      subcode[i] = CDB_REG_DATATRNSW;

   if (!cd_end_transfer_rs(&cd_cmd_rs, 0))
      return;

   if (cd_cmd_rs.CR2 != 5 ||
       subcode[0] != 0x4101 ||
       subcode[1] != 0x0100 ||
       subcode[2] != 0x0000 ||
       subcode[3] != 0x0000 ||
       subcode[4] != 0x0096)
   {
      do_tests_unexp_data_error("%04X %04X %04X %04X %04X %04X %04X %04X %04X %04X", 
      CDB_REG_HIRQ, cd_cmd_rs.CR1, cd_cmd_rs.CR2, cd_cmd_rs.CR3, cd_cmd_rs.CR4, subcode[0], subcode[1], subcode[2], subcode[3], subcode[4]);
      return;
   }

#if 0
   cd_cmd.CR1 = 0x2001;
   cd_cmd.CR2 = cd_cmd.CR3 = cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(HIRQ_DRDY, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {
      do_cdb_tests_error(ret);
      return;
   }

   if (!cd_wait_hirq(HIRQ_DRDY))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   // Read out Subcode and verify
   for (i = 0; i < cd_cmd_rs.CR2; i++)
      subcode[i] = CDB_REG_DATATRNSW;      

   if (!cd_end_transfer_rs(&cd_cmd_rs))
      return;

   if (cd_cmd_rs.CR2 != 12)
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }
#endif

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_set_cddev_con()
{
   int ret;

   if ((ret = cd_connect_cd_to_filter(3)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_get_cd_dev_con()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x3100;
   cd_cmd.CR2 = cd_cmd.CR3 = cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(HIRQ_ESEL, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   if ((cd_cmd_rs.CR3 >> 8) != 0x03)
   {   
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_get_last_buffer()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x3200;
   cd_cmd.CR2 = cd_cmd.CR3 = cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(0, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   // Check returned status to see if it's right
   if (cd_cmd_rs.CR1 != (STATUS_PAUSE << 8) ||
       cd_cmd_rs.CR2 != 0x00 ||
       (cd_cmd_rs.CR3 >= 0x1800 && cd_cmd_rs.CR3 != 0xFF00) ||
       cd_cmd_rs.CR4 != 0x00)
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_set_filter_range()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x4000;
   cd_cmd.CR2 = 0x0098;
   cd_cmd.CR3 = 0x0100;
   cd_cmd.CR4 = 0x0097;

   if ((ret = cd_exec_command(HIRQ_ESEL, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   if (!cd_wait_hirq(HIRQ_ESEL))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_get_filter_range()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x4100;
   cd_cmd.CR2 = cd_cmd.CR4 = 0x0000;
   cd_cmd.CR3 = 0x0100;

   if ((ret = cd_exec_command(0, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   // Check returned status to see if it's right
   if (cd_cmd_rs.CR1 != 0x0100 ||
       cd_cmd_rs.CR2 != 0x0098 ||
       cd_cmd_rs.CR3 != 0x0100 ||
       cd_cmd_rs.CR4 != 0x0097)
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_set_filter_sh_cond()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x4202;
   cd_cmd.CR2 = 0x0304;
   cd_cmd.CR3 = 0x0105;
   cd_cmd.CR4 = 0x0607;

   if ((ret = cd_exec_command(HIRQ_ESEL, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   if (!cd_wait_hirq(HIRQ_ESEL))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_get_filter_sh_cond()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x4300;
   cd_cmd.CR2 = cd_cmd.CR4 = 0x0000;
   cd_cmd.CR3 = 0x0100;

   if ((ret = cd_exec_command(0, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   // Check returned status to see if it's right
   if (cd_cmd_rs.CR1 != 0x0102 ||
      cd_cmd_rs.CR2 != 0x0304 ||
      cd_cmd_rs.CR3 != 0x0105 ||
      cd_cmd_rs.CR4 != 0x0607)
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_set_filter_mode()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x447F;
   cd_cmd.CR3 = 0x0300;
   cd_cmd.CR2 = cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(HIRQ_ESEL, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   if (!cd_wait_hirq(HIRQ_ESEL))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_get_filter_mode()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x4500;
   cd_cmd.CR3 = 0x0300;
   cd_cmd.CR2 = cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(0, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   if (cd_cmd_rs.CR1 != 0x017F ||
       cd_cmd_rs.CR2 != 0x0000 ||
       cd_cmd_rs.CR3 != 0x0300 ||
       cd_cmd_rs.CR4 != 0x0000)
   {   
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_set_filter_con()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x4602;
   cd_cmd.CR2 = 0x0304;
   cd_cmd.CR3 = 0x0100;
   cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(HIRQ_ESEL, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   if (!cd_wait_hirq(HIRQ_ESEL))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_get_filter_con()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x4700;
   cd_cmd.CR3 = 0x0100;
   cd_cmd.CR2 = cd_cmd.CR4 = 0x0000;
   
   if ((ret = cd_exec_command(0, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   if (cd_cmd_rs.CR1 != 0x0100 ||
      cd_cmd_rs.CR2 != 0x0104 ||
      cd_cmd_rs.CR3 != 0x0100 ||
      cd_cmd_rs.CR4 != 0x0000)
   {   
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_reset_selector()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x48FC;
   cd_cmd.CR2 = cd_cmd.CR3 = cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(HIRQ_ESEL, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   if (!cd_wait_hirq(HIRQ_ESEL))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_get_buffer_size()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   // Connect CD device to filter 0
   if ((ret = cd_connect_cd_to_filter(1)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   if (!cd_wait_hirq(HIRQ_ESEL))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   // Start reading sectors
   if ((ret = cd_play_fad(1, 150, 16)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   // Wait until play finished
   if (!cd_wait_hirq(HIRQ_PEND))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   cd_cmd.CR1 = 0x5000;
   cd_cmd.CR2 = cd_cmd.CR3 = cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(0, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   if (cd_cmd_rs.CR1 != 0x0100 ||
      cd_cmd_rs.CR2 != 0x00A8 || // Should only be 168 blocks free since we 
                                 // fetched 16 sectors to two different 
                                 // connected filters(0/1)
      cd_cmd_rs.CR3 != 0x1800 ||
      cd_cmd_rs.CR4 != 0x00C8)
   {   
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_get_sector_number()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;
   
   cd_cmd.CR1 = 0x5100;
   cd_cmd.CR3 = 0x0100;
   cd_cmd.CR2 = cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(0, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   if (cd_cmd_rs.CR1 != 0x0100 ||
       cd_cmd_rs.CR2 != 0x0000 ||
       cd_cmd_rs.CR3 != 0x0000 ||
       cd_cmd_rs.CR4 != 0x0020) // Should be 16*2 sectors stored
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

int cd_calc_actual_size_rs(cd_cmd_struct *cd_cmd_rs, int sector_offset, int buf_no, int size)
{
   cd_cmd_struct cd_cmd;

   cd_cmd.CR1 = 0x5200;
   cd_cmd.CR2 = 0x0000;
   cd_cmd.CR3 = (buf_no << 8);
   cd_cmd.CR4 = size;

   return cd_exec_command(HIRQ_ESEL, &cd_cmd, cd_cmd_rs);
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_calculate_actual_size()
{
   cd_cmd_struct cd_cmd_rs;
   int ret;

   if ((ret = cd_calc_actual_size_rs(&cd_cmd_rs, 0, 1, 1)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   if (!cd_wait_hirq(HIRQ_ESEL))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

int cd_get_actual_size_rs(cd_cmd_struct *cd_cmd_rs)
{
   cd_cmd_struct cd_cmd;

   cd_cmd.CR1 = 0x5300;
   cd_cmd.CR2 = cd_cmd.CR3 = cd_cmd.CR4 = 0x0000;

   return cd_exec_command(HIRQ_ESEL, &cd_cmd, cd_cmd_rs);
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_get_actual_size()
{
   cd_cmd_struct cd_cmd_rs;
   int ret;

   if ((ret = cd_get_actual_size_rs(&cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   if ((cd_cmd_rs.CR1 & 0xFF) != 0 ||
       cd_cmd_rs.CR2 != (2048/2) ||
       cd_cmd_rs.CR3 != 0 ||
       cd_cmd_rs.CR4 != 0)
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_get_sector_info()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x5400;
   cd_cmd.CR2 = 0x0001;
   cd_cmd.CR3 = 0x0100;
   cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(0, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   if ((cd_cmd_rs.CR1 & 0xFF) != 0x00 ||
        cd_cmd_rs.CR2 != 0x0097 ||
        cd_cmd_rs.CR3 != 0x0000 ||
        cd_cmd_rs.CR4 != 0x0000)
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_exec_fad_search()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x5500;
   cd_cmd.CR2 = 0x0000;
   cd_cmd.CR3 = 0x0100;
   cd_cmd.CR4 = 0x0097;

   if ((ret = cd_exec_command(HIRQ_ESEL, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   if (!cd_wait_hirq(HIRQ_ESEL))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_get_fad_search_results()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x5600;
   cd_cmd.CR2 = 0x0000;
   cd_cmd.CR3 = 0x0000;
   cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(0, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   if ((cd_cmd_rs.CR1 & 0xFF) != 0x00 ||
      cd_cmd_rs.CR2 != 0x0001 ||
      cd_cmd_rs.CR3 != 0x0100 ||
      cd_cmd_rs.CR4 != 0x0097)
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_set_sector_length()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x6000 | SECT_2352;
   cd_cmd.CR2 = (SECT_2352 << 8);
   cd_cmd.CR3 = cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(HIRQ_ESEL, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   if (!cd_wait_hirq(HIRQ_ESEL))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   // Should grab a sector to verify it's the right size, etc.
   if ((ret = cd_play_fad(0, 166, 1)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   // Wait until play finished
   if (!cd_wait_hirq(HIRQ_PEND))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   // Wait a few seconds, then check status
   if (!cd_wait_status_rs(STATUS_PAUSE, 60 * 5, 0, &cd_cmd_rs))
      return;

   // Get read sector size
   if ((ret = cd_calc_actual_size_rs(&cd_cmd_rs, 0, 0, 1)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   if ((ret = cd_get_actual_size_rs(&cd_cmd_rs)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   if ((cd_cmd_rs.CR1 & 0xFF) != 0 ||
      cd_cmd_rs.CR2 == 0x498)
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_get_sector_data()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;
   u8 buf[2352];
   int i;

   cd_cmd.CR1 = 0x6100;
   cd_cmd.CR2 = 0x0000;
   cd_cmd.CR3 = 0x0100;
   cd_cmd.CR4 = 0x0001;

   if ((ret = cd_exec_command(HIRQ_EHST | HIRQ_DRDY, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   tests_log_textf("%04X %04X %04X %04X %04X\n", CDB_REG_HIRQ, cd_cmd_rs.CR1, cd_cmd_rs.CR2, cd_cmd_rs.CR3, cd_cmd_rs.CR4);

   if (!cd_wait_hirq(HIRQ_DRDY))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   for (i = 0; i < 0x400; i++)
      buf[i] = CDB_REG_DATATRNS;

   if (!cd_end_transfer_rs(&cd_cmd_rs, HIRQ_EHST))
      return;

   if (!cd_wait_hirq(HIRQ_EHST))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   //for (i = 0; i < 2048; i++)
   //   tests_log_textf("%02X\n", buf[i]);
   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_del_sector_data()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x6200;
   cd_cmd.CR2 = 0x0000;
   cd_cmd.CR3 = 0x0100;
   cd_cmd.CR4 = 0x0001;

   if ((ret = cd_exec_command(HIRQ_EHST, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   tests_log_textf("%04X %04X %04X %04X %04X\n", CDB_REG_HIRQ, cd_cmd_rs.CR1, cd_cmd_rs.CR2, cd_cmd_rs.CR3, cd_cmd_rs.CR4);

   if (!cd_wait_hirq(HIRQ_EHST))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_get_then_del_sector_data()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret, i;
   u8 buf[2352];

   cd_cmd.CR1 = 0x6300;
   cd_cmd.CR2 = 0x0000;
   cd_cmd.CR3 = 0x0100;
   cd_cmd.CR4 = 0x0001;

   if ((ret = cd_exec_command(HIRQ_EHST | HIRQ_DRDY, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   tests_log_textf("%04X %04X %04X %04X %04X\n", CDB_REG_HIRQ, cd_cmd_rs.CR1, cd_cmd_rs.CR2, cd_cmd_rs.CR3, cd_cmd_rs.CR4);

   if (!cd_wait_hirq(HIRQ_DRDY))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   for (i = 0; i < 0x400; i++)
      buf[i] = CDB_REG_DATATRNS;

   if (!cd_end_transfer_rs(&cd_cmd_rs, HIRQ_EHST))
      return;

   if (!cd_wait_hirq(HIRQ_EHST))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_put_sector_data()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;
   int i;

   cd_cmd.CR1 = 0x6400;
   cd_cmd.CR2 = 0x0000;
   cd_cmd.CR3 = 0x0200;
   cd_cmd.CR4 = 0x0001;

   if ((ret = cd_exec_command(HIRQ_EHST | HIRQ_DRDY, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   if (!cd_wait_hirq(HIRQ_DRDY))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   //for (i = 0; i < (2352/4); i++)
   for (i = 0; i < (2048/4); i++)
      CDB_REG_DATATRNS = i;

   if ((ret = cd_end_transfer()) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }
 
   if (!cd_wait_hirq(HIRQ_EHST))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_copy_sector_data()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x6502;
   cd_cmd.CR2 = 0x0000;
   cd_cmd.CR3 = 0x0100;
   cd_cmd.CR4 = 0x0001;

   if ((ret = cd_exec_command(HIRQ_ECPY, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   tests_log_textf("%04X %04X %04X %04X %04X\n", CDB_REG_HIRQ, cd_cmd_rs.CR1, cd_cmd_rs.CR2, cd_cmd_rs.CR3, cd_cmd_rs.CR4);

   if (!cd_wait_hirq(HIRQ_ECPY))
   {
      stage_status = STAGESTAT_BADDATA;
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_move_sector_data()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x6600;
   cd_cmd.CR2 = 0x0000;
   cd_cmd.CR3 = 0x0100;
   cd_cmd.CR4 = 0x0001;

   if ((ret = cd_exec_command(HIRQ_ECPY, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   if (!cd_wait_hirq(HIRQ_ECPY))
   {   
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_get_copy_error()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x6700;
   cd_cmd.CR2 = cd_cmd.CR3 = cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(0, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   if ((cd_cmd_rs.CR1 & 0xFF) != 0)
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

int cd_change_dir_rs(int filter, u32 fid)
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x7000;
   cd_cmd.CR2 = 0x0000;
   cd_cmd.CR3 = (filter << 8) | ((fid & 0xFFFFFF) >> 16);
   cd_cmd.CR4 = fid & 0xFFFF;

   if ((ret = cd_exec_command(HIRQ_EFLS, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return FALSE;
   }

   if (!cd_wait_hirq(HIRQ_EFLS))
   {
      do_cdb_tests_unexp_cr_data_error();
      return FALSE;
   }

   stage_status = STAGESTAT_DONE;
   return TRUE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_change_directory()
{
   int ret;

   // Clear out buffer before doing anything else
   if ((ret = cd_reset_selector_all()) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   if (!cd_change_dir_rs(1, 0xFFFFFF))
      return;

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_read_directory()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x7100;
   cd_cmd.CR2 = 0x0000;
   cd_cmd.CR3 = 0x0100;
   cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(HIRQ_EFLS, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   tests_log_textf("%04X %04X %04X %04X %04X\n", CDB_REG_HIRQ, cd_cmd_rs.CR1, cd_cmd_rs.CR2, cd_cmd_rs.CR3, cd_cmd_rs.CR4);

   if (!cd_wait_hirq(HIRQ_EFLS))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_get_file_system_scope()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   if (!cd_change_dir_rs(1, 0xFFFFFF))
      return;

   cd_cmd.CR1 = 0x7200;
   cd_cmd.CR2 = 0x0000;
   cd_cmd.CR3 = 0x0000;
   cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(HIRQ_EFLS, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   tests_log_textf("%04X %04X %04X %04X %04X\n", CDB_REG_HIRQ, cd_cmd_rs.CR1, cd_cmd_rs.CR2, cd_cmd_rs.CR3, cd_cmd_rs.CR4);

#if 0
   if (!cd_wait_hirq(HIRQ_EFLS))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }
#endif

   if (cd_cmd_rs.CR2 != 0x0003 ||
      cd_cmd_rs.CR3 != 0x0100 ||
      cd_cmd_rs.CR4 != 0x0002)
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_get_file_info()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x7300;
   cd_cmd.CR2 = 0x0000;
   cd_cmd.CR3 = 0x0000;
   cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(HIRQ_DRDY, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   tests_log_textf("%04X %04X %04X %04X %04X\n", CDB_REG_HIRQ, cd_cmd_rs.CR1, cd_cmd_rs.CR2, cd_cmd_rs.CR3, cd_cmd_rs.CR4);

   if (!cd_wait_hirq(HIRQ_DRDY))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   // fixme get file info here

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_read_file()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x7400;
   cd_cmd.CR2 = 0x0000;
   cd_cmd.CR3 = 0x0100;
   cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(HIRQ_EFLS, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   tests_log_textf("%04X %04X %04X %04X %04X\n", CDB_REG_HIRQ, cd_cmd_rs.CR1, cd_cmd_rs.CR2, cd_cmd_rs.CR3, cd_cmd_rs.CR4);

   if (!cd_wait_hirq(HIRQ_EFLS))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_abort_file()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x7500;
   cd_cmd.CR2 = 0x0000;
   cd_cmd.CR3 = 0x0000;
   cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(HIRQ_EFLS, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      do_tests_error_noarg(ret);
      return;
   }

   tests_log_textf("%04X %04X %04X %04X %04X\n", CDB_REG_HIRQ, cd_cmd_rs.CR1, cd_cmd_rs.CR2, cd_cmd_rs.CR3, cd_cmd_rs.CR4);

   if (!cd_wait_hirq(HIRQ_EFLS))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void misc_cd_test()
{
}

//////////////////////////////////////////////////////////////////////////////
