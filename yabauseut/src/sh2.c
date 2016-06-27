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

#define SH2REG_CCR      (*(volatile u8  *)0xFFFFFE92)
#define SH2REG_IPRA     (*(volatile u16 *)0xFFFFFEE2)
#define SH2REG_DVSR     (*(volatile u32 *)0xFFFFFF00)
#define SH2REG_DVDNT    (*(volatile u32 *)0xFFFFFF04)
#define SH2REG_DVCR     (*(volatile u32 *)0xFFFFFF08)
#define SH2REG_VCRDIV   (*(volatile u32 *)0xFFFFFF0C)
#define SH2REG_DVDNTH   (*(volatile u32 *)0xFFFFFF10)
#define SH2REG_DVDNTL   (*(volatile u32 *)0xFFFFFF14)
#define SH2REG_DVDNTUH  (*(volatile u32 *)0xFFFFFF18)
#define SH2REG_DVDNTUL  (*(volatile u32 *)0xFFFFFF1C)
#define SH2REG_DVSR2    (*(volatile u32 *)0xFFFFFF20)
#define SH2REG_DVDNT2   (*(volatile u32 *)0xFFFFFF24)
#define SH2REG_DVCR2    (*(volatile u32 *)0xFFFFFF28)
#define SH2REG_VCRDIV2  (*(volatile u32 *)0xFFFFFF2C)
#define SH2REG_DVDNTH2  (*(volatile u32 *)0xFFFFFF30)
#define SH2REG_DVDNTL2  (*(volatile u32 *)0xFFFFFF34)
#define SH2REG_DVDNTUH2 (*(volatile u32 *)0xFFFFFF38)
#define SH2REG_DVDNTUL2 (*(volatile u32 *)0xFFFFFF3C)

#define SH2REG_SAR0     (*(volatile u32 *)0xFFFFFF80)
#define SH2REG_DAR0     (*(volatile u32 *)0xFFFFFF84)
#define SH2REG_TCR0     (*(volatile u32 *)0xFFFFFF88)
#define SH2REG_CHCR0    (*(volatile u32 *)0xFFFFFF8C)
#define SH2REG_VCRMDA0  (*(volatile u32 *)0xFFFFFFA0)
#define SH2REG_DRCR0    (*(volatile u8  *)0xFFFFFE71)

#define SH2REG_SAR1     (*(volatile u32 *)0xFFFFFF90)
#define SH2REG_DAR1     (*(volatile u32 *)0xFFFFFF94)
#define SH2REG_TCR1     (*(volatile u32 *)0xFFFFFF98)
#define SH2REG_CHCR1    (*(volatile u32 *)0xFFFFFF9C)
#define SH2REG_VCRMDA1  (*(volatile u32 *)0xFFFFFFA8)
#define SH2REG_DRCR1    (*(volatile u8  *)0xFFFFFE72)

#define SH2REG_DMAOR    (*(volatile u32 *)0xFFFFFFB0)

#define RSTCSR_W (*(volatile u16 *)0XFFFFFE82)

void div_mirror_test(void);
void div_operation_test(void);
void div_interrupt_test(void);

//////////////////////////////////////////////////////////////////////////////

void sh2_test()
{
   interrupt_set_level_mask(0xF);

   init_iapetus(RES_320x224);

   // Setup a screen for us draw on
   vdp_rbg0_init(&test_disp_settings);
   vdp_set_default_palette();

   // Display On
   vdp_disp_on();

   unregister_all_tests();
   register_test(&div_mirror_test, "DIV register access");
   register_test(&div_operation_test, "DIV operations");
   register_test(&div_interrupt_test, "DIV overflow interrupt");
   do_tests("SH2 tests", 0, 0);

   // Other tests to do: instruction tests, check all register accesses,
   // onchip functions
}

//////////////////////////////////////////////////////////////////////////////

#define test_access_b(r) \
   r = 0x01; \
   if (r != 0x01) \
   { \
      stage_status = STAGESTAT_BADDATA; \
      return; \
   }

