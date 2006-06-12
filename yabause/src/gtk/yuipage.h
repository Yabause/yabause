#ifndef YUI_PAGE_H
#define YUI_PAGE_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtktable.h>

G_BEGIN_DECLS

#define YUI_PAGE_TYPE            (yui_page_get_type ())
#define YUI_PAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), YUI_PAGE_TYPE, YuiPage))
#define YUI_PAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  YUI_PAGE_TYPE, YuiPageClass))
#define IS_YUI_PAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), YUI_PAGE_TYPE))
#define IS_YUI_PAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  YUI_PAGE_TYPE))

#define YUI_FILE_SETTING	1
#define YUI_RANGE_SETTING	2
#define YUI_RESOLUTION_SETTING	3

typedef struct _YuiPageItem   YuiPageItem;
typedef struct _YuiPageDesc   YuiPageDesc;
typedef struct _YuiPage       YuiPage;
typedef struct _YuiPageClass  YuiPageClass;

struct _YuiPageItem {
	const gchar * name;
	guint type;
	gpointer data;
};

struct _YuiPageDesc {
	const gchar * name;
	YuiPageItem * items;
};

struct _YuiPage
{
  GtkVBox vbox;
};

struct _YuiPageClass
{
  GtkHBoxClass parent_class;

  void (* yui_page) (YuiPage * yfe);
};

GType          yui_page_get_type        (void);
GtkWidget *    yui_page_new             (GKeyFile * keyfile, const gchar * group, YuiPageDesc * desc);

G_END_DECLS

#endif /* YUI_PAGE_H */
