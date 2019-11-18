#include "common_glshader.h"

GLuint _prgid[PG_MAX] = { 0 };

static const GLchar Yglprg_vdp2_sprite_palette_only[] =
"bool isRGBCode(int index) {"
" return false;\n"
"}\n";

static const GLchar Yglprg_vdp2_sprite_palette_rgb[] =
"bool isRGBCode(int index) {"
" return ((index & 0x8000)!=0);\n"
"}\n";

static const GLchar Yglprg_vdp2_sprite_type_0[] =
"FBCol getVDP1PixelCode(vec2 col) {\n"
"  FBCol ret = zeroFBCol();\n"
"  if (any(notEqual(col,vec2(0.0)))) ret.valid = 1;\n"
"  else return ret;\n"
"  ret.code = int(col.x*255.0) | (int(col.y*255.0) << 8);\n"
"  if (isRGBCode(ret.code)) {\n"
"    ret.prio = 0;\n"
"    ret.isRGB = 1;\n"
"    ret.cc = 0;\n"
"    if ((use_sp_win != 0) && (ret.code == 0x8000)) ret.valid = 0;\n"
"    ret.color.rgb = getRGB(ret.code).rgb;\n"
"  } else {\n"
"    ret.prio = (ret.code >> 14) & 0x3;\n"
"    ret.cc = (ret.code >> 11) & 0x7;\n"
"    ret.code = ret.code & 0x7FF;\n"
"    ret.normalShadow = (ret.code == 0x7FE);\n"
"    ret.color.rg = getVec2(ret.code).xy;\n"
"    ret.color.b = 0.0;\n"
"  }\n"
"  return ret;\n"
"}\n";

static const GLchar Yglprg_vdp2_sprite_type_1[] =
"FBCol getVDP1PixelCode(vec2 col) {\n"
"  FBCol ret = zeroFBCol();\n"
"  if (any(notEqual(col,vec2(0.0)))) ret.valid = 1;\n"
"  else return ret;\n"
"  ret.code = int(col.x*255.0) | (int(col.y*255.0) << 8);\n"
"  if (isRGBCode(ret.code)) {\n"
"    ret.prio = 0;\n"
"    ret.isRGB = 1;\n"
"    ret.cc = 0;\n"
"    if ((use_sp_win != 0) && (ret.code == 0x8000)) ret.valid = 0;\n"
"    ret.color.rgb = getRGB(ret.code).rgb;\n"
"  } else {\n"
"    ret.prio = (ret.code >> 13) & 0x7;\n"
"    ret.cc = (ret.code >> 11) & 0x3;\n"
"    ret.code = ret.code & 0x7FF;\n"
"    ret.normalShadow = (ret.code == 0x7FE);\n"
"    ret.color.rg = getVec2(ret.code).xy;\n"
"    ret.color.b = 0.0;\n"
"  }\n"
"  return ret;\n"
"}\n";

static const GLchar Yglprg_vdp2_sprite_type_2[] =
"FBCol getVDP1PixelCode(vec2 col) {\n"
"  FBCol ret = zeroFBCol();\n"
"  if (any(notEqual(col,vec2(0.0)))) ret.valid = 1;\n"
"  else return ret;\n"
"  ret.code = int(col.x*255.0) | (int(col.y*255.0) << 8);\n"
"  if (isRGBCode(ret.code)) {\n"
"    ret.prio = 0;\n"
"    ret.isRGB = 1;\n"
"    ret.cc = 0;\n"
"    if ((use_sp_win != 0) && (ret.code == 0x8000)) ret.valid = 0;\n"
"    ret.color.rgb = getRGB(ret.code).rgb;\n"
"  } else {\n"
"    ret.prio = (ret.code >> 14) & 0x1;\n"
"    ret.cc = (ret.code >> 11) & 0x7;\n"
"    if (use_sp_win == 0) {\n"
"      ret.MSBshadow = (((ret.code >> 15) & 0x1) == 1);\n"
"    } else { \n"
"      ret.spwin = (((ret.code >> 15) & 0x1) == 1);\n"
"      if (ret.code == 0x8000) ret.valid = 0;\n"
"    }\n"
"    ret.code = ret.code & 0x7FF;\n"
"    ret.normalShadow = (ret.code == 0x7FE);\n"
"    ret.color.rg = getVec2(ret.code).xy;\n"
"    ret.color.b = 0.0;\n"
"  }\n"
"  return ret;\n"
"}\n";

static const GLchar Yglprg_vdp2_sprite_type_3[] =
"FBCol getVDP1PixelCode(vec2 col) {\n"
"  FBCol ret = zeroFBCol();\n"
"  if (any(notEqual(col,vec2(0.0)))) ret.valid = 1;\n"
"  else return ret;\n"
"  ret.code = int(col.x*255.0) | (int(col.y*255.0) << 8);\n"
"  if (isRGBCode(ret.code)) {\n"
"    ret.prio = 0;\n"
"    ret.isRGB = 1;\n"
"    ret.cc = 0;\n"
"    if ((use_sp_win != 0) && (ret.code == 0x8000)) ret.valid = 0;\n"
"    ret.color.rgb = getRGB(ret.code).rgb;\n"
"  } else {\n"
"    ret.prio = (ret.code >> 13) & 0x3;\n"
"    ret.cc = (ret.code >> 11) & 0x3;\n"
"    if (use_sp_win == 0) {\n"
"      ret.MSBshadow = (((ret.code >> 15) & 0x1) == 1);\n"
"    } else { \n"
"      ret.spwin = (((ret.code >> 15) & 0x1) == 1);\n"
"      if (ret.code == 0x8000) ret.valid = 0;\n"
"    }\n"
"    ret.code = ret.code & 0x7FF;\n"
"    ret.normalShadow = (ret.code == 0x7FE);\n"
"    ret.color.rg = getVec2(ret.code).xy;\n"
"    ret.color.b = 0.0;\n"
"  }\n"
"  return ret;\n"
"}\n";

static const GLchar Yglprg_vdp2_sprite_type_4[] =
"FBCol getVDP1PixelCode(vec2 col) {\n"
"  FBCol ret = zeroFBCol();\n"
"  if (any(notEqual(col,vec2(0.0)))) ret.valid = 1;\n"
"  else return ret;\n"
"  ret.code = int(col.x*255.0) | (int(col.y*255.0) << 8);\n"
"  if (isRGBCode(ret.code)) {\n"
"    ret.prio = 0;\n"
"    ret.isRGB = 1;\n"
"    ret.cc = 0;\n"
"    if ((use_sp_win != 0) && (ret.code == 0x8000)) ret.valid = 0;\n"
"    ret.color.rgb = getRGB(ret.code).rgb;\n"
"  } else {\n"
"    ret.prio = (ret.code >> 13) & 0x3;\n"
"    ret.cc = (ret.code >> 10) & 0x7;\n"
"    if (use_sp_win == 0) {\n"
"      ret.MSBshadow = (((ret.code >> 15) & 0x1) == 1);\n"
"    } else { \n"
"      ret.spwin = (((ret.code >> 15) & 0x1) == 1);\n"
"      if (ret.code == 0x8000) ret.valid = 0;\n"
"    }\n"
"    ret.code = ret.code & 0x3FF;\n"
"    ret.normalShadow = (ret.code == 0x3FE);\n"
"    ret.color.rg = getVec2(ret.code).xy;\n"
"    ret.color.b = 0.0;\n"
"  }\n"
"  return ret;\n"
"}\n";

