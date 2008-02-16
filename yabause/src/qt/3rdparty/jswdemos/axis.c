/*
	Advanced axis event handling with critical timming

	Demostrates advanced techniques or measuring axis position
	changes relative to a fixed interval. For thie example the
	interval will be 1000 ms (1 second).

	While it is running, whenever an axis event is recieved, its
	position, interval, and delta movement will be printed. For
	simplicitly only one axis (given at input) will be checked.

	Press CTRL+C to interrupt.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#ifdef __MSW__

#else
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
	int status, axis_num = 0;
	const char *device = JSDefaultDevice;
	const char *calib = JSDefaultCalibration;
	js_data_struct jsd;

	/* Set up signal handler to catch the interrupt signal. */
	signal(SIGINT, MySignalHandler);

	/* Need three arguments; joystick device name, calibration
	 * file and axis number.
	 */
	if(argc < 4)
	{
	    printf(
		"Usage: axis <joystick_device> <calibation_file> <axis_num>\n"
	    );
	}
	else
	{
	    /* Fetch values from input. */
	    device = argv[1];
	    calib = argv[2];
	    axis_num = atoi(argv[3]);
	}

	/* Initialize the joystick. */
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
	    /* Joystick was initialized successfully, but we still
	     * need to check if the axis specified by axis_num exists.
	     */
	    if(!JSIsAxisAllocated(&jsd, axis_num))
	    {
		fprintf(
		    stderr,
"%s: Axis %i: No such axis, available axises are %i to %i.\n",
		    device,
		    axis_num,
		    0, jsd.total_axises - 1
		);
		JSClose(&jsd);
		return(1);
	    }

	    printf(
"Reading values from `%s' (press CTRL+C to interrupt).\n",
		device
	    );
	}


	runlevel = 2;
	while(runlevel >= 2)
	{
	    /* Update joystick and check for event. */
	    if(JSUpdate(&jsd) == JSGotEvent)
	    {
		printf("\r");

		if(JSIsAxisAllocated(&jsd, axis_num))
		{
		    double pos_change_coeff;
		    time_t dt, interval = 1000;
		    js_axis_struct *axis_ptr = jsd.axis[axis_num];


		    /* Calculate the time from the last event to the 
		     * current event in milliseconds.
		     */
		    dt = axis_ptr->time - axis_ptr->last_time;

		    /* Has axis moved in between cycles? */
		    if(dt < interval)
		    {
			/* Axis moved too quickly, get the coefficient
			 * of its movement effect.
			 */
			pos_change_coeff = (double)dt / (double)interval;
		    }
		    else
		    {
			/* Axis moved but it was a long time since it 
			 * last moved.
			 */
			pos_change_coeff = 1.0;
		    }


		    /* Print value of this axis, the Cur: indicates the
		     * current raw position and Prev: indicates the
		     * previous raw position. Change Coeff: is a value
		     * from 0.0 to 1.0, where 0.0 is a fast movement and
		     * 1.0 is slow or `longer than cycle' movement.
		     */
		    printf(
			"Axis %i:\tCur:%i\tPrev:%i\tChange Coeff:%.3f ",
			axis_num,
			axis_ptr->cur,
			axis_ptr->prev,
			pos_change_coeff
		    );
		}

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
