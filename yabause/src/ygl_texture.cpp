/*
        Copyright 2019 devMiyax(smiyaxdev@gmail.com)

This file is part of YabaSanshiro.

        YabaSanshiro is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

YabaSanshiro is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
along with YabaSanshiro; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

extern "C" {
#include "ygl.h"
#include "yui.h"
#include "vidshared.h"

  extern vdp2rotationparameter_struct  paraA;
  extern vdp2rotationparameter_struct  paraB;
  extern Vdp2 * fixVdp2Regs;
}

#if defined(HAVE_VULKAN)
#include "vulkan/VIDVulkan.h"
#include "vulkan/vulkan.hpp"
#endif


#define YGLDEBUG LOG

const char prg_generate_rbg[] =
#if defined(_OGLES3_)
"#version 310 es \n"
"precision highp float; \n"
"precision highp int;\n"
"precision highp image2D;\n"
#else
"#version 430 \n"
#endif
"#pragma optionNV(inline all)\n"
"layout(local_size_x = 16, local_size_y = 16) in;\n"
"layout(rgba8, binding = 0)  uniform writeonly image2D outSurface;\n"
"layout(std430, binding = 1) readonly buffer VDP2 { uint vram[]; };\n"
"struct vdp2rotationparameter_struct{ \n"
" uint PlaneAddrv[16];\n"
" float Xst;\n"
" float Yst;\n"
" float Zst;\n"
" float deltaXst;\n"
" float deltaYst;\n"
" float deltaX;\n"
" float deltaY;\n"
" float A;\n"
" float B;\n"
" float C;\n"
" float D;\n"
" float E;\n"
" float F;\n"
" float Px;\n"
" float Py;\n"
" float Pz;\n"
" float Cx;\n"
" float Cy;\n"
" float Cz;\n"
" float Mx;\n"
" float My;\n"
" float kx;\n"
" float ky;\n"
" float KAst;\n"
" float deltaKAst;\n"
" float deltaKAx;\n"
" uint coeftbladdr;\n"
" int coefenab;\n"
" int coefmode;\n"
" int coefdatasize;\n"
" int use_coef_for_linecolor;\n"
" float Xp;\n"
" float Yp;\n"
" float dX;\n"
" float dY;\n"
" int screenover;\n"
" int msb;\n"
" uint charaddr;\n"
" int planew, planew_bits, planeh, planeh_bits;\n"
" int MaxH, MaxV;\n"
" float Xsp;\n"
" float Ysp;\n"
" float dx;\n"
" float dy;\n"
" float lkx;\n"
" float lky;\n"
" int KtablV;\n"
" int ShiftPaneX;\n"
" int ShiftPaneY;\n"
" int MskH;\n"
" int MskV;\n"
" uint lineaddr;\n"
" int k_mem_type;\n"
" uint over_pattern_name;\n"
" int linecoefenab;\n"
"};\n"
"layout(std430, binding = 2) readonly buffer vdp2Param { \n"
"  vdp2rotationparameter_struct para[2];\n"
"};\n"
"layout(std140, binding = 3) uniform  RBGDrawInfo { \n"
"  float hres_scale; \n"
"  float vres_scale; \n"
"  int cellw_; \n"
"  int cellh_; \n"
"  uint paladdr_; \n"
"  int pagesize; \n"
"  int patternshift; \n"
"  int planew; \n"
"  int pagewh; \n"
"  int patterndatasize; \n"
"  uint supplementdata; \n"
"  int auxmode; \n"
"  int patternwh;\n"
"  uint coloroffset;\n"
"  int transparencyenable;\n"
"  int specialcolormode;\n"
"  int specialcolorfunction;\n"
"  uint specialcode;\n"
"  int colornumber;\n"
"  int window_area_mode;"
"  float alpha_;"
"  int cram_shift;"
"  int hires_shift;"
"  int specialprimode;"
"  uint priority;"
"};\n"
" struct vdp2WindowInfo\n"
"{\n"
"  int WinShowLine;\n"
"  int WinHStart;\n"
"  int WinHEnd;\n"
"};\n"
"layout(std430, binding = 4) readonly buffer windowinfo { \n"
"  vdp2WindowInfo pWinInfo[];\n"
"};\n"
"layout(std430, binding = 5) readonly buffer VDP2C { uint cram[]; };\n"
" int GetKValue( int paramid, float posx, float posy, out float ky, out uint lineaddr ){ \n"
"  uint kdata;\n"
"  int kindex = int( ceil(para[paramid].deltaKAst*posy+(para[paramid].deltaKAx*posx)) ); \n"
"  if (para[paramid].coefdatasize == 2) { \n"
"    uint addr = ( uint( int(para[paramid].coeftbladdr) + (kindex<<1)) &0x7FFFFu); "
"    if( para[paramid].k_mem_type == 0) { \n"
"	     kdata = vram[ addr>>2 ]; \n"
"      if( (addr & 0x02u) != 0u ) { kdata >>= 16; } \n"
"      kdata = (((kdata) >> 8 & 0xFFu) | ((kdata) & 0xFFu) << 8);\n"
"    }else{\n"
"      kdata = cram[ ((0x800u + (addr&0x7FFu))>>2)  ]; \n"
"      if( (addr & 0x02u) != 0u ) { kdata >>= 16; } \n"
"    }\n"
"    if ( (kdata & 0x8000u) != 0u) { return -1; }\n"
"	   if((kdata&0x4000u)!=0u) ky=float( int(kdata&0x7FFFu)| int(0xFFFF8000u) )/1024.0; else ky=float(kdata&0x7FFFu)/1024.0;\n"
"  }else{\n"
"    uint addr = ( uint( int(para[paramid].coeftbladdr) + (kindex<<2))&0x7FFFFu);"
"    if( para[paramid].k_mem_type == 0) { \n"
"	     kdata = vram[ addr>>2 ]; \n"
"      kdata = ((kdata&0xFF000000u) >> 24 | ((kdata) >> 8 & 0xFF00u) | ((kdata) & 0xFF00u) << 8 | (kdata&0x000000FFu) << 24);\n"
"    }else{\n"
"      kdata = cram[ ((0x800u + (addr&0x7FFu) )>>2) ]; \n"
"      kdata = ((kdata&0xFFFF0000u)>>16|(kdata&0x0000FFFFu)<<16);\n"
"    }\n"
"	 if( para[paramid].linecoefenab != 0) lineaddr = (kdata >> 24) & 0x7Fu; else lineaddr = 0u;\n"
"	 if((kdata&0x80000000u)!=0u){ return -1;}\n"
"	 if((kdata&0x00800000u)!=0u) ky=float( int(kdata&0x00FFFFFFu)| int(0xFF800000u) )/65536.0; else ky=float(kdata&0x00FFFFFFu)/65536.0;\n"
"  }\n"
"  return 0;"
" }\n"

" bool isWindowInside(int posx, int posy) {\n"
" posy <<= hires_shift;"
"	if (window_area_mode == 0) {\n"
"		if (pWinInfo[posy].WinShowLine == 0) {\n"
"			return true;\n"
"		}\n"
"		else {\n"
"			if (posx < pWinInfo[posy].WinHStart || posx >= pWinInfo[posy].WinHEnd) {\n"
"				return true;\n"
"			}\n"
"			else {\n"
"				return false;\n"
"			}\n"
"		}\n"
"	}\n"
"	else {\n"
"		if (pWinInfo[posy].WinShowLine == 0) {\n"
"			return false;\n"
"		}\n"
"		else {\n"
"			if (posx < pWinInfo[posy].WinHStart || posx >= pWinInfo[posy].WinHEnd) {\n"
"				return false;\n"
"			}\n"
"			else {\n"
"				return true;\n"
"			}\n"
"		}\n"
"	}\n"
"	return true;\n"
"}\n"

"uint get_cram_msb(uint colorindex) { \n"
"	uint colorval = 0u; \n"
"	colorindex = (colorindex << cram_shift) & 0xFFFu; \n"
"	colorval = cram[colorindex >> 2]; \n"
"	if ((colorindex & 0x02u) != 0u) { colorval >>= 16; } \n"
"	return (colorval & 0x8000u); \n"
"}\n"


"int PixelIsSpecialPriority( uint specialcode, uint dot ) { \n"
"  dot &= 0xfu; \n"
"  if ( (specialcode & 0x01u) != 0u && (dot == 0u || dot == 1u) ){ return 1;} \n"
"  if ( (specialcode & 0x02u) != 0u && (dot == 2u || dot == 3u) ){ return 1;} \n"
"  if ( (specialcode & 0x04u) != 0u && (dot == 4u || dot == 5u) ){ return 1;} \n"
"  if ( (specialcode & 0x08u) != 0u && (dot == 6u || dot == 7u) ){ return 1;} \n"
"  if ( (specialcode & 0x10u) != 0u && (dot == 8u || dot == 9u) ){ return 1;} \n"
"  if ( (specialcode & 0x20u) != 0u && (dot == 0xau || dot == 0xbu) ){ return 1;} \n"
"  if ( (specialcode & 0x40u) != 0u && (dot == 0xcu || dot == 0xdu) ){ return 1;} \n"
"  if ( (specialcode & 0x80u) != 0u && (dot == 0xeu || dot == 0xfu) ){ return 1;} \n"
"  return 0; \n"
"} \n"


//----------------------------------------------------------------------
// Main
//----------------------------------------------------------------------
"void main(){ \n"
"  int x, y;\n"
"  int paramid = 0;\n"
"  int cellw;\n"
"  uint paladdr; \n"
"  uint charaddr; \n"
"  uint lineaddr = 0u; \n"
"  float ky; \n"
"  uint kdata;\n"
"  uint patternname = 0xFFFFFFFFu;\n"
"  uint specialfunction_in = 0u;\n"
"  uint specialcolorfunction_in = 0u;\n"
"  ivec2 texel = ivec2(gl_GlobalInvocationID.xy);\n"
"  ivec2 size = imageSize(outSurface);\n"
"  if (texel.x >= size.x || texel.y >= size.y ) return;\n"
"  float posx = float(texel.x) * hres_scale;\n"
"  float posy = float(texel.y) * vres_scale;\n"
"  specialfunction_in = (supplementdata >> 9) & 0x1u; \n"
"  specialcolorfunction_in = (supplementdata >> 8) & 0x1u; \n";

const char prg_rbg_rpmd0_2w[] =
"  paramid = 0; \n"
"  ky = para[paramid].ky; \n"
"  lineaddr = para[paramid].lineaddr; \n"
"  if( para[paramid].coefenab != 0 ){ \n"
"   if( GetKValue(paramid,posx,posy,ky,lineaddr ) == -1 ) { imageStore(outSurface,texel,vec4(0.0)); return;} \n"
"  }\n";

const char prg_rbg_rpmd1_2w[] =
"  paramid = 1; \n"
"  ky = para[paramid].ky; \n"
"  lineaddr = para[paramid].lineaddr; \n"
"  if( para[paramid].coefenab != 0 ){ \n"
"   if( GetKValue(paramid,posx,posy,ky,lineaddr ) == -1 ) { imageStore(outSurface,texel,vec4(0.0)); return;} \n"
"  }\n";


const char prg_rbg_rpmd2_2w[] =
"  paramid = 0; \n"
"  ky = para[paramid].ky; \n"
"  lineaddr = para[paramid].lineaddr; \n"
"  if( para[paramid].coefenab != 0 ){ \n"
"    if( GetKValue(paramid,posx,posy,ky,lineaddr ) == -1 ) { \n"
"      paramid=1;\n"
"      if( para[paramid].coefenab != 0 ){ \n"
"        if( GetKValue(paramid,posx,posy,ky,lineaddr ) == -1 ) { imageStore(outSurface,texel,vec4(0.0)); return;} \n"
"      }else{ \n"
"        ky = para[paramid].ky; \n"
"      }\n"
"    }\n"
"  }\n";


const char prg_get_param_mode03[] =
"  if( isWindowInside( int(posx), int(posy)) ) { "
"    paramid = 0; \n"
"    if( para[paramid].coefenab != 0 ){ \n"
"      if( GetKValue(paramid,posx,posy,ky,lineaddr ) == -1 ) { \n"
"        paramid=1;\n"
"        if( para[paramid].coefenab != 0 ){ \n"
"          if( GetKValue(paramid,posx,posy,ky,lineaddr ) == -1 ) { imageStore(outSurface,texel,vec4(0.0)); return;} \n"
"        }else{ \n"
"          ky = para[paramid].ky; \n"
"          lineaddr = para[paramid].lineaddr; \n"
"        }\n"
"      }\n"
"    }else{\n"
"      ky = para[paramid].ky; \n"
"      lineaddr = para[paramid].lineaddr; \n"
"    }\n"
"  }else{\n"
"    paramid = 1; \n"
"    if( para[paramid].coefenab != 0 ){ \n"
"      if( GetKValue(paramid,posx,posy,ky,lineaddr ) == -1 ) { \n"
"        paramid=0;\n"
"        if( para[paramid].coefenab != 0 ){ \n"
"          if( GetKValue(paramid,posx,posy,ky,lineaddr ) == -1 ) { imageStore(outSurface,texel,vec4(0.0)); return;} \n"
"        }else{ \n"
"          ky = para[paramid].ky; \n"
"          lineaddr = para[paramid].lineaddr; \n"
"        }\n"
"      }\n"
"    }else{\n"
"      ky = para[paramid].ky; \n"
"      lineaddr = para[paramid].lineaddr; \n"
"    }\n"
" }\n";


const char prg_rbg_xy[] =
"  float Xsp = para[paramid].A * ((para[paramid].Xst + para[paramid].deltaXst * posy) - para[paramid].Px) +\n"
"  para[paramid].B * ((para[paramid].Yst + para[paramid].deltaYst * posy) - para[paramid].Py) +\n"
"  para[paramid].C * (para[paramid].Zst - para[paramid].Pz);\n"
"  float Ysp = para[paramid].D * ((para[paramid].Xst + para[paramid].deltaXst *posy) - para[paramid].Px) +\n"
"  para[paramid].E * ((para[paramid].Yst + para[paramid].deltaYst * posy) - para[paramid].Py) +\n"
"  para[paramid].F * (para[paramid].Zst - para[paramid].Pz);\n"
"  float fh = (ky * (Xsp + para[paramid].dx * posx) + para[paramid].Xp);\n"
"  float fv = (ky * (Ysp + para[paramid].dy * posx) + para[paramid].Yp);\n";

const char prg_rbg_get_bitmap[] =
"  cellw = cellw_;\n"
"  charaddr = para[paramid].charaddr;\n"
"  paladdr = paladdr_;\n"
"  switch( para[paramid].screenover){ \n "
"  case 0: // OVERMODE_REPEAT \n"
"  case 1: // OVERMODE_SELPATNAME \n"
"    x = int(fh) & (cellw-1);\n"
"    y = int(fv) & (cellh_-1);\n"
"    break;\n"
"  case 2: // OVERMODE_TRANSE \n"
"    if ((fh < 0.0) || (fh > float(cellw_)) || (fv < 0.0) || (fv > float(cellh_)) ) {\n"
"      imageStore(outSurface,texel,vec4(0.0));\n"
"      return; \n"
"    }\n"
"    x = int(fh);\n"
"    y = int(fv);\n"
"    break;\n"
"  case 3: // OVERMODE_512 \n"
"    if ((fh < 0.0) || (fh > 512.0) || (fv < 0.0) || (fv > 512.0)) {\n"
"      imageStore(outSurface,texel,vec4(0.0));\n"
"      return; \n"
"    }\n"
"    x = int(fh);\n"
"    y = int(fv);\n"
"    break;\n"
"  }\n";

const char prg_rbg_overmode_repeat[] =
"  switch( para[paramid].screenover){ \n "
"  case 0: // OVERMODE_REPEAT \n"
"    x = int(fh) & (para[paramid].MaxH-1);\n"
"    y = int(fv) & (para[paramid].MaxV-1);\n"
"    break;\n"
"  case 1: // OVERMODE_SELPATNAME \n"
"    if ((fh < 0.0) || (fh > float(para[paramid].MaxH)) || (fv < 0.0) || (fv > float(para[paramid].MaxV)) ) {\n"
"        patternname = para[paramid].over_pattern_name;\n"
"    }"
"    x = int(fh);\n"
"    y = int(fv);\n"
"    break;\n"
"  case 2: // OVERMODE_TRANSE \n"
"    if ((fh < 0.0) || (fh > float(para[paramid].MaxH) ) || (fv < 0.0) || (fv > float(para[paramid].MaxV)) ) {\n"
"      imageStore(outSurface,texel,vec4(0.0));\n"
"      return; \n"
"    }\n"
"    x = int(fh);\n"
"    y = int(fv);\n"
"    break;\n"
"  case 3: // OVERMODE_512 \n"
"    if ((fh < 0.0) || (fh > 512.0) || (fv < 0.0) || (fv > 512.0)) {\n"
"      imageStore(outSurface,texel,vec4(0.0));\n"
"      return; \n"
"    }\n"
"    x = int(fh);\n"
"    y = int(fv);\n"
"    break;\n"
"   }\n";


const char prg_rbg_get_patternaddr[] =
"  int planenum = (x >> para[paramid].ShiftPaneX) + ((y >> para[paramid].ShiftPaneY) << 2);\n"
"  x &= (para[paramid].MskH);\n"
"  y &= (para[paramid].MskV);\n"
"  uint addr = para[paramid].PlaneAddrv[planenum];\n"
"  addr += uint( (((y >> 9) * pagesize * planew) + \n"
"  ((x >> 9) * pagesize) + \n"
"  (((y & 511) >> patternshift) * pagewh) + \n"
"  ((x & 511) >> patternshift)) << patterndatasize ); \n"
"  addr &= 0x7FFFFu;\n";

const char prg_rbg_get_pattern_data_1w[] =
"  if( patternname == 0xFFFFFFFFu){\n"
"    patternname = vram[addr>>2]; \n" // WORD mode( patterndatasize == 1 )
"    if( (addr & 0x02u) != 0u ) { patternname >>= 16; } \n"
"    patternname = (((patternname >> 8) & 0xFFu) | ((patternname) & 0xFFu) << 8);\n"
"  }\n"
"  if(colornumber==0) paladdr = ((patternname & 0xF000u) >> 12) | ((supplementdata & 0xE0u) >> 1); else paladdr = (patternname & 0x7000u) >> 8;\n" // not in 16 colors
"  uint flipfunction;\n"
"  switch (auxmode)\n"
"  {\n"
"  case 0: \n"
"    flipfunction = (patternname & 0xC00u) >> 10;\n"
"    switch (patternwh)\n"
"    {\n"
"    case 1:\n"
"      charaddr = (patternname & 0x3FFu) | ((supplementdata & 0x1Fu) << 10);\n"
"      break;\n"
"    case 2:\n"
"      charaddr = ((patternname & 0x3FFu) << 2) | (supplementdata & 0x3u) | ((supplementdata & 0x1Cu) << 10);\n"
"      break;\n"
"    }\n"
"    break;\n"
"  case 1:\n"
"    flipfunction = 0u;\n"
"    switch (patternwh)\n"
"    {\n"
"    case 1:\n"
"      charaddr = (patternname & 0xFFFu) | ((supplementdata & 0x1Cu) << 10);\n"
"      break;\n"
"    case 2:\n"
"      charaddr = ((patternname & 0xFFFu) << 2) | (supplementdata & 0x3u) | ((supplementdata & 0x10u) << 10);\n"
"      break;\n"
"    }\n"
"    break;\n"
"  }\n"
"  charaddr &= 0x3FFFu;\n"
"  charaddr *= 0x20u;\n";

const char prg_rbg_get_pattern_data_2w[] =
"  patternname = vram[addr>>2]; \n"
"  uint tmp1 = patternname & 0x7FFFu; \n"
"  charaddr = patternname >> 16; \n"
"  charaddr = (((charaddr >> 8) & 0xFFu) | ((charaddr) & 0xFFu) << 8);\n"
"  tmp1 = (((tmp1 >> 8) & 0xFFu) | ((tmp1) & 0xFFu) << 8);\n"
"  uint flipfunction = (tmp1 & 0xC000u) >> 14;\n"
"  if(colornumber==0) paladdr = tmp1 & 0x7Fu; else paladdr = tmp1 & 0x70u;\n" // not in 16 colors
"  specialfunction_in = (tmp1 & 0x2000u) >> 13;\n"
"  specialcolorfunction_in = (tmp1 & 0x1000u) >> 12;\n"
"  charaddr &= 0x3FFFu;\n"
"  charaddr *= 0x20u;\n";

const char prg_rbg_get_charaddr[] =
"  cellw = 8; \n"
"  if (patternwh == 1) { \n" // Figure out which pixel in the tile we want
"    x &= 0x07;\n"
"    y &= 0x07;\n"
"    if ( (flipfunction & 0x2u) != 0u ) y = 7 - y;\n"
"    if ( (flipfunction & 0x1u) != 0u ) x = 7 - x;\n"
"  }else{\n"
"    if (flipfunction != 0u) { \n"
"      y &= 16 - 1;\n"
"      if ( (flipfunction & 0x2u) != 0u ) {\n"
"        if ( (y & 8) == 0 ) {\n"
"          y = 8 - 1 - y + 16;\n"
"        }else{ \n"
"          y = 16 - 1 - y;\n"
"        }\n"
"      } else if ( (y & 8) != 0 ) { \n"
"        y += 8; \n"
"      }\n"
"      if ((flipfunction & 0x1u) != 0u ) {\n"
"        if ( (x & 8) == 0 ) y += 8;\n"
"        x &= 8 - 1;\n"
"        x = 8 - 1 - x;\n"
"      } else if ( (x & 8) != 0 ) {\n"
"        y += 8;\n"
"        x &= 8 - 1;\n"
"      } else {\n"
"        x &= 8 - 1;\n"
"      }\n"
"    }else{\n"
"      y &= 16 - 1;\n"
"      if ( (y & 8) != 0 ) y += 8;\n"
"      if ( (x & 8) != 0 ) y += 8;\n"
"      x &= 8 - 1;\n"
"    }\n"
"  }\n";


// 4 BPP
const char prg_rbg_getcolor_4bpp[] =
"  uint dot = 0u;\n"
"  uint cramindex = 0u;\n"
"  float alpha = alpha_;\n"
"  uint dotaddr = ((charaddr + uint(((y * cellw) + x) >> 1)) & 0x7FFFFu);\n"
"  dot = vram[ dotaddr >> 2];\n"
"  if( (dotaddr & 0x3u) == 0u ) dot >>= 0;\n"
"  else if( (dotaddr & 0x3u) == 1u ) dot >>= 8;\n"
"  else if( (dotaddr & 0x3u) == 2u ) dot >>= 16;\n"
"  else if( (dotaddr & 0x3u) == 3u ) dot >>= 24;\n"
"  if ( (x & 0x1) == 0 ) dot >>= 4;\n"
"  if ( (dot & 0xFu) == 0u && transparencyenable != 0 ) { \n"
"    cramindex = 0u; \n"
"    alpha = 0.0;\n"
"  } else {\n"
"    cramindex = (coloroffset + ((paladdr << 4) | (dot & 0xFu)));\n"
"    if (specialprimode == 2) { \n"
"      uint spriority = priority & 0xEu; \n"
"      if ( (specialfunction_in & 0x01u) != 0u ) { \n"
"        if( PixelIsSpecialPriority(specialcode, dot ) == 1 ){ \n"
"          spriority |= 0x1u; \n"
"        }\n"
"      }\n"
"      cramindex |= spriority << 16; \n"
"    }\n"
"    switch (specialcolormode)\n"
"    {\n"
"    case 1:\n"
"      if (specialcolorfunction == 0) { alpha = 1.0; } break;\n"
"    case 2:\n"
"      if (specialcolorfunction == 0) { alpha = 1.0; }\n"
"      else { if ((specialcode & (1u << ((dot & 0xFu) >> 1))) == 0u) { alpha = 1.0; } } \n"
"      break; \n"
"    case 3:\n"
"	   if (get_cram_msb(cramindex) == 0u) { alpha = 1.0; }\n"
"	   break;\n"
"    }\n"
"  }\n";


// 8BPP
const char prg_rbg_getcolor_8bpp[] =
"  uint dot = 0u;\n"
"  uint cramindex = 0u;\n"
"  float alpha = alpha_;\n"
"  uint dotaddr = charaddr + uint((y*cellw)+x);\n"
"  dot = vram[ dotaddr >> 2];\n"
"  if( (dotaddr & 0x3u) == 0u ) dot >>= 0;\n"
"  else if( (dotaddr & 0x3u) == 1u ) dot >>= 8;\n"
"  else if( (dotaddr & 0x3u) == 2u ) dot >>= 16;\n"
"  else if( (dotaddr & 0x3u) == 3u ) dot >>= 24;\n"
"  dot = dot & 0xFFu; \n"
"  if ( dot == 0u && transparencyenable != 0 ) { \n"
"    cramindex = 0u; \n"
"    alpha = 0.0;\n"
"  } else {\n"
"    cramindex = (coloroffset + ((paladdr << 4) | dot));\n"
"    if (specialprimode == 2) { \n"
"      uint spriority = priority & 0xEu; \n"
"      if ( (specialfunction_in & 0x01u) != 0u) { \n"
"        if( PixelIsSpecialPriority(specialcode, dot ) == 1 ){ \n"
"          spriority |= 0x1u; \n"
"        }\n"
"      }\n"
"      cramindex |= spriority << 16; \n"
"    }\n"
"    switch (specialcolormode)\n"
"    {\n"
"    case 1:\n"
"      if (specialcolorfunction == 0) { alpha = 1.0; } break;\n"
"    case 2:\n"
"      if (specialcolorfunction == 0) { alpha = 1.0; }\n"
"      else { if ((specialcode & (1u << ((dot & 0xFu) >> 1))) == 0u) { alpha = 1.0; } } \n"
"      break; \n"
"    case 3:\n"
"	   if (get_cram_msb(cramindex) == 0u) { alpha = 1.0; }\n"
"	   break;\n"
"    }\n"
"  }\n";


const char prg_rbg_getcolor_16bpp_palette[] =
"  uint dot = 0u;\n"
"  uint cramindex = 0u;\n"
"  float alpha = alpha_;\n"
"  uint dotaddr = charaddr + uint((y*cellw)+x) * 2u;\n"
"  dot = vram[dotaddr>>2]; \n"
"  if( (dotaddr & 0x02u) != 0u ) { dot >>= 16; } \n"
"  dot = (((dot) >> 8 & 0xFF) | ((dot) & 0xFF) << 8);\n"
"  if ( dot == 0 && transparencyenable != 0 ) { \n"
"    cramindex = 0u; \n"
"    alpha = 0.0;\n"
"  } else {\n"
"    cramindex = (coloroffset + dot);\n"
"  }\n";

const char prg_rbg_getcolor_16bpp_rbg[] =
"  uint dot = 0u;\n"
"  uint cramindex = 0u;\n"
"  float alpha = alpha_;\n"
"  uint dotaddr = charaddr + uint((y*cellw)+x) * 2u;\n"
"  dot = vram[dotaddr>>2]; \n"
"  if( (dotaddr & 0x02u) != 0u ) { dot >>= 16; } \n"
"  dot = (((dot >> 8) & 0xFFu) | ((dot) & 0xFFu) << 8);\n"
"  if ( (dot&0x8000u) == 0u && transparencyenable != 0 ) { \n"
"    cramindex = 0u; \n"
"    alpha = 0.0;\n"
"  } else {\n"
"    cramindex = (dot & 0x1Fu) << 3 | (dot & 0x3E0u) << 6 | (dot & 0x7C00u) << 9;\n"
"  }\n";


const char prg_rbg_getcolor_32bpp_rbg[] =
"  uint dot = 0u;\n"
"  uint cramindex = 0u;\n"
"  float alpha = alpha_;\n"
"  uint dotaddr = charaddr + uint((y*cellw)+x) * 4u;\n"
"  dot = vram[dotaddr>>2]; \n"
"  dot = ((dot&0xFF000000u) >> 24 | ((dot >> 8) & 0xFF00u) | ((dot) & 0xFF00u) << 8 | (dot&0x000000FFu) << 24);\n"
"  if ( (dot&0x80000000u) == 0u && transparencyenable != 0 ) { \n"
"    cramindex = 0u; \n"
"    alpha = 0.0;\n"
"  } else {\n"
"    cramindex = dot & 0x00FFFFFFu;\n"
"  }\n";


const char prg_generate_rbg_end[] =
"  imageStore(outSurface,texel,vec4( float(cramindex&0xFFu)/255.0, float((cramindex>>8) &0xFFu)/255.0, float((cramindex>>16) &0xFFu)/255.0, alpha));\n"
"}\n";

const char prg_generate_rbg_line_end[] =
"  cramindex |= 0x8000u;\n"
"  uint line_color = 0u;\n"
"  if( lineaddr != 0xFFFFFFFFu && lineaddr != 0u ) line_color = ((lineaddr & 0x7Fu) | 0x80u);"
"  imageStore(outSurface,texel,vec4( float(cramindex&0xFFu)/255.0, float((cramindex>>8)&0xFFu)/255.0, float((line_color)&0xFFu)/255.0, alpha));\n"
"}\n";




const GLchar * a_prg_rbg_0_2w_bitmap[] = {
  prg_generate_rbg,
  prg_rbg_rpmd0_2w,
  prg_rbg_xy,
  prg_rbg_get_bitmap,
  prg_rbg_getcolor_8bpp,
  prg_generate_rbg_end
};

const GLchar * a_prg_rbg_0_2w_p1_4bpp[] = {
  prg_generate_rbg,
  prg_rbg_rpmd0_2w,
  prg_rbg_xy,
  prg_rbg_overmode_repeat,
  prg_rbg_get_patternaddr,
  prg_rbg_get_pattern_data_1w,
  prg_rbg_get_charaddr,
  prg_rbg_getcolor_4bpp,
  prg_generate_rbg_end };

const GLchar * a_prg_rbg_0_2w_p2_4bpp[] = {
  prg_generate_rbg,
  prg_rbg_rpmd0_2w,
  prg_rbg_xy,
  prg_rbg_overmode_repeat,
  prg_rbg_get_patternaddr,
  prg_rbg_get_pattern_data_2w,
  prg_rbg_get_charaddr,
  prg_rbg_getcolor_4bpp,
  prg_generate_rbg_end };

const GLchar * a_prg_rbg_0_2w_p1_8bpp[] = {
  prg_generate_rbg,
  prg_rbg_rpmd0_2w,
  prg_rbg_xy,
  prg_rbg_overmode_repeat,
  prg_rbg_get_patternaddr,
  prg_rbg_get_pattern_data_1w,
  prg_rbg_get_charaddr,
  prg_rbg_getcolor_8bpp,
  prg_generate_rbg_end };

const GLchar * a_prg_rbg_0_2w_p2_8bpp[] = {
  prg_generate_rbg,
  prg_rbg_rpmd0_2w,
  prg_rbg_xy,
  prg_rbg_overmode_repeat,
  prg_rbg_get_patternaddr,
  prg_rbg_get_pattern_data_2w,
  prg_rbg_get_charaddr,
  prg_rbg_getcolor_8bpp,
  prg_generate_rbg_end };

//--------------------------------------------------
// RPMD 1
const GLchar * a_prg_rbg_1_2w_bitmap[] = {
  prg_generate_rbg,
  prg_rbg_rpmd1_2w,
  prg_rbg_xy,
  prg_rbg_get_bitmap,
  prg_rbg_getcolor_8bpp,
  prg_generate_rbg_end
};

const GLchar * a_prg_rbg_1_2w_p1_4bpp[] = {
  prg_generate_rbg,
  prg_rbg_rpmd1_2w,
  prg_rbg_xy,
  prg_rbg_overmode_repeat,
  prg_rbg_get_patternaddr,
  prg_rbg_get_pattern_data_1w,
  prg_rbg_get_charaddr,
  prg_rbg_getcolor_4bpp,
  prg_generate_rbg_end };

const GLchar * a_prg_rbg_1_2w_p2_4bpp[] = {
  prg_generate_rbg,
  prg_rbg_rpmd1_2w,
  prg_rbg_xy,
  prg_rbg_overmode_repeat,
  prg_rbg_get_patternaddr,
  prg_rbg_get_pattern_data_2w,
  prg_rbg_get_charaddr,
  prg_rbg_getcolor_4bpp,
  prg_generate_rbg_end };

const GLchar * a_prg_rbg_1_2w_p1_8bpp[] = {
  prg_generate_rbg,
  prg_rbg_rpmd1_2w,
  prg_rbg_xy,
  prg_rbg_overmode_repeat,
  prg_rbg_get_patternaddr,
  prg_rbg_get_pattern_data_1w,
  prg_rbg_get_charaddr,
  prg_rbg_getcolor_8bpp,
  prg_generate_rbg_end };

const GLchar * a_prg_rbg_1_2w_p2_8bpp[] = {
  prg_generate_rbg,
  prg_rbg_rpmd1_2w,
  prg_rbg_xy,
  prg_rbg_overmode_repeat,
  prg_rbg_get_patternaddr,
  prg_rbg_get_pattern_data_2w,
  prg_rbg_get_charaddr,
  prg_rbg_getcolor_8bpp,
  prg_generate_rbg_end };


//--------------------------------------------------
// RPMD 2
const GLchar * a_prg_rbg_2_2w_bitmap[] = {
  prg_generate_rbg,
  prg_rbg_rpmd2_2w,
  prg_rbg_xy,
  prg_rbg_get_bitmap,
  prg_rbg_getcolor_8bpp,
  prg_generate_rbg_end
};

const GLchar * a_prg_rbg_2_2w_p1_4bpp[] = {
  prg_generate_rbg,
  prg_rbg_rpmd2_2w,
  prg_rbg_xy,
  prg_rbg_overmode_repeat,
  prg_rbg_get_patternaddr,
  prg_rbg_get_pattern_data_1w,
  prg_rbg_get_charaddr,
  prg_rbg_getcolor_4bpp,
  prg_generate_rbg_end };

const GLchar * a_prg_rbg_2_2w_p2_4bpp[] = {
  prg_generate_rbg,
  prg_rbg_rpmd2_2w,
  prg_rbg_xy,
  prg_rbg_overmode_repeat,
  prg_rbg_get_patternaddr,
  prg_rbg_get_pattern_data_2w,
  prg_rbg_get_charaddr,
  prg_rbg_getcolor_4bpp,
  prg_generate_rbg_end };

const GLchar * a_prg_rbg_2_2w_p1_8bpp[] = {
  prg_generate_rbg,
  prg_rbg_rpmd2_2w,
  prg_rbg_xy,
  prg_rbg_overmode_repeat,
  prg_rbg_get_patternaddr,
  prg_rbg_get_pattern_data_1w,
  prg_rbg_get_charaddr,
  prg_rbg_getcolor_8bpp,
  prg_generate_rbg_end };

const GLchar * a_prg_rbg_2_2w_p2_8bpp[] = {
  prg_generate_rbg,
  prg_rbg_rpmd2_2w,
  prg_rbg_xy,
  prg_rbg_overmode_repeat,
  prg_rbg_get_patternaddr,
  prg_rbg_get_pattern_data_2w,
  prg_rbg_get_charaddr,
  prg_rbg_getcolor_8bpp,
  prg_generate_rbg_end };

//--------------------------------------------------
// RPMD 3
const GLchar * a_prg_rbg_3_2w_bitmap[] = {
  prg_generate_rbg,
  prg_get_param_mode03,
  prg_rbg_xy,
  prg_rbg_get_bitmap,
  prg_rbg_getcolor_8bpp,
  prg_generate_rbg_end
};

const GLchar * a_prg_rbg_3_2w_p1_4bpp[] = {
  prg_generate_rbg,
  prg_get_param_mode03,
  prg_rbg_xy,
  prg_rbg_overmode_repeat,
  prg_rbg_get_patternaddr,
  prg_rbg_get_pattern_data_1w,
  prg_rbg_get_charaddr,
  prg_rbg_getcolor_4bpp,
  prg_generate_rbg_end };

const GLchar * a_prg_rbg_3_2w_p2_4bpp[] = {
  prg_generate_rbg,
  prg_get_param_mode03,
  prg_rbg_xy,
  prg_rbg_overmode_repeat,
  prg_rbg_get_patternaddr,
  prg_rbg_get_pattern_data_2w,
  prg_rbg_get_charaddr,
  prg_rbg_getcolor_4bpp,
  prg_generate_rbg_end };

const GLchar * a_prg_rbg_3_2w_p1_8bpp[] = {
  prg_generate_rbg,
  prg_get_param_mode03,
  prg_rbg_xy,
  prg_rbg_overmode_repeat,
  prg_rbg_get_patternaddr,
  prg_rbg_get_pattern_data_1w,
  prg_rbg_get_charaddr,
  prg_rbg_getcolor_8bpp,
  prg_generate_rbg_end };

const GLchar * a_prg_rbg_3_2w_p2_8bpp[] = {
  prg_generate_rbg,
  prg_get_param_mode03,
  prg_rbg_xy,
  prg_rbg_overmode_repeat,
  prg_rbg_get_patternaddr,
  prg_rbg_get_pattern_data_2w,
  prg_rbg_get_charaddr,
  prg_rbg_getcolor_8bpp,
  prg_generate_rbg_end };


struct RBGUniform {
  RBGUniform() {
    pagesize = 0;
    patternshift = 0;
    planew = 0;
    pagewh = 0;
    patterndatasize = 0;
    supplementdata = 0;
    auxmode = 0;
    patternwh = 0;
    coloroffset = 0;
    transparencyenable = 0;
    specialcolormode = 0;
    specialcolorfunction = 0;
    specialcode = 0;
    window_area_mode = 0;
    alpha_ = 0.0;
    cram_shift = 1;
    hires_shift = 0;
    specialprimode = 0;
    priority = 0;
  }
  float hres_scale;
  float vres_scale;
  int cellw;
  int cellh;
  int paladdr_;
  int pagesize;
  int patternshift;
  int planew;
  int pagewh;
  int patterndatasize;
  int supplementdata;
  int auxmode;
  int patternwh;
  unsigned int coloroffset;
  int transparencyenable;
  int specialcolormode;
  int specialcolorfunction;
  unsigned int specialcode;
  int colornumber;
  int window_area_mode;
  float alpha_;
  int cram_shift;
  int hires_shift;
  int specialprimode;
  unsigned int priority;
};

class RBGGenerator {
  GLuint prg_rbg_0_2w_bitmap_4bpp_ = 0;
  GLuint prg_rbg_0_2w_bitmap_8bpp_ = 0;
  GLuint prg_rbg_0_2w_bitmap_16bpp_p_ = 0;
  GLuint prg_rbg_0_2w_bitmap_16bpp_ = 0;
  GLuint prg_rbg_0_2w_bitmap_32bpp_ = 0;
  GLuint prg_rbg_0_2w_p1_4bpp_ = 0;
  GLuint prg_rbg_0_2w_p2_4bpp_ = 0;
  GLuint prg_rbg_0_2w_p1_8bpp_ = 0;
  GLuint prg_rbg_0_2w_p2_8bpp_ = 0;
  GLuint prg_rbg_0_2w_p1_16bpp_p_ = 0;
  GLuint prg_rbg_0_2w_p2_16bpp_p_ = 0;
  GLuint prg_rbg_0_2w_p1_16bpp_ = 0;
  GLuint prg_rbg_0_2w_p2_16bpp_ = 0;
  GLuint prg_rbg_0_2w_p1_32bpp_ = 0;
  GLuint prg_rbg_0_2w_p2_32bpp_ = 0;

  GLuint prg_rbg_1_2w_bitmap_4bpp_ = 0;
  GLuint prg_rbg_1_2w_bitmap_8bpp_ = 0;
  GLuint prg_rbg_1_2w_bitmap_16bpp_p_ = 0;
  GLuint prg_rbg_1_2w_bitmap_16bpp_ = 0;
  GLuint prg_rbg_1_2w_bitmap_32bpp_ = 0;
  GLuint prg_rbg_1_2w_p1_4bpp_ = 0;
  GLuint prg_rbg_1_2w_p2_4bpp_ = 0;
  GLuint prg_rbg_1_2w_p1_8bpp_ = 0;
  GLuint prg_rbg_1_2w_p2_8bpp_ = 0;
  GLuint prg_rbg_1_2w_p1_16bpp_p_ = 0;
  GLuint prg_rbg_1_2w_p2_16bpp_p_ = 0;
  GLuint prg_rbg_1_2w_p1_16bpp_ = 0;
  GLuint prg_rbg_1_2w_p2_16bpp_ = 0;
  GLuint prg_rbg_1_2w_p1_32bpp_ = 0;
  GLuint prg_rbg_1_2w_p2_32bpp_ = 0;

  GLuint prg_rbg_2_2w_bitmap_4bpp_ = 0;
  GLuint prg_rbg_2_2w_bitmap_8bpp_ = 0;
  GLuint prg_rbg_2_2w_bitmap_16bpp_p_ = 0;
  GLuint prg_rbg_2_2w_bitmap_16bpp_ = 0;
  GLuint prg_rbg_2_2w_bitmap_32bpp_ = 0;
  GLuint prg_rbg_2_2w_p1_4bpp_ = 0;
  GLuint prg_rbg_2_2w_p2_4bpp_ = 0;
  GLuint prg_rbg_2_2w_p1_8bpp_ = 0;
  GLuint prg_rbg_2_2w_p2_8bpp_ = 0;
  GLuint prg_rbg_2_2w_p1_16bpp_p_ = 0;
  GLuint prg_rbg_2_2w_p2_16bpp_p_ = 0;
  GLuint prg_rbg_2_2w_p1_16bpp_ = 0;
  GLuint prg_rbg_2_2w_p2_16bpp_ = 0;
  GLuint prg_rbg_2_2w_p1_32bpp_ = 0;
  GLuint prg_rbg_2_2w_p2_32bpp_ = 0;

  GLuint prg_rbg_3_2w_bitmap_4bpp_ = 0;
  GLuint prg_rbg_3_2w_bitmap_8bpp_ = 0;
  GLuint prg_rbg_3_2w_bitmap_16bpp_p_ = 0;
  GLuint prg_rbg_3_2w_bitmap_16bpp_ = 0;
  GLuint prg_rbg_3_2w_bitmap_32bpp_ = 0;
  GLuint prg_rbg_3_2w_p1_4bpp_ = 0;
  GLuint prg_rbg_3_2w_p2_4bpp_ = 0;
  GLuint prg_rbg_3_2w_p1_8bpp_ = 0;
  GLuint prg_rbg_3_2w_p2_8bpp_ = 0;
  GLuint prg_rbg_3_2w_p1_16bpp_p_ = 0;
  GLuint prg_rbg_3_2w_p2_16bpp_p_ = 0;
  GLuint prg_rbg_3_2w_p1_16bpp_ = 0;
  GLuint prg_rbg_3_2w_p2_16bpp_ = 0;
  GLuint prg_rbg_3_2w_p1_32bpp_ = 0;
  GLuint prg_rbg_3_2w_p2_32bpp_ = 0;

  GLuint prg_rbg_0_2w_bitmap_4bpp_line_ = 0;
  GLuint prg_rbg_0_2w_bitmap_8bpp_line_ = 0;
  GLuint prg_rbg_0_2w_bitmap_16bpp_p_line_ = 0;
  GLuint prg_rbg_0_2w_bitmap_16bpp_line_ = 0;
  GLuint prg_rbg_0_2w_bitmap_32bpp_line_ = 0;
  GLuint prg_rbg_0_2w_p1_4bpp_line_ = 0;
  GLuint prg_rbg_0_2w_p2_4bpp_line_ = 0;
  GLuint prg_rbg_0_2w_p1_8bpp_line_ = 0;
  GLuint prg_rbg_0_2w_p2_8bpp_line_ = 0;
  GLuint prg_rbg_0_2w_p1_16bpp_p_line_ = 0;
  GLuint prg_rbg_0_2w_p2_16bpp_p_line_ = 0;
  GLuint prg_rbg_0_2w_p1_16bpp_line_ = 0;
  GLuint prg_rbg_0_2w_p2_16bpp_line_ = 0;
  GLuint prg_rbg_0_2w_p1_32bpp_line_ = 0;
  GLuint prg_rbg_0_2w_p2_32bpp_line_ = 0;

  GLuint prg_rbg_1_2w_bitmap_4bpp_line_ = 0;
  GLuint prg_rbg_1_2w_bitmap_8bpp_line_ = 0;
  GLuint prg_rbg_1_2w_bitmap_16bpp_p_line_ = 0;
  GLuint prg_rbg_1_2w_bitmap_16bpp_line_ = 0;
  GLuint prg_rbg_1_2w_bitmap_32bpp_line_ = 0;
  GLuint prg_rbg_1_2w_p1_4bpp_line_ = 0;
  GLuint prg_rbg_1_2w_p2_4bpp_line_ = 0;
  GLuint prg_rbg_1_2w_p1_8bpp_line_ = 0;
  GLuint prg_rbg_1_2w_p2_8bpp_line_ = 0;
  GLuint prg_rbg_1_2w_p1_16bpp_p_line_ = 0;
  GLuint prg_rbg_1_2w_p2_16bpp_p_line_ = 0;
  GLuint prg_rbg_1_2w_p1_16bpp_line_ = 0;
  GLuint prg_rbg_1_2w_p2_16bpp_line_ = 0;
  GLuint prg_rbg_1_2w_p1_32bpp_line_ = 0;
  GLuint prg_rbg_1_2w_p2_32bpp_line_ = 0;

  GLuint prg_rbg_2_2w_bitmap_4bpp_line_ = 0;
  GLuint prg_rbg_2_2w_bitmap_8bpp_line_ = 0;
  GLuint prg_rbg_2_2w_bitmap_16bpp_p_line_ = 0;
  GLuint prg_rbg_2_2w_bitmap_16bpp_line_ = 0;
  GLuint prg_rbg_2_2w_bitmap_32bpp_line_ = 0;
  GLuint prg_rbg_2_2w_p1_4bpp_line_ = 0;
  GLuint prg_rbg_2_2w_p2_4bpp_line_ = 0;
  GLuint prg_rbg_2_2w_p1_8bpp_line_ = 0;
  GLuint prg_rbg_2_2w_p2_8bpp_line_ = 0;
  GLuint prg_rbg_2_2w_p1_16bpp_p_line_ = 0;
  GLuint prg_rbg_2_2w_p2_16bpp_p_line_ = 0;
  GLuint prg_rbg_2_2w_p1_16bpp_line_ = 0;
  GLuint prg_rbg_2_2w_p2_16bpp_line_ = 0;
  GLuint prg_rbg_2_2w_p1_32bpp_line_ = 0;
  GLuint prg_rbg_2_2w_p2_32bpp_line_ = 0;

  GLuint prg_rbg_3_2w_bitmap_4bpp_line_ = 0;
  GLuint prg_rbg_3_2w_bitmap_8bpp_line_ = 0;
  GLuint prg_rbg_3_2w_bitmap_16bpp_p_line_ = 0;
  GLuint prg_rbg_3_2w_bitmap_16bpp_line_ = 0;
  GLuint prg_rbg_3_2w_bitmap_32bpp_line_ = 0;
  GLuint prg_rbg_3_2w_p1_4bpp_line_ = 0;
  GLuint prg_rbg_3_2w_p2_4bpp_line_ = 0;
  GLuint prg_rbg_3_2w_p1_8bpp_line_ = 0;
  GLuint prg_rbg_3_2w_p2_8bpp_line_ = 0;
  GLuint prg_rbg_3_2w_p1_16bpp_p_line_ = 0;
  GLuint prg_rbg_3_2w_p2_16bpp_p_line_ = 0;
  GLuint prg_rbg_3_2w_p1_16bpp_line_ = 0;
  GLuint prg_rbg_3_2w_p2_16bpp_line_ = 0;
  GLuint prg_rbg_3_2w_p1_32bpp_line_ = 0;
  GLuint prg_rbg_3_2w_p2_32bpp_line_ = 0;

  GLuint tex_surface_ = 0;
  GLuint tex_surface_1 = 0;
  GLuint ssbo_vram_ = 0;
  GLuint ssbo_cram_ = 0;
  GLuint ssbo_window_ = 0;
  GLuint ssbo_paraA_ = 0;
  int tex_width_ = 0;
  int tex_height_ = 0;
  static RBGGenerator * instance_;
  GLuint scene_uniform = 0;
  RBGUniform uniform;
  int struct_size_;

  void * mapped_vram = nullptr;

protected:
  RBGGenerator() {
    tex_surface_ = 0;
    tex_width_ = 0;
    tex_height_ = 0;
    struct_size_ = sizeof(vdp2rotationparameter_struct);
    int am = sizeof(vdp2rotationparameter_struct) % 16;
    if (am != 0) {
      struct_size_ += 16 - am;
    }
  }

public:
  static RBGGenerator * getInstance() {
    if (instance_ == nullptr) {
      instance_ = new RBGGenerator();
    }
    return instance_;
  }

  void resize(int width, int height) {
    if (tex_width_ == width && tex_height_ == height) return;

    YGLDEBUG("resize %d, %d\n", width, height);

    glGetError();

    if (tex_surface_ != 0) {
      glDeleteTextures(1, &tex_surface_);
      tex_surface_ = 0;
    }

    glGenTextures(1, &tex_surface_);
    ErrorHandle("glGenTextures");

    tex_width_ = width;
    tex_height_ = height;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_surface_);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    ErrorHandle("glBindTexture");
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tex_width_, tex_height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, tex_width_, tex_height_);
    ErrorHandle("glTexStorage2D");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    ErrorHandle("glTexParameteri");

    if (tex_surface_1 != 0) {
      glDeleteTextures(1, &tex_surface_1);
      glGenTextures(1, &tex_surface_1);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, tex_surface_1);
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
      ErrorHandle("glBindTexture");
      //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tex_width_, tex_height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
      glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, tex_width_, tex_height_);
      ErrorHandle("glTexStorage2D");
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      ErrorHandle("glTexParameteri");
    }

    YGLDEBUG("resize tex_surface_=%d, tex_surface_1=%d\n", tex_surface_, tex_surface_1);

  }

  GLuint createProgram(int count, const GLchar** prg_strs) {
    GLuint result = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(result, count, prg_strs, NULL);
    glCompileShader(result);
    GLint status;
    glGetShaderiv(result, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
      GLint length;
      glGetShaderiv(result, GL_INFO_LOG_LENGTH, &length);
      GLchar *info = new GLchar[length];
      glGetShaderInfoLog(result, length, NULL, info);
      YGLDEBUG("[COMPILE] %s\n", info);
      FILE * fp = fopen("tmp.cpp", "w");
      if (fp) {
        for (int i = 0; i < count; i++) {
          fprintf(fp, "%s", prg_strs[i]);
        }
        fclose(fp);
      }
      abort();
      delete[] info;
    }
    GLuint program = glCreateProgram();
    glAttachShader(program, result);
    glLinkProgram(program);
    glDetachShader(program, result);
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
      GLint length;
      glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
      GLchar *info = new GLchar[length];
      glGetProgramInfoLog(program, length, NULL, info);
      YGLDEBUG("[LINK] %s\n", info);
      FILE * fp = fopen("tmp.cpp", "w");
      if (fp) {
        for (int i = 0; i < count; i++) {
          fprintf(fp, "%s", prg_strs[i]);
        }
        fclose(fp);
      }
      YabThreadUSleep(1000000);
      abort();
      delete[] info;
    }
    return program;
  }

  //----------------------------------------------- 
  void init(int width, int height) {

    resize(width, height);
    if (ssbo_vram_ != 0) return; // always inisialized!

    glGenBuffers(1, &ssbo_vram_);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_vram_);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 0x80000, (void*)Vdp2Ram, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &ssbo_paraA_);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_paraA_);
    glBufferData(GL_SHADER_STORAGE_BUFFER, struct_size_ * 2, NULL, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &scene_uniform);
    glBindBuffer(GL_UNIFORM_BUFFER, scene_uniform);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(RBGUniform), &uniform, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glGenBuffers(1, &ssbo_window_);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_window_);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(vdp2WindowInfo) * 512, NULL, GL_DYNAMIC_DRAW);

    //prg_rbg_0_2w_bitmap_8bpp_ = createProgram(sizeof(a_prg_rbg_0_2w_bitmap) / sizeof(char*), (const GLchar**)a_prg_rbg_0_2w_bitmap);
    //prg_rbg_0_2w_p1_4bpp_ = createProgram(sizeof(a_prg_rbg_0_2w_p1_4bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_0_2w_p1_4bpp);
    //prg_rbg_0_2w_p2_4bpp_ = createProgram(sizeof(a_prg_rbg_0_2w_p2_4bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_0_2w_p2_4bpp);
    //prg_rbg_0_2w_p1_8bpp_ = createProgram(sizeof(a_prg_rbg_0_2w_p1_8bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_0_2w_p1_8bpp);
    //prg_rbg_0_2w_p2_8bpp_ = createProgram(sizeof(a_prg_rbg_0_2w_p2_8bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_0_2w_p2_8bpp);

    //prg_rbg_1_2w_bitmap_8bpp_ = createProgram(sizeof(a_prg_rbg_1_2w_bitmap) / sizeof(char*), (const GLchar**)a_prg_rbg_1_2w_bitmap);
    //prg_rbg_1_2w_p1_4bpp_ = createProgram(sizeof(a_prg_rbg_1_2w_p1_4bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_1_2w_p1_4bpp);
    //prg_rbg_1_2w_p2_4bpp_ = createProgram(sizeof(a_prg_rbg_1_2w_p2_4bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_1_2w_p2_4bpp);
    //prg_rbg_1_2w_p1_8bpp_ = createProgram(sizeof(a_prg_rbg_1_2w_p1_8bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_1_2w_p1_8bpp);
    //prg_rbg_1_2w_p2_8bpp_ = createProgram(sizeof(a_prg_rbg_1_2w_p2_8bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_1_2w_p2_8bpp);

    //prg_rbg_2_2w_bitmap_8bpp_ = createProgram(sizeof(a_prg_rbg_2_2w_bitmap) / sizeof(char*), (const GLchar**)a_prg_rbg_2_2w_bitmap);
    //prg_rbg_2_2w_p1_4bpp_ = createProgram(sizeof(a_prg_rbg_2_2w_p1_4bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_2_2w_p1_4bpp);
    //prg_rbg_2_2w_p2_4bpp_ = createProgram(sizeof(a_prg_rbg_2_2w_p2_4bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_2_2w_p2_4bpp);
    //prg_rbg_2_2w_p1_8bpp_ = createProgram(sizeof(a_prg_rbg_2_2w_p1_8bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_2_2w_p1_8bpp);
    //prg_rbg_2_2w_p2_8bpp_ = createProgram( sizeof(a_prg_rbg_2_2w_p2_8bpp)/sizeof(char*) , (const GLchar**)a_prg_rbg_2_2w_p2_8bpp);

    //prg_rbg_3_2w_bitmap_8bpp_ = createProgram(sizeof(a_prg_rbg_3_2w_bitmap) / sizeof(char*), (const GLchar**)a_prg_rbg_3_2w_bitmap);
    //prg_rbg_3_2w_p1_4bpp_ = createProgram(sizeof(a_prg_rbg_3_2w_p1_4bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_3_2w_p1_4bpp);
    //prg_rbg_3_2w_p2_4bpp_ = createProgram(sizeof(a_prg_rbg_3_2w_p2_4bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_3_2w_p2_4bpp);
    //prg_rbg_3_2w_p1_8bpp_ = createProgram(sizeof(a_prg_rbg_3_2w_p1_8bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_3_2w_p1_8bpp);
    //prg_rbg_3_2w_p2_8bpp_ = createProgram(sizeof(a_prg_rbg_3_2w_p2_8bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_3_2w_p2_8bpp);


    //a_prg_rbg_0_2w_bitmap[sizeof(a_prg_rbg_0_2w_bitmap) / sizeof(char*) -1] = prg_generate_rbg_line_end;
    //prg_rbg_0_2w_bitmap_8bpp_line_ = createProgram(sizeof(a_prg_rbg_0_2w_bitmap) / sizeof(char*), (const GLchar**)a_prg_rbg_0_2w_bitmap);

    //a_prg_rbg_0_2w_p1_4bpp[sizeof(a_prg_rbg_0_2w_p1_4bpp) / sizeof(char*) - 1] = prg_generate_rbg_line_end;
    //prg_rbg_0_2w_p1_4bpp_line_ = createProgram(sizeof(a_prg_rbg_0_2w_p1_4bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_0_2w_p1_4bpp);

    //a_prg_rbg_0_2w_p2_4bpp[sizeof(a_prg_rbg_0_2w_p2_4bpp) / sizeof(char*) -1] = prg_generate_rbg_line_end;
    //prg_rbg_0_2w_p2_4bpp_line_ = createProgram(sizeof(a_prg_rbg_0_2w_p2_4bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_0_2w_p2_4bpp);

    //a_prg_rbg_0_2w_p1_8bpp[sizeof(a_prg_rbg_0_2w_p1_8bpp) / sizeof(char*) -1] = prg_generate_rbg_line_end;
    //prg_rbg_0_2w_p1_8bpp_line_ = createProgram(sizeof(a_prg_rbg_0_2w_p1_8bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_0_2w_p1_8bpp);

    //a_prg_rbg_0_2w_p2_8bpp[sizeof(a_prg_rbg_0_2w_p2_8bpp) / sizeof(char*) -1] = prg_generate_rbg_line_end;
    //prg_rbg_0_2w_p2_8bpp_line_ = createProgram(sizeof(a_prg_rbg_0_2w_p2_8bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_0_2w_p2_8bpp);

    //a_prg_rbg_1_2w_bitmap[sizeof(a_prg_rbg_1_2w_bitmap) / sizeof(char*) - 1] = prg_generate_rbg_line_end;
    //prg_rbg_1_2w_bitmap_8bpp_line_ = createProgram(sizeof(a_prg_rbg_1_2w_bitmap) / sizeof(char*), (const GLchar**)a_prg_rbg_1_2w_bitmap);

    //a_prg_rbg_1_2w_p1_4bpp[sizeof(a_prg_rbg_1_2w_p1_4bpp) / sizeof(char*) - 1] = prg_generate_rbg_line_end;
    //prg_rbg_1_2w_p1_4bpp_line_ = createProgram(sizeof(a_prg_rbg_1_2w_p1_4bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_1_2w_p1_4bpp);

    //a_prg_rbg_1_2w_p2_4bpp[sizeof(a_prg_rbg_1_2w_p2_4bpp) / sizeof(char*) - 1] = prg_generate_rbg_line_end;
    //prg_rbg_1_2w_p2_4bpp_line_ = createProgram(sizeof(a_prg_rbg_1_2w_p2_4bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_1_2w_p2_4bpp);

    //a_prg_rbg_1_2w_p1_8bpp[sizeof(a_prg_rbg_1_2w_p1_8bpp) / sizeof(char*) - 1] = prg_generate_rbg_line_end;
    //prg_rbg_1_2w_p1_8bpp_line_ = createProgram(sizeof(a_prg_rbg_1_2w_p1_8bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_1_2w_p1_8bpp);

    //a_prg_rbg_1_2w_p2_8bpp[sizeof(a_prg_rbg_1_2w_p2_8bpp) / sizeof(char*) - 1] = prg_generate_rbg_line_end;
    //prg_rbg_1_2w_p2_8bpp_line_ = createProgram(sizeof(a_prg_rbg_1_2w_p2_8bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_1_2w_p2_8bpp);

    //a_prg_rbg_2_2w_bitmap[sizeof(a_prg_rbg_2_2w_bitmap) / sizeof(char*) - 1] = prg_generate_rbg_line_end;
    //prg_rbg_2_2w_bitmap_8bpp_line_ = createProgram(sizeof(a_prg_rbg_2_2w_bitmap) / sizeof(char*), (const GLchar**)a_prg_rbg_2_2w_bitmap);

    //a_prg_rbg_2_2w_p1_4bpp[sizeof(a_prg_rbg_2_2w_p1_4bpp) / sizeof(char*) - 1] = prg_generate_rbg_line_end;
    //prg_rbg_2_2w_p1_4bpp_line_ = createProgram(sizeof(a_prg_rbg_2_2w_p1_4bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_2_2w_p1_4bpp);

    //a_prg_rbg_2_2w_p2_4bpp[sizeof(a_prg_rbg_2_2w_p2_4bpp) / sizeof(char*) - 1] = prg_generate_rbg_line_end;
    //prg_rbg_2_2w_p2_4bpp_line_ = createProgram(sizeof(a_prg_rbg_2_2w_p2_4bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_2_2w_p2_4bpp);

    //a_prg_rbg_2_2w_p1_8bpp[sizeof(a_prg_rbg_2_2w_p1_8bpp) / sizeof(char*) - 1] = prg_generate_rbg_line_end;
    //prg_rbg_2_2w_p1_8bpp_line_ = createProgram(sizeof(a_prg_rbg_2_2w_p1_8bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_2_2w_p1_8bpp);

    //a_prg_rbg_2_2w_p2_8bpp[sizeof(a_prg_rbg_2_2w_p2_8bpp) / sizeof(char*) - 1] = prg_generate_rbg_line_end;
    //prg_rbg_2_2w_p2_8bpp_line_ = createProgram(sizeof(a_prg_rbg_2_2w_p2_8bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_2_2w_p2_8bpp);

    //a_prg_rbg_3_2w_bitmap[sizeof(a_prg_rbg_3_2w_bitmap) / sizeof(char*) - 1] = prg_generate_rbg_line_end;
    //prg_rbg_3_2w_bitmap_8bpp_line_ = createProgram(sizeof(a_prg_rbg_3_2w_bitmap) / sizeof(char*), (const GLchar**)a_prg_rbg_3_2w_bitmap);

    //a_prg_rbg_3_2w_p1_4bpp[sizeof(a_prg_rbg_3_2w_p1_4bpp) / sizeof(char*) - 1] = prg_generate_rbg_line_end;
    //prg_rbg_3_2w_p1_4bpp_line_ = createProgram(sizeof(a_prg_rbg_3_2w_p1_4bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_3_2w_p1_4bpp);

    //a_prg_rbg_3_2w_p2_4bpp[sizeof(a_prg_rbg_3_2w_p2_4bpp) / sizeof(char*) - 1] = prg_generate_rbg_line_end;
    //prg_rbg_3_2w_p2_4bpp_line_ = createProgram(sizeof(a_prg_rbg_3_2w_p2_4bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_3_2w_p2_4bpp);

    //a_prg_rbg_3_2w_p1_8bpp[sizeof(a_prg_rbg_3_2w_p1_8bpp) / sizeof(char*) - 1] = prg_generate_rbg_line_end;
    //prg_rbg_3_2w_p1_8bpp_line_ = createProgram(sizeof(a_prg_rbg_3_2w_p1_8bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_3_2w_p1_8bpp);

    //a_prg_rbg_3_2w_p2_8bpp[sizeof(a_prg_rbg_3_2w_p2_8bpp) / sizeof(char*) - 1] = prg_generate_rbg_line_end;
    //prg_rbg_3_2w_p2_8bpp_line_ = createProgram(sizeof(a_prg_rbg_3_2w_p2_8bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_3_2w_p2_8bpp);

  }

  bool ErrorHandle(const char* name)
  {
    GLenum   error_code = glGetError();
    if (error_code == GL_NO_ERROR) {
      return  true;
    }
    do {
      const char* msg = "";
      switch (error_code) {
      case GL_INVALID_ENUM:      msg = "INVALID_ENUM";      break;
      case GL_INVALID_VALUE:     msg = "INVALID_VALUE";     break;
      case GL_INVALID_OPERATION: msg = "INVALID_OPERATION"; break;
      case GL_OUT_OF_MEMORY:     msg = "OUT_OF_MEMORY";     break;
      case GL_INVALID_FRAMEBUFFER_OPERATION:  msg = "INVALID_FRAMEBUFFER_OPERATION"; break;
      default:  msg = "Unknown"; break;
      }
      YGLDEBUG("GLErrorLayer:ERROR:%04x'%s' %s\n", error_code, msg, name);
      abort();
      error_code = glGetError();
    } while (error_code != GL_NO_ERROR);
    return  false;
  }

  template<typename T>
  T Add(T a, T b) {
    return a + b;
  }


  //#define COMPILE_COLOR_DOT( BASE, COLOR , DOT ) ({ GLuint PRG; BASE[sizeof(BASE)/sizeof(char*) - 2] = COLOR; BASE[sizeof(BASE)/sizeof(char*) - 1] = DOT; PRG=createProgram(sizeof(BASE)/sizeof(char*), (const GLchar**)BASE);}) 

#define COMPILE_COLOR_DOT( BASE, COLOR , DOT )
#define S(A) A, sizeof(A)/sizeof(char*)

  GLuint compile_color_dot(const char * base[], int size, const char * color, const char * dot) {
    base[size - 2] = color;
    base[size - 1] = dot;
    return createProgram(size, (const GLchar**)base);
  }

  //-----------------------------------------------
  void update(RBGDrawInfo * rbg) {
    GLuint error;
    int local_size_x = 4;
    int local_size_y = 4;

    int work_groups_x = 1 + (tex_width_ - 1) / local_size_x;
    int work_groups_y = 1 + (tex_height_ - 1) / local_size_y;

    error = glGetError();
    // Line color insersion
    if (rbg->info.LineColorBase != 0 && VDP2_CC_NONE != (rbg->info.blendmode & 0x03)) {
      if (fixVdp2Regs->RPMD == 0 || (fixVdp2Regs->RPMD == 3 && (fixVdp2Regs->WCTLD & 0xA) == 0)) {
        if (rbg->info.isbitmap) {
          if (prg_rbg_0_2w_bitmap_8bpp_line_ == 0) {
            prg_rbg_0_2w_bitmap_8bpp_line_ = compile_color_dot(
              S(a_prg_rbg_0_2w_bitmap),
              prg_rbg_getcolor_8bpp,
              prg_generate_rbg_line_end);
          }
          glUseProgram(prg_rbg_0_2w_bitmap_8bpp_line_);
        }
        else {
          if (rbg->info.patterndatasize == 1) {
            switch (rbg->info.colornumber) {
            case 0: { // Decathalete ToDo: Line Color Bug
              if (prg_rbg_0_2w_p1_4bpp_line_ == 0) {
                prg_rbg_0_2w_p1_4bpp_line_ = compile_color_dot(
                  S(a_prg_rbg_0_2w_p1_4bpp),
                  prg_rbg_getcolor_4bpp,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_0_2w_p1_4bpp_line_);
              break;
            }
            case 1: { // Sakatuku 2 Ground, GUNDAM Side Story 2, SonicR ToDo: 2Player
              if (prg_rbg_0_2w_p1_8bpp_line_ == 0) {
                prg_rbg_0_2w_p1_8bpp_line_ = compile_color_dot(
                  S(a_prg_rbg_0_2w_p1_8bpp),
                  prg_rbg_getcolor_8bpp,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_0_2w_p1_8bpp_line_);
              break;
            }
            case 2: {
              if (prg_rbg_0_2w_p1_16bpp_p_line_ == 0) {
                prg_rbg_0_2w_p1_16bpp_p_line_ = compile_color_dot(
                  S(a_prg_rbg_0_2w_p1_4bpp),
                  prg_rbg_getcolor_16bpp_palette,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_0_2w_p1_16bpp_p_line_);
              break;
            }
            case 3: {
              if (prg_rbg_0_2w_p1_16bpp_line_ == 0) {
                prg_rbg_0_2w_p1_16bpp_line_ = compile_color_dot(
                  S(a_prg_rbg_0_2w_p1_4bpp),
                  prg_rbg_getcolor_16bpp_rbg,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_0_2w_p1_16bpp_line_);
              break;
            }
            case 4: {
              if (prg_rbg_0_2w_p1_32bpp_line_ == 0) {
                prg_rbg_0_2w_p1_32bpp_line_ = compile_color_dot(
                  S(a_prg_rbg_0_2w_p1_4bpp),
                  prg_rbg_getcolor_32bpp_rbg,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_0_2w_p1_32bpp_line_);
              break;
            }
            }
          }
          else {
            switch (rbg->info.colornumber) {
            case 0: {
              if (prg_rbg_0_2w_p2_4bpp_line_ == 0) {
                prg_rbg_0_2w_p2_4bpp_line_ = compile_color_dot(
                  S(a_prg_rbg_0_2w_p2_4bpp),
                  prg_rbg_getcolor_4bpp,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_0_2w_p2_4bpp_line_);
              break;
            }
            case 1: { // Thunder Force V
              if (prg_rbg_0_2w_p2_8bpp_line_ == 0) {
                prg_rbg_0_2w_p2_8bpp_line_ = compile_color_dot(
                  S(a_prg_rbg_0_2w_p2_8bpp),
                  prg_rbg_getcolor_8bpp,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_0_2w_p2_8bpp_line_);
              break;
            }
            case 2: {
              if (prg_rbg_0_2w_p2_16bpp_p_line_ == 0) {
                prg_rbg_0_2w_p2_16bpp_p_line_ = compile_color_dot(
                  S(a_prg_rbg_0_2w_p2_4bpp),
                  prg_rbg_getcolor_16bpp_palette,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_0_2w_p2_16bpp_p_line_);
              break;
            }
            case 3: {
              if (prg_rbg_0_2w_p2_16bpp_line_ == 0) {
                prg_rbg_0_2w_p2_16bpp_line_ = compile_color_dot(
                  S(a_prg_rbg_0_2w_p2_4bpp),
                  prg_rbg_getcolor_16bpp_rbg,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_0_2w_p2_16bpp_line_);
              break;
            }
            case 4: {
              if (prg_rbg_0_2w_p2_32bpp_line_ == 0) {
                prg_rbg_0_2w_p2_32bpp_line_ = compile_color_dot(
                  S(a_prg_rbg_0_2w_p2_4bpp),
                  prg_rbg_getcolor_32bpp_rbg,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_0_2w_p2_32bpp_line_);
              break;
            }
            }
          }
        }
      }
      else if (fixVdp2Regs->RPMD == 1) {
        if (rbg->info.isbitmap) {
          if (prg_rbg_1_2w_bitmap_8bpp_line_ == 0) {
            prg_rbg_1_2w_bitmap_8bpp_line_ = compile_color_dot(
              S(a_prg_rbg_1_2w_bitmap),
              prg_rbg_getcolor_8bpp,
              prg_generate_rbg_line_end);
          }
          glUseProgram(prg_rbg_1_2w_bitmap_8bpp_line_);
        }
        else {
          if (rbg->info.patterndatasize == 1) {
            switch (rbg->info.colornumber) {
            case 0: {
              if (prg_rbg_1_2w_p1_4bpp_line_ == 0) {
                prg_rbg_1_2w_p1_4bpp_line_ = compile_color_dot(
                  S(a_prg_rbg_1_2w_p1_4bpp),
                  prg_rbg_getcolor_4bpp,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_1_2w_p1_4bpp_line_);
              break;
            }
            case 1: {
              if (prg_rbg_1_2w_p1_8bpp_line_ == 0) {
                prg_rbg_1_2w_p1_8bpp_line_ = compile_color_dot(
                  S(a_prg_rbg_1_2w_p1_8bpp),
                  prg_rbg_getcolor_8bpp,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_1_2w_p1_8bpp_line_);
              break;
            }
            case 2: {
              if (prg_rbg_1_2w_p1_16bpp_p_line_ == 0) {
                prg_rbg_1_2w_p1_16bpp_p_line_ = compile_color_dot(
                  S(a_prg_rbg_1_2w_p1_4bpp),
                  prg_rbg_getcolor_16bpp_palette,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_1_2w_p1_16bpp_p_line_);
              break;
            }
            case 3: {
              if (prg_rbg_1_2w_p1_16bpp_line_ == 0) {
                prg_rbg_1_2w_p1_16bpp_line_ = compile_color_dot(
                  S(a_prg_rbg_1_2w_p1_4bpp),
                  prg_rbg_getcolor_16bpp_rbg,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_1_2w_p1_16bpp_line_);
              break;
            }
            case 4: {
              if (prg_rbg_1_2w_p1_32bpp_line_ == 0) {
                prg_rbg_1_2w_p1_32bpp_line_ = compile_color_dot(
                  S(a_prg_rbg_0_2w_p1_4bpp),
                  prg_rbg_getcolor_32bpp_rbg,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_1_2w_p1_32bpp_line_);
              break;
            }
            }
          }
          else {
            switch (rbg->info.colornumber) {
            case 0: {
              if (prg_rbg_1_2w_p2_4bpp_line_ == 0) {
                prg_rbg_1_2w_p2_4bpp_line_ = compile_color_dot(
                  S(a_prg_rbg_1_2w_p2_4bpp),
                  prg_rbg_getcolor_4bpp,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_1_2w_p2_4bpp_line_);
              break;
            }
            case 1: {
              if (prg_rbg_1_2w_p2_8bpp_line_ == 0) {
                prg_rbg_1_2w_p2_8bpp_line_ = compile_color_dot(
                  S(a_prg_rbg_1_2w_p2_8bpp),
                  prg_rbg_getcolor_8bpp,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_1_2w_p2_8bpp_line_);
              break;
            }
            case 2: {
              if (prg_rbg_1_2w_p2_16bpp_p_line_ == 0) {
                prg_rbg_1_2w_p2_16bpp_p_line_ = compile_color_dot(
                  S(a_prg_rbg_1_2w_p2_4bpp),
                  prg_rbg_getcolor_16bpp_palette,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_1_2w_p2_16bpp_p_line_);
              break;
            }
            case 3: {
              if (prg_rbg_1_2w_p2_16bpp_line_ == 0) {
                prg_rbg_1_2w_p2_16bpp_line_ = compile_color_dot(
                  S(a_prg_rbg_1_2w_p2_4bpp),
                  prg_rbg_getcolor_16bpp_rbg,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_1_2w_p2_16bpp_line_);
              break;
            }
            case 4: {
              if (prg_rbg_1_2w_p2_32bpp_line_ == 0) {
                prg_rbg_1_2w_p2_32bpp_line_ = compile_color_dot(
                  S(a_prg_rbg_1_2w_p2_4bpp),
                  prg_rbg_getcolor_32bpp_rbg,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_1_2w_p2_32bpp_line_);
              break;
            }
            }
          }
        }
      }
      else if (fixVdp2Regs->RPMD == 2) {
        if (rbg->info.isbitmap) {
          if (prg_rbg_2_2w_bitmap_8bpp_line_ == 0) {
            prg_rbg_2_2w_bitmap_8bpp_line_ = compile_color_dot(
              S(a_prg_rbg_2_2w_bitmap),
              prg_rbg_getcolor_8bpp,
              prg_generate_rbg_line_end);
          }
          glUseProgram(prg_rbg_2_2w_bitmap_8bpp_line_);
        }
        else {
          if (rbg->info.patterndatasize == 1) {
            switch (rbg->info.colornumber) {
            case 0: {
              if (prg_rbg_2_2w_p1_4bpp_line_ == 0) {
                prg_rbg_2_2w_p1_4bpp_line_ = compile_color_dot(
                  S(a_prg_rbg_2_2w_p1_4bpp),
                  prg_rbg_getcolor_4bpp,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_2_2w_p1_4bpp_line_);
              break;
            }
            case 1: { // Panzer Dragoon 1
              if (prg_rbg_2_2w_p1_8bpp_line_ == 0) {
                prg_rbg_2_2w_p1_8bpp_line_ = compile_color_dot(
                  S(a_prg_rbg_2_2w_p1_8bpp),
                  prg_rbg_getcolor_8bpp,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_2_2w_p1_8bpp_line_);
              break;
            }
            case 2: {
              if (prg_rbg_2_2w_p1_16bpp_p_line_ == 0) {
                prg_rbg_2_2w_p1_16bpp_p_line_ = compile_color_dot(
                  S(a_prg_rbg_2_2w_p1_4bpp),
                  prg_rbg_getcolor_16bpp_palette,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_2_2w_p1_16bpp_p_line_);
              break;
            }
            case 3: {
              if (prg_rbg_2_2w_p1_16bpp_line_ == 0) {
                prg_rbg_2_2w_p1_16bpp_line_ = compile_color_dot(
                  S(a_prg_rbg_2_2w_p1_4bpp),
                  prg_rbg_getcolor_16bpp_rbg,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_2_2w_p1_16bpp_line_);
              break;
            }
            case 4: {
              if (prg_rbg_2_2w_p1_32bpp_line_ == 0) {
                prg_rbg_2_2w_p1_32bpp_line_ = compile_color_dot(
                  S(a_prg_rbg_2_2w_p1_4bpp),
                  prg_rbg_getcolor_32bpp_rbg,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_2_2w_p1_32bpp_line_);
              break;
            }
            }
          }
          else {
            switch (rbg->info.colornumber) {
            case 0: {
              if (prg_rbg_2_2w_p2_4bpp_line_ == 0) {
                prg_rbg_2_2w_p2_4bpp_line_ = compile_color_dot(
                  S(a_prg_rbg_2_2w_p2_4bpp),
                  prg_rbg_getcolor_4bpp,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_2_2w_p2_4bpp_line_);
              break;
            }
            case 1: {
              if (prg_rbg_2_2w_p2_8bpp_line_ == 0) {
                prg_rbg_2_2w_p2_8bpp_line_ = compile_color_dot(
                  S(a_prg_rbg_2_2w_p2_8bpp),
                  prg_rbg_getcolor_8bpp,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_2_2w_p2_8bpp_line_);
              break;
            }
            case 2: {
              if (prg_rbg_2_2w_p2_16bpp_p_line_ == 0) {
                prg_rbg_2_2w_p2_16bpp_p_line_ = compile_color_dot(
                  S(a_prg_rbg_2_2w_p2_4bpp),
                  prg_rbg_getcolor_16bpp_palette,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_2_2w_p2_16bpp_p_line_);
              break;
            }
            case 3: {
              if (prg_rbg_2_2w_p2_16bpp_line_ == 0) {
                prg_rbg_2_2w_p2_16bpp_line_ = compile_color_dot(
                  S(a_prg_rbg_2_2w_p2_4bpp),
                  prg_rbg_getcolor_16bpp_rbg,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_2_2w_p2_16bpp_line_);
              break;
            }
            case 4: {
              if (prg_rbg_2_2w_p2_32bpp_line_ == 0) {
                prg_rbg_2_2w_p2_32bpp_line_ = compile_color_dot(
                  S(a_prg_rbg_2_2w_p2_4bpp),
                  prg_rbg_getcolor_32bpp_rbg,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_2_2w_p2_32bpp_line_);
              break;
            }
            }
          }
        }
      }
      else if (fixVdp2Regs->RPMD == 3) {

        if (rbg->info.isbitmap) {
          if (prg_rbg_3_2w_bitmap_8bpp_line_ == 0) {
            prg_rbg_3_2w_bitmap_8bpp_line_ = compile_color_dot(
              S(a_prg_rbg_3_2w_bitmap),
              prg_rbg_getcolor_8bpp,
              prg_generate_rbg_line_end);
          }
          glUseProgram(prg_rbg_3_2w_bitmap_8bpp_line_);
        }
        else {
          if (rbg->info.patterndatasize == 1) {
            switch (rbg->info.colornumber) {
            case 0: {
              if (prg_rbg_3_2w_p1_4bpp_line_ == 0) {
                prg_rbg_3_2w_p1_4bpp_line_ = compile_color_dot(
                  S(a_prg_rbg_3_2w_p1_4bpp),
                  prg_rbg_getcolor_4bpp,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_3_2w_p1_4bpp_line_);
              break;
            }
            case 1: {
              if (prg_rbg_3_2w_p1_8bpp_line_ == 0) {
                prg_rbg_3_2w_p1_8bpp_line_ = compile_color_dot(
                  S(a_prg_rbg_3_2w_p1_8bpp),
                  prg_rbg_getcolor_8bpp,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_3_2w_p1_8bpp_line_);
              break;
            }
            case 2: {
              if (prg_rbg_3_2w_p1_16bpp_p_line_ == 0) {
                prg_rbg_3_2w_p1_16bpp_p_line_ = compile_color_dot(
                  S(a_prg_rbg_3_2w_p1_4bpp),
                  prg_rbg_getcolor_16bpp_palette,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_3_2w_p1_16bpp_p_line_);
              break;
            }
            case 3: {
              if (prg_rbg_3_2w_p1_16bpp_line_ == 0) {
                prg_rbg_3_2w_p1_16bpp_line_ = compile_color_dot(
                  S(a_prg_rbg_3_2w_p1_4bpp),
                  prg_rbg_getcolor_16bpp_rbg,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_3_2w_p1_16bpp_line_);
              break;
            }
            case 4: {
              if (prg_rbg_3_2w_p1_32bpp_line_ == 0) {
                prg_rbg_3_2w_p1_32bpp_line_ = compile_color_dot(
                  S(a_prg_rbg_3_2w_p1_4bpp),
                  prg_rbg_getcolor_32bpp_rbg,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_3_2w_p1_32bpp_line_);
              break;
            }
            }
          }
          else {
            switch (rbg->info.colornumber) {
            case 0: {
              if (prg_rbg_3_2w_p2_4bpp_line_ == 0) {
                prg_rbg_3_2w_p2_4bpp_line_ = compile_color_dot(
                  S(a_prg_rbg_3_2w_p2_4bpp),
                  prg_rbg_getcolor_4bpp,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_3_2w_p2_4bpp_line_);
              break;
            }
            case 1: {
              if (prg_rbg_3_2w_p2_8bpp_line_ == 0) {
                prg_rbg_3_2w_p2_8bpp_line_ = compile_color_dot(
                  S(a_prg_rbg_3_2w_p2_8bpp),
                  prg_rbg_getcolor_8bpp,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_3_2w_p2_8bpp_line_);
              break;
            }
            case 2: {
              if (prg_rbg_3_2w_p2_16bpp_p_line_ == 0) {
                prg_rbg_3_2w_p2_16bpp_p_line_ = compile_color_dot(
                  S(a_prg_rbg_3_2w_p2_4bpp),
                  prg_rbg_getcolor_16bpp_palette,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_3_2w_p2_16bpp_p_line_);
              break;
            }
            case 3: {
              if (prg_rbg_3_2w_p2_16bpp_line_ == 0) {
                prg_rbg_3_2w_p2_16bpp_line_ = compile_color_dot(
                  S(a_prg_rbg_3_2w_p2_4bpp),
                  prg_rbg_getcolor_16bpp_rbg,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_3_2w_p2_16bpp_line_);
              break;
            }
            case 4: {
              if (prg_rbg_3_2w_p2_32bpp_line_ == 0) {
                prg_rbg_3_2w_p2_32bpp_line_ = compile_color_dot(
                  S(a_prg_rbg_3_2w_p2_4bpp),
                  prg_rbg_getcolor_32bpp_rbg,
                  prg_generate_rbg_line_end);
              }
              glUseProgram(prg_rbg_3_2w_p2_32bpp_line_);
              break;
            }
            }
          }
        }
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_window_);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(vdp2WindowInfo)*(int(rbg->vres / rbg->rotate_mval_v) << rbg->info.hres_shift), (void*)rbg->info.pWinInfo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo_window_);
      }

    }

    // no line color insersion
    else {
      if (rbg->rgb_type == 0 && (fixVdp2Regs->RPMD == 0 || (fixVdp2Regs->RPMD == 3 && (fixVdp2Regs->WCTLD & 0xA) == 0))) {
        if (rbg->info.isbitmap) {
          switch (rbg->info.colornumber) {
          case 0: {
            if (prg_rbg_0_2w_bitmap_4bpp_ == 0) {
              prg_rbg_0_2w_bitmap_4bpp_ = compile_color_dot(
                S(a_prg_rbg_0_2w_bitmap),
                prg_rbg_getcolor_4bpp,
                prg_generate_rbg_end);
            }
            glUseProgram(prg_rbg_0_2w_bitmap_4bpp_);
            break;
          }
          case 1: { // SF3S1( Initial )
            if (prg_rbg_0_2w_bitmap_8bpp_ == 0) {
              prg_rbg_0_2w_bitmap_8bpp_ = compile_color_dot(
                S(a_prg_rbg_0_2w_bitmap),
                prg_rbg_getcolor_8bpp,
                prg_generate_rbg_end);
            }
            glUseProgram(prg_rbg_0_2w_bitmap_8bpp_);
            break;
          }
          case 2: {
            if (prg_rbg_0_2w_bitmap_16bpp_p_ == 0) {
              prg_rbg_0_2w_bitmap_16bpp_p_ = compile_color_dot(
                S(a_prg_rbg_0_2w_bitmap),
                prg_rbg_getcolor_16bpp_palette,
                prg_generate_rbg_end);
            }
            glUseProgram(prg_rbg_0_2w_bitmap_16bpp_p_);
            break;
          }
          case 3: { // NHL 97 Title, GRANDIA Title
            if (prg_rbg_0_2w_bitmap_16bpp_ == 0) {
              prg_rbg_0_2w_bitmap_16bpp_ = compile_color_dot(
                S(a_prg_rbg_0_2w_bitmap),
                prg_rbg_getcolor_16bpp_rbg,
                prg_generate_rbg_end);
            }
            glUseProgram(prg_rbg_0_2w_bitmap_16bpp_);
            break;
          }
          case 4: {
            if (prg_rbg_0_2w_bitmap_32bpp_ == 0) {
              prg_rbg_0_2w_bitmap_32bpp_ = compile_color_dot(
                S(a_prg_rbg_0_2w_bitmap),
                prg_rbg_getcolor_32bpp_rbg,
                prg_generate_rbg_end);
            }
            glUseProgram(prg_rbg_0_2w_bitmap_32bpp_);
            break;
          }
          }
        }
        else {
          if (rbg->info.patterndatasize == 1) {
            switch (rbg->info.colornumber) {
            case 0: { // Dead or Alive, Rediant Silver Gun, Diehard
              if (prg_rbg_0_2w_p1_4bpp_ == 0) {
                prg_rbg_0_2w_p1_4bpp_ = compile_color_dot(
                  S(a_prg_rbg_0_2w_p1_4bpp),
                  prg_rbg_getcolor_4bpp,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_0_2w_p1_4bpp_);
              break;
            }
            case 1: { // Sakatuku 2 ( Initial Setting ), Virtua Fighter 2, Virtual-on
              if (prg_rbg_0_2w_p1_8bpp_ == 0) {
                prg_rbg_0_2w_p1_8bpp_ = compile_color_dot(
                  S(a_prg_rbg_0_2w_p1_8bpp),
                  prg_rbg_getcolor_8bpp,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_0_2w_p1_8bpp_);
              break;
            }
            case 2: {
              if (prg_rbg_0_2w_p1_16bpp_p_ == 0) {
                prg_rbg_0_2w_p1_16bpp_p_ = compile_color_dot(
                  S(a_prg_rbg_0_2w_p1_4bpp),
                  prg_rbg_getcolor_16bpp_palette,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_0_2w_p1_16bpp_p_);
              break;
            }
            case 3: {
              if (prg_rbg_0_2w_p1_16bpp_ == 0) {
                prg_rbg_0_2w_p1_16bpp_ = compile_color_dot(
                  S(a_prg_rbg_0_2w_p1_4bpp),
                  prg_rbg_getcolor_16bpp_rbg,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_0_2w_p1_16bpp_);
              break;
            }
            case 4: {
              if (prg_rbg_0_2w_p1_32bpp_ == 0) {
                prg_rbg_0_2w_p1_32bpp_ = compile_color_dot(
                  S(a_prg_rbg_0_2w_p1_4bpp),
                  prg_rbg_getcolor_32bpp_rbg,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_0_2w_p1_32bpp_);
              break;
            }
            }
          }
          else {
            switch (rbg->info.colornumber) {
            case 0: {
              if (prg_rbg_0_2w_p2_4bpp_ == 0) {
                prg_rbg_0_2w_p2_4bpp_ = compile_color_dot(
                  S(a_prg_rbg_0_2w_p2_4bpp),
                  prg_rbg_getcolor_4bpp,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_0_2w_p2_4bpp_);
              break;
            }
            case 1: { // NHL97(In Game), BIOS 
              if (prg_rbg_0_2w_p2_8bpp_ == 0) {
                prg_rbg_0_2w_p2_8bpp_ = compile_color_dot(
                  S(a_prg_rbg_0_2w_p2_8bpp),
                  prg_rbg_getcolor_8bpp,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_0_2w_p2_8bpp_);
              break;
            }
            case 2: {
              if (prg_rbg_0_2w_p2_16bpp_p_ == 0) {
                prg_rbg_0_2w_p2_16bpp_p_ = compile_color_dot(
                  S(a_prg_rbg_0_2w_p2_4bpp),
                  prg_rbg_getcolor_16bpp_palette,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_0_2w_p2_16bpp_p_);
              break;
            }
            case 3: {
              if (prg_rbg_0_2w_p2_16bpp_ == 0) {
                prg_rbg_0_2w_p2_16bpp_ = compile_color_dot(
                  S(a_prg_rbg_0_2w_p2_4bpp),
                  prg_rbg_getcolor_16bpp_rbg,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_0_2w_p2_16bpp_);
              break;
            }
            case 4: {
              if (prg_rbg_0_2w_p2_32bpp_ == 0) {
                prg_rbg_0_2w_p2_32bpp_ = compile_color_dot(
                  S(a_prg_rbg_0_2w_p2_4bpp),
                  prg_rbg_getcolor_32bpp_rbg,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_0_2w_p2_32bpp_);
              break;
            }
            }
          }
        }
      }
      else if ((fixVdp2Regs->RPMD == 1 && rbg->rgb_type == 0) || rbg->rgb_type == 0x04) {
        if (rbg->info.isbitmap) {
          switch (rbg->info.colornumber) {
          case 0: {
            if (prg_rbg_1_2w_bitmap_4bpp_ == 0) {
              prg_rbg_1_2w_bitmap_4bpp_ = compile_color_dot(
                S(a_prg_rbg_1_2w_bitmap),
                prg_rbg_getcolor_4bpp,
                prg_generate_rbg_end);
            }
            glUseProgram(prg_rbg_1_2w_bitmap_4bpp_);
            break;
          }
          case 1: {
            if (prg_rbg_1_2w_bitmap_8bpp_ == 0) {
              prg_rbg_1_2w_bitmap_8bpp_ = compile_color_dot(
                S(a_prg_rbg_1_2w_bitmap),
                prg_rbg_getcolor_8bpp,
                prg_generate_rbg_end);
            }
            glUseProgram(prg_rbg_1_2w_bitmap_8bpp_);
            break;
          }
          case 2: {
            if (prg_rbg_1_2w_bitmap_16bpp_p_ == 0) {
              prg_rbg_1_2w_bitmap_16bpp_p_ = compile_color_dot(
                S(a_prg_rbg_1_2w_bitmap),
                prg_rbg_getcolor_16bpp_palette,
                prg_generate_rbg_end);
            }
            glUseProgram(prg_rbg_1_2w_bitmap_16bpp_p_);
            break;
          }
          case 3: {
            if (prg_rbg_1_2w_bitmap_16bpp_ == 0) {
              prg_rbg_1_2w_bitmap_16bpp_ = compile_color_dot(
                S(a_prg_rbg_1_2w_bitmap),
                prg_rbg_getcolor_16bpp_rbg,
                prg_generate_rbg_end);
            }
            glUseProgram(prg_rbg_1_2w_bitmap_16bpp_);
            break;
          }
          case 4: {
            if (prg_rbg_1_2w_bitmap_32bpp_ == 0) {
              prg_rbg_1_2w_bitmap_32bpp_ = compile_color_dot(
                S(a_prg_rbg_1_2w_bitmap),
                prg_rbg_getcolor_32bpp_rbg,
                prg_generate_rbg_end);
            }
            glUseProgram(prg_rbg_1_2w_bitmap_32bpp_);
            break;
          }
          }
        }
        else {
          if (rbg->info.patterndatasize == 1) {
            switch (rbg->info.colornumber) {
            case 0: {
              if (prg_rbg_1_2w_p1_4bpp_ == 0) {
                prg_rbg_1_2w_p1_4bpp_ = compile_color_dot(
                  S(a_prg_rbg_1_2w_p1_4bpp),
                  prg_rbg_getcolor_4bpp,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_1_2w_p1_4bpp_);
              break;
            }
            case 1: {
              if (prg_rbg_1_2w_p1_8bpp_ == 0) {
                prg_rbg_1_2w_p1_8bpp_ = compile_color_dot(
                  S(a_prg_rbg_1_2w_p1_8bpp),
                  prg_rbg_getcolor_8bpp,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_1_2w_p1_8bpp_);
              break;
            }
            case 2: {
              if (prg_rbg_1_2w_p1_16bpp_p_ == 0) {
                prg_rbg_1_2w_p1_16bpp_p_ = compile_color_dot(
                  S(a_prg_rbg_1_2w_p1_4bpp),
                  prg_rbg_getcolor_16bpp_palette,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_1_2w_p1_16bpp_p_);
              break;
            }
            case 3: {
              if (prg_rbg_1_2w_p1_16bpp_ == 0) {
                prg_rbg_1_2w_p1_16bpp_ = compile_color_dot(
                  S(a_prg_rbg_1_2w_p1_4bpp),
                  prg_rbg_getcolor_16bpp_rbg,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_1_2w_p1_16bpp_);
              break;
            }
            case 4: {
              if (prg_rbg_1_2w_p1_32bpp_ == 0) {
                prg_rbg_1_2w_p1_32bpp_ = compile_color_dot(
                  S(a_prg_rbg_0_2w_p1_4bpp),
                  prg_rbg_getcolor_32bpp_rbg,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_1_2w_p1_32bpp_);
              break;
            }
            }
          }
          else {
            switch (rbg->info.colornumber) {
            case 0: {
              if (prg_rbg_1_2w_p2_4bpp_ == 0) {
                prg_rbg_1_2w_p2_4bpp_ = compile_color_dot(
                  S(a_prg_rbg_1_2w_p2_4bpp),
                  prg_rbg_getcolor_4bpp,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_1_2w_p2_4bpp_);
              break;
            }
            case 1: {
              if (prg_rbg_1_2w_p2_8bpp_ == 0) {
                prg_rbg_1_2w_p2_8bpp_ = compile_color_dot(
                  S(a_prg_rbg_1_2w_p2_8bpp),
                  prg_rbg_getcolor_8bpp,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_1_2w_p2_8bpp_);
              break;
            }
            case 2: {
              if (prg_rbg_1_2w_p2_16bpp_p_ == 0) {
                prg_rbg_1_2w_p2_16bpp_p_ = compile_color_dot(
                  S(a_prg_rbg_1_2w_p2_4bpp),
                  prg_rbg_getcolor_16bpp_palette,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_1_2w_p2_16bpp_p_);
              break;
            }
            case 3: {
              if (prg_rbg_1_2w_p2_16bpp_ == 0) {
                prg_rbg_1_2w_p2_16bpp_ = compile_color_dot(
                  S(a_prg_rbg_1_2w_p2_4bpp),
                  prg_rbg_getcolor_16bpp_rbg,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_1_2w_p2_16bpp_);
              break;
            }
            case 4: {
              if (prg_rbg_1_2w_p2_32bpp_ == 0) {
                prg_rbg_1_2w_p2_32bpp_ = compile_color_dot(
                  S(a_prg_rbg_1_2w_p2_4bpp),
                  prg_rbg_getcolor_32bpp_rbg,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_1_2w_p2_32bpp_);
              break;
            }
            }
          }
        }
      }
      else if (fixVdp2Regs->RPMD == 2) {
        if (rbg->info.isbitmap) {
          switch (rbg->info.colornumber) {
          case 0: {
            if (prg_rbg_2_2w_bitmap_4bpp_ == 0) {
              prg_rbg_2_2w_bitmap_4bpp_ = compile_color_dot(
                S(a_prg_rbg_2_2w_bitmap),
                prg_rbg_getcolor_4bpp,
                prg_generate_rbg_end);
            }
            glUseProgram(prg_rbg_2_2w_bitmap_4bpp_);
            break;
          }
          case 1: {
            if (prg_rbg_2_2w_bitmap_8bpp_ == 0) {
              prg_rbg_2_2w_bitmap_8bpp_ = compile_color_dot(
                S(a_prg_rbg_2_2w_bitmap),
                prg_rbg_getcolor_8bpp,
                prg_generate_rbg_end);
            }
            glUseProgram(prg_rbg_2_2w_bitmap_8bpp_);
            break;
          }
          case 2: {
            if (prg_rbg_2_2w_bitmap_16bpp_p_ == 0) {
              prg_rbg_2_2w_bitmap_16bpp_p_ = compile_color_dot(
                S(a_prg_rbg_2_2w_bitmap),
                prg_rbg_getcolor_16bpp_palette,
                prg_generate_rbg_end);
            }
            glUseProgram(prg_rbg_2_2w_bitmap_16bpp_p_);
            break;
          }
          case 3: {
            if (prg_rbg_2_2w_bitmap_16bpp_ == 0) {
              prg_rbg_2_2w_bitmap_16bpp_ = compile_color_dot(
                S(a_prg_rbg_2_2w_bitmap),
                prg_rbg_getcolor_16bpp_rbg,
                prg_generate_rbg_end);
            }
            glUseProgram(prg_rbg_2_2w_bitmap_16bpp_);
            break;
          }
          case 4: {
            if (prg_rbg_2_2w_bitmap_32bpp_ == 0) {
              prg_rbg_2_2w_bitmap_32bpp_ = compile_color_dot(
                S(a_prg_rbg_2_2w_bitmap),
                prg_rbg_getcolor_32bpp_rbg,
                prg_generate_rbg_end);
            }
            glUseProgram(prg_rbg_2_2w_bitmap_32bpp_);
            break;
          }
          }

        }
        else {
          if (rbg->info.patterndatasize == 1) {
            switch (rbg->info.colornumber) {
            case 0: { // BlukSlash
              if (prg_rbg_2_2w_p1_4bpp_ == 0) {
                prg_rbg_2_2w_p1_4bpp_ = compile_color_dot(
                  S(a_prg_rbg_2_2w_p1_4bpp),
                  prg_rbg_getcolor_4bpp,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_2_2w_p1_4bpp_);
              break;
            }
            case 1: { // Panzer Dragoon Zwei, Toshiden(Title) ToDo: Sky bug
              if (prg_rbg_2_2w_p1_8bpp_ == 0) {
                prg_rbg_2_2w_p1_8bpp_ = compile_color_dot(
                  S(a_prg_rbg_2_2w_p1_8bpp),
                  prg_rbg_getcolor_8bpp,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_2_2w_p1_8bpp_);
              break;
            }
            case 2: {
              if (prg_rbg_2_2w_p1_16bpp_p_ == 0) {
                prg_rbg_2_2w_p1_16bpp_p_ = compile_color_dot(
                  S(a_prg_rbg_2_2w_p1_4bpp),
                  prg_rbg_getcolor_16bpp_palette,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_2_2w_p1_16bpp_p_);
              break;
            }
            case 3: {
              if (prg_rbg_2_2w_p1_16bpp_ == 0) {
                prg_rbg_2_2w_p1_16bpp_ = compile_color_dot(
                  S(a_prg_rbg_2_2w_p1_4bpp),
                  prg_rbg_getcolor_16bpp_rbg,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_2_2w_p1_16bpp_);
              break;
            }
            case 4: {
              if (prg_rbg_2_2w_p1_32bpp_ == 0) {
                prg_rbg_2_2w_p1_32bpp_ = compile_color_dot(
                  S(a_prg_rbg_2_2w_p1_4bpp),
                  prg_rbg_getcolor_32bpp_rbg,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_2_2w_p1_32bpp_);
              break;
            }
            }
          }
          else {
            switch (rbg->info.colornumber) {
            case 0: {
              if (prg_rbg_2_2w_p2_4bpp_ == 0) {
                prg_rbg_2_2w_p2_4bpp_ = compile_color_dot(
                  S(a_prg_rbg_2_2w_p2_4bpp),
                  prg_rbg_getcolor_4bpp,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_2_2w_p2_4bpp_);
              break;
            }
            case 1: {
              if (prg_rbg_2_2w_p2_8bpp_ == 0) {
                prg_rbg_2_2w_p2_8bpp_ = compile_color_dot(
                  S(a_prg_rbg_2_2w_p2_8bpp),
                  prg_rbg_getcolor_8bpp,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_2_2w_p2_8bpp_);
              break;
            }
            case 2: {
              if (prg_rbg_2_2w_p2_16bpp_p_ == 0) {
                prg_rbg_2_2w_p2_16bpp_p_ = compile_color_dot(
                  S(a_prg_rbg_2_2w_p2_4bpp),
                  prg_rbg_getcolor_16bpp_palette,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_2_2w_p2_16bpp_p_);
              break;
            }
            case 3: {
              if (prg_rbg_2_2w_p2_16bpp_ == 0) {
                prg_rbg_2_2w_p2_16bpp_ = compile_color_dot(
                  S(a_prg_rbg_2_2w_p2_4bpp),
                  prg_rbg_getcolor_16bpp_rbg,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_2_2w_p2_16bpp_);
              break;
            }
            case 4: {
              if (prg_rbg_2_2w_p2_32bpp_ == 0) {
                prg_rbg_2_2w_p2_32bpp_ = compile_color_dot(
                  S(a_prg_rbg_2_2w_p2_4bpp),
                  prg_rbg_getcolor_32bpp_rbg,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_2_2w_p2_32bpp_);
              break;
            }
            }
          }
        }
      }
      else if (fixVdp2Regs->RPMD == 3) {

        if (rbg->info.isbitmap) {
          switch (rbg->info.colornumber) {
          case 0: {
            if (prg_rbg_3_2w_bitmap_4bpp_ == 0) {
              prg_rbg_3_2w_bitmap_4bpp_ = compile_color_dot(
                S(a_prg_rbg_3_2w_bitmap),
                prg_rbg_getcolor_4bpp,
                prg_generate_rbg_end);
            }
            glUseProgram(prg_rbg_3_2w_bitmap_4bpp_);
            break;
          }
          case 1: {
            if (prg_rbg_3_2w_bitmap_8bpp_ == 0) {
              prg_rbg_3_2w_bitmap_8bpp_ = compile_color_dot(
                S(a_prg_rbg_3_2w_bitmap),
                prg_rbg_getcolor_8bpp,
                prg_generate_rbg_end);
            }
            glUseProgram(prg_rbg_3_2w_bitmap_8bpp_);
            break;
          }
          case 2: {
            if (prg_rbg_3_2w_bitmap_16bpp_p_ == 0) {
              prg_rbg_3_2w_bitmap_16bpp_p_ = compile_color_dot(
                S(a_prg_rbg_3_2w_bitmap),
                prg_rbg_getcolor_16bpp_palette,
                prg_generate_rbg_end);
            }
            glUseProgram(prg_rbg_3_2w_bitmap_16bpp_p_);
            break;
          }
          case 3: {
            if (prg_rbg_3_2w_bitmap_16bpp_ == 0) {
              prg_rbg_3_2w_bitmap_16bpp_ = compile_color_dot(
                S(a_prg_rbg_3_2w_bitmap),
                prg_rbg_getcolor_16bpp_rbg,
                prg_generate_rbg_end);
            }
            glUseProgram(prg_rbg_3_2w_bitmap_16bpp_);
            break;
          }
          case 4: {
            if (prg_rbg_3_2w_bitmap_32bpp_ == 0) {
              prg_rbg_3_2w_bitmap_32bpp_ = compile_color_dot(
                S(a_prg_rbg_3_2w_bitmap),
                prg_rbg_getcolor_32bpp_rbg,
                prg_generate_rbg_end);
            }
            glUseProgram(prg_rbg_3_2w_bitmap_32bpp_);
            break;
          }
          }

        }
        else {
          if (rbg->info.patterndatasize == 1) {
            switch (rbg->info.colornumber) {
            case 0: {
              if (prg_rbg_3_2w_p1_4bpp_ == 0) {
                prg_rbg_3_2w_p1_4bpp_ = compile_color_dot(
                  S(a_prg_rbg_3_2w_p1_4bpp),
                  prg_rbg_getcolor_4bpp,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_3_2w_p1_4bpp_);
              break;
            }
            case 1: { // Final Fight Revenge, Grandia main
              if (prg_rbg_3_2w_p1_8bpp_ == 0) {
                prg_rbg_3_2w_p1_8bpp_ = compile_color_dot(
                  S(a_prg_rbg_3_2w_p1_8bpp),
                  prg_rbg_getcolor_8bpp,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_3_2w_p1_8bpp_);
              break;
            }
            case 2: {
              if (prg_rbg_3_2w_p1_16bpp_p_ == 0) {
                prg_rbg_3_2w_p1_16bpp_p_ = compile_color_dot(
                  S(a_prg_rbg_3_2w_p1_4bpp),
                  prg_rbg_getcolor_16bpp_palette,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_3_2w_p1_16bpp_p_);
              break;
            }
            case 3: { // Power Drift
              if (prg_rbg_3_2w_p1_16bpp_ == 0) {
                prg_rbg_3_2w_p1_16bpp_ = compile_color_dot(
                  S(a_prg_rbg_3_2w_p1_4bpp),
                  prg_rbg_getcolor_16bpp_rbg,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_3_2w_p1_16bpp_);
              break;
            }
            case 4: {
              if (prg_rbg_3_2w_p1_32bpp_ == 0) {
                prg_rbg_3_2w_p1_32bpp_ = compile_color_dot(
                  S(a_prg_rbg_3_2w_p1_4bpp),
                  prg_rbg_getcolor_32bpp_rbg,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_3_2w_p1_32bpp_);
              break;
            }
            }
          }
          else {
            switch (rbg->info.colornumber) {
            case 0: {
              if (prg_rbg_3_2w_p2_4bpp_ == 0) {
                prg_rbg_3_2w_p2_4bpp_ = compile_color_dot(
                  S(a_prg_rbg_3_2w_p2_4bpp),
                  prg_rbg_getcolor_4bpp,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_3_2w_p2_4bpp_);
              break;
            }
            case 1: {
              if (prg_rbg_3_2w_p2_8bpp_ == 0) {
                prg_rbg_3_2w_p2_8bpp_ = compile_color_dot(
                  S(a_prg_rbg_3_2w_p2_8bpp),
                  prg_rbg_getcolor_8bpp,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_3_2w_p2_8bpp_);
              break;
            }
            case 2: {
              if (prg_rbg_3_2w_p2_16bpp_p_ == 0) {
                prg_rbg_3_2w_p2_16bpp_p_ = compile_color_dot(
                  S(a_prg_rbg_3_2w_p2_4bpp),
                  prg_rbg_getcolor_16bpp_palette,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_3_2w_p2_16bpp_p_);
              break;
            }
            case 3: {
              if (prg_rbg_3_2w_p2_16bpp_ == 0) {
                prg_rbg_3_2w_p2_16bpp_ = compile_color_dot(
                  S(a_prg_rbg_3_2w_p2_4bpp),
                  prg_rbg_getcolor_16bpp_rbg,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_3_2w_p2_16bpp_);
              break;
            }
            case 4: {
              if (prg_rbg_3_2w_p2_32bpp_ == 0) {
                prg_rbg_3_2w_p2_32bpp_ = compile_color_dot(
                  S(a_prg_rbg_3_2w_p2_4bpp),
                  prg_rbg_getcolor_32bpp_rbg,
                  prg_generate_rbg_end);
              }
              glUseProgram(prg_rbg_3_2w_p2_32bpp_);
              break;
            }
            }
          }
        }
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_window_);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(vdp2WindowInfo)*(int(rbg->vres / rbg->rotate_mval_v) << rbg->info.hres_shift), (void*)rbg->info.pWinInfo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo_window_);
      }
    }


    ErrorHandle("glUseProgram");

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_vram_);
    //glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 0x80000, (void*)Vdp2Ram);
    if (mapped_vram == nullptr) mapped_vram = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 0x80000, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
    memcpy(mapped_vram, Vdp2Ram, 0x80000);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    mapped_vram = nullptr;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_vram_);
    ErrorHandle("glBindBufferBase");

    if (rbg->info.specialcolormode == 3 || paraA.k_mem_type != 0 || paraB.k_mem_type != 0) {
      if (ssbo_cram_ == 0) {
        glGenBuffers(1, &ssbo_cram_);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_cram_);
        glBufferData(GL_SHADER_STORAGE_BUFFER, 0x1000, NULL, GL_DYNAMIC_DRAW);
      }
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_cram_);
      glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 0x1000, (void*)Vdp2ColorRam);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssbo_cram_);
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_paraA_);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(vdp2rotationparameter_struct), (void*)&paraA);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(vdp2rotationparameter_struct), sizeof(vdp2rotationparameter_struct), (void*)&paraB);
    ErrorHandle("glBufferSubData");
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_paraA_);

    uniform.hres_scale = 1.0 / rbg->rotate_mval_h;
    uniform.vres_scale = 1.0 / rbg->rotate_mval_v;
    uniform.cellw = rbg->info.cellw;
    uniform.cellh = rbg->info.cellh;
    uniform.paladdr_ = rbg->info.paladdr;
    uniform.pagesize = rbg->pagesize;
    uniform.patternshift = rbg->patternshift;
    uniform.planew = rbg->info.planew;
    uniform.pagewh = rbg->info.pagewh;
    uniform.patterndatasize = rbg->info.patterndatasize;
    uniform.supplementdata = rbg->info.supplementdata;
    uniform.auxmode = rbg->info.auxmode;
    uniform.patternwh = rbg->info.patternwh;
    uniform.coloroffset = rbg->info.coloroffset;
    uniform.transparencyenable = rbg->info.transparencyenable;
    uniform.specialcolormode = rbg->info.specialcolormode;
    uniform.specialcolorfunction = rbg->info.specialcolorfunction;
    uniform.specialcode = rbg->info.specialcode;
    uniform.colornumber = rbg->info.colornumber;
    uniform.window_area_mode = rbg->info.WindwAreaMode;
    uniform.alpha_ = (float)rbg->info.alpha / 255.0f;
    uniform.specialprimode = rbg->info.specialprimode;
    uniform.priority = rbg->info.priority;

    if (Vdp2Internal.ColorMode < 2) {
      uniform.cram_shift = 1;
    }
    else {
      uniform.cram_shift = 2;
    }
    uniform.hires_shift = rbg->info.hres_shift;

    glBindBuffer(GL_UNIFORM_BUFFER, scene_uniform);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(RBGUniform), (void*)&uniform);
    ErrorHandle("glBufferSubData");
    glBindBufferBase(GL_UNIFORM_BUFFER, 3, scene_uniform);

    if (rbg->rgb_type == 0x04) {
      if (tex_surface_1 == 0) {
        glActiveTexture(GL_TEXTURE0);
        glGenTextures(1, &tex_surface_1);
        glBindTexture(GL_TEXTURE_2D, tex_surface_1);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        ErrorHandle("glBindTexture");
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, tex_width_, tex_height_);
        ErrorHandle("glTexStorage2D");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        ErrorHandle("glTexParameteri");
        glBindImageTexture(0, tex_surface_1, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
      }
      else {
        glBindImageTexture(0, tex_surface_1, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
        ErrorHandle("glBindImageTexture 1");
      }
    }
    else {
      glBindImageTexture(0, tex_surface_, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
      ErrorHandle("glBindImageTexture 0");
    }

    glDispatchCompute(work_groups_x, work_groups_y, 1);
    ErrorHandle("glDispatchCompute");

    glBindBuffer(GL_UNIFORM_BUFFER, 0);
  }

  //-----------------------------------------------
  GLuint getTexture(int id) {
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    if (id == 1) {
      return tex_surface_;
    }
    return tex_surface_1;
  }

  //-----------------------------------------------
  void onFinish() {
    if (ssbo_vram_ != 0 && mapped_vram == nullptr) {
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_vram_);
      mapped_vram = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 0x80000, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }
  }

};

#if defined(HAVE_VULKAN)

#include "vulkan/RBGGeneratorVulkan.h"
#include <iostream>

#define VK_CHECK_RESULT(f)																				\
{																										\
	vk::Result res = (f);																					\
	if (res != vk::Result::eSuccess)																				\
	{																									\
		std::cout << "Fatal : VkResult is \"" << vk::to_string(res) << "\" in " << __FILE__ << " at line " << __LINE__ << "\n"; \
		assert(res == vk::Result::eSuccess);																		\
	}																									\
}

#include "shaderc/shaderc.hpp"
using shaderc::Compiler;
using shaderc::CompileOptions;
using shaderc::SpvCompilationResult;

RBGGeneratorVulkan::RBGGeneratorVulkan() {

  rbgUniformParam = new RBGUniform();

}

RBGGeneratorVulkan::~RBGGeneratorVulkan() {
  delete rbgUniformParam;
}


void RBGGeneratorVulkan::init(VIDVulkan * vulkan, int width, int height) {

  this->vulkan = vulkan;
  VkDevice device = vulkan->getDevice();
  vk::Device d(device);
  resize(width,height);
  if (queue) return;

  //queue = vk::Queue(vulkan->getVulkanQueue()); //d.getQueue(vulkan->getVulkanComputeQueueFamilyIndex(), 0);
  queue = d.getQueue(vulkan->getVulkanComputeQueueFamilyIndex(), 0);

  semaphores.ready = d.createSemaphore({});
  semaphores.complete = d.createSemaphore({});

  //vk::SubmitInfo computeSubmitInfo;
  //computeSubmitInfo.signalSemaphoreCount = 1;
  //computeSubmitInfo.pSignalSemaphores = &semaphores.complete;
  //queue.submit(computeSubmitInfo, {});
  
  //commandPool = d.createCommandPool({ vk::CommandPoolCreateFlagBits::eResetCommandBuffer, vulkan->getVulkanGraphicsQueueFamilyIndex() /*vulkan->getVulkanComputeQueueFamilyIndex()*/ });
  commandPool = d.createCommandPool({ vk::CommandPoolCreateFlagBits::eResetCommandBuffer, vulkan->getVulkanComputeQueueFamilyIndex() });
  command = d.allocateCommandBuffers({ commandPool, vk::CommandBufferLevel::ePrimary, 1 })[0];
  
  // Create sampler
  vk::SamplerCreateInfo sampler;
  sampler.magFilter = vk::Filter::eLinear;
  sampler.minFilter = vk::Filter::eLinear;
  sampler.mipmapMode = vk::SamplerMipmapMode::eNearest;
  sampler.addressModeU = vk::SamplerAddressMode::eClampToBorder;
  sampler.addressModeV = sampler.addressModeU;
  sampler.addressModeW = sampler.addressModeU;
  sampler.mipLodBias = 0.0f;
  sampler.maxAnisotropy = 1.0f;
  sampler.compareOp = vk::CompareOp::eNever;
  sampler.minLod = 0.0f;
  sampler.maxLod = 1.0f;
  sampler.borderColor = vk::BorderColor::eFloatOpaqueWhite;
  this->sampler = d.createSampler(sampler);


  int size = sizeof(RBGUniform);
  auto allocatedSize = getAllainedSize(size);

  VkBuffer u;
  VkDeviceMemory m;
  vulkan->createBuffer(allocatedSize,
    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, u, m);
  rbgUniform.buf = vk::Buffer(u);
  rbgUniform.mem = vk::DeviceMemory(m);

  allocatedSize = getAllainedSize(0x80000);
  vulkan->createBuffer(allocatedSize,
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, u, m);
  ssbo_vram_.buf = vk::Buffer(u);
  ssbo_vram_.mem = vk::DeviceMemory(m);

  struct_size_ = sizeof(vdp2rotationparameter_struct);

  allocatedSize = getAllainedSize(struct_size_) * 2;
  vulkan->createBuffer(allocatedSize,
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, u, m);
  ssbo_paraA_.buf = vk::Buffer(u);
  ssbo_paraA_.mem = vk::DeviceMemory(m);

  allocatedSize = getAllainedSize(sizeof(vdp2WindowInfo) * 512);
  vulkan->createBuffer(allocatedSize,
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, u, m);
  ssbo_window_.buf = vk::Buffer(u);
  ssbo_window_.mem = vk::DeviceMemory(m);

  allocatedSize = getAllainedSize(0x1000);
  vulkan->createBuffer(allocatedSize,
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, u, m);
  ssbo_cram_.buf = vk::Buffer(u);
  ssbo_cram_.mem = vk::DeviceMemory(m);

  std::vector<vk::DescriptorPoolSize> poolSizes = {
      vk::DescriptorPoolSize{ vk::DescriptorType::eStorageImage, 1 },
      vk::DescriptorPoolSize{ vk::DescriptorType::eStorageBuffer, 1 },
      vk::DescriptorPoolSize{ vk::DescriptorType::eStorageBuffer, 1 },
      vk::DescriptorPoolSize{ vk::DescriptorType::eUniformBuffer, 1 },
      vk::DescriptorPoolSize{ vk::DescriptorType::eStorageBuffer, 1 },
      vk::DescriptorPoolSize{ vk::DescriptorType::eStorageBuffer, 1 },

  };

  descriptorPool = d.createDescriptorPool(vk::DescriptorPoolCreateInfo{ {}, 6, (uint32_t)poolSizes.size(), poolSizes.data() });
  std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings = {
    vk::DescriptorSetLayoutBinding{ 0, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute },
    vk::DescriptorSetLayoutBinding{ 1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute },
    vk::DescriptorSetLayoutBinding{ 2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute },
    vk::DescriptorSetLayoutBinding{ 3, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eCompute },
    vk::DescriptorSetLayoutBinding{ 4, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute },
    vk::DescriptorSetLayoutBinding{ 5, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute }
  };

  descriptorSetLayout = d.createDescriptorSetLayout({ {}, (uint32_t)setLayoutBindings.size(), setLayoutBindings.data() });
  descriptorSet = d.allocateDescriptorSets({ descriptorPool, 1, &descriptorSetLayout })[0];
  updateDescriptorSets(0);

  std::string target;

  for (int i = 0; i < 6; i++) {
    target += a_prg_rbg_0_2w_bitmap[i];
  }
  Compiler compiler;
  CompileOptions options;
  options.SetOptimizationLevel(shaderc_optimization_level_performance);
  SpvCompilationResult result = compiler.CompileGlslToSpv(
    target,
    shaderc_compute_shader,
    "a_prg_rbg_0_2w_bitmap",
    options);

  printf("%s%d\n", " erros: ", (int)result.GetNumErrors());
  if (result.GetNumErrors() != 0) {
    printf("%s%s\n", "messages", result.GetErrorMessage().c_str());
    throw std::runtime_error("failed to create shader module!");
  }
  std::vector<uint32_t> data = { result.cbegin(), result.cend() };

  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = data.size() * sizeof(uint32_t);
  createInfo.pCode = data.data();
  VkShaderModule shaderModule;
  if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module!");
  }

  pipelineLayout = d.createPipelineLayout({ {}, 1, &descriptorSetLayout });

  vk::ComputePipelineCreateInfo computePipelineCreateInfo;
  computePipelineCreateInfo.layout = pipelineLayout;
  computePipelineCreateInfo.stage.module = vk::ShaderModule(shaderModule);
  computePipelineCreateInfo.stage.stage = vk::ShaderStageFlagBits::eCompute;
  computePipelineCreateInfo.stage.pName = "main";
  auto a = d.createComputePipelines(nullptr, computePipelineCreateInfo);
  pipeline = a.value[0];

  d.destroyShaderModule(vk::ShaderModule(shaderModule));

}

