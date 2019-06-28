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

static const char vdp2blit_start_f[] =
SHADER_VERSION_COMPUTE
"#ifdef GL_ES\n"
"precision highp float; \n"
"#endif\n"
"layout(local_size_x = 4, local_size_y = 4) in;\n"
"layout(rgba8, binding = 14) writeonly highp uniform image2D outSurface;\n"
"layout(std430, binding = 15) readonly buffer VDP2DrawInfo { \n"
"  float u_emu_height;\n"
"  float u_vheight; \n"
"  int fbon;  \n"
"  int screen_nb;  \n"
"  int ram_mode; \n"
"  int extended_cc; \n"
"  int use_cc_win; \n"
"  int u_lncl[7];  \n"
"  int mode[7];  \n"
"  int isRGB[6]; \n"
"  int isBlur[7]; \n"
"};\n"
"layout(binding = 0) uniform sampler2D s_texture0;  \n"
"layout(binding = 1) uniform sampler2D s_texture1;  \n"
"layout(binding = 2) uniform sampler2D s_texture2;  \n"
"layout(binding = 3) uniform sampler2D s_texture3;  \n"
"layout(binding = 4) uniform sampler2D s_texture4;  \n"
"layout(binding = 5) uniform sampler2D s_texture5;  \n"
"layout(binding = 7) uniform sampler2D s_back;  \n"
"layout(binding = 8) uniform sampler2D s_lncl;  \n"
"layout(binding = 9) uniform sampler2D s_vdp1FrameBuffer;\n"
"layout(binding = 10) uniform sampler2D s_vdp1FrameBufferAttr;\n"
"layout(binding = 11) uniform sampler2D s_color; \n"
"layout(std430, binding = 12) readonly buffer VDP2reg { int s_vdp2reg[]; }; \n"
"layout(binding = 13) uniform sampler2D s_cc_win;  \n"

"vec4 finalColor;\n"
"ivec2 texel = ivec2(0,0);\n"
"int fbmode = 1;\n"
"int vdp1mode = 1;\n"
"vec4 FBColor = vec4(0.0);\n"
"vec4 vdp2col0 = vec4(0.0);\n"
"vec4 vdp2col1 = vec4(0.0);\n"
"vec4 vdp2col2 = vec4(0.0);\n"
"vec4 vdp2col3 = vec4(0.0);\n"
"vec4 vdp2col4 = vec4(0.0);\n"
"vec4 vdp2col5 = vec4(0.0);\n"
"vec4 FBShadow = vec4(0.0);\n"
"int FBPrio = 0;\n"
"int FBMesh = 0;\n"
"int FBMeshPrio = 0;\n"
"int NoVdp1 = 0;\n"
"int cc_enabled; \n"

"struct Col \n"
"{ \n"
"  vec4 Color; \n"
"  vec4 meshColor;\n"
"  int mesh;\n"
"  int lncl; \n"
"  int mode; \n"
"  int isRGB; \n"
"  int isSprite; \n"
"  int layer; \n"
"}; \n"

