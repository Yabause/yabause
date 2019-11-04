#ifndef VDP1_PROG_COMPUTE_H
#define VDP1_PROG_COMPUTE_H

#include "ygl.h"

#define QuoteIdent(ident) #ident
#define Stringify(macro) QuoteIdent(macro)

#define POLYGON 0
#define DISTORTED 1
#define NORMAL 2
#define SCALED 3
#define POLYLINE 4
#define LINE 5
#define SYSTEM_CLIPPING 6
#define USER_CLIPPING 7

#define NB_COARSE_RAST_X 8
#define NB_COARSE_RAST_Y 8

#define LOCAL_SIZE_X 8
#define LOCAL_SIZE_Y 8

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
"  int idx = int(texel.x * upscale.x) + int((size.y - 1.0 - texel.y)*512 * upscale.y);\n"
"  float a = float((Vdp1FB[idx] >> 24) & 0xFFu)/255.0;\n"
"  float r = float((Vdp1FB[idx] >> 16) & 0xFFu)/255.0;\n"
"  float g = float((Vdp1FB[idx] >> 8) & 0xFFu)/255.0;\n"
"  float b = float((Vdp1FB[idx] >> 0) & 0xFFu)/255.0;\n"
"  if ((int(b * 255.0)&0x80) != 0)\n"
"    imageStore(outSurface,texel,vec4(a,r,g,b));\n"
"}\n";

static const char vdp1_read_f[] =
SHADER_VERSION_COMPUTE
"#ifdef GL_ES\n"
"precision highp float;\n"
"#endif\n"
"layout(local_size_x = "Stringify(LOCAL_SIZE_X)", local_size_y = "Stringify(LOCAL_SIZE_Y)") in;\n"
"layout(std430, binding = 0) writeonly buffer VDP1FB { uint Vdp1FB[]; };\n"
"layout(rgba8, binding = 1) readonly uniform image2D s_texture;  \n"
"layout(location = 2) uniform vec2 upscale;\n"
"void main()\n"
"{\n"
"  ivec2 size = imageSize(s_texture);\n"
"  ivec2 texel = ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);\n"
"  if (texel.x >= size.x || texel.y >= size.y ) return;\n"
"  int idx = int(texel.x * upscale.x) + int((size.y - 1.0 - texel.y)*512 * upscale.y);\n"
"  vec4 pix = imageLoad(s_texture, ivec2(vec2(texel.x,texel.y)*upscale));\n"
"  uint val = (uint(pix.r*255.0)<<24) | (uint(pix.g*255.0)<<16) | (uint(pix.b*255.0)<<8) | (uint(pix.a*255.0)<<0);\n"
"  Vdp1FB[idx] = val;\n"
"}\n";

static const char vdp1_clear_f[] =
SHADER_VERSION_COMPUTE
"#ifdef GL_ES\n"
"precision highp float;\n"
"#endif\n"
"layout(local_size_x = "Stringify(LOCAL_SIZE_X)", local_size_y = "Stringify(LOCAL_SIZE_Y)") in;\n"
"layout(rgba8, binding = 0) writeonly uniform image2D outSurface;\n"
"layout(rgba8, binding = 1) writeonly uniform image2D outSurfaceAttr;\n"
"layout(location = 2) uniform vec4 col;\n"
"void main()\n"
"{\n"
"  ivec2 size = imageSize(outSurface);\n"
"  ivec2 texel = ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);\n"
"  if (texel.x >= size.x || texel.y >= size.y ) return;\n"
"  imageStore(outSurface,texel,col);\n"
"  imageStore(outSurfaceAttr,texel,vec4(0.0));\n"
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
#ifdef USE_VDP1_TEX
"  uint texX;\n"
"  uint texY;\n"
"  uint texW;\n"
"  uint texH;\n"
#endif
"  uint flip;\n"
"  uint cor;\n"
"  uint cob;\n"
"  uint cog;\n"
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
"  int P[8];\n"
"  uint B[4];\n"
"  int COLOR[4];\n"
"  uint CMDGRDA;\n"
"  uint SPCTL;\n"
"};\n"

