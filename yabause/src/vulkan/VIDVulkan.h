/*
        Copyright 2021 devMiyax(smiyaxdev@gmail.com)

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

#pragma once

extern "C" {
#include "vidshared.h"
#include "debug.h"
#include "vdp2.h"
#include "yabause.h"
#include "ygl.h"
#include "yui.h"
#include "frameprofile.h"
}


#include "VulkanScene.h"
#include "VdpPipeline.h"
#include "TextureManager.h"

class TextureManager;
class VertexManager;
class CharTexture;
class VdpPipelineFactory;
class TextureCache;
class Vdp1Renderer;
class FramebufferRenderer;
class RBGGeneratorVulkan;
class WindowRenderer;

#define ATLAS_BIAS (0.025f)


struct UniformBufferObject {
  glm::mat4 mvp;
  glm::vec4 color_offset;
  int blendmode;
  int offsetx;
  int offsety;
  int windowWidth;
  int windowHeight;
  int winmask;
  int winflag;
  int winmode;
  float emu_height;
  float vheight;
  float viewport_offset;
  float u_tw;
  float u_th;
  int u_mosaic_x;
  int u_mosaic_y;
  int specialPriority;
  int u_specialColorFunc;
};

class DynamicTexture {
public:
  int width = 2;
  int height = 2;
  VkBuffer stagingBuffer = VK_NULL_HANDLE;
  VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
  VkImage image = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  VkImageView imageView = VK_NULL_HANDLE;
  VkSampler sampler = VK_NULL_HANDLE;
  void create(VIDVulkan * vulkan, int width, int height);
  void update(VIDVulkan * vulkan, VkCommandBuffer commandBuffer);
  u32* dynamicBuf = nullptr;
};


class VIDVulkan : public VulkanScene {

public:
  inline static VIDVulkan * getInstance() {
    if (_instance == nullptr) _instance = new VIDVulkan();
    return _instance;
  };
  virtual ~VIDVulkan();


protected:
  static VIDVulkan * _instance;
  VIDVulkan();

public:
  // Interface for yabause
  int init(void);
  void deInit(void);
  void resize(int originx, int originy, unsigned int w, unsigned int h, int on, int aspect_rate_mode);
  int isFullscreen(void);
  int Vdp1Reset(void);
  void Vdp1DrawStart(void);
  void Vdp1DrawEnd(void);
  void Vdp1NormalSpriteDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer);
  void Vdp1ScaledSpriteDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer);
  void Vdp1DistortedSpriteDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer);
  void Vdp1PolygonDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer);
  void Vdp1PolylineDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer);
  void Vdp1LineDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer);
  void Vdp1UserClipping(u8 * ram, Vdp1 * regs);
  void Vdp1SystemClipping(u8 * ram, Vdp1 * regs);
  void Vdp1LocalCoordinate(u8 * ram, Vdp1 * regs);
  int Vdp2Reset(void);
  void Vdp2DrawStart(void);
  void Vdp2DrawEnd(void);
  void Vdp2DrawScreens(void);
  void Vdp2SetResolution(u16 TVMD);
  void GetGlSize(int *width, int *height);
  void Vdp1ReadFrameBuffer(u32 type, u32 addr, void * out);
  void SetFilterMode(int type);
  void Sync();
  void Vdp1WriteFrameBuffer(u32 type, u32 addr, u32 val);
  void Vdp1EraseWrite(void);
  void Vdp1FrameChange(void);
  void SetSettingValue(int type, int value);
  void GetNativeResolution(int *width, int *height, int * interlace);
  void Vdp2DispOff(void);
  void genLineinfo(vdp2draw_struct *info);

  int genPolygon(
    VdpPipeline ** pipleLine,
    vdp2draw_struct * input,
    CharTexture * output,
    float * colors, TextureCache * c, int cash_flg);

  void onUpdateColorRamWord(u32 addr);

  VkImageView getCramImageView() {
    return cram.imageView;
  }

  VkSampler getCramSampler() {
    return cram.sampler;
  }

  TextureManager * getTM() {
    return tm;
  }

  VertexManager * getVM() {
    return vm;
  }


  typedef struct {
    int useb = 0;
    vdp2draw_struct info = {};
    CharTexture texture;
    int rgb_type = 0;
    int pagesize = 0;
    int patternshift = 0;
    u32 LineColorRamAdress = 0;
    vdp2draw_struct line_info = {};
    CharTexture line_texture;
    TextureCache c;
    TextureCache cline;
    int vres = 0;
    int hres = 0;
    int async = 0;
    volatile int vdp2_sync_flg = 0;
    float rotate_mval_h = 0;
    float rotate_mval_v = 0;

  } RBGDrawInfo;

  int vdp2width; // virtual VDP2 resolution X
  int vdp2height; // virtual VDP2 resolution Y
  int originx; // rendering start point x
  int originy; // rendering start point y 
  int renderWidth;  // rendering width(not device width)
  int renderHeight; // rendering height(not device height)
  int finalWidth;  // final rendering width(not device width)
  int finalHeight; // final rendering height(not device height)

protected:

  Vdp1Renderer * vdp1;
  WindowRenderer * windowRenderer;
  uint64_t frameCount;
  POLYGONMODE polygonMode;
  int rebuildPipelines = 0;

  VdpPipelineFactory * pipleLineFactory;

  VdpPipeline * pipleLineNBG0;
  VdpPipeline * pipleLineNBG1;
  VdpPipeline * pipleLineNBG2;
  VdpPipeline * pipleLineNBG3;
  VdpPipeline * pipleLineRBG0;

  int rbg_use_compute_shader = 1;
  RBGGeneratorVulkan * rbgGenerator;

  //std::vector<VdpPipeline*> sortedLayer;
  std::vector< std::vector<VdpPipeline*> > layers;


  FramebufferRenderer * fbRender;

  // VDP Emulation
  Vdp2 _baseVdp2Regs;
  Vdp2 * fixVdp2Regs = NULL;

  float _clear_r;
  float _clear_g;
  float _clear_b;

  void readVdp2ColorOffset(Vdp2 * regs, vdp2draw_struct *info, int mask);
  void setClearColor(float r, float g, float b);

  // Screen
  ASPECT_RATE_MODE aspect_rate_mode = ORIGINAL;
  bool isFullScreen = false;
  bool rotate_screen = false;

  int _vdp2_interlace = 0;
  int _nbg0priority = 0;
  int _nbg1priority = 0;
  int _nbg2priority = 0;
  int _nbg3priority = 0;
  int _rbg0priority = 0;
  int vdp1_interlace = 0;

  TextureManager * tm;
  VertexManager * vm;

  void drawNBG0();
  void drawNBG1();
  void drawNBG2();
  void drawNBG3();
  void drawRBG0();

  void drawMap(vdp2draw_struct *info, CharTexture *texture);
  void drawBitmap(vdp2draw_struct *info, CharTexture *texture);
  void drawMapPerLine(vdp2draw_struct *info, CharTexture *texture);
  void drawMapPerLineNbg23(vdp2draw_struct *info, CharTexture *texture, int id, int xoffset);
  void drawBitmapLineScroll(vdp2draw_struct *info, CharTexture *texture);
  void drawBitmapCoordinateInc(vdp2draw_struct *info, CharTexture *texture);


  void getPalAndCharAddr(vdp2draw_struct *info, int planex, int x, int planey, int y);
  void drawPattern(vdp2draw_struct *info, CharTexture *texture, int x, int y, int cx, int cy);
  void drawCell(vdp2draw_struct *info, CharTexture *texture);
  void genQuadVertex(vdp2draw_struct * input, CharTexture * output, YglCache * c, int cx, int cy, float sx, float sy, int cash_flg);
  u32 getPixel4bpp(vdp2draw_struct *info, u32 addr, CharTexture *texture);
  u32 getPixel8bpp(vdp2draw_struct *info, u32 addr, CharTexture *texture);
  u32 getPixel16bpp(vdp2draw_struct *info, u32 addr);
  u32 getPixel16bppbmp(vdp2draw_struct *info, u32 addr);
  u32 getPixel32bppbmp(vdp2draw_struct *info, u32 addr);
  u32 getAlpha(vdp2draw_struct *info, u8 dot, u32 cramindex);
  u16 getRawColor(u32 colorindex);
  //void setSpecialPriority(vdp2draw_struct *info, u8 dot, u32 * cramindex);

  vdp2Lineinfo lineNBG0[512];
  vdp2Lineinfo lineNBG1[512];

  YabMutex * crammutex;
  DynamicTexture cram;
  DynamicTexture backColor;
  DynamicTexture lineColor;
  DynamicTexture perline[enBGMAX];


  void generatePerLineColorCalcuration(vdp2draw_struct * info, int id);

  void updateColorRam(VkCommandBuffer commandBuffer);
  void updateLineColor(void);

  // Window Parameter
  int bUpdateWindow;

  RBGDrawInfo * curret_rbg = NULL;
  RBGDrawInfo g_rgb0;
  RBGDrawInfo g_rgb1;
  vdp2rotationparameter_struct  paraA = { 0 };
  vdp2rotationparameter_struct  paraB = { 0 };

  void drawRotation(RBGDrawInfo * rbg, VdpPipeline ** pipleLine);
  void drawRotation_in(RBGDrawInfo * rbg);
  void getPatternAddr(vdp2draw_struct *info);
  void getPatternAddrUsingPatternname(vdp2draw_struct *info, u16 paternname);
  int genPolygonRbg0(
    VdpPipeline ** pipleLine,
    vdp2draw_struct * input,
    CharTexture * output,
    TextureCache * c,
    TextureCache * line,
    int rbg_type);

  int resolutionMode = RES_NATIVE;
  int rbgResolutionMode = RBG_RES_ORIGINAL;
  int rebuildFrameBuffer = 0;

  void SetSaturnResolution(int width, int height);


  struct FrameBufferAttachment {
    VkImage image = VK_NULL_HANDLE;;
    VkDeviceMemory mem = VK_NULL_HANDLE;;
    VkImageView view = VK_NULL_HANDLE;;
    VkSemaphore _render_complete_semaphore = VK_NULL_HANDLE;;
  };

  struct SubRenderTarget {
    int32_t width = -1;
    int32_t height = -1;
    VkFramebuffer frameBuffer = VK_NULL_HANDLE;
    FrameBufferAttachment color, depth;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    VkDescriptorImageInfo descriptor;
  } subRenderTarget;

  void generateSubRenderTarget(int width, int height);
  void freeSubRenderTarget();
  void blitSubRenderTarget(VkCommandBuffer commandBuffer);


  struct OffscreenPass {
    int32_t width = -1;
    int32_t height = -1;
    VkFramebuffer frameBuffer = VK_NULL_HANDLE;
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory mem = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    VkDescriptorImageInfo descriptor;
  };

  OffscreenPass offscreenPass;
  OffscreenPass depthPass;

  struct OffscreenRenderer {
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
    VdpPipelineBlit * blit;
    VdpPipelineMosaic * mosaic;
  } offscreenRenderer;


  void generateOffscreenPath(int width, int height);
  void generateOffscreenRenderer();
  void renderToOffecreenTarget(VkCommandBuffer commandBuffer, VdpPipeline * render);
  void renderEffectToMainTarget(VkCommandBuffer commandBuffer, const UniformBufferObject & ubo, int mode);
  void renderWithLineEffectToMainTarget(VdpPipeline * p, VkCommandBuffer commandBuffer, const UniformBufferObject & ubo, VkImageView lineinfo);
  void deleteOfscreenPath();
  int checkCharAccessPenalty(int char_access, int ptn_access);

  VdpBack * backPiepline;
  void updateBackColor();
  void genPolygonBack();

};
