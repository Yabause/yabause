/*  Copyright 2015 Theo Berkau

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#ifndef SCSPDSP_H
#define SCSPDSP_H

#include "core.h"

//dsp instruction format

//bits 63-48
//|  ?  |                   tra                   | twt |                   twa                   |
//|  F  |  E  |  D  |  C  |  B  |  A  |  9  |  8  |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |

//bits 47-32
//|xsel |    ysel   |  ?  |                ira                | iwt |             iwa             |
//|  F  |  E  |  D  |  C  |  B  |  A  |  9  |  8  |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |

//bits 31-16
//|table| mwt | mrd | ewt |          ewa          |adrl |frcl |   shift   | yrl |negb |zero |bsel |
//|  F  |  E  |  D  |  C  |  B  |  A  |  9  |  8  |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |

//bits 15-0
//|nofl |                coef               |     ?     |            masa             |adreb|nxadr|
//|  F  |  E  |  D  |  C  |  B  |  A  |  9  |  8  |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |

#ifdef WORDS_BIGENDIAN
union ScspDspInstruction {
   struct {
      u64 unknown : 1;
      u64 tra : 7;
      u64 twt : 1;
      u64 twa : 7;

      u64 xsel : 1;
      u64 ysel : 2;
      u64 unknown2 : 1;
      u64 ira : 6;
      u64 iwt : 1;
      u64 iwa : 5;

      u64 table : 1;
      u64 mwt : 1;
      u64 mrd : 1;
      u64 ewt : 1;
      u64 ewa : 4;
      u64 adrl : 1;
      u64 frcl : 1;
      u64 shift : 2;
      u64 yrl : 1;
      u64 negb : 1;
      u64 zero : 1;
      u64 bsel : 1;

      u64 nofl : 1;
      u64 coef : 6;
      u64 unknown3 : 2;
      u64 masa : 5;
      u64 adreb : 1;
      u64 nxadr : 1;
   } part;
   u32 all;
};
#else
union ScspDspInstruction {
   struct {
      u64 nxadr : 1;
      u64 adreb : 1;
      u64 masa : 5;
      u64 unknown3 : 2;
      u64 coef : 6;
      u64 nofl : 1;
      u64 bsel : 1;
      u64 zero : 1;
      u64 negb : 1;
      u64 yrl : 1;
      u64 shift : 2;
      u64 frcl : 1;
      u64 adrl : 1;
      u64 ewa : 4;
      u64 ewt : 1;
      u64 mrd : 1;
      u64 mwt : 1;
      u64 table : 1;
      u64 iwa : 5;
      u64 iwt : 1;
      u64 ira : 6;
      u64 unknown2 : 1;
      u64 ysel : 2;
      u64 xsel : 1;
      u64 twa : 7;
      u64 twt : 1;
      u64 tra : 7;
      u64 unknown : 1;
   } part;
   u64 all;
};
#endif

void ScspDspDisasm(u8 addr, char *outstring);
s32 float_to_int(u16 f_val);
u32 int_to_float(u32 i_val);

struct ScspDspInterface
{
   void (*set_rbl_rbp)(u32 rbl, u32 rbp);
   void(*set_exts)(u32 l, u32 r);
   void(*set_mixs)(u32 i, u32 data);
   u32(*get_effect_out)(int i);
   void(*set_mpro)(u64 input, u32 addr);
   u64 (*get_mpro)(u32 addr);
   void(*set_coef)(u32 input, u32 addr);
   void(*set_madrs)(u32 input, u32 addr);
   u32(*get_coef)(u32 addr);
   u32(*get_exts)(u32 addr);
   u32(*get_madrs)(u32 addr);
   u32(*get_mems)(u32 addr);
   void(*exec)();
};

extern struct ScspDspInterface dsp_inf;

void scsp_dsp_int_init();
#endif
