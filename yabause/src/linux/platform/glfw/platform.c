#include <stdio.h>

#if defined(_USEGLEW_)
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

static GLFWwindow* g_window = NULL;
static GLFWwindow* g_offscreen_context;

int platform_YuiRevokeOGLOnThisThread(){
#if defined(YAB_ASYNC_RENDERING)
  glfwMakeContextCurrent(g_offscreen_context);
#endif
  return 0;
}

int platform_YuiUseOGLOnThisThread(){
#if defined(YAB_ASYNC_RENDERING)
  glfwMakeContextCurrent(g_window);
#endif
  return 0;
}

void platform_swapBuffers(void) {
   if( g_window == NULL ){
      return;
   }
   glfwSwapBuffers(g_window);
}

static void error_callback(int error, const char* description)
{
  fputs(description, stderr);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GL_TRUE);
}

int platform_SetupOpenGL(int w, int h) {
  if (!glfwInit())
    return 0;

  glfwSetErrorCallback(error_callback);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API) ;
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_RED_BITS,8);
  glfwWindowHint(GLFW_GREEN_BITS,8);
  glfwWindowHint(GLFW_BLUE_BITS,8);

  g_window = glfwCreateWindow(w, h, "Yabause", NULL, NULL);

  if (!g_window)
  {
    glfwTerminate();
    return 0;
  }

  glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
  g_offscreen_context = glfwCreateWindow(w,h, "", NULL, g_window);

  glfwMakeContextCurrent(g_window);
  glfwSwapInterval(0);
#if defined(_USEGLEW_)
  glewExperimental=GL_TRUE;
#endif

  glfwSetKeyCallback(g_window, key_callback);
  return 1;
}

int platform_shouldClose() {
  return glfwWindowShouldClose(g_window);
}

void platform_Close() {
  glfwSetWindowShouldClose(g_window, GL_TRUE);
}

int platform_Deinit(void) {
  glfwDestroyWindow(g_window);
  glfwDestroyWindow(g_offscreen_context);
  glfwTerminate();
}

void platform_HandleEvent(void) {
  glfwPollEvents();
}

