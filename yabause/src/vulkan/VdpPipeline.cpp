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

#include "VdpPipeline.h"
#include "VIDVulkan.h"
#include "TextureManager.h"
#include "VertexManager.h"
#include "VulkanTools.h"

#include "shaderc/shaderc.hpp"
using shaderc::Compiler;
using shaderc::CompileOptions;
using shaderc::SpvCompilationResult;

#include <iostream>
using std::cout;


const int VdpPipeline::bindIdTexture = 1;
const int VdpPipeline::bindIdColorRam = 2;
const int VdpPipeline::bindIdFbo = 2;
const int VdpPipeline::bindIdLine = 3;
const int VdpPipeline::bindIdWindow = 4;



VkShaderModule ShaderManager::getShader(uint32_t id) {
    auto it = shaders.find(id);
    if (it == shaders.end()) {
      return 0; // not found
    }
  return it->second;
}

ShaderManager::~ShaderManager() {
  const VkDevice device = vulkan->getDevice();
  for (int i = 0; i < shaders.size(); i++) {
    vkDestroyShaderModule(device, shaders[i], nullptr);
  }
}



std::string ShaderManager::get_shader_header() {
#if defined(ANDROID)
    return "#version 310 es\n precision highp float; \n precision highp int;\n #extension GL_ANDROID_extension_pack_es31a : enable \n ";
#else
    return "#version 450\n";
#endif
}

VkShaderModule ShaderManager::compileShader(uint32_t id, const string & code, int type) {
    const VkDevice device = vulkan->getDevice();

    LOGI("%s%d", "compiling: ", id);

    string target = get_shader_header() + code;

    Compiler compiler;
    CompileOptions options;
    options.SetOptimizationLevel(shaderc_optimization_level_performance);
    //options.SetOptimizationLevel(shaderc_optimization_level_zero);
    SpvCompilationResult result = compiler.CompileGlslToSpv(
      target,
      (shaderc_shader_kind)type,
      "VdpPipeline",
      options);

    LOGI("%s%d", "erros: ", (int)result.GetNumErrors());
    if (result.GetNumErrors() != 0) {
      LOGI("%s%s", "messages: ", result.GetErrorMessage().c_str());
      cout << target;
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
    shaders[id] = shaderModule;
    return shaderModule;
}

ShaderManager * ShaderManager::instance = nullptr;


VdpPipeline::VdpPipeline(
  VIDVulkan * vulkan,
  TextureManager * tm,
  VertexManager * vm

) {

  bwin0 = 0;
  logwin0 = 0;
  bwin1 = 0;
  logwin1 = 0;
  bwinsp = 0;
  logwinsp = 0;
  winmode = -1;

  vdp2Uniform = R"u(
  layout(binding = 0) uniform UniformBufferObject {
    mat4 mvp;
    vec4 u_color_offset;
    int u_blendmode;
    int offsetx;
    int offsety;
    int windowWidth;
    int windowHeight;
    int winmask;
    int winflag;
    int winmode;
    float u_emu_height;
    float u_vheight;
    float u_viewport_offset;
    float u_tw;
    float u_th;
    int u_mosaic_x;
    int u_mosaic_y;
    int specialPriority;
    int u_specialColorFunc;
  };
  )u";

  vertexShaderName = vdp2Uniform + R"s(
  layout(location = 0) in vec4 a_position;
  layout(location = 1) in vec4 a_color;
  layout(location = 2) in vec4 a_texcoord;

  layout(location = 0) out vec4 fragTexCoord;

  void main() {
    gl_Position = mvp * a_position;
    fragTexCoord = a_texcoord;
  }
  )s";

  fragFuncCheckWindow = R"S(
  void checkWindow() {
    if( winmode != -1 ){
      vec2 winaddr = vec2( (gl_FragCoord.x-float(offsetx)) /float(windowWidth), (gl_FragCoord.y-float(offsety)) /float(windowHeight));
      vec4 wintexture = texture(windowSampler,winaddr);
      int winvalue = int(wintexture.r * 255.0);

        // and
        if( winmode == 0 ){ 
            if( (winvalue & winmask) != winflag ){
                discard;
            }
        // or
        }else{
            if( (winvalue & winmask) == winflag ){
                discard;
            }
        }
    }
  }
  )S";

  //fragShaderName = "./shaders/shader.frag.spv";

  bindid.clear();
  bindid.push_back(bindIdTexture);
  bindid.push_back(bindIdWindow);

  fragShaderName = vdp2Uniform + R"s(
  layout(location = 0) in vec4 fragTexCoord;
  layout(binding = 1) uniform highp sampler2D texSampler;
  layout(location = 0) out vec4 outColor;
  layout(binding = 4) uniform highp sampler2D windowSampler;
  )s" +
    fragFuncCheckWindow
    + R"s(
  void main() {
    checkWindow();
    ivec2 addr;
    addr.x = int(fragTexCoord.x);
    addr.y = int(fragTexCoord.y);
    vec4 txcol = texelFetch(texSampler, addr, 0);
    if (txcol.a > 0.0) {
      outColor = txcol + u_color_offset;
    }
    else {
      discard;
    }
  }
  )s";

  prgid = PG_NORMAL;
  _fragShaderModule = 0;
  _vertShaderModule = 0;

  _descriptorPool = 0;
  _descriptorSetLayout = 0;
  _pipelineLayout = 0;
  _graphicsPipeline = 0;

  this->vulkan = vulkan;
  this->tm = tm;
  this->vm = vm;

}

VdpPipeline::~VdpPipeline() {

  VkDevice device = vulkan->getDevice();

  if (_descriptorPool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(device, _descriptorPool, nullptr);
    _descriptorPool = VK_NULL_HANDLE;
  }

  if (_descriptorSetLayout != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(device, _descriptorSetLayout, nullptr);
    _descriptorSetLayout = VK_NULL_HANDLE;
  }

  if (_pipelineLayout != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(device, _pipelineLayout, nullptr);
    _pipelineLayout = VK_NULL_HANDLE;
  }

#if 0
  if (_fragShaderModule != VK_NULL_HANDLE) {
    vkDestroyShaderModule(device, _fragShaderModule, nullptr);
    _fragShaderModule = VK_NULL_HANDLE;
  }

  if (_vertShaderModule != VK_NULL_HANDLE) {
    vkDestroyShaderModule(device, _vertShaderModule, nullptr);
    _vertShaderModule = VK_NULL_HANDLE;
  }
#endif

  if (_graphicsPipeline != 0) {
    vkDestroyPipeline(device, _graphicsPipeline, nullptr);
    _graphicsPipeline = VK_NULL_HANDLE;
  }
}

std::string VdpPipeline::get_shader_header() {
  return "#version 450\n";
}

VkShaderModule VdpPipeline::compileShader(const string & code, int type) {
  ShaderManager * sm = ShaderManager::getInstance();
  VkShaderModule shaderModule = sm->getShader(prgid | (type << 16));
  if (shaderModule == 0) {
    sm->setVulkan(vulkan);
    shaderModule = sm->compileShader(prgid | (type << 16), code, type);
  }
  return shaderModule;
}

void VdpPipeline::moveToVertexBuffer(const vector<Vertex> & vertices, const vector<uint16_t> & indices) {
  if (vertices.size() > 0) {
    vm->add(vertices, indices, vectexBlock, vertexOffset, vertexSize, indexOffset, indexSize);
  }
}

void VdpPipeline::setUBO(const void * ubo, int size) {
  const VkDevice device = vulkan->getDevice();
  void* data;
  vkMapMemory(device, _uniformBufferMemory, 0, size, 0, &data);
  memcpy(data, ubo, size);
  vkUnmapMemory(device, _uniformBufferMemory);
  uboSize = size;
  if (MAX_UBO_SIZE < uboSize) {
    throw std::runtime_error("MAX_UBO_SIZE over!!");
  }
}


#include <array>
#include <chrono>
#include <iostream>
#include <fstream>

std::vector<char> VdpPipeline::readFile(const std::string& filename) {
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

void VdpPipeline::createGraphicsPipeline() {

  VkDevice device = vulkan->getDevice();

  vulkan->createBuffer(MAX_UBO_SIZE,
    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    _uniformBuffer, _uniformBufferMemory);


  initDescriptorSets(bindid);

  _vertShaderModule = compileShader(vertexShaderName, shaderc_vertex_shader);
  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
  vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = _vertShaderModule;
  vertShaderStageInfo.pName = "main";

  _fragShaderModule = compileShader(fragShaderName, shaderc_fragment_shader);
  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
  fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = _fragShaderModule;
  fragShaderStageInfo.pName = "main";

  vector<VkPipelineShaderStageCreateInfo> shaderStages;
  shaderStages.push_back(vertShaderStageInfo);
  shaderStages.push_back(fragShaderStageInfo);

  VkPipelineShaderStageCreateInfo tcShaderStageInfo = {};
  if (tessControll != "") {
    tessControllModule = compileShader(tessControll, shaderc_tess_control_shader);
    tcShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    tcShaderStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    tcShaderStageInfo.module = tessControllModule;
    tcShaderStageInfo.pName = "main";
    shaderStages.push_back(tcShaderStageInfo);
  }

  VkPipelineShaderStageCreateInfo teShaderStageInfo = {};
  if (tessEvaluation != "") {
    tessEvaluationModule = compileShader(tessEvaluation, shaderc_tess_evaluation_shader);
    teShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    teShaderStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    teShaderStageInfo.module = tessEvaluationModule;
    teShaderStageInfo.pName = "main";
    shaderStages.push_back(teShaderStageInfo);
  }

  VkPipelineShaderStageCreateInfo gShaderStageInfo = {};
  if (geometry != "") {
    geometryModule = compileShader(geometry, shaderc_geometry_shader);
    gShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    gShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
    gShaderStageInfo.module = geometryModule;
    gShaderStageInfo.pName = "main";
    shaderStages.push_back(gShaderStageInfo);
  }


  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 0;
  vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
  vertexInputInfo.vertexAttributeDescriptionCount = 0;
  vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
  createInputAssembly(inputAssembly);

  VkRect2D render_area{};
  render_area.offset.x = 0;
  render_area.offset.y = 0;
  render_area.extent = vulkan->getSurfaceSize();

  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)render_area.extent.width;
  viewport.height = (float)render_area.extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {};
  scissor.offset = { 0, 0 };
  scissor.extent = render_area.extent;

  VkPipelineViewportStateCreateInfo viewportState = {};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer = {};
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

  VkPipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading = 1.0f; // Optional
  multisampling.pSampleMask = nullptr; // Optional
  multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
  multisampling.alphaToOneEnable = VK_FALSE; // Optional
                                             // ToDo

  VkPipelineDepthStencilStateCreateInfo depthStencil = {};
  createDpethStencil(depthStencil);

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
  createColorAttachment(colorBlendAttachment);


  VkPipelineColorBlendStateCreateInfo colorBlending = {};
  createColorBlending(colorBlending, colorBlendAttachment);

  /*
  VkDynamicState dynamicStates[] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR,
  };
  */

  dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
  dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);

  VkPipelineDynamicStateCreateInfo dynamicState = {};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = dynamicStates.size();
  dynamicState.pDynamicStates = dynamicStates.data();

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

  VkPipelineTessellationStateCreateInfo tessellationState = vks::initializers::pipelineTessellationStateCreateInfo(4);

  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = shaderStages.size();
  pipelineInfo.pStages = shaderStages.data();
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.pTessellationState = &tessellationState;
  pipelineInfo.layout = _pipelineLayout;
  pipelineInfo.renderPass = this->renderPass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
  pipelineInfo.basePipelineIndex = -1; // Optional

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_graphicsPipeline) != VK_SUCCESS) {
    throw std::runtime_error("failed to create graphics pipeline!");
  }

}

void VdpPipeline::createInputAssembly(VkPipelineInputAssemblyStateCreateInfo & inputAssembly) {
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  if (tessControll != "") {
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
  }else{
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  }
  inputAssembly.primitiveRestartEnable = VK_FALSE;
}

void VdpPipeline::createColorAttachment(VkPipelineColorBlendAttachmentState & color) {

  // Defalut Blend function
  color.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color.blendEnable = VK_TRUE;
  color.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  color.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  color.colorBlendOp = VK_BLEND_OP_ADD;
  color.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  color.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  color.alphaBlendOp = VK_BLEND_OP_ADD;

}


// Clear regison Stencil 0
VDP1SystemClip::VDP1SystemClip(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm) : VdpPipeline(vulkan, tm, vm) {

  bindid.clear();

  vertexShaderName = R"s(
    layout(binding = 0) uniform UniformBufferObject {
       mat4 u_mvpMatrix;
       vec2 u_texsize;
       int u_fbowidth;
       int u_fbohegiht;
    } ubo;

    layout (location = 0) in vec4 a_position;
    layout (location = 1) in vec4 a_grcolor;
    layout (location = 2) in vec4 a_texcoord;

    void main() {
      vec4 pos = vec4( a_position.x, a_position.y, 0.0, 1.0);
      gl_Position = ubo.u_mvpMatrix * a_position;
    }
  )s";

  fragShaderName = R"s(
  layout(location = 0) out vec4 outColor;

  void main() {
    outColor = vec4(1.0);
  }
  )s";

  this->prgid = PG_VDP1_SYSTEM_CLIP;


}

void VDP1SystemClip::createColorAttachment(VkPipelineColorBlendAttachmentState & color) {
  // Defalut Blend function
  color.colorWriteMask = 0;
  color.blendEnable = VK_FALSE;
}


void VDP1SystemClip::createDpethStencil(VkPipelineDepthStencilStateCreateInfo & depthStencil) {
  depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable = VK_TRUE;
  depthStencil.depthWriteEnable = VK_TRUE;
  depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL; // VK_COMPARE_OP_LESS;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.minDepthBounds = 0.0f; // Optional
  depthStencil.maxDepthBounds = 1.0f; // Optional
  depthStencil.stencilTestEnable = VK_FALSE;
  depthStencil.front = {}; // Optional
  depthStencil.stencilTestEnable = VK_TRUE;
  depthStencil.back.compareOp = VK_COMPARE_OP_ALWAYS;
  depthStencil.back.failOp = VK_STENCIL_OP_REPLACE;
  depthStencil.back.depthFailOp = VK_STENCIL_OP_REPLACE;
  depthStencil.back.passOp = VK_STENCIL_OP_REPLACE;
  depthStencil.back.compareMask = 0xff;
  depthStencil.back.writeMask = 0xff;
  depthStencil.back.reference = 1;
}


// Clear regison Stencil 1
VDP1UserClip::VDP1UserClip(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm) : VDP1SystemClip(vulkan, tm, vm) {

  this->prgid = PG_VDP1_USER_CLIP;
}

void VDP1UserClip::createDpethStencil(VkPipelineDepthStencilStateCreateInfo & depthStencil) {
  depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable = VK_TRUE;
  depthStencil.depthWriteEnable = VK_TRUE;
  depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL; // VK_COMPARE_OP_LESS;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.minDepthBounds = 0.0f; // Optional
  depthStencil.maxDepthBounds = 1.0f; // Optional
  depthStencil.stencilTestEnable = VK_FALSE;
  depthStencil.front = {}; // Optional
  depthStencil.stencilTestEnable = VK_TRUE;
  depthStencil.back.compareOp = VK_COMPARE_OP_ALWAYS;
  depthStencil.back.failOp = VK_STENCIL_OP_REPLACE;
  depthStencil.back.depthFailOp = VK_STENCIL_OP_REPLACE;
  depthStencil.back.passOp = VK_STENCIL_OP_REPLACE;
  depthStencil.back.compareMask = 0xff;
  depthStencil.back.writeMask = 0xff;
  depthStencil.back.reference = 2;
}



void VdpPipeline::createDpethStencil(VkPipelineDepthStencilStateCreateInfo & depthStencil) {
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
}

void VdpPipeline::createColorBlending(VkPipelineColorBlendStateCreateInfo & colorBlending, VkPipelineColorBlendAttachmentState & colorAttachment) {
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorAttachment;
  colorBlending.blendConstants[0] = 0.0f; // Optional
  colorBlending.blendConstants[1] = 0.0f; // Optional
  colorBlending.blendConstants[2] = 0.0f; // Optional
  colorBlending.blendConstants[3] = 0.0f; // Optional
}

VdpPipelineDst::VdpPipelineDst(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm) : VdpPipeline(vulkan, tm, vm) {
  prgid = PG_NORMAL_DSTALPHA;
}

void VdpPipelineDst::createColorAttachment(VkPipelineColorBlendAttachmentState & color) {
  // Defalut Blend function
  color.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color.blendEnable = VK_TRUE;
  color.srcColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
  color.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
  color.colorBlendOp = VK_BLEND_OP_ADD;
  color.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  color.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  color.alphaBlendOp = VK_BLEND_OP_ADD;
}



VdpPipelineAdd::VdpPipelineAdd(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm) : VdpPipeline(vulkan, tm, vm) {
  prgid = PG_VDP2_ADDBLEND;
}

void VdpPipelineAdd::createColorAttachment(VkPipelineColorBlendAttachmentState & color) {
  // Defalut Blend function
  color.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color.blendEnable = VK_TRUE;
  color.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  color.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
  color.colorBlendOp = VK_BLEND_OP_ADD;
  color.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  color.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  color.alphaBlendOp = VK_BLEND_OP_ADD;
}

VdpPipelineNoblend::VdpPipelineNoblend(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm) : VdpPipeline(vulkan, tm, vm) {
  prgid = PG_VDP2_NOBLEND;
}

void VdpPipelineNoblend::createColorAttachment(VkPipelineColorBlendAttachmentState & color) {
  // Defalut Blend function
  color.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color.blendEnable = VK_FALSE;
  color.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  color.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  color.colorBlendOp = VK_BLEND_OP_ADD;
  color.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  color.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  color.alphaBlendOp = VK_BLEND_OP_ADD;
}


VdpLineBase::VdpLineBase(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm) : VdpPipeline(vulkan, tm, vm) {

  bindid.clear();
  bindid.push_back(bindIdTexture);
  bindid.push_back(bindIdColorRam);
  bindid.push_back(bindIdLine);
  bindid.push_back(bindIdWindow);

  fragShaderName = vdp2Uniform + R"S(
    layout(location = 0) in highp vec4 v_texcoord;
    layout(binding = 1) uniform highp sampler2D s_texture;
    layout(binding = 2) uniform highp sampler2D s_color;
    layout(binding = 3) uniform highp sampler2D s_line;
    layout(binding = 4) uniform highp sampler2D windowSampler;
    layout(location = 0) out vec4 fragColor;

    )S" + fragFuncCheckWindow

    + R"s(  void main() {
      checkWindow();
      ivec2 addr;
      addr.x = int(v_texcoord.x);
      addr.y = int(v_texcoord.y);
      ivec2 linepos;
      linepos.y = 0;
      linepos.x = int( (gl_FragCoord.y-u_viewport_offset) * u_emu_height);
      vec4 txcol = texelFetch( s_texture, addr,0 );
      vec4 lncol = texelFetch( s_line, linepos,0 );
      if(txcol.a > 0.0){
  )s";

}

VdpLineCol::VdpLineCol(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm) : VdpLineBase(vulkan, tm, vm) {
  fragShaderName += R"s(
        fragColor = txcol+u_color_offset+lncol;
        fragColor.a = 1.0;
        //if( specialPriority == 2 ){ gl_FragDepth = txcol.b*255.0/10.0; }else{ gl_FragDepth = gl_FragCoord.z; }
      }else{
        discard;
      }
    }
  )s";
  prgid = PG_LINECOLOR_INSERT;
}

VdpCramLine::VdpCramLine(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm) : VdpLineBase(vulkan, tm, vm) {
  fragShaderName += R"s(
        vec4 txcolc = texelFetch( s_color,  ivec2( ( int(txcol.g*65280.0) | int(txcol.r*255.0)) ,0 )  , 0 );
        fragColor = txcolc+u_color_offset+lncol;
        fragColor.a = 1.0;
        //if( specialPriority == 2 ){ gl_FragDepth = txcol.b*255.0/10.0; }else{ gl_FragDepth = gl_FragCoord.z; }
      }else{
        discard;
      }
    }
  )s";
  prgid = PG_LINECOLOR_INSERT_CRAM;
}


void VdpPipeline::initDescriptorSets(const vector<int> & bindid) {

  VkDevice device = vulkan->getDevice();

  vector<VkDescriptorSetLayoutBinding> layoutBindings;

  // _descriptorSetLayout 
  VkDescriptorSetLayoutBinding uboLayoutBinding = {};
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  if (tessControll != "") uboLayoutBinding.stageFlags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
  if (tessEvaluation != "") uboLayoutBinding.stageFlags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
  if (geometry != "") uboLayoutBinding.stageFlags |= VK_SHADER_STAGE_GEOMETRY_BIT;
  uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

  layoutBindings.push_back(uboLayoutBinding);

  for (int i = 0; i < bindid.size(); i++) {
    VkDescriptorSetLayoutBinding sampler = {};
    sampler.binding = bindid[i];
    sampler.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler.descriptorCount = 1;
    sampler.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    sampler.pImmutableSamplers = nullptr;
    layoutBindings.push_back(sampler);
  }

  VkDescriptorSetLayoutCreateInfo layoutInfo = {};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
  layoutInfo.pBindings = layoutBindings.data();


  if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &_descriptorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
  }

  vector<VkDescriptorPoolSize> poolSizes;

  VkDescriptorPoolSize uni;
  uni.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uni.descriptorCount = 1;
  poolSizes.push_back(uni);

  for (int i = 0; i < bindid.size(); i++) {
    VkDescriptorPoolSize pool = {};
    pool.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool.descriptorCount = 1;
    poolSizes.push_back(pool);
  }

  VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = 1;

  if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &_descriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool!");
  }

  VkDescriptorSetLayout layouts[] = { _descriptorSetLayout };
  VkDescriptorSetAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = _descriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = layouts;
  if (vkAllocateDescriptorSets(device, &allocInfo, &_descriptorSet) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor set!");
  }

}

void VdpPipeline::updateDescriptorSets()
{

  VkDevice device = vulkan->getDevice();

  std::vector<VkWriteDescriptorSet> descriptorWrites;


  VkDescriptorBufferInfo bufferInfo = {};
  bufferInfo.buffer = _uniformBuffer;
  bufferInfo.offset = 0;
  bufferInfo.range = uboSize;

  VkWriteDescriptorSet descriptorWrite = {};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = _descriptorSet;
  descriptorWrite.dstBinding = 0;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pBufferInfo = &bufferInfo;
  descriptorWrites.push_back(descriptorWrite);

  VkDescriptorImageInfo imageInfos[32];
  for (int i = 0; i < bindid.size(); i++) {

    imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfos[i].imageView = samplers[bindid[i]].img;
    imageInfos[i].sampler = samplers[bindid[i]].smp;

    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = _descriptorSet;
    descriptorWrite.dstBinding = bindid[i];
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfos[i];
    descriptorWrites.push_back(descriptorWrite);
  }

  vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

}

VdpPipelineMosaic::VdpPipelineMosaic(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm) : VdpPipelineBlit(vulkan, tm, vm) {
  bindid.clear();
  bindid.push_back(bindIdTexture);
  bindid.push_back(bindIdWindow);

  fragShaderName = vdp2Uniform + R"S(
    layout(location = 0) in vec4 v_texcoord;
    layout(binding = 1) uniform highp sampler2D s_texture;
    layout(binding = 4) uniform highp sampler2D windowSampler;
    layout(location = 0) out vec4 fragColor;
  )S" +
    fragFuncCheckWindow
    + R"s(
  void main() {
    //checkWindow();
    ivec2 addr;
    addr.x = int(u_tw * v_texcoord.x);
    addr.y = int(u_th * v_texcoord.y);
    addr.x = addr.x / u_mosaic_x * u_mosaic_x;
    addr.y = addr.y / u_mosaic_y * u_mosaic_y;
    vec4 txcol = texelFetch( s_texture, addr,0 ) ;
    if(txcol.a > 0.0)
      fragColor = txcol;
    else
      discard;
    }
  )s";

  prgid = PG_VDP2_MOSAIC;

}

VdpPipelinePerLine::VdpPipelinePerLine(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm) : VdpPipelineBlit(vulkan, tm, vm) {
  bindid.clear();
  bindid.push_back(bindIdTexture);
  bindid.push_back(bindIdLine);
  bindid.push_back(bindIdWindow);

  fragShaderName = vdp2Uniform + R"S(
    layout(location = 0) in vec4 v_texcoord;
    layout(binding = 1) uniform highp sampler2D s_texture;
    layout(binding = 3) uniform highp sampler2D s_line;
    layout(binding = 4) uniform highp sampler2D windowSampler;
    layout(location = 0) out vec4 fragColor;
  )S" +
    fragFuncCheckWindow
    + R"s(
  void main() {
    checkWindow();
    ivec2 addr;
    addr.x = int(u_tw * v_texcoord.x);
    addr.y = int(u_th * v_texcoord.y);
    vec4 txcol = texelFetch( s_texture, addr,0 ) ;
    if(txcol.a > 0.0){
      addr.x = int(u_th * v_texcoord.y);
      addr.y = 0;
      if(u_specialColorFunc == 0 ) {
          txcol.a = texelFetch( s_line, addr,0 ).a;
      }else{
         if( txcol.a != 1.0 ) txcol.a = texelFetch( s_line, addr,0 ).a;
      }
      txcol.r += (texelFetch( s_line, addr,0 ).r-0.5)*2.0;
      txcol.g += (texelFetch( s_line, addr,0 ).g-0.5)*2.0;
      txcol.b += (texelFetch( s_line, addr,0 ).b-0.5)*2.0;
      if( txcol.a > 0.0 )
        fragColor = txcol;
      else
         discard;
    }else{
      discard;
    }
    //if( specialPriority == 2 ){ gl_FragDepth = txindex.b*255.0/10.0; }else{ gl_FragDepth = gl_FragCoord.z; }
   }
  )s";

  prgid = PG_VDP2_PER_LINE_ALPHA;

}

VdpPipelineSpecialPriorityColorOffset::VdpPipelineSpecialPriorityColorOffset(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm) : VdpPipeline(vulkan, tm, vm) {
  bindid.clear();
  bindid.push_back(bindIdTexture);
  bindid.push_back(bindIdLine);
  bindid.push_back(bindIdColorRam);
  bindid.push_back(bindIdWindow);

  fragShaderName = vdp2Uniform + R"S(
    layout(location = 0) in vec4 v_texcoord;
    layout(binding = 1) uniform highp sampler2D s_texture;
    layout(binding = 2) uniform highp sampler2D s_color;
    layout(binding = 3) uniform highp sampler2D s_linetexture;
    layout(binding = 4) uniform highp sampler2D windowSampler;
    layout(location = 0) out vec4 fragColor;
    layout(location = 1) out float fargDepth;
  )S" +
    fragFuncCheckWindow
    + R"s(
  void main() {
    checkWindow();
        vec4 txindex = texelFetch( s_texture, ivec2(int(v_texcoord.x),int(v_texcoord.y)) ,0 );
        if(txindex.a == 0.0) { discard; }
        vec4 txcol = texelFetch( s_color,  ivec2( ( int(txindex.g*65280.0) | int(txindex.r*255.0)) ,0 )  , 0 );
        vec4 color_offset = texelFetch( s_linetexture, ivec2( int(v_texcoord.q), 0), 0 );
        fragColor.r = txcol.r + (color_offset.r-0.5)*2.0;
        fragColor.g = txcol.g + (color_offset.g-0.5)*2.0;
        fragColor.b = txcol.b + (color_offset.b-0.5)*2.0;
        fragColor.a = txindex.a;
        gl_FragDepth = (txindex.b*255.0/10.0);
    }
  )s";

  prgid = PG_VDP2_NORMAL_CRAM_SPECIAL_PRIORITY_COLOROFFSET;
}

VdpPipelinePerLineDst::VdpPipelinePerLineDst(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm) : VdpPipelinePerLine(vulkan, tm, vm) {
  prgid = PG_VDP2_PER_LINE_ALPHA_DST;
}

void VdpPipelinePerLineDst::createColorAttachment(VkPipelineColorBlendAttachmentState & color) {
  color.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color.blendEnable = VK_TRUE;
  color.srcColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
  color.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
  color.colorBlendOp = VK_BLEND_OP_ADD;
  color.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  color.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  color.alphaBlendOp = VK_BLEND_OP_ADD;
}

VdpBack::VdpBack(
  VIDVulkan * vulkan,
  TextureManager * tm,
  VertexManager * vm
) : VdpPipeline(vulkan, tm, vm) {

  bindid.clear();
  bindid.push_back(bindIdTexture);

  fragShaderName = vdp2Uniform + R"S(
    layout(location = 0) in vec4 v_texcoord;
    layout(binding = 1) uniform highp sampler2D s_texture;
    layout(location = 0) out vec4 fragColor;
  void main() {
    if( u_blendmode == 0 ){
      fragColor = texelFetch( s_texture, ivec2(0,0) ,0 );
      return;
    }else{
      ivec2 linepos;
      linepos.y = 0; 
      linepos.x = int( (gl_FragCoord.y-u_viewport_offset) * u_emu_height);
      fragColor = texelFetch( s_texture, linepos,0 );
      return;
    }
    return;
  }
  )S";

  this->prgid = PG_VDP2_BACK;
}

void VdpBack::createColorAttachment(VkPipelineColorBlendAttachmentState & color) {
  color.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color.blendEnable = VK_FALSE;
  color.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  color.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  color.colorBlendOp = VK_BLEND_OP_ADD;
  color.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  color.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  color.alphaBlendOp = VK_BLEND_OP_ADD;
}


VdpPipelineCram::VdpPipelineCram(
  VIDVulkan * vulkan,
  TextureManager * tm,
  VertexManager * vm
) : VdpPipeline(vulkan, tm, vm) {

  bindid.clear();
  bindid.push_back(bindIdTexture);
  bindid.push_back(bindIdLine);
  bindid.push_back(bindIdColorRam);
  bindid.push_back(bindIdWindow);

  fragShaderName = vdp2Uniform + R"S(
    layout(location = 0) in vec4 v_texcoord;
    layout(binding = 1) uniform highp sampler2D s_texture;
    layout(binding = 2) uniform highp sampler2D s_color;
    layout(binding = 4) uniform highp sampler2D windowSampler;
    layout(location = 0) out vec4 fragColor;
    layout(location = 1) out float fargDepth;
  )S" +
    fragFuncCheckWindow
    + R"s(
  void main() {
    checkWindow();
        vec4 txindex = texelFetch( s_texture, ivec2(int(v_texcoord.x),int(v_texcoord.y)) ,0 );
        if(txindex.a == 0.0) { discard; }
        vec4 txcol = texelFetch( s_color,  ivec2( ( int(txindex.g*65280.0) | int(txindex.r*255.0)) ,0 )  , 0 );
        fragColor = txcol + u_color_offset;
        fragColor.a = txindex.a;
        if( specialPriority == 2 ){ gl_FragDepth = txindex.b*255.0/10.0; }else{ gl_FragDepth = gl_FragCoord.z; }
    }
  )s";

  prgid = PG_VDP2_NORMAL_CRAM;
}


VdpPipelinePreLineAlphaCram::VdpPipelinePreLineAlphaCram(
  VIDVulkan * vulkan,
  TextureManager * tm,
  VertexManager * vm
) : VdpPipeline(vulkan, tm, vm) {

  bindid.clear();
  bindid.push_back(bindIdTexture);
  bindid.push_back(bindIdColorRam);
  bindid.push_back(bindIdLine);
  bindid.push_back(bindIdWindow);

  fragShaderName = vdp2Uniform + R"S(
    layout(location = 0) in vec4 v_texcoord;
    layout(binding = 1) uniform highp sampler2D s_texture;
    layout(binding = 2) uniform highp sampler2D s_color;
    layout(binding = 3) uniform highp sampler2D s_line;
    layout(binding = 4) uniform highp sampler2D windowSampler;
    layout(location = 0) out vec4 fragColor;
    layout(location = 1) out float fargDepth;
  )S" +
    fragFuncCheckWindow
    + R"s(
  void main() {
    checkWindow();
        vec4 txindex = texelFetch( s_texture, ivec2(int(v_texcoord.x),int(v_texcoord.y)) ,0 );
        if(txindex.a == 0.0) { discard; }
        vec4 txcol = texelFetch( s_color,  ivec2( ( int(txindex.g*65280.0) | int(txindex.r*255.0)) ,0 )  , 0 );
        ivec2 addr;
        addr.x = int(u_th * v_texcoord.y);
        addr.y = 0;
        txcol.r += (texelFetch( s_line, addr,0 ).r-0.5)*2.0;
        txcol.g += (texelFetch( s_line, addr,0 ).g-0.5)*2.0;
        txcol.b += (texelFetch( s_line, addr,0 ).b-0.5)*2.0;
        fragColor = txcol;
        fragColor.a = txindex.a;
        if( specialPriority == 2 ){ gl_FragDepth = txindex.b*255.0/10.0; }else{ gl_FragDepth = gl_FragCoord.z; }
    }
  )s";

  prgid = PG_VDP2_PER_LINE_ALPHA_CRAM;
}


VdpPipelineCramNoblend::VdpPipelineCramNoblend(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm) : VdpPipelineCram(vulkan, tm, vm) {
  prgid = PG_VDP2_NOBLEND_CRAM;
}

void VdpPipelineCramNoblend::createColorAttachment(VkPipelineColorBlendAttachmentState & color) {
  // Defalut Blend function
  color.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color.blendEnable = VK_FALSE;
  color.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  color.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  color.colorBlendOp = VK_BLEND_OP_ADD;
  color.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  color.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  color.alphaBlendOp = VK_BLEND_OP_ADD;
}

VdpPipelineCramSpecialPriority::VdpPipelineCramSpecialPriority(
  VIDVulkan * vulkan,
  TextureManager * tm,
  VertexManager * vm
) : VdpPipeline(vulkan, tm, vm) {
  bindid.clear();
  bindid.push_back(bindIdTexture);
  bindid.push_back(bindIdColorRam);
  bindid.push_back(bindIdWindow);

  fragShaderName = vdp2Uniform + R"S(
    layout(location = 0) in vec4 v_texcoord;
    layout(binding = 1) uniform highp sampler2D s_texture;
    layout(binding = 2) uniform highp sampler2D s_color;
    layout(binding = 4) uniform highp sampler2D windowSampler;
    layout(location = 0) out vec4 fragColor;
  )S" +
    fragFuncCheckWindow
    + R"S(
  void main() {
    checkWindow();
    vec4 txindex = texelFetch( s_texture, ivec2(int(v_texcoord.x),int(v_texcoord.y)) ,0 );
    if(txindex.a == 0.0) { discard; }
    vec4 txcol = texelFetch( s_color,  ivec2( ( int(txindex.g*65280.0) | int(txindex.r*255.0)) ,0 )  , 0 );
    fragColor = clamp(txcol+u_color_offset,vec4(0.0),vec4(1.0));
    fragColor.a = txindex.a;
    gl_FragDepth = (txindex.b*255.0/10.0) ;
    }
  )S";
  prgid = PG_VDP2_CRAM_SPECIAL_PRIORITY;
}


VdpPipelineCramAdd::VdpPipelineCramAdd(
  VIDVulkan * vulkan,
  TextureManager * tm,
  VertexManager * vm
) : VdpPipelineCram(vulkan, tm, vm) {

  bindid.clear();
  bindid.push_back(bindIdTexture);
  bindid.push_back(bindIdColorRam);
  bindid.push_back(bindIdWindow);

  fragShaderName = vdp2Uniform + R"S(
    layout(location = 0) in vec4 v_texcoord;
    layout(binding = 1) uniform highp sampler2D s_texture;
    layout(binding = 2) uniform highp sampler2D s_color;
    layout(binding = 4) uniform highp sampler2D windowSampler;
    layout(location = 0) out vec4 fragColor;
  )S" +
    fragFuncCheckWindow
    + R"S(
  void main() {
    checkWindow();
        vec4 txindex = texelFetch( s_texture, ivec2(int(v_texcoord.x),int(v_texcoord.y)) ,0 );
        if(txindex.a == 0.0) { discard; }
        vec4 txcol = texelFetch( s_color,  ivec2( ( int(txindex.g*65280.0) | int(txindex.r*255.0)) ,0 )  , 0 );
        fragColor = txcol + u_color_offset;
        if( txindex.a > 0.5) { fragColor.a = 1.0;} else {fragColor.a = 0.0;};
    }
  )S";

  prgid = PG_VDP2_ADDCOLOR_CRAM;
}

void VdpPipelineCramAdd::createColorAttachment(VkPipelineColorBlendAttachmentState & color) {
  // Defalut Blend function
  color.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color.blendEnable = VK_TRUE;
  color.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  color.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
  color.colorBlendOp = VK_BLEND_OP_ADD;
  color.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  color.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  color.alphaBlendOp = VK_BLEND_OP_ADD;
}


VdpPipelineCramDst::VdpPipelineCramDst(
  VIDVulkan * vulkan,
  TextureManager * tm,
  VertexManager * vm
) : VdpPipelineCram(vulkan, tm, vm) {
  prgid = PG_VDP2_NORMAL_CRAM_DSTALPHA;
}

void VdpPipelineCramDst::createColorAttachment(VkPipelineColorBlendAttachmentState & color) {
  // Defalut Blend function
  color.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color.blendEnable = VK_TRUE;
  color.srcColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
  color.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
  color.colorBlendOp = VK_BLEND_OP_ADD;
  color.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  color.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  color.alphaBlendOp = VK_BLEND_OP_ADD;
}


VdpPipelineBlit::VdpPipelineBlit(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm) : VdpPipeline(vulkan, tm, vm) {
  bindid.clear();
  bindid.push_back(bindIdTexture);

  vertexShaderName = vdp2Uniform + R"s(
    layout(location = 0) in vec4 a_position;
    layout(location = 1) in vec4 inColor;
    layout(location = 2) in vec4 a_texcoord;
    layout (location = 0) out vec4 v_texcoord;
    void main() {
        v_texcoord  = a_texcoord;
        gl_Position = a_position;
        gl_Position.z = u_vheight;
    }

  )s";


  fragShaderName = R"s(
  layout(binding = 1) uniform sampler2D s_texture;
  layout(location = 0) in vec2 v_texcoord;
  layout(location = 0) out vec4 outColor;
  void main() {
    outColor = texture( s_texture, v_texcoord ) ;
  }

  )s";

  prgid = PG_VULKAN_BLIT;

}


VdpPipelineWindow::VdpPipelineWindow(int id, VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm) : VdpPipeline(vulkan, tm, vm) {

  this->id = id;

  bindid.clear();

  vertexShaderName = R"s(
  layout(binding = 0) uniform UniformBufferObject {
    mat4 mvp;
    int windowBit;
  };

  layout(location = 0) in vec4 a_position;
  layout(location = 1) in vec4 inColor;
  layout(location = 2) in vec4 a_texcoord;

  void main() {
    vec4 pos = vec4( a_position.x, a_position.y, 0.0, 1.0);
    gl_Position = mvp * pos;
  }
  )s";


  fragShaderName = R"s(
  layout(binding = 0) uniform UniformBufferObject {
    mat4 mvp;
    int windowBit;
  };

  layout(location = 0) out vec4 outColor;

  void main() {
    outColor = vec4(float(windowBit) / 255.0 ,0.0,0.0,0.0);
  }

  )s";

  prgid = PG_VULKAN_WINDOW;
}


void VdpPipelineWindow::createColorBlending(VkPipelineColorBlendStateCreateInfo & colorBlending, VkPipelineColorBlendAttachmentState & colorAttachment) {
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  //colorBlending.logicOpEnable = VK_TRUE;
  //colorBlending.logicOp = VK_LOGIC_OP_OR;

  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional

  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorAttachment;
  colorBlending.blendConstants[0] = 0.0f;
  colorBlending.blendConstants[1] = 0.0f;
  colorBlending.blendConstants[2] = 0.0f;
  colorBlending.blendConstants[3] = 0.0f;
}


void VdpPipelineWindow::createInputAssembly(VkPipelineInputAssemblyStateCreateInfo & inputAssembly) {
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
  inputAssembly.primitiveRestartEnable = VK_FALSE;
}


void VdpPipelineWindow::createColorAttachment(VkPipelineColorBlendAttachmentState & color) {
  color.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color.blendEnable = VK_TRUE;
  color.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  color.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
  color.colorBlendOp = VK_BLEND_OP_ADD;
  color.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  color.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  color.alphaBlendOp = VK_BLEND_OP_ADD;

}

void VdpPipelineWindow::createDpethStencil(VkPipelineDepthStencilStateCreateInfo & depthStencil) {
  depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable = VK_FALSE;
  depthStencil.depthWriteEnable = VK_TRUE;
  depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL; // VK_COMPARE_OP_LESS;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.minDepthBounds = 0.0f; // Optional
  depthStencil.maxDepthBounds = 1.0f; // Optional
  depthStencil.stencilTestEnable = VK_FALSE;
  depthStencil.front = {}; // Optional
  depthStencil.back = {}; // Optional
}


int Vdp1GroundShading::polygonMode = PERSPECTIVE_CORRECTION;

void Vdp1GroundShading::createColorAttachment(VkPipelineColorBlendAttachmentState & color) {
  color.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color.blendEnable = VK_FALSE;
}

void Vdp1GroundShading::createDpethStencil(VkPipelineDepthStencilStateCreateInfo & depthStencil) {

  depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable = VK_FALSE;
  depthStencil.depthWriteEnable = VK_FALSE;
  depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL; // VK_COMPARE_OP_LESS;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.minDepthBounds = 0.0f; // Optional
  depthStencil.maxDepthBounds = 1.0f; // Optional
  depthStencil.stencilTestEnable = VK_TRUE;

  switch (clipmode) {
  case 0: // nodrmal
    depthStencil.back.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencil.back.failOp = VK_STENCIL_OP_KEEP;
    depthStencil.back.depthFailOp = VK_STENCIL_OP_KEEP;
    depthStencil.back.passOp = VK_STENCIL_OP_KEEP;
    depthStencil.back.compareMask = 0xff;
    depthStencil.back.writeMask = 0xff;
    depthStencil.back.reference = 1;
    depthStencil.front.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencil.front.failOp = VK_STENCIL_OP_KEEP;
    depthStencil.front.depthFailOp = VK_STENCIL_OP_KEEP;
    depthStencil.front.passOp = VK_STENCIL_OP_KEEP;
    depthStencil.front.compareMask = 0xff;
    depthStencil.front.writeMask = 0xff;
    depthStencil.front.reference = 1;
    break;
  case 1: // inside
    depthStencil.back.compareOp = VK_COMPARE_OP_EQUAL;
    depthStencil.back.failOp = VK_STENCIL_OP_KEEP;
    depthStencil.back.depthFailOp = VK_STENCIL_OP_KEEP;
    depthStencil.back.passOp = VK_STENCIL_OP_KEEP;
    depthStencil.back.compareMask = 0xff;
    depthStencil.back.writeMask = 0xff;
    depthStencil.back.reference = 2;
    depthStencil.front.compareOp = VK_COMPARE_OP_EQUAL;
    depthStencil.front.failOp = VK_STENCIL_OP_KEEP;
    depthStencil.front.depthFailOp = VK_STENCIL_OP_KEEP;
    depthStencil.front.passOp = VK_STENCIL_OP_KEEP;
    depthStencil.front.compareMask = 0xff;
    depthStencil.front.writeMask = 0xff;
    depthStencil.front.reference = 2;
    break;
  case 2: // outside
    depthStencil.back.compareOp = VK_COMPARE_OP_EQUAL;
    depthStencil.back.failOp = VK_STENCIL_OP_KEEP;
    depthStencil.back.depthFailOp = VK_STENCIL_OP_KEEP;
    depthStencil.back.passOp = VK_STENCIL_OP_KEEP;
    depthStencil.back.compareMask = 0xff;
    depthStencil.back.writeMask = 0xff;
    depthStencil.back.reference = 1;
    depthStencil.front.compareOp = VK_COMPARE_OP_EQUAL;
    depthStencil.front.failOp = VK_STENCIL_OP_KEEP;
    depthStencil.front.depthFailOp = VK_STENCIL_OP_KEEP;
    depthStencil.front.passOp = VK_STENCIL_OP_KEEP;
    depthStencil.front.compareMask = 0xff;
    depthStencil.front.writeMask = 0xff;
    depthStencil.front.reference = 1;
    break;

  }
}

Vdp1GroundShadingClipInside::Vdp1GroundShadingClipInside(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm)
  : Vdp1GroundShading(vulkan, tm, vm) {
  prgid = PG_VFP1_GOURAUDSAHDING_CLIP_INSIDE;
  clipmode = 1;
}

Vdp1GroundShadingClipOutside::Vdp1GroundShadingClipOutside(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm)
  : Vdp1GroundShading(vulkan, tm, vm) {
  prgid = PG_VFP1_GOURAUDSAHDING_CLIP_OUTSIDE;
  clipmode = 2;
}


Vdp1GroundShading::Vdp1GroundShading(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm)
  : VdpPipeline(vulkan, tm, vm) {

  if (Vdp1GroundShading::polygonMode == PERSPECTIVE_CORRECTION) {
      vertexShaderName = R"S(
      layout(binding = 0) uniform UniformBufferObject {
         mat4 u_mvpMatrix;
         vec2 u_texsize;
         int u_fbowidth;
         int u_fbohegiht;
         float TessLevelInner;
         float TessLevelOuter;
      };

      layout (location = 0) in vec4 a_position;
      layout (location = 1) in vec4 a_grcolor;
      layout (location = 2) in vec4 a_texcoord;

      layout(location = 0) out  vec4 v_texcoord;
      layout(location = 1) out  vec4 v_vtxcolor;

      void main() {
         v_vtxcolor  = a_grcolor; 
         v_texcoord  = a_texcoord;
         v_texcoord.x  = v_texcoord.x / u_texsize.x;
         v_texcoord.y  = v_texcoord.y / u_texsize.y;
         gl_Position =  u_mvpMatrix * a_position;
      }
    )S";

      tessControll = "";
      tessEvaluation = "";
      geometry = "";
  }
  else /*if(Vdp1GroundShading::polygonMode == GPU_TESSERATION)*/ {
    vertexShaderName = R"S(

    layout(binding = 0) uniform UniformBufferObject {
       mat4 u_mvpMatrix;
       vec2 u_texsize;
       int u_fbowidth;
       int u_fbohegiht;
       float TessLevelInner;
       float TessLevelOuter;
    };


   layout (location = 0) in vec4 a_position;
   layout (location = 1) in vec4 a_grcolor; 
   layout (location = 2) in vec4 a_texcoord;
   layout (location = 0) out vec4 v_position; 
   layout (location = 1) out vec4 v_texcoord; 
   layout (location = 2) out vec4 v_vtxcolor;
   void main() {              
      v_position  = a_position; 
      v_vtxcolor  = a_grcolor;  
      v_texcoord  = a_texcoord; 
      v_texcoord.x  = v_texcoord.x / u_texsize.x; 
      v_texcoord.y  = v_texcoord.y / u_texsize.y;
    }
  )S";

    tessControll = R"S(

    layout(binding = 0) uniform UniformBufferObject {
       mat4 u_mvpMatrix;
       vec2 u_texsize;
       int u_fbowidth;
       int u_fbohegiht;
       float TessLevelInner;
       float TessLevelOuter;
    } ;


    layout(vertices = 4) out; //<???? what does it means?
    layout (location = 0) in vec4 v_position[];
    layout (location = 1) in vec4 v_texcoord[];
    layout (location = 2) in vec4 v_vtxcolor[];
    layout (location = 0) out vec4 tcPosition[];
    layout (location = 1) out vec4 tcTexCoord[];
    layout (location = 2) out vec4 tcColor[];
    #define ID gl_InvocationID
    void main() 
    { 
      tcPosition[ID] = v_position[ID];
    	tcTexCoord[ID] = v_texcoord[ID];
    	tcColor[ID] = v_vtxcolor[ID]; 
    	if (ID == 0) {
    		gl_TessLevelInner[0] = TessLevelInner;
    		gl_TessLevelInner[1] = TessLevelInner;
    		gl_TessLevelOuter[0] = TessLevelOuter;
    		gl_TessLevelOuter[1] = TessLevelOuter;
    		gl_TessLevelOuter[2] = TessLevelOuter;
    		gl_TessLevelOuter[3] = TessLevelOuter;
    	}
    }
  )S";

    tessEvaluation = R"S(

    layout(binding = 0) uniform UniformBufferObject {
       mat4 u_mvpMatrix;
       vec2 u_texsize;
       int u_fbowidth;
       int u_fbohegiht;
       float TessLevelInner;
       float TessLevelOuter;
    } ;


    layout(quads, equal_spacing, ccw) in;
    layout (location = 0) in vec4 tcPosition[];
    layout (location = 1) in vec4 tcTexCoord[];
    layout (location = 2) in vec4 tcColor[];
    layout (location = 0) out vec4 teTexCoord;
    layout (location = 1) out vec4 teColor;
    void main()
    {
  	  float u = gl_TessCoord.x, v = gl_TessCoord.y;
  	  vec4 tePosition;
  	  vec4 a = mix(tcPosition[0], tcPosition[3], u);
  	  vec4 b = mix(tcPosition[1], tcPosition[2], u);
  	  tePosition = mix(a, b, v);
  	  gl_Position = u_mvpMatrix * tePosition;
  	  vec4 ta = mix(tcTexCoord[0], tcTexCoord[3], u);
  	  vec4 tb = mix(tcTexCoord[1], tcTexCoord[2], u);
  	  teTexCoord = mix(ta, tb, v); 
  	  vec4 ca = mix(tcColor[0], tcColor[3], u);
  	  vec4 cb = mix(tcColor[1], tcColor[2], u);
  	  teColor = mix(ca, cb, v);
    }
  )S";

    geometry = R"S(

    layout(binding = 0) uniform UniformBufferObject {
       mat4 u_mvpMatrix;
       vec2 u_texsize;
       int u_fbowidth;
       int u_fbohegiht;
       float TessLevelInner;
       float TessLevelOuter;
    } ;


    layout(triangles) in;
    layout(triangle_strip, max_vertices = 3) out;
    layout(location = 0) in vec4 teTexCoord[3];
    layout(location = 1) in vec4 teColor[3];
    layout(location = 0) out vec4 v_texcoord;
    layout(location = 1) out vec4 v_vtxcolor;
    void main()
    { 
  	  v_texcoord = teTexCoord[0];
  	  v_vtxcolor = teColor[0];
  	  gl_Position = gl_in[0].gl_Position; EmitVertex();
  	  v_texcoord = teTexCoord[1];
  	  v_vtxcolor = teColor[1];
  	  gl_Position = gl_in[1].gl_Position; EmitVertex();
  	  v_texcoord = teTexCoord[2];
  	  v_vtxcolor = teColor[2];
  	  gl_Position = gl_in[2].gl_Position; EmitVertex();
  	  EndPrimitive();
    } 
  )S";

  }

  bindid.clear();
  bindid.push_back(bindIdTexture);

  fragShaderName = R"S(
    layout(binding = 1) uniform sampler2D u_sprite;
    layout(location = 0) in vec4 v_texcoord;
    layout(location = 1) in vec4 v_vtxcolor;
    layout(location = 0) out vec4 fragColor;

    void main() { 

      vec2 addr = v_texcoord.st;
      addr.s = addr.s / (v_texcoord.q);
      addr.t = addr.t / (v_texcoord.q);
      vec4 spriteColor = texture(u_sprite,addr);
      if( spriteColor.a == 0.0 ) discard; 
      fragColor = spriteColor+v_vtxcolor;
      fragColor.a = spriteColor.a;
    }
  )S";
  prgid = PG_VFP1_GOURAUDSAHDING;
  clipmode = 0;
}

Vdp1GroundShadingTess::Vdp1GroundShadingTess(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm)
  : Vdp1GroundShading(vulkan, tm, vm) {



}

Vdp1GroundShadingSpd::Vdp1GroundShadingSpd(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm)
  : Vdp1GroundShading(vulkan, tm, vm) {

  bindid.clear();
  bindid.push_back(bindIdTexture);

  fragShaderName = R"S(
    layout(binding = 1) uniform sampler2D u_sprite;
    layout(location = 0) in vec4 v_texcoord;
    layout(location = 1) in vec4 v_vtxcolor;
    layout(location = 0) out vec4 fragColor;

    void main() { 

      vec2 addr = v_texcoord.st;
      addr.s = addr.s / (v_texcoord.q);
      addr.t = addr.t / (v_texcoord.q);
      vec4 spriteColor = texture(u_sprite,addr);
      fragColor = spriteColor;
      fragColor = clamp(spriteColor+v_vtxcolor,vec4(0.0),vec4(1.0));
      fragColor.a = spriteColor.a;
    }
  )S";
  prgid = PG_VFP1_GOURAUDSAHDING_SPD;
}

Vdp1GroundShadingSpdClipInside::Vdp1GroundShadingSpdClipInside(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm)
  : Vdp1GroundShadingSpd(vulkan, tm, vm)
{
  prgid = PG_VFP1_GOURAUDSAHDING_SPD_CLIP_INSIDE;
  this->clipmode = 1;
}

Vdp1GroundShadingSpdClipOutside::Vdp1GroundShadingSpdClipOutside(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm)
  : Vdp1GroundShadingSpd(vulkan, tm, vm)
{
  prgid = PG_VFP1_GOURAUDSAHDING_SPD_CLIP_OUTSIDE;
  this->clipmode = 2;
}

VDP1Mesh::VDP1Mesh(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm) : Vdp1GroundShading(vulkan, tm, vm) {

  bindid.clear();
  bindid.push_back(bindIdTexture);

  fragShaderName = R"S(
    layout(binding = 1) uniform sampler2D u_sprite;
    layout(location = 0) in vec4 v_texcoord;
    layout(location = 1) in vec4 v_vtxcolor;
    layout(location = 0) out vec4 fragColor;

    void main() {                                                                                
      vec2 addr = v_texcoord.st;
      addr.s = addr.s / (v_texcoord.q);
      addr.t = addr.t / (v_texcoord.q);
      vec4 spriteColor = texture(u_sprite,addr);
      if( spriteColor.a == 0.0 ) discard;
      if( (int(gl_FragCoord.y) & 0x01) == 0 ){
        if( (int(gl_FragCoord.x) & 0x01) == 0 ){
           discard;
        }
      }else{
        if( (int(gl_FragCoord.x) & 0x01) == 1 ){
           discard;
        }
      }
      spriteColor += vec4(v_vtxcolor.r,v_vtxcolor.g,v_vtxcolor.b,0.0);
      fragColor = spriteColor;
    }
  )S";

  prgid = PG_VFP1_MESH;
}

VDP1MeshClipInside::VDP1MeshClipInside(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm)
  : VDP1Mesh(vulkan, tm, vm)
{
  prgid = PG_VFP1_MESH_CLIP_INSIDE;
  this->clipmode = 1;
}

VDP1MeshClipOutside::VDP1MeshClipOutside(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm)
  : VDP1Mesh(vulkan, tm, vm)
{
  prgid = PG_VFP1_MESH_CLIP_OUTSIDE;
  this->clipmode = 2;
}

VDP1HalfLuminance::VDP1HalfLuminance(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm) :
  Vdp1GroundShading(vulkan, tm, vm) {

  bindid.clear();
  bindid.push_back(bindIdTexture);
  bindid.push_back(bindIdFbo);


  fragShaderName = R"S(
    layout(binding = 0) uniform UniformBufferObject {
       mat4 u_mvpMatrix;
       vec2 u_texsize;
       int u_fbowidth;
       int u_fbohegiht;
    };
    layout(binding = 1) uniform sampler2D u_sprite;
    layout(binding = 2) uniform sampler2D u_fbo;
    layout(location = 0) in vec4 v_texcoord;
    layout(location = 1) in vec4 v_vtxcolor;
    layout(location = 0) out vec4 fragColor;

    void main() {                                                                                
      vec2 addr = v_texcoord.st;
      addr.s = addr.s / (v_texcoord.q);
      addr.t = addr.t / (v_texcoord.q);
      vec4 FragColor = texture( u_sprite, addr ); 
      if( FragColor.a == 0.0 ) discard; 
      fragColor.r = FragColor.r * 0.5;
      fragColor.g = FragColor.g * 0.5;
      fragColor.b = FragColor.b * 0.5;
      fragColor.a = FragColor.a;
    }
  )S";

  prgid = PG_VFP1_HALF_LUMINANCE;
}

VDP1HalfLuminanceClipInside::VDP1HalfLuminanceClipInside(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm)
  : VDP1HalfLuminance(vulkan, tm, vm)
{
  prgid = PG_VFP1_HALF_LUMINANCE_INSIDE;
  this->clipmode = 1;
}

VDP1HalfLuminanceClipOutside::VDP1HalfLuminanceClipOutside(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm)
  : VDP1HalfLuminance(vulkan, tm, vm)
{
  prgid = PG_VFP1_HALF_LUMINANCE_OUTSIDE;
  this->clipmode = 2;
}

