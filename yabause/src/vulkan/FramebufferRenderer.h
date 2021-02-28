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

#include "VIDVulkan.h"

#include <string>
using std::string;

#include <map>
using std::map;

#define MAX_RENDER_COUNT 32

class FramebufferRenderer {
public:

  enum ColorClacMode {
    SRC_ALPHA,
    ADD,
    DST_ALPHA,
    NONE
  };

  FramebufferRenderer(VIDVulkan * vulkan);
  ~FramebufferRenderer();
  void setup();
  void setupShaders();
  VkPipeline compileShader(const char * code, const char * name, enum ColorClacMode c);

  VkPipeline findShader(const char * name, const char * code, enum ColorClacMode c) {
    VkPipeline pgid;
    auto prg = pipelines.find(name);
    if (prg == pipelines.end()) {
      pgid = compileShader(code, name, c);
    }
    else {
      pgid = prg->second;
    }
    return pgid;
  }

  map<string, VkPipeline> pipelines;

  void setFrameBuffer(VkImageView framebuffer) {
    this->framebuffer = framebuffer;
  }

  void setColorRam(VkImageView colorRam) {
    this->cram = colorRam;
  }

  void setLineColor(VkImageView lineColor) {
    this->lineColor = lineColor;
  }


  void onStartFrame(Vdp2 * fixVdp2Reg, VkCommandBuffer commandBuffer);
  void draw(Vdp2 * fixVdp2Reg, VkCommandBuffer commandBuffer, int from, int to);
  void drawWithDestAlphaMode(Vdp2 * fixVdp2Regs, VkCommandBuffer commandBuffer, int from, int to);
  void drawShadow(Vdp2 * fixVdp2Reg, VkCommandBuffer commandBuffer, int from, int to);
  void onEndFrame();

  void chenageResolution(int vdp2Width, int vdp2Height, int renderWidth, int renderHeight);

  void flush(VkCommandBuffer commandBuffer) {
    if (renderCount == 0 && lineTexture != VK_NULL_HANDLE) {
      perline.update(vulkan, commandBuffer);
    }
  }

protected:
  VIDVulkan * vulkan;

  string vertexShaderName;
  string fragShaderName;

  VkImageView framebuffer;
  VkImageView cram;
  VkImageView lineColor;

  VkSampler sampler;

  DynamicTexture perline;
  /*
    int renderWidth, renderHeight;
    int vdp2Width, vdp2Height;
  */
  struct UniformBufferObject {
    glm::mat4 matrix;
    float pri[8 * 4];
    float alpha[8 * 4];
    float coloroffset[4];
    float cctl;
    float emu_height;
    float vheight;
    int color_ram_offset;
    float viewport_offset;
    int sprite_window;
    float from;
    float to;
  } ubo;

  struct UniformBuffer {
    VkBuffer buf = 0;
    VkDeviceMemory mem = 0;
  };

  std::vector<UniformBuffer> ubuffer;

  std::vector<Vertex> vertices;
  std::vector<uint16_t> indices;

  void createVertexBuffer(const std::vector<Vertex> & vertices);
  void createIndexBuffer(const std::vector<uint16_t> & indices);

  VkBuffer _vertexBuffer;
  VkDeviceMemory _vertexBufferMemory;
  VkBuffer _indexBuffer;
  VkDeviceMemory _indexBufferMemory;

  void createDescriptorSets();
  void updateDescriptorSets(int index);


  VkDescriptorSet _descriptorSet[MAX_RENDER_COUNT];
  VkDescriptorSetLayout _descriptorSetLayout;
  VkDescriptorPool _descriptorPool;
  VkShaderModule _vertShaderModule;
  VkShaderModule _fragShaderModule;
  VkPipelineLayout _pipelineLayout;
  VkPipeline _graphicsPipeline;

  VkViewport viewport = {};
  VkRect2D scissor = {};
  VkPipelineViewportStateCreateInfo viewportState = {};
  VkPipelineRasterizationStateCreateInfo rasterizer = {};
  VkPipelineMultisampleStateCreateInfo multisampling = {};
  VkPipelineDepthStencilStateCreateInfo depthStencil = {};

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
  VkPipelineColorBlendStateCreateInfo colorBlending = {};

  VkPipelineColorBlendAttachmentState colorBlendAttachmentAdd = {};
  VkPipelineColorBlendStateCreateInfo colorBlendingAdd = {};

  VkPipelineColorBlendAttachmentState colorBlendAttachmentDestAlpha = {};
  VkPipelineColorBlendStateCreateInfo colorBlendingDestAlpha = {};

  VkPipelineColorBlendAttachmentState colorBlendAttachmentNone = {};
  VkPipelineColorBlendStateCreateInfo colorBlendingNone = {};


  int renderCount = 0;

  void updateVdp2Reg(Vdp2 * fixVdp2Regs);

  VkImageView lineTexture = VK_NULL_HANDLE;

  std::string get_shader_header() {
#if defined(ANDROID)
    return "#version 310 es\n";
#else
    return "#version 450\n";
#endif
  }

};