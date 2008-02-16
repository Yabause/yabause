#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <gtk/gtk.h>

#include "../include/string.h"

#include "guiutils.h"
#include "cdialog.h"
#include "fb.h"
#include "pdialog.h"
#include "jc.h"

#include "config.h"
#include "helpmesgs.h"
#include "images/icon_game_controller_32x32.xpm"


void JCSignalHandler(int s);
static gboolean JCDoPromptSaveCalibration(jc_struct *jc);
static void JCDoPromptLogicalAxisBounds(jc_struct *jc, gint axis_num);
static void JCDoPromptJoystickProperties(jc_struct *jc);
gint JCExitCB(GtkWidget *widget, GdkEvent *event, gpointer data);
void JCMenuCB(GtkWidget *widget, gpointer data);
void JCSwitchPageCB(
	GtkNotebook *notebook, GtkNotebookPage *page, guint page_num,
	gpointer data
);
void JCJSDeviceEntryCB(GtkWidget *widget, gpointer data);
void JCRefreshCB(GtkWidget *widget, gpointer data);
void JCJSOpenDeviceCB(gpointer data);
void JCCalibToggleCB(GtkWidget *widget, gpointer data);
void JCEditableChangedCB(GtkWidget *widget, gpointer data);
gint JCEnterNotifyEventCB(
	GtkWidget *widget, GdkEventCrossing *crossing, gpointer data
);
gint JCLeaveNotifyEventCB(
	GtkWidget *widget, GdkEventCrossing *crossing, gpointer data
);
void JCCalibIsHatCheckCB(GtkWidget *widget, gpointer data);
void JCCalibFlipCheckCB(GtkWidget *widget, gpointer data);
gint JCExposeEventCB(
	GtkWidget *widget, GdkEventExpose *expose, gpointer data
);
gint JCButtonPressEventCB(
	GtkWidget *widget, GdkEventButton *button, gpointer data
);
gint JCButtonReleaseEventCB(
	GtkWidget *widget, GdkEventButton *button, gpointer data
);
gint JCTimeoutCB(gpointer data);
gint JCDoOpenCalibration(
	jc_struct *jc, const gchar *path
);
void JCJSOpenDeviceCB(gpointer data);
gint JCDoOpenJoystick(
	jc_struct *jc, const gchar *path
);
void JCDoCloseJoystick(jc_struct *jc);


#define MIN(a,b)	(((a) < (b)) ? (a) : (b))
#define MAX(a,b)	(((a) > (b)) ? (a) : (b))


/*
 *	Signal handler.
 */
void JCSignalHandler(int s)
{
	static gint segfault_count = 0;
	jc_struct *jc = jc_core;

	switch(s)
	{
	  case SIGINT:
	  case SIGTERM:
	  case SIGKILL:
	    /* Must reset has changes before calling JCExitCB() */
	    if(jc != NULL)
	    {
		jc->has_changes = FALSE;
		JCExitCB(NULL, NULL, jc);
	    }
	    break;

	  case SIGSEGV:
	    segfault_count++;
	    g_printerr(
		"%s triggered a segmentation fault (%i times)\n",
		PROG_NAME,
		segfault_count
	    );
	    if(segfault_count >= 3)
	    {
		g_printerr(
		    "%s attempting immediate process exit().\n",
		    PROG_NAME
		);
		exit(1);
	    }
	    break;
	}
}


/*
 *	Procedure to prompt the user to save calibration. This function
 *	should be called whenever jc->has_changes is TRUE and the next
 *	operation will discard those changes.
 *
 *	This function will call JCDoSaveCalibration() if the user wants
 *	to save the calibration.
 *
 *	Returns TRUE to indicate to proceed with next operation or FALSE
 *	to indicate to abort the operation.
 */
static gboolean JCDoPromptSaveCalibration(jc_struct *jc)
{
	static gboolean reenterent = FALSE;
	gint status;
	GtkWidget *toplevel;


	if(jc == NULL)
	    return(FALSE);

	if(reenterent)
	    return(FALSE);
	else
	    reenterent = TRUE;

	/* Get toplevel widget */
	toplevel = jc->toplevel;

	/* Query user for save confirmation */
	CDialogSetTransientFor(toplevel);
	status = CDialogGetResponse(
"Save Changes?",
"Changes to the current joystick's calibration have not\n\
been saved, do you want to save those changes?",
"There are changes made to the current joystick's\n\
calibration that have not been saved. If you want to\n\
save those changes then click on `Yes'. If you want to\n\
discard those changes then click on `No'. If you do not\n\
want to proceed with this operation at all then\n\
click on `Cancel'.",
	    CDIALOG_ICON_WARNING,
	    CDIALOG_BTNFLAG_YES | CDIALOG_BTNFLAG_NO |
	    CDIALOG_BTNFLAG_CANCEL | CDIALOG_BTNFLAG_HELP,
	    CDIALOG_BTNFLAG_YES
	);
	CDialogSetTransientFor(NULL);
	switch(status)
	{
	  case CDIALOG_RESPONSE_CANCEL:
	  case CDIALOG_RESPONSE_NOT_AVAILABLE:
	    /* User canceled or no response available, in which
	     * case the safest conclusion is to abort this operation
	     * entirly.
	     */
	    reenterent = FALSE;
	    return(FALSE);
	    break;

	  case CDIALOG_RESPONSE_NO:
	    /* Do not save calibration, discard changes and proceed with
	     * next operation.
	     */
	    reenterent = FALSE;
	    return(TRUE);
	    break;

	  case CDIALOG_RESPONSE_YES:
	  case CDIALOG_RESPONSE_YES_TO_ALL:
	  case CDIALOG_RESPONSE_OK:
	    /* Check if calibration file has no name, if it has no name
	     * then the file browser needs to be mapped to query for a
	     * file name to save to.
	     */
	    if(jc->calib_file == NULL)
	    {
		gchar **path_rtn = NULL;
		gint total_path_rtns = 0;
		fb_type_struct **ftype = NULL, *ftype_rtn = NULL;
		int total_ftypes = 0;

		/* Create file type extensions list */
		FileBrowserTypeListNew(
		    &ftype, &total_ftypes,
		    "*.*", "All files"
		);
		/* Get response */
		FileBrowserSetTransientFor(toplevel);
		status = FileBrowserGetResponse(
		    "Save Calibration File",
		    "Save", "Cancel",
		    NULL,		/* Use last path */
		    ftype, total_ftypes,
		    &path_rtn, &total_path_rtns,
		    &ftype_rtn
		);
		FileBrowserSetTransientFor(NULL);
		/* Got response? */
		if(status)
		{
		    if((total_path_rtns > 0) ? (path_rtn[0] != NULL) : 0)
		    {
			JCSetBusy(jc);
			JCDoSaveCalibration(jc, path_rtn[0]);
			JCSetReady(jc);

			/* Update new calibration file paths */
			g_free(jc->calib_file);
			jc->calib_file = g_strdup(path_rtn[0]);
			g_free(jc->jsd.calibration_file);
			jc->jsd.calibration_file = g_strdup(path_rtn[0]);
		    }
		}
		/* Deallocate file type extensions list */
		FileBrowserDeleteTypeList(ftype, total_ftypes);
		ftype = NULL;
		total_ftypes = 0;
	    }
	    else
	    {
		/* Save calibration */
		JCSetBusy(jc);
		JCDoSaveCalibration(jc, jc->calib_file);
		JCSetReady(jc);
	    }
	    /* Check if the save was successful by checking if
	     * has_changes is FALSE, if it is not FALSE then that
	     * implies there was a failure saving or the user canceled
	     * the save.
	     */
	    reenterent = FALSE;
	    if(jc->has_changes)
		return(FALSE);
	    else
		return(TRUE);
	    break;
	}

	/* This point should not be reached unless CDialogGetResponse()
	 * returned an unsupported response code. In which case its safest
	 * to return FALSE, indicating to not proceed with the next 
	 * operation.
	 */
	reenterent = FALSE;
	return(FALSE);
}

/*
 *	Maps the prompt dialog and querys the user for new bound values
 *	relative to the given axis_num.
 */
void JCDoPromptLogicalAxisBounds(jc_struct *jc, gint axis_num)
{
	static gboolean reenterent = FALSE;
	gchar **strv = NULL;
	gint strc = 0;
	gchar title[80], numstr[80];
	GtkWidget *toplevel;
	js_data_struct *jsd;
	js_axis_struct *axis_ptr;

	if((jc == NULL) || PDialogIsQuery())
	    return;

	if(reenterent)
	    return;
	else
	    reenterent = TRUE;

	toplevel = jc->toplevel;
	jsd = &jc->jsd;
	if(!JSIsInit(jsd))
	{
	    reenterent = FALSE;
	    return;
	}

	axis_ptr = JSIsAxisAllocated(jsd, axis_num) ?
	    jsd->axis[axis_num] : NULL;
	if(axis_ptr == NULL)
	{
	    reenterent = FALSE;
	    return;
	}

	/* Reset prompt dialog */
	PDialogDeleteAllPrompts();

	g_snprintf(
	    numstr, sizeof(numstr),
	    "%i", axis_ptr->min
	);
	PDialogAddPrompt(NULL, "Minimum:", numstr);

	g_snprintf(
	    numstr, sizeof(numstr),
	    "%i", axis_ptr->cen
	);
	PDialogAddPrompt(NULL, "Center:", numstr);

	g_snprintf(
	    numstr, sizeof(numstr),
	    "%i", axis_ptr->max
	);
	PDialogAddPrompt(NULL, "Maximum:", numstr);

	g_snprintf(
	    numstr, sizeof(numstr),
	    "%i", axis_ptr->tolorance
	);
	PDialogAddPrompt(NULL, "Tolorance:", numstr);

	PDialogSetSize(-1, -1);
	PDialogSetTransientFor(toplevel);

	/* Get user response */
	g_snprintf(
	    title, sizeof(title),
	    "Axis %i Bounds",
	    axis_num
	);
	strv = PDialogGetResponse(
	    title,
	    NULL, NULL,
	    PDIALOG_ICON_QUESTION,
	    "Set", "Cancel",
	    PDIALOG_BTNFLAG_SUBMIT | PDIALOG_BTNFLAG_CANCEL,
	    PDIALOG_BTNFLAG_SUBMIT,
	    &strc
	);
	PDialogSetTransientFor(NULL);
	if((strv != NULL) && (strc > 0))
	{
	    const gchar *cstrptr;


	    /* Get new axis values */
	    cstrptr = (strc > 0) ? strv[0] : NULL;
	    if(cstrptr != NULL)
		axis_ptr->min = atoi(cstrptr);

	    cstrptr = (strc > 1) ? strv[1] : NULL;
	    if(cstrptr != NULL)
		axis_ptr->cen = atoi(cstrptr);

	    cstrptr = (strc > 2) ? strv[2] : NULL;
	    if(cstrptr != NULL)
		axis_ptr->max = atoi(cstrptr);

	    cstrptr = (strc > 3) ? strv[3] : NULL;
	    if(cstrptr != NULL)
		axis_ptr->tolorance = atoi(cstrptr);


	    /* Mark has changes */
	    jc->has_changes = TRUE;

	    /* Redraw axises and buttons */
	    JCDrawAxises(jc);
	    JCDrawButtons(jc);
	}

	reenterent = FALSE;
}

/*
 *      Maps the prompt dialog and querys the user for joystick properties
 *	of the joystick specified by the jsd structure on the given jc
 *	structure.
 */
static void JCDoPromptJoystickProperties(jc_struct *jc)
{
	static gboolean reenterent = FALSE;
	jc_jsprops_struct *jsp;


	if(jc == NULL)
	    return;

	/* Get pointer to jsp structure, if it is NULL then create a
	 * new one.
	 */
	jsp = jc->jsprops;
	if(jsp == NULL)
	    jc->jsprops = jsp = JCJSPropsNew(jc);
	if(jsp == NULL)
	    return;

	JCJSPropsGetValues(jsp);
	JCJSPropsMap(jsp);

	reenterent = FALSE;
}


/*
 *	Exit callback handler.
 */
gint JCExitCB(
	GtkWidget *widget, GdkEvent *event, gpointer data
)
{
	jc_struct *jc = (jc_struct *)data;


	if(jc == NULL)
	    return(FALSE);

	if(jc->processing)
	    return(TRUE);

	/* Check if any dialog resources are in query, if they are then
	 * we should not exit.
	 */
	if(CDialogIsQuery())
	    return(TRUE);
	if(FileBrowserIsQuery())
	    return(TRUE);
	if(PDialogIsQuery())
	    return(TRUE);

	/* Are there unsaved changes? */
	if(jc->has_changes)
	{
	    /* Prompt user to save calibration and save calibration if
	     * user requests it. If we should not proceed to next
	     * operation (ie user canceled or there was an error) then
	     * this call will return FALSE.
	     */
	    if(!JCDoPromptSaveCalibration(jc))
		return(TRUE);
	}

	/* All checks for exit have passed, begin exiting */

	/* Deallocate all resources */
	JCShutdown();

	/* Break out of the top most GTK+ main loop */
	gtk_main_quit();

	return(TRUE);
}


/*
 *	Menu item activate callback.
 */
void JCMenuCB(GtkWidget *widget, gpointer data)
{
	static gboolean reenterent = FALSE;
	gint status;
	GtkWidget *toplevel;
	gchar **path_rtn = NULL;
	gint total_path_rtns = 0;
	fb_type_struct **ftype = NULL, *ftype_rtn = NULL;
	gint total_ftypes = 0;
	jc_struct *jc = jc_core;
	gint command = (gint)data;


	if(jc == NULL)
	    return;

	if(!jc->initialized || jc->processing)
	    return;

	if(reenterent)
	    return;
	else
	    reenterent = TRUE;

	toplevel = jc->toplevel;

	/* Handle by command */
	switch(command)
	{
	  /* Do nothing */
	  case JC_CMD_NONE:
	    break;

	  /* Open calibration file */
	  case JC_CMD_OPEN_CALIB:
	    /* Are there unsaved changes? */
	    if(jc->has_changes)
	    {
		/* Prompt user to save calibration and save calibration if
		 * user requests it. If we should not proceed to next
		 * operation (ie user canceled or there was an error) then
		 * this call will return FALSE.
		 */
		if(!JCDoPromptSaveCalibration(jc))
		{
		    reenterent = FALSE;
		    return;
		}
	    }

	    /* Create file type extensions list */
	    FileBrowserTypeListNew(
		&ftype, &total_ftypes,
		"*.*", "All files"
	    );
	    /* Get response */
	    FileBrowserSetTransientFor(toplevel);
	    status = FileBrowserGetResponse(
		"Open Calibration File",
		"Open", "Cancel",
		NULL,			/* Use last path */
		ftype, total_ftypes,
		&path_rtn, &total_path_rtns,
		&ftype_rtn
	    );
	    FileBrowserSetTransientFor(NULL);
	    /* Got response? */
	    if(status)
	    {
		if(total_path_rtns > 0)
		    JCDoOpenCalibration(jc, path_rtn[0]);
	    }
	    /* Deallocate file type extensions list */
	    FileBrowserDeleteTypeList(ftype, total_ftypes);
	    ftype = NULL;
	    total_ftypes = 0;
	    break;

	  /* Open joystick device */
	  case JC_CMD_OPEN_DEVICE:
	    /* Are there unsaved changes? */
	    if(jc->has_changes)
	    {
		/* Prompt user to save calibration and save calibration if
		 * user requests it. If we should not proceed to next
		 * operation (ie user canceled or there was an error) then
		 * this call will return FALSE.
		 */
		if(!JCDoPromptSaveCalibration(jc))
		{
		    reenterent = FALSE;
		    return;
		}
	    }

	    /* Open the device, this will close the joystick first (as 
	     * needed) and discard any changes. Then the joystick will be
	     * opened.
	     */
	    JCJSOpenDeviceCB(jc);
	    break;

	  /* Reopen joystick device */
	  case JC_CMD_REOPEN_DEVICE:
	    /* Are there unsaved changes? */
	    if(jc->has_changes)
	    {
		/* Prompt user to save calibration and save calibration if
		 * user requests it. If we should not proceed to next
		 * operation (ie user canceled or there was an error) then
		 * this call will return FALSE.
		 */
		if(!JCDoPromptSaveCalibration(jc))
		{
		    reenterent = FALSE;
		    return;
		}
	    }

	    /* Reopen the device, this will close the joystick first (as
	     * needed) and discard any changes. Then the joystick will be
	     * reopened.
	     */
	    JCJSOpenDeviceCB(jc);
	    break;

	  /* Close joystick device */
	  case JC_CMD_CLOSE_DEVICE:
	    /* Are there unsaved changes? */
	    if(jc->has_changes)
	    {
		/* Prompt user to save calibration and save calibration if
		 * user requests it. If we should not proceed to next
		 * operation (ie user canceled or there was an error) then
		 * this call will return FALSE.
		 */
		if(!JCDoPromptSaveCalibration(jc))
		{
		    reenterent = FALSE;
		    return;
		}
	    }

	    /* Close the joystick device */
	    JCDoCloseJoystick(jc);
	    break;

	  /* Save calibration file */
	  case JC_CMD_SAVE_CALIB:
	    /* Check if calibration file has no name, if it has no name
	     * then the file browser needs to be mapped to query for a
	     * file name to save to.
	     */
	    if(jc->calib_file == NULL)
	    {
		/* Create file type extensions list */
		FileBrowserTypeListNew(
		    &ftype, &total_ftypes,
		    "*.*", "All files"
		);
		/* Get response */
		FileBrowserSetTransientFor(toplevel);
		status = FileBrowserGetResponse(
		    "Save Calibration File",
		    "Save", "Cancel",
		    NULL,		/* Use last path */
		    ftype, total_ftypes,
		    &path_rtn, &total_path_rtns,
		    &ftype_rtn
		);
		FileBrowserSetTransientFor(NULL);
		/* Got response? */
		if(status)
		{
		    if((total_path_rtns > 0) ? (path_rtn[0] != NULL) : 0)
		    {
			JCSetBusy(jc);
			JCDoSaveCalibration(jc, path_rtn[0]);
			JCSetReady(jc);

			/* Update new calibration file paths */
			g_free(jc->calib_file);
			jc->calib_file = g_strdup(path_rtn[0]);
			g_free(jc->jsd.calibration_file);
			jc->jsd.calibration_file = g_strdup(path_rtn[0]);
		    }
		}
		/* Deallocate file type extensions list */
		FileBrowserDeleteTypeList(ftype, total_ftypes);
		ftype = NULL;
		total_ftypes = 0;
	    }
	    else
	    {
		/* Save calibration */
		JCSetBusy(jc);
		JCDoSaveCalibration(jc, jc->calib_file);
		JCSetReady(jc);
	    }
	    break;

	  /* Save as alternate calibration file */
	  case JC_CMD_SAVEAS_CALIB:
	    /* Create file type extensions list */
	    FileBrowserTypeListNew(
		&ftype, &total_ftypes,
		"*.*", "All files"
	    );
	    /* Get response */
	    FileBrowserSetTransientFor(toplevel);
	    status = FileBrowserGetResponse(
		"Save Calibration File",
		"Save", "Cancel",
		NULL,		/* Use last path */
		ftype, total_ftypes,
		&path_rtn, &total_path_rtns,
		&ftype_rtn
	    );
	    FileBrowserSetTransientFor(NULL);
	    /* Got response? */
	    if(status)
	    {
		if((total_path_rtns > 0) ? (path_rtn[0] != NULL) : 0)
		{
		    JCSetBusy(jc);
		    JCDoSaveCalibration(jc, path_rtn[0]);
		    JCSetReady(jc);

		    /* Update new calibration file paths */
		    g_free(jc->calib_file);
		    jc->calib_file = g_strdup(path_rtn[0]);
		    g_free(jc->jsd.calibration_file);
		    jc->jsd.calibration_file = g_strdup(path_rtn[0]);
		}
	    }
	    /* Deallocate file type extensions list */
	    FileBrowserDeleteTypeList(ftype, total_ftypes);
	    ftype = NULL;
	    total_ftypes = 0;
	    break;

	  /* Clean up calibration file */
	  case JC_CMD_CLEANUP_CALIB:
	    JCSetBusy(jc);
	    JCDoCalibrationCleanUp(jc, jc->calib_file);
	    JCSetReady(jc);
	    break;

	  /* Exit the program */
	  case JC_CMD_EXIT:
	    /* Check if it is safe to exit, prompt user to save any
	     * changes, and exit if possible.
	     */
	    JCExitCB(NULL, NULL, jc);
	    break;

	  /* Change joystick axis layout display to representative */
	  case JC_CMD_LAYOUT_REPRESENTATIVE:
	    gtk_notebook_set_page(
		GTK_NOTEBOOK(jc->axises_notebook),
		JC_LAYOUT_REPRESENTATIVE
	    );
	    break;

	  /* Change joystick axis layout display to representative */
	  case JC_CMD_LAYOUT_LOGICAL:
	    gtk_notebook_set_page(
		GTK_NOTEBOOK(jc->axises_notebook),
		JC_LAYOUT_LOGICAL
	    );
	    break;

	  /* Refresh */
	  case JC_CMD_REFRESH:
	    JCRefreshCB(toplevel, jc);
	    break;

	  /* Joystick properties */
	  case JC_CMD_JOYSTICK_PROPERTIES:
	    JCDoPromptJoystickProperties(jc);
	    break;

	  /* Help: Index */
	  case JC_CMD_HELP_INDEX:
	    JCHelp("Contents", toplevel);
	    break;

	  /* Help: Introduction */
	  case JC_CMD_HELP_INTRODUCTION:
	    JCHelp("Introduction", toplevel);
	    break;

	  /* Help: Introduction */
	  case JC_CMD_HELP_HOW_TO_CALIBRATE:
	    JCHelp("How To Calibrate", toplevel);
	    break;

	  /* Help: About */
	  case JC_CMD_HELP_ABOUT:
	    if(TRUE)
	    {
		gchar *buf = g_strdup_printf(
		    "%s\nVersion %s\n\n%s\n\n%s",
		    PROG_NAME_FULL, PROG_VERSION, PROG_URL,
		    PROG_COPYRIGHT
		);
		CDialogSetTransientFor(toplevel);
		CDialogGetResponseIconData(
		    PROG_NAME_FULL,
		    buf,
		    "About this program.",
		    (guint8 **)icon_game_controller_32x32_xpm,
		    CDIALOG_BTNFLAG_OK,
		    CDIALOG_BTNFLAG_OK
		);
		CDialogSetTransientFor(NULL);
		g_free(buf);
	    }
	    break;

	}

	reenterent = FALSE;
}

/*
 *	Notebook switch page callback.
 */
void JCSwitchPageCB(
	GtkNotebook *notebook, GtkNotebookPage *page, guint page_num,
	gpointer data
)
{
	static gboolean reenterant = FALSE;
	jc_struct *jc = (jc_struct *)data;
	if((jc == NULL) || (notebook == NULL))
	    return;

	if(reenterant)
	    return;
	else
	    reenterant = TRUE;

	/* Axises notebook? */
	if((gpointer)notebook == (gpointer)jc->axises_notebook)
	{
	    /* Set layout state (which always matches page number) */
	    jc->layout_state = page_num;
	}

	reenterant = FALSE;
}

/*
 *	Joystick device combo's entry "activate" callback. This handles.
 */
void JCJSDeviceEntryCB(GtkWidget *widget, gpointer data)
{
	jc_struct *jc = (jc_struct *)data;
	if(jc == NULL)
	    return;

	if(!jc->initialized || jc->processing)
	    return;

	/* Are there unsaved changes? */
	if(jc->has_changes)
	{
	    /* Prompt user to save calibration and save calibration if
	     * user requests it. If we should not proceed to next
	     * operation (ie user canceled or there was an error) then
	     * this call will return FALSE.
	     */
	    if(!JCDoPromptSaveCalibration(jc))
		return;
	}

	/* Open the device, this will close the joystick first (as
	 * needed) and discard any changes. Then the joystick will be
	 * opened.
	 */
	JCJSOpenDeviceCB(jc);
}

/*
 *	Refresh procedure callback.
 */
void JCRefreshCB(GtkWidget *widget, gpointer data)
{
	static gboolean reenterant = FALSE;
	jc_struct *jc = (jc_struct *)data;
	if(jc == NULL)
	    return;

	if(jc->processing)
	    return;

	if(reenterant)
	    return;
	else
	    reenterant = TRUE;

/* Need to add more things to refresh here */



	/* Update listing of joysticks in jc's js device combo list */
	JCUpateDeviceComboList(jc);

	/* Get values from axises and store them on the logical layout
	 * widgets.
	 */
	JCLogicalLayoutWidgetsGetValues(
	    jc, &jc->layout_logical
	);

	/* Redraw axis and buttons layout widgets */
	JCDrawAxises(jc);
	JCDrawButtons(jc);

	/* Update information on status bar */
	StatusBarSetJSState(&jc->status_bar, 1);
	StatusBarSetMesg(&jc->status_bar, "Joystick refreshed");

	reenterant = FALSE;
}

/*
 *	Calibrate toggle callback, these start/end calibration sequences
 *	when a calibrate toggle button from any of the axises layout
 *	widgets structures is toggled.
 *
 *	If the layout is set to representative then and calibration
 *	toggle is switched back off, then each axis will have its assumed
 *	orientation, style, flags, etc values set up.
 */
void JCCalibToggleCB(GtkWidget *widget, gpointer data)
{
	static gboolean reenterant = FALSE;
	layout_rep_struct *lr;
	layout_logical_struct *ll;
	jc_struct *jc = (jc_struct *)data;
	if((widget == NULL) || (jc == NULL))
	    return;

	if(jc->processing)
	    return;

	if(reenterant)
	    return;
	else
	    reenterant = TRUE;

	/* Handle by currently selected axises layout page */
	switch(jc->layout_state)
	{
	  /* ******************************************************* */
	  case JC_LAYOUT_REPRESENTATIVE:
	    lr = &jc->layout_rep;
	    if((widget != lr->calib_toggle) &&
	       (widget != NULL)
	    )
	        break;

	    /* Starting calibration? */
	    if(GTK_TOGGLE_BUTTON(widget)->active)
	    {
		/* Begin calibration in representative layout */
		gint i;
		js_data_struct *jsd = &jc->jsd;
		js_axis_struct *axis, **axis_list = jsd->axis;
		const gint total_axises = jsd->total_axises;

		/* Reset all axis limit values and related values */
		for(i = 0; i < total_axises; i++)
		{
		    axis = axis_list[i];
		    if(axis == NULL)
			continue;

		    axis->min = axis->cur;
		    axis->max = axis->cur;
		    axis->cen = axis->cur;

		    axis->flags |= JSAxisFlagTolorance;
		    axis->tolorance = 0;

		    axis->correction_level = 0;
		    axis->dz_min = axis->cur;
		    axis->dz_max = axis->cur;
		    axis->corr_coeff_min1 = 0.0;
		    axis->corr_coeff_max1 = 0.0;
		    axis->corr_coeff_min2 = 0.0;
		    axis->corr_coeff_max2 = 0.0;
		}
		/* Sync values for tolorance in error correction specified
		 * in the current axis structures to the low-level joystick
		 * driver
		 */
		JSResetAllAxisTolorance(jsd);

		/* Mark that we now have changes */
		if(!jc->has_changes)
		    jc->has_changes = TRUE;

		StatusBarSetMesg(
		    &jc->status_bar,
 "Calibrating: Move all axises to extremas and then center, click here gain when done"
		);
	    }
	    else
	    {
		/* Calibrating done, process new calibrated values in
		 * representative layout
		 *
		 * Note that representative layout is very imprecise,
		 * so we will be setting many assumed values
		 */
		gint i, len1, len2, len;
		gchar *s;
		js_data_struct *jsd = &jc->jsd;
		js_axis_struct *axis, **axis_list = jsd->axis; 
		const gint total_axises = jsd->total_axises;

		StatusBarSetMesg(
		    &jc->status_bar,
 "Processing new values (do not touch the device)..."
		);

		/* Iterate through each axis */
		for(i = 0; i < total_axises; i++)
		{
		    axis = axis_list[i];
		    if(axis == NULL)
			continue;

		    /* Set new center */             
		    axis->cen = axis->cur;

		    /* Update status message for this axis */
		    s = g_strdup_printf(
   "Axis %i: Processing calibration values...",
			i
		    );
		    StatusBarSetMesg(&jc->status_bar, s);
		    g_free(s);
		    GTK_EVENTS_FLUSH

		    /* Adjust null zone */
		    len1 = MAX(axis->max - axis->cen, 0);
		    len2 = MAX(axis->cen - axis->min, 0);
		    len = MAX(len1, len2);
		    axis->nz = len * 0.25;	/* 25% of len is nz */

		    /* Begin setting other values based on the axis
		     * number:
		     *
		     *	0	X
		     *  1	Y
		     *	2	Throttle
		     */
		    switch(i)
		    {
		      case 0:	/* Axis #0 */
			/* Nothing significant about this axis */
			break;

		      case 1:	/* Axis #1 */
			/* This is usually the Y or up/down axis, and
			 * most of the time it needs to be flipped.
			 */
			axis->flags |= JSAxisFlagFlipped;
			break;

		      case 2:	/* Axis #2 */
			/* This is sometimes the `twist' or `rotate',
			 * while other times it is the throttle. To best
			 * guess we check the total number of axises.
			 */
			switch(jsd->total_axises)
			{
			  case 5:	/* Have x, y, thr, hat */
			  case 3:	/* Have x, y, thr */
			    /* This is probably the throttle */
			    axis->nz = 0;	/* No nz for throttle */
			    axis->flags |= JSAxisFlagFlipped;
			    break;

			  case 6:	/* Have x, y, rot, thr, hat */
			  case 4:	/* Have x, y, rot, thr */
			  default:	/* More than 6 axises total */
			    /* All else assume rotate, nothing to do for
			     * rotate.
			     */
			    break;
			}
			break;

		      case 3:	/* Axis #3 */
			/* This is sometimes the hat or throttle. To best
			 * guess we check the total number of axises.
			 */
			switch(jsd->total_axises)
			{
			  case 5:	/* Have x, y, thr, hat */
			    /* This is probably the hat x */
			    axis->flags |= JSAxisFlagIsHat;
			    break;

			  case 6:       /* Have x, y, rot, thr, hat */
			  case 4:       /* Have x, y, rot, thr */
			  default:	/* More than 6 axises total */
			    /* This is probably the throttle */
			    axis->nz = 0;   /* No nz for throttle */
			    axis->flags |= JSAxisFlagFlipped;
			    break;
			}
			break;

		      case 4:	/* Axis #4 */
			/* Must be hat x */
			axis->flags |= JSAxisFlagIsHat;
			break;

		      case 5:	/* Axis #5 */
			/* Must be hat y */
			axis->flags |= JSAxisFlagFlipped;	/* Also flipped */
			axis->flags |= JSAxisFlagIsHat;
			break;

		      default:	/* Beyond the 6th axis */
			/* Not understood so make no assumptions and
			 * do not set any special values to it.
			 */
			break;
		    }

		    /* Calculate tolorance for error correction */
		    JCDoSetAxisTolorance(jc, i);
		}
		/* Sync values for tolorance in error correction specified
		 * in the current axis structures to the low-level joystick
		 * driver
		 */
		JSResetAllAxisTolorance(jsd);

		/* Need to update logical axis widget values based on
		 * newly set axis flags and options set here
		 */
		JCLogicalLayoutWidgetsGetValues(jc, &jc->layout_logical);

		StatusBarSetMesg(
		    &jc->status_bar,
		    "New calibration values set"
		);
	    }
	    break;

	  /* ******************************************************* */
	  case JC_LAYOUT_LOGICAL:
	    ll = &jc->layout_logical;
	    if((ll != NULL) && (widget != NULL))
	    {
		gint i;
		const gint total_axises = ll->total_axises;
		gchar *s;
		layout_logical_axis_struct *ll_axis;
		js_data_struct *jsd = &jc->jsd;
		js_axis_struct *axis, **axis_list = jsd->axis;

		/* Go through axises, search for which axis
		 * is to be toggled
		 */
		for(i = 0; i < total_axises; i++)
		{
		    ll_axis = ll->axis[i];
		    if(ll_axis == NULL)
			continue;

		    /* Skip uninitialized ones */
		    if(!ll_axis->initialized)
			continue;

		    if(ll_axis->calib_toggle != widget)
			continue;

		    if(JSIsAxisAllocated(jsd, i))
			axis = axis_list[i];
		    else
			continue;

		    /* Mark that we now have changes */
		    if(!jc->has_changes)
			jc->has_changes = TRUE;

		    /* Begin calibrating? */
		    if(GTK_TOGGLE_BUTTON(widget)->active)
		    {
			/* Reset values and enter calibration for
			 * this axis
			 */
			axis->cen = axis->cur;
			axis->min = axis->cur;
			axis->max = axis->cur;

			axis->flags |= JSAxisFlagTolorance;
			axis->tolorance = 0;

			/* Do not reset other axis values since user is
			 * allowed to modify them manually and user may
			 * want to keep their current values
			 */

			/* Sync values for tolorance in error correction
			 * specified in the current axis structures to the
			 * low-level joystick driver
			 */
			JSResetAllAxisTolorance(jsd);

			s = g_strdup_printf(
 "Calibrating axis %i: Move axis to extremas and then center, click here again when done",
			    i
			);
		    }
		    else
		    {
			/* Done calibrating this axis */

			StatusBarSetMesg(
			    &jc->status_bar,
 "Processing new values (do not touch the device)..." 
			);
			GTK_EVENTS_FLUSH

			/* Set new center */
			axis->cen = axis->cur;

			/* Calculate tolorance for error correction */
			JCDoSetAxisTolorance(jc, i);

			/* Sync values for tolorance in error correction
			 * specified in the current axis structures to the
			 * low-level joystick driver
			 */
			JSResetAllAxisTolorance(jsd);

			s = g_strdup_printf(
   "Axis %i: Calibration values set",
			    i
			);
		    }
		    StatusBarSetMesg(&jc->status_bar, s);
		    g_free(s);
		}
	    }
	    break;
	}

	reenterant = FALSE;
}

