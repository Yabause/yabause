
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

#include "ygl.h"
#include "VulkanScene.h"

#include <vector>
using std::vector;

class VdpPipeline;
class VIDVulkan;
class TextureManager;
class VertexManager;

class VdpPipelineFactory {
public:
  VdpPipelineFactory();
  ~VdpPipelineFactory();

  VdpPipeline * getPipeline(
    YglPipelineId id,
    VIDVulkan * vulkan,
    TextureManager * tm,
    VertexManager * vm
  );

  void setRenderPath(VkRenderPass renderPass) {
    this->renderPass = renderPass;
  }
  void garbage(VdpPipeline * p);

  void dicardAllPielines();

protected:

  VdpPipeline * findInGarbage(YglPipelineId id);
  vector<VdpPipeline*> garbageCollction;

  VkRenderPass renderPass;
};