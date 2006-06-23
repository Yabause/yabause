#include <Carbon/Carbon.h>

#include "../yabause.h"
#include "../peripheral.h"
#include "../sh2core.h"
#include "../sh2int.h"
#include "../vidogl.h"
#include "../scsp.h"
#include "../cdbase.h"
#include "../cs0.h"

extern yabauseinit_struct yinit;

WindowRef CreateSettingsWindow();

typedef struct _YuiAction YuiAction;

struct _YuiAction {
	UInt32 key;
	const char * name;
	void (*press)(void);
	void (*release)(void);
};

extern YuiAction key_config[];
