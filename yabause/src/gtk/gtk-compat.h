#ifndef YUI_GTK_COMPAT_H
#define YUI_GTK_COMPAT_H

#include <gtk/gtk.h>

#if ((GLIB_MAJOR_VERSION < 2) || ((GLIB_MAJOR_VERSION == 2) && (GLIB_MINOR_VERSION < 8)))
gboolean g_file_set_contents(const gchar *, const gchar *, gssize, GError **);
#endif

#endif
