/*  Copyright 2003-2006 Guillaume Duhamel
    Copyright 2004 Lawrence Sebald
    Copyright 2004-2007 Theo Berkau

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

/*! \file vidogl.c
    \brief OpenGL video renderer
*/
#if defined(HAVE_LIBGL) || defined(__ANDROID__) || defined(IOS)

#include <math.h>
#define EPSILON (1e-10 )


#include "vidogl.h"
#include "vidshared.h"
#include "debug.h"
#include "vdp2.h"
#include "yabause.h"
#include "ygl.h"
#include "yui.h"
#include "frameprofile.h"
#include "vdp1/vdp1_compute.h"
#ifdef SPRITE_CACHE
#include "patternManager.h"
#endif

#define Y_MAX(a, b) ((a) > (b) ? (a) : (b))
#define Y_MIN(a, b) ((a) < (b) ? (a) : (b))

#define CLAMP(A,LOW,HIGH) ((A)<(LOW)?(LOW):((A)>(HIGH))?(HIGH):(A))

#define LOG_AREA

static int vidogl_renderer_started = 0;
static Vdp2 baseVdp2Regs;
//#define PERFRAME_LOG
#ifdef PERFRAME_LOG
int fount = 0;
FILE *ppfp = NULL;
#endif

#ifdef _WINDOWS
int yprintf(const char * fmt, ...)
{
  static FILE * dbugfp = NULL;
  if (dbugfp == NULL) {
    dbugfp = fopen("debug.txt", "w");
  }
  if (dbugfp) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(dbugfp, fmt, ap);
    va_end(ap);
    fflush(dbugfp);
  }
  return 0;
}


void OSDPushMessageDirect(char * msg) {
}

#endif

#define YGL_THREAD_DEBUG
//#define YGL_THREAD_DEBUG yprintf




#define COLOR_ADDt(b)      (b>0xFF?0xFF:(b<0?0:b))
#define COLOR_ADDb(b1,b2)   COLOR_ADDt((signed) (b1) + (b2))
#ifdef WORDS_BIGENDIAN
#define COLOR_ADD(l,r,g,b)   (COLOR_ADDb((l >> 24) & 0xFF, r) << 24) | \
            (COLOR_ADDb((l >> 16) & 0xFF, g) << 16) | \
            (COLOR_ADDb((l >> 8) & 0xFF, b) << 8) | \
            (l & 0xFF)
#else
#define COLOR_ADD(l,r,g,b)   COLOR_ADDb((l & 0xFF), r) | \
            (COLOR_ADDb((l >> 8 ) & 0xFF, g) << 8) | \
            (COLOR_ADDb((l >> 16 ) & 0xFF, b) << 16) | \
            (l & 0xFF000000)
#endif

#define WA_INSIDE (0)
#define WA_OUTSIDE (1)

int VIDOGLInit(void);
void VIDOGLDeInit(void);
void VIDOGLResize(int, int, unsigned int, unsigned int, int);
int VIDOGLIsFullscreen(void);
int VIDOGLVdp1Reset(void);
void VIDOGLVdp1Draw();
void VIDOGLVdp1NormalSpriteDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer);
void VIDOGLVdp1ScaledSpriteDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer);
void VIDOGLVdp1DistortedSpriteDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer);
void VIDOGLVdp1PolygonDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer);
void VIDOGLVdp1PolylineDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer);
void VIDOGLVdp1LineDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer);
void VIDOGLVdp1UserClipping(u8 * ram, Vdp1 * regs);
void VIDOGLVdp1SystemClipping(u8 * ram, Vdp1 * regs);
void VIDOGLVdp1LocalCoordinate(u8 * ram, Vdp1 * regs);
int VIDOGLVdp2Reset(void);
void VIDOGLVdp2Draw(void);
static void VIDOGLVdp2DrawScreens(void);
void VIDOGLVdp2SetResolution(u16 TVMD);
void YglGetGlSize(int *width, int *height);
void VIDOGLGetNativeResolution(int *width, int *height, int*interlace);
void VIDOGLVdp1ReadFrameBuffer(u32 type, u32 addr, void * out);
void VIDOGLVdp1WriteFrameBuffer(u32 type, u32 addr, u32 val);
void VIDOGLSetSettingValueMode(int type, int value);
void VIDOGLSync();
void VIDOGLGetNativeResolution(int *width, int *height, int*interlace);
void VIDOGLVdp2DispOff(void);
void waitVdp2DrawScreensEnd(int sync, int abort);

VideoInterface_struct VIDOGL = {
VIDCORE_OGL,
"OpenGL Video Interface",
VIDOGLInit,
VIDOGLDeInit,
VIDOGLResize,
VIDOGLIsFullscreen,
VIDOGLVdp1Reset,
VIDOGLVdp1Draw,
VIDOGLVdp1NormalSpriteDraw,
VIDOGLVdp1ScaledSpriteDraw,
VIDOGLVdp1DistortedSpriteDraw,
VIDOGLVdp1PolygonDraw,
VIDOGLVdp1PolylineDraw,
VIDOGLVdp1LineDraw,
VIDOGLVdp1UserClipping,
VIDOGLVdp1SystemClipping,
VIDOGLVdp1LocalCoordinate,
VIDOGLVdp1ReadFrameBuffer,
VIDOGLVdp1WriteFrameBuffer,
YglEraseWriteVDP1,
YglFrameChangeVDP1,
VIDOGLVdp2Reset,
VIDOGLVdp2Draw,
YglGetGlSize,
VIDOGLSetSettingValueMode,
VIDOGLSync,
VIDOGLGetNativeResolution,
VIDOGLVdp2DispOff,
YglRender
};

static int vdp1_interlace = 0;

int GlWidth = 320;
int GlHeight = 224;

int vdp1cor = 0;
int vdp1cog = 0;
int vdp1cob = 0;

static int vdp2_interlace = 0;
static int nbg0priority = 0;
static int nbg1priority = 0;
static int nbg2priority = 0;
static int nbg3priority = 0;
static int rbg0priority = 0;

static int vdp2busy = 0;

vdp2rotationparameter_struct  Vdp1ParaA;

typedef struct {
  RBGDrawInfo *rbg;
  Vdp2 *varVdp2Regs;
} rotationTask;



typedef struct {
  vdp1cmd_struct *cmd;
  YglTexture *texture;
  Vdp2 *varVdp2Regs;
  int w,h;
} vdp1TextureTask;

#ifdef RGB_ASYNC
static YabEventQueue *rotq = NULL;
static YabEventQueue *rotq_end = NULL;
static YabEventQueue *rotq_end_task = NULL;
static int rotation_run = 0;
#endif

#ifdef VDP1_TEXTURE_ASYNC
YabEventQueue *vdp1q;
YabEventQueue *vdp1q_end;
static int vdp1text_run = 0;
#endif


#define LOG_ASYN

static void FASTCALL Vdp2DrawCell_in_sync(vdp2draw_struct *info, YglTexture *texture, Vdp2 *varVdp2Regs);

#define NB_MSG 128

#ifdef CELL_ASYNC

static void executeDrawCell();

YabEventQueue *cellq = NULL;
YabEventQueue *cellq_end = NULL;
static int drawcell_run = 0;

typedef struct {
  vdp2draw_struct *info;
  YglTexture *texture;
  Vdp2 *varVdp2Regs;
  int order;
} drawCellTask;

static int nbLoop = 0;
static int nbClear = 0;

YglTexture *textureTable[NB_MSG];
vdp2draw_struct *infoTable[NB_MSG];
Vdp2* vdp2RegsTable[NB_MSG];
int orderTable[NB_MSG];

#define CELL_SINGLE 0x1
#define CELL_QUAD   0x2

#define IS_MESH(a) ((a&0x100) == 0x100)
#define IS_SPD(a) ((a&0x40) == 0x40)
#define IS_MSB_SHADOW(a) ((a&0x8000)!=0)

static int getCCProgramId(int CMDPMOD) {
  int cctype = CMDPMOD & 0x7;
  int MSB = IS_MSB_SHADOW(CMDPMOD)?1:0;
  int Mesh = IS_MESH(CMDPMOD)?1:0;
  int SPD = IS_SPD(CMDPMOD)?1:0;
  int TESS = (_Ygl->polygonmode == GPU_TESSERATION)?1:0;
  if (cctype == 5) return -1;
  if (cctype > 5) cctype -=1;

  YGLLOG("Setup program %d %d %d %d %d\n", cctype, SPD, Mesh, MSB, TESS);

  return cctype+7*(SPD+2*(Mesh+3*(MSB+2*TESS)))+PG_VDP1_START;
}

void Vdp2DrawCell_in_async(void *p)
{
   while(drawcell_run != 0){
     drawCellTask *task = (drawCellTask *)YabWaitEventQueue(cellq);
     if (task != NULL) {
       if (task->order == CELL_SINGLE) {
         Vdp2DrawCell_in_sync(task->info, task->texture, task->varVdp2Regs);
       } else {
         Vdp2DrawCell_in_sync(task->info, task->texture, task->varVdp2Regs);
         task->texture->textdata -= (task->texture->w + 8) * 8 - 8;
         Vdp2DrawCell_in_sync(task->info, task->texture, task->varVdp2Regs);
         task->texture->textdata -= 8;
         Vdp2DrawCell_in_sync(task->info, task->texture, task->varVdp2Regs);
         task->texture->textdata -= (task->texture->w + 8) * 8 - 8;
         Vdp2DrawCell_in_sync(task->info, task->texture, task->varVdp2Regs);
       }
       free(task->texture);
       free(task->info);
       free(task->varVdp2Regs);
       free(task);
     }
     YabWaitEventQueue(cellq_end);
   }
}

static void FASTCALL Vdp2DrawCell(vdp2draw_struct *info, YglTexture *texture, Vdp2 *varVdp2Regs, int order) {
   drawCellTask *task = malloc(sizeof(drawCellTask));

   task->texture = malloc(sizeof(YglTexture));
   memcpy(task->texture, texture, sizeof(YglTexture));

   task->info = malloc(sizeof(vdp2draw_struct));
   memcpy(task->info, info, sizeof(vdp2draw_struct));

   task->varVdp2Regs = malloc(sizeof(Vdp2));
   memcpy(task->varVdp2Regs, varVdp2Regs, sizeof(Vdp2));

   task->order = order;


   if (drawcell_run == 0) {
     drawcell_run = 1;
     cellq = YabThreadCreateQueue(NB_MSG);
     cellq_end = YabThreadCreateQueue(NB_MSG);
     YabThreadStart(YAB_THREAD_VDP2_NBG0, Vdp2DrawCell_in_async, 0);
   }
   YabAddEventQueue(cellq_end, NULL);
   YabAddEventQueue(cellq, task);
   YabThreadYield();
}

static void requestDrawCellOrder(vdp2draw_struct * info, YglTexture *texture, Vdp2* varVdp2Regs, int order) {
  textureTable[nbLoop] = malloc(sizeof(YglTexture));
  memcpy(textureTable[nbLoop], texture, sizeof(YglTexture));
  infoTable[nbLoop] = malloc(sizeof(vdp2draw_struct));
  memcpy(infoTable[nbLoop], info, sizeof(vdp2draw_struct));
  vdp2RegsTable[nbLoop] = malloc(sizeof(Vdp2));
  memcpy(vdp2RegsTable[nbLoop], varVdp2Regs, sizeof(Vdp2));
  orderTable[nbLoop] = order;
  nbLoop++;
  if (nbLoop >= NB_MSG) {
    LOG_ASYN("Too much drawCell request!\n");
    executeDrawCell();
  }
}
#endif

static void executeDrawCell() {
#ifdef CELL_ASYNC
  int i = 0;
  while (i < nbLoop) {
    Vdp2DrawCell(infoTable[i], textureTable[i], vdp2RegsTable[i], orderTable[i]);
    free(infoTable[i]);
    free(textureTable[i]);
    free(vdp2RegsTable[i]);
    i++;
  }
  nbLoop = 0;
#endif
}

static void requestDrawCell(vdp2draw_struct * info, YglTexture *texture, Vdp2* varVdp2Regs) {
#ifdef CELL_ASYNC
   requestDrawCellOrder(info, texture, varVdp2Regs, CELL_SINGLE);
#else
         Vdp2DrawCell_in_sync(info, texture, varVdp2Regs);
#endif
}

static void requestDrawCellQuad(vdp2draw_struct * info, YglTexture *texture, Vdp2* varVdp2Regs) {
#ifdef CELL_ASYNC
   requestDrawCellOrder(info, texture, varVdp2Regs, CELL_QUAD);
#else
   Vdp2DrawCell_in_sync(info, texture, varVdp2Regs);
   texture->textdata -= (texture->w + 8) * 8 - 8;
   Vdp2DrawCell_in_sync(info, texture, varVdp2Regs);
   texture->textdata -= 8;
   Vdp2DrawCell_in_sync(info, texture, varVdp2Regs);
   texture->textdata -= (texture->w + 8) * 8 - 8;
   Vdp2DrawCell_in_sync(info, texture, varVdp2Regs);
#endif
}



static void Vdp2DrawRBG0(Vdp2* varVdp2Regs);

static u32 Vdp2ColorRamGetColor(u32 colorindex, int alpha);
static void Vdp2PatternAddrPos(vdp2draw_struct *info, int planex, int x, int planey, int y, Vdp2 *varVdp2Regs);
static void Vdp2DrawPatternPos(vdp2draw_struct *info, YglTexture *texture, int x, int y, int cx, int cy,  int lines, Vdp2 *varVdp2Regs);
static INLINE void ReadVdp2ColorOffset(Vdp2 * regs, vdp2draw_struct *info, int mask);
static INLINE u16 Vdp2ColorRamGetColorRaw(u32 colorindex);
static void FASTCALL Vdp2DrawRotation(RBGDrawInfo * rbg, Vdp2 *varVdp2Regs);
static void Vdp2DrawRotation_in(RBGDrawInfo * rbg, Vdp2 *varVdp2Regs);
static void Vdp2DrawRotation_in_sync(RBGDrawInfo * rbg, Vdp2 *varVdp2Regs);

static vdp2Lineinfo lineNBG0[512];
static vdp2Lineinfo lineNBG1[512];



vdp2rotationparameter_struct * FASTCALL vdp2rGetKValue2W(vdp2rotationparameter_struct * param, int index);
vdp2rotationparameter_struct * FASTCALL vdp2rGetKValue1W(vdp2rotationparameter_struct * param, int index);
vdp2rotationparameter_struct * FASTCALL vdp2rGetKValue2Wm3(vdp2rotationparameter_struct * param, int index);
vdp2rotationparameter_struct * FASTCALL vdp2rGetKValue1Wm3(vdp2rotationparameter_struct * param, int index);
vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode00NoK(RBGDrawInfo * rgb, int h, int v, Vdp2* varVdp2Regs);
vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode00WithK(RBGDrawInfo * rgb, int h, int v, Vdp2* varVdp2Regs);
vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode01NoK(RBGDrawInfo * rgb, int h, int v, Vdp2* varVdp2Regs);
vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode01WithK(RBGDrawInfo * rgb, int h, int v, Vdp2* varVdp2Regs);
vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode02NoK(RBGDrawInfo * rgb, int h, int v, Vdp2* varVdp2Regs);
vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode02WithKA(RBGDrawInfo * rgb, int h, int v, Vdp2* varVdp2Regs);
vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode02WithKAWithKB(RBGDrawInfo * rgb, int h, int v, Vdp2* varVdp2Regs);
vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode02WithKB(RBGDrawInfo * rgb, int h, int v, Vdp2* varVdp2Regs);
vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode03NoK(RBGDrawInfo * rgb, int h, int v, Vdp2* varVdp2Regs);
vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode03WithKA(RBGDrawInfo * rgb, int h, int v, Vdp2* varVdp2Regs);
vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode03WithKB(RBGDrawInfo * rgb, int h, int v, Vdp2* varVdp2Regs);
vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode03WithK(RBGDrawInfo * rgb, int h, int v, Vdp2* varVdp2Regs);


static void FASTCALL Vdp1ReadPriority(vdp1cmd_struct *cmd, int * priority, int * colorcl, int * normal_shadow, Vdp2 *varVdp2Regs);
static void FASTCALL Vdp1ReadTexture(vdp1cmd_struct *cmd, YglSprite *sprite, YglTexture *texture, Vdp2 *varVdp2Regs);

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL Vdp1ReadPolygonColor(vdp1cmd_struct *cmd, Vdp2* varVdp2Regs)
{
  // Check if transparent sprite window
  // hard/vdp2/hon/p08_12.htm#SPWINEN_
  if ((cmd->CMDCOLR & 0x8000) && // Sprite Window Color
      (varVdp2Regs->SPCTL & 0x10) && // Sprite Window is enabled
      !(cmd->CMDPMOD & 4) &&
      (((cmd->CMDPMOD >> 3) & 0x7) < 5)  && //Is palette
      ((varVdp2Regs->SPCTL & 0xF)  >=2 && (varVdp2Regs->SPCTL & 0xF) < 8)) // inside sprite type
  {
    return 0;
  }

  return VDP1COLOR(cmd->CMDPMOD, cmd->CMDCOLR);
}

static void FASTCALL Vdp1ReadTexture_in_sync(vdp1cmd_struct *cmd, int spritew, int spriteh, YglTexture *texture, Vdp2 *varVdp2Regs)
{
  int shadow = 0;
  int normalshadow = 0;
  int priority = 0;
  int colorcl = 0;
#ifdef SPRITE_CACHE
  u32* pixBuf = texture->textdata;
  Pattern* pattern = NULL;
#endif
  int endcnt = 0;
  int normal_shadow = 0;
  u32 charAddr = cmd->CMDSRCA * 8;
  u32 dot;
  u8 SPD = ((cmd->CMDPMOD & 0x40) != 0);
  u8 END = ((cmd->CMDPMOD & 0x80) != 0);
  u8 MSB = ((cmd->CMDPMOD & 0x8000) != 0);
  u8 MSB_SHADOW = 0;
  u32 alpha = 0xF8;
  int SPCCCS = (varVdp2Regs->SPCTL >> 12) & 0x3;
  VDP1LOG("Making new sprite %08X\n", charAddr);

  if (/*varVdp2Regs->SDCTL != 0 &&*/ MSB != 0) {
    MSB_SHADOW = 1;
    _Ygl->msb_shadow_count_[_Ygl->drawframe]++;
  }

#ifdef SPRITE_CACHE
  if (yabsys.useVdp1cache) {

    pattern = getPattern(cmd, Vdp1Ram, varVdp2Regs);
    if (pattern != NULL) {
      int characterWidth = ((cmd->CMDSIZE >> 8) & 0x3F) * 8;
      int characterHeight = cmd->CMDSIZE & 0xFF;
      for (int i=0; i<characterHeight; i++) {
         memcpy(&texture->textdata[i*(texture->w+characterWidth)], &pattern->pix[i*characterWidth], characterWidth*sizeof(u32));
      }
       texture->textdata+=characterHeight*(texture->w+characterWidth);
     return;
    }
  }
#endif
  switch ((cmd->CMDPMOD >> 3) & 0x7)
  {
  case 0:
  {
    // 4 bpp Bank mode
    u32 colorBank = cmd->CMDCOLR&0xFFF0;
    u16 i;

    for (i = 0; i < spriteh; i++) {
      u16 j;
      j = 0;
      while (j < spritew) {
        dot = Vdp1RamReadByte(NULL, Vdp1Ram, charAddr);
        if (((dot >> 4) == 0) && !SPD) *texture->textdata++ = 0x00;
        else if (((dot >> 4) == 0x0F) && !END) *texture->textdata++ = 0x00;
        else *texture->textdata++ = VDP1COLOR(cmd->CMDPMOD, ((dot >> 4) | colorBank));
        j += 1;
        if (((dot & 0xF) == 0) && !SPD) *texture->textdata++ = 0x00;
        else if (((dot & 0xF) == 0x0F) && !END) *texture->textdata++ = 0x00;
        else *texture->textdata++ = VDP1COLOR(cmd->CMDPMOD, ((dot & 0xF) | colorBank));
        j += 1;
        charAddr += 1;
      }
      texture->textdata += texture->w;
    }
    break;
  }
  case 1:
  {
    // 4 bpp LUT mode
    u16 temp;
    u32 colorLut = cmd->CMDCOLR * 8;
    u16 i;

    for (i = 0; i < spriteh; i++)
    {
      u16 j;
      j = 0;
      endcnt = 0;
      while (j < spritew)
      {
        dot = Vdp1RamReadByte(NULL, Vdp1Ram, charAddr);
        if (!END && endcnt >= 2) {
          *texture->textdata++ = 0;
        }
        else if (((dot >> 4) == 0) && !SPD)
        {
          *texture->textdata++ = 0;
        }
        else if (((dot >> 4) == 0x0F) && !END) // 6. Commandtable end code
        {
          *texture->textdata++ = 0;
          endcnt++;
        }
        else {
          u16 temp = Vdp1RamReadWord(NULL, Vdp1Ram, ((dot >> 4) * 2 + colorLut));
          *texture->textdata++ = VDP1COLOR(cmd->CMDPMOD, temp);
        }
        j += 1;
        if (!END && endcnt >= 2)
        {
          *texture->textdata++ = 0x00;
        }
        else if (((dot & 0xF) == 0) && !SPD)
        {
          *texture->textdata++ = 0x00;
        }
        else if (((dot & 0x0F) == 0x0F) && !END)
        {
          *texture->textdata++ = 0x0;
          endcnt++;
        }
        else {
          temp = Vdp1RamReadWord(NULL, Vdp1Ram, ((dot & 0xF) * 2 + colorLut));
          *texture->textdata++ = VDP1COLOR(cmd->CMDPMOD, temp);
        }
        j += 1;
        charAddr += 1;
      }
      texture->textdata += texture->w;
    }
    break;
  }
  case 2:
  {
    // 8 bpp(64 color) Bank mode
    u32 colorBank = cmd->CMDCOLR & 0xFFC0;

    u16 i, j;

    for (i = 0; i < spriteh; i++)
    {
      for (j = 0; j < spritew; j++)
      {
        dot = Vdp1RamReadByte(NULL, Vdp1Ram, charAddr);
        charAddr++;
        if ((dot == 0) && !SPD) *texture->textdata++ = 0x00;
        else if ((dot == 0xFF) && !END) *texture->textdata++ = 0x00;
        else *texture->textdata++ = VDP1COLOR(cmd->CMDPMOD, ((dot & 0x3F) | colorBank));
      }
      texture->textdata += texture->w;
    }
    break;
  }
  case 3:
  {
    // 8 bpp(128 color) Bank mode
    u32 colorBank = cmd->CMDCOLR & 0xFF80;
    u16 i, j;

    for (i = 0; i < spriteh; i++)
    {
      for (j = 0; j < spritew; j++)
      {
        dot = Vdp1RamReadByte(NULL, Vdp1Ram, charAddr);
        charAddr++;
        if ((dot == 0) && !SPD) *texture->textdata++ = 0x00;
        else if ((dot == 0xFF) && !END) *texture->textdata++ = 0x00;
        else *texture->textdata++ = VDP1COLOR(cmd->CMDPMOD, ((dot & 0x7F) | colorBank));
      }
      texture->textdata += texture->w;
    }
    break;
  }
  case 4:
  {
    // 8 bpp(256 color) Bank mode
    u32 colorBank = cmd->CMDCOLR & 0xFF00;
    u16 i, j;

    for (i = 0; i < spriteh; i++) {
      for (j = 0; j < spritew; j++) {
        dot = Vdp1RamReadByte(NULL, Vdp1Ram, charAddr);
        charAddr++;
        if ((dot == 0) && !SPD) *texture->textdata++ = 0x00;
        else if ((dot == 0xFF) && !END) *texture->textdata++ = 0x0;
        else *texture->textdata++ = VDP1COLOR(cmd->CMDPMOD, (dot | colorBank));
      }
      texture->textdata += texture->w;
    }
    break;
  }
  case 5:
  {
    // 16 bpp Bank mode
    u16 i, j;
    u16 temp;

    // hard/vdp2/hon/p09_20.htm#no9_21
    u8 *cclist = (u8 *)&varVdp2Regs->CCRSA;
    cclist[0] &= 0x1F;


    for (i = 0; i < spriteh; i++)
    {
      endcnt = 0;
      for (j = 0; j < spritew; j++)
      {
        temp = Vdp1RamReadWord(NULL, Vdp1Ram, charAddr);
        if ((temp & 0x8000) == 0) temp = 0;
        
        charAddr += 2;

        if (endcnt == 2) {
          *texture->textdata++ = 0x0;
        }
        else if ((temp == 0x7FFF) && !END) {
          *texture->textdata++ = 0x0;
          endcnt++;
        }
        else
          *texture->textdata++ = VDP1COLOR(cmd->CMDPMOD, temp);
      }
      texture->textdata += texture->w;
    }
    break;
  }
  default:
    VDP1LOG("Unimplemented sprite color mode: %X\n", (cmd->CMDPMOD >> 3) & 0x7);
    break;
   }
#ifdef SPRITE_CACHE
  if (yabsys.useVdp1cache) {
    addPattern(cmd, Vdp1Ram, pixBuf, texture->w, varVdp2Regs);
  }
#endif
}

#ifdef VDP1_TEXTURE_ASYNC
void Vdp1ReadTexture_in_async(void *p)
{
   while(vdp1text_run != 0){
     vdp1TextureTask *task = (vdp1TextureTask *)YabWaitEventQueue(vdp1q);
     if (task != NULL) {
       Vdp1ReadTexture_in_sync(task->cmd, task->w, task->h, task->texture, task->varVdp2Regs);
       free(task->cmd);
       free(task->texture);
       free(task->varVdp2Regs);
       free(task);
     }
     YabWaitEventQueue(vdp1q_end);
   }
}

static void FASTCALL Vdp1ReadTexture(vdp1cmd_struct *cmd, YglSprite *sprite, YglTexture *texture, Vdp2 *varVdp2Regs) {
   vdp1TextureTask *task = malloc(sizeof(vdp1TextureTask));

   task->cmd = malloc(sizeof(vdp1cmd_struct));
   task->texture = malloc(sizeof(YglTexture));
   task->varVdp2Regs = malloc(sizeof(Vdp2));

   memcpy(task->cmd, cmd, sizeof(vdp1cmd_struct));
   memcpy(task->texture, texture, sizeof(YglTexture));
   memcpy(task->varVdp2Regs, varVdp2Regs, sizeof(Vdp2));

   task->w = sprite->w;
   task->h = sprite->h;

   if (vdp1text_run == 0) {
     vdp1text_run = 1;
     vdp1q = YabThreadCreateQueue(NB_MSG);
     vdp1q_end = YabThreadCreateQueue(NB_MSG);
     YabThreadStart(YAB_THREAD_VDP1_0, Vdp1ReadTexture_in_async, 0);
     YabThreadStart(YAB_THREAD_VDP1_1, Vdp1ReadTexture_in_async, 0);
     YabThreadStart(YAB_THREAD_VDP1_2, Vdp1ReadTexture_in_async, 0);
     YabThreadStart(YAB_THREAD_VDP1_3, Vdp1ReadTexture_in_async, 0);
   }
   YabAddEventQueue(vdp1q_end, NULL);
   YabAddEventQueue(vdp1q, task);
   YabThreadYield();
}

