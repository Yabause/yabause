#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "../include/jsw.h"


int JSIsButtonAllocated(js_data_struct *jsd, int n);
int JSGetButtonState(js_data_struct *jsd, int n);


#define ATOI(s)         (((s) != NULL) ? atoi(s) : 0)
#define ATOL(s)         (((s) != NULL) ? atol(s) : 0)
#define ATOF(s)         (((s) != NULL) ? atof(s) : 0.0f)
#define STRDUP(s)       (((s) != NULL) ? strdup(s) : NULL)

#define MAX(a,b)        (((a) > (b)) ? (a) : (b))
#define MIN(a,b)        (((a) < (b)) ? (a) : (b))
#define CLIP(a,l,h)     (MIN(MAX((a),(l)),(h)))
#define STRLEN(s)       (((s) != NULL) ? strlen(s) : 0)
#define STRISEMPTY(s)   (((s) != NULL) ? (*(s) == '\0') : 1)


/*
 *      Checks if button n is allocated on jsd.
 */
int JSIsButtonAllocated(js_data_struct *jsd, int n)
{
	if(jsd == NULL)
	    return(0);
	else if((n < 0) || (n >= jsd->total_buttons))
	    return(0);
	else if(jsd->button[n] == NULL)
	    return(0);
	else
	    return(1);
}

/*
 *      Gets the button state of button n, one of JSButtonState*.
 */
int JSGetButtonState(js_data_struct *jsd, int n)
{
	if(JSIsButtonAllocated(jsd, n))
	    return(jsd->button[n]->state);
	else
	    return(JSButtonStateOff);
}
