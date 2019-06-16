/*
Copyright 2011-2015 Shinya Miyamoto(devmiyax)

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

extern "C"{
#include "ygl.h"
#include "yui.h"
#include "vidshared.h"
}

#define YGLDEBUG

#define DEBUGWIP

const char prg_generate_rbg[] =
SHADER_VERSION_COMPUTE
"precision highp float; \n"
"precision highp int;\n"
"precision highp image2D;\n"
"layout(local_size_x = 4, local_size_y = 4) in;\n"
"layout(rgba8, binding = 0) writeonly highp uniform image2D outSurface;\n"
"layout(std430, binding = 1) readonly buffer VDP2 { uint vram[]; };\n"
" struct vdp2rotationparameter_struct{ \n"
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
" int padding;\n"
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
"  uint alpha;\n"
"  uint priority;\n"
"  int cram_shift;\n"
"  int startLine;\n"
"  int endLine;\n"
"  uint specialprimode;\n"
"  uint specialfunction;\n"
"};\n"
// " struct vdp2WindowInfo\n"
// "{\n"
// "  int WinShowLine;\n"
// "  int WinHStart;\n"
// "  int WinHEnd;\n"
// "};\n"
// "layout(std430, binding = 4) readonly buffer windowinfo { \n"
// "  vdp2WindowInfo pWinInfo[];\n"
// "};\n"
"layout(std430, binding = 5) readonly buffer VDP2C { uint cram[]; };\n"
"layout(std430, binding = 6) readonly buffer ROTW { uint  rotWin[]; };\n"
" int GetKValue( int paramid, float posx, float posy, out float ky, out uint lineaddr ){ \n"
"  uint kdata;\n"
"  int kindex = int( ceil(para[paramid].deltaKAst*posy+(para[paramid].deltaKAx*posx)) ); \n"
"  if (para[paramid].coefdatasize == 2) { \n"
//Revoir la gestion de la vram
"    uint addr = ( uint( int(para[paramid].coeftbladdr) + (kindex<<1)) &0x7FFFFu); \n"
"    if( para[paramid].k_mem_type == 0) { \n"
"	     kdata = vram[ addr>>2 ]; \n"
"      if( (addr & 0x02u) != 0u ) { kdata >>= 16; } \n"
"      kdata = (((kdata) >> 8 & 0xFFu) | ((kdata) & 0xFFu) << 8);\n"
"    }else{\n"
"      kdata = cram[ ((0x800u + (addr&0xFFFu))>>2)  ]; \n"
"      if( (addr & 0x02u) != 0u ) { kdata >>= 16; } \n"
"    }\n"
"    if ( (kdata & 0x8000u) != 0u) { return -1; }\n"
"	   if((kdata&0x4000u)!=0u) ky=float( int(kdata&0x7FFFu)| int(0xFFFF8000u) )/1024.0; else ky=float(kdata&0x7FFFu)/1024.0;\n"
"  }else{\n"
//Revoir la gestion de la vram
"    uint addr = ( uint( int(para[paramid].coeftbladdr) + (kindex<<2))&0x7FFFFu); \n"
"    if( para[paramid].k_mem_type == 0) { \n"
"	     kdata = vram[ addr>>2 ]; \n"
"      kdata = ((kdata&0xFF000000u) >> 24 | ((kdata) >> 8 & 0xFF00u) | ((kdata) & 0xFF00u) << 8 | (kdata&0x000000FFu) << 24);\n"
"    }else{\n"
"      kdata = cram[ ((0x800u + (addr&0xFFFu) )>>2) ]; \n"
"      kdata = ((kdata&0xFFFF0000u)>>16|(kdata&0x0000FFFFu)<<16);\n"
"    }\n"
"	 if( para[paramid].linecoefenab != 0) lineaddr = (kdata >> 24) & 0x7Fu; else lineaddr = 0u;\n"
"	 if((kdata&0x80000000u)!=0u){ return -1;}\n"
"	 if((kdata&0x00800000u)!=0u) ky=float( int(kdata&0x00FFFFFFu)| int(0xFF800000u) )/65536.0; else ky=float(kdata&0x00FFFFFFu)/65536.0;\n"
"  }\n"
"  return 0;\n"
" }\n"

"bool isWindowInside(uint x, uint y)\n"
"{\n"
"  uint upLx = rotWin[y] & 0xFFFFu;\n"
"  uint upRx = (rotWin[y] >> 16) & 0xFFFFu;\n"
"  // inside\n"
"  if (window_area_mode == 1)\n"
"  {\n"
"    if (rotWin[y] == 0u) return false;\n"
"    if (x >= upLx && x <= upRx)\n"
"    {\n"
"      return true;\n"
"    }\n"
"    else {\n"
"      return false;\n"
"    }\n"
"    // outside\n"
"  }\n"
"  else {\n"
"    if (rotWin[y] == 0u) return true;\n"
"    if (x < upLx) return true;\n"
"    if (x > upRx) return true;\n"
"    return false;\n"
"  }\n"
"  return false;\n"
"}\n"

"uint get_cram_msb(uint colorindex) { \n"
"	uint colorval = 0u; \n"
"	colorindex = (colorindex << cram_shift) & 0xFFFu; \n"
"	colorval = cram[colorindex >> 2]; \n"
"	if ((colorindex & 0x02u) != 0u) { colorval >>= 16; } \n"
"	return (colorval & 0x8000u); \n"
"}\n"

" vec4 vdp2color(uint alpha_, uint prio, uint cc_on, uint index) {\n"
" uint ret = (((alpha_ & 0xF8u) | prio) << 24 | ((cc_on & 0x1u)<<16) | (index& 0xFEFFFFu));\n"
" return vec4(float((ret >> 0)&0xFFu)/255.0,float((ret >> 8)&0xFFu)/255.0, float((ret >> 16)&0xFFu)/255.0, float((ret >> 24)&0xFFu)/255.0);"
"\n}"

"uint PixelIsSpecialPriority(uint specialcode_, uint dot)\n"
"{\n"
"   dot &= 0xfu;\n"
"   if ((specialcode_ & 0x01u) != 0u)\n"
"   {\n"
"      if (dot == 0u || dot == 1u)\n"
"         return 1u;\n"
"   }\n"
"   if ((specialcode_ & 0x02u) != 0u)\n"
"   {\n"
"      if (dot == 2u || dot == 3u)\n"
"         return 1u;\n"
"   }\n"
"   if ((specialcode_ & 0x04u) != 0u)\n"
"   {\n"
"      if (dot == 4u || dot == 5u)\n"
"         return 1u;\n"
"   }\n"
"   if ((specialcode_ & 0x08u) != 0u)\n"
"   {\n"
"      if (dot == 6u || dot == 7u)\n"
"         return 1u;\n"
"   }\n"
"   if ((specialcode_ & 0x10u) != 0u)\n"
"   {\n"
"      if (dot == 8u || dot == 9u)\n"
"         return 1u;\n"
"   }\n"
"   if ((specialcode_ & 0x20u) != 0u)\n"
"   {\n"
"      if (dot == 0xau || dot == 0xbu)\n"
"         return 1u;\n"
"   }\n"
"   if ((specialcode_ & 0x40u) != 0u)\n"
"   {\n"
"      if (dot == 0xcu || dot == 0xdu)\n"
"         return 1u;\n"
"   }\n"
"   if ((specialcode_ & 0x80u) != 0u)\n"
"   {\n"
"      if (dot == 0xeu || dot == 0xfu)\n"
"         return 1u;\n"
"   }\n"
"   return 0u;\n"
"}\n"

"uint Vdp2SetSpecialPriority(uint dot) {\n"
"  uint prio = priority;\n"
"  if (specialprimode == 2u) {\n"
"    prio = priority & 0xEu;\n"
"    if ((specialfunction & 1u) != 0u) {\n"
"      if (PixelIsSpecialPriority(specialcode, dot) != 0u)\n"
"      {\n"
"        prio |= 1u;\n"
"      }\n"
"    }\n"
"  }\n"
"	return prio;\n"
"}\n"

"uint setCCOn(uint index, uint dot) {\n"
"    uint cc_ = 1u;\n"
"    switch (specialcolormode)\n"
"    {\n"
"    case 1:\n"
"      if (specialcolorfunction == 0) { cc_ = 0; } break;\n"
"    case 2:\n"
"      if (specialcolorfunction == 0) { cc_ = 0; }\n"
"      else { if ((specialcode & (1u << ((dot & 0xFu) >> 1))) == 0u) { cc_ = 0; } } \n"
"      break; \n"
"    case 3:\n"
"	   if (get_cram_msb(index) == 0u) { cc_ = 0; }\n"
"	   break;\n"
"    }\n"
"    return cc_\n;"
"}\n"



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
"  uint cc = 1u;\n"
"  int discarded = 0;\n"
"  uint priority_ = priority;\n"
"  uint patternname = 0xFFFFFFFFu;\n"
"  ivec2 texel = ivec2(gl_GlobalInvocationID.xy);\n"
"  ivec2 size = imageSize(outSurface);\n"
"  if (texel.x >= size.x || texel.y >= size.y ) return;\n"
"  float posx = float(texel.x) * hres_scale;\n"
"  float posy = float(texel.y) * vres_scale;\n"
"  if (posy < startLine || posy >= endLine ) return;\n";

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
"  if( isWindowInside( uint(posx), uint(posy) ) ) { "
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
"  uint specialfunction_in = (tmp1 & 0x2000u) >> 13;\n"
"  uint specialcolorfunction_in = (tmp1 & 0x1000u) >> 12;\n"
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
//Aligner avec Vdp2GetPixel4bpp
//Jeu de test Dead or Alive
"  uint dot = 0u;\n"
"  uint cramindex = 0u;\n"
"  uint dotaddr = ((charaddr + uint(((y * cellw) + x) >> 1)) & 0x7FFFFu);\n"
"  dot = vram[ dotaddr >> 2];\n"
"  if( (dotaddr & 0x3u) == 0u ) dot >>= 0;\n"
"  else if( (dotaddr & 0x3u) == 1u ) dot >>= 8;\n"
"  else if( (dotaddr & 0x3u) == 2u ) dot >>= 16;\n"
"  else if( (dotaddr & 0x3u) == 3u ) dot >>= 24;\n"
"  if ( (x & 0x1) == 0 ) dot >>= 4;\n"
"  if ( (dot & 0xFu) == 0u && transparencyenable != 0 ) { \n"
"    discarded = 1; \n"
"  } else {\n"
"    cramindex = (coloroffset + ((paladdr << 4) | (dot & 0xFu)));\n"
"    priority_ = Vdp2SetSpecialPriority(dot);\n"
"    cc = setCCOn(cramindex, dot);\n"
"  }\n";


// 8BPP
const char prg_rbg_getcolor_8bpp[] =
"  uint dot = 0u;\n"
"  uint cramindex = 0u;\n"
"  uint dotaddr = charaddr + uint((y*cellw)+x);\n"
"  dot = vram[ dotaddr >> 2];\n"
"  if( (dotaddr & 0x3u) == 0u ) dot >>= 0;\n"
"  else if( (dotaddr & 0x3u) == 1u ) dot >>= 8;\n"
"  else if( (dotaddr & 0x3u) == 2u ) dot >>= 16;\n"
"  else if( (dotaddr & 0x3u) == 3u ) dot >>= 24;\n"
"  dot = dot & 0xFFu; \n"
"  if ( dot == 0u && transparencyenable != 0 ) { \n"
"    discarded = 1; \n"
"  } else {\n"
"    cramindex = (coloroffset + ((paladdr << 4) | dot));\n"
"    priority_ = Vdp2SetSpecialPriority(dot);\n"
"    cc = setCCOn(cramindex, dot);\n"
"  }\n";


const char prg_rbg_getcolor_16bpp_palette[] =
"  uint dot = 0u;\n"
"  uint cramindex = 0u;\n"
"  uint dotaddr = charaddr + uint((y*cellw)+x) * 2u;\n"
"  dot = vram[dotaddr>>2]; \n"
"  if( (dotaddr & 0x02u) != 0u ) { dot >>= 16; } \n"
"  dot = (((dot) >> 8 & 0xFF) | ((dot) & 0xFF) << 8);\n"
"  if ( dot == 0 && transparencyenable != 0 ) { \n"
"    discarded = 1; \n"
"  } else {\n"
"    cramindex = (coloroffset + dot);\n"
"    priority_ = Vdp2SetSpecialPriority(dot);\n"
"    cc = setCCOn(cramindex, dot);\n"
"  }\n";

const char prg_rbg_getcolor_16bpp_rbg[] =
"  uint dot = 0u;\n"
"  uint cramindex = 0u;\n"
"  uint dotaddr = charaddr + uint((y*cellw)+x) * 2u;\n"
"  dot = vram[dotaddr>>2]; \n"
"  if( (dotaddr & 0x02u) != 0u ) { dot >>= 16; } \n"
"  dot = (((dot >> 8) & 0xFFu) | ((dot) & 0xFFu) << 8);\n"
"  if ( (dot&0x8000u) == 0u && transparencyenable != 0 ) { \n"
"    discarded = 1; \n"
"  } else {\n"
"    cramindex = (dot & 0x1Fu) << 3 | (dot & 0x3E0u) << 6 | (dot & 0x7C00u) << 9;\n"
"    cc = setCCOn(cramindex, dot);\n"
"  }\n";


const char prg_rbg_getcolor_32bpp_rbg[] =
"  uint dot = 0u;\n"
"  uint cramindex = 0u;\n"
"  uint dotaddr = charaddr + uint((y*cellw)+x) * 4u;\n"
"  dot = vram[dotaddr>>2]; \n"
"  dot = ((dot&0xFF000000u) >> 24 | ((dot >> 8) & 0xFF00u) | ((dot) & 0xFF00u) << 8 | (dot&0x000000FFu) << 24);\n"
"  if ( (dot&0x80000000u) == 0u && transparencyenable != 0 ) { \n"
"    discarded = 1; \n"
"  } else {\n"
"    cc = setCCOn(cramindex, dot);\n"
"    cramindex = dot & 0x00FFFFFFu;\n"
"  }\n";


const char prg_generate_rbg_end[] =
"  if (discarded != 0) imageStore(outSurface,texel,vec4(0.0));\n"
"  else imageStore(outSurface,texel,vdp2color(alpha, priority_, cc, cramindex));\n"
"}\n";

const char prg_generate_rbg_line_end[] =
"  if (discarded != 0) imageStore(outSurface,texel,vec4(0.0));\n"
"  else {\n"
"    cramindex |= 0x8000u;\n"
"    uint line_color = 0u;\n"
"    if( lineaddr != 0xFFFFFFFFu && lineaddr != 0u ) line_color = ((lineaddr & 0x7Fu) | 0x80u);\n"
"    cramindex |= (line_color << 16);\n"
"    imageStore(outSurface,texel,vdp2color(alpha, priority_, cc, cramindex));\n"
"  }\n"
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

//ICI
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
	prg_rbg_rpmd0_2w,
	prg_rbg_xy,
	prg_rbg_overmode_repeat,
	prg_rbg_get_patternaddr,
	prg_rbg_get_pattern_data_1w,
	prg_rbg_get_charaddr,
	prg_rbg_getcolor_4bpp,
	prg_generate_rbg_end };

const GLchar * a_prg_rbg_1_2w_p2_4bpp[] = {
	prg_generate_rbg,
	prg_rbg_rpmd0_2w,
	prg_rbg_xy,
	prg_rbg_overmode_repeat,
	prg_rbg_get_patternaddr,
	prg_rbg_get_pattern_data_2w,
	prg_rbg_get_charaddr,
	prg_rbg_getcolor_4bpp,
	prg_generate_rbg_end };

const GLchar * a_prg_rbg_1_2w_p1_8bpp[] = {
	prg_generate_rbg,
	prg_rbg_rpmd0_2w,
	prg_rbg_xy,
	prg_rbg_overmode_repeat,
	prg_rbg_get_patternaddr,
	prg_rbg_get_pattern_data_1w,
	prg_rbg_get_charaddr,
	prg_rbg_getcolor_8bpp,
	prg_generate_rbg_end };

const GLchar * a_prg_rbg_1_2w_p2_8bpp[] = {
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
    specialcolorfunction=0;
    specialcode=0;
		window_area_mode = 0;
		alpha = 0;
		priority = 0;
		cram_shift = 1;
		startLine = 0;
		endLine = 0;
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
  unsigned int alpha;
  unsigned int priority;
  int cram_shift;
	int startLine;
	int endLine;
	unsigned int specialprimode;
	unsigned int specialfunction;
};

class RBGGenerator{
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
	GLuint ssbo_rotwin_ = 0;
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

	DEBUGWIP("resize %d, %d\n",width,height);

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
	}
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

	YGLDEBUG("resize tex_surface_=%d, tex_surface_1=%d\n",tex_surface_,tex_surface_1);

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
			if( fp ) {
				for (int i = 0; i < count; i++) {
					fprintf(fp,"%s", prg_strs[i]);
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
			if( fp ) {
				for (int i = 0; i < count; i++) {
					fprintf(fp,"%s", prg_strs[i]);
				}
				fflush(fp);
				fclose(fp);
			}
		  abort();
		  delete[] info;
	  }
	  return program;
  }

  //-----------------------------------------------
  void init( int width, int height ) {

	resize(width,height);
	if (ssbo_vram_ != 0) return; // always inisialized!

DEBUGWIP("Init\n");

  glGenBuffers(1, &ssbo_vram_);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_vram_);
  glBufferData(GL_SHADER_STORAGE_BUFFER, 0x100000,(void*)Vdp2Ram,GL_DYNAMIC_DRAW);

  glGenBuffers(1, &ssbo_paraA_);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_paraA_);
  glBufferData(GL_SHADER_STORAGE_BUFFER, struct_size_*2, NULL, GL_DYNAMIC_DRAW);

  glGenBuffers(1, &scene_uniform);
  glBindBuffer(GL_UNIFORM_BUFFER, scene_uniform);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(RBGUniform), &uniform, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);

	// glGenBuffers(1, &ssbo_window_);
	// glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_window_);
	// glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(vdp2WindowInfo)*512, NULL, GL_DYNAMIC_DRAW);

	prg_rbg_0_2w_bitmap_8bpp_ = createProgram(sizeof(a_prg_rbg_0_2w_bitmap) / sizeof(char*), (const GLchar**)a_prg_rbg_0_2w_bitmap);
	prg_rbg_0_2w_p1_4bpp_ = createProgram(sizeof(a_prg_rbg_0_2w_p1_4bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_0_2w_p1_4bpp);
	prg_rbg_0_2w_p2_4bpp_ = createProgram(sizeof(a_prg_rbg_0_2w_p2_4bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_0_2w_p2_4bpp);
	prg_rbg_0_2w_p1_8bpp_ = createProgram(sizeof(a_prg_rbg_0_2w_p1_8bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_0_2w_p1_8bpp);
	prg_rbg_0_2w_p2_8bpp_ = createProgram(sizeof(a_prg_rbg_0_2w_p2_8bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_0_2w_p2_8bpp);

	prg_rbg_1_2w_bitmap_8bpp_ = createProgram(sizeof(a_prg_rbg_1_2w_bitmap) / sizeof(char*), (const GLchar**)a_prg_rbg_1_2w_bitmap);
	prg_rbg_1_2w_p1_4bpp_ = createProgram(sizeof(a_prg_rbg_1_2w_p1_4bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_1_2w_p1_4bpp);
	prg_rbg_1_2w_p2_4bpp_ = createProgram(sizeof(a_prg_rbg_1_2w_p2_4bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_1_2w_p2_4bpp);
	prg_rbg_1_2w_p1_8bpp_ = createProgram(sizeof(a_prg_rbg_1_2w_p1_8bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_1_2w_p1_8bpp);
	prg_rbg_1_2w_p2_8bpp_ = createProgram(sizeof(a_prg_rbg_1_2w_p2_8bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_1_2w_p2_8bpp);

	prg_rbg_2_2w_bitmap_8bpp_ = createProgram(sizeof(a_prg_rbg_2_2w_bitmap) / sizeof(char*), (const GLchar**)a_prg_rbg_2_2w_bitmap);
	prg_rbg_2_2w_p1_4bpp_ = createProgram(sizeof(a_prg_rbg_2_2w_p1_4bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_2_2w_p1_4bpp);
	prg_rbg_2_2w_p2_4bpp_ = createProgram(sizeof(a_prg_rbg_2_2w_p2_4bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_2_2w_p2_4bpp);
	prg_rbg_2_2w_p1_8bpp_ = createProgram(sizeof(a_prg_rbg_2_2w_p1_8bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_2_2w_p1_8bpp);
	prg_rbg_2_2w_p2_8bpp_ = createProgram( sizeof(a_prg_rbg_2_2w_p2_8bpp)/sizeof(char*) , (const GLchar**)a_prg_rbg_2_2w_p2_8bpp);

	prg_rbg_3_2w_bitmap_8bpp_ = createProgram(sizeof(a_prg_rbg_3_2w_bitmap) / sizeof(char*), (const GLchar**)a_prg_rbg_3_2w_bitmap);
	prg_rbg_3_2w_p1_4bpp_ = createProgram(sizeof(a_prg_rbg_3_2w_p1_4bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_3_2w_p1_4bpp);
	prg_rbg_3_2w_p2_4bpp_ = createProgram(sizeof(a_prg_rbg_3_2w_p2_4bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_3_2w_p2_4bpp);
	prg_rbg_3_2w_p1_8bpp_ = createProgram(sizeof(a_prg_rbg_3_2w_p1_8bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_3_2w_p1_8bpp);
	prg_rbg_3_2w_p2_8bpp_ = createProgram(sizeof(a_prg_rbg_3_2w_p2_8bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_3_2w_p2_8bpp);


	a_prg_rbg_0_2w_bitmap[sizeof(a_prg_rbg_0_2w_bitmap) / sizeof(char*) -1] = prg_generate_rbg_line_end;
	prg_rbg_0_2w_bitmap_8bpp_line_ = createProgram(sizeof(a_prg_rbg_0_2w_bitmap) / sizeof(char*), (const GLchar**)a_prg_rbg_0_2w_bitmap);

	a_prg_rbg_0_2w_p1_4bpp[sizeof(a_prg_rbg_0_2w_p1_4bpp) / sizeof(char*) - 1] = prg_generate_rbg_line_end;
	prg_rbg_0_2w_p1_4bpp_line_ = createProgram(sizeof(a_prg_rbg_0_2w_p1_4bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_0_2w_p1_4bpp);

	a_prg_rbg_0_2w_p2_4bpp[sizeof(a_prg_rbg_0_2w_p2_4bpp) / sizeof(char*) -1] = prg_generate_rbg_line_end;
	prg_rbg_0_2w_p2_4bpp_line_ = createProgram(sizeof(a_prg_rbg_0_2w_p2_4bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_0_2w_p2_4bpp);

	a_prg_rbg_0_2w_p1_8bpp[sizeof(a_prg_rbg_0_2w_p1_8bpp) / sizeof(char*) -1] = prg_generate_rbg_line_end;
	prg_rbg_0_2w_p1_8bpp_line_ = createProgram(sizeof(a_prg_rbg_0_2w_p1_8bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_0_2w_p1_8bpp);

	a_prg_rbg_0_2w_p2_8bpp[sizeof(a_prg_rbg_0_2w_p2_8bpp) / sizeof(char*) -1] = prg_generate_rbg_line_end;
	prg_rbg_0_2w_p2_8bpp_line_ = createProgram(sizeof(a_prg_rbg_0_2w_p2_8bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_0_2w_p2_8bpp);

	a_prg_rbg_1_2w_bitmap[sizeof(a_prg_rbg_1_2w_bitmap) / sizeof(char*) - 1] = prg_generate_rbg_line_end;
	prg_rbg_1_2w_bitmap_8bpp_line_ = createProgram(sizeof(a_prg_rbg_1_2w_bitmap) / sizeof(char*), (const GLchar**)a_prg_rbg_1_2w_bitmap);

	a_prg_rbg_1_2w_p1_4bpp[sizeof(a_prg_rbg_1_2w_p1_4bpp) / sizeof(char*) - 1] = prg_generate_rbg_line_end;
	prg_rbg_1_2w_p1_4bpp_line_ = createProgram(sizeof(a_prg_rbg_1_2w_p1_4bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_1_2w_p1_4bpp);

	a_prg_rbg_1_2w_p2_4bpp[sizeof(a_prg_rbg_1_2w_p2_4bpp) / sizeof(char*) - 1] = prg_generate_rbg_line_end;
	prg_rbg_1_2w_p2_4bpp_line_ = createProgram(sizeof(a_prg_rbg_1_2w_p2_4bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_1_2w_p2_4bpp);

	a_prg_rbg_1_2w_p1_8bpp[sizeof(a_prg_rbg_1_2w_p1_8bpp) / sizeof(char*) - 1] = prg_generate_rbg_line_end;
	prg_rbg_1_2w_p1_8bpp_line_ = createProgram(sizeof(a_prg_rbg_1_2w_p1_8bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_1_2w_p1_8bpp);

	a_prg_rbg_1_2w_p2_8bpp[sizeof(a_prg_rbg_1_2w_p2_8bpp) / sizeof(char*) - 1] = prg_generate_rbg_line_end;
	prg_rbg_1_2w_p2_8bpp_line_ = createProgram(sizeof(a_prg_rbg_1_2w_p2_8bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_1_2w_p2_8bpp);

	a_prg_rbg_2_2w_bitmap[sizeof(a_prg_rbg_2_2w_bitmap) / sizeof(char*) - 1] = prg_generate_rbg_line_end;
	prg_rbg_2_2w_bitmap_8bpp_line_ = createProgram(sizeof(a_prg_rbg_2_2w_bitmap) / sizeof(char*), (const GLchar**)a_prg_rbg_2_2w_bitmap);

	a_prg_rbg_2_2w_p1_4bpp[sizeof(a_prg_rbg_2_2w_p1_4bpp) / sizeof(char*) - 1] = prg_generate_rbg_line_end;
	prg_rbg_2_2w_p1_4bpp_line_ = createProgram(sizeof(a_prg_rbg_2_2w_p1_4bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_2_2w_p1_4bpp);

	a_prg_rbg_2_2w_p2_4bpp[sizeof(a_prg_rbg_2_2w_p2_4bpp) / sizeof(char*) - 1] = prg_generate_rbg_line_end;
	prg_rbg_2_2w_p2_4bpp_line_ = createProgram(sizeof(a_prg_rbg_2_2w_p2_4bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_2_2w_p2_4bpp);

	a_prg_rbg_2_2w_p1_8bpp[sizeof(a_prg_rbg_2_2w_p1_8bpp) / sizeof(char*) - 1] = prg_generate_rbg_line_end;
	prg_rbg_2_2w_p1_8bpp_line_ = createProgram(sizeof(a_prg_rbg_2_2w_p1_8bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_2_2w_p1_8bpp);

	a_prg_rbg_2_2w_p2_8bpp[sizeof(a_prg_rbg_2_2w_p2_8bpp) / sizeof(char*) - 1] = prg_generate_rbg_line_end;
	prg_rbg_2_2w_p2_8bpp_line_ = createProgram(sizeof(a_prg_rbg_2_2w_p2_8bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_2_2w_p2_8bpp);

	a_prg_rbg_3_2w_bitmap[sizeof(a_prg_rbg_3_2w_bitmap) / sizeof(char*) - 1] = prg_generate_rbg_line_end;
	prg_rbg_3_2w_bitmap_8bpp_line_ = createProgram(sizeof(a_prg_rbg_3_2w_bitmap) / sizeof(char*), (const GLchar**)a_prg_rbg_3_2w_bitmap);

	a_prg_rbg_3_2w_p1_4bpp[sizeof(a_prg_rbg_3_2w_p1_4bpp) / sizeof(char*) - 1] = prg_generate_rbg_line_end;
	prg_rbg_3_2w_p1_4bpp_line_ = createProgram(sizeof(a_prg_rbg_3_2w_p1_4bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_3_2w_p1_4bpp);

	a_prg_rbg_3_2w_p2_4bpp[sizeof(a_prg_rbg_3_2w_p2_4bpp) / sizeof(char*) - 1] = prg_generate_rbg_line_end;
	prg_rbg_3_2w_p2_4bpp_line_ = createProgram(sizeof(a_prg_rbg_3_2w_p2_4bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_3_2w_p2_4bpp);

	a_prg_rbg_3_2w_p1_8bpp[sizeof(a_prg_rbg_3_2w_p1_8bpp) / sizeof(char*) - 1] = prg_generate_rbg_line_end;
	prg_rbg_3_2w_p1_8bpp_line_ = createProgram(sizeof(a_prg_rbg_3_2w_p1_8bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_3_2w_p1_8bpp);

	a_prg_rbg_3_2w_p2_8bpp[sizeof(a_prg_rbg_3_2w_p2_8bpp) / sizeof(char*) - 1] = prg_generate_rbg_line_end;
	prg_rbg_3_2w_p2_8bpp_line_ = createProgram(sizeof(a_prg_rbg_3_2w_p2_8bpp) / sizeof(char*), (const GLchar**)a_prg_rbg_3_2w_p2_8bpp);

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

  GLuint compile_color_dot( const char * base[], int size, const char * color, const char * dot) {
	  base[size - 2] = color;
	  base[size - 1] = dot;
	  return createProgram(size, (const GLchar**)base);
  }

  //-----------------------------------------------
  void updateRBG0( RBGDrawInfo * rbg, Vdp2 *varVdp2Regs) {
		// Line color insersion
		if (rbg->info.LineColorBase != 0 && VDP2_CC_NONE != (rbg->info.blendmode & 0x03)) {
			if (varVdp2Regs->RPMD == 0 || (varVdp2Regs->RPMD == 3 && (varVdp2Regs->WCTLD & 0xA) == 0)) {
				if (rbg->info.isbitmap) {
					DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_0_2w_bitmap_8bpp_line_);
				}
				else {
					if (rbg->info.patterndatasize == 1) {
						switch (rbg->info.colornumber) {
						case 0: { // Decathalete ToDo: Line Color Bug
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_0_2w_p1_4bpp_line_);
							break;
						}
						case 1: { // Sakatuku 2 Ground, GUNDAM Side Story 2, SonicR ToDo: 2Player
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_0_2w_p1_8bpp_line_);
							break;
						}
						case 2: {
							if (prg_rbg_0_2w_p1_16bpp_p_line_ == 0) {
								prg_rbg_0_2w_p1_16bpp_p_line_ = compile_color_dot(
									S(a_prg_rbg_0_2w_p1_4bpp),
									prg_rbg_getcolor_16bpp_palette,
									prg_generate_rbg_line_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_0_2w_p1_16bpp_p_line_);
							break;
						}
						case 3: {
							if (prg_rbg_0_2w_p1_16bpp_line_ == 0) {
								prg_rbg_0_2w_p1_16bpp_line_ = compile_color_dot(
									S(a_prg_rbg_0_2w_p1_4bpp),
									prg_rbg_getcolor_16bpp_rbg,
									prg_generate_rbg_line_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_0_2w_p1_16bpp_line_);
							break;
						}
						case 4: {
							if (prg_rbg_0_2w_p1_32bpp_line_ == 0) {
								prg_rbg_0_2w_p1_32bpp_line_ = compile_color_dot(
									S(a_prg_rbg_0_2w_p1_4bpp),
									prg_rbg_getcolor_32bpp_rbg,
									prg_generate_rbg_line_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_0_2w_p1_32bpp_line_);
							break;
						}
						}
					}
					else {
						switch (rbg->info.colornumber) {
						case 0: {
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_0_2w_p2_4bpp_line_);
							break;
						}
						case 1: { // Thunder Force V
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_0_2w_p2_8bpp_line_);
							break;
						}
						case 2: {
							if (prg_rbg_0_2w_p2_16bpp_p_line_ == 0) {
								prg_rbg_0_2w_p2_16bpp_p_line_ = compile_color_dot(
									S(a_prg_rbg_0_2w_p2_4bpp),
									prg_rbg_getcolor_16bpp_palette,
									prg_generate_rbg_line_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_0_2w_p2_16bpp_p_line_);
							break;
						}
						case 3: {
							if (prg_rbg_0_2w_p2_16bpp_line_ == 0) {
								prg_rbg_0_2w_p2_16bpp_line_ = compile_color_dot(
									S(a_prg_rbg_0_2w_p2_4bpp),
									prg_rbg_getcolor_16bpp_rbg,
									prg_generate_rbg_line_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_0_2w_p2_16bpp_line_);
							break;
						}
						case 4: {
							if (prg_rbg_0_2w_p2_32bpp_line_ == 0) {
								prg_rbg_0_2w_p2_32bpp_line_ = compile_color_dot(
									S(a_prg_rbg_0_2w_p2_4bpp),
									prg_rbg_getcolor_32bpp_rbg,
									prg_generate_rbg_line_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_0_2w_p2_32bpp_line_);
							break;
						}
						}
					}
				}
			}
			else if (varVdp2Regs->RPMD == 1) {
				if (rbg->info.isbitmap) {
					DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_bitmap_8bpp_line_);
				}
				else {
					if (rbg->info.patterndatasize == 1) {
						switch (rbg->info.colornumber) {
						case 0: {
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_p1_4bpp_line_);
							break;
						}
						case 1: {
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_p1_8bpp_line_);
							break;
						}
						case 2: {
							if (prg_rbg_1_2w_p1_16bpp_p_line_ == 0) {
								prg_rbg_1_2w_p1_16bpp_p_line_ = compile_color_dot(
									S(a_prg_rbg_1_2w_p1_4bpp),
									prg_rbg_getcolor_16bpp_palette,
									prg_generate_rbg_line_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_p1_16bpp_p_line_);
							break;
						}
						case 3: {
							if (prg_rbg_1_2w_p1_16bpp_line_ == 0) {
								prg_rbg_1_2w_p1_16bpp_line_ = compile_color_dot(
									S(a_prg_rbg_1_2w_p1_4bpp),
									prg_rbg_getcolor_16bpp_rbg,
									prg_generate_rbg_line_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_p1_16bpp_line_);
							break;
						}
						case 4: {
							if (prg_rbg_1_2w_p1_32bpp_line_ == 0) {
								prg_rbg_1_2w_p1_32bpp_line_ = compile_color_dot(
									S(a_prg_rbg_0_2w_p1_4bpp),
									prg_rbg_getcolor_32bpp_rbg,
									prg_generate_rbg_line_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_p1_32bpp_line_);
							break;
						}
						}
					}
					else {
						switch (rbg->info.colornumber) {
						case 0: {
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_p2_4bpp_line_);
							break;
						}
						case 1: {
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_p2_8bpp_line_);
							break;
						}
						case 2: {
							if (prg_rbg_1_2w_p2_16bpp_p_line_ == 0) {
								prg_rbg_1_2w_p2_16bpp_p_line_ = compile_color_dot(
									S(a_prg_rbg_1_2w_p2_4bpp),
									prg_rbg_getcolor_16bpp_palette,
									prg_generate_rbg_line_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_p2_16bpp_p_line_);
							break;
						}
						case 3: {
							if (prg_rbg_1_2w_p2_16bpp_line_ == 0) {
								prg_rbg_1_2w_p2_16bpp_line_ = compile_color_dot(
									S(a_prg_rbg_1_2w_p2_4bpp),
									prg_rbg_getcolor_16bpp_rbg,
									prg_generate_rbg_line_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_p2_16bpp_line_);
							break;
						}
						case 4: {
							if (prg_rbg_1_2w_p2_32bpp_line_ == 0) {
								prg_rbg_1_2w_p2_32bpp_line_ = compile_color_dot(
									S(a_prg_rbg_1_2w_p2_4bpp),
									prg_rbg_getcolor_32bpp_rbg,
									prg_generate_rbg_line_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_p2_32bpp_line_);
							break;
						}
						}
					}
				}
			}
			else if (varVdp2Regs->RPMD == 2) {
				if (rbg->info.isbitmap) {
					DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_2_2w_bitmap_8bpp_line_);
				}
				else {
					if (rbg->info.patterndatasize == 1) {
						switch (rbg->info.colornumber) {
						case 0: {
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_2_2w_p1_4bpp_line_);
							break;
						}
						case 1: { // Panzer Dragoon 1
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_2_2w_p1_8bpp_line_);
							break;
						}
						case 2: {
							if (prg_rbg_2_2w_p1_16bpp_p_line_ == 0) {
								prg_rbg_2_2w_p1_16bpp_p_line_ = compile_color_dot(
									S(a_prg_rbg_2_2w_p1_4bpp),
									prg_rbg_getcolor_16bpp_palette,
									prg_generate_rbg_line_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_2_2w_p1_16bpp_p_line_);
							break;
						}
						case 3: {
							if (prg_rbg_2_2w_p1_16bpp_line_ == 0) {
								prg_rbg_2_2w_p1_16bpp_line_ = compile_color_dot(
									S(a_prg_rbg_2_2w_p1_4bpp),
									prg_rbg_getcolor_16bpp_rbg,
									prg_generate_rbg_line_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_2_2w_p1_16bpp_line_);
							break;
						}
						case 4: {
							if (prg_rbg_2_2w_p1_32bpp_line_ == 0) {
								prg_rbg_2_2w_p1_32bpp_line_ = compile_color_dot(
									S(a_prg_rbg_2_2w_p1_4bpp),
									prg_rbg_getcolor_32bpp_rbg,
									prg_generate_rbg_line_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_2_2w_p1_32bpp_line_);
							break;
						}
						}
					}
					else {
						switch (rbg->info.colornumber) {
						case 0: {
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_2_2w_p2_4bpp_line_);
							break;
						}
						case 1: {
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_2_2w_p2_8bpp_line_);
							break;
						}
						case 2: {
							if (prg_rbg_2_2w_p2_16bpp_p_line_ == 0) {
								prg_rbg_2_2w_p2_16bpp_p_line_ = compile_color_dot(
									S(a_prg_rbg_2_2w_p2_4bpp),
									prg_rbg_getcolor_16bpp_palette,
									prg_generate_rbg_line_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_2_2w_p2_16bpp_p_line_);
							break;
						}
						case 3: {
							if (prg_rbg_2_2w_p2_16bpp_line_ == 0) {
								prg_rbg_2_2w_p2_16bpp_line_ = compile_color_dot(
									S(a_prg_rbg_2_2w_p2_4bpp),
									prg_rbg_getcolor_16bpp_rbg,
									prg_generate_rbg_line_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_2_2w_p2_16bpp_line_);
							break;
						}
						case 4: {
							if (prg_rbg_2_2w_p2_32bpp_line_ == 0) {
								prg_rbg_2_2w_p2_32bpp_line_ = compile_color_dot(
									S(a_prg_rbg_2_2w_p2_4bpp),
									prg_rbg_getcolor_32bpp_rbg,
									prg_generate_rbg_line_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_2_2w_p2_32bpp_line_);
							break;
						}
						}
					}
				}
			}
			else if (varVdp2Regs->RPMD == 3) {

				if (rbg->info.isbitmap) {
					DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_3_2w_bitmap_8bpp_line_);
				}
				else {
					if (rbg->info.patterndatasize == 1) {
						switch (rbg->info.colornumber) {
						case 0: {
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_3_2w_p1_4bpp_line_);
							break;
						}
						case 1: {
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_3_2w_p1_8bpp_line_);
							break;
						}
						case 2: {
							if (prg_rbg_3_2w_p1_16bpp_p_line_ == 0) {
								prg_rbg_3_2w_p1_16bpp_p_line_ = compile_color_dot(
									S(a_prg_rbg_3_2w_p1_4bpp),
									prg_rbg_getcolor_16bpp_palette,
									prg_generate_rbg_line_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_3_2w_p1_16bpp_p_line_);
							break;
						}
						case 3: {
							if (prg_rbg_3_2w_p1_16bpp_line_ == 0) {
								prg_rbg_3_2w_p1_16bpp_line_ = compile_color_dot(
									S(a_prg_rbg_3_2w_p1_4bpp),
									prg_rbg_getcolor_16bpp_rbg,
									prg_generate_rbg_line_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_3_2w_p1_16bpp_line_);
							break;
						}
						case 4: {
							if (prg_rbg_3_2w_p1_32bpp_line_ == 0) {
								prg_rbg_3_2w_p1_32bpp_line_ = compile_color_dot(
									S(a_prg_rbg_3_2w_p1_4bpp),
									prg_rbg_getcolor_32bpp_rbg,
									prg_generate_rbg_line_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_3_2w_p1_32bpp_line_);
							break;
						}
						}
					}
					else {
						switch (rbg->info.colornumber) {
						case 0: {
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_3_2w_p2_4bpp_line_);
							break;
						}
						case 1: {
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_3_2w_p2_8bpp_line_);
							break;
						}
						case 2: {
							if (prg_rbg_3_2w_p2_16bpp_p_line_ == 0) {
								prg_rbg_3_2w_p2_16bpp_p_line_ = compile_color_dot(
									S(a_prg_rbg_3_2w_p2_4bpp),
									prg_rbg_getcolor_16bpp_palette,
									prg_generate_rbg_line_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_3_2w_p2_16bpp_p_line_);
							break;
						}
						case 3: {
							if (prg_rbg_3_2w_p2_16bpp_line_ == 0) {
								prg_rbg_3_2w_p2_16bpp_line_ = compile_color_dot(
									S(a_prg_rbg_3_2w_p2_4bpp),
									prg_rbg_getcolor_16bpp_rbg,
									prg_generate_rbg_line_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_3_2w_p2_16bpp_line_);
							break;
						}
						case 4: {
							if (prg_rbg_3_2w_p2_32bpp_line_ == 0) {
								prg_rbg_3_2w_p2_32bpp_line_ = compile_color_dot(
									S(a_prg_rbg_3_2w_p2_4bpp),
									prg_rbg_getcolor_32bpp_rbg,
									prg_generate_rbg_line_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_3_2w_p2_32bpp_line_);
							break;
						}
						}
					}
				}
				// glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_window_);
				// glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(vdp2WindowInfo)*rbg->vres, (void*)rbg->info.pWinInfo);
				// glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo_window_);
			}

		}

		// no line color insersion
		else {
			if (varVdp2Regs->RPMD == 0 || (varVdp2Regs->RPMD == 3 && (varVdp2Regs->WCTLD & 0xA) == 0) ) {
				if (rbg->info.isbitmap) {
					switch (rbg->info.colornumber) {
					case 0: {
						if (prg_rbg_0_2w_bitmap_4bpp_ == 0) {
							prg_rbg_0_2w_bitmap_4bpp_ = compile_color_dot(
								S(a_prg_rbg_0_2w_bitmap),
								prg_rbg_getcolor_4bpp,
								prg_generate_rbg_end);
						}
						DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_0_2w_bitmap_4bpp_);
						break;
					}
					case 1: { // SF3S1( Initial )
						DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_0_2w_bitmap_8bpp_);
						break;
					}
					case 2: {
						if (prg_rbg_0_2w_bitmap_16bpp_p_ == 0) {
							prg_rbg_0_2w_bitmap_16bpp_p_ = compile_color_dot(
								S(a_prg_rbg_0_2w_bitmap),
								prg_rbg_getcolor_16bpp_palette,
								prg_generate_rbg_end);
						}
						DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_0_2w_bitmap_16bpp_p_);
						break;
					}
					case 3: { // NHL 97 Title, GRANDIA Title
						if (prg_rbg_0_2w_bitmap_16bpp_ == 0) {
							prg_rbg_0_2w_bitmap_16bpp_ = compile_color_dot(
								S(a_prg_rbg_0_2w_bitmap),
								prg_rbg_getcolor_16bpp_rbg,
								prg_generate_rbg_end);
						}
						DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_0_2w_bitmap_16bpp_);
						break;
					}
					case 4: {
						if (prg_rbg_0_2w_bitmap_32bpp_ == 0) {
							prg_rbg_0_2w_bitmap_32bpp_ = compile_color_dot(
								S(a_prg_rbg_0_2w_bitmap),
								prg_rbg_getcolor_32bpp_rbg,
								prg_generate_rbg_end);
						}
						DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_0_2w_bitmap_32bpp_);
						break;
					}
					}
				}
				else {
					if (rbg->info.patterndatasize == 1) {
						switch (rbg->info.colornumber) {
							case 0: { // Dead or Alive, Radiant Silver Gun, Diehard
								DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_0_2w_p1_4bpp_);
								break;
							}
							case 1: { // Sakatuku 2 ( Initial Setting ), Virtua Fighter 2, Virtual-on
								DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_0_2w_p1_8bpp_);
								break;
							}
							case 2: {
								if (prg_rbg_0_2w_p1_16bpp_p_ == 0) {
									prg_rbg_0_2w_p1_16bpp_p_ = compile_color_dot(
										S(a_prg_rbg_0_2w_p1_4bpp),
										prg_rbg_getcolor_16bpp_palette,
										prg_generate_rbg_end);
								}
								DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_0_2w_p1_16bpp_p_);
								break;
							}
							case 3: {
								if (prg_rbg_0_2w_p1_16bpp_ == 0) {
									prg_rbg_0_2w_p1_16bpp_ = compile_color_dot(
										S(a_prg_rbg_0_2w_p1_4bpp),
										prg_rbg_getcolor_16bpp_rbg,
										prg_generate_rbg_end);
								}
								DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_0_2w_p1_16bpp_);
								break;
							}
							case 4: {
								if (prg_rbg_0_2w_p1_32bpp_ == 0) {
									prg_rbg_0_2w_p1_32bpp_ = compile_color_dot(
										S(a_prg_rbg_0_2w_p1_4bpp),
										prg_rbg_getcolor_32bpp_rbg,
										prg_generate_rbg_end);
								}
								DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_0_2w_p1_32bpp_);
								break;
							}
						}
					}
					else {
						switch (rbg->info.colornumber) {
						case 0: {
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_0_2w_p2_4bpp_);
							break;
						}
						case 1: { // NHL97(In Game), BIOS
							//ICI
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_0_2w_p2_8bpp_);
							break;
						}
						case 2: {
							if (prg_rbg_0_2w_p2_16bpp_p_ == 0) {
								prg_rbg_0_2w_p2_16bpp_p_ = compile_color_dot(
									S(a_prg_rbg_0_2w_p2_4bpp),
									prg_rbg_getcolor_16bpp_palette,
									prg_generate_rbg_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_0_2w_p2_16bpp_p_);
							break;
						}
						case 3: {
							if (prg_rbg_0_2w_p2_16bpp_ == 0) {
								prg_rbg_0_2w_p2_16bpp_ = compile_color_dot(
									S(a_prg_rbg_0_2w_p2_4bpp),
									prg_rbg_getcolor_16bpp_rbg,
									prg_generate_rbg_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_0_2w_p2_16bpp_);
							break;
						}
						case 4: {
							if (prg_rbg_0_2w_p2_32bpp_ == 0) {
								prg_rbg_0_2w_p2_32bpp_ = compile_color_dot(
									S(a_prg_rbg_0_2w_p2_4bpp),
									prg_rbg_getcolor_32bpp_rbg,
									prg_generate_rbg_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_0_2w_p2_32bpp_);
							break;
						}
						}
					}
				}
			}
			else if (varVdp2Regs->RPMD == 1) {
				if (rbg->info.isbitmap) {
					switch (rbg->info.colornumber) {
					case 0: {
						if (prg_rbg_1_2w_bitmap_4bpp_ == 0) {
							prg_rbg_1_2w_bitmap_4bpp_ = compile_color_dot(
								S(a_prg_rbg_1_2w_bitmap),
								prg_rbg_getcolor_4bpp,
								prg_generate_rbg_end);
						}
						DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_bitmap_4bpp_);
						break;
					}
					case 1: {
						DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_bitmap_8bpp_);
						break;
					}
					case 2: {
						if (prg_rbg_1_2w_bitmap_16bpp_p_ == 0) {
							prg_rbg_1_2w_bitmap_16bpp_p_ = compile_color_dot(
								S(a_prg_rbg_1_2w_bitmap),
								prg_rbg_getcolor_16bpp_palette,
								prg_generate_rbg_end);
						}
						DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_bitmap_16bpp_p_);
						break;
					}
					case 3: {
						if (prg_rbg_1_2w_bitmap_16bpp_ == 0) {
							prg_rbg_1_2w_bitmap_16bpp_ = compile_color_dot(
								S(a_prg_rbg_1_2w_bitmap),
								prg_rbg_getcolor_16bpp_rbg,
								prg_generate_rbg_end);
						}
						DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_bitmap_16bpp_);
						break;
					}
					case 4: {
						if (prg_rbg_1_2w_bitmap_32bpp_ == 0) {
							prg_rbg_1_2w_bitmap_32bpp_ = compile_color_dot(
								S(a_prg_rbg_1_2w_bitmap),
								prg_rbg_getcolor_32bpp_rbg,
								prg_generate_rbg_end);
						}
						DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_bitmap_32bpp_);
						break;
					}
					}
				}
				else {
					if (rbg->info.patterndatasize == 1) {
						switch (rbg->info.colornumber) {
						case 0: {
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_p1_4bpp_);
							break;
						}
						case 1: {
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_p1_8bpp_);
							break;
						}
						case 2: {
							if (prg_rbg_1_2w_p1_16bpp_p_ == 0) {
								prg_rbg_1_2w_p1_16bpp_p_ = compile_color_dot(
									S(a_prg_rbg_1_2w_p1_4bpp),
									prg_rbg_getcolor_16bpp_palette,
									prg_generate_rbg_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_p1_16bpp_p_);
							break;
						}
						case 3: {
							if (prg_rbg_1_2w_p1_16bpp_ == 0) {
								prg_rbg_1_2w_p1_16bpp_ = compile_color_dot(
									S(a_prg_rbg_1_2w_p1_4bpp),
									prg_rbg_getcolor_16bpp_rbg,
									prg_generate_rbg_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_p1_16bpp_);
							break;
						}
						case 4: {
							if (prg_rbg_1_2w_p1_32bpp_ == 0) {
								prg_rbg_1_2w_p1_32bpp_ = compile_color_dot(
									S(a_prg_rbg_0_2w_p1_4bpp),
									prg_rbg_getcolor_32bpp_rbg,
									prg_generate_rbg_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_p1_32bpp_);
							break;
						}
						}
					}
					else {
						switch (rbg->info.colornumber) {
						case 0: {
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_p2_4bpp_);
							break;
						}
						case 1: {
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_p2_8bpp_);
							break;
						}
						case 2: {
							if (prg_rbg_1_2w_p2_16bpp_p_ == 0) {
								prg_rbg_1_2w_p2_16bpp_p_ = compile_color_dot(
									S(a_prg_rbg_1_2w_p2_4bpp),
									prg_rbg_getcolor_16bpp_palette,
									prg_generate_rbg_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_p2_16bpp_p_);
							break;
						}
						case 3: {
							if (prg_rbg_1_2w_p2_16bpp_ == 0) {
								prg_rbg_1_2w_p2_16bpp_ = compile_color_dot(
									S(a_prg_rbg_1_2w_p2_4bpp),
									prg_rbg_getcolor_16bpp_rbg,
									prg_generate_rbg_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_p2_16bpp_);
							break;
						}
						case 4: {
							if (prg_rbg_1_2w_p2_32bpp_ == 0) {
								prg_rbg_1_2w_p2_32bpp_ = compile_color_dot(
									S(a_prg_rbg_1_2w_p2_4bpp),
									prg_rbg_getcolor_32bpp_rbg,
									prg_generate_rbg_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_p2_32bpp_);
							break;
						}
						}
					}
				}
			}
			else if (varVdp2Regs->RPMD == 2) {
					if (rbg->info.isbitmap) {
						switch (rbg->info.colornumber) {
						case 0: {
							if (prg_rbg_2_2w_bitmap_4bpp_ == 0) {
								prg_rbg_2_2w_bitmap_4bpp_ = compile_color_dot(
									S(a_prg_rbg_2_2w_bitmap),
									prg_rbg_getcolor_4bpp,
									prg_generate_rbg_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_2_2w_bitmap_4bpp_);
							break;
						}
						case 1: {
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_2_2w_bitmap_8bpp_);
							break;
						}
						case 2: {
							if (prg_rbg_2_2w_bitmap_16bpp_p_ == 0) {
								prg_rbg_2_2w_bitmap_16bpp_p_ = compile_color_dot(
									S(a_prg_rbg_2_2w_bitmap),
									prg_rbg_getcolor_16bpp_palette,
									prg_generate_rbg_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_2_2w_bitmap_16bpp_p_);
							break;
						}
						case 3: {
							if (prg_rbg_2_2w_bitmap_16bpp_ == 0) {
								prg_rbg_2_2w_bitmap_16bpp_ = compile_color_dot(
									S(a_prg_rbg_2_2w_bitmap),
									prg_rbg_getcolor_16bpp_rbg,
									prg_generate_rbg_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_2_2w_bitmap_16bpp_);
							break;
						}
						case 4: {
							if (prg_rbg_2_2w_bitmap_32bpp_ == 0) {
								prg_rbg_2_2w_bitmap_32bpp_ = compile_color_dot(
									S(a_prg_rbg_2_2w_bitmap),
									prg_rbg_getcolor_32bpp_rbg,
									prg_generate_rbg_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_2_2w_bitmap_32bpp_);
							break;
						}
						}

				}
				else {
					if (rbg->info.patterndatasize == 1) {
						switch (rbg->info.colornumber) {
						case 0: { // BlukSlash
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_2_2w_p1_4bpp_);
							break;
						}
						case 1: { // Panzer Dragoon Zwei, Toshiden(Title) ToDo: Sky bug
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_2_2w_p1_8bpp_);
							break;
						}
						case 2: {
							if (prg_rbg_2_2w_p1_16bpp_p_ == 0) {
								prg_rbg_2_2w_p1_16bpp_p_ = compile_color_dot(
									S(a_prg_rbg_2_2w_p1_4bpp),
									prg_rbg_getcolor_16bpp_palette,
									prg_generate_rbg_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_2_2w_p1_16bpp_p_);
							break;
						}
						case 3: {
							if (prg_rbg_2_2w_p1_16bpp_ == 0) {
								prg_rbg_2_2w_p1_16bpp_ = compile_color_dot(
									S(a_prg_rbg_2_2w_p1_4bpp),
									prg_rbg_getcolor_16bpp_rbg,
									prg_generate_rbg_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_2_2w_p1_16bpp_);
							break;
						}
						case 4: {
							if (prg_rbg_2_2w_p1_32bpp_ == 0) {
								prg_rbg_2_2w_p1_32bpp_ = compile_color_dot(
									S(a_prg_rbg_2_2w_p1_4bpp),
									prg_rbg_getcolor_32bpp_rbg,
									prg_generate_rbg_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_2_2w_p1_32bpp_);
							break;
						}
						}
					}
					else {
						switch (rbg->info.colornumber) {
						case 0: {
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_2_2w_p2_4bpp_);
							break;
						}
						case 1: {
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_2_2w_p2_8bpp_);
							break;
						}
						case 2: {
							if (prg_rbg_2_2w_p2_16bpp_p_ == 0) {
								prg_rbg_2_2w_p2_16bpp_p_ = compile_color_dot(
									S(a_prg_rbg_2_2w_p2_4bpp),
									prg_rbg_getcolor_16bpp_palette,
									prg_generate_rbg_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_2_2w_p2_16bpp_p_);
							break;
						}
						case 3: {
							if (prg_rbg_2_2w_p2_16bpp_ == 0) {
								prg_rbg_2_2w_p2_16bpp_ = compile_color_dot(
									S(a_prg_rbg_2_2w_p2_4bpp),
									prg_rbg_getcolor_16bpp_rbg,
									prg_generate_rbg_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_2_2w_p2_16bpp_);
							break;
						}
						case 4: {
							if (prg_rbg_2_2w_p2_32bpp_ == 0) {
								prg_rbg_2_2w_p2_32bpp_ = compile_color_dot(
									S(a_prg_rbg_2_2w_p2_4bpp),
									prg_rbg_getcolor_32bpp_rbg,
									prg_generate_rbg_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_2_2w_p2_32bpp_);
							break;
						}
						}
					}
				}
			}
			else if (varVdp2Regs->RPMD == 3) {
				if (rbg->info.isbitmap) {
					switch (rbg->info.colornumber) {
					case 0: {
						if (prg_rbg_3_2w_bitmap_4bpp_ == 0) {
							prg_rbg_3_2w_bitmap_4bpp_ = compile_color_dot(
								S(a_prg_rbg_3_2w_bitmap),
								prg_rbg_getcolor_4bpp,
								prg_generate_rbg_end);
						}
						DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_3_2w_bitmap_4bpp_);
						break;
					}
					case 1: {
						DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_3_2w_bitmap_8bpp_);
						break;
					}
					case 2: {
						if (prg_rbg_3_2w_bitmap_16bpp_p_ == 0) {
							prg_rbg_3_2w_bitmap_16bpp_p_ = compile_color_dot(
								S(a_prg_rbg_3_2w_bitmap),
								prg_rbg_getcolor_16bpp_palette,
								prg_generate_rbg_end);
						}
						DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_3_2w_bitmap_16bpp_p_);
						break;
					}
					case 3: {
						if (prg_rbg_3_2w_bitmap_16bpp_ == 0) {
							prg_rbg_3_2w_bitmap_16bpp_ = compile_color_dot(
								S(a_prg_rbg_3_2w_bitmap),
								prg_rbg_getcolor_16bpp_rbg,
								prg_generate_rbg_end);
						}
						DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_3_2w_bitmap_16bpp_);
						break;
					}
					case 4: {
						if (prg_rbg_3_2w_bitmap_32bpp_ == 0) {
							prg_rbg_3_2w_bitmap_32bpp_ = compile_color_dot(
								S(a_prg_rbg_3_2w_bitmap),
								prg_rbg_getcolor_32bpp_rbg,
								prg_generate_rbg_end);
						}
						DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_3_2w_bitmap_32bpp_);
						break;
					}
					}

				}
				else {
					if (rbg->info.patterndatasize == 1) {
						switch (rbg->info.colornumber) {
						case 0: {
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_3_2w_p1_4bpp_);
							break;
						}
						case 1: { // Final Fight Revenge, Grandia main
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_3_2w_p1_8bpp_);
							break;
						}
						case 2: {
							if (prg_rbg_3_2w_p1_16bpp_p_ == 0) {
								prg_rbg_3_2w_p1_16bpp_p_ = compile_color_dot(
									S(a_prg_rbg_3_2w_p1_4bpp),
									prg_rbg_getcolor_16bpp_palette,
									prg_generate_rbg_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_3_2w_p1_16bpp_p_);
							break;
						}
						case 3: { // Power Drift
							if (prg_rbg_3_2w_p1_16bpp_ == 0) {
								prg_rbg_3_2w_p1_16bpp_ = compile_color_dot(
									S(a_prg_rbg_3_2w_p1_4bpp),
									prg_rbg_getcolor_16bpp_rbg,
									prg_generate_rbg_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_3_2w_p1_16bpp_);
							break;
						}
						case 4: {
							if (prg_rbg_3_2w_p1_32bpp_ == 0) {
								prg_rbg_3_2w_p1_32bpp_ = compile_color_dot(
									S(a_prg_rbg_3_2w_p1_4bpp),
									prg_rbg_getcolor_32bpp_rbg,
									prg_generate_rbg_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_3_2w_p1_32bpp_);
							break;
						}
						}
					}
					else {
						switch (rbg->info.colornumber) {
						case 0: {
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_3_2w_p2_4bpp_);
							break;
						}
						case 1: {
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_3_2w_p2_8bpp_);
							break;
						}
						case 2: {
							if (prg_rbg_3_2w_p2_16bpp_p_ == 0) {
								prg_rbg_3_2w_p2_16bpp_p_ = compile_color_dot(
									S(a_prg_rbg_3_2w_p2_4bpp),
									prg_rbg_getcolor_16bpp_palette,
									prg_generate_rbg_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_3_2w_p2_16bpp_p_);
							break;
						}
						case 3: {
							if (prg_rbg_3_2w_p2_16bpp_ == 0) {
								prg_rbg_3_2w_p2_16bpp_ = compile_color_dot(
									S(a_prg_rbg_3_2w_p2_4bpp),
									prg_rbg_getcolor_16bpp_rbg,
									prg_generate_rbg_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_3_2w_p2_16bpp_);
							break;
						}
						case 4: {
							if (prg_rbg_3_2w_p2_32bpp_ == 0) {
								prg_rbg_3_2w_p2_32bpp_ = compile_color_dot(
									S(a_prg_rbg_3_2w_p2_4bpp),
									prg_rbg_getcolor_32bpp_rbg,
									prg_generate_rbg_end);
							}
							DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_3_2w_p2_32bpp_);
							break;
						}
						}
					}
				}
				// glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_window_);
				// glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(vdp2WindowInfo)*rbg->vres, (void*)rbg->info.pWinInfo);
				// glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo_window_);
			}
		}

	}

	void updateRBG1( RBGDrawInfo * rbg, Vdp2 *varVdp2Regs) {
		if (rbg->info.isbitmap) {
			switch (rbg->info.colornumber) {
			case 0: {
				if (prg_rbg_1_2w_bitmap_4bpp_ == 0) {
					prg_rbg_1_2w_bitmap_4bpp_ = compile_color_dot(
						S(a_prg_rbg_1_2w_bitmap),
						prg_rbg_getcolor_4bpp,
						prg_generate_rbg_end);
				}
				DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_bitmap_4bpp_);
				break;
			}
			case 1: {
				DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_bitmap_8bpp_);
				break;
			}
			case 2: {
				if (prg_rbg_1_2w_bitmap_16bpp_p_ == 0) {
					prg_rbg_1_2w_bitmap_16bpp_p_ = compile_color_dot(
						S(a_prg_rbg_1_2w_bitmap),
						prg_rbg_getcolor_16bpp_palette,
						prg_generate_rbg_end);
				}
				DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_bitmap_16bpp_p_);
				break;
			}
			case 3: {
				if (prg_rbg_1_2w_bitmap_16bpp_ == 0) {
					prg_rbg_1_2w_bitmap_16bpp_ = compile_color_dot(
						S(a_prg_rbg_1_2w_bitmap),
						prg_rbg_getcolor_16bpp_rbg,
						prg_generate_rbg_end);
				}
				DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_bitmap_16bpp_);
				break;
			}
			case 4: {
				if (prg_rbg_1_2w_bitmap_32bpp_ == 0) {
					prg_rbg_1_2w_bitmap_32bpp_ = compile_color_dot(
						S(a_prg_rbg_1_2w_bitmap),
						prg_rbg_getcolor_32bpp_rbg,
						prg_generate_rbg_end);
				}
				DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_bitmap_32bpp_);
				break;
			}
			}
		}
		else {
			if (rbg->info.patterndatasize == 1) {
				switch (rbg->info.colornumber) {
				case 0: {
					DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_p1_4bpp_);
					break;
				}
				case 1: {
					DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_p1_8bpp_);
					break;
				}
				case 2: {
					if (prg_rbg_1_2w_p1_16bpp_p_ == 0) {
						prg_rbg_1_2w_p1_16bpp_p_ = compile_color_dot(
							S(a_prg_rbg_1_2w_p1_4bpp),
							prg_rbg_getcolor_16bpp_palette,
							prg_generate_rbg_end);
					}
					DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_p1_16bpp_p_);
					break;
				}
				case 3: {
					if (prg_rbg_1_2w_p1_16bpp_ == 0) {
						prg_rbg_1_2w_p1_16bpp_ = compile_color_dot(
							S(a_prg_rbg_1_2w_p1_4bpp),
							prg_rbg_getcolor_16bpp_rbg,
							prg_generate_rbg_end);
					}
					DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_p1_16bpp_);
					break;
				}
				case 4: {
					if (prg_rbg_1_2w_p1_32bpp_ == 0) {
						prg_rbg_1_2w_p1_32bpp_ = compile_color_dot(
							S(a_prg_rbg_0_2w_p1_4bpp),
							prg_rbg_getcolor_32bpp_rbg,
							prg_generate_rbg_end);
					}
					DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_p1_32bpp_);
					break;
				}
				}
			}
			else {
				switch (rbg->info.colornumber) {
				case 0: {
					DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_p2_4bpp_);
					break;
				}
				case 1: {
					DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_p2_8bpp_);
					break;
				}
				case 2: {
					if (prg_rbg_1_2w_p2_16bpp_p_ == 0) {
						prg_rbg_1_2w_p2_16bpp_p_ = compile_color_dot(
							S(a_prg_rbg_1_2w_p2_4bpp),
							prg_rbg_getcolor_16bpp_palette,
							prg_generate_rbg_end);
					}
					DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_p2_16bpp_p_);
					break;
				}
				case 3: {
					if (prg_rbg_1_2w_p2_16bpp_ == 0) {
						prg_rbg_1_2w_p2_16bpp_ = compile_color_dot(
							S(a_prg_rbg_1_2w_p2_4bpp),
							prg_rbg_getcolor_16bpp_rbg,
							prg_generate_rbg_end);
					}
					DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_p2_16bpp_);
					break;
				}
				case 4: {
					if (prg_rbg_1_2w_p2_32bpp_ == 0) {
						prg_rbg_1_2w_p2_32bpp_ = compile_color_dot(
							S(a_prg_rbg_1_2w_p2_4bpp),
							prg_rbg_getcolor_32bpp_rbg,
							prg_generate_rbg_end);
					}
					DEBUGWIP("prog %d\n", __LINE__);glUseProgram(prg_rbg_1_2w_p2_32bpp_);
					break;
				}
				}
			}
		}
	}

	void update( RBGDrawInfo * rbg, Vdp2 *varVdp2Regs) {

    if (prg_rbg_0_2w_p1_4bpp_line_ == 0) return;

    GLuint error;
    int local_size_x = 4;
    int local_size_y = 4;

    int work_groups_x = 1 + (tex_width_ - 1) / local_size_x;
    int work_groups_y = 1 + (tex_height_ - 1) / local_size_y;

    error = glGetError();

    if (rbg->info.idScreen == RBG0) updateRBG0(rbg, varVdp2Regs);
		else updateRBG1(rbg, varVdp2Regs);

	ErrorHandle("glUseProgram");

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_vram_);
  //glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 0x80000, (void*)Vdp2Ram);
  if(mapped_vram == nullptr) mapped_vram = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 0x80000, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
  memcpy(mapped_vram, Vdp2Ram, 0x100000);
  glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
  mapped_vram = nullptr;
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_vram_);
  ErrorHandle("glBindBufferBase");

	if (rbg->info.specialcolormode == 3 || rbg->paraA.k_mem_type != 0 || rbg->paraB.k_mem_type != 0 ) {
		if (ssbo_cram_ == 0) {
			glGenBuffers(1, &ssbo_cram_);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_cram_);
			glBufferData(GL_SHADER_STORAGE_BUFFER, 0x1000, NULL, GL_DYNAMIC_DRAW);
		}
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_cram_);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 0x1000, (void*)Vdp2ColorRam);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssbo_cram_);
	}

	if (ssbo_rotwin_ == 0) {
		glGenBuffers(1, &ssbo_rotwin_);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_rotwin_);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 0x800, NULL, GL_DYNAMIC_DRAW);
	}
	if (rbg->info.RotWin != NULL) {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_rotwin_);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 0x800, (void*)rbg->info.RotWin);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssbo_rotwin_);
  }

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_paraA_);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(vdp2rotationparameter_struct), (void*)&rbg->paraA);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, struct_size_, sizeof(vdp2rotationparameter_struct), (void*)&rbg->paraB);
	ErrorHandle("glBufferSubData");
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_paraA_);

	uniform.hres_scale = (float)_Ygl->rheight/(float)_Ygl->height;
	uniform.vres_scale = (float)_Ygl->rwidth/(float)_Ygl->width;
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
	uniform.window_area_mode = rbg->info.RotWinMode;
	uniform.alpha = rbg->info.alpha;
	uniform.priority = rbg->info.priority;

	if (Vdp2Internal.ColorMode < 2) {
		uniform.cram_shift = 1;
	}
	else {
		uniform.cram_shift = 2;
	}

	uniform.startLine = rbg->info.startLine;
	uniform.endLine = rbg->info.endLine;
	uniform.specialprimode = rbg->info.specialprimode;
	uniform.specialfunction = rbg->info.specialfunction;

  glBindBuffer(GL_UNIFORM_BUFFER, scene_uniform);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(RBGUniform), (void*)&uniform);
	ErrorHandle("glBufferSubData");
  glBindBufferBase(GL_UNIFORM_BUFFER, 3, scene_uniform);

	if (rbg->rgb_type == 0x04  ) {
		DEBUGWIP("Draw RBG1\n");
		glBindImageTexture(0, tex_surface_1, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
		ErrorHandle("glBindImageTexture 1");
	}
	else {
		DEBUGWIP("Draw RBG0\n");
		glBindImageTexture(0, tex_surface_, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
		ErrorHandle("glBindImageTexture 0");
	}

  glDispatchCompute(work_groups_x, work_groups_y, 1);
	// glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  ErrorHandle("glDispatchCompute");

  glBindBuffer(GL_UNIFORM_BUFFER, 0);
  }

  //-----------------------------------------------
  GLuint getTexture( int id ) {
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		if (id == 1) {
			return tex_surface_;
		}
		return tex_surface_1;
  }

  //-----------------------------------------------
  void onFinish() {
    if ( ssbo_vram_ != 0 && mapped_vram == nullptr) {
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_vram_);
      mapped_vram = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 0x100000, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0 );
    }
  }

};

RBGGenerator * RBGGenerator::instance_ = nullptr;

extern "C" {
  void RBGGenerator_init(int width, int height) {
    if (_Ygl->rbg_use_compute_shader == 0) return;

    RBGGenerator * instance = RBGGenerator::getInstance();
    instance->init( width, height);
  }
  void RBGGenerator_resize(int width, int height) {
    if (_Ygl->rbg_use_compute_shader == 0) return;
    YGLDEBUG("RBGGenerator_resize\n");
	  RBGGenerator * instance = RBGGenerator::getInstance();
	  instance->resize(width, height);
  }
  void RBGGenerator_update(RBGDrawInfo * rbg, Vdp2 *varVdp2Regs ) {
    if (_Ygl->rbg_use_compute_shader == 0) return;
    RBGGenerator * instance = RBGGenerator::getInstance();
    instance->update(rbg, varVdp2Regs);
  }
  GLuint RBGGenerator_getTexture( int id ) {
    if (_Ygl->rbg_use_compute_shader == 0) return 0;

    RBGGenerator * instance = RBGGenerator::getInstance();
    return instance->getTexture( id );
  }
  void RBGGenerator_onFinish() {

    if (_Ygl->rbg_use_compute_shader == 0) return;
    RBGGenerator * instance = RBGGenerator::getInstance();
    instance->onFinish();
  }
}