"struct FBCol \n"
"{ \n"
"  vec4 color; \n"
"  vec4 meshColor; \n"
"  int mesh;\n"
"  int prio; \n"
"  int meshPrio;\n"
"}; \n"
"float getVdp2RegAsFloat(int id) {\n"
"  return float(s_vdp2reg[id])/255.0;\n"
"};\n"
"FBCol getFB(int x, vec2 texcoord){ \n"
"  FBCol ret;\n"
"  ret.color = vec4(0.0);\n"
"  ret.prio = 0;\n"
"  ret.meshColor = vec4(0.0);\n"
"  ret.mesh = 0;\n"
"  if (fbon != 1) return ret;\n"
"  fbmode = 1;\n"
"  vdp1mode = 1;\n"
"  vec4 fbColor = texelFetch(s_vdp1FrameBuffer, ivec2(texcoord.st * textureSize(s_vdp1FrameBuffer, 0)+ivec2(x, 0)), 0);\n"
"  vec4 fbColorAttr = texelFetch(s_vdp1FrameBufferAttr, ivec2(texcoord.st * textureSize(s_vdp1FrameBufferAttr, 0)+ivec2(x, 0)), 0);\n"
"  vec4 tmpColor = vec4(0.0);\n"
"  vec4 tmpmeshColor = vec4(0.0);\n"
"  int line = int((u_vheight-texel.y) * u_emu_height)*24;\n"
"  vec3 u_coloroffset = vec3(getVdp2RegAsFloat(17 + line), getVdp2RegAsFloat(18 + line), getVdp2RegAsFloat(19 + line));\n"
"  vec3 u_coloroffset_sign = vec3(getVdp2RegAsFloat(20 + line), getVdp2RegAsFloat(21 + line), getVdp2RegAsFloat(22 + line));\n"
"  if (u_coloroffset_sign.r != 0.0) u_coloroffset.r = float(int(u_coloroffset.r*255.0)-256.0)/255.0;\n"
"  if (u_coloroffset_sign.g != 0.0) u_coloroffset.g = float(int(u_coloroffset.g*255.0)-256.0)/255.0;\n"
"  if (u_coloroffset_sign.b != 0.0) u_coloroffset.b = float(int(u_coloroffset.b*255.0)-256.0)/255.0;\n"
"  int u_color_ram_offset = s_vdp2reg[23+line]<<8;\n"
"  int u_cctl = s_vdp2reg[16+line];\n"
"  int additional = int(fbColor.a * 255.0);\n"
"  int additionalAttr = int(fbColorAttr.a * 255.0);\n"
"  int additionalAlpha = int(fbColorAttr.r * 255.0);\n"
"  if( ((additional & 0x80) == 0) && ((additionalAttr & 0xC0) == 0) ){ return ret;} // show? \n"
"  int prinumber = (additional&0x07); \n"
"  int primesh = additionalAttr&0x7;\n"
"  int tmpmeshprio = 0;\n"
"  int depth = s_vdp2reg[prinumber+8+line];\n"
"  int alpha = s_vdp2reg[((additional>>3)&0x07)+line]<<3; \n"
"  int opaque = 0xF8;\n"
"  int tmpmesh = 0;\n"
"  int msb = int(fbColor.b*255.0)&0x1;\n"
"  fbColor.b = float(int(fbColor.b * 255.0)&0xFE)/255.0;\n"
"  vec4 txcol=vec4(0.0,0.0,0.0,1.0);\n"
"  if((additional & 0x80) != 0) {\n"
"    if( (additional & 0x40) != 0 ){  // index color? \n"
"      int colindex = ( int(fbColor.g*255.0)<<8 | int(fbColor.r*255.0)); \n"
"      if( colindex == 0 && prinumber == 0 && ((additionalAttr & 0x80) == 0)) { \n"
"        if ((additionalAttr & 0x40) != 0) {\n"
"          if ((additionalAttr & 0x08) != 0) {\n"
"            ret.meshColor.rgb = fbColorAttr.rgb;\n"
"          } else { \n"
"            colindex = ( int(fbColorAttr.g*255.0)<<8 | int(fbColorAttr.r*255.0)); \n"
"            if( colindex != 0) {\n"
"              colindex = colindex + u_color_ram_offset; \n"
"              txcol = texelFetch( s_color,  ivec2( colindex ,0 )  , 0 );\n"
"              ret.meshColor.rgb = txcol.rgb;\n"
"            } else { \n"
"              ret.meshColor.rgb = vec3(0.0);\n"
"            }\n"
"          }\n"
"          ret.meshColor.a = fbColor.a;\n"
"          ret.mesh = 1;\n"
"        }\n"
"        ret.meshPrio = s_vdp2reg[primesh+8+line];\n"
"        return ret; \n"
"      } // hard/vdp1/hon/p02_11.htm 0 data is ignoerd \n"
"      if( colindex != 0 || prinumber != 0) {\n"
"        colindex = colindex + u_color_ram_offset; \n"
"        txcol = texelFetch( s_color,  ivec2( colindex ,0 )  , 0 );\n"
"        if (txcol.a != 0.0) msb = 1;\n" //linked to VDP2 ColorRam alpha management
"        else msb = 0;\n"
"        tmpColor = txcol;\n"
"      } else { \n"
"        tmpColor = vec4(0.0);\n"
"      }\n"
"    }else{ // direct color \n"
"      tmpColor = fbColor;\n"
"    } \n"
"    tmpColor.rgb = clamp(tmpColor.rgb + u_coloroffset, vec3(0.0), vec3(1.0));  \n"
"  } else { \n"
"    tmpColor = fbColor;\n"
"  } \n"
"  if ((additionalAttr & 0x80) != 0) {\n"
"    if (tmpColor.rgb == vec3(0.0)) {\n"
"      alpha = 0x78;\n"
"      vdp1mode = 5;\n"
"      fbmode = 0;\n"
"    } else { \n"
"      if (s_vdp2reg[(additionalAttr & 0x7)+8+line]-1 == depth) {\n"
"        tmpColor.rgb = tmpColor.rgb * 0.5;\n"
"      }\n"
"    }\n"
"  } \n"
"  if ((additionalAttr & 0x40) != 0) {\n"
"    if ((additionalAttr & 0x08) != 0) {\n"
"      tmpmeshColor.rgb = fbColorAttr.rgb;\n"
"    } else { \n"
"      int colindex = ( int(fbColorAttr.g*255.0)<<8 | int(fbColorAttr.r*255.0)); \n"
"      if( colindex != 0) {\n"
"        colindex = colindex + u_color_ram_offset; \n"
"        txcol = texelFetch( s_color,  ivec2( colindex ,0 )  , 0 );\n"
"        tmpmeshColor.rgb = txcol.rgb;\n"
"      } else { \n"
"        tmpmeshColor.rgb = vec3(0.0);\n"
"      }\n"
"    }\n"
"    tmpmeshColor.a = fbColor.a;\n"
"    tmpmeshprio = s_vdp2reg[primesh+8+line];"
"    tmpmesh = 1;\n"
"  }\n"
"  if (fbmode != 0) {\n";

const GLchar Yglprg_vdp2_drawfb_cram_no_color_col_f[]    = " alpha = opaque; \n";

const GLchar Yglprg_vdp2_drawfb_cram_less_color_col_f[]  = " if( depth > u_cctl ){ alpha = opaque; fbmode = 0;} \n ";
const GLchar Yglprg_vdp2_drawfb_cram_equal_color_col_f[] = " if( depth != u_cctl ){ alpha = opaque; fbmode = 0;} \n ";
const GLchar Yglprg_vdp2_drawfb_cram_more_color_col_f[]  = " if( depth < u_cctl ){ alpha = opaque; fbmode = 0;} \n ";
const GLchar Yglprg_vdp2_drawfb_cram_msb_color_col_f[]   = " if( msb == 0 ){ alpha = opaque; fbmode = 0;} \n ";


const GLchar Yglprg_vdp2_drawfb_cram_epiloge_none_f[] =
"\n";
const GLchar Yglprg_vdp2_drawfb_cram_epiloge_as_is_f[] =
" if (fbmode == 1) vdp1mode = 2; \n";
const GLchar Yglprg_vdp2_drawfb_cram_epiloge_src_alpha_f[] =
" if (fbmode == 1) vdp1mode = 3; \n";
const GLchar Yglprg_vdp2_drawfb_cram_epiloge_dst_alpha_f[] =
" if (fbmode == 1) vdp1mode = 4; \n";

