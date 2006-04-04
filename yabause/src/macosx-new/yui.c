#include "yui.h"
#include "../cs0.h"
#include "../sndsdl.h"
#include "../vidsdlgl.h"
#include "../vidsdlsoft.h"
#include "../persdl.h"
#import <SDL/SDL.h>

#include "cd.h"

#define FS_X_DEFAULT 640
#define FS_Y_DEFAULT 448
#define CARTTYPE_DEFAULT 0
#define N_KEPT_FILES 8

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

int isRunning = 0;
unsigned int setw = 320;
unsigned int seth = 224;

char * bios = NULL;
char * iso_or_cd = NULL;
char *backup_ram = "backup.ram"; 
char *cartidge_path = "saved_games"; // Will add support for other cartidges later, when restyling the interface
int carttype = CART_BACKUPRAM32MBIT;
int cdcore = CDCORE_DEFAULT;
int soundenable = 1;

void YuiSetBiosFilename(const char * biosfilename) 
{	
	if(bios)
		free(bios);
	
	if(biosfilename != NULL)
	{
		bios = malloc(strlen(biosfilename)+1);
		strcpy(bios, biosfilename);
	}
	else
		bios = NULL;
	
	return;
}

void YuiSetCartidgeFilename(const char * cartfilename) 
{	
	if(cartidge_path)
		free(cartidge_path);
	
	if(cartfilename != NULL)
	{
		bios = malloc(strlen(cartfilename)+1);
		strcpy(cartidge_path, cartfilename);
	}
	else
		cartidge_path = NULL;
	
	return;
}

void YuiSetCartidgeType(int type)
{
	carttype = type;
}

void YuiSetIsoFilename(const char * isofilename) 
{
	cdcore = CDCORE_ISO;
	
	if(iso_or_cd)
		free(iso_or_cd);
	
	if(isofilename != NULL)
	{
		iso_or_cd = malloc(strlen(isofilename)+1);
		strcpy(iso_or_cd, isofilename);
	}
	else
		iso_or_cd = NULL;
	
	return;
}

void YuiSetCdromFilename(const char * cdromfilename) {
	cdcore = CDCORE_ARCH;
	if(iso_or_cd)
		free(iso_or_cd);
	
	if(cdromfilename != NULL)
	{
		iso_or_cd = malloc(strlen(cdromfilename)+1);
		strcpy(iso_or_cd, cdromfilename);
	}
	else
		iso_or_cd = NULL;
	
	return;
}

void YuiSetSoundEnable(int enablesound)
{
   soundenable = enablesound;
}

void YuiHideShow(void) {
}

void YuiQuit(void) 
{
	isRunning = 0;
}

int YuiInit(void) 
{
	yabauseinit_struct yinit;
	
	isRunning = 1;
	
	yinit.percoretype = PERCORE_SDL;
	yinit.sh2coretype = SH2CORE_DEFAULT;
	yinit.vidcoretype = VIDCORE_SDLGL;
	
	if (soundenable)
		yinit.sndcoretype = SNDCORE_SDL;
	else
		yinit.sndcoretype = SNDCORE_DUMMY;
	
	
	yinit.carttype = carttype; 
	yinit.regionid = REGION_AUTODETECT;
	yinit.biospath = bios;
	
	yinit.cdcoretype = cdcore;
	yinit.cdpath = iso_or_cd;
	
	
	yinit.buppath = backup_ram;
	yinit.mpegpath = NULL;
	yinit.cartpath = cartidge_path;
	
	
	if (YabauseInit(&yinit) != 0)
		return -1;
	
	VIDCore->Resize(setw, seth, 0);
	
	while (isRunning)
	{
		if (PERCore->HandleEvents() != 0)
		{
			SDL_Quit();
			YabauseDeInit();
			return -1;
		}
	}
	
	SDL_Quit();
	YabauseDeInit();
	return 0;
}

void YuiErrorMsg(const char *string) 
{
   fprintf(stderr, string);
}

void YuiVideoResize(unsigned int w, unsigned int h, int isfullscreen) 
{
	if(!isfullscreen && (w != setw || h != seth) && (w == 320 && w == 224))
		VIDCore->Resize(setw, seth, 0);
}

void YuiSetDefaultWindowSize(unsigned int w, unsigned int h)
{
	seth = h;
	setw = w;
}

int YuiIsRunning()
{
	return isRunning;
}
