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

#ifndef CDBH
#define CDBH

#define do_cdb_tests_unexp_cr_data_error() \
   do_tests_unexp_data_error("%04X %04X %04X %04X %04X", CDB_REG_HIRQ, cd_cmd_rs.CR1, cd_cmd_rs.CR2, cd_cmd_rs.CR3, cd_cmd_rs.CR4);

enum IAPETUS_ERR init_cdb_tests();
void cdb_test();
void cd_cmd_test();
void cd_rw_test();
void misc_cd_test();

void test_cdb_mbx();
void test_cmd_cd_status();
void test_cmd_get_hw_info();
void test_cmd_get_toc();
void test_cmd_get_session_info();
void test_cmd_init_cd();
void test_cmd_open_tray();
void test_cmd_end_data_transfer();
void test_cmd_play_disc();
void test_cmd_seek_disc();
void test_cmd_scan_disc();
void test_cmd_get_subcode_qrw();
void test_cmd_set_cddev_con();
void test_cmd_get_cd_dev_con();
void test_cmd_get_last_buffer();
void test_cmd_set_filter_range();
void test_cmd_get_filter_range();
void test_cmd_set_filter_sh_cond();
void test_cmd_get_filter_sh_cond();
void test_cmd_set_filter_mode();
void test_cmd_get_filter_mode();
void test_cmd_set_filter_con();
void test_cmd_get_filter_con();
void test_cmd_reset_selector();
void test_cmd_get_buffer_size();
void test_cmd_get_sector_number();
void test_cmd_calculate_actual_size();
void test_cmd_get_actual_size();
void test_cmd_get_sector_info();
void test_cmd_exec_fad_search();
void test_cmd_get_fad_search_results();
void test_cmd_set_sector_length();
void test_cmd_get_sector_data();
void test_cmd_del_sector_data();
void test_cmd_get_then_del_sector_data();
void test_cmd_put_sector_data();
void test_cmd_copy_sector_data();
void test_cmd_move_sector_data();
void test_cmd_get_copy_error();
void test_cmd_change_directory();
void test_cmd_read_directory();
void test_cmd_get_file_system_scope();
void test_cmd_get_file_info();
void test_cmd_read_file();
void test_cmd_abort_file();

#endif
