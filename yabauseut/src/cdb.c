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
//   register_test(&TestCMDCDStatus, "CD Status");
   register_test(&test_cmd_get_hw_info, "Get Hardware Info");
//   register_test(&TestCMDGetTOC, "Get TOC");
   register_test(&test_cmd_get_session_info, "Get Session Info");
//   register_test(&TestCMDInitCD, "Initialize CD System");
//   register_test(&TestCMDOpenTray, "Open Tray");
//   register_test(&TestCMDEndDataTransfer, "End Data Transfer");
//   register_test(&TestCMDPlayDisc, "Play Disc");
//   register_test(&TestCMDSeekDisc, "Seek Disc");
//   register_test(&TestCMDScanDisc, "Scan Disc");
//   register_test(&TestCMDGetSubcodeQRW, "Get Subcode Q/RW");
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
//   register_test(&TestCMDResetSelector, "Reset Selector");
//   register_test(&TestCMDGetBufferSize, "Get Buffer Size");
//   register_test(&TestCMDGetSectorNumber, "Get Sector Number");
//   register_test(&TestCMDCalculateActualSize, "Calculate Actual Size");
//   register_test(&TestCMDGetActualSize, "Get Actual Size");
//   register_test(&TestCMDGetSectorInfo, "Get Sector Info");
   register_test(&test_cmd_set_sector_length, "Set Sector Length");
//   register_test(&TestCMDGetSectorData, "Get Sector Data");
//   register_test(&TestCMDDelSectorData, "Delete Sector Data");
//   register_test(&TestCMDGetThenDelSectorData, "Get Then Delete Sector Data");
//   register_test(&TestCMDPutSectorData, "Put Sector Data");
//   register_test(&TestCMDCopySectorData, "Copy Sector Data");
//   register_test(&TestCMDMoveSectorData, "Move Sector Data");
//   register_test(&TestCMDGetCopyError, "Get Copy Error");
//   register_test(&TestCMDChangeDirectory, "Change Directory");
//   register_test(&TestCMDReadDirectory, "Read Directory");
//   register_test(&TestCMDGetFileSystemScope, "Get File System Scope");
//   register_test(&TestCMDGetFileInfo, "Get File Info");
//   register_test(&TestCMDReadFile, "Read File");
//   register_test(&TestCMDAbortFile, "Abort File");
   do_tests("CD Commands tests", 0, 0);
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
         tests_disp_iapetus_error(ret, __FILE__, __LINE__);
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
         vdp_printf(&test_disp_font, 2 * 8, 22 * 8, 0xF, "mbx %d test failed", i);
         stage_status = STAGESTAT_BADTIMING;
         return;
      }
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_cd_status()
{
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
      tests_disp_iapetus_error(ret, __FILE__, __LINE__);
      stage_status = STAGESTAT_BADDATA;
      return;
   }

   // Verify that the data returned is correct here

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_get_toc()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x0200;
   cd_cmd.CR2 = cd_cmd.CR3 = cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(HIRQ_DRDY, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      tests_disp_iapetus_error(ret, __FILE__, __LINE__);
      stage_status = STAGESTAT_BADDATA;
      return;
   }

   // Verify that the data returned is correct here

   // Read out TOC and verify

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
      tests_disp_iapetus_error(ret, __FILE__, __LINE__);
      stage_status = STAGESTAT_BADDATA;
      return;
   }

   // Verify that the data returned is correct here

   // Test individual sessions here

   // Verify that the data returned is correct here

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_set_cddev_con()
{
   int ret;

   if ((ret = cd_connect_cd_to_filter(3)) != IAPETUS_ERR_OK)
   {   
      tests_disp_iapetus_error(ret, __FILE__, __LINE__);
      stage_status = STAGESTAT_BADDATA;
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
      tests_disp_iapetus_error(ret, __FILE__, __LINE__);
      stage_status = STAGESTAT_BADDATA;
      return;
   }

   if ((cd_cmd_rs.CR3 >> 8) != 0x03)
   {   
      tests_disp_iapetus_error(IAPETUS_ERR_UNEXPECTDATA, __FILE__, __LINE__);
      stage_status = STAGESTAT_BADDATA;
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
      tests_disp_iapetus_error(ret, __FILE__, __LINE__);
      stage_status = STAGESTAT_BADDATA;
      return;
   }

   // Check returned status to see if it's right here

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_set_filter_range()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x4000;
   cd_cmd.CR2 = cd_cmd.CR3 = cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(HIRQ_ESEL, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      tests_disp_iapetus_error(ret, __FILE__, __LINE__);
      stage_status = STAGESTAT_BADDATA;
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
   cd_cmd.CR2 = cd_cmd.CR3 = cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(HIRQ_ESEL, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      tests_disp_iapetus_error(ret, __FILE__, __LINE__);
      stage_status = STAGESTAT_BADDATA;
      return;
   }

   // Check returned status to see if it's right here

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_set_filter_sh_cond()
{
   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_get_filter_sh_cond()
{
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
      tests_disp_iapetus_error(ret, __FILE__, __LINE__);
      stage_status = STAGESTAT_BADDATA;
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

   if ((ret = cd_exec_command(HIRQ_ESEL, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      tests_disp_iapetus_error(ret, __FILE__, __LINE__);
      stage_status = STAGESTAT_BADDATA;
      return;
   }

   if ((cd_cmd_rs.CR1 & 0xFF) != 0x7F)
   {   
      tests_disp_iapetus_error(IAPETUS_ERR_UNEXPECTDATA, __FILE__, __LINE__);
      stage_status = STAGESTAT_BADDATA;
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_set_filter_con()
{
   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_get_filter_con()
{
   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_set_sector_length()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x6000;
   cd_cmd.CR2 = cd_cmd.CR3 = cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(HIRQ_ESEL, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {   
      tests_disp_iapetus_error(ret, __FILE__, __LINE__);
      stage_status = STAGESTAT_BADDATA;
      return;
   }

   // Should grab a sector to verify it's the right size, etc. here

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void misc_cd_test()
{
}

//////////////////////////////////////////////////////////////////////////////
