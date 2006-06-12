#ifndef YUI_WINDOW_H
#define YUI_WINDOW_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtkwindow.h>

G_BEGIN_DECLS

#define YUI_WINDOW_TYPE            (yui_window_get_type ())
#define YUI_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), YUI_WINDOW_TYPE, YuiWindow))
#define YUI_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  YUI_WINDOW_TYPE, YuiWindowClass))
#define IS_YUI_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), YUI_WINDOW_TYPE))
#define IS_YUI_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  YUI_WINDOW_TYPE))

typedef struct _YuiAction       YuiAction;
typedef struct _YuiWindow       YuiWindow;
typedef struct _YuiWindowClass  YuiWindowClass;

struct _YuiAction {
	guint key;
	const char * name;
	void (*press)(void);
	void (*release)(void);
};

struct _YuiWindow {
	GtkWindow hbox;

	GtkWidget * box;
	GtkWidget * menu;
	GtkWidget * area;

	YuiAction * actions;
};

struct _YuiWindowClass {
	GtkWindowClass parent_class;

	void (* yui_window) (YuiWindow * yw);
};

GType		yui_window_get_type       (void);
GtkWidget *	yui_window_new            (YuiAction * act);
void		yui_window_toggle_fullscreen(YuiWindow * yui);

G_END_DECLS

#endif /* YUI_WINDOW_H */
