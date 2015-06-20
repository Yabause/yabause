/*  Copyright 2011 Guillaume Duhamel

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include <jni.h>
#include <android/native_window.h> // requires ndk r5 or newer
#include <android/native_window_jni.h> // requires ndk r5 or newer
#include <android/bitmap.h>
#include <android/log.h>

#include "../../config.h"
#include "yabause.h"
#include "scsp.h"
#include "vidsoft.h"
#include "vidogl.h"
#include "peripheral.h"
#include "m68kcore.h"
#include "sh2core.h"
#include "sh2int.h"
#include "cdbase.h"
#include "cs2.h"
#include "debug.h"
#include "osdcore.h"

#include <stdio.h>
#include <dlfcn.h>

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <pthread.h>

#include "sndaudiotrack.h"
#include "sndopensl.h"

JavaVM * yvm;
static jobject yabause;

static char mpegpath[256] = "\0";
static char cartpath[256] = "\0";

EGLDisplay g_Display = EGL_NO_DISPLAY;
EGLSurface g_Surface = EGL_NO_SURFACE;
EGLContext g_Context = EGL_NO_CONTEXT;
ANativeWindow *g_window = 0;
GLuint g_FrameBuffer = 0;
GLuint g_VertexBuffer = 0;
GLuint programObject  = 0;
GLuint positionLoc    = 0;
GLuint texCoordLoc    = 0;
GLuint samplerLoc     = 0;
int g_buf_width = -1;
int g_buf_height = -1;
pthread_mutex_t g_mtxGlLock = PTHREAD_MUTEX_INITIALIZER;
float vertices [] = {
   -1.0f, 1.0f, 0, 0,
   1.0f, 1.0f, 0, 0,
   1.0f, -1.0f, 0, 0,
   -1.0f,-1.0f, 0, 0
};

// Setting Infomation From
const char * s_biospath = NULL;
const char * s_cdpath = NULL;
const char * s_buppath = NULL;
const char * s_cartpath = NULL;
int s_vidcoretype = VIDCORE_OGL;

enum RenderThreadMessage {
        MSG_NONE = 0,
        MSG_WINDOW_SET,
        MSG_RENDER_LOOP_EXIT
};

int g_msg = MSG_NONE;
pthread_t _threadId;

M68K_struct * M68KCoreList[] = {
&M68KDummy,
#ifdef HAVE_C68K
&M68KC68K,
#endif
#ifdef HAVE_Q68
&M68KQ68,
#endif
NULL
};

SH2Interface_struct *SH2CoreList[] = {
&SH2Interpreter,
&SH2DebugInterpreter,
#ifdef SH2_DYNAREC
&SH2Dynarec,
#endif
NULL
};

PerInterface_struct *PERCoreList[] = {
&PERDummy,
NULL
};

CDInterface *CDCoreList[] = {
&DummyCD,
&ISOCD,
NULL
};

SoundInterface_struct *SNDCoreList[] = {
&SNDDummy,
&SNDOpenSL,
&SNDAudioTrack,
NULL
};

VideoInterface_struct *VIDCoreList[] = {
&VIDDummy,
&VIDSoft,
&VIDOGL,
NULL
};

void YuidrawSoftwareBuffer() ;

#define  LOG_TAG    "yabause"

/* Override printf for debug*/
int yprintf( const char * fmt, ... )
{
   va_list ap;
   va_start(ap, fmt);
   int result = __android_log_vprint(ANDROID_LOG_INFO, LOG_TAG, fmt, ap);
   va_end(ap);
   return result;
}

const char * GetBiosPath()
{
    jclass yclass;
    jmethodID getBiosPath;
    jstring message;
    jboolean dummy;
    JNIEnv * env;
    if ((*yvm)->GetEnv(yvm, (void**) &env, JNI_VERSION_1_6) != JNI_OK){
      return NULL;
    }

    yclass = (*env)->GetObjectClass(env, yabause);
    getBiosPath = (*env)->GetMethodID(env, yclass, "getBiosPath", "()Ljava/lang/String;");
    message = (*env)->CallObjectMethod(env, yabause, getBiosPath);
    if ((*env)->GetStringLength(env, message) == 0)
        return NULL;
    else
        return (*env)->GetStringUTFChars(env, message, &dummy);
}

