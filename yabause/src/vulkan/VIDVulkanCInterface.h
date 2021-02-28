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

#ifndef VIDVULKAN_H
#define VIDVULKAN_H

#include "vdp1.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VIDCORE_VULKAN   4
  extern VideoInterface_struct CVIDVulkan;

  void VIDVulkanVdp2DrawStart(void);
  void VIDVulkanVdp2DrawEnd(void);
  void VIDVulkanVdp2DrawScreens(void);

#ifdef __cplusplus
}
#endif

#endif