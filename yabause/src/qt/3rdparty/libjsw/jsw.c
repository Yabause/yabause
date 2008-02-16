#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#if defined(__FreeBSD__)
# include <sys/joystick.h>
#endif

#include "../include/forcefeedback.h"

#include "../include/string.h"
#include "../include/disk.h"
#include "../include/fio.h"

#include "../include/jsw.h"


int JSInit(
	js_data_struct *jsd,
	const char *device,
	const char *calibration,
	unsigned int flags
);
static void SetAxisValue(js_axis_struct *axis, int value, time_t t);
static void SetButtonValue(js_button_struct *button, int value, time_t t);
int JSUpdate(js_data_struct *jsd);
void JSClose(js_data_struct *jsd);


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
 *      Initializes the joystick and stores the new initialized values
 *      into the jsd structure.
 *
 *      If the device is not specified (set to NULL), then it will
 *      be defauled to JSDefaultDevice.
 *
 *      If the calibration file is not specified (set to NULL), then
 *      it will be defaulted to JSDefaultCalibration. The HOME
 *      enviroment value will be used as the prefix to the path of
 *      JSDefaultCalibration. The calibration file does not have to
 *      exist.
 *
 *	Available flags are:
 *
 *	JSFlagNonBlocking		Open in non-blocking mode.
 *	JSFlagForceFeedback		Open in read/write mode.
 */
int JSInit(
	js_data_struct *jsd,
	const char *device,
	const char *calibration,
	unsigned int flags
)
{
	int i;
	js_axis_struct *axis;
	js_button_struct *button;

#if defined(__linux__) || defined(__FreeBSD__)
	unsigned char axes = 2;
	unsigned char buttons = 2;
	int version = 0x000800;

# ifndef LINUX_JS_NAME_MAX
#  define LINUX_JS_NAME_MAX	128
# endif

	char name[LINUX_JS_NAME_MAX] = "Unknown";

#endif	/* __linux__ || __FreeBSD__ */


	if(jsd == NULL)
	    return(JSBadValue);

	/* Reset values */
	jsd->name = NULL;

	jsd->axis = NULL;
	jsd->total_axises = 0;

	jsd->button = NULL;
	jsd->total_buttons = 0;

	jsd->device_name = NULL;
	jsd->calibration_file = NULL;

	jsd->events_received = 0;
	jsd->events_sent = 0;

	jsd->fd = -1;
	jsd->flags = 0;
	jsd->driver_version = 0;
	jsd->last_calibrated = 0;
	jsd->force_feedback = NULL;


	/* Set default device name as needed */
	if(device == NULL)
	    device = JSDefaultDevice;

	/* Set default calibration file name as needed */
	if(calibration == NULL)
	{
	    const char *home = getenv("HOME");
	    calibration = PrefixPaths(
		(home != NULL) ? home : "/",
		JSDefaultCalibration
	    );
	    if(calibration == NULL)
		calibration = JSDefaultCalibration;
	}

	/* From this point on device and calibration are not NULL, so
	 * record device and calibration file names on the jsd
	 */
	jsd->device_name = STRDUP(device);
	jsd->calibration_file = STRDUP(calibration);


#if defined(__linux__) || defined(__FreeBSD__)
	/* Open joystick */
	jsd->fd = open(jsd->device_name, O_RDONLY);
	if(jsd->fd < 0)
	{
	    JSClose(jsd);
	    return(JSNoAccess);
	}
#endif

#if defined(__linux__)
	/* Fetch device values */
	/* Raw version string */
	ioctl(jsd->fd, JSIOCGVERSION, &version);
	jsd->driver_version = (unsigned int)version;

	/* Total number of axises */
	ioctl(jsd->fd, JSIOCGAXES, &axes);	/* Total axises */
	jsd->total_axises = axes;

	/* Total number of buttons */
	ioctl(jsd->fd, JSIOCGBUTTONS, &buttons);	/* Total buttons */
	jsd->total_buttons = buttons;

	/* Device descriptive name */
	ioctl(jsd->fd, JSIOCGNAME(LINUX_JS_NAME_MAX), name);
	jsd->name = STRDUP(name);
#elif defined(__FreeBSD__)
	jsd->driver_version = version = 1;
	jsd->total_axises = axes = 2;
	jsd->total_buttons = buttons = 2;
	strlcpy(name, "FreeBSD-Gameport", LINUX_JS_NAME_MAX);
	jsd->name = STRDUP(name);
#endif
	/* Allocate axises */
	if(jsd->total_axises > 0)
	{
	    jsd->axis = (js_axis_struct **)calloc(
	        jsd->total_axises,
	        sizeof(js_axis_struct *)
	    );
	    if(jsd->axis == NULL)
	    {
	        jsd->total_axises = 0;
	        JSClose(jsd);
	        return(JSNoBuffers);
	    }
	}
	for(i = 0; i < jsd->total_axises; i++)
	{
	    jsd->axis[i] = axis = (js_axis_struct *)calloc(
		1, sizeof(js_axis_struct)
	    );
	    if(axis == NULL)
	    {
		JSClose(jsd);
		return(JSNoBuffers);
	    }

	    /* Reset axis values */
	    axis->cur = JSDefaultCenter;
	    axis->min = JSDefaultMin;
	    axis->max = JSDefaultMax;
	    axis->cen = JSDefaultCenter;
	    axis->nz = JSDefaultNullZone;
	    axis->tolorance = JSDefaultTolorance;
	    axis->flags = 0;
	}

	/* Allocate buttons */  
	if(jsd->total_buttons > 0)
	{
	    jsd->button = (js_button_struct **)calloc(
		jsd->total_buttons,
		sizeof(js_button_struct *)
	    );
	    if(jsd->button == NULL)   
	    {
		jsd->total_buttons = 0;
		JSClose(jsd);   
		return(JSNoBuffers);
	    }
	}
	for(i = 0; i < jsd->total_buttons; i++)
	{
	    jsd->button[i] = button = (js_button_struct *)calloc(
		1, sizeof(js_button_struct)
	    );
	    if(button == NULL)
	    {
		JSClose(jsd);
		return(JSNoBuffers);
	    }

	    /* Reset button values */
	    button->state = JSButtonStateOff;
	}

	/* Set to non-blocking? */
	if(flags & JSFlagNonBlocking)
	{
	    fcntl(jsd->fd, F_SETFL, O_NONBLOCK);
	    jsd->flags |= JSFlagNonBlocking;
 	}

	/* Mark successful initialization */
	jsd->flags |= JSFlagIsInit;

	/* Load calibration from calibration file */
	JSLoadCalibrationUNIX(jsd);

	/* Set axis tolorance for error correction */
	JSResetAllAxisTolorance(jsd);


	return(JSSuccess);
}


