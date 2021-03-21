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

#include "VIDVulkan.h"
#include "VIDVulkanCInterface.h"
#include "Window.h"
#include <iostream>
#include <fstream>

#include "TextureManager.h"
#include "VertexManager.h"

#include "VdpPipelineFactory.h"

#include "Vdp1Renderer.h"
#include "WindowRenderer.h"

#include "FramebufferRenderer.h"

#include "Shared.h"
#include "RBGGeneratorVulkan.h"

#include "VulkanInitializers.hpp"
#include "VulkanTools.h"

#if defined(__ANDROID__)
#define VK_CHECK_RESULT(f)																				\
{																										\
	VkResult res = (f);																					\
	if (res != VK_SUCCESS)																				\
	{																									\
		LOGE("Fatal : VkResult is \" %s \" in %s at line %d", vks::tools::errorString(res).c_str(), __FILE__, __LINE__); \
		assert(res == VK_SUCCESS);																		\
	}																									\
}
#else
#include <iostream>
#define VK_CHECK_RESULT(f)																				\
{																										\
	VkResult res = (f);																					\
	if (res != VK_SUCCESS)																				\
	{																									\
		std::cout << "Fatal : VkResult is \"" << vks::tools::errorString(res) << "\" in " << __FILE__ << " at line " << __LINE__ << "\n"; \
		assert(res == VK_SUCCESS);																		\
	}																									\
}
#endif

int NanovgVulkanSetDevices(VkDevice device, VkPhysicalDevice gpu, VkRenderPass renderPass, VkCommandBuffer cmdBuffer);


VIDVulkan * VIDVulkan::_instance = nullptr;

extern "C" {

#include "vidogl.h" 
#include "vidshared.h"
#include "debug.h"
#include "vdp2.h"
#include "yabause.h"
#include "ygl.h"
#include "yui.h"
#include "frameprofile.h"

  int VIDVulkanInit(void) { return VIDVulkan::getInstance()->init(); }
  void VIDVulkanDeInit(void) { VIDVulkan::getInstance()->deInit(); }
  void VIDVulkanResize(int originx, int originy, unsigned int w, unsigned int h, int on, int aspect_rate_mode) { VIDVulkan::getInstance()->resize(originx, originy, w, h, on, aspect_rate_mode); }
  int VIDVulkanIsFullscreen(void) { return VIDVulkan::getInstance()->isFullscreen(); }
  int VIDVulkanVdp1Reset(void) { return VIDVulkan::getInstance()->Vdp1Reset(); }
  void VIDVulkanVdp1DrawStart(void) { VIDVulkan::getInstance()->Vdp1DrawStart(); }
  void VIDVulkanVdp1DrawEnd(void) { VIDVulkan::getInstance()->Vdp1DrawEnd(); }
  void VIDVulkanVdp1NormalSpriteDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer) { VIDVulkan::getInstance()->Vdp1NormalSpriteDraw(ram, regs, back_framebuffer); }
  void VIDVulkanVdp1ScaledSpriteDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer) { VIDVulkan::getInstance()->Vdp1ScaledSpriteDraw(ram, regs, back_framebuffer); }
  void VIDVulkanVdp1DistortedSpriteDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer) { VIDVulkan::getInstance()->Vdp1DistortedSpriteDraw(ram, regs, back_framebuffer); }
  void VIDVulkanVdp1PolygonDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer) { VIDVulkan::getInstance()->Vdp1PolygonDraw(ram, regs, back_framebuffer); }
  void VIDVulkanVdp1PolylineDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer) { VIDVulkan::getInstance()->Vdp1PolylineDraw(ram, regs, back_framebuffer); }
  void VIDVulkanVdp1LineDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer) { VIDVulkan::getInstance()->Vdp1LineDraw(ram, regs, back_framebuffer); }
  void VIDVulkanVdp1UserClipping(u8 * ram, Vdp1 * regs) { VIDVulkan::getInstance()->Vdp1UserClipping(ram, regs); }
  void VIDVulkanVdp1SystemClipping(u8 * ram, Vdp1 * regs) { VIDVulkan::getInstance()->Vdp1SystemClipping(ram, regs); }
  void VIDVulkanVdp1LocalCoordinate(u8 * ram, Vdp1 * regs) { VIDVulkan::getInstance()->Vdp1LocalCoordinate(ram, regs); }
  int VIDVulkanVdp2Reset(void) { return VIDVulkan::getInstance()->Vdp2Reset(); }
  void VIDVulkanVdp2DrawStart(void) { VIDVulkan::getInstance()->Vdp2DrawStart(); }
  void VIDVulkanVdp2DrawEnd(void) { VIDVulkan::getInstance()->Vdp2DrawEnd(); }
  void VIDVulkanVdp2DrawScreens(void) { VIDVulkan::getInstance()->Vdp2DrawScreens(); }
  void VIDVulkanVdp2SetResolution(u16 TVMD) { VIDVulkan::getInstance()->Vdp2SetResolution(TVMD); }
  void VIDGetGlSize(int *width, int *height) { VIDVulkan::getInstance()->GetGlSize(width, height); }
  void VIDVulkanVdp1ReadFrameBuffer(u32 type, u32 addr, void * out) { VIDVulkan::getInstance()->Vdp1ReadFrameBuffer(type, addr, out); }
  void VIDVulkanSetFilterMode(int type) { VIDVulkan::getInstance()->SetFilterMode(type); }
  void VIDVulkanSync() { VIDVulkan::getInstance()->Sync(); }
  void VIDVulkanVdp1WriteFrameBuffer(u32 type, u32 addr, u32 val) { VIDVulkan::getInstance()->Vdp1WriteFrameBuffer(type, addr, val); }
  void VIDVulkanVdp1EraseWrite(void) { VIDVulkan::getInstance()->Vdp1EraseWrite(); }
  void VIDVulkanVdp1FrameChange(void) { VIDVulkan::getInstance()->Vdp1FrameChange(); }
  void VIDVulkanSetSettingValue(int type, int value) { VIDVulkan::getInstance()->SetSettingValue(type, value); }
  void VIDVulkanGetNativeResolution(int *width, int *height, int * interlace) { VIDVulkan::getInstance()->GetNativeResolution(width, height, interlace); }
  void VIDVulkanVdp2DispOff(void) { VIDVulkan::getInstance()->Vdp2DispOff(); }
  void VIDVulkanOnUpdateColorRamWord(u32 addr) { VIDVulkan::getInstance()->onUpdateColorRamWord(addr); }
  void VIDVulkanGetScreenshot(void ** outbuf, int * width, int * height) { VIDVulkan::getInstance()->getScreenshot(outbuf, *width, *height); }

  VideoInterface_struct CVIDVulkan = {
    VIDCORE_VULKAN,
    "Vulkan Video Interface",
    VIDVulkanInit,
    VIDVulkanDeInit,
    VIDVulkanResize,
    VIDVulkanIsFullscreen,
    VIDVulkanVdp1Reset,
    VIDVulkanVdp1DrawStart,
    VIDVulkanVdp1DrawEnd,
    VIDVulkanVdp1NormalSpriteDraw,
    VIDVulkanVdp1ScaledSpriteDraw,
    VIDVulkanVdp1DistortedSpriteDraw,
    VIDVulkanVdp1PolygonDraw,
    VIDVulkanVdp1PolylineDraw,
    VIDVulkanVdp1LineDraw,
    VIDVulkanVdp1UserClipping,
    VIDVulkanVdp1SystemClipping,
    VIDVulkanVdp1LocalCoordinate,
    VIDVulkanVdp1ReadFrameBuffer,
    VIDVulkanVdp1WriteFrameBuffer,
    VIDVulkanVdp1EraseWrite,
    VIDVulkanVdp1FrameChange,
    VIDVulkanVdp2Reset,
    VIDVulkanVdp2DrawStart,
    VIDVulkanVdp2DrawEnd,
    VIDVulkanVdp2DrawScreens,
    VIDGetGlSize,
    VIDVulkanSetSettingValue,
    VIDVulkanSync,
    VIDVulkanGetNativeResolution,
    VIDVulkanVdp2DispOff,
    VIDVulkanOnUpdateColorRamWord,
    VIDVulkanGetScreenshot
  };

  extern int YglCalcTextureQ(float   *pnts, float *q);

  vdp2rotationparameter_struct * FASTCALL vdp2rGetKValue2W(vdp2rotationparameter_struct * param, int index);
  vdp2rotationparameter_struct * FASTCALL vdp2rGetKValue1W(vdp2rotationparameter_struct * param, int index);
  vdp2rotationparameter_struct * FASTCALL vdp2rGetKValue2Wm3(vdp2rotationparameter_struct * param, int index);
  vdp2rotationparameter_struct * FASTCALL vdp2rGetKValue1Wm3(vdp2rotationparameter_struct * param, int index);
  vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode00NoK(vdp2draw_struct * info, int h, int v);
  vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode00WithK(vdp2draw_struct * info, int h, int v);
  vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode01NoK(vdp2draw_struct * info, int h, int v);
  vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode01WithK(vdp2draw_struct * info, int h, int v);
  vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode02NoK(vdp2draw_struct * info, int h, int v);
  vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode02WithKA(vdp2draw_struct * info, int h, int v);
  vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode02WithKAWithKB(vdp2draw_struct * info, int h, int v);
  vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode02WithKB(vdp2draw_struct * info, int h, int v);
  vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode03NoK(vdp2draw_struct * info, int h, int v);
  vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode03WithKA(vdp2draw_struct * info, int h, int v);
  vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode03WithKB(vdp2draw_struct * info, int h, int v);
  vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode03WithK(vdp2draw_struct * info, int h, int v);
  int Vdp2SetGetColor(vdp2draw_struct * info);

  extern Vdp2 * fixVdp2Regs;


  PFN_vkCmdSetStencilTestEnableEXT vkCmdSetStencilTestEnable = 0;
  PFN_vkCmdSetStencilOpEXT vkCmdSetStencilOp = 0;

}


VIDVulkan::VIDVulkan()
{
  vdp2width = 320;
  vdp2height = 240;
  SetSaturnResolution(vdp2width, vdp2height);
  crammutex = NULL; 
  pipleLineNBG0 = NULL;
  pipleLineNBG1 = NULL;
  pipleLineNBG2 = NULL;
  pipleLineNBG3 = NULL;
  pipleLineRBG0 = NULL;
  frameCount = 0;
  pipleLineFactory = NULL;
  layers.reserve(10);
}


VIDVulkan::~VIDVulkan()
{
}

#include <set>
using std::set;

std::set<std::string> get_supported_extensions() {
  uint32_t count;
  vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr); //get number of extensions
  std::vector<VkExtensionProperties> extensions(count);
  vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data()); //populate buffer
  std::set<std::string> results;
  for (auto & extension : extensions) {
    results.insert(extension.extensionName);
  }
  return results;
}

int VIDVulkan::init(void)
{
  int rtn;
  if ((rtn = VulkanScene::init()) != 0) {
    return rtn;
  }

  crammutex = YabThreadCreateMutex();
  pipleLineFactory = new VdpPipelineFactory();

  cram.create(this, 2048, 1);
  lineColor.create(this, 512, 9);
  backColor.create(this, 512, 1);

  tm = new TextureManager(this);
  tm->init(512, 1024);

  vm = new VertexManager(this);
  vm->init(1024 * 128, 1024 * 128);

  vdp1 = new Vdp1Renderer(_device_width, _device_height, this);
  vdp1->setUp();

  windowRenderer = new WindowRenderer(vdp2width, vdp2height, this);
  windowRenderer->setUp();

  fbRender = new FramebufferRenderer(this);
  fbRender->setup();

  rbgGenerator = new RBGGeneratorVulkan();
  rbgGenerator->init(this, _device_width, _device_height);

  pipleLineFactory->setRenderPath(getRenderPass());

  backPiepline = new VdpBack(this, tm, vm);
  backPiepline->setRenderPass(getRenderPass());
  backPiepline->createGraphicsPipeline();

  std::set<std::string> exts = get_supported_extensions();

  const VkDevice device = _renderer->GetVulkanDevice();
  vkCmdSetStencilTestEnable = (PFN_vkCmdSetStencilTestEnableEXT)vkGetDeviceProcAddr(device, "vkCmdSetStencilTestEnableEXT");
  vkCmdSetStencilOp = (PFN_vkCmdSetStencilOpEXT)vkGetDeviceProcAddr(device, "vkCmdSetStencilOpEXT");

  generateOffscreenRenderer();

  createCommandPool();

  g_rgb0.async = 0;
  g_rgb0.rgb_type = 0;
  g_rgb0.vdp2_sync_flg = -1;
  g_rgb0.rotate_mval_h = 1.0;
  g_rgb0.rotate_mval_v = 1.0;
  g_rgb1.async = 0;
  g_rgb1.rgb_type = 0x04;
  g_rgb1.vdp2_sync_flg = -1;
  g_rgb1.rotate_mval_h = 1.0;
  g_rgb1.rotate_mval_v = 1.0;

  return 0;
}

void VIDVulkan::deInit(void)
{
  if (vdp1 != nullptr) {
    delete vdp1;
    vdp1 = nullptr;
  }

  if (windowRenderer != nullptr) {
    delete windowRenderer;
    windowRenderer = nullptr;
  }

  if (rbgGenerator != nullptr) {
    delete rbgGenerator;
    rbgGenerator = nullptr;
  }

  if (backPiepline != nullptr) {
    delete backPiepline;
    backPiepline = nullptr;
  }

  deleteOfscreenPath();
  freeSubRenderTarget();

  if (vm != nullptr) {
    delete vm;
    vm = nullptr;
  }

  if (tm != nullptr) {
    delete tm;
    tm = nullptr;
  }

  if (crammutex != NULL) {
    YabThreadFreeMutex(crammutex);
    crammutex = NULL;
  }


  if (pipleLineNBG0 != NULL) {
    pipleLineFactory->garbage(pipleLineNBG0);
    pipleLineNBG0 = NULL;
  }

  if (pipleLineNBG1 != NULL) {
    pipleLineFactory->garbage(pipleLineNBG1);
    pipleLineNBG1 = NULL;
  }
  if (pipleLineNBG2 != NULL) {
    pipleLineFactory->garbage(pipleLineNBG2);
    pipleLineNBG2 = NULL;
  }
  if (pipleLineNBG3 != NULL) {
    pipleLineFactory->garbage(pipleLineNBG3);
    pipleLineNBG3 = NULL;
  }

  if (pipleLineRBG0 != NULL) {
    pipleLineFactory->garbage(pipleLineRBG0);
    pipleLineRBG0 = NULL;
  }

  if (pipleLineFactory != nullptr) {
    delete pipleLineFactory;
    pipleLineFactory = nullptr;
  }
  ShaderManager::free();

  VulkanScene::deInit();

  delete this;
  _instance = nullptr;

}

void VIDVulkan::resize(int originx, int originy, unsigned int w, unsigned int h, int on, int aspect_rate_mode)
{
  this->_renderer->getWindow()->resize(w, h);
  _device_width = w;
  _device_height = h;

  isFullScreen = on;
  originx = originx;
  originy = originy;
  aspect_rate_mode = aspect_rate_mode;

  int tmpw = vdp2width;
  int tmph = vdp2height;
  vdp2width = 0;   // to force update 
  vdp2height = 0;
  SetSaturnResolution(tmpw, tmph);
}

int VIDVulkan::isFullscreen(void)
{
  return isFullScreen;
}

int VIDVulkan::Vdp1Reset(void)
{
  return 0;
}

void VIDVulkan::Vdp1DrawStart(void)
{
  if (rebuildPipelines == 1) {
    pipleLineFactory->dicardAllPielines();
    vdp1->setPolygonMode(polygonMode);
    Vdp1GroundShading::polygonMode = polygonMode;
  }

  vdp1->drawStart();
}

void VIDVulkan::Vdp1DrawEnd(void)
{
  vdp1->drawEnd();
}

void VIDVulkan::Vdp1NormalSpriteDraw(u8 * ram, Vdp1 * regs, u8 * back_framebuffer)
{
  vdp1->NormalSpriteDraw(ram, regs, back_framebuffer);
}

void VIDVulkan::Vdp1ScaledSpriteDraw(u8 * ram, Vdp1 * regs, u8 * back_framebuffer)
{
  vdp1->ScaledSpriteDraw(ram, regs, back_framebuffer);
}

void VIDVulkan::Vdp1DistortedSpriteDraw(u8 * ram, Vdp1 * regs, u8 * back_framebuffer)
{
  vdp1->DistortedSpriteDraw(ram, regs, back_framebuffer);
}

void VIDVulkan::Vdp1PolygonDraw(u8 * ram, Vdp1 * regs, u8 * back_framebuffer)
{
  vdp1->PolygonDraw(ram, regs, back_framebuffer);
}

void VIDVulkan::Vdp1PolylineDraw(u8 * ram, Vdp1 * regs, u8 * back_framebuffer)
{
  vdp1->PolylineDraw(ram, regs, back_framebuffer);
}

void VIDVulkan::Vdp1LineDraw(u8 * ram, Vdp1 * regs, u8 * back_framebuffer)
{
  vdp1->LineDraw(ram, regs, back_framebuffer);
}

void VIDVulkan::Vdp1UserClipping(u8 * ram, Vdp1 * regs)
{
  vdp1->UserClipping(ram, regs);
}

void VIDVulkan::Vdp1SystemClipping(u8 * ram, Vdp1 * regs)
{
  vdp1->SystemClipping(ram, regs);
}

void VIDVulkan::Vdp1LocalCoordinate(u8 * ram, Vdp1 * regs)
{
  vdp1->LocalCoordinate(ram, regs);
}

int VIDVulkan::Vdp2Reset(void)
{
  return 0;
}

void VIDVulkan::Vdp2DrawStart(void)
{
  fixVdp2Regs = Vdp2RestoreRegs(0, Vdp2Lines);
  if (fixVdp2Regs == NULL) fixVdp2Regs = Vdp2Regs;
  memcpy(&_baseVdp2Regs, fixVdp2Regs, sizeof(Vdp2));
  fixVdp2Regs = &_baseVdp2Regs;
  ::fixVdp2Regs = &_baseVdp2Regs;
  Vdp2SetResolution(fixVdp2Regs->TVMD);
  if (rebuildFrameBuffer) {
    createCommandPool();
    vdp1->changeResolution(renderWidth, renderHeight);
    vdp1->setVdp2Resolution(vdp2width, vdp2height);
    windowRenderer->changeResolution(vdp2width, vdp2height);
    generateOffscreenPath(vdp2width, vdp2height);
    if (resolutionMode == RES_NATIVE) {
      freeSubRenderTarget();
    }
    else {
      freeSubRenderTarget();
      generateSubRenderTarget(renderWidth, renderHeight);
    }
    rebuildFrameBuffer = 0;
  }
  vkResetCommandPool(getDevice(), this->_command_pool, 0);
  tm->reset();
  tm->resetCache();
  vm->reset();
}

