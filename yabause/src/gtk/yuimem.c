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
#include <gdk/gdkkeysyms.h>

#include "yuimem.h"
#include "settings.h"

static void yui_mem_class_init		(YuiMemClass * klass);
static void yui_mem_init		(YuiMem      * yfe);
static void yui_mem_clear		(YuiMem * vdp1);
static void yui_mem_address_changed	(GtkWidget * w, YuiMem * ym);
static void yui_mem_content_changed  ( GtkCellRendererText *cellrenderertext, gchar *arg1, gchar *arg2, YuiMem *ym);
static void yui_mem_pagedown_pressed (GtkWidget *w, gpointer func, gpointer data, gpointer data2, YuiMem *ym);
static void yui_mem_pageup_pressed   (GtkWidget *w, gpointer func, gpointer data, gpointer data2, YuiMem *ym);
static void yui_mem_update		(YuiMem * ym);
static void yui_mem_combo_changed(GtkWidget * w, YuiMem * ym);

struct { gchar* name; u32 address; } quickAddress[] = 
  { {"VDP2_VRAM_A0", 0x25E00000 },
    {"VDP2_VRAM_A1", 0x25E20000 },
    {"VDP2_VRAM_B0", 0x25E40000 },
    {"VDP2_VRAM_B1", 0x25E60000 },
    {"VDP2_CRAM", 0x25F00000 },
    {"LWRAM", 0x20200000 },
    {"HWRAM", 0x26000000 },
    {"SpriteVRAM", 0x25C00000 },
    {0, 0 } };

GType yui_mem_get_type (void) {
  static GType yfe_type = 0;

  if (!yfe_type)
    {
      static const GTypeInfo yfe_info =
      {
	sizeof (YuiMemClass),
	NULL, /* base_init */
        NULL, /* base_finalize */
	(GClassInitFunc) yui_mem_class_init,
        NULL, /* class_finalize */
	NULL, /* class_data */
        sizeof (YuiMem),
	0,
	(GInstanceInitFunc) yui_mem_init,
      };

      yfe_type = g_type_register_static(GTK_TYPE_WINDOW, "YuiMem", &yfe_info, 0);
    }

  return yfe_type;
}

static void yui_mem_class_init (YuiMemClass * klass) {
}

static void yui_mem_init (YuiMem * yv) {
	GtkWidget * view;
	GtkWidget * scroll;
	GtkWidget * hboxTop;
	GtkCellRenderer * renderer;
	GtkTreeViewColumn * column;
	GtkAccelGroup *accelGroup;
	gint i;

	gtk_window_set_title(GTK_WINDOW(yv), "Memory dump");

	yv->vbox = gtk_vbox_new(FALSE, 2);
	gtk_container_set_border_width(GTK_CONTAINER(yv->vbox), 4);
	gtk_box_set_spacing(GTK_BOX(yv->vbox), 4);
	gtk_container_add(GTK_CONTAINER(yv), yv->vbox);

	hboxTop = gtk_hbox_new(FALSE, 2);
	gtk_box_pack_start(GTK_BOX(yv->vbox), hboxTop, FALSE, FALSE, 0);	

	yv->entry = gtk_entry_new();
	gtk_entry_set_max_length( GTK_ENTRY( yv->entry ), 8 );
	g_signal_connect(yv->entry, "activate", G_CALLBACK(yui_mem_address_changed), yv);
	gtk_box_pack_start(GTK_BOX(hboxTop), yv->entry, FALSE, FALSE, 0);

	yv->quickCombo = gtk_combo_box_new_text();
	gtk_combo_box_insert_text( GTK_COMBO_BOX( yv->quickCombo ), 0, "Select ..." );
	for ( i = 0 ; quickAddress[i].name ; i++ )
	  gtk_combo_box_insert_text( GTK_COMBO_BOX( yv->quickCombo ), i+1, quickAddress[i].name );
	gtk_combo_box_set_active( GTK_COMBO_BOX(yv->quickCombo), 0 );
	gtk_box_pack_start( GTK_BOX( hboxTop ), yv->quickCombo, FALSE, FALSE, 0 );
	g_signal_connect(yv->quickCombo, "changed", G_CALLBACK(yui_mem_combo_changed), yv );

	yv->store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL (yv->store));

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Address", renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW (view), column);
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Dump", renderer, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW (view), column);
	g_object_set(G_OBJECT(renderer), "editable", TRUE, "mode", GTK_CELL_RENDERER_MODE_EDITABLE, NULL );
	g_signal_connect(G_OBJECT(renderer), "edited", GTK_SIGNAL_FUNC(yui_mem_content_changed), yv );
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scroll), view);
	gtk_box_pack_start(GTK_BOX(yv->vbox), scroll, TRUE, TRUE, 0);

	g_signal_connect(G_OBJECT(yv), "delete-event", GTK_SIGNAL_FUNC(yui_mem_destroy), NULL);

	yv->hbox = gtk_hbutton_box_new();
	gtk_box_set_spacing(GTK_BOX(yv->hbox), 4);
	gtk_box_pack_start(GTK_BOX(yv->vbox ), yv->hbox, FALSE, FALSE, 4);

	accelGroup = gtk_accel_group_new ();
	gtk_accel_group_connect( accelGroup, GDK_Page_Up, 0, 0, 
				 g_cclosure_new (G_CALLBACK(yui_mem_pageup_pressed), yv, NULL) );
	gtk_accel_group_connect( accelGroup, GDK_Page_Down, 0, 0, 
				 g_cclosure_new (G_CALLBACK(yui_mem_pagedown_pressed), yv, NULL) );
	gtk_window_add_accel_group( GTK_WINDOW( yv ), accelGroup );

	yv->address = 0;
	yv->wLine = 8;
}

static YuiWindow * yui;

GtkWidget * yui_mem_new(YuiWindow * y) {
	GtkWidget * dialog;
	YuiMem * yv;

	yui = y;

	if (!( yui->state & YUI_IS_INIT )) {
	  yui_window_run(dialog, yui);
	  yui_window_pause(dialog, yui);
	}
	
	dialog = GTK_WIDGET(g_object_new(yui_mem_get_type(), NULL));
	yv = YUI_MEM(dialog);

	{
		GtkWidget * but2, * but3, * but4;
		but2 = gtk_button_new();
		gtk_action_connect_proxy(gtk_action_group_get_action(yui->action_group, "run"), but2);
		gtk_box_pack_start(GTK_BOX(yv->hbox), but2, FALSE, FALSE, 2);

		but3 = gtk_button_new();
		gtk_action_connect_proxy(gtk_action_group_get_action(yui->action_group, "pause"), but3);
		gtk_box_pack_start(GTK_BOX(yv->hbox), but3, FALSE, FALSE, 2);

		but4 = gtk_button_new_from_stock("gtk-close");
		g_signal_connect_swapped(but4, "clicked", G_CALLBACK(yui_mem_destroy), dialog);
		gtk_box_pack_start(GTK_BOX(yv->hbox), but4, FALSE, FALSE, 2);
	}
	yv->paused_handler = g_signal_connect_swapped(yui, "paused", G_CALLBACK(yui_mem_update), yv);
	yv->running_handler = g_signal_connect_swapped(yui, "running", G_CALLBACK(yui_mem_clear), yv);

	if ((yui->state & (YUI_IS_RUNNING | YUI_IS_INIT)) == YUI_IS_INIT)
		yui_mem_update(yv);

	gtk_widget_show_all(GTK_WIDGET(yv));

	return dialog;
}

void yui_mem_destroy(YuiMem * ym) {
	g_signal_handler_disconnect(yui, ym->running_handler);
	g_signal_handler_disconnect(yui, ym->paused_handler);

	gtk_widget_destroy(GTK_WIDGET(ym));
}

static void yui_mem_clear(YuiMem * vdp1) {
}

static void yui_mem_address_changed(GtkWidget * w, YuiMem * ym) {
	sscanf(gtk_entry_get_text(GTK_ENTRY(w)), "%x", &ym->address);
	yui_mem_update(ym);
}

static void yui_mem_combo_changed(GtkWidget * w, YuiMem * ym) {

  gint i = gtk_combo_box_get_active( GTK_COMBO_BOX(w) );
  
  if ( i>0 ) {
    ym->address = quickAddress[i-1].address;
    yui_mem_update(ym);
  }
}

static gint hexaDigitToInt( gchar c ) {

  if (( c >= '0' )&&( c <= '9' )) return c-'0';
  if (( c >= 'a' )&&( c <= 'f' )) return c-'a' + 0xA;
  if (( c >= 'A' )&&( c <= 'F' )) return c-'A' + 0xA;
  return -1;
}

static void yui_mem_pageup_pressed(GtkWidget *w, gpointer func, gpointer data, gpointer data2, YuiMem *ym) {

  ym->address -= 2*ym->wLine;
  yui_mem_update(ym);
}

static void yui_mem_pagedown_pressed(GtkWidget *w, gpointer func, gpointer data, gpointer data2, YuiMem *ym) {

  ym->address += 2*ym->wLine;
  yui_mem_update(ym);
}

static void yui_mem_content_changed( GtkCellRendererText *cellrenderertext,
		      gchar *arg1,
		      gchar *arg2,
		      YuiMem *ym) {
  /* dump line <arg1> has been modified - new content is <arg2> */

  GtkTreeIter iter;
  gint i = atoi(arg1);
  gint j,k;
  gchar *curs;
  u32 addr = ym->address + i*ym->wLine;

  gtk_tree_model_get_iter_from_string( GTK_TREE_MODEL( ym->store ), &iter, arg1 );
  
  /* check the format : wLine*2 hexa digits */
  for ( curs = arg2, j=0 ; *curs ; curs++ )
    if ( hexaDigitToInt( *curs ) != -1 ) j++;

  if ( j != ym->wLine * 2 ) return;

  /* convert */
  for ( curs = arg2, k=-1 ; *curs ; curs++ ) { 

    if ( hexaDigitToInt( *curs )!=-1 ) {

      if ( k==-1 ) k = hexaDigitToInt( *curs );
      else { MappedMemoryWriteByte( addr++, 16*k + hexaDigitToInt( *curs ) ); k = -1;
      }
    }
  }
  yui_window_invalidate(GTK_WIDGET(ym), yui);
}

static void yui_mem_update(YuiMem * ym) {
	int i, j;
	GtkTreeIter iter;
	char address[10];
	char dump[30];

	gtk_list_store_clear(ym->store);

	for(i = 0;i < 6;i++) {
		sprintf(address, "%08x", ym->address + (8 * i));
		for(j = 0;j < 8;j++) {
			sprintf(dump + (j * 3), "%02x ", MappedMemoryReadByte(ym->address + (8 * i) + j));
		}

		gtk_list_store_append(ym->store, &iter);
		gtk_list_store_set(GTK_LIST_STORE(ym->store ), &iter, 0, address, 1, dump, -1);
	}
	sprintf( address, "%08X", ym->address );
	gtk_entry_set_text( GTK_ENTRY(ym->entry), address );
	gtk_combo_box_set_active( GTK_COMBO_BOX(ym->quickCombo), 0 );
}
