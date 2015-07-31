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

#include "tests.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

extern vdp2_settings_struct vdp2_settings;

void vdp2_nbg0_test ();
void vdp2_nbg1_test ();
void vdp2_nbg2_test ();
void vdp2_nbg3_test ();
void vdp2_rbg0_test ();
void vdp2_rbg1_test ();
void vdp2_window_test ();
void vdp2_all_scroll_test();
void vdp2_line_color_screen_test();
void vdp2_extended_color_calculation_test();
void vdp2_sprite_priority_shadow_test();
void vdp2_special_priority_test();
//////////////////////////////////////////////////////////////////////////////

void hline(int x1, int y1, int x2, u8 color)
{
   int i;
   volatile u8 *buf=(volatile u8 *)(0x25E00000+(y1 * 512));

   for (i = x1; i < (x2+1); i++)
      buf[i] = color;
}

//////////////////////////////////////////////////////////////////////////////

void vline(int x1, int y1, int y2, u8 color)
{
   int i;
   volatile u8 *buf=(volatile u8 *)(0x25E00000+(y1 * 512) + x1);

   for (i = 0; i < (y2-y1+1); i++)
      buf[i * 512] = color;
}

//////////////////////////////////////////////////////////////////////////////

void draw_box(int x1, int y1, int x2, int y2, u8 color)
{
   hline(x1, y1, x2, color); 
   hline(x1, y2, x2, color); 
   vline(x1, y1, y2, color);
   vline(x2, y1, y2, color);
}

//////////////////////////////////////////////////////////////////////////////