/*
 *	Editable "changed" signal callback.
 *
 *	Used to check for changes on spin widgets.
 */
void JCEditableChangedCB(GtkWidget *widget, gpointer data)
{
	gint i;
	gfloat value;
	js_axis_struct *axis;
	layout_logical_struct *ll;
	layout_logical_axis_struct *ll_axis;
	jc_struct *jc = (jc_struct *)data;
	if((widget == NULL) || (jc == NULL))
	    return;

	if(jc->processing)
	    return;

	/* Get value by widget type */
	if(GTK_IS_ENTRY(widget))
	{
	    GtkEntry *entry = GTK_ENTRY(widget);
	    const gchar *cstrptr = gtk_entry_get_text(entry);
	    value = (cstrptr != NULL) ? atof(cstrptr) : 0.0;
	}
	else
	{
	    value = 0.0;
	}


	/* Check logical layout page */
	ll = &jc->layout_logical;
	if(ll == NULL)
	    return;

	/* Itterate through each axis */
	for(i = 0; i < ll->total_axises; i++)
	{
	    ll_axis = ll->axis[i];
	    if(ll_axis == NULL)
		continue;

	    /* Skip uninitialized ones */
	    if(!ll_axis->initialized)
		continue;

	    if(JSIsAxisAllocated(&jc->jsd, i))
		axis = jc->jsd.axis[i];
	    else
		break;

	    /* Check which widget on the axis structure matches the given
	     * widget.
	     */
	    if(widget == ll_axis->nz_spin)
	    {
		gint prev_value = axis->nz;

		/* Set new value */
		axis->nz = (gint)value;
		/* Check for change */
		if(prev_value != (gint)value)
		    jc->has_changes = TRUE;

		JCDrawAxises(jc);

		break;
	    }
	    else if(widget == ll_axis->correction_level_spin)
	    {
		gint prev_value = axis->correction_level;

		/* Set new value */
		axis->correction_level = (gint)value;
		/* Check for change */
		if(prev_value != (gint)value)
		    jc->has_changes = TRUE;

		JCDrawAxises(jc);

		break;
	    }
	    else if(widget == ll_axis->dz_min_spin)
	    {
		gint prev_value = axis->dz_min;

		/* Set new value */
		axis->dz_min = (gint)value;
		/* Check for change */
		if(prev_value != (gint)value)
		    jc->has_changes = TRUE;

		JCDrawAxises(jc);

		break;
	    }
	    else if(widget == ll_axis->dz_max_spin)
	    {
		gint prev_value = axis->dz_max;

		/* Set new value */
		axis->dz_max = (int)value;
		/* Check for change */
		if(prev_value != (gint)value)
		    jc->has_changes = TRUE;

		JCDrawAxises(jc);

		break;
	    }
	    else if(widget == ll_axis->corr_coeff_min1_spin)
	    {
		gfloat prev_value = axis->corr_coeff_min1;

		/* Set new value */
		axis->corr_coeff_min1 = value;
		/* Check for change */
		if(prev_value != value)
		    jc->has_changes = TRUE;

		JCDrawAxises(jc);

		break;
	    }
	    else if(widget == ll_axis->corr_coeff_max1_spin)
	    {
		gfloat prev_value = axis->corr_coeff_max1;

		/* Set new value */
		axis->corr_coeff_max1 = value;
		/* Check for change */
		if(prev_value != value)
		    jc->has_changes = TRUE;

		JCDrawAxises(jc);

		break;
	    }

	}
}

/*
 *	Widget "enter_notify_event" signal callback.
 */
gint JCEnterNotifyEventCB(
	GtkWidget *widget, GdkEventCrossing *crossing, gpointer data
)
{
	gint i;
	layout_rep_struct *lr;
	layout_logical_struct *ll;
	layout_logical_axis_struct *ll_axis_ptr;
	jc_struct *jc = (jc_struct *)data;
	if((widget == NULL) || (jc == NULL))
	    return(FALSE);

	if(jc->processing)
	    return(TRUE);

	/* Begin checking representative layout widgets */
	lr = &jc->layout_rep;
	if(widget == lr->calib_toggle)
	{
	    if(GTK_TOGGLE_BUTTON(widget)->active)
		StatusBarSetMesg(
		    &jc->status_bar,
		    "Click here to end calibration of joystick"
		);
	    else
		StatusBarSetMesg(
		    &jc->status_bar,
		    "Click here to start calibration of joystick"
		);
	}

	/* Begin checking logical layout widgets */
	ll = &jc->layout_logical;
	for(i = 0; i < ll->total_axises; i++)
	{
	    ll_axis_ptr = ll->axis[i];
	    if(ll_axis_ptr == NULL)
		continue;

	    /* Skip uninitialized ones */
	    if(!ll_axis_ptr->initialized)
		continue;

	    if(widget == ll_axis_ptr->calib_toggle)
	    {
		gchar *buf;
		if(GTK_TOGGLE_BUTTON_GET_ACTIVE(widget))
		    buf = g_strdup_printf(
			"Click here to end calibration of axis %i",
			i
		    );
		else
		    buf = g_strdup_printf(
			"Click here to start calibration of axis %i",
			i
		    );
		StatusBarSetMesg(&jc->status_bar, buf);
		g_free(buf);
	    }
	    else if(widget == ll_axis_ptr->nz_spin)
	    {
		gchar *buf = g_strdup_printf(
"Null zone radius of axis %i in raw units (set to 0 for no null zone)",
		    i
		);
		StatusBarSetMesg(&jc->status_bar, buf);
		g_free(buf);
	    }
	    else if(widget == ll_axis_ptr->is_hat_check)
	    {
		gchar *buf;
		if(GTK_TOGGLE_BUTTON_GET_ACTIVE(widget))
		    buf = g_strdup_printf(   
			"Toggle to denote axis %i is not a hat",
			i
		    );
		else
		    buf = g_strdup_printf(   
			"Toggle to denote axis %i is a hat",
			i
		    );
		StatusBarSetMesg(&jc->status_bar, buf);
		g_free(buf);
	    }
	    else if(widget == ll_axis_ptr->flipped_check)
	    {
		gchar *buf = g_strdup_printf(   
"Toggle to flip the value of axis %i",
		    i
		);
		StatusBarSetMesg(&jc->status_bar, buf);
		g_free(buf);
	    }
	    else if(widget == ll_axis_ptr->correction_level_spin)
	    {
		gchar *buf = g_strdup_printf(   
"Level of error correction for axis %i (set to 0 for no correction)",
		    i
		);
		StatusBarSetMesg(&jc->status_bar, buf);
		g_free(buf);
	    }
	    else if(widget == ll_axis_ptr->dz_min_spin)
	    {
		gchar *buf = g_strdup_printf(
"Lower dead zone boundary position of axis %i in raw units",
		    i
		);
		StatusBarSetMesg(&jc->status_bar, buf);
		g_free(buf);
	    }
	    else if(widget == ll_axis_ptr->dz_max_spin)
	    {
		gchar *buf = g_strdup_printf(
"Upper dead zone boundary position of axis %i in raw units",
		    i
		);
		StatusBarSetMesg(&jc->status_bar, buf);
		g_free(buf);
	    }
	    else if(widget == ll_axis_ptr->corr_coeff_min1_spin)
	    {
		gchar *buf = g_strdup_printf(
"Lower correctional coefficient of axis %i in [0.0 to 1.0]",
		    i
		);
		StatusBarSetMesg(&jc->status_bar, buf);
		g_free(buf);
	    }
	    else if(widget == ll_axis_ptr->corr_coeff_max1_spin)
	    {
		gchar *buf = g_strdup_printf(   
"Upper correctional coefficient of axis %i in [0.0 to 1.0]",
		    i
		);
		StatusBarSetMesg(&jc->status_bar, buf);
		g_free(buf);
	    }

	}
	return(TRUE);
}

/*
 *	Widget "leave_notify_event" signal callback.
 */
gint JCLeaveNotifyEventCB(
	GtkWidget *widget, GdkEventCrossing *crossing, gpointer data
)
{
	gint i;
	layout_rep_struct *lr;
	layout_logical_struct *ll;
	layout_logical_axis_struct *ll_axis_ptr;
	jc_struct *jc = (jc_struct *)data;
	if((widget == NULL) || (jc == NULL)) 
	    return(FALSE);

	if(jc->processing)
	    return(TRUE);

	/* Begin checking representative layout widgets */   
	lr = &jc->layout_rep;
	if(widget == lr->calib_toggle)
	    StatusBarSetMesg(&jc->status_bar, NULL);

	/* Begin checking logical layout widgets */
	ll = &jc->layout_logical;
	for(i = 0; i < ll->total_axises; i++)
	{
	    ll_axis_ptr = ll->axis[i];
	    if(ll_axis_ptr == NULL)
		continue;

	    /* Skip uninitialized ones */
	    if(!ll_axis_ptr->initialized)
		continue;

	    if(widget == ll_axis_ptr->calib_toggle)
		StatusBarSetMesg(&jc->status_bar, NULL);
	    else if(widget == ll_axis_ptr->nz_spin)
		StatusBarSetMesg(&jc->status_bar, NULL);
	    else if(widget == ll_axis_ptr->is_hat_check)
		StatusBarSetMesg(&jc->status_bar, NULL);
	    else if(widget == ll_axis_ptr->flipped_check)
		StatusBarSetMesg(&jc->status_bar, NULL);
	    else if(widget == ll_axis_ptr->correction_level_spin)
		StatusBarSetMesg(&jc->status_bar, NULL);
	    else if(widget == ll_axis_ptr->dz_min_spin)
		StatusBarSetMesg(&jc->status_bar, NULL);
	    else if(widget == ll_axis_ptr->dz_max_spin)
		StatusBarSetMesg(&jc->status_bar, NULL);
	    else if(widget == ll_axis_ptr->corr_coeff_min1_spin)
		StatusBarSetMesg(&jc->status_bar, NULL);
	    else if(widget == ll_axis_ptr->corr_coeff_max1_spin)
		StatusBarSetMesg(&jc->status_bar, NULL);
	}
	return(TRUE);
}

/*
 *	Callback to the logical layout is hat check.
 */
void JCCalibIsHatCheckCB(GtkWidget *widget, gpointer data)
{
	gint i;
	unsigned int prev_flags;
	js_axis_struct *axis_ptr;
	layout_logical_struct *ll;
	layout_logical_axis_struct *ll_axis_ptr;
	jc_struct *jc = (jc_struct *)data;
	if((widget == NULL) || (jc == NULL))
	    return;

	if(jc->processing)
	    return;

	/* Not on the logical axises layout page? */
	if(jc->layout_state != JC_LAYOUT_LOGICAL)
	    return;

	ll = &jc->layout_logical;

	/* Go through each axis */
	for(i = 0; i < ll->total_axises; i++)
	{
	    ll_axis_ptr = ll->axis[i];
	    if((ll_axis_ptr != NULL) ?
		(widget != ll_axis_ptr->is_hat_check) : 1
	    )
		continue;

	    /* Skip uninitialized ones */
	    if(!ll_axis_ptr->initialized)
		continue;

	    if(JSIsAxisAllocated(&jc->jsd, i))
		axis_ptr = jc->jsd.axis[i];
	    else
		break;

	    prev_flags = axis_ptr->flags;
	    if(GTK_TOGGLE_BUTTON(widget)->active)
		axis_ptr->flags |= JSAxisFlagIsHat;
	    else
		axis_ptr->flags &= ~JSAxisFlagIsHat;

	    if(prev_flags != axis_ptr->flags)
		jc->has_changes = TRUE;

	    JCDrawAxises(jc);

	    break;
	}
}

/*
 *	Callback to the logical layout flipped check.
 */
void JCCalibFlipCheckCB(GtkWidget *widget, gpointer data)
{
	gint i;
	unsigned int prev_flags;
	js_axis_struct *axis_ptr;
	layout_logical_struct *ll;
	layout_logical_axis_struct *ll_axis_ptr;
	jc_struct *jc = (jc_struct *)data;
	if((widget == NULL) || (jc == NULL))
	    return;

	if(jc->processing)
	    return;

	/* Not on the logical axises layout page? */
	if(jc->layout_state != JC_LAYOUT_LOGICAL)
	    return;
 
	ll = &jc->layout_logical;

	/* Go through each axis */                
	for(i = 0; i < ll->total_axises; i++)
	{       
	    ll_axis_ptr = ll->axis[i];
	    if((ll_axis_ptr != NULL) ?
		(widget != ll_axis_ptr->flipped_check) : 1
	    )
		continue;

	    /* Skip uninitialized ones */
	    if(!ll_axis_ptr->initialized)
		continue;

	    if(JSIsAxisAllocated(&jc->jsd, i))
		axis_ptr = jc->jsd.axis[i];
	    else
		break;

	    prev_flags = axis_ptr->flags;
	    if(GTK_TOGGLE_BUTTON(widget)->active)
		axis_ptr->flags |= JSAxisFlagFlipped;
	    else
		axis_ptr->flags &= ~JSAxisFlagFlipped;

	    if(prev_flags != axis_ptr->flags)
		jc->has_changes = TRUE;

	    JCDrawAxises(jc);

	    break;
	}
}

/*
 *	Widget "expose_event" signal callback.
 *
 *	If widget is NULL then all widgets will be redrawn.
 */
gint JCExposeEventCB(
	GtkWidget *widget, GdkEventExpose *expose, gpointer data
)
{
	static gboolean reenterent = FALSE;
	jc_struct *jc = (jc_struct *)data;
	if(jc == NULL)
	    return(TRUE);

	if(reenterent)
	    return(TRUE);
	else
	    reenterent = TRUE;

	if(widget == NULL)
	{
	    /* The widget is not given so assume to redraw everything */
	    JCDrawAxises(jc);
	}
	else
	{
	    /* The widget is given so see which widget we need to
	     * redraw for.
	     */
	    gint i;
	    layout_rep_struct *lr = &jc->layout_rep;
	    layout_logical_struct *ll = &jc->layout_logical;
	    layout_logical_axis_struct *ll_axis_ptr;


	    /* Begin checking layout representative widgets */
	    if((widget == lr->axis_da) ||
	       (widget == lr->throttle_da) ||
	       (widget == lr->rotate_da) ||
	       (widget == lr->hat_da)
	    )
	    {
		JCDrawAxises(jc);
	    }

	    /* Begin checking layout logical widgets */
	    for(i = 0; i < ll->total_axises; i++)
	    {
		ll_axis_ptr = ll->axis[i];
		if(ll_axis_ptr == NULL)
		    continue;

		/* Skip if not initialized */
		if(!ll_axis_ptr->initialized)
		    continue;

		if(widget == ll_axis_ptr->position_da)
		{
		    JCDrawAxises(jc);
		}
	    }

	    /* Signal level drawing area? */
	    if(widget == jc->signal_level_da)
	    {
		JCDrawSignalLevel(jc);
	    }
	}

	reenterent = FALSE;
	return(TRUE);
}

/*
 *	Widget "button_press_event" signal callback.
 */
gint JCButtonPressEventCB(
	GtkWidget *widget, GdkEventButton *button, gpointer data
)
{
	static gboolean reenterent = FALSE;	
	gint i;
	layout_rep_struct *lr;
	layout_logical_struct *ll;
	layout_logical_axis_struct *ll_axis_ptr;
	jc_struct *jc = (jc_struct *)data;
	if((widget == NULL) || (jc == NULL))
	    return(TRUE);

	if(reenterent)
	    return(TRUE);
	else
	    reenterent = TRUE;

	/* Begin checking layout representative widgets */
	lr = &jc->layout_rep;

/* Nothing to check */


	/* Begin checking layout logical widgets */
	ll = &jc->layout_logical;
	for(i = 0; i < ll->total_axises; i++)
	{
	    ll_axis_ptr = ll->axis[i];
	    if(ll_axis_ptr == NULL)
		continue;

	    /* Skip if not initialized */
	    if(!ll_axis_ptr->initialized)
		continue;

	    if(widget == ll_axis_ptr->position_da)
	    {
		switch(button->button)
		{
		  case 1:
		    JCDoPromptLogicalAxisBounds(jc, i);
		    break;

		}

	    }
	}

	reenterent = FALSE;
	return(TRUE);
}

/*
 *	Widget "button_release_event" signal callback.
 */
gint JCButtonReleaseEventCB(
	GtkWidget *widget, GdkEventButton *button, gpointer data
)
{
	static gboolean reenterent = FALSE;
	gint i;
	layout_rep_struct *lr;
	layout_logical_struct *ll;
	layout_logical_axis_struct *ll_axis_ptr;
	jc_struct *jc = (jc_struct *)data;
	if((widget == NULL) || (jc == NULL))
	    return(TRUE);

	if(reenterent)
	    return(TRUE);
	else
	    reenterent = TRUE;

	/* Begin checking layout representative widgets */
	lr = &jc->layout_rep;

/* Nothing to check */

	/* Begin checking layout logical widgets */
	ll = &jc->layout_logical;
	for(i = 0; i < ll->total_axises; i++)
	{
	    ll_axis_ptr = ll->axis[i];
	    if(ll_axis_ptr == NULL)
		continue;

	    /* Skip if not initialized */
	    if(!ll_axis_ptr->initialized)
		continue;




	}

	reenterent = FALSE;
	return(TRUE);
}


/*
 *	Timeout callback, this function is called once per loop.
 */
gint JCTimeoutCB(gpointer data)
{
	static gboolean reenterent = FALSE;
	gint i, n, status;
	guint16 *history;
	js_data_struct *jsd;
	jc_struct *jc = (jc_struct *)data;
	if(jc == NULL)
	    return(FALSE);

	if(jc->processing)
	    return(TRUE);

	if(reenterent)
	    return(TRUE);
	else
	    reenterent = TRUE;

	/* Set new timeout callback for this function. Because this
	 * function will always return FALSE from this point on, the
	 * previous timeout callback will be unset and this new one
	 * will have affect.
	 */
	jc->manage_toid = gtk_timeout_add(
	    16,			/* Every 16 milliseconds */
	    (GtkFunction)JCTimeoutCB,
	    (gpointer)jc
	);


	/* Get pointer to joystick data structure */
	jsd = &jc->jsd;


	/* ********************************************************** */
	/* Open joystick device for the first time as needed */
	if(!jc->tried_load_device_on_init)
	{
	    GtkCombo *combo = (GtkCombo *)jc->js_device_combo;

	    if((combo != NULL) && (jsd->fd < 0))
		JCDoOpenJoystick(
		    jc,
		    gtk_entry_get_text(GTK_ENTRY(combo->entry))
		);

	    jc->tried_load_device_on_init = TRUE;
	}

	/* ********************************************************** */
	/* `Shift' joystick signal load history */
	history = jc->signal_history;
	for(i = 0; i < (JC_SIGNAL_HISTORY_MAX - 1); i++)
	    history[i] = history[i + 1];

	/* Get address of last history position */
	history = &jc->signal_history[JC_SIGNAL_HISTORY_MAX - 1];


	/* Get new joystick values/event */
	if(jsd->fd > -1)
	    status = JSUpdate(jsd);
	else
	    status = JSNoEvent;

	/* Check if a joystick event was recieved */
	if(status == JSGotEvent)
	{
	    /* Perform calibrations if needed */
	    JCDoCalibrate(jc);

	    /* Redraw axises and buttons */
	    JCDrawAxises(jc);
	    JCDrawButtons(jc);

	    /* Update signal load history */
	    n = (*history) + 1000;
	    if(n > (gint)((guint16)-1))
		n = (gint)((guint16)-1);
	    (*history) = n;
	}
	else
	{
	    /* Update signal load history */
	    n = (*history) - 1000;
	    if(n < 0)
		n = 0;
	    (*history) = n;
	}

	/* ********************************************************** */
	/* Redraw signal load level */
	JCDrawSignalLevel(jc);

	reenterent = FALSE;
	/* Always return FALSE telling the previous set callback to not
	 * call this function again. However this function will be called
	 * again because a new timeout was set within this function.
	 */
	return(FALSE);
}

/*
 *	Procedure to open a new calibration file.
 */
gint JCDoOpenCalibration(
	jc_struct *jc, const gchar *path
)
{
	struct stat stat_buf;


	if((jc == NULL) || (path == NULL))
	    return(-1);

	/* Make sure path exists */
	if(stat(path, &stat_buf))
	{
	    gchar *buf = g_strdup_printf(
"No such file:\n\
\n\
    %s\n",
		path
	    );

	    CDialogSetTransientFor(jc->toplevel);
	    CDialogGetResponse(
"Open Failed",
buf,
"The calibration file could not be found at the specified\n\
location. Please verify that the location is correct and\n\
permissions allow the file to be opened.",
		CDIALOG_ICON_ERROR,
		CDIALOG_BTNFLAG_OK | CDIALOG_BTNFLAG_HELP,
		CDIALOG_BTNFLAG_OK
	    );
	    CDialogSetTransientFor(NULL);

	    g_free(buf);

	    /* Set statusbar message */
	    StatusBarSetMesg(
		&jc->status_bar,
		"Could not open calibration file"
	    );

	    return(-1);
	}


	/* Close currently opened joystick if any */
	JCDoCloseJoystick(jc);


	/* Set new calibration file name */
	if(path != jc->calib_file)
	{
	    g_free(jc->calib_file);
	    jc->calib_file = g_strdup(path);
	}

	/* Set startup initialized joystick to FALSE so that the joystick
	 * gets initialized in the timeout callback
	 */
	jc->tried_load_device_on_init = FALSE;

	return(0);
}