"layout(local_size_x = "Stringify(LOCAL_SIZE_X)", local_size_y = "Stringify(LOCAL_SIZE_Y)") in;\n"
"layout(rgba8, binding = 0) writeonly uniform image2D outSurface;\n"
"layout(rgba8, binding = 1) writeonly uniform image2D outSurfaceAttr;\n"
#ifdef USE_VDP1_TEX
"layout(location = 2) uniform sampler2D texSurface;\n"
#endif
"layout(std430, binding = 3) readonly buffer VDP1RAM { uint Vdp1Ram[]; };\n"
"layout(std430, binding = 4) readonly buffer NB_CMD { uint nbCmd[]; };\n"
"layout(std430, binding = 5) readonly buffer CMD { \n"
"  cmdparameter_struct cmd[];\n"
"};\n"
"layout(location = 6) uniform vec2 upscale;\n"
"layout(location = 7) uniform ivec2 sysClip;\n"
"layout(location = 8) uniform ivec4 usrClip;\n"
"layout(location = 9) uniform mat4 rot;\n"
// from here http://geomalgorithms.com/a03-_inclusion.html
// a Point is defined by its coordinates {int x, y;}
//===================================================================
// isLeft(): tests if a point is Left|On|Right of an infinite line.
//    Input:  three points P0, P1, and P2
//    Return: >0 for P2 left of the line through P0 and P1
//            =0 for P2  on the line
//            <0 for P2  right of the line
//    See: Algorithm 1 "Area of Triangles and Polygons"
"float isLeft( vec2 P0, vec2 P1, vec2 P2 ){\n"
//This can be used to detect an exact edge
"    return ( (P1.x - P0.x) * (P2.y - P0.y) - (P2.x -  P0.x) * (P1.y - P0.y) );\n"
"}\n"
// wn_PnPoly(): winding number test for a point in a polygon
//      Input:   P = a point,
//               V[] = vertex points of a polygon V[n+1] with V[n]=V[0]
//      Return:  wn = the winding number (=0 only when P is outside)
"uint wn_PnPoly( vec2 P, vec2 V[5]){\n"
"  uint wn = 0;\n"    // the  winding number counter
   // loop through all edges of the polygon
"  for (int i=0; i<4; i++) {\n"   // edge from V[i] to  V[i+1]
"    if (V[i].y <= P.y) {\n"          // start y <= P.y
"      if (V[i+1].y > P.y) \n"      // an upward crossing
"        if (isLeft( V[i], V[i+1], P) > 0)\n"  // P left of  edge
"          ++wn;\n"            // have  a valid up intersect
"    }\n"
"    else {\n"                        // start y > P.y (no test needed)
"      if (V[i+1].y <= P.y)\n"     // a downward crossing
"        if (isLeft( V[i], V[i+1], P) < 0)\n"  // P right of  edge
"          --wn;\n"            // have  a valid down intersect
"    }\n"
"  }\n"
"  return wn;\n"
"}\n"
//===================================================================

"vec2 dist( vec2 P,  vec2 P0, vec2 P1 )\n"
// dist_Point_to_Segment(): get the distance of a point to a segment
//     Input:  a Point P and a Segment S (P0, P1) (in any dimension)
//     Return: the x,y distance from P to S
"{\n"
"  vec2 v = P1 - P0;\n"
"  vec2 w = P - P0;\n"
"  float c1 = dot(w,v);\n"
"  if ( c1 <= 0 )\n"
"    return abs(P-P0);\n"
"  float c2 = dot(v,v);\n"
"  if ( c2 <= c1 )\n"
"    return abs(P-P1);\n"
"  float b = c1 / c2;\n"
"  vec2 Pb = P0 + b * v;\n"
"  return abs(P-Pb);\n"
"}\n"

