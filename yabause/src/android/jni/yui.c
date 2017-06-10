/*  Copyright 2011 Guillaume Duhamel
    Copyright 2015 devMiyax

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

#include "config.h"
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
#ifdef HAVE_OPENSL
#include "sndopensl.h"
#endif

#include "libpng/png.h"

JavaVM * yvm;
static jobject yabause;

static char mpegpath[256] = "\0";
static char cartpath[256] = "\0";
static char screenShotFilename[256] = "\0";
static char last_state_filename[256] = "\0";

EGLDisplay g_Display = EGL_NO_DISPLAY;
EGLSurface g_Surface = EGL_NO_SURFACE;
EGLContext g_Context = EGL_NO_CONTEXT;
EGLContext g_Context_Sub = EGL_NO_CONTEXT;
EGLSurface g_Pbuffer = EGL_NO_SURFACE;
EGLConfig  g_Config = 0;
ANativeWindow *g_window = 0;
GLuint g_FrameBuffer = 0;
GLuint g_VertexBuffer = 0;
GLuint programObject  = 0;
GLuint positionLoc    = 0;
GLuint texCoordLoc    = 0;
GLuint samplerLoc     = 0;
int g_buf_width = -1;
int g_buf_height = -1;
int g_major_version=0;
int g_minor_version=0;
int g_minorminor_version=0;
int g_pad_mode = -1;
int g_EnagleFPS = 0;
int g_CpuType = 2;
int g_VideoFilter = 0;
int g_PolygonGenerationMode = 0;
static int g_SoundEngine = 0;
static int g_resolution_mode = 0;

static int s_status = 0;
pthread_mutex_t g_mtxGlLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t g_mtxFuncSync = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t g_cndFuncSync = PTHREAD_COND_INITIALIZER;


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
int s_carttype;
char s_savepath[256] ="\0";
int s_vidcoretype = VIDCORE_OGL;
int s_player2Enable = -1;

#define MAKE_PAD(a,b) ((a<<24)|(b))
void update_pad_mode();

enum RenderThreadMessage {
        MSG_NONE = 0,
        MSG_WINDOW_SET,
        MSG_WINDOW_CHG,
        MSG_RENDER_LOOP_EXIT,
        MSG_SAVE_STATE,
        MSG_LOAD_STATE,
        MSG_PAUSE,
        MSG_RESUME,
        MSG_SCREENSHOT,
        MSG_OPEN_TRAY,
        MSG_CLOSE_TRAY,
        MSG_RESET

};

volatile int g_msg = MSG_NONE;
pthread_t _threadId;

M68K_struct * M68KCoreList[] = {
&M68KDummy,
#ifdef HAVE_C68K
&M68KC68K,
#endif
#ifdef HAVE_Q68
&M68KQ68,
#endif
#ifdef HAVE_MUSASHI
&M68KMusashi,
#endif
NULL
};

SH2Interface_struct *SH2CoreList[] = {
&SH2Interpreter,
&SH2DebugInterpreter,
#ifdef SH2_DYNAREC
&SH2Dynarec,
#endif
#ifdef DYNAREC_DEVMIYAX
&SH2Dyn,
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
&SNDAudioTrack,
#ifdef HAVE_OPENSL
&SNDOpenSL,
#endif
NULL
};

VideoInterface_struct *VIDCoreList[] = {
&VIDDummy,
&VIDSoft,
&VIDOGL,
NULL
};

#ifdef YAB_PORT_OSD
#include "nanovg/nanovg_osdcore.h"
OSD_struct *OSDCoreList[] = {
&OSDNnovg,
NULL
};  
#endif

void YuidrawSoftwareBuffer();
static int saveScreenshot( const char * filename );


#define  LOG_TAG    "yabause"

/* Override printf for debug*/
int yprintf( const char * fmt, ... )
{
    int result = 0;
   va_list ap;
   va_start(ap, fmt);
   result = __android_log_vprint(ANDROID_LOG_INFO, LOG_TAG, fmt, ap);
   va_end(ap);
   return result;
}

int printf( const char * fmt, ... )
{
    int result = 0;
   va_list ap;
   va_start(ap, fmt);
   result = __android_log_vprint(ANDROID_LOG_INFO, LOG_TAG, fmt, ap);
   va_end(ap);
   return result;
}


#define YUI_LOG yprintf
//#define YUI_LOG

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
    getCartridgeType = (*env)->GetMethodID(env, yclass, "getCartridgeType", "()I");
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

int GetPlayer2Device(){
	
    jclass yclass;
    jmethodID getPlayer2InputDevice;
    JNIEnv * env;
    if ((*yvm)->GetEnv(yvm, (void**) &env, JNI_VERSION_1_6) != JNI_OK){
        return -1;
    }

    yclass = (*env)->GetObjectClass(env, yabause);
    getPlayer2InputDevice = (*env)->GetMethodID(env, yclass, "getPlayer2InputDevice", "()I");
    return (*env)->CallIntMethod(env, yabause, getPlayer2InputDevice);
}

void YuiErrorMsg(const char *string)
{
	//YUI_LOG("YuiErrorMsg %s",string);
	
    jclass yclass;
    jmethodID errorMsg;
    jstring message;
    JNIEnv * env;

	(*yvm)->AttachCurrentThread(yvm, &env, NULL);	

	YUI_LOG("YuiErrorMsg2 %s",string);
	
    yclass = (*env)->GetObjectClass(env, yabause);
    errorMsg = (*env)->GetMethodID(env, yclass, "errorMsg", "(Ljava/lang/String;)V");
    message = (*env)->NewStringUTF(env, string);
    (*env)->CallVoidMethod(env, yabause, errorMsg, message);
	(*yvm)->DetachCurrentThread(yvm);
}

void* threadStartCallback(void *myself);

void YuiSwapBuffers(void)
{
   if( g_Display == EGL_NO_DISPLAY ){
      return;
   }
   if( s_vidcoretype == VIDCORE_SOFT ){
       YuidrawSoftwareBuffer();
   }
   eglSwapBuffers(g_Display,g_Surface);
   SetOSDToggle(g_EnagleFPS);   
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
         YUI_LOG ( "Error compiling shader:\n%s\n", infoLog );
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
        YUI_LOG ( "Error linking program:\n%s\n", infoLog );
        free ( infoLog );
         return GL_FALSE;
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
          YUI_LOG("g_FrameBuffer gl error %04X", error );
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
          YUI_LOG("g_VertexBuffer gl error %04X", error );
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
         YUI_LOG("gl error %d", error );
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
        
        if( g_window == 0 ){
            g_window = ANativeWindow_fromSurface(jenv, surface);
            YUI_LOG("Got window %08X %d,%d", g_window, g_msg,width,height );
            g_msg = MSG_WINDOW_SET;         
        }else{
            //if( g_window != ANativeWindow_fromSurface(jenv, surface) ){
                g_window = ANativeWindow_fromSurface(jenv, surface);
                YUI_LOG("Chg window %08X %d,%d ", g_window, g_msg,width,height);
                g_msg = MSG_WINDOW_CHG;
            //}
            //YUI_LOG("Got window ignore %p %d,%d", g_window, g_msg,width,height );
        }
    } else {
        YUI_LOG("Releasing window");
        ANativeWindow_release(g_window);
    }
    
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


JNIEXPORT int JNICALL Java_org_uoyabause_android_YabauseRunnable_pause( JNIEnv* env )
{
    pthread_mutex_lock(&g_mtxGlLock);
    g_msg = MSG_PAUSE;
    pthread_mutex_unlock(&g_mtxGlLock);
}

JNIEXPORT int JNICALL Java_org_uoyabause_android_YabauseRunnable_resume( JNIEnv* env )
{
    pthread_mutex_lock(&g_mtxGlLock);
    g_msg = MSG_RESUME;
    pthread_mutex_unlock(&g_mtxGlLock);
}

JNIEXPORT void JNICALL Java_org_uoyabause_android_YabauseRunnable_openTray( JNIEnv* env )
{
    yprintf("sending MSG_OPEN_TRAY");
    pthread_mutex_lock(&g_mtxGlLock);
    g_msg = MSG_OPEN_TRAY;
    pthread_mutex_unlock(&g_mtxGlLock);
}

JNIEXPORT void JNICALL Java_org_uoyabause_android_YabauseRunnable_closeTray( JNIEnv* env )
{
    yprintf("sending MSG_CLOSE_TRAY");
    s_cdpath = GetGamePath();    
    pthread_mutex_lock(&g_mtxGlLock);
    yprintf("new cd is %s",s_cdpath);    
    g_msg = MSG_CLOSE_TRAY;
    pthread_mutex_unlock(&g_mtxGlLock);
}


JNIEXPORT jstring  JNICALL Java_org_uoyabause_android_YabauseRunnable_getCurrentGameCode( JNIEnv* env)
{
    return(*env)->NewStringUTF(env,(const char*)Cs2GetCurrentGmaecode());
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
      YUI_LOG(dlerror());
      return -1;
   }

   eglGetCurrentDisplay = dlsym(handle, "eglGetCurrentDisplay");
   if( eglGetCurrentDisplay == NULL){ YUI_LOG(dlerror()); return -1; }

   eglGetCurrentSurface = dlsym(handle, "eglGetCurrentSurface");
   if( eglGetCurrentSurface == NULL){ YUI_LOG(dlerror()); return -1; }

   eglGetCurrentContext = dlsym(handle, "eglGetCurrentContext");
   if( eglGetCurrentContext == NULL){ YUI_LOG(dlerror()); return -1; }

   eglQuerySurface      = dlsym(handle, "eglQuerySurface");
   if( eglQuerySurface == NULL){ YUI_LOG(dlerror()); return -1; }

   eglSwapInterval      = dlsym(handle, "eglSwapInterval");
   if( eglSwapInterval == NULL){ YUI_LOG(dlerror()); return -1; }

   eglMakeCurrent       = dlsym(handle, "eglMakeCurrent");
   if( eglMakeCurrent == NULL){ YUI_LOG(dlerror()); return -1; }

   eglSwapBuffers       = dlsym(handle, "eglSwapBuffers");
   if( eglSwapBuffers == NULL){ YUI_LOG(dlerror()); return -1; }

   eglQueryString       = dlsym(handle, "eglQueryString");
   if( eglQueryString == NULL){ YUI_LOG(dlerror()); return -1; }

   eglGetError          = dlsym(handle, "eglGetError");
   if( eglGetError == NULL){ YUI_LOG(dlerror()); return -1; }

   return 0;
}
#else
int initEGLFunc()
{
   return 0;
}
#endif

jstring Java_org_uoyabause_android_YabauseRunnable_savestate( JNIEnv* env, jobject thiz, jstring  path ){

    jboolean dummy;
    if( cdip == NULL ) return NULL;
    const char *cpath = (*env)->GetStringUTFChars(env,path, &dummy);

    pthread_mutex_lock(&g_mtxGlLock);
    strcpy(s_savepath,cpath);
    g_msg =MSG_SAVE_STATE;
    pthread_mutex_unlock(&g_mtxGlLock);

    (*env)->ReleaseStringUTFChars(env,path, cpath);

    pthread_mutex_lock(&g_mtxFuncSync); 
    pthread_cond_wait(&g_cndFuncSync,&g_mtxFuncSync);
    pthread_mutex_unlock(&g_mtxFuncSync);

    return (*env)->NewStringUTF(env,(const char*)last_state_filename);
}

jint Java_org_uoyabause_android_YabauseRunnable_loadstate( JNIEnv* env, jobject thiz, jstring  path ){

    jboolean dummy;
    const char *cpath = (*env)->GetStringUTFChars(env,path, &dummy);

    pthread_mutex_lock(&g_mtxGlLock);
    strcpy(s_savepath,cpath);
    g_msg =MSG_LOAD_STATE;
    pthread_mutex_unlock(&g_mtxGlLock);

    (*env)->ReleaseStringUTFChars(env,path, cpath);

    pthread_mutex_lock(&g_mtxFuncSync); 
    pthread_cond_wait(&g_cndFuncSync,&g_mtxFuncSync);
    pthread_mutex_unlock(&g_mtxFuncSync);

    return 0;
}

