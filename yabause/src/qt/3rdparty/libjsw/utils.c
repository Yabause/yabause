#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include "../include/jsw.h"


/* Public functions */
int JSIsInit(js_data_struct *jsd);
unsigned int JSDriverVersion(js_data_struct *jsd);
int JSDriverQueryVersion(
	js_data_struct *jsd,
	int *major_rtn, int *minor_rtn, int *release_rtn
);


#define MAX(a,b)        (((a) > (b)) ? (a) : (b))
#define MIN(a,b)        (((a) < (b)) ? (a) : (b))
#define CLIP(a,l,h)     (MIN(MAX((a),(l)),(h)))


/*
 *	Checks if the joystick is initialized.
 */
int JSIsInit(js_data_struct *jsd)
{
	if(jsd == NULL)
	    return(0);
	else if((jsd->flags & JSFlagIsInit) &&
		(jsd->fd > -1)
	)
	    return(1);
	else
	    return(0);
}

/*
 *      Returns the driver version value from the driver unparsed.
 *
 *	If you want the parsed one use JSDriverQueryVersion().
 *
 *	This requires that the calling function knows how to handle
 *	the returned version value.
 */
unsigned int JSDriverVersion(js_data_struct *jsd)
{
	if(jsd == NULL)
	    return(0);
	else
	    return(jsd->driver_version);
}

/*
 *	Returns the joystick driver version formatted, if any
 *	inputs are NULL then that value will not be set.
 *
 *	This function has some disadvantages compared to JSDriverVersion()
 *	as this function may leave out some additional pieces of
 *	information, if available (ie vendor code, if the implmentation
 *	supports it).
 *
 *	Returns 0 on failure and 1 on success.
 */
int JSDriverQueryVersion(
	js_data_struct *jsd, 
	int *major_rtn, int *minor_rtn, int *release_rtn 
)
{
	unsigned int ver_unparsed;

	/* Reset returns */
	if(major_rtn != NULL)
	    (*major_rtn) = 0;
	if(minor_rtn != NULL)
	    (*minor_rtn) = 0;
	if(release_rtn != NULL)
	    (*release_rtn) = 0;

	if(jsd == NULL)
	    return(0);

#ifdef __linux__
	ver_unparsed = JSDriverVersion(jsd);

	if(major_rtn != NULL)
	    (*major_rtn) = (int)((ver_unparsed >> 16) & 0xff);
	if(minor_rtn != NULL)
	    (*minor_rtn) = (int)((ver_unparsed >> 8) & 0xff);
	if(release_rtn != NULL)
	    (*release_rtn) = (int)(ver_unparsed & 0xff); 

	return(1);	/* Success */
#else
	/* Platform unsupported, return failure */
	return(0);
#endif
}