const char * GetGamePath()
{
    jclass yclass;
    jmethodID getGamePath;
    jstring message;
    jboolean dummy;
    JNIEnv * env;
    if ((*yvm)->GetEnv(yvm, (void**) &env, JNI_VERSION_1_6) != JNI_OK){
        return NULL;
    }

    yclass = (*env)->GetObjectClass(env, yabause);
    getGamePath = (*env)->GetMethodID(env, yclass, "getGamePath", "()Ljava/lang/String;");
    message = (*env)->CallObjectMethod(env, yabause, getGamePath);
    if ((*env)->GetStringLength(env, message) == 0)
        return NULL;
    else
        return (*env)->GetStringUTFChars(env, message, &dummy);
}

const char * GetMemoryPath()
{
    jclass yclass;
    jmethodID getMemoryPath;
    jstring message;
    jboolean dummy;
    JNIEnv * env;
    if ((*yvm)->GetEnv(yvm, (void**) &env, JNI_VERSION_1_6) != JNI_OK){
        return NULL;
    }

    yclass = (*env)->GetObjectClass(env, yabause);
    getMemoryPath = (*env)->GetMethodID(env, yclass, "getMemoryPath", "()Ljava/lang/String;");
    message = (*env)->CallObjectMethod(env, yabause, getMemoryPath);
    if ((*env)->GetStringLength(env, message) == 0)
        return NULL;
    else
        return (*env)->GetStringUTFChars(env, message, &dummy);
}

int GetCartridgeType()
{
    jclass yclass;
    jmethodID getCartridgeType;
    JNIEnv * env;
    if ((*yvm)->GetEnv(yvm, (void**) &env, JNI_VERSION_1_6) != JNI_OK){
        return -1;
    }

    yclass = (*env)->GetObjectClass(env, yabause);
    getCartridgeType = (*env)->GetMethodID(env, yclass, "getCartridgePath", "()I");
    return (*env)->CallIntMethod(env, yabause, getCartridgeType);
}

int GetVideoInterface()
{
    jclass yclass;
    jmethodID getVideoInterface;
    JNIEnv * env;
    if ((*yvm)->GetEnv(yvm, (void**) &env, JNI_VERSION_1_6) != JNI_OK){
        return -1;
    }

    yclass = (*env)->GetObjectClass(env, yabause);
    getVideoInterface = (*env)->GetMethodID(env, yclass, "getVideoInterface", "()I");
    return (*env)->CallIntMethod(env, yabause, getVideoInterface);
}

const char * GetCartridgePath()
{
    jclass yclass;
    jmethodID getCartridgePath;
    jstring message;
    jboolean dummy;
    JNIEnv * env;
    if ((*yvm)->GetEnv(yvm, (void**) &env, JNI_VERSION_1_6) != JNI_OK){
        return NULL;
    }

    yclass = (*env)->GetObjectClass(env, yabause);
    getCartridgePath = (*env)->GetMethodID(env, yclass, "getCartridgePath", "()Ljava/lang/String;");
    message = (*env)->CallObjectMethod(env, yabause, getCartridgePath);
    if ((*env)->GetStringLength(env, message) == 0)
        return NULL;
    else
        return (*env)->GetStringUTFChars(env, message, &dummy);
}

void YuiErrorMsg(const char *string)
{
    jclass yclass;
    jmethodID errorMsg;
    jstring message;
    JNIEnv * env;
    if ((*yvm)->GetEnv(yvm, (void**) &env, JNI_VERSION_1_6) != JNI_OK) {
        return;
    }

    yclass = (*env)->GetObjectClass(env, yabause);
    errorMsg = (*env)->GetMethodID(env, yclass, "errorMsg", "(Ljava/lang/String;)V");
    message = (*env)->NewStringUTF(env, string);
    (*env)->CallVoidMethod(env, yabause, errorMsg, message);
}

