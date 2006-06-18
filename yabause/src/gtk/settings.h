#ifndef YUI_SETTINGS_H
#define YUI_SETTINGS_H

#include <gtk/gtk.h>

#include "../vdp1.h"
#include "../vdp2.h"
#include "../scsp.h"
#include "../yabause.h"
#include "../cdbase.h"
#include "../peripheral.h"

#include "yuiwindow.h"

extern GKeyFile * keyfile;
extern yabauseinit_struct yinit;

extern VideoInterface_struct * VIDCoreList[];
extern SoundInterface_struct * SNDCoreList[];

GtkWidget* create_dialog1 (void);
GtkWidget* create_menu (YuiWindow *);

void yui_run(void);
void yui_conf(void);
void yui_resize(guint, guint, gboolean);

void gtk_yui_toggle_fullscreen(void);

#endif
