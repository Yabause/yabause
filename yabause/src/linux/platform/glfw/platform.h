#ifndef PLATFORM_GLFW_H
#define PLATFORM_GLFW_H

#ifdef PLATFORM_LINUX
#error "A platform has already been set"
#else
#define PLATFORM_LINUX
#endif

typedef void (*k_callback)(unsigned int key, unsigned char state);

extern int platform_SetupOpenGL(int w, int h, int fullscreen);
extern int platform_YuiRevokeOGLOnThisThread();
extern int platform_YuiUseOGLOnThisThread();
extern void platform_swapBuffers(void);
extern int platform_shouldClose();
extern void platform_Close();
extern int platform_Deinit(void);
extern void platform_HandleEvent();
extern void platform_SetKeyCallback(k_callback call);
extern void platform_getFBSize(int *w, int*h);

#endif
