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

#include "../../config.h"
#include "yabause.h"
#include "scsp.h"
#include "vidsoft.h"
#include "peripheral.h"
#include "m68kcore.h"
#include "sh2core.h"
#include "sh2int.h"
#include "cdbase.h"
#include "cs2.h"
#include "debug.h"

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
    errorMsg = (*env)->GetMethodID(env, yclass, "errorMsg", "(Ljava/lang/String;)V");
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
    VIDCore->GetGlSize(&buf_width, &buf_height);
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
    int res;
    void * padbits;

    yabause = (*env)->NewGlobalRef(env, yab);
    ybitmap = (*env)->NewGlobalRef(env, bitmap);

    yinit.m68kcoretype = M68KCORE_C68K;
    yinit.percoretype = PERCORE_DUMMY;
#ifdef SH2_DYNAREC
    yinit.sh2coretype = 2;
#else
    yinit.sh2coretype = SH2CORE_DEFAULT;
#endif
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

    res = YabauseInit(&yinit);

    PerPortReset();
    padbits = PerPadAdd(&PORTDATA1);
    PerSetKey(1, PERPAD_LEFT, padbits);
    PerSetKey(4, PERPAD_UP, padbits);
    PerSetKey(6, PERPAD_DOWN, padbits);
    PerSetKey(9, PERPAD_RIGHT, padbits);

    return res;
}

void
Java_org_yabause_android_YabauseRunnable_deinit( JNIEnv* env )
{
    YabauseDeInit();
}

void
Java_org_yabause_android_YabauseRunnable_exec( JNIEnv* env )
{
    YabauseExec();
}

void
Java_org_yabause_android_YabauseRunnable_press( JNIEnv* env, jobject obj, jint key )
{
    PerKeyDown(key);
}

void
Java_org_yabause_android_YabauseRunnable_release( JNIEnv* env, jobject obj, jint key )
{
    PerKeyUp(key);
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
