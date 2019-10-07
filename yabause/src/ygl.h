/*  Copyright 2005 Guillaume Duhamel
    Copyright 2005-2006 Theo Berkau

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
#ifndef  _YGL_H_
#define  _YGL_H_
#ifdef __cplusplus
extern "C" {
#endif

#if defined(HAVE_LIBGL) || defined(__ANDROID__) || defined(IOS)

#if defined(__LIBRETRO__)
    #if defined(__APPLE__)
        #include <OpenGL/gl3.h>
        #define GL_TEXTURE_FETCH_BARRIER_BIT      0x00000008
        #define GL_TEXTURE_UPDATE_BARRIER_BIT     0x00000100
        #define GL_FRAMEBUFFER_BARRIER_BIT        0x00000400
    #endif
    #include <glsym/glsym.h>
    #include <glsm/glsm.h>
#elif defined(__ANDROID__)
    #include <GLES3/gl3.h>
    #include <GLES3/gl3ext.h>
    #include <EGL/egl.h>


#define GL_GEOMETRY_SHADER                0x8DD9

#define GL_ARB_tessellation_shader

#ifdef GL_ARB_tessellation_shader
#define GL_PATCHES                        0x000E
#define GL_PATCH_VERTICES                 0x8E72
#define GL_PATCH_DEFAULT_INNER_LEVEL      0x8E73
#define GL_PATCH_DEFAULT_OUTER_LEVEL      0x8E74
#define GL_TESS_CONTROL_OUTPUT_VERTICES   0x8E75
#define GL_TESS_GEN_MODE                  0x8E76
#define GL_TESS_GEN_SPACING               0x8E77
#define GL_TESS_GEN_VERTEX_ORDER          0x8E78
#define GL_TESS_GEN_POINT_MODE            0x8E79
/* reuse GL_TRIANGLES */
/* reuse GL_QUADS */
#define GL_ISOLINES                       0x8E7A
/* reuse GL_EQUAL */
#define GL_FRACTIONAL_ODD                 0x8E7B
#define GL_FRACTIONAL_EVEN                0x8E7C
/* reuse GL_CCW */
/* reuse GL_CW */
#define GL_MAX_PATCH_VERTICES             0x8E7D
#define GL_MAX_TESS_GEN_LEVEL             0x8E7E
#define GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS 0x8E7F
#define GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS 0x8E80
#define GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS 0x8E81
#define GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS 0x8E82
#define GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS 0x8E83
#define GL_MAX_TESS_PATCH_COMPONENTS      0x8E84
#define GL_MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS 0x8E85
#define GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS 0x8E86
#define GL_MAX_TESS_CONTROL_UNIFORM_BLOCKS 0x8E89
#define GL_MAX_TESS_EVALUATION_UNIFORM_BLOCKS 0x8E8A
#define GL_MAX_TESS_CONTROL_INPUT_COMPONENTS 0x886C
#define GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS 0x886D
#define GL_MAX_COMBINED_TESS_CONTROL_UNIFORM_COMPONENTS 0x8E1E
#define GL_MAX_COMBINED_TESS_EVALUATION_UNIFORM_COMPONENTS 0x8E1F
#define GL_UNIFORM_BLOCK_REFERENCED_BY_TESS_CONTROL_SHADER 0x84F0
#define GL_UNIFORM_BLOCK_REFERENCED_BY_TESS_EVALUATION_SHADER 0x84F1
#define GL_TESS_EVALUATION_SHADER         0x8E87
#define GL_TESS_CONTROL_SHADER            0x8E88
#endif

typedef void(*PFNGLPATCHPARAMETERIPROC) (GLenum pname, GLint value);
extern PFNGLPATCHPARAMETERIPROC glPatchParameteri;

#define GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT 0x00000001
#define GL_ELEMENT_ARRAY_BARRIER_BIT      0x00000002
#define GL_UNIFORM_BARRIER_BIT            0x00000004
#define GL_TEXTURE_FETCH_BARRIER_BIT      0x00000008
#define GL_SHADER_IMAGE_ACCESS_BARRIER_BIT 0x00000020
#define GL_COMMAND_BARRIER_BIT            0x00000040
#define GL_PIXEL_BUFFER_BARRIER_BIT       0x00000080
#define GL_TEXTURE_UPDATE_BARRIER_BIT     0x00000100
#define GL_BUFFER_UPDATE_BARRIER_BIT      0x00000200
#define GL_FRAMEBUFFER_BARRIER_BIT        0x00000400
#define GL_TRANSFORM_FEEDBACK_BARRIER_BIT 0x00000800
#define GL_ATOMIC_COUNTER_BARRIER_BIT     0x00001000
#define GL_ALL_BARRIER_BITS               0xFFFFFFFF

