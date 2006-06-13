#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "yuiwindow.h"
#include "gtkglwidget.h"

#include "menu.h"

#include "icon.xpm"

static void yui_window_class_init	(YuiWindowClass * klass);
static void yui_window_init		(YuiWindow      * yfe);
static void yui_window_changed	(GtkWidget * widget, YuiWindow * yfe);
static gboolean yui_window_keypress(GtkWidget *widget, GdkEventKey *event, gpointer user_data);
static gboolean yui_window_keyrelease(GtkWidget *widget, GdkEventKey *event, gpointer user_data);

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

static void yui_window_class_init (YuiWindowClass * klass) {
}

static void yui_window_init (YuiWindow * yw) {
	gtk_window_set_icon(GTK_WINDOW(yw), gdk_pixbuf_new_from_xpm_data((const char **)icon_xpm));
	gtk_window_set_title (GTK_WINDOW(yw), "Yabause");

	yw->box = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(yw), yw->box);

	yw->menu = create_menu(yw);
	gtk_box_pack_start(GTK_BOX(yw->box), yw->menu, FALSE, FALSE, 0);

	yw->area = gtk_gl_widget_new();
	gtk_box_pack_start(GTK_BOX(yw->box), yw->area, TRUE, TRUE, 0);

	g_signal_connect(G_OBJECT(yw), "destroy", G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(G_OBJECT(yw), "key-press-event", G_CALLBACK(yui_window_keypress), yw);
	g_signal_connect(G_OBJECT(yw), "key-release-event", G_CALLBACK(yui_window_keyrelease), yw);

	yw->log = gtk_text_view_new();
	gtk_box_pack_start(GTK_BOX(yw->box), yw->log, FALSE, FALSE, 0);

	gtk_widget_show(yw->box);
	gtk_widget_show_all(yw->menu);
	gtk_widget_show(yw->area);
}

GtkWidget * yui_window_new(YuiAction * act) {
	GtkWidget * widget;
	YuiWindow * yw;

	widget = GTK_WIDGET(g_object_new(yui_window_get_type(), 0));
	yw = YUI_WINDOW(widget);

	yw->actions = act;

	return widget;
}

void yui_window_toggle_fullscreen(YuiWindow * yui) {
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
			return TRUE;
		}
		i++;
	}
	if (event->keyval == GDK_f) {
		yui_window_toggle_fullscreen(yui);
		return TRUE;
	}
	return FALSE;
}

static gboolean yui_window_keyrelease(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
	int i = 0;
	YuiWindow * yui = YUI_WINDOW(widget);

	while(yui->actions[i].name) {
		if (event->keyval == yui->actions[i].key) {
			yui->actions[i].release();
			return TRUE;
		}
		i++;
	}
	return FALSE;
}

void yui_window_update(YuiWindow * yui) {
	draw(yui->area);
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
