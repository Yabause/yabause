#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/cfgfmt.h"
#include "../include/string.h"
#include "../include/fio.h"
#include "../include/disk.h"

#include "guiutils.h"
#include "cdialog.h"

#include "jc.h"
#include "config.h"


static js_data_struct **JCGetJSDList(
	const gchar *path, gint *total
);
static void JCWriteAxisCalibBlock(
	FILE *fp, js_data_struct *jsd, gint axis_num
);
static void JCWriteJSCalibBlock(FILE *fp, js_data_struct *jsd);
static gint JCDoWriteCalibration(
	const gchar *path,
	js_data_struct **jsd, int total_jsds,
	js_data_struct *src_jsd         /* Can be NULL */
);
gint JCDoSaveCalibration(jc_struct *jc, const gchar *path);
gint JCDoCalibrationCleanUp(jc_struct *jc, const gchar *path);


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
 *	Gets a list of js_data_struct from the calibration file specified
 *	by path. None of the structures will reference an opened joystick
 *	however its values must be deallocated with a call to JSClose().
 *
 *	The calling function must call JSClose() on each returned
 *	structure and deallocate each structure and the pointer array.
 */
js_data_struct **JCGetJSDList(
	const gchar *path, gint *total
)
{
	const gchar *dev_name;
	gint i, strc;
	gchar **strv;
	gint total_jsds = 0;
	js_data_struct **jsd = NULL, *jsd_ptr;


	if(total != NULL)
	    *total = 0;

	if(path == NULL)
	    return(jsd);


	/* Get list of device names specified in the calibration file */
	strv = JSLoadDeviceNamesUNIX(&strc, path);

	/* Allocate jsd list to match the number of device names just
	 * loaded above.
	 */
	total_jsds = strc;
	if(total_jsds > 0)
	{
	    /* Allocate more pointers in the jsd list */
	    jsd = (js_data_struct **)g_realloc(
		jsd,
		total_jsds * sizeof(js_data_struct *)
	    );
	    if(jsd == NULL)
	    {
		total_jsds = 0;
	    }
	    else
	    {
		/* Allocate each new jsd structure on the jsd list */
		for(i = 0; i < total_jsds; i++)
		{
		    jsd[i] = jsd_ptr = (js_data_struct *)g_malloc0(
			sizeof(js_data_struct)
		    );
		    if(jsd_ptr == NULL)
			continue;

		    /* Set up jsd values for the newly allocated local jsd
		     * structure. It is important that the values be set
		     * up currectly or else a pass to JSClose() will mess
		     * things up if it tries to deallocate resources that
		     * arn't actually allocated (ie the fd).
		     */
		    jsd_ptr->name = NULL;
		    jsd_ptr->axis = NULL;
		    jsd_ptr->total_axises = 0;
		    jsd_ptr->button = NULL;
		    jsd_ptr->total_buttons = 0;
		    jsd_ptr->device_name = NULL;
		    jsd_ptr->calibration_file = NULL;
		    jsd_ptr->events_received = 0;
		    jsd_ptr->events_sent = 0;
		    jsd_ptr->fd = -1;
		    jsd_ptr->flags = 0;
		    jsd_ptr->driver_version = 0;
		    jsd_ptr->last_calibrated = 0;
		    jsd_ptr->force_feedback = NULL;
		}
	    }
	}
	else
	{
	    g_free(jsd);
	    jsd = NULL;
	}

	/* Iterate through jsd list, loading calibration for each device
	 * specified by its respective device name in the device names
	 * list
	 */
	for(i = 0; i < total_jsds; i++)
	{
	    dev_name = strv[i];
	    if(dev_name == NULL)
		continue;

	    jsd_ptr = jsd[i];
	    if(jsd_ptr == NULL)
		continue;

	    /* Update device name and calibration file on the jsd */
	    g_free(jsd_ptr->device_name);
	    jsd_ptr->device_name = STRDUP(dev_name);

	    g_free(jsd_ptr->calibration_file);
	    jsd_ptr->calibration_file = STRDUP(path);

	    /* Load the calibration values from file for the device
	     * specified by dev_name in the specified jsd
	     *
	     * This will allocate axies and buttons and set their values
	     * from the data defined in the file specified by
	     * calibration_file on the specified jsd
	     */
	    JSLoadCalibrationUNIX(jsd_ptr);
	}

	/* Free device names, they are no longer needed */
	strlistfree(strv, strc);
	strv = NULL;
	strc = 0;

	/* Update returns */
	if(total != NULL)
	    *total = total_jsds;

	return(jsd);
}

/*
 *	Writes an axis calibration block for the specified axis_num
 *	on the jsd structure to the stream pointed to by fp.
 */
static void JCWriteAxisCalibBlock(
	FILE *fp, js_data_struct *jsd, gint axis_num
)
{
	js_axis_struct *axis_ptr;


	if((fp == NULL) || (jsd == NULL))
	    return;

	if(JSIsAxisAllocated(jsd, axis_num))
	    axis_ptr = jsd->axis[axis_num];
	else
	    return;

	/* Begin writing axis calibration block */
	fprintf(
	    fp,
	    "    BeginAxis = %i\n",
	    axis_num
	);
	fprintf(
	    fp,
	    "        Minimum = %i\n",
	    axis_ptr->min
	);
	fprintf(
	    fp,
	    "        Center = %i\n",
	    axis_ptr->cen
	);
	fprintf(
	    fp,
	    "        Maximum = %i\n",
	    axis_ptr->max
	);
	fprintf(
	    fp,
	    "        NullZone = %i\n",
	    axis_ptr->nz
	);
	/* Always enable tolorance */
	fprintf(
	    fp,
	    "        Tolorance = %i\n",
	    axis_ptr->tolorance
	);
	if(axis_ptr->flags & JSAxisFlagFlipped)
	    fprintf(
		fp,
		"        Flip\n"
	    );
	if(axis_ptr->flags & JSAxisFlagIsHat)
	    fprintf( 
		fp,
		"        IsHat\n"
	    );
	/* Correctional stuff (new since 1.5.0) */
	fprintf(
	    fp,
	    "        CorrectionLevel = %i\n",
	    axis_ptr->correction_level
	);
	fprintf(
	    fp,
	    "        DeadZoneMinimum = %i\n",
	    axis_ptr->dz_min
	);
	fprintf(
	    fp,
	    "        DeadZoneMaximum = %i\n",
	    axis_ptr->dz_max
	);
	fprintf(
	    fp,
	    "        CorrectionalCoefficientMinimum1 = %f\n",
	    axis_ptr->corr_coeff_min1
	);
	fprintf(
	    fp,
	    "        CorrectionalCoefficientMaximum1 = %f\n",
	    axis_ptr->corr_coeff_max1
	);
	fprintf(
	    fp,
	    "        CorrectionalCoefficientMinimum2 = %f\n",
	    axis_ptr->corr_coeff_min2
	);
	fprintf(
	    fp,
	    "        CorrectionalCoefficientMaximum2 = %f\n",
	    axis_ptr->corr_coeff_max2
	);

	fprintf(fp, "    EndAxis\n");

}

/*
 *	Writes a joystick calibration block for the joystick specified
 *	by jsd to the stream pointed to by fp.
 */
static void JCWriteJSCalibBlock(FILE *fp, js_data_struct *jsd)
{
	gint axis_num;

	if((fp == NULL) || (jsd == NULL))
	    return;

	/* Begin writing joystick calibration block */

	/* Begin joystick configuration block statement */
	fprintf(fp, "BeginJoystick = %s\n", jsd->device_name);

	/* Write friendly name of joystick device */
	if(jsd->name != NULL)
	{
	    gchar *tmp_name = g_strdup(jsd->name);
	    if(tmp_name != NULL)
	    {
		gchar *strptr;

		/* Terminate newlines */
		strptr = strchr(tmp_name, '\n');
		if(strptr != NULL)
		    *strptr = '\0';
		strptr = strchr(tmp_name, '\r');
		if(strptr != NULL)
		    *strptr = '\0';

		fprintf(fp, "    Name = %s\n", tmp_name);

		g_free(tmp_name);
		tmp_name = NULL;
	    }
	}

	/* Last calibrated time */
	fprintf(fp, "    LastCalibrated = %ld\n", jsd->last_calibrated);

	/* Write each axis */
	for(axis_num = 0; axis_num < jsd->total_axises; axis_num++)
	    JCWriteAxisCalibBlock(fp, jsd, axis_num);

	/* Skip buttons */

/* Other configuration that should be written needs to be done here */

	/* End joystick configuration block statement */
	fprintf(fp, "EndJoystick\n");
}

/*
 *	Writes the calibration file specified by path with respect to the
 *	given list of jsd structures.
 *
 *	If src_jsd is not NULL then any occurance of a matching device in
 *	the given jsd list with src_jsd will have src_jsd written instead
 *	and the jsd in the list will be ignored.
 *
 *	Returns non-zero on error.
 */
static gint JCDoWriteCalibration(
	const gchar *path,
	js_data_struct **jsd, int total_jsds,
	js_data_struct *src_jsd		/* Can be NULL */
)
{
	gint i;
	gboolean wrote_src_js = FALSE;
	FILE *fp;
	js_data_struct *jsd_ptr;
	const gchar *device_name, *src_device_name;

	if(path == NULL)
	    return(-1);

	/* If the source jsd is specified, then get its device
	 * name as src_device_name
	 */
	src_device_name = (src_jsd != NULL) ?
	    src_jsd->device_name : NULL;


	/* Open calibration file for writing */
	fp = FOpen(path, "w");
	if(fp != NULL)
	{
	    /* Write header */
	    fprintf(
		fp,
"# Joystick calibration file.\n\
# Generated by %s version %s.\n\
#\n",
		PROG_NAME_FULL, PROG_VERSION
	    );

	    /* Write each joystick device block */
	    for(i = 0; i < total_jsds; i++)
	    {
		jsd_ptr = jsd[i];
		if(jsd_ptr == NULL)
		    continue;

		/* Skip if device name is not specified, because we
		 * need the device name specified on the jsd to check
		 * if the device name matches that of the source jsd.
		 */
		device_name = jsd_ptr->device_name;
		if(device_name == NULL)
		    continue;

		/* Check if joystick device values we are about to
		 * write matches the device name of the source joystick
		 * device.
		 */
		if((src_device_name != NULL) ?
		    !strcmp(device_name, src_device_name) : 0
		)
		{
		    /* This is the source jsd, do not use the calibration
		     * for the jsd from the jsd list, instead use the
		     * values for the jsd from the jc structure (since
		     * that is the source joystick).
		     */
		    JCWriteJSCalibBlock(fp, src_jsd);

		    /* Mark that we already wrote the source jsd */
		    wrote_src_js = TRUE;
		}
		else
		{

		    /* Calibration block for some other joystick,
		     * just write the calibration for that joystick
		     * to the calibration file as is.
		     */
		    JCWriteJSCalibBlock(fp, jsd_ptr);
		}
	    }

	    /* All joystick calibration blocks from the local jsd list
	     * should now all be written to the file. But we need to
	     * double check if the source joystick calibration information
	     * was written or not. Since this might occure if the source
	     * joystick was not specified at all in the calibration file.
	     */
	    if(!wrote_src_js && (src_jsd != NULL))
	    {
		/* Looks like the source joystick calibration values were
		 * not written, so write it now.
		 */
		JCWriteJSCalibBlock(fp, src_jsd);

		/* Mark that we have now written values for the source
		 * joystick device.
		 */
		wrote_src_js = TRUE;
	    }

	    /* Close calibration file */
	    FClose(fp);
	    fp = NULL;
	}

	return(0);
}


/*
 *	Saves calibration values in the jsd structure on the specified
 *	jc to the given calibration file specified by path.
 *
 *	If path is NULL then the default calibration file name in the
 *	user's home directory will be used.
 *
 *	The jsd from the given jc structure will have its member
 *	last_calibration updated to the current time.
 *
 *	Returns non-zero on error. If and only if calibration was saved
 *	successfully, then has_changes on the jc structure will be reset
 *	to FALSE.
 */
gint JCDoSaveCalibration(jc_struct *jc, const gchar *path)
{
	gint i, status, total_jsds;
	gchar *calib_file, *dev_name;
	js_data_struct *src_jsd, *jsd_ptr, **jsd;

	if(jc == NULL)
	    return(-1);

	/* Get pointer to source joystick data */
	src_jsd = &jc->jsd;

	/* Update last calibration time on the source jsd */
	src_jsd->last_calibrated = time(NULL);

	/* Calibration file not specified? */
	if(path == NULL)
	{
	    /* Use default calibration file assumed located in the
	     * user's home directory
	     */
	    const gchar *home = g_getenv(ENV_VAR_NAME_HOME);
#ifdef JSDefaultCalibration
	    calib_file = STRDUP(PrefixPaths(
		(home != NULL) ? home : "/",
		JSDefaultCalibration
	    ));
#else
	    calib_file = STRDUP(PrefixPaths(
		(home != NULL) ? home : "/",
		".joystick"
	    ));
#endif
	}
	else
	{
	    calib_file = STRDUP(path);
	}

	/* Get the device name for the jsd structure on the given jc
	 * structure. If no device name is specified then use the
	 * default device name.
	 */
	if(src_jsd->device_name != NULL)
	    dev_name = STRDUP(src_jsd->device_name);
	else
#ifdef JSDefaultDevice
	    dev_name = STRDUP(JSDefaultDevice);
#else
	    dev_name = STRDUP("/dev/js0");
#endif

	/* Get jsd list from the calibration file */
	jsd = JCGetJSDList(calib_file, &total_jsds);

	/* Write calibration specified by the jsd list to the
	 * calibration file
	 */
	status = JCDoWriteCalibration(
	    calib_file,
	    jsd, total_jsds,
	    src_jsd
	);

	/* Delete the jsd list */
	for(i = 0; i < total_jsds; i++)
	{
	    jsd_ptr = jsd[i];
	    if(jsd_ptr == NULL)
		continue;

	    JSClose(jsd_ptr);
	    g_free(jsd_ptr);
	}
	g_free(jsd);
	jsd = NULL;
	total_jsds = 0;

	/* Check if we successfully saved the joystick calibration to
	 * the calibration file
	 */
	if(!status)
	{
	    /* Reset has changes mark to FALSE since calibration was
	     * successfully saved
	     */
	    if(jc->has_changes)
		jc->has_changes = FALSE;

	    /* Update statusbar message */
	    StatusBarSetMesg(
		&jc->status_bar,
		"Calibration saved"
	    );
	}
	else
	{
	    /* Save failed */
	    gchar *buf = g_strdup_printf(
"Unable to save calibration of joystick:\n\
\n\
    %s\n\
\n\
To calibration file:\n\
\n\
    %s\n",
		dev_name, calib_file
	    );

	    StatusBarSetMesg(&jc->status_bar, NULL);

	    CDialogSetTransientFor(jc->toplevel);
	    CDialogGetResponse(
"Save Calibration Failed",
buf,
"The joystick calibration could not be saved due to an\n\
an error. Please verify that the specified location of\n\
the calibration file is valid.",
		CDIALOG_ICON_ERROR,
		CDIALOG_BTNFLAG_OK,
		CDIALOG_BTNFLAG_OK
	    );
	    CDialogSetTransientFor(NULL);
	    g_free(buf);
	}

	g_free(calib_file);
	g_free(dev_name);

	return(0);
}


/*
 *	Checks the specified calibration file and loads all configured
 *	joysticks, any device found to be defined in the calibration
 *	file but does not exist will be prompted to be removed.
 *
 *	The calibration file will then be saved at the end of the check.
 *
 *	Returns non-zero on error.
 */
gint JCDoCalibrationCleanUp(jc_struct *jc, const gchar *path)
{
	gint i, n, status;
	gboolean yes_to_all = FALSE;
	const gchar *dev_name;
	gchar *buf, *calib_file;
	js_data_struct *jsd_ptr, **jsd;
	gint total_jsds;
	js_attribute_struct *jsattrib, *jsattrib_ptr;
	gint total_jsattribs;
	GtkWidget *toplevel;


	if(jc == NULL)
	    return(-1);

	toplevel = jc->toplevel;

	/* Calibration file name not specified? */
	if(path == NULL)
	{
	    /* Calibration file name not specified, so use default
	     * calibration file name assumed located in the user's home
	     * directory
	     */
	    const gchar *home = g_getenv(ENV_VAR_NAME_HOME);
#ifdef JSDefaultCalibration
	    calib_file = STRDUP(PrefixPaths(
		(home != NULL) ? home : "/",
		JSDefaultCalibration
	    ));
#else
	    calib_file = STRDUP(PrefixPaths(
		(home != NULL) ? home : "/",
		".joystick"
	    ));
#endif
	}
	else
	{
	    calib_file = STRDUP(path);
	}

	/* Report initial status message */
	StatusBarSetMesg(
	    &jc->status_bar,
	    "Cleaning calibration file..."
	);

	/* Get the jsd list from the calibration file */
	jsd = JCGetJSDList(calib_file, &total_jsds);

	/* Get joystick attributes list */
	jsattrib = JSGetAttributesList(&total_jsattribs, calib_file);


	/* Iterate through the jsd list and compare each joystick
	 * specified by the jsd list to those defined in the jsattrib
	 * list
	 *
	 * If it does not exist then prompt for removal.
	 */
	for(i = 0; i < total_jsds; i++)
	{
	    jsd_ptr = jsd[i];
	    if(jsd_ptr == NULL)
		continue;

	    if(jsd_ptr->device_name == NULL)
		continue;

	    /* Report progress */
	    buf = g_strdup_printf(
		"Checking: %s",
		jsd_ptr->device_name
	    );
	    StatusBarSetMesg(&jc->status_bar, buf);
	    g_free(buf);
	    GTK_EVENTS_FLUSH

	    /* Iterate through joystick attributes list */
	    for(n = 0; n < total_jsattribs; n++)
	    {
		jsattrib_ptr = &jsattrib[n];
		dev_name = jsattrib_ptr->device_name;
		if(dev_name == NULL)
		    continue;

		/* Name of joystick device matches device name from
		 * the attributes list?
		 */
		if(!strcmp(jsd_ptr->device_name, dev_name))
		    break;
	    }
	    /* Joystick not found in attributes list? */
	    if(n >= total_jsattribs)
	    {
		/* Prompt for removal */
		if(yes_to_all)
		{
		    status = CDIALOG_RESPONSE_YES_TO_ALL;	
		}
		else
		{
		    buf = g_strdup_printf(
"Joystick:\n\
\n\
    %s\n\
\n\
Does not appear to be installed or set up on the system.\n\
\n\
Do you wish to remove its defination from the calibration\n\
file?\n",
			jsd_ptr->device_name
		    );
		    CDialogSetTransientFor(toplevel);
		    status = CDialogGetResponse(
"Clean Up Confirmation",
buf,
"The device was found to be defined in the calibration file\n\
but not actually installed or set up on the system. It may\n\
be unplugged or its driver is not loaded. If you are\n\
certain that this device is no longer needed then click on\n\
`Yes' to remove its defination in the calibration file,\n\
otherwise click on `No' to keep it.",
			CDIALOG_ICON_QUESTION,
			CDIALOG_BTNFLAG_YES | CDIALOG_BTNFLAG_YES_TO_ALL |
			    CDIALOG_BTNFLAG_NO | CDIALOG_BTNFLAG_HELP,
			CDIALOG_BTNFLAG_NO
		    );
		    CDialogSetTransientFor(NULL);
		    g_free(buf);
		}
		switch(status)
		{
		  case CDIALOG_RESPONSE_YES_TO_ALL:
		    yes_to_all = TRUE;
		  case CDIALOG_RESPONSE_YES:
		    /* Yes, remove this device from the jsd list */
		    JSClose(jsd_ptr);
		    g_free(jsd_ptr);
		    jsd[i] = jsd_ptr = NULL;
		    break;
		}
	    }
	    /* Was the device removed? */
	    if(jsd_ptr == NULL)
		continue;


	}

	/* Delete the joystick attributes list */
	JSFreeAttributesList(jsattrib, total_jsattribs);
	jsattrib = NULL;
	total_jsattribs = 0;

	/* Write calibration specified by the jsd list to file */
	status = JCDoWriteCalibration(
	    calib_file,
	    jsd, total_jsds,
	    NULL		/* No source jsd */
	);

	/* Delete the jsd list */
	for(i = 0; i < total_jsds; i++)
	{
	    jsd_ptr = jsd[i];
	    if(jsd_ptr == NULL)
		continue;

	    JSClose(jsd_ptr);
	    g_free(jsd_ptr);
	}
	g_free(jsd);
	jsd = NULL;
	total_jsds = 0;

	/* Report final status message */
	StatusBarSetMesg(
	    &jc->status_bar,
	    "Calibration file clean up done"
	);

	g_free(calib_file);

	return(0);
}
