#ifndef YUI_MEM_H
#define YUI_MEM_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtktable.h>

#include "yuiwindow.h"

G_BEGIN_DECLS

#define YUI_MEM_TYPE            (yui_mem_get_type ())
#define YUI_MEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), YUI_MEM_TYPE, YuiMem))
#define YUI_MEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  YUI_MEM_TYPE, YuiMemClass))
#define IS_YUI_MEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), YUI_MEM_TYPE))
#define IS_YUI_MEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  YUI_MEM_TYPE))

typedef struct _YuiMem       YuiMem;
typedef struct _YuiMemClass  YuiMemClass;

struct _YuiMem
{
  GtkWindow dialog;

  GtkWidget * vbox;
  GtkWidget * hbox;

  GtkListStore * store;

  guint32 address;
};

struct _YuiMemClass
{
  GtkWindowClass parent_class;

  void (* yui_mem) (YuiMem * yv);
};

GType		yui_mem_get_type(void);
GtkWidget *	yui_mem_new	(YuiWindow * yui);
void		yui_mem_fill	(YuiMem * vdp1);
void		yui_mem_destroy	(YuiMem * vdp1);

G_END_DECLS

#endif
