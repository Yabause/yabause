
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

#include "VulkanScene.h"
class VIDVulkan;

#include <iostream>

#include <vector>
using std::vector;

class VertexBlock {
public:
  VkBuffer _vertexBuffer = 0;
  VkDeviceMemory _vertexBufferMemory = 0;
  int vbufferSize = 0;

  VkBuffer _indexBuffer = 0;
  VkDeviceMemory _indexBufferMemory = 0;
  int ibufferSize = 0;

  VkBuffer stagingBuffer = 0;
  VkDeviceMemory stagingBufferMemory = 0;

  VkBuffer istagingBuffer = 0;
  VkDeviceMemory istagingBufferMemory = 0;

  unsigned int currentVertex = 0;
  unsigned int currentIndex = 0;

  //VkCommandBuffer commandBuffer;

  char * vdata;
  char * idata;

  void freeBuffers(VkDevice device) {


    vkDeviceWaitIdle(device);

#if 1 // Driver BUG??? can not free these memorys
    std::cout << "freeBlock " << " stagingBuffer=" << stagingBuffer << " stagingBufferMemory=" << stagingBufferMemory << std::endl;
    vkUnmapMemory(device, stagingBufferMemory);

    std::cout << "freeBlock " << " istagingBuffer=" << istagingBuffer << " istagingBufferMemory=" << istagingBufferMemory << std::endl;
    vkUnmapMemory(device, istagingBufferMemory);

    if (stagingBuffer != 0) {
      vkDestroyBuffer(device, stagingBuffer, nullptr);
      stagingBuffer = 0;
    }

    if (stagingBufferMemory != 0) {
      vkFreeMemory(device, stagingBufferMemory, nullptr);
      stagingBufferMemory = 0;
    }

    if (istagingBuffer != 0) {
      vkDestroyBuffer(device, istagingBuffer, nullptr);
      istagingBuffer = 0;
    }

    if (istagingBufferMemory != 0) {
      vkFreeMemory(device, istagingBufferMemory, nullptr);
      istagingBufferMemory = 0;
    }

    if (_vertexBuffer != 0) {
      vkDestroyBuffer(device, _vertexBuffer, nullptr);
      _vertexBuffer = 0;
    }
    if (_vertexBufferMemory != 0) {
      vkFreeMemory(device, _vertexBufferMemory, nullptr);
      _vertexBufferMemory = 0;
    }
    if (_indexBuffer != 0) {
      vkDestroyBuffer(device, _indexBuffer, nullptr);
      _indexBuffer = 0;
    }

    if (_indexBufferMemory != 0) {
      vkFreeMemory(device, _indexBufferMemory, nullptr);
      _indexBufferMemory = 0;
    }
#endif
    vbufferSize = 0;
    ibufferSize = 0;
  }
};


class VertexManager {
public:
  VertexManager(VIDVulkan * vulkan);
  ~VertexManager();
  void init(unsigned int vertexSize, unsigned int indexSize);
  void genBlock(VertexBlock & block, unsigned int vertexSize, unsigned int indexSize);
  void reset();
  void add(
    const std::vector<Vertex> & vertices,
    const std::vector<uint16_t> & indices,
    uint32_t & block,
    uint32_t & vertexOffset,
    uint32_t & vertexSize,
    uint32_t & indexOffset,
    uint32_t & indexSize
  );

  VkBuffer getVertexBuffer(int block) {
    return vertexBlocks[block]._vertexBuffer;
  }

  VkBuffer getIndexBuffer(int block) {
    return vertexBlocks[block]._indexBuffer;
  }

  void flush(VkCommandBuffer commandBuffer);

protected:

  VIDVulkan * vulkan;
  VkCommandPool commandPool;

  uint32_t baseVertexBlockSize;
  uint32_t baseIndexBlockSize;

  vector<VertexBlock> vertexBlocks;

  int currentBlock = 0;

  void copyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t offset, VkDeviceSize size);

};
