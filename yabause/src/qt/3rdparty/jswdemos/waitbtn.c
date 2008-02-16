/*
 *	Monitors the specified joystick for a button press and returns
 *	the button number or 0 on error.
 *
 *	Wed Jan 12 15:06:25 PST 2005
 *
 *	"Tara Milana" <learfox@twu.net>
 */
#include <stdio.h>
#include <jsw.h>   
#include <unistd.h>

int main(int argc, char *argv[])
{
	int status, pressed_button = 0;
	char *device = JSDefaultDevice;
	char *calib = JSDefaultCalibration;
	js_data_struct jsd;

	if(argc < 2)
	{
	    printf(
"Usage: waitbtn <joystick_device> [calibration_file]\n"
	    );
	    return(0);
	}

	if(argc >= 1)
	    device = argv[1];
	if(argc >= 2)
	    calib = argv[2];

	/* Initialize joystick */
	status = JSInit(
	    &jsd,
	    device,
	    calib,
	    JSFlagNonBlocking
	);

	if(status != JSSuccess)
	{
	    fprintf(
		stderr,
"Unable to open joystick \"%s\", error code %i.\n",
		device, status
	    );
	    JSClose(&jsd);
	    return(1);
	}

	printf(
"Waiting for button press on joystick \"%s\"...\n",
	    device
	);

	while(1)
	{
	    /* Get joystick event */
	    if(JSUpdate(&jsd) == JSGotEvent)
	    {
		/* Got event, now check the state of each button */
		int i;
		for(i = 0; i < jsd.total_buttons; i++)
		{
		    if(JSGetButtonState(&jsd, i) == JSButtonStateOn)
		    {
			/* This button is pressed, now convert from
			 * button index to button number by adding 1
			 */
			pressed_button = i + 1;
			break;
		    }
		}

		/* Was a button pressed? */
		if(pressed_button > 0)
		    break;
	    }

	    usleep(8000);
	}

	/* Close the joystick device */
	JSClose(&jsd);

	/* Print the button that was pressed */
	printf("Pressed button %i\n", pressed_button);

	return(pressed_button);
}