static const char vdp2blit_end_f[] =" }\n"
" if(additionalAlpha != 0) {\n"
"   alpha = 0x78;\n"
"   vdp1mode = 3;\n"
" }\n"
" tmpColor.a = float(alpha|vdp1mode)/255.0; \n"
" ret.color = tmpColor;\n"
" ret.meshColor = tmpmeshColor;\n"
" ret.mesh = tmpmesh;\n"
" ret.meshPrio = tmpmeshprio;\n"
" ret.prio = depth;\n"
" return ret;\n"
"}\n"

"vec4 getPixel(sampler2D tex, vec2 st) {\n"
" ivec2 addr = ivec2(textureSize(tex, 0) * st);\n"
" vec4 result = texelFetch( tex, addr,0 );\n"
//" result.rgb = Filter( tex, st ).rgb;\n"
" return result;\n"
"}\n"

"Col getPriorityColor(int prio, int nbPrio)   \n"
"{  \n"
"  Col ret, empty; \n"
"  vec4 tmpColor;\n"
"  int remPrio = nbPrio;\n"
"  empty.Color = vec4(0.0);\n"
"  empty.mode = 0;\n"
"  empty.lncl = 0;\n"
"  empty.isSprite = 0;\n"
"  empty.layer = -1;\n"
"  empty.meshColor = vec4(0.0);\n"
"  empty.mesh = 0;\n"
"  ret = empty;\n"
"  int priority; \n"
"  int alpha; \n"
"  if ((fbon == 1) && (prio == FBPrio)) {\n"
"    ret.mode = int(FBColor.a*255.0)&0x7; \n"
"    ret.lncl = u_lncl[6];\n"
"    ret.Color = FBColor; \n"
"    remPrio = remPrio - 1;\n"
"    alpha = int(ret.Color.a*255.0)&0xF8; \n"
"    ret.Color.a = float(alpha>>3)/31.0; \n"
"    ret.isRGB = 0;\n" //Shall not be the case always... Need to get RGB format per pixel
"    ret.isSprite = 1;\n"
"    ret.layer = 6;\n"
"    if (remPrio == 0) return ret;\n"
"  }\n"
"  if (screen_nb == 0) return empty;\n"
"  ret.isSprite = 0;\n"
"  tmpColor = vdp2col0; \n"
"  priority = int(tmpColor.a*255.0)&0x7; \n"
"  if (priority == prio) {\n"
"    remPrio = remPrio - 1;\n"
"    ret.lncl=u_lncl[0];\n"
"    ret.Color = tmpColor; \n"
"    ret.isRGB = isRGB[0];\n"
"    alpha = int(ret.Color.a*255.0)&0xF8; \n"
"    ret.mode = mode[0]; \n"
"    ret.Color.a = float(alpha>>3)/31.0; \n"
"    ret.layer = 0;\n"
"    if (remPrio == 0) return ret;\n"
"  }\n"
"  if (screen_nb == 1) return empty;\n"
"  tmpColor = vdp2col1; \n"
"  priority = int(tmpColor.a*255.0)&0x7; \n"
"  if (priority == prio) {\n"
"    remPrio = remPrio - 1;\n"
"    ret.lncl=u_lncl[1];\n"
"    ret.isRGB = isRGB[1];\n"
"    ret.Color = tmpColor; \n"
"    alpha = int(ret.Color.a*255.0)&0xF8; \n"
"    ret.mode = mode[1]; \n"
"    ret.Color.a = float(alpha>>3)/31.0; \n"
"    ret.layer = 1;\n"
"    if (remPrio == 0) return ret;\n"
"  }\n"
"  if (screen_nb == 2) return empty;\n"
"  tmpColor = vdp2col2; \n"
"  priority = int(tmpColor.a*255.0)&0x7; \n"
"  if (priority == prio) {\n"
"    remPrio = remPrio - 1;\n"
"    ret.lncl=u_lncl[2];\n"
"    ret.isRGB = isRGB[2];\n"
"    ret.Color = tmpColor; \n"
"    alpha = int(ret.Color.a*255.0)&0xF8; \n"
"    ret.mode = mode[2]; \n"
"    ret.Color.a = float(alpha>>3)/31.0; \n"
"    ret.layer = 2;\n"
"    if (remPrio == 0) return ret;\n"
"  }\n"
"  if (screen_nb == 3) return empty;\n"
"  tmpColor = vdp2col3; \n"
"  priority = int(tmpColor.a*255.0)&0x7; \n"
"  if (priority == prio) {\n"
"    remPrio = remPrio - 1;\n"
"    ret.lncl=u_lncl[3];\n"
"    ret.isRGB = isRGB[3];\n"
"    ret.Color = tmpColor; \n"
"    alpha = int(ret.Color.a*255.0)&0xF8; \n"
"    ret.mode = mode[3]; \n"
"    ret.Color.a = float(alpha>>3)/31.0; \n"
"    ret.layer = 3;\n"
"    if (remPrio == 0) return ret;\n"
"  }\n"
"  if (screen_nb == 4) return empty;\n"
"  tmpColor = vdp2col4; \n"
"  priority = int(tmpColor.a*255.0)&0x7; \n"
"  if (priority == prio) {\n"
"    remPrio = remPrio - 1;\n"
"    ret.Color = tmpColor; \n"
"    ret.lncl=u_lncl[4];\n"
"    ret.isRGB = isRGB[4];\n"
"    alpha = int(ret.Color.a*255.0)&0xF8; \n"
"    ret.mode = mode[4]; \n"
"    ret.Color.a = float(alpha>>3)/31.0; \n"
"    ret.layer = 4;\n"
"    if (remPrio == 0) return ret;\n"
"  }\n"
"  if (screen_nb == 5) return empty;\n"
"  tmpColor = vdp2col5; \n"
"  priority = int(tmpColor.a*255.0)&0x7; \n"
"  if (priority == prio) {\n"
"    remPrio = remPrio - 1;\n"
"    ret.Color = tmpColor; \n"
"    ret.lncl=u_lncl[5];\n"
"    ret.isRGB = isRGB[5];\n"
"    alpha = int(ret.Color.a*255.0)&0xF8; \n"
"    ret.mode = mode[5]; \n"
"    ret.Color.a = float(alpha>>3)/31.0; \n"
"    ret.layer = 5;\n"
"    if (remPrio == 0) return ret;\n"
"  }\n"
"  return empty;\n"
"}  \n"