typedef void (* PFNGLMEMORYBARRIERPROC) (GLbitfield barriers);
extern PFNGLMEMORYBARRIERPROC glMemoryBarrier;

#elif defined(_WIN32)

#include <windows.h>
  #if defined(_USEGLEW_)
    #include <GL/glew.h>
    #include <GL/gl.h>
    #include "glext.h"
#else
    #include <GL/gl.h>
    #include "glext.h"
    extern PFNGLACTIVETEXTUREPROC glActiveTexture;
#endif

#elif defined(IOS)
#include <OpenGLES/ES3/gl.h>
#include <OpenGLES/ES3/glext.h>

#define GL_GEOMETRY_SHADER                0x8DD9

#ifndef GL_ARB_tessellation_shader
#define GL_PATCHES                        0x000E
#define GL_PATCH_VERTICES                 0x8E72
#define GL_PATCH_DEFAULT_INNER_LEVEL      0x8E73
#define GL_PATCH_DEFAULT_OUTER_LEVEL      0x8E74
#define GL_TESS_CONTROL_OUTPUT_VERTICES   0x8E75
#define GL_TESS_GEN_MODE                  0x8E76
#define GL_TESS_GEN_SPACING               0x8E77
#define GL_TESS_GEN_VERTEX_ORDER          0x8E78
#define GL_TESS_GEN_POINT_MODE            0x8E79
/* reuse GL_TRIANGLES */
/* reuse GL_QUADS */
#define GL_ISOLINES                       0x8E7A
/* reuse GL_EQUAL */
#define GL_FRACTIONAL_ODD                 0x8E7B
#define GL_FRACTIONAL_EVEN                0x8E7C
/* reuse GL_CCW */
/* reuse GL_CW */
#define GL_MAX_PATCH_VERTICES             0x8E7D
#define GL_MAX_TESS_GEN_LEVEL             0x8E7E
#define GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS 0x8E7F
#define GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS 0x8E80
#define GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS 0x8E81
#define GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS 0x8E82
#define GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS 0x8E83
#define GL_MAX_TESS_PATCH_COMPONENTS      0x8E84
#define GL_MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS 0x8E85
#define GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS 0x8E86
#define GL_MAX_TESS_CONTROL_UNIFORM_BLOCKS 0x8E89
#define GL_MAX_TESS_EVALUATION_UNIFORM_BLOCKS 0x8E8A
#define GL_MAX_TESS_CONTROL_INPUT_COMPONENTS 0x886C
#define GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS 0x886D
#define GL_MAX_COMBINED_TESS_CONTROL_UNIFORM_COMPONENTS 0x8E1E
#define GL_MAX_COMBINED_TESS_EVALUATION_UNIFORM_COMPONENTS 0x8E1F
#define GL_UNIFORM_BLOCK_REFERENCED_BY_TESS_CONTROL_SHADER 0x84F0
#define GL_UNIFORM_BLOCK_REFERENCED_BY_TESS_EVALUATION_SHADER 0x84F1
#define GL_TESS_EVALUATION_SHADER         0x8E87
#define GL_TESS_CONTROL_SHADER            0x8E88
#endif

typedef void(*PFNGLPATCHPARAMETERIPROC) (GLenum pname, GLint value);
extern PFNGLPATCHPARAMETERIPROC glPatchParameteri;

#define GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT 0x00000001
#define GL_ELEMENT_ARRAY_BARRIER_BIT      0x00000002
#define GL_UNIFORM_BARRIER_BIT            0x00000004
#define GL_TEXTURE_FETCH_BARRIER_BIT      0x00000008
#define GL_SHADER_IMAGE_ACCESS_BARRIER_BIT 0x00000020
#define GL_COMMAND_BARRIER_BIT            0x00000040
#define GL_PIXEL_BUFFER_BARRIER_BIT       0x00000080
#define GL_TEXTURE_UPDATE_BARRIER_BIT     0x00000100
#define GL_BUFFER_UPDATE_BARRIER_BIT      0x00000200
#define GL_FRAMEBUFFER_BARRIER_BIT        0x00000400
#define GL_TRANSFORM_FEEDBACK_BARRIER_BIT 0x00000800
#define GL_ATOMIC_COUNTER_BARRIER_BIT     0x00001000
#define GL_ALL_BARRIER_BITS               0xFFFFFFFF