int waitVdp1Textures( int sync) {
    int empty = 1;
    if (vdp1q_end == NULL) return 1;
    while (((empty = YaGetQueueSize(vdp1q_end))!=0) && (sync == 1))
    {
      YabThreadYield();
    }
    return (empty == 0);
}
#else
static void FASTCALL Vdp1ReadTexture(vdp1cmd_struct *cmd, YglSprite *sprite, YglTexture *texture, Vdp2 *varVdp2Regs) {
   Vdp1ReadTexture_in_sync(cmd, sprite->w, sprite->h, texture, varVdp2Regs);
}
#endif

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL Vdp1ReadPriority(vdp1cmd_struct *cmd, int * priority, int * colorcl, int * normal_shadow, Vdp2 *varVdp2Regs)
{
  u8 SPCLMD = varVdp2Regs->SPCTL;
  int sprite_register;
  u16 lutPri;
  u16 reg_src = cmd->CMDCOLR;
  int not_lut = 1;

  // is the sprite is RGB or LUT (in fact, LUT can use bank color, we just hope it won't...)
  if ((SPCLMD & 0x20) && (cmd->CMDCOLR & 0x8000))
  {
    // RGB data, use register 0
    *priority = 0;
    *colorcl = 0;
    return;
  }

  if (((cmd->CMDPMOD >> 3) & 0x7) == 1) {
    u32 charAddr, dot, colorLut;

    *priority = 0;

    charAddr = cmd->CMDSRCA * 8;
    dot = Vdp1RamReadByte(NULL, Vdp1Ram, charAddr);
    colorLut = cmd->CMDCOLR * 8;
    lutPri = Vdp1RamReadWord(NULL, Vdp1Ram, (dot >> 4) * 2 + colorLut);
    if (!(lutPri & 0x8000)) {
      not_lut = 0;
      reg_src = lutPri;
    }
    else
      return;
  }

  {
    u8 sprite_type = SPCLMD & 0xF;
    switch (sprite_type)
    {
    case 0:
      sprite_register = (reg_src & 0xC000) >> 14;
      *priority = sprite_register;
      *colorcl = (cmd->CMDCOLR >> 11) & 0x07;
      *normal_shadow = 0x7FE;
      if (not_lut) cmd->CMDCOLR &= 0x7FF;
      break;
    case 1:
      sprite_register = (reg_src & 0xE000) >> 13;
      *priority = sprite_register;
      *colorcl = (cmd->CMDCOLR >> 11) & 0x03;
      *normal_shadow = 0x7FE;
      if (not_lut) cmd->CMDCOLR &= 0x7FF;
      break;
    case 2:
      sprite_register = (reg_src >> 14) & 0x1;
      *priority = sprite_register;
      *colorcl = (cmd->CMDCOLR >> 11) & 0x07;
      *normal_shadow = 0x7FE;
      if (not_lut) cmd->CMDCOLR &= 0x7FF;
      break;
    case 3:
      sprite_register = (reg_src & 0x6000) >> 13;
      *priority = sprite_register;
      *colorcl = ((cmd->CMDCOLR >> 11) & 0x03);
      *normal_shadow = 0x7FE;
      if (not_lut) cmd->CMDCOLR &= 0x7FF;
      break;
    case 4:
      sprite_register = (reg_src & 0x6000) >> 13;
      *priority = sprite_register;
      *colorcl = (cmd->CMDCOLR >> 10) & 0x07;
      *normal_shadow = 0x3FE;
      if (not_lut) cmd->CMDCOLR &= 0x3FF;
      break;
    case 5:
      sprite_register = (reg_src & 0x7000) >> 12;
      *priority = sprite_register & 0x7;
      *colorcl = (cmd->CMDCOLR >> 11) & 0x01;
      *normal_shadow = 0x7FE;
      if (not_lut) cmd->CMDCOLR &= 0x7FF;
      break;
    case 6:
      sprite_register = (reg_src & 0x7000) >> 12;
      *priority = sprite_register;
      *colorcl = (cmd->CMDCOLR >> 10) & 0x03;
      *normal_shadow = 0x3FE;
      if (not_lut) cmd->CMDCOLR &= 0x3FF;
      break;
    case 7:
      sprite_register = (reg_src & 0x7000) >> 12;
      *priority = sprite_register;
      *colorcl  = (cmd->CMDCOLR >> 9) & 0x07;
      *normal_shadow = 0x1FE;
      if (not_lut) cmd->CMDCOLR &= 0x1FF;
      break;
    case 8:
      sprite_register = (reg_src & 0x80) >> 7;
      *priority = sprite_register;
      *normal_shadow = 0x7E;
      *colorcl = 0;
      if (not_lut) cmd->CMDCOLR &= 0x7F;
      break;
    case 9:
      sprite_register = (reg_src & 0x80) >> 7;
      *priority = sprite_register;;
      *colorcl = ((cmd->CMDCOLR >> 6) & 0x01);
      *normal_shadow = 0x3E;
      if (not_lut) cmd->CMDCOLR &= 0x3F;
      break;
    case 10:
      sprite_register = (reg_src & 0xC0) >> 6;
      *priority = sprite_register;
      *colorcl = 0;
      if (not_lut) cmd->CMDCOLR &= 0x3F;
      break;
    case 11:
      sprite_register = 0;
      *priority = sprite_register;
      *colorcl  = (cmd->CMDCOLR >> 6) & 0x03;
      *normal_shadow = 0x3E;
      if (not_lut) cmd->CMDCOLR &= 0x3F;
      break;
    case 12:
      sprite_register = (reg_src & 0x80) >> 7;
      *priority = sprite_register;
      *colorcl = 0;
      *normal_shadow = 0xFE;
      if (not_lut) cmd->CMDCOLR &= 0xFF;
      break;
    case 13:
      sprite_register = (reg_src & 0x80) >> 7;
      *priority = sprite_register;
      *colorcl = (cmd->CMDCOLR >> 6) & 0x01;
      *normal_shadow = 0xFE;
      if (not_lut) cmd->CMDCOLR &= 0xFF;
      break;
    case 14:
      sprite_register = (reg_src & 0xC0) >> 6;
      *priority = sprite_register;
      *colorcl = 0;
      *normal_shadow = 0xFE;
      if (not_lut) cmd->CMDCOLR &= 0xFF;
      break;
    case 15:
      sprite_register = 0;
      *priority = sprite_register;
      *colorcl = ((cmd->CMDCOLR >> 6) & 0x03);
      *normal_shadow = 0xFE;
      if (not_lut) cmd->CMDCOLR &= 0xFF;
      break;
    default:
      VDP1LOG("sprite type %d not implemented\n", sprite_type);
      *priority = 0;
      *colorcl = 0;
      break;
  }
}
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp1SetTextureRatio(int vdp2widthratio, int vdp2heightratio)
{
  float vdp1w = 1;
  float vdp1h = 1;

  // may need some tweaking
  if (Vdp1Regs->TVMR & 0x1) VDP1_MASK = 0xFF;
  else VDP1_MASK = 0xFFFF;

  // Figure out which vdp1 screen mode to use
  switch (Vdp1Regs->TVMR & 7)
  {
  case 0:
  case 2:
  case 3:
    vdp1w = 1;
    break;
  case 1:
    vdp1w = 2;
    break;
  default:
    vdp1w = 1;
    vdp1h = 1;
    break;
  }

  // Is double-interlace enabled?
  if (Vdp1Regs->FBCR & 0x8) {
    vdp1h = 2;
    vdp1_interlace = (Vdp1Regs->FBCR & 0x4) ? 2 : 1;
  }
  else {
    vdp1_interlace = 0;
  }

  _Ygl->vdp1wratio = (float)vdp2widthratio / vdp1w;
  _Ygl->vdp1hratio = (float)vdp2heightratio / vdp1h;
}

//////////////////////////////////////////////////////////////////////////////
static u16 Vdp2ColorRamGetColorRaw(u32 colorindex) {
  switch (Vdp2Internal.ColorMode)
  {
  case 0:
  case 1:
  {
    colorindex <<= 1;
    return T2ReadWord(Vdp2ColorRam, colorindex & 0xFFF);
  }
  case 2:
  {
    colorindex <<= 2;
    colorindex &= 0xFFF;
    return T2ReadWord(Vdp2ColorRam, colorindex);
  }
  default: break;
  }
  return 0;
}

static u32 Vdp2ColorRamGetColor(u32 colorindex, int alpha)
{
  switch (Vdp2Internal.ColorMode)
  {
  case 0:
  case 1:
  {
    u32 tmp;
    colorindex <<= 1;
    tmp = T2ReadWord(Vdp2ColorRam, colorindex & 0xFFF);
    return SAT2YAB1(alpha, tmp);
  }
  case 2:
  {
    u32 tmp1, tmp2;
    colorindex <<= 2;
    colorindex &= 0xFFF;
    tmp1 = T2ReadWord(Vdp2ColorRam, colorindex);
    tmp2 = T2ReadWord(Vdp2ColorRam, colorindex + 2);
    return SAT2YAB2(alpha, tmp1, tmp2);
  }
  default: break;
  }
  return 0;
}


//////////////////////////////////////////////////////////////////////////////
// Window

int Vdp2GenerateWindowInfo(Vdp2 *varVdp2Regs)
{
  int HShift;
  int v = 0;
  u32 LineWinAddr;
  int upWindow = 0;
  int update = 0;
  u32 val = 0;

  YglGetWindowPointer(0);
  YglGetWindowPointer(1);

  HShift = 0;
  if (_Ygl->rwidth >= 640) HShift = 0; else HShift = 1;

  // Line Table mode
  if ((varVdp2Regs->LWTA0.part.U & 0x8000))
  {
    // start address
    LineWinAddr = (u32)((((varVdp2Regs->LWTA0.part.U & 0x07) << 15) | (varVdp2Regs->LWTA0.part.L >> 1)) << 2);
    for (v = 0; v < _Ygl->rheight; v++) {
      if (v >= varVdp2Regs->WPSY0 && v <= varVdp2Regs->WPEY0) {
        short HStart = Vdp2RamReadWord(NULL, Vdp2Ram, LineWinAddr + (v << 2));
        short HEnd = Vdp2RamReadWord(NULL, Vdp2Ram, LineWinAddr + (v << 2) + 2);
        u32 temp = (HStart>>HShift) | ((HEnd>>HShift) << 16);
        if (HStart > HEnd) val = 0x0;
        else val = temp;
      } else val = 0x0;
      if (val != _Ygl->win[0][v]) {
        _Ygl->win[0][v] = val;
        update = 1;
      }
    }
    // Parameter Mode
  }
  else {
    for (v = 0; v < _Ygl->rheight; v++) {
      if (v >= varVdp2Regs->WPSY0 && v <= varVdp2Regs->WPEY0) {
        u32 temp = (varVdp2Regs->WPSX0 >>HShift) | ((varVdp2Regs->WPEX0>>HShift) << 16);
        if (varVdp2Regs->WPSX0 > varVdp2Regs->WPEX0) val = 0x0;
        else val = temp;
      } else val = 0x0;
      if (val != _Ygl->win[0][v]) {
        _Ygl->win[0][v] = val;
        update = 1;
      }
    }
  }
  // Line Table mode
  if ((varVdp2Regs->LWTA1.part.U & 0x8000))
  {
    // start address
    LineWinAddr = (u32)((((varVdp2Regs->LWTA1.part.U & 0x07) << 15) | (varVdp2Regs->LWTA1.part.L >> 1)) << 2);
    for (v = 0; v < _Ygl->rheight; v++) {
      if (v >= varVdp2Regs->WPSY1 && v <= varVdp2Regs->WPEY1) {
        short HStart = Vdp2RamReadWord(NULL, Vdp2Ram, LineWinAddr + (v << 2));
        short HEnd = Vdp2RamReadWord(NULL, Vdp2Ram, LineWinAddr + (v << 2) + 2);
        u32 temp = (HStart>>HShift) | ((HEnd>>HShift) << 16);
        if (HStart > HEnd) val = 0x0;
        else val = temp;
      } else val = 0x0;
      if (val != _Ygl->win[1][v]) {
        _Ygl->win[1][v] = val;
        update = 1;
      }
    }
    // Parameter Mode
  }
  else {
    for (v = 0; v < _Ygl->rheight; v++) {
      if (v >= varVdp2Regs->WPSY1 && v <= varVdp2Regs->WPEY1) {
        u32 temp = (varVdp2Regs->WPSX1 >>HShift) | ((varVdp2Regs->WPEX1>>HShift) << 16);
        if (varVdp2Regs->WPSX1 > varVdp2Regs->WPEX1) val = 0x0;
        else val = temp;
      } else val = 0x0;
      if (val != _Ygl->win[1][v]) {
        _Ygl->win[1][v] = val;
        update = 1;
      }
    }
  }
  YglSetWindow(0);
  YglSetWindow(1);
  return update;
}

// 0 .. outside,1 .. inside
static INLINE int Vdp2CheckWindow(vdp2draw_struct *info, int x, int y, int area, u32* win)
{
  if (y < 0) return 0;
  if (y >= _Ygl->rheight) return 0;
  int upLx = win[y] & 0xFFFF;
  int upRx = (win[y] >> 16) & 0xFFFF;
  // inside
  if (area == 1)
  {
    if (win[y] == 0) return 0;
    if (x >= upLx && x <= upRx)
    {
      return 1;
    }
    else {
      return 0;
    }
    // outside
  }
  else {
    if (win[y] == 0) return 1;
    if (x < upLx) return 1;
    if (x > upRx) return 1;
    return 0;
  }
  return 0;
}

// 0 .. all outsize, 1~3 .. partly inside, 4.. all inside
static int FASTCALL Vdp2CheckWindowRange(vdp2draw_struct *info, int x, int y, int w, int h, Vdp2 *varVdp2Regs)
{
  int rtn = 0;

  if (_Ygl->Win0[info->idScreen]  != 0 && _Ygl->Win1[info->idScreen]  == 0)
  {
    rtn += Vdp2CheckWindow(info, x, y, _Ygl->Win0_mode[info->idScreen], _Ygl->win[0]);
    rtn += Vdp2CheckWindow(info, x + w, y, _Ygl->Win0_mode[info->idScreen], _Ygl->win[0]);
    rtn += Vdp2CheckWindow(info, x + w, y + h, _Ygl->Win0_mode[info->idScreen], _Ygl->win[0]);
    rtn += Vdp2CheckWindow(info, x, y + h, _Ygl->Win0_mode[info->idScreen], _Ygl->win[0]);
    return rtn;
  }
  else if (_Ygl->Win0[info->idScreen]  == 0 && _Ygl->Win1[info->idScreen]  != 0)
  {
    rtn += Vdp2CheckWindow(info, x, y, _Ygl->Win1_mode[info->idScreen], _Ygl->win[1]);
    rtn += Vdp2CheckWindow(info, x + w, y, _Ygl->Win1_mode[info->idScreen], _Ygl->win[1]);
    rtn += Vdp2CheckWindow(info, x + w, y + h, _Ygl->Win1_mode[info->idScreen], _Ygl->win[1]);
    rtn += Vdp2CheckWindow(info, x, y + h, _Ygl->Win1_mode[info->idScreen], _Ygl->win[1]);
    return rtn;
  }
  else if (_Ygl->Win0[info->idScreen]  != 0 && _Ygl->Win1[info->idScreen]  != 0)
  {
    if (_Ygl->Win_op[info->idScreen] == 0)
    {
      rtn += (Vdp2CheckWindow(info, x, y, _Ygl->Win0_mode[info->idScreen], _Ygl->win[0]) &
        Vdp2CheckWindow(info, x, y, _Ygl->Win1_mode[info->idScreen], _Ygl->win[1]));
      rtn += (Vdp2CheckWindow(info, x + w, y, _Ygl->Win0_mode[info->idScreen], _Ygl->win[0])&
        Vdp2CheckWindow(info, x + w, y, _Ygl->Win1_mode[info->idScreen], _Ygl->win[1]));
      rtn += (Vdp2CheckWindow(info, x + w, y + h, _Ygl->Win0_mode[info->idScreen], _Ygl->win[0])&
        Vdp2CheckWindow(info, x + w, y + h, _Ygl->Win1_mode[info->idScreen], _Ygl->win[1]));
      rtn += (Vdp2CheckWindow(info, x, y + h, _Ygl->Win0_mode[info->idScreen], _Ygl->win[0]) &
        Vdp2CheckWindow(info, x, y + h, _Ygl->Win1_mode[info->idScreen], _Ygl->win[1]));
      return rtn;
    }
    else {
      rtn += (Vdp2CheckWindow(info, x, y, _Ygl->Win0_mode[info->idScreen], _Ygl->win[0]) |
        Vdp2CheckWindow(info, x, y, _Ygl->Win1_mode[info->idScreen], _Ygl->win[1]));
      rtn += (Vdp2CheckWindow(info, x + w, y, _Ygl->Win0_mode[info->idScreen], _Ygl->win[0]) |
        Vdp2CheckWindow(info, x + w, y, _Ygl->Win1_mode[info->idScreen], _Ygl->win[1]));
      rtn += (Vdp2CheckWindow(info, x + w, y + h, _Ygl->Win0_mode[info->idScreen], _Ygl->win[0]) |
        Vdp2CheckWindow(info, x + w, y + h, _Ygl->Win1_mode[info->idScreen], _Ygl->win[1]));
      rtn += (Vdp2CheckWindow(info, x, y + h, _Ygl->Win0_mode[info->idScreen], _Ygl->win[0]) |
        Vdp2CheckWindow(info, x, y + h, _Ygl->Win1_mode[info->idScreen], _Ygl->win[1]));
      return rtn;
    }
  }
  return 0;
}

void Vdp2GenLineinfo(vdp2draw_struct *info)
{
  int bound = 0;
  int i;
  u16 val1, val2;
  int index = 0;
  int lineindex = 0;
  if (info->lineinc == 0 || info->islinescroll == 0) return;

  if (VDPLINE_SY(info->islinescroll)) bound += 0x04;
  if (VDPLINE_SX(info->islinescroll)) bound += 0x04;
  if (VDPLINE_SZ(info->islinescroll)) bound += 0x04;

  for (i = 0; i < _Ygl->rheight; i += info->lineinc)
  {
    index = 0;
    if (VDPLINE_SX(info->islinescroll))
    {
      info->lineinfo[lineindex].LineScrollValH = Vdp2RamReadWord(NULL, Vdp2Ram, info->linescrolltbl + (i / info->lineinc)*bound);
      if ((info->lineinfo[lineindex].LineScrollValH & 0x400)) info->lineinfo[lineindex].LineScrollValH |= 0xF800; else info->lineinfo[lineindex].LineScrollValH &= 0x07FF;
      index += 4;
    }
    else {
      info->lineinfo[lineindex].LineScrollValH = 0;
    }

    if (VDPLINE_SY(info->islinescroll))
    {
      info->lineinfo[lineindex].LineScrollValV = Vdp2RamReadWord(NULL, Vdp2Ram, info->linescrolltbl + (i / info->lineinc)*bound + index);
      if ((info->lineinfo[lineindex].LineScrollValV & 0x400)) info->lineinfo[lineindex].LineScrollValV |= 0xF800; else info->lineinfo[lineindex].LineScrollValV &= 0x07FF;
      index += 4;
    }
    else {
      info->lineinfo[lineindex].LineScrollValV = 0;
    }

    if (VDPLINE_SZ(info->islinescroll))
    {
      val1 = Vdp2RamReadWord(NULL, Vdp2Ram, info->linescrolltbl + (i / info->lineinc)*bound + index);
      val2 = Vdp2RamReadWord(NULL, Vdp2Ram, info->linescrolltbl + (i / info->lineinc)*bound + index + 2);
      info->lineinfo[lineindex].CoordinateIncH = (((int)((val1) & 0x07) << 8) | (int)((val2) >> 8));
      index += 4;
    }
    else {
      info->lineinfo[lineindex].CoordinateIncH = 0x0100;
    }

    lineindex++;
  }
}

INLINE void Vdp2SetSpecialPriority(vdp2draw_struct *info, u8 dot, u32 *prio, u32 * cramindex ) {
  *prio = info->priority;
  if (info->specialprimode == 2) {
    *prio = info->priority & 0xE;
    if (info->specialfunction & 1) {
      if (PixelIsSpecialPriority(info->specialcode, dot))
      {
        *prio |= 1;
      }
    }
  }
}

static INLINE u32 Vdp2GetCCOn(vdp2draw_struct *info, u8 dot, u32 cramindex, Vdp2 *varVdp2Regs) {

  const int CCMD = ((varVdp2Regs->CCCTL >> 8) & 0x01);  // hard/vdp2/hon/p12_14.htm#CCMD_
  int cc = 1;
  if (CCMD == 0) {  // Calculate Rate mode
    switch (info->specialcolormode)
    {
    case 1: if (info->specialcolorfunction == 0) { cc = 0; } break;
    case 2:
      if (info->specialcolorfunction == 0) { cc = 0; }
      else { if ((info->specialcode & (1 << ((dot & 0xF) >> 1))) == 0) { cc = 0; } }
      break;
   case 3:
     if (((Vdp2ColorRamGetColorRaw(cramindex) & 0x8000) == 0)) { cc = 0; }
     break;
    }
  }
  else {  // Calculate Add mode
    switch (info->specialcolormode)
    {
    case 1:
      if (info->specialcolorfunction == 0) { cc = 0; }
      break;
    case 2:
      if (info->specialcolorfunction == 0) { cc = 0; }
      else { if ((info->specialcode & (1 << ((dot & 0xF) >> 1))) == 0) { cc = 0; } }
      break;
   case 3:
     if (((Vdp2ColorRamGetColorRaw(cramindex) & 0x8000) == 0)) { cc = 0; }
     break;
    }
  }
  return cc;
}


static INLINE u32 Vdp2GetPixel4bpp(vdp2draw_struct *info, u32 addr, YglTexture *texture, Vdp2 *varVdp2Regs) {

  u32 cramindex;
  u16 dotw = Vdp2RamReadWord(NULL, Vdp2Ram, addr);
  u8 dot;
  u32 cc;
  u32 priority = 0;

  dot = (dotw & 0xF000) >> 12;
  if (!(dot & 0xF) && info->transparencyenable) {
    *texture->textdata++ = 0x00000000;
  } else {
    cramindex = (info->coloroffset + ((info->paladdr << 4) | (dot & 0xF)));
    Vdp2SetSpecialPriority(info, dot, &priority, &cramindex);
    cc = Vdp2GetCCOn(info, dot, cramindex, varVdp2Regs);
    *texture->textdata++ = VDP2COLOR(info->idScreen, info->alpha, priority, cc, cramindex);
  }

  cc = 1;
  dot = (dotw & 0xF00) >> 8;
  if (!(dot & 0xF) && info->transparencyenable) {
    *texture->textdata++ = 0x00000000;
  }
  else {
    cramindex = (info->coloroffset + ((info->paladdr << 4) | (dot & 0xF)));
    Vdp2SetSpecialPriority(info, dot, &priority, &cramindex);
    cc = Vdp2GetCCOn(info, dot, cramindex, varVdp2Regs);
    *texture->textdata++ = VDP2COLOR(info->idScreen, info->alpha, priority, cc, cramindex);
  }

  cc = 1;
  dot = (dotw & 0xF0) >> 4;
  if (!(dot & 0xF) && info->transparencyenable) {
    *texture->textdata++ = 0x00000000;
  }
  else {
    cramindex = (info->coloroffset + ((info->paladdr << 4) | (dot & 0xF)));
    Vdp2SetSpecialPriority(info, dot, &priority, &cramindex);
    cc = Vdp2GetCCOn(info, dot, cramindex, varVdp2Regs);
    *texture->textdata++ = VDP2COLOR(info->idScreen, info->alpha, priority, cc, cramindex);
  }

  cc = 1;
  dot = (dotw & 0xF);
  if (!(dot & 0xF) && info->transparencyenable) {
    *texture->textdata++ = 0x00000000;
  }
  else {
    cramindex = (info->coloroffset + ((info->paladdr << 4) | (dot & 0xF)));
    Vdp2SetSpecialPriority(info, dot, &priority, &cramindex);
    cc = Vdp2GetCCOn(info, dot, cramindex, varVdp2Regs);
    *texture->textdata++ = VDP2COLOR(info->idScreen, info->alpha, priority, cc, cramindex);
  }
  return 0;
}

static INLINE u32 Vdp2GetPixel8bpp(vdp2draw_struct *info, u32 addr, YglTexture *texture, Vdp2* varVdp2Regs) {

  u32 cramindex;
  u16 dotw = Vdp2RamReadWord(NULL, Vdp2Ram, addr);
  u8 dot;
  u32 cc;
  u32 priority = 0;

  cc = 1;
  dot = (dotw & 0xFF00)>>8;
  if (!(dot & 0xFF) && info->transparencyenable) *texture->textdata++ = 0x00000000;
  else {
    cramindex = info->coloroffset + ((info->paladdr << 4) | (dot & 0xFF));
    Vdp2SetSpecialPriority(info, dot, &priority, &cramindex);
    cc = Vdp2GetCCOn(info, dot, cramindex, varVdp2Regs);
    *texture->textdata++ = VDP2COLOR(info->idScreen, info->alpha, priority, cc, cramindex);
  }
  cc = 1;
  dot = (dotw & 0xFF);
  if (!(dot & 0xFF) && info->transparencyenable) *texture->textdata++ = 0x00000000;
  else {
    cramindex = info->coloroffset + ((info->paladdr << 4) | (dot & 0xFF));
    Vdp2SetSpecialPriority(info, dot, &priority, &cramindex);
    cc = Vdp2GetCCOn(info, dot, cramindex, varVdp2Regs);
    *texture->textdata++ = VDP2COLOR(info->idScreen, info->alpha, priority, cc, cramindex);
  }
  return 0;
}


static INLINE u32 Vdp2GetPixel16bpp(vdp2draw_struct *info, u32 addr, Vdp2* varVdp2Regs) {
  u32 cramindex;
  u8 cc;
  u16 dot = Vdp2RamReadWord(NULL, Vdp2Ram, addr);
  u32 priority = 0;
  if ((dot == 0) && info->transparencyenable) return 0x00000000;
  else {
    cramindex = info->coloroffset + dot;
    Vdp2SetSpecialPriority(info, dot, &priority, &cramindex);
    cc = Vdp2GetCCOn(info, dot, cramindex, varVdp2Regs);
    return VDP2COLOR(info->idScreen, info->alpha, priority, cc, cramindex);
  }
}

static INLINE u32 Vdp2GetPixel16bppbmp(vdp2draw_struct *info, u32 addr, Vdp2 *varVdp2Regs) {
  u32 color;
  u16 dot = Vdp2RamReadWord(NULL, Vdp2Ram, addr);
//if (info->patternwh == 2) printf("%x\n", dot);
//Ca deconne ici
  int cc = Vdp2GetCCOn(info, dot, 0, varVdp2Regs);
  if (!(dot & 0x8000) && info->transparencyenable) color = 0x00000000;
  else color = VDP2COLOR(info->idScreen, info->alpha, info->priority, cc, RGB555_TO_RGB24(dot));
  return color;
}

static INLINE u32 Vdp2GetPixel32bppbmp(vdp2draw_struct *info, u32 addr, Vdp2 *varVdp2Regs) {
  u32 color;
  u16 dot1, dot2;
  int cc;
  dot1 = Vdp2RamReadWord(NULL, Vdp2Ram, addr);
  dot2 = Vdp2RamReadWord(NULL, Vdp2Ram, addr+2);

  cc = Vdp2GetCCOn(info, 0, 0, varVdp2Regs);

  if (!(dot1 & 0x8000) && info->transparencyenable) color = 0x00000000;
  else color = VDP2COLOR(info->idScreen, info->alpha, info->priority, cc, (((dot1 & 0xFF) << 16) | (dot2 & 0xFF00) | (dot2 & 0xFF)));
  return color;
}

static void FASTCALL Vdp2DrawCellInterlace(vdp2draw_struct *info, YglTexture *texture, Vdp2 *varVdp2Regs) {
  int i, j, h, addr, inc;
  unsigned int *start;
  unsigned int color;
  switch (info->colornumber)
  {
  case 0: // 4 BPP
    for (i = 0; i < info->cellh; i++)
    {
      for (j = 0; j < info->cellw; j += 4)
      {
        Vdp2GetPixel4bpp(info, info->charaddr, texture, varVdp2Regs);
        info->charaddr += 2;
      }
      texture->textdata += texture->w;
    }
    break;
  case 1: // 8 BPP
    for (i = 0; i < info->cellh; i++)
    {
      for (j = 0; j < info->cellw; j += 2)
      {

        Vdp2GetPixel8bpp(info, info->charaddr, texture, varVdp2Regs);
        info->charaddr += 2;
      }
      texture->textdata += texture->w;
    }
    break;
  case 2: // 16 BPP(palette)
    for (i = 0; i < info->cellh; i++)
    {
      for (j = 0; j < info->cellw; j++)
      {
        *texture->textdata++ = Vdp2GetPixel16bpp(info, info->charaddr, varVdp2Regs);
        info->charaddr += 2;
      }
      texture->textdata += texture->w;
    }
    break;
  case 3: // 16 BPP(RGB)
    addr = info->charaddr;
    inc = (info->cellw + texture->w);
    start = texture->textdata;
    texture->textdata += (info->cellw + texture->w)*info->cellh/2;
    info->charaddr += (!vdp2_is_odd_frame)?0:2*info->cellw;
    for (i = 0; i < info->cellh/2; i+=2)
    {
      for (j = 0; j < info->cellw; j++)
      {
        color = Vdp2GetPixel16bppbmp(info, info->charaddr, varVdp2Regs);
        *(texture->textdata+inc) = color;
        *texture->textdata = color;
        *texture->textdata++;
        info->charaddr += 2;
      }
      info->charaddr += 2*info->cellw;
      texture->textdata += inc;
    }
    info->charaddr = addr;
    info->charaddr += (!vdp2_is_odd_frame)?2*info->cellw:0;
    texture->textdata = start;
    for (i = 0; i < info->cellh/2; i+=2)
    {
      for (j = 0; j < info->cellw; j++)
      {
        color = Vdp2GetPixel16bppbmp(info, info->charaddr, varVdp2Regs);
        *(texture->textdata+inc) = color;
        *texture->textdata = color;
        *texture->textdata++;
        info->charaddr += 2;
      }
      info->charaddr += 2*info->cellw;
      texture->textdata += inc;
    }
    break;
  case 4: // 32 BPP
    for (i = 0; i < info->cellh; i++)
    {
      for (j = 0; j < info->cellw; j++)
      {
        *texture->textdata++ = Vdp2GetPixel32bppbmp(info, info->charaddr, varVdp2Regs);
        info->charaddr += 4;
      }
      texture->textdata += texture->w;
    }
    break;
  }
}

static void FASTCALL Vdp2DrawCell_in_sync(vdp2draw_struct *info, YglTexture *texture, Vdp2 *varVdp2Regs)
{
  int i, j;


  if ((vdp2_interlace == 1) && (_Ygl->rheight > 448)) {
    // Weird... Partly fix True Pinball in case of interlace only but it is breaking Zen Nihon Pro Wres, so use the bad test of the height
    Vdp2DrawCellInterlace(info, texture, varVdp2Regs);
    return;
  }

  switch (info->colornumber)
  {
  case 0: // 4 BPP
    for (i = 0; i < info->cellh; i++)
    {
      for (j = 0; j < info->cellw; j += 4)
      {
        Vdp2GetPixel4bpp(info, info->charaddr, texture, varVdp2Regs);
        info->charaddr += 2;
      }
      texture->textdata += texture->w;
    }
    break;
  case 1: // 8 BPP
    for (i = 0; i < info->cellh; i++)
    {
      for (j = 0; j < info->cellw; j += 2)
      {
        Vdp2GetPixel8bpp(info, info->charaddr, texture, varVdp2Regs);
        info->charaddr += 2;
      }
      texture->textdata += texture->w;
    }
    break;
  case 2: // 16 BPP(palette)
    for (i = 0; i < info->cellh; i++)
    {
      for (j = 0; j < info->cellw; j++)
      {
        *texture->textdata++ = Vdp2GetPixel16bpp(info, info->charaddr, varVdp2Regs);
        info->charaddr += 2;
      }
      texture->textdata += texture->w;
    }
    break;
  case 3: // 16 BPP(RGB)
    for (i = 0; i < info->cellh; i++)
    {
      for (j = 0; j < info->cellw; j++)
      {
        *texture->textdata++ = Vdp2GetPixel16bppbmp(info, info->charaddr, varVdp2Regs);
        info->charaddr += 2;
      }
      texture->textdata += texture->w;
    }
    break;
  case 4: // 32 BPP
    for (i = 0; i < info->cellh; i++)
    {
      for (j = 0; j < info->cellw; j++)
      {
        *texture->textdata++ = Vdp2GetPixel32bppbmp(info, info->charaddr, varVdp2Regs);
        info->charaddr += 4;
      }
      texture->textdata += texture->w;
    }
    break;
  }
}


static void FASTCALL Vdp2DrawBitmapLineScroll(vdp2draw_struct *info, YglTexture *texture, int width, int height,  Vdp2 *varVdp2Regs)
{
  int i, j;

  for (i = 0; i < height; i++)
  {
    int sh, sv;
    u32 baseaddr;
    vdp2Lineinfo * line;
    baseaddr = (u32)info->charaddr;
    line = &(info->lineinfo[i*info->lineinc]);

    if (VDPLINE_SX(info->islinescroll))
      sh = line->LineScrollValH + info->sh;
    else
      sh = info->sh;

    if (VDPLINE_SY(info->islinescroll))
      sv = line->LineScrollValV + info->sv;
    else
      sv = i + info->sv;

    sv &= (info->cellh - 1);
    sh &= (info->cellw - 1);
    if (line->LineScrollValH >= 0 && line->LineScrollValH < sh) {
      sv -= 1;
    }
    switch (info->colornumber) {
    case 0:
      baseaddr += ((sh + sv * (info->cellw >> 2)) << 1);
      for (j = 0; j < width; j += 4)
      {
        Vdp2GetPixel4bpp(info, baseaddr, texture, varVdp2Regs);
        baseaddr += 2;
      }
      break;
    case 1:
      baseaddr += sh + sv * info->cellw;
      for (j = 0; j < width; j += 2)
      {
        Vdp2GetPixel8bpp(info, baseaddr, texture, varVdp2Regs);
        baseaddr += 2;
      }
      break;
    case 2:
      baseaddr += ((sh + sv * info->cellw) << 1);
      for (j = 0; j < width; j++)
      {
        *texture->textdata++ = Vdp2GetPixel16bpp(info, baseaddr, varVdp2Regs);
        baseaddr += 2;

      }
      break;
    case 3:
      baseaddr += ((sh + sv * info->cellw) << 1);
      for (j = 0; j < width; j++)
      {
        *texture->textdata++ = Vdp2GetPixel16bppbmp(info, baseaddr, varVdp2Regs);
        baseaddr += 2;
      }
      break;
    case 4:
      baseaddr += ((sh + sv * info->cellw) << 2);
      for (j = 0; j < width; j++)
      {
        //if (info->isverticalscroll){
        //	sv += T1ReadLong(Vdp2Ram, info->verticalscrolltbl+(j>>3) ) >> 16;
        //}
        *texture->textdata++ = Vdp2GetPixel32bppbmp(info, baseaddr, varVdp2Regs);
        baseaddr += 4;
      }
      break;
    }
    texture->textdata += texture->w;
  }
}


static void FASTCALL Vdp2DrawBitmapCoordinateInc(vdp2draw_struct *info, YglTexture *texture,  Vdp2 *varVdp2Regs)
{
  u32 color;
  int i, j;

  int incv = 1.0 / info->coordincy*256.0;
  int inch = 1.0 / info->coordincx*256.0;

  int lineinc = 1;
  int linestart = 0;

  int height = _Ygl->rheight;

  // Is double-interlace enabled?
/*
  if ((vdp1_interlace != 0) || (height >= 448)) {
    lineinc = 2;
  }

  if (vdp1_interlace != 0) {
    linestart = vdp1_interlace - 1;
  }
*/

  for (i = linestart; i < lineinc*height; i += lineinc)
  {
    int sh, sv;
    int v;
    u32 baseaddr;
    vdp2Lineinfo * line;
    baseaddr = (u32)info->charaddr;
    line = &(info->lineinfo[i*info->lineinc]);

    v = (i*incv) >> 8;
    if (VDPLINE_SZ(info->islinescroll))
      inch = line->CoordinateIncH;

    if (inch == 0) inch = 1;

    if (VDPLINE_SX(info->islinescroll))
      sh = info->sh + line->LineScrollValH;
    else
      sh = info->sh;

    if (VDPLINE_SY(info->islinescroll))
      sv = info->sv + line->LineScrollValV;
    else
      sv = v + info->sv;

    //sh &= (info->cellw - 1);
    sv &= (info->cellh - 1);

    switch (info->colornumber) {
    case 0:
      baseaddr = baseaddr + (sh >> 1) + (sv * (info->cellw >> 1));
      for (j = 0; j < _Ygl->rwidth; j++)
      {
        u32 h = ((j*inch) >> 8);
        u32 addr = (baseaddr + (h >> 1));
        if (addr >= 0x80000) {
          *texture->textdata++ = 0x0000;
        }
        else {
          int cc = 1;
          u8 dot = Vdp2RamReadByte(NULL, Vdp2Ram, addr);
          u32 alpha = info->alpha;
          if (!(h & 0x01)) dot = dot >> 4;
          if (!(dot & 0xF) && info->transparencyenable) *texture->textdata++ = 0x00000000;
          else {
            color = (info->coloroffset + ((info->paladdr << 4) | (dot & 0xF)));
            switch (info->specialcolormode)
            {
            case 1: if (info->specialcolorfunction == 0) { cc = 0; } break;
            case 2:
              if (info->specialcolorfunction == 0) { cc = 0; }
              else { if ((info->specialcode & (1 << ((dot & 0xF) >> 1))) == 0) { cc = 0; } }
              break;
            case 3:
              if (((T2ReadWord(Vdp2ColorRam, (color << 1) & 0xFFF) & 0x8000) == 0)) { cc = 0; }
              break;
            }
            *texture->textdata++ = VDP2COLOR(info->idScreen, alpha, info->priority, cc, color);
          }
        }
      }
      break;
    case 1:
      baseaddr += sh + sv * info->cellw;

      for (j = 0; j < _Ygl->rwidth; j++)
      {
        int h = ((j*inch) >> 8);
        u32 alpha = info->alpha;
        u8 dot = Vdp2RamReadByte(NULL, Vdp2Ram, baseaddr + h);
        if (!dot && info->transparencyenable) {
          *texture->textdata++ = 0; continue;
        }
        else {
          int cc = 1;
          color = info->coloroffset + ((info->paladdr << 4) | (dot & 0xFF));
          switch (info->specialcolormode)
          {
          case 1: if (info->specialcolorfunction == 0) { cc = 0; } break;
          case 2:
            if (info->specialcolorfunction == 0) { cc = 0; }
            else { if ((info->specialcode & (1 << ((dot & 0xF) >> 1))) == 0) { cc = 0; } }
            break;
          case 3:
            if (((T2ReadWord(Vdp2ColorRam, (color << 1) & 0xFFF) & 0x8000) == 0)) { cc = 0; }
            break;
          }
          *texture->textdata++ = VDP2COLOR(info->idScreen, alpha, info->priority, cc, color);
        }
      }

      break;
    case 2:
      baseaddr += ((sh + sv * info->cellw) << 1);
      for (j = 0; j < _Ygl->rwidth; j++)
      {
        int h = ((j*inch) >> 8) << 1;
        *texture->textdata++ = Vdp2GetPixel16bpp(info, baseaddr + h, varVdp2Regs);

      }
      break;
    case 3:
      baseaddr += ((sh + sv * info->cellw) << 1);
      for (j = 0; j < _Ygl->rwidth; j++)
      {
        int h = ((j*inch) >> 8) << 1;
        *texture->textdata++ = Vdp2GetPixel16bppbmp(info, baseaddr + h, varVdp2Regs);
      }
      break;
    case 4:
      baseaddr += ((sh + sv * info->cellw) << 2);
      for (j = 0; j < _Ygl->rwidth; j++)
      {
        int h = (j*inch >> 8) << 2;
        *texture->textdata++ = Vdp2GetPixel32bppbmp(info, baseaddr + h, varVdp2Regs);
      }
      break;
    }
    texture->textdata += texture->w;
  }
}

static void Vdp2DrawPatternPos(vdp2draw_struct *info, YglTexture *texture, int x, int y, int cx, int cy, int lines, Vdp2 *varVdp2Regs)
{
  u64 cacheaddr = ((u32)(info->alpha >> 3) << 27) |
    (info->paladdr << 20) | info->charaddr | info->transparencyenable |
    ((info->patternpixelwh >> 4) << 1) | (((u64)(info->coloroffset >> 8) & 0x07) << 32) | (((u64)(info->idScreen) & 0x07) << 39);
  int priority = info->priority;
  YglCache c;
  vdp2draw_struct tile = *info;
  int winmode = 0;
  tile.dst = 0;
  tile.uclipmode = 0;
  tile.colornumber = info->colornumber;
  tile.blendmode = info->blendmode;
  tile.mosaicxmask = info->mosaicxmask;
  tile.mosaicymask = info->mosaicymask;
  tile.lineTexture = info->lineTexture;
  tile.idScreen = info->idScreen;

  tile.cellw = tile.cellh = info->patternpixelwh;
  tile.flipfunction = info->flipfunction;

  if (info->specialprimode == 1) {
    info->priority = (info->priority & 0xFFFFFFFE) | info->specialfunction;
  }

  cacheaddr |= ((u64)(info->priority) & 0x07) << 42;

  tile.priority = info->priority;

  tile.vertices[0] = x;
  tile.vertices[1] = y;
  tile.vertices[2] = (x + tile.cellw);
  tile.vertices[3] = y;
  tile.vertices[4] = (x + tile.cellh);
  tile.vertices[5] = (y + lines /*(float)info->lineinc*/);
  tile.vertices[6] = x;
  tile.vertices[7] = (y + lines/*(float)info->lineinc*/ );

  // Screen culling
  //if (tile.vertices[0] >= _Ygl->rwidth || tile.vertices[1] >= _Ygl->rheight || tile.vertices[2] < 0 || tile.vertices[5] < 0)
  //{
  //	return;
  //}

  if ((_Ygl->Win0[info->idScreen] != 0 || _Ygl->Win1[info->idScreen] != 0) && info->coordincx == 1.0f && info->coordincy == 1.0f)
  {                                                 // coordinate inc is not supported yet.
    winmode = Vdp2CheckWindowRange(info, x - cx, y - cy, tile.cellw, info->lineinc, varVdp2Regs);
    if (winmode == 0) // all outside, no need to draw
    {
      return;
    }
  }

  tile.cor = info->cor;
  tile.cog = info->cog;
  tile.cob = info->cob;

  if (1 == YglIsCached(YglTM_vdp2, cacheaddr, &c))
  {
    YglCachedQuadOffset(&tile, &c, cx, cy, info->coordincx, info->coordincy, YglTM_vdp2);
    return;
  }

  YglQuadOffset(&tile, texture, &c, cx, cy, info->coordincx, info->coordincy, YglTM_vdp2);
  YglCacheAdd(YglTM_vdp2, cacheaddr, &c);
  switch (info->patternwh)
  {
  case 1:
    requestDrawCell(info, texture, varVdp2Regs);
    break;
  case 2:
    texture->w += 8;
    requestDrawCellQuad(info, texture, varVdp2Regs);
    break;
  }
  info->priority = priority;
}


//////////////////////////////////////////////////////////////////////////////

static void Vdp2PatternAddrUsingPatternname(vdp2draw_struct *info, u16 paternname, Vdp2 *varVdp2Regs )
{
    u16 tmp = paternname;
    info->specialfunction = (info->supplementdata >> 9) & 0x1;
    info->specialcolorfunction = (info->supplementdata >> 8) & 0x1;

    switch (info->colornumber)
    {
    case 0: // in 16 colors
      info->paladdr = ((tmp & 0xF000) >> 12) | ((info->supplementdata & 0xE0) >> 1);
      break;
    default: // not in 16 colors
      info->paladdr = (tmp & 0x7000) >> 8;
      break;
    }

    switch (info->auxmode)
    {
    case 0:
      info->flipfunction = (tmp & 0xC00) >> 10;

      switch (info->patternwh)
      {
      case 1:
        info->charaddr = (tmp & 0x3FF) | ((info->supplementdata & 0x1F) << 10);
        break;
      case 2:
        info->charaddr = ((tmp & 0x3FF) << 2) | (info->supplementdata & 0x3) | ((info->supplementdata & 0x1C) << 10);
        break;
      }
      break;
    case 1:
      info->flipfunction = 0;

      switch (info->patternwh)
      {
      case 1:
        info->charaddr = (tmp & 0xFFF) | ((info->supplementdata & 0x1C) << 10);
        break;
      case 2:
        info->charaddr = ((tmp & 0xFFF) << 2) | (info->supplementdata & 0x3) | ((info->supplementdata & 0x10) << 10);
        break;
      }
      break;
    }

  if (!(varVdp2Regs->VRSIZE & 0x8000))
    info->charaddr &= 0x3FFF;

  info->charaddr *= 0x20; // thanks Runik
}

static void Vdp2PatternAddr(vdp2draw_struct *info, Vdp2 *varVdp2Regs)
{
  info->addr &= 0x7FFFF;
  switch (info->patterndatasize)
  {
  case 1:
  {
    u16 tmp = Vdp2RamReadWord(NULL, Vdp2Ram, info->addr);

    info->addr += 2;
    info->specialfunction = (info->supplementdata >> 9) & 0x1;
    info->specialcolorfunction = (info->supplementdata >> 8) & 0x1;

    switch (info->colornumber)
    {
    case 0: // in 16 colors
      info->paladdr = ((tmp & 0xF000) >> 12) | ((info->supplementdata & 0xE0) >> 1);
      break;
    default: // not in 16 colors
      info->paladdr = (tmp & 0x7000) >> 8;
      break;
    }

    switch (info->auxmode)
    {
    case 0:
      info->flipfunction = (tmp & 0xC00) >> 10;

      switch (info->patternwh)
      {
      case 1:
        info->charaddr = (tmp & 0x3FF) | ((info->supplementdata & 0x1F) << 10);
        break;
      case 2:
        info->charaddr = ((tmp & 0x3FF) << 2) | (info->supplementdata & 0x3) | ((info->supplementdata & 0x1C) << 10);
        break;
      }
      break;
    case 1:
      info->flipfunction = 0;

      switch (info->patternwh)
      {
      case 1:
        info->charaddr = (tmp & 0xFFF) | ((info->supplementdata & 0x1C) << 10);
        break;
      case 2:
        info->charaddr = ((tmp & 0xFFF) << 2) | (info->supplementdata & 0x3) | ((info->supplementdata & 0x10) << 10);
        break;
      }
      break;
    }

    break;
  }
  case 2: {
    u16 tmp1 = Vdp2RamReadWord(NULL, Vdp2Ram, info->addr);
    u16 tmp2 = Vdp2RamReadWord(NULL, Vdp2Ram, info->addr + 2);
    info->addr += 4;
    info->charaddr = tmp2 & 0x7FFF;
    info->flipfunction = (tmp1 & 0xC000) >> 14;
    switch (info->colornumber) {
    case 0:
      info->paladdr = (tmp1 & 0x7F);
      break;
    default:
      info->paladdr = (tmp1 & 0x70);
      break;
    }
    info->specialfunction = (tmp1 & 0x2000) >> 13;
    info->specialcolorfunction = (tmp1 & 0x1000) >> 12;
    break;
  }
  }

  if (!(varVdp2Regs->VRSIZE & 0x8000))
    info->charaddr &= 0x3FFF;

  info->charaddr *= 0x20; // thanks Runik
}


static void Vdp2PatternAddrPos(vdp2draw_struct *info, int planex, int x, int planey, int y, Vdp2 *varVdp2Regs)
{
  u32 addr = info->addr +
    (info->pagewh*info->pagewh*info->planew*planey +
      info->pagewh*info->pagewh*planex +
      info->pagewh*y +
      x)*info->patterndatasize * 2;

  switch (info->patterndatasize)
  {
  case 1:
  {
    u16 tmp = Vdp2RamReadWord(NULL, Vdp2Ram, addr);

    info->specialfunction = (info->supplementdata >> 9) & 0x1;
    info->specialcolorfunction = (info->supplementdata >> 8) & 0x1;

    switch (info->colornumber)
    {
    case 0: // in 16 colors
      info->paladdr = ((tmp & 0xF000) >> 12) | ((info->supplementdata & 0xE0) >> 1);
      break;
    default: // not in 16 colors
      info->paladdr = (tmp & 0x7000) >> 8;
      break;
    }

    switch (info->auxmode)
    {
    case 0:
      info->flipfunction = (tmp & 0xC00) >> 10;

      switch (info->patternwh)
      {
      case 1:
        info->charaddr = (tmp & 0x3FF) | ((info->supplementdata & 0x1F) << 10);
        break;
      case 2:
        info->charaddr = ((tmp & 0x3FF) << 2) | (info->supplementdata & 0x3) | ((info->supplementdata & 0x1C) << 10);
        break;
      }
      break;
    case 1:
      info->flipfunction = 0;

      switch (info->patternwh)
      {
      case 1:
        info->charaddr = (tmp & 0xFFF) | ((info->supplementdata & 0x1C) << 10);
        break;
      case 2:
        info->charaddr = ((tmp & 0xFFF) << 2) | (info->supplementdata & 0x3) | ((info->supplementdata & 0x10) << 10);
        break;
      }
      break;
    }

    break;
  }
  case 2: {
    u16 tmp1 = Vdp2RamReadWord(NULL, Vdp2Ram, addr);
    u16 tmp2 = Vdp2RamReadWord(NULL, Vdp2Ram, addr + 2);
    info->charaddr = tmp2 & 0x7FFF;
    info->flipfunction = (tmp1 & 0xC000) >> 14;
    switch (info->colornumber) {
    case 0:
      info->paladdr = (tmp1 & 0x7F);
      break;
    default:
      info->paladdr = (tmp1 & 0x70);
      break;
    }
    info->specialfunction = (tmp1 & 0x2000) >> 13;
    info->specialcolorfunction = (tmp1 & 0x1000) >> 12;
    break;
  }
  }

  if (!(varVdp2Regs->VRSIZE & 0x8000))
    info->charaddr &= 0x3FFF;

  info->charaddr *= 0x20; // thanks Runik
}



//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawMapPerLine(vdp2draw_struct *info, YglTexture *texture, Vdp2 *varVdp2Regs) {

  int lineindex = 0;

  int sx; //, sy;
  int mapx, mapy;
  int planex, planey;
  int pagex, pagey;
  int charx, chary;
  int dot_on_planey;
  int dot_on_pagey;
  int dot_on_planex;
  int dot_on_pagex;
  int h, v;
  const int planeh_shift = 9 + (info->planeh - 1);
  const int planew_shift = 9 + (info->planew - 1);
  const int plane_shift = 9;
  const int plane_mask = 0x1FF;
  const int page_shift = 9 - 7 + (64 / info->pagewh);
  const int page_mask = 0x0f >> ((info->pagewh / 32) - 1);

  info->patternpixelwh = 8 * info->patternwh;
  info->draww = (int)((float)_Ygl->rwidth / info->coordincx);
  info->drawh = (int)((float)_Ygl->rheight / info->coordincy);

  int res_shift = 0;
  if (_Ygl->rheight >= 440) res_shift = 1;

  int linemask = 0;
  switch (info->lineinc) {
  case 1:
    linemask = 0;
    break;
  case 2:
    linemask = 0x01;
    break;
  case 4:
    linemask = 0x03;
    break;
  case 8:
    linemask = 0x07;
    break;
  }

  for (v = 0; v < info->drawh; v += 1) {  // ToDo: info->coordincy
    int targetv = 0;
    sx = info->x + info->lineinfo[(int)(lineindex*info->coordincy)].LineScrollValH;
    if (VDPLINE_SY(info->islinescroll)) {
      targetv = info->y + (v & linemask) + info->lineinfo[(int)(lineindex*info->coordincy)].LineScrollValV;
    }
    else {
      targetv = info->y + v;
    }

    if (info->isverticalscroll) {
      // this is *wrong*, vertical scroll use a different value per cell
      // info->verticalscrolltbl should be incremented by info->verticalscrollinc
      // each time there's a cell change and reseted at the end of the line...
      // or something like that :)
		//On ne peut pas decaler de 16 un word de 16 bits...
      targetv += Vdp2RamReadWord(NULL, Vdp2Ram, info->verticalscrolltbl) >> 16;
    }

    if (VDPLINE_SZ(info->islinescroll)) {
      info->coordincx = info->lineinfo[(int)(lineindex*info->coordincy)].CoordinateIncH / 256.0f;
      if (info->coordincx == 0) {
        info->coordincx = _Ygl->rwidth;;
      }
      else {
        info->coordincx = 1.0f / info->coordincx;
      }
    }

    if (info->coordincx < info->maxzoom) info->coordincx = info->maxzoom;
    info->draww = (int)((float)_Ygl->rwidth / info->coordincx);

    // determine which chara shoud be used.
    //mapy   = (v+sy) / (512 * info->planeh);
    mapy = (targetv) >> planeh_shift;
    //int dot_on_planey = (v + sy) - mapy*(512 * info->planeh);
    dot_on_planey = (targetv)-(mapy << planeh_shift);
    mapy = mapy & 0x01;
    //planey = dot_on_planey / 512;
    planey = dot_on_planey >> plane_shift;
    //int dot_on_pagey = dot_on_planey - planey * 512;
    dot_on_pagey = dot_on_planey & plane_mask;
    planey = planey & (info->planeh - 1);
    //pagey = dot_on_pagey / (512 / info->pagewh);
    pagey = dot_on_pagey >> page_shift;
    //chary = dot_on_pagey - pagey*(512 / info->pagewh);
    chary = dot_on_pagey & page_mask;
    if (pagey < 0) pagey = info->pagewh - 1 + pagey;

    for (h = -info->patternpixelwh; h < info->draww + info->patternpixelwh; h += info->patternpixelwh) {

      //mapx = (h + sx) / (512 * info->planew);
      mapx = (h + sx) >> planew_shift;
      //int dot_on_planex = (h + sx) - mapx*(512 * info->planew);
      dot_on_planex = (h + sx) - (mapx << planew_shift);
      mapx = mapx & 0x01;
      //planex = dot_on_planex / 512;
      planex = dot_on_planex >> plane_shift;
      //int dot_on_pagex = dot_on_planex - planex * 512;
      dot_on_pagex = dot_on_planex & plane_mask;
      planex = planex & (info->planew - 1);
      //pagex = dot_on_pagex / (512 / info->pagewh);
      pagex = dot_on_pagex >> page_shift;
      //charx = dot_on_pagex - pagex*(512 / info->pagewh);
      charx = dot_on_pagex & page_mask;
      if (pagex < 0) pagex = info->pagewh - 1 + pagex;

      info->PlaneAddr(info, info->mapwh * mapy + mapx, varVdp2Regs);
      Vdp2PatternAddrPos(info, planex, pagex, planey, pagey, varVdp2Regs);
      Vdp2DrawPatternPos(info, texture, h, v, charx, chary, 1, varVdp2Regs);

    }
    if((v & linemask) == linemask) lineindex++;
  }

}