"Col getBlur(ivec2 addr, Col pix, vec2 texcoord) \n"
"{  \n"
"  Col ret = pix;\n"
"  vec4 txcoll;\n"
"  vec4 txcolll;\n"
"  vec4 txcol = pix.Color;\n"
"  if (pix.layer == 6) { \n"
"    txcoll = getFB(-1, texcoord).color;\n"
"    txcolll = getFB(-2, texcoord).color;\n"
"  }\n"
"  if (pix.layer == 0) { \n"
"    txcoll = texelFetch( s_texture0, ivec2(addr.x-1, addr.y),0 );      \n"
"    txcolll = texelFetch( s_texture0, ivec2(addr.x-2, addr.y),0 );      \n"
"  }\n"
"  if (pix.layer == 1) { \n"
"    txcoll = texelFetch( s_texture1, ivec2(addr.x-1, addr.y),0 );      \n"
"    txcolll = texelFetch( s_texture1, ivec2(addr.x-2, addr.y),0 );      \n"
"  }\n"
"  if (pix.layer == 2) { \n"
"    txcoll = texelFetch( s_texture2, ivec2(addr.x-1, addr.y),0 );      \n"
"    txcolll = texelFetch( s_texture2, ivec2(addr.x-2, addr.y),0 );      \n"
"  }\n"
"  if (pix.layer == 3) { \n"
"    txcoll = texelFetch( s_texture3, ivec2(addr.x-1, addr.y),0 );      \n"
"    txcolll = texelFetch( s_texture3, ivec2(addr.x-2, addr.y),0 );      \n"
"  }\n"
"  if (pix.layer == 4) { \n"
"    txcoll = texelFetch( s_texture4, ivec2(addr.x-1, addr.y),0 );      \n"
"    txcolll = texelFetch( s_texture4, ivec2(addr.x-2, addr.y),0 );      \n"
"  }\n"
"  if (pix.layer == 5) { \n"
"    txcoll = texelFetch( s_texture5, ivec2(addr.x-1, addr.y),0 );      \n"
"    txcolll = texelFetch( s_texture5, ivec2(addr.x-2, addr.y),0 );      \n"
"  }\n"
"  ret.Color.rgb = txcol.rgb/2.0 + txcoll.rgb/4.0 + txcolll.rgb/4.0; \n"
"  return ret; \n"
"}  \n"

"void main()   \n"
"{  \n"
"  int x, y;\n"
"  texel = ivec2(gl_GlobalInvocationID.xy);\n"
"  ivec2 size = imageSize(outSurface);\n"
"  if (texel.x >= size.x || texel.y >= size.y ) return;\n"
"  vec2 v_texcoord = vec2(float(texel.x)/float(size.x),float(texel.y)/float(size.y));\n"
"  vec4 topImage = vec4(0.0); \n"
"  vec4 secondImage = vec4(0.0); \n"
"  vec4 colortop = vec4(0.0);  \n"
"  vec4 colorsecond = vec4(0.0); \n"
"  vec4 colorthird = vec4(0.0); \n"
"  vec4 colorfourth = vec4(0.0); \n"
"  vec4 colorback = vec4(0.0); \n"
"  int foundColor1 = 0; \n"
"  int foundColor2 = 0; \n"
"  int foundColor3 = 0; \n"
"  int modetop = 1; \n"
"  int modesecond = 1; \n"
"  int modethird = 1; \n"
"  int isRGBtop = 0;\n"
"  int isRGBsecond = 0;\n"
"  int isRGBthird = 0;\n"
"  int isRGBfourth = 0;\n"
"  int use_lncl = 0; \n"
"  int shadow = 0;\n"
"  int mesh = 0;\n"
"  vec3 meshCol = vec3(0.0);\n"
"  float alphatop = 1.0; \n"
"  float alphasecond = 1.0; \n"
"  float alphathird = 1.0; \n"
"  float alphafourth = 1.0; \n"
"  ivec2 addr = ivec2(textureSize(s_back, 0) * v_texcoord.st); \n"
"  colorback = texelFetch( s_back, addr,0 ); \n"
"  addr = ivec2(textureSize(s_texture0, 0) * v_texcoord.st); \n"
"  colortop = colorback; \n"
"  isRGBtop = 1; \n"
"  alphatop = float((int(colorback.a * 255.0)&0xF8)>>3)/31.0;\n"
"  FBCol tmp = getFB(0,v_texcoord); \n"
"  FBColor = tmp.color;\n"
"  FBPrio = tmp.prio;\n"
"  FBShadow = tmp.meshColor;\n"
"  FBMeshPrio = tmp.meshPrio;\n"
"  FBMesh = tmp.mesh;\n"
"  if ((int(FBColor.a * 255.0)&0x7) == 5) {\n"
"    FBPrio = 0;\n"
"    shadow = 1;\n"
"  }\n"
"  if (screen_nb > 0) vdp2col0 = getPixel( s_texture0, v_texcoord.st ); \n"
"  if (screen_nb > 1) vdp2col1 = getPixel( s_texture1, v_texcoord.st ); \n"
"  if (screen_nb > 2) vdp2col2 = getPixel( s_texture2, v_texcoord.st ); \n"
"  if (screen_nb > 3) vdp2col3 = getPixel( s_texture3, v_texcoord.st ); \n"
"  if (screen_nb > 4) vdp2col4 = getPixel( s_texture4, v_texcoord.st ); \n"
"  if (screen_nb > 5) vdp2col5 = getPixel( s_texture5, v_texcoord.st ); \n"
"  for (int i = 7; i>0; i--) { \n"
"    if ((foundColor1 == 0) || (foundColor2 == 0) || (foundColor3 == 0)) { \n"
"      int hasColor = 1;\n"
"      while (hasColor != 0) {\n"
"        if ((foundColor1 == 0) && (fbon == 1) && (i == FBMeshPrio)) {\n"
"          if (FBMesh == 1) {\n"
"            mesh = 1;\n"
"            meshCol = FBShadow.rgb;\n"
"          }\n"
"        }\n"
"        Col prio = getPriorityColor(i, hasColor);\n"
"        hasColor = hasColor+1;\n"
"        if (prio.mode != 0) { \n"
"          if (foundColor1 == 0) { \n"
"            prio.mode = (prio.mode & 0x7); \n"
"            if (prio.isSprite == 0) {\n"
"              if ((int(prio.Color.b*255.0)&0x1) == 0) {\n"
                 //Special color calulation mode => CC is off on this pixel
