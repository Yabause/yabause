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

#include "Renderer.h"

#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Window.h"

#include <array>
#include <chrono>



struct Vertex {
  glm::vec4 pos;
  glm::vec4 color;
  glm::vec4 texCoord;

  Vertex(glm::vec4 pos, glm::vec4 color, glm::vec4 texCoord) {
    this->pos = pos;
    this->color = color;
    this->texCoord = texCoord;
  }

  Vertex() {
    pos.x = pos.y = pos.z = 0.0f;
    pos.w = 1.0f;
    color.x = color.y = color.z = color.w = 0.0f;
    texCoord.x = texCoord.y = texCoord.z = texCoord.w = 0.0f;
  }

  static VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
  }
  static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT; // vec2
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT; // vec3
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

    return attributeDescriptions;
  }
};


class VulkanScene
{
public:
  VulkanScene();
  ~VulkanScene();

  void setRenderer(Renderer * r) { _renderer = r; };
  void present();

  VkDevice getDevice() {
    return _renderer->GetVulkanDevice();
  }

  VkExtent2D getSurfaceSize() {
    return _renderer->getWindow()->GetVulkanSurfaceSize();
  }

  VkRenderPass getRenderPass() {
    return _renderer->getWindow()->GetVulkanRenderPass();
  }


  //VkBuffer getUniformBuffer() {
  //  return _uniformBuffer;
  //}

  const VkPhysicalDevice getPhysicalDevice() {
    return _renderer->GetVulkanPhysicalDevice();
  }

  const uint32_t getVulkanGraphicsQueueFamilyIndex() {
    return _renderer->GetVulkanGraphicsQueueFamilyIndex();
  }

  const uint32_t getVulkanComputeQueueFamilyIndex() {
    return _renderer->GetVulkanComputeQueueFamilyIndex();
  }

  const uint32_t getFrameBufferCount() {
    return _renderer->getWindow()->GetFrameBufferCount();
  }

  const VkQueue getVulkanQueue() {
    return _renderer->GetVulkanQueue();
  }

  const VkCommandPool getCommandPool() {
    return _command_pool;
  }

  VkShaderModule createShaderModule(const std::vector<char>& code);

  VkCommandBuffer beginSingleTimeCommands();
  void endSingleTimeCommands(VkCommandBuffer commandBuffer);
  uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

  bool screenshotSaved;

  void getScreenshot(void ** outbuf, int & width, int & height);

  void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
  void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

  void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
  void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
  void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);


protected:
  virtual int init(void);
  virtual void deInit(void);

  uint32_t _device_width;
  uint32_t _device_height;

  Renderer * _renderer;

  void createCommandPool();
  VkCommandPool _command_pool = VK_NULL_HANDLE;
  VkSemaphore _render_complete_semaphore = VK_NULL_HANDLE;
  std::vector<VkCommandBuffer> _command_buffers;
  uint32_t _current_frame = 0;

  VkImage dstScreenImage = VK_NULL_HANDLE;
  VkDeviceMemory dstImageScreenMemory = VK_NULL_HANDLE;
  unsigned char * dstbuf = nullptr;

};

