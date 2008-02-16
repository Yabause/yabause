#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "../include/cfgfmt.h"
#include "../include/string.h"
#include "../include/disk.h"
#include "../include/fio.h"

#include "../include/jsw.h"


void JSResetAllAxisTolorance(js_data_struct *jsd);
int JSLoadCalibrationUNIX(js_data_struct *jsd);
char **JSLoadDeviceNamesUNIX(
	int *total, const char *calibration
);


#define ATOI(s)         (((s) != NULL) ? atoi(s) : 0)
#define ATOL(s)         (((s) != NULL) ? atol(s) : 0)
#define ATOF(s)         (((s) != NULL) ? atof(s) : 0.0f)
#define STRDUP(s)       (((s) != NULL) ? strdup(s) : NULL)

#define MAX(a,b)        (((a) > (b)) ? (a) : (b))
#define MIN(a,b)        (((a) < (b)) ? (a) : (b))
#define CLIP(a,l,h)     (MIN(MAX((a),(l)),(h)))
#define STRLEN(s)       (((s) != NULL) ? strlen(s) : 0)
#define STRISEMPTY(s)   (((s) != NULL) ? (*(s) == '\0') : 1)


/*
 *      Loads the calibration data from the calibration file specifeid
 *      on the given jsd structure. First an entry for the device
 *      specified by the jsd structure is looked for in the calibration
 *      file if (and only if) it is found then the axis and button
 *      values for that device will be loaded.
 *
 *      Additional axises and buttons may be allocated on the given
 *      jsd structure by this function if they are found to be defined
 *      for the device in question in the calibration file.
 */
int JSLoadCalibrationUNIX(js_data_struct *jsd)
{
	char is_this_device = 0;
	const char *this_device_name;
	char *line_buf;
  
	FILE *fp;
  
	char parm[CFG_PARAMETER_MAX];
	char val[CFG_VALUE_MAX];
	int lines_read = 0;

	int axis_num, button_num;
	js_axis_struct *axis_ptr;
	js_button_struct *button_ptr;


	if(jsd == NULL)
	    return(-1);

	/* Device name must be specified */
	this_device_name = (const char *)jsd->device_name;
	if(this_device_name == NULL)
	    return(-1);

	/* Calibration file must be specified */
	if(STRISEMPTY(jsd->calibration_file))
	    return(-1);

	/* Open the calibration file */
	fp = FOpen(jsd->calibration_file, "rb");
	if(fp == NULL)
	    return(-1);

/* Reads the next line from fp, first deleting line_buf and then
 * allocating the new line to line_buf, lines_read will be incremented
 */
#define GET_NEXT_LINE	{			\
 const char *s;					\
						\
 /* Read next non-comment line */		\
 free(line_buf);				\
 line_buf = FReadNextLineAllocCount(		\
  fp, UNIXCFG_COMMENT_CHAR, &lines_read		\
 );						\
 if(line_buf == NULL)				\
  break;					\
						\
 /* Fetch parameter */				\
 s = StringCfgParseParm(line_buf);		\
 if(s == NULL)					\
  continue;					\
 strncpy(parm, s, sizeof(parm));		\
 parm[sizeof(parm) - 1] = '\0';			\
						\
 /* Fetch value */				\
 s = StringCfgParseValue(line_buf);		\
 if(s == NULL)					\
  s = "0";					\
 strncpy(val, s, sizeof(val));			\
 val[sizeof(val) - 1] = '\0';			\
}

	/* Begin reading the calibration file */
	line_buf = NULL;
	while(1)
	{
	    GET_NEXT_LINE

	    /* Start of joystick device block? */
	    if(!strcasecmp(parm, "BeginJoystick"))
	    {
		/* Joystick device name must match the value for
		 * the BeginJoystick parameter, this is so that we
		 * know that this is a configuration block for
		 * this device (jsd->device_name)
		 */
		if(strcmp(val, this_device_name))
		    is_this_device = 0;
		else
		    is_this_device = 1;

		/* Enter loop to read and handle each line for this
		 * configuration block
		 */
		while(1)
		{
		    GET_NEXT_LINE

		    /* BeginAxis */
		    if(!strcasecmp(parm, "BeginAxis") &&
		       is_this_device
		    )
		    {
			/* Get axis number and check if it matches one
			 * of the axises on the jsd
			 */
			axis_num = ATOI(val);
			if(JSIsAxisAllocated(jsd, axis_num))
			{
			    axis_ptr = jsd->axis[axis_num];
			}
			else if((axis_num >= jsd->total_axises) &&
				(axis_num >= 0)
			)
			{
			    /* Need to allocate more axises */
			    int i, p = jsd->total_axises;
			    jsd->total_axises = axis_num + 1;
			    jsd->axis = (js_axis_struct **)realloc(
				jsd->axis,
				jsd->total_axises * sizeof(js_axis_struct *)
			    );
			    if(jsd->axis == NULL)
			    {
				jsd->total_axises = 0;
				axis_ptr = NULL;
				axis_num = -1;
			    }
			    else
			    {
				for(i = p; i < jsd->total_axises; i++)
				    jsd->axis[i] = NULL;
				jsd->axis[axis_num] = axis_ptr = (js_axis_struct *)calloc(
				    1, sizeof(js_axis_struct)
				);
			    }
			}
			else
			{
			    axis_ptr = NULL;
			    axis_num = -1;
			}

			/* Enter loop to read and handle each line for
			 * this configuration block
			 */
			while(1)
			{
			    GET_NEXT_LINE

			    /* Minimum */
			    if(!strcasecmp(parm, "Minimum"))
			    {
				if(axis_ptr != NULL)
				    axis_ptr->min = ATOI(val);
			    }
			    /* Maximum */
			    else if(!strcasecmp(parm, "Maximum"))
			    {
				if(axis_ptr != NULL)
				    axis_ptr->max = ATOI(val);
			    }
			    /* Center */
			    else if(!strcasecmp(parm, "Center"))
			    {
				if(axis_ptr != NULL)
				    axis_ptr->cen = ATOI(val);
			    }
			    /* NullZone */
			    else if(!strcasecmp(parm, "NullZone"))
			    {
				if(axis_ptr != NULL)
				    axis_ptr->nz = MAX(ATOI(val), 0);
			    }
			    /* Tolorance */
			    else if(!strcasecmp(parm, "Tolorance"))
			    {
				if(axis_ptr != NULL)
				{
				    axis_ptr->tolorance = MAX(ATOI(val), 0);
				    axis_ptr->flags |= JSAxisFlagTolorance;
				}
			    }
			    /* Flip */
			    else if(!strcasecmp(parm, "Flip") ||
				    !strcasecmp(parm, "Flipped")
			    )
			    {
				if(axis_ptr != NULL)
				    axis_ptr->flags |= JSAxisFlagFlipped;
			    }
			    /* IsHat */
			    else if(!strcasecmp(parm, "IsHat"))
			    {
				if(axis_ptr != NULL)
				    axis_ptr->flags |= JSAxisFlagIsHat;
			    }

			    /* Correctional Parameters (Version 1.5.x) */
			    /* CorrectionLevel */
			    else if(!strcasecmp(parm, "CorrectionLevel"))
			    {
				if(axis_ptr != NULL)
				    axis_ptr->correction_level = MAX(ATOI(val), 0);
			    }
			    /* DeadZoneMinimum */
			    else if(!strcasecmp(parm, "DeadZoneMinimum") ||
				    !strcasecmp(parm, "DeadZoneMin") ||
				    !strcasecmp(parm, "DZMin")
			    )
			    {
				if(axis_ptr != NULL)
				    axis_ptr->dz_min = ATOI(val);
			    }
			    /* DeadZoneMaximum */
			    else if(!strcasecmp(parm, "DeadZoneMaximum") ||
				    !strcasecmp(parm, "DeadZoneMax") ||
				    !strcasecmp(parm, "DZMax")
			    )
			    {
				if(axis_ptr != NULL)
				    axis_ptr->dz_max = ATOI(val);
			    }
			    /* CorrectionalCoefficientMinimum1 */
			    else if(!strcasecmp(parm, "CorrectionalCoefficientMinimum1") ||
				    !strcasecmp(parm, "CorrCoeffMin1")
			    )
			    {
				if(axis_ptr != NULL)
				    axis_ptr->corr_coeff_min1 = CLIP(ATOF(val), 0.0, 1.0);
			    }
			    /* CorrectionalCoefficientMaximum1 */
			    else if(!strcasecmp(parm, "CorrectionalCoefficientMaximum1") ||
				    !strcasecmp(parm, "CorrCoeffMax1")
			    )
			    {
				if(axis_ptr != NULL)
				    axis_ptr->corr_coeff_max1 = CLIP(ATOF(val), 0.0, 1.0);
			    }
			    /* CorrectionCoefficientMinimum2 */
			    else if(!strcasecmp(parm, "CorrectionCoefficientMinimum2") ||
				    !strcasecmp(parm, "CorrCoeffMin2")
			    )
			    {
				if(axis_ptr != NULL)
				    axis_ptr->corr_coeff_min2 = CLIP(ATOF(val), 0.0, 1.0);
			    }
			    /* CorrectionCoefficientMaximum2 */
			    else if(!strcasecmp(parm, "CorrectionCoefficientMaximum2") ||
				    !strcasecmp(parm, "CorrCoeffMax2")
			    )
			    {
				if(axis_ptr != NULL)
				    axis_ptr->corr_coeff_max2 = CLIP(ATOF(val), 0.0, 1.0);
			    }

			    /* EndAxis */
			    else if(!strcasecmp(parm, "EndAxis"))
			    {
				axis_ptr = NULL;
				axis_num = -1;
				break;
			    }
			}	/* Read axis block loop */
		    }
		    /* BeginButton */
		    else if(!strcasecmp(parm, "BeginButton") &&
			    is_this_device
		    )
		    {
			/* Get button number and check if it matches one
			 * of the buttons on the jsd
			 */
			button_num = ATOI(val);
			if(JSIsButtonAllocated(jsd, button_num))
			{
			    button_ptr = jsd->button[button_num];
			}
			else if((button_num >= jsd->total_buttons) &&
				(button_num >= 0)
			)
			{
			    /* Need to allocate more buttons */
			    int i, p = jsd->total_buttons;
			    jsd->total_buttons = button_num + 1;
			    jsd->button = (js_button_struct **)realloc(
				jsd->button,
				jsd->total_buttons * sizeof(js_button_struct *)
			    );
			    if(jsd->button == NULL)
			    {
				jsd->total_buttons = 0;
				button_ptr = NULL;
				button_num = -1;
			    }
			    else
			    {
				for(i = p; i < jsd->total_buttons; i++)
				    jsd->button[i] = NULL;
				jsd->button[button_num] = button_ptr = (js_button_struct *)calloc(
				    1, sizeof(js_button_struct)
				);
			    }
			}
			else
			{
			    button_ptr = NULL;
			    button_num = -1;
			}

			/* Enter loop to read and handle each line for
			 * this configuration block
			 */
			while(1)
			{
			    GET_NEXT_LINE

			    /* EndButton */
			    if(!strcasecmp(parm, "EndButton"))
			    {
				button_ptr = NULL;
				button_num = -1;
				break;
			    }
			}	/* Read button block loop */
		    }
		    /* Name */
		    else if(!strcasecmp(parm, "Name") &&
			    is_this_device
		    )
		    {
			free(jsd->name);
			jsd->name = STRDUP(val);
		    }
		    /* LastCalibrated */
		    else if(!strcasecmp(parm, "LastCalibrated") &&
			    is_this_device
		    )
		    {
			jsd->last_calibrated = MAX(ATOL(val), 0l);
		    }
		    /* EndJoystick */
		    else if(!strcasecmp(parm, "EndJoystick"))
		    {
			is_this_device = 0;
			break;
		    }
		    /* Other parameter? */
		    else
		    {
			/* Other parameter, ignore */
		    }
		}	/* Read device block loop */
	    }
	    /* Other parameter? */
	    else
	    {
		/* Other parameter, ignore */
	    }
	}	/* Begin reading file */

#undef GET_NEXT_LINE

	/* Delete line buffer */
	free(line_buf);

	/* Close calibration file */
	FClose(fp);

	return(0);
}

/*
 *	Gets a list of calibrated devices found in the specified
 *	calibration file.
 *
 *	The returned list of strings and the pointer array must be
 *	deallocated by the calling function.
 */
char **JSLoadDeviceNamesUNIX(
	int *total, const char *calibration
)
{
	int lines_read = 0;
	char *line_buf;
	char parm[CFG_PARAMETER_MAX];
	char val[CFG_VALUE_MAX];

	FILE *fp;

	int strc = 0;
	char **strv = NULL;


	if(total != NULL)
	    *total = 0;

	/* Given calibration file exists? */
	if(STRISEMPTY(calibration))
	    return(strv);   

	/* Open the calibration file */
	fp = FOpen(calibration, "rb");
	if(fp == NULL)
	    return(strv);

/* Reads the next line from fp, first deleting line_buf and then
 * allocating the new line to line_buf, lines_read will be incremented
 */
#define GET_NEXT_LINE	{		\
 const char *s;				\
					\
 /* Read next non-comment line */	\
 free(line_buf);			\
 line_buf = FReadNextLineAllocCount(	\
  fp, UNIXCFG_COMMENT_CHAR, &lines_read	\
 );					\
 if(line_buf == NULL)			\
  break;				\
					\
 /* Fetch parameter */			\
 s = StringCfgParseParm(line_buf);	\
 if(s == NULL)				\
  continue;				\
 strncpy(parm, s, sizeof(parm));	\
 parm[sizeof(parm) - 1] = '\0';		\
					\
 /* Fetch value */			\
 s = StringCfgParseValue(line_buf);	\
 if(s == NULL)				\
  s = "0";				\
 strncpy(val, s, sizeof(val));		\
 val[sizeof(val) - 1] = '\0';		\
}

	/* Begin reading the calibration file */
	line_buf = NULL;
	while(1)
	{
	    GET_NEXT_LINE

	    /* BeginJoystick */
	    if(!strcasecmp(parm, "BeginJoystick"))
	    {
		int n = strc;
		strc = n + 1;
		strv = (char **)realloc(
		    strv,
		    strc * sizeof(char *)
		);
		if(strv == NULL)
		{
		    strc = 0;
		    continue;
		}

		strv[n] = STRDUP(val);

		/* Enter loop to read and handle each line for this
		 * configuration block
		 */
		while(1)
		{
		    GET_NEXT_LINE

		    /* EndJoystick */
		    if(!strcasecmp(parm, "EndJoystick"))
		    {
			break;
		    }
		}       /* Read device block loop */
	    }
	}	/* Begin reading calibration file */

#undef GET_NEXT_LINE

	/* Delete line buffer */
	free(line_buf);

	/* Close the calibration file */
	FClose(fp);

	/* Update total strings returned */
	if(total != NULL)
	    *total = strc;

	return(strv);
}