"                prio.mode = 1;\n"
"                prio.Color.a = 1.0;\n"
"              }\n"
"            }\n"
"            if (prio.lncl == 0) { \n"
"              colorsecond = colortop;\n"
"              alphasecond = alphatop;\n"
"              modesecond = modetop;\n"
"              isRGBsecond = isRGBtop;\n"
"            } else { \n"
"              ivec2 linepos; \n "
"              linepos.y = 0; \n "
"              linepos.x = int( (u_vheight-texel.y) * u_emu_height);\n"
"              colorsecond = texelFetch( s_lncl, linepos ,0 );\n"
"              modesecond = mode[6];\n"
"              alphasecond = float((int(colorsecond.a * 255.0)&0xF8)>>3)/31.0;\n"
"              isRGBsecond = 1;\n"
"              use_lncl = 1;\n"
"            }\n"
"            colortop = prio.Color; \n"
"            modetop = prio.mode&0x7; \n"
"            isRGBtop = prio.isRGB; \n"
"            alphatop = prio.Color.a; \n"
"            foundColor1 = 1; \n"
"            if (isBlur[prio.layer] != 0) { \n"
"              Col blur = getBlur(addr, prio, v_texcoord);"
"              modesecond = blur.mode&0x7; \n"
"              colorsecond = blur.Color; \n"
"              alphasecond = blur.Color.a; \n"
"              isRGBsecond = blur.isRGB; \n"
"              foundColor2 = 1; \n"
"            }\n"
"          } else if (foundColor2 == 0) { \n"
"            if ((use_lncl == 0)||(prio.lncl == 1)) {\n"
"              if ((prio.lncl == 0)||((use_lncl == 1)&&(prio.lncl == 1))) { \n"
"                colorthird = colorsecond;\n"
"                alphathird = alphasecond;\n"
"                modethird = modesecond;\n"
"                isRGBthird = isRGBsecond;\n"
"              } else { \n"
"                ivec2 linepos; \n "
"                linepos.y = 0; \n "
"                linepos.x = int( (u_vheight-texel.y) * u_emu_height);\n"
"                colorthird = texelFetch( s_lncl, linepos ,0 );\n"
"                modethird = mode[6];\n"
"                alphathird = float((int(colorthird.a * 255.0)&0xF8)>>3)/31.0;\n"
"                isRGBthird = 1;\n"
"                use_lncl = 1;\n"
"              }\n"
"              modesecond = prio.mode&0x7; \n"
"              colorsecond = prio.Color; \n"
"              alphasecond = prio.Color.a; \n"
"              isRGBsecond = prio.isRGB; \n"
"              foundColor2 = 1; \n"
"            } else { \n"
"              foundColor2 = 1; \n"
"              foundColor3 = 1; \n"
"              colorfourth = colorthird;\n"
"              alphafourth = alphathird;\n"
"              isRGBfourth = isRGBthird; \n"
"              modethird= prio.mode&0x7; \n"
"              colorthird = prio.Color; \n"
"              alphathird = prio.Color.a; \n"
"              isRGBthird = prio.isRGB; \n"
"            } \n"
"            if (isBlur[prio.layer] != 0) { \n"
"              Col blur = getBlur(addr, prio, v_texcoord);"
"              modesecond = blur.mode&0x7; \n"
"              colorsecond = blur.Color; \n"
"              alphasecond = blur.Color.a; \n"
"              isRGBsecond = blur.isRGB; \n"
"              foundColor2 = 1; \n"
"            }\n"
"          } else if (foundColor3 == 0) { \n"
"            if (prio.lncl == 0) { \n"
"              colorfourth = colorthird;\n"
"              alphafourth = alphathird;\n"
"              isRGBfourth = isRGBthird; \n"
"            } else { \n"
"              ivec2 linepos; \n "
"              linepos.y = 0; \n "
"              linepos.x = int( (u_vheight-texel.y) * u_emu_height);\n"
"              colorfourth = texelFetch( s_lncl, linepos ,0 );\n"
"              alphafourth = float((int(colorfourth.a * 255.0)&0xF8)>>3)/31.0;\n"
"              isRGBfourth = 1;\n"
"              use_lncl = 1;\n"
"            }\n"
"            modethird= prio.mode&0x7; \n"
"            colorthird = prio.Color; \n"
"            alphathird = prio.Color.a; \n"
"            isRGBthird = prio.isRGB; \n"
"            foundColor3 = 1; \n"
"          } \n"
"        } \n"
"        if (((prio.mode&0x7) == 0) || ((foundColor1 == 1)&&(foundColor2 == 1)&&(foundColor3 == 1))) { \n"
"          hasColor = 0; \n"
"        } \n"
"      }\n"
"    } \n"
"  } \n"
"  if ((FBMesh == 1) && (foundColor1 == 0)) {\n"
"    mesh = 1;\n"
"    meshCol = FBShadow.rgb;\n"
"  }\n"
//Take care  of the extended coloration mode
"  cc_enabled = use_cc_win;\n"
"  if (use_cc_win != 0) {\n"
"    ivec2 cc_addr = ivec2(textureSize(s_cc_win, 0) * v_texcoord.st); \n"
"    vec4 tmp = texelFetch( s_cc_win, cc_addr ,0 ); \n"
"    if (tmp.a == 0.0) cc_enabled = 0;\n"
"  }\n"
"  if (cc_enabled == 0) {\n"
"  if (extended_cc != 0) { \n"
"    if (ram_mode == 0) { \n"
"      if (use_lncl == 0) { \n"
"        if (modesecond == 1) \n"
"          secondImage.rgb = vec3(colorsecond.rgb); \n"
"        else \n"
"          secondImage.rgb = vec3(0.5 * colorsecond.rgb + 0.5 * colorthird.rgb); \n"
"      } else {\n"
"        if (modesecond == 1) \n"
"          secondImage.rgb = vec3(colorsecond.rgb); \n"
"        else {\n"
"          if (modethird == 1) \n"
"            secondImage.rgb = vec3(0.5 * colorsecond.rgb + 0.5 * colorthird.rgb); \n"
"          else \n"
"            secondImage.rgb = vec3(0.66666 * colorsecond.rgb + 0.33334 * colorthird.rgb); \n"
"        }\n"
"      }\n"
"    } else {\n"
"      if (use_lncl == 0) { \n"
"       if (isRGBthird == 0) { \n"
"          secondImage.rgb = vec3(colorsecond.rgb); \n"
"       } else { \n"
"         if (modesecond == 1) { \n"
"           secondImage.rgb = vec3(colorsecond.rgb); \n"
"         } else {\n"
"           secondImage.rgb = vec3(0.5 * colorsecond.rgb + 0.5 * colorthird.rgb); \n"
"         } \n"
"       }\n"
"      } else {\n"
"       if (isRGBthird == 0) { \n"
"           secondImage.rgb = vec3(colorsecond.rgb); \n"
"       } else { \n"
"         if (isRGBfourth == 0) {\n"
"           if (modesecond == 1) secondImage.rgb = vec3(colorsecond.rgb);\n"
"           else secondImage.rgb = vec3(0.5 * colorsecond.rgb + 0.5 * colorthird.rgb);\n"
"         } else { \n"
"           if (modesecond == 1) secondImage.rgb = vec3(colorsecond.rgb);\n"
"           else { \n"
"             if (modethird == 1) secondImage.rgb = vec3(0.5 * colorsecond.rgb + 0.5 * colorthird.rgb);\n"
"             else secondImage.rgb = vec3(0.5 * colorsecond.rgb + 0.25 * colorthird.rgb + 0.25 * colorfourth.rgb);\n"
"           }\n"
"         }\n"
"       }\n"
"      }\n"
"    } \n"
"  } else { \n"
"    secondImage.rgb = vec3(colorsecond.rgb); \n"
"  } \n"