static void Vdp2DrawMapTest(vdp2draw_struct *info, YglTexture *texture, Vdp2 *varVdp2Regs) {

  int lineindex = 0;

  int sx; //, sy;
  int mapx, mapy;
  int planex, planey;
  int pagex, pagey;
  int charx, chary;
  int dot_on_planey;
  int dot_on_pagey;
  int dot_on_planex;
  int dot_on_pagex;
  int h, v;
  int cell_count = 0;

  const int planeh_shift = 9 + (info->planeh - 1);
  const int planew_shift = 9 + (info->planew - 1);
  const int plane_shift = 9;
  const int plane_mask = 0x1FF;
  const int page_shift = 9 - 7 + (64 / info->pagewh);
  const int page_mask = 0x0f >> ((info->pagewh / 32) - 1);

  info->patternpixelwh = 8 * info->patternwh;
  info->draww = (int)((float)_Ygl->rwidth / info->coordincx);
  info->drawh = (int)((float)_Ygl->rheight / info->coordincy);
  info->lineinc = info->patternpixelwh;

  //info->coordincx = 1.0f;

  for (v = -info->patternpixelwh; v < info->drawh + info->patternpixelwh; v += info->patternpixelwh) {
    int targetv = 0;
    sx = info->x;

    if (!info->isverticalscroll) {
      targetv = info->y + v;
      // determine which chara shoud be used.
      //mapy   = (v+sy) / (512 * info->planeh);
      mapy = (targetv) >> planeh_shift;
      //int dot_on_planey = (v + sy) - mapy*(512 * info->planeh);
      dot_on_planey = (targetv)-(mapy << planeh_shift);
      mapy = mapy & 0x01;
      //planey = dot_on_planey / 512;
      planey = dot_on_planey >> plane_shift;
      //int dot_on_pagey = dot_on_planey - planey * 512;
      dot_on_pagey = dot_on_planey & plane_mask;
      planey = planey & (info->planeh - 1);
      //pagey = dot_on_pagey / (512 / info->pagewh);
      pagey = dot_on_pagey >> page_shift;
      //chary = dot_on_pagey - pagey*(512 / info->pagewh);
      chary = dot_on_pagey & page_mask;
      if (pagey < 0) pagey = info->pagewh - 1 + pagey;
    }
    else {
      cell_count = 0;
    }

    for (h = -info->patternpixelwh; h < info->draww + info->patternpixelwh; h += info->patternpixelwh) {

      if (info->isverticalscroll) {
        targetv = info->y + v + (Vdp2RamReadLong(NULL, Vdp2Ram, info->verticalscrolltbl + cell_count) >> 16);
        cell_count += info->verticalscrollinc;
        // determine which chara shoud be used.
        //mapy   = (v+sy) / (512 * info->planeh);
        mapy = (targetv) >> planeh_shift;
        //int dot_on_planey = (v + sy) - mapy*(512 * info->planeh);
        dot_on_planey = (targetv)-(mapy << planeh_shift);
        mapy = mapy & 0x01;
        //planey = dot_on_planey / 512;
        planey = dot_on_planey >> plane_shift;
        //int dot_on_pagey = dot_on_planey - planey * 512;
        dot_on_pagey = dot_on_planey & plane_mask;
        planey = planey & (info->planeh - 1);
        //pagey = dot_on_pagey / (512 / info->pagewh);
        pagey = dot_on_pagey >> page_shift;
        //chary = dot_on_pagey - pagey*(512 / info->pagewh);
        chary = dot_on_pagey & page_mask;
        if (pagey < 0) pagey = info->pagewh - 1 + pagey;
      }

      //mapx = (h + sx) / (512 * info->planew);
      mapx = (h + sx) >> planew_shift;
      //int dot_on_planex = (h + sx) - mapx*(512 * info->planew);
      dot_on_planex = (h + sx) - (mapx << planew_shift);
      mapx = mapx & 0x01;
      //planex = dot_on_planex / 512;
      planex = dot_on_planex >> plane_shift;
      //int dot_on_pagex = dot_on_planex - planex * 512;
      dot_on_pagex = dot_on_planex & plane_mask;
      planex = planex & (info->planew - 1);
      //pagex = dot_on_pagex / (512 / info->pagewh);
      pagex = dot_on_pagex >> page_shift;
      //charx = dot_on_pagex - pagex*(512 / info->pagewh);
      charx = dot_on_pagex & page_mask;
      if (pagex < 0) pagex = info->pagewh - 1 + pagex;

      info->PlaneAddr(info, info->mapwh * mapy + mapx, varVdp2Regs);
      Vdp2PatternAddrPos(info, planex, pagex, planey, pagey, varVdp2Regs);
      Vdp2DrawPatternPos(info, texture, h - charx, v - chary, 0, 0, info->lineinc, varVdp2Regs);
    }

    lineindex++;
  }

}

//////////////////////////////////////////////////////////////////////////////

static u32 FASTCALL DoNothing(UNUSED void *info, u32 pixel)
{
  return pixel;
}

//////////////////////////////////////////////////////////////////////////////

static u32 FASTCALL DoColorOffset(void *info, u32 pixel)
{
  return pixel;
}

#if 0
static u32 FASTCALL DoColorOffset(void *info, u32 pixel)
{
  return COLOR_ADD(pixel, ((vdp2draw_struct *)info)->cor,
    ((vdp2draw_struct *)info)->cog,
    ((vdp2draw_struct *)info)->cob);
}
#endif



//////////////////////////////////////////////////////////////////////////////

static INLINE void ReadVdp2ColorOffset(Vdp2 * regs, vdp2draw_struct *info, int mask)
{
  if (regs->CLOFEN & mask)
  {
    // color offset enable
    if (regs->CLOFSL & mask)
    {
      // color offset B
      info->cor = regs->COBR & 0xFF;
      if (regs->COBR & 0x100)
        info->cor |= 0xFFFFFF00;

      info->cog = regs->COBG & 0xFF;
      if (regs->COBG & 0x100)
        info->cog |= 0xFFFFFF00;

      info->cob = regs->COBB & 0xFF;
      if (regs->COBB & 0x100)
        info->cob |= 0xFFFFFF00;
    }
    else
    {
      // color offset A
      info->cor = regs->COAR & 0xFF;
      if (regs->COAR & 0x100)
        info->cor |= 0xFFFFFF00;

      info->cog = regs->COAG & 0xFF;
      if (regs->COAG & 0x100)
        info->cog |= 0xFFFFFF00;

      info->cob = regs->COAB & 0xFF;
      if (regs->COAB & 0x100)
        info->cob |= 0xFFFFFF00;
    }
    info->PostPixelFetchCalc = &DoColorOffset;
  }
  else { // color offset disable

    info->PostPixelFetchCalc = &DoNothing;
    info->cor = 0;
    info->cob = 0;
    info->cog = 0;

  }
}

//////////////////////////////////////////////////////////////////////////////

static INLINE u32 Vdp2RotationFetchPixel(vdp2draw_struct *info, int x, int y, int cellw)
{
  u32 dot;
  u32 cramindex;
  u32 alpha = info->alpha;
  u8 lowdot = 0x00;
  switch (info->colornumber)
  {
  case 0: // 4 BPP
    dot = Vdp2RamReadByte(NULL, Vdp2Ram, (info->charaddr + (((y * cellw) + x) >> 1) ));
    if (!(x & 0x1)) dot >>= 4;
    if (!(dot & 0xF) && info->transparencyenable) return 0x00000000;
    else {
      int cc = 1;
      cramindex = (info->coloroffset + ((info->paladdr << 4) | (dot & 0xF)));
      switch (info->specialcolormode)
      {
      case 1: if (info->specialcolorfunction == 0) { cc = 0; } break;
      case 2:
        if (info->specialcolorfunction == 0) { cc = 0; }
        else { if ((info->specialcode & (1 << ((dot & 0xF) >> 1))) == 0) { cc = 0; } }
        break;
      case 3:
        if (((T2ReadWord(Vdp2ColorRam, (cramindex << 1) & 0xFFF) & 0x8000) == 0)) { cc = 0; }
        break;
      }
      return   VDP2COLOR(info->idScreen, alpha, info->priority, cc, cramindex);
    }
  case 1: // 8 BPP
    dot = Vdp2RamReadByte(NULL, Vdp2Ram, (info->charaddr + (y * cellw) + x));
    if (!(dot & 0xFF) && info->transparencyenable) return 0x00000000;
    else {
      int cc = 1;
      cramindex = info->coloroffset + ((info->paladdr << 4) | (dot & 0xFF));
      switch (info->specialcolormode)
      {
      case 1: if (info->specialcolorfunction == 0) { cc = 0; } break;
      case 2:
        if (info->specialcolorfunction == 0) { cc = 0; }
        else { if ((info->specialcode & (1 << ((dot & 0xF) >> 1))) == 0) { cc = 0; } }
        break;
      case 3:
        if (((T2ReadWord(Vdp2ColorRam, (cramindex << 1) & 0xFFF) & 0x8000) == 0)) { cc = 0; }
        break;
      }
      return   VDP2COLOR(info->idScreen, alpha, info->priority, cc, cramindex);
    }
  case 2: // 16 BPP(palette)
    dot = Vdp2RamReadWord(NULL, Vdp2Ram, (info->charaddr + ((y * cellw) + x) * 2));
    if ((dot == 0) && info->transparencyenable) return 0x00000000;
    else {
      int cc = 1;
      cramindex = (info->coloroffset + dot);
      switch (info->specialcolormode)
      {
      case 1: if (info->specialcolorfunction == 0) { cc = 0; } break;
      case 2:
        if (info->specialcolorfunction == 0) { cc = 0; }
        else { if ((info->specialcode & (1 << ((dot & 0xF) >> 1))) == 0) { cc = 0; } }
        break;
      case 3:
        if (((T2ReadWord(Vdp2ColorRam, (cramindex << 1) & 0xFFF) & 0x8000) == 0)) { cc = 0; }
        break;
      }
      return   VDP2COLOR(info->idScreen, alpha, info->priority, cc, cramindex);
    }
  case 3: // 16 BPP(RGB)
    dot = Vdp2RamReadWord(NULL, Vdp2Ram, (info->charaddr + ((y * cellw) + x) * 2));
    if (!(dot & 0x8000) && info->transparencyenable) return 0x00000000;
    else return VDP2COLOR(info->idScreen, info->alpha, info->priority, 1, RGB555_TO_RGB24(dot & 0xFFFF));
  case 4: // 32 BPP
    dot = Vdp2RamReadLong(NULL, Vdp2Ram, (info->charaddr + ((y * cellw) + x) * 4));
    if (!(dot & 0x80000000) && info->transparencyenable) return 0x00000000;
    else return VDP2COLOR(info->idScreen, info->alpha, info->priority, 1, dot & 0xFFFFFF);
  default:
    return 0;
  }
}

#define RBG_IDLE -1
#define RBG_REQ_RENDER 0
#define RBG_FINIESED 1
#define RBG_TEXTURE_SYNCED 2



/*------------------------------------------------------------------------------
 Rotate Screen drawing
 ------------------------------------------------------------------------------*/
static void FASTCALL Vdp2DrawRotation(RBGDrawInfo * rbg, Vdp2 *varVdp2Regs)
{
  vdp2draw_struct *info = &rbg->info;
  int rgb_type = rbg->rgb_type;
  YglTexture *texture = &rbg->texture;
  YglTexture *line_texture = &rbg->line_texture;

  int x, y;
  int cellw, cellh;
  int oldcellx = -1, oldcelly = -1;
  int lineInc = varVdp2Regs->LCTA.part.U & 0x8000 ? 2 : 0;
  int linecl = 0xFF;
  Vdp2 * regs;
  int screenHeight = _Ygl->rheight;
  int screenWidth  = _Ygl->rwidth;

  if ((varVdp2Regs->CCCTL >> 5) & 0x01) {
    linecl = ((~varVdp2Regs->CCRLB & 0x1F) << 3);
  }

    if (_Ygl->rheight >= 448) lineInc <<= 1;
    if (_Ygl->rheight >= 448) rbg->vres = (_Ygl->rheight >> 1); else rbg->vres = _Ygl->rheight;
    if (_Ygl->rwidth >= 640) rbg->hres = (_Ygl->rwidth >> 1); else rbg->hres = _Ygl->rwidth;


  if (rbg->use_cs) {
    rbg->hres *= _Ygl->widthRatio;
    rbg->vres *= _Ygl->heightRatio;

    RBGGenerator_init(_Ygl->width, _Ygl->height);
  }

  info->vertices[0] = 0;
  info->vertices[1] = (screenHeight * info->startLine)/yabsys.VBlankLineCount;
  info->vertices[2] = screenWidth;
  info->vertices[3] = (screenHeight * info->startLine)/yabsys.VBlankLineCount;
  info->vertices[4] = screenWidth;
  info->vertices[5] = (screenHeight * info->endLine)/yabsys.VBlankLineCount;
  info->vertices[6] = 0;
  info->vertices[7] = (screenHeight * info->endLine)/yabsys.VBlankLineCount;
  cellw = info->cellw;
  cellh = info->cellh;
  info->cellw = rbg->hres;
  info->cellh = rbg->vres;
  info->flipfunction = 0;
  info->cor = 0x00;
  info->cog = 0x00;
  info->cob = 0x00;

  if (varVdp2Regs->RPMD != 0) rbg->useb = 1;
  if (rgb_type == 0x04) rbg->useb = 1;

  if (!info->isbitmap)
  {
    oldcellx = -1;
    oldcelly = -1;
    rbg->pagesize = info->pagewh*info->pagewh;
    rbg->patternshift = (2 + info->patternwh);
  }
  else
  {
    oldcellx = 0;
    oldcelly = 0;
    rbg->pagesize = 0;
    rbg->patternshift = 0;
  }

  regs = Vdp2RestoreRegs(info->startLine, Vdp2Lines);
  if (regs) ReadVdp2ColorOffset(regs, info, info->linecheck_mask);

  if (info->lineTexture != 0) {
     info->cor = 0;
     info->cog = 0;
     info->cob = 0;
   }

    u64 cacheaddr = 0x90000000BAD;

    rbg->vdp2_sync_flg = -1;
    if (!rbg->use_cs) {
      YglTMAllocate(YglTM_vdp2, &rbg->texture, info->cellw, info->cellh, &x, &y);
      rbg->c.x = x;
      rbg->c.y = y;
      YglCacheAdd(YglTM_vdp2, cacheaddr, &rbg->c);
    }
    info->cellw = cellw;
    info->cellh = cellh;

  rbg->line_texture.textdata = NULL;
  if (info->LineColorBase != 0)
  {
    rbg->line_info.blendmode = 0;
    rbg->LineColorRamAdress = (Vdp2RamReadWord(NULL, Vdp2Ram, info->LineColorBase) & 0x7FF);// +info->coloroffset;

    u64 cacheaddr = 0xA0000000DAD;
    YglTMAllocate(YglTM_vdp2, &rbg->line_texture, rbg->vres, 1,  &x, &y);
    rbg->cline.x = x;
    rbg->cline.y = y;
    YglCacheAdd(YglTM_vdp2, cacheaddr, &rbg->cline);

  }
  else {
    rbg->LineColorRamAdress = 0x00;
    rbg->cline.x = -1;
    rbg->cline.y = -1;
    rbg->line_texture.textdata = NULL;
    rbg->line_texture.w = 0;
  }
#ifdef RGB_ASYNC
  if ((rgb_type == 0x0) && (rbg->use_cs == 0)) Vdp2DrawRotation_in(rbg, varVdp2Regs);
  else
#endif
  {
    Vdp2DrawRotation_in_sync(rbg, varVdp2Regs);
    if (!rbg->use_cs) {
      YglQuadRbg0(rbg, NULL, &rbg->c, &rbg->cline, rbg->rgb_type, YglTM_vdp2, NULL);
    }
  }
}


#ifdef RGB_ASYNC
static void finishRbgQueue() {
  while (YaGetQueueSize(rotq_end_task)!=0)
  {
    RBGDrawInfo *rbg = (RBGDrawInfo *) YabWaitEventQueue(rotq_end_task);
    YglQuadRbg0(rbg, NULL, &rbg->c, &rbg->cline, rbg->rgb_type, YglTM_vdp2, NULL);
    free(rbg);
  }
}
#endif

#define ceilf(a) ((a)+0.99999f)

static INLINE int vdp2rGetKValue(vdp2rotationparameter_struct * parameter, int i) {
  float kval;
  int   kdata;
  int h = ceilf(parameter->KtablV + (parameter->deltaKAx * i));
  if (parameter->coefdatasize == 2) {
    if (parameter->k_mem_type == 0) { // vram
      kdata = Vdp2RamReadWord(NULL, Vdp2Ram, (parameter->coeftbladdr + (h << 1)));
    } else { // cram
      kdata = Vdp2ColorRamReadWord(NULL, Vdp2Ram,((parameter->coeftbladdr + (int)(h << 1)) & 0x7FF) + 0x800);
    }
    if (kdata & 0x8000) { return 0; }
    kval = (float)(signed)((kdata & 0x7FFF) | (kdata & 0x4000 ? 0x8000 : 0x0000)) / 1024.0f;
    switch (parameter->coefmode) {
    case 0:  parameter->kx = kval; parameter->ky = kval; break;
    case 1:  parameter->kx = kval; break;
    case 2:  parameter->ky = kval; break;
    case 3:  /*ToDo*/  break;
    }
  }
  else {
    if (parameter->k_mem_type == 0) { // vram
      kdata = Vdp2RamReadLong(NULL, Vdp2Ram, (parameter->coeftbladdr + (h << 2)) & 0x7FFFF);
    } else { // cram
      kdata = Vdp2ColorRamReadLong(NULL, Vdp2Ram, ((parameter->coeftbladdr + (int)(h << 2)) & 0x7FF) + 0x800 );
    }
    parameter->lineaddr = (kdata >> 24) & 0x7F;
    if (kdata & 0x80000000) { return 0; }
    kval = (float)(int)((kdata & 0x00FFFFFF) | (kdata & 0x00800000 ? 0xFF800000 : 0x00000000)) / 65536.0f;
    switch (parameter->coefmode) {
    case 0:  parameter->kx = kval; parameter->ky = kval; break;
    case 1:  parameter->kx = kval; break;
    case 2:  parameter->ky = kval; break;
    case 3:  /*ToDo*/  break;
    }
  }
  return 1;
}

