/*
                             Joystick Calibrator
 */

#ifndef JC_H
#define JC_H

#include <jsw.h>
#include <gtk/gtk.h>

#include "jcjsprops.h"


/*
 *	Command codes:
 */
#define JC_CMD_NONE			0

#define JC_CMD_OPEN_CALIB		10
#define JC_CMD_OPEN_DEVICE		11
#define JC_CMD_SAVE_CALIB               12
#define JC_CMD_SAVEAS_CALIB             13
#define JC_CMD_CLEANUP_CALIB		14
#define JC_CMD_EXIT                     15

#define JC_CMD_REFRESH                  20
#define JC_CMD_REOPEN_DEVICE		21
#define JC_CMD_CLOSE_DEVICE		22
#define JC_CMD_JOYSTICK_PROPERTIES	23

#define JC_CMD_LAYOUT_REPRESENTATIVE	30
#define JC_CMD_LAYOUT_LOGICAL		31

#define JC_CMD_HELP_INDEX		40
#define JC_CMD_HELP_INTRODUCTION	41
#define JC_CMD_HELP_HOW_TO_CALIBRATE	42
#define JC_CMD_HELP_ABOUT		51



/* Macro to call gtk_main_iteration() while there are events to
 * be handled
 */
#ifndef GTK_EVENTS_FLUSH
# define GTK_EVENTS_FLUSH {	\
 while(gtk_events_pending())	\
  gtk_main_iteration();		\
}
#endif  /* GTK_EVENTS_FLUSH */



/*
 *	Cursors:
 */
typedef struct {

	GdkCursor	*processing;	/* Hourglass or watch */

} jc_cursor_struct;
extern jc_cursor_struct jc_cursor;


/*
 *	Representative joystick axises layout widgets structure:
 */
typedef struct {

	GtkWidget	*toplevel,	/* Hbox inside notebook */
			*instructions_label,
			*axis_da,	/* First two axises drawing area */
			*throttle_da,
			*rotate_da,
			*hat_da,
			*calib_toggle;

	GdkPixmap	*axis_buf,	/* First two axises */
			*throttle_buf,
			*rotate_buf,
			*hat_buf;

	GdkColor	c_axis_bg,	/* Background for axises */
			c_axis_fg,	/* Foreground for axises */
			c_axis_grid,
			c_axis_hat,
			c_throttle_fg;	/* Throttle axis fg */

} layout_rep_struct;


/*
 *	Logical joystick axises layout widgets structure:
 */
typedef struct {

	gboolean initialized;

	GtkWidget	*toplevel,	/* Parent vbox */
			*axis_num_label,
			*position_da,
			*calib_toggle,
			*nz_spin,
			*is_hat_check,
			*flipped_check,
			*correction_level_spin,
			*dz_min_spin, *dz_max_spin,
                        *corr_coeff_min1_spin, *corr_coeff_max1_spin;

	GtkObject	*nz_adj,
			*correction_level_adj,
			*dz_min_adj, *dz_max_adj,
			*corr_coeff_min1_adj, *corr_coeff_max1_adj;

	GdkPixmap	*position_buf;

	GdkColor	c_axis_bg,	/* Background for axises */
			c_axis_fg,	/* Foreground for axises */
			c_axis_grid,
			c_axis_nz;	/* Null zone background */

	GdkFont		*font;

} layout_logical_axis_struct;

typedef struct {

	GtkWidget       *toplevel;	/* Hbox inside notebook */

	layout_logical_axis_struct **axis;
	gint total_axises;

} layout_logical_struct;


/*
 *	Joystick button widgets structure:
 */
typedef struct {

	GtkWidget *toplevel;		/* Parent vbox */

	/* Button label widgets, corresponds with number of buttons */
	GtkWidget **button;
	gint total_buttons;

} buttons_list_struct;


/*
 *	Status bar structure:
 */
typedef struct {

        GtkWidget	*toplevel,	/* Parent vbox */
			*js_icon_fixed,	/* Dipicts js being init/shutdown */
			*js_icon_pixmap,
			*mesg_label;

} status_bar_struct;


/*
 *	Joystick calibrator main window:
 */
typedef struct {

	gboolean	initialized;
	gboolean	map_state;
	gboolean	processing;

	GtkWidget	*toplevel,
			*menu_bar,
			*js_device_combo,
			*signal_level_da,	/* JS signal meter drawing area */
			*axises_notebook;	/* Notebook holds axis layout widgets */

	GdkPixmap	*signal_level_pixmap;

	GdkColormap	*colormap;	/* From toplevel */

	GdkColor	c_signal_level_bg,
			c_signal_level_fg;

	GdkGC *gc;	/* General use graphics context */

	/* Layout state, this matches the axises_notebook page number */
#define JC_LAYOUT_REPRESENTATIVE	0
#define JC_LAYOUT_LOGICAL		1
	gint		layout_state;

	/* Axis layout substructures */
	/* Logical axis layout widgets and structure, the
	 * layout_logical_vp is a scrolled window that is the parent
	 * for layout_logical_parent. The layout_logical_parent is
	 * the parent for each axis widget group's parent widget.
	 */
	GtkWidget		*layout_logical_vp,	/* Viewport */
				*layout_logical_parent;	/* Parent vbox */
	layout_logical_struct	layout_logical;

	/* Representative axis layout widgets, a GtkVBox parent for the
	 * layout_rep_parent_client
	 *
	 * The layout_rep_parent_client is created and destroyed
	 * whenever the axises change
	 */
        GtkWidget		*layout_rep_parent,
				*layout_rep_parent_client;	/* Parent vbox */
	layout_rep_struct	layout_rep;


	/* Buttons list */
	GtkWidget		*buttons_list_vp,	/* Viewport */
				*buttons_list_parent;	/* Parent vbox */
	buttons_list_struct	buttons_list;

	/* Status bar */
	status_bar_struct	status_bar;

	/* Manage callback timeout id */
	guint		manage_toid;

	/* Joystick properties dialog */
	jc_jsprops_struct	*jsprops;


	/* Calibration file */
	gchar		*calib_file;
	gboolean	tried_load_device_on_init;

	/* Modifications marker */
	gboolean	has_changes;

	/* Joystick data */
	js_data_struct	jsd;


	/* Joystick signal load level history */
#define JC_SIGNAL_HISTORY_MAX   30
	guint16		signal_history[JC_SIGNAL_HISTORY_MAX];

} jc_struct;
#define JC(p)		((jc_struct *)(p))
extern jc_struct *jc_core;