"  if (modetop == 1) topImage = vec4(colortop.rgb, 1.0); \n"
"  if (modetop == 2) topImage = vec4(colortop.rgb, 0.0); \n"
"  if (modetop == 3) topImage = vec4(colortop.rgb*alphatop, alphatop); \n"
"  if (modetop == 4) topImage = vec4(colortop.rgb*alphasecond, alphasecond); \n"

"  finalColor = vec4( topImage.rgb + (1.0 - topImage.a) * secondImage.rgb, 1.0); \n"
"  } else {\n"
"  finalColor = vec4(colortop.rgb, 1.0);\n"
"  }\n"
"  if (shadow == 1) finalColor.rgb = finalColor.rgb * 0.5;\n"
"  if (mesh == 1) finalColor.rgb = finalColor.rgb * 0.5 + meshCol.rgb * 0.5;\n"
"  imageStore(outSurface,texel,finalColor);\n"
"}\n";


const GLchar * a_prg_vdp2_composer[20][4] = {
	{
		vdp2blit_start_f,
		Yglprg_vdp2_drawfb_cram_no_color_col_f,
		Yglprg_vdp2_drawfb_cram_epiloge_none_f,
		vdp2blit_end_f
  }, {
		vdp2blit_start_f,
		Yglprg_vdp2_drawfb_cram_no_color_col_f,
		Yglprg_vdp2_drawfb_cram_epiloge_as_is_f,
		vdp2blit_end_f
	}, {
		vdp2blit_start_f,
		Yglprg_vdp2_drawfb_cram_no_color_col_f,
		Yglprg_vdp2_drawfb_cram_epiloge_src_alpha_f,
		vdp2blit_end_f
	}, {
		vdp2blit_start_f,
		Yglprg_vdp2_drawfb_cram_no_color_col_f,
		Yglprg_vdp2_drawfb_cram_epiloge_dst_alpha_f,
		vdp2blit_end_f
	}, {
		vdp2blit_start_f,
		Yglprg_vdp2_drawfb_cram_less_color_col_f,
		Yglprg_vdp2_drawfb_cram_epiloge_none_f,
		vdp2blit_end_f
  }, {
		vdp2blit_start_f,
		Yglprg_vdp2_drawfb_cram_less_color_col_f,
		Yglprg_vdp2_drawfb_cram_epiloge_as_is_f,
		vdp2blit_end_f
	}, {
		vdp2blit_start_f,
		Yglprg_vdp2_drawfb_cram_less_color_col_f,
		Yglprg_vdp2_drawfb_cram_epiloge_src_alpha_f,
		vdp2blit_end_f
	}, {
		vdp2blit_start_f,
		Yglprg_vdp2_drawfb_cram_less_color_col_f,
		Yglprg_vdp2_drawfb_cram_epiloge_dst_alpha_f,
		vdp2blit_end_f
	}, {
		vdp2blit_start_f,
		Yglprg_vdp2_drawfb_cram_equal_color_col_f,
		Yglprg_vdp2_drawfb_cram_epiloge_none_f,
		vdp2blit_end_f
  }, {
		vdp2blit_start_f,
		Yglprg_vdp2_drawfb_cram_equal_color_col_f,
		Yglprg_vdp2_drawfb_cram_epiloge_as_is_f,
		vdp2blit_end_f
	}, {
		vdp2blit_start_f,
		Yglprg_vdp2_drawfb_cram_equal_color_col_f,
		Yglprg_vdp2_drawfb_cram_epiloge_src_alpha_f,
		vdp2blit_end_f
	}, {
		vdp2blit_start_f,
		Yglprg_vdp2_drawfb_cram_equal_color_col_f,
		Yglprg_vdp2_drawfb_cram_epiloge_dst_alpha_f,
		vdp2blit_end_f
	}, {
		vdp2blit_start_f,
		Yglprg_vdp2_drawfb_cram_more_color_col_f,
		Yglprg_vdp2_drawfb_cram_epiloge_none_f,
		vdp2blit_end_f
  }, {
		vdp2blit_start_f,
		Yglprg_vdp2_drawfb_cram_more_color_col_f,
		Yglprg_vdp2_drawfb_cram_epiloge_as_is_f,
		vdp2blit_end_f
	}, {
		vdp2blit_start_f,
		Yglprg_vdp2_drawfb_cram_more_color_col_f,
		Yglprg_vdp2_drawfb_cram_epiloge_src_alpha_f,
		vdp2blit_end_f
	}, {
		vdp2blit_start_f,
		Yglprg_vdp2_drawfb_cram_more_color_col_f,
		Yglprg_vdp2_drawfb_cram_epiloge_dst_alpha_f,
		vdp2blit_end_f
	}, {
		vdp2blit_start_f,
		Yglprg_vdp2_drawfb_cram_msb_color_col_f,
		Yglprg_vdp2_drawfb_cram_epiloge_none_f,
		vdp2blit_end_f
  }, {
		vdp2blit_start_f,
		Yglprg_vdp2_drawfb_cram_msb_color_col_f,
		Yglprg_vdp2_drawfb_cram_epiloge_as_is_f,
		vdp2blit_end_f
	}, {
		vdp2blit_start_f,
		Yglprg_vdp2_drawfb_cram_msb_color_col_f,
		Yglprg_vdp2_drawfb_cram_epiloge_src_alpha_f,
		vdp2blit_end_f
	}, {
		vdp2blit_start_f,
		Yglprg_vdp2_drawfb_cram_msb_color_col_f,
		Yglprg_vdp2_drawfb_cram_epiloge_dst_alpha_f,
		vdp2blit_end_f
	}
};