static void Vdp2DrawRotation_in_sync(RBGDrawInfo * rbg, Vdp2 *varVdp2Regs) {

  if (rbg == NULL) return;

  vdp2draw_struct *info = &rbg->info;
  int rgb_type = rbg->rgb_type;
  YglTexture *texture = &rbg->texture;
  YglTexture *line_texture = &rbg->line_texture;

  float i, j;
  int k,l;
  int x, y;
  int cellw, cellh;
  int oldcellx = -1, oldcelly = -1;
  u32 color;
  int vres, hres, vstart;
  int h;
  int v;
  int lineInc = varVdp2Regs->LCTA.part.U & 0x8000 ? 2 : 0;
  int linecl = 0xFF;
  vdp2rotationparameter_struct *parameter;
  Vdp2 * regs;
  if ((varVdp2Regs->CCCTL >> 5) & 0x01) {
    linecl = ((~varVdp2Regs->CCRLB & 0x1F) << 3);
  }

  if (_Ygl->rheight >= 448) lineInc <<= 1;
  vres = rbg->vres * (rbg->info.endLine - rbg->info.startLine)/yabsys.VBlankLineCount;
  vstart = rbg->vres * rbg->info.startLine/yabsys.VBlankLineCount;
  hres = rbg->hres;

  cellw = rbg->info.cellw;
  cellh = rbg->info.cellh;

  x = 0;
  y = 0;
  u32 lineaddr = 0;
  if (rgb_type == 0)
  {
    rbg->paraA.dx = rbg->paraA.A * rbg->paraA.deltaX + rbg->paraA.B * rbg->paraA.deltaY;
    rbg->paraA.dy = rbg->paraA.D * rbg->paraA.deltaX + rbg->paraA.E * rbg->paraA.deltaY;
    rbg->paraA.Xp = rbg->paraA.A * (rbg->paraA.Px - rbg->paraA.Cx) +
      rbg->paraA.B * (rbg->paraA.Py - rbg->paraA.Cy) +
      rbg->paraA.C * (rbg->paraA.Pz - rbg->paraA.Cz) + rbg->paraA.Cx + rbg->paraA.Mx;
    rbg->paraA.Yp = rbg->paraA.D * (rbg->paraA.Px - rbg->paraA.Cx) +
      rbg->paraA.E * (rbg->paraA.Py - rbg->paraA.Cy) +
      rbg->paraA.F * (rbg->paraA.Pz - rbg->paraA.Cz) + rbg->paraA.Cy + rbg->paraA.My;
  }

  if (rbg->useb)
  {
    rbg->paraB.dx = rbg->paraB.A * rbg->paraB.deltaX + rbg->paraB.B * rbg->paraB.deltaY;
    rbg->paraB.dy = rbg->paraB.D * rbg->paraB.deltaX + rbg->paraB.E * rbg->paraB.deltaY;
    rbg->paraB.Xp = rbg->paraB.A * (rbg->paraB.Px - rbg->paraB.Cx) + rbg->paraB.B * (rbg->paraB.Py - rbg->paraB.Cy)
      + rbg->paraB.C * (rbg->paraB.Pz - rbg->paraB.Cz) + rbg->paraB.Cx + rbg->paraB.Mx;
    rbg->paraB.Yp = rbg->paraB.D * (rbg->paraB.Px - rbg->paraB.Cx) + rbg->paraB.E * (rbg->paraB.Py - rbg->paraB.Cy)
      + rbg->paraB.F * (rbg->paraB.Pz - rbg->paraB.Cz) + rbg->paraB.Cy + rbg->paraB.My;
  }

  rbg->paraA.over_pattern_name = varVdp2Regs->OVPNRA;
  rbg->paraB.over_pattern_name = varVdp2Regs->OVPNRB;


  rbg->info.cellw = rbg->hres;
  rbg->info.cellh = (rbg->vres * (info->endLine - info->startLine))/yabsys.VBlankLineCount;
  rbg->info.celly = (rbg->vres * info->startLine)/yabsys.VBlankLineCount;

  if (rbg->use_cs) {

    vres /= _Ygl->heightRatio;
    vstart /= _Ygl->heightRatio;
    hres /= _Ygl->widthRatio;

	  if (info->LineColorBase != 0) {
		  const float vstep = 1.0 / _Ygl->heightRatio;
		  j = 0.0f;
      int lvres = rbg->vres;
      if (vres >= 480) {
        lvres >>= 1;
      }
		  for (int jj = 0; jj < lvres; jj++) {
			  if ((varVdp2Regs->LCTA.part.U & 0x8000) != 0) {
				  rbg->LineColorRamAdress = T1ReadWord(Vdp2Ram, info->LineColorBase + lineInc*(int)(j));
				  *line_texture->textdata = rbg->LineColorRamAdress | (linecl << 24);
				  line_texture->textdata++;
          if (vres >= 480) {
            *line_texture->textdata = rbg->LineColorRamAdress | (linecl << 24);
            line_texture->textdata++;
          }
			  }
			  else {
				  *line_texture->textdata = rbg->LineColorRamAdress;
				  line_texture->textdata++;
			  }
			  j += vstep;
		  }
	  }
    YglQuadRbg0(rbg, NULL, &rbg->c, &rbg->cline, rbg->rgb_type, YglTM_vdp2, varVdp2Regs);
	  return;
  }

  for (k = vstart; k < vstart+vres; k++)
  {

    if (rgb_type == 0) {
      rbg->paraA.Xsp = rbg->paraA.A * ((rbg->paraA.Xst + rbg->paraA.deltaXst * k) - rbg->paraA.Px) +
        rbg->paraA.B * ((rbg->paraA.Yst + rbg->paraA.deltaYst * k) - rbg->paraA.Py) +
        rbg->paraA.C * (rbg->paraA.Zst - rbg->paraA.Pz);

      rbg->paraA.Ysp = rbg->paraA.D * ((rbg->paraA.Xst + rbg->paraA.deltaXst *k) - rbg->paraA.Px) +
        rbg->paraA.E * ((rbg->paraA.Yst + rbg->paraA.deltaYst * k) - rbg->paraA.Py) +
        rbg->paraA.F * (rbg->paraA.Zst - rbg->paraA.Pz);

      rbg->paraA.KtablV = rbg->paraA.deltaKAst* k;
    }
    if (rbg->useb)
    {
      rbg->paraB.Xsp = rbg->paraB.A * ((rbg->paraB.Xst + rbg->paraB.deltaXst * k) - rbg->paraB.Px) +
        rbg->paraB.B * ((rbg->paraB.Yst + rbg->paraB.deltaYst * k) - rbg->paraB.Py) +
        rbg->paraB.C * (rbg->paraB.Zst - rbg->paraB.Pz);

      rbg->paraB.Ysp = rbg->paraB.D * ((rbg->paraB.Xst + rbg->paraB.deltaXst * k) - rbg->paraB.Px) +
        rbg->paraB.E * ((rbg->paraB.Yst + rbg->paraB.deltaYst * k) - rbg->paraB.Py) +
        rbg->paraB.F * (rbg->paraB.Zst - rbg->paraB.Pz);

      rbg->paraB.KtablV = rbg->paraB.deltaKAst * k;
  }

    if (info->LineColorBase != 0)
    {
      if ((varVdp2Regs->LCTA.part.U & 0x8000) != 0) {
        rbg->LineColorRamAdress = Vdp2RamReadWord(NULL, Vdp2Ram, info->LineColorBase);
        *line_texture->textdata = rbg->LineColorRamAdress | (linecl << 24);
        line_texture->textdata++;
        info->LineColorBase += lineInc;
      }
      else {
        *line_texture->textdata = rbg->LineColorRamAdress;
        line_texture->textdata++;
      }
    }

    //	  if (regs) ReadVdp2ColorOffset(regs, info, info->linecheck_mask);
    for (l = 0; l < hres; l++)
    {
      switch (varVdp2Regs->RPMD | rgb_type ) {
      case 0:
        parameter = &rbg->paraA;
        if (parameter->coefenab) {
          if (vdp2rGetKValue(parameter, l) == 0) {
            *(texture->textdata++) = 0x00000000;
            continue;
          }
        }
        break;
      case 1:
        parameter = &rbg->paraB;
        if (parameter->coefenab) {
          if (vdp2rGetKValue(parameter, l) == 0) {
            *(texture->textdata++) = 0x00000000;
            continue;
          }
        }
        break;
      case 2:
        if (!(rbg->paraA.coefenab)) {
          parameter = &rbg->paraA;
        } else {
          if (rbg->paraB.coefenab) {
            parameter = &rbg->paraA;
            if (vdp2rGetKValue(parameter, l) == 0) {
              parameter = &rbg->paraB;
              if( vdp2rGetKValue(parameter, l) == 0) {
                *(texture->textdata++) = 0x00000000;
                continue;
              }
            }
          }
          else {
            parameter = &rbg->paraA;
            if (vdp2rGetKValue(parameter, l) == 0) {
              rbg->paraB.lineaddr = rbg->paraA.lineaddr;
              parameter = &rbg->paraB;
            }
          }
        }
        break;
      default:
        parameter = info->GetRParam(rbg, l, k, varVdp2Regs);
        break;
      }
      if (parameter == NULL)
      {
        *(texture->textdata++) = 0x00000000;
        continue;
      }

      h = (parameter->ky * (parameter->Xsp + parameter->dx * l) + parameter->Xp);
      v = (parameter->ky * (parameter->Ysp + parameter->dy * l) + parameter->Yp);
      if (info->isbitmap)
      {

        switch (parameter->screenover) {
        case OVERMODE_REPEAT:
          h &= cellw - 1;
          v &= cellh - 1;
          break;
        case OVERMODE_SELPATNAME:
          VDP2LOG("Screen-over mode 1 not implemented");
          h &= cellw - 1;
          v &= cellh - 1;
          break;
        case OVERMODE_TRANSE:
          if ((h < 0) || (h >= cellw) || (v < 0) || (v >= cellh)) {
            *(texture->textdata++) = 0x00;
            continue;
          }
          break;
        case OVERMODE_512:
          if ((h < 0) || (h > 512) || (v < 0) || (v > 512)) {
            *(texture->textdata++) = 0x00;
            continue;
          }
        }
        // Fetch Pixel
        info->charaddr = parameter->charaddr;
        color = Vdp2RotationFetchPixel(info, h, v, cellw);
      }
      else
      {
        // Tile
        int planenum;
        switch (parameter->screenover) {
        case OVERMODE_TRANSE:
          if ((h < 0) || (h >= parameter->MaxH) || (v < 0) || (v >= parameter->MaxV)) {
            *(texture->textdata++) = 0x00;
            continue;
          }
          x = h;
          y = v;
          if ((x >> rbg->patternshift) != oldcellx || (y >> rbg->patternshift) != oldcelly) {
            oldcellx = x >> rbg->patternshift;
            oldcelly = y >> rbg->patternshift;

            // Calculate which plane we're dealing with
            planenum = (x >> parameter->ShiftPaneX) + ((y >> parameter->ShiftPaneY) << 2);
            x &= parameter->MskH;
            y &= parameter->MskV;
            info->addr = parameter->PlaneAddrv[planenum];

            // Figure out which page it's on(if plane size is not 1x1)
            info->addr += (((y >> 9) * rbg->pagesize * info->planew) +
              ((x >> 9) * rbg->pagesize) +
              (((y & 511) >> rbg->patternshift) * info->pagewh) +
              ((x & 511) >> rbg->patternshift)) << info->patterndatasize;

            Vdp2PatternAddr(info, varVdp2Regs); // Heh, this could be optimized
          }
          break;
        case OVERMODE_512:
          if ((h < 0) || (h > 512) || (v < 0) || (v > 512)) {
            *(texture->textdata++) = 0x00;
            continue;
          }
          x = h;
          y = v;
          if ((x >> rbg->patternshift) != oldcellx || (y >> rbg->patternshift) != oldcelly) {
              oldcellx = x >> rbg->patternshift;
              oldcelly = y >> rbg->patternshift;

              // Calculate which plane we're dealing with
              planenum = (x >> parameter->ShiftPaneX) + ((y >> parameter->ShiftPaneY) << 2);
              x &= parameter->MskH;
              y &= parameter->MskV;
              info->addr = parameter->PlaneAddrv[planenum];

              // Figure out which page it's on(if plane size is not 1x1)
              info->addr += (((y >> 9) * rbg->pagesize * info->planew) +
                ((x >> 9) * rbg->pagesize) +
                (((y & 511) >> rbg->patternshift) * info->pagewh) +
                ((x & 511) >> rbg->patternshift)) << info->patterndatasize;

              Vdp2PatternAddr(info, varVdp2Regs); // Heh, this could be optimized
          }
          break;
        case OVERMODE_REPEAT: {
          h &= (parameter->MaxH - 1);
          v &= (parameter->MaxV - 1);
          x = h;
          y = v;
          if ((x >> rbg->patternshift) != oldcellx || (y >> rbg->patternshift) != oldcelly) {
              oldcellx = x >> rbg->patternshift;
              oldcelly = y >> rbg->patternshift;

              // Calculate which plane we're dealing with
              planenum = (x >> parameter->ShiftPaneX) + ((y >> parameter->ShiftPaneY) << 2);
              x &= parameter->MskH;
              y &= parameter->MskV;
              info->addr = parameter->PlaneAddrv[planenum];

              // Figure out which page it's on(if plane size is not 1x1)
              info->addr += (((y >> 9) * rbg->pagesize * info->planew) +
                ((x >> 9) * rbg->pagesize) +
                (((y & 511) >> rbg->patternshift) * info->pagewh) +
                ((x & 511) >> rbg->patternshift)) << info->patterndatasize;

              Vdp2PatternAddr(info, varVdp2Regs); // Heh, this could be optimized
            }
          }
          break;
        case OVERMODE_SELPATNAME: {
            x = h;
            y = v;
            if ((x >> rbg->patternshift) != oldcellx || (y >> rbg->patternshift) != oldcelly) {
              oldcellx = x >> rbg->patternshift;
              oldcelly = y >> rbg->patternshift;

              if ((h < 0) || (h >= parameter->MaxH) || (v < 0) || (v >= parameter->MaxV)) {
                x &= parameter->MskH;
                y &= parameter->MskV;
                Vdp2PatternAddrUsingPatternname(info, parameter->over_pattern_name, varVdp2Regs);
              }
              else {
                planenum = (x >> parameter->ShiftPaneX) + ((y >> parameter->ShiftPaneY) << 2);
                x &= parameter->MskH;
                y &= parameter->MskV;
                info->addr = parameter->PlaneAddrv[planenum];
                // Figure out which page it's on(if plane size is not 1x1)
                info->addr += (((y >> 9) * rbg->pagesize * info->planew) +
                  ((x >> 9) * rbg->pagesize) +
                  (((y & 511) >> rbg->patternshift) * info->pagewh) +
                  ((x & 511) >> rbg->patternshift)) << info->patterndatasize;
                  Vdp2PatternAddr(info, varVdp2Regs); // Heh, this could be optimized
              }
            }
          }
          break;
        }

        // Figure out which pixel in the tile we want
        if (info->patternwh == 1)
        {
          x &= 8 - 1;
          y &= 8 - 1;

          // vertical flip
          if (info->flipfunction & 0x2)
            y = 8 - 1 - y;

          // horizontal flip
          if (info->flipfunction & 0x1)
            x = 8 - 1 - x;
        }
        else
        {
          if (info->flipfunction)
          {
            y &= 16 - 1;
            if (info->flipfunction & 0x2)
            {
              if (!(y & 8))
                y = 8 - 1 - y + 16;
              else
                y = 16 - 1 - y;
            }
            else if (y & 8)
              y += 8;

            if (info->flipfunction & 0x1)
            {
              if (!(x & 8))
                y += 8;

              x &= 8 - 1;
              x = 8 - 1 - x;
            }
            else if (x & 8)
            {
              y += 8;
              x &= 8 - 1;
            }
            else
              x &= 8 - 1;
          }
          else
          {
            y &= 16 - 1;
            if (y & 8)
              y += 8;
            if (x & 8)
              y += 8;
            x &= 8 - 1;
          }
        }

        // Fetch pixel
        color = Vdp2RotationFetchPixel(info, x, y, 8);
      }

      if (info->LineColorBase != 0 && VDP2_CC_NONE != (info->blendmode&0x03)) {
        if ((color & 0xFF000000) != 0 ) {
          color |= 0x8000;
          if (parameter->lineaddr != 0xFFFFFFFF && parameter->lineaddr != 0x000000 ) {
            color |= ((parameter->lineaddr & 0x7F) | 0x80) << 16;
          }
        }
      }
      *(texture->textdata++) = color; //Already in VDP2 format due to Vdp2RotationFetchPixel
    }
    texture->textdata += texture->w;
    }

    rbg->info.flipfunction = 0;

    LOG_AREA("%d %d %d\n", rbg->info.cellw, rbg->info.cellh, rbg->info.celly);
  }

#ifdef RGB_ASYNC
void Vdp2DrawRotation_in_async(void *p)
{
   while(rotation_run != 0){
     rotationTask *task = (rotationTask *)YabWaitEventQueue(rotq);
     if (task != NULL) {
       Vdp2DrawRotation_in_sync(task->rbg, task->varVdp2Regs);
       YabAddEventQueue(rotq_end_task, task->rbg);
       free(task->varVdp2Regs);
       free(task);
     }
     YabWaitEventQueue(rotq_end);
   }
}

static void Vdp2DrawRotation_in(RBGDrawInfo * rbg, Vdp2 *varVdp2Regs) {
   rotationTask *task = malloc(sizeof(rotationTask));

   task->rbg = malloc(sizeof(RBGDrawInfo));
   memcpy(task->rbg, rbg, sizeof(RBGDrawInfo));

   task->varVdp2Regs = malloc(sizeof(Vdp2));
   memcpy(task->varVdp2Regs, varVdp2Regs, sizeof(Vdp2));

   if (rotation_run == 0) {
     rotation_run = 1;
     rotq = YabThreadCreateQueue(32);
     rotq_end = YabThreadCreateQueue(32);
     rotq_end_task = YabThreadCreateQueue(32);
     YabThreadStart(YAB_THREAD_VDP2_RBG1, Vdp2DrawRotation_in_async, 0);
   }
   YabAddEventQueue(rotq_end, NULL);
   YabAddEventQueue(rotq, task);
   YabThreadYield();
}
#endif

//////////////////////////////////////////////////////////////////////////////

static void SetSaturnResolution(int width, int height)
{
  YglChangeResolution(width, height);
  YglSetDensity((vdp2_interlace == 0) ? 1 : 2);
}

//////////////////////////////////////////////////////////////////////////////