/*
 *	Called by JSUpdate() to set axis structure value.
 */
static void SetAxisValue(js_axis_struct *axis, int value, time_t t)
{
       /* Record previous axis position */
       axis->prev = axis->cur;

       /* Set new current axis value */
       axis->cur = value;

       /* Record time stamp (in ms) */
       axis->last_time = axis->time;

       /* Set new time stamp (in ms) */
       axis->time = t;
}

/*
 *	Called by JSUpdate() to set button structure value.
 */
static void SetButtonValue(js_button_struct *button, int value, time_t t)
{
       /* Record previous state */
       button->prev_state = button->state;

       /* Set new button state */
       button->state = value ? JSButtonStateOn : JSButtonStateOff;

       /* Update state change */
       if((button->prev_state == JSButtonStateOn) &&
	       (button->state == JSButtonStateOff)
       )
	       button->changed_state = JSButtonChangedStateOnToOff;
       else if((button->prev_state == JSButtonStateOff) &&
	       (button->state == JSButtonStateOn)
       )
	       button->changed_state = JSButtonChangedStateOffToOn;

       /* Record time stamp (in ms) */
       button->last_time = button->time;

       /* Set new time stamp (in ms) */
       button->time = t;
}

/*
 *	Updates the information in jsd, returns JSGotEvent if there
 *	was some change or JSNoEvent if there was no change.
 *
 *	jsd needs to be previously initialized by a call to
 *	JSInit().
 */
