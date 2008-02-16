#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "../include/jsw.h"


int JSIsAxisAllocated(js_data_struct *jsd, int n);
double JSGetAxisCoeff(js_data_struct *jsd, int n);
double JSGetAxisCoeffNZ(js_data_struct *jsd, int n);
void JSResetAllAxisTolorance(js_data_struct *jsd);


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
 *	Checks if axis n is allocated on jsd.
 */
int JSIsAxisAllocated(js_data_struct *jsd, int n)
{
	if(jsd == NULL)
	    return(0);
	else if((n < 0) || (n >= jsd->total_axises))
	    return(0);
	else if(jsd->axis[n] == NULL)
	    return(0);
	else
	    return(1);
}

/*
 *	Returns coefficient value from -1 to 1 for current
 *	axis position.
 */
double JSGetAxisCoeff(js_data_struct *jsd, int n)
{
	js_axis_struct *axis;

	if(JSIsAxisAllocated(jsd, n))
	    axis = jsd->axis[n];
	else
	    return(0.0);

	/* Calculate axis position coeffcient by its correction level */
	switch(axis->correction_level)
	{
	  case 2:
	    /* Fall through to correction level 1 */
	  case 1:	/* Minimal correction (level 1) */
	    if(1)
	    {
		int r, r_dz;			/* Range and range w/ dead zone applied */
		int x = axis->cur;              /* Current raw axis position */
		int dx = x - axis->cen;         /* Raw delta from center */

		/* Negative from center? */
		if(dx < 0)
		{
		    /* Negative from center */
		    double dz_bounds_coeff = CLIP(axis->corr_coeff_min1, 0.0, 1.0);
		    int dz_dmin = axis->dz_min - axis->cen;	/* Dead zone relative
								 * to center */
		    r = axis->min - axis->cen;			/* Negative range */
		    r_dz = r - dz_dmin;

		    /* Inside the dead zone? */
		    if(dx >= dz_dmin)
		    {
			/* Inside of dead zone */
			double dz_coeff = ((dz_dmin < 0) ?
			    (double)dx / (double)dz_dmin : 0.0
			);
			/* Note that dz_coeff is relative to the dead zone bound, not
			 * the range of the axis.
			 */
			return(
			    dz_coeff * dz_bounds_coeff *
			    ((axis->flags & JSAxisFlagFlipped) ? 1.0 : -1.0)
			);
		    }
		    else
		    {
			/* Outside of dead zone */
			if(r_dz < 0)
			    return(
				(((double)(dx - dz_dmin) / (double)r_dz *
				(1.0 - dz_bounds_coeff)) + dz_bounds_coeff) *
				((axis->flags & JSAxisFlagFlipped) ? 1.0 : -1.0)
			    );
			else
			    return(0.0);
		    }
		}
		else
		{
		    /* Positive from center */
		    double dz_bounds_coeff = CLIP(axis->corr_coeff_max1, 0.0, 1.0);
		    int dz_dmax = axis->dz_max - axis->cen;	/* Dead zone relative
								 * to center */
		    r = axis->max - axis->cen;			/* Positive range */
		    r_dz = r - dz_dmax;

		    /* Inside the dead zone? */
		    if(dx <= dz_dmax)
		    {
			/* Inside of dead zone */
			double dz_coeff = ((dz_dmax > 0) ?
			    (double)dx / (double)dz_dmax : 0.0
			);
			/* Note that dz_coeff is relative to the dead
			 * zone bound, not the range of the axis
			 */
			return(
			    dz_coeff * dz_bounds_coeff *
			    ((axis->flags & JSAxisFlagFlipped) ? -1.0 : 1.0)
			);
		    }
		    else
		    {
			/* Outside of dead zone */
			if(r_dz > 0)
			    return(
				(((double)(dx - dz_dmax) / (double)r_dz *
				(1.0 - dz_bounds_coeff)) + dz_bounds_coeff) *
				((axis->flags & JSAxisFlagFlipped) ? -1.0 : 1.0)
			    );
			else
			    return(0.0);
		    }
		}
	    }
	    break;

	  default:      /* No correction (level 0) */
	    if(1)
	    {
		int r;
		int x = axis->cur;              /* Current raw axis position */
		int dx = x - axis->cen;         /* Raw delta from center */

		/* Negative from center? */
		if(dx < 0)
		{
		    /* Negative from center, calculate negative range */
		    r = axis->min - axis->cen;
		    if(r < 0)
			return(
			    (double)dx / (double)r *
			    ((axis->flags & JSAxisFlagFlipped) ? 1.0 : -1.0)
			);
		    else
			return(0.0);
		}
		else
		{
		    /* Positive from center, calculate positive range */
		    r = axis->max - axis->cen;
		    if(r > 0)
			return(
			    (double)dx / (double)r *
			    ((axis->flags & JSAxisFlagFlipped) ? -1.0 : 1.0)
			);
		    else
			return(0.0);
		}
	    }
	    break;
	}
}

/*
 *      Same as JSGetAxisCoefficient() except that it takes
 *      the nullzone into account.
 *
 *	Returns 0.0 if the position is in the nullzone.
 */
double JSGetAxisCoeffNZ(js_data_struct *jsd, int n)
{
	js_axis_struct *axis;

	if(JSIsAxisAllocated(jsd, n))
	    axis = jsd->axis[n];
	else
	    return(0.0);

	/* Calculate axis position coeffcient by its correction level */
	switch(axis->correction_level)
	{
	  case 2:
	    /* Fall through to correction level 1 */
	  case 1:       /* Minimal correction (level 1) */
	    if(1)
	    {
		int r, r_dz_nz;         /* Range and range w/ null and dead zones applied */
		int nz = axis->nz;      /* Null zone in raw units from center */
		int x = axis->cur;              /* Current raw axis position */
		int dx = x - axis->cen;         /* Raw delta from center */


		/* Null zone check */
		if((dx <= nz) && (dx >= -nz))
		    return(0.0);

		/* Negative from center? */
		if(dx < 0)
		{
		    /* Negative from center */
		    double dz_bounds_coeff = CLIP(axis->corr_coeff_min1, 0.0, 1.0);
		    int dz_dmin = axis->dz_min - axis->cen;     /* Dead zone relative
								 * to center */
		    int dz_dmin_nz = dz_dmin + nz;

		    r = axis->min - axis->cen;                  /* Negative range */
		    r_dz_nz = r - MIN(-nz, dz_dmin);

		    /* Inside the dead zone? */
		    if(dx >= dz_dmin)
		    {
			/* Inside of dead zone */
			double dz_coeff = ((dz_dmin_nz < 0) ?
			    (double)(dx + nz) / (double)dz_dmin_nz : 0.0
			);
			/* Note that dz_coeff is relative to the dead
			 * zone bound, not the range of the axis
			 */
			return(
			    dz_coeff * dz_bounds_coeff *
			    ((axis->flags & JSAxisFlagFlipped) ? 1.0 : -1.0)
			);
		    }
		    else
		    {
			/* Outside of dead zone */
			if(r_dz_nz < 0)
			    return(
				(((double)(dx - MIN(-nz, dz_dmin)) / (double)r_dz_nz *
				(1.0 - dz_bounds_coeff)) + dz_bounds_coeff) *
				((axis->flags & JSAxisFlagFlipped) ? 1.0 : -1.0)
			    );
			else
			    return(0.0);
		    }
		}
		else
		{
		    /* Positive from center */
		    double dz_bounds_coeff = CLIP(axis->corr_coeff_max1, 0.0, 1.0);
		    int dz_dmax = axis->dz_max - axis->cen;     /* Dead zone relative
								 * to center */
		    int dz_dmax_nz = dz_dmax - nz;

		    r = axis->max - axis->cen;                  /* Positive range */
		    r_dz_nz = r - MAX(nz, dz_dmax);

		    /* Inside the dead zone? */
		    if(dx <= dz_dmax)
		    {
			/* Inside of dead zone */
			double dz_coeff = ((dz_dmax_nz > 0) ?
			    (double)(dx - nz) / (double)dz_dmax_nz : 0.0
			);
			/* Note that dz_coeff is relative to the dead
			 * zone bound, not the range of the axis
			 */
			return(
			    dz_coeff * dz_bounds_coeff *
			    ((axis->flags & JSAxisFlagFlipped) ? -1.0 : 1.0)
			);
		    }
		    else
		    {
			/* Outside of dead zone */
			if(r_dz_nz > 0)
			    return(
				(((double)(dx - MAX(nz, dz_dmax)) / (double)r_dz_nz *
				(1.0 - dz_bounds_coeff)) + dz_bounds_coeff) *
				((axis->flags & JSAxisFlagFlipped) ? -1.0 : 1.0)
			    );
			else
			    return(0.0);
		    }
		}
	    }
	    break;

	  default:      /* No correction (level 0) */
	    if(1)
	    {
		int r, r_nz;            /* Range and range with nullzone applied */
		int nz = axis->nz;      /* Null zone in raw units from center */
		int x = axis->cur;      /* Current raw axis position */
		int dx = x - axis->cen; /* Raw delta from center */

		/* Null zone check */
		if((dx <= nz) && (dx >= -nz))
		    return(0.0);

		/* Negative from center? */
		if(dx < 0)
		{
		    /* Negative from center, calculate negative range */
		    r = axis->min - axis->cen;
		    r_nz = r + nz;
		    if(r_nz < 0)
			return(
			    (double)(dx + nz) / (double)r_nz *
			    (double)((axis->flags & JSAxisFlagFlipped) ? 1 : -1)
			);
		    else
			return(0.0);
		}
		else
		{
		    /* Positive from center, calculate positive range */
		    r = axis->max - axis->cen;
		    r_nz = r - nz;
		    if(r_nz > 0)
			return(
			    (double)(dx - nz) / (double)r_nz *
			    (double)((axis->flags & JSAxisFlagFlipped) ? -1 : 1)
			);
		    else
			return(0.0);
		}
	    }
	    break;
	}
}

/*
 *	Applies the tolorance value defined on each axis on the given
 *	jsd to the low-level joystick driver's tolorance.
 *
 *	This function is automatically called by JSInit().
 */
void JSResetAllAxisTolorance(js_data_struct *jsd)
{
	if(!JSIsInit(jsd))
	    return;

#if defined(__linux__)
	if(jsd->total_axises > 0)
	{
	    int i;
	    js_axis_struct *axis_ptr;
	    struct js_corr *corr = (struct js_corr *)calloc(
		jsd->total_axises, sizeof(struct js_corr)
	    );
	    if(corr == NULL)
		return;

	    for(i = 0; i < jsd->total_axises; i++)
	    {
		axis_ptr = jsd->axis[i];
		if(axis_ptr == NULL)
		    continue;

		corr[i].type = JS_CORR_NONE;
		corr[i].prec = (axis_ptr->flags & JSAxisFlagTolorance) ?
		    axis_ptr->tolorance : 0;
	    }

	    if(ioctl(jsd->fd, JSIOCSCORR, corr))
		fprintf(
		    stderr,
"Failed to set joystick %s correction values: %s\n",
		    jsd->device_name, strerror(errno)
		);

	    free(corr);
	}
#endif	/* __linux__ */


}
