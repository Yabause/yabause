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

#include "WindowRenderer.h"
#include "VIDVulkan.h"
#include "VulkanInitializers.hpp"
#include "VulkanTools.h"
#include "TextureManager.h"
#include "VertexManager.h"
#include "VdpPipelineFactory.h"
#include "VdpPipeline.h"

#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define FB_COLOR_FORMAT VK_FORMAT_R8G8B8A8_UNORM

#include <iostream>

// Macro to check and display Vulkan return results
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

Vdp2Window::Vdp2Window() {
  this->id = -1;
  info = nullptr;
  size = 0;
  isUpdated = 0;
  vertex = nullptr;
  vertexcnt = 0;
  vulkan = nullptr;
  pipeline = nullptr;
  vertexBuffer = VK_NULL_HANDLE;
  vertexBufferMemory = VK_NULL_HANDLE;
  stagingBuffer = VK_NULL_HANDLE;
  stagingBufferMemory = VK_NULL_HANDLE;
}

int Vdp2Window::init(int id, VIDVulkan * vulkan, VkRenderPass offscreenPass) {

  this->id = id;
  VkDeviceSize bufferSize = sizeof(Vertex) * 512;
  const VkDevice device = vulkan->getDevice();

  this->vulkan = vulkan;


  pipeline = new VdpPipelineWindow(id, vulkan, vulkan->getTM(), vulkan->getVM());
  pipeline->setRenderPass(offscreenPass);
  pipeline->createGraphicsPipeline();


  vulkan->createBuffer(bufferSize,
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    vertexBuffer, vertexBufferMemory);

  vulkan->createBuffer(bufferSize,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    stagingBuffer, stagingBufferMemory);


  vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, (void**)&vertex);
  memset((void*)vertex, 0, bufferSize);


  return 0;
}

int Vdp2Window::flush(VkCommandBuffer commandBuffer) {

  if (vertexcnt <= 0) return 1;

  VkDevice device = vulkan->getDevice();
  vkUnmapMemory(device, stagingBufferMemory);

  VkBufferCopy copyRegion = {};
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = 0;
  copyRegion.size = vertexcnt * sizeof(Vertex);
  vkCmdCopyBuffer(commandBuffer, stagingBuffer, vertexBuffer, 1, &copyRegion);

  VkDeviceSize bufferSize = sizeof(Vertex) * 512;
  vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, (void**)&vertex);

  return 0;

}

int Vdp2Window::draw(VkCommandBuffer commandBuffer) {

  isUpdated = 0;

  if (vertexcnt <= 0) return 1;

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
    pipeline->getPipelineLayout(), 0, 1, &pipeline->_descriptorSet, 0, nullptr);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getGraphicsPipeline());

  VkBuffer vertexBuffers[] = { vertexBuffer };
  VkDeviceSize offsets[] = { 0 };

  vkCmdBindVertexBuffers(commandBuffer,
    0, 1, vertexBuffers, offsets);

  vkCmdDraw(commandBuffer, vertexcnt, 1, 0, 0);

  return 0;
}

int Vdp2Window::updateSize(int width, int height) {
  WindowUbo ubo = {};
  glm::mat4 m4(1.0f);
  ubo.model = glm::ortho(0.0f, (float)width, 0.0f, (float)height, 10.0f, 0.0f);
  ubo.windowBit = 1 << id;
  pipeline->setUBO(&ubo, sizeof(ubo));
  pipeline->updateDescriptorSets();
  return 0;
}


int Vdp2Window::free() {
  delete pipeline;
  VkDevice device = vulkan->getDevice();
  vkDestroyBuffer(device, vertexBuffer, nullptr);
  vkFreeMemory(device, vertexBufferMemory, nullptr);
  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);
  return 0;
}

WindowRenderer::WindowRenderer(int width, int height, VIDVulkan * vulkan)
{
  vdp2width = width;
  vdp2height = height;
  this->vulkan = vulkan;

}

void WindowRenderer::setUp() {
  createCommandPool();
  prepareOffscreen();
  window[0].init(0, vulkan, offscreenPass.renderPass);
  window[0].updateSize(vdp2width, vdp2height);
  window[1].init(1, vulkan, offscreenPass.renderPass);
  window[1].updateSize(vdp2width, vdp2height);

  VkDevice device = vulkan->getDevice();
}

WindowRenderer::~WindowRenderer() {

  VkDevice device = vulkan->getDevice();
  vkDestroySampler(device, offscreenPass.sampler, nullptr);
  vkDestroyImage(device, offscreenPass.color.image, nullptr);
  vkFreeMemory(device, offscreenPass.color.mem, nullptr);
  vkDestroyImageView(device, offscreenPass.color.view, nullptr);
  vkDestroyImage(device, offscreenPass.depth.image, nullptr);
  vkFreeMemory(device, offscreenPass.depth.mem, nullptr);
  vkDestroyImageView(device, offscreenPass.depth.view, nullptr);
  vkDestroyFramebuffer(device, offscreenPass.frameBuffer, nullptr);

}

void WindowRenderer::changeResolution(int width, int height) {

  if (width == this->vdp2width && height == this->vdp2height) return;

  this->vdp2width = width;
  this->vdp2height = height;

  vkQueueWaitIdle(vulkan->getVulkanQueue());

  VkDevice device = vulkan->getDevice();

  vkDestroySampler(device, offscreenPass.sampler, nullptr);
  vkDestroyImage(device, offscreenPass.color.image, nullptr);
  vkFreeMemory(device, offscreenPass.color.mem, nullptr);
  vkDestroyImageView(device, offscreenPass.color.view, nullptr);
  vkDestroyImage(device, offscreenPass.depth.image, nullptr);
  vkFreeMemory(device, offscreenPass.depth.mem, nullptr);
  vkDestroyImageView(device, offscreenPass.depth.view, nullptr);
  vkDestroyFramebuffer(device, offscreenPass.frameBuffer, nullptr);
  prepareOffscreen();

  window[0].updateSize(vdp2width, vdp2height);
  window[1].updateSize(vdp2width, vdp2height);

}

void WindowRenderer::flush(VkCommandBuffer commandbuffer) {
  if (window[0].isNeedDraw() || window[1].isNeedDraw()) {
    window[0].flush(commandbuffer);
    window[1].flush(commandbuffer);
  }
}


void WindowRenderer::draw(VkCommandBuffer commandBuffer) {
  if (window[0].isNeedDraw() || window[1].isNeedDraw()) {
    VkDevice device = vulkan->getDevice();

    std::array<VkClearValue, 2> clear_values{};
    clear_values[1].depthStencil.depth = 0.0f;
    clear_values[1].depthStencil.stencil = 0;
    clear_values[0].color.float32[0] = 0.0f;
    clear_values[0].color.float32[1] = 0.0f;
    clear_values[0].color.float32[2] = 0.0f;
    clear_values[0].color.float32[3] = 0.0f;

    VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
    renderPassBeginInfo.renderPass = offscreenPass.renderPass;
    renderPassBeginInfo.framebuffer = offscreenPass.frameBuffer;
    renderPassBeginInfo.renderArea.extent.width = offscreenPass.width;
    renderPassBeginInfo.renderArea.extent.height = offscreenPass.height;
    renderPassBeginInfo.clearValueCount = clear_values.size();
    renderPassBeginInfo.pClearValues = clear_values.data();
    VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = vks::initializers::viewport((float)vdp2width, (float)vdp2height, 0.0f, 1.0f);
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = vks::initializers::rect2D(vdp2width, vdp2height, 0, 0);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkDeviceSize offsets[1] = { 0 };

    window[0].draw(commandBuffer);
    window[1].draw(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);

  }
}

void WindowRenderer::createCommandPool() {

  VkDevice device = vulkan->getDevice();

  VkCommandPoolCreateInfo pool_create_info{};
  pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  pool_create_info.queueFamilyIndex = vulkan->getVulkanGraphicsQueueFamilyIndex();
  vkCreateCommandPool(device, &pool_create_info, nullptr, &_command_pool);

  command_buffers.resize(vulkan->getFrameBufferCount() * 2);

  VkCommandBufferAllocateInfo	command_buffer_allocate_info{};
  command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  command_buffer_allocate_info.commandPool = _command_pool;
  command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  command_buffer_allocate_info.commandBufferCount = command_buffers.size();
  vkAllocateCommandBuffers(device, &command_buffer_allocate_info, command_buffers.data());
}


void WindowRenderer::generateWindowInfo(Vdp2 * fixVdp2Regs, int which) {
  bool isWindowUsed = false;
  int i;
  int HShift;
  u32 v = 0;
  u32 LineWinAddr;
  u32 WPSX;
  u32 WPEX;
  u32 WPSY;
  u32 WPEY;
  int lineTableEnable;

  Vdp2Window * w = &window[which];

  if (which == 0) {

    if ((fixVdp2Regs->WCTLA & 0X2) || (fixVdp2Regs->WCTLA & 0X200) || (fixVdp2Regs->WCTLB & 0X2) || (fixVdp2Regs->WCTLB & 0X200) ||
      (fixVdp2Regs->WCTLC & 0X2) || (fixVdp2Regs->WCTLC & 0X200) || (fixVdp2Regs->WCTLD & 0X2) || (fixVdp2Regs->WCTLD & 0X200) || (fixVdp2Regs->RPMD == 0X03)) {
      isWindowUsed = true;
    }
    lineTableEnable = (fixVdp2Regs->LWTA0.part.U & 0x8000);
    LineWinAddr = (u32)((((fixVdp2Regs->LWTA0.part.U & 0x07) << 15) | (fixVdp2Regs->LWTA0.part.L >> 1)) << 2);

    WPSX = fixVdp2Regs->WPSX0;
    WPEX = fixVdp2Regs->WPEX0;
    WPSY = fixVdp2Regs->WPSY0;
    WPEY = fixVdp2Regs->WPEY0;

  }
  else {
    if ((fixVdp2Regs->WCTLA & 0x8) || (fixVdp2Regs->WCTLA & 0x800) || (fixVdp2Regs->WCTLB & 0x8) || (fixVdp2Regs->WCTLB & 0x800) ||
      (fixVdp2Regs->WCTLC & 0x8) || (fixVdp2Regs->WCTLC & 0x800) || (fixVdp2Regs->WCTLD & 0x8) || (fixVdp2Regs->WCTLD & 0x800)) {
      isWindowUsed = true;
    }
    lineTableEnable = (fixVdp2Regs->LWTA1.part.U & 0x8000);
    LineWinAddr = (u32)((((fixVdp2Regs->LWTA1.part.U & 0x07) << 15) | (fixVdp2Regs->LWTA1.part.L >> 1)) << 2);

    WPSX = fixVdp2Regs->WPSX1;
    WPEX = fixVdp2Regs->WPEX1;
    WPSY = fixVdp2Regs->WPSY1;
    WPEY = fixVdp2Regs->WPEY1;

  }

  if (isWindowUsed) {

    // resize to fit resolusion
    if (w->size != vdp2height)
    {
      if (w->info != NULL) free(w->info);
      w->info = (vdp2WindowInfo*)malloc(sizeof(vdp2WindowInfo)*(vdp2height + 8));

      for (i = 0; i < vdp2height; i++)
      {
        w->info[i].WinShowLine = 1;
        w->info[i].WinHStart = 0;
        w->info[i].WinHEnd = 1024;
      }

      w->size = vdp2height;
      w->isUpdated = 1;
    }

    HShift = 0;
    if (vdp2width >= 640) HShift = 0; else HShift = 1;

    if (lineTableEnable) {
      int preHStart = -1;
      int preHEnd = -1;
      int preshowline = 0;
      w->vertexcnt = 0;

      for (v = 0; v < vdp2height; v++)
      {
        if (v < WPSY || v > WPEY)
        {
          if (w->info[v].WinShowLine) w->isUpdated = 1;
          w->info[v].WinShowLine = 0;
          // finish vertex
          if (w->info[v].WinShowLine != preshowline) {
            w->vertex[w->vertexcnt].pos.y = v;
            w->vertexcnt++;
            w->vertex[w->vertexcnt].pos.x = preHEnd + 1;
            w->vertex[w->vertexcnt].pos.y = v;
            w->vertexcnt++;
            w->vertex[w->vertexcnt].pos.x = preHEnd + 1; // add terminator
            w->vertex[w->vertexcnt].pos.y = v;
            w->vertexcnt++;
          }
        }
        else {
          short HStart = Vdp2RamReadWord(LineWinAddr + (v << 2));
          short HEnd = Vdp2RamReadWord(LineWinAddr + (v << 2) + 2);

          if (HStart < HEnd)
          {
            HStart >>= HShift;
            HEnd >>= HShift;

            if (!(w->info[v].WinHStart == HStart && w->info[v].WinHEnd == HEnd))
            {
              w->isUpdated = 1;
            }

            w->info[v].WinHStart = HStart;
            w->info[v].WinHEnd = HEnd;
            w->info[v].WinShowLine = 1;

          }
          else {
            if (w->info[v].WinShowLine) w->isUpdated = 1;
            w->info[v].WinHStart = 0;
            w->info[v].WinHEnd = 0;
            w->info[v].WinShowLine = 0;

          }

          if (w->info[v].WinShowLine != preshowline) {
            // start vertex
            if (w->info[v].WinShowLine) {
              w->vertex[w->vertexcnt].pos.x = HStart; // add terminator
              w->vertex[w->vertexcnt].pos.y = v;
              w->vertexcnt++;
              w->vertex[w->vertexcnt].pos.x = HStart;
              w->vertex[w->vertexcnt].pos.y = v;
              w->vertexcnt++;
              w->vertex[w->vertexcnt].pos.x = HEnd + 1;
              w->vertex[w->vertexcnt].pos.y = v;
              w->vertexcnt++;
            }
            // finish vertex
            else {
              w->vertex[w->vertexcnt].pos.x = preHStart;
              w->vertex[w->vertexcnt].pos.y = v;
              w->vertexcnt++;
              w->vertex[w->vertexcnt].pos.x = preHEnd + 1;
              w->vertex[w->vertexcnt].pos.y = v;
              w->vertexcnt++;
              w->vertex[w->vertexcnt].pos.x = preHEnd + 1; // add terminator
              w->vertex[w->vertexcnt].pos.y = v;
              w->vertexcnt++;
            }
          }
          else {
            if (w->info[v].WinShowLine) {
              if (HStart != preHStart || HEnd != preHEnd) {
                // close line 
                w->vertex[w->vertexcnt].pos.x = preHStart;
                w->vertex[w->vertexcnt].pos.y = v;
                w->vertexcnt++;
                w->vertex[w->vertexcnt].pos.x = preHEnd + 1;
                w->vertex[w->vertexcnt].pos.y = v;
                w->vertexcnt++;
                w->vertex[w->vertexcnt].pos.x = preHEnd + 1; // add terminator
                w->vertex[w->vertexcnt].pos.y = v;
                w->vertexcnt++;

                // start new line
                w->vertex[w->vertexcnt].pos.x = HStart; // add terminator
                w->vertex[w->vertexcnt].pos.y = v;
                w->vertexcnt++;
                w->vertex[w->vertexcnt].pos.x = HStart;
                w->vertex[w->vertexcnt].pos.y = v;
                w->vertexcnt++;
                w->vertex[w->vertexcnt].pos.x = HEnd + 1;
                w->vertex[w->vertexcnt].pos.y = v;
                w->vertexcnt++;
              }
              // Close polygon, since reach the final line while WinShowLine is true
              else if (v == WPEY - 1) {
                w->vertex[w->vertexcnt].pos.x = HStart;
                w->vertex[w->vertexcnt].pos.y = v;
                w->vertexcnt++;
                w->vertex[w->vertexcnt].pos.x = HEnd + 1;
                w->vertex[w->vertexcnt].pos.y = v;
                w->vertexcnt++;
              }
            }
          }

          if (w->vertexcnt >= 1024) {
            w->vertexcnt = 1023;
          }

          preHStart = HStart;
          preHEnd = HEnd;
        }
        preshowline = w->info[v].WinShowLine;
      }

    }
    else {

      // Check Update
      if (!(w->info[0].WinHStart == (WPSX >> HShift) && w->info[0].WinHEnd == (WPEX >> HShift)))
      {
        w->isUpdated = 1;
      }

      for (v = 0; v < vdp2height; v++)
      {

        w->info[v].WinHStart = WPSX >> HShift;
        w->info[v].WinHEnd = WPEX >> HShift;
        if (v < WPSY || v > WPEY)
        {
          if (w->info[v].WinShowLine) w->isUpdated = 1;
          w->info[v].WinShowLine = 0;
        }
        else {
          if (w->info[v].WinShowLine == 0) w->isUpdated = 1;
          w->info[v].WinShowLine = 1;
        }
      }

      if (WPSX < WPEX) {
        w->vertex[0].pos.x = WPSX >> HShift;
        w->vertex[0].pos.y = WPSY;
        w->vertex[1].pos.x = (WPEX >> HShift) + 1;
        w->vertex[1].pos.y = WPSY;
        w->vertex[2].pos.x = WPSX >> HShift;
        w->vertex[2].pos.y = WPEY + 1;
        w->vertex[3].pos.x = (WPEX >> HShift) + 1;
        w->vertex[3].pos.y = WPEY + 1;
      }
      else {
        w->vertex[0].pos.x = 0;
        w->vertex[0].pos.y = 0;
        w->vertex[1].pos.x = 0;
        w->vertex[1].pos.y = 0;
        w->vertex[2].pos.x = 0;
        w->vertex[2].pos.y = 0;
        w->vertex[3].pos.x = 0;
        w->vertex[3].pos.y = 0;
      }
      w->vertexcnt = 4;
    }

    // no Window is used
  }
  else {
    if (w->size != 0)
    {
      w->isUpdated = 1;
    }

    if (w->info != NULL)
    {
      free(w->info);
      w->info = NULL;
    }
    w->size = 0;
    w->vertexcnt = 0;
  }

}

void WindowRenderer::prepareOffscreen() {

  VkDevice device = vulkan->getDevice();
  VkPhysicalDevice physicalDevice = vulkan->getPhysicalDevice();

  offscreenPass.width = vdp2width;
  offscreenPass.height = vdp2height;

  // Find a suitable depth format
  VkFormat fbDepthFormat;
  VkBool32 validDepthFormat = vks::tools::getSupportedDepthFormat(physicalDevice, &fbDepthFormat);
  assert(validDepthFormat);

  // Color attachment
  VkImageCreateInfo image = vks::initializers::imageCreateInfo();
  image.imageType = VK_IMAGE_TYPE_2D;
  image.format = FB_COLOR_FORMAT;
  image.extent.width = offscreenPass.width;
  image.extent.height = offscreenPass.height;
  image.extent.depth = 1;
  image.mipLevels = 1;
  image.arrayLayers = 1;
  image.samples = VK_SAMPLE_COUNT_1_BIT;
  image.tiling = VK_IMAGE_TILING_OPTIMAL;
  // We will sample directly from the color attachment
  image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;

  VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
  VkMemoryRequirements memReqs;

  VK_CHECK_RESULT(vkCreateImage(device, &image, nullptr, &offscreenPass.color.image));
  vkGetImageMemoryRequirements(device, offscreenPass.color.image, &memReqs);
  memAlloc.allocationSize = memReqs.size;
  //memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  memAlloc.memoryTypeIndex = vulkan->findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &offscreenPass.color.mem));
  VK_CHECK_RESULT(vkBindImageMemory(device, offscreenPass.color.image, offscreenPass.color.mem, 0));

  VkImageViewCreateInfo colorImageView = vks::initializers::imageViewCreateInfo();
  colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
  colorImageView.format = FB_COLOR_FORMAT;
  colorImageView.subresourceRange = {};
  colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  colorImageView.subresourceRange.baseMipLevel = 0;
  colorImageView.subresourceRange.levelCount = 1;
  colorImageView.subresourceRange.baseArrayLayer = 0;
  colorImageView.subresourceRange.layerCount = 1;
  colorImageView.image = offscreenPass.color.image;
  VK_CHECK_RESULT(vkCreateImageView(device, &colorImageView, nullptr, &offscreenPass.color.view));


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
  VK_CHECK_RESULT(vkCreateSampler(device, &samplerInfo, nullptr, &offscreenPass.sampler));

  // Depth stencil attachment
  image.format = fbDepthFormat;
  image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

  VK_CHECK_RESULT(vkCreateImage(device, &image, nullptr, &offscreenPass.depth.image));
  vkGetImageMemoryRequirements(device, offscreenPass.depth.image, &memReqs);
  memAlloc.allocationSize = memReqs.size;
  memAlloc.memoryTypeIndex = vulkan->findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &offscreenPass.depth.mem));
  VK_CHECK_RESULT(vkBindImageMemory(device, offscreenPass.depth.image, offscreenPass.depth.mem, 0));

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
  depthStencilView.image = offscreenPass.depth.image;
  VK_CHECK_RESULT(vkCreateImageView(device, &depthStencilView, nullptr, &offscreenPass.depth.view));

  // Create a separate render pass for the offscreen rendering as it may differ from the one used for scene rendering

  std::array<VkAttachmentDescription, 2> attchmentDescriptions = {};
  // Color attachment
  attchmentDescriptions[0].format = FB_COLOR_FORMAT;
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
  attchmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attchmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attchmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attchmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
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
  renderPassInfo.dependencyCount = 0;// static_cast<uint32_t>(dependencies.size());
  renderPassInfo.pDependencies = nullptr; // dependencies.data();

  VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &offscreenPass.renderPass));



  VkImageView attachments[2];
  attachments[0] = offscreenPass.color.view;
  attachments[1] = offscreenPass.depth.view;

  VkFramebufferCreateInfo fbufCreateInfo = vks::initializers::framebufferCreateInfo();
  fbufCreateInfo.renderPass = offscreenPass.renderPass;
  fbufCreateInfo.attachmentCount = 2;
  fbufCreateInfo.pAttachments = attachments;
  fbufCreateInfo.width = offscreenPass.width;
  fbufCreateInfo.height = offscreenPass.height;
  fbufCreateInfo.layers = 1;
  VK_CHECK_RESULT(vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &offscreenPass.frameBuffer));


  //  VkSemaphoreCreateInfo semaphore_create_info{};
  //  semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  //  vkCreateSemaphore(device, &semaphore_create_info, nullptr, &offscreenPass.color._render_complete_semaphore);


    // Fill a descriptor for later use in a descriptor set
  offscreenPass.descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  offscreenPass.descriptor.imageView = offscreenPass.color.view;
  offscreenPass.descriptor.sampler = offscreenPass.sampler;

  //vulkan->transitionImageLayout(offscreenPass.color.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

}
