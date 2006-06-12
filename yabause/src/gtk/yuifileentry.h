#ifndef YUI_FILE_ENTRY_H
#define YUI_FILE_ENTRY_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtktable.h>

G_BEGIN_DECLS

#define YUI_FILE_ENTRY_TYPE            (yui_file_entry_get_type ())
#define YUI_FILE_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), YUI_FILE_ENTRY_TYPE, YuiFileEntry))
#define YUI_FILE_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  YUI_FILE_ENTRY_TYPE, YuiFileEntryClass))
#define IS_YUI_FILE_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), YUI_FILE_ENTRY_TYPE))
#define IS_YUI_FILE_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  YUI_FILE_ENTRY_TYPE))


typedef struct _YuiFileEntry       YuiFileEntry;
typedef struct _YuiFileEntryClass  YuiFileEntryClass;

struct _YuiFileEntry
{
  GtkHBox hbox;

  GtkWidget * entry;
  GtkWidget * button;

  GKeyFile * keyfile;
  gchar * group;
  gchar * key;
};

struct _YuiFileEntryClass
{
  GtkHBoxClass parent_class;

  void (* yui_file_entry) (YuiFileEntry * yfe);
};

GType          yui_file_entry_get_type        (void);
GtkWidget*     yui_file_entry_new             (GKeyFile *, const gchar *, const gchar *);

G_END_DECLS

#endif /* YUI_FILE_ENTRY_H */