typedef void (* PFNGLMEMORYBARRIERPROC) (GLbitfield barriers);
extern PFNGLMEMORYBARRIERPROC glMemoryBarrier;

#elif  defined(__APPLE__)
    #include <OpenGL/gl.h>
    #include <OpenGL/gl3.h>

#else // Linux?
    #if defined(_OGLES3_)||defined(_OGL3_)
        #define GL_GLEXT_PROTOTYPES 1
        #define GLX_GLXEXT_PROTOTYPES 1
        #include <GL/glew.h>
        #include <GL/gl.h>
    #else
        #include <GL/gl.h>
    #endif
#endif

#include <stdarg.h>
#include <string.h>

#ifndef YGL_H
#define YGL_H

#include "core.h"
#include "threads.h"
#include "vidshared.h"
#include "vdp1.h"

//#define DEBUG_BLIT

#ifdef DEBUG_BLIT
#define NB_RENDER_LAYER 5
#else
#define NB_RENDER_LAYER 1
#endif

typedef struct {
	float vertices[8];
	int w;
	int h;
	int flip;
	int priority;
	int dst;
    int uclipmode;
    int blendmode;
    s32 cor;
    s32 cog;
    s32 cob;
    int linescreen;
    int idScreen;
    int idReg;
} YglSprite;

typedef struct {
	float x;
	float y;
} YglCache;

typedef struct {
	unsigned int * textdata;
	unsigned int w;
} YglTexture;

#define HASHSIZE  (0xFFFF)
typedef struct _YglCacheHash {
	u64 addr;
	float x;
	float y;
	struct _YglCacheHash * next;
} YglCacheHash;

typedef struct {
	unsigned int currentX;
	unsigned int currentY;
	unsigned int yMax;
	unsigned int * texture;
	unsigned int width;
	unsigned int height;
        YabMutex *mtx;
	YglCacheHash *HashTable[HASHSIZE];
	YglCacheHash CashLink[HASHSIZE * 2];
	u32 CashLink_index;
	GLuint textureID;
	GLuint pixelBufferID;
} YglTextureManager;

extern YglTextureManager * YglTM_vdp1[2];
extern YglTextureManager * YglTM_vdp2;

YglTextureManager * YglTMInit(unsigned int, unsigned int);
void YglTMDeInit(YglTextureManager * tm );
void YglTMReset( YglTextureManager * tm );
void YglTMReserve(YglTextureManager * tm, unsigned int w, unsigned int h);
void YglTMAllocate(YglTextureManager * tm, YglTexture *, unsigned int, unsigned int, unsigned int *, unsigned int *);
void YglTmPush(YglTextureManager * tm);
void YglTmPull(YglTextureManager * tm, u32 flg);
void YglTMCheck();

void YglCacheInit(YglTextureManager * tm);
void YglCacheDeInit(YglTextureManager * tm);
int YglIsCached(YglTextureManager * tm, u64, YglCache *);
void YglCacheAdd(YglTextureManager * tm, u64, YglCache *);
void YglCacheReset(YglTextureManager * tm);
void setupMaxSize();

void YglCheckFBSwitch(int sync);

#define VDP2_CC_NONE 0x00

enum
{
   //VDP1 Programs
   //From 0 to 2*3*2*7*2

   PG_VDP1_START = 1,
   PG_VDP1_VDP2 = (2*3*2*2*7)+1, //168

   PG_VDP1_STARTUSERCLIP=200,
   PG_VDP1_ENDUSERCLIP=201,


   PG_VDP2_NORMAL=203,
   PG_VDP2_MOSAIC=204,
   PG_VDP2_NORMAL_CRAM=205,
   PG_VDP2_MOSAIC_CRAM=206,



   PG_VDP2_DRAWFRAMEBUFF_NONE=256,
   PG_VDP2_DRAWFRAMEBUFF_LESS_NONE=(PG_VDP2_DRAWFRAMEBUFF_NONE+128),
   PG_VDP2_DRAWFRAMEBUFF_EUQAL_NONE=(PG_VDP2_DRAWFRAMEBUFF_NONE+128*2),
   PG_VDP2_DRAWFRAMEBUFF_MORE_NONE=(PG_VDP2_DRAWFRAMEBUFF_NONE+128*3),
   PG_VDP2_DRAWFRAMEBUFF_MSB_NONE=(PG_VDP2_DRAWFRAMEBUFF_NONE+128*4),

   PG_MAX = (PG_VDP2_DRAWFRAMEBUFF_NONE+128*5)
};