void VIDVulkan::Vdp2DrawEnd(void)
{
  if (_renderer == nullptr) return;
  const VkDevice device = _renderer->GetVulkanDevice();
  if (device == VK_NULL_HANDLE) return;

  backPiepline->moveToVertexBuffer(backPiepline->vertices, backPiepline->indices);
  backPiepline->setSampler(VdpPipeline::bindIdTexture, backColor.imageView, backColor.sampler);

  for (uint32_t i = 0; i < layers.size(); i++) {
    for (uint32_t j = 0; j < layers[i].size(); j++) {
      layers[i][j]->moveToVertexBuffer(layers[i][j]->vertices, layers[i][j]->indices);

      if (layers[i][j]->interuput_texture != VK_NULL_HANDLE) {
        layers[i][j]->setSampler(VdpPipeline::bindIdTexture, layers[i][j]->interuput_texture, tm->getTextureSampler());
        layers[i][j]->setSampler(VdpPipeline::bindIdLine, tm->geTextureImageView(), tm->getTextureSampler());
      }
      else {
        layers[i][j]->setSampler(VdpPipeline::bindIdTexture, tm->geTextureImageView(), tm->getTextureSampler());
        layers[i][j]->setSampler(VdpPipeline::bindIdLine, lineColor.imageView, lineColor.sampler);
      }
      layers[i][j]->setSampler(VdpPipeline::bindIdColorRam, getCramImageView(), getCramSampler());


      layers[i][j]->setSampler(VdpPipeline::bindIdWindow, windowRenderer->getImageView(), windowRenderer->getSampler());

    }
  }

  tm->updateTextureImage([&](VkCommandBuffer commandBuffer)
  {
    updateColorRam(commandBuffer);
    lineColor.update(this, commandBuffer);
    backColor.update(this, commandBuffer);
    vm->flush(commandBuffer);
    windowRenderer->flush(commandBuffer);
    fbRender->flush(commandBuffer);

    for (uint32_t i = 0; i < layers.size(); i++) {
      for (uint32_t j = 0; j < layers[i].size(); j++) {
        if (layers[i][j]->lineTexture != 0) {
          if (perline[0].imageView == layers[i][j]->lineTexture) perline[0].update(this, commandBuffer);
          if (perline[1].imageView == layers[i][j]->lineTexture) perline[1].update(this, commandBuffer);
          if (perline[2].imageView == layers[i][j]->lineTexture) perline[2].update(this, commandBuffer);
          if (perline[3].imageView == layers[i][j]->lineTexture) perline[3].update(this, commandBuffer);
          if (perline[4].imageView == layers[i][j]->lineTexture) perline[4].update(this, commandBuffer);
        }
      }
    }
  }
  );

  fbRender->setFrameBuffer(vdp1->getFrameBufferImage());
  fbRender->setColorRam(cram.imageView);
  fbRender->setLineColor(lineColor.imageView);

  static auto startTime = std::chrono::high_resolution_clock::now();
  auto currentTime = std::chrono::high_resolution_clock::now();
  float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();


  int fi = _renderer->getWindow()->BeginRender();


  VkCommandBufferBeginInfo command_buffer_begin_info{};
  command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  VkRect2D render_area{};
  render_area.offset.x = 0;
  render_area.offset.y = 0;
  render_area.extent = _renderer->getWindow()->GetVulkanSurfaceSize();

  std::array<VkClearValue, 2> clear_values{};
  clear_values[1].depthStencil.depth = 0.0f;
  clear_values[1].depthStencil.stencil = 0;
  clear_values[0].color.float32[0] = 0.0;
  clear_values[0].color.float32[1] = 0.0;
  clear_values[0].color.float32[2] = 0.0;
  clear_values[0].color.float32[3] = 0.0f;

  VkRenderPassBeginInfo render_pass_begin_info{};
  render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_begin_info.renderPass = _renderer->getWindow()->GetVulkanRenderPass();
  render_pass_begin_info.framebuffer = _renderer->getWindow()->GetVulkanActiveFramebuffer();
  render_pass_begin_info.renderArea = render_area;
  render_pass_begin_info.clearValueCount = clear_values.size();
  render_pass_begin_info.pClearValues = clear_values.data();


  vkBeginCommandBuffer(_command_buffers[fi], &command_buffer_begin_info);

  if ((Vdp2Regs->TVMD & 0x8000) == 0) {

    VkRenderPassBeginInfo render_pass_begin_info{};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass = _renderer->getWindow()->GetVulkanRenderPass();
    render_pass_begin_info.framebuffer = _renderer->getWindow()->GetVulkanActiveFramebuffer();
    render_pass_begin_info.renderArea = render_area;
    render_pass_begin_info.clearValueCount = clear_values.size();
    render_pass_begin_info.pClearValues = clear_values.data();
    vkCmdBeginRenderPass(_command_buffers[fi], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    NanovgVulkanSetDevices(device, this->getPhysicalDevice(), _renderer->getWindow()->GetVulkanRenderPass(), _command_buffers[fi]);
    OSDDisplayMessages(NULL, 0, 0);
    vkCmdEndRenderPass(_command_buffers[fi]);

  }
  else {

    windowRenderer->draw(_command_buffers[fi]);

    if (resolutionMode != RES_NATIVE) {
      VkRect2D tmp_render_area{};
      tmp_render_area.offset.x = 0;
      tmp_render_area.offset.y = 0;
      tmp_render_area.extent.width = renderWidth;
      tmp_render_area.extent.height = renderHeight;

      VkRenderPassBeginInfo tmp_render_pass_begin_info{};
      tmp_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      tmp_render_pass_begin_info.renderPass = subRenderTarget.renderPass;
      tmp_render_pass_begin_info.framebuffer = subRenderTarget.frameBuffer;
      tmp_render_pass_begin_info.renderArea = tmp_render_area;
      tmp_render_pass_begin_info.clearValueCount = clear_values.size();
      tmp_render_pass_begin_info.pClearValues = clear_values.data();
      vkCmdBeginRenderPass(_command_buffers[fi], &tmp_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    }
    else {
      vkCmdBeginRenderPass(_command_buffers[fi], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    }

    auto c = vk::CommandBuffer(_command_buffers[fi]);
    vk::Viewport viewport;
    vk::Rect2D scissor;
    if (resolutionMode != RES_NATIVE) {
      viewport.width = renderWidth;
      viewport.height = renderHeight;
      viewport.x = 0;
      viewport.y = 0;
      viewport.minDepth = 0.0f;
      viewport.maxDepth = 1.0f;
      c.setViewport(0, 1, &viewport);

      scissor.extent.width = renderWidth;
      scissor.extent.height = renderHeight;
      scissor.offset.x = 0;
      scissor.offset.y = 0;
      c.setScissor(0, 1, &scissor);

    }
    else {
      viewport.width = renderWidth;
      viewport.height = renderHeight;
      viewport.x = originx;
      viewport.y = originy;
      viewport.minDepth = 0.0f;
      viewport.maxDepth = 1.0f;
      c.setViewport(0, 1, &viewport);

      scissor.extent.width = renderWidth;
      scissor.extent.height = renderHeight;
      scissor.offset.x = originx;
      scissor.offset.y = originy;
      c.setScissor(0, 1, &scissor);
    }


    UniformBufferObject ubo = {};
    ubo.emu_height = (float)vdp2height / (float)renderHeight;
    ubo.vheight = renderHeight;
    ubo.viewport_offset = viewport.y;


    fbRender->onStartFrame(fixVdp2Regs, _command_buffers[fi]);

    int from = 0;
    int to = 0;
    bool fbRendered = false;
    bool isDestinationAlpha = (((Vdp2Regs->CCCTL >> 9) & 0x01) == 0x01);

    vector<VdpPipeline*> onFramePipelines;

    glm::mat4 m4(1.0f);
    glm::mat4 model = m4;
    glm::mat4 proj = glm::ortho(0.0f, (float)vdp2width, 0.0f, (float)vdp2height, 10.0f, 0.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, -1.0));

    // Draw Back Color 
    if (backPiepline->vertices.size() > 0) {
      glm::mat4 MVP = model * proj;
      ubo.mvp = MVP;
      ubo.blendmode = backPiepline->blendmode;
      backPiepline->setUBO(&ubo, sizeof(ubo));
      backPiepline->updateDescriptorSets();
      vkCmdBindDescriptorSets(_command_buffers[fi], VK_PIPELINE_BIND_POINT_GRAPHICS,
        backPiepline->getPipelineLayout(), 0, 1, &backPiepline->_descriptorSet, 0, nullptr);

      vkCmdBindPipeline(_command_buffers[fi], VK_PIPELINE_BIND_POINT_GRAPHICS, backPiepline->getGraphicsPipeline());

      VkBuffer vertexBuffers[] = { vm->getVertexBuffer(backPiepline->vectexBlock) };
      VkDeviceSize offsets[] = { backPiepline->vertexOffset };

      vkCmdBindVertexBuffers(_command_buffers[fi], 0, 1, vertexBuffers, offsets);

      vkCmdBindIndexBuffer(_command_buffers[fi],
        vm->getIndexBuffer(backPiepline->vectexBlock),
        backPiepline->indexOffset, VK_INDEX_TYPE_UINT16);

      vkCmdDrawIndexed(_command_buffers[fi], backPiepline->indexSize, 1, 0, 0, 0);
    }



    for (uint32_t i = 0; i < layers.size(); i++) {

      glm::mat4 MVP = model * proj;
      ubo.mvp = MVP;
      //void* data;
      //vkMapMemory(device, _uniformBufferMemory, 0, sizeof(ubo), 0, &data);
      //memcpy(data, &ubo, sizeof(ubo));
      //vkUnmapMemory(device, _uniformBufferMemory);

      fbRendered = false;
      for (uint32_t j = 0; j < layers[i].size(); j++) {
        if (layers[i][j]->vertexSize > 0) {

          if (layers[i][j]->blendmode != 0 && fbRendered == false) {
            to = i;
            if (Vdp1External.disptoggle & 0x01) {
              if (isDestinationAlpha) {
                fbRender->drawWithDestAlphaMode(fixVdp2Regs, _command_buffers[fi], from, to);
              }
              else {
                fbRender->draw(fixVdp2Regs, _command_buffers[fi], from, to);
              }
            }
            from = to;
            fbRendered = true;
          }

          VdpPipeline * prg = layers[i][j];

          ubo.u_specialColorFunc = 0;
          ubo.specialPriority = prg->specialPriority;

          ubo.color_offset.r = prg->color_offset_val[0];
          ubo.color_offset.g = prg->color_offset_val[1];
          ubo.color_offset.b = prg->color_offset_val[2];
          ubo.color_offset.a = 0;
          ubo.blendmode = prg->blendmode;


          if (prg->bwin0 || prg->bwin1 || prg->bwinsp)
          {
            u8 bwin1 = prg->bwin1 << 1;
            u8 logwin1 = prg->logwin1 << 1;
            u8 bwinsp = prg->bwinsp << 2;
            u8 logwinsp = prg->logwinsp << 2;

            int winmask = (prg->bwin0 | bwin1 | bwinsp);
            int winflag = 0;
            if (prg->winmode == 0) { // and
              if (prg->bwin0)  winflag = prg->logwin0;
              if (prg->bwin1)  winflag |= logwin1;
              if (prg->bwinsp) winflag |= logwinsp;
            }
            else { // or
              winflag = winmask;
              if (prg->bwin0)  winflag &= ~prg->logwin0;
              if (prg->bwin1)  winflag &= ~logwin1;
              if (prg->bwinsp) winflag &= ~logwinsp;
            }
            ubo.winmode = prg->winmode;
            ubo.winmask = winmask;
            ubo.winflag = winflag;
            ubo.offsetx = viewport.x;
            ubo.offsety = viewport.y;
            ubo.windowWidth = viewport.width;
            ubo.windowHeight = viewport.height;

          }
          else {
            ubo.winmode = -1;
            ubo.winmask = -1;
            ubo.winflag = -1;
            ubo.windowWidth = -1;
            ubo.windowHeight = -1;

          }

          // Goto Mosaic render pass 
          if (layers[i][j]->mosaic[0] != 1 || layers[i][j]->mosaic[1] != 1) {

            UniformBufferObject ubomini = ubo;
            ubomini.offsetx = 0;
            ubomini.offsety = 0;
            ubomini.windowWidth = offscreenPass.width;
            ubomini.windowHeight = offscreenPass.height;
            layers[i][j]->setUBO(&ubomini, sizeof(ubomini));
            layers[i][j]->updateDescriptorSets();
            renderToOffecreenTarget(_command_buffers[fi], layers[i][j]);

            ubo.u_tw = vdp2width;
            ubo.u_th = vdp2height;
            ubo.vheight = (i / 10.0f);
            ubo.u_mosaic_x = layers[i][j]->mosaic[0];
            ubo.u_mosaic_y = layers[i][j]->mosaic[1];
            renderEffectToMainTarget(_command_buffers[fi], ubo, 1);
          }

          // per line color calculation mode
          else if (layers[i][j]->lineTexture != VK_NULL_HANDLE && layers[i][j]->specialPriority == 0) {

            UniformBufferObject ubomini = ubo;
            ubomini.offsetx = 0;
            ubomini.offsety = 0;
            ubomini.windowWidth = offscreenPass.width;
            ubomini.windowHeight = offscreenPass.height;
            layers[i][j]->setUBO(&ubomini, sizeof(ubomini));
            layers[i][j]->updateDescriptorSets();
            renderToOffecreenTarget(_command_buffers[fi], layers[i][j]);

            ubo.u_tw = vdp2width;
            ubo.u_th = vdp2height;
            ubo.vheight = (i / 10.0f);
            ubo.u_specialColorFunc = prg->specialcolormode;

            VdpPipeline * p;
            if (isDestinationAlpha) {
              p = pipleLineFactory->getPipeline(PG_VDP2_PER_LINE_ALPHA_DST, this, this->tm, this->vm);
            }
            else {
              p = pipleLineFactory->getPipeline(PG_VDP2_PER_LINE_ALPHA, this, this->tm, this->vm);
            }
            renderWithLineEffectToMainTarget(p, _command_buffers[fi], ubo, layers[i][j]->lineTexture);
            onFramePipelines.push_back(p);

          }

          // Normal mode
          else {

            if (layers[i][j]->lineTexture != VK_NULL_HANDLE) {
              layers[i][j]->setSampler(VdpPipeline::bindIdLine, layers[i][j]->lineTexture, lineColor.sampler);
            }

            layers[i][j]->setUBO(&ubo, sizeof(ubo));
            layers[i][j]->updateDescriptorSets();

            vkCmdBindDescriptorSets(_command_buffers[fi], VK_PIPELINE_BIND_POINT_GRAPHICS,
              layers[i][j]->getPipelineLayout(), 0, 1, &layers[i][j]->_descriptorSet, 0, nullptr);

            vkCmdBindPipeline(_command_buffers[fi], VK_PIPELINE_BIND_POINT_GRAPHICS, layers[i][j]->getGraphicsPipeline());

            VkBuffer vertexBuffers[] = { vm->getVertexBuffer(layers[i][j]->vectexBlock) };
            VkDeviceSize offsets[] = { layers[i][j]->vertexOffset };

            vkCmdBindVertexBuffers(_command_buffers[fi], 0, 1, vertexBuffers, offsets);

            vkCmdBindIndexBuffer(_command_buffers[fi],
              vm->getIndexBuffer(layers[i][j]->vectexBlock),
              layers[i][j]->indexOffset, VK_INDEX_TYPE_UINT16);

            vkCmdDrawIndexed(_command_buffers[fi], layers[i][j]->indexSize, 1, 0, 0, 0);
          }
        }
      }
      model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.1));

    }
    if (Vdp1External.disptoggle & 0x01) {
      if (isDestinationAlpha) {
        fbRender->drawWithDestAlphaMode(fixVdp2Regs, _command_buffers[fi], from, 10);
      }
      else {
        fbRender->draw(fixVdp2Regs, _command_buffers[fi], from, 10);
      }
    }

    for (int i = 0; i < onFramePipelines.size(); i++) {
      this->pipleLineFactory->garbage(onFramePipelines[i]);
    }

    if ((fixVdp2Regs->SDCTL & 0xFF) != 0 || this->vdp1->getMsbShadowCount() != 0) {
      fbRender->drawShadow(fixVdp2Regs, _command_buffers[fi], 0, 10);
    }

    if (resolutionMode == RES_NATIVE) {
      NanovgVulkanSetDevices(device, this->getPhysicalDevice(), _renderer->getWindow()->GetVulkanRenderPass(), _command_buffers[fi]);
      OSDDisplayMessages(NULL, 0, 0);
    }

    vkCmdEndRenderPass(_command_buffers[fi]);

    if (resolutionMode != RES_NATIVE) {
      vkCmdBeginRenderPass(_command_buffers[fi], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
      auto c = vk::CommandBuffer(_command_buffers[fi]);
      vk::Viewport viewport;
      vk::Rect2D scissor;
      viewport.width = finalWidth;
      viewport.height = finalHeight;
      viewport.x = originx;
      viewport.y = originy;
      viewport.minDepth = 0.0f;
      viewport.maxDepth = 1.0f;
      c.setViewport(0, 1, &viewport);
      scissor.extent.width = finalWidth;
      scissor.extent.height = finalHeight;
      scissor.offset.x = originx;
      scissor.offset.y = originy;
      c.setScissor(0, 1, &scissor);
      vkCmdEndRenderPass(_command_buffers[fi]);
      blitSubRenderTarget(_command_buffers[fi]);
      NanovgVulkanSetDevices(device, this->getPhysicalDevice(), _renderer->getWindow()->GetVulkanRenderPass(), _command_buffers[fi]);
      OSDDisplayMessages(NULL, 0, 0);
    }
  }


  vkEndCommandBuffer(_command_buffers[fi]);


  vector<VkSemaphore> waitSem;

  VkSemaphore texSem = tm->getCompleteSemaphore();
  if (texSem != VK_NULL_HANDLE) {
    waitSem.push_back(texSem);
  }

  VkSemaphore rbgSem = rbgGenerator->getCompleteSemaphore();
  if (rbgSem != VK_NULL_HANDLE) {
    waitSem.push_back(rbgSem);
  }

  /*
    VkSemaphore fbSem = vdp1->getFrameBufferSem();
    if (fbSem != VK_NULL_HANDLE) {
      waitSem.push_back(fbSem);
    }
  */
  VkPipelineStageFlags graphicsWaitStageMasks[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
  VkSemaphore graphicsWaitSemaphores[] = { rbgGenerator->getCompleteSemaphore() };

  // Submit command buffer
  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.waitSemaphoreCount = waitSem.size();
  submit_info.pWaitSemaphores = waitSem.data();
  submit_info.pWaitDstStageMask = graphicsWaitStageMasks;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &_command_buffers[fi];
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = &_render_complete_semaphore;
  ErrorCheck(vkQueueSubmit(_renderer->GetVulkanQueue(), 1, &submit_info, VK_NULL_HANDLE));

  //vkQueueWaitIdle(getVulkanQueue());
  //vkDeviceWaitIdle(device);


#if 0
  if (resolutionMode != RES_NATIVE) {

    VkCommandBufferAllocateInfo cmdBufAllocateInfo = vks::initializers::commandBufferAllocateInfo(_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
    VkCommandBuffer cmdBuffer;
    VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &cmdBuffer));
    // If requested, also start recording for the new command buffer
    VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
    VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));

    vkCmdBeginRenderPass(cmdBuffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    auto c = vk::CommandBuffer(cmdBuffer);
    vk::Viewport viewport;
    vk::Rect2D scissor;
    viewport.width = finalWidth;
    viewport.height = finalHeight;
    viewport.x = originx;
    viewport.y = originy;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    c.setViewport(0, 1, &viewport);

    scissor.extent.width = finalWidth;
    scissor.extent.height = finalHeight;
    scissor.offset.x = originx;
    scissor.offset.y = originy;
    c.setScissor(0, 1, &scissor);
    vkCmdEndRenderPass(cmdBuffer);

    blitSubRenderTarget(cmdBuffer);

    VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));

    VkSubmitInfo submitInfo = vks::initializers::submitInfo();
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer;
    // Create fence to ensure that the command buffer has finished executing
    VkFenceCreateInfo fenceInfo = vks::initializers::fenceCreateInfo(VK_FLAGS_NONE);
    VkFence fence;
    VK_CHECK_RESULT(vkCreateFence(device, &fenceInfo, nullptr, &fence));
    // Submit to the queue
    VK_CHECK_RESULT(vkQueueSubmit(getVulkanQueue(), 1, &submitInfo, fence));
    // Wait for the fence to signal that command buffer has finished executing
    VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));
    vkDestroyFence(device, fence, nullptr);
    vkFreeCommandBuffers(device, _command_pool, 1, &cmdBuffer);
  }
#endif

  YuiSwapBuffers();
  fbRender->onEndFrame();
  rbgGenerator->onFinish();
  frameCount++;

}



void VIDVulkan::readVdp2ColorOffset(Vdp2 * regs, vdp2draw_struct *info, int mask)
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
  }
  else { // color offset disable
    info->cor = 0;
    info->cob = 0;
    info->cog = 0;

  }
}

#define Y_MAX(a, b) ((a) > (b) ? (a) : (b))

void VIDVulkan::updateBackColor() {

  u32 scrAddr;
  int dot;
  vdp2draw_struct info;

  static unsigned char lineColors[512 * 3];
  static int line[512 * 4];

  if (fixVdp2Regs->VRSIZE & 0x8000)
    scrAddr = (((fixVdp2Regs->BKTAU & 0x7) << 16) | fixVdp2Regs->BKTAL) * 2;
  else
    scrAddr = (((fixVdp2Regs->BKTAU & 0x3) << 16) | fixVdp2Regs->BKTAL) * 2;

  readVdp2ColorOffset(fixVdp2Regs, &info, 0x20);
  dot = T1ReadWord(Vdp2Ram, scrAddr);
  u32* back_pixel_data = backColor.dynamicBuf;
  int lineCount = 1;
  if ((fixVdp2Regs->BKTAU & 0x8000) != 0) {
    lineCount = vdp2height;
  }

  for (int i = 0; i < lineCount; i++) {
    u8 r, g, b, a;
    dot = T1ReadWord(Vdp2Ram, (scrAddr + 2 * i));
    r = Y_MAX(((dot & 0x1F) << 3) + info.cor, 0);
    g = Y_MAX((((dot & 0x3E0) >> 5) << 3) + info.cog, 0);
    b = Y_MAX((((dot & 0x7C00) >> 10) << 3) + info.cob, 0);
    if (fixVdp2Regs->CCCTL & 0x2) {
      a = 0xFF;
    }
    else {
      a = ((~fixVdp2Regs->CCRLB & 0x1F00) >> 5) + 0x7;
    }
    *back_pixel_data++ = (a << 24) | ((b & 0xFF) << 16) | ((g & 0xFF) << 8) | (r & 0xFF);
  }
  genPolygonBack();
}


void VIDVulkan::updateLineColor(void) {
  u32 cacheaddr = 0xFFFFFFFF;
  int inc = 0;
  int line_cnt = vdp2height;
  int i = 0;
  u32 * line_pixel_data = NULL;
  u32 addr = 0;

  if (fixVdp2Regs->LNCLEN == 0) return;

  line_pixel_data = lineColor.dynamicBuf;
  if (line_pixel_data == NULL) {
    return;
  }

  if ((fixVdp2Regs->LCTA.part.U & 0x8000)) {
    inc = 0x02; // single color
  }
  else {
    inc = 0x00; // color per line
  }

  u8 alpha = 0xFF;
  // Color calculation ratio mode Destination Alpha
  // Use blend value CRLB
  if ((fixVdp2Regs->CCCTL & 0x0300) == 0x0200) {
    alpha = (float)(fixVdp2Regs->CCRLB & 0x1F) / 32.0f * 255.0f;
  }

  addr = (fixVdp2Regs->LCTA.all & 0x7FFFF) * 0x2;
  for (i = 0; i < line_cnt; i++) {
    u16 LineColorRamAdress = T1ReadWord(Vdp2Ram, addr);
    *(line_pixel_data) = Vdp2ColorRamGetColor(LineColorRamAdress, alpha);
    line_pixel_data++;
    addr += inc;
  }

}


void VIDVulkan::Vdp2DrawScreens(void)
{
  u32 scrAddr;
  int dot;
  vdp2draw_struct info;

#if 0
  readVdp2ColorOffset(fixVdp2Regs, &info, 0x20);

  if (fixVdp2Regs->VRSIZE & 0x8000)
    scrAddr = (((fixVdp2Regs->BKTAU & 0x7) << 16) | fixVdp2Regs->BKTAL) * 2;
  else
    scrAddr = (((fixVdp2Regs->BKTAU & 0x3) << 16) | fixVdp2Regs->BKTAL) * 2;

  dot = T1ReadWord(Vdp2Ram, scrAddr);
  setClearColor(
    (float)(((dot & 0x1F) << 3) + info.cor) / (float)(0xFF),
    (float)((((dot & 0x3E0) >> 5) << 3) + info.cog) / (float)(0xFF),
    (float)((((dot & 0x7C00) >> 10) << 3) + info.cob) / (float)(0xFF));
#endif

  layers.clear();
  layers.resize(10);
  windowRenderer->generateWindowInfo(fixVdp2Regs, 0);
  windowRenderer->generateWindowInfo(fixVdp2Regs, 1);
  updateLineColor();
  updateBackColor();
  drawNBG3();
  drawNBG2();
  drawNBG1();
  drawNBG0();
  drawRBG0();
}

void VIDVulkan::Vdp2SetResolution(u16 TVMD)
{
  int width = 0, height = 0;
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
  default: break;
  }

  hratio = 1;

  // Check for interlace
  switch ((TVMD >> 6) & 0x3)
  {
  case 3: // Double-density Interlace
    height *= 2;
    hratio = 2;
    _vdp2_interlace = 1;
    break;
  case 2: // Single-density Interlace
  case 0: // Non-interlace
  default:
    _vdp2_interlace = 0;
    break;
  }

  SetSaturnResolution(width, height);
  vdp1->setTextureRatio(wratio, hratio);
}

void VIDVulkan::GetGlSize(int * width, int * height)
{
  *width = this->renderWidth;
  *height = this->renderHeight;
}

void VIDVulkan::Vdp1ReadFrameBuffer(u32 type, u32 addr, void * out)
{
  this->vdp1->readFrameBuffer(type, addr, out);
}

void VIDVulkan::SetFilterMode(int type)
{
}

void VIDVulkan::Sync()
{
}

void VIDVulkan::Vdp1WriteFrameBuffer(u32 type, u32 addr, u32 val)
{
  this->vdp1->writeFrameBuffer(type, addr, val);
}

void VIDVulkan::Vdp1EraseWrite(void)
{
  vdp1->erase();
}

void VIDVulkan::Vdp1FrameChange(void)
{
  vdp1->change();
}

void VIDVulkan::SetSettingValue(int type, int value)
{

  switch (type) {
  case VDP_SETTING_FILTERMODE:
    //_Ygl->aamode = value; ToDo
    break;
  case VDP_SETTING_RESOLUTION_MODE:
    if (resolutionMode != value) {
      resolutionMode = value;
      rebuildFrameBuffer = 1; // rebuild frame buffers at the top of frame
    }
    break;
  case VDP_SETTING_RBG_RESOLUTION_MODE:
    rbgResolutionMode = value;
    break;

  case VDP_SETTING_RBG_USE_COMPUTESHADER:
    /* Force use compute Shader
        _Ygl->rbg_use_compute_shader = value;
        if (_Ygl->rbg_use_compute_shader) {
          g_rgb0.async = 0;
        }
        else {
          g_rgb0.async = 1;
        }
    */
    break;
  case VDP_SETTING_POLYGON_MODE:
      if (polygonMode != value) {
        if (!this->_renderer->isTessellationAvailable() && value == GPU_TESSERATION) {
          polygonMode = PERSPECTIVE_CORRECTION;
          YuiErrorMsg("This device does not support GPU Tesseration." );
          return;
        }
        rebuildPipelines = 1; // rebuild frame buffers at the top of frame
      }
      polygonMode = (POLYGONMODE)value;
    break;
    //case VDP_SETTING_ROTATE_SCREEN:
      //_Ygl->rotate_screen = value;
    //  break;
  }
}

void VIDVulkan::GetNativeResolution(int * width, int * height, int * interlace)
{
}

void VIDVulkan::Vdp2DispOff(void)
{
}

void VIDVulkan::setClearColor(float r, float g, float b)
{
  _clear_r = r;
  _clear_g = g;
  _clear_b = b;
}

void VIDVulkan::SetSaturnResolution(int width, int height) {

  //YglChangeResolution(width, height); ToDo
  //YglSetDensity((vdp2_interlace == 0) ? 1 : 2); ToDo


  if (vdp2width != width || vdp2height != height) {

    int deviceWidth = _renderer->getWindow()->GetVulkanSurfaceSize().width;
    int deviceHeight = _renderer->getWindow()->GetVulkanSurfaceSize().height;
    renderHeight = deviceHeight;
    renderWidth = deviceWidth;

    vdp2width = width;
    vdp2height = height;
    if (deviceWidth != 0 && deviceHeight != 0) {
      originx = 0;
      originy = 0;

      if (aspect_rate_mode != FULL) {

        float hrate;
        float wrate;
        switch (aspect_rate_mode) {
        case ORIGINAL:
          hrate = (float)((vdp2height > 256) ? vdp2height / 2 : vdp2height) / (float)((vdp2width > 352) ? vdp2width / 2 : vdp2width);
          wrate = (float)((vdp2width > 352) ? vdp2width / 2 : vdp2width) / (float)((vdp2height > 256) ? vdp2height / 2 : vdp2height);
          break;
        case _4_3:
          hrate = 3.0 / 4.0;
          wrate = 4.0 / 3.0;
          break;
        case _16_9:
          hrate = 9.0 / 16.0;
          wrate = 16.0 / 9.0;
          break;
        }

        if (rotate_screen) {
          if (isFullScreen) {
            if (deviceWidth > deviceHeight) {
              originy = deviceHeight - (deviceHeight - deviceWidth * wrate);
              renderHeight = deviceWidth * wrate;
              renderWidth = deviceWidth;
            }
            else {
              originx = (deviceWidth - deviceHeight * hrate) / 2.0f;
              renderWidth = deviceHeight * hrate;
              renderHeight = deviceHeight;
            }
          }
          else {
            originx = (deviceWidth - deviceHeight * hrate) / 2.0f;
            renderWidth = deviceHeight * hrate;
            renderHeight = deviceHeight;
          }
        }
        else {
          if (isFullScreen) {
            if (deviceHeight > deviceWidth) {
              originy = 0; // deviceWidth * hrate; // deviceHeight; // -(deviceHeight - deviceWidth * hrate);
              renderHeight = deviceWidth * hrate;
              renderWidth = deviceWidth;
            }
            else {
              originx = (deviceWidth - deviceHeight * wrate) / 2.0f;
              renderWidth = deviceHeight * wrate;
              renderHeight = deviceHeight;
            }
          }
          else {

            if (deviceHeight > deviceWidth) {
              originy = (deviceHeight - deviceWidth * hrate) / 2.0f;
              renderHeight = deviceWidth * hrate;
              renderWidth = deviceWidth;
            }
            else {
              originx = (deviceWidth - deviceHeight * wrate) / 2.0f;
              renderWidth = deviceHeight * wrate;
              renderHeight = deviceHeight;
              if (originx < 0) {
                originx = 0;
                originy = (deviceHeight - deviceWidth * hrate) / 2.0f;
                renderHeight = deviceWidth * hrate;
                renderWidth = deviceWidth;
              }
            }
          }
        }
      }
    }

    finalWidth = renderWidth;
    finalHeight = renderHeight;

    switch (resolutionMode) {
    case RES_NATIVE:
      //renderWidth = GlWidth;
      //renderHeight = GlHeight;
      rebuildFrameBuffer = 1;
      break;
    case RES_4x:
      renderWidth = vdp2width * 4;
      renderHeight = vdp2height * 4;
      rebuildFrameBuffer = 1;
      break;
    case RES_2x:
      renderWidth = vdp2width * 2;
      renderHeight = vdp2height * 2;
      rebuildFrameBuffer = 1;
      break;
    case RES_ORIGINAL:
      renderWidth = vdp2width;
      renderHeight = vdp2height;
      rebuildFrameBuffer = 1;
      break;
    case RES_720P:
      renderWidth = 1280;
      renderHeight = 720;
      rebuildFrameBuffer = 1;
      break;
    case RES_1080P:
      renderWidth = 1920;
      renderHeight = 1080;
      rebuildFrameBuffer = 1;
      break;
    }
  }

}