"uint pixIsInside (ivec2 Pin, uint idx){\n"
"  vec2 Quad[5];\n"
"  vec2 P;\n"
"  if (cmd[idx].type >= "Stringify(SYSTEM_CLIPPING)") return 6u;\n"
"  if (any(lessThan(Pin, ivec2(cmd[idx].B[0],cmd[idx].B[2]))) || any(greaterThan(Pin, ivec2(cmd[idx].B[1],cmd[idx].B[3])))) return 0u;\n"
"  Quad[0] = vec2(cmd[idx].CMDXA,cmd[idx].CMDYA);\n"
"  Quad[1] = vec2(cmd[idx].CMDXB,cmd[idx].CMDYB);\n"
"  Quad[2] = vec2(cmd[idx].CMDXC,cmd[idx].CMDYC);\n"
"  Quad[3] = vec2(cmd[idx].CMDXD,cmd[idx].CMDYD);\n"
"  Quad[4] = Quad[0];\n"
"  P = vec2(Pin)/upscale;\n"

"  if (cmd[idx].type < "Stringify(POLYLINE)") {\n"
"    if (wn_PnPoly(P, Quad) != 0u) return 1u;\n"
"  }"
"  if (cmd[idx].type != "Stringify(NORMAL)") {\n"
"    if (all(lessThanEqual(dist(P, Quad[0], Quad[1]), vec2(0.5, 0.5)))) {return 2u;}\n"
"    if (cmd[idx].type < "Stringify(LINE)") {\n"
"      if (all(lessThanEqual(dist(P, Quad[1], Quad[2]), vec2(0.5, 0.5)))) {return 3u;}\n"
"      if (all(lessThanEqual(dist(P, Quad[2], Quad[3]), vec2(0.5, 0.5)))) {return 4u;}\n"
"      if (all(lessThanEqual(dist(P, Quad[3], Quad[0]), vec2(0.5, 0.5)))) {return 5u;}\n"
"    }\n"
"  }\n"
"  return 0u;\n"
"}\n"

"int getCmd(ivec2 P, uint id, uint start, uint end, out uint zone)\n"
"{\n"
"  for(uint i=id+start; i<id+end; i++) {\n"
"   zone = pixIsInside(P, i);\n"
"   if (zone != 0u) {\n"
"     return int(i);\n"
"   }\n"
"  }\n"
"  return -1;\n"
"}\n"

"float cross( in vec2 a, in vec2 b ) { return a.x*b.y - a.y*b.x; }\n"

"vec2 bary(ivec2 p, vec2 a, vec2 b, vec2 c, vec2 auv, vec2 buv, vec2 cuv) {\n"
"  vec2 v0 = b - a;\n"
"  vec2 v1 = c - a;\n"
"  vec2 v2 = p - a;\n"
"  float den = v0.x * v1.y - v1.x * v0.y;\n"
"  float v = (v2.x * v1.y - v1.x * v2.y) / den;\n"
"  float w = (v0.x * v2.y - v2.x * v0.y) / den;\n"
"  float u = 1.0f - v - w;\n"
"  return (u*auv+v*buv+w*cuv);\n"
"}\n"

"vec2 getTexCoord(ivec2 texel, vec2 a, vec2 b, vec2 c, vec2 d) {\n"
"  if (all(lessThanEqual(abs(a-b),vec2(1.0)))) return bary(texel,a,c,d,vec2(0,0),vec2(1,1),vec2(0,1));\n"
"  if (all(lessThanEqual(abs(b-c),vec2(1.0)))) return bary(texel,a,c,d,vec2(0,0),vec2(1,1),vec2(0,1));\n"
"  if (all(lessThanEqual(abs(c-d),vec2(1.0)))) return bary(texel,a,b,c,vec2(0,0),vec2(1,0),vec2(0,1));\n"
"  if (all(lessThanEqual(abs(d-a),vec2(1.0)))) return bary(texel,a,b,c,vec2(0,0),vec2(1,0),vec2(0,1));\n"
"  vec2 p = vec2(texel)/upscale;\n"
"  vec2 e = b-a;\n"
"  vec2 f = d-a;\n"
"  vec2 h = p-a;\n"
"  vec2 g = a-b+c-d;\n"
"  if (e.x == 0.0) e.x = 1.0;\n"
"  float u = 0.0;\n"
"  float v = 0.0;\n"
"  float k1 = cross( e, f ) + cross( h, g );\n"
"  float k0 = cross( h, e );\n"
"  v = clamp(-k0/k1, 0.0, 1.0);\n"
"  u = clamp((h.x*k1+f.x*k0) / (e.x*k1-g.x*k0), 0.0, 1.0);\n"
"  return vec2( u, v );\n"
"}\n"

