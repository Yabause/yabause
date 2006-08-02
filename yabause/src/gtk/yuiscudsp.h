#ifndef YUI_SCUDSP_H
#define YUI_SCUDSP_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtktable.h>

#include "../scu.h"
#include "yuiwindow.h"

G_BEGIN_DECLS

#define YUI_SCUDSP_TYPE            (yui_scudsp_get_type ())
#define YUI_SCUDSP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), YUI_SCUDSP_TYPE, YuiScudsp))
#define YUI_SCUDSP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  YUI_SCUDSP_TYPE, YuiScudspClass))
#define IS_YUI_SCUDSP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), YUI_SCUDSP_TYPE))
#define IS_YUI_SCUDSP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  YUI_SCUDSP_TYPE))

typedef struct _YuiScudsp       YuiScudsp;
typedef struct _YuiScudspClass  YuiScudspClass;

struct _YuiScudsp
{
  GtkWindow dialog;

  GtkWidget *vbox, *vboxmain;
  GtkWidget *hbox, *hboxmain;
  GtkWidget * buttonStep, * buttonStepOver;
  GtkWidget * bpList, *regList, *uLabel, *uFrame;
  GtkListStore *bpListStore, *regListStore;
  GtkCellRenderer *bpListRenderer, *regListRenderer1, *regListRenderer2;
  GtkTreeViewColumn *bpListColumn, *regListColumn1, *regListColumn2;
  u32 cbp[MAX_BREAKPOINTS]; /* the list of breakpoint positions, as they can be found in the list widget */
  u32 lastCode; /* offset of last unassembly. Try to reuse it to prevent sliding. */
  gulong paused_handler;
  gulong running_handler;
};

struct _YuiScudspClass
{
  GtkWindowClass parent_class;

  void (* yui_scudsp) (YuiScudsp * yv);
};

GType		yui_scudsp_get_type       (void);
GtkWidget *     yui_scudsp_new(YuiWindow * y); 
void		yui_scudsp_update(YuiScudsp * scudsp);
void		yui_scudsp_destroy(YuiScudsp * scudsp);

G_END_DECLS

#endif