JNIEXPORT int JNICALL Java_org_uoyabause_android_YabauseRunnable_screenshot( JNIEnv* env, jobject thiz, jstring filename )
{
    jboolean dummy;
    const char *cpath = (*env)->GetStringUTFChars(env,filename, &dummy);
    pthread_mutex_lock(&g_mtxGlLock);
    strcpy(screenShotFilename,cpath);
    g_msg = MSG_SCREENSHOT;
    pthread_mutex_unlock(&g_mtxGlLock);
    (*env)->ReleaseStringUTFChars(env,filename, cpath);

    pthread_mutex_lock(&g_mtxFuncSync); 
    s_status = -1;
    pthread_cond_wait(&g_cndFuncSync,&g_mtxFuncSync);
    pthread_mutex_unlock(&g_mtxFuncSync);
    return s_status;
}

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
    s_carttype =  GetCartridgeType();
	s_player2Enable = GetPlayer2Device();
	
	 YUI_LOG("YabauseRunnable_init s_vidcoretype = %d", s_vidcoretype);
    
    OSDInit(0);

    pthread_create(&_threadId, 0, threadStartCallback, NULL );

    return res;
}

int YuiRevokeOGLOnThisThread(){
#if defined(YAB_ASYNC_RENDERING)
    if (!eglMakeCurrent(g_Display, g_Pbuffer, g_Pbuffer, g_Context_Sub)) {
        YUI_LOG("eglMakeCurrent() returned error %X", eglGetError());
        
        // retry three times
        usleep(10000);
         if (!eglMakeCurrent(g_Display, g_Pbuffer, g_Pbuffer, g_Context_Sub)) {
			 YUI_LOG("eglMakeCurrent() returned error %X 2", eglGetError());
             usleep(10000);
             if (!eglMakeCurrent(g_Display, g_Pbuffer, g_Pbuffer, g_Context_Sub)) {
				 YUI_LOG("eglMakeCurrent() returned error %X 3", eglGetError());
                 usleep(10000);
                 if (!eglMakeCurrent(g_Display, g_Pbuffer, g_Pbuffer, g_Context_Sub)) {
					 YUI_LOG("eglMakeCurrent() returned error %X 4", eglGetError());
						usleep(10000);
						if (!eglMakeCurrent(g_Display, g_Pbuffer, g_Pbuffer, g_Context_Sub)) {
							YUI_LOG("eglMakeCurrent() returned error %X 5", eglGetError());
							return -1;
						}

                 }
             }
             
         }
    }	
#endif
    return 0;
}

int YuiUseOGLOnThisThread(){
#if defined(YAB_ASYNC_RENDERING)
    if (!eglMakeCurrent(g_Display, g_Surface, g_Surface, g_Context)) {
        YUI_LOG("eglMakeCurrent() returned error %X", eglGetError());
        
        // retry three times
        usleep(10000);
         if (!eglMakeCurrent(g_Display, g_Surface, g_Surface, g_Context)) {
			 YUI_LOG("eglMakeCurrent() returned error %X 2", eglGetError());
             usleep(10000);
             if (!eglMakeCurrent(g_Display, g_Surface, g_Surface, g_Context)) {
				 YUI_LOG("eglMakeCurrent() returned error %X 3", eglGetError());
                 usleep(10000);
                 if (!eglMakeCurrent(g_Display, g_Surface, g_Surface, g_Context)) {
					 YUI_LOG("eglMakeCurrent() returned error %X 4", eglGetError());
						usleep(10000);
						if (!eglMakeCurrent(g_Display, g_Surface, g_Surface, g_Context)) {
							YUI_LOG("eglMakeCurrent() returned error %X 5", eglGetError());
							return -1;
						}

                 }
             }
             
         }
    }
#endif
    return 0;
}


int initEgl( ANativeWindow* window )
{
	int i;
    int res;
    yabauseinit_struct yinit;
    void * padbits;

     const EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT|EGL_PBUFFER_BIT,
		EGL_RENDERABLE_TYPE,EGL_OPENGL_ES2_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE,24,
        EGL_STENCIL_SIZE,8,

        EGL_NONE
    };

     EGLint pbuffer_attribs[] = {
        EGL_WIDTH, 8,
        EGL_HEIGHT, 8,
        //EGL_TEXTURE_FORMAT, EGL_TEXTURE_RGBA,
        //EGL_TEXTURE_TARGET, EGL_TEXTURE_2D,
        //EGL_LARGEST_PBUFFER, EGL_TRUE,
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


    YUI_LOG("Initializing context");

    if ((display = eglGetDisplay(EGL_DEFAULT_DISPLAY)) == EGL_NO_DISPLAY) {
        YUI_LOG("eglGetDisplay() returned error %X", eglGetError());
        return -1;
    }
    if (!eglInitialize(display, 0, 0)) {
        YUI_LOG("eglInitialize() returned error %X", eglGetError());
        return -1;
    }

    if (!eglChooseConfig(display, attribs, &config, 1, &numConfigs)) {
        YUI_LOG("eglChooseConfig() returned error %X", eglGetError());
        destroy();
        return -1;
    }

    if (!eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format)) {
        YUI_LOG("eglGetConfigAttrib() returned error %X", eglGetError());
        destroy();
        return -1;
    }
    //YUI_LOG("ANativeWindow_setBuffersGeometry");
    //ANativeWindow_setBuffersGeometry(window, 0, 0, format);

    YUI_LOG("eglCreateContext");
    if (!(context = eglCreateContext(display, config, 0, attrib_list))) {
        YUI_LOG("eglCreateContext() for context returned error %d, Fall back to software vidcore mode", eglGetError());
        s_vidcoretype = VIDCORE_SOFT;
        attrib_list[1]=2;
        if (!(context = eglCreateContext(display, config, 0, attrib_list))) {
            YUI_LOG("eglCreateContext() returned error %d", eglGetError());
            destroy();
            return -1;
        }
    }

    if (!(g_Context_Sub = eglCreateContext(display, config, context, attrib_list))) {
        YUI_LOG("eglCreateContext() for g_Context_Sub returned error %d, Fall back to software vidcore mode", eglGetError());
        s_vidcoretype = VIDCORE_SOFT;
        attrib_list[1]=2;
        if (!(g_Context_Sub = eglCreateContext(display, config, context, attrib_list))) {
            YUI_LOG("eglCreateContext() returned error %d", eglGetError());
            destroy();
            return -1;
        }
    }
	
    YUI_LOG("eglCreateWindowSurface");
    if (!(surface = eglCreateWindowSurface(display, config, window, 0))) {
        YUI_LOG("eglCreateWindowSurface() returned error %X", eglGetError());
        destroy();
        return -1;
    }
    
    eglQuerySurface(display,surface,EGL_WIDTH,&width);
    eglQuerySurface(display,surface,EGL_HEIGHT,&height);
    YUI_LOG("eglCreateWindowSurface() ok size = %d,%d", width,height);  

    pbuffer_attribs[1] = ANativeWindow_getWidth(window);
    pbuffer_attribs[3] = ANativeWindow_getHeight(window);

	YUI_LOG("eglCreatePbufferSurface");
    if (!(g_Pbuffer = eglCreatePbufferSurface(display, config, pbuffer_attribs ))) {
        YUI_LOG("eglCreatePbufferSurface() returned error %X", eglGetError());
        destroy();
        return -1;
    }



    YUI_LOG("eglMakeCurrent");
    if (!eglMakeCurrent(display, surface, surface, context)) {
        YUI_LOG("eglMakeCurrent() returned error %X", eglGetError());
        destroy();
        return -1;
    }
   glClearColor( 0.0f, 0.0f,0.0f,1.0f);
    if (!eglQuerySurface(display, surface, EGL_WIDTH, &width) ||
        !eglQuerySurface(display, surface, EGL_HEIGHT, &height)) {
        YUI_LOG("eglQuerySurface() returned error %X", eglGetError());
        destroy();
        return -1;
    }


    g_Display = display;
    g_Surface = surface;
    g_Context = context;
    g_Config  = config;

    YUI_LOG("%s",glGetString(GL_VENDOR));
    YUI_LOG("%s",glGetString(GL_RENDERER));
    YUI_LOG("%s",glGetString(GL_VERSION));
    YUI_LOG("%s",glGetString(GL_EXTENSIONS));
    YUI_LOG("%s",eglQueryString(g_Display,EGL_EXTENSIONS));


    glViewport(0,0,width,height);

    glClearColor( 0.0f, 0.0f,0.0f,1.0f);
    glClear( GL_COLOR_BUFFER_BIT );

	memset(&yinit,0,sizeof(yinit));
    //yinit.m68kcoretype = M68KCORE_C68K;
	yinit.m68kcoretype = M68KCORE_MUSASHI;
    yinit.percoretype = PERCORE_DUMMY;
#ifdef SH2_DYNAREC
    yinit.sh2coretype = g_CpuType;
#else
    yinit.sh2coretype = SH2CORE_DEFAULT;
#endif
    yinit.vidcoretype = s_vidcoretype;
#ifdef HAVE_OPENSL
    yinit.sndcoretype = SNDCORE_OPENSL;
#else
    yinit.sndcoretype = SNDCORE_AUDIOTRACK;
#endif
    yinit.cdcoretype = CDCORE_ISO;
    yinit.carttype = GetCartridgeType();
    yinit.regionid = 0;

    yinit.biospath = s_biospath;
    yinit.cdpath = s_cdpath;
    yinit.buppath = s_buppath;
    yinit.carttype = s_carttype;
    yinit.cartpath = s_cartpath;

    yinit.mpegpath = mpegpath;
    yinit.videoformattype = VIDEOFORMATTYPE_NTSC;
    yinit.frameskip = 0;
    yinit.usethreads = 0;
    yinit.skip_load = 0;
    yinit.video_filter_type = g_VideoFilter;
	yinit.polygon_generation_mode = g_PolygonGenerationMode;
	yinit.use_new_scsp = g_SoundEngine;
    yinit.resolution_mode =g_resolution_mode;

    res = YabauseInit(&yinit);
    if (res != 0) {
      YUI_LOG("Fail to YabauseInit %d", res);
      return -1;
    }

    update_pad_mode();

    //ScspSetFrameAccurate(1);

    if( s_vidcoretype == VIDCORE_OGL ){
        OSDChangeCore(OSDCORE_NANOVG);
	   for (i = 0; VIDCoreList[i] != NULL; i++)
	   {
		  if (VIDCoreList[i]->id == s_vidcoretype)
		  {
			 VIDCoreList[i]->Resize(0,0,width,height,0);
			 break;
		  }
	   }
    }else{
        OSDChangeCore(OSDCORE_SOFT);
        if( YuiInitProgramForSoftwareRendering() != GL_TRUE ){
            YUI_LOG("Fail to YuiInitProgramForSoftwareRendering");
            return -1;
        }
    }

    return 0;
}

int switchWindow( ANativeWindow* window ){

    EGLint format;
    EGLSurface surface;
    EGLint width;
    EGLint height;
	int i;
    
    if( g_Display == EGL_NO_DISPLAY ||  g_Config == NULL ) return -1;
    
    eglMakeCurrent(g_Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    VdpRevoke(); 
    if( YuiUseOGLOnThisThread() == -1 ){
        // Don't care
    }
    eglMakeCurrent(g_Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(g_Display, g_Surface);
    
    //YUI_LOG("ANativeWindow_setBuffersGeometry");
    //ANativeWindow_setBuffersGeometry(window, 0, 0, format);

    YUI_LOG("switchWindow eglCreateWindowSurface");
    if (!(surface = eglCreateWindowSurface(g_Display, g_Config, window, 0))) {
        YUI_LOG("eglCreateWindowSurface() returned error %X", eglGetError());
        destroy();
        return -1;
    }
    
    eglQuerySurface(g_Display,surface,EGL_WIDTH,&width);
    eglQuerySurface(g_Display,surface,EGL_HEIGHT,&height);
    YUI_LOG("eglCreateWindowSurface() ok size = %d,%d", width,height);
    if( eglMakeCurrent(g_Display, surface, surface, g_Context) != EGL_TRUE ){
		YUI_LOG("eglMakeCurrent() returned error %X", eglGetError());
	}

   for (i = 0; VIDCoreList[i] != NULL; i++)
   {
	  if (VIDCoreList[i]->id == s_vidcoretype)
	  {
		YUI_LOG("Resize %d,%s %d,%d",s_vidcoretype,VIDCoreList[i]->Name,width,height);
		 VIDCoreList[i]->Resize(0,0,width,height,0);
		 break;
	  }
   }

    g_Surface = surface;
    eglMakeCurrent(g_Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    VdpResume();
	YuiRevokeOGLOnThisThread();
    return 0;
}

int destroy() {
    YabauseDeInit();

    eglMakeCurrent(g_Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(g_Display, g_Context_Sub);
    eglDestroyContext(g_Display, g_Context);
    eglDestroySurface(g_Display, g_Surface);
    eglDestroySurface(g_Display, g_Pbuffer);
    eglTerminate(g_Display);

    g_window = 0;
    g_Display = EGL_NO_DISPLAY;
    g_Context_Sub = EGL_NO_CONTEXT;
    g_Context = EGL_NO_CONTEXT;
    g_Surface = EGL_NO_SURFACE;
    g_Pbuffer = EGL_NO_SURFACE;
 
    return 0;
}

void
Java_org_uoyabause_android_YabauseRunnable_deinit( JNIEnv* env )
{
    g_msg = MSG_RENDER_LOOP_EXIT;
	//pthread_join(_threadId,NULL);
}

void
Java_org_uoyabause_android_YabauseRunnable_exec( JNIEnv* env )
{
}

void
Java_org_uoyabause_android_YabauseRunnable_reset( JNIEnv* env )
{
    yprintf("sending MSG_OPEN_TRAY");
    pthread_mutex_lock(&g_mtxGlLock);
    g_msg = MSG_RESET;
    pthread_mutex_unlock(&g_mtxGlLock);    
}

void
Java_org_uoyabause_android_YabauseRunnable_press( JNIEnv* env, jobject obj, jint key, jint player )
{
	//yprintf("press: %d,%d",player,key);
    PerKeyDown(MAKE_PAD(player,key));
}

void update_pad_mode(){
    void * padbits;
    if( g_pad_mode == 0 ) {

        PerPortReset();
        padbits = PerPadAdd(&PORTDATA1);

        PerSetKey(MAKE_PAD(0,PERPAD_UP), PERPAD_UP, padbits);
        PerSetKey(MAKE_PAD(0,PERPAD_RIGHT), PERPAD_RIGHT, padbits);
        PerSetKey(MAKE_PAD(0,PERPAD_DOWN), PERPAD_DOWN, padbits);
        PerSetKey(MAKE_PAD(0,PERPAD_LEFT), PERPAD_LEFT, padbits);
        PerSetKey(MAKE_PAD(0,PERPAD_START), PERPAD_START, padbits);
        PerSetKey(MAKE_PAD(0,PERPAD_A), PERPAD_A, padbits);
        PerSetKey(MAKE_PAD(0,PERPAD_B), PERPAD_B, padbits);
        PerSetKey(MAKE_PAD(0,PERPAD_C), PERPAD_C, padbits);
        PerSetKey(MAKE_PAD(0,PERPAD_X), PERPAD_X, padbits);
        PerSetKey(MAKE_PAD(0,PERPAD_Y), PERPAD_Y, padbits);
        PerSetKey(MAKE_PAD(0,PERPAD_Z), PERPAD_Z, padbits);
        PerSetKey(MAKE_PAD(0,PERPAD_RIGHT_TRIGGER),PERPAD_RIGHT_TRIGGER,padbits);
        PerSetKey(MAKE_PAD(0,PERPAD_LEFT_TRIGGER),PERPAD_LEFT_TRIGGER,padbits);
        
        if( s_player2Enable != -1 ) {
            padbits = PerPadAdd(&PORTDATA2);
        
            PerSetKey(MAKE_PAD(1,PERPAD_UP), PERPAD_UP, padbits);
            PerSetKey(MAKE_PAD(1,PERPAD_RIGHT), PERPAD_RIGHT, padbits);
            PerSetKey(MAKE_PAD(1,PERPAD_DOWN), PERPAD_DOWN, padbits);
            PerSetKey(MAKE_PAD(1,PERPAD_LEFT), PERPAD_LEFT, padbits);
            PerSetKey(MAKE_PAD(1,PERPAD_START), PERPAD_START, padbits);
            PerSetKey(MAKE_PAD(1,PERPAD_A), PERPAD_A, padbits);
            PerSetKey(MAKE_PAD(1,PERPAD_B), PERPAD_B, padbits);
            PerSetKey(MAKE_PAD(1,PERPAD_C), PERPAD_C, padbits);
            PerSetKey(MAKE_PAD(1,PERPAD_X), PERPAD_X, padbits);
            PerSetKey(MAKE_PAD(1,PERPAD_Y), PERPAD_Y, padbits);
            PerSetKey(MAKE_PAD(1,PERPAD_Z), PERPAD_Z, padbits);
            PerSetKey(MAKE_PAD(1,PERPAD_RIGHT_TRIGGER),PERPAD_RIGHT_TRIGGER,padbits);
            PerSetKey(MAKE_PAD(1,PERPAD_LEFT_TRIGGER),PERPAD_LEFT_TRIGGER,padbits);
        }

    }else if( g_pad_mode == 1 ){

        PerPortReset();
        padbits = Per3DPadAdd(&PORTDATA1);

        PerSetKey(MAKE_PAD(0,PERANALOG_AXIS1), PERANALOG_AXIS1, padbits);
        PerSetKey(MAKE_PAD(0,PERANALOG_AXIS2), PERANALOG_AXIS2, padbits);
        PerSetKey(MAKE_PAD(0,PERANALOG_AXIS3), PERANALOG_AXIS3, padbits);
        PerSetKey(MAKE_PAD(0,PERANALOG_AXIS4), PERANALOG_AXIS4, padbits);

        PerSetKey(MAKE_PAD(0,PERPAD_UP), PERPAD_UP, padbits);
        PerSetKey(MAKE_PAD(0,PERPAD_RIGHT), PERPAD_RIGHT, padbits);
        PerSetKey(MAKE_PAD(0,PERPAD_DOWN), PERPAD_DOWN, padbits);
        PerSetKey(MAKE_PAD(0,PERPAD_LEFT), PERPAD_LEFT, padbits);
        PerSetKey(MAKE_PAD(0,PERPAD_START), PERPAD_START, padbits);
        PerSetKey(MAKE_PAD(0,PERPAD_A), PERPAD_A, padbits);
        PerSetKey(MAKE_PAD(0,PERPAD_B), PERPAD_B, padbits);
        PerSetKey(MAKE_PAD(0,PERPAD_C), PERPAD_C, padbits);
        PerSetKey(MAKE_PAD(0,PERPAD_X), PERPAD_X, padbits);
        PerSetKey(MAKE_PAD(0,PERPAD_Y), PERPAD_Y, padbits);
        PerSetKey(MAKE_PAD(0,PERPAD_Z), PERPAD_Z, padbits);
        PerSetKey(MAKE_PAD(0,PERPAD_RIGHT_TRIGGER),PERPAD_RIGHT_TRIGGER,padbits);
        PerSetKey(MAKE_PAD(0,PERPAD_LEFT_TRIGGER),PERPAD_LEFT_TRIGGER,padbits);
        
        if( s_player2Enable != -1 ) {
            padbits = Per3DPadAdd(&PORTDATA2);
        
            PerSetKey(MAKE_PAD(1,PERANALOG_AXIS1), PERANALOG_AXIS1, padbits);
            PerSetKey(MAKE_PAD(1,PERANALOG_AXIS2), PERANALOG_AXIS2, padbits);
            PerSetKey(MAKE_PAD(1,PERANALOG_AXIS3), PERANALOG_AXIS3, padbits);
            PerSetKey(MAKE_PAD(1,PERANALOG_AXIS4), PERANALOG_AXIS4, padbits);

            PerSetKey(MAKE_PAD(1,PERPAD_UP), PERPAD_UP, padbits);
            PerSetKey(MAKE_PAD(1,PERPAD_RIGHT), PERPAD_RIGHT, padbits);
            PerSetKey(MAKE_PAD(1,PERPAD_DOWN), PERPAD_DOWN, padbits);
            PerSetKey(MAKE_PAD(1,PERPAD_LEFT), PERPAD_LEFT, padbits);
            PerSetKey(MAKE_PAD(1,PERPAD_START), PERPAD_START, padbits);
            PerSetKey(MAKE_PAD(1,PERPAD_A), PERPAD_A, padbits);
            PerSetKey(MAKE_PAD(1,PERPAD_B), PERPAD_B, padbits);
            PerSetKey(MAKE_PAD(1,PERPAD_C), PERPAD_C, padbits);
            PerSetKey(MAKE_PAD(1,PERPAD_X), PERPAD_X, padbits);
            PerSetKey(MAKE_PAD(1,PERPAD_Y), PERPAD_Y, padbits);
            PerSetKey(MAKE_PAD(1,PERPAD_Z), PERPAD_Z, padbits);
            PerSetKey(MAKE_PAD(1,PERPAD_RIGHT_TRIGGER),PERPAD_RIGHT_TRIGGER,padbits);
            PerSetKey(MAKE_PAD(1,PERPAD_LEFT_TRIGGER),PERPAD_LEFT_TRIGGER,padbits);
        }
    }

}

void
Java_org_uoyabause_android_YabauseRunnable_switch_1padmode( JNIEnv* env, jobject obj, jint mode )
{
    g_pad_mode = mode;
    update_pad_mode();
}

void
Java_org_uoyabause_android_YabauseRunnable_axis( JNIEnv* env, jobject obj, jint key, jint player, jint val )
{
	//yprintf("axis: %d,%d,%d",player,key,val);
    PerAxisValue(MAKE_PAD(player,key),val); // from 0 to 255
}





void
Java_org_uoyabause_android_YabauseRunnable_release( JNIEnv* env, jobject obj, jint key, jint player )
{
	//yprintf("release: %d,%d",player,key);
    PerKeyUp(MAKE_PAD(player,key));
}

void
Java_org_uoyabause_android_YabauseRunnable_enableFPS( JNIEnv* env, jobject obj, jint enable )
{
    g_EnagleFPS = enable;
}

void
Java_org_uoyabause_android_YabauseRunnable_setCpu( JNIEnv* env, jobject obj, jint cpu )
{
    g_CpuType = cpu;
}

void
Java_org_uoyabause_android_YabauseRunnable_setFilter( JNIEnv* env, jobject obj, jint filter )
{
    g_VideoFilter = filter;
}

void
Java_org_uoyabause_android_YabauseRunnable_setSoundEngine( JNIEnv* env, jobject obj, jint sound_engine )
{
    g_SoundEngine = sound_engine;
}

void
Java_org_uoyabause_android_YabauseRunnable_setResolutionMode( JNIEnv* env, jobject obj, jint resolution_mode )
{
    g_resolution_mode = resolution_mode;
}

void
Java_org_uoyabause_android_YabauseRunnable_setPolygonGenerationMode(JNIEnv* env, jobject obj, jint pgm )
{
	g_PolygonGenerationMode = pgm;
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
    ScspSetVolume(volume);
}

jstring Java_org_uoyabause_android_YabauseRunnable_getGameTitle(JNIEnv *env) {
	
	char * buf;
	
	jstring rtn;
	if( cdip == NULL ) return NULL;
	buf = (char*)malloc(1024);
	if( buf == NULL ) return NULL;
    if( strcmp(cdip->cdinfo,"CD-1/1") == 0 ){
        sprintf(buf,"%s",cdip->gamename);
    }else{
	    sprintf(buf,"%s(%s)",cdip->gamename,cdip->cdinfo);
    }
			
	rtn = (*env)->NewStringUTF(env,buf);
	free(buf);
	return rtn;
}

void Java_org_uoyabause_android_YabauseRunnable_updateCheat(JNIEnv *env, jobject object, jobjectArray stringArray) {

    if( stringArray == NULL || (*env)->GetArrayLength(env,stringArray) == 0 ){
        CheatClearCodes();    
        return;
    }

    int stringCount = (*env)->GetArrayLength(env,stringArray);
    int i = 0;
    int index = 0;
    CheatClearCodes();
    for (i=0; i<stringCount; i++) {
        jstring string = (jstring) ((*env)->GetObjectArrayElement(env,stringArray, i));
        if( string == NULL ){
            continue;
        }
        const char * rawString = (*env)->GetStringUTFChars(env,string, 0);
        // Don't forget to call `ReleaseStringUTFChars` when you're done.

        index = CheatAddARCode(rawString);
        CheatEnableCode(index);
        (*env)->ReleaseStringUTFChars(env,string,rawString);
    }
    // CheatDoPatches(); will call at  Vblank-in
    return;
}


jstring Java_org_uoyabause_android_YabauseRunnable_getGameinfo(JNIEnv *env) {
	
	char * buf;
	
	jstring rtn;
	if( cdip == NULL ) return NULL;
	buf = (char*)malloc(1024);
	if( buf == NULL ) return NULL;
	sprintf(buf,
		"{game:{maker_id:\"%s\",product_number:\"%s\",version:\"%s\","
		"release_date:\"%s\",\"device_infomation\":\"%s\","
		"area:\"%s\",game_title:\"%s\",input_device:\"%s\"}}",
			cdip->company,cdip->itemnum,cdip->version,cdip->date,cdip->cdinfo,cdip->region, cdip->gamename, cdip->peripheral);
			
	rtn = (*env)->NewStringUTF(env,buf);
	free(buf);
	return rtn;
}

#if 0
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
#endif

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
    int pause = 0;
	
	YabThreadSetCurrentThreadAffinityMask(0x00);


    while (renderingEnabled != 0) {

        if (g_Display && pause == 0) {
           YabauseExec();
        }

        pthread_mutex_lock(&g_mtxGlLock);        
        // process incoming messages
        switch (g_msg) {

            case MSG_WINDOW_SET:
                YUI_LOG("MSG_WINDOW_SET");
                if( initEgl( g_window ) != 0 ){
                  destroy();
                  pthread_mutex_unlock(&g_mtxGlLock);
                  return;
                }
                break;
            case MSG_WINDOW_CHG:
                YUI_LOG("MSG_WINDOW_CHG");
                if( switchWindow(g_window) != 0 ){
                  destroy();
                  pthread_mutex_unlock(&g_mtxGlLock);
                  return;
                }
                break;
            case MSG_RENDER_LOOP_EXIT:
                YUI_LOG("MSG_RENDER_LOOP_EXIT");            
                renderingEnabled = 0;
                destroy();
                break;

            case MSG_SAVE_STATE:
            {
                int ret;
                time_t t = time(NULL);
                YUI_LOG("MSG_SAVE_STATE");

                sprintf(last_state_filename, "%s/%s_%ld.yss", s_savepath, cdip->itemnum, t);
                ret = YabSaveState(last_state_filename);

                pthread_mutex_lock(&g_mtxFuncSync);
                pthread_cond_signal(&g_cndFuncSync);
                pthread_mutex_unlock(&g_mtxFuncSync);                
            }
            break;

            case MSG_LOAD_STATE:
            {
                int rtn;
                YUI_LOG("MSG_LOAD_STATE");
                rtn = YabLoadState(s_savepath);
                switch(rtn){
                case 0:
                    YUI_LOG("Load State: OK");
                    break;
                case -1:
                    YUI_LOG("Load State: File Not Found");
                    break;
                case -2:
                    YUI_LOG("Load State: Bad format");
                    break;
                case -3:
                    YUI_LOG("Load State: Bad format deep inside");
                    break;                    
                default:
                    YUI_LOG("Load State: Fail unkown");
                    break;                    
                }
                pthread_mutex_lock(&g_mtxFuncSync);
                pthread_cond_signal(&g_cndFuncSync);
                pthread_mutex_unlock(&g_mtxFuncSync);
            }
            break;
            case MSG_PAUSE:
                YUI_LOG("MSG_PAUSE");
                YabFlushBackups();
                ScspMuteAudio(SCSP_MUTE_SYSTEM);
                pause = 1;
                break;
            case MSG_RESUME:
                YUI_LOG("MSG_RESUME");
                ScspUnMuteAudio(SCSP_MUTE_SYSTEM);
                pause = 0;
                break;
            case MSG_OPEN_TRAY:
                YUI_LOG("MSG_OPEN_TRAY");
                Cs2ForceOpenTray();
                break;
            case MSG_CLOSE_TRAY:
                YUI_LOG("MSG_CLOSE_TRAY");
                Cs2ForceCloseTray(CDCORE_ISO, s_cdpath);
                break;                
            case MSG_SCREENSHOT:
                YUI_LOG("MSG_SCREENSHOT");
                s_status = saveScreenshot(screenShotFilename);
                pthread_mutex_lock(&g_mtxFuncSync);
                pthread_cond_signal(&g_cndFuncSync);
                pthread_mutex_unlock(&g_mtxFuncSync);
                break;
            case MSG_RESET:
                YabauseReset();
                break;
            default:
                break;
        }
        g_msg = MSG_NONE;
        pthread_mutex_unlock(&g_mtxGlLock);
    }
    
    YUI_LOG("byebye");

}

void* threadStartCallback(void *myself)
{
    renderLoop();
    pthread_exit(0);
    return NULL;
}

int saveScreenshot( const char * filename ){
    
    int width;
    int height;
    unsigned char * buf = NULL;
    unsigned char * bufRGB = NULL;
    png_bytep * row_pointers = NULL;
    int quality = 100; // best
    FILE * outfile = NULL;
    int row_stride;
    int glerror;
    int u,v;
    int pmode;
    png_byte color_type;
    png_byte bit_depth; 
    png_structp png_ptr;
    png_infop info_ptr;
    int number_of_passes;
    int rtn = -1;
    
    eglMakeCurrent(g_Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    
    VdpRevoke(); 
    if( YuiUseOGLOnThisThread() == -1 ){
        VdpResume();
        return -1;
    }
    
    YUI_LOG("saveScreenshot %s", filename);
    
    eglQuerySurface(g_Display, g_Surface, EGL_WIDTH, &width);
    eglQuerySurface(g_Display, g_Surface, EGL_HEIGHT, &height);
    
    YUI_LOG("screen %d,%d",width,height);
    
    buf = (unsigned char *)malloc(width*height*4);
    if( buf == NULL ) {
        YUI_LOG("not enough memory\n");
        goto FINISH;
    }

    glReadBuffer(GL_BACK);
    pmode = GL_RGBA;
    glGetError();
    glReadPixels(0, 0, width, height, pmode, GL_UNSIGNED_BYTE, buf);
    if( (glerror = glGetError()) != GL_NO_ERROR ){
        YUI_LOG("glReadPixels %04X\n",glerror);
         goto FINISH;
    }
	
	for( u = 3; u <width*height*4; u+=4 ){
		buf[u]=0xFF;
	}
    row_pointers = malloc(sizeof(png_bytep) * height);
    for (v=0; v<height; v++)
        row_pointers[v] = (png_byte*)&buf[ (height-1-v) * width * 4];

    // save as png
    if ((outfile = fopen(filename, "wb")) == NULL) {
        YUI_LOG("can't open %s\n", filename);
        goto FINISH;
    }

    /* initialize stuff */
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png_ptr){
        YUI_LOG("[write_png_file] png_create_write_struct failed");
        goto FINISH;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr){
        YUI_LOG("[write_png_file] png_create_info_struct failed");
        goto FINISH;
    }

    if (setjmp(png_jmpbuf(png_ptr))){
        YUI_LOG("[write_png_file] Error during init_io");
        goto FINISH;
    }
    /* write header */
    png_init_io(png_ptr, outfile);
    
    if (setjmp(png_jmpbuf(png_ptr))){
        YUI_LOG("[write_png_file] Error during writing header");
        goto FINISH;
    }
    bit_depth = 8;
    color_type = PNG_COLOR_TYPE_RGB_ALPHA;
    png_set_IHDR(png_ptr, info_ptr, width, height,
        bit_depth, color_type, PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    //png_set_gAMA(png_ptr, info_ptr, 1.0);
    {
        png_text text[3];
        int txt_fields = 0;
        char desc[256];
        
        time_t      gmt;
        png_time    mod_time;
        
        time(&gmt);
        png_convert_from_time_t(&mod_time, gmt);
        png_set_tIME(png_ptr, info_ptr, &mod_time);
    
        text[txt_fields].key = "Title";
        text[txt_fields].text = Cs2GetCurrentGmaecode();
        text[txt_fields].compression = PNG_TEXT_COMPRESSION_NONE;
        txt_fields++;

        sprintf( desc, "uoYabause Version %s\n VENDER: %s\n RENDERER: %s\n VERSION %s\n",YAB_VERSION,glGetString(GL_VENDOR),glGetString(GL_RENDERER),glGetString(GL_VERSION));
        text[txt_fields].key ="Description";
        text[txt_fields].text=desc;
        text[txt_fields].compression = PNG_TEXT_COMPRESSION_NONE;
        txt_fields++;
        
        png_set_text(png_ptr, info_ptr, text,txt_fields);
    }       
    png_write_info(png_ptr, info_ptr);


    /* write bytes */
    if (setjmp(png_jmpbuf(png_ptr))){
        YUI_LOG("[write_png_file] Error during writing bytes");
        goto FINISH;
    }
    png_write_image(png_ptr, row_pointers);

    /* end write */
    if (setjmp(png_jmpbuf(png_ptr))){
        YUI_LOG("[write_png_file] Error during end of write");
        goto FINISH;
    }
    
    png_write_end(png_ptr, NULL);

#if 0 // JPEG Version
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_pointer[1];

    // Convert to RGB
    bufRGB = (unsigned char *)malloc(width*height*3);
    if( bufRGB == NULL ){
        YUI_LOG("not enough memory\n");
        goto FINISH;
    }
    
    if( pmode == GL_RGBA ){
        
        for( v=(height-1); v>0; v-- ){
            unsigned char * in  = &buf[ v*width*4 ] ;
            unsigned char * out = &bufRGB[ (height-1-v)*width*3 ] ;
            for( u=0; u<width; u++ ){
                out[u*3+0] = in[u*4+0];
                out[u*3+1] = in[u*4+1];
                out[u*3+2] = in[u*4+2];
            }
        }
    }else{
        for( v=(height-1); v>0; v-- ){
            unsigned char * in  = &buf[ v*width*3 ] ;
            unsigned char * out = &bufRGB[ (height-1-v)*width*3 ] ;
            for( u=0; u<width; u++ ){
                out[u*3+0] = in[u*3+0];
                out[u*3+1] = in[u*3+1];
                out[u*3+2] = in[u*3+2];
            }
        }
    }
    // save as jpeg
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    if ((outfile = fopen(filename, "wb")) == NULL) {
        YUI_LOG("can't open %s\n", filename);
        goto FINISH;
    }
    jpeg_stdio_dest(&cinfo, outfile);
    cinfo.image_width = width;  
    cinfo.image_height = height;
    cinfo.input_components = 3; 
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);
    jpeg_start_compress(&cinfo, TRUE);
    row_stride = width * 3;
    while (cinfo.next_scanline < cinfo.image_height) {
        row_pointer[0] = & bufRGB[cinfo.next_scanline * row_stride];
        (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
#endif
    rtn = 0;
FINISH: 
    if(outfile) fclose(outfile);
    if(buf) free(buf);
    if(bufRGB) free(bufRGB);
    if(row_pointers) free(row_pointers);
    eglMakeCurrent(g_Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    VdpResume(); 
    YUI_LOG("write screenshot as  %s\n", filename);
	YuiRevokeOGLOnThisThread();
    
    return rtn;
}
