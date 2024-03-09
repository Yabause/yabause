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

#include "VulkanScene.h"
class VIDVulkan;

#include <vector>
using std::vector;

#include <string>
using std::string;

#include <map>
using std::map;


class TextureManager;
class VertexManager;

#define MAX_UBO_SIZE (256)
#define MAX_DS_SIZE (4)

class ShaderManager {

public:
  static ShaderManager * getInstance() {
    if (instance == nullptr) {
      instance = new ShaderManager();
    }
    return instance;
  }

  static void free() {
    if (instance != nullptr) {
      delete instance;
    }
    instance = nullptr;
  }

  ~ShaderManager();

  void setVulkan(VIDVulkan * vulkan) {
    this->vulkan = vulkan;
  }

  VkShaderModule getShader(uint32_t id);
  std::string get_shader_header();
  

  VkShaderModule compileShader(uint32_t id, const string & code, int type);

private:
  VIDVulkan * vulkan;
  static ShaderManager * instance;
  ShaderManager() {

  }

  map<uint32_t, VkShaderModule> shaders;

};


class VdpPipeline {
public:

  static VkPipelineCache threadPipelineCache;

  VdpPipeline(
    VIDVulkan * vulkan,
    TextureManager * tm,
    VertexManager * vm
  );
  ~VdpPipeline();

  YglPipelineId prgid;
  int priority;

  //GLuint prg;
  //GLuint vertexBuffer;
  uint32_t vectexBlock;
  uint32_t vertexOffset;
  uint32_t vertexSize;
  uint32_t indexOffset;
  uint32_t indexSize;

  vector<Vertex> vertices;
  vector<uint16_t> indices;

  struct UniformBuffer {
    VkBuffer _uniformBuffer;  
    VkDeviceMemory _uniformBufferMemory = 0;
  };
  std::vector<UniformBuffer> ubuffer;
  int dsIndex = 0;

  uint32_t uboSize;
  void setUBO(const void * ubo, int size);

  
  


  struct SamplerSet {
    VkImageView img;
    VkSampler smp;
  };

  SamplerSet samplers[32];

  static const int bindIdTexture;
  static const int bindIdColorRam;
  static const int bindIdFbo;
  static const int bindIdLine;
  static const int bindIdWindow;

  void setSampler(int id, VkImageView img, VkSampler smp) {
    samplers[id].img = img;
    samplers[id].smp = smp;
  }

  void setRenderPass(VkRenderPass renderPass) {
    this->renderPass = renderPass;
  }

  void moveToVertexBuffer(const vector<Vertex> & vertices, const vector<uint16_t> & indices);

  int vaid;
  char uClipMode;
  short ux1, uy1, ux2, uy2;
  int blendmode;
  int preblendmode;
  int bwin0, logwin0, bwin1, logwin1, bwinsp, logwinsp, winmode;
  GLuint vertexp;
  GLuint texcoordp;
  GLuint mtxModelView;
  GLuint mtxTexture;
  GLuint color_offset;
  GLuint tex0;
  GLuint tex1;
  float color_offset_val[4];
  int specialPriority = 0;
  int specialcolormode = 0;

  YglVdp1CommonParam * ids;
  float * matrix;
  int mosaic[2];
  VkImageView lineTexture = VK_NULL_HANDLE;
  int id;
  int colornumber;

  string vdp2Uniform;

  void createGraphicsPipeline();
  virtual void updateDescriptorSets();


  VkPipelineLayout getPipelineLayout() { return _pipelineLayout; }
  VkPipeline getGraphicsPipeline() { return _graphicsPipeline; }
  VkDescriptorSet * getDescriptorSet() { return &_descriptorSet[dsIndex]; }

  VkDescriptorSet _descriptorSet[MAX_DS_SIZE];

  std::string get_shader_header();
  VkShaderModule compileShader(const string & code, int type);

  VkImageView interuput_texture = VK_NULL_HANDLE;

  bool isNeedBarrier() { return needBarrier; }

  uint8_t getWinFlg() { return winflag;  }
  void setWinFlg(uint8_t flg) { winflag = flg; }

  static uint8_t genWinFlag(int winmode, int bwin0, int bwin1, int bwinsp, int logwin0, int logwin1, int logwinsp) {
    return(((winmode & 0x1) << 7) | ((bwin0 & 0x1) << 3) | ((bwin1 & 0x1) << 4) | ((bwinsp & 0x1) << 5)
      | ((logwin0 & 0x1) << 0) | ((logwin1 & 0x1) << 1) | ((logwinsp & 0x1) << 2));
  }

protected:

  VIDVulkan * vulkan;
  TextureManager * tm;
  VertexManager * vm;

  VkRenderPass renderPass;

  std::vector<char> readFile(const std::string& filename);