int VIDOGLInit(void)
{
  if(vidogl_renderer_started)
    return -1;

#ifdef SPRITE_CACHE
  if (yabsys.useVdp1cache) {
    initPatternCache();
  }
#endif

  if (YglInit(2048, 1024, 8) != 0)
    return -1;

  SetSaturnResolution(320, 224);
  YglReset(_Ygl->vdp1levels[0]);
  YglReset(_Ygl->vdp1levels[1]);
  for (int i=0; i<SPRITE; i++)
    YglReset(_Ygl->vdp2levels[i]);

  _Ygl->vdp1wratio = 1.0;
  _Ygl->vdp1hratio = 1.0;

  vidogl_renderer_started = 1;

  return 0;
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLDeInit(void)
{
  if(!vidogl_renderer_started)
    return;

#ifdef CELL_ASYNC
  if (drawcell_run == 1) {
    drawcell_run = 0;
    for (int i=0; i<4; i++) {
      YabAddEventQueue(cellq_end, NULL);
      YabAddEventQueue(cellq, NULL);
    }
    YabThreadWait(YAB_THREAD_VDP2_NBG0);
    YabThreadWait(YAB_THREAD_VDP2_NBG1);
    YabThreadWait(YAB_THREAD_VDP2_NBG2);
    YabThreadWait(YAB_THREAD_VDP2_NBG3);
  }
#endif
#ifdef RGB_ASYNC
  if (rotation_run == 1) {
    rotation_run = 0;
    YabAddEventQueue(rotq_end, NULL);
    YabAddEventQueue(rotq, NULL);
    YabThreadWait(YAB_THREAD_VDP2_RBG1);
  }
#endif
  YglDeInit();

#ifdef SPRITE_CACHE
  if (yabsys.useVdp1cache) {
    deinitPatternCache();
  }
#endif
  vidogl_renderer_started = 0;
}

//////////////////////////////////////////////////////////////////////////////

int _VIDOGLIsFullscreen;

void VIDOGLResize(int originx, int originy, unsigned int w, unsigned int h, int on)
{

  _VIDOGLIsFullscreen = on;

  GlWidth = w;
  GlHeight = h;

  SetSaturnResolution(_Ygl->rwidth, _Ygl->rheight);

  _Ygl->originx = originx;
  _Ygl->originy = originy;

  glViewport(originx, originy, GlWidth, GlHeight);

}

//////////////////////////////////////////////////////////////////////////////

int VIDOGLIsFullscreen(void) {
  return _VIDOGLIsFullscreen;
}

//////////////////////////////////////////////////////////////////////////////

int VIDOGLVdp1Reset(void)
{
#ifdef SPRITE_CACHE
  if (yabsys.useVdp1cache) {
    resetPatternCache();
  }
#endif
  return 0;
}

//////////////////////////////////////////////////////////////////////////////
void VIDOGLVdp1Draw()
{
  int i;
  int maxpri;
  int minpri;
  int line = 0;
  Vdp2 *varVdp2Regs = &Vdp2Lines[Vdp1External.plot_trigger_line];
  _Ygl->vpd1_running = 1;


#ifdef PERFRAME_LOG
  if (ppfp == NULL) {
    ppfp = fopen("ppfp0.txt", "w");
  }
  else {
    char fname[64];
    sprintf(fname, "ppfp%d.txt", fount);
    fclose(ppfp);
    ppfp = fopen(fname, "w");
}
  fount++;
#endif

  //varVdp2Regs = Vdp2RestoreRegs(0, Vdp2Lines);
  //if (varVdp2Regs == NULL) varVdp2Regs = Vdp2Regs;
  //memcpy(&baseVdp2Regs, varVdp2Regs, sizeof(Vdp2));
  //varVdp2Regs = &baseVdp2Regs;

  u8 *sprprilist = (u8 *)&(Vdp2Lines[0].PRISA);

  FrameProfileAdd("Vdp1Command start");

  YglTmPull(YglTM_vdp1[_Ygl->drawframe], 0);
  vdp1_setup();

  maxpri = 0x00;
  minpri = 0x07;
  for (i = 0; i < 8; i++)
  {
    if ((sprprilist[i] & 0x07) < minpri) minpri = (sprprilist[i] & 0x07);
    if ((sprprilist[i] & 0x07) > maxpri) maxpri = (sprprilist[i] & 0x07);
  }
  _Ygl->vdp1_maxpri = maxpri;
  _Ygl->vdp1_minpri = minpri;

  int firstalpha = (Vdp2External.perline_alpha_draw[0] & 0x40);
  int prioChanged = 0;
  int max = (yabsys.VBlankLineCount<270)?yabsys.VBlankLineCount:270;
  for(i = 0; i<max; i++) {
     if ((Vdp2External.perline_alpha_draw[i] & 0x40) != firstalpha) {
       prioChanged = 1;
       break;
     }
  }
  if (prioChanged == 1) {
    u32 * linebuf;
    int line_shift = 0;
    if (_Ygl->rheight > 256) {
      line_shift = 1;
    }
    else {
      line_shift = 0;
    }

    linebuf = YglGetPerlineBuf(&_Ygl->bg[SPRITE], _Ygl->rheight, 1+8+8 );
    for (line = 0; line < _Ygl->rheight; line++) {
      linebuf[line] = 0xFF000000;
      Vdp2 * lVdp2Regs = &Vdp2Lines[line >> line_shift];

      u8 *cclist = (u8 *)&lVdp2Regs->CCRSA;
      u8 *prilist = (u8 *)&lVdp2Regs->PRISA;
      for (i = 0; i < 8; i++) {
        linebuf[line + _Ygl->rheight * (1 + i)] = (prilist[i] & 0x7) << 24;
        linebuf[line + _Ygl->rheight * (1 + 8 + i)] = (0xFF - (((cclist[i] & 0x1F) << 3) & 0xF8)) << 24;
      }

      if (lVdp2Regs->CLOFEN & 0x40) {

        // color offset enable
        if (lVdp2Regs->CLOFSL & 0x40)
        {
          // color offset B
          vdp1cor = lVdp2Regs->COBR & 0xFF;
          if (lVdp2Regs->COBR & 0x100)
            vdp1cor |= 0xFFFFFF00;

          vdp1cog = lVdp2Regs->COBG & 0xFF;
          if (lVdp2Regs->COBG & 0x100)
            vdp1cog |= 0xFFFFFF00;

          vdp1cob = lVdp2Regs->COBB & 0xFF;
          if (lVdp2Regs->COBB & 0x100)
            vdp1cob |= 0xFFFFFF00;
        }
        else
        {
          // color offset A
          vdp1cor = lVdp2Regs->COAR & 0xFF;
          if (lVdp2Regs->COAR & 0x100)
            vdp1cor |= 0xFFFFFF00;

          vdp1cog = lVdp2Regs->COAG & 0xFF;
          if (lVdp2Regs->COAG & 0x100)
            vdp1cog |= 0xFFFFFF00;

          vdp1cob = lVdp2Regs->COAB & 0xFF;
          if (lVdp2Regs->COAB & 0x100)
            vdp1cob |= 0xFFFFFF00;
        }


        linebuf[line] |= ((int)(128.0f + (vdp1cor / 2.0)) & 0xFF) << 16;
        linebuf[line] |= ((int)(128.0f + (vdp1cog / 2.0)) & 0xFF) << 8;
        linebuf[line] |= ((int)(128.0f + (vdp1cob / 2.0)) & 0xFF) << 0;
      }
      else {
        linebuf[line] |= 0x00808080;
      }
    }
    YglSetPerlineBuf(&_Ygl->bg[SPRITE], linebuf, _Ygl->rheight, 1+8+8);
    _Ygl->vdp1_lineTexture = _Ygl->bg[SPRITE].lincolor_tex;
  }
  else {
    _Ygl->vdp1_lineTexture = 0;
    if (varVdp2Regs->CLOFEN & 0x40)
    {
      // color offset enable
      if (varVdp2Regs->CLOFSL & 0x40)
      {
        // color offset B
        vdp1cor = varVdp2Regs->COBR & 0xFF;
        if (varVdp2Regs->COBR & 0x100)
          vdp1cor |= 0xFFFFFF00;

        vdp1cog = varVdp2Regs->COBG & 0xFF;
        if (varVdp2Regs->COBG & 0x100)
          vdp1cog |= 0xFFFFFF00;

        vdp1cob = varVdp2Regs->COBB & 0xFF;
        if (varVdp2Regs->COBB & 0x100)
          vdp1cob |= 0xFFFFFF00;
      }
      else
      {
        // color offset A
        vdp1cor = varVdp2Regs->COAR & 0xFF;
        if (varVdp2Regs->COAR & 0x100)
          vdp1cor |= 0xFFFFFF00;

        vdp1cog = varVdp2Regs->COAG & 0xFF;
        if (varVdp2Regs->COAG & 0x100)
          vdp1cog |= 0xFFFFFF00;

        vdp1cob = varVdp2Regs->COAB & 0xFF;
        if (varVdp2Regs->COAB & 0x100)
          vdp1cob |= 0xFFFFFF00;
      }
    }
    else // color offset disable
      vdp1cor = vdp1cog = vdp1cob = 0;
  }

  _Ygl->msb_shadow_count_[_Ygl->drawframe] = 0;

  Vdp1DrawCommands(Vdp1Ram, Vdp1Regs, NULL);
  FrameProfileAdd("Vdp1Command end ");

#ifdef SPRITE_CACHE
  if (yabsys.useVdp1cache) {
    releasePattern();
  }
#endif
  _Ygl->vpd1_running = 0;

}

//////////////////////////////////////////////////////////////////////////////

#define IS_ZERO(A) (((A) < EPSILON)&&((A) > -EPSILON))

#define IS_LESS(A,V) ((A) < (V))
#define IS_MORE(A,V) ((A) > (V))

#define BORDER 1.0f

#define CW (1)
#define CCW (-1)

//#define TEST

static int test = 0;

static int expandVertices(float* in, float* out, int distorted)
{

  int i, j;
  int dst = 0;
  int isTriangle = 0;
  int isPoint = 1;
  int isQuad = 1;
  int isLine = 0;
  float nx[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  float ny[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  float vx[4];
  float vy[4];
  float dot;

#ifdef TEST
#include "testquad.inc"
#endif

//les quads concaves ne sont pas decoupés de la bonne facon parfois. Voir celui dans testquad.inc
// reste le triangle associé a une ligne

  for (i = 0; i<4; i++) {
    if (in[(((i+0)%4) << 1) + 0] != in[(((i+1)%4) << 1) + 0]) isPoint = 0;
    if (in[(((i+0)%4) << 1) + 1] != in[(((i+1)%4) << 1) + 1]) isPoint = 0;
    for (j=i+1; j<4; j++) {
      if ((in[i*2] == in[j*2]) && (in[i*2+1] == in[j*2+1])) {
        isTriangle = 1;
      }
    }
  }
  vx[0] = in[0] - in[4];
  vx[1] = in[2] - in[6];
  vx[2] = in[0] - in[6];
  vx[3] = in[4] - in[2];
  vy[0] = in[1] - in[5];
  vy[1] = in[3] - in[7];
  vy[2] = in[1] - in[7];
  vy[3] = in[5] - in[3];

  dot = vx[0]*vy[1] - vy[0]*vx[1];
  if (IS_ZERO(dot)) {
    dot = vx[2]*vy[3] - vy[2]*vx[3];
    if (IS_ZERO(dot)) {
      isLine = 1;
    }
  }

  nx[0] = in[0];
  ny[0] = in[1];
  nx[1] = in[2];
  ny[1] = in[3];
  nx[2] = in[4];
  ny[2] = in[5];
  nx[3] = in[6];
  ny[3] = in[7];

  if (isPoint) {
    isTriangle = 0;
    isLine = 0;
    isQuad = 0;
    nx[0] -= BORDER;
    ny[0] -= BORDER;
    nx[1] += BORDER;
    ny[1] -= BORDER;
    nx[2] += BORDER;
    ny[2] += BORDER;
    nx[3] -= BORDER;
    ny[3] += BORDER;
  }
  if (isLine) {
    float max[2] = {nx[0], ny[0]};
    float min[2] = {nx[0], ny[0]};
    float dx, dy, l;
    isTriangle = 0;
    isQuad = 0;
    for (i = 1; i<4; i++) {
      if ((nx[i] > max[0]) || (ny[i] > max[1])){
        max[0] = nx[i];
        max[1] = ny[i];
      }
      if ((nx[i] < max[0]) || (ny[i] < max[1])){
        min[0] = nx[i];
        min[1] = ny[i];
      };
    }
    dx = max[0] - min[0];
    dy = max[1] - min[1];
    l = sqrtf(dx*dx+dy*dy);
    if (IS_ZERO(l)) l = 1.0f;
    dx /= l;
    dy /= l;
    nx[0] = min[0] - dy * BORDER;
    ny[0] = min[1] + dx * BORDER;
    nx[1] = max[0] - dy * BORDER;
    ny[1] = max[1] + dx * BORDER;
    nx[2] = max[0] + dy * BORDER;
    ny[2] = max[1] - dx * BORDER;
    nx[3] = min[0] + dy * BORDER;
    ny[3] = min[1] - dx * BORDER;
  }
  if(isTriangle) {
    isQuad = 0;
  }

  if (isTriangle) {
    int i,j;
    float dx[4];
    float dy[4];

    int c1, c2 = -1;

    for (int i = 0; i < 4; i++) {
      for (int j = i+1; j < 4; j++) {
        if (IS_ZERO(in[2*i]-in[2*j]) && IS_ZERO(in[2*i+1]-in[2*j+1])) {
          c1 = i;
          c2 = j;
          break;
        }
      }
    }

    dx[0] = in[0] - in[2];
    dx[1] = in[2] - in[4];
    dx[2] = in[4] - in[6];
    dx[3] = in[6] - in[0];

    dy[0] = in[1] - in[3];
    dy[1] = in[3] - in[5];
    dy[2] = in[5] - in[7];
    dy[3] = in[7] - in[1];

    for (int i = 0; i < 4; i++) {
      float l;
      float cx = (dx[i] - dx[(i+3)%4]);
      float cy = (dy[i] - dy[(i+3)%4]);
      if ((i == c1) && (c2 == c1+1)) {
        cx = (dx[i]+dx[(i+1)%4] - dx[(i+3)%4]);
        cy = (dy[i]+dy[(i+1)%4] - dy[(i+3)%4]);
      }
      if ((i == (c1+1)%4) && (c2 != c1+1)) {
        cx = dx[i] + dx[(i+1)%4] - dx[(i+3)%4];
        cy = dy[i] + dy[(i+1)%4] - dy[(i+3)%4];
      }
      if ((i == (c1+3)%4) && (c2 != c1+1)) {
        cx = dx[i] - dx[(i+2)%4] - dx[(i+3)%4];
        cy = dy[i] - dy[(i+2)%4] - dy[(i+3)%4];
      }
      l = sqrtf(cx*cx+cy*cy);
      if (!IS_ZERO(l)) {
        cx /= l;
        cy /= l;
      }

      if (!IS_ZERO(cx)) {
        if (IS_LESS(cx, -EPSILON)) cx = -BORDER;
        if (IS_MORE(cx, EPSILON)) cx = BORDER;
      }
      if (!IS_ZERO(cy)) {
        if (IS_LESS(cy, -EPSILON)) cy = -BORDER;
        if (IS_MORE(cy, EPSILON)) cy = BORDER;
      }
      nx[i] = in[i*2] + cx;
      ny[i] = in[i*2+1] + cy;
      if (i == c2) {
        nx[i] = nx[c1];
        ny[i] = ny[c1];
      }
    }
#if 0
printf("%d %d\n", c1, c2);
printf("(%f %f) (%f %f) (%f %f) (%f %f)\n", in[0], in[1], in[2], in[3], in[4], in[5], in[6], in[7]);
printf("[(%f %f)] [(%f %f)] [(%f %f)] [(%f %f)]\n", nx[0], ny[0], nx[1], ny[1], nx[2], ny[2], nx[3], ny[3]);
#endif
  }

  if (isQuad) dst = 1;
  if ((isQuad || isTriangle) && distorted) {
    int disp = 0;
    int isSquare = 1;
//Quad tomb decone ici
    float dx[4];
    float dy[4];
    float entrant[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float angle[4];

    dx[0] = in[0] - in[2];
    dx[1] = in[2] - in[4];
    dx[2] = in[4] - in[6];
    dx[3] = in[6] - in[0];

    dy[0] = in[1] - in[3];
    dy[1] = in[3] - in[5];
    dy[2] = in[5] - in[7];
    dy[3] = in[7] - in[1];

    for (int i = 0; i<4; i++) {
      float l = sqrtf(dx[i]*dx[i]+dy[i]*dy[i]);
      if (!IS_ZERO(l)) {
        dx[i] /= l;
        dy[i] /= l;
      }
    }

    int rot = ((vx[0]*vy[1] - vy[0]*vx[1])>=0)?CW:CCW;
//Detect entrant vertex
    for (int i = 0; i < 4; i++) {
      float px = in[((i+1)%4)*2] - in[((i+3)%4)*2];
      float py = in[((i+1)%4)*2+1] - in[((i+3)%4)*2+1];
      float cx = in[i*2] - in[((i+3)%4)*2];
      float cy = in[i*2+1] - in[((i+3)%4)*2+1];
      if ((rot*(-py)*cx+rot*px*cy) > EPSILON) entrant[i] = -1.0f;
    }

    for (int i = 0; i < 4; i++) {
      float cx = (dx[i] - dx[(i+3)%4]);
      float cy = (dy[i] - dy[(i+3)%4]);
      float dx = cx;
      float dy = cy;

      if (!IS_ZERO(cx)) {
        if (IS_LESS(cx, -EPSILON)) dx = -BORDER;
        if (IS_MORE(cx, EPSILON)) dx = BORDER;
      } else
      if (!IS_ZERO(cy)) {
        if (IS_LESS(cy, -EPSILON)) dy = -BORDER;
        if (IS_MORE(cy, EPSILON)) dy = BORDER;
      }
#if 0
      if (IS_ZERO(dx[i]) && IS_ZERO(dy[(i+3)%4])) {
        cx = cy = 0.0f;
      }
      if (IS_ZERO(dy[i]) && IS_ZERO(dx[(i+3)%4])) {
        cx = cy = 0.0f;
      }
#endif
      nx[i] = in[i*2] + entrant[i] * dx;
      ny[i] = in[i*2+1] + entrant[i] * dy;
    }
#if 0
printf("(%f %f) (%f %f) (%f %f) (%f %f)\n", in[0], in[1], in[2], in[3], in[4], in[5], in[6], in[7]);
printf("[(%f %f)]%f [(%f %f)]%f [(%f %f)]%f [(%f %f)]%f\n", nx[0], ny[0], entrant[0], nx[1], ny[1], entrant[1], nx[2], ny[2], entrant[2], nx[3], ny[3], entrant[3]);
#endif

#if 1
  for (i = 0; i < 3; i++) {
    float dx = nx[(i + 1) & 0x3] - nx[i];
    float dy = ny[(i + 1) & 0x3] - ny[i];
    float d2x = nx[(i + 2) & 0x3] - nx[(i + 1) & 0x3];
    float d2y = ny[(i + 2) &  0x3] - ny[(i + 1) & 0x3];

    float dot = dx*d2x + dy*d2y;
    if (dot >= EPSILON || dot <= -EPSILON) {
      isSquare = 0;
      break;
    }
  }

  if (isSquare) {
    // find upper left opsition
    float minx = 65535.0f;
    float miny = 65535.0f;
    int lt_index = -1;

    for (i = 0; i < 4; i++) {
      if (nx[i] <= minx /*&& sprite.vertices[(i << 1) + 1] <= miny*/) {

        if (minx == nx[i]) {
          if (ny[i] < miny) {
            minx = nx[i];
            miny = ny[i];
            lt_index = i;
          }
        }
        else {
          minx = nx[i];
          miny = ny[i];
          lt_index = i;
        }
      }
    }

    float adx = nx[(lt_index + 1) & 0x03] - nx[lt_index];
    float ady = ny[(lt_index + 1) & 0x03] - ny[lt_index];
    float bdx = nx[(lt_index + 2) & 0x03] - nx[lt_index];
    float bdy = ny[(lt_index + 2) & 0x03] - ny[lt_index];
    float cross = (adx * bdy) - (bdx * ady);

    // clockwise
    if (cross >= 0) {
      nx[(lt_index + 1) & 0x03] += 1;
      ny[(lt_index + 1) & 0x03] += 0;
      nx[(lt_index + 2) & 0x03] += 1;
      ny[(lt_index + 2) & 0x03] += 1;
      nx[(lt_index + 3) & 0x03] += 0;
      ny[(lt_index + 3) & 0x03] += 1;
    }
    // counter-clockwise
    else {
      nx[(lt_index + 1) & 0x03] += 0;
      ny[(lt_index + 1) & 0x03] += 1;
      nx[(lt_index + 2) & 0x03] += 1;
      ny[(lt_index + 2) & 0x03] += 1;
      nx[(lt_index + 3) & 0x03] += 1;
      ny[(lt_index + 3) & 0x03] += 0;
    }
  }
#endif

  }
  out[0] = nx[0];
  out[1] = ny[0];
  out[2] = nx[1];
  out[3] = ny[1];
  out[4] = nx[2];
  out[5] = ny[2];
  out[6] = nx[3];
  out[7] = ny[3];
  return dst;
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp1NormalSpriteDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer)
{
  vdp1cmd_struct cmd;
  YglSprite sprite;
  YglTexture texture;
  YglCache cash;
  u64 tmp;
  s16 x, y;
  u16 color2;
  float col[4 * 4];
  Vdp2 *varVdp2Regs = &Vdp2Lines[0];
  float vert[8];

  Vdp1ReadCommand(&cmd, Vdp1Regs->addr, Vdp1Ram);
  if ((cmd.CMDSIZE & 0x8000)) {
    regs->EDSR |= 2;
    return; // BAD Command
  }

  sprite.dst = 0;
  sprite.blendmode = 0;

  int badgeometry = 1;

  if (((cmd.CMDXA & 0xFC00) == 0x0) || ((cmd.CMDXA & 0xFC00) == 0xFC00)) badgeometry = 0;
  if (((cmd.CMDYA & 0xFC00) == 0x0) || ((cmd.CMDYA & 0xFC00) == 0xFC00)) badgeometry = 0;

  if (badgeometry == 1) return;

  cmd.CMDXA = (s16)cmd.CMDXA;
  cmd.CMDYA = (s16)cmd.CMDYA;


  x = cmd.CMDXA;
  y = cmd.CMDYA;
  sprite.w = ((cmd.CMDSIZE >> 8) & 0x3F) * 8;
  sprite.h = cmd.CMDSIZE & 0xFF;
  if (sprite.w == 0 || sprite.h == 0) {
    return; //bad command
  }

  sprite.flip = (cmd.CMDCTRL & 0x30) >> 4;

  vert[0] = (float)x;
  vert[1] = (float)y;
  vert[2] = (float)(x + sprite.w);
  vert[3] = (float)y;
  vert[4] = (float)(x + sprite.w);
  vert[5] = (float)(y + sprite.h);
  vert[6] = (float)x;
  vert[7] = (float)(y + sprite.h);

  expandVertices(vert, sprite.vertices, 0);

  for (int i = 0; i<4; i++) {
    sprite.vertices[2*i] = (sprite.vertices[2*i] + Vdp1Regs->localX) * _Ygl->vdp1wratio;
    sprite.vertices[2*i+1] = (sprite.vertices[2*i+1] + Vdp1Regs->localY) * _Ygl->vdp1hratio;
  }

  tmp = cmd.CMDSRCA;
  tmp <<= 16;
  tmp |= cmd.CMDCOLR;
  tmp <<= 16;
  tmp |= cmd.CMDSIZE;

  sprite.priority = 0;

  // damaged data
  if (((cmd.CMDPMOD >> 3) & 0x7) > 5) {
    return;
  }

  sprite.uclipmode = (cmd.CMDPMOD >> 9) & 0x03;

  if ((cmd.CMDPMOD & 0x8000) != 0)
  {
    tmp |= 0x00020000;
  }

  sprite.blendmode = getCCProgramId(cmd.CMDPMOD);

  if (sprite.blendmode == -1) return; //Invalid color mode

  if ((cmd.CMDPMOD & 4))
  {
    for (int i = 0; i < 4; i++)
    {
      color2 = Vdp1RamReadWord(NULL, Vdp1Ram, (Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0x1C) << 3) + (i << 1));
      col[(i << 2) + 0] = (float)((color2 & 0x001F)) / (float)(0x1F) - 0.5f;
      col[(i << 2) + 1] = (float)((color2 & 0x03E0) >> 5) / (float)(0x1F) - 0.5f;
      col[(i << 2) + 2] = (float)((color2 & 0x7C00) >> 10) / (float)(0x1F) - 0.5f;
      col[(i << 2) + 3] = 1.0f;
    }

    if (sprite.w > 0 && sprite.h > 0)
    {
      if (1 == YglIsCached(YglTM_vdp1[_Ygl->drawframe], tmp, &cash))
      {
        YglCacheQuadGrowShading(&sprite, col, &cash, YglTM_vdp1[_Ygl->drawframe]);
        return;
      }

      YglQuadGrowShading(&sprite, &texture, col, &cash, YglTM_vdp1[_Ygl->drawframe]);
      YglCacheAdd(YglTM_vdp1[_Ygl->drawframe], tmp, &cash);
      Vdp1ReadTexture(&cmd, &sprite, &texture, &Vdp2Lines[0]);
      return;
    }

  }
  else // No Gouraud shading, use same color for all 4 vertices
  {
    if (sprite.w > 0 && sprite.h > 0)
    {
      if (1 == YglIsCached(YglTM_vdp1[_Ygl->drawframe], tmp, &cash))
      {
        YglCacheQuadGrowShading(&sprite, NULL, &cash, YglTM_vdp1[_Ygl->drawframe]);
        return;
      }

      YglQuadGrowShading(&sprite, &texture, NULL, &cash, YglTM_vdp1[_Ygl->drawframe]);
      YglCacheAdd(YglTM_vdp1[_Ygl->drawframe], tmp, &cash);

      Vdp1ReadTexture(&cmd, &sprite, &texture, varVdp2Regs);
    }
  }
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp1ScaledSpriteDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer)
{
  vdp1cmd_struct cmd;
  YglSprite sprite;
  YglTexture texture;
  YglCache cash;
  u64 tmp;
  s16 rw = 0, rh = 0;
  s16 x, y;
  u16 CMDPMOD;
  u16 color2;
  float col[4 * 4];
  float vert[8];
  int i;
  Vdp2 *varVdp2Regs = &Vdp2Lines[0];

  Vdp1ReadCommand(&cmd, Vdp1Regs->addr, Vdp1Ram);
  if (cmd.CMDSIZE == 0) {
    return; // BAD Command
  }

  sprite.dst = 0;
  sprite.blendmode = 0;

  int badgeometry = 1;

  if (((cmd.CMDXA & 0xFC00) == 0x0) || ((cmd.CMDXA & 0xFC00) == 0xFC00)) badgeometry = 0;
  if (((cmd.CMDYA & 0xFC00) == 0x0) || ((cmd.CMDYA & 0xFC00) == 0xFC00)) badgeometry = 0;

  if (((cmd.CMDCTRL & 0xF00) >> 8) == 0) {
    if (((cmd.CMDXC & 0xFC00) == 0x0) || ((cmd.CMDXC & 0xFC00) == 0xFC00)) badgeometry = 0;
    if (((cmd.CMDYC & 0xFC00) == 0x0) || ((cmd.CMDYC & 0xFC00) == 0xFC00)) badgeometry = 0;
  } else {
    if (((cmd.CMDXB & 0xFC00) == 0x0) || ((cmd.CMDXB & 0xFC00) == 0xFC00)) badgeometry = 0;
    if (((cmd.CMDYB & 0xFC00) == 0x0) || ((cmd.CMDYB & 0xFC00) == 0xFC00)) badgeometry = 0;
  }
  if (badgeometry == 1) return;

  cmd.CMDXA = (s16)cmd.CMDXA;
  cmd.CMDYA = (s16)cmd.CMDYA;
  cmd.CMDXB = (s16)cmd.CMDXB;
  cmd.CMDYB = (s16)cmd.CMDYB;
  cmd.CMDXC = (s16)cmd.CMDXC;
  cmd.CMDYC = (s16)cmd.CMDYC;

  x = cmd.CMDXA + Vdp1Regs->localX;
  y = cmd.CMDYA + Vdp1Regs->localY;
  sprite.w = ((cmd.CMDSIZE >> 8) & 0x3F) * 8;
  sprite.h = cmd.CMDSIZE & 0xFF;
  sprite.flip = (cmd.CMDCTRL & 0x30) >> 4;

  // Setup Zoom Point
  switch ((cmd.CMDCTRL & 0xF00) >> 8)
  {
  case 0x0: // Only two coordinates
    rw = cmd.CMDXC - cmd.CMDXA;
    rh = cmd.CMDYC - cmd.CMDYA;
    if (rw > 0) { rw += 1; } else { x += 1; }
    if (rh > 0) { rh += 1; } else { y += 1; }
    break;
  case 0x5: // Upper-left
    rw = cmd.CMDXB + 1;
    rh = cmd.CMDYB + 1;
    break;
  case 0x6: // Upper-Center
    rw = cmd.CMDXB;
    rh = cmd.CMDYB;
    x = x - rw / 2;
    rw++;
    rh++;
    break;
  case 0x7: // Upper-Right
    rw = cmd.CMDXB;
    rh = cmd.CMDYB;
    x = x - rw;
    rw++;
    rh++;
    break;
  case 0x9: // Center-left
    rw = cmd.CMDXB;
    rh = cmd.CMDYB;
    y = y - rh / 2;
    rw++;
    rh++;
    break;
  case 0xA: // Center-center
    rw = cmd.CMDXB;
    rh = cmd.CMDYB;
    x = x - rw / 2;
    y = y - rh / 2;
    rw++;
    rh++;
    break;
  case 0xB: // Center-right
    rw = cmd.CMDXB;
    rh = cmd.CMDYB;
    x = x - rw;
    y = y - rh / 2;
    rw++;
    rh++;
    break;
  case 0xD: // Lower-left
    rw = cmd.CMDXB;
    rh = cmd.CMDYB;
    y = y - rh;
    rw++;
    rh++;
    break;
  case 0xE: // Lower-center
    rw = cmd.CMDXB;
    rh = cmd.CMDYB;
    x = x - rw / 2;
    y = y - rh;
    rw++;
    rh++;
    break;
  case 0xF: // Lower-right
    rw = cmd.CMDXB;
    rh = cmd.CMDYB;
    x = x - rw;
    y = y - rh;
    rw++;
    rh++;
    break;
  default: break;
  }

  vert[0] = (float)x;
  vert[1] = (float)y;
  vert[2] = (float)(x + rw);
  vert[3] = (float)y;
  vert[4] = (float)(x + rw);
  vert[5] = (float)(y + rh);
  vert[6] = (float)x;
  vert[7] = (float)(y + rh);

  expandVertices(vert, sprite.vertices, 0);

  for (int i =0; i<4; i++) {
    sprite.vertices[2*i] = (sprite.vertices[2*i]) * _Ygl->vdp1wratio;
    sprite.vertices[2*i+1] = (sprite.vertices[2*i+1]) * _Ygl->vdp1hratio;
  }

  tmp = cmd.CMDSRCA;
  tmp <<= 16;
  tmp |= cmd.CMDCOLR;
  tmp <<= 16;
  tmp |= cmd.CMDSIZE;

  CMDPMOD = Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0x4);
  sprite.uclipmode = (CMDPMOD >> 9) & 0x03;

  sprite.priority = 0;

  // MSB
  if ((CMDPMOD & 0x8000) != 0)
  {
    tmp |= 0x00020000;
  }

  sprite.blendmode = getCCProgramId(CMDPMOD);

  if (sprite.blendmode == -1) return; //Invalid color mode

  if ((CMDPMOD & 4))
  {
    for (i = 0; i < 4; i++)
    {
      color2 = Vdp1RamReadWord(NULL, Vdp1Ram, (Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0x1C) << 3) + (i << 1));
      col[(i << 2) + 0] = (float)((color2 & 0x001F)) / (float)(0x1F) - 0.5f;
      col[(i << 2) + 1] = (float)((color2 & 0x03E0) >> 5) / (float)(0x1F) - 0.5f;
      col[(i << 2) + 2] = (float)((color2 & 0x7C00) >> 10) / (float)(0x1F) - 0.5f;
      col[(i << 2) + 3] = 1.0f;
    }

    if (sprite.w > 0 && sprite.h > 0)
    {
      if (1 == YglIsCached(YglTM_vdp1[_Ygl->drawframe], tmp, &cash))
      {
        YglCacheQuadGrowShading(&sprite, col, &cash, YglTM_vdp1[_Ygl->drawframe]);
        return;
      }

      YglQuadGrowShading(&sprite, &texture, col, &cash, YglTM_vdp1[_Ygl->drawframe]);
      YglCacheAdd(YglTM_vdp1[_Ygl->drawframe], tmp, &cash);
      Vdp1ReadTexture(&cmd, &sprite, &texture, varVdp2Regs);
      return;
    }


  }
  else // No Gouraud shading, use same color for all 4 vertices
  {

    if (sprite.w > 0 && sprite.h > 0)
    {
      if (1 == YglIsCached(YglTM_vdp1[_Ygl->drawframe], tmp, &cash))
      {
        YglCacheQuadGrowShading(&sprite, NULL, &cash, YglTM_vdp1[_Ygl->drawframe]);
        return;
      }

      YglQuadGrowShading(&sprite, &texture, NULL, &cash, YglTM_vdp1[_Ygl->drawframe]);
      YglCacheAdd(YglTM_vdp1[_Ygl->drawframe], tmp, &cash);
      Vdp1ReadTexture(&cmd, &sprite, &texture, varVdp2Regs);
    }

  }

}

//////////////////////////////////////////////////////////////////////////////
int isSquare(float *vert) {
  float vec1x = fabs(vert[6] - vert[0]);
  float vec1y = fabs(vert[3] - vert[1]);
  float vec2x = fabs(vert[4] - vert[2]);
  float vec2y = fabs(vert[5] - vert[7]);
  if ((vec1x == 0) && (vec2x == 0) && (vec1y == 0) && (vec2y == 0)) return 1;
  return 0;
}

int isTriangle(float *vert) {
  if ((vert[0] == vert[2]) && (vert[1] == vert[3])) return 1;
  if ((vert[2] == vert[4]) && (vert[3] == vert[5])) return 1;
  if ((vert[4] == vert[6]) && (vert[5] == vert[7])) return 1;
  if ((vert[6] == vert[0]) && (vert[7] == vert[1])) return 1;
  return 0;
}

void fixVerticesSize(float *vert) {
  int square = 1;

  for (int i = 0; i < 3; i++) {
    float dx = vert[((i + 1) << 1) + 0] - vert[((i + 0) << 1) + 0];
    float dy = vert[((i + 1) << 1) + 1] - vert[((i + 0) << 1) + 1];
    if ((dx <= 1.0f && dx >= -1.0f) && (dy <= 1.0f && dy >= -1.0f)) {
      square = 0;
      break;
    }

    float d2x = vert[(((i + 2) & 0x3) << 1) + 0] - vert[((i + 1) << 1) + 0];
    float d2y = vert[(((i + 2) & 0x3) << 1) + 1] - vert[((i + 1) << 1) + 1];
    if ((d2x <= 1.0f && d2x >= -1.0f) && (d2y <= 1.0f && d2y >= -1.0f)) {
      square = 0;
      break;
    }

    float dot = dx*d2x + dy*d2y;
    if (dot > EPSILON || dot < -EPSILON) {
      square = 0;
      break;
    }
  }

  //if (square) {
    float minx;
    float miny;
    int lt_index;

    // find upper left opsition
    minx = 65535.0f;
    miny = 65535.0f;
    lt_index = -1;
    for (int i = 0; i < 4; i++) {
      if (vert[(i << 1) + 0] <= minx && vert[(i << 1) + 1] <= miny) {
        minx = vert[(i << 1) + 0];
        miny = vert[(i << 1) + 1];
        lt_index = i;
      }
    }

    for (int i = 0; i < 4; i++) {
      if (i != lt_index) {
        float nx;
        float ny;
        // vectorize
        float dx = vert[(i << 1) + 0] - vert[((lt_index) << 1) + 0];
        float dy = vert[(i << 1) + 1] - vert[((lt_index) << 1) + 1];

        // normalize
        float len = fabsf(sqrtf(dx*dx + dy*dy));
        if (len <= EPSILON) {
          continue;
        }
        nx = dx / len;
        ny = dy / len;
        if (nx >= EPSILON) nx = 1.0f; else nx = 0.0f;
        if (ny >= EPSILON) ny = 1.0f; else ny = 0.0f;

        // expand vertex
        vert[(i << 1) + 0] += nx;
        vert[(i << 1) + 1] += ny;
      }
    }
  //}
}

void VIDOGLVdp1DistortedSpriteDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer)
{
  vdp1cmd_struct cmd;
  YglSprite sprite;
  YglTexture texture;
  YglCache cash;
  u64 tmp;
  u16 CMDPMOD;
  u16 color2;
  int i;
  float col[4 * 4];
  float vert[8];
  int square = 0;
  int triangle = 0;
  Vdp2 *varVdp2Regs = &Vdp2Lines[0];

  Vdp1ReadCommand(&cmd, Vdp1Regs->addr, Vdp1Ram);
  if (cmd.CMDSIZE == 0) {
    return; // BAD Command
  }

  sprite.blendmode = 0;
  sprite.w = ((cmd.CMDSIZE >> 8) & 0x3F) * 8;
  sprite.h = cmd.CMDSIZE & 0xFF;
  sprite.cor = 0;
  sprite.cog = 0;
  sprite.cob = 0;

  // ??? sakatuku2 new scene bug ???
  if (sprite.h != 0 && sprite.w == 0) {
    sprite.w = 8;
    sprite.h = 8;
  }

  sprite.flip = (cmd.CMDCTRL & 0x30) >> 4;

  int badgeometry = 1;

  if (((cmd.CMDXA & 0xFC00) == 0x0) || ((cmd.CMDXA & 0xFC00) == 0xFC00)) badgeometry = 0;
  if (((cmd.CMDYA & 0xFC00) == 0x0) || ((cmd.CMDYA & 0xFC00) == 0xFC00)) badgeometry = 0;
  if (((cmd.CMDXB & 0xFC00) == 0x0) || ((cmd.CMDXB & 0xFC00) == 0xFC00)) badgeometry = 0;
  if (((cmd.CMDYB & 0xFC00) == 0x0) || ((cmd.CMDYB & 0xFC00) == 0xFC00)) badgeometry = 0;
  if (((cmd.CMDXC & 0xFC00) == 0x0) || ((cmd.CMDXC & 0xFC00) == 0xFC00)) badgeometry = 0;
  if (((cmd.CMDYC & 0xFC00) == 0x0) || ((cmd.CMDYC & 0xFC00) == 0xFC00)) badgeometry = 0;
  if (((cmd.CMDXD & 0xFC00) == 0x0) || ((cmd.CMDXD & 0xFC00) == 0xFC00)) badgeometry = 0;
  if (((cmd.CMDYD & 0xFC00) == 0x0) || ((cmd.CMDYD & 0xFC00) == 0xFC00)) badgeometry = 0;

  if (badgeometry == 1) return;

  vert[0] = (float)(s16)cmd.CMDXA;
  vert[1] = (float)(s16)cmd.CMDYA;
  vert[2] = (float)(s16)cmd.CMDXB;
  vert[3] = (float)(s16)cmd.CMDYB;
  vert[4] = (float)(s16)cmd.CMDXC;
  vert[5] = (float)(s16)cmd.CMDYC;
  vert[6] = (float)(s16)cmd.CMDXD;
  vert[7] = (float)(s16)cmd.CMDYD;

  square = isSquare(vert);
  triangle = isTriangle(vert);
  sprite.dst = !square && !triangle;

  expandVertices(vert, sprite.vertices, !square);

  fixVerticesSize(sprite.vertices);

  for (int i = 0; i<4; i++) {
    sprite.vertices[2*i] = (sprite.vertices[2*i] + Vdp1Regs->localX) * _Ygl->vdp1wratio;
    sprite.vertices[2*i+1] = (sprite.vertices[2*i+1] + Vdp1Regs->localY) * _Ygl->vdp1hratio;
  }

  tmp = cmd.CMDSRCA;
  tmp <<= 16;
  tmp |= cmd.CMDCOLR;
  tmp <<= 16;
  tmp |= cmd.CMDSIZE;

  sprite.priority = 0;

  sprite.uclipmode = (cmd.CMDPMOD >> 9) & 0x03;

  // MSB
  if ((cmd.CMDPMOD & 0x8000) != 0)
  {
    tmp |= 0x00020000;
  }

  sprite.blendmode = getCCProgramId(cmd.CMDPMOD);

  if (sprite.blendmode == -1) return; //Invalid color mode

  // Check if the Gouraud shading bit is set and the color mode is RGB
  if ((cmd.CMDPMOD & 4))
  {
    for (i = 0; i < 4; i++)
    {
      color2 = Vdp1RamReadWord(NULL, Vdp1Ram, (Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0x1C) << 3) + (i << 1));
      col[(i << 2) + 0] = (float)((color2 & 0x001F)) / (float)(0x1F) - 0.5f;
      col[(i << 2) + 1] = (float)((color2 & 0x03E0) >> 5) / (float)(0x1F) - 0.5f;
      col[(i << 2) + 2] = (float)((color2 & 0x7C00) >> 10) / (float)(0x1F) - 0.5f;
      col[(i << 2) + 3] = 1.0f;
    }

    if (1 == YglIsCached(YglTM_vdp1[_Ygl->drawframe], tmp, &cash))
    {
      YglCacheQuadGrowShading(&sprite, col, &cash, YglTM_vdp1[_Ygl->drawframe]);
      return;
    }

    YglQuadGrowShading(&sprite, &texture, col, &cash, YglTM_vdp1[_Ygl->drawframe]);
    YglCacheAdd(YglTM_vdp1[_Ygl->drawframe], tmp, &cash);
    Vdp1ReadTexture(&cmd, &sprite, &texture, varVdp2Regs);
    return;
  }
  else // No Gouraud shading, use same color for all 4 vertices
  {
    if (1 == YglIsCached(YglTM_vdp1[_Ygl->drawframe], tmp, &cash))
    {
      YglCacheQuadGrowShading(&sprite, NULL, &cash, YglTM_vdp1[_Ygl->drawframe]);
      return;
    }
    YglQuadGrowShading(&sprite, &texture, NULL, &cash, YglTM_vdp1[_Ygl->drawframe]);
    YglCacheAdd(YglTM_vdp1[_Ygl->drawframe], tmp, &cash);
    Vdp1ReadTexture(&cmd, &sprite, &texture, varVdp2Regs);
  }

  return;
      }

//////////////////////////////////////////////////////////////////////////////


static void  makeLinePolygon(s16 *v1, s16 *v2, float *outv) {
#define THICK 0.5f
  float dx;
  float dy;
  float len;
  float nx;
  float ny;
  float ex;
  float ey;
  float offset;

  if ((v1[0] == v2[0]) && (v1[1] == v2[1])) {
    v2[0] += 1;
    v2[1] += 1;
    outv[0] = v1[0];
    outv[1] = v1[1];
    outv[2] = v2[0];
    outv[3] = v1[1];
    outv[4] = v2[0];
    outv[5] = v2[1];
    outv[6] = v1[0];
    outv[7] = v2[1];
    return;
  }

  // vectorize;
  dx = v2[0] - v1[0];
  dy = v2[1] - v1[1];

  // normalize
  len = fabs(sqrtf((dx*dx) + (dy*dy)));
  if (len < EPSILON) {
    // fail;
    outv[0] = v1[0];
    outv[1] = v1[1];
    outv[2] = v2[0];
    outv[3] = v2[1];
    outv[4] = v2[0];
    outv[5] = v2[1];
    outv[6] = v1[0];
    outv[7] = v1[1];
    return;
  }

  nx = dx / len;
  ny = dy / len;

  // turn
  dx = ny  * THICK;
  dy = -nx * THICK;

  // extend
  ex = nx * THICK;
  ey = ny * THICK;

  // offset
  offset = THICK;

  // triangle
  outv[0] = v1[0] - ex - dx + offset;
  outv[1] = v1[1] - ey - dy + offset;
  outv[2] = v1[0] - ex + dx + offset;
  outv[3] = v1[1] - ey + dy + offset;
  outv[4] = v2[0] + ex + dx + offset;
  outv[5] = v2[1] + ey + dy + offset;
  outv[6] = v2[0] + ex - dx + offset;
  outv[7] = v2[1] + ey - dy + offset;

}


//////////////////////////////////////////////////////////////////////////////

static int isLine(s16 *v) {
  s16 dx[4];
  s16 dy[4];

  dx[0] = v[0] - v[2];
  dy[0] = v[1] - v[3];

  dx[1] = v[0] - v[4];
  dy[1] = v[1] - v[5];

  dx[2] = v[2] - v[4];
  dy[2] = v[3] - v[5];

  dx[3] = v[2] - v[6];
  dy[3] = v[3] - v[7];

  if (dx[1]*dy[0] == dx[0]*dy[1] == dx[1]*dy[2] == dx[2]*dy[1] == dx[2]*dy[3] == dx[3]*dy[2]) {
    if ((dx[0]== 0)&&(dy[0]== 0)&&(dy[1]== 0)&&(dy[1]== 0) && (dx[2] == 0) && (dy[2]== 0) && (dx[3]==0) && (dy[3]==0)) {
      return 0;
    } else {
      return 1;
    }
  }
  return 0;
}


void VIDOGLVdp1PolygonDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer)
{

//The polygon calculation seems not so good. A good test game is break point. All the lines are blinking and none is really thin and straight.

  u16 color;
  YglSprite sprite;
  YglTexture texture;
  u16 color2;
  int i;
  float col[4 * 4];
  float vert[8];
  int gouraud = 0;
  int priority = 0;
  int shadow = 0;
  int normalshadow = 0;
  int colorcalc = 0;
  vdp1cmd_struct cmd;
  float line_polygon[8];
  Vdp2 *varVdp2Regs = &Vdp2Lines[0];

  Vdp1ReadCommand(&cmd, Vdp1Regs->addr, Vdp1Ram);

  int badgeometry = 1;

  if (((cmd.CMDXA & 0xFC00) == 0x0) || ((cmd.CMDXA & 0xFC00) == 0xFC00)) badgeometry = 0;
  if (((cmd.CMDYA & 0xFC00) == 0x0) || ((cmd.CMDYA & 0xFC00) == 0xFC00)) badgeometry = 0;
  if (((cmd.CMDXB & 0xFC00) == 0x0) || ((cmd.CMDXB & 0xFC00) == 0xFC00)) badgeometry = 0;
  if (((cmd.CMDYB & 0xFC00) == 0x0) || ((cmd.CMDYB & 0xFC00) == 0xFC00)) badgeometry = 0;
  if (((cmd.CMDXC & 0xFC00) == 0x0) || ((cmd.CMDXC & 0xFC00) == 0xFC00)) badgeometry = 0;
  if (((cmd.CMDYC & 0xFC00) == 0x0) || ((cmd.CMDYC & 0xFC00) == 0xFC00)) badgeometry = 0;
  if (((cmd.CMDXD & 0xFC00) == 0x0) || ((cmd.CMDXD & 0xFC00) == 0xFC00)) badgeometry = 0;
  if (((cmd.CMDYD & 0xFC00) == 0x0) || ((cmd.CMDYD & 0xFC00) == 0xFC00)) badgeometry = 0;

  if (badgeometry == 1) return;


  sprite.blendmode = 0;
  sprite.dst = 0;

  vert[0] = (float)(s16)cmd.CMDXA;
  vert[1] = (float)(s16)cmd.CMDYA;
  vert[2] = (float)(s16)cmd.CMDXB;
  vert[3] = (float)(s16)cmd.CMDYB;
  vert[4] = (float)(s16)cmd.CMDXC;
  vert[5] = (float)(s16)cmd.CMDYC;
  vert[6] = (float)(s16)cmd.CMDXD;
  vert[7] = (float)(s16)cmd.CMDYD;

  //expandVertices(vert, sprite.vertices, !isSquare(vert));
  memcpy(sprite.vertices, vert, sizeof(float)*8);
  fixVerticesSize(sprite.vertices);

  for (int i = 0; i<4; i++) {
    sprite.vertices[2*i] = (sprite.vertices[2*i] + Vdp1Regs->localX) * _Ygl->vdp1wratio;
    sprite.vertices[2*i+1] = (sprite.vertices[2*i+1] + Vdp1Regs->localY) * _Ygl->vdp1hratio;
  }

  color = cmd.CMDCOLR;
  sprite.uclipmode = (cmd.CMDPMOD >> 9) & 0x03;

  // Check if the Gouraud shading bit is set and the color mode is RGB
  if ((cmd.CMDPMOD & 4))
  {
    for (i = 0; i < 4; i++)
    {
      color2 = Vdp1RamReadWord(NULL, Vdp1Ram, (Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0x1C) << 3) + (i << 1));
      col[(i << 2) + 0] = (float)((color2 & 0x001F)) / (float)(0x1F) - 0.5f;
      col[(i << 2) + 1] = (float)((color2 & 0x03E0) >> 5) / (float)(0x1F) - 0.5f;
      col[(i << 2) + 2] = (float)((color2 & 0x7C00) >> 10) / (float)(0x1F) - 0.5f;
      col[(i << 2) + 3] = 1.0f;
    }
    gouraud = 1;
  }

  sprite.priority = 0;
  sprite.w = 1;
  sprite.h = 1;
  sprite.flip = 0;
  sprite.cor = 0x00;
  sprite.cog = 0x00;
  sprite.cob = 0x00;

  sprite.blendmode = getCCProgramId(cmd.CMDPMOD);

  if (sprite.blendmode == -1) return; //Invalid color mode

  if (gouraud == 1)
  {
    YglQuadGrowShading(&sprite, &texture, col, NULL, YglTM_vdp1[_Ygl->drawframe]);
  }
  else {
    YglQuadGrowShading(&sprite, &texture, NULL, NULL, YglTM_vdp1[_Ygl->drawframe]);
  }

  *texture.textdata = Vdp1ReadPolygonColor(&cmd,varVdp2Regs);
}

