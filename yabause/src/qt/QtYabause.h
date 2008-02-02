#ifndef QTYABAUSE_H
#define QTYABAUSE_H

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

typedef yabauseinit_struct YabauseConf;

#include "ui/UIYabause.h"
#include "Settings.h"

namespace QtYabause
{
	UIYabause* mainWindow();
	Settings* settings();

	// get core by id
	M68K_struct* getM68KCore( int id );
	SH2Interface_struct* getSH2Core( int id );
	PerInterface_struct* getPERCore( int id );
	CDInterface* getCDCore( int id );
	SoundInterface_struct* getSNDCore( int id );
	VideoInterface_struct* getVDICore( int id );
};

#endif // QTYABAUSE_H