"vec2 getTexCoordDistorted(ivec2 texel, vec2 a, vec2 b, vec2 c, vec2 d) {\n"
//http://iquilezles.org/www/articles/ibilinear/ibilinear.htm
"  if (all(lessThanEqual(abs(a-b),vec2(1.0)))) return bary(texel,a,c,d,vec2(0,0),vec2(1,1),vec2(0,1));\n"
"  if (all(lessThanEqual(abs(b-c),vec2(1.0)))) return bary(texel,a,c,d,vec2(0,0),vec2(1,1),vec2(0,1));\n"
"  if (all(lessThanEqual(abs(c-d),vec2(1.0)))) return bary(texel,a,b,c,vec2(0,0),vec2(1,0),vec2(0,1));\n"
"  if (all(lessThanEqual(abs(d-a),vec2(1.0)))) return bary(texel,a,b,c,vec2(0,0),vec2(1,0),vec2(0,1));\n"
"  vec2 p = vec2(texel)/upscale;\n"
"  vec2 e = b-a;\n"
"  vec2 f = d-a;\n"
"  vec2 h = p-a;\n"
"  vec2 g = a-b+c-d;\n"
"  if (e.x == 0.0) e.x = 1.0;\n"
"  float u = 0.0;\n"
"  float v = 0.0;\n"
"  float k2 = cross( g, f );\n"
"  float k1 = cross( e, f ) + cross( h, g );\n"
"  float k0 = cross( h, e );\n"
"  if (abs(k2) >= 0.00001) {\n"
"    float w = k1*k1 - 4.0*k0*k2;\n"
"    if( w>=0.0 ) {\n"
"      w = sqrt( w );\n"
"      float ik2 = 0.5/k2;\n"
"      v = (-k1 - w)*ik2; \n"
"      if( v<0.0 || v>1.0 ) v = (-k1 + w)*ik2;\n"
"      u = (h.x - f.x*v)/(e.x + g.x*v);\n"
"      if( ((u<0.0) || u>(1.0)) || ((v<0.0) || v>(1.0)) ) return vec2(-1.0);\n"
"    }\n"
"  } else { \n"
"    v = clamp(-k0/k1, 0.0, 1.0);\n"
"    u = clamp((h.x*k1+f.x*k0) / (e.x*k1-g.x*k0), 0.0, 1.0);\n"
"  }\n"
"  return vec2( u, v );\n"
"}\n"

