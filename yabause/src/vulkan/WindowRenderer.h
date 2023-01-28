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

#include <functional>

#include "VulkanScene.h"
class VIDVulkan;
class TextureManager;
class VertexManager;
class CharTexture;
class TextureCache;
class VdpPipelineFactory;
class VdpPipelineWindow;


struct WindowUbo {
  glm::mat4 model;
  int windowBit;
};

class Vdp2Window {

public:
  Vdp2Window();

  int id;
  vdp2WindowInfo * info;
  int size;
  int isUpdated;
  Vertex * vertex;
  int vertexcnt;

  int width;
  int height;

  VIDVulkan * vulkan;
  VdpPipelineWindow * pipeline;
  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  int init(int id, VIDVulkan * vulkan, VkRenderPass offscreenPass);
  int updateSize(int width, int height, int pretransformFlag, bool rotateScreen );
  int flush(VkCommandBuffer commandBuffer);
  int draw(VkCommandBuffer commandBuffer);
  int free();

  bool isNeedDraw() {
    return (isUpdated == 1) ? true : false;
  }

};

class WindowRenderer {

  struct FrameBufferAttachment {
    VkImage image;
    VkDeviceMemory mem;
    VkImageView view;
  };
  struct OffscreenPass {
    int32_t width, height;
    VkFramebuffer frameBuffer;
    FrameBufferAttachment color, depth;
    VkRenderPass renderPass;
    VkSampler sampler;
    VkDescriptorImageInfo descriptor;
  } offscreenPass;

  void createCommandPool();
  VkCommandPool _command_pool = VK_NULL_HANDLE;
  std::vector<VkCommandBuffer> command_buffers;

  bool isSpriteWindowEnabled = false;
   
public:
  WindowRenderer(int width, int height, VIDVulkan * vulkan);
  ~WindowRenderer();
  void setUp();
  void changeResolution(int width, int height, int pretransformFlag, bool rotateScreen);

  vdp2WindowInfo * getVdp2WindowInfo(int which) {
    return window[which].info;
  }

  void generateWindowInfo(Vdp2 * fixVdp2Regs, int which);

  void flush(VkCommandBuffer cb);
  void draw(VkCommandBuffer commandBuffer, const std::function<void(VkCommandBuffer commandBuffer)>& f);

  VkImageView getImageView() { return offscreenPass.color.view; }
  VkSampler getSampler() { return offscreenPass.sampler; }

  bool isReady(){ return ready; }

  void setSpriteWindow(bool enable) {
	  isSpriteWindowEnabled = enable;
  }

  int getWidth(){ return offscreenPass.width; }
  int getHeight(){ return offscreenPass.height; }


protected:
  VdpPipelineFactory * pipleLineFactory;
  uint32_t vdp2width;
  uint32_t vdp2height;
  int pretransformFlag = 0;
  bool rotateScreen = false;
  VIDVulkan * vulkan;
  Vdp2Window window[2];
  void prepareOffscreen();
  bool ready = false;
};
