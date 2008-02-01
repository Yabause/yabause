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
};

#endif // QTYABAUSE_H