// VdpPipeline ** pipleLine out �������ꂽ�p�C�v���C���I�u�W�F�N�g
// vdp2draw_struct * input  in �����p�����[�^
// CharTexture * output out �������ꂽ�e�N�X�`����� NULL�̏ꍇ,�L���b�V�����W���g��
// float * colors ���_���Ƃ̐F VDP2�ł͎g��Ȃ�
// TextureCache * c �L���b�V�����W
// int cash_flg 1�̏ꍇ c�̃L���b�V�����W���X�V����
int VIDVulkan::genPolygon(VdpPipeline ** pipleLine, vdp2draw_struct * input, CharTexture * output, float * colors, TextureCache * c, int cash_flg) {
  unsigned int x, y;
  VdpPipeline *program;
  //texturecoordinate_struct *tmp;
  float * vtxa;
  float q[4];
  YglPipelineId prg = PG_NORMAL;
  float * pos;
  //float * vtxa;

  const int CCRTMD = ((fixVdp2Regs->CCCTL >> 9) & 0x01); // hard/vdp2/hon/p12_14.htm#CCRTMD_

  if (input->colornumber >= 3) {

    if ((input->blendmode & 0x03) == VDP2_CC_NONE) {
      prg = PG_VDP2_NOBLEND;
    }
    else if ((input->blendmode & 0x03) == VDP2_CC_ADD) {
      prg = PG_VDP2_ADDBLEND;
    }
    else {
      if (CCRTMD == 0) {
        prg = PG_NORMAL; // src alpha
      }
      else {
        prg = PG_NORMAL_DSTALPHA; // dst alpha
      }
    }

    if (input->mosaicxmask != 1 || input->mosaicymask != 1) {
      prg = PG_VDP2_MOSAIC;
    }
    /*
        if ((input->blendmode & VDP2_CC_BLUR) != 0) {
          prg = PG_VDP2_BLUR;
        }
    */
    if (input->linescreen == 1) {
      prg = PG_LINECOLOR_INSERT;
      if (((Vdp2Regs->CCCTL >> 9) & 0x01)) {
        prg = PG_LINECOLOR_INSERT_DESTALPHA;
      }
    }
    else if (input->linescreen == 2) { // per line operation by HBLANK
      if (input->specialprimode == 2) {
        prg = PG_VDP2_NORMAL_CRAM_SPECIAL_PRIORITY_COLOROFFSET; // Assault Leynos 2
      }
      else {
        prg = PG_VDP2_PER_LINE_ALPHA_CRAM;
      }
    }

  }
  else {
    if ((input->blendmode & 0x03) == VDP2_CC_NONE) {
      prg = PG_VDP2_NOBLEND_CRAM;
    }
    else if ((input->blendmode & 0x03) == VDP2_CC_ADD) {
      prg = PG_VDP2_ADDCOLOR_CRAM;
    }
    else { // VDP2_CC_RATE
      if (CCRTMD == 0) {
        prg = PG_VDP2_NORMAL_CRAM; // src alpha
      }
      else {
        prg = PG_VDP2_NORMAL_CRAM_DSTALPHA; // dst alpha
      }

    }

    if (input->mosaicxmask != 1 || input->mosaicymask != 1) {
    }
    /*
        if (((input->blendmode & VDP2_CC_BLUR) != 0)) {
          prg = PG_VDP2_BLUR_CRAM;
        }
    */
    if (input->linescreen == 1) {
      prg = PG_LINECOLOR_INSERT_CRAM;
      if (((Vdp2Regs->CCCTL >> 9) & 0x01)) {
        prg = PG_LINECOLOR_INSERT_DESTALPHA_CRAM;
      }
    }
    else if (input->linescreen == 2) { // per line operation by HBLANK
      prg = PG_VDP2_NOBLEND_CRAM;
      //if (input->specialprimode == 2) {
      //  prg = PG_VDP2_NORMAL_CRAM_SPECIAL_PRIORITY_COLOROFFSET; // Assault Leynos 2
      //}
      //else {
      //     prg = PG_VDP2_PER_LINE_ALPHA_CRAM;
      //}
    }

  }

  if (*pipleLine == NULL) {
    *pipleLine = pipleLineFactory->getPipeline(prg, this, this->tm, this->vm);
  }
  else if ((*pipleLine)->prgid != prg) {
    pipleLineFactory->garbage(*pipleLine);
    *pipleLine = pipleLineFactory->getPipeline(prg, this, this->tm, this->vm);
  }

  program = *pipleLine;

  program->priority = input->priority;

  program->bwin0 = input->bEnWin0;
  program->logwin0 = input->WindowArea0;
  program->bwin1 = input->bEnWin1;
  program->logwin1 = input->WindowArea1;
  program->bwinsp = 0;
  program->logwinsp = 0;
  program->blendmode = input->blendmode;
  program->winmode = input->LogicWin;
  program->mosaic[0] = input->mosaicxmask;
  program->mosaic[1] = input->mosaicymask;
  program->specialPriority = input->specialprimode;
  program->interuput_texture = VK_NULL_HANDLE;
  program->lineTexture = (VkImageView)input->lineTexture;
  program->specialcolormode = input->specialcolormode;


  Vertex tmp[4];
  tmp[0].pos[0] = (input->vertices[0] - input->cx) * input->coordincx;
  tmp[0].pos[1] = input->vertices[1] * input->coordincy;
  tmp[0].pos[2] = (float)input->priorityOffset / 2.0f;
  tmp[0].pos[3] = 1.0f;

  tmp[1].pos[0] = (input->vertices[2] - input->cx) * input->coordincx;
  tmp[1].pos[1] = input->vertices[3] * input->coordincy;
  tmp[1].pos[2] = (float)input->priorityOffset / 2.0f;
  tmp[1].pos[3] = 1.0f;

  tmp[2].pos[0] = (input->vertices[4] - input->cx) * input->coordincx;
  tmp[2].pos[1] = input->vertices[5] * input->coordincy;
  tmp[2].pos[2] = (float)input->priorityOffset / 2.0f;
  tmp[2].pos[3] = 1.0f;

  tmp[3].pos[0] = (input->vertices[6] - input->cx) * input->coordincx;
  tmp[3].pos[1] = input->vertices[7] * input->coordincy;
  tmp[3].pos[2] = (float)input->priorityOffset / 2.0f;
  tmp[3].pos[3] = 1.0f;


  if (output != NULL) {
    tm->allocate(output, input->cellw, input->cellh, &x, &y);
  }
  else {
    x = c->x;
    y = c->y;
  }

  if (input->flipfunction & 0x1) {
    tmp[0].texCoord[0] = tmp[3].texCoord[0] = (float)((x + input->cellw) - ATLAS_BIAS);
    tmp[1].texCoord[0] = tmp[2].texCoord[0] = (float)((x)+ATLAS_BIAS);
  }
  else {
    tmp[0].texCoord[0] = tmp[3].texCoord[0] = (float)((x)+ATLAS_BIAS);
    tmp[1].texCoord[0] = tmp[2].texCoord[0] = (float)((x + input->cellw) - ATLAS_BIAS);
  }
  if (input->flipfunction & 0x2) {
    tmp[0].texCoord[1] = tmp[1].texCoord[1] = (float)((y + input->cellh - input->cy) - ATLAS_BIAS);
    tmp[2].texCoord[1] = tmp[3].texCoord[1] = (float)((y - input->cy) + ATLAS_BIAS);
  }
  else {
    tmp[0].texCoord[1] = tmp[1].texCoord[1] = (float)((y + input->cy) + ATLAS_BIAS);
    tmp[2].texCoord[1] = tmp[3].texCoord[1] = (float)((y + input->cy + input->cellh) - ATLAS_BIAS);
  }

  if (c != NULL && cash_flg == 1)
  {
    c->x = x;
    c->y = y;
  }

  if (input->dst == 1)
  {
    YglCalcTextureQ(input->vertices, q);

    tmp[0].texCoord[0] *= q[0];
    tmp[0].texCoord[1] *= q[0];
    tmp[1].texCoord[0] *= q[1];
    tmp[1].texCoord[1] *= q[1];
    tmp[2].texCoord[0] *= q[2];
    tmp[2].texCoord[1] *= q[2];
    tmp[3].texCoord[0] *= q[3];
    tmp[3].texCoord[1] *= q[3];

    tmp[0].texCoord[2] = 0.0f;
    tmp[0].texCoord[3] = q[0];
    tmp[1].texCoord[2] = 0.0f;
    tmp[1].texCoord[3] = q[1];
    tmp[2].texCoord[2] = 0.0f;
    tmp[2].texCoord[3] = q[2];
    tmp[3].texCoord[2] = 0.0f;
    tmp[3].texCoord[3] = q[3];

  }
  else {
    tmp[0].texCoord[2] = 0.0f;
    tmp[0].texCoord[3] = 1.0f;
    tmp[1].texCoord[2] = 0.0f;
    tmp[1].texCoord[3] = 1.0f;
    tmp[2].texCoord[2] = 0.0f;
    tmp[2].texCoord[3] = 1.0f;
    tmp[3].texCoord[2] = 0.0f;
    tmp[3].texCoord[3] = 1.0f;
  }

  program->color_offset_val[0] = (float)(input->cor) / 255.0f;
  program->color_offset_val[1] = (float)(input->cog) / 255.0f;
  program->color_offset_val[2] = (float)(input->cob) / 255.0f;
  program->color_offset_val[3] = 0;

  int lastVertex = program->vertices.size();

  program->indices.push_back(lastVertex + 0);
  program->indices.push_back(lastVertex + 1);
  program->indices.push_back(lastVertex + 2);
  program->indices.push_back(lastVertex + 2);
  program->indices.push_back(lastVertex + 3);
  program->indices.push_back(lastVertex + 0);

  program->vertices.push_back(tmp[0]);
  program->vertices.push_back(tmp[1]);
  program->vertices.push_back(tmp[2]);
  program->vertices.push_back(tmp[3]);

  return 0;
}

void VIDVulkan::genPolygonBack() {

  VdpPipeline *program;
  program = (VdpPipeline*)backPiepline;

  program->blendmode = (fixVdp2Regs->BKTAU & 0x8000);


  Vertex tmp[4] = {};
  tmp[0].pos[0] = 0;
  tmp[0].pos[1] = 0;
  tmp[0].pos[2] = 0;
  tmp[0].pos[3] = 1.0f;

  tmp[1].pos[0] = vdp2width;
  tmp[1].pos[1] = 0;
  tmp[1].pos[2] = 0;
  tmp[0].pos[3] = 1.0f;

  tmp[2].pos[0] = vdp2width;
  tmp[2].pos[1] = vdp2height;
  tmp[2].pos[2] = 0;
  tmp[0].pos[3] = 1.0f;

  tmp[3].pos[0] = 0;
  tmp[3].pos[1] = vdp2height;
  tmp[3].pos[2] = 0;
  tmp[0].pos[3] = 1.0f;

  program->indices.resize(6);
  program->indices[0] = 0;
  program->indices[1] = 1;
  program->indices[2] = 2;
  program->indices[3] = 2;
  program->indices[4] = 3;
  program->indices[5] = 0;

  program->vertices.resize(4);
  program->vertices[0] = tmp[0];
  program->vertices[1] = tmp[1];
  program->vertices[2] = tmp[2];
  program->vertices[3] = tmp[3];

  return;

}

int VIDVulkan::genPolygonRbg0(
  VdpPipeline ** pipleLine,
  vdp2draw_struct * input,
  CharTexture * output,
  TextureCache * c,
  TextureCache * line,
  int rbg_type) {
  unsigned int x, y;
  VdpPipeline *program;
  //texturecoordinate_struct *tmp;
  float * vtxa;
  float q[4];
  YglPipelineId prg = PG_NORMAL;
  float * pos;
  //float * vtxa;

  const int CCRTMD = ((fixVdp2Regs->CCCTL >> 9) & 0x01); // hard/vdp2/hon/p12_14.htm#CCRTMD_

  if (input->colornumber >= 3) {
    prg = PG_NORMAL;
    if (input->mosaicxmask != 1 || input->mosaicymask != 1) {
      prg = PG_VDP2_MOSAIC;
    }
    if ((input->blendmode & VDP2_CC_BLUR) != 0) {
      prg = PG_VDP2_BLUR;
    }

    if ((input->blendmode & 0x03) == VDP2_CC_NONE) {
      prg = PG_VDP2_NOBLEND;
    }
    else if ((input->blendmode & 0x03) == VDP2_CC_ADD) {
      prg = PG_VDP2_ADDBLEND;
    }
    else {
      if (CCRTMD == 0) {
        prg = PG_NORMAL; // src alpha
      }
      else {
        prg = PG_NORMAL_DSTALPHA; // dst alpha
      }
    }

    if (input->lineTexture != 0) { // Line color blend screen blend is done in 2nd path
      prg = PG_VDP2_NOBLEND;
    }

    if (input->linescreen == 1) {
      prg = PG_LINECOLOR_INSERT;
      if (((Vdp2Regs->CCCTL >> 9) & 0x01)) {
        prg = PG_LINECOLOR_INSERT_DESTALPHA;
      }
    }
    //else if (input->linescreen == 2) { // per line operation by HBLANK
    //  prg = PG_VDP2_PER_LINE_ALPHA;
    //}
  }
  else {

    if (line->x != -1 && VDP2_CC_NONE != input->blendmode) {
      prg = PG_VDP2_RBG_CRAM_LINE;
    }
    else if (input->mosaicxmask != 1 || input->mosaicymask != 1) {
      prg = PG_VDP2_MOSAIC_CRAM;
    }
    else if ((input->blendmode & VDP2_CC_BLUR) != 0) {
      prg = PG_VDP2_BLUR_CRAM;
    }
    else if (input->linescreen == 1) {
      prg = PG_LINECOLOR_INSERT_CRAM;
      if (((Vdp2Regs->CCCTL >> 9) & 0x01)) {
        prg = PG_LINECOLOR_INSERT_DESTALPHA_CRAM;
      }
    }
    else if (input->linescreen == 2) { // per line operation by HBLANK
      if (input->specialprimode == 2) {
        prg = PG_VDP2_NORMAL_CRAM_SPECIAL_PRIORITY_COLOROFFSET; // Assault Leynos 2
      }
      else {
        prg = PG_VDP2_NOBLEND_CRAM;
      }
    }
    else {
      //if (input->specialprimode == 2) {
      //  prg = PG_VDP2_NORMAL_CRAM_SPECIAL_PRIORITY;
      //}
      //else {
//        prg = PG_VDP2_NORMAL_CRAM;
      //}

      if ((input->blendmode & 0x03) == VDP2_CC_NONE) {
        prg = PG_VDP2_NOBLEND_CRAM;
      }
      else if ((input->blendmode & 0x03) == VDP2_CC_ADD) {
        prg = PG_VDP2_ADDCOLOR_CRAM;
      }
      else { // VDP2_CC_RATE
        if (CCRTMD == 0) {
          prg = PG_VDP2_NORMAL_CRAM; // src alpha
        }
        else {
          prg = PG_VDP2_NORMAL_CRAM_DSTALPHA; // dst alpha
        }
      }

      if (input->lineTexture != 0) { // Line color blend screen blend is done in 2nd path
        prg = PG_VDP2_NOBLEND_CRAM;
      }


    }
  }

  if (*pipleLine == NULL) {
    *pipleLine = pipleLineFactory->getPipeline(prg, this, this->tm, this->vm);
  }
  else if ((*pipleLine)->prgid != prg) {
    pipleLineFactory->garbage(*pipleLine);
    *pipleLine = pipleLineFactory->getPipeline(prg, this, this->tm, this->vm);
  }

  program = *pipleLine;
  program->priority = input->priority;

  program->colornumber = input->colornumber;
  program->bwin0 = input->bEnWin0;
  program->logwin0 = input->WindowArea0;
  program->bwin1 = input->bEnWin1;
  program->logwin1 = input->WindowArea1;
  program->winmode = input->LogicWin;
  program->lineTexture = (VkImageView)input->lineTexture;
  program->blendmode = input->blendmode;
  program->specialcolormode = input->specialcolormode;
  program->specialPriority = input->specialprimode;

  program->mosaic[0] = input->mosaicxmask;
  program->mosaic[1] = input->mosaicymask;

  program->color_offset_val[0] = (float)(input->cor) / 255.0f;
  program->color_offset_val[1] = (float)(input->cog) / 255.0f;
  program->color_offset_val[2] = (float)(input->cob) / 255.0f;
  program->color_offset_val[3] = 0;

  Vertex tmp[4];
  tmp[0].pos[0] = input->vertices[0];
  tmp[0].pos[1] = input->vertices[1];
  tmp[0].pos[2] = 0;
  tmp[0].pos[3] = 1.0f;

  tmp[1].pos[0] = input->vertices[2];
  tmp[1].pos[1] = input->vertices[3];
  tmp[1].pos[2] = 0;
  tmp[0].pos[3] = 1.0f;

  tmp[2].pos[0] = input->vertices[4];
  tmp[2].pos[1] = input->vertices[5];
  tmp[2].pos[2] = 0;
  tmp[0].pos[3] = 1.0f;

  tmp[3].pos[0] = input->vertices[6];
  tmp[3].pos[1] = input->vertices[7];
  tmp[3].pos[2] = 0;
  tmp[0].pos[3] = 1.0f;

  if (rbg_use_compute_shader) {

    if (rbg_type == 0)
      program->interuput_texture = rbgGenerator->getTexture(0);
    else
      program->interuput_texture = rbgGenerator->getTexture(1);

    tmp[0].texCoord[0] = tmp[3].texCoord[0] = 0;
    tmp[1].texCoord[0] = tmp[2].texCoord[0] = input->cellw;
    tmp[0].texCoord[1] = tmp[1].texCoord[1] = 0;
    tmp[2].texCoord[1] = tmp[3].texCoord[1] = input->cellh;

  }
  else {


    x = c->x;
    y = c->y;

    tmp[0].texCoord[0] = tmp[3].texCoord[0] = (float)((x)+ATLAS_BIAS);
    tmp[1].texCoord[0] = tmp[2].texCoord[0] = (float)((x + input->cellw) - ATLAS_BIAS);
    tmp[0].texCoord[1] = tmp[1].texCoord[1] = (float)((y)+ATLAS_BIAS);
    tmp[2].texCoord[1] = tmp[3].texCoord[1] = (float)((y + input->cellh) - ATLAS_BIAS);
  }


  if (line == NULL) {
    tmp[0].texCoord[2] = tmp[1].texCoord[2] = tmp[2].texCoord[2] = tmp[3].texCoord[2] = 0;
    tmp[0].texCoord[3] = tmp[1].texCoord[3] = tmp[2].texCoord[3] = tmp[3].texCoord[3] = 0;
  }
  else {
    tmp[0].texCoord[2] = (float)(line->x) + ATLAS_BIAS;
    tmp[0].texCoord[3] = (float)(line->y) + ATLAS_BIAS;

    tmp[1].texCoord[2] = (float)(line->x) + ATLAS_BIAS;
    tmp[1].texCoord[3] = (float)(line->y + 1) - ATLAS_BIAS;

    tmp[2].texCoord[2] = (float)(line->x + input->cellh) - ATLAS_BIAS;
    tmp[2].texCoord[3] = (float)(line->y + 1) - ATLAS_BIAS;

    tmp[3].texCoord[2] = (float)(line->x + input->cellh) - ATLAS_BIAS;
    tmp[3].texCoord[3] = (float)(line->y) + ATLAS_BIAS;
  }


  int lastVertex = program->vertices.size();

  program->indices.push_back(lastVertex + 0);
  program->indices.push_back(lastVertex + 1);
  program->indices.push_back(lastVertex + 2);
  program->indices.push_back(lastVertex + 2);
  program->indices.push_back(lastVertex + 3);
  program->indices.push_back(lastVertex + 0);

  program->vertices.push_back(tmp[0]);
  program->vertices.push_back(tmp[1]);
  program->vertices.push_back(tmp[2]);
  program->vertices.push_back(tmp[3]);

  return 0;
}

