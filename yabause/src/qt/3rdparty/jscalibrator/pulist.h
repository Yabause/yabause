/*
		      Popup List & Popup List Box
 */

#ifndef PULIST_H
#define PULIST_H

#include <gtk/gtk.h>


/*
 *	Popup Relative:
 */
typedef enum {
	PULIST_RELATIVE_CENTER,
	PULIST_RELATIVE_UP,
	PULIST_RELATIVE_DOWN,
	PULIST_RELATIVE_ABOVE,
	PULIST_RELATIVE_BELOW
} pulist_relative;


/*
 *	Shadow Styles:
 */
typedef enum {
	PULIST_SHADOW_NONE,
	PULIST_SHADOW_DITHERED,
	PULIST_SHADOW_BLACK,
	PULIST_SHADOW_DARKENED
} pulist_shadow_style;


/* Finding */
extern gint PUListFindItemFromValue(
	GtkWidget *w,
	const gchar *value
);
extern gpointer PUListGetDataFromValue(
	GtkWidget *w,
	const gchar *value
);

/* Add/Remove Item */
extern gint PUListAddItem(
	GtkWidget *w,
	const gchar *value
);
extern gint PUListAddItemPixText(
	GtkWidget *w,
	const gchar *value,
	GdkPixmap *pixmap, GdkBitmap *mask
);
extern void PUListClear(GtkWidget *w);

/* Set Item */
extern void PUListSetItemText(
	GtkWidget *w,
	const gint i,
	const gchar *value  
);
extern void PUListSetItemPixText(
	GtkWidget *w,
	const gint i,
	const gchar *value,
	GdkPixmap *pixmap, GdkBitmap *mask
);
extern void PUListSetItemSensitive(
	GtkWidget *w,
	const gint i,
	const gboolean sensitive
);
extern void PUListSetItemData(
	GtkWidget *w,
	const gint i,
	gpointer data
);
extern void PUListSetItemDataFull(
	GtkWidget *w,
	const gint i,
	gpointer data,
	GtkDestroyNotify destroy_cb
);

/* Get Item */
extern gint PUListGetTotalItems(GtkWidget *w);
extern void PUListGetItemText(
	GtkWidget *w,
	const gint i,
	const gchar **value_rtn
);
extern void PUListGetItemPixText(
	GtkWidget *w,
	const gint i,
	const gchar **value_rtn,
	GdkPixmap **pixmap_rtn, GdkBitmap **mask_rtn
);
extern gboolean PUListGetItemSensitive(
	GtkWidget *w,
	const gint i
);
extern gpointer PUListGetItemData(
	GtkWidget *w,
	const gint i
);

/* Selecting */
extern gint PUListGetSelectedLast(GtkWidget *w);
extern void PUListSelect(
	GtkWidget *w,
	const gint i
);
extern void PUListUnselectAll(GtkWidget *w);

/* Query */
extern gboolean PUListIsQuery(GtkWidget *w);
extern void PUListBreakQuery(GtkWidget *w);
extern const gchar *PUListMapQuery(
	GtkWidget *w,
	const gchar *value,
	const gint nlines_visible,
	const pulist_relative relative,
	GtkWidget *rel_widget,
	GtkWidget *map_widget
);

/* Popup List */
extern GtkWidget *PUListNew(void);
extern GtkWidget *PUListGetCList(GtkWidget *w);
extern void PUListSetShadowStyle(
	GtkWidget *w,
	const pulist_shadow_style shadow_style
);
extern GtkWidget *PUListRef(GtkWidget *w);
extern GtkWidget *PUListUnref(GtkWidget *w);


/* Map Button */
extern GtkWidget *PUListNewMapButton(
	void (*map_cb)(GtkWidget *, gpointer),
	gpointer data
);
extern GtkWidget *PUListNewMapButtonArrow(
	GtkArrowType arrow_type,
	GtkShadowType shadow_type,
	void (*map_cb)(GtkWidget *, gpointer),
	gpointer data
);


/* Popup List Box */
extern GtkWidget *PUListBoxNew(
	const gint width, const gint height
);
extern GtkWidget *PUListBoxGetPUList(GtkWidget *w);
extern void PUListBoxSetLinesVisible(
	GtkWidget *w,
	const gint nlines_visible
);
extern void PUListBoxSetChangedCB(
	GtkWidget *w,
	void (*func)(
		GtkWidget *,			/* Popup List Box */
		const gint,			/* Item index */
		gpointer			/* Data */
	),
	gpointer data
);
extern void PUListBoxSetTip(
	GtkWidget *w,
	const gchar *tip
);
extern void PUListBoxSelect(
	GtkWidget *w,
	const gint i
);
extern gint PUListBoxGetTotalItems(GtkWidget *w);
extern gint PUListBoxGetSelected(GtkWidget *w);
extern gpointer PUListBoxGetSelectedData(GtkWidget *w);
extern void PUListBoxGrabFocus(GtkWidget *w);


#endif	/* PULIST_H */
