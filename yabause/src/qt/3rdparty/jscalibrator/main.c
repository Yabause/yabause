#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#include "../include/string.h"

#include "guiutils.h"
#include "cdialog.h"
#include "fprompt.h"
#include "fb.h"
#include "pdialog.h"

#include "jc.h"
#include "config.h"


gint JCInit(gint argc, gchar **argv);
void JCShutdown(void);


jc_cursor_struct jc_cursor;
jc_struct *jc_core;


/*
 *	Procedure to initialize all resources.
 */
gint JCInit(gint argc, gchar **argv)
{
	gint i;
	const gchar *arg;
	jc_struct *jc;


	/* Reset globals */
	memset(&jc_cursor, 0x00, sizeof(jc_cursor_struct));

	jc_core = NULL;


	/* Set time zone */
	tzset();

	/* Set signal handlers */
	signal(SIGINT, JCSignalHandler);
	signal(SIGTERM, JCSignalHandler);
	signal(SIGSEGV, JCSignalHandler);


	/* Parse arguments */
	for(i = 0; i < argc; i++)
	{
	    arg = argv[i];
	    if(arg == NULL)
		continue;

	    /* Help */
	    if(!g_strcasecmp(arg, "--help") ||
	       !g_strcasecmp(arg, "-help") ||
	       !g_strcasecmp(arg, "--h") ||
	       !g_strcasecmp(arg, "-h")
	    )
	    {
		g_print(PROG_USAGE_MESG);
		return(-4);
	    }
	    /* Version */
	    else if(!g_strcasecmp(arg, "--version") ||
		    !g_strcasecmp(arg, "-version")
	    )
	    {
		g_print(
		    "%s Version %s\n%s\n",
		    PROG_NAME_FULL, PROG_VERSION,
		    PROG_COPYRIGHT
		);
		return(-4);
	    }
	}

	/* Initialize GTK */
	gtk_init(&argc, &argv);

	/* Load cursors */
	jc_cursor.processing = gdk_cursor_new(GDK_WATCH);

	/* Initialize Dialogs */
	CDialogInit();
	FPromptInit();
	FileBrowserInit();
	PDialogInit();

	/* Perform version check */
	if((PROG_VERSION_MAJOR != JSWVersionMajor) ||
	   (PROG_VERSION_MINOR != JSWVersionMinor) ||
	   (PROG_VERSION_RELEASE != JSWVersionRelease)
	)
	{
	    CDialogSetTransientFor(NULL);
	    CDialogGetResponse(
"Initialization Warning",
"Version of this program differs from libjsw.",
"The version of this program and the version of libjsw\n\
are different, this may result in runtime errors. You\n\
should reinstall the latest version of libjsw and\n\
this program.",
		CDIALOG_ICON_WARNING,
		CDIALOG_BTNFLAG_OK | CDIALOG_BTNFLAG_HELP,
		CDIALOG_BTNFLAG_OK
	    );
	    CDialogSetTransientFor(NULL);
	}

	/* Create a new joystick calibrator window and map it */
	jc_core = jc = JCNew(argc, argv);
	if(jc == NULL)
	    return(-1);
	else
	    JCMap(jc);

	return(0);
}

/*
 *	Deletes all resources.
 */
void JCShutdown(void)
{
	/* Destroy everything we allocated */

	/* Joystick caliberator window */
	JCDelete(jc_core);
	jc_core = NULL;

	/* Shutdown dialog resources */
	PDialogShutdown();
	FileBrowserShutdown();
	FPromptShutdown();
	CDialogShutdown();

	/* Cursors */
	GDK_CURSOR_DESTROY(jc_cursor.processing)
	jc_cursor.processing = NULL;
}


int main(int argc, char **argv)
{
	gint status;

	status = JCInit(argc, argv);
	switch(status)
	{
	  case 0:
	    break;

	  case -4:
	    /* No shutdown of resources */
	    return(0);
	    break;

	  default:
	    JCShutdown();
	    return(-1);
	    break;
	}

	gtk_main();

	return(0);
}