/* In jc.c */
extern jc_struct *JCNew(gint argc, gchar **argv);
extern void JCSetBusy(jc_struct *jc);
extern void JCSetReady(jc_struct *jc);
extern void JCMap(jc_struct *jc);
extern void JCUnmap(jc_struct *jc);
extern void JCDelete(jc_struct *jc);

/* In jccalib.c */
extern void JCDoCalibrate(jc_struct *jc);
extern void JCDoSetAxisTolorance(jc_struct *jc, gint axis_num);

/* In jccb.c */
extern void JCSignalHandler(int s);
extern gint JCExitCB(
	GtkWidget *widget, GdkEvent *event, gpointer data
);
extern void JCMenuCB(GtkWidget *widget, gpointer data);
extern void JCSwitchPageCB(
	GtkNotebook *notebook, GtkNotebookPage *page, guint page_num,
	gpointer data
);
extern void JCJSDeviceEntryCB(GtkWidget *widget, gpointer data);
extern void JCRefreshCB(GtkWidget *widget, gpointer data);
extern void JCJSOpenDeviceCB(gpointer data);
extern void JCCalibToggleCB(GtkWidget *widget, gpointer data);
extern void JCEditableChangedCB(GtkWidget *widget, gpointer data);
extern gint JCEnterNotifyEventCB(
	GtkWidget *widget, GdkEventCrossing *crossing, gpointer data
);
extern gint JCLeaveNotifyEventCB(
	GtkWidget *widget, GdkEventCrossing *crossing, gpointer data
);
extern void JCCalibIsHatCheckCB(GtkWidget *widget, gpointer data);
extern void JCCalibFlipCheckCB(GtkWidget *widget, gpointer data);
extern gint JCExposeEventCB(
	GtkWidget *widget, GdkEventExpose *expose, gpointer data
);
extern gint JCButtonPressEventCB(
        GtkWidget *widget, GdkEventButton *button, gpointer data
);
extern gint JCButtonReleaseEventCB(
        GtkWidget *widget, GdkEventButton *button, gpointer data
);
extern gint JCTimeoutCB(gpointer data);
extern void JCDoPrintSpecifications(jc_struct *jc);
extern gint JCDoOpenCalibration(
	jc_struct *jc, const gchar *path
);
extern void JCJSOpenDeviceCB(gpointer data);
extern gint JCDoOpenJoystick(
        jc_struct *jc, const gchar *path
);
extern void JCDoCloseJoystick(jc_struct *jc);

/* In jcdraw.c */
extern void JCDrawAxises(jc_struct *jc);
extern void JCDrawButtons(jc_struct *jc);
extern void JCDrawSignalLevel(jc_struct *jc);

/* In jcfile.c */
extern gint JCDoSaveCalibration(jc_struct *jc, const gchar *path);
extern gint JCDoCalibrationCleanUp(jc_struct *jc, const gchar *path);

/* In jchelp.c */
extern void JCHelp(const gchar *topic, GtkWidget *toplevel);

/* In jcwidgets.c */
extern gint JCCreateRepresentativeLayoutWidgets(
        jc_struct *jc, layout_rep_struct *lr
);
extern void JCDestroyRepresentativeLayoutWidgets(
        jc_struct *jc, layout_rep_struct *lr
);
extern gint JCCreateLogicalLayoutWidgets(
        jc_struct *jc, layout_logical_struct *ll,
        GtkWidget *parent       /* Hbox inside notebook */
);
extern void JCLogicalLayoutWidgetsGetValues(
        jc_struct *jc, layout_logical_struct *ll
);
extern void JCDestroyLogicalLayoutWidgets(
        jc_struct *jc, layout_logical_struct *ll
);
extern gint JCCreateButtonsList(jc_struct *jc);
extern void JCDestroyButtonsList(jc_struct *jc);

extern void JCUpateDeviceComboList(jc_struct *jc);
extern GtkWidget *JCCreateMenuBar(jc_struct *jc);
extern gint JCCreateWidgets(jc_struct *jc, gint argc, gchar **argv);
extern void JCDestroyWidgets(jc_struct *jc);

/* In main.c */
extern gint JCInit(gint argc, gchar **argv);
extern void JCShutdown(void);

/* In statusbar.c */
extern void StatusBarSetJSIcon(
        status_bar_struct *sb, u_int8_t **icon_data
);
extern void StatusBarSetJSState(status_bar_struct *sb, gint state);
extern void StatusBarSetMesg(status_bar_struct *sb, const gchar *mesg);
extern gint StatusBarInit(status_bar_struct *sb, GtkWidget *parent);
extern void StatusBarDestroy(status_bar_struct *sb);


#endif	/* JC_H */
