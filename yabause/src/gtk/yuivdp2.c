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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include <gtk/gtk.h>

#include "yuivdp2.h"
#include "../vdp2.h"
#include "../yabause.h"
#include "settings.h"
#include "../vdp2debug.h"

static void yui_vdp2_class_init	(YuiVdp2Class * klass);
static void yui_vdp2_init		(YuiVdp2      * yfe);
static void yui_vdp2_clear(YuiVdp2 * vdp2);
static void yui_vdp2_view_cursor_changed(GtkWidget * view, YuiVdp2 * vdp2);

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
        NULL,
      };

      yfe_type = g_type_register_static(GTK_TYPE_WINDOW, "YuiVdp2", &yfe_info, 0);
    }

  return yfe_type;
}

static void yui_vdp2_class_init (UNUSED YuiVdp2Class * klass) {
}

static void yui_vdp2_init (YuiVdp2 * yv) {
	GtkWidget * text;
	GtkWidget * scroll;
	GtkWidget * box, * box2;
	GtkWidget * hpane;
	GtkWidget * view;
	const char * screens[] = { "General", "NBG0/RBG1", "NBG1", "NBG2", "NBG3", "RBG0" };
	unsigned int i;

	gtk_window_set_title(GTK_WINDOW(yv), "VDP2");

	box = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(box), 0);
	gtk_container_add(GTK_CONTAINER(yv), box);

	yv->toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(yv->toolbar), GTK_TOOLBAR_ICONS);
	gtk_box_pack_start(GTK_BOX(box), yv->toolbar, FALSE, FALSE, 0);

	hpane = gtk_hpaned_new();
	gtk_container_add(GTK_CONTAINER(box), hpane);

	yv->store = gtk_list_store_new(1, G_TYPE_STRING);
	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL (yv->store));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);
	{
		GtkWidget * scroll;
		GtkCellRenderer *renderer;
		GtkTreeViewColumn *column;

		scroll = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes("Command", renderer, "text", 0, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW (view), column);

		gtk_container_add(GTK_CONTAINER(scroll), view);
		gtk_paned_pack1(GTK_PANED(hpane), scroll, FALSE, TRUE);
	}
	g_signal_connect(view, "cursor-changed", G_CALLBACK(yui_vdp2_view_cursor_changed), yv);

	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_paned_pack2(GTK_PANED(hpane), scroll, TRUE, TRUE);
	box2 = gtk_vbox_new(FALSE, 10);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll), box2);

	for(i = 0;i < (sizeof(screens) / sizeof(screens[0]));i++) {
		GtkTreeIter iter;
		gtk_list_store_append(yv->store, &iter);
		gtk_list_store_set(yv->store, &iter, 0, screens[i], -1);
	}

	text = gtk_text_view_new();
	gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text), FALSE);
	gtk_container_add(GTK_CONTAINER(box2), text);

	yv->buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));

	gtk_window_set_default_size(GTK_WINDOW(yv), 500, 450);

	gtk_paned_set_position(GTK_PANED(hpane), 120);

	g_signal_connect(G_OBJECT(yv), "delete-event", GTK_SIGNAL_FUNC(yui_vdp2_destroy), NULL);
}

GtkWidget * yui_vdp2_new(YuiWindow * y) {
	GtkWidget * dialog;
	YuiVdp2 * yv;
	
	dialog = GTK_WIDGET(g_object_new(yui_vdp2_get_type(), NULL));
	yv = YUI_VDP2(dialog);	

	yv->yui = y;

	if (!( yv->yui->state & YUI_IS_INIT )) {
	  yui_window_run(yv->yui);
	  yui_window_pause(yv->yui);
	}
	
	{
		GtkToolItem * play_button, * pause_button;

		play_button = gtk_tool_button_new_from_stock("run");
		gtk_action_connect_proxy(gtk_action_group_get_action(yv->yui->action_group, "run"), GTK_WIDGET(play_button));
		gtk_toolbar_insert(GTK_TOOLBAR(yv->toolbar), GTK_TOOL_ITEM(play_button), -1);

		pause_button = gtk_tool_button_new_from_stock("pause");
		gtk_action_connect_proxy(gtk_action_group_get_action(yv->yui->action_group, "pause"), GTK_WIDGET(pause_button));
		gtk_toolbar_insert(GTK_TOOLBAR(yv->toolbar), GTK_TOOL_ITEM(pause_button), -1);
	}
	yv->paused_handler = g_signal_connect_swapped(yv->yui, "paused", G_CALLBACK(yui_vdp2_update), yv);
	yv->running_handler = g_signal_connect_swapped(yv->yui, "running", G_CALLBACK(yui_vdp2_clear), yv);

	if ((yv->yui->state & (YUI_IS_RUNNING | YUI_IS_INIT)) == YUI_IS_INIT)
		yui_vdp2_update(yv);

	gtk_widget_show_all(GTK_WIDGET(yv));

	return dialog;
}

void yui_vdp2_update(YuiVdp2 * vdp2) {
	gchar nameTemp[VDP2_DEBUG_STRING_SIZE];
	gboolean isscrenabled;

	switch(vdp2->cursor) {
		case 0:
			Vdp2DebugStatsGeneral(nameTemp, &isscrenabled);  
			break;
		case 1:
			Vdp2DebugStatsNBG0(nameTemp, &isscrenabled);  
			break;
		case 2:
			Vdp2DebugStatsNBG1(nameTemp, &isscrenabled);  
			break;
		case 3:
			Vdp2DebugStatsNBG2(nameTemp, &isscrenabled);  
			break;
		case 4:
			Vdp2DebugStatsNBG3(nameTemp, &isscrenabled);  
			break;
		case 5:
			Vdp2DebugStatsRBG0(nameTemp, &isscrenabled);  
			break;
	}

	if (isscrenabled) {
		gtk_text_buffer_set_text(vdp2->buffer, nameTemp, -1);
	} else {
		gtk_text_buffer_set_text(vdp2->buffer, "", -1);
	}
}

void yui_vdp2_destroy(YuiVdp2 * vdp2) {
	g_signal_handler_disconnect(vdp2->yui, vdp2->paused_handler);
	g_signal_handler_disconnect(vdp2->yui, vdp2->running_handler);
	gtk_widget_destroy(GTK_WIDGET(vdp2));
}

static void yui_vdp2_clear(YuiVdp2 * vdp2) {
	gtk_text_buffer_set_text(vdp2->buffer, "", -1);
}

void yui_vdp2_view_cursor_changed(GtkWidget * view, YuiVdp2 * vdp2) {
	GtkTreePath * path;
	gchar * strpath;
	int i;

	gtk_tree_view_get_cursor(GTK_TREE_VIEW(view), &path, NULL);

	if (path) {
		strpath = gtk_tree_path_to_string(path);

		sscanf(strpath, "%i", &i);

		vdp2->cursor = i;

		yui_vdp2_update(vdp2);

		g_free(strpath);
		gtk_tree_path_free(path);
	}
}