"vec2 getTexCoordPolygon(ivec2 texel, vec2 a, vec2 b, vec2 c, vec2 d) {\n"
//http://iquilezles.org/www/articles/ibilinear/ibilinear.htm
"  if (all(lessThanEqual(abs(a-b),vec2(1.0)))) return bary(texel,a,c,d,vec2(0,0),vec2(1,1),vec2(0,1));\n"
"  if (all(lessThanEqual(abs(b-c),vec2(1.0)))) return bary(texel,a,c,d,vec2(0,0),vec2(1,1),vec2(0,1));\n"
"  if (all(lessThanEqual(abs(c-d),vec2(1.0)))) return bary(texel,a,b,c,vec2(0,0),vec2(1,0),vec2(0,1));\n"
"  if (all(lessThanEqual(abs(d-a),vec2(1.0)))) return bary(texel,a,b,c,vec2(0,0),vec2(1,0),vec2(0,1));\n"
"  vec2 p = vec2(texel)/upscale;\n"
"  vec2 e = b-a;\n"
"  vec2 f = d-a;\n"
"  vec2 h = p-a;\n"
"  vec2 g = a-b+c-d;\n"
"  float u = 0.0;\n"
"  float v = 0.0;\n"
"  float k2 = cross( g, f );\n"
"  float k1 = cross( e, f ) + cross( h, g );\n"
"  float k0 = cross( h, e );\n"
"  if (abs(k2) >= 0.0001) {\n"
"    float w = k1*k1 - 4.0*k0*k2;\n"
"    if( w>=0.0 ) {\n"
"      w = sqrt( w );\n"
"      float ik2 = 0.5/k2;\n"
"      v = (-k1 - w)*ik2; \n"
"      if ( (v<0.0) || (v>1.0) ) v = (-k1 + w)*ik2;\n"
"      if (abs(e.x + g.x*v) < 0.0001) u = 1.0;\n"
"      u = (h.x - f.x*v)/(e.x + g.x*v);\n"
"      u = clamp(u, 0.0, 1.0);\n"
"      v=clamp(v, 0.0, 1.0);\n"
"    }\n"
"  } else { \n"
"    if (abs(k1) < 0.0001) v = 1.0;\n"
"    else v = clamp(-k0/k1, 0.0, 1.0);\n"
"    if (abs(e.x*k1-g.x*k0) < 0.0001) u = 1.0;\n"
"    else u = clamp((h.x*k1+f.x*k0) / (e.x*k1-g.x*k0), 0.0, 1.0);\n"
"  }\n"
"  return vec2( u, v );\n"
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

"vec4 ReadSpriteColor(cmdparameter_struct pixcmd, vec2 uv, ivec2 texel){\n"
"  vec4 color = vec4(0.0);\n"
"  uint endcnt = 0;\n"
"  uint x = uint(uv.x*pixcmd.w - 0.001);\n"
"  uint pos = (uint(pixcmd.h*uv.y - 0.001)*pixcmd.w+uint(uv.x*pixcmd.w - 0.001));\n"
"  uint charAddr = pixcmd.CMDSRCA * 8 + pos;\n"
"  uint dot;\n"
"  bool SPD = ((pixcmd.CMDPMOD & 0x40u) != 0);\n"
"  bool END = ((pixcmd.CMDPMOD & 0x80u) != 0);\n"
//Pour le end, n a besoin d'une barriere par ligne entre toute les commandes
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
"      if ((dot == 0x0Fu) && !END) color = VDP1COLOR(0x00);\n"
"      else if ((dot == 0) && !SPD) color = VDP1COLOR(0x00);\n"
"      else color = VDP1COLOR(dot | colorBank);\n"
"      break;\n"
"    }\n"
"    case 1:\n"
"    {\n"
      // 4 bpp LUT mode
"       uint temp;\n"
"       charAddr = pixcmd.CMDSRCA * 8 + pos/2;\n"
"       uint colorLut = pixcmd.CMDCOLR * 8;\n"
"       endcnt = 0;\n" //Ne sert pas mais potentiellement pb
"       dot = Vdp1RamReadByte(charAddr);\n"
"       if ((x & 0x1u) == 0u) dot = (dot>>4)&0xFu;\n"
"       else dot = (dot)&0xFu;\n"
"       if ((dot == 0x0Fu) && !END) color = VDP1COLOR(0x00);\n"
"       else if ((dot == 0) && !SPD) color = VDP1COLOR(0x00);\n"
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
"      if ((dot == 0xFFu) && !END) color = VDP1COLOR(0x00);\n"
"      else if ((dot == 0) && !SPD) color = VDP1COLOR(0x00);\n"
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
"      if ((dot == 0xFFu) && !END) color = VDP1COLOR(0x00);\n"
"      else if ((dot == 0) && !SPD) color = VDP1COLOR(0x00);\n"
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
"      if ((dot == 0xFFu) && !END) color = VDP1COLOR(0x00);\n"
"      else if ((dot == 0) && !SPD) color = VDP1COLOR(0x00);\n"
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
"      if ((temp == 0x7FFFu) && !END) color = VDP1COLOR(0x00);\n"
"      else if (((temp & 0x8000u) == 0) && !SPD) color = VDP1COLOR(0x00);\n"
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

#ifdef USE_VDP1_TEX
"vec4 ReadTexColor(cmdparameter_struct pixcmd, vec2 uv, ivec2 texel){\n"
"  int x = int(uv.x*pixcmd.w - 0.5) + int(pixcmd.texX);\n"
"  int y = int(uv.y*pixcmd.h - 0.5) + int(pixcmd.texY);\n"
"  ivec2 size = textureSize(texSurface,0);\n"
"  return texelFetch(texSurface,ivec2(x, y), 0);\n"
"}\n"
#endif

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
"  uint discarded = 0;\n"
"  vec2 texcoord = vec2(0);\n"
"  vec2 gouraudcoord = vec2(0);\n"
"  ivec2 size = imageSize(outSurface);\n"
"  ivec2 pos = ivec2(gl_GlobalInvocationID.xy);\n"
"  if (pos.x >= size.x || pos.y >= size.y ) return;\n"
"  ivec2 texel = ivec2((vec4(float(pos.x),float(pos.y), 1.0, 1.0) * inverse(rot)).xy);\n"
"  ivec2 index = ivec2((texel.x*"Stringify(NB_COARSE_RAST_X)")/size.x, (texel.y*"Stringify(NB_COARSE_RAST_Y)")/size.y);\n"
"  ivec2 syslimit = sysClip;\n"
"  ivec4 userlimit = usrClip;\n"
"  uint lindex = index.y*"Stringify(NB_COARSE_RAST_X)"+ index.x;\n"
"  uint cmdIndex = lindex * 2000u;\n"

