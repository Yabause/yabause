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
#include "mpeg.h"

file_struct mpeg_file;

#define CDWORKBUF ((void *)0x202C0000)

void mpeg_test()
{
   int choice;

   menu_item_struct mpeg_menu[] = {
      { "MPEG Commands", &mpeg_cmd_test, },
      { "MPEG Play", &mpeg_play_test, },
      { "\0", NULL }
   };

   for (;;)
   {
      choice = gui_do_menu(mpeg_menu, &test_disp_font, 0, 0, "MPEG Card Tests", MTYPE_CENTER, -1);
      gui_clear_scr(&test_disp_font);
      if (choice == -1)
         break;
   }   
}

//////////////////////////////////////////////////////////////////////////////

int mpeg_load()
{
   cd_sh_cond_struct cd_sh_cond;
   cd_range_struct cd_range;
   cd_con_struct cd_con;
   int ret;

   if ((ret = cdfs_init(CDWORKBUF, 4096)) != IAPETUS_ERR_OK)
      return ret;

   if ((ret = cdfs_open("M2TEST\\MOVIE.MPG", &mpeg_file)) != IAPETUS_ERR_OK)
      return ret;

   // Setup CD filters
   cd_range.fad = mpeg_file.lba+150;
   cd_range.range = mpeg_file.size / 2048;

   cd_sh_cond.channel = 0;
   cd_sh_cond.file_id = 0;
   cd_sh_cond.ci_val = 0;
   cd_sh_cond.ci_mask = 0;
   cd_sh_cond.sm_val = SM_VIDEO;
   cd_sh_cond.sm_mask = SM_VIDEO;

   cd_con.connect_flags = CD_CON_TRUE | CD_CON_FALSE;
   cd_con.true_con = 0;
   cd_con.false_con = 1;

   if ((ret = cd_set_filter(0, FM_FAD | FM_FN | FM_SM, &cd_sh_cond, &cd_range, &cd_con)) != IAPETUS_ERR_OK)
      return ret;

   cd_sh_cond.channel = 0;
   cd_sh_cond.file_id = 0;
   cd_sh_cond.ci_val = 0;
   cd_sh_cond.ci_mask = 0;
   cd_sh_cond.sm_val = SM_AUDIO;
   cd_sh_cond.sm_mask = SM_AUDIO;

   cd_con.connect_flags = CD_CON_TRUE | CD_CON_FALSE;
   cd_con.true_con = 1;
   cd_con.false_con = 0xFF;

   if ((ret = cd_set_filter(1, FM_FAD | FM_FN | FM_SM, &cd_sh_cond, &cd_range, &cd_con)) != IAPETUS_ERR_OK)
      return ret;

   return IAPETUS_ERR_OK;
}

//////////////////////////////////////////////////////////////////////////////