struct VDP2DrawInfo {
	float u_emu_height;
	float u_vheight;
	int fbon;
	int screen_nb;
	int ram_mode;
	int extended_cc;
	int use_cc_win;
	int u_lncl[7];
	int mode[7];
	int isRGB[6];
	int isBlur[7];
};

class VDP2Generator{
  GLuint prg_vdp2_composer[20] = {0};

  int tex_width_ = 0;
  int tex_height_ = 0;
  static VDP2Generator * instance_;
  GLuint scene_uniform = 0;
	GLuint ssbo_vdp2reg_ = 0;
  VDP2DrawInfo uniform;
	int struct_size_;

protected:
  VDP2Generator() {
    tex_width_ = 0;
    tex_height_ = 0;
		scene_uniform = 0;
		ssbo_vdp2reg_ = 0;
		struct_size_ = sizeof(VDP2DrawInfo);
		int am = sizeof(VDP2DrawInfo) % 4;
		if (am != 0) {
			struct_size_ += 4 - am;
		}
  }

public:
  static VDP2Generator * getInstance() {
    if (instance_ == nullptr) {
      instance_ = new VDP2Generator();
    }
    return instance_;
  }

  void resize(int width, int height) {
	if (tex_width_ == width && tex_height_ == height) return;

	DEBUGWIP("resize %d, %d\n",width,height);

	glGetError();

	tex_width_ = width;
	tex_height_ = height;
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

		if (scene_uniform != 0) return; // always inisialized!

		glGenBuffers(1, &scene_uniform);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, scene_uniform);
		glBufferData(GL_SHADER_STORAGE_BUFFER, struct_size_, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		glGenBuffers(1, &ssbo_vdp2reg_);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_vdp2reg_);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 512*sizeof(int)*NB_VDP2_REG,(void*)YglGetVDP2RegPointer(), GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	}

  bool ErrorHandle(const char* name)
  {
  #ifdef DEBUG
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
	#else
	  return true;
	#endif
  }

  template<typename T>
  T Add(T a, T b) {
	  return a + b;
  }


//#define COMPILE_COLOR_DOT( BASE, COLOR , DOT ) ({ GLuint PRG; BASE[sizeof(BASE)/sizeof(char*) - 2] = COLOR; BASE[sizeof(BASE)/sizeof(char*) - 1] = DOT; PRG=createProgram(sizeof(BASE)/sizeof(char*), (const GLchar**)BASE);})