"  if (nbCmd[lindex] == 0u) return;\n"
"  uint idCmd = 0;\n"
"  uint zone = 0;\n"
"  int cmdindex = 0;\n"
"  bool useGouraud = false;\n"
"  while ((cmdindex != -1) && (idCmd<nbCmd[lindex]) ) {\n"
"    newColor = vec4(0.0);\n"
"    outColor = vec4(0.0);\n"
"    cmdindex = getCmd(texel, cmdIndex, idCmd, nbCmd[lindex], zone);\n"
"    if (cmdindex == -1) continue;\n"
"    idCmd = cmdindex + 1 - cmdIndex;\n"
"    pixcmd = cmd[cmdindex];\n"
"    if (pixcmd.type == "Stringify(SYSTEM_CLIPPING)") {\n"
"      syslimit = ivec2(pixcmd.CMDXC,pixcmd.CMDYC);\n"
"      continue;\n"
"    }\n"
"    if (pixcmd.type == "Stringify(USER_CLIPPING)") {\n"
"      userlimit = ivec4(pixcmd.CMDXA,pixcmd.CMDYA,pixcmd.CMDXC,pixcmd.CMDYC);\n"
"      continue;\n"
"    }\n"
"    if (any(greaterThan(pos,syslimit*upscale))) continue;\n"
"    if (((pixcmd.CMDPMOD >> 9) & 0x3u) == 2u) {\n"
//Draw inside
"      if (any(lessThan(pos,userlimit.xy*upscale)) || any(greaterThan(texel,userlimit.zw*upscale))) continue;\n"
"    }\n"
"    if (((pixcmd.CMDPMOD >> 9) & 0x3u) == 3u) {\n"
//Draw outside
"      if (all(greaterThanEqual(texel,userlimit.xy*upscale)) && all(lessThanEqual(texel,userlimit.zw*upscale))) continue;\n"
"    }\n"
"    useGouraud = ((pixcmd.CMDPMOD & 0x4u) == 0x4u);\n"
"    if (pixcmd.type == "Stringify(POLYGON)") {\n"
"      texcoord = getTexCoordPolygon(texel, vec2(pixcmd.P[0],pixcmd.P[1])/2.0, vec2(pixcmd.P[2],pixcmd.P[3])/2.0, vec2(pixcmd.P[4],pixcmd.P[5])/2.0, vec2(pixcmd.P[6],pixcmd.P[7])/2.0);\n"
"      gouraudcoord = texcoord;\n"
"      if ((texcoord.x == -1.0) && (texcoord.y == -1.0)) continue;\n"
"      else {\n"
"        if ((pixcmd.flip & 0x1u) == 0x1u) texcoord.x = 1.0 - texcoord.x;\n" //invert horizontally
"        if ((pixcmd.flip & 0x2u) == 0x2u) texcoord.y = 1.0 - texcoord.y;\n" //invert vertically
"        newColor = extractPolygonColor(pixcmd);\n"
"      }\n"
"    } else if (pixcmd.type == "Stringify(POLYLINE)") {\n"
"      texcoord = getTexCoordPolygon(texel, vec2(pixcmd.P[0],pixcmd.P[1])/2.0, vec2(pixcmd.P[2],pixcmd.P[3])/2.0, vec2(pixcmd.P[4],pixcmd.P[5])/2.0, vec2(pixcmd.P[6],pixcmd.P[7])/2.0);\n"
"      gouraudcoord = texcoord;\n"
"      if ((texcoord.x == -1.0) && (texcoord.y == -1.0)) continue;\n"
"      else {\n"
"        if ((pixcmd.flip & 0x1u) == 0x1u) texcoord.x = 1.0 - texcoord.x;\n" //invert horizontally
"        if ((pixcmd.flip & 0x2u) == 0x2u) texcoord.y = 1.0 - texcoord.y;\n" //invert vertically
"        newColor = extractPolygonColor(pixcmd);\n"
"      }\n"
"    } else if (pixcmd.type == "Stringify(LINE)") {\n"
"      texcoord = getTexCoordPolygon(texel, vec2(pixcmd.P[0],pixcmd.P[1])/2.0, vec2(pixcmd.P[2],pixcmd.P[3])/2.0, vec2(pixcmd.P[4],pixcmd.P[5])/2.0, vec2(pixcmd.P[6],pixcmd.P[7])/2.0);\n"
"      gouraudcoord = texcoord;\n"
"      if ((texcoord.x == -1.0) && (texcoord.y == -1.0)) continue;\n"
"      else {\n"
"        if ((pixcmd.flip & 0x1u) == 0x1u) texcoord.x = 1.0 - texcoord.x;\n" //invert horizontally
"        if ((pixcmd.flip & 0x2u) == 0x2u) texcoord.y = 1.0 - texcoord.y;\n" //invert vertically
"        newColor = extractPolygonColor(pixcmd);\n"
"      }\n"
"    } else if (pixcmd.type == "Stringify(DISTORTED)") {\n"
"      texcoord = getTexCoordDistorted(texel, vec2(pixcmd.P[0],pixcmd.P[1])/2.0, vec2(pixcmd.P[2],pixcmd.P[3])/2.0, vec2(pixcmd.P[4],pixcmd.P[5])/2.0, vec2(pixcmd.P[6],pixcmd.P[7])/2.0);\n"
"      gouraudcoord = texcoord;\n"
"      if ((texcoord.x == -1.0) && (texcoord.y == -1.0)) continue;\n"
"      else {\n"
"        if ((pixcmd.flip & 0x1u) == 0x1u) texcoord.x = 1.0 - texcoord.x;\n" //invert horizontally
"        if ((pixcmd.flip & 0x2u) == 0x2u) texcoord.y = 1.0 - texcoord.y;\n" //invert vertically
#ifdef USE_VDP1_TEX
"        newColor = ReadTexColor(pixcmd, texcoord, texel);\n"
#else
"        newColor = ReadSpriteColor(pixcmd, texcoord, texel);\n"
#endif
"      }\n"
"    } else if (pixcmd.type == "Stringify(SCALED)") {\n"
"      texcoord = getTexCoord(texel, vec2(pixcmd.P[0],pixcmd.P[1])/2.0, vec2(pixcmd.P[2],pixcmd.P[3])/2.0, vec2(pixcmd.P[4],pixcmd.P[5])/2.0, vec2(pixcmd.P[6],pixcmd.P[7])/2.0);\n"
"      gouraudcoord = texcoord;\n"
"      if ((pixcmd.flip & 0x1u) == 0x1u) texcoord.x = 1.0 - texcoord.x;\n" //invert horizontally
"      if ((pixcmd.flip & 0x2u) == 0x2u) texcoord.y = 1.0 - texcoord.y;\n" //invert vertically
#ifdef USE_VDP1_TEX
"      newColor = ReadTexColor(pixcmd, texcoord, texel);\n"
#else
"        newColor = ReadSpriteColor(pixcmd, texcoord, texel);\n"
#endif
"    } else if (pixcmd.type == "Stringify(NORMAL)") {\n"
"      texcoord = vec2(float(texel.x-pixcmd.CMDXA+1)/float(pixcmd.CMDXB-pixcmd.CMDXA), float(texel.y-pixcmd.CMDYA+1)/float(pixcmd.CMDYD-pixcmd.CMDYA));\n"
"      gouraudcoord = texcoord;\n"
"      if ((pixcmd.flip & 0x1u) == 0x1u) texcoord.x = 1.0 - texcoord.x;\n" //invert horizontally
"      if ((pixcmd.flip & 0x2u) == 0x2u) texcoord.y = 1.0 - texcoord.y;\n" //invert vertically
#ifdef USE_VDP1_TEX
"      newColor = ReadTexColor(pixcmd, texcoord, texel);\n"
#else
"        newColor = ReadSpriteColor(pixcmd, texcoord, texel);\n"
#endif
"    }\n"
"    if (newColor == vec4(0.0)) continue;\n"
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
           //gouraud_half_trans_mode,
           GOURAUD_PROCESS(newColor)
           RECOLINDEX(newColor)
           HALF_TRANPARENT_MIX(newColor, finalColor)