void RBGGeneratorVulkan::updateDescriptorSets( int texindex ) {

  VkDevice device = vulkan->getDevice();
  vk::Device d(device);

  if (descriptorSet == (vk::DescriptorSet)nullptr) return;

  vk::DescriptorBufferInfo descriptor;
  descriptor.buffer = rbgUniform.buf;
  descriptor.range = VK_WHOLE_SIZE;
  descriptor.offset = 0;

  vk::DescriptorImageInfo texDescriptor{
    this->sampler, tex_surface[texindex].view, vk::ImageLayout::eGeneral
  };

  vk::DescriptorBufferInfo descriptor2;
  descriptor2.buffer = ssbo_vram_.buf;
  descriptor2.range = VK_WHOLE_SIZE;
  descriptor2.offset = 0;

  vk::DescriptorBufferInfo descriptor3;
  descriptor3.buffer = ssbo_paraA_.buf;
  descriptor3.range = VK_WHOLE_SIZE;
  descriptor3.offset = 0;

  vk::DescriptorBufferInfo descriptor4;
  descriptor4.buffer = ssbo_window_.buf;
  descriptor4.range = VK_WHOLE_SIZE;
  descriptor4.offset = 0;

  vk::DescriptorBufferInfo descriptor5;
  descriptor5.buffer = ssbo_cram_.buf;
  descriptor5.range = VK_WHOLE_SIZE;
  descriptor5.offset = 0;


  std::vector<vk::WriteDescriptorSet> computeWriteDescriptorSets{
    { descriptorSet, 0, 0, 1, vk::DescriptorType::eStorageImage, &texDescriptor },
    { descriptorSet, 1, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &descriptor2 },
    { descriptorSet, 2, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &descriptor3 },
    { descriptorSet, 3, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &descriptor },
    { descriptorSet, 4, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &descriptor4 },
    { descriptorSet, 5, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &descriptor5 },
  };

  
  d.updateDescriptorSets(computeWriteDescriptorSets, {});
}


