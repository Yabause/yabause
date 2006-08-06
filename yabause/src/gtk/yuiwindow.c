#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "yuiwindow.h"
#include "gtkglwidget.h"
#include "../yabause.h"

#include "settings.h"

#include "icon.xpm"

static void yui_window_class_init	(YuiWindowClass * klass);
static void yui_window_init		(YuiWindow      * yfe);
static void yui_window_changed	(GtkWidget * widget, YuiWindow * yfe);
static gboolean yui_window_keypress(GtkWidget *widget, GdkEventKey *event, gpointer user_data);
static gboolean yui_window_keyrelease(GtkWidget *widget, GdkEventKey *event, gpointer user_data);
static void yui_window_keep_clean(GtkWidget * widget, GdkEventExpose * event, YuiWindow * yui);

static GtkActionEntry action_entries[] = {
	{ "run" , "gtk-media-play", "Run", "<Ctrl>r", NULL, G_CALLBACK(yui_window_run) },
	{ "pause" , "gtk-media-pause", "Pause", "<Ctrl>p", NULL, G_CALLBACK(yui_window_pause) },
	{ "reset", NULL, "Reset", NULL, NULL, G_CALLBACK(yui_window_reset) },
	{ "fullscreen", "gtk-fullscreen", "Fullscreen", "<Ctrl>f", NULL, G_CALLBACK(yui_window_toggle_fullscreen) },
	{ "quit", "gtk-quit", "Quit", "<Ctrl>q", NULL, gtk_main_quit }
};

GType yui_window_get_type (void) {
  static GType yfe_type = 0;

  if (!yfe_type)
    {
      static const GTypeInfo yfe_info =
      {
	sizeof (YuiWindowClass),
	NULL, /* base_init */
        NULL, /* base_finalize */
	(GClassInitFunc) yui_window_class_init,
        NULL, /* class_finalize */
	NULL, /* class_data */
        sizeof (YuiWindow),
	0,
	(GInstanceInitFunc) yui_window_init,
      };

      yfe_type = g_type_register_static(GTK_TYPE_WINDOW, "YuiWindow", &yfe_info, 0);
    }

  return yfe_type;
}

enum { YUI_WINDOW_RUNNING_SIGNAL, YUI_WINDOW_PAUSED_SIGNAL, LAST_SIGNAL };

static guint yui_window_signals[LAST_SIGNAL] = { 0, 0 };

static void yui_window_class_init (YuiWindowClass * klass) {
	yui_window_signals[YUI_WINDOW_RUNNING_SIGNAL] = g_signal_new ("running", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			G_STRUCT_OFFSET(YuiWindowClass, yui_window_running), NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	yui_window_signals[YUI_WINDOW_PAUSED_SIGNAL] = g_signal_new ("paused", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			G_STRUCT_OFFSET(YuiWindowClass, yui_window_paused), NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void yui_set_accel_group(gpointer action, gpointer group) {
	gtk_action_set_accel_group(action, group);
}

void yui_popup( YuiWindow* w, gchar* text, GtkMessageType mType ) {
  
  GtkWidget* dialog = gtk_message_dialog_new (GTK_WINDOW(w),
				   GTK_DIALOG_DESTROY_WITH_PARENT,
				   mType,
				   GTK_BUTTONS_CLOSE,
				   text );
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

static void yui_window_init (YuiWindow * yw) {
	GtkAccelGroup * accel_group = gtk_accel_group_new();

	yw->action_group = gtk_action_group_new("yui");
	gtk_action_group_add_actions(yw->action_group, action_entries, sizeof(action_entries) / sizeof(GtkActionEntry), yw);
	gtk_action_set_sensitive(gtk_action_group_get_action(yw->action_group, "pause"), FALSE);
	gtk_action_set_sensitive(gtk_action_group_get_action(yw->action_group, "reset"), FALSE);
	{
		GList * list = gtk_action_group_list_actions(yw->action_group);
		g_list_foreach(list, yui_set_accel_group, accel_group);
	}
	gtk_window_add_accel_group(GTK_WINDOW(yw), accel_group);

	gtk_window_set_icon(GTK_WINDOW(yw), gdk_pixbuf_new_from_xpm_data((const char **)icon_xpm));
	gtk_window_set_title (GTK_WINDOW(yw), "Yabause");

	yw->box = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(yw), yw->box);

	yw->menu = create_menu(yw);
	gtk_box_pack_start(GTK_BOX(yw->box), yw->menu, FALSE, FALSE, 0);

	yw->area = gtk_gl_widget_new();
	gtk_box_pack_start(GTK_BOX(yw->box), yw->area, TRUE, TRUE, 0);
	gtk_widget_set_size_request(GTK_WIDGET(yw->area), 320, 240 );

	g_signal_connect(G_OBJECT(yw), "destroy", G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(G_OBJECT(yw), "key-press-event", G_CALLBACK(yui_window_keypress), yw);
	g_signal_connect(G_OBJECT(yw), "key-release-event", G_CALLBACK(yui_window_keyrelease), yw);

	yw->log = gtk_text_view_new();
	gtk_box_pack_start(GTK_BOX(yw->box), yw->log, FALSE, FALSE, 0);

	gtk_widget_show(yw->box);
	gtk_widget_show_all(yw->menu);
	gtk_widget_show(yw->area);

	yw->clean_handler = g_signal_connect(yw->area, "expose-event", G_CALLBACK(yui_window_keep_clean), yw);
	yw->state = 0;
}

GtkWidget * yui_window_new(YuiAction * act, GCallback ifunc, gpointer idata,
		GSourceFunc rfunc, GCallback resetfunc) {
	GtkWidget * widget;
	YuiWindow * yw;

	widget = GTK_WIDGET(g_object_new(yui_window_get_type(), 0));
	yw = YUI_WINDOW(widget);

	yw->actions = act;
	yw->init_func = ifunc;
	yw->init_data = idata;
	yw->run_func = rfunc;
	yw->reset_func = resetfunc;

	return widget;
}

void yui_window_toggle_fullscreen(GtkWidget * w, YuiWindow * yui) {
	static int togglefullscreen = 0;
	togglefullscreen = 1 - togglefullscreen;
	if (togglefullscreen) {
		gtk_widget_hide(yui->menu);
		gtk_window_fullscreen(GTK_WINDOW(yui));
	} else {
		gtk_window_unfullscreen(GTK_WINDOW(yui));
		gtk_widget_show(yui->menu);
	}
}

static gboolean yui_window_keypress(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
	int i = 0;
	YuiWindow * yui = YUI_WINDOW(widget);

	while(yui->actions[i].name) {
		if (event->keyval == yui->actions[i].key) {
			yui->actions[i].press();
			//return TRUE;
		}
		i++;
	}
	return FALSE;
}

static gboolean yui_window_keyrelease(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
	int i = 0;
	YuiWindow * yui = YUI_WINDOW(widget);

	while(yui->actions[i].name) {
		if (event->keyval == yui->actions[i].key) {
			yui->actions[i].release();
			//return TRUE;
		}
		i++;
	}
	return FALSE;
}

void yui_window_update(YuiWindow * yui) {

  if (!(yui->state & YUI_IS_RUNNING)) drawPause(yui->area);
  else draw(yui->area);
}

void yui_window_log(YuiWindow * yui, const char * message) {
	GtkTextBuffer * buffer;
	GtkTextIter iter;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(yui->log));
	gtk_text_buffer_get_end_iter(buffer, &iter);
	gtk_text_buffer_insert(buffer, &iter, message, -1);
}

void yui_window_show_log(YuiWindow * yui) {
	static int i = 0;
	if (i)
		gtk_widget_hide(yui->log);
	else
		gtk_widget_show(yui->log);
	i = !i;
}

static void yui_window_keep_clean(GtkWidget * widget, GdkEventExpose * event, YuiWindow * yui) {
	glClear(GL_COLOR_BUFFER_BIT);
	yui_window_update(yui);
}

void yui_window_start(GtkWidget * w, YuiWindow * yui) {
	if ((yui->state & YUI_IS_INIT) == 0) {
	  ((int (*)(gpointer)) yui->init_func)(yui->init_data);
	  yui->state |= YUI_IS_INIT;
	  gtk_action_set_sensitive(gtk_action_group_get_action(yui->action_group, "reset"), TRUE);
	}
}

void yui_window_run(GtkWidget * w, YuiWindow * yui) {
	yui_window_start(w, yui);

	if ((yui->state & YUI_IS_RUNNING) == 0) {
		g_idle_add(yui->run_func, GINT_TO_POINTER(1));
		g_signal_emit(G_OBJECT(yui), yui_window_signals[YUI_WINDOW_RUNNING_SIGNAL], 0);
		yui->state |= YUI_IS_RUNNING;
		gtk_action_set_sensitive(gtk_action_group_get_action(yui->action_group, "run"), FALSE);
		gtk_action_set_sensitive(gtk_action_group_get_action(yui->action_group, "pause"), TRUE);
	}
}

void yui_window_pause(GtkWidget * w, YuiWindow * yui) {
	if (yui->state & YUI_IS_RUNNING) {
		g_idle_remove_by_data(GINT_TO_POINTER(1));
		g_signal_emit(G_OBJECT(yui), yui_window_signals[YUI_WINDOW_PAUSED_SIGNAL], 0);
		yui->state &= ~YUI_IS_RUNNING;
		gtk_action_set_sensitive(gtk_action_group_get_action(yui->action_group, "run"), TRUE);
		gtk_action_set_sensitive(gtk_action_group_get_action(yui->action_group, "pause"), FALSE);
	}
}

void yui_window_reset(GtkWidget * w, YuiWindow * yui) {
	if (yui->state & YUI_IS_INIT) {
		yui->reset_func();
	}
}

void yui_window_invalidate(GtkWidget * w, YuiWindow * yui ) {

  /* Emit a pause signal while already in pause means refresh all debug views */

  if ( !(yui->state & YUI_IS_RUNNING ))
    g_signal_emit(G_OBJECT(yui), yui_window_signals[YUI_WINDOW_PAUSED_SIGNAL], 0);
}