/*
 *	Open joystick callback, fetches joystick device path and passes
 *	it to JCDoOpenJoystick(). 
 */
void JCJSOpenDeviceCB(gpointer data)
{
	static gboolean reenterent = FALSE;
	const gchar *cstrptr;
	GtkCombo *combo;
	jc_struct *jc = (jc_struct *)data;
	if(jc == NULL)
	    return;

	if(jc->processing)
	    return;

	if(reenterent)
	    return;
	else
	    reenterent = TRUE;


	/* Get current device name */
	combo = (GtkCombo *)jc->js_device_combo;
	if(combo != NULL)
	    cstrptr = gtk_entry_get_text(GTK_ENTRY(combo->entry));
	else
	    cstrptr = NULL;
	if(cstrptr != NULL)
	{
	    gchar path[PATH_MAX + NAME_MAX - 1];


	    /* Copy text from js device entry, JCDoOpenJoystick() needs
	     * it done this way.
	     */
	    strncpy(path, cstrptr, PATH_MAX + NAME_MAX);
	    path[PATH_MAX + NAME_MAX - 1] = '\0';

	    /* Open the joystick */
	    JCDoOpenJoystick(jc, path);
	}

	reenterent = FALSE;
} 


/*
 *	Procedure to open a joystick. Any joystick currently opened on
 *	the specified jc structure will be first closed with a call to
 *	JCDoCloseJoystick(). The has_changes member in the jc structure
 *	will be reset to FALSE.
 *
 *	The joystick specified by path will then  be attempted to be
 *	opened, any errors encountered will be printed on the message
 *	dialog. If opening was successful then all resources and messages
 *	will be updated to reflect the newly opened joystick.
 *
 *	Returns non-zero on error.
 */
gint JCDoOpenJoystick(jc_struct *jc, const gchar *path)
{
	gchar *strptr;
	GtkCombo *combo;
	gint status;


	if((jc == NULL) || (path == NULL))
	    return(-1);

	/* Reset has changes on jc when opening a new joystick */
	if(jc->has_changes)
	    jc->has_changes = FALSE;

	/* Close currently opened joystick if any, this will also
	 * destroy any axis layout widgets and the buttons in the
	 * buttons list. Plus any other related resources that need
	 * to be destroyed.
	 */
	JCDoCloseJoystick(jc);

	/* Flush close joystick messages */
	GTK_EVENTS_FLUSH

	/* Open the joystick, the value of status will contain the
	 * success or error code.
	 */
	status = JSInit(
	    &jc->jsd,
	    path,
	    jc->calib_file,
	    JSFlagNonBlocking
	);
	/* Cannot open joystick? */
	if(status != JSSuccess)
	{
	    /* Print message indicating the error code */
	    gchar *buf;
	    switch(status)
	    {
	      case JSBadValue:
		buf = g_strdup_printf(
"In invalid value was encountered while attempting to initialize the\n\
joystick. Please check that all values needed to initialize the\n\
joystick are specified and specified within their ranges (ie file paths\n\
and joystick value limits)."
		);
		break;

	      case JSNoAccess:
		buf = g_strdup_printf(
"Could not access the specified path(s), verify that the path(s)\n\
are specified correctly and that you have sufficient permission\n\
to access them. Also make sure that the joystick in question is\n\
actually connected, turned on (as needed), not in use by another\n\
program, and that the joystick driver or module is loaded (type\n\
`modprobe <driver_name>')."
		);
		break;

	      case JSNoBuffers:
		buf = g_strdup_printf(
"Insufficient memory available to initialize joystick."
		);
		break;

	      default:  /* JSError */
		buf = g_strdup_printf(
"Unknown error \"%i\" recieved while initializing joystick:\n\
\n\
    %s\n",
		    status, path
		);
		break;
	    }
	    CDialogSetTransientFor(jc->toplevel);
	    CDialogGetResponse(
"Joystick Initialization Failed",
buf,
"The joystick device could not be opened and initialized\n\
due to the specified error.\n",
		CDIALOG_ICON_ERROR,
		CDIALOG_BTNFLAG_OK,
		CDIALOG_BTNFLAG_OK 
	    );
	    CDialogSetTransientFor(NULL);
	    g_free(buf);

	    buf = g_strdup_printf(
		"Unable to open joystick device \"%s\"",
		path
	    );
	    StatusBarSetMesg(&jc->status_bar, buf);
	    g_free(buf);

	    return(-1);
	}

	/* Joystick opened successfully, begin updating values */

	/* Update the joystick device combo's entry value because the
	 * given path may differ from the one currently specified on
	 * the combo's entry widget.
	 */
	combo = (GtkCombo *)jc->js_device_combo;
	if(combo != NULL)
	    strptr = gtk_entry_get_text(GTK_ENTRY(combo->entry));
	else
	    strptr = NULL;

	if(path != strptr)
	{
	    gtk_entry_set_text(
		GTK_ENTRY(combo->entry),
		path
	    );
	}


	/* Destroy axises and buttons widgets and related resources */
	JCDestroyRepresentativeLayoutWidgets(
	    jc, &jc->layout_rep
	);
	JCDestroyLogicalLayoutWidgets(
	    jc, &jc->layout_logical
	);
	JCDestroyButtonsList(jc);

	/* Create axises and button widgets which should not reflect
	 * the values of the newly opened joystick specified on the
	 * jsd structure on the jc.
	 */
	JCCreateRepresentativeLayoutWidgets(
	    jc, &jc->layout_rep
	);
	JCCreateLogicalLayoutWidgets(
	    jc, &jc->layout_logical,
	    jc->layout_logical_parent
	);
	JCCreateButtonsList(jc);


	/* Update jc toplevel window title to contain the name of
	 * joystick device fetched from the newly opened jsd
	 */
	if(jc->jsd.name != NULL)
	{
	    gchar *title = g_strdup_printf(
		"%s: %s",
		PROG_NAME_FULL, jc->jsd.name
	    );
	    gtk_window_set_title(
		GTK_WINDOW(jc->toplevel),
		title
	    );
	    g_free(title);
	}

	/* Redraw axis and buttons layout widgets */
	JCDrawAxises(jc);
	JCDrawButtons(jc);

	/* Update information on status bar */
	StatusBarSetJSState(&jc->status_bar, 1);
	StatusBarSetMesg(&jc->status_bar, "Joystick initialized");

	return(0);
}

/*
 *	Procedure to close a joystick. This will close the (if opened)
 *	joystick device specified on the jc's jsd structure, any
 *	widgets on the axis layouts will be destroyed as well.
 *
 *	All values will be updated to reflect the closed joystick.
 */
void JCDoCloseJoystick(jc_struct *jc)
{
	GtkWidget *w;

	if(jc == NULL)
	    return;

	/* Reset has changes marker */
	if(jc->has_changes)
	    jc->has_changes = FALSE;

	/* Destroy axises and buttons widgets and related resources
	 * on the axises layout structures and buttons list.
	 */
	JCDestroyRepresentativeLayoutWidgets(
	    jc, &jc->layout_rep
	);
	JCDestroyLogicalLayoutWidgets(
	    jc, &jc->layout_logical
	);
	JCDestroyButtonsList(jc);


	/* Close the joystick device, this will reset all values
	 * on the jsd structure.
	 */
	JSClose(&jc->jsd);

	/* Update jc's toplevel window title */
	w = jc->toplevel;
	if(w != NULL)
	{
	    gtk_window_set_title(
	        GTK_WINDOW(jc->toplevel),
		PROG_NAME_FULL
	    );
	}

	/* Redraw axis and buttons layout */
	JCDrawAxises(jc);
	JCDrawButtons(jc);

	/* Update information on status bar */
	StatusBarSetJSState(&jc->status_bar, 0);
	StatusBarSetMesg(&jc->status_bar, "Joystick shutdown");
}