#define COMPILE_COLOR_DOT( BASE, COLOR , DOT )
#define S(A) A, sizeof(A)/sizeof(char*)

  int getProgramId(int mode, Vdp2 *varVdp2Regs) {
		int pgid = 0;

		const int SPCCN = ((varVdp2Regs->CCCTL >> 6) & 0x01); // hard/vdp2/hon/p12_14.htm#NxCCEN_
		const int CCRTMD = ((varVdp2Regs->CCCTL >> 9) & 0x01); // hard/vdp2/hon/p12_14.htm#CCRTMD_
		const int CCMD = ((varVdp2Regs->CCCTL >> 8) & 0x01);  // hard/vdp2/hon/p12_14.htm#CCMD_
		const int SPLCEN = (varVdp2Regs->LNCLEN & 0x20); // hard/vdp2/hon/p11_30.htm#NxLCEN_

		if ( SPCCN ) {
			const int SPCCCS = (varVdp2Regs->SPCTL >> 12) & 0x3;
			switch (SPCCCS)
			{
				case 0:
					pgid = 4;
					break;
				case 1:
					pgid = 8;
					break;
				case 2:
					pgid = 12;
					break;
				case 3:
					pgid = 16;
					break;
		 	}
	 	}
	 	pgid += mode-NONE;

	 	return pgid;
	}

	void update( int outputTex, YglPerLineInfo *bg, int* prioscreens, int* modescreens, int* isRGB, int * isBlur, int* lncl, GLuint* vdp1fb, Vdp2 *varVdp2Regs) {

    GLuint error;
    int local_size_x = 4;
    int local_size_y = 4;
	  int nbScreen = 6;

    int work_groups_x = 1 + (tex_width_ - 1) / local_size_x;
    int work_groups_y = 1 + (tex_height_ - 1) / local_size_y;

		int gltext[6] = {GL_TEXTURE0, GL_TEXTURE1, GL_TEXTURE2, GL_TEXTURE3, GL_TEXTURE4, GL_TEXTURE5};

    error = glGetError();

	  DEBUGWIP("prog %d\n", __LINE__);
		int progId = getProgramId(getSpriteRenderMode(varVdp2Regs), varVdp2Regs);
		if (prg_vdp2_composer[progId] == 0)
			prg_vdp2_composer[progId] = createProgram(sizeof(a_prg_vdp2_composer[progId]) / sizeof(char*), (const GLchar**)a_prg_vdp2_composer[progId]);
		glUseProgram(prg_vdp2_composer[progId]);

		ErrorHandle("glUseProgram");

		memcpy(uniform.u_lncl,lncl, 7*sizeof(int));
		memcpy(uniform.mode, modescreens, 7*sizeof(int));
		memcpy(uniform.isRGB, isRGB, 6*sizeof(int));
		memcpy(uniform.isBlur, isBlur, 7*sizeof(int));
		uniform.u_emu_height = (float)_Ygl->rheight / (float)_Ygl->height;
		uniform.u_vheight = (float)_Ygl->height;
		uniform.fbon = (_Ygl->vdp1On[_Ygl->readframe] != 0);
		uniform.ram_mode = Vdp2Internal.ColorMode;
		uniform.extended_cc = ((varVdp2Regs->CCCTL & 0x400) != 0);
		uniform.use_cc_win = (_Ygl->use_cc_win != 0);

		glActiveTexture(GL_TEXTURE7);
	  glBindTexture(GL_TEXTURE_2D, _Ygl->back_fbotex[0]);

	  glActiveTexture(GL_TEXTURE8);
	  glBindTexture(GL_TEXTURE_2D, _Ygl->lincolor_tex);

		glActiveTexture(GL_TEXTURE9);
		glBindTexture(GL_TEXTURE_2D, vdp1fb[0]);
		glActiveTexture(GL_TEXTURE10);
		glBindTexture(GL_TEXTURE_2D, vdp1fb[1]);

		glActiveTexture(GL_TEXTURE11);
	  glBindTexture(GL_TEXTURE_2D, _Ygl->cram_tex);

#if 0
		glActiveTexture(GL_TEXTURE12);
	  glBindTexture(GL_TEXTURE_2D, _Ygl->vdp2reg_tex);
#else
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_vdp2reg_);
		u8* vdp2buf = YglGetVDP2RegPointer();
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 512*sizeof(int)*NB_VDP2_REG,(void*)vdp2buf);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, ssbo_vdp2reg_);
#endif
		if (_Ygl->use_cc_win != 0) {
	    glActiveTexture(GL_TEXTURE13);
	    glBindTexture(GL_TEXTURE_2D, _Ygl->window_cc_fbotex);
	  }

		int id = 0;
	  for (int i=0; i<nbScreen; i++) {
	    if (prioscreens[i] != 0) {
	      glActiveTexture(gltext[i]);
	      glBindTexture(GL_TEXTURE_2D, prioscreens[i]);
	      id++;
	    }
	  }
	  uniform.screen_nb = id;

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, scene_uniform);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, struct_size_, (void*)&uniform);
		ErrorHandle("glBufferSubData");
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 15, scene_uniform);

		DEBUGWIP("Draw RBG0\n");
		glBindImageTexture(14, outputTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
		ErrorHandle("glBindImageTexture 0");

	  glDispatchCompute(work_groups_x, work_groups_y, 1);
		// glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	  ErrorHandle("glDispatchCompute");

	  glBindBuffer(GL_UNIFORM_BUFFER, 0);
  }

  //-----------------------------------------------
  void onFinish() {
  }

};

VDP2Generator * VDP2Generator::instance_ = nullptr;

extern "C" {
  void VDP2Generator_init(int width, int height) {
    if (_Ygl->vdp2_use_compute_shader == 0) return;

    VDP2Generator * instance = VDP2Generator::getInstance();
    instance->init( width, height);
  }
  void VDP2Generator_resize(int width, int height) {
    if (_Ygl->vdp2_use_compute_shader == 0) return;
    YGLDEBUG("VDP2Generator_resize\n");
	  VDP2Generator * instance = VDP2Generator::getInstance();
	  instance->resize(width, height);
  }
  void VDP2Generator_update(int tex, YglPerLineInfo *bg, int* prioscreens, int* modescreens, int* isRGB, int * isBlur, int* lncl, GLuint* vdp1fb, Vdp2 *varVdp2Regs ) {
    if (_Ygl->vdp2_use_compute_shader == 0) return;
    VDP2Generator * instance = VDP2Generator::getInstance();
    instance->update(tex, bg, prioscreens, modescreens, isRGB, isBlur, lncl, vdp1fb, varVdp2Regs);
  }
  void VDP2Generator_onFinish() {

    if (_Ygl->vdp2_use_compute_shader == 0) return;
    VDP2Generator * instance = VDP2Generator::getInstance();
    instance->onFinish();
  }
}
