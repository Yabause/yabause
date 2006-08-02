#ifndef YUI_SH_H
#define YUI_SH_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtktable.h>

#include "../sh2core.h"
#include "yuiwindow.h"

G_BEGIN_DECLS

#define YUI_SH_TYPE            (yui_sh_get_type ())
#define YUI_SH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), YUI_SH_TYPE, YuiSh))
#define YUI_SH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  YUI_SH_TYPE, YuiShClass))
#define IS_YUI_SH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), YUI_SH_TYPE))
#define IS_YUI_SH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  YUI_SH_TYPE))

typedef struct _YuiSh       YuiSh;
typedef struct _YuiShClass  YuiShClass;

struct _YuiSh
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
  SH2_struct *debugsh;  
  gboolean bMaster;
  gboolean breakpointEnabled;
  gulong paused_handler;
  gulong running_handler;
};

struct _YuiShClass
{
  GtkWindowClass parent_class;

  void (* yui_sh) (YuiSh * yv);
};

GType		yui_sh_get_type       (void);
GtkWidget * yui_msh_new(YuiWindow * y); 
GtkWidget * yui_ssh_new(YuiWindow * y);
void		yui_sh_fill		(YuiSh * sh);
void		yui_sh_update		(YuiSh * sh);
void		yui_sh_destroy	(YuiSh * sh);

G_END_DECLS

#endif