int JSUpdate(js_data_struct *jsd)
{
	int n;
	int status = JSNoEvent;
#if defined(__linux__)
	int keep_handling = 1;
	int bytes_read, cycles;
	struct js_event event;
#elif defined(__FreeBSD__)
	struct joystick js;
#endif
#if defined(__linux__) || defined(__FreeBSD__)
	js_axis_struct **axis;
	js_button_struct **button;
#endif  /* __linux__ || __FreeBSD__ */


	if(jsd == NULL)
	    return(status);

	if(jsd->fd < 0)
	    return(status);

#if defined(__linux__) || defined(__FreeBSD__)
	/* Reset all button state change value on all buttons */
	for(n = 0, button = jsd->button;
	    n < jsd->total_buttons;
	    n++, button++
	)
	{
	    if(*button != NULL)
		(*button)->changed_state = JSButtonChangedStateNone;
	}

	/* Reset current and previous axis values */
	for(n = 0, axis = jsd->axis;
	    n < jsd->total_axises;
	    n++, axis++
	)
	{
	    if(*axis != NULL)
		(*axis)->prev = (*axis)->cur;
	}
#endif /* __linux__ || __FreeBSD__ */


#if defined(__linux__)
	/* Linux joystick device fetching
	 *
	 * Handle up to 16 events from joystick driver
	 */
	cycles = 0;
	while(keep_handling &&
	      (cycles < 16)
	)
	{
	    /* Get event */
	    bytes_read = read(
	        jsd->fd,
	        &event,
	        sizeof(struct js_event)
	    );
	    /* No more events to be read? */
	    if(bytes_read != sizeof(struct js_event))
	        return(status);

	    /* Handle by event type */
	    switch(event.type & ~JS_EVENT_INIT)
	    {
	      /* Axis event */
	      case JS_EVENT_AXIS:
	        /* Get axis number */
		n = event.number;

		/* Does axis exist? */
		if(JSIsAxisAllocated(jsd, n))
		    SetAxisValue(
			jsd->axis[n],
			(int)event.value,
			(time_t)event.time
		    );
		jsd->events_received++;	/* Increment events recv count */
	        status = JSGotEvent;	/* Mark that we got event */
	        break;

	      /* Button event */
	      case JS_EVENT_BUTTON:
	        /* Get button number */
	        n = event.number;

	        /* Does button exist? */
		if(JSIsButtonAllocated(jsd, n))
		    SetButtonValue(
			jsd->button[n],
			(int)event.value, (time_t)event.time
		    );
		jsd->events_received++;	/* Increment events recv count */
	        status = JSGotEvent;	/* Mark that we got event */
	        break;

	      /* Other event */
	      default:
		keep_handling = 0;	/* Stop handling events */
		break;
	    }

	    cycles++;
	}
#elif defined(__FreeBSD__)
	/* FreeBSD joystick device fetching */
	if(read(jsd->fd, &js, sizeof(struct joystick)) == sizeof(struct joystick))
	{
	    status = JSGotEvent;
	    if(JSIsAxisAllocated(jsd, 0))
		SetAxisValue(jsd->axis[0], js.x, time(NULL));
	    if(JSIsAxisAllocated(jsd, 1))
		SetAxisValue(jsd->axis[1], js.y, time(NULL));
	    if(JSIsButtonAllocated(jsd, 0))
		SetButtonValue(jsd->button[0], js.b1, time(NULL));
	    if(JSIsButtonAllocated(jsd, 1))
		SetButtonValue(jsd->button[1], js.b2, time(NULL));
	}
#endif

	return(status);
}

/*
 *	Closes the joystick and deallocates all resources on the given
 *	jsd structure. The jsd structure itself is not deallocated however
 *	its values will be reset to defaults.
 */
void JSClose(js_data_struct *jsd)
{
	int i;

	if(jsd == NULL)
	    return;

	/* Delete the force feedback resources */
	JSFFDelete(jsd->force_feedback);
	jsd->force_feedback = NULL;

	/* Close the joystick */
	if(jsd->fd > -1)
	{
	    close(jsd->fd);
	    jsd->fd = -1;
	}

	free(jsd->name);
	jsd->name = NULL;

	/* Delete all axises */
	for(i = 0; i < jsd->total_axises; i++)
	    free(jsd->axis[i]);
	free(jsd->axis);
	jsd->axis = NULL;
	jsd->total_axises = 0;

	/* Delete all buttons */
	for(i = 0; i < jsd->total_buttons; i++)
	    free(jsd->button[i]);
	free(jsd->button);
	jsd->button = NULL;
	jsd->total_buttons = 0;

	/* Delete device name */
	free(jsd->device_name);
	jsd->device_name = NULL;

	/* Delete calibration file name */
	free(jsd->calibration_file);
	jsd->calibration_file = NULL;

	/* Reset rest of the values */
	jsd->flags = 0;
	jsd->driver_version = 0;
	jsd->last_calibrated = 0;

	/* Do not delete the jsd structure */
}
