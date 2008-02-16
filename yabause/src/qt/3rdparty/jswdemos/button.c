/*
	Advanced button event handling with critical timming

	Demostrates advanced techniques or measuring button position
	changes relative to a fixed interval. For thie example the
	interval will be 1000 ms (1 second).

	While it is running, whenever a button event is recieved, its
	state, interval, and state change will be printed. For
	simplicitly only one button (given at input) will be checked.

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
	int status, button_num = 0;
	const char *device = JSDefaultDevice;
	const char *calib = JSDefaultCalibration;
	js_data_struct jsd;


	/* Set up signal handler to catch the interrupt signal. */
	signal(SIGINT, MySignalHandler);


	/* Need three arguments; joystick device name, calibration
	 * file and button number.
	 */
	if(argc < 4)
	{
	    printf(
		"Usage: button <joystick_device> <calibation_file> <button_num>\n"
	    );
	}
	else
	{
	    /* Fetch values from input. */
	    device = argv[1];
	    calib = argv[2];
	    button_num = atoi(argv[3]);
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
	     * need to check if the button specified by button_num
	     * exists.
	     */
	    if(!JSIsButtonAllocated(&jsd, button_num))
	    {
		fprintf(
		    stderr,
"%s: Button %i: No such button, available buttons are %i to %i.\n",
		    device,
		    button_num,
		    0, jsd.total_buttons - 1
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
		if(JSIsButtonAllocated(&jsd, button_num))
		{
		    double pos_change_coeff;
		    time_t dt, interval = 1000;
		    js_button_struct *button_ptr = jsd.button[button_num];


		    /* Handle by button changed state (not the current
		     * button state).
		     */
		    switch(button_ptr->changed_state)
		    {
		      /* Just released? */
		      case JSButtonChangedStateOnToOff:
                        printf("Button %i: Released\n", button_num);

			/* Calculate time of click. */
			dt = button_ptr->time - button_ptr->last_time;

			/* Button moved too quickly, get the coefficient
			 * of its movement effect relative to 1000
			 * milliseconds (1 second).
			 */
			pos_change_coeff = (double)dt / (double)interval;
                        printf(
			    "Button %i: Clicked Time: %ld ms  Change Coeff: %.3f\n",
			    button_num, dt, pos_change_coeff
			);
			break;

		      /* Just pressed. */
		      case JSButtonChangedStateOffToOn:
			printf("Button %i: Pressed\n", button_num);
			break;
		    }
		}
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
