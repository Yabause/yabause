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
static jclass yabause;
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

void
Java_org_yabause_android_YabauseRunnable_init( JNIEnv* env, jobject obj, jobject bitmap )
{
    yabauseinit_struct yinit;

    ybitmap = (*env)->NewGlobalRef(env, bitmap);

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

    YabauseInit(&yinit);
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
