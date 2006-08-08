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

#include <gtk/gtksignal.h>
#include <gtk/gtktable.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkentry.h>
#include "yuiinputentry.h"

static void yui_input_entry_class_init          (YuiInputEntryClass *klass);
static void yui_input_entry_init                (YuiInputEntry      *yie);
gboolean yui_input_entry_keypress(GtkWidget * widget, GdkEventKey * event, gpointer name);

GType yui_input_entry_get_type (void) {
	static GType yie_type = 0;

	if (!yie_type) {
		static const GTypeInfo yie_info = {
			sizeof (YuiInputEntryClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) yui_input_entry_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (YuiInputEntry),
			0,
			(GInstanceInitFunc) yui_input_entry_init,
		};

		yie_type = g_type_register_static (GTK_TYPE_TABLE, "YuiInputEntry", &yie_info, 0);
	}

	return yie_type;
}

#define PROP_KEYFILE	1
#define PROP_GROUP	2

static void yui_input_entry_set_property(GObject * object, guint property_id,
		const GValue * value, GParamSpec * pspec) {
	switch(property_id) {
		case PROP_KEYFILE:
			YUI_INPUT_ENTRY(object)->keyfile = g_value_get_pointer(value);
			break;
		case PROP_GROUP:
			YUI_INPUT_ENTRY(object)->group = g_value_get_pointer(value);
			break;
	}
}

static void yui_input_entry_get_property(GObject * object, guint property_id,
		GValue * value, GParamSpec * pspec) {
}

static void yui_input_entry_class_init (YuiInputEntryClass *klass) {
	GParamSpec * param;

	G_OBJECT_CLASS(klass)->set_property = yui_input_entry_set_property;
	G_OBJECT_CLASS(klass)->get_property = yui_input_entry_get_property;

	param = g_param_spec_pointer("key-file", 0, 0, G_PARAM_READABLE | G_PARAM_WRITABLE);
	g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_KEYFILE, param);

	param = g_param_spec_pointer("group", 0, 0, G_PARAM_READABLE | G_PARAM_WRITABLE);
	g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_GROUP, param);
}

static void yui_input_entry_init(YuiInputEntry *yie) {
	gtk_container_set_border_width(GTK_CONTAINER(yie), 10);
	gtk_table_set_row_spacings(GTK_TABLE(yie), 10);
	gtk_table_set_col_spacings(GTK_TABLE(yie), 10);
}

GtkWidget* yui_input_entry_new(GKeyFile * keyfile, const gchar * group, const gchar * keys[]) {
	GtkWidget * widget;
	GtkWidget * label;
	GtkWidget * entry;
	gchar * keyName;
	int row = 0;

	widget = GTK_WIDGET(g_object_new(yui_input_entry_get_type(), "key-file", keyfile, "group", group, NULL));

	while(keys[row]) {
		gtk_table_resize(GTK_TABLE(widget), row + 1, 2);
		label = gtk_label_new(keys[row]);
  
		gtk_table_attach(GTK_TABLE(widget), label, 0, 1, row , row + 1,
			(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
		gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

		entry = gtk_entry_new ();

		keyName = g_key_file_get_value(keyfile, group, keys[row], 0);
		if ( !keyName ) keyName = "";
		gtk_entry_set_text(GTK_ENTRY(entry), keyName );
		g_signal_connect(entry, "key-press-event", G_CALLBACK(yui_input_entry_keypress), keys[row]);
  
		gtk_table_attach(GTK_TABLE(widget), entry,  1, 2, row, row + 1,
			(GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0);
		gtk_entry_set_invisible_char (GTK_ENTRY (entry), 9679);
		row++;
	}

	return widget;
}

gboolean yui_input_entry_keypress(GtkWidget * widget, GdkEventKey * event, gpointer name) {
	gtk_entry_set_text(GTK_ENTRY(widget), gdk_keyval_name(event->keyval));
	g_key_file_set_value(YUI_INPUT_ENTRY(gtk_widget_get_parent(widget))->keyfile,
		YUI_INPUT_ENTRY(gtk_widget_get_parent(widget))->group,
		name, gdk_keyval_name(event->keyval));

	return TRUE;
}
