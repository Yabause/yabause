#include <gtk/gtk.h>
#include <unistd.h>
#include "jc.h"


static void JCCalibrateRepresentative(jc_struct *jc);
static void JCCalibrateLogical(jc_struct *jc);
void JCDoCalibrate(jc_struct *jc);
void JCDoSetAxisTolorance(jc_struct *jc, gint axis_num);


#define ATOI(s)         (((s) != NULL) ? atoi(s) : 0)
#define ATOL(s)         (((s) != NULL) ? atol(s) : 0)
#define ATOF(s)         (((s) != NULL) ? atof(s) : 0.0f)
#define STRDUP(s)       (((s) != NULL) ? g_strdup(s) : NULL)

#define MAX(a,b)        (((a) > (b)) ? (a) : (b))
#define MIN(a,b)        (((a) < (b)) ? (a) : (b))
#define CLIP(a,l,h)     (MIN(MAX((a),(l)),(h)))
#define STRLEN(s)       (((s) != NULL) ? strlen(s) : 0)
#define STRISEMPTY(s)   (((s) != NULL) ? (*(s) == '\0') : TRUE)


/*
 *	Calibrate axises in representative layout, this is called once
 *	per cycle to check the current axis positions and measure their
 *	bounds.
 */
static void JCCalibrateRepresentative(jc_struct *jc)
{
	gint i;
	js_axis_struct *axis_ptr;
	js_data_struct *jsd;
	GtkWidget *w;
	layout_rep_struct *lr = &jc->layout_rep;
	if(lr == NULL)
	    return;

	/* Get calibration toggle widget */
	w = lr->calib_toggle;
	if(w == NULL)
	    return;

	/* Is toggle widget toggled? */
	if(!(GTK_TOGGLE_BUTTON(w)->active))
	    return;

	/* Get joystick data structure */
	jsd = &jc->jsd;

	/* Iterate through each axis */
	for(i = 0; i < jsd->total_axises; i++)
	{
	    axis_ptr = jsd->axis[i];
	    if(axis_ptr == NULL)
		continue;

	    /* Update axis bounds */
	    if(axis_ptr->min > axis_ptr->cur)
		axis_ptr->min = axis_ptr->cur;

	    if(axis_ptr->max < axis_ptr->cur)
		axis_ptr->max = axis_ptr->cur;
	}
}

/*
 *	Calibrate axises in logical layout, this is called once
 *      per cycle to check the current axis positions and measure their
 *      bounds.
 */
static void JCCalibrateLogical(jc_struct *jc)
{
	gint i;
	js_axis_struct *axis_ptr;
	js_data_struct *jsd;
	GtkWidget *w;
	layout_logical_struct *ll = &jc->layout_logical;
	layout_logical_axis_struct *ll_axis_ptr;
	if(ll == NULL)
	    return;

	jsd = &jc->jsd;

	/* Iterate through each axis */
	for(i = 0; i < ll->total_axises; i++)
	{
	    ll_axis_ptr = ll->axis[i];
	    if(ll_axis_ptr == NULL)
		continue;

	    /* Skip if not initialized */
	    if(!ll_axis_ptr->initialized)
		continue;

	    /* Get calibrate toggle widget */
	    w = ll_axis_ptr->calib_toggle;
	    if(w == NULL)
		continue;

	    /* Calibrate toggle is toggled? */
	    if(!GTK_TOGGLE_BUTTON(w)->active)
		continue;

	    /* Get pointer to axis on the jsd structure */
	    if(JSIsAxisAllocated(jsd, i))
		axis_ptr = jsd->axis[i];
	    else
		continue;

	    /* Update axis bounds */
	    if(axis_ptr->min > axis_ptr->cur)
		axis_ptr->min = axis_ptr->cur;

	    if(axis_ptr->max < axis_ptr->cur)
		axis_ptr->max = axis_ptr->cur;
	}
}


/*
 *	Calibrate axis values, this function should be called once per
 *	cycle. It will check if any axises need to be calibrated and
 *	if so update the respective axis' bounds.
 */
void JCDoCalibrate(jc_struct *jc)
{
	if(jc == NULL)
	    return;

	switch(jc->layout_state)
	{
	  case JC_LAYOUT_REPRESENTATIVE:
	    JCCalibrateRepresentative(jc);
	    break;

	  case JC_LAYOUT_LOGICAL:
	    JCCalibrateLogical(jc);
	    break;
	}
}


/*
 *	Procedure to set the tolorance for the axis specified by 
 *	axis_num.
 */
void JCDoSetAxisTolorance(jc_struct *jc, gint axis_num)
{
	gint i, status;
	gint total_loops = 15;	/* 1.5 seconds */
	js_data_struct *jsd;
	js_axis_struct *axis_ptr;
	gint x, e1, e2;


	if(jc == NULL)
	    return;

	if(jc->processing)
	    return;

	/* Get joystick data structure and make sure it is initialized */
	jsd = &jc->jsd;
	if(!JSIsInit(jsd))
	    return;

	if(JSIsAxisAllocated(jsd, axis_num))
	    axis_ptr = jsd->axis[axis_num];
	else
	    return;


	/* Mark as processing */
	JCSetBusy(jc);
	jc->processing = TRUE;


	/* Begin calculating error for tolorance */
	for(i = 0, x = axis_ptr->cur, e1 = 0;
	    i < total_loops;
	    i++
	)
	{
	    usleep(100000);

	    /* Get new axis value positions */
	    status = JSUpdate(jsd);
	    if(status == JSNoEvent)
		continue;

	    /* Calculate differance between last and current axis value */
	    e2 = x - axis_ptr->cur;
	    if(e2 < 0)
		e2 *= -1;	/* Must be absolute value */

	    /* New error greater than old? */
	    if(e2 > e1)
		e1 = e2;

	    /* Record last axis value */
	    x = axis_ptr->cur;
	}

	/* Set new tolorance based on the calculated error */
	axis_ptr->tolorance = e1;


	/* Mark as done processing */
	jc->processing = FALSE;
	JCSetReady(jc);
}