void VIDVulkan::drawNBG0() {
  vdp2draw_struct info = { 0 };
  CharTexture texture;
  TextureCache tmpc;
  int char_access = 0;
  int ptn_access = 0;
  info.dst = 0;
  info.uclipmode = 0;
  info.id = 0;
  info.coordincx = 1.0f;
  info.coordincy = 1.0f;
  info.cor = 0;
  info.cog = 0;
  info.cob = 0;
  info.enable = 0;
  info.cellh = 256;
  info.specialcolorfunction = 0;

  info.pipeline = (void*)(&pipleLineNBG0);
  if (pipleLineNBG0) {
    pipleLineNBG0->vertices.clear();
    pipleLineNBG0->indices.clear();
  }

  for (int i = 0; i < 4; i++) {
    info.char_bank[i] = 0;
    info.pname_bank[i] = 0;
    for (int j = 0; j < 8; j++) {
      if (Vdp2External.AC_VRAM[i][j] == 0x04) {
        info.char_bank[i] = 1;
        char_access |= 1 << j;
      }
      if (Vdp2External.AC_VRAM[i][j] == 0x00) {
        info.pname_bank[i] = 1;
        ptn_access |= 1 << j;
      }
    }
  }

  if (fixVdp2Regs->BGON & 0x20)
  {
    // RBG1 mode
    info.enable = fixVdp2Regs->BGON & 0x20;
    if (!info.enable) return;

    // Read in Parameter B
    Vdp2ReadRotationTable(1, &paraB, fixVdp2Regs, Vdp2Ram);

    if ((info.isbitmap = fixVdp2Regs->CHCTLA & 0x2) != 0)
    {
      // Bitmap Mode

      ReadBitmapSize(&info, fixVdp2Regs->CHCTLA >> 2, 0x3);

      info.charaddr = (fixVdp2Regs->MPOFR & 0x70) * 0x2000;
      info.paladdr = (fixVdp2Regs->BMPNA & 0x7) << 4;
      info.flipfunction = 0;
      info.specialfunction = 0;
    }
    else
    {
      // Tile Mode
      info.mapwh = 4;
      ReadPlaneSize(&info, fixVdp2Regs->PLSZ >> 12);
      ReadPatternData(&info, fixVdp2Regs->PNCN0, fixVdp2Regs->CHCTLA & 0x1);

      paraB.ShiftPaneX = 8 + info.planew;
      paraB.ShiftPaneY = 8 + info.planeh;
      paraB.MskH = (8 * 64 * info.planew) - 1;
      paraB.MskV = (8 * 64 * info.planeh) - 1;
      paraB.MaxH = 8 * 64 * info.planew * 4;
      paraB.MaxV = 8 * 64 * info.planeh * 4;

    }
    info.rotatenum = 1;
    //paraB.PlaneAddr = (void FASTCALL(*)(void *, int, Vdp2*))&Vdp2ParameterBPlaneAddr;
    paraB.coefenab = fixVdp2Regs->KTCTL & 0x100;
    paraB.charaddr = (fixVdp2Regs->MPOFR & 0x70) * 0x2000;
    ReadPlaneSizeR(&paraB, fixVdp2Regs->PLSZ >> 12);
    for (int i = 0; i < 16; i++)
    {
      Vdp2ParameterBPlaneAddr(&info, i, fixVdp2Regs);
      paraB.PlaneAddrv[i] = info.addr;
    }

    info.LineColorBase = 0x00;
    if (paraB.coefenab)
      info.GetRParam = (Vdp2GetRParam_func)vdp2RGetParamMode01WithK;
    else
      info.GetRParam = (Vdp2GetRParam_func)vdp2RGetParamMode01NoK;

    if (paraB.coefdatasize == 2)
    {
      if (paraB.coefmode < 3)
      {
        info.GetKValueB = vdp2rGetKValue1W;
      }
      else {
        info.GetKValueB = vdp2rGetKValue1Wm3;
      }

    }
    else {
      if (paraB.coefmode < 3)
      {
        info.GetKValueB = vdp2rGetKValue2W;
      }
      else {
        info.GetKValueB = vdp2rGetKValue2Wm3;
      }
    }
  }
  else if ((fixVdp2Regs->BGON & 0x1) || info.enable)
  {
    // NBG0 mode
    info.enable = fixVdp2Regs->BGON & 0x1;
    if (!info.enable) return;

    if ((info.isbitmap = fixVdp2Regs->CHCTLA & 0x2) != 0)
    {
      // Bitmap Mode

      ReadBitmapSize(&info, fixVdp2Regs->CHCTLA >> 2, 0x3);

      info.x = -((fixVdp2Regs->SCXIN0 & 0x7FF) % info.cellw);
      info.y = -((fixVdp2Regs->SCYIN0 & 0x7FF) % info.cellh);

      info.charaddr = (fixVdp2Regs->MPOFN & 0x7) * 0x20000;
      info.paladdr = (fixVdp2Regs->BMPNA & 0x7) << 4;
      info.flipfunction = 0;
      info.specialcolorfunction = (fixVdp2Regs->BMPNA & 0x10) >> 4;
      info.specialfunction = 0;
    }
    else
    {
      // Tile Mode
      info.mapwh = 2;

      ReadPlaneSize(&info, fixVdp2Regs->PLSZ);

      info.x = -((fixVdp2Regs->SCXIN0 & 0x7FF) % (512 * info.planew));
      info.y = -((fixVdp2Regs->SCYIN0 & 0x7FF) % (512 * info.planeh));
      ReadPatternData(&info, fixVdp2Regs->PNCN0, fixVdp2Regs->CHCTLA & 0x1);
    }

    if ((fixVdp2Regs->ZMXN0.all & 0x7FF00) == 0)
      info.coordincx = 1.0f;
    else
      info.coordincx = (float)65536 / (fixVdp2Regs->ZMXN0.all & 0x7FF00);

    switch (fixVdp2Regs->ZMCTL & 0x03)
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

    if ((fixVdp2Regs->ZMYN0.all & 0x7FF00) == 0)
      info.coordincy = 1.0f;
    else
      info.coordincy = (float)65536 / (fixVdp2Regs->ZMYN0.all & 0x7FF00);

    info.PlaneAddr = (void FASTCALL(*)(void *, int, Vdp2*))&Vdp2NBG0PlaneAddr;
  }
  else {
    return;
  }



  ReadMosaicData(&info, 0x1, fixVdp2Regs);

  info.transparencyenable = !(fixVdp2Regs->BGON & 0x100);
  info.specialprimode = fixVdp2Regs->SFPRMD & 0x3;
  info.specialcolormode = fixVdp2Regs->SFCCMD & 0x3;
  if (fixVdp2Regs->SFSEL & 0x1)
    info.specialcode = fixVdp2Regs->SFCODE >> 8;
  else
    info.specialcode = fixVdp2Regs->SFCODE & 0xFF;

  info.colornumber = (fixVdp2Regs->CHCTLA & 0x70) >> 4;
  int dest_alpha = ((fixVdp2Regs->CCCTL >> 9) & 0x01);

  info.blendmode = 0;

  // 12.13 blur
  if ((fixVdp2Regs->CCCTL & 0xF000) == 0xA000) {
    info.alpha = ((~fixVdp2Regs->CCRNA & 0x1F) << 3) + 0x7;
    info.blendmode |= VDP2_CC_BLUR;
  }


  // Enable Color Calculation
  if (fixVdp2Regs->CCCTL & 0x1)
  {
    // Color calculation ratio
    info.alpha = ((~fixVdp2Regs->CCRNA & 0x1F) << 3) + 0x7;

    // Color calculation mode bit
    if (fixVdp2Regs->CCCTL & 0x100) { // Add Color
      info.blendmode |= VDP2_CC_ADD;
      info.alpha = 0xFF;
    }
    else { // Use Color calculation ratio
      if (info.specialcolormode != 0 && dest_alpha) { // Just currently not supported
        info.blendmode |= VDP2_CC_NONE;
      }
      else {
        info.blendmode |= VDP2_CC_RATE;
      }
    }
    // Disable Color Calculation
  }
  else {

    // Use Destination Alpha 12.14 CCRTMD
    if (dest_alpha) {

      // Color calculation will not be operated.
      // But need to write alpha value
      info.alpha = ((~fixVdp2Regs->CCRNA & 0x1F) << 3) + 0x7;
    }
    else {
      info.alpha = 0xFF;
    }
    info.blendmode |= VDP2_CC_NONE;
  }


  generatePerLineColorCalcuration(&info, NBG0);
  info.linescreen = 0;
  if (fixVdp2Regs->LNCLEN & 0x1)
    info.linescreen = 1;

  info.coloroffset = (fixVdp2Regs->CRAOFA & 0x7) << 8;
  readVdp2ColorOffset(fixVdp2Regs, &info, 0x1);
  if (info.lineTexture != 0) {
    info.cor = 0;
    info.cog = 0;
    info.cob = 0;
    info.linescreen = 2;
  }
  info.linecheck_mask = 0x01;
  info.priority = fixVdp2Regs->PRINA & 0x7;

  if (!(info.enable & Vdp2External.disptoggle) || (info.priority == 0))
    return;

  // Window Mode
  info.bEnWin0 = (fixVdp2Regs->WCTLA >> 1) & 0x01;
  info.WindowArea0 = (fixVdp2Regs->WCTLA >> 0) & 0x01;
  info.bEnWin1 = (fixVdp2Regs->WCTLA >> 3) & 0x01;
  info.WindowArea1 = (fixVdp2Regs->WCTLA >> 2) & 0x01;
  info.LogicWin = (fixVdp2Regs->WCTLA >> 7) & 0x01;


  ReadLineScrollData(&info, fixVdp2Regs->SCRCTL & 0xFF, fixVdp2Regs->LSTA0.all);
  info.lineinfo = lineNBG0;
  genLineinfo(&info);
  Vdp2SetGetColor(&info);

  if (fixVdp2Regs->SCRCTL & 1)
  {
    info.isverticalscroll = 1;
    info.verticalscrolltbl = (fixVdp2Regs->VCSTA.all & 0x7FFFE) << 1;
    if (fixVdp2Regs->SCRCTL & 0x100)
      info.verticalscrollinc = 8;
    else
      info.verticalscrollinc = 4;
  }
  else
    info.isverticalscroll = 0;

  if (info.enable == 1)
  {
    // NBG0 draw
    if (info.isbitmap)
    {
      if (info.coordincx != 1.0f || info.coordincy != 1.0f || VDPLINE_SZ(info.islinescroll)) {
        info.sh = (fixVdp2Regs->SCXIN0 & 0x7FF);
        info.sv = (fixVdp2Regs->SCYIN0 & 0x7FF);
        info.x = 0;
        info.y = 0;
        info.vertices[0] = 0;
        info.vertices[1] = 0;
        info.vertices[2] = vdp2width;
        info.vertices[3] = 0;
        info.vertices[4] = vdp2width;
        info.vertices[5] = vdp2height;
        info.vertices[6] = 0;
        info.vertices[7] = vdp2height;
        vdp2draw_struct infotmp = info;
        infotmp.cellw = vdp2width;
        if (vdp2height >= 448)
          infotmp.cellh = (vdp2height >> 1);
        else
          infotmp.cellh = vdp2height;
        infotmp.cx = 0;
        infotmp.cy = 0;
        infotmp.coordincx = 1.0;
        infotmp.coordincy = 1.0;
        genPolygon(&pipleLineNBG0, &infotmp, &texture, NULL, &tmpc, 0);
        drawBitmapCoordinateInc(&info, &texture);
      }
      else {

        int xx, yy;
        int isCached = 0;

        if (info.islinescroll) // Nights Movie
        {
          info.sh = (fixVdp2Regs->SCXIN0 & 0x7FF);
          info.sv = (fixVdp2Regs->SCYIN0 & 0x7FF);
          info.x = 0;
          info.y = 0;
          info.vertices[0] = 0;
          info.vertices[1] = 0;
          info.vertices[2] = vdp2width;
          info.vertices[3] = 0;
          info.vertices[4] = vdp2width;
          info.vertices[5] = vdp2height;
          info.vertices[6] = 0;
          info.vertices[7] = vdp2height;
          vdp2draw_struct infotmp = info;
          infotmp.cellw = vdp2width;
          infotmp.cellh = vdp2height;
          infotmp.cx = 0;
          infotmp.cy = 0;
          infotmp.coordincx = 1.0;
          infotmp.coordincy = 1.0;
          genPolygon(&pipleLineNBG0, &infotmp, &texture, NULL, NULL, 0);
          drawBitmapLineScroll(&info, &texture);
        }
        else {
          yy = info.y;
          while (yy + info.y < vdp2height)
          {
            xx = info.x;
            while (xx + info.x < vdp2width)
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
                genPolygon(&pipleLineNBG0, &info, &texture, NULL, &tmpc, 1);
                if (info.islinescroll) {
                  drawBitmapLineScroll(&info, &texture);
                }
                else {
                  drawBitmap(&info, &texture);
                }
                isCached = 1;
              }
              else {
                genPolygon(&pipleLineNBG0, &info, NULL, NULL, &tmpc, 0);
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
        info.sh = (fixVdp2Regs->SCXIN0 & 0x7FF);
        info.sv = (fixVdp2Regs->SCYIN0 & 0x7FF);
        info.x = 0;
        info.y = 0;
        info.vertices[0] = 0;
        info.vertices[1] = 0;
        info.vertices[2] = vdp2width;
        info.vertices[3] = 0;
        info.vertices[4] = vdp2width;
        info.vertices[5] = vdp2height;
        info.vertices[6] = 0;
        info.vertices[7] = vdp2height;
        vdp2draw_struct infotmp = info;
        infotmp.cellw = vdp2width;
        infotmp.cellh = vdp2height;

        infotmp.flipfunction = 0;
        genPolygon(&pipleLineNBG0, &infotmp, &texture, NULL, &tmpc, 0);
        drawMapPerLine(&info, &texture);
      }
      else {
        info.x = fixVdp2Regs->SCXIN0 & 0x7FF;
        info.y = fixVdp2Regs->SCYIN0 & 0x7FF;
        drawMap(&info, &texture);
      }
    }
  }
  else
  {
    // RBG1 draw
    memcpy(&g_rgb1.info, &info, sizeof(info));
    drawRotation(&g_rgb1, &pipleLineNBG0);
  }

  if (pipleLineNBG0 != nullptr && pipleLineNBG0->vertices.size() != 0) {
    layers[info.priority].push_back(pipleLineNBG0);
  }


}

void VIDVulkan::drawNBG1() {
  vdp2draw_struct info = {};
  CharTexture texture;
  TextureCache tmpc;
  info.dst = 0;
  info.id = 1;
  info.uclipmode = 0;
  info.cor = 0;
  info.cog = 0;
  info.cob = 0;
  info.specialcolorfunction = 0;

  info.pipeline = (void*)(&pipleLineNBG1);
  if (pipleLineNBG1) {
    pipleLineNBG1->vertices.clear();
    pipleLineNBG1->indices.clear();
  }

  info.enable = fixVdp2Regs->BGON & 0x2;
  if (!info.enable) return;

  int char_access = 0;
  int ptn_access = 0;

  for (int i = 0; i < 4; i++) {
    info.char_bank[i] = 0;
    info.pname_bank[i] = 0;
    for (int j = 0; j < 8; j++) {
      if (Vdp2External.AC_VRAM[i][j] == 0x05) {
        info.char_bank[i] = 1;
        char_access |= (1 << j);
      }
      if (Vdp2External.AC_VRAM[i][j] == 0x01) {
        info.pname_bank[i] = 1;
        ptn_access |= (1 << j);
      }
    }
  }

  info.transparencyenable = !(fixVdp2Regs->BGON & 0x200);
  info.specialprimode = (fixVdp2Regs->SFPRMD >> 2) & 0x3;

  info.colornumber = (fixVdp2Regs->CHCTLA & 0x3000) >> 12;

  if ((info.isbitmap = fixVdp2Regs->CHCTLA & 0x200) != 0)
  {
    ReadBitmapSize(&info, fixVdp2Regs->CHCTLA >> 10, 0x3);

    info.x = -((fixVdp2Regs->SCXIN1 & 0x7FF) % info.cellw);
    info.y = -((fixVdp2Regs->SCYIN1 & 0x7FF) % info.cellh);
    info.charaddr = ((fixVdp2Regs->MPOFN & 0x70) >> 4) * 0x20000;
    info.paladdr = (fixVdp2Regs->BMPNA & 0x700) >> 4;
    info.flipfunction = 0;
    info.specialfunction = 0;
    info.specialcolorfunction = (fixVdp2Regs->BMPNA & 0x1000) >> 4;
  }
  else
  {
    info.mapwh = 2;

    ReadPlaneSize(&info, fixVdp2Regs->PLSZ >> 2);

    info.x = -((fixVdp2Regs->SCXIN1 & 0x7FF) % (512 * info.planew));
    info.y = -((fixVdp2Regs->SCYIN1 & 0x7FF) % (512 * info.planeh));

    ReadPatternData(&info, fixVdp2Regs->PNCN1, fixVdp2Regs->CHCTLA & 0x100);
  }

  info.specialcolormode = (fixVdp2Regs->SFCCMD >> 2) & 0x3;
  if (fixVdp2Regs->SFSEL & 0x2)
    info.specialcode = fixVdp2Regs->SFCODE >> 8;
  else
    info.specialcode = fixVdp2Regs->SFCODE & 0xFF;

  ReadMosaicData(&info, 0x2, fixVdp2Regs);


  info.blendmode = 0;
  // 12.13 blur
  if ((fixVdp2Regs->CCCTL & 0xF000) == 0xC000) {
    info.alpha = ((~fixVdp2Regs->CCRNA & 0x1F00) >> 5) + 0x7;
    info.blendmode |= VDP2_CC_BLUR;
  }

  if (fixVdp2Regs->CCCTL & 0x2) {
    info.alpha = ((~fixVdp2Regs->CCRNA & 0x1F00) >> 5) + 0x7;
    if (fixVdp2Regs->CCCTL & 0x100) {
      info.blendmode |= VDP2_CC_ADD;
    }
    else {
      info.blendmode |= VDP2_CC_RATE;
    }
  }
  else {
    // 12.14 CCRTMD
    if (((fixVdp2Regs->CCCTL >> 9) & 0x01) == 0x01) {
      info.alpha = ((~fixVdp2Regs->CCRNA & 0x1F00) >> 5) + 0x7;
    }
    else {
      info.alpha = 0xFF;
    }
    info.blendmode |= VDP2_CC_NONE;
  }

  generatePerLineColorCalcuration(&info, NBG1);
  info.linescreen = 0;
  if (fixVdp2Regs->LNCLEN & 0x2)
    info.linescreen = 1;

  info.coloroffset = (fixVdp2Regs->CRAOFA & 0x70) << 4;
  readVdp2ColorOffset(fixVdp2Regs, &info, 0x2);
  if (info.lineTexture != 0) {
    info.cor = 0;
    info.cog = 0;
    info.cob = 0;
    info.linescreen = 2;
  }

  info.linecheck_mask = 0x02;

  if ((fixVdp2Regs->ZMXN1.all & 0x7FF00) == 0)
    info.coordincx = 1.0f;
  else
    info.coordincx = (float)65536 / (fixVdp2Regs->ZMXN1.all & 0x7FF00);

  switch ((fixVdp2Regs->ZMCTL >> 8) & 0x03)
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

  if ((fixVdp2Regs->ZMYN1.all & 0x7FF00) == 0)
    info.coordincy = 1.0f;
  else
    info.coordincy = (float)65536 / (fixVdp2Regs->ZMYN1.all & 0x7FF00);


  info.priority = (fixVdp2Regs->PRINA >> 8) & 0x7;;
  info.PlaneAddr = (void FASTCALL(*)(void *, int, Vdp2*))&Vdp2NBG1PlaneAddr;

  if (!(info.enable & Vdp2External.disptoggle) || (info.priority == 0) ||
    (fixVdp2Regs->BGON & 0x1 && (fixVdp2Regs->CHCTLA & 0x70) >> 4 == 4)) // If NBG0 16M mode is enabled, don't draw
    return;

  // Window Mode
  info.bEnWin0 = (fixVdp2Regs->WCTLA >> 9) & 0x01;
  info.WindowArea0 = (fixVdp2Regs->WCTLA >> 8) & 0x01;
  info.bEnWin1 = (fixVdp2Regs->WCTLA >> 11) & 0x01;
  info.WindowArea1 = (fixVdp2Regs->WCTLA >> 10) & 0x01;
  info.LogicWin = (fixVdp2Regs->WCTLA >> 15) & 0x01;


  ReadLineScrollData(&info, fixVdp2Regs->SCRCTL >> 8, fixVdp2Regs->LSTA1.all);
  info.lineinfo = lineNBG1;
  genLineinfo(&info);
  Vdp2SetGetColor(&info);

  if (fixVdp2Regs->SCRCTL & 0x100)
  {
    info.isverticalscroll = 1;
    if (fixVdp2Regs->SCRCTL & 0x1)
    {
      info.verticalscrolltbl = 4 + ((fixVdp2Regs->VCSTA.all & 0x7FFFE) << 1);
      info.verticalscrollinc = 8;
    }
    else
    {
      info.verticalscrolltbl = (fixVdp2Regs->VCSTA.all & 0x7FFFE) << 1;
      info.verticalscrollinc = 4;
    }
  }
  else
    info.isverticalscroll = 0;


  if (info.isbitmap)
  {

    if (info.coordincx != 1.0f || info.coordincy != 1.0f) {
      info.sh = (fixVdp2Regs->SCXIN1 & 0x7FF);
      info.sv = (fixVdp2Regs->SCYIN1 & 0x7FF);
      info.x = 0;
      info.y = 0;
      info.vertices[0] = 0;
      info.vertices[1] = 0;
      info.vertices[2] = vdp2width;
      info.vertices[3] = 0;
      info.vertices[4] = vdp2width;
      info.vertices[5] = vdp2height;
      info.vertices[6] = 0;
      info.vertices[7] = vdp2height;
      vdp2draw_struct infotmp = info;
      infotmp.cellw = vdp2width;
      if (vdp2height >= 448)
        infotmp.cellh = (vdp2height >> 1);
      else
        infotmp.cellh = vdp2height;
      infotmp.cx = 0;
      infotmp.cy = 0;
      infotmp.coordincx = 1.0;
      infotmp.coordincy = 1.0;
      genPolygon(&pipleLineNBG1, &infotmp, &texture, NULL, &tmpc, 0);
      drawBitmapCoordinateInc(&info, &texture);
    }
    else {

      int xx, yy;
      int isCached = 0;

      if (info.islinescroll) // Nights Movie
      {
        info.sh = (fixVdp2Regs->SCXIN1 & 0x7FF);
        info.sv = (fixVdp2Regs->SCYIN1 & 0x7FF);
        info.x = 0;
        info.y = 0;
        info.vertices[0] = 0;
        info.vertices[1] = 0;
        info.vertices[2] = vdp2width;
        info.vertices[3] = 0;
        info.vertices[4] = vdp2width;
        info.vertices[5] = vdp2height;
        info.vertices[6] = 0;
        info.vertices[7] = vdp2height;
        vdp2draw_struct infotmp = info;
        infotmp.cellw = vdp2width;
        infotmp.cellh = vdp2height;
        infotmp.flipfunction = 0;
        infotmp.cx = 0;
        infotmp.cy = 0;
        infotmp.coordincx = 1.0;
        infotmp.coordincy = 1.0;
        genPolygon(&pipleLineNBG1, &infotmp, &texture, NULL, &tmpc, 0);
        drawBitmapLineScroll(&info, &texture);


      }
      else {
        yy = info.y;
        while (yy + info.y < vdp2height)
        {
          xx = info.x;
          while (xx + info.x < vdp2width)
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
              genPolygon(&pipleLineNBG1, &info, &texture, NULL, &tmpc, 1);
              if (info.islinescroll) {
                drawBitmapLineScroll(&info, &texture);
              }
              else {
                drawBitmap(&info, &texture);
              }
              isCached = 1;
            }
            else {
              genPolygon(&pipleLineNBG1, &info, NULL, NULL, &tmpc, 0);
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
      info.sh = (fixVdp2Regs->SCXIN1 & 0x7FF);
      info.sv = (fixVdp2Regs->SCYIN1 & 0x7FF);
      info.x = 0;
      info.y = 0;
      info.vertices[0] = 0;
      info.vertices[1] = 0;
      info.vertices[2] = vdp2width;
      info.vertices[3] = 0;
      info.vertices[4] = vdp2width;
      info.vertices[5] = vdp2height;
      info.vertices[6] = 0;
      info.vertices[7] = vdp2height;
      vdp2draw_struct infotmp = info;
      infotmp.cellw = vdp2width;
      infotmp.cellh = vdp2height;
      infotmp.flipfunction = 0;
      genPolygon(&pipleLineNBG1, &infotmp, &texture, NULL, &tmpc, 0);
      drawMapPerLine(&info, &texture);
    }
    else {
      //Vdp2DrawMap(&info, &texture);
      info.x = fixVdp2Regs->SCXIN1 & 0x7FF;
      info.y = fixVdp2Regs->SCYIN1 & 0x7FF;
      drawMap(&info, &texture);
    }
  }

  if (pipleLineNBG1 != nullptr && pipleLineNBG1->vertices.size() != 0) {
    layers[info.priority].push_back(pipleLineNBG1);
  }

}

inline
int VIDVulkan::checkCharAccessPenalty(int char_access, int ptn_access) {
  if (vdp2width >= 640) {
    //if (char_access < ptn_access) {
    //  return -1;
    //}
    if (ptn_access & 0x01) { // T0
      // T0-T2
      if ((char_access & 0x07) != 0) {
        if (char_access < ptn_access) {
          return -1;
        }
        return 0;
      }
    }

    if (ptn_access & 0x02) { // T1
      // T1-T3
      if ((char_access & 0x0E) != 0) {
        if (char_access < ptn_access) {
          return -1;
        }
        return 0;
      }
    }

    if (ptn_access & 0x04) { // T2
      // T0,T2,T3
      if ((char_access & 0x0D) != 0) {
        if (char_access < ptn_access) {
          return -1;
        }
        return 0;
      }
    }

    if (ptn_access & 0x08) { // T3
      // T0,T1,T3
      if ((char_access & 0xB) != 0) {
        if (char_access < ptn_access) {
          return -1;
        }
        return 0;
      }
    }
    return -1;
  }
  else {

    if (ptn_access & 0x01) { // T0
      // T0-T2, T4-T7
      if ((char_access & 0xF7) != 0) {
        return 0;
      }
    }

    if (ptn_access & 0x02) { // T1
      // T0-T3, T5-T7
      if ((char_access & 0xEF) != 0) {
        return 0;
      }
    }

    if (ptn_access & 0x04) { // T2
      // T0-T3, T6-T7
      if ((char_access & 0xCF) != 0) {
        return 0;
      }
    }

    if (ptn_access & 0x08) { // T3
      // T0-T3, T7
      if ((char_access & 0x8F) != 0) {
        return 0;
      }
    }

    if (ptn_access & 0x10) { // T4
      // T0-T3
      if ((char_access & 0x0F) != 0) {
        return 0;
      }
    }

    if (ptn_access & 0x20) { // T5
      // T1-T3
      if ((char_access & 0x0E) != 0) {
        return 0;
      }
    }

    if (ptn_access & 0x40) { // T6
      // T2,T3
      if ((char_access & 0x0C) != 0) {
        return 0;
      }
    }

    if (ptn_access & 0x80) { // T7
      // T3
      if ((char_access & 0x08) != 0) {
        return 0;
      }
    }
    return -1;
  }
  return 0;
}



void VIDVulkan::drawNBG2() {
  vdp2draw_struct info = {};
  CharTexture texture;
  info.dst = 0;
  info.id = 2;
  info.uclipmode = 0;
  info.cor = 0;
  info.cog = 0;
  info.cob = 0;
  info.specialcolorfunction = 0;
  info.blendmode = 0;

  info.pipeline = (void*)(&pipleLineNBG2);
  if (pipleLineNBG2) {
    pipleLineNBG2->vertices.clear();
    pipleLineNBG2->indices.clear();
  }

  info.enable = fixVdp2Regs->BGON & 0x4;
  if (!info.enable) return;
  info.transparencyenable = !(fixVdp2Regs->BGON & 0x400);
  info.specialprimode = (fixVdp2Regs->SFPRMD >> 4) & 0x3;

  info.colornumber = (fixVdp2Regs->CHCTLB & 0x2) >> 1;
  info.mapwh = 2;

  ReadPlaneSize(&info, fixVdp2Regs->PLSZ >> 4);
  info.x = -((fixVdp2Regs->SCXN2 & 0x7FF) % (512 * info.planew));
  info.y = -((fixVdp2Regs->SCYN2 & 0x7FF) % (512 * info.planeh));
  ReadPatternData(&info, fixVdp2Regs->PNCN2, fixVdp2Regs->CHCTLB & 0x1);

  ReadMosaicData(&info, 0x4, fixVdp2Regs);

  info.specialcolormode = (fixVdp2Regs->SFCCMD >> 4) & 0x3;
  if (fixVdp2Regs->SFSEL & 0x4)
    info.specialcode = fixVdp2Regs->SFCODE >> 8;
  else
    info.specialcode = fixVdp2Regs->SFCODE & 0xFF;

  info.blendmode = 0;

  // 12.13 blur
  if ((fixVdp2Regs->CCCTL & 0xF000) == 0xD000) {
    info.alpha = ((~fixVdp2Regs->CCRNB & 0x1F) << 3) + 0x7;
    info.blendmode |= VDP2_CC_BLUR;
  }


  if (fixVdp2Regs->CCCTL & 0x4)
  {
    info.alpha = ((~fixVdp2Regs->CCRNB & 0x1F) << 3) + 0x7;
    if (fixVdp2Regs->CCCTL & 0x100 /*&& info.specialcolormode == 0*/)
    {
      info.blendmode |= VDP2_CC_ADD;
    }
    else {
      info.blendmode |= VDP2_CC_RATE;
    }
  }
  else {
    // 12.14 CCRTMD
    if (((fixVdp2Regs->CCCTL >> 9) & 0x01) == 0x01) {
      info.alpha = ((~fixVdp2Regs->CCRNB & 0x1F) << 3) + 0x7;
    }
    else {
      info.alpha = 0xFF;
    }
    info.blendmode |= VDP2_CC_NONE;
  }


  generatePerLineColorCalcuration(&info, NBG2);
  info.linescreen = 0;
  if (fixVdp2Regs->LNCLEN & 0x4)
    info.linescreen = 1;

  info.coloroffset = fixVdp2Regs->CRAOFA & 0x700;
  readVdp2ColorOffset(fixVdp2Regs, &info, 0x4);

  if (info.lineTexture != 0) {
    info.linescreen = 2;
    info.cor = 0;
    info.cog = 0;
    info.cob = 0;
  }


  info.linecheck_mask = 0x04;
  info.coordincx = info.coordincy = 1;
  info.priority = fixVdp2Regs->PRINB & 0x7;;
  info.PlaneAddr = (void FASTCALL(*)(void *, int, Vdp2*))&Vdp2NBG2PlaneAddr;

  if (/*!(info.enable & Vdp2External.disptoggle) ||*/ (info.priority == 0) ||
    (fixVdp2Regs->BGON & 0x1 && (fixVdp2Regs->CHCTLA & 0x70) >> 4 >= 2)) // If NBG0 2048/32786/16M mode is enabled, don't draw
    return;

  // Window Mode
  info.bEnWin0 = (fixVdp2Regs->WCTLB >> 1) & 0x01;
  info.WindowArea0 = (fixVdp2Regs->WCTLB >> 0) & 0x01;
  info.bEnWin1 = (fixVdp2Regs->WCTLB >> 3) & 0x01;
  info.WindowArea1 = (fixVdp2Regs->WCTLB >> 2) & 0x01;
  info.LogicWin = (fixVdp2Regs->WCTLB >> 7) & 0x01;

  //Vdp2SetGetColor(&info); ToDO

  info.islinescroll = 0;
  info.linescrolltbl = 0;
  info.lineinc = 0;
  info.isverticalscroll = 0;

  int xoffset = 0;
  {
    int char_access = 0;
    int ptn_access = 0;

    for (int i = 0; i < 4; i++) {
      info.char_bank[i] = 0;
      info.pname_bank[i] = 0;
      for (int j = 0; j < 8; j++) {
        if (Vdp2External.AC_VRAM[i][j] == 0x06) {
          info.char_bank[i] = 1;
          char_access |= (1 << j);
        }
        if (Vdp2External.AC_VRAM[i][j] == 0x02) {
          info.pname_bank[i] = 1;
          ptn_access |= (1 << j);
        }
      }
    }

    // Setting miss of cycle patten need to plus 8 dot vertical 
    if (checkCharAccessPenalty(char_access, ptn_access) != 0) {
      xoffset = -8;
    }
  }

  if ((*Vdp2External.perline_alpha & 0x100) != 0) {
    TextureCache tmpc;
    info.sh = 0;
    info.sv = 0;
    info.x = 0;
    info.y = 0;
    info.vertices[0] = 0;
    info.vertices[1] = 0;
    info.vertices[2] = vdp2width;
    info.vertices[3] = 0;
    info.vertices[4] = vdp2width;
    info.vertices[5] = vdp2height;
    info.vertices[6] = 0;
    info.vertices[7] = vdp2height;
    vdp2draw_struct infotmp = info;
    infotmp.cellw = vdp2width;
    infotmp.cellh = vdp2height;
    infotmp.flipfunction = 0;
    genPolygon(&pipleLineNBG2, &infotmp, &texture, NULL, &tmpc, 0);
    drawMapPerLineNbg23(&info, &texture, 2, xoffset);
  }
  else {
    info.x = (fixVdp2Regs->SCXN2 & 0x7FF) + xoffset;
    info.y = fixVdp2Regs->SCYN2 & 0x7FF;
    drawMap(&info, &texture);
  }


  if (pipleLineNBG2 != nullptr && pipleLineNBG2->vertices.size() != 0) {
    layers[info.priority].push_back(pipleLineNBG2);
  }

}

void VIDVulkan::drawNBG3() {
  vdp2draw_struct info = {};
  CharTexture texture;
  info.id = 3;
  info.dst = 0;
  info.uclipmode = 0;
  info.cor = 0;
  info.cog = 0;
  info.cob = 0;
  info.specialcolorfunction = 0;
  info.blendmode = 0;

  info.pipeline = (void*)(&pipleLineNBG3);
  if (pipleLineNBG3) {
    pipleLineNBG3->vertices.clear();
    pipleLineNBG3->indices.clear();
  }

  info.enable = fixVdp2Regs->BGON & 0x8;
  if (!info.enable) return;
  info.transparencyenable = !(fixVdp2Regs->BGON & 0x800);
  info.specialprimode = (fixVdp2Regs->SFPRMD >> 6) & 0x3;

  info.colornumber = (fixVdp2Regs->CHCTLB & 0x20) >> 5;

  info.mapwh = 2;

  ReadPlaneSize(&info, fixVdp2Regs->PLSZ >> 6);
  info.x = -((fixVdp2Regs->SCXN3 & 0x7FF) % (512 * info.planew));
  info.y = -((fixVdp2Regs->SCYN3 & 0x7FF) % (512 * info.planeh));
  ReadPatternData(&info, fixVdp2Regs->PNCN3, fixVdp2Regs->CHCTLB & 0x10);

  ReadMosaicData(&info, 0x8, fixVdp2Regs);

  info.specialcolormode = (fixVdp2Regs->SFCCMD >> 6) & 0x03;
  if (fixVdp2Regs->SFSEL & 0x8)
    info.specialcode = fixVdp2Regs->SFCODE >> 8;
  else
    info.specialcode = fixVdp2Regs->SFCODE & 0xFF;

  // 12.13 blur
  if ((fixVdp2Regs->CCCTL & 0xF000) == 0xE000) {
    info.alpha = ((~fixVdp2Regs->CCRNB & 0x1F00) >> 5) + 0x7;
    info.blendmode |= VDP2_CC_BLUR;
  }

  if (fixVdp2Regs->CCCTL & 0x8)
  {
    info.alpha = ((~fixVdp2Regs->CCRNB & 0x1F00) >> 5) + 0x7;
    if (fixVdp2Regs->CCCTL & 0x100)
    {
      info.blendmode |= VDP2_CC_ADD;
    }
    else {
      info.blendmode |= VDP2_CC_RATE;
    }
  }
  else {
    // 12.14 CCRTMD
    if (((fixVdp2Regs->CCCTL >> 9) & 0x01) == 0x01) {
      info.alpha = ((~fixVdp2Regs->CCRNB & 0x1F00) >> 5) + 0x7;
    }
    else {
      info.alpha = 0xFF;
    }

    info.blendmode |= VDP2_CC_NONE;
  }


  generatePerLineColorCalcuration(&info, NBG3);
  info.linescreen = 0;
  if (fixVdp2Regs->LNCLEN & 0x8)
    info.linescreen = 1;


  info.coloroffset = (fixVdp2Regs->CRAOFA & 0x7000) >> 4;
  readVdp2ColorOffset(fixVdp2Regs, &info, 0x8);
  if (info.lineTexture != 0) {
    info.cor = 0;
    info.cog = 0;
    info.cob = 0;
    info.linescreen = 2;
  }

  info.linecheck_mask = 0x08;
  info.coordincx = info.coordincy = 1;

  info.priority = (fixVdp2Regs->PRINB >> 8) & 0x7;
  info.PlaneAddr = (void FASTCALL(*)(void *, int, Vdp2*))&Vdp2NBG3PlaneAddr;

  if (!(info.enable & Vdp2External.disptoggle) || (info.priority == 0) ||
    (fixVdp2Regs->BGON & 0x1 && (fixVdp2Regs->CHCTLA & 0x70) >> 4 == 4) || // If NBG0 16M mode is enabled, don't draw
    (fixVdp2Regs->BGON & 0x2 && (fixVdp2Regs->CHCTLA & 0x3000) >> 12 >= 2)) // If NBG1 2048/32786 is enabled, don't draw
    return;

  // Window Mode
  info.bEnWin0 = (fixVdp2Regs->WCTLB >> 9) & 0x01;
  info.WindowArea0 = (fixVdp2Regs->WCTLB >> 8) & 0x01;
  info.bEnWin1 = (fixVdp2Regs->WCTLB >> 11) & 0x01;
  info.WindowArea1 = (fixVdp2Regs->WCTLB >> 10) & 0x01;
  info.LogicWin = (fixVdp2Regs->WCTLB >> 15) & 0x01;

  //Vdp2SetGetColor(&info); TODO

  int xoffset = 0;
  {
    int char_access = 0;
    int ptn_access = 0;
    for (int i = 0; i < 4; i++) {
      info.char_bank[i] = 0;
      info.pname_bank[i] = 0;
      for (int j = 0; j < 8; j++) {
        if (Vdp2External.AC_VRAM[i][j] == 0x07) {
          info.char_bank[i] = 1;
          char_access |= (1 << j);
        }
        if (Vdp2External.AC_VRAM[i][j] == 0x03) {
          info.pname_bank[i] = 1;
          ptn_access |= (1 << j);
        }
      }
    }
    // Setting miss of cycle patten need to plus 8 dot vertical
    if (checkCharAccessPenalty(char_access, ptn_access) != 0) {
      xoffset = -8;
    }
  }

  if ((*Vdp2External.perline_alpha & 0x80) != 0) {
    TextureCache tmpc;
    info.sh = 0;
    info.sv = 0;
    info.x = 0;
    info.y = 0;
    info.vertices[0] = 0;
    info.vertices[1] = 0;
    info.vertices[2] = vdp2width;
    info.vertices[3] = 0;
    info.vertices[4] = vdp2width;
    info.vertices[5] = vdp2height;
    info.vertices[6] = 0;
    info.vertices[7] = vdp2height;
    vdp2draw_struct infotmp = info;
    infotmp.cellw = vdp2width;
    infotmp.cellh = vdp2height;
    infotmp.flipfunction = 0;
    genPolygon(&pipleLineNBG3, &infotmp, &texture, NULL, &tmpc, 0);
    drawMapPerLineNbg23(&info, &texture, 3, xoffset);
  }
  else {
    info.x = (fixVdp2Regs->SCXN3 & 0x7FF) + xoffset;
    info.y = fixVdp2Regs->SCYN3 & 0x7FF;
    drawMap(&info, &texture);
  }

  if (pipleLineNBG3 != nullptr && pipleLineNBG3->vertices.size() != 0) {

    layers[info.priority].push_back(pipleLineNBG3);
  }


}

void VIDVulkan::drawRBG0() {
  vdp2draw_struct  * info = &g_rgb0.info;
  g_rgb0.rgb_type = 0;

  info->dst = 0;
  info->id = 4;
  info->uclipmode = 0;
  info->cor = 0;
  info->cog = 0;
  info->cob = 0;
  info->specialcolorfunction = 0;
  info->enable = fixVdp2Regs->BGON & 0x10;

  info->pipeline = (void*)(&pipleLineRBG0);
  if (pipleLineRBG0) {
    pipleLineRBG0->vertices.clear();
    pipleLineRBG0->indices.clear();
  }


  if (!info->enable) return;
  info->priority = fixVdp2Regs->PRIR & 0x7;
  if (!(info->enable & Vdp2External.disptoggle) || (info->priority == 0)) {

    if (Vdp1Regs->TVMR & 0x02) {
      Vdp2ReadRotationTable(0, &paraA, fixVdp2Regs, Vdp2Ram);
    }
    return;
  }

  for (int i = 0; i < 4; i++) {
    g_rgb0.info.char_bank[i] = 1;
    g_rgb0.info.pname_bank[i] = 1;
    g_rgb1.info.char_bank[i] = 1;
    g_rgb1.info.pname_bank[i] = 1;
  }

  generatePerLineColorCalcuration(info, RBG0);

  info->transparencyenable = !(fixVdp2Regs->BGON & 0x1000);
  info->specialprimode = (fixVdp2Regs->SFPRMD >> 8) & 0x3;

  info->colornumber = (fixVdp2Regs->CHCTLB & 0x7000) >> 12;

  info->bEnWin0 = (fixVdp2Regs->WCTLC >> 1) & 0x01;
  info->WindowArea0 = (fixVdp2Regs->WCTLC >> 0) & 0x01;

  info->bEnWin1 = (fixVdp2Regs->WCTLC >> 3) & 0x01;
  info->WindowArea1 = (fixVdp2Regs->WCTLC >> 2) & 0x01;

  info->LogicWin = (fixVdp2Regs->WCTLC >> 7) & 0x01;

  info->islinescroll = 0;
  info->linescrolltbl = 0;
  info->lineinc = 0;

  Vdp2ReadRotationTable(0, &paraA, fixVdp2Regs, Vdp2Ram);
  Vdp2ReadRotationTable(1, &paraB, fixVdp2Regs, Vdp2Ram);
  A0_Updated = 0;
  A1_Updated = 0;
  B0_Updated = 0;
  B1_Updated = 0;

  //paraA.PlaneAddr = (void FASTCALL(*)(void *, int, Vdp2*))&Vdp2ParameterAPlaneAddr;
  //paraB.PlaneAddr = (void FASTCALL(*)(void *, int, Vdp2*))&Vdp2ParameterBPlaneAddr;
  paraA.charaddr = (fixVdp2Regs->MPOFR & 0x7) * 0x20000;
  paraB.charaddr = (fixVdp2Regs->MPOFR & 0x70) * 0x2000;
  ReadPlaneSizeR(&paraA, fixVdp2Regs->PLSZ >> 8);
  ReadPlaneSizeR(&paraB, fixVdp2Regs->PLSZ >> 12);



  if (paraA.coefdatasize == 2)
  {
    if (paraA.coefmode < 3)
    {
      info->GetKValueA = vdp2rGetKValue1W;
    }
    else {
      info->GetKValueA = vdp2rGetKValue1Wm3;
    }

  }
  else {
    if (paraA.coefmode < 3)
    {
      info->GetKValueA = vdp2rGetKValue2W;
    }
    else {
      info->GetKValueA = vdp2rGetKValue2Wm3;
    }
  }

  if (paraB.coefdatasize == 2)
  {
    if (paraB.coefmode < 3)
    {
      info->GetKValueB = vdp2rGetKValue1W;
    }
    else {
      info->GetKValueB = vdp2rGetKValue1Wm3;
    }

  }
  else {
    if (paraB.coefmode < 3)
    {
      info->GetKValueB = vdp2rGetKValue2W;
    }
    else {
      info->GetKValueB = vdp2rGetKValue2Wm3;
    }
  }

  info->pWinInfo = nullptr;
  if (fixVdp2Regs->RPMD == 0x00)
  {
    if (!(paraA.coefenab))
    {
      info->GetRParam = (Vdp2GetRParam_func)vdp2RGetParamMode00NoK;
    }
    else {
      info->GetRParam = (Vdp2GetRParam_func)vdp2RGetParamMode00WithK;
    }

  }
  else if (fixVdp2Regs->RPMD == 0x01)
  {
    if (!(paraB.coefenab))
    {
      info->GetRParam = (Vdp2GetRParam_func)vdp2RGetParamMode01NoK;
    }
    else {
      info->GetRParam = (Vdp2GetRParam_func)vdp2RGetParamMode01WithK;
    }

  }
  else if (fixVdp2Regs->RPMD == 0x02)
  {
    if (!(paraA.coefenab))
    {
      info->GetRParam = (Vdp2GetRParam_func)vdp2RGetParamMode02NoK;
    }
    else {
      if (paraB.coefenab)
        info->GetRParam = (Vdp2GetRParam_func)vdp2RGetParamMode02WithKAWithKB;
      else
        info->GetRParam = (Vdp2GetRParam_func)vdp2RGetParamMode02WithKA;
    }

  }
  else if (fixVdp2Regs->RPMD == 0x03)
  {
    // Enable Window0(RPW0E)?
    if (((fixVdp2Regs->WCTLD >> 1) & 0x01) == 0x01)
    {
      info->pWinInfo = windowRenderer->getVdp2WindowInfo(0);
      // RPW0A( inside = 0, outside = 1 )
      info->WindwAreaMode = (fixVdp2Regs->WCTLD & 0x01);
      // Enable Window1(RPW1E)?
    }
    else if (((fixVdp2Regs->WCTLD >> 3) & 0x01) == 0x01)
    {
      info->pWinInfo = windowRenderer->getVdp2WindowInfo(1);
      // RPW1A( inside = 0, outside = 1 )
      info->WindwAreaMode = ((fixVdp2Regs->WCTLD >> 2) & 0x01);
      // Bad Setting Both Window is disabled
    }
    else {
      info->pWinInfo = windowRenderer->getVdp2WindowInfo(0);
      info->WindwAreaMode = (fixVdp2Regs->WCTLD & 0x01);
    }

    if (paraA.coefenab == 0 && paraB.coefenab == 0)
    {
      info->GetRParam = (Vdp2GetRParam_func)vdp2RGetParamMode03NoK;
    }
    else if (paraA.coefenab && paraB.coefenab == 0)
    {
      info->GetRParam = (Vdp2GetRParam_func)vdp2RGetParamMode03WithKA;
    }
    else if (paraA.coefenab == 0 && paraB.coefenab)
    {
      info->GetRParam = (Vdp2GetRParam_func)vdp2RGetParamMode03WithKB;
    }
    else if (paraA.coefenab && paraB.coefenab)
    {
      info->GetRParam = (Vdp2GetRParam_func)vdp2RGetParamMode03WithK;
    }
  }


  paraA.screenover = (fixVdp2Regs->PLSZ >> 10) & 0x03;
  paraB.screenover = (fixVdp2Regs->PLSZ >> 14) & 0x03;



  // Figure out which Rotation Parameter we're uqrt
  switch (fixVdp2Regs->RPMD & 0x3)
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
    info->rotatemode = 1 + (fixVdp2Regs->RPMD & 0x1);
    info->PlaneAddr = (void FASTCALL(*)(void *, int, Vdp2*))&Vdp2ParameterAPlaneAddr;
    break;
  }



  if ((info->isbitmap = fixVdp2Regs->CHCTLB & 0x200) != 0)
  {
    // Bitmap Mode
    ReadBitmapSize(info, fixVdp2Regs->CHCTLB >> 10, 0x1);

    if (info->rotatenum == 0)
      // Parameter A
      info->charaddr = (fixVdp2Regs->MPOFR & 0x7) * 0x20000;
    else
      // Parameter B
      info->charaddr = (fixVdp2Regs->MPOFR & 0x70) * 0x2000;

    info->paladdr = (fixVdp2Regs->BMPNB & 0x7) << 4;
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
      ReadPlaneSize(info, fixVdp2Regs->PLSZ >> 8);
    else
      // Parameter B
      ReadPlaneSize(info, fixVdp2Regs->PLSZ >> 12);

    ReadPatternData(info, fixVdp2Regs->PNCR, fixVdp2Regs->CHCTLB & 0x100);

    paraA.ShiftPaneX = 8 + paraA.planew;
    paraA.ShiftPaneY = 8 + paraA.planeh;
    paraB.ShiftPaneX = 8 + paraB.planew;
    paraB.ShiftPaneY = 8 + paraB.planeh;

    paraA.MskH = (8 * 64 * paraA.planew) - 1;
    paraA.MskV = (8 * 64 * paraA.planeh) - 1;
    paraB.MskH = (8 * 64 * paraB.planew) - 1;
    paraB.MskV = (8 * 64 * paraB.planeh) - 1;

    paraA.MaxH = 8 * 64 * paraA.planew * 4;
    paraA.MaxV = 8 * 64 * paraA.planeh * 4;
    paraB.MaxH = 8 * 64 * paraB.planew * 4;
    paraB.MaxV = 8 * 64 * paraB.planeh * 4;

    if (paraA.screenover == OVERMODE_512)
    {
      paraA.MaxH = 512;
      paraA.MaxV = 512;
    }

    if (paraB.screenover == OVERMODE_512)
    {
      paraB.MaxH = 512;
      paraB.MaxV = 512;
    }

    for (i = 0; i < 16; i++)
    {
      Vdp2ParameterAPlaneAddr(info, i, fixVdp2Regs);
      paraA.PlaneAddrv[i] = info->addr;
      Vdp2ParameterBPlaneAddr(info, i, fixVdp2Regs);
      paraB.PlaneAddrv[i] = info->addr;
    }
  }

  ReadMosaicData(info, 0x10, fixVdp2Regs);

  info->specialcolormode = (fixVdp2Regs->SFCCMD >> 8) & 0x03;
  if (fixVdp2Regs->SFSEL & 0x10)
    info->specialcode = fixVdp2Regs->SFCODE >> 8;
  else
    info->specialcode = fixVdp2Regs->SFCODE & 0xFF;

  info->blendmode = VDP2_CC_NONE;
  if ((fixVdp2Regs->LNCLEN & 0x10) == 0x00)
  {
    info->LineColorBase = 0x00;
    paraA.lineaddr = 0xFFFFFFFF;
    paraB.lineaddr = 0xFFFFFFFF;
  }
  else {
    //      info->alpha = 0xFF;
    info->LineColorBase = ((fixVdp2Regs->LCTA.all) & 0x7FFFF) << 1;
    if (info->LineColorBase >= 0x80000) info->LineColorBase = 0x00;
    paraA.lineaddr = 0xFFFFFFFF;
    paraB.lineaddr = 0xFFFFFFFF;
  }

  info->blendmode = 0;

  // 12.13 blur
  if ((fixVdp2Regs->CCCTL & 0xF000) == 0x9000) {
    info->alpha = ((~fixVdp2Regs->CCRR & 0x1F) << 3) + 0x7;
    info->blendmode |= VDP2_CC_BLUR;
  }

  if ((fixVdp2Regs->CCCTL & 0x010) == 0x10) {
    info->alpha = ((~fixVdp2Regs->CCRR & 0x1F) << 3) + 0x7;
    if (fixVdp2Regs->CCCTL & 0x100) {
      info->blendmode |= VDP2_CC_ADD;
    }
    else {
      info->blendmode |= VDP2_CC_RATE;
    }
  }
  else {
    // 12.14 CCRTMD
    if (((fixVdp2Regs->CCCTL >> 9) & 0x01) == 0x01) {
      info->alpha = ((~fixVdp2Regs->CCRR & 0x1F) << 3) + 0x7;
    }
    else {
      info->alpha = 0xFF;
    }
    info->blendmode |= VDP2_CC_NONE;
  }

  info->coloroffset = (fixVdp2Regs->CRAOFB & 0x7) << 8;

  readVdp2ColorOffset(fixVdp2Regs, info, 0x10);
  info->linecheck_mask = 0x10;
  info->coordincx = info->coordincy = 1;

  // Window Mode
  info->bEnWin0 = (fixVdp2Regs->WCTLC >> 1) & 0x01;
  info->WindowArea0 = (fixVdp2Regs->WCTLC >> 0) & 0x01;
  info->bEnWin1 = (fixVdp2Regs->WCTLC >> 3) & 0x01;
  info->WindowArea1 = (fixVdp2Regs->WCTLC >> 2) & 0x01;
  info->LogicWin = (fixVdp2Regs->WCTLC >> 7) & 0x01;

  Vdp2SetGetColor(info);
  drawRotation(&g_rgb0, &pipleLineRBG0);

  if (pipleLineRBG0 != nullptr && pipleLineRBG0->vertices.size() != 0) {
    layers[info->priority].push_back(pipleLineRBG0);
  }

}

void VIDVulkan::drawRotation(RBGDrawInfo * rbg, VdpPipeline ** pipleLine)
{
  vdp2draw_struct *info = &rbg->info;
  int rgb_type = rbg->rgb_type;
  CharTexture *texture = &rbg->texture;
  CharTexture *line_texture = &rbg->line_texture;

  uint32_t x, y;
  int cellw, cellh;
  int oldcellx = -1, oldcelly = -1;
  int lineInc = fixVdp2Regs->LCTA.part.U & 0x8000 ? 2 : 0;
  int linecl = 0xFF;
  Vdp2 * regs;
  if ((fixVdp2Regs->CCCTL >> 5) & 0x01) {
    linecl = ((~fixVdp2Regs->CCRLB & 0x1F) << 3) + 0x7;
  }


  if (vdp2height >= 448) lineInc <<= 1;
  if (vdp2height >= 448) rbg->vres = (vdp2height >> 1); else rbg->vres = vdp2height;
  if (vdp2width >= 640) rbg->hres = (vdp2width >> 1); else rbg->hres = vdp2width;


  switch (rbgResolutionMode) {
  case RBG_RES_ORIGINAL:
    rbg->rotate_mval_h = 1.0f;
    rbg->rotate_mval_v = 1.0f;
    rbg->hres = rbg->hres * rbg->rotate_mval_h;
    rbg->vres = rbg->vres * rbg->rotate_mval_v;
    break;
  case RBG_RES_2x:
    rbg->rotate_mval_h = 2.0f;
    rbg->rotate_mval_v = 2.0f;
    rbg->hres = rbg->hres * rbg->rotate_mval_h;
    rbg->vres = rbg->vres * rbg->rotate_mval_v;
    break;
  case RBG_RES_720P:
    rbg->rotate_mval_h = 1280.0f / rbg->hres;
    rbg->rotate_mval_v = 720.0f / rbg->vres;
    rbg->hres = 1280;
    rbg->vres = 720;
    break;
  case RBG_RES_1080P:
    rbg->rotate_mval_h = 1920.0f / rbg->hres;
    rbg->rotate_mval_v = 1080.0f / rbg->vres;
    rbg->hres = 1920;
    rbg->vres = 1080;
    break;
  case RBG_RES_FIT_TO_EMULATION:
    rbg->rotate_mval_h = (float)_device_width / rbg->hres;
    rbg->rotate_mval_v = (float)_device_height / rbg->vres;
    rbg->hres = _device_width;
    rbg->vres = _device_height;
    break;
  default:
    rbg->rotate_mval_h = 1.0;
    rbg->rotate_mval_v = 1.0;
    break;
  }

  if (rbg_use_compute_shader) {
    rbgGenerator->init(this, rbg->hres, rbg->vres);
  }

  info->vertices[0] = 0;
  info->vertices[1] = 0;
  info->vertices[2] = vdp2width;
  info->vertices[3] = 0;
  info->vertices[4] = vdp2width;
  info->vertices[5] = vdp2height;
  info->vertices[6] = 0;
  info->vertices[7] = vdp2height;
  cellw = info->cellw;
  cellh = info->cellh;
  info->cellw = rbg->hres;
  info->cellh = rbg->vres;
  info->flipfunction = 0;
  info->linescreen = 0;
  info->cor = 0x00;
  info->cog = 0x00;
  info->cob = 0x00;

  if (fixVdp2Regs->RPMD != 0) rbg->useb = 1;
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

  regs = Vdp2RestoreRegs(3, Vdp2Lines);
  if (regs) readVdp2ColorOffset(regs, info, info->linecheck_mask);

  if (info->lineTexture != 0) {
    info->cor = 0;
    info->cog = 0;
    info->cob = 0;
    info->linescreen = 2;
  }

#if 0 // ToDo
  if (rbg->async) {

    u64 cacheaddr = 0x80000000BAD;
    YglTMAllocate(_Ygl->texture_manager, &rbg->texture, info->cellw, info->cellh, &x, &y);
    rbg->c.x = x;
    rbg->c.y = y;
    YglCacheAdd(_Ygl->texture_manager, cacheaddr, &rbg->c);
    info->cellw = cellw;
    info->cellh = cellh;
    curret_rbg = rbg;



    rbg->line_texture.textdata = NULL;
    if (info->LineColorBase != 0)
    {
      rbg->line_info.blendmode = 0;
      rbg->LineColorRamAdress = (T1ReadWord(Vdp2Ram, info->LineColorBase) & 0x7FF);// +info->coloroffset;

      u64 cacheaddr = 0xA0000000DAD;
      YglTMAllocate(_Ygl->texture_manager, &rbg->line_texture, rbg->vres, 1, &x, &y);
      rbg->cline.x = x;
      rbg->cline.y = y;
      YglCacheAdd(_Ygl->texture_manager, cacheaddr, &rbg->cline);

    }
    else {
      rbg->LineColorRamAdress = 0x00;
      rbg->cline.x = -1;
      rbg->cline.y = -1;
      rbg->line_texture.textdata = NULL;
      rbg->line_texture.w = 0;
    }



    if (Vdp2DrawRotationThread_running == 0) {
      Vdp2DrawRotationThread_running = 1;
      g_rotate_mtx = YabThreadCreateMutex();
      YabThreadLock(g_rotate_mtx);
      YabThreadStart(YAB_THREAD_VIDSOFT_LAYER_RBG0, Vdp2DrawRotationThread, NULL);
    }
    Vdp2RgbTextureSync();
    YGL_THREAD_DEBUG("Vdp2DrawRotation in %d\n", curret_rbg->vdp2_sync_flg);
    curret_rbg->vdp2_sync_flg = RBG_REQ_RENDER;
    YabThreadUnLock(g_rotate_mtx);
    YGL_THREAD_DEBUG("Vdp2DrawRotation out %d\n", curret_rbg->vdp2_sync_flg);
  }
  else {
#endif
    u64 cacheaddr = 0x90000000BAD;

    rbg->vdp2_sync_flg = -1;
    if (rbg_use_compute_shader == 0) {
      tm->allocate(&rbg->texture, info->cellw, info->cellh, &x, &y);
      rbg->c.x = x;
      rbg->c.y = y;
      tm->addCache(cacheaddr, &rbg->c);
    }
    info->cellw = cellw;
    info->cellh = cellh;

    rbg->line_texture.textdata = NULL;
    if (info->LineColorBase != 0)
    {
      rbg->line_info.blendmode = 0;
      rbg->LineColorRamAdress = (T1ReadWord(Vdp2Ram, info->LineColorBase) & 0x7FF);// +info->coloroffset;

      u64 cacheaddr = 0xA0000000DAD;
      tm->allocate(&rbg->line_texture, rbg->vres, 1, &x, &y);
      rbg->cline.x = x;
      rbg->cline.y = y;
      tm->addCache(cacheaddr, &rbg->cline);

    }
    else {
      rbg->LineColorRamAdress = 0x00;
      rbg->cline.x = -1;
      rbg->cline.y = -1;
      rbg->line_texture.textdata = NULL;
      rbg->line_texture.w = 0;
    }

    drawRotation_in(rbg);

    //if (rbg_use_compute_shader == 0) {
    rbg->info.cellw = rbg->hres;
    rbg->info.cellh = rbg->vres;
    rbg->info.flipfunction = 0;
    genPolygonRbg0(pipleLine, &rbg->info, NULL, &rbg->c, &rbg->cline, rbg->rgb_type);
    //}
    //else {
    //  curret_rbg = rbg;
    //}
#if 0 // ToDo
  }
#endif
}

#define ceilf(a) ((a)+0.99999f)

static inline int vdp2rGetKValue(vdp2rotationparameter_struct * parameter, float i) {
  float kval;
  int   kdata;
  int h = ceilf(parameter->KtablV + (parameter->deltaKAx * i));
  if (parameter->coefdatasize == 2) {
    if (parameter->k_mem_type == 0) { // vram
      kdata = T1ReadWord(Vdp2Ram, (parameter->coeftbladdr + (int)(h << 1)) & 0x7FFFF);
    }
    else { // cram
      kdata = Vdp2ColorRamReadWord(((parameter->coeftbladdr + (int)(h << 1)) & 0x7FF) + 0x800);
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
      kdata = T1ReadLong(Vdp2Ram, (parameter->coeftbladdr + (int)(h << 2)) & 0x7FFFF);
    }
    else { // cram
      kdata = Vdp2ColorRamReadLong(((parameter->coeftbladdr + (int)(h << 2)) & 0x7FF) + 0x800);
    }
    parameter->lineaddr = (kdata >> 24) & 0x7F;
    if (kdata & 0x80000000) { return 0; }
    kval = (float)(int)((kdata & 0x00FFFFFF) | (kdata & 0x00800000 ? 0xFF800000 : 0x00000000)) / (65536.0f);
    switch (parameter->coefmode) {
    case 0:  parameter->kx = kval; parameter->ky = kval; break;
    case 1:  parameter->kx = kval; break;
    case 2:  parameter->ky = kval; break;
    case 3:  /*ToDo*/  break;
    }
  }
  return 1;
}

static inline u16 Vdp2ColorRamGetColorRaw(u32 colorindex) {
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


inline void setSpecialPriority(vdp2draw_struct *info, u8 dot, u32 * cramindex) {
  int priority;
  if (info->specialprimode == 2) {
    priority = info->priority & 0xE;
    if (info->specialfunction & 1) {
      if (PixelIsSpecialPriority(info->specialcode, dot))
      {
        priority |= 1;
      }
    }
    (*cramindex) |= priority << 16;
  }
}


static inline u32 Vdp2RotationFetchPixel(vdp2draw_struct *info, int x, int y, int cellw)
{
  u32 dot;
  u32 cramindex;
  u32 alpha = info->alpha;
  u8 lowdot = 0x00;
  switch (info->colornumber)
  {
  case 0: // 4 BPP
    dot = T1ReadByte(Vdp2Ram, ((info->charaddr + (((y * cellw) + x) >> 1)) & 0x7FFFF));
    if (!(x & 0x1)) dot >>= 4;
    if (!(dot & 0xF) && info->transparencyenable) return 0x00000000;
    else {
      cramindex = (info->coloroffset + ((info->paladdr << 4) | (dot & 0xF)));
      setSpecialPriority(info, dot, &cramindex);
      switch (info->specialcolormode)
      {
      case 1: if (info->specialcolorfunction == 0) { alpha = 0xFF; } break;
      case 2:
        if (info->specialcolorfunction == 0) { alpha = 0xFF; }
        else { if ((info->specialcode & (1 << ((dot & 0xF) >> 1))) == 0) { alpha = 0xFF; } }
        break;
      case 3:
        if (((Vdp2ColorRamGetColorRaw(cramindex) & 0x8000) == 0)) { alpha = 0xFF; }
        break;
      }
      return   cramindex | alpha << 24;
    }
  case 1: // 8 BPP
    dot = T1ReadByte(Vdp2Ram, ((info->charaddr + (y * cellw) + x) & 0x7FFFF));
    if (!(dot & 0xFF) && info->transparencyenable) return 0x00000000;
    else {
      cramindex = info->coloroffset + ((info->paladdr << 4) | (dot & 0xFF));
      setSpecialPriority(info, dot, &cramindex);
      switch (info->specialcolormode)
      {
      case 1: if (info->specialcolorfunction == 0) { alpha = 0xFF; } break;
      case 2:
        if (info->specialcolorfunction == 0) { alpha = 0xFF; }
        else { if ((info->specialcode & (1 << ((dot & 0xF) >> 1))) == 0) { alpha = 0xFF; } }
        break;
      case 3:
        if (((Vdp2ColorRamGetColorRaw(cramindex) & 0x8000) == 0)) { alpha = 0xFF; }
        break;
      }
      return   cramindex | alpha << 24;
    }
  case 2: // 16 BPP(palette)
    dot = T1ReadWord(Vdp2Ram, ((info->charaddr + ((y * cellw) + x) * 2) & 0x7FFFF));
    if ((dot == 0) && info->transparencyenable) return 0x00000000;
    else {
      cramindex = (info->coloroffset + dot);
      setSpecialPriority(info, dot, &cramindex);
      switch (info->specialcolormode)
      {
      case 1: if (info->specialcolorfunction == 0) { alpha = 0xFF; } break;
      case 2:
        if (info->specialcolorfunction == 0) { alpha = 0xFF; }
        else { if ((info->specialcode & (1 << ((dot & 0xF) >> 1))) == 0) { alpha = 0xFF; } }
        break;
      case 3:
        if (((Vdp2ColorRamGetColorRaw(cramindex) & 0x8000) == 0)) { alpha = 0xFF; }
        break;
      }
      return   cramindex | alpha << 24;
    }
  case 3: // 16 BPP(RGB)
    dot = T1ReadWord(Vdp2Ram, ((info->charaddr + ((y * cellw) + x) * 2) & 0x7FFFF));
    if (!(dot & 0x8000) && info->transparencyenable) return 0x00000000;
    else return SAT2YAB1(alpha, dot);
  case 4: // 32 BPP
    dot = T1ReadLong(Vdp2Ram, ((info->charaddr + ((y * cellw) + x) * 4) & 0x7FFFF));
    if (!(dot & 0x80000000) && info->transparencyenable) return 0x00000000;
    else return SAT2YAB2(alpha, (dot >> 16), dot);
  default:
    return 0;
  }
}


void VIDVulkan::drawRotation_in(RBGDrawInfo * rbg) {

  if (rbg == NULL) return;

  vdp2draw_struct *info = &rbg->info;
  int rgb_type = rbg->rgb_type;
  CharTexture *texture = &rbg->texture;
  CharTexture *line_texture = &rbg->line_texture;

  float i, j;
  int x, y;
  int cellw, cellh;
  int oldcellx = -1, oldcelly = -1;
  u32 color;
  int vres, hres;
  int h, h2;
  int v, v2;
  int lineInc = fixVdp2Regs->LCTA.part.U & 0x8000 ? 2 : 0;
  int linecl = 0xFF;
  vdp2rotationparameter_struct *parameter;
  Vdp2 * regs;
  if ((fixVdp2Regs->CCCTL >> 5) & 0x01) {
    linecl = ((~fixVdp2Regs->CCRLB & 0x1F) << 3) + 0x7;
  }

  if (vdp2height >= 448) {
    lineInc <<= 1;
    info->drawh = (vdp2height >> 1);
    info->hres_shift = 1;
  }
  else {
    info->hres_shift = 0;
    info->drawh = vdp2height;
  }

  vres = rbg->vres / rbg->rotate_mval_v;
  hres = rbg->hres / rbg->rotate_mval_h;
  cellw = rbg->info.cellw;
  cellh = rbg->info.cellh;
  regs = Vdp2RestoreRegs(3, Vdp2Lines);

  x = 0;
  y = 0;
  u32 lineaddr = 0;
  if (rgb_type == 0)
  {
    paraA.dx = paraA.A * paraA.deltaX + paraA.B * paraA.deltaY;
    paraA.dy = paraA.D * paraA.deltaX + paraA.E * paraA.deltaY;
    paraA.Xp = paraA.A * (paraA.Px - paraA.Cx) +
      paraA.B * (paraA.Py - paraA.Cy) +
      paraA.C * (paraA.Pz - paraA.Cz) + paraA.Cx + paraA.Mx;
    paraA.Yp = paraA.D * (paraA.Px - paraA.Cx) +
      paraA.E * (paraA.Py - paraA.Cy) +
      paraA.F * (paraA.Pz - paraA.Cz) + paraA.Cy + paraA.My;
  }

  if (rbg->useb)
  {
    paraB.dx = paraB.A * paraB.deltaX + paraB.B * paraB.deltaY;
    paraB.dy = paraB.D * paraB.deltaX + paraB.E * paraB.deltaY;
    paraB.Xp = paraB.A * (paraB.Px - paraB.Cx) + paraB.B * (paraB.Py - paraB.Cy)
      + paraB.C * (paraB.Pz - paraB.Cz) + paraB.Cx + paraB.Mx;
    paraB.Yp = paraB.D * (paraB.Px - paraB.Cx) + paraB.E * (paraB.Py - paraB.Cy)
      + paraB.F * (paraB.Pz - paraB.Cz) + paraB.Cy + paraB.My;
  }

  paraA.over_pattern_name = fixVdp2Regs->OVPNRA;
  paraB.over_pattern_name = fixVdp2Regs->OVPNRB;

  if (rbg_use_compute_shader) {
    rbgGenerator->update(rbg, paraA, paraB);

    if (info->LineColorBase != 0) {
      const float vstep = 1.0 / rbg->rotate_mval_v;
      j = 0.0f;
      int lvres = rbg->vres;
      if (vres >= 480) {
        lvres >>= 1;
      }
      for (int jj = 0; jj < lvres; jj++) {
        if ((fixVdp2Regs->LCTA.part.U & 0x8000) != 0) {
          rbg->LineColorRamAdress = T1ReadWord(Vdp2Ram, info->LineColorBase + lineInc * (int)(j));
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

    return;
  }

  const float vstep = 1.0 / rbg->rotate_mval_v;
  const float hstep = 1.0 / rbg->rotate_mval_h;
  //for (j = 0; j < vres; j += vstep)
  j = 0.0f;
  for (int jj = 0; jj < rbg->vres; jj++)
  {
#if 0 // PERLINE
    Vdp2 * regs = Vdp2RestoreRegs(j, Vdp2Lines);
#endif

    if (rgb_type == 0) {
#if 0 // PERLINE
      paraA.charaddr = (regs->MPOFR & 0x7) * 0x20000;
      ReadPlaneSizeR(&paraA, regs->PLSZ >> 8);
      for (i = 0; i < 16; i++) {
        paraA.PlaneAddr(info, i, regs);
        paraA.PlaneAddrv[i] = info->addr;
      }
#endif
      paraA.Xsp = paraA.A * ((paraA.Xst + paraA.deltaXst * j) - paraA.Px) +
        paraA.B * ((paraA.Yst + paraA.deltaYst * j) - paraA.Py) +
        paraA.C * (paraA.Zst - paraA.Pz);

      paraA.Ysp = paraA.D * ((paraA.Xst + paraA.deltaXst *j) - paraA.Px) +
        paraA.E * ((paraA.Yst + paraA.deltaYst * j) - paraA.Py) +
        paraA.F * (paraA.Zst - paraA.Pz);

      paraA.KtablV = paraA.deltaKAst* j;
    }
    if (rbg->useb)
    {
#if 0 // PERLINE
      Vdp2ReadRotationTable(1, &paraB, regs, Vdp2Ram);
      paraB.dx = paraB.A * paraB.deltaX + paraB.B * paraB.deltaY;
      paraB.dy = paraB.D * paraB.deltaX + paraB.E * paraB.deltaY;
      paraB.Xp = paraB.A * (paraB.Px - paraB.Cx) + paraB.B * (paraB.Py - paraB.Cy)
        + paraB.C * (paraB.Pz - paraB.Cz) + paraB.Cx + paraB.Mx;
      paraB.Yp = paraB.D * (paraB.Px - paraB.Cx) + paraB.E * (paraB.Py - paraB.Cy)
        + paraB.F * (paraB.Pz - paraB.Cz) + paraB.Cy + paraB.My;

      ReadPlaneSize(info, regs->PLSZ >> 12);
      ReadPatternData(info, regs->PNCN0, regs->CHCTLA & 0x1);

      paraB.charaddr = (regs->MPOFR & 0x70) * 0x2000;
      ReadPlaneSizeR(&paraB, regs->PLSZ >> 12);
      for (i = 0; i < 16; i++) {
        paraB.PlaneAddr(info, i, regs);
        paraB.PlaneAddrv[i] = info->addr;
      }
#endif
      paraB.Xsp = paraB.A * ((paraB.Xst + paraB.deltaXst * j) - paraB.Px) +
        paraB.B * ((paraB.Yst + paraB.deltaYst * j) - paraB.Py) +
        paraB.C * (paraB.Zst - paraB.Pz);

      paraB.Ysp = paraB.D * ((paraB.Xst + paraB.deltaXst * j) - paraB.Px) +
        paraB.E * ((paraB.Yst + paraB.deltaYst * j) - paraB.Py) +
        paraB.F * (paraB.Zst - paraB.Pz);

      paraB.KtablV = paraB.deltaKAst * j;
    }

    if (info->LineColorBase != 0)
    {
      if ((fixVdp2Regs->LCTA.part.U & 0x8000) != 0) {
        rbg->LineColorRamAdress = T1ReadWord(Vdp2Ram, info->LineColorBase + lineInc * (int)(j));
        *line_texture->textdata = rbg->LineColorRamAdress | (linecl << 24);
        line_texture->textdata++;
      }
      else {
        *line_texture->textdata = rbg->LineColorRamAdress;
        line_texture->textdata++;
      }
    }

    if (regs) readVdp2ColorOffset(regs, info, info->linecheck_mask);
    //for (i = 0; i < hres; i += hstep)
    i = 0.0;
    for (int ii = 0; ii < rbg->hres; ii++)
    {
      /*
            if (Vdp2CheckWindowDot( info, (int)i, (int)j) == 0) {
              *(texture->textdata++) = 0x00000000;
              continue; // may be faster than GPU
            }
      */
      switch (fixVdp2Regs->RPMD | rgb_type) {
      case 0:
        parameter = &paraA;
        if (parameter->coefenab) {
          if (vdp2rGetKValue(parameter, i) == 0) {
            *(texture->textdata++) = 0x00000000;
            i += hstep;
            continue;
          }
        }
        break;
      case 1:
        parameter = &paraB;
        if (parameter->coefenab) {
          if (vdp2rGetKValue(parameter, i) == 0) {
            *(texture->textdata++) = 0x00000000;
            i += hstep;
            continue;
          }
        }
        break;
      case 2:
        if (!(paraA.coefenab)) {
          parameter = &paraA;
        }
        else {
          if (paraB.coefenab) {
            parameter = &paraA;
            if (vdp2rGetKValue(parameter, i) == 0) {
              parameter = &paraB;
              if (vdp2rGetKValue(parameter, i) == 0) {
                *(texture->textdata++) = 0x00000000;
                i += hstep;
                continue;
              }
            }
          }
          else {
            parameter = &paraA;
            if (vdp2rGetKValue(parameter, i) == 0) {
              paraB.lineaddr = paraA.lineaddr;
              parameter = &paraB;
            }
            else {
              int a = 0;
            }
          }
        }
        break;
      default:
        parameter = info->GetRParam(info, (int)i, (int)j);
        break;
      }
      if (parameter == NULL)
      {
        *(texture->textdata++) = 0x00000000;
        i += hstep;
        continue;
      }

      float fh = (parameter->kx * (parameter->Xsp + parameter->dx * i) + parameter->Xp);
      float fv = (parameter->ky * (parameter->Ysp + parameter->dy * i) + parameter->Yp);
      h = fh;
      v = fv;

      //v = jj;
      //h = ii;

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
            *(texture->textdata++) = 0x0;
            i += hstep;
            continue;
          }
          break;
        case OVERMODE_512:
          if ((h < 0) || (h > 512) || (v < 0) || (v > 512)) {
            *(texture->textdata++) = 0x00;
            i += hstep;
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
            i += hstep;
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

            getPatternAddr(info); // Heh, this could be optimized
          }
          break;
        case OVERMODE_512:
          if ((h < 0) || (h > 512) || (v < 0) || (v > 512)) {
            *(texture->textdata++) = 0x00;
            i += hstep;
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

            getPatternAddr(info); // Heh, this could be optimized
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

            getPatternAddr(info); // Heh, this could be optimized
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
              getPatternAddrUsingPatternname(info, parameter->over_pattern_name);
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
              getPatternAddr(info); // Heh, this could be optimized
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

      if (info->LineColorBase != 0 && VDP2_CC_NONE != (info->blendmode & 0x03)) {
        if ((color & 0xFF000000) != 0) {
          color |= 0x8000;
          if (parameter->linecoefenab && parameter->lineaddr != 0xFFFFFFFF && parameter->lineaddr != 0x000000) {
            color |= ((parameter->lineaddr & 0x7F) | 0x80) << 16;
          }
        }
      }

      *(texture->textdata++) = color;
      i += hstep;
    }
    texture->textdata += texture->w;
    j += vstep;
  }
}



void VIDVulkan::genLineinfo(vdp2draw_struct *info)
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

  for (i = 0; i < vdp2height; i += info->lineinc)
  {
    index = 0;
    if (VDPLINE_SX(info->islinescroll))
    {
      info->lineinfo[lineindex].LineScrollValH = T1ReadWord(Vdp2Ram, info->linescrolltbl + (i / info->lineinc)*bound);
      if ((info->lineinfo[lineindex].LineScrollValH & 0x400)) info->lineinfo[lineindex].LineScrollValH |= 0xF800; else info->lineinfo[lineindex].LineScrollValH &= 0x07FF;
      index += 4;
    }
    else {
      info->lineinfo[lineindex].LineScrollValH = 0;
    }

    if (VDPLINE_SY(info->islinescroll))
    {
      info->lineinfo[lineindex].LineScrollValV = T1ReadWord(Vdp2Ram, info->linescrolltbl + (i / info->lineinc)*bound + index);
      if ((info->lineinfo[lineindex].LineScrollValV & 0x400)) info->lineinfo[lineindex].LineScrollValV |= 0xF800; else info->lineinfo[lineindex].LineScrollValV &= 0x07FF;
      index += 4;
    }
    else {
      info->lineinfo[lineindex].LineScrollValV = 0;
    }

    if (VDPLINE_SZ(info->islinescroll))
    {
      val1 = T1ReadWord(Vdp2Ram, info->linescrolltbl + (i / info->lineinc)*bound + index);
      val2 = T1ReadWord(Vdp2Ram, info->linescrolltbl + (i / info->lineinc)*bound + index + 2);
      info->lineinfo[lineindex].CoordinateIncH = (((int)((val1) & 0x07) << 8) | (int)((val2) >> 8));
      index += 4;
    }
    else {
      info->lineinfo[lineindex].CoordinateIncH = 0x0100;
    }

    lineindex++;
  }
}

void VIDVulkan::generatePerLineColorCalcuration(vdp2draw_struct * info, int id) {
  int bit = 1 << id;
  int line = 0;
  if (*Vdp2External.perline_alpha_draw & bit) {
    u32 * linebuf;
    int line_shift = 0;
    if (vdp2height > 256) {
      line_shift = 1;
    }
    else {
      line_shift = 0;
    }

    info->blendmode = VDP2_CC_NONE;

    if (perline[id].dynamicBuf == nullptr) {
      perline[id].create(this, 512, 1);
    }
    linebuf = perline[id].dynamicBuf;
    for (line = 0; line < vdp2height; line++) {
      if ((Vdp2Lines[line >> line_shift].BGON & bit) == 0x00) {
        linebuf[line] = 0x00;
      }
      else {
        info->enable = 1;
        if (Vdp2Lines[line >> line_shift].CCCTL & bit)
        {
          if (fixVdp2Regs->CCCTL & 0x100) { // Add Color
            info->blendmode |= VDP2_CC_ADD;
          }
          else {
            info->blendmode |= VDP2_CC_RATE;
          }

          switch (id) {
          case NBG0:
            linebuf[line] = (((~Vdp2Lines[line >> line_shift].CCRNA & 0x1F) << 3) + 0x7) << 24;
            break;
          case NBG1:
            linebuf[line] = (((~Vdp2Lines[line >> line_shift].CCRNA & 0x1F00) >> 5) + 0x7) << 24;
            break;
          case NBG2:
            linebuf[line] = (((~Vdp2Lines[line >> line_shift].CCRNB & 0x1F) << 3) + 0x7) << 24;
            break;
          case NBG3:
            linebuf[line] = (((~Vdp2Lines[line >> line_shift].CCRNB & 0x1F00) >> 5) + 0x7) << 24;
            break;
          case RBG0:
            linebuf[line] = (((~Vdp2Lines[line >> line_shift].CCRR & 0x1F) << 3) + 0x7) << 24;
            break;
          }

        }
        else {
          linebuf[line] = 0xFF000000;
        }

        if ((Vdp2Lines[line >> line_shift].CLOFEN  & bit) != 0) {
          readVdp2ColorOffset(&Vdp2Lines[line >> line_shift], info, bit);
          linebuf[line] |= ((int)(128.0f + (info->cor / 2.0)) & 0xFF) << 0;
          linebuf[line] |= ((int)(128.0f + (info->cog / 2.0)) & 0xFF) << 8;
          linebuf[line] |= ((int)(128.0f + (info->cob / 2.0)) & 0xFF) << 16;
        }
        else {
          linebuf[line] |= 0x00808080;
        }

      }
    }
    info->lineTexture = (u64)(perline[id].imageView);
  }
  else {
    info->lineTexture = 0;
  }

}



void VIDVulkan::drawBitmap(vdp2draw_struct *info, CharTexture *texture)
{
  int i, j;

  switch (info->colornumber)
  {
  case 0: // 4 BPP
    for (i = 0; i < info->cellh; i++)
    {
      if (info->char_bank[info->charaddr >> 17] == 0) {
        for (j = 0; j < info->cellw; j += 4)
        {
          *texture->textdata = 0;
          *texture->textdata++;
          *texture->textdata = 0;
          *texture->textdata++;
          *texture->textdata = 0;
          *texture->textdata++;
          *texture->textdata = 0;
          *texture->textdata++;
          info->charaddr += 2;
        }
      }
      else {
        for (j = 0; j < info->cellw; j += 4)
        {
          getPixel4bpp(info, info->charaddr, texture);
          info->charaddr += 2;
        }
      }
      texture->textdata += texture->w;
    }
    break;
  case 1: // 8 BPP
    for (i = 0; i < info->cellh; i++)
    {
      if (info->char_bank[info->charaddr >> 17] == 0) {
        for (j = 0; j < info->cellw; j += 2)
        {
          *texture->textdata = 0x00000000;
          texture->textdata++;
          *texture->textdata = 0x00000000;
          texture->textdata++;
          info->charaddr += 2;
        }
      }
      else {
        for (j = 0; j < info->cellw; j += 2)
        {
          getPixel8bpp(info, info->charaddr, texture);
          info->charaddr += 2;
        }
      }
      texture->textdata += texture->w;
    }
    break;
  case 2: // 16 BPP(palette)
    for (i = 0; i < info->cellh; i++)
    {
      if (info->char_bank[info->charaddr >> 17] == 0) {
        for (j = 0; j < info->cellw; j++)
        {
          *texture->textdata = 0;
          *texture->textdata++;
          info->charaddr += 2;
        }
      }
      else {
        for (j = 0; j < info->cellw; j++)
        {
          *texture->textdata++ = getPixel16bpp(info, info->charaddr);
          info->charaddr += 2;
        }
      }
      texture->textdata += texture->w;
    }
    break;
  case 3: // 16 BPP(RGB)
    for (i = 0; i < info->cellh; i++)
    {
      if (info->char_bank[info->charaddr >> 17] == 0) {
        for (j = 0; j < info->cellw; j++)
        {
          *texture->textdata = 0;
          *texture->textdata++;
          info->charaddr += 2;
        }
      }
      else {
        for (j = 0; j < info->cellw; j++)
        {
          *texture->textdata++ = getPixel16bppbmp(info, info->charaddr);
          info->charaddr += 2;
        }
      }
      texture->textdata += texture->w;
    }
    break;
  case 4: // 32 BPP
    for (i = 0; i < info->cellh; i++)
    {
      /*
            if (info->char_bank[info->charaddr >> 17] == 0) {
              for (j = 0; j < info->cellw; j++)
              {
                *texture->textdata = 0;
                *texture->textdata++;
                info->charaddr += 4;
              }
            }
            else {
              for (j = 0; j < info->cellw; j++)
              {
                *texture->textdata++ = Vdp2GetPixel32bppbmp(info, info->charaddr);
                info->charaddr += 4;
              }
            }
      */
      for (j = 0; j < info->cellw; j++)
      {
        *texture->textdata++ = getPixel32bppbmp(info, info->charaddr);
        info->charaddr += 4;
      }
      texture->textdata += texture->w;
    }
    break;
  }
}



void VIDVulkan::drawBitmapLineScroll(vdp2draw_struct *info, CharTexture *texture)
{
  int i, j;
  int height = vdp2height;

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
    if (line->LineScrollValH >= 0 && line->LineScrollValH < sh && sv > 0) {
      sv -= 1;
    }

    switch (info->colornumber) {
    case 0:
      baseaddr += ((sh + sv * (info->cellw >> 2)) << 1);
      if (info->char_bank[baseaddr >> 17] == 0) {
        for (j = 0; j < vdp2width; j += 4)
        {
          *texture->textdata++ = 0;
          *texture->textdata++ = 0;
          *texture->textdata++ = 0;
          *texture->textdata++ = 0;
          baseaddr += 2;
        }
      }
      else {
        for (j = 0; j < vdp2width; j += 4)
        {
          getPixel4bpp(info, baseaddr, texture);
          baseaddr += 2;
        }
      }
      break;
    case 1:
      baseaddr += sh + sv * info->cellw;
      if (info->char_bank[baseaddr >> 17] == 0) {
        for (j = 0; j < vdp2width; j += 2)
        {
          *texture->textdata++ = 0;
          *texture->textdata++ = 0;
          baseaddr += 2;
        }
      }
      else {
        for (j = 0; j < vdp2width; j += 2)
        {
          getPixel8bpp(info, baseaddr, texture);
          baseaddr += 2;
        }
      }
      break;
    case 2:
      baseaddr += ((sh + sv * info->cellw) << 1);
      if (info->char_bank[baseaddr >> 17] == 0) {
        for (j = 0; j < vdp2width; j++)
        {
          *texture->textdata++ = 0;
          baseaddr += 2;
        }
      }
      else {
        for (j = 0; j < vdp2width; j++)
        {
          *texture->textdata++ = getPixel16bpp(info, baseaddr);
          baseaddr += 2;

        }
      }
      break;
    case 3:
      baseaddr += ((sh + sv * info->cellw) << 1);
      if (info->char_bank[baseaddr >> 17] == 0) {
        for (j = 0; j < vdp2width; j++)
        {
          *texture->textdata++ = 0;
          baseaddr += 2;
        }
      }
      else {
        for (j = 0; j < vdp2width; j++)
        {
          *texture->textdata++ = getPixel16bppbmp(info, baseaddr);
          baseaddr += 2;
        }
      }
      break;
    case 4:
      baseaddr += ((sh + sv * info->cellw) << 2);
      if (info->char_bank[baseaddr >> 17] == 0) {
        for (j = 0; j < vdp2width; j++)
        {
          *texture->textdata++ = 0;
          baseaddr += 4;
        }
      }
      else {
        for (j = 0; j < vdp2width; j++)
        {
          //if (info->isverticalscroll){
          //	sv += T1ReadLong(Vdp2Ram, info->verticalscrolltbl+(j>>3) ) >> 16;
          //}
          *texture->textdata++ = getPixel32bppbmp(info, baseaddr);
          baseaddr += 4;
        }
      }
      break;
    }
    texture->textdata += texture->w;
  }
}

void VIDVulkan::drawBitmapCoordinateInc(vdp2draw_struct *info, CharTexture *texture)
{
  u32 color;
  int i, j;

  int incv = 1.0 / info->coordincy*256.0;
  int inch = 1.0 / info->coordincx*256.0;

  int lineinc = 1;
  int linestart = 0;

  int height = vdp2height;
  if ((vdp1_interlace != 0) || (height >= 448)) {
    lineinc = 2;
  }

  if (vdp1_interlace != 0) {
    linestart = vdp1_interlace - 1;
  }

  for (i = linestart; i < height; i += lineinc)
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
      for (j = 0; j < vdp2width; j++)
      {
        u32 h = ((j*inch) >> 8);
        u32 addr = ((sh + h)&(info->cellw - 1) + (sv*info->cellw)) >> 1; // Not confrimed!
        if (addr >= 0x80000) {
          *texture->textdata++ = 0x0000;
        }
        else {
          u8 dot = T1ReadByte(Vdp2Ram, baseaddr + addr);
          u32 alpha = info->alpha;
          if (!(h & 0x01)) dot >>= 4;
          if (!(dot & 0xF) && info->transparencyenable) *texture->textdata++ = 0x00000000;
          else {
            color = (info->coloroffset + ((info->paladdr << 4) | (dot & 0xF)));
            switch (info->specialcolormode)
            {
            case 1: if (info->specialcolorfunction == 0) { alpha = 0xFF; } break;
            case 2:
              if (info->specialcolorfunction == 0) { alpha = 0xFF; }
              else { if ((info->specialcode & (1 << ((dot & 0xF) >> 1))) == 0) { alpha = 0xFF; } }
              break;
            case 3:
              if (((T2ReadWord(Vdp2ColorRam, (color << 1) & 0xFFF) & 0x8000) == 0)) { alpha = 0xFF; }
              break;
            }
            *texture->textdata++ = color | (alpha << 24);
          }
        }
      }
      break;
    case 1: {

      // Shining force 3 battle sciene
      u32 maxaddr = (info->cellw * info->cellh) + info->cellw;
      for (j = 0; j < vdp2width; j++)
      {
        int h = ((j*inch) >> 8);
        u32 alpha = info->alpha;
        u32 addr = ((sh + h)&(info->cellw - 1)) + sv * info->cellw;
        u8 dot = T1ReadByte(Vdp2Ram, baseaddr + addr);
        if (!dot && info->transparencyenable) {
          *texture->textdata++ = 0; continue;
        }
        else {
          color = info->coloroffset + ((info->paladdr << 4) | (dot & 0xFF));
          switch (info->specialcolormode)
          {
          case 1: if (info->specialcolorfunction == 0) { alpha = 0xFF; } break;
          case 2:
            if (info->specialcolorfunction == 0) { alpha = 0xFF; }
            else { if ((info->specialcode & (1 << ((dot & 0xF) >> 1))) == 0) { alpha = 0xFF; } }
            break;
          case 3:
            if (((T2ReadWord(Vdp2ColorRam, (color << 1) & 0xFFF) & 0x8000) == 0)) { alpha = 0xFF; }
            break;
          }
        }
        *texture->textdata++ = color | (alpha << 24);
      }
      break;
    }
    case 2:
      //baseaddr += ((sh + sv * info->cellw) << 1);
      for (j = 0; j < vdp2width; j++)
      {
        int h = ((j*inch) >> 8);
        u32 addr = (((sh + h)&(info->cellw - 1)) + sv * info->cellw) << 1;  // Not confrimed
        *texture->textdata++ = getPixel16bpp(info, baseaddr + addr);

      }
      break;
    case 3:
      //baseaddr += ((sh + sv * info->cellw) << 1);
      for (j = 0; j < vdp2width; j++)
      {
        int h = ((j*inch) >> 8);
        u32 addr = (((sh + h)&(info->cellw - 1)) + sv * info->cellw) << 1;  // Not confrimed
        *texture->textdata++ = getPixel16bppbmp(info, baseaddr + addr);
      }
      break;
    case 4:
      //baseaddr += ((sh + sv * info->cellw) << 2);
      for (j = 0; j < vdp2width; j++)
      {
        int h = (j*inch >> 8);
        u32 addr = (((sh + h)&(info->cellw - 1)) + sv * info->cellw) << 2;  // Not confrimed
        *texture->textdata++ = getPixel32bppbmp(info, baseaddr + addr);
      }
      break;
    }
    texture->textdata += texture->w;
  }
}

void VIDVulkan::drawMapPerLineNbg23(vdp2draw_struct *info, CharTexture *texture, int id, int xoffset) {

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

  int preplanex = -1;
  int preplaney = -1;
  int prepagex = -1;
  int prepagey = -1;
  int mapid = 0;
  int premapid = -1;

  info->patternpixelwh = 8 * info->patternwh;
  info->draww = vdp2width;

  int res_shift = 0;
  if (vdp2height >= 448) {
    info->drawh = (vdp2height >> 1);
    res_shift = 1;
  }
  else {
    info->drawh = vdp2height;
    res_shift = 0;
  }

  for (v = 0; v < vdp2height; v += 1) {

    int targetv = 0;
    Vdp2 * regs = Vdp2RestoreRegs(v >> res_shift, Vdp2Lines);

    if (id == 2) {
      sx = (regs->SCXN2 & 0x7FF) + xoffset;
      targetv = (regs->SCYN2 & 0x7FF);
    }
    else if (id == 3) {
      sx = (regs->SCXN3 & 0x7FF) + xoffset;
      targetv = (regs->SCYN3 & 0x7FF);
    }
    else {
      LOG("Bad id");
      return;
    }

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

    for (int j = 0; j < info->draww; j += 1) {

      //mapx = (h + sx) / (512 * info->planew);
      mapx = (j + sx) >> planew_shift;
      //int dot_on_planex = (h + sx) - mapx*(512 * info->planew);
      dot_on_planex = (j + sx) - (mapx << planew_shift);
      mapx = mapx & 0x01;

      mapid = info->mapwh * mapy + mapx;
      if (mapid != premapid) {
        info->PlaneAddr(info, mapid, fixVdp2Regs);
        premapid = mapid;
      }

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

      if (planex != preplanex || pagex != prepagex || planey != preplaney || pagey != prepagey) {
        getPalAndCharAddr(info, planex, pagex, planey, pagey);
        preplanex = planex;
        preplaney = planey;
        prepagex = pagex;
        prepagey = pagey;
      }

      int x = charx;
      int y = chary;

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
      *texture->textdata++ = Vdp2RotationFetchPixel(info, x, y, info->cellw);
    }
    texture->textdata += texture->w;
  }

}


void VIDVulkan::drawMap(vdp2draw_struct *info, CharTexture *texture) {
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
  info->draww = (int)((float)vdp2width / info->coordincx);
  info->drawh = (int)((float)vdp2height / info->coordincy);
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
        targetv = info->y + v + (T1ReadLong(Vdp2Ram, info->verticalscrolltbl + cell_count) >> 16);
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

      info->PlaneAddr(info, info->mapwh * mapy + mapx, fixVdp2Regs);
      getPalAndCharAddr(info, planex, pagex, planey, pagey);
      drawPattern(info, texture, h - charx, v - chary, 0, 0);

    }

    lineindex++;
  }

}

void VIDVulkan::drawMapPerLine(vdp2draw_struct *info, CharTexture *texture) {

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

  int preplanex = -1;
  int preplaney = -1;
  int prepagex = -1;
  int prepagey = -1;
  int mapid = 0;
  int premapid = -1;

  info->patternpixelwh = 8 * info->patternwh;
  info->draww = vdp2width;

  if (vdp2height >= 448)
    info->drawh = (vdp2height >> 1);
  else
    info->drawh = vdp2height;

  const int incv = 1.0 / info->coordincy*256.0;
  const int res_shift = 0;
  //if (vdp2height >= 440) res_shift = 0;

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


  for (v = 0; v < vdp2height; v += 1) {  // ToDo: info->coordincy

    int targetv = 0;

    if (VDPLINE_SX(info->islinescroll)) {
      sx = info->sh + info->lineinfo[lineindex << res_shift].LineScrollValH;
    }
    else {
      sx = info->sh;
    }

    if (VDPLINE_SY(info->islinescroll)) {
      targetv = info->sv + (v&linemask) + info->lineinfo[lineindex << res_shift].LineScrollValV;
    }
    else {
      targetv = info->sv + ((v*incv) >> 8);
    }

    if (info->isverticalscroll) {
      // this is *wrong*, vertical scroll use a different value per cell
      // info->verticalscrolltbl should be incremented by info->verticalscrollinc
      // each time there's a cell change and reseted at the end of the line...
      // or something like that :)
      targetv += T1ReadLong(Vdp2Ram, info->verticalscrolltbl) >> 16;
    }

    if (VDPLINE_SZ(info->islinescroll)) {
      info->coordincx = info->lineinfo[lineindex << res_shift].CoordinateIncH / 256.0f;
      if (info->coordincx == 0) {
        info->coordincx = vdp2width;
      }
      else {
        info->coordincx = 1.0f / info->coordincx;
      }
    }

    if (info->coordincx < info->maxzoom) info->coordincx = info->maxzoom;


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

    int inch = 1.0 / info->coordincx*256.0;

    for (int j = 0; j < info->draww; j += 1) {

      int h = ((j*inch) >> 8);

      //mapx = (h + sx) / (512 * info->planew);
      mapx = (h + sx) >> planew_shift;
      //int dot_on_planex = (h + sx) - mapx*(512 * info->planew);
      dot_on_planex = (h + sx) - (mapx << planew_shift);
      mapx = mapx & 0x01;

      mapid = info->mapwh * mapy + mapx;
      if (mapid != premapid) {
        info->PlaneAddr(info, mapid, fixVdp2Regs);
        premapid = mapid;
      }

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

      if (planex != preplanex || pagex != prepagex || planey != preplaney || pagey != prepagey) {
        getPalAndCharAddr(info, planex, pagex, planey, pagey);
        preplanex = planex;
        preplaney = planey;
        prepagex = pagex;
        prepagey = pagey;
      }

      int x = charx;
      int y = chary;

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

      *texture->textdata++ = Vdp2RotationFetchPixel(info, x, y, info->cellw);

    }
    if ((v & linemask) == linemask) lineindex++;
    texture->textdata += texture->w;
  }

}


void VIDVulkan::getPatternAddrUsingPatternname(vdp2draw_struct *info, u16 paternname)
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

  if (!(fixVdp2Regs->VRSIZE & 0x8000))
    info->charaddr &= 0x3FFF;

  info->charaddr *= 0x20; // thanks Runik
}


