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
#include "Window.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <chrono>
#include <iostream>
#include <fstream>

#include "VulkanInitializers.hpp"
#include "VulkanTools.h"

static std::vector<char> readFile(const std::string& filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("failed to open file!");
  }
  size_t fileSize = (size_t)file.tellg();
  std::vector<char> buffer(fileSize);
  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();
  return buffer;
}

VulkanScene::VulkanScene()
{
}


VulkanScene::~VulkanScene()
{
}

void VulkanScene::present()
{
  _renderer->getWindow()->EndRender({ _render_complete_semaphore });
}

int VulkanScene::init(void)
{
  if (_renderer == nullptr) return -1;
  const VkDevice device = _renderer->GetVulkanDevice();
  if (device == VK_NULL_HANDLE) return -1;

  _device_width = _renderer->getWindow()->GetVulkanSurfaceSize().width;
  _device_height = _renderer->getWindow()->GetVulkanSurfaceSize().height;

  // Setup Command
  createCommandPool();
  
  return 0;
}

void VulkanScene::deInit(void)
{
  if (_renderer == nullptr) return;

  const VkDevice device = _renderer->GetVulkanDevice();
  if (device == VK_NULL_HANDLE) return;

  if (_command_buffers.size() != 0) {
    vkFreeCommandBuffers(device, _command_pool, _command_buffers.size(), _command_buffers.data());
    _command_buffers.clear();
  }
  vkDestroyCommandPool(device, _command_pool, nullptr);

  vkDestroySemaphore(device, _render_complete_semaphore, nullptr);
  _render_complete_semaphore = VK_NULL_HANDLE;
  _command_pool = VK_NULL_HANDLE;


}

VkShaderModule VulkanScene::createShaderModule(const std::vector<char>& code) {
  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
  VkShaderModule shaderModule;
  if (vkCreateShaderModule(_renderer->GetVulkanDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module!");
  }
  return shaderModule;
}

void VulkanScene::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {

  if (_renderer == nullptr) return;
  const VkDevice device = _renderer->GetVulkanDevice();
  if (device == VK_NULL_HANDLE) return;

  VkImageCreateInfo imageInfo = {};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = format;
  imageInfo.tiling = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = usage;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
    throw std::runtime_error("failed to create image!");
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(device, image, &memRequirements);

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate image memory!");
  }

  vkBindImageMemory(device, image, imageMemory, 0);
}

void VulkanScene::createCommandPool()
{
  if (_renderer == nullptr) return;
  const VkDevice device = _renderer->GetVulkanDevice();
  if (device == VK_NULL_HANDLE) return;

  if (_command_buffers.size() != 0) {
    vkFreeCommandBuffers(device, _command_pool, _command_buffers.size(), _command_buffers.data());
  }
  if (_command_pool != VK_NULL_HANDLE) vkDestroyCommandPool(device, _command_pool, nullptr);


  VkCommandPoolCreateInfo pool_create_info{};
  pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  pool_create_info.queueFamilyIndex = _renderer->GetVulkanGraphicsQueueFamilyIndex();
  vkCreateCommandPool(_renderer->GetVulkanDevice(), &pool_create_info, nullptr, &_command_pool);

  _command_buffers.resize(_renderer->getWindow()->GetFrameBufferCount());

  VkCommandBufferAllocateInfo	command_buffer_allocate_info{};
  command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  command_buffer_allocate_info.commandPool = _command_pool;
  command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  command_buffer_allocate_info.commandBufferCount = _command_buffers.size();
  vkAllocateCommandBuffers(_renderer->GetVulkanDevice(), &command_buffer_allocate_info, _command_buffers.data());

  VkSemaphoreCreateInfo semaphore_create_info{};
  semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  vkCreateSemaphore(_renderer->GetVulkanDevice(), &semaphore_create_info, nullptr, &_render_complete_semaphore);
}

void VulkanScene::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
  if (_renderer == nullptr) return;
  const VkDevice device = _renderer->GetVulkanDevice();

  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to create buffer!");
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate buffer memory!");
  }

  vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

VkCommandBuffer VulkanScene::beginSingleTimeCommands() {

  if (_renderer == nullptr) return VK_NULL_HANDLE;
  const VkDevice device = _renderer->GetVulkanDevice();

  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = _command_pool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}

void VulkanScene::endSingleTimeCommands(VkCommandBuffer commandBuffer) {

  if (_renderer == nullptr) return;
  const VkDevice device = _renderer->GetVulkanDevice();

  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(_renderer->GetVulkanQueue(), 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(_renderer->GetVulkanQueue());

  vkFreeCommandBuffers(device, _command_pool, 1, &commandBuffer);
}

void VulkanScene::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();
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
  barrier.srcAccessMask = 0; // TODO
  barrier.dstAccessMask = 0; // TODO

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }
  else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL) {

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

  }
  else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL) {

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

  }
  else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {

    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }
  else {
    throw std::invalid_argument("unsupported layout transition!");
  }


  vkCmdPipelineBarrier(
    commandBuffer,
    sourceStage, destinationStage,
    0,
    0, nullptr,
    0, nullptr,
    1, &barrier
  );
  endSingleTimeCommands(commandBuffer);

}

void VulkanScene::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();
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
    width,
    height,
    1
  };
  vkCmdCopyBufferToImage(
    commandBuffer,
    buffer,
    image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1,
    &region
  );
  endSingleTimeCommands(commandBuffer);
}


void VulkanScene::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
  if (_renderer == nullptr) return;

  VkCommandBuffer commandBuffer = beginSingleTimeCommands();
  if (commandBuffer == VK_NULL_HANDLE) return;

  VkBufferCopy copyRegion = {};
  copyRegion.srcOffset = 0; // Optional
  copyRegion.dstOffset = 0; // Optional
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  endSingleTimeCommands(commandBuffer);

}

uint32_t VulkanScene::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
  if (_renderer == nullptr) return -1;
  //VkPhysicalDevice physicalDevice = _renderer->GetVulkanPhysicalDevice();
  VkPhysicalDeviceMemoryProperties memProperties = _renderer->GetVulkanPhysicalDeviceMemoryProperties();
  //vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }
  throw std::runtime_error("failed to find suitable memory type!");
  return -1;
}


// Take a screenshot from the current swapchain image
// This is done using a blit from the swapchain image to a linear image whose memory content is then saved as a ppm image
// Getting the image date directly from a swapchain image wouldn't work as they're usually stored in an implementation dependent optimal tiling format
// Note: This requires the swapchain images to be created with the VK_IMAGE_USAGE_TRANSFER_SRC_BIT flag (see VulkanSwapChain::create)
void VulkanScene::getScreenshot(void ** outbuf, int & width, int & height)
{
  screenshotSaved = false;
  bool supportsBlit = true;

  _device_width = _renderer->getWindow()->GetVulkanSurfaceSize().width;
  _device_height = _renderer->getWindow()->GetVulkanSurfaceSize().height;


  // Check blit support for source and destination
  VkFormatProperties formatProps;

  // Check if the device supports blitting from optimal images (the swapchain images are in optimal format)
  vkGetPhysicalDeviceFormatProperties(getPhysicalDevice(), _renderer->getWindow()->getColorFormat(), &formatProps);
  if (!(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)) {
    std::cerr << "Device does not support blitting from optimal tiled images, using copy instead of blit!" << std::endl;
    supportsBlit = false;
  }

  // Check if the device supports blitting to linear images
  vkGetPhysicalDeviceFormatProperties(getPhysicalDevice(), VK_FORMAT_R8G8B8A8_UNORM, &formatProps);
  if (!(formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)) {
    std::cerr << "Device does not support blitting to linear tiled images, using copy instead of blit!" << std::endl;
    supportsBlit = false;
  }

  // Source for the copy is the last rendered swapchain image
  VkImage srcImage = _renderer->getWindow()->getCurrentImage();


  //if (dstScreenImage == VK_NULL_HANDLE) {

    // Create the linear tiled destination image to copy to and to read the memory from
    VkImageCreateInfo imageCreateCI(vks::initializers::imageCreateInfo());
    imageCreateCI.imageType = VK_IMAGE_TYPE_2D;
    // Note that vkCmdBlitImage (if supported) will also do format conversions if the swapchain color format would differ
    imageCreateCI.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageCreateCI.extent.width = _device_width;
    imageCreateCI.extent.height = _device_height;
    imageCreateCI.extent.depth = 1;
    imageCreateCI.arrayLayers = 1;
    imageCreateCI.mipLevels = 1;
    imageCreateCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateCI.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateCI.tiling = VK_IMAGE_TILING_LINEAR;
    imageCreateCI.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    // Create the image
    VK_CHECK_RESULT(vkCreateImage(getDevice(), &imageCreateCI, nullptr, &dstScreenImage));
    // Create memory to back up the image
    VkMemoryRequirements memRequirements;
    VkMemoryAllocateInfo memAllocInfo(vks::initializers::memoryAllocateInfo());

    vkGetImageMemoryRequirements(getDevice(), dstScreenImage, &memRequirements);
    memAllocInfo.allocationSize = memRequirements.size;
    // Memory must be host visible to copy from
    memAllocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(getDevice(), &memAllocInfo, nullptr, &dstImageScreenMemory));
    VK_CHECK_RESULT(vkBindImageMemory(getDevice(), dstScreenImage, dstImageScreenMemory, 0));

  //}
  //else {
  //}

  // Do the actual blit from the swapchain image to our host visible destination image
  VkCommandBuffer copyCmd = beginSingleTimeCommands();

  // Transition destination image to transfer destination layout
  vks::tools::insertImageMemoryBarrier(
    copyCmd,
    dstScreenImage,
    0,
    VK_ACCESS_TRANSFER_WRITE_BIT,
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

  // Transition swapchain image from present to transfer source layout
  vks::tools::insertImageMemoryBarrier(
    copyCmd,
    srcImage,
    VK_ACCESS_MEMORY_READ_BIT,
    VK_ACCESS_TRANSFER_READ_BIT,
    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

  // If source and destination support blit we'll blit as this also does automatic format conversion (e.g. from BGR to RGB)
  if (supportsBlit)
  {
    // Define the region to blit (we will blit the whole swapchain image)
    VkOffset3D blitSize;
    blitSize.x = _device_width;
    blitSize.y = _device_height;
    blitSize.z = 1;
    VkImageBlit imageBlitRegion{};
    imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageBlitRegion.srcSubresource.layerCount = 1;
    imageBlitRegion.srcOffsets[1] = blitSize;
    imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageBlitRegion.dstSubresource.layerCount = 1;
    imageBlitRegion.dstOffsets[1] = blitSize;

    // Issue the blit command
    vkCmdBlitImage(
      copyCmd,
      srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      dstScreenImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1,
      &imageBlitRegion,
      VK_FILTER_NEAREST);
  }
  else
  {
    // Otherwise use image copy (requires us to manually flip components)
    VkImageCopy imageCopyRegion{};
    imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopyRegion.srcSubresource.layerCount = 1;
    imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopyRegion.dstSubresource.layerCount = 1;
    imageCopyRegion.extent.width = _device_width;
    imageCopyRegion.extent.height = _device_height;
    imageCopyRegion.extent.depth = 1;

    // Issue the copy command
    vkCmdCopyImage(
      copyCmd,
      srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      dstScreenImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1,
      &imageCopyRegion);
  }

  // Transition destination image to general layout, which is the required layout for mapping the image memory later on
  vks::tools::insertImageMemoryBarrier(
    copyCmd,
    dstScreenImage,
    VK_ACCESS_TRANSFER_WRITE_BIT,
    VK_ACCESS_MEMORY_READ_BIT,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_IMAGE_LAYOUT_GENERAL,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

  // Transition back the swap chain image after the blit is done
  vks::tools::insertImageMemoryBarrier(
    copyCmd,
    srcImage,
    VK_ACCESS_TRANSFER_READ_BIT,
    VK_ACCESS_MEMORY_READ_BIT,
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

  //vulkanDevice->flushCommandBuffer(copyCmd, queue);
  endSingleTimeCommands(copyCmd);

  // Get layout of the image (including row pitch)
  VkImageSubresource subResource{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
  VkSubresourceLayout subResourceLayout;
  vkGetImageSubresourceLayout(getDevice(), dstScreenImage, &subResource, &subResourceLayout);

  // Map image memory so we can start copying from it
  unsigned char* data;
  vkMapMemory(getDevice(), dstImageScreenMemory, 0, VK_WHOLE_SIZE, 0, (void**)&data);
  data += subResourceLayout.offset;


  width = _device_width;
  height = _device_height;

  if (dstbuf != nullptr) {
    free(dstbuf);
  }
  dstbuf = (unsigned char*)malloc(width*height * 4);

  for (uint32_t y = 0; y < height; y++)
  {
    unsigned char *srcrow = &data[(_device_height - 1 - y)*subResourceLayout.rowPitch];
    unsigned char *dstrow = &dstbuf[y*width * 4];

    for (uint32_t x = 0; x < width; x++) {
      dstrow[x * 4 + 0] = srcrow[x * 4 + 0];
      dstrow[x * 4 + 1] = srcrow[x * 4 + 1];
      dstrow[x * 4 + 2] = srcrow[x * 4 + 2];
      dstrow[x * 4 + 3] = srcrow[x * 4 + 3];
    }

  }
  *outbuf = (void*)dstbuf;
#if 0

  std::ofstream file(filename, std::ios::out | std::ios::binary);

  // ppm header
  file << "P6\n" << _device_width << "\n" << _device_height << "\n" << 255 << "\n";

  // If source is BGR (destination is always RGB) and we can't use blit (which does automatic conversion), we'll have to manually swizzle color components
  bool colorSwizzle = false;
  // Check if source is BGR
  // Note: Not complete, only contains most common and basic BGR surface formats for demonstration purposes
  if (!supportsBlit)
  {
    std::vector<VkFormat> formatsBGR = { VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SNORM };
    colorSwizzle = (std::find(formatsBGR.begin(), formatsBGR.end(), _renderer->getWindow()->getColorFormat()) != formatsBGR.end());
  }

  // ppm binary pixel data
  for (uint32_t y = 0; y < height; y++)
  {
    unsigned int *row = (unsigned int*)data;
    for (uint32_t x = 0; x < width; x++)
    {
      if (colorSwizzle)
      {
        file.write((char*)row + 2, 1);
        file.write((char*)row + 1, 1);
        file.write((char*)row, 1);
      }
      else
      {
        file.write((char*)row, 3);
      }
      row++;
    }
    data += subResourceLayout.rowPitch;
  }
  file.close();
#endif
  std::cout << "Screenshot saved to disk" << std::endl;


  vkUnmapMemory(getDevice(), dstImageScreenMemory);
  vkFreeMemory(getDevice(), dstImageScreenMemory, nullptr);
  vkDestroyImage(getDevice(), dstScreenImage, nullptr);
  dstImageScreenMemory = VK_NULL_HANDLE;
  dstScreenImage = VK_NULL_HANDLE;

  screenshotSaved = true;
}


