#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "guiutils.h"

#include "jcjsprops.h"
#include "jc.h"

#include "images/icon_ok_20x20.xpm"
#include "images/icon_select_20x20.xpm"
#include "images/icon_cancel_20x20.xpm"
#include "images/icon_game_controller_32x32.xpm"
#include "images/icon_force_feedback_32x32.xpm"
#include "jscalibrator.xpm"


static gint JCJSPropsDeleteEventCB(
	GtkWidget *widget, GdkEvent *event, gpointer data
);
static void JCJSPropsOKCB(GtkWidget *widget, gpointer data);
static void JCJSPropsApplyCB(GtkWidget *widget, gpointer data);
static void JCJSPropsCancelCB(GtkWidget *widget, gpointer data);
static void JCJSPropsSwitchPageCB(
	GtkNotebook *notebook, GtkNotebookPage *page, guint page_num,
	gpointer data
);

jc_jsprops_struct *JCJSPropsNew(
	gpointer core_ptr		/* jc_struct * */
);
void JCJSPropsGetValues(jc_jsprops_struct *jsp);
void JCJSPropsSetValues(jc_jsprops_struct *jsp);
void JCJSPropsMap(jc_jsprops_struct *jsp);
void JCJSPropsUnmap(jc_jsprops_struct *jsp);
void JCJSPropsDelete(jc_jsprops_struct *jsp);


#define ATOI(s)         (((s) != NULL) ? atoi(s) : 0)
#define ATOL(s)         (((s) != NULL) ? atol(s) : 0)
#define ATOF(s)         (((s) != NULL) ? atof(s) : 0.0f)
#define STRDUP(s)       (((s) != NULL) ? g_strdup(s) : NULL)

#define MAX(a,b)        (((a) > (b)) ? (a) : (b))
#define MIN(a,b)        (((a) < (b)) ? (a) : (b))
#define CLIP(a,l,h)     (MIN(MAX((a),(l)),(h)))
#define STRLEN(s)       (((s) != NULL) ? strlen(s) : 0)
#define STRISEMPTY(s)   (((s) != NULL) ? (*(s) == '\0') : TRUE)


#define JCJSPROPS_BTN_WIDTH	(100 + (2 * 3))
#define JCJSPROPS_BTN_HEIGHT	(30 + (2 * 3))


/*
 *	Widget "delete_event" signal callback.
 */
static gint JCJSPropsDeleteEventCB(
	GtkWidget *widget, GdkEvent *event, gpointer data
)
{
	jc_jsprops_struct *jsp = (jc_jsprops_struct *)data;
	if(jsp == NULL)
	    return(FALSE);

	JCJSPropsUnmap(jsp);

	return(TRUE);
}

/*
 *	OK button callback.
 */
static void JCJSPropsOKCB(GtkWidget *widget, gpointer data)
{
	jc_jsprops_struct *jsp = (jc_jsprops_struct *)data;
	if(jsp == NULL)
	    return;

	JCJSPropsSetValues(jsp);
	JCJSPropsUnmap(jsp);
}

/*
 *	Apply button callback.
 */
static void JCJSPropsApplyCB(GtkWidget *widget, gpointer data)
{
	jc_jsprops_struct *jsp = (jc_jsprops_struct *)data;
	if(jsp == NULL)
	    return;

	JCJSPropsSetValues(jsp);

	/* Need to get values again after setting them because they may
	 * have changed
	 */
	JCJSPropsGetValues(jsp);
}

/*
 *	Cancel button callback.
 */
static void JCJSPropsCancelCB(GtkWidget *widget, gpointer data)
{
	jc_jsprops_struct *jsp = (jc_jsprops_struct *)data;
	if(jsp == NULL)
	    return;

	JCJSPropsUnmap(jsp);
}

/*
 *	GtkNotebook "switch_page" signal callback.
 */
static void JCJSPropsSwitchPageCB(
	GtkNotebook *notebook, GtkNotebookPage *page, guint page_num,
	gpointer data
)
{
	jc_jsprops_struct *jsp = (jc_jsprops_struct *)data;
	if(jsp == NULL)
	    return;

}


/*
 *	Creates a new jc_jsprops_struct structure.
 */
jc_jsprops_struct *JCJSPropsNew(
	gpointer core_ptr               /* jc_struct * */
)
{
	const gint	border_minor = 2,
			border_major = 5;
	gint width, height;
	GdkWindow *window;
	GtkAccelGroup *accelgrp;
	gpointer label_rtn, entry_rtn;
	GtkWidget	*w, *parent, *parent2, *parent3, *parent4,
			*parent5, *parent6;
	jc_jsprops_struct *jsp = (jc_jsprops_struct *)g_malloc0(
	    sizeof(jc_jsprops_struct)
	);
	if(jsp == NULL)
	    return(jsp);

	/* Reset values */
	jsp->initialized = FALSE;
	jsp->map_state = FALSE;
	jsp->core_ptr = core_ptr;

	/* Keyboard accelerator group */
	jsp->accelgrp = accelgrp = gtk_accel_group_new();

	/* Toplevel */
	jsp->toplevel = w = gtk_window_new(GTK_WINDOW_DIALOG);
	gtk_widget_set_usize(GTK_WIDGET(w), 400, 450);
	gtk_window_set_title(GTK_WINDOW(w), "Joystick Properties");
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
	    GTK_SIGNAL_FUNC(JCJSPropsDeleteEventCB), jsp
	);
	gtk_accel_group_attach(accelgrp, GTK_OBJECT(w));
	gtk_container_border_width(GTK_CONTAINER(w), 0);

	/* Main vbox */
	parent = jsp->toplevel;
	w = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(parent), w);
	gtk_widget_show(w);
	parent = w;

	/* Hbox to hold main notebook */
	w = gtk_hbox_new(FALSE, border_minor);
	gtk_container_border_width(GTK_CONTAINER(w), border_major);
	gtk_box_pack_start(GTK_BOX(parent), w, TRUE, TRUE, 0);
	gtk_widget_show(w);
	parent2 = w;

	/* Main notebook */
	jsp->notebook = w = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(w), GTK_POS_TOP);
	gtk_box_pack_start(GTK_BOX(parent2), w, TRUE, TRUE, 0);
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(w), TRUE);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(w), TRUE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(w), TRUE);
/*      gtk_notebook_set_page(GTK_NOTEBOOK(w), 0); */
	gtk_signal_connect(
	    GTK_OBJECT(w), "switch_page",
	    GTK_SIGNAL_FUNC(JCJSPropsSwitchPageCB), (gpointer)jsp
	);
	gtk_widget_show(w);
	parent3 = w;


	/* Begin creating notebook pages */

	/* Begin creating "General" page */
	/* Vbox */
	jsp->general_page_vbox = w = gtk_vbox_new(FALSE, border_major);
	gtk_container_border_width(GTK_CONTAINER(w), border_major);
	gtk_notebook_append_page(
	    GTK_NOTEBOOK(parent3),
	    w,
	    gtk_label_new("General")
	);
	gtk_widget_show(w);
	parent4 = w;

	/* Identity frame */
	w = gtk_frame_new("Identity");
	gtk_frame_set_shadow_type(GTK_FRAME(w), GTK_SHADOW_ETCHED_IN);
	gtk_box_pack_start(GTK_BOX(parent4), w, TRUE, TRUE, 0);
	gtk_widget_show(w);
	parent5 = w;
	/* Hbox */
	w = gtk_hbox_new(FALSE, border_major);
	gtk_container_border_width(GTK_CONTAINER(w), border_major);
	gtk_container_add(GTK_CONTAINER(parent5), w);
	gtk_widget_show(w);
	parent5 = w;

	/* Vbox for icon */
	w = gtk_vbox_new(FALSE, border_minor);
	gtk_box_pack_start(GTK_BOX(parent5), w, FALSE, FALSE, 0);
	gtk_widget_show(w);
	parent6 = w;
	/* Joystick icon */
	jsp->general_icon_fixed = w = (GtkWidget *)GUICreateMenuItemIcon(
	    (guint8 **)icon_game_controller_32x32_xpm
	);
	gtk_widget_set_usize(w, 32, 32);
	gtk_box_pack_start(GTK_BOX(parent6), w, FALSE, FALSE, 0);
	gtk_widget_show(w);
	jsp->general_icon_pm = NULL;

	/* Vbox for entries and other widgets */
	w = gtk_vbox_new(FALSE, border_minor);
	gtk_box_pack_start(GTK_BOX(parent5), w, TRUE, TRUE, 0);
	gtk_widget_show(w);
	parent6 = w;

	/* Descriptive name entry */
	w = GUIPromptBar(
	    NULL, "Name:",
	    &label_rtn, &entry_rtn
	);
	gtk_box_pack_start(GTK_BOX(parent6), w, FALSE, FALSE, 0);
	gtk_widget_show(w);
	jsp->name_entry = w = (GtkWidget *)entry_rtn;

	/* Device entry */
	w = GUIPromptBar(
	    NULL, "Device:",
	    &label_rtn, &entry_rtn
	);
	gtk_box_pack_start(GTK_BOX(parent6), w, FALSE, FALSE, 0);
	gtk_widget_show(w);
	jsp->device_entry = w = (GtkWidget *)entry_rtn;
	gtk_entry_set_editable(GTK_ENTRY(w), FALSE);

	/* Driver version entry */
	w = GUIPromptBar(
	    NULL, "Driver Version:",
	    &label_rtn, &entry_rtn
	);
	gtk_box_pack_start(GTK_BOX(parent6), w, FALSE, FALSE, 0);
	gtk_widget_show(w);
	jsp->driver_version_entry = w = (GtkWidget *)entry_rtn;
	gtk_entry_set_editable(GTK_ENTRY(w), FALSE);

	/* Calibration file entry */
	w = GUIPromptBar(
	    NULL, "Calibration File:",
	    &label_rtn, &entry_rtn
	);
	gtk_box_pack_start(GTK_BOX(parent6), w, FALSE, FALSE, 0);
	gtk_widget_show(w);
	jsp->calibration_file_entry = w = (GtkWidget *)entry_rtn;
	gtk_entry_set_editable(GTK_ENTRY(w), FALSE);


	/* Statistics frame */
	w = gtk_frame_new("Statistics");
	gtk_frame_set_shadow_type(GTK_FRAME(w), GTK_SHADOW_ETCHED_IN);
	gtk_box_pack_start(GTK_BOX(parent4), w, FALSE, FALSE, 0);
	gtk_widget_show(w);
	parent5 = w;
	/* Vbox */
	w = gtk_vbox_new(FALSE, border_major);
	gtk_container_border_width(GTK_CONTAINER(w), border_major);
	gtk_container_add(GTK_CONTAINER(parent5), w);
	gtk_widget_show(w);
	parent5 = w;

	/* Total axises hbox and labels */
	/* Hbox */
	w = gtk_hbox_new(FALSE, border_minor);
	gtk_box_pack_start(GTK_BOX(parent5), w, FALSE, FALSE, 0);
	gtk_widget_show(w);
	parent6 = w;
	/* Parameter label */
	w = gtk_label_new("Total Axises:");
	gtk_box_pack_start(GTK_BOX(parent6), w, FALSE, FALSE, 0);
	gtk_widget_show(w);
	/* Value label */
	jsp->total_axises_label = w = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(parent6), w, FALSE, FALSE, 0);
	gtk_widget_show(w);

	/* Total buttons hbox and labels */
	/* Hbox */
	w = gtk_hbox_new(FALSE, border_minor);
	gtk_box_pack_start(GTK_BOX(parent5), w, FALSE, FALSE, 0);
	gtk_widget_show(w);
	parent6 = w;
	/* Parameter label */
	w = gtk_label_new("Total Buttons:");
	gtk_box_pack_start(GTK_BOX(parent6), w, FALSE, FALSE, 0);
	gtk_widget_show(w);
	/* Value label */
	jsp->total_buttons_label = w = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(parent6), w, FALSE, FALSE, 0);
	gtk_widget_show(w);

	/* Last calibrated hbox and labels */
	/* Hbox */
	w = gtk_hbox_new(FALSE, border_minor);
	gtk_box_pack_start(GTK_BOX(parent5), w, FALSE, FALSE, 0);
	gtk_widget_show(w);
	parent6 = w;
	/* Parameter label */
	w = gtk_label_new("Last Calibrated:");
	gtk_box_pack_start(GTK_BOX(parent6), w, FALSE, FALSE, 0);
	gtk_widget_show(w);
	/* Value label */
	jsp->last_calibrated_label = w = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(parent6), w, FALSE, FALSE, 0);
	gtk_widget_show(w);

	/* Has changes hbox and labels */
	/* Hbox */
	w = gtk_hbox_new(FALSE, border_minor);
	gtk_box_pack_start(GTK_BOX(parent5), w, FALSE, FALSE, 0);
	gtk_widget_show(w);
	parent6 = w;
	/* Parameter label */
	w = gtk_label_new("Has Changes:");
	gtk_box_pack_start(GTK_BOX(parent6), w, FALSE, FALSE, 0);
	gtk_widget_show(w);
	/* Value label */
	jsp->has_changes_label = w = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(parent6), w, FALSE, FALSE, 0);
	gtk_widget_show(w);



	/* Begin creating "Force Feedback" page */
	/* Vbox */
	jsp->force_feedback_page_vbox = w = gtk_vbox_new(FALSE, border_major);
	gtk_container_border_width(GTK_CONTAINER(w), border_major);
	gtk_notebook_append_page(
	    GTK_NOTEBOOK(parent3),
	    w,
	    gtk_label_new("Force Feedback")
	);
	gtk_widget_show(w);
	parent4 = w;

	/* Identity frame */
	w = gtk_frame_new("Identity");
	gtk_frame_set_shadow_type(GTK_FRAME(w), GTK_SHADOW_ETCHED_IN);
	gtk_box_pack_start(GTK_BOX(parent4), w, TRUE, TRUE, 0);
	gtk_widget_show(w);
	parent5 = w;
	/* Hbox */
	w = gtk_hbox_new(FALSE, border_major);
	gtk_container_border_width(GTK_CONTAINER(w), border_major);
	gtk_container_add(GTK_CONTAINER(parent5), w);
	gtk_widget_show(w);
	parent5 = w;

	/* Vbox for icon */
	w = gtk_vbox_new(FALSE, border_minor);
	gtk_box_pack_start(GTK_BOX(parent5), w, FALSE, FALSE, 0);
	gtk_widget_show(w);
	parent6 = w;
	/* Joystick icon */
	jsp->ff_icon_fixed = w = (GtkWidget *)GUICreateMenuItemIcon(
	    (guint8 **)icon_force_feedback_32x32_xpm
	);
	gtk_widget_set_usize(w, 32, 32);
	gtk_box_pack_start(GTK_BOX(parent6), w, FALSE, FALSE, 0);
	gtk_widget_show(w);
	jsp->ff_icon_pm = NULL;

	/* Vbox for entries and other widgets */
	w = gtk_vbox_new(FALSE, border_minor);
	gtk_box_pack_start(GTK_BOX(parent5), w, TRUE, TRUE, 0);
	gtk_widget_show(w);
	parent6 = w;

	/* Product ID entry */
	w = GUIPromptBar(
	    NULL, "Product ID:",
	    &label_rtn, &entry_rtn
	);
	gtk_box_pack_start(GTK_BOX(parent6), w, FALSE, FALSE, 0);
	gtk_widget_show(w);
	jsp->product_id_entry = w = (GtkWidget *)entry_rtn;
	gtk_entry_set_editable(GTK_ENTRY(w), FALSE);











	/* Separator */
	w = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(parent), w, FALSE, FALSE, 0);
	gtk_widget_show(w);


	/* Buttons hbox */
	w = gtk_hbox_new(TRUE, 0);
	gtk_box_pack_start(GTK_BOX(parent), w, FALSE, FALSE, border_major);
	gtk_widget_show(w);
	parent2 = w;

	/* OK button */
	width = JCJSPROPS_BTN_WIDTH;
	height = JCJSPROPS_BTN_HEIGHT;
	jsp->ok_btn = w = GUIButtonPixmapLabelH(
	    (guint8 **)icon_ok_20x20_xpm, "OK", NULL
	);
	gtk_widget_set_usize(w, width, height);
	GTK_WIDGET_SET_FLAGS(w, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(parent2), w, TRUE, FALSE, 0);
	gtk_signal_connect(
	    GTK_OBJECT(w), "clicked",
	    GTK_SIGNAL_FUNC(JCJSPropsOKCB), (gpointer)jsp
	);
	gtk_accel_group_add(
	    accelgrp, GDK_Return, 0, GTK_ACCEL_VISIBLE,
	    GTK_OBJECT(w), "clicked"
	);
	gtk_accel_group_add(
	    accelgrp, GDK_3270_Enter, 0, GTK_ACCEL_VISIBLE,
	    GTK_OBJECT(w), "clicked"
	);
	gtk_accel_group_add(
	    accelgrp, GDK_KP_Enter, 0, GTK_ACCEL_VISIBLE,
	    GTK_OBJECT(w), "clicked"
	);
	gtk_accel_group_add(
	    accelgrp, GDK_ISO_Enter, 0, GTK_ACCEL_VISIBLE,
	    GTK_OBJECT(w), "clicked"
	);
	gtk_widget_show(w);

	/* Apply button */
	jsp->apply_btn = w = GUIButtonPixmapLabelH(
	    (guint8 **)icon_select_20x20_xpm, "Apply", NULL
	);
	gtk_widget_set_usize(w, width, height);
	GTK_WIDGET_SET_FLAGS(w, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(parent2), w, TRUE, FALSE, 0);
	gtk_signal_connect(
	    GTK_OBJECT(w), "clicked",
	    GTK_SIGNAL_FUNC(JCJSPropsApplyCB), (gpointer)jsp
	);
	gtk_widget_show(w);

	/* Cancel button */
	jsp->cancel_btn = w = GUIButtonPixmapLabelH(
	    (guint8 **)icon_cancel_20x20_xpm, "Cancel", NULL
	);
	gtk_widget_set_usize(w, width, height);
	GTK_WIDGET_SET_FLAGS(w, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(parent2), w, TRUE, FALSE, 0);
	gtk_signal_connect(
	    GTK_OBJECT(w), "clicked",
	    GTK_SIGNAL_FUNC(JCJSPropsCancelCB), (gpointer)jsp
	);
	gtk_accel_group_add(
	    accelgrp, GDK_Escape, 0, GTK_ACCEL_VISIBLE,
	    GTK_OBJECT(w), "clicked"
	);
	gtk_widget_show(w);



	/* Mark as initialized */
	jsp->initialized = TRUE;

	return(jsp);
}

/*
 *	Gets values from the jsd structure found on the core structure
 *	on the given jsp. The values are then stored on the widgets on the
 *	given jsp.
 */
void JCJSPropsGetValues(jc_jsprops_struct *jsp)
{
	GtkWidget *w;
	jc_struct *jc;
	js_data_struct *jsd;


	if(jsp == NULL)
	    return;

	if(!jsp->initialized)
	    return;

	jc = (jc_struct *)jsp->core_ptr;
	if(jc == NULL)
	    return;

	/* Get joystick data structure and check if it is initialized */
	jsd = &jc->jsd;
	if(JSIsInit(jsd))
	{
	    /* Descriptive name */
	    w = jsp->name_entry;
	    if((w != NULL) ? GTK_IS_ENTRY(w) : FALSE)
		gtk_entry_set_text(
		    GTK_ENTRY(w),
		    (jsd->name != NULL) ? jsd->name : ""
		);

	    /* Device */
	    w = jsp->device_entry;
	    if((w != NULL) ? GTK_IS_ENTRY(w) : FALSE)
		gtk_entry_set_text(
		    GTK_ENTRY(w),
		    (jsd->device_name != NULL) ? jsd->device_name : ""
		);

	    /* Driver version */
	    w = jsp->driver_version_entry;
	    if((w != NULL) ? GTK_IS_ENTRY(w) : FALSE)
	    {
		gint version_major, version_minor, version_release;
		gchar *buf;

		JSDriverQueryVersion(
		    jsd,
		    &version_major, &version_minor, &version_release
		);
		buf = g_strdup_printf(
		    "%i.%i.%i",
		    version_major, version_minor, version_release
		);
		gtk_entry_set_text(GTK_ENTRY(w), buf);
		g_free(buf);
	    }

	    /* Calibration file */
	    w = jsp->calibration_file_entry;
	    if((w != NULL) ? GTK_IS_ENTRY(w) : FALSE)
		gtk_entry_set_text(
		    GTK_ENTRY(w),
		    (jsd->calibration_file != NULL) ? jsd->calibration_file : ""
		);

	    /* Total axises */
	    w = jsp->total_axises_label;
	    if((w != NULL) ? GTK_IS_LABEL(w) : FALSE)
	    {
		gchar *buf = g_strdup_printf(
		    "%i", jsd->total_axises
		);
		gtk_label_set_text(GTK_LABEL(w), buf);
		g_free(buf);
	    }

	    /* Total buttons */
	    w = jsp->total_buttons_label;
	    if((w != NULL) ? GTK_IS_LABEL(w) : FALSE)
	    {
		gchar *buf = g_strdup_printf(
		    "%i", jsd->total_buttons
		);
		gtk_label_set_text(GTK_LABEL(w), buf);
		g_free(buf);
	    }

	    /* Last calibrated */
	    w = jsp->last_calibrated_label;
	    if((w != NULL) ? GTK_IS_LABEL(w) : FALSE)
	    {
		gchar last_calibrated_str[1024];
		time_t dt = time(NULL) - jsd->last_calibrated;
		if((dt > 0) && (jsd->last_calibrated > 0))
		{
#define DAYS_TO_SECONDS(x)      ((x) * 24 * 60 * 60)

		    /* Less than an hour? */
		    if(dt < (60 * 60))
		    {
			strcpy(last_calibrated_str, "Less than an hour ago");
		    }
		    /* Less than a day? */
		    else if(dt < DAYS_TO_SECONDS(1))
		    {
			gint x = dt / 60 / 60;
			g_snprintf(
			    last_calibrated_str, sizeof(last_calibrated_str),
			    "%i hour%s ago",
			    x, (x == 1) ? "" : "s"
			);
		    }
		    /* Less than a week? */
		    else if(dt < DAYS_TO_SECONDS(7))
		    {
			gint x = dt / 60 / 60 / 24;
			g_snprintf(
			    last_calibrated_str, sizeof(last_calibrated_str),
			    "%i day%s ago",
			    x, (x == 1) ? "" : "s"
			);
		    }
		    /* Less than a month? */
		    else if(dt < DAYS_TO_SECONDS(30))
		    {
			gint x = dt / 60 / 60 / 24 / 7;
			g_snprintf(
			    last_calibrated_str, sizeof(last_calibrated_str),
			    "%i week%s ago",
			    x, (x == 1) ? "" : "s"
			);
		    }
		    /* Less than 6 months? */
		    else if(dt < DAYS_TO_SECONDS(6 * 30))
		    {
			gint x = dt / 60 / 60 / 24 / 30;
			g_snprintf(
			    last_calibrated_str, sizeof(last_calibrated_str),
			    "%i month%s ago",
			    x, (x == 1) ? "" : "s"
			);
		    }
		    /* More than 6 months */
		    else
		    {
			gchar *s, *buf = STRDUP(ctime(&jsd->last_calibrated));
			s = strchr(buf, '\n');
			if(s != NULL)
			    *s = '\0';
			strcpy(last_calibrated_str, buf);
			g_free(buf);
		    }
#undef DAYS_TO_SECONDS
		}
		else
		{
		    strcpy(last_calibrated_str, "Unknown");
		}
		gtk_label_set_text(GTK_LABEL(w), last_calibrated_str);
	    }

	    /* Has changes */
	    w = jsp->has_changes_label;
	    if((w != NULL) ? GTK_IS_LABEL(w) : FALSE)
	    {
		gtk_label_set_text(
		    GTK_LABEL(w),
		    (jc->has_changes) ? "Yes" : "No"
		);
	    }


	    /* Product id */
	    w = jsp->product_id_entry;
	    if((w != NULL) ? GTK_IS_ENTRY(w) : FALSE)
		gtk_entry_set_text(GTK_ENTRY(w), "");
	}
	else
	{
	    /* Descriptive name */
	    w = jsp->name_entry;
	    if((w != NULL) ? GTK_IS_ENTRY(w) : FALSE)
		gtk_entry_set_text(GTK_ENTRY(w), "");

	    /* Device */
	    w = jsp->device_entry;
	    if((w != NULL) ? GTK_IS_ENTRY(w) : FALSE)
		gtk_entry_set_text(GTK_ENTRY(w), "");

	    /* Driver version */
	    w = jsp->driver_version_entry;
	    if((w != NULL) ? GTK_IS_ENTRY(w) : FALSE)
		gtk_entry_set_text(GTK_ENTRY(w), "");

	    /* Calibration file */
	    w = jsp->calibration_file_entry;
	    if((w != NULL) ? GTK_IS_ENTRY(w) : FALSE)
		gtk_entry_set_text(GTK_ENTRY(w), "");

	    /* Total axises */
	    w = jsp->total_axises_label;
	    if((w != NULL) ? GTK_IS_LABEL(w) : FALSE)
		gtk_label_set_text(GTK_LABEL(w), "");

	    /* Total buttons */
	    w = jsp->total_buttons_label;
	    if((w != NULL) ? GTK_IS_LABEL(w) : FALSE)
		gtk_label_set_text(GTK_LABEL(w), "");

	    /* Last calibrated */
	    w = jsp->last_calibrated_label;
	    if((w != NULL) ? GTK_IS_LABEL(w) : FALSE)
		gtk_label_set_text(GTK_LABEL(w), "");

	    /* Has changes */
	    w = jsp->has_changes_label;
	    if((w != NULL) ? GTK_IS_LABEL(w) : FALSE)
		gtk_label_set_text(GTK_LABEL(w), "");


	    /* Product id */
	    w = jsp->product_id_entry;
	    if((w != NULL) ? GTK_IS_ENTRY(w) : FALSE)
		gtk_entry_set_text(GTK_ENTRY(w), "");

	}
}

/*
 *      Sets values on the widgets of the given jsp to the core structure
 *	specified by the given jsp.
 */
void JCJSPropsSetValues(jc_jsprops_struct *jsp)
{
	GtkWidget *w;
	jc_struct *jc;
	js_data_struct *jsd;


	if(jsp == NULL)
	    return;

	if(!jsp->initialized)
	    return;

	jc = (jc_struct *)jsp->core_ptr;
	if(jc == NULL)
	    return;

	/* Get joystick data structure and check if it is initialized */
	jsd = &jc->jsd;
	if(JSIsInit(jsd))
	{
	    /* Descriptive name */
	    w = jsp->name_entry;
	    if((w != NULL) ? GTK_IS_ENTRY(w) : FALSE)
	    {
		g_free(jsd->name);
		jsd->name = STRDUP(gtk_entry_get_text(GTK_ENTRY(w)));
	    }
	}

	/* Update has changes marker on the jc */
	jc->has_changes = TRUE;

	/* Redraw axises and buttons on the jc */
	JCDrawAxises(jc);
	JCDrawButtons(jc);
}

/*
 *	Maps the given jsp.
 */
void JCJSPropsMap(jc_jsprops_struct *jsp)
{
	GtkWidget *w;


	if(jsp == NULL)
	    return;

	if(!jsp->initialized)
	    return;

	if(!jsp->map_state)
	{
	    w = jsp->ok_btn;
	    if(w != NULL)
	    {
		gtk_widget_grab_focus(w);
		gtk_widget_grab_default(w);
	    }

	    w = jsp->notebook;
	    if(w != NULL)
		gtk_notebook_set_page(GTK_NOTEBOOK(w), 0);

	    w = jsp->toplevel;
	    if(w != NULL)
		gtk_widget_show(w);

	    jsp->map_state = TRUE;
	}
}

/*
 *	Unmaps the given jsp.
 */
void JCJSPropsUnmap(jc_jsprops_struct *jsp)
{
	GtkWidget *w;


	if(jsp == NULL)
	    return;

	if(!jsp->initialized)
	    return;

	if(jsp->map_state)
	{
	    w = jsp->toplevel;
	    if(w != NULL)
		gtk_widget_hide(w);

	    jsp->map_state = FALSE;
	}
}

/*
 *	Deletes the JSProperties dialog.
 */
void JCJSPropsDelete(jc_jsprops_struct *jsp)
{
	if(jsp == NULL)
	    return;

	if(jsp->initialized)
	{
	    GTK_WIDGET_DESTROY(jsp->general_icon_fixed);
            GTK_WIDGET_DESTROY(jsp->general_icon_pm);
            GTK_WIDGET_DESTROY(jsp->name_entry);
            GTK_WIDGET_DESTROY(jsp->device_entry);
            GTK_WIDGET_DESTROY(jsp->driver_version_entry);
            GTK_WIDGET_DESTROY(jsp->calibration_file_entry);
            GTK_WIDGET_DESTROY(jsp->total_axises_label);
            GTK_WIDGET_DESTROY(jsp->total_buttons_label);
            GTK_WIDGET_DESTROY(jsp->last_calibrated_label);
            GTK_WIDGET_DESTROY(jsp->has_changes_label);
            GTK_WIDGET_DESTROY(jsp->ff_icon_fixed);
            GTK_WIDGET_DESTROY(jsp->ff_icon_pm);
            GTK_WIDGET_DESTROY(jsp->product_id_entry);
            GTK_WIDGET_DESTROY(jsp->general_page_vbox);
            GTK_WIDGET_DESTROY(jsp->force_feedback_page_vbox);
            GTK_WIDGET_DESTROY(jsp->notebook);
            GTK_WIDGET_DESTROY(jsp->ok_btn);
            GTK_WIDGET_DESTROY(jsp->apply_btn);
            GTK_WIDGET_DESTROY(jsp->cancel_btn);
            GTK_WIDGET_DESTROY(jsp->toplevel);

	    GTK_ACCEL_GROUP_UNREF(jsp->accelgrp);
	}

	g_free(jsp);
}