int mpeg_get_lsi(u8 cpu, u8 reg, u16 *val)
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0xAE00 | cpu;
   cd_cmd.CR2 = reg;
   cd_cmd.CR3 = cd_cmd.CR4 = 0x0000;

   if ((ret=cd_exec_command(0, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
      return ret;

   *val = cd_cmd_rs.CR4;
   return IAPETUS_ERR_OK;
}

//////////////////////////////////////////////////////////////////////////////

void lsi_dump()
{
   int i;
   int ret;
   u16 val;

   for (i = 0; i < 0x100; i+=2)
   {
      if ((ret = mpeg_get_lsi(0, i, &val)) != IAPETUS_ERR_OK)
         tests_disp_iapetus_error(ret, __FILE__, __LINE__, "");
      else
         tests_log_textf("%08X: %04X\n", 0xA100000+i, val);
   }

   if ((ret = mpeg_get_lsi(2, i, &val)) != IAPETUS_ERR_OK)
      tests_disp_iapetus_error(ret, __FILE__, __LINE__, "");
   else
      tests_log_textf("%08X: %04X\n", 0xA180000, val);
}

//////////////////////////////////////////////////////////////////////////////

void mpeg_cmd_test()
{
   int ret;
   screen_settings_struct settings;

   init_cdb_tests();

   if (!is_mpeg_card_present())
   {
      tests_log_textf("mpeg card not detected\n");
      return;
   }

   if ((ret=mpeg_load()) != IAPETUS_ERR_OK)
   {
      tests_disp_iapetus_error(ret, __FILE__, __LINE__, "mpeg load failed");
      return;
   }

   // Enable the external audio through SCSP
   sound_external_audio_enable(7, 7);

   // Setup NBG1 as EXBG
   settings.is_bitmap = TRUE;
   settings.bitmap_size = BG_BITMAP512x256;
   settings.transparent_bit = 0;
   settings.color = BG_32786COLOR;
   settings.special_priority = 0;
   settings.special_color_calc = 0;
   settings.extra_palette_num = 0;
   settings.map_offset = 0;
   if (vdp_exbg_init(&settings) != IAPETUS_ERR_OK)
   {
      tests_disp_iapetus_error(ret, __FILE__, __LINE__, "exbg init failed");
      return;
   }

   unregister_all_tests();

   register_test(&test_cmd_mpeg_init, "MPEG Init");
   register_test(&test_cmd_mpeg_get_status, "MPEG Get Status");
   register_test(&test_cmd_mpeg_get_int, "MPEG Get Interrupt");
   register_test(&test_cmd_mpeg_set_int_mask, "MPEG Set Interrupt Mask");
   register_test(&test_cmd_mpeg_set_mode, "MPEG Set Mode");
   register_test(&test_cmd_mpeg_set_decode_method, "MPEG Set Decoding Method");
   //register_test(&test_cmd_mpeg_out_decode_sync, "MPEG Out Decoding Sync");
   //register_test(&test_cmd_mpeg_get_pts, "MPEG Get PTS");
   register_test(&test_cmd_mpeg_set_stream, "MPEG Set Stream");
   register_test(&test_cmd_mpeg_get_stream, "MPEG Get Stream");
   register_test(&test_cmd_mpeg_set_con, "MPEG Set Connection");
   register_test(&test_cmd_mpeg_get_con, "MPEG Get Connection");
   //register_test(&test_cmd_mpeg_chg_con, "MPEG Change Connection");
   //register_test(&test_cmd_mpeg_get_picture_size, "MPEG Get Picture Size");
   register_test(&test_cmd_mpeg_display, "MPEG Display");
   register_test(&test_cmd_mpeg_set_window, "MPEG Set Window");
   register_test(&test_cmd_mpeg_set_border_color, "MPEG Set Border Color");
   register_test(&test_cmd_mpeg_set_fade, "MPEG Set Fade");
   register_test(&test_cmd_mpeg_set_video_effects, "MPEG Set Video Effects");
   register_test(&test_cmd_mpeg_play, "MPEG Play");
   register_test(&test_cmd_mpeg_get_timecode, "MPEG Get Timecode");
   //register_test(&test_cmd_mpeg_get_image, "MPEG Get Image");
   //register_test(&test_cmd_mpeg_set_image, "MPEG Set Image");
   //register_test(&test_cmd_mpeg_read_image, "MPEG Read Image");
   //register_test(&test_cmd_mpeg_write_image, "MPEG Write Image");
   //register_test(&test_cmd_mpeg_read_sector, "MPEG Read Sector");
   //register_test(&test_cmd_mpeg_write_sector, "MPEG Write Sector");
   //register_test(&test_cmd_mpeg_get_lsi, "MPEG Get LSI");
   //register_test(&test_cmd_mpeg_set_lsi, "MPEG Set LSI");
   do_tests("MPEG Command tests", 0, 0);

   vdp_exbg_deinit();
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_mpeg_get_status()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x9000;
   cd_cmd.CR2 = cd_cmd.CR3 = cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(0, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   // Verify that the data returned is correct
   if ((cd_cmd_rs.CR1 & 0xFF) != 0x19 ||
      cd_cmd_rs.CR3 != 0x00C0 ||
      cd_cmd_rs.CR4 != 0x0001)
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_mpeg_get_int()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x9100;
   cd_cmd.CR2 = cd_cmd.CR3 = cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(0, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   // Verify that the data returned is correct
   if ((cd_cmd_rs.CR1 & 0xFF) != 0x00 ||
      cd_cmd_rs.CR2 != 0x0000 ||
      cd_cmd_rs.CR3 != 0x0000 ||
      cd_cmd_rs.CR4 != 0x0000)
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_mpeg_set_int_mask()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x92FF;
   cd_cmd.CR2 = 0xFFFF;
   cd_cmd.CR3 = cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(0, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_mpeg_set_mode()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x94FF;
   cd_cmd.CR2 = 0x0000;
   cd_cmd.CR3 = 0x0100;
   cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(0, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_mpeg_play()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   // Start CD transfer
   if ((ret = cd_play_fad(0, mpeg_file.lba+150, mpeg_file.size / 2048)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   // Clear hirq flags
   CDB_REG_HIRQ = ~(HIRQ_MPED);

   // Start MPEG decoding
   cd_cmd.CR1 = 0x9500 | MPPM_SYNC;
   cd_cmd.CR2 = (MPTM_NO_CHANGE << 8) | MPTM_AUTO;
   cd_cmd.CR3 = 0x0000;
   cd_cmd.CR4 = MPCS_NO_CHANGE & 0xFF;

   if ((ret = cd_exec_command(0, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_mpeg_set_decode_method()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x9604; // Unmute both channels
   cd_cmd.CR2 = 0x0001; // no pause
   cd_cmd.CR3 = 0x0000;
   cd_cmd.CR4 = 0x0001; // no freeze

   if ((ret = cd_exec_command(0, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_mpeg_out_decode_sync()
{
   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_mpeg_get_timecode()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;
   int old_transparent;

   cd_cmd.CR1 = 0x9800;
   cd_cmd.CR2 = cd_cmd.CR3 = cd_cmd.CR4 = 0x0000;

   while (!(CDB_REG_HIRQ & HIRQ_MPED))
   {
      mpeg_status_struct mpeg_status;
      if ((ret = mpeg_get_status(&mpeg_status)) != IAPETUS_ERR_OK )
      {
         do_tests_error_noarg(ret);
         return;
      }

      if (mpeg_status.play_status == 0x19)
         break;

      if ((ret = cd_exec_command(0, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
      {
         do_tests_error_noarg(ret);
         return;
      }

      old_transparent = test_disp_font.transparent;
      test_disp_font.transparent = 0;
      vdp_printf(&test_disp_font, 0 * 8, 27 * 8, 0xF, "%02d:%02d:%02d:%02d", cd_cmd_rs.CR3 >> 8, cd_cmd_rs.CR3 & 0xFF, cd_cmd_rs.CR4 >> 8, cd_cmd_rs.CR4 & 0xFF);
      test_disp_font.transparent = old_transparent;
      vdp_vsync();
   }

   if (cd_cmd_rs.CR3 != 0 ||
       cd_cmd_rs.CR4 != 0x0E03)
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_mpeg_get_pts()
{
   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_mpeg_set_con()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x9A00 | (SCCM_SECTOR_DEL | SCCM_SYSTEM_END);
   cd_cmd.CR2 = (SCLS_SYSTEM << 8) | 1;
   cd_cmd.CR3 = (STM_SEL_CURRENT << 8) | (SCCM_CLEAR_VBV_WBC | SCCM_SECTOR_DEL | SCCM_SYSTEM_END);
   cd_cmd.CR4 = ((SCLS_SYSTEM | SCLS_AVSEARCH) << 8) | 0;

   if ((ret = cd_exec_command(0, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_mpeg_get_con()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x9B00;
   cd_cmd.CR3 = 0x0000;
   cd_cmd.CR2 = cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(0, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   // Verify that the data returned is correct
   if ((cd_cmd_rs.CR1 & 0xFF) != (SCCM_SECTOR_DEL | SCCM_SYSTEM_END) ||
      cd_cmd_rs.CR2 != ((SCLS_SYSTEM << 8) | 1) ||
      cd_cmd_rs.CR3 != (SCCM_CLEAR_VBV_WBC | SCCM_SECTOR_DEL | SCCM_SYSTEM_END) ||
      cd_cmd_rs.CR4 != (((SCLS_SYSTEM | SCLS_AVSEARCH) << 8) | 0))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_mpeg_chg_con()
{
   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_mpeg_set_stream()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x9D00;
   cd_cmd.CR2 = 0x0000;
   cd_cmd.CR3 = 0x0000;
   cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(0, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_mpeg_get_stream()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0x9E00;
   cd_cmd.CR2 = 0x0000;
   cd_cmd.CR3 = 0x0000;
   cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(0, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_mpeg_get_picture_size()
{
   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_mpeg_display()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0xA000;
   cd_cmd.CR2 = 0x0100; // Enable, frame buffer number
   cd_cmd.CR3 = cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(0, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_mpeg_set_window()
{
   cd_cmd_struct cd_cmd[5] = {
      { 0xA100, 1, 22, 0 },    // Frame Buffer Window position
      { 0xA101, 1, 15, 1 },    // Frame Buffer Window zoom rate
      { 0xA102, 1, 0, 8 },     // Display Window position
      { 0xA103, 1, 320, 224 }, // Display Window size
      //{ 0xA104, 1, 0, 0 },     // Display Window offset
   };
   cd_cmd_struct cd_cmd_rs;
   int i, ret;

   for (i = 0; i < 5; i++)
   {
      if ((ret = cd_exec_command(0, &cd_cmd[i], &cd_cmd_rs)) != IAPETUS_ERR_OK)
      {
         do_tests_error_noarg(ret);
         return;
      }
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_mpeg_set_border_color()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0xA200;
   cd_cmd.CR2 = 0x0000; // Black
   cd_cmd.CR3 = cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(0, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_mpeg_set_fade()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0xA300;
   cd_cmd.CR2 = 0x0000; // Y/C Gain
   cd_cmd.CR3 = cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(0, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_mpeg_set_video_effects()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   cd_cmd.CR1 = 0xA400;
   cd_cmd.CR2 = 0x0F00; // Interpolation, Lumi-Key
   cd_cmd.CR3 = 0x0000; // Mosaic w/h
   cd_cmd.CR4 = 0x0000; // Blur w/h

   if ((ret = cd_exec_command(0, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_mpeg_get_image()
{
   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_mpeg_set_image()
{
   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_mpeg_read_image()
{
   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_mpeg_write_image()
{
   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_mpeg_read_sector()
{
   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_mpeg_write_sector()
{
   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_mpeg_get_lsi()
{
   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_mpeg_set_lsi()
{
   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_cmd_mpeg_init()
{
   cd_cmd_struct cd_cmd;
   cd_cmd_struct cd_cmd_rs;
   int ret;

   // Software timer off
   cd_cmd.CR1 = 0x9300;
   cd_cmd.CR2 = cd_cmd.CR3 = cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(HIRQ_MPED | HIRQ_MPCM, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   if (!cd_wait_hirq(HIRQ_MPED))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   tests_log_textf("%04X %04X %04X %04X %04X\n", CDB_REG_HIRQ, cd_cmd_rs.CR1, cd_cmd_rs.CR2, cd_cmd_rs.CR3, cd_cmd_rs.CR4);

   // Verify that the data returned is correct
   if (cd_cmd_rs.CR1 != (STATUS_PAUSE << 8) ||
      cd_cmd_rs.CR2 != 0 ||
      cd_cmd_rs.CR3 != 0x0 ||
      cd_cmd_rs.CR4 != 0x0)
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   // Software timer on
   cd_cmd.CR2 = 0x0001;

   if ((ret = cd_exec_command(HIRQ_MPED | HIRQ_MPCM, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   if (!cd_wait_hirq(HIRQ_MPED | HIRQ_MPCM))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   lsi_dump();

   if (!cd_wait_hirq(HIRQ_MPCM))
   {
      do_cdb_tests_unexp_cr_data_error();
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void mpeg_play_test()
{
   init_cdb_tests();

   unregister_all_tests();

   register_test(&test_mpegplay_init, "MPEG Init");
   register_test(&test_mpegplay_play, "MPEG Play");
   register_test(&test_mpegplay_pause, "MPEG Pause");
   register_test(&test_mpegplay_unpause, "MPEG Unpause");
   register_test(&test_mpegplay_stop, "MPEG Stop");
   do_tests("MPEG Play tests", 0, 0);

   vdp_exbg_deinit();
}

//////////////////////////////////////////////////////////////////////////////

void test_mpegplay_init()
{
   int ret;

   if ((ret = mpeg_init()) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
   }
   else
      stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

BOOL test_mpeg_status(test_mpeg_status_struct *settings)
{
   u32 freq;
   mpeg_status_struct mpeg_status;
   u16 old_v_counter;
   int ret;

   timer_setup(TIMER_HBLANK, &freq);
   timer_delay(freq, settings->delay);

   if ((ret = mpeg_get_status(&mpeg_status)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return FALSE;
   }

   if (mpeg_status.play_status != (MS_PS_VIDEO_PLAYING | MS_PS_AUDIO_PLAYING) &&
      mpeg_status.mpeg_audio_status != (MS_AS_DECODE_OP | MS_AS_LEFT_OUTPUT | MS_AS_RIGHT_OUTPUT) &&
      (mpeg_status.mpeg_video_status & 0xF) != (MS_VS_DECODE_OP | MS_VS_DISPLAYING))
   {
      do_tests_unexp_data_error("%X %X %X", mpeg_status.play_status, mpeg_status.mpeg_audio_status, mpeg_status.mpeg_video_status);
      return FALSE;
   }

   // Verify that the v_counter is incrementing
   old_v_counter = mpeg_status.v_counter;
   vdp_vsync();

   if ((ret = mpeg_get_status(&mpeg_status)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return FALSE;
   }

   if (old_v_counter+1 != mpeg_status.v_counter)
   {
      do_tests_unexp_data_error("%X %X", old_v_counter+1, mpeg_status.v_counter);
      return FALSE;
   }

   return TRUE;
}

void test_mpegplay_play()
{
   int ret;
   test_mpeg_status_struct tms_settings;

   if ((ret = cdfs_init(CDWORKBUF, 4096)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   if ((ret = cdfs_open("M2TEST\\MOVIE.MPG", &mpeg_file)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   if ((ret = mpeg_play(&mpeg_file)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   tms_settings.delay = 2000;
   tms_settings.play_status = (MS_PS_VIDEO_PLAYING | MS_PS_AUDIO_PLAYING);
   tms_settings.play_status = 0xFF;
   tms_settings.mpeg_audio_status = (MS_AS_DECODE_OP | MS_AS_LEFT_OUTPUT | MS_AS_RIGHT_OUTPUT);
   tms_settings.mpeg_audio_status = 0xFF;
   tms_settings.mpeg_video_status = (MS_VS_DECODE_OP | MS_VS_DISPLAYING);
   tms_settings.mpeg_video_status = 0x000F;
   tms_settings.v_counter_inc = TRUE;

   if (!test_mpeg_status(&tms_settings))
   {
      mpeg_stop(&mpeg_file);
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_mpegplay_pause()
{
   int ret;
   test_mpeg_status_struct tms_settings;

   if ((ret = mpeg_pause(&mpeg_file)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   tms_settings.delay = 2000;
   tms_settings.play_status = (MS_PS_VIDEO_PLAYING | MS_PS_AUDIO_PLAYING);
   tms_settings.play_status = 0xFF;
   tms_settings.mpeg_audio_status = (MS_AS_DECODE_OP | MS_AS_LEFT_OUTPUT | MS_AS_RIGHT_OUTPUT);
   tms_settings.mpeg_audio_status = 0xFF;
   tms_settings.mpeg_video_status = (MS_VS_DECODE_OP | MS_VS_DISPLAYING | MS_VS_PAUSED);
   tms_settings.mpeg_video_status = 0x000F;
   tms_settings.v_counter_inc = TRUE;

   if (!test_mpeg_status(&tms_settings))
   {
      mpeg_stop(&mpeg_file);
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_mpegplay_unpause()
{
   int ret;
   test_mpeg_status_struct tms_settings;

   if ((ret = mpeg_unpause(&mpeg_file)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   tms_settings.delay = 2000;
   tms_settings.play_status = (MS_PS_VIDEO_PLAYING | MS_PS_AUDIO_PLAYING);
   tms_settings.play_status = 0xFF;
   tms_settings.mpeg_audio_status = (MS_AS_DECODE_OP | MS_AS_LEFT_OUTPUT | MS_AS_RIGHT_OUTPUT);
   tms_settings.mpeg_audio_status = 0xFF;
   tms_settings.mpeg_video_status = (MS_VS_DECODE_OP | MS_VS_DISPLAYING);
   tms_settings.mpeg_video_status = 0x000F;
   tms_settings.v_counter_inc = TRUE;

   if (!test_mpeg_status(&tms_settings))
   {
      mpeg_stop(&mpeg_file);
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_mpegplay_stop()
{
   int ret;
   test_mpeg_status_struct tms_settings;

   if ((ret = mpeg_stop(&mpeg_file)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

   tms_settings.delay = 2000;
   tms_settings.play_status = (MS_PS_VIDEO_RECEIVING | MS_PS_AUDIO_RECEIVING);
   tms_settings.play_status = 0xFF;
   tms_settings.mpeg_audio_status = (MS_AS_DECODE_OP | MS_AS_BUFFER_EMPTY | MS_AS_LEFT_OUTPUT | MS_AS_RIGHT_OUTPUT);
   tms_settings.mpeg_audio_status = 0xFF;
   tms_settings.mpeg_video_status = (MS_VS_DECODE_OP | MS_VS_DISPLAYING | MS_VS_PAUSED);
   tms_settings.mpeg_video_status = 0x000F;

   if (!test_mpeg_status(&tms_settings))
   {
      mpeg_stop(&mpeg_file);
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////