#define test_access_w(r) \
   r = 0x0102; \
   if (r != 0x0102) \
   { \
      stage_status = STAGESTAT_BADDATA; \
      return; \
   }

#define test_access_l(r) \
   r = 0x01020304; \
   if (r != 0x01020304) \
   { \
      stage_status = STAGESTAT_BADDATA; \
      return; \
   }

void div_mirror_test(void)
{
   // This tests DIV register reads/writes and checks mirroring
   test_access_w(SH2REG_VCRDIV)
   test_access_w(SH2REG_VCRDIV2)
   SH2REG_VCRDIV = 0;

   test_access_b(SH2REG_DVCR)
   test_access_b(SH2REG_DVCR2)
   SH2REG_DVCR = 0;

   test_access_l(SH2REG_DVDNTH)
   test_access_l(SH2REG_DVDNTH2)
   test_access_l(SH2REG_DVSR)
   test_access_l(SH2REG_DVSR2)
   test_access_l(SH2REG_DVDNTUH)
   test_access_l(SH2REG_DVDNTUH2)
   test_access_l(SH2REG_DVDNTUL)
   test_access_l(SH2REG_DVDNTUL2)

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void div_operation_test(void)
{
   // This tests to make sure the dividing operation is correct
   int i;

   // Test 64-bit/32-bit operation
   SH2REG_DVSR = 0x10;
   SH2REG_DVDNTH = 0x1;
   SH2REG_DVDNTL = 0x9;

   // Wait a bit
   for (i = 0; i < 20; i++) {}

   if (SH2REG_DVDNTL != 0x10000000 ||
       SH2REG_DVDNTL2 != 0x10000000 ||
       SH2REG_DVDNT != 0x10000000 ||
       SH2REG_DVDNT2 != 0x10000000 ||
       SH2REG_DVDNTUL != 0x10000000 ||
       SH2REG_DVDNTUL2 != 0x10000000 ||
       SH2REG_DVDNTH != 9 ||
       SH2REG_DVDNTH2 != 9 ||
       SH2REG_DVDNTUH != 9 ||
       SH2REG_DVDNTUH2 != 9)
   {
      stage_status = STAGESTAT_BADDATA;
      return;
   }

   // Ok, mirrors are working alright, and 64-bit/32-bit operation is correct

   // Test 32-bit/32-bit operation
   SH2REG_DVSR = 0x10;
   SH2REG_DVDNT = 0x10000009;

   // Wait a bit
   for (i = 0; i < 20; i++) {}

   if (SH2REG_DVDNTL != 0x1000000 ||
       SH2REG_DVDNTL2 != 0x1000000 ||
       SH2REG_DVDNT != 0x1000000 ||
       SH2REG_DVDNT2 != 0x1000000 ||
       SH2REG_DVDNTUL != 0x1000000 ||
       SH2REG_DVDNTUL2 != 0x1000000 ||
       SH2REG_DVDNTH != 9 ||
       SH2REG_DVDNTH2 != 9 ||
       SH2REG_DVDNTUH != 9 ||
       SH2REG_DVDNTUH2 != 9)
   {
      stage_status = STAGESTAT_BADDATA;
      return;
   }

   // Ok, mirrors are working alright, and 32-bit/32-bit operation is correct

   // Now let's do an overflow test(It seems you can only trigger it using 64-bit/32-bit operation)
   SH2REG_DVSR = 0x1;
   SH2REG_DVCR = 0;
   SH2REG_DVDNTH = 0x1;
   SH2REG_DVDNTL = 0x0;

   // Wait a bit
   for (i = 0; i < 20; i++) {}

   if (SH2REG_DVDNTL != 0x7FFFFFFF ||
       SH2REG_DVDNTH != 0xFFFFFFFE ||
       SH2REG_DVCR != 0x1)
   {
      stage_status = STAGESTAT_BADDATA;
      return;
   }

   // Lastly, do two divide by zero tests
   SH2REG_DVSR = 0;
   SH2REG_DVCR = 0;
   SH2REG_DVDNT = 0;

   // Wait a bit
   for (i = 0; i < 20; i++) {}

   if (SH2REG_DVDNT != 0x7FFFFFFF ||
       SH2REG_DVDNTH != 0 ||
       SH2REG_DVCR != 0x1)
   {
      stage_status = STAGESTAT_BADDATA;
      return;
   }

   SH2REG_DVSR = 0;
   SH2REG_DVCR = 0;
   SH2REG_DVDNT = 0xD0000000;

   // Wait a bit
   for (i = 0; i < 20; i++) {}

   if (SH2REG_DVDNT != 0x80000000 ||
       SH2REG_DVDNTH != 0xFFFFFFFE ||
       SH2REG_DVCR != 0x1)
   {
      stage_status = STAGESTAT_BADDATA;
      return;
   }

   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void sh2_int_test_func(void) __attribute__ ((interrupt_handler));

void sh2_int_test_func(void) 
{
   bios_set_sh2_interrupt(0x6E, 0);
   SH2REG_DVCR = 0;
   stage_status = STAGESTAT_DONE;
}

//////////////////////////////////////////////////////////////////////////////

void div_interrupt_test(void)
{
   // This tests to make sure an interrupt is generated when the registers are setup
   // for it, and an overflow occurs

   stage_status = STAGESTAT_WAITINGFORINT;
   bios_set_sh2_interrupt(0x6E, sh2_int_test_func);
   SH2REG_VCRDIV = 0x6E;
   SH2REG_IPRA = 0xF << 12;
   interrupt_set_level_mask(0xE);

   SH2REG_DVSR = 0;
   SH2REG_DVCR = 0x2;
   SH2REG_DVDNT = 0xD0000000;

   // Alright, test is all setup, now when the interrupt is done, the test
   // will successfully complete
}

//////////////////////////////////////////////////////////////////////////////

void clear_framebuffer()
{
   u32* dest = (u32 *)VDP2_RAM;

   int q;

   for (q = 0; q < 0x10000; q++)
   {
      dest[q] = 0;
   }
}

//////////////////////////////////////////////////////////////////////////////

//assuming the 28th bit is 0
//0b0000 0x0 cache area
//0b0010 0x2 cache through
//0b0100 0x4 purge
//0b0110 0x6 address array
//0b1100 0xC data array
//0x1110 0xE I/O area

volatile u32 *wram_cache = (volatile u32 *)(0x06000000);
volatile u32 *wram_cache_through = (volatile u32 *)(0x26000000);
volatile u32 *address_array = (volatile u32 *)(0x60000000);
volatile u32 *data_array = (volatile u32 *)(0xC0000000);

//////////////////////////////////////////////////////////////////////////////

void print_addr_data(int x_start, int y_start, u32 addr_val)
{
   int x_pos = x_start;
   int y_pos = y_start;
   
   u32 tag = addr_val & 0x1FFFFC00;
   u32 lru = (addr_val >> 4) & 0x3f;
   u32 v = (addr_val >> 2) & 1;

   y_pos += 2;

   vdp_printf(&test_disp_font, x_pos * 8, y_pos * 8, 0xC, "tag: 0x%08x lru: 0x%02x v: 0x%01x", tag, lru, v);
}

//////////////////////////////////////////////////////////////////////////////

void print_cache_for_way(int x_start, int y_start, int way, int index)
{
   int x_pos = x_start;
   int y_pos = y_start;

   vdp_printf(&test_disp_font, x_pos * 8, y_pos * 8, 0xC, "0x%02x", way);

   x_pos += 5;

   SH2REG_CCR = way << 6;

   u32 addr = 0x60000000 | (index << 4);
   u32 addr_val = (*(volatile u32 *)addr);

   vdp_printf(&test_disp_font, x_pos * 8, y_pos * 8, 0xC, "0x%08x", addr_val);

   x_pos += 11;

   u32 data = data_array[(index * 4) + 0 + (way * 0x100)];
   vdp_printf(&test_disp_font, x_pos * 8, y_pos * 8, 0xC, "0x%08x", data);

   x_pos += 11;
   
   data = data_array[(index * 4) + 1 + (way * 0x100)];
   vdp_printf(&test_disp_font, x_pos * 8, y_pos * 8, 0xC, "0x%08x", data);

   x_pos -= 11;
   y_pos += 1;
   
   data = data_array[(index * 4) + 2 + (way * 0x100)];
   vdp_printf(&test_disp_font, x_pos * 8, y_pos * 8, 0xC, "0x%08x", data);

   x_pos += 11;
   
   data = data_array[(index * 4) + 3 + (way * 0x100)];
   vdp_printf(&test_disp_font, x_pos * 8, y_pos * 8, 0xC, "0x%08x", data);

   x_pos = 0;

   print_addr_data(x_pos, y_pos, addr_val);
}

//////////////////////////////////////////////////////////////////////////////

void print_cache_all_ways(int y_start, int index)
{
   int y = y_start;

   vdp_printf(&test_disp_font, 0 * 8, y * 8, 0xC, "index: 0x%02x", index);
   y += 2;

   int way;
   
   for (way = 0; way < 4; way++)
   {
      print_cache_for_way(0, y, way, index);
      y += 6;
   }
}

//////////////////////////////////////////////////////////////////////////////

void print_cache_at_index(int index)
{
   clear_framebuffer();

   vdp_printf(&test_disp_font, 0 * 8, 0 * 8, 0xC, "way    address    data");

   print_cache_all_ways(2, index);
}

//////////////////////////////////////////////////////////////////////////////

void cache_print_and_wait()
{
   int index = 0;

   print_cache_at_index(0);

   for (;;)
   {
      vdp_vsync();

      if (per[0].but_push_once & PAD_A)
      {
         break;
      }

      if (per[0].but_push_once & PAD_Y)
      {
         reset_system();
      }

      if (per[0].but_push_once & PAD_UP)
      {
         index--;
         if (index < 0)
            index = 0;
         print_cache_at_index(index);
      }

      if (per[0].but_push_once & PAD_DOWN)
      {
         index++;
         print_cache_at_index(index);
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

void cache_test()
{
   int i = 0;
   int way = 0;

   SH2REG_CCR = 0;//disable cache

   //clear address arrays
   for (way = 0; way < 4; way++)
   {
      //bank switch
      SH2REG_CCR = way << 6;

      for (i = 0; i < 64; i++)
      {
         address_array[i] = 0;
      }
   }

   //clear data array
   for (i = 0; i < 0x400; i++)
      data_array[i] = 0;

   wram_cache_through[0x11000] = 0xdeadbeef;
   wram_cache_through[0x11001] = 0xcafef00d;
   wram_cache_through[0x11002] = 0xb01dface;
   wram_cache_through[0x11003] = 0xba5eba11;

   for (i = 0; i < 64; i++)
   {
      wram_cache_through[0x12000 + i] = i;
   }

   SH2REG_CCR = (1 << 4); //purge
   SH2REG_CCR = 1; //enable

   //test write
   wram_cache[0x10000] = wram_cache[0x11000];

   SH2REG_CCR = 0;//disable cache

   way = 0;

   cache_print_and_wait();

   //part 2

   for (i = 0; i < 64; i+=4)
   {
      SH2REG_CCR = 1; //enable

      wram_cache[0x13000 + i] = wram_cache[0x12000 + i];

      SH2REG_CCR = 0;//disable

      cache_print_and_wait();
   }
}

void test_sh2_dma_impl(
   u32 src_addr, 
   u32 dst_addr,
   u32 count, 
   u8 transfer_size
   )
{
   volatile u32 *source = (volatile u32 *)(src_addr);
   volatile u32 *dest = (volatile u32 *)(dst_addr);
   int i;

   clear_framebuffer();

   //read te
   volatile u32 dummy = SH2REG_CHCR0;

   //and clear it
   SH2REG_CHCR0 = 0;

   //read nmif
   dummy = SH2REG_DMAOR;

   //and clear it
   SH2REG_DMAOR = 0;

   for (i = 0; i < count; i+=2)
   {
      source[i] =   0xdeadbeef;
      source[i+2] = 0xcafef00d;
   }

   for (i = 0; i < count; i++)
   {
      dest[i] = 0;
   }

   u8 round_robin = 0;
   u8 all_dma_enabled = 1;

   SH2REG_DMAOR =
      (round_robin << 3) |
      all_dma_enabled;

   SH2REG_SAR0 = src_addr;
   SH2REG_DAR0 = dst_addr;
   SH2REG_TCR0 = count;

   u8 destination_address_mode = 1;
   u8 source_address_mode = 1;
   u8 transfer_enabled = 1;
   u8 auto_request_mode = 1;//must be set

   SH2REG_CHCR0 =
      (destination_address_mode << 14) |
      (source_address_mode << 12) |
      (transfer_size << 10) |
      (auto_request_mode << 9) |
      transfer_enabled;

   while (!(SH2REG_CHCR0 & 2)) {}//wait for TE to be set

   for (;;)
   {
      vdp_vsync();

      vdp_printf(&test_disp_font, 0 * 8, 4 * 8, 0xC, "SAR0 0x%08x DAR0 0x%08x", 
         SH2REG_SAR0,
         SH2REG_DAR0);

      vdp_printf(&test_disp_font, 0 * 8, 5 * 8, 0xC, "TCR0 0x%08x CHR0 0x%08x",
         SH2REG_TCR0,
         SH2REG_CHCR0);

      vdp_printf(&test_disp_font, 0 * 8, 6 * 8, 0xC, "DMAOR 0x%08x",
         SH2REG_DMAOR);

      volatile u8 de = SH2REG_CHCR0 & 1;
      volatile u8 dme = SH2REG_DMAOR & 1;
      volatile u8 te = (SH2REG_CHCR0 >> 1) & 1;
      volatile u8 nmif = (SH2REG_DMAOR >> 1) & 1;
      volatile u8 ae = (SH2REG_DMAOR >> 2) & 1;

      vdp_printf(&test_disp_font, 0 * 8, 7 * 8, 0xC, "DE %01x DME %01x TE %01x NMIF %01x AE %01x",
         de,dme,te,nmif,ae);

      if (per[0].but_push_once & PAD_Y)
      {
         reset_system();
      }

      if (per[0].but_push_once & PAD_A)
      {
         break;
      }
   }
}

void test_sh2_dma()
{
   test_sh2_dma_impl(0x260ED000, 0x25E00000, 0x1000, 2);
   test_sh2_dma_impl(0x260ED000, 0x25E00000, 0x800, 3);
}

u32 do_test_asm(u32 value, u32 addr, int is_write, int size, u32 * b2, int num_times)
{
   u32 a = value;
   u32 b = addr;
   u32 c = num_times;
   u32 a_out, b_out, sr;

   if (is_write)
   {
      if (size == 0)
      {
         asm(
            "mov %[a],r0\n\t"
            "mov %[b],r1\n\t"
            "mov %[c],r2\n\t"
            "mov #0, r3\n\t"//loop counter
            "loop%=:  \n\t"
            "cmp/ge r2,r3 \n\t"
            "bt end%= \n\t"
            "add #1, r3 \n\t"
            "mov.b r0, @r1\n\t"
            "bra loop%=        \n\t"
            "nop           \n\t"
            "end%=:           \n\t"
            "mov r0,%[a_out]\n\t"
            "mov r1,%[b_out]\n\t"
            : [a_out] "=r" (a_out), [b_out] "=r" (b_out), [sr] "=r" (sr)//output
            : [a] "r" (a), [b] "r" (b), [c] "r" (c)//input
            : "r0", "r1", "r2", "r3");//clobbers
      }
      else if (size == 1)
      {
         asm(
            "mov %[a],r0\n\t"
            "mov %[b],r1\n\t"
            "mov %[c],r2\n\t"
            "mov #0, r3\n\t"//loop counter
            "loop%=:  \n\t"
            "cmp/ge r2,r3 \n\t"
            "bt end%= \n\t"
            "add #1, r3 \n\t"
            "mov.w r0, @r1\n\t"
            "bra loop%=        \n\t"
            "nop           \n\t"
            "end%=:           \n\t"
            "mov r0,%[a_out]\n\t"
            "mov r1,%[b_out]\n\t"
            : [a_out] "=r" (a_out), [b_out] "=r" (b_out), [sr] "=r" (sr)//output
            : [a] "r" (a), [b] "r" (b), [c] "r" (c)//input
            : "r0", "r1", "r2", "r3");//clobbers
      }
      else
      {
         asm(
            "mov %[a],r0\n\t"
            "mov %[b],r1\n\t"
            "mov %[c],r2\n\t"
            "mov #0, r3\n\t"//loop counter
            "loop%=:  \n\t"
            "cmp/ge r2,r3 \n\t"
            "bt end%= \n\t"
            "add #1, r3 \n\t"
            "mov.l r0, @r1\n\t"
            "bra loop%=        \n\t"
            "nop           \n\t"
            "end%=:           \n\t"
            "mov r0,%[a_out]\n\t"
            "mov r1,%[b_out]\n\t"
            : [a_out] "=r" (a_out), [b_out] "=r" (b_out), [sr] "=r" (sr)//output
            : [a] "r" (a), [b] "r" (b), [c] "r" (c)//input
            : "r0", "r1", "r2", "r3");//clobbers
      }


   }
   else
   {
      if (size == 0)
      {
         asm(
            "mov %[a],r0\n\t"
            "mov %[b],r1\n\t"
            "mov %[c],r2\n\t"
            "mov #0, r3\n\t"//loop counter
            "loop%=:  \n\t"
            "cmp/ge r2,r3 \n\t"
            "bt end%= \n\t"
            "add #1, r3 \n\t"
            "mov.b @r1, r0\n\t"
            "bra loop%=        \n\t"
            "nop           \n\t"
            "end%=:           \n\t"
            "mov r0,%[a_out]\n\t"
            "mov r1,%[b_out]\n\t"
            : [a_out] "=r" (a_out), [b_out] "=r" (b_out), [sr] "=r" (sr)//output
            : [a] "r" (a), [b] "r" (b), [c] "r" (c)//input
            : "r0", "r1", "r2", "r3");//clobbers
      }
      else if (size == 1)
      {
         asm(
            "mov %[a],r0\n\t"
            "mov %[b],r1\n\t"
            "mov %[c],r2\n\t"
            "mov #0, r3\n\t"//loop counter
            "loop%=:  \n\t"
            "cmp/ge r2,r3 \n\t"
            "bt end%= \n\t"
            "add #1, r3 \n\t"
            "mov.w @r1, r0\n\t"
            "bra loop%=       \n\t"
            "nop           \n\t"
            "end%=:           \n\t"
            "mov r0,%[a_out]\n\t"
            "mov r1,%[b_out]\n\t"
            : [a_out] "=r" (a_out), [b_out] "=r" (b_out), [sr] "=r" (sr)//output
            : [a] "r" (a), [b] "r" (b), [c] "r" (c)//input
            : "r0", "r1", "r2", "r3");//clobbers
      }
      else
      {
         asm(
            "mov %[a],r0\n\t"
            "mov %[b],r1\n\t"
            "mov %[c],r2\n\t"
            "mov #0, r3\n\t"//loop counter
            "loop%=:  \n\t"
            "cmp/ge r2,r3 \n\t"
            "bt end%= \n\t"
            "add #1, r3 \n\t"
            "mov.l @r1, r0\n\t"
            "bra loop%=        \n\t"
            "nop           \n\t"
            "end%=:           \n\t"
            "mov r0,%[a_out]\n\t"
            "mov r1,%[b_out]\n\t"
            : [a_out] "=r" (a_out), [b_out] "=r" (b_out), [sr] "=r" (sr)//output
            : [a] "r" (a), [b] "r" (b), [c] "r" (c)//input
            : "r0", "r1", "r2", "r3");//clobbers
      }

   }

   *b2 = b_out;

   return a_out;
}

void clear_framebuffer();

void sh2_write_timing(u32 destination, char*test_name, int is_write, int size)
{
   clear_framebuffer();

   int print_pos = 10;

   int i;

   if (is_write)
      vdp_printf(&test_disp_font, 0 * 8, 1 * 8, 0xF, "write to %s", test_name);
   else
      vdp_printf(&test_disp_font, 0 * 8, 1 * 8, 0xF, "read from %s", test_name);

   if (size == 0)
      vdp_printf(&test_disp_font, 0 * 8, 2 * 8, 0xF, "byte");
   else if (size == 1)
      vdp_printf(&test_disp_font, 0 * 8, 2 * 8, 0xF, "word");
   else
      vdp_printf(&test_disp_font, 0 * 8, 2 * 8, 0xF, "long");

   for (i = 0; i < 5; i++)
   {
      vdp_wait_vblankin();

      int num_writes = 2048;

      //zero watchdog timer
      SH2_REG_WTCNT_W(0);
      RSTCSR_W = 0;

      //enable timer
      SH2_REG_WTCSR_W(1 << 5);

      u8 wdt_end_time = SH2_REG_WTCNT_R;

      //disable timer
      SH2_REG_WTCSR_W(0);

      frc_clear();
      u32 b2 = 0;

      u32 output = do_test_asm(0xdeadbeef, destination, is_write, size, &b2, num_writes);

      u32 frc_end_time = frc_get();

      vdp_printf(&test_disp_font, 0 * 8, print_pos * 8, 0xF, "frc: %d (~%d cycles), ~%d cycles", frc_end_time, frc_end_time * 8, (frc_end_time * 8) / num_writes);
      vdp_printf(&test_disp_font, 0 * 8, (print_pos + 6) * 8, 0xF, "%08X %08X", output, b2);
      print_pos++;
   }

   for (;;)
   {
      vdp_wait_vblankin();

      if (per[0].but_push_once & PAD_A)
      {
         break;
      }

      if (per[0].but_push_once & PAD_Y)
      {
         reset_system();
      }
   }
}

void do_timing(int is_write, int size)
{

   int i = is_write;
   int j = size;

   sh2_write_timing(0x20000000, "bios", i, j);

   sh2_write_timing(0x20100000, "smpc", i, j);

   sh2_write_timing(0x25E00000, "vdp2 vram", i, j);

   sh2_write_timing(0x25f00000, "vdp2 color ram", i, j);

   if ((i != 1) && (j != 2))//crashes
      sh2_write_timing(0x25f80026, "vdp2 regs", i, j);

   sh2_write_timing(0x25a00000, "scsp ram", i, j);

   sh2_write_timing(0x25b00000, "scsp regs", i, j);

   sh2_write_timing(0x25C00000, "vdp1 ram", i, j);

   sh2_write_timing(0x25C80000, "vdp1 framebuffer", i, j);

   sh2_write_timing(0x25d00000, "vdp1 regs", i, j);

   sh2_write_timing(0x20200000, "low work ram", i, j);

   sh2_write_timing(0x260FF000, "high work ram", i, j);

   sh2_write_timing(0x25890008, "ygr", i, j);

   sh2_write_timing(0x25fe0000, "scu regs", i, j);
}

void sh2_write_timing_test()
{
   (*(volatile u8  *)0xFFFFFE92) = 1;//enable cache

   int i = 1;

   for (i = 0; i < 2; i++)
   {
      int j;
      for (j = 0; j < 3; j++)
      {
         do_timing(i, j);
      }
   }
}