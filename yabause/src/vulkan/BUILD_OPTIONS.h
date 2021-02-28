/* -----------------------------------------------------
This source code is public domain ( CC0 )
The code is provided as-is without limitations, requirements and responsibilities.
Creators and contributors to this source code are provided as a token of appreciation
and no one associated with this source code can be held responsible for any possible
damages or losses of any kind.

Original file creator:  Niko Kauppi (Code maintenance)
Contributors:
----------------------------------------------------- */

#pragma once

#define BUILD_ENABLE_VULKAN_DEBUG								1
#define BUILD_ENABLE_VULKAN_RUNTIME_DEBUG						1

// To use GLFW, make sure that it's available in the include directories
// along with library directories. You will also need to add
// GLFWs "glfw3.lib" file into the project, this task is up to you.
// GLFW version 3.2 or newer is required.
#if defined(ANDROID)
#else
#define BUILD_USE_GLFW											1
#endif
