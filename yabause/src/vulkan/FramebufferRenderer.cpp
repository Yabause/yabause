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


#include "FramebufferRenderer.h"
#include "VIDVulkan.h"
#include "VulkanTools.h"

extern "C" {
#include "vidshared.h"
#include "debug.h"
#include "vdp2.h"
#include "yabause.h"
#include "ygl.h"
#include "yui.h"
#include "frameprofile.h"
}

#include "shaderc/shaderc.hpp"
using shaderc::Compiler;
using shaderc::CompileOptions;
using shaderc::SpvCompilationResult;

#include <string>
using std::string;

#include <iostream>

extern "C" {
  extern const GLchar Yglprg_vdp2_drawfb_cram_vulkan_f[];
  extern const GLchar Yglprg_vdp2_drawfb_cram_no_color_col_f[];
  extern const GLchar Yglprg_vdp2_drawfb_cram_destalpha_col_f[];
  extern const GLchar Yglprg_vdp2_drawfb_cram_less_color_col_f[];
  extern const GLchar Yglprg_vdp2_drawfb_cram_equal_color_col_f[];
  extern const GLchar Yglprg_vdp2_drawfb_cram_more_color_col_f[];
  extern const GLchar Yglprg_vdp2_drawfb_cram_msb_color_col_f[];
  extern const GLchar Yglprg_vdp2_drawfb_cram_less_color_add_f[];
  extern const GLchar Yglprg_vdp2_drawfb_cram_equal_color_add_f[];
  extern const GLchar Yglprg_vdp2_drawfb_cram_more_color_add_f[];
  extern const GLchar Yglprg_vdp2_drawfb_cram_msb_color_add_f[];
  extern const GLchar Yglprg_vdp2_drawfb_line_blend_fv[];
  extern const GLchar Yglprg_vdp2_drawfb_line_add_fv[];
  extern const GLchar Yglprg_vdp2_drawfb_cram_less_line_dest_alpha_fv[];
  extern const GLchar Yglprg_vdp2_drawfb_cram_equal_line_dest_alpha_fv[];
  extern const GLchar Yglprg_vdp2_drawfb_cram_more_line_dest_alpha_fv[];
  extern const GLchar Yglprg_vdp2_drawfb_cram_msb_line_dest_alpha_fv[];
  extern const GLchar Yglprg_vdp2_drawfb_cram_eiploge_f[];
  extern const GLchar Yglprg_vdp2_drawfb_hblank_vulkan_f[];
  extern const GLchar Yglprg_vdp2_drawfb_cram_less_color_col_hblank_f[];
  extern const GLchar Yglprg_vdp2_drawfb_cram_equal_color_col_hblank_f[];
  extern const GLchar Yglprg_vdp2_drawfb_cram_more_color_col_hblank_f[];
  extern const GLchar Yglprg_vdp2_drawfb_cram_msb_color_col_hblank_f[];
  extern const GLchar Yglprg_vdp2_drawfb_cram_destalpha_col_hblank_f[];
  extern const GLchar Yglprg_vdp2_drawfb_cram_eiploge_vulkan_f[];
}


void FramebufferRenderer::setupShaders() {

  //compileShader(Yglprg_vdp2_drawfb_cram_no_color_col_f, "no_color_col",SRC_ALPHA);
  //compileShader(Yglprg_vdp2_drawfb_cram_destalpha_col_f, "destalpha_col",SRC_ALPHA);

  //compileShader(Yglprg_vdp2_drawfb_cram_less_color_col_f, "less_color_col", SRC_ALPHA);
  //compileShader(Yglprg_vdp2_drawfb_cram_equal_color_col_f, "equal_color_col", SRC_ALPHA);
  //compileShader(Yglprg_vdp2_drawfb_cram_more_color_col_f, "more_color_col", SRC_ALPHA);
  //compileShader(Yglprg_vdp2_drawfb_cram_msb_color_col_f, "msb_color_col", SRC_ALPHA);

  //compileShader(Yglprg_vdp2_drawfb_cram_less_color_add_f, "less_color_add", ADD);
  //compileShader(Yglprg_vdp2_drawfb_cram_equal_color_add_f, "equal_color_add", ADD);
  //compileShader(Yglprg_vdp2_drawfb_cram_more_color_add_f, "more_color_add", ADD);
  //compileShader(Yglprg_vdp2_drawfb_cram_msb_color_add_f, "msb_color_add", ADD);

  //compileShader(Yglprg_vdp2_drawfb_line_blend_fv, "line_blend", SRC_ALPHA);
  //compileShader(Yglprg_vdp2_drawfb_line_add_fv, "line_add", SRC_ALPHA);

  string a;

  a = string(Yglprg_vdp2_drawfb_cram_less_line_dest_alpha_fv) + string(Yglprg_vdp2_drawfb_cram_destalpha_col_f);
  compileShader(a.c_str(), "less_destalpha_line", SRC_ALPHA);

  a = string(Yglprg_vdp2_drawfb_cram_equal_line_dest_alpha_fv) + string(Yglprg_vdp2_drawfb_cram_destalpha_col_f);
  compileShader(a.c_str(), "equal_destalpha_line", SRC_ALPHA);

  a = string(Yglprg_vdp2_drawfb_cram_more_line_dest_alpha_fv) + string(Yglprg_vdp2_drawfb_cram_destalpha_col_f);
  compileShader(a.c_str(), "more_destalpha_line", SRC_ALPHA);

  a = string(Yglprg_vdp2_drawfb_cram_msb_line_dest_alpha_fv) + string(Yglprg_vdp2_drawfb_cram_destalpha_col_f);
  compileShader(a.c_str(), "msb_destalpha_line", SRC_ALPHA);

  a = string(Yglprg_vdp2_drawfb_cram_less_color_col_f) + string(Yglprg_vdp2_drawfb_line_blend_fv);
  compileShader(a.c_str(), "less_color_col_line", NONE);

  a = string(Yglprg_vdp2_drawfb_cram_equal_color_col_f) + string(Yglprg_vdp2_drawfb_line_blend_fv);
  compileShader(a.c_str(), "equal_color_col_line", NONE);

  a = string(Yglprg_vdp2_drawfb_cram_more_color_col_f) + string(Yglprg_vdp2_drawfb_line_blend_fv);
  compileShader(a.c_str(), "more_color_col_line", NONE);

  a = string(Yglprg_vdp2_drawfb_cram_msb_color_col_f) + string(Yglprg_vdp2_drawfb_line_blend_fv);
  compileShader(a.c_str(), "msb_color_col_line", NONE);

  a = string(Yglprg_vdp2_drawfb_cram_less_color_add_f) + string(Yglprg_vdp2_drawfb_line_add_fv);
  compileShader(a.c_str(), "less_color_add_line", NONE);

  a = string(Yglprg_vdp2_drawfb_cram_equal_color_add_f) + string(Yglprg_vdp2_drawfb_line_add_fv);
  compileShader(a.c_str(), "equal_color_add_line", NONE);

  a = string(Yglprg_vdp2_drawfb_cram_more_color_add_f) + string(Yglprg_vdp2_drawfb_line_add_fv);
  compileShader(a.c_str(), "more_color_add_line", NONE);

  a = string(Yglprg_vdp2_drawfb_cram_msb_color_add_f) + string(Yglprg_vdp2_drawfb_line_add_fv);
  compileShader(a.c_str(), "msb_color_add_line", NONE);

#if 0
  compileShader(Yglprg_vdp2_drawfb_cram_no_color_col_f, "hblank", NONE);
  compileShader(Yglprg_vdp2_drawfb_cram_destalpha_col_hblank_f, "hblank_destalpha", NONE);
  compileShader(Yglprg_vdp2_drawfb_cram_less_color_col_hblank_f, "less_color_col_hblank", NONE);
  compileShader(Yglprg_vdp2_drawfb_cram_equal_color_col_hblank_f, "equal_color_col_hblank", NONE);
  compileShader(Yglprg_vdp2_drawfb_cram_more_color_col_hblank_f, "equal_color_col_hblank", NONE);
  compileShader(Yglprg_vdp2_drawfb_cram_msb_color_col_hblank_f, "msb_color_col_hblank", NONE);
  compileShader(Yglprg_vdp2_drawfb_cram_less_color_add_f, "less_add_hblank", NONE);
  compileShader(Yglprg_vdp2_drawfb_cram_equal_color_add_f, "equal_add_hblank", NONE);
  compileShader(Yglprg_vdp2_drawfb_cram_more_color_add_f, "more_add_hblank", NONE);
  compileShader(Yglprg_vdp2_drawfb_cram_msb_color_add_f, "msb_add_hblank", NONE);
#endif  

}



FramebufferRenderer::FramebufferRenderer(VIDVulkan * vulkan) {
  this->vulkan = vulkan;

  Vertex a(glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
  vertices.push_back(a);
  Vertex b(glm::vec4(1.0f, -1.0f, 0.0f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
  vertices.push_back(b);
  Vertex c(glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
  vertices.push_back(c);
  Vertex d(glm::vec4(-1.0f, 1.0f, 0.0f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
  vertices.push_back(d);

  indices.push_back(0);
  indices.push_back(1);
  indices.push_back(2);
  indices.push_back(2);
  indices.push_back(3);
  indices.push_back(0);

  memset(&ubo, 0, sizeof(ubo));

  vertexShaderName = R"S(
    layout(binding = 0) uniform vdp2regs { 
     mat4 u_mvpMatrix;   
     float u_pri[8]; 
     float u_alpha[8]; 
     vec4 u_coloroffset;
     float u_cctl;
     float u_emu_height;
     float u_vheight;
     int u_color_ram_offset;
     float u_viewport_offset;
     int u_sprite_window;
     float u_from;
     float u_to;
    } ubo;

    layout(location = 0) in vec4 a_position;
    layout(location = 1) in vec4 inColor;
    layout(location = 2) in vec4 a_texcoord;

    layout (location = 0) out vec4 v_texcoord;

    void main() {
        v_texcoord  = a_texcoord;
        gl_Position = ubo.u_mvpMatrix * a_position;

    }
  )S";


  _vertexBuffer = 0;
  _vertexBufferMemory = 0;
  _indexBuffer = 0;
  _indexBufferMemory = 0;

  ubuffer.resize(MAX_RENDER_COUNT);

  memset(_descriptorSet, 0, sizeof(VkDescriptorSet) * MAX_RENDER_COUNT);
  _descriptorSetLayout = 0;
  _descriptorPool = 0;
  _vertShaderModule = 0;
  _fragShaderModule = 0;
  _pipelineLayout = 0;
  _graphicsPipeline = 0;
}

FramebufferRenderer::~FramebufferRenderer() {

}

void FramebufferRenderer::setup() {
  createVertexBuffer(vertices);
  createIndexBuffer(indices);
  createDescriptorSets();
  setupShaders();
}

void FramebufferRenderer::onStartFrame(Vdp2 * fixVdp2Regs, VkCommandBuffer commandBuffer) {
  renderCount = 0;
  updateVdp2Reg(fixVdp2Regs);
  if (lineTexture != VK_NULL_HANDLE) {
    perline.update(vulkan, commandBuffer);
  }
}

void FramebufferRenderer::onEndFrame() {

}

void FramebufferRenderer::updateVdp2Reg(Vdp2 * fixVdp2Regs) {

  int i, line;
  u8 *cclist = (u8 *)&fixVdp2Regs->CCRSA;
  u8 *prilist = (u8 *)&fixVdp2Regs->PRISA;

  int vdp1cor = 0;
  int vdp1cog = 0;
  int vdp1cob = 0;

  for (i = 0; i < 8; i++) {
    ubo.alpha[i * 4] = (float)(0xFF - (((cclist[i] & 0x1F) << 3) & 0xF8)) / 255.0f;
    ubo.pri[i * 4] = ((float)(prilist[i] & 0x7) / 10.0f) + 0.05f;
  }
  ubo.cctl = ((float)((fixVdp2Regs->SPCTL >> 8) & 0x07) / 10.0f) + 0.05f;

  if (*Vdp2External.perline_alpha_draw & 0x40) {
    u32 * linebuf;
    int line_shift = 0;
    if (vulkan->vdp2height > 256) {
      line_shift = 1;
    }
    else {
      line_shift = 0;
    }

    if (perline.dynamicBuf == nullptr) {
      perline.create(vulkan, 512, 1 + 8 + 8);
    }
    linebuf = perline.dynamicBuf;
    for (line = 0; line < vulkan->vdp2height; line++) {
      linebuf[line] = 0xFF000000;
      Vdp2 * lVdp2Regs = &Vdp2Lines[line >> line_shift];

      u8 *cclist = (u8 *)&lVdp2Regs->CCRSA;
      u8 *prilist = (u8 *)&lVdp2Regs->PRISA;
      for (i = 0; i < 8; i++) {
        linebuf[line + 512 * (1 + i)] = (prilist[i] & 0x7) << 24;
        linebuf[line + 512 * (1 + 8 + i)] = (0xFF - (((cclist[i] & 0x1F) << 3) & 0xF8)) << 24;
      }

      if (lVdp2Regs->CLOFEN & 0x40) {

        // color offset enable
        if (lVdp2Regs->CLOFSL & 0x40)
        {
          // color offset B
          vdp1cor = lVdp2Regs->COBR & 0xFF;
          if (lVdp2Regs->COBR & 0x100)
            vdp1cor |= 0xFFFFFF00;

          vdp1cog = lVdp2Regs->COBG & 0xFF;
          if (lVdp2Regs->COBG & 0x100)
            vdp1cog |= 0xFFFFFF00;

          vdp1cob = lVdp2Regs->COBB & 0xFF;
          if (lVdp2Regs->COBB & 0x100)
            vdp1cob |= 0xFFFFFF00;
        }
        else
        {
          // color offset A
          vdp1cor = lVdp2Regs->COAR & 0xFF;
          if (lVdp2Regs->COAR & 0x100)
            vdp1cor |= 0xFFFFFF00;

          vdp1cog = lVdp2Regs->COAG & 0xFF;
          if (lVdp2Regs->COAG & 0x100)
            vdp1cog |= 0xFFFFFF00;

          vdp1cob = lVdp2Regs->COAB & 0xFF;
          if (lVdp2Regs->COAB & 0x100)
            vdp1cob |= 0xFFFFFF00;
        }


        linebuf[line] |= ((int)(128.0f + (vdp1cor / 2.0)) & 0xFF) << 0;
        linebuf[line] |= ((int)(128.0f + (vdp1cog / 2.0)) & 0xFF) << 8;
        linebuf[line] |= ((int)(128.0f + (vdp1cob / 2.0)) & 0xFF) << 16;
      }
      else {
        linebuf[line] |= 0x00808080;
      }
    }
    lineTexture = perline.imageView;
  }
  else
  {
    lineTexture = 0;
    if (fixVdp2Regs->CLOFEN & 0x40)
    {
      // color offset enable
      if (fixVdp2Regs->CLOFSL & 0x40)
      {
        // color offset B
        vdp1cor = fixVdp2Regs->COBR & 0xFF;
        if (fixVdp2Regs->COBR & 0x100)
          vdp1cor |= 0xFFFFFF00;

        vdp1cog = fixVdp2Regs->COBG & 0xFF;
        if (fixVdp2Regs->COBG & 0x100)
          vdp1cog |= 0xFFFFFF00;

        vdp1cob = fixVdp2Regs->COBB & 0xFF;
        if (fixVdp2Regs->COBB & 0x100)
          vdp1cob |= 0xFFFFFF00;
      }
      else
      {
        // color offset A
        vdp1cor = fixVdp2Regs->COAR & 0xFF;
        if (fixVdp2Regs->COAR & 0x100)
          vdp1cor |= 0xFFFFFF00;

        vdp1cog = fixVdp2Regs->COAG & 0xFF;
        if (fixVdp2Regs->COAG & 0x100)
          vdp1cog |= 0xFFFFFF00;

        vdp1cob = fixVdp2Regs->COAB & 0xFF;
        if (fixVdp2Regs->COAB & 0x100)
          vdp1cob |= 0xFFFFFF00;
      }
    }
    else // color offset disable
      vdp1cor = vdp1cog = vdp1cob = 0;
  }


  ubo.coloroffset[0] = vdp1cor / 255.0f;
  ubo.coloroffset[1] = vdp1cog / 255.0f;
  ubo.coloroffset[2] = vdp1cob / 255.0f;
  ubo.coloroffset[3] = 0.0f;

  // For Line Color insersion
  ubo.emu_height = (float)vulkan->vdp2height / (float)vulkan->renderHeight;
  ubo.vheight = (float)vulkan->renderHeight;
  ubo.color_ram_offset = (fixVdp2Regs->CRAOFB & 0x70) << 4;
  //if (vulkan->resolutionMode == RES_NATIVE) {
  //  _Ygl->fbu_.u_viewport_offset = (float)_Ygl->originy;
  //}
  //else {
  ubo.viewport_offset = 0.0f;
  //}

  // Check if transparent sprite window
  // hard/vdp2/hon/p08_12.htm#SPWINEN_
  if ((fixVdp2Regs->SPCTL & 0x10) && // Sprite Window is enabled
    ((fixVdp2Regs->SPCTL & 0xF) >= 2 && (fixVdp2Regs->SPCTL & 0xF) < 8)) // inside sprite type
  {
    ubo.sprite_window = 1;
  }
  else {
    ubo.sprite_window = 0;
  }


}


void FramebufferRenderer::drawWithDestAlphaMode(Vdp2 * fixVdp2Regs, VkCommandBuffer commandBuffer, int from, int to) {

  const VkDevice device = vulkan->getDevice();
  glm::mat4 m4(1.0f);
  ubo.matrix = m4;
  VkPipeline pgid;


  u8 *sprprilist = (u8 *)&fixVdp2Regs->PRISA;
  int maxpri = 0x00;
  int minpri = 0x07;
  for (int i = 0; i < 8; i++)
  {
    if ((sprprilist[i] & 0x07) < minpri) minpri = (sprprilist[i] & 0x07);
    if ((sprprilist[i] & 0x07) > maxpri) maxpri = (sprprilist[i] & 0x07);
  }

  for (int i = from; i < to; i++) {

    ubo.from = i / 10.0f;
    ubo.to = (i + 1) / 10.0f;

    if (i > maxpri) break;
    if ((i + 1) < minpri) continue;

    void* data;
    vkMapMemory(device, ubuffer[renderCount].mem, 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(device, ubuffer[renderCount].mem);
    updateDescriptorSets(renderCount);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      _pipelineLayout, 0, 1, &_descriptorSet[this->renderCount], 0, nullptr);


    if (((fixVdp2Regs->CCCTL >> 6) & 0x01) == 0x01) {
      switch ((fixVdp2Regs->SPCTL >> 12) & 0x3) {
      case 0:
        if (i <= ((fixVdp2Regs->SPCTL >> 8) & 0x07)) {
          if (lineTexture != VK_NULL_HANDLE) {
            pgid = findShader("hblank_destalpha", Yglprg_vdp2_drawfb_cram_destalpha_col_hblank_f, DST_ALPHA);
          }
          else {
            pgid = findShader("destalpha_col", Yglprg_vdp2_drawfb_cram_destalpha_col_f, DST_ALPHA);
          }
        }
        else {
          pgid = findShader("destalpha_col_noblend", Yglprg_vdp2_drawfb_cram_destalpha_col_f, NONE);
        }
        break;
      case 1:
        if (i == ((fixVdp2Regs->SPCTL >> 8) & 0x07)) {
          if (lineTexture != VK_NULL_HANDLE) {
            pgid = findShader("hblank_destalpha", Yglprg_vdp2_drawfb_cram_destalpha_col_hblank_f, DST_ALPHA);
          }
          else {
            pgid = findShader("destalpha_col", Yglprg_vdp2_drawfb_cram_destalpha_col_f, DST_ALPHA);
          }
        }
        else {
          pgid = findShader("destalpha_col_noblend", Yglprg_vdp2_drawfb_cram_destalpha_col_f, NONE);
        }
        break;
      case 2:
        if (i >= ((fixVdp2Regs->SPCTL >> 8) & 0x07)) {
          if (lineTexture != VK_NULL_HANDLE) {
            pgid = findShader("hblank_destalpha", Yglprg_vdp2_drawfb_cram_destalpha_col_hblank_f, DST_ALPHA);
          }
          else {
            pgid = findShader("destalpha_col", Yglprg_vdp2_drawfb_cram_destalpha_col_f, DST_ALPHA);
          }

        }
        else {
          pgid = findShader("destalpha_col_noblend", Yglprg_vdp2_drawfb_cram_destalpha_col_f, NONE);
        }
        break;
      case 3:
        // ToDO: MSB color cacuration
        if (lineTexture != VK_NULL_HANDLE) {
          pgid = findShader("hblank_destalpha", Yglprg_vdp2_drawfb_cram_destalpha_col_hblank_f, DST_ALPHA);
        }
        else {
          pgid = findShader("destalpha_col", Yglprg_vdp2_drawfb_cram_destalpha_col_f, DST_ALPHA);
        }
        break;
      }
      //      default:
      //        continue;
    }
    else {
      pgid = findShader("destalpha_col", Yglprg_vdp2_drawfb_cram_destalpha_col_f, DST_ALPHA);
    }


    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pgid);

    VkBuffer vertexBuffers[] = { _vertexBuffer };
    VkDeviceSize offsets[] = { 0 };

    vkCmdBindVertexBuffers(commandBuffer,
      0, 1, vertexBuffers, offsets);

    vkCmdBindIndexBuffer(commandBuffer,
      _indexBuffer,
      0, VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(commandBuffer, indices.size(), 1, 0, 0, 0);

    renderCount++;

  }
}

void FramebufferRenderer::draw(Vdp2 * fixVdp2Regs, VkCommandBuffer commandBuffer, int from, int to) {

  const VkDevice device = vulkan->getDevice();

  glm::mat4 m4(1.0f);
  ubo.matrix = m4; // glm::ortho(0.0f, (float)352, (float)224, 0.0f, 10.0f, 0.0f);
  ubo.from = from / 10.0f;
  ubo.to = to / 10.0f;

  void* data;
  vkMapMemory(device, ubuffer[renderCount].mem, 0, sizeof(ubo), 0, &data);
  memcpy(data, &ubo, sizeof(ubo));
  vkUnmapMemory(device, ubuffer[renderCount].mem);

  updateDescriptorSets(renderCount);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
    _pipelineLayout, 0, 1, &_descriptorSet[this->renderCount], 0, nullptr);

  //----------------------------------------------------------------------------------
  VkPipeline pgid = findShader("no_color_col", Yglprg_vdp2_drawfb_cram_no_color_col_f, NONE);

  const int SPCCN = ((fixVdp2Regs->CCCTL >> 6) & 0x01); // hard/vdp2/hon/p12_14.htm#NxCCEN_
  const int CCRTMD = ((fixVdp2Regs->CCCTL >> 9) & 0x01); // hard/vdp2/hon/p12_14.htm#CCRTMD_
  const int CCMD = ((fixVdp2Regs->CCCTL >> 8) & 0x01);  // hard/vdp2/hon/p12_14.htm#CCMD_
  const int SPLCEN = (fixVdp2Regs->LNCLEN & 0x20); // hard/vdp2/hon/p11_30.htm#NxLCEN_
  int blend = 1;
  if (blend && SPCCN) {
    const int SPCCCS = (fixVdp2Regs->SPCTL >> 12) & 0x3;
    if (CCMD == 0) {  // Calculate Rate mode
      if (CCRTMD == 0) {  // Source Alpha Mode
        if (SPLCEN == 0) { // No Line Color Insertion
          switch (SPCCCS)
          {
          case 0:
            if (lineTexture != VK_NULL_HANDLE) {
              pgid = findShader("less_color_col_hblank", Yglprg_vdp2_drawfb_cram_less_color_col_hblank_f, SRC_ALPHA);
            }
            else {
              pgid = findShader("less_color_col", Yglprg_vdp2_drawfb_cram_less_color_col_f, SRC_ALPHA);
            }
            break;
          case 1:
            if (lineTexture != VK_NULL_HANDLE) {
              pgid = findShader("equal_color_col_hblank", Yglprg_vdp2_drawfb_cram_equal_color_col_hblank_f, SRC_ALPHA);
            }
            else {
              pgid = findShader("equal_color_col", Yglprg_vdp2_drawfb_cram_equal_color_col_f, SRC_ALPHA);
            }
            break;
          case 2:
            if (lineTexture != VK_NULL_HANDLE) {
              pgid = findShader("more_color_col_hblank", Yglprg_vdp2_drawfb_cram_more_color_col_hblank_f, SRC_ALPHA);
            }
            else {
              pgid = findShader("more_color_col", Yglprg_vdp2_drawfb_cram_more_color_col_f, SRC_ALPHA);
            }
            break;
          case 3:
            if (lineTexture != VK_NULL_HANDLE) {
              pgid = findShader("msb_color_col_hblank", Yglprg_vdp2_drawfb_cram_msb_color_col_hblank_f, SRC_ALPHA);
            }
            else {
              pgid = findShader("msb_color_col", Yglprg_vdp2_drawfb_cram_msb_color_col_f, SRC_ALPHA);
            }
            break;
          }
        }
        else { // Line Color Insertion
          switch (SPCCCS)
          {
          case 0:
            //pgid = PG_VDP2_DRAWFRAMEBUFF_LESS_CCOL_LINE;
            pgid = pipelines["less_color_col_line"];
            break;
          case 1:
            //pgid = PG_VDP2_DRAWFRAMEBUFF_EUQAL_CCOL_LINE;
            pgid = pipelines["equal_color_col_line"];
            break;
          case 2:
            //pgid = PG_VDP2_DRAWFRAMEBUFF_MORE_CCOL_LINE;
            pgid = pipelines["more_color_col_line"];
            break;
          case 3:
            //pgid = PG_VDP2_DRAWFRAMEBUFF_MSB_CCOL_LINE;
            pgid = pipelines["msb_color_col_line"];
            break;
          }
        }
      }
      else { // Destination Alpha Mode

        if (SPLCEN == 0) { // No Line Color Insertion
          //pgid = PG_VDP2_DRAWFRAMEBUFF_DESTALPHA;
          if (lineTexture != VK_NULL_HANDLE) {
            pgid = findShader("hblank_destalpha", Yglprg_vdp2_drawfb_cram_destalpha_col_hblank_f, DST_ALPHA);
          }
          else {
            pgid = findShader("destalpha_col", Yglprg_vdp2_drawfb_cram_destalpha_col_f, DST_ALPHA);
          }
        }
        else {
          switch (SPCCCS)
          {
          case 0:
            //pgid = PG_VDP2_DRAWFRAMEBUFF_LESS_DESTALPHA_LINE;
            pgid = pipelines["less_destalpha_line"];
            break;
          case 1:
            //pgid = PG_VDP2_DRAWFRAMEBUFF_EQUAL_DESTALPHA_LINE;
            pgid = pipelines["equal_destalpha_line"];
            break;
          case 2:
            //pgid = PG_VDP2_DRAWFRAMEBUFF_MORE_DESTALPHA_LINE;
            pgid = pipelines["more_destalpha_line"];
            break;
          case 3:
            //pgid = PG_VDP2_DRAWFRAMEBUFF_MSB_DESTALPHA_LINE;
            pgid = pipelines["msb_destalpha_line"];
            break;
          }
        }
      }
    }
    else { // Add Color Mode
      //glEnable(GL_BLEND);
      //glBlendFunc(GL_ONE, GL_SRC_ALPHA);
      if (SPLCEN == 0) { // No Line Color Insertion
        switch (SPCCCS)
        {
        case 0:
          //pgid = PG_VDP2_DRAWFRAMEBUFF_LESS_ADD;
          if (lineTexture != VK_NULL_HANDLE) {
            pgid = findShader("less_add_hblank", Yglprg_vdp2_drawfb_cram_less_color_add_f, ADD);
          }
          else {
            pgid = findShader("less_color_add", Yglprg_vdp2_drawfb_cram_less_color_add_f, ADD);
          }
          break;
        case 1:
          //pgid = PG_VDP2_DRAWFRAMEBUFF_EUQAL_ADD;
          if (lineTexture != VK_NULL_HANDLE) {
            pgid = findShader("equal_add_hblank", Yglprg_vdp2_drawfb_cram_equal_color_add_f, ADD);
          }
          else {
            pgid = findShader("equal_color_add", Yglprg_vdp2_drawfb_cram_equal_color_add_f, ADD);
          }
          break;
        case 2:
          //pgid = PG_VDP2_DRAWFRAMEBUFF_MORE_ADD;
          if (lineTexture != VK_NULL_HANDLE) {
            pgid = findShader("more_add_hblank", Yglprg_vdp2_drawfb_cram_more_color_add_f, ADD);
          }
          else {
            pgid = findShader("more_color_add", Yglprg_vdp2_drawfb_cram_more_color_add_f, ADD);
          }
          break;
        case 3:
          //pgid = PG_VDP2_DRAWFRAMEBUFF_MSB_ADD;
          if (lineTexture != VK_NULL_HANDLE) {
            pgid = findShader("msb_add_hblank", Yglprg_vdp2_drawfb_cram_msb_color_add_f, ADD);
          }
          else {
            pgid = findShader("msb_color_add", Yglprg_vdp2_drawfb_cram_msb_color_add_f, ADD);
          }
          break;
        }
      }
      else {
        switch (SPCCCS)
        {
        case 0:
          //pgid = PG_VDP2_DRAWFRAMEBUFF_LESS_ADD_LINE;
          pgid = pipelines["less_color_add_line"];
          break;
        case 1:
          //pgid = PG_VDP2_DRAWFRAMEBUFF_EUQAL_ADD_LINE;
          pgid = pipelines["less_color_add_line"];
          break;
        case 2:
          //pgid = PG_VDP2_DRAWFRAMEBUFF_MORE_ADD_LINE;
          pgid = pipelines["more_color_add_line"];
          break;
        case 3:
          //pgid = PG_VDP2_DRAWFRAMEBUFF_MSB_ADD_LINE;
          pgid = pipelines["msb_color_add_line"];
          break;
        }
      }
    }
  }
  else { // No Color Calculation
    //glDisable(GL_BLEND);
    //pgid = PG_VDP2_DRAWFRAMEBUFF;
    if (lineTexture != VK_NULL_HANDLE) {
      pgid = findShader("hblank", Yglprg_vdp2_drawfb_cram_no_color_col_f, NONE);
    }
    else {
      pgid = findShader("no_color_col", Yglprg_vdp2_drawfb_cram_no_color_col_f, NONE);
    }
  }


  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pgid);

  VkBuffer vertexBuffers[] = { _vertexBuffer };
  VkDeviceSize offsets[] = { 0 };

  vkCmdBindVertexBuffers(commandBuffer,
    0, 1, vertexBuffers, offsets);

  vkCmdBindIndexBuffer(commandBuffer,
    _indexBuffer,
    0, VK_INDEX_TYPE_UINT16);

  vkCmdDrawIndexed(commandBuffer, indices.size(), 1, 0, 0, 0);

  renderCount++;
}


void FramebufferRenderer::drawShadow(Vdp2 * fixVdp2Regs, VkCommandBuffer commandBuffer, int from, int to) {

  const VkDevice device = vulkan->getDevice();

  glm::mat4 m4(1.0f);
  ubo.matrix = m4; // glm::ortho(0.0f, (float)352, (float)224, 0.0f, 10.0f, 0.0f);
  ubo.from = from / 10.0f;
  ubo.to = to / 10.0f;

  void* data;
  vkMapMemory(device, ubuffer[renderCount].mem, 0, sizeof(ubo), 0, &data);
  memcpy(data, &ubo, sizeof(ubo));
  vkUnmapMemory(device, ubuffer[renderCount].mem);


  updateDescriptorSets(renderCount);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
    _pipelineLayout, 0, 1, &_descriptorSet[this->renderCount], 0, nullptr);

  const GLchar drawfb_shadow_f[] =
#if defined(_OGLES3_)
    "#version 310 es \n"
    "precision highp sampler2D; \n"
    "precision highp float;\n"
#else
    "#version 430 \n"
#endif
    "layout(binding = 0) uniform vdp2regs { \n"
    " mat4 matrix; \n"
    " float u_pri[8]; \n"
    " float u_alpha[8]; \n"
    " vec4 u_coloroffset;\n"
    " float u_cctl; \n"
    " float u_emu_height; \n"
    " float u_vheight; \n"
    " int u_color_ram_offset; \n"
    " float u_viewport_offset; \n"
    " int u_sprite_window; \n"
    " float u_from;\n"
    " float u_to;\n"
    "}; \n"
    "layout(binding = 1) uniform highp sampler2D s_vdp1FrameBuffer;\n"
    "layout(binding = 2) uniform sampler2D s_color; \n"
    "layout(binding = 3) uniform sampler2D s_line; \n"
    "layout(location = 0) in vec2 v_texcoord;\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "void main()\n"
    "{\n"
    "  vec2 addr = v_texcoord;\n"
    "  highp vec4 fbColor = texture(s_vdp1FrameBuffer,addr);\n"
    "  int additional = int(fbColor.a * 255.0);\n"
    "  if( (additional & 0x80) == 0 ){ discard; } // show? \n"
    "  highp float depth = u_pri[ (additional&0x07) ];\n"
    "  if( (additional & 0x40) != 0 && fbColor.b != 0.0 ){  // index color and shadow? \n"
    "    fragColor = vec4(0.0,0.0,0.0,0.5);\n"
    "  }else{ // direct color \n"
    "    discard;\n"
    "  } \n"
    "  gl_FragDepth = depth;\n"
    "}\n";


  //----------------------------------------------------------------------------------
  VkPipeline pgid = findShader("shadow", drawfb_shadow_f, SRC_ALPHA);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pgid);

  VkBuffer vertexBuffers[] = { _vertexBuffer };
  VkDeviceSize offsets[] = { 0 };

  vkCmdBindVertexBuffers(commandBuffer,
    0, 1, vertexBuffers, offsets);

  vkCmdBindIndexBuffer(commandBuffer,
    _indexBuffer,
    0, VK_INDEX_TYPE_UINT16);

  vkCmdDrawIndexed(commandBuffer, indices.size(), 1, 0, 0, 0);

  renderCount++;
}


//----------------------------------------------------------------------------------------
// protected

void FramebufferRenderer::createVertexBuffer(const std::vector<Vertex> & vertices) {
  const VkDevice device = vulkan->getDevice();

  VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  vulkan->createBuffer(bufferSize,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    stagingBuffer, stagingBufferMemory);

  void* data;
  vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, vertices.data(), (size_t)bufferSize);
  vkUnmapMemory(device, stagingBufferMemory);

  vulkan->createBuffer(bufferSize,
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    _vertexBuffer, _vertexBufferMemory);

  vulkan->copyBuffer(stagingBuffer, _vertexBuffer, bufferSize);

  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);

}

void FramebufferRenderer::createIndexBuffer(const std::vector<uint16_t> & indices) {

  const VkDevice device = vulkan->getDevice();
  VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  vulkan->createBuffer(bufferSize,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    stagingBuffer,
    stagingBufferMemory);

  void* data;
  vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, indices.data(), (size_t)bufferSize);
  vkUnmapMemory(device, stagingBufferMemory);

  vulkan->createBuffer(bufferSize,
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _indexBuffer, _indexBufferMemory);

  vulkan->copyBuffer(stagingBuffer, _indexBuffer, bufferSize);

  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);

}

#include <array>
#include <chrono>
#include <iostream>
#include <fstream>

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

VkPipeline FramebufferRenderer::compileShader(const char * code, const char * name, enum ColorClacMode c) {

  VkDevice device = vulkan->getDevice();

  string sname = string(name);

  string target;

  if (sname == "shadow") {
    target = string(code);
  }
  else {
    if (sname.find("hblank") != string::npos) {
      target = Yglprg_vdp2_drawfb_hblank_vulkan_f;
    }
    else {
      target = Yglprg_vdp2_drawfb_cram_vulkan_f;
    }

    target += string(code);
    target += Yglprg_vdp2_drawfb_cram_eiploge_vulkan_f;
  }


  LOGI("%s%s", "compiling: ", name);

  Compiler compiler;
  CompileOptions options;
  options.SetOptimizationLevel(shaderc_optimization_level_performance);
  //options.SetOptimizationLevel(shaderc_optimization_level_zero);
  SpvCompilationResult result = compiler.CompileGlslToSpv(
    target,
    shaderc_fragment_shader,
    name,
    options);

  LOGI("%s%d\n", " erros: ", (int)result.GetNumErrors());
  if (result.GetNumErrors() != 0) {
    LOGI("%s%s\n", "messages", result.GetErrorMessage().c_str());
    throw std::runtime_error("failed to create shader module!");
  }
  std::vector<uint32_t> data = { result.cbegin(), result.cend() };

  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = data.size() * sizeof(uint32_t);
  createInfo.pCode = data.data();
  VkShaderModule shaderModule;
  if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module!");
  }

  //auto vertShaderCode = readFile(vertexShaderName);
  //_vertShaderModule = vulkan->createShaderModule(vertShaderCode);


  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
  vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = _vertShaderModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
  fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = shaderModule;
  fragShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 0;
  vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
  vertexInputInfo.vertexAttributeDescriptionCount = 0;
  vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  auto bindingDescription = Vertex::getBindingDescription();
  auto attributeDescriptions = Vertex::getAttributeDescriptions();

  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();


  VkDynamicState dynamicStates[] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR,
  };

  VkPipelineDynamicStateCreateInfo dynamicState = {};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = 2; // 2;
  dynamicState.pDynamicStates = dynamicStates; // dynamicStates;

  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = &depthStencil;
  if (c == SRC_ALPHA) {
    pipelineInfo.pColorBlendState = &colorBlending;
  }
  else if (c == ADD) {
    pipelineInfo.pColorBlendState = &colorBlendingAdd;
  }
  else if (c == DST_ALPHA) {
    pipelineInfo.pColorBlendState = &colorBlendingDestAlpha;
  }
  else {
    pipelineInfo.pColorBlendState = &colorBlendingNone;
  }
  pipelineInfo.pDynamicState = &dynamicState; // Optional
  pipelineInfo.layout = _pipelineLayout;
  pipelineInfo.renderPass = vulkan->getRenderPass();
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
  pipelineInfo.basePipelineIndex = -1; // Optional

  VkPipeline graphicsPipeline;;
  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
    throw std::runtime_error("failed to create graphics pipeline!");
  }

  pipelines[name] = graphicsPipeline;

  return graphicsPipeline;

}


void FramebufferRenderer::createDescriptorSets() {

  VkDevice device = vulkan->getDevice();

  VkDeviceSize bufferSize = sizeof(ubo);

  for (UniformBuffer & u : ubuffer) {
    vulkan->createBuffer(bufferSize,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      u.buf, u.mem);
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
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 0.0f;
  if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
    throw std::runtime_error("failed to create texture sampler!");
  }

  // _descriptorSetLayout 
  VkDescriptorSetLayoutBinding uboLayoutBinding = {};
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

  VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
  samplerLayoutBinding.binding = 1;
  samplerLayoutBinding.descriptorCount = 1;
  samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.pImmutableSamplers = nullptr;
  samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutBinding samplerColorLayoutBinding = {};
  samplerColorLayoutBinding.binding = 2;
  samplerColorLayoutBinding.descriptorCount = 1;
  samplerColorLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerColorLayoutBinding.pImmutableSamplers = nullptr;
  samplerColorLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutBinding samplerLineLayoutBinding = {};
  samplerLineLayoutBinding.binding = 3;
  samplerLineLayoutBinding.descriptorCount = 1;
  samplerLineLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLineLayoutBinding.pImmutableSamplers = nullptr;
  samplerLineLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;


  std::array<VkDescriptorSetLayoutBinding, 4> bindings = {
    uboLayoutBinding, samplerLayoutBinding, samplerColorLayoutBinding,samplerLineLayoutBinding
  };

  VkDescriptorSetLayoutCreateInfo layoutInfo = {};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();

  if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &_descriptorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
  }

  // _descriptorPool
  std::array<VkDescriptorPoolSize, 4> poolSizes = {};
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = MAX_RENDER_COUNT;
  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount = MAX_RENDER_COUNT;
  poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[2].descriptorCount = MAX_RENDER_COUNT;
  poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[3].descriptorCount = MAX_RENDER_COUNT;

  VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = MAX_RENDER_COUNT;

  if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &_descriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool!");
  }

  VkDescriptorSetLayout layouts[] = { _descriptorSetLayout };
  VkDescriptorSetAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = _descriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = layouts;

  for (int i = 0; i < MAX_RENDER_COUNT; i++) {
    if (vkAllocateDescriptorSets(device, &allocInfo, &_descriptorSet[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to allocate descriptor set!");
    }
  }

  Compiler compiler;
  CompileOptions options;
  options.SetOptimizationLevel(shaderc_optimization_level_performance);
  //options.SetOptimizationLevel(shaderc_optimization_level_zero);
  SpvCompilationResult result = compiler.CompileGlslToSpv(
    get_shader_header() + vertexShaderName,
    shaderc_vertex_shader,
    "framebuffer",
    options);

  std::cout << " erros: " << result.GetNumErrors() << std::endl;
  if (result.GetNumErrors() != 0) {
    std::cout << "messages: " << result.GetErrorMessage() << std::endl;
    throw std::runtime_error("failed to create shader module!");
  }
  std::vector<uint32_t> data = { result.cbegin(), result.cend() };

  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = data.size() * sizeof(uint32_t);
  createInfo.pCode = data.data();
  if (vkCreateShaderModule(device, &createInfo, nullptr, &_vertShaderModule) != VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module!");
  }


#if 0
  auto vertShaderCode = readFile(vertexShaderName);
  auto fragShaderCode = readFile(fragShaderName);
  _vertShaderModule = vulkan->createShaderModule(vertShaderCode);
  _fragShaderModule = vulkan->createShaderModule(fragShaderCode);


  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
  vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = _vertShaderModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
  fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = _fragShaderModule;
  fragShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
#endif

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 0;
  vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
  vertexInputInfo.vertexAttributeDescriptionCount = 0;
  vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkRect2D render_area{};
  render_area.offset.x = 0;
  render_area.offset.y = 0;
  render_area.extent = vulkan->getSurfaceSize();


  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)render_area.extent.width;
  viewport.height = (float)render_area.extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;


  scissor.offset = { 0, 0 };
  scissor.extent = render_area.extent;


  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;


  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;

  //rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  //rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

  rasterizer.cullMode = VK_CULL_MODE_NONE;
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f; // Optional
  rasterizer.depthBiasClamp = 0.0f; // Optional
  rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading = 1.0f; // Optional
  multisampling.pSampleMask = nullptr; // Optional
  multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
  multisampling.alphaToOneEnable = VK_FALSE; // Optional
                                             // ToDo


  depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable = VK_TRUE;
  depthStencil.depthWriteEnable = VK_TRUE;
  depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL; // VK_COMPARE_OP_LESS;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.minDepthBounds = 0.0f; // Optional
  depthStencil.maxDepthBounds = 1.0f; // Optional
  depthStencil.stencilTestEnable = VK_FALSE;
  depthStencil.front = {}; // Optional
  depthStencil.back = {}; // Optional


/*
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
*/

  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_TRUE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;


  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f; // Optional
  colorBlending.blendConstants[1] = 0.0f; // Optional
  colorBlending.blendConstants[2] = 0.0f; // Optional
  colorBlending.blendConstants[3] = 0.0f; // Optional

  colorBlendAttachmentAdd.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachmentAdd.blendEnable = VK_TRUE;
  colorBlendAttachmentAdd.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachmentAdd.dstColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachmentAdd.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachmentAdd.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachmentAdd.dstAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachmentAdd.alphaBlendOp = VK_BLEND_OP_ADD;

  colorBlendingAdd.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlendingAdd.logicOpEnable = VK_FALSE;
  colorBlendingAdd.logicOp = VK_LOGIC_OP_COPY; // Optional
  colorBlendingAdd.attachmentCount = 1;
  colorBlendingAdd.pAttachments = &colorBlendAttachmentAdd;
  colorBlendingAdd.blendConstants[0] = 0.0f; // Optional
  colorBlendingAdd.blendConstants[1] = 0.0f; // Optional
  colorBlendingAdd.blendConstants[2] = 0.0f; // Optional
  colorBlendingAdd.blendConstants[3] = 0.0f; // Optional


  colorBlendAttachmentNone.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachmentNone.blendEnable = VK_FALSE;
  colorBlendAttachmentNone.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachmentNone.dstColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachmentNone.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachmentNone.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachmentNone.dstAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachmentNone.alphaBlendOp = VK_BLEND_OP_ADD;

  colorBlendingNone.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlendingNone.logicOpEnable = VK_FALSE;
  colorBlendingNone.logicOp = VK_LOGIC_OP_COPY; // Optional
  colorBlendingNone.attachmentCount = 1;
  colorBlendingNone.pAttachments = &colorBlendAttachmentNone;
  colorBlendingNone.blendConstants[0] = 0.0f; // Optional
  colorBlendingNone.blendConstants[1] = 0.0f; // Optional
  colorBlendingNone.blendConstants[2] = 0.0f; // Optional
  colorBlendingNone.blendConstants[3] = 0.0f; // Optional

  colorBlendAttachmentDestAlpha.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachmentDestAlpha.blendEnable = VK_TRUE;
  colorBlendAttachmentDestAlpha.srcColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
  colorBlendAttachmentDestAlpha.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
  colorBlendAttachmentDestAlpha.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachmentDestAlpha.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachmentDestAlpha.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachmentDestAlpha.alphaBlendOp = VK_BLEND_OP_ADD;

  colorBlendingDestAlpha.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlendingDestAlpha.logicOpEnable = VK_FALSE;
  colorBlendingDestAlpha.logicOp = VK_LOGIC_OP_COPY; // Optional
  colorBlendingDestAlpha.attachmentCount = 1;
  colorBlendingDestAlpha.pAttachments = &colorBlendAttachmentDestAlpha;
  colorBlendingDestAlpha.blendConstants[0] = 0.0f; // Optional
  colorBlendingDestAlpha.blendConstants[1] = 0.0f; // Optional
  colorBlendingDestAlpha.blendConstants[2] = 0.0f; // Optional
  colorBlendingDestAlpha.blendConstants[3] = 0.0f; // Optional


  VkDynamicState dynamicStates[] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR,
  };

  VkPipelineDynamicStateCreateInfo dynamicState = {};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = 2; // 2;
  dynamicState.pDynamicStates = dynamicStates; // dynamicStates;

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
  pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &_descriptorSetLayout;

  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &_pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
  }

  auto bindingDescription = Vertex::getBindingDescription();
  auto attributeDescriptions = Vertex::getAttributeDescriptions();

  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

