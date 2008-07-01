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
#include <gtk/gtkcheckbutton.h>
#include "yuiresolution.h"

static void yui_resolution_class_init          (YuiResolutionClass *klass);
static void yui_resolution_init                (YuiResolution      *yie);
static void yui_resolution_width_changed	(GtkWidget * w, YuiResolution * yr);
static void yui_resolution_height_changed	(GtkWidget * w, YuiResolution * yr);
static void yui_resolution_fullscreen_toggled	(GtkWidget * w, YuiResolution * yr);
static void yui_resolution_keep_ratio_toggled	(GtkWidget * w, YuiResolution * yr);

GType yui_resolution_get_type (void) {
	static GType yie_type = 0;

	if (!yie_type) {
		static const GTypeInfo yie_info = {
			sizeof (YuiResolutionClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) yui_resolution_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (YuiResolution),
			0,
			(GInstanceInitFunc) yui_resolution_init,
		};

		yie_type = g_type_register_static (GTK_TYPE_TABLE, "YuiResolution", &yie_info, 0);
	}

	return yie_type;
}

#define PROP_KEYFILE	1
#define PROP_GROUP	2

static void yui_resolution_set_property(GObject * object, guint property_id,
		const GValue * value, GParamSpec * pspec) {
	switch(property_id) {
		case PROP_KEYFILE:
			YUI_RESOLUTION(object)->keyfile = g_value_get_pointer(value);
			break;
		case PROP_GROUP:
			YUI_RESOLUTION(object)->group = g_value_get_pointer(value);
			break;
	}
}

static void yui_resolution_get_property(GObject * object, guint property_id,
		GValue * value, GParamSpec * pspec) {
}

static void yui_resolution_class_init (YuiResolutionClass *klass) {
	GParamSpec * param;

	G_OBJECT_CLASS(klass)->set_property = yui_resolution_set_property;
	G_OBJECT_CLASS(klass)->get_property = yui_resolution_get_property;

	param = g_param_spec_pointer("key-file", 0, 0, G_PARAM_READABLE | G_PARAM_WRITABLE);
	g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_KEYFILE, param);

	param = g_param_spec_pointer("group", 0, 0, G_PARAM_READABLE | G_PARAM_WRITABLE);
	g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_GROUP, param);
}

static void yui_resolution_init(YuiResolution * yr) {
	GtkWidget * label_w, * label_h;

	gtk_container_set_border_width (GTK_CONTAINER (yr), 10);
	gtk_table_set_row_spacings (GTK_TABLE (yr), 10);
	gtk_table_set_col_spacings (GTK_TABLE (yr), 10);

	label_w = gtk_label_new ("Width");
  
	gtk_table_attach (GTK_TABLE (yr), label_w, 0, 1, 0, 1,
        	(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label_w), 0, 0.5);

	label_h = gtk_label_new ("Height");
  
	gtk_table_attach (GTK_TABLE (yr), label_h, 0, 1, 1, 2,
        	(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label_h), 0, 0.5);

	yr->entry_w = gtk_entry_new ();
  
	gtk_table_attach (GTK_TABLE (yr), yr->entry_w, 1, 2, 0, 1,
        	(GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_entry_set_invisible_char (GTK_ENTRY (yr->entry_w), 9679);

	yr->entry_h = gtk_entry_new ();
  
	gtk_table_attach (GTK_TABLE (yr), yr->entry_h, 1, 2, 1, 2,
        	(GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_entry_set_invisible_char (GTK_ENTRY (yr->entry_h), 9679);

	yr->fullscreen = gtk_check_button_new_with_mnemonic ("Fullscreen");
	yr->keep_ratio = gtk_check_button_new_with_mnemonic ("Keep ratio");
  
	gtk_table_attach (GTK_TABLE (yr), yr->fullscreen, 2, 3, 0, 1,
        	(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_table_attach (GTK_TABLE (yr), yr->keep_ratio, 2, 3, 1, 2,
        	(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);

	g_signal_connect(yr->entry_w, "changed", G_CALLBACK(yui_resolution_width_changed), yr);
	g_signal_connect(yr->entry_h, "changed", G_CALLBACK(yui_resolution_height_changed), yr);
	g_signal_connect(yr->fullscreen, "toggled", G_CALLBACK(yui_resolution_fullscreen_toggled), yr);
	g_signal_connect(yr->keep_ratio, "toggled", G_CALLBACK(yui_resolution_keep_ratio_toggled), yr);

}

GtkWidget* yui_resolution_new(GKeyFile * keyfile, const gchar * group) {
	GtkWidget * widget;
	YuiResolution * yr;
	gchar *widthText, *heightText;

	widget = GTK_WIDGET(g_object_new(yui_resolution_get_type(), "key-file", keyfile, "group", group, NULL));
	yr = YUI_RESOLUTION(widget);

	widthText = g_key_file_get_value(yr->keyfile, yr->group, "Width", 0);
	if ( !widthText ) widthText = "";
	heightText = g_key_file_get_value(yr->keyfile, yr->group, "Height", 0);
	if ( !heightText ) heightText = "";
	gtk_entry_set_text(GTK_ENTRY(yr->entry_w), widthText );
	gtk_entry_set_text(GTK_ENTRY(yr->entry_h), heightText );
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(yr->fullscreen), g_key_file_get_integer(yr->keyfile, yr->group, "Fullscreen", 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(yr->keep_ratio), g_key_file_get_integer(yr->keyfile, yr->group, "KeepRatio", 0));

	return widget;
}

static void yui_resolution_width_changed(GtkWidget * w, YuiResolution * yr) {
	g_key_file_set_value(yr->keyfile, yr->group, "Width", gtk_entry_get_text(GTK_ENTRY(w)));
}

static void yui_resolution_height_changed(GtkWidget * w, YuiResolution * yr) {
	g_key_file_set_value(yr->keyfile, yr->group, "Height", gtk_entry_get_text(GTK_ENTRY(w)));
}

static void yui_resolution_fullscreen_toggled(GtkWidget * w, YuiResolution * yr) {
	g_key_file_set_integer(yr->keyfile, yr->group, "Fullscreen", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)));
}

static void yui_resolution_keep_ratio_toggled(GtkWidget * w, YuiResolution * yr) {
	g_key_file_set_integer(yr->keyfile, yr->group, "KeepRatio", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)));
}
