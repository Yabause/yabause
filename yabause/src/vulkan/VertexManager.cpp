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

#include "VertexManager.h"
#include "VIDVulkan.h"
#include <iostream>

VertexManager::VertexManager(VIDVulkan * vulkan) {
  this->vulkan = vulkan;
}

VertexManager::~VertexManager() {

  for (int i = 0; i < vertexBlocks.size(); i++) {
    vertexBlocks[i].freeBuffers(vulkan->getDevice());
  }
  vertexBlocks.clear();
}

void VertexManager::init(unsigned int vertexSize, unsigned int indexSize) {

  VkDevice device = vulkan->getDevice();

  VertexBlock block;

  genBlock(block, sizeof(Vertex) * vertexSize, sizeof(uint16_t) * indexSize);

  baseVertexBlockSize = sizeof(Vertex) * vertexSize;
  baseIndexBlockSize = sizeof(uint16_t) * indexSize;

  this->vertexBlocks.push_back(block);

}

void VertexManager::genBlock(VertexBlock & block, unsigned int vertexSize, unsigned int indexSize) {

  std::cout << "genBlock " << " vertexSize=" << vertexSize << " indexSize=" << indexSize << std::endl;
  const VkDevice device = vulkan->getDevice();

  VkDeviceSize bufferSize = vertexSize;

  vulkan->createBuffer(bufferSize,
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    block._vertexBuffer, block._vertexBufferMemory);

  block.vbufferSize = bufferSize;

  vulkan->createBuffer(bufferSize,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    block.stagingBuffer, block.stagingBufferMemory);

  std::cout << "genBlock " << " stagingBuffer=" << block.stagingBuffer << " stagingBufferMemory=" << block.stagingBufferMemory << std::endl;

  bufferSize = indexSize;

  vulkan->createBuffer(bufferSize,
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    block._indexBuffer, block._indexBufferMemory);

  block.ibufferSize = bufferSize;

  vulkan->createBuffer(bufferSize,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    block.istagingBuffer, block.istagingBufferMemory);

  std::cout << "genBlock " << " istagingBuffer=" << block.istagingBuffer << " istagingBufferMemory=" << block.istagingBufferMemory << std::endl;

#if 0
  VkCommandPoolCreateInfo pool_create_info{};
  pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  pool_create_info.queueFamilyIndex = vulkan->getVulkanGraphicsQueueFamilyIndex();
  vkCreateCommandPool(device, &pool_create_info, nullptr, &commandPool);

  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

  allocInfo.commandPool = commandPool;
  allocInfo.commandBufferCount = 1;

  vkAllocateCommandBuffers(device, &allocInfo, &block.commandBuffer);
#endif

  vkMapMemory(device, block.stagingBufferMemory, 0, block.vbufferSize, 0, (void**)&block.vdata);
  vkMapMemory(device, block.istagingBufferMemory, 0, block.ibufferSize, 0, (void**)&block.idata);


}

void VertexManager::copyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t offset, VkDeviceSize size) {
  VkBufferCopy copyRegion = {};
  copyRegion.srcOffset = 0; // Optional
  copyRegion.dstOffset = offset; // Optional
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
}


void VertexManager::reset() {
  currentBlock = 0;
  vertexBlocks[0].currentVertex = 0;
  vertexBlocks[0].currentIndex = 0;
}

