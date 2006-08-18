/*  Copyright 2006 Guillaume Duhamel
    Copyright 2006 Fabien Coulon

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

#include "gtkglwidget.h"
#include <gtk/gtkgl.h>

static void yui_gl_class_init	(YuiGlClass * klass);
static void yui_gl_init		(YuiGl      * yfe);
static gboolean yui_gl_resize   (GtkWidget *w,GdkEventConfigure *event, gpointer data);

int yui_gl_draw(YuiGl * glxarea) {
	GdkGLContext *glcontext = gtk_widget_get_gl_context (GTK_WIDGET(glxarea));
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (GTK_WIDGET(glxarea));

	if (!gdk_gl_drawable_make_current (gldrawable, glcontext)) {
		g_print("Cannot set gl drawable current\n");
		return;
	}

	gdk_gl_drawable_swap_buffers(gldrawable);
}

int yui_gl_draw_pause(YuiGl * glxarea) {
	if (glxarea->pixels) {
		glDrawPixels(glxarea->pixels_width, glxarea->pixels_height, GL_RGB, GL_UNSIGNED_BYTE, glxarea->pixels);
		yui_gl_draw(glxarea);
	} else {
		gdk_draw_rectangle(GTK_WIDGET(glxarea)->window, GTK_WIDGET(glxarea)->style->bg_gc[GTK_WIDGET_STATE (glxarea)],
				TRUE, 0, 0, GTK_WIDGET(glxarea)->allocation.width, GTK_WIDGET(glxarea)->allocation.height);
	}
}

static gboolean yui_gl_resize(GtkWidget *w,GdkEventConfigure *event, gpointer data) {
	GdkGLContext *glcontext = gtk_widget_get_gl_context (w);
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (w);

	if (!gdk_gl_drawable_gl_begin (gldrawable, glcontext))
		return FALSE;

	glViewport(0, 0, event->width, event->height);

	return FALSE;
}

GtkWidget * yui_gl_new(void) {
	GtkWidget * drawingArea;
	int attribs[] = {
		GDK_GL_RGBA,
		GDK_GL_RED_SIZE,   1,
		GDK_GL_GREEN_SIZE, 1,
		GDK_GL_BLUE_SIZE,  1,

		GDK_GL_DOUBLEBUFFER,

		GDK_GL_DEPTH_SIZE ,1,

		GDK_GL_ATTRIB_LIST_NONE 
	};

	drawingArea = GTK_WIDGET(g_object_new(yui_gl_get_type(), NULL));

	gtk_widget_set_gl_capability(drawingArea, gdk_gl_config_new(attribs), NULL, TRUE, GDK_GL_RGBA_TYPE);

	g_signal_connect (GTK_OBJECT(drawingArea),"configure_event", GTK_SIGNAL_FUNC(yui_gl_resize),0);

	return drawingArea;
}

void yui_gl_dump_screen(YuiGl * glxarea) {
	int size;

	glxarea->pixels_width = GTK_WIDGET(glxarea)->allocation.width;
	glxarea->pixels_height = GTK_WIDGET(glxarea)->allocation.height;
	glxarea->pixels_rowstride = glxarea->pixels_width * 3;
	glxarea->pixels_rowstride += (glxarea->pixels_rowstride % 4)? (4 - (glxarea->pixels_rowstride % 4)): 0;

        size = glxarea->pixels_rowstride * glxarea->pixels_height;
 
	if (glxarea->pixels) free(glxarea->pixels);
        glxarea->pixels = malloc(sizeof(GLubyte) * size);
        if (glxarea->pixels == NULL) return;    

        glReadPixels(0, 0, glxarea->pixels_width, glxarea->pixels_height, GL_RGB, GL_UNSIGNED_BYTE, glxarea->pixels);
}

void yui_gl_screenshot(YuiGl * glxarea) {
	GdkPixbuf * pixbuf, * correct;
       
	yui_gl_dump_screen(glxarea);
	pixbuf = gdk_pixbuf_new_from_data(glxarea->pixels, GDK_COLORSPACE_RGB, FALSE, 8,
			glxarea->pixels_width, glxarea->pixels_height, glxarea->pixels_rowstride, NULL, NULL);
	correct = gdk_pixbuf_flip(pixbuf, FALSE);

	gdk_pixbuf_save(correct, "screenshot.png", "png", NULL, NULL);

	g_object_unref(pixbuf);
	g_object_unref(correct);
}

GType yui_gl_get_type (void) {
  static GType yfe_type = 0;

  if (!yfe_type)
    {
      static const GTypeInfo yfe_info =
      {
	sizeof (YuiGlClass),
	NULL, /* base_init */
        NULL, /* base_finalize */
	(GClassInitFunc) yui_gl_class_init,
        NULL, /* class_finalize */
	NULL, /* class_data */
        sizeof (YuiGl),
	0,
	(GInstanceInitFunc) yui_gl_init,
      };

      yfe_type = g_type_register_static(GTK_TYPE_DRAWING_AREA, "YuiGl", &yfe_info, 0);
    }

  return yfe_type;
}

static void yui_gl_class_init (YuiGlClass * klass) {
}

static void yui_gl_init (YuiGl * y) {
	y->pixels = NULL;
}