void VIDVulkan::getPatternAddr(vdp2draw_struct *info)
{
  info->addr &= 0x7FFFF;
  switch (info->patterndatasize)
  {
  case 1:
  {
    u16 tmp = T1ReadWord(Vdp2Ram, info->addr);

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
    u16 tmp1 = T1ReadWord(Vdp2Ram, (info->addr & 0x7FFFF));
    u16 tmp2 = T1ReadWord(Vdp2Ram, (info->addr & 0x7FFFF) + 2);
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

  if (!(fixVdp2Regs->VRSIZE & 0x8000))
    info->charaddr &= 0x3FFF;

  info->charaddr *= 0x20; // thanks Runik
}



void VIDVulkan::getPalAndCharAddr(vdp2draw_struct *info, int planex, int x, int planey, int y)
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
    u16 tmp = T1ReadWord(Vdp2Ram, addr);

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
    u16 tmp1 = T1ReadWord(Vdp2Ram, addr);
    u16 tmp2 = T1ReadWord(Vdp2Ram, addr + 2);
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

  if (!(fixVdp2Regs->VRSIZE & 0x8000))
    info->charaddr &= 0x3FFF;

  info->charaddr *= 0x20; // thanks Runik
}

void VIDVulkan::drawPattern(vdp2draw_struct *info, CharTexture *texture, int x, int y, int cx, int cy)
{
  u64 cacheaddr = ((u32)(info->alpha >> 3) << 27) |
    (info->paladdr << 20) | info->charaddr | info->transparencyenable |
    ((info->patternpixelwh >> 4) << 1) | (((u64)(info->coloroffset >> 8) & 0x07) << 32);

  TextureCache c;
  vdp2draw_struct tile = {};
  int winmode = 0;
  tile.dst = 0;
  tile.uclipmode = 0;
  tile.colornumber = info->colornumber;
  tile.blendmode = info->blendmode;
  tile.linescreen = info->linescreen;
  tile.mosaicxmask = info->mosaicxmask;
  tile.mosaicymask = info->mosaicymask;
  tile.bEnWin0 = info->bEnWin0;
  tile.WindowArea0 = info->WindowArea0;
  tile.bEnWin1 = info->bEnWin1;
  tile.WindowArea1 = info->WindowArea1;
  tile.LogicWin = info->LogicWin;
  tile.lineTexture = info->lineTexture;
  tile.id = info->id;

  tile.cellw = tile.cellh = info->patternpixelwh;
  tile.flipfunction = info->flipfunction;

  tile.cx = cx;
  tile.cy = cy;
  tile.coordincx = info->coordincx;
  tile.coordincy = info->coordincy;

  //if (info->specialprimode == 1)
  //  tile.priority = (info->priority & 0xFFFFFFFE) | info->specialfunction;
  //else
  tile.priority = info->priority;
  if (info->specialprimode == 1) {
    tile.priorityOffset = (int)((info->priority & 0xFFFFFFFE) | info->specialfunction) - (int)info->priority;
  }
  else {
    tile.priorityOffset = 0;
  }

  tile.vertices[0] = x;
  tile.vertices[1] = y;
  tile.vertices[2] = (x + tile.cellw);
  tile.vertices[3] = y;
  tile.vertices[4] = (x + tile.cellh);
  tile.vertices[5] = (y + (float)info->lineinc);
  tile.vertices[6] = x;
  tile.vertices[7] = (y + (float)info->lineinc);

  // Screen culling
  //if (tile.vertices[0] >= vdp2width || tile.vertices[1] >= vdp2height || tile.vertices[2] < 0 || tile.vertices[5] < 0)
  //{
  //	return;
  //}
/*
  if ((info->bEnWin0 != 0 || info->bEnWin1 != 0) && info->coordincx == 1.0f && info->coordincy == 1.0f)
  {                                                 // coordinate inc is not supported yet.
    winmode = Vdp2CheckWindowRange(info, x - cx, y - cy, tile.cellw, info->lineinc);
    if (winmode == 0) // all outside, no need to draw
    {
      return;
    }
  }
*/
  tile.cor = info->cor;
  tile.cog = info->cog;
  tile.cob = info->cob;

  /*
    if (1 == YglIsCached(_Ygl->texture_manager, cacheaddr, &c))
    {
      YglCachedQuadOffset(&tile, &c, cx, cy, info->coordincx, info->coordincy);
      return;
    }

    YglQuadOffset(&tile, texture, &c, cx, cy, info->coordincx, info->coordincy);
    YglCacheAdd(_Ygl->texture_manager, cacheaddr, &c);
  */

  //genQuadVertex(&tile, texture, &c, cx, cy, info->coordincx, info->coordincy,0);


  if (tm->isCached(cacheaddr, &c)) {
    genPolygon((VdpPipeline**)info->pipeline, &tile, NULL, NULL, &c, 0);
    return;
  }

  genPolygon((VdpPipeline**)info->pipeline, &tile, texture, NULL, &c, 1);
  tm->addCache(cacheaddr, &c);

  switch (info->patternwh)
  {
  case 1:
    drawCell(info, texture);
    break;
  case 2:
    texture->w += 8;
    drawCell(info, texture);
    texture->textdata -= (texture->w + 8) * 8 - 8;
    drawCell(info, texture);
    texture->textdata -= 8;
    drawCell(info, texture);
    texture->textdata -= (texture->w + 8) * 8 - 8;
    drawCell(info, texture);
    break;
  }
}


void VIDVulkan::genQuadVertex(vdp2draw_struct * input, CharTexture * output, YglCache * c, int cx, int cy, float sx, float sy, int cash_flg) {
  unsigned int x, y;
  if (output != NULL) {
    tm->allocate(output, input->cellw, input->cellh, &x, &y);
  }
  else {
    x = c->x;
    y = c->y;
  }
}



void VIDVulkan::drawCell(vdp2draw_struct *info, CharTexture *texture)
{
  int i, j;

  // Access Denied(Wizardry - Llylgamyn Saga)
  if (info->char_bank[info->charaddr >> 17] == 0) {
    for (i = 0; i < info->cellh; i++)
    {
      for (j = 0; j < info->cellw; j++)
      {
        *texture->textdata++ = 0x00000000;
      }
      texture->textdata += texture->w;
    }
    return;
  }

  switch (info->colornumber)
  {
  case 0: // 4 BPP
    for (i = 0; i < info->cellh; i++)
    {
      for (j = 0; j < info->cellw; j += 4)
      {
        getPixel4bpp(info, info->charaddr, texture);
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
        getPixel8bpp(info, info->charaddr, texture);
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
        *texture->textdata++ = getPixel16bpp(info, info->charaddr);
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
        *texture->textdata++ = getPixel16bppbmp(info, info->charaddr);
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
        *texture->textdata++ = getPixel32bppbmp(info, info->charaddr);
        info->charaddr += 4;
      }
      texture->textdata += texture->w;
    }
    break;
  }
}


u32 VIDVulkan::getPixel4bpp(vdp2draw_struct *info, u32 addr, CharTexture *texture) {

  u32 cramindex;
  u16 dotw = T1ReadWord(Vdp2Ram, addr & 0x7FFFF);
  u8 dot;
  u32 alpha = 0xFF;

  alpha = info->alpha;
  dot = (dotw & 0xF000) >> 12;
  if (!(dot & 0xF) && info->transparencyenable) {
    *texture->textdata++ = 0x00000000;
  }
  else {
    cramindex = (info->coloroffset + ((info->paladdr << 4) | (dot & 0xF)));
    setSpecialPriority(info, dot, &cramindex);
    alpha = getAlpha(info, dot, cramindex);
    *texture->textdata++ = cramindex | alpha << 24;
  }

  alpha = info->alpha;
  dot = (dotw & 0xF00) >> 8;
  if (!(dot & 0xF) && info->transparencyenable) {
    *texture->textdata++ = 0x00000000;
  }
  else {
    cramindex = (info->coloroffset + ((info->paladdr << 4) | (dot & 0xF)));
    setSpecialPriority(info, dot, &cramindex);
    alpha = getAlpha(info, dot, cramindex);
    *texture->textdata++ = cramindex | alpha << 24;
  }

  alpha = info->alpha;
  dot = (dotw & 0xF0) >> 4;
  if (!(dot & 0xF) && info->transparencyenable) {
    *texture->textdata++ = 0x00000000;
  }
  else {
    cramindex = (info->coloroffset + ((info->paladdr << 4) | (dot & 0xF)));
    setSpecialPriority(info, dot, &cramindex);
    alpha = getAlpha(info, dot, cramindex);
    *texture->textdata++ = cramindex | alpha << 24;
  }

  alpha = info->alpha;
  dot = (dotw & 0xF);
  if (!(dot & 0xF) && info->transparencyenable) {
    *texture->textdata++ = 0x00000000;
  }
  else {
    cramindex = (info->coloroffset + ((info->paladdr << 4) | (dot & 0xF)));
    setSpecialPriority(info, dot, &cramindex);
    alpha = getAlpha(info, dot, cramindex);
    *texture->textdata++ = cramindex | alpha << 24;
  }
  return 0;
}

u32 VIDVulkan::getPixel8bpp(vdp2draw_struct *info, u32 addr, CharTexture *texture) {

  u32 cramindex;
  u16 dotw = T1ReadWord(Vdp2Ram, addr & 0x7FFFF);
  u8 dot;
  u32 alpha = info->alpha;

  alpha = info->alpha;
  dot = (dotw & 0xFF00) >> 8;
  if (!(dot & 0xFF) && info->transparencyenable) *texture->textdata++ = 0x00000000;
  else {
    cramindex = info->coloroffset + ((info->paladdr << 4) | (dot & 0xFF));
    setSpecialPriority(info, dot, &cramindex);
    alpha = getAlpha(info, dot, cramindex);
    *texture->textdata++ = cramindex | alpha << 24;
  }
  alpha = info->alpha;
  dot = (dotw & 0xFF);
  if (!(dot & 0xFF) && info->transparencyenable) *texture->textdata++ = 0x00000000;
  else {
    cramindex = info->coloroffset + ((info->paladdr << 4) | (dot & 0xFF));
    setSpecialPriority(info, dot, &cramindex);
    alpha = getAlpha(info, dot, cramindex);
    *texture->textdata++ = cramindex | alpha << 24;
  }
  return 0;
}


u32 VIDVulkan::getPixel16bpp(vdp2draw_struct *info, u32 addr) {
  u32 cramindex;
  u8 alpha = info->alpha;
  u16 dot = T1ReadWord(Vdp2Ram, addr & 0x7FFFF);
  if ((dot == 0) && info->transparencyenable) return 0x00000000;
  else {
    cramindex = info->coloroffset + dot;
    setSpecialPriority(info, dot, &cramindex);
    alpha = getAlpha(info, dot, cramindex);
    return   cramindex | alpha << 24;
  }
}

u32 VIDVulkan::getPixel16bppbmp(vdp2draw_struct *info, u32 addr) {
  u32 color;
  u16 dot = T1ReadWord(Vdp2Ram, addr & 0x7FFFF);
  if (!(dot & 0x8000) && info->transparencyenable) color = 0x00000000;
  else color = SAT2YAB1(info->alpha, dot);
  return color;
}

u32 VIDVulkan::getPixel32bppbmp(vdp2draw_struct *info, u32 addr) {
  u32 color;
  u16 dot1, dot2;
  dot1 = T1ReadWord(Vdp2Ram, addr & 0x7FFFF);
  dot2 = T1ReadWord(Vdp2Ram, addr + 2 & 0x7FFFF);
  if (!(dot1 & 0x8000) && info->transparencyenable) color = 0x00000000;
  else color = SAT2YAB2(info->alpha, dot1, dot2);
  return color;
}

u32 VIDVulkan::getAlpha(vdp2draw_struct *info, u8 dot, u32 cramindex) {
  u32 alpha = info->alpha;
  const int CCMD = ((fixVdp2Regs->CCCTL >> 8) & 0x01);  // hard/vdp2/hon/p12_14.htm#CCMD_
  if (CCMD == 0) {  // Calculate Rate mode
    switch (info->specialcolormode)
    {
    case 1: if (info->specialcolorfunction == 0) { alpha = 0xFF; } break;
    case 2:
      if (info->specialcolorfunction == 0) { alpha = 0xFF; }
      else { if ((info->specialcode & (1 << ((dot & 0xF) >> 1))) == 0) { alpha = 0xFF; } }
      break;
    case 3:
      if (((getRawColor(cramindex) & 0x8000) == 0)) { alpha = 0xFF; }
      break;
    }
  }
  else {  // Calculate Add mode
    alpha = 0xFF;
    switch (info->specialcolormode)
    {
    case 1:
      if (info->specialcolorfunction == 0) { alpha = 0x40; }
      break;
    case 2:
      if (info->specialcolorfunction == 0) { alpha = 0x40; }
      else { if ((info->specialcode & (1 << ((dot & 0xF) >> 1))) == 0) { alpha = 0x40; } }
      break;
    case 3:
      if (((getRawColor(cramindex) & 0x8000) == 0)) { alpha = 0x40; }
      break;
    }
  }
  return alpha;
}

//////////////////////////////////////////////////////////////////////////////
u16 VIDVulkan::getRawColor(u32 colorindex) {
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

void DynamicTexture::create(VIDVulkan * vulkan, int texWidth, int texHeight) {

  const VkDevice device = vulkan->getDevice();
  width = texWidth;
  height = texHeight;
  VkDeviceSize imageSize = texWidth * texHeight * 4;

  char * pixels = (char*)malloc(texWidth * texHeight * 4);
  memset(pixels, 0, texWidth * texHeight * 4);

  vulkan->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

  vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, (void**)&dynamicBuf);
  memcpy(dynamicBuf, pixels, static_cast<size_t>(imageSize));
  vkUnmapMemory(device, stagingBufferMemory);

  vulkan->createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, memory);
  vulkan->transitionImageLayout(image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  vulkan->copyBufferToImage(stagingBuffer, image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
  vulkan->transitionImageLayout(image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  //vkDestroyBuffer(device, stagingBuffer, nullptr);
  //vkFreeMemory(device, stagingBufferMemory, nullptr);
  free(pixels);

  VkImageViewCreateInfo viewInfo = {};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
    throw std::runtime_error("failed to create texture image view!");
  }


  VkSamplerCreateInfo samplerInfo = {};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_NEAREST;
  samplerInfo.minFilter = VK_FILTER_NEAREST;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable = VK_FALSE;
  //samplerInfo.maxAnisotropy = 16;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 0.0f;
  if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
    throw std::runtime_error("failed to create texture sampler!");
  }

  vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, (void**)&dynamicBuf);


}



