#include <jsw.h>
#include <gtk/gtk.h>

#include "../include/string.h"
#include "../include/disk.h"

#include "jc.h"
#include "config.h"


jc_struct *JCNew(gint argc, gchar **argv);
void JCSetBusy(jc_struct *jc);
void JCSetReady(jc_struct *jc);
void JCMap(jc_struct *jc);
void JCUnmap(jc_struct *jc);
void JCDelete(jc_struct *jc);


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
 *	Creates a new joystick calibrator window.
 */
jc_struct *JCNew(gint argc, gchar **argv)
{
	gint i, status;
	const gchar *s, *s2, *arg;
	js_data_struct *jsd;
	jc_struct *jc = (jc_struct *)g_malloc0(sizeof(jc_struct));
	if(jc == NULL)
	    return(jc);


	/* Begin resetting jc structure. */
	jc->initialized = TRUE;
	jc->map_state = FALSE;
	jc->processing = FALSE;

	/* Reset joystick data structure values. */
	jsd = &jc->jsd;
	jsd->name = NULL;
	jsd->axis = NULL;
	jsd->total_axises = 0;
	jsd->button = NULL;
	jsd->total_buttons = 0;
	jsd->device_name = NULL;
	jsd->calibration_file = NULL;
	jsd->fd = -1;
	jsd->flags = 0;
	jsd->driver_version = 0;

	jc->manage_toid = (guint)-1;
	jc->calib_file = NULL;
	jc->tried_load_device_on_init = FALSE;


	/* Set default joystick calibration file path. */
	s = g_getenv(ENV_VAR_NAME_HOME);
	if(s == NULL)
	     s2 = JSDefaultCalibration;
	else
	     s2 = PrefixPaths(s, JSDefaultCalibration);
	jc->calib_file = STRDUP(s2);
	jc->tried_load_device_on_init = FALSE;

	/* Reset has changes marker. */
	jc->has_changes = FALSE;


	/* Create widgets for the joystick calibrator window. */
	status = JCCreateWidgets(jc, argc, argv);
	if(status)
	{
	    g_free(jc);
	    jc = NULL;
	    return(jc);
	}

	/* Set timeout callback. */
	jc->manage_toid = gtk_timeout_add(
	    16,                 /* Every 16 milliseconds. */
	    (GtkFunction)JCTimeoutCB,
	    (gpointer)jc
	);


	/* Handle arguments. */
	for(i = 1; i < argc; i++)
	{
	    arg = argv[i];
	    if(arg == NULL)
		continue;

	    /* Specify alternate calibration file. */
	    if(strcasepfx(arg, "-f"))
	    {
		i++;
		if(i < argc)
		{
		    g_free(jc->calib_file);
		    jc->calib_file = (argv[i] != NULL) ?
			g_strdup(argv[i]) : NULL;
		}
		else
		{
		    g_printerr(
			"%s: Requires argument.\n",
			argv[i - 1]
		    );
		}
	    }
	    /* Specify startup device. */
	    else if(strcasepfx(arg, "-d"))
	    {
		i++;
		if(i < argc)
		{
		    GtkCombo *combo = (GtkCombo *)jc->js_device_combo;

		    if(combo != NULL)
			gtk_entry_set_text(
			    GTK_ENTRY(combo->entry),
			    argv[i]
		        );
		}
		else
		{
		    g_printerr(
			"%s: Requires argument.\n",
			argv[i - 1] 
		    );
		}
	    }
	}


	return(jc);
}

/*
 *      Marks the given jc as busy.
 */
void JCSetBusy(jc_struct *jc)
{
	GtkWidget *w;


	if(jc == NULL)
	    return;

	w = jc->toplevel;
	if(w == NULL)
	    return;

	gdk_window_set_cursor(w->window, jc_cursor.processing);
	gdk_flush();
}

/*
 *      Marks the given jc as ready.
 */
void JCSetReady(jc_struct *jc)
{
	GtkWidget *w;


	if(jc == NULL)
	    return;

	w = jc->toplevel;
	if(w == NULL)
	    return;

	gdk_window_set_cursor(w->window, NULL);
	gdk_flush();
}

/*
 *	Maps the given jc.
 */
void JCMap(jc_struct *jc) 
{
	if(jc == NULL)
	    return;

	if(!jc->map_state)
	{
	    GtkWidget *w = jc->toplevel;

	    if(w != NULL)
		gtk_widget_show(w);
	}
}

/*
 *	Unmaps the given jc.
 */
void JCUnmap(jc_struct *jc)
{
	if(jc == NULL)
	    return;

	if(jc->map_state)
	{
	    GtkWidget *w = jc->toplevel;

	    if(w != NULL)
		gtk_widget_hide(jc->toplevel);
	}
}

/*
 *	Procedure to deallocate all resources of the given jc structure
 *	and the structure itself. The given jc should not be referenced
 *	again after this call.
 */
void JCDelete(jc_struct *jc)
{
	if(jc == NULL)
	    return;

	/* Delete joystick properties dialog. */
	JCJSPropsDelete(jc->jsprops);
	jc->jsprops = NULL;

	/* Remove timeout id (as needed). */
	if(jc->manage_toid != (guint)-1)
	{
	    gtk_timeout_remove(jc->manage_toid);
	    jc->manage_toid = (guint)-1;
	}

	/* Close joystick. */
	JCDoCloseJoystick(jc);

	/* Destroy all widgets. */
	JCDestroyWidgets(jc);


	/* Deallocate memory. */
	g_free(jc->calib_file);
	jc->calib_file = NULL;

	/* Deallocate structure itself. */
	g_free(jc);
}
