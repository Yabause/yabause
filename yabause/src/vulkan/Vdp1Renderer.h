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
class VIDVulkan;
class TextureManager;
class VertexManager;
class CharTexture;
class TextureCache;
class VdpPipelineFactory;
class VdpPipeline;

#include <vector>  
using std::vector;

class Vdp1Renderer {


  // Framebuffer for offscreen rendering
  struct FrameBufferAttachment {
    VkImage image;
    VkDeviceMemory mem;
    VkImageView view;
    VkSemaphore _render_complete_semaphore;
    bool updated;
    bool readed;
  };
  struct OffscreenPass {
    int32_t width, height;
    VkFramebuffer frameBuffer[2];
    FrameBufferAttachment color[2], depth;
    VkRenderPass renderPass;
    VkSampler sampler;
    VkDescriptorImageInfo descriptor;
  } offscreenPass;

  struct vdp1Ubo {
    glm::mat4 model;
    glm::vec2 texsize;
    int u_fbowidth;
    int u_fbohegiht;
    float TessLevelInner;
    float TessLevelOuter;
  };

public:
  Vdp1Renderer(int width, int height, VIDVulkan * vulkan);
  ~Vdp1Renderer();

  void setUp();

  bool updated = false;

  Vdp2 * fixVdp2Regs = NULL;
  float vdp1wratio = 1.0f;
  float vdp1hratio = 1.0f;

  void drawStart(void);
  void drawEnd(void);
  void NormalSpriteDraw(u8 * ram, Vdp1 * regs, u8 * back_framebuffer);
  void ScaledSpriteDraw(u8 * ram, Vdp1 * regs, u8 * back_framebuffer);
  void DistortedSpriteDraw(u8 * ram, Vdp1 * regs, u8 * back_framebuffer);
  void PolygonDraw(u8 * ram, Vdp1 * regs, u8 * back_framebuffer);
  void PolylineDraw(u8 * ram, Vdp1 * regs, u8 * back_framebuffer);
  void LineDraw(u8 * ram, Vdp1 * regs, u8 * back_framebuffer);
  void UserClipping(u8 * ram, Vdp1 * regs);
  void SystemClipping(u8 * ram, Vdp1 * regs);
  void LocalCoordinate(u8 * ram, Vdp1 * regs);

  void erase();
  void change();

  void setTextureRatio(int vdp2widthratio, int vdp2heightratio);

  VkImageView getFrameBufferImage() {
    blitCpuWrittenFramebuffer(readframe);
    return offscreenPass.color[readframe].view;
  }
  VkSemaphore getFrameBufferSem() {
    if (offscreenPass.color[readframe].updated) {
      offscreenPass.color[readframe].updated = false;
      return offscreenPass.color[readframe]._render_complete_semaphore;
    }
    else {
      return VK_NULL_HANDLE;
    }
  }

  void changeResolution(int width, int height);
  void setVdp2Resolution(int width, int height) {
    vdp2Width = width;
    vdp2Height = height;
  }

  void readFrameBuffer(u32 type, u32 addr, void * out);

  void writeFrameBuffer(u32 type, u32 addr, u32 val);

  int getMsbShadowCount() {
    return msbShadowCount[readframe];
  }

  void setPolygonMode(POLYGONMODE p) {
    proygonMode = p;
  }

protected:

  VIDVulkan * vulkan;
  TextureManager * tm;
  VertexManager  * vm;
  VdpPipelineFactory * pipleLineFactory;

  int readframe = 0;
  int drawframe = 1;

  VdpPipeline *currentPipeLine = nullptr;

  VkBuffer _uniformBuffer;
  VkDeviceMemory _uniformBufferMemory;

  vector<VdpPipeline*> piplelines;

  void createCommandPool();
  VkCommandPool _command_pool = VK_NULL_HANDLE;
  std::vector<VkCommandBuffer> _command_buffers;
  uint32_t _current_command_buffer = 0;

  VkCommandBuffer getNextCommandBuffer() {
    VkCommandBuffer rtn = _command_buffers[_current_command_buffer];
    _current_command_buffer += 1;
    if (_current_command_buffer >= _command_buffers.size()) {
      _current_command_buffer = 0;
    }
    return rtn;
  }

  uint32_t _current_frame = 0;

  

  VkSubmitInfo submitInfo;

  s16 localX;
  s16 localY;

  void createUniformBuffer();
  void prepareOffscreen();

  int width;
  int height;
  int vdp2Width;
  int vdp2Height;

  void readPriority(vdp1cmd_struct *cmd, int * priority, int * colorcl, int * normal_shadow);
  void readTexture(vdp1cmd_struct *cmd, YglSprite *sprite, CharTexture *texture);
  u32 readPolygonColor(vdp1cmd_struct *cmd);

  int genPolygon(YglSprite * input, CharTexture * output, float * colors, TextureCache * c, int cash_flg);

  void makeLinePolygon(s16 *v1, s16 *v2, float *outv);

  void genClearPipeline();

  struct ClearUbo {
    glm::vec4 clearColor;
  } clearUbo;


  std::string get_shader_header() {
#if defined(ANDROID)
    return "#version 310 es\n precision highp float; \n precision highp int;\n";
#else
    return "#version 450\n";
#endif
  }


  VkBuffer _vertexBuffer;
  VkDeviceMemory _vertexBufferMemory;
  VkBuffer _indexBuffer;
  VkDeviceMemory _indexBufferMemory;
  VkBuffer _clearUniformBuffer;
  VkDeviceMemory _clearUniformBufferMemory;
  VkDescriptorSet _descriptorSet;
  VkDescriptorSetLayout _descriptorSetLayout;
  VkDescriptorPool _descriptorPool;
  VkShaderModule _vertShaderModule;
  VkShaderModule _fragShaderModule;
  VkPipelineLayout _pipelineLayout;
  VkPipeline _graphicsPipeline;

  Vdp2 baseVdp2Regs;
  void * frameBuffer;

  VkImage dstDeviceImage = VK_NULL_HANDLE;
  VkDeviceMemory dstDeviceImageMemory = VK_NULL_HANDLE;
  VkImage dstImage = VK_NULL_HANDLE;
  VkDeviceMemory dstImageMemory = VK_NULL_HANDLE;
  int dstWidth = -1;
  int dstHeight = -1;

  int cpuFramebufferWriteCount[2];
  uint32_t * cpuWriteBuffer = nullptr;
  int cpuWidth = -1;
  int cpuHeight = -1;

  VkImage writeDeviceImage = VK_NULL_HANDLE;
  VkDeviceMemory writeDeviceImageMemory = VK_NULL_HANDLE;
  VkImage writeImage = VK_NULL_HANDLE;
  VkDeviceMemory writeImageMemory = VK_NULL_HANDLE;
  int writeWidth = -1;
  int writeHeight = -1;

  void blitCpuWrittenFramebuffer(int target);

  int msbShadowCount[2] = {};

  POLYGONMODE proygonMode;

};