void VIDVulkan::onUpdateColorRamWord(u32 addr) {
  YabThreadLock(crammutex);
  Vdp2ColorRamUpdated = 1;

  if (cram.dynamicBuf == NULL) {
    YabThreadUnLock(crammutex);
    return;
  }

  switch (Vdp2Internal.ColorMode)
  {
  case 0:
  case 1:
  {
    u16 tmp;
    u8 alpha = 0;
    tmp = T2ReadWord(Vdp2ColorRam, addr);
    if (tmp & 0x8000) alpha = 0xFF;
    cram.dynamicBuf[(addr >> 1) & 0x7FF] = SAT2YAB1(alpha, tmp);
    break;
  }
  case 2:
  {
    u32 tmp1 = T2ReadWord(Vdp2ColorRam, (addr & 0xFFC));
    u32 tmp2 = T2ReadWord(Vdp2ColorRam, (addr & 0xFFC) + 2);
    u8 alpha = 0;
    if (tmp1 & 0x8000) alpha = 0xFF;
    cram.dynamicBuf[(addr >> 2) & 0x7FF] = SAT2YAB2(alpha, tmp1, tmp2);
    break;
  }
  default:
    break;
  }
  YabThreadUnLock(crammutex);

}

void VIDVulkan::updateColorRam(VkCommandBuffer commandBuffer) {
  YabThreadLock(crammutex);
  if (Vdp2ColorRamUpdated) {
    cram.update(this, commandBuffer);
  }
  YabThreadUnLock(crammutex);
}


void DynamicTexture::update(VIDVulkan * vulkan, VkCommandBuffer commandBuffer) {
  vkUnmapMemory(vulkan->getDevice(), stagingBufferMemory);

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  VkImageLayout newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

  VkImageMemoryBarrier barrier = {};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

  vkCmdPipelineBarrier(
    commandBuffer,
    sourceStage, destinationStage,
    0,
    0, nullptr,
    0, nullptr,
    1, &barrier
  );

  //------------------------------------------------------------------------
  VkBufferImageCopy region = {};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;

  region.imageOffset = { 0, 0, 0 };
  region.imageExtent = {
    (uint32_t)width,
    (uint32_t)height,
    1
  };
  vkCmdCopyBufferToImage(
    commandBuffer,
    stagingBuffer,
    image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1,
    &region
  );

  VkImageMemoryBarrier destbarrier = {};
  destbarrier = barrier;
  destbarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  destbarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  destbarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  destbarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  destbarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

  vkCmdPipelineBarrier(
    commandBuffer,
    sourceStage, destinationStage,
    0,
    0, nullptr,
    0, nullptr,
    1, &destbarrier
  );
  VkDeviceSize imageSize = width * height * 4;
  vkMapMemory(vulkan->getDevice(), stagingBufferMemory, 0, imageSize, 0, (void**)&dynamicBuf);

}

/*
struct OffscreenPass {
  int32_t width, height;
  VkFramebuffer frameBuffer;
  VkImage image;
  VkDeviceMemory mem;
  VkImageView view;
  VkRenderPass renderPass;
  VkSampler sampler;
  VkDescriptorImageInfo descriptor;
} offscreenPass;

void generateOffscreenPath(int width, int height);
void deleteOfscreenPath();
*/

