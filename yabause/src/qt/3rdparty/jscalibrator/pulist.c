#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "guiutils.h"
#include "pulist.h"


typedef struct _PopupList		PopupList;
#define POPUP_LIST(p)			((PopupList *)(p))
#define POPUP_LIST_KEY			"PopupList"

typedef struct _PopupListBox		PopupListBox;
#define POPUP_LIST_BOX(p)		((PopupListBox *)(p))
#define POPUP_LIST_BOX_KEY		"PopupListBox"


/*
 *	Popup List Flags:
 */
typedef enum {
	POPUP_LIST_MAPPED		= (1 << 0),
	POPUP_LIST_REALIZED		= (1 << 1),
	POPUP_LIST_BUTTON_PRESS_SENT	= (1 << 2)	/* Marks that the first
							 * "button_press_event"
							 * signal was sent to
							 * the GtkCList */
} PopupListFlags;


/* Popup List Callbacks */
static void PUListDestroyCB(gpointer data);
static gint PUListDeleteEventCB(
	GtkWidget *widget, GdkEvent *event, gpointer data
);
static gint PUListKeyPressEventCB(
	GtkWidget *widget, GdkEventKey *key, gpointer data
);
static gint PUListButtonPressEventCB(
	GtkWidget *widget, GdkEventButton *button, gpointer data
);
static gint PUListMotionNotifyEventCB(
	GtkWidget *widget, GdkEventMotion *motion, gpointer data
);
static gint PUListCrossingEventCB(
	GtkWidget *widget, GdkEventCrossing *crossing, gpointer data
);
static gint PUListMapIdleCB(gpointer data);
static void PUListRealizeCB(GtkWidget *widget, gpointer data);

/* Shadow Callbacks */
static gint PUListShadowExposeEventCB(
	GtkWidget *widget, GdkEventExpose *expose, gpointer data
);
static void PUListShadowDrawCB(
	GtkWidget *widget, GdkRectangle *area, gpointer data
);
static void PUListShadowDraw(PopupList *list);

/* Drag Setup/Cleanup */
static void PUListCListDragSetUp(PopupList *list);
static void PUListCListDragCleanUp(PopupList *list);


/* Map Button Callbacks */
static gint PUListMapButtonExposeCB(
	GtkWidget *widget, GdkEventExpose *expose, gpointer data
);


/* Popup List Box Callbacks */
static void PUListBoxDestroyCB(gpointer data);
static gint PUListBoxEventCB(
	GtkWidget *widget, GdkEvent *event, gpointer data
);
static void PUListBoxMapCB(GtkWidget *widget, gpointer data);
static void PUListBoxDraw(PopupListBox *box);


/* Finding */
gint PUListFindItemFromValue(
	GtkWidget *w,
	const gchar *value
);
gpointer PUListGetDataFromValue(
	GtkWidget *w,
	const gchar *value
);

/* Add/Remove Item */
gint PUListAddItem(
	GtkWidget *w,
	const gchar *value
);
gint PUListAddItemPixText(
	GtkWidget *w,
	const gchar *value,
	GdkPixmap *pixmap, GdkBitmap *mask
);
void PUListClear(GtkWidget *w);

/* Set Item */
void PUListSetItemText(
	GtkWidget *w,
	const gint i,
	const gchar *value
);
void PUListSetItemPixText(
	GtkWidget *w,
	const gint i,
	const gchar *value,
	GdkPixmap *pixmap, GdkBitmap *mask
);
void PUListSetItemSensitive(
	GtkWidget *w,
	const gint i,
	const gboolean sensitive
);
void PUListSetItemData(
	GtkWidget *w,
	const gint i,
	gpointer data
);
void PUListSetItemDataFull(
	GtkWidget *w,
	const gint i,
	gpointer data,
	GtkDestroyNotify destroy_cb
);

/* Get Item */
gint PUListGetTotalItems(GtkWidget *w);
void PUListGetItemText(
	GtkWidget *w,
	const gint i,
	const gchar **value_rtn
);
void PUListGetItemPixText(
	GtkWidget *w,
	const gint i,
	const gchar **value_rtn,
	GdkPixmap **pixmap_rtn, GdkBitmap **mask_rtn
);
gboolean PUListGetItemSensitive(
	GtkWidget *w,
	const gint i
);
gpointer PUListGetItemData(
	GtkWidget *w,
	const gint i
);

/* Selecting */
gint PUListGetSelectedLast(GtkWidget *w);
void PUListSelect(
	GtkWidget *w,
	const gint i
);
void PUListUnselectAll(GtkWidget *w);

/* Query */
gboolean PUListIsQuery(GtkWidget *w);
void PUListBreakQuery(GtkWidget *w);
const gchar *PUListMapQuery(
	GtkWidget *w,
	const gchar *value,
	const gint nlines_visible,
	const pulist_relative relative,
	GtkWidget *rel_widget,
	GtkWidget *map_widget
);

/* Popup List */
static PopupList *PUListGetWidgetData(
        GtkWidget *w,
        const gchar *func_name
);
GtkWidget *PUListNew(void);
GtkWidget *PUListGetCList(GtkWidget *w);
void PUListSetShadowStyle(
	GtkWidget *w,
	const pulist_shadow_style shadow_style
);
GtkWidget *PUListRef(GtkWidget *w);
GtkWidget *PUListUnref(GtkWidget *w);


/* Map Button */
GtkWidget *PUListNewMapButton(
	void (*map_cb)(GtkWidget *, gpointer),
	gpointer client_data
);
GtkWidget *PUListNewMapButtonArrow(
	GtkArrowType arrow_type, GtkShadowType shadow_type,
	void (*map_cb)(GtkWidget *, gpointer),
	gpointer client_data
);


/* Popup List Box */
static PopupListBox *PUListBoxGetWidgetData(
        GtkWidget *w,
        const gchar *func_name
);
GtkWidget *PUListBoxNew(
	const gint width, const gint height
);
GtkWidget *PUListBoxGetPUList(GtkWidget *w);
void PUListBoxSetLinesVisible(
	GtkWidget *w,
	const gint nlines_visible
);
void PUListBoxSetChangedCB(
	GtkWidget *w,
	void (*func)(
		GtkWidget *,			/* Popup List Box */
		const gint,			/* Item index */
		gpointer			/* Data */
	),
	gpointer data
);
void PUListBoxSetTip(
	GtkWidget *w,
	const gchar *tip
);
void PUListBoxGrabFocus(GtkWidget *w);
void PUListBoxSelect(
	GtkWidget *w,
	const gint i
);
gint PUListBoxGetTotalItems(GtkWidget *w);
gint PUListBoxGetSelected(GtkWidget *w);
gpointer PUListBoxGetSelectedData(GtkWidget *w);


#define POPUP_LIST_DEF_WIDTH		320
#define POPUP_LIST_DEF_HEIGHT		200

#define POPUP_LIST_ROW_SPACING		20

#define POPUP_LIST_MAP_BTN_WIDTH	17
#define POPUP_LIST_MAP_BTN_HEIGHT	-1

#define PULIST_SHADOW_OFFSET_X	5
#define PULIST_SHADOW_OFFSET_Y	5

#define POPUP_LIST_DEF_NLINES_VISIBLE	10


/* Timeout interval in milliseconds, this is effectivly the scrolling
 * interval of the GtkCList when the button is first pressed to map
 * the Popup List and then dragged over to the GtkCList without
 * releasing the button
 */
#define POPUP_LIST_TIMEOUT_INT		80l


#define ATOI(s)		(((s) != NULL) ? atoi(s) : 0)
#define ATOL(s)		(((s) != NULL) ? atol(s) : 0)
#define ATOF(s)		(((s) != NULL) ? atof(s) : 0.0f)
#define STRDUP(s)	(((s) != NULL) ? g_strdup(s) : NULL)

#define MAX(a,b)	(((a) > (b)) ? (a) : (b))
#define MIN(a,b)	(((a) < (b)) ? (a) : (b))
#define CLIP(a,l,h)	(MIN(MAX((a),(l)),(h)))
#define STRLEN(s)	(((s) != NULL) ? strlen(s) : 0)
#define STRISEMPTY(s)	(((s) != NULL) ? (*(s) == '\0') : TRUE)


/*
 *	Popup List:
 */
struct _PopupList {

	GtkWidget	*toplevel;
	gint		freeze_count,
			ref_count,
			main_level;
	PopupListFlags	flags;
	pulist_shadow_style     shadow_style;

	GtkWidget	*main_vbox,
			*scrolled_window,
			*vscrollbar,
			*hscrollbar,
			*clist,
			*shadow;

	GdkPixmap	*shadow_pm;

	/* Shared reference to the GtkWidget that mapped the Popup
	 * List
	 */
	GtkWidget	*map_widget;

	/* Copy of the last value for return in PUListMapQuery() */
	gchar		*last_value;
};


/*
 *      Popup List Box:
 */
struct _PopupListBox {

	GtkWidget	*toplevel;
	gint		freeze_count;

	GtkWidget	*box_frame,		/* Box GtkFrame */
			*box_da,		/* Box GtkDrawingArea */
			*map_btn,		/* Map GtkButton */
			*popup_list;		/* Popup List */

	gint		nlines_visible;		/* Number of lines visible
						 * on the Popup List's
						 * GtkCList */

	/* Changed Callback */
	void	(*changed_cb)(
		GtkWidget *,			/* Popup List Box */
		const gint,			/* Item */
		gpointer			/* Data */
	);
	gpointer	changed_data;

};


/*
 *	Popup List "destroy" signal callback.
 */
static void PUListDestroyCB(gpointer data)
{
	PopupList *list = POPUP_LIST(data);
	if(list == NULL)
	    return;

	if(list->ref_count > 0)
	{
	    g_printerr(
"PUListDestroyCB():\
 Warning: Destroying PopupList %p with %i reference counts remaining.\n\
Was PUListUnref() not used properly to unref this PopupList?\n",
		list->toplevel,
		list->ref_count
	    );
	}

	list->freeze_count++;

	/* Break out of any remaining GTK main levels */
	if(list->main_level > 0)
	{
	    g_printerr(
"PUListDestroyCB(): \
Warning: Destroying PopupList %p with %i main levels remaining.\n\
Is this PopupList being destroyed during a query?\n",
		list->toplevel,
		list->main_level
	    );
	    while(list->main_level > 0)
	    {
		list->main_level--;
		gtk_main_quit();
	    }
	    g_printerr(
"Leaving PUListDestroyCB() at a lower main level, any further\n\
references to this PopupList will cause a segmentation fault.\n"
	    );
	}

	/* Destroy the shadow GtkWindow */
	GTK_WIDGET_DESTROY(list->shadow);

	/* Unref the shadow GdkPixmap */
	GDK_PIXMAP_UNREF(list->shadow_pm);

	/* Delete the return value */
	g_free(list->last_value);

	list->freeze_count--;

	g_free(list);
}

/*
 *	Popup List toplevel GtkWindow "delete_event" signal callback.
 */
static gint PUListDeleteEventCB(
	GtkWidget *widget, GdkEvent *event, gpointer data
)
{
	PopupList *list = POPUP_LIST(data);
	if(list == NULL)
	    return(FALSE);

	if(list->freeze_count > 0)
	    return(FALSE);

	/* Break out of any main levels */
	while(list->main_level > 0)
	{
	    list->main_level--;
	    gtk_main_quit();
	}

	return(FALSE);
}

/*
 *	Popup List "key_press_event" and "key_release_event" signal
 *      callback.
 */
static gint PUListKeyPressEventCB(
	GtkWidget *widget, GdkEventKey *key, gpointer data
)
{
	gboolean press;
	gint status = FALSE;
	guint keyval, state;
	PopupList *list = POPUP_LIST(data);
	if((widget == NULL) || (key == NULL) || (list == NULL))
	    return(status);

	if(list->freeze_count > 0)
	    return(status);

/* Breaks out of all the GTK main levels */
#define BREAK_ALL_GTK_MAIN_LEVELS(_list_) {	\
 while((_list_)->main_level > 0) {		\
  (_list_)->main_level--;			\
  gtk_main_quit();				\
 }						\
}

/* Emits a signal stop for the key event */
#define STOP_KEY_SIGNAL_EMIT(_w_)	{	\
 gtk_signal_emit_stop_by_name(			\
  GTK_OBJECT(_w_),				\
  press ?					\
   "key_press_event" : "key_release_event"	\
 );						\
}

	press = (key->type == GDK_KEY_PRESS) ? TRUE : FALSE;
	keyval = key->keyval;
	state = key->state;

	/* Note: Only "key_press_events" seem to be reported and not
	 * "key_release_events", so always check if press is TRUE
	 */
	if(widget == list->clist)
	{
	    GtkCList *clist = GTK_CLIST(widget);

	    /* Handle by key value */
	    switch(keyval)
	    {
	      case GDK_space:
	      case GDK_Return:
	      case GDK_KP_Enter:
	      case GDK_ISO_Enter:
	      case GDK_3270_Enter:
		if(press)
		{
		    /* Activate selected item */
		    GList *glist = clist->selection_end;
		    gint row = (glist != NULL) ? (gint)glist->data : -1;
		    if((row >= 0) && (row < clist->rows))
		    {
			gchar *text = NULL;
			guint8 spacing;
			GdkPixmap *pixmap;
			GdkBitmap *mask;

			switch(gtk_clist_get_cell_type(clist, row, 0))
			{
			  case GTK_CELL_TEXT:
			    gtk_clist_get_text(clist, row, 0, &text);
			    break;
			  case GTK_CELL_PIXTEXT:
			    gtk_clist_get_pixtext(
				clist, row, 0, &text,
				&spacing, &pixmap, &mask
			    );
			    break;
			  case GTK_CELL_PIXMAP:
			  case GTK_CELL_WIDGET:
			  case GTK_CELL_EMPTY:
			    break;
			}
			if(!STRISEMPTY(text))
			{
			    g_free(list->last_value);
			    list->last_value = STRDUP(text);
			}
		    }
		    /* Break query */
		    BREAK_ALL_GTK_MAIN_LEVELS(list);
		}
		STOP_KEY_SIGNAL_EMIT(widget);
		status = TRUE;
		break;

	      case GDK_Escape:
		if(press)
		{
		    /* Break query */
		    BREAK_ALL_GTK_MAIN_LEVELS(list);
		}
		STOP_KEY_SIGNAL_EMIT(widget);
		status = TRUE;
		break;
#if 0
	      case GDK_Page_Up:
	      case GDK_KP_Page_Up:
		if(press)
		{
		    /* Get adjustment and scroll up one page */
		    GtkAdjustment *adj = clist->vadjustment;
		    if(adj != NULL)
			GTK_ADJUSTMENT_SET_VALUE(
			    adj,
			    adj->value - adj->page_increment
			);
		}
		STOP_KEY_SIGNAL_EMIT(widget);
		status = TRUE;
		break;

	      case GDK_Page_Down:
	      case GDK_KP_Page_Down:
		if(press)
		{
		    /* Get adjustment and scroll down one page */
		    GtkAdjustment *adj = clist->vadjustment;
		    if(adj != NULL)
			GTK_ADJUSTMENT_SET_VALUE(
			    adj,
			    adj->value + adj->page_increment
			);
		}
		STOP_KEY_SIGNAL_EMIT(widget);
		status = TRUE;
		break;

	      case GDK_Home:
	      case GDK_KP_Home:
		if(press)
		{
		    /* Get adjustment and scroll all the way up */
		    GtkAdjustment *adj = clist->vadjustment;
		    if(adj != NULL)
			GTK_ADJUSTMENT_SET_VALUE(
			    adj,
			    adj->lower
			);
		}
		STOP_KEY_SIGNAL_EMIT(widget);
		status = TRUE;
		break;

	      case GDK_End:
	      case GDK_KP_End:
		if(press)
		{
		    /* Get adjustment and scroll all the way up */
		    GtkAdjustment *adj = clist->vadjustment;
		    if(adj != NULL)
			GTK_ADJUSTMENT_SET_VALUE(
			    adj,
			    adj->upper - adj->page_size
			);
		}
		STOP_KEY_SIGNAL_EMIT(widget);
		status = TRUE;
		break;
#endif
	    }
	}

#undef BREAK_ALL_GTK_MAIN_LEVELS
#undef STOP_KEY_SIGNAL_EMIT

	return(status);
}

/*
 *	Popup List "button_press_event" or "button_release_event"
 *	signal callback.
 */
static gint PUListButtonPressEventCB(
	GtkWidget *widget, GdkEventButton *button, gpointer data
)
{
	gint status = FALSE;
	PopupList *list = POPUP_LIST(data);
	if((widget == NULL) || (button == NULL) || (list == NULL))
	    return(status);

	if(list->freeze_count > 0)
	    return(status);

#define BREAK_ALL_GTK_MAIN_LEVELS(_list_)	{	\
 while((_list_)->main_level > 0) {		\
  (_list_)->main_level--;			\
  gtk_main_quit();				\
} }

#define RESTORE_MAP_WIDGET(_map_w_)	{	\
 if((_map_w_) != NULL) {			\
  /* Restore by the widget's type */		\
  if(GTK_IS_BUTTON(_map_w_)) {			\
   /* It is a button, make it go into		\
    * its released state			\
    */						\
   GtkButton *button = GTK_BUTTON(_map_w_);	\
   button->in_button = 1;			\
   button->button_down = 1;			\
   gtk_signal_emit_by_name(			\
    GTK_OBJECT(_map_w_), "released"		\
   );						\
   gtk_signal_emit_by_name(			\
    GTK_OBJECT(_map_w_), "leave"		\
   );						\
  }						\
 }						\
}

	/* See which widget this event is for */
	if(widget == list->clist)
	{
	    gint x, y;
	    GtkCList *clist = GTK_CLIST(widget);

	    /* Get the widget that this event occured over */
	    GtkWidget *w = gtk_get_event_widget((GdkEvent *)button);

	    switch((gint)button->type)
	    {
	      case GDK_BUTTON_PRESS:
		x = (gint)button->x;
		y = (gint)button->y;

		/* Did this event occure outside the clist? */
		if(w != widget)
		{
		    /* Clicked on one of the other GtkWidgets
		     * belonging to the Popup List?
		     */
		    if((w == list->vscrollbar) ||
		       (w == list->hscrollbar) ||
		       (w == list->scrolled_window) ||
		       (w == list->toplevel)
		    )
		    {
			/* Forward event to that widget */
			gtk_widget_event(w, (GdkEvent *)button);
		    }
		    else
		    {
			/* All else assume pressed in some other widget
			 *
			 * Break out of the block loop and restore the
			 * map widget
			 */
			BREAK_ALL_GTK_MAIN_LEVELS(list);
			RESTORE_MAP_WIDGET(list->map_widget);
		    }
		}
		/* If button 1 was pressed then mark the initial button
		 * press as sent (regardless of if this event is
		 * synthetic or not)
		 */
		if(button->button == GDK_BUTTON1)
		{
		    if(!(list->flags & POPUP_LIST_BUTTON_PRESS_SENT))
			list->flags |= POPUP_LIST_BUTTON_PRESS_SENT;
		}
		status = TRUE;
		break;

	      case GDK_BUTTON_RELEASE:
		x = (gint)button->x;
		y = (gint)button->y;
		switch(button->button)
		{
		  case GDK_BUTTON1:
		    /* Did this event occure inside the clist? */
		    if((w == widget) &&
		       (x >= 0) && (x < widget->allocation.width) &&
		       (y >= 0) && (y < widget->allocation.height)
		    )
		    {
			/* Button was released inside the clist,
			 * meaning we now have a matched item
			 */
			GList *glist = clist->selection_end;
			gint row = (glist != NULL) ? (gint)glist->data : -1;
			if((row >= 0) && (row < clist->rows))
			{
			    gchar *text = NULL;
			    guint8 spacing;
			    GdkPixmap *pixmap;
			    GdkBitmap *mask;
			    switch(gtk_clist_get_cell_type(clist, row, 0))
			    {
			      case GTK_CELL_TEXT:
				gtk_clist_get_text(clist, row, 0, &text);
			        break;
			      case GTK_CELL_PIXTEXT:
			        gtk_clist_get_pixtext(
				    clist, row, 0, &text,
				    &spacing, &pixmap, &mask
			        );
			        break;
			      case GTK_CELL_PIXMAP:
			      case GTK_CELL_WIDGET:
			      case GTK_CELL_EMPTY:
			        break;
			    }
			    /* If we got a value then update the
			     * last recorded value on the Popup List
			     */
			    if(!STRISEMPTY(text))
			    {
				g_free(list->last_value);
				list->last_value = STRDUP(text);
			    }
		        }

			/* Break query */
			BREAK_ALL_GTK_MAIN_LEVELS(list);
		    }
		    else
		    {
			/* The button was released outside of the clist */

			/* Released over one of the other GtkWidgets
			 * belonging to the Popup List?
			 */
			if((w == list->vscrollbar) ||
			   (w == list->hscrollbar) ||
			   (w == list->scrolled_window) ||
			   (w == list->toplevel)
			)
			{
			    /* Forward this event to that widget */
			    gtk_widget_event(w, (GdkEvent *)button);
			}

			/* Grab the clist again, since releasing the
			 * button outside the clist would have
			 * ungrabbed it and not unmap it
			 */
			if(widget != gtk_grab_get_current())
			    gtk_grab_add(widget);
		    }
		    break;
		}
		/* Restore the map widget on any button release */
		RESTORE_MAP_WIDGET(list->map_widget);
		status = TRUE;
		break;
	    }
	}

#undef RESTORE_MAP_WIDGET
#undef BREAK_ALL_GTK_MAIN_LEVELS

	return(status);
}

/*
 *	Popup List "motion_notify_event" signal callback.
 */
static gint PUListMotionNotifyEventCB(
	GtkWidget *widget, GdkEventMotion *motion, gpointer data
)
{
	gint status = FALSE;
	PopupList *list = POPUP_LIST(data);
	if((widget == NULL) || (motion == NULL) || (list == NULL))
	    return(status);

	if(list->freeze_count > 0)
	    return(status);

	/* See which widget this event is for */
	if(widget == list->clist)
	{
	    GtkCList *clist = GTK_CLIST(widget);

	    /* Get the widget that this event occured over */
	    GtkWidget *w = gtk_get_event_widget((GdkEvent *)motion);

	    if(widget == w)
	    {
		/* Send a "button_press_event" signal to the
		 * GtkCList if the event was not sent yet and
		 * button 1 is held
		 *
		 * This is so that the GtkCList catches the pointer
		 * when it enters it
		 */
		if(!(list->flags & POPUP_LIST_BUTTON_PRESS_SENT))
		{
		    gint x, y;
		    GdkModifierType mask;
		    GdkWindow *window = clist->clist_window;
		    GdkEvent ev;
		    GdkEventButton *button = (GdkEventButton *)&ev;

		    gdk_window_get_pointer(window, &x, &y, &mask);

		    button->type = GDK_BUTTON_PRESS;
		    button->window = window;
		    button->send_event = TRUE;
		    button->time = motion->time;
		    button->x = motion->x;
		    button->y = motion->y;
		    button->pressure = 1.0;
		    button->xtilt = 0.0;
		    button->ytilt = 0.0;
		    button->button = 1;
		    button->state = mask;
		    button->source = 0;
		    button->deviceid = 0;
		    button->x_root = 0;
		    button->y_root = 0;

		    if(mask & GDK_BUTTON1_MASK)
			gtk_widget_event(w, &ev);

		    status = TRUE;
		}
	    }
	    else
	    {
		/* Moved over one of the other GtkWidgets belonging
		 * to the Popup List?
		 */
		if((w == list->vscrollbar) ||
		   (w == list->hscrollbar) ||
		   (w == list->scrolled_window) ||
		   (w == list->toplevel)
		)
		{
		    /* Forward event to that GtkWidget */
		    gtk_widget_event(w, (GdkEvent *)motion);
		}
	    }
	}

	return(status);
}

/*
 *	Popup List "button_press_event" or "button_release_event"
 *	signal callback.
 */
static gint PUListCrossingEventCB(
	GtkWidget *widget, GdkEventCrossing *crossing, gpointer data
)
{
	gint status = FALSE;
	PopupList *list = POPUP_LIST(data);
	if((widget == NULL) || (crossing == NULL) || (list == NULL))
	    return(status);

	if(list->freeze_count > 0)
	    return(status);

	/* See which widget this event is for */
	if(widget == list->clist)
	{
	    /* Get the widget that this event occured over */
	    GtkWidget *w = gtk_get_event_widget((GdkEvent *)crossing);

	    switch((gint)crossing->type)
	    {
	      case GDK_ENTER_NOTIFY:
		/* Did this event occure outside the clist? */
		if(w != widget)
		{
		    /* Clicked on one of the other GtkWidgets
		     * belonging to the Popup List?
		     */
		    if((w == list->vscrollbar) ||
		       (w == list->hscrollbar) ||
		       (w == list->scrolled_window) ||
		       (w == list->toplevel)
		    )
		    {
			/* Forward event to that widget */
			gtk_widget_event(w, (GdkEvent *)crossing);
		    }
		}
		status = TRUE;
		break;

	      case GDK_LEAVE_NOTIFY:
		/* Did this event occure outside the clist? */
		if(w != widget)
		{
		    /* Clicked on one of the other GtkWidgets
		     * belonging to the Popup List?
		     */
		    if((w == list->vscrollbar) ||
		       (w == list->hscrollbar) ||
		       (w == list->scrolled_window) ||
		       (w == list->toplevel)
		    )
		    {
			/* Forward event to that widget */
			gtk_widget_event(w, (GdkEvent *)crossing);
		    }
		}
		status = TRUE;
		break;
	    }
	}

	return(status);
}


/*
 *	Popup List's map idle callback.
 *
 *	This is called each time right after the popup list is
 *	mapped.
 */
static gint PUListMapIdleCB(gpointer data)
{
	gint i;
	GtkCList *clist;
	PopupList *list = POPUP_LIST(data);
	if(list == NULL)
	    return(FALSE);

	if(list->freeze_count > 0)
	    return(FALSE);

	clist = GTK_CLIST(list->clist);
	if(clist == NULL)
	    return(FALSE);

	/* Automatically resize the columns */
	gtk_clist_columns_autosize(clist);

	/* Scroll to the selected item */
	i = PUListGetSelectedLast(list->toplevel);
	if(i > -1)
	{
	    const gint row = i;
	    if(gtk_clist_row_is_visible(clist, row) !=
		GTK_VISIBILITY_FULL
	    )
		gtk_clist_moveto(
		    clist,
		    row, -1,
		    0.5f, 0.0f
		);
	}

	return(FALSE);
}

/*
 *	Popup List GtkCList "realize" signal callback.
 */
static void PUListRealizeCB(GtkWidget *widget, gpointer data)
{
	PopupList *list = POPUP_LIST(data);
	if(list == NULL)
	    return;

	/* Mark that the Popup List has been realized */
	list->flags |= POPUP_LIST_REALIZED;
}


/*
 *	Popup List's Shadow GtkWindow "expose_event" signal callback.
 */
static gint PUListShadowExposeEventCB(
	GtkWidget *widget, GdkEventExpose *expose, gpointer data
)
{
	PopupList *list = POPUP_LIST(data);
	if((widget == NULL) || (list == NULL))
	    return(FALSE);

	PUListShadowDraw(list);

	return(TRUE);
}

/*
 *	Popup List's Shadow GtkWindow "draw" signal callback.
 */
static void PUListShadowDrawCB(
	GtkWidget *widget, GdkRectangle *area, gpointer data
)
{
	PopupList *list = POPUP_LIST(data);
	if((widget == NULL) || (list == NULL))
	    return;

	PUListShadowDraw(list);
}

/*
 *	Draws the Popup List's Shadow.
 */
static void PUListShadowDraw(PopupList *list)
{
	gint width, height;
	GdkPixmap *pixmap;
	GdkWindow *window;
	GtkStyle *style;
	GtkWidget *w = list->shadow;
	if(w == NULL)
	    return;

	window = w->window;
	pixmap = list->shadow_pm;
	style = gtk_widget_get_style(w);
	if((window == NULL) || (pixmap == NULL) || (style == NULL))
	    return;

	gdk_window_get_size(
	    (GdkWindow *)pixmap,
	    &width, &height
	);
	if((width <= 0) || (height <= 0))
	    return;

	gdk_draw_pixmap(
	    (GdkDrawable *)window,
	    style->white_gc,
	    (GdkDrawable *)pixmap,
	    0, 0,
	    0, 0,
	    width, height
	);
}


/*
 *	Sets up the Popup List's GtkCList for the start of the drag.
 */
static void PUListCListDragSetUp(PopupList *list)
{
	/* Use the GtkCList as the grab GtkWidget */
	GtkWidget *w = list->clist;
	if(w != NULL)
	{
	    /* Set up the grab GtkWidget */
	    gtk_widget_grab_focus(w);
	    gtk_widget_grab_default(w);
	    if(w != gtk_grab_get_current())
		gtk_grab_add(w);
	}
}

/*
 *	Removes all grabs from the Popup List's GtkCList, marking the
 *	end of the drag.
 */
static void PUListCListDragCleanUp(PopupList *list)
{
	/* Use the GtkCList as the grab widget */
	GtkWidget *w = list->clist;
	if(w != NULL)
	{
	    /* Remove the grab */
	    gtk_grab_remove(w);
	}
}


/*
 *	Map Button GtkDrawingArea "expose" event signal callback.
 */
static gint PUListMapButtonExposeCB(
	GtkWidget *widget, GdkEventExpose *expose, gpointer data
)
{
	gint status = FALSE;
	gint y, y_inc, width, height;
	GdkWindow *window;
	GdkDrawable *drawable;
	GdkGC *gc;
	GtkStateType state;
	GtkStyle *style;
	GtkWidget *button;

	if(widget == NULL)
	    return(status);

	/* Get the parent of the GtkDrawingArea widget, which should
	 * be the GtkButton
	 */
	button = widget->parent;
	if(button == NULL)
	    return(status);

	window = widget->window;
	state = GTK_WIDGET_STATE(widget);
	style = gtk_widget_get_style(button);
	if((window == NULL) || (style == NULL))
	    return(status);

	drawable = (GdkDrawable *)window;

	gdk_window_get_size(
	    (GdkWindow *)drawable,
	    &width, &height
	);
	if((width <= 0) || (height <= 0))
	    return(status);

	/* Begin drawing */

	/* Background */
	gtk_style_apply_default_background(
	    style,
	    drawable,
	    FALSE,
	    state,
	    NULL,
	    0, 0,
	    width, height
	);

	/* Details */
	y_inc = 5;
	for(y = (gint)(height * 0.5f) - 1;
	    y >= 0;
	    y -= y_inc
	)
	{
	    gc = style->light_gc[state];
	    gdk_draw_line(
		drawable,
		gc,
		0, y + 0,
		width, y + 0
	    );
	    gc = style->dark_gc[state];
	    gdk_draw_line(
		drawable,
		gc,
		0, y + 1,
		width, y + 1
	    );
#if 0
	    gc = style->black_gc;
	    gdk_draw_line(
		drawable,
		gc,
		0, y + 1,
		width, y + 1
	    );
#endif
	}
	for(y = (gint)(height * 0.5) - 1 + y_inc;
	    y < height;
	    y += y_inc
	)
	{
	    gc = style->light_gc[state];
	    gdk_draw_line(
		drawable,
		gc,
		0, y + 0,
		width, y + 0
	    );
	    gc = style->dark_gc[state];
	    gdk_draw_line(
		drawable,
		gc,
		0, y + 1,
		width, y + 1
	    );
#if 0
	    gc = style->black_gc;
	    gdk_draw_line(
		drawable,
		gc,
		0, y + 1,
		width, y + 1
	    );
#endif
	}

	/* Send the drawable to the window if it is not the window */
	if(drawable != (GdkDrawable *)window)
	    gdk_draw_pixmap(
		(GdkDrawable *)window,
		style->white_gc,
		drawable,
		0, 0,
		0, 0,
		width, height
	    );

	status = TRUE;

	return(status);
}


/*
 *	Popup List Box "destroy" signal callback.
 */
static void PUListBoxDestroyCB(gpointer data)
{
	PopupListBox *box = POPUP_LIST_BOX(data);
	if(box == NULL)
	    return;

	box->freeze_count++;

	/* Unref the Popup List */
	box->popup_list = PUListUnref(box->popup_list);

	box->freeze_count--;

	g_free(box);
}

/*
 *	Popup List Box GtkDrawingArea event signal callback.
 */
static gint PUListBoxEventCB(
	GtkWidget *widget, GdkEvent *event, gpointer data
)
{
	gint status = FALSE;
	gboolean press;
	GdkEventFocus *focus;
	GdkEventKey *key;
	GdkEventButton *button;
	GtkWidget *list;
	GtkCList *clist;
	PopupListBox *box = POPUP_LIST_BOX(data);
	if((widget == NULL) || (event == NULL) || (box == NULL))
	    return(status);

	if(box->freeze_count > 0)
	    return(status);

	switch((gint)event->type)
	{
	  case GDK_EXPOSE:
	    PUListBoxDraw(box);
	    status = TRUE;
	    break;

	  case GDK_FOCUS_CHANGE:
	    focus = (GdkEventFocus *)event;
	    if(focus->in && !GTK_WIDGET_HAS_FOCUS(widget))
	    {
		GTK_WIDGET_SET_FLAGS(widget, GTK_HAS_FOCUS);
		gtk_widget_queue_draw(widget);
		status = TRUE;
	    }
	    else if(!focus->in && GTK_WIDGET_HAS_FOCUS(widget))
	    {
		GTK_WIDGET_UNSET_FLAGS(widget, GTK_HAS_FOCUS);
		gtk_widget_queue_draw(widget);
		status = TRUE;
	    }
	    break;

	  case GDK_KEY_PRESS:
	  case GDK_KEY_RELEASE:
	    key = (GdkEventKey *)event;
	    press = (key->type == GDK_KEY_PRESS) ? TRUE : FALSE;
#define STOP_KEY_SIGNAL_EMIT(_w_)	{	\
if((_w_) != NULL)				\
 gtk_signal_emit_stop_by_name(			\
  GTK_OBJECT(_w_),				\
  press ?					\
   "key_press_event" : "key_release_event"	\
 );						\
}
	    switch(key->keyval)
	    {
	      case GDK_Return:
	      case GDK_KP_Enter:
	      case GDK_space:
		if(press)
		    PUListBoxMapCB(box->map_btn, box);
		STOP_KEY_SIGNAL_EMIT(widget);
		status = TRUE; 
		break;

	      case GDK_Up:
	      case GDK_KP_Up:
		if(press)
		{
		    list = PUListBoxGetPUList(box->toplevel);
		    clist = (GtkCList *)PUListGetCList(list);
		    if(clist != NULL)
		    {
			GList *glist = clist->selection_end;
			gint row = (glist != NULL) ? (gint)glist->data : -1;
			if(row < 0)
			    row = 0;
			else 
			    row--;
			if((row >= 0) && (row < clist->rows))
			{
			    gtk_clist_unselect_all(clist);
			    gtk_clist_select_row(clist, row, 0);

			    gtk_widget_queue_draw(widget);

			    /* Report new value */
			    if(box->changed_cb != NULL)
				box->changed_cb(
				    box->toplevel,	/* Popup List Box */
				    row,		/* Item */
				    box->changed_data	/* Data */
				);
			}
		    }
		}
		STOP_KEY_SIGNAL_EMIT(widget);
		status = TRUE;
		break;

	      case GDK_Down:
	      case GDK_KP_Down:
		if(press)
		{
		    list = PUListBoxGetPUList(box->toplevel);
		    clist = (GtkCList *)PUListGetCList(list);
		    if(clist != NULL)
		    {
			GList *glist = clist->selection_end;
			gint row = (glist != NULL) ? (gint)glist->data : -1;
			if(row < 0)
			    row = 0;
			else
			    row++;
			if((row >= 0) && (row < clist->rows))
			{
			    gtk_clist_unselect_all(clist);
			    gtk_clist_select_row(clist, row, 0);

			    gtk_widget_queue_draw(widget);

			    /* Report new value */
			    if(box->changed_cb != NULL)
				box->changed_cb(
				    box->toplevel,	/* Popup List Box */
				    row,		/* Item */
				    box->changed_data	/* Data */
				);
			}
		    }
		}
		STOP_KEY_SIGNAL_EMIT(widget);
		status = TRUE;
		break;

	      case GDK_Page_Up:
	      case GDK_KP_Page_Up:
		if(press)
		{
		    list = PUListBoxGetPUList(box->toplevel);
		    clist = (GtkCList *)PUListGetCList(list);
		    if(clist != NULL)
		    {
			GList *glist = clist->selection_end;
			const gint sel_row = (glist != NULL) ? (gint)glist->data : -1;
			gint row = sel_row;
			if(row < 0)
			    row = 0;
			else
			    row -= MAX((box->nlines_visible / 2), 1);
			if(row < 0)
			    row = 0;
			if((row >= 0) && (row < clist->rows) &&
			   (row != sel_row)
			)
			{
			    gtk_clist_unselect_all(clist);
			    gtk_clist_select_row(clist, row, 0);

			    gtk_widget_queue_draw(widget);

			    /* Report new value */
			    if(box->changed_cb != NULL)
				box->changed_cb(
				    box->toplevel,	/* Popup List Box */
				    row,		/* Item */
				    box->changed_data	/* Data */
				);
			}
		    }
		}
		STOP_KEY_SIGNAL_EMIT(widget);
		status = TRUE;
		break;

	      case GDK_Page_Down:
	      case GDK_KP_Page_Down:
		if(press)
		{
		    list = PUListBoxGetPUList(box->toplevel);
		    clist = (GtkCList *)PUListGetCList(list);
		    if(clist != NULL)
		    {
			GList *glist = clist->selection_end;
			const gint sel_row = (glist != NULL) ? (gint)glist->data : -1;
			gint row = sel_row;
			if(row < 0)
			    row = 0;
			else
			    row += MAX((box->nlines_visible / 2), 1);
			if(row >= clist->rows)
			    row = clist->rows - 1;
			if((row >= 0) && (row < clist->rows) &&
			   (row != sel_row)
			)
			{
			    gtk_clist_unselect_all(clist);
			    gtk_clist_select_row(clist, row, 0);

			    gtk_widget_queue_draw(widget);

			    /* Report new value */
			    if(box->changed_cb != NULL)
				box->changed_cb(
				    box->toplevel,	/* Popup List Box */
				    row,		/* Item */
				    box->changed_data	/* Data */
				);
			}
		    }
		}
		STOP_KEY_SIGNAL_EMIT(widget);
		status = TRUE;
		break;

	      case GDK_Home:
	      case GDK_KP_Home:
		if(press)
		{
		    list = PUListBoxGetPUList(box->toplevel);
		    clist = (GtkCList *)PUListGetCList(list);
		    if(clist != NULL)
		    {
			GList *glist = clist->selection_end;
			const gint sel_row = (glist != NULL) ? (gint)glist->data : -1;
			gint row = 0;
			if((row >= 0) && (row < clist->rows) &&
			   (row != sel_row)
			)
			{
			    gtk_clist_unselect_all(clist);
			    gtk_clist_select_row(clist, row, 0);

			    gtk_widget_queue_draw(widget);

			    /* Report new value */
			    if(box->changed_cb != NULL)
				box->changed_cb(
				    box->toplevel,	/* Popup List Box */
				    row,		/* Item */
				    box->changed_data	/* Data */
				);
			}
		    }
		}
		STOP_KEY_SIGNAL_EMIT(widget);
		status = TRUE;
		break;

	      case GDK_End:
	      case GDK_KP_End:
		if(press)
		{
		    list = PUListBoxGetPUList(box->toplevel);
		    clist = (GtkCList *)PUListGetCList(list);
		    if(clist != NULL)
		    {
			GList *glist = clist->selection_end;
			const gint sel_row = (glist != NULL) ? (gint)glist->data : -1;
			gint row = clist->rows - 1;
			if(row < 0)
			    row = 0;
			if((row >= 0) && (row < clist->rows) &&
			   (row != sel_row)
			)
			{
			    gtk_clist_unselect_all(clist);
			    gtk_clist_select_row(clist, row, 0);

			    gtk_widget_queue_draw(widget);

			    /* Report new value */
			    if(box->changed_cb != NULL)
				box->changed_cb(
				    box->toplevel,	/* Popup List Box */
				    row,		/* Item */
				    box->changed_data	/* Data */
				);
			}
		    }
		}
		STOP_KEY_SIGNAL_EMIT(widget);
		status = TRUE;
		break;
	    }
	    break;

	  case GDK_BUTTON_PRESS:
	    button = (GdkEventButton *)event;
	    if(!GTK_WIDGET_HAS_FOCUS(widget))
		gtk_widget_grab_focus(widget);
	    switch(button->button)
	    {
	      case GDK_BUTTON1:
		PUListBoxMapCB(box->map_btn, box);
		break;

	      case GDK_BUTTON4:
		list = PUListBoxGetPUList(box->toplevel);
		if(list != NULL)
		{
		    const gint	i = PUListGetSelectedLast(list),
				n = PUListGetTotalItems(list);
		    GtkCList *clist = (GtkCList *)PUListGetCList(list);
		    if((i > 0) && (n > 0))
		    {
			const gint row = i - 1;

			gtk_clist_unselect_all(clist);
			gtk_clist_select_row(clist, row, 0);

			gtk_widget_queue_draw(widget);

			if(box->changed_cb != NULL)
			    box->changed_cb(
				box->toplevel,		/* Popup List Box */
				row,			/* Item */
				box->changed_data	/* Data */
			    );
		    }
		}
		break;

	      case GDK_BUTTON5:
		list = PUListBoxGetPUList(box->toplevel);
		if(list != NULL)
		{
		    const gint	i = PUListGetSelectedLast(list),
				n = PUListGetTotalItems(list);
		    GtkCList *clist = (GtkCList *)PUListGetCList(list);
		    if((i < (n - 1)) && (n > 0))
		    {
			const gint row = i + 1;

			gtk_clist_unselect_all(clist);
			gtk_clist_select_row(clist, row, 0);

			gtk_widget_queue_draw(widget);

			if(box->changed_cb != NULL)
			    box->changed_cb(
				box->toplevel,		/* Popup List Box */
				row,			/* Item */
				box->changed_data	/* Data */
			    );
		    }
		}
		break;
	    }
	    status = TRUE;
	    break;
	}

	return(status);
}

/*
 *	Popup List Box map callback.
 */
static void PUListBoxMapCB(GtkWidget *widget, gpointer data)
{
	gint prev_i;
	const gchar *v, *prev_v;
	gchar *dprev_v;
	GtkWidget *list;
	PopupListBox *box = POPUP_LIST_BOX(data);
	if((widget == NULL) || (box == NULL))
	    return;

	if(box->freeze_count > 0)
	    return;

	list = PUListBoxGetPUList(box->toplevel);
	if(list == NULL)
	    return;

	prev_i = PUListGetSelectedLast(list);
	if(prev_i < 0)
	{
	    prev_i = 0;
	    PUListUnselectAll(list);
	    PUListSelect(list, prev_i);
	}

	PUListGetItemText(list, prev_i, &prev_v);
	if(prev_v != NULL)
	    dprev_v = STRDUP(prev_v);
	else
	    dprev_v = NULL;

	/* Map the Popup List and query the user */
	v = PUListMapQuery(
	    list,
	    dprev_v,
	    box->nlines_visible,
	    PULIST_RELATIVE_BELOW,
	    box->box_frame,			/* Relative GtkWidget */
	    widget				/* Map GtkWidget */
	);

	g_free(dprev_v);

	/* Got new value? */
	if(v != NULL)
	{
	    const gint i = PUListFindItemFromValue(list, v);

	    gtk_widget_grab_focus(box->box_da);
	    gtk_widget_queue_draw(box->box_da);

	    /* Report new value */
	    if((box->changed_cb != NULL) && (i != prev_i))
		box->changed_cb(
		    box->toplevel,		/* Popup List Box */
		    i,				/* Item */
		    box->changed_data		/* Data */
		);
	}
}

/*
 *	Redraws the Popup List Box.
 */
static void PUListBoxDraw(PopupListBox *box)
{
	gboolean has_focus;
	const gint	frame_border = 2;
	gint		width, height,
			font_height;
	GdkFont *font;
	GdkWindow *window;
	GdkDrawable *drawable;
	GtkStateType state;
	GtkStyle *style;
	GtkWidget *w;
	PopupList *list;

	if(box == NULL)
	    return;

	w = box->box_da;
	if(w == NULL)
	    return;

	list = POPUP_LIST(GTK_OBJECT_GET_DATA(
	    box->popup_list,
	    POPUP_LIST_KEY
	));
	if(list == NULL)
	    return;

	has_focus = GTK_WIDGET_HAS_FOCUS(w) ||
	    GTK_WIDGET_HAS_FOCUS(box->map_btn);
	window = w->window;
	state = GTK_WIDGET_STATE(w);
	style = gtk_widget_get_style(w);
	if((window == NULL) || (style == NULL))
	    return;

	gdk_window_get_size(window, &width, &height);
	font = style->font;
	if((font == NULL) || (width <= 0) || (height <= 0))
	    return;
		   
	font_height = font->ascent + font->descent;

	drawable = (GdkDrawable *)window;

	/* Draw the background */
	gdk_draw_rectangle(
	    drawable,
	    has_focus ?
		style->bg_gc[GTK_STATE_SELECTED] :
		style->base_gc[state],
	    TRUE,
	    0, 0, width, height
	);

	/* Draw the value */
	if(list->clist != NULL)
	{
	    /* Get selected row or first row */
	    GdkGC *gc = has_focus ?
		style->text_gc[GTK_STATE_SELECTED] :
		style->text_gc[state];
	    GtkCList *clist = GTK_CLIST(list->clist);
	    GList *glist = clist->selection_end;
	    gint row = (glist != NULL) ? (gint)glist->data : 0;

	    if((row >= 0) && (row < clist->rows))
	    {
		gint x = frame_border, y;
		gchar *text = NULL;
		guint8 spacing = 0;
		GdkPixmap *pixmap = NULL;
		GdkBitmap *mask = NULL;

		/* Get icon and text */
		switch(gtk_clist_get_cell_type(clist, row, 0))
		{
		  case GTK_CELL_TEXT:
		    gtk_clist_get_text(clist, row, 0, &text);
		    break;
		  case GTK_CELL_PIXTEXT:
		    gtk_clist_get_pixtext(
			clist, row, 0, &text,
			&spacing, &pixmap, &mask
		    );
		    break;
		  case GTK_CELL_PIXMAP:
		    gtk_clist_get_pixmap(
			clist, row, 0, &pixmap, &mask
		    );
		    break;
		  case GTK_CELL_WIDGET:
		  case GTK_CELL_EMPTY:
		    break;
		}
		/* Draw the icon? */
		if(pixmap != NULL)
		{
		    gint pm_width, pm_height;
		    gdk_window_get_size(pixmap, &pm_width, &pm_height);
		    y = (height - pm_height) / 2;
		    gdk_gc_set_clip_mask(gc, mask);
		    gdk_gc_set_clip_origin(gc, x, y);
		    gdk_draw_pixmap(
			drawable, gc, pixmap,
			0, 0, x, y, pm_width, pm_height
		    );
		    gdk_gc_set_clip_mask(gc, NULL);
		    x += pm_width + (gint)spacing;
		}
		/* Draw the text? */
		if(text != NULL)
		{
		    GdkTextBounds b;
		    gdk_string_bounds(font, text, &b);
		    gdk_draw_string(
			drawable, font, gc,
			x - b.lbearing,
			((height - font_height) / 2) +
			    font->ascent,
			text
		    );
		}
	    }
	}

	/* Draw focus if widget is focused */
	if(has_focus && GTK_WIDGET_SENSITIVE(w))
	{
	    GdkGCValues gcv;
	    GdkGC *gc = style->fg_gc[state];
	    gdk_gc_get_values(gc, &gcv);
	    gdk_gc_set_function(gc, GDK_INVERT);
	    gdk_draw_rectangle(
		drawable, gc, FALSE,
		0, 0, width - 1, height - 1
	    );
	    gdk_gc_set_function(gc, gcv.function);
	}

	/* Send drawable to window if drawable is not the window */
	if(drawable != window)
	    gdk_draw_pixmap(
		window, style->fg_gc[state], drawable,
		0, 0, 0, 0, width, height
	    );
}

/*
 *	Gets the Popup List item by text.
 *
 *	The w specifies the Popup List.
 *
 *	The value specifies the text.
 *
 *	Returns the matched item's index or negative on error.
 */
gint PUListFindItemFromValue(
	GtkWidget *w,
	const gchar *value
)
{
	gint i;
	gchar *text;
	guint8 spacing;
	GdkPixmap *pixmap;
	GdkBitmap *mask;
	GtkCList *clist = (GtkCList *)PUListGetCList(w);
	if((clist == NULL) || (value == NULL))
	    return(-1);

	for(i = 0; i < clist->rows; i++)
	{
	    text = NULL;
	    switch(gtk_clist_get_cell_type(clist, i, 0))
	    {
	      case GTK_CELL_TEXT:
		gtk_clist_get_text(
		    clist,
		    i, 0,
		    &text
		);
		break;
	      case GTK_CELL_PIXTEXT:
		gtk_clist_get_pixtext(
		    clist,
		    i, 0,
		    &text,
		    &spacing,
		    &pixmap, &mask
		);
		break;
	      case GTK_CELL_PIXMAP:
	      case GTK_CELL_WIDGET:
	      case GTK_CELL_EMPTY:
		break;
	    }
	    if(text != NULL)
	    {
		if(text == value)
		    return(i);

		if(!strcmp((const char *)text, (const char *)value))
		    return(i);
	    }
	}
	 
	return(-1);
}

/*
 *	Gets the Popup List item's data by text.
 *
 *	The w specifies the Popup List.
 *
 *	The value specifies the text to match.
 *
 *	Returns the matched item's data or NULL on error.
 */
gpointer PUListGetDataFromValue(
	GtkWidget *w,
	const gchar *value
)
{
	gint i;
	GtkCList *clist = (GtkCList *)PUListGetCList(w);
	if((clist == NULL) || (value == NULL))
	    return(NULL);

	i = PUListFindItemFromValue(w, value);
	if(i < 0)
	    return(NULL);

	return(gtk_clist_get_row_data(clist, i));
}


/*
 *	Adds a new item to Popup List with the specified text.
 *
 *	The w specifies the Popup List.
 *
 *	The value specifies the item's text.
 *
 *	Returns the new item's index or negative on error.
 */
gint PUListAddItem(
	GtkWidget *w,
	const gchar *value
)
{
	return(PUListAddItemPixText(
	    w,
	    value,
	    NULL, NULL
	));
}

/*
 *	Adds a new item to Popup List with the specified icon & text.
 *
 *	The w specifies the Popup List.
 *
 *	The value specifies the item's text.
 *
 *	The pixmap and mask specifies the item's icon.
 *
 *	Returns the new item's index or negative on error.
 */
gint PUListAddItemPixText(
	GtkWidget *w,
	const gchar *value,
	GdkPixmap *pixmap, GdkBitmap *mask
)
{
	gint i;
	gchar **strv;
	GtkCList *clist = (GtkCList *)PUListGetCList(w);
	if((clist != NULL) ? (clist->columns <= 0) : TRUE)
	    return(-1);

	/* Allocate cell values for new row */
	strv = (gchar **)g_malloc(
	    clist->columns * sizeof(gchar *)
	);
	if(strv == NULL)
	    return(-1);
	for(i = 0; i < clist->columns; i++)
	    strv[i] = "";

	/* Append a new row */
	i = gtk_clist_append(clist, strv);

	/* Delete the row cell values */
	g_free(strv);

	if(i < 0)
	    return(-1);

	PUListSetItemPixText(
	    w,
	    i,
	    value,
	    pixmap, mask
	);

	return(i);
}

/*
 *	Deletes all the items in the Popup List.
 *
 *	The w specifies the Popup List.
 */
void PUListClear(GtkWidget *w)
{
	GtkCList *clist = (GtkCList *)PUListGetCList(w);
	if(clist == NULL)
	    return;

	gtk_clist_freeze(clist);
	gtk_clist_clear(clist);
	gtk_clist_thaw(clist);
}


/*
 *	Sets the item's text.
 *
 *	The w specifies the Popup List.
 *
 *	The i specifies the item.
 *
 *	The value specifies the item's text.
 */
void PUListSetItemText(
	GtkWidget *w,
	const gint i,
	const gchar *value
)
{
	GtkCList *clist = (GtkCList *)PUListGetCList(w);
	if(clist == NULL)
	    return;

	if((i < 0) || (i >= clist->rows))
	    return;

	gtk_clist_set_text(
	    clist,
	    i, 0,
	    (value != NULL) ? value : ""
	);
}

/*
 *	Sets the item's icon & text.
 *
 *	The w specifies the Popup List.
 *
 *	The i specifies the item.
 *
 *	The value specifies the item's text.
 *
 *	The pixmap and mask specifies the item's icon.
 */
void PUListSetItemPixText(
	GtkWidget *w,
	const gint i,
	const gchar *value,
	GdkPixmap *pixmap, GdkBitmap *mask
)
{
	GtkCList *clist = (GtkCList *)PUListGetCList(w);
	if(clist == NULL)
	    return;

	if((i < 0) || (i >= clist->rows))
	    return;

	if(pixmap != NULL)
	    gtk_clist_set_pixtext(
		clist,
		i, 0,
		(value != NULL) ? value : "",
		2,
		pixmap, mask
	    );
	else
	    gtk_clist_set_text(
		clist,
		i, 0,
		(value != NULL) ? value : ""  
	    );
}

/*
 *	Sets the item sensitive or insensitive.
 *
 *	The w specifies the Popup List.
 *
 *	The i specifies the item.
 *
 *	The sensitive specifies the sensitivity.
 */
void PUListSetItemSensitive(
	GtkWidget *w,
	const gint i,
	const gboolean sensitive
)
{
	GtkCList *clist = (GtkCList *)PUListGetCList(w);
	if(clist == NULL)
	    return;

	if((i < 0) || (i >= clist->rows))
	    return;

	gtk_clist_set_selectable(
	    clist,
	    i,
	    sensitive
	);
}

/*
 *	Sets the item's data.
 *
 *	The w specifies the Popup List.
 *
 *	The i specifies the item.
 *
 *	The data specifies the item's data.
 */
void PUListSetItemData(
	GtkWidget *w,
	const gint i,
	gpointer data
)
{
	GtkCList *clist = (GtkCList *)PUListGetCList(w);
	if(clist == NULL)
	    return;

	if((i < 0) || (i >= clist->rows))
	    return;

	gtk_clist_set_row_data(
	    clist,
	    i,
	    data
	);
}

/*
 *	Sets the item's data & destroy callback.
 *
 *	The w specifies the Popup List.
 *
 *	The i specifies the item.
 *
 *	The data specifies the item's data.
 *
 *	The destroy_cb specifies the item data destroy callback. If
 *	destroy_cb is NULL then no item data destroy callback will
 *	be set.
 */
void PUListSetItemDataFull(
	GtkWidget *w,
	const gint i,
	gpointer data,
	GtkDestroyNotify destroy_cb
)
{
	GtkCList *clist = (GtkCList *)PUListGetCList(w);
	if(clist == NULL)
	    return;

	if((i < 0) || (i >= clist->rows))
	    return;

	if(destroy_cb != NULL)
	    gtk_clist_set_row_data_full(
		clist,
		i,
		data, destroy_cb
	    );
	else
	    gtk_clist_set_row_data(
		clist,
		i,
		data
	    );
}


/*
 *	Gets the total number of items.
 *
 *	The w specifies the Popup List.
 */
gint PUListGetTotalItems(GtkWidget *w)
{
	GtkCList *clist = (GtkCList *)PUListGetCList(w);
	if(clist == NULL)
	    return(0);

	return(clist->rows);
}

/*
 *	Gets the item's text.
 *
 *	The w specifies the Popup List.
 *
 *	The i specifies the item.
 *
 *	The value_rtn specifies the text return.
 */
void PUListGetItemText(
	GtkWidget *w,
	const gint i,
	const gchar **value_rtn
)
{
	PUListGetItemPixText(
	    w,
	    i,
	    value_rtn,
	    NULL, NULL
	);
}

/*
 *	Gets the item's icon & text.
 *
 *	The w specifies the Popup List.
 *
 *	The i specifies the item.
 *
 *	The value_rtn specifies the text return.
 *
 *	The pixmap_rtn and mask_rtn specifies the icon return.
 */
void PUListGetItemPixText(
	GtkWidget *w,
	const gint i,
	const gchar **value_rtn,
	GdkPixmap **pixmap_rtn, GdkBitmap **mask_rtn
)
{
	gchar *text = NULL;
	guint8 spacing = 0;
	GdkPixmap *pixmap = NULL;
	GdkBitmap *mask = NULL;
	GtkCList *clist;

	if(value_rtn != NULL)
	    *value_rtn = NULL;
	if(pixmap_rtn != NULL)
	    *pixmap_rtn = NULL;
	if(mask_rtn != NULL)
	    *mask_rtn = NULL;

	clist = (GtkCList *)PUListGetCList(w);
	if(clist == NULL)
	    return;

	if((i < 0) || (i >= clist->rows))
	    return;

	switch(gtk_clist_get_cell_type(clist, i, 0))
	{
	  case GTK_CELL_TEXT:
	    gtk_clist_get_text(
		clist,
		i, 0,
		&text
	    );
	    break;
	  case GTK_CELL_PIXTEXT:
	    gtk_clist_get_pixtext(
		clist,
		i, 0,
		&text,
		&spacing,
		&pixmap, &mask
	    );
	    break;
	  case GTK_CELL_PIXMAP:
	    gtk_clist_get_pixmap(
		clist,
		i, 0,
		&pixmap, &mask
	    );
	    break;
	  case GTK_CELL_WIDGET:
	  case GTK_CELL_EMPTY:
	    break;
	}

	if(value_rtn != NULL)
	    *value_rtn = text;
	if(pixmap_rtn != NULL)
	    *pixmap_rtn = pixmap;
	if(mask_rtn != NULL)
	    *mask_rtn = mask;
}

/*
 *	Gets the item's sensitivity.
 *
 *	The w specifies the Popup List.
 *
 *	The i specifies the item.
 *
 *	Returns the item's sensitivity.
 */
gboolean PUListGetItemSensitive(
	GtkWidget *w,
	const gint i
)
{
	GtkCList *clist = (GtkCList *)PUListGetCList(w);
	if(clist == NULL)
	    return(FALSE);

	if((i < 0) || (i >= clist->rows))
	    return(FALSE);

	return(gtk_clist_get_selectable(clist, i));
}

/*
 *	Gets the item's data.
 *
 *	The w specifies the Popup List.
 *
 *	The i specifies the item.
 *
 *	Returns the item's data.
 */
gpointer PUListGetItemData(
	GtkWidget *w,
	const gint i
)
{
	GtkCList *clist = (GtkCList *)PUListGetCList(w);
	if(clist == NULL)
	    return(NULL);

	if((i < 0) || (i >= clist->rows))
	    return(NULL);

	return(gtk_clist_get_row_data(clist, i));
}


/*
 *	Gets the last selected item on the Popup List.
 *
 *	The w specifies the Popup List.
 *
 *	Returns the item's index or negative on error.
 */
gint PUListGetSelectedLast(GtkWidget *w)
{
	GList *glist;
	GtkCList *clist = (GtkCList *)PUListGetCList(w);
	if(clist == NULL)
	    return(-1);

	glist = clist->selection_end;
	if(glist == NULL)
	    return(-1);

	return((gint)glist->data);
}

/*
 *	Selects the Popup List item.
 *
 *	The w specifies the Popup List.
 *
 *	The i specifies the item to selected.
 */
void PUListSelect(
	GtkWidget *w,
	const gint i
)
{
	GtkCList *clist = (GtkCList *)PUListGetCList(w);
	if(clist == NULL)
	    return;

	if((i < 0) || (i >= clist->rows))
	    return;

	gtk_clist_freeze(clist);
	gtk_clist_unselect_all(clist);
	gtk_clist_select_row(clist, i, 0);
	gtk_clist_thaw(clist);
}

/*
 *	Unselects all items in the Popup List.
 *
 *	The w specifies the Popup List.
 */
void PUListUnselectAll(GtkWidget *w)
{
	GtkCList *clist = (GtkCList *)PUListGetCList(w);
	if(clist == NULL)
	    return;

	gtk_clist_freeze(clist);
	gtk_clist_unselect_all(clist);
	gtk_clist_thaw(clist);
}


/*
 *	Checks if the Popup List is querying (if it is mapped).
 *
 *	The w specifies the Popup List.
 *
 *	Returns TRUE if the Popup List is in query or FALSE if it is
 *	not.
 */
gboolean PUListIsQuery(GtkWidget *w)
{
	PopupList *list = POPUP_LIST(GTK_OBJECT_GET_DATA(
	    w,
	    POPUP_LIST_KEY
	));
	if(list == NULL)
	    return(FALSE);

	return(GTK_WIDGET_MAPPED(list->toplevel));
}

/*
 *	Breaks the Popup List query and unmaps it.
 */
void PUListBreakQuery(GtkWidget *w)
{
	PopupList *list = POPUP_LIST(GTK_OBJECT_GET_DATA(
	    w,
	    POPUP_LIST_KEY
	));
	if(list == NULL)
	    return;

	while(list->main_level > 0)
	{
	    list->main_level--;
	    gtk_main_quit();
	}
}

/*
 *	Maps the Popup List and blocks until the user responds.
 *
 *	The w specifies the Popup List.
 *
 *	The value specifies the initial value, which may be NULL.
 *
 *	The nlines_visible specifies the number of lines visible or -1
 *	for default number of lines.
 *
 *	The relative specifies how to position the popup list relative
 *	to the rel_widget.
 *
 *	The rel_widget specifies the relative widget ot map the popup
 *	list to.
 *
 *	The map_widget specifies the widget that is to be consider the
 *	widget that trigged the mapping of the popup list. Ihe
 *	map_widget is typically a GtkButton widget.
 *
 *	Returns the selected value, the returned pointer must not be
 *	modified or deleted.
 *
 *	If the user clicks outside of the Popup List or the escape
 *	key is pressed then NULL will be returned.
 *
 *	The toplevel widget of rel_widget must be set to receive
 *	button press and button release events otherwise clicking
 *	elsewhere not on the popup list will not unmap the popup list.
 */
const gchar *PUListMapQuery(
	GtkWidget *w,
	const gchar *value,
	const gint nlines_visible,
	const pulist_relative relative,
	GtkWidget *rel_widget,
	GtkWidget *map_widget
)
{
	const gint	min_width = 20,
			min_height = (POPUP_LIST_ROW_SPACING + 1) + (2 * 5);
	gint		sel_item = -1,
			x = 0, y = 0,
			width = POPUP_LIST_DEF_WIDTH,
			height = POPUP_LIST_DEF_HEIGHT,
			root_width, root_height;
	GdkWindow *root;
	GtkWidget	*toplevel,
			*shadow;
	PopupList *list = POPUP_LIST(GTK_OBJECT_GET_DATA(
	    w,
	    POPUP_LIST_KEY
	));
	if(list == NULL)
	    return(NULL);

	toplevel = list->toplevel;
	if(toplevel == NULL)
	    return(NULL);

	/* Is the Popup List already mapped or waiting on a query? */
	if((list->flags & POPUP_LIST_MAPPED) || (list->main_level > 0))
	    return(NULL);

	/* Add a reference count */
	PUListRef(toplevel);

	list->freeze_count++;

	/* Get the parent of the Popup List's toplevel GdkWindow,
	 * which should be the desktop
	 */
	root = gdk_window_get_parent(toplevel->window);

	/* Get the Popup List's shadow GtkWindow */
	shadow = list->shadow;

	/* Clear the value */
	g_free(list->last_value);
	list->last_value = NULL;

	/* Clear the initial "button_press_event" sent flag */
	list->flags &= ~POPUP_LIST_BUTTON_PRESS_SENT;

	/* Record the map GtkWidget */
	list->map_widget = map_widget;

	/* Get the desktop's size */
	gdk_window_get_size(
	    root,
	    &root_width, &root_height
	);

	/* Calculate the Popup List toplevel GtkWindow's size
	 *
	 * If a relative GtkWidget is specified then calculate the
	 * width of the popup list based on the width of the
	 * relative GtkWidget + the map GtkWidget
	 */
	w = rel_widget;
	if(w != NULL)
	{
	    if((w != map_widget) && (map_widget != NULL))
		width = w->allocation.width + map_widget->allocation.width;
	    else
		width = w->allocation.width;
	}
	height = (((nlines_visible < 0) ?
	    POPUP_LIST_DEF_NLINES_VISIBLE : nlines_visible) *
	    (POPUP_LIST_ROW_SPACING + 1)
	) + (2 * 5);

	/* Make sure that the Popup List's size remains at or above
	 * the minimum size
	 */
	if(width < min_width)
	    width = min_width;
	if(height < min_height)
	    height = min_height;

	/* If a map GtkWidget is specified then we need to restore
	 * its GtkStateType before mapping the Popup List
	 */
	w = map_widget;
	if(w != NULL)
	{
	    /* Remove the grab from the map GtkWidget as needed */
	    gtk_grab_remove(w);

	    /* Handle any additional restoring of the GtkWidget
	     * GtkStateType and characteristics by its type
	     *
	     * GtkButton
	     */
	    if(GTK_IS_BUTTON(w))
	    {
		GtkButton *button = GTK_BUTTON(w);
		button->in_button = 1;
		button->button_down = 1;
	    }
	}

	/* If a value is specified then select the item in the
	 * PopupList's GtkCList that matches that value
	 */
	w = list->clist;
	if((w != NULL) && (value != NULL))
	{
	    sel_item = PUListFindItemFromValue(toplevel, value);
	    if(sel_item > -1)
	    {
		GtkCList *clist = GTK_CLIST(w);
		gtk_clist_unselect_all(clist);
		gtk_clist_select_row(clist, sel_item, 0);
		clist->focus_row = sel_item;
	    }
	}

	/* Calculate the Popup List toplevel GtkWindow's position
	 *
	 * Relative GtkWidget specified?
	 */
	w = rel_widget;
	if((w != NULL) ? (w->window != NULL) : FALSE)
	{
	    gint rel_x, rel_y;
	    GdkWindow *window = w->window;
	    GtkAllocation *alloc = &w->allocation;

	    /* Get the relative GtkWidget's position with respect
	     * to the desktop
	     */
	    gdk_window_get_root_position(
		window,
		&rel_x, &rel_y
	    );

	    /* If the relative GtkWidget has no GdkWindow then we
	     * need to account for its intended position relative to
	     * its parent GdkWindow
	     */
	    if(GTK_WIDGET_NO_WINDOW(w))
	    {
		rel_x += alloc->x;
		rel_y += alloc->y;
	    }

	    /* Calculate the toplevel GtkWindow's position */
	    switch(relative)
	    {
	      case PULIST_RELATIVE_CENTER:
		x = rel_x;
		y = rel_y - (height / 2) + (alloc->height / 2);
		break;
	      case PULIST_RELATIVE_UP:
		x = rel_x;
		y = rel_y - height + alloc->height;
		if(y < 0)
		    y = rel_y;
		break;
	      case PULIST_RELATIVE_DOWN:
		x = rel_x;
		y = rel_y;
		if(y > (root_height - height))
		    y = rel_y - height + alloc->height;
		break;
	      case PULIST_RELATIVE_ABOVE:
		x = rel_x;
		y = rel_y - height;
		if(y < 0)
		    y = rel_y + alloc->height;
		break;
	      case PULIST_RELATIVE_BELOW:
		x = rel_x;
		y = rel_y + alloc->height;
		if(y > (root_height - height))
		    y = rel_y - height;
		break;
	    }
	}
	else if(root != NULL)
	{
	    /* No relative GtkWidget not specified, so calculate the
	     * toplevel GtkWindow's position relative to the current
	     * position of the pointer
	     */
	    gint px = 0, py = 0;
	    GdkModifierType mask;

	    gdk_window_get_pointer(
		root,
		&px, &py,
		&mask
	    );
	    x = px - (width / 2);
	    y = py - (height / 2);
	}

	/* Clip the position */
	if(x > (root_width - width))
	    x = root_width - width;
	if(x < 0)
	    x = 0;
	if(y > (root_height - height))
	    y = root_height - height;
	if(y < 0)
	    y = 0;

	/* Recreate the shadow's GdkPixmap */
	if((list->shadow_style != PULIST_SHADOW_NONE) && (shadow != NULL))
	{
	    GtkWidget *w = shadow;
	    GdkWindow *window = w->window;
	    GdkPixmap *pixmap = gdk_pixmap_new(
		window,
		width, height,
		-1
	    );
	    GtkStyle *style = gtk_widget_get_style(w);

	    GDK_PIXMAP_UNREF(list->shadow_pm);
	    list->shadow_pm = pixmap;

	    if(pixmap != NULL)
	    {
		gint mask_width = 8, mask_height = 2;
		guint8 bm_data_8x2[] = { 0x55, 0xaa };
		GdkBitmap *mask;
		GdkColor *c, shadow_color;
		GdkColormap *colormap = gdk_window_get_colormap(window);
		GdkGC *gc;

		switch(list->shadow_style)
		{
		  case PULIST_SHADOW_NONE:
		    break;

		  case PULIST_SHADOW_DITHERED:
		    if(root != NULL)
		    {
			gc = gdk_gc_new(window);
			gdk_gc_set_subwindow(gc, GDK_INCLUDE_INFERIORS);
			gdk_window_copy_area(
			    (GdkWindow *)pixmap,
			    gc,
			    0, 0,
			    root,
			    x + PULIST_SHADOW_OFFSET_X,
			    y + PULIST_SHADOW_OFFSET_Y,
			    width, height
			);
			gdk_gc_set_subwindow(gc, GDK_CLIP_BY_CHILDREN);

			mask = gdk_bitmap_create_from_data(
			    window,
			    (const gchar *)bm_data_8x2,
			    mask_width, mask_height
			);
			if(mask != NULL)
			{
			    gint x, y;
			    gdk_gc_set_clip_mask(gc, mask);
			    gdk_gc_set_foreground(gc, &style->black);
			    for(y = 0; y < height; y += mask_height)
			    {
				for(x = 0; x < width; x += mask_width)
				{
				    gdk_gc_set_clip_origin(gc, x, y);
				    gdk_draw_rectangle(
					(GdkDrawable *)pixmap,
					gc,
					TRUE,	/* Fill */
					x, y,
					mask_width, mask_height
				    );
				}
			    }
			    gdk_gc_set_clip_mask(gc, NULL);
			}
			GDK_GC_UNREF(gc);
			GDK_BITMAP_UNREF(mask);
		    }
		    break;

		  case PULIST_SHADOW_BLACK:
		    gdk_draw_rectangle(
			(GdkDrawable *)pixmap,
			style->black_gc,
			TRUE,			/* Fill */
			0, 0,
			width, height
		    );
		    break;

		  case PULIST_SHADOW_DARKENED:
		    if(root != NULL)
		    {
			gc = gdk_gc_new(window);
			c = &shadow_color;
			c->red = 0x5fff;
			c->green = 0x5fff;
			c->blue = 0x5fff;
			GDK_COLORMAP_ALLOC_COLOR(colormap, c);

			gdk_gc_set_subwindow(gc, GDK_INCLUDE_INFERIORS);
			gdk_window_copy_area(
			    (GdkWindow *)pixmap,
			    gc,
			    0, 0,
			    root,
			    x + PULIST_SHADOW_OFFSET_X,
			    y + PULIST_SHADOW_OFFSET_Y,
			    width, height
			);
			gdk_gc_set_subwindow(gc, GDK_CLIP_BY_CHILDREN);

			gdk_gc_set_function(gc, GDK_AND);
			gdk_gc_set_foreground(gc, c);
			gdk_draw_rectangle(
			    (GdkDrawable *)pixmap,
			    gc,
			    TRUE,		/* Fill */
			    0, 0,
			    width, height
			);
			gdk_gc_set_function(gc, GDK_COPY);

			GDK_GC_UNREF(gc);
			GDK_COLORMAP_FREE_COLOR(colormap, c);
		    }
		    break;
		}
	    }
	}
	else
	{
	    GDK_PIXMAP_UNREF(list->shadow_pm);
	    list->shadow_pm = NULL;
	}

	/* Map the shadow */
	if(list->shadow_style != PULIST_SHADOW_NONE)
	{
	    gtk_widget_set_usize(shadow, width, height);
	    gtk_widget_popup(
		shadow,
		x + PULIST_SHADOW_OFFSET_X,
		y + PULIST_SHADOW_OFFSET_Y
	    );
	}

	/* Map the Popup List */
	gtk_widget_set_usize(toplevel, width, height);
	gtk_widget_popup(toplevel, x, y);
	list->flags |= POPUP_LIST_MAPPED;

	/* Set up the popup list's GtkCList for dragged selecting */
	PUListCListDragSetUp(list);

	/* Schedual the map idle callback */
	gtk_idle_add(PUListMapIdleCB, list);

	/* Wait for user response by pushing a GTK main level */
	if(list->main_level < 0)
	    list->main_level = 0;
	list->main_level++;

	list->freeze_count--;

	gtk_main();

	list->freeze_count++;

	/* Broke out of our GTK main level */

	/* Remove grabs from clist and do clean up after dragged
	 * selecting
	 */
	PUListCListDragCleanUp(list);

	/* Unmap the shadow and the Popup List */
	gtk_widget_hide(toplevel);
	gtk_widget_hide(shadow);
	list->flags &= ~POPUP_LIST_MAPPED;

	/* Unref the shadow GdkPixmap */
	GDK_PIXMAP_UNREF(list->shadow_pm);
	list->shadow_pm = NULL;

#if 0
	/* Restore the map widget after selecting */
	w = list->map_widget;
	if(w != NULL)
	{
	    /* Handle additional state restoring by widget type */
	    if(GTK_IS_BUTTON(w))
	    {
/* TODO */

	    }
	}
#endif

	/* Unset the map GtkWidget */
	list->map_widget = NULL;

	list->freeze_count--;

	/* Remove our reference count */
	(void)PUListUnref(toplevel);

	return(list->last_value);
}


/*
 *	Gets the PopupList data from the GtkWidget.
 */
static PopupList *PUListGetWidgetData(
        GtkWidget *w,
        const gchar *func_name
)
{
        const gchar *key = POPUP_LIST_KEY;
	PopupList *list;

        if(w == NULL)
            return(NULL);

	list = POPUP_LIST(gtk_object_get_data(
            GTK_OBJECT(w),
            key 
        ));
        if(list == NULL)
        {
            g_printerr(
"%s(): Warning: GtkWidget %p:\
 Unable to find the data that matches the key \"%s\".\n",
                func_name,
                w,
                key
            );
            return(NULL);
        }

        return(list);
}

/*
 *	Creates a new Popup List.
 */
GtkWidget *PUListNew(void)
{
	GdkWindow *window;
	GtkWidget	*w,
			*parent, *parent2,
			*toplevel,
			*shadow;
	GtkCList *clist;
	PopupList *list = (PopupList *)g_malloc0(
	    sizeof(PopupList)
	);
	if(list == NULL)
	    return(NULL);

	list->toplevel = toplevel = gtk_window_new(GTK_WINDOW_POPUP);
/*	list->freeze_count = 0; */
	list->ref_count = 1;
/*	list->main_level = 0; */
/*	list->flags = 0; */
	list->shadow = shadow = gtk_window_new(GTK_WINDOW_POPUP);
/*	list->shadow_pm = NULL; */
	list->shadow_style = PULIST_SHADOW_DARKENED;
/*
	list->map_widget = NULL;
	list->last_value = NULL;
 */

	list->freeze_count++;

	/* Toplevel GtkWindow */
	w = toplevel;
	gtk_object_set_data_full(
	    GTK_OBJECT(w), POPUP_LIST_KEY,
	    list, PUListDestroyCB
	);
	gtk_window_set_policy(GTK_WINDOW(w), FALSE, FALSE, FALSE);
	gtk_widget_add_events(
	    w,
	    GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK |
	    GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
	    GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK
	);
	gtk_widget_realize(w);
	window = w->window;
	if(window != NULL)
	{
	    /* Set the window to have no WM decorations or functions */
	    gdk_window_set_decorations(window, 0);
	    gdk_window_set_functions(window, 0);
	}
	gtk_signal_connect(
	    GTK_OBJECT(w), "delete_event",
	    GTK_SIGNAL_FUNC(PUListDeleteEventCB), list
	);
	parent = w;

	/* Main GtkVBox */
	list->main_vbox = w = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(parent), w);
	gtk_widget_show(w);
	parent = w;

	/* Outside GtkFrame */
	w = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(w), GTK_SHADOW_OUT);
	gtk_box_pack_start(GTK_BOX(parent), w, TRUE, TRUE, 0);
	gtk_widget_show(w);
	parent2 = w;

	/* GtkScrolledWindow for the GtkCList */
	list->scrolled_window = w = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(
	    GTK_SCROLLED_WINDOW(w),
	    GTK_POLICY_AUTOMATIC,
	    GTK_POLICY_AUTOMATIC
	);
	gtk_container_add(GTK_CONTAINER(parent2), w);
	gtk_widget_show(w);
	list->vscrollbar = GTK_SCROLLED_WINDOW(w)->vscrollbar;
	list->hscrollbar = GTK_SCROLLED_WINDOW(w)->hscrollbar;
	parent2 = w;

	/* GtkCList */
	list->clist = w = gtk_clist_new(1);
	clist = GTK_CLIST(w);
	GTK_WIDGET_SET_FLAGS(w, GTK_CAN_DEFAULT | GTK_CAN_FOCUS);
	gtk_widget_add_events(
	    w,
	    GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK |
	    GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
	    GDK_POINTER_MOTION_MASK | GDK_ENTER_NOTIFY_MASK |
	    GDK_LEAVE_NOTIFY_MASK
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "realize",
	    GTK_SIGNAL_FUNC(PUListRealizeCB), list
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "key_press_event",
	    GTK_SIGNAL_FUNC(PUListKeyPressEventCB), list
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "key_release_event",
	    GTK_SIGNAL_FUNC(PUListKeyPressEventCB), list
	);
	gtk_signal_connect_after(
	    GTK_OBJECT(w), "button_press_event",
	    GTK_SIGNAL_FUNC(PUListButtonPressEventCB), list
	);
	gtk_signal_connect_after(
	    GTK_OBJECT(w), "button_release_event",
	    GTK_SIGNAL_FUNC(PUListButtonPressEventCB), list
	);
	gtk_signal_connect_after(
	    GTK_OBJECT(w), "motion_notify_event",
	    GTK_SIGNAL_FUNC(PUListMotionNotifyEventCB), list
	);
	gtk_signal_connect_after(
	    GTK_OBJECT(w), "enter_notify_event",
	    GTK_SIGNAL_FUNC(PUListCrossingEventCB), list
	);
	gtk_signal_connect_after(
	    GTK_OBJECT(w), "leave_notify_event",
	    GTK_SIGNAL_FUNC(PUListCrossingEventCB), list
	);
	gtk_container_add(GTK_CONTAINER(parent2), w);
	gtk_widget_realize(w);
	gtk_clist_set_shadow_type(clist, GTK_SHADOW_IN);
	gtk_clist_set_selection_mode(clist, GTK_SELECTION_BROWSE);
	gtk_clist_set_row_height(clist, POPUP_LIST_ROW_SPACING);
	gtk_widget_show(w);


	/* Shadow GtkWindow */
	w = shadow;
	gtk_window_set_policy(GTK_WINDOW(w), FALSE, FALSE, FALSE);
	gtk_widget_set_app_paintable(w, TRUE);
	gtk_widget_add_events(
	    w,
	    GDK_STRUCTURE_MASK | GDK_EXPOSURE_MASK
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "expose_event",
	    GTK_SIGNAL_FUNC(PUListShadowExposeEventCB), list
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "draw",
	    GTK_SIGNAL_FUNC(PUListShadowDrawCB), list
	);
	gtk_widget_realize(w);
	window = w->window;
	if(window != NULL)
	{
	    /* Set the window to have no WM decorations or functions */
	    gdk_window_set_decorations(window, 0);
	    gdk_window_set_functions(window, 0);
#if !defined(_WIN32)
	    gdk_window_set_override_redirect(window, TRUE);
#endif	/* !_WIN32 */
	}

	list->freeze_count--;

	return(list->toplevel);
}

/*
 *	Gets the Popup List's GtkCList.
 *
 *	The w specifies the Popup List.
 */
GtkWidget *PUListGetCList(GtkWidget *w)
{
	PopupList *list = PUListGetWidgetData(
	    w,
	    "PUListGetCList"
	);
	if(list == NULL)
	    return(NULL);

	return(list->clist);
}

/*
 *	Sets the Popup List's shadow style.
 */
void PUListSetShadowStyle(
	GtkWidget *w,
	const pulist_shadow_style shadow_style
)
{
	PopupList *list = PUListGetWidgetData(
	    w,
	    "PUListSetShadowStyle"
	);
	if(list == NULL)
	    return;

	list->shadow_style = shadow_style;
}

/*
 *	Adds a reference count to the Popup List.
 *
 *	Returns the Popup List or NULL on error.
 */
GtkWidget *PUListRef(GtkWidget *w)
{
	PopupList *list = PUListGetWidgetData(
	    w,
	    "PUListRef"
	);
	if(list == NULL)
	    return(NULL);

	list->ref_count++;

	return(w);
}

/*
 *	Removes a reference count from the Popup List.
 *
 *	If the reference counts reach 0 then the Popup List will be
 *	destroyed.
 *
 *	Always returns NULL.
 */
GtkWidget *PUListUnref(GtkWidget *w)
{
	PopupList *list = PUListGetWidgetData(
	    w,
	    "PUListUnref"
	);
	if(list == NULL)
	    return(NULL);

	list->ref_count--;

	if(list->ref_count <= 0)
	    gtk_widget_destroy(w);

	return(NULL);
}


/*
 *	Creates a new GtkButton that is to appear as a popup list
 *	map button.
 */
GtkWidget *PUListNewMapButton(
	void (*map_cb)(GtkWidget *, gpointer),
	gpointer client_data
)
{
	GtkWidget *w, *button;

	/* Create the GtkButton */
	button = w = gtk_button_new();
	if(button == NULL)
	    return(button);

	/* Set the standard fixed size for the GtkButton */
	gtk_widget_set_usize(
	    w,
	    POPUP_LIST_MAP_BTN_WIDTH,
	    POPUP_LIST_MAP_BTN_HEIGHT
	);
	/* Set map callback function as "pressed" signal as needed */
	if(map_cb != NULL)
	    gtk_signal_connect_after(
		GTK_OBJECT(w), "pressed",
		GTK_SIGNAL_FUNC(map_cb), client_data
	    );

	/* Create the GtkDrawingArea */
	w = gtk_drawing_area_new();
	gtk_widget_add_events(w, GDK_EXPOSURE_MASK);
	gtk_signal_connect(
	    GTK_OBJECT(w), "expose_event",
	    GTK_SIGNAL_FUNC(PUListMapButtonExposeCB), NULL
	);
	gtk_container_add(GTK_CONTAINER(button), w);
	gtk_widget_show(w);

	return(button);
}

/*
 *	Creates a new GtkButton that is to appear as a popup list
 *	map button with an arrow.
 */
GtkWidget *PUListNewMapButtonArrow(
	GtkArrowType arrow_type, GtkShadowType shadow_type,
	void (*map_cb)(GtkWidget *, gpointer),
	gpointer client_data
)
{
	GtkWidget *w, *button;

	/* Create the GtkButton */
	button = w = gtk_button_new();
	if(button == NULL)
	    return(button);

	/* Set the standard fixed size for the GtkButton */
	gtk_widget_set_usize(
	    w,
	    POPUP_LIST_MAP_BTN_WIDTH,
	    POPUP_LIST_MAP_BTN_HEIGHT
	);
	/* Set map callback function as "pressed" signal as needed */
	if(map_cb != NULL)
	    gtk_signal_connect_after(
		GTK_OBJECT(w), "pressed",
		GTK_SIGNAL_FUNC(map_cb), client_data
	    );

	/* Create the GtkArrow */
	w = gtk_arrow_new(arrow_type, shadow_type);
	gtk_container_add(GTK_CONTAINER(button), w);
	gtk_widget_show(w);

	return(button);
}


/*
 *	Gets the PopupListBox data from the GtkWidget.
 */
static PopupListBox *PUListBoxGetWidgetData(
        GtkWidget *w,
        const gchar *func_name
)
{
        const gchar *key = POPUP_LIST_BOX_KEY;
	PopupListBox *box;

        if(w == NULL)
            return(NULL);

	box = POPUP_LIST_BOX(gtk_object_get_data(
            GTK_OBJECT(w),
            key 
        ));
        if(box == NULL)
        {
            g_printerr(
"%s(): Warning: GtkWidget %p:\
 Unable to find the data that matches the key \"%s\".\n",
                func_name,
                w,
                key
            );
            return(NULL);
        }

        return(box);
}

/*
 *	Creates a new Popup List Box.
 *
 *	The parent specifies the parent GtkWidget, which must be either
 *	a GtkBox or any type of GtkContainer. If parent is NULL then
 *	the Popup List Box's toplevel GtkWidget will not be parented.
 *
 *	The width and height specifies the size of the Popup List Box.
 *	If either width or height is -1 then the default size for that
 *	dimension is used.
 *
 *	Returns the new Popup List Box or NULL on error.
 */
GtkWidget *PUListBoxNew(
	const gint width, const gint height
)
{
	gint	_width = width,
		_height = height;
	GtkWidget *w, *parent, *parent2, *toplevel;
	PopupListBox *box = POPUP_LIST_BOX(g_malloc0(
	    sizeof(PopupListBox)
	));
	if(box == NULL)
	    return(NULL);

	box->toplevel = toplevel = gtk_vbox_new(TRUE, 0);
	box->freeze_count = 0;
	box->nlines_visible = -1;

	box->freeze_count++;

	/* Toplevel GtkHBox */
	w = toplevel;
	gtk_object_set_data_full(
	    GTK_OBJECT(w), POPUP_LIST_BOX_KEY,
	    box, PUListBoxDestroyCB
	);
	parent = w;

	w = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(parent), w, FALSE, FALSE, 0);
	gtk_widget_show(w);
	parent = w;

	/* Calculate the default minimum height? */
	if(_height < 0)
	{
	    GtkStyle *style = gtk_widget_get_style(w);
	    if(style != NULL)
	    {
		const gint frame_border = 4;
		GdkFont *font = style->font;
		if(font != NULL)
		    _height = font->ascent + font->descent +
			(2 * frame_border);
	    }
	}
	gtk_widget_set_usize(w, _width, _height);

	/* Box GtkFrame */
	box->box_frame = w = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(w), GTK_SHADOW_IN);
	gtk_box_pack_start(GTK_BOX(parent), w, TRUE, TRUE, 0);
	gtk_widget_show(w);
	parent2 = w;

	/* Box GtkDrawingArea */
	box->box_da = w = gtk_drawing_area_new();
	GTK_WIDGET_SET_FLAGS(w, GTK_CAN_FOCUS);
	gtk_widget_add_events(
	    w,
	    GDK_STRUCTURE_MASK | GDK_EXPOSURE_MASK |
	    GDK_FOCUS_CHANGE_MASK |
	    GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK |
	    GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
	    GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "expose_event",
	    GTK_SIGNAL_FUNC(PUListBoxEventCB), box
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "focus_in_event",
	    GTK_SIGNAL_FUNC(PUListBoxEventCB), box
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "focus_out_event",
	    GTK_SIGNAL_FUNC(PUListBoxEventCB), box
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "key_press_event",
	    GTK_SIGNAL_FUNC(PUListBoxEventCB), box
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "key_release_event",
	    GTK_SIGNAL_FUNC(PUListBoxEventCB), box
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "button_press_event",
	    GTK_SIGNAL_FUNC(PUListBoxEventCB), box
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "button_release_event",
	    GTK_SIGNAL_FUNC(PUListBoxEventCB), box
	);
	gtk_container_add(GTK_CONTAINER(parent2), w);
	gtk_widget_show(w);

	/* Map GtkButton */
	box->map_btn = w = PUListNewMapButtonArrow(
	    GTK_ARROW_DOWN,
	    GTK_SHADOW_OUT,
	    PUListBoxMapCB, box
	);
	gtk_box_pack_start(GTK_BOX(parent), w, FALSE, FALSE, 0);
	gtk_widget_show(w);

	/* Popup List */
	box->popup_list = PUListNew();

	box->freeze_count--;

	return(box->toplevel);
}

/*
 *	Gets the Popup List Box's Popup List.
 */
GtkWidget *PUListBoxGetPUList(GtkWidget *w)
{
	PopupListBox *box = PUListBoxGetWidgetData(
	    w,
	    "PUListBoxGetPUList"
	);
	if(box == NULL)
	    return(NULL);

	return(box->popup_list);
}

/*
 *	Sets the number of lines visible on the Popup List Box's
 *	Popup List.
 *
 *	The box specifies the Popup List Box.
 *
 *	The nlines_visible specifies the number of lines visible or
 *	-1 for the default number of lines visisble.
 */
void PUListBoxSetLinesVisible(
	GtkWidget *w,
	const gint nlines_visible
)
{
	PopupListBox *box = PUListBoxGetWidgetData(
	    w,
	    "PUListBoxSetLinesVisible"
	);
	if(box == NULL)
	    return;

	box->nlines_visible = nlines_visible;
}

/*
 *	Sets the Popup List Box's changed callback.
 */
void PUListBoxSetChangedCB(
	GtkWidget *w,
	void (*func)(
		GtkWidget *,			/* Popup List Box */
		const gint,			/* Item index */
		gpointer			/* Data */
	),
	gpointer data 
)
{
	PopupListBox *box = PUListBoxGetWidgetData(
	    w,
	    "PUListBoxSetChangedCB"
	);
	if(box == NULL)
	    return;

	box->changed_cb = func;
	box->changed_data = data;
}

/*
 *	Sets the Popup List Box's tip.
 */
void PUListBoxSetTip(
	GtkWidget *w,
	const gchar *tip
)
{
	PopupListBox *box = PUListBoxGetWidgetData(
	    w,
	    "PUListBoxSetTip"
	);
	if(box == NULL)
	    return;

	GUISetWidgetTip(
	    box->box_da,
	    tip
	);
}

/*
 *	Grabs focus.
 */
void PUListBoxGrabFocus(GtkWidget *w)
{
	PopupListBox *box = PUListBoxGetWidgetData(
	    w,
	    "PUListBoxGrabFocus"
	);
	if(box == NULL)
	    return;

	gtk_widget_grab_focus(box->box_da);
}

/*
 *	Selects the Popup List Box's item.
 *
 *	The box specifies the Popup List Box.
 *
 *	The i specifies the item to selected.
 */
void PUListBoxSelect(
	GtkWidget *w,
	const gint i
)
{
	PopupListBox *box = PUListBoxGetWidgetData(
	    w,
	    "PUListBoxSelect"
	);
	if(box == NULL)
	    return;

	/* Do not check for a change in the selected item since
	 * the item's value may have changed and checking its index
	 * to be changed is not reliable
	 */

	PUListSelect(box->popup_list, i);
	gtk_widget_queue_draw(box->box_da);
}

/*
 *	Gets the total number of items in the Popup List Box's Popup
 *	List.
 *
 *	The box specifies the Popup List Box.
 *
 *	Returns the total number of items in the opup List Box's
 *	Popup List.
 */
gint PUListBoxGetTotalItems(GtkWidget *w)
{
	GtkWidget *list = PUListBoxGetPUList(w);
	return(PUListGetTotalItems(list));
}

/*
 *	Gets the Popup List Box's current selected item's index.
 *
 *	The box specifies the Popup List Box.
 *
 *	Returns the current selected item's index or negative on error.
 */
gint PUListBoxGetSelected(GtkWidget *w)
{
	GtkWidget *list = PUListBoxGetPUList(w);
	return(PUListGetSelectedLast(list));
}

/*
 *	Gets the Popup List Box's current selected item's data.
 *
 *	The box specifies the Popup List Box.
 *
 *	Returns the current selected item's data or NULL on error.
 */
gpointer PUListBoxGetSelectedData(GtkWidget *w)
{
	GtkWidget *list = PUListBoxGetPUList(w);
	if(list == NULL)
	    return(NULL);

	return(PUListGetItemData(
	    list,
	    PUListGetSelectedLast(list)
	));
}