"          outColor.rg = newColor.rg;\n"
"          }; break;\n"
"        case 7u: {\n"
           //gouraud_half_luminance_mode,
           GOURAUD_PROCESS(newColor)
           RECOLINDEX(newColor)
           HALF_LUMINANCE(newColor)
"          outColor.rg = newColor.rg;\n"
"          }; break;\n"
"        default:\n"
"          outColor = vec4(0.0);\n"
"          continue;\n"
"          break;\n"
"      }\n"
"    }\n";

static const char vdp1_continue_f[] =
"    finalColor.ba = tag;\n"
"    finalColor.rg = outColor.rg;\n";

static const char vdp1_end_f[] =
"  }\n"
"  if ((finalColor == vec4(0.0))) return;\n"
"    imageStore(outSurface,ivec2(int(pos.x), int(size.y - 1.0 - pos.y)),finalColor);\n"
"}\n";

static const char vdp1_standard_mesh_f[] =
//Normal mesh
"  if ((pixcmd.CMDPMOD & 0x100u)==0x100u){\n"//IS_MESH
"    if( (texel.y & 0x01) == 0 ){ \n"
"      if( (texel.x & 0x01) == 0 ){ \n"
"        newColor = vec4(0.0);\n"
"        continue;\n"
"      }\n"
"    }else{ \n"
"      if( (texel.x & 0x01) == 1 ){ \n"
"        newColor = vec4(0.0);\n"
"        continue;\n"
"      } \n"
"    } \n"
"  } else {\n"
"    tag = vec2(0.0);\n"
"  }\n";

static const char vdp1_improved_mesh_f[] =
//Improved mesh
"  if ((pixcmd.CMDPMOD & 0x100u)==0x100u){\n"//IS_MESH
"    tag = outColor.rg;\n"
"    outColor.rg = finalColor.rg;\n"
"  } else {\n"
"    tag = vec2(0.0);\n"
"  }\n";
#endif //VDP1_PROG_COMPUTE_H