static const GLchar Yglprg_vdp2_sprite_type_5[] =
"FBCol getVDP1PixelCode(vec2 col) {\n"
"  FBCol ret = zeroFBCol();\n"
"  if (any(notEqual(col,vec2(0.0)))) ret.valid = 1;\n"
"  else return ret;\n"
"  ret.code = int(col.x*255.0) | (int(col.y*255.0) << 8);\n"
"  if (isRGBCode(ret.code)) {\n"
"    ret.prio = 0;\n"
"    ret.isRGB = 1;\n"
"    ret.cc = 0;\n"
"    if ((use_sp_win != 0) && (ret.code == 0x8000)) ret.valid = 0;\n"
"    ret.color.rgb = getRGB(ret.code).rgb;\n"
"  } else {\n"
"    ret.prio = (ret.code >> 12) & 0x7;\n"
"    ret.cc = (ret.code >> 11) & 0x1;\n"
"    if (use_sp_win == 0) {\n"
"      ret.MSBshadow = (((ret.code >> 15) & 0x1) == 1);\n"
"    } else { \n"
"      ret.spwin = (((ret.code >> 15) & 0x1) == 1);\n"
"      if (ret.code == 0x8000) ret.valid = 0;\n"
"    }\n"
"    ret.code = ret.code & 0x7FF;\n"
"    ret.normalShadow = (ret.code == 0x7FE);\n"
"    ret.color.rg = getVec2(ret.code).xy;\n"
"    ret.color.b = 0.0;\n"
"  }\n"
"  return ret;\n"
"}\n";

static const GLchar Yglprg_vdp2_sprite_type_6[] =
"FBCol getVDP1PixelCode(vec2 col) {\n"
"  FBCol ret = zeroFBCol();\n"
"  if (any(notEqual(col,vec2(0.0)))) ret.valid = 1;\n"
"  else return ret;\n"
"  ret.code = int(col.x*255.0) | (int(col.y*255.0) << 8);\n"
"  if (isRGBCode(ret.code)) {\n"
"    ret.prio = 0;\n"
"    ret.isRGB = 1;\n"
"    ret.cc = 0;\n"
"    if ((use_sp_win != 0) && (ret.code == 0x8000)) ret.valid = 0;\n"
"    ret.color.rgb = getRGB(ret.code).rgb;\n"
"  } else {\n"
"    ret.prio = (ret.code >> 12) & 0x7;\n"
"    ret.cc = (ret.code >> 10) & 0x3;\n"
"    if (use_sp_win == 0) {\n"
"      ret.MSBshadow = (((ret.code >> 15) & 0x1) == 1);\n"
"    } else { \n"
"      ret.spwin = (((ret.code >> 15) & 0x1) == 1);\n"
"      if (ret.code == 0x8000) ret.valid = 0;\n"
"    }\n"
"    ret.code = ret.code & 0x3FF;\n"
"    ret.normalShadow = (ret.code == 0x3FE);\n"
"    ret.color.rg = getVec2(ret.code).xy;\n"
"    ret.color.b = 0.0;\n"
"  }\n"
"  return ret;\n"
"}\n";

static const GLchar Yglprg_vdp2_sprite_type_7[] =
"FBCol getVDP1PixelCode(vec2 col) {\n"
"  FBCol ret = zeroFBCol();\n"
"  if (any(notEqual(col,vec2(0.0)))) ret.valid = 1;\n"
"  else return ret;\n"
"  ret.code = int(col.x*255.0) | (int(col.y*255.0) << 8);\n"
"  if (isRGBCode(ret.code)) {\n"
"    ret.prio = 0;\n"
"    ret.isRGB = 1;\n"
"    ret.cc = 0;\n"
"    if ((use_sp_win != 0) && (ret.code == 0x8000)) ret.valid = 0;\n"
"    ret.color.rgb = getRGB(ret.code).rgb;\n"
"  } else {\n"
"    ret.prio = (ret.code >> 12) & 0x7;\n"
"    ret.cc = (ret.code >> 9) & 0x7;\n"
"    if (use_sp_win == 0) {\n"
"      ret.MSBshadow = (((ret.code >> 15) & 0x1) == 1);\n"
"    } else { \n"
"      ret.spwin = (((ret.code >> 15) & 0x1) == 1);\n"
"      if (ret.code == 0x8000) ret.valid = 0;\n"
"    }\n"
"    ret.code = ret.code & 0x1FF;\n"
"    ret.normalShadow = (ret.code == 0x1FE);\n"
"    ret.color.rg = getVec2(ret.code).xy;\n"
"    ret.color.b = 0.0;\n"
"  }\n"
"  return ret;\n"
"}\n";

static const GLchar Yglprg_vdp2_sprite_type_8[] =
"FBCol getVDP1PixelCode(vec2 col) {\n"
"  FBCol ret = zeroFBCol();\n"
"  if (any(notEqual(col,vec2(0.0)))) ret.valid = 1;\n"
"  else return ret;\n"
"  ret.code = int(col.x*255.0) | (int(col.y*255.0) << 8);\n"
"  if (isRGBCode(ret.code)) {\n"
"    ret.prio = 0;\n"
"    ret.isRGB = 1;\n"
"    ret.cc = 0;\n"
"    if ((use_sp_win != 0) && (ret.code == 0x8000)) ret.valid = 0;\n"
"    ret.color.rgb = getRGB(ret.code).rgb;\n"
"  } else {\n"
"    ret.prio = (ret.code >> 7) & 0x1;\n"
"    ret.code = ret.code & 0x7F;\n"
"    ret.normalShadow = (ret.code == 0x7E);\n"
"    ret.color.rg = getVec2(ret.code).xy;\n"
"    ret.color.b = 0.0;\n"
"  }\n"
"  return ret;\n"
"}\n";

static const GLchar Yglprg_vdp2_sprite_type_9[] =
"FBCol getVDP1PixelCode(vec2 col) {\n"
"  FBCol ret = zeroFBCol();\n"
"  if (any(notEqual(col,vec2(0.0)))) ret.valid = 1;\n"
"  else return ret;\n"
"  ret.code = int(col.x*255.0) | (int(col.y*255.0) << 8);\n"
"  if (isRGBCode(ret.code)) {\n"
"    ret.prio = 0;\n"
"    ret.isRGB = 1;\n"
"    ret.cc = 0;\n"
"    if ((use_sp_win != 0) && (ret.code == 0x8000)) ret.valid = 0;\n"
"    ret.color.rgb = getRGB(ret.code).rgb;\n"
"  } else {\n"
"    ret.prio = (ret.code >> 7) & 0x1;\n"
"    ret.cc = (ret.code >> 6) & 0x1;\n"
"    ret.code = ret.code & 0x3F;\n"
"    ret.normalShadow = (ret.code == 0x3E);\n"
"    ret.color.rg = getVec2(ret.code).xy;\n"
"    ret.color.b = 0.0;\n"
"  }\n"
"  return ret;\n"
"}\n";

static const GLchar Yglprg_vdp2_sprite_type_A[] =
"FBCol getVDP1PixelCode(vec2 col) {\n"
"  FBCol ret = zeroFBCol();\n"
"  if (any(notEqual(col,vec2(0.0)))) ret.valid = 1;\n"
"  else return ret;\n"
"  ret.code = int(col.x*255.0) | (int(col.y*255.0) << 8);\n"
"  if (isRGBCode(ret.code)) {\n"
"    ret.prio = 0;\n"
"    ret.isRGB = 1;\n"
"    ret.cc = 0;\n"
"    if ((use_sp_win != 0) && (ret.code == 0x8000)) ret.valid = 0;\n"
"    ret.color.rgb = getRGB(ret.code).rgb;\n"
"  } else {\n"
"    ret.prio = (ret.code >> 6) & 0x3;\n"
"    ret.code = ret.code & 0x3F;\n"
"    ret.normalShadow = (ret.code == 0x3E);\n"
"    ret.color.rg = getVec2(ret.code).xy;\n"
"    ret.color.b = 0.0;\n"
"  }\n"
"  return ret;\n"
"}\n";

