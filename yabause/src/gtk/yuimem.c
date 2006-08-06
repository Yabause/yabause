#include <gtk/gtk.h>

#include "yuimem.h"
#include "settings.h"

static void yui_mem_class_init	(YuiMemClass * klass);
static void yui_mem_init	(YuiMem      * yfe);
static void yui_mem_clear(YuiMem * vdp1);

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
	GtkWidget * entry;
	GtkWidget * combo;
	GtkWidget * view;
	GtkWidget * scroll;
	GtkListStore * store;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	gtk_window_set_title(GTK_WINDOW(yv), "Memory dump");

	yv->vbox = gtk_vbox_new(FALSE, 2);
	gtk_container_set_border_width(GTK_CONTAINER(yv->vbox), 4);
	gtk_box_set_spacing(GTK_BOX(yv->vbox), 4);
	gtk_container_add(GTK_CONTAINER(yv), yv->vbox);

	entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(yv->vbox), entry, FALSE, FALSE, 0);

	combo = gtk_combo_box_new();
	gtk_box_pack_start(GTK_BOX(yv->vbox), combo, FALSE, FALSE, 0);

	store = gtk_list_store_new(1, G_TYPE_STRING);
	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL (store));

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Address", renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW (view), column);
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Dump", renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW (view), column);
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scroll), view);
	gtk_box_pack_start(GTK_BOX(yv->vbox), scroll, TRUE, TRUE, 0);

	g_signal_connect(G_OBJECT(yv), "delete-event", GTK_SIGNAL_FUNC(yui_mem_destroy), NULL);

	yv->hbox = gtk_hbutton_box_new();
	gtk_box_set_spacing(GTK_BOX(yv->hbox), 4);
	gtk_box_pack_start(GTK_BOX(yv->vbox ), yv->hbox, FALSE, FALSE, 4);
}

static gulong paused_handler;
static gulong running_handler;
static YuiWindow * yui;

GtkWidget * yui_mem_new(YuiWindow * y) {
	GtkWidget * dialog;
	YuiMem * yv;

	yui = y;

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
	paused_handler = g_signal_connect_swapped(yui, "paused", G_CALLBACK(yui_mem_update), yv);
	running_handler = g_signal_connect_swapped(yui, "running", G_CALLBACK(yui_mem_clear), yv);

	if ((yui->state & (YUI_IS_RUNNING | YUI_IS_INIT)) == YUI_IS_INIT)
		yui_mem_update(yv);

	gtk_widget_show_all(GTK_WIDGET(yv));

	return dialog;
}

void yui_mem_update(YuiMem * vdp1) {
}

void yui_mem_destroy(YuiMem * vdp1) {
	g_signal_handler_disconnect(yui, running_handler);
	g_signal_handler_disconnect(yui, paused_handler);

	gtk_widget_destroy(GTK_WIDGET(vdp1));
}

static void yui_mem_clear(YuiMem * vdp1) {
}
