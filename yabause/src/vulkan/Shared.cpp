/* -----------------------------------------------------
This source code is public domain ( CC0 )
The code is provided as-is without limitations, requirements and responsibilities.
Creators and contributors to this source code are provided as a token of appreciation
and no one associated with this source code can be held responsible for any possible
damages or losses of any kind.

Original file creator:  Niko Kauppi (Code maintenance)
Contributors:
----------------------------------------------------- */
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

#include "BUILD_OPTIONS.h"
#include "Shared.h"

#include "VulkanTools.h"

#include <sstream>
using std::ostringstream;

#if defined(ANDROID)
#include <iostream>
#include <iomanip>

#include <unwind.h>
#include <dlfcn.h>

namespace {

  struct BacktraceState
  {
    void** current;
    void** end;
  };

  static _Unwind_Reason_Code unwindCallback(struct _Unwind_Context* context, void* arg)
  {
    BacktraceState* state = static_cast<BacktraceState*>(arg);
    uintptr_t pc = _Unwind_GetIP(context);
    if (pc) {
      if (state->current == state->end) {
        return _URC_END_OF_STACK;
      }
      else {
        *state->current++ = reinterpret_cast<void*>(pc);
      }
    }
    return _URC_NO_REASON;
  }

}

size_t captureBacktrace(void** buffer, size_t max)
{
  BacktraceState state = { buffer, buffer + max };
  _Unwind_Backtrace(unwindCallback, &state);

  return state.current - buffer;
}

void dumpBacktrace(std::ostream& os, void** buffer, size_t count)
{
  for (size_t idx = 0; idx < count; ++idx) {
    const void* addr = buffer[idx];
    const char* symbol = "";

    Dl_info info;
    if (dladdr(addr, &info) && info.dli_sname) {
      symbol = info.dli_sname;
    }

    os << "  #" << std::setw(2) << idx << ": " << addr << "  " << symbol << "\n";
  }
}

#include <sstream>
#include <android/log.h>

void backtraceToLogcat()
{
  const size_t max = 30;
  void* buffer[max];
  std::ostringstream oss;

  dumpBacktrace(oss, buffer, captureBacktrace(buffer, max));

  __android_log_print(ANDROID_LOG_INFO, "app_name", "%s", oss.str().c_str());
}

#endif

#if BUILD_ENABLE_VULKAN_RUNTIME_DEBUG

void ErrorCheck(VkResult result)
{
  std::ostringstream stream;
  if (result < 0) {
    switch (result) {
    case VK_ERROR_OUT_OF_HOST_MEMORY:
      stream << "VK_ERROR_OUT_OF_HOST_MEMORY" << std::endl;
      break;
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
      stream << "VK_ERROR_OUT_OF_DEVICE_MEMORY" << std::endl;
      break;
    case VK_ERROR_INITIALIZATION_FAILED:
      stream << "VK_ERROR_INITIALIZATION_FAILED" << std::endl;
      break;
    case VK_ERROR_DEVICE_LOST:
      stream << "VK_ERROR_DEVICE_LOST" << std::endl;
      break;
    case VK_ERROR_MEMORY_MAP_FAILED:
      stream << "VK_ERROR_MEMORY_MAP_FAILED" << std::endl;
      break;
    case VK_ERROR_LAYER_NOT_PRESENT:
      stream << "VK_ERROR_LAYER_NOT_PRESENT" << std::endl;
      break;
    case VK_ERROR_EXTENSION_NOT_PRESENT:
      stream << "VK_ERROR_EXTENSION_NOT_PRESENT" << std::endl;
      break;
    case VK_ERROR_FEATURE_NOT_PRESENT:
      stream << "VK_ERROR_FEATURE_NOT_PRESENT" << std::endl;
      break;
    case VK_ERROR_INCOMPATIBLE_DRIVER:
      stream << "VK_ERROR_INCOMPATIBLE_DRIVER" << std::endl;
      break;
    case VK_ERROR_TOO_MANY_OBJECTS:
      stream << "VK_ERROR_TOO_MANY_OBJECTS" << std::endl;
      break;
    case VK_ERROR_FORMAT_NOT_SUPPORTED:
      stream << "VK_ERROR_FORMAT_NOT_SUPPORTED" << std::endl;
      break;
    case VK_ERROR_SURFACE_LOST_KHR:
      stream << "VK_ERROR_SURFACE_LOST_KHR" << std::endl;
      break;
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
      stream << "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR" << std::endl;
      break;
    case VK_SUBOPTIMAL_KHR:
      stream << "VK_SUBOPTIMAL_KHR" << std::endl;
      break;
    case VK_ERROR_OUT_OF_DATE_KHR:
      stream << "VK_ERROR_OUT_OF_DATE_KHR" << std::endl;
      break;
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
      stream << "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR" << std::endl;
      break;
    case VK_ERROR_VALIDATION_FAILED_EXT:
      stream << "VK_ERROR_VALIDATION_FAILED_EXT" << std::endl;
      break;
    default:
      break;
    }
#if (ANDROID)
    LOGE("%s", stream.str().c_str());
    backtraceToLogcat();
#endif
    std::cout << stream.str();
    //assert( 0 && "Vulkan runtime error." );
  }
}

#else

void ErrorCheck(VkResult result) {};

#endif // BUILD_ENABLE_VULKAN_RUNTIME_DEBUG

uint32_t FindMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties * gpu_memory_properties, const VkMemoryRequirements * memory_requirements, const VkMemoryPropertyFlags required_properties)
{
  for (uint32_t i = 0; i < gpu_memory_properties->memoryTypeCount; ++i) {
    if (memory_requirements->memoryTypeBits & (1 << i)) {
      if ((gpu_memory_properties->memoryTypes[i].propertyFlags & required_properties) == required_properties) {
        return i;
      }
    }
  }
  assert(0 && "Couldn't find proper memory type.");
  return UINT32_MAX;
}
