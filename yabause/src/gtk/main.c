#include "gtkglwidget.h"
#include "yuiwindow.h"

#include "../yabause.h"
#include "../peripheral.h"
#include "../sh2core.h"
#include "../sh2int.h"
#include "../vidogl.h"
#include "../cs0.h"
#include "../cdbase.h"
#include "../scsp.h"
#include "../sndsdl.h"

#include "settings.h"

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
&VIDOGL,
NULL
};

GtkWidget * yui;
GKeyFile * keyfile;
yabauseinit_struct yinit;
gulong clean_handler;
YuiAction key_config[] = {
	{ 0 , "Up", PerUpPressed, PerUpReleased },
	{ 0 , "Right", PerRightPressed, PerRightReleased },
	{ 0 , "Down", PerDownPressed, PerDownReleased },
	{ 0 , "Left", PerLeftPressed, PerLeftReleased },
	{ 0 , "Right trigger", PerRTriggerPressed, PerRTriggerReleased },
	{ 0 , "Left trigger", PerLTriggerPressed, PerLTriggerReleased },
	{ 0 , "Start", PerStartPressed, PerStartReleased },
	{ 0 , "A", PerAPressed, PerAReleased },
	{ 0 , "B", PerBPressed, PerBReleased },
	{ 0 , "C", PerCPressed, PerCReleased },
	{ 0 , "X", PerXPressed, PerXReleased },
	{ 0 , "Y", PerYPressed, PerYReleased },
	{ 0 , "Z", PerZPressed, PerZReleased },
	{ 0, 0, 0, 0 }
};

void keep_clean(void) {
	glClear(GL_COLOR_BUFFER_BIT);
	yui_window_update(YUI_WINDOW(yui));
}

GtkWidget * yui_new() {
	yui = yui_window_new(key_config);

	clean_handler = g_signal_connect(YUI_WINDOW(yui)->area, "expose-event", G_CALLBACK(keep_clean), 0);

	gtk_widget_show(yui);

	return yui;
}

void yui_settings_init(void) {
	yinit.percoretype = PERCORE_DUMMY;
	yinit.sh2coretype = SH2CORE_DEFAULT;
	yinit.vidcoretype = VIDCORE_OGL;
	yinit.sndcoretype = SNDCORE_DUMMY;
	yinit.cdcoretype = CDCORE_DEFAULT;
	yinit.carttype = CART_NONE;
	yinit.regionid = 0;
	yinit.biospath = 0;
	yinit.cdpath = 0;
	yinit.buppath = 0;
	yinit.mpegpath = 0;
	yinit.cartpath = 0;
}

gchar * inifile;

void yui_settings_load(void) {
	int i;
	g_key_file_load_from_file(keyfile, inifile, G_KEY_FILE_NONE, 0);
	if (yinit.biospath)
		g_free(yinit.biospath);
	yinit.biospath = g_strdup(g_key_file_get_value(keyfile, "General", "BiosPath", 0));
	if (yinit.cdpath)
		g_free(yinit.cdpath);
	yinit.cdpath = g_strdup(g_key_file_get_value(keyfile, "General", "CDROMDrive", 0));
	yinit.cdcoretype = g_key_file_get_integer(keyfile, "General", "CDROMCore", 0);
	{
		char * region = g_key_file_get_value(keyfile, "General", "Region", 0);
		if ((region == 0) || !strcmp(region, "Auto")) {
			yinit.regionid = 0;
		} else {
			switch(region[0]) {
				case 'J': yinit.regionid = 1; break;
				case 'T': yinit.regionid = 2; break;
				case 'U': yinit.regionid = 4; break;
				case 'B': yinit.regionid = 5; break;
				case 'K': yinit.regionid = 6; break;
				case 'A': yinit.regionid = 0xA; break;
				case 'E': yinit.regionid = 0xC; break;
				case 'L': yinit.regionid = 0xD; break;
			}
		}
	}
	if (yinit.cartpath)
		g_free(yinit.cartpath);
	yinit.cartpath = g_strdup(g_key_file_get_value(keyfile, "General", "CartPath", 0));
	if (yinit.buppath)
		g_free(yinit.buppath);
	yinit.buppath = g_strdup(g_key_file_get_value(keyfile, "General", "BackupRamPath", 0));
	if (yinit.mpegpath)
		g_free(yinit.mpegpath);
	yinit.mpegpath = g_strdup(g_key_file_get_value(keyfile, "General", "MpegRomPath", 0));
	yinit.carttype = g_key_file_get_integer(keyfile, "General", "CartType", 0);
	yinit.vidcoretype = g_key_file_get_integer(keyfile, "General", "VideoCore", 0);
	yinit.sndcoretype = g_key_file_get_integer(keyfile, "General", "SoundCore", 0);
	i = 0;
	while(key_config[i].name) {
		key_config[i].key =
			gdk_keyval_from_name(g_key_file_get_value(keyfile, "Input", key_config[i].name, 0));
		i++;
	}
	yui_resize(g_key_file_get_integer(keyfile, "General", "Width", 0),
			g_key_file_get_integer(keyfile, "General", "Height", 0),
			g_key_file_get_integer(keyfile, "General", "Keep ratio", 0));
}

int main(int argc, char *argv[]) {
	inifile = g_build_filename(g_get_home_dir(), ".yabause.ini", NULL);
	
	keyfile = g_key_file_new();

	gtk_init(&argc, &argv);
	gtk_gl_init(&argc, &argv);

	yui_settings_init();

	yui = yui_new();

	yui_settings_load();

	gtk_main ();

	return 0;
}

void YuiQuit(void) {
}

void YuiErrorMsg(const char * string) {
	yui_window_log(YUI_WINDOW(yui), string);
}

void YuiVideoResize(unsigned int w, unsigned int h, int isfullscreen) {
}

void YuiSetVideoAttribute(int type, int val) {
}

int YuiSetVideoMode(int width, int height, int bpp, int fullscreen) {
	return 0;
}

void YuiSwapBuffers(void) {
	yui_window_update(YUI_WINDOW(yui));
}

int yui_main(void) {
	YabauseExec();
	return TRUE;
}

void yui_run(void) {
	if(YabauseInit(&yinit) == -1) {
		g_print("Yabause initialization failed\n");
		return;
	}

	g_signal_handler_disconnect(YUI_WINDOW(yui)->area, clean_handler);
	g_idle_add(yui_main, 0);
}

void yui_conf(void) {
	gint result;
	GtkWidget * dialog = create_dialog1();
	gtk_widget_show_all(dialog);

	result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	switch(result) {
		case GTK_RESPONSE_OK:
			g_file_set_contents(inifile, g_key_file_to_data(keyfile, 0, 0), -1, 0);
			yui_settings_load();
			break;
		case GTK_RESPONSE_CANCEL:
			g_key_file_load_from_file(keyfile, inifile, G_KEY_FILE_NONE, 0);
			break;
	}
}

void yui_resize(guint width, guint height, gboolean keep_ratio) {
	gtk_widget_set_size_request(YUI_WINDOW(yui)->area, width, height);
}