  string vertexShaderName;
  string fragShaderName;
  string tessControll;
  string tessEvaluation;
  string geometry;

  string fragFuncCheckWindow;


  VkDescriptorSetLayout _descriptorSetLayout;
  VkDescriptorPool _descriptorPool;
  VkShaderModule _vertShaderModule;
  VkShaderModule _fragShaderModule;
  VkShaderModule tessControllModule;
  VkShaderModule tessEvaluationModule;
  VkShaderModule geometryModule;
  VkPipelineLayout _pipelineLayout;
  VkPipeline _graphicsPipeline;

  virtual void createColorAttachment(VkPipelineColorBlendAttachmentState & colorAttachment);
  virtual void createDpethStencil(VkPipelineDepthStencilStateCreateInfo & depthStencil);
  virtual void createInputAssembly(VkPipelineInputAssemblyStateCreateInfo & inputAssembly);
  virtual void createColorBlending(VkPipelineColorBlendStateCreateInfo & colorBlending, VkPipelineColorBlendAttachmentState & colorAttachment);


  void initDescriptorSets(const vector<int> & bindid);


  vector<VkDynamicState> dynamicStates;
  vector<int> bindid;

  bool needBarrier = false;

  /*
  if (bwin0 || bwin1 || bwinsp)
  {
	  glEnable(GL_STENCIL_TEST);
	  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	  int winmask = (bwin0 | bwin1 | bwinsp);
	  int winflag = 0;
	  if (winmode == 0) { // and
		  if (bwin0)  winflag = logwin0;
		  if (bwin1)  winflag |= logwin1;
		  if (bwinsp) winflag |= logwinsp;
		  glStencilFunc(GL_EQUAL, winflag, winmask);
	  }
	  else { // or
		  winflag = winmask;
		  if (bwin0)  winflag &= ~logwin0;
		  if (bwin1)  winflag &= ~logwin1;
		  if (bwinsp) winflag &= ~logwinsp;
		  glStencilFunc(GL_NOTEQUAL, winflag, winmask);
	  }
  }
  */

  // bit spec
  // 8 winmode
  // 7
  // 6
  // 5 bwinsp
  // 4 bwin1
  // 3 bwin0
  // 2 logwinsp
  // 1 logwin1
  // 0 logwin0
  uint8_t winflag;


  void decodeWinFlag(int & winmode, int & bwin0, int & bwin1, int & bwinsp, int & logwin0, int & logwin1, int & logwinsp ) {
	  winmode = (winflag >> 7) & 0x01;
	  bwin0 = (winflag >> 3) & 0x01;
	  bwin1 = (winflag >> 4) & 0x01;
	  bwinsp = (winflag >> 5) & 0x01;
	  logwin0 = (winflag >> 0) & 0x01;
	  logwin1 = (winflag >> 1) & 0x01;
	  logwinsp = (winflag >> 2) & 0x01;
  }

};