void working_query(const char *question)
{
   // Ask the user if it's visible
   vdp_printf(&test_disp_font, 2 * 8, 20 * 8, 0xF, "%s", question);
   vdp_printf(&test_disp_font, 2 * 8, 21 * 8, 0xF, "C - Yes B - No");

   for(;;)
   {
      vdp_vsync();

      if (per[0].but_push_once & PAD_B)
      {
         stage_status = STAGESTAT_BADGRAPHICS;
         break;
      }
      else if (per[0].but_push_once & PAD_C)
      {
         stage_status = STAGESTAT_DONE;
         break;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

void vdp2_test()
{
   // Put system in minimalized state
   interrupt_set_level_mask(0xF);

   init_iapetus(RES_320x224);
   vdp_rbg0_init(&test_disp_settings);
   vdp_set_default_palette();

   // Display On
   vdp_disp_on();

   unregister_all_tests();
//   register_test(&Vdp2InterruptTest, "Sound Request Interrupt");
//   register_test(&Vdp2RBG0Test, "RBG0 bitmap");
   register_test(&vdp2_window_test, "Window test");
   //register_test(&vdp2_all_scroll_test, "NBG0-4 scroll test");
   //register_test(&vdp2_line_color_screen_test, "Line color screen test");
   //register_test(&vdp2_extended_color_calculation_test, "Extended color calc test");
   //register_test(&vdp2_sprite_priority_shadow_test, "Sprite priority shadow test");
   //register_test(&vdp2_special_priority_test, "Special priority test");

   do_tests("VDP2 Screen tests", 0, 0);
}

//////////////////////////////////////////////////////////////////////////////

void load_font_8x8_to_vram_1bpp_to_4bpp(u32 tile_start_address, u32 ram_pointer)
{
   int x, y;
   int chr;
   volatile u8 *dest = (volatile u8 *)(ram_pointer + tile_start_address);

   for (chr = 0; chr < 128; chr++)//128 ascii chars total
   {
      for (y = 0; y < 8; y++)
      {
         u8 scanline = font_8x8[(chr * 8) + y];//get one scanline

         for (x = 0; x < 8; x++)
         {
            //get the corresponding bit for the x pos
            u8 bit = (scanline >> (x ^ 7)) & 1;

            if ((x & 1) == 0) 
               bit *= 16;
            
            dest[(chr * 32) + (y * 4) + (x / 2)] |= bit;
         }
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

void write_str_as_pattern_name_data_special(int x_pos, int y_pos, const char* str, 
   int palette, u32 base, u32 tile_start_address,int special_priority,
   int special_color)
{
   int x;

   int len = strlen(str);

   for (x = 0; x < len; x++)
   {
      int name = str[x];

      int offset = x + x_pos;

      volatile u32 *p = (volatile u32 *)(VDP2_RAM + base);
      //64 cells across in the plane
      p[(y_pos * 64) + offset] = (special_priority << 29) | 
         (special_color << 28) | (tile_start_address >> 5) | name | 
         (palette << 16);
   }
}

//////////////////////////////////////////////////////////////////////////////

void write_str_as_pattern_name_data(int x_pos, int y_pos, const char* str,
   int palette, u32 base, u32 tile_start_address)
{
   write_str_as_pattern_name_data_special(x_pos, y_pos, str, palette, base, tile_start_address, 0, 0);  
}

//////////////////////////////////////////////////////////////////////////////

//simple menu and reg writing system for tests that need a lot of reg changes
#define REG_ADJUSTER_MAX_VARS 28
#define REG_ADJUSTER_STRING_LEN 32

struct RegAdjusterVarInfo
{
   char name[REG_ADJUSTER_STRING_LEN];
   int num_values;
   int value;
   int *dest;
};

struct RegAdjusterState
{
   int repeat_timer;
   int menu_selection;
   int num_menu_items;
   struct RegAdjusterVarInfo vars[REG_ADJUSTER_MAX_VARS];
};

//////////////////////////////////////////////////////////////////////////////

void ra_add_var(struct RegAdjusterState* s, int * dest, char* name, int num_vals)
{
   strcpy(s->vars[s->num_menu_items].name, name);
   s->vars[s->num_menu_items].num_values = num_vals;
   s->vars[s->num_menu_items].dest = dest;
   s->num_menu_items++;
}

//////////////////////////////////////////////////////////////////////////////

void ra_update_vars(struct RegAdjusterState* s)
{
   int i;
   for (i = 0; i < s->num_menu_items; i++)
   {
      *s->vars[i].dest = s->vars[i].value;
   }
}

//////////////////////////////////////////////////////////////////////////////

void ra_add_array(struct RegAdjusterState* s, int(*vars)[], int length, char* name, int num_vals)
{
   int i;
   for (i = 0; i < length; i++)
   {
      char str[REG_ADJUSTER_STRING_LEN] = { 0 };
      strcpy(str, name);

      char str2[REG_ADJUSTER_STRING_LEN] = { 0 };
      sprintf(str2, "%d", i);
      strcat(str, str2);
      ra_add_var(s, &(*vars)[i], str, num_vals);
   }
}

//////////////////////////////////////////////////////////////////////////////

void ra_do_menu(struct RegAdjusterState* s, int x_pos, int y_pos)
{
   int i;
   for (i = 0; i < s->num_menu_items; i++)
   {
      char current_line[REG_ADJUSTER_STRING_LEN] = { 0 };

      if (s->menu_selection == i)
      {
         strcat(current_line, ">");
      }
      else
      {
         strcat(current_line, " ");
      }
      char value[REG_ADJUSTER_STRING_LEN] = { 0 };
      sprintf(value, "=%02d", s->vars[i].value);
      strcat(current_line, s->vars[i].name);
      strcat(current_line, value);
      write_str_as_pattern_name_data(x_pos, i+y_pos, current_line, 3, 0x000000, 0x40000);
   }

   if (per[0].but_push_once & PAD_UP)
   {
      s->menu_selection--;
      if (s->menu_selection < 0)
      {
         s->menu_selection = s->num_menu_items - 1;
      }
   }

   if (per[0].but_push_once & PAD_DOWN)
   {
      s->menu_selection++;
      if (s->menu_selection >(s->num_menu_items - 1))
      {
         s->menu_selection = 0;
      }
   }

   if (per[0].but_push_once & PAD_LEFT)
   {
      s->vars[s->menu_selection].value--;
      if (s->vars[s->menu_selection].value < 0)
      {
         s->vars[s->menu_selection].value = s->vars[s->menu_selection].num_values;
      }
   }

   if (per[0].but_push_once & PAD_RIGHT)
   {
      s->vars[s->menu_selection].value++;
      if (s->vars[s->menu_selection].value > s->vars[s->menu_selection].num_values)
      {
         s->vars[s->menu_selection].value = 0;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

void ra_do_preset(struct RegAdjusterState* s, int * vars)
{
   int i;
   for (i = 0; i < s->num_menu_items; i++)
   {
      s->vars[i].value = vars[i];
   }
}

//////////////////////////////////////////////////////////////////////////////

void vdp2_scroll_test_set_scroll(int pos)
{
   //scroll the first two layers a little slower
   //than 1px per frame in the x direction
   *(volatile u32 *)0x25F80070 = pos << 15;
   VDP2_REG_SCYIN0 = pos;

   *(volatile u32 *)0x25F80080 = -(pos << 15);
   VDP2_REG_SCYIN1 = pos;

   VDP2_REG_SCXN2 = pos;
   VDP2_REG_SCYN2 = pos;

   VDP2_REG_SCXN3 = -pos;
   VDP2_REG_SCYN3 = pos;
}

//////////////////////////////////////////////////////////////////////////////

void vdp2_scroll_test_set_map(screen_settings_struct* settings, int which)
{
   int i;

   for (i = 0; i < 4; i++)
   {
      settings->map[i] = which;
   }
}

//////////////////////////////////////////////////////////////////////////////

void vdp2_basic_tile_scroll_setup(const u32 tile_address)
{
   int i;

   vdp_rbg0_deinit();

   VDP2_REG_CYCA0L = 0x0123;
   VDP2_REG_CYCA0U = 0xFFFF;
   VDP2_REG_CYCB0L = 0xF4F5;
   VDP2_REG_CYCB0U = 0xFF76;

   load_font_8x8_to_vram_1bpp_to_4bpp(tile_address, VDP2_RAM);

   screen_settings_struct settings;

   settings.color = 0;
   settings.is_bitmap = 0;
   settings.char_size = 0;
   settings.pattern_name_size = 0;
   settings.plane_size = 0;
   vdp2_scroll_test_set_map(&settings, 0);
   settings.transparent_bit = 0;
   settings.map_offset = 0;
   vdp_nbg0_init(&settings);

   vdp2_scroll_test_set_map(&settings, 1);
   vdp_nbg1_init(&settings);

   vdp2_scroll_test_set_map(&settings, 2);
   vdp_nbg2_init(&settings);

   vdp2_scroll_test_set_map(&settings, 3);
   vdp_nbg3_init(&settings);

   vdp_set_default_palette();

   //set pattern name data to the empty font tile
   for (i = 0; i < 256; i++)
   {
      write_str_as_pattern_name_data(0, i, "                                                                ", 0, 0x000000, tile_address);
   }

   vdp_disp_on();
}

//////////////////////////////////////////////////////////////////////////////


void vdp2_basic_tile_scroll_deinit()
{
   int i;
   //restore settings so the menus don't break

   vdp_nbg0_deinit();
   vdp_nbg1_deinit();
   vdp_nbg2_deinit();
   vdp_nbg3_deinit();

   vdp_rbg0_init(&test_disp_settings);

   //clear the vram we used
   for (i = 0; i < 0x11000; i++)
   {
      vdp2_ram[i] = 0;
   }

   VDP2_REG_CYCA0U = 0xffff;
   VDP2_REG_CYCB0U = 0xffff;

   VDP2_REG_MPABN1 = 0x0000;
   VDP2_REG_MPCDN1 = 0x0000;
   VDP2_REG_MPABN2 = 0x0000;
   VDP2_REG_MPCDN2 = 0x0000;
   VDP2_REG_MPABN3 = 0x0000;
   VDP2_REG_MPCDN3 = 0x0000;

   VDP2_REG_PRIR  = 0x0002;
   VDP2_REG_PRISA = 0x0101;
   VDP2_REG_PRISB = 0x0101;
   VDP2_REG_PRISC = 0x0101;
   VDP2_REG_PRISD = 0x0101;

   VDP2_REG_CCCTL = 0;
   VDP2_REG_LNCLEN = 0;
   VDP2_REG_RAMCTL = 0x1000;
   VDP2_REG_CCRNA = 0;
   VDP2_REG_CCRNB = 0;
   VDP2_REG_CCRLB = 0;

   yabauseut_init();
}
//////////////////////////////////////////////////////////////////////////////

void vdp2_all_scroll_test()
{
   int i;
   const u32 tile_address = 0x40000;

   vdp2_basic_tile_scroll_setup(tile_address);

   for (i = 0; i < 64; i += 4)
   {
      write_str_as_pattern_name_data(0, i, "A button: Start scrolling. NBG0. Testing NBG0. This is NBG0.... ", 3, 0x000000, tile_address);
      write_str_as_pattern_name_data(0, i + 1, "B button: Stop scrolling.  NBG1. Testing NBG1. This is NBG1.... ", 4, 0x004000, tile_address);
      write_str_as_pattern_name_data(0, i + 2, "C button: Reset.           NBG2. Testing NBG2. This is NBG2.... ", 5, 0x008000, tile_address);
      write_str_as_pattern_name_data(0, i + 3, "Start:    Exit.            NBG3. Testing NBG3. This is NBG3.... ", 6, 0x00C000, tile_address);
   }

   int do_scroll = 0;
   int scroll_pos = 0;
   int framecount = 0;

   for (;;)
   {
      vdp_vsync();

      framecount++;

      if (do_scroll)
      {
         if ((framecount % 3) == 0)
            scroll_pos++;

         vdp2_scroll_test_set_scroll(scroll_pos);
      }

      if (per[0].but_push_once & PAD_A)
      {
         do_scroll = 1;
      }

      if (per[0].but_push_once & PAD_B)
      {
         do_scroll = 0;
      }

      if (per[0].but_push_once & PAD_C)
      {
         do_scroll = 0;
         scroll_pos = 0;
         vdp2_scroll_test_set_scroll(scroll_pos);
      }

      if (per[0].but_push_once & PAD_START)
         break;
   }

   vdp2_basic_tile_scroll_deinit();
}

//////////////////////////////////////////////////////////////////////////////

void vdp2_line_color_screen_test_set_up_line_screen(const u32 table_address)
{
   int i;

   //create palette entries used by line screen
   //and write them to color ram

   int r = 0, g = 0, b = 31;

   const int line_palette_start = 256;//starting entry in color ram

   volatile u16 * color_ram_ptr = (volatile u16 *)(VDP2_CRAM + line_palette_start * 2);

   for (i = 0; i < 224; i++)
   {
      if ((i % 8) == 0)
      {
         r++;
         b--;
      }
      color_ram_ptr[i] = RGB555(r, g, b);
   }

   //write line screen table to vram
   volatile u16 *color_table_ptr = (volatile u16 *)(VDP2_RAM + table_address);

   for (i = 0; i < 224; i++)
   {
      color_table_ptr[i] = i + line_palette_start;
   }
}

//////////////////////////////////////////////////////////////////////////////

struct Ccctl {
   int exccen, ccrtmd, ccmd, spccen, lcccen, r0ccen;
   int nccen[4];
};

void do_color_ratios(int *framecount, int * ratio, int *ratio_dir)
{
   *framecount = *framecount + 1;

   if ((*framecount % 3) == 0)
      *ratio = *ratio_dir + *ratio;

   if (*ratio > 30)
      *ratio_dir = -1;
   if (*ratio < 2)
      *ratio_dir = 1;
}

//////////////////////////////////////////////////////////////////////////////

void vdp2_line_color_screen_test()
{
   const u32 tile_address = 0x40000;
   const u32 table_address = 0x10000;
   int i;

   vdp2_basic_tile_scroll_setup(tile_address);

   vdp2_line_color_screen_test_set_up_line_screen(table_address);

   //set up instructions
   char * instructions[] = {
      "Controls:     (START to exit)          ",
      "A:     ccmd   (as is/ratio)    on/off  ",
      "B:     ccrtmd (2nd/top)        on/off  ",
      "C:     lnclen (insert line)    on/off  ",
      "X:     exccen (extended calc)  on/off  ",
      "Y:     NBG0-4 ratio update     on/off  ",
      "Z:     line color per line     on/off  ",
      "UP:    set bugged emu settings #1      ",
      "LEFT:  set bugged emu settings #2      ",
      "RIGHT: set bugged emu settings #3      "
   };

   for (i = 0; i < 10; i++)
   {
      write_str_as_pattern_name_data(0, 8 + i, instructions[i], 3, 0x000000, tile_address);
   }

   //test pattern at bottom of screen
   for (i = 20; i < 28; i += 4)
   {
      
      write_str_as_pattern_name_data(0, i, "\n\n\n\nNBG0\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\nNBG0\n\n\n\n", 3, 0x000000, tile_address);
      write_str_as_pattern_name_data(0, i + 1, "\n\n\n\nNBG1\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\nNBG1\n\n\n\n", 3, 0x004000, tile_address);
      write_str_as_pattern_name_data(0, i + 2, "\n\n\n\nNBG2\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\nNBG2\n\n\n\n", 3, 0x008000, tile_address);
      write_str_as_pattern_name_data(0, i + 3, "\n\n\n\nNBG3\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\nNBG3\n\n\n\n", 3, 0x00C000, tile_address);
   }

   //overlapping tiles, so that they will be colored differently from the rest of the screen depending on settings
   write_str_as_pattern_name_data(0, 19, "NBG0 and NBG1 overlap on this line.      ", 3, 0x000000, tile_address);
   write_str_as_pattern_name_data(0, 19, "NBG0 and NBG1 overlap on this line.      ", 4, 0x004000, tile_address);

   struct Ccctl ccctl;

   ccctl.exccen = 0;
   ccctl.ccrtmd = 1;
   ccctl.ccmd = 0;
   ccctl.lcccen = 1;

   int lnclen = 0x3f;
   int lcclmd = 1;

   int framecount = 0;
   int ratio = 0;
   int ratio_dir = 1;
   int update_nbg_ratios = 1;
   int nbg_ratio[4] = {0};
   char ratio_status_str[64];

   for (;;)
   {
      vdp_vsync();

      VDP2_REG_CCCTL = (ccctl.exccen << 10) | (ccctl.ccrtmd << 9) | (ccctl.ccmd << 8) | (ccctl.lcccen << 5) | 0xf;
      VDP2_REG_LNCLEN = lnclen;
      *(volatile u32 *)0x25F800A8 = (lcclmd << 31) | (table_address / 2);

      //update color calculation ratios
      do_color_ratios(&framecount, &ratio, &ratio_dir);

      if (update_nbg_ratios)
      {
         nbg_ratio[2] = nbg_ratio[0] = ((-ratio) & 0x1f);
         nbg_ratio[3] = nbg_ratio[1] = ratio;

         VDP2_REG_CCRNA = nbg_ratio[0] | nbg_ratio[1] << 8;
         VDP2_REG_CCRNB = nbg_ratio[2] | nbg_ratio[3] << 8;
      }

      VDP2_REG_CCRLB = ratio;

      //update register status
      if (ccctl.ccmd)
      {
         write_str_as_pattern_name_data(0, 0, "CCMD  =   1  Add as is                   ", 3, 0x000000, tile_address);
      }
      else
      {
         write_str_as_pattern_name_data(0, 0, "CCMD  =   0  Add according to reg value  ", 3, 0x000000, tile_address);
      }

      if (ccctl.ccrtmd)
      {
         write_str_as_pattern_name_data(0, 1, "CCRTMD=   1  Select per 2nd screen       ", 3, 0x000000, tile_address);
      }
      else
      {
         write_str_as_pattern_name_data(0, 1, "CCRTMD=   0  Select per top screen       ", 3, 0x000000, tile_address);
      }

      if (lnclen)
      {
         write_str_as_pattern_name_data(0, 2, "LNCLEN=0x3f  Line screen inserts on all  ", 3, 0x000000, tile_address);
      }
      else
      {
         write_str_as_pattern_name_data(0, 2, "LNCLEN=0x00  Line screen disabled on all ", 3, 0x000000, tile_address);
      }

      if (ccctl.exccen)
      {
         write_str_as_pattern_name_data(0, 3, "EXCCEN=   1  Extended color calc on      ", 3, 0x000000, tile_address);
      }
      else
      {
         write_str_as_pattern_name_data(0, 3, "EXCCEN=   0  Extended color calc off     ", 3, 0x000000, tile_address);
      }
      if (lcclmd)
      {
         write_str_as_pattern_name_data(0, 4, "LCCLMD=   1  Line color per line         ", 3, 0x000000, tile_address);
      }
      else
      {
         write_str_as_pattern_name_data(0, 4, "LCCLMD=   0  Single color                ", 3, 0x000000, tile_address);
      }

      sprintf(ratio_status_str, "Ratios    NBG0=%04x NBG1=%04x           ", nbg_ratio[0], nbg_ratio[1]);
      write_str_as_pattern_name_data(0, 5, ratio_status_str, 3, 0x000000, tile_address);

      sprintf(ratio_status_str, "          NBG2=%04x NBG3=%04x LINE=%04x ", nbg_ratio[2], nbg_ratio[3], ratio);
      write_str_as_pattern_name_data(0, 6, ratio_status_str, 3, 0x000000, tile_address);

      //handle user inputs
      if (per[0].but_push_once & PAD_A)
      {
         ccctl.ccmd = !ccctl.ccmd;
      }

      if (per[0].but_push_once & PAD_B)
      {
         ccctl.ccrtmd = !ccctl.ccrtmd;
      }

      if (per[0].but_push_once & PAD_C)
      {
         if (lnclen == 0)
         {
            lnclen = 0x3f;//enable all
         }
         else
         {
            lnclen = 0;//disable all
         }
      }

      if (per[0].but_push_once & PAD_X)
      {
         ccctl.exccen = !ccctl.exccen;
      }

      if (per[0].but_push_once & PAD_Y)
      {
         update_nbg_ratios = !update_nbg_ratios;
      }

      if (per[0].but_push_once & PAD_Z)
      {
         lcclmd = !lcclmd;
      }

      if (per[0].but_push_once & PAD_UP)
      {
         ccctl.ccrtmd = 1;
         ccctl.ccmd = 0;
         ccctl.exccen = 0;
         update_nbg_ratios = 0;
         lcclmd = 0;
         lnclen = 0x3f;
      }

      if (per[0].but_push_once & PAD_LEFT)
      {
         ccctl.ccrtmd = 1;
         ccctl.ccmd = 0;
         ccctl.exccen = 0;
         update_nbg_ratios = 0;
         lcclmd = 1;
         lnclen = 0x3f;
      }

      if (per[0].but_push_once & PAD_RIGHT)
      {
         ccctl.ccrtmd = 0;
         ccctl.ccmd = 1;
         ccctl.exccen = 0;
         update_nbg_ratios = 1;
         lcclmd = 1;
         lnclen = 0x3f;
      }

      if (per[0].but_push_once & PAD_START)
         break;
   }

   vdp2_basic_tile_scroll_deinit();
   vdp_set_default_palette();
}

//////////////////////////////////////////////////////////////////////////////

void write_large_font(int x_pos, int y_pos, int* number, int palette, u32 base, const u32 tile_address)
{
   int y, x, j = 0;
   for (y = 0; y < 5; y++)
   {
      for (x = 0; x < 3; x++)
      {
         if (number[j])
         {
            write_str_as_pattern_name_data(x + x_pos, y + y_pos, "\n", palette, base, tile_address);
         }
         j++;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

void vdp2_extended_color_calculation_test()
{
   const u32 tile_address = 0x40000;
   const u32 line_table_address = 0x10000;
   int zero[] = { 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1 };
   int one[] = { 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0 };
   int two[] = { 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1 };
   int three[] = { 1, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 1, 1 };
   const char* fill = "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";

   int i;

   vdp2_basic_tile_scroll_setup(tile_address);
   vdp2_line_color_screen_test_set_up_line_screen(line_table_address);

   int palette = 9;

   for (i = 0; i < 32; i++)
   {
      write_str_as_pattern_name_data(0, i, fill, palette, 0x000000, tile_address);
   }

   palette++;
   for (i = 7; i < 32; i++)
   {
      write_str_as_pattern_name_data(0, i, fill, palette, 0x004000, tile_address);
   }

   palette++;
   for (i = 14; i < 32; i++)
   {
      write_str_as_pattern_name_data(0, i, fill, palette, 0x008000, tile_address);
   }

   palette++;
   for (i = 21; i < 32; i++)
   {
      write_str_as_pattern_name_data(0, i, fill, palette, 0x00c000, tile_address);
   }

   for (i = 1; i < 24; i += 7)
   {
      write_large_font(1, i, zero, 0, 0x000000, tile_address);
      write_large_font(5, i, one, 0, 0x004000, tile_address);
      write_large_font(9, i, two, 0, 0x008000, tile_address);
      write_large_font(13, i, three, 0, 0x00C000, tile_address);
   }


   //set up instructions
   char * instructions[] = {
      "U/D/L/R: Reg menu     ",
      "A: Change Preset      ",
      "Start: Exit           "
   };

   int j = 0;
   for (i = 24; i < 24 + 7; i++)
   {
      write_str_as_pattern_name_data(18, i, instructions[j++], 3, 0x000000, tile_address);
   }

   //vars for reg adjuster
   struct {
      int nbg_priority[4];
      int nbg_color_calc_enable[4];
      int nbg_color_calc_ratio[4];
      int color_calculation_ratio_mode;
      int extended_color_calculation;
      int color_calculation_mode_bit;
      int color_ram_mode;
      int line_color_screen_inserts[4];
      int line_color_mode_bit;
      int line_color_screen_color_calc_enable;
      int line_color_screen_ratio;
   }v = { { 0 } };

   struct RegAdjusterState s = { 0 };

   ra_add_array(&s, (int(*)[])v.nbg_priority, 4,          "NBG Priority   NBG", 7);
   ra_add_array(&s, (int(*)[])v.nbg_color_calc_enable, 4, "Color cl enabl NBG", 1);
   ra_add_array(&s, (int(*)[])v.nbg_color_calc_ratio, 4,  "Color cl ratio NBG", 31);
   ra_add_var(&s, &v.extended_color_calculation,          "Extended color cal ", 1);
   ra_add_var(&s, &v.color_calculation_ratio_mode,        "Color cal ratio md ", 1);
   ra_add_var(&s, &v.color_calculation_mode_bit,          "Color cal mode bit ", 1);
   ra_add_var(&s, &v.color_ram_mode,                      "Color ram mode     ", 3);
   ra_add_array(&s, &v.line_color_screen_inserts, 4, "Line screen in NBG", 1);
   ra_add_var(&s, &v.line_color_mode_bit,                 "Line color mode bt ", 1);
   ra_add_var(&s, &v.line_color_screen_color_calc_enable, "Line col cal enabl ", 1);
   ra_add_var(&s, &v.line_color_screen_ratio,             "Line color cal rat ", 31);
   
   int presets[][24] =
   {
      //preset 0
      {
         //nbg priority
         1, 1, 1, 1,
         //nbg color calc enable
         1, 1, 1, 1,
         //nbg color calc ratio
         15, 0, 0, 0,
         //extended color calc
         1,
         //color calc ratio mode
         0,
         //color calc mode bit
         0,
         //color ram mode
         0,
         //Line screen inserts
         0, 0, 0, 0,
         //Line color mode bit
         0,
         //Line color calc enable
         0,
         //Line color ratio
         0
      },

      //preset 1
      {
         //nbg priority
         3, 2, 4, 0,
         //nbg color calc enable
         1, 1, 1, 1,
         //nbg color calc ratio
         15, 0, 0, 0,
         //extended color calc
         1,
         //color calc ratio mode
         0,
         //color calc mode bit
         1,
         //color ram mode
         0,
         //Line screen inserts
         0, 0, 0, 0,
         //Line color mode bit
         0,
         //Line color calc enable
         0,
         //Line color ratio
         0
      },

      //preset 2
      {
         //nbg priority
         3, 2, 7, 7,
         //nbg color calc enable
         1, 1, 1, 1,
         //nbg color calc ratio
         15, 0, 0, 0,
         //extended color calc
         1,
         //color calc ratio mode
         0,
         //color calc mode bit
         1,
         //color ram mode
         0,
         //Line screen inserts
         0, 0, 0, 0,
         //Line color mode bit
         0,
         //Line color calc enable
         0,
         //Line color ratio
         0
      },

      //preset 3
      {
         //nbg priority
         5, 7, 6, 0,
         //nbg color calc enable
         1, 0, 1, 1,
         //nbg color calc ratio
         15, 0, 0, 0,
         //extended color calc
         1,
         //color calc ratio mode
         0,
         //color calc mode bit
         1,
         //color ram mode
         0,
         //Line screen inserts
         0, 0, 0, 0,
         //Line color mode bit
         0,
         //Line color calc enable
         0,
         //Line color ratio
         0
      },

      //preset 4
      {
         //nbg priority
         7, 7, 0, 4,
         //nbg color calc enable
         1, 0, 1, 1,
         //nbg color calc ratio
         15, 0, 0, 0,
         //extended color calc
         1,
         //color calc ratio mode
         0,
         //color calc mode bit
         1,
         //color ram mode
         0,
         //Line screen inserts
         0, 0, 0, 0,
         //Line color mode bit
         0,
         //Line color calc enable
         0,
         //Line color ratio
         0
      },

      //preset 5
      {
         //nbg priority
         1, 1, 1, 1,
         //nbg color calc enable
         1, 1, 1, 1,
         //nbg color calc ratio
         0, 20, 31, 2,
         //extended color calc
         1,
         //color calc ratio mode
         1,
         //color calc mode bit
         0,
         //color ram mode
         0,
         //Line screen inserts
         0,0,0,0,
         //Line color mode bit
         0,
         //Line color calc enable
         0,
         //Line color ratio
         0
      }
   };

   int preset = 1;

   ra_do_preset(&s, presets[preset]);

   for (;;)
   {
      vdp_vsync();

      ra_update_vars(&s);

      ra_do_menu(&s, 17,0);

      VDP2_REG_CCCTL =
         (v.extended_color_calculation << 10) |
         (v.color_calculation_ratio_mode << 9) |
         (v.color_calculation_mode_bit << 8) |
         (v.nbg_color_calc_enable[0]) |
         (v.nbg_color_calc_enable[1] << 1) |
         (v.nbg_color_calc_enable[2] << 2) |
         (v.nbg_color_calc_enable[3] << 3) |
         (v.line_color_screen_color_calc_enable << 5);

      VDP2_REG_CCRNA = v.nbg_color_calc_ratio[0] | (v.nbg_color_calc_ratio[1] << 8);
      VDP2_REG_CCRNB = v.nbg_color_calc_ratio[2] | (v.nbg_color_calc_ratio[3] << 8);
      VDP2_REG_CCRLB = v.line_color_screen_ratio;

      VDP2_REG_LNCLEN = (v.line_color_screen_inserts[0]) |
         (v.line_color_screen_inserts[1] << 1) |
         (v.line_color_screen_inserts[2] << 2) |
         (v.line_color_screen_inserts[3] << 3);

      VDP2_REG_PRINA = v.nbg_priority[0] | (v.nbg_priority[1] << 8);
      VDP2_REG_PRINB = v.nbg_priority[2] | (v.nbg_priority[3] << 8);

      VDP2_REG_RAMCTL = v.color_ram_mode << 12;

      *(volatile u32 *)0x25F800A8 = (v.line_color_mode_bit << 31) | (line_table_address / 2);

      char preset_status[64] = { 0 };
      sprintf(preset_status, "Preset %d", preset);
      write_str_as_pattern_name_data(18, 27, preset_status, 3, 0x000000, tile_address);

      if (per[0].but_push_once & PAD_A)
      {
         preset++;

         if (preset > 5)
            preset = 0;

         ra_do_preset(&s, presets[preset]);
      }

      if (per[0].but_push_once & PAD_START)
         break;
   }
   vdp2_basic_tile_scroll_deinit();
}

//////////////////////////////////////////////////////////////////////////////

void vpd2_priority_shadow_draw_sprites(int start_x, int start_y, u32 vdp1_tile_address, int operation)
{
   int i, j;
   sprite_struct quad = { 0 };

   const int size = 8;

   quad.x = start_x * size;
   quad.y = start_y * size;

   int vdp2_priority = 0;
   int vdp2_color_calc = 0;
   int palette = 4;

   for (j = 0; j < 4; j++)
   {
      quad.x = start_x * size;

      for (i = 0; i < 4; i++)
      {
         quad.bank = (vdp2_priority << 12) | (vdp2_color_calc << 9) | (palette << 4);

         //use the "\n" tile
         quad.addr = vdp1_tile_address + (10 * 32);

         //msb on
         if (operation == 2)
         {
            quad.attr = (1 << 15);
         }

         quad.height = size;
         quad.width = size;

         vdp_draw_normal_sprite(&quad);

         quad.x += size;

         if (operation == 0 || operation == 1)
         {
            vdp2_priority++;
            vdp2_priority &= 7;
         }
         if (operation == 1)
         {
            vdp2_color_calc++;
            vdp2_color_calc &= 7;
         }

      }
      quad.y += size;
   }
}

//////////////////////////////////////////////////////////////////////////////

void write_tiles_4x(int x, int y, char * str, u32 vdp2_tile_address, u32 base)
{
   int i;
   for (i = 0; i < 4; i++)
   {
      write_str_as_pattern_name_data(x, y + i, str, 3, base, vdp2_tile_address);
   }
}

//////////////////////////////////////////////////////////////////////////////

void draw_normal_shadow_sprite(int x, int y, const char * str)
{
   sprite_struct quad = { 0 };

   int size = 32;

   int top_right_x = x * 8;
   int top_right_y = y * 8;
   quad.x = top_right_x + size;
   quad.y = top_right_y;
   quad.x2 = top_right_x + size;
   quad.y2 = top_right_y + size;
   quad.x3 = top_right_x;
   quad.y3 = top_right_y + size;
   quad.x4 = top_right_x;
   quad.y4 = top_right_y;
   quad.bank = 0x0ffe;

   vdp_draw_polygon(&quad);
}

//////////////////////////////////////////////////////////////////////////////

void vdp2_sprite_priority_shadow_test()
{
   const u32 vdp2_tile_address = 0x40000;
   const u32 vdp1_tile_address = 0x10000;
   vdp2_basic_tile_scroll_setup(vdp2_tile_address);

   load_font_8x8_to_vram_1bpp_to_4bpp(vdp1_tile_address, VDP1_RAM);

   VDP1_REG_PTMR = 0x02;//draw automatically with frame change

   int s0prin = 7;
   int s1prin = 6;

   VDP2_REG_PRISA = s0prin | (s1prin << 8);
   VDP2_REG_PRISB = 5 | (4 << 8);
   VDP2_REG_PRISC = 3 | (2 << 8);
   VDP2_REG_PRISD = 1 | (0 << 8);

   int nbg_priority[4] = { 0 };

   nbg_priority[0] = 7;
   nbg_priority[1] = 6;
   nbg_priority[2] = 5;
   nbg_priority[3] = 4;

   VDP2_REG_PRINA = nbg_priority[0] | (nbg_priority[1] << 8);
   VDP2_REG_PRINB = nbg_priority[2] | (nbg_priority[3] << 8);

   VDP2_REG_SDCTL = 0x9 | (1 << 8);

   int nbg_ratio[4] = { 0 };
   int framecount = 0;
   int ratio = 0;
   int ratio_dir = 1;

   int spccs = 2;
   int spccn = 5;

   write_str_as_pattern_name_data(0, 0, "Normal Shadow    ->", 3, 0x000000, vdp2_tile_address);
   write_str_as_pattern_name_data(0, 5, "MSB Transparent ", 3, 0x000000, vdp2_tile_address);
   write_str_as_pattern_name_data(0, 6, "          Shadow ->", 3, 0x000000, vdp2_tile_address);
   write_str_as_pattern_name_data(0, 10, "Sprite priority  ->", 3, 0x000000, vdp2_tile_address);
   write_str_as_pattern_name_data(0, 15, "MSB Sprite     ", 3, 0x000000, vdp2_tile_address);
   write_str_as_pattern_name_data(0, 16, "          Shadow ->", 3, 0x000000, vdp2_tile_address);
   write_str_as_pattern_name_data(0, 20, "Special Color    ", 3, 0x000000, vdp2_tile_address);
   write_str_as_pattern_name_data(0, 21, "     Calculation ->", 3, 0x000000, vdp2_tile_address);
   write_str_as_pattern_name_data(20, 25, "NBG0 NBG1 NBG2 NBG3", 3, 0x000000, vdp2_tile_address);

   int x;

   u32 addresses[4] = { 0x000000, 0x004000, 0x008000, 0x00c000 };

   char* str = "\n\n\n\n";

   for (x = 0; x < 4; x++)
   {
      int x_pos = 20 + (5 * x);
      write_tiles_4x(x_pos, 0, str, vdp2_tile_address, addresses[x]);//normal shadow tiles
      write_tiles_4x(x_pos, 5, str, vdp2_tile_address, addresses[x]);//msb shadow tiles
      write_tiles_4x(x_pos, 10, str, vdp2_tile_address, addresses[x]);//sprite priority tiles
      write_tiles_4x(x_pos, 15, str, vdp2_tile_address, addresses[x]);//msb shadow and cc tiles
      write_tiles_4x(x_pos, 20, str, vdp2_tile_address, addresses[x]);//special color calc tiles
   }

   for (;;)
   {
      vdp_vsync();

      VDP2_REG_SPCTL = (spccs << 12) | (spccn << 8) | (0 << 5) | 7;

      VDP2_REG_CCCTL = (1 << 6) | (1 << 0) | (1 << 3);

      VDP2_REG_CCRSA = (u16)(nbg_ratio[0] | (nbg_ratio[1] << 8));
      VDP2_REG_CCRSB = (u16)(nbg_ratio[2] | (nbg_ratio[3] << 8));
      VDP2_REG_CCRSC = (u16)(nbg_ratio[0] | (nbg_ratio[1] << 8));
      VDP2_REG_CCRSD = (u16)(nbg_ratio[2] | (nbg_ratio[3] << 8));

      vdp_start_draw_list();
      sprite_struct quad = { 0 };

      //system clipping
      quad.x = 320;
      quad.y = 224;

      vdp_system_clipping(&quad);

      //user clipping
      quad.x = 0;
      quad.y = 0;
      quad.x2 = 320;
      quad.y2 = 224;

      vdp_user_clipping(&quad);

      quad.x = 0;
      quad.y = 0;

      vdp_local_coordinate(&quad);

      for (x = 0; x < 4; x++)
      {
         int x_pos = 20 + (5 * x);
         //normal shadow
         draw_normal_shadow_sprite(x_pos, 0, str);
         //msb shadow
         vpd2_priority_shadow_draw_sprites(x_pos, 5, vdp1_tile_address, 2);
         //sprite priority
         vpd2_priority_shadow_draw_sprites(x_pos, 10, vdp1_tile_address, 0);
         //msb shadow and color calculation
         vpd2_priority_shadow_draw_sprites(x_pos, 15, vdp1_tile_address, 0);
         vpd2_priority_shadow_draw_sprites(x_pos, 15, vdp1_tile_address, 2);
         //special color calc
         vpd2_priority_shadow_draw_sprites(x_pos, 20, vdp1_tile_address, 1);
      }

      vdp_end_draw_list();

      char status[64] = "";

      sprintf(status, "S0PRIN=%02x S1PRIN=%02x", s0prin, s1prin);
      write_str_as_pattern_name_data(0, 25, status, 3, 0x000000, vdp2_tile_address);
      sprintf(status, "SOCCRT=%02x S1CCRT=%02x", nbg_ratio[0], nbg_ratio[1]);
      write_str_as_pattern_name_data(0, 26, status, 3, 0x000000, vdp2_tile_address);
      sprintf(status, "SPCCS =%02x SPCCN =%02x (Press A,B) ", spccs, spccn);
      write_str_as_pattern_name_data(0, 27, status, 3, 0x000000, vdp2_tile_address);

      do_color_ratios(&framecount, &ratio, &ratio_dir);

      nbg_ratio[0] = ((-ratio) & 0x1f);
      nbg_ratio[2] = nbg_ratio[0];
      nbg_ratio[3] = nbg_ratio[1] = ratio;

      if (per[0].but_push_once & PAD_A)
      {
         spccs++;
         spccs &= 3;
      }

      if (per[0].but_push_once & PAD_B)
      {
         spccn++;
         spccn &= 7;
      }

      if (per[0].but_push_once & PAD_START)
         break;
   }

   vdp2_basic_tile_scroll_deinit();
}

//////////////////////////////////////////////////////////////////////////////

void vdp2_change_4bbp_tile_color(u32 address, int amount)
{
   int i;

   for (i = 0; i < font_8x8_size; i++)
   {
      volatile u8 *dest = (volatile u8 *)(VDP2_RAM + address);

      u8 value = dest[i];
      u8 pix1 = (value >> 4) & 0xf;
      u8 pix2 = value & 0xf;

      if (pix1 != 0)
         pix1 += amount;
      if (pix2 != 0)
         pix2 += amount;

      dest[i] = (pix1 << 4) | pix2;
   }
}

//////////////////////////////////////////////////////////////////////////////

void vdp2_special_priority_test()
{
   const u32 vdp2_tile_address = 0x40000;
   const u32 vdp2_tile_address_alt = 0x42000;
   vdp2_basic_tile_scroll_setup(vdp2_tile_address);

   load_font_8x8_to_vram_1bpp_to_4bpp(vdp2_tile_address_alt, VDP2_RAM);

   //alter the colors of the second set of tiles
   vdp2_change_4bbp_tile_color(vdp2_tile_address_alt, 3);

   u32 addresses[4] = { 0x000000, 0x004000, 0x008000, 0x00c000 };

   char* str = "\n";
   int y = 0;
   int x = 0;

   for (y = 0; y < 4; y++)
   {
      for (x = 0; x < 4; x++)
      {
         int checker_pattern = (x^y) & 1;

         //nbg1 tiles over nbg0 tiles (move layer underneath to top)
         write_str_as_pattern_name_data_special(x, y, str, 3, addresses[0], vdp2_tile_address, 0, 0);//priority 6
         write_str_as_pattern_name_data_special(x, y, str, 4, addresses[1], vdp2_tile_address, checker_pattern, 0);//priority 6, 7 when checker=1

         //nbg2 tiles under nbg3 tiles (move layer on top underneath)
         write_str_as_pattern_name_data_special(4 + x, y, str, 5, addresses[2], vdp2_tile_address, checker_pattern, 0);//priority 7, 6 when checker=0
         write_str_as_pattern_name_data_special(4 + x, y, str, 6, addresses[3], vdp2_tile_address, 1, 0);//priority 7

         //altered tiles to to test different special function code
         write_str_as_pattern_name_data_special(8 + x, y, str, 3, addresses[0], vdp2_tile_address_alt, 0, 0);
         write_str_as_pattern_name_data_special(8 + x, y, str, 4, addresses[1], vdp2_tile_address_alt, checker_pattern, 0);

         write_str_as_pattern_name_data_special(12 + x, y, str, 5, addresses[2], vdp2_tile_address_alt, checker_pattern, 0);
         write_str_as_pattern_name_data_special(12 + x, y, str, 6, addresses[3], vdp2_tile_address_alt, 1, 0);

         //top layer color calculates
         write_str_as_pattern_name_data_special(x, y + 5, str, 3, addresses[0], vdp2_tile_address, 0, checker_pattern);
         write_str_as_pattern_name_data_special(x, y + 5, str, 4, addresses[1], vdp2_tile_address, 0, 1);

         write_str_as_pattern_name_data_special(4 + x, y + 5, str, 5, addresses[2], vdp2_tile_address, 0, 0);
         write_str_as_pattern_name_data_special(4 + x, y + 5, str, 6, addresses[3], vdp2_tile_address, 0, checker_pattern);

         write_str_as_pattern_name_data_special(8 + x, y + 5, str, 3, addresses[0], vdp2_tile_address_alt, 0, 1);
         write_str_as_pattern_name_data_special(8 + x, y + 5, str, 4, addresses[1], vdp2_tile_address_alt, 0, checker_pattern);

         write_str_as_pattern_name_data_special(12 + x, y + 5, str, 5, addresses[2], vdp2_tile_address_alt, 0, checker_pattern);
         write_str_as_pattern_name_data_special(12 + x, y + 5, str, 6, addresses[3], vdp2_tile_address_alt, 0, 1);
      }
   }

   write_str_as_pattern_name_data_special(0, 10, "NBG0", 3, addresses[0], vdp2_tile_address, 0, 0);
   write_str_as_pattern_name_data_special(0, 11, "NBG1", 4, addresses[1], vdp2_tile_address, 0, 0);
   write_str_as_pattern_name_data_special(4, 10, "NBG2", 5, addresses[2], vdp2_tile_address, 0, 0);
   write_str_as_pattern_name_data_special(4, 11, "NBG3", 6, addresses[3], vdp2_tile_address, 0, 0);

   write_str_as_pattern_name_data_special(8, 10, "NBG0", 3, addresses[0], vdp2_tile_address_alt, 0, 0);
   write_str_as_pattern_name_data_special(8, 11, "NBG1", 4, addresses[1], vdp2_tile_address_alt, 0, 0);
   write_str_as_pattern_name_data_special(12, 10, "NBG2", 5, addresses[2], vdp2_tile_address_alt, 0, 0);
   write_str_as_pattern_name_data_special(12, 11, "NBG3", 6, addresses[3], vdp2_tile_address_alt, 0, 0);

   int preset = 0;

   int framecount = 0;
   int ratio = 0;
   int ratio_dir = 1;

   int nbg_ratio[4] = { 0 };
   

   //vars for reg adjuster
   struct {
      int special_color_calc_mode[4];
      int nbg_color_calc_enable[4];
      int color_calculation_ratio_mode;//select per top
      int special_priority_mode_bit[4];
      int special_function_code_select[4];
      int special_function_code_bit[8];
      int color_calculation_mode_bit;
      int nbg_priority[4];
   }v = { { 0 } };

   struct RegAdjusterState s = { 0 };

   ra_add_array(&s, (int(*)[])v.special_color_calc_mode, 4, "Spcl clr cl md NBG", 3);
   ra_add_array(&s, (int(*)[])v.nbg_color_calc_enable, 4, "Color calc enb NBG", 1);
   ra_add_array(&s, (int(*)[])v.special_function_code_bit, 4, "Specl functn cde #", 1);
   ra_add_array(&s, (int(*)[])v.special_priority_mode_bit, 4, "Special priort NBG", 3);
   ra_add_array(&s, (int(*)[])v.special_function_code_select, 4, "Specl func cod NBG", 1);
   ra_add_var(&s, &v.color_calculation_ratio_mode, "Color cal rati mode", 1);
   ra_add_var(&s, &v.color_calculation_mode_bit, "Color calcultn mode", 1);
   ra_add_array(&s, (int(*)[])v.nbg_priority, 4, "Priority       NBG", 7);

   int init_state[] =
   {//special color calc mode
      0, 0, 0, 0,
      //nbg color calc enable
      0, 0, 0, 0,
      //special function code bit
      0, 0, 0, 0,
      //special priority mode bit
      0, 1, 1, 0,
      //special function code select
      0, 0, 0, 0,
      //color calculation ratio mode
      0,
      //color calculation mode bit
      0,
      //nbg priority
      6,6,7,7
   };

   char* preset_strings[] = {
      "Preset 0 : Per-tile priority            ",
      "Preset 1 : Per-pixel priority           ",
      "Preset 2 : Color calc per tile          ",
      "Preset 3 : Color calc per pixel         ",
      "Preset 4 : Per-tile priority 0 to 1     "
   };

   //set up instructions
   char * instructions[] = {
      "A:     Do preset  ",
      "Up:    Move up    ",
      "Down:  Move down  ",
      "Right: Decrease   ",
      "Left:  Increase   ",
      "Start: Exit       "
   };

   int i; 
   for (i = 0; i < 6; i++)
   {
      write_str_as_pattern_name_data(0, 17 + i, instructions[i], 3, 0x000000, vdp2_tile_address);
   }

   ra_do_preset(&s, init_state);

   //display the dot data bits
   volatile u32 *vram_ptr = (volatile u32 *)(VDP2_RAM + vdp2_tile_address);
   int pos = 8;
   int unchanged_data = vram_ptr[pos];

   vram_ptr = (volatile u32 *)(VDP2_RAM + vdp2_tile_address_alt);
   int changed_data = vram_ptr[pos];
   char output[64] = { 0 };
   sprintf(output, "0x%08x", unchanged_data);
   write_str_as_pattern_name_data(0, 24, output, 3, 0x000000, vdp2_tile_address);
   sprintf(output, "0x%08x", changed_data);
   write_str_as_pattern_name_data(0, 25, output, 3, 0x000000, vdp2_tile_address);

   for (;;)
   {
      vdp_vsync();

      ra_update_vars(&s);

      ra_do_menu(&s, 17,0);

      do_color_ratios(&framecount, &ratio, &ratio_dir);

      nbg_ratio[0] = ((-ratio) & 0x1f);
      nbg_ratio[2] = nbg_ratio[0];
      nbg_ratio[3] = nbg_ratio[1] = ratio;

      VDP2_REG_SFPRMD = v.special_priority_mode_bit[0] |
         (v.special_priority_mode_bit[1] << 2) |
         (v.special_priority_mode_bit[2] << 4) |
         (v.special_priority_mode_bit[3] << 6);

      VDP2_REG_PRINA = v.nbg_priority[0] | (v.nbg_priority[1] << 8);
      VDP2_REG_PRINB = v.nbg_priority[2] | (v.nbg_priority[3] << 8);

      VDP2_REG_SFSEL = v.special_function_code_select[0] |
         (v.special_function_code_select[1] << 1) |
         (v.special_function_code_select[2] << 2) |
         (v.special_function_code_select[3] << 3);

      VDP2_REG_SFCODE =
         (v.special_function_code_bit[0] << 0) |
         (v.special_function_code_bit[1] << 1) |
         (v.special_function_code_bit[2] << 2) |
         (v.special_function_code_bit[3] << 3) |
         (v.special_function_code_bit[4] << 4) |
         (v.special_function_code_bit[5] << 5) |
         (v.special_function_code_bit[6] << 6) |
         (v.special_function_code_bit[7] << 7);

      VDP2_REG_CCRNA = (u16)(nbg_ratio[0] | (nbg_ratio[1] << 8));
      VDP2_REG_CCRNB = (u16)(nbg_ratio[2] | (nbg_ratio[3] << 8));

      VDP2_REG_CCCTL =
         (v.color_calculation_ratio_mode << 9) |
         (v.color_calculation_mode_bit << 8) |
         (v.nbg_color_calc_enable[0]) |
         (v.nbg_color_calc_enable[1] << 1) |
         (v.nbg_color_calc_enable[2] << 2) |
         (v.nbg_color_calc_enable[3] << 3);

      VDP2_REG_SFCCMD =
         v.special_color_calc_mode[0] |
         (v.special_color_calc_mode[1] << 2) |
         (v.special_color_calc_mode[2] << 4) |
         (v.special_color_calc_mode[3] << 6);

      char ratio_status[64] = { 0 };

      write_str_as_pattern_name_data(0, 13, "Ratios", 3, 0x000000, vdp2_tile_address);

      sprintf(ratio_status, "NBG0=%02x NBG1=%02x", nbg_ratio[0], nbg_ratio[1]);
      write_str_as_pattern_name_data(0, 14, ratio_status, 3, 0x000000, vdp2_tile_address);

      sprintf(ratio_status, "NBG2=%02x NBG3=%02x", nbg_ratio[2], nbg_ratio[3]);
      write_str_as_pattern_name_data(0, 15, ratio_status, 3, 0x000000, vdp2_tile_address);

      if (preset == 1)
      {
         //blink the two patterns
         if (framecount % 30 == 0)
         {
            //special_function_code_bit[0]
            s.vars[8].value = !s.vars[8].value;
         }
         if (framecount % 60 == 0)
         {
            //special_function_code_bit[2]
            s.vars[10].value = !s.vars[10].value;
         }
      }
      if (preset == 3)
      {
         if (framecount % 30 == 0)
         {
            //special_function_code_bit[0]
            s.vars[8].value = !s.vars[8].value;
         }
      }

      write_str_as_pattern_name_data(0, 27, preset_strings[preset], 3, 0x000000, vdp2_tile_address);

      if (per[0].but_push_once & PAD_A)
      {
         preset++;

         if (preset > 4)
            preset = 0;

         if (preset == 0)
         {
            ra_do_preset(&s, init_state);
         }
         else if (preset == 1)
         {
            int vars[] =
            {//special color calc mode
               0, 0, 0, 0,
               //nbg color calc enable
               0, 0, 0, 0,
               //special function code bit
               1, 0, 0, 0, 
               //special priority mode bit
               3, 2, 2, 0,
               //special function code select
               0, 0, 0, 0,
               //color calculation ratio mode
               0,
               //color calculation mode bit
               0,
               //nbg priority
               6, 6, 7, 7
            };

            ra_do_preset(&s, vars);
         }
         else if (preset == 2)
         {
            int vars[] =
            {//special color calc mode
               1, 1, 1, 1,
               //nbg color calc enable
               1, 1, 1, 1,
               //special function code bit
               0, 0, 0, 0,
               //special priority mode bit
               0, 1, 1, 0,
               //special function code select
               0, 0, 0, 0,
               //color calculation ratio mode
               0,
               //color calculation mode bit
               0,
               //nbg priority
               6, 6, 7, 7
            };

            ra_do_preset(&s, vars);
         }
         else if (preset == 3)
         {
            int vars[] =
            {//special color calc mode
               2, 2, 2, 2,
               //nbg color calc enable
               1, 1, 1, 1,
               //special function code bit
               1, 0, 0, 0,
               //special priority mode bit
               0, 1, 1, 0,
               //special function code select
               0, 0, 0, 0,
               //color calculation ratio mode
               0,
               //color calculation mode bit
               0,
               //nbg priority
               6, 6, 7, 7
            };

            ra_do_preset(&s, vars);
         }
         else if (preset == 4)
         {
            int init_state[] =
            {//special color calc mode
               0, 0, 0, 0,
               //nbg color calc enable
               0, 0, 0, 0,
               //special function code bit
               0, 0, 0, 0,
               //special priority mode bit
               0, 1, 1, 0,
               //special function code select
               0, 0, 0, 0,
               //color calculation ratio mode
               0,
               //color calculation mode bit
               0,
               //nbg priority
               1, 1, 0, 0
            };
            ra_do_preset(&s, init_state);
         }
      }

      if (per[0].but_push_once & PAD_START)
         break;
   }

   vdp2_basic_tile_scroll_deinit();
}

void vdp2_set_line_window_tables(u32 * line_window_table_address)
{
   u16 ellipse[] = {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 290, 348, 268, 370, 253, 385,
      242, 396, 231, 407, 222, 416, 214, 424, 207, 431, 200, 438, 193, 445, 187, 451, 181, 457, 175, 463,
      170, 468, 165, 473, 160, 478, 156, 482, 151, 487, 147, 491, 143, 495, 139, 499, 135, 503, 131, 507,
      128, 510, 124, 514, 121, 517, 117, 521, 114, 524, 111, 527, 108, 530, 105, 533, 102, 536, 99, 539,
      97, 541, 94, 544, 92, 546, 89, 549, 87, 551, 84, 554, 82, 556, 80, 558, 77, 561, 75, 563,
      73, 565, 71, 567, 69, 569, 67, 571, 65, 573, 63, 575, 61, 577, 60, 578, 58, 580, 56, 582,
      55, 583, 53, 585, 51, 587, 50, 588, 48, 590, 47, 591, 46, 592, 44, 594, 43, 595, 42, 596,
      40, 598, 39, 599, 38, 600, 37, 601, 36, 602, 35, 603, 34, 604, 33, 605, 32, 606, 31, 607,
      30, 608, 29, 609, 28, 610, 27, 611, 26, 612, 26, 612, 25, 613, 24, 614, 24, 614, 23, 615,
      22, 616, 22, 616, 21, 617, 21, 617, 20, 618, 20, 618, 19, 619, 19, 619, 18, 620, 18, 620,
      18, 620, 17, 621, 17, 621, 17, 621, 17, 621, 17, 621, 16, 622, 16, 622, 16, 622, 16, 622,
      16, 622, 16, 622, 16, 622, 16, 622, 16, 622, 16, 622, 16, 622, 17, 621, 17, 621, 17, 621,
      17, 621, 17, 621, 18, 620, 18, 620, 18, 620, 19, 619, 19, 619, 20, 618, 20, 618, 21, 617,
      21, 617, 22, 616, 22, 616, 23, 615, 24, 614, 24, 614, 25, 613, 26, 612, 26, 612, 27, 611,
      28, 610, 29, 609, 30, 608, 31, 607, 32, 606, 33, 605, 34, 604, 35, 603, 36, 602, 37, 601,
      38, 600, 39, 599, 40, 598, 42, 596, 43, 595, 44, 594, 46, 592, 47, 591, 48, 590, 50, 588,
      51, 587, 53, 585, 55, 583, 56, 582, 58, 580, 60, 578, 61, 577, 63, 575, 65, 573, 67, 571,
      69, 569, 71, 567, 73, 565, 75, 563, 77, 561, 80, 558, 82, 556, 84, 554, 87, 551, 89, 549,
      92, 546, 94, 544, 97, 541, 99, 539, 102, 536, 105, 533, 108, 530, 111, 527, 114, 524, 117, 521,
      121, 517, 124, 514, 128, 510, 131, 507, 135, 503, 139, 499, 143, 495, 147, 491, 151, 487, 156, 482,
      160, 478, 165, 473, 170, 468, 175, 463, 181, 457, 187, 451, 193, 445, 200, 438, 207, 431, 214, 424,
      222, 416, 231, 407, 242, 396, 253, 385, 268, 370, 290, 348, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
   };

   u16 ellipse2[] = {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 290, 348, 269, 369, 254, 384, 243, 395, 233, 405,
      224, 414, 216, 422, 208, 430, 201, 437, 195, 443, 189, 449, 183, 455, 178, 460, 173, 465, 168, 470,
      163, 475, 159, 479, 154, 484, 150, 488, 146, 492, 142, 496, 139, 499, 135, 503, 131, 507, 128, 510,
      125, 513, 122, 516, 118, 520, 115, 523, 113, 525, 110, 528, 107, 531, 104, 534, 102, 536, 99, 539,
      97, 541, 94, 544, 92, 546, 90, 548, 87, 551, 85, 553, 83, 555, 81, 557, 79, 559, 77, 561,
      75, 563, 74, 564, 72, 566, 70, 568, 68, 570, 67, 571, 65, 573, 64, 574, 62, 576, 61, 577,
      59, 579, 58, 580, 57, 581, 55, 583, 54, 584, 53, 585, 52, 586, 51, 587, 49, 589, 48, 590,
      47, 591, 46, 592, 45, 593, 44, 594, 44, 594, 43, 595, 42, 596, 41, 597, 40, 598, 40, 598,
      39, 599, 38, 600, 38, 600, 37, 601, 37, 601, 36, 602, 36, 602, 35, 603, 35, 603, 34, 604,
      34, 604, 34, 604, 33, 605, 33, 605, 33, 605, 33, 605, 32, 606, 32, 606, 32, 606, 32, 606,
      32, 606, 32, 606, 32, 606, 32, 606, 32, 606, 32, 606, 32, 606, 33, 605, 33, 605, 33, 605,
      33, 605, 34, 604, 34, 604, 34, 604, 35, 603, 35, 603, 36, 602, 36, 602, 37, 601, 37, 601,
      38, 600, 38, 600, 39, 599, 40, 598, 40, 598, 41, 597, 42, 596, 43, 595, 44, 594, 44, 594,
      45, 593, 46, 592, 47, 591, 48, 590, 49, 589, 51, 587, 52, 586, 53, 585, 54, 584, 55, 583,
      57, 581, 58, 580, 59, 579, 61, 577, 62, 576, 64, 574, 65, 573, 67, 571, 68, 570, 70, 568,
      72, 566, 74, 564, 75, 563, 77, 561, 79, 559, 81, 557, 83, 555, 85, 553, 87, 551, 90, 548,
      92, 546, 94, 544, 97, 541, 99, 539, 102, 536, 104, 534, 107, 531, 110, 528, 113, 525, 115, 523,
      118, 520, 122, 516, 125, 513, 128, 510, 131, 507, 135, 503, 139, 499, 142, 496, 146, 492, 150, 488,
      154, 484, 159, 479, 163, 475, 168, 470, 173, 465, 178, 460, 183, 455, 189, 449, 195, 443, 201, 437,
      208, 430, 216, 422, 224, 414, 233, 405, 243, 395, 254, 384, 269, 369, 290, 348, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
   };

   volatile u16 *line_window_table_table_ptr = (volatile u16 *)(VDP2_RAM + line_window_table_address[0]);
   volatile u16 *line_window_table_table_ptr2 = (volatile u16 *)(VDP2_RAM + line_window_table_address[1]);

   int i;
   for (i = 0; i < 224 * 2; i++)
   {
      line_window_table_table_ptr[i] = ellipse[i];
      line_window_table_table_ptr2[i] = ellipse2[i];
   }
}

void vdp2_line_window_test()
{
   const u32 vdp2_tile_address = 0x40000;
   vdp2_basic_tile_scroll_setup(vdp2_tile_address);

   int i;
   for (i = 0; i < 32; i += 4)
   {
      write_str_as_pattern_name_data_special(0, 0 + i, "\n\n\n\nNBG0                        NBG0\n\n\n\n", 3, 0, vdp2_tile_address, 0, 0);
      write_str_as_pattern_name_data_special(0, 1 + i, "\n\n\n\nNBG1                        NBG1\n\n\n\n", 4, 0x004000, vdp2_tile_address, 0, 0);
      write_str_as_pattern_name_data_special(0, 2 + i, "\n\n\n\nNBG2                        NBG2\n\n\n\n", 5, 0x008000, vdp2_tile_address, 0, 0);
      write_str_as_pattern_name_data_special(0, 3 + i, "\n\n\n\nNBG3                        NBG3\n\n\n\n", 6, 0x00C000, vdp2_tile_address, 0, 0);
   }

   write_str_as_pattern_name_data_special(0, 0,  "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n", 3, 0x000000, vdp2_tile_address, 0, 0);
   write_str_as_pattern_name_data_special(0, 1, "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n", 4, 0x004000, vdp2_tile_address, 0, 0);
   write_str_as_pattern_name_data_special(0, 2, "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n", 5, 0x008000, vdp2_tile_address, 0, 0);

   write_str_as_pattern_name_data_special(0, 25, "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n", 4, 0x004000, vdp2_tile_address, 0, 0);
   write_str_as_pattern_name_data_special(0, 26, "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n", 5, 0x008000, vdp2_tile_address, 0, 0);
   write_str_as_pattern_name_data_special(0, 27, "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n", 6, 0x00C000, vdp2_tile_address, 0, 0);

   //vars for reg adjuster
   struct {
      int line_window_enable[2];
      struct {
         int window_logic;
         int window_enable[2];
         int window_area[2];
      }nbg[4];
   }v = { { 0 } };

   struct RegAdjusterState s = { 0 };

   ra_add_array(&s, (int(*)[])v.line_window_enable, 2, "Line window enab #", 1);

   for (i = 0; i < 4; i++)
   { 
      char str[64] = { 0 };
      sprintf(str, "NBG%d window logic  ", i);
      ra_add_var(&s, &v.nbg[i].window_logic,              str, 1);
      sprintf(str, "NBG%d window enabl ", i);
      ra_add_array(&s, (int(*)[])v.nbg[i].window_enable, 2, str, 1);
      sprintf(str, "NBG%d window area  ", i);
      ra_add_array(&s, (int(*)[])v.nbg[i].window_area, 2, str, 1);
   }

   int presets[][22] =
   {
      //preset 0
      {
         //line window enable
         1, 1,
         //nbg0
         0, 0, 0, 0, 0,
         //nbg1
         0, 0, 1, 1, 1,
         //nbg2
         0, 1, 1, 1, 0,
         //nbg3
         1, 1, 1, 0, 1
      },
      //preset 1
      {
         //line window enable
         0,0,
         //nbg0
         0, 0, 0, 0, 0,
         //nbg1
         0, 0, 1, 1, 1,
         //nbg2
         0, 1, 1, 1, 0,
         //nbg3
         1, 1, 1, 0, 1
      },
      //preset 1
      {
         //line window enable
         0, 0,
         //nbg0
         0, 0, 0, 0, 0,
         //nbg1
         0, 0, 0, 0, 0,
         //nbg2
         0, 0, 0, 0, 0,
         //nbg3
         0, 0, 0, 0, 0
      }
   };

   int preset = 0;

   ra_do_preset(&s, presets[preset]);

   u32 line_window_table_address[2] = { 0x50000, 0x52000};

   vdp2_set_line_window_tables(line_window_table_address);
   
   for (;;)
   {
      vdp_vsync();

      *(volatile u32 *)0x25F800D8 = (v.line_window_enable[0] << 31) | (line_window_table_address[0] / 2);
      *(volatile u32 *)0x25F800DC = (v.line_window_enable[1] << 31) | (line_window_table_address[1] / 2);

      VDP2_REG_WCTLA = (v.nbg[0].window_enable[0] << 1) | (v.nbg[1].window_enable[0] << 9) |
         (v.nbg[0].window_logic << 7) | (v.nbg[1].window_logic << 15) |
         (v.nbg[0].window_area[0] << 0) | (v.nbg[1].window_area[0] << 8) |
         (v.nbg[0].window_enable[1] << 3) | (v.nbg[1].window_enable[1] << 11) |
         (v.nbg[0].window_area[1] << 2) | (v.nbg[1].window_area[1] << 10);

      VDP2_REG_WCTLB = (v.nbg[2].window_enable[0] << 1) | (v.nbg[3].window_enable[0] << 9) |
         (v.nbg[2].window_logic << 7) | (v.nbg[3].window_logic << 15) |
         (v.nbg[2].window_area[0] << 0) | (v.nbg[3].window_area[0] << 8) |
         (v.nbg[2].window_enable[1] << 3) | (v.nbg[3].window_enable[1] << 11) |
         (v.nbg[2].window_area[1] << 2) | (v.nbg[3].window_area[1] << 10);

      VDP2_REG_WPSX0 = (1 * 8) * 2;
      VDP2_REG_WPSY0 = 1 * 8;
      VDP2_REG_WPEX0 = ((39 * 8) * 2) - 2;
      VDP2_REG_WPEY0 = (27 * 8) - 1;

      VDP2_REG_WPSX1 = (2 * 8) * 2;
      VDP2_REG_WPSY1 = 2 * 8;
      VDP2_REG_WPEX1 = ((38 * 8) * 2) - 2;
      VDP2_REG_WPEY1 = (26 * 8) - 1;

      ra_update_vars(&s);

      ra_do_menu(&s, 8,3);

      if (per[0].but_push_once & PAD_A)
      {
         preset++;

         if (preset > 2)
            preset = 0;

         ra_do_preset(&s, presets[preset]);
      }

      if (per[0].but_push_once & PAD_START)
      {
         break;
      }
   }
   vdp2_basic_tile_scroll_deinit();
}

//////////////////////////////////////////////////////////////////////////////

void vdp2_nbg0_test ()
{
   screen_settings_struct settings;

   // Draw a box on our default screen
   draw_box(120, 180, 80, 40, 15);

   // Setup NBG0 for drawing
   settings.is_bitmap = TRUE;
   settings.bitmap_size = BG_BITMAP512x256;
   settings.transparent_bit = 0;
   settings.color = BG_256COLOR;
   settings.special_priority = 0;
   settings.special_color_calc = 0;
   settings.extra_palette_num = 0;
//   settings.map_offset = 0;
//   settings.parameteraddr = 0x25E60000;
   vdp_nbg0_init(&settings);

   // Draw some stuff on the screen

   working_query("Is the above graphics displayed?");

   // Disable NBG0
   vdp_nbg0_deinit();
}

//////////////////////////////////////////////////////////////////////////////

void vdp2_nbg1_test ()
{
}

//////////////////////////////////////////////////////////////////////////////

void vdp2_nbg2_test ()
{
}

//////////////////////////////////////////////////////////////////////////////

void vdp2_nbg3_test ()
{
}

//////////////////////////////////////////////////////////////////////////////

void vdp2_rbg0_test ()
{
   // Draw a box on our default screen
   draw_box(120, 180, 80, 40, 15);

   // Draw some graphics on the RBG0 layer

   working_query("Is the above graphics displayed?");
}

//////////////////////////////////////////////////////////////////////////////

void vdp2_rbg1_test ()
{
}

//////////////////////////////////////////////////////////////////////////////

void vdp2_resolution_test ()
{
/*
   vdp_printf(&test_disp_font, 0 * 8, 2 * 8, 0xF, "vidmode = %02X", vidmode);

   // Display Main Menu
   for(;;)
   {
      vdp_vsync();

      if (per[0].but_push_once & PAD_A)
      {
         if ((vidmode & 0x7) == 7)
            vidmode &= 0xF0;
         else
            vidmode++;
         vdp_init(vidmode);
         vdp_rbg0_init(&testdispsettings);
         vdp_set_default_palette();
         vdp_set_font(SCREEN_RBG0, &test_disp_font, 1);
         vdp_disp_on();
         vdp_printf(&test_disp_font, 0 * 8, 2 * 8, 0xF, "vidmode =   ", vidmode);
         vdp_printf(&test_disp_font, 0 * 8, 2 * 8, 0xF, "vidmode = %02X", vidmode);
      }
      else if (per[0].but_push_once & PAD_B)
      {
         if ((vidmode & 0x30) == 0x30)
            vidmode &= 0xCF;
         else
            vidmode += 0x10;
         vdp_init(vidmode);
         vdp_rbg0_init(&testdispsettings);
         vdp_set_default_palette();
         vdp_set_font(SCREEN_RBG0, &test_disp_font, 1);
         vdp_disp_on();
         vdp_printf(&test_disp_font, 0 * 8, 2 * 8, 0xF, "vidmode =   ", vidmode);
         vdp_printf(&test_disp_font, 0 * 8, 2 * 8, 0xF, "vidmode = %02X", vidmode);
      }
      else if (per[0].but_push_once & PAD_C)
      {
         if ((vidmode & 0xC0) == 0xC0)
            vidmode &= 0x3F;
         else
            vidmode += 0x40;
         vdp_init(vidmode);
         vdp_rbg0_init(&testdispsettings);
         vdp_set_default_palette();
         vdp_set_font(SCREEN_RBG0, &test_disp_font, 1);
         vdp_disp_on();
         vdp_printf(&test_disp_font, 0 * 8, 2 * 8, 0xF, "vidmode =   ", vidmode);
         vdp_printf(&test_disp_font, 0 * 8, 2 * 8, 0xF, "vidmode = %02X", vidmode);
      }
   }
*/
}

//////////////////////////////////////////////////////////////////////////////

void vdp2_window_test ()
{
   screen_settings_struct settings;
   int dir=-1;
   int counter=320-1, counter2=224-1;
   int i;
   int nbg0_wnd;
   int nbg1_wnd;

   // Draw a box on our default screen
//   DrawBox(120, 180, 80, 40, 15);

   // Setup NBG0 for drawing
   settings.is_bitmap = TRUE;
   settings.bitmap_size = BG_BITMAP512x256;
   settings.transparent_bit = 0;
   settings.color = BG_256COLOR;
   settings.special_priority = 0;
   settings.special_color_calc = 0;
   settings.extra_palette_num = 0;
   settings.map_offset = (0x20000 >> 17);
//   settings.parameteraddr = 0x25E60000;
   vdp_nbg0_init(&settings);

   // Setup NBG1 for drawing
   settings.is_bitmap = TRUE;
   settings.bitmap_size = BG_BITMAP512x256;
   settings.transparent_bit = 0;
   settings.color = BG_256COLOR;
   settings.special_priority = 0;
   settings.special_color_calc = 0;
   settings.extra_palette_num = 0;
   settings.map_offset = (0x40000 >> 17);
//   settings.parameteraddr = 0x25E60000;
   vdp_nbg1_init(&settings);

   // Draw some stuff on the screen

   vdp_set_font(SCREEN_NBG0, &test_disp_font, 1);
   test_disp_font.out = (u8 *)0x25E20000;
   for (i = 5; i < 24; i+=2)
      vdp_printf(&test_disp_font, 0 * 8, i * 8, 0xB, "NBG0 NBG0 NBG0 NBG0 NBG0 NBG0 NBG0");
   vdp_set_font(SCREEN_NBG1, &test_disp_font, 1);
   test_disp_font.out = (u8 *)0x25E40000;
   for (i = 6; i < 24; i+=2)
      vdp_printf(&test_disp_font, 0 * 8, i * 8, 0xC, "NBG1 NBG1 NBG1 NBG1 NBG1 NBG1 NBG1");
   vdp_set_font(SCREEN_RBG0, &test_disp_font, 0);
   test_disp_font.out = (u8 *)0x25E00000;

   vdp_set_priority(SCREEN_NBG0, 2);
   vdp_set_priority(SCREEN_NBG1, 3);

   VDP2_REG_WPSX0 = 0;
   VDP2_REG_WPSY0 = 0;
   VDP2_REG_WPEX0 = counter << 1;
   VDP2_REG_WPEY0 = counter2;
   VDP2_REG_WPSX1 = ((320 - 40) / 2) << 1;
   VDP2_REG_WPSY1 = (224 - 40) / 2;
   VDP2_REG_WPEX1 = ((320 + 40) / 2) << 1;
   VDP2_REG_WPEY1 = (224 + 40) / 2;
   nbg0_wnd = 0x83; // enable outside of window 0 for nbg0
   nbg1_wnd = 0x88; // enable inside of window 1 for nbg1
   VDP2_REG_WCTLA = (nbg1_wnd << 8) | nbg0_wnd;        
   vdp_disp_on();

//   WorkingQuerry("Is the above graphics displayed?");
   for(;;)
   {
      vdp_vsync();

      if(dir > 0)
      {
         if (counter2 >= (224-1))
         {
            dir = -1;
            nbg0_wnd ^= 1;
         }
         else
         {
            counter2++;
            counter=counter2 * (320-1) / (224-1);
         }
      }
      else
      {
         if (counter2 <= 0)
         {
            dir = 1;
            nbg0_wnd ^= 1;
         }
         else
         {
            counter2--;
            counter=counter2 * (320-1) / (224-1);
         }
      }

      VDP2_REG_WPEX0 = counter << 1;
      VDP2_REG_WPEY0 = counter2;
      VDP2_REG_WCTLA = (nbg1_wnd << 8) | nbg0_wnd;

      vdp_printf(&test_disp_font, 0 * 8, 26 * 8, 0xC, "%03d %03d", counter, counter2);

      if (per[0].but_push_once & PAD_START || per[0].but_push_once & PAD_B)
         break;
   }

   // Disable NBG0/NBG1
   vdp_nbg0_deinit();
   vdp_nbg1_deinit();
   yabauseut_init();
}

//////////////////////////////////////////////////////////////////////////////

