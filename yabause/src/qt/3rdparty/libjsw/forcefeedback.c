#include <stdio.h>
#include <stdlib.h>

#include "../include/jsw.h"

#include "../include/forcefeedback.h"


void *JSFFNew(void);
void JSFFDelete(void *ptr);


/*
 *      Force feedback structure, this structure has not been determined
 *      yet due to pending development of the I-Force driver for Linux.
 */
typedef struct {

	/* Time stamp of latest ff signal send and second to latest ff
	 * signal send (in ms).
	 */
	time_t time, last_time;

} js_force_feedback_struct;





/*
 *	Allocates a new js_force_feedback_struct structure and initializes
 *	all resources.
 */
void *JSFFNew(void)
{
	js_force_feedback_struct *ff = (js_force_feedback_struct *)calloc(
	    1, sizeof(js_force_feedback_struct)
	);
	if(ff == NULL)
	    return(ff);


	return(ff);
}


/*
 *	Closes all force feedback resources and deallocates the given
 *	ff structure itself.
 */
void JSFFDelete(void *ptr)
{
	js_force_feedback_struct *ff = (js_force_feedback_struct *)ptr;
	if(ff == NULL)
	    return;


	/* Deallocate structure itself. */
	free(ff);
	ff = ptr = NULL;
}