class VdpBack : public VdpPipeline {
public:
  VdpBack(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
  virtual void createColorAttachment(VkPipelineColorBlendAttachmentState & colorAttachment);
};


class VdpLineBase : public VdpPipeline {
public:
  VdpLineBase(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
};

class VdpLineCol : public VdpLineBase {
public:
  VdpLineCol(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
};

class VdpCramLine : public VdpLineBase {
public:
  VdpCramLine(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
};


class VdpPipelineAdd : public VdpPipeline {
public:
  VdpPipelineAdd(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
  void createColorAttachment(VkPipelineColorBlendAttachmentState & color);
};


class VdpPipelineDst : public VdpPipeline {
public:
  VdpPipelineDst(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
  void createColorAttachment(VkPipelineColorBlendAttachmentState & color);
};

class VdpPipelineNoblend : public VdpPipeline {
public:
  VdpPipelineNoblend(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
  void createColorAttachment(VkPipelineColorBlendAttachmentState & color);
};


class VdpPipelineCram : public VdpPipeline {
public:
  VdpPipelineCram(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
};

class VdpPipelinePreLineAlphaCram : public VdpPipeline {
public:
  VdpPipelinePreLineAlphaCram(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
};

class VdpPipelineSpecialPriorityColorOffset : public VdpPipeline {
public:
  VdpPipelineSpecialPriorityColorOffset(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
};


class VdpPipelineCramSpecialPriority : public VdpPipeline {
public:
  VdpPipelineCramSpecialPriority(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
};


class VdpPipelineCramAdd : public VdpPipelineCram {
public:
  VdpPipelineCramAdd(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
  void createColorAttachment(VkPipelineColorBlendAttachmentState & color);
};

class VdpPipelineCramDst : public VdpPipelineCram {
public:
  VdpPipelineCramDst(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
  void createColorAttachment(VkPipelineColorBlendAttachmentState & color);
};

class VdpPipelineCramNoblend : public VdpPipelineCram {
public:
  VdpPipelineCramNoblend(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
  void createColorAttachment(VkPipelineColorBlendAttachmentState & color);
};


class VdpPipelineWindow : public VdpPipeline {
  int id;
public:
  VdpPipelineWindow(int id, VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
  void createColorAttachment(VkPipelineColorBlendAttachmentState & color);
  void createDpethStencil(VkPipelineDepthStencilStateCreateInfo & depthStencil);
  void createInputAssembly(VkPipelineInputAssemblyStateCreateInfo & inputAssembly);
  void createColorBlending(VkPipelineColorBlendStateCreateInfo & colorBlending, VkPipelineColorBlendAttachmentState & colorAttachment);
};


class VdpPipelineBlit : public VdpPipeline {
public:
  VdpPipelineBlit(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
};

class VdpPipelineMosaic : public VdpPipelineBlit {
public:
  VdpPipelineMosaic(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
};

class VdpPipelinePerLine : public VdpPipelineBlit {
public:
  VdpPipelinePerLine(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
};

class VdpPipelinePerLineDst : public VdpPipelinePerLine {
public:
  VdpPipelinePerLineDst(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
  void createColorAttachment(VkPipelineColorBlendAttachmentState & color);
};


//---------------------------------------------------------------------------------------------------
// VDP1 Shaders
class Vdp1GroundShading : public VdpPipeline {
public:
  static int polygonMode;
protected: 
  int clipmode;
public:
  Vdp1GroundShading(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
  virtual void createColorAttachment(VkPipelineColorBlendAttachmentState & color);
  virtual void createDpethStencil(VkPipelineDepthStencilStateCreateInfo & depthStencil);
};

class Vdp1GroundShadingClipInside : public Vdp1GroundShading {
public:
  Vdp1GroundShadingClipInside(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
};

class Vdp1GroundShadingClipOutside : public Vdp1GroundShading {
public:
  Vdp1GroundShadingClipOutside(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
};

class Vdp1GroundShadingTess : public Vdp1GroundShading {
public:
  Vdp1GroundShadingTess(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
};


class Vdp1GroundShadingSpd : public Vdp1GroundShading {
public:
  Vdp1GroundShadingSpd(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
};

class Vdp1GroundShadingSpdClipInside : public Vdp1GroundShadingSpd {
public:
  Vdp1GroundShadingSpdClipInside(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
};

class Vdp1GroundShadingSpdClipOutside : public Vdp1GroundShadingSpd {
public:
  Vdp1GroundShadingSpdClipOutside(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
};


class VDP1GlowShadingAndHalfTransOperation : public Vdp1GroundShading {
public:
  VDP1GlowShadingAndHalfTransOperation(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
};

class VDP1GlowShadingAndHalfTransOperationClipInside : public VDP1GlowShadingAndHalfTransOperation {
public:
  VDP1GlowShadingAndHalfTransOperationClipInside(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
};

class VDP1GlowShadingAndHalfTransOperationClipOutside : public VDP1GlowShadingAndHalfTransOperation {
public:
  VDP1GlowShadingAndHalfTransOperationClipOutside(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
};


class VDP1Mesh : public Vdp1GroundShading {
public:
  VDP1Mesh(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
};

class VDP1MeshClipInside : public VDP1Mesh {
public:
  VDP1MeshClipInside(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
};

class VDP1MeshClipOutside : public VDP1Mesh {
public:
  VDP1MeshClipOutside(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
};


class VDP1HalfLuminance : public Vdp1GroundShading {
public:
  VDP1HalfLuminance(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
};

class VDP1HalfLuminanceClipInside : public VDP1HalfLuminance {
public:
  VDP1HalfLuminanceClipInside(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
};

class VDP1HalfLuminanceClipOutside : public VDP1HalfLuminance {
public:
  VDP1HalfLuminanceClipOutside(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
};


class VDP1Shadow : public VDP1GlowShadingAndHalfTransOperation {
public:
  VDP1Shadow(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
};

class VDP1ShadowClipInside : public VDP1Shadow {
public:
  VDP1ShadowClipInside(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
};

class VDP1ShadowClipOutsize : public VDP1Shadow {
public:
  VDP1ShadowClipOutsize(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
};


class VdpRbgCramLinePipeline : public VdpPipeline {
public:
  VdpRbgCramLinePipeline(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
};


class VDP1SystemClip : public VdpPipeline {
public:
  VDP1SystemClip(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
  virtual void createColorAttachment(VkPipelineColorBlendAttachmentState & color);
  virtual void createDpethStencil(VkPipelineDepthStencilStateCreateInfo & depthStencil);
};


class VDP1UserClip : public VDP1SystemClip {
public:
  VDP1UserClip(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm);
  virtual  void createDpethStencil(VkPipelineDepthStencilStateCreateInfo & depthStencil);
};

