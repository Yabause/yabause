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

#include "TextureManager.h"
#include "VIDVulkan.h"
#include "vulkan/vulkan.hpp"

TextureManager::~TextureManager() {
  const VkDevice device = vulkan->getDevice();

  vkQueueWaitIdle(vulkan->getVulkanQueue());

  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);
  vkDestroySampler(device, _textureSampler, nullptr);
  vkDestroyImageView(device, _textureImageView, nullptr);
  vkDestroyImage(device, _textureImage, nullptr);
  vkFreeMemory(device, _textureImageMemory, nullptr);


  vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
  vkDestroyCommandPool(device, commandPool, nullptr);
}

int TextureManager::init(unsigned int w, unsigned int h) {

  const VkDevice device = vulkan->getDevice();
  vk::Device d(device);

  _texture = (uint32_t*)createTextureImage(w, h); //(uint32_t*)malloc( w*h*4);
  _width = w;
  _height = h;
  reset();

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

  vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

  VkSemaphoreCreateInfo semaphore_create_info{};
  semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  vkCreateSemaphore(device, &semaphore_create_info, nullptr, &complete);

  return 0;
}

void TextureManager::reset() {
  _currentX = 0;
  _currentY = 0;
  _yMax = 0;
  resetCache();
}

void TextureManager::realloc(unsigned int width, unsigned int height) {
  uint32_t * newtexture = (uint32_t*)reallocTextureImage(_texture, width, height);
  _width = width;
  _height = height;
  _texture = newtexture;
}

void TextureManager::allocate(CharTexture * output, unsigned int w, unsigned int h, unsigned int * x, unsigned int * y) {
  if (_width < w) {
    //YGLDEBUG("can't allocate texture: %dx%d\n", w, h);
    this->realloc(w, _height);
    this->allocate(output, w, h, x, y);
    return;
  }
  if ((_height - _currentY) < h) {
    //YGLDEBUG("can't allocate texture: %dx%d\n", w, h);
    this->realloc(_width, _height + 512);
    this->allocate(output, w, h, x, y);
    return;
  }

  if ((_width - _currentX) >= w) {
    *x = _currentX;
    *y = _currentY;
    output->w = _width - w;
    output->textdata = _texture + _currentY * _width + _currentX;
    //yprintf("allocate %lx", (uintptr_t)output->textdata );
    _currentX += w;

    if ((_currentY + h) > _yMax) {
      _yMax = _currentY + h;
    }
  }
  else {
    _currentX = 0;
    _currentY = _yMax;
    this->allocate(output, w, h, x, y);
  }
}

void TextureManager::push() {

}

void TextureManager::pull() {

}

int TextureManager::isCached(uint64_t addr, TextureCache * c) {
  auto it = _texCache.find(addr);
  if (it == _texCache.end()) {
    return 0; // not found
  }

  //*c = _texCache[addr];
  c->addr = it->second.addr;
  c->x = it->second.x;
  c->y = it->second.y;

  return 1; // found
}

void TextureManager::addCache(uint64_t addr, TextureCache * c) {
  _texCache[addr] = *c;
}

void TextureManager::resetCache() {
  _texCache.clear();
}