typedef struct {
    GLint  sprite;
    GLint  tessLevelInner;
    GLint  tessLevelOuter;
    GLint  fbo;
    GLint  texsize;
    GLuint mtxModelView;
    GLuint tex0;
    GLint vaid;
    GLint vertexp;
    GLint texcoordp;
} YglVdp1CommonParam;

#define TESS_COUNT (8)
void Ygl_Vdp1CommonGetUniformId(GLuint pgid, YglVdp1CommonParam * param);
int Ygl_uniformVdp1CommonParam(void * p, YglTextureManager *tm, Vdp2 *varVdp2Regs, int id);
int Ygl_cleanupVdp1CommonParam(void * p, YglTextureManager *tm);

// std140
typedef struct  {
 int u_pri[8*4];
 int u_alpha[8*4];
 float u_coloroffset[4];
 int u_cctll;
 int u_color_ram_offset;
} UniformFrameBuffer;

/*
int isSpecialColorCal;   // (fixVdp2Regs->CCCTL >> 6) & 0x01);
int SpecialColorCalMode; // (fixVdp2Regs->SPCTL >> 12) & 0x3;
*/

#define FRAME_BUFFER_UNIFORM_ID (5)

typedef struct {
   int prgid;
   GLuint prg;
   GLuint vertexBuffer;
   float * quads;
   float * textcoords;
   float * vertexAttribute;
   int currentQuad;
   int maxQuad;
   int vaid;
   char uClipMode;
   short ux1,uy1,ux2,uy2;
   int blendmode;
   int preblendmode;
   GLuint vertexp;
   GLuint texcoordp;
   GLuint mtxModelView;
   GLuint color_offset;
   GLuint tex0;
   GLuint tex1;
   float color_offset_val[4];
   int var1, var2, var3, var4, var5;
   int (*setupUniform)(void *, YglTextureManager *tm, Vdp2* regs, int id);
   int (*cleanupUniform)(void *, YglTextureManager *tm);
   YglVdp1CommonParam * ids;
   GLfloat* matrix;
   int mosaic[2];
   u32 lineTexture;
   int id;
   int colornumber;
   float emu_height;
   float vheight;
   float emu_width;
   float vwidth;
   GLuint interuput_texture;
} YglProgram;

typedef struct {
   int prgcount;
   int prgcurrent;
   int uclipcurrent;
   short ux1,uy1,ux2,uy2;
   int blendmode;
   YglProgram * prg;
} YglLevel;

typedef struct
{
    GLfloat   m[4][4];
} YglMatrix;

typedef enum
{
  AA_NONE = 0,
  AA_BILINEAR_FILTER,
  AA_BICUBIC_FILTER
} AAMODE;

typedef enum
{
  UP_NONE = 0,
  UP_HQ4X,
  UP_4XBRZ,
  UP_2XBRZ,
} UPMODE;

typedef enum
{
    PERSPECTIVE_CORRECTION = 0,
    CPU_TESSERATION,
    GPU_TESSERATION
} POLYGONMODE;

typedef enum
{
    COMPUTE_RBG_OFF = 0,
    COMPUTE_RBG_ON,
} COMPUTESHADERMODE;

typedef enum
{
    ORIGINAL_MESH = 0,
    IMPROVED_MESH
} MESHMODE;

typedef enum
{
    RES_ORIGINAL = 1,
    RES_480p = 2,
    RES_720p = 4,
    RES_1080p = 8,
    RES_NATIVE = 16,
} RESOLUTION_MODE;

typedef enum
{
	ORIGINAL_RATIO = 0,
	STREETCH_RATIO
} RATIOMODE;


typedef enum {
    VDP_SETTING_FILTERMODE = 0,
    VDP_SETTING_POLYGON_MODE,
    VDP_SETTING_RESOLUTION_MODE,
    VDP_SETTING_UPSCALMODE,
    VDP_SETTING_ASPECT_RATIO,
    VDP_SETTING_SCANLINE,
    VDP_SETTING_MESH_MODE,
    VDP_SETTING_COMPUTE_SHADER
} enSettings;


typedef enum {
	NBG0 = 0,
	NBG1,
	NBG2,
	NBG3,
	RBG0,
	RBG1,
	SPRITE,
	enBGMAX
} enBG;

