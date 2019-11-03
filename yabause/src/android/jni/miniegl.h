/*
        Copyright 2019 devMiyax(smiyaxdev@gmail.com)

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

#ifndef _MINIEGL_
#define _MINIEGL_

typedef int EGLint;
typedef unsigned int EGLBoolean;
typedef unsigned int EGLenum;
typedef void *EGLConfig;
typedef void *EGLContext;
typedef void *EGLDisplay;
typedef void *EGLSurface;
typedef void *EGLClientBuffer;

/* EGL aliases */
#define EGL_FALSE           0
#define EGL_TRUE            1

/* Out-of-band handle values */
#define EGL_DEFAULT_DISPLAY     ((EGLNativeDisplayType)0)
#define EGL_NO_CONTEXT          ((EGLContext)0)
#define EGL_NO_DISPLAY          ((EGLDisplay)0)
#define EGL_NO_SURFACE          ((EGLSurface)0)

/* GetCurrentSurface targets */
#define EGL_DRAW            0x3059
#define EGL_READ            0x305A

/* QuerySurface / SurfaceAttrib / CreatePbufferSurface targets */
#define EGL_HEIGHT          0x3056
#define EGL_WIDTH           0x3057

/* QueryString targets */
#define EGL_VENDOR          0x3053
#define EGL_VERSION         0x3054
#define EGL_EXTENSIONS          0x3055
#define EGL_CLIENT_APIS         0x308D

EGLContext (*eglGetCurrentContext)(void);
EGLSurface (*eglGetCurrentSurface)(EGLint readdraw);
EGLDisplay (*eglGetCurrentDisplay)(void);
EGLBoolean (*eglQuerySurface)(EGLDisplay dpy, EGLSurface surface,EGLint attribute, EGLint *value);
EGLBoolean (*eglSwapInterval)(EGLDisplay dpy, EGLint interval);
EGLBoolean (*eglMakeCurrent)(EGLDisplay dpy, EGLSurface draw,EGLSurface read, EGLContext ctx);
EGLBoolean (*eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
const char * (*eglQueryString)(EGLDisplay dpy, EGLint name);
EGLint (*eglGetError)(void);


#endif // _MINIEGL_