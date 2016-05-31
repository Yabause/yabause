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

void mpeg_cmd_test()
{
   init_cdb_tests();

   if (!is_mpeg_card_present())
   {
      tests_log_textf("mpeg card not detected\n");
      return;
   }

   unregister_all_tests();

   register_test(&test_cmd_mpeg_init, "MPEG Init");
   register_test(&test_cmd_mpeg_get_status, "MPEG Get Status");
   register_test(&test_cmd_mpeg_get_int, "MPEG Get Interrupt");
   register_test(&test_cmd_mpeg_set_int_mask, "MPEG Set Interrupt Mask");
   register_test(&test_cmd_mpeg_set_mode, "MPEG Set Mode");
   //register_test(&test_cmd_mpeg_play, "MPEG Play");
   //register_test(&test_cmd_mpeg_set_decode_method, "MPEG Set Decoding Method");
   //register_test(&test_cmd_mpeg_out_decode_sync, "MPEG Out Decoding Sync");
   //register_test(&test_cmd_mpeg_get_timecode, "MPEG Get Timecode");
   //register_test(&test_cmd_mpeg_get_pts, "MPEG Get PTS");
   //register_test(&test_cmd_mpeg_set_con, "MPEG Set Connection");
   //register_test(&test_cmd_mpeg_get_con, "MPEG Get Connection");
   //register_test(&test_cmd_mpeg_chg_con, "MPEG Change Connection");
   //register_test(&test_cmd_mpeg_set_stream, "MPEG Set Stream");
   //register_test(&test_cmd_mpeg_get_stream, "MPEG Get Stream");
   //register_test(&test_cmd_mpeg_get_picture_size, "MPEG Get Picture Size");
   //register_test(&test_cmd_mpeg_display, "MPEG Display");
   //register_test(&test_cmd_mpeg_set_window, "MPEG Set Window");
   //register_test(&test_cmd_mpeg_set_border_color, "MPEG Set Border Color");
   //register_test(&test_cmd_mpeg_set_fade, "MPEG Set Fade");
   //register_test(&test_cmd_mpeg_set_video_effects, "MPEG Set Video Effects");
   //register_test(&test_cmd_mpeg_get_image, "MPEG Get Image");
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

   cd_cmd.CR1 = 0x9400;
   cd_cmd.CR2 = cd_cmd.CR3 = cd_cmd.CR4 = 0x0000;

   if ((ret = cd_exec_command(0, &cd_cmd, &cd_cmd_rs)) != IAPETUS_ERR_OK)
   {
      do_tests_error_noarg(ret);
      return;
   }

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
