/*
	                Joystick Wrapper Library

		     http://wolfpack.twu.net/libjsw


	Copyright (C) 1997-2006 WolfPack Entertainment.

	For documentation and examples, see:
	http://wolfpack.twu.net/libjsw

 */

#ifndef JSW_H
#define JSW_H

#include <time.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif


/*
 *	Version:
 */
#define JSWVersion			"1.5.7"
#define JSWVersionMajor			1
#define JSWVersionMinor			5
#define JSWVersionRelease		7


/*
 *	Default Device & Calibration File:
 */
#if defined(__linux__)
# include <linux/joystick.h>
# define JSDefaultDevice		"/dev/js0"
# define JSDefaultCalibration		".joystick"
#elif defined(__FreeBSD__)
# define JSDefaultDevice		"/dev/joy0"
# define JSDefaultCalibration		".joystick"
#else
# define JSDefaultDevice		"/dev/js0"
# define JSDefaultCalibration		".joystick"
#endif


/*
 *	Default Ranges (in raw units):
 *
 *	These are used only for initial calibration values and should
 *	never be used as a substitute for calibrated values. Always
 *	use calibrated values when possible.
 */
#define JSDefaultMin			-32767
#define JSDefaultMax			32767
#define JSDefaultCenter			0
#define JSDefaultNullZone		1024
#define JSDefaultTolorance		10


/*
 *	Joystick Device Flags:
 */
#define JSFlagIsInit			(1 << 1)
#define JSFlagNonBlocking		(1 << 2)	/* Open in non-blocking mode */
#define JSFlagForceFeedback		(1 << 3)	/* Open in read/write mode */

/*
 *	Axis Flags:
 */
#define JSAxisFlagFlipped		(1 << 1)	/* Flip values */
#define JSAxisFlagIsHat			(1 << 2)	/* Is a hat */
#define JSAxisFlagTolorance		(1 << 3)	/* Use error connection */


/*
 *	Error Codes:
 */
#define JSSuccess			0
#define JSError				1
#define JSBadValue			2
#define JSNoAccess			3
#define JSNoBuffers			4


/*
 *	Event Codes:
 */
#define JSNoEvent			0
#define JSGotEvent			1

/*
 *	Button States:
 */
#define JSButtonStateOff		0
#define JSButtonStateOn			1

/*
 *	Button Change Codes:
 */
#define JSButtonChangedStateNone	0	/* No change */
#define JSButtonChangedStateOffToOn	1	/* Up to down position */
#define JSButtonChangedStateOnToOff	2	/* Down to up position */


/*
 *	Joystick Axis:
 */
typedef struct {

	/* Public */

	/* Axis ranges and bounds (all raw units) */
	int		cur, prev;	/* Current and previous position */
	int		min, cen, max;	/* Bounds */
	int		nz;		/* Null zone, in raw units from cen */
	int		tolorance;	/* Precision snap in raw units (used only
					 * if JSAxisFlagTolorance is set) */

	/* Flags, any of JSAxisFlag* */
	unsigned int	flags;

	/* Time stamp of current (newest) event and last event (in ms) */
	time_t		time,
			last_time;

	/* Correction level information (new since 1.5.0) */
	int		correction_level;	/* 0 to 2 supported, higher
						 * values are allowed */

	int		dz_min,		/* Dead zone bounds in raw units */
			dz_max;

	double		corr_coeff_min1,	/* 1st degree correctional coeff */
			corr_coeff_max1;
	double		corr_coeff_min2,	/* 2nd degree correctional coeff */
			corr_coeff_max2;

} js_axis_struct;
#define JS_AXIS(p)		((js_axis_struct *)(p))

/*
 *	Joystick Button:
 */
typedef struct {

	/* Current and previous button states, one of JSButtonState* */
	int		state,
			prev_state;

	/* Changed state from previous call to JSUpdate(), one of
	 * JSButtonChangedState*
	 */
	int		changed_state;

	/* Time stamp of current (newest) event and last event (in ms) */
	time_t		time,
			last_time;

} js_button_struct;
#define JS_BUTTON(p)		((js_button_struct *)(p))

/*
 *	Opened Joystick Calibration & Resources:
 */
typedef struct {

	/* Public (Read-Only) */
	char		*name;		/* Descriptive name */

	js_axis_struct **axis;		/* Axises */
	int		total_axises;

	js_button_struct **button;	/* Buttons */
	int		total_buttons;

	char		*device_name;	/* Device name */
	char		*calibration_file;	/* Associated calibration file */

	unsigned int	events_received,	/* Event counters */
			events_sent;

	/* Private */
	int		fd;		/* Descriptor to joystick
					 * (can be -1 for none) */
	unsigned int	flags;		/* Any of JSFlag* */
	unsigned int	driver_version;	/* Raw joystick driver version code */
	time_t		last_calibrated;/* Last time of calibration in systime
					 * seconds, can be 0 for never
					 * calibrated */
	void		*force_feedback;/* Reserved, always NULL for now */

} js_data_struct;
#define JS_DARA(p)		((js_data_struct *)(p))

/*
 *	Joystick Device Attributes:
 */
typedef struct {

	char		*name;		/* Descriptive name */
	char		*device_name;	/* Device name */

	/* Specifies that this joystick device is configured and
	 * calibrated properly if true, otherwise opening this
	 * joystick may produce unreliable values */
	int		is_configured;

	/* If true then suggests device may already be opened */
	int		is_in_use;

	/* Device may not disconnected at the moment, turned off, or
	 * driver not loaded for it but it may be configured
	 * (see is_configured for that information) */
	int		not_accessable;

} js_attribute_struct;
#define JS_ATTRIBUTE(p)		((js_attribute_struct *)(p))


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
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" int JSLoadCalibrationUNIX(js_data_struct *jsd);
#else
extern int JSLoadCalibrationUNIX(js_data_struct *jsd);
#endif

/*
 *      Gets a list of calibrated devices found in the specified
 *      calibration file.
 *
 *      The returned list of strings and the pointer array must be
 *      free()'ed by the calling function.
 */
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" char **JSLoadDeviceNamesUNIX(
	int *total, const char *calibration
);
#else
extern char **JSLoadDeviceNamesUNIX(
	int *total, const char *calibration
);
#endif


/*
 *	Checks if the joystick is initialized.
 */
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" int JSIsInit(js_data_struct *jsd);
#else
extern int JSIsInit(js_data_struct *jsd);
#endif


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
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" js_attribute_struct *JSGetAttributesList(
	int *total, const char *calibration
);
#else
extern js_attribute_struct *JSGetAttributesList(
	int *total, const char *calibration
);
#endif

/*
 *	Deletes the Joystick Attributes list.
 */
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" void JSFreeAttributesList(js_attribute_struct *list, int total);
#else
extern void JSFreeAttributesList(js_attribute_struct *list, int total);
#endif


/*
 *	Returns the raw unparsed version code of the joystick driver
 *	(not the version of the libjsw library).
 */
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" unsigned int JSDriverVersion(js_data_struct *jsd);
#else
extern unsigned int JSDriverVersion(js_data_struct *jsd);
#endif

/*
 *      Returns the parsed version code of the joystick driver 
 *      (not the version of the libjsw library).
 */
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" int JSDriverQueryVersion(
	js_data_struct *jsd,
	int *major_rtn, int *minor_rtn, int *release_rtn
);
#else
extern int JSDriverQueryVersion(
	js_data_struct *jsd,
	int *major_rtn, int *minor_rtn, int *release_rtn
);
#endif


/*
 *	Checks if the joystick axis is valid and allocated.
 */
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" int JSIsAxisAllocated(js_data_struct *jsd, int n);
#else
extern int JSIsAxisAllocated(js_data_struct *jsd, int n);
#endif

/*
 *	Checks if the joystick button is valid and allocated.
 */
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" int JSIsButtonAllocated(js_data_struct *jsd, int n);
#else
extern int JSIsButtonAllocated(js_data_struct *jsd, int n);
#endif


/*
 *	Gets the coefficient value of axis n, does not take null
 *	zone into account.
 */
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" double JSGetAxisCoeff(js_data_struct *jsd, int n);
#else
extern double JSGetAxisCoeff(js_data_struct *jsd, int n);
#endif

/*
 *	Same as JSGetAxisCoeff, except takes null zone into account.
 */
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" double JSGetAxisCoeffNZ(js_data_struct *jsd, int n);
#else
extern double JSGetAxisCoeffNZ(js_data_struct *jsd, int n);
#endif

/*
 *	Gets the button state of button n, one of JSButtonState*.
 */
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" int JSGetButtonState(js_data_struct *jsd, int n);
#else
extern int JSGetButtonState(js_data_struct *jsd, int n);
#endif

/*
 *      Applies the tolorance value defined on each axis on the given
 *      jsd to the low-level joystick driver's tolorance.
 *
 *      This function is automatically called by JSInit().
 */
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" void JSResetAllAxisTolorance(js_data_struct *jsd);
#else
extern void JSResetAllAxisTolorance(js_data_struct *jsd);
#endif


/*
 *	Initializes the joystick and stores the new initialized values
 *	into the jsd structure.
 *
 *	If the device is not specified (set to NULL), then it will
 *	be defauled to JSDefaultDevice.
 *
 *	If the calibration file is not specified (set to NULL), then
 *	it will be defaulted to JSDefaultCalibration. The HOME
 *	enviroment value will be used as the prefix to the path of
 *	JSDefaultCalibration. The calibration file does not have to
 *	exist.
 *
 *	Available inputs for flags are any of the or'ed following:
 *
 *	JSFlagNonBlocking		Open in non-blocking mode.
 *	JSFlagForceFeedback		Open in read/write mode.
 */
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" int JSInit(
	js_data_struct *jsd,
	const char *device,
	const char *calibration,
	unsigned int flags
);
#else
extern int JSInit(
	js_data_struct *jsd,
	const char *device,
	const char *calibration,
	unsigned int flags
);
#endif

/*
 *	Fetches the next event and updates joystick values specified in
 *	the jsd.  Can return JSNoEvent or JSGotEvent depending on if
 *	an event was recieved.
 */
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" int JSUpdate(js_data_struct *jsd);
#else
extern int JSUpdate(js_data_struct *jsd);
#endif

/*
 *      Closes the joystick and deallocates all resources on the given
 *      jsd structure. The jsd structure itself is not deallocated however
 *      its values will be reset to defaults.
 */
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" void JSClose(js_data_struct *jsd);
#else
extern void JSClose(js_data_struct *jsd);
#endif


#ifdef __cplusplus
}
#endif

#endif	/* JSW_H */
