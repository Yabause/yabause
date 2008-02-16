#include <jsw.h>
#include <gtk/gtk.h>

#include "guiutils.h"
#include "jc.h"

#include "config.h"
#include "helpmesgs.h"

#include "images/jscalibrator.xpm"
#include "images/icon_calibrate_20x20.xpm"


gint JCCreateRepresentativeLayoutWidgets(
	jc_struct *jc, layout_rep_struct *lr
);
void JCDestroyRepresentativeLayoutWidgets(
	jc_struct *jc, layout_rep_struct *lr
);

gint JCCreateLogicalLayoutWidgets(
	jc_struct *jc, layout_logical_struct *ll,
	GtkWidget *parent       /* Hbox inside notebook */
);
void JCUpdateLogicalLayoutWidgets(
	jc_struct *jc, layout_logical_struct *ll
);
void JCDestroyLogicalLayoutWidgets(
	jc_struct *jc, layout_logical_struct *ll
);
gint JCCreateButtonsList(jc_struct *jc);
void JCDestroyButtonsList(jc_struct *jc);

void JCUpateDeviceComboList(jc_struct *jc);
GtkWidget *JCCreateMenuBar(jc_struct *jc);
gint JCCreateWidgets(jc_struct *jc, gint argc, gchar **argv);
void JCDestroyWidgets(jc_struct *jc);


#define ATOI(s)         (((s) != NULL) ? atoi(s) : 0)
#define ATOL(s)         (((s) != NULL) ? atol(s) : 0)
#define ATOF(s)         (((s) != NULL) ? atof(s) : 0.0f)
#define STRDUP(s)       (((s) != NULL) ? g_strdup(s) : NULL)

#define MAX(a,b)        (((a) > (b)) ? (a) : (b))
#define MIN(a,b)        (((a) < (b)) ? (a) : (b))
#define CLIP(a,l,h)     (MIN(MAX((a),(l)),(h)))
#define STRLEN(s)       (((s) != NULL) ? strlen(s) : 0)
#define STRISEMPTY(s)   (((s) != NULL) ? (*(s) == '\0') : TRUE)


/*
 *	Create widgets for the representative layout.
 */
gint JCCreateRepresentativeLayoutWidgets(
	jc_struct *jc, layout_rep_struct *lr
)
{
	const gint border_minor = 2;
	gint width, height;
	GdkColor *c;
	GdkColormap *colormap;
	GdkPixmap *pixmap;
	GtkWidget *w, *parent, *parent2, *parent3;

	if((jc == NULL) || (lr == NULL))
	    return(-1);

	/* Representative axis layout parent must be valid */
	if(jc->layout_rep_parent == NULL)
	    return(-1);

	/* Get the representative axis layout client parent and create
	 * it as needed
	 */
	w = jc->layout_rep_parent_client;
	if(w == NULL)
	{
	    jc->layout_rep_parent_client = w = gtk_hbox_new(TRUE, 0);
	    gtk_box_pack_start(
		GTK_BOX(jc->layout_rep_parent), w,
		TRUE, TRUE, 0
	    );
	    gtk_widget_show(w);
	}
	/* Record client parent on representative layout */
	lr->toplevel = parent = w;


	/* Get JC toplevel */
	w = jc->toplevel;

	/* Get colormap and begin creating colors */
	colormap = jc->colormap;

	c = &lr->c_axis_bg;
	c->red = 0xffff;
	c->green = 0xffff;
	c->blue = 0xffff;
	gdk_colormap_alloc_color(colormap, c, TRUE, TRUE);

	c = &lr->c_axis_fg;
	c->red = 0x0000;
	c->green = 0x0000;
	c->blue = 0x0000;
	gdk_colormap_alloc_color(colormap, c, TRUE, TRUE);

	c = &lr->c_axis_grid;
	c->red = 0x8000;
	c->green = 0x8000;
	c->blue = 0x8000;
	gdk_colormap_alloc_color(colormap, c, TRUE, TRUE);

	c = &lr->c_axis_hat;
	c->red = 0x0000;
	c->green = 0x0000;
	c->blue = 0x0000;
	gdk_colormap_alloc_color(colormap, c, TRUE, TRUE);

	c = &lr->c_throttle_fg;
	c->red = 0xffff;
	c->green = 0x0000;
	c->blue = 0x0000;
	gdk_colormap_alloc_color(colormap, c, TRUE, TRUE);


	/* Begin creating widgets */

	/* Table to hold label and axis drawing area widgets */
	w = gtk_table_new(4, 3, FALSE);
	gtk_box_pack_start(GTK_BOX(parent), w, FALSE, FALSE, 0);
	gtk_widget_show(w);
	parent2 = w;

	/* Instructions label */
	lr->instructions_label = w = gtk_label_new(
	    HELP_MESG_REPRESENTATIVE
	);
	gtk_table_attach(
	    GTK_TABLE(parent2), w,
	    0, 1,
	    0, 4,
	    GTK_SHRINK,
	    GTK_SHRINK,
	    border_minor, border_minor
	);
	gtk_label_set_justify(GTK_LABEL(w), GTK_JUSTIFY_LEFT);
	gtk_widget_show(w);

	/* Calibrate toggle */
	lr->calib_toggle = w = GUIToggleButtonPixmapLabelH(
	    (guint8 **)icon_calibrate_20x20_xpm,
	    "Calibrate",
	    NULL
	);
	gtk_table_attach(
	    GTK_TABLE(parent2), w,
	    1, 2,
	    0, 1,
	    GTK_SHRINK,
	    GTK_SHRINK,
	    border_minor, border_minor
	);
	gtk_widget_set_usize(
	    w,
	    JC_DEF_CALIBRATE_TOGGLE_WIDTH, JC_DEF_CALIBRATE_TOGGLE_HEIGHT
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "clicked",
	    GTK_SIGNAL_FUNC(JCCalibToggleCB),
	    (gpointer)jc
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "enter_notify_event",
	    GTK_SIGNAL_FUNC(JCEnterNotifyEventCB),
	    (gpointer)jc
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "leave_notify_event",
	    GTK_SIGNAL_FUNC(JCLeaveNotifyEventCB),
	    (gpointer)jc
	);
	gtk_widget_show(w);  


	/* Hat axises */
	/* Frame */
	w = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(w), GTK_SHADOW_IN);
	gtk_table_attach(
	    GTK_TABLE(parent2), w,
	    1, 2,
	    1, 2,
	    GTK_SHRINK,
	    GTK_SHRINK,
	    2, 2
	);
	gtk_widget_show(w);
	parent3 = w;
	/* Drawing area */
	width = 50;
	height = 50;
	lr->hat_da = w = gtk_drawing_area_new();
	gtk_widget_set_events(
	    w,
	    GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK |
	    GDK_BUTTON_RELEASE_MASK
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "expose_event",
	    GTK_SIGNAL_FUNC(JCExposeEventCB), (gpointer)jc
	);
	gtk_drawing_area_size(GTK_DRAWING_AREA(w), width, height);
	gtk_container_add(GTK_CONTAINER(parent3), w);
	gtk_widget_show(w);
	/* GdkPixmap drawing buffer */
	lr->hat_buf = pixmap = GDK_PIXMAP_NEW(width, height);


	/* First two (x and y) axises */
	/* Frame */
	w = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(w), GTK_SHADOW_IN);
	gtk_table_attach(
	    GTK_TABLE(parent2), w,
	    1, 2,
	    2, 3,
	    GTK_SHRINK,
	    GTK_SHRINK,
	    border_minor, border_minor
	);
	gtk_widget_show(w);
	parent3 = w;
	/* Drawing area */
	width = 100;
	height = 100;
	lr->axis_da = w = gtk_drawing_area_new();
	gtk_widget_set_events(
	    w,
	    GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK |
	    GDK_BUTTON_RELEASE_MASK
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "expose_event",
	    GTK_SIGNAL_FUNC(JCExposeEventCB), (gpointer)jc
	);
	gtk_drawing_area_size(GTK_DRAWING_AREA(w), width, height);
	gtk_container_add(GTK_CONTAINER(parent3), w);
	gtk_widget_show(w);

	lr->axis_buf = pixmap = GDK_PIXMAP_NEW(width, height);


	/* Throttle axis */
	/* Frame */
	w = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(w), GTK_SHADOW_IN);
	gtk_table_attach(
	    GTK_TABLE(parent2), w,
	    2, 3,
	    2, 3,
	    GTK_SHRINK,
	    GTK_SHRINK,
	    border_minor, border_minor
	);
	gtk_widget_show(w);
	parent3 = w;
	/* Drawing area */
	width = 15;
	height = 100;
	lr->throttle_da = w = gtk_drawing_area_new();
	gtk_widget_set_events(
	    w,
	    GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK |
	    GDK_BUTTON_RELEASE_MASK
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "expose_event",
	    GTK_SIGNAL_FUNC(JCExposeEventCB), (gpointer)jc
	);
	gtk_drawing_area_size(GTK_DRAWING_AREA(w), width, height);
	gtk_container_add(GTK_CONTAINER(parent3), w);
	gtk_widget_show(w);   

	lr->throttle_buf = pixmap = GDK_PIXMAP_NEW(width, height);


	/* Rotate axis */
	/* Frame */
	w = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(w), GTK_SHADOW_IN);
	gtk_table_attach(
	    GTK_TABLE(parent2), w,
	    1, 2,
	    3, 4,
	    GTK_SHRINK,
	    GTK_SHRINK,
	    border_minor, border_minor
	);
	gtk_widget_show(w);
	parent3 = w;
	/* Drawing area */
	width = 100;
	height = 15;
	lr->rotate_da = w = gtk_drawing_area_new();
	gtk_widget_set_events(
	    w,
	    GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK |
	    GDK_BUTTON_RELEASE_MASK
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "expose_event",
	    GTK_SIGNAL_FUNC(JCExposeEventCB), (gpointer)jc
	);
	gtk_drawing_area_size(GTK_DRAWING_AREA(w), width, height);
	gtk_container_add(GTK_CONTAINER(parent3), w);
	gtk_widget_show(w);

	lr->rotate_buf = pixmap = GDK_PIXMAP_NEW(width, height);


	return(0);
}

/*
 *	Destroys all widgets and deallocates all resources for the given
 *	representative layout structure but not the structure itself.
 *
 *	Structure will be reset.
 */
void JCDestroyRepresentativeLayoutWidgets(
	jc_struct *jc, layout_rep_struct *lr
)
{
	GdkColormap *colormap;

	if((jc == NULL) || (lr == NULL))
	    return;

	colormap = jc->colormap;

	GTK_WIDGET_DESTROY(lr->axis_da);
	GTK_WIDGET_DESTROY(lr->throttle_da);
	GTK_WIDGET_DESTROY(lr->rotate_da);
	GTK_WIDGET_DESTROY(lr->hat_da);
	GTK_WIDGET_DESTROY(lr->calib_toggle);
	GTK_WIDGET_DESTROY(lr->instructions_label);
	GTK_WIDGET_DESTROY(lr->toplevel);
	jc->layout_rep_parent_client = NULL;	/* Mark rep toplevel as being destroyed */

	GDK_COLORMAP_FREE_COLOR(colormap, &lr->c_axis_bg);
	GDK_COLORMAP_FREE_COLOR(colormap, &lr->c_axis_fg);
	GDK_COLORMAP_FREE_COLOR(colormap, &lr->c_axis_grid);
	GDK_COLORMAP_FREE_COLOR(colormap, &lr->c_axis_hat);
	GDK_COLORMAP_FREE_COLOR(colormap, &lr->c_throttle_fg);

	GDK_PIXMAP_UNREF(lr->rotate_buf);
	GDK_PIXMAP_UNREF(lr->throttle_buf);
	GDK_PIXMAP_UNREF(lr->axis_buf);
	GDK_PIXMAP_UNREF(lr->hat_buf);

	memset(lr, 0x00, sizeof(layout_rep_struct));
}


/*
 *	Create widgets for logical layout.
 */
gint JCCreateLogicalLayoutWidgets(
	jc_struct *jc, layout_logical_struct *ll,
	GtkWidget *parent	/* Hbox inside notebook */
)
{
	GtkWidget *w, *parent2, *parent3;
	GtkObject *adj;
	GdkColor *c;
	GdkColormap *colormap;


	if((jc == NULL) || (ll == NULL) || (parent == NULL))
	    return(-1);

	/* Get colormap */
	colormap = jc->colormap;

	/* Record values on layout logical structure */
	ll->toplevel = parent;

	ll->axis = NULL;
	ll->total_axises = 0;


	/* Create each axis layout widget group */
	ll->total_axises = jc->jsd.total_axises;
	if(ll->total_axises > 0)
	{
	    gint i, width, height;
	    gchar *s;
	    js_axis_struct *axis;
	    layout_logical_axis_struct *ll_axis;

	    /* Allocate pointers to axis layout structures */
	    ll->axis = (layout_logical_axis_struct **)g_malloc0(
	        ll->total_axises * sizeof(layout_logical_axis_struct *)
	    );
	    if(ll->axis == NULL)
	    {
	        ll->total_axises = 0;
	        return(-1);
	    }

	    /* Create each new logical layout axis */
	    for(i = 0; i < ll->total_axises; i++)
	    {
		/* Allocate a new structure */
		ll->axis[i] = ll_axis = (layout_logical_axis_struct *)g_malloc0(
		    sizeof(layout_logical_axis_struct)
		);
		if(ll_axis == NULL)
		    continue;

		/* Get pointer to axis on joystick data */
		if(JSIsAxisAllocated(&jc->jsd, i))
		    axis = jc->jsd.axis[i];
		else
		    axis = NULL;

		/* Begin creating widgets, marking ll_axis as not
		 * initialized so callbacks know its not done 
		 * intiializing
		 */
		ll_axis->initialized = FALSE;

		/* Create colors */
		c = &ll_axis->c_axis_bg;
		GDK_COLOR_SET_COEFF(c, 1.0f, 1.0f, 1.0f);
		GDK_COLORMAP_ALLOC_COLOR(colormap, c);
		c = &ll_axis->c_axis_fg;
		GDK_COLOR_SET_COEFF(c, 0.0f, 0.0f, 0.0f);
		GDK_COLORMAP_ALLOC_COLOR(colormap, c);
		c = &ll_axis->c_axis_grid;
		GDK_COLOR_SET_COEFF(c, 0.5f, 0.5f, 0.5f);
		GDK_COLORMAP_ALLOC_COLOR(colormap, c);
		c = &ll_axis->c_axis_nz;
		GDK_COLOR_SET_COEFF(c, 0.75f, 0.75f, 0.75f);
		GDK_COLORMAP_ALLOC_COLOR(colormap, c);

		/* Font */
		ll_axis->font = gdk_font_load(
"-adobe-helvetica-medium-r-normal-*-10-*-*-*-*-*-iso8859-1"
		);
		if(ll_axis->font == NULL)
		    ll_axis->font = gdk_font_load(
"-*-fixed-*-*-*-*-10-*-*-*-*-*-*-*"
		    );

		/* Main hbox (toplevel), parent for this axis's widgets */
		ll_axis->toplevel = w = gtk_hbox_new(FALSE, 2);
		gtk_box_pack_start(GTK_BOX(parent), w, FALSE, FALSE, 0);
		gtk_widget_show(w);
		parent2 = w;


		/* Axis number label */
		s = g_strdup_printf("Axis %i:", i);
		ll_axis->axis_num_label = w = gtk_label_new(s);
		g_free(s);
		gtk_box_pack_start(GTK_BOX(parent2), w, FALSE, FALSE, 0);
		gtk_widget_show(w);


		/* Vbox for axis gauge, to align it centered */
		w = gtk_vbox_new(TRUE, 0);
		gtk_box_pack_start(GTK_BOX(parent2), w, FALSE, FALSE, 0);
		gtk_widget_show(w);
		parent3 = w;
		/* Frame */
		w = gtk_frame_new(NULL);
		gtk_frame_set_shadow_type(GTK_FRAME(w), GTK_SHADOW_IN);
		gtk_box_pack_start(GTK_BOX(parent3), w, TRUE, FALSE, 0);
		gtk_widget_show(w);
		parent3 = w;
		/* Drawing area */
		width = 100;
		height = 18;
		ll_axis->position_da = w = gtk_drawing_area_new();
		gtk_widget_set_events(
		    w,
		    GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK |
		    GDK_BUTTON_RELEASE_MASK
		);
		gtk_signal_connect(
		    GTK_OBJECT(w), "expose_event",
		    GTK_SIGNAL_FUNC(JCExposeEventCB), (gpointer)jc
		);
		gtk_signal_connect(
		    GTK_OBJECT(w), "button_press_event",
		    GTK_SIGNAL_FUNC(JCButtonPressEventCB), (gpointer)jc
		);
		gtk_signal_connect(
		    GTK_OBJECT(w), "button_release_event",
		    GTK_SIGNAL_FUNC(JCButtonReleaseEventCB), (gpointer)jc
		);
		gtk_drawing_area_size(GTK_DRAWING_AREA(w), width, height);
		gtk_container_add(GTK_CONTAINER(parent3), w);
		gtk_widget_show(w);
		/* GdkPixmap buffer */
		ll_axis->position_buf = GDK_PIXMAP_NEW(width, height);


		/* Calibrate toggle */
		ll_axis->calib_toggle = w = GUIToggleButtonPixmap(
		    (guint8 **)icon_calibrate_20x20_xpm
		);
		gtk_box_pack_start(GTK_BOX(parent2), w, FALSE, FALSE, 0);
		gtk_signal_connect(
		    GTK_OBJECT(w), "clicked",
		    GTK_SIGNAL_FUNC(JCCalibToggleCB), (gpointer)jc
		);
		gtk_signal_connect(
		    GTK_OBJECT(w), "enter_notify_event",
		    GTK_SIGNAL_FUNC(JCEnterNotifyEventCB), (gpointer)jc
		);
		gtk_signal_connect(
		    GTK_OBJECT(w), "leave_notify_event",
		    GTK_SIGNAL_FUNC(JCLeaveNotifyEventCB), (gpointer)jc
		);
		gtk_widget_show(w);


		/* Null zone spin */
		w = gtk_label_new("NZ:");
		gtk_box_pack_start(GTK_BOX(parent2), w, FALSE, FALSE, 0);
		gtk_widget_show(w);

		ll_axis->nz_adj = adj = gtk_adjustment_new(
		    (axis != NULL) ? axis->nz : 0.0,
		    0.0,
		    1000000.0,	/* One million should be a good upper bound */
		    1.0, 5.0, 1.0	/* Step inc, page inc, and page size */
		);
		gtk_object_ref(adj);	/* Keep a refcount for ourself */
		ll_axis->nz_spin = w = gtk_spin_button_new(
		    (GtkAdjustment *)adj,
		    1.0,		/* Climb rate (acceleration) */
		    0			/* Must be whole numbers */
		);
		gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(w), FALSE);
		gtk_widget_set_usize(w, 55, -1);
		gtk_box_pack_start(GTK_BOX(parent2), w, FALSE, FALSE, 0);
		gtk_signal_connect(
		    GTK_OBJECT(w), "changed",
		    GTK_SIGNAL_FUNC(JCEditableChangedCB), (gpointer)jc
		);
		gtk_signal_connect(
		    GTK_OBJECT(w), "enter_notify_event",
		    GTK_SIGNAL_FUNC(JCEnterNotifyEventCB), (gpointer)jc
		);
		gtk_signal_connect(
		    GTK_OBJECT(w), "leave_notify_event",
		    GTK_SIGNAL_FUNC(JCLeaveNotifyEventCB), (gpointer)jc
		);
		gtk_widget_show(w);


		/* Correction level spin */
		w = gtk_label_new("CL:");
		gtk_box_pack_start(GTK_BOX(parent2), w, FALSE, FALSE, 0);
		gtk_widget_show(w);

		ll_axis->correction_level_adj = adj = gtk_adjustment_new(
		    (axis != NULL) ? axis->correction_level : 0.0,
		    0.0,
		    100.0,	/* One hundred should be a good upper bound */
		    1.0, 1.0, 1.0       /* Step inc, page inc, and page size */
		);
		gtk_object_ref(adj);    /* Keep a refcount for ourself */
		ll_axis->correction_level_spin = w = gtk_spin_button_new(
		    (GtkAdjustment *)adj,
		    1.0,                /* Climb rate (acceleration) */
		    0                   /* Must be whole numbers */
		);
		gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(w), FALSE);
		gtk_widget_set_usize(w, 40, -1);
		gtk_box_pack_start(GTK_BOX(parent2), w, FALSE, FALSE, 0);
		gtk_signal_connect(
		    GTK_OBJECT(w), "changed",
		    GTK_SIGNAL_FUNC(JCEditableChangedCB), (gpointer)jc
		);
		gtk_signal_connect(
		    GTK_OBJECT(w), "enter_notify_event",
		    GTK_SIGNAL_FUNC(JCEnterNotifyEventCB),
		    (gpointer)jc
		);
		gtk_signal_connect(
		    GTK_OBJECT(w), "leave_notify_event",
		    GTK_SIGNAL_FUNC(JCLeaveNotifyEventCB),
		    (gpointer)jc
		);
		gtk_widget_show(w);


		/* Dead zone min and max spins */
		w = gtk_label_new("DZ:");
		gtk_box_pack_start(GTK_BOX(parent2), w, FALSE, FALSE, 0);
		gtk_widget_show(w);

		ll_axis->dz_min_adj = adj = gtk_adjustment_new(
		    (axis != NULL) ? axis->dz_min : 0.0,
		    -1000000.0, 10000000.0,	/* Big bounds */
		    1.0, 10.0, 1.0	/* Step inc, page inc, and page size */
		);
		gtk_object_ref(adj);    /* Keep a refcount for ourself */
		ll_axis->dz_min_spin = w = gtk_spin_button_new(
		    (GtkAdjustment *)adj,
		    1.0,                /* Climb rate (acceleration) */
		    0                   /* Must be whole numbers */
		);
		gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(w), FALSE);
		gtk_widget_set_usize(w, 55, -1);
		gtk_box_pack_start(GTK_BOX(parent2), w, FALSE, FALSE, 0);
		gtk_signal_connect(
		    GTK_OBJECT(w), "changed",
		    GTK_SIGNAL_FUNC(JCEditableChangedCB), (gpointer)jc
		);
		gtk_signal_connect(
		    GTK_OBJECT(w), "enter_notify_event",
		    GTK_SIGNAL_FUNC(JCEnterNotifyEventCB), (gpointer)jc
		);
		gtk_signal_connect(
		    GTK_OBJECT(w), "leave_notify_event",
		    GTK_SIGNAL_FUNC(JCLeaveNotifyEventCB), (gpointer)jc
		);
		gtk_widget_show(w);

		ll_axis->dz_max_adj = adj = gtk_adjustment_new(
		    (axis != NULL) ? axis->dz_max : 0.0,
		    -1000000.0, 10000000.0,     /* Big bounds */
		    1.0, 10.0, 1.0      /* Step inc, page inc, and page size */
		);
		gtk_object_ref(adj);    /* Keep a refcount for ourself */
		ll_axis->dz_max_spin = w = gtk_spin_button_new(
		    (GtkAdjustment *)adj,
		    1.0,                /* Climb rate (acceleration) */
		    0                   /* Must be whole numbers */
		);
		gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(w), FALSE);
		gtk_widget_set_usize(w, 55, -1);
		gtk_box_pack_start(GTK_BOX(parent2), w, FALSE, FALSE, 0);
		gtk_signal_connect(
		    GTK_OBJECT(w), "changed",
		    GTK_SIGNAL_FUNC(JCEditableChangedCB), (gpointer)jc
		);
		gtk_signal_connect(
		    GTK_OBJECT(w), "enter_notify_event",
		    GTK_SIGNAL_FUNC(JCEnterNotifyEventCB), (gpointer)jc
		);
		gtk_signal_connect(
		    GTK_OBJECT(w), "leave_notify_event",
		    GTK_SIGNAL_FUNC(JCLeaveNotifyEventCB), (gpointer)jc
		);
		gtk_widget_show(w);


		/* Correctional coefficient min and max spins */
		w = gtk_label_new("CC:");
		gtk_box_pack_start(GTK_BOX(parent2), w, FALSE, FALSE, 0);
		gtk_widget_show(w);

		ll_axis->corr_coeff_min1_adj = adj = gtk_adjustment_new(
		    (axis != NULL) ? axis->corr_coeff_min1 : 0.0,
		    0.0, 1.0,
		    0.01, 0.05, 0.01	/* Step inc, page inc, and page size */
		);
		gtk_object_ref(adj);    /* Keep a refcount for ourself */
		ll_axis->corr_coeff_min1_spin = w = gtk_spin_button_new(
		    (GtkAdjustment *)adj,
		    1.0,                /* Climb rate (acceleration) */
		    2                   /* Two decimals */
		);
		gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(w), FALSE);
		gtk_widget_set_usize(w, 50, -1);
		gtk_box_pack_start(GTK_BOX(parent2), w, FALSE, FALSE, 0);
		gtk_signal_connect(
		    GTK_OBJECT(w), "changed",
		    GTK_SIGNAL_FUNC(JCEditableChangedCB), (gpointer)jc
		);
		gtk_signal_connect(
		    GTK_OBJECT(w), "enter_notify_event",
		    GTK_SIGNAL_FUNC(JCEnterNotifyEventCB), (gpointer)jc
		);
		gtk_signal_connect(
		    GTK_OBJECT(w), "leave_notify_event",
		    GTK_SIGNAL_FUNC(JCLeaveNotifyEventCB), (gpointer)jc
		);
		gtk_widget_show(w);

		ll_axis->corr_coeff_max1_adj = adj = gtk_adjustment_new(
		    (axis != NULL) ? axis->corr_coeff_max1 : 0.0,
		    0.0, 1.0,
		    0.01, 0.05, 0.01    /* Step inc, page inc, and page size */
		);
		gtk_object_ref(adj);    /* Keep a refcount for ourself */
		ll_axis->corr_coeff_max1_spin = w = gtk_spin_button_new(
		    (GtkAdjustment *)adj,
		    1.0,                /* Climb rate (acceleration) */
		    2                   /* Two decimals */
		);
		gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(w), FALSE);
		gtk_widget_set_usize(w, 50, -1);
		gtk_box_pack_start(GTK_BOX(parent2), w, FALSE, FALSE, 0);
		gtk_signal_connect(
		    GTK_OBJECT(w), "changed",
		    GTK_SIGNAL_FUNC(JCEditableChangedCB), (gpointer)jc
		);
		gtk_signal_connect(
		    GTK_OBJECT(w), "enter_notify_event",
		    GTK_SIGNAL_FUNC(JCEnterNotifyEventCB), (gpointer)jc
		);
		gtk_signal_connect(
		    GTK_OBJECT(w), "leave_notify_event",
		    GTK_SIGNAL_FUNC(JCLeaveNotifyEventCB), (gpointer)jc
		);
		gtk_widget_show(w);


		/* Flip axis values check */
		ll_axis->flipped_check = w = gtk_check_button_new_with_label(
		    "Flip"
		);
		gtk_box_pack_start(GTK_BOX(parent2), w, FALSE, FALSE, 0);
		if(axis != NULL)
		    gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(w),
			(axis->flags & JSAxisFlagFlipped) ? TRUE : FALSE
		    );
		gtk_signal_connect(
		    GTK_OBJECT(w), "clicked",
		    GTK_SIGNAL_FUNC(JCCalibFlipCheckCB), (gpointer)jc
		);
		gtk_signal_connect(
		    GTK_OBJECT(w), "enter_notify_event",
		    GTK_SIGNAL_FUNC(JCEnterNotifyEventCB), (gpointer)jc
		);
		gtk_signal_connect(
		    GTK_OBJECT(w), "leave_notify_event",
		    GTK_SIGNAL_FUNC(JCLeaveNotifyEventCB), (gpointer)jc
		);
		gtk_widget_show(w);


		/* Is hat check */
		ll_axis->is_hat_check = w = gtk_check_button_new_with_label(
		    "Hat"
		);
		gtk_box_pack_start(GTK_BOX(parent2), w, FALSE, FALSE, 0);
		if(axis != NULL)
		    gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(w),
			(axis->flags & JSAxisFlagIsHat) ? TRUE : FALSE
		    );
		gtk_signal_connect(
		    GTK_OBJECT(w), "clicked",
		    GTK_SIGNAL_FUNC(JCCalibIsHatCheckCB), (gpointer)jc
		);
		gtk_signal_connect(
		    GTK_OBJECT(w), "enter_notify_event",
		    GTK_SIGNAL_FUNC(JCEnterNotifyEventCB), (gpointer)jc
		);
		gtk_signal_connect(
		    GTK_OBJECT(w), "leave_notify_event",
		    GTK_SIGNAL_FUNC(JCLeaveNotifyEventCB), (gpointer)jc
		);
		gtk_widget_show(w);


		/* Mark axis as initialized after all callbacks have been set */
		ll_axis->initialized = TRUE;

	    }	/* for(i = 0; i < ll->total_axises; i++) */
	}


	return(0);
}

/*
 *	Updates all logical axis layout widgets in the given structure
 *	to fetch new values from the currently opened joystick device.
 */
void JCLogicalLayoutWidgetsGetValues(
	jc_struct *jc, layout_logical_struct *ll
)
{
	gint i;
	layout_logical_axis_struct *ll_axis;
	GtkWidget *w;
	js_axis_struct *axis;


	if((jc == NULL) || (ll == NULL))
	    return;

	/* Itterate through each axis on the logical axis layout
	 * structure and update each axis with values from the jsd
	 * structure's axises.
	 */
	for(i = 0; i < ll->total_axises; i++)
	{
	    ll_axis = ll->axis[i];
	    if(ll_axis == NULL)
		continue;

	    /* Skip if not initialized */
	    if(!ll_axis->initialized)
		continue;

	    /* Get pointer to axis on joystick data */
	    if(JSIsAxisAllocated(&jc->jsd, i))
		axis = jc->jsd.axis[i];
	    else
		continue;

	    /* Begin getting values from axis structure and storing them
	     * on the widgets.
	     */

	    /* Null zone spin */
	    w = ll_axis->nz_spin;
	    if(w != NULL)
		gtk_spin_button_set_value(
		    GTK_SPIN_BUTTON(w),
		    (gfloat)axis->nz
		);

	    /* Is hat check */
	    w = ll_axis->is_hat_check;
	    if(w != NULL)
		gtk_toggle_button_set_active(
		    GTK_TOGGLE_BUTTON(w),
		    (axis->flags & JSAxisFlagIsHat) ? TRUE : FALSE
		);

	    /* Flip axis check */
	    w = ll_axis->flipped_check;
	    if(w != NULL)
		gtk_toggle_button_set_active(
		    GTK_TOGGLE_BUTTON(w),
		    (axis->flags & JSAxisFlagFlipped) ? TRUE : FALSE
		); 

	    /* Error correction level spin */
	    w = ll_axis->correction_level_spin;
	    if(w != NULL)
		gtk_spin_button_set_value(
		    GTK_SPIN_BUTTON(w),
		    (gfloat)axis->correction_level
		);

	    /* Dead zone min spin */
	    w = ll_axis->dz_min_spin;
	    if(w != NULL)
		gtk_spin_button_set_value(
		    GTK_SPIN_BUTTON(w),
		    (gfloat)axis->dz_min
		);

	    /* Dead zone max spin */
	    w = ll_axis->dz_max_spin;
	    if(w != NULL)
		gtk_spin_button_set_value(
		    GTK_SPIN_BUTTON(w),
		    (gfloat)axis->dz_max
		);

	    /* Correctional coefficient min 1 spin */
	    w = ll_axis->corr_coeff_min1_spin;
	    if(w != NULL)
		gtk_spin_button_set_value(
		    GTK_SPIN_BUTTON(w),
		    (gfloat)axis->corr_coeff_min1
		);

	    /* Correctional coefficient max 1 spin */
	    w = ll_axis->corr_coeff_max1_spin;
	    if(w != NULL)
		gtk_spin_button_set_value(
		    GTK_SPIN_BUTTON(w),
		    (gfloat)axis->corr_coeff_max1
		);

	}
}

/*
 *      Destroys all widgets and resources on the given axis logical
 *	layout structure.
 */
void JCDestroyLogicalLayoutWidgets(
	jc_struct *jc, layout_logical_struct *ll
)
{
	gint i;
	GdkColormap *colormap;
	layout_logical_axis_struct *ll_axis;

	if((jc == NULL) || (ll == NULL))
	    return;

	colormap = jc->colormap;

	/* Iterate through each axis */
	for(i = 0; i < ll->total_axises; i++)
	{
	    ll_axis = ll->axis[i];
	    if(ll_axis == NULL)
		continue;

	    GTK_WIDGET_DESTROY(ll_axis->axis_num_label);
	    GTK_WIDGET_DESTROY(ll_axis->position_da);
	    GTK_WIDGET_DESTROY(ll_axis->calib_toggle);
	    GTK_WIDGET_DESTROY(ll_axis->nz_spin);
	    GTK_WIDGET_DESTROY(ll_axis->is_hat_check);
	    GTK_WIDGET_DESTROY(ll_axis->flipped_check);
	    GTK_WIDGET_DESTROY(ll_axis->correction_level_spin);
	    GTK_WIDGET_DESTROY(ll_axis->dz_min_spin);
	    GTK_WIDGET_DESTROY(ll_axis->dz_max_spin);
	    GTK_WIDGET_DESTROY(ll_axis->corr_coeff_min1_spin);
	    GTK_WIDGET_DESTROY(ll_axis->corr_coeff_max1_spin);
	    GTK_WIDGET_DESTROY(ll_axis->toplevel);

	    GDK_PIXMAP_UNREF(ll_axis->position_buf);

	    GDK_FONT_UNREF(ll_axis->font);

	    GDK_COLORMAP_FREE_COLOR(colormap, &ll_axis->c_axis_bg);
	    GDK_COLORMAP_FREE_COLOR(colormap, &ll_axis->c_axis_fg);
	    GDK_COLORMAP_FREE_COLOR(colormap, &ll_axis->c_axis_grid);
	    GDK_COLORMAP_FREE_COLOR(colormap, &ll_axis->c_axis_nz);

	    GTK_OBJECT_UNREF(ll_axis->nz_adj);
	    GTK_OBJECT_UNREF(ll_axis->correction_level_adj);
	    GTK_OBJECT_UNREF(ll_axis->dz_min_adj);
	    GTK_OBJECT_UNREF(ll_axis->dz_max_adj);
	    GTK_OBJECT_UNREF(ll_axis->corr_coeff_min1_adj);
	    GTK_OBJECT_UNREF(ll_axis->corr_coeff_max1_adj);

	    g_free(ll_axis);
	    ll->axis[i] = ll_axis = NULL;
	}
	g_free(ll->axis);
	ll->axis = NULL;
	ll->total_axises = 0;

	memset(ll, 0x00, sizeof(layout_logical_struct));
}


/*
 *	Creates buttons list for the given jc.
 */
gint JCCreateButtonsList(jc_struct *jc)
{
	gint i, n, total;
	GtkWidget *w, *parent;
	buttons_list_struct *bl;

	if(jc == NULL)
	    return(-1);

	bl = &jc->buttons_list;
	if(bl == NULL)
	    return(-1);

	/* Get parent vbox */
	parent = jc->buttons_list_parent;
	bl->toplevel = parent;

	bl->button = NULL;
	bl->total_buttons = 0;

	total = jc->jsd.total_buttons;

	/* Create each button */
	for(i = 0; i < total; i++)
	{
	    /* Allocate a new pointer on the buttons list */
	    n = bl->total_buttons;
	    bl->total_buttons++;
	    bl->button = (GtkWidget **)g_realloc(
		bl->button,
		bl->total_buttons * sizeof(GtkWidget *)
	    );
	    if(bl->button == NULL)
	    {
		bl->total_buttons = 0;
		break;
	    }
	    else
	    {
		/* Create a new button */
		gchar *s = g_strdup_printf("Button %i", i);
		bl->button[n] = w = gtk_button_new_with_label(s);
		g_free(s);
		gtk_button_released(GTK_BUTTON(w));
		gtk_box_pack_start(GTK_BOX(parent), w, FALSE, FALSE, 0);
		gtk_widget_set_usize(
		    w,
		    JC_DEF_JS_BUTTON_WIDTH, JC_DEF_JS_BUTTON_HEIGHT
		);
		gtk_widget_show(w);
	    }
	}


	return(0);
}

/*
 *	Destroys all widgets for the buttons list.
 */
void JCDestroyButtonsList(jc_struct *jc)
{
	gint i;
	buttons_list_struct *bl;
	GtkWidget *w;


	if(jc == NULL)
	    return;

	/* Get buttons list */
	bl = &jc->buttons_list;

	/* Itterate through each button on the buttons list */
	for(i = 0; i < bl->total_buttons; i++)
	{
	    w = bl->button[i];
	    if(w == NULL)
		continue;

	    gtk_widget_destroy(w);
	    bl->button[i] = w = NULL;
	}

	g_free(bl->button);
	bl->button = NULL;
	bl->total_buttons = 0;
}


/*
 *	Updates the joystick device combo list.
 */
void JCUpateDeviceComboList(jc_struct *jc)
{
	const gchar *s;
	gint i, total_jsas;
	js_attribute_struct *jsa, *jsa_ptr;
	GList *glist;
	GtkCombo *combo;

	if(jc == NULL)
	    return;

	combo = (GtkCombo *)jc->js_device_combo;
	if(combo == NULL)
	    return;

	/* Get attributes of all joysticks on system
	 *
	 * The calibration file is passed as NULL since we do not need
	 * details from the calibration file
	 */
	jsa = JSGetAttributesList(&total_jsas, NULL);

	/* Create new list */
	glist = NULL;
	for(i = 0; i < total_jsas; i++)
	{
	    jsa_ptr = &jsa[i];

	    s = jsa_ptr->device_name;
	    if(s != NULL)
		glist = g_list_append(glist, STRDUP(s));
	}

	/* Delete joystick attributes list */
	JSFreeAttributesList(jsa, total_jsas);

	/* Set/transfer the new list */
	GUIComboSetList(GTK_WIDGET(combo), glist);
}

/*
 *	Procedure to create menu bar widget to the given jc.
 */
GtkWidget *JCCreateMenuBar(jc_struct *jc)
{
	static GtkItemFactoryEntry menu_items[] = {

{ "/_Calibration",          NULL,         NULL,     JC_CMD_NONE, "<Branch>" },
{ "/Calibration/_Open",     "<control>O", JCMenuCB, JC_CMD_OPEN_CALIB, NULL },
{ "/Calibration/sep1",      NULL,         NULL,     JC_CMD_NONE, "<Separator>" },
{ "/Calibration/_Save",     "<control>S", JCMenuCB, JC_CMD_SAVE_CALIB, NULL },
{ "/Calibration/Save _As",  "<control>A", JCMenuCB, JC_CMD_SAVEAS_CALIB, NULL },
{ "/Calibration/sep2",      NULL,         NULL,     JC_CMD_NONE, "<Separator>" },
{ "/Calibration/Clean Up",  NULL,         JCMenuCB, JC_CMD_CLEANUP_CALIB, NULL },
{ "/Calibration/sep3",      NULL,         NULL,     JC_CMD_NONE, "<Separator>" },
{ "/Calibration/E_xit",     "<control>X", JCMenuCB, JC_CMD_EXIT, NULL },
{ "/_Joystick",             NULL,         NULL,     JC_CMD_NONE, "<Branch>" },
{ "/Joystick/_Refresh",     "<control>R", JCMenuCB, JC_CMD_REFRESH, NULL },
{ "/Joystick/sep1",         NULL,         NULL,     JC_CMD_NONE, "<Separator>" },
{ "/Joystick/R_eopen",      "<control>E", JCMenuCB, JC_CMD_REOPEN_DEVICE, NULL },
{ "/Joystick/_Close",       "<control>C", JCMenuCB, JC_CMD_CLOSE_DEVICE, NULL },
{ "/Joystick/sep2",         NULL,         NULL,     JC_CMD_NONE, "<Separator>" },
{ "/Joystick/Properties...", NULL,        JCMenuCB, JC_CMD_JOYSTICK_PROPERTIES, NULL },
{ "/_View",                 NULL,         NULL,     JC_CMD_NONE, "<Branch>" },
{ "/View/Re_presentative",  "<control>P", JCMenuCB, JC_CMD_LAYOUT_REPRESENTATIVE, NULL },
{ "/View/_Logical",         "<control>L", JCMenuCB, JC_CMD_LAYOUT_LOGICAL, NULL },
{ "/_Help",                 NULL,         NULL,     JC_CMD_NONE, "<LastBranch>" },
{ "/Help/Index",            NULL,         JCMenuCB, JC_CMD_HELP_INDEX, NULL },
{ "/Help/sep1",             NULL,         NULL,     JC_CMD_NONE, "<Separator>" },
{ "/Help/Introduction",     NULL,         JCMenuCB, JC_CMD_HELP_INTRODUCTION, NULL },
{ "/Help/How To Calibrate", NULL,         JCMenuCB, JC_CMD_HELP_HOW_TO_CALIBRATE, NULL },
{ "/Help/sep2",             NULL,         NULL,     JC_CMD_NONE, "<Separator>" },
{ "/Help/About",            NULL,         JCMenuCB, JC_CMD_HELP_ABOUT, NULL }

	};

	GtkWidget *menubar;
	GtkItemFactory *item_factory;
	GtkAccelGroup *accel_group;
	gint nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);


	if(jc == NULL)
	    return(NULL);


	accel_group = gtk_accel_group_new();

	/* This function initializes the item factory.
	 * Param 1: The type of menu - can be GTK_TYPE_MENU_BAR, GTK_TYPE_MENU,
	 *          or GTK_TYPE_OPTION_MENU.
	 * Param 2: The path of the menu.
	 * Param 3: A pointer to a gtk_accel_group.  The item factory sets up
	 *          the accelerator table while generating menus.
	 */
	item_factory = gtk_item_factory_new(
	    GTK_TYPE_MENU_BAR,
	    "<main>", accel_group
	);

	/* This function generates the menu items. Pass the item factory,
	 * the number of items in the array, the array itself, and any
	 * callback data for the the menu items.
	 */
	gtk_item_factory_create_items(
	    item_factory,
	    nmenu_items, menu_items,
	    jc				/* Callback data */
	);

	/* Attach the new accelerator group to the window */
	gtk_accel_group_attach(accel_group, GTK_OBJECT(jc->toplevel));

	/* Create the menu bar widget using the item factory */
	menubar = gtk_item_factory_get_widget(item_factory, "<main>");

	return(menubar);
}

/*
 *	Procedure to create widgets for the given jc.
 */
gint JCCreateWidgets(jc_struct *jc, gint argc, gchar **argv)
{
	const gint	border_major = 5,
			border_minor = 2;
	gint width, height;
	GdkColor *c;
	GdkColormap *colormap;
	GdkWindow *window;
	GdkGCValues gcv;
	GdkGC *gc;
	GtkWidget *w, *parent, *parent2, *parent3;
	GtkCombo *combo;
	gpointer ptr_rtn;


	if(jc == NULL)
	    return(-1);


	/* Reset values that apply to widgets only, all other values
	 * should already be reset by the calling function
	 */
	jc->layout_state = JC_LAYOUT_REPRESENTATIVE;


	/* Toplevel */
	jc->toplevel = w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_usize(GTK_WIDGET(w), JC_DEF_WIDTH, JC_DEF_HEIGHT);
	gtk_window_set_title(GTK_WINDOW(w), PROG_NAME_FULL);
	gtk_window_set_wmclass(
	    GTK_WINDOW(w), "jscalibrator", PROG_NAME
	);
	gtk_window_set_policy(GTK_WINDOW(w), TRUE, TRUE, TRUE);
	gtk_widget_realize(w);
	window = w->window;
	if(window != NULL)
	{
	    GdkGeometry geo;
	    geo.min_width = 100;
	    geo.min_height = 70;
	    geo.base_width = 0;
	    geo.base_height = 0;
	    geo.width_inc = 1;
	    geo.height_inc = 1;
	    gdk_window_set_geometry_hints(
		window,
		&geo,
		GDK_HINT_MIN_SIZE | GDK_HINT_BASE_SIZE |
		GDK_HINT_RESIZE_INC
	    );
	    gdk_window_set_decorations(
		window,
		GDK_DECOR_BORDER | GDK_DECOR_TITLE | GDK_DECOR_MENU |
		GDK_DECOR_MINIMIZE
	    );
	    gdk_window_set_functions(
		window,
		GDK_FUNC_MOVE | GDK_FUNC_RESIZE |
		GDK_FUNC_MINIMIZE | GDK_FUNC_CLOSE
	    );
	    GUISetWMIcon(window, (guint8 **)jscalibrator_xpm);
	}
	gtk_signal_connect(
	    GTK_OBJECT(w), "delete_event",
	    GTK_SIGNAL_FUNC(JCExitCB), jc
	);
	gtk_container_border_width(GTK_CONTAINER(w), 0);

	/* Get toplevel widget's colormap and increase its refcount
	 * (since we want to keep it around)
	 */
	jc->colormap = colormap = gtk_widget_get_colormap(w);
	if(colormap != NULL)
	    gdk_colormap_ref(colormap);

	/* Create graphics context */
	gcv.foreground.red = 0xffff;
	gcv.foreground.green = 0xffff;
	gcv.foreground.blue = 0xffff;
	gcv.background.red = 0x0000;
	gcv.background.green = 0x0000;
	gcv.background.blue = 0x0000;
	gcv.font = NULL;
	gcv.function = GDK_COPY;
	gcv.fill = GDK_SOLID;
	gcv.tile = NULL;
	gcv.stipple = NULL;
	gcv.clip_mask = NULL;
	gcv.subwindow_mode = GDK_CLIP_BY_CHILDREN;
	gcv.ts_x_origin = 0;
	gcv.ts_y_origin = 0;
	gcv.clip_x_origin = 0;
	gcv.clip_y_origin = 0;
	gcv.graphics_exposures = 0;
	gcv.line_width = 1;
	gcv.line_style = GDK_LINE_SOLID;
	gcv.cap_style = GDK_CAP_BUTT;
	gcv.join_style = GDK_JOIN_MITER;
	jc->gc = gc = gdk_gc_new_with_values(
	    window,
	    &gcv,
	    GDK_GC_FOREGROUND | GDK_GC_BACKGROUND |
/*	    GDK_GC_FONT | */
	    GDK_GC_FUNCTION | GDK_GC_FILL |
/*	    GDK_GC_TILE | GDK_GC_STIPPLE |
	    GDK_GC_CLIP_MASK |
 */
	    GDK_GC_SUBWINDOW |
	    GDK_GC_TS_X_ORIGIN | GDK_GC_TS_Y_ORIGIN |
	    GDK_GC_CLIP_X_ORIGIN | GDK_GC_CLIP_Y_ORIGIN |
	    GDK_GC_EXPOSURES | GDK_GC_LINE_WIDTH |
	    GDK_GC_LINE_STYLE |
	    GDK_GC_CAP_STYLE |
	    GDK_GC_JOIN_STYLE
	);
	if(gc == NULL)
	    return(-3);


	/* Colors */
	w = jc->toplevel;
	c = &jc->c_signal_level_bg;
	c->red = 0x0000;
	c->green = 0x0000;
	c->blue = 0x0000;
	GDK_COLORMAP_ALLOC_COLOR(colormap, c);

	w = jc->toplevel;
	c = &jc->c_signal_level_fg;
	c->red = 0x0000;
	c->green = 0xffff;
	c->blue = 0x0000;
	GDK_COLORMAP_ALLOC_COLOR(colormap, c);


	/* Main vbox */
	parent = jc->toplevel;
	w = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(parent), w);
	gtk_widget_show(w);
	parent = w;

	/* Menu bar */
	jc->menu_bar = w = JCCreateMenuBar(jc);
	gtk_box_pack_start(GTK_BOX(parent), w, FALSE, FALSE, 0);
	gtk_widget_show(w);

	/* Hbox to hold device combo and joystick signal meter drawing
	 * area widgets
	 */
	w = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(parent), w, FALSE, FALSE, 0);
	gtk_widget_show(w);
	parent2 = w;

	/* Device combo */
	w = (GtkWidget *)GUIComboCreate(
	    "Joystick Device:",		/* Label */
	    JSDefaultDevice,		/* Initial value */
	    NULL,			/* Initial glist of items */
	    10,				/* Max items */
	    &ptr_rtn,			/* Combo widget return */
	    jc,				/* Client data */
	    JCJSDeviceEntryCB,		/* Activate callback */
	    NULL			/* List change callback */
	);
	combo = (GtkCombo *)ptr_rtn;
	jc->js_device_combo = (GtkWidget *)combo;
	gtk_box_pack_start(GTK_BOX(parent2), w, TRUE, TRUE, 0);
	GUISetWidgetTip(
	    combo->entry, HELP_TTM_DEVICES_COMBO
	);
	gtk_widget_show(w);

	/* Update combo listing */
	JCUpateDeviceComboList(jc);


	/* Joystick signal meter frame and drawing area */
	w = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(w), GTK_SHADOW_IN);
	gtk_box_pack_start(GTK_BOX(parent2), w, FALSE, FALSE, 0);
	gtk_widget_show(w);
	parent3 = w;

	/* Signal meter drawing area */
	width = JC_SIGNAL_HISTORY_MAX;	/* One pixel per signal tick */
	height = 20;
	w = gtk_drawing_area_new();
	jc->signal_level_da = w;
	gtk_drawing_area_size(GTK_DRAWING_AREA(w), width, height);
	gtk_container_add(GTK_CONTAINER(parent3), w);
	gtk_widget_realize(w);
	gtk_widget_show(w);

	jc->signal_level_pixmap = gdk_pixmap_new(
	    ((jc->toplevel == NULL) ? 0 : jc->toplevel->window),
	    width, height, -1
	);


	/* Horizontal paned widget to contain axises notebook and
	 * buttons viewport
	 */
	w = gtk_hpaned_new();
	gtk_box_pack_start(GTK_BOX(parent), w, TRUE, TRUE, 0);
	gtk_paned_set_position(GTK_PANED(w), 480);
	gtk_paned_set_handle_size(GTK_PANED(w), 10);
	gtk_paned_set_gutter_size(GTK_PANED(w), 12);
	gtk_widget_show(w);
	parent2 = w;


	/* Notebook for axises layouts */
	jc->axises_notebook = w = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(w), GTK_POS_TOP);
	gtk_paned_add1(GTK_PANED(parent2), w);
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(w), TRUE);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(w), TRUE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(w), TRUE);
/*      gtk_notebook_set_page(GTK_NOTEBOOK(w), 0); */
	gtk_signal_connect(
	    GTK_OBJECT(w), "switch_page",
	    GTK_SIGNAL_FUNC(JCSwitchPageCB),
	    (gpointer)jc
	);
	gtk_widget_show(w);


	/* Add vboxes into each page of the notebook */
	/* Representative axises layout vbox */
	jc->layout_rep_parent = w = gtk_vbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(w), border_major);
	gtk_notebook_append_page(
	    GTK_NOTEBOOK(jc->axises_notebook),
	    w,
	    gtk_label_new("Representative")
	);
	gtk_widget_show(w);
	jc->layout_rep_parent_client = NULL;	/* Created when needed */

	/* Logical axises layout viewport and vbox */
	jc->layout_logical_vp = w = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(
	    GTK_SCROLLED_WINDOW(w),
	    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC
	);
	gtk_container_border_width(GTK_CONTAINER(w), border_major);
	gtk_notebook_append_page(  
	    GTK_NOTEBOOK(jc->axises_notebook),
	    w,
	    gtk_label_new("Logical")
	);
	gtk_widget_show(w);
	parent3 = w;
	/* Vbox */
	jc->layout_logical_parent = w = gtk_vbox_new(FALSE, border_minor);
	gtk_container_border_width(GTK_CONTAINER(w), border_major);
	gtk_scrolled_window_add_with_viewport(
	    GTK_SCROLLED_WINDOW(parent3), w
	);
	gtk_widget_show(w);


	/* Implicitly reset axis layout structures */
	memset(&jc->layout_logical, 0x00, sizeof(layout_logical_struct));
	memset(&jc->layout_rep, 0x00, sizeof(layout_rep_struct));


	/* Viewport for buttons list vbox */
	w = gtk_scrolled_window_new(NULL, NULL);
	jc->buttons_list_vp = w;
	gtk_paned_add2(GTK_PANED(parent2), w);
	gtk_scrolled_window_set_policy(
	    GTK_SCROLLED_WINDOW(w),
	    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC
	);
	gtk_widget_show(w);
	parent3 = w;

	/* Button list vbox */
	jc->buttons_list_parent = w = gtk_vbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(w), 0);
	gtk_scrolled_window_add_with_viewport(
	    GTK_SCROLLED_WINDOW(parent3), w
	);
	gtk_widget_show(w);


	/* Status bar */
	StatusBarInit(&jc->status_bar, parent);
	StatusBarSetJSState(&jc->status_bar, 0);


	return(0);
}


/*
 *	Deletes the JSCalibrator.
 */
void JCDestroyWidgets(jc_struct *jc)
{
	GdkColormap *colormap;

	if(jc == NULL)
	    return;

	colormap = jc->colormap;

	/* Destroy axis layout structures and their widgets */
	JCDestroyRepresentativeLayoutWidgets(jc, &jc->layout_rep);
	JCDestroyLogicalLayoutWidgets(jc, &jc->layout_logical);

	/* Destroy buttons list widgets */
	JCDestroyButtonsList(jc);

	/* Destroy status bar */
	StatusBarDestroy(&jc->status_bar);

	GTK_WIDGET_DESTROY(jc->buttons_list_parent);
	GTK_WIDGET_DESTROY(jc->buttons_list_vp);
	GTK_WIDGET_DESTROY(jc->js_device_combo);
	GTK_WIDGET_DESTROY(jc->signal_level_da);
	GTK_WIDGET_DESTROY(jc->axises_notebook);
	GTK_WIDGET_DESTROY(jc->menu_bar);
	GTK_WIDGET_DESTROY(jc->toplevel);

	GDK_PIXMAP_UNREF(jc->signal_level_pixmap);
	GDK_GC_UNREF(jc->gc);
	GDK_COLORMAP_FREE_COLOR(colormap, &jc->c_signal_level_bg);
	GDK_COLORMAP_FREE_COLOR(colormap, &jc->c_signal_level_fg);
	GDK_COLORMAP_UNREF(colormap);

	memset(jc, 0x00, sizeof(jc_struct));
}
