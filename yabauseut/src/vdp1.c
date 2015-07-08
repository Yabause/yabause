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
    along with Lapetus; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <iapetus.h>
#include "tests.h"
#include "vdp1.h"

//////////////////////////////////////////////////////////////////////////////

void vdp1_test()
{
   int choice;

   menu_item_struct vdp1_menu[] = {
   { "Draw commands" , &vdp1_draw_test, },
   { "Clip commands" , &vdp1_clip_test, },
   { "Misc" , &vdp1_misc_test, },
   { "\0", NULL }
   };

   for (;;)
   {
      choice = gui_do_menu(vdp1_menu, &test_disp_font, 0, 0, "VDP1 Tests", MTYPE_CENTER, -1);
      if (choice == -1)
         break;
   }   
}

//////////////////////////////////////////////////////////////////////////////

void vdp1_draw_test()
{
}

//////////////////////////////////////////////////////////////////////////////

void vdp1_clip_test()
{
   int gouraud_table_address = 0x40000;
   u32 clipping_mode = 3;//outside

   u16* p = (u16 *)(0x25C00000 + gouraud_table_address);

   for (;;)
   {
      vdp_start_draw_list();

      sprite_struct quad;

      //system clipping
      quad.x = 319 - 8;
      quad.y = 223 - 8;

      vdp_system_clipping(&quad);

      //user clipping
      quad.x = 8;
      quad.y = 8;
      quad.x2 = 319 - 16;
      quad.y2 = 223 - 16;

      vdp_user_clipping(&quad);

      //fullscreen polygon
      quad.x = 319;
      quad.y = 0;
      quad.x2 = 319;
      quad.y2 = 223;
      quad.x3 = 0;
      quad.y3 = 223;
      quad.x4 = 0;
      quad.y4 = 0;

      quad.bank = RGB16(0x10, 0x10, 0x10);//gray

      quad.gouraud_addr = gouraud_table_address;

      quad.attr = (clipping_mode << 9) | 4;//use gouraud shading

      //red, green, blue, and white
      p[0] = RGB16(31, 0, 0);
      p[1] = RGB16(0, 31, 0);
      p[2] = RGB16(0, 0, 31);
      p[3] = RGB16(31, 31, 31);

      vdp_draw_polygon(&quad);

      vdp_end_draw_list();

      vdp_vsync();

      if (per[0].but_push_once & PAD_A)
      {
         clipping_mode = 0; //clipping disabled
      }
      if (per[0].but_push_once & PAD_B)
      {
         clipping_mode = 2; //inside drawing mode
      }
      if (per[0].but_push_once & PAD_C)
      {
         clipping_mode = 3; //outside drawing mode
      }

      if (per[0].but_push_once & PAD_START)
         break;
   }
}

//////////////////////////////////////////////////////////////////////////////

void vdp1_misc_test()
{
}

//////////////////////////////////////////////////////////////////////////////

