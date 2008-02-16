#include <gtk/gtk.h>

#include "guiutils.h"
#include "jc.h"


static js_axis_struct *JCGetAxis(js_data_struct *jsd, gint i);

static void JCDrawAxisesRepresentative(jc_struct *jc);
static void JCDrawAxisesLogical(jc_struct *jc);
void JCDrawAxises(jc_struct *jc);
void JCDrawButtons(jc_struct *jc);
void JCDrawSignalLevel(jc_struct *jc);


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
 *	Returns the axis structure i from the jsd structure.
 *
 *	Can return NULL if axis number i is invalid or on error.
 */
static js_axis_struct *JCGetAxis(js_data_struct *jsd, gint i)
{
	if(jsd == NULL)
	    return(NULL);
	else if((i < 0) || (i >= jsd->total_axises))
	    return(NULL);
	else
	    return(jsd->axis[i]);
}


/*
 *	Redraws the Joystick Calibrator's representative axises layout.
 *
 *	The specified inputs are assumed to be valid.
 */
static void JCDrawAxisesRepresentative(jc_struct *jc)
{ 
	gint x, y, width, height, total_axises;
	GtkWidget *w;
	GdkPixmap *pixmap;
	GdkGC *gc;
	layout_rep_struct *lr;
	js_axis_struct *axis_ptr1, *axis_ptr2;
	gfloat	axis0_value, axis1_value, axis2_value, axis3_value,
		axis4_value, axis5_value;


	lr = &jc->layout_rep;

	/* Not initialized? */
	if(lr->toplevel == NULL)
	    return;

	gc = jc->gc;
	if(gc == NULL)
	    return;

	/* Get first four axis values (set value 0 if they do not
	 * exist)
	 */
	total_axises = jc->jsd.total_axises;
	axis0_value = JSGetAxisCoeff(&jc->jsd, 0);	/* Axis X */
	axis1_value = JSGetAxisCoeff(&jc->jsd, 1);	/* Axis Y */
	axis2_value = JSGetAxisCoeff(&jc->jsd, 2);	/* Rotate */
	axis3_value = JSGetAxisCoeff(&jc->jsd, 3);	/* Throttle */
	axis4_value = JSGetAxisCoeff(&jc->jsd, 4);	/* Hat X */
	axis5_value = JSGetAxisCoeff(&jc->jsd, 5);	/* Hat Y */


	/* Hat */
	w = lr->hat_da;
	pixmap = lr->hat_buf;
	gdk_window_get_size(pixmap, &width, &height);

	/* Draw solid background, this `clearing' the pixmap */
	gdk_gc_set_foreground(gc, &lr->c_axis_bg);
	gdk_draw_rectangle(
	    pixmap, gc,
	    TRUE,
	    0, 0,
	    width, height
	);

	/* Draw grids */
	x = (width / 2);
	y = (height / 2);
	gdk_gc_set_foreground(gc, &lr->c_axis_grid);
	gdk_draw_line(
	    pixmap, gc,
	    x, 0,
	    x, height - 1
	);
	gdk_draw_line( 
	    pixmap, gc,
	    0, y,
	    width - 1, y
	);

	/* Draw hat, determine the hat axis value by the total number
	 * of axises
	 */
	if(total_axises >= 6)
	{
	    x = (axis4_value * (width / 4)) + (width / 2);
	    y = (gfloat)height - (
	        (axis5_value * (height / 4)) + (height / 2)
	    );
	}
	else if(total_axises == 5)
	{
	    x = (axis3_value * (width / 4)) + (width / 2);
	    y = (gfloat)height - (
		(axis4_value * (height / 4)) + (height / 2)
	    );
	}
	else
	{
	    x = (width / 2);
	    y = height - (height / 2);
	}
	/* Draw a solid circle, representing the hat */
	gdk_gc_set_foreground(gc, &lr->c_axis_hat);
	gdk_draw_arc(
	    pixmap, gc,
	    TRUE,
	    x - 10, y - 10,
	    20, 20,
	    0 * 64, 360 * 64
	);
	/* Put the entire drawn pixmap to the window */
	gdk_draw_pixmap(
	    w->window, gc, pixmap,
	    0, 0,
	    0, 0,
	    width, height
	);


	/* First two axises */
	w = lr->axis_da;
	pixmap = lr->axis_buf;
	gdk_window_get_size(pixmap, &width, &height);

	/* Draw solid background, this `clearing' the pixmap */
	gdk_gc_set_foreground(gc, &lr->c_axis_bg);
	gdk_draw_rectangle(
	    pixmap, gc,
	    TRUE,
	    0, 0,
	    width, height
	);

	/* Null zone circle and grids */
	x = (width / 2);
	y = (height / 2);
	gdk_gc_set_foreground(gc, &lr->c_axis_grid);
	axis_ptr1 = JCGetAxis(&jc->jsd, 0);
	axis_ptr2 = JCGetAxis(&jc->jsd, 1);
	if((axis_ptr1 != NULL) &&
	   (axis_ptr2 != NULL)
	)
	{
	    gint axis_len1, win_len1;
	    gint axis_len2, win_len2;
  
	    axis_len1 = MAX(axis_ptr1->max - axis_ptr1->min, 1);
	    axis_len2 = MAX(axis_ptr2->max - axis_ptr2->min, 1);
	    win_len1 = (gfloat)width * (
		(gfloat)(2 * axis_ptr1->nz) / (gfloat)axis_len1
	    );
	    win_len2 = (gfloat)height * (
		(gfloat)(2 * axis_ptr2->nz) / (gfloat)axis_len2
	    );

	    gdk_draw_arc(
		pixmap, gc,
		FALSE,
		x - (win_len1 / 2), y - (win_len2 / 2),
		win_len1, win_len2,
		0 * 64, 360 * 64
	    );
	}
	gdk_draw_line(
	    pixmap, gc,
	    x, 0,
	    x, height - 1
	);
	gdk_draw_line(
	    pixmap, gc,
	    0, y,
	    width - 1, y
	);

	/* Cross hairs */
	x = (axis0_value * (gfloat)width / 2) + ((gfloat)width / 2);
	y = (gfloat)height - (gfloat)(
	    (axis1_value * (gfloat)height / 2) + ((gfloat)height / 2)
	);
	gdk_gc_set_foreground(gc, &lr->c_axis_fg);
	gdk_draw_line(
	    pixmap, gc,
	    x - 5, y, x + 6, y
	);
	gdk_draw_line(
	    pixmap, gc,
	    x, y - 5, x, y + 6
	);
	/* Put the entire drawn pixmap to the window */
	gdk_draw_pixmap(
	    w->window,
	    gc,
	    pixmap,
	    0, 0,
	    0, 0,
	    width, height
	);


	/* Throttle */
	w = lr->throttle_da;
	pixmap = lr->throttle_buf;
	gdk_window_get_size(pixmap, &width, &height);

	/* Draw solid background, this `clearing' the pixmap */
	gdk_gc_set_foreground(gc, &lr->c_axis_bg);
	gdk_draw_rectangle(
	    pixmap, gc,
	    TRUE,
	    0, 0,
	    width, height
	);

	/* Get throttle axis number based on total axises */
	if((total_axises >= 6) || (total_axises == 4))
	{
	    y = height - (
		(axis3_value * height / 2) + (height / 2)
	    );
	}
	else if((total_axises == 5) || (total_axises == 3))
	{
	    y = height - (
		(axis2_value * height / 2) + (height / 2)
	    );
	}
	else
	{
	    y = height - 0;
	}
	/* Draw throttle */
	gdk_gc_set_foreground(gc, &lr->c_throttle_fg);
	gdk_draw_rectangle(
	    pixmap, gc,
	    TRUE,
	    0, y,
	    width, height - y
	);
	/* Put the entire drawn pixmap to the window */
	gdk_draw_pixmap(
	    w->window, gc, pixmap,
	    0, 0,
	    0, 0,
	    width, height
	);


	/* Rotate */
	w = lr->rotate_da;
	pixmap = lr->rotate_buf;
	gdk_window_get_size(pixmap, &width, &height);

	/* Draw solid background, this `clearing' the pixmap */
	gdk_gc_set_foreground(gc, &lr->c_axis_bg);
	gdk_draw_rectangle(
	    pixmap, gc,
	    TRUE,
	    0, 0,
	    width, height
	);

	/* Draw grids and null zone bounds */
	gdk_gc_set_foreground(gc, &lr->c_axis_grid);
	x = width / 2;
	axis_ptr1 = JCGetAxis(&jc->jsd, 2);
	if(axis_ptr1 != NULL)
	{
	    gint axis_len1, win_len1;

	    axis_len1 = MAX(axis_ptr1->max - axis_ptr1->min, 1);
	    win_len1 = (gfloat)width * (
		(gfloat)(2 * axis_ptr1->nz) / (gfloat)axis_len1
	    );

	    gdk_draw_line(
		pixmap, gc,
		x - (win_len1 / 2), 1,
		x - (win_len1 / 2), height - 1
	    );
	    gdk_draw_line(
		pixmap, gc,
		x + (win_len1 / 2), 1,
		x + (win_len1 / 2), height - 1
	    );
	}
	gdk_draw_line(
	    pixmap, gc,
	    x, 1,
	    x, height - 1
	);

	/* Get rotate axis value by the total number of axises */
	if((total_axises >= 6) || (total_axises == 4))
	{
	    x = (axis2_value * width / 2) + (width / 2);
	}
	else
	{
	    x = width / 2;
	}
	gdk_gc_set_foreground(gc, &lr->c_axis_fg);
	gdk_draw_line(
	    pixmap, gc,
	    x, 1,
	    x, height - 1
	);
	/* Put the entire drawn pixmap to the window */
	gdk_draw_pixmap(
	    w->window, gc, pixmap,
	    0, 0,
	    0, 0,
	    width, height
	);
}

/*
 *	Redraws the Joystick Calibrator's logical axises layout.
 *
 *	The specified inputs are assumed to be valid.
 */
static void JCDrawAxisesLogical(jc_struct *jc)
{
	const gint border_minor = 2;
	gint i, x, width, height, center_w;
	gfloat rtow_coeff, axis_coeff_value;
	GtkWidget *w;
	GdkPixmap *pixmap;
	GdkGC *gc;
	GdkFont *font;
	layout_logical_struct *ll;
	layout_logical_axis_struct *ll_axis_ptr;
	js_axis_struct *axis_ptr;


	ll = &jc->layout_logical;

	/* Not initialized? */
	if(ll->toplevel == NULL)
	    return;

	gc = jc->gc;
	if(gc == NULL)
	    return;

	/* Itterate through each axis */
	for(i = 0; i < ll->total_axises; i++)
	{
	    ll_axis_ptr = ll->axis[i];
	    if(ll_axis_ptr == NULL)
		continue;

	    /* Skip uninitialized ones */
	    if(!ll_axis_ptr->initialized)
		continue;

	    if((ll_axis_ptr->position_buf == NULL) ||
	       (ll_axis_ptr->position_da == NULL)
	    )
		continue;

	    /* Get axis value coefficient */
	    axis_coeff_value = JSGetAxisCoeff(&jc->jsd, i);

	    /* Get drawing area widget and pixmap buffer */
	    w = ll_axis_ptr->position_da;
	    pixmap = ll_axis_ptr->position_buf;

	    /* Get size of pixmap buffer */
	    gdk_window_get_size(pixmap, &width, &height);

	    font = ll_axis_ptr->font;


	    /* Begin drawing on the pixmap buffer */

	    /* Draw a solid rectangle to `clear' the pixmap buffer */
	    gdk_gc_set_foreground(gc, &ll_axis_ptr->c_axis_bg);
	    gdk_draw_rectangle(
		pixmap, gc,
		TRUE,
		0, 0,
		width, height
	    );

	    /* Calculate center position in window coordinates */
	    center_w = width / 2;

	    /* Get pointer to axis */
	    axis_ptr = JCGetAxis(&jc->jsd, i);
	    if(axis_ptr != NULL)
	    {
		gboolean flip;
		gint n, axis_range, axis_range_w, nz_len_w;
		GdkPoint p[3];


		/* Get flip flag */
		flip = (axis_ptr->flags & JSAxisFlagFlipped) ? TRUE : FALSE;

		/* Get full range of axis in raw units, this value
		 * is gauranteed to be positive.
		 */
		axis_range = MAX(axis_ptr->max - axis_ptr->min, 1);

		/* Calculate axis range in window coordinates, this
		 * will be the width minus margins on both sides.
		 */
		axis_range_w = width - (2 * border_minor);

		/* Calculate horizontal raw units to window units 
		 * coefficient relative to axis_range_w, not width.
		 */
		rtow_coeff = (gfloat)axis_range_w / (gfloat)axis_range;

		/* Get length of null zone in window units */
		nz_len_w = (2 * axis_ptr->nz) * rtow_coeff;


		/* Draw null zone region */
		gdk_gc_set_foreground(gc, &ll_axis_ptr->c_axis_nz);
		gdk_draw_rectangle(
		    pixmap, gc,
		    TRUE,
		    center_w - (nz_len_w / 2), 0,
		    nz_len_w, height
		);


		/* Begin drawing grid and ticks */
		gdk_gc_set_foreground(gc, &ll_axis_ptr->c_axis_grid);
		/* Vertical center line */
		gdk_draw_line(
		    pixmap, gc,
		    center_w, height - 5 - border_minor,
		    center_w, height - 1 - border_minor
		);
		/* Vertical ticks at every 25% to the left */
		for(n = 0; n < 3; n++)
		{
		    x = center_w - (axis_range_w / 8) - (n * axis_range_w / 8);
		    gdk_draw_line(
			pixmap, gc,
			x, height - 5 - border_minor,
			x, height - 1 - border_minor
		    );
		}
		/* Left most vertical tick */
		x = border_minor;
		gdk_draw_line(
		    pixmap, gc,
		    x, height - 5 - border_minor,
		    x, height - 1 - border_minor
		);
		/* Vertical ticks at every 25% to the right */
		for(n = 0; n < 3; n++)
		{
		    x = center_w + (axis_range_w / 8) + (n * axis_range_w / 8);
		    gdk_draw_line(
			pixmap, gc,
			x, height - 5 - border_minor,
			x, height - 1 - border_minor
		    );
		}
		/* Right most vertical tick */
		x = width - 1 - border_minor;
		gdk_draw_line(
		    pixmap, gc,
		    x, height - 5 - border_minor,
		    x, height - 1 - border_minor
		);
		/* Long horizontal grid */
		gdk_draw_line(
		    pixmap, gc,
		    border_minor, height - 1 - border_minor,
		    width - border_minor, height - 1 - border_minor
		);


		/* Draw dead zone bars, only if correction level is above
		 * level 0.
		 */
		if(axis_ptr->correction_level > 0)
		{
		    int dz_min = MIN(axis_ptr->dz_min - axis_ptr->cen, 0),
			dz_max = MAX(axis_ptr->dz_max - axis_ptr->cen, 0);
		    int dz_min_w = dz_min * rtow_coeff,
			dz_max_w = dz_max * rtow_coeff;

		    /* Draw dead zone edge lines */
		    gdk_gc_set_foreground(gc, &ll_axis_ptr->c_axis_fg);
		    x = center_w + (flip ? -dz_min_w : dz_min_w);
		    if(dz_min < 0)
		    {
			gdk_draw_line(
			    pixmap, gc,
			    x, height - 5 - border_minor,
			    x, height - 1 - border_minor
			);
			gdk_draw_line(
			    pixmap, gc,
			    x, height - 2 - border_minor,
			    center_w, height - 2 - border_minor
			);
		    }
		    x = center_w + (flip ? -dz_max_w : dz_max_w);
		    if(dz_max > 0)
		    {
			gdk_draw_line(
			    pixmap, gc,
			    x, height - 5 - border_minor,
			    x, height - 1 - border_minor
			);
			gdk_draw_line(
			    pixmap, gc,
			    center_w, height - 2 - border_minor,
			    x, height - 2 - border_minor
			);
		    }
		}


		/* Calculate current axis position in window coordinates
		 * and set up to draw current axis position arrow.
		 */
		x = (axis_coeff_value * (gfloat)axis_range_w / 2) +
		    center_w;

		/* Calculate arrow points, check if current axis
		 * position x in window coordinates is in bounds or
		 * not.
		 */
		if(x < border_minor)
		{
		    /* At or beyond the left edge, draw an arrow pointing
		     * to the left
		     */
		    x = 0;
		    p[0].x = x + 4;
		    p[0].y = height - 11 - border_minor;
		    p[1].x = x;
		    p[1].y = height - 8 - border_minor;
		    p[2].x = x + 4;
		    p[2].y = height - 5 - border_minor;
		}
		else if(x > (width - 1 - border_minor))
		{
		    /* At or beyond the right edge, draw an arrow pointing
		     * to the right
		     */
		    x = width - 1;
		    p[0].x = x - 4;
		    p[0].y = height - 11 - border_minor;
		    p[1].x = x;
		    p[1].y = height - 8 - border_minor;
		    p[2].x = x - 4;
		    p[2].y = height - 5 - border_minor;
		}
		else
		{
		    /* In bounds, draw arrow pointing down */
		    p[0].x = x - 3;
		    p[0].y = height - 9 - border_minor;
		    p[1].x = x + 4;
		    p[1].y = height - 9 - border_minor;
		    p[2].x = x;
		    p[2].y = height - 5 - border_minor;
		}
		/* Draw the arrow as a polygon */
		gdk_gc_set_foreground(gc, &ll_axis_ptr->c_axis_fg);
		gdk_draw_polygon(
		    pixmap, gc,
		    TRUE,
		    p, 3
		);

		/* Draw text */
		if((axis_ptr != NULL) && (font != NULL))
		{
		    gchar text[80];
		    GdkTextBounds b;

		    g_snprintf(
			text, sizeof(text),
			"%i", axis_ptr->cur
		    );
		    gdk_string_bounds(font, text, &b);
		    gdk_draw_string(
			pixmap, font, gc,
			(axis_coeff_value > -0.25) ?
			    border_minor + b.lbearing :
			    width - b.width - border_minor + b.lbearing,
			border_minor + font->ascent,
			text
		    );
		}
	    }

	    /* Put the entire drawn pixmap to the window */
	    gdk_draw_pixmap(
		w->window, gc, pixmap,
		0, 0,
		0, 0,
		width, height
	    );
	}
}


/*
 *	Redraws the Joystick Calibrator's axises layout.
 */
void JCDrawAxises(jc_struct *jc)
{
	if(jc == NULL)
	    return;

	switch(jc->layout_state)
	{
	  case JC_LAYOUT_REPRESENTATIVE:
	    JCDrawAxisesRepresentative(jc);
	    break;

	  case JC_LAYOUT_LOGICAL:
	    JCDrawAxisesLogical(jc);
	    break;
	}
}

/*
 *	Draws the Joystick Calibrator's button labels.
 */
void JCDrawButtons(jc_struct *jc)
{
	gint i, total;
	buttons_list_struct *bl;
	js_button_struct *button_ptr;
	GtkWidget *w;


	if(jc == NULL)
	    return;

	/* Get buttons list from jc structure */
	bl = &jc->buttons_list;

	/* Get total button labels */
	total = bl->total_buttons;

	/* Make sure that total for button labels does not exceed the
	 * total number of buttons on the joystick data
	 */
	if(total > jc->jsd.total_buttons)
	    total = jc->jsd.total_buttons;

	/* Itterate through each button */
	for(i = 0; i < total; i++)
	{
	    w = bl->button[i];
	    if(w == NULL)
		continue;

	    button_ptr = jc->jsd.button[i];
	    if(button_ptr == NULL)
		continue;

	    /* Set new button state, thus `redrawing it' */
	    GTK_BUTTON(w)->in_button = (button_ptr->state == JSButtonStateOn) ?
		1 : 0;
	    gtk_signal_emit_by_name(
		GTK_OBJECT(w),
		(button_ptr->state == JSButtonStateOn) ?
		    "pressed" : "released"
	    );
	}
}


/*
 *	Draws the Joystick Calibrator's signal level.
 */
void JCDrawSignalLevel(jc_struct *jc)
{
	gint i, width, height;
	guint16 *history;
	GtkWidget *w;
	GdkPixmap *pixmap;
	GdkGC *gc;

	if(jc == NULL)
	    return;

	/* Get references to the graphics context, drawing area and
	 * GdkPixmap buffer
	 */
	gc = jc->gc;
	w = jc->signal_level_da;
	pixmap = jc->signal_level_pixmap;
	if((gc == NULL) || (w == NULL) || (pixmap == NULL))
	    return;

	/* Get size of the GdkPixmap buffer */
	gdk_window_get_size(pixmap, &width, &height);
	if((width < 1) || (height < 1))
	    return;

	/* Draw background, thus `clearing' the pixmap */
	gdk_gc_set_foreground(gc, &jc->c_signal_level_bg);
	gdk_draw_rectangle(
	    pixmap, gc,
	    TRUE,
	    0, 0,
	    width, height
	);

	/* Begin drawing each vertical bar representing the signal value */
	gdk_gc_set_foreground(gc, &jc->c_signal_level_fg);
	history = jc->signal_history;
	for(i = 0; i < JC_SIGNAL_HISTORY_MAX; i++)
	{
	    gdk_draw_line(
		pixmap, gc,
		i,
		height - (
		    height * ((gfloat)history[i] /
			(gfloat)((guint16)-1)
		    )
		) - 1,
		i,
		height - 1
	    );
	}

	/* Put entire drawn pixmap to window */
	gdk_draw_pixmap(
	    w->window, gc,
	    pixmap,
	    0, 0,
	    0, 0,
	    width, height
	);
}
