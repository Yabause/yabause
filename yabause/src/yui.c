#include "yui.h"
#include "cs0.h"
#include "sndsdl.h"
#include "vidsdlgl.h"
#include "vidsdlsoft.h"
#include "persdl.h"
#include "SDL.h"

SH2Interface_struct *SH2CoreList[] = {
&SH2Interpreter,
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

const char * bios = "jap.rom";
const char * iso_or_cd = 0;
const char * backup_ram = "backup.ram";
int cdcore = CDCORE_DEFAULT;
int soundenable = 1;

void YuiSetBiosFilename(const char * biosfilename) {
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
   yinit.cdcoretype = cdcore;
   yinit.carttype = CART_NONE; // fix me
   yinit.regionid = REGION_AUTODETECT;
   yinit.biospath = bios;
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