void VIDOGLVdp1PolylineDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer)
{
  s16 v[8];
  float line_poygon[8];
  u16 color;
  u16 CMDPMOD;
  u8 alpha;
  YglSprite polygon;
  YglTexture texture;
  YglCache c;
  int priority = 0;
  vdp1cmd_struct cmd;
  float col[4 * 4];
  float linecol[4 * 4];
  int gouraud = 0;
  u16 color2;
  int shadow = 0;
  int normalshadow = 0;
  int colorcalc = 0;
  Vdp2 *varVdp2Regs = &Vdp2Lines[0];
  polygon.blendmode = 0;
  polygon.dst = 0;
  v[0] = Vdp1Regs->localX + (Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0x0C));
  v[1] = Vdp1Regs->localY + (Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0x0E));
  v[2] = Vdp1Regs->localX + (Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0x10));
  v[3] = Vdp1Regs->localY + (Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0x12));
  v[4] = Vdp1Regs->localX + (Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0x14));
  v[5] = Vdp1Regs->localY + (Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0x16));
  v[6] = Vdp1Regs->localX + (Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0x18));
  v[7] = Vdp1Regs->localY + (Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0x1A));

  color = Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0x6);
  CMDPMOD = Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0x4);
  polygon.uclipmode = (CMDPMOD >> 9) & 0x03;
  polygon.cor = 0x00;
  polygon.cog = 0x00;
  polygon.cob = 0x00;

  if (color & 0x8000)
    priority = varVdp2Regs->PRISA & 0x7;
  else
  {
    Vdp1ProcessSpritePixel(varVdp2Regs->SPCTL & 0xF, &color, &shadow, &normalshadow, &priority, &colorcalc);
#ifdef WORDS_BIGENDIAN
    priority = ((u8 *)&varVdp2Regs->PRISA)[priority ^ 1] & 0x7;
#else
    priority = ((u8 *)&varVdp2Regs->PRISA)[priority] & 0x7;
#endif
  }

  polygon.priority = 8;
  polygon.w = 1;
  polygon.h = 1;
  polygon.flip = 0;

  if ((CMDPMOD & 4))
  {
    int i;
    for (i = 0; i < 4; i++)
    {
      color2 = Vdp1RamReadWord(NULL, Vdp1Ram, (Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0x1C) << 3) + (i << 1));
      col[(i << 2) + 0] = (float)((color2 & 0x001F)) / (float)(0x1F) - 0.5f;
      col[(i << 2) + 1] = (float)((color2 & 0x03E0) >> 5) / (float)(0x1F) - 0.5f;
      col[(i << 2) + 2] = (float)((color2 & 0x7C00) >> 10) / (float)(0x1F) - 0.5f;
      col[(i << 2) + 3] = 1.0f;
    }
    gouraud = 1;
  }

  makeLinePolygon(&v[0], &v[2], line_poygon);
  polygon.vertices[0] = line_poygon[0] * _Ygl->vdp1wratio;
  polygon.vertices[1] = line_poygon[1] * _Ygl->vdp1hratio;
  polygon.vertices[2] = line_poygon[2] * _Ygl->vdp1wratio;
  polygon.vertices[3] = line_poygon[3] * _Ygl->vdp1hratio;
  polygon.vertices[4] = line_poygon[4] * _Ygl->vdp1wratio;
  polygon.vertices[5] = line_poygon[5] * _Ygl->vdp1hratio;
  polygon.vertices[6] = line_poygon[6] * _Ygl->vdp1wratio;
  polygon.vertices[7] = line_poygon[7] * _Ygl->vdp1hratio;

  polygon.blendmode = getCCProgramId(CMDPMOD);

  if (polygon.blendmode == -1) return; //Invalid color mode

  if (gouraud) {
    linecol[0] = col[(0 << 2) + 0];
    linecol[1] = col[(0 << 2) + 1];
    linecol[2] = col[(0 << 2) + 2];
    linecol[3] = col[(0 << 2) + 3];
    linecol[4] = col[(0 << 2) + 0];
    linecol[5] = col[(0 << 2) + 1];
    linecol[6] = col[(0 << 2) + 2];
    linecol[7] = col[(0 << 2) + 3];
    linecol[8] = col[(1 << 2) + 0];
    linecol[9] = col[(1 << 2) + 1];
    linecol[10] = col[(1 << 2) + 2];
    linecol[11] = col[(1 << 2) + 3];
    linecol[12] = col[(1 << 2) + 0];
    linecol[13] = col[(1 << 2) + 1];
    linecol[14] = col[(1 << 2) + 2];
    linecol[15] = col[(1 << 2) + 3];
    YglQuadGrowShading(&polygon, &texture, linecol, &c, YglTM_vdp1[_Ygl->drawframe]);
  }
  else {
    YglQuadGrowShading(&polygon, &texture, NULL, &c, YglTM_vdp1[_Ygl->drawframe]);
  }

  Vdp1ReadCommand(&cmd, Vdp1Regs->addr, Vdp1Ram);
  *texture.textdata = Vdp1ReadPolygonColor(&cmd,varVdp2Regs);

/*
  if (color == 0)
  {
    alpha = 0;
    priority = 0;
  }
  else {
    alpha = 0xF8;
    if (CMDPMOD & 0x100)
    {
      alpha = 0x80;
    }
  }

  alpha |= priority;

  if (color & 0x8000 && (varVdp2Regs->SPCTL & 0x20)) {
    int SPCCCS = (varVdp2Regs->SPCTL >> 12) & 0x3;
    if (SPCCCS == 0x03) {
      int colorcl;
      int normal_shadow;
      Vdp1ReadPriority(&cmd, &priority, &colorcl, &normal_shadow);
      u32 talpha = 0xF8 - ((colorcl << 3) & 0xF8);
      talpha |= priority;
      *texture.textdata = SAT2YAB1(talpha, color);
    }
    else {
      alpha |= priority;
      *texture.textdata = SAT2YAB1(alpha, color);
    }
  }
  else {
    Vdp1ReadCommand(&cmd, Vdp1Regs->addr, Vdp1Ram);
    *texture.textdata = Vdp1ReadPolygonColor(&cmd);
  }
*/


  makeLinePolygon(&v[2], &v[4], line_poygon);
  polygon.vertices[0] = line_poygon[0] * _Ygl->vdp1wratio;
  polygon.vertices[1] = line_poygon[1] * _Ygl->vdp1hratio;
  polygon.vertices[2] = line_poygon[2] * _Ygl->vdp1wratio;
  polygon.vertices[3] = line_poygon[3] * _Ygl->vdp1hratio;
  polygon.vertices[4] = line_poygon[4] * _Ygl->vdp1wratio;
  polygon.vertices[5] = line_poygon[5] * _Ygl->vdp1hratio;
  polygon.vertices[6] = line_poygon[6] * _Ygl->vdp1wratio;
  polygon.vertices[7] = line_poygon[7] * _Ygl->vdp1hratio;
  if (gouraud) {
    linecol[0] = col[(1 << 2) + 0];
    linecol[1] = col[(1 << 2) + 1];
    linecol[2] = col[(1 << 2) + 2];
    linecol[3] = col[(1 << 2) + 3];

    linecol[4] = col[(1 << 2) + 0];
    linecol[5] = col[(1 << 2) + 1];
    linecol[6] = col[(1 << 2) + 2];
    linecol[7] = col[(1 << 2) + 3];

    linecol[8] = col[(2 << 2) + 0];
    linecol[9] = col[(2 << 2) + 1];
    linecol[10] = col[(2 << 2) + 2];
    linecol[11] = col[(2 << 2) + 3];

    linecol[12] = col[(2 << 2) + 0];
    linecol[13] = col[(2 << 2) + 1];
    linecol[14] = col[(2 << 2) + 2];
    linecol[15] = col[(2 << 2) + 3];

    YglCacheQuadGrowShading(&polygon, linecol, &c, YglTM_vdp1[_Ygl->drawframe]);
  }
  else {
    YglCacheQuadGrowShading(&polygon, NULL, &c, YglTM_vdp1[_Ygl->drawframe]);
  }

  makeLinePolygon(&v[4], &v[6], line_poygon);
  polygon.vertices[0] = line_poygon[0] * _Ygl->vdp1wratio;
  polygon.vertices[1] = line_poygon[1] * _Ygl->vdp1hratio;
  polygon.vertices[2] = line_poygon[2] * _Ygl->vdp1wratio;
  polygon.vertices[3] = line_poygon[3] * _Ygl->vdp1hratio;
  polygon.vertices[4] = line_poygon[4] * _Ygl->vdp1wratio;
  polygon.vertices[5] = line_poygon[5] * _Ygl->vdp1hratio;
  polygon.vertices[6] = line_poygon[6] * _Ygl->vdp1wratio;
  polygon.vertices[7] = line_poygon[7] * _Ygl->vdp1hratio;
  if (gouraud) {
    linecol[0] = col[(2 << 2) + 0];
    linecol[1] = col[(2 << 2) + 1];
    linecol[2] = col[(2 << 2) + 2];
    linecol[3] = col[(2 << 2) + 3];
    linecol[4] = col[(2 << 2) + 0];
    linecol[5] = col[(2 << 2) + 1];
    linecol[6] = col[(2 << 2) + 2];
    linecol[7] = col[(2 << 2) + 3];
    linecol[8] = col[(3 << 2) + 0];
    linecol[9] = col[(3 << 2) + 1];
    linecol[10] = col[(3 << 2) + 2];
    linecol[11] = col[(3 << 2) + 3];
    linecol[12] = col[(3 << 2) + 0];
    linecol[13] = col[(3 << 2) + 1];
    linecol[14] = col[(3 << 2) + 2];
    linecol[15] = col[(3 << 2) + 3];
    YglCacheQuadGrowShading(&polygon, linecol, &c, YglTM_vdp1[_Ygl->drawframe]);
  }
  else {
    YglCacheQuadGrowShading(&polygon, NULL, &c, YglTM_vdp1[_Ygl->drawframe]);
  }


  if (!(v[6] == v[0] && v[7] == v[1])) {
    makeLinePolygon(&v[6], &v[0], line_poygon);
    polygon.vertices[0] = line_poygon[0] * _Ygl->vdp1wratio;
    polygon.vertices[1] = line_poygon[1] * _Ygl->vdp1hratio;
    polygon.vertices[2] = line_poygon[2] * _Ygl->vdp1wratio;
    polygon.vertices[3] = line_poygon[3] * _Ygl->vdp1hratio;
    polygon.vertices[4] = line_poygon[4] * _Ygl->vdp1wratio;
    polygon.vertices[5] = line_poygon[5] * _Ygl->vdp1hratio;
    polygon.vertices[6] = line_poygon[6] * _Ygl->vdp1wratio;
    polygon.vertices[7] = line_poygon[7] * _Ygl->vdp1hratio;
    if (gouraud) {
      linecol[0] = col[(3 << 2) + 0];
      linecol[1] = col[(3 << 2) + 1];
      linecol[2] = col[(3 << 2) + 2];
      linecol[3] = col[(3 << 2) + 3];
      linecol[4] = col[(3 << 2) + 0];
      linecol[5] = col[(3 << 2) + 1];
      linecol[6] = col[(3 << 2) + 2];
      linecol[7] = col[(3 << 2) + 3];
      linecol[8] = col[(0 << 2) + 0];
      linecol[9] = col[(0 << 2) + 1];
      linecol[10] = col[(0 << 2) + 2];
      linecol[11] = col[(0 << 2) + 3];
      linecol[12] = col[(0 << 2) + 0];
      linecol[13] = col[(0 << 2) + 1];
      linecol[14] = col[(0 << 2) + 2];
      linecol[15] = col[(0 << 2) + 3];
      YglCacheQuadGrowShading(&polygon, linecol, &c, YglTM_vdp1[_Ygl->drawframe]);
    }
    else {
      YglCacheQuadGrowShading(&polygon, NULL, &c, YglTM_vdp1[_Ygl->drawframe]);
    }
  }


}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp1LineDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer)
{
  s16 v[4];
  u16 color;
  u16 CMDPMOD;
  u8 alpha;
  YglSprite polygon;
  YglTexture texture;
  int priority = 0;
  float line_poygon[8];
  vdp1cmd_struct cmd;
  float col[4 * 2 * 2];
  int gouraud = 0;
  int shadow = 0;
  int normalshadow = 0;
  int colorcalc = 0;
  u16 color2;
  polygon.cor = 0x00;
  polygon.cog = 0x00;
  polygon.cob = 0x00;
  Vdp2 *varVdp2Regs = &Vdp2Lines[0];

  polygon.dst = 0;
  v[0] = Vdp1Regs->localX + (Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0x0C));
  v[1] = Vdp1Regs->localY + (Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0x0E));
  v[2] = Vdp1Regs->localX + (Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0x10));
  v[3] = Vdp1Regs->localY + (Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0x12));

  color = Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0x6);
  CMDPMOD = Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0x4);
  polygon.uclipmode = (CMDPMOD >> 9) & 0x03;

  if (color & 0x8000)
    priority = varVdp2Regs->PRISA & 0x7;
  else
  {
    Vdp1ProcessSpritePixel(varVdp2Regs->SPCTL & 0xF, &color, &shadow, &normalshadow, &priority, &colorcalc);
#ifdef WORDS_BIGENDIAN
    priority = ((u8 *)&varVdp2Regs->PRISA)[priority ^ 1] & 0x7;
#else
    priority = ((u8 *)&varVdp2Regs->PRISA)[priority] & 0x7;
#endif
  }

  polygon.priority = 8;

  // Check if the Gouraud shading bit is set and the color mode is RGB
  if ((CMDPMOD & 4))
  {
    int i;
    for (i = 0; i < 4; i += 2)
    {
      color2 = Vdp1RamReadWord(NULL, Vdp1Ram, (Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0x1C) << 3) + (i << 1));
      col[(i << 2) + 0] = (float)((color2 & 0x001F)) / (float)(0x1F) - 0.5f;
      col[(i << 2) + 1] = (float)((color2 & 0x03E0) >> 5) / (float)(0x1F) - 0.5f;
      col[(i << 2) + 2] = (float)((color2 & 0x7C00) >> 10) / (float)(0x1F) - 0.5f;
      col[(i << 2) + 3] = 1.0f;
      col[((i + 1) << 2) + 0] = col[(i << 2) + 0];
      col[((i + 1) << 2) + 1] = col[(i << 2) + 1];
      col[((i + 1) << 2) + 2] = col[(i << 2) + 2];
      col[((i + 1) << 2) + 3] = 1.0f;
    }
    gouraud = 1;
  }


  makeLinePolygon(&v[0], &v[2], line_poygon);
  polygon.vertices[0] = line_poygon[0] * _Ygl->vdp1wratio;
  polygon.vertices[1] = line_poygon[1] * _Ygl->vdp1hratio;
  polygon.vertices[2] = line_poygon[2] * _Ygl->vdp1wratio;
  polygon.vertices[3] = line_poygon[3] * _Ygl->vdp1hratio;
  polygon.vertices[4] = line_poygon[4] * _Ygl->vdp1wratio;
  polygon.vertices[5] = line_poygon[5] * _Ygl->vdp1hratio;
  polygon.vertices[6] = line_poygon[6] * _Ygl->vdp1wratio;
  polygon.vertices[7] = line_poygon[7] * _Ygl->vdp1hratio;

  polygon.w = 1;
  polygon.h = 1;
  polygon.flip = 0;

  polygon.blendmode = getCCProgramId(cmd.CMDPMOD);

  if (polygon.blendmode == -1) return; //Invalid color mode

  if (gouraud == 1) {
    YglQuadGrowShading(&polygon, &texture, col, NULL, YglTM_vdp1[_Ygl->drawframe]);
  }
  else {
    YglQuadGrowShading(&polygon, &texture, NULL, NULL, YglTM_vdp1[_Ygl->drawframe]);
  }
  Vdp1ReadCommand(&cmd, Vdp1Regs->addr, Vdp1Ram);
  *texture.textdata = Vdp1ReadPolygonColor(&cmd,varVdp2Regs);
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp1UserClipping(u8 * ram, Vdp1 * regs)
{
  Vdp1Regs->userclipX1 = Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0xC);
  Vdp1Regs->userclipY1 = Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0xE);
  Vdp1Regs->userclipX2 = Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0x14);
  Vdp1Regs->userclipY2 = Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0x16);
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp1SystemClipping(u8 * ram, Vdp1 * regs)
{
  Vdp1Regs->systemclipX1 = 0;
  Vdp1Regs->systemclipY1 = 0;
  Vdp1Regs->systemclipX2 = Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0x14);
  Vdp1Regs->systemclipY2 = Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0x16);
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp1LocalCoordinate(u8 * ram, Vdp1 * regs)
{
  Vdp1Regs->localX = Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0xC);
  Vdp1Regs->localY = Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0xE);
}

//////////////////////////////////////////////////////////////////////////////

int VIDOGLVdp2Reset(void)
{
  return 0;
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp2Draw(void)
{
  YglCheckFBSwitch(1);
  //varVdp2Regs = Vdp2RestoreRegs(0, Vdp2Lines);
  //if (varVdp2Regs == NULL) varVdp2Regs = Vdp2Regs;
  VIDOGLVdp2SetResolution(Vdp2Lines[0].TVMD);
  if (_Ygl->rwidth > YglTM_vdp2->width) {
    int new_width = _Ygl->rwidth;
    int new_height = YglTM_vdp2->height;
    YglTMDeInit(YglTM_vdp2);
    YglTM_vdp2 = YglTMInit(new_width, new_height);
  }
  YglTmPull(YglTM_vdp2, 0);

  if (Vdp2Regs->TVMD & 0x8000) {
    VIDOGLVdp2DrawScreens();
  }

  FrameProfileAdd("Vdp2Draw end");


  /* It would be better to reset manualchange in a Vdp1SwapFrameBuffer
  function that would be called here and during a manual change */
  //Vdp1External.manualchange = 0;
}

//////////////////////////////////////////////////////////////////////////////



static void Vdp2DrawBackScreen(Vdp2 *varVdp2Regs)
{
  u32 scrAddr;
  int dot;
  vdp2draw_struct info;

  static int line[512 * 4];

  if (varVdp2Regs->VRSIZE & 0x8000)
    scrAddr = (((varVdp2Regs->BKTAU & 0x7) << 16) | varVdp2Regs->BKTAL) * 2;
  else
    scrAddr = (((varVdp2Regs->BKTAU & 0x3) << 16) | varVdp2Regs->BKTAL) * 2;

  ReadVdp2ColorOffset(varVdp2Regs, &info, 0x20);

#if defined(__ANDROID__) || defined(_OGLES3_) || defined(_OGL3_)
  if ((varVdp2Regs->BKTAU & 0x8000) != 0 ) {
    // per line background color
    u32* back_pixel_data = YglGetBackColorPointer();
    if (back_pixel_data != NULL) {
      for (int i = 0; i < _Ygl->rheight; i++) {
        u8 r, g, b, a;
        dot = Vdp2RamReadWord(NULL, Vdp2Ram, (scrAddr + 2 * i));
        r = Y_MAX(((dot & 0x1F) << 3) + info.cor, 0);
        g = Y_MAX((((dot & 0x3E0) >> 5) << 3) + info.cog, 0);
        b = Y_MAX((((dot & 0x7C00) >> 10) << 3) + info.cob, 0);
        a = ((~varVdp2Regs->CCRLB & 0x1F00) >> 5)|NONE;
        *back_pixel_data++ = (a << 24) | ((b&0xFF) << 16) | ((g&0xFF) << 8) | (r&0xFF);
      }
      YglSetBackColor(_Ygl->rheight);
    }
  }
  else {
    dot = Vdp2RamReadWord(NULL, Vdp2Ram, scrAddr);
    YglSetClearColor(
      (float)(((dot & 0x1F) << 3) + info.cor) / (float)(0xFF),
      (float)((((dot & 0x3E0) >> 5) << 3) + info.cog) / (float)(0xFF),
      (float)((((dot & 0x7C00) >> 10) << 3) + info.cob) / (float)(0xFF)
    );
  }
#else
  if (varVdp2Regs->BKTAU & 0x8000)
  {
    int y;

    for (y = 0; y < _Ygl->rheight; y++)
    {
      dot = Vdp2RamReadWord(NULL, Vdp2Ram, scrAddr);
      scrAddr += 2;

      lineColors[3 * y + 0] = (dot & 0x1F) << 3;
      lineColors[3 * y + 1] = (dot & 0x3E0) >> 2;
      lineColors[3 * y + 2] = (dot & 0x7C00) >> 7;
      line[4 * y + 0] = 0;
      line[4 * y + 1] = y;
      line[4 * y + 2] = _Ygl->rwidth;
      line[4 * y + 3] = y;
    }

    glColorPointer(3, GL_UNSIGNED_BYTE, 0, lineColors);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(2, GL_INT, 0, line);
    glEnableClientState(GL_VERTEX_ARRAY);
    glDrawArrays(GL_LINES, 0, _Ygl->rheight * 2);
    glDisableClientState(GL_COLOR_ARRAY);
    glColor3ub(0xFF, 0xFF, 0xFF);
  }
  else
  {
    dot = Vdp2RamReadWord(NULL, Vdp2Ram, scrAddr);

    glColor3ub((dot & 0x1F) << 3, (dot & 0x3E0) >> 2, (dot & 0x7C00) >> 7);

    line[0] = 0;
    line[1] = 0;
    line[2] = _Ygl->rwidth;
    line[3] = 0;
    line[4] = _Ygl->rwidth;
    line[5] = _Ygl->rheight;
    line[6] = 0;
    line[7] = _Ygl->rheight;

    glVertexPointer(2, GL_INT, 0, line);
    glEnableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 8);
    glColor3ub(0xFF, 0xFF, 0xFF);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
}
#endif
  }

//////////////////////////////////////////////////////////////////////////////
// 11.3 Line Color insertion
//  7.1 Line Color Screen
static void Vdp2DrawLineColorScreen(Vdp2 *varVdp2Regs)
{

  u32 cacheaddr = 0xFFFFFFFF;
  int inc = 0;
  int line_cnt = _Ygl->rheight;
  int i;
  u32 * line_pixel_data;
  u32 addr;

  if (varVdp2Regs->LNCLEN == 0) return;

  line_pixel_data = YglGetLineColorPointer();
  if (line_pixel_data == NULL) {
    return;
  }

  if ((varVdp2Regs->LCTA.part.U & 0x8000)) {
    inc = 0x02; // single color
  }
  else {
    inc = 0x00; // color per line
  }

  u8 alpha = (varVdp2Regs->CCRLB & 0x1F) << 3;

  addr = (varVdp2Regs->LCTA.all & 0x7FFFF) * 0x2;
  for (i = 0; i < line_cnt; i++) {
    u16 LineColorRamAdress = Vdp2RamReadWord(NULL, Vdp2Ram, addr);
    *(line_pixel_data) = Vdp2ColorRamGetColor(LineColorRamAdress, alpha);
    line_pixel_data++;
    addr += inc;
  }

  YglSetLineColor(line_pixel_data, line_cnt);

}

void Vdp2GeneratePerLineColorCalcuration(vdp2draw_struct * info, int id, Vdp2 *varVdp2Regs) {
  int bit = 1 << id;
  int line = 0;
  int i;
  int displayedbyLine = 0;
  u8 first = info->display[info->startLine];
  int firstalpha = (Vdp2External.perline_alpha[info->startLine] & bit);
  for(i = info->startLine; i<info->endLine; i++) {
     if (first != info->display[i]) {
       displayedbyLine = 1;
       break;
     }
     if ((Vdp2External.perline_alpha[i] & bit) != firstalpha) {
       displayedbyLine = 1;
       break;
     }
  }
  if (displayedbyLine) {
    u32 * linebuf;
    int line_shift = 0;
    if (_Ygl->rheight > 256) {
      line_shift = 1;
    }
    else {
      line_shift = 0;
    }

    linebuf = YglGetPerlineBuf(&_Ygl->bg[id], _Ygl->rheight, 1);
    for (line = info->startLine, i=0; line < info->endLine; line++, i++) {
      if (info->display[line >> line_shift] == 0x0) {
        linebuf[i] = 0x00;
      }
      else {
        if (Vdp2Lines[line >> line_shift].CCCTL & bit)
        {

         switch (id) {
          case NBG0:
            linebuf[line] = ((~Vdp2Lines[line >> line_shift].CCRNA & 0x1F)<<3) << 24;
            break;
          case NBG1:
            linebuf[line] = ((~Vdp2Lines[line >> line_shift].CCRNA & 0x1F00) >> 5) << 24;
            break;
          case NBG2:
            linebuf[line] = ((~Vdp2Lines[line >> line_shift].CCRNB & 0x1F)<<3) << 24;
            break;
          case NBG3:
            linebuf[line] = ((~Vdp2Lines[line >> line_shift].CCRNB & 0x1F00) >> 5) << 24;
            break;
          case RBG0:
            linebuf[line] = ((~Vdp2Lines[line >> line_shift].CCRR & 0x1F)<<3) << 24;
            break;
          }

        }
        else {
          linebuf[line] = 0xF8000000;
        }

        if (Vdp2Lines[line >> line_shift].CLOFEN  & bit) {
          ReadVdp2ColorOffset(&Vdp2Lines[line >> line_shift], info, bit);
          linebuf[line] |= ((int)(128.0f + (info->cor / 2.0)) & 0xFF) << 16;
          linebuf[line] |= ((int)(128.0f + (info->cog / 2.0)) & 0xFF) << 8;
          linebuf[line] |= ((int)(128.0f + (info->cob / 2.0)) & 0xFF) << 0;
        }
        else {
          linebuf[line] |= 0x00808080;
        }

      }
    }
    YglSetPerlineBuf(&_Ygl->bg[id], linebuf, _Ygl->rheight, 1);
    info->lineTexture = _Ygl->bg[id].lincolor_tex;
  }
  else {
    info->lineTexture = 0;
  }

}


//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawRBG1_part(RBGDrawInfo *rgb, Vdp2* varVdp2Regs)
{
  YglTexture texture;
  YglCache tmpc;
  vdp2draw_struct* info = &rgb->info;

  info->dst = 0;
  info->idScreen = RBG1;
  info->idReg = 4;
  info->uclipmode = 0;
  info->cor = 0;
  info->cog = 0;
  info->cob = 0;
  info->specialcolorfunction = 0;

  int i;
  info->enable = 0;

  info->cellh = 256 << vdp2_interlace;
  info->specialcolorfunction = 0;

// RBG1 mode
  info->enable = ((varVdp2Regs->BGON & 0x20)!=0);
  // RBG1 shall not work without RGB0 but it looks like the HW is able to... MechWarrior 2 - 31st Century Combat - Arcade Combat Edition is using this capability...
  //if (!(varVdp2Regs->BGON & 0x10)) info->enable = 0; //When both R0ON and R1ON are 1, the normal scroll screen can no longer be displayed vdp2 pdf, section 4.1 Screen Display Control

  if (!info->enable) {
   free(rgb);
   return;
  }
  for (int i=info->startLine; i<info->endLine; i++) info->display[i] = info->enable;

    // Read in Parameter B
    Vdp2ReadRotationTable(1, &rgb->paraB, varVdp2Regs, Vdp2Ram);

    if ((info->isbitmap = varVdp2Regs->CHCTLA & 0x2) != 0)
    {
      // Bitmap Mode

      ReadBitmapSize(info, varVdp2Regs->CHCTLA >> 2, 0x3);
      if (vdp2_interlace) info->cellh *= 2;

      info->charaddr = (varVdp2Regs->MPOFR & 0x70) * 0x2000;
      info->paladdr = (varVdp2Regs->BMPNA & 0x7) << 4;
      info->flipfunction = 0;
      info->specialfunction = 0;
    }
    else
    {
      // Tile Mode
      info->mapwh = 4;
      ReadPlaneSize(info, varVdp2Regs->PLSZ >> 12);
      ReadPatternData(info, varVdp2Regs->PNCN0, varVdp2Regs->CHCTLA & 0x1);

      rgb->paraB.ShiftPaneX = 8 + info->planew;
      rgb->paraB.ShiftPaneY = 8 + info->planeh;
      rgb->paraB.MskH = (8 * 64 * info->planew) - 1;
      rgb->paraB.MskV = (8 * 64 * info->planeh) - 1;
      rgb->paraB.MaxH = 8 * 64 * info->planew * 4;
      rgb->paraB.MaxV = 8 * 64 * info->planeh * 4;

    }

    info->rotatenum = 1;
    //rgb->paraB.PlaneAddr = (void FASTCALL(*)(void *, int, Vdp2*))&Vdp2ParameterBPlaneAddr;
    rgb->paraB.coefenab = varVdp2Regs->KTCTL & 0x100;
    rgb->paraB.charaddr = (varVdp2Regs->MPOFR & 0x70) * 0x2000;
    ReadPlaneSizeR(&rgb->paraB, varVdp2Regs->PLSZ >> 12);
    for (i = 0; i < 16; i++)
    {
      Vdp2ParameterBPlaneAddr(info, i, varVdp2Regs);
      rgb->paraB.PlaneAddrv[i] = info->addr;
    }

    info->LineColorBase = 0x00;
    if (rgb->paraB.coefenab)
      info->GetRParam = (Vdp2GetRParam_func)vdp2RGetParamMode01WithK;
    else
      info->GetRParam = (Vdp2GetRParam_func)vdp2RGetParamMode01NoK;

    if (rgb->paraB.coefdatasize == 2)
    {
      if (rgb->paraB.coefmode < 3)
      {
        info->GetKValueB = vdp2rGetKValue1W;
      }
      else {
        info->GetKValueB = vdp2rGetKValue1Wm3;
      }

    }
    else {
      if (rgb->paraB.coefmode < 3)
      {
        info->GetKValueB = vdp2rGetKValue2W;
      }
      else {
        info->GetKValueB = vdp2rGetKValue2Wm3;
      }
    }


  ReadMosaicData(info, 0x1, varVdp2Regs);

  info->transparencyenable = !(varVdp2Regs->BGON & 0x100);
  info->specialprimode = varVdp2Regs->SFPRMD & 0x3;
  info->specialcolormode = varVdp2Regs->SFCCMD & 0x3;
  if (varVdp2Regs->SFSEL & 0x1)
    info->specialcode = varVdp2Regs->SFCODE >> 8;
  else
    info->specialcode = varVdp2Regs->SFCODE & 0xFF;

  info->colornumber = (varVdp2Regs->CHCTLA & 0x70) >> 4;
  int dest_alpha = ((varVdp2Regs->CCCTL >> 9) & 0x01);

  info->blendmode = 0;

  // Color calculation ratio
  info->alpha = (~varVdp2Regs->CCRNA & 0x1F)<<3;


  Vdp2GeneratePerLineColorCalcuration(info, NBG0, varVdp2Regs);
  _Ygl->perLine[RBG1] = info->lineTexture;

  info->coloroffset = (varVdp2Regs->CRAOFA & 0x7) << 8;
  ReadVdp2ColorOffset(varVdp2Regs, info, 0x1);
  if (info->lineTexture != 0) {
    info->cor = 0;
    info->cog = 0;
    info->cob = 0;
  }
  info->linecheck_mask = 0x01;
  info->priority = varVdp2Regs->PRINA & 0x7;

  if (((Vdp2External.disptoggle & 0x20)==0) || (info->priority == 0)) {
    free(rgb);
    return;
  }

  ReadLineScrollData(info, varVdp2Regs->SCRCTL & 0xFF, varVdp2Regs->LSTA0.all);
  info->lineinfo = lineNBG0;
  Vdp2GenLineinfo(info);
  if (varVdp2Regs->SCRCTL & 1)
  {
    info->isverticalscroll = 1;
    info->verticalscrolltbl = (varVdp2Regs->VCSTA.all & 0x7FFFE) << 1;
    if (varVdp2Regs->SCRCTL & 0x100)
      info->verticalscrollinc = 8;
    else
      info->verticalscrollinc = 4;
  }
  else
    info->isverticalscroll = 0;

  // RBG1 draw
  Vdp2DrawRotation(rgb, varVdp2Regs);
  free(rgb);
}

