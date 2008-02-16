#ifndef CONFIG_H
#define CONFIG_H


/*
 *      Languages (define only one):
 */
#define PROG_LANGUAGE_ENGLISH   1
/* #define PROG_LANGUAGE_SPANISH        2 */
/* #define PROG_LANGUAGE_FRENCH         3 */
/* incomplete #define PROG_LANGUAGE_GERMAN      4 */
/* incomplete #define PROG_LANGUAGE_ITALIAN     5 */
/* incomplete #define PROG_LANGUAGE_NORWEGIAN   6 */
/* incomplete #define PROG_LANGUAGE_PORTUGUESE  7 */


/*
 *	Program name and version:
 */
#define PROG_NAME			"JSCalibrator"
#define PROG_NAME_FULL			"Joystick Calibrator"
#define PROG_VERSION			"1.5.7"

#define PROG_VERSION_MAJOR		1
#define PROG_VERSION_MINOR		5
#define PROG_VERSION_RELEASE		7


/*
 *	Program usage message:
 */
#define PROG_USAGE_MESG	"\
Usage: jscalibrator [options] [GUI_options]\n\
\n\
    [options] can be any of the following:\n\
\n\
        -d <device>            Specifies the initial joystick device to\n\
                               open at startup (default /dev/js0).\n\
        -f <file>              Specifies the joystick calibration file\n\
                               to open and save to (default ~/.joystick).\n\
        --help                 Prints (this) help screen and exits.\n\
        --version              Prints version information and exits.\n\
\n\
    [GUI_options] can be any options standard to your GUI, consult\n\
    your GUI's manual for available options.\n\
\n"

/*
 *	Program URL:
 */
#define PROG_URL	"http://www.battlefieldlinux.com/wolfpack/libjsw"

/*
 *	Copyright:
 */
#define PROG_COPYRIGHT  "\
Copyright (C) 1997-2006 WolfPack Entertainment.\n\
This program is protected by international copyright laws and treaties,\n\
distribution and/or modification of this software in violation of the\n\
GNU Public License is strictly prohibited. Violators will be prosecuted\n\
to the fullest extent of the law."


/*
 *	Environment Variable Names:
 */
#define ENV_VAR_NAME_HOME		"HOME"
#define ENV_VAR_NAME_HELPBROWSER	"HELPBROWSER"
#define ENV_VAR_NAME_BROWSER		"BROWSER"
#define ENV_VAR_NAME_TMPDIR		"TMPDIR"

/*
 *	Global Data Directory:
 */
#define JC_DATA_DIR     "/usr/share/libjsw"

/*
 *	Default size of the JSCalibrator window.
 */
#define JC_DEF_WIDTH			600
#define JC_DEF_HEIGHT			350

/*
 *	Default size of joystick button:
 */
#define JC_DEF_JS_BUTTON_WIDTH		80
#define JC_DEF_JS_BUTTON_HEIGHT		25

/*
 *	Default size of calibrate toggle button:
 */
#define JC_DEF_CALIBRATE_TOGGLE_WIDTH	80
#define JC_DEF_CALIBRATE_TOGGLE_HEIGHT	25


#endif	/* CONFIG_H */