typedef enum {
  S0CCRT = 0, //Alpha
  S1CCRT,
  S2CCRT,
  S3CCRT,
  S4CCRT,
  S5CCRT,
  S6CCRT,
  S7CCRT,
  S0PRI, //Prio
  S1PRI,
  S2PRI,
  S3PRI,
  S4PRI,
  S5PRI,
  S6PRI,
  S7PRI,
  SPCC, //CC condition
  VDP1COR, //Color offset
  VDP1COG,
  VDP1COB,
  VDP1CORS, //Color offset sign
  VDP1COGS,
  VDP1COBS,
  CRAOFB, //COLOR RAM OFFSET
	NB_VDP2_REG
} enVdp2;


typedef enum {
	NONE = 1,
	AS_IS = 2,
	SRC_ALPHA = 3,
	DST_ALPHA = 4
} SpriteMode;

typedef struct {
	u32 lincolor_tex;
	u32 linecolor_pbo;
	u32 * lincolor_buf;
  u32 depth;
} YglPerLineInfo;

typedef struct {
   //GLuint texture;
   //GLuint pixelBufferID;
   int st;
   int originx;
   int originy;
   unsigned int width;
   unsigned int height;
   unsigned int depth;

   float clear[4];

   // VDP1 Info
   int vdp1_maxpri;
   int vdp1_minpri;
   u32 vdp1_lineTexture;

   // VDP1 Framebuffer
   int rwidth;
   int rheight;
   float widthRatio;
   float heightRatio;
   int density;
   int drawframe;
   int readframe;
   int vdp1On[2];
   GLuint rboid_depth;
   GLuint rboid_depth_win;
   GLuint vdp1fbo;
   GLuint vdp1fbowin;
   GLuint vdp1FrameBuff[3];
   GLuint* vdp1Tex; //Texture for VDP1 CS
   GLuint smallfbo;
   GLuint smallfbotex;
   GLuint vdp1pixelBufferID;
   void * pFrameBuffer;
   GLuint vdp1AccessFB;
   GLuint vdp1AccessTex;
   GLuint vdp1_pbo;
   GLuint vdp1IsNotEmpty;
   u32* vdp1fb_buf;
   u32* vdp1fb_buf_read;
   GLuint original_fbo;
   GLuint original_fbotex[NB_RENDER_LAYER];

   GLuint compute_tex;

   GLuint back_fbo;
   GLuint back_fbotex[2];

   GLuint screen_fbo;
   GLuint screen_fbotex[SPRITE];

   GLuint rbg_compute_fbo;
   GLuint rbg_compute_fbotex[2];

   GLuint window_fbo;
   GLuint window_fbotex[enBGMAX];

   GLuint window_cc_fbo;
   GLuint window_cc_fbotex;

   GLuint tmpfbo;
   GLuint tmpfbotex;

   GLuint upfbo;
   GLuint upfbotex;

   // Message Layer
   int msgwidth;
   int msgheight;
   GLuint msgtexture;
   u32 * messagebuf;

   u32* win[2];
   u32 window_tex[2];

   int Win0[enBGMAX+1];
   int Win0_mode[enBGMAX+1];
   int Win1[enBGMAX+1];
   int Win1_mode[enBGMAX+1];
   int WinS[enBGMAX+1];
   int WinS_mode[enBGMAX+1];
   int Win_op[enBGMAX+1];

   YglMatrix mtxModelView;
   YglMatrix rbgModelView;

   YglProgram windowpg;

   YglLevel * vdp1levels;
   YglLevel * vdp2levels;

   // Thread
   YabMutex * mutex;

   u32 lincolor_tex;
   u32 linecolor_pbo;
   u32 * lincolor_buf;
   int perLine[enBGMAX];

   u32 vdp2reg_tex;
   u32 vdp2reg_pbo;
   u8* vdp2reg_buf;
   u8 * vdp2buf;

   u32 back_tex;
   u32 back_pbo;
   u32 * backcolor_buf;

   AAMODE aamode;
   UPMODE upmode;
   POLYGONMODE polygonmode;
   MESHMODE meshmode;
   int scanline;
   RATIOMODE stretch;
   RESOLUTION_MODE resolution_mode;
   COMPUTESHADERMODE use_cs;
   GLsync sync;
   GLsync syncVdp1[2];
   GLuint default_fbo;
   YglPerLineInfo bg[enBGMAX];
   int vpd1_running;
   int needVdp1Render;
   GLint m_viewport[4];
   int min_fb_x;
   int max_fb_x;
   int min_fb_y;
   int max_fb_y;

   GLuint cram_tex;
   GLuint cram_tex_pbo;
   u32 * cram_tex_buf;
   u32 colupd_min_addr;
   u32 colupd_max_addr;
   YabMutex * crammutex;

   int msb_shadow_count_[2];
   GLuint vao;
   GLuint vertices_buf;
   GLuint texcord_buf;
   GLuint win0v_buf;
   GLuint win1v_buf;
   GLuint vertexPosition_buf;
   GLuint textureCoordFlip_buf;
   GLuint textureCoord_buf;
   GLuint vb_buf;
   GLuint tb_buf;

   GLuint quads_buf;
   GLuint textcoords_buf;
   GLuint vertexAttribute_buf;

   int vdp1_stencil_mode;

   int rbg_use_compute_shader;
   int vdp2_use_compute_shader;

   float vdp1wratio;
   float vdp1hratio;

} Ygl;

