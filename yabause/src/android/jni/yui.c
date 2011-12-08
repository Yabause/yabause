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
#include <android/bitmap.h>
#include <android/log.h>

#include "../../yabause.h"
#include "../../scsp.h"
#include "../../vidsoft.h"
#include "../../peripheral.h"
#include "../../m68kcore.h"
#include "../../sh2core.h"
#include "../../sh2int.h"
#include "../../cdbase.h"
#include "../../cs2.h"

static JavaVM * yvm;
static jobject yabause;
static jobject ybitmap;

static char biospath[256] = "/mnt/sdcard/jap.rom";
static char cdpath[256] = "\0";
static char buppath[256] = "\0";
static char mpegpath[256] = "\0";
static char cartpath[256] = "\0";

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
NULL
};

VideoInterface_struct *VIDCoreList[] = {
&VIDDummy,
&VIDSoft,
NULL
};

void YuiErrorMsg(const char *string)
{
    jclass yclass;
    jmethodID errorMsg;
    jstring message;
    JNIEnv * env;
    if ((*yvm)->GetEnv(yvm, (void**) &env, JNI_VERSION_1_6) != JNI_OK)
        return;

    yclass = (*env)->GetObjectClass(env, yabause);
    __android_log_print(ANDROID_LOG_INFO, "yabause", "yclass = %p", yclass);
    errorMsg = (*env)->GetMethodID(env, yclass, "errorMsg", "(Ljava/lang/String;)V");
    __android_log_print(ANDROID_LOG_INFO, "yabause", "errorMsg = %p", errorMsg);
    message = (*env)->NewStringUTF(env, string);
    (*env)->CallVoidMethod(env, yabause, errorMsg, message);
}

void YuiSwapBuffers(void)
{
    u32 * buffer;
    int buf_width, buf_height;
    AndroidBitmapInfo info;
    void * pixels;
    JNIEnv * env;
    int x, y;
    if ((*yvm)->GetEnv(yvm, (void**) &env, JNI_VERSION_1_6) != JNI_OK)
        return;

    AndroidBitmap_getInfo(env, ybitmap, &info);

    AndroidBitmap_lockPixels(env, ybitmap, &pixels);

    buffer = dispbuffer;
    VIDSoftGetScreenSize(&buf_width, &buf_height);
    for(y = 0;y < info.height;y++) {
        for(x = 0;x < info.width;x++) {
            *((uint32_t *) pixels + x) = *(buffer + x);
        }
        pixels += info.stride;
        buffer += buf_width;
    }

    AndroidBitmap_unlockPixels(env, ybitmap);
}

jint
Java_org_yabause_android_YabauseRunnable_init( JNIEnv* env, jobject obj, jobject yab, jobject bitmap )
{
    yabauseinit_struct yinit;

    yabause = (*env)->NewGlobalRef(env, yab);
    __android_log_print(ANDROID_LOG_INFO, "yabause", "yabause = %p", yabause);
    ybitmap = (*env)->NewGlobalRef(env, bitmap);
    __android_log_print(ANDROID_LOG_INFO, "yabause", "ybitmap = %p", ybitmap);

    yinit.m68kcoretype = M68KCORE_C68K;
    yinit.percoretype = PERCORE_DUMMY;
    yinit.sh2coretype = SH2CORE_DEFAULT;
    yinit.vidcoretype = VIDCORE_SOFT;
    yinit.sndcoretype = SNDCORE_DUMMY;
    yinit.cdcoretype = CDCORE_DEFAULT;
    yinit.carttype = CART_NONE;
    yinit.regionid = 0;
    yinit.biospath = biospath;
    yinit.cdpath = cdpath;
    yinit.buppath = buppath;
    yinit.mpegpath = mpegpath;
    yinit.cartpath = cartpath;
    yinit.videoformattype = VIDEOFORMATTYPE_NTSC;

    return YabauseInit(&yinit);
}

void
Java_org_yabause_android_YabauseRunnable_exec( JNIEnv* env )
{
    YabauseExec();
}

jint JNI_OnLoad(JavaVM * vm, void * reserved)
{
    JNIEnv * env;
    if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_6) != JNI_OK)
        return -1;

    yvm = vm;

    return JNI_VERSION_1_6;
}