static const GLchar Yglprg_vdp2_sprite_type_B[] =
"FBCol getVDP1PixelCode(vec2 col) {\n"
"  FBCol ret = zeroFBCol();\n"
"  if (any(notEqual(col,vec2(0.0)))) ret.valid = 1;\n"
"  else return ret;\n"
"  ret.code = int(col.x*255.0) | (int(col.y*255.0) << 8);\n"
"  if (isRGBCode(ret.code)) {\n"
"    ret.prio = 0;\n"
"    ret.isRGB = 1;\n"
"    ret.cc = 0;\n"
"    if ((use_sp_win != 0) && (ret.code == 0x8000)) ret.valid = 0;\n"
"    ret.color.rgb = getRGB(ret.code).rgb;\n"
"  } else {\n"
"    ret.cc = (ret.code >> 6) & 0x3;\n"
"    ret.code = ret.code & 0x3F;\n"
"    ret.normalShadow = (ret.code == 0x3E);\n"
"    ret.color.rg = getVec2(ret.code).xy;\n"
"    ret.color.b = 0.0;\n"
"  }\n"
"  return ret;\n"
"}\n";

static const GLchar Yglprg_vdp2_sprite_type_C[] =
"FBCol getVDP1PixelCode(vec2 col) {\n"
"  FBCol ret = zeroFBCol();\n"
"  if (any(notEqual(col,vec2(0.0)))) ret.valid = 1;\n"
"  else return ret;\n"
"  ret.code = int(col.x*255.0) | (int(col.y*255.0) << 8);\n"
"  if (isRGBCode(ret.code)) {\n"
"    ret.prio = 0;\n"
"    ret.isRGB = 1;\n"
"    ret.cc = 0;\n"
"    if ((use_sp_win != 0) && (ret.code == 0x8000)) ret.valid = 0;\n"
"    ret.color.rgb = getRGB(ret.code).rgb;\n"
"  } else {\n"
"    ret.prio = (ret.code >> 7) & 0x1;\n"
"    ret.code = ret.code & 0xFF;\n"
"    ret.normalShadow = (ret.code == 0xFE);\n"
"    ret.color.rg = getVec2(ret.code).xy;\n"
"    ret.color.b = 0.0;\n"
"  }\n"
"  return ret;\n"
"}\n";

static const GLchar Yglprg_vdp2_sprite_type_D[] =
"FBCol getVDP1PixelCode(vec2 col) {\n"
"  FBCol ret = zeroFBCol();\n"
"  if (any(notEqual(col,vec2(0.0)))) ret.valid = 1;\n"
"  else return ret;\n"
"  ret.code = int(col.x*255.0) | (int(col.y*255.0) << 8);\n"
"  if (isRGBCode(ret.code)) {\n"
"    ret.prio = 0;\n"
"    ret.isRGB = 1;\n"
"    ret.cc = 0;\n"
"    if ((use_sp_win != 0) && (ret.code == 0x8000)) ret.valid = 0;\n"
"    ret.color.rgb = getRGB(ret.code).rgb;\n"
"  } else {\n"
"    ret.prio = (ret.code >> 7) & 0x1;\n"
"    ret.cc = (ret.code >> 6) & 0x1;\n"
"    ret.code = ret.code & 0xFF;\n"
"    ret.normalShadow = (ret.code == 0xFE);\n"
"    ret.color.rg = getVec2(ret.code).xy;\n"
"    ret.color.b = 0.0;\n"
"  }\n"
"  return ret;\n"
"}\n";

static const GLchar Yglprg_vdp2_sprite_type_E[] =
"FBCol getVDP1PixelCode(vec2 col) {\n"
"  FBCol ret = zeroFBCol();\n"
"  if (any(notEqual(col,vec2(0.0)))) ret.valid = 1;\n"
"  else return ret;\n"
"  ret.code = int(col.x*255.0) | (int(col.y*255.0) << 8);\n"
"  if (isRGBCode(ret.code)) {\n"
"    ret.prio = 0;\n"
"    ret.isRGB = 1;\n"
"    ret.cc = 0;\n"
"    if ((use_sp_win != 0) && (ret.code == 0x8000)) ret.valid = 0;\n"
"    ret.color.rgb = getRGB(ret.code).rgb;\n"
"  } else {\n"
"    ret.prio = (ret.code >> 6) & 0x3;\n"
"    ret.code = ret.code & 0xFF;\n"
"    ret.normalShadow = (ret.code == 0xFE);\n"
"    ret.color.rg = getVec2(ret.code).xy;\n"
"    ret.color.b = 0.0;\n"
"  }\n"
"  return ret;\n"
"}\n";

static const GLchar Yglprg_vdp2_sprite_type_F[] =
"FBCol getVDP1PixelCode(vec2 col) {\n"
"  FBCol ret = zeroFBCol();\n"
"  if (any(notEqual(col,vec2(0.0)))) ret.valid = 1;\n"
"  else return ret;\n"
"  ret.code = int(col.x*255.0) | (int(col.y*255.0) << 8);\n"
"  if (isRGBCode(ret.code)) {\n"
"    ret.prio = 0;\n"
"    ret.isRGB = 1;\n"
"    ret.cc = 0;\n"
"    if ((use_sp_win != 0) && (ret.code == 0x8000)) ret.valid = 0;\n"
"    ret.color.rgb = getRGB(ret.code).rgb;\n"
"  } else {\n"
"    ret.cc = (ret.code >> 6) & 0x3;\n"
"    ret.code = ret.code & 0xFF;\n"
"    ret.normalShadow = (ret.code == 0xFE);\n"
"    ret.color.rg = getVec2(ret.code).xy;\n"
"    ret.color.b = 0.0;\n"
"  }\n"
"  return ret;\n"
"}\n";

/*
 Color calculation option
  hard/vdp2/hon/p09_21.htm
*/
static const GLchar Yglprg_vdp2_drawfb_cram_no_color_col_f[]    = " fbmode = 0; \n";

static const GLchar Yglprg_vdp2_drawfb_cram_less_color_col_f[]  = " if( depth > u_cctl ){ fbmode = 0;} \n ";
static const GLchar Yglprg_vdp2_drawfb_cram_equal_color_col_f[] = " if( depth != u_cctl ){ fbmode = 0;} \n ";
static const GLchar Yglprg_vdp2_drawfb_cram_more_color_col_f[]  = " if( depth < u_cctl ){ fbmode = 0;} \n ";
static const GLchar Yglprg_vdp2_drawfb_cram_msb_color_col_f[]   = " if( msb == 0 ){ fbmode = 0;} \n ";

static const GLchar Yglprg_vdp2_drawfb_cram_epiloge_none_f[] =
"//No Color calculation\n";
static const GLchar Yglprg_vdp2_drawfb_cram_epiloge_as_is_f[] =
" if (fbmode == 1) vdp1mode = 2; \n";
static const GLchar Yglprg_vdp2_drawfb_cram_epiloge_src_alpha_f[] =
" if (fbmode == 1) vdp1mode = 3; \n";
static const GLchar Yglprg_vdp2_drawfb_cram_epiloge_dst_alpha_f[] =
" if (fbmode == 1) vdp1mode = 4; \n";

static const GLchar Yglprg_vdp2_drawfb_cram_eiploge_f[] =
" }\n"
" tmpColor.a = float(alpha|vdp1mode)/255.0; \n"
" ret.color = tmpColor;\n"
" ret.prio = depth;\n"
" return ret;\n"
"}\n";

static const GLchar Yglprg_vdp2_common_start[] =

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
"vec4 FBTest = vec4(0.0);\n"
"int FBPrio = 0;\n"
"bool FBSPwin = false;\n"
"int FBMesh = 0;\n"
"int FBRgb = 0;\n"
"int FBMeshPrio = 0;\n"
"bool FBNormalShadow = false;\n"
"bool FBMSBShadow = false;\n"
"int NoVdp1 = 0;\n"

"struct Col \n"
"{ \n"
"  vec4 Color; \n"
"  vec4 meshColor;\n"
"  int mesh;\n"
"  int lncl; \n"
"  int mode; \n"
"  int isRGB; \n"
"  int isSprite; \n"
"  bool normalShadow; \n"
"  int layer; \n"
"}; \n"

"struct FBCol \n"
"{ \n"
"  vec4 color; \n"
"  int prio; \n"
"  int mesh;\n"
"  vec4 meshColor; \n"
"  int meshPrio;\n"
"  int cc;\n"
"  int code;\n"
"  int valid;\n"
"  int isRGB;\n"
"  bool MSBshadow;\n"
"  bool spwin;\n"
"  bool normalShadow;\n"
"}; \n"

"FBCol zeroFBCol(){ \n"
"  FBCol ret;\n"
"  ret.color = vec4(0.0); \n"
"  ret.meshColor = vec4(0.0); \n"
"  ret.mesh = 0;\n"
"  ret.prio = 0; \n"
"  ret.meshPrio = 0;\n"
"  ret.cc = 0;\n"
"  ret.code = 0;\n"
"  ret.valid = 0;\n"
"  ret.isRGB = 0;\n"
"  ret.MSBshadow = false;\n"
"  ret.normalShadow = false;\n"
"  ret.spwin = false;\n"
"  return ret;\n"
"}\n"

"vec3 getRGB(int colindex) {\n"
" vec3 ret;\n"
" ret.r = float(((colindex & 0x1F)) & 0x1F)/31.0;\n"
" ret.g = float(((colindex & 0x3E0) >> 5) & 0x1F)/31.0;\n"
" ret.b = float(((colindex & 0x7C00) >> 10) & 0x1F)/31.0;\n"
" return ret;\n"
"}\n"
"vec2 getVec2(int colindex) {\n"
" vec2 ret;\n"
" ret.x = float(colindex & 0xFF)/255.0;\n"
" ret.y = float((colindex & 0xFF00) >> 8)/255.0;\n"
" return ret;\n"
"}\n";

static const GLchar Yglprg_vdp2_common_draw[] =
//GetFB()
"  FBCol ret = zeroFBCol();\n"
"  FBCol mesh = zeroFBCol();\n"
"  if (fbon != 1) return ret;\n"
"  if (u_coloroffset_sign.r != 0.0) u_coloroffset.r = float(int(u_coloroffset.r*255.0)-256.0)/255.0;\n"
"  if (u_coloroffset_sign.g != 0.0) u_coloroffset.g = float(int(u_coloroffset.g*255.0)-256.0)/255.0;\n"
"  if (u_coloroffset_sign.b != 0.0) u_coloroffset.b = float(int(u_coloroffset.b*255.0)-256.0)/255.0;\n"
"  int u_color_ram_offset = getVDP2Reg(23+line)<<8;\n"
"  fbmode = 1;\n"
"  vdp1mode = 1;\n"
"  ivec2 fbCoord = addr + ivec2(x, 0);\n"
"  fbCoord = ivec2(vec4(vdp1Ratio.x* fbCoord.x, vdp1Ratio.y*(fbCoord.y), 1.0, 1.0) * fbMat).xy;\n"
"  vec4 col = texelFetch(s_vdp1FrameBuffer, fbCoord, 0);\n"
"  FBTest = col;\n"
"  ret = getVDP1PixelCode(col.rg);\n"
"  mesh = getVDP1PixelCode(col.ba);"
"  if (mesh.valid != 0) { \n"
"    vec4 meshcol=vec4(0.0,0.0,0.0,1.0);\n"
"    int meshdepth = getVDP2Reg(mesh.prio+8+line);\n"
"    if( mesh.isRGB == 0 ){ \n"// index color?
"      if( mesh.code != 0 || meshdepth != 0){\n"
"        mesh.code = mesh.code + u_color_ram_offset; \n"
"        meshcol = texelFetch( s_color,  ivec2( mesh.code ,0 )  , 0 );\n"
"      } else { \n"
"        meshcol = vec4(0.0);\n"
"      }\n"
"    }else{ // direct color \n"
"      meshcol = mesh.color;\n"
"    } \n"
"    meshcol.rgb = clamp(meshcol.rgb + u_coloroffset, vec3(0.0), vec3(1.0));  \n"
"    ret.meshColor = meshcol;\n"
"    ret.meshPrio = meshdepth;\n"
"    ret.mesh = 1;\n"
"  };\n"
"  if(ret.valid == 0){ return ret;} // show? \n"
"  vec4 tmpColor = vec4(0.0);\n"
"  int u_cctl = getVDP2Reg(16+line);\n"
"  int depth = getVDP2Reg(ret.prio+8+line);\n"
"  int alpha = getVDP2Reg(ret.cc+line)<<3; \n"
"  int opaque = 0xF8;\n"
"  int tmpmesh = 0;\n"
"  int msb = 0;\n" //RGB will have msb set
"  vec4 txcol=vec4(0.0,0.0,0.0,1.0);\n"
"  if( ret.isRGB == 0 ){  // index color? \n"
"    if( ret.code != 0 || depth != 0){\n"
"      ret.code = ret.code + u_color_ram_offset; \n"
"      txcol = texelFetch( s_color,  ivec2( ret.code ,0 )  , 0 );\n"
"      tmpColor = txcol;\n"
"      if (txcol.a != 0.0) msb = 1;\n"
"      else msb = 0;\n"
"    } else { \n"
"      tmpColor = vec4(0.0);\n"
"      msb = 0;\n"
"    }\n"
"  }else{ // direct color \n"
"    tmpColor = ret.color;\n"
"    msb = 1;\n"
"  } \n"
"  tmpColor.rgb = clamp(tmpColor.rgb + u_coloroffset, vec3(0.0), vec3(1.0));  \n"
"  if (fbmode != 0) {\n";

static const GLchar Yglprg_vdp2_common_end[] =
"ivec2 startW0 = ivec2(0);\n"
"ivec2 startW1 = ivec2(0);\n"
"ivec2 endW0 = ivec2(0.0);\n"
"ivec2 endW1 = ivec2(0.0);\n"
"void initLineWindow() {\n"
"  ivec2 linepos; \n "
"  linepos.y = 0; \n "
"  linepos.x = int( (u_vheight-PosY) * u_emu_height);\n"
"  vec4 lineW0 = texelFetch(s_win0,linepos,0);\n"
"  startW0.x = int(((lineW0.r*255.0) + (int(lineW0.g*255.0)<<8))*u_emu_vdp2_width);\n"
"  endW0.x = int(((lineW0.b*255.0) + (int(lineW0.a*255.0)<<8))*u_emu_vdp2_width);\n"
"  startW0.y = int(((lineW0.r*255.0) + (int(lineW0.g*255.0)<<8))*u_emu_vdp1_width);\n"
"  endW0.y = int(((lineW0.b*255.0) + (int(lineW0.a*255.0)<<8))*u_emu_vdp1_width);\n"
"  vec4 lineW1 = texelFetch(s_win1,linepos,0);\n"
"  startW1.x = int(((lineW1.r*255.0) + (int(lineW1.g*255.0)<<8))*u_emu_vdp2_width);\n"
"  endW1.x = int(((lineW1.b*255.0) + (int(lineW1.a*255.0)<<8))*u_emu_vdp2_width);\n"
"  startW1.y = int(((lineW1.r*255.0) + (int(lineW1.g*255.0)<<8))*u_emu_vdp1_width);\n"
"  endW1.y = int(((lineW1.b*255.0) + (int(lineW1.a*255.0)<<8))*u_emu_vdp1_width);\n"
"}\n"
"bool inNormalWindow0(int id, int pos) {\n"
"  bool valid = true; \n"
"  int sW0 = startW0.x;\n"
"  int eW0 = endW0.x;\n"
"  if (id == 6) { sW0 = startW0.y; eW0 = endW0.y;}\n"
"  if (win0_mode[id] != 0) { \n"
"    if ((sW0 < eW0) && ((pos < sW0) || (pos >= eW0))) valid = false;\n"
"    if (sW0 == eW0) valid = false;\n"
"  } else { \n"
"    if ((sW0 < eW0) && ((pos >= sW0) && (pos < eW0))) valid = false;\n"
"  }\n"
"  return valid;\n"
"}\n"
"bool inNormalWindow1(int id, int pos) {\n"
"  bool valid = true; \n"
"  int sW1 = startW1.x;\n"
"  int eW1 = endW1.x;\n"
"  if (id == 6) { sW1 = startW1.y; eW1 = endW1.y;}\n"
"  if (win1_mode[id] != 0) { \n"
"    if ((sW1 < eW1) && ((pos < sW1) || (pos >= eW1))) valid = false;\n"
"    if (sW1 == eW1) valid = false;\n"
"  } else { \n"
"    if ((sW1 < eW1) && ((pos >= sW1) && (pos < eW1))) valid = false;\n"
"  }\n"
"  return valid;\n"
"}\n"
"bool inSpriteWindow(int id) {\n"
" if (win_s_mode[id] == 0) return FBSPwin;\n"
" else return !FBSPwin;\n"
"}\n"
"bool inWindow(int id) {\n"
"  int pos = int(PosX);\n"
"  bool valid = true;\n"
"  if (win_op[id] != 0) {\n"
    //And
"    if (win0[id] != 0) valid = valid && inNormalWindow0(id,pos);\n"
"    if (win1[id] != 0) valid = valid && inNormalWindow1(id,pos);\n"
"    if ((win_s[id] != 0)&&(use_sp_win != 0)) valid = valid && inSpriteWindow(id);\n"
"  } else {\n"
    //Or
"    if ((win1[id] != 0) || (win0[id] != 0) || ((win_s[id] != 0)&&(use_sp_win != 0))) valid = false;\n"
"    if (win0[id] != 0) valid = valid || inNormalWindow0(id,pos);\n"
"    if (win1[id] != 0) valid = valid || inNormalWindow1(id,pos);\n"
"    if ((win_s[id] != 0)&&(use_sp_win != 0)) valid = valid || inSpriteWindow(id);\n"
"  }\n"
"  return valid;\n"
"}\n"
"bool inCCWindow() {\n"
"  if ((win1[7] != 0) || (win0[7] != 0) || (win_s[7] != 0)) {\n"
"    return inWindow(7);\n"
"  } else {return false;}\n"
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
"  empty.normalShadow = false;\n"
"  ret = empty;\n"
"  int priority; \n"
"  ret.normalShadow = false;\n"
"  int alpha; \n"
"  if ((fbon == 1) && (prio == FBPrio) && inWindow(6)) {\n"
"    ret.mode = int(FBColor.a*255.0)&0x7; \n"
"    ret.lncl = u_lncl[6];\n"
"    ret.Color = FBColor; \n"
"    remPrio = remPrio - 1;\n"
"    alpha = int(ret.Color.a*255.0)&0xF8; \n"
"    ret.Color.a = float(alpha>>3)/31.0; \n"
"    ret.isRGB = FBRgb;\n" //Shall not be the case always... Need to get RGB format per pixel
"    ret.isSprite = 1;\n"
"    ret.layer = 6;\n"
"    ret.normalShadow = FBNormalShadow;\n"
"    if (remPrio == 0) return ret;\n"
"  }\n"
"  if (screen_nb == 0) return empty;\n"
"  ret.isSprite = 0;\n"
"  tmpColor = vdp2col0; \n"
"  priority = int(tmpColor.a*255.0)&0x7; \n"
"  if ((priority == prio) && inWindow(0)) {\n"
"    remPrio = remPrio - 1;\n"
"    ret.lncl=u_lncl[0];\n"
"    ret.Color = tmpColor; \n"
"    ret.isRGB = isRGB[0];\n"
"    ret.normalShadow = (isShadow[0] != 0);\n"
"    alpha = int(ret.Color.a*255.0)&0xF8; \n"
"    ret.mode = mode[0]; \n"
"    ret.Color.a = float(alpha>>3)/31.0; \n"
"    ret.layer = 0;\n"
"    if (remPrio == 0) return ret;\n"
"  }\n"
"  if (screen_nb == 1) return empty;\n"
"  tmpColor = vdp2col1; \n"
"  priority = int(tmpColor.a*255.0)&0x7; \n"
"  if ((priority == prio) && inWindow(1)) {\n"
"    remPrio = remPrio - 1;\n"
"    ret.lncl=u_lncl[1];\n"
"    ret.isRGB = isRGB[1];\n"
"    ret.normalShadow = (isShadow[1] != 0);\n"
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
"  if ((priority == prio) && inWindow(2)) {\n"
"    remPrio = remPrio - 1;\n"
"    ret.lncl=u_lncl[2];\n"
"    ret.isRGB = isRGB[2];\n"
"    ret.normalShadow = (isShadow[2] != 0);\n"
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
"  if ((priority == prio) && inWindow(3)) {\n"
"    remPrio = remPrio - 1;\n"
"    ret.lncl=u_lncl[3];\n"
"    ret.isRGB = isRGB[3];\n"
"    ret.normalShadow = (isShadow[3] != 0);\n"
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
"  if ((priority == prio) && inWindow(4)) {\n"
"    remPrio = remPrio - 1;\n"
"    ret.Color = tmpColor; \n"
"    ret.lncl=u_lncl[4];\n"
"    ret.isRGB = isRGB[4];\n"
"    ret.normalShadow = (isShadow[4] != 0);\n"
"    alpha = int(ret.Color.a*255.0)&0xF8; \n"
"    ret.mode = mode[4]; \n"
"    ret.Color.a = float(alpha>>3)/31.0; \n"
"    ret.layer = 4;\n"
"    if (remPrio == 0) return ret;\n"
"  }\n"
"  if (screen_nb == 5) return empty;\n"
"  tmpColor = vdp2col5; \n"
"  priority = int(tmpColor.a*255.0)&0x7; \n"
"  if ((priority == prio) && inWindow(5)) {\n"
"    remPrio = remPrio - 1;\n"
"    ret.Color = tmpColor; \n"
"    ret.lncl=u_lncl[5];\n"
"    ret.isRGB = isRGB[5];\n"
"    ret.normalShadow = (isShadow[5] != 0);\n"
"    alpha = int(ret.Color.a*255.0)&0xF8; \n"
"    ret.mode = mode[5]; \n"
"    ret.Color.a = float(alpha>>3)/31.0; \n"
"    ret.layer = 5;\n"
"    if (remPrio == 0) return ret;\n"
"  }\n"
"  return empty;\n"
"}  \n"

"Col getBlur(ivec2 addr, Col pix) \n"
"{  \n"
"  Col ret = pix;\n"
"  vec4 txcoll;\n"
"  vec4 txcolll;\n"
"  vec4 txcol = pix.Color;\n"
"  if (pix.layer == 6) { \n"
"    txcoll = getFB(-1, addr).color;\n"
"    txcolll = getFB(-2, addr).color;\n"
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
"{  \n";

static const GLchar Yglprg_vdp2_common_final[]=
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
"  int mesh = 0;\n"
"  vec3 meshCol = vec3(0.0);\n"
"  float alphatop = 1.0; \n"
"  float alphasecond = 1.0; \n"
"  float alphathird = 1.0; \n"
"  float alphafourth = 1.0; \n"
"  ivec2 addr = ivec2(textureSize(s_back, 0) * v_texcoord.st); \n"
"  colorback = texelFetch( s_back, addr,0 ); \n"
"  addr = ivec2(tvSize * v_texcoord.st); \n"
"  addr.y += int(float(textureSize(s_vdp1FrameBuffer, 0).y)/vdp1Ratio.y) - tvSize.y;\n"
"  initLineWindow();\n"
"  colortop = colorback; \n"
"  isRGBtop = 1; \n"
"  alphatop = float((int(colorback.a * 255.0)&0xF8)>>3)/31.0;\n"
"  FBCol tmp = getFB(0, addr); \n"
"  FBColor = tmp.color;\n"
"  FBPrio = tmp.prio;\n"
"  FBSPwin = tmp.spwin;\n"
"  FBRgb = tmp.isRGB;\n"
"  bool processShadow = false;\n"
"  FBNormalShadow = tmp.normalShadow;\n"
"  FBMSBShadow = tmp.MSBshadow;\n"
"  FBShadow = tmp.meshColor;\n"
"  FBMeshPrio = tmp.meshPrio;\n"
"  FBMesh = tmp.mesh;\n"
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
"        Col prio = getPriorityColor(i, hasColor);\n"
"        hasColor = hasColor+1;\n"
"        if (prio.mode != 0) { \n"
"          if (foundColor1 == 0) { \n"
"            prio.mode = (prio.mode & 0x7); \n"
"            if (prio.isSprite == 0) {\n"
"              if ((FBMesh == 1) && (fbon == 1) && (i <= FBMeshPrio)) {\n"
"                mesh = 1;\n"
"                meshCol = FBShadow.rgb;\n"
"              }\n"
"              if ((int(prio.Color.b*255.0)&0x1) == 0) {\n"
                 //Special color calulation mode => CC is off on this pixel
                 //Get from VDP2COLOR
"                prio.mode = 1;\n"
"                prio.Color.a = 1.0;\n"
"              }\n"
"            } else {\n"
"              if ((FBMesh == 1) && (fbon == 1)) {\n"
"                mesh = 1;\n"
"                meshCol = FBShadow.rgb;\n"
"              }\n"
"              if (FBNormalShadow) {\n"
//Normal shadow is always a transparent shadow. It does not have to be processed
//As a top image. But the shadow process shall be processed
"                processShadow = true;\n"
"                continue;\n"
"              }\n"
"              if (FBMSBShadow) {\n"
//The MSB shadow is only effetive when the sprite window is not used
"                processShadow = true;\n"
//The shadow process shall be processed for any of color code
"                if (tmp.code == 0) {\n"
//In case of a code of zero and if the transparent shadow code is enabled, then we do not process as a top image
"                    processShadow = (use_trans_shadow != 0);\n"
"                    continue;\n"
"                  }\n"
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
"              linepos.x = int( (u_vheight-PosY) * u_emu_height);\n"
"              colorsecond = texelFetch( s_lncl, linepos ,0 );\n"
"              modesecond = mode[6];\n"
"              alphasecond = float((int(colorsecond.a * 255.0)&0xF8)>>3)/31.0;\n"
"              isRGBsecond = 1;\n"
"              use_lncl = 1;\n"
"              foundColor2 = 1; \n"
"            }\n"
"            colortop = prio.Color; \n"
"            modetop = prio.mode&0x7; \n"
"            isRGBtop = prio.isRGB; \n"
"            alphatop = prio.Color.a; \n"
"            foundColor1 = 1; \n"
"            if (isBlur[prio.layer] != 0) { \n"
"              Col blur = getBlur(addr, prio);"
"              modesecond = blur.mode&0x7; \n"
"              colorsecond = blur.Color; \n"
"              alphasecond = blur.Color.a; \n"
"              isRGBsecond = blur.isRGB; \n"
"              foundColor2 = 1; \n" //semble corriger steep slope
//le mesh mode deconne dans steep slope
"            }\n"
"          } else if (foundColor2 == 0) { \n" // A revoir du coup
"            if (prio.isSprite == 1) {\n"
"              if (FBNormalShadow) {\n"
                 //shadow are transparent and not computed when on lower priority
"                continue;\n"
"              }\n"
"              if (FBMSBShadow) {\n"
"                if ((tmp.code == 0) && (use_trans_shadow == 1)) { \n"
"                  continue;\n"
"                }"
"              }\n"
"            }\n"
"            if (prio.lncl == 0) { \n"
"              colorthird = colorsecond;\n"
"              alphathird = alphasecond;\n"
"              modethird = modesecond;\n"
"              isRGBthird = isRGBsecond;\n"
"            } else { \n"
"              ivec2 linepos; \n "
"              linepos.y = 0; \n "
"              linepos.x = int( (u_vheight-PosY) * u_emu_height);\n"
"              colorthird = texelFetch( s_lncl, linepos ,0 );\n"
"              modethird = mode[6];\n"
"              alphathird = float((int(colorthird.a * 255.0)&0xF8)>>3)/31.0;\n"
"              isRGBthird = 1;\n"
"              use_lncl = 1;\n"
"              foundColor3 = 1; \n"
"            }\n"
"            modesecond = prio.mode&0x7; \n"
"            colorsecond = prio.Color; \n"
"            alphasecond = prio.Color.a; \n"
"            isRGBsecond = prio.isRGB; \n"
"            foundColor2 = 1; \n"
"            if (isBlur[prio.layer] != 0) { \n"
"              Col blur = getBlur(addr, prio);"
"              modesecond = blur.mode&0x7; \n"
"              colorsecond = blur.Color; \n"
"              alphasecond = blur.Color.a; \n"
"              isRGBsecond = blur.isRGB; \n"
"              foundColor2 = 1; \n"
"            }\n"
"          } else if (foundColor3 == 0) { \n"
"            if (prio.isSprite == 1) {\n"
"              if (FBNormalShadow) {\n"
                 //shadow are transparent and not computed when on lower priority
"                continue;\n"
"              }\n"
"              if (FBMSBShadow) {\n"
"                if ((tmp.code == 0) && (use_trans_shadow == 1)) { \n"
"                  continue;\n"
"                }"
"              }\n"
"            }\n"

"            if (prio.lncl == 0) { \n"
"              colorfourth = colorthird;\n"
"              alphafourth = alphathird;\n"
"              isRGBfourth = isRGBthird; \n"
"            } else { \n"
"              ivec2 linepos; \n "
"              linepos.y = 0; \n "
"              linepos.x = int( (u_vheight-PosY) * u_emu_height);\n"
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
"  if ((FBMesh == 1) && (fbon == 1) && (foundColor1 == 0)) {\n"
"    mesh = 1;\n"
"    meshCol = FBShadow.rgb;\n"
"  }\n"
//Take care  of the extended coloration mode
"  if (!inCCWindow()) {\n"
"    if (extended_cc != 0) { \n"
"      if (ram_mode == 0) { \n"
"        if (use_lncl == 0) { \n"
"          if (modesecond == 1) \n"
"            secondImage.rgb = vec3(colorsecond.rgb); \n"
"          else \n"
"            secondImage.rgb = vec3(0.5 * colorsecond.rgb + 0.5 * colorthird.rgb); \n"
"        } else {\n"
"          if (modesecond == 1) \n"
"            secondImage.rgb = vec3(colorsecond.rgb); \n"
"          else {\n"
"            if (modethird == 1) \n"
"              secondImage.rgb = vec3(0.5 * colorsecond.rgb + 0.5 * colorthird.rgb); \n"
"            else \n"
"              secondImage.rgb = vec3(0.66666 * colorsecond.rgb + 0.33334 * colorthird.rgb); \n"
"          }\n"
"        }\n"
"      } else {\n"
"        if (use_lncl == 0) { \n"
"          if (isRGBthird == 0) { \n"
"            secondImage.rgb = vec3(colorsecond.rgb); \n"
"          } else { \n"
"            if (modesecond == 1) { \n"
"              secondImage.rgb = vec3(colorsecond.rgb); \n"
"            } else {\n"
"              secondImage.rgb = vec3(0.5 * colorsecond.rgb + 0.5 * colorthird.rgb); \n"
"            } \n"
"          }\n"
"        } else {\n"
"          if (isRGBthird == 0) { \n"
"            secondImage.rgb = vec3(colorsecond.rgb); \n"
"          } else { \n"
"            if (isRGBfourth == 0) {\n"
"              if (modesecond == 1) secondImage.rgb = vec3(colorsecond.rgb);\n"
"              else secondImage.rgb = vec3(0.5 * colorsecond.rgb + 0.5 * colorthird.rgb);\n"
"            } else { \n"
"              if (modesecond == 1) secondImage.rgb = vec3(colorsecond.rgb);\n"
"              else { \n"
"                if (modethird == 1) secondImage.rgb = vec3(0.5 * colorsecond.rgb + 0.5 * colorthird.rgb);\n"
"                else secondImage.rgb = vec3(0.5 * colorsecond.rgb + 0.25 * colorthird.rgb + 0.25 * colorfourth.rgb);\n"
"              }\n"
"            }\n"
"          }\n"
"        }\n"
"      } \n"
"    } else { \n"
"      secondImage.rgb = vec3(colorsecond.rgb); \n"
"    } \n"
"    if ((mesh == 0) && (FBMesh == 1) && (fbon == 1)) {\n"
"       meshCol = FBShadow.rgb;\n"
"       secondImage.rgb = secondImage.rgb * 0.5 + meshCol.rgb * 0.5;\n"
"    }\n"
"    if (modetop == 1) topImage = vec4(colortop.rgb, 1.0); \n"
"    if (modetop == 2) topImage = vec4(colortop.rgb, 0.0); \n"
"    if (modetop == 3) topImage = vec4(colortop.rgb*alphatop, alphatop); \n"
"    if (modetop == 4) topImage = vec4(colortop.rgb*alphasecond, alphasecond); \n"
"    finalColor = vec4( topImage.rgb + (1.0 - topImage.a) * secondImage.rgb, 1.0); \n"
"  } else {\n"
"    finalColor = vec4(colortop.rgb, 1.0);\n"
"  }\n"
"  if (mesh == 1) finalColor.rgb = finalColor.rgb * 0.5 + meshCol.rgb * 0.5;\n"
"  if (processShadow) finalColor.rgb = finalColor.rgb * 0.5;\n";

static const GLchar vdp2blit_filter_f[] =
"vec4 getPixel(sampler2D tex, vec2 st) {\n"
" ivec2 addr = ivec2(textureSize(tex, 0) * st);\n"
" vec4 result = texelFetch( tex, addr,0 );\n"
//" result.rgb = Filter( tex, st ).rgb;\n"
" return result;\n"
"}\n";

const GLchar * vdp2blit_palette_mode_f[2]= {
  Yglprg_vdp2_sprite_palette_only,
  Yglprg_vdp2_sprite_palette_rgb
};
const GLchar * vdp2blit_srite_type_f[16] = {
  Yglprg_vdp2_sprite_type_0,
  Yglprg_vdp2_sprite_type_1,
  Yglprg_vdp2_sprite_type_2,
  Yglprg_vdp2_sprite_type_3,
  Yglprg_vdp2_sprite_type_4,
  Yglprg_vdp2_sprite_type_5,
  Yglprg_vdp2_sprite_type_6,
  Yglprg_vdp2_sprite_type_7,
  Yglprg_vdp2_sprite_type_8,
  Yglprg_vdp2_sprite_type_9,
  Yglprg_vdp2_sprite_type_A,
  Yglprg_vdp2_sprite_type_B,
  Yglprg_vdp2_sprite_type_C,
  Yglprg_vdp2_sprite_type_D,
  Yglprg_vdp2_sprite_type_E,
  Yglprg_vdp2_sprite_type_F
};
const GLchar * Yglprg_color_condition_f[5] = {
  Yglprg_vdp2_drawfb_cram_no_color_col_f,
  Yglprg_vdp2_drawfb_cram_less_color_col_f,
  Yglprg_vdp2_drawfb_cram_equal_color_col_f,
  Yglprg_vdp2_drawfb_cram_more_color_col_f,
  Yglprg_vdp2_drawfb_cram_msb_color_col_f
};
const GLchar * Yglprg_color_mode_f[4] = {
  Yglprg_vdp2_drawfb_cram_epiloge_none_f,
  Yglprg_vdp2_drawfb_cram_epiloge_as_is_f,
  Yglprg_vdp2_drawfb_cram_epiloge_src_alpha_f,
  Yglprg_vdp2_drawfb_cram_epiloge_dst_alpha_f,
};

const GLchar * pYglprg_vdp2_blit_f[128*5][16];

void initVDP2DrawCode(const GLchar* start, const GLchar* draw, const GLchar* end, const GLchar* final) {
  //VDP2 programs
  for (int i = 0; i<5; i++) {
     // Sprite color calculation condition are separated by 128
    for (int j = 0; j<4; j++) {
     // 4 Sprite color calculation mode
      for (int k = 0; k<2; k++) {
        // Palette only mode or palette/RGB mode
        for (int l = 0; l<16; l++) {
          //16 sprite typed
          int index = 16*(2*(4*i+j)+k)+l;
          pYglprg_vdp2_blit_f[index][0] = start;
          pYglprg_vdp2_blit_f[index][1] = Yglprg_vdp2_common_start;
          pYglprg_vdp2_blit_f[index][2] = vdp2blit_palette_mode_f[k];
          pYglprg_vdp2_blit_f[index][3] = vdp2blit_srite_type_f[l];
          pYglprg_vdp2_blit_f[index][4] = draw;
          pYglprg_vdp2_blit_f[index][5] = Yglprg_vdp2_common_draw;
          pYglprg_vdp2_blit_f[index][6] = Yglprg_color_condition_f[i];
          pYglprg_vdp2_blit_f[index][7] = Yglprg_color_mode_f[j];
          pYglprg_vdp2_blit_f[index][8] = Yglprg_vdp2_drawfb_cram_eiploge_f;
          pYglprg_vdp2_blit_f[index][9] = vdp2blit_filter_f;
          pYglprg_vdp2_blit_f[index][10] = Yglprg_vdp2_common_end;
          pYglprg_vdp2_blit_f[index][11] = end;
          pYglprg_vdp2_blit_f[index][12] = Yglprg_vdp2_common_final;
          pYglprg_vdp2_blit_f[index][13] = final;
          pYglprg_vdp2_blit_f[index][14] =  NULL;
        }
      }
    }
  }
}

static void YglCommon_printShaderError( GLuint shader )
{
  GLsizei bufSize;

  glGetShaderiv(shader, GL_INFO_LOG_LENGTH , &bufSize);
  if (bufSize > 1) {
    GLchar *infoLog;

    infoLog = (GLchar *)malloc(bufSize);
    if (infoLog != NULL) {
      GLsizei length;
      glGetShaderInfoLog(shader, bufSize, &length, infoLog);
      YuiMsg("Shaderlog:\n%s\n", infoLog);
      free(infoLog);
    }
  }
}

int YglInitShader(int id, const GLchar * vertex[], int vcount, const GLchar * frag[], int fcount, const GLchar * tc[], const GLchar * te[], const GLchar * g[] )
{
    GLint compiled,linked;
    GLuint vshader;
    GLuint fshader;
  GLuint tcsHandle = 0;
  GLuint tesHandle = 0;
  GLuint gsHandle = 0;
  YGLLOG( "Compile Program %d\n", id);
   _prgid[id] = glCreateProgram();
    if (_prgid[id] == 0 ) return -1;
    vshader = glCreateShader(GL_VERTEX_SHADER);
    fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vshader, vcount, vertex, NULL);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
       YGLLOG( "Compile error in vertex shader. %d\n", id );
       YglCommon_printShaderError(vshader);
       _prgid[id] = 0;
       return -1;
    }
    glShaderSource(fshader, fcount, frag, NULL);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
       YGLLOG( "Compile error in fragment shader.%d \n", id);
       YglCommon_printShaderError(fshader);
       _prgid[id] = 0;
       return -1;
     }
    glAttachShader(_prgid[id], vshader);
    glAttachShader(_prgid[id], fshader);
  if (tc != NULL){
    if (tc[0] != NULL){
      tcsHandle = glCreateShader(GL_TESS_CONTROL_SHADER);
      if (tcsHandle == 0){
        YGLLOG("GL_TESS_CONTROL_SHADER is not supported\n");
      }
      glShaderSource(tcsHandle, 1, tc, NULL);
      glCompileShader(tcsHandle);
      glGetShaderiv(tcsHandle, GL_COMPILE_STATUS, &compiled);
      if (compiled == GL_FALSE) {
        YGLLOG("Compile error in GL_TESS_CONTROL_SHADER shader.\n");
        YglCommon_printShaderError(tcsHandle);
        _prgid[id] = 0;
        return -1;
      }
      glAttachShader(_prgid[id], tcsHandle);
    }
  }
  if (te != NULL){
    if (te[0] != NULL){
      tesHandle = glCreateShader(GL_TESS_EVALUATION_SHADER);
      if (tesHandle == 0){
        YGLLOG("GL_TESS_EVALUATION_SHADER is not supported\n");
      }
      glShaderSource(tesHandle, 1, te, NULL);
      glCompileShader(tesHandle);
      glGetShaderiv(tesHandle, GL_COMPILE_STATUS, &compiled);
      if (compiled == GL_FALSE) {
        YGLLOG("Compile error in GL_TESS_EVALUATION_SHADER shader.\n");
        YglCommon_printShaderError(tesHandle);
        _prgid[id] = 0;
        return -1;
      }
      glAttachShader(_prgid[id], tesHandle);
    }
  }
  if (g != NULL){
    if (g[0] != NULL){
      gsHandle = glCreateShader(GL_GEOMETRY_SHADER);
      if (gsHandle == 0){
        YGLLOG("GL_GEOMETRY_SHADER is not supported\n");
      }
      glShaderSource(gsHandle, 1, g, NULL);
      glCompileShader(gsHandle);
      glGetShaderiv(gsHandle, GL_COMPILE_STATUS, &compiled);
      if (compiled == GL_FALSE) {
        YGLLOG("Compile error in GL_TESS_EVALUATION_SHADER shader.\n");
        YglCommon_printShaderError(gsHandle);
        _prgid[id] = 0;
        return -1;
      }
      glAttachShader(_prgid[id], gsHandle);
    }
  }
    glLinkProgram(_prgid[id]);
    glGetProgramiv(_prgid[id], GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE) {
       YGLLOG("Link error..\n");
       YglCommon_printShaderError(_prgid[id]);
       _prgid[id] = 0;
       return -1;
    }
    YGLLOG( "Compile Program %d success(%d)\n", id, _prgid[id]);
    return 0;
}

int setupVDP2Prog(Vdp2* varVdp2Regs, int CS) {
  int pgid = PG_VDP2_DRAWFRAMEBUFF_NONE;
  int mode = getSpriteRenderMode(varVdp2Regs);
  const int SPCCN = ((varVdp2Regs->CCCTL >> 6) & 0x01); // hard/vdp2/hon/p12_14.htm#NxCCEN_
  const int CCRTMD = ((varVdp2Regs->CCCTL >> 9) & 0x01); // hard/vdp2/hon/p12_14.htm#CCRTMD_
  const int CCMD = ((varVdp2Regs->CCCTL >> 8) & 0x01);  // hard/vdp2/hon/p12_14.htm#CCMD_
  const int SPLCEN = (varVdp2Regs->LNCLEN & 0x20); // hard/vdp2/hon/p11_30.htm#NxLCEN_

  if ( SPCCN ) {
    const int SPCCCS = (varVdp2Regs->SPCTL >> 12) & 0x3;
    switch (SPCCCS)
    {
      case 0:
        pgid = PG_VDP2_DRAWFRAMEBUFF_LESS_NONE;
        break;
      case 1:
        pgid = PG_VDP2_DRAWFRAMEBUFF_EUQAL_NONE;
        break;
      case 2:
        pgid = PG_VDP2_DRAWFRAMEBUFF_MORE_NONE;
        break;
      case 3:
        pgid = PG_VDP2_DRAWFRAMEBUFF_MSB_NONE;
        break;
   }
  }
  int colormode =  (varVdp2Regs->SPCTL & 0x20) != 0;
  int spritetype =  (varVdp2Regs->SPCTL & 0xF);

  pgid += (mode-NONE)*16*2 + colormode*16 + spritetype;

  if (_prgid[pgid] == 0) {
   if (YglInitDrawFrameBufferShaders(pgid, CS) != 0) {
     abort();
   }
  }
  GLUSEPROG(_prgid[pgid]);
  return pgid;
}

GLuint createCSProgram(int id, int count, const GLchar * cs[]) {

  _prgid[id] = glCreateProgram();
   if (_prgid[id] == 0 ) return -1;

  GLuint result = glCreateShader(GL_COMPUTE_SHADER);
  glShaderSource(result, count, cs, NULL);
  glCompileShader(result);
  GLint status;
  glGetShaderiv(result, GL_COMPILE_STATUS, &status);
  if (status == GL_FALSE) {
    YGLLOG("CS Compile error..\n");
    YglCommon_printShaderError(result);
    _prgid[id] = 0;
    return -1;
  }
  glAttachShader(_prgid[id], result);
  glLinkProgram(_prgid[id]);
  glDetachShader(_prgid[id], result);
  glGetProgramiv(_prgid[id], GL_LINK_STATUS, &status);
  if (status == GL_FALSE) {
    YGLLOG("Link error..\n");
    YglCommon_printShaderError(_prgid[id]);
    _prgid[id] = 0;
    return -1;
  }
  return 0;
}


void compileVDP2Prog(int id, const GLchar *v, int CS){
  YGLLOG("PG_VDP2_DRAWFRAMEBUFF_NONE --START [%d]--\n", arrayid);
  if (CS == 0) {
    if (YglInitShader(id, v, 1, pYglprg_vdp2_blit_f[id-PG_VDP2_DRAWFRAMEBUFF_NONE], 14, NULL, NULL, NULL) != 0) { printf("Error init prog %d\n",id); abort(); }
  } else {
    if (createCSProgram(id, 14, pYglprg_vdp2_blit_f[id-PG_VDP2_DRAWFRAMEBUFF_NONE])!= 0) { printf("Error init prog %d\n",id); abort(); }
  }
  YGLLOG("PG_VDP2_DRAWFRAMEBUFF_NONE --DONE [%d]--\n", arrayid);
}
