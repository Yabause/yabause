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

#include "yuivdp1.h"
#include "../vdp1.h"
#include "../yabause.h"
#include "settings.h"

static void yui_vdp1_class_init	(YuiVdp1Class * klass);
static void yui_vdp1_init		(YuiVdp1      * yfe);
static void yui_vdp1_spin_cursor_changed(GtkWidget * spin, YuiVdp1 * vdp1);
static void yui_vdp1_view_cursor_changed(GtkWidget * view, YuiVdp1 * vdp1);
static void yui_vdp1_clear(YuiVdp1 * vdp1);

GType yui_vdp1_get_type (void) {
  static GType yfe_type = 0;

  if (!yfe_type)
    {
      static const GTypeInfo yfe_info =
      {
	sizeof (YuiVdp1Class),
	NULL, /* base_init */
        NULL, /* base_finalize */
	(GClassInitFunc) yui_vdp1_class_init,
        NULL, /* class_finalize */
	NULL, /* class_data */
        sizeof (YuiVdp1),
	0,
	(GInstanceInitFunc) yui_vdp1_init,
      };

      yfe_type = g_type_register_static(GTK_TYPE_WINDOW, "YuiVdp1", &yfe_info, 0);
    }

  return yfe_type;
}

static void yui_vdp1_class_init (YuiVdp1Class * klass) {
}

static void yui_vdp1_init (YuiVdp1 * yv) {
	gtk_window_set_title(GTK_WINDOW(yv), "VDP1");

	yv->vbox = gtk_vbox_new(FALSE, 2);
	gtk_container_set_border_width(GTK_CONTAINER(yv->vbox), 4);
	gtk_container_add(GTK_CONTAINER(yv), yv->vbox);

	yv->spin = gtk_spin_button_new_with_range(0, MAX_VDP1_COMMAND, 1);
	gtk_spin_button_set_range(GTK_SPIN_BUTTON(yv->spin), 0, 0);
	gtk_box_pack_start(GTK_BOX(yv->vbox), yv->spin, FALSE, FALSE, 4);
	g_signal_connect(G_OBJECT(yv->spin), "value-changed", GTK_SIGNAL_FUNC(yui_vdp1_spin_cursor_changed), yv);

	yv->store = gtk_list_store_new(1, G_TYPE_STRING);
	yv->view = gtk_tree_view_new_with_model(GTK_TREE_MODEL (yv->store));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(yv->view), FALSE);
	{
		GtkCellRenderer *renderer;
		GtkTreeViewColumn *column;

		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes("Command", renderer, "text", 0, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW (yv->view), column);
	}
	gtk_box_pack_start(GTK_BOX(yv->vbox), yv->view, FALSE, FALSE, 4);
	g_signal_connect(yv->view, "cursor-changed", G_CALLBACK(yui_vdp1_view_cursor_changed), yv);

	g_signal_connect(G_OBJECT(yv), "delete-event", GTK_SIGNAL_FUNC(yui_vdp1_destroy), NULL);

	{
		GtkWidget * scroll = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		GtkWidget * text = gtk_text_view_new();
		gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
		gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text), FALSE);
		yv->buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text));
		gtk_container_add(GTK_CONTAINER(scroll), text);
		gtk_box_pack_start(GTK_BOX(yv->vbox), scroll, TRUE, TRUE, 4);
	}

	yv->hbox = gtk_hbutton_box_new();
	gtk_box_set_spacing(GTK_BOX(yv->hbox), 4);
	gtk_box_pack_start(GTK_BOX(yv->vbox ), yv->hbox, FALSE, FALSE, 4);

	yv->cursor = 0;
}

static gulong paused_handler;
static gulong running_handler;
static YuiWindow * yui;

GtkWidget * yui_vdp1_new(YuiWindow * y) {
	GtkWidget * dialog;
	YuiVdp1 * yv;

	yui = y;

	dialog = GTK_WIDGET(g_object_new(yui_vdp1_get_type(), NULL));
	yv = YUI_VDP1(dialog);

	{
		GtkWidget * but2, * but3, * but4;
		but2 = gtk_button_new();
		gtk_action_connect_proxy(gtk_action_group_get_action(yui->action_group, "run"), but2);
		gtk_box_pack_start(GTK_BOX(yv->hbox), but2, FALSE, FALSE, 2);

		but3 = gtk_button_new();
		gtk_action_connect_proxy(gtk_action_group_get_action(yui->action_group, "pause"), but3);
		gtk_box_pack_start(GTK_BOX(yv->hbox), but3, FALSE, FALSE, 2);

		but4 = gtk_button_new_from_stock("gtk-close");
		g_signal_connect_swapped(but4, "clicked", G_CALLBACK(yui_vdp1_destroy), dialog);
		gtk_box_pack_start(GTK_BOX(yv->hbox), but4, FALSE, FALSE, 2);
	}
	paused_handler = g_signal_connect_swapped(yui, "paused", G_CALLBACK(yui_vdp1_update), yv);
	running_handler = g_signal_connect_swapped(yui, "running", G_CALLBACK(yui_vdp1_clear), yv);

	if ((yui->state & (YUI_IS_RUNNING | YUI_IS_INIT)) == YUI_IS_INIT)
		yui_vdp1_update(yv);

	gtk_widget_show_all(GTK_WIDGET(yv));

	return dialog;
}

void yui_vdp1_fill(YuiVdp1 * vdp1) {
	gint i, j;
	gchar * string;
	gchar nameTemp[1024];
	GtkTreeIter iter;

	yui_vdp1_clear(vdp1);

	j = vdp1->cursor - 2;
	if (j < 0) j = 0;
	i = j + 5;
	string = Vdp1DebugGetCommandNumberName(j);
	while(string && (j < i)) {
		gtk_list_store_append(vdp1->store, &iter);
		gtk_list_store_set(vdp1->store, &iter, 0, string, -1);

		if (j == vdp1->cursor) {
			GtkTreePath * path = gtk_tree_model_get_path(GTK_TREE_MODEL(vdp1->store), &iter);
			gtk_tree_view_set_cursor(GTK_TREE_VIEW(vdp1->view), path, NULL, 0);
			gtk_tree_path_free(path);
		}

		j++;
		string = Vdp1DebugGetCommandNumberName(j);
	}

	Vdp1DebugCommand(vdp1->cursor, nameTemp);
	gtk_text_buffer_set_text(vdp1->buffer, g_strstrip(nameTemp), -1);
}

static void yui_vdp1_spin_cursor_changed(GtkWidget * spin, YuiVdp1 * vdp1) {
	vdp1->cursor = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin));
	yui_vdp1_fill(vdp1);
}

static void yui_vdp1_view_cursor_changed(GtkWidget * view, YuiVdp1 * vdp1) {
}

void yui_vdp1_update(YuiVdp1 * vdp1) {
	gint i;
	for(i = 0 ; i < MAX_VDP1_COMMAND ; i++ ) if ( !Vdp1DebugGetCommandNumberName(i)) break;
	gtk_spin_button_set_range(GTK_SPIN_BUTTON(vdp1->spin), 0, i-1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(vdp1->spin), 0);
	vdp1->cursor = 0;
	yui_vdp1_fill(vdp1);
}

void yui_vdp1_destroy(YuiVdp1 * vdp1) {
	g_signal_handler_disconnect(yui, running_handler);
	g_signal_handler_disconnect(yui, paused_handler);

	gtk_widget_destroy(GTK_WIDGET(vdp1));
}

static void yui_vdp1_clear(YuiVdp1 * vdp1) {
	gtk_list_store_clear(vdp1->store);
	gtk_text_buffer_set_text(vdp1->buffer, "", -1);
}