int sameVDP2RegRBG0(Vdp2 *a, Vdp2 *b)
{
  if ((a->BGON & 0x1010) != (b->BGON & 0x1010)) return 0;
//  if ((a->PRIR & 0x7) != (b->PRIR & 0x7)) return 0;
//  if ((a->CCCTL & 0xFF10) != (b->CCCTL & 0xFF10)) return 0;
//  if ((a->SFPRMD & 0x300) != (b->SFPRMD & 0x300)) return 0;
//  if ((a->CHCTLB & 0x7700) != (b->CHCTLB & 0x7700)) return 0;
//  if ((a->WCTLC & 0xFF) != (b->WCTLC & 0xFF)) return 0;
  if ((a->RPTA.all) != (b->RPTA.all)) return 0;
//  if ((a->VRSIZE & 0x8000) != (b->VRSIZE & 0x8000)) return 0;
//  if ((a->RAMCTL & 0x80FF) != (b->RAMCTL & 0x80FF)) return 0;
//  if ((a->KTCTL & 0xFFFF) != (b->KTCTL & 0xFFFF)) return 0;
//  if ((a->PLSZ & 0xFF00) != (b->PLSZ & 0xFF00)) return 0;
//  if ((a->KTAOF & 0x707) != (b->KTAOF & 0x707)) return 0;
//  if ((a->MPOFR & 0x77) != (b->MPOFR & 0x77)) return 0;
  if ((a->RPMD & 0x3) != (b->RPMD & 0x3)) return 0;
//  if ((a->WCTLD & 0xF) != (b->WCTLD & 0xF)) return 0;
//  if ((a->BMPNB & 0x7) != (b->BMPNB & 0x7)) return 0;
//  if ((a->PNCR & 0xFFFF) != (b->PNCR & 0xFFFF)) return 0;
//  if ((a->MZCTL & 0xFF10) != (b->MZCTL & 0xFF10)) return 0;
//  if ((a->SFCCMD &0x300) != (b->SFCCMD &0x300)) return 0;
//  if ((a->SFSEL & 0x10) != (b->SFSEL & 0x10)) return 0;
//  if ((a->SFCODE & 0xFFFF) != (b->SFCODE & 0xFFFF)) return 0;
//  if ((a->LNCLEN & 0x10) != (b->LNCLEN & 0x10)) return 0;
//  if ((a->LCTA.all) != (b->LCTA.all)) return 0;
//  if ((a->CRAOFB & 0x7) != (b->CRAOFB & 0x7)) return 0;
//  if ((a->CLOFSL & 0x10) != (b->CLOFSL & 0x10)) return 0;
//  if ((a->COBR & 0x1FF) != (b->COBR & 0x1FF)) return 0;
//  if ((a->COBG & 0x1FF) != (b->COBG & 0x1FF)) return 0;
//  if ((a->COBB & 0x1FF) != (b->COBB & 0x1FF)) return 0;
//  if ((a->COAR & 0x1FF) != (b->COAR & 0x1FF)) return 0;
//  if ((a->COAG & 0x1FF) != (b->COAG & 0x1FF)) return 0;
//  if ((a->COAB & 0x1FF) != (b->COAB & 0x1FF)) return 0;
  return 1;
}

int sameVDP2RegRBG1(Vdp2 *a, Vdp2 *b)
{

  if ((a->BGON & 0x130) != (b->BGON & 0x130)) return 0;
//  if ((a->PRINA & 0x7) != (b->PRINA & 0x7)) return 0;
//  if ((a->CCCTL & 0xFF01) != (b->CCCTL & 0xFF01)) return 0;
//  if ((a->BMPNA & 0x7) != (b->BMPNA & 0x7)) return 0;
//  if ((a->MPOFR & 0x77) != (b->MPOFR & 0x77)) return 0;
//  if ((a->VRSIZE & 0x8000) != (b->VRSIZE & 0x8000)) return 0;
//  if ((a->RAMCTL & 0x80FF) != (b->RAMCTL & 0x80FF)) return 0;
//  if ((a->KTCTL & 0xFFFF) != (b->KTCTL & 0xFFFF)) return 0;
//  if ((a->PLSZ & 0xFF00) != (b->PLSZ & 0xFF00)) return 0;
//  if ((a->SFPRMD & 0x3) != (b->SFPRMD & 0x3)) return 0;
//  if ((a->CHCTLA & 0x7F) != (b->CHCTLA & 0x7F)) return 0;
//  if ((a->MZCTL & 0xFF01) != (b->MZCTL & 0xFF01)) return 0;
//  if ((a->SFCCMD &0x3) != (b->SFCCMD &0x3)) return 0;
//  if ((a->SFSEL & 0x1) != (b->SFSEL & 0x1)) return 0;
//  if ((a->SFCODE & 0xFFFF) != (b->SFCODE & 0xFFFF)) return 0;
//  if ((a->LNCLEN & 0x1) != (b->LNCLEN & 0x1)) return 0;
//  if ((a->CRAOFA & 0x7) != (b->CRAOFA & 0x7)) return 0;
//  if ((a->CLOFSL & 0x1) != (b->CLOFSL & 0x1)) return 0;
//  if ((a->WCTLA & 0xFF) != (b->WCTLA & 0xFF)) return 0;
//  if ((a->PNCN0 & 0xFFFF) != (b->PNCN0 & 0xFFFF)) return 0;
//  if ((a->SCRCTL & 0x3F) != (b->SCRCTL & 0x3F)) return 0;
//  if ((a->LSTA0.all) != (b->LSTA0.all)) return 0;
//  if ((a->VCSTA.all) != (b->VCSTA.all)) return 0;
  if ((a->RPTA.all) != (b->RPTA.all)) return 0;
//  if ((a->WCTLD & 0xF) != (b->WCTLD & 0xF)) return 0;
//  if ((a->LCTA.all) != (b->LCTA.all)) return 0;
//  if ((a->COBR & 0x1FF) != (b->COBR & 0x1FF)) return 0;
//  if ((a->COBG & 0x1FF) != (b->COBG & 0x1FF)) return 0;
//  if ((a->COBB & 0x1FF) != (b->COBB & 0x1FF)) return 0;
//  if ((a->COAR & 0x1FF) != (b->COAR & 0x1FF)) return 0;
//  if ((a->COAG & 0x1FF) != (b->COAG & 0x1FF)) return 0;
//  if ((a->COAB & 0x1FF) != (b->COAB & 0x1FF)) return 0;
  return 1;
}

int sameVDP2Reg(int id, Vdp2 *a, Vdp2 *b)
{
  switch (id) {
    case RBG0: return sameVDP2RegRBG0(a, b);
    case RBG1: return sameVDP2RegRBG1(a, b);
    default:
    break;
  }
  return 1;
}

static void Vdp2DrawRBG1(Vdp2 *varVdp2Regs)
{
  int nbZone = 1;
  int lastLine = 0;
  int line;
  int max = (yabsys.VBlankLineCount >= 270)?270:yabsys.VBlankLineCount;
  RBGDrawInfo *rgb;
  for (line = 2; line<max; line++) {
    if (!sameVDP2Reg(RBG1, &Vdp2Lines[line-1], &Vdp2Lines[line])) {
      rgb = (RBGDrawInfo *)calloc(1, sizeof(RBGDrawInfo));
      rgb->rgb_type = 0x04;
      rgb->use_cs = _Ygl->rbg_use_compute_shader;
      rgb->info.startLine = lastLine;
      rgb->info.endLine = line;
      lastLine = line;
      LOG_AREA("RBG1 Draw from %d to %d %x\n", rgb->info.startLine, rgb->info.endLine, varVdp2Regs->BGON);
      Vdp2DrawRBG1_part(rgb, &Vdp2Lines[rgb->info.startLine]);
    }
  }
  rgb = (RBGDrawInfo *)calloc(1, sizeof(RBGDrawInfo));
  rgb->rgb_type = 0x04;
  rgb->use_cs = _Ygl->rbg_use_compute_shader;
  rgb->info.startLine = lastLine;
  rgb->info.endLine = line;
  LOG_AREA("RBG1 Draw from %d to %d %x\n", rgb->info.startLine, rgb->info.endLine, varVdp2Regs->BGON);
  Vdp2DrawRBG1_part(rgb, &Vdp2Lines[rgb->info.startLine]);

}

static void Vdp2DrawNBG0(Vdp2* varVdp2Regs) {
  vdp2draw_struct info = {0};
  YglTexture texture;
  YglCache tmpc;
  info.dst = 0;
  info.uclipmode = 0;
  info.idScreen = NBG0;
  info.idReg = 0;
  info.coordincx = 1.0f;
  info.coordincy = 1.0f;

  info.cor = 0;
  info.cog = 0;
  info.cob = 0;
  int i;
  info.enable = 0;
  info.startLine = 0;
  info.endLine = (yabsys.VBlankLineCount < 270)?yabsys.VBlankLineCount:270;

  info.cellh = 256 << vdp2_interlace;
  info.specialcolorfunction = 0;

    // NBG0 mode
  for (i=0; i<yabsys.VBlankLineCount; i++) {
    info.display[i] = ((Vdp2Lines[i].BGON & 0x1)!=0);
    if ((Vdp2Lines[i].BGON & 0x20) && (Vdp2Lines[i].BGON & 0x10)) info.display[i] = 0; //When both R0ON and R1ON are 1, the normal scroll screen can no longer be displayed vdp2 pdf, section 4.1 Screen Display Control
    info.enable |= info.display[i];
  }
    if (!info.enable) return;

    if ((info.isbitmap = varVdp2Regs->CHCTLA & 0x2) != 0)
    {
      // Bitmap Mode

      ReadBitmapSize(&info, varVdp2Regs->CHCTLA >> 2, 0x3);
      if (vdp2_interlace) info.cellh *= 2;

      info.x = -((varVdp2Regs->SCXIN0 & 0x7FF) % info.cellw);
      info.y = -((varVdp2Regs->SCYIN0 & 0x7FF) % info.cellh);

      info.charaddr = (varVdp2Regs->MPOFN & 0x7) * 0x20000;
      info.paladdr = (varVdp2Regs->BMPNA & 0x7) << 4;
      info.flipfunction = 0;
      info.specialcolorfunction = (varVdp2Regs->BMPNA & 0x10) >> 4;
      info.specialfunction = (varVdp2Regs->BMPNA >> 5) & 0x01;
    }
    else
    {
      // Tile Mode
      info.mapwh = 2;

      ReadPlaneSize(&info, varVdp2Regs->PLSZ);

      info.x = -((varVdp2Regs->SCXIN0 & 0x7FF) % (512 * info.planew));
      info.y = -((varVdp2Regs->SCYIN0 & 0x7FF) % (512 * info.planeh));
      ReadPatternData(&info, varVdp2Regs->PNCN0, varVdp2Regs->CHCTLA & 0x1);
    }

    if ((varVdp2Regs->ZMXN0.all & 0x7FF00) == 0)
      info.coordincx = 1.0f;
    else
      info.coordincx = (float)65536 / (varVdp2Regs->ZMXN0.all & 0x7FF00);

    switch (varVdp2Regs->ZMCTL & 0x03)
    {
    case 0:
      info.maxzoom = 1.0f;
      break;
    case 1:
      info.maxzoom = 0.5f;
      //if( info.coordincx < 0.5f )  info.coordincx = 0.5f;
      break;
    case 2:
    case 3:
      info.maxzoom = 0.25f;
      //if( info.coordincx < 0.25f )  info.coordincx = 0.25f;
      break;
    }

    if ((varVdp2Regs->ZMYN0.all & 0x7FF00) == 0)
      info.coordincy = 1.0f;
    else
      info.coordincy = (float)65536 / (varVdp2Regs->ZMYN0.all & 0x7FF00);

    info.PlaneAddr = (void FASTCALL(*)(void *, int, Vdp2*))&Vdp2NBG0PlaneAddr;


  ReadMosaicData(&info, 0x1, varVdp2Regs);

  info.transparencyenable = !(varVdp2Regs->BGON & 0x100);
  info.specialprimode = varVdp2Regs->SFPRMD & 0x3;
  info.specialcolormode = varVdp2Regs->SFCCMD & 0x3;

  if (varVdp2Regs->SFSEL & 0x1)
    info.specialcode = varVdp2Regs->SFCODE >> 8;
  else
    info.specialcode = varVdp2Regs->SFCODE & 0xFF;

  info.colornumber = (varVdp2Regs->CHCTLA & 0x70) >> 4;

  int dest_alpha = ((varVdp2Regs->CCCTL >> 9) & 0x01);

  info.blendmode = 0;

  info.alpha = (~varVdp2Regs->CCRNA & 0x1F) << 3;

  Vdp2GeneratePerLineColorCalcuration(&info, NBG0, varVdp2Regs);
  _Ygl->perLine[NBG0] = info.lineTexture;

  info.coloroffset = (varVdp2Regs->CRAOFA & 0x7) << 8;
  ReadVdp2ColorOffset(varVdp2Regs, &info, 0x1);
  if (info.lineTexture != 0) {
    info.cor = 0;
    info.cog = 0;
    info.cob = 0;
  }
  info.linecheck_mask = 0x01;
  info.priority = varVdp2Regs->PRINA & 0x7;

  if (((Vdp2External.disptoggle & 0x1)==0) || (info.priority == 0))
    return;

  ReadLineScrollData(&info, varVdp2Regs->SCRCTL & 0xFF, varVdp2Regs->LSTA0.all);
  info.lineinfo = lineNBG0;
  Vdp2GenLineinfo(&info);

  if (varVdp2Regs->SCRCTL & 1)
  {
    info.isverticalscroll = 1;
    info.verticalscrolltbl = (varVdp2Regs->VCSTA.all & 0x7FFFE) << 1;
    if (varVdp2Regs->SCRCTL & 0x100)
      info.verticalscrollinc = 8;
    else
      info.verticalscrollinc = 4;
  }
  else
    info.isverticalscroll = 0;

// NBG0 draw
    if (info.isbitmap)
    {
      if (info.coordincx != 1.0f || info.coordincy != 1.0f || VDPLINE_SZ(info.islinescroll)) {
        info.sh = (varVdp2Regs->SCXIN0 & 0x7FF);
        info.sv = (varVdp2Regs->SCYIN0 & 0x7FF);
        info.x = 0;
        info.y = 0;
        info.vertices[0] = 0;
        info.vertices[1] = 0;
        info.vertices[2] = _Ygl->rwidth;
        info.vertices[3] = 0;
        info.vertices[4] = _Ygl->rwidth;
        info.vertices[5] = _Ygl->rheight;
        info.vertices[6] = 0;
        info.vertices[7] = _Ygl->rheight;
        vdp2draw_struct infotmp = info;
        infotmp.cellw = _Ygl->rwidth;
        if (_Ygl->rheight >= 448)
          infotmp.cellh = (_Ygl->rheight >> 1) << vdp2_interlace;
        else
          infotmp.cellh = _Ygl->rheight << vdp2_interlace;
        YglQuad(&infotmp, &texture, &tmpc, YglTM_vdp2);
        Vdp2DrawBitmapCoordinateInc(&info, &texture, varVdp2Regs);
      }
      else {

        int xx, yy;
        int isCached = 0;

        if (info.islinescroll) // Nights Movie
        {
          info.sh = (varVdp2Regs->SCXIN0 & 0x7FF);
          info.sv = (varVdp2Regs->SCYIN0 & 0x7FF);
          info.x = 0;
          info.y = 0;
          info.vertices[0] = 0;
          info.vertices[1] = 0;
          info.vertices[2] = _Ygl->rwidth;
          info.vertices[3] = 0;
          info.vertices[4] = _Ygl->rwidth;
          info.vertices[5] = _Ygl->rheight;
          info.vertices[6] = 0;
          info.vertices[7] = _Ygl->rheight;
          vdp2draw_struct infotmp = info;
          infotmp.cellw = _Ygl->rwidth;
          infotmp.cellh = _Ygl->rheight << vdp2_interlace;
          YglQuad(&infotmp, &texture, &tmpc, YglTM_vdp2);
          Vdp2DrawBitmapLineScroll(&info, &texture, _Ygl->rwidth, _Ygl->rheight, varVdp2Regs);

        }
        else {
          yy = info.y;
          while (yy + info.y < _Ygl->rheight)
          {
            xx = info.x;
            while (xx + info.x < _Ygl->rwidth)
            {
              info.vertices[0] = xx;
              info.vertices[1] = yy;
              info.vertices[2] = (xx + info.cellw);
              info.vertices[3] = yy;
              info.vertices[4] = (xx + info.cellw);
              info.vertices[5] = (yy + info.cellh);
              info.vertices[6] = xx;
              info.vertices[7] = (yy + info.cellh);
              if (isCached == 0)
              {
                YglQuad(&info, &texture, &tmpc, YglTM_vdp2);
                if (info.islinescroll) {
                  Vdp2DrawBitmapLineScroll(&info, &texture, info.cellw, info.cellh, varVdp2Regs);
                } else {
                  requestDrawCell(&info, &texture, varVdp2Regs);
                }
                isCached = 1;
              }
              else {
                YglCachedQuad(&info, &tmpc, YglTM_vdp2);
              }
              xx += info.cellw;
            }
            yy += info.cellh;
          }
        }
      }
    }
    else
    {
      if (info.islinescroll) {
        info.x = varVdp2Regs->SCXIN0 & 0x7FF;
        info.y = varVdp2Regs->SCYIN0 & 0x7FF;
        Vdp2DrawMapPerLine(&info, &texture, varVdp2Regs);
      }
      else {
        info.x = varVdp2Regs->SCXIN0 & 0x7FF;
        info.y = varVdp2Regs->SCYIN0 & 0x7FF;
        Vdp2DrawMapTest(&info, &texture, varVdp2Regs);
      }
    }
  executeDrawCell();
}

static void Vdp2DrawNBG0RBG1(Vdp2* varVdp2Regs)
{
  Vdp2DrawRBG1(varVdp2Regs);
  Vdp2DrawNBG0(varVdp2Regs);
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawNBG1(Vdp2* varVdp2Regs)
{
  vdp2draw_struct info;
  YglTexture texture;
  YglCache tmpc;
  info.dst = 0;
  info.idScreen = NBG1;
  info.idReg = 1;
  info.uclipmode = 0;
  info.cor = 0;
  info.cog = 0;
  info.cob = 0;
  info.specialcolorfunction = 0;
  info.enable = 0;
  info.startLine = 0;
  info.endLine = (yabsys.VBlankLineCount < 270)?yabsys.VBlankLineCount:270;

  for (int i=0; i<yabsys.VBlankLineCount; i++) {
    info.display[i] = ((Vdp2Lines[i].BGON & 0x2)!=0);
    if ((Vdp2Lines[i].BGON & 0x20) && (Vdp2Lines[i].BGON & 0x10)) info.display[i] = 0; //When both R0ON and R1ON are 1, the normal scroll screen can no longer be displayed vdp2 pdf, section 4.1 Screen Display Control
    info.enable |= info.display[i];
  }
  if (!info.enable) return;
  info.transparencyenable = !(varVdp2Regs->BGON & 0x200);
  info.specialprimode = (varVdp2Regs->SFPRMD >> 2) & 0x3;

  info.colornumber = (varVdp2Regs->CHCTLA & 0x3000) >> 12;

  if ((info.isbitmap = varVdp2Regs->CHCTLA & 0x200) != 0)
  {

    ReadBitmapSize(&info, varVdp2Regs->CHCTLA >> 10, 0x3);

    info.x = -((varVdp2Regs->SCXIN1 & 0x7FF) % info.cellw);
    info.y = -((varVdp2Regs->SCYIN1 & 0x7FF) % info.cellh);
    info.charaddr = ((varVdp2Regs->MPOFN & 0x70) >> 4) * 0x20000;
    info.paladdr = (varVdp2Regs->BMPNA & 0x700) >> 4;
    info.flipfunction = 0;
    info.specialfunction = 0;
    info.specialcolorfunction = (varVdp2Regs->BMPNA & 0x1000) >> 4;
  }
  else
  {
    info.mapwh = 2;

    ReadPlaneSize(&info, varVdp2Regs->PLSZ >> 2);

    info.x = -((varVdp2Regs->SCXIN1 & 0x7FF) % (512 * info.planew));
    info.y = -((varVdp2Regs->SCYIN1 & 0x7FF) % (512 * info.planeh));

    ReadPatternData(&info, varVdp2Regs->PNCN1, varVdp2Regs->CHCTLA & 0x100);
  }

  info.specialcolormode = (varVdp2Regs->SFCCMD >> 2) & 0x3;

  if (varVdp2Regs->SFSEL & 0x2)
    info.specialcode = varVdp2Regs->SFCODE >> 8;
  else
    info.specialcode = varVdp2Regs->SFCODE & 0xFF;

  ReadMosaicData(&info, 0x2, varVdp2Regs);


  info.blendmode = 0;

  info.alpha = ((~varVdp2Regs->CCRNA & 0x1F00) >> 5);

  Vdp2GeneratePerLineColorCalcuration(&info, NBG1, varVdp2Regs);
  _Ygl->perLine[NBG1] = info.lineTexture;

  info.coloroffset = (varVdp2Regs->CRAOFA & 0x70) << 4;
  ReadVdp2ColorOffset(varVdp2Regs, &info, 0x2);
  if (info.lineTexture != 0) {
    info.cor = 0;
    info.cog = 0;
    info.cob = 0;
  }
  info.linecheck_mask = 0x02;

  if ((varVdp2Regs->ZMXN1.all & 0x7FF00) == 0)
    info.coordincx = 1.0f;
  else
    info.coordincx = (float)65536 / (varVdp2Regs->ZMXN1.all & 0x7FF00);

  switch ((varVdp2Regs->ZMCTL >> 8) & 0x03)
  {
  case 0:
    info.maxzoom = 1.0f;
    break;
  case 1:
    info.maxzoom = 0.5f;
    //      if( info.coordincx < 0.5f )  info.coordincx = 0.5f;
    break;
  case 2:
  case 3:
    info.maxzoom = 0.25f;
    //      if( info.coordincx < 0.25f )  info.coordincx = 0.25f;
    break;
  }
  if ((varVdp2Regs->ZMYN1.all & 0x7FF00) == 0)
    info.coordincy = 1.0f;
  else
    info.coordincy = (float)65536 / (varVdp2Regs->ZMYN1.all & 0x7FF00);


  info.priority = (varVdp2Regs->PRINA >> 8) & 0x7;

  info.PlaneAddr = (void FASTCALL(*)(void *, int, Vdp2*))&Vdp2NBG1PlaneAddr;

  if (((Vdp2External.disptoggle & 0x2)==0) || (info.priority == 0) ||
    (varVdp2Regs->BGON & 0x1 && (varVdp2Regs->CHCTLA & 0x70) >> 4 == 4)) // If NBG0 16M mode is enabled, don't draw
    return;

  ReadLineScrollData(&info, varVdp2Regs->SCRCTL >> 8, varVdp2Regs->LSTA1.all);
  info.lineinfo = lineNBG1;
  Vdp2GenLineinfo(&info);
  if (varVdp2Regs->SCRCTL & 0x100)
  {
    info.isverticalscroll = 1;
    if (varVdp2Regs->SCRCTL & 0x1)
    {
      info.verticalscrolltbl = 4 + ((varVdp2Regs->VCSTA.all & 0x7FFFE) << 1);
      info.verticalscrollinc = 8;
    }
    else
    {
      info.verticalscrolltbl = (varVdp2Regs->VCSTA.all & 0x7FFFE) << 1;
      info.verticalscrollinc = 4;
    }
  }
  else
    info.isverticalscroll = 0;

  if (info.isbitmap)
  {

    if (info.coordincx != 1.0f || info.coordincy != 1.0f) {
      info.sh = (varVdp2Regs->SCXIN1 & 0x7FF);
      info.sv = (varVdp2Regs->SCYIN1 & 0x7FF);
      info.x = 0;
      info.y = 0;
      info.vertices[0] = 0;
      info.vertices[1] = 0;
      info.vertices[2] = _Ygl->rwidth;
      info.vertices[3] = 0;
      info.vertices[4] = _Ygl->rwidth;
      info.vertices[5] = _Ygl->rheight;
      info.vertices[6] = 0;
      info.vertices[7] = _Ygl->rheight;
      vdp2draw_struct infotmp = info;
      infotmp.cellw = _Ygl->rwidth;
      if (_Ygl->rheight >= 448)
        infotmp.cellh = (_Ygl->rheight >> 1);
      else
        infotmp.cellh = _Ygl->rheight;
      YglQuad(&infotmp, &texture, &tmpc, YglTM_vdp2);
      Vdp2DrawBitmapCoordinateInc(&info, &texture, varVdp2Regs);
    }
    else {

      int xx, yy;
      int isCached = 0;

      if (info.islinescroll) // Nights Movie
      {
        info.sh = (varVdp2Regs->SCXIN1 & 0x7FF);
        info.sv = (varVdp2Regs->SCYIN1 & 0x7FF);
        info.x = 0;
        info.y = 0;
        info.vertices[0] = 0;
        info.vertices[1] = 0;
        info.vertices[2] = _Ygl->rwidth;
        info.vertices[3] = 0;
        info.vertices[4] = _Ygl->rwidth;
        info.vertices[5] = _Ygl->rheight;
        info.vertices[6] = 0;
        info.vertices[7] = _Ygl->rheight;
        vdp2draw_struct infotmp = info;
        infotmp.cellw = _Ygl->rwidth;
        infotmp.cellh = _Ygl->rheight;
        YglQuad(&infotmp, &texture, &tmpc, YglTM_vdp2);
        Vdp2DrawBitmapLineScroll(&info, &texture, _Ygl->rwidth, _Ygl->rheight, varVdp2Regs);

      }
      else {
        yy = info.y;
        while (yy + info.y < _Ygl->rheight)
        {
          xx = info.x;
          while (xx + info.x < _Ygl->rwidth)
          {
            info.vertices[0] = xx;
            info.vertices[1] = yy;
            info.vertices[2] = (xx + info.cellw);
            info.vertices[3] = yy;
            info.vertices[4] = (xx + info.cellw);
            info.vertices[5] = (yy + info.cellh);
            info.vertices[6] = xx;
            info.vertices[7] = (yy + info.cellh);
            if (isCached == 0)
            {
              YglQuad(&info, &texture, &tmpc, YglTM_vdp2);
              if (info.islinescroll) {
                Vdp2DrawBitmapLineScroll(&info, &texture, info.cellw, info.cellh, varVdp2Regs);
              }
              else {
                requestDrawCell(&info, &texture, varVdp2Regs);
              }
              isCached = 1;
            }
            else {
              YglCachedQuad(&info, &tmpc, YglTM_vdp2);
            }
            xx += info.cellw;
          }
          yy += info.cellh;
        }
      }
    }
  }
  else {
    if (info.islinescroll) {
      info.x = (varVdp2Regs->SCXIN1 & 0x7FF);
      info.y = (varVdp2Regs->SCYIN1 & 0x7FF);
      Vdp2DrawMapPerLine(&info, &texture, varVdp2Regs);
    }
    else {
      //Vdp2DrawMap(&info, &texture);
      info.x = varVdp2Regs->SCXIN1 & 0x7FF;
      info.y = varVdp2Regs->SCYIN1 & 0x7FF;
      Vdp2DrawMapTest(&info, &texture, varVdp2Regs);
    }
  }
  executeDrawCell();
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawNBG2(Vdp2* varVdp2Regs)
{
  vdp2draw_struct info;
  YglTexture texture;
  info.dst = 0;
  info.idScreen = NBG2;
  info.idReg = 2;
  info.uclipmode = 0;
  info.cor = 0;
  info.cog = 0;
  info.cob = 0;
  info.specialcolorfunction = 0;
  info.blendmode = 0;
  info.enable = 0;
  info.startLine = 0;
  info.endLine = (yabsys.VBlankLineCount < 270)?yabsys.VBlankLineCount:270;

  for (int i=0; i<yabsys.VBlankLineCount; i++) {
    info.display[i] = ((Vdp2Lines[i].BGON & 0x4)!=0);
    if ((Vdp2Lines[i].BGON & 0x20) && (Vdp2Lines[i].BGON & 0x10)) info.display[i] = 0; //When both R0ON and R1ON are 1, the normal scroll screen can no longer be displayed vdp2 pdf, section 4.1 Screen Display Control
    info.enable |= info.display[i];
  }
  if (!info.enable) return;

  info.transparencyenable = !(varVdp2Regs->BGON & 0x400);
  info.specialprimode = (varVdp2Regs->SFPRMD >> 4) & 0x3;

  info.colornumber = (varVdp2Regs->CHCTLB & 0x2) >> 1;
  info.mapwh = 2;

  ReadPlaneSize(&info, varVdp2Regs->PLSZ >> 4);
  info.x = -((varVdp2Regs->SCXN2 & 0x7FF) % (512 * info.planew));
  info.y = -((varVdp2Regs->SCYN2 & 0x7FF) % (512 * info.planeh));
  ReadPatternData(&info, varVdp2Regs->PNCN2, varVdp2Regs->CHCTLB & 0x1);

  ReadMosaicData(&info, 0x4, varVdp2Regs);

  info.specialcolormode = (varVdp2Regs->SFCCMD >> 4) & 0x3;

  if (varVdp2Regs->SFSEL & 0x4)
    info.specialcode = varVdp2Regs->SFCODE >> 8;
  else
    info.specialcode = varVdp2Regs->SFCODE & 0xFF;

  info.alpha = (~varVdp2Regs->CCRNB & 0x1F) << 3;

  Vdp2GeneratePerLineColorCalcuration(&info, NBG2, varVdp2Regs);
  _Ygl->perLine[NBG2] = info.lineTexture;

  info.coloroffset = varVdp2Regs->CRAOFA & 0x700;
  ReadVdp2ColorOffset(varVdp2Regs, &info, 0x4);

  if (info.lineTexture != 0) {
    info.cor = 0;
    info.cog = 0;
    info.cob = 0;
  }


  info.linecheck_mask = 0x04;
  info.coordincx = info.coordincy = 1;
  info.priority = varVdp2Regs->PRINB & 0x7;
  info.PlaneAddr = (void FASTCALL(*)(void *, int, Vdp2*))&Vdp2NBG2PlaneAddr;

  if (((Vdp2External.disptoggle & 0x4)==0) || (info.priority == 0) ||
    (varVdp2Regs->BGON & 0x1 && (varVdp2Regs->CHCTLA & 0x70) >> 4 >= 2)) // If NBG0 2048/32786/16M mode is enabled, don't draw
    return;

  info.islinescroll = 0;
  info.linescrolltbl = 0;
  info.lineinc = 0;
  info.isverticalscroll = 0;
  info.x = varVdp2Regs->SCXN2 & 0x7FF;
  info.y = varVdp2Regs->SCYN2 & 0x7FF;
  Vdp2DrawMapTest(&info, &texture, varVdp2Regs);
  executeDrawCell();

}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawNBG3(Vdp2* varVdp2Regs)
{
  vdp2draw_struct info;
  YglTexture texture;
  info.idScreen = NBG3;
  info.idReg = 3;
  info.dst = 0;
  info.uclipmode = 0;
  info.cor = 0;
  info.cog = 0;
  info.cob = 0;
  info.specialcolorfunction = 0;
  info.blendmode = 0;
  info.enable = 0;
  info.startLine = 0;
  info.endLine = (yabsys.VBlankLineCount < 270)?yabsys.VBlankLineCount:270;

  for (int i=0; i<yabsys.VBlankLineCount; i++) {
    info.display[i] = ((Vdp2Lines[i].BGON & 0x8)!=0);
    if ((Vdp2Lines[i].BGON & 0x20) && (Vdp2Lines[i].BGON & 0x10)) info.display[i] = 0; //When both R0ON and R1ON are 1, the normal scroll screen can no longer be displayed vdp2 pdf, section 4.1 Screen Display Control
    info.enable |= info.display[i];
  }
  if (!info.enable) return;
  info.transparencyenable = !(varVdp2Regs->BGON & 0x800);
  info.specialprimode = (varVdp2Regs->SFPRMD >> 6) & 0x3;

  info.colornumber = (varVdp2Regs->CHCTLB & 0x20) >> 5;

  info.mapwh = 2;

  ReadPlaneSize(&info, varVdp2Regs->PLSZ >> 6);
  info.x = -((varVdp2Regs->SCXN3 & 0x7FF) % (512 * info.planew));
  info.y = -((varVdp2Regs->SCYN3 & 0x7FF) % (512 * info.planeh));
  ReadPatternData(&info, varVdp2Regs->PNCN3, varVdp2Regs->CHCTLB & 0x10);

  ReadMosaicData(&info, 0x8, varVdp2Regs);

  info.specialcolormode = (varVdp2Regs->SFCCMD >> 6) & 0x03;
  if (varVdp2Regs->SFSEL & 0x8)
    info.specialcode = varVdp2Regs->SFCODE >> 8;
  else
    info.specialcode = varVdp2Regs->SFCODE & 0xFF;

  info.alpha = (~varVdp2Regs->CCRNB & 0x1F00) >> 5;

  Vdp2GeneratePerLineColorCalcuration(&info, NBG3, varVdp2Regs);
  _Ygl->perLine[NBG3] = info.lineTexture;

  info.coloroffset = (varVdp2Regs->CRAOFA & 0x7000) >> 4;
  ReadVdp2ColorOffset(varVdp2Regs, &info, 0x8);
  if (info.lineTexture != 0) {
    info.cor = 0;
    info.cog = 0;
    info.cob = 0;
  }

  info.linecheck_mask = 0x08;
  info.coordincx = info.coordincy = 1;

  info.priority = (varVdp2Regs->PRINB >> 8) & 0x7;
  info.PlaneAddr = (void FASTCALL(*)(void *, int, Vdp2*))&Vdp2NBG3PlaneAddr;

  if (((Vdp2External.disptoggle & 0x8)==0) || (info.priority == 0) ||
    (varVdp2Regs->BGON & 0x1 && (varVdp2Regs->CHCTLA & 0x70) >> 4 == 4) || // If NBG0 16M mode is enabled, don't draw
    (varVdp2Regs->BGON & 0x2 && (varVdp2Regs->CHCTLA & 0x3000) >> 12 >= 2)) // If NBG1 2048/32786 is enabled, don't draw
    return;

  info.islinescroll = 0;
  info.linescrolltbl = 0;
  info.lineinc = 0;
  info.isverticalscroll = 0;
  info.x = varVdp2Regs->SCXN3 & 0x7FF;
  info.y = varVdp2Regs->SCYN3 & 0x7FF;
  Vdp2DrawMapTest(&info, &texture, varVdp2Regs);
  executeDrawCell();
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawRBG0_part( RBGDrawInfo *rgb, Vdp2* varVdp2Regs)
{
  vdp2draw_struct* info = &rgb->info;

  info->dst = 0;
  info->idScreen = RBG0;
  info->idReg = 4;
  info->uclipmode = 0;
  info->cor = 0;
  info->cog = 0;
  info->cob = 0;
  info->specialcolorfunction = 0;
  info->enable = 0;
  info->RotWin = NULL;
  info->RotWinMode = 0;

  info->enable = ((varVdp2Regs->BGON & 0x10)!=0);
  if (!info->enable) {
    free(rgb);
    return;
  }

  for (int i=info->startLine; i<info->endLine; i++) info->display[i] = info->enable;

  info->priority = varVdp2Regs->PRIR & 0x7;
  if (((Vdp2External.disptoggle & 0x10)==0) || (info->priority == 0)) {
    free(rgb);
    return;
  }

  info->blendmode = 0;

  Vdp2GeneratePerLineColorCalcuration(info, RBG0, varVdp2Regs);
  _Ygl->perLine[RBG0] = info->lineTexture;
  //info->lineTexture = 0;

  info->transparencyenable = !(varVdp2Regs->BGON & 0x1000);

  info->specialprimode = (varVdp2Regs->SFPRMD >> 8) & 0x3;

  info->colornumber = (varVdp2Regs->CHCTLB & 0x7000) >> 12;

  info->islinescroll = 0;
  info->linescrolltbl = 0;
  info->lineinc = 0;

  Vdp2ReadRotationTable(0, &rgb->paraA, varVdp2Regs, Vdp2Ram);
  Vdp2ReadRotationTable(1, &rgb->paraB, varVdp2Regs, Vdp2Ram);
  A0_Updated = 0;
  A1_Updated = 0;
  B0_Updated = 0;
  B1_Updated = 0;

  //rgb->paraA.PlaneAddr = (void FASTCALL(*)(void *, int, Vdp2*))&Vdp2ParameterAPlaneAddr;
  //rgb->paraB.PlaneAddr = (void FASTCALL(*)(void *, int, Vdp2*))&Vdp2ParameterBPlaneAddr;
  rgb->paraA.charaddr = (varVdp2Regs->MPOFR & 0x7) * 0x20000;
  rgb->paraB.charaddr = (varVdp2Regs->MPOFR & 0x70) * 0x2000;
  ReadPlaneSizeR(&rgb->paraA, varVdp2Regs->PLSZ >> 8);
  ReadPlaneSizeR(&rgb->paraB, varVdp2Regs->PLSZ >> 12);

  if (rgb->paraA.coefdatasize == 2)
  {
    if (rgb->paraA.coefmode < 3)
    {
      info->GetKValueA = vdp2rGetKValue1W;
    }
    else {
      info->GetKValueA = vdp2rGetKValue1Wm3;
    }

  }
  else {
    if (rgb->paraA.coefmode < 3)
    {
      info->GetKValueA = vdp2rGetKValue2W;
    }
    else {
      info->GetKValueA = vdp2rGetKValue2Wm3;
    }
  }

  if (rgb->paraB.coefdatasize == 2)
  {
    if (rgb->paraB.coefmode < 3)
    {
      info->GetKValueB = vdp2rGetKValue1W;
    }
    else {
      info->GetKValueB = vdp2rGetKValue1Wm3;
    }

  }
  else {
    if (rgb->paraB.coefmode < 3)
    {
      info->GetKValueB = vdp2rGetKValue2W;
    }
    else {
      info->GetKValueB = vdp2rGetKValue2Wm3;
    }
  }
  if (varVdp2Regs->RPMD == 0x00)
  {
    //printf("RPMD 0x0\n");
    if (!(rgb->paraA.coefenab))
    {
      info->GetRParam = (Vdp2GetRParam_func)vdp2RGetParamMode00NoK;
    }
    else {
      info->GetRParam = (Vdp2GetRParam_func)vdp2RGetParamMode00WithK;
    }
  }
  else if (varVdp2Regs->RPMD == 0x01)
  {
    //printf("RPMD 0x1\n");
    if (!(rgb->paraB.coefenab))
    {
      info->GetRParam = (Vdp2GetRParam_func)vdp2RGetParamMode01NoK;
    }
    else {
      info->GetRParam = (Vdp2GetRParam_func)vdp2RGetParamMode01WithK;
    }
  }
  else if (varVdp2Regs->RPMD == 0x02)
  {
    //printf("RPMD 0x2\n");
    if (!(rgb->paraA.coefenab))
    {
      info->GetRParam = (Vdp2GetRParam_func)vdp2RGetParamMode02NoK;
    }
    else {
      if (rgb->paraB.coefenab)
        info->GetRParam = (Vdp2GetRParam_func)vdp2RGetParamMode02WithKAWithKB;
      else
        info->GetRParam = (Vdp2GetRParam_func)vdp2RGetParamMode02WithKA;
    }
  }
  else if (varVdp2Regs->RPMD == 0x03)
  {
    //printf("RPMD 0x3\n");
    // Enable Window0(RPW0E)?
    if (((varVdp2Regs->WCTLD >> 1) & 0x01) == 0x01)
    {
      info->RotWin = _Ygl->win[0];
      // RPW0A( inside = 0, outside = 1 )
      info->RotWinMode = (varVdp2Regs->WCTLD & 0x01);
      // Enable Window1(RPW1E)?
    }
    else if (((varVdp2Regs->WCTLD >> 3) & 0x01) == 0x01)
    {
      info->RotWin = _Ygl->win[1];
      // RPW1A( inside = 0, outside = 1 )
      info->RotWinMode = ((varVdp2Regs->WCTLD >> 2) & 0x01);
      // Bad Setting Both Window is disabled
    }

    if (rgb->paraA.coefenab == 0 && rgb->paraB.coefenab == 0)
    {
      info->GetRParam = (Vdp2GetRParam_func)vdp2RGetParamMode03NoK;
    }
    else if (rgb->paraA.coefenab && rgb->paraB.coefenab == 0)
    {
      info->GetRParam = (Vdp2GetRParam_func)vdp2RGetParamMode03WithKA;
    }
    else if (rgb->paraA.coefenab == 0 && rgb->paraB.coefenab)
    {
      info->GetRParam = (Vdp2GetRParam_func)vdp2RGetParamMode03WithKB;
    }
    else if (rgb->paraA.coefenab && rgb->paraB.coefenab)
    {
      info->GetRParam = (Vdp2GetRParam_func)vdp2RGetParamMode03WithK;
    }
  }

  rgb->paraA.screenover = (varVdp2Regs->PLSZ >> 10) & 0x03;
  rgb->paraB.screenover = (varVdp2Regs->PLSZ >> 14) & 0x03;

  // Figure out which Rotation Parameter we're uqrt
  switch (varVdp2Regs->RPMD & 0x3)
  {
  case 0:
    // Parameter A
    info->rotatenum = 0;
    info->rotatemode = 0;
    info->PlaneAddr = (void FASTCALL(*)(void *, int, Vdp2*))&Vdp2ParameterAPlaneAddr;
    break;
  case 1:
    // Parameter B
    info->rotatenum = 1;
    info->rotatemode = 0;
    info->PlaneAddr = (void FASTCALL(*)(void *, int, Vdp2*))&Vdp2ParameterBPlaneAddr;
    break;
  case 2:
    // Parameter A+B switched via coefficients
    // FIX ME(need to figure out which Parameter is being used)
  case 3:
  default:
    // Parameter A+B switched via rotation parameter window
    // FIX ME(need to figure out which Parameter is being used)
    info->rotatenum = 0;
    info->rotatemode = 1 + (varVdp2Regs->RPMD & 0x1);
    info->PlaneAddr = (void FASTCALL(*)(void *, int, Vdp2*))&Vdp2ParameterAPlaneAddr;
    break;
  }



  if ((info->isbitmap = varVdp2Regs->CHCTLB & 0x200) != 0)
  {
    // Bitmap Mode
    ReadBitmapSize(info, varVdp2Regs->CHCTLB >> 10, 0x1);

    if (info->rotatenum == 0)
      // Parameter A
      info->charaddr = (varVdp2Regs->MPOFR & 0x7) * 0x20000;
    else
      // Parameter B
      info->charaddr = (varVdp2Regs->MPOFR & 0x70) * 0x2000;

    info->paladdr = (varVdp2Regs->BMPNB & 0x7) << 4;
    info->flipfunction = 0;
    info->specialfunction = 0;
  }
  else
  {
    int i;
    // Tile Mode
    info->mapwh = 4;

    if (info->rotatenum == 0)
      // Parameter A
      ReadPlaneSize(info, varVdp2Regs->PLSZ >> 8);
    else
      // Parameter B
      ReadPlaneSize(info, varVdp2Regs->PLSZ >> 12);

    ReadPatternData(info, varVdp2Regs->PNCR, varVdp2Regs->CHCTLB & 0x100);

    rgb->paraA.ShiftPaneX = 8 + rgb->paraA.planew;
    rgb->paraA.ShiftPaneY = 8 + rgb->paraA.planeh;
    rgb->paraB.ShiftPaneX = 8 + rgb->paraB.planew;
    rgb->paraB.ShiftPaneY = 8 + rgb->paraB.planeh;

    rgb->paraA.MskH = (8 * 64 * rgb->paraA.planew) - 1;
    rgb->paraA.MskV = (8 * 64 * rgb->paraA.planeh) - 1;
    rgb->paraB.MskH = (8 * 64 * rgb->paraB.planew) - 1;
    rgb->paraB.MskV = (8 * 64 * rgb->paraB.planeh) - 1;

    rgb->paraA.MaxH = 8 * 64 * rgb->paraA.planew * 4;
    rgb->paraA.MaxV = 8 * 64 * rgb->paraA.planeh * 4;
    rgb->paraB.MaxH = 8 * 64 * rgb->paraB.planew * 4;
    rgb->paraB.MaxV = 8 * 64 * rgb->paraB.planeh * 4;

    if (rgb->paraA.screenover == OVERMODE_512)
    {
      rgb->paraA.MaxH = 512;
      rgb->paraA.MaxV = 512;
    }

    if (rgb->paraB.screenover == OVERMODE_512)
    {
      rgb->paraB.MaxH = 512;
      rgb->paraB.MaxV = 512;
    }

    for (i = 0; i < 16; i++)
    {
	  Vdp2ParameterAPlaneAddr(info, i, varVdp2Regs);
      rgb->paraA.PlaneAddrv[i] = info->addr;
	  Vdp2ParameterBPlaneAddr(info, i, varVdp2Regs);
      rgb->paraB.PlaneAddrv[i] = info->addr;
    }
  }

  ReadMosaicData(info, 0x10, varVdp2Regs);

  info->specialcolormode = (varVdp2Regs->SFCCMD >> 8) & 0x03;
  if (varVdp2Regs->SFSEL & 0x10)
    info->specialcode = varVdp2Regs->SFCODE >> 8;
  else
    info->specialcode = varVdp2Regs->SFCODE & 0xFF;

  info->blendmode = VDP2_CC_NONE;
  if ((varVdp2Regs->LNCLEN & 0x10) == 0x00)
  {
    info->LineColorBase = 0x00;
    rgb->paraA.lineaddr = 0xFFFFFFFF;
    rgb->paraB.lineaddr = 0xFFFFFFFF;
  }
  else {
    //      info->alpha = 0xFF;
    info->LineColorBase = ((varVdp2Regs->LCTA.all) & 0x7FFFF) << 1;
    if (info->LineColorBase >= 0x80000) info->LineColorBase = 0x00;
    rgb->paraA.lineaddr = 0xFFFFFFFF;
    rgb->paraB.lineaddr = 0xFFFFFFFF;
  }

  info->blendmode = 0;

  info->alpha = (~varVdp2Regs->CCRR & 0x1F) << 3;

  info->coloroffset = (varVdp2Regs->CRAOFB & 0x7) << 8;

  ReadVdp2ColorOffset(varVdp2Regs, info, 0x10);
  info->linecheck_mask = 0x10;
  info->coordincx = info->coordincy = 1;

  Vdp2DrawRotation(rgb, varVdp2Regs);
  free(rgb);
}


static void Vdp2DrawRBG0(Vdp2* varVdp2Regs)
{
  int nbZone = 1;
  int lastLine = 0;
  int line;
  int max = (yabsys.VBlankLineCount >= 270)?270:yabsys.VBlankLineCount;
  RBGDrawInfo *rgb;
  for (line = 2; line<max; line++) {
    if (!sameVDP2Reg(RBG0, &Vdp2Lines[line-1], &Vdp2Lines[line])) {
      rgb = (RBGDrawInfo *)calloc(1, sizeof(RBGDrawInfo));
      rgb->rgb_type = 0x0;
      rgb->use_cs = _Ygl->rbg_use_compute_shader;
      rgb->info.startLine = lastLine;
      rgb->info.endLine = line;
      lastLine = line;
      LOG_AREA("RBG0 Draw from %d to %d %x\n", rgb->info.startLine, rgb->info.endLine, varVdp2Regs->BGON);
      Vdp2DrawRBG0_part(rgb, &Vdp2Lines[rgb->info.startLine]);
    }
  }
  rgb = (RBGDrawInfo *)calloc(1, sizeof(RBGDrawInfo));
  rgb->rgb_type = 0x0;
  rgb->use_cs = _Ygl->rbg_use_compute_shader;
  rgb->info.startLine = lastLine;
  rgb->info.endLine = line;
  LOG_AREA("RBG0 Draw from %d to %d %x\n", rgb->info.startLine, rgb->info.endLine, varVdp2Regs->BGON);
  Vdp2DrawRBG0_part(rgb, &Vdp2Lines[rgb->info.startLine]);

}

//////////////////////////////////////////////////////////////////////////////
#define BG_PROFILE 0

#define VDP2_DRAW_LINE 0
static void VIDOGLVdp2DrawScreens(void)
{
  u64 before;
  u64 now;
  u32 difftime;
  char str[64];

#if BG_PROFILE
  before = YabauseGetTicks() * 1000000 / yabsys.tickfreq;
#endif

  _Ygl->perLine[NBG0] = 0;
  _Ygl->perLine[NBG1] = 0;
  _Ygl->perLine[NBG2] = 0;
  _Ygl->perLine[NBG3] = 0;
  _Ygl->perLine[RBG0] = 0;
  _Ygl->perLine[RBG1] = 0;

  YglUpdateColorRam();

LOG_ASYN("===================================\n");
  if (Vdp1Regs->TVMR & 0x02) {
    Vdp2ReadRotationTable(0, &Vdp1ParaA, &Vdp2Lines[VDP2_DRAW_LINE], Vdp2Ram);
  }
  Vdp2DrawBackScreen(&Vdp2Lines[VDP2_DRAW_LINE]);
  Vdp2DrawLineColorScreen(&Vdp2Lines[VDP2_DRAW_LINE]);

  Vdp2DrawRBG0(&Vdp2Lines[VDP2_DRAW_LINE]);
  FrameProfileAdd("RBG0 end");
  Vdp2DrawNBG3(&Vdp2Lines[VDP2_DRAW_LINE]);
  FrameProfileAdd("NBG0 end");
  Vdp2DrawNBG2(&Vdp2Lines[VDP2_DRAW_LINE]);
  FrameProfileAdd("NBG1 end");
  Vdp2DrawNBG1(&Vdp2Lines[VDP2_DRAW_LINE]);
  FrameProfileAdd("NBG2 end");
  Vdp2DrawNBG0(&Vdp2Lines[VDP2_DRAW_LINE]);
  FrameProfileAdd("NBG3 end");
  Vdp2DrawRBG1(&Vdp2Lines[VDP2_DRAW_LINE]);
  FrameProfileAdd("RBG1 end");

LOG_ASYN("*********************************\n");
  vdp2busy = 1;



#if BG_PROFILE
  now = YabauseGetTicks() * 1000000 / yabsys.tickfreq;
  if (now > before) {
    difftime = now - before;
  }
  else {
    difftime = now + (ULLONG_MAX - before);
  }
  sprintf(str, "VIDOGLVdp2DrawScreens = %d", difftime);
  DisplayMessage(str);
#endif
}

int WaitVdp2Async(int sync) {
  int empty = 0;
  if ((vdp2busy == 1)) {
#ifdef RGB_ASYNC
    if (rotq_end != NULL) {
      empty = 1;
      while (((empty = YaGetQueueSize(rotq_end))!=0) && (sync == 1))
      {
        YabThreadYield();
      }
      finishRbgQueue();
      if (empty != 0) return empty;
    }
#endif
#ifdef CELL_ASYNC
    if (cellq_end != NULL) {
      empty = 1;
      while (((empty = YaGetQueueSize(cellq_end))!=0) && (sync == 1))
      {
        YabThreadYield();
      }
    }
#endif
    RBGGenerator_onFinish();
    if (empty == 0) vdp2busy = 0;
  }
  return empty;
}

void waitVdp2DrawScreensEnd(int sync, int abort) {
  if (abort == 0){
    YglCheckFBSwitch(0);
    if ((vdp2busy == 1)) {
      int empty = WaitVdp2Async(sync);
      if (empty == 0) {
        YglTmPush(YglTM_vdp2);
        if (VIDCore != NULL) VIDCore->composeFB(&Vdp2Lines[0]);
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp2SetResolution(u16 TVMD)
{
  int width = 1, height = 1;
  int wratio = 1, hratio = 1;

  // Horizontal Resolution
  switch (TVMD & 0x7)
  {
  case 0:
    width = 320;
    wratio = 1;
    break;
  case 1:
    width = 352;
    wratio = 1;
    break;
  case 2:
    width = 640;
    wratio = 2;
    break;
  case 3:
    width = 704;
    wratio = 2;
    break;
  case 4:
    width = 320;
    wratio = 1;
    break;
  case 5:
    width = 352;
    wratio = 1;
    break;
  case 6:
    width = 640;
    wratio = 2;
    break;
  case 7:
    width = 704;
    wratio = 2;
    break;
  }

  // Vertical Resolution
  switch ((TVMD >> 4) & 0x3)
  {
  case 0:
    height = 224;
    break;
  case 1: height = 240;
    break;
  case 2: height = 256;
    break;
  default: height = 256;
    break;
  }

  hratio = 1;

  // Check for interlace
  switch ((TVMD >> 6) & 0x3)
  {
  case 3: // Double-density Interlace
    height *= 2;
    hratio = 2;
    vdp2_interlace = 1;
    break;
  case 2: // Single-density Interlace
  case 0: // Non-interlace
  default:
    vdp2_interlace = 0;
    break;
  }

  Vdp1SetTextureRatio(wratio, hratio);
  if ((width != _Ygl->rwidth) || (height != _Ygl->rheight))SetSaturnResolution(width, height);
}

//////////////////////////////////////////////////////////////////////////////

void YglGetGlSize(int *width, int *height)
{
  *width = GlWidth;
  *height = GlHeight;
}

void VIDOGLGetNativeResolution(int *width, int *height, int*interlace)
{
  *width = _Ygl->rwidth;
  *height = _Ygl->rheight;
  *interlace = vdp2_interlace;
}

void VIDOGLVdp2DispOff()
{
}

vdp2rotationparameter_struct * FASTCALL vdp2rGetKValue2W(vdp2rotationparameter_struct * param, int index)
{
  float kval;
  int   kdata;

  //if (index < 0) index = 0;
  //index &= 0x7FFFF;
  //if (index >= param->ktablesize) index = param->ktablesize - 1;
  //kdata = param->prefecth_k2w[index];

  if (param->k_mem_type == 0) { // vram
    kdata = Vdp2RamReadLong(NULL, Vdp2Ram, (param->coeftbladdr + (index << 2)));
  }
  else { // cram
    kdata = T2ReadLong((Vdp2ColorRam + 0x800), (param->coeftbladdr + (index << 2)) & 0xFFF);
  }
  param->lineaddr = (kdata >> 24) & 0x7F;

  if (kdata & 0x80000000) return NULL;

  kval = (float)(int)((kdata & 0x00FFFFFF) | (kdata & 0x00800000 ? 0xFF800000 : 0x00000000)) / 65536.0f;

  switch (param->coefmode)
  {
  case 0:
    param->kx = kval;
    param->ky = kval;
    break;
  case 1:
    param->kx = kval;
    break;
  case 2:
    param->ky = kval;
    break;
  }
  return param;
}

vdp2rotationparameter_struct * FASTCALL vdp2rGetKValue1W(vdp2rotationparameter_struct * param, int index)
{
  float kval;
  u16   kdata;

  if (param->k_mem_type == 0) { // vram
    kdata = Vdp2RamReadWord(NULL, Vdp2Ram, (param->coeftbladdr + (index << 1)));
  }
  else { // cram
    kdata = T2ReadWord((Vdp2ColorRam + 0x800), (param->coeftbladdr + (index << 1)) & 0xFFF);
  }

  if (kdata & 0x8000) return NULL;

  kval = (float)(signed)((kdata & 0x7FFF) | (kdata & 0x4000 ? 0x8000 : 0x0000)) / 1024.0f;

  switch (param->coefmode)
  {
  case 0:
    param->kx = kval;
    param->ky = kval;
    break;
  case 1:
    param->kx = kval;
    break;
  case 2:
    param->ky = kval;
    break;
  }
  return param;

}

vdp2rotationparameter_struct * FASTCALL vdp2rGetKValue2Wm3(vdp2rotationparameter_struct * param, int index)
{
  return param; // ToDo:
}

vdp2rotationparameter_struct * FASTCALL vdp2rGetKValue1Wm3(vdp2rotationparameter_struct * param, int index)
{
  return param; // ToDo:
}


vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode00NoK(RBGDrawInfo * rgb, int h, int v, Vdp2* varVdp2Regs)
{
  return &rgb->paraA;
}

vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode00WithK(RBGDrawInfo * rgb, int h, int v, Vdp2* varVdp2Regs)
{
  h = ceilf(rgb->paraA.KtablV + (rgb->paraA.deltaKAx * h));
  return rgb->info.GetKValueA(&rgb->paraA, h);
}

vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode01NoK(RBGDrawInfo * rgb, int h, int v, Vdp2* varVdp2Regs)
{
  return &rgb->paraB;
}

vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode01WithK(RBGDrawInfo * rgb, int h, int v, Vdp2* varVdp2Regs)
{
  h = ceilf(rgb->paraB.KtablV + (rgb->paraB.deltaKAx * h));
  return rgb->info.GetKValueB(&rgb->paraB, h);
}

vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode02NoK(RBGDrawInfo * rgb, int h, int v, Vdp2* varVdp2Regs)
{
  return &rgb->paraA;
}

vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode02WithKA(RBGDrawInfo * rgb, int h, int v, Vdp2* varVdp2Regs)
{
  if (rgb->info.GetKValueA(&rgb->paraA, ceilf( rgb->paraA.KtablV + (rgb->paraA.deltaKAx * h))) == NULL)
  {
    rgb->paraB.lineaddr = rgb->paraA.lineaddr;
    return &rgb->paraB;
  }
  return &rgb->paraA;
}

vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode02WithKAWithKB(RBGDrawInfo * rgb, int h, int v, Vdp2* varVdp2Regs)
{
  if (rgb->info.GetKValueA(&rgb->paraA, ceilf(rgb->paraA.KtablV + (rgb->paraA.deltaKAx * h))) == NULL)
  {
    rgb->info.GetKValueB(&rgb->paraB, ceilf(rgb->paraB.KtablV + (rgb->paraB.deltaKAx * h)));
    return &rgb->paraB;
  }
  return &rgb->paraA;
}

vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode02WithKB(RBGDrawInfo * rgb, int h, int v, Vdp2* varVdp2Regs)
{
  printf("vdp2RGetParamMode02WithKB used!\n");
  return &rgb->paraA;
}

vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode03NoK(RBGDrawInfo * rgb, int h, int v, Vdp2* varVdp2Regs)
{
  if (rgb->info.RotWin == NULL) {
    return (&rgb->paraA);
  }

  short start = rgb->info.RotWin[v] & 0xFFFF;
  short end = (rgb->info.RotWin[v] >> 16) & 0xFFFF;

  if (rgb->info.RotWinMode == 1)
  {
    if (start == end)
    {
      return (&rgb->paraA);
    }
    else {
      if (h < start || h >= end)
      {
        return (&rgb->paraB);
      }
      else {
        return (&rgb->paraA);
      }
    }
  }
  else
  {
    if (start == end)
    {
      return (&rgb->paraA);
    }
    else {
      if (h < start || h >= end)
      {
        return (&rgb->paraA);
      }
      else {
        return (&rgb->paraB);
      }
    }
  }
  return NULL;
}

vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode03WithKA(RBGDrawInfo * rgb, int h, int v, Vdp2* varVdp2Regs)
{
  // Virtua Fighter2
  if (rgb->info.RotWin == NULL) {
    if (rgb->info.RotWinMode == 0)
    {
      //printf("vdp2RGetParamMode03WithKA NULL!\n");
      h = ceilf(rgb->paraA.KtablV + (rgb->paraA.deltaKAx * h));
      return rgb->info.GetKValueA(&rgb->paraA, h);
    } else {
      h = ceilf(rgb->paraA.KtablV + (rgb->paraA.deltaKAx * h));
      return rgb->info.GetKValueA(&rgb->paraA, h);
    }
  }
  short start = rgb->info.RotWin[v] & 0xFFFF;
  short end = (rgb->info.RotWin[v] >> 16) & 0xFFFF;
  if (rgb->info.RotWinMode == 0)
  {
    if (start == end)
    {
      h = ceilf(rgb->paraA.KtablV + (rgb->paraA.deltaKAx * h));
      return rgb->info.GetKValueA(&rgb->paraA, h);
    }
    else {
      if (h < start || h >= end)
      {
        return (&rgb->paraB);
      }
      else {
        h = ceilf(rgb->paraA.KtablV + (rgb->paraA.deltaKAx * h));
        return rgb->info.GetKValueA(&rgb->paraA, h);
      }
    }
  }
  else {
    if (start == end)
    {
      h = ceilf(rgb->paraA.KtablV + (rgb->paraA.deltaKAx * h));
      return rgb->info.GetKValueA(&rgb->paraA, h);
    }
    else {
      if (h < start|| h >= end)
      {
        h = ceilf(rgb->paraA.KtablV + (rgb->paraA.deltaKAx * h));
        return rgb->info.GetKValueA(&rgb->paraA, h);
      }
      else {
        return (&rgb->paraB);
      }
    }
  }
  return NULL;
}

vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode03WithKB(RBGDrawInfo * rgb, int h, int v, Vdp2* varVdp2Regs)
{
  if (rgb->info.RotWin == NULL) {
    //printf("vdp2RGetParamMode03WithKB NULL!\n");
    if (rgb->info.RotWinMode == 0)
     return (&rgb->paraA);
   else {
     h = ceilf(rgb->paraB.KtablV + (rgb->paraB.deltaKAx * h));
     return rgb->info.GetKValueB(&rgb->paraB, h);
   }
  }
  short start = rgb->info.RotWin[v] & 0xFFFF;
  short end = (rgb->info.RotWin[v] >> 16) & 0xFFFF;
  if (rgb->info.RotWinMode == 0)
  {
    if (start == end)
    {
      return &rgb->paraA;
    }
    else {
      if (h < start || h >= end)
      {
        h = ceilf(rgb->paraB.KtablV + (rgb->paraB.deltaKAx * h));
        return rgb->info.GetKValueB(&rgb->paraB, h);
      }
      else {
        return &rgb->paraA;
      }
    }
  }
  else {
    {
      if (start == end)
      {
        h = ceilf(rgb->paraB.KtablV + (rgb->paraB.deltaKAx * h));
        return rgb->info.GetKValueB(&rgb->paraB, h);
      }
      else {
        if (h < start || h >= end)
        {
          return &rgb->paraA;
        }
        else {
          h = ceilf(rgb->paraB.KtablV + (rgb->paraB.deltaKAx * h));
          return rgb->info.GetKValueB(&rgb->paraB, h);
        }
      }
    }
  }
  return NULL;
}

vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode03WithK(RBGDrawInfo * rgb, int h, int v, Vdp2* varVdp2Regs)
{
  vdp2rotationparameter_struct * p;
  if (rgb->info.RotWin == NULL) {
    //printf("vdp2RGetParamMode03WithK NULL\n");
    if (rgb->info.RotWinMode == WA_INSIDE) {
      h = ceilf(rgb->paraA.KtablV + (rgb->paraA.deltaKAx * h));
      p = rgb->info.GetKValueA(&rgb->paraA, h);
      if (p) return p;
      h = ceilf(rgb->paraB.KtablV + (rgb->paraB.deltaKAx * h));
      return rgb->info.GetKValueB(&rgb->paraB, h);
    } else {
      h = ceilf(rgb->paraB.KtablV + (rgb->paraB.deltaKAx * h));
      p = rgb->info.GetKValueB(&rgb->paraB, h);
      if (p) return p;
      h = ceilf(rgb->paraA.KtablV + (rgb->paraA.deltaKAx * h));
      return rgb->info.GetKValueA(&rgb->paraA, h);
    }
  }
  short start = rgb->info.RotWin[v] & 0xFFFF;
  short end = (rgb->info.RotWin[v] >> 16) & 0xFFFF;

  // Final Fight Revenge
  if (rgb->info.RotWinMode == WA_INSIDE) {
    if (start == end) {
      h = ceilf(rgb->paraA.KtablV + (rgb->paraA.deltaKAx * h));
      p = rgb->info.GetKValueA(&rgb->paraA, h);
      if (p) return p;
      h = ceilf(rgb->paraB.KtablV + (rgb->paraB.deltaKAx * h));
      return rgb->info.GetKValueB(&rgb->paraB, h);
    }
    else {
      if (h < start || h >= end) {
        h = (rgb->paraA.KtablV + (rgb->paraA.deltaKAx * h));
        p = rgb->info.GetKValueA(&rgb->paraA, h);
        if (p) return p;
        h = ceilf(rgb->paraB.KtablV + (rgb->paraB.deltaKAx * h));
        return rgb->info.GetKValueB(&rgb->paraB, h);
      }
      else {
        h = (rgb->paraB.KtablV + (rgb->paraB.deltaKAx * h));
        p = rgb->info.GetKValueB(&rgb->paraB, h);
        if (p) return p;
        h = ceilf(rgb->paraA.KtablV + (rgb->paraA.deltaKAx * h));
        return rgb->info.GetKValueA(&rgb->paraA, h);
      }
    }
  }
  else {
    if (start == end) {
      h = ceilf(rgb->paraB.KtablV + (rgb->paraB.deltaKAx * h));
      p = rgb->info.GetKValueB(&rgb->paraB, h);
      if (p) return p;
      h = ceilf(rgb->paraA.KtablV + (rgb->paraA.deltaKAx * h));
      return rgb->info.GetKValueA(&rgb->paraA, h);
    }
    else {
      if (h < start || h >= end) {
        h = ceilf(rgb->paraB.KtablV + (rgb->paraB.deltaKAx * h));
        p = rgb->info.GetKValueB(&rgb->paraB, h);
        if (p) return p;
        h = ceilf(rgb->paraA.KtablV + (rgb->paraA.deltaKAx * h));
        return rgb->info.GetKValueA(&rgb->paraA, h);
      }
      else {
        h = ceilf(rgb->paraA.KtablV + (rgb->paraA.deltaKAx * h));
        p = rgb->info.GetKValueA(&rgb->paraA, h);
        if (p) return p;
        h = ceilf(rgb->paraB.KtablV + (rgb->paraB.deltaKAx * h));
        return rgb->info.GetKValueB(&rgb->paraB, h);
      }
    }
  }
  return NULL;
}

void VIDOGLSetSettingValueMode(int type, int value) {

  switch (type) {
  case VDP_SETTING_FILTERMODE:
    _Ygl->aamode = value;
    break;
  case VDP_SETTING_UPSCALMODE:
    _Ygl->upmode = value;
    break;
  case VDP_SETTING_RESOLUTION_MODE:
    if (_Ygl->resolution_mode != value) {
       _Ygl->resolution_mode = value;
       SetSaturnResolution(_Ygl->rwidth, _Ygl->rheight);
    }
    break;
  case VDP_SETTING_POLYGON_MODE:
    if (value == GPU_TESSERATION && _Ygl->polygonmode != GPU_TESSERATION) {
      int maj, min;
      glGetIntegerv(GL_MAJOR_VERSION, &maj);
      glGetIntegerv(GL_MINOR_VERSION, &min);
#if defined(_OGLES3_)
      if ((maj >=3) && (min >=1)) {
#else
      if ((maj >=4) && (min >=2)) {
#endif
        if (glPatchParameteri) {
          _Ygl->polygonmode = value;
        } else {
          YuiMsg("GPU tesselation is not possible - fallback on CPU tesselation\n");
          _Ygl->polygonmode = CPU_TESSERATION;
        }
      } else {
        YuiMsg("GPU tesselation is not possible - fallback on CPU tesselation\n");
        _Ygl->polygonmode = CPU_TESSERATION;
      }
    } else {


      _Ygl->polygonmode = value;
    }
  break;
  case VDP_SETTING_COMPUTE_SHADER:
    if (value == COMPUTE_RBG_ON && _Ygl->rbg_use_compute_shader != COMPUTE_RBG_ON) {
      int maj, min;
      glGetIntegerv(GL_MAJOR_VERSION, &maj);
      glGetIntegerv(GL_MINOR_VERSION, &min);
#if defined(_OGLES3_)
      if ((maj >=3) && (min >=1)) {
#else
      if ((maj >=4) && (min >=3)) {
#endif
          _Ygl->rbg_use_compute_shader = value;
      } else {
        YuiMsg("Compute shader usage is not possible - disabling\n");
        _Ygl->rbg_use_compute_shader = COMPUTE_RBG_OFF;
      }
    } else {
      _Ygl->rbg_use_compute_shader = value;
    }
    SetSaturnResolution(_Ygl->rwidth, _Ygl->rheight);
  break;
  case VDP_SETTING_ASPECT_RATIO:
    _Ygl->stretch = value;
  break;
  case VDP_SETTING_SCANLINE:
    _Ygl->scanline = value;
  break;
  case VDP_SETTING_MESH_MODE:
    _Ygl->meshmode = value;
  break;
  default:
  return;
  }
}

#endif
