#include <gtk/gtk.h>

#include "guiutils.h"

#include "jc.h"

#include "images/js_con0.xpm"
#include "images/js_con1.xpm"


void StatusBarSetJSIcon(
	status_bar_struct *sb, u_int8_t **icon_data
);

void StatusBarSetJSState(status_bar_struct *sb, gint state);

void StatusBarSetMesg(status_bar_struct *sb, const gchar *mesg);
gint StatusBarInit(status_bar_struct *sb, GtkWidget *parent);
void StatusBarDestroy(status_bar_struct *sb);


/* Height of status bar in pixels */
#define SB_DEF_HEIGHT		26

/* Size of status bar's joystick state icon in pixels */
#define SB_JS_ICON_WIDTH	16
#define SB_JS_ICON_HEIGHT	16


/*
 *	Changes the icon displayed on the status bar with
 *	the given xpm data.
 */
void StatusBarSetJSIcon(
	status_bar_struct *sb, u_int8_t **icon_data
)
{
	GdkGC *gc;
	GdkWindow *window;
	GdkPixmap *gdk_pixmap;
	GdkBitmap *mask;
	GtkWidget *w, *pixmap;
	GtkStyle *style;
	gint x, y, width, height, width2, height2;
	jc_struct *jc = jc_core;


	if((jc == NULL) || (sb == NULL) || (icon_data == NULL))
	    return;

	/* Get toplevel's GdkWindow */
	w = jc->toplevel;
	if((w != NULL) ? GTK_WIDGET_NO_WINDOW(w) : 1)
	    return;
	else
	    window = w->window;
	if(window == NULL)
	    return;

	/* Get toplevel's style */
	style = gtk_widget_get_style(w);
	if(style == NULL)
	    style = gtk_widget_get_default_style();
	if(style == NULL)
	    return;

	/* Get background GC from style */
	gc = style->black_gc;


	/* Get GtkFixed as the parent for the icon */
	w = sb->js_icon_fixed;
	if(w == NULL)
	    return;


	/* Begin creating new icon */

	/* Create GdkPixmap and GdkBitmap pair */
	gdk_pixmap = gdk_pixmap_create_from_xpm_d(
	    window, &mask,
	    &style->bg[GTK_STATE_NORMAL],
	    (gchar **)icon_data
	);
	if(gdk_pixmap == NULL)
	    return;

	/* Create GtkPixmap from the GdkPixmap and GdkBitmap pair */
	pixmap = gtk_pixmap_new(gdk_pixmap, mask);
	if(pixmap == NULL)
	{
	    gdk_pixmap_unref(gdk_pixmap);
	    if(mask != NULL)
		gdk_bitmap_unref(mask);
	    return;
	}

	/* Get size of the pixmap */
	gdk_window_get_size((GdkWindow *)gdk_pixmap, &width, &height);
	width2 = w->allocation.width;
	height2 = w->allocation.height;
	x = (width2 / 2) - (width / 2);
	y = (height2 / 2) - (height / 2);

	/* Adjust size of the GtkFixed widget to fit for the pixmap */
/*	gtk_widget_set_usize(w, width2, height2); */

	/* Put pixmap into fixed widget */
	gtk_fixed_put(GTK_FIXED(w), pixmap, x, y);
	gtk_widget_shape_combine_mask(w, mask, x, y);
	gtk_widget_show(pixmap);

	/* Unref the GdkPixmap and GdkBitmap */
	gdk_pixmap_unref(gdk_pixmap);
	gdk_pixmap = NULL;
	if(mask != NULL)
	{
	    gdk_bitmap_unref(mask);
	    mask = NULL;
	}

	/* Destroy the old GtkPixmap */
	if(sb->js_icon_pixmap != NULL)
	    gtk_widget_destroy(sb->js_icon_pixmap);
	/* Record new GtkPixmap */
	sb->js_icon_pixmap = pixmap;
}

/*
 *	Sets the joystick `initialized' state.
 */
void StatusBarSetJSState(status_bar_struct *sb, gint state)
{
	if(sb == NULL)
	    return;

	switch(state)
	{
	  case 1:
	    StatusBarSetJSIcon(sb, (u_int8_t **)js_con1_xpm);
	    break;

	  default:
	    StatusBarSetJSIcon(sb, (u_int8_t **)js_con0_xpm);
	    break;
	}
}

/*
 *	Sets a new message on the given status bar.
 *	If mesg is null, the message is then cleared.
 */
void StatusBarSetMesg(status_bar_struct *sb, const gchar *mesg)
{
	GtkWidget *w;


	if(sb == NULL)
	    return;

	w = sb->mesg_label;
	if(w == NULL)
	    return;

	gtk_label_set_text(
	    GTK_LABEL(w),
	    ((mesg == NULL) ? "" : mesg)
	);
}

/*
 *	Creates widgest for the given status bar and parent
 *	it to the specified parent assumed to be a vbox.
 */
gint StatusBarInit(status_bar_struct *sb, GtkWidget *parent)
{
	gint border_minor = 2;
	GtkWidget *w, *parent2, *parent3;


	if((sb == NULL) || (parent == NULL))
	    return(-1);

	/* Record parent as the status bar's toplevel widget */
	sb->toplevel = parent;

	/* Main table */
	w = gtk_table_new(1, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(parent), w, FALSE, FALSE, 0);
	gtk_widget_show(w);
	parent2 = w;

	/* Outer frame goes in main table */
	w = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(w), GTK_SHADOW_OUT);
	gtk_widget_set_usize(w, -1, SB_DEF_HEIGHT);
	gtk_table_attach(
	    GTK_TABLE(parent2), w,
	    0, 2,
	    0, 1,
	    GTK_FILL | GTK_SHRINK | GTK_EXPAND,
	    GTK_FILL | GTK_SHRINK | GTK_EXPAND,
	    0, 0
	);
	gtk_widget_show(w);


	/* Joystick icon fixed widget */
	w = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(w), GTK_SHADOW_IN);
	gtk_table_attach(
	    GTK_TABLE(parent2), w,
	    0, 1,
	    0, 1,
	    GTK_SHRINK | GTK_FILL,
	    GTK_SHRINK | GTK_FILL,
	    border_minor, border_minor
	);
	gtk_widget_show(w);
	parent3 = w;

	sb->js_icon_fixed = w = gtk_fixed_new();
	sb->js_icon_pixmap = NULL;
	gtk_widget_set_usize(w, SB_JS_ICON_WIDTH, SB_JS_ICON_HEIGHT);
	gtk_container_add(GTK_CONTAINER(parent3), w);
	gtk_widget_show(w);


	/* Label and its frame */
	w = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(w), GTK_SHADOW_IN);
	gtk_table_attach(
	    GTK_TABLE(parent2), w,
	    1, 2,
	    0, 1,
	    GTK_FILL | GTK_SHRINK | GTK_EXPAND,
	    GTK_FILL | GTK_SHRINK | GTK_EXPAND,
	    border_minor, border_minor
	);
	gtk_widget_show(w);

	parent3 = w;

	/* Put a hbox inside frame */
	w = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(parent3), w);
	gtk_widget_show(w);
	parent3 = w;

	sb->mesg_label = w = gtk_label_new("Ready");
	gtk_box_pack_start(GTK_BOX(parent3), w, FALSE, FALSE, border_minor);
	gtk_label_set_justify(GTK_LABEL(w), GTK_JUSTIFY_LEFT);
	gtk_widget_show(w);


	return(0);
}

/*
 *	Deletes the Status Bar.
 */
void StatusBarDestroy(status_bar_struct *sb)
{
	if(sb == NULL)
	    return;

	GTK_WIDGET_DESTROY(sb->js_icon_pixmap);
	sb->js_icon_pixmap = NULL;

	/* Do not destroy the toplevel widget, it belongs to the Status
	 * Bar's parent
	 */
	sb->toplevel = NULL;
}
