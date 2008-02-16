#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <gtk/gtk.h>
#include <gtk/gtkinvisible.h>
#include <gdk/gdkkeysyms.h>
#if defined(_WIN32)
# include <gdk/gdkprivate.h>
# include <gdk/gdkwin32.h>
#else
# include <gdk/gdkx.h>
#endif

#if defined(__SOLARIS__) || defined(__hpux)
# include "../include/os.h"
#endif

#include "guiutils.h"


#include "images/icon_browse_20x20.xpm"
#include "images/icon_cut_20x20.xpm"
#include "images/icon_copy_20x20.xpm"
#include "images/icon_paste_20x20.xpm"
#include "images/icon_cancel_20x20.xpm"


/*
 *	Root GdkWindow:
 */
static GdkWindow	*gui_window_root = NULL;


/*
 *	Block Input Data:
 *
 *	For grabbing and blocking input in GUIBlockInput*().
 */
typedef struct {
	GtkWidget	*invisible;	/* To grab all input */
	gint		level;
} GUIBlockInputData;
#define GUI_BLOCK_INPUT_DATA(p)		((GUIBlockInputData *)(p))
#define GUI_BLOCK_INPUT_DATA_KEY	"GUI/BlockInput"


/*
 *	DDE Selection Owner Data:
 */
typedef struct {
	GtkWidget	*w;			/* Owner GtkWidget */
	GdkAtom		target_atom,		/* GdkTarget */
			type_atom;		/* GdkSelectionType */
	guint		selection_clear_event_sigid,
			selection_get_sigid;
	gpointer	data;			/* Data (coppied) */
	gint		length;			/* Data length in bytes */
} GUIDDEOwnerData;
#define GUI_DDE_OWNER_DATA(p)		((GUIDDEOwnerData *)(p))
#define GUI_DDE_OWNER_DATA_KEY		"GUI/DDE/Owner"


/*
 *	DND Source GtkWidget Data:
 */
typedef struct {
	GtkWidget	*widget;		/* This source GtkWidget */
	GList		*targets_list;		/* GList of GtkTargetEntry *
						 * targets */
} GUIDNDSourceData;
#define GUI_DND_SOURCE_DATA(p)		((GUIDNDSourceData *)(p))
#define GUI_DND_SOURCE_DATA_KEY		"GUI/DND/Source"

/*
 *	GList of GtkWidget * DND source widgets.
 */
static GList	*gui_dnd_source_widgets_list = NULL;


/*
 *	DND Target GtkWidget Data:
 */
typedef enum {
	GUI_DND_TARGET_HIGHLIGHT	= (1 << 0),	/* Do highlighting of
							 * the target GtkWidget */
	GUI_DND_TARGET_HIGHLIGHTED	= (1 << 1),	/* Currently highlighted */
	GUI_DND_TARGET_DRAG_OVER	= (1 << 2)	/* Drag is currently
							 * over the target
							 * GtkWidget (not
							 * nessisarily accepted) */
} GUIDNDTargetFlags;

typedef struct {
	GUIDNDTargetFlags	flags;
	GtkWidget	*widget;		/* This target GtkWidget */
	GList		*targets_list;		/* GList of GtkTargetEntry *
						 * targets */
	GdkDragAction	allowed_actions,	/* List of allowed actions */
			default_action_same,	/* Action to use if same widget */
			default_action;		/* Action for all other cases */
	gint		x, y;			/* Current drag position over
						 * the widget */
	guint		highlight_draw_sigid,
			unhighlight_draw_sigid,
			drag_leave_sigid,
			drag_motion_sigid;
} GUIDNDTargetData;
#define GUI_DND_TARGET_DATA(p)		((GUIDNDTargetData *)(p))
#define GUI_DND_TARGET_DATA_KEY		"GUI/DND/Target"


/*
 *	DND Icon Widget:
 *
 *	Initialized on the first call to GUIDNDSetDragIcon() and used
 *	in GUIDNDDragBeginCB().
 */
typedef struct {
	GtkWidget	*toplevel,	/* GtkWindow */
			*icon_pm;	/* Icon GtkPixmap */
	gint		x,		/* Hot spot */
			y,
			width,		/* Size */
			height;
} GUIDNDIcon;
#define GUI_DND_ICON(p)			((GUIDNDIcon *)(p))
static GUIDNDIcon			gui_dnd_icon;


/*
 *	Banner Data:
 */
typedef struct {
	GtkWidget	*toplevel;	/* GtkDrawingArea */
	gchar		*label;
	GtkJustification	justify;
	GdkFont		*font;
} GUIBannerData;
#define GUI_BANNER_DATA(p)		((GUIBannerData *)(p))
#define GUI_BANNER_DATA_KEY		"GUI/Banner"


/*
 *	Button Data:
 *
 *	For recording widgets on buttons and toggle buttons, stored as
 *	the object's user data.
 */
typedef struct {
	GtkWidget	*button,	/* GtkButton */
			*main_box,	/* GtkBox */
			*label,		/* GtkLabel */
			*pixmap,	/* GtkPixmap */
			*arrow;		/* GtkArrow */
	gchar		*label_text;	/* Copy of label text */
	const guint8	**icon_data;	/* Shared icon data */
	GtkArrowType	arrow_type;
} GUIButtonData;
#define GUI_BUTTON_DATA(p)		((GUIButtonData *)(p))
#define GUI_BUTTON_DATA_KEY		"GUI/Button"

/*
 *	Pullout Data:
 */
#define GUI_PULLOUT_DATA_KEY		"GUI/PullOut"

/*
 *	Combo Data:
 */
#define GUI_COMBO_DATA_KEY		"GUI/Combo"


/*
 *	Menu Item Data:
 *
 *	For recording widgets on menu items, stored as the object's
 *	user data.
 */
typedef struct {
	GtkWidget	*menu_item,
			*label,
			*accel_label,
			*pixmap;
	gchar		*label_text,	/* Copy of label text */
			*accel_text;	/* Copy of accel label text */
	const guint8	**icon_data;	/* Shared icon data */
	guint		accel_key,
			accel_mods;
} GUIMenuItemData;
#define GUI_MENU_ITEM_DATA(p)		((GUIMenuItemData *)(p))
#define GUI_MENU_ITEM_DATA_KEY		"GUI/Menu/Item"

/*
 *	Menu Item Activate Data:
 *
 *	Used to provide the widget, callback function, and data
 *	information to pass to the callback function when a menu item's
 *	"activate" signal is called.
 */
typedef struct {

	GtkWidget	*w;

	void		(*cb)(
		GtkWidget *w,
		gpointer
	);
	gpointer	data;

} GUIMenuItemActivateData;
#define GUI_MENU_ITEM_ACTIVATE_DATA(p)	((GUIMenuItemActivateData *)(p))
#define GUI_MENU_ITEM_ACTIVATE_DATA_KEY	"GUI/Menu/Item/Activate"


/*
 *	GtkEditable Popup Menu:
 */
typedef struct {

	GtkWidget	*menu,
			*editable,
			*undo_mi,
			*undo_msep,
			*cut_mi,
			*copy_mi,
			*paste_mi,
			*delete_mi,
			*select_msep,
			*select_all_mi,
			*unselect_all_mi;

	gint		freeze_count;
	gui_editable_popup_menu_flags	flags;

	gchar		*undo_buf,
			*redo_buf;
	gint		undo_last_pos,
			redo_last_pos;

} GUIEditablePopupMenuData;
#define GUI_EDITABLE_POPUP_MENU_DATA(p)		((GUIEditablePopupMenuData *)(p))
#define GUI_EDITABLE_POPUP_MENU_DATA_KEY	"GUI/Editable/PopupMenu"


/*
 *	GtkTooltips Group:
 *
 *	Used as the tool tips group for all GUI*() functions.
 */
static GtkTooltips	*gui_tooltips = NULL;
static gboolean		gui_tooltips_state;	/* Global enabled or disabled */


/* Get Key Name */
static gchar *GUIGetKeyName(guint key);

/* Block Input Callbacks */
static void GUIBlockInputDataDestroyCB(gpointer data);

/* Dynamic Data Exchange Callbacks */
static void GUIDDEOwnerDestroyCB(gpointer data);
static gboolean GUIDDESelectionClearEventCB(
	GtkWidget *widget, GdkEventSelection *event, gpointer data
);
static void GUIDDESelectionGetCB(
	GtkWidget *widget, GtkSelectionData *selection_data,
	guint info, guint t, gpointer data
);
static void GUIDDESelectionReceivedCB(
	GtkWidget *widget, GtkSelectionData *selection_data, guint t,
	gpointer data
);

/* Drag & Drop Source Callbacks */
static void GUIDNDDragBeginCB(
	GtkWidget *widget, GdkDragContext *dc, gpointer data
);
static void GUIDNDDragEndCB(
	GtkWidget *widget, GdkDragContext *dc,
	gpointer data
);
static void GUIDNDSourceDataDestroyCB(gpointer data);

/* Drag & Drop Target Callbacks */
static gint GUIDNDTargetHighlightDrawCB(gpointer data);
static gint GUIDNDTargetUnhighlightDrawCB(gpointer data);
static void GUIDNDTargetDragLeaveCB(
	GtkWidget *widget, GdkDragContext *dc,
	guint t,
	gpointer data
);
static gboolean GUIDNDTargetDragMotionCB(
	GtkWidget *widget, GdkDragContext *dc,
	gint x, gint y,
	guint t,
	gpointer data
);
static void GUIDNDTargetDataDestroyCB(gpointer data);

/* Button Callbacks */
static void GUIButtonDestroyCB(gpointer data);
static void GUIButtonStateChangedCB(
	GtkWidget *widget, GtkStateType previous_state, gpointer data
);

/* Banner Callbacks */
static void GUIBannerDataDestroyCB(gpointer data);
static gint GUIBannerEventCB(
	GtkWidget *widget, GdkEvent *event, gpointer data
);

/* GtkCombo Callbacks */
static void GUIComboActivateCB(GtkWidget *widget, gpointer data);
static void GUIComboChangedCB(GtkWidget *widget, gpointer data);
static void GUIComboDataDestroyCB(gpointer data);

/* GtkMenuItem Callbacks */
static void GUIMenuItemActivateCB(gpointer data);
static void GUIMenuItemActivateDataDestroyCB(gpointer data);
static void GUIMenuItemDataDestroyCB(gpointer data);

/* GtkEditable Popup Menu Callbacks */
static void GUIEditablePopupMenuDataDestroyCB(gpointer data);
static void GUIEditablePopupMenuUndoCB(GtkWidget *widget, gpointer data);
static void GUIEditablePopupMenuCutCB(GtkWidget *widget, gpointer data);
static void GUIEditablePopupMenuCopyCB(GtkWidget *widget, gpointer data);
static void GUIEditablePopupMenuPasteCB(GtkWidget *widget, gpointer data);
static void GUIEditablePopupMenuDeleteCB(GtkWidget *widget, gpointer data);
static void GUIEditablePopupMenuSelectAllCB(GtkWidget *widget, gpointer data);
static void GUIEditablePopupMenuUnselectAllCB(GtkWidget *widget, gpointer data);
static gint GUIEditablePopupMenuEditableEventCB(
	GtkWidget *widget, GdkEvent *event, gpointer data
);

/* Pull Out Callbacks */
static void GUIPullOutDataDestroyCB(gpointer data);
static void GUIPullOutDrawHandleDrawCB(
	GtkWidget *widget, GdkRectangle *area, gpointer data
);
static gint GUIPullOutDrawHandleCB(
	GtkWidget *widget, GdkEvent *event, gpointer data
);
static gint GUIPullOutPullOutBtnCB(
	GtkWidget *widget, GdkEvent *event, gpointer data
);
static gint GUIPullOutCloseCB(GtkWidget *widget, GdkEvent *event, gpointer data);


/* GtkRcStyle Utilities */
GtkRcStyle *GUIRCStyleCopy(const GtkRcStyle *rcstyle);
void GUIRCStyleSetRecursive(GtkWidget *w, GtkRcStyle *rcstyle);


/* Geometry */
GdkGeometryFlags GUIParseGeometry(
	const gchar *s,
	gint *x, gint *y, gint *width, gint *height
);
gboolean GUICListGetCellGeometry(
	GtkCList *clist, const gint column, const gint row,
	gint *x, gint *y, gint *width, gint *height
);
gint GUICTreeNodeRow(GtkCTree *ctree, GtkCTreeNode *node);
static void GUICTreeNodeDeltaRowsIterate(
	GtkCTree *ctree, GtkCTreeNode *node, GtkCTreeNode *end,
	gint *row_count, gboolean *end_found
);
gint GUICTreeNodeDeltaRows(
	GtkCTree *ctree,
	GtkCTreeNode *start,	/* Use NULL for first/toplevel node */
	GtkCTreeNode *end
);
void GUIGetWindowRootPosition(
	GdkWindow *window, gint *x, gint *y
);
void GUIGetPositionRoot(
	GdkWindow *w, gint x, gint y, gint *rx, gint *ry
);


/* String Utilities */
void GUIGetTextBounds(
	GdkFont *font, const gchar *text, const gint text_length,
	GdkTextBounds *bounds
);
void GUIGetStringBounds(
	GdkFont *font, const gchar *string,
	GdkTextBounds *bounds
);


/* GdkGC Utilities */
GdkGC *GDK_GC_NEW(void);


/* Block Input */
void GUIBlockInput(
	GtkWidget *w,
	const gboolean block
);
gint GUIBlockInputGetLevel(GtkWidget *w);


/* GdkWindow Utilities */
gint GUIWindowGetRefCount(GdkWindow *window);


/* WM Utilities */
void GUISetWMIcon(GdkWindow *window, guint8 **data);
void GUISetWMIconFile(GdkWindow *window, const gchar *filename);


/* GtkWindow Utilities */
void GUIWindowApplyArgs(
	GtkWindow *w,
	const gint argc, gchar **argv
);
gboolean GUIIsWindowArg(const gchar *arg);

/* GtkCTree Utilities */
gboolean GUICTreeOptimizeExpandPosition(
	GtkCTree *ctree,
	GtkCTreeNode *node
);


/* GdkBitmap */
GdkBitmap *GUICreateBitmapFromDataRGBA(
	const gint width, const gint height,
	const gint bpl,
	const guint8 *rgba,
	const guint8 threshold,
	GdkWindow *window
);


/* Pixmap Utilities */
GdkPixmap *GDK_PIXMAP_NEW(gint width, gint height);
GdkPixmap *GDK_PIXMAP_NEW_FROM_XPM_DATA(
	GdkBitmap **mask_rtn,
	guint8 **data
);
GdkPixmap *GDK_PIXMAP_NEW_FROM_XPM_FILE(
	GdkBitmap **mask_rtn,
	const gchar *filename
);
GtkWidget *gtk_pixmap_new_from_xpm_d(
	GdkWindow *window,
	GdkColor *transparent_color,
	guint8 **data
);
GtkWidget *GUICreateMenuItemIcon(guint8 **icon_data);


/* Objects */
GtkObject *GUIObjectRef(GtkObject *o);


/* Adjutments */
void GUIAdjustmentSetValue(
	GtkAdjustment *adj,
	const gfloat v,
	const gboolean emit_value_changed
);


/* Widget & Window Mapping */
void GUIWidgetMapRaise(GtkWidget *w);
void GUIWindowMinimize(GtkWindow *window);


/* Tooltips */
static void GUICreateGlobalTooltips(void);
void GUISetWidgetTip(
	GtkWidget *w,
	const gchar *tip
);
void GUIShowTipsNow(GtkWidget *w);
void GUISetGlobalTipsState(const gboolean state);


/* Buttons With Icon & Label */
void GUIButtonChangeLayout(
	GtkWidget *w,
	const gboolean show_pixmap,
	const gboolean show_label
);
void GUIButtonLabelUnderline(
	GtkWidget *w,
	const guint c
);
void GUIButtonPixmapUpdate(
	GtkWidget *w,
	guint8 **icon,
	const gchar *label
);
void GUIButtonArrowUpdate(
	GtkWidget *w,
	GtkArrowType arrow_type,
	const gint arrow_width, const gint arrow_height,
	const gchar *label
);
GtkWidget *GUIButtonGetMainBox(GtkWidget *w);
GtkWidget *GUIButtonGetLabel(GtkWidget *w);
GtkWidget *GUIButtonGetPixmap(GtkWidget *w);
GtkWidget *GUIButtonGetArrow(GtkWidget *w);
const gchar *GUIButtonGetLabelText(GtkWidget *w);
static GtkWidget *GUIButtonPixmapLabelHV(
	guint8 **icon,
	const gchar *label,
	GtkWidget **label_rtn,
	gboolean horizontal
);
GtkWidget *GUIButtonPixmapLabelH(
	guint8 **icon,
	const gchar *label,
	GtkWidget **label_rtn
);
GtkWidget *GUIButtonPixmapLabelV(
	guint8 **icon,
	const gchar *label,
	GtkWidget **label_rtn
);
GtkWidget *GUIButtonPixmap(guint8 **icon);

/* Buttons With Arrow & Label */
static GtkWidget *GUIButtonArrowLabelHV(
	const GtkArrowType arrow_type,
	const gint arrow_width,
	const gint arrow_height,
	const gchar *label,
	GtkWidget **label_rtn,
	const gboolean horizontal
);
GtkWidget *GUIButtonArrowLabelH(
	GtkArrowType arrow_type, gint arrow_width, gint arrow_height, 
	const gchar *label, GtkWidget **label_rtn
);
GtkWidget *GUIButtonArrowLabelV(
	GtkArrowType arrow_type, gint arrow_width, gint arrow_height,
	const gchar *label, GtkWidget **label_rtn
);
GtkWidget *GUIButtonArrow(
	GtkArrowType arrow_type, gint arrow_width, gint arrow_height
);

/* Toggle Buttons With Icon & Label */
static GtkWidget *GUIToggleButtonPixmapLabelHV(
	guint8 **icon,
	const gchar *label,
	GtkWidget **label_rtn,
	const gboolean horizontal
);
GtkWidget *GUIToggleButtonPixmapLabelH(
	guint8 **icon,
	const gchar *label,
	GtkWidget **label_rtn
);
GtkWidget *GUIToggleButtonPixmapLabelV(
	guint8 **icon,
	const gchar *label,
	GtkWidget **label_rtn
);
GtkWidget *GUIToggleButtonPixmap(guint8 **icon);


/* Prompts With Icon, Label, Entry, and Browse */
static GtkWidget *GUIPromptBarOrBrowse(
	guint8 **icon, const gchar *label,
	gpointer *label_rtn, gpointer *entry_rtn, gpointer *browse_rtn
);
GtkWidget *GUIPromptBarWithBrowse(
	guint8 **icon, const gchar *label,
	gpointer *label_rtn, gpointer *entry_rtn, gpointer *browse_rtn,
	gpointer browse_data, void (*browse_cb)(GtkWidget *, gpointer)
);
GtkWidget *GUIPromptBar(
	guint8 **icon, const gchar *label,
	gpointer *label_rtn, gpointer *entry_rtn
);


/* GtkTargetEntry */
GtkTargetEntry *gtk_target_entry_new(void);
GtkTargetEntry *gtk_target_entry_copy(const GtkTargetEntry *entry);
void gtk_target_entry_delete(GtkTargetEntry *entry);


/* Dynamic Data Exchange */
void GUIDDESetDirect(
	GtkWidget *w,
	GdkAtom selection_atom,
	const gulong t,
	GdkAtom target_atom,
	guint8 *data, const gint length
);
void GUIDDESet(
	GtkWidget *w,
	GdkAtom selection_atom,
	const gulong t,
	GdkAtom target_atom,
	const guint8 *data, const gint length
);
guint8 *GUIDDEGet(
	GtkWidget *w,
	GdkAtom selection_atom,
	const gulong t,
	GdkAtom target_atom,
	gint *length
);
void GUIDDEClear(GtkWidget *w);
void GUIDDESetBinary(
	GtkWidget *w,
	GdkAtom selection_atom,
	const gulong t,
	const guint8 *data, const gint length
);
guint8 *GUIDDEGetBinary(
	GtkWidget *w,
	GdkAtom selection_atom,
	const gulong t,
	gint *length
);
void GUIDDESetString(
	GtkWidget *w,
	GdkAtom selection_atom,
	const gulong t,
	const gchar *data
);
gchar *GUIDDEGetString(
	GtkWidget *w,
	GdkAtom selection_atom,
	const gulong t
);


/* Drag & Drop */
void GUIDNDSetSrc(
	GtkWidget *w,
	const GtkTargetEntry *types_list, const gint ntypes,
	const GdkDragAction actions,
	const GdkModifierType buttons,
	void (*drag_begin_cb)(
		GtkWidget *, GdkDragContext *,
		gpointer
	),
	void (*drag_data_get_cb)(
		GtkWidget *, GdkDragContext *,
		GtkSelectionData *,
		guint, guint,
		gpointer
	),
	void (*drag_data_delete_cb)(
		GtkWidget *, GdkDragContext *,
		gpointer
	),
	void (*drag_end_cb)(
		GtkWidget *, GdkDragContext *,
		gpointer
	),
	gpointer data
);
void GUIDNDSetSrcTypes(
	GtkWidget *w,
	const GtkTargetEntry *types_list, const gint ntypes,
	const GdkDragAction actions,
	const GdkModifierType buttons
);
void GUIDNDSetTar(
	GtkWidget *w,
	const GtkTargetEntry *types_list, const gint ntypes,
	const GdkDragAction actions,
	const GdkDragAction default_action_same,
	const GdkDragAction default_action,
	void (*drag_data_recieved_cb)(
		GtkWidget *, GdkDragContext *,
		gint, gint,
		GtkSelectionData *,
		guint, guint,
		gpointer     
	),
	gpointer drag_data_received_data,
	const gboolean highlight
);
void GUIDNDSetTarTypes(
	GtkWidget *w,
	const GtkTargetEntry *types_list, const gint ntypes,
	const GdkDragAction actions,
	const GdkDragAction default_action_same,
	const GdkDragAction default_action,
	const gboolean highlight
);
void GUIDNDGetTarActions(
	GtkWidget *w,
	GdkDragAction *allowed_actions_rtn,
	GdkDragAction *default_action_same_rtn,
	GdkDragAction *default_action_rtn
); 
GdkDragAction GUIDNDDetermineDragAction(
	GdkDragAction requested_actions,
	GdkDragAction allowed_actions,
	GdkDragAction default_action
);
gboolean GUIDNDIsTargetAccepted(
	GtkWidget *w,
	const gchar *name,
	const GtkTargetFlags flags
);
static void GUIDNDSetDragIconFromWidgetNexus(
	GtkWidget *w,
	GList *cells_list,
	const gint nitems,
	const GtkOrientation layout_orientation,
	const GtkOrientation icon_text_orientation,
	const gint item_spacing,
	const gboolean drop_shadow,
	const gint hot_x, const gint hot_y
);
void GUIDNDSetDragIconFromCellsList(
	GtkWidget *w,
	GList *cells_list,
	const gint nitems,
	const GtkOrientation layout_orientation,
	const GtkOrientation icon_text_orientation,
	const gint item_spacing
);
void GUIDNDSetDragIconFromCListSelection(GtkCList *clist);
void GUIDNDSetDragIconFromCListCell(
	GtkCList *clist,
	const gint row, const gint column
);
void GUIDNDSetDragIconFromCList(GtkCList *clist);
void GUIDNDSetDragIconFromCTreeSelection(GtkCTree *ctree);
void GUIDNDSetDragIconFromCTreeNode(
	GtkCTree *ctree,
	GtkCTreeNode *node,
	const gint column
);
void GUIDNDSetDragIcon(
	GdkPixmap *pixmap, GdkBitmap *mask,
	const gint hot_x, const gint hot_y
);
void GUIDNDClearDragIcon(void);


/* Banners */
GtkWidget *GUIBannerCreate(
	const gchar *label,
	const gchar *font_name,
	GdkColor color_fg,
	GdkColor color_bg,
	const GtkJustification justify,
	const gboolean expand
);
void GUIBannerDraw(GtkWidget *w);


/* Combo With Label, Combo, Initial Value, and Initial List */
GtkWidget *GUIComboCreate(
	const gchar *label,	/* Label */
	const gchar *text,	/* Entry Value */
	GList *list,		/* Combo List */
	gint max_items,		/* Maximum Items In Combo List */
	gpointer *combo_rtn,	/* GtkCombo Return */
	gpointer data,
	void (*func_cb)(GtkWidget *w, gpointer),
	void (*list_change_cb)(GtkWidget *, gpointer, GList *)
);
void GUIComboActivateValue(GtkWidget *w, const gchar *value);
void GUIComboAddItem(GtkWidget *w, const gchar *value);
GList *GUIComboGetList(GtkWidget *w);
void GUIComboSetList(GtkWidget *w, GList *list);
void GUIComboClearAll(GtkWidget *w);


/* Menu Bar & Menu Item */
GtkWidget *GUIMenuBarCreate(GtkAccelGroup **accelgrp_rtn);
GtkWidget *GUIMenuCreateTearOff(void);
GtkWidget *GUIMenuCreate(void);
GtkWidget *GUIMenuItemCreate(
	GtkWidget *menu,
	gui_menu_item_type type,	/* One of GUI_MENU_ITEM_TYPE_* */
	GtkAccelGroup *accelgrp,
	guint8 **icon, const gchar *label,
	guint accel_key, guint accel_mods,
	gpointer *functional_widget_rtn,
	gpointer data,
	void (*func_cb)(GtkWidget *w, gpointer)
);
void GUISetMenuItemDefault(GtkWidget *w);
void GUISetMenuItemCrossingCB(
	GtkWidget *w,
	gint (*enter_cb)(GtkWidget *, GdkEvent *, gpointer),
	gpointer enter_data,
	gint (*leave_cb)(GtkWidget *, GdkEvent *, gpointer),
	gpointer leave_data
);
GtkWidget *GUIMenuAddToMenuBar(
	GtkWidget *menu_bar,
	GtkWidget *menu,
	const gchar *label,
	const GtkJustification justify
);
void GUIMenuItemSetLabel(GtkWidget *menu_item, const gchar *label);
void GUIMenuItemSetPixmap(GtkWidget *menu_item, guint8 **icon_data);
void GUIMenuItemSetPixmap2(
	GtkWidget *menu_item,
	GdkPixmap *pixmap, GdkBitmap *mask
);
void GUIMenuItemSetAccelKey(
	GtkWidget *menu_item, GtkAccelGroup *accelgrp,
	guint accel_key, guint accel_mods
);
void GUIMenuItemSetCheck(
	GtkWidget *menu_item,
	const gboolean active,
	const gboolean emit_signal
);
gboolean GUIMenuItemGetCheck(GtkWidget *menu_item);
GtkWidget *GUIMenuAddToMenuBarPixmapH(
	GtkWidget *menu_bar,
	GtkWidget *menu,
	const gchar *label,
	guint8 **icon_data,
	const GtkJustification justify
);
void GUIMenuItemSetSubMenu(
	GtkWidget *menu_item,
	GtkWidget *sub_menu
);


/* Editable Popup Menu */
static void GUIEditablePopupMenuRecordUndo(GUIEditablePopupMenuData *d);
static void GUIEditablePopupMenuUpdate(GUIEditablePopupMenuData *d);
GtkWidget *GUIEditableGetPopupMenu(GtkWidget *w);
void GUIEditableEndowPopupMenu(
	GtkWidget *w, const gui_editable_popup_menu_flags flags
);


/* Pull Out Window */
void *GUIPullOutCreateH(
	void *parent_box,
	int homogeneous, int spacing,		/* Of client vbox */
	int expand, int fill, int padding,	/* Of holder hbox */
	int toplevel_width, int toplevel_height,
	void *pull_out_client_data,
	void (*pull_out_cb)(void *, void *),
	void *push_in_client_data,
	void (*push_in_cb)(void *, void *)
);
void *GUIPullOutGetToplevelWindow(
	void *client_box,
	int *x, int *y, int *width, int *height
);
void GUIPullOutPullOut(void *client_box);
void GUIPullOutPushIn(void *client_box); 


#ifndef ISBLANK
# define ISBLANK(c)	(((c) == ' ') || ((c) == '\t'))
#endif

#define ATOI(s)		(((s) != NULL) ? atoi(s) : 0)
#define ATOL(s)		(((s) != NULL) ? atol(s) : 0)
#define ATOF(s)		(((s) != NULL) ? atof(s) : 0.0f)
#define STRDUP(s)	(((s) != NULL) ? g_strdup(s) : NULL)

#define MAX(a,b)	(((a) > (b)) ? (a) : (b))
#define MIN(a,b)	(((a) < (b)) ? (a) : (b))
#define CLIP(a,l,h)	(MIN(MAX((a),(l)),(h)))
#define STRLEN(s)	(((s) != NULL) ? strlen(s) : 0)
#define STRISEMPTY(s)	(((s) != NULL) ? (*(s) == '\0') : TRUE)


static gchar *G_STRCAT(gchar *s, const gchar *s2)
{
	if(s != NULL) {
	    if(s2 != NULL) {
		gchar *sr = g_strconcat(s, s2, NULL);
		g_free(s);
		s = sr;
	    }
	} else {
	    if(s2 != NULL)
		s = STRDUP(s2);
	    else
		s = STRDUP("");
	}
	return(s);
}


/*
 *	Returns a statically allocated string describing the specified
 *	key.
 */
static gchar *GUIGetKeyName(guint key)
{
	switch(key)
	{
	  case GDK_BackSpace:
	    return("BackSpace");
	  case GDK_Tab:
	    return("Tab");
	  case GDK_Linefeed:
	    return("LineFeed");
	  case GDK_Clear:
	    return("Clear");
	  case GDK_Return:
	    return("Return");
	  case GDK_Pause:
	    return("Pause");
	  case GDK_Scroll_Lock:
	    return("ScrollLock");
	  case GDK_Sys_Req:
	    return("SysReq");
	  case GDK_Escape:
	    return("Escape");
	  case GDK_space:
	    return("Space");
	  case GDK_Delete:
	    return("Delete");
	  case GDK_Home:
	    return("Home");
	  case GDK_Left:
	    return("Left");
	  case GDK_Right:
	    return("Right");
	  case GDK_Down:
	    return("Down");
	  case GDK_Up:
	    return("Up");
	  case GDK_Page_Up:
	    return("PageUp");
	  case GDK_Page_Down:
	    return("PageDown");
	  case GDK_End:
	    return("End");
	  case GDK_Begin:
	    return("Begin");
	  case GDK_Select:
	    return("Select");
	  case GDK_Print:
	    return("Print");
	  case GDK_Execute:
	    return("Execute");
	  case GDK_Insert:
	    return("Insert");
	  case GDK_Undo:
	    return("Undo");
	  case GDK_Redo:
	    return("Redo");
	  case GDK_Menu:
	    return("Menu");
	  case GDK_Find:
	    return("Find");
	  case GDK_Cancel:
	    return("Cancel");
	  case GDK_Help:
	    return("Help");
	  case GDK_Break:
	    return("Break");
	  case GDK_Num_Lock:
	    return("NumLock");

	  case GDK_KP_Space:
	    return("KPSpace");
	  case GDK_KP_Tab:
	    return("KBTab");
	  case GDK_KP_Enter:
	    return("KPEnter");
	  case GDK_KP_F1:
	    return("KPF1");
	  case GDK_KP_F2:
	    return("KPF2");
	  case GDK_KP_F3:
	    return("KPF3");
	  case GDK_KP_F4:
	    return("KPF4");
	  case GDK_KP_Home:
	    return("KPHome");
	  case GDK_KP_Left:
	    return("KPLeft");
	  case GDK_KP_Up:
	    return("KPUp");
	  case GDK_KP_Right:
	    return("KPRight");
	  case GDK_KP_Down:
	    return("KPDown");
	  case GDK_KP_Page_Up:
	    return("KPPageUp");
	  case GDK_KP_Page_Down:
	    return("KPPageDown");
	  case GDK_KP_End:
	    return("KPEnd");
	  case GDK_KP_Begin:
	    return("KPBegin");
	  case GDK_KP_Insert:
	    return("KPInsert");
	  case GDK_KP_Delete:
	    return("KPDelete");
	  case GDK_KP_Equal:
	    return("KPEqual");
	  case GDK_KP_Multiply:
	    return("KPMultiply");
	  case GDK_KP_Add:
	    return("KPAdd");
	  case GDK_KP_Separator:
	    return("KPSeparator");
	  case GDK_KP_Subtract:
	    return("KPSubtract");
	  case GDK_KP_Decimal:
	    return("KPDecimal");
	  case GDK_KP_Divide:
	    return("KPDivide");
	  case GDK_KP_0:
	    return("KP0");
	  case GDK_KP_1:
	    return("KP1");
	  case GDK_KP_2:
	    return("KP2");
	  case GDK_KP_3:
	    return("KP3");
	  case GDK_KP_4:
	    return("KP4");
	  case GDK_KP_5:
	    return("KP5");
	  case GDK_KP_6:
	    return("KP6");
	  case GDK_KP_7:
	    return("KP7");
	  case GDK_KP_8:
	    return("KP8");
	  case GDK_KP_9:
	    return("KP9");

	  case GDK_F1:
	    return("F1");
	  case GDK_F2:
	    return("F2");
	  case GDK_F3:
	    return("F3");
	  case GDK_F4:
	    return("F4");
	  case GDK_F5:
	    return("F5");
	  case GDK_F6:
	    return("F6");
	  case GDK_F7:
	    return("F7");
	  case GDK_F8:
	    return("F8");
	  case GDK_F9:
	    return("F9");
	  case GDK_F10:
	    return("F10");
	  case GDK_F11:
	    return("F11");
	  case GDK_F12:
	    return("F12");
	  case GDK_F13:
	    return("F13");
	  case GDK_F14:
	    return("F14");
	  case GDK_F15:
	    return("F15");
	  case GDK_F16:
	    return("F16");
	  case GDK_F17:
	    return("F17");
	  case GDK_F18:
	    return("F18");
	  case GDK_F19:
	    return("F19");
	  case GDK_F20:
	    return("F20");
	  case GDK_F21:
	    return("F21");
	  case GDK_F22:
	    return("F22");
	  case GDK_F23:
	    return("F23");
	  case GDK_F24:
	    return("F24");
	  case GDK_F25:
	    return("F25");
	  case GDK_F26:
	    return("F26");
	  case GDK_F27:
	    return("F27");
	  case GDK_F28:
	    return("F28");
	  case GDK_F29:
	    return("F29");
	  case GDK_F30:
	    return("F30");

	  case GDK_Shift_L:
	    return("ShiftL");
	  case GDK_Shift_R:
	    return("ShiftR");
	  case GDK_Control_L:
	    return("ControlL");
	  case GDK_Control_R:
	    return("ControlR");
	  case GDK_Caps_Lock:
	    return("CapsLock");
	  case GDK_Shift_Lock:
	    return("ShiftLock");
	  case GDK_Meta_L:
	    return("MetaL");
	  case GDK_Meta_R:
	    return("MetaR");
	  case GDK_Alt_L: 
	    return("AltL");
	  case GDK_Alt_R:
	    return("AltR");
	  case GDK_Super_L:
	    return("SuperL");
	  case GDK_Super_R: 
	    return("SuperR");
	  case GDK_Hyper_L: 
	    return("HyperL");
	  case GDK_Hyper_R: 
	    return("HyperR");
	  default:
	    return("");
	}
}


/*
 *	Block input data "destroy" signal callback.
 */
static void GUIBlockInputDataDestroyCB(gpointer data)
{
	GUIBlockInputData *d = GUI_BLOCK_INPUT_DATA(data);
	if(d == NULL)
	    return;

	gtk_widget_destroy(d->invisible);
	g_free(d);
}


/*
 *	DDE Owner data "destroy" signal callback.
 */
static void GUIDDEOwnerDestroyCB(gpointer data)
{
	GtkWidget *w;
	GUIDDEOwnerData *owner = GUI_DDE_OWNER_DATA(data);
	if(owner == NULL)
	    return;

	w = owner->w;
	if(w != NULL)
	{
#if 0
/* Do not disconnect the functions, they are already disconnected
 * when the object is destroyed
 */
	    gtk_signal_disconnect_by_func(
		GTK_OBJECT(w),
		GTK_SIGNAL_FUNC(GUIDDESelectionClearEventCB), owner
	    );
	    gtk_signal_disconnect_by_func(
		GTK_OBJECT(w),
		GTK_SIGNAL_FUNC(GUIDDESelectionGetCB), owner
	    );
#endif
	}
	g_free(owner->data);
	g_free(owner);
}

/*
 *	DDE GtkWidget "selection_clear_event" signal callback.
 */
static gboolean GUIDDESelectionClearEventCB(
	GtkWidget *widget, GdkEventSelection *event, gpointer data
)
{
	GUIDDEOwnerData *owner_data = GUI_DDE_OWNER_DATA(data);
	if((widget == NULL) || (event == NULL) || (owner_data == NULL))
	    return(FALSE);

	owner_data->target_atom = GDK_NONE;
	owner_data->type_atom = GDK_NONE;

	/* Delete the owner's data */
	g_free(owner_data->data);
	owner_data->data = NULL;
	owner_data->length = 0;

	return(TRUE);
}

/*
 *	DDE GtkWidget "selection_get" signal callback.
 */
static void GUIDDESelectionGetCB(
	GtkWidget *widget, GtkSelectionData *selection_data,
	guint info, guint t, gpointer data
)
{
	GdkAtom target;
	GUIDDEOwnerData *owner_data = GUI_DDE_OWNER_DATA(data);
	if((widget == NULL) || (selection_data == NULL) ||
	   (owner_data == NULL)
	)
	    return;

	target = owner_data->target_atom;

	/* Owner no longer has data? */
	if(target == GDK_NONE)
	{
	    gtk_selection_data_set(
		selection_data,
		GDK_NONE,
		0,
		NULL, 0
	    );
	    return;
	}

	/* Handle by owner's selection target type
	 *
	 * GdkBitmap
	 */
	if(target == GDK_TARGET_BITMAP)
	{
/* TODO */
	}
	/* GdkColormap */
	else if(target == GDK_TARGET_COLORMAP)
	{
/* TODO */
	}
	/* GdkDrawable */
	else if(target == GDK_TARGET_DRAWABLE)
	{
/* TODO */
	}
	/* GdkPixmap */
	else if(target == GDK_TARGET_PIXMAP)
	{
/* TODO */
	}
	/* String (latin-1 characters) */
	else if(target == GDK_TARGET_STRING)
	{
	    gtk_selection_data_set(
		selection_data,
		owner_data->type_atom,
		8,
		owner_data->data,
		owner_data->length
	    );
	}
	/* All else assume binary */
	else
	{
	    gtk_selection_data_set(
		selection_data,
		owner_data->type_atom,
		8,
		owner_data->data,
		owner_data->length
	    );
	}
}

/*
 *	DDE GtkWidget "selection_received" signal callback.
 */
static void GUIDDESelectionReceivedCB(
	GtkWidget *widget, GtkSelectionData *selection_data, guint t,
	gpointer data
)
{
	GdkAtom type;
	GtkSelectionData *sel_rtn = (GtkSelectionData *)data;
	if((widget == NULL) || (selection_data == NULL) ||
	   (sel_rtn == NULL)
	)
	    return;

	/* Already got the data? */
	if(sel_rtn->data != NULL)
	    return;

	/* Mark that the selection was received on the callback data */
	sel_rtn->length = 0;

	/* No or empty data? */
	if((selection_data->data == NULL) ||
	   (selection_data->length <= 0)
	)
	    return;

	/* Begin transfering the selection data to the return data
	 * by its type
	 */
	sel_rtn->type = type = selection_data->type;
	/* Atoms List */
	if(type == GDK_SELECTION_TYPE_ATOM)
	{
	    gchar *s = NULL;
	    gint i, m = selection_data->length / sizeof(GdkAtom);
	    gchar *name;
	    GdkAtom *atoms = (GdkAtom *)selection_data->data;

	    for(i = 0; i < m; i++)
	    {
		name = gdk_atom_name(atoms[i]);
		if(name != NULL)
		{
		    s = G_STRCAT(s, name);
		    if(i < (m - 1))
			s = G_STRCAT(s, " ");
		    g_free(name);
		}
	    }
	    sel_rtn->data = s;
	    sel_rtn->length = STRLEN(s);

	    /* Change the return type to string */
	    sel_rtn->type = GDK_SELECTION_TYPE_STRING;
	}
	/* GdkBitmap */
	else if(type == GDK_SELECTION_TYPE_BITMAP)
	{
/* TODO */
	}
	/* GdkColormap */
	else if(type == GDK_SELECTION_TYPE_COLORMAP)
	{
/* TODO */
	}
	/* GdkDrawable */
	else if(type == GDK_SELECTION_TYPE_DRAWABLE)
	{
/* TODO */
	}
	/* Integer */
	else if(type == GDK_SELECTION_TYPE_INTEGER)
	{
	    gint len = sel_rtn->length = selection_data->length;
	    sel_rtn->data = g_malloc(len);
	    memcpy(sel_rtn->data, selection_data->data, len);
	}
	/* GdkPixmap */
	else if(type == GDK_SELECTION_TYPE_PIXMAP)
	{
/* TODO */
	}
	/* GdkWindow */
	else if(type == GDK_SELECTION_TYPE_WINDOW)
	{
/* TODO */
	}
	/* String (latin-1 characters) */
	else if(type == GDK_SELECTION_TYPE_STRING)
	{
	    /* Need to copy the data and append a null byte at the
	     * end (even if it already has one)
	     */
	    const gint len = sel_rtn->length = selection_data->length;
	    gchar *s = (gchar *)g_malloc((len + 1) * sizeof(gchar));
	    memcpy(s, selection_data->data, len * sizeof(gchar));
	    s[len] = '\0';
	    sel_rtn->data = s;
	}
	/* All else assume binary data */
	else
	{
	    sel_rtn->data = g_memdup(
		selection_data->data,
		selection_data->length
	    );
	    sel_rtn->length = selection_data->length;
	}
}


/*
 *	DND source "drag_begin" signal callback.
 */
static void GUIDNDDragBeginCB(
	GtkWidget *widget, GdkDragContext *dc, gpointer data
)
{
	GUIDNDIcon *dnd_icon = &gui_dnd_icon;
	GUIDNDSourceData *d = GUI_DND_SOURCE_DATA(data);
	if((widget == NULL) || (dc == NULL) || (d == NULL))
	    return;

	/* Set the DND Icon if it is available */
	if(dnd_icon->toplevel != NULL)
	    gtk_drag_set_icon_widget(
		dc,
		dnd_icon->toplevel,
		dnd_icon->x,
		dnd_icon->y
	    );
}

/*
 *	DND source "drag_end" signal callback.
 */
static void GUIDNDDragEndCB(
	GtkWidget *widget, GdkDragContext *dc, gpointer data
)
{
	GUIDNDSourceData *d = GUI_DND_SOURCE_DATA(data);
	if((widget == NULL) || (dc == NULL) || (d == NULL))
	    return;
}

/*
 *	DND source "destroy" signal callback.
 */
static void GUIDNDSourceDataDestroyCB(gpointer data)
{
	GUIDNDSourceData *d = GUI_DND_SOURCE_DATA(data);
	if(d == NULL)
	    return;

	/* Remove this GtkWidget from the list of DND source GtkWidgets */
	if(d->widget != NULL)
	    gui_dnd_source_widgets_list = g_list_remove(
		gui_dnd_source_widgets_list,
		d->widget
	    );

	if(d->targets_list != NULL)
	{
	    g_list_foreach(
		d->targets_list,
		(GFunc)gtk_target_entry_delete,
		NULL
	    );
	    g_list_free(d->targets_list);
	}

	g_free(d);
}


/*
 *	DND target highlight draw idle callback.
 *
 *	Clears the highlight_draw_sigid and draws the drag highlight
 *	on the GtkWidget plus any additional highlighting depending
 *	on the GtkWidget's type.
 *
 *	If the GUI_DND_TARGET_HIGHLIGHTED is not set then only the
 *	highlight_draw_sigid will be cleared.
 */
static gint GUIDNDTargetHighlightDrawCB(gpointer data)
{
	gint width, height;
	GdkDrawable *drawable;
	GtkStateType state;
	GtkStyle *style;
	GtkWidget *w;
	GUIDNDTargetData *d = GUI_DND_TARGET_DATA(data);
	if(d == NULL)
	    return(FALSE);

	/* Clear this callback's signal id since we are handling it now */
	d->highlight_draw_sigid = 0;

	/* Do not draw the highlight if the highlighted flag is not set */
	if(!(d->flags & GUI_DND_TARGET_HIGHLIGHTED))
	    return(FALSE);

	w = d->widget;
	if(w == NULL)
	    return(FALSE);

	drawable = (GdkDrawable *)w->window;
	state = GTK_WIDGET_STATE(w);
	style = gtk_widget_get_style(w);
	if((drawable == NULL) || (style == NULL))
	    return(FALSE);

	gdk_window_get_size(drawable, &width, &height);
	if((width <= 0) || (height <= 0))
	    return(FALSE);

	/* Highlight by the target GtkWidget's type since it may
	 * not automatically highlight itself because it may not be
	 * in focus
	 *
	 * GtkCList
	 */
	if(GTK_IS_CLIST(w))
	{
	    gint row, column, list_start_y;
	    GtkCList *clist = GTK_CLIST(w);
	    const gboolean show_titles = GTK_CLIST_SHOW_TITLES(clist);
	    GdkRectangle *column_title_area = &clist->column_title_area;

	    /* Calculate the starting position of the list since we
	     * need to subtract this from the drag coordinates
	     * because the drag coordinates include the GtkCList's
	     * title area
	     */
	    list_start_y = (show_titles) ?
		(column_title_area->y + column_title_area->height) : 0;

	    /* Calculate the column and row that the drag is over */
	    if(!gtk_clist_get_selection_info(
		clist,
		d->x,
		d->y - list_start_y,
		&row, &column
	    ))
	    {
		row = -1;
		column = -1;
	    }

	    /* Change in drag position? */
	    if(row != clist->focus_row)
	    {
		GdkDrawable *drawable = (GdkDrawable *)clist->clist_window;
		GtkAdjustment	*vadj = clist->vadjustment,
				*hadj = clist->hadjustment;

		/* Set and redraw the focus row by having the widget
		 * draw itself normally without the focus row and
		 * then we will draw the focus row
		 */
		clist->focus_row = -1;
		gtk_widget_draw(w, NULL);
		clist->focus_row = row;

		/* Draw the focus row if it is visible */
		if((vadj != NULL) && (hadj != NULL) && (row > -1))
		{
		    gint	x, y,
				width, height,

		    /* Get the row height */
		    row_height = GTK_CLIST_ROW_HEIGHT_SET(clist) ?
			clist->row_height : 0;
		    if((row_height <= 0) && (style->font != NULL))
		    {
			GdkFont *font = style->font;
			row_height = font->ascent + font->descent;
		    }

		    /* Account for spacing between rows */
		    row_height += 1;

		    /* Calculate the focus row's coordinates and size */
		    x = -(gint)hadj->value;
		    y = (row_height * row) - (gint)vadj->value;
		    width = (gint)(hadj->upper - hadj->lower);
		    height = row_height;

		    /* Draw the focus row */
		    gtk_draw_focus(
			style,
			drawable,
			x, y,
			width - 1, height - 1
		    );
		}
	    }
	}

	/* Draw the drag highlight around this GtkWidget */
	gtk_draw_focus(
	    style,
	    drawable,
	    0, 0,
	    width - 1, height - 1
	);

	return(FALSE);
}

/*
 *	DND target unhighlight draw idle callback.
 *
 *	Clears the unhighlight_draw_sigid and draws the GtkWidget
 *	normally without any highlight.
 *
 *	If the GUI_DND_TARGET_HIGHLIGHTED is still set then only the
 *	unhighlight_draw_sigid will be cleared.
 */
static gint GUIDNDTargetUnhighlightDrawCB(gpointer data)
{
	GtkWidget *w;
	GUIDNDTargetData *d = GUI_DND_TARGET_DATA(data);
	if(d == NULL)
	    return(FALSE);

	/* Clear this callback's signal id since we are handling it now */
	d->unhighlight_draw_sigid = 0;

	/* Do not draw the GtkWidget normally if the highlighted flag
	 * is still set
	 */
	if(d->flags & GUI_DND_TARGET_HIGHLIGHTED)
	    return(FALSE);

	w = d->widget;
	if(w == NULL)
	    return(FALSE);

	/* Clear the focus based on the GtkWidget's type
	 *
	 * GtkCList
	 */
	if(GTK_IS_CLIST(w))
	{
	    GtkCList *clist = GTK_CLIST(w);
	    clist->focus_row = -1;
	}

	/* Unhighlight this GtkWidget by clearing the highlighed flag
	 * and queueing the GtkWidget to redraw normally
	 */
	d->flags &= ~GUI_DND_TARGET_HIGHLIGHTED;
	gtk_widget_draw(w, NULL);

	return(FALSE);
}

/*
 *	DND target "drag_leave" signal callback.
 */
static void GUIDNDTargetDragLeaveCB(
	GtkWidget *widget, GdkDragContext *dc,
	guint t,
	gpointer data
) 
{
	GUIDNDTargetData *d = GUI_DND_TARGET_DATA(data);
	if((widget == NULL) || (dc == NULL) || (d == NULL))
	    return;

	/* Clear the drag over flag */
	if(d->flags & GUI_DND_TARGET_DRAG_OVER)
	    d->flags &= ~GUI_DND_TARGET_DRAG_OVER;

	/* Clear the highlighted flag and queue the GtkWidget to
	 * redraw normally without the highlight
	 */
	if(d->flags & GUI_DND_TARGET_HIGHLIGHTED)
	{
	    d->flags &= ~GUI_DND_TARGET_HIGHLIGHTED;

	    if(d->unhighlight_draw_sigid == 0)
		d->unhighlight_draw_sigid = gtk_idle_add_priority(
		    GTK_PRIORITY_REDRAW,
		    GUIDNDTargetUnhighlightDrawCB, d
		);
	}
}

/*
 *	DND target "drag_motion" signal callback.
 *
 *	Responds with the appropriate drag action by calling
 *	gdk_drag_status() and performs highlighting.
 */
static gboolean GUIDNDTargetDragMotionCB(
	GtkWidget *widget, GdkDragContext *dc,
	gint x, gint y,
	guint t,
	gpointer data
)
{
	gboolean same_widget, same_process;
	gchar *target_name;
	GList *glist;
	GdkDragAction	requested_actions,
			allowed_actions,
			default_action,
			action;
	GtkTargetFlags target_flags;
	GtkWidget	*src_widget,
			*tar_widget;
	GUIDNDTargetData *d = GUI_DND_TARGET_DATA(data);
	if((widget == NULL) || (dc == NULL) || (d == NULL))
	    return(FALSE);

	/* Set the drag over flag */
	if(!(d->flags & GUI_DND_TARGET_DRAG_OVER))
	    d->flags |= GUI_DND_TARGET_DRAG_OVER;

	/* Record the drag position over this GtkWidget */
	d->x = x;
	d->y = y;

	/* Is the target GtkWidget not sensitive? */
	if(!GTK_WIDGET_SENSITIVE(widget))
	{
	    /* Report that we will not accept this drag because the
	     * target GtkWidget is not sensitive
	     */
	    gdk_drag_status(dc, 0, t);
	    return(TRUE);
	}

	/* Get the GtkWidget that the drag originated from */
	src_widget = gtk_drag_get_source_widget(dc);

	/* Get the GtkWidget that the drag is currently over */
	tar_widget = widget;

	/* Are the two GtkWidgets the same? */
	same_widget = (src_widget == tar_widget) ? TRUE : FALSE;

	/* Is the source GtkWidget from the same process as the
	 * target GtkWidget?
	 */
	if(same_widget)
	{
	    same_process = TRUE;
	}
	else
	{
	    if(g_list_find(gui_dnd_source_widgets_list, src_widget) != NULL)
		same_process = TRUE;
	    else
		same_process = FALSE;
	}

	/* Set the target flags to denote if this is the same
	 * GtkWidget or not
	 */
	target_flags = ((same_process) ? GTK_TARGET_SAME_APP : 0) |
	    ((same_widget) ? GTK_TARGET_SAME_WIDGET : 0);

	/* Iterate through the list of targets on the drag context
	 * and check if we will accept one of them
	 */
	for(glist = dc->targets;
	    glist != NULL;
	    glist = g_list_next(glist)
	)
	{
	    /* Get the next target name on the drag context */
	    target_name = gdk_atom_name((GdkAtom)glist->data);
	    if(target_name != NULL)
	    {
		/* Check if this target name is in the list of
		 * accepted targets on the GtkWidget
		 */
		if(GUIDNDIsTargetAccepted(
		    tar_widget,
		    target_name,
		    target_flags
		))
		{
		    g_free(target_name);
		    break;
		}

		g_free(target_name);
	    }
	}
	/* Drag not accepted? */
	if(glist == NULL)
	{
	    /* Report that we will not accept this drag because none
	     * of its targets are in the list of accepted targets on
	     * the GtkWidget
	     */
	    gdk_drag_status(dc, 0, t);
	    return(TRUE);
	}

	/* Get the list of requested actions from the drag context */
	requested_actions = dc->actions;

	/* Get the list of allowed actions on the target GtkWidget */
	allowed_actions = d->allowed_actions;

	/* Get the default action on the target GtkWidget */
	default_action = same_widget ?
	    d->default_action_same : d->default_action;

	/* Determine the appropriate drag action */
	action = GUIDNDDetermineDragAction(
	    requested_actions,
	    allowed_actions,
	    default_action
	);

	/* Respond with the appropriate drag action */
	gdk_drag_status(dc, action, t);


	/* Highlight this GtkWidget? */
	if((d->flags & GUI_DND_TARGET_HIGHLIGHT) &&
	   GTK_WIDGET_SENSITIVE(tar_widget)
	)
	{
	    /* Was a drag action determined? */
	    if(action != 0)
	    {
		/* Set the highlighted flag */
		if(!(d->flags & GUI_DND_TARGET_HIGHLIGHTED))
		    d->flags |= GUI_DND_TARGET_HIGHLIGHTED;

		/* Queue the highlight draw regardless of if the
		 * GUI_DND_TARGET_HIGHLIGHTED flag was set or not since
		 * a drag motion may have changed to a different row
		 * on a list and the new row needs to be highlighted
		 */
		if(d->highlight_draw_sigid == 0)
		    d->highlight_draw_sigid = gtk_idle_add_priority(
			GTK_PRIORITY_REDRAW,
			GUIDNDTargetHighlightDrawCB, d
		    );
	    }
	    else
	    {
		/* No drag actions were allowed, clear the highlighted
		 * flag and queue the GtkWidget to redraw normally
		 * without the highlight
		 */
		if(d->flags & GUI_DND_TARGET_HIGHLIGHTED)
		{
		    d->flags &= ~GUI_DND_TARGET_HIGHLIGHTED;

		    if(d->unhighlight_draw_sigid == 0)
			d->unhighlight_draw_sigid = gtk_idle_add_priority(
			    GTK_PRIORITY_REDRAW,
			    GUIDNDTargetUnhighlightDrawCB, d
			);
		}
	    }
	}

	/* Return TRUE since we responded with gdk_drag_status() */
	return(TRUE);
}

/*
 *	DND target data "destroy" signal callback.
 */
static void GUIDNDTargetDataDestroyCB(gpointer data)
{
	GUIDNDTargetData *d = GUI_DND_TARGET_DATA(data);
	if(d == NULL)
	    return;

	if(d->highlight_draw_sigid != 0)
	    gtk_idle_remove(d->highlight_draw_sigid);
	if(d->unhighlight_draw_sigid != 0)
	    gtk_idle_remove(d->unhighlight_draw_sigid);

	if(d->targets_list != NULL)
	{
	    g_list_foreach(
		d->targets_list,
		(GFunc)gtk_target_entry_delete,
		NULL
	    );
	    g_list_free(d->targets_list);
	}

	g_free(d);
}


/*
 *	GtkButton "destroy" signal callback.
 */
static void GUIButtonDestroyCB(gpointer data)
{
	GUIButtonData *d = GUI_BUTTON_DATA(data);
	if(d == NULL)
	    return;

	g_free(d->label_text);
	g_free(d);
}

/*
 *	GtkButton "state_changed" signal callback.
 */
static void GUIButtonStateChangedCB(
	GtkWidget *widget, GtkStateType previous_state, gpointer data
)
{
	GUIButtonData *d = GUI_BUTTON_DATA(data);
	if((widget == NULL) || (d == NULL))
	    return;

	/* Is there a GtkArrow in this button? */
	if(d->arrow != NULL)
	{
	    /* The GtkArrow's state needs to be reset when changing
	     * from GTK_STATE_INSENSITIVE to any other state because
	     * it apparently does not do so on its own
	     */
	    if(previous_state == GTK_STATE_INSENSITIVE)
		gtk_widget_set_state(
		    d->arrow,
		    GTK_WIDGET_STATE(widget)
		);
	}
}


/*
 *	Banner "destroy" signal callback.
 */
static void GUIBannerDataDestroyCB(gpointer data)
{
	GUIBannerData *banner = GUI_BANNER_DATA(data);
	if(banner == NULL)
	    return;

	g_free(banner->label);
	GDK_FONT_UNREF(banner->font);
	g_free(banner);
}

/*
 *	Banner event signal callback.
 */
static gint GUIBannerEventCB(
	GtkWidget *widget, GdkEvent *event, gpointer data
)
{
	gint status = FALSE;
	GdkEventConfigure *configure;
	GdkEventExpose *expose;
	GUIBannerData *banner = GUI_BANNER_DATA(data);
	if((widget == NULL) || (event == NULL) || (banner == NULL))
	    return(status);

	switch((gint)event->type)
	{
	  case GDK_CONFIGURE:
	    configure = (GdkEventConfigure *)event;
	    status = TRUE;
	    break;

	  case GDK_EXPOSE:
	    expose = (GdkEventExpose *)event;
	    GUIBannerDraw(widget);
	    status = TRUE;
	    break;
	}

	return(status);
}


/*
 *	GtkCombo "activate" signal callback.
 *
 *	First it will update the glist for the combo box and update
 *	the the combo's list. Then call (if not NULL) the specified
 *	activate callback function.
 */
static void GUIComboActivateCB(GtkWidget *widget, gpointer data)
{
	gint max_items;
	GtkWidget *parent, *wlabel;
	GtkCombo *combo;
	GList *glist_in;
	gpointer client_data;
	void (*func_cb)(GtkWidget *, gpointer);
	void (*list_change_cb)(GtkWidget *, gpointer, GList *);
	gchar *new_value;
	gpointer *cb_data = (gpointer *)data;
	if((widget == NULL) || (cb_data == NULL))
	    return;

	/* Parse callback data, format is:
	 *
	 * table		Parent table that holds combo and label
	 * label		Label (may be NULL)
	 * combo		This combo widget
	 * GList		Contains list of strings in combo list
	 * max_items		Max items allowed in combo list
	 * client_data		Calling function set callback data
	 * func_cb		Calling function set callback function
	 * list_change_cb	Calling function set list change function
	 * NULL
	 */
	parent = (GtkWidget *)(cb_data[0]);
	wlabel = (GtkWidget *)(cb_data[1]);
	combo = (GtkCombo *)(cb_data[2]);
	glist_in = (GList *)(cb_data[3]);
	max_items = (gint)(cb_data[4]);
	client_data = (gpointer)(cb_data[5]);
	func_cb = cb_data[6];
	list_change_cb = cb_data[7];

	if(combo == NULL)
	    return;

	/* Get new value string from combo's entry widget */
	new_value = gtk_entry_get_text(GTK_ENTRY(combo->entry));

	/* Make a duplicate of the recieved value or NULL if it is not
	 * available
	 */
	new_value = STRDUP(new_value);
	if(new_value == NULL)
	    return;

	/* Add the new value to the combo box list, this will only
	 * be added if value is not already in the list. Excess items
	 * may be truncated if adding this item would cause items to
	 * exceed the set max items on the combo box
	 *
	 * List change callback will be called if the value was input
	 * to the combo box's list
	 */
	GUIComboAddItem((gpointer)combo, new_value);

	/* Call activate function */
	if(func_cb != NULL)
	    func_cb(
		(GtkWidget *)combo,	/* Combo */
		client_data		/* Data */
	    );

	/* Free new_value which we duplicated */
	g_free(new_value);
}

/*
 *	GtkCombo "changed" signal callback.
 */
static void GUIComboChangedCB(GtkWidget *widget, gpointer data)
{
	GList *glist;
	GtkCombo *combo;
	gpointer client_data;
	void (*list_change_cb)(GtkWidget *, gpointer, GList *);
	gpointer *cb_data = (gpointer *)data;
	if((widget == NULL) || (cb_data == NULL))
	    return;

	/* Parse callback data, format is:
	 *
	 * table		Parent table that holds combo and label
	 * label		Label (may be NULL)
	 * combo		This combo widget
	 * GList		Contains list of strings in combo list
	 * max_items		Max items allowed in combo list
	 * client_data		Calling function set callback data
	 * func_cb		Calling function set callback function
	 * list_change_cb	Calling function set list change function
	 * NULL
	 */
	combo = (GtkCombo *)(cb_data[2]);
	glist = (GList *)cb_data[3];
	client_data = cb_data[5];
	list_change_cb = cb_data[7];

	/* Call list change function */
	if(list_change_cb != NULL)
	    list_change_cb(
		(GtkWidget *)combo,	/* Combo */
		client_data,		/* Data */
		glist			/* List */
	    );
}

/*
 *	Combo data destroy callback.
 */
static void GUIComboDataDestroyCB(gpointer data)
{
	GtkWidget *parent, *wlabel;
	GtkCombo *combo;
	GList *glist;
	gint max_items;
	gpointer client_data;
	void (*func_cb)(GtkWidget *, gpointer);
	void (*list_change_cb)(GtkWidget *, gpointer, GList *);
	gpointer *cb_data = (gpointer *)data;


	if(cb_data == NULL)
	    return;

	/* Parse callback data, format is:
	 *
	 * table		Parent table that holds combo and label
	 * label		Label (may be NULL)
	 * combo		This combo widget
	 * GList		Contains list of strings in combo list
	 * max_items		Max items allowed in combo list
	 * client_data		Calling function set callback data
	 * func_cb		Calling function set callback function
	 * list_change_cb	Calling function set list change function
	 * NULL
	 */
	parent = (GtkWidget *)(cb_data[0]);
	wlabel = (GtkWidget *)(cb_data[1]);
	combo = (GtkCombo *)(cb_data[2]);
	glist = (GList *)(cb_data[3]);
	max_items = (gint)(cb_data[4]);
	client_data = (gpointer)(cb_data[5]);
	func_cb = cb_data[6];
	list_change_cb = cb_data[7];


	/* Do not call list change callback function on destroy */

	/* Begin deallocating data referenced on callback data */

	/* Delete list */
	if(glist != NULL)
	{
	    g_list_foreach(glist, (GFunc)g_free, NULL);
	    g_list_free(glist);
	    glist = NULL;
	}

	/* Delete callback data */
	g_free(data);
}


/*
 *	Local menu item activate callback, this function will
 *	take the given data pointer as a dynamically allocated
 *	(void **).
 */
static void GUIMenuItemActivateCB(gpointer data)
{
	GUIMenuItemActivateData *d = GUI_MENU_ITEM_ACTIVATE_DATA(data);
	if(d == NULL)
	    return;

	if(d->cb != NULL)
	    d->cb(
		d->w,
		d->data
	    );
}

/*
 *	Menu item client data destroy callback.
 */
static void GUIMenuItemActivateDataDestroyCB(gpointer data)
{
	GUIMenuItemActivateData *d = GUI_MENU_ITEM_ACTIVATE_DATA(data);
	if(d == NULL)
	    return;

	g_free(d);
}

/*
 *	Menu item data destroy callback.
 */
static void GUIMenuItemDataDestroyCB(gpointer data)
{
	GUIMenuItemData *d = GUI_MENU_ITEM_DATA(data);
	if(d == NULL)
	    return;

	g_free(d->label_text);
	g_free(d->accel_text);
	g_free(d);
}


/*
 *      Editable popup menu data destroy callback.
 */
static void GUIEditablePopupMenuDataDestroyCB(gpointer data)
{
	GUIEditablePopupMenuData *d = GUI_EDITABLE_POPUP_MENU_DATA(data);
	if(d == NULL)
	    return;

	d->freeze_count++;

	GTK_WIDGET_DESTROY(d->menu);
	g_free(d->undo_buf);
	g_free(d->redo_buf);

	d->freeze_count--;

	g_free(d);
}

/*
 *	Editable popup menu undo/redo callback.
 */
static void GUIEditablePopupMenuUndoCB(GtkWidget *widget, gpointer data)
{
	GUIEditablePopupMenuData *d = GUI_EDITABLE_POPUP_MENU_DATA(data);
	if(d == NULL)
	    return;

	if(d->freeze_count > 0)
	    return;

	if(!(d->flags & GUI_EDITABLE_POPUP_MENU_UNDO) ||
	   (d->flags & GUI_EDITABLE_POPUP_MENU_READ_ONLY)
	)
	    return;

	d->freeze_count++;

	if(d->editable != NULL)
	{
	    GtkEditable *editable = GTK_EDITABLE(d->editable);

	    /* Redo? */
	    if(d->redo_buf != NULL)
	    {
		gint position = 0;

		g_free(d->undo_buf);
		d->undo_buf = gtk_editable_get_chars(editable, 0, -1);
		d->undo_last_pos = gtk_editable_get_position(editable);

		gtk_editable_delete_text(editable, 0, -1);
		gtk_editable_insert_text(editable, d->redo_buf, STRLEN(d->redo_buf), &position);
		gtk_editable_set_position(editable, d->redo_last_pos);

		g_free(d->redo_buf);
		d->redo_buf = NULL;
		d->redo_last_pos = 0;
	    }
	    /* Undo? */
	    else if(d->undo_buf != NULL)
	    {
		gint position = 0;

		g_free(d->redo_buf);
		d->redo_buf = gtk_editable_get_chars(editable, 0, -1);
		d->redo_last_pos = gtk_editable_get_position(editable);

		gtk_editable_delete_text(editable, 0, -1);
		gtk_editable_insert_text(editable, d->undo_buf, STRLEN(d->undo_buf), &position);
		gtk_editable_set_position(editable, d->undo_last_pos);

		g_free(d->undo_buf);
		d->undo_buf = NULL;
		d->undo_last_pos = 0;
	    }
	    else
	    {
		g_free(d->redo_buf);
		d->redo_buf = gtk_editable_get_chars(editable, 0, -1);
		d->redo_last_pos = gtk_editable_get_position(editable);

		gtk_editable_delete_text(editable, 0, -1);
	    }

	    GUIEditablePopupMenuUpdate(d);
	}

	d->freeze_count--;
}

/*
 *      Editable popup menu cut callback.
 */
static void GUIEditablePopupMenuCutCB(GtkWidget *widget, gpointer data)
{
	GUIEditablePopupMenuData *d = GUI_EDITABLE_POPUP_MENU_DATA(data);
	if(d == NULL)
	    return;

	if(d->freeze_count > 0)
	    return;

	if(d->flags & GUI_EDITABLE_POPUP_MENU_READ_ONLY)
	    return;

	d->freeze_count++;

	GUIEditablePopupMenuRecordUndo(d);
	gtk_editable_cut_clipboard(GTK_EDITABLE(d->editable));
	GUIEditablePopupMenuUpdate(d);

	d->freeze_count--;
}

/*
 *      Editable popup menu copy callback.
 */                            
static void GUIEditablePopupMenuCopyCB(GtkWidget *widget, gpointer data)
{
	GUIEditablePopupMenuData *d = GUI_EDITABLE_POPUP_MENU_DATA(data);
	if(d == NULL)
	    return;

	if(d->freeze_count > 0)
	    return;

	d->freeze_count++;

	gtk_editable_copy_clipboard(GTK_EDITABLE(d->editable));
	GUIEditablePopupMenuUpdate(d);

	d->freeze_count--;
}

/*
 *      Editable popup menu paste callback.
 */
static void GUIEditablePopupMenuPasteCB(GtkWidget *widget, gpointer data)
{
	GUIEditablePopupMenuData *d = GUI_EDITABLE_POPUP_MENU_DATA(data);
	if(d == NULL)
	    return;

	if(d->freeze_count > 0)
	    return;

	if(d->flags & GUI_EDITABLE_POPUP_MENU_READ_ONLY)
	    return;

	d->freeze_count++;

	GUIEditablePopupMenuRecordUndo(d);
	gtk_editable_paste_clipboard(GTK_EDITABLE(d->editable));
	GUIEditablePopupMenuUpdate(d);

	d->freeze_count--;
}

/*
 *      Editable popup menu delete callback.
 */
static void GUIEditablePopupMenuDeleteCB(GtkWidget *widget, gpointer data)
{
	GUIEditablePopupMenuData *d = GUI_EDITABLE_POPUP_MENU_DATA(data);
	if(d == NULL)
	    return;

	if(d->freeze_count > 0)
	    return;

	if(d->flags & GUI_EDITABLE_POPUP_MENU_READ_ONLY)
	    return;

	d->freeze_count++;

	GUIEditablePopupMenuRecordUndo(d);
	gtk_editable_delete_selection(GTK_EDITABLE(d->editable));
	GUIEditablePopupMenuUpdate(d);

	d->freeze_count--;
}

/*
 *      Editable popup menu select all callback.
 */
static void GUIEditablePopupMenuSelectAllCB(GtkWidget *widget, gpointer data)
{
	GUIEditablePopupMenuData *d = GUI_EDITABLE_POPUP_MENU_DATA(data);
	if(d == NULL)
	    return;

	if(d->freeze_count > 0)
	    return;

	d->freeze_count++;

	gtk_editable_select_region(GTK_EDITABLE(d->editable), 0, 0);

	while(gtk_events_pending() > 0)
	    gtk_main_iteration();

	gtk_editable_select_region(GTK_EDITABLE(d->editable), 0, -1);
	GUIEditablePopupMenuUpdate(d);

	d->freeze_count--;
}

/*
 *      Editable popup menu unselect all callback.
 */
static void GUIEditablePopupMenuUnselectAllCB(GtkWidget *widget, gpointer data)
{
	GUIEditablePopupMenuData *d = GUI_EDITABLE_POPUP_MENU_DATA(data);
	if(d == NULL)
	    return;

	if(d->freeze_count > 0)
	    return;
		   
	d->freeze_count++;

	gtk_editable_select_region(GTK_EDITABLE(d->editable), 0, 0);
	GUIEditablePopupMenuUpdate(d);

	d->freeze_count--;
}

/*
 *	Editable popup menu GtkEditable event signal callback.
 */
static gint GUIEditablePopupMenuEditableEventCB(
	GtkWidget *widget, GdkEvent *event, gpointer data
)
{
	gint status = FALSE;
	GdkEventKey *key;
	GdkEventButton *button;
	GUIEditablePopupMenuData *d = GUI_EDITABLE_POPUP_MENU_DATA(data);
	if((widget == NULL) || (event == NULL) || (d == NULL))
	    return(status);

	if(d->freeze_count > 0)
	    return(status);

	/* If the event is from a GtkSpinButton then make sure that
	 * the event's GdkWindow is the GtkSpinButton GtkEntry's
	 * text area GdkWindow, this is needed to make sure that the
	 * event did not occure on any of the GtkSpinButton's buttons
	 */
	if(GTK_IS_SPIN_BUTTON(widget))
	{
	    GdkEventAny *any = (GdkEventAny *)event;
	    GtkEntry *entry = GTK_ENTRY(widget);
	    if(entry->text_area != any->window)
		return(status);
	}

	d->freeze_count++;

	switch((gint)event->type)
	{
	  case GDK_KEY_PRESS:
	    key = (GdkEventKey *)event;
	    if(d->flags & GUI_EDITABLE_POPUP_MENU_UNDO)
	    {
		const guint keyval = key->keyval;
		if(isprint(keyval) || (keyval == GDK_BackSpace) ||
		   (keyval == GDK_Delete) || (keyval == GDK_Return) ||
		   (keyval == GDK_KP_Enter) || (keyval == GDK_ISO_Enter)
		)
		{
		    GUIEditablePopupMenuRecordUndo(d);
		    GUIEditablePopupMenuUpdate(d);
		}
	    }
	    break;

	  case GDK_BUTTON_PRESS:
	    button = (GdkEventButton *)event;
	    switch(button->button)
	    {
	      case GDK_BUTTON3:
		if(d->menu != NULL)
		{
		    /* Update the menu items */
		    GUIEditablePopupMenuUpdate(d);

		    /* Map the popup menu */
		    gtk_menu_popup(  
			GTK_MENU(d->menu),
			NULL, NULL, NULL, d,
			button->button, button->time
		    );

		    /* Need to mark the GtkText button as 0 or else it
		     * will keep marking
		     */
		    if(GTK_IS_TEXT(widget))
		    {
			GtkText *text = GTK_TEXT(widget);
			text->button = 0;
			gtk_grab_remove(widget);
			gtk_signal_emit_stop_by_name(
			    GTK_OBJECT(widget), "button_press_event"
			);
		    }
		    else if(GTK_IS_ENTRY(widget))
		    {
			GtkEntry *entry = GTK_ENTRY(widget);
			entry->button = 0;
			gtk_grab_remove(widget);
			gtk_signal_emit_stop_by_name(
			    GTK_OBJECT(widget), "button_press_event"
			);
		    }
		    status = TRUE;
		}
		break;
	      case GDK_BUTTON4:
		if(GTK_IS_SPIN_BUTTON(widget))
		{
		    GtkEntry *entry = GTK_ENTRY(widget);
		    GtkSpinButton *spin = GTK_SPIN_BUTTON(widget);
		    GtkAdjustment *adj = gtk_spin_button_get_adjustment(spin);
		    gtk_spin_button_spin(
			spin,
			GTK_SPIN_STEP_FORWARD,
			adj->step_increment
		    );
		    entry->button = 0;
		    gtk_grab_remove(widget);
		    gtk_signal_emit_stop_by_name(
			GTK_OBJECT(widget), "button_press_event"
		    );
		    status = TRUE;
		}
		break;
	      case GDK_BUTTON5:
		if(GTK_IS_SPIN_BUTTON(widget))
		{
		    GtkEntry *entry = GTK_ENTRY(widget);
		    GtkSpinButton *spin = GTK_SPIN_BUTTON(widget);
		    GtkAdjustment *adj = gtk_spin_button_get_adjustment(spin);
		    gtk_spin_button_spin(
			spin,
			GTK_SPIN_STEP_BACKWARD,
			adj->step_increment
		    );
		    entry->button = 0;
		    gtk_grab_remove(widget);
		    gtk_signal_emit_stop_by_name(
			GTK_OBJECT(widget), "button_press_event"
		    );
		    status = TRUE;
		}
		break;
	    }
	    break;

	  case GDK_BUTTON_RELEASE:
	    button = (GdkEventButton *)event;
	    switch(button->button)
	    {
	      case GDK_BUTTON3:
		if(d->menu != NULL)
		{
		    /* Need to mark the GtkText button as 0 or else it
		     * will keep marking
		     */
		    if(GTK_IS_TEXT(widget))
		    {
			GtkText *text = GTK_TEXT(widget);
			text->button = 0;
			gtk_grab_remove(widget);
			gtk_signal_emit_stop_by_name(
			    GTK_OBJECT(widget), "button_release_event"
			);
		    }
		    else if(GTK_IS_ENTRY(widget))
		    {
			GtkEntry *entry = GTK_ENTRY(widget);
			entry->button = 0;
			gtk_grab_remove(widget);
			gtk_signal_emit_stop_by_name(
			    GTK_OBJECT(widget), "button_release_event"
			);
		    }
		    status = TRUE;
		}
		break;
	      case GDK_BUTTON4:
		if(GTK_IS_SPIN_BUTTON(widget))
		{
		    GtkEntry *entry = GTK_ENTRY(widget);
		    entry->button = 0;
		    gtk_grab_remove(widget);
		    gtk_signal_emit_stop_by_name(
			GTK_OBJECT(widget), "button_release_event"
		    );
		    status = TRUE;
		}
		break;
	      case GDK_BUTTON5:
		if(GTK_IS_SPIN_BUTTON(widget))
		{
		    GtkEntry *entry = GTK_ENTRY(widget);
		    entry->button = 0;
		    gtk_grab_remove(widget);
		    gtk_signal_emit_stop_by_name(
			GTK_OBJECT(widget), "button_release_event"
		    );
		    status = TRUE;
		}
		break;
	    }
	    break;
	}

	d->freeze_count--;

	return(status);
}


/*
 *	Pullout data destroy callback.
 */
static void GUIPullOutDataDestroyCB(gpointer data)
{
	GtkWidget *client_hbox, *holder_hbox, *toplevel;
	gpointer *cb_data = (gpointer *)data;

	if(cb_data != NULL)
	{
	    /* Format is as follows (12 arguments):
	     *
	     *	client_hbox
	     *	holder_hbox
	     *	holder_window
	     *	holder_window_x
	     *	holder_window_y
	     *	holder_window_width
	     *	holder_window_height
	     *	in_place		(1 if true)
	     *	pull_out_client_data
	     *	pull_out_cb
	     *	push_in_client_data
	     *	push_in_cb
	     */
	    client_hbox = (GtkWidget *)cb_data[0];
	    holder_hbox = (GtkWidget *)cb_data[1];
	    toplevel = (GtkWidget *)cb_data[2];

	    /* Destroy the toplevel window */
	    if(toplevel != NULL)
	    {
		GTK_WIDGET_DESTROY(toplevel)
		cb_data[2] = toplevel = NULL;
	    }
	}

	g_free(cb_data);
}

/*
 *	Pull out draw handle for signal "draw".
 */
static void GUIPullOutDrawHandleDrawCB(
	GtkWidget *widget, GdkRectangle *area, gpointer data  
)
{
	GdkWindow *window;
	GtkStyle *style;

	if(widget == NULL)
	    return;

	if(GTK_WIDGET_NO_WINDOW(widget))
	    return;

	window = widget->window;
	style = gtk_widget_get_style(widget);
	if((window == NULL) || (style == NULL))
	    return;

	gdk_window_clear(window);
#ifndef _WIN32
	gtk_draw_handle(
	    style,
	    window,
	    GTK_STATE_NORMAL,
	    GTK_SHADOW_OUT,
	    0, 0,
	    widget->allocation.width,
	    widget->allocation.height,
	    GTK_ORIENTATION_HORIZONTAL
	);
#endif
}

/*
 *	Pull out draw handle callback for signal "expose_event".
 *
 *	This redraws the handle graphics on the given widget.
 */
static gint GUIPullOutDrawHandleCB(
	GtkWidget *widget, GdkEvent *event, gpointer data
)
{
	if((widget != NULL) ? GTK_WIDGET_NO_WINDOW(widget) : TRUE)
	    return(TRUE);
	gtk_widget_queue_draw(widget);
	return(TRUE);
}

/*
 *	Pull out button callback.
 */
static gint GUIPullOutPullOutBtnCB(
	GtkWidget *widget, GdkEvent *event, gpointer data
)
{
	GtkWidget *client_hbox, *holder_hbox, *toplevel;
	gpointer *cb_data = (gpointer *)data;
	gint holder_window_width, holder_window_height;
	gpointer pull_out_client_data;
	void (*pull_out_cb)(gpointer, gpointer);

	if(cb_data != NULL)
	{
	    /* Format is as follows (12 arguments):
	     *
	     *	client_hbox
	     *	holder_hbox
	     *	holder_window
	     *	holder_window_x
	     *	holder_window_y
	     *	holder_window_width
	     *	holder_window_height
	     *	in_place		(1 if true).
	     *	pull_out_client_data
	     *	pull_out_cb
	     *	push_in_client_data
	     *	push_in_cb
	     */
	    client_hbox = (GtkWidget *)cb_data[0];
	    holder_hbox = (GtkWidget *)cb_data[1];
	    toplevel = (GtkWidget *)cb_data[2];

	    holder_window_width = (gint)cb_data[5];
	    holder_window_height = (gint)cb_data[6];

	    pull_out_client_data = cb_data[8];
	    pull_out_cb = cb_data[9];

	    /* Create toplevel window as needed */
	    if(toplevel == NULL)
	    {
		GtkWidget *w;

		toplevel = w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gtk_window_set_policy(GTK_WINDOW(w), TRUE, TRUE, TRUE);
		cb_data[2] = (gpointer)w;
		gtk_widget_realize(w);
		gtk_widget_set_usize(
		    w,
		    (holder_window_width <= 0) ? -1 : holder_window_width,
		    (holder_window_height <= 0) ? -1 : holder_window_height
		);
		gtk_signal_connect(
		    GTK_OBJECT(w), "delete_event",
		    GTK_SIGNAL_FUNC(GUIPullOutCloseCB),
		    (gpointer)cb_data
		);
	    }

	    /* Reparent client_hbox to toplevel window */
	    if((client_hbox != NULL) && (toplevel != NULL))
	    {
		gtk_widget_show(toplevel);

		if((GTK_WIDGET_FLAGS(client_hbox) & GTK_NO_REPARENT))
		    g_printerr("Cannot reparent.\n");
		else
		    gtk_widget_reparent(client_hbox, toplevel);
	    }

	    /* Hide holder hbox */
	    if(holder_hbox != NULL)
		gtk_widget_hide(holder_hbox);

	    /* Mark in callback data that its been `pulled out' */
	    cb_data[7] = (gpointer)0;

	    /* Call pull out callback? */
	    if(pull_out_cb != NULL)
		pull_out_cb(client_hbox, pull_out_client_data);
	}

	return(TRUE);
}

/*
 *	Close (push in) callback.
 */
static gint GUIPullOutCloseCB(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	GtkWidget *client_hbox, *holder_hbox, *toplevel;
	gpointer *cb_data = (gpointer *)data;
	gpointer push_in_client_data;
	void (*push_in_cb)(gpointer, gpointer);

	if(cb_data != NULL)
	{
	    /* Format is as follows (12 arguments):
	     *
	     *	client_hbox
	     *	holder_hbox
	     *	holder_window
	     *	holder_window_x
	     *	holder_window_y
	     *	holder_window_width
	     *	holder_window_height
	     *	in_place		(1 if true).
	     *	pull_out_client_data
	     *	pull_out_cb
	     *	push_in_client_data
	     *	push_in_cb
	     */
	    client_hbox = (GtkWidget *)cb_data[0];
	    holder_hbox = (GtkWidget *)cb_data[1];
	    toplevel = (GtkWidget *)cb_data[2];

	    push_in_client_data = cb_data[10];
	    push_in_cb = cb_data[11];


	    /* Reparent client_hbox to holder_hbox */
	    if((client_hbox != NULL) && (holder_hbox != NULL))
	    {
		gtk_widget_show(holder_hbox);

		if((GTK_WIDGET_FLAGS(client_hbox) & GTK_NO_REPARENT))
		    g_printerr("Cannot reparent.\n");
		else
		    gtk_widget_reparent(client_hbox, holder_hbox);
	    }

	    /* Hide toplevel */
	    if(toplevel != NULL)
		gtk_widget_hide(toplevel);

	    /* Mark in callback data that its been `pushed in' */
	    cb_data[7] = (gpointer)1;

	    /* Call push in callback? */
	    if(push_in_cb != NULL)
		push_in_cb(client_hbox, push_in_client_data);
	}

	return(TRUE);
}


/*
 *	Creates a new GtkRcStyle based on the given GtkRcStyle.
 */
GtkRcStyle *GUIRCStyleCopy(const GtkRcStyle *rcstyle)
{
#if (GTK_MAJOR_VERSION >= 2)
	return((rcstyle != NULL) ? gtk_rc_style_copy(rcstyle) : NULL);
#else
	gint i;
	const GtkRcStyle *src_style = rcstyle;
	GtkRcStyle *tar_style;

	if(src_style == NULL)
	    return(NULL);

	tar_style = gtk_rc_style_new();
	tar_style->name = STRDUP(src_style->name);
	tar_style->font_name = STRDUP(src_style->font_name);
	tar_style->fontset_name = STRDUP(src_style->fontset_name);
	for(i = 0; i < 5; i++)
	{
	    tar_style->bg_pixmap_name[i] = STRDUP(src_style->bg_pixmap_name[i]);
	    tar_style->color_flags[i] = src_style->color_flags[i];
	    memcpy(&tar_style->fg[i], &src_style->fg[i], sizeof(GdkColor));
	    memcpy(&tar_style->bg[i], &src_style->bg[i], sizeof(GdkColor));
	    memcpy(&tar_style->text[i], &src_style->text[i], sizeof(GdkColor));
	    memcpy(&tar_style->base[i], &src_style->base[i], sizeof(GdkColor));
	}
	tar_style->engine = src_style->engine;
	tar_style->engine_data = src_style->engine_data;
#endif
	return(tar_style);
}

/*
 *	Sets the widget's rc style and all of its child widgets.
 */
void GUIRCStyleSetRecursive(GtkWidget *w, GtkRcStyle *rcstyle)
{
	if((w == NULL) || (rcstyle == NULL))
	    return;

	gtk_widget_modify_style(w, rcstyle);

	if(GTK_IS_CONTAINER(w))
	    gtk_container_forall(
		GTK_CONTAINER(w),
		(GtkCallback)GUIRCStyleSetRecursive, rcstyle
	    );
}


/*
 *	Parses the geometry string s.
 *
 *	Returns a set of GdkGeometryFlags flags.
 */
GdkGeometryFlags GUIParseGeometry(
	const gchar *s,
	gint *x, gint *y, gint *width, gint *height
)
{
	GdkGeometryFlags status = 0x00000000;

	if(x != NULL)
	    *x = 0;
	if(y != NULL)
	    *y = 0;
	if(width != NULL)
	    *width = 0;
	if(height != NULL)
	    *height = 0;

	if(s == NULL)
	    return(status);

	/* Seek past initial spaces and '=' deliminator (if any) */
	while(ISBLANK(*s))
	    s++;
	while(*s == '=')
	    s++;
	while(ISBLANK(*s))
	    s++;

	/* String at width and height arguments? */
	if((*s != '+') && (*s != '-'))
	{
	    /* Parse width value */
	    if((width != NULL) && (*s != '\0'))
	    {
		*width = ATOI(s);
		status |= GDK_GEOMETRY_WIDTH;
	    }

	    /* Begin seeking to next argument */
	    if(*s != '\0')
		s++;
	    while((toupper(*s) != 'X') && (*s != '\0'))
		s++;
	    while((toupper(*s) == 'X') || ISBLANK(*s))
		s++;

	    /* Parse height value */
	    if((height != NULL) && (*s != '\0'))
	    {
		*height = ATOI(s);
		status |= GDK_GEOMETRY_HEIGHT;
	    }

	    /* Begin seeking to next argument (probably an offset
	     * value)
	     */
	    if(*s != '\0')
		s++;
	    while((*s != '+') && (*s != '-') && (*s != '\0'))
		s++;
	}

	/* String seeked to the offsets arguments? */
	if((*s == '+') || (*s == '-'))
	{
	    /* Seek past '+' character as needed and get x offset
	     * value
	     */
	    if(*s == '+')
		s++;
	    if((x != NULL) && (*s != '\0'))
	    {
		*x = ATOI(s);
		status |= GDK_GEOMETRY_X;
	    }

	    /* Begin seeking to next argument */
	    if(*s != '\0')
		s++;
	    while((*s != '+') && (*s != '-') && (*s != '\0'))
		s++;

	    /* Seek past '+' character as needed and get y offset
	     * value
	     */
	    if(*s == '+')
		s++;
	    if((y != NULL) && (*s != '\0'))
	    {
		*y = ATOI(s);
		status |= GDK_GEOMETRY_Y;
	    }
	}

	return(status);
}

/*
 *	Gets the geometry of the GtkCList cell.
 *
 *	The clist specifies the GtkCList.
 *
 *	The column and row specifiy the cell.
 *
 *	The x, y, width, and height specifies the pointers to the
 *	cell geometry return. Any of these values may be NULL if
 *	the value is not requested.
 *
 *	Returns TRUE if the cell geometry was obtained or FALSE on
 *	failed match or error.
 */
gboolean GUICListGetCellGeometry(
	GtkCList *clist, const gint column, const gint row,
	gint *x, gint *y, gint *width, gint *height
)
{
	gint i, cx, cy, cwidth, cheight;
	GList *glist;
	const GtkAdjustment *hadj, *vadj;
	const GtkCListRow *row_ptr;
	const GtkCListColumn *column_ptr;

	if(x != NULL)
	    *x = 0;
	if(y != NULL)
	    *y = 0;
	if(width != NULL)
	    *width = 0;
	if(height != NULL)
	    *height = 0;

	if(clist == NULL)
	    return(FALSE);

	hadj = clist->hadjustment;
	vadj = clist->vadjustment;

	/* Given row and column in bounds? */
	if((column < 0) || (column >= clist->columns))
	    return(FALSE);
	if((row < 0) || (row >= clist->rows))
	    return(FALSE);

	/* Get cy and cheight */
	cy = 0;
	cheight = 0;
	glist = clist->row_list;
	for(i = 0; glist != NULL; i++)
	{
	    row_ptr = (const GtkCListRow *)glist->data;
	    if(row_ptr != NULL)
	    {
		const GtkCellText *cell_text_ptr;
		const GtkCellPixmap *cell_pixmap_ptr;
		const GtkCellPixText *cell_pixtext_ptr;
		const GtkCellWidget *cell_widget_ptr;

		const GtkCell *cell_ptr = row_ptr->cell;
		cheight = clist->row_height;
		if(cell_ptr != NULL)
		{
		    switch(cell_ptr->type)
		    {
		      case GTK_CELL_TEXT:
			cell_text_ptr = (GtkCellText *)cell_ptr;
			cheight = MAX(cell_text_ptr->vertical, cheight);
			break;
		      case GTK_CELL_PIXMAP:
			cell_pixmap_ptr = (GtkCellPixmap *)cell_ptr;
			cheight = MAX(cell_pixmap_ptr->vertical, cheight);
			break;
		      case GTK_CELL_PIXTEXT:
			cell_pixtext_ptr = (GtkCellPixText *)cell_ptr;
			cheight = MAX(cell_pixtext_ptr->vertical, cheight);
			break;
		      case GTK_CELL_WIDGET:
			cell_widget_ptr = (GtkCellWidget *)cell_ptr;
			cheight = MAX(cell_widget_ptr->vertical, cheight);
			break;
		      case GTK_CELL_EMPTY:
			cheight = 0;
			break;
		    }
		}
		cheight += 1;	/* Need to add 1 pixel for cell borders */

		if(i == row)
		    break;

		cy += cheight;
	    }

	    glist = g_list_next(glist);
	}

	/* Get cx and cwidth */
	cx = 0;
	cwidth = 0;
	if(clist->column != NULL)
	{
	    for(i = 0; i < clist->columns; i++)
	    {
		column_ptr = &clist->column[i];
		if(column_ptr == NULL)
		    continue;

		/* Get width of this column plus margins */
		cwidth = column_ptr->width + 7;

		if(i == column)
		    break;

		cx += cwidth;
	    }
	}

	/* Offset cx and cy with scroll adjustments */
	if(hadj != NULL)
	    cx = cx - (gint)(hadj->value - hadj->lower);
	if(vadj != NULL)
	    cy = cy - (gint)(vadj->value - vadj->lower);

	/* Update returns */
	if(x != NULL)
	    *x = cx;
	if(y != NULL)
	    *y = cy;
	if(width != NULL)
	    *width = cwidth;
	if(height != NULL)
	    *height = cheight;

	return(TRUE);
}


/*
 *	Returns the row index of the specified node.
 */
gint GUICTreeNodeRow(GtkCTree *ctree, GtkCTreeNode *node)
{
	gint row;
	GList *glist;
	GtkCList *clist;
	GtkCTreeRow *row_ptr;

	if((ctree == NULL) || (node == NULL))
	    return(-1);

	clist = GTK_CLIST(ctree);

	row_ptr = GTK_CTREE_ROW(node);
	if(row_ptr == NULL)
	    return(FALSE);

	/* Count rows until we encounter the specified node's row */
	for(glist = clist->row_list, row = 0;
	    glist != NULL;
	    glist = g_list_next(glist), row++
	)
	{
	    if(row_ptr == (GtkCTreeRow *)glist->data)
		break;
	}

	return((glist != NULL) ? row : -1);
}


/*
 *	Used by GUICTreeNodeDeltaRows().
 */
static void GUICTreeNodeDeltaRowsIterate(
	GtkCTree *ctree, GtkCTreeNode *node, GtkCTreeNode *end,
	gint *row_count, gboolean *end_found
)
{
	GtkCTreeRow *row_ptr;

	while((node != NULL) && !(*end_found))
	{
	    /* Found end node? */
	    if(node == end)
	    {
		*end_found = TRUE;
		break;
	    }

	    /* Count this node */
	    *row_count = *row_count + 1;

	    row_ptr = GTK_CTREE_ROW(node);
	    if(row_ptr == NULL)
		break;

	    /* Count child nodes only if expanded */
	    if(row_ptr->expanded && !row_ptr->is_leaf)
	    {
		GUICTreeNodeDeltaRowsIterate(
		    ctree, row_ptr->children, end, row_count, end_found
		);
		if(*end_found)
		    break;
	    }

	    /* Get next sibling */
	    node = row_ptr->sibling;
	}
}

/*
 *	Returns the number of rows between the GtkCTree nodes.
 */
gint GUICTreeNodeDeltaRows(
	GtkCTree *ctree,
	GtkCTreeNode *start,	/* Use NULL for first/toplevel node */
	GtkCTreeNode *end
)
{
	gboolean end_found = FALSE;
	gint row_count = 0;

	if((ctree == NULL) || (end == NULL))
	    return(row_count);

	if(start == NULL)
	    start = gtk_ctree_node_nth(ctree, 0);

	GUICTreeNodeDeltaRowsIterate(
	    ctree, start, end, &row_count, &end_found
	);

	return(row_count);
}

/*
 *	Returns the coordinate position for the specified GdkWindow
 *	relative to the root GdkWindow.
 */
void GUIGetWindowRootPosition(
	GdkWindow *window, gint *x, gint *y
)
{
	gint cx, cy;
	GdkWindow *cur_w = window, *root_w;

	if(x != NULL)
	    *x = 0;
	if(y != NULL)
	    *y = 0;

	if(cur_w == NULL)
	    return;

	if(gui_window_root == NULL)
	     gui_window_root = (GdkWindow *)GDK_ROOT_PARENT();
	root_w = gui_window_root;
	if(root_w == NULL)
	    return;

	while(cur_w != root_w)
	{
	    gdk_window_get_position(cur_w, &cx, &cy);
	    if(x != NULL)
		*x += cx;
	    if(y != NULL)
		*y += cy;
	    cur_w = gdk_window_get_parent(cur_w);
	}
}

/*
 *	Converts the coordinates x and y (relative to the window w)
 *	be relative to the root window.
 */
void GUIGetPositionRoot(
	GdkWindow *w, gint x, gint y, gint *rx, gint *ry
)
{
	GdkWindow *cur = w, *root_w;
	gint cx, cy;


	if(rx != NULL)
	    *rx = 0;
	if(ry != NULL)
	    *ry = 0;

	if(gui_window_root == NULL)
	     gui_window_root = (GdkWindow *)GDK_ROOT_PARENT();
	root_w = gui_window_root;

	if((cur == NULL) || (root_w == NULL))
	    return;

	while((cur != root_w) && (cur != NULL))
	{
	    gdk_window_get_position(cur, &cx, &cy);
	    x += cx;
	    y += cy;
	    cur = gdk_window_get_parent(cur);
	}

	if(rx != NULL)
	    *rx = x;
	if(ry != NULL)
	    *ry = y;
}


/*
 *	Gets the text bounds.
 */
void GUIGetTextBounds(
	GdkFont *font, const gchar *text, const gint text_length,
	GdkTextBounds *bounds
)
{
	if((font == NULL) || (text == NULL) || (bounds == NULL))
	{
	    if(bounds != NULL)
		memset(bounds, 0x00, sizeof(GdkTextBounds));
	    return;
	}

	gdk_text_extents(
	    font, text, text_length,
	    &bounds->lbearing, &bounds->rbearing, &bounds->width,
	    &bounds->ascent, &bounds->descent
	);
}

/*
 *	Gets the string bounds.
 */
void GUIGetStringBounds(
	GdkFont *font, const gchar *string,
	GdkTextBounds *bounds
)
{
	GUIGetTextBounds(font, string, STRLEN(string), bounds);
}


/*
 *	Creates a new GC
 */
GdkGC *GDK_GC_NEW(void)
{
	if(gui_window_root == NULL) 
	     gui_window_root = (GdkWindow *)GDK_ROOT_PARENT();
	return(gdk_gc_new(gui_window_root));
}


/*
 *	Blocks input to the toplevel GtkWindow.
 *
 *	The w specifies the toplevel GtkWindow requesting the input
 *	block.
 *
 *	The block specifies whether to block or unblock input.
 */
void GUIBlockInput(
	GtkWidget *w,
	const gboolean block
)
{
	GtkWidget *invis;
	GUIBlockInputData *d;

	if(w == NULL)
	    return;

	if(!GTK_IS_WINDOW(w))
	    return;

	/* Get the block input data from the GtkWindow */
	d = GUI_BLOCK_INPUT_DATA(gtk_object_get_data(
	    GTK_OBJECT(w),
	    GUI_BLOCK_INPUT_DATA_KEY
	));
	if(d == NULL)
	{
	    /* No block input data was set on this GtkWindow so create
	     * and set it for the first time
	     */
	    d = GUI_BLOCK_INPUT_DATA(g_malloc(sizeof(GUIBlockInputData)));
	    if(d == NULL)
		return;

	    d->invisible = gtk_invisible_new();
	    d->level = 0;

	    gtk_object_set_data_full(
		GTK_OBJECT(w), GUI_BLOCK_INPUT_DATA_KEY,
		d, GUIBlockInputDataDestroyCB
	    );
	}

	/* Get the GtkInvisible to grab all input */
	invis = d->invisible;
	if(invis == NULL)
	    return;

	/* Block input? */
	if(block)
	{
	    /* Increase the block input level and check if it is the
	     * first blocking level
	     */
	    d->level++;
	    if(d->level == 1)
	    {
		/* This is the first blocking level, so we need to
		 * map the GtkInvisible and grab it so that input
		 * will be blocked
		 */
		gtk_widget_show(invis);
		if(gtk_grab_get_current() != invis)
		    gtk_grab_add(invis);
	    }
	}
	else
	{
	    /* Decrease the block input level and check if it is the
	     * last level
	     */
	    d->level--;
	    if(d->level == 0)
	    {
		if(gtk_grab_get_current() == invis)
		    gtk_grab_remove(invis);
		gtk_widget_hide(invis);
	    }

	    /* Clip the level */
	    if(d->level < 0)
	    {
		g_printerr(
"GUIBlockInput(): Block level underflow to level %i.\n",
		    d->level
		);
		d->level = 0;
	    }
	}
}

/*
 *	Gets the block input level.
 */
gint GUIBlockInputGetLevel(GtkWidget *w)
{
	GUIBlockInputData *d;

	if(w == NULL)
	    return(0);

	d = GUI_BLOCK_INPUT_DATA(gtk_object_get_data(
	    GTK_OBJECT(w),
	    GUI_BLOCK_INPUT_DATA_KEY
	));
	if(d == NULL)
	    return(0);

	return(d->level);
}


/*
 *	Gets the GdkWindow's reference count.
 */
gint GUIWindowGetRefCount(GdkWindow *window)
{
#if defined(_WIN32)
	return((window != NULL) ? 1 : 0);
#else
	return((window != NULL) ?
	    ((GdkWindowPrivate *)window)->ref_count : 0
	);
#endif
}


/*
 *	Sets the WM icon for the GdkWindow w based on the specified XPM
 *	data.
 */
void GUISetWMIcon(GdkWindow *window, guint8 **data)
{
	GdkBitmap *mask;
	GdkPixmap *pixmap;

	if((window == NULL) || (data == NULL))
	    return;

	/* Load icon pixmap and mask */
	pixmap = GDK_PIXMAP_NEW_FROM_XPM_DATA(&mask, data);
	if(pixmap != NULL)
	{
	    /* Set WM icon 
	     *
	     * Note that gdk_window_set_icon() will take care of 
	     * unref'ing the pixmap and mask
	     */
	    gdk_window_set_icon(window, NULL, pixmap, mask);
	}
}

/*
 *	Sets the WM icon for the GdkWindow w based on the specified XPM
 *	file.
 */
void GUISetWMIconFile(GdkWindow *window, const gchar *filename)
{ 
	GdkBitmap *mask;
	GdkPixmap *pixmap;

	if((window == NULL) || STRISEMPTY(filename))
	    return;

	/* Load icon pixmap and mask */
	pixmap = GDK_PIXMAP_NEW_FROM_XPM_FILE(&mask, filename);
	if(pixmap != NULL)
	{
	    /* Set WM icon
	     *
	     * Note that gdk_window_set_icon() will take care of
	     * unref'ing the pixmap and mask
	     */
	    gdk_window_set_icon(window, NULL, pixmap, mask);
	}
}


/*
 *	Applies the command line arguments to the GtkWindow.
 *
 *	Command line arguments that will be applied are as follows:
 *
 *	--title <title>			Set WM title bar title
 *	--class <class>			Set WM class
 *	--icon <filename>		Set WM icon
 *	--icon-name <name>		Set WM icon's name
 *	--font-name <font>		Set font
 *	--font
 *	-fn
 *	--foreground-color <color>	Set foreground color
 *	-fg
 *	--background-color <color>	Set background color
 *	-bg
 *	--text-color <color>		Set text color
 *	--base-color <color>		Set base color
 *	--background-pixmap <filename>	Set background pixmap
 *	--bg-pixmap
 *	--sforeground-color <color>	Set selected foreground color
 *	-sfg
 *	--sbackground-color <color>	Set selected background color
 *	-sbg
 *	--stext-color <color>		Set selected text color
 *	--sbase-color <color>		Set selected base color
 *	--sbackground-pixmap <filename>	Set selected background pixmap
 *	--sbg-pixmap
 *	--geometry <geometry>		Set geometry
 *	--iconic			Startup iconified
 *
 */
void GUIWindowApplyArgs(
	GtkWindow *w,
	const gint argc, gchar **argv
)
{
	gint i;
	gboolean arg_on;
	const gchar *arg, *argp;
	gchar		*title = NULL,
			*wmclass = NULL,
			*icon = NULL,
			*icon_name = NULL;
	GdkGeometryFlags geometry_flags = 0x00000000;
	GdkRectangle	*geometry = NULL;
	gboolean	iconic = FALSE;
	GtkRcStyle	*rcstyle = NULL;
	GdkWindow *window;

#define NEW_RCSTYLE_AS_NEEDED	{	\
if(rcstyle == NULL)			\
 rcstyle = gtk_rc_style_new();		\
}

	if((w == NULL) || (argc <= 0) || (argv == NULL))
	    return;

	if(!GTK_IS_WINDOW(GTK_WIDGET(w)) ||
	   GTK_WIDGET_NO_WINDOW(GTK_WIDGET(w))
	)
	    return;

	window = GTK_WIDGET(w)->window;
	if(window == NULL)
	    return;

	/* Iterate through arguments and gather values to be applied to
	 * the GtkWindow
	 */
	for(i = 0; i < argc; i++)
	{
	    arg = argv[i];
	    if(STRISEMPTY(arg))
		continue;

	    /* If the argument does not start with a '-' character then
	     * we can skip the checking of the argument for convience
	     */
	    if((*arg != '-') && (*arg != '+'))
		continue;

	    /* If arg is prefixed with a '+' character then set arg_on
	     * to TRUE so that toggle arguments can know to enable or
	     * disable
	     */
	    arg_on = (*arg == '+') ? TRUE : FALSE;

	    /* Get argp to seek past the '-' characters in arg */
	    argp = arg;
	    while((*argp == '-') || (*argp == '+'))
		argp++;

#define PRINT_REQUIRES_ARG(_a_)	{	\
 g_printerr(				\
  "%s: Requires argument.\n",		\
  (_a_)					\
 );					\
}

	    /* Handle by argument */

	    /* Title */
	    if(!g_strcasecmp(argp, "title"))
	    {
		i++;
		arg = (i < argc) ? argv[i] : NULL;
		if(arg != NULL)
		{
		    g_free(title);
		    title = STRDUP(arg);
		}
		else
		{
		    PRINT_REQUIRES_ARG(argv[i - 1])
		}
	    }
	    /* Class */
	    else if(!g_strcasecmp(argp, "class"))
	    {
		i++;
		arg = (i < argc) ? argv[i] : NULL;
		if(arg != NULL)
		{
		    g_free(wmclass);
		    wmclass = STRDUP(arg);
		}
		else
		{
		    PRINT_REQUIRES_ARG(argv[i - 1])
		}
	    }
	    /* Icon */
	    else if(!g_strcasecmp(argp, "icon"))
	    {
		i++;
		arg = (i < argc) ? argv[i] : NULL;
		if(arg != NULL)
		{
		    g_free(icon);
		    icon = STRDUP(arg);
		}
		else
		{
		    PRINT_REQUIRES_ARG(argv[i - 1])
		}
	    }
	    /* Icon Name */
	    else if(!g_strcasecmp(argp, "icon-name"))
	    {
		i++;
		arg = (i < argc) ? argv[i] : NULL;
		if(arg != NULL)
		{
		    g_free(icon_name);
		    icon_name = STRDUP(arg);
		}
		else
		{
		    PRINT_REQUIRES_ARG(argv[i - 1])
		}
	    }
	    /* Font */
	    else if(!g_strcasecmp(argp, "font-name") ||
		    !g_strcasecmp(argp, "font") ||
		    !g_strcasecmp(argp, "fn")
	    )
	    {
		i++;
		arg = (i < argc) ? argv[i] : NULL;
		if(arg != NULL)
		{
		    NEW_RCSTYLE_AS_NEEDED
		    g_free(rcstyle->font_name);
		    rcstyle->font_name = STRDUP(arg);
		}
		else
		{
		    PRINT_REQUIRES_ARG(argv[i - 1])
		}
	    }
	    /* Foreground Color */
	    else if(!g_strcasecmp(argp, "foreground-color") ||
		    !g_strcasecmp(argp, "foreground") ||
		    !g_strcasecmp(argp, "fg")
	    )
	    {
		i++;
		arg = (i < argc) ? argv[i] : NULL;
		if(arg != NULL)
		{
		    GdkColor *c;
		    GtkStateType state = GTK_STATE_NORMAL;
		    NEW_RCSTYLE_AS_NEEDED
		    rcstyle->color_flags[state] |= GTK_RC_FG;
		    c = &rcstyle->fg[state];
		    gdk_color_parse(arg, c);
		}
		else
		{
		    PRINT_REQUIRES_ARG(argv[i - 1])
		}
	    }
	    /* Background Color */
	    else if(!g_strcasecmp(argp, "background-color") ||
		    !g_strcasecmp(argp, "background") ||
		    !g_strcasecmp(argp, "bg")
	    )
	    {
		i++;
		arg = (i < argc) ? argv[i] : NULL;
		if(arg != NULL)
		{
		    GdkColor *c;
		    GtkStateType state = GTK_STATE_NORMAL;
		    NEW_RCSTYLE_AS_NEEDED
		    rcstyle->color_flags[state] |= GTK_RC_BG;
		    c = &rcstyle->bg[state];
		    gdk_color_parse(arg, c);
		}
		else
		{
		    PRINT_REQUIRES_ARG(argv[i - 1])
		}
	    }
	    /* Text Color */
	    else if(!g_strcasecmp(argp, "text-color"))
	    {
		i++;
		arg = (i < argc) ? argv[i] : NULL;
		if(arg != NULL)
		{
		    GdkColor *c;
		    GtkStateType state = GTK_STATE_NORMAL;
		    NEW_RCSTYLE_AS_NEEDED
		    rcstyle->color_flags[state] |= GTK_RC_TEXT;
		    c = &rcstyle->text[state];
		    gdk_color_parse(arg, c);
		}
		else
		{
		    PRINT_REQUIRES_ARG(argv[i - 1])
		}
	    }
	    /* Base Color */
	    else if(!g_strcasecmp(argp, "base-color"))
	    {
		i++;
		arg = (i < argc) ? argv[i] : NULL;
		if(arg != NULL)
		{
		    GdkColor *c;
		    GtkStateType state = GTK_STATE_NORMAL;
		    NEW_RCSTYLE_AS_NEEDED
		    rcstyle->color_flags[state] |= GTK_RC_BASE;
		    c = &rcstyle->base[state];
		    gdk_color_parse(arg, c);
		}
		else
		{
		    PRINT_REQUIRES_ARG(argv[i - 1])
		}
	    }
	    /* Background Pixmap */
	    else if(!g_strcasecmp(argp, "background-pixmap") ||
		    !g_strcasecmp(argp, "bg-pixmap")
	    )
	    {
		i++;
		arg = (i < argc) ? argv[i] : NULL;
		if(arg != NULL)
		{
		    GtkStateType state = GTK_STATE_NORMAL;
		    NEW_RCSTYLE_AS_NEEDED
		    g_free(rcstyle->bg_pixmap_name[state]);
		    rcstyle->bg_pixmap_name[state] = STRDUP(arg);
		}
		else
		{
		    PRINT_REQUIRES_ARG(argv[i - 1])
		}
	    }
	    /* Selected Foreground Color */
	    else if(!g_strcasecmp(argp, "sforeground-color") ||
		    !g_strcasecmp(argp, "sforeground") ||
		    !g_strcasecmp(argp, "sfg")
	    )
	    {
		i++;
		arg = (i < argc) ? argv[i] : NULL;
		if(arg != NULL)
		{
		    GdkColor *c;
		    GtkStateType state = GTK_STATE_SELECTED;
		    NEW_RCSTYLE_AS_NEEDED
		    rcstyle->color_flags[state] |= GTK_RC_FG;
		    c = &rcstyle->fg[state];
		    gdk_color_parse(arg, c);
		}
		else
		{
		    PRINT_REQUIRES_ARG(argv[i - 1])
		}
	    }
	    /* Selected Background Color */
	    else if(!g_strcasecmp(argp, "sbackground-color") ||
		    !g_strcasecmp(argp, "sbackground") ||
		    !g_strcasecmp(argp, "sbg")
	    )
	    {
		i++;
		arg = (i < argc) ? argv[i] : NULL;
		if(arg != NULL)
		{
		    GdkColor *c;
		    GtkStateType state = GTK_STATE_SELECTED;
		    NEW_RCSTYLE_AS_NEEDED                  
		    rcstyle->color_flags[state] |= GTK_RC_BG;
		    c = &rcstyle->bg[state];
		    gdk_color_parse(arg, c);
		}
		else
		{
		    PRINT_REQUIRES_ARG(argv[i - 1])
		}
	    }
	    /* Selected Text Color */
	    else if(!g_strcasecmp(argp, "stext-color"))
	    {
		i++;
		arg = (i < argc) ? argv[i] : NULL;
		if(arg != NULL)
		{
		    GdkColor *c;
		    GtkStateType state = GTK_STATE_SELECTED;
		    NEW_RCSTYLE_AS_NEEDED
		    rcstyle->color_flags[state] |= GTK_RC_TEXT;
		    c = &rcstyle->text[state];
		    gdk_color_parse(arg, c);
		}
		else
		{
		    PRINT_REQUIRES_ARG(argv[i - 1])
		}
	    }
	    /* Selected Base Color */
	    else if(!g_strcasecmp(argp, "sbase-color"))
	    {
		i++;
		arg = (i < argc) ? argv[i] : NULL;
		if(arg != NULL)
		{
		    GdkColor *c;
		    GtkStateType state = GTK_STATE_SELECTED;
		    NEW_RCSTYLE_AS_NEEDED
		    rcstyle->color_flags[state] |= GTK_RC_BASE;
		    c = &rcstyle->base[state];
		    gdk_color_parse(arg, c);
		}
		else
		{
		    PRINT_REQUIRES_ARG(argv[i - 1])
		}
	    }
	    /* Selected Background Pixmap */
	    else if(!g_strcasecmp(argp, "sbackground-pixmap") ||
		    !g_strcasecmp(argp, "sbg-pixmap")
	    )
	    {
		i++;
		arg = (i < argc) ? argv[i] : NULL;
		if(arg != NULL)
		{
		    GtkStateType state = GTK_STATE_SELECTED;
		    NEW_RCSTYLE_AS_NEEDED
		    g_free(rcstyle->bg_pixmap_name[state]);
		    rcstyle->bg_pixmap_name[state] = STRDUP(arg);
		}
		else
		{
		    PRINT_REQUIRES_ARG(argv[i - 1])
		}
	    }

	    /* Geometry */
	    else if(!g_strcasecmp(argp, "geometry"))
	    {
		i++;
		arg = (i < argc) ? argv[i] : NULL;
		if(arg != NULL)
		{
		    gint x, y, width, height;
		    const GdkGeometryFlags flags = gdk_parse_geometry(
			arg,
			&x, &y,
			&width, &height
		    );
		    geometry_flags = flags;
		    if(flags != 0)
		    {
			GdkRectangle *rect = geometry;
			if(rect == NULL)
			    geometry = rect = (GdkRectangle *)g_malloc0(
				sizeof(GdkRectangle)
			    );
			rect->x = (flags & GDK_GEOMETRY_X) ? x : 0;
			rect->y = (flags & GDK_GEOMETRY_Y) ? y : 0;
			rect->width = (flags & GDK_GEOMETRY_WIDTH) ? width : 0;
			rect->height = (flags & GDK_GEOMETRY_HEIGHT) ? height : 0;
		    }
		}
		else
		{
		    PRINT_REQUIRES_ARG(argv[i - 1])
		}
	    }
	    /* Iconic */
	    else if(!g_strcasecmp(argp, "iconic"))
	    {
		iconic = TRUE;
	    }
#undef PRINT_REQUIRES_ARG
	}


	/* Begin applying values */

	/* Set title? */
	if(title != NULL)
	{
	    gtk_window_set_title(w, title);
	}

	/* Set class? */
	if(wmclass != NULL)
	{
#if 0
TODO: Should not set this after GtkWindow has been realized
	    gtk_window_set_wmclass(w, wmclass, wmclass);
#endif
	}

	/* Set icon? */
	if(icon != NULL)
	{
	    GUISetWMIconFile(window, icon);
	}

	/* Set icon name? */
	if(icon_name != NULL)
	{
	    gdk_window_set_icon_name(window, icon_name);
	}

	/* Set geometry? */
	if((geometry_flags != 0) && (geometry != NULL))
	{
	    const GdkGeometryFlags flags = geometry_flags;
	    const GdkRectangle *rect = geometry;
	    if((flags & GDK_GEOMETRY_WIDTH) || (flags & GDK_GEOMETRY_HEIGHT))
		gtk_widget_set_usize(
		    GTK_WIDGET(w),
		    (flags & GDK_GEOMETRY_WIDTH) ? rect->width : -1,
		    (flags & GDK_GEOMETRY_HEIGHT) ? rect->height : -1
		);
	    if((flags & GDK_GEOMETRY_X) || (flags & GDK_GEOMETRY_Y))
		gtk_widget_set_uposition(
		    GTK_WIDGET(w),
		    (flags & GDK_GEOMETRY_X) ? rect->x : 0,
		    (flags & GDK_GEOMETRY_Y) ? rect->y : 0
		);
	}

	/* Iconic? */
	if(iconic)
	{
/* TODO */

	}

	/* Set rc style? */
	if(rcstyle != NULL)
	{
	    gtk_widget_modify_style_recursive(
		GTK_WIDGET(w), rcstyle
	    );
	}

	g_free(title);
	g_free(wmclass);
	g_free(icon);
	g_free(icon_name);
	GTK_RC_STYLE_UNREF(rcstyle);
	g_free(geometry);

#undef NEW_RCSTYLE_AS_NEEDED
}


/*
 *	Returns TRUE if the specified argument is one that is handled
 *	by GUIWindowApplyArgs.
 */
gboolean GUIIsWindowArg(const gchar *arg)
{
	if(arg == NULL)
	    return(FALSE);

	if(*arg != '-')
	    return(FALSE);

	/* Seek past the initial '-' characters */
	while(*arg == '-')
	    arg++;

	if(!g_strcasecmp(arg, "title") ||
	   !g_strcasecmp(arg, "class") ||
	   !g_strcasecmp(arg, "icon") ||
	   !g_strcasecmp(arg, "icon-name") ||
	   !g_strcasecmp(arg, "font-name") ||
	   !g_strcasecmp(arg, "font") ||
	   !g_strcasecmp(arg, "fn") ||
	   !g_strcasecmp(arg, "foreground-color") ||
	   !g_strcasecmp(arg, "fg") ||
	   !g_strcasecmp(arg, "background-color") ||
	   !g_strcasecmp(arg, "bg") ||
	   !g_strcasecmp(arg, "text-color") ||
	   !g_strcasecmp(arg, "base-color") ||
	   !g_strcasecmp(arg, "background-pixmap") ||
	   !g_strcasecmp(arg, "bg-pixmap") ||
	   !g_strcasecmp(arg, "sforeground-color") ||
	   !g_strcasecmp(arg, "sfg") ||
	   !g_strcasecmp(arg, "sbackground-color") ||
	   !g_strcasecmp(arg, "sbg") ||
	   !g_strcasecmp(arg, "stext-color") ||
	   !g_strcasecmp(arg, "sbase-color") ||
	   !g_strcasecmp(arg, "sbackground-pixmap") ||
	   !g_strcasecmp(arg, "sbg-pixmap") ||
	   !g_strcasecmp(arg, "geometry") ||
	   !g_strcasecmp(arg, "iconic")
	)
	    return(TRUE);
	else
	    return(FALSE);
}


/*
 *	Sets the GtkCTree's position for optimul viewing of all the
 *	nodes starting from the expanded node.
 *
 *	The node specifies the expanded parent node.
 *
 *	Returns TRUE if the position has changed.
 */
gboolean GUICTreeOptimizeExpandPosition(
	GtkCTree *ctree,
	GtkCTreeNode *node
)
{
	gboolean need_scroll = FALSE;
	gint row_height, rows_visible, nchild_rows;
/*	GtkAdjustment *vadj; */
	GtkCList *clist;
	GtkCTreeRow *row_ptr;
	GtkCTreeNode *cur_node, *last_child_node, *scroll_end_node = NULL;

	if((ctree == NULL) || (node == NULL))
	    return(FALSE);

	/* Get the clist information */
	clist = GTK_CLIST(ctree);
	if(clist->columns <= 0)
	    return(FALSE);

#if 0
	vadj = clist->vadjustment;
	if(vadj == NULL)
	    return(FALSE);
#endif

	/* Get the row height if it is set and assume uniform row
	 * spacing, otherwise assume the default to be 20
	 */
	if(GTK_CLIST_ROW_HEIGHT_SET(clist))
	    row_height = clist->row_height + 1;
	else
	    row_height = 20;
	if(row_height <= 0)
	    return(FALSE);

	/* Calculate the number of rows visible */
	rows_visible = clist->clist_window_height / row_height;
	if(rows_visible <= 0)
	    return(FALSE);

	/* Get the first child of the expanded node */
	last_child_node = NULL;
	nchild_rows = 1;		/* Include the parent */
	row_ptr = GTK_CTREE_ROW(node);
	cur_node = (row_ptr != NULL) ? row_ptr->children : NULL;
	while(cur_node != NULL)
	{
	    last_child_node = cur_node;
	    nchild_rows++;
	    if(nchild_rows > rows_visible)
	    {
		need_scroll = TRUE;
		break;
	    }

	    row_ptr = GTK_CTREE_ROW(cur_node);
	    cur_node = (row_ptr != NULL) ? row_ptr->sibling : NULL;
	}

	/* Check if last child node is not visible */
	if(!need_scroll && (last_child_node != NULL))
	{
#if 1
	    const gint	branch_start_rows = GUICTreeNodeDeltaRows(
		ctree, NULL, node
	    ),
			branch_end_rows = branch_start_rows + nchild_rows;
	    GtkCTreeNode *end_node = gtk_ctree_node_nth(
		ctree, branch_end_rows - 1
	    );
	    if(end_node != NULL)
	    {
		if(gtk_ctree_node_is_visible(
		    ctree, end_node
		) != GTK_VISIBILITY_FULL)
		{
		    /* It is not visible so mark that we need to scroll
		     * and get the end node
		     */
		    need_scroll = TRUE;
		    scroll_end_node = end_node;
		}
	    }
#else
	    const gint	branch_start_rows = GUICTreeNodeDeltaRows(
		ctree, NULL, node
	    ),
			branch_end_rows = branch_start_rows + nchild_rows;
	    const gint	y_scroll_pos = (gint)MAX(vadj->value, vadj->lower),
			height = MAX(clist->clist_window_height, 0),
			y = (branch_end_rows * row_height) - y_scroll_pos;
	    if(y > height)
	    {
		/* It is not visible so mark that we need to scroll
		 * and get the end node
		 */
		need_scroll = TRUE;
		scroll_end_node = gtk_ctree_node_nth(ctree, branch_end_rows - 1);
	    }
#endif
	}

	/* Need to scroll? */
	if(need_scroll)
	{
	    if(scroll_end_node != NULL)
		gtk_ctree_node_moveto(
		    ctree,
		    scroll_end_node, -1,
		    1.0f, 0.0f	/* Row align, column align */
		);
	    else
		gtk_ctree_node_moveto(
		    ctree,
		    node, -1,
		    0.0f, 0.0f	/* Row align, column align */
		);
	    return(TRUE);
	}
	else
	{
	    return(FALSE);
	}
}


/*
 *	Creates a bitmap from the given RGBA data
 *
 *	Only the alpha byte will be used to create the bitmap.
 *
 *	An alpha byte value greater than or equal to the value
 *	specified by threshold will set a 1 bit in the bitmap and a
 *	value less will set a 0 bit.
 *
 *	If bytes per line bpl is -1, then it will be automatically
 *	calculated.
 */
GdkBitmap *GUICreateBitmapFromDataRGBA(
	const gint width, const gint height,
	const gint bpl,
	const guint8 *rgba,
	const guint8 threshold,
	GdkWindow *window
)
{
	const gint bpp = 4;
	gint		x, y,
			_bpl = bpl,
			xbm_bpl;
	guint8 *xbm_data;
	const guint8 *src_ptr;
	GdkBitmap *bitmap;

	if(gui_window_root == NULL)
	     gui_window_root = (GdkWindow *)GDK_ROOT_PARENT();

	if(window == NULL)
	    window = gui_window_root;

	if((rgba == NULL) || (width <= 0) || (height <= 0))
	    return(NULL);

	/* Calculate bytes per line, the number of bytes to be able to
	 * hold one whole line of bits
	 */
	xbm_bpl = (width / 8) + ((width % 8) ? 1 : 0);

	if(_bpl < 0)
	    _bpl = width * bpp;
	if(_bpl <= 0)
	    return(NULL);

	/* Allocate bitmap */
	xbm_data = (guint8 *)g_malloc0(
	    xbm_bpl * height * sizeof(guint8)
	);
	if(xbm_data == NULL)
	    return(NULL);

	/* Begin converting the RGBA data to the bitmap data */
	for(y = 0; y < height; y++)
	{
	    for(x = 0; x < width; x++)
	    {
		src_ptr = &rgba[
		    (y * _bpl) + (x * bpp)
		];
		/* Alpha value greater or equal to threshold? */
		if(src_ptr[3] >= threshold)
		    xbm_data[
			(y * xbm_bpl) + (x / 8)
		    ] |= (guint8)(1 << (x % 8));
	    }
	}

	/* Create bitmap */
	bitmap = gdk_bitmap_create_from_data(
	    window, xbm_data,
	    width, height
	);
	g_free(xbm_data);	/* No longer needed */

	return(bitmap);
}


/*
 *	Creates a new GdkPixmap of the specified size and current
 *	depth.
 */
GdkPixmap *GDK_PIXMAP_NEW(gint width, gint height)
{
	if(gui_window_root == NULL)
	     gui_window_root = (GdkWindow *)GDK_ROOT_PARENT();

	if((width <= 0) || (height <= 0))
	    return(NULL);

	return(gdk_pixmap_new(gui_window_root, width, height, -1));
}

/*
 *	Creates a new GdkPixmap from XPM data.
 */
GdkPixmap *GDK_PIXMAP_NEW_FROM_XPM_DATA(
	GdkBitmap **mask_rtn,
	guint8 **data
)
{
	GdkPixmap *pixmap;
	GdkBitmap *mask;
	GdkWindow *window;

	if(mask_rtn != NULL)
	    *mask_rtn = NULL;

	if(data == NULL)
	    return(NULL);

	if(gui_window_root == NULL)
	     gui_window_root = (GdkWindow *)GDK_ROOT_PARENT();

	window = gui_window_root;

	/* Create a new GdkPixmap from the XPM data */
	pixmap = gdk_pixmap_create_from_xpm_d(
	    window,
	    &mask,
	    NULL,				/* No background color */
	    (gchar **)data
	);
	if(pixmap == NULL)
	    return(NULL);

	if(mask_rtn != NULL)
	    *mask_rtn = mask;
	else
	    GDK_BITMAP_UNREF(mask);

	return(pixmap);
}

/*
 *	Creates a new GdkPixmap from an XPM file.
 */
GdkPixmap *GDK_PIXMAP_NEW_FROM_XPM_FILE(
	GdkBitmap **mask_rtn,
	const gchar *filename
)
{
	GdkPixmap *pixmap;
	GdkBitmap *mask;
	GdkWindow *window;

	if(mask_rtn != NULL)
	    *mask_rtn = NULL;

	if(STRISEMPTY(filename))
	    return(NULL);

	if(gui_window_root == NULL)
	     gui_window_root = (GdkWindow *)GDK_ROOT_PARENT();

	window = gui_window_root;

	/* Create a new GdkPixmap from the XPM file */
	pixmap = gdk_pixmap_create_from_xpm(
	    window,
	    &mask,
	    NULL,				/* No background color */
	    filename
	);
	if(pixmap == NULL)
	    return(NULL);

	if(mask_rtn != NULL)
	    *mask_rtn = mask;
	else
	    GDK_BITMAP_UNREF(mask);

	return(pixmap);
}

/*
 *	Creates a new GtkPixmap from XPM data.
 */
GtkWidget *gtk_pixmap_new_from_xpm_d(
	GdkWindow *window,
	GdkColor *transparent_color,
	guint8 **data
)
{
	GdkPixmap *pixmap;
	GdkBitmap *mask;
	GtkWidget *w;

	if(gui_window_root == NULL)
	     gui_window_root = (GdkWindow *)GDK_ROOT_PARENT();

	if(window == NULL)
	    window = gui_window_root;

	/* Create a new GdkPixmap from the XPM data */
	pixmap = gdk_pixmap_create_from_xpm_d(
	    window,
	    &mask,
	    transparent_color,
	    (gchar **)data
	);
	if(pixmap != NULL)
	{
	    /* Create a new GtkPixmap from the GdkPixmap */
	    w = gtk_pixmap_new(pixmap, mask);
	}
	else
	{
	    w = NULL;
	}

	/* Unref the pixmap */
	GDK_PIXMAP_UNREF(pixmap);
	GDK_BITMAP_UNREF(mask);

	return(w);
}

/*
 *	Creates a new GtkPixmap from XPM data.
 */
GtkWidget *GUICreateMenuItemIcon(guint8 **icon_data)
{
	GdkWindow *window;
	GtkWidget *w = NULL;

	if(gui_window_root == NULL)
	     gui_window_root = (GdkWindow *)GDK_ROOT_PARENT();

	window = gui_window_root;

	if(icon_data != NULL)
	{
	    /* Create a new GdkPixmap from XPM data */
	    GdkBitmap *mask;
	    GdkPixmap *pixmap = gdk_pixmap_create_from_xpm_d(
	        window,
		&mask,
		NULL,
	        (gchar **)icon_data
	    );
	    if(pixmap != NULL)
	    {
		/* Create a new GtkPixmap from the pixmap */
		w = gtk_pixmap_new(pixmap, mask);

		/* Get the size of the new GtkPixmap */
/*		gdk_window_get_size((GdkWindow *)pixmap, &width, &height); */

		GDK_PIXMAP_UNREF(pixmap);
		GDK_BITMAP_UNREF(mask);
	    }
	}
	else
	{
	    GdkBitmap *mask;
	    GdkPixmap *pixmap;

	    /* Set the width and height of the empty icon */
	    const gint	width = GUI_MENU_ITEM_DEF_HEIGHT,
			height = GUI_MENU_ITEM_DEF_HEIGHT;

	    /* Create an empty GdkBitmap */
	    gchar *bitmap_data = (gchar *)g_malloc0(
		(((width / 8) + 1) * height) * sizeof(gchar)
	    );
	    if(bitmap_data != NULL)
		mask = gdk_bitmap_create_from_data(
		    window,
		    bitmap_data,
		    width, height
		);
	    else
		mask = NULL;

	    g_free(bitmap_data);

	    /* Create an empty GdkPixmap */
	    pixmap = gdk_pixmap_new(
		window,
		width, height,
		-1
	    );
	    if(pixmap != NULL)
	    {
		/* Create a GtkPixmap based on new pixmap and mask pair */
		w = gtk_pixmap_new(pixmap, mask);

		GDK_PIXMAP_UNREF(pixmap);
		GDK_BITMAP_UNREF(mask);
	    }
	}

	return(w);
}


/*
 *	Adds a ref count to the GtkObject.
 */
GtkObject *GUIObjectRef(GtkObject *o)
{
	if(o == NULL)
	    return(NULL);

	gtk_object_ref(o);

	return(o);
}


/*
 *	Sets the GtkAdjutment's value.
 *
 *	The adj specifies the GtkAdjustment.
 *
 *	The v specifies the value, this value will be clamped to the
 *	adjustment's range.
 *
 *	If emit_value_changed is TRUE then a "value_changed" signal
 *	will be emitted.
 */
void GUIAdjustmentSetValue(
	GtkAdjustment *adj,
	const gfloat v,
	const gboolean emit_value_changed
)
{
	gfloat _v = v;

	if(adj == NULL)
	    return;

	/* Clamp the value */
	if(_v > (adj->upper - adj->page_size))
	    _v = adj->upper - adj->page_size;
	if(_v < adj->lower)
	    _v = adj->lower;

	/* Change in value? */
	if(adj->value != _v)
	{
	    /* Set the new value */
	    adj->value = _v;

	    if(emit_value_changed)
		gtk_signal_emit_by_name(
		    GTK_OBJECT(adj),
		    "value_changed"
		);
	}
}


/*
 *	Maps and raises the GtkWidget.
 *
 *	If the GtkWidget is a GtkWindow then it will be raised.
 */
void GUIWidgetMapRaise(GtkWidget *w)
{
	if(w == NULL)
	    return;

	gtk_widget_show(w);
	if(GTK_IS_WINDOW(w))
	    gdk_window_raise(w->window);
}

/*
 *	Minimizes (iconifies) the GtkWindow.
 */
void GUIWindowMinimize(GtkWindow *window)
{
#ifdef _XLIB_H_
	Display *dpy;
	Window xwin;
	GdkWindow *win;
	gint scr_num;

	if(window == NULL)
	    return;

	if(!GTK_WIDGET_REALIZED(GTK_WIDGET(window)))
	    return;

	win = GTK_WIDGET(window)->window;
	if(win == NULL)
	    return;

	dpy = GDK_WINDOW_XDISPLAY(win);
	xwin = GDK_WINDOW_XWINDOW(win);
	if((dpy == NULL) || (xwin == None))
	    return;

	scr_num = DefaultScreen(dpy);
	XIconifyWindow(dpy, xwin, scr_num);

#endif	/* _XLIB_H_ */
}

/*
 *	Creates the global gui_tooltips widget for controlling the
 *	enabled/disabled state of all tooltips set by these functions.
 *
 *	When the global gui_tooltips widget is first created, the
 *	tooltips will be enabled and gui_tooltips_state will be reset
 *	to TRUE.
 */
static void GUICreateGlobalTooltips(void)
{
	/* Need to create global gui_tooltips widget? */
	if(gui_tooltips == NULL)
	{
	    GtkWidget *w = (GtkWidget *)gtk_tooltips_new();
	    gui_tooltips = (GtkTooltips *)w;
	    if(w != NULL)
	    {
		gtk_tooltips_enable(GTK_TOOLTIPS(w));
		gui_tooltips_state = TRUE;
	    }
	}
}

/*
 *	Sets the widget's tooltip.
 *
 *	The w specifies the GtkWidget.
 *
 *	The tip specifies the string describing the tooltip. If tip
 *	is NULL then the widget's tooltip will be cleared.
 */
void GUISetWidgetTip(
	GtkWidget *w,
	const gchar *tip
)
{
	if(w == NULL)
	    return;

	/* Create the global GtkTooltips as needed */
	GUICreateGlobalTooltips();
	if(gui_tooltips == NULL)
	    return;

	gtk_tooltips_set_tip(
	    gui_tooltips,
	    w,
	    tip,
	    NULL				/* No private tip */
	);
}

/*
 *	Makes the tooltip associated with the given widget shown
 *	immediatly if the widget has a tip defined. Also requires
 *	that the global gui_tooltips_state be TRUE, otherwise
 *	this function does nothing.
 */
void GUIShowTipsNow(GtkWidget *w)
{
	GdkEventCrossing e;
	GdkWindow *window;

	if((w == NULL) || !gui_tooltips_state)
	    return;

	if(GTK_WIDGET_NO_WINDOW(w))
	    return;

	window = w->window;
	if(window == NULL)
	    return;

	/* Send a fake enter notify event to make widget think
	 * its time to show the tooltips. Note that the widget
	 * should be watching for the enter_notify_event.
	 */
	e.type = GDK_ENTER_NOTIFY;
	e.window = window;
	e.send_event = 1;		/* True if we're sending event */
	e.subwindow = window;
	e.time = GDK_CURRENT_TIME;
	e.x = 0.0;
	e.y = 0.0;
	e.x_root = 0.0;
	e.y_root = 0.0;
	e.mode = GDK_CROSSING_NORMAL;
	e.detail = GDK_NOTIFY_ANCESTOR;
	e.focus = TRUE;			/* Focus */
	e.state = 0;			/* Key modifiers */
	gdk_event_put((GdkEvent *)&e);
}

/*
 *	Enables/disables the global tooltips state.
 */
void GUISetGlobalTipsState(const gboolean state)
{
	/* Create global tooltips as needed */
	GUICreateGlobalTooltips();
	if(gui_tooltips == NULL)
	    return;

	if(state)
	    gtk_tooltips_enable(gui_tooltips);
	else
	    gtk_tooltips_disable(gui_tooltips);

	/* Update global tool tips state */
	gui_tooltips_state = (state) ? TRUE : FALSE;
}


/*
 *	Changes the button layout.
 *
 *	The button must be one created by GUIButtonPixmap*()
 *	or GUIToggleButtonPixmap*().
 */
void GUIButtonChangeLayout(
	GtkWidget *w,
	const gboolean show_pixmap,
	const gboolean show_label
)
{
	GUIButtonData *d;

	if(w == NULL)
	    return;

	d = GUI_BUTTON_DATA(gtk_object_get_data(
	    GTK_OBJECT(w),
	    GUI_BUTTON_DATA_KEY
	));
	if(d == NULL)
	    return;

	w = d->label;
	if(w != NULL)
	{
	    if(show_label)
		gtk_widget_show(w);
	    else
		gtk_widget_hide(w);
	}

	w = d->pixmap;
	if(w != NULL)
	{
	    if(show_pixmap)
		gtk_widget_show(w); 
	    else
		gtk_widget_hide(w);
	}

	w = d->arrow;
	if(w != NULL)
	{
	    if(show_pixmap)
		gtk_widget_show(w);
	    else
		gtk_widget_hide(w);
	}
}

/*
 *	Underlines the specified character in the button's label.
 */
void GUIButtonLabelUnderline(
	GtkWidget *w,
	const guint c
)
{
	GUIButtonData *d;

	if((w == NULL) || (c == '\0'))
	    return;

	d = GUI_BUTTON_DATA(gtk_object_get_data(
	    GTK_OBJECT(w),
	    GUI_BUTTON_DATA_KEY
	));
	if(d == NULL)
	    return;

	if((d->label != NULL) && (d->label_text != NULL))
	{
	    GtkWidget *w = d->label;
	    const gchar *text = d->label_text;
	    gchar *s, *pattern = STRDUP(text);
	    gint i = (gint)toupper((gint)c);

	    for(s = pattern; *s != '\0'; s++)
	    {
		if((gint)toupper((gint)*s) == i)
		{
		    *s = '_';
		    i = '\0';
		}
		else
		{
		    *s = ' ';
		}
	    }

	    gtk_label_set_pattern(GTK_LABEL(w), pattern);

	    g_free(pattern);
	}
}

/*
 *	Changes the button's icon and/or label.
 *
 *	If any value is NULL then it will not be modified.
 *
 *	The button must be one returned from GUIButtonPixmap*()
 *	or GUIToggleButtonPixmap*().
 */
void GUIButtonPixmapUpdate(
	GtkWidget *w,
	guint8 **icon,
	const gchar *label
)
{
	GtkWidget *parent;
	GUIButtonData *d;

	if(w == NULL)
	    return;

	d = GUI_BUTTON_DATA(gtk_object_get_data(
	    GTK_OBJECT(w),
	    GUI_BUTTON_DATA_KEY
	));
	if(d == NULL)
	    return;

	if(gui_window_root == NULL)
	     gui_window_root = (GdkWindow *)GDK_ROOT_PARENT();

	parent = d->main_box;
	if(parent == NULL)
	    return;

	/* Update icon? */
	if((icon != NULL) &&
	   ((const guint8 **)icon != d->icon_data)
	)
	{
	    GdkWindow *window = gui_window_root;
	    if(window != NULL)
	    {
		GdkBitmap *mask;
		GdkPixmap *pixmap = gdk_pixmap_create_from_xpm_d(
		    window,
		    &mask,
		    NULL,
		    (gchar **)icon
		);
		if(pixmap != NULL)
		{
		    gint width, height;
		    gdk_window_get_size(pixmap, &width, &height);

		    /* Record icon data pointer */
		    d->icon_data = (const guint8 **)icon;

		    /* Set new icon pixmap */
		    w = d->pixmap;
		    if(w != NULL)
		    {
			gtk_pixmap_set(GTK_PIXMAP(w), pixmap, mask);
		    }
		    else
		    {
			d->pixmap = w = gtk_pixmap_new(pixmap, mask);
			gtk_box_pack_start(GTK_BOX(parent), w, TRUE, FALSE, 0);
			gtk_widget_show(w);
		    }
		    if(w != NULL)
		    {
			gtk_widget_set_usize(
			    w,
			    width + (2 * 0),
			    height + (2 * 0)
			);
		    }

		    /* Unref the loaded pixmap and mask pair, they are
		     * no longer needed
		     */
		    GDK_PIXMAP_UNREF(pixmap);
		    GDK_BITMAP_UNREF(mask);
		}
	    }
	}

	/* Update label text? */
	if(label != NULL)
	{
	    w = d->label;
	    if(w != NULL)
	    {
		/* Change in label text? */
		if((d->label_text != NULL) ?
		    strcmp(d->label_text, label) : TRUE
		)
		{
		    /* Set new label text */
		    g_free(d->label_text);
		    d->label_text = STRDUP(label);
		    gtk_label_set_text(GTK_LABEL(w), d->label_text);
		}
	    }
	}
}

/*
 *	Changes the button's arrow and/or label.
 */
void GUIButtonArrowUpdate(
	GtkWidget *w,
	GtkArrowType arrow_type,
	const gint arrow_width,
	const gint arrow_height,
	const gchar *label
)
{
	GtkWidget *parent;
	GUIButtonData *d;

	if(w == NULL)
	    return;

	d = GUI_BUTTON_DATA(gtk_object_get_data(
	    GTK_OBJECT(w),
	    GUI_BUTTON_DATA_KEY
	));
	if(d == NULL)
	    return;

	parent = d->main_box;
	if(parent == NULL)
	    return;

	/* Update arrow? */
	if(arrow_type >= 0)
	{
	    w = d->arrow;
	    if(w != NULL)
	    {
		if(d->arrow_type != arrow_type)
		{
		    d->arrow_type = arrow_type;
		    gtk_arrow_set(
			GTK_ARROW(w),
			d->arrow_type,
			GTK_SHADOW_OUT
		    );
		}
		gtk_widget_set_usize(w, arrow_width, arrow_height);
	    }
	}

	/* Update label text? */
	if(label != NULL)
	{
	    w = d->label;
	    if(w != NULL)
	    {
		/* Change in label text? */
		if((d->label_text != NULL) ?
		    strcmp((const char *)d->label_text, (const char *)label) : TRUE
		)
		{
		    /* Set new label text */
		    g_free(d->label_text);
		    d->label_text = STRDUP(label);
		    gtk_label_set_text(GTK_LABEL(w), d->label_text);
		}
	    }
	}
}

/*
 *	Gets the button's main GtkBox.
 */
GtkWidget *GUIButtonGetMainBox(GtkWidget *w)
{
	GUIButtonData *d;

	if(w == NULL)
	    return(NULL);

	d = GUI_BUTTON_DATA(gtk_object_get_data(
	    GTK_OBJECT(w),
	    GUI_BUTTON_DATA_KEY
	));
	if(d == NULL)
	    return(NULL);

	return(d->main_box);
}

/*
 *	Gets the button's GtkLabel.
 */
GtkWidget *GUIButtonGetLabel(GtkWidget *w)
{
	GUIButtonData *d;

	if(w == NULL)
	    return(NULL);

	d = GUI_BUTTON_DATA(gtk_object_get_data(
	    GTK_OBJECT(w),
	    GUI_BUTTON_DATA_KEY
	));
	if(d == NULL)
	    return(NULL);

	return(d->label);
}

/*
 *	Gets the button's GtkPixmap.
 */
GtkWidget *GUIButtonGetPixmap(GtkWidget *w)
{
	GUIButtonData *d;

	if(w == NULL)
	    return(NULL);

	d = GUI_BUTTON_DATA(gtk_object_get_data(
	    GTK_OBJECT(w),
	    GUI_BUTTON_DATA_KEY
	));
	if(d == NULL)
	    return(NULL);

	return(d->pixmap);
}

/*
 *	Gets the button's GtkArrow.
 */
GtkWidget *GUIButtonGetArrow(GtkWidget *w)
{
	GUIButtonData *d;

	if(w == NULL)
	    return(NULL);

	d = GUI_BUTTON_DATA(gtk_object_get_data(
	    GTK_OBJECT(w),
	    GUI_BUTTON_DATA_KEY
	));
	if(d == NULL)
	    return(NULL);

	return(d->arrow);
}

/*
 *	Gets the button's label text.
 */
const gchar *GUIButtonGetLabelText(GtkWidget *w)
{
	GUIButtonData *d;

	if(w == NULL)
	    return(NULL);

	d = GUI_BUTTON_DATA(gtk_object_get_data(
	    GTK_OBJECT(w),
	    GUI_BUTTON_DATA_KEY
	));
	if(d == NULL)
	    return(NULL);

	return(d->label_text);
}

/*
 *	GtkButton creation nexus.
 */
static GtkWidget *GUIButtonPixmapLabelHV(
	guint8 **icon,
	const gchar *label,
	GtkWidget **label_rtn,
	gboolean horizontal
)
{
	const gint border = (label != NULL) ? 1 : 0;
	GtkWidget *w, *parent;
	GUIButtonData *d;

	if(label_rtn != NULL)
	    *label_rtn = NULL;

	if(gui_window_root == NULL)
	     gui_window_root = (GdkWindow *)GDK_ROOT_PARENT();

	/* Create the button data */
	d = GUI_BUTTON_DATA(g_malloc0(
	    sizeof(GUIButtonData)
	));
	if(d == NULL)
	    return(NULL);

	/* Create button */
	d->button = w = gtk_button_new();
	gtk_object_set_data_full(
	    GTK_OBJECT(w), GUI_BUTTON_DATA_KEY,
	    d, GUIButtonDestroyCB
	);
	parent = w;

	/* Create the main GtkBox */
	if(horizontal)
	    w = gtk_hbox_new(FALSE, 0);
	else
	    w = gtk_vbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(w), 0);
	gtk_container_add(GTK_CONTAINER(parent), w);
	gtk_widget_show(w);
	d->main_box = parent = w;

	/* Create the icon? */
	if(icon != NULL)
	{
	    GdkWindow *window = gui_window_root;
	    if(window != NULL)
	    {
		GdkBitmap *mask = NULL;
		GdkPixmap *pixmap = gdk_pixmap_create_from_xpm_d(
		    window,
		    &mask,
		    NULL,
		    (gchar **)icon
		);
		if(pixmap != NULL)
		{
		    gint width, height;

		    gdk_window_get_size(
			(GdkWindow *)pixmap,
			&width, &height
		    );

		    d->icon_data = (const guint8 **)icon;

		    d->pixmap = w = gtk_pixmap_new(pixmap, mask);
		    gtk_widget_set_usize(
			w,
			width, height
		    );
		    gtk_box_pack_start(GTK_BOX(parent), w, TRUE, FALSE, border);
		    gtk_widget_show(w);

		    GDK_PIXMAP_UNREF(pixmap);
		    GDK_BITMAP_UNREF(mask);
		}
	    }
	}

	/* Create the label? */
	if(label != NULL)
	{
	    d->label_text = STRDUP(label);
	    d->label = w = gtk_label_new(d->label_text);
	    if(label_rtn != NULL)
		*label_rtn = w;
	    gtk_label_set_justify(
		GTK_LABEL(w),
		horizontal ? GTK_JUSTIFY_LEFT : GTK_JUSTIFY_CENTER
	    );
	    gtk_box_pack_start(GTK_BOX(parent), w, TRUE, FALSE, border);
	    gtk_widget_show(w);
	}

	return(d->button);
}

/*
 *	Creates a horizonal GtkButton with the specified icon and
 *	label.
 */
GtkWidget *GUIButtonPixmapLabelH(
	guint8 **icon,
	const gchar *label,
	GtkWidget **label_rtn
)
{
	return(GUIButtonPixmapLabelHV(icon, label, label_rtn, 1));
}

/*
 *	Creates a vertical GtkButton with the specified icon and
 *	label.
 */
GtkWidget *GUIButtonPixmapLabelV(
	guint8 **icon,
	const gchar *label,
	GtkWidget **label_rtn
)
{
	return(GUIButtonPixmapLabelHV(icon, label, label_rtn, 0));
}

/*
 *	Creates a GtkButton with the specified icon.
 */
GtkWidget *GUIButtonPixmap(guint8 **icon)
{
	return(GUIButtonPixmapLabelH(icon, NULL, NULL));
}


/*
 *	GtkButton with Arrow creation nexus.
 */
static GtkWidget *GUIButtonArrowLabelHV(
	const GtkArrowType arrow_type,
	const gint arrow_width, const gint arrow_height,
	const gchar *label,
	GtkWidget **label_rtn,
	const gboolean horizontal
)
{
	const gint border = (label != NULL) ? 1 : 0;
	GtkWidget *w, *parent;
	GUIButtonData *d;

	if(label_rtn != NULL)
	    *label_rtn = NULL;

	/* Create the button data */
	d = GUI_BUTTON_DATA(g_malloc0(
	    sizeof(GUIButtonData)
	));
	if(d == NULL)
	    return(NULL);

	/* GtkButton */
	d->button = w = gtk_button_new();
	gtk_object_set_data_full(
	    GTK_OBJECT(w), GUI_BUTTON_DATA_KEY,
	    d, GUIButtonDestroyCB
	);
	/* GtkArrows in GtkButtons need to have their GtkStateType
	 * manually reset in some cases, GUIButtonStateChangedCB()
	 * will manually reset the GtkArrow's GtkStateType
	 */
	gtk_signal_connect(
	    GTK_OBJECT(w), "state_changed",
	    GTK_SIGNAL_FUNC(GUIButtonStateChangedCB), d
	);
	parent = w;

	/* Main GtkBox */
	if(horizontal)
	    w = gtk_hbox_new(FALSE, 0);
	else
	    w = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(parent), w);
	gtk_widget_show(w);
	d->main_box = parent = w;

	/* GtkArrow */
	d->arrow_type = arrow_type;
	d->arrow = w = gtk_arrow_new(
	    d->arrow_type,
	    GTK_SHADOW_OUT
	);
	gtk_widget_set_usize(
	    w,
	    (arrow_width > 0) ? arrow_width : 20,
	    (arrow_height > 0) ? arrow_height : 20
	);
	gtk_box_pack_start(GTK_BOX(parent), w, TRUE, FALSE, border);
	gtk_widget_show(w);

	/* GtkLabel */
	if(label != NULL)
	{
	    d->label_text = STRDUP(label);
	    d->label = w = gtk_label_new(d->label_text);
	    if(label_rtn != NULL)
		*label_rtn = w;
	    gtk_label_set_justify(
		GTK_LABEL(w),
		horizontal ? GTK_JUSTIFY_LEFT : GTK_JUSTIFY_CENTER
	    );
	    gtk_box_pack_start(GTK_BOX(parent), w, TRUE, FALSE, border);
	    gtk_widget_show(w);
	}

	return(d->button);
}

/*
 *	Creates a horizontal GtkButton with the specified arrow and
 *	label.
 */
GtkWidget *GUIButtonArrowLabelH(
	const GtkArrowType arrow_type,
	const gint arrow_width, const gint arrow_height,
	const gchar *label,
	GtkWidget **label_rtn
)
{
	return(GUIButtonArrowLabelHV(
	    arrow_type, arrow_width, arrow_height,
	    label, label_rtn, TRUE
	));
}

/*
 *	Creates a vertical GtkButton with the specified arrow and
 *	label.
 */
GtkWidget *GUIButtonArrowLabelV(
	const GtkArrowType arrow_type,
	const gint arrow_width, const gint arrow_height,
	const gchar *label,
	GtkWidget **label_rtn
)
{
	return(GUIButtonArrowLabelHV(
	    arrow_type, arrow_width, arrow_height,
	    label, label_rtn, FALSE
	));
}

/*
 *	Creates a GtkButton with the specified arrow.
 */
GtkWidget *GUIButtonArrow(
	const GtkArrowType arrow_type,
	const gint arrow_width, const gint arrow_height
)
{
	return(GUIButtonArrowLabelHV(
	    arrow_type, arrow_width, arrow_height,
	    NULL, NULL, FALSE
	));
}


/*
 *	GtkToggleButton creation nexus.
 */
GtkWidget *GUIToggleButtonPixmapLabelHV(
	guint8 **icon,
	const gchar *label,
	GtkWidget **label_rtn,
	const gboolean horizontal
)
{
	const gint border = (label != NULL) ? 1 : 0;
	GtkWidget *w, *parent;
	GUIButtonData *d;

	if(label_rtn != NULL)
	    *label_rtn = NULL;

	if(gui_window_root == NULL)
	     gui_window_root = (GdkWindow *)GDK_ROOT_PARENT();

	/* Create the button data */
	d = GUI_BUTTON_DATA(g_malloc0(
	    sizeof(GUIButtonData)
	));
	if(d == NULL)
	    return(NULL);

	/* Create button */
	d->button = w = gtk_toggle_button_new();
	gtk_object_set_data_full(
	    GTK_OBJECT(w), GUI_BUTTON_DATA_KEY,
	    d, GUIButtonDestroyCB
	);
	parent = w;

	/* Create main box */
	if(horizontal)
	    w = gtk_hbox_new(FALSE, 0);
	else
	    w = gtk_vbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(w), 0);
	gtk_container_add(GTK_CONTAINER(parent), w);
	gtk_widget_show(w);
	d->main_box = parent = w;

	/* Create icon? */
	if(icon != NULL)
	{
	    GdkWindow *window = gui_window_root;
	    if(window != NULL)
	    {
		GdkBitmap *mask = NULL;
		GdkPixmap *pixmap = gdk_pixmap_create_from_xpm_d(
		    window,
		    &mask,
		    NULL,
		    (gchar **)icon
		);
		if(pixmap != NULL)
		{
		    gint width, height;
		    gdk_window_get_size(pixmap, &width, &height);

		    d->icon_data = (const guint8 **)icon;

		    d->pixmap = w = gtk_pixmap_new(pixmap, mask);
		    gtk_widget_set_usize(
			w,
			width + (2 * 0),
			height + (2 * 0)
		    );
		    gtk_box_pack_start(
			GTK_BOX(parent), w, TRUE, FALSE, border
		    );
		    gtk_widget_show(w);

		    GDK_PIXMAP_UNREF(pixmap);
		    GDK_BITMAP_UNREF(mask);
		}
	    }
	}

	/* Create label? */
	if(label != NULL)
	{
	    d->label_text = STRDUP(label);

	    d->label = w = gtk_label_new(d->label_text);
	    if(label_rtn != NULL)
		*label_rtn = w;
	    gtk_label_set_justify(
		GTK_LABEL(w),
		horizontal ? GTK_JUSTIFY_LEFT : GTK_JUSTIFY_CENTER
	    );
	    gtk_box_pack_start(
		GTK_BOX(parent), w, TRUE, FALSE, border
	    );
	    gtk_widget_show(w);
	}

	return(d->button);
}

/*
 *	Creates a horizonal GtkToggleButton with the specified icon and
 *	label.
 */
GtkWidget *GUIToggleButtonPixmapLabelH(
	guint8 **icon, const gchar *label, GtkWidget **label_rtn
)
{
	return(GUIToggleButtonPixmapLabelHV(icon, label, label_rtn, 1));
}

/*
 *	Creates a vertical GtkToggleButton with the specified icon and
 *	label.
 */
GtkWidget *GUIToggleButtonPixmapLabelV(
	guint8 **icon, const gchar *label, GtkWidget **label_rtn
)
{
	return(GUIToggleButtonPixmapLabelHV(icon, label, label_rtn, 0));
}

/*
 *	Creates a GtkToggleButton with the specified icon.
 */
GtkWidget *GUIToggleButtonPixmap(guint8 **icon)
{
	return(GUIToggleButtonPixmapLabelHV(icon, NULL, NULL, 0)); 
}


/*
 *	Prompt creation nexus
 */
static GtkWidget *GUIPromptBarOrBrowse(
	guint8 **icon, const gchar *label,
	gpointer *label_rtn, gpointer *entry_rtn, gpointer *browse_rtn
)
{
	const gint border_minor = 2;
	gint attach_x, attach_y;
	GtkWidget *parent, *w;

	/* Reset returns */
	if(label_rtn != NULL)
	    *label_rtn = NULL;
	if(entry_rtn != NULL)
	    *entry_rtn = NULL;
	if(browse_rtn != NULL)
	    *browse_rtn = NULL;

	if(gui_window_root == NULL)
	     gui_window_root = (GdkWindow *)GDK_ROOT_PARENT();

	/* Create prompt bar parent */
	parent = w = gtk_table_new(
	    1,
	    ((icon != NULL) ? 1 : 0) +
	    ((label != NULL) ? 1 : 0) +
	    1 +
	    ((browse_rtn != NULL) ? 1 : 0),
	    FALSE
	);
	gtk_table_set_col_spacings(GTK_TABLE(w), border_minor);
	attach_x = 0;
	attach_y = 0;

	/* Create icon */
	if(icon != NULL)
	{
	    GdkWindow *window = gui_window_root;
	    GdkBitmap *mask = NULL;
	    GdkPixmap *pixmap = gdk_pixmap_create_from_xpm_d(
		window,
		&mask,
		NULL,
		(gchar **)icon
	    );
	    if(pixmap != NULL)
	    {
		gint width, height;

		gdk_window_get_size(pixmap, &width, &height);

		attach_x = 0;
		w = gtk_pixmap_new(pixmap, mask);
		gtk_widget_set_usize(w, width, height);
		gtk_table_attach(
		    GTK_TABLE(parent), w,
		    (guint)attach_x, (guint)(attach_x + 1),
		    (guint)attach_y, (guint)(attach_y + 1),
		    0,
		    0,
		    0, 0
		);
		gtk_widget_show(w);

		GDK_PIXMAP_UNREF(pixmap);
		GDK_BITMAP_UNREF(mask);
	    }
	}

	/* Create label? */
	if(label != NULL)
	{
	    attach_x = (icon != NULL) ? 1 : 0;

	    w = gtk_label_new(label);
	    if(label_rtn != NULL)
		*label_rtn = w;
	    gtk_label_set_justify(GTK_LABEL(w), GTK_JUSTIFY_RIGHT);
	    gtk_table_attach(
		GTK_TABLE(parent), w,
		(guint)attach_x, (guint)(attach_x + 1),
		(guint)attach_y, (guint)(attach_y + 1),
		0,
		0,
		0, 0
	    );
	    gtk_widget_show(w);
	}

	/* Create text entry? */
	if(TRUE)
	{
	    attach_x = ((icon != NULL) ? 1 : 0) +
		((label != NULL) ? 1 : 0);

	    w = gtk_entry_new();
	    if(entry_rtn != NULL)
		*entry_rtn = w;
	    gtk_table_attach(
		GTK_TABLE(parent), w,
		(guint)attach_x, (guint)(attach_x + 1),
		(guint)attach_y, (guint)(attach_y + 1),
		GTK_SHRINK | GTK_EXPAND | GTK_FILL,
		0,
		0, 0
	    );
	    gtk_widget_show(w);
	}

	/* Create browse button? */
	if(browse_rtn != NULL)
	{
	    attach_x = ((icon != NULL) ? 1 : 0) +
		((label != NULL) ? 1 : 0) +
		1;

	    w = GUIButtonPixmap(
		(guint8 **)icon_browse_20x20_xpm
	    );
	    *browse_rtn = w;
	    gtk_table_attach(
		GTK_TABLE(parent), w,
		(guint)attach_x, (guint)(attach_x + 1),
		(guint)attach_y, (guint)(attach_y + 1),
		0,
		0,
		0, 0
	    );
	    gtk_widget_show(w);
	}

	return(parent);
}

/*
 *	Creates a new prompt with an icon, label, entry, and browse
 *	button.
 */
GtkWidget *GUIPromptBarWithBrowse(
	guint8 **icon, const gchar *label,
	gpointer *label_rtn, gpointer *entry_rtn, gpointer *browse_rtn,
	gpointer browse_data, void (*browse_cb)(GtkWidget *, gpointer)
)
{
	GtkWidget *parent;

	parent = GUIPromptBarOrBrowse(
	    icon, label, label_rtn, entry_rtn, browse_rtn
	);

	/* Set signal callback for the browse button */
	if((browse_rtn != NULL) && (browse_cb != NULL))
	{
	    GtkWidget *w = (GtkWidget *)*browse_rtn;
	    if(w != NULL)
		gtk_signal_connect(
		    GTK_OBJECT(w), "clicked",
		    GTK_SIGNAL_FUNC(browse_cb), browse_data
		);
	}

	return(parent);
}

/*
 *	Creates a new prompt with an icon, label, and entry.
 */
GtkWidget *GUIPromptBar(
	guint8 **icon, const gchar *label,
	gpointer *label_rtn, gpointer *entry_rtn
)
{
	return(GUIPromptBarOrBrowse(
	    icon, label, label_rtn, entry_rtn, NULL
	));
}


/*
 *	Creates a new GtkTargetEntry.
 */
GtkTargetEntry *gtk_target_entry_new(void)
{
	return((GtkTargetEntry *)g_malloc0(sizeof(GtkTargetEntry)));
}

/*
 *	COppies the GtkTargetEntry.
 */
GtkTargetEntry *gtk_target_entry_copy(const GtkTargetEntry *entry)
{
	const GtkTargetEntry *src;
	GtkTargetEntry *tar;

	if(entry == NULL)
	     return(NULL);

	src = entry;
	tar = gtk_target_entry_new();
	if(tar == NULL)
	    return(NULL);

	tar->target = STRDUP(src->target);
	tar->flags = src->flags;
	tar->info = src->info;

	return(tar);
}

/*
 *	Deletes the GtkTargetEntry.
 */
void gtk_target_entry_delete(GtkTargetEntry *entry)
{
	if(entry == NULL)
	     return;

	g_free(entry->target);
	g_free(entry);
}


/*
 *	Sets the DDE data.
 *
 *	The w specifies the owner GtkWidget that will be the owner
 *	of the data.
 *
 *	The selection_atom specifies the selection GdkAtom, if this
 *	value is GDK_NONE then GDK_SELECTION_PRIMARY will be used
 *	instead.
 *
 *	The t specifies the time stamp, this value can also be
 *	GDK_CURRENT_TIME.
 *
 *	The target_atom specifies the data type GdkAtom, this value
 *	may not be GDK_NONE.
 *
 *	The data and length specifies the data which will be transfered
 *	to the owner GtkWidget. The data and length should not be
 *	referenced again after this call.
 */
void GUIDDESetDirect(
	GtkWidget *w,
	GdkAtom selection_atom,
	const gulong t,
	GdkAtom target_atom,
	guint8 *data, const gint length
)
{
	GdkAtom type_atom = gdk_atom_intern(GUI_TYPE_NAME_STRING, FALSE);
	GUIDDEOwnerData *owner_data;

	if(selection_atom == GDK_NONE)
	    selection_atom = GDK_SELECTION_PRIMARY;

	if((w == NULL) || (target_atom == GDK_NONE) ||
	   (data == NULL) || (length <= 0)
	)
	{
	    g_free(data);
	    return;
	}

	if(GTK_WIDGET_NO_WINDOW(w))
	{
	    g_printerr(
"GUIDDESetDirect(): Failed:\
 GtkWidget %p does not have a GdkWindow.\n",
		w
	    );
	    g_free(data);
	    return;
	}

	/* Get the owner data from the GtkWidget */
	owner_data = GUI_DDE_OWNER_DATA(gtk_object_get_data(
	    GTK_OBJECT(w),
	    GUI_DDE_OWNER_DATA_KEY
	));
	if(owner_data == NULL)
	{
	    /* The owner data was not set, create it */
	    owner_data = GUI_DDE_OWNER_DATA(g_malloc0(sizeof(GUIDDEOwnerData)));
	    if(owner_data == NULL)
	    {
		g_free(data);
		return;
	    }

	    gtk_object_set_data_full(
		GTK_OBJECT(w), GUI_DDE_OWNER_DATA_KEY,
		owner_data, GUIDDEOwnerDestroyCB
	    );
	    owner_data->w = w;
	    owner_data->selection_clear_event_sigid = gtk_signal_connect(
		GTK_OBJECT(w), "selection_clear_event",
		GTK_SIGNAL_FUNC(GUIDDESelectionClearEventCB), owner_data
	    );
	    owner_data->selection_get_sigid = gtk_signal_connect(
		GTK_OBJECT(w), "selection_get",
		GTK_SIGNAL_FUNC(GUIDDESelectionGetCB), owner_data
	    );
	}

	/* Set the new target and type */
	owner_data->target_atom = target_atom;
	owner_data->type_atom = type_atom;

	/* Delete the owner's existing data and transfer the new data */
	g_free(owner_data->data);
	owner_data->data = data;
	owner_data->length = length;

	/* Set the widget as supporting the selection of the
	 * specified target
	 */
	gtk_selection_add_target(
	    w,			/* Widget */
	    selection_atom,	/* Selection */
	    target_atom,	/* Target */
	    0			/* Info */
	);

	/* Set the selection owner */
	if(!gtk_selection_owner_set(
	    w,			/* Owner GtkWidget */
	    selection_atom,	/* Selection */
	    (guint32)t		/* Time */
	))
	{
	    /* Unable to set the selection owner */
	    owner_data->target_atom = GDK_NONE;
	    owner_data->type_atom = GDK_NONE;
	    g_free(owner_data->data);
	    owner_data->data = NULL;
	    owner_data->length = 0;
	    return;
	}
}

/*
 *	Sets the DDE data.
 *
 *	The w specifies the owner GtkWidget that will be the owner
 *	of the data.
 *
 *	The selection_atom specifies the selection GdkAtom, if this
 *	value is GDK_NONE then GDK_SELECTION_PRIMARY will be used
 *	instead.
 *
 *	The t specifies the time stamp, this value can also be
 *	GDK_CURRENT_TIME.
 *
 *	The target_atom specifies the data type GdkAtom, this value
 *	may not be GDK_NONE.
 *
 *	The data and length specifies the data which will be coppied
 *	to the owner GtkWidget.
 */
void GUIDDESet(
	GtkWidget *w,
	GdkAtom selection_atom,
	const gulong t,
	GdkAtom target_atom,
	const guint8 *data, const gint length
)
{
	GUIDDESetDirect(
	    w,
	    selection_atom,
	    t,
	    target_atom,
	    g_memdup(data, length), length
	);
}

/*
 *	Gets the DDE data.
 *
 *	The w specifies the GtkWidget making the request.
 *
 *	The selection_atom specifies the selection GdkAtom, if this
 *	value is GDK_NONE then GDK_SELECTION_PRIMARY will be used
 *	instead.
 *
 *	The t specifies the time stamp, this value can also be
 *	GDK_CURRENT_TIME.
 *
 *	The target_atom specifies the data type GdkAtom, this value
 *	may not be GDK_NONE.
 *
 *	The length specifies the length of the returned data in bytes.
 *
 *	The calling function must delete the returned data.
 */
guint8 *GUIDDEGet(
	GtkWidget *w,
	GdkAtom selection_atom,
	const gulong t,
	GdkAtom target_atom,
	gint *length
)
{
	guint8 *data;
	guint selection_received_sigid;
	GtkSelectionData *selection_data;

#define DELETE_CALLBACK_DATA(_d_)	{	\
 if((_d_) != NULL) {				\
  g_free((_d_)->data);				\
  g_free(_d_);					\
 }						\
}

	if(length != NULL)
	    *length = 0;

	if(selection_atom == GDK_NONE)
	    selection_atom = GDK_SELECTION_PRIMARY;

	if((w == NULL) || (target_atom == GDK_NONE))
	    return(NULL);

	if(GTK_WIDGET_NO_WINDOW(w))
	{
	    g_printerr(
"GUIDDEGet(): Failed:\
 GtkWidget %p does not have a GdkWindow.\n",
		w
	    );
	    return(NULL);
	}

	/* Set up the callback data */
	selection_data = (GtkSelectionData *)g_malloc0(
	    sizeof(GtkSelectionData)
	);
	if(selection_data == NULL)
	    return(NULL);

	selection_data->selection = selection_atom;
	selection_data->target = target_atom;
	selection_data->type = GDK_NONE;
	selection_data->format = -1;
	selection_data->data = NULL;
	selection_data->length = -1;	/* -1 means data not received */

	/* Set up the requesting GtkWidget to receive the
	 * "selection_received" signal
	 */
	selection_received_sigid = gtk_signal_connect(
	    GTK_OBJECT(w), "selection_received",
	    GTK_SIGNAL_FUNC(GUIDDESelectionReceivedCB), selection_data
	);

	/* Request the selection data */
	if(!gtk_selection_convert(
	    w,			/* Widget */
	    selection_atom,	/* Selection */
	    target_atom,	/* Target */
	    (guint32)t		/* Time */
	))
	{
	    /* Unable to get the selection data */
	    gtk_signal_disconnect(
		GTK_OBJECT(w),
		selection_received_sigid
	    );
	    DELETE_CALLBACK_DATA(selection_data);
	    return(NULL);
	}

	/* Wait for the selection data to arrive
	 *
	 * This is only needed if the owner is on another application,
	 * otherwise gtk_selection_convert() calls the callback
	 * immediately
	 */
	while(selection_data->length < 0)
	{
	    while(gtk_events_pending() > 0)
		gtk_main_iteration();
	}

	/* Disconnect the signals */
	gtk_signal_disconnect(
	    GTK_OBJECT(w),
	    selection_received_sigid
	);

	/* Got the selection, transfer the selection's data to the
	 * return data
	 */
	data = (guint8 *)selection_data->data;
	if(length != NULL)
	    *length = selection_data->length;
	selection_data->data = NULL;
	selection_data->length = 0;

	/* Delete the callback data */
	DELETE_CALLBACK_DATA(selection_data);

	return(data);
#undef DELETE_CALLBACK_DATA
}

/*
 *	Clears the DDE from the widget.
 *
 *	The w specifies the GtkWidget who's DDE data is to be cleared.
 */
void GUIDDEClear(GtkWidget *w)
{
	GUIDDEOwnerData *owner;

	if(w == NULL)
	    return;

	/* Get the owner data from the GtkWidget */
	owner = GUI_DDE_OWNER_DATA(gtk_object_get_data(
	    GTK_OBJECT(w),
	    GUI_DDE_OWNER_DATA_KEY
	));
	if(owner == NULL)
	    return;

	/* Clear the target and type */
	owner->target_atom = GDK_NONE;
	owner->type_atom = GDK_NONE;

	/* Delete the owner's existing data and set the new data */
	g_free(owner->data);
	owner->data = NULL;
	owner->length = 0;

	/* Delete the GtkSelectionData on the owner GtkWidget */
	if(owner->w != NULL)
	    gtk_selection_remove_all(owner->w);
}


/*
 *	Sets the DDE as binary data.
 *
 *	The w specifies the owner GtkWidget that will be the owner
 *	of the data.
 *
 *	The selection_atom specifies the selection GdkAtom, if this
 *	value is GDK_NONE then GDK_SELECTION_PRIMARY will be used
 *	instead.
 *
 *	The t specifies the time stamp, this value can also be
 *	GDK_CURRENT_TIME.
 *
 *	The target_atom specifies the data type GdkAtom, this value
 *	may not be GDK_NONE.
 *
 *	The data and length specifies the binary data which will be
 *	coppied to the owner GtkWidget.
 */
void GUIDDESetBinary(
	GtkWidget *w,
	GdkAtom selection_atom,
	const gulong t,
	const guint8 *data, const gint length
)
{
	GUIDDESet(
	    w,
	    selection_atom,
	    t,
	    gdk_atom_intern(GUI_TARGET_NAME_STRING, FALSE),
	    data, length
	);
}

/*
 *	Gets the DDE binary data.
 *
 *	The w specifies the GtkWidget making the request.
 *
 *	The selection_atom specifies the selection GdkAtom, if this
 *	value is GDK_NONE then GDK_SELECTION_PRIMARY will be used
 *	instead.
 *
 *	The t specifies the time stamp, this value can also be
 *	GDK_CURRENT_TIME.
 *
 *	The length specifies the length of the returned data in bytes.
 *
 *	The calling function must delete the returned data.
 */
guint8 *GUIDDEGetBinary(
	GtkWidget *w,
	GdkAtom selection_atom,
	const gulong t,
	gint *length
)
{
	return(GUIDDEGet(
	    w,
	    selection_atom,
	    t,
	    gdk_atom_intern(GUI_TARGET_NAME_STRING, FALSE),
	    length
	));
}


/*
 *	Sets the DDE as a string.
 *
 *	The w specifies the owner GtkWidget that will be the owner
 *	of the data.
 *
 *	The selection_atom specifies the selection GdkAtom, if this
 *	value is GDK_NONE then GDK_SELECTION_PRIMARY will be used
 *	instead.
 *
 *	The t specifies the time stamp, this value can also be
 *	GDK_CURRENT_TIME.
 *
 *	The target_atom specifies the data type GdkAtom, this value
 *	may not be GDK_NONE.
 *
 *	The data specifies the string which will be coppied to the
 *	owner GtkWidget.
 */
void GUIDDESetString(
	GtkWidget *w,
	GdkAtom selection_atom,
	const gulong t,
	const gchar *data
)
{
	GUIDDESet(
	    w,
	    selection_atom,
	    t,
	    gdk_atom_intern(GUI_TARGET_NAME_STRING, FALSE),
	    (const guint8 *)data, STRLEN(data)
	);
}

/*
 *	Gets the DDE binary data.
 *
 *	The w specifies the GtkWidget making the request.
 *
 *	The selection_atom specifies the selection GdkAtom, if this
 *	value is GDK_NONE then GDK_SELECTION_PRIMARY will be used
 *	instead.
 *
 *	The t specifies the time stamp, this value can also be
 *	GDK_CURRENT_TIME.
 *
 *	The calling function must delete the returned data.
 */
gchar *GUIDDEGetString(
	GtkWidget *w,
	GdkAtom selection_atom,
	const gulong t
)
{
	gint length;
	gchar *s;
	guint8 *buf = GUIDDEGet(
	    w,
	    selection_atom,
	    t,
	    gdk_atom_intern(GUI_TARGET_NAME_STRING, FALSE),
	    &length
	);
	if(buf == NULL)
	    return(NULL);

	s = (gchar *)g_realloc(
	    buf,
	    (length + 1) * sizeof(gchar)
	);
	if(s != NULL)
	    s[length] = '\0';

	return(s);
}


/*
 *	Sets up widget as a DND drag source.
 *
 *	The w specifies the GtkWidget. The GtkWidget must have a
 *	GdkWindow in order for DND to work on it.
 *
 *	The types_list and ntypes specifies the DND types
 *	list, which determines what type of drags the GtkWidget will
 *	provide.
 *
 *	The allowed_actions specifies the GdkDragActions that are accepted,
 *	which can be any of GDK_ACTION_*.
 *
 *	The buttons specifies the GdkModifierTypes that are accepted,
 *	which can be any of GDK_BUTTON*_MASK.
 *
 *	The drag_begin_cb specifies the "drag_begin" signal callback.
 *
 *	The drag_data_get_cb specifies the "drag_data_get" signal
 *	callback.
 *
 *	The drag_data_delete_cb specifies the "drag_data_delete"
 *	signal callback.
 *
 *	The drag_end_cb specifies the "drag_end" signal callback.
 *
 *	The data specifies the data for all the callbacks.
 */
void GUIDNDSetSrc(
	GtkWidget *w,
	const GtkTargetEntry *types_list, const gint ntypes,
	const GdkDragAction allowed_actions,
	const GdkModifierType buttons,
	void (*drag_begin_cb)(
		GtkWidget *, GdkDragContext *,
		gpointer
	),
	void (*drag_data_get_cb)(
		GtkWidget *, GdkDragContext *,
		GtkSelectionData *,
		guint, guint,
		gpointer
	),
	void (*drag_data_delete_cb)(
		GtkWidget *, GdkDragContext *,
		gpointer
	),
	void (*drag_end_cb)(
		GtkWidget *, GdkDragContext *,
		gpointer
	),
	gpointer data
)
{
	gint i;
	GUIDNDSourceData *d;

	if((w == NULL) || (types_list == NULL) || (ntypes <= 0))
	    return;

	/* The GtkWidget must have a GdkWindow */
	if(GTK_WIDGET_NO_WINDOW(w))
	{
	    g_printerr(
"GUIDNDSetSrc(): GtkWidget %p does not have a GdkWindow.\n",
		w
	    );
	    return;
	}

	/* Get the DND source data from the GtkWidget */
	d = GUI_DND_SOURCE_DATA(gtk_object_get_data(
	    GTK_OBJECT(w),
	    GUI_DND_SOURCE_DATA_KEY
	));
	if(d == NULL)
	{
	    /* No DND source data was set on this GtkWidget so create
	     * and set it for the first time
	     */
	    d = GUI_DND_SOURCE_DATA(g_malloc0(sizeof(GUIDNDSourceData)));
	    if(d == NULL)
		return;

	    gtk_object_set_data_full(
		GTK_OBJECT(w), GUI_DND_SOURCE_DATA_KEY,
		d, GUIDNDSourceDataDestroyCB
	    );

	    /* Add this GtkWidget to the DND source GtkWidgets list */
	    gui_dnd_source_widgets_list = g_list_append(
		gui_dnd_source_widgets_list,
		w
	    );
	}

	/* Set the DND source data */
	d->widget = w;
	if(d->targets_list != NULL)
	{
	    g_list_foreach(
		d->targets_list,
		(GFunc)gtk_target_entry_delete,
		NULL
	    );
	    g_list_free(d->targets_list);
	    d->targets_list = NULL;
	}
	for(i = 0; i < ntypes; i++)
	    d->targets_list = g_list_append(
		d->targets_list,
		gtk_target_entry_copy(&types_list[i])
	    );

	/* Set the widget as a DND drag source */
	gtk_drag_source_set(
	    w,
	    buttons,
	    types_list, ntypes,
	    allowed_actions
	);

	/* Set the "drag_begin" signal callback, our local callback must
	 * be connected before GTK internal drag begin callbacks
	 */
	gtk_signal_connect_after(
	    GTK_OBJECT(w), "drag_begin",
	    GTK_SIGNAL_FUNC(GUIDNDDragBeginCB), d
	);
	if(drag_begin_cb != NULL)
	    gtk_signal_connect_after(
		GTK_OBJECT(w), "drag_begin",
		GTK_SIGNAL_FUNC(drag_begin_cb), data
	    );

	/* Set the "drag_data_get" signal callback */
	if(drag_data_get_cb != NULL)
	    gtk_signal_connect_after(
		GTK_OBJECT(w), "drag_data_get",
		GTK_SIGNAL_FUNC(drag_data_get_cb), data
	    );

	/* Set the "drag_data_delete" signal callback */
	if(drag_data_delete_cb != NULL)
	    gtk_signal_connect_after(
		GTK_OBJECT(w), "drag_data_delete",
		GTK_SIGNAL_FUNC(drag_data_delete_cb), data
	    );

	/* Set the "drag_end" signal callback */
	if(drag_end_cb != NULL)
	    gtk_signal_connect_after(
		GTK_OBJECT(w), "drag_end",
		GTK_SIGNAL_FUNC(drag_end_cb), data
	    );

	/* Set our own "drag_end" signal callback to be called after
	 * the one for the client function
	 */
	gtk_signal_connect_after(
	    GTK_OBJECT(w), "drag_end",
	    GTK_SIGNAL_FUNC(GUIDNDDragEndCB), d
	);
}

/*
 *	Changes the widget's DND source types.
 *
 *	The w specifies the GtkWidget. The GtkWidget must have a
 *	GdkWindow in order for DND to work on it. A prior call to
 *	GUIDNDSetSrc() is required to set up the GtkWidget.
 *
 *	The types_list and ntypes specifies the DND types
 *	list, which determines what type of drags the GtkWidget will
 *	provide.
 *
 *	The allowed_actions specifies the GdkDragActions that are accepted,
 *	which can be any of GDK_ACTION_*.
 *
 *	The buttons specifies the GdkModifierTypes that are accepted,
 *	which can be any of GDK_BUTTON*_MASK.
 */
void GUIDNDSetSrcTypes(
	GtkWidget *w,
	const GtkTargetEntry *types_list, const gint ntypes,
	const GdkDragAction allowed_actions,
	const GdkModifierType buttons
)
{
	GUIDNDSourceData *d;

	if(w == NULL)
	    return;

	/* Get the DND source data from the GtkWidget */
	d = GUI_DND_SOURCE_DATA(gtk_object_get_data(
	    GTK_OBJECT(w),
	    GUI_DND_SOURCE_DATA_KEY
	));
	if(d != NULL)
	{
	    gint i;

	    /* Set the DND source data */
	    if(d->targets_list != NULL)
	    {
		g_list_foreach(
		    d->targets_list,
		    (GFunc)gtk_target_entry_delete,
		    NULL
		);
		g_list_free(d->targets_list);
		d->targets_list = NULL;
	    }
	    for(i = 0; i < ntypes; i++)
		d->targets_list = g_list_append(
		    d->targets_list,
		    gtk_target_entry_copy(&types_list[i])
		);
	}

	/* Change the GtkWidget's DND source types */
	gtk_drag_source_unset(w);
	if(types_list != NULL)
	    gtk_drag_source_set(
		w,
		buttons,
		types_list, ntypes,
		allowed_actions
	    );
}

/*
 *	Sets up a widget as a DND drop target.
 *
 *	The w specifies the GtkWidget. The GtkWidget must have a
 *	GdkWindow in order for DND to work on it.
 *
 *	The types_list and ntypes specifies the DND types
 *	list, which determines what type of drops the GtkWidget will
 *	accept.
 *
 *	The allowed_actions specifies the GdkDragActions that are accepted,
 *	which can be any of GDK_ACTION_*.
 *
 *	The default_action_same specifies the default GdkDragAction
 *	if the drag originates from the same GtkWidget.
 *
 *	The default_action specifies the default GdkDragAction if the
 *	drag originates a different GtkWidget.
 *
 *	The drag_data_recieved_cb and drag_data_received_data
 *	specifies the "drag_data_received" signal callback.
 *
 *	If highlight is TRUE then automatic highlighting of the
 *	GtkWidget will be performed when a drag comes over it.
 */
void GUIDNDSetTar(
	GtkWidget *w,
	const GtkTargetEntry *types_list, const gint ntypes,
	const GdkDragAction allowed_actions,
	const GdkDragAction default_action_same,
	const GdkDragAction default_action,
	void (*drag_data_recieved_cb)(
		GtkWidget *, GdkDragContext *,
		gint, gint,
		GtkSelectionData *,
		guint,
		guint,
		gpointer     
	),
	gpointer drag_data_received_data,
	const gboolean highlight
)
{
	gint i;
	GUIDNDTargetData *d;

	if((w == NULL) || (types_list == NULL) || (ntypes <= 0))
	    return;

	/* The GtkWidget must have a GdkWindow */
	if(GTK_WIDGET_NO_WINDOW(w))
	{
	    g_printerr(
"GUIDNDSetTar(): GtkWidget %p does not have GdkWindow.\n",
		w
	    );
	    return;
	}

	/* Get the DND target data from the GtkWidget */
	d = GUI_DND_TARGET_DATA(gtk_object_get_data(
	    GTK_OBJECT(w),
	    GUI_DND_TARGET_DATA_KEY
	));
	if(d == NULL)
	{
	    /* No DND target data was set on this GtkWidget so create
	     * and set it for the first time
	     */
	    d = GUI_DND_TARGET_DATA(g_malloc0(sizeof(GUIDNDTargetData)));
	    if(d == NULL)
		return;

	    gtk_object_set_data_full(
		GTK_OBJECT(w), GUI_DND_TARGET_DATA_KEY,
		d, GUIDNDTargetDataDestroyCB
	    );
	}

	/* Set the DND target data */
	d->flags = ((highlight) ? GUI_DND_TARGET_HIGHLIGHT : 0);
	d->widget = w;
	if(d->targets_list != NULL)
	{
	    g_list_foreach(
		d->targets_list,
		(GFunc)gtk_target_entry_delete,
		NULL
	    );
	    g_list_free(d->targets_list);
	    d->targets_list = NULL;
	}
	for(i = 0; i < ntypes; i++)
	    d->targets_list = g_list_append(
		d->targets_list,
		gtk_target_entry_copy(&types_list[i])
	    );
	d->allowed_actions = allowed_actions;
	d->default_action_same = default_action_same;
	d->default_action = default_action;

	/* Set the widget as a DND drop target */
	gtk_drag_dest_set(
	    w,
	    GTK_DEST_DEFAULT_DROP,	/* Use the built-in drop handler */
	    types_list, ntypes,
	    allowed_actions
	);

	/* Connect our callback signal after so that the calling
	 * function can override the signals' affects by connecting
	 * their callbacks after ours
	 */
	if(d->drag_leave_sigid == 0)
	    d->drag_leave_sigid = gtk_signal_connect_after(
		GTK_OBJECT(w), "drag_leave",
		GTK_SIGNAL_FUNC(GUIDNDTargetDragLeaveCB), d
	    );
	if(d->drag_motion_sigid == 0)
	    d->drag_motion_sigid = gtk_signal_connect_after(
		GTK_OBJECT(w), "drag_motion",
		GTK_SIGNAL_FUNC(GUIDNDTargetDragMotionCB), d
	    );

	/* Connect the calling function's "drag_data_received"
	 * signal callback.
	 */
	if(drag_data_recieved_cb != NULL)
	    gtk_signal_connect(
		GTK_OBJECT(w), "drag_data_received",
		GTK_SIGNAL_FUNC(drag_data_recieved_cb),
		drag_data_received_data
	    );
}

/*
 *	Changes the widget's DND target types.
 *
 *	The w specifies the GtkWidget. The GtkWidget must have a
 *	GdkWindow in order for DND to work on it. A prior call to
 *	GUIDNDSetTar() is required to set up the GtkWidget.
 *
 *	The types_list and ntypes specifies the DND types
 *	list, which determines what type of drops the GtkWidget will
 *	accept.
 *
 *	The allowed_actions specifies the GdkDragActions that are accepted,
 *	which can be any of GDK_ACTION_*.
 *
 *	The default_action_same specifies the default GdkDragAction
 *	if the drag originates from the same GtkWidget.
 *
 *	The default_action specifies the default GdkDragAction if the
 *	drag originates a different GtkWidget.
 *
 *	If highlight is TRUE then automatic highlighting of the
 *	GtkWidget will be performed when a drag comes over it.
 */
void GUIDNDSetTarTypes(
	GtkWidget *w,
	const GtkTargetEntry *types_list, const gint ntypes,
	const GdkDragAction allowed_actions,
	const GdkDragAction default_action_same,
	const GdkDragAction default_action,
	const gboolean highlight
)
{
	GUIDNDTargetData *d;

	if(w == NULL)
	    return;

	/* Get the DND target data from the GtkWidget */
	d = GUI_DND_TARGET_DATA(gtk_object_get_data(
	    GTK_OBJECT(w),
	    GUI_DND_TARGET_DATA_KEY
	));
	if(d != NULL)
	{
	    gint i;

	    /* Set the DND target data */
	    d->flags = ((highlight) ? GUI_DND_TARGET_HIGHLIGHT : 0);
	    if(d->targets_list != NULL)
	    {
		g_list_foreach(
		    d->targets_list,
		    (GFunc)gtk_target_entry_delete,
		    NULL
		);
		g_list_free(d->targets_list);
		d->targets_list = NULL;
	    }
	    for(i = 0; i < ntypes; i++)
		d->targets_list = g_list_append(
		    d->targets_list,
		    gtk_target_entry_copy(&types_list[i])
		);
	    d->allowed_actions = allowed_actions;
	    d->default_action_same = default_action_same;
	    d->default_action = default_action;
	}

	/* Change the GtkWidget's DND target types */
	gtk_drag_dest_unset(w);
	if(types_list != NULL)
	    gtk_drag_dest_set(
		w,
		GTK_DEST_DEFAULT_DROP,	/* Use the built-in drop handler */
		types_list, ntypes,
		allowed_actions
	    );
}

/*
 *	Gets the DND target GtkWidget's drag actions.
 */
void GUIDNDGetTarActions(
	GtkWidget *w,
	GdkDragAction *allowed_actions_rtn,
	GdkDragAction *default_action_same_rtn,
	GdkDragAction *default_action_rtn
)
{
	GUIDNDTargetData *d;

	if(allowed_actions_rtn != NULL)
	    *allowed_actions_rtn = 0;
	if(default_action_same_rtn != NULL)
	    *default_action_same_rtn = 0;
	if(default_action_rtn != NULL)
	    *default_action_rtn = 0;

	if(w == NULL)
	    return;

	/* Get the DND target data from the GtkWidget */
	d = GUI_DND_TARGET_DATA(gtk_object_get_data(
	    GTK_OBJECT(w),
	    GUI_DND_TARGET_DATA_KEY
	));
	if(d == NULL)
	    return;

	if(allowed_actions_rtn != NULL)
	    *allowed_actions_rtn = d->allowed_actions;
	if(default_action_same_rtn != NULL)
	    *default_action_same_rtn = d->default_action_same;
	if(default_action_rtn != NULL)
	    *default_action_rtn = d->default_action;
}

/*
 *	Determines the allowed drag action from the requested actions.
 *
 *	The requested_actions specifies the requested actions.
 *
 *	The allowed_actions specifies all the actions that are allowed.
 *
 *	The default_action specifies the default action to allow when
 *	more than one of the requested actions are allowed. The
 *	default action must be in the allowed_actions list. If
 *	allowed_actions is 0 then default_action must also be 0.
 *
 *	Returns exactly one of the requested actions or 0 if none of
 *	the requested actions are allowed.
 */
GdkDragAction GUIDNDDetermineDragAction(
	GdkDragAction requested_actions,
	GdkDragAction allowed_actions,
	GdkDragAction default_action
)
{
	/* Exactly one action requested? */
	switch(requested_actions)
	{
	  case GDK_ACTION_DEFAULT:
	  case GDK_ACTION_COPY:
	  case GDK_ACTION_MOVE:
	  case GDK_ACTION_LINK:
	  case GDK_ACTION_PRIVATE:
	  case GDK_ACTION_ASK:
	    /* Is this action allowed? */
	    if(requested_actions & allowed_actions)
		return(requested_actions);
	    break;
	}

	/* Multiple actions, check if the default action is one of them */
	if(requested_actions & default_action)
	    return(default_action);
	else
	    return(0);
}

/*
 *	Checks if the GtkWidget will accept the target.
 *
 *	The w specifies the target GtkWidget.
 *
 *	The name specifies the target's name.
 *
 *	The flags specifies the GtkTargetFlags.
 *
 *	Returns TRUE if the GtkWidget will accept the target.
 */
gboolean GUIDNDIsTargetAccepted(
	GtkWidget *w,
	const gchar *name,
	const GtkTargetFlags flags
)
{
	GList *glist;
	GtkTargetEntry *entry;
	GUIDNDTargetData *d;

	if((w == NULL) || STRISEMPTY(name))
	    return(FALSE);

	/* Get the DND data from the GtkWidget */
	d = GUI_DND_TARGET_DATA(gtk_object_get_data(
	    GTK_OBJECT(w),
	    GUI_DND_TARGET_DATA_KEY
	));
	if(d == NULL)
	    return(FALSE);

	/* Check if the specified target name is in the list of targets
	 * specified on the GtkWidget
	 */
	for(glist = d->targets_list;
	    glist != NULL;
	    glist = g_list_next(glist)
	)
	{
	    entry = (GtkTargetEntry *)glist->data;
	    if(entry == NULL)
		continue;

	    if(entry->target == NULL)
		continue;

	    /* Target names match? */
	    if(!g_strcasecmp(name, entry->target))
	    {
		/* Must be the same application? */
		if(entry->flags & GTK_TARGET_SAME_APP)
		{
		    if(!(flags & GTK_TARGET_SAME_APP))
			continue;
		}

		/* Must be the same GtkWidget? */
		if(entry->flags & GTK_TARGET_SAME_WIDGET)
		{
		    if(!(flags & GTK_TARGET_SAME_WIDGET))
			continue;
		}

		return(TRUE);
	    }
	}

	return(FALSE);
}

/*
 *	Sets the DND drag icon from the selected rows on the GtkCList
 *	or the selected nodes on the GtkCTree.
 *
 *	The w specifies the GtkCTree or GtkCList.
 *
 *	The cells_list specifies a GList of GtkCellText *,
 *	GtkCellPixmap *, or GtkCellPixText * cells. Only the first 5
 *	cells will be used.
 *
 *	The nitems specifies the total number of items, which can be
 *	greater than the number of items in cells_list. If nitems
 *	is greater than the number of items in cells_list then the
 *	number of additional items will be stated by text on the
 *	drag icon. If nitems is negative then additional items will
 *	not be stated.
 *
 *	The layout_orientation specifies whether to layout the items
 *	horizontally or vertically.
 *
 *	The icon_text_orientation specifies to either display the
 *	icon on the left and the text on the right
 *	(GTK_ORIENTATION_HORIZONTAL) or the icon on top and the text
 *	on the bottom (GTK_ORIENTATION_VERTICAL).
 *
 *	The item_spacing specifies the spacing between each item.
 *
 *	If drop_shadow is TRUE then a shadow will be rendered
 *	into the DND drag icon.
 *
 *	The hot_x and hot_y specifies the hot spot.
 */
static void GUIDNDSetDragIconFromWidgetNexus(
	GtkWidget *w,
	GList *cells_list,
	const gint nitems,
	const GtkOrientation layout_orientation,
	const GtkOrientation icon_text_orientation,
	const gint item_spacing,
	const gboolean drop_shadow,
	const gint hot_x, const gint hot_y
)
{
	const gint	max_width = gdk_screen_width(),
			max_height = gdk_screen_height(),
			shadow_offset_x = GUI_DND_DRAG_ICON_DEF_SHADOW_OFFSET_X,
			shadow_offset_y = GUI_DND_DRAG_ICON_DEF_SHADOW_OFFSET_Y;
	gint		nitems_in_list,
			item_x, item_y,
			width, height,		/* Size of the entire DND
						 * drag icon pixmap and mask */
			item_highest,
			icon_width, icon_height,
			spacing = 0,		/* Spacing between the icon
						 * and the text */
			font_height, font_ascent;
	const gchar *text = NULL;
	GList *glist;
	GdkBitmap	*mask,			/* DND drag icon mask */
			*icon_mask = NULL;
	GdkPixmap	*pixmap,		/* DND drag icon pixmap */
			*icon_pixmap = NULL;
	GdkWindow *window = w->window;
	GdkFont *font;
	GdkGC *mask_gc;
	const GtkStateType state = GTK_WIDGET_STATE(w);
	GtkCellText *cell_text;
	GtkCellPixmap *cell_pixmap;
	GtkCellPixText *cell_pixtext;
	GtkStyle *style = gtk_widget_get_style(w);
	if(style == NULL)
	    return;

	font = style->font;
	if(font != NULL)
	{
	    font_ascent = font->ascent;
	    font_height = MAX((font->ascent + font->descent), 0);
	}
	else
	{
	    font_ascent = 0;
	    font_height = 0;
	}

	/* Calculate the size of the entire DND drag icon */
	width = 0;
	height = 0;
	item_highest = 0;
	for(glist = cells_list, nitems_in_list = 0;
	    glist != NULL;
	    glist = g_list_next(glist), nitems_in_list++
	)
	{
	    /* Get the information from this cell */
	    switch(*(GtkCellType *)glist->data)
	    {
	      case GTK_CELL_EMPTY:
	      case GTK_CELL_WIDGET:
		text = NULL;
		spacing = 0;
		icon_pixmap = NULL;
		icon_mask = NULL;
		break;
	      case GTK_CELL_TEXT:
		cell_text = (GtkCellText *)glist->data;
		text = cell_text->text;
		spacing = 0;
		icon_pixmap = NULL;
		icon_mask = NULL;
		break;
	      case GTK_CELL_PIXMAP:
		cell_pixmap = (GtkCellPixmap *)glist->data;
		text = NULL;
		spacing = 0;
		icon_pixmap = cell_pixmap->pixmap;
		icon_mask = cell_pixmap->mask;
		break;
	      case GTK_CELL_PIXTEXT:
		cell_pixtext = (GtkCellPixText *)glist->data;
		text = cell_pixtext->text;
		spacing = cell_pixtext->spacing;
		icon_pixmap = cell_pixtext->pixmap;
		icon_mask = cell_pixtext->mask;
		break;
	    }
	    if((icon_pixmap != NULL) && (text != NULL))
	    {
		gint	item_width = 0, item_height = 0,
			swidth;

		/* Get the size of the icon and text */
	        gdk_window_get_size(icon_pixmap, &icon_width, &icon_height);
		swidth = gdk_string_measure(font, text);

		/* Calculate the size of the row and update the subtotal
		 * size of the DND drag icon
		 */
		switch(icon_text_orientation)
		{
		  case GTK_ORIENTATION_HORIZONTAL:
		    item_width = icon_width + spacing + swidth + 3;
		    item_height = MAX(icon_height, (font_height + 3));
		    break;
		  case GTK_ORIENTATION_VERTICAL:
		    item_width = MAX(icon_width, (swidth + 3));
		    item_height = icon_height + spacing + font_height + 3;
		    break;
		}
		switch(layout_orientation)
		{
		  case GTK_ORIENTATION_HORIZONTAL:
		    width += item_width + item_spacing;
		    if(height < item_height)
			height = item_height;
		    break;
		  case GTK_ORIENTATION_VERTICAL:
		    if(width < item_width)
			width = item_width;
		    height += item_height + item_spacing;
		    break;
		}
		if(item_height > item_highest)
		    item_highest = item_height;
	    }
	    else if(icon_pixmap != NULL)
	    {
		gint item_width = 0, item_height = 0;

		/* Get the size of the icon */
	        gdk_window_get_size(icon_pixmap, &icon_width, &icon_height);

		/* Calculate the size of the row and update the subtotal
		 * size of the DND drag icon
		 */
		switch(icon_text_orientation)
		{
		  case GTK_ORIENTATION_HORIZONTAL:
		  case GTK_ORIENTATION_VERTICAL:
		    item_width = icon_width;
		    item_height = icon_height;
		    break;
		}
		switch(layout_orientation)
		{
		  case GTK_ORIENTATION_HORIZONTAL:
		    width += item_width + item_spacing;
		    if(height < item_height)
			height = item_height;
		    break;
		  case GTK_ORIENTATION_VERTICAL:
		    if(width < item_width)
			width = item_width;
		    height += item_height + item_spacing;
		    break;
		}
		if(item_height > item_highest)
		    item_highest = item_height;
	    }
	    else if(text != NULL)
	    {
		gint	item_width = 0, item_height = 0,
			swidth;

		/* Get the width needed to draw the text */
		swidth = gdk_string_measure(font, text);

		/* Calculate the size of the row and update the subtotal
		 * size of the DND drag icon
		 */
		switch(icon_text_orientation)
		{
		  case GTK_ORIENTATION_HORIZONTAL:
		  case GTK_ORIENTATION_VERTICAL:
		    item_width = swidth + 3;
		    item_height = font_height + 3;
		    break;
		}
		switch(layout_orientation)
		{
		  case GTK_ORIENTATION_HORIZONTAL:
		    width += item_width + item_spacing;
		    if(height < item_height)
			height = item_height;
		    break;
		  case GTK_ORIENTATION_VERTICAL:
		    if(width < item_width)
			width = item_width;
		    height += item_height + item_spacing;
		    break;
		}
		if(item_height > item_highest)
		    item_highest = item_height;
	    }
	}
	if((width <= 0) || (height <= 0))
	    return;

	/* Add one more line to state the number of additional items? */
	if((nitems > -1) ? (nitems > nitems_in_list) : FALSE)
	{
	    const gchar *text = "... 123456789 more items";
	    const gint swidth = gdk_string_measure(font, text);
	    if(width < (swidth + 3))
		width = swidth + 3;
	    height += font_height + 3 + item_spacing;
	}

	/* Add space for the drop shadow? */
	if(drop_shadow)
	{
	    width += MAX(shadow_offset_x, 0);
	    height += MAX(shadow_offset_y, 0);
	}

	/* Make sure that the size of the DND drag icon does not
	 * exceede the maximum
	 */
	if(width > max_width)
	    width = max_width;
	if(height > max_height)
	    height = max_height;

	/* Create the pixmap and mask for the DND drag icon */
	mask = (GdkBitmap *)gdk_pixmap_new(
	    window,
	    width, height,
	    1
	);
	pixmap = gdk_pixmap_new(
	    window,
	    width, height,
	    -1
	);
	if((mask == NULL) || (pixmap == NULL))
	{
	    GDK_PIXMAP_UNREF(pixmap);
	    GDK_BITMAP_UNREF(mask);
	    return;
	}

	/* Create the GC for the mask */
	mask_gc = gdk_gc_new((GdkWindow *)mask);
	if(mask_gc == NULL)
	{
	    GDK_PIXMAP_UNREF(pixmap);
	    GDK_BITMAP_UNREF(mask);
	    return;
	}

	/* Clear the mask */
	gdk_gc_set_function(mask_gc, GDK_CLEAR);
	gdk_draw_rectangle(
	    mask,
	    mask_gc,
	    TRUE,				/* Fill */
	    0, 0,
	    width, height
	);

#define DRAW_MASK_STRING(_xo, _yo)	{	\
 gdk_draw_string(				\
  mask,						\
  font,						\
  mask_gc,					\
  x + (_xo), y + (_yo),				\
  text						\
 );						\
}
#define DRAW_PIXMAP_STRING(_xo, _yo) {		\
 gdk_draw_string(				\
  pixmap,					\
  font,						\
  text_gc,					\
  x + (_xo), y + (_yo),				\
  text						\
 );						\
}

	/* Draw each item */
	gdk_gc_set_function(mask_gc, GDK_SET);
	gdk_gc_set_fill(mask_gc, GDK_SOLID);
	item_x = 0;
	item_y = 0;
	for(glist = cells_list;
	    glist != NULL;
	    glist = g_list_next(glist)
	)
	{
	    /* Has the current item coordinates gone off bounds
	     * of the DND drag icon's area?
	     */
	    if(layout_orientation == GTK_ORIENTATION_HORIZONTAL)
	    {
		if(item_x >= max_width)
		    break;
	    }
	    else if(layout_orientation == GTK_ORIENTATION_VERTICAL)
	    {
		if(item_y >= max_height)
		    break;
	    }

	    /* Get the information from this cell */
	    switch(*(GtkCellType *)glist->data)
	    {
	      case GTK_CELL_EMPTY:
	      case GTK_CELL_WIDGET:
		text = NULL;
		spacing = 0;
		icon_pixmap = NULL;
		icon_mask = NULL;
		break;
	      case GTK_CELL_TEXT:
		cell_text = (GtkCellText *)glist->data;
		text = cell_text->text;
		spacing = 0;
		icon_pixmap = NULL;
		icon_mask = NULL;
		break;
	      case GTK_CELL_PIXMAP:
		cell_pixmap = (GtkCellPixmap *)glist->data;
		text = NULL;
		spacing = 0;
		icon_pixmap = cell_pixmap->pixmap;
		icon_mask = cell_pixmap->mask;
		break;
	      case GTK_CELL_PIXTEXT:
		cell_pixtext = (GtkCellPixText *)glist->data;
		text = cell_pixtext->text;
		spacing = cell_pixtext->spacing;
		icon_pixmap = cell_pixtext->pixmap;
		icon_mask = cell_pixtext->mask;
		break;
	    }
	    if((icon_pixmap != NULL) && (text != NULL))
	    {
		gint	x = 0, y = 0,
			item_width = 0, item_height = 0,
			lbearing = 0, rbearing, swidth = 0,
			ascent, descent;
		GdkGC *gc, *text_gc;

		/* Get the size of the icon and text */
	        gdk_window_get_size(icon_pixmap, &icon_width, &icon_height);
		gdk_string_extents(
		    font, text,
		    &lbearing, &rbearing, &swidth, &ascent, &descent
		);

		/* Calculate the size of this cell */
		switch(icon_text_orientation)
		{
		  case GTK_ORIENTATION_HORIZONTAL:
		    item_width = icon_width + spacing + swidth + 3;
		    item_height = MAX(icon_height, (font_height + 3));
		    break;
		  case GTK_ORIENTATION_VERTICAL:
		    item_width = MAX(icon_width, (swidth + 3));
		    item_height = icon_height + spacing + font_height + 3;
		    break;
		}

		/* Draw the icon on the mask */
		switch(icon_text_orientation)
		{
		  case GTK_ORIENTATION_HORIZONTAL:
		    x = item_x;
		    y = item_y + ((item_height - icon_height) / 2);
		    break;
		  case GTK_ORIENTATION_VERTICAL:
		    x = item_x + ((item_width - icon_width) / 2);
		    switch(layout_orientation)
		    {
		      case GTK_ORIENTATION_HORIZONTAL:
			y = item_y + ((item_highest - item_height) / 2);
			break;
		      case GTK_ORIENTATION_VERTICAL:
			y = item_y;
			break;
		    }
		    break;
		}
		gdk_gc_set_clip_mask(mask_gc, icon_mask);
		if(drop_shadow)
		{
		    const gint	shadow_x = x + shadow_offset_x,
				shadow_y = y + shadow_offset_y;
		    gdk_gc_set_clip_origin(mask_gc, shadow_x, shadow_y);
		    gdk_draw_rectangle(
			mask,
			mask_gc,
			TRUE,			/* Fill */
			shadow_x, shadow_y,
			icon_width, icon_height
		    );
		}
	        gdk_gc_set_clip_origin(mask_gc, x, y);
	        gdk_draw_rectangle(
	            mask,
	            mask_gc,
		    TRUE,			/* Fill */
		    x, y,
		    icon_width, icon_height
		);
		gdk_gc_set_clip_mask(mask_gc, NULL);

		/* Draw the icon on the pixmap */
		if(drop_shadow)
		    gdk_draw_rectangle(
			pixmap,
			style->black_gc,
			TRUE,			/* Fill */
			x + shadow_offset_x, y + shadow_offset_y,
			icon_width, icon_height
		    );
		gc = style->white_gc;
		gdk_gc_set_clip_mask(gc, icon_mask);
	        gdk_gc_set_clip_origin(gc, x, y);
		gdk_draw_pixmap(
		    pixmap,
		    gc,
		    icon_pixmap,
		    0, 0,			/* Source x, y */
		    x, y,			/* Destination x, y */
		    icon_width, icon_height	/* Source size */
		);
		gdk_gc_set_clip_mask(gc, NULL);

		/* Draw the text on the mask */
		switch(icon_text_orientation)
		{
		  case GTK_ORIENTATION_HORIZONTAL:
		    x += icon_width + spacing + 1 - lbearing;
		    y = item_y + ((item_height - font_height) / 2) + 1 + font_ascent;
		    break;
		  case GTK_ORIENTATION_VERTICAL:
		    x = item_x + ((item_width - swidth) / 2) + 1 - lbearing;
		    y += icon_height + spacing + 1 + font_ascent;
		    break;
		}
		if(drop_shadow)
		    DRAW_MASK_STRING(shadow_offset_x, shadow_offset_y);
		DRAW_MASK_STRING(0, 0);
		DRAW_MASK_STRING(-1, 0);
		DRAW_MASK_STRING(0, -1);
		DRAW_MASK_STRING(1, 0);
		DRAW_MASK_STRING(0, 1);

		/* Draw the text on the pixmap */
		if(drop_shadow)
		{
		    text_gc = style->black_gc;
		    DRAW_PIXMAP_STRING(shadow_offset_x, shadow_offset_y);
		}
		text_gc = style->base_gc[state];
		DRAW_PIXMAP_STRING(-1, 0);
		DRAW_PIXMAP_STRING(0, -1);
		DRAW_PIXMAP_STRING(1, 0);
		DRAW_PIXMAP_STRING(0, 1);
		text_gc = style->text_gc[state];
		DRAW_PIXMAP_STRING(0, 0);

		/* Increment to the next cell coordinates */
		switch(layout_orientation)
		{
		  case GTK_ORIENTATION_HORIZONTAL:
		    item_x += item_width + item_spacing;
		    break;
		  case GTK_ORIENTATION_VERTICAL:
		    item_y += item_height + item_spacing;
		    break;
		}
	    }
	    else if(icon_pixmap != NULL)
	    {
		gint	x = 0, y = 0,
			item_width = 0, item_height = 0;
		GdkGC *gc;

		/* Get the size of this pixmap */
	        gdk_window_get_size(icon_pixmap, &icon_width, &icon_height);

		/* Calculate the size of this cell */
		switch(icon_text_orientation)
		{
		  case GTK_ORIENTATION_HORIZONTAL:
		  case GTK_ORIENTATION_VERTICAL:
		    item_width = icon_width;
		    item_height = icon_height;
		    break;
		}

		/* Draw the icon on the mask */
		x = item_x;
		y = item_y + ((item_height - icon_height) / 2);
		gdk_gc_set_clip_mask(mask_gc, icon_mask);
		if(drop_shadow)
		{
		    const gint	shadow_x = x + shadow_offset_x,
				shadow_y = y + shadow_offset_y;
		    gdk_gc_set_clip_origin(mask_gc, shadow_x, shadow_y);
		    gdk_draw_rectangle(
			mask,
			mask_gc,
			TRUE,			/* Fill */
			shadow_x, shadow_y,
			icon_width, icon_height
		    );
		}
	        gdk_gc_set_clip_origin(mask_gc, x, y);
	        gdk_draw_rectangle(
	            mask,
	            mask_gc,
		    TRUE,			/* Fill */
		    x, y,
		    icon_width, icon_height
		);
		gdk_gc_set_clip_mask(mask_gc, NULL);

		/* Draw the icon on the pixmap */
		if(drop_shadow)
		    gdk_draw_rectangle(
			pixmap,
			style->black_gc,
			TRUE,			/* Fill */
			x + shadow_offset_x, y + shadow_offset_y,
			icon_width, icon_height
		    );
		gc = style->white_gc;
		gdk_gc_set_clip_mask(gc, icon_mask);
	        gdk_gc_set_clip_origin(gc, x, y);
		gdk_draw_pixmap(
		    pixmap,
		    gc,
		    icon_pixmap,
		    0, 0,			/* Source x, y */
		    x, y,			/* Destination x, y */
		    icon_width, icon_height	/* Source size */
		);
		gdk_gc_set_clip_mask(gc, NULL);

		/* Increment to the next cell coordinates */
		switch(layout_orientation)
		{
		  case GTK_ORIENTATION_HORIZONTAL:
		    item_x += item_width + item_spacing;
		    break;
		  case GTK_ORIENTATION_VERTICAL:
		    item_y += item_height + item_spacing;
		    break;
		}
	    }
	    else if(text != NULL)
	    {
		gint	x = 0, y = 0,
			item_width = 0, item_height = 0,
			lbearing = 0, rbearing, swidth = 0,
			ascent, descent;
		GdkGC *text_gc;

		/* Get the size of the text */
		gdk_string_extents(
		    font, text,
		    &lbearing, &rbearing, &swidth, &ascent, &descent
		);

		/* Calculate the size of this cell */
		switch(icon_text_orientation)
		{
		  case GTK_ORIENTATION_HORIZONTAL:
		  case GTK_ORIENTATION_VERTICAL:
		    item_width = swidth + 3;
		    item_height = font_height + 3;
		    break;
		}

		/* Draw the text on the mask */
		x = item_x + 1 - lbearing;
		y = item_y + ((item_height - font_height) / 2) + 1 + font_ascent;
		if(drop_shadow)
		    DRAW_MASK_STRING(shadow_offset_x, shadow_offset_y);
		DRAW_MASK_STRING(0, 0);
		DRAW_MASK_STRING(-1, 0);
		DRAW_MASK_STRING(0, -1);
		DRAW_MASK_STRING(1, 0);
		DRAW_MASK_STRING(0, 1);

		/* Draw the text on the pixmap */
		if(drop_shadow)
		{
		    text_gc = style->black_gc;
		    DRAW_PIXMAP_STRING(shadow_offset_x, shadow_offset_y);
		}
		text_gc = style->base_gc[state];
		DRAW_PIXMAP_STRING(-1, 0);
		DRAW_PIXMAP_STRING(0, -1);
		DRAW_PIXMAP_STRING(1, 0);
		DRAW_PIXMAP_STRING(0, 1);
		text_gc = style->text_gc[state];
		DRAW_PIXMAP_STRING(0, 0);

		/* Increment to the next cell coordinates */
		switch(layout_orientation)
		{
		  case GTK_ORIENTATION_HORIZONTAL:
		    item_x += item_width + item_spacing;
		    break;
		  case GTK_ORIENTATION_VERTICAL:
		    item_y += item_height + item_spacing;
		    break;
		}
	    }
	}
	/* More items not shown? */
	if((nitems > -1) ? (nitems > nitems_in_list) : FALSE)
	{
	    const gint nmore_items = MAX((nitems - nitems_in_list), 0);
	    gint	x = 0, y = 0,
			lbearing = 0, rbearing, swidth = 0,
			ascent, descent;
	    gchar *text = g_strdup_printf(
		"... %i more %s",
		nmore_items,
		(nmore_items == 1) ? "item" : "items"
	    );
	    GdkGC *text_gc;

	    /* Get the size of the text */
	    gdk_string_extents(
		font, text,
		&lbearing, &rbearing, &swidth, &ascent, &descent
	    );

	    /* Draw the text on the mask */
	    x = ((width - swidth) / 2) + 1 - lbearing;
	    y = height - (drop_shadow ? shadow_offset_y : 0) -
		font_height + 1 + font_ascent - 2;
	    if(drop_shadow)
		DRAW_MASK_STRING(shadow_offset_x, shadow_offset_y);
	    DRAW_MASK_STRING(0, 0);
	    DRAW_MASK_STRING(-1, 0);
	    DRAW_MASK_STRING(0, -1);
	    DRAW_MASK_STRING(1, 0);
	    DRAW_MASK_STRING(0, 1);

	    /* Draw the text on the pixmap */
	    if(drop_shadow)
	    {
		text_gc = style->black_gc;
		DRAW_PIXMAP_STRING(shadow_offset_x, shadow_offset_y);
	    }
	    text_gc = style->base_gc[state];
	    DRAW_PIXMAP_STRING(-1, 0);
	    DRAW_PIXMAP_STRING(0, -1);
	    DRAW_PIXMAP_STRING(1, 0);
	    DRAW_PIXMAP_STRING(0, 1);
	    text_gc = style->text_gc[state];
	    DRAW_PIXMAP_STRING(0, 0);

	    g_free(text);
	}

#undef DRAW_MASK_STRING
#undef DRAW_PIXMAP_STRING

	GUIDNDSetDragIcon(
	    pixmap, mask,
	    hot_x, hot_y
	);

	GDK_GC_UNREF(mask_gc);
	GDK_PIXMAP_UNREF(pixmap);
	GDK_BITMAP_UNREF(mask);
}

/*
 *	Sets the DND drag icon from the GtkCells list.
 *
 *	The w specifies the GtkWidget.
 *
 *	The cells_list specifies a GList of GtkCellText *,
 *	GtkCellPixmap *, or GtkCellPixText * cells. Only the first 5
 *	cells will be used.
 *
 *	The nitems specifies the total number of items, which can be
 *	greater than the number of items in cells_list. If nitems
 *	is greater than the number of items in cells_list then the
 *	number of additional items will be stated by text on the
 *	drag icon. If nitems is negative then additional items will
 *	not be stated.
 *
 *	The layout_orientation specifies whether to layout the items
 *	horizontally or vertically.
 *
 *	The icon_text_orientation specifies to either display the
 *	icon on the left and the text on the right
 *	(GTK_ORIENTATION_HORIZONTAL) or the icon on top and the text
 *	on the bottom (GTK_ORIENTATION_VERTICAL).
 *
 *	The item_spacing specifies the spacing between each item.
 */
void GUIDNDSetDragIconFromCellsList(
	GtkWidget *w,
	GList *cells_list,
	const gint nitems,
	const GtkOrientation layout_orientation,
	const GtkOrientation icon_text_orientation,
	const gint item_spacing
)
{
	if((w == NULL) || (cells_list == NULL))
	    return;

	GUIDNDSetDragIconFromWidgetNexus(
	    w,
	    cells_list,
	    nitems,
	    layout_orientation,
	    icon_text_orientation,
	    item_spacing,
	    TRUE,			/* Render drop shadow */
	    GUI_DND_DRAG_ICON_DEF_HOT_X, GUI_DND_DRAG_ICON_DEF_HOT_Y
	);
}

/*
 *	Sets the DND drag icon from the GtkClist's selected rows.
 */
void GUIDNDSetDragIconFromCListSelection(GtkCList *clist)
{
	const gint	column = 0,
			max_rows = GUI_DND_DRAG_ICON_DEF_NITEMS_VISIBLE;
	gint i, row, ncolumns;
	guint8 spacing = 0;
	gchar *text = NULL;
	GList *glist, *cells_list;
	GdkPixmap *pixmap = NULL;
	GdkBitmap *mask = NULL;

	if(clist == NULL)
	    return;

	ncolumns = clist->columns;
	if(column >= ncolumns)
	    return;

	/* Create the cells list from the selected rows */
	cells_list = NULL;
	for(glist = clist->selection, i = 0;
	    (glist != NULL) && (i < max_rows);
	    glist = g_list_next(glist), i++
	)
	{
	    row = (gint)glist->data;
	    switch(gtk_clist_get_cell_type(clist, row, column))
	    {
	      case GTK_CELL_TEXT:
		gtk_clist_get_text(
		    clist,
		    row, column,
		    &text
		);
		if(text != NULL)
		{
		    GtkCellText *cell_text = (GtkCellText *)g_malloc0(
			sizeof(GtkCellText)
		    );
		    if(cell_text != NULL)
		    {
			cell_text->type = GTK_CELL_TEXT;
			cell_text->text = text;
			cells_list = g_list_append(
			    cells_list,
			    cell_text
			);
		    }
		}
		break;

	      case GTK_CELL_PIXMAP:
		gtk_clist_get_pixmap(
		    clist,
		    row, column,
		    &pixmap, &mask
		);
		if(pixmap != NULL)
		{
		    GtkCellPixmap *cell_pixmap = (GtkCellPixmap *)g_malloc0(
			sizeof(GtkCellPixmap)
		    );
		    if(cell_pixmap != NULL)
		    {
			cell_pixmap->type = GTK_CELL_PIXMAP;
			cell_pixmap->pixmap = pixmap;
			cell_pixmap->mask = mask;
			cells_list = g_list_append(
			    cells_list,
			    cell_pixmap
			);
		    }
		}
		break;

	      case GTK_CELL_PIXTEXT:
		gtk_clist_get_pixtext(
		    clist,
		    row, column,
		    &text,
		    &spacing,
		    &pixmap, &mask
		);
		if((pixmap != NULL) && (text != NULL))
		{
		    GtkCellPixText *cell_pixtext = (GtkCellPixText *)g_malloc0(
			sizeof(GtkCellPixText)
		    );
		    if(cell_pixtext != NULL)
		    {
			cell_pixtext->type = GTK_CELL_PIXTEXT;
			cell_pixtext->text = text;
			cell_pixtext->spacing = spacing;
			cell_pixtext->pixmap = pixmap;
			cell_pixtext->mask = mask;
			cells_list = g_list_append(
			    cells_list,
			    cell_pixtext
			);
		    }
		}
		break;

	      case GTK_CELL_WIDGET:
	      case GTK_CELL_EMPTY:
		break;
	    }
	}

	if(cells_list != NULL)
	{
	    GUIDNDSetDragIconFromWidgetNexus(
		GTK_WIDGET(clist),
		cells_list,
		g_list_length(clist->selection),
		GTK_ORIENTATION_VERTICAL,
		GTK_ORIENTATION_HORIZONTAL,
		GUI_DND_DRAG_ICON_DEF_ITEM_SPACING,
		TRUE,			/* Render drop shadow */
		GUI_DND_DRAG_ICON_DEF_HOT_X, GUI_DND_DRAG_ICON_DEF_HOT_Y
	    );
	    g_list_foreach(
		cells_list,
		(GFunc)g_free,
		NULL
	    );
	    g_list_free(cells_list);
	}
}
	
/*
 *	Sets the DND drag icon from the GtkCList.
 *
 *	The clist specifies the GtkCList to set the DND drag icon from.
 *
 *	The row and column specifies the cell that contains the
 *	information (GdkPixmap) that will be used to set the DND drag
 *	icon from. If column is -1 then the first cell with useable
 *	information on the specified row will be used.
 */
void GUIDNDSetDragIconFromCListCell(
	GtkCList *clist,
	const gint row, const gint column
)
{
	GdkPixmap *pixmap = NULL;
	GdkBitmap *mask = NULL;

	if((clist == NULL) || (row < 0))
	    return;

	if((row >= clist->rows) || (column >= clist->columns))
	    return;

	/* Column specified? */
	if(column > -1)
	{
	    gchar *text = NULL;
	    guint8 spacing = 0;

	    switch(gtk_clist_get_cell_type(clist, row, column))
	    {
	      case GTK_CELL_PIXMAP:
		gtk_clist_get_pixmap(
		    clist,
		    row, column,
		    &pixmap, &mask
		);
		break;
	      case GTK_CELL_PIXTEXT:
		gtk_clist_get_pixtext(
		    clist,
		    row, column,
		    &text,
		    &spacing,
		    &pixmap, &mask
		);
		break;
	      case GTK_CELL_TEXT:
	      case GTK_CELL_WIDGET:
	      case GTK_CELL_EMPTY:
		break;
	    }
	}
	else
	{
	    const gint ncolumns = clist->columns;
	    gint i;
	    gchar *text = NULL;
	    guint8 spacing = 0;

	    /* Iterate through each cell of the specified row and
	     * use the first pixmap encountered
	     */
	    for(i = 0; i < ncolumns; i++)
	    {
		switch(gtk_clist_get_cell_type(clist, row, i))
		{
		  case GTK_CELL_PIXMAP:
		    gtk_clist_get_pixmap(
			clist,
			row, i,
			&pixmap, &mask
		    );
		    break;
		  case GTK_CELL_PIXTEXT:
		    gtk_clist_get_pixtext(
			clist,
			row, i,
			&text,
			&spacing,
			&pixmap, &mask
		    );
		    break;
		  case GTK_CELL_TEXT:
		  case GTK_CELL_WIDGET:
		  case GTK_CELL_EMPTY:
		    break;
		}
		if(pixmap != NULL)
		    break;
	    }
	}

	GUIDNDSetDragIcon(
	    pixmap, mask,
	    GUI_DND_DRAG_ICON_DEF_HOT_X, GUI_DND_DRAG_ICON_DEF_HOT_Y
	);
}

/*
 *	Sets the DND drag icon from the GtkCList's selected row.
 *
 *	The clist specifies the GtkCList to set the DND drag icon from.
 *
 *	The last selected row with the first cell with useable
 *	information (GdkPixmap) will be used. If no rows are selected
 *	then no DND drag icon will be set.
 */
void GUIDNDSetDragIconFromCList(GtkCList *clist)
{
	GList *glist;

	if(clist == NULL)
	    return;

	glist = clist->selection_end;
	if(glist == NULL)
	    return;

	GUIDNDSetDragIconFromCListCell(
	    clist,
	    (gint)glist->data,
	    -1					/* Use the first useable cell */
	);
}

/*
 *	Sets the DND drag icon from the GtkCTree's selected nodes.
 */
void GUIDNDSetDragIconFromCTreeSelection(GtkCTree *ctree)
{
	gboolean	is_leaf = FALSE,
			expanded = FALSE;
	const gint max_rows = GUI_DND_DRAG_ICON_DEF_NITEMS_VISIBLE;
	gint i;
	gchar *text = NULL;
	guint8 spacing = 0;
	GdkBitmap	*mask = NULL,
			*mask_closed = NULL,
			*mask_opened = NULL;
	GdkPixmap	*pixmap = NULL,
			*pixmap_closed = NULL,
			*pixmap_opened = NULL;
	GList *glist, *cells_list;
	GtkCList *clist;
	GtkCTreeNode *node;

	if(ctree == NULL)
	    return;

	clist = GTK_CLIST(ctree);

	/* Create the cells list from the selected rows */
	cells_list = NULL;
	for(glist = clist->selection, i = 0;
	    (glist != NULL) && (i < max_rows);
	    glist = g_list_next(glist), i++
	)
	{
	    /* Get the next selected node */
	    node = (GtkCTreeNode *)glist->data;
	    if(node == NULL)
		continue;

	    gtk_ctree_get_node_info(
		ctree,
		node,
		&text,
		&spacing,
		&pixmap_closed,
		&mask_closed,
		&pixmap_opened,
		&mask_opened,
		&is_leaf,
		&expanded
	    );
	    if((text != NULL) || (pixmap_closed != NULL))
	    {
		/* Is the node opened? */
		if(expanded)
		{
		    /* Get the opened GdkPixmap */
		    pixmap = pixmap_opened;
		    mask = mask_opened;

		    /* If the opened GdkPixmap was not available then
		     * get the closed GdkPixmap
		     */
		    if(pixmap == NULL)
		    {
			pixmap = pixmap_closed;
			mask = mask_closed;
		    }
		}
		else
		{
		    /* Get the closed GdkPixmap */
		    pixmap = pixmap_closed;
		    mask = mask_closed;
		}

		if((text != NULL) && (pixmap != NULL))
		{
		    GtkCellPixText *cell_pixtext = (GtkCellPixText *)g_malloc0(
			sizeof(GtkCellPixText)
		    );
		    if(cell_pixtext != NULL)
		    {
			cell_pixtext->type = GTK_CELL_PIXTEXT;
			cell_pixtext->text = text;
			cell_pixtext->spacing = spacing;
			cell_pixtext->pixmap = pixmap;
			cell_pixtext->mask = mask;
			cells_list = g_list_append(
			    cells_list,
			    cell_pixtext
			);
		    }
		}
		else if(pixmap != NULL)
		{
		    GtkCellPixmap *cell_pixmap = (GtkCellPixmap *)g_malloc0(
			sizeof(GtkCellPixmap)
		    );
		    if(cell_pixmap != NULL)
		    {
			cell_pixmap->type = GTK_CELL_PIXMAP;
			cell_pixmap->pixmap = pixmap;
			cell_pixmap->mask = mask;
			cells_list = g_list_append(
			    cells_list,
			    cell_pixmap
			);
		    }
		}
		else if(text != NULL)
		{
		    GtkCellText *cell_text = (GtkCellText *)g_malloc0(
			sizeof(GtkCellText)
		    );
		    if(cell_text != NULL)
		    {
			cell_text->type = GTK_CELL_TEXT;
			cell_text->text = text;
			cells_list = g_list_append(
			    cells_list,
			    cell_text
			);
		    }
		}
	    }
	}

	if(cells_list != NULL)
	{
	    GUIDNDSetDragIconFromWidgetNexus(
		GTK_WIDGET(ctree),
		cells_list,
		g_list_length(clist->selection),
		GTK_ORIENTATION_VERTICAL,
		GTK_ORIENTATION_HORIZONTAL,
		GUI_DND_DRAG_ICON_DEF_ITEM_SPACING,
		TRUE,			/* Render drop shadow */
		GUI_DND_DRAG_ICON_DEF_HOT_X, GUI_DND_DRAG_ICON_DEF_HOT_Y
	    );
	    g_list_foreach(
		cells_list,
		(GFunc)g_free,
		NULL
	    );
	    g_list_free(cells_list);
	}
}

/*
 *	Sets the DND drag icon from the GtkCTree node.
 *
 *	The ctree specifies the GtkCTree to set the DND drag icon from.
 *
 *	The node and column specifies the cell that contains the
 *	information (GdkPixmap) that will be used to set the DND drag
 *	icon from. If column is -1 then the first cell with useable
 *	information on the specified row will be used.
 */
void GUIDNDSetDragIconFromCTreeNode(
	GtkCTree *ctree,
	GtkCTreeNode *node,
	const gint column
)
{
	GdkPixmap *pixmap = NULL;
	GdkBitmap *mask = NULL;
	GtkCList *clist;

	if((ctree == NULL) || (node == NULL))
	    return;

	clist = GTK_CLIST(ctree);
	if(column >= clist->columns)
	    return;

	/* No column specified? */
	if(column < 0)
	{
	    /* Use the first cell */
	    GtkCTreeRow *row_ptr = GTK_CTREE_ROW(node);
	    if(row_ptr != NULL)
	    {
		/* Is the node opened? */
		if(row_ptr->expanded)
		{
		    /* Get the opened GdkPixmap */
		    pixmap = row_ptr->pixmap_opened;
		    mask = row_ptr->mask_opened;

		    /* If the opened GdkPixmap was not available then
		     * get the closed GdkPixmap
		     */
		    if(pixmap == NULL)
		    {
			pixmap = row_ptr->pixmap_closed;
			mask = row_ptr->mask_closed;
		    }
		}
		else
		{
		    /* Get the closed GdkPixmap */
		    pixmap = row_ptr->pixmap_closed;
		    mask = row_ptr->mask_closed;
		}
	    }
	}
	else
	{
	    gchar *text = NULL;
	    guint8 spacing = 0;

	    switch(gtk_ctree_node_get_cell_type(ctree, node, column))
	    {
	      case GTK_CELL_PIXMAP:
		gtk_ctree_node_get_pixmap(
		    ctree,
		    node, column,
		    &pixmap, &mask
		);
		break;
	      case GTK_CELL_PIXTEXT:
		gtk_ctree_node_get_pixtext(
		    ctree,
		    node, column,
		    &text,
		    &spacing,
		    &pixmap, &mask
		);
		break;
	      case GTK_CELL_TEXT:
	      case GTK_CELL_WIDGET:
	      case GTK_CELL_EMPTY:
		break;
	    }
	}

	GUIDNDSetDragIcon(
	    pixmap, mask,
	    GUI_DND_DRAG_ICON_DEF_HOT_X, GUI_DND_DRAG_ICON_DEF_HOT_Y
	);
}

/*
 *	Sets the DND drag icon.
 *
 *	The pixmap and mask specifies the GdkPixmap and GdkBitmap to
 *	use as the new DND drag icon. If pixmap is NULL then the
 *	DND drag icon is cleared.
 *
 *	The hot_x and hot_y specifies the hot spot.
 */
void GUIDNDSetDragIcon(
	GdkPixmap *pixmap, GdkBitmap *mask,
	const gint hot_x, const gint hot_y
)
{
	gboolean is_pixmap_set = FALSE;
	GdkWindow *window;
	GtkWidget *w;
	GUIDNDIcon *dnd_icon = &gui_dnd_icon;

	/* Clear the DND drag icon? */
	if(pixmap == NULL)
	{
	    /* Destroy the DND drag icon GtkWidget */
	    GTK_WIDGET_DESTROY(dnd_icon->icon_pm);
	    GTK_WIDGET_DESTROY(dnd_icon->toplevel);
	    memset(dnd_icon, 0x00, sizeof(GUIDNDIcon));
	    return;
	}

	/* If the DND drag icon GtkWidget does not exist then this
	 * the very first call to this function
	 */
	w = dnd_icon->toplevel;
	if(w == NULL)
	{
	    /* Create the DND drag icon GtkWidget */
	    GtkWidget *parent;

	    dnd_icon->toplevel = w = gtk_window_new(GTK_WINDOW_POPUP);
	    gtk_window_set_policy(GTK_WINDOW(w), FALSE, FALSE, FALSE);
	    gtk_widget_realize(w);
	    parent = w;

	    dnd_icon->icon_pm = w = gtk_pixmap_new(pixmap, mask);
	    gtk_container_add(GTK_CONTAINER(parent), w);
	    gtk_widget_show(w);
	    is_pixmap_set = TRUE;
	}

	/* Get the DND drag icon's toplevel GdkWindow */
	w = dnd_icon->toplevel;
	window = w->window;
	if(window == NULL)
	    return;

	/* Set the hot spot */
	dnd_icon->x = hot_x;
	dnd_icon->y = hot_y;

	/* Set the size of the new DND drag icon */
	gdk_window_get_size(
	    pixmap,
	    &dnd_icon->width, &dnd_icon->height
	);

	/* Adjust the size and shape of the DND drag icon's toplevel
	 * GdkWindow
	 */
	gtk_widget_set_usize(
	    w,
	    dnd_icon->width, dnd_icon->height
	);
	gdk_window_set_back_pixmap(
	    window,
	    pixmap,
	    FALSE				/* Not parent relative */
	);
	gtk_widget_shape_combine_mask(
	    w,
	    mask,
	    0, 0				/* Offset */
	);

	/* Need to set the icon on to the GtkPixmap? */
	if(!is_pixmap_set)
	    gtk_pixmap_set(
		GTK_PIXMAP(dnd_icon->icon_pm),
		pixmap, mask
	    );
}

/*
 *	Clears the DND drag icon.
 *
 *	The DND drag icon will be reset to the default GTK DND drag
 *	icon.
 */
void GUIDNDClearDragIcon(void)
{
	GUIDNDSetDragIcon(
	    NULL, NULL,
	    0, 0
	);
}


/*
 *	Creates a new Banner.
 */
GtkWidget *GUIBannerCreate(
	const gchar *label,
	const gchar *font_name,
	GdkColor color_fg,
	GdkColor color_bg,
	const GtkJustification justify,
	const gboolean expand
)
{
	GdkFont *font;
	GtkRcStyle *rcstyle;
	GtkWidget *w;
	GUIBannerData *banner = GUI_BANNER_DATA(g_malloc0(
	    sizeof(GUIBannerData)
	));
	if(banner == NULL)
	    return(NULL);

	banner->toplevel = gtk_drawing_area_new();
	banner->label = STRDUP(label);
	banner->justify = justify;
	banner->font = font = (font_name != NULL) ?
	    gdk_font_load(font_name) : NULL;

	w = banner->toplevel;
	gtk_object_set_data_full(
	    GTK_OBJECT(w), GUI_BANNER_DATA_KEY,
	    banner, GUIBannerDataDestroyCB
	);
	gtk_widget_add_events(
	    w,
	    GDK_STRUCTURE_MASK | GDK_EXPOSURE_MASK
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "configure_event",
	    GTK_SIGNAL_FUNC(GUIBannerEventCB), banner
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "expose_event",
	    GTK_SIGNAL_FUNC(GUIBannerEventCB), banner
	);

	/* Set style */
	rcstyle = gtk_rc_style_new();
	if(!STRISEMPTY(font_name))
	    rcstyle->font_name = STRDUP(font_name);
	rcstyle->color_flags[GTK_STATE_NORMAL] = GTK_RC_FG |
	    GTK_RC_BG | GTK_RC_TEXT;
	rcstyle->color_flags[GTK_STATE_INSENSITIVE] = GTK_RC_FG |
	    GTK_RC_BG | GTK_RC_TEXT;
	memcpy(&rcstyle->fg[GTK_STATE_NORMAL], &color_fg, sizeof(GdkColor));
	memcpy(&rcstyle->text[GTK_STATE_NORMAL], &color_fg, sizeof(GdkColor));
	memcpy(&rcstyle->bg[GTK_STATE_NORMAL], &color_bg, sizeof(GdkColor));
	/* Modify the foreground and background colors for GTK_STATE_INSENSITIVE */
#define LOWER_VALUE(c)	(gushort)((((gint)(c) - (gint)((guint16)-1 / 2)) * 0.75) + \
(gint)((guint16)-1 / 2))
	color_fg.red = LOWER_VALUE(color_fg.red);
	color_fg.green = LOWER_VALUE(color_fg.green);
	color_fg.blue = LOWER_VALUE(color_fg.blue);
	color_bg.red = LOWER_VALUE(color_bg.red);
	color_bg.green = LOWER_VALUE(color_bg.green);
	color_bg.blue = LOWER_VALUE(color_bg.blue);
#undef LOWER_VALUE
	memcpy(&rcstyle->fg[GTK_STATE_INSENSITIVE], &color_fg, sizeof(GdkColor));
	memcpy(&rcstyle->text[GTK_STATE_INSENSITIVE], &color_fg, sizeof(GdkColor));
	memcpy(&rcstyle->bg[GTK_STATE_INSENSITIVE], &color_bg, sizeof(GdkColor));

	gtk_widget_modify_style(w, rcstyle);

	GTK_RC_STYLE_UNREF(rcstyle);

	/* Get the new style values from widget */
	if(font != NULL)
	{
	    gint width, height;

	    if(expand)
		width = -1;
	    else if(label != NULL)
		width = gdk_string_measure(font, label);
	    else
		width = -1;

	    height = font->ascent + font->descent;
  
	    gtk_widget_set_usize(w, width, height);
	}

	return(banner->toplevel);
}

/*
 *	Draws the Banner.
 */
void GUIBannerDraw(GtkWidget *w)
{
	gint width, height;
	const gchar *label;
	GdkFont *font;
	GdkWindow *window;
	GdkDrawable *drawable;
	GtkStateType state;
	GtkJustification justify;
	GtkStyle *style;
	GUIBannerData *banner = GUI_BANNER_DATA(GTK_OBJECT_GET_DATA(
	    w,
	    GUI_BANNER_DATA_KEY
	));
	if(banner == NULL)
	    return;

	label = banner->label;
	justify = banner->justify;
	font = banner->font;

	window = w->window;
	state = GTK_WIDGET_STATE(w);
	style = gtk_widget_get_style(w);
	if((window == NULL) || (style == NULL))
	    return;

	drawable = (GdkDrawable *)window;

	gdk_window_get_size(
	    (GdkWindow *)drawable,
	    &width, &height
	);
	if((width <= 0) || (height <= 0))
	    return;

	/* Draw the background */
	gdk_draw_rectangle(
	    drawable,
	    style->bg_gc[state],
	    TRUE,				/* Fill */
	    0, 0,
	    width, height
	);

	/* Got the font and label? */
	if((font != NULL) && !STRISEMPTY(label))
	{
	    const gint border = 3;
	    gint lbearing, rbearing, swidth, ascent, descent;

	    gdk_string_extents(
		font,
		label,
		&lbearing, &rbearing, &swidth, &ascent, &descent
	    );

	    switch(justify)
	    {
	      case GTK_JUSTIFY_CENTER:
		gdk_draw_string(
		    drawable,
		    font,
		    style->text_gc[state],
		    ((width - swidth) / 2) - lbearing,
		    ((height - (font->ascent + font->descent)) / 2) +
			font->ascent,
		    label  
		);
		break;
	      case GTK_JUSTIFY_RIGHT:
		gdk_draw_string(
		    drawable,
		    font,
		    style->text_gc[state],
		    (width - swidth - border) - lbearing,
		    ((height - (font->ascent + font->descent)) / 2) +
			font->ascent,
		    label
		);
		break;
	      case GTK_JUSTIFY_LEFT:
		gdk_draw_string(
		    drawable,
		    font,
		    style->text_gc[state],
		    border - lbearing,
		    ((height - (font->ascent + font->descent)) / 2) +
			font->ascent,
		    label                    
		);
		break;
	      case GTK_JUSTIFY_FILL:
		break;
	    }
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
}


/*
 *	Creates a new GtkCombo with label, combo, initial value, and
 *	initial list of items.
 *
 *	Returns the GtkTable (the parent) or NULL on failure.
 *
 *	The given GList list will be duplicated, so the one passed to
 *	this call will not be modified.
 */
GtkWidget *GUIComboCreate(
	const gchar *label,	/* Label */
	const gchar *text,	/* Entry Value */
	GList *list,		/* Combo List */
	gint max_items,		/* Maximum Items In Combo List */
	gpointer *combo_rtn,	/* GtkCombo Return */
	gpointer data,
	void (*func_cb)(GtkWidget *w, gpointer),
	void (*list_change_cb)(GtkWidget *, gpointer, GList *)
)
{
	GtkWidget *parent, *wlabel = NULL, *w;
	GtkCombo *combo;
	const gchar *cstrptr;
	gpointer *cb_data;


	/* Reset return values */
	if(combo_rtn != NULL)
	    *combo_rtn = NULL;

	/* Create parent table widget */
	parent = gtk_table_new(
	    1,
	    ((label != NULL) ? 1 : 0) +
	    1,
	    FALSE
	);
	/* Do not show the table parent */

	/* Create label? */
	if(label != NULL)
	{
	    gint x = 0;

	    wlabel = gtk_label_new(label);
	    gtk_table_attach(
		GTK_TABLE(parent), wlabel,
		(guint)x, (guint)(x + 1),
		0, 1,
		0,
		0,
		2, 2
	    );
	    gtk_label_set_justify(GTK_LABEL(wlabel), GTK_JUSTIFY_RIGHT);
	    gtk_widget_show(wlabel);
	}

	/* Create combo */
	if(TRUE)
	{
	    gint i;
	    gint x = (label != NULL) ? 1 : 0;
	    GList *glist_in, *glist_out;
	    GtkEntry *entry;


	    w = gtk_combo_new();
	    combo = GTK_COMBO(w);
	    entry = GTK_ENTRY(combo->entry);
	    if(combo_rtn != NULL)
		*combo_rtn = w;
	    gtk_table_attach(
		GTK_TABLE(parent), w,
		(guint)x, (guint)(x + 1),
		0, 1,
		GTK_SHRINK | GTK_EXPAND | GTK_FILL,
		0,
		2, 2
	    );
	    gtk_combo_disable_activate(combo);
	    gtk_entry_set_text(entry, (text != NULL) ? text : "");
	    gtk_entry_set_editable(entry, TRUE);
	    gtk_combo_set_case_sensitive(combo, FALSE);


	    /* Begin creating output glist, a duplicate input glist */
	    glist_out = NULL;
	    glist_in = (GList *)list;

	    /* Is input glist given? */
	    if(glist_in == NULL)
	    {
		/* Input glist is NULL so create a bunch of empty strings
		 * totalling that specified by max_items for the output
		 * glist.
		 */
		for(i = 0; i < max_items; i++)
		    glist_out = g_list_append(glist_out, STRDUP(""));
	    }
	    else
	    {
		/* Input glist is given, make a duplicate of it as the
		 * output glist.
		 */
		i = 0;
		while((i < max_items) && (glist_in != NULL))
		{
		    cstrptr = (const gchar *)glist_in->data;
		    glist_out = g_list_append(
			glist_out, STRDUP(cstrptr)
		    );
		    glist_in = g_list_next(glist_in);
		    i++;
		}
	    }
	    glist_in = NULL;	/* Input glist should no longer be used */

	    /* Set new output glist to combo box */
	    if(glist_out != NULL)
		gtk_combo_set_popdown_strings(combo, glist_out);

	    /* Do not delete the output glist, it will be passed to the
	     * callback data below.
	     */

	    /* Allocate callback data, 9 pointers of format;
	     * table		Parent table that holds this combo and label.
	     * label		Label (may be NULL).
	     * combo		This combo widget.
	     * GList		Contains list of strings in combo list.
	     * max_items	Max items allowed in combo list.
	     * data		Calling function set callback data.
	     * func_cb		Calling function set callback function.
	     * list_change_cb	Calling function set list change function.
	     * NULL
	     */
	    cb_data = (gpointer *)g_malloc0(9 * sizeof(gpointer));
	    cb_data[0] = parent;
	    cb_data[1] = wlabel;
	    cb_data[2] = combo;
	    cb_data[3] = glist_out;
	    cb_data[4] = (gpointer)MAX(max_items, 0);
	    cb_data[5] = data;
	    cb_data[6] = (gpointer)func_cb;
	    cb_data[7] = (gpointer)list_change_cb;
	    cb_data[8] = NULL;
	    gtk_object_set_data_full(
		GTK_OBJECT(combo), GUI_COMBO_DATA_KEY,
		cb_data, GUIComboDataDestroyCB
	    );

	    gtk_signal_connect(
		GTK_OBJECT(entry), "activate",
		GTK_SIGNAL_FUNC(GUIComboActivateCB), cb_data
	    );
	    gtk_signal_connect(
		GTK_OBJECT(entry), "changed",
		GTK_SIGNAL_FUNC(GUIComboChangedCB), cb_data
	    );

	    gtk_widget_show(w);
	}

	return(parent);
}

/*
 *	Adds the specified value to the combo widget w's list and current
 *	value for the combo widget's entry widget.
 *
 *	The item will only be added to the list if it is not NULL
 *	and does not already exist in the list.
 *
 *	Excess items may be truncated if adding this item would exceed
 *	the combo list's maximum allowed items.
 */
void GUIComboActivateValue(GtkWidget *w, const gchar *value)
{
	gpointer *cb_data;
	GtkCombo *combo = (GtkCombo *)w;
	if((combo == NULL) || (value == NULL))
	    return;

	cb_data = (gpointer *)gtk_object_get_data(
	    GTK_OBJECT(combo),
	    GUI_COMBO_DATA_KEY
	);
	if(cb_data == NULL)
	    return;

	/* Set new value on combo's entry widget */
	gtk_entry_set_text(
	    GTK_ENTRY(combo->entry), value
	);

	/* Call activate callback as if this was an actual activate
	 * signal
	 */
	GUIComboActivateCB(GTK_WIDGET(combo), (gpointer)cb_data);
}

/*
 *	Adds the given value to the beginning of the glist for the
 *	combo box w. Older items in the list may be truncated if adding
 *	a new value would exceed the set maximum items for the combo box.
 *
 *	If value is already in the list then no operation will be
 *	performed.
 *
 *	The combo box's set list change callback will be called if it is
 *	not NULL and the new value was different and added succesfully.
 */
void GUIComboAddItem(GtkWidget *w, const gchar *value)
{
	gint i, max_items;
	gboolean status;
	const gchar *cstrptr;
	GtkWidget *parent, *wlabel;
	GtkCombo *combo = (GtkCombo *)w;
	GList *glist_in, *glist_next_in;
	gpointer client_data;
	void (*func_cb)(GtkWidget *, gpointer);
	void (*list_change_cb)(GtkWidget *, gpointer, GList *);
	gpointer *cb_data;
	if((combo == NULL) || (value == NULL))
	    return;

	/* Get object callback data */
	cb_data = (gpointer *)gtk_object_get_data(
	    GTK_OBJECT(combo),
	    GUI_COMBO_DATA_KEY
	);
	if(cb_data == NULL)
	    return;

	/* Parse callback data, format is as follows;
	 * table        Parent table that holds this combo and label.
	 * label        Label (may be NULL).
	 * combo        This combo widget.
	 * GList        Contains list of strings in combo list.
	 * max_items    Max items allowed in combo list.
	 * client_data  Calling function set callback data.
	 * func_cb      Calling function set callback function.
	 * list_change_cb       Calling function set list change function.
	 * NULL
	 */
	parent = (GtkWidget *)(cb_data[0]);
	wlabel = (GtkWidget *)(cb_data[1]);
	combo = (GtkCombo *)(cb_data[2]);
	glist_in = (GList *)(cb_data[3]);
	max_items = (gint)(cb_data[4]);
	client_data = (gpointer)(cb_data[5]);
	func_cb = cb_data[6];
	list_change_cb = cb_data[7];


	/* Check if new value is already in the input glist */
	status = TRUE;
	i = 0;
	glist_next_in = glist_in;
	/* Iterate from 0 to max_items or when the input glist is
	 * NULL (whichever occures first).
	 */
	while((i < max_items) && (glist_next_in != NULL))
	{
	    cstrptr = (gchar *)glist_next_in->data;
	    if(cstrptr != NULL)
	    {
 /* Check if case sensitive? */
		/* Compare list item value with given value */
		if(!g_strcasecmp(cstrptr, value))
		{
		    /* Given value matches a value in the list, so set
		     * status to FALSE indicating there is a value already
		     * in the list.
		     */
		    status = FALSE;
		    break;
		}
	    }
	    i++;
	    glist_next_in = g_list_next(glist_next_in);
	}
	/* Variable status will be set to FALSE if the value is already
	 * in the list.
	 */

	/* Check if max_items allows us to add a new item to the list and
	 * if status is TRUE (indicating value is not already in the
	 * list).
	 */
	if((max_items > 0) && status)
	{ 
	    /* Create new output glist */
	    GList *glist_out = NULL;

	    /* Add first item in output glist to be the new value fetched
	     * from the combo's entry.
	     */
	    glist_out = g_list_append(glist_out, STRDUP(value));
	
	    /* Now copy old input glist items to output glist, starting
	     * with i = 1 since we already have one item in the output
	     * glist.
	     */
	    i = 1;
	    glist_next_in = glist_in;
	    while((i < max_items) && (glist_next_in != NULL)) 
	    {
		cstrptr = (const gchar *)glist_next_in->data;
		glist_out = g_list_append(glist_out, STRDUP(cstrptr));
		glist_next_in = g_list_next(glist_next_in);
		i++;
	    }

	    /* Set new output glist to the combo box's list */
	    if(glist_out != NULL)
		gtk_combo_set_popdown_strings(combo, glist_out);

	    /* Free old input glist since its no longer being used */
	    if(glist_in != NULL)
	    {
		g_list_foreach(glist_in, (GFunc)g_free, NULL);
		g_list_free(glist_in);
		glist_in = NULL;
	    }

	    /* Update input glist to be that of the output glist */
	    glist_in = glist_out;

	    /* Record new glist on callback data */
	    cb_data[3] = (gpointer)glist_in;

	    /* Call list change function and pass the new glist */
	    if(list_change_cb != NULL)
	    {
		list_change_cb(
		    (GtkWidget *)combo,	/* Pass combo box as widget */
		    client_data,	/* Client data */
		    glist_in		/* New glist */
		);
	    }
	}
}

/*
 *	Returns the GList for the combo widget created by
 *	GUIComboCreate().
 */
GList *GUIComboGetList(GtkWidget *w)
{
	gpointer *cb_data;

	if(w == NULL)
	    return(NULL);

	cb_data = (gpointer *)gtk_object_get_data(
	    GTK_OBJECT(w),
	    GUI_COMBO_DATA_KEY
	);
	if(cb_data == NULL)
	    return(NULL);

	/* Parse callback data, format is as follows;
	 * table            Parent table that holds this combo and label.
	 * label            Label (may be NULL).
	 * combo            This combo widget.
	 * GList            Contains list of strings in combo list.
	 * max_items        Max items allowed in combo list.
	 * client_data      Calling function set callback data.
	 * func_cb          Calling function set callback function.
	 * list_change_cb   Calling function set list change function.
	 * NULL
	 */

	return((GList *)cb_data[3]);
}

/*
 *	Sets the new list for the combo.
 *
 *	If the pointer of the given list is the same as the combo's
 *	current list then the given list will simply be pdated to the
 *	combo with a call to gtk_combo_set_popdown_strings().
 *
 *	If the given list is NULL then the combo's current list will
 *	be deleted.
 *
 *	In any case the given list should not be referenced again after
 *	this call.
 */
void GUIComboSetList(GtkWidget *w, GList *list)
{
	GList *glist;
	GtkCombo *combo;
	gpointer *cb_data;

	if(w == NULL)
	    return;

	combo = GTK_COMBO(w);
	if(combo == NULL)
	    return;

	cb_data = (gpointer *)gtk_object_get_data(
	    GTK_OBJECT(w),
	    GUI_COMBO_DATA_KEY
	);
	if(cb_data == NULL)
	    return;

	/* Parse callback data, format is as follows;
	 * table            Parent table that holds this combo and label.
	 * label            Label (may be NULL).
	 * combo            This combo widget.
	 * GList            Contains list of strings in combo list.
	 * max_items        Max items allowed in combo list.
	 * client_data      Calling function set callback data.
	 * func_cb          Calling function set callback function.
	 * list_change_cb   Calling function set list change function.
	 * NULL
	 */
	glist = (GList *)cb_data[3];

	/* Is given list NULL? */
	if(list == NULL)
	{
	    gint i, max_items = (gint)cb_data[4];
	    GList	*glist_new,
			*glist_old = glist;

	    /* Create a new glist containing just empty strings */
	    glist_new = NULL;
	    for(i = 0; i < max_items; i++)
		glist_new = g_list_append(glist_new, STRDUP(""));

	    /* Was new glist created successfully? */
	    if(glist_new != NULL)
	    {
		/* Set new glist to combo */
		gtk_combo_set_popdown_strings(combo, glist_new);

		/* If old glist exists, then delete it */
		if(glist_old != NULL)
		{
		    g_list_foreach(glist_old, (GFunc)g_free, NULL);
		    g_list_free(glist_old);
		    glist_old = NULL;
		}

		/* Update pointer to new glist */
		cb_data[3] = (gpointer)glist_new;
		glist = glist_new;
	    }
	}
	/* Given list matches current list's base pointer values? */
	else if((gpointer)list == (gpointer)glist)
	{
	    /* Just update list on combo then */
	    gtk_combo_set_popdown_strings(combo, glist);

	    /* No need to change pointer on callback data */

	    /* No need to deallocate given list */
	}
	else
	{
	    /* New and current list base pointers do not match and
	     * current glist may be NULL
	     */
	    const gchar *s;
	    gint i, max_items;
	    GList	*glist_new, *glist_in,
			*glist_old = glist;

	    /* Make a copy the given list as glist_new and limit the
	     * number of items to max_items
	     */
	    i = 0;
	    max_items = (gint)cb_data[4];
	    glist_new = NULL;		/* New glist */
	    glist_in = (GList *)list;	/* Input glist */
	    while((i < max_items) && (glist_in != NULL))
	    {
		s = (const gchar *)glist_in->data;
		glist_new = g_list_append(glist_new, STRDUP(s));
		glist_in = g_list_next(glist_in);
		i++;
	    }

	    /* Destroy the given list */
	    glist_in = (GList *)list;	/* Input glist */
	    list = NULL;		/* Mark input list as NULL */
	    if(glist_in != NULL)
	    {
		g_list_foreach(glist_in, (GFunc)g_free, NULL);
		g_list_free(glist_in);
		glist_in = NULL;
	    }

	    /* Is new coppied glist valid? */
	    if(glist_new != NULL)
	    {
		/* Set new glist to combo */
		gtk_combo_set_popdown_strings(combo, glist_new);

		/* If old glist exists, then delete it */
		if(glist_old != NULL)
		{ 
		    g_list_foreach(glist_old, (GFunc)g_free, NULL);
		    g_list_free(glist_old);
		    glist_old = NULL;
		}

		/* Update pointer to new glist on callback data */
		cb_data[3] = (gpointer)glist_new;
		glist = glist_new;
	    }
	}
}

/*
 *	Resets all values in the combo widget.
 */
void GUIComboClearAll(GtkWidget *w)
{
	GtkCombo *combo = (GtkCombo *)w;
	if(combo == NULL)
	    return;

	/* Clear text entry */
 	gtk_entry_set_text(GTK_ENTRY(combo->entry), "");
	gtk_entry_set_position(GTK_ENTRY(combo->entry), 0);

	/* Clear combo's glist by setting a new one as NULL */
	GUIComboSetList(w, NULL);
}


/*
 *	Creates a new Menu Bar & GtkAccelGroup.
 */
GtkWidget *GUIMenuBarCreate(GtkAccelGroup **accelgrp_rtn)
{
	if(accelgrp_rtn != NULL)
	    *accelgrp_rtn = gtk_accel_group_new();

	return(gtk_menu_bar_new());
}

/*
 *	Creates a new Tear Off Menu.
 */
GtkWidget *GUIMenuCreateTearOff(void)
{
	GtkWidget	*menu = gtk_menu_new(),
			*w = gtk_tearoff_menu_item_new();
	gtk_menu_append(GTK_MENU(menu), w);
	gtk_widget_show(w);
	return(menu);
}

/*
 *	Creates a New Menu.
 */
GtkWidget *GUIMenuCreate(void)
{
	return(gtk_menu_new());
}

/*
 *	Creates a new Menu Item.
 *
 *	If accelgrp is NULL then only the accelerator key label will
 *	state the accelerator key and the accelerator key will not be
 *	added to the accelerator group.
 *
 *	If functional_widget_rtn not NULL then it will refer to the
 *	functional widget of the new Menu Item. For example, if the type
 *	was GUI_MENU_ITEM_TYPE_LABEL then *functional_widget_rtn will
 *	refer to the GtkLabel.
 */
GtkWidget *GUIMenuItemCreate(
	GtkWidget *menu,
	gui_menu_item_type type,	/* One of GUI_MENU_ITEM_TYPE_* */
	GtkAccelGroup *accelgrp,
	guint8 **icon, const gchar *label,
	guint accel_key, guint accel_mods,
	gpointer *functional_widget_rtn,
	gpointer data,
	void (*func_cb)(GtkWidget *w, gpointer)
)
{
	const gint	border_minor = 2,
			border_accel_label = 15;
	GtkWidget	*w, *menu_item = NULL,
			*parent, *parent2;
	GUIMenuItemData *mi;

	if(functional_widget_rtn != NULL)
	    *functional_widget_rtn = NULL;

	if(menu == NULL)
	    return(NULL);

	/* Create the menu item's data */
	mi = GUI_MENU_ITEM_DATA(g_malloc0(
	    sizeof(GUIMenuItemData)
	));
	if(mi == NULL)
	    return(NULL);

	/* Create the menu item by type */
	switch(type)
	{
	  case GUI_MENU_ITEM_TYPE_LABEL:
	  case GUI_MENU_ITEM_TYPE_SUBMENU:
	    /* Create a GtkMenuItem */
	    mi->menu_item = menu_item = parent = w = gtk_menu_item_new();
	    if(!GTK_WIDGET_NO_WINDOW(w))
		gtk_widget_add_events(
		    w,
		    GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK
		);
	    gtk_menu_append(GTK_MENU(menu), w);
	    gtk_object_set_data_full(
		GTK_OBJECT(w), GUI_MENU_ITEM_DATA_KEY,
		mi, GUIMenuItemDataDestroyCB
	    );
	    gtk_widget_show(w);
	    /* GtkTable for the icon and labels */
	    w = gtk_table_new(1, 3, FALSE);
	    gtk_container_add(GTK_CONTAINER(parent), w);
#if !defined(_WIN32)
	    gtk_table_set_col_spacing(
		GTK_TABLE(w), 0,
		(guint)border_minor
	    );
	    gtk_table_set_col_spacing(
		GTK_TABLE(w), 1,
		(guint)border_accel_label
	    );
#endif
	    gtk_widget_show(w);
	    parent = w;

	    /* Create the icon, if the icon is NULL then an empty
	     * fixed widget would have been returned
	     */
	    mi->icon_data = (const guint8 **)icon;
	    mi->pixmap = w = GUICreateMenuItemIcon(icon);
	    if(w != NULL)
	    {
		gtk_table_attach(
		    GTK_TABLE(parent), w,
		    0, 1,
		    0, 1,
		    GTK_SHRINK,
		    GTK_SHRINK,
		    0, 0
		);
		gtk_widget_show(w);
	    }

	    /* GtkAlignment to align the label */
	    w = gtk_alignment_new(0.0f, 0.5f, 0.0f, 0.0f);
	    gtk_table_attach(
		GTK_TABLE(parent), w,
		1, 2,
		0, 1,
		GTK_SHRINK | GTK_EXPAND | GTK_FILL,
		GTK_SHRINK,
		0, 0
	    );
	    gtk_widget_show(w);
	    parent2 = w;
	    /* Create a GtkLabel as the label */
	    mi->label_text = STRDUP((label != NULL) ? label : "");
	    mi->label = w = gtk_label_new(mi->label_text);
	    gtk_label_set_justify(GTK_LABEL(w), GTK_JUSTIFY_LEFT);
	    gtk_container_add(GTK_CONTAINER(parent2), w);
	    gtk_widget_show(w);
	    /* Record the GtkLabel as the functional widget */
	    if(functional_widget_rtn != NULL)
	        *functional_widget_rtn = w;

	    /* Set the callback for the menu item? */
	    if(func_cb != NULL)
	    {
		/* Set the menu item's activate callback */
		GUIMenuItemActivateData *d = GUI_MENU_ITEM_ACTIVATE_DATA(g_malloc0(
		    sizeof(GUIMenuItemActivateData)
		));

		d->w = w;
		d->cb = func_cb;
		d->data = data;

		gtk_object_set_data_full(
		    GTK_OBJECT(menu_item), GUI_MENU_ITEM_ACTIVATE_DATA_KEY,
		    d, GUIMenuItemActivateDataDestroyCB
		);

		gtk_signal_connect_object(
		    GTK_OBJECT(menu_item), "activate",
		    GTK_SIGNAL_FUNC(GUIMenuItemActivateCB),
		    (GtkObject *)d
		);
	    }

	    /* Create a GtkAlignment for the accelerator key label */
	    w = gtk_alignment_new(1.0f, 0.5f, 0.0f, 0.0f);
	    gtk_table_attach(
		GTK_TABLE(parent), w,
		2, 3,
		0, 1,
		GTK_SHRINK,
		GTK_SHRINK,
		0, 0
	    );
	    gtk_widget_show(w);
	    parent2 = w;
	    /* Create a GtkLabel as the accelerator key label */
	    mi->accel_label = w = gtk_label_new("");
	    gtk_label_set_justify(GTK_LABEL(w), GTK_JUSTIFY_RIGHT);
	    gtk_container_add(GTK_CONTAINER(parent2), w);

	    /* Set the accelerator key and accelerator key GtkLabel */
	    GUIMenuItemSetAccelKey(
		menu_item, accelgrp, accel_key, accel_mods
	    );
	    break;

	  case GUI_MENU_ITEM_TYPE_CHECK:
	    /* Create a GtkCheckMenuItem */
	    mi->label_text = STRDUP((label != NULL) ? label : "");
	    mi->menu_item = menu_item = parent = w =
		gtk_check_menu_item_new_with_label(mi->label_text);
	    /* Need to set default height */
	    gtk_widget_set_usize(w, -1, GUI_MENU_ITEM_DEF_HEIGHT + 2);
	    gtk_menu_append(GTK_MENU(menu), w);
	    gtk_object_set_data_full(
		GTK_OBJECT(w), GUI_MENU_ITEM_DATA_KEY,
		mi, GUIMenuItemDataDestroyCB
	    );
	    gtk_widget_show(w);

	    /* Record check menu item as the functional widget */
	    if(functional_widget_rtn != NULL)
		*functional_widget_rtn = w;

	    /* Set callback for the check menu item? */
	    if(func_cb != NULL)
	    {
		/* Set the menu item's activate callback */
		GUIMenuItemActivateData *d = GUI_MENU_ITEM_ACTIVATE_DATA(g_malloc0(
		    sizeof(GUIMenuItemActivateData)
		));

		d->w = w;
		d->cb = func_cb;
		d->data = data;

		gtk_object_set_data_full(
		    GTK_OBJECT(menu_item), GUI_MENU_ITEM_ACTIVATE_DATA_KEY,
		    d, GUIMenuItemActivateDataDestroyCB
		);

		gtk_signal_connect_object(
		    GTK_OBJECT(menu_item), "toggled",
		    GTK_SIGNAL_FUNC(GUIMenuItemActivateCB),
		    (GtkObject *)d
		);
	    }

	    /* Set accelerator key */
	    GUIMenuItemSetAccelKey(
		menu_item, accelgrp, accel_key, accel_mods
	    );
	    break;

	  case GUI_MENU_ITEM_TYPE_SEPARATOR:
	    /* Create a GtkMenuItem */
	    mi->menu_item = menu_item = parent = w = gtk_menu_item_new();
	    gtk_menu_append(GTK_MENU(menu), w);
	    gtk_object_set_data_full(
		GTK_OBJECT(w), GUI_MENU_ITEM_DATA_KEY,
		mi, GUIMenuItemDataDestroyCB
	    );
	    gtk_widget_show(w);

 	    /* Create horizontal separator */
	    w = gtk_hseparator_new();
	    gtk_container_add(GTK_CONTAINER(parent), w);
	    GTK_WIDGET_UNSET_FLAGS(GTK_WIDGET(menu_item), GTK_SENSITIVE);
	    gtk_widget_show(w);

	    /* Record horizontal separator as the functional widget */
	    if(functional_widget_rtn != NULL)
		*functional_widget_rtn = w;
	    break;

	  default:
	    GUIMenuItemDataDestroyCB(mi);
	    mi = NULL;
	    break;
	}


	return(menu_item);
}

/*
 *	Sets the Menu Item as the default item.
 */
void GUISetMenuItemDefault(GtkWidget *w)
{
#if 0
	GtkRcStyle *rcstyle;
#endif
	GUIMenuItemData *d;

	if(w == NULL)
	    return;

	d = GUI_MENU_ITEM_DATA(gtk_object_get_data(
	    GTK_OBJECT(w),
	    GUI_MENU_ITEM_DATA_KEY
	));
	if(d == NULL)
	    return;

#if 0
	rcstyle = gtk_rc_style_new();
	rcstyle->font_name = STRDUP(
	    "-*-*-bold-*-*-*-*-*-*-*-*-*-*-*"
/* "-adobe-helvetica-bold-r-normal-*-12-*-*-*-*-*-iso8859-1" */
	);
	w = d->label;
	if(w != NULL)
	    gtk_widget_modify_style(w, rcstyle);
	w = d->accel_label;
	if(w != NULL)
	    gtk_widget_modify_style(w, rcstyle);
	GTK_RC_STYLE_UNREF(rcstyle)
#endif
}

/*
 *	Sets the Menu Item's "enter_notify_event" and
 *	"leave_notify_event" signal callbacks.
 */
void GUISetMenuItemCrossingCB(
	GtkWidget *w,
	gint (*enter_cb)(GtkWidget *, GdkEvent *, gpointer),
	gpointer enter_data,
	gint (*leave_cb)(GtkWidget *, GdkEvent *, gpointer),
	gpointer leave_data
)
{
	if(w == NULL)
	    return;

	if(enter_cb != NULL)
	    gtk_signal_connect(
		GTK_OBJECT((GtkWidget *)w), "enter_notify_event",
		GTK_SIGNAL_FUNC(enter_cb), enter_data
	    );
	if(leave_cb != NULL)
	    gtk_signal_connect(
		GTK_OBJECT((GtkWidget *)w), "leave_notify_event",
		GTK_SIGNAL_FUNC(leave_cb), leave_data
	    );
}

/*
 *	Adds the Menu to the Menu Bar with the specified label.
 *
 *	Returns the Menu Bar Label or NULL on error.
 */
GtkWidget *GUIMenuAddToMenuBar(
	GtkWidget *menu_bar,
	GtkWidget *menu,
	const gchar *label,
	const GtkJustification justify
)
{
	GtkWidget *menu_bar_item;

	if((menu_bar == NULL) || (menu == NULL))
	    return(NULL);

	/* Create the menu bar item */
	if(label != NULL)
	{
	    gtk_menu_set_title(GTK_MENU(menu), label);
	    menu_bar_item = gtk_menu_item_new_with_label(label);
	}
	else
	{
	    GtkMenuShell *menu_shell = GTK_MENU_SHELL(menu_bar);
	    GList *glist = (menu_shell != NULL) ?
		menu_shell->children : NULL;
	    gchar *s = g_strdup_printf(
		"Menu %i",
		g_list_length(glist) + 1
	    );
	    gtk_menu_set_title(GTK_MENU(menu), s);
	    menu_bar_item = gtk_menu_item_new_with_label(s);
	    g_free(s);
	}

	/* Set the new menu bar item to refer to the menu */
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_bar_item), menu);

	/* Append the new menu bar item to the menu bar */
	gtk_menu_bar_append(GTK_MENU_BAR(menu_bar), menu_bar_item);

	gtk_widget_show(menu_bar_item);

	/* Set the menu bar item's justification */
	switch(justify)
	{
	  case GTK_JUSTIFY_LEFT:
	    break;
	  case GTK_JUSTIFY_RIGHT:
	    gtk_menu_item_right_justify(GTK_MENU_ITEM(menu_bar_item));
	    break;
	  case GTK_JUSTIFY_CENTER:
	  case GTK_JUSTIFY_FILL:
	    break;
	}

	return(menu_bar_item);
}

/*
 *	Sets the Menu Item's label.
 */
void GUIMenuItemSetLabel(GtkWidget *menu_item, const gchar *label)
{
	GtkWidget *w;
	GUIMenuItemData *d;

	if(menu_item == NULL)
	    return;

	d = GUI_MENU_ITEM_DATA(gtk_object_get_data(
	    GTK_OBJECT(menu_item),
	    GUI_MENU_ITEM_DATA_KEY
	));
	if(d == NULL)
	    return;

	w = d->label;

	if(label != NULL)
	{
	    /* No change in label text? */
	    const gchar *old_label = d->label_text;
	    if((old_label != NULL) ? !strcmp(old_label, label) : FALSE)
		return;

	    /* Record new label text */
	    g_free(d->label_text);
	    d->label_text = STRDUP(label);

	    /* Set new label text */
	    if((w != NULL) ? GTK_IS_LABEL(w) : FALSE)
		gtk_label_set_text(GTK_LABEL(w), d->label_text);
	}
	else
	{
	    /* Label text was already empty? */
	    if(d->label_text == NULL)
		return;

	    /* Record new label text */
	    g_free(d->label_text);
	    d->label_text = STRDUP("(null)");
    
	    /* Set new label text */
	    if((w != NULL) ? GTK_IS_LABEL(w) : FALSE)
		gtk_label_set_text(GTK_LABEL(w), d->label_text);
	}
}

/*
 *	Sets the Menu Item's pixmap.
 */
void GUIMenuItemSetPixmap(GtkWidget *menu_item, guint8 **icon_data)
{
	GdkWindow *window;
	GtkWidget *w;
	GUIMenuItemData *d;

	if((menu_item == NULL) || (icon_data == NULL))
	    return;

	d = GUI_MENU_ITEM_DATA(gtk_object_get_data(
	    GTK_OBJECT(menu_item),
	    GUI_MENU_ITEM_DATA_KEY
	));
	if(d == NULL)
	    return;

	if(gui_window_root == NULL)
	     gui_window_root = (GdkWindow *)GDK_ROOT_PARENT();
	window = gui_window_root;

	/* No change in icon data? */
	if(d->icon_data == (const guint8 **)icon_data)
	    return;

	w = d->pixmap;
	if((w != NULL) ? GTK_IS_PIXMAP(w) : FALSE)
	{
	    GdkBitmap *mask = NULL;
	    GdkPixmap *pixmap = gdk_pixmap_create_from_xpm_d(
		window,
		&mask,
		NULL,
		(gchar **)icon_data
	    );
	    if(pixmap != NULL)
	    {
		gint width, height;

		/* Record new icon data */
		d->icon_data = (const guint8 **)icon_data;

		/* Update the pixmap */
		gtk_pixmap_set(GTK_PIXMAP(w), pixmap, mask);

		/* Get size of newly loaded pixmap */
		gdk_window_get_size(pixmap, &width, &height);
	    }
	    GDK_PIXMAP_UNREF(pixmap);
	    GDK_BITMAP_UNREF(mask);
	}
}

/*
 *	Sets the Menu Item's pixmap.
 */
void GUIMenuItemSetPixmap2(
	GtkWidget *menu_item, 
	GdkPixmap *pixmap, GdkBitmap *mask
)
{
	GtkWidget *w;
	GUIMenuItemData *d;

	if(menu_item == NULL)
	    return;

	d = GUI_MENU_ITEM_DATA(gtk_object_get_data(
	    GTK_OBJECT(menu_item),
	    GUI_MENU_ITEM_DATA_KEY
	));
	if(d == NULL)
	    return;

	w = d->pixmap;
	if((w != NULL) ? GTK_IS_PIXMAP(w) : FALSE)
	{
	    /* Update the pixmap */
	    gtk_pixmap_set(GTK_PIXMAP(w), pixmap, mask);

	    /* Clear the new icon data */
	    d->icon_data = NULL;
	}
}

/*
 *	Sets the Menu Item's accelerator key.
 *
 *	If accelgrp is NULL then only the accelerator key label will be
 *	updated.
 */
void GUIMenuItemSetAccelKey(
	GtkWidget *menu_item, GtkAccelGroup *accelgrp,
	guint accel_key, guint accel_mods
)
{
	GtkWidget *w;
	GUIMenuItemData *d;

	if(menu_item == NULL)
	    return;

	d = GUI_MENU_ITEM_DATA(gtk_object_get_data(
	    GTK_OBJECT(menu_item),
	    GUI_MENU_ITEM_DATA_KEY
	));
	if(d == NULL)
	    return;

	/* No change? */
	if((d->accel_key == accel_key) &&
	   (d->accel_mods == accel_mods)
	)
	    return;
 
	/* Update the GtkAccelGroup? */
	if(accelgrp != NULL)
	{
	    /* Remove existing accelerator key? */
	    if(d->accel_key != 0)
		gtk_widget_remove_accelerator(
		    menu_item, accelgrp,
		    d->accel_key, d->accel_mods
		);
	    /* Add new accelerator key */
	    gtk_widget_add_accelerator(
		menu_item, "activate", accelgrp,
		accel_key, accel_mods,
		GTK_ACCEL_VISIBLE
	    );
	}

	/* Update the accelerator key GtkLabel */
	w = d->accel_label;
	if(w != NULL)
	{
	    GtkLabel *label = GTK_LABEL(w);

	    /* Accelerator key specified? */
	    if(accel_key != 0)
	    {
		const gchar *key_name;
		gchar *accel_text = STRDUP("");

		/* Append modifier name to accelerator text */
		if(accel_mods & GDK_LOCK_MASK)
		{
		    gchar *s = g_strconcat(accel_text, "CapsLock+", NULL);
		    g_free(accel_text);
		    accel_text = s;
		}
		if(accel_mods & GDK_CONTROL_MASK)
		{
		    gchar *s = g_strconcat(accel_text, "Ctrl+", NULL);
		    g_free(accel_text);
		    accel_text = s;
		} 
		if(accel_mods & GDK_SHIFT_MASK)
		{
		    gchar *s = g_strconcat(accel_text, "Shift+", NULL);
		    g_free(accel_text);
		    accel_text = s;
		}
		if(accel_mods & GDK_MOD1_MASK)
		{
		    gchar *s = g_strconcat(accel_text, "Alt+", NULL);
		    g_free(accel_text);
		    accel_text = s;
		}
		if(accel_mods & GDK_MOD2_MASK)
		{
		    gchar *s = g_strconcat(accel_text, "NumLock+", NULL);
		    g_free(accel_text);
		    accel_text = s;
		}
		if(accel_mods & GDK_MOD3_MASK)
		{
		    gchar *s = g_strconcat(accel_text, "Mod3+", NULL);
		    g_free(accel_text);
		    accel_text = s;
		}
		if(accel_mods & GDK_MOD4_MASK)
		{
		    gchar *s = g_strconcat(accel_text, "Mod4+", NULL);
		    g_free(accel_text);
		    accel_text = s;
		}
		if(accel_mods & GDK_MOD5_MASK)
		{
		    gchar *s = g_strconcat(accel_text, "ScrollLock+", NULL);
		    g_free(accel_text);
		    accel_text = s;
		}

		/* Append key name to accelerator key text */
		key_name = GUIGetKeyName(accel_key);
/*		key_name = gdk_keyval_name(accel_key); */
		if(*key_name != '\0')
		{
		    gchar *s = g_strconcat(accel_text, key_name, NULL);
		    g_free(accel_text);
		    accel_text = s;
		}
		else
		{
		    gchar *char_name = g_strdup_printf(
			"%c", toupper(accel_key)
		    );
		    gchar *s = g_strconcat(accel_text, char_name, NULL);
		    g_free(char_name);
		    g_free(accel_text);
		    accel_text = s;
		}

		/* Set and map accelerator key GtkLabel */
		g_free(d->accel_text);
		d->accel_text = STRDUP(accel_text);
		gtk_label_set_text(label, accel_text);
		gtk_widget_show(w);

		g_free(accel_text);
	    }
	    else
	    {
		/* Clear and unmap accelerator key GtkLabel */
		g_free(d->accel_text);
		d->accel_text = NULL;
		gtk_label_set_text(label, "");
		gtk_widget_hide(w);
	    }
	}

	/* Record new accelerator key value */
	d->accel_key = accel_key;
	d->accel_mods = accel_mods;
}

/*
 *	Sets the Check Menu Item's active state.
 */
void GUIMenuItemSetCheck(
	GtkWidget *menu_item,
	const gboolean active,
	const gboolean emit_signal
)
{
	if(menu_item == NULL)
	    return;

	if(!GTK_IS_CHECK_MENU_ITEM(menu_item))
	    return;

	if(emit_signal)
	    gtk_check_menu_item_set_active(
		GTK_CHECK_MENU_ITEM(menu_item),
		active
	    );
	else
	    GTK_CHECK_MENU_ITEM(menu_item)->active = active;
}

/*
 *	Gets the Check Menu Item's active state.
 */
gboolean GUIMenuItemGetCheck(GtkWidget *menu_item)
{
	if(menu_item == NULL)
	    return(FALSE);

	if(GTK_IS_CHECK_MENU_ITEM(menu_item))
	    return(GTK_CHECK_MENU_ITEM(menu_item)->active);
	else
	    return(FALSE);
}


/*
 *	Same as GUIMenuAddToMenuBar() except a pixmap is placed to the
 *	left of the label on the menu bar.
 */
GtkWidget *GUIMenuAddToMenuBarPixmapH(
	GtkWidget *menu_bar,
	GtkWidget *menu,
	const gchar *label,
	guint8 **icon_data,
	const GtkJustification justify
)
{
	return(GUIMenuAddToMenuBar(
	    menu_bar,
	    menu,
	    label,
	    justify
	));
}

/*
 *	Sets the menu item's sub menu.
 *
 *	If the menu item's previous submenu exists then it will be
 *	destroyed first.
 *
 *	If sub_menu is NULL then the menu item's previous submenu (if
 *	any) be destroyed and the menu item will be set as not having
 *	any submenu.
 */
void GUIMenuItemSetSubMenu(
	GtkWidget *menu_item,
	GtkWidget *sub_menu
)
{
	GtkMenuItem *mi;

	if((menu_item != NULL) ? !GTK_IS_MENU_ITEM(menu_item) : TRUE)
	    return;

	mi = GTK_MENU_ITEM(menu_item);

	if((sub_menu != NULL) ? GTK_IS_MENU(sub_menu) : FALSE)
	    gtk_menu_item_set_submenu(mi, sub_menu);
	else
	    gtk_menu_item_remove_submenu(mi);
}


/*
 *	Editable popup menu record undo.
 */
static void GUIEditablePopupMenuRecordUndo(GUIEditablePopupMenuData *d)
{
	GtkEditable *editable = GTK_EDITABLE(d->editable);

	g_free(d->undo_buf);
	d->undo_buf = gtk_editable_get_chars(editable, 0, -1);
	d->undo_last_pos = gtk_editable_get_position(editable);

	g_free(d->redo_buf);
	d->redo_buf = NULL;
	d->redo_last_pos = 0;
}

/*
 *	Editable popup menu update.
 */
static void GUIEditablePopupMenuUpdate(GUIEditablePopupMenuData *d)
{
	GtkEditable *editable = GTK_EDITABLE(d->editable);
	const gboolean has_selection = editable->has_selection;
	gboolean b;

	d->freeze_count++;

	if(d->flags & GUI_EDITABLE_POPUP_MENU_UNDO)
	{
	    b = ((d->undo_buf != NULL) || (d->redo_buf != NULL)) ?
		TRUE : FALSE;
	    GTK_WIDGET_SET_SENSITIVE(d->undo_mi, b);
	    GUIMenuItemSetLabel(
		d->undo_mi,
		(d->redo_buf != NULL) ? "Redo" : "Undo"
	    );
	    gtk_widget_show(d->undo_mi);
	    gtk_widget_show(d->undo_msep);
	}
	else
	{
	    gtk_widget_hide(d->undo_mi);
	    gtk_widget_hide(d->undo_msep);
	}

	if(d->flags & GUI_EDITABLE_POPUP_MENU_READ_ONLY)
	{
	    gtk_widget_hide(d->cut_mi);
	    gtk_widget_show(d->copy_mi);
	    gtk_widget_hide(d->paste_mi);
	    gtk_widget_hide(d->delete_mi);
	    gtk_widget_show(d->select_msep);
	    gtk_widget_show(d->select_all_mi);
	    gtk_widget_show(d->unselect_all_mi);
	}
	else
	{
	    gtk_widget_show(d->cut_mi);
	    gtk_widget_show(d->copy_mi);
	    gtk_widget_show(d->paste_mi);
	    gtk_widget_show(d->delete_mi);
	    gtk_widget_show(d->select_msep);
	    gtk_widget_show(d->select_all_mi);
	    gtk_widget_show(d->unselect_all_mi);
	}

	GTK_WIDGET_SET_SENSITIVE(d->cut_mi, has_selection);
	GTK_WIDGET_SET_SENSITIVE(d->copy_mi, has_selection);
	GTK_WIDGET_SET_SENSITIVE(d->paste_mi, TRUE);
	GTK_WIDGET_SET_SENSITIVE(d->delete_mi, has_selection);
	GTK_WIDGET_SET_SENSITIVE(d->select_all_mi, TRUE);
	GTK_WIDGET_SET_SENSITIVE(d->unselect_all_mi, TRUE);

	d->freeze_count--;
}

/*
 *	Gets the popup menu from the GtkEditable.
 */
GtkWidget *GUIEditableGetPopupMenu(GtkWidget *w)
{
	GUIEditablePopupMenuData *d;

	if(w == NULL)
	    return(NULL);

	d = GUI_EDITABLE_POPUP_MENU_DATA(gtk_object_get_data(
	    GTK_OBJECT(w),
	    GUI_EDITABLE_POPUP_MENU_DATA_KEY
	));
	if(d == NULL)
	    return(NULL);

	return(d->menu);
}

/*
 *	Endows the GtkEditable with a popup menu for editing.
 *
 *	The w specifies the GtkEditable.
 *
 *	The flags specifies the popup menu options.
 */
void GUIEditableEndowPopupMenu(
	GtkWidget *w,
	const gui_editable_popup_menu_flags flags
)
{
	guint8 **icon;
	const gchar *label;
	guint accel_key, accel_mods;
	gpointer menu_item_data;
	GtkAccelGroup *accelgrp;
	GtkWidget *menu, *menu_item;
	GUIEditablePopupMenuData *d;
	void (*func_cb)(GtkWidget *w, gpointer);

	if(w == NULL)
	    return;

	if(!GTK_IS_EDITABLE(w))
	    return;

	/* Already set? */
	d = GUI_EDITABLE_POPUP_MENU_DATA(gtk_object_get_data(
	    GTK_OBJECT(w),
	    GUI_EDITABLE_POPUP_MENU_DATA_KEY
	));
	if(d != NULL)
	    return;

	/* Allocate the editable popup menu data */
	d = GUI_EDITABLE_POPUP_MENU_DATA(g_malloc0(
	    sizeof(GUIEditablePopupMenuData)
	));
	if(d == NULL)
	    return;

	d->editable = w;
	d->freeze_count = 0;
	d->flags = flags;
	d->undo_buf = NULL;
	d->redo_buf = NULL;

	d->freeze_count++;

	gtk_object_set_data_full(
	    GTK_OBJECT(w), GUI_EDITABLE_POPUP_MENU_DATA_KEY,
	    d, GUIEditablePopupMenuDataDestroyCB
	);

	/* Right-click popup menu */
	d->menu = menu = GUIMenuCreate();
	accelgrp = NULL;
	accel_key = 0;
	accel_mods = 0;
	menu_item_data = d;
	func_cb = NULL;

#define ADD_MENU_ITEM_LABEL	{		\
 menu_item = GUIMenuItemCreate(			\
  menu,						\
  GUI_MENU_ITEM_TYPE_LABEL,			\
  accelgrp,					\
  icon, label,					\
  accel_key, accel_mods,			\
  NULL,						\
  menu_item_data, func_cb			\
 );						\
}
#define ADD_MENU_ITEM_CHECK	{		\
 menu_item = GUIMenuItemCreate(			\
  menu,						\
  GUI_MENU_ITEM_TYPE_CHECK,			\
  accelgrp,					\
  icon, label,					\
  accel_key, accel_mods,			\
  NULL,						\
  menu_item_data, func_cb			\
 );						\
}
#define ADD_MENU_ITEM_SEPARATOR	{		\
 menu_item = GUIMenuItemCreate(			\
  menu,						\
  GUI_MENU_ITEM_TYPE_SEPARATOR,			\
  NULL,						\
  NULL, NULL,					\
  0, 0,						\
  NULL,						\
  NULL, NULL					\
 );						\
}

	icon = NULL;
	label = "Undo";
	func_cb = GUIEditablePopupMenuUndoCB;
	ADD_MENU_ITEM_LABEL
	d->undo_mi = menu_item;

	ADD_MENU_ITEM_SEPARATOR
	d->undo_msep = menu_item;

	icon = (guint8 **)icon_cut_20x20_xpm;
	label =
#if defined(PROG_LANGUAGE_SPANISH)
"Corte"
#elif defined(PROG_LANGUAGE_FRENCH)
"Couper"
#elif defined(PROG_LANGUAGE_GERMAN)
"Schnitt"
#elif defined(PROG_LANGUAGE_ITALIAN)
"Taglio"
#elif defined(PROG_LANGUAGE_DUTCH)
"Snee"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"Corte"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Snitt"
#else
"Cut"
#endif
	;
	func_cb = GUIEditablePopupMenuCutCB;
	ADD_MENU_ITEM_LABEL
	d->cut_mi = menu_item;
			      
	icon = (guint8 **)icon_copy_20x20_xpm;
	label =
#if defined(PROG_LANGUAGE_SPANISH)
"Copia"
#elif defined(PROG_LANGUAGE_FRENCH)
"Copier"
#elif defined(PROG_LANGUAGE_GERMAN)
"Kopie"
#elif defined(PROG_LANGUAGE_ITALIAN)
"Copia"
#elif defined(PROG_LANGUAGE_DUTCH)
"Kopie"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"Cópia"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Kopi"
#else
"Copy"
#endif
	;
	func_cb = GUIEditablePopupMenuCopyCB;
	ADD_MENU_ITEM_LABEL
	d->copy_mi = menu_item;

	icon = (guint8 **)icon_paste_20x20_xpm;
	label =
#if defined(PROG_LANGUAGE_SPANISH)
"Pasta"
#elif defined(PROG_LANGUAGE_FRENCH)
"Coller"
#elif defined(PROG_LANGUAGE_GERMAN)
"Paste"
#elif defined(PROG_LANGUAGE_ITALIAN)
"Pasta"
#elif defined(PROG_LANGUAGE_DUTCH)
"Plakmiddel"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"Pasta"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Masse"
#else
"Paste"
#endif
	;
	func_cb = GUIEditablePopupMenuPasteCB;
	ADD_MENU_ITEM_LABEL
	d->paste_mi = menu_item;

	icon = (guint8 **)icon_cancel_20x20_xpm;
	label =
#if defined(PROG_LANGUAGE_SPANISH)
"Borre"
#elif defined(PROG_LANGUAGE_FRENCH)
"Supprimer"
#elif defined(PROG_LANGUAGE_GERMAN)
"Löschen"
#elif defined(PROG_LANGUAGE_ITALIAN)
"Cancellare"
#elif defined(PROG_LANGUAGE_DUTCH)
"Schrap"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"Anule"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Stryk"
#else
"Delete"
#endif
	;
	func_cb = GUIEditablePopupMenuDeleteCB;
	ADD_MENU_ITEM_LABEL
	d->delete_mi = menu_item;

	ADD_MENU_ITEM_SEPARATOR
	d->select_msep = menu_item;

	icon = NULL;
	label = "Select All";
	func_cb = GUIEditablePopupMenuSelectAllCB;
	ADD_MENU_ITEM_LABEL
	d->select_all_mi = menu_item;

	icon = NULL;
	label = "Unselect All";
	func_cb = GUIEditablePopupMenuUnselectAllCB;
	ADD_MENU_ITEM_LABEL
	d->unselect_all_mi = menu_item;

#undef ADD_MENU_ITEM_LABEL
#undef ADD_MENU_ITEM_CHECK
#undef ADD_MENU_ITEM_SEPARATOR

	/* Attach signals to trigger the mapping of the popup menu
	 * and to monitor change
	 */
	gtk_signal_connect(
	    GTK_OBJECT(w), "key_press_event",
	    GTK_SIGNAL_FUNC(GUIEditablePopupMenuEditableEventCB), d
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "key_release_event",
	    GTK_SIGNAL_FUNC(GUIEditablePopupMenuEditableEventCB), d
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "button_press_event",
	    GTK_SIGNAL_FUNC(GUIEditablePopupMenuEditableEventCB), d
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "button_release_event",
	    GTK_SIGNAL_FUNC(GUIEditablePopupMenuEditableEventCB), d
	);

	GUIEditablePopupMenuUpdate(d);

	d->freeze_count--;
}


/*
 *	Creates a pull out widget horizontally, returning the hbox which
 *	the calling function can pack child widgets into.
 *
 *	The given parent must be a hbox or a vbox.
 *
 *	The toplevel_width and toplevel_height specify the size of the
 *	toplevel window that will be used when this widget is `pulled
 *	out'.
 *
 *	gtk_widget_show() will be called for you, the client function need
 *	not call it.
 */
void *GUIPullOutCreateH(
	void *parent_box,
	int homogeneous, int spacing,		/* Of client vbox */
	int expand, int fill, int padding,	/* Of holder hbox */
	int toplevel_width, int toplevel_height,
	void *pull_out_client_data,
	void (*pull_out_cb)(void *, void *),
	void *push_in_client_data,
	void (*push_in_cb)(void *, void *)
)
{
	gpointer *cb_data;
	GtkWidget *pull_out_btn, *holder_hbox, *client_hbox;

	if(parent_box == NULL)
	    return(NULL);

	/* Create a hbox to place into the given parent box. This hbox
	 * will hold the parenting hbox plus some other widgets.
	 */
	holder_hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(
	    (GtkBox *)parent_box, holder_hbox,
	    expand, fill, padding
	);
	gtk_widget_show(holder_hbox);

	/* Create pull out button and parent it to the holder hbox.
	 * Note that this is not really a button but an event box.
	 */
	pull_out_btn = gtk_event_box_new();
	gtk_box_pack_start(
	    GTK_BOX(holder_hbox), pull_out_btn, FALSE, FALSE, 0
	);
	gtk_widget_set_usize(pull_out_btn, 10, -1);
	gtk_widget_show(pull_out_btn);


	/* Create the client hbox, which will be parented to the above
	 * holder hbox when `placed in' and reparented to a toplevel
	 * GtkWindow when `pulled out'.
	 */
	client_hbox = gtk_hbox_new(homogeneous, spacing);
	gtk_box_pack_start(
	    GTK_BOX(holder_hbox), client_hbox, TRUE, TRUE, 0
	);
	gtk_widget_show(client_hbox);


	/* Set callbacks and callback data */

	/* Allocate and set up callback data */
	cb_data = (gpointer *)g_malloc0(13 * sizeof(gpointer));
	if(cb_data != NULL)
	{
	    /* Format is as follows (13 arguments):
	     *
	     *	client_hbox
	     *	holder_hbox
	     *	holder_window
	     *	holder_window_x
	     *	holder_window_y
	     *	holder_window_width
	     *	holder_window_height
	     *	in_place		(1 if true).
	     *	pull_out_client_data
	     *	pull_out_cb
	     *	push_in_client_data
	     *	push_in_cb
	     */
	    cb_data[0] = (gpointer)client_hbox;
	    cb_data[1] = (gpointer)holder_hbox;
	    cb_data[2] = NULL;
	    cb_data[3] = 0;
	    cb_data[4] = 0;
	    cb_data[5] = (gpointer)MAX(toplevel_width, 0);
	    cb_data[6] = (gpointer)MAX(toplevel_height, 0);
	    cb_data[7] = (gpointer)1;		/* Initially `pushed in' */
	    cb_data[8] = (gpointer)pull_out_client_data;
	    cb_data[9] = (gpointer)pull_out_cb;
	    cb_data[10] = (gpointer)push_in_client_data;
	    cb_data[11] = (gpointer)push_in_cb;
	    cb_data[12] = NULL;
	    gtk_object_set_data_full(
		GTK_OBJECT(client_hbox), GUI_PULLOUT_DATA_KEY,
		cb_data, GUIPullOutDataDestroyCB
	    );
	}

	gtk_signal_connect(
	    GTK_OBJECT(pull_out_btn), "button_press_event",
	    GTK_SIGNAL_FUNC(GUIPullOutPullOutBtnCB), cb_data
	);
	gtk_signal_connect_after(
	    GTK_OBJECT(pull_out_btn), "expose_event",
	    GTK_SIGNAL_FUNC(GUIPullOutDrawHandleCB), NULL
	);
	gtk_signal_connect(
	    GTK_OBJECT(pull_out_btn), "draw",
	    GTK_SIGNAL_FUNC(GUIPullOutDrawHandleDrawCB), NULL
	);

	return(client_hbox);
}

/*
 *	Returns the pointer to the toplevel window (if any) of the
 *	client_hbox and the geometry of that window.
 *
 *	The client box should be one created by GUIPullOutCreate*().
 */
void *GUIPullOutGetToplevelWindow(
	void *client_box,
	int *x, int *y, int *width, int *height
)
{
	gpointer *cb_data;
	GtkWidget *w = NULL;

	if(x != NULL)
	    *x = 0;
	if(y != NULL)
	    *y = 0;
	if(width != NULL)
	    *width = 0;
	if(height != NULL)
	    *height = 0;

	if(client_box == NULL)
	    return(w);

	cb_data = (gpointer *)gtk_object_get_data(
	    GTK_OBJECT(client_box),
	    GUI_PULLOUT_DATA_KEY
	);
	if(cb_data != NULL)
	{
	    /* Format is as follows (13 arguments):
	     *
	     *	client_hbox
	     *	holder_hbox
	     *	holder_window
	     *	holder_window_x
	     *	holder_window_y
	     *	holder_window_width
	     *	holder_window_height
	     *	in_place		(1 if true)
	     *	pull_out_client_data
	     *	pull_out_cb
	     *	push_in_client_data
	     *	push_in_cb   
	     */
	    w = (GtkWidget *)cb_data[2];

	    if((w != NULL) ? !GTK_WIDGET_NO_WINDOW(w) : FALSE)
	    {
		GdkWindow *window = w->window;
		gint rx, ry, rwidth, rheight, rdepth;

		gdk_window_get_geometry(
		    window,
		    &rx, &ry, &rwidth, &rheight,
		    &rdepth
		);

		if(x != NULL)
		    *x = rx;
		if(y != NULL)
		    *y = ry;
		if(width != NULL)
		    *width = rwidth;
		if(height != NULL)
		    *height = rheight;
	    }
	}

	return(w);
}

/*
 *	Pulls out the pull out box.
 */
void GUIPullOutPullOut(void *client_box)
{
	gpointer *cb_data;

	if(client_box == NULL)
	    return;

	cb_data = (gpointer *)gtk_object_get_data(
	    GTK_OBJECT(client_box),
	    GUI_PULLOUT_DATA_KEY
	);
	if(cb_data != NULL)
	{
	    /* Format is as follows (13 arguments):
	     *
	     *	client_hbox
	     *	holder_hbox
	     *	holder_window
	     *	holder_window_x
	     *	holder_window_y
	     *	holder_window_width
	     *	holder_window_height
	     *	in_place		(1 if true)
	     *	pull_out_client_data
	     *	pull_out_cb
	     *	push_in_client_data
	     *	push_in_cb
	     */

	    /* In place (pushed in)? */
	    if((gint)cb_data[7])
	    {
		GUIPullOutPullOutBtnCB(
		    NULL,
		    NULL,
		    (gpointer)cb_data
		);
	    }
	}
}

/*
 *	Pushes in the pull out box.
 */
void GUIPullOutPushIn(void *client_box)
{
	GtkWidget *w;
	gpointer *cb_data;

	if(client_box == NULL)
	    return;

	cb_data = (gpointer *)gtk_object_get_data(
	    GTK_OBJECT(client_box),
	    GUI_PULLOUT_DATA_KEY
	);
	if(cb_data != NULL)
	{
	    /* Format is as follows (13 arguments):
	     *
	     *	client_hbox
	     *	holder_hbox
	     *	holder_window
	     *	holder_window_x
	     *	holder_window_y
	     *	holder_window_width
	     *	holder_window_height
	     *	in_place		(1 if true)
	     *	pull_out_client_data
	     *	pull_out_cb
	     *	push_in_client_data
	     *	push_in_cb
	     */

	    w = (GtkWidget *)cb_data[2];

	    /* Not in place (pulled out)? */
	    if(!((gint)cb_data[7]))
	    {
		GUIPullOutCloseCB(
		    (GtkWidget *)cb_data[2],
		    NULL,
		    (gpointer)cb_data
		);
	    }
	}
}
