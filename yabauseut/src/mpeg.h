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

#ifndef MPEGH
#define MPEGH

typedef struct 
{
   int delay;
   u8 play_status;
   u8 play_status_mask;
   u8 mpeg_audio_status;
   u8 mpeg_audio_status_mask;
   u8 mpeg_video_status;
   u8 mpeg_video_status_mask;
   BOOL v_counter_inc;
} test_mpeg_status_struct;

void mpeg_test();

void mpeg_cmd_test();
void mpeg_play_test();

void test_cmd_mpeg_get_status();
void test_cmd_mpeg_get_int();
void test_cmd_mpeg_set_int_mask();
void test_cmd_mpeg_init();
void test_cmd_mpeg_set_mode();
void test_cmd_mpeg_play();
void test_cmd_mpeg_set_decode_method();
void test_cmd_mpeg_out_decode_sync();
void test_cmd_mpeg_get_timecode();
void test_cmd_mpeg_get_pts();
void test_cmd_mpeg_set_con();
void test_cmd_mpeg_get_con();
void test_cmd_mpeg_chg_con();
void test_cmd_mpeg_set_stream();
void test_cmd_mpeg_get_stream();
void test_cmd_mpeg_get_picture_size();
void test_cmd_mpeg_display();
void test_cmd_mpeg_set_window();
void test_cmd_mpeg_set_border_color();
void test_cmd_mpeg_set_fade();
void test_cmd_mpeg_set_video_effects();
void test_cmd_mpeg_get_image();
void test_cmd_mpeg_set_image();
void test_cmd_mpeg_read_image();
void test_cmd_mpeg_write_image();
void test_cmd_mpeg_read_sector();
void test_cmd_mpeg_write_sector();
void test_cmd_mpeg_get_lsi();
void test_cmd_mpeg_set_lsi();

void test_mpegplay_init();
void test_mpegplay_play();
void test_mpegplay_pause();
void test_mpegplay_unpause();
void test_mpegplay_stop();

#endif