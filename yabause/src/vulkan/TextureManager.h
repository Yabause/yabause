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

#include <unordered_map>
#include <vulkan/vulkan.h>
#include <functional>

class CharTexture {
public:
  uint32_t * textdata = nullptr;
  uint32_t w = 0;
};

class TextureCache {
public:
  uint64_t addr = 0;
  float x = 0.0f;
  float y = 0.0f;
};

class VIDVulkan;

class TextureManager {
protected:
  unsigned int _currentX = 0;
  unsigned int _currentY = 0;
  unsigned int _yMax = 0;
  unsigned int * _texture = nullptr;
  unsigned int _width = 0;
  unsigned int _height = 0;
  std::unordered_map<uint64_t, TextureCache> _texCache;
  VIDVulkan * vulkan;
public:
  TextureManager(VIDVulkan * vulkan) {
    this->vulkan = vulkan;
  }

  virtual ~TextureManager();
  int init(unsigned int w, unsigned int h);
  void reset();
  void realloc(unsigned int width, unsigned int height);
  void allocate(CharTexture *, unsigned int, unsigned int, unsigned int *, unsigned int *);
  void push();
  void pull();

  int isCached(uint64_t, TextureCache *);
  void addCache(uint64_t, TextureCache *);
  void resetCache();

  void * data() { return (void*)_texture; }


  VkImageView geTextureImageView() {
    return _textureImageView;
  }

  VkSampler getTextureSampler() {
    return _textureSampler;
  }

  VkSemaphore getCompleteSemaphore() {
    return VK_NULL_HANDLE; // complete;
  }

  void * createTextureImage(int texWidth, int texHeight);
  void * reallocTextureImage(void * pixels, int texWidth, int texHeight);

  void updateTextureImage(const std::function<void(VkCommandBuffer commandBuffer)>& f);

  VkImage _textureImage;
  VkDeviceMemory _textureImageMemory;
  VkImageView _textureImageView;
  VkSampler _textureSampler;

  int texwidth;
  int texheight;

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  void* imageBuffer;

  VkCommandPool commandPool;
  VkCommandBuffer commandBuffer;

  VkSemaphore complete;

};