void* threadStartCallback(void *myself);

void YuiSwapBuffers(void)
{
   if( g_Display == EGL_NO_DISPLAY ){
      return;
   }

   OSDDisplayMessages();
   if( s_vidcoretype == VIDCORE_SOFT ){
       YuidrawSoftwareBuffer();
   }
   eglSwapBuffers(g_Display,g_Surface);
}

GLuint LoadShader ( GLenum type, const char *shaderSrc )
{
   GLuint shader;
   GLint compiled;

   // Create the shader object
   shader = glCreateShader ( type );

   if ( shader == 0 )
    return 0;

   // Load the shader source
   glShaderSource ( shader, 1, &shaderSrc, NULL );

   // Compile the shader
   glCompileShader ( shader );

   // Check the compile status
   glGetShaderiv ( shader, GL_COMPILE_STATUS, &compiled );

   if ( !compiled )
   {
      GLint infoLen = 0;

      glGetShaderiv ( shader, GL_INFO_LOG_LENGTH, &infoLen );

      if ( infoLen > 1 )
      {
         char* infoLog = malloc (sizeof(char) * infoLen );
         glGetShaderInfoLog ( shader, infoLen, NULL, infoLog );
         yprintf ( "Error compiling shader:\n%s\n", infoLog );
         free ( infoLog );
      }

      glDeleteShader ( shader );
      return 0;
   }

   return shader;

}

int YuiInitProgramForSoftwareRendering()
{
   GLbyte vShaderStr[] =
      "attribute vec4 a_position;   \n"
      "attribute vec2 a_texCoord;   \n"
      "varying vec2 v_texCoord;     \n"
      "void main()                  \n"
      "{                            \n"
      "   gl_Position = a_position; \n"
      "   v_texCoord = a_texCoord;  \n"
      "}                            \n";

   GLbyte fShaderStr[] =
      "precision mediump float;                            \n"
      "varying vec2 v_texCoord;                            \n"
      "uniform sampler2D s_texture;                        \n"
      "void main()                                         \n"
      "{                                                   \n"
      "  gl_FragColor = texture2D( s_texture, v_texCoord );\n"
      "}                                                   \n";

   GLuint vertexShader;
   GLuint fragmentShader;
   GLint linked;

   // Load the vertex/fragment shaders
   vertexShader = LoadShader ( GL_VERTEX_SHADER, vShaderStr );
   fragmentShader = LoadShader ( GL_FRAGMENT_SHADER, fShaderStr );

   // Create the program object
   programObject = glCreateProgram ( );

   if ( programObject == 0 )
      return 0;

   glAttachShader ( programObject, vertexShader );
   glAttachShader ( programObject, fragmentShader );

   // Link the program
   glLinkProgram ( programObject );

   // Check the link status
   glGetProgramiv ( programObject, GL_LINK_STATUS, &linked );

   if ( !linked )
   {
      GLint infoLen = 0;

      glGetProgramiv ( programObject, GL_INFO_LOG_LENGTH, &infoLen );

      if ( infoLen > 1 )
      {
        char* infoLog = malloc (sizeof(char) * infoLen );
        glGetProgramInfoLog ( programObject, infoLen, NULL, infoLog );
        yprintf ( "Error linking program:\n%s\n", infoLog );
        free ( infoLog );
         return;
      }

      glDeleteProgram ( programObject );
      return GL_FALSE;
   }


   // Get the attribute locations
   positionLoc = glGetAttribLocation ( programObject, "a_position" );
   texCoordLoc = glGetAttribLocation ( programObject, "a_texCoord" );

   // Get the sampler location
   samplerLoc = glGetUniformLocation ( programObject, "s_texture" );

   glUseProgram(programObject);


   return GL_TRUE;
}

