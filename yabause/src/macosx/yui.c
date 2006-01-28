#include "yui.h"
#include "../cs0.h"
#include "../sndsdl.h"
#include "../vidsdlgl.h"
#include "../vidsdlsoft.h"
#include "../persdl.h"
#include "SDL.h"

#include "cd.h"

#define FS_X_DEFAULT 640
#define FS_Y_DEFAULT 448
#define CARTTYPE_DEFAULT 0
#define N_KEPT_FILES 8

// static boolean forceSoundDisabled = FALSE;

static struct {
  
  enum {GTKYUI_WAIT,GTKYUI_RUN,GTKYUI_PAUSE} running;
} yui;

static void yuiRun(void);
static void yuiPause(void);

SH2Interface_struct *SH2CoreList[] = {
&SH2Interpreter,
&SH2DebugInterpreter,
NULL
};

PerInterface_struct *PERCoreList[] = {
&PERSDL,
NULL
};

CDInterface *CDCoreList[] = {
&DummyCD,
&ISOCD,
&ArchCD,
NULL
};

SoundInterface_struct *SNDCoreList[] = {
&SNDDummy,
&SNDSDL,
NULL
};

VideoInterface_struct *VIDCoreList[] = {
&VIDDummy,
&VIDSDLGL,
&VIDSDLSoft,
NULL
};

int stop;

const char * bios = "./jap.rom";
const char * iso_or_cd = 0;
const char * backup_ram = "backup.ram";
int cdcore = CDCORE_DEFAULT;
int soundenable = 1;

void YuiSetBiosFilename(const char * biosfilename) {
        printf("Bios %s\n",biosfilename);
		
		bios = biosfilename;
}

void YuiSetIsoFilename(const char * isofilename) {
	cdcore = CDCORE_ISO;
	iso_or_cd = isofilename;
}

void YuiSetCdromFilename(const char * cdromfilename) {
	cdcore = CDCORE_ARCH;
	iso_or_cd = cdromfilename;
}

void YuiSetSoundEnable(int enablesound)
{
   soundenable = enablesound;
}

void YuiHideShow(void) {
}

void YuiQuit(void) {
   stop = 1;
}

int YuiInit(void) {


   yabauseinit_struct yinit;

   stop = 0;

   atexit(SDL_Quit);

   yinit.percoretype = PERCORE_SDL;
   yinit.sh2coretype = SH2CORE_DEFAULT;
   yinit.vidcoretype = VIDCORE_SDLGL;
   if (soundenable)
      yinit.sndcoretype = SNDCORE_SDL;
   else
      yinit.sndcoretype = SNDCORE_DUMMY;
  
   
   yinit.carttype = CART_NONE; // fix me
   yinit.regionid = REGION_AUTODETECT;
   yinit.biospath = bios;
   
   // MacOSXCDInit("/dev/disk1s1s2");
   // YuiSetCdromFilename("/dev/disk1");
   printf("ISO OR CD %s\n",iso_or_cd);
   
   // ISO or CD ?
   
   yinit.cdcoretype = cdcore;
   yinit.cdpath = iso_or_cd;
   
   
   yinit.buppath = backup_ram;
   yinit.mpegpath = NULL;
   yinit.cartpath = NULL;


  if (YabauseInit(&yinit) != 0)
       return -1;


   while (!stop)
   {
     if (PERCore->HandleEvents() != 0)
     
	     return -1;
   }

   return 0;




}

void YuiErrorMsg(const char *string) {
   fprintf(stderr, string);
}

void YuiVideoResize(unsigned int w, unsigned int h, int isfullscreen) {
if (VIDCore->IsFullscreen())
      VIDCore->Resize( 320, 224, 0);
}

static void yuiRun(void) {

  switch ( yui.running ) {

  case GTKYUI_WAIT:
    YuiInit();
    if ( yui.running != GTKYUI_PAUSE ) return;
    /* fall through */
  case GTKYUI_PAUSE:
   
    yui.running = GTKYUI_RUN;
    // g_idle_add((gboolean (*)(void*)) GtkWorkaround, (gpointer)1 );
  }
}

static void yuiPause(void) {

  if ( yui.running == GTKYUI_RUN ) {

    yui.running = GTKYUI_PAUSE;
       if (VIDCore->IsFullscreen())
      VIDCore->Resize( 320, 224, 0 );
    //g_idle_remove_by_data( (gpointer)1 );
    // debugUpdateViews();
  }
}