void VertexManager::add(
  const std::vector<Vertex> & vertices,
  const std::vector<uint16_t> & indices,
  uint32_t & block,
  uint32_t & vertexOffset,
  uint32_t & vertexSize,
  uint32_t & indexOffset,
  uint32_t & indexSize
) {

  VkDevice device = vulkan->getDevice();
  char* data;
  VkDeviceSize bufferSize;
  VertexBlock & last = vertexBlocks[currentBlock];

  int reqVertexSize = last.currentVertex + sizeof(Vertex) * vertices.size();
  int reqIndexSize = last.currentIndex + sizeof(uint16_t) * indices.size();

  if (last.vbufferSize < reqVertexSize || last.ibufferSize < reqIndexSize) {

    std::cout << "buffer Over (" << currentBlock << ") vertexSize=" << last.vbufferSize << "/" << reqVertexSize
      << " indexSize=" << last.ibufferSize << "/" << reqIndexSize << std::endl;

    currentBlock++;
    if (currentBlock >= vertexBlocks.size()) {
      VertexBlock newBlock;
      genBlock(newBlock, baseVertexBlockSize + sizeof(Vertex) * vertices.size() * 2, baseIndexBlockSize + sizeof(uint16_t) * indices.size() * 2);
      this->vertexBlocks.push_back(newBlock);
    }

    if (vertexBlocks[currentBlock].vbufferSize < reqVertexSize ||
      vertexBlocks[currentBlock].ibufferSize < reqIndexSize) {

      std::cout << "Used buffer Over (" << currentBlock << ") vertexSize=" << vertexBlocks[currentBlock].vbufferSize << "/" << reqVertexSize
        << " indexSize=" << vertexBlocks[currentBlock].ibufferSize << "/" << reqIndexSize << std::endl;


      vertexBlocks[currentBlock].freeBuffers(device);
      genBlock(vertexBlocks[currentBlock], reqVertexSize + baseVertexBlockSize, reqIndexSize + baseIndexBlockSize);
    }

    vertexBlocks[currentBlock].currentIndex = 0;
    vertexBlocks[currentBlock].currentVertex = 0;
    last = vertexBlocks[currentBlock];
  }

  // copy vertex buffer
  bufferSize = sizeof(Vertex) * vertices.size();
  memcpy(last.vdata + last.currentVertex, vertices.data(), (size_t)bufferSize);
  vertexOffset = last.currentVertex;
  last.currentVertex += bufferSize;

  // copy index buffer
  bufferSize = sizeof(indices[0]) * indices.size();
  memcpy(last.idata + last.currentIndex, indices.data(), (size_t)bufferSize);
  indexOffset = last.currentIndex;
  last.currentIndex += bufferSize;

  block = currentBlock;
  vertexSize = vertices.size();
  indexSize = indices.size();
}


void VertexManager::flush(VkCommandBuffer commandBuffer) {

  VkDevice device = vulkan->getDevice();

  for (int i = 0; i < currentBlock + 1; i++) {

    if (vertexBlocks[i].currentVertex <= 0) {
      continue;
    }

    vkUnmapMemory(device, vertexBlocks[i].stagingBufferMemory);

    copyBuffer(commandBuffer,
      vertexBlocks[i].stagingBuffer,
      vertexBlocks[i]._vertexBuffer,
      0, vertexBlocks[i].currentVertex);

    VkMemoryBarrier memoryBarrier = {};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
      memoryBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;

    vkCmdPipelineBarrier(
      commandBuffer,
      VK_PIPELINE_STAGE_TRANSFER_BIT,      // srcStageMask
      VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,   // dstStageMask
      0,
      1,                                    // memoryBarrierCount
      &memoryBarrier,                       // pMemoryBarriers
      0, nullptr,
      0, nullptr
    );


    vkMapMemory(device, vertexBlocks[i].stagingBufferMemory, 0, vertexBlocks[i].vbufferSize, 0, (void**)&vertexBlocks[i].vdata);

    vkUnmapMemory(device, vertexBlocks[i].istagingBufferMemory);

    copyBuffer(commandBuffer,
      vertexBlocks[i].istagingBuffer,
      vertexBlocks[i]._indexBuffer,
      0, vertexBlocks[i].currentIndex);

    vkCmdPipelineBarrier(
      commandBuffer,
      VK_PIPELINE_STAGE_TRANSFER_BIT,      // srcStageMask
      VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,   // dstStageMask
      0,
      1,                                    // memoryBarrierCount
      &memoryBarrier,                       // pMemoryBarriers
      0, nullptr,
      0, nullptr
    );

    vkMapMemory(device, vertexBlocks[i].istagingBufferMemory, 0, vertexBlocks[i].ibufferSize, 0, (void**)&vertexBlocks[i].idata);

  }

}