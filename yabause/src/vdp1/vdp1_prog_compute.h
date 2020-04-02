#ifndef VDP1_PROG_COMPUTE_H
#define VDP1_PROG_COMPUTE_H

#include "ygl.h"

#define QuoteIdent(ident) #ident
#define Stringify(macro) QuoteIdent(macro)


// To do: In order to know if a pixel has to be considered for a command,
// each command has o be expressed a set of lines (dx,dy) on AD segment, (dx2,dy2) on BC Segment
// if a pixel is on a line, it has to be considered as part of command => it shall simulate the per line rasterizer of the real VDP1

#define POLYGON 0
#define QUAD_POLY 1
#define POLYLINE 2
#define LINE 3
#define DISTORTED 4
#define QUAD 5
#define SYSTEM_CLIPPING 6
#define USER_CLIPPING 7

#define NB_COARSE_RAST_X 16
#define NB_COARSE_RAST_Y 16

#define LOCAL_SIZE_X 4
#define LOCAL_SIZE_Y 4

#define QUEUE_SIZE 512

//#define SHOW_QUAD

static const char vdp1_write_f[] =
SHADER_VERSION_COMPUTE
"#ifdef GL_ES\n"
"precision highp float;\n"
"#endif\n"
"layout(local_size_x = "Stringify(LOCAL_SIZE_X)", local_size_y = "Stringify(LOCAL_SIZE_Y)") in;\n"
"layout(rgba8, binding = 0) writeonly uniform image2D outSurface;\n"
"layout(std430, binding = 1) readonly buffer VDP1FB { uint Vdp1FB[]; };\n"
"layout(location = 2) uniform vec2 upscale;\n"
"void main()\n"
"{\n"
"  ivec2 size = imageSize(outSurface);\n"
"  ivec2 texel = ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);\n"
"  if (texel.x >= size.x || texel.y >= size.y ) return;\n"
"  float x = float(texel.x) * upscale.x;\n"
"  float y = float(size.y - 1 - texel.y) * upscale.y;\n"
"  int idx = int(x) + int(y)*512;\n"
"  float g = float((Vdp1FB[idx] >> 24) & 0xFFu)/255.0;\n"
"  float r = float((Vdp1FB[idx] >> 16) & 0xFFu)/255.0;\n"
"  imageStore(outSurface,texel,vec4(g, r, 0.0, 0.0));\n"
"}\n";

static const char vdp1_read_f[] =
SHADER_VERSION_COMPUTE
"#ifdef GL_ES\n"
"precision highp float;\n"
"#endif\n"
"layout(local_size_x = "Stringify(LOCAL_SIZE_X)", local_size_y = "Stringify(LOCAL_SIZE_Y)") in;\n"
"layout(rgba8, binding = 0) readonly uniform image2D s_texture;  \n"
"layout(std430, binding = 1) writeonly buffer VDP1FB { uint Vdp1FB[]; };\n"
"layout(location = 2) uniform vec2 upscale;\n"
"void main()\n"
"{\n"
"  ivec2 size = imageSize(s_texture);\n"
"  ivec2 texel = ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);\n"
"  if (texel.x >= size.x || texel.y >= size.y ) return;\n"
"  float x = float(texel.x) * upscale.x;\n"
"  float y = float(size.y - 1 - texel.y) * upscale.y;\n"
"  int idx = int(x) + int(y)*512;\n"
"  vec4 pix = imageLoad(s_texture, ivec2(vec2(texel.x,texel.y)*upscale));\n"
"  uint val = (uint(pix.r*255.0)<<24) | (uint(pix.g*255.0)<<16);\n"
"  Vdp1FB[idx] = val;\n"
"}\n";

static const char vdp1_clear_f[] =
SHADER_VERSION_COMPUTE
"#ifdef GL_ES\n"
"precision highp float;\n"
"#endif\n"
"layout(local_size_x = "Stringify(LOCAL_SIZE_X)", local_size_y = "Stringify(LOCAL_SIZE_Y)") in;\n"
"layout(rgba8, binding = 0) writeonly uniform image2D outSurface;\n"
"layout(location = 2) uniform vec4 col;\n"
"void main()\n"
"{\n"
"  ivec2 size = imageSize(outSurface);\n"
"  ivec2 texel = ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);\n"
"  if (texel.x >= size.x || texel.y >= size.y ) return;\n"
"  imageStore(outSurface,texel,col);\n"
"}\n";

#define COLINDEX(A) \
"int col"Stringify(A)" = (int("Stringify(A)".r*255.0) | (int("Stringify(A)".g*255.0)<<8));\n"

#define RECOLINDEX(A) \
"col"Stringify(A)" = (int("Stringify(A)".r*255.0) | (int("Stringify(A)".g*255.0)<<8));\n"

#define SHADOW(A) \
"if ((col"Stringify(A)" & 0x8000) != 0) { \n\
  int Rs = ((col"Stringify(A)" >> 00) & 0x1F)>>1;\n \
  int Gs = ((col"Stringify(A)" >> 05) & 0x1F)>>1;\n \
  int Bs = ((col"Stringify(A)" >> 10) & 0x1F)>>1;\n \
  int MSBs = (col"Stringify(A)" & 0x8000) >> 8;\n \
  "Stringify(A)".r = float(Rs | ((Gs & 0x7)<<5))/255.0;\n \
  "Stringify(A)".g = float((Gs>>3) | (Bs<<2) | MSBs)/255.0;\n \
} \n"

#define MSB_SHADOW(A) \
"int Rm = ((col"Stringify(A)" >> 00) & 0x1F);\n \
int Gm = ((col"Stringify(A)" >> 05) & 0x1F);\n \
int Bm = ((col"Stringify(A)" >> 10) & 0x1F);\n \
int MSBm = 0x80;\n \
"Stringify(A)".r = float(Rm | ((Gm & 0x7)<<5))/255.0;\n \
"Stringify(A)".g = float((Gm>>3) | (Bm<<2) | MSBm)/255.0;\n"

#define HALF_LUMINANCE(A) \
"int Rhl = ((col"Stringify(A)" >> 00) & 0x1F)>>1;\n \
int Ghl = ((col"Stringify(A)" >> 05) & 0x1F)>>1;\n \
int Bhl = ((col"Stringify(A)" >> 10) & 0x1F)>>1;\n \
int MSBhl = (col"Stringify(A)" & 0x8000) >> 8;\n \
"Stringify(A)".r = float(Rhl | ((Ghl & 0x7)<<5))/255.0;\n \
"Stringify(A)".g = float((Ghl>>3) | (Bhl<<2) | MSBhl)/255.0;\n"

#define HALF_TRANPARENT_MIX(A, B) \
"if ((col"Stringify(B)" & 0x8000) != 0) { \
  int Rht = int(clamp(((float((col"Stringify(A)" >> 00) & 0x1F)/31.0) + (float((col"Stringify(B)" >> 00) & 0x1F)/31.0))*0.5, 0.0, 1.0)*31.0);\n \
  int Ght = int(clamp(((float((col"Stringify(A)" >> 05) & 0x1F)/31.0) + (float((col"Stringify(B)" >> 05) & 0x1F)/31.0))*0.5, 0.0, 1.0)*31.0);\n \
  int Bht = int(clamp(((float((col"Stringify(A)" >> 10) & 0x1F)/31.0) + (float((col"Stringify(B)" >> 10) & 0x1F)/31.0))*0.5, 0.0, 1.0)*31.0);\n \
  int MSBht = (col"Stringify(A)" & 0x8000) >> 8;\n \
  "Stringify(A)".r = float(Rht | ((Ght & 0x7)<<5))/255.0;\n \
  "Stringify(A)".g = float((Ght>>3) | (Bht<<2) | MSBht)/255.0;\n \
}\n"

#define GOURAUD_PROCESS(A) "\
float Rg = float((col"Stringify(A)" >> 00) & 0x1F)/31.0;\n \
float Gg = float((col"Stringify(A)" >> 05) & 0x1F)/31.0;\n \
float Bg = float((col"Stringify(A)" >> 10) & 0x1F)/31.0;\n \
int MSBg = (col"Stringify(A)" & 0x8000) >> 8;\n \
Rg = clamp(Rg + mix(mix(pixcmd.G[0],pixcmd.G[4],gouraudcoord.x), mix(pixcmd.G[12],pixcmd.G[8],gouraudcoord.x), gouraudcoord.y), 0.0, 1.0);\n \
Gg = clamp(Gg+ mix(mix(pixcmd.G[1],pixcmd.G[5],gouraudcoord.x), mix(pixcmd.G[13],pixcmd.G[9],gouraudcoord.x), gouraudcoord.y), 0.0, 1.0);\n \
Bg = clamp(Bg + mix(mix(pixcmd.G[2],pixcmd.G[6],gouraudcoord.x), mix(pixcmd.G[14],pixcmd.G[10],gouraudcoord.x), gouraudcoord.y), 0.0, 1.0);\n \
"Stringify(A)".r = float(int(Rg*31.0) | ((int(Gg*31.0) & 0x7)<<5))/255.0;\n \
"Stringify(A)".g = float((int(Gg*31.0)>>3) | (int(Bg*31.0)<<2) | MSBg)/255.0;\n"

static const char vdp1_start_f[] =
SHADER_VERSION_COMPUTE
"#ifdef GL_ES\n"
"precision highp float;\n"
"#endif\n"

"struct cmdparameter_struct{ \n"
"  float G[16];\n"
"  uint priority;\n"
"  uint w;\n"
"  uint h;\n"
"  uint flip;\n"
"  uint type;\n"
"  uint CMDCTRL;\n"
"  uint CMDLINK;\n"
"  uint CMDPMOD;\n"
"  uint CMDCOLR;\n"
"  uint CMDSRCA;\n"
"  uint CMDSIZE;\n"
"  int CMDXA;\n"
"  int CMDYA;\n"
"  int CMDXB;\n"
"  int CMDYB;\n"
"  int CMDXC;\n"
"  int CMDYC;\n"
"  int CMDXD;\n"
"  int CMDYD;\n"
"  uint B[4];\n"
"  int COLOR[4];\n"
"  uint CMDGRDA;\n"
"  uint SPCTL;\n"
"  uint nbStep;\n"
"  float uAstepx;\n"
"  float uAstepy;\n"
"  float uBstepx;\n"
"  float uBstepy;\n"
"  int pad[2];\n"
"};\n"

"layout(local_size_x = "Stringify(LOCAL_SIZE_X)", local_size_y = "Stringify(LOCAL_SIZE_Y)") in;\n"
"layout(rgba8, binding = 0) writeonly uniform image2D outSurface;\n"
"layout(std430, binding = 3) readonly buffer VDP1RAM { uint Vdp1Ram[]; };\n"
"layout(std430, binding = 4) readonly buffer NB_CMD { uint nbCmd[]; };\n"
"layout(std430, binding = 5) readonly buffer CMD { \n"
"  cmdparameter_struct cmd[];\n"
"};\n"
"layout(location = 6) uniform vec2 upscale;\n"
"layout(location = 7) uniform ivec2 sysClip;\n"
"layout(location = 8) uniform ivec4 usrClip;\n"
"layout(location = 9) uniform mat4 rot;\n"
//===================================================================
"vec3 antiAliasedPoint( vec2 P,  vec2 P0, vec2 P1 )\n"
// dist_Point_to_Segment(): get the distance of a point to a segment
//     Input:  a Point P and a Segment S (P0, P1) (in any dimension)
//     Return: the x,y distance from P to S
"{\n"
"  vec2 v = P1 - P0;\n"
"  vec2 w = P - P0;\n"
"  float c1 = dot(w,v);\n"
"  if ( c1 <= 0.0 )\n"
"    return vec3(P0,0.0);\n"
"  float c2 = dot(v,v);\n"
"  if ( c2 <= c1 )\n"
"    return vec3(P1,1.0);\n"
"  float b = c1 / c2;\n"
"  vec2 Pb = P0 + b * v;\n"
"  return vec3(Pb,b);\n"
"}\n"

"uint isOnAQuadLine( vec2 P, vec2 V0, vec2 V1, vec2 sA, vec2 sB, uint step, out vec2 uv){\n"
"  vec2 A = V0 + vec2(0.5);\n"
"  vec2 B = V1 + vec2(0.5);\n"
"  for (uint i=0; i<step; i++) {\n"
//A pixel shall be considered as part of an anti-aliased line if the distance of the pixel center to the line is shorter than (sqrt(0.5), which is the diagonal of the pixel
//This represent the behavior of antialiasing as displayed in vdp1 spec.
"    vec3 d = antiAliasedPoint(P+vec2(0.5), A, B);\n" //Get the projection of the point P to the line segment
"    if (distance(d.xy/upscale, P/upscale+vec2(0.5)) <= 0.7072) {\n" //Test the distance between the projection on line and the center of the pixel
"      float ux= d.z;\n" //u is the relative distance from first point to projected position
"      float uy= (float(i)+0.5)/float(step+1);\n" //v is the ratio between the current line and the total number of lines
"      uv = vec2(ux,uy);\n"
"      return 1u;\n"
"    }\n"
"    A += sA;\n"
"    B += sB;\n"
"  }\n"
"  return 0u;\n"
"}\n"

"vec3 aliasedPoint( vec2 P,  vec2 P0, vec2 P1 )\n"
// dist_Point_to_Segment(): get the distance of a point to a segment
//     Input:  a Point P and a Segment S (P0, P1) (in any dimension)
//     Return: the x,y distance from P to S
"{\n"
"  if (all(equal(P0,P1))) return vec3(P0, 0.0);\n"
"  float r = 0.0;\n"
"  if (abs(P1.y-P0.y) > abs(P1.x-P0.x)) {\n"
"    if(P1.y != P0.y) r = (P.y-P0.y)/(P1.y-P0.y);\n"
"    else r = (P.x-P0.x)/(P1.x-P0.x);\n"
"  } else {\n"
"    if(P1.x != P0.x) r = (P.x-P0.x)/(P1.x-P0.x);\n"
"    else r = (P.y-P0.y)/(P1.y-P0.y);\n"
"  }"
"  if (r >= 1.0) return vec3(P1, 1.0);\n"
"  else if (r <= 0.0) return vec3(P0, 0.0);\n"
"  else return vec3(P0 + ((P1-P0) * r), r);\n"
"}\n"

"uint isOnALine( vec2 P, vec2 V0, vec2 V1, out vec2 uv){\n"
//A pixel shall be considered as part of an aliased line if all coordinates of the pixel center to the line is shorter than 0.5, which is the horizontal or vertical half-size of the pixel
//This represent the behavior of aliasing as displayed in vdp1 spec.
"  vec2 A = V0+vec2(0.5);\n" //Get the center of the first point
"  vec2 B = V1+vec2(0.5);\n" //Get the center of the last point
"  vec3 d = aliasedPoint(P+vec2(0.5), A, B);\n" //Get the projection of the point P to the line segment
"  if (all(lessThanEqual(abs(floor(d.xy)-floor(P+vec2(0.5))), vec2(0.5*upscale)))) {\n" //If the line is passing through the same pixel as the point P
"    float ux= d.z;\n" //u is the relative distance from first point to projected position
"    float uy= 0.5;\n" //v is the ratio between the current line and the total number of lines
"    uv = vec2(ux,uy);\n"
"    return 1u;\n"
"  }\n"
"  return 0u;\n"
"}\n"

"uint isOnAQuad(vec2 P, vec2 V0, vec2 V1, out vec2 uv) {\n"
"  uint step = uint(abs(V1.y-V0.y)+1);\n"
"  float way = sign(V1.y - V0.y);\n"
"  vec2 A = V0+vec2(0.5);\n"
"  vec2 B = vec2(V1.x, V0.y)+vec2(0.5);\n"
"  for (uint i=0; i<step; i++) {\n"
"    vec3 d = antiAliasedPoint(P+vec2(0.5), A, B);\n"
"    if (distance(d.xy/upscale, P/upscale+vec2(0.5)) <= 0.7072) {\n"
"      float ux= d.z;\n"
"      float uy= float(i)/float(step+1);\n"
"      uv = vec2(ux,uy);\n"
"      return 1u;\n"
"    }\n"
"    A.y += way;\n"
"    B.y += way;\n"
"  }\n"
"  return 0u;\n"
"}\n"

"uint pixIsInside (vec2 Pin, uint idx, out vec2 uv){\n"
"  vec2 Quad[4];\n"
"  if (cmd[idx].type >= "Stringify(SYSTEM_CLIPPING)") return 6u;\n"
//Bounding box test
"  if (any(lessThan(Pin, ivec2(cmd[idx].B[0],cmd[idx].B[2]))) || any(greaterThan(Pin, ivec2(cmd[idx].B[1],cmd[idx].B[3])))) return 0u;\n"
"  Quad[0] = vec2(cmd[idx].CMDXA,cmd[idx].CMDYA)*upscale;\n"
"  Quad[1] = vec2(cmd[idx].CMDXB,cmd[idx].CMDYB)*upscale;\n"
"  Quad[2] = vec2(cmd[idx].CMDXC,cmd[idx].CMDYC)*upscale;\n"
"  Quad[3] = vec2(cmd[idx].CMDXD,cmd[idx].CMDYD)*upscale;\n"

"  if ((cmd[idx].type == "Stringify(DISTORTED)") || (cmd[idx].type == "Stringify(POLYGON)")) {\n"
"    return isOnAQuadLine(Pin, Quad[0], Quad[1], vec2(cmd[idx].uAstepx, cmd[idx].uAstepy)*upscale, vec2(cmd[idx].uBstepx, cmd[idx].uBstepy)*upscale, uint(float(cmd[idx].nbStep)), uv);\n"
"  } else {\n"
"    if ((cmd[idx].type == "Stringify(QUAD)")  || (cmd[idx].type == "Stringify(QUAD_POLY)")) {\n"
"     return isOnAQuad(Pin, Quad[0], Quad[2], uv);\n"
"    } else if (cmd[idx].type == "Stringify(POLYLINE)") {\n"
"      if (isOnALine(Pin, Quad[0], Quad[1], uv) != 0u) return 1u;\n"
"      if (isOnALine(Pin, Quad[1], Quad[2], uv) != 0u) return 1u;\n"
"      if (isOnALine(Pin, Quad[2], Quad[3], uv) != 0u) return 1u;\n"
"      if (isOnALine(Pin, Quad[3], Quad[0], uv) != 0u) return 1u;\n"
"      return 0u;\n"
"    } else if (cmd[idx].type == "Stringify(LINE)") {\n"
"      if (isOnALine(Pin, Quad[0], Quad[1], uv) != 0u) return 1u;\n"
"    }\n"
"  }\n"
"  return 0u;\n"
"}\n"

"int getCmd(vec2 P, uint id, uint start, uint end, out uint zone, bool wait_sysclip, out vec2 uv)\n"
"{\n"
"  for(uint i=id+start; i<id+end; i++) {\n"
"     if (wait_sysclip && (cmd[i].type != "Stringify(SYSTEM_CLIPPING)")) continue;"
"     zone = pixIsInside(P, i, uv);\n"
"     if (zone != 0u) {\n"
"       return int(i);\n"
"     }\n"
"  }\n"
"  return -1;\n"
"}\n"

"uint Vdp1RamReadByte(uint addr) {\n"
"  addr &= 0x7FFFFu;\n"
"  uint read = Vdp1Ram[addr>>2];\n"
"  return (read>>(8*(addr&0x3u)))&0xFFu;\n"
"}\n"

"uint Vdp1RamReadWord(uint addr) {\n"
"  addr &= 0x7FFFFu;\n"
"  uint read = Vdp1Ram[addr>>2];\n"
"  if( (addr & 0x02u) != 0u ) { read >>= 16; } \n"
"  return (((read) >> 8 & 0xFFu) | ((read) & 0xFFu) << 8);\n"
"}\n"

"vec4 VDP1COLOR(uint CMDCOLR) {\n"
"  return vec4(float((CMDCOLR>>0)&0xFFu)/255.0,float((CMDCOLR>>8)&0xFFu)/255.0,0.0,0.0);\n"
"}\n"

"vec4 ReadSpriteColor(cmdparameter_struct pixcmd, vec2 uv, vec2 texel, out bool discarded){\n"
"  vec4 color = vec4(0.0);\n"
"      if(uv.x==1.0) uv.x = 0.99;\n"
"      if(uv.y==1.0) uv.y = 0.99;\n"
"  uint x = clamp(uint(uv.x*pixcmd.w), 0u, uint(pixcmd.w));\n"
"  uint pos = (uint(clamp(ceil(pixcmd.h*uv.y), 0u, uint(pixcmd.h-1)))*pixcmd.w+x);\n"
"  uint charAddr = ((pixcmd.CMDSRCA * 8)& 0x7FFFFu) + pos;\n"
"  uint dot;\n"
"  bool SPD = ((pixcmd.CMDPMOD & 0x40u) != 0);\n"
"  bool END = ((pixcmd.CMDPMOD & 0x80u) != 0);\n"
"  discarded = false;\n"
"  switch ((pixcmd.CMDPMOD >> 3) & 0x7u)\n"
"  {\n"
"    case 0:\n"
"    {\n"
      // 4 bpp Bank mode
"      uint colorBank = pixcmd.CMDCOLR & 0xFFF0u;\n"
"      uint i;\n"
"      charAddr = pixcmd.CMDSRCA * 8 + pos/2;\n"
"      dot = Vdp1RamReadByte(charAddr);\n"
"       if ((x & 0x1u) == 0u) dot = (dot>>4)&0xFu;\n"
"       else dot = (dot)&0xFu;\n"
"      if ((dot == 0x0Fu) && !END) {\n"
"        discarded = true;\n"
"      }\n"
"      else if ((dot == 0) && !SPD) {\n"
"        discarded = true;\n"
"      }\n"
"      else color = VDP1COLOR(dot | colorBank);\n"
"      break;\n"
"    }\n"
"    case 1:\n"
"    {\n"
      // 4 bpp LUT mode
"       uint temp;\n"
"       charAddr = pixcmd.CMDSRCA * 8 + pos/2;\n"
"       uint colorLut = pixcmd.CMDCOLR * 8;\n"
"       dot = Vdp1RamReadByte(charAddr);\n"
"       if ((x & 0x1u) == 0u) dot = (dot>>4)&0xFu;\n"
"       else dot = (dot)&0xFu;\n"
"       if ((dot == 0x0Fu) && !END) {\n"
"        discarded = true;\n"
"      }\n"
"      else if ((dot == 0) && !SPD) {\n"
"        discarded = true;\n"
"      }\n"
"       else {\n"
"         temp = Vdp1RamReadWord((dot * 2 + colorLut));\n"
"         color = VDP1COLOR(temp);\n"
"       }\n"
"       break;\n"
"    }\n"
"    case 2:\n"
"    {\n"
      // 8 bpp(64 color) Bank mode
"      uint colorBank = pixcmd.CMDCOLR & 0xFFC0u;\n"
"      dot = Vdp1RamReadByte(charAddr);\n"
"      if ((dot == 0xFFu) && !END) {\n"
"        discarded = true;\n"
"      }\n"
"      else if ((dot == 0) && !SPD) {\n"
"        discarded = true;\n"
"      }\n"
"      else {\n"
"        color = VDP1COLOR((dot & 0x3Fu) | colorBank);\n"
"      }\n"
"      break;\n"
"    }\n"
"    case 3:\n"
"    {\n"
      // 8 bpp(128 color) Bank mode
"      uint colorBank = pixcmd.CMDCOLR & 0xFF80u;\n"
"      dot = Vdp1RamReadByte(charAddr);\n"
"      if ((dot == 0xFFu) && !END) {\n"
"        discarded = true;\n"
"      }\n"
"      else if ((dot == 0) && !SPD) {\n"
"        discarded = true;\n"
"      }\n"
"      else {\n"
"        color = VDP1COLOR((dot & 0x7Fu) | colorBank);\n"
"      }\n"
"      break;\n"
"    }\n"
"    case 4:\n"
"    {\n"
      // 8 bpp(256 color) Bank mode
"      uint colorBank = pixcmd.CMDCOLR & 0xFF00u;\n"
"      dot = Vdp1RamReadByte(charAddr);\n"
"      if ((dot == 0xFFu) && !END) {\n"
"        discarded = true;\n"
"      }\n"
"      else if ((dot == 0) && !SPD) {\n"
"        discarded = true;\n"
"      }\n"
"      else {\n"
"          color = VDP1COLOR(dot | colorBank);\n"
"      }\n"
"      break;\n"
"    }\n"
"    case 5:\n"
"    {\n"
      // 16 bpp Bank mode
"      uint temp;\n"
"      charAddr += pos;\n"
"      temp = Vdp1RamReadWord(charAddr);\n"
"      if ((temp == 0x7FFFu) && !END) {\n"
"        discarded = true;\n"
"      }\n"
"      else if (((temp & 0x8000u) == 0) && !SPD) {\n"
"        discarded = true;\n"
"      }\n"
"      else {\n"
"        color = VDP1COLOR(temp);\n"
"      }\n"
"      break;\n"
"    }\n"
"    default:\n"
"      break;\n"
"  }\n"
"  return color;\n"
"}\n"

"vec4 extractPolygonColor(cmdparameter_struct pixcmd){\n"
"  uint color = pixcmd.COLOR[0];\n"
"  return VDP1COLOR(color);\n"
"};\n"

"void main()\n"
"{\n"
"  vec4 finalColor = vec4(0.0);\n"
"  vec4 newColor = vec4(0.0);\n"
"  vec4 outColor = vec4(0.0);\n"
"  vec2 tag = vec2(0.0);\n"
"  cmdparameter_struct pixcmd;\n"
"  bool discarded = false;\n"
"  bool drawn = false;\n"
"  vec2 texcoord = vec2(0);\n"
"  vec2 gouraudcoord = vec2(0.0);\n"
"  ivec2 size = imageSize(outSurface);\n"
"  ivec2 pos = ivec2(gl_GlobalInvocationID.xy);\n"
"  if (pos.x >= size.x || pos.y >= size.y ) return;\n"
"  vec2 texel = (vec4(float(pos.x),float(-pos.y), 1.0, 1.0) *rot).xy;\n"
"  texel.y = -texel.y;\n"
"  ivec2 index = ivec2((texel.x*"Stringify(NB_COARSE_RAST_X)")/size.x, (texel.y*"Stringify(NB_COARSE_RAST_Y)")/size.y);\n"
"  ivec2 syslimit = sysClip;\n"
"  ivec4 userlimit = usrClip;\n"
"  uint lindex = index.y*"Stringify(NB_COARSE_RAST_X)"+ index.x;\n"
"  uint cmdIndex = lindex * "Stringify(QUEUE_SIZE)"u;\n"

"  if (nbCmd[lindex] == 0u) return;\n"
"  uint idCmd = 0;\n"
"  uint zone = 0;\n"
"  int cmdindex = 0;\n"
"  bool useGouraud = false;\n"
"  bool waitSysClip = false;\n"
"  vec2 OriginTexel = texel;\n"
"  while ((cmdindex != -1) && (idCmd<nbCmd[lindex]) ) {\n"
"    discarded = false;\n"
"    vec2 uv = vec2(0.0);\n"
"    newColor = vec4(0.0);\n"
"    outColor = vec4(0.0);\n"
"    texel = OriginTexel;\n"
"    cmdindex = getCmd(texel, cmdIndex, idCmd, nbCmd[lindex], zone, waitSysClip, uv);\n"
"    if (cmdindex == -1) continue;\n"
"    idCmd = cmdindex + 1 - cmdIndex;\n"
"    pixcmd = cmd[cmdindex];\n"
"    if (pixcmd.type == "Stringify(SYSTEM_CLIPPING)") {\n"
"      syslimit = ivec2(pixcmd.CMDXC+1+2*int(ceil(rot[0].w)),pixcmd.CMDYC+1+2*int(ceil(rot[1].w)));\n"
"      waitSysClip = false;\n"
"      continue;\n"
"    }\n"
"    if (pixcmd.type == "Stringify(USER_CLIPPING)") {\n"
"      userlimit = ivec4(pixcmd.CMDXA,pixcmd.CMDYA,pixcmd.CMDXC+1+int(ceil(rot[0].w)),pixcmd.CMDYC+1+int(ceil(rot[1].w)));\n"
"      continue;\n"
"    }\n"
"    if (any(greaterThan(pos,syslimit*upscale))) { \n"
"      waitSysClip = true;\n"
"      continue;\n"
"    }"
"    if (((pixcmd.CMDPMOD >> 9) & 0x3u) == 2u) {\n"
//Draw inside
"      if (any(lessThan(pos,userlimit.xy*upscale)) || any(greaterThan(pos,userlimit.zw*upscale))) continue;\n"
"    }\n"
"    if (((pixcmd.CMDPMOD >> 9) & 0x3u) == 3u) {\n"
//Draw outside
"      if (all(greaterThanEqual(pos,userlimit.xy*upscale)) && all(lessThanEqual(pos,userlimit.zw*upscale))) continue;\n"
"    }\n"
"    useGouraud = ((pixcmd.CMDPMOD & 0x4u) == 0x4u);\n"
"    texcoord = uv;\n"
"    gouraudcoord = texcoord;\n"
"    if ((pixcmd.flip & 0x1u) == 0x1u) texcoord.x = 1.0 - texcoord.x;\n" //invert horizontally
"    if ((pixcmd.flip & 0x2u) == 0x2u) texcoord.y = 1.0 - texcoord.y;\n" //invert vertically
"    if (pixcmd.type <= "Stringify(LINE)") {\n"
"      newColor = extractPolygonColor(pixcmd);\n"
"    } else if (pixcmd.type <= "Stringify(QUAD)") {\n"
"      newColor = ReadSpriteColor(pixcmd, texcoord, texel, discarded);\n"
"    }\n"
"    if (discarded) continue;\n"
"    else drawn = true;\n"
"    texel = OriginTexel;\n"
     COLINDEX(finalColor)
     COLINDEX(newColor)
"    if ((pixcmd.CMDPMOD & 0x8000u) == 0x8000u) {\n"
       //MSB shadow
       MSB_SHADOW(finalColor)
"      outColor.rg = finalColor.rg;\n"
"    } else {"
"      switch (pixcmd.CMDPMOD & 0x7u){\n"
"        case 0u: {\n"
           // replace_mode
"          outColor.rg = newColor.rg;\n"
"          }; break;\n"
"        case 1u: {\n"
           //shadow_mode,
           SHADOW(finalColor)
"          outColor.rg = finalColor.rg;\n"
"          }; break;\n"
"        case 2u: {\n"
           //half_luminance_mode,
           HALF_LUMINANCE(newColor)
"          outColor.rg = newColor.rg;\n"
"          }; break;\n"
"        case 3u: {\n"
           //half_trans_mode,
           HALF_TRANPARENT_MIX(newColor, finalColor)
"          outColor.rg = newColor.rg;\n"
"          }; break;\n"
"        case 4u: {\n"
           //gouraud_mode,
           GOURAUD_PROCESS(newColor)
"          outColor.rg = newColor.rg;\n"
"          }; break;\n"
"        case 6u: {\n"
           //gouraud_half_luminance_mode,
           GOURAUD_PROCESS(newColor)
           RECOLINDEX(newColor)
           HALF_LUMINANCE(newColor)
"          outColor.rg = newColor.rg;\n"
"          }; break;\n"
"        case 7u: {\n"
           //gouraud_half_trans_mode,
           GOURAUD_PROCESS(newColor)
           RECOLINDEX(newColor)
           HALF_TRANPARENT_MIX(newColor, finalColor)
"          outColor.rg = newColor.rg;\n"
"          }; break;\n"
"        default:\n"
"          discarded = true;\n"
"          break;\n"
"      }\n"
"    }\n";


static const char vdp1_standard_mesh_f[] =
//Normal mesh
"  tag = vec2(0.0);\n"
"  if ((pixcmd.CMDPMOD & 0x100u)==0x100u){\n"//IS_MESH
"    if( (int(texel.y) & 0x01) == 0 ){ \n"
"      if( (int(texel.x) & 0x01) == 0 ){ \n"
"        discarded = true;\n"
"        continue;\n"
"      }\n"
"    }else{ \n"
"      if( (int(texel.x) & 0x01) == 1 ){ \n"
"        discarded = true;\n"
"        continue;\n"
"      } \n"
"    } \n"
"  }\n";

static const char vdp1_improved_mesh_f[] =
//Improved mesh
"  if ((pixcmd.CMDPMOD & 0x100u)==0x100u){\n"//IS_MESH
"    tag = outColor.rg;\n"
"    outColor.rg = finalColor.rg;\n"
"  } else {\n"
"    tag = vec2(0.0);\n"
"  }\n";

static const char vdp1_continue_f[] =
"    if (drawn) {"
"      finalColor.ba = tag;\n"
"      finalColor.rg = outColor.rg;\n"
"    }\n";

static const char vdp1_end_f[] =
"  }\n"
"  if (!drawn) return;\n"
"  imageStore(outSurface,ivec2(int(pos.x), int(size.y - 1.0 - pos.y)),finalColor);\n"
"}\n";
#endif //VDP1_PROG_COMPUTE_H
