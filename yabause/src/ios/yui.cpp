
extern "C"{
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
#include "osdcore_ios.h"
#include "sndal.h"
}

#include <OpenGLES/ES3/gl.h>
#include <OpenGLES/ES3/glext.h>

#define YUI_LOG printf
#define MAKE_PAD(a,b) ((a<<24)|(b))

extern "C"{
    void RevokeOGLOnThisThread();
    void UseOGLOnThisThread();
}


extern "C" {
int yprintf( const char * fmt, ... )
{
    int result = 0;
  // va_list ap;
   //va_start(ap, fmt);
   //result = __android_log_vprint(ANDROID_LOG_INFO, LOG_TAG, fmt, ap);
   //va_end(ap);
   return result;
}
}

// Setting Infomation From
static char mpegpath[256] = "\0";
static char cartpath[256] = "\0";
static char screenShotFilename[256] = "\0";
const char * s_biospath = NULL;
const char * s_cdpath = NULL;
char s_buppath[256] ="\0";
char s_cartpath[256] ="\0";
int s_carttype;
char s_savepath[256] ="\0";
int s_vidcoretype = VIDCORE_OGL;
int s_player2Enable = -1;
int g_EnagleFPS = 1;
int g_CpuType = 2;
int g_VideoFilter = 0;
VideoInterface_struct *VIDCoreList[] = {
    &VIDDummy,
    &VIDSoft,
    &VIDOGL,
    NULL
};

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
    &SNDAL,
    NULL
};


extern "C" {
    
    const char * GetBiosPath();
    const char * GetGamePath();
    const char * GetMemoryPath();
    const char * GetStateSavePath();
    int GetCartridgeType();
    int GetVideoInterface();
    const char * GetCartridgePath();
    int GetPlayer2Device();
    int GetEnableFPS();
    int GetEnableFrameSkip();
    int GetUseNewScsp();
    int GetVideFilterType();
    int GetResolutionType();
    int GetIsRotateScreen();
    
int swapAglBuffer ();
    
int start_emulation( int originx, int originy, int width, int height ){
	int i;
    int res;
    yabauseinit_struct yinit ={};
    void * padbits;

    s_biospath = GetBiosPath();
    s_cdpath = GetGamePath();
    strcpy(s_buppath,GetMemoryPath());
    strcpy(s_cartpath,GetCartridgePath());
    strcpy(s_savepath,GetStateSavePath());
    s_vidcoretype = GetVideoInterface();
    s_carttype =  GetCartridgeType();
    
    //s_player2Enable = GetPlayer2Device();

    YUI_LOG("%s",glGetString(GL_VENDOR));
    YUI_LOG("%s",glGetString(GL_RENDERER));
    YUI_LOG("%s",glGetString(GL_VERSION));
    YUI_LOG("%s",glGetString(GL_EXTENSIONS));
    //YUI_LOG("%s",eglQueryString(g_Display,EGL_EXTENSIONS));

    g_EnagleFPS = GetEnableFPS();
 
    glViewport(0,0,width,height);

    glClearColor( 0.0f, 0.0f,0.0f,1.0f);
    glClear( GL_COLOR_BUFFER_BIT );

    memset( &yinit,0,sizeof(yinit) );
    yinit.m68kcoretype = M68KCORE_MUSASHI; //M68KCORE_Q68; //M68KCORE_Q68; //M68KCORE_DUMMY; //M68KCORE_C68K;
    yinit.percoretype = PERCORE_DUMMY;
    yinit.sh2coretype = SH2CORE_DEFAULT;
    yinit.vidcoretype = VIDCORE_OGL;
    yinit.sndcoretype = SNDCORE_AL; //SNDCORE_DEFAULT;
    yinit.cdcoretype = CDCORE_ISO;
    yinit.regionid = 0;

    yinit.biospath = s_biospath;
    yinit.cdpath = s_cdpath;
    yinit.buppath = s_buppath;
    printf("buppath = %s\n",yinit.buppath);
    yinit.carttype = s_carttype;
    yinit.cartpath = s_cartpath;
    
    printf("bios %s¥n",s_biospath);
    LogStart();

    yinit.mpegpath = mpegpath;
    yinit.videoformattype = VIDEOFORMATTYPE_NTSC;
    yinit.frameskip = GetEnableFrameSkip();
    yinit.usethreads = 0;
    yinit.skip_load = 0;
    yinit.video_filter_type = g_VideoFilter;
    s_vidcoretype = VIDCORE_OGL;
    yinit.use_new_scsp = GetUseNewScsp();
    yinit.video_filter_type = GetVideFilterType();
    yinit.resolution_mode = GetResolutionType();
    yinit.rotate_screen = GetIsRotateScreen();
    yinit.extend_backup = 1;

    res = YabauseInit(&yinit);
    if (res != 0) {
      YUI_LOG("Fail to YabauseInit %d", res);
      return -1;
    }

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

    //ScspSetFrameAccurate(1);
    ScspUnMuteAudio(SCSP_MUTE_SYSTEM);
    ScspSetVolume(100);

    OSDInit(0);
    OSDChangeCore(OSDCORE_NANOVG);
    
    if( s_vidcoretype == VIDCORE_OGL ){
        
	   for (i = 0; VIDCoreList[i] != NULL; i++)
	   {
		  if (VIDCoreList[i]->id == s_vidcoretype)
		  {
			 VIDCoreList[i]->Resize(originx,originy,width,height,0);
			 break;
		  }
	   }
    }else{
        //OSDChangeCore(OSDCORE_SOFT);
        //if( YuiInitProgramForSoftwareRendering() != GL_TRUE ){
        //    YUI_LOG("Fail to YuiInitProgramForSoftwareRendering");
        //    return -1;
        //}
    }

    return 0;
}
    void resize_screen( int x, int y, int width, int height ){
        int i=0;
        for (i = 0; VIDCoreList[i] != NULL; i++)
        {
            if (VIDCoreList[i]->id == s_vidcoretype)
            {
                VIDCoreList[i]->Resize(x,y,width,height,0);
                break;
            }
        }
    }


    void YuiErrorMsg(const char *string)
    {
        printf("%s",string);
    }
    void YuiSwapBuffers(void)
    {
        SetOSDToggle(g_EnagleFPS);
        OSDDisplayMessages(NULL,0,0);
        swapAglBuffer();
        
    }


    int YuiRevokeOGLOnThisThread(){
        RevokeOGLOnThisThread();
    }
    int YuiUseOGLOnThisThread(){
        UseOGLOnThisThread();
    }
    
    int emulation_step( int command ){

        int rtn;

        switch (command ) {
            case 1:
                YUI_LOG("MSG_SAVE_STATE %s\n",s_savepath);
                if( (rtn = YabSaveStateSlot(s_savepath, 1)) != 0 ){
                    YUI_LOG("StateSave is failed %d\n",rtn);
                }
                break;
            case 2:
                YUI_LOG("MSG_LOAD_STATE %s\n",s_savepath);
                 if( (rtn = YabLoadStateSlot(s_savepath, 1)) != 0 ){
                    YUI_LOG("StateLoad is failed %d\n",rtn);
                }               
                break;            
        }

        YabauseExec();
    }

    int MuteSound(){
        ScspMuteAudio(SCSP_MUTE_SYSTEM);
        return 0;
    }

    int UnMuteSound(){
        ScspUnMuteAudio(SCSP_MUTE_SYSTEM);
        return 0;
    }

    

    int enterBackGround(){
        YabFlushBackups();
        return 0;
    }
    
}