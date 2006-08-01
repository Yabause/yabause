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

#define YUI_IS_INIT	1
#define YUI_IS_RUNNING	2

struct _YuiWindow {
	GtkWindow hbox;

	GtkWidget * box;
	GtkWidget * menu;
	GtkWidget * area;
	GtkWidget * log;

	YuiAction * actions;
	gulong clean_handler;
	GCallback init_func;
	gpointer init_data;
	GSourceFunc run_func;
	GCallback reset_func;

	guint state;

	GtkActionGroup * action_group;
};

struct _YuiWindowClass {
	GtkWindowClass parent_class;

	void (* yui_window_running) (YuiWindow * yw);
	void (* yui_window_paused) (YuiWindow * yw);
};

GType		yui_window_get_type	(void);
GtkWidget *	yui_window_new		(YuiAction * act, GCallback ifunc, gpointer idata,
					GSourceFunc rfunc, GCallback resetfunc);
void		yui_window_toggle_fullscreen(GtkWidget * w, YuiWindow * yui);
void		yui_window_update	(YuiWindow * yui);
void		yui_window_log		(YuiWindow * yui, const char * message);
void		yui_window_show_log	(YuiWindow * yui);
void            yui_popup               (YuiWindow *w, gchar* text, GtkMessageType mType );
void		yui_window_start	(GtkWidget * w, YuiWindow * yui);
void		yui_window_run		(GtkWidget * w, YuiWindow * yui);
void		yui_window_pause	(GtkWidget * w, YuiWindow * yui);
void		yui_window_reset	(GtkWidget * w, YuiWindow * yui);
void            yui_window_invalidate(GtkWidget * w, YuiWindow * yui );

G_END_DECLS

#endif /* YUI_WINDOW_H */
