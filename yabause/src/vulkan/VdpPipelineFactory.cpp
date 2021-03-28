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

#include "VdpPipelineFactory.h"
#include "VdpPipeline.h"
#include "VIDVulkan.h"


VdpPipelineFactory::VdpPipelineFactory() {

}

VdpPipelineFactory::~VdpPipelineFactory() {
  for (int i = 0; i < garbageCollction.size(); i++) {
    delete garbageCollction[i];
  }
  dicardAllPielines();
}


VdpPipeline * VdpPipelineFactory::getPipeline(
  YglPipelineId id,
  VIDVulkan * vulkan,
  TextureManager * tm,
  VertexManager * vm
) {

  VdpPipeline * current = nullptr;

  current = findInGarbage(id);
  if (current != NULL) {
    return current;
  }

  switch (id) {
  case PG_VDP2_NORMAL_CRAM:
    current = new VdpPipelineCram(vulkan, tm, vm);
    break;
  case PG_NORMAL:
    current = new VdpPipeline(vulkan, tm, vm);
    break;
  case PG_VFP1_GOURAUDSAHDING:
    current = new Vdp1GroundShading(vulkan, tm, vm);
    break;
  case PG_VFP1_GOURAUDSAHDING_CLIP_INSIDE:
    current = new Vdp1GroundShadingClipInside(vulkan, tm, vm);
    break;
  case PG_VFP1_GOURAUDSAHDING_CLIP_OUTSIDE:
    current = new Vdp1GroundShadingClipOutside(vulkan, tm, vm);
    break;
  case PG_VFP1_GOURAUDSAHDING_SPD:
    current = new Vdp1GroundShadingSpd(vulkan, tm, vm);
    break;
  case PG_VFP1_GOURAUDSAHDING_SPD_CLIP_INSIDE:
    current = new Vdp1GroundShadingSpdClipInside(vulkan, tm, vm);
    break;
  case PG_VFP1_GOURAUDSAHDING_SPD_CLIP_OUTSIDE:
    current = new Vdp1GroundShadingSpdClipOutside(vulkan, tm, vm);
    break;
  case PG_VDP2_RBG_CRAM_LINE:
    current = new VdpRbgCramLinePipeline(vulkan, tm, vm);
    break;
  case PG_VFP1_GOURAUDSAHDING_HALFTRANS:
    current = new VDP1GlowShadingAndHalfTransOperation(vulkan, tm, vm);
    break;
  case PG_VFP1_GOURAUDSAHDING_HALFTRANS_CLIP_INSIDE:
    current = new VDP1GlowShadingAndHalfTransOperationClipInside(vulkan, tm, vm);
    break;
  case PG_VFP1_GOURAUDSAHDING_HALFTRANS_CLIP_OUTSIDE:
    current = new VDP1GlowShadingAndHalfTransOperationClipOutside(vulkan, tm, vm);
    break;
  case PG_VFP1_MESH:
    current = new VDP1Mesh(vulkan, tm, vm);
    break;
  case PG_VFP1_MESH_CLIP_INSIDE:
    current = new VDP1MeshClipInside(vulkan, tm, vm);
    break;
  case PG_VFP1_MESH_CLIP_OUTSIDE:
    current = new VDP1MeshClipOutside(vulkan, tm, vm);
    break;
  case PG_VFP1_HALF_LUMINANCE:
    current = new VDP1HalfLuminance(vulkan, tm, vm);
    break;
  case PG_VFP1_HALF_LUMINANCE_INSIDE:
    current = new VDP1HalfLuminanceClipInside(vulkan, tm, vm);
    break;
  case PG_VFP1_HALF_LUMINANCE_OUTSIDE:
    current = new VDP1HalfLuminanceClipOutside(vulkan, tm, vm);
    break;
  case PG_VFP1_SHADOW:
    current = new VDP1Shadow(vulkan, tm, vm);
    break;
  case PG_VFP1_SHADOW_CLIP_INSIDE:
    current = new VDP1ShadowClipInside(vulkan, tm, vm);
    break;
  case PG_VFP1_SHADOW_CLIP_OUTSIDE:
    current = new VDP1ShadowClipOutsize(vulkan, tm, vm);
    break;
  case PG_VDP2_ADDBLEND:
    current = new VdpPipelineAdd(vulkan, tm, vm);
    break;
  case PG_VDP2_ADDCOLOR_CRAM:
    current = new VdpPipelineCramAdd(vulkan, tm, vm);
    break;
  case PG_LINECOLOR_INSERT:
    current = new VdpLineCol(vulkan, tm, vm);
    break;
  case PG_LINECOLOR_INSERT_CRAM:
    current = new VdpCramLine(vulkan, tm, vm);
    break;
  case PG_VDP2_MOSAIC:
    current = new VdpPipelineMosaic(vulkan, tm, vm);
    break;
  case PG_VDP1_SYSTEM_CLIP:
    current = new VDP1SystemClip(vulkan, tm, vm);
    break;
  case PG_VDP1_USER_CLIP:
    current = new VDP1UserClip(vulkan, tm, vm);
    break;
  case PG_NORMAL_DSTALPHA:
    current = new VdpPipelineDst(vulkan, tm, vm);
    break;
  case PG_VDP2_NORMAL_CRAM_DSTALPHA:
    current = new VdpPipelineCramDst(vulkan, tm, vm);
    break;
  case PG_VDP2_NOBLEND:
    current = new VdpPipelineNoblend(vulkan, tm, vm);
    break;
  case PG_VDP2_NOBLEND_CRAM:
    current = new VdpPipelineCramNoblend(vulkan, tm, vm);
    break;
  case PG_VDP2_PER_LINE_ALPHA_CRAM:
    current = new VdpPipelinePreLineAlphaCram(vulkan, tm, vm);
    break;
  case PG_VDP2_NORMAL_CRAM_SPECIAL_PRIORITY_COLOROFFSET:
    current = new VdpPipelineSpecialPriorityColorOffset(vulkan, tm, vm);
    break;
  case PG_VDP2_PER_LINE_ALPHA_DST:
    current = new VdpPipelinePerLineDst(vulkan, tm, vm);
    break;
  case PG_VDP2_PER_LINE_ALPHA:
    current = new VdpPipelinePerLine(vulkan, tm, vm);
    break;
  default:
    assert(0);
    current = new VdpPipeline(vulkan, tm, vm);
    break;
  }

  current->setRenderPass(this->renderPass);
  current->createGraphicsPipeline();
  return current;
}

void VdpPipelineFactory::garbage(VdpPipeline * p) {

  for (int i = 0; i < garbageCollction.size(); i++) {
    if (garbageCollction[i] == p) {
      return;
    }
  }

  p->indices.clear();
  p->vertices.clear();
  garbageCollction.push_back(p);
}

VdpPipeline * VdpPipelineFactory::findInGarbage(YglPipelineId id) {

  VdpPipeline * p;
  int removeIndex = -1;
  for (int i = 0; i < garbageCollction.size(); i++) {
    if (garbageCollction[i]->prgid == id) {
      p = garbageCollction[i];
      removeIndex = i;
      break;
    }
  }

  if (removeIndex != -1) {
    garbageCollction.erase(garbageCollction.begin() + removeIndex);
    return p;
  }
  return NULL;
}

void VdpPipelineFactory::dicardAllPielines() {
  garbageCollction.clear();
}