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
#include "ygl.h"
#include "yui.h"
#include "vidshared.h"

  extern vdp2rotationparameter_struct  paraA;
  extern vdp2rotationparameter_struct  paraB;
  extern Vdp2 * fixVdp2Regs;

}

#include "vulkan/VIDVulkan.h"
#include "vulkan/vulkan.hpp"

struct RBGUniform;

class RBGGeneratorVulkan {
public:
  RBGGeneratorVulkan();
  ~RBGGeneratorVulkan();

  void init(VIDVulkan * vulkan, int width, int height);
  void resize(int width, int height);
  void update(VIDVulkan::RBGDrawInfo * rbg, const vdp2rotationparameter_struct & paraa, const vdp2rotationparameter_struct & parab);
  VkImageView getTexture(int id);
  void onFinish();

  void updateDescriptorSets(int id);

  VkSemaphore getCompleteSemaphore() {
    if (tex_surface[0].rendered) {
      tex_surface[0].rendered = false;
      return static_cast<VkSemaphore>(semaphores.complete);
    }
    return VK_NULL_HANDLE;

  }

protected:
  int getAllainedSize(int size);

  VIDVulkan * vulkan = nullptr;

  struct ImageTarget {
    vk::Image image = nullptr;
    vk::DeviceMemory mem = nullptr;
    vk::ImageView view = nullptr;
    bool rendered = false;
  };

  struct Buffer {
    vk::Buffer buf = nullptr;
    vk::DeviceMemory mem = nullptr;
  };

  vk::Sampler sampler = nullptr;

  ImageTarget tex_surface[2];

  Buffer ssbo_vram_;
  Buffer ssbo_cram_;
  Buffer ssbo_window_;
  Buffer ssbo_paraA_;
  Buffer rbgUniform;

  vk::PipelineLayout pipelineLayout = nullptr;
  vk::DescriptorPool descriptorPool = nullptr;
  vk::DescriptorSetLayout descriptorSetLayout = nullptr;
  vk::DescriptorSet descriptorSet = nullptr;
  vk::Pipeline pipeline = nullptr;

  int tex_width_ = 0;
  int tex_height_ = 0;

  RBGUniform * rbgUniformParam;
  int struct_size_;

  void * mapped_vram = nullptr;

  vk::Queue queue = nullptr;
  vk::CommandPool commandPool = nullptr;
  vk::CommandBuffer command = nullptr;

  struct Semaphores {
    vk::Semaphore ready = nullptr;
    vk::Semaphore complete = nullptr;
  } semaphores;

  vk::Pipeline compile_color_dot(const char * base[], int size, const char * color, const char * dot);

  vk::Pipeline prg_rbg_0_2w_bitmap_4bpp_ = nullptr;
  vk::Pipeline prg_rbg_0_2w_bitmap_8bpp_ = nullptr;
  vk::Pipeline prg_rbg_0_2w_bitmap_16bpp_p_ = nullptr;
  vk::Pipeline prg_rbg_0_2w_bitmap_16bpp_ = nullptr;
  vk::Pipeline prg_rbg_0_2w_bitmap_32bpp_ = nullptr;
  vk::Pipeline prg_rbg_0_2w_p1_4bpp_ = nullptr;
  vk::Pipeline prg_rbg_0_2w_p2_4bpp_ = nullptr;
  vk::Pipeline prg_rbg_0_2w_p1_8bpp_ = nullptr;
  vk::Pipeline prg_rbg_0_2w_p2_8bpp_ = nullptr;
  vk::Pipeline prg_rbg_0_2w_p1_16bpp_p_ = nullptr;
  vk::Pipeline prg_rbg_0_2w_p2_16bpp_p_ = nullptr;
  vk::Pipeline prg_rbg_0_2w_p1_16bpp_ = nullptr;
  vk::Pipeline prg_rbg_0_2w_p2_16bpp_ = nullptr;
  vk::Pipeline prg_rbg_0_2w_p1_32bpp_ = nullptr;
  vk::Pipeline prg_rbg_0_2w_p2_32bpp_ = nullptr;

  vk::Pipeline prg_rbg_1_2w_bitmap_4bpp_ = nullptr;
  vk::Pipeline prg_rbg_1_2w_bitmap_8bpp_ = nullptr;
  vk::Pipeline prg_rbg_1_2w_bitmap_16bpp_p_ = nullptr;
  vk::Pipeline prg_rbg_1_2w_bitmap_16bpp_ = nullptr;
  vk::Pipeline prg_rbg_1_2w_bitmap_32bpp_ = nullptr;
  vk::Pipeline prg_rbg_1_2w_p1_4bpp_ = nullptr;
  vk::Pipeline prg_rbg_1_2w_p2_4bpp_ = nullptr;
  vk::Pipeline prg_rbg_1_2w_p1_8bpp_ = nullptr;
  vk::Pipeline prg_rbg_1_2w_p2_8bpp_ = nullptr;
  vk::Pipeline prg_rbg_1_2w_p1_16bpp_p_ = nullptr;
  vk::Pipeline prg_rbg_1_2w_p2_16bpp_p_ = nullptr;
  vk::Pipeline prg_rbg_1_2w_p1_16bpp_ = nullptr;
  vk::Pipeline prg_rbg_1_2w_p2_16bpp_ = nullptr;
  vk::Pipeline prg_rbg_1_2w_p1_32bpp_ = nullptr;
  vk::Pipeline prg_rbg_1_2w_p2_32bpp_ = nullptr;

  vk::Pipeline prg_rbg_2_2w_bitmap_4bpp_ = nullptr;
  vk::Pipeline prg_rbg_2_2w_bitmap_8bpp_ = nullptr;
  vk::Pipeline prg_rbg_2_2w_bitmap_16bpp_p_ = nullptr;
  vk::Pipeline prg_rbg_2_2w_bitmap_16bpp_ = nullptr;
  vk::Pipeline prg_rbg_2_2w_bitmap_32bpp_ = nullptr;
  vk::Pipeline prg_rbg_2_2w_p1_4bpp_ = nullptr;
  vk::Pipeline prg_rbg_2_2w_p2_4bpp_ = nullptr;
  vk::Pipeline prg_rbg_2_2w_p1_8bpp_ = nullptr;
  vk::Pipeline prg_rbg_2_2w_p2_8bpp_ = nullptr;
  vk::Pipeline prg_rbg_2_2w_p1_16bpp_p_ = nullptr;
  vk::Pipeline prg_rbg_2_2w_p2_16bpp_p_ = nullptr;
  vk::Pipeline prg_rbg_2_2w_p1_16bpp_ = nullptr;
  vk::Pipeline prg_rbg_2_2w_p2_16bpp_ = nullptr;
  vk::Pipeline prg_rbg_2_2w_p1_32bpp_ = nullptr;
  vk::Pipeline prg_rbg_2_2w_p2_32bpp_ = nullptr;

  vk::Pipeline prg_rbg_3_2w_bitmap_4bpp_ = nullptr;
  vk::Pipeline prg_rbg_3_2w_bitmap_8bpp_ = nullptr;
  vk::Pipeline prg_rbg_3_2w_bitmap_16bpp_p_ = nullptr;
  vk::Pipeline prg_rbg_3_2w_bitmap_16bpp_ = nullptr;
  vk::Pipeline prg_rbg_3_2w_bitmap_32bpp_ = nullptr;
  vk::Pipeline prg_rbg_3_2w_p1_4bpp_ = nullptr;
  vk::Pipeline prg_rbg_3_2w_p2_4bpp_ = nullptr;
  vk::Pipeline prg_rbg_3_2w_p1_8bpp_ = nullptr;
  vk::Pipeline prg_rbg_3_2w_p2_8bpp_ = nullptr;
  vk::Pipeline prg_rbg_3_2w_p1_16bpp_p_ = nullptr;
  vk::Pipeline prg_rbg_3_2w_p2_16bpp_p_ = nullptr;
  vk::Pipeline prg_rbg_3_2w_p1_16bpp_ = nullptr;
  vk::Pipeline prg_rbg_3_2w_p2_16bpp_ = nullptr;
  vk::Pipeline prg_rbg_3_2w_p1_32bpp_ = nullptr;
  vk::Pipeline prg_rbg_3_2w_p2_32bpp_ = nullptr;

  vk::Pipeline prg_rbg_0_2w_bitmap_4bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_0_2w_bitmap_8bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_0_2w_bitmap_16bpp_p_line_ = nullptr;
  vk::Pipeline prg_rbg_0_2w_bitmap_16bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_0_2w_bitmap_32bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_0_2w_p1_4bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_0_2w_p2_4bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_0_2w_p1_8bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_0_2w_p2_8bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_0_2w_p1_16bpp_p_line_ = nullptr;
  vk::Pipeline prg_rbg_0_2w_p2_16bpp_p_line_ = nullptr;
  vk::Pipeline prg_rbg_0_2w_p1_16bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_0_2w_p2_16bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_0_2w_p1_32bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_0_2w_p2_32bpp_line_ = nullptr;

  vk::Pipeline prg_rbg_1_2w_bitmap_4bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_1_2w_bitmap_8bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_1_2w_bitmap_16bpp_p_line_ = nullptr;
  vk::Pipeline prg_rbg_1_2w_bitmap_16bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_1_2w_bitmap_32bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_1_2w_p1_4bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_1_2w_p2_4bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_1_2w_p1_8bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_1_2w_p2_8bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_1_2w_p1_16bpp_p_line_ = nullptr;
  vk::Pipeline prg_rbg_1_2w_p2_16bpp_p_line_ = nullptr;
  vk::Pipeline prg_rbg_1_2w_p1_16bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_1_2w_p2_16bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_1_2w_p1_32bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_1_2w_p2_32bpp_line_ = nullptr;

  vk::Pipeline prg_rbg_2_2w_bitmap_4bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_2_2w_bitmap_8bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_2_2w_bitmap_16bpp_p_line_ = nullptr;
  vk::Pipeline prg_rbg_2_2w_bitmap_16bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_2_2w_bitmap_32bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_2_2w_p1_4bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_2_2w_p2_4bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_2_2w_p1_8bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_2_2w_p2_8bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_2_2w_p1_16bpp_p_line_ = nullptr;
  vk::Pipeline prg_rbg_2_2w_p2_16bpp_p_line_ = nullptr;
  vk::Pipeline prg_rbg_2_2w_p1_16bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_2_2w_p2_16bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_2_2w_p1_32bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_2_2w_p2_32bpp_line_ = nullptr;

  vk::Pipeline prg_rbg_3_2w_bitmap_4bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_3_2w_bitmap_8bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_3_2w_bitmap_16bpp_p_line_ = nullptr;
  vk::Pipeline prg_rbg_3_2w_bitmap_16bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_3_2w_bitmap_32bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_3_2w_p1_4bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_3_2w_p2_4bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_3_2w_p1_8bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_3_2w_p2_8bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_3_2w_p1_16bpp_p_line_ = nullptr;
  vk::Pipeline prg_rbg_3_2w_p2_16bpp_p_line_ = nullptr;
  vk::Pipeline prg_rbg_3_2w_p1_16bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_3_2w_p2_16bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_3_2w_p1_32bpp_line_ = nullptr;
  vk::Pipeline prg_rbg_3_2w_p2_32bpp_line_ = nullptr;


};
