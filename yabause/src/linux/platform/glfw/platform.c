#include <stdio.h>
#include "platform.h"
#include "../peripheral.h"
#include "../sh2core.h"

#if defined(_USEGLEW_)
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

static GLFWwindow* g_window = NULL;
static GLFWwindow* g_offscreen_context;

static k_callback k_call = NULL;
static unsigned char inputMap[512];

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

static void handleIntrospection(int key, int action) {
  char buffer[128];
  char command;
  int addr;
  int addr2;
  int param;
  int i;
  char res[512];
  int introspectionMode;
  if ((key == GLFW_KEY_I) && (action == GLFW_PRESS)) {
     introspectionMode = 1;
     printf("Introspection Mode\n");
     while (introspectionMode == 1){
       command = 0;
       printf("Command: ");
       scanf("%c", &command);
       switch(command) {
          case 'i':
            introspectionMode = 0;
            printf("End of Introspection Mode\n");
          break;
          case 'd':
            printf("Disassemble\n");
            printf("Address? (0x.....): ");
            scanf("%x", &addr);
	    SH2Disasm(addr, MappedMemoryReadWord(MSH2, addr), 0, &(MSH2->regs), res);
            printf("%s\n", res);
          break;
          case 'r':
            printf("Read Size?(b,w,l): ");
            scanf("%s", &param);
            printf("Address? (0x.....): ");
            scanf("%x", &addr);
            if (param == 'b')
              printf("0x%x\n", MappedMemoryReadByte(MSH2, addr));
            if (param == 'w')
              printf("0x%x\n", MappedMemoryReadWord(MSH2, addr));
            if (param == 'l')
              printf("0x%x\n", MappedMemoryReadLong(MSH2, addr));
          break;
          case 'l':
            printf("Dump Size?(b,w,l): ");
            scanf("%s", &param);
            printf("Start Address? (0x.....): ");
            scanf("%x", &addr);
            printf("End Address? (0x.....): ");
            scanf("%x", &addr2);
            if (param == 'b')
              for(i=addr; i<= addr2; i++) printf("0x%x = 0x%x\n", i, MappedMemoryReadByte(MSH2, i));
            if (param == 'w')
              for(i=(addr&0xFFFFFFFE); i<= addr2; i+=2) printf("0x%x = 0x%x\n", i, MappedMemoryReadWord(MSH2, i));
            if (param == 'l')
              for(i=(addr&0xFFFFFFFC); i<= addr2; i+=4) printf("0x%x = 0x%x\n", i, MappedMemoryReadLong(MSH2, i));
          break;
          case 's':
            printf("Search Size?(b,w,l): ");
            scanf("%s", &param);
            printf("Value? (0x.....): ");
            scanf("%x", &addr);
            printf("Look for 0x%x\n", addr);
            if (param == 'b')
              for(i=0x0; i< 0x7000000; i++) if (addr == MappedMemoryReadByte(MSH2, i)) printf("0x%x = 0x%x\n", i, addr);
            if (param == 'w')
              for(i=0x0; i< 0x7000000; i+=2) if (addr == MappedMemoryReadWord(MSH2, i)) printf("0x%x = 0x%x\n", i, addr);
            if (param == 'l')
              for(i=0x0; i< 0x7000000; i+=4) if (addr == MappedMemoryReadLong(MSH2, i)) printf("0x%x = 0x%x\n", i, addr);
          break;
       }
     }
  }
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GL_TRUE);
  if ((k_call != NULL) && (inputMap[key] != -1))  {
    switch(action) {
    case GLFW_RELEASE:
      k_call(inputMap[key], 0);
    break;
    case GLFW_PRESS:
      k_call(inputMap[key], 1);
    break;
    case GLFW_REPEAT:
      k_call(inputMap[key], 0);
      k_call(inputMap[key], 1); 
    break;
    default:
    break;
    }
  }
  handleIntrospection(key, action);
}

void platform_getFBSize(int *w, int*h) {
   glfwGetFramebufferSize(g_window, w, h);
}

int platform_SetupOpenGL(int w, int h, int fullscreen) {
  int i;
  if (!glfwInit())
    return 0;

  glfwSetErrorCallback(error_callback);
#ifdef _OGLES3_
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API) ;
#else
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);  
#endif
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_RED_BITS,8);
  glfwWindowHint(GLFW_GREEN_BITS,8);
  glfwWindowHint(GLFW_BLUE_BITS,8);
  glfwWindowHint(GLFW_ALPHA_BITS,8);
  glfwWindowHint(GLFW_DEPTH_BITS,24);
  glfwWindowHint(GLFW_STENCIL_BITS,8);

  if (!fullscreen) {
    g_window = glfwCreateWindow(w, h, "Yabause", NULL, NULL);
  }
  else {
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    g_window = glfwCreateWindow(mode->width, mode->height, "Yabause", glfwGetPrimaryMonitor(), NULL);
  }

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

  for (i=0; i< 512; i++)
    inputMap[i] = -1;

   inputMap[GLFW_KEY_UP] = PERPAD_UP;
   inputMap[GLFW_KEY_RIGHT] = PERPAD_RIGHT;
   inputMap[GLFW_KEY_DOWN] = PERPAD_DOWN;
   inputMap[GLFW_KEY_LEFT] = PERPAD_LEFT;
   inputMap[GLFW_KEY_R] = PERPAD_RIGHT_TRIGGER;
   inputMap[GLFW_KEY_F] = PERPAD_LEFT_TRIGGER;
   inputMap[GLFW_KEY_SPACE] = PERPAD_START;
   inputMap[GLFW_KEY_Q] = PERPAD_A;
   inputMap[GLFW_KEY_S] = PERPAD_B;
   inputMap[GLFW_KEY_D] = PERPAD_C;
   inputMap[GLFW_KEY_A] = PERPAD_X;
   inputMap[GLFW_KEY_Z] = PERPAD_Y;
   inputMap[GLFW_KEY_E] = PERPAD_Z;

   inputMap[GLFW_KEY_G] = PERJAMMA_SERVICE;
   inputMap[GLFW_KEY_T] = PERJAMMA_TEST;

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

void platform_SetKeyCallback(k_callback call) {
  k_call = call;
}

