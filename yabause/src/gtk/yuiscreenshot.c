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

#include "yuiscreenshot.h"
#include "gtkglwidget.h"

static void yui_screenshot_class_init	(YuiScreenshotClass * klass);
static void yui_screenshot_init		(YuiScreenshot      * yfe);
static gboolean yui_screenshot_expose(GtkWidget *widget, GdkEventExpose *event, gpointer data);
static void yui_screenshot_update	(YuiScreenshot	* ys, gpointer data);
static void yui_screenshot_save		(GtkWidget * widget, gpointer data);

GType yui_screenshot_get_type (void) {
  static GType yfe_type = 0;

  if (!yfe_type)
    {
      static const GTypeInfo yfe_info =
      {
	sizeof (YuiScreenshotClass),
	NULL, /* base_init */
        NULL, /* base_finalize */
	(GClassInitFunc) yui_screenshot_class_init,
        NULL, /* class_finalize */
	NULL, /* class_data */
        sizeof (YuiScreenshot),
	0,
	(GInstanceInitFunc) yui_screenshot_init,
      };

      yfe_type = g_type_register_static(GTK_TYPE_WINDOW, "YuiScreenshot", &yfe_info, 0);
    }

  return yfe_type;
}

static void yui_screenshot_class_init (YuiScreenshotClass * klass) {
}

static YuiWindow * yui;

static void yui_screenshot_init (YuiScreenshot * yv) {
	GtkWidget * box;
	GtkWidget * button_box;
	GtkWidget * button;

	gtk_window_set_title(GTK_WINDOW(yv), "Screenshot");
	gtk_container_set_border_width(GTK_CONTAINER(yv), 4);

	box = gtk_vbox_new(FALSE, 4);
	gtk_container_add(GTK_CONTAINER(yv), box);

	yv->image = gtk_drawing_area_new();
	gtk_box_pack_start(GTK_BOX(box), yv->image, FALSE, FALSE, 0);
	gtk_widget_set_size_request(GTK_WIDGET(yv->image), 320, 224);

	button_box = gtk_hbutton_box_new();
	gtk_box_pack_start(GTK_BOX(box), button_box, FALSE, FALSE, 0);

	button = gtk_button_new_from_stock(GTK_STOCK_REFRESH);
	gtk_box_pack_start(GTK_BOX(button_box), button, FALSE, FALSE, 0);
	g_signal_connect_swapped(button, "clicked", G_CALLBACK(yui_screenshot_update), yv);

	button = gtk_button_new_from_stock(GTK_STOCK_SAVE);
	gtk_box_pack_start(GTK_BOX(button_box), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(yui_screenshot_save), NULL);

	button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	gtk_box_pack_start(GTK_BOX(button_box), button, FALSE, FALSE, 0);
	g_signal_connect_swapped(button, "clicked", G_CALLBACK(gtk_widget_destroy), yv);

	g_signal_connect(yv->image, "expose_event", G_CALLBACK(yui_screenshot_expose), yv);
}

GtkWidget * yui_screenshot_new(YuiWindow * y) {
	GtkWidget * dialog;
	YuiScreenshot * yv;

	yui = y;

	dialog = GTK_WIDGET(g_object_new(yui_screenshot_get_type(), NULL));
	yv = YUI_SCREENSHOT(dialog);

	gtk_widget_show_all(dialog);
       
	yui_gl_dump_screen(YUI_GL(yui->area));

	return dialog;
}

static gboolean yui_screenshot_expose(GtkWidget *widget, GdkEventExpose *event, gpointer data) {
	GdkPixbuf * pixbuf, * correct;

	pixbuf = gdk_pixbuf_new_from_data(YUI_GL(yui->area)->pixels, GDK_COLORSPACE_RGB, FALSE, 8,
			YUI_GL(yui->area)->pixels_width, YUI_GL(yui->area)->pixels_height, YUI_GL(yui->area)->pixels_rowstride, NULL, NULL);
	correct = gdk_pixbuf_flip(pixbuf, FALSE);

	gdk_draw_pixbuf(GTK_WIDGET(YUI_SCREENSHOT(data)->image)->window, NULL, correct, 0, 0, 0, 0,
			  YUI_GL(yui->area)->pixels_width, YUI_GL(yui->area)->pixels_height, GDK_RGB_DITHER_NONE, 0, 0);

	g_object_unref(pixbuf);
	g_object_unref(correct);

	return TRUE;
}

static void yui_screenshot_update(YuiScreenshot	* ys, gpointer data) {
	yui_gl_dump_screen(YUI_GL(yui->area));
	gtk_widget_queue_draw_area(ys->image, 0, 0, 320, 224);
}

static void yui_screenshot_save(GtkWidget * widget, gpointer data) {
	GdkPixbuf * pixbuf, * correct;
        GtkWidget * file_selector;
        gint result;
	char * filename;

        file_selector = gtk_file_chooser_dialog_new ("Please choose a file", NULL, GTK_FILE_CHOOSER_ACTION_SAVE,
                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);

        gtk_widget_show(file_selector);

        result = gtk_dialog_run(GTK_DIALOG(file_selector));

        switch(result) {
                case GTK_RESPONSE_ACCEPT:
			filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_selector));
			pixbuf = gdk_pixbuf_new_from_data(YUI_GL(yui->area)->pixels, GDK_COLORSPACE_RGB, FALSE, 8,
				YUI_GL(yui->area)->pixels_width, YUI_GL(yui->area)->pixels_height, YUI_GL(yui->area)->pixels_rowstride, NULL, NULL);
			correct = gdk_pixbuf_flip(pixbuf, FALSE);

			gdk_pixbuf_save(correct, filename, "png", NULL, NULL);

			g_object_unref(pixbuf);
			g_object_unref(correct);
                        break;
                case GTK_RESPONSE_CANCEL:
                        break;
        }

        gtk_widget_destroy(file_selector);
}