void VIDVulkan::generateOffscreenPath(int width, int height) {
  VkDevice device = getDevice();
  VkPhysicalDevice physicalDevice = getPhysicalDevice();

  if (offscreenPass.width == width && offscreenPass.height == height) return;

  deleteOfscreenPath();

  offscreenPass.width = width;
  offscreenPass.height = height;

  // Color attachment
  VkImageCreateInfo image = {};
  image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image.imageType = VK_IMAGE_TYPE_2D;
  image.format = VK_FORMAT_R8G8B8A8_UNORM;
  image.extent.width = offscreenPass.width;
  image.extent.height = offscreenPass.height;
  image.extent.depth = 1;
  image.mipLevels = 1;
  image.arrayLayers = 1;
  image.samples = VK_SAMPLE_COUNT_1_BIT;
  image.tiling = VK_IMAGE_TILING_OPTIMAL;
  // We will sample directly from the color attachment
  image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

  VkMemoryAllocateInfo memAlloc = {};
  memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

  VkMemoryRequirements memReqs;

  VK_CHECK_RESULT(vkCreateImage(device, &image, nullptr, &offscreenPass.image));
  vkGetImageMemoryRequirements(device, offscreenPass.image, &memReqs);
  memAlloc.allocationSize = memReqs.size;
  memAlloc.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &offscreenPass.mem));
  VK_CHECK_RESULT(vkBindImageMemory(device, offscreenPass.image, offscreenPass.mem, 0));

  VkImageViewCreateInfo colorImageView = {};
  colorImageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
  colorImageView.format = VK_FORMAT_R8G8B8A8_UNORM;
  colorImageView.subresourceRange = {};
  colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  colorImageView.subresourceRange.baseMipLevel = 0;
  colorImageView.subresourceRange.levelCount = 1;
  colorImageView.subresourceRange.baseArrayLayer = 0;
  colorImageView.subresourceRange.layerCount = 1;
  colorImageView.image = offscreenPass.image;
  VK_CHECK_RESULT(vkCreateImageView(device, &colorImageView, nullptr, &offscreenPass.view));


  // Create sampler to sample from the attachment in the fragment shader
  VkSamplerCreateInfo samplerInfo = {};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_NEAREST;
  samplerInfo.minFilter = VK_FILTER_NEAREST;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeV = samplerInfo.addressModeU;
  samplerInfo.addressModeW = samplerInfo.addressModeU;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.maxAnisotropy = 1.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 1.0f;
  samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  VK_CHECK_RESULT(vkCreateSampler(device, &samplerInfo, nullptr, &offscreenPass.sampler));


  std::array<VkAttachmentDescription, 1> attchmentDescriptions = {};
  // Color attachment
  attchmentDescriptions[0].format = VK_FORMAT_R8G8B8A8_UNORM;
  attchmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
  attchmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attchmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attchmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attchmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attchmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attchmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

  VkSubpassDescription subpassDescription = {};
  subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpassDescription.colorAttachmentCount = 1;
  subpassDescription.pColorAttachments = &colorReference;
  subpassDescription.pDepthStencilAttachment = nullptr;

  // Use subpass dependencies for layout transitions
  std::array<VkSubpassDependency, 2> dependencies;

  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  dependencies[1].srcSubpass = 0;
  dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;


  // Create the actual renderpass
  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = static_cast<uint32_t>(attchmentDescriptions.size());
  renderPassInfo.pAttachments = attchmentDescriptions.data();

  //renderPassInfo.subpassCount = 0;
  //renderPassInfo.pSubpasses = VK_NULL_HANDLE; // Optional


  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpassDescription;
  renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
  renderPassInfo.pDependencies = dependencies.data();

  VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &offscreenPass.renderPass));



  VkImageView attachments[1];
  attachments[0] = offscreenPass.view;

  VkFramebufferCreateInfo fbufCreateInfo = {};
  fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  fbufCreateInfo.renderPass = offscreenPass.renderPass;
  fbufCreateInfo.attachmentCount = 1;
  fbufCreateInfo.pAttachments = attachments;
  fbufCreateInfo.width = offscreenPass.width;
  fbufCreateInfo.height = offscreenPass.height;
  fbufCreateInfo.layers = 1;
  VK_CHECK_RESULT(vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &offscreenPass.frameBuffer));

#if 0
  VkSemaphoreCreateInfo semaphore_create_info{};
  semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  vkCreateSemaphore(device, &semaphore_create_info, nullptr, &offscreenPass.color[0]._render_complete_semaphore);
  vkCreateSemaphore(device, &semaphore_create_info, nullptr, &offscreenPass.color[1]._render_complete_semaphore);
#endif

  // Fill a descriptor for later use in a descriptor set
  offscreenPass.descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  offscreenPass.descriptor.imageView = offscreenPass.view;
  offscreenPass.descriptor.sampler = offscreenPass.sampler;


}

void VIDVulkan::generateOffscreenRenderer() {
  std::vector<Vertex> vertices;
  std::vector<uint16_t> indices;

  Vertex a(glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
  vertices.push_back(a);
  Vertex b(glm::vec4(1.0f, -1.0f, 0.0f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
  vertices.push_back(b);
  Vertex c(glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
  vertices.push_back(c);
  Vertex d(glm::vec4(-1.0f, 1.0f, 0.0f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
  vertices.push_back(d);

  indices.push_back(0);
  indices.push_back(1);
  indices.push_back(2);
  indices.push_back(2);
  indices.push_back(3);
  indices.push_back(0);


  const VkDevice device = getDevice();

  // VertexBuffer
  VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  createBuffer(bufferSize,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    stagingBuffer, stagingBufferMemory);

  void* data;
  vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, vertices.data(), (size_t)bufferSize);
  vkUnmapMemory(device, stagingBufferMemory);

  createBuffer(bufferSize,
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    offscreenRenderer.vertexBuffer, offscreenRenderer.vertexBufferMemory);

  copyBuffer(stagingBuffer, offscreenRenderer.vertexBuffer, bufferSize);

  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);


  // IndexBuffer
  bufferSize = sizeof(indices[0]) * indices.size();

  createBuffer(bufferSize,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    stagingBuffer,
    stagingBufferMemory);

  vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, indices.data(), (size_t)bufferSize);
  vkUnmapMemory(device, stagingBufferMemory);

  createBuffer(bufferSize,
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, offscreenRenderer.indexBuffer, offscreenRenderer.indexBufferMemory);

  copyBuffer(stagingBuffer, offscreenRenderer.indexBuffer, bufferSize);

  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);

  offscreenRenderer.blit = new VdpPipelineBlit(this, tm, vm);
  offscreenRenderer.blit->setRenderPass(_renderer->getWindow()->GetVulkanKeepRenderPass());
  offscreenRenderer.blit->createGraphicsPipeline();

  offscreenRenderer.mosaic = new VdpPipelineMosaic(this, tm, vm);
  offscreenRenderer.mosaic->setRenderPass(_renderer->getWindow()->GetVulkanKeepRenderPass());
  offscreenRenderer.mosaic->createGraphicsPipeline();

#if 0
  offscreenRenderer.perline = new VdpPipelinePerLine(this, tm, vm);
  offscreenRenderer.perline->setRenderPass(_renderer->getWindow()->GetVulkanKeepRenderPass());
  offscreenRenderer.perline->createGraphicsPipeline();

  offscreenRenderer.perlineDestination = new VdpPipelinePerLineDst(this, tm, vm);
  offscreenRenderer.perlineDestination->setRenderPass(_renderer->getWindow()->GetVulkanKeepRenderPass());
  offscreenRenderer.perlineDestination->createGraphicsPipeline();
#endif

}

void VIDVulkan::deleteOfscreenPath() {
  VkDevice device = getDevice();
  if (offscreenPass.frameBuffer != VK_NULL_HANDLE) {
    vkDestroySampler(device, offscreenPass.sampler, nullptr);
    offscreenPass.sampler = VK_NULL_HANDLE;
    vkDestroyImage(device, offscreenPass.image, nullptr);
    offscreenPass.image = VK_NULL_HANDLE;
    vkFreeMemory(device, offscreenPass.mem, nullptr);
    offscreenPass.mem = VK_NULL_HANDLE;
    vkDestroyImageView(device, offscreenPass.view, nullptr);
    offscreenPass.view = VK_NULL_HANDLE;
    vkDestroyFramebuffer(device, offscreenPass.frameBuffer, nullptr);
    offscreenPass.frameBuffer = VK_NULL_HANDLE;
  }
}


void VIDVulkan::renderToOffecreenTarget(VkCommandBuffer commandBuffer, VdpPipeline * render) {

  std::array<VkClearValue, 1> clear_values{};
  clear_values[0].color.float32[0] = 0.0f;
  clear_values[0].color.float32[1] = 0.0f;
  clear_values[0].color.float32[2] = 0.0f;
  clear_values[0].color.float32[3] = 0.0f;

  VkRenderPassBeginInfo renderPassBeginInfo = {};
  renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassBeginInfo.renderPass = offscreenPass.renderPass;
  renderPassBeginInfo.framebuffer = offscreenPass.frameBuffer;
  renderPassBeginInfo.renderArea.extent.width = offscreenPass.width;
  renderPassBeginInfo.renderArea.extent.height = offscreenPass.height;
  renderPassBeginInfo.clearValueCount = 1;
  renderPassBeginInfo.pClearValues = clear_values.data();

  vkCmdEndRenderPass(commandBuffer);

  vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

  auto c = vk::CommandBuffer(commandBuffer);
  vk::Viewport viewport;
  vk::Rect2D scissor;

  viewport.width = offscreenPass.width;
  viewport.height = offscreenPass.height;
  viewport.x = 0;
  viewport.y = 0;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  c.setViewport(0, 1, &viewport);

  scissor.extent.width = offscreenPass.width;
  scissor.extent.height = offscreenPass.height;
  scissor.offset.x = 0;
  scissor.offset.y = 0;
  c.setScissor(0, 1, &scissor);


  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
    render->getPipelineLayout(), 0, 1, &render->_descriptorSet, 0, nullptr);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render->getGraphicsPipeline());

  VkBuffer vertexBuffers[] = { vm->getVertexBuffer(render->vectexBlock) };
  VkDeviceSize offsets[] = { render->vertexOffset };

  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

  vkCmdBindIndexBuffer(commandBuffer,
    vm->getIndexBuffer(render->vectexBlock),
    render->indexOffset, VK_INDEX_TYPE_UINT16);

  vkCmdDrawIndexed(commandBuffer, render->indexSize, 1, 0, 0, 0);

  c.endRenderPass();


}


void VIDVulkan::renderWithLineEffectToMainTarget(VdpPipeline *p, VkCommandBuffer commandBuffer, const UniformBufferObject & ubo, VkImageView lineinfo) {

  UniformBufferObject ubomini = ubo;
  VkRect2D render_area{};
  render_area.offset.x = 0;
  render_area.offset.y = 0;
  render_area.extent = _renderer->getWindow()->GetVulkanSurfaceSize();

  VkRenderPassBeginInfo render_pass_begin_info{};
  render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_begin_info.renderPass = _renderer->getWindow()->GetVulkanKeepRenderPass();
  render_pass_begin_info.framebuffer = _renderer->getWindow()->GetVulkanActiveFramebuffer();
  render_pass_begin_info.renderArea = render_area;
  render_pass_begin_info.clearValueCount = 0;
  render_pass_begin_info.pClearValues = nullptr;

  vkCmdBeginRenderPass(commandBuffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

  auto c = vk::CommandBuffer(commandBuffer);
  vk::Viewport viewport;
  vk::Rect2D scissor;
  if (resolutionMode != RES_NATIVE) {
    viewport.width = renderWidth;
    viewport.height = renderHeight;
    viewport.x = 0;
    viewport.y = 0;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    c.setViewport(0, 1, &viewport);

    scissor.extent.width = renderWidth;
    scissor.extent.height = renderHeight;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    c.setScissor(0, 1, &scissor);

  }
  else {
    viewport.width = renderWidth;
    viewport.height = renderHeight;
    viewport.x = originx;
    viewport.y = originy;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    c.setViewport(0, 1, &viewport);

    scissor.extent.width = renderWidth;
    scissor.extent.height = renderHeight;
    scissor.offset.x = originx;
    scissor.offset.y = originy;
    c.setScissor(0, 1, &scissor);
  }

  p->setUBO(&ubomini, sizeof(ubomini));
  p->setSampler(VdpPipeline::bindIdTexture, offscreenPass.view, offscreenPass.sampler);
  p->setSampler(VdpPipeline::bindIdLine, lineinfo, offscreenPass.sampler);
  p->setSampler(VdpPipeline::bindIdWindow, windowRenderer->getImageView(), windowRenderer->getSampler());
  p->updateDescriptorSets();

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
    p->getPipelineLayout(), 0, 1, &p->_descriptorSet, 0, nullptr);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p->getGraphicsPipeline());

  VkBuffer vertexBuffers[] = { offscreenRenderer.vertexBuffer };
  VkDeviceSize offsets[] = { 0 };

  vkCmdBindVertexBuffers(commandBuffer,
    0, 1, vertexBuffers, offsets);

  vkCmdBindIndexBuffer(commandBuffer,
    offscreenRenderer.indexBuffer,
    0, VK_INDEX_TYPE_UINT16);

  vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);

}

void VIDVulkan::renderEffectToMainTarget(VkCommandBuffer commandBuffer, const UniformBufferObject & ubo, int mode) {

  VkRect2D render_area{};
  render_area.offset.x = 0;
  render_area.offset.y = 0;
  render_area.extent = _renderer->getWindow()->GetVulkanSurfaceSize();

  VkRenderPassBeginInfo render_pass_begin_info{};
  render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_begin_info.renderPass = _renderer->getWindow()->GetVulkanKeepRenderPass();
  render_pass_begin_info.framebuffer = _renderer->getWindow()->GetVulkanActiveFramebuffer();
  render_pass_begin_info.renderArea = render_area;
  render_pass_begin_info.clearValueCount = 0;
  render_pass_begin_info.pClearValues = nullptr;

  vkCmdBeginRenderPass(commandBuffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

  auto c = vk::CommandBuffer(commandBuffer);
  vk::Viewport viewport;
  vk::Rect2D scissor;
  if (resolutionMode != RES_NATIVE) {
    viewport.width = renderWidth;
    viewport.height = renderHeight;
    viewport.x = 0;
    viewport.y = 0;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    c.setViewport(0, 1, &viewport);

    scissor.extent.width = renderWidth;
    scissor.extent.height = renderHeight;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    c.setScissor(0, 1, &scissor);

  }
  else {
    viewport.width = renderWidth;
    viewport.height = renderHeight;
    viewport.x = originx;
    viewport.y = originy;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    c.setViewport(0, 1, &viewport);

    scissor.extent.width = renderWidth;
    scissor.extent.height = renderHeight;
    scissor.offset.x = originx;
    scissor.offset.y = originy;
    c.setScissor(0, 1, &scissor);
  }

  if (mode == 0) {
    int dmyubo = 0;
    offscreenRenderer.blit->setUBO(&dmyubo, sizeof(dmyubo));
    offscreenRenderer.blit->setSampler(VdpPipeline::bindIdTexture, offscreenPass.view, offscreenPass.sampler);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      offscreenRenderer.blit->getPipelineLayout(), 0, 1, &offscreenRenderer.blit->_descriptorSet, 0, nullptr);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, offscreenRenderer.blit->getGraphicsPipeline());
  }
  else if (mode == 1) {

    offscreenRenderer.mosaic->setUBO(&ubo, sizeof(ubo));
    offscreenRenderer.mosaic->setSampler(VdpPipeline::bindIdTexture, offscreenPass.view, offscreenPass.sampler);
    offscreenRenderer.mosaic->setSampler(VdpPipeline::bindIdWindow, windowRenderer->getImageView(), windowRenderer->getSampler());
    offscreenRenderer.mosaic->updateDescriptorSets();

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      offscreenRenderer.mosaic->getPipelineLayout(), 0, 1, &offscreenRenderer.mosaic->_descriptorSet, 0, nullptr);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, offscreenRenderer.mosaic->getGraphicsPipeline());

  }

  VkBuffer vertexBuffers[] = { offscreenRenderer.vertexBuffer };
  VkDeviceSize offsets[] = { 0 };

  vkCmdBindVertexBuffers(commandBuffer,
    0, 1, vertexBuffers, offsets);

  vkCmdBindIndexBuffer(commandBuffer,
    offscreenRenderer.indexBuffer,
    0, VK_INDEX_TYPE_UINT16);

  vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);

}

void VIDVulkan::generateSubRenderTarget(int width, int height) {

  VkDevice device = getDevice();
  VkPhysicalDevice physicalDevice = getPhysicalDevice();

  subRenderTarget.width = width;
  subRenderTarget.height = height;

  // Find a suitable depth format
  VkFormat fbDepthFormat;
  VkBool32 validDepthFormat = vks::tools::getSupportedDepthFormat(physicalDevice, &fbDepthFormat);
  assert(validDepthFormat);

  // Color attachment
  VkImageCreateInfo image = vks::initializers::imageCreateInfo();
  image.imageType = VK_IMAGE_TYPE_2D;
  image.format = VK_FORMAT_R8G8B8A8_UNORM;
  image.extent.width = subRenderTarget.width;
  image.extent.height = subRenderTarget.height;
  image.extent.depth = 1;
  image.mipLevels = 1;
  image.arrayLayers = 1;
  image.samples = VK_SAMPLE_COUNT_1_BIT;
  image.tiling = VK_IMAGE_TILING_OPTIMAL;
  // We will sample directly from the color attachment
  image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

  VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
  VkMemoryRequirements memReqs;

  VK_CHECK_RESULT(vkCreateImage(device, &image, nullptr, &subRenderTarget.color.image));
  vkGetImageMemoryRequirements(device, subRenderTarget.color.image, &memReqs);
  memAlloc.allocationSize = memReqs.size;
  memAlloc.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &subRenderTarget.color.mem));
  VK_CHECK_RESULT(vkBindImageMemory(device, subRenderTarget.color.image, subRenderTarget.color.mem, 0));

  VkImageViewCreateInfo colorImageView = vks::initializers::imageViewCreateInfo();
  colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
  colorImageView.format = VK_FORMAT_R8G8B8A8_UNORM;
  colorImageView.subresourceRange = {};
  colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  colorImageView.subresourceRange.baseMipLevel = 0;
  colorImageView.subresourceRange.levelCount = 1;
  colorImageView.subresourceRange.baseArrayLayer = 0;
  colorImageView.subresourceRange.layerCount = 1;
  colorImageView.image = subRenderTarget.color.image;
  VK_CHECK_RESULT(vkCreateImageView(device, &colorImageView, nullptr, &subRenderTarget.color.view));


  // Create sampler to sample from the attachment in the fragment shader
  VkSamplerCreateInfo samplerInfo = vks::initializers::samplerCreateInfo();
  samplerInfo.magFilter = VK_FILTER_NEAREST;
  samplerInfo.minFilter = VK_FILTER_NEAREST;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeV = samplerInfo.addressModeU;
  samplerInfo.addressModeW = samplerInfo.addressModeU;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.maxAnisotropy = 1.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 1.0f;
  samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  VK_CHECK_RESULT(vkCreateSampler(device, &samplerInfo, nullptr, &subRenderTarget.sampler));

  // Depth stencil attachment
  image.format = fbDepthFormat;
  image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

  VK_CHECK_RESULT(vkCreateImage(device, &image, nullptr, &subRenderTarget.depth.image));
  vkGetImageMemoryRequirements(device, subRenderTarget.depth.image, &memReqs);
  memAlloc.allocationSize = memReqs.size;
  memAlloc.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &subRenderTarget.depth.mem));
  VK_CHECK_RESULT(vkBindImageMemory(device, subRenderTarget.depth.image, subRenderTarget.depth.mem, 0));

  VkImageViewCreateInfo depthStencilView = vks::initializers::imageViewCreateInfo();
  depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
  depthStencilView.format = fbDepthFormat;
  depthStencilView.flags = 0;
  depthStencilView.subresourceRange = {};
  depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
  depthStencilView.subresourceRange.baseMipLevel = 0;
  depthStencilView.subresourceRange.levelCount = 1;
  depthStencilView.subresourceRange.baseArrayLayer = 0;
  depthStencilView.subresourceRange.layerCount = 1;
  depthStencilView.image = subRenderTarget.depth.image;
  VK_CHECK_RESULT(vkCreateImageView(device, &depthStencilView, nullptr, &subRenderTarget.depth.view));

  // Create a separate render pass for the offscreen rendering as it may differ from the one used for scene rendering

  std::array<VkAttachmentDescription, 2> attchmentDescriptions = {};
  // Color attachment
  attchmentDescriptions[0].format = VK_FORMAT_R8G8B8A8_UNORM;
  attchmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
  attchmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attchmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attchmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attchmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attchmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attchmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  // Depth attachment
  attchmentDescriptions[1].format = fbDepthFormat;
  attchmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
  attchmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attchmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attchmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  attchmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
  attchmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attchmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_GENERAL/*VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/ };
  VkAttachmentReference depthReference = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

  VkSubpassDescription subpassDescription = {};
  subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpassDescription.colorAttachmentCount = 1;
  subpassDescription.pColorAttachments = &colorReference;
  subpassDescription.pDepthStencilAttachment = &depthReference;

  // Use subpass dependencies for layout transitions
  std::array<VkSubpassDependency, 3> dependencies;

  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  dependencies[1].srcSubpass = 0;
  dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  dependencies[2].srcSubpass = 0;
  dependencies[2].dstSubpass = 0;
  dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[2].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[2].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  // Create the actual renderpass
  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = static_cast<uint32_t>(attchmentDescriptions.size());
  renderPassInfo.pAttachments = attchmentDescriptions.data();

  //renderPassInfo.subpassCount = 0;
  //renderPassInfo.pSubpasses = VK_NULL_HANDLE; // Optional


  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpassDescription;
  renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
  renderPassInfo.pDependencies = dependencies.data();

  VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &subRenderTarget.renderPass));

  VkImageView attachments[2];
  attachments[0] = subRenderTarget.color.view;
  attachments[1] = subRenderTarget.depth.view;

  VkFramebufferCreateInfo fbufCreateInfo = vks::initializers::framebufferCreateInfo();
  fbufCreateInfo.renderPass = subRenderTarget.renderPass;
  fbufCreateInfo.attachmentCount = 2;
  fbufCreateInfo.pAttachments = attachments;
  fbufCreateInfo.width = subRenderTarget.width;
  fbufCreateInfo.height = subRenderTarget.height;
  fbufCreateInfo.layers = 1;
  VK_CHECK_RESULT(vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &subRenderTarget.frameBuffer));


  VkSemaphoreCreateInfo semaphore_create_info{};
  semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  vkCreateSemaphore(device, &semaphore_create_info, nullptr, &subRenderTarget.color._render_complete_semaphore);

  // Fill a descriptor for later use in a descriptor set
  subRenderTarget.descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  subRenderTarget.descriptor.imageView = subRenderTarget.color.view;
  subRenderTarget.descriptor.sampler = subRenderTarget.sampler;

}

void VIDVulkan::freeSubRenderTarget() {

  vkQueueWaitIdle(getVulkanQueue());
  VkDevice device = getDevice();

  if (subRenderTarget.sampler != VK_NULL_HANDLE) {
    vkDestroySampler(device, subRenderTarget.sampler, nullptr); subRenderTarget.sampler = VK_NULL_HANDLE;
  }
  if (subRenderTarget.color.image != VK_NULL_HANDLE) {
    vkDestroyImage(device, subRenderTarget.color.image, nullptr); subRenderTarget.color.image = VK_NULL_HANDLE;
  }
  if (subRenderTarget.color.mem != VK_NULL_HANDLE) {
    vkFreeMemory(device, subRenderTarget.color.mem, nullptr); subRenderTarget.color.mem = VK_NULL_HANDLE;
  }
  if (subRenderTarget.color.view != VK_NULL_HANDLE) {
    vkDestroyImageView(device, subRenderTarget.color.view, nullptr); subRenderTarget.color.view = VK_NULL_HANDLE;
  }
  if (subRenderTarget.depth.image != VK_NULL_HANDLE) {
    vkDestroyImage(device, subRenderTarget.depth.image, nullptr); subRenderTarget.depth.image = VK_NULL_HANDLE;
  }
  if (subRenderTarget.depth.mem != VK_NULL_HANDLE) {
    vkFreeMemory(device, subRenderTarget.depth.mem, nullptr); subRenderTarget.depth.mem = VK_NULL_HANDLE;
  }
  if (subRenderTarget.depth.view != VK_NULL_HANDLE) {
    vkDestroyImageView(device, subRenderTarget.depth.view, nullptr); subRenderTarget.depth.view = VK_NULL_HANDLE;
  }
  if (subRenderTarget.frameBuffer != VK_NULL_HANDLE) {
    vkDestroyFramebuffer(device, subRenderTarget.frameBuffer, nullptr); subRenderTarget.frameBuffer = VK_NULL_HANDLE;
  }

  if (subRenderTarget.renderPass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(device, subRenderTarget.renderPass, nullptr); subRenderTarget.renderPass = VK_NULL_HANDLE;
  }


}

void VIDVulkan::blitSubRenderTarget(VkCommandBuffer commandBuffer) {
  VkDevice device = getDevice();
  VkImage dstImage = _renderer->getWindow()->getCurrentImage();

  // Transition destination image to transfer destination layout
  vks::tools::insertImageMemoryBarrier(
    commandBuffer,
    dstImage,
    0,
    VK_ACCESS_TRANSFER_WRITE_BIT,
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });


  // Transition swapchain image from present to transfer source layout
  vks::tools::insertImageMemoryBarrier(
    commandBuffer,
    subRenderTarget.color.image,
    VK_ACCESS_MEMORY_READ_BIT,
    VK_ACCESS_TRANSFER_READ_BIT,
    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

  // Define the region to blit (we will blit the whole swapchain image)
  VkImageBlit imageBlitRegion{};
  imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageBlitRegion.srcSubresource.layerCount = 1;
  imageBlitRegion.srcOffsets[1].x = subRenderTarget.width;
  imageBlitRegion.srcOffsets[1].y = subRenderTarget.height;
  imageBlitRegion.srcOffsets[1].z = 1;
  imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageBlitRegion.dstSubresource.layerCount = 1;
  imageBlitRegion.dstOffsets[0].x = originx;
  imageBlitRegion.dstOffsets[0].y = originy;
  imageBlitRegion.dstOffsets[0].z = 0;
  imageBlitRegion.dstOffsets[1].x = originx + finalWidth;
  imageBlitRegion.dstOffsets[1].y = originy + finalHeight;
  imageBlitRegion.dstOffsets[1].z = 1;

  // Issue the blit command
  vkCmdBlitImage(
    commandBuffer,
    subRenderTarget.color.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1,
    &imageBlitRegion,
    VK_FILTER_NEAREST);


  // Transition destination image to general layout, which is the required layout for mapping the image memory later on
  vks::tools::insertImageMemoryBarrier(
    commandBuffer,
    dstImage,
    VK_ACCESS_TRANSFER_WRITE_BIT,
    VK_ACCESS_MEMORY_READ_BIT,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_IMAGE_LAYOUT_GENERAL,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

  // Transition back the swap chain image after the blit is done
  vks::tools::insertImageMemoryBarrier(
    commandBuffer,
    subRenderTarget.color.image,
    VK_ACCESS_TRANSFER_READ_BIT,
    VK_ACCESS_MEMORY_READ_BIT,
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });


}