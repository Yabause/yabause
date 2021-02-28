/* -----------------------------------------------------
This source code is public domain ( CC0 )
The code is provided as-is without limitations, requirements and responsibilities.
Creators and contributors to this source code are provided as a token of appreciation
and no one associated with this source code can be held responsible for any possible
damages or losses of any kind.

Original file creator:  Teagan Chouinard (GLFW)
Contributors:
Niko Kauppi (Code maintenance)
----------------------------------------------------- */

#include "BUILD_OPTIONS.h"
#include "Platform.h"

#include "Window.h"
#include "Shared.h"
#include "Renderer.h"

#include <assert.h>
#include <iostream>

#include "VulkanTools.h"

#if defined(__ANDROID__)
#include <vulkan/vulkan_android.h>

void Window::_InitOSWindow()
{	

}

void Window::_DeInitOSWindow()
{

}

void Window::_UpdateOSWindow()
{

}

void Window::_InitOSSurface()
{
/*    
	if ( VK_SUCCESS != glfwCreateWindowSurface( _renderer->GetVulkanInstance(), _glfw_window, nullptr, &_surface ) ){
		glfwTerminate();
		assert ( 0 && "GLFW could not create the window surface." );
		return;
	}
*/
    LOGI("_InitOSSurface %08lX",(intptr_t)window);

	VkAndroidSurfaceCreateInfoKHR info{VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR};
	info.window = window;

	if( VK_SUCCESS != vkCreateAndroidSurfaceKHR(_renderer->GetVulkanInstance(), &info, nullptr, &_surface)){
		assert ( 0 && "Android could not create the window surface." );
        LOGI("_InitOSSurface FAIL: %08lX",(intptr_t)info.window);
		return;
    }

    LOGI("_InitOSSurface OK: %08lX",(intptr_t)_surface);
	return;


}

#endif // USE_FRAMEWORK_GLFW
