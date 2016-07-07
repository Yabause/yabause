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
#include "cart.h"

//////////////////////////////////////////////////////////////////////////////

void cart_test()
{
   int choice;

   menu_item_struct cart_menu[] = {
   { "Action Replay" , &ar_test, },
   { "1 Mbit DRAM Cart", &dram_1mbit_test, },
   { "4 Mbit DRAM Cart", &dram_4mbit_test, },
   { "Backup RAM Cart" , &bup_test, },
   { "Netlink Cart" , &netlink_test, },
   { "ROM Cart" , &rom_test, },
   { "Misc" , &misc_cart_test, },
   { "\0", NULL }
   };

   for (;;)
   {
      choice = gui_do_menu(cart_menu, &test_disp_font, 0, 0, "Cart Tests", MTYPE_CENTER, -1);
      if (choice == -1)
         break;
   }   
}

//////////////////////////////////////////////////////////////////////////////

void ar_test ()
{
}

//////////////////////////////////////////////////////////////////////////////

int dram_init(u8 expect_id, u8 min_size)
{
   u8 id = *((volatile u8 *)0x24FFFFFF);

   if (id == 0xFF || (expect_id != 0xFF && id != expect_id))
      return IAPETUS_ERR_HWNOTFOUND;
   if (min_size == 1 && id != 0x5A && id != 0x5C)
      return IAPETUS_ERR_HWNOTFOUND;
   if (min_size == 4 && id != 0x5C)
      return IAPETUS_ERR_HWNOTFOUND;

   *((u16 *)0x257EFFFE) = 1;
   SCU_REG_ABUSSRCS0CS1 = 0x23301FF0;
   SCU_REG_ABUSREFRESH = 0x00000013;

   // Check wram write
   *((u32 *)0x22400000) = 0xDEADBEEF;

   if (*((u32 *)0x22400000) != 0xDEADBEEF)
      return IAPETUS_ERR_UNEXPECTDATA;

   return IAPETUS_ERR_OK;
}

//////////////////////////////////////////////////////////////////////////////

void test_dram_init(u8 expect_id)
{
   int ret;
   if ((ret = dram_init(expect_id, -1)) != IAPETUS_ERR_OK)
   {
      do_tests_error(ret, "dram init failed");
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_dram_range(u32 dram_bank_size, u32 min_time, u32 max_time)
{
   u32 addr_list[2] = { 0x22400000, 0x22600000 };
   volatile u32 *i;
   int j;
   u32 freq;
   u32 start_time, end_time, diff_time;

   timer_setup(TIMER_HBLANK, &freq);
   start_time = timer_counter();
   for (j = 0; j < 2; j++)
   {
      for (i = (u32 *)addr_list[j]; i < (u32 *)(addr_list[j]+dram_bank_size); i+=4)
      {
         u32 test_pattern=0xA55A5AA5;
         u32 data;
         *i = test_pattern;
         data = *i;
         if (data != test_pattern)
         {
            do_tests_unexp_data_error("%08X wrote %08X, read back %08X\n", (u32)i, test_pattern, data);
            return;
         }
      }
   }

   end_time = timer_counter();
   diff_time = end_time-start_time;
   if (diff_time < min_time || diff_time > max_time)
   {
      tests_disp_iapetus_error(IAPETUS_ERR_TIMEOUT, __FILE__, __LINE__, "dram access time range expected(> %d && < %d). Calculated %d\n", min_time, max_time, diff_time);
      stage_status = STAGESTAT_BADTIMING;
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void test_dram_1mbit_init()
{
   test_dram_init(0x5A);
}

//////////////////////////////////////////////////////////////////////////////

void test_dram_1mbit_range()
{
   test_dram_range(0x80000, 0xFFFFFFFE, 0);
}

//////////////////////////////////////////////////////////////////////////////

void dram_1mbit_test ()
{
   unregister_all_tests();

   register_test(&test_dram_1mbit_init, "Init check");
   register_test(&test_dram_1mbit_range, "1MB range check");
   do_tests("DRAM 1MB tests", 0, 0);
}

//////////////////////////////////////////////////////////////////////////////

void test_dram_4mbit_init()
{
   test_dram_init(0x5C);
}

//////////////////////////////////////////////////////////////////////////////

void test_dram_4mbit_range()
{
   test_dram_range(0x200000, 0xFFFFFFFE, 0);   
}

//////////////////////////////////////////////////////////////////////////////

void dram_4mbit_test ()
{
   unregister_all_tests();

   register_test(&test_dram_4mbit_init, "Init check");
   register_test(&test_dram_4mbit_range, "4MB range check");
   do_tests("DRAM 4MB tests", 0, 0);
}

//////////////////////////////////////////////////////////////////////////////

void bup_test ()
{
}

//////////////////////////////////////////////////////////////////////////////

void netlink_test ()
{
}

//////////////////////////////////////////////////////////////////////////////

void rom_test ()
{
}

//////////////////////////////////////////////////////////////////////////////

void misc_cart_test ()
{
}

//////////////////////////////////////////////////////////////////////////////