extern Ygl * _Ygl;
extern int opengl_mode; // 0 => gles3 , 1 => gl3.3

// Rotate Screen

typedef struct {
  int useb;
  vdp2draw_struct info;
  YglTexture texture;
  int rgb_type;
  int pagesize;
  int patternshift;
  u32 LineColorRamAdress;
  vdp2draw_struct line_info;
  YglTexture line_texture;
  YglCache c;
  YglCache cline;
  int vres;
  int hres;
  int async;
  volatile int vdp2_sync_flg;
  vdp2rotationparameter_struct  paraA;
  vdp2rotationparameter_struct  paraB;
  Vdp2 *varVdp2Regs;
  int use_cs;
} RBGDrawInfo;

int YglInit(int, int, unsigned int);
void YglDeInit(void);
float * YglQuad(vdp2draw_struct *, YglTexture *, YglCache * c, YglTextureManager *tm);
int YglQuadRbg0(RBGDrawInfo * rbg, YglTexture * output, YglCache * c, YglCache * line, int rbg_type, YglTextureManager *tm, Vdp2 *varVdp2Regs);
void YglQuadOffset(vdp2draw_struct * input, YglTexture * output, YglCache * c, int cx, int cy, float sx, float sy, YglTextureManager *tm);
void YglCachedQuadOffset(vdp2draw_struct * input, YglCache * cache, int cx, int cy, float sx, float sy, YglTextureManager *tm);
void YglCachedQuad(vdp2draw_struct *, YglCache *, YglTextureManager *tm);
void YglRender(Vdp2 *varVdp2Regs);
void YglReset(YglLevel level);
void YglShowTexture(void);
void YglChangeResolution(int, int);
void YglSetDensity(int d);
void YglCacheQuadGrowShading(YglSprite * input, float * colors, YglCache * cache, YglTextureManager *tm);
int YglQuadGrowShading(YglSprite * input, YglTexture * output, float * colors,YglCache * c, YglTextureManager *tm);
void YglSetClearColor(float r, float g, float b);
void YglStartWindow( vdp2draw_struct * info, int win0, int logwin0, int win1, int logwin1, int mode );
void YglEndWindow( vdp2draw_struct * info );

int YglVDP1AllocateTexture(vdp1cmd_struct * input, YglTexture * output, YglTextureManager *tm);

void YglOnUpdateColorRamWord(u32 addr);
void YglUpdateColorRam();
int YglInitShader(int id, const GLchar * vertex[], int vcount, const GLchar * frag[], int fcount, const GLchar * tc[], const GLchar * te[], const GLchar * g[] );

int YglTriangleGrowShading(YglSprite * input, YglTexture * output, float * colors, YglCache * c, YglTextureManager *tm);
void YglCacheTriangleGrowShading(YglSprite * input, float * colors, YglCache * cache, YglTextureManager *tm);

u32 * YglGetPerlineBuf(YglPerLineInfo * perline, int linecount,int depth );
void YglSetPerlineBuf(YglPerLineInfo * perline, u32 * pbuf, int linecount, int depth);

// 0.. no belnd, 1.. Alpha, 2.. Add
int YglSetLevelBlendmode( int pri, int mode );

extern int YglBlitSimple(int texture, int blend);
extern int YglBlitTexture(YglPerLineInfo *bg, int* prioscreens, int* modescreens, int* isRGB, int * isBlur, int* isShadow, int* lncl, GLuint* vdp1fb, int* win_s, int* win_s_mode, int* Win0, int* Win0_mode, int* Win1, int* Win1_mode, int* Win_op,  Vdp2 *varVdp2Regs);
extern SpriteMode getSpriteRenderMode(Vdp2* varVdp2Regs);
extern void executeTMVDP1(int in, int out);

extern u8 * YglGetVDP2RegPointer();

