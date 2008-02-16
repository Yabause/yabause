/*
	Basic libjsw API procedures and usage

	Demostrates the fundimentals of the libjsw API; it opens the
	joystick device by calling JSInit(), manages events by calling
	JSUpdate(), and finally closes the joystick device by calling
	JSClose().

	While it is running, whenever a joystick device event is recieved
	(both axis and button events) will be printed to stdout.

	Press CTRL+C to interrupt.
 */

#include <stdio.h>
#ifdef __MSW__

# else
# include <unistd.h>
#endif
#include <signal.h>

#include <jsw.h>	/* libjsw */


/*
 *	Global runlevel code for this program, used for maintaining the
 *	main while() loop.
 */
static int runlevel;


/* Function prototypes. */
static void MySignalHandler(int s);


/*
 *	Signal handler, to catch the interrupt signal.
 */
static void MySignalHandler(int s)
{
	switch(s)
	{
	  case SIGINT:
	    /* Set runlevel to 1, causing the main while() loop to break. */
	    runlevel = 1;
	    break;
	}
}


int main(int argc, char *argv[])
{
	int i, status;

	/* Default device and calibration file paths. */
	const char *device = JSDefaultDevice;
	const char *calib = JSDefaultCalibration;

	/* Joystick data structure. */
	js_data_struct jsd;


	/* Set up signal handler to catch the interrupt signal. */
	signal(SIGINT, MySignalHandler);


	/* Parse arguments, first argument is the joystick device name
	 * and the second is the calibration file name. Did we get
	 * enough arguments?
	 */
	if(argc < 3)
	{
	    /* Use default joystick device and calibration file
	     * but also print usage.
	     */
	    printf(
		"Usage: basic <joystick_device> <calibation_file>\n"
	    );
	}
	else
	{
	    /* Fetch device and calibration file names from input. */
	    device = argv[1];
	    calib = argv[2];
	}


	/* Initialize (open) the joystick, passing it the pointer to
	 * the jsd structure (it is safe to pass an uninitialized jsd
	 * structure as seen here).
	 */
	status = JSInit(
	    &jsd,
	    device,
	    calib,
	    JSFlagNonBlocking
	);
	/* Error occured opening joystick? */
	if(status != JSSuccess)
	{
	    /* There was an error opening the joystick, print by the
	     * recieved error code from JSInit().
	     */
	    switch(status)
	    {
	      case JSBadValue:
		fprintf(
		    stderr,
"%s: Invalid value encountered while initializing joystick.\n",
		    device
		);
		break;

	      case JSNoAccess:
		fprintf(
		    stderr,
"%s: Cannot access resources, check permissions and file locations.\n",
                    device
                );
		break;

	      case JSNoBuffers:
                fprintf(
		    stderr,
"%s: Insufficient memory to initialize.\n",
                    device
                );
		break;

	      default:	/* JSError */
                fprintf(
		    stderr,
"%s: Error occured while initializing.\n",
                    device
                );
		break;
	    }

	    /* Print some helpful advice. */
	    fprintf(
		stderr,
"Make sure that:\n\
    1. Your joystick is connected (and turned on as needed).\n\
    2. Your joystick modules are loaded (`modprobe <driver>').\n\
    3. Your joystick needs to be calibrated (use `jscalibrator').\n\
\n"
            );

	    /* Close joystick jsd structure (just in case). */
	    JSClose(&jsd);

	    return(1);
	}
	else
	{
	    /* Opened joystick successfully. */
	    printf(
"Reading values from `%s' (press CTRL+C to interrupt).\n",
		device
	    );
	}


	/* Set runlevel to 2 and begin the main while() loop. Here we
	 * will print the values of each axis and button as they
	 * change.
	 */
	runlevel = 2;
	while(runlevel >= 2)
	{
	    /* Fetch for event, JSUpdate() will return JSGotEvent
	     * if we got an event.
	     */
	    if(JSUpdate(&jsd) == JSGotEvent)
	    {
		/* Got an event, now print the values to reflect the
		 * change in axises and/or buttons.
		 */
		printf("\r");

		/* Print value of each axis. */
		for(i = 0; i < jsd.total_axises; i++)
		    printf(
			"A%i:%.2f ",
			i,
			JSGetAxisCoeffNZ(&jsd, i)
		    );

		printf(" ");

		/* Print state of each button. */
		for(i = 0; i < jsd.total_buttons; i++)
                    printf(
                        "B%i:%i ",
			i,
                        JSGetButtonState(&jsd, i)
                    );

		/* That's it, besure to flush output stream. */
		fflush(stdout);
	    }
#ifdef __MSW__

#else
	    usleep(16000);	/* Don't hog the cpu. */
#endif
	}


	/* Close the joystick, now jsd may no longer be used after
	 * this call to any other JS*() calls except JSInit() which
	 * would initialize it again.
	 */
        printf("\nClosing joystick...\n");
	JSClose(&jsd);

	printf("Done.\n");

	return(0);
}