int RBGGeneratorVulkan::getAllainedSize(int size) {

  VkDevice device = vulkan->getDevice();
  vk::Device d(device);
  auto p = vk::PhysicalDevice(vulkan->getPhysicalDevice());
  auto alignment = p.getProperties().limits.minUniformBufferOffsetAlignment;
  auto extra = size % alignment;
  auto count = 1;
  auto alignedSize = size + (alignment - extra);
  auto allocatedSize = count * alignedSize;

  return allocatedSize;

}

void RBGGeneratorVulkan::resize(int width, int height) {
  if (tex_width_ == width && tex_height_ == height) return;

  VkDevice device = vulkan->getDevice();
  vk::Device d(device);

  YGLDEBUG("resize %d, %d\n", width, height);

  tex_width_ = width;
  tex_height_ = height;

  if (tex_surface[0].image) d.destroyImage(tex_surface[0].image);
  if (tex_surface[0].view) d.destroyImageView(tex_surface[0].view);
  if (tex_surface[0].mem) d.freeMemory(tex_surface[0].mem);
  if (tex_surface[1].image) d.destroyImage(tex_surface[1].image);
  if (tex_surface[1].view) d.destroyImageView(tex_surface[1].view);
  if (tex_surface[1].mem) d.freeMemory(tex_surface[1].mem);

  
  vk::ImageCreateInfo imageCreateInfo;
  imageCreateInfo.imageType = vk::ImageType::e2D;
  imageCreateInfo.format = vk::Format::eR8G8B8A8Unorm;
  imageCreateInfo.extent.width = width;
  imageCreateInfo.extent.height = height;
  imageCreateInfo.extent.depth = 1;
  imageCreateInfo.mipLevels = 1;
  imageCreateInfo.arrayLayers = 1;
  imageCreateInfo.tiling = vk::ImageTiling::eOptimal;
  imageCreateInfo.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage;
  imageCreateInfo.sharingMode = vk::SharingMode::eConcurrent;
  imageCreateInfo.queueFamilyIndexCount = 2;
  uint32_t indexes[] = { vulkan->getVulkanGraphicsQueueFamilyIndex(),vulkan->getVulkanComputeQueueFamilyIndex() };
  imageCreateInfo.pQueueFamilyIndices = indexes;

  for (int i = 0; i < 2; i++) {
    VK_CHECK_RESULT(d.createImage(&imageCreateInfo, nullptr, &tex_surface[i].image));

    vk::MemoryRequirements memReqs = d.getImageMemoryRequirements(tex_surface[i].image);
    vk::MemoryAllocateInfo memAllocInfo;

    memAllocInfo.allocationSize = memReqs.size;
    memAllocInfo.memoryTypeIndex = vulkan->findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT);
    tex_surface[i].mem = d.allocateMemory(memAllocInfo);
    d.bindImageMemory(tex_surface[i].image, tex_surface[i].mem, 0);

    vk::ImageViewCreateInfo view;
    view.viewType = vk::ImageViewType::e2D;
    view.format = vk::Format::eR8G8B8A8Unorm;
    view.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
    view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
    view.image = tex_surface[i].image;
    tex_surface[i].view = d.createImageView(view);

    vulkan->transitionImageLayout(VkImage(tex_surface[i].image), VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
  }

  updateDescriptorSets(0);
}