VDP1Shadow::VDP1Shadow(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm) :
  VDP1GlowShadingAndHalfTransOperation(vulkan, tm, vm) {

  bindid.clear();
  bindid.push_back(bindIdTexture);
  bindid.push_back(bindIdFbo);

  fragShaderName = R"S(
    layout(binding = 0) uniform UniformBufferObject {
       mat4 u_mvpMatrix;
       vec2 u_texsize;
       int u_fbowidth;
       int u_fbohegiht;
    };
    layout(binding = 1) uniform sampler2D u_sprite;
    layout(binding = 2) uniform sampler2D u_fbo;
    layout(location = 0) in vec4 v_texcoord;
    layout(location = 1) in vec4 v_vtxcolor;
    layout(location = 0) out vec4 fragColor;

    void main() {                                                                                
      vec2 addr = v_texcoord.st;
      vec2 faddr = vec2( gl_FragCoord.x/float(u_fbowidth), gl_FragCoord.y/float(u_fbohegiht));
      addr.s = addr.s / (v_texcoord.q);
      addr.t = addr.t / (v_texcoord.q);
      vec4 spriteColor = texture(u_sprite,addr);
      if( spriteColor.a == 0.0 ){ discard; }
      vec4 fboColor = texture(u_fbo,faddr);
      int additional = int(fboColor.a * 255.0);
      if( ((additional & 0xC0)==0x80) ) {
        fragColor = vec4(fboColor.r*0.5,fboColor.g*0.5,fboColor.b*0.5,fboColor.a);
      }else{
        discard;
      }
    }
  )S";

  prgid = PG_VFP1_SHADOW;
}

VDP1ShadowClipInside::VDP1ShadowClipInside(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm)
  : VDP1Shadow(vulkan, tm, vm)
{
  prgid = PG_VFP1_SHADOW_CLIP_INSIDE;
  this->clipmode = 1;
}

VDP1ShadowClipOutsize::VDP1ShadowClipOutsize(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm)
  : VDP1Shadow(vulkan, tm, vm)
{
  prgid = PG_VFP1_SHADOW_CLIP_OUTSIDE;
  this->clipmode = 2;
}

VDP1GlowShadingAndHalfTransOperation::VDP1GlowShadingAndHalfTransOperation(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm)
  : Vdp1GroundShading(vulkan, tm, vm) {

  bindid.clear();
  bindid.push_back(bindIdTexture);
  bindid.push_back(bindIdFbo);

  fragShaderName = R"S(
    layout(binding = 0) uniform UniformBufferObject {
       mat4 u_mvpMatrix;
       vec2 u_texsize;
       int u_fbowidth;
       int u_fbohegiht;
    };
    layout(binding = 1) uniform sampler2D u_sprite;
    layout(binding = 2) uniform sampler2D u_fbo;
    layout(location = 0) in vec4 v_texcoord;
    layout(location = 1) in vec4 v_vtxcolor;
    layout(location = 0) out vec4 fragColor;
    void main() { 
      vec2 addr = v_texcoord.st;                                                               
      vec2 faddr = vec2( gl_FragCoord.x/float(u_fbowidth), gl_FragCoord.y/float(u_fbohegiht));
      addr.s = addr.s / (v_texcoord.q);
      addr.t = addr.t / (v_texcoord.q);
      vec4 spriteColor = texture(u_sprite,addr);
      if( spriteColor.a == 0.0 ) discard;
      vec4 fboColor    = texture(u_fbo,faddr);
      int additional = int(fboColor.a * 255.0);
      spriteColor += vec4(v_vtxcolor.r,v_vtxcolor.g,v_vtxcolor.b,0.0);
      if( (additional & 0x40) == 0 )
      {
        fragColor = spriteColor*0.5 + fboColor*0.5;
        fragColor.a = spriteColor.a;
      }else{
        fragColor = spriteColor;
      }
    }
  )S";
  prgid = PG_VFP1_GOURAUDSAHDING_HALFTRANS;
  needBarrier = true;

}

VDP1GlowShadingAndHalfTransOperationClipInside::VDP1GlowShadingAndHalfTransOperationClipInside(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm)
  : VDP1GlowShadingAndHalfTransOperation(vulkan, tm, vm)
{
  prgid = PG_VFP1_GOURAUDSAHDING_HALFTRANS_CLIP_INSIDE;
  this->clipmode = 1;
}

VDP1GlowShadingAndHalfTransOperationClipOutside::VDP1GlowShadingAndHalfTransOperationClipOutside(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm)
  : VDP1GlowShadingAndHalfTransOperation(vulkan, tm, vm)
{
  prgid = PG_VFP1_GOURAUDSAHDING_HALFTRANS_CLIP_OUTSIDE;
  this->clipmode = 2;
}


VdpRbgCramLinePipeline::VdpRbgCramLinePipeline(VIDVulkan * vulkan, TextureManager * tm, VertexManager * vm)
  : VdpPipeline(vulkan, tm, vm) {

  bindid.clear();
  bindid.push_back(bindIdTexture);
  bindid.push_back(bindIdColorRam);
  bindid.push_back(bindIdLine);
  bindid.push_back(bindIdWindow);

  prgid = PG_VDP2_RBG_CRAM_LINE;

  fragShaderName =
    vdp2Uniform +
    "layout(location = 0) in vec4 v_texcoord;\n"
    "layout(binding = 1) uniform highp sampler2D s_texture;\n"
    "layout(binding = 2) uniform highp sampler2D s_color;\n"
    "layout(binding = 3) uniform highp sampler2D s_line_texture;\n"
    "layout(binding = 4) uniform highp sampler2D windowSampler;"
    "layout (location = 0) out vec4 fragColor;\n"
    "void main()\n"
    "{\n"
    "    if (winmode != -1) { \n"
    "      vec2 winaddr = vec2(gl_FragCoord.x / float(windowWidth), gl_FragCoord.y / float(windowHeight)); \n"
    "      vec4 wintexture = texture(windowSampler, winaddr); \n"
    "      int winvalue = int(wintexture.r * 255.0); \n"
    "      // and \n"
    "      if (winmode == 0) {    \n"
    "       if ((winvalue & winmask) != winflag) { \n"
    "         discard; \n"
    "       } \n"
    "       // or \n"
    "     } \n"
    "     else { \n"
    "       if ((winvalue & winmask) == winflag) { \n"
    "         discard; \n"
    "       } \n"
    "     } \n"
    "   } \n"
    "  vec4 txindex = texelFetch( s_texture, ivec2(int(v_texcoord.x),int(v_texcoord.y)) ,0 );         \n"
    "  if(txindex.a > 0.0) {\n"
    "    highp int highg = int(txindex.g*255.0);"
    "    vec4 txcol = texelFetch( s_color, ivec2( ((highg&0x7F)<<8) | int(txindex.r*255.0) , 0 ) , 0 );\n"
    "    txcol.a = txindex.a; \n"
    "    if( (highg & 0x80)  != 0) {\n"
    "      int coef = int(txindex.b*255.0);\n"
    "      vec4 linecol;\n"
    "      vec4 lineindex = texelFetch( s_line_texture,  ivec2( int(v_texcoord.z),int(v_texcoord.w))  ,0 );\n"
    "      int lineparam = ((int(lineindex.g*255.0) & 0x7F)<<8) | int(lineindex.r*255.0); \n"
    "      if( (coef & 0x80) != 0 ){\n"
    "        int caddr = (lineparam&0x780) | (coef&0x7F);\n "
    "        linecol = texelFetch( s_color, ivec2( caddr,0  ) , 0 );\n"
    "      }else{\n"
    "        linecol = texelFetch( s_color, ivec2( lineparam , 0 ) , 0 );\n"
    "      }\n"
    "      if( u_blendmode == 1 ) { \n"
    "        txcol = mix(txcol,  linecol , 1.0-txindex.a); txcol.a = txindex.a + 0.25;\n"
    "      }else if( u_blendmode == 2 ) {\n"
    "        txcol = clamp(txcol+linecol,vec4(0.0),vec4(1.0)); txcol.a = txindex.a; \n"
    "      }\n"
    "    }\n"
    "    fragColor = clamp(txcol+u_color_offset,vec4(0.0),vec4(1.0));\n"
    "  }else \n"
    "    discard;\n"
    "}\n";

}

