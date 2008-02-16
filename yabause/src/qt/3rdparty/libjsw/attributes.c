#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include "../include/string.h"
#include "../include/jsw.h"


js_attribute_struct *JSGetAttributesList(
	int *total, const char *calibration
);
void JSFreeAttributesList(js_attribute_struct *list, int total);


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
 *	Gets the Joystick Attributes list for all joysticks accessable
 *	(configured or not) on the system regardless if the joystick is
 *	already opened or not.
 *
 *	The returned list must be deleted by calling
 *	JSFreeAttributesList().
 *
 *	If the specified calibration file calibration is NULL then the
 *	is_configured and name values will not be obtained.
 */
js_attribute_struct *JSGetAttributesList(
	int *total, const char *calibration
)
{
	int total_attribs = 0;
	js_attribute_struct *attrib = NULL, *attrib_ptr;

	if(total != NULL)
	    *total = 0;

#if defined(__linux__) || defined(__FreeBSD__)
	/* Begin fetching Joystick Attributes list */
	if(1)
	{
	    int i, j, total_dev_names;
	    const char *s;
	    char **dev_name, dev_path[PATH_MAX + NAME_MAX];
	    js_data_struct jsd;

#define JS_ATTRIB_LIST_APPEND	{		\
 const int n = MAX(total_attribs, 0);		\
 total_attribs = n + 1;				\
 attrib = (js_attribute_struct *)realloc(	\
  attrib,					\
  total_attribs * sizeof(js_attribute_struct)	\
 );						\
 if(attrib == NULL) {				\
  total_attribs = 0;				\
  attrib_ptr = NULL;				\
 } else {					\
  attrib_ptr = &attrib[n];			\
  memset(attrib_ptr, 0x00, sizeof(js_attribute_struct)); \
 }						\
}

	    /* Begin searching for each device until one is missing,
	     * look in the "/dev/js#" paths where # is a number
	     */
	    for(i = 0; 1; i++)
	    {
		/* Format joystick device path */
#if defined(__FreeBSD__)
		sprintf(dev_path, "/dev/joy%i", i);
#else
		sprintf(dev_path, "/dev/js%i", i);
#endif

		/* Joystick device does not exist? */
		if(access(dev_path, F_OK))
		     break;

		/* Append a new Joystick Attribute to the list */
		JS_ATTRIB_LIST_APPEND
		if(attrib_ptr != NULL)
		{
		    /* Try to open the device */
		    const int fd = open(dev_path, O_RDONLY);

		    /* Record device path as the Joystick Attribute's
		     * device name
		     */
		    free(attrib_ptr->device_name);
		    attrib_ptr->device_name = STRDUP(dev_path);

		    /* Unable to open device? */
		    if(fd < 0)
		    {
			/* Could not open it, handle errno */
			switch(errno)
			{
			  case ENODEV:
			  case ENFILE:
			    attrib_ptr->not_accessable = 1;
			    break;

			  default:
			    /* All else assume is in use */
			    attrib_ptr->is_in_use = 1;
			    break;
			}
		    }
		    else
		    {
			/* Opened it just fine, now close it */
			close(fd);
		    }
		}
	    }

#if defined(__linux__)
	    /* Repeat the above for USB devices, begin searching in
	     * "/dev/input/js#" paths where # is a number
	     */
	    for(i = 0; 1; i++)
	    {
		/* Format joystick device path */
		sprintf(dev_path, "/dev/input/js%i", i);

		/* Joystick device does not exist? */
		if(access(dev_path, F_OK))
		     break;

		/* Append a new Joystick Attribute to the list */
		JS_ATTRIB_LIST_APPEND
		if(attrib_ptr != NULL)
		{
		    /* Try to open the device */
		    const int fd = open(dev_path, O_RDONLY);

		    /* Record device path as the Joystick Attribute's
		     * device name
		     */
		    free(attrib_ptr->device_name);
		    attrib_ptr->device_name = STRDUP(dev_path);

		    /* Unable to open device? */
		    if(fd < 0)
		    {
			/* Could not open it, handle errno */
			switch(errno)
			{
			  case ENODEV:
			  case ENFILE:
			    attrib_ptr->not_accessable = 1;
			    break;

			  default:
			    /* All else assume is in use */
			    attrib_ptr->is_in_use = 1;
			    break;
			}
		    }
		    else
		    {
			/* Opened it just fine, now close it */
			close(fd);
		    }
		}
	    }
#endif

	    /* The Joystick Attributes list has been obtained, now
	     * scan through calibration file (if calibration is not
	     * NULL) and see which devices are configured properly
	     */
	    dev_name = JSLoadDeviceNamesUNIX(
		&total_dev_names, calibration
	    );

	    /* Iterate through the Joystick Attributes list */
	    for(i = 0; i < total_attribs; i++)
	    {
		attrib_ptr = &attrib[i];
		if(STRISEMPTY(attrib_ptr->device_name))
		    continue;

		/* Iterate through device names, see if a device name
		 * in the list matches this device's name
		 */
		for(j = 0; j < total_dev_names; j++)
		{
		    s = dev_name[j];
		    if(STRISEMPTY(s))
			continue;

		    /* Device name does not match device name on this
		     * Joystick Attribute?
		     */
		    if(strcmp(s, attrib_ptr->device_name))
		        continue;

		    /* This device is in the device names list, so
		     * mark it as being configured properly
		     */
		    attrib_ptr->is_configured = 1;

		    /* Get calibration info for this device, first
		     * reset values on the jsd and then set the device
		     * name on the jsd and call the calibration file
		     * loader
		     */
		    memset(&jsd, 0x00, sizeof(js_data_struct));
		    jsd.fd = -1;
		    jsd.device_name = STRDUP(s);
		    jsd.calibration_file = STRDUP(calibration);
		    JSLoadCalibrationUNIX(&jsd);

		    /* Update values to attribute */
		    free(attrib_ptr->name);
		    attrib_ptr->name = STRDUP(jsd.name);

		    /* Delete the jsd's resources */
		    JSClose(&jsd);

		    break;
		}
	    }

	    /* Free loaded device names */
	    strlistfree(dev_name, total_dev_names);

#undef JS_ATTRIB_LIST_APPEND
	}
#else
#warning JSGetAttributesList() Does not support this platform
#endif

	/* Update returns */
	if(total != NULL)
	    *total = total_attribs;
 
	return(attrib);
}

/*
 *	Deletes the Joystick Attributes list.
 */
void JSFreeAttributesList(js_attribute_struct *list, int total)
{
	int i;
	js_attribute_struct *a;

	for(i = 0; i < total; i++)
	{
	    a = &list[i];

	    free(a->name);
	    free(a->device_name);
	}
	free(list);
}