void YuidrawSoftwareBuffer() {

    int buf_width, buf_height;
    int error;

    glClearColor( 0.0f,0.0f,0.0f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if( g_FrameBuffer == 0 )
    {
       glGenTextures(1,&g_FrameBuffer);
       glActiveTexture ( GL_TEXTURE0 );
       glBindTexture(GL_TEXTURE_2D, g_FrameBuffer);
       glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
       glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
       glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
       glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
       glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
       error = glGetError();
       if( error != GL_NO_ERROR )
       {
          printf("g_FrameBuffer gl error %04X", error );
          return;
       }
    }else{
       glBindTexture(GL_TEXTURE_2D, g_FrameBuffer);
    }

    VIDCore->GetGlSize(&buf_width, &buf_height);
    glTexSubImage2D(GL_TEXTURE_2D, 0,0,0,buf_width,buf_height,GL_RGBA,GL_UNSIGNED_BYTE,dispbuffer);


    if( g_VertexBuffer == 0 )
    {
       glGenBuffers(1, &g_VertexBuffer);
       glBindBuffer(GL_ARRAY_BUFFER, g_VertexBuffer);
       glBufferData(GL_ARRAY_BUFFER, sizeof(vertices),vertices,GL_STATIC_DRAW);
       error = glGetError();
       if( error != GL_NO_ERROR )
       {
          printf("g_VertexBuffer gl error %04X", error );
          return;
       }
    }else{
       glBindBuffer(GL_ARRAY_BUFFER, g_VertexBuffer);
    }

   if( buf_width != g_buf_width ||  buf_height != g_buf_height )
   {
      vertices[6]=vertices[10]=(float)buf_width/1024.f;
      vertices[11]=vertices[15]=(float)buf_height/1024.f;
      glBufferData(GL_ARRAY_BUFFER, sizeof(vertices),vertices,GL_STATIC_DRAW);
      glVertexAttribPointer ( positionLoc, 2, GL_FLOAT,  GL_FALSE, 4 * sizeof(GLfloat), 0 );
      glVertexAttribPointer ( texCoordLoc, 2, GL_FLOAT,  GL_FALSE, 4 * sizeof(GLfloat), (void*)(sizeof(float)*2) );
      glEnableVertexAttribArray ( positionLoc );
      glEnableVertexAttribArray ( texCoordLoc );
      g_buf_width  = buf_width;
      g_buf_height = buf_height;
      error = glGetError();
      if( error != GL_NO_ERROR )
      {
         yprintf("gl error %d", error );
         return;
      }
   }else{
      glBindBuffer(GL_ARRAY_BUFFER, g_VertexBuffer);
   }

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}


JNIEXPORT int JNICALL Java_org_uoyabause_android_YabauseRunnable_initViewport( JNIEnv* jenv, jobject obj, jobject surface, int width, int height)
{
    if (surface != 0) {
        g_window = ANativeWindow_fromSurface(jenv, surface);
        printf("Got window %p", g_window);
    } else {
        printf("Releasing window");
        ANativeWindow_release(g_window);
    }
    g_msg = MSG_WINDOW_SET;
   return 0;
}

JNIEXPORT int JNICALL Java_org_uoyabause_android_YabauseRunnable_lockGL()
{
   pthread_mutex_lock(&g_mtxGlLock);
}

JNIEXPORT int JNICALL Java_org_uoyabause_android_YabauseRunnable_unlockGL()
{
   pthread_mutex_unlock(&g_mtxGlLock);
}

JNIEXPORT int JNICALL Java_org_uoyabause_android_YabauseRunnable_toggleShowFps( JNIEnv* env )
{
    printf("%s","Java_org_uoyabause_android_YabauseRunnable_toggleShowFps");
   ToggleFPS();
}

static int enableautofskip = 0;
JNIEXPORT int JNICALL Java_org_uoyabause_android_YabauseRunnable_toggleFrameSkip( JNIEnv* env )
{
    enableautofskip = 1 - enableautofskip;

     printf("%s:%d","Java_org_uoyabause_android_YabauseRunnable_toggleFrameSkip",enableautofskip);

    if (enableautofskip)
       EnableAutoFrameSkip();
    else
       DisableAutoFrameSkip();
}

#ifdef _ANDROID_2_2_
int initEGLFunc()
{
   void * handle;
   char *error;

   handle = dlopen("libEGL.so",RTLD_LAZY);
   if( handle == NULL )
   {
      yprintf(dlerror());
      return -1;
   }

   eglGetCurrentDisplay = dlsym(handle, "eglGetCurrentDisplay");
   if( eglGetCurrentDisplay == NULL){ yprintf(dlerror()); return -1; }

   eglGetCurrentSurface = dlsym(handle, "eglGetCurrentSurface");
   if( eglGetCurrentSurface == NULL){ yprintf(dlerror()); return -1; }

   eglGetCurrentContext = dlsym(handle, "eglGetCurrentContext");
   if( eglGetCurrentContext == NULL){ yprintf(dlerror()); return -1; }

   eglQuerySurface      = dlsym(handle, "eglQuerySurface");
   if( eglQuerySurface == NULL){ yprintf(dlerror()); return -1; }

   eglSwapInterval      = dlsym(handle, "eglSwapInterval");
   if( eglSwapInterval == NULL){ yprintf(dlerror()); return -1; }

   eglMakeCurrent       = dlsym(handle, "eglMakeCurrent");
   if( eglMakeCurrent == NULL){ yprintf(dlerror()); return -1; }

   eglSwapBuffers       = dlsym(handle, "eglSwapBuffers");
   if( eglSwapBuffers == NULL){ yprintf(dlerror()); return -1; }

   eglQueryString       = dlsym(handle, "eglQueryString");
   if( eglQueryString == NULL){ yprintf(dlerror()); return -1; }

   eglGetError          = dlsym(handle, "eglGetError");
   if( eglGetError == NULL){ yprintf(dlerror()); return -1; }

   return 0;
}
#else
int initEGLFunc()
{
   return 0;
}
#endif



jint Java_org_uoyabause_android_YabauseRunnable_init( JNIEnv* env, jobject obj, jobject yab )
{
    int res=0;

    if( initEGLFunc() == -1 ) return -1;

    yabause = (*env)->NewGlobalRef(env, yab);

    s_biospath = GetBiosPath();
    s_cdpath = GetGamePath();
    s_buppath = GetMemoryPath();
    s_cartpath = GetCartridgePath();
    s_vidcoretype = GetVideoInterface();

    pthread_create(&_threadId, 0, threadStartCallback, NULL );

    return res;
}

int initEgl( ANativeWindow* window )
{
    int res;
    yabauseinit_struct yinit;
    void * padbits;

     const EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE,24,
        EGL_STENCIL_SIZE,8,
        EGL_NONE
    };
    EGLDisplay display;
    EGLConfig config;
    EGLint numConfigs;
    EGLint format;
    EGLSurface surface;
    EGLContext context;
    EGLint width;
    EGLint height;
    int tmp;
    GLfloat ratio;
    int attrib_list[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };

    if( s_vidcoretype == VIDCORE_SOFT ) {
        attrib_list[1]=2;
    }


    printf("Initializing context");

    if ((display = eglGetDisplay(EGL_DEFAULT_DISPLAY)) == EGL_NO_DISPLAY) {
        printf("eglGetDisplay() returned error %d", eglGetError());
        return -1;
    }
    if (!eglInitialize(display, 0, 0)) {
        printf("eglInitialize() returned error %d", eglGetError());
        return -1;
    }

    if (!eglChooseConfig(display, attribs, &config, 1, &numConfigs)) {
        printf("eglChooseConfig() returned error %d", eglGetError());
        destroy();
        return -1;
    }

    if (!eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format)) {
        printf("eglGetConfigAttrib() returned error %d", eglGetError());
        destroy();
        return -1;
    }
    printf("ANativeWindow_setBuffersGeometry");
    ANativeWindow_setBuffersGeometry(window, 0, 0, format);

    printf("eglCreateWindowSurface");
    if (!(surface = eglCreateWindowSurface(display, config, window, 0))) {
        printf("eglCreateWindowSurface() returned error %d", eglGetError());
        destroy();
        return -1;
    }

    printf("eglCreateContext");
    if (!(context = eglCreateContext(display, config, 0, attrib_list))) {
        printf("eglCreateContext() returned error %d, Fall back to software vidcore mode", eglGetError());
        s_vidcoretype = VIDCORE_SOFT;
        attrib_list[1]=2;
        if (!(context = eglCreateContext(display, config, 0, attrib_list))) {
            printf("eglCreateContext() returned error %d", eglGetError());
            destroy();
            return -1;
        }
    }

    printf("eglMakeCurrent");
    if (!eglMakeCurrent(display, surface, surface, context)) {
        printf("eglMakeCurrent() returned error %d", eglGetError());
        destroy();
        return -1;
    }
   glClearColor( 0.0f, 0.0f,0.0f,1.0f);
    if (!eglQuerySurface(display, surface, EGL_WIDTH, &width) ||
        !eglQuerySurface(display, surface, EGL_HEIGHT, &height)) {
        printf("eglQuerySurface() returned error %d", eglGetError());
        destroy();
        return -1;
    }


    g_Display = display;
    g_Surface = surface;
    g_Context = context;

    printf("%s",glGetString(GL_VENDOR));
    printf("%s",glGetString(GL_RENDERER));
    printf("%s",glGetString(GL_VERSION));
    printf("%s",glGetString(GL_EXTENSIONS));
    printf("%s",eglQueryString(g_Display,EGL_EXTENSIONS));


    yinit.m68kcoretype = M68KCORE_C68K;
    yinit.percoretype = PERCORE_DUMMY;
