/*  Copyright 2006 Guillaume Duhamel
    Copyright 2005-2006 Fabien Coulon

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <gtk/gtk.h>

#include "yuivdp2.h"
#include "../vdp2.h"
#include "../yabause.h"
#include "settings.h"

static void yui_vdp2_class_init	(YuiVdp2Class * klass);
static void yui_vdp2_init		(YuiVdp2      * yfe);
static void yui_vdp2_clear(YuiVdp2 * vdp2);

GType yui_vdp2_get_type (void) {
  static GType yfe_type = 0;

  if (!yfe_type)
    {
      static const GTypeInfo yfe_info =
      {
	sizeof (YuiVdp2Class),
	NULL, /* base_init */
        NULL, /* base_finalize */
	(GClassInitFunc) yui_vdp2_class_init,
        NULL, /* class_finalize */
	NULL, /* class_data */
        sizeof (YuiVdp2),
	0,
	(GInstanceInitFunc) yui_vdp2_init,
      };

      yfe_type = g_type_register_static(GTK_TYPE_WINDOW, "YuiVdp2", &yfe_info, 0);
    }

  return yfe_type;
}

static void yui_vdp2_class_init (YuiVdp2Class * klass) {
}

static void yui_vdp2_init (YuiVdp2 * yv) {
	GtkWidget * frame;
	GtkWidget * label;
	GtkWidget * text;
	GtkWidget * scroll;
	GtkWidget * box2;
	const char * screens[] = { "<b>NBG0/RBG1</b>", "<b>NBG1</b>", "<b>NBG2</b>", "<b>NBG3</b>", "<b>RBG0</b>" };
	int i;

	gtk_window_set_title(GTK_WINDOW(yv), "VDP2");

	yv->box = gtk_vbox_new(FALSE, 10);
	gtk_container_set_border_width(GTK_CONTAINER(yv->box), 4);
	gtk_container_add(GTK_CONTAINER(yv), yv->box);

	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(yv->box), scroll);
	box2 = gtk_vbox_new(FALSE, 10);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll), box2);

	for(i = 0;i < (sizeof(screens) / sizeof(screens[0]));i++) {
		frame = gtk_frame_new(NULL);
		gtk_box_pack_start(GTK_BOX(box2), frame, FALSE, FALSE, 0);
		gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
		label = gtk_label_new(screens[i]);
		gtk_frame_set_label_widget(GTK_FRAME(frame), label);
		gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
		text = gtk_text_view_new();
		gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
		gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text), FALSE);
		gtk_container_add(GTK_CONTAINER(frame), text);

		yv->buffer[i] = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
	}
}

static gulong paused_handler;
static gulong running_handler;
static YuiWindow * yui;

GtkWidget * yui_vdp2_new(YuiWindow * y) {
	GtkWidget * dialog;
	YuiVdp2 * yv;

	dialog = GTK_WIDGET(g_object_new(yui_vdp2_get_type(), NULL));
	yv = YUI_VDP2(dialog);

	yui = y;

	{
		GtkWidget * but2, * but3, * but4, * hbox;

		hbox = gtk_hbutton_box_new();
		gtk_box_set_spacing(GTK_BOX(hbox), 4);
		gtk_box_pack_start(GTK_BOX(yv->box ), hbox, FALSE, FALSE, 4);

		but2 = gtk_button_new();
		gtk_action_connect_proxy(gtk_action_group_get_action(yui->action_group, "run"), but2);
		gtk_box_pack_start(GTK_BOX(hbox), but2, FALSE, FALSE, 2);

		but3 = gtk_button_new();
		gtk_action_connect_proxy(gtk_action_group_get_action(yui->action_group, "pause"), but3);
		gtk_box_pack_start(GTK_BOX(hbox), but3, FALSE, FALSE, 2);

		but4 = gtk_button_new_from_stock("gtk-close");
		g_signal_connect_swapped(but4, "clicked", G_CALLBACK(yui_vdp2_destroy), dialog);
		gtk_box_pack_start(GTK_BOX(hbox), but4, FALSE, FALSE, 2);
	}
	paused_handler = g_signal_connect_swapped(yui, "paused", G_CALLBACK(yui_vdp2_update), yv);
	running_handler = g_signal_connect_swapped(yui, "running", G_CALLBACK(yui_vdp2_clear), yv);

	if ((yui->state & (YUI_IS_RUNNING | YUI_IS_INIT)) == YUI_IS_INIT)
		yui_vdp2_update(yv);

	gtk_widget_show_all(GTK_WIDGET(yv));

	return dialog;
}

void yui_vdp2_fill(YuiVdp2 * vdp2) {
}

void yui_vdp2_update(YuiVdp2 * vdp2) {
	gchar tempstr[1024];
	gboolean isscrenabled;

	Vdp2DebugStatsNBG0(tempstr, &isscrenabled);  
	if (isscrenabled) gtk_text_buffer_set_text(vdp2->buffer[0], tempstr, -1);
	else gtk_text_buffer_set_text(vdp2->buffer[0], "<disabled>", -1);

	Vdp2DebugStatsNBG1(tempstr, &isscrenabled);  
	if (isscrenabled) gtk_text_buffer_set_text(vdp2->buffer[1], tempstr, -1);
	else gtk_text_buffer_set_text(vdp2->buffer[1], "<disabled>", -1);

	Vdp2DebugStatsNBG2(tempstr, &isscrenabled);  
	if (isscrenabled) gtk_text_buffer_set_text(vdp2->buffer[2], tempstr, -1);
	else gtk_text_buffer_set_text(vdp2->buffer[2], "<disabled>", -1);

	Vdp2DebugStatsNBG3(tempstr, &isscrenabled);  
	if (isscrenabled) gtk_text_buffer_set_text(vdp2->buffer[3], tempstr, -1);
	else gtk_text_buffer_set_text(vdp2->buffer[3], "<disabled>", -1);

	Vdp2DebugStatsRBG0(tempstr, &isscrenabled);  
	if (isscrenabled) gtk_text_buffer_set_text(vdp2->buffer[4], tempstr, -1);
	else gtk_text_buffer_set_text(vdp2->buffer[4], "<disabled>", -1);
}

void yui_vdp2_destroy(YuiVdp2 * vdp2) {
	g_signal_handler_disconnect(yui, paused_handler);
	g_signal_handler_disconnect(yui, running_handler);
	gtk_widget_destroy(GTK_WIDGET(vdp2));
}

static void yui_vdp2_clear(YuiVdp2 * vdp2) {
	int i;
	for(i = 0;i < 5;i++) {
		gtk_text_buffer_set_text(vdp2->buffer[i], "", -1);
	}
}
