#ifndef YUI_TRANSFERT_H
#define YUI_TRANSFERT_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtktable.h>

#include "yuiwindow.h"

G_BEGIN_DECLS

#define YUI_TRANSFERT_TYPE            (yui_transfer_get_type ())
#define YUI_TRANSFERT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), YUI_TRANSFERT_TYPE, YuiTransfer))
#define YUI_TRANSFERT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  YUI_TRANSFERT_TYPE, YuiTransferClass))
#define IS_YUI_TRANSFERT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), YUI_TRANSFERT_TYPE))
#define IS_YUI_TRANSFERT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  YUI_TRANSFERT_TYPE))

#define YUI_TRANSFER_LOAD	1
#define YUI_TRANSFER_LOAD_EXEC	2
#define YUI_TRANSFER_STORE	3

typedef struct _YuiTransfer       YuiTransfer;
typedef struct _YuiTransferClass  YuiTransferClass;

struct _YuiTransfer
{
  GtkWindow window;

  GtkWidget * file_entry;
  GtkWidget * from_entry;
  GtkWidget * to_label;
  GtkWidget * to_entry;
  GtkWidget * transfer_button;

  int mode;
};

struct _YuiTransferClass
{
  GtkWindowClass parent_class;

  void (* yui_transfer) (YuiTransfer * yfe);
};

GType          yui_transfer_get_type        (void);
GtkWidget*     yui_transfer_new             (YuiWindow * yw);

G_END_DECLS

#endif