#ifdef SH2_DYNAREC
    yinit.sh2coretype = 2;
#else
    yinit.sh2coretype = SH2CORE_DEFAULT;
#endif
    yinit.vidcoretype = s_vidcoretype;
    yinit.sndcoretype = SNDCORE_OPENSL;
    yinit.cdcoretype = CDCORE_ISO;
    yinit.carttype = CART_NONE;
    yinit.regionid = 0;

    yinit.biospath = s_biospath;
    yinit.cdpath = s_cdpath;
    yinit.buppath = s_buppath;
    yinit.cartpath = s_cartpath;

    yinit.mpegpath = mpegpath;
    yinit.videoformattype = VIDEOFORMATTYPE_NTSC;
    yinit.frameskip = 0;
    res = YabauseInit(&yinit);
    if (res != 0) {
      printf("Fail to YabauseInit %d", res);
      return -1;
    }

    PerPortReset();
    padbits = PerPadAdd(&PORTDATA1);
    PerSetKey(PERPAD_UP, PERPAD_UP, padbits);
    PerSetKey(PERPAD_RIGHT, PERPAD_RIGHT, padbits);
    PerSetKey(PERPAD_DOWN, PERPAD_DOWN, padbits);
    PerSetKey(PERPAD_LEFT, PERPAD_LEFT, padbits);
    PerSetKey(PERPAD_START, PERPAD_START, padbits);
    PerSetKey(PERPAD_A, PERPAD_A, padbits);
    PerSetKey(PERPAD_B, PERPAD_B, padbits);
    PerSetKey(PERPAD_C, PERPAD_C, padbits);
    PerSetKey(PERPAD_X, PERPAD_X, padbits);
    PerSetKey(PERPAD_Y, PERPAD_Y, padbits);
    PerSetKey(PERPAD_Z, PERPAD_Z, padbits);
    PerSetKey(PERPAD_RIGHT_TRIGGER,PERPAD_RIGHT_TRIGGER,padbits);
    PerSetKey(PERPAD_LEFT_TRIGGER,PERPAD_LEFT_TRIGGER,padbits);

    ScspSetFrameAccurate(1);

    if( s_vidcoretype == VIDCORE_OGL ){
        OSDChangeCore(OSDCORE_NANOVG);
        VIDCore->Resize(width,height,0);
    }else{
        OSDChangeCore(OSDCORE_SOFT);
        if( YuiInitProgramForSoftwareRendering() != GL_TRUE ){
            return -1;
        }
    }
    glViewport(0,0,width,height);

    glClearColor( 0.0f, 0.0f,0.0f,1.0f);
    glClear( GL_COLOR_BUFFER_BIT );
    return 0;
}

