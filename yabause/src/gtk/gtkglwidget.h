#ifndef GTKGLWIDGET_H
#define GTKGLWIDGET_H

#include <gtk/gtk.h>

#include <GL/gl.h>

int draw(GtkWidget *);
int drawPause(GtkWidget *);
GtkWidget * gtk_gl_widget_new(void);

#endif
