#ifndef YUI_RESOLUTION_H
#define YUI_RESOLUTION_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtktable.h>

G_BEGIN_DECLS

#define YUI_RESOLUTION_TYPE            (yui_resolution_get_type ())
#define YUI_RESOLUTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), YUI_RESOLUTION_TYPE, YuiResolution))
#define YUI_RESOLUTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), YUI_RESOLUTION_TYPE, YuiResolutionClass))
#define IS_YUI_RESOLUTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), YUI_RESOLUTION_TYPE))
#define IS_YUI_RESOLUTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), YUI_RESOLUTION_TYPE))


typedef struct _YuiResolution       YuiResolution;
typedef struct _YuiResolutionClass  YuiResolutionClass;

struct _YuiResolution
{
	GtkTable table;

	GtkWidget * entry_w;
	GtkWidget * entry_h;
	GtkWidget * keep_ratio;

	GKeyFile * keyfile;
	gchar * group;
};

struct _YuiResolutionClass {
	GtkTableClass parent_class;

	void (* yui_resolution) (YuiResolution *yie);
};

GType          yui_resolution_get_type        (void);
GtkWidget*     yui_resolution_new             (GKeyFile * keyfile, const gchar * group);

G_END_DECLS

#endif /* YUI_RESOLUTION_H */