vk::Pipeline RBGGeneratorVulkan::compile_color_dot(
  const char * base[], int size, const char * color, const char * dot) {
  
  VkDevice device = vulkan->getDevice();
  vk::Device d(device);

  vk::Pipeline rtn = nullptr;

  std::string target;
  for (int i = 0; i < size-2; i++) {
    target += base[i];
  }

  target += color;
  target += dot;

  Compiler compiler;
  CompileOptions options;
  options.SetOptimizationLevel(shaderc_optimization_level_performance);
  //options.SetOptimizationLevel(shaderc_optimization_level_zero);
  SpvCompilationResult result = compiler.CompileGlslToSpv(
    target,
    shaderc_compute_shader,
    "RBG",
    options);

  printf("%s%d\n", " erros: ", (int)result.GetNumErrors());
  if (result.GetNumErrors() != 0) {
    printf("%s", target.c_str());
    printf("%s%s\n", "messages", result.GetErrorMessage().c_str());
    throw std::runtime_error("failed to create shader module!");
  }
  std::vector<uint32_t> data = { result.cbegin(), result.cend() };

  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = data.size() * sizeof(uint32_t);
  createInfo.pCode = data.data();
  VkShaderModule shaderModule;
  if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module!");
  }

  pipelineLayout = d.createPipelineLayout({ {}, 1, &descriptorSetLayout });

  vk::ComputePipelineCreateInfo computePipelineCreateInfo;
  computePipelineCreateInfo.layout = pipelineLayout;
  computePipelineCreateInfo.stage.module = vk::ShaderModule(shaderModule);
  computePipelineCreateInfo.stage.stage = vk::ShaderStageFlagBits::eCompute;
  computePipelineCreateInfo.stage.pName = "main";
  auto a = d.createComputePipelines(nullptr, computePipelineCreateInfo);
  rtn = a.value[0];

  d.destroyShaderModule(vk::ShaderModule(shaderModule));

  return rtn;
}

void RBGGeneratorVulkan::update(VIDVulkan::RBGDrawInfo * rbg, const vdp2rotationparameter_struct & paraa, const vdp2rotationparameter_struct & parab){

  VkDevice device = vulkan->getDevice();
  vk::Device d(device);

  vk::Pipeline CurrentPipeline = nullptr;

  int texindex = 0;
  if (rbg->rgb_type != 0) {
    texindex = 1;
  }

  // Line color insersion
  if (rbg->info.LineColorBase != 0 && VDP2_CC_NONE != (rbg->info.blendmode & 0x03)) {
    if (fixVdp2Regs->RPMD == 0 || (fixVdp2Regs->RPMD == 3 && (fixVdp2Regs->WCTLD & 0xA) == 0)) {
      if (rbg->info.isbitmap) {
        if (!prg_rbg_0_2w_bitmap_8bpp_line_) {
          prg_rbg_0_2w_bitmap_8bpp_line_ = compile_color_dot(
            S(a_prg_rbg_0_2w_bitmap),
            prg_rbg_getcolor_8bpp,
            prg_generate_rbg_line_end);
        }
        CurrentPipeline = prg_rbg_0_2w_bitmap_8bpp_line_;
      }
      else {
        if (rbg->info.patterndatasize == 1) {
          switch (rbg->info.colornumber) {
          case 0: { // Decathalete ToDo: Line Color Bug
            if (!prg_rbg_0_2w_p1_4bpp_line_) {
              prg_rbg_0_2w_p1_4bpp_line_ = compile_color_dot(
                S(a_prg_rbg_0_2w_p1_4bpp),
                prg_rbg_getcolor_4bpp,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_0_2w_p1_4bpp_line_;
            break;
          }
          case 1: { // Sakatuku 2 Ground, GUNDAM Side Story 2, SonicR ToDo: 2Player
            if (!prg_rbg_0_2w_p1_8bpp_line_) {
              prg_rbg_0_2w_p1_8bpp_line_ = compile_color_dot(
                S(a_prg_rbg_0_2w_p1_8bpp),
                prg_rbg_getcolor_8bpp,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_0_2w_p1_8bpp_line_;
            break;
          }
          case 2: {
            if (!prg_rbg_0_2w_p1_16bpp_p_line_) {
              prg_rbg_0_2w_p1_16bpp_p_line_ = compile_color_dot(
                S(a_prg_rbg_0_2w_p1_4bpp),
                prg_rbg_getcolor_16bpp_palette,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_0_2w_p1_16bpp_p_line_;
            break;
          }
          case 3: {
            if (!prg_rbg_0_2w_p1_16bpp_line_) {
              prg_rbg_0_2w_p1_16bpp_line_ = compile_color_dot(
                S(a_prg_rbg_0_2w_p1_4bpp),
                prg_rbg_getcolor_16bpp_rbg,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_0_2w_p1_16bpp_line_;
            break;
          }
          case 4: {
            if (!prg_rbg_0_2w_p1_32bpp_line_) {
              prg_rbg_0_2w_p1_32bpp_line_ = compile_color_dot(
                S(a_prg_rbg_0_2w_p1_4bpp),
                prg_rbg_getcolor_32bpp_rbg,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_0_2w_p1_32bpp_line_;
            break;
          }
          }
        }
        else {
          switch (rbg->info.colornumber) {
          case 0: {
            if (!prg_rbg_0_2w_p2_4bpp_line_) {
              prg_rbg_0_2w_p2_4bpp_line_ = compile_color_dot(
                S(a_prg_rbg_0_2w_p2_4bpp),
                prg_rbg_getcolor_4bpp,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_0_2w_p2_4bpp_line_;
            break;
          }
          case 1: { // Thunder Force V
            if (!prg_rbg_0_2w_p2_8bpp_line_) {
              prg_rbg_0_2w_p2_8bpp_line_ = compile_color_dot(
                S(a_prg_rbg_0_2w_p2_8bpp),
                prg_rbg_getcolor_8bpp,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_0_2w_p2_8bpp_line_;
            break;
          }
          case 2: {
            if (!prg_rbg_0_2w_p2_16bpp_p_line_) {
              prg_rbg_0_2w_p2_16bpp_p_line_ = compile_color_dot(
                S(a_prg_rbg_0_2w_p2_4bpp),
                prg_rbg_getcolor_16bpp_palette,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_0_2w_p2_16bpp_p_line_;
            break;
          }
          case 3: {
            if (!prg_rbg_0_2w_p2_16bpp_line_) {
              prg_rbg_0_2w_p2_16bpp_line_ = compile_color_dot(
                S(a_prg_rbg_0_2w_p2_4bpp),
                prg_rbg_getcolor_16bpp_rbg,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_0_2w_p2_16bpp_line_;
            break;
          }
          case 4: {
            if (!prg_rbg_0_2w_p2_32bpp_line_) {
              prg_rbg_0_2w_p2_32bpp_line_ = compile_color_dot(
                S(a_prg_rbg_0_2w_p2_4bpp),
                prg_rbg_getcolor_32bpp_rbg,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_0_2w_p2_32bpp_line_;
            break;
          }
          }
        }
      }
    }
    else if (fixVdp2Regs->RPMD == 1) {
      if (rbg->info.isbitmap) {
        if (!prg_rbg_1_2w_bitmap_8bpp_line_) {
          prg_rbg_1_2w_bitmap_8bpp_line_ = compile_color_dot(
            S(a_prg_rbg_1_2w_bitmap),
            prg_rbg_getcolor_8bpp,
            prg_generate_rbg_line_end);
        }
        CurrentPipeline=prg_rbg_1_2w_bitmap_8bpp_line_;
      }
      else {
        if (rbg->info.patterndatasize == 1) {
          switch (rbg->info.colornumber) {
          case 0: {
            if (!prg_rbg_1_2w_p1_4bpp_line_) {
              prg_rbg_1_2w_p1_4bpp_line_ = compile_color_dot(
                S(a_prg_rbg_1_2w_p1_4bpp),
                prg_rbg_getcolor_4bpp,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_1_2w_p1_4bpp_line_;
            break;
          }
          case 1: {
            if (!prg_rbg_1_2w_p1_8bpp_line_) {
              prg_rbg_1_2w_p1_8bpp_line_ = compile_color_dot(
                S(a_prg_rbg_1_2w_p1_8bpp),
                prg_rbg_getcolor_8bpp,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_1_2w_p1_8bpp_line_;
            break;
          }
          case 2: {
            if (!prg_rbg_1_2w_p1_16bpp_p_line_) {
              prg_rbg_1_2w_p1_16bpp_p_line_ = compile_color_dot(
                S(a_prg_rbg_1_2w_p1_4bpp),
                prg_rbg_getcolor_16bpp_palette,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_1_2w_p1_16bpp_p_line_;
            break;
          }
          case 3: {
            if (!prg_rbg_1_2w_p1_16bpp_line_) {
              prg_rbg_1_2w_p1_16bpp_line_ = compile_color_dot(
                S(a_prg_rbg_1_2w_p1_4bpp),
                prg_rbg_getcolor_16bpp_rbg,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_1_2w_p1_16bpp_line_;
            break;
          }
          case 4: {
            if (!prg_rbg_1_2w_p1_32bpp_line_) {
              prg_rbg_1_2w_p1_32bpp_line_ = compile_color_dot(
                S(a_prg_rbg_0_2w_p1_4bpp),
                prg_rbg_getcolor_32bpp_rbg,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_1_2w_p1_32bpp_line_;
            break;
          }
          }
        }
        else {
          switch (rbg->info.colornumber) {
          case 0: {
            if (!prg_rbg_1_2w_p2_4bpp_line_) {
              prg_rbg_1_2w_p2_4bpp_line_ = compile_color_dot(
                S(a_prg_rbg_1_2w_p2_4bpp),
                prg_rbg_getcolor_4bpp,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_1_2w_p2_4bpp_line_;
            break;
          }
          case 1: {
            if (!prg_rbg_1_2w_p2_8bpp_line_) {
              prg_rbg_1_2w_p2_8bpp_line_ = compile_color_dot(
                S(a_prg_rbg_1_2w_p2_8bpp),
                prg_rbg_getcolor_8bpp,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_1_2w_p2_8bpp_line_;
            break;
          }
          case 2: {
            if (!prg_rbg_1_2w_p2_16bpp_p_line_) {
              prg_rbg_1_2w_p2_16bpp_p_line_ = compile_color_dot(
                S(a_prg_rbg_1_2w_p2_4bpp),
                prg_rbg_getcolor_16bpp_palette,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_1_2w_p2_16bpp_p_line_;
            break;
          }
          case 3: {
            if (!prg_rbg_1_2w_p2_16bpp_line_) {
              prg_rbg_1_2w_p2_16bpp_line_ = compile_color_dot(
                S(a_prg_rbg_1_2w_p2_4bpp),
                prg_rbg_getcolor_16bpp_rbg,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_1_2w_p2_16bpp_line_;
            break;
          }
          case 4: {
            if (!prg_rbg_1_2w_p2_32bpp_line_) {
              prg_rbg_1_2w_p2_32bpp_line_ = compile_color_dot(
                S(a_prg_rbg_1_2w_p2_4bpp),
                prg_rbg_getcolor_32bpp_rbg,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_1_2w_p2_32bpp_line_;
            break;
          }
          }
        }
      }
    }
    else if (fixVdp2Regs->RPMD == 2) {
      if (rbg->info.isbitmap) {
        if (!prg_rbg_2_2w_bitmap_8bpp_line_) {
          prg_rbg_2_2w_bitmap_8bpp_line_ = compile_color_dot(
            S(a_prg_rbg_2_2w_bitmap),
            prg_rbg_getcolor_8bpp,
            prg_generate_rbg_line_end);
        }
        CurrentPipeline=prg_rbg_2_2w_bitmap_8bpp_line_;
      }
      else {
        if (rbg->info.patterndatasize == 1) {
          switch (rbg->info.colornumber) {
          case 0: {
            if (!prg_rbg_2_2w_p1_4bpp_line_) {
              prg_rbg_2_2w_p1_4bpp_line_ = compile_color_dot(
                S(a_prg_rbg_2_2w_p1_4bpp),
                prg_rbg_getcolor_4bpp,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_2_2w_p1_4bpp_line_;
            break;
          }
          case 1: { // Panzer Dragoon 1
            if (!prg_rbg_2_2w_p1_8bpp_line_) {
              prg_rbg_2_2w_p1_8bpp_line_ = compile_color_dot(
                S(a_prg_rbg_2_2w_p1_8bpp),
                prg_rbg_getcolor_8bpp,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_2_2w_p1_8bpp_line_;
            break;
          }
          case 2: {
            if (!prg_rbg_2_2w_p1_16bpp_p_line_) {
              prg_rbg_2_2w_p1_16bpp_p_line_ = compile_color_dot(
                S(a_prg_rbg_2_2w_p1_4bpp),
                prg_rbg_getcolor_16bpp_palette,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_2_2w_p1_16bpp_p_line_;
            break;
          }
          case 3: {
            if (!prg_rbg_2_2w_p1_16bpp_line_) {
              prg_rbg_2_2w_p1_16bpp_line_ = compile_color_dot(
                S(a_prg_rbg_2_2w_p1_4bpp),
                prg_rbg_getcolor_16bpp_rbg,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_2_2w_p1_16bpp_line_;
            break;
          }
          case 4: {
            if (!prg_rbg_2_2w_p1_32bpp_line_) {
              prg_rbg_2_2w_p1_32bpp_line_ = compile_color_dot(
                S(a_prg_rbg_2_2w_p1_4bpp),
                prg_rbg_getcolor_32bpp_rbg,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_2_2w_p1_32bpp_line_;
            break;
          }
          }
        }
        else {
          switch (rbg->info.colornumber) {
          case 0: {
            if (!prg_rbg_2_2w_p2_4bpp_line_) {
              prg_rbg_2_2w_p2_4bpp_line_ = compile_color_dot(
                S(a_prg_rbg_2_2w_p2_4bpp),
                prg_rbg_getcolor_4bpp,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_2_2w_p2_4bpp_line_;
            break;
          }
          case 1: {
            if (!prg_rbg_2_2w_p2_8bpp_line_) {
              prg_rbg_2_2w_p2_8bpp_line_ = compile_color_dot(
                S(a_prg_rbg_2_2w_p2_8bpp),
                prg_rbg_getcolor_8bpp,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_2_2w_p2_8bpp_line_;
            break;
          }
          case 2: {
            if (!prg_rbg_2_2w_p2_16bpp_p_line_) {
              prg_rbg_2_2w_p2_16bpp_p_line_ = compile_color_dot(
                S(a_prg_rbg_2_2w_p2_4bpp),
                prg_rbg_getcolor_16bpp_palette,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_2_2w_p2_16bpp_p_line_;
            break;
          }
          case 3: {
            if (!prg_rbg_2_2w_p2_16bpp_line_) {
              prg_rbg_2_2w_p2_16bpp_line_ = compile_color_dot(
                S(a_prg_rbg_2_2w_p2_4bpp),
                prg_rbg_getcolor_16bpp_rbg,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_2_2w_p2_16bpp_line_;
            break;
          }
          case 4: {
            if (!prg_rbg_2_2w_p2_32bpp_line_) {
              prg_rbg_2_2w_p2_32bpp_line_ = compile_color_dot(
                S(a_prg_rbg_2_2w_p2_4bpp),
                prg_rbg_getcolor_32bpp_rbg,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_2_2w_p2_32bpp_line_;
            break;
          }
          }
        }
      }
    }
    else if (fixVdp2Regs->RPMD == 3) {

      if (rbg->info.isbitmap) {
        if (!prg_rbg_3_2w_bitmap_8bpp_line_) {
          prg_rbg_3_2w_bitmap_8bpp_line_ = compile_color_dot(
            S(a_prg_rbg_3_2w_bitmap),
            prg_rbg_getcolor_8bpp,
            prg_generate_rbg_line_end);
        }
        CurrentPipeline=prg_rbg_3_2w_bitmap_8bpp_line_;
      }
      else {
        if (rbg->info.patterndatasize == 1) {
          switch (rbg->info.colornumber) {
          case 0: {
            if (!prg_rbg_3_2w_p1_4bpp_line_) {
              prg_rbg_3_2w_p1_4bpp_line_ = compile_color_dot(
                S(a_prg_rbg_3_2w_p1_4bpp),
                prg_rbg_getcolor_4bpp,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_3_2w_p1_4bpp_line_;
            break;
          }
          case 1: {
            if (!prg_rbg_3_2w_p1_8bpp_line_) {
              prg_rbg_3_2w_p1_8bpp_line_ = compile_color_dot(
                S(a_prg_rbg_3_2w_p1_8bpp),
                prg_rbg_getcolor_8bpp,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_3_2w_p1_8bpp_line_;
            break;
          }
          case 2: {
            if (!prg_rbg_3_2w_p1_16bpp_p_line_) {
              prg_rbg_3_2w_p1_16bpp_p_line_ = compile_color_dot(
                S(a_prg_rbg_3_2w_p1_4bpp),
                prg_rbg_getcolor_16bpp_palette,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_3_2w_p1_16bpp_p_line_;
            break;
          }
          case 3: {
            if (!prg_rbg_3_2w_p1_16bpp_line_) {
              prg_rbg_3_2w_p1_16bpp_line_ = compile_color_dot(
                S(a_prg_rbg_3_2w_p1_4bpp),
                prg_rbg_getcolor_16bpp_rbg,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_3_2w_p1_16bpp_line_;
            break;
          }
          case 4: {
            if (!prg_rbg_3_2w_p1_32bpp_line_) {
              prg_rbg_3_2w_p1_32bpp_line_ = compile_color_dot(
                S(a_prg_rbg_3_2w_p1_4bpp),
                prg_rbg_getcolor_32bpp_rbg,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_3_2w_p1_32bpp_line_;
            break;
          }
          }
        }
        else {
          switch (rbg->info.colornumber) {
          case 0: {
            if (!prg_rbg_3_2w_p2_4bpp_line_) {
              prg_rbg_3_2w_p2_4bpp_line_ = compile_color_dot(
                S(a_prg_rbg_3_2w_p2_4bpp),
                prg_rbg_getcolor_4bpp,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_3_2w_p2_4bpp_line_;
            break;
          }
          case 1: {
            if (!prg_rbg_3_2w_p2_8bpp_line_) {
              prg_rbg_3_2w_p2_8bpp_line_ = compile_color_dot(
                S(a_prg_rbg_3_2w_p2_8bpp),
                prg_rbg_getcolor_8bpp,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_3_2w_p2_8bpp_line_;
            break;
          }
          case 2: {
            if (!prg_rbg_3_2w_p2_16bpp_p_line_) {
              prg_rbg_3_2w_p2_16bpp_p_line_ = compile_color_dot(
                S(a_prg_rbg_3_2w_p2_4bpp),
                prg_rbg_getcolor_16bpp_palette,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_3_2w_p2_16bpp_p_line_;
            break;
          }
          case 3: {
            if (!prg_rbg_3_2w_p2_16bpp_line_) {
              prg_rbg_3_2w_p2_16bpp_line_ = compile_color_dot(
                S(a_prg_rbg_3_2w_p2_4bpp),
                prg_rbg_getcolor_16bpp_rbg,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_3_2w_p2_16bpp_line_;
            break;
          }
          case 4: {
            if (!prg_rbg_3_2w_p2_32bpp_line_) {
              prg_rbg_3_2w_p2_32bpp_line_ = compile_color_dot(
                S(a_prg_rbg_3_2w_p2_4bpp),
                prg_rbg_getcolor_32bpp_rbg,
                prg_generate_rbg_line_end);
            }
            CurrentPipeline=prg_rbg_3_2w_p2_32bpp_line_;
            break;
          }
          }
        }
      }
      // ToDo: Copy Window Info
      //glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_window_);
      //glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(vdp2WindowInfo)*(int(rbg->vres / rbg->rotate_mval_v) << rbg->info.hres_shift), (void*)rbg->info.pWinInfo);
      //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo_window_);
    }

  }

  // no line color insersion
  else {
    if (rbg->rgb_type == 0 && (fixVdp2Regs->RPMD == 0 || (fixVdp2Regs->RPMD == 3 && (fixVdp2Regs->WCTLD & 0xA) == 0))) {
      if (rbg->info.isbitmap) {
        switch (rbg->info.colornumber) {
        case 0: {
          if (!prg_rbg_0_2w_bitmap_4bpp_) {
            prg_rbg_0_2w_bitmap_4bpp_ = compile_color_dot(
              S(a_prg_rbg_0_2w_bitmap),
              prg_rbg_getcolor_4bpp,
              prg_generate_rbg_end);
          }
          CurrentPipeline=prg_rbg_0_2w_bitmap_4bpp_;
          break;
        }
        case 1: { // SF3S1( Initial )
          if (!prg_rbg_0_2w_bitmap_8bpp_) {
            prg_rbg_0_2w_bitmap_8bpp_ = compile_color_dot(
              S(a_prg_rbg_0_2w_bitmap),
              prg_rbg_getcolor_8bpp,
              prg_generate_rbg_end);
          }
          CurrentPipeline=prg_rbg_0_2w_bitmap_8bpp_;
          break;
        }
        case 2: {
          if (!prg_rbg_0_2w_bitmap_16bpp_p_) {
            prg_rbg_0_2w_bitmap_16bpp_p_ = compile_color_dot(
              S(a_prg_rbg_0_2w_bitmap),
              prg_rbg_getcolor_16bpp_palette,
              prg_generate_rbg_end);
          }
          CurrentPipeline=prg_rbg_0_2w_bitmap_16bpp_p_;
          break;
        }
        case 3: { // NHL 97 Title, GRANDIA Title
          if (!prg_rbg_0_2w_bitmap_16bpp_) {
            prg_rbg_0_2w_bitmap_16bpp_ = compile_color_dot(
              S(a_prg_rbg_0_2w_bitmap),
              prg_rbg_getcolor_16bpp_rbg,
              prg_generate_rbg_end);
          }
          CurrentPipeline=prg_rbg_0_2w_bitmap_16bpp_;
          break;
        }
        case 4: {
          if (!prg_rbg_0_2w_bitmap_32bpp_) {
            prg_rbg_0_2w_bitmap_32bpp_ = compile_color_dot(
              S(a_prg_rbg_0_2w_bitmap),
              prg_rbg_getcolor_32bpp_rbg,
              prg_generate_rbg_end);
          }
          CurrentPipeline=prg_rbg_0_2w_bitmap_32bpp_;
          break;
        }
        }
      }
      else {
        if (rbg->info.patterndatasize == 1) {
          switch (rbg->info.colornumber) {
          case 0: { // Dead or Alive, Rediant Silver Gun, Diehard
            if (!prg_rbg_0_2w_p1_4bpp_) {
              prg_rbg_0_2w_p1_4bpp_ = compile_color_dot(
                S(a_prg_rbg_0_2w_p1_4bpp),
                prg_rbg_getcolor_4bpp,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_0_2w_p1_4bpp_;
            break;
          }
          case 1: { // Sakatuku 2 ( Initial Setting ), Virtua Fighter 2, Virtual-on
            if (!prg_rbg_0_2w_p1_8bpp_) {
              prg_rbg_0_2w_p1_8bpp_ = compile_color_dot(
                S(a_prg_rbg_0_2w_p1_8bpp),
                prg_rbg_getcolor_8bpp,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_0_2w_p1_8bpp_;
            break;
          }
          case 2: {
            if (!prg_rbg_0_2w_p1_16bpp_p_) {
              prg_rbg_0_2w_p1_16bpp_p_ = compile_color_dot(
                S(a_prg_rbg_0_2w_p1_4bpp),
                prg_rbg_getcolor_16bpp_palette,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_0_2w_p1_16bpp_p_;
            break;
          }
          case 3: {
            if (!prg_rbg_0_2w_p1_16bpp_) {
              prg_rbg_0_2w_p1_16bpp_ = compile_color_dot(
                S(a_prg_rbg_0_2w_p1_4bpp),
                prg_rbg_getcolor_16bpp_rbg,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_0_2w_p1_16bpp_;
            break;
          }
          case 4: {
            if (!prg_rbg_0_2w_p1_32bpp_) {
              prg_rbg_0_2w_p1_32bpp_ = compile_color_dot(
                S(a_prg_rbg_0_2w_p1_4bpp),
                prg_rbg_getcolor_32bpp_rbg,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_0_2w_p1_32bpp_;
            break;
          }
          }
        }
        else {
          switch (rbg->info.colornumber) {
          case 0: {
            if (!prg_rbg_0_2w_p2_4bpp_) {
              prg_rbg_0_2w_p2_4bpp_ = compile_color_dot(
                S(a_prg_rbg_0_2w_p2_4bpp),
                prg_rbg_getcolor_4bpp,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_0_2w_p2_4bpp_;
            break;
          }
          case 1: { // NHL97(In Game), BIOS 
            if (!prg_rbg_0_2w_p2_8bpp_) {
              prg_rbg_0_2w_p2_8bpp_ = compile_color_dot(
                S(a_prg_rbg_0_2w_p2_8bpp),
                prg_rbg_getcolor_8bpp,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_0_2w_p2_8bpp_;
            break;
          }
          case 2: {
            if (!prg_rbg_0_2w_p2_16bpp_p_) {
              prg_rbg_0_2w_p2_16bpp_p_ = compile_color_dot(
                S(a_prg_rbg_0_2w_p2_4bpp),
                prg_rbg_getcolor_16bpp_palette,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_0_2w_p2_16bpp_p_;
            break;
          }
          case 3: {
            if (!prg_rbg_0_2w_p2_16bpp_) {
              prg_rbg_0_2w_p2_16bpp_ = compile_color_dot(
                S(a_prg_rbg_0_2w_p2_4bpp),
                prg_rbg_getcolor_16bpp_rbg,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_0_2w_p2_16bpp_;
            break;
          }
          case 4: {
            if (!prg_rbg_0_2w_p2_32bpp_) {
              prg_rbg_0_2w_p2_32bpp_ = compile_color_dot(
                S(a_prg_rbg_0_2w_p2_4bpp),
                prg_rbg_getcolor_32bpp_rbg,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_0_2w_p2_32bpp_;
            break;
          }
          }
        }
      }
    }
    else if ((fixVdp2Regs->RPMD == 1 && rbg->rgb_type == 0) || rbg->rgb_type == 0x04) {
      if (rbg->info.isbitmap) {
        switch (rbg->info.colornumber) {
        case 0: {
          if (!prg_rbg_1_2w_bitmap_4bpp_) {
            prg_rbg_1_2w_bitmap_4bpp_ = compile_color_dot(
              S(a_prg_rbg_1_2w_bitmap),
              prg_rbg_getcolor_4bpp,
              prg_generate_rbg_end);
          }
          CurrentPipeline=prg_rbg_1_2w_bitmap_4bpp_;
          break;
        }
        case 1: {
          if (!prg_rbg_1_2w_bitmap_8bpp_) {
            prg_rbg_1_2w_bitmap_8bpp_ = compile_color_dot(
              S(a_prg_rbg_1_2w_bitmap),
              prg_rbg_getcolor_8bpp,
              prg_generate_rbg_end);
          }
          CurrentPipeline=prg_rbg_1_2w_bitmap_8bpp_;
          break;
        }
        case 2: {
          if (!prg_rbg_1_2w_bitmap_16bpp_p_) {
            prg_rbg_1_2w_bitmap_16bpp_p_ = compile_color_dot(
              S(a_prg_rbg_1_2w_bitmap),
              prg_rbg_getcolor_16bpp_palette,
              prg_generate_rbg_end);
          }
          CurrentPipeline=prg_rbg_1_2w_bitmap_16bpp_p_;
          break;
        }
        case 3: {
          if (!prg_rbg_1_2w_bitmap_16bpp_) {
            prg_rbg_1_2w_bitmap_16bpp_ = compile_color_dot(
              S(a_prg_rbg_1_2w_bitmap),
              prg_rbg_getcolor_16bpp_rbg,
              prg_generate_rbg_end);
          }
          CurrentPipeline=prg_rbg_1_2w_bitmap_16bpp_;
          break;
        }
        case 4: {
          if (!prg_rbg_1_2w_bitmap_32bpp_) {
            prg_rbg_1_2w_bitmap_32bpp_ = compile_color_dot(
              S(a_prg_rbg_1_2w_bitmap),
              prg_rbg_getcolor_32bpp_rbg,
              prg_generate_rbg_end);
          }
          CurrentPipeline=prg_rbg_1_2w_bitmap_32bpp_;
          break;
        }
        }
      }
      else {
        if (rbg->info.patterndatasize == 1) {
          switch (rbg->info.colornumber) {
          case 0: {
            if (!prg_rbg_1_2w_p1_4bpp_) {
              prg_rbg_1_2w_p1_4bpp_ = compile_color_dot(
                S(a_prg_rbg_1_2w_p1_4bpp),
                prg_rbg_getcolor_4bpp,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_1_2w_p1_4bpp_;
            break;
          }
          case 1: {
            if (!prg_rbg_1_2w_p1_8bpp_) {
              prg_rbg_1_2w_p1_8bpp_ = compile_color_dot(
                S(a_prg_rbg_1_2w_p1_8bpp),
                prg_rbg_getcolor_8bpp,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_1_2w_p1_8bpp_;
            break;
          }
          case 2: {
            if (!prg_rbg_1_2w_p1_16bpp_p_) {
              prg_rbg_1_2w_p1_16bpp_p_ = compile_color_dot(
                S(a_prg_rbg_1_2w_p1_4bpp),
                prg_rbg_getcolor_16bpp_palette,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_1_2w_p1_16bpp_p_;
            break;
          }
          case 3: {
            if (!prg_rbg_1_2w_p1_16bpp_) {
              prg_rbg_1_2w_p1_16bpp_ = compile_color_dot(
                S(a_prg_rbg_1_2w_p1_4bpp),
                prg_rbg_getcolor_16bpp_rbg,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_1_2w_p1_16bpp_;
            break;
          }
          case 4: {
            if (!prg_rbg_1_2w_p1_32bpp_) {
              prg_rbg_1_2w_p1_32bpp_ = compile_color_dot(
                S(a_prg_rbg_0_2w_p1_4bpp),
                prg_rbg_getcolor_32bpp_rbg,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_1_2w_p1_32bpp_;
            break;
          }
          }
        }
        else {
          switch (rbg->info.colornumber) {
          case 0: {
            if (!prg_rbg_1_2w_p2_4bpp_) {
              prg_rbg_1_2w_p2_4bpp_ = compile_color_dot(
                S(a_prg_rbg_1_2w_p2_4bpp),
                prg_rbg_getcolor_4bpp,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_1_2w_p2_4bpp_;
            break;
          }
          case 1: {
            if (!prg_rbg_1_2w_p2_8bpp_) {
              prg_rbg_1_2w_p2_8bpp_ = compile_color_dot(
                S(a_prg_rbg_1_2w_p2_8bpp),
                prg_rbg_getcolor_8bpp,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_1_2w_p2_8bpp_;
            break;
          }
          case 2: {
            if (!prg_rbg_1_2w_p2_16bpp_p_) {
              prg_rbg_1_2w_p2_16bpp_p_ = compile_color_dot(
                S(a_prg_rbg_1_2w_p2_4bpp),
                prg_rbg_getcolor_16bpp_palette,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_1_2w_p2_16bpp_p_;
            break;
          }
          case 3: {
            if (!prg_rbg_1_2w_p2_16bpp_) {
              prg_rbg_1_2w_p2_16bpp_ = compile_color_dot(
                S(a_prg_rbg_1_2w_p2_4bpp),
                prg_rbg_getcolor_16bpp_rbg,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_1_2w_p2_16bpp_;
            break;
          }
          case 4: {
            if (!prg_rbg_1_2w_p2_32bpp_) {
              prg_rbg_1_2w_p2_32bpp_ = compile_color_dot(
                S(a_prg_rbg_1_2w_p2_4bpp),
                prg_rbg_getcolor_32bpp_rbg,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_1_2w_p2_32bpp_;
            break;
          }
          }
        }
      }
    }
    else if (fixVdp2Regs->RPMD == 2) {
      if (rbg->info.isbitmap) {
        switch (rbg->info.colornumber) {
        case 0: {
          if (!prg_rbg_2_2w_bitmap_4bpp_) {
            prg_rbg_2_2w_bitmap_4bpp_ = compile_color_dot(
              S(a_prg_rbg_2_2w_bitmap),
              prg_rbg_getcolor_4bpp,
              prg_generate_rbg_end);
          }
          CurrentPipeline=prg_rbg_2_2w_bitmap_4bpp_;
          break;
        }
        case 1: {
          if (!prg_rbg_2_2w_bitmap_8bpp_) {
            prg_rbg_2_2w_bitmap_8bpp_ = compile_color_dot(
              S(a_prg_rbg_2_2w_bitmap),
              prg_rbg_getcolor_8bpp,
              prg_generate_rbg_end);
          }
          CurrentPipeline=prg_rbg_2_2w_bitmap_8bpp_;
          break;
        }
        case 2: {
          if (!prg_rbg_2_2w_bitmap_16bpp_p_) {
            prg_rbg_2_2w_bitmap_16bpp_p_ = compile_color_dot(
              S(a_prg_rbg_2_2w_bitmap),
              prg_rbg_getcolor_16bpp_palette,
              prg_generate_rbg_end);
          }
          CurrentPipeline=prg_rbg_2_2w_bitmap_16bpp_p_;
          break;
        }
        case 3: {
          if (!prg_rbg_2_2w_bitmap_16bpp_) {
            prg_rbg_2_2w_bitmap_16bpp_ = compile_color_dot(
              S(a_prg_rbg_2_2w_bitmap),
              prg_rbg_getcolor_16bpp_rbg,
              prg_generate_rbg_end);
          }
          CurrentPipeline=prg_rbg_2_2w_bitmap_16bpp_;
          break;
        }
        case 4: {
          if (!prg_rbg_2_2w_bitmap_32bpp_) {
            prg_rbg_2_2w_bitmap_32bpp_ = compile_color_dot(
              S(a_prg_rbg_2_2w_bitmap),
              prg_rbg_getcolor_32bpp_rbg,
              prg_generate_rbg_end);
          }
          CurrentPipeline=prg_rbg_2_2w_bitmap_32bpp_;
          break;
        }
        }

      }
      else {
        if (rbg->info.patterndatasize == 1) {
          switch (rbg->info.colornumber) {
          case 0: { // BlukSlash
            if (!prg_rbg_2_2w_p1_4bpp_) {
              prg_rbg_2_2w_p1_4bpp_ = compile_color_dot(
                S(a_prg_rbg_2_2w_p1_4bpp),
                prg_rbg_getcolor_4bpp,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_2_2w_p1_4bpp_;
            break;
          }
          case 1: { // Panzer Dragoon Zwei, Toshiden(Title) ToDo: Sky bug
            if (!prg_rbg_2_2w_p1_8bpp_) {
              prg_rbg_2_2w_p1_8bpp_ = compile_color_dot(
                S(a_prg_rbg_2_2w_p1_8bpp),
                prg_rbg_getcolor_8bpp,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_2_2w_p1_8bpp_;
            break;
          }
          case 2: {
            if (!prg_rbg_2_2w_p1_16bpp_p_) {
              prg_rbg_2_2w_p1_16bpp_p_ = compile_color_dot(
                S(a_prg_rbg_2_2w_p1_4bpp),
                prg_rbg_getcolor_16bpp_palette,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_2_2w_p1_16bpp_p_;
            break;
          }
          case 3: {
            if (!prg_rbg_2_2w_p1_16bpp_) {
              prg_rbg_2_2w_p1_16bpp_ = compile_color_dot(
                S(a_prg_rbg_2_2w_p1_4bpp),
                prg_rbg_getcolor_16bpp_rbg,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_2_2w_p1_16bpp_;
            break;
          }
          case 4: {
            if (!prg_rbg_2_2w_p1_32bpp_) {
              prg_rbg_2_2w_p1_32bpp_ = compile_color_dot(
                S(a_prg_rbg_2_2w_p1_4bpp),
                prg_rbg_getcolor_32bpp_rbg,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_2_2w_p1_32bpp_;
            break;
          }
          }
        }
        else {
          switch (rbg->info.colornumber) {
          case 0: {
            if (!prg_rbg_2_2w_p2_4bpp_) {
              prg_rbg_2_2w_p2_4bpp_ = compile_color_dot(
                S(a_prg_rbg_2_2w_p2_4bpp),
                prg_rbg_getcolor_4bpp,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_2_2w_p2_4bpp_;
            break;
          }
          case 1: {
            if (!prg_rbg_2_2w_p2_8bpp_) {
              prg_rbg_2_2w_p2_8bpp_ = compile_color_dot(
                S(a_prg_rbg_2_2w_p2_8bpp),
                prg_rbg_getcolor_8bpp,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_2_2w_p2_8bpp_;
            break;
          }
          case 2: {
            if (!prg_rbg_2_2w_p2_16bpp_p_) {
              prg_rbg_2_2w_p2_16bpp_p_ = compile_color_dot(
                S(a_prg_rbg_2_2w_p2_4bpp),
                prg_rbg_getcolor_16bpp_palette,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_2_2w_p2_16bpp_p_;
            break;
          }
          case 3: {
            if (!prg_rbg_2_2w_p2_16bpp_) {
              prg_rbg_2_2w_p2_16bpp_ = compile_color_dot(
                S(a_prg_rbg_2_2w_p2_4bpp),
                prg_rbg_getcolor_16bpp_rbg,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_2_2w_p2_16bpp_;
            break;
          }
          case 4: {
            if (!prg_rbg_2_2w_p2_32bpp_) {
              prg_rbg_2_2w_p2_32bpp_ = compile_color_dot(
                S(a_prg_rbg_2_2w_p2_4bpp),
                prg_rbg_getcolor_32bpp_rbg,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_2_2w_p2_32bpp_;
            break;
          }
          }
        }
      }
    }
    else if (fixVdp2Regs->RPMD == 3) {

      if (rbg->info.isbitmap) {
        switch (rbg->info.colornumber) {
        case 0: {
          if (!prg_rbg_3_2w_bitmap_4bpp_) {
            prg_rbg_3_2w_bitmap_4bpp_ = compile_color_dot(
              S(a_prg_rbg_3_2w_bitmap),
              prg_rbg_getcolor_4bpp,
              prg_generate_rbg_end);
          }
          CurrentPipeline=prg_rbg_3_2w_bitmap_4bpp_;
          break;
        }
        case 1: {
          if (!prg_rbg_3_2w_bitmap_8bpp_) {
            prg_rbg_3_2w_bitmap_8bpp_ = compile_color_dot(
              S(a_prg_rbg_3_2w_bitmap),
              prg_rbg_getcolor_8bpp,
              prg_generate_rbg_end);
          }
          CurrentPipeline=prg_rbg_3_2w_bitmap_8bpp_;
          break;
        }
        case 2: {
          if (!prg_rbg_3_2w_bitmap_16bpp_p_) {
            prg_rbg_3_2w_bitmap_16bpp_p_ = compile_color_dot(
              S(a_prg_rbg_3_2w_bitmap),
              prg_rbg_getcolor_16bpp_palette,
              prg_generate_rbg_end);
          }
          CurrentPipeline=prg_rbg_3_2w_bitmap_16bpp_p_;
          break;
        }
        case 3: {
          if (!prg_rbg_3_2w_bitmap_16bpp_) {
            prg_rbg_3_2w_bitmap_16bpp_ = compile_color_dot(
              S(a_prg_rbg_3_2w_bitmap),
              prg_rbg_getcolor_16bpp_rbg,
              prg_generate_rbg_end);
          }
          CurrentPipeline=prg_rbg_3_2w_bitmap_16bpp_;
          break;
        }
        case 4: {
          if (!prg_rbg_3_2w_bitmap_32bpp_) {
            prg_rbg_3_2w_bitmap_32bpp_ = compile_color_dot(
              S(a_prg_rbg_3_2w_bitmap),
              prg_rbg_getcolor_32bpp_rbg,
              prg_generate_rbg_end);
          }
          CurrentPipeline=prg_rbg_3_2w_bitmap_32bpp_;
          break;
        }
        }

      }
      else {
        if (rbg->info.patterndatasize == 1) {
          switch (rbg->info.colornumber) {
          case 0: {
            if (!prg_rbg_3_2w_p1_4bpp_) {
              prg_rbg_3_2w_p1_4bpp_ = compile_color_dot(
                S(a_prg_rbg_3_2w_p1_4bpp),
                prg_rbg_getcolor_4bpp,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_3_2w_p1_4bpp_;
            break;
          }
          case 1: { // Final Fight Revenge, Grandia main
            if (!prg_rbg_3_2w_p1_8bpp_) {
              prg_rbg_3_2w_p1_8bpp_ = compile_color_dot(
                S(a_prg_rbg_3_2w_p1_8bpp),
                prg_rbg_getcolor_8bpp,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_3_2w_p1_8bpp_;
            break;
          }
          case 2: {
            if (!prg_rbg_3_2w_p1_16bpp_p_) {
              prg_rbg_3_2w_p1_16bpp_p_ = compile_color_dot(
                S(a_prg_rbg_3_2w_p1_4bpp),
                prg_rbg_getcolor_16bpp_palette,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_3_2w_p1_16bpp_p_;
            break;
          }
          case 3: { // Power Drift
            if (!prg_rbg_3_2w_p1_16bpp_) {
              prg_rbg_3_2w_p1_16bpp_ = compile_color_dot(
                S(a_prg_rbg_3_2w_p1_4bpp),
                prg_rbg_getcolor_16bpp_rbg,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_3_2w_p1_16bpp_;
            break;
          }
          case 4: {
            if (!prg_rbg_3_2w_p1_32bpp_) {
              prg_rbg_3_2w_p1_32bpp_ = compile_color_dot(
                S(a_prg_rbg_3_2w_p1_4bpp),
                prg_rbg_getcolor_32bpp_rbg,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_3_2w_p1_32bpp_;
            break;
          }
          }
        }
        else {
          switch (rbg->info.colornumber) {
          case 0: {
            if (!prg_rbg_3_2w_p2_4bpp_) {
              prg_rbg_3_2w_p2_4bpp_ = compile_color_dot(
                S(a_prg_rbg_3_2w_p2_4bpp),
                prg_rbg_getcolor_4bpp,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_3_2w_p2_4bpp_;
            break;
          }
          case 1: {
            if (!prg_rbg_3_2w_p2_8bpp_) {
              prg_rbg_3_2w_p2_8bpp_ = compile_color_dot(
                S(a_prg_rbg_3_2w_p2_8bpp),
                prg_rbg_getcolor_8bpp,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_3_2w_p2_8bpp_;
            break;
          }
          case 2: {
            if (!prg_rbg_3_2w_p2_16bpp_p_) {
              prg_rbg_3_2w_p2_16bpp_p_ = compile_color_dot(
                S(a_prg_rbg_3_2w_p2_4bpp),
                prg_rbg_getcolor_16bpp_palette,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_3_2w_p2_16bpp_p_;
            break;
          }
          case 3: {
            if (!prg_rbg_3_2w_p2_16bpp_) {
              prg_rbg_3_2w_p2_16bpp_ = compile_color_dot(
                S(a_prg_rbg_3_2w_p2_4bpp),
                prg_rbg_getcolor_16bpp_rbg,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_3_2w_p2_16bpp_;
            break;
          }
          case 4: {
            if (!prg_rbg_3_2w_p2_32bpp_) {
              prg_rbg_3_2w_p2_32bpp_ = compile_color_dot(
                S(a_prg_rbg_3_2w_p2_4bpp),
                prg_rbg_getcolor_32bpp_rbg,
                prg_generate_rbg_end);
            }
            CurrentPipeline=prg_rbg_3_2w_p2_32bpp_;
            break;
          }
          }
        }
      }
      // ToDo: Copy Window Info
      //glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_window_);
      //glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(vdp2WindowInfo)*(int(rbg->vres / rbg->rotate_mval_v) << rbg->info.hres_shift), (void*)rbg->info.pWinInfo);
      //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo_window_);
    }
  }



  void* data;
  if (rbg->info.pWinInfo != nullptr) {
    int wsize = sizeof(vdp2WindowInfo)*(int(rbg->vres / rbg->rotate_mval_v) << rbg->info.hres_shift);
    data = d.mapMemory(ssbo_window_.mem, 0, wsize);
    memcpy(data, (void*)rbg->info.pWinInfo, wsize);
    d.unmapMemory(ssbo_window_.mem);
  }

  data = d.mapMemory(ssbo_vram_.mem, 0, 0x80000);
  memcpy(data, Vdp2Ram, 0x80000);
  d.unmapMemory(ssbo_vram_.mem);

  if (rbg->info.specialcolormode == 3 || paraa.k_mem_type != 0 || parab.k_mem_type != 0) {
    data = d.mapMemory(ssbo_cram_.mem, 0, 0x1000);
    memcpy(data, Vdp2ColorRam, 0x1000);
    d.unmapMemory(ssbo_cram_.mem);
  }

  int asize = getAllainedSize(sizeof(vdp2rotationparameter_struct));
  data = d.mapMemory(ssbo_paraA_.mem, 0, asize*2);
  memcpy(data, &paraa, sizeof(vdp2rotationparameter_struct) );
  memcpy(((char*)data + sizeof(vdp2rotationparameter_struct)), &parab, sizeof(vdp2rotationparameter_struct));
  d.unmapMemory(ssbo_paraA_.mem);

  rbgUniformParam->hres_scale = 1.0 / rbg->rotate_mval_h;
  rbgUniformParam->vres_scale = 1.0 / rbg->rotate_mval_v;
  rbgUniformParam->cellw = rbg->info.cellw;
  rbgUniformParam->cellh = rbg->info.cellh;
  rbgUniformParam->paladdr_ = rbg->info.paladdr;
  rbgUniformParam->pagesize = rbg->pagesize;
  rbgUniformParam->patternshift = rbg->patternshift;
  rbgUniformParam->planew = rbg->info.planew;
  rbgUniformParam->pagewh = rbg->info.pagewh;
  rbgUniformParam->patterndatasize = rbg->info.patterndatasize;
  rbgUniformParam->supplementdata = rbg->info.supplementdata;
  rbgUniformParam->auxmode = rbg->info.auxmode;
  rbgUniformParam->patternwh = rbg->info.patternwh;
  rbgUniformParam->coloroffset = rbg->info.coloroffset;
  rbgUniformParam->transparencyenable = rbg->info.transparencyenable;
  rbgUniformParam->specialcolormode = rbg->info.specialcolormode;
  rbgUniformParam->specialcolorfunction = rbg->info.specialcolorfunction;
  rbgUniformParam->specialcode = rbg->info.specialcode;
  rbgUniformParam->colornumber = rbg->info.colornumber;
  rbgUniformParam->window_area_mode = rbg->info.WindwAreaMode;
  rbgUniformParam->alpha_ = (float)rbg->info.alpha / 255.0f;
  rbgUniformParam->specialprimode = rbg->info.specialprimode;
  rbgUniformParam->priority = rbg->info.priority;

  if (Vdp2Internal.ColorMode < 2) {
    rbgUniformParam->cram_shift = 1;
  }
  else {
    rbgUniformParam->cram_shift = 2;
  }
  rbgUniformParam->hires_shift = rbg->info.hres_shift;

  data = d.mapMemory(rbgUniform.mem, 0, sizeof(*rbgUniformParam));
  memcpy(data, rbgUniformParam, sizeof(*rbgUniformParam));
  d.unmapMemory(rbgUniform.mem);

  queue.waitIdle();

  //vk::CommandBufferUsageFlagBits::eSimultaneousUse

  updateDescriptorSets(texindex);

  auto c = vk::CommandBuffer(command);
  c.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
  
  vk::ImageMemoryBarrier barrierBegin;
  barrierBegin.oldLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
  barrierBegin.newLayout = vk::ImageLayout::eGeneral;
  barrierBegin.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrierBegin.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrierBegin.image = tex_surface[texindex].image;
  barrierBegin.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  barrierBegin.subresourceRange.baseMipLevel = 0;
  barrierBegin.subresourceRange.levelCount = 1;
  barrierBegin.subresourceRange.baseArrayLayer = 0;
  barrierBegin.subresourceRange.layerCount = 1;
  barrierBegin.srcAccessMask = vk::AccessFlagBits::eShaderRead;
  barrierBegin.dstAccessMask = vk::AccessFlags();

  c.pipelineBarrier(
    vk::PipelineStageFlagBits::eComputeShader,
    vk::PipelineStageFlagBits::eComputeShader,
    vk::DependencyFlags(),
    0, nullptr,
    0, nullptr,
    1, &barrierBegin);
  

  c.bindPipeline(vk::PipelineBindPoint::eCompute, CurrentPipeline);
  c.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, descriptorSet, nullptr);
  c.dispatch(tex_width_ / 16, tex_height_ / 16, 1);

  vk::ImageMemoryBarrier barrier;
  barrier.oldLayout = vk::ImageLayout::eGeneral;
  barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = tex_surface[texindex].image;
  barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.srcAccessMask = vk::AccessFlags();
  barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

  c.pipelineBarrier(
    vk::PipelineStageFlagBits::eComputeShader,
    vk::PipelineStageFlagBits::eComputeShader,
    vk::DependencyFlags(),
    0, nullptr,
    0, nullptr,
    1, &barrier);

  c.end();
    
  static const std::vector<vk::PipelineStageFlags> waitStages{ vk::PipelineStageFlagBits::eComputeShader };
  // Submit compute commands
  vk::SubmitInfo computeSubmitInfo;
  computeSubmitInfo.commandBufferCount = 1;
  computeSubmitInfo.pCommandBuffers = &c;
  //computeSubmitInfo.waitSemaphoreCount = 1;
  //computeSubmitInfo.pWaitSemaphores = &semaphores.ready;
  //computeSubmitInfo.pWaitDstStageMask = waitStages.data();
  computeSubmitInfo.signalSemaphoreCount = 1;
  computeSubmitInfo.pSignalSemaphores = &semaphores.complete;
  tex_surface[texindex].rendered = true;
  queue.submit(computeSubmitInfo, {});
  
  
  //vulkan->transitionImageLayout(VkImage(tex_surface[0].image), 
  //  VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  }

VkImageView RBGGeneratorVulkan::getTexture(int id) {
  return static_cast<VkImageView>(tex_surface[id].view);
}

void RBGGeneratorVulkan::onFinish() {

/*
  if (tex_surface[0].rendered) {
    vulkan->transitionImageLayout(VkImage(tex_surface[0].image),
      VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
    tex_surface[0].rendered = false;
  }

  if (tex_surface[1].rendered) {
    vulkan->transitionImageLayout(VkImage(tex_surface[1].image),
      VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
    tex_surface[1].rendered = false;
  }
*/
}

#endif

RBGGenerator * RBGGenerator::instance_ = nullptr;

extern "C" {
  void RBGGenerator_init(int width, int height) {
    if (_Ygl->rbg_use_compute_shader == 0) return;

    RBGGenerator * instance = RBGGenerator::getInstance();
    instance->init(width, height);
  }
  void RBGGenerator_resize(int width, int height) {
    if (_Ygl->rbg_use_compute_shader == 0) return;

    RBGGenerator * instance = RBGGenerator::getInstance();
    instance->resize(width, height);
  }
  void RBGGenerator_update(RBGDrawInfo * rbg) {
    if (_Ygl->rbg_use_compute_shader == 0) return;

    RBGGenerator * instance = RBGGenerator::getInstance();
    instance->update(rbg);
  }
  GLuint RBGGenerator_getTexture(int id) {
    if (_Ygl->rbg_use_compute_shader == 0) return 0;

    RBGGenerator * instance = RBGGenerator::getInstance();
    return instance->getTexture(id);
  }
  void RBGGenerator_onFinish() {

    if (_Ygl->rbg_use_compute_shader == 0) return;
    RBGGenerator * instance = RBGGenerator::getInstance();
    instance->onFinish();
  }
}