int Ygl_uniformVDP2DrawFramebuffer(float * offsetcol, SpriteMode mode, Vdp2* varVdp2Regs);

void YglScalef(YglMatrix *result, GLfloat sx, GLfloat sy, GLfloat sz);
void YglTranslatef(YglMatrix *result, GLfloat tx, GLfloat ty, GLfloat tz);
void YglRotatef(YglMatrix *result, GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void YglFrustum(YglMatrix *result, float left, float right, float bottom, float top, float nearZ, float farZ);
void YglPerspective(YglMatrix *result, float fovy, float aspect, float nearZ, float farZ);
void YglOrtho(YglMatrix *result, float left, float right, float bottom, float top, float nearZ, float farZ);
void YglLoadIdentity(YglMatrix *result);
void YglMatrixMultiply(YglMatrix *result, YglMatrix *srcA, YglMatrix *srcB);

int YglInitVertexBuffer( int initsize );
void YglDeleteVertexBuffer();
int YglUnMapVertexBuffer();
int YglMapVertexBuffer();
int YglUserDirectVertexBuffer();
int YglUserVertexBuffer();
int YglGetVertexBuffer( int size, void ** vpos, void **tcpos, void **vapos );
int YglExpandVertexBuffer( int addsize, void ** vpos, void **tcpos, void **vapos );
intptr_t YglGetOffset( void* address );
int YglBlitVDP1(u32 srcTexture, float w, float h, int flip);
int YglBlitFramebuffer(u32 srcTexture, float w, float h, float dispw, float disph);
int YglUpscaleFramebuffer(u32 srcTexture, u32 targetFbo, float w, float h, float texw, float texh);

void YglRenderVDP1(void);
u32 * YglGetLineColorPointer();
void YglSetLineColor(u32 * pbuf, int size);

u32* YglGetBackColorPointer();
void YglSetBackColor(int size);

void YglGetWindowPointer(int id);
void YglSetWindow(int id);

int Ygl_uniformWindow(void * p, YglTextureManager *tm, Vdp2 *varVdp2Regs, int id);
int YglProgramInit();
int YglProgramChange( YglLevel * level, int prgid );
int Ygl_cleanupNormal(void * p, YglTextureManager *tm);

int YglGenerateOriginalBuffer();
int YglGenerateComputeBuffer();

int YglSetupWindow(YglProgram * prg);
int Vdp2GenerateWindowInfo(Vdp2 *varVdp2Regs);

void YglEraseWriteVDP1();
void YglFrameChangeVDP1();

void YglEraseWriteCSVDP1();
void YglFrameChangeCSVDP1();

extern void RBGGenerator_init(int width, int height);
extern void RBGGenerator_resize(int width, int height);
extern void RBGGenerator_update(RBGDrawInfo * rbg, Vdp2 *varVdp2Regs );
extern GLuint RBGGenerator_getTexture( int id );
extern void RBGGenerator_onFinish();

extern void VDP2Generator_init(int width, int height);
extern void VDP2Generator_update(int tex, YglPerLineInfo *bg, int* prioscreens, int* modescreens, int* isRGB, int * isBlur, int* isShadow, int* lncl, GLuint* vdp1fb, Vdp2 *varVdp2Regs);
extern void VDP2Generator_resize(int width, int height);


// Keep a way to switch to gles shaders for embedded devices
#if defined(_OGLES3_)
#define SHADER_VERSION "#version 310 es \n"
#define SHADER_VERSION_TESS "#version 310 es \n#extension GL_ANDROID_extension_pack_es31a : enable \n"
#define SHADER_VERSION_COMPUTE "#version 310 es \n"

#else
#define SHADER_VERSION "#version 330 core \n"
#define SHADER_VERSION_TESS "#version 420 core \n"
#define SHADER_VERSION_COMPUTE "#version 430 core \n"
#endif

#if !defined(__APPLE__) && !defined(__ANDROID__) && !defined(_USEGLEW_) && !defined(_OGLES3_) && !defined(__LIBRETRO__)

extern GLuint (STDCALL *glCreateProgram)(void);
extern GLuint (STDCALL *glCreateShader)(GLenum);
extern void (STDCALL *glShaderSource)(GLuint,GLsizei,const GLchar **,const GLint *);
extern void (STDCALL *glCompileShader)(GLuint);
extern void (STDCALL *glAttachShader)(GLuint,GLuint);
extern void (STDCALL *glLinkProgram)(GLuint);
extern void (STDCALL *glUseProgram)(GLuint);
extern GLint (STDCALL *glGetUniformLocation)(GLuint,const GLchar *);
extern void (STDCALL *glUniform1i)(GLint,GLint);
extern void (STDCALL *glGetShaderInfoLog)(GLuint,GLsizei,GLsizei *,GLchar *);
extern void (STDCALL *glVertexAttribPointer)(GLuint index,GLint size, GLenum type, GLboolean normalized, GLsizei stride,const void *pointer);
extern void (STDCALL *glBindAttribLocation)( GLuint program, GLuint index, const GLchar * name);
extern void (STDCALL *glGetProgramiv)( GLuint    program, GLenum pname, GLint * params);
extern void (STDCALL *glGetShaderiv)(GLuint shader,GLenum pname,GLint *    params);
extern GLint (STDCALL *glGetAttribLocation)(GLuint program,const GLchar *    name);

extern void (STDCALL *glEnableVertexAttribArray)(GLuint index);
extern void (STDCALL *glDisableVertexAttribArray)(GLuint index);


//GL_ARB_framebuffer_object
extern PFNGLISRENDERBUFFERPROC glIsRenderbuffer;
extern PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer;
extern PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers;
extern PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers;
extern PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage;
extern PFNGLGETRENDERBUFFERPARAMETERIVPROC glGetRenderbufferParameteriv;
extern PFNGLISFRAMEBUFFERPROC glIsFramebuffer;
extern PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
extern PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
extern PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
extern PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;
extern PFNGLFRAMEBUFFERTEXTURE1DPROC glFramebufferTexture1D;
extern PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
extern PFNGLFRAMEBUFFERTEXTURE3DPROC glFramebufferTexture3D;
extern PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer;
extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC glGetFramebufferAttachmentParameteriv;
extern PFNGLGENERATEMIPMAPPROC glGenerateMipmap;
extern PFNGLBLITFRAMEBUFFERPROC glBlitFramebuffer;
extern PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC glRenderbufferStorageMultisample;
extern PFNGLFRAMEBUFFERTEXTURELAYERPROC glFramebufferTextureLayer;

extern PFNGLUNIFORM4FPROC glUniform4f;
extern PFNGLUNIFORM1FPROC glUniform1f;
extern PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;



#endif // !defined(__APPLE__) && !defined(__ANDROID__) && !defined(_USEGLEW_)

/*
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|S|C|A|A|A|P|P|P|s| | | | | | | |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
S show flag
C index or direct color
A alpha index
P priority
s Shadow Flag

*/

extern int VDP1_MASK;

INLINE u32 VDP1COLOR(u16 CMDPMOD, u16 CMDCOLR) {
  return CMDCOLR & VDP1_MASK;
}

INLINE u32 VDP1COLORFB(u16 CMDCOLR) {
  return (0xFF<<16)|(CMDCOLR & VDP1_MASK);
}


INLINE u32 VDP2COLOR(int id, u32 alpha, u32 priority, u32 cc_on, u32 cramindex) {
  return (((alpha & 0xF8) | priority) << 24 | ((cc_on & 0x1)<<16) | (cramindex& 0xFEFFFF));
  //In 32 bit rgb mode, the Blue LSB is always considered as 0.
  //This small artifact allows to implement Special color calculation without the need of an extra texture.
}

#define RGB555_TO_RGB24(temp)  ((temp & 0x1F) << 3 | (temp & 0x3E0) << 6 | (temp & 0x7C00) << 9)

#if defined WORDS_BIGENDIAN
#define SAT2YAB1(alpha,temp)      (alpha | (temp & 0x7C00) << 1 | (temp & 0x3E0) << 14 | (temp & 0x1F) << 27)
#else
#define SAT2YAB1(alpha,temp)      (alpha << 24 | (temp & 0x1F) << 3 | (temp & 0x3E0) << 6 | (temp & 0x7C00) << 9)
#endif

#if defined WORDS_BIGENDIAN
#define SAT2YAB2(alpha,dot1,dot2)       ((dot2 & 0xFF << 24) | ((dot2 & 0xFF00) << 8) | ((dot1 & 0xFF) << 8) | alpha)
#else
//Here FE is not accurate to get an extra bit for MSB shadow information...
#define SAT2YAB2(alpha,dot1,dot2)       (alpha << 24 | ((dot1 & 0xFE) << 16) | (dot2 & 0xFF00) | (dot2 & 0xFF))
#endif

#endif // YGL_H

#endif // defined(HAVE_LIBGL) || defined(__ANDROID__)

#ifdef __cplusplus
}
#endif

#endif // _YGL_H_