destroy() {
    printf("Destroying context");

    eglMakeCurrent(g_Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(g_Display, g_Context);
    eglDestroySurface(g_Display, g_Surface);
    eglTerminate(g_Display);

    g_Display = EGL_NO_DISPLAY;
    g_Surface = EGL_NO_SURFACE;
    g_Context = EGL_NO_CONTEXT;
    return;
}

void
Java_org_uoyabause_android_YabauseRunnable_deinit( JNIEnv* env )
{
    YabauseDeInit();
}

void
Java_org_uoyabause_android_YabauseRunnable_exec( JNIEnv* env )
{
 //   YabauseExec();
}

void
Java_org_uoyabause_android_YabauseRunnable_press( JNIEnv* env, jobject obj, jint key )
{
    PerKeyDown(key);
}

void
Java_org_uoyabause_android_YabauseRunnable_release( JNIEnv* env, jobject obj, jint key )
{
    PerKeyUp(key);
}

void
Java_org_uoyabause_android_YabauseRunnable_enableFPS( JNIEnv* env, jobject obj, jint enable )
{
    SetOSDToggle(enable);
}

void
Java_org_uoyabause_android_YabauseRunnable_enableFrameskip( JNIEnv* env, jobject obj, jint enable )
{
    if (enable)
        EnableAutoFrameSkip();
    else
        DisableAutoFrameSkip();
}

void
Java_org_uoyabause_android_YabauseRunnable_setVolume( JNIEnv* env, jobject obj, jint volume )
{
    if (0 == volume)
       ScspMuteAudio(SCSP_MUTE_USER);
    else {
       ScspUnMuteAudio(SCSP_MUTE_USER);
       ScspSetVolume(volume);
    }
}

void
Java_org_uoyabause_android_YabauseRunnable_screenshot( JNIEnv* env, jobject obj, jobject bitmap )
{
    u32 * buffer;
    AndroidBitmapInfo info;
    void * pixels;
    int x, y;

    AndroidBitmap_getInfo(env, bitmap, &info);

    AndroidBitmap_lockPixels(env, bitmap, &pixels);

    buffer = dispbuffer;

    for(y = 0;y < info.height;y++) {
        for(x = 0;x < info.width;x++) {
            *((uint32_t *) pixels + x) = *(buffer + x);
        }
        pixels += info.stride;
        buffer += g_buf_width;
    }

    AndroidBitmap_unlockPixels(env, bitmap);
}

void log_callback(char * message)
{
    __android_log_print(ANDROID_LOG_INFO, "yabause", "%s", message);
}

jint JNI_OnLoad(JavaVM * vm, void * reserved)
{
    JNIEnv * env;
    if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_6) != JNI_OK)
        return -1;

    yvm = vm;

    LogStart();
    LogChangeOutput(DEBUG_CALLBACK, (char *) log_callback);

    return JNI_VERSION_1_6;
}


void renderLoop()
{
    int renderingEnabled = 1;

    printf("enter render loop!");

    while (renderingEnabled != 0) {

        pthread_mutex_lock(&g_mtxGlLock);

        // process incoming messages
        switch (g_msg) {

            case MSG_WINDOW_SET:
                if( initEgl( g_window ) != 0 ){
                  pthread_mutex_unlock(&g_mtxGlLock);
                  return;
                }
                break;

            case MSG_RENDER_LOOP_EXIT:
                renderingEnabled = 0;
                destroy();
                break;

            default:
                break;
        }
        g_msg = MSG_NONE;

        if (g_Display) {
           YabauseExec();
        }
        pthread_mutex_unlock(&g_mtxGlLock);
    }
}

void* threadStartCallback(void *myself)
{
    renderLoop();
    pthread_exit(0);
    return NULL;
}