void * TextureManager::createTextureImage(int texWidth, int texHeight) {

  const VkDevice device = vulkan->getDevice();
  if (device == VK_NULL_HANDLE) return NULL;

  VkDeviceSize imageSize = texWidth * texHeight * 4;

  char * pixels = (char*)malloc(texWidth * texHeight * 4);
  memset(pixels, 0, texWidth * texHeight * 4);

  vulkan->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
  vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &imageBuffer);
  memcpy(imageBuffer, pixels, static_cast<size_t>(imageSize));
  vkUnmapMemory(device, stagingBufferMemory);
  //stbi_image_free(pixels);

  vulkan->createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _textureImage, _textureImageMemory);
  vulkan->transitionImageLayout(_textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  vulkan->copyBufferToImage(stagingBuffer, _textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
  vulkan->transitionImageLayout(_textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  free(pixels);

  vkMapMemory(device, stagingBufferMemory, 0, VK_WHOLE_SIZE, 0, &imageBuffer);
  //yprintf("createTextureImage vkMapMemory %lx", (uintptr_t)imageBuffer);

  VkImageViewCreateInfo viewInfo = {};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = _textureImage;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  if (vkCreateImageView(device, &viewInfo, nullptr, &_textureImageView) != VK_SUCCESS) {
    throw std::runtime_error("failed to create texture image view!");
  }

  VkSamplerCreateInfo samplerInfo = {};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_NEAREST;
  samplerInfo.minFilter = VK_FILTER_NEAREST;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.anisotropyEnable = VK_FALSE;
  //samplerInfo.maxAnisotropy = 16;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 0.0f;
  if (vkCreateSampler(device, &samplerInfo, nullptr, &_textureSampler) != VK_SUCCESS) {
    throw std::runtime_error("failed to create texture sampler!");
  }

  this->texwidth = texWidth;
  this->texheight = texHeight;

  return imageBuffer;

}

#include <iostream>


void * TextureManager::reallocTextureImage(void * pixels, int texWidth, int texHeight) {

  std::cout << "reallocTextureImage height:" << texHeight << std::endl;

  const VkDevice device = vulkan->getDevice();
  if (device == VK_NULL_HANDLE) return NULL;


  VkDeviceSize imageSize = texWidth * texHeight * 4;

  char * newpixels = (char*)malloc(texWidth * texHeight * 4);
  for (int i = 0; i < this->texheight; i++) {
    char * newrow = &newpixels[i*texWidth * 4];
    char * oldrow = &((char*)pixels)[i*this->texwidth * 4];
    for (int j = 0; j < this->texwidth * 4; j++) {
      newrow[j] = oldrow[j];
    }
  }

  vkUnmapMemory(device, stagingBufferMemory);
  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);


  vulkan->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
  vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &imageBuffer);
  memcpy(imageBuffer, newpixels, static_cast<size_t>(imageSize));
  vkUnmapMemory(device, stagingBufferMemory);

  vkDestroyImage(device, _textureImage, nullptr);
  vkFreeMemory(device, _textureImageMemory, nullptr);
  vulkan->createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _textureImage, _textureImageMemory);
  vulkan->transitionImageLayout(_textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  vulkan->copyBufferToImage(stagingBuffer, _textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
  vulkan->transitionImageLayout(_textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  free(newpixels);

  vkMapMemory(device, stagingBufferMemory, 0, VK_WHOLE_SIZE, 0, &imageBuffer);
  //yprintf("reallocTextureImage vkMapMemory %lx", (uintptr_t)imageBuffer);

  vkDestroyImageView(device, _textureImageView, nullptr);
  VkImageViewCreateInfo viewInfo = {};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = _textureImage;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  if (vkCreateImageView(device, &viewInfo, nullptr, &_textureImageView) != VK_SUCCESS) {
    throw std::runtime_error("failed to create texture image view!");
  }

  this->texwidth = texWidth;
  this->texheight = texHeight;


  return imageBuffer;

}


void TextureManager::updateTextureImage(const std::function<void(VkCommandBuffer commandBuffer)>& f) {

  const VkDevice device = vulkan->getDevice();
  if (device == VK_NULL_HANDLE) return;


  VkDeviceSize imageSize = _width * _height * 4;

  vkUnmapMemory(device, stagingBufferMemory);


  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  //beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  //------------------------------------------------------------------------
  if (this->_yMax > 0) {

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    VkImageLayout oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkImageLayout newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = _textureImage;
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
      _width,
      this->_yMax,
      1
    };
    vkCmdCopyBufferToImage(
      commandBuffer,
      stagingBuffer,
      _textureImage,
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
  }

  f(commandBuffer);


  vkEndCommandBuffer(commandBuffer);
  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;
  submitInfo.signalSemaphoreCount = 0; //1;
  submitInfo.pSignalSemaphores = nullptr; // (VkSemaphore*)&complete;
  vkQueueSubmit(vulkan->getVulkanQueue(), 1, &submitInfo, VK_NULL_HANDLE);

//  vkQueueWaitIdle(vulkan->getVulkanQueue());
  //vkDeviceWaitIdle(device);

  vkMapMemory(device, stagingBufferMemory, 0, VK_WHOLE_SIZE, 0, &imageBuffer);
  //yprintf("vkMapMemory %lx to %lx", (uintptr_t)imageBuffer, (uintptr_t)imageBuffer + imageSize);

  _texture = (unsigned int *)imageBuffer;

}

