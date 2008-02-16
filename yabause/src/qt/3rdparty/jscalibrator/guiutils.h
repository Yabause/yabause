#ifndef GUIUTILS_H
#define GUIUTILS_H

#include <gtk/gtk.h>


/*
 *	GTK 2 wrappers:
 */
#if (GTK_MAJOR_VERSION >= 2)

#define GTK_WINDOW_DIALOG		GTK_WINDOW_TOPLEVEL

#define gtk_accel_group_add(_a_,_c_,_m_,_f_,_w_,_s_)	\
 gtk_widget_add_accelerator(				\
  (GtkWidget *)(_w_), (_s_), (_a_),			\
  (_c_), (_m_), (_f_)					\
 )



#endif	/* End of GTK 2 wrappers */


/*
 *	Standard Target Names:
 */
#define GUI_TARGET_NAME_STRING		"STRING"
#define GUI_TARGET_NAME_TEXT_PLAIN	"text/plain"
#define GUI_TARGET_NAME_TEXT_URI_LIST	"text/uri-list"


/*
 *	Standard Type Names:
 */
#define GUI_TYPE_NAME_STRING		"STRING"


/*
 *	Button Numbers:
 */
#ifndef GDK_BUTTON1
# define GDK_BUTTON1		1
#endif
#ifndef GDK_BUTTON2
# define GDK_BUTTON2		2
#endif
#ifndef GDK_BUTTON3
# define GDK_BUTTON3		3
#endif
#ifndef GDK_BUTTON4
# define GDK_BUTTON4		4
#endif
#ifndef GDK_BUTTON5
# define GDK_BUTTON5		5
#endif
#ifndef GDK_BUTTON6
# define GDK_BUTTON6		6
#endif
#ifndef GDK_BUTTON7
# define GDK_BUTTON7		7
#endif
#ifndef GDK_BUTTON8
# define GDK_BUTTON8		8
#endif
#ifndef GDK_BUTTON9
# define GDK_BUTTON9		9
#endif
#ifndef GDK_BUTTON10
# define GDK_BUTTON10		10
#endif
#ifndef GDK_BUTTON11
# define GDK_BUTTON11		11
#endif
#ifndef GDK_BUTTON12
# define GDK_BUTTON12		12
#endif
#ifndef GDK_BUTTON13
# define GDK_BUTTON13		13
#endif
#ifndef GDK_BUTTON14
# define GDK_BUTTON14		14
#endif
#ifndef GDK_BUTTON15
# define GDK_BUTTON15		15
#endif
#ifndef GDK_BUTTON16
# define GDK_BUTTON16		16
#endif


/*
 *	Geometry Flags:
 */
typedef enum {
	GDK_GEOMETRY_X			= (1 << 0),
	GDK_GEOMETRY_Y			= (1 << 1),
	GDK_GEOMETRY_WIDTH		= (1 << 2),
	GDK_GEOMETRY_HEIGHT		= (1 << 3)
} GdkGeometryFlags;

/*
 *	String Bounds:
 */
typedef struct {
	gint		lbearing,
			rbearing,
			width,
			ascent,
			descent;
} GdkTextBounds;

/*
 *	DND Drag Icon Defaults:
 */
#define GUI_DND_DRAG_ICON_DEF_HOT_X		-5
#define GUI_DND_DRAG_ICON_DEF_HOT_Y		-5

#define GUI_DND_DRAG_ICON_DEF_ITEM_SPACING	1
#define GUI_DND_DRAG_ICON_DEF_NITEMS_VISIBLE	5

#define GUI_DND_DRAG_ICON_DEF_SHADOW_OFFSET_X	5
#define GUI_DND_DRAG_ICON_DEF_SHADOW_OFFSET_Y	5

/*
 *	Button default sizes:
 */
#define GUI_BUTTON_HLABEL_WIDTH			95
#define GUI_BUTTON_HLABEL_HEIGHT		-1

#define GUI_BUTTON_VLABEL_WIDTH			60
#define GUI_BUTTON_VLABEL_HEIGHT		50

/* With GTK_CAN_DEFAULT border */
#define GUI_BUTTON_HLABEL_WIDTH_DEF		(100 + (2 * 3))
#define GUI_BUTTON_HLABEL_HEIGHT_DEF		(30 + (2 * 3))


/*
 *	Menu Item default size:
 */
#define GUI_MENU_ITEM_DEF_WIDTH			-1
#define GUI_MENU_ITEM_DEF_HEIGHT		20

/*
 *	Menu Item Types:
 */
typedef enum {
	GUI_MENU_ITEM_TYPE_LABEL,	/* GtkMenuItem (active) */
	GUI_MENU_ITEM_TYPE_CHECK,	/* GtkCheckMenuItem */
	GUI_MENU_ITEM_TYPE_SUBMENU,	/* GtkMenuItem (refering toa submenu) */
	GUI_MENU_ITEM_TYPE_SEPARATOR	/* GtkSeparator */
} gui_menu_item_type;

/*
 *	Menu Bar Item Alignment:
 *
 *	Depreciated.
 */
#define GUI_MENU_BAR_ALIGN_LEFT		GTK_JUSTIFY_LEFT
#define GUI_MENU_BAR_ALIGN_RIGHT	GTK_JUSTIFY_RIGHT
#define GUI_MENU_BAR_ALIGN_CENTER	GTK_JUSTIFY_CENTER

/*
 *	GtkEditable Popup Menu Flags:
 */
typedef enum {
	GUI_EDITABLE_POPUP_MENU_READ_ONLY	= (1 << 0),
	GUI_EDITABLE_POPUP_MENU_UNDO		= (1 << 1)	/* And redo */
} gui_editable_popup_menu_flags;


/*
 *	String Macros:
 */
#define g_strcasepfx(_str_,_pfx_)			\
(!g_strncasecmp((_str_),(_pfx_),strlen(_pfx_)))


/*
 *	Colormap & Color Macros:
 */
#define GDK_COLOR_SET_BYTE(_c_,_r_,_g_,_b_)		\
{ if((_c_) != NULL) {					\
 (_c_)->red	= ((gushort)(_r_) << 8) |		\
		  ((gushort)(_r_));			\
 (_c_)->green	= ((gushort)(_g_) << 8) |		\
		  ((gushort)(_g_));			\
 (_c_)->blue	= ((gushort)(_b_) << 8) |		\
		  ((gushort)(_b_));			\
} }
#define GDK_COLOR_SET_COEFF(_c_,_r_,_g_,_b_)		\
{ if((_c_) != NULL) {					\
 const gfloat m = (gfloat)((gushort)-1);		\
 (_c_)->red	= (gushort)((gfloat)(_r_) * m);		\
 (_c_)->green	= (gushort)((gfloat)(_g_) * m);		\
 (_c_)->blue	= (gushort)((gfloat)(_b_) * m);		\
} }

#define GDK_COLORMAP_ALLOC_COLOR(_cmap_,_c_)	\
{ if(((_cmap_) != NULL) && ((_c_) != NULL))	\
 gdk_colormap_alloc_color((_cmap_), (_c_), TRUE, TRUE); \
}
#define GDK_COLORMAP_ALLOC_COLORS(_cmap_,_c_,_n_)\
{ if(((_cmap_) != NULL) && ((_c_) != NULL) &&	\
     ((_n_) > 0)) {				\
 gboolean status;				\
 gdk_colormap_alloc_colors(			\
  (_cmap_), (_c_), (_n_), TRUE, TRUE, &status	\
 );						\
} }

#define GDK_COLORMAP_FREE_COLOR(_cmap_,_c_)	\
{ if(((_cmap_) != NULL) && ((_c_) != NULL))	\
 gdk_colormap_free_colors((_cmap_), (_c_), 1);	\
}
#define GDK_COLORMAP_FREE_COLORS(_cmap_,_c_,_n_)	\
{ if(((_cmap_) != NULL) && ((_c_) != NULL))		\
 gdk_colormap_free_colors((_cmap_), (_c_), (_n_));	\
}

/*
 *	GdkFont, GdkBitmap, GdkPixmap, GdkCursor, and GdkGC
 *	Macros:
 */
#define GDK_FONT_REF(_p_)	(			\
 ((_p_) != NULL) ? gdk_font_ref(_p_) : NULL		\
)
#define GDK_FONT_UNREF(_p_)	\
{ if((_p_) != NULL) gdk_font_unref(_p_); }
#define GDK_FONT_GET_FONT_NAME_SIZE(_p_)	(	\
 ((_p_) != NULL) ?					\
  (((_p_)->ascent + (_p_)->descent) - 2) : 0		\
)

#define GDK_COLORMAP_REF(_p_)	(			\
 ((_p_) != NULL) ? gdk_colormap_ref(_p_) : NULL		\
)
#define GDK_COLORMAP_UNREF(_p_)	\
{ if((_p_) != NULL) gdk_colormap_unref(_p_); }

#define GDK_BITMAP_REF(_p_)	(			\
 ((_p_) != NULL) ? gdk_bitmap_ref(_p_) : NULL		\
)
#define GDK_BITMAP_UNREF(_p_)	\
{ if((_p_) != NULL) gdk_bitmap_unref(_p_); }

#define GDK_PIXMAP_REF(_p_)	(			\
 ((_p_) != NULL) ? gdk_pixmap_ref(_p_) : NULL		\
)
#define GDK_PIXMAP_UNREF(_p_)	\
{ if((_p_) != NULL) gdk_pixmap_unref(_p_); }

#define GDK_CURSOR_DESTROY(_p_)	\
{ if((_p_) != NULL) gdk_cursor_destroy(_p_); }

#define GDK_GC_REF(_p_)		(			\
 ((_p_) != NULL) ? gdk_gc_ref(_p_) : NULL		\
)
#define GDK_GC_UNREF(_p_)	\
{ if((_p_) != NULL) gdk_gc_unref(_p_); }


/*
 *	GtkObject Macros:
 */
#define GTK_OBJECT_REF				GUIObjectRef
#define GTK_OBJECT_UNREF(_o_)			\
{ if((_o_) != NULL) gtk_object_unref(GTK_OBJECT(_o_)); }
#define GTK_OBJECT_GET_DATA(_o_,_key_)		\
 (((_o_) != NULL) ? gtk_object_get_data((GtkObject *)(_o_), (_key_)) : NULL)

/*
 *	GtkSignal Macros:
 */
#define GTK_SIGNAL_DISCONNECT(_o_,_i_)		\
{ if(((_o_) != NULL) && ((_i_) > 0)) {		\
 GtkSignalQuery *q = gtk_signal_query(_i_);	\
 if(q != NULL) {				\
  g_free(q);					\
  gtk_signal_disconnect(GTK_OBJECT(_o_), (_i_));\
 }						\
} }
#define GTK_TIMEOUT_REMOVE(_i_)			\
{ if((_i_) > 0)					\
 gtk_timeout_remove(_i_);			\
}
#define GTK_IDLE_REMOVE(_i_)			\
{ if((_i_) > 0)					\
 gtk_idle_remove(_i_);				\
}



/*
 *	GtkStyle & GtkRcStyle Macros:
 */
#define GTK_RC_STYLE_UNREF(_s_)		\
{ if((_s_) != NULL) gtk_rc_style_unref(_s_); }
#define GTK_STYLE_UNREF(_s_)		\
{ if((_s_) != NULL) gtk_style_unref(_s_); }

/*
 *	GtkAccelGroup Macros:
 */
#define GTK_ACCEL_GROUP_UNREF(_a_)	\
{ if((_a_) != NULL) gtk_accel_group_unref(_a_); }

/*
 *      GtkAdjustment Macros:
 */
#define GTK_ADJUSTMENT_GET_VALUE(_a_)	(((_a_) != NULL) ? \
 ((GtkAdjustment *)(_a_))->value : 0.0f)

/*
 *      GtkWidget Macros:
 */
#define GTK_WIDGET_REF(_w_)			\
{ if((_w_) != NULL) gtk_widget_ref(_w_); }
#define GTK_WIDGET_UNREF(_w_)			\
{ if((_w_) != NULL) gtk_widget_unref(_w_); }
#define GTK_WIDGET_SET_SENSITIVE(_w_,_b_)	\
{ if((_w_) != NULL) gtk_widget_set_sensitive((_w_), (_b_)); }
#define GTK_WIDGET_DESTROY(_w_)			\
{ if((_w_) != NULL) gtk_widget_destroy(_w_); }

/*
 *	GtkToggleButton Macros:
 */
#define GTK_TOGGLE_BUTTON_GET_ACTIVE(_w_)	\
 (((_w_) != NULL) ? GTK_TOGGLE_BUTTON(_w_)->active : FALSE)
#define GTK_TOGGLE_BUTTON_SET_ACTIVE(_w_,_b_)	\
{ if((_w_) != NULL)				\
 gtk_toggle_button_set_active(			\
  GTK_TOGGLE_BUTTON(_w_), (_b_)			\
 );						\
}

/*
 *	GtkCheckMenuItem Macros:
 */
#define GTK_CHECK_MENU_ITEM_GET_ACTIVE(_w_)	\
 (((_w_) != NULL) ? GTK_CHECK_MENU_ITEM(_w_)->active : FALSE)
#define GTK_CHECK_MENU_ITEM_SET_ACTIVE(_w_,_b_)	\
{ if((_w_) != NULL)				\
 gtk_check_menu_item_set_active(		\
  GTK_CHECK_MENU_ITEM(_w_), (_b_)		\
 );						\
}

/*
 *	GtkRange Macros:
 */
#define GTK_RANGE_GET_ADJUSTMENT(_w_)		\
 (((_w_) != NULL) ? gtk_range_get_adjustment(GTK_RANGE(_w_)) : NULL)

/*
 *	GtkEntry Macros:
 */
#define GTK_ENTRY_GET_TEXT(_w_)			\
 (((_w_) != NULL) ? gtk_entry_get_text(GTK_ENTRY(_w_)) : NULL)
#define GTK_ENTRY_GET_VALUE	GTK_ENTRY_GET_TEXT
#define GTK_ENTRY_GET_VALUEI(_w_)		\
 (((_w_) != NULL) ? atoi(gtk_entry_get_text(GTK_ENTRY(_w_))) : 0)
#define GTK_ENTRY_GET_VALUEL(_w_)		\
 (((_w_) != NULL) ? atol(gtk_entry_get_text(GTK_ENTRY(_w_))) : 0l)
#define GTK_ENTRY_GET_VALUEF(_w_)		\
 (((_w_) != NULL) ? atof(gtk_entry_get_text(GTK_ENTRY(_w_))) : 0.0f)


/* GtkRcStyle Utilities */
#if (GTK_MAJOR_VERSION == 1)
# define gtk_rc_style_copy	GUIRCStyleCopy
#endif
extern GtkRcStyle *GUIRCStyleCopy(const GtkRcStyle *rcstyle);
#define gtk_widget_modify_style_recursive	GUIRCStyleSetRecursive
extern void GUIRCStyleSetRecursive(GtkWidget *w, GtkRcStyle *rcstyle);


/* Geometry Utilities */
#define gdk_parse_geometry	GUIParseGeometry
extern GdkGeometryFlags GUIParseGeometry(
	const gchar *s,
	gint *x, gint *y, gint *width, gint *height
);
#define gtk_clist_get_cell_geometry	GUICListGetCellGeometry
extern gboolean GUICListGetCellGeometry(
	GtkCList *clist, const gint column, const gint row,
	gint *x, gint *y, gint *width, gint *height
);
#define gtk_ctree_get_node_row_index	GUICTreeNodeRow
extern gint GUICTreeNodeRow(GtkCTree *ctree, GtkCTreeNode *node);

extern gint GUICTreeNodeDeltaRows(
	GtkCTree *ctree,
	GtkCTreeNode *start,	/* Use NULL for first/toplevel node */
	GtkCTreeNode *end
);
#define gdk_window_get_root_position	GUIGetWindowRootPosition
extern void GUIGetWindowRootPosition(
	GdkWindow *window, gint *x, gint *y
);
extern void GUIGetPositionRoot(
	GdkWindow *w, gint x, gint y, gint *rx, gint *ry
);


/* String Utilities */
#define gdk_text_bounds		GUIGetTextBounds
extern void GUIGetTextBounds(
	GdkFont *font, const gchar *text, const gint text_length,
	GdkTextBounds *bounds
);
#define gdk_string_bounds	GUIGetStringBounds
extern void GUIGetStringBounds(
	GdkFont *font, const gchar *string,
	GdkTextBounds *bounds
);


/* GdkGC utilities */
extern GdkGC *GDK_GC_NEW(void);


/* Block/Unblock Input */
extern void GUIBlockInput(
	GtkWidget *w,
	const gboolean block
);
extern gint GUIBlockInputGetLevel(GtkWidget *w);


/* GdkWindow utilities */
#define gdk_window_get_ref_count	GUIWindowGetRefCount
extern gint GUIWindowGetRefCount(GdkWindow *window);


/* WM Utilities */
extern void GUISetWMIcon(GdkWindow *window, guint8 **data);
extern void GUISetWMIconFile(GdkWindow *window, const gchar *filename);


/* GtkWindow Utilities */
#define gtk_window_apply_args GUIWindowApplyArgs
extern void GUIWindowApplyArgs(GtkWindow *w, const gint argc, gchar **argv);
#define gtk_is_window_arg GUIIsWindowArg
extern gboolean GUIIsWindowArg(const gchar *arg);


/* GtkCTree Utilities */
extern gboolean GUICTreeOptimizeExpandPosition(
	GtkCTree *ctree,
	GtkCTreeNode *node      /* Expanded parent node */
);


/* Bitmap Utilities */
extern GdkBitmap *GUICreateBitmapFromDataRGBA(
	const gint width, const gint height,
	const gint bpl,
	const guint8 *rgba,
	const guint8 threshold,
	GdkWindow *window
);

/* Pixmap Utilities */
extern GdkPixmap *GDK_PIXMAP_NEW(gint width, gint height);
extern GdkPixmap *GDK_PIXMAP_NEW_FROM_XPM_DATA(
	GdkBitmap **mask_rtn, guint8 **data
); 
extern GdkPixmap *GDK_PIXMAP_NEW_FROM_XPM_FILE(
	GdkBitmap **mask_rtn, const gchar *filename
);
extern GtkWidget *gtk_pixmap_new_from_xpm_d(
	GdkWindow *window, GdkColor *transparent_color,
	guint8 **data
);
extern GtkWidget *GUICreateMenuItemIcon(guint8 **icon_data);


/* Objects */
extern GtkObject *GUIObjectRef(GtkObject *o);


/* Adjustments */
#define GTK_ADJUSTMENT_SET_VALUE(_adj_,_v_)	\
	GUIAdjustmentSetValue((_adj_), (_v_), TRUE)
extern void GUIAdjustmentSetValue(
        GtkAdjustment *adj,
        const gfloat v,
        const gboolean emit_value_changed
);


/* Widget & Window Mapping */
#define gtk_widget_show_raise   GUIWidgetMapRaise
extern void GUIWidgetMapRaise(GtkWidget *w);
#define gtk_window_minimize     GUIWindowMinimize
extern void GUIWindowMinimize(GtkWindow *window);


/* Tool Tips */
extern void GUISetWidgetTip(GtkWidget *w, const gchar *tip);
extern void GUIShowTipsNow(GtkWidget *w);
extern void GUISetGlobalTipsState(const gboolean state);


/* Buttons Modifying */
extern void GUIButtonChangeLayout(
	GtkWidget *w,
	const gboolean show_pixmap,
	const gboolean show_label
);
extern void GUIButtonLabelUnderline(
	GtkWidget *w,
	const guint c
);
extern void GUIButtonPixmapUpdate(
	GtkWidget *w,
	guint8 **icon,
	const gchar *label
);
extern void GUIButtonArrowUpdate(
	GtkWidget *w,
	const GtkArrowType arrow_type,
	const gint arrow_width, const gint arrow_height,
	const gchar *label
);
extern GtkWidget *GUIButtonGetMainBox(GtkWidget *w);
extern GtkWidget *GUIButtonGetLabel(GtkWidget *w);
extern GtkWidget *GUIButtonGetPixmap(GtkWidget *w);
extern GtkWidget *GUIButtonGetArrow(GtkWidget *w);
extern const gchar *GUIButtonGetLabelText(GtkWidget *w);

/* Buttons With Icon & Label */
extern GtkWidget *GUIButtonPixmap(guint8 **icon);
extern GtkWidget *GUIButtonPixmapLabelH(
	guint8 **icon,
	const gchar *label,
	GtkWidget **label_rtn
);
extern GtkWidget *GUIButtonPixmapLabelV(
	guint8 **icon,
	const gchar *label,
	GtkWidget **label_rtn
);

/* Buttons With Arrow & Label */
extern GtkWidget *GUIButtonArrowLabelH(
	const GtkArrowType arrow_type,
	const gint arrow_width, const gint arrow_height,
	const gchar *label,
	GtkWidget **label_rtn
);
extern GtkWidget *GUIButtonArrowLabelV(
	const GtkArrowType arrow_type,
	const gint arrow_width, const gint arrow_height,
	const gchar *label,
	GtkWidget **label_rtn
);
extern GtkWidget *GUIButtonArrow(
	const GtkArrowType arrow_type,
	const gint arrow_width, const gint arrow_height
);

/* Toggle Buttons With Icon & Label */
extern GtkWidget *GUIToggleButtonPixmap(guint8 **icon);
extern GtkWidget *GUIToggleButtonPixmapLabelH(
	guint8 **icon,
	const gchar *label,
	GtkWidget **label_rtn
);
extern GtkWidget *GUIToggleButtonPixmapLabelV(
	guint8 **icon,
	const gchar *label,
	GtkWidget **label_rtn
);


/* Prompts With Icon, Label, Entry, and Browse */
extern GtkWidget *GUIPromptBarWithBrowse(
	guint8 **icon,
	const gchar *label,
	gpointer *label_rtn,
	gpointer *entry_rtn,
	gpointer *browse_rtn,
	gpointer browse_data,
	void (*browse_cb)(GtkWidget *, gpointer)
);
extern GtkWidget *GUIPromptBar(
	guint8 **icon,
	const gchar *label,
	gpointer *label_rtn,
	gpointer *entry_rtn
);

/* GtkTargetEntry */
extern GtkTargetEntry *gtk_target_entry_new(void);
extern GtkTargetEntry *gtk_target_entry_copy(const GtkTargetEntry *entry);
extern void gtk_target_entry_delete(GtkTargetEntry *entry);

/* Dynamic Data Exchange */
extern void GUIDDESetDirect(
	GtkWidget *w,
	GdkAtom selection_atom,
	const gulong t,
	GdkAtom target_atom,
	guint8 *data, const gint length
);
extern void GUIDDESet(
	GtkWidget *w,
	GdkAtom selection_atom,
	const gulong t,
	GdkAtom target_atom,
	const guint8 *data, const gint length
);
extern guint8 *GUIDDEGet(
	GtkWidget *w,
	GdkAtom selection_atom,
	const gulong t,
	GdkAtom target_atom,
	gint *length
);
extern void GUIDDEClear(GtkWidget *w);
extern void GUIDDESetBinary(
	GtkWidget *w,
	GdkAtom selection_atom,
	const gulong t,
	const guint8 *data, const gint length
);
extern guint8 *GUIDDEGetBinary(
	GtkWidget *w,
	GdkAtom selection_atom,
	const gulong t,
	gint *length
);
extern void GUIDDESetString(
	GtkWidget *w,
	GdkAtom selection_atom,
	const gulong t,
	const gchar *data
);
extern gchar *GUIDDEGetString(
	GtkWidget *w,
	GdkAtom selection_atom,
	const gulong t
);


/* Drag & Drop Utilities */
extern void GUIDNDSetSrc(
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
);
extern void GUIDNDSetSrcTypes(
	GtkWidget *w,
	const GtkTargetEntry *types_list, const gint ntypes,
	const GdkDragAction allowed_actions,
	const GdkModifierType buttons
);
extern void GUIDNDSetTar(
	GtkWidget *w,
	const GtkTargetEntry *types_list, const gint ntypes,
	const GdkDragAction allowed_actions,
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
extern void GUIDNDSetTarTypes(
	GtkWidget *w,
	const GtkTargetEntry *types_list, const gint ntypes,
	const GdkDragAction allowed_actions,
	const GdkDragAction default_action_same,
	const GdkDragAction default_action,
	const gboolean highlight
);
extern void GUIDNDGetTarActions(
	GtkWidget *w,
	GdkDragAction *allowed_actions_rtn,
	GdkDragAction *default_action_same_rtn,
	GdkDragAction *default_action_rtn
);
extern GdkDragAction GUIDNDDetermineDragAction(
	GdkDragAction requested_actions,
	GdkDragAction allowed_actions,
	GdkDragAction default_action
);
extern gboolean GUIDNDIsTargetAccepted(
	GtkWidget *w,
	const gchar *name,
	const GtkTargetFlags flags
);
extern void GUIDNDSetDragIconFromCells(
	GtkWidget *w,
	GList *cells_list
);
extern void GUIDNDSetDragIconFromCellsList(
        GtkWidget *w,
        GList *cells_list,
	const gint nitems,
        const GtkOrientation layout_orientation,
        const GtkOrientation icon_text_orientation,
	const gint item_spacing
);
extern void GUIDNDSetDragIconFromCListSelection(GtkCList *clist);
extern void GUIDNDSetDragIconFromCListCell(
	GtkCList *clist,
	const gint row, const gint column
);
extern void GUIDNDSetDragIconFromCList(GtkCList *clist);
extern void GUIDNDSetDragIconFromCTreeSelection(GtkCTree *ctree);
extern void GUIDNDSetDragIconFromCTreeNode(
	GtkCTree *ctree,
	GtkCTreeNode *node,
	const gint column
);
extern void GUIDNDSetDragIcon(
	GdkPixmap *pixmap, GdkBitmap *mask,
	const gint hot_x, const gint hot_y
);


/* Banners */
extern GtkWidget *GUIBannerCreate(
	const gchar *label,
	const gchar *font_name,
	GdkColor color_fg,
	GdkColor color_bg,
	const GtkJustification justify,
	const gboolean expand
);
extern void GUIBannerDraw(GtkWidget *w);


/* Combo With Label, Combo, Initial Value, and Initial List */
extern GtkWidget *GUIComboCreate(
	const gchar *label,	/* Label */
	const gchar *text,	/* Entry Value */
	GList *list,		/* Combo List */
	gint max_items,		/* Maximum Items In Combo List */
	gpointer *combo_rtn,	/* GtkCombo Return */
	gpointer data,
	void (*func_cb)(GtkWidget *w, gpointer),
	void (*list_change_cb)(GtkWidget *, gpointer, GList *)
);
extern void GUIComboActivateValue(GtkWidget *w, const gchar *value);
extern void GUIComboAddItem(GtkWidget *w, const gchar *value);
extern GList *GUIComboGetList(GtkWidget *w);
extern void GUIComboSetList(GtkWidget *w, GList *list);
extern void GUIComboClearAll(GtkWidget *w);


/* Menu Bar & Menu Items */
extern GtkWidget *GUIMenuBarCreate(GtkAccelGroup **accelgrp_rtn);
extern GtkWidget *GUIMenuCreateTearOff(void);
extern GtkWidget *GUIMenuCreate(void);
extern GtkWidget *GUIMenuItemCreate(
	GtkWidget *menu,
	gui_menu_item_type type,	/* One of GUI_MENU_ITEM_TYPE_* */
	GtkAccelGroup *accelgrp,
	guint8 **icon, const gchar *label,
	guint accel_key, guint accel_mods,
	gpointer *functional_widget_rtn,
	gpointer data,
	void (*func_cb)(GtkWidget *w, gpointer)
);
extern void GUISetMenuItemDefault(GtkWidget *w);
extern void GUISetMenuItemCrossingCB(
	GtkWidget *w,
	gint (*enter_cb)(GtkWidget *, GdkEvent *, gpointer),
	gpointer enter_data,
	gint (*leave_cb)(GtkWidget *, GdkEvent *, gpointer),
	gpointer leave_data
);
extern GtkWidget *GUIMenuAddToMenuBar(
	GtkWidget *menu_bar,
	GtkWidget *menu,
	const gchar *label,
	const GtkJustification justify
);
extern void GUIMenuItemSetLabel(
	GtkWidget *menu_item,
	const gchar *label
);
extern void GUIMenuItemSetPixmap(
	GtkWidget *menu_item,
	guint8 **icon_data
);
extern void GUIMenuItemSetPixmap2(
	GtkWidget *menu_item,
	GdkPixmap *pixmap, GdkBitmap *mask
);
extern void GUIMenuItemSetAccelKey(
	GtkWidget *menu_item, GtkAccelGroup *accelgrp,
	guint accel_key, guint accel_mods
);
extern void GUIMenuItemSetCheck(
	GtkWidget *menu_item,
	const gboolean active,
	const gboolean emit_signal
);
extern gboolean GUIMenuItemGetCheck(GtkWidget *menu_item);
extern GtkWidget *GUIMenuAddToMenuBarPixmapH(
	GtkWidget *menu_bar,
	GtkWidget *menu,
	const gchar *label,
	guint8 **icon_data,
	const GtkJustification justify
);
extern void GUIMenuItemSetSubMenu(
	GtkWidget *menu_item,
	GtkWidget *sub_menu
);


/* Editable Popup Menu */
extern GtkWidget *GUIEditableGetPopupMenu(GtkWidget *w);
extern void GUIEditableEndowPopupMenu(
	GtkWidget *w, const gui_editable_popup_menu_flags flags
);


/* Pull Out Window */
extern void *GUIPullOutCreateH(
	gpointer parent_box,
	gint homogeneous, gint spacing,		/* Of client vbox */
	gint expand, gint fill, gint padding,	/* Of holder hbox */
	gint toplevel_width, gint toplevel_height,
	void *pull_out_client_data,
	void (*pull_out_cb)(void *, void *),
	void *push_in_client_data,
	void (*push_in_cb)(void *, void *)
);
extern void *GUIPullOutGetToplevelWindow(
	gpointer client_box,
	gint *x, gint *y, gint *width, gint *height
);
extern void GUIPullOutPullOut(gpointer client_box);
extern void GUIPullOutPushIn(gpointer client_box);


#endif	/* GUIUTILS_H */
