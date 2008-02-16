/*
	How to list installed game controllers

	Demostrates how to obtain a list of all game controllers and their
	attributes.

	The only input required is the calibration file.

	If a joystick is already in use, then it will be marked as such in
	the listing (no failure should occure).
 */

#include <stdio.h>

#include <jsw.h>	/* libjsw */


int main(int argc, char *argv[])
{
	/* Default calibration file paths. */
	const char *calib = JSDefaultCalibration;

	/* Joystick attributes list. */
	js_attribute_struct *js_attrib;
	int total_js_attribs;


        /* No calibration file given? */
        if(argc < 2)
        {
	    /* Print usage header and use default calibration file. */
            printf(
                "Usage: listing <calibation_file>\n\n"
            );
        }
	else
	{
	    /* Fetch calibration file name from arguments. */
	    calib = argv[1];
	}

	/* Get list of joystick device attributes. */
	js_attrib = JSGetAttributesList(
	    &total_js_attribs, calib
	);
	/* Got list? */
	if(js_attrib != NULL)
	{
	    int i;
	    const js_attribute_struct *a;

	    /* Itterate through joystick attributes list and print each
	     * joystick device's attributes.
	     */
	    for(i = 0; i < total_js_attribs; i++)
	    {
		a = &js_attrib[i];

		if(a->device_name != NULL)
		    printf("Device: %s\n", a->device_name);
		if(a->name != NULL)
		    printf("Name: %s\n", a->name);
		printf(
		    "Configured: %s\n",
		    a->is_configured ? "Yes" : "No"
		);
                printf(
                    "In Use: %s\n",
                    a->is_in_use ? "Yes" : "No"
                );
                printf(
                    "Accessable: %s\n",
                    a->not_accessable ? "No" : "Yes"
                );
		printf("\n");
	    }
	}
	else
	{
	    /* Failed to obtain list. */
	    fprintf(
		stderr,
"%s: Unable to obtain joystick device attributes list.\n",
		calib
	    );
	}


	/* Deallocate joystick device attributes list. */
	JSFreeAttributesList(js_attrib, total_js_attribs);

	return(0);
}
