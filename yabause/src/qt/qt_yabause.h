#ifndef QT_YABAUSE_H
#define QT_YABAUSE_H

extern "C"
{
	#include "../yabause.h"
	#include "../peripheral.h"
	#include "../sh2core.h"
	#include "../sh2int.h"
	#include "../vidogl.h"
	#include "../vidsoft.h"
	#include "../cs0.h"
	#include "../cdbase.h"
	#include "../scsp.h"
	#include "../sndsdl.h"
	#include "../persdljoy.h"
	#include "../debug.h"
	#include "../m68kcore.h"
	#include "../m68kc68k.h"
}

#include "ui/UIYabause.h"
#include "Settings.h"

namespace Yabause
{
	void init();
	void deInit();
	void exec();
	void log( const char* string );
	
	UIYabause* mainWindow();
	Settings* settings();
	
	/*
	void YuiErrorMsg(const char *string);
	void YuiVideoResize(unsigned int w, unsigned int h, int isfullscreen);
	int YuiSetVideoMode(int width, int height, int bpp, int fullscreen);
	void YuiSetVideoAttribute(int type, int val);
	void YuiSwapBuffers();
	*/
};

#endif // QT_YABAUSE_H