#if 0
  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = nullptr; // Optional
  pipelineInfo.layout = _pipelineLayout;
  pipelineInfo.renderPass = vulkan->getRenderPass();
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
  pipelineInfo.basePipelineIndex = -1; // Optional

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_graphicsPipeline) != VK_SUCCESS) {
    throw std::runtime_error("failed to create graphics pipeline!");
  }
#endif

}

void FramebufferRenderer::updateDescriptorSets(int index) {
  VkDevice device = vulkan->getDevice();

  VkDescriptorBufferInfo bufferInfo = {};
  bufferInfo.buffer = ubuffer[index].buf;
  bufferInfo.offset = 0;
  bufferInfo.range = sizeof(ubo);

  VkDescriptorImageInfo imageInfo = {};
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo.imageView = framebuffer;
  imageInfo.sampler = sampler;

  VkDescriptorImageInfo imageInfoCram = {};
  imageInfoCram.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfoCram.imageView = cram;
  imageInfoCram.sampler = sampler;

  VkDescriptorImageInfo imageInfoLine = {};
  imageInfoLine.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  if (lineTexture != VK_NULL_HANDLE) {
    imageInfoLine.imageView = lineTexture;
  }
  else {
    imageInfoLine.imageView = lineColor;
  }
  imageInfoLine.sampler = sampler;

  std::array<VkWriteDescriptorSet, 4> descriptorWrites = {};

  descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[0].dstSet = _descriptorSet[index];
  descriptorWrites[0].dstBinding = 0;
  descriptorWrites[0].dstArrayElement = 0;
  descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptorWrites[0].descriptorCount = 1;
  descriptorWrites[0].pBufferInfo = &bufferInfo;

  descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[1].dstSet = _descriptorSet[index];
  descriptorWrites[1].dstBinding = 1;
  descriptorWrites[1].dstArrayElement = 0;
  descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorWrites[1].descriptorCount = 1;
  descriptorWrites[1].pImageInfo = &imageInfo;

  descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[2].dstSet = _descriptorSet[index];
  descriptorWrites[2].dstBinding = 2;
  descriptorWrites[2].dstArrayElement = 0;
  descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorWrites[2].descriptorCount = 1;
  descriptorWrites[2].pImageInfo = &imageInfoCram;

  descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[3].dstSet = _descriptorSet[index];
  descriptorWrites[3].dstBinding = 3;
  descriptorWrites[3].dstArrayElement = 0;
  descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorWrites[3].descriptorCount = 1;
  descriptorWrites[3].pImageInfo = &imageInfoLine;

  vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}



