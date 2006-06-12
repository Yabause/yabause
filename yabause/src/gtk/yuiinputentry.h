#ifndef YUI_INPUT_ENTRY_H
#define YUI_INPUT_ENTRY_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtktable.h>

G_BEGIN_DECLS

#define YUI_INPUT_ENTRY_TYPE            (yui_input_entry_get_type ())
#define YUI_INPUT_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), YUI_INPUT_ENTRY_TYPE, YuiInputEntry))
#define YUI_INPUT_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), YUI_INPUT_ENTRY_TYPE, YuiInputEntryClass))
#define IS_YUI_INPUT_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), YUI_INPUT_ENTRY_TYPE))
#define IS_YUI_INPUT_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), YUI_INPUT_ENTRY_TYPE))


typedef struct _YuiInputEntry       YuiInputEntry;
typedef struct _YuiInputEntryClass  YuiInputEntryClass;

struct _YuiInputEntry
{
	GtkTable table;

	GKeyFile * keyfile;
	gchar * group;
};

struct _YuiInputEntryClass {
	GtkTableClass parent_class;

	void (* yui_input_entry) (YuiInputEntry *yie);
};

GType          yui_input_entry_get_type        (void);
GtkWidget*     yui_input_entry_new             (GKeyFile * keyfile, const gchar * group, const gchar * keys[]);
void           yui_input_entry_clear           (YuiInputEntry *yie);

G_END_DECLS

#endif /* __YUI_INPUT_ENTRY_H__ */
