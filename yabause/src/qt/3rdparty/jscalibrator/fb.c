#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#if defined(_WIN32)
# include <windows.h>
#else
# include <utime.h>
# include <unistd.h>
# include <fnmatch.h>
# if defined(__SOLARIS__)
#  include <sys/mnttab.h>
#  include <sys/vfstab.h>
# elif defined(__FreeBSD__)
/* #  include <mntent.h> */
# else
#  include <mntent.h>
# endif
#endif
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "../include/string.h"
#include "../include/disk.h"

#include "guiutils.h"
#include "cdialog.h"
#include "pulist.h"
#include "fprompt.h"
#include "fb.h"
#include "config.h"


#include "images/icon_file_20x20.xpm"
#include "images/icon_file_hidden_20x20.xpm"
#include "images/icon_folder_closed_20x20.xpm"
#include "images/icon_folder_opened_20x20.xpm"
#include "images/icon_folder_parent_20x20.xpm"
#include "images/icon_folder_noaccess_20x20.xpm"
#include "images/icon_folder_home_20x20.xpm"
#include "images/icon_folder_hidden_20x20.xpm"
#include "images/icon_link2_20x20.xpm"
#include "images/icon_pipe_20x20.xpm"
#include "images/icon_socket_20x20.xpm"
#include "images/icon_device_block_20x20.xpm"
#include "images/icon_device_character_20x20.xpm"
#include "images/icon_executable_20x20.xpm"
#include "images/icon_drive_fixed_20x20.xpm"
#include "images/icon_drive_root_20x20.xpm"

#include "images/icon_fb_list_standard_20x20.xpm"
#include "images/icon_fb_list_vertical_20x20.xpm"
#include "images/icon_fb_list_vertical_details_20x20.xpm"
#include "images/icon_rename_20x20.xpm"
#include "images/icon_chmod_20x20.xpm"
#include "images/icon_reload_20x20.xpm"
#include "images/icon_select_20x20.xpm"
#include "images/icon_ok_20x20.xpm"
#include "images/icon_cancel_20x20.xpm"

#include "images/icon_trash_32x32.xpm"


typedef struct _FileBrowserIcon		FileBrowserIcon;
#define FILE_BROWSER_ICON(p)		((FileBrowserIcon *)(p))

typedef struct _FileBrowserObject	FileBrowserObject;
#define FILE_BROWSER_OBJECT(p)		((FileBrowserObject *)(p))

typedef struct _FileBrowserColumn	FileBrowserColumn;
#define FILE_BROWSER_COLUMN(p)		((FileBrowserColumn *)(p))

typedef struct _FileBrowser		FileBrowser;
#define FILE_BROWSER(p)			((FileBrowser *)(p))


/*
 *	Flags:
 */
typedef enum {
	FB_MAPPED			= (1 << 0),
	FB_REALIZED			= (1 << 1),
	FB_VSB_MAPPED			= (1 << 2),
	FB_HSB_MAPPED			= (1 << 3),
	FB_SHOW_HIDDEN_OBJECTS		= (1 << 8),
	FB_RESPONSE_OK			= (1 << 15)	/* User clicked on OK,
							 * otherwise, user
							 * clicked on cancel */
} FileBrowserFlags;


/*
 *	List Formats:
 */
typedef enum {
	FB_LIST_FORMAT_STANDARD,		/* Horizontal */
	FB_LIST_FORMAT_VERTICAL,		/* Vertical */
	FB_LIST_FORMAT_VERTICAL_DETAILS		/* Vertical with details */
} FileBrowserListFormat;


/*
 *	Icon Index Values:
 *
 *	The values represent the index of each icon defined in
 *	FB_ICON_DATA_LIST.
 */
typedef enum {
	FB_ICON_FILE,				/* 0 */
	FB_ICON_FILE_HIDDEN,
	FB_ICON_FOLDER_CLOSED,
	FB_ICON_FOLDER_OPENED,
	FB_ICON_FOLDER_PARENT,
	FB_ICON_FOLDER_NOACCESS,
	FB_ICON_FOLDER_HOME,
	FB_ICON_FOLDER_HIDDEN,
	FB_ICON_LINK,
	FB_ICON_PIPE,
	FB_ICON_SOCKET,				/* 10 */
	FB_ICON_DEVICE_BLOCK,
	FB_ICON_DEVICE_CHARACTER,
	FB_ICON_EXECUTABLE,
	FB_ICON_DRIVE_FIXED,
	FB_ICON_DRIVE_ROOT			/* 15 */
} FileBrowserIconNum;


/* Utilities */
static GList *FileBrowserDNDBufParse(
	const gchar *buf,
	const gint len
);
static gchar *FileBrowserDNDBufFormat(
	GList *objects_list,
	GList *selection,
	gint *len_rtn
);
static gboolean FileBrowserObjectNameFilter(
	const gchar *name,
	const gchar *full_path,
	const gchar *ext
);


/* Get Drive Paths */
static GList *FileBrowserGetDrivePaths(void);


/* Busy/Ready Setting */
static void FileBrowserSetBusy(
	FileBrowser *fb,
	const gboolean busy
);


/* Show Hidden Objects Setting */
static void FileBrowserSetShowHiddenObjects(
	FileBrowser *fb,
	const gboolean show
);


/* List Format Setting */
static void FileBrowserSetListFormat(
	FileBrowser *fb,
	const FileBrowserListFormat list_format
);


/* Location */
static void FileBrowserSetLocation(
	FileBrowser *fb,
	const gchar *path
);
static void FileBrowserGoToParent(FileBrowser *fb);
static const gchar *FileBrowserGetLocation(FileBrowser *fb);


/* Locations Popup List */
static void FileBrowserLocationsPUListUpdate(FileBrowser *fb);


/* Icons */
static FileBrowserIcon *FileBrowserIconNew(void);
static void FileBrowserIconDelete(FileBrowserIcon *icon);
static FileBrowserIcon *FileBrowserGetIcon(FileBrowser *fb, const gint i);
static FileBrowserIcon *FileBrowserIconAppend(
	FileBrowser *fb,
	guint8 **data,
	const gchar *desc
);
static gint FileBrowserMatchIconNumFromPath(
	FileBrowser *fb, const gchar *full_path,
	struct stat *lstat_buf
);


/* Objects */
static FileBrowserObject *FileBrowserObjectNew(void);
static void FileBrowserObjectDelete(FileBrowserObject *o);
static FileBrowserObject *FileBrowserGetObject(FileBrowser *fb, const gint i);
static void FileBrowserObjectUpdateValues(
	FileBrowser *fb,
	FileBrowserObject *o
);
static FileBrowserObject *FileBrowserObjectAppend(
	FileBrowser *fb,
	const gchar *name,
	const gchar *full_path,
	struct stat *lstat_buf
);


/* List */
static void FileBrowserListUpdate(
	FileBrowser *fb,
	const gchar *filter
);
static void FileBrowserListUpdatePositions(FileBrowser *fb);
static void FileBrowserListSetDNDDragIcon(FileBrowser *fb);
static GtkVisibility FileBrowserListObjectVisibility(
	FileBrowser *fb,
	const gint i
);
static void FileBrowserListMoveToObject(
	FileBrowser *fb,
	const gint i,
	const gfloat coeff
);
static gint FileBrowserListSelectCoordinates(
	FileBrowser *fb,
	const gint x, const gint y
);
static void FileBrowserListHeaderDraw(FileBrowser *fb);
static void FileBrowserListHeaderQueueDraw(FileBrowser *fb);
static void FileBrowserListDraw(FileBrowser *fb);
static void FileBrowserListQueueDraw(FileBrowser *fb);


/* List Columns */
static FileBrowserColumn *FileBrowserColumnNew(void);
static void FileBrowserColumnDelete(FileBrowserColumn *column);
static FileBrowserColumn *FileBrowserListGetColumn(
	FileBrowser *fb,
	const gint i
);
static FileBrowserColumn *FileBrowserListColumnAppend(
	FileBrowser *fb,
	const gchar *label,
	const gint width
);
static void FileBrowserListColumnsClear(FileBrowser *fb);


/* File Name Entry */
static void FileBrowserEntrySetSelectedObjects(FileBrowser *fb);


/* Types List */
static void FileBrowserTypesListSetTypes(
	FileBrowser *fb,
	fb_type_struct **list, const gint total
);


/* File Operations */
static void FileBrowserNewDirectory(FileBrowser *fb);
static gint FileBrowserMoveIteration(
	FileBrowser *fb,
	const gchar *old_path,
	const gchar *new_path
);
static gint FileBrowserCopyIteration(
	FileBrowser *fb,
	const gchar *old_path,
	const gchar *new_path
);
static void FileBrowserMove(
	FileBrowser *fb,
	GList *paths_list,
	const gchar *tar_path
);
static void FileBrowserCopy(
	FileBrowser *fb,
	GList *paths_list,
	const gchar *tar_path
);
static void FileBrowserLink(
	FileBrowser *fb,
	const gchar *src_path,
	const gchar *tar_path
);
static void FileBrowserRenameFPromptCB(gpointer data, const gchar *value);
static void FileBrowserRenameQuery(FileBrowser *fb);
static void FileBrowserCHModFPromptCB(gpointer data, const gchar *value);
static void FileBrowserCHModQuery(FileBrowser *fb);
static gint FileBrowserDeleteIteration(
	FileBrowser *fb,
	const gchar *path
);
static void FileBrowserDelete(FileBrowser *fb);


/* Operations */
static void FileBrowserRefresh(FileBrowser *fb);
static void FileBrowserEntryActivate(FileBrowser *fb);
static void FileBrowserOK(FileBrowser *fb);
static void FileBrowserCancel(FileBrowser *fb);


/* Callbacks */
static void FileBrowserLocationsPUListItemDestroyCB(gpointer data);
static void FileBrowserTypesPUListItemDestroyCB(gpointer data);

static void FileBrowserListDragBeginCB(
	GtkWidget *widget, GdkDragContext *dc, gpointer data
);
static void FileBrowserListDragDataGetCB(
	GtkWidget *widget, GdkDragContext *dc,
	GtkSelectionData *selection_data, guint info,
	guint t,
	gpointer data
);
static void FileBrowserListDragDataDeleteCB(
	GtkWidget *widget, GdkDragContext *dc, gpointer data
);
static void FileBrowserListDragEndCB(
	GtkWidget *widget, GdkDragContext *dc, gpointer data
);

static void FileBrowserListDragLeaveCB(
	GtkWidget *widget, GdkDragContext *dc,
	guint t,
	gpointer data
);
static gboolean FileBrowserListDragMotionCB(
	GtkWidget *widget, GdkDragContext *dc,
	gint x, gint y,
	guint t,
	gpointer data
);
static void FileBrowserDragDataReceivedCB(
	GtkWidget *widget, GdkDragContext *dc,
	gint x, gint y,
	GtkSelectionData *selection_data, guint info,
	guint t,
	gpointer data
);

static void FileBrowserShowHiddenObjectsToggleCB(GtkWidget *widget, gpointer data);
static void FileBrowserListFormatToggleCB(GtkWidget *widget, gpointer data);
static void FileBrowserGoToParentCB(GtkWidget *widget, gpointer data);
static void FileBrowserNewDirectoryCB(GtkWidget *widget, gpointer data);
static void FileBrowserRefreshCB(GtkWidget *widget, gpointer data);

static void FileBrowserLocPUListChangedCB(
	GtkWidget *widget, const gint i, gpointer data
);

static gint FileBrowserListHeaderEventCB(
	GtkWidget *widget, GdkEvent *event, gpointer data
);
static gint FileBrowserListEventCB(
	GtkWidget *widget, GdkEvent *event, gpointer data
);
static void FileBrowserListRealizeCB(GtkWidget *widget, gpointer data);
static void FileBrowserListScrollCB(GtkAdjustment *adj, gpointer data);

static void FileBrowserSelectAllCB(GtkWidget *widget, gpointer data);
static void FileBrowserUnselectAllCB(GtkWidget *widget, gpointer data);
static void FileBrowserInvertSelectionCB(GtkWidget *widget, gpointer data);

static void FileBrowserRenameCB(GtkWidget *widget, gpointer data);
static void FileBrowserCHModCB(GtkWidget *widget, gpointer data);
static void FileBrowserDeleteCB(GtkWidget *widget, gpointer data);

static gint FileBrowserEntryCompletePathKeyCB(
	GtkWidget *widget, GdkEventKey *key, gpointer data
);
static void FileBrowserEntryActivateCB(GtkWidget *widget, gpointer data);

static void FileBrowserTypesPUListChangedCB(
	GtkWidget *widget, const gint i, gpointer data
);

static void FileBrowserOKCB(GtkWidget *widget, gpointer data);
static void FileBrowserCancelCB(GtkWidget *widget, gpointer data);


/* File Browser Front Ends */
gint FileBrowserInit(void);
void FileBrowserSetStyle(GtkRcStyle *rc_style);
void FileBrowserSetTransientFor(GtkWidget *w);
void FileBrowserSetObjectCreatedCB(
	void (*cb)(
		const gchar *path,
		gpointer data
	),
	gpointer data
);
void FileBrowserSetObjectModifiedCB(
	void (*cb)(
		const gchar *old_path,
		const gchar *new_path,
		gpointer data
	),
	gpointer data
);
void FileBrowserSetObjectDeletedCB(
	void (*cb)(
		const gchar *path,
		gpointer data
	),
	gpointer data
);
gboolean FileBrowserIsQuery(void);
void FileBrowserBreakQuery(void);
GtkWidget *FileBrowserGetToplevel(void);
void FileBrowserReset(void);
gboolean FileBrowserGetResponse(
	const gchar *title,
	const gchar *ok_label,
	const gchar *cancel_label,
	const gchar *path,
	fb_type_struct **type, gint total_types,
	gchar ***path_rtn, gint *path_total_rtns,
	fb_type_struct **type_rtn
);
void FileBrowserMap(void);
void FileBrowserUnmap(void);
void FileBrowserShutdown(void);

const gchar *FileBrowserGetLastLocation(void);
void FileBrowserShowHiddenObjects(const gboolean show);
void FileBrowserListStandard(void);
void FileBrowserListDetailed(void);


/* File Types List */
gint FileBrowserTypeListNew(
	fb_type_struct ***list, gint *total,
	const gchar *ext,	/* Space separated list of extensions */
	const gchar *name	/* Descriptive name */
);
static void FileBrowserTypeDelete(fb_type_struct *t);
void FileBrowserDeleteTypeList(
	fb_type_struct **list, const gint total
);


#define FB_WIDTH			480
#define FB_HEIGHT			300

#define FB_TB_BTN_WIDTH			(20 + 5)
#define FB_TB_BTN_HEIGHT		(20 + 5)

#define FB_LOC_LIST_MAX_CHARS		25	/* Maximum characters for
						 * each item in the
						 * locations list */

#define FB_LIST_ITEM_MAX_CHARS		25	/* Maximum characters for
						 * each item in the list */

#define FB_LIST_HEADER_WIDTH		-1
#define FB_LIST_HEADER_HEIGHT		(20 + 2)

#define FB_LIST_WIDTH			-1
#define FB_LIST_HEIGHT			-1

#define FB_LIST_OBJECT_SPACING		1	/* Spacing between objects */

#define FB_LIST_ICON_WIDTH		20
#define FB_LIST_ICON_HEIGHT		20
#define FB_LIST_ICON_TEXT_SPACING	2	/* Spacing between icon and
						 * text */

#define FB_DEFAULT_TYPE_STR		"All Files (*.*)"

#define FB_NEW_DIRECTORY_NAME		"new_directory"


#define FB_IS_OBJECT_SELECTED(_fb_,_obj_num_)	\
 ((((_fb_) != NULL) && ((_obj_num_) >= 0)) ?	\
  ((g_list_find((_fb_)->selection, (gpointer)(_obj_num_)) != NULL) ? \
   TRUE : FALSE) : FALSE)


#define ATOI(s)		(((s) != NULL) ? atoi(s) : 0)
#define ATOL(s)		(((s) != NULL) ? atol(s) : 0)
#define ATOF(s)		(((s) != NULL) ? atof(s) : 0.0f)
#define STRDUP(s)	(((s) != NULL) ? g_strdup(s) : NULL)

#define MAX(a,b)	(((a) > (b)) ? (a) : (b))
#define MIN(a,b)	(((a) < (b)) ? (a) : (b))
#define CLIP(a,l,h)	(MIN(MAX((a),(l)),(h)))
#define STRLEN(s)	(((s) != NULL) ? strlen(s) : 0)
#define STRISEMPTY(s)	(((s) != NULL) ? (*(s) == '\0') : TRUE)

#ifndef ISBLANK
# define ISBLANK(c)	(((c) == ' ') || ((c) == '\t'))
#endif


/*
 *	Icons Data List:
 *
 *	The order of this list must correspond with FileBrowserIconNum.
 *
 *	Each set contains 6 gpointers, and the gpointers in the last
 *	set are all NULL.
 */
#define FB_ICON_DATA_LIST	{	\
	"file",				\
	icon_file_20x20_xpm,		\
	NULL, NULL, NULL, NULL,		\
					\
	"file/hidden",			\
	icon_file_hidden_20x20_xpm,	\
	NULL, NULL, NULL, NULL,		\
					\
	"directory/closed",		\
	icon_folder_closed_20x20_xpm,	\
	NULL, NULL, NULL, NULL,		\
					\
	"directory/opened",		\
	icon_folder_opened_20x20_xpm,	\
	NULL, NULL, NULL, NULL,		\
					\
	"directory/parent",		\
	icon_folder_parent_20x20_xpm,	\
	NULL, NULL, NULL, NULL,		\
					\
	"directory/noaccess",		\
	icon_folder_noaccess_20x20_xpm,	\
	NULL, NULL, NULL, NULL,		\
					\
	"directory/home",		\
	icon_folder_home_20x20_xpm,	\
	NULL, NULL, NULL, NULL,		\
					\
	"directory/hidden",		\
	icon_folder_hidden_20x20_xpm,	\
	NULL, NULL, NULL, NULL,		\
					\
	"link",				\
	icon_link2_20x20_xpm,		\
	NULL, NULL, NULL, NULL,		\
					\
	"pipe",				\
	icon_pipe_20x20_xpm,		\
	NULL, NULL, NULL, NULL,		\
					\
	"socket",			\
	icon_socket_20x20_xpm,		\
	NULL, NULL, NULL, NULL,		\
					\
	"device/block",			\
	icon_device_block_20x20_xpm,	\
	NULL, NULL, NULL, NULL,		\
					\
	"device/character",		\
	icon_device_character_20x20_xpm,\
	NULL, NULL, NULL, NULL,		\
					\
	"executable",			\
	icon_executable_20x20_xpm,	\
	NULL, NULL, NULL, NULL,		\
					\
	"drive/fixed",			\
	icon_drive_fixed_20x20_xpm,	\
	NULL, NULL, NULL, NULL,		\
					\
	"drive/root",			\
	icon_drive_root_20x20_xpm,	\
	NULL, NULL, NULL, NULL,		\
					\
	NULL, NULL,			\
	NULL, NULL, NULL, NULL		\
}

/*
 *	Drag Flags:
 */
typedef enum {
	FILE_BROWSER_DRAG_ACTIVE	= (1 << 0),	/* We initiated a drag */
	FILE_BROWSER_DRAG_ACTIVE_MOVE	= (1 << 1),	/* Initiated drag is
							 * currently a move */
	FILE_BROWSER_DRAG_OVER		= (1 << 7)	/* Drag is over the
							 * list and is accepted */
} FileBrowserDragFlags;


/*
 *	Icon:
 */
struct _FileBrowserIcon {

	GdkPixmap	*pixmap;
	GdkBitmap	*mask;
	gint		width, height;
	gchar		*desc;		/* Used for MIME type */

};


/*
 *	Object:
 */
struct _FileBrowserObject {

	gchar		*name,
			*full_path,
			*displayed_name;	/* Name shown on the list
						 * (may be abbriviated) */
	gint		icon_num;
	gint		x, y,			/* Position and size on list */
			width, height;

	struct stat	lstat_buf;

};


/*
 *	Column:
 *
 *	Used in the FileBrowser's list when displaying things that
 *	need columns.
 */
struct _FileBrowserColumn {

	GtkWidgetFlags	flags;
	gint		position;
	gchar		*label;
	GtkJustification	label_justify;

	/* If the member drag is TRUE then it means this column is
	 * being dragged (resized) and the member drag_position
	 * indicates the right or lower edge of this column
	 */
	gboolean	drag;
	gint		drag_position;
	gint		drag_last_drawn_position;

};


/*
 *	File Browser:
 */
struct _FileBrowser {

	GtkWidget	*toplevel;
	GtkAccelGroup	*accelgrp;
	GdkGC		*gc;
	gint		busy_count,
			freeze_count,
			main_level;
	FileBrowserFlags	flags;

	GdkBitmap	*transparent_stipple_bm;

	GdkCursor	*cur_busy,
			*cur_column_hresize,
			*cur_translate;

	GtkWidget	*main_vbox,
			*location_pulistbox,
			*goto_parent_btn,
			*new_directory_btn,
			*rename_btn,
			*refresh_btn,
			*show_hidden_objects_tb,
			*show_hidden_objects_micheck,
			*list_format_standard_tb,
			*list_format_standard_micheck,
			*list_format_vertical_tb,
			*list_format_vertical_micheck,
			*list_format_vertical_details_tb,
			*list_format_vertical_details_micheck,
			*list_header_da,	/* List Header GtkDrawingArea */
			*list_da,		/* List GtkDrawingArea */
			*list_vsb,		/* List Vertical GtkScrollBar */
			*list_hsb,		/* List Horizontal GtkScrollBar */
 			*list_menu,		/* List Right Click GtkMenu */
			*entry,			/* File Name GtkEntry */
			*types_pulistbox,
			*ok_btn_label,
			*ok_btn,
			*cancel_btn_label,
			*cancel_btn;

	GdkPixmap	*list_pm;		/* List GtkDrawingArea back
						 * buffer */

	/* Current Location */
	gchar		*cur_location;

	/* List Format */
	FileBrowserListFormat	list_format;

	/* Columns */
	GList		*columns_list;		/* GList of FileBrowserColumn * columns */

	/* Icons */
	GList		*icons_list;		/* GList of FileBrowserIcon * icons */

	gint		uid, euid;
	gint		gid, egid;
	gchar		*home_path;

	/* Devices */
	GList		*device_paths_list;	/* List of gchar * device paths */

	/* Objects */
	GList		*objects_list;		/* GList of FileBrowserObject * objects */
	gint		objects_per_row;	/* For use with
						 * FB_LIST_FORMAT_STANDARD */
	GList		*selection,
			*selection_end;
	gint		focus_object,
			last_single_select_object;

	/* Current File Type Return */
	fb_type_struct	cur_type;

	/* Selected Paths Return */
	gchar 		**selected_path;
	gint		total_selected_paths;

	/* Callbacks */
	void		(*object_created_cb)(
		const gchar *path,
		gpointer data
	);
	gpointer	object_created_data;

	void		(*object_modified_cb)(
		const gchar *old_path,
		const gchar *new_path,
		gpointer data
	);
	gpointer	object_modified_data;

	void		(*object_deleted_cb)(
		const gchar *path,
		gpointer data
	);
	gpointer	object_deleted_data;

	/* Last button pressed and pointer position */
	gint		button;
	FileBrowserDragFlags	drag_flags;
	gint		drag_last_x,
			drag_last_y;

};

static FileBrowser file_browser;


/*
 *	Parses the DND buffer into a list of paths.
 *
 *	Returns a GList of gchar * paths or NULL on error. The calling
 *	function must delete the list and each path.
 */
static GList *FileBrowserDNDBufParse(
	const gchar *buf,
	const gint len
)
{
	const gchar	*buf_ptr = buf,
			*buf_end = buf_ptr + len;
	GList *paths_list;

	if((buf_ptr == NULL) || (len <= 0))
	    return(NULL);

	/* Iterate through the buffer */
	paths_list = NULL;
	while(buf_ptr < buf_end)
	{
	    const gchar *s = buf_ptr;
	    gint s_len = STRLEN(s);

	    /* Is the protocol prefix of the string one that we want? */
	    if(g_strcasepfx(s, "file://"))
	    {
		/* Seek past protocol "file://" */
		s += STRLEN("file://");

		/* Seek past "username:password@host:port"
		 * too many programs fail to do this!
		 */
		s = (s != NULL) ?
		    (gchar *)strchr((char *)s, '/') : NULL;

		/* Able to seek to full path portion of the string? */
		if(s != NULL)
		{
		    /* Add this path to the paths list */
		    paths_list = g_list_append(
			paths_list,
			STRDUP(s)
		    );
		}
	    }

	    /* Seek to the next segment, past any '\0'
	     * deliminator(s)
	     */
	    buf_ptr += s_len;
	    while((buf_ptr < buf_end) ? (*buf_ptr == '\0') : FALSE)
		buf_ptr++;
	}

	return(paths_list);
}

/*
 *	Generates a DND buffer based on the list of objects that are
 *	selected.
 *
 *	The objects_list specifies the GList of FileBrowserObject *
 *	objects.
 *
 *	The selection specifies the GList of gint selections.
 *
 *	The len_rtn specifies the return value for the length of the
 *	buffer.
 *
 *	Returns a null deliminator string describing the objects in
 *	URL format or NULL on error.
 */
static gchar *FileBrowserDNDBufFormat(
	GList *objects_list,
	GList *selection,
	gint *len_rtn
)
{
	gchar *buf = NULL;
	gint buf_len = 0;
	GList *glist;
	FileBrowserObject *o;

	/* Iterate through the selection list */
	for(glist = selection;
	    glist != NULL;
	    glist = g_list_next(glist)
	)
	{
	    o = FILE_BROWSER_OBJECT(g_list_nth_data(
		objects_list, (guint)glist->data
	    ));
	    if(o == NULL)
		continue;

	    if(o->full_path != NULL)
	    {
		gchar *url = g_strconcat(
		    "file://",
		    o->full_path,
		    NULL
		);
		const gint	url_len = STRLEN(url),
				last_buf_len = buf_len;

		/* Increase the buffer allocation for this url string
		 * which we will append to the buffer (include the
		 * '\0' character counted in buf_len)
		 */
		buf_len += url_len + 1;
		buf = (gchar *)g_realloc(buf, buf_len * sizeof(gchar));
		if(buf == NULL)
		{
		    buf_len = 0;
		    break;
		}
		memcpy(
		    buf + last_buf_len,
		    (url != NULL) ? url : "",
		    url_len + 1		/* Include the '\0' character */
		);

		g_free(url);
	    }
	}

	if(len_rtn != NULL)
	    *len_rtn = buf_len;

	return(buf);
}


/*
 *	Checks if the object name specified by name and full_path
 *	match the extensions given in ext.
 */
static gboolean FileBrowserObjectNameFilter(
	const gchar *name,
	const gchar *full_path,
	const gchar *ext
)
{
	gboolean use_regex;
	gchar *st, *st_end, cur_ext[80];
	gint cur_ext_len, name_len;

	if((name == NULL) || (full_path == NULL) || (ext == NULL))
	    return(FALSE);

	name_len = STRLEN(name);

	while(ISBLANK(*ext) || (*ext == ','))
	    ext++;

	/* Iterate through each extension in ext */
	while(*ext != '\0')
	{
	    use_regex = FALSE;

	    /* Copy current extension string in ext to cur_ext and
	     * seek ext to next position
	     */
	    cur_ext_len = 0;
	    st = cur_ext;
	    st_end = cur_ext + 79;	/* Set end 1 character premature */
	    while((st < st_end) && !ISBLANK(*ext) && (*ext != ',') &&
		  (*ext != '\0')
	    )
	    {
		/* Use this opportunity to check if there are characters
		 * in the extension string to warrent the use of regex
		 */
		if((*ext == '*') || (*ext == '?'))
		    use_regex = TRUE;

		*st++ = *ext++;
		cur_ext_len++;
	    }
	    *st = '\0';

	    /* Seek ext to next extension string */
	    while(ISBLANK(*ext) || (*ext == ','))
		ext++;

	    /* Check if there is a match */
#if defined(_WIN32)
	    if(name_len >= cur_ext_len)
	    {
		if(!g_strcasecmp(
		    name + name_len - cur_ext_len,
		    cur_ext
		))
		return(TRUE);
	    }
#else
	    if(use_regex)
	    {
		/* Use regex to match */
		if(!fnmatch(cur_ext, name, 0))
		    return(TRUE);
	    }
	    else
	    {
		/* Check if cur_ext is a postfix of name */
		if(name_len >= cur_ext_len)
		{
		    if(!g_strcasecmp(
			name + name_len - cur_ext_len,
			cur_ext
		    ))
			return(TRUE);
		}
	    }
#endif
	}
	return(FALSE);
}


/*
 *	Returns a list of strings describing the drive paths.
 */
static GList *FileBrowserGetDrivePaths(void)
{
#if defined(_WIN32)
	gchar	drive_letter = 'a',
		drive_name[10];
	GList *paths_list = NULL;

	for(drive_letter = 'a'; drive_letter <= 'g'; drive_letter++)
	{
	    g_snprintf(
		drive_name, sizeof(drive_name),
		"%c:\\",
		toupper(drive_letter)
	    );
	    paths_list = g_list_append(
		paths_list,
		STRDUP(drive_name)
	    );
	}

	return(paths_list);
#elif defined(__FreeBSD__)
	return(NULL);
#else	/* UNIX */
	GList *paths_list = NULL;
#ifdef __SOLARIS__
	struct vfstab *vfs_ptr = NULL;
	int mtback;
#else
	struct mntent *mt_ptr;
#endif

	/* Open system devices list file */
#ifdef __SOLARIS__
	FILE *fp = fopen("/etc/vfstab", "rb");
#else
	FILE *fp = setmntent("/etc/fstab", "rb");
#endif
	if(fp == NULL)
	    return(NULL);

	/* Begin reading system devices list file */
#ifdef __SOLARIS__
	vfs_ptr = (struct vfstab *)g_malloc(sizeof(struct vfstab));
	mtback = getvfsent(fp, vfs_ptr);
	while(mtback != 0)
#else
	mt_ptr = getmntent(fp);
	while(mt_ptr != NULL)
#endif
	{
	    /* Get mount path as the drive path */
#ifdef __SOLARIS__
	    paths_list = g_list_append(
		paths_list,
		STRDUP(vfs_ptr->vfs_mountp)
	    );
#else
	    paths_list = g_list_append(
		paths_list,
		STRDUP(mt_ptr->mnt_dir)
	    );
#endif

	    /* Read next mount entry */
#ifdef __SOLARIS__
	    mtback = getmntent(fp, vfs_ptr);
#else
	    mt_ptr = getmntent(fp);
#endif
	}


	/* Close the system devices list file */
#ifdef __SOLARIS__
	fclose(fp);
	fp = NULL;
	vfs_ptr = NULL;
#else
	endmntent(fp);
	fp = NULL;
#endif

	return(paths_list);
#endif
}


/*
 *	Sets the file browser busy or ready.
 */
static void FileBrowserSetBusy(
	FileBrowser *fb,
	const gboolean busy
)
{
	GdkCursor *cursor;
	GtkWidget *w;

	if(fb == NULL)
	    return;

	w = fb->toplevel;
	if(w != NULL)
	{
	    if(busy)
	    {
		/* Increase busy count */
		fb->busy_count++;

		/* If already busy then don't change anything */
		if(fb->busy_count > 1)
		    return;

		cursor = fb->cur_busy;
	    }
	    else
	    {
		/* Reduce busy count */
		fb->busy_count--;
		if(fb->busy_count < 0)
		    fb->busy_count = 0;

		/* If still busy do not change anything */
		if(fb->busy_count > 0)
		    return;

		cursor = NULL;	/* Use default cursor */
	    }

	    /* Update toplevel window's cursor */
	    if(w->window != NULL)
	    {
		gdk_window_set_cursor(w->window, cursor);
		gdk_flush();
	    }
	}
}


/*
 *	Sets the showing of hidden objects and updates the list as
 *	needed.
 */
static void FileBrowserSetShowHiddenObjects(
	FileBrowser *fb,
	const gboolean show
)
{
	if(fb == NULL)
	    return;

	if(show && !(fb->flags & FB_SHOW_HIDDEN_OBJECTS))
	{
	    fb->flags |= FB_SHOW_HIDDEN_OBJECTS;
	    FileBrowserShowHiddenObjectsToggleCB(NULL, fb);
	}
	else if(!show && (fb->flags & FB_SHOW_HIDDEN_OBJECTS))
	{
	    fb->flags &= ~FB_SHOW_HIDDEN_OBJECTS;
	    FileBrowserShowHiddenObjectsToggleCB(NULL, fb);
	}
}


/*
 *	Sets the list format and updates the list as needed.
 */
static void FileBrowserSetListFormat(
	FileBrowser *fb,
	const FileBrowserListFormat list_format
)
{
	GtkWidget *w, *toplevel;
	FileBrowserColumn *column;

	if(fb == NULL)
	    return;

	/* Skip if there is no change */
	if(fb->list_format == list_format)
	    return;

	fb->freeze_count++;

	toplevel = fb->toplevel;

	/* Set new list format */
	fb->list_format = list_format;

	/* Update list format toggle buttons, make sure we freeze so the
	 * toggle callback does not recurse
	 */
	switch(list_format)
	{
	  case FB_LIST_FORMAT_VERTICAL:
	    GTK_TOGGLE_BUTTON_SET_ACTIVE(
		fb->list_format_standard_tb, FALSE
	    );
	    GTK_CHECK_MENU_ITEM_SET_ACTIVE(
		fb->list_format_standard_micheck, FALSE
	    );
	    GTK_TOGGLE_BUTTON_SET_ACTIVE(
		fb->list_format_vertical_tb, TRUE
	    );
	    GTK_CHECK_MENU_ITEM_SET_ACTIVE(
		fb->list_format_vertical_micheck, TRUE
	    );
	    GTK_TOGGLE_BUTTON_SET_ACTIVE(
		fb->list_format_vertical_details_tb, FALSE
	    );
	    GTK_CHECK_MENU_ITEM_SET_ACTIVE(
		fb->list_format_vertical_details_micheck, FALSE
	    );
	    break;
	  case FB_LIST_FORMAT_VERTICAL_DETAILS:
	    GTK_TOGGLE_BUTTON_SET_ACTIVE(
		fb->list_format_standard_tb, FALSE
	    );
	    GTK_CHECK_MENU_ITEM_SET_ACTIVE(
		fb->list_format_standard_micheck, FALSE
	    );
	    GTK_TOGGLE_BUTTON_SET_ACTIVE(
		fb->list_format_vertical_tb, FALSE
	    );
	    GTK_CHECK_MENU_ITEM_SET_ACTIVE(
		fb->list_format_vertical_micheck, FALSE
	    );
	    GTK_TOGGLE_BUTTON_SET_ACTIVE(
		fb->list_format_vertical_details_tb, TRUE
	    );
	    GTK_CHECK_MENU_ITEM_SET_ACTIVE(
		fb->list_format_vertical_details_micheck, TRUE
	    );
	    break;
	  case FB_LIST_FORMAT_STANDARD:
	    GTK_TOGGLE_BUTTON_SET_ACTIVE(
		fb->list_format_standard_tb, TRUE
	    );
	    GTK_CHECK_MENU_ITEM_SET_ACTIVE(
		fb->list_format_standard_micheck, TRUE
	    );
	    GTK_TOGGLE_BUTTON_SET_ACTIVE(
		fb->list_format_vertical_tb, FALSE
	    );
	    GTK_CHECK_MENU_ITEM_SET_ACTIVE(
		fb->list_format_vertical_micheck, FALSE
	    );
	    GTK_TOGGLE_BUTTON_SET_ACTIVE(
		fb->list_format_vertical_details_tb, FALSE
	    );
	    GTK_CHECK_MENU_ITEM_SET_ACTIVE(
		fb->list_format_vertical_details_micheck, FALSE
	    );
	    break;
	}

	/* Begin updating things to recognize the new list format */

	/* Update the list columns */
	FileBrowserListColumnsClear(fb);
	switch(list_format)
	{
	  case FB_LIST_FORMAT_VERTICAL_DETAILS:
	    /* Set up list columns */
	    /* Name */
	    column = FileBrowserListColumnAppend(
		fb,
		"Name",
		145
	    );
	    /* Size */
	    column = FileBrowserListColumnAppend(
		fb,
		"Size",
		70
	    );
	    column->label_justify = GTK_JUSTIFY_RIGHT;
	    /* Permissions */
	    column = FileBrowserListColumnAppend(
		fb,
		"Permissions",
		78
	    );
	    /* Last Modified */
	    column = FileBrowserListColumnAppend(
		fb,
		"Last Modified",
		160
	    );

	    /* Show the List Header GtkDrawingArea */
	    w = fb->list_header_da;
	    if(w != NULL)
		gtk_widget_show(w);
	    break;

	  case FB_LIST_FORMAT_VERTICAL:
	  case FB_LIST_FORMAT_STANDARD:
	    /* Hide the List Header GtkDrawingArea */
	    w = fb->list_header_da;
	    if(w != NULL)
		gtk_widget_hide(w);
	    break;
	}
	gtk_widget_queue_resize(toplevel);


	/* Update the list */
	FileBrowserListUpdate(fb, NULL);

	/* Update the File Name GtkEntry's value */
	FileBrowserEntrySetSelectedObjects(fb);

	fb->freeze_count--;
}

/*
 *	Sets the new location.
 *
 *	Updates the locations popup list.
 *
 *	The path specifies the full path to the new location. If
 *	path does not lead to a directory then its parent will
 *	be used instead. If the specified location and the current
 *	location are the same then nothing will be done.
 */
static void FileBrowserSetLocation(
	FileBrowser *fb,
	const gchar *path
)
{
	gchar *new_location, *new_name;
	GtkWidget *w;

	if((fb == NULL) || (path == NULL))
	    return;

	if(path == fb->cur_location)
	    return;

	/* Make a copy of the specified path which will be modified
	 * and checked
	 */
	new_location = STRDUP(path);

	new_name = NULL;


	/* Special notation checks */

	/* Current location? */
	if(!strcmp((const char *)new_location, "."))
	{
	    g_free(new_location);
#if defined(_WIN32)
	    new_location = (fb->cur_location != NULL) ?
		STRDUP(fb->cur_location) : STRDUP("C:");
#else
	    new_location = (fb->cur_location != NULL) ?
		STRDUP(fb->cur_location) : STRDUP("/");
#endif
	}

	/* Parent? */
	if(!strcmp((const char *)new_location, ".."))
	{
	    g_free(new_location);
#if defined(_WIN32)
	    new_location = (fb->cur_location != NULL) ?
		g_dirname(fb->cur_location) : STRDUP("C:");
#else
	    new_location = (fb->cur_location != NULL) ?
		g_dirname(fb->cur_location) : STRDUP("/");
#endif
	}

	/* Home directory prefix? */
	if(*new_location == '~')
	{
	    /* Prefix the home directory to the new location path */
	    const gchar	*home = fb->home_path,
			*child = new_location + 1;
	    gchar *s = g_strconcat(
		(home != NULL) ? home : "",
		(*child != G_DIR_SEPARATOR) ? G_DIR_SEPARATOR_S : "",
		child,
		NULL
	    );
	    g_free(new_location);
	    new_location = s;
	}

	/* At this point we need to check if the new location is an
	 * absolute path, if it is not then the current working dir
	 * must be prefixed to it
	 */
	if(!g_path_is_absolute(new_location))
	{
	    const gchar *cwd = g_get_current_dir();
	    gchar *s = g_strconcat(
		(cwd != NULL) ? cwd : "",
		G_DIR_SEPARATOR_S,
		new_location,
		NULL
	    );
	    g_free(new_location);
	    new_location = s;
	}

	/* Make sure that the new location is a directory, if it is
	 * not then use its parent
	 */
	if(!ISPATHDIR(new_location))
	{
	    gchar *parent = g_dirname(new_location);
	    const gchar *name = (const gchar *)strrchr(
		(const char *)new_location,
		G_DIR_SEPARATOR
	    );

	    /* Record that we have an object name */
	    g_free(new_name);
	    new_name = (name != NULL) ? STRDUP(name + 1) : NULL;

	    /* Record parent directory */
	    g_free(new_location);
	    new_location = parent;
	}

	/* Strip the tailing deliminator */
	if(new_location != NULL)
	{
#if defined(_WIN32)
	    gchar *s = (gchar *)strrchr((char *)new_location, G_DIR_SEPARATOR);
	    const gint prefix_len = STRLEN("x:\\");
#else
	    gchar *s = (gchar *)strrchr((char *)new_location, G_DIR_SEPARATOR);
	    const gint prefix_len = STRLEN("/");
#endif
	    while(s >= (new_location + prefix_len))
	    {
		if((s[0] == G_DIR_SEPARATOR) &&
		   (s[1] == '\0')
		)
		    *s = '\0';
		else
		    break;
		s--;
	    }
	}

	/* Memory allocation error? */
	if(new_location == NULL)
	{
	    g_free(new_name);
	    return;
	}

	/* No change in the location? */
	if(fb->cur_location != NULL)
	{
	    if(!strcmp((const char *)fb->cur_location, (const char *)new_location))
	    {
		/* No change in location, but check if we still need
		 * to update the lists if the lists were reset/cleared
		 */
		if(fb->objects_list != NULL)
		{
		    /* Lists were not cleared, so no need to update */
		    g_free(new_location);
		    g_free(new_name);
		    return;
		}
	    }
	}

	/* Accept and set the new location */
	g_free(fb->cur_location);
	fb->cur_location = new_location;
	new_location = NULL;

	/* Update the file name entry */
	w = fb->entry;
	if(w != NULL)
	{
	    gtk_entry_set_text(
		GTK_ENTRY(w),
		(!STRISEMPTY(new_name)) ? new_name : ""
	    );
	    gtk_entry_set_position(GTK_ENTRY(w), -1);
	}

	/* Get the listing of the objects at the new location */
	FileBrowserListUpdate(fb, NULL);

	/* Update the locations popup list */
	FileBrowserLocationsPUListUpdate(fb);

	g_free(new_location);
	g_free(new_name);
}

/*
 *	Changes the location to the parent directory of the current
 *	location.
 */
static void FileBrowserGoToParent(FileBrowser *fb)
{
	const gchar *cur_location;
	gchar *parent;

	if(fb == NULL)
	    return;

	cur_location = FileBrowserGetLocation(fb);
	if(cur_location == NULL)
	    return;

	parent = g_dirname(cur_location);
	if(parent == NULL)
	    return;

	FileBrowserSetLocation(fb, parent);

	g_free(parent);
}

/*
 *	Gets the current location.
 *
 *	Returns a statically allocated string describing the current
 *	location.
 */
static const gchar *FileBrowserGetLocation(FileBrowser *fb)
{
#if defined(_WIN32)
	return((fb != NULL) ? fb->cur_location : "C:\\");
#else
	return((fb != NULL) ? fb->cur_location : "/");
#endif
}


/*
 *	List GtkDrawingArea DND source "drag_begin" signal callback.
 */
static void FileBrowserListDragBeginCB(
	GtkWidget *widget, GdkDragContext *dc, gpointer data
)
{
	FileBrowser *fb = FILE_BROWSER(data);
	if((widget == NULL) || (dc == NULL) || (fb == NULL))
	    return;

	/* Mark that we have initiated the drag */
	fb->drag_flags |= FILE_BROWSER_DRAG_ACTIVE;
	FileBrowserListQueueDraw(fb);
}

/*
 *	List GtkDrawingArea DND source "drag_data_get" signal
 *	callback.
 */
static void FileBrowserListDragDataGetCB(
	GtkWidget *widget, GdkDragContext *dc,
	GtkSelectionData *selection_data, guint info,
	guint t,
	gpointer data
)
{
	gboolean data_sent = FALSE;
	FileBrowser *fb = FILE_BROWSER(data);
	if((widget == NULL) || (fb == NULL))
	    return;

	/* Make sure the DND info is one that we support */
	if((info == 0) || (info == 1) || (info == 2))
	{
	    /* Handle by destination widget */
	    /* List */
	    if(widget == fb->list_da)
	    {
		gint buf_len;
		gchar *buf = FileBrowserDNDBufFormat(
		    fb->objects_list, fb->selection, &buf_len
		);
		if(buf != NULL)
		{
		    /* Send out DND data */
		    gtk_selection_data_set(
			selection_data,
			GDK_SELECTION_TYPE_STRING,
			8,		/* 8 bits per character */
			buf, buf_len
		    );
		    data_sent = TRUE;
		    g_free(buf);
		}
	    }
	}

	/* Failed to send out DND data? */
	if(!data_sent)
	{
	    /* Send a response indicating error */
	    const gchar *s = "Error";
	    gtk_selection_data_set(
		selection_data,
		GDK_SELECTION_TYPE_STRING,
		8,			/* 8 bits per character */
		s,
		STRLEN(s) + 1	/* Include the '\0' character */
	    );
	    data_sent = TRUE;
	}
}

/*
 *	List GtkDrawingArea DND source "drag_data_delete" signal
 *	callback.
 */
static void FileBrowserListDragDataDeleteCB(
	GtkWidget *widget, GdkDragContext *dc, gpointer data
)
{
	FileBrowser *fb = FILE_BROWSER(data);
	if((widget == NULL) || (fb == NULL))
	    return;

	/* Typically this means that some objects have been deleted
	 * so we need to refresh the list.
	 */
	FileBrowserRefresh(fb);
}

/*
 *	List GtkDrawingArea DND source "drag_end" signal callback.
 */
static void FileBrowserListDragEndCB(
	GtkWidget *widget, GdkDragContext *dc, gpointer data
)
{
	FileBrowser *fb = FILE_BROWSER(data);
	if((widget == NULL) || (dc == NULL) || (fb == NULL))
	    return;

	/* Clear the drag flags */
	fb->drag_flags &= ~FILE_BROWSER_DRAG_ACTIVE;
	fb->drag_flags &= ~FILE_BROWSER_DRAG_ACTIVE_MOVE;
	FileBrowserListQueueDraw(fb);
}


/*
 *	List GtkDrawingArea DND target "drag_leave" signal callback.
 */
static void FileBrowserListDragLeaveCB(
	GtkWidget *widget, GdkDragContext *dc,
	guint t,
	gpointer data
)
{
	FileBrowser *fb = FILE_BROWSER(data);
	if((widget == NULL) || (dc == NULL) || (fb == NULL))
	    return;

	if(!GTK_WIDGET_SENSITIVE(widget))
	    return;

	/* Mark that the drag has left */
	if(fb->drag_flags & FILE_BROWSER_DRAG_OVER)
	{
	    fb->drag_flags &= ~FILE_BROWSER_DRAG_OVER;
	    FileBrowserListQueueDraw(fb);
	}
	if(fb->focus_object > -1)
	{
	    fb->focus_object = -1;
	    FileBrowserListQueueDraw(fb);
	}
}

/*
 *	List GtkDrawingArea DND target "drag_motion" signal callback.
 */
static gboolean FileBrowserListDragMotionCB(
	GtkWidget *widget, GdkDragContext *dc,
	gint x, gint y,
	guint t,
	gpointer data
)
{
	gboolean same;
	gint i;
	gchar *target_name;
	GList *glist;
	GdkDragAction	allowed_actions,
			default_action_same,
			default_action,
			action;
	GtkTargetFlags target_flags;
	GtkWidget *src_w;
	FileBrowser *fb = FILE_BROWSER(data);
	if((widget == NULL) || (dc == NULL) || (fb == NULL))
	    return(FALSE);

	if(!GTK_WIDGET_SENSITIVE(widget))
	    return(FALSE);

	/* Get the GtkWidget that the drag originated from */
	src_w = gtk_drag_get_source_widget(dc);

	/* Are they the same GtkWidgets? */
	same = (src_w == widget) ? TRUE : FALSE;

	/* Set the target flags to denote if this is the same
	 * GtkWidget or not
	 */
	target_flags = ((same) ? GTK_TARGET_SAME_WIDGET : 0);

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
		    widget,
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
	    return(FALSE);

	/* Determine the appropriate drag action */
	GUIDNDGetTarActions(
	    widget,
	    &allowed_actions,
	    &default_action_same,
	    &default_action
	);
	action = GUIDNDDetermineDragAction(
	    dc->actions,                        /* Requested actions */
	    allowed_actions,
	    (same) ? default_action_same : default_action
	);

	/* Did the drag originate from this list? */
	if(src_w == fb->list_da)
	{
	    if(action == GDK_ACTION_MOVE)
	    {
		if(!(fb->drag_flags & FILE_BROWSER_DRAG_ACTIVE_MOVE))
		{
		    /* Mark that the drag we initiated is currently a move */
		    fb->drag_flags |= FILE_BROWSER_DRAG_ACTIVE_MOVE;
		    FileBrowserListQueueDraw(fb);
		}
	    }
	    else
	    {
		if(fb->drag_flags & FILE_BROWSER_DRAG_ACTIVE_MOVE)
		{
		    /* Clear the initiated drag is a move flag */
		    fb->drag_flags &= ~FILE_BROWSER_DRAG_ACTIVE_MOVE;
		    FileBrowserListQueueDraw(fb);
		}
	    }
	}

	/* Mark that the drag is occuring over this widget */
	if((action != 0) &&
	   !(fb->drag_flags & FILE_BROWSER_DRAG_OVER)
	)
	{
	    fb->drag_flags |= FILE_BROWSER_DRAG_OVER;
	    FileBrowserListQueueDraw(fb);
	}
	else if((action == 0) &&
	        (fb->drag_flags & FILE_BROWSER_DRAG_OVER)
	)
	{
	    fb->drag_flags &= ~FILE_BROWSER_DRAG_OVER;
	    FileBrowserListQueueDraw(fb);
	}

	if(action != 0)
	    i = FileBrowserListSelectCoordinates(fb, x, y);
	else
	    i = -1;
	if(fb->focus_object != i)
	{
	    fb->focus_object = i;
	    FileBrowserListQueueDraw(fb);
	}

	return(FALSE);
}

/*
 *	List GtkDrawingArea or File Name GtkEntry DND target
 *	"drag_data_received" signal callback.
 */
static void FileBrowserDragDataReceivedCB(
	GtkWidget *widget, GdkDragContext *dc,
	gint x, gint y,
	GtkSelectionData *selection_data, guint info,
	guint t,
	gpointer data
)
{
	FileBrowser *fb = FILE_BROWSER(data);
	if((widget == NULL) || (dc == NULL) || (selection_data == NULL) ||
	   (fb == NULL)
	)
	    return;

	if((selection_data->data == NULL) ||
	   (selection_data->length <= 0)
	)
	    return;

	/* Make sure the DND info is one that we want */
	if((info == 0) || (info == 1) || (info == 2))
	{
	    /* List GtkDrawingArea */
	    if(widget == fb->list_da)
	    {
		/* When dropping to the list, first check if there is
		 * exactly one object and if so then check if it leads
		 * to a directory, in which case we will set the new
		 * location.  Otherwise we just set it to the file
		 * name entry.
		 */
		GList *paths_list = FileBrowserDNDBufParse(
		    (const gchar *)selection_data->data,
		    selection_data->length
		);
		if(paths_list != NULL)
		{
		    gchar *tar_path;

		    /* Get the object that was dropped on */
		    const gint i = FileBrowserListSelectCoordinates(
			fb,
			x, y
		    );
		    FileBrowserObject *o = FileBrowserGetObject(fb, i);

		    /* Get the target path */
		    if(o != NULL)
		    {
			tar_path = STRDUP(o->full_path);
			if(!ISPATHDIR(tar_path))
			{
			    g_free(tar_path);
			    tar_path = STRDUP(FileBrowserGetLocation(fb));
			}
		    }
		    else
		    {
			tar_path = STRDUP(FileBrowserGetLocation(fb));
		    }
		    if(tar_path != NULL)
		    {
			switch(dc->action)
			{
			  case GDK_ACTION_COPY:
			    FileBrowserCopy(
				fb,
				paths_list,
				tar_path
			    );
			    break;
			  case GDK_ACTION_MOVE:
			    FileBrowserMove(
				fb,
				paths_list,
				tar_path
			    );
			    break;
			  case GDK_ACTION_LINK:
			    FileBrowserLink(
				fb,
				(const gchar *)paths_list->data,
				tar_path
			    );
			    break;
	                  case GDK_ACTION_DEFAULT:
	                  case GDK_ACTION_PRIVATE:
	                  case GDK_ACTION_ASK:
			    break;
			}
			g_free(tar_path);
		    }

		    g_list_foreach(paths_list, (GFunc)g_free, NULL);
		    g_list_free(paths_list);
		}
	    }
	    /* File Name GtkEntry */
	    else if(widget == fb->entry)
	    {
		GtkEditable *editable = GTK_EDITABLE(widget);
		gint len = selection_data->length;
		const gchar *ss = (const gchar *)selection_data->data;
		gchar *s;

		if(ss[len - 1] == '\0')
		    len--;

		s = (gchar *)g_malloc(len + (1 * sizeof(gchar)));
		if(s != NULL)
		{
		    gint position;
		    gchar *s2, *s_end;

		    if(len > 0)
			memcpy(s, selection_data->data, len);
		    s[len] = '\0';

		    /* Remove any null deliminators within the string */
		    s2 = s;
		    s_end = s2 + len;
		    while(s2 < s_end)
		    {
		        if(*s2 == '\0')
			    *s2 = ' ';
			s2++;
		    }

		    /* Insert the string into the entry */
		    position = gtk_editable_get_position(editable);
		    if(len > 0)
			gtk_editable_insert_text(
			    editable,
			    s, len,
			    &position
			);

		    g_free(s);
		}
	    }
	}
}


/*
 *	Show hidden objects GtkToggleButton "toggled" signal callback.
 */
static void FileBrowserShowHiddenObjectsToggleCB(GtkWidget *widget, gpointer data)
{
	gboolean b;
	FileBrowser *fb = FILE_BROWSER(data);
	if(fb == NULL)
	    return;

	if(fb->freeze_count > 0)
	    return;

	fb->freeze_count++;

	/* Toggle the show hidden objects flag */
	if(fb->flags & FB_SHOW_HIDDEN_OBJECTS)
	{
	    fb->flags &= ~FB_SHOW_HIDDEN_OBJECTS;
	    b = FALSE;
	}
	else
	{
	    fb->flags |= FB_SHOW_HIDDEN_OBJECTS;
	    b = TRUE;
	}

	/* Update the show hidden objects GtkWidgets */
	GTK_CHECK_MENU_ITEM_SET_ACTIVE(
	    fb->show_hidden_objects_micheck,
	    b
	);
	GTK_TOGGLE_BUTTON_SET_ACTIVE(
	    fb->show_hidden_objects_tb,
	    b
	);

	/* Update the listing */
	FileBrowserListUpdate(fb, NULL);

	/* Update the File Name GtkEntry's value */
	FileBrowserEntrySetSelectedObjects(fb);

	fb->freeze_count--;
}

/*
 *	List format GtkToggleButton "toggled" signal callback.
 */
static void FileBrowserListFormatToggleCB(GtkWidget *widget, gpointer data)
{
	FileBrowserListFormat list_format = FB_LIST_FORMAT_STANDARD;
	FileBrowser *fb = FILE_BROWSER(data);
	if((widget == NULL) || (fb == NULL))
	    return;

	if(fb->freeze_count > 0)
	    return;

	fb->freeze_count++;

	/* Do not let toggle button become untoggled when toggling
	 * was not frozen
	 */
	if(GTK_IS_CHECK_MENU_ITEM(widget))
	{
	    GtkCheckMenuItem *micheck = GTK_CHECK_MENU_ITEM(widget);
	    if(!micheck->active)
	    {
		GTK_CHECK_MENU_ITEM_SET_ACTIVE(micheck, TRUE);
		fb->freeze_count--;
		return;
	    }
	}
	else if(GTK_IS_TOGGLE_BUTTON(widget))
	{
	    GtkToggleButton *tb = GTK_TOGGLE_BUTTON(widget);
	    if(!tb->active)
	    {
		gtk_toggle_button_set_active(tb, TRUE);
		fb->freeze_count--;
		return;
	    }
	}

	/* See which toggle button this signal was for so we know
	 * what list format to set
	 */
	/* Standard */
	if((widget == fb->list_format_standard_tb) ||
	   (widget == fb->list_format_standard_micheck)
	)
	    list_format = FB_LIST_FORMAT_STANDARD;
	/* Vertical */
	else if((widget == fb->list_format_vertical_tb) ||
	        (widget == fb->list_format_vertical_micheck)
	)
	    list_format = FB_LIST_FORMAT_VERTICAL;
	/* Vertical with details */
	else if((widget == fb->list_format_vertical_details_tb) ||
		(widget == fb->list_format_vertical_details_micheck)
	)
	    list_format = FB_LIST_FORMAT_VERTICAL_DETAILS;

	/* Set the new list format and reget the listing */
	FileBrowserSetListFormat(fb, list_format);

	fb->freeze_count--;
}

/*
 *	Go to parent callback.
 */
static void FileBrowserGoToParentCB(GtkWidget *widget, gpointer data)
{
	FileBrowserGoToParent(FILE_BROWSER(data));
}

/*
 *	New Directory callback.
 */
static void FileBrowserNewDirectoryCB(GtkWidget *widget, gpointer data)
{
	FileBrowserNewDirectory(FILE_BROWSER(data));
}

/*
 *	Refresh callback.
 */
static void FileBrowserRefreshCB(GtkWidget *widget, gpointer data)
{
	FileBrowserRefresh(FILE_BROWSER(data));
}


/*
 *	Updates the locations popup list based on the current
 *	location.
 */
static void FileBrowserLocationsPUListUpdate(FileBrowser *fb)
{
	struct stat stat_buf;
	const gchar *cur_location;
	GtkWidget *pulist, *pulistbox;
	const FileBrowserIcon *icon;
	GtkDestroyNotify destroy_cb = FileBrowserLocationsPUListItemDestroyCB;

	if(fb == NULL)
	    return;

	pulistbox = fb->location_pulistbox;
	pulist = PUListBoxGetPUList(pulistbox);
	if(pulist == NULL)
	    return;

	FileBrowserSetBusy(fb, TRUE);

	/* Delete all the existing items */
	PUListClear(pulist);

#define GET_ICON(p)	{		\
 if(stat((const char *)(p), &stat_buf))	\
  icon = NULL;				\
 else					\
  icon = FileBrowserGetIcon(		\
   fb,					\
   FileBrowserMatchIconNumFromPath(	\
    fb, (p), &stat_buf			\
   )					\
  );					\
}

/* Takes string s and shortens it, allocating a new string s2
 * that is a shortened (as needed) version of s.  The string s2
 * must be deleted
 */
#define ALLOC_STRING_SHORTENED(_s_) {	\
 if((_s_) != NULL) {			\
  const gint len = STRLEN(_s_),		\
	     max_characters = FB_LOC_LIST_MAX_CHARS;\
					\
  /* Length of _s_ is too long? */	\
  if(len > max_characters)		\
   s2 = g_strdup_printf(		\
    "...%s",				\
    &(_s_)[len - max_characters + 3]	\
   );					\
  else					\
   s2 = STRDUP(_s_);			\
 } else {				\
  s2 = NULL;				\
 }					\
}

	/* Get the current location and set the items for it as
	 * each of its parents
	 */
	cur_location = FileBrowserGetLocation(fb);
	if(cur_location != NULL)
	{
	    gint i, icon_num;
	    gchar	*s = STRDUP(cur_location),
			*sd = strrchr(s, G_DIR_SEPARATOR),
			*s2;

	    /* Current location */
	    if(stat(s, &stat_buf))
		icon_num = -1;
	    else
		icon_num = FileBrowserMatchIconNumFromPath(
		    fb, s, &stat_buf
		);
	    if(icon_num == FB_ICON_FOLDER_CLOSED)
		icon_num = FB_ICON_FOLDER_OPENED;
	    icon = FileBrowserGetIcon(fb, icon_num);
	    ALLOC_STRING_SHORTENED(s);
	    if(icon != NULL)
		i = PUListAddItemPixText(
		    pulist,
		    s2,
		    icon->pixmap, icon->mask
		);
	    else
		i = PUListAddItem(
		    pulist,
		    s2
		);
	    g_free(s2);
	    PUListSetItemDataFull(
		pulist,
		i,
		STRDUP(s), destroy_cb
	    );

	    /* Parent locations */
#if defined(_WIN32)
	    while(sd > (s + 3))
#else
	    while(sd > s)
#endif
	    {
		*sd = '\0';

		GET_ICON(s);
		ALLOC_STRING_SHORTENED(s);
		if(icon != NULL)
		    i = PUListAddItemPixText(
			pulist, s2, icon->pixmap, icon->mask
		    );
		else
		    i = PUListAddItem(pulist, s2);
		g_free(s2);
		PUListSetItemDataFull(pulist, i, STRDUP(s), destroy_cb);

		sd = strrchr(s, G_DIR_SEPARATOR);
	    }

	    /* Toplevel */
#if defined(_WIN32)
	    /* On Win32, use s as is from the above parent fetching loop */
	    sd = strchr(s, G_DIR_SEPARATOR);
	    if((sd != NULL) ? (*(sd + 1) != '\0') : FALSE)
	    {
		*(sd + 1) = '\0';
		GET_ICON(s);
		ALLOC_STRING_SHORTENED(s);
		if(icon != NULL)
		    i = PUListAddItemPixText(
			pulist, s2, icon->pixmap, icon->mask
		    );
		else
		    i = PUListAddItem(pulist, s2);
		g_free(s2);
		PUListSetItemDataFull(pulist, i, STRDUP(s), destroy_cb);
	    }
	    g_free(s);
#else
	    g_free(s);
	    s = STRDUP("/");
	    GET_ICON(s);
	    ALLOC_STRING_SHORTENED(s);
	    if(icon != NULL)
		i = PUListAddItemPixText(
		    pulist, s2, icon->pixmap, icon->mask
		);
	    else
		i = PUListAddItem(pulist, s2);
	    g_free(s2);
	    PUListSetItemDataFull(pulist, i, STRDUP(s), destroy_cb);
	    g_free(s);
#endif
	}

#if !defined(_WIN32)
	/* Home */
	if(fb->home_path != NULL)
	{
	    const gchar *s = fb->home_path;
	    gint i;
	    gchar *s2;
	    GET_ICON(s);
	    ALLOC_STRING_SHORTENED(s);
	    if(icon != NULL)
		i = PUListAddItemPixText(
		    pulist, s2, icon->pixmap, icon->mask
		);
	    else
		i = PUListAddItem(pulist, s2);
	    g_free(s2);
	    PUListSetItemDataFull(pulist, i, STRDUP(s), destroy_cb);
	}
#endif	/* !_WIN32 */

	/* Drives */
	if(fb->device_paths_list != NULL)
	{
	    gint i;
	    const gchar *path;
	    gchar *s2;
	    GList *glist;

	    for(glist = fb->device_paths_list;
		glist != NULL;
		glist = g_list_next(glist)
	    )
	    {
		path = (const gchar *)glist->data;
		if(path == NULL)
		    continue;

		/* Ignore the toplevel path */
		if(!g_strcasecmp((const char *)path, "/"))
		    continue;

#if !defined(_WIN32)
		/* Ignore drives that do not have absolute paths */
		if(!g_path_is_absolute(path))
		    continue;
#endif	/* !_WIN32 */

#if defined(_WIN32)
		icon = FileBrowserGetIcon(fb, FB_ICON_DRIVE_FIXED);
#else
		GET_ICON(path);
#endif
		ALLOC_STRING_SHORTENED(path);
		if(icon != NULL)
		    i = PUListAddItemPixText(
			pulist, s2, icon->pixmap, icon->mask
		    );
		else
		    i = PUListAddItem(
			pulist, s2
		    );
		g_free(s2);
		PUListSetItemDataFull(pulist, i, STRDUP(path), destroy_cb);
	    }
	}

/* Other things to be added to the popup list, like mounted drives? */


	/* Make sure that no more than 10 lines are visible */
	PUListBoxSetLinesVisible(
	    pulistbox,
	    MIN(PUListGetTotalItems(pulist), 10)
	);

	/* Select the first item */
	PUListBoxSelect(pulistbox, 0);


	FileBrowserSetBusy(fb, FALSE);

#undef GET_ICON
#undef ALLOC_STRING_SHORTENED
}


/*
 *	Creates a new icon.
 */
static FileBrowserIcon *FileBrowserIconNew(void)
{
	return(FILE_BROWSER_ICON(g_malloc0(
	    sizeof(FileBrowserIcon)
	)));
}

/*
 *	Deletes the icon.
 */
static void FileBrowserIconDelete(FileBrowserIcon *icon)
{
	if(icon == NULL)
	    return;

	GDK_PIXMAP_UNREF(icon->pixmap);
	GDK_BITMAP_UNREF(icon->mask);
	g_free(icon->desc);
	g_free(icon);
}

/*
 *	Returns the icon at index i or NULL on error.
 */
static FileBrowserIcon *FileBrowserGetIcon(FileBrowser *fb, const gint i)
{
	if((fb == NULL) || (i < 0))
	    return(NULL);

	return(FILE_BROWSER_ICON(g_list_nth_data(
	    fb->icons_list,
	    (guint)i
	)));
}

/*
 *	Appends an icon to the list.
 */
static FileBrowserIcon *FileBrowserIconAppend(
	FileBrowser *fb,
	guint8 **data,
	const gchar *desc
)
{
	FileBrowserIcon *icon;

	if((fb == NULL) || (data == NULL))
	    return(NULL);

	/* Create a new icon */
	icon = FileBrowserIconNew();
	if(icon == NULL)
	    return(NULL);

	/* Append it to the list */
	fb->icons_list = g_list_append(fb->icons_list, icon);

	/* Set the new icon's values */
	icon->pixmap = GDK_PIXMAP_NEW_FROM_XPM_DATA(&icon->mask, data);
	gdk_window_get_size(icon->pixmap, &icon->width, &icon->height);
	icon->desc = STRDUP(desc);

	return(icon);
}


/*
 *	Returns the icon index appropriate for the object specified by
 *	full_path and lstat_buf.
 */
static gint FileBrowserMatchIconNumFromPath(
	FileBrowser *fb,
	const gchar *full_path,
	struct stat *lstat_buf
)
{
	gint uid, gid;
	const gchar *ext, *name;
	mode_t m;

	if(fb == NULL)
	    return(FB_ICON_FILE);

	if(full_path != NULL)
	{
	    const gchar *s;
	    GList *glist;

	    name = g_basename(full_path);

#if !defined(_WIN32)
	    /* Home directory? */
	    s = fb->home_path;
	    if((s != NULL) ? !strcmp(s, full_path) : FALSE)
		return(FB_ICON_FOLDER_HOME);

	    /* Toplevel? */
	    if(!g_strcasecmp(full_path, "/"))
		return(FB_ICON_DRIVE_ROOT);
#endif	/* !_WIN32 */

	    /* Drive path? */
	    for(glist = fb->device_paths_list;
		glist != NULL;
		glist = g_list_next(glist)
	    )
	    {
		s = (const gchar *)glist->data;
		if(s == NULL)
		    continue;

		if(!strcmp(s, full_path))
		    return(FB_ICON_DRIVE_FIXED);
	    }

	    /* Get the extension (if any) */
	    ext = (const gchar *)strrchr((const char *)full_path, '.');
	}
	else
	{
	    name = NULL;
	    ext = NULL;
	}

	/* Get object's statistics */
	if(lstat_buf != NULL)
	{
	    m = lstat_buf->st_mode;
	    uid = lstat_buf->st_uid;
	    gid = lstat_buf->st_gid;
	}
	else
	{
	    m = 0;
	    uid = 0;
	    gid = 0;
	}

	/* Directory? */
#ifdef S_ISDIR
	if(S_ISDIR(m))
#else
	if(FALSE)
#endif
	{
#if defined(_WIN32)
	    return(FB_ICON_FOLDER_CLOSED);
#else
	    /* Accessable? */
	    if(fb->euid != 0)
	    {
		int accessable_icon = FB_ICON_FOLDER_CLOSED;

		/* Hidden? */
		if((name != NULL) ? (*name == '.') : FALSE)
		{
		    if((name[2] != '\0') && (name[2] != '.'))
			accessable_icon = FB_ICON_FOLDER_HIDDEN;
		}

		/* For non-root process id's we should check if the
		 * process or the process' group owns the object and
		 * respectively see if the object (the directory) is
		 * executable (accessable) and return the appropriate
		 * icon number.
		 */
		/* This process owns object? */
		if(fb->euid == uid)
		    return((m & S_IXUSR) ?
			accessable_icon : FB_ICON_FOLDER_NOACCESS
		    );
		/* This process' group id owns object? */
		else if(fb->egid == gid)
		    return((m & S_IXGRP) ?
			accessable_icon : FB_ICON_FOLDER_NOACCESS
		    );
		/* Anonymous */
		else
		    return((m & S_IXOTH) ?
			accessable_icon : FB_ICON_FOLDER_NOACCESS
		    );
	    }
	    else
	    {
		/* Root always owns the object, so check if owner has
		 * access
		 */
		if(!(m & S_IXUSR))
		    return(FB_ICON_FOLDER_NOACCESS);

		/* Hidden? */
		if((name != NULL) ? (*name == '.') : FALSE)
		{
		    if((name[2] != '\0') && (name[2] != '.'))
			return(FB_ICON_FOLDER_HIDDEN);
		}

		return(FB_ICON_FOLDER_CLOSED);
	    }
#endif
	}

#if defined(_WIN32)
	if(ext != NULL)
	{
	    if(!g_strcasecmp(ext, ".exe") ||
	       !g_strcasecmp(ext, ".com") ||
	       !g_strcasecmp(ext, ".bat")
	    )
		return(FB_ICON_EXECUTABLE);
	    else
		return(FB_ICON_FILE);
	}
	else
	{
	    return(FB_ICON_FILE);
	}
#else
#ifdef S_ISLNK
	if(S_ISLNK(m))
	    return(FB_ICON_LINK);
#endif
#ifdef S_ISFIFO
	if(S_ISFIFO(m))
	    return(FB_ICON_PIPE);
#endif
#ifdef S_ISSOCK
	if(S_ISSOCK(m))
	    return(FB_ICON_SOCKET);
#endif
#ifdef S_ISBLK
	if(S_ISBLK(m))
	    return(FB_ICON_DEVICE_BLOCK);
#endif
#ifdef S_ISCHR
	if(S_ISCHR(m))
	    return(FB_ICON_DEVICE_CHARACTER);
#endif
#if defined(S_IXUSR) && defined(S_IXGRP) && defined(S_IXOTH)
	if((m & S_IXUSR) || (m & S_IXGRP) || (m & S_IXOTH))
	    return(FB_ICON_EXECUTABLE);
#endif

	/* Hidden? */
	if((name != NULL) ? (*name == '.') : FALSE)
	{
	    if((name[2] != '\0') && (name[2] != '.'))
		return(FB_ICON_FILE_HIDDEN);
	}

	return(FB_ICON_FILE);
#endif
}


/*
 *	Creates a new object.
 */
static FileBrowserObject *FileBrowserObjectNew(void)
{
	return(FILE_BROWSER_OBJECT(g_malloc0(
	    sizeof(FileBrowserObject)
	)));
}

/*
 *	Deletes the object.
 */
static void FileBrowserObjectDelete(FileBrowserObject *o)
{
	if(o == NULL)
	    return;

	g_free(o->displayed_name);
	g_free(o->full_path);
	g_free(o->name);
	g_free(o);
}

/*
 *	Returns the object at index i or NULL on error.
 */
static FileBrowserObject *FileBrowserGetObject(FileBrowser *fb, const gint i)
{
	if(fb == NULL)
	    return(NULL);

	if(i < 0)
	    return(NULL);

	return(FILE_BROWSER_OBJECT(
	    g_list_nth_data(fb->objects_list, (guint)i)
	));
}

/*
 *	Updates the Object's values.
 *
 *	The name and lstat_buf must be updated prior to this call since
 *	the information used to update the other values are based on
 *	them.
 *
 *	The updated values are; icon_num, width, and height.
 */
static void FileBrowserObjectUpdateValues(
	FileBrowser *fb,
	FileBrowserObject *o
)
{
	gint icon_num;
	GdkFont *font;
	GtkStyle *style;
	GtkWidget *w;
	FileBrowserIcon *icon;

	if((fb == NULL) || (o == NULL))
	    return;

	/* Get the list widget's style and font */
	w = fb->list_da;
	style = (w != NULL) ? gtk_widget_get_style(w) : NULL;
	font = (style != NULL) ? style->font : NULL;

	/* Get icon number based on the object's path and statistics */
	o->icon_num = icon_num = FileBrowserMatchIconNumFromPath(
	    fb, o->full_path, &o->lstat_buf
	);

	/* Get icon */
	icon = FileBrowserGetIcon(fb, icon_num);


	/* Set displayed_name from name, abbreviate it as needed */
	if(o->name != NULL)
	{
#if 0
/* Do not shorten the displayed name anymore */
	    const gchar *name = o->name;
	    const gint	len = STRLEN(name),
			max_chars = FB_LIST_ITEM_MAX_CHARS;

	    g_free(o->displayed_name);
	    if((len > max_chars) && (len > 3))
		o->displayed_name = g_strdup_printf(
		    "...%s",
		    name + (len - max_chars + 3)
		);
	    else
		o->displayed_name = STRDUP(name);
#endif
	    g_free(o->displayed_name);
	    o->displayed_name = STRDUP(o->name);
	}
	else
	{
	    g_free(o->displayed_name);
	    o->displayed_name = STRDUP("");
	}


	/* Update size */
	if((o->displayed_name != NULL) && (font != NULL))
	{
	    const gchar *name = o->displayed_name;
	    const gint font_height = font->ascent + font->descent;
	    GdkTextBounds b;
	    gdk_string_bounds(font, name, &b);
	    if(icon != NULL)
	    {
		o->width = MAX(
		    icon->width + FB_LIST_ICON_TEXT_SPACING + 2 + b.width,
		    1
		);
		o->height = MAX(
		    icon->height,
		    font_height
		);
	    }
	    else
	    {
		o->width = MAX(
		    b.width,
		    1
		);
		o->height = MAX(
		    font_height,
		    1
		);
	    }
	}
	else
	{
	    if(icon != NULL)
	    {
		o->width = icon->width;
		o->height = icon->height;
	    }
	    else
	    {
		o->width = 1;
		o->height = 1;
	    }
	}
}

/*
 *	Appends an object to the list.
 *
 *	Returns the new object or NULL on error.  The calling function
 *	needs to calculate the position.
 */
static FileBrowserObject *FileBrowserObjectAppend(
	FileBrowser *fb,
	const gchar *name,
	const gchar *full_path,
	struct stat *lstat_buf
)
{
	FileBrowserObject *o;

	if((fb == NULL) || (full_path == NULL))
	    return(NULL);

	/* Create a new object */
	o = FileBrowserObjectNew();
	if(o == NULL)
	    return(NULL);

	/* Append this object to the list */
	fb->objects_list = g_list_append(fb->objects_list, o);

	o->name = STRDUP(name);
	o->full_path = STRDUP(full_path);
	if(lstat_buf != NULL)
	    memcpy(&o->lstat_buf, lstat_buf, sizeof(struct stat));
	else
	    memset(&o->lstat_buf, 0x00, sizeof(struct stat));

	/* Update values */
	FileBrowserObjectUpdateValues(fb, o);

	return(o);
}

/*
 *	Updates the list.
 *
 *	Regets the list of objects from the current location and then
 *	calls FileBrowserListUpdatePositions() to update the scroll
 *	bar values. The focus object and selections list will be
 *	cleared.
 *
 *	The filter specifies a string describing which objects are
 *	to be added to the list by name. If filter is NULL then all
 *	objects will be added.
 */
static void FileBrowserListUpdate(
	FileBrowser *fb,
	const gchar *filter
)
{
	gboolean	match_all_files,
			show_hidden_objects;
	gint i;
	const gchar *cur_location, *ext;

	if(fb == NULL)
	    return;

	FileBrowserSetBusy(fb, TRUE);

	/* Show hidden objects? */
	show_hidden_objects = (fb->flags & FB_SHOW_HIDDEN_OBJECTS) ? TRUE : FALSE;

	/* Get the extension from the filter (if specified) or from
	 * the current file type
	 */
	ext = (filter != NULL) ? filter : fb->cur_type.ext;
	if(ext != NULL)
	    match_all_files = (!strcmp(ext, "*.*") || !strcmp(ext, "*")) ? TRUE : FALSE;
	else
	    match_all_files = TRUE;

	/* Get listing of contents in current location */

	/* Delete current objects list and the selection */

	/* Unselect all */
	fb->focus_object = -1;
	g_list_free(fb->selection);
	fb->selection = fb->selection_end = NULL;

	/* Delete all the objects */
	if(fb->objects_list != NULL)
	{
	    g_list_foreach(
		fb->objects_list, (GFunc)FileBrowserObjectDelete, NULL
	    );
	    g_list_free(fb->objects_list);
	    fb->objects_list = NULL;
	}

	/* Reset the number of objects per row */
	fb->objects_per_row = 0;


	/* Get the current location */
	cur_location = FileBrowserGetLocation(fb);
	if(cur_location != NULL)
	{
	    /* Get the listing of objects in the current location */
	    gint nobjs;
	    gchar **names_list = GetDirEntNames2(cur_location, &nobjs);
	    if(names_list != NULL)
	    {
		struct stat stat_buf;
		const gchar *name;
		gchar *full_path;

		StringQSort(names_list, nobjs);

		/* Iterate through the names list and get only the
		 * directories
		 */
		for(i = 0; i < nobjs; i++)
		{
		    name = names_list[i];
		    if(name == NULL)
			continue;

		    /* Skip special directory notations */
		    if(!strcmp(name, ".") ||
		       !strcmp(name, "..")
		    )
		    {
			g_free(names_list[i]);
			names_list[i] = NULL;
			continue;
		    }

		    /* Skip hidden objects? */
		    if(!show_hidden_objects)
		    {
			/* Does the name start with a '.' character? */
			if(*name == '.')
			{
			    g_free(names_list[i]);
			    names_list[i] = NULL;
			    continue;
			}
		    }

		    /* Get the full path to the object */
		    full_path = STRDUP(PrefixPaths(
			(const char *)cur_location,
			(const char *)name
		    ));
		    if(full_path == NULL)
		    {
			g_free(names_list[i]);
			names_list[i] = NULL;
			continue;
		    }

		    /* Get this object's destination stats */
		    if(stat((const char *)full_path, &stat_buf))
		    {
			g_free(full_path);
			/* Do not delete this object from the names list */
			continue;
		    }

		    /* Skip this object if it's destination is not
		     * a directory
		     */
		    if(!S_ISDIR(stat_buf.st_mode))
		    {
			g_free(full_path);
			/* Do not delete this object from the names list */
			continue;
		    }

		    /* Get this directory's local statistics */
#if !defined(_WIN32)
		    if(lstat((const char *)full_path, &stat_buf))
			memset(&stat_buf, 0x00, sizeof(struct stat));
#endif

		    /* Append this directory and its stats to the list */
		    FileBrowserObjectAppend(
			fb,
			name,
			full_path,
			&stat_buf
		    );

		    g_free(full_path);

		    /* Delete this directory from the names list */
		    g_free(names_list[i]);
		    names_list[i] = NULL;
		}

		/* Add all the other objects */
		for(i = 0; i < nobjs; i++)
		{
		    name = names_list[i];
		    if(name == NULL)
			continue;

		    /* Get the full path to the object */
		    full_path = STRDUP(PrefixPaths(cur_location, name));
		    if(full_path == NULL)
		    {
			g_free(names_list[i]);
			names_list[i] = NULL;
			continue;
		    }

		    /* Filter this object's name */
		    if(!match_all_files)
		    {
			if(!FileBrowserObjectNameFilter(
			    name, full_path, ext
			))
			{
			    g_free(full_path);
			    g_free(names_list[i]);
			    names_list[i] = NULL;
			    continue;
			}
		    }

		    /* Get this object's local stats */
#if defined(_WIN32)
		    if(stat((const char *)full_path, &stat_buf))
			memset(&stat_buf, 0x00, sizeof(struct stat));
#else
		    if(lstat((const char *)full_path, &stat_buf))
			memset(&stat_buf, 0x00, sizeof(struct stat));
#endif

		    /* Append this object and its stats to the list */
		    FileBrowserObjectAppend(
			fb,
			name,
			full_path,
			&stat_buf
		    );

		    g_free(full_path);

		    /* Delete this object from the names list */
		    g_free(names_list[i]);
		    names_list[i] = NULL;
		}

		/* Delete the names list */
		g_free(names_list);
	    }
	}

	/* Update each object's position and the scroll bars values */
	FileBrowserListUpdatePositions(fb);

	/* If the objects list is not empty then reset the focus
	 * object
	 */
	if(fb->objects_list != NULL)
	    fb->focus_object = 0;

	FileBrowserSetBusy(fb, FALSE);
}

/*
 *	Updates each object's position and the scroll bar values.
 *
 *	This may produce a change in the list window's size due to
 *	the mapping or unmapping of the scroll bar, in which case
 *	a "configure_event" signal will be queued.
 */
static void FileBrowserListUpdatePositions(FileBrowser *fb)
{
	gboolean	need_resize = FALSE;
	const gint      border_major = 5,
			border_minor = 2,
			border_x = border_minor,
			border_y = border_minor,
			object_spacing = FB_LIST_OBJECT_SPACING;
	gint		width, height,
			objects_per_row,
			cur_x = border_x,
			cur_y = border_y,
			longest_width = 0,
			last_longest_width = longest_width,
			list_x_max = 0, list_y_max = 0,
			list_x_inc = 0, list_x_page_inc = 0,
			list_y_inc = 0, list_y_page_inc = 0;
	GList *glist;
	GdkWindow *window;
	GtkWidget *w;
	FileBrowserListFormat list_format;
	FileBrowserObject *o = NULL;

	if(fb == NULL)
	    return;

	list_format = fb->list_format;
	w = fb->list_da;
	if(w == NULL)
	    return;

	window = w->window;
	if(window == NULL)
	    return;

	/* Get the size of the list */
	gdk_window_get_size(window, &width, &height);

	/* Update the object positions and get values for the scroll
	 * bar
	 */
	switch(list_format)
	{
	  case FB_LIST_FORMAT_VERTICAL_DETAILS:
	  case FB_LIST_FORMAT_VERTICAL:
	    for(glist = fb->objects_list;
		glist != NULL;
		glist = g_list_next(glist)
	    )
	    {
	        o = FILE_BROWSER_OBJECT(glist->data);
	        if(o == NULL)
		    continue;

		if(longest_width < o->width)
		    last_longest_width = longest_width = o->width;

		o->x = cur_x;
		o->y = cur_y;
		cur_y += o->height + object_spacing;
	    }
	    break;
	  case FB_LIST_FORMAT_STANDARD:
	    objects_per_row = 0;
	    for(glist = fb->objects_list;
		glist != NULL;
		glist = g_list_next(glist)
	    )
	    {
	        o = FILE_BROWSER_OBJECT(glist->data);
	        if(o == NULL)
		    continue;

		if(longest_width < o->width)
		    last_longest_width = longest_width = o->width;

		o->x = cur_x;
		o->y = cur_y;
		cur_y += o->height + object_spacing;
		objects_per_row++;

		/* Update the objects per row if this object's y
		 * position has exceeded the list's height
		 */
		if((cur_y + o->height) > height)
		{
		    if(objects_per_row > fb->objects_per_row)
			fb->objects_per_row = objects_per_row;
		    objects_per_row = 0;
		    cur_y = border_y;
		    cur_x += longest_width + border_major;	/* border_major
								 * as spacing
								 * between rows */
		    longest_width = 0;
		}
	    }
	    break;
	}

	/* Use the last object to set the list's bounds, these
	 * bounds will be used in updating of the scroll bar's values
	 */
	if(o != NULL)
	{
	    list_x_max = o->x + last_longest_width + border_x;
	    list_y_max = o->y + o->height + border_y;
	    list_x_inc = (gint)(width * 0.25f);
	    list_x_page_inc = (gint)(width * 0.5f);
	    list_y_inc = o->height;
	    list_y_page_inc = (gint)(height * 0.5f);
	}

	/* Show/hide the scroll bars */
	switch(list_format)
	{
	  case FB_LIST_FORMAT_VERTICAL_DETAILS:
	  case FB_LIST_FORMAT_VERTICAL:
	    w = fb->list_hsb;
	    if(w != NULL)
	    {
		if(fb->flags & FB_HSB_MAPPED)
		{
		    gtk_widget_hide(w);
		    fb->flags &= ~FB_HSB_MAPPED;
		    need_resize = TRUE;
		}
	    }
	    w = fb->list_vsb;
	    if(w != NULL)
	    {
		GtkRange *range = GTK_RANGE(w);
		GtkAdjustment *adj = range->adjustment;
		if(adj != NULL)
		{
		    adj->lower = 0.0f;
		    adj->upper = (gfloat)list_y_max;
		    adj->value = adj->lower;
		    adj->step_increment = (gfloat)list_y_inc;
		    adj->page_increment = (gfloat)list_y_page_inc;
		    adj->page_size = (gfloat)height;
		    gtk_signal_emit_by_name(
			GTK_OBJECT(adj),
			"changed"
		    );
		    gtk_signal_emit_by_name(
			GTK_OBJECT(adj),
			"value_changed"
		    );

		    /* If content size is larger than visible size then
		     * map the scrollbar, otherwise unmap the scrollbar
		     * since it would not be needed.
		     */
		    if((adj->upper - adj->lower) > adj->page_size)
		    {
			if(!(fb->flags & FB_VSB_MAPPED))
			{
			    gtk_widget_show(w);
			    fb->flags |= FB_VSB_MAPPED;
			    need_resize = TRUE;
			}
		    }
		    else
		    {
			if(fb->flags & FB_VSB_MAPPED)
			{
			    gtk_widget_hide(w);
			    fb->flags &= ~FB_VSB_MAPPED;
			    need_resize = TRUE;
			}
		    } 
		}
	    }
	    break;

	  case FB_LIST_FORMAT_STANDARD:
	    w = fb->list_vsb;
	    if(w != NULL)
	    {
		if(fb->flags & FB_VSB_MAPPED)
		{
		    gtk_widget_hide(w);
		    fb->flags &= ~FB_VSB_MAPPED;
		    need_resize = TRUE;
		}
	    }
	    w = fb->list_hsb;
	    if(w != NULL)
	    {
		GtkRange *range = GTK_RANGE(w);
		GtkAdjustment *adj = range->adjustment;
		if(adj != NULL)
		{
		    adj->lower = 0.0f;
		    adj->upper = (gfloat)list_x_max;
		    adj->value = adj->lower;
		    adj->step_increment = (gfloat)list_x_inc;
		    adj->page_increment = (gfloat)list_x_page_inc;
		    adj->page_size = (gfloat)width;
		    gtk_signal_emit_by_name(
			GTK_OBJECT(adj),
			"changed"
		    );
		    gtk_signal_emit_by_name(
			GTK_OBJECT(adj),
			"value_changed"
		    );
		    /* If content size is larger than visible size then
		     * map the scrollbar, otherwise unmap the scrollbar
		     * since it would not be needed
		     */
		    if((adj->upper - adj->lower) > adj->page_size)
		    {
			if(!(fb->flags & FB_HSB_MAPPED))
			{
			    gtk_widget_show(w);
			    fb->flags |= FB_HSB_MAPPED;
			    need_resize = TRUE;
			}
		    }
		    else
		    {
			if(fb->flags & FB_HSB_MAPPED)
			{
			    gtk_widget_hide(w);
			    fb->flags &= ~FB_HSB_MAPPED;
			    need_resize = TRUE;
			}
		    }
		}
	    }
	    break;
	}

	/* Need to resize due to widgets being mapped or unmapped? */
	if(need_resize)
	    gtk_widget_queue_resize(fb->toplevel);
}

/*
 *	Sets the DND drag icon based on the selected objects in
 *	the list.
 */
static void FileBrowserListSetDNDDragIcon(FileBrowser *fb)
{
	const gint nitems_visible = GUI_DND_DRAG_ICON_DEF_NITEMS_VISIBLE;
	gint i;
	GList *glist, *cells_list;
	GtkCellPixText *cell_pixtext;
	FileBrowserObject *o;

	/* Create a GtkCells list from the selected objects */
	cells_list = NULL;
	for(glist = fb->selection, i = 0;
	    (glist != NULL) && (i < nitems_visible);
	    glist = g_list_next(glist), i++
	)
	{
	    o = FileBrowserGetObject(
		fb,
		(gint)glist->data
	    );
	    if(o == NULL)
		continue;

	    cell_pixtext = (GtkCellPixText *)g_malloc0(
		sizeof(GtkCellPixText)
	    );
	    if(cell_pixtext != NULL)
	    {
		FileBrowserIcon *icon = FileBrowserGetIcon(fb, o->icon_num);
		cell_pixtext->type = GTK_CELL_PIXTEXT;
		cell_pixtext->text = o->displayed_name;
		cell_pixtext->spacing = FB_LIST_ICON_TEXT_SPACING;
		if(icon != NULL)
		{
		    cell_pixtext->pixmap = icon->pixmap;
		    cell_pixtext->mask = icon->mask;
		}
		cells_list = g_list_append(
		    cells_list,
		    cell_pixtext
		);
	    }
	}
	if(cells_list != NULL)
	{
	    /* Set the DND drag icon from the GtkCells list */
	    GUIDNDSetDragIconFromCellsList(
		fb->list_da,
		cells_list,
		g_list_length(fb->selection),
		GTK_ORIENTATION_VERTICAL,
		GTK_ORIENTATION_HORIZONTAL,
		FB_LIST_OBJECT_SPACING
	    );

	    /* Delete the GtkCells list */
	    g_list_foreach(
		cells_list,
		(GFunc)g_free,
		NULL
	    );
	    g_list_free(cells_list);
	}
}

/*
 *	Returns the visibility of the object in the list.
 *
 *	The i specifies the object's index.
 *
 *	Returns one of GTK_VISIBILITY_*.
 */
static GtkVisibility FileBrowserListObjectVisibility(
	FileBrowser *fb,
	const gint i
)
{
	gint scroll_x = 0, scroll_y = 0;
	gint x, y, width, height;
	GdkWindow *window;
	GtkWidget *w;
	FileBrowserObject *o;

	if(fb == NULL)
	    return(GTK_VISIBILITY_NONE);

	/* Get list's GdkWindow and its size, making sure it exists and
	 * its size is positive
	 */
	w = fb->list_da;
	window = (w != NULL) ? w->window : NULL;
	if(window == NULL)
	    return(GTK_VISIBILITY_NONE);
	gdk_window_get_size(window, &width, &height);
	if((width <= 0) || (height <= 0))
	    return(GTK_VISIBILITY_NONE);

	/* Get object */
	o = FileBrowserGetObject(fb, i);
	if(o == NULL)
	    return(GTK_VISIBILITY_NONE);

	/* Get current scroll position */
	if(fb->list_hsb != NULL)
	{
	    GtkRange *range = GTK_RANGE(fb->list_hsb);
	    GtkAdjustment *adj = range->adjustment;
	    scroll_x = (gint)((adj != NULL) ? adj->value : 0.0f);
	}
	if(fb->list_vsb != NULL)
	{
	    GtkRange *range = GTK_RANGE(fb->list_vsb);
	    GtkAdjustment *adj = range->adjustment;
	    scroll_y = (gint)((adj != NULL) ? adj->value : 0.0f);
	}

	/* Check visibility by list display format */
	switch(fb->list_format)
	{
	  case FB_LIST_FORMAT_VERTICAL_DETAILS:
	  case FB_LIST_FORMAT_VERTICAL:
	    y = o->y - scroll_y;
	    if(((y + o->height) <= 0) || (y >= height))
		return(GTK_VISIBILITY_NONE);
	    else if((y < 0) || ((y + o->height) > height))
		return(GTK_VISIBILITY_PARTIAL);
	    else
		return(GTK_VISIBILITY_FULL);
	    break;

	  case FB_LIST_FORMAT_STANDARD:
	    x = o->x - scroll_x;
	    if(((x + o->width) <= 0) || (x >= width))
		return(GTK_VISIBILITY_NONE);
	    else if((x < 0) || ((x + o->width) > width))
		return(GTK_VISIBILITY_PARTIAL);
	    else
		return(GTK_VISIBILITY_FULL);
	    break;
	}

	return(GTK_VISIBILITY_NONE);
}

/*
 *	Scrolls to the object in the list.
 *
 *	The i specifies the object's index.
 *
 *	The coefficient specifies the offset within the list window.
 */
static void FileBrowserListMoveToObject(
	FileBrowser *fb,
	const gint i,
	const gfloat coeff
)
{
	gint x, y, width, height;
	GdkWindow *window;
	GtkWidget *w;
	FileBrowserObject *o;

	if(fb == NULL)
	    return;

	w = fb->list_da;
	o = FileBrowserGetObject(fb, i);
	if((w == NULL) || (o == NULL))
	    return;

	window = w->window;
	if(window == NULL)
	    return;

	gdk_window_get_size(window, &width, &height);
	if((width <= 0) || (height <= 0))
	    return;

	/* Move to the object based on the list format */
	switch(fb->list_format)
	{
	  case FB_LIST_FORMAT_VERTICAL_DETAILS:
	  case FB_LIST_FORMAT_VERTICAL:
	    y = o->y + (gint)(o->height * coeff);
	    y -= (gint)(height * CLIP(coeff, 0.0f, 1.0f));
	    if(fb->list_vsb != NULL)
	    {
		GTK_ADJUSTMENT_SET_VALUE(
		    GTK_RANGE(fb->list_vsb)->adjustment,
		    (gfloat)y
		);
	    }
	    break;

	  case FB_LIST_FORMAT_STANDARD:
	    x = o->x + (gint)(o->width * coeff);
	    x -= (gint)(width * CLIP(coeff, 0.0f, 1.0f));
	    if(fb->list_hsb != NULL)
	    {
		GTK_ADJUSTMENT_SET_VALUE(
		    GTK_RANGE(fb->list_hsb)->adjustment,
		    (gfloat)x
		);
	    }
	    break;
	}
}

/*
 *	Gets the object at the coordinates within the list.
 *
 *	The x and y specifies the coordinates in the list window.
 *
 *	Returns the object index or -1 on failed match.
 */
static gint FileBrowserListSelectCoordinates(
	FileBrowser *fb,
	const gint x, const gint y
)
{
	gint i, cx, cy, scroll_x = 0, scroll_y = 0;
	GList *glist;
	FileBrowserObject *o;

	if(fb == NULL)
	    return(-1);

	/* Get scroll position */
	if(fb->list_hsb != NULL)
	{
	    GtkRange *range = GTK_RANGE(fb->list_hsb);
	    GtkAdjustment *adj = range->adjustment;
	    scroll_x = (gint)((adj != NULL) ? adj->value : 0.0f);
	}
	if(fb->list_vsb != NULL)
	{
	    GtkRange *range = GTK_RANGE(fb->list_vsb);
	    GtkAdjustment *adj = range->adjustment;
	    scroll_y = (gint)((adj != NULL) ? adj->value : 0.0f);
	}

	/* Find object by list display format */
	switch(fb->list_format)
	{
	  case FB_LIST_FORMAT_VERTICAL_DETAILS:
	  case FB_LIST_FORMAT_VERTICAL:
	    for(glist = fb->objects_list, i = 0;
		glist != NULL;
		glist = g_list_next(glist), i++
	    )
	    {
	        o = FILE_BROWSER_OBJECT(glist->data);
		if(o == NULL)
		    continue;

		if((o->width <= 0) || (o->height <= 0))
		    continue;

		/* Calculate object position with scrolled adjust */
		cx = o->x;
		cy = o->y - scroll_y;

		/* In bounds? */
		if((x >= cx) && (x < (cx + o->width)) &&
		   (y >= cy) && (y < (cy + o->height))
		)
		    return(i);
	    }
	    break;

	  case FB_LIST_FORMAT_STANDARD:
	    for(glist = fb->objects_list, i = 0;
		glist != NULL;
		glist = g_list_next(glist), i++
	    )
	    {
	        o = FILE_BROWSER_OBJECT(glist->data);
		if(o == NULL)
		    continue;

		if((o->width <= 0) || (o->height <= 0))
		    continue;

		/* Calculate object position with scrolled adjust */
		cx = o->x - scroll_x;
		cy = o->y;

		/* In bounds? */
		if((x >= cx) && (x < (cx + o->width)) &&
		   (y >= cy) && (y < (cy + o->height))
		)
		    return(i);
	    }
	    break;
	}

	return(-1);
}

/*
 *	Redraws the File Browser's list header.
 */
static void FileBrowserListHeaderDraw(FileBrowser *fb)
{
	gint width, height, font_height;
	GdkFont *font;
	GdkDrawable *drawable;
	GdkWindow *window;
	GdkGC *gc;
	GtkStateType state;
	GtkStyle *style;
	GtkWidget *w;

	if(fb == NULL)
	    return;

	gc = fb->gc;
	w = fb->list_header_da;
	if((gc == NULL) || (w == NULL))
	    return;

	if(!GTK_WIDGET_VISIBLE(w))
	    return;

	window = w->window;
	if(window == NULL)
	    return;

	drawable = (GdkDrawable *)fb->list_pm;
	if(drawable == NULL)
	    drawable = window;

	state = GTK_WIDGET_STATE(w);
	style = gtk_widget_get_style(w);
	gdk_window_get_size(window, &width, &height);
	if((style == NULL) || (width <= 0) || (height <= 0))
	    return;

	font = style->font;
	if(font == NULL)
	    return;

	font_height = font->ascent + font->descent;

	/* Draw background */
	gdk_gc_set_function(gc, GDK_COPY);
	gdk_gc_set_foreground(gc, &style->bg[state]);
	gdk_gc_set_fill(gc, GDK_SOLID);
	gdk_draw_rectangle(
	    drawable,
	    gc,
	    TRUE,				/* Fill */
	    0, 0,
	    width, height
	);

	/* Draw the frame around entire header */
	gtk_draw_box(
	    style,
	    drawable,
	    state,
	    GTK_SHADOW_OUT,
	    0, 0,
	    width, height
	);

	/* Any column headings to draw? */
	if(fb->columns_list != NULL)
	{
	    gint last_column_position = 0;
	    GList *glist;
	    GdkRectangle rect;
	    FileBrowserColumn *column;

	    /* Draw each column heading */
	    for(glist = fb->columns_list;
		glist != NULL;
		glist = g_list_next(glist)
	    )
	    {
		column = FILE_BROWSER_COLUMN(glist->data);
		if(column == NULL)
		    continue;

		rect.x = last_column_position;
		rect.y = 0;
		rect.width = MAX(column->position - last_column_position, 0);
		rect.height = height;

		if(rect.width == 0)
		    continue;

		gtk_draw_box(
		    style,
		    drawable,
		    (column->flags & GTK_HAS_FOCUS) ?
			GTK_STATE_ACTIVE : state,
		    (column->flags & GTK_HAS_FOCUS) ?
			GTK_SHADOW_IN : GTK_SHADOW_OUT,
		    rect.x, rect.y, rect.width, rect.height
		);

		gdk_gc_set_foreground(
		    gc,
		    (column->flags & GTK_SENSITIVE) ?
			&style->text[state] : &style->text[GTK_STATE_INSENSITIVE]
		);

		if((column->label != NULL) ? (*column->label != '\0') : FALSE)
		{
		    const gchar *label = column->label;
		    GdkTextBounds b;
		    gdk_string_bounds(font, label, &b);
		    gdk_gc_set_clip_origin(gc, 0, 0);
		    gdk_gc_set_clip_rectangle(gc, &rect);
		    switch(column->label_justify)
		    {
		      case GTK_JUSTIFY_FILL:
		      case GTK_JUSTIFY_CENTER:
			gdk_draw_string(
			    drawable,
			    font,
			    gc,
			    (rect.x + ((rect.width - b.width) / 2)) -
				b.lbearing,
			    (rect.y + ((rect.height - font_height) / 2)) +
				font->ascent,
			    label
			);
			break;
		      case GTK_JUSTIFY_RIGHT:
			gdk_draw_string(
			    drawable,
			    font,
			    gc,
			    (rect.x + rect.width - b.width - 5) -
				b.lbearing,
			    (rect.y + ((rect.height - font_height) / 2)) +
				font->ascent,
			    label
			);
			break;
		      case GTK_JUSTIFY_LEFT:
			gdk_draw_string(
			    drawable,
			    font,
			    gc,
			    (rect.x + 5) - b.lbearing,
			    (rect.y + ((rect.height - font_height) / 2)) +
				font->ascent,
			    label
			);
			break;
		    }
		    gdk_gc_set_clip_rectangle(gc, NULL);
		}

		last_column_position = column->position;
	    }
	}

	/* Send drawable to window if drawable is not the window */
	if(drawable != (GdkDrawable *)window)
	    gdk_draw_pixmap(
		window,
		gc,
		drawable,
		0, 0,
		0, 0,
		width, height
	    );
}

/*
 *	Queues a draw of the File Browser's list header.
 */
static void FileBrowserListHeaderQueueDraw(FileBrowser *fb)
{
	if(fb == NULL)
	    return;

	gtk_widget_queue_draw(fb->list_header_da);
}

/*
 *	Draws the File Browser's list.
 */
static void FileBrowserListDraw(FileBrowser *fb)
{
	gint		scroll_x = 0, scroll_y = 0,
			width, height,
			font_height;
	GdkFont *font;
	GdkDrawable *drawable;
	GdkWindow *window;
	GdkGC *gc;
	GtkStateType state;
	GtkStyle *style;
	GtkWidget *w;
	FileBrowserDragFlags drag_flags;

	if(fb == NULL)
	    return;

	drag_flags = fb->drag_flags;
	gc = fb->gc;
	w = fb->list_da;
	if((gc == NULL) || (w == NULL))
	    return;

	/* Do not draw if the list is not visible */
#if 0
	if(!GTK_WIDGET_VISIBLE(w))
	    return;
#endif
	window = w->window;
	if(window == NULL)
	    return;

	drawable = (GdkDrawable *)fb->list_pm;
	if(drawable == NULL)
	    drawable = window;

	state = GTK_WIDGET_STATE(w);
	style = gtk_widget_get_style(w);
	gdk_window_get_size(window, &width, &height);
	if((style == NULL) || (width <= 0) || (height <= 0))
	    return;

	font = style->font;
	if(font == NULL)
	    return;

	font_height = font->ascent + font->descent;

	/* Draw the background */
	gdk_gc_set_function(gc, GDK_COPY);
	gdk_gc_set_fill(gc, GDK_SOLID);
	gdk_gc_set_foreground(gc, &style->base[state]);
	gdk_draw_rectangle(
	    drawable,
	    gc,
	    TRUE,				/* Fill */
	    0, 0,
	    width, height
	);
#if 0
	if(style->bg_pixmap[state] != NULL)
	    gtk_style_apply_default_background(
		style,
		drawable,
		FALSE,
		state,
		NULL,
		0, 0,
		width, height
	    );
#endif

	/* Get the scroll position */
	if(fb->list_hsb != NULL)
	{
	    GtkRange *range = GTK_RANGE(fb->list_hsb);
	    GtkAdjustment *adj = range->adjustment;
	    scroll_x = (gint)((adj != NULL) ? adj->value : 0.0f);
	}
	if(fb->list_vsb != NULL)
	{
	    GtkRange *range = GTK_RANGE(fb->list_vsb);
	    GtkAdjustment *adj = range->adjustment;
	    scroll_y = (gint)((adj != NULL) ? adj->value : 0.0f);
	}

	/* Begin drawing objects by list display format */
	switch(fb->list_format)
	{
	    gboolean o_is_selected, o_is_dragged;
	    gint i, x, y, icon_width;
	    GList *glist;
	    GdkTextBounds b;
	    GdkColor *fg_color, *text_color;
	    FileBrowserObject *o;
	    FileBrowserIcon *icon;

	  case FB_LIST_FORMAT_VERTICAL_DETAILS:
	    /* Make sure we have enough columns to draw for this
	     * list display format
	     */
	    if(g_list_length(fb->columns_list) >= 4)
	    {
		mode_t m;
		struct stat *lstat_buf;
		GdkRectangle rect;
		FileBrowserColumn *column[4];

		/* Get the pointers to each column */
		for(glist = fb->columns_list, i = 0;
		    (glist != NULL) && (i < 4);
		    glist = g_list_next(glist), i++
		)
		    column[i] = FILE_BROWSER_COLUMN(glist->data);

		/* Draw each object */
		for(glist = fb->objects_list, i = 0;
		    glist != NULL;
		    glist = g_list_next(glist), i++
		)
		{
		    o = FILE_BROWSER_OBJECT(glist->data);
		    if(o == NULL)
			continue;

		    if((o->width <= 0) || (o->height <= 0))
			continue;

		    x = o->x;
		    y = o->y - scroll_y;

		    /* Is this object completely off the y axis? */
		    if(((y + o->height) < 0) || (y >= height))
			continue;

		    /* Get this object's values */
		    lstat_buf = &o->lstat_buf;
		    m = lstat_buf->st_mode;

		    /* Get calculate this object's boundary
		     * rectangle
		     */
		    rect.x = x - 0;
		    rect.y = y;
		    rect.width = MIN(o->width, column[0]->position - 0);
		    rect.height = o->height;

		    /* Is this object selected? */
		    o_is_selected = FB_IS_OBJECT_SELECTED(fb, i);
		    if(o_is_selected)
		    {
			/* Mark the object as dragged as a move if
			 * we initiate the drag and it is a move
			 */
			if(fb->drag_flags & FILE_BROWSER_DRAG_ACTIVE)
			    o_is_dragged = (fb->drag_flags & FILE_BROWSER_DRAG_ACTIVE_MOVE) ? TRUE : FALSE;
			else
			    o_is_dragged = FALSE;

			/* Get the selected GCs and draw the selected
			 * background
			 */
			gdk_gc_set_foreground(gc, &style->bg[GTK_STATE_SELECTED]);
			if(o_is_dragged)
			{
			    /* Object is dragged as a move, draw with
			     * the transparent stipple GdkBitmap
			     */
			    gdk_gc_set_stipple(
				gc,
				(GdkPixmap *)fb->transparent_stipple_bm
			    );
			    gdk_gc_set_ts_origin(gc, 0, 0);
			    gdk_gc_set_fill(gc, GDK_STIPPLED);
			}
			if(rect.width > 0)
			    gdk_draw_rectangle(
				drawable,
				gc,
				TRUE,			/* Fill */
				rect.x, rect.y,
				rect.width, rect.height
			    );

			fg_color = &style->fg[GTK_STATE_SELECTED];
			text_color = &style->text[GTK_STATE_SELECTED];
		    }
		    else
		    {
			fg_color = &style->fg[state];
			text_color = &style->text[state];
			o_is_dragged = FALSE;
		    }

		    /* Get the icon, its width, and draw it */
		    icon = FileBrowserGetIcon(fb, o->icon_num);
		    if((icon != NULL) ? (icon->pixmap != NULL) : FALSE)
		    {
			GdkBitmap *mask = icon->mask;
			GdkPixmap *pixmap = icon->pixmap;
			gdk_gc_set_clip_origin(gc, x, y);
			gdk_gc_set_clip_mask(gc, mask);
			gdk_draw_pixmap(
			    drawable,
			    gc,
			    (GdkDrawable *)pixmap,
			    0, 0,		/* Source position */
			    x, y,		/* Target position */
			    icon->width, icon->height	/* Size */
			);
			gdk_gc_set_clip_mask(gc, NULL);
			icon_width = icon->width;
		    }
		    else
		    {
			icon_width = 0;
		    }

		    /* Draw the displayed name */
		    if(o->displayed_name != NULL)
		    {
			const gchar *s = o->displayed_name;
			gdk_string_bounds(font, s, &b);
			gdk_gc_set_clip_rectangle(gc, &rect);
			gdk_gc_set_foreground(gc, text_color);
			gdk_draw_string(
			    drawable,
			    font,
			    gc,
			    (x + icon_width + FB_LIST_ICON_TEXT_SPACING) -
				b.lbearing,
			    (y + ((o->height - font_height) / 2)) +
				font->ascent,
			    s
			);
		    }
		    else
		    {
			/* No name, but we still need to have the font
			 * extents for use for the subsequent drawing
			 */
			gdk_string_bounds(font, "X", &b);
		    }

		    /* Stop using the clip rectangle */
		    gdk_gc_set_clip_rectangle(gc, NULL);

		    /* Use the non-selected colors */
		    fg_color = &style->fg[state];
		    text_color = &style->text[state];

		    /* Draw the size */
		    if(S_ISREG(m))
		    {
			gchar s[80];
			g_snprintf(
			    s, sizeof(s),
			    "%ld",
			    (gulong)lstat_buf->st_size
			);
			gdk_string_bounds(font, s, &b);
			rect.x = column[0]->position;
			rect.width = column[1]->position - column[0]->position;
			if(rect.width > 0)
			{
			    gdk_gc_set_clip_rectangle(gc, &rect);
			    gdk_gc_set_foreground(gc, text_color);
			    gdk_draw_string(
				drawable,
				font,
				gc,
				(rect.x + rect.width - b.width - 2) -
				    b.lbearing,
				(y + (o->height / 2) -
				    ((font->ascent + font->descent) / 2)) +
				    font->ascent,
				s
			    );
			}
		    }

		    /* Draw the permissions */
#ifdef S_ISLNK
		    if(!S_ISLNK(m))
#else
		    if(TRUE)
#endif
		    {
			gchar s[80];
#if defined(S_IRUSR) && defined(S_IWUSR) && defined(S_IXUSR)
			g_snprintf(
			    s, sizeof(s),
			    "%c%c%c%c%c%c%c%c%c",
			    (m & S_IRUSR) ? 'r' : '-',
			    (m & S_IWUSR) ? 'w' : '-',
			    (m & S_ISUID) ? 'S' :
				((m & S_IXUSR) ? 'x' : '-'),
			    (m & S_IRGRP) ? 'r' : '-',
			    (m & S_IWGRP) ? 'w' : '-',
			    (m & S_ISGID) ? 'G' :
				((m & S_IXGRP) ? 'x' : '-'),
			    (m & S_IROTH) ? 'r' : '-',
			    (m & S_IWOTH) ? 'w' : '-',
			    (m & S_ISVTX) ? 'T' :
				((m & S_IXOTH) ? 'x' : '-')
			);
#else
			strcpy(s, "rwxrwxrwx");
#endif
			gdk_string_bounds(font, s, &b);
			rect.x = column[1]->position;
			rect.width = column[2]->position - column[1]->position;
			if(rect.width > 0)
			{
			    gdk_gc_set_clip_rectangle(gc, &rect);
			    gdk_gc_set_foreground(gc, text_color);
			    gdk_draw_string(
				drawable,
				font,
				gc,
				(rect.x + 2) - b.lbearing,
				(y + (o->height / 2) -
				    ((font->ascent + font->descent) / 2)) +
				    font->ascent,
				s
			    );
			}
		    }

		    /* Draw the modified time */
		    if(S_ISREG(m))
		    {
			time_t mtime = lstat_buf->st_mtime;
			gchar *s = STRDUP((mtime > 0) ? ctime(&mtime) : "*undefined*");
			gchar *s2 = strchr(s, '\n');
			if(s2 == NULL)
			    s2 = strchr(s, '\r');
			if(s2 != NULL)
			    *s2 = '\0';
			gdk_string_bounds(font, s, &b);
			rect.x = column[2]->position;
			rect.width = column[3]->position - column[2]->position;
			if(rect.width > 0)
			{
			    gdk_gc_set_clip_rectangle(gc, &rect);
			    gdk_gc_set_foreground(gc, text_color);
			    gdk_draw_string(
				drawable,
				font,
				gc,
				(rect.x + 2) - b.lbearing,
				(y + (o->height / 2) -
				    ((font->ascent + font->descent) / 2)) +
				    font->ascent,
				s
			    );
			}
			g_free(s);
		    }

		    /* Stop using the clip rectangle */
		    gdk_gc_set_clip_rectangle(gc, NULL);

		    /* Draw the focus rectangle around this object? */
		    if(GTK_WIDGET_HAS_FOCUS(w) ||
		       (fb->drag_flags & FILE_BROWSER_DRAG_OVER)
		    )
		    {
			if(i == fb->focus_object)
			{
			    rect.x = x - 0;
			    rect.width = MIN(o->width, column[0]->position - 0);
			    gdk_gc_set_function(gc, GDK_INVERT);
			    gdk_draw_rectangle(
				drawable,
				gc,
				FALSE,		/* Outline */
				rect.x, rect.y,
				rect.width - 1, rect.height - 1
			    );
			    gdk_gc_set_function(gc, GDK_COPY);
			}
		    }

		    /* Restore the graphic context */
		    if(o_is_dragged)
		    {
			gdk_gc_set_fill(gc, GDK_SOLID);
		    }
		}
	    }
	    break;

	  case FB_LIST_FORMAT_VERTICAL:
	    for(glist = fb->objects_list, i = 0;
		glist != NULL;
		glist = g_list_next(glist), i++
	    )
	    {
		o = FILE_BROWSER_OBJECT(glist->data);
		if(o == NULL)
		    continue;

		if((o->width <= 0) || (o->height <= 0))
		    continue;

		x = o->x;
		y = o->y - scroll_y;

		/* Is this object completely off the y axis? */
		if(((y + o->height) < 0) || (y >= height))
		    continue;

		/* Is object selected? */
		o_is_selected = FB_IS_OBJECT_SELECTED(fb, i);
		if(o_is_selected)
		{
		    /* Mark the object as dragged as a move if we
		     * initiate the drag and it is a move
		     */
		    if(fb->drag_flags & FILE_BROWSER_DRAG_ACTIVE)
			o_is_dragged = (fb->drag_flags & FILE_BROWSER_DRAG_ACTIVE_MOVE) ? TRUE : FALSE;
		    else
			o_is_dragged = FALSE;

		    /* Get the selected GCs and draw the selected
		     * background
		     */
		    gdk_gc_set_foreground(gc, &style->bg[GTK_STATE_SELECTED]);
		    if(o_is_dragged)
		    {
			/* Object is dragged as a move, draw with the
			 * transparent stipple GdkBitmap
			 */
			gdk_gc_set_stipple(
			    gc,
			    (GdkPixmap *)fb->transparent_stipple_bm
			);
			gdk_gc_set_ts_origin(gc, 0, 0);
			gdk_gc_set_fill(gc, GDK_STIPPLED);
		    }
		    if(o->width > 0)
			gdk_draw_rectangle(
			    drawable,
			    gc,
			    TRUE,		/* Fill */
			    x, y,
			    o->width, o->height
			);

		    fg_color = &style->fg[GTK_STATE_SELECTED];
		    text_color = &style->text[GTK_STATE_SELECTED];
		}
		else
		{
		    fg_color = &style->fg[state];
		    text_color = &style->text[state];
		    o_is_dragged = FALSE;
		}

		/* Get the icon, its size, and draw it */
		icon = FileBrowserGetIcon(fb, o->icon_num);
		if((icon != NULL) ? (icon->pixmap != NULL) : FALSE)
		{
		    GdkBitmap *mask = icon->mask;
		    GdkPixmap *pixmap = icon->pixmap;
		    gdk_gc_set_clip_origin(gc, x, y);
		    gdk_gc_set_clip_mask(gc, mask);
		    gdk_draw_pixmap(
			drawable,
			gc,
			(GdkDrawable *)pixmap,
			0, 0,			/* Source position */
			x, y,			/* Target position */
			icon->width, icon->height	/* Size */
		    );
		    gdk_gc_set_clip_mask(gc, NULL);
		    icon_width = icon->width;
		}
		else
		{
		    icon_width = 0;
		}

		/* Draw the name */
		if(o->displayed_name != NULL)
		{
		    const gchar *s = o->displayed_name;
		    gdk_string_bounds(font, s, &b);
		    gdk_gc_set_foreground(gc, text_color);
		    gdk_draw_string(
			drawable,
			font,
			gc,
			(x + icon_width + FB_LIST_ICON_TEXT_SPACING) -
			    b.lbearing,
			(y + ((o->height - font_height) / 2)) +
			    font->ascent,
			s
		    );
		}

		/* Draw the focus rectangle around this object? */
		if(GTK_WIDGET_HAS_FOCUS(w) ||
		   (fb->drag_flags & FILE_BROWSER_DRAG_OVER)
		)
		{
		    if(i == fb->focus_object)
		    {
			gdk_gc_set_function(gc, GDK_INVERT);
			gdk_draw_rectangle(
			    drawable,
			    gc,
			    FALSE,		/* Outline */
			    x, y,
			    o->width - 1, o->height - 1
			);
			gdk_gc_set_function(gc, GDK_COPY);
		    }
		}

		/* Restore the graphic context */
		if(o_is_dragged)
		{
		    gdk_gc_set_fill(gc, GDK_SOLID);
		}
	    }
	    break;

	  case FB_LIST_FORMAT_STANDARD:
	    /* Draw each object */
	    for(glist = fb->objects_list, i = 0;
		glist != NULL;
		glist = g_list_next(glist), i++
	    )
	    {
		o = FILE_BROWSER_OBJECT(glist->data);
		if(o == NULL)
		    continue;

		if((o->width <= 0) || (o->height <= 0))
		    continue;

		x = o->x - scroll_x;
		y = o->y;

		/* Is this object completely off the x axis? */
		if(((x + o->width) < 0) || (x >= width))
		    continue;

		/* Is this object selected? */
		o_is_selected = FB_IS_OBJECT_SELECTED(fb, i);
		if(o_is_selected)
		{
		    /* Mark the object as dragged as a move if we
		     * initiate the drag and it is a move
		     */
		    if(fb->drag_flags & FILE_BROWSER_DRAG_ACTIVE)
			o_is_dragged = (fb->drag_flags & FILE_BROWSER_DRAG_ACTIVE_MOVE) ? TRUE : FALSE;
		    else
			o_is_dragged = FALSE;

		    /* Get the selected GCs and draw the selected
		     * background
		     */
		    gdk_gc_set_foreground(gc, &style->bg[GTK_STATE_SELECTED]);
		    if(o_is_dragged)
		    {
			/* Object is dragged as a move, draw with the
			 * transparent stipple GdkBitmap
			 */
			gdk_gc_set_stipple(
			    gc,
			    (GdkPixmap *)fb->transparent_stipple_bm
			);
			gdk_gc_set_ts_origin(gc, 0, 0);
			gdk_gc_set_fill(gc, GDK_STIPPLED);
		    }
		    if(o->width > 0)
			gdk_draw_rectangle(
			    drawable,
			    gc,
			    TRUE,		/* Fill */
			    x, y,
			    o->width, o->height
			);

		    fg_color = &style->fg[GTK_STATE_SELECTED];
		    text_color = &style->text[GTK_STATE_SELECTED];
		}
		else
		{
		    fg_color = &style->fg[state];
		    text_color = &style->text[state];
		    o_is_dragged = FALSE;
		}

		/* Get the icon, its width, and draw it */
		icon = FileBrowserGetIcon(fb, o->icon_num);
		if((icon != NULL) ? (icon->pixmap != NULL) : FALSE)
		{
		    GdkBitmap *mask = icon->mask;
		    GdkPixmap *pixmap = icon->pixmap;
		    gdk_gc_set_clip_origin(gc, x, y);
		    gdk_gc_set_clip_mask(gc, mask);
		    gdk_draw_pixmap(
			drawable,
			gc,
			(GdkDrawable *)pixmap,
			0, 0,			/* Source position */
			x, y,			/* Target position */
			icon->width, icon->height	/* Size */
		    );
		    gdk_gc_set_clip_mask(gc, NULL);
		    icon_width = icon->width;
		}
		else
		{
		    icon_width = 0;
		}

		/* Draw the name */
		if(o->displayed_name != NULL)
		{
		    const gchar *s = o->displayed_name;
		    gdk_string_bounds(font, s, &b);
		    gdk_gc_set_foreground(gc, text_color);
		    gdk_draw_string(
			drawable,
			font,
			gc,
			(x + icon_width + FB_LIST_ICON_TEXT_SPACING) -
			    b.lbearing,
			(y + ((o->height - font_height) / 2)) +
			    font->ascent,
			s
		    );
		}

		/* Draw the focus rectangle around this object? */
		if(GTK_WIDGET_HAS_FOCUS(w) ||
		   (fb->drag_flags & FILE_BROWSER_DRAG_OVER)
		)
		{
		    if(i == fb->focus_object)
		    {
			gdk_gc_set_function(gc, GDK_INVERT);
			gdk_draw_rectangle(
			    drawable,
			    gc,
			    FALSE,		/* Outline */
			    x, y,
			    o->width - 1, o->height - 1
			);
			gdk_gc_set_function(gc, GDK_COPY);
		    }
		}

		/* Restore the GCs */
		if(o_is_dragged)
		{
		    gdk_gc_set_fill(gc, GDK_SOLID);
		}
	    }
	    break;
	}

	/* Is there a drag over this list? */
	if(drag_flags & FILE_BROWSER_DRAG_OVER)
	    gtk_draw_focus(
		style,
		drawable,
		0, 0,
		width - 1, height - 1
	    );

	/* Send drawable to window if drawable is not the window */
	if(drawable != (GdkDrawable *)window)
	    gdk_draw_pixmap(
		(GdkDrawable *)window,
		style->fg_gc[state],
		drawable,
		0, 0,				/* Source position */
		0, 0,				/* Target position */
		width, height			/* Size */
	    );
}

/*
 *	Queues a redraw of the File Browser's list.
 */
static void FileBrowserListQueueDraw(FileBrowser *fb)
{
	if(fb == NULL)
	    return;

	gtk_widget_queue_draw(fb->list_da);
}


/*
 *	Creates a new list column.
 */
static FileBrowserColumn *FileBrowserColumnNew(void)
{
	return(FILE_BROWSER_COLUMN(g_malloc0(
	    sizeof(FileBrowserColumn)
	)));
}

/*
 *	Deletes the list column.
 */
static void FileBrowserColumnDelete(FileBrowserColumn *column)
{
	if(column == NULL)
	    return;

	g_free(column->label);
	g_free(column);
}

/*
 *	Returns the list column at index i.
 */
static FileBrowserColumn *FileBrowserListGetColumn(
	FileBrowser *fb,
	const gint i
)
{
	if((fb == NULL) || (i < 0))
	    return(NULL);

	return(FILE_BROWSER_COLUMN(g_list_nth_data(
	    fb->columns_list,
	    (guint)i)
	));
}

/*
 *	Appends a new list column.
 */
static FileBrowserColumn *FileBrowserListColumnAppend(
	FileBrowser *fb,
	const gchar *label,
	const gint width
)
{
	gint ncolumns;
	FileBrowserColumn *column, *column_prev;

	if(fb == NULL)
	    return(NULL);

	/* Create a new list column */
	column = FileBrowserColumnNew();
	if(column == NULL)
	    return(NULL);

	fb->columns_list = g_list_append(fb->columns_list, column);
	ncolumns = g_list_length(fb->columns_list);

	column->label = STRDUP(label);
	column_prev = FileBrowserListGetColumn(fb, ncolumns - 2);
	if(column_prev != NULL)
	    column->position = column_prev->position + width;
	else
	    column->position = width;
	column->label_justify = GTK_JUSTIFY_LEFT;
	column->flags = GTK_SENSITIVE | GTK_CAN_FOCUS |
			GTK_CAN_DEFAULT;

	column->drag = FALSE;
	column->drag_position = column->position;
	column->drag_last_drawn_position = column->drag_position;

	return(column);
}

/*
 *	Deletes all the list columns.
 */
static void FileBrowserListColumnsClear(FileBrowser *fb)
{
	if(fb == NULL)
	    return;

	if(fb->columns_list != NULL)
	{
	    g_list_foreach(
		fb->columns_list, (GFunc)FileBrowserColumnDelete, NULL
	    );
	    g_list_free(fb->columns_list);
	    fb->columns_list = NULL;
	}
}


/*
 *	Updates the File Name GtkEntry with the current selected
 *	objects.
 */
static void FileBrowserEntrySetSelectedObjects(FileBrowser *fb)
{
	gchar *s, *s2;
	GList *glist;
	GtkEntry *entry;
	const FileBrowserObject *o;

	if(fb == NULL)
	    return;

	entry = GTK_ENTRY(fb->entry);

	s = NULL;
	for(glist = fb->selection;
	    glist != NULL;
	    glist = g_list_next(glist)
	)
	{
	    o = FileBrowserGetObject(fb, (gint)glist->data);
	    if(o == NULL)
		continue;

	    if(o->name == NULL)
		continue;

	    if(s != NULL)
		s2 = g_strconcat(
		    s,
		    ",",
		    o->name,
		    NULL
		);
	    else
		s2 = STRDUP(o->name);
	    g_free(s);
	    s = s2;
	}

	gtk_entry_set_text(entry, (s != NULL) ? s : "");
	g_free(s);
	gtk_entry_set_position(entry, -1);
}


/*
 *	Sets the types list.
 *
 *	The list and total specifies the new types list.
 */
static void FileBrowserTypesListSetTypes(
	FileBrowser *fb,
	fb_type_struct **list, const gint total
)
{
	gint i, n;
	gchar *label;
	GtkWidget *pulist, *pulistbox;
	fb_type_struct *t, *t2;

	if(fb == NULL)
	    return;

	pulistbox = fb->types_pulistbox;
	pulist = PUListBoxGetPUList(pulistbox);
	if(pulist == NULL)
	    return;

	fb->freeze_count++;

	/* Delete all the types in the list */
	PUListClear(pulist);

	/* Set the new types list */
	for(i = 0; i < total; i++) 
	{
	    t = list[i];
	    if(t == NULL)
		continue;

	    if(t->ext == NULL)
		continue;

	    t2 = FB_TYPE(g_malloc0(sizeof(fb_type_struct)));
	    if(t2 == NULL)
		continue;

	    t2->ext = STRDUP(t->ext);
	    t2->name = STRDUP(t->name);

	    if(STRISEMPTY(t2->name))
		label = STRDUP(t2->ext);
	    else
		label = g_strdup_printf(
		    "%s (%s)", t2->name, t2->ext
		);
	    n = PUListAddItem(
		pulist,
		label
	    );
	    g_free(label);
	    if(n < 0)
	    {
		FileBrowserTypeDelete(t2);
		continue;
	    }

	    PUListSetItemDataFull(
		pulist,
		n,
		t2, FileBrowserTypesPUListItemDestroyCB
	    );
	}

	/* Make sure that no more than 10 items are visible */
	n = PUListGetTotalItems(pulist);
	PUListBoxSetLinesVisible(
	    pulistbox,
	    CLIP(n, 1, 10)
	);

	/* Select the first item */
	PUListBoxSelect(pulistbox, 0);
	FileBrowserTypesPUListChangedCB(pulistbox, 0, fb);

	fb->freeze_count--;
}


/*
 *	Create new directory.
 */
static void FileBrowserNewDirectory(FileBrowser *fb)
{
	const guint m = (guint)umask(0);
	gchar *name, *path;
	const gchar *cur_location;
	GtkWidget *toplevel;

	umask((mode_t)m);

	if(fb == NULL)
	    return;

#if defined(PROG_LANGUAGE_SPANISH)
# define TITLE_MKDIR_FAILED	"Cree Guía Nueva Fallada"
#elif defined(PROG_LANGUAGE_FRENCH)
# define TITLE_MKDIR_FAILED	"Créer Le Nouvel Annuaire Echoué"
#elif defined(PROG_LANGUAGE_GERMAN)
# define TITLE_MKDIR_FAILED	"Schaffen Sie Neues Versagten Verzeichnis"
#elif defined(PROG_LANGUAGE_ITALIAN)
# define TITLE_MKDIR_FAILED	"Creare L'Elenco Nuovo Fallito"
#elif defined(PROG_LANGUAGE_DUTCH)
# define TITLE_MKDIR_FAILED	"Creëer Nieuwe Gids Verzuimde"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
# define TITLE_MKDIR_FAILED	"Crie Novo Guia Fracassado"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
# define TITLE_MKDIR_FAILED	"Skap New Directory Failed"
#else
# define TITLE_MKDIR_FAILED	"Create New Directory Failed"
#endif

#define MESSAGE_MKDIR_FAILED(s)		{	\
 CDialogSetTransientFor(toplevel);		\
 CDialogGetResponse(				\
  TITLE_MKDIR_FAILED,				\
  (s), NULL,					\
  CDIALOG_ICON_WARNING,				\
  CDIALOG_BTNFLAG_OK, CDIALOG_BTNFLAG_OK	\
 );						\
 CDialogSetTransientFor(NULL);			\
}

	FileBrowserSetBusy(fb, TRUE);

	toplevel = fb->toplevel;
	cur_location = FileBrowserGetLocation(fb);
	if(cur_location == NULL)
	{
	    FileBrowserSetBusy(fb, FALSE);
	    return;
	}

	/* Generate new directory's name and path */
	name = STRDUP(FB_NEW_DIRECTORY_NAME);
	path = STRDUP(PrefixPaths(
	    (const char *)cur_location,
	    (const char *)name
	));
	if(path == NULL)
	{
	    g_free(name);
	    FileBrowserSetBusy(fb, FALSE);
	    return;
	}

	/* Create the new directory */
#if defined(_WIN32)
	if(mkdir((const char *)path))
#else
	if(mkdir(
	    (const char *)path,
	    (mode_t)(~m) &
		(S_IRUSR | S_IWUSR | S_IXUSR |
		 S_IRGRP | S_IWGRP | S_IXGRP |
		 S_IROTH | S_IWOTH | S_IXOTH)
	))
#endif
	{
	    gboolean created = FALSE;
	    gint i, error_code;

	    /* Failed to create the new directory, try creating it
	     * under a different name
	     */
	    for(i = 2; i < 100; i++)
	    {
		/* Regenerate new name and full path */
		g_free(name);
		g_free(path);
		name = g_strdup_printf(
		    "%s%i",
		    FB_NEW_DIRECTORY_NAME, i
		);
		path = STRDUP(PrefixPaths(
		    (const char *)cur_location,
		    (const char *)name
		));
		if(path == NULL)
		    break;

		/* Create the new directory with the new name */
#if defined(_WIN32)
		if(!mkdir((const char *)path))
#else
		if(!mkdir(
		    (const char *)path,
	            (mode_t)(~m) &
	                (S_IRUSR | S_IWUSR | S_IXUSR |
	                 S_IRGRP | S_IWGRP | S_IXGRP |
	                 S_IROTH | S_IWOTH | S_IXOTH)
		))
#endif
		{
		    created = TRUE;
		    break;
		}

		/* Error creating directory and it was not because it
		 * already exists?
		 */
		error_code = (gint)errno;
		if(error_code != EEXIST)
		{
		    gchar *s = g_strdup_printf(
			"%s.",
			g_strerror(error_code)
		    );
		    MESSAGE_MKDIR_FAILED(s);
		    g_free(s);
		    break;
		}
	    }
	    /* Failed to create the new directory? */
	    if(!created)
	    {
		g_free(name);
		g_free(path);
		FileBrowserSetBusy(fb, FALSE);
		return;
	    }
	}

	/* Update the list */
	FileBrowserListUpdate(fb, NULL);

	/* Select the newly created directory */
	if(path != NULL)
	{
	    gint i;
	    GList *glist;
	    FileBrowserObject *o;

	    for(glist = fb->objects_list, i = 0;
		glist != NULL;
		glist = g_list_next(glist), i++
	    )
	    {
		o = FILE_BROWSER_OBJECT(glist->data);
		if(o == NULL)
		    continue;

		if(o->full_path == NULL)
		    continue;

		if(!strcmp(
		    (const char *)o->full_path,
		    (const char *)path
		))
		{
		    /* Select this object */
		    fb->focus_object = i;
		    fb->selection = g_list_append(
			fb->selection,
			(gpointer)i
		    );
		    fb->selection_end = g_list_last(fb->selection);

		    /* Scroll to this object */
		    FileBrowserListMoveToObject(fb, i, 0.5f);

		    break;
		}
	    }
	}
	FileBrowserListQueueDraw(fb);

	/* Update the File Name GtkEntry's value */
	FileBrowserEntrySetSelectedObjects(fb);

	/* Notify about the created directory */
	if(fb->object_created_cb != NULL)
	    fb->object_created_cb(
		path,
		fb->object_created_data
	    );

	g_free(name);
	g_free(path);

	FileBrowserSetBusy(fb, FALSE);

#undef MESSAGE_MKDIR_FAILED
#undef TITLE_MKDIR_FAILED
}

/*
 *	Move iteration.
 */
static gint FileBrowserMoveIteration(
	FileBrowser *fb,
	const gchar *old_path,
	const gchar *new_path
)
{
	struct stat lstat_buf;

	/* If the target already exists then fail */
#if defined(_WIN32)
	if(!stat((const char *)new_path, &lstat_buf))
#else
	if(!lstat((const char *)new_path, &lstat_buf))
#endif
	{
	    /* Report that the target already exists */
	    errno = EEXIST;
	    return(-1);
	}

	/* Move */
	if(rename(
	    (const char *)old_path,
	    (const char *)new_path
	))
	{
	    /* Move failed */
	    const gint error_code = (gint)errno;

	    /* Moving across devices? */
#ifdef EXDEV
	    if(error_code == EXDEV)
#else
	    if(FALSE)
#endif	
	    {
		/* Need to copy the source to the target and then
		 * delete the source
		 */
		const gint status = FileBrowserCopyIteration(
		    fb,
		    old_path,
		    new_path
		);
		if(status != 0)
		{
		    /* Copy failed */
		    return(status);
		}
		else
		{
		    /* Copy successful, delete the source */
		    return(FileBrowserDeleteIteration(fb, old_path));
		}
	    }
	    else
	    {
		/* Move failed, other error */
		return(-1);
	    }
	}
	else
	{
	    /* Move successful */
	    return(0);
	}
}

/*
 *	Copy iteration.
 */
static gint FileBrowserCopyIteration(
	FileBrowser *fb,
	const gchar *old_path,
	const gchar *new_path
)
{
	mode_t m;
	struct stat lstat_buf;

	/* If the target already exists then fail */
#if defined(_WIN32)
	if(!stat((const char *)new_path, &lstat_buf))
#else
	if(!lstat((const char *)new_path, &lstat_buf))
#endif
	{
	    /* Report that the target already exists */
	    errno = EEXIST;
	    return(-1);
	}

	/* Get the source object's local statistics */
#if defined(_WIN32)
	if(stat((const char *)old_path, &lstat_buf))
#else
	if(lstat((const char *)old_path, &lstat_buf))
#endif
	    return(-1);

	m = lstat_buf.st_mode;

	/* Directory */
#ifdef S_ISDIR
	if(S_ISDIR(m))
#else
	if(FALSE)
#endif
	{
#if !defined(_WIN32)
	    struct utimbuf utime_buf;
#endif
	    gint status;
	    gchar **names_list;

	    /* Create the target directory */
	    if(mkdir(
		(const char *)new_path,
		m
	    ))
		return(-1);

	    /* Set the target directory's properties */
	    chown(
		(const char *)new_path,
		lstat_buf.st_uid,
		lstat_buf.st_gid
	    );
#if !defined(_WIN32)
	    utime_buf.actime = lstat_buf.st_atime;
	    utime_buf.modtime = lstat_buf.st_mtime;
	    (void)utime(
		(const char *)new_path,
		&utime_buf
	    );
#endif

	    /* Get the list of objects in the source directory */
	    status = 0;
	    names_list = GetDirEntNames(old_path);
	    if(names_list != NULL)
	    {
		gint i;
		gchar *name, *old_child_path, *new_child_path;

		/* Copy all the objects in the source directory */
		for(i = 0; names_list[i] != NULL; i++)
		{
		    name = names_list[i];
		    if(!strcmp((const char *)name, ".") ||
		       !strcmp((const char *)name, "..")
		    )
		    {
			g_free(name);
			continue;
		    }

		    /* Get the full path to this child object */
		    old_child_path = STRDUP(PrefixPaths(
			(const char *)old_path,
			(const char *)name
		    ));
		    new_child_path = STRDUP(PrefixPaths(
			(const char *)new_path,
			(const char *)name
		    ));
		    if((old_child_path != NULL) && (new_child_path != NULL))
		    {
			/* Copy this child object */
			const gint status2 = FileBrowserCopyIteration(
			    fb,
			    old_child_path,
			    new_child_path
			);
			if(status2 != 0)
			{
			    if(status == 0)
				status = status2;
			}
		    }

		    g_free(old_child_path);
		    g_free(new_child_path);

		    g_free(name);
		}

		g_free(names_list);
	    }

	    return(status);
	}
	/* Link */
#ifdef S_ISLNK
	else if(S_ISLNK(m))
#else
	else if(FALSE)
#endif
	{
	    const gint target_len = (gint)lstat_buf.st_size;
	    gint bytes_read;
	    gchar *target = (gchar *)g_malloc(
		(target_len + 1) * sizeof(gchar)
	    );
	    if(target == NULL)
		return(-3);

	    /* Get the source link's target */
	    bytes_read = readlink(old_path, target, (size_t)target_len);
	    if(bytes_read < 0)
	    {
		const gint error_code = (gint)errno;
		g_free(target);
		errno = (gint)error_code;
		return(-1);
	    }
	    if(bytes_read < target_len)
		target[bytes_read] = '\0';
	    else
		target[target_len] = '\0';

	    /* Create the target link */
	    if(symlink(
		(const char *)target,
		(const char *)new_path
	    ))
	    {
		const gint error_code = (gint)errno;
		g_free(target);
		errno = (gint)error_code;
		return(-1);
	    }

	    /* Set the target link's properties */
	    lchown(
		(const char *)new_path,
		lstat_buf.st_uid,
		lstat_buf.st_gid
	    );

	    /* Delete the target value */
	    g_free(target);

	    return(0);		
	}
	/* File */
#ifdef S_ISREG
	else if(S_ISREG(m))
#else
	else if(FALSE)
#endif
	{
#if !defined(_WIN32)
	    struct utimbuf utime_buf;
#endif
	    struct stat tar_stat_buf;
	    FILE	*src_fp,
			*tar_fp;
	    gint	tar_fd;
	    const gulong file_size = (gulong)lstat_buf.st_size;
	    gulong	io_buf_len,
			coppied_size;
	    guint8 *io_buf;

	    /* Open the source file for reading */
	    src_fp = fopen((const char *)old_path, "rb");
	    if(src_fp == NULL)
		return(-1);

	    /* Create/open the target file for writing */
	    tar_fp = fopen((const char *)new_path, "wb");
	    if(tar_fp == NULL)
	    {
		const gint error_code = (gint)errno;
		fclose(src_fp);
		errno = (int)error_code;
		return(-1);
	    }

	    tar_fd = (gint)fileno(tar_fp);

	    /* Calculate the size of the I/O buffer */
#if defined(_WIN32)
	    io_buf_len = 1024l;
#else
	    io_buf_len = (gulong)lstat_buf.st_blksize;
#endif

	    /* Get the target file's statistics */
	    if(!fstat((int)tar_fd, &tar_stat_buf))
	    {
		/* Need to use a smaller I/O buffer? */
#if !defined(_WIN32)
		if((gulong)tar_stat_buf.st_blksize < io_buf_len)
		    io_buf_len = (gulong)tar_stat_buf.st_blksize;
#endif
	    }
	    if(io_buf_len < 1l)
		io_buf_len = 1l;

	    /* Set the target file's properties to match the source
	     * file's properties
	     */
	    (void)fchmod(
		(int)tar_fd,
		m
	    );
	    (void)fchown(
		(int)tar_fd,
		lstat_buf.st_uid,
		lstat_buf.st_gid
	    );

	    coppied_size = 0l;

	    /* Allocate the I/O buffer */
	    io_buf = (guint8 *)g_malloc(io_buf_len);
	    if(io_buf != NULL)
	    {
		gint units_read;

		/* Copy */
		while(!feof(src_fp))
		{
		    /* Read */
		    units_read = (gint)fread(
			io_buf,
			sizeof(guint8),
			(size_t)(io_buf_len / sizeof(guint8)),
			src_fp
		    );
		    if(units_read < 0)
		    {
			const gint error_code = (gint)errno;
			fclose(src_fp);
			fclose(tar_fp);
			errno = (int)error_code;
			return(-1);
		    }
		    else if(units_read == 0)
		    {
			break;
		    }

		    /* More than the requested number of bytes read? */
		    if(units_read > (gint)(io_buf_len / sizeof(guint8)))
			break;

		    /* Write */
		    if(fwrite(
			io_buf,
			sizeof(guint8),
			(size_t)units_read,
			tar_fp
		    ) != (size_t)units_read)
		    {
			const gint error_code = (gint)errno;
			fclose(src_fp);
			fclose(tar_fp);
			errno = (int)error_code;
			return(-1);
		    }

		    /* Count the number of bytes read */
		    coppied_size += (gulong)(units_read * sizeof(guint8));
		}

		/* Delete the I/O buffer */
		g_free(io_buf);
	    }
	    else
	    {
		const gint error_code = (gint)errno;
		fclose(src_fp);
		fclose(tar_fp);
		errno = (int)error_code;
		return(-3);
	    }

	    /* Close the source and target files */
	    fclose(src_fp);
	    fclose(tar_fp);

#if !defined(_WIN32)
	    /* Set the target file's time stamps */
	    utime_buf.actime = lstat_buf.st_atime;
	    utime_buf.modtime = lstat_buf.st_mtime;
	    (void)utime(
		(const char *)new_path,
		&utime_buf
	    );
#endif

	    /* Did not copy the entire file? */
	    if(coppied_size < file_size)
	    {
		errno = EIO;
		return(-1);
	    }

	    return(0);
	}
	/* Other */
	else
	{
	    /* Not supported */
	    errno = EINVAL;
	    return(-2);
	}
}

/*
 *	Move.
 *
 *	The paths_list specifies a GList of gchar * paths to the
 *	objects being moved.
 *
 *	The tar_path specifies the full path to the target location.
 *	The target location must be a directory.
 */
static void FileBrowserMove(
	FileBrowser *fb,
	GList *paths_list,
	const gchar *tar_path
)
{
	gint i, status, response, nobjs_moved;
	const gchar *full_path, *old_path;
	gchar *msg, *new_path;
	GList *glist;
	GtkWidget *toplevel;

	if((fb == NULL) || (paths_list == NULL) || STRISEMPTY(tar_path) ||
	   CDialogIsQuery()
	)
	    return;

#define MESSAGE_MOVE_FAILED(s)	{		\
 CDialogSetTransientFor(toplevel);		\
 CDialogGetResponse(				\
  "Move Failed",				\
  (s), NULL,					\
  CDIALOG_ICON_WARNING,				\
  CDIALOG_BTNFLAG_OK, CDIALOG_BTNFLAG_OK	\
 );						\
 CDialogSetTransientFor(NULL);			\
}

	FileBrowserSetBusy(fb, TRUE);

	toplevel = fb->toplevel;

	/* Format the confirmation message, if only one object is
	 * selected then confirm with its name, otherwise confirm
	 * the number of objects to be moved
	 */
	glist = paths_list;
	i = g_list_length(glist);
	if(i == 1)
	{
	    full_path = (const gchar *)glist->data;
	    msg = g_strdup_printf(
"Move:\n\
\n\
    %s\n\
\n\
To:\n\
\n\
    %s",
		full_path,
		tar_path
	    );
	}
	else
	{
	    msg = g_strdup_printf(
"Move %i objects to:\n\
\n\
    %s",
		i,
		tar_path
	    );
	}

	/* Confirm move */
	CDialogSetTransientFor(toplevel);
	response = CDialogGetResponse(
	    "Confirm Move",
	    msg,
	    NULL,
	    CDIALOG_ICON_FILE_MOVE,
	    CDIALOG_BTNFLAG_YES | CDIALOG_BTNFLAG_NO,
	    CDIALOG_BTNFLAG_YES
	);
	g_free(msg);
	CDialogSetTransientFor(NULL);
	if(response != CDIALOG_RESPONSE_YES)
	{
	    FileBrowserSetBusy(fb, FALSE);
	    return;
	}

	/* Move the objects */
	for(glist = paths_list, nobjs_moved = 0;
	    glist != NULL;
	    glist = g_list_next(glist)
	)
	{
	    old_path = (const gchar *)glist->data;
	    if(STRISEMPTY(old_path))
		continue;

	    new_path = STRDUP(PrefixPaths(
		(const gchar *)tar_path,
		(const gchar *)g_basename(old_path)
	    ));
	    if(new_path == NULL)
		continue;

	    /* Move */
	    status = FileBrowserMoveIteration(
		fb,
		old_path,
		new_path
	    );
	    if(status != 0)
	    {
		const gint error_code = (gint)errno;
		gchar *msg = g_strdup_printf(
		    "%s.",
		    g_strerror(error_code)
		);
		MESSAGE_MOVE_FAILED(msg);
		g_free(msg);
	    }
	    else
	    {
		nobjs_moved++;

		/* Notify about the object being moved */
		if(fb->object_created_cb != NULL)
		    fb->object_created_cb(
			new_path,
			fb->object_created_data
		    );
		if(fb->object_deleted_cb != NULL)
		    fb->object_deleted_cb(
			old_path,
			fb->object_deleted_data
		    );
	    }

	    g_free(new_path);
	}

	/* Refresh the list after the move */
	FileBrowserRefresh(fb);

	FileBrowserSetBusy(fb, FALSE);

#undef MESSAGE_MOVE_FAILED
}

/*
 *	Copy.
 */
static void FileBrowserCopy(
	FileBrowser *fb,
	GList *paths_list,
	const gchar *tar_path
)
{
	gint i, status, response, nobjs_coppied;
	const gchar *full_path, *old_path;
	gchar *msg, *new_path;
	GList *glist;
	GtkWidget *toplevel;

	if((fb == NULL) || (paths_list == NULL) || STRISEMPTY(tar_path) ||
	   CDialogIsQuery()
	)
	    return;

#define MESSAGE_COPY_FAILED(s)	{		\
 CDialogSetTransientFor(toplevel);		\
 CDialogGetResponse(				\
  "Copy Failed",				\
  (s), NULL,					\
  CDIALOG_ICON_WARNING,				\
  CDIALOG_BTNFLAG_OK, CDIALOG_BTNFLAG_OK	\
 );						\
 CDialogSetTransientFor(NULL);			\
}

	FileBrowserSetBusy(fb, TRUE);

	toplevel = fb->toplevel;

	/* Format the confirmation message, if only one object is
	 * selected then confirm with its name, otherwise confirm
	 * the number of objects to be coppied
	 */
	glist = paths_list;
	i = g_list_length(glist);
	if(i == 1)
	{
	    full_path = (const gchar *)glist->data;
	    msg = g_strdup_printf(
"Copy:\n\
\n\
    %s\n\
\n\
To:\n\
\n\
    %s",
		full_path,
		tar_path
	    );
	}
	else
	{
	    msg = g_strdup_printf(
"Copy %i objects to:\n\
\n\
    %s",
		i,
		tar_path
	    );
	}

	/* Confirm copy */
	CDialogSetTransientFor(toplevel);
	response = CDialogGetResponse(
	    "Confirm Copy",
	    msg,
	    NULL,
	    CDIALOG_ICON_FILE_COPY,
	    CDIALOG_BTNFLAG_YES | CDIALOG_BTNFLAG_NO,
	    CDIALOG_BTNFLAG_YES
	);
	g_free(msg);
	CDialogSetTransientFor(NULL);
	if(response != CDIALOG_RESPONSE_YES)
	{
	    FileBrowserSetBusy(fb, FALSE);
	    return;
	}

	/* Copy the objects */
	for(glist = paths_list, nobjs_coppied = 0;
	    glist != NULL;
	    glist = g_list_next(glist)
	)
	{
	    old_path = (const gchar *)glist->data;
	    if(STRISEMPTY(old_path))
		continue;

	    new_path = STRDUP(PrefixPaths(
		(const gchar *)tar_path,
		(const gchar *)g_basename(old_path)
	    ));
	    if(new_path == NULL)
		continue;

	    /* Copy */
	    status = FileBrowserCopyIteration(
		fb,
		old_path,
		new_path
	    );
	    if(status != 0)
	    {
		const gint error_code = (gint)errno;
		gchar *msg = g_strdup_printf(
		    "%s.",
		    g_strerror(error_code)
		);
		MESSAGE_COPY_FAILED(msg);
		g_free(msg);
	    }
	    else
	    {
		nobjs_coppied++;

		/* Notify about the object being coppied */
		if(fb->object_created_cb != NULL)
		    fb->object_created_cb(
			new_path,
			fb->object_created_data
		    );
	    }

	    g_free(new_path);
	}

	/* Refresh the list after the copy */
	FileBrowserRefresh(fb);

	FileBrowserSetBusy(fb, FALSE);

#undef MESSAGE_COPY_FAILED
}

/*
 *	Link.
 */
static void FileBrowserLink(
	FileBrowser *fb,
	const gchar *src_path,
	const gchar *tar_path
)
{
	struct stat lstat_buf;
	gint response;
	gchar *msg, *new_path;
	GtkWidget *toplevel;

	if((fb == NULL) || STRISEMPTY(src_path) || STRISEMPTY(tar_path) ||
	   CDialogIsQuery()
	)
	    return;

#define MESSAGE_LINK_FAILED(s)	{		\
 CDialogSetTransientFor(toplevel);		\
 CDialogGetResponse(				\
  "Link Failed",				\
  (s), NULL,					\
  CDIALOG_ICON_WARNING,				\
  CDIALOG_BTNFLAG_OK, CDIALOG_BTNFLAG_OK	\
 );						\
 CDialogSetTransientFor(NULL);			\
}

	FileBrowserSetBusy(fb, TRUE);

	toplevel = fb->toplevel;

	/* Format the confirmation message */
	msg = g_strdup_printf(
"Link:\n\
\n\
    %s\n\
\n\
To:\n\
\n\
    %s",
	    src_path,
	    tar_path
	);

	/* Confirm link */
	CDialogSetTransientFor(toplevel);
	response = CDialogGetResponse(
	    "Confirm Link",
	    msg,
	    NULL,
	    CDIALOG_ICON_LINK,
	    CDIALOG_BTNFLAG_YES | CDIALOG_BTNFLAG_NO,
	    CDIALOG_BTNFLAG_YES
	);
	g_free(msg);
	CDialogSetTransientFor(NULL);
	if(response != CDIALOG_RESPONSE_YES)
	{
	    FileBrowserSetBusy(fb, FALSE);
	    return;
	}

	/* Format the path to the new link */
	new_path = STRDUP(PrefixPaths(
	    (const gchar *)tar_path,
	    (const gchar *)g_basename(src_path)
	));
	if(new_path == NULL)
	{
	    FileBrowserSetBusy(fb, FALSE);
	    return;
	}

	/* Already exists? */
#if defined(_WIN32)
	if(!stat((const char *)new_path, &lstat_buf))
#else
	if(!lstat((const char *)new_path, &lstat_buf))
#endif
	{
	    const gint error_code = EEXIST;
	    gchar *msg = g_strdup_printf(
		"%s.",
		g_strerror(error_code)
	    );
	    MESSAGE_LINK_FAILED(msg);
	    g_free(msg);

	    g_free(new_path);
	    FileBrowserSetBusy(fb, FALSE);
	    return;
	}

	/* Create the link */
	if(symlink(
	    (const char *)src_path,
	    (const char *)new_path
	))
	{
	    const gint error_code = (gint)errno;
	    gchar *msg = g_strdup_printf(
		"%s.",
		g_strerror(error_code)
	    );
	    MESSAGE_LINK_FAILED(msg);
	    g_free(msg);
	}
	else
	{
	    /* Notify about the new link being created */
	    if(fb->object_created_cb != NULL)
		fb->object_created_cb(
		    new_path,
		    fb->object_created_data
		);
	}

	/* Delete the path to the new link */
	g_free(new_path);

	/* Refresh the list after the link */
	FileBrowserRefresh(fb);

	FileBrowserSetBusy(fb, FALSE);

#undef MESSAGE_LINK_FAILED
}


/*
 *	Used by FileBrowserRenameQuery() as the apply callback for
 *	the floating prompt.
 */
static void FileBrowserRenameFPromptCB(gpointer data, const gchar *value)
{
	gint i;
	GList *glist;
	GtkWidget *toplevel;
	FileBrowserObject *o;
	FileBrowser *fb = FILE_BROWSER(data);
	if((fb == NULL) || STRISEMPTY(value))
	    return;

#if defined(PROG_LANGUAGE_SPANISH)
# define TITLE_RENAME_FAILED     "Reagrupe Fallado"
#elif defined(PROG_LANGUAGE_FRENCH)
# define TITLE_RENAME_FAILED     "Renommer Echoué"
#elif defined(PROG_LANGUAGE_GERMAN)
# define TITLE_RENAME_FAILED     "Benennen Sie Versagt Um"
#elif defined(PROG_LANGUAGE_ITALIAN)
# define TITLE_RENAME_FAILED     "Rinominare Fallito"
#elif defined(PROG_LANGUAGE_DUTCH)
# define TITLE_RENAME_FAILED     "Herdoop Geverzuimenene"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
# define TITLE_RENAME_FAILED     "O Rename Fracassou"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
# define TITLE_RENAME_FAILED     "Ombenevn Failed"
#else
# define TITLE_RENAME_FAILED     "Rename Failed"
#endif

#define MESSAGE_RENAME_FAILED(s)	{	\
 CDialogSetTransientFor(toplevel);		\
 CDialogGetResponse(				\
  TITLE_RENAME_FAILED,				\
  (s), NULL,					\
  CDIALOG_ICON_WARNING,				\
  CDIALOG_BTNFLAG_OK,				\
  CDIALOG_BTNFLAG_OK				\
 );						\
 CDialogSetTransientFor(NULL);			\
}

	FileBrowserSetBusy(fb, TRUE);

	toplevel = fb->toplevel;

	/* No directory deliminators may exist in the new value */
	if(strchr((const char *)value, G_DIR_SEPARATOR) != NULL)
	{
	    gchar *msg = g_strdup_printf(
#if defined(PROG_LANGUAGE_SPANISH)
"El nombre nuevo \"%s\" contiene deliminators de\n\
guía de '%c' que no se permiten en un nombre de objeto."
#elif defined(PROG_LANGUAGE_FRENCH)
"Le nouveau \"%s\" de nom contient deliminators d'annuaire\n\
de '%c' qui ne sont pas permis dans un nom de l'objet."
#elif defined(PROG_LANGUAGE_GERMAN)
"Der neu \"%s\" enthält '%c' Verzeichnis deliminators, das\n\
im Namen eines Objekts nicht erlaubt wird."
#elif defined(PROG_LANGUAGE_ITALIAN)
"Lo \"%s\" di nome nuovo contiene il deliminators\n\
di elenco di '%c' che non sono lasciati in un nome dell'oggetto."
#elif defined(PROG_LANGUAGE_DUTCH)
"De nieuwe naam \"%s\" bevat '%c' gids deliminators,\n\
die welk in de naam van een voorwerp niet toegestaan is."
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"O novo \"%s\" de nome contem deliminators de guia\n\
de '%c' que nao são permitidos num nome do objeto."
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Den nye navne \"%s\" inneholder '%c' katalog deliminators\n\
som ikke tillater i et objekts navn."
#else
"The new name \"%s\" contains '%c' directory\n\
deliminators which are not allowed in an object's name"
#endif
		,
		value, G_DIR_SEPARATOR
	    );
	    MESSAGE_RENAME_FAILED(msg);
	    g_free(msg);
	    FileBrowserSetBusy(fb, FALSE);
	    return;
	}

	/* Get the last selected object */
	glist = fb->selection_end;
	i = (glist != NULL) ? (gint)glist->data : -1;
	o = FileBrowserGetObject(fb, i);
	if((o != NULL) ?
	    (o->name != NULL) && (o->full_path != NULL) : FALSE
	)
	{
	    struct stat lstat_buf;
	    const gchar *cur_location = FileBrowserGetLocation(fb);
	    gchar	*old_full_path = STRDUP(o->full_path),
			*new_full_path = STRDUP(PrefixPaths(
			    (const char *)cur_location,
			    (const char *)value
			));

	    /* New name and old name the same? */
	    if(!strcmp((const char *)o->name, (const char *)value))
	    {

	    }
	    /* Make sure that the new name does not refer to an
	     * existing object
	     */
#if defined(_WIN32)
	    else if(!stat((const char *)new_full_path, &lstat_buf))
#else
	    else if(!lstat((const char *)new_full_path, &lstat_buf))
#endif
	    {
		gchar *msg = g_strdup_printf(
"An object by the name of \"%s\" already exists.",
		    value
		);
		MESSAGE_RENAME_FAILED(msg);
		g_free(msg);
	    }
	    /* Rename */
	    else if(rename(
		(const char *)old_full_path,
		(const char *)new_full_path
	    ))
	    {
		const gint error_code = (gint)errno;
		gchar *msg = g_strdup_printf(
		    "%s.",
		    g_strerror(error_code)
		);
		MESSAGE_RENAME_FAILED(msg);
		g_free(msg);
	    }
	    else
	    {
		/* Rename successful */

		/* Update the list */
		FileBrowserListUpdate(fb, NULL);

		/* Reselect the object */
		for(glist = fb->objects_list, i = 0;
		    glist != NULL;
		    glist = g_list_next(glist), i++
		)
		{
		    o = FILE_BROWSER_OBJECT(glist->data);
		    if(o == NULL)
			continue;

		    if(o->full_path == NULL)
			continue;

		    if(!strcmp(
			(const char *)o->full_path,
			(const char *)new_full_path
		    ))
		    {
			/* Select this object */
			fb->focus_object = i;
			fb->selection = g_list_append(
			    fb->selection,
			    (gpointer)i
			);
			fb->selection_end = g_list_last(fb->selection);

			/* Scroll to this object */
			FileBrowserListMoveToObject(fb, i, 0.5f);

			break;
		    }
		}
		FileBrowserListQueueDraw(fb);

		/* Update the File Name GtkEntry's value */
		FileBrowserEntrySetSelectedObjects(fb);

		/* Notify about the rename */
		if(fb->object_modified_cb != NULL)
		    fb->object_modified_cb(
			old_full_path,
			new_full_path,
			fb->object_modified_data
		    );
	    }

	    g_free(old_full_path);
	    g_free(new_full_path);
	}

	FileBrowserSetBusy(fb, FALSE);

#undef MESSAGE_RENAME_FAILED
#undef TITLE_RENAME_FAILED
}

/*
 *	Rename.
 */
static void FileBrowserRenameQuery(FileBrowser *fb)
{
	gint i, x, y, width, height;
	GList *glist;
	GtkWidget *toplevel;
	FileBrowserObject *o;

	if((fb == NULL) || FPromptIsQuery())
	    return;

	toplevel = fb->toplevel;

	/* Get the last selected object */
	glist = fb->selection_end;
	i = (glist != NULL) ? (gint)glist->data : -1;
	o = FileBrowserGetObject(fb, i);
	if(o == NULL)
	    return;

	/* Calculate the fprompt's position and size */
	x = o->x;
	y = o->y;
	width = MAX(o->width + 4, 100);
	height = -1;
	switch(fb->list_format)
	{
	  case FB_LIST_FORMAT_VERTICAL_DETAILS:
	  case FB_LIST_FORMAT_VERTICAL:
	    if(fb->list_vsb != NULL)
	    {
		y -= GTK_ADJUSTMENT_GET_VALUE(
		    GTK_RANGE(fb->list_vsb)->adjustment
		);
	    }
	    break;
	  case FB_LIST_FORMAT_STANDARD:
	    if(fb->list_hsb != NULL)
	    {
		x -= GTK_ADJUSTMENT_GET_VALUE(
		    GTK_RANGE(fb->list_hsb)->adjustment
		);
	    }
	    break;
	}

	/* Get root window relative coordinates of the object for
	 * placement of the floating prompt.
	 */
	if(fb->list_da != NULL)
	{
	    GtkWidget *w = fb->list_da;
	    GdkWindow *window = w->window;
	    if(window != NULL)
	    {
		gint rx, ry;
		gdk_window_get_deskrelative_origin(
		    window, &rx, &ry
		);
		x += rx;
		y += ry;
	    }
	}
	/* At this point x and y should now be the root window
	 * relative position for the floating prompt
	 *
	 * Set the fprompt's position and size, then map it
	 */
	FPromptSetTransientFor(toplevel);
	FPromptSetPosition(x - 2, y - 2);
	FPromptMapQuery(
	    NULL,				/* No label */
	    o->name,
	    NULL,				/* No tooltip */
	    FPROMPT_MAP_NO_MOVE,		/* Map code */
	    width, height,			/* Width and height */
	    0,					/* No buttons */
	    fb,					/* Data */
	    NULL,				/* No browse callback */
	    FileBrowserRenameFPromptCB,
	    NULL				/* No cancel callback */
	);
}

/*
 *	Change permissions fprompt callback.
 */
static void FileBrowserCHModFPromptCB(gpointer data, const gchar *value)
{
	gint i;
	GList *glist;
	GtkWidget *toplevel;
	FileBrowserObject *o;
	FileBrowser *fb = FILE_BROWSER(data);
	if((fb == NULL) || STRISEMPTY(value))
	    return;

#if defined(PROG_LANGUAGE_SPANISH)
# define TITLE_CHMOD_FAILED     "Cambie El Modo Fallado"
#elif defined(PROG_LANGUAGE_FRENCH)
# define TITLE_CHMOD_FAILED     "Changer Le Mode A échoué"
#elif defined(PROG_LANGUAGE_GERMAN)
# define TITLE_CHMOD_FAILED     "Ändern Sie Versagten Modus"
#elif defined(PROG_LANGUAGE_ITALIAN)
# define TITLE_CHMOD_FAILED     "Cambiare Il Modo Fallito"
#elif defined(PROG_LANGUAGE_DUTCH)
# define TITLE_CHMOD_FAILED     "Verandeer Modus Verzuimde"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
# define TITLE_CHMOD_FAILED     "Mude Modo Fracassado"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
# define TITLE_CHMOD_FAILED     "Forandr Sviktet Modus"
#else
# define TITLE_CHMOD_FAILED     "Change Permissions Failed"
#endif

#define MESSAGE_CHMOD_FAILED(s)		{	\
 CDialogSetTransientFor(toplevel);		\
 CDialogGetResponse(				\
  TITLE_CHMOD_FAILED,				\
  (s), NULL,					\
  CDIALOG_ICON_WARNING,				\
  CDIALOG_BTNFLAG_OK, CDIALOG_BTNFLAG_OK	\
 );						\
 CDialogSetTransientFor(NULL);			\
}

	toplevel = fb->toplevel;

	/* Get the last selected object */
	glist = fb->selection_end;
	i = (glist != NULL) ? (gint)glist->data : -1;
	o = FileBrowserGetObject(fb, i);
	if((o != NULL) ? (o->full_path != NULL) : FALSE)
	{
#if defined(S_IRUSR) && defined(S_IWUSR) && defined(S_IXUSR)
	    mode_t m = 0;
	    const gchar	*s = value,
			*path = o->full_path;

	    /* Begin parsing the value string to obtain the mode value */
	    while(ISBLANK(*s))
		s++;

	    /* User */
	    /* Read */
	    if(*s != '\0')
	    {
		if(tolower(*s) == 'r')
		    m |= S_IRUSR;
		s++;
	    }
	    /* Write */
	    if(*s != '\0')
	    {
		if(tolower(*s) == 'w')
		    m |= S_IWUSR;
		s++;
	    }
	    /* Execute */
	    if(*s != '\0')
	    {
		if(tolower(*s) == 'x')
		    m |= S_IXUSR;
		else if(tolower(*s) == 's')
		    m |= S_ISUID;
		s++;
	    }
	    /* Group */
	    /* Read */
	    if(*s != '\0')
	    {
		if(tolower(*s) == 'r')
		    m |= S_IRGRP;
		s++;
	    }
	    /* Write */
	    if(*s != '\0')
	    {
		if(tolower(*s) == 'w')
		    m |= S_IWGRP;
		s++;
	    }
	    /* Execute */
	    if(*s != '\0')
	    {
		if(tolower(*s) == 'x')
		    m |= S_IXGRP;
		else if(tolower(*s) == 'g')
		    m |= S_ISGID;
		s++;
	    }
	    /* Other */
	    /* Read */
	    if(*s != '\0')
	    {
		if(tolower(*s) == 'r')
		    m |= S_IROTH;
		s++;
	    }
	    /* Write */
	    if(*s != '\0')
	    {
		if(tolower(*s) == 'w')
		    m |= S_IWOTH;
		s++;
	    }
	    /* Execute */
	    if(*s != '\0')
	    {
		if(tolower(*s) == 'x')
		    m |= S_IXOTH;
		else if(tolower(*s) == 't')
		    m |= S_ISVTX;
		s++;
	    }

	    /* CHMod */
	    if(chmod(
		(const char *)path,
		m
	    ))
	    {
		const gint error_code = (gint)errno;
		gchar *s = g_strdup_printf(
		    "%s.",
		    g_strerror(error_code)
		);
		MESSAGE_CHMOD_FAILED(s);
		g_free(s);
	    }
	    else
	    {
		/* Statistics */
#if defined(_WIN32)
		if(stat((const char *)path, &o->lstat_buf))
#else
		if(lstat((const char *)path, &o->lstat_buf))
#endif
		    memset(&o->lstat_buf, 0x00, sizeof(struct stat));

		/* Update values */
		FileBrowserObjectUpdateValues(fb, o);
	    }
#endif
	}

	/* Need to redraw and update the value in the entry
	 * since one of the selected object's names may have
	 * changed
	 */
	FileBrowserListQueueDraw(fb);
	FileBrowserEntrySetSelectedObjects(fb);

#undef MESSAGE_RENAME_FAILED
#undef TITLE_RENAME_FAILED
}

/*
 *	Change permissions.
 */
static void FileBrowserCHModQuery(FileBrowser *fb)
{
	mode_t m;
	gint		i,
			x, y,
			width, height;
	gchar *value;
	GList *glist;
	GtkWidget *toplevel;
	FileBrowserObject *o;
	FileBrowserColumn *column, *column2;
	
	if((fb == NULL) || FPromptIsQuery())
	    return;

	toplevel = fb->toplevel;

	/* Get the last selected object */
	glist = fb->selection_end;
	i = (glist != NULL) ? (gint)glist->data : -1;
	o = FileBrowserGetObject(fb, i);
	if(o == NULL)
	    return;

	m = o->lstat_buf.st_mode;

#ifdef S_ISLNK
	/* Cannot chmod links */
	if(S_ISLNK(m))
	{
	    CDialogSetTransientFor(toplevel);
	    CDialogGetResponse(
"Change Permissions Failed",
"Unable to change the permissions of links",
		NULL,
		CDIALOG_ICON_WARNING,
		CDIALOG_BTNFLAG_OK,
		CDIALOG_BTNFLAG_OK
	    );
	    CDialogSetTransientFor(NULL);
	    return;
	}
#endif

#if !defined(S_IRUSR) || !defined(S_IWUSR) || !defined(S_IXUSR)
	/* Since permissions not supported we cannot chmod */
	if(TRUE)
	{
	    CDialogSetTransientFor(toplevel);
	    CDialogGetResponse(
"Change Permissions Failed",
"Changing the permissions of objects is not supported on this system.",
		NULL,
		CDIALOG_ICON_WARNING,
		CDIALOG_BTNFLAG_OK,
		CDIALOG_BTNFLAG_OK
	    );
	    CDialogSetTransientFor(NULL);
	    return;
	}
#else
	/* Format current value */
	value = g_strdup_printf(
	    "%c%c%c%c%c%c%c%c%c",
	    (m & S_IRUSR) ? 'r' : '-',
	    (m & S_IWUSR) ? 'w' : '-',
	    (m & S_ISUID) ? 'S' :
		((m & S_IXUSR) ? 'x' : '-'),
	    (m & S_IRGRP) ? 'r' : '-',
	    (m & S_IWGRP) ? 'w' : '-',
	    (m & S_ISGID) ? 'G' :
		((m & S_IXGRP) ? 'x' : '-'),
	    (m & S_IROTH) ? 'r' : '-',
	    (m & S_IWOTH) ? 'w' : '-',
	    (m & S_ISVTX) ? 'T' :
		((m & S_IXOTH) ? 'x' : '-')
	);
#endif

	/* Calculate the fprompt's position and size */
	x = o->x;
	y = o->y;
	width = MAX(o->width + 4, 80);
	height = -1;
	switch(fb->list_format)
	{
	  case FB_LIST_FORMAT_VERTICAL_DETAILS:
	  case FB_LIST_FORMAT_VERTICAL:
	    if(fb->list_vsb != NULL)
	    {
		y -= GTK_ADJUSTMENT_GET_VALUE(
		    GTK_RANGE(fb->list_vsb)->adjustment
		);
	    }
	    column = FileBrowserListGetColumn(fb, 1);
	    if(column != NULL)
		x += column->position;
	    column2 = FileBrowserListGetColumn(fb, 2);
	    if((column != NULL) && (column2 != NULL))
		width = MAX(column2->position - column->position + 4, 80);
	    break;

	  case FB_LIST_FORMAT_STANDARD:
	    if(fb->list_hsb != NULL)
	    {
		x += GTK_ADJUSTMENT_GET_VALUE(
		    GTK_RANGE(fb->list_hsb)->adjustment
		);
	    }
	    break;
	}
	/* Get root window relative coordinates of the object for
	 * placement of the floating prompt
	 */
	if(fb->list_da != NULL)
	{
	    GtkWidget *w = fb->list_da;
	    GdkWindow *window = w->window;
	    if(window != NULL)
	    {
		gint rx, ry;
		gdk_window_get_deskrelative_origin(
		    window,
		    &rx, &ry
		);
		x += rx;
		y += ry;
	    }
	}
	/* At this point x and y should now be the root window
	 * relative position for the floating prompt
	 */
	FPromptSetTransientFor(toplevel);
	FPromptSetPosition(x - 2, y - 2);
	FPromptMapQuery(
	    NULL,			/* No label */
	    value,
	    NULL,			/* No tooltip */
	    FPROMPT_MAP_NO_MOVE,	/* Map code */
	    width, height,		/* Width and height */
	    0,				/* No buttons */
	    fb,				/* Data */
	    NULL,			/* No browse callback */
	    FileBrowserCHModFPromptCB,
	    NULL                        /* No cancel callback */
	);

	g_free(value);
}

/*
 *	Deletes the object.
 *
 *	The path specifies the full path to the object. If the object
 *	is a directory then it and all of its child objects will be
 *	deleted.
 */
static gint FileBrowserDeleteIteration(
	FileBrowser *fb,
	const gchar *path
)
{
	/* Is the object a directory? */
	if(ISLPATHDIR(path))
	{
	    gint status = 0;

	    /* Get the list of objects in this directory */
	    gchar **names_list = GetDirEntNames(path);
	    if(names_list != NULL)
	    {
		gint i;
		gchar *name, *child_path;

		/* Delete all the objects in this directory */
		for(i = 0; names_list[i] != NULL; i++)
		{
		    name = names_list[i];
		    if(!strcmp((const char *)name, ".") ||
		       !strcmp((const char *)name, "..")
		    )
		    {
			g_free(name);
			continue;
		    }

		    /* Get the full path to this child object */
		    child_path = STRDUP(PrefixPaths(
			(const char *)path,
			(const char *)name
		    ));
		    if(child_path != NULL)
		    {
			/* Delete this child object */
			const gint status2 = FileBrowserDeleteIteration(
			    fb,
			    child_path
			);
			if(status2 != 0)
			{
			    if(status == 0)
				status = status2;
			}

			g_free(child_path);
		    }

		    g_free(name);
		}

		g_free(names_list);
	    }

	    /* Failed to delete a child object? */
	    if(status != 0)
		return(status);

	    /* Delete the directory */
	    if(rmdir((const char *)path))
		return(-1);
	}
	else
	{
	    /* Delete the object */
	    if(unlink((const char *)path))
		return(-1);
	}
	return(0);
}

/*
 *	Delete.
 */
static void FileBrowserDelete(FileBrowser *fb)
{
	gint i, response, nobjs_deleted;
	gchar *msg;
	GList *glist;
	GtkWidget *toplevel;
	FileBrowserObject *o;

	if((fb == NULL) || CDialogIsQuery())
	    return;

#if defined(PROG_LANGUAGE_SPANISH)
# define TITLE_DELETE_FAILED	"Borre Fallado"
#elif defined(PROG_LANGUAGE_FRENCH)
# define TITLE_DELETE_FAILED	"Effacer Echoué"
#elif defined(PROG_LANGUAGE_GERMAN)
# define TITLE_DELETE_FAILED	"Löschen Sie Versagt"
#elif defined(PROG_LANGUAGE_ITALIAN)
# define TITLE_DELETE_FAILED	"Cancellare Fallito"
#elif defined(PROG_LANGUAGE_DUTCH)
# define TITLE_DELETE_FAILED	"Schrap Geverzuimenene"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
# define TITLE_DELETE_FAILED	"Anule Fracassado"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
# define TITLE_DELETE_FAILED	"Stryk Failed"
#else
# define TITLE_DELETE_FAILED	"Delete Failed"
#endif

#define MESSAGE_DELETE_FAILED(s)	{	\
 CDialogSetTransientFor(toplevel);		\
 CDialogGetResponse(				\
  TITLE_DELETE_FAILED,				\
  (s), NULL,					\
  CDIALOG_ICON_WARNING,				\
  CDIALOG_BTNFLAG_OK, CDIALOG_BTNFLAG_OK	\
 );						\
 CDialogSetTransientFor(NULL);			\
}

	toplevel = fb->toplevel;

	/* Get the selections list */
	glist = fb->selection;
	if(glist == NULL)
	    return;

	FileBrowserSetBusy(fb, TRUE);

	/* Format confirmation message, if only one object is selected
	 * then confirm with its name. Otherwise confirm the number of
	 * objects to be deleted.
	 */
	i = g_list_length(glist);
	if(i == 1)
	{
	    i = (gint)glist->data;
	    o = FileBrowserGetObject(fb, i);
	    if((o != NULL) ? (o->full_path != NULL) : FALSE)
	    {
		if(ISLPATHDIR(o->full_path))
		    msg = g_strdup_printf(
#if defined(PROG_LANGUAGE_SPANISH)
"¿Usted está seguro que usted quiere borrar \"%s\"?"
#elif defined(PROG_LANGUAGE_FRENCH)
"Etes-vous sûr que vous voulez effacer \"%s\"?"
#elif defined(PROG_LANGUAGE_GERMAN)
"Sind sie sicher sie \"%s\" wollen löschen?"
#elif defined(PROG_LANGUAGE_ITALIAN)
"Lei sono sicuro che lei vuole cancellare \"%s\"?"
#elif defined(PROG_LANGUAGE_DUTCH)
"Bent u zeker u \"%s\" wil schrappen?"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"Estão seguro quer anular \"%s\"?"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Er De sikker De stryker \"%s\"?"
#else
"Delete directory \"%s\"?\n\
\n\
Including any contents within that directory."
#endif
			,
			o->name
		    );
		else
		    msg = g_strdup_printf(
#if defined(PROG_LANGUAGE_SPANISH)
"¿Usted está seguro que usted quiere borrar \"%s\"?"
#elif defined(PROG_LANGUAGE_FRENCH)
"Etes-vous sûr que vous voulez effacer \"%s\"?"
#elif defined(PROG_LANGUAGE_GERMAN)
"Sind sie sicher sie \"%s\" wollen löschen?"
#elif defined(PROG_LANGUAGE_ITALIAN)
"Lei sono sicuro che lei vuole cancellare \"%s\"?"
#elif defined(PROG_LANGUAGE_DUTCH)
"Bent u zeker u \"%s\" wil schrappen?"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"Estão seguro quer anular \"%s\"?"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Er De sikker De stryker \"%s\"?"
#else
"Delete \"%s\"?"
#endif
			,
			o->name
		    );
	    }
	    else
		msg = STRDUP(
"Delete (unnamed) object?"
		);
	}
	else
	{
	    msg = g_strdup_printf(
#if defined(PROG_LANGUAGE_SPANISH)
"¿Borra %i objetos escogidos?"
#elif defined(PROG_LANGUAGE_FRENCH)
"Efface %i objets choisis?"
#elif defined(PROG_LANGUAGE_GERMAN)
"Löschen Sie %i ausgewählte Objekte?"
#elif defined(PROG_LANGUAGE_ITALIAN)
"Cancella %i oggetti scelti?"
#elif defined(PROG_LANGUAGE_DUTCH)
"Schrap %i geselecteerde voorwerpen?"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"Anule %i objetos selecionados?"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Stryk %i valgte ut objekt?"
#else
"Delete %i selected objects?"
#endif
		, i
	    );
	}

	/* Confirm delete */
	CDialogSetTransientFor(toplevel);
	response = CDialogGetResponseIconData(
#if defined(PROG_LANGUAGE_SPANISH)
"Confirme Borre"
#elif defined(PROG_LANGUAGE_FRENCH)
"Confirmer Effacer"
#elif defined(PROG_LANGUAGE_GERMAN)
"Bestätigen Sie Löscht"
#elif defined(PROG_LANGUAGE_ITALIAN)
"Confermare Cancellare"
#elif defined(PROG_LANGUAGE_DUTCH)
"Bevestiig Schrappet"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"Confirme Anula"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Bekreft Delete"
#else
"Confirm Delete"
#endif
	    ,
	    msg,
	    NULL,
	    (guint8 **)icon_trash_32x32_xpm,
	    CDIALOG_BTNFLAG_YES | CDIALOG_BTNFLAG_NO,
	    CDIALOG_BTNFLAG_NO
	);
	g_free(msg);
	CDialogSetTransientFor(NULL);
	if(response != CDIALOG_RESPONSE_YES)
	{
	    FileBrowserSetBusy(fb, FALSE);
	    return;
	}

	/* Delete the selected objects */
	nobjs_deleted = 0;
	while(glist != NULL)
	{
	    i = (gint)glist->data;
	    o = FileBrowserGetObject(fb, i);
	    if((o != NULL) ? (o->full_path != NULL) : FALSE)
	    {
		const gchar *path = o->full_path;

		/* Delete this object */
		if(!FileBrowserDeleteIteration(fb, path))
		{
		    nobjs_deleted++;

		    /* Notify about the deleted object */
		    if(fb->object_deleted_cb != NULL)
			fb->object_deleted_cb(
			    path,
			    fb->object_deleted_data
			);
		}
	    }

	    glist = g_list_next(glist);
	}

	/* Refresh the list after the delete */
	FileBrowserRefresh(fb);

	FileBrowserSetBusy(fb, FALSE);

#undef MESSAGE_DELETE_FAILED
#undef TITLE_DELETE_FAILED
}


/*
 *	Refresh.
 */
static void FileBrowserRefresh(FileBrowser *fb)
{
	gint		last_scroll_x = 0,
			last_scroll_y = 0;
	GtkWidget *w;

	if(fb == NULL)
	    return;

	FileBrowserSetBusy(fb, TRUE);

	/* Record the last scroll positions */
	w = fb->list_hsb;
	if(w != NULL)
	    last_scroll_x = (gint)GTK_ADJUSTMENT_GET_VALUE(
		GTK_RANGE(w)->adjustment
	    );
	w = fb->list_vsb;
	if(w != NULL)
	    last_scroll_y = (gint)GTK_ADJUSTMENT_GET_VALUE(
		GTK_RANGE(w)->adjustment  
	    );


	/* Begin refreshing */

	/* Reget the Locations List */
	FileBrowserLocationsPUListUpdate(fb);

	/* Reget the Objects List */
	FileBrowserListUpdate(fb, NULL);

	/* Update the file name GtkEntry */
	FileBrowserEntrySetSelectedObjects(fb);

	/* Restore the GtkScrollBar positions */
	w = fb->list_hsb;
	if(w != NULL)
	    GTK_ADJUSTMENT_SET_VALUE(
		GTK_RANGE(w)->adjustment,
		(gfloat)last_scroll_x
	    );
	w = fb->list_vsb;
	if(w != NULL)
	    GTK_ADJUSTMENT_SET_VALUE(
		GTK_RANGE(w)->adjustment,
		(gfloat)last_scroll_y
	    );

	FileBrowserSetBusy(fb, FALSE);
}

/*
 *	File Name GtkEntry activate.
 *
 *	If the File Name GtkEntry's value is a single value specifying
 *	a directory then the current location will be set as that
 *	directory. Otherwise, in the case of multiple objects (','
 *	deliminators in the value) or an object that does not lead to
 *	a directory (including non-existant objects), FileBrowserOK()
 *	will be called.
 */
static void FileBrowserEntryActivate(FileBrowser *fb)
{
	const gchar *s;
	gchar *v;
	GtkWidget *w;

	if(fb == NULL)
	    return;

	w = fb->entry;
	if(w == NULL)
	    return;

	/* Get the file name GtkEntry's value */
	s = gtk_entry_get_text(GTK_ENTRY(w));
	if(STRISEMPTY(s))
	    return;

	v = STRDUP(s);				/* Copy the value */

	/* Filter specified? */
	if((strchr((const char *)v, '*') != NULL) ||
	   (strchr((const char *)v, '?') != NULL))
	{
	    /* Reget the listing with the newly specified filter */
	    FileBrowserListUpdate(fb, v);
	}
	/* Multiple objects specified? */
	else if(strchr((const char *)v, ',') != NULL)
	{
	    /* Multiple objects selected, perform the OK operation */
	    FileBrowserOK(fb);
	}
	/* Current directory specified? */
	else if(!strcmp((const char *)v, "."))
	{
	    /* Do nothing */
	}
	/* Parent directory specified? */
	else if(!strcmp((const char *)v, ".."))
	{
	    /* Go to the parent location */
	    FileBrowserGoToParent(fb);
	}
	/* Home directory specified? */
	else if(*v == '~')
	{
	    /* Go to the home directory */
	    FileBrowserSetLocation(fb, v);
	}
	else
	{
	    /* All else assume a single object was specified, it may
	     * or may not actually exist
	     */
	    struct stat stat_buf;
	    gchar *full_path;

	    /* Check if the object specified is specified as a full
	     * path and if it is not then prefix the current location
	     * to it
	     */
	    if(g_path_is_absolute(v))
	    {
		full_path = STRDUP(v);
	    }
	    else
	    {
		const gchar *cur_location = FileBrowserGetLocation(fb);
		full_path = STRDUP(PrefixPaths(
		    (const char *)cur_location,
		    (const char *)v
		));
	    }

	    /* Check if the object specified by full_path exists and if
	     * it leads to a directory
	     */
	    if(stat((const char *)full_path, &stat_buf))
	    {
		/* Unable to obtain the object's statistics, this
		 * object may not exist (in the case of the user
		 * wanting to save to a new file that does not exist
		 * yet), so perform the OK operation
		 */
		FileBrowserOK(fb);
	    }
	    else
	    {
		/* If the object specified by full_path is a directory
		 * then go to that directory, otherwise call the OK
		 * callback
		 */
#ifdef S_ISDIR
		if(S_ISDIR(stat_buf.st_mode))
#else
		if(FALSE)
#endif
		    FileBrowserSetLocation(fb, full_path);
		else
		    FileBrowserOK(fb);
	    }

	    g_free(full_path);
	}

	g_free(v);
}

/*
 *	OK operation.
 *
 *	Checks if any dialogs or prompts are still in query and
 *	breaks out of those queries.
 *
 *	Otherwise, the FB_RESPONSE_OK flag is set, the File Name
 *	GtkEntry value is coppied to the selected paths list, unmaps
 *	the File Browser and breaks out of any main levels.
 */
static void FileBrowserOK(FileBrowser *fb)
{
	if(fb == NULL)
	    return;

	if(FPromptIsQuery())
	{
	    FPromptBreakQuery();
	}
	else if(PUListIsQuery(PUListBoxGetPUList(fb->location_pulistbox)))
	{
	    PUListBreakQuery(PUListBoxGetPUList(fb->location_pulistbox));
	}
	else if(PUListIsQuery(PUListBoxGetPUList(fb->types_pulistbox)))
	{
	    PUListBreakQuery(PUListBoxGetPUList(fb->types_pulistbox));
	}
	else if(CDialogIsQuery())
	{

	}
	else
	{
	    gint i;
	    GtkWidget *w;

	    /* Mark that user response was OK */
	    fb->flags |= FB_RESPONSE_OK;

	    /* Delete previous list of selected paths */
	    for(i = 0; i < fb->total_selected_paths; i++)
		g_free(fb->selected_path[i]);
	    g_free(fb->selected_path);
	    fb->selected_path = NULL;
	    fb->total_selected_paths = 0;

	    /* Get value in entry and explode it to the list of
	     * return paths
	     */
	    w = fb->entry;
	    if(w != NULL)
	    {
		const gchar *s = gtk_entry_get_text(GTK_ENTRY(w));
		if(!STRISEMPTY(s))
		{
		    /* Explode the entry's string value at all ','
		     * characters and then prefix the current location
		     * to each exploded string
		     */
		    gint i, n;
		    const gchar *cur_location = FileBrowserGetLocation(fb);
		    gchar	*name,
				**names_list = g_strsplit(s, ",", -1);
		    for(i = 0; names_list[i] != NULL; i++)
		    {
			name = names_list[i];
/* Do not strip the spaces in the object's name since spaces may,
 * in fact, be part of the object's name
 */
/*			name = g_strstrip(name); */

			/* Append a new path to the selected_path
			 * array, the new path will be either name
			 * (if name is an absolute path) or the
			 * cur_location prefixed to name (if name is
			 * not an absolute path).
			 */
			n = MAX(fb->total_selected_paths, 0);
			fb->total_selected_paths = n + 1;
			fb->selected_path = (gchar **)g_realloc(
			    fb->selected_path,
			    fb->total_selected_paths * sizeof(gchar *)
			);
			if(fb->selected_path != NULL)
			{
			    /* If name is an absolute path then copy
			     * it to the selected_path list
			     *
			     * Otherwise copy the cur_location prefixed
			     * to name
			     */
			    if(g_path_is_absolute(name))
				fb->selected_path[n] = STRDUP(name);
			    else
				fb->selected_path[n] = STRDUP(PrefixPaths(
				    cur_location, name
				));
			}
			else
			{
			    fb->total_selected_paths = 0;
			}

			g_free(name);
		    }
		    g_free(names_list);
	        }
	        else
	        {
		    /* File name entry is empty, so set the current
		     * location as the return path.
		     */
		    const gchar *cur_location = FileBrowserGetLocation(fb);
		    gint n = MAX(fb->total_selected_paths, 0);
		    fb->total_selected_paths = n + 1;
		    fb->selected_path = (gchar **)g_realloc(
		        fb->selected_path,
		        fb->total_selected_paths * sizeof(gchar *)
		    );
		    if(fb->selected_path == NULL)
		    {
			fb->total_selected_paths = 0;
		    }
		    else
		    {
		        fb->selected_path[n] = STRDUP(cur_location);
		    }
	        }
	    }

	    /* Unmap */
	    FileBrowserUnmap();

	    /* Break out of blocking loop */
	    if(fb->main_level > 0)
	    {
		gtk_main_quit();
		fb->main_level--;
	    }
	}
}

/*
 *	Cancel operation.
 *
 *	Checks if any dialogs or prompts are still in query and
 *	breaks out of those queries.
 *
 *	Otherwise, the File Browser is unmapped and any main levels
 *	will be broken out of.
 */
static void FileBrowserCancel(FileBrowser *fb)
{
	if(fb == NULL)
	    return;

	if(FPromptIsQuery())
	{
	    FPromptBreakQuery();
	}
	else if(PUListIsQuery(PUListBoxGetPUList(fb->location_pulistbox)))
	{
	    PUListBreakQuery(PUListBoxGetPUList(fb->location_pulistbox));
	}
	else if(PUListIsQuery(PUListBoxGetPUList(fb->types_pulistbox)))
	{
	    PUListBreakQuery(PUListBoxGetPUList(fb->types_pulistbox));
	}
	else if(CDialogIsQuery())
	{

	}
	else
	{
	    /* Unmap */
	    FileBrowserUnmap();

	    /* Break out of blocking loop */
	    if(fb->main_level > 0)
	    {
		gtk_main_quit();
		fb->main_level--;
	    }
	}
}


/*
 *	Locations popup list item destroy callback.
 */
static void FileBrowserLocationsPUListItemDestroyCB(gpointer data)
{
#if 0
	gchar *full_path = (gchar *)data;
	if(full_path == NULL)
	    return;
#endif
	g_free(data);
}

/*
 *	Types popup list item destroy callback.
 */
static void FileBrowserTypesPUListItemDestroyCB(gpointer data)
{
	FileBrowserTypeDelete(FB_TYPE(data));
}

/*
 *	Toplevel GtkWindow "delete_event" signal callback.
 */
static gint FileBrowserDeleteEventCB(
	GtkWidget *widget, GdkEvent *event, gpointer data
)
{
	FileBrowserCancel(FILE_BROWSER(data));
	return(TRUE);
}


/*
 *	Locations popup list "changed" signal callback.
 */
static void FileBrowserLocPUListChangedCB(
	GtkWidget *widget, const gint i, gpointer data
)
{
	const gchar *location;
	GtkWidget *pulist;
	FileBrowser *fb = FILE_BROWSER(data);
	if((widget == NULL) || (fb == NULL))
	    return;

	if(fb->freeze_count > 0)
	    return;

	pulist = PUListBoxGetPUList(widget);
	if(pulist == NULL)
	    return;

	location = (const gchar *)PUListGetItemData(
	    pulist, i
	);
	if(location == NULL)
	    return;

	fb->freeze_count++;

	FileBrowserSetLocation(fb, location);

	fb->freeze_count--;
}


/*
 *	List Header GtkDrawingArea event signal callback.
 */
static gint FileBrowserListHeaderEventCB(
	GtkWidget *widget, GdkEvent *event, gpointer data
)
{
	gint status = FALSE;
	GdkEventConfigure *configure;
	GdkEventExpose *expose;
	GdkEventFocus *focus;
	GdkEventButton *button;
	GdkEventMotion *motion;
	GdkEventCrossing *crossing;
	GtkWidget *w;
	FileBrowser *fb = FILE_BROWSER(data);
	if((event == NULL) || (fb == NULL))
	    return(status);

	w = fb->list_header_da;
	if(w == NULL)
	    return(status);

	/* List widget must also be valid */
	if(fb->list_da == NULL)
	    return(status);

#define DRAW_DRAG_LINE(x)					\
{ if(column != NULL) {						\
 gint line_p = (gint)(x);					\
 GdkWindow *window1, *window2;					\
 GdkGC *gc = fb->gc;						\
								\
 window1 = w->window;						\
 window2 = fb->list_da->window;					\
								\
 if((window1 != NULL) && (window2 != NULL) && (gc != NULL)) {	\
  gint width, height;						\
								\
  gdk_gc_set_function(gc, GDK_INVERT);				\
								\
  /* Draw drag line on window1 */				\
  gdk_window_get_size(window1, &width, &height);		\
  gdk_draw_line(window1, gc, line_p, 0, line_p, height);	\
								\
  /* Draw drag line on window2 */				\
  gdk_window_get_size(window2, &width, &height);		\
  gdk_draw_line(window2, gc, line_p, 0, line_p, height);	\
								\
  gdk_gc_set_function(gc, GDK_COPY);				\
 }								\
} }

	switch((gint)event->type)
	{
	  case GDK_CONFIGURE:
	    configure = (GdkEventConfigure *)event;
	    status = TRUE;
	    break;

	  case GDK_EXPOSE:
	    expose = (GdkEventExpose *)event;
	    FileBrowserListHeaderDraw(fb);
	    status = TRUE;
	    break;

	  case GDK_FOCUS_CHANGE:
	    focus = (GdkEventFocus *)event;
	    if(focus->in && !GTK_WIDGET_HAS_FOCUS(w))
	    {
		GTK_WIDGET_SET_FLAGS(w, GTK_HAS_FOCUS);
		FileBrowserListHeaderQueueDraw(fb);
	    }
	    else if(!focus->in && GTK_WIDGET_HAS_FOCUS(w))
	    {
		GTK_WIDGET_UNSET_FLAGS(w, GTK_HAS_FOCUS);
		FileBrowserListHeaderQueueDraw(fb);
	    }
	    status = TRUE;
	    break;

	  case GDK_BUTTON_PRESS:
	    button = (GdkEventButton *)event;
	    if(!GTK_WIDGET_HAS_FOCUS(w))
		gtk_widget_grab_focus(w);
	    if(fb->columns_list != NULL)
	    {
		gint cp, tolor = 3;
		gint p = (gint)button->x;
		GList *glist;
		FileBrowserColumn *column, *prev_column;

		/* Iterate through the list of columns, checking a
		 * column that the pointer is over its resizing
		 * area
		 */
		for(glist = g_list_last(fb->columns_list);
		    glist != NULL;
		    glist = g_list_previous(glist)
		)
		{
		    column = FILE_BROWSER_COLUMN(glist->data);
		    if(column == NULL)
			continue;

		    cp = column->position;
		    if((p >= (cp - tolor)) && (p < (cp + (2 * tolor))))
		    {
			/* Update column drag values */
			column->drag = TRUE;
			column->drag_position = cp;

			/* Draw drag line on list header and list */
			DRAW_DRAG_LINE(cp);
			column->drag_last_drawn_position =
			    column->drag_position;

			break;
		    }
		}
		/* No column was clicked on its resizing area? */
		if(glist == NULL)
		{
		    /* Iterate through all columns to update the focus
		     * and reset the drag state
		     */
		    prev_column = NULL;
		    for(glist = fb->columns_list;
		        glist != NULL;
		        glist = g_list_next(glist)
		    )
		    {
			column = FILE_BROWSER_COLUMN(glist->data);
			if(column == NULL)
			    continue;

			/* Get the left edge column position */
			cp = (prev_column != NULL) ? prev_column->position : 0;

			/* Clicked on this column? */
			if((p >= (cp + tolor)) && (p < (column->position - tolor)))
			{
			    /* Invert the focus */
			    if(column->flags & GTK_HAS_FOCUS)
				column->flags &= ~GTK_HAS_FOCUS;
			    else
				column->flags |= GTK_HAS_FOCUS;
			}
			else
			{
			    column->flags &= ~GTK_HAS_FOCUS;
			}

			column->drag = FALSE;

			prev_column = column;
		    }
		}
		FileBrowserListHeaderQueueDraw(fb);
	    }
	    status = TRUE;
	    break;

	  case GDK_BUTTON_RELEASE:
	    button = (GdkEventButton *)event;
	    if(fb->columns_list != NULL)
	    {
		gint pos_shift_delta = 0;
		GList *glist;
		FileBrowserColumn *column;

		/* Iterate through the columns list, checking for one
		 * that is being dragged and set that column's new
		 * position
		 */
		for(glist = fb->columns_list;
		    glist != NULL;
		    glist = g_list_next(glist)
		)
		{
		    column = FILE_BROWSER_COLUMN(glist->data);
		    if(column == NULL)
			continue;

		    /* This column being dragged? */
		    if(column->drag)
		    {
			column->drag = FALSE;
			pos_shift_delta = column->drag_position -
			    column->position;
			column->position = column->drag_position;
		    }
		    else
		    {
			column->position += pos_shift_delta;
		    }
		}

		/* Redraw the list header and the list */
		FileBrowserListHeaderQueueDraw(fb);
		FileBrowserListQueueDraw(fb);
	    }
	    status = TRUE;
	    break;

	  case GDK_MOTION_NOTIFY:
	    motion = (GdkEventMotion *)event;
	    if(fb->columns_list != NULL)
	    {
		gint tolor = 3, left_column_pos = 0;
		gint p = (gint)motion->x;
		GList *glist;
		GdkCursor *cursor = NULL;
		FileBrowserColumn *column;

		/* Iterate through all columns and check if one is
		 * being dragged (resized) in which case it will be
		 * handled accordingly.  Also if a pointer has
		 * moved into the dragging area of a column then
		 * the new cursor will be specified.
		 */
		for(glist = fb->columns_list;
		    glist != NULL;
		    glist = g_list_next(glist)
		)
		{
		    column = FILE_BROWSER_COLUMN(glist->data);
		    if(column == NULL)
			continue;

		    /* Is this column being dragged? */
		    if(column->drag)
		    {
			column->drag_position = CLIP(
			    p,
			    left_column_pos,
			    w->allocation.width
			);

			/* Draw drag line on list header and list */
			DRAW_DRAG_LINE(column->drag_last_drawn_position);
			DRAW_DRAG_LINE(column->drag_position);
			column->drag_last_drawn_position =
			    column->drag_position;

			/* Update cursor just in case */
			cursor = fb->cur_column_hresize;

			/* No need to handle other columns after
			 * this one since it should be the only
			 * being dragged.
			 */
			break;
		    }
		    else
		    {
			/* Column not being dragged, check if the
			 * pointer has moved into the dragging
			 * area of this column.
			 */
			const gint cp = column->position;
			if((p >= (cp - tolor)) && (p < (cp + (2 * tolor))))
			    cursor = fb->cur_column_hresize;

			left_column_pos = column->position;
		    }
		}
		gdk_window_set_cursor(w->window, cursor);
		gdk_flush();
	    }
	    status = TRUE;
	    break;

	  case GDK_ENTER_NOTIFY:
	    crossing = (GdkEventCrossing *)event;
	    status = TRUE;
	    break;

	  case GDK_LEAVE_NOTIFY:
	    crossing = (GdkEventCrossing *)event;
	    gdk_window_set_cursor(w->window, NULL);
	    gdk_flush();
	    status = TRUE;
	    break;
	}

#undef DRAW_DRAG_LINE

	return(status);
}

/*
 *	List GtkDrawingArea event signal callback.
 */
static gint FileBrowserListEventCB(
	GtkWidget *widget, GdkEvent *event, gpointer data
)
{
	gboolean key_press;
	gint status = FALSE;
	guint key_state;
	GdkEventConfigure *configure;
	GdkEventExpose *expose;
	GdkEventFocus *focus;
	GdkEventKey *key;
	GdkEventButton *button;
	GdkEventMotion *motion;
	GdkEventCrossing *crossing;
	FileBrowser *fb = FILE_BROWSER(data);
	if((widget == NULL) || (event == NULL) || (fb == NULL))
	    return(status);

	switch((gint)event->type)
	{
	  case GDK_CONFIGURE:
	    configure = (GdkEventConfigure *)event;
	    /* Recreate the list's GdkPixmap buffer */
	    GDK_PIXMAP_UNREF(fb->list_pm);
	    if((configure->width > 0) && (configure->height > 0))
		fb->list_pm = gdk_pixmap_new(
		    configure->window,
		    configure->width, configure->height,
		    -1
		);
	    else
		fb->list_pm = NULL;
	    /* Update the list's GtkAdjustments and object positions */
	    FileBrowserListUpdatePositions(fb);
	    status = TRUE;
	    break;

	  case GDK_EXPOSE:
	    expose = (GdkEventExpose *)event;
	    FileBrowserListDraw(fb);
	    status = TRUE;
	    break;

	  case GDK_FOCUS_CHANGE:
	    focus = (GdkEventFocus *)event;
	    if(focus->in && !GTK_WIDGET_HAS_FOCUS(widget))
	    {
		GTK_WIDGET_SET_FLAGS(widget, GTK_HAS_FOCUS);

		/* Set the focus object if it was unset and there
		 * are objects in the list
		 */
		if((fb->focus_object < 0) && (fb->objects_list != NULL))
		    fb->focus_object = 0;

		FileBrowserListQueueDraw(fb);
	    }
	    else if(!focus->in && GTK_WIDGET_HAS_FOCUS(widget))
	    {
		GTK_WIDGET_UNSET_FLAGS(widget, GTK_HAS_FOCUS);
		FileBrowserListQueueDraw(fb);
	    }
	    status = TRUE;
	    break;

	  case GDK_KEY_PRESS:
	  case GDK_KEY_RELEASE:
	    key = (GdkEventKey *)event;
	    key_press = (key->type == GDK_KEY_PRESS) ? TRUE : FALSE;
	    key_state = key->state;
#define STOP_KEY_SIGNAL_EMIT	{		\
 if(widget != NULL)				\
  gtk_signal_emit_stop_by_name(			\
   GTK_OBJECT(widget),				\
   key_press ?					\
    "key_press_event" : "key_release_event"	\
  );						\
}
	    /* First handle key event by list format */
	    switch(fb->list_format)
	    {
	      /* Vertical List Format */
	      case FB_LIST_FORMAT_VERTICAL_DETAILS:
	      case FB_LIST_FORMAT_VERTICAL:
		switch(key->keyval)
		{
		  case GDK_Up:			/* Change Focus Up */
		  case GDK_KP_Up:
		  case GDK_Left:
		  case GDK_KP_Left:
		    if(key_press)
		    {
			/* Scroll only? */
			if(key_state & GDK_CONTROL_MASK)
			{
			    GtkAdjustment *adj = GTK_RANGE(fb->list_vsb)->adjustment;
			    if(adj != NULL)
				GTK_ADJUSTMENT_SET_VALUE(
				    adj,
				    adj->value - adj->step_increment
				);
			}
			else if(fb->focus_object > 0)
			{
			    gint	n = fb->focus_object,
					i = n - 1;

			    fb->focus_object = i;

			    /* Select? */
			    if((key_state & GDK_SHIFT_MASK) &&
			       (fb->objects_list != NULL)
			    )
			    {
				if(!FB_IS_OBJECT_SELECTED(fb, n))
				    fb->selection = g_list_append(
					fb->selection,
					(gpointer)n
				    );
				if(!FB_IS_OBJECT_SELECTED(fb, i))
				    fb->selection = g_list_append(
					fb->selection,
					(gpointer)i
				    );
				fb->selection_end = g_list_last(fb->selection);
				FileBrowserEntrySetSelectedObjects(fb);
			    }

			    /* Scroll if focus object is not fully visible */
			    if(FileBrowserListObjectVisibility(fb, i) !=
				GTK_VISIBILITY_FULL
			    )
				FileBrowserListMoveToObject(fb, i, 0.5f);
			    else
				FileBrowserListQueueDraw(fb);
			}
		    }
		    STOP_KEY_SIGNAL_EMIT		    
		    status = TRUE;
		    break;

		  case GDK_Down:		/* Change Focus Down */
		  case GDK_KP_Down:
		  case GDK_Right:
		  case GDK_KP_Right:
		    if(key_press)
		    {
			/* Scroll only? */
			const gint nobjs = g_list_length(fb->objects_list);
			if(key_state & GDK_CONTROL_MASK)
			{
			    GtkAdjustment *adj = GTK_RANGE(fb->list_vsb)->adjustment;
			    if(adj != NULL)
				GTK_ADJUSTMENT_SET_VALUE(
				    adj,
				    adj->value + adj->step_increment
				);
			}
			else if(fb->focus_object < (nobjs - 1))
			{
			    gint	n = fb->focus_object,
					i = n + 1;

			    fb->focus_object = i;

			    /* Select? */
			    if((key_state & GDK_SHIFT_MASK) &&
			       (nobjs > 0)
			    )
			    {
				if(!FB_IS_OBJECT_SELECTED(fb, n))
				    fb->selection = g_list_append(
					fb->selection,
					(gpointer)n
				    );
				if(!FB_IS_OBJECT_SELECTED(fb, i))
				    fb->selection = g_list_append(
					fb->selection,
					(gpointer)i
				    );
				fb->selection_end = g_list_last(fb->selection);
				FileBrowserEntrySetSelectedObjects(fb);
			    }

			    /* Scroll if focus object is not fully visible */
			    if(FileBrowserListObjectVisibility(fb, i) !=
				GTK_VISIBILITY_FULL
			    )
				FileBrowserListMoveToObject(fb, i, 0.5f);
			    else
				FileBrowserListQueueDraw(fb);
			}
		    }
		    STOP_KEY_SIGNAL_EMIT
		    status = TRUE;
		    break;

		  case GDK_Page_Up:		/* Page Focus Up */
		  case GDK_KP_Page_Up:
		    if(key_press)
		    {
			/* Scroll only? */
			const gint nobjs = g_list_length(fb->objects_list);
			if(key_state & GDK_CONTROL_MASK)
			{
			    GtkAdjustment *adj = GTK_RANGE(fb->list_vsb)->adjustment;
			    if(adj != NULL)
				GTK_ADJUSTMENT_SET_VALUE(
				    adj,
				    adj->value - adj->page_increment
			        );
			}
			else
			{
			    FileBrowserObject *o = FILE_BROWSER_OBJECT(
				g_list_nth_data(fb->objects_list, 0)
			    );
			    gint i, n = fb->focus_object,
				row_height = (o != NULL) ?
				    (o->height + 1) : 0;
			    GtkRange *range = (GtkRange *)fb->list_vsb;
			    GtkAdjustment *adj = (range != NULL) ?
				range->adjustment : NULL;

			    if((row_height > 0) && (adj != NULL))
			    {
				fb->focus_object = i = MAX(
				    fb->focus_object -
				    (gint)(adj->page_increment / row_height),
				    0
				);

				/* Select? */
				if((key_state & GDK_SHIFT_MASK) &&
				   (nobjs > 0)
				)
				{
				    gint j;
				    for(j = MIN(n, nobjs - 1); j >= i; j--)
				    {
					if(!FB_IS_OBJECT_SELECTED(fb, j))
					    fb->selection = g_list_append(
						fb->selection, (gpointer)j
					    );
				    }
				    fb->selection_end = g_list_last(fb->selection);
				    FileBrowserEntrySetSelectedObjects(fb);
				}

				/* Scroll if focus object is not fully visible */
				if(FileBrowserListObjectVisibility(fb, i) !=
				    GTK_VISIBILITY_FULL
				)
				    FileBrowserListMoveToObject(fb, i, 0.0f);
				else
				    FileBrowserListQueueDraw(fb);
			    }
			}
		    }
		    status = TRUE;
		    break;

		  case GDK_Page_Down:		/* Page Focus Down */
		  case GDK_KP_Page_Down:
		    if(key_press)
		    {
			/* Scroll only? */
			const gint nobjs = g_list_length(fb->objects_list);
			if(key_state & GDK_CONTROL_MASK)
			{
			    GtkAdjustment *adj = GTK_RANGE(fb->list_vsb)->adjustment;
			    if(adj != NULL)
				GTK_ADJUSTMENT_SET_VALUE(
				    adj,
				    adj->value + adj->page_increment
			        );
			}
			else
			{
			    FileBrowserObject *o = FILE_BROWSER_OBJECT(
				g_list_nth_data(fb->objects_list, 0)
			    );
			    gint i, n = fb->focus_object,
				row_height = (o != NULL) ?
				    (o->height + 1) : 0;
			    GtkRange *range = (GtkRange *)fb->list_vsb;
			    GtkAdjustment *adj = (range != NULL) ?
				range->adjustment : NULL;

			    if((row_height > 0) && (adj != NULL))
			    {
				fb->focus_object = i = MIN(
				    fb->focus_object +
					(gint)(adj->page_increment / row_height),
				    (nobjs - 1)
				);

				/* Select? */
				if((key_state & GDK_SHIFT_MASK) &&
				   (nobjs > 0)
				)
				{
				    gint j;
				    for(j = MAX(n, 0); j <= i; j++)
				    {
					if(!FB_IS_OBJECT_SELECTED(fb, j))
					    fb->selection = g_list_append(
						fb->selection, (gpointer)j
					    );
				    }
				    fb->selection_end = g_list_last(fb->selection);
				    FileBrowserEntrySetSelectedObjects(fb);
				}

				/* Scroll if focus object is not fully visible */
				if(FileBrowserListObjectVisibility(fb, i) !=
				    GTK_VISIBILITY_FULL
				)
				    FileBrowserListMoveToObject(fb, i, 1.0f);
				else
				    FileBrowserListQueueDraw(fb);
			    }
			}
		    }
		    status = TRUE;
		    break;

		  case GDK_Home:		/* Focus Beginning */
		  case GDK_KP_Home:
		    if(key_press)
		    {
			/* Scroll only? */
			if(key_state & GDK_CONTROL_MASK)
			{
			    GtkAdjustment *adj = GTK_RANGE(fb->list_vsb)->adjustment;
			    if(adj != NULL)
				GTK_ADJUSTMENT_SET_VALUE(
				    adj,
				    adj->lower
				);
			}
			else if(fb->focus_object > 0)
			{
			    const gint nobjs = g_list_length(fb->objects_list);
			    gint	i = 0,
					n = fb->focus_object;

			    fb->focus_object = i;

			    /* Select? */
			    if((key_state & GDK_SHIFT_MASK) &&
			       (nobjs > 0)
			    )
			    {
				gint j;
				for(j = MIN(n, (nobjs - 1)); j >= i; j--)
				{
				    if(!FB_IS_OBJECT_SELECTED(fb, j))
					fb->selection = g_list_append(
					    fb->selection, (gpointer)j
					);
				}
				fb->selection_end = g_list_last(fb->selection);
				FileBrowserEntrySetSelectedObjects(fb);
			    }

			    /* Scroll if focus object is not fully visible */
			    if(FileBrowserListObjectVisibility(fb, i) !=
				GTK_VISIBILITY_FULL
			    )
				FileBrowserListMoveToObject(fb, i, 0.0f);
			    else
				FileBrowserListQueueDraw(fb);
			}
		    }
		    status = TRUE;
		    break;

		  case GDK_End:			/* Focus End */
		  case GDK_KP_End:
		    if(key_press)
		    {
			/* Scroll only? */
			const gint nobjs = g_list_length(fb->objects_list);
			if(key_state & GDK_CONTROL_MASK)
			{
			    GtkAdjustment *adj = GTK_RANGE(fb->list_vsb)->adjustment;
			    if(adj != NULL)
				GTK_ADJUSTMENT_SET_VALUE(
				    adj,
				    adj->upper - adj->page_size
				);
			}
			else if(fb->focus_object < (nobjs - 1))
			{
			    gint	i = MAX((nobjs - 1), 0),
					n = fb->focus_object;

			    fb->focus_object = i;

			    /* Select? */
			    if((key_state & GDK_SHIFT_MASK) &&
			       (nobjs > 0)
			    )
			    {
				gint j;
				for(j = MAX(n, 0); j <= i; j++)
				{
				    if(!FB_IS_OBJECT_SELECTED(fb, j))
					fb->selection = g_list_append(
					    fb->selection, (gpointer)j
					);
				}
				fb->selection_end = g_list_last(fb->selection);
				FileBrowserEntrySetSelectedObjects(fb);
			    }

			    /* Scroll if focus object is not fully visible */
			    if(FileBrowserListObjectVisibility(fb, i) !=
				GTK_VISIBILITY_FULL
			    )
				FileBrowserListMoveToObject(fb, i, 1.0f);
			    else
				FileBrowserListQueueDraw(fb);
			}
		    }
		    status = TRUE;
		    break;
		}
		break;

	      /* Horizontal List Format */
	      case FB_LIST_FORMAT_STANDARD:
		switch(key->keyval)
		{
		  case GDK_Up:			/* Focus Up */
		  case GDK_KP_Up:
		    if(key_press)
		    {
			/* Scroll only? */
			if(key_state & GDK_CONTROL_MASK)
			{
			    /* No vertical scrolling */
			}
			else if(fb->focus_object > 0)
			{
			    const gint	nobjs = g_list_length(fb->objects_list),
					n = fb->focus_object,
					i = n - 1;

			    fb->focus_object = i;

			    /* Select? */
			    if((key_state & GDK_SHIFT_MASK) &&
			       (nobjs > 0)
			    )
			    {
				if(!FB_IS_OBJECT_SELECTED(fb, n))
				    fb->selection = g_list_append(
					fb->selection, (gpointer)n
				    );
				if(!FB_IS_OBJECT_SELECTED(fb, i))
				    fb->selection = g_list_append(
					fb->selection, (gpointer)i
				    );
				fb->selection_end = g_list_last(fb->selection);
				FileBrowserEntrySetSelectedObjects(fb);
			    }

			    /* Scroll if focus object is not fully visible */
			    if(FileBrowserListObjectVisibility(fb, i) !=
				GTK_VISIBILITY_FULL
			    )
				FileBrowserListMoveToObject(fb, i, 0.5f);
			    else
				FileBrowserListQueueDraw(fb);
			}
		    }
		    STOP_KEY_SIGNAL_EMIT
		    status = TRUE;
		    break;

		  case GDK_Down:		/* Focus Down */
		  case GDK_KP_Down:
		    if(key_press)
		    {
			/* Scroll only? */
			const gint nobjs = g_list_length(fb->objects_list);
			if(key_state & GDK_CONTROL_MASK)
			{
			    /* No vertical scrolling */
			}
			else if(fb->focus_object < (nobjs - 1))
			{
			    const gint	n = MAX(fb->focus_object, 0),
					i = n + 1;

			    fb->focus_object = i;

			    /* Select? */
			    if((key_state & GDK_SHIFT_MASK) &&
			       (nobjs > 0)
			    )
			    {
				if(!FB_IS_OBJECT_SELECTED(fb, n))
				    fb->selection = g_list_append(
					fb->selection, (gpointer)n
				    );
				if(!FB_IS_OBJECT_SELECTED(fb, i))
				    fb->selection = g_list_append(
					fb->selection, (gpointer)i
				    );
				fb->selection_end = g_list_last(fb->selection);
				FileBrowserEntrySetSelectedObjects(fb);
			    }

			    /* Scroll if focus object is not fully visible */
			    if(FileBrowserListObjectVisibility(fb, i) !=
				GTK_VISIBILITY_FULL
			    )
				FileBrowserListMoveToObject(fb, i, 0.5f);
			    else
				FileBrowserListQueueDraw(fb);
			}
		    }
		    STOP_KEY_SIGNAL_EMIT
		    status = TRUE;
		    break;

		  case GDK_Left:		/* Focus Left */
		  case GDK_KP_Left:
		    if(key_press)
		    {
			/* Scroll only? */
			if(key_state & GDK_CONTROL_MASK)
			{
			    GtkAdjustment *adj = GTK_RANGE(fb->list_hsb)->adjustment;
			    if(adj != NULL)
				GTK_ADJUSTMENT_SET_VALUE(
				    adj,
				    adj->value - adj->step_increment
				);
			}
			else if(fb->focus_object > 0)
			{
			    const gint	nobjs = g_list_length(fb->objects_list),
					n = fb->focus_object,
					i = MAX(
			(fb->focus_object - fb->objects_per_row), 0
					);

			    fb->focus_object = i;

			    /* Select? */
			    if((key_state & GDK_SHIFT_MASK) &&
			       (nobjs > 0)
			    )
			    {
				gint j;
				for(j = MIN(n, (nobjs - 1)); j >= i; j--)
				{
				    if(!FB_IS_OBJECT_SELECTED(fb, j))
					fb->selection = g_list_append(
					    fb->selection, (gpointer)j
					);
				}
				fb->selection_end = g_list_last(fb->selection);
				FileBrowserEntrySetSelectedObjects(fb);
			    }

			    /* Scroll if focus object is not fully visible */
			    if(FileBrowserListObjectVisibility(fb, i) !=
			        GTK_VISIBILITY_FULL
			    )
			        FileBrowserListMoveToObject(fb, i, 0.5f);
			    else
			        FileBrowserListQueueDraw(fb);
			}
		    }
		    STOP_KEY_SIGNAL_EMIT
		    status = TRUE;
		    break;

		  case GDK_Right:		/* Focus Right */
		  case GDK_KP_Right:
		    if(key_press)
		    {
			/* Scroll only? */
			const gint nobjs = g_list_length(fb->objects_list);
			if(key_state & GDK_CONTROL_MASK)
			{
			    GtkAdjustment *adj = GTK_RANGE(fb->list_hsb)->adjustment;
			    if(adj != NULL)
				GTK_ADJUSTMENT_SET_VALUE(
				    adj,
				    adj->value + adj->step_increment
				);
			}
			else if(fb->focus_object < (nobjs - 1))
			{
			    const gint	n = MAX(fb->focus_object, 0),
					i = MIN(
					    (n + fb->objects_per_row),
					    (nobjs - 1)
					);

			    fb->focus_object = i;

			    /* Select? */
			    if((key_state & GDK_SHIFT_MASK) &&
			       (nobjs > 0)
			    )
			    {
				gint j;
				for(j = n; j <= i; j++)
				{
				    if(!FB_IS_OBJECT_SELECTED(fb, j))
					fb->selection = g_list_append(
					    fb->selection, (gpointer)j
					);
				}
				fb->selection_end = g_list_last(fb->selection);
				FileBrowserEntrySetSelectedObjects(fb);
			    }

			    /* Scroll if focus object is not fully visible */
			    if(FileBrowserListObjectVisibility(fb, i) !=
			        GTK_VISIBILITY_FULL
			    )
			        FileBrowserListMoveToObject(fb, i, 0.5f);
			    else
			        FileBrowserListQueueDraw(fb);
			}
		    }
		    STOP_KEY_SIGNAL_EMIT
		    status = TRUE;
		    break;

		  case GDK_Page_Up:		/* Page Focus Left */
		  case GDK_KP_Page_Up:
		    if(key_press)
		    {
			/* Scroll only? */
			if(key_state & GDK_CONTROL_MASK)
			{
			    GtkAdjustment *adj = GTK_RANGE(fb->list_hsb)->adjustment;
			    if(adj != NULL)
				GTK_ADJUSTMENT_SET_VALUE(
				    adj,
				    adj->value - adj->page_increment
			        );
			}
			else if(fb->focus_object > 0)
			{
			    const gint	nobjs = g_list_length(fb->objects_list),
					n = fb->focus_object,
					i = MAX(
			(fb->focus_object - fb->objects_per_row), 0
					);

			    fb->focus_object = i;

			    /* Select? */
			    if((key_state & GDK_SHIFT_MASK) &&
			       (nobjs > 0)
			    )
			    {
				gint j;
				for(j = MIN(n, (nobjs - 1)); j >= i; j--)
				{
				    if(!FB_IS_OBJECT_SELECTED(fb, j))
					fb->selection = g_list_append(
					    fb->selection, (gpointer)j
					);
				}
				fb->selection_end = g_list_last(fb->selection);
				FileBrowserEntrySetSelectedObjects(fb);
			    }

			    /* Scroll if focus object is not fully visible */
			    if(FileBrowserListObjectVisibility(fb, i) !=
				GTK_VISIBILITY_FULL
			    )
				FileBrowserListMoveToObject(fb, i, 0.0f);
			    else
				FileBrowserListQueueDraw(fb);
			}
		    }
		    status = TRUE;
		    break;

		  case GDK_Page_Down:		/* Page Focus Right */
		  case GDK_KP_Page_Down:
		    if(key_press)
		    {
			/* Scroll only? */
			const gint nobjs = g_list_length(fb->objects_list);
			if(key_state & GDK_CONTROL_MASK)
			{
			    GtkAdjustment *adj = GTK_RANGE(fb->list_hsb)->adjustment;
			    if(adj != NULL)
				GTK_ADJUSTMENT_SET_VALUE(
				    adj,
				    adj->value + adj->page_increment
			        );
			}
			else if(fb->focus_object < (nobjs - 1))
			{
			    const gint	n = fb->focus_object,
					i = MIN(
			    (fb->focus_object + fb->objects_per_row),
					    (nobjs - 1)
					);

			    fb->focus_object = i;

			    /* Select? */
			    if((key_state & GDK_SHIFT_MASK) &&
			       (nobjs > 0)
			    )
			    {
				gint j;
				for(j = MAX(n, 0); j <= i; j++)
				{
				    if(!FB_IS_OBJECT_SELECTED(fb, j))
					fb->selection = g_list_append(
					    fb->selection, (gpointer)j
					);
				}
				fb->selection_end = g_list_last(fb->selection);
				FileBrowserEntrySetSelectedObjects(fb);
			    }

			    /* Scroll if focus object is not fully visible */
			    if(FileBrowserListObjectVisibility(fb, i) !=
				GTK_VISIBILITY_FULL
			    )
				FileBrowserListMoveToObject(fb, i, 1.0f);
			    else
				FileBrowserListQueueDraw(fb);
			}
		    }
		    status = TRUE;
		    break;

		  case GDK_Home:		/* Focus Beginning */
		  case GDK_KP_Home:
		    if(key_press)
		    {
			/* Scroll only? */
			if(key_state & GDK_CONTROL_MASK)
			{
			    GtkAdjustment *adj = GTK_RANGE(fb->list_hsb)->adjustment;
			    if(adj != NULL)
				GTK_ADJUSTMENT_SET_VALUE(
				    adj,
				    adj->lower
				);
			}
			else if(fb->focus_object > 0)
			{
			    const gint	nobjs = g_list_length(fb->objects_list),
					i = 0,
					n = fb->focus_object;

			    fb->focus_object = i;

			    /* Select? */
			    if((key_state & GDK_SHIFT_MASK) &&
			       (nobjs > 0)
			    )
			    {
				gint j;
				for(j = MIN(n, (nobjs - 1)); j >= i; j--)
				{
				    if(!FB_IS_OBJECT_SELECTED(fb, j))
					fb->selection = g_list_append(
					    fb->selection, (gpointer)j
					);
				}
				fb->selection_end = g_list_last(fb->selection);
				FileBrowserEntrySetSelectedObjects(fb);
			    }

			    /* Scroll if focus object is not fully visible */
			    if(FileBrowserListObjectVisibility(fb, i) !=
				GTK_VISIBILITY_FULL
			    )
				FileBrowserListMoveToObject(fb, i, 0.0f);
			    else
				FileBrowserListQueueDraw(fb);
			}
		    }
		    status = TRUE;
		    break;

		  case GDK_End:			/* Focus End */
		  case GDK_KP_End:
		    if(key_press)
		    {
			/* Scroll only? */
			const gint nobjs = g_list_length(fb->objects_list);
			if(key_state & GDK_CONTROL_MASK)
			{
			    GtkAdjustment *adj = GTK_RANGE(fb->list_hsb)->adjustment;
			    if(adj != NULL)
				GTK_ADJUSTMENT_SET_VALUE(
				    adj,
				    adj->upper - adj->page_size
			        );
			}
			else if(fb->focus_object < (nobjs - 1))
			{
			    const gint	i = MAX(nobjs - 1, 0),
					n = fb->focus_object;

			    fb->focus_object = i;

			    /* Select? */
			    if((key_state & GDK_SHIFT_MASK) &&
			       (nobjs > 0)
			    )
			    {
				gint j;
				for(j = MAX(n, 0); j <= i; j++)
				{
				    if(!FB_IS_OBJECT_SELECTED(fb, j))
					fb->selection = g_list_append(
					    fb->selection, (gpointer)j
					);
				}
				fb->selection_end = g_list_last(fb->selection);
				FileBrowserEntrySetSelectedObjects(fb);
			    }

			    /* Scroll if focus object is not fully visible */
			    if(FileBrowserListObjectVisibility(fb, i) !=
				GTK_VISIBILITY_FULL
			    )
				FileBrowserListMoveToObject(fb, i, 1.0f);
			    else
				FileBrowserListQueueDraw(fb);
			}
		    }
		    status = TRUE;
		    break;
		}
		break;
	    }
	    /* If the key event was not handled above then we will
	     * handle it generally independent of the list format
	     * here
	     */
	    if(!status)
	    {
		switch(key->keyval)
		{
		  case GDK_space:		/* Toggle Select */
		    if(key_press)
		    {
			const gint i = fb->focus_object;
			if(FB_IS_OBJECT_SELECTED(fb, i))
			    fb->selection = g_list_remove(
				fb->selection,
				(gpointer)i
			    );
			else
			    fb->selection = g_list_append(
				fb->selection,
				(gpointer)i
			    );
			fb->selection_end = g_list_last(fb->selection);
			FileBrowserListQueueDraw(fb);
			FileBrowserEntrySetSelectedObjects(fb);
		    }
		    status = TRUE;
		    break;

		  case GDK_BackSpace:		/* Go to parent */
		    if(key_press)
			FileBrowserGoToParent(fb);
		    status = TRUE;
		    break;

		  case GDK_Delete:		/* Delete */
		    if(key_press)
			FileBrowserDelete(fb);
		    status = TRUE;
		    break;

		  default:			/* Seek to the object */
		    if(key_press && isalnum((int)key->keyval))
		    {
			/* Seek to the object who's name starts with
			 * the first character matching the key that
			 * was pressed
			 */
			const gchar c = key->keyval;

			/* Backwards? */
			if(key_state & GDK_MOD1_MASK)
			{
			    gint i;
			    GList *glist;
			    FileBrowserObject *o;

			    for(glist = g_list_last(fb->objects_list),
				    i = g_list_length(fb->objects_list) - 1;
				glist != NULL;
				glist = g_list_previous(glist), i--
			    )
			    {
				o = FILE_BROWSER_OBJECT(glist->data);
				if(o == NULL)
				    continue;

				if(o->name == NULL)
				    continue;

				if(c == *o->name)
				{
				    FileBrowserListMoveToObject(fb, i, 0.5f);
				    break;
				}
			    }
			}
			else
			{
			    gint i;
			    GList *glist;
			    FileBrowserObject *o;

			    for(glist = fb->objects_list, i = 0;
				glist != NULL;
				glist = g_list_next(glist), i++
			    )
			    {
				o = FILE_BROWSER_OBJECT(glist->data);
				if(o == NULL)
				    continue;

				if(o->name == NULL)
				    continue;

				if(c == *o->name)
				{
				    FileBrowserListMoveToObject(fb, i, 0.5f);
				    break;
				}
			    }
			}
			status = TRUE;
		    }
		    break;
		}
	    }
#undef STOP_KEY_SIGNAL_EMIT
	    break;

	  case GDK_BUTTON_PRESS:
	    button = (GdkEventButton *)event;
	    /* Unmap the floating prompt as needed */
	    if(FPromptIsQuery())
		FPromptBreakQuery();
	    /* Grab the focus as needed */
	    if(!GTK_WIDGET_HAS_FOCUS(widget))
		gtk_widget_grab_focus(widget);
	    /* Handle by the button number */
	    switch(button->button)
	    {
	      case GDK_BUTTON1:		/* Select */
		/* Multiple select? */
		if(button->state & GDK_CONTROL_MASK)
		{
		    const gint i = FileBrowserListSelectCoordinates(
			fb,
			(gint)button->x, (gint)button->y
		    );
		    if(i > -1)
		    {
			/* If this object is already selected then
			 * unselect it, otherwise select it
			 */
			fb->last_single_select_object = -1;
			fb->focus_object = i;
			if(FB_IS_OBJECT_SELECTED(fb, i))
			{
			    fb->selection = g_list_remove(
				fb->selection,
				(gpointer)i
			    );
			    fb->selection_end = g_list_last(fb->selection);
			}
			else
			{
			    fb->selection = g_list_append(
				fb->selection,
				(gpointer)i
			    );
			    fb->selection_end = g_list_last(fb->selection);

			    /* Update the DND drag icon */
			    FileBrowserListSetDNDDragIcon(fb);
			}

			/* If this object is not fully visibile then
			 * scroll to it
			 */
			if(FileBrowserListObjectVisibility(fb, i) !=
			    GTK_VISIBILITY_FULL
			)
			    FileBrowserListMoveToObject(fb, i, 0.5f);
		    }
		}
		/* Series select? */
		else if(button->state & GDK_SHIFT_MASK)
		{
		    const gint i = FileBrowserListSelectCoordinates(
			fb,
			(gint)button->x, (gint)button->y
		    );
		    if(i > -1)
		    {
			/* Select all objects between the last selected
			 * object and this one
			 */
			fb->last_single_select_object = -1;
			fb->focus_object = i;
			if(fb->selection_end != NULL)
			{
			    const gint n = (gint)fb->selection_end->data;
			    if(i > n)
			    {
				gint j = n + 1;
				while(j <= i)
				{
				    if(!FB_IS_OBJECT_SELECTED(fb, j))
					fb->selection = g_list_append(
					    fb->selection,
					    (gpointer)j
					);
				    j++;
				}
				fb->selection_end = g_list_last(fb->selection);
			    }
			    else if(i < n)
			    {
				gint j = n - 1;
				while(j >= i)
				{
				    if(!FB_IS_OBJECT_SELECTED(fb, j))
					fb->selection = g_list_append(
					    fb->selection,
					    (gpointer)j
					);
				    j--;
				}
				fb->selection_end = g_list_last(fb->selection);
			    }
			}
			else
			{
			    /* No previously selected object, so just
			     * select this one
			     */
			    fb->selection = g_list_append(
				fb->selection,
				(gpointer)i
			    );
			    fb->selection_end = g_list_last(fb->selection);
			}

			/* Update the DND drag icon */
			FileBrowserListSetDNDDragIcon(fb);

			/* If this object is not fully visibile then
			 * scroll it it
			 */
			if(FileBrowserListObjectVisibility(fb, i) !=
			    GTK_VISIBILITY_FULL
			)
			    FileBrowserListMoveToObject(fb, i, 0.5f);
		    }
		}
		/* Single select */
		else
		{
		    const gint i = FileBrowserListSelectCoordinates(
			fb,
			(gint)button->x, (gint)button->y
		    );
		    if(i > -1)
		    {
			/* If the last single selected object is
			 * the same as this object then update
			 * last_single_select_object
			 */
			if((g_list_length(fb->selection) == 1) ?
			    ((gint)fb->selection->data == i) : FALSE
			)
			    fb->last_single_select_object = i;
			else
			    fb->last_single_select_object = -1;

			/* Unselect all */
			fb->focus_object = -1;
			g_list_free(fb->selection);
			fb->selection = fb->selection_end = NULL;

			/* Select this object */
			fb->focus_object = i;
			fb->selection = g_list_append(
			    fb->selection,
			    (gpointer)i
			);
			fb->selection_end = g_list_last(fb->selection);

			/* Update the DND drag icon */
			FileBrowserListSetDNDDragIcon(fb);

			/* If this object is not fully visibile then
			 * scroll to it
			 */
			if(FileBrowserListObjectVisibility(fb, i) !=
			    GTK_VISIBILITY_FULL
			)
			    FileBrowserListMoveToObject(fb, i, 0.5f);
		    }
		    else
		    {
			/* Unselect all */
			fb->focus_object = -1;
			g_list_free(fb->selection);
			fb->selection = fb->selection_end = NULL;
		    }
		}

		FileBrowserListQueueDraw(fb);

		/* Update the File Name GtkEntry's value with the
		 * new list of selected objects
		 */
		FileBrowserEntrySetSelectedObjects(fb);

		status = TRUE;
		break;

	      case GDK_BUTTON2:
		/* Start scrolling */
		gdk_pointer_grab(
		    button->window,
		    FALSE,
		    GDK_BUTTON_PRESS_MASK |
			GDK_BUTTON_RELEASE_MASK |
			GDK_POINTER_MOTION_MASK |
			GDK_POINTER_MOTION_HINT_MASK,
		    NULL,
		    fb->cur_translate,
		    button->time
		);
		status = TRUE;
		break;

	      case GDK_BUTTON3:
		/* Right Click GtkMenu */
		if(fb->list_menu != NULL)
		{
		    GtkMenu *menu = GTK_MENU(fb->list_menu);
		    const gint i = FileBrowserListSelectCoordinates(
			fb,
			(gint)button->x, (gint)button->y
		    );
		    /* If ctrl or shift modifier keys are held then do
		     * not modify selection before mapping of menu
		     */
		    if((button->state & GDK_CONTROL_MASK) ||
		       (button->state & GDK_SHIFT_MASK)
		    )
		    {
			/* Do not modify selection, just update focus
			 * object
			 */
			fb->focus_object = i;
		    }
		    else if(i > -1)
		    {
			/* Unselect all the objects */
			fb->focus_object = -1;
			if(fb->selection != NULL)
			    g_list_free(fb->selection);
			fb->selection = fb->selection_end = NULL;

			/* Select this object */
			fb->focus_object = i;
			fb->selection = g_list_append(
			    fb->selection,
			    (gpointer)i
			);
			fb->selection_end = g_list_last(fb->selection);

			/* Update the DND drag icon */
			FileBrowserListSetDNDDragIcon(fb);

			/* Update the File Name GtkEntry's value with
			 * the new list of selected objects
			 */
			FileBrowserEntrySetSelectedObjects(fb);
		    }

		    /* Map the Right Click GtkMenu */
		    gtk_menu_popup(
			menu,
			NULL, NULL,
			NULL, NULL,
			button->button, button->time
		    );
		}
		status = TRUE;
		break;

	      case GDK_BUTTON4:
		/* Scroll left */
		if(fb->list_format == FB_LIST_FORMAT_STANDARD)
		{
		    GtkAdjustment *adj = GTK_RANGE(fb->list_hsb)->adjustment;
		    if(adj != NULL)
		    {
			const gfloat inc = MAX(
			    (0.25f * adj->page_size),
			    adj->step_increment
			);
			GTK_ADJUSTMENT_SET_VALUE(
			    adj,
			    adj->value - inc
			);
		    }
		}
		/* Scroll up */
		else
		{
		    GtkAdjustment *adj = GTK_RANGE(fb->list_vsb)->adjustment;
		    if(adj != NULL)
		    {
			const gfloat inc = MAX(
			    (0.25f * adj->page_size),
			    adj->step_increment
			);
			GTK_ADJUSTMENT_SET_VALUE(
			    adj,
			    adj->value - inc
			);
		    }
		}
		status = TRUE;
		break;

	      case GDK_BUTTON5:
		/* Scroll right */
		if(fb->list_format == FB_LIST_FORMAT_STANDARD)
		{
		    GtkAdjustment *adj = GTK_RANGE(fb->list_hsb)->adjustment;
		    if(adj != NULL)
		    {
			const gfloat inc = MAX(
			    (0.25f * adj->page_size),
			    adj->step_increment
			);
			GTK_ADJUSTMENT_SET_VALUE(
			    adj,
			    adj->value + inc
			);
		    }
		}
		/* Scroll down */
		else
		{
		    GtkAdjustment *adj = GTK_RANGE(fb->list_vsb)->adjustment;
		    if(adj != NULL)
		    {
			const gfloat inc = MAX(
			    (0.25f * adj->page_size),
			    adj->step_increment
			);
			GTK_ADJUSTMENT_SET_VALUE(
			    adj,
			    adj->value + inc
			);
		    }
		}
		status = TRUE;
		break;
	    }
	    fb->button = button->button;
	    fb->drag_last_x = (gint)button->x;
	    fb->drag_last_y = (gint)button->y;
	    break;

	  case GDK_BUTTON_RELEASE:
	    button = (GdkEventButton *)event;
	    /* Handle by button number */
	    switch(button->button)
	    {
	      case GDK_BUTTON2:
		/* Stop scrolling as needed */
		if(gdk_pointer_is_grabbed())
		    gdk_pointer_ungrab(button->time);
		status = TRUE;
		break;
	    }
	    fb->button = 0;
	    break;

	  case GDK_2BUTTON_PRESS:
	    button = (GdkEventButton *)event;
	    /* Handle by button number */
	    switch(button->button)
	    {
	      case GDK_BUTTON1:
		if(fb->last_single_select_object > -1)
		{
		    /* File Name GtkEntry activate */
		    FileBrowserEntryActivate(fb);
		}
		status = TRUE;
		break;
	    }
	    break;

	  case GDK_MOTION_NOTIFY:
	    motion = (GdkEventMotion *)event;
#define STOP_SIGNAL_EMIT(_w_)	{		\
 gtk_signal_emit_stop_by_name(			\
  GTK_OBJECT(_w_),				\
  "motion_notify_event"				\
 );						\
}
	    switch(fb->button)
	    {
		gint dx, dy;
		GtkAdjustment *adj;

	      case GDK_BUTTON1:			/* Drag and drop starting */
		/* Needed to start drag */
		if(motion->is_hint)
		{
		    gint x, y;
		    GdkModifierType mask;
		    gdk_window_get_pointer(
			motion->window, &x, &y, &mask
		    );
		}
		status = TRUE;
		break;

	      case GDK_BUTTON2:			/* Middle click scrolling */
		if(motion->is_hint)
		{
		    gint x, y;
		    GdkModifierType mask;
		    gdk_window_get_pointer(
			motion->window, &x, &y, &mask
		    );
		    dx = x - fb->drag_last_x;
		    dy = y - fb->drag_last_y;
		    fb->drag_last_x = x;
		    fb->drag_last_y = y;
		}
		else
		{
		    dx = (gint)(motion->x - fb->drag_last_x);
		    dy = (gint)(motion->y - fb->drag_last_y);
		    fb->drag_last_x = (gint)motion->x;
		    fb->drag_last_y = (gint)motion->y;
		}

		adj = GTK_RANGE(fb->list_hsb)->adjustment;
		if((adj != NULL) && (dx != 0))
		    GTK_ADJUSTMENT_SET_VALUE(
			adj,
			adj->value - dx
		    );

		adj = GTK_RANGE(fb->list_vsb)->adjustment;
		if((adj != NULL) && (dy != 0))
		    GTK_ADJUSTMENT_SET_VALUE(
			adj,
			adj->value - dy
		    );

		STOP_SIGNAL_EMIT(widget);
		status = TRUE;
		break;
	    }
#undef STOP_SIGNAL_EMIT
	    break;

	  case GDK_ENTER_NOTIFY:
	    crossing = (GdkEventCrossing *)event;
	    status = TRUE;
	    break;

	  case GDK_LEAVE_NOTIFY:
	    crossing = (GdkEventCrossing *)event;
	    status = TRUE;
	    break;
	}

	return(status);
}

/*
 *	List "realize" signal callback.
 */
static void FileBrowserListRealizeCB(GtkWidget *widget, gpointer data)
{
	GdkWindow *window;
	FileBrowser *fb = FILE_BROWSER(data);
	if((widget == NULL) || (fb == NULL))
	    return;

	window = widget->window;
	if(window == NULL)
	    return;

	if(fb->transparent_stipple_bm == NULL)
	{
	    const gchar transparent_stipple_data[] = {
		0x55, 0xaa, 0x55, 0xaa,
		0x55, 0xaa, 0x55, 0xaa
	    };
	    fb->transparent_stipple_bm = gdk_bitmap_create_from_data(
		window,
		transparent_stipple_data,
		8, 8
	    );
	}

	if(fb->gc == NULL)
	    fb->gc = gdk_gc_new(window);

	/* Mark that the List GtkDrawingArea was realized */
	fb->flags |= FB_REALIZED;
}

/*
 *	List scrollbar GtkAdjustment "value_changed" signal callback.
 */
static void FileBrowserListScrollCB(
	GtkAdjustment *adj, gpointer data
)
{
	FileBrowser *fb = FILE_BROWSER(data);
	if(fb == NULL)
	    return;

	FileBrowserListQueueDraw(fb);
}

/*
 *	Select all callback.
 */
static void FileBrowserSelectAllCB(GtkWidget *widget, gpointer data)
{
	gint i, nobjs;
	FileBrowser *fb = FILE_BROWSER(data);
	if(fb == NULL)
	    return;

	if(fb->selection != NULL)
	    g_list_free(fb->selection);
	fb->selection = NULL;
	nobjs = g_list_length(fb->objects_list);
	for(i = 0; i < nobjs; i++)
	    fb->selection = g_list_append(
		fb->selection, (gpointer)i
	    );
	fb->selection_end = g_list_last(fb->selection);
	FileBrowserListQueueDraw(fb);
	FileBrowserEntrySetSelectedObjects(fb);
}

/*
 *	Unselect all callback.
 */
static void FileBrowserUnselectAllCB(GtkWidget *widget, gpointer data)
{
	FileBrowser *fb = FILE_BROWSER(data);
	if(fb == NULL)
	    return;

	if(fb->selection != NULL)
	    g_list_free(fb->selection);
	fb->selection = fb->selection_end = NULL;
	FileBrowserListQueueDraw(fb);
	FileBrowserEntrySetSelectedObjects(fb);
}

/*
 *	Invert selection callback.
 */
static void FileBrowserInvertSelectionCB(GtkWidget *widget, gpointer data)
{
	gint i, nobjs;
	GList *glist;
	FileBrowser *fb = FILE_BROWSER(data);
	if(fb == NULL)
	    return;

	nobjs = g_list_length(fb->objects_list);
	glist = NULL;
	for(i = 0; i < nobjs; i++)
	{
	    if(!FB_IS_OBJECT_SELECTED(fb, i))
		glist = g_list_append(glist, (gpointer)i);
	}
	if(fb->selection != NULL)
	    g_list_free(fb->selection);
	fb->selection = glist;
	fb->selection_end = g_list_last(fb->selection);
	FileBrowserListQueueDraw(fb);
	FileBrowserEntrySetSelectedObjects(fb);
}


/*
 *	Rename callback.
 */
static void FileBrowserRenameCB(GtkWidget *widget, gpointer data)
{
	FileBrowserRenameQuery(FILE_BROWSER(data));
}

/*
 *	Change permissions callback.
 */
static void FileBrowserCHModCB(GtkWidget *widget, gpointer data)
{
	FileBrowserCHModQuery(FILE_BROWSER(data));
}

/*
 *	Delete callback.
 */
static void FileBrowserDeleteCB(GtkWidget *widget, gpointer data)
{
	FileBrowserDelete(FILE_BROWSER(data));
}


/*
 *	Entry complete path "key_press_event" or "key_release_event"
 *	signal callback.
 */
static gint FileBrowserEntryCompletePathKeyCB(
	GtkWidget *widget, GdkEventKey *key, gpointer data
)
{
	gboolean key_press;
	gint status = FALSE;
	guint key_state;
	GtkEntry *entry = GTK_ENTRY(widget);
	FileBrowser *fb = FILE_BROWSER(data);
	if((entry == NULL) || (key == NULL) || (fb == NULL))
	    return(status);

	key_press = (key->type == GDK_KEY_PRESS) ? TRUE : FALSE;
	key_state = key->state;

#define SIGNAL_EMIT_STOP	{		\
 gtk_signal_emit_stop_by_name(			\
  GTK_OBJECT(widget),				\
  (key_press) ?					\
   "key_press_event" : "key_release_event"	\
 );						\
}

	switch(key->keyval)
	{
	  case GDK_Tab:
	    /* Skip this if the shift or ctrl keys are held */
	    if((key_state & GDK_CONTROL_MASK) ||
	       (key_state & GDK_SHIFT_MASK)
	    )
		return(status);

	    if(key_press)
	    {
		gchar *path = STRDUP(gtk_entry_get_text(entry));
		if(path != NULL)
		{
		    gint status;
		    gchar *prev_path;

		    /* Because the entry is usually blank or not an
		     * absolute path, the path must first be made into
		     * an absolute path before completing it
		     */
		    if(!g_path_is_absolute(path))
		    {
			if(STRISEMPTY(path))
			{
			    g_free(path);
			    path = STRDUP(FileBrowserGetLocation(fb));
			}
			else
			{
			    gchar *path2 = STRDUP(PrefixPaths(
				FileBrowserGetLocation(fb), path
			    ));
			    g_free(path);
			    path = path2;
			}
		    }

		    prev_path = STRDUP(path);

		    /* Complete the path */
		    path = CompletePath(path, &status);

		    /* Set the completed path to the entry */
		    gtk_entry_set_text(entry, (path != NULL) ? path : "");
		    gtk_entry_set_position(entry, -1);

		    /* If there was no change in the path then beep */
		    if((prev_path != NULL) && (path != NULL))
		    {
			if(!strcmp(
			    (const char *)prev_path,
			    (const char *)path
			))
			    gdk_beep();
		    }

		    g_free(prev_path);
		    g_free(path);
		}
	    }
	    SIGNAL_EMIT_STOP
	    status = TRUE;
	    break;
	}

#undef SIGNAL_EMIT_STOP

	return(status);
}

/*
 *	File Name GtkEntry "activate" signal callback.
 */
static void FileBrowserEntryActivateCB(GtkWidget *widget, gpointer data)
{
	FileBrowserEntryActivate(FILE_BROWSER(data));
}

/*
 *	Types popup list box "changed" signal callback.
 */
static void FileBrowserTypesPUListChangedCB(
	GtkWidget *widget, const gint i, gpointer data
)
{
	GtkWidget *pulist;
	fb_type_struct *t_sel, *t;
	FileBrowser *fb = FILE_BROWSER(data);
	if((widget == NULL) || (fb == NULL))
	    return;

	pulist = PUListBoxGetPUList(widget);
	if(pulist == NULL)
	    return;

	t_sel = FB_TYPE(PUListGetItemData(pulist, i));
	if(t_sel == NULL)
	    return;

	t = &fb->cur_type;

	g_free(t->ext);
	t->ext = STRDUP(t_sel->ext);

	g_free(t->name);
	t->name = STRDUP(t_sel->name);

	/* Update the listings only if not frozen */
	if(fb->freeze_count <= 0)
	{
	    fb->freeze_count++;

	    /* Update the list */
	    FileBrowserListUpdate(fb, NULL);

	    fb->freeze_count--;
	}
}

/*
 *	File browser ok button signal callback.
 */
static void FileBrowserOKCB(GtkWidget *widget, gpointer data)
{
	FileBrowserOK(FILE_BROWSER(data));
}

/*
 *	File browser cancel button signal callback.
 */
static void FileBrowserCancelCB(GtkWidget *widget, gpointer data)
{
	FileBrowserCancel(FILE_BROWSER(data));
}


/*
 *	Initializes the file browser.
 */
gint FileBrowserInit(void)
{
	const gint	border_major = 5,
			border_minor = 2;
	gint i;
	GdkWindow *window;
	GtkAdjustment *adj;
	GtkAccelGroup *accelgrp;
	GtkWidget	*w,
			*parent, *parent2, *parent3, *parent4,
			*toplevel,
			*pulist;
	FileBrowser *fb = &file_browser;

	fb->toplevel = toplevel = gtk_window_new(GTK_WINDOW_DIALOG);
	fb->accelgrp = accelgrp = gtk_accel_group_new();
	fb->gc = NULL;
	fb->busy_count = 0;
	fb->freeze_count = 0;
	fb->main_level = 0;
	fb->flags = FB_SHOW_HIDDEN_OBJECTS;
	fb->transparent_stipple_bm = NULL;
	fb->cur_busy = gdk_cursor_new(GDK_WATCH);
	fb->cur_column_hresize = gdk_cursor_new(GDK_SB_H_DOUBLE_ARROW);
	fb->cur_translate = gdk_cursor_new(GDK_FLEUR);
	fb->list_pm = NULL;
	fb->cur_location = NULL;
	fb->list_format = FB_LIST_FORMAT_STANDARD;
	fb->columns_list = NULL;
	fb->icons_list = NULL;
#if defined(_WIN32)
	fb->uid = 0;
	fb->euid = 0;
	fb->gid = 0;
	fb->egid = 0;
#else
	fb->uid = getuid();
	fb->euid = geteuid();
	fb->gid = getgid();
	fb->egid = getegid();
#endif
#if defined(_WIN32)
	fb->home_path = STRDUP("C:\\");
#else
	fb->home_path = STRDUP(g_getenv("HOME"));
#endif
	fb->device_paths_list = NULL;
	fb->objects_list = NULL;
	fb->objects_per_row = 0;
	fb->focus_object = -1;
	fb->last_single_select_object = -1;
	fb->selection = fb->selection_end = NULL;
	memset(&fb->cur_type, 0x00, sizeof(fb_type_struct));
	fb->selected_path = NULL;
	fb->total_selected_paths = 0;
	fb->object_created_cb = NULL;
	fb->object_created_data = NULL;
	fb->object_modified_cb = NULL;
	fb->object_modified_data = NULL;
	fb->object_deleted_cb = NULL;
	fb->object_deleted_data = NULL;
	fb->button = 0;
	fb->drag_flags = 0;
	fb->drag_last_x = 0;
	fb->drag_last_y = 0;

	fb->freeze_count++;

	/* Toplevel GtkWindow */
	w = toplevel;
	gtk_window_set_policy(GTK_WINDOW(w), FALSE, FALSE, FALSE);
	gtk_widget_set_usize(w, FB_WIDTH, FB_HEIGHT);
	gtk_window_set_title(GTK_WINDOW(w), "Select File");
#ifdef PROG_NAME
	gtk_window_set_wmclass(
	    GTK_WINDOW(w), "fileselection", PROG_NAME
	);
#endif
	gtk_widget_realize(w);
	window = w->window;
	if(window != NULL)
	{
	    GdkGeometry geo;
	    gdk_window_set_decorations(
		window,
		GDK_DECOR_BORDER | GDK_DECOR_TITLE
	    );
	    gdk_window_set_functions(
		window,
		GDK_FUNC_MOVE | GDK_FUNC_CLOSE
	    );
	    geo.min_width = 100;
	    geo.min_height = 70;
	    geo.max_width = gdk_screen_width() - 10;
	    geo.max_height = gdk_screen_height() - 10;
	    geo.base_width = 0;
	    geo.base_height = 0;
	    geo.width_inc = 1;
	    geo.height_inc = 1;
	    geo.min_aspect = 1.3f;
	    geo.max_aspect = 1.3f;
	    gdk_window_set_geometry_hints(
		window, &geo,
		GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE |
		GDK_HINT_BASE_SIZE | GDK_HINT_RESIZE_INC
	    );
	}
	gtk_widget_add_events(
	    w,
	    GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK |
	    GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "delete_event",
	    GTK_SIGNAL_FUNC(FileBrowserDeleteEventCB), fb
	);
	gtk_container_border_width(GTK_CONTAINER(w), 0);
	gtk_window_add_accel_group(GTK_WINDOW(w), accelgrp);
	parent = w;

	/* Main GtkVBox */
	fb->main_vbox = w = gtk_vbox_new(FALSE, border_major);
	gtk_container_border_width(GTK_CONTAINER(w), border_major);
	gtk_container_add(GTK_CONTAINER(parent), w);
	gtk_widget_show(w);
	parent = w;


	w = gtk_hbox_new(FALSE, border_major);
	gtk_box_pack_start(GTK_BOX(parent), w, FALSE, FALSE, 0);
	gtk_widget_show(w);
	parent2 = w;


	/* GtkHBox for the location label and locations popup list */
	w = gtk_hbox_new(FALSE, border_minor);
	gtk_box_pack_start(GTK_BOX(parent2), w, TRUE, TRUE, 0);
	gtk_widget_show(w);
	parent3 = w;

	w = gtk_label_new(
#if defined(PROG_LANGUAGE_SPANISH)
"Ubicación:"
#elif defined(PROG_LANGUAGE_FRENCH)
"Emplacement:"
#elif defined(PROG_LANGUAGE_GERMAN)
"Ort:"
#elif defined(PROG_LANGUAGE_ITALIAN)
"Posizione:"
#elif defined(PROG_LANGUAGE_DUTCH)
"Plaats:"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"Localidade:"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Plassering:"
#else
"Location:"
#endif
	);
	gtk_label_set_justify(GTK_LABEL(w), GTK_JUSTIFY_RIGHT);
	gtk_box_pack_start(GTK_BOX(parent3), w, FALSE, FALSE, 0);
	gtk_widget_show(w);

	/* Locations Popup List Box */
	fb->location_pulistbox = w = PUListBoxNew(
	    -1, 20 + (2 * 3)
	);
	gtk_box_pack_start(GTK_BOX(parent3), w, TRUE, TRUE, 0);
	PUListBoxSetChangedCB(
	    w,
	    FileBrowserLocPUListChangedCB, fb
	);
	gtk_widget_show(w);


	/* Go To Parent */
	fb->goto_parent_btn = w = GUIButtonPixmap(
	    (guint8 **)icon_folder_parent_20x20_xpm
	);
	gtk_box_pack_start(GTK_BOX(parent2), w, FALSE, FALSE, 0);
	gtk_signal_connect(
	    GTK_OBJECT(w), "clicked",
	    GTK_SIGNAL_FUNC(FileBrowserGoToParentCB), fb
	);
#if 0
	gtk_accel_group_add(
	    accelgrp, GDK_BackSpace, 0, GTK_ACCEL_VISIBLE,
	    GTK_OBJECT(w), "clicked"
	);
#endif
	GUISetWidgetTip(
	    w,
#if defined(PROG_LANGUAGE_SPANISH)
"El Padre"
#elif defined(PROG_LANGUAGE_FRENCH)
"Le Parent"
#elif defined(PROG_LANGUAGE_GERMAN)
"Elternteil"
#elif defined(PROG_LANGUAGE_ITALIAN)
"Il Genitore"
#elif defined(PROG_LANGUAGE_DUTCH)
"Ouder"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"O Pai"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Mor Eller Far"
#else
"Parent"
#endif
	);
	gtk_widget_show(w);

	/* New Directory */
	fb->new_directory_btn = w = GUIButtonPixmap(
	    (guint8 **)icon_folder_closed_20x20_xpm
	);
	gtk_box_pack_start(GTK_BOX(parent2), w, FALSE, FALSE, 0);
	gtk_signal_connect(
	    GTK_OBJECT(w), "clicked",
	    GTK_SIGNAL_FUNC(FileBrowserNewDirectoryCB), fb
	);
#if 0
	gtk_accel_group_add(
	    accelgrp, GDK_Insert, 0, GTK_ACCEL_VISIBLE,
	    GTK_OBJECT(w), "clicked"
	);
#endif
	GUISetWidgetTip(
	    w,
#if defined(PROG_LANGUAGE_SPANISH)
"La Guía Nueva"
#elif defined(PROG_LANGUAGE_FRENCH)
"Nouvel Annuaire"
#elif defined(PROG_LANGUAGE_GERMAN)
"Neues Verzeichnis"
#elif defined(PROG_LANGUAGE_ITALIAN)
"L'Elenco Nuovo"
#elif defined(PROG_LANGUAGE_DUTCH)
"Nieuwe Gids"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"Novo Guia"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Ny Directory"
#else
"New Directory"
#endif
	);
	gtk_widget_show(w);

	/* Rename */
	fb->rename_btn = w = GUIButtonPixmap(
	    (guint8 **)icon_rename_20x20_xpm
	);
	gtk_box_pack_start(GTK_BOX(parent2), w, FALSE, FALSE, 0);
	gtk_signal_connect(
	    GTK_OBJECT(w), "clicked",
	    GTK_SIGNAL_FUNC(FileBrowserRenameCB), fb
	);
#if 0
	gtk_accel_group_add(
	    accelgrp, GDK_F2, 0, GTK_ACCEL_VISIBLE,
	    GTK_OBJECT(w), "clicked"
	);
#endif
	GUISetWidgetTip(w,
#if defined(PROG_LANGUAGE_SPANISH)
"Reagrupe"
#elif defined(PROG_LANGUAGE_FRENCH)
"Renommer"
#elif defined(PROG_LANGUAGE_GERMAN)
"Benennen Sie Um"
#elif defined(PROG_LANGUAGE_ITALIAN)
"Rinominare"
#elif defined(PROG_LANGUAGE_DUTCH)
"Herdoop"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"Mude Nome"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Ombenevn"
#else
"Rename"
#endif
	);
	gtk_widget_show(w);

	/* Refresh */
	fb->refresh_btn = w = GUIButtonPixmap(
	    (guint8 **)icon_reload_20x20_xpm
	);
	gtk_box_pack_start(GTK_BOX(parent2), w, FALSE, FALSE, 0);
	gtk_signal_connect(
	    GTK_OBJECT(w), "clicked",
	    GTK_SIGNAL_FUNC(FileBrowserRefreshCB), fb
	);
	GUISetWidgetTip(
	    w,
#if defined(PROG_LANGUAGE_SPANISH)
"Refresque"
#elif defined(PROG_LANGUAGE_FRENCH)
"Rafraîchir"
#elif defined(PROG_LANGUAGE_GERMAN)
"Erfrischen Sie"
#elif defined(PROG_LANGUAGE_ITALIAN)
"Rinfrescare"
#elif defined(PROG_LANGUAGE_DUTCH)
"Verfris"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"Refresque-se"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Forfrisk"
#else
"Refresh"
#endif
	);
	gtk_widget_show(w);

	/* Show Hidden Objects GtkToggleButton */
	fb->show_hidden_objects_tb = w = GUIToggleButtonPixmap(
	    (guint8 **)icon_file_hidden_20x20_xpm
	);
	gtk_widget_set_usize(w, FB_TB_BTN_WIDTH, FB_TB_BTN_HEIGHT);
	gtk_box_pack_start(GTK_BOX(parent2), w, FALSE, FALSE, 0);
	gtk_signal_connect(
	    GTK_OBJECT(w), "toggled",
	    GTK_SIGNAL_FUNC(FileBrowserShowHiddenObjectsToggleCB), fb
	);
	if(fb->flags & FB_SHOW_HIDDEN_OBJECTS)
	    GTK_TOGGLE_BUTTON_SET_ACTIVE(w, TRUE);
	GUISetWidgetTip(
	    w,
"Show Hidden Objects"
	);
	gtk_widget_show(w);

	/* List Format GtkHBox */
	w = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(parent2), w, FALSE, FALSE, 0);
	gtk_widget_show(w);
	parent3 = w;

	/* List Format Standard GtkToggleButton */
	fb->list_format_standard_tb = w = GUIToggleButtonPixmap(
	    (guint8 **)icon_fb_list_standard_20x20_xpm
	);
	gtk_widget_set_usize(w, FB_TB_BTN_WIDTH, FB_TB_BTN_HEIGHT);
	gtk_box_pack_start(GTK_BOX(parent3), w, FALSE, FALSE, 0);
	gtk_signal_connect(
	    GTK_OBJECT(w), "toggled",
	    GTK_SIGNAL_FUNC(FileBrowserListFormatToggleCB), fb
	);
	GUISetWidgetTip(
	    w,
#if defined(PROG_LANGUAGE_SPANISH)
"Listar De Estándar"
#elif defined(PROG_LANGUAGE_FRENCH)
"Enumérer De Norme"
#elif defined(PROG_LANGUAGE_GERMAN)
"Standard Aufführen"
#elif defined(PROG_LANGUAGE_ITALIAN)
"Elencare Di Norma"
#elif defined(PROG_LANGUAGE_DUTCH)
"Standaard Opsommen"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"O Alistamento De Padrão"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Normal Listing"
#else
"Standard Listing"
#endif
	);
	gtk_widget_show(w);

	/* List Format Vertical GtkToggleButton */
	fb->list_format_vertical_tb = w = GUIToggleButtonPixmap(
	    (guint8 **)icon_fb_list_vertical_20x20_xpm
	);
	gtk_widget_set_usize(w, FB_TB_BTN_WIDTH, FB_TB_BTN_HEIGHT);
	gtk_box_pack_start(GTK_BOX(parent3), w, FALSE, FALSE, 0);
	gtk_signal_connect(
	    GTK_OBJECT(w), "toggled",
	    GTK_SIGNAL_FUNC(FileBrowserListFormatToggleCB), fb
	);
	GUISetWidgetTip(
	    w,
#if defined(PROG_LANGUAGE_SPANISH)
"Listar Detallado"
#elif defined(PROG_LANGUAGE_FRENCH)
"Enumérer Détaillé"
#elif defined(PROG_LANGUAGE_GERMAN)
"Ausführliches Aufführen"
#elif defined(PROG_LANGUAGE_ITALIAN)
"Elencare Dettagliato"
#elif defined(PROG_LANGUAGE_DUTCH)
"Gedetailleerd Opsommen"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"O Alistamento De Detailed"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Detaljert Listing"
#else
"Detailed Listing"
#endif
	);
/* Don't offer the use of the vertical list anymore */
/*	gtk_widget_show(w); */

	/* List Format Vertical Details GtkToggleButton */
	fb->list_format_vertical_details_tb = w = GUIToggleButtonPixmap(
	    (guint8 **)icon_fb_list_vertical_details_20x20_xpm
	);
	gtk_widget_set_usize(w, FB_TB_BTN_WIDTH, FB_TB_BTN_HEIGHT);
	gtk_box_pack_start(GTK_BOX(parent3), w, FALSE, FALSE, 0);
	gtk_signal_connect(
	    GTK_OBJECT(w), "toggled",
	    GTK_SIGNAL_FUNC(FileBrowserListFormatToggleCB), fb
	);
	GUISetWidgetTip(
	    w,
#if defined(PROG_LANGUAGE_SPANISH)
"Listar Detallado"
#elif defined(PROG_LANGUAGE_FRENCH)
"Enumérer Détaillé"
#elif defined(PROG_LANGUAGE_GERMAN)
"Ausführliches Aufführen"
#elif defined(PROG_LANGUAGE_ITALIAN)
"Elencare Dettagliato"
#elif defined(PROG_LANGUAGE_DUTCH)
"Gedetailleerd Opsommen"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"O Alistamento De Detailed"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Detaljert Listing"
#else
"Detailed Listing"
#endif
	);
	gtk_widget_show(w);



	/* Table for list */
	w = gtk_table_new(2, 2, FALSE);
	gtk_table_set_row_spacing(GTK_TABLE(w), 0, border_minor);
	gtk_table_set_col_spacing(GTK_TABLE(w), 0, border_minor);
	gtk_box_pack_start(GTK_BOX(parent), w, TRUE, TRUE, 0);
	gtk_widget_show(w);
	parent2 = w;

	/* List Header GtkFrame */
	w = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(w), GTK_SHADOW_IN);
	gtk_table_attach(
	    GTK_TABLE(parent2), w,
	    0, 1, 0, 1,
	    GTK_FILL | GTK_SHRINK | GTK_EXPAND,
	    GTK_FILL | GTK_SHRINK | GTK_EXPAND,
	    0, 0
	);
	gtk_widget_show(w);
	parent3 = w;

	/* List Header GtkHBox */
	w = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(parent3), w);
	gtk_widget_show(w);
	parent3 = w;

	/* List Header GtkDrawingArea */
	fb->list_header_da = w = gtk_drawing_area_new();
	gtk_widget_set_usize(w, FB_LIST_HEADER_WIDTH, FB_LIST_HEADER_HEIGHT);
	gtk_widget_add_events(
	    w,
	    GDK_STRUCTURE_MASK | GDK_EXPOSURE_MASK |
	    GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK |
	    GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
	    GDK_POINTER_MOTION_MASK |
	    GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "configure_event",
	    GTK_SIGNAL_FUNC(FileBrowserListHeaderEventCB), fb
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "expose_event",
	    GTK_SIGNAL_FUNC(FileBrowserListHeaderEventCB), fb
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "button_press_event",
	    GTK_SIGNAL_FUNC(FileBrowserListHeaderEventCB), fb
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "button_release_event",
	    GTK_SIGNAL_FUNC(FileBrowserListHeaderEventCB), fb
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "motion_notify_event",
	    GTK_SIGNAL_FUNC(FileBrowserListHeaderEventCB), fb
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "enter_notify_event",
	    GTK_SIGNAL_FUNC(FileBrowserListHeaderEventCB), fb
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "leave_notify_event",
	    GTK_SIGNAL_FUNC(FileBrowserListHeaderEventCB), fb
	);
	gtk_box_pack_start(GTK_BOX(parent3), w, FALSE, FALSE, 0);
	gtk_widget_show(w);

	/* List GtkDrawingArea */
	fb->list_da = w = gtk_drawing_area_new();
	gtk_widget_set_usize(w, FB_LIST_WIDTH, FB_LIST_HEIGHT);
	GTK_WIDGET_SET_FLAGS(w, GTK_CAN_FOCUS);
	gtk_widget_add_events(
	    w,
	    GDK_STRUCTURE_MASK | GDK_EXPOSURE_MASK |
	    GDK_FOCUS_CHANGE_MASK |
	    GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK |
	    GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
	    GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK |
	    GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "realize",
	    GTK_SIGNAL_FUNC(FileBrowserListRealizeCB), fb
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "configure_event",
	    GTK_SIGNAL_FUNC(FileBrowserListEventCB), fb
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "expose_event",
	    GTK_SIGNAL_FUNC(FileBrowserListEventCB), fb
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "focus_in_event",
	    GTK_SIGNAL_FUNC(FileBrowserListEventCB), fb
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "focus_out_event",
	    GTK_SIGNAL_FUNC(FileBrowserListEventCB), fb
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "key_press_event",
	    GTK_SIGNAL_FUNC(FileBrowserListEventCB), fb
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "key_release_event",
	    GTK_SIGNAL_FUNC(FileBrowserListEventCB), fb
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "button_press_event",
	    GTK_SIGNAL_FUNC(FileBrowserListEventCB), fb
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "button_release_event",
	    GTK_SIGNAL_FUNC(FileBrowserListEventCB), fb
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "motion_notify_event",
	    GTK_SIGNAL_FUNC(FileBrowserListEventCB), fb
	);
	gtk_box_pack_start(GTK_BOX(parent3), w, TRUE, TRUE, 0);
	gtk_widget_show(w);
	if(w != NULL)
	{
	    const GtkTargetEntry dnd_tar_types[] = {
{GUI_TARGET_NAME_TEXT_PLAIN,	0,		0},
{GUI_TARGET_NAME_TEXT_URI_LIST,	0,		1},
{GUI_TARGET_NAME_STRING,	0,		2}
	    };
	    const GtkTargetEntry dnd_src_types[] = {
{GUI_TARGET_NAME_TEXT_PLAIN,	0,		0},
{GUI_TARGET_NAME_TEXT_URI_LIST,	0,		1},
{GUI_TARGET_NAME_STRING,	0,		2}
	    };
	    gtk_signal_connect_after(
		GTK_OBJECT(w), "drag_leave",
		GTK_SIGNAL_FUNC(FileBrowserListDragLeaveCB), fb
	    );
	    gtk_signal_connect_after(
		GTK_OBJECT(w), "drag_motion",
		GTK_SIGNAL_FUNC(FileBrowserListDragMotionCB), fb
	    );
	    GUIDNDSetTar(
		w,
		dnd_tar_types,
		sizeof(dnd_tar_types) / sizeof(GtkTargetEntry),
		GDK_ACTION_COPY | GDK_ACTION_MOVE |
		    GDK_ACTION_LINK,		/* Allowed actions */
		GDK_ACTION_MOVE,		/* Default action if same */
		GDK_ACTION_MOVE,		/* Default action */
		FileBrowserDragDataReceivedCB,
		fb,
		FALSE				/* Do not highlight */
	    );
	    GUIDNDSetSrc(
		w,
		dnd_src_types,
		sizeof(dnd_src_types) / sizeof(GtkTargetEntry),
		GDK_ACTION_COPY | GDK_ACTION_MOVE |
		    GDK_ACTION_LINK,		/* Allowed actions */
		GDK_BUTTON1_MASK,		/* Buttons */
		FileBrowserListDragBeginCB,
		FileBrowserListDragDataGetCB,
		FileBrowserListDragDataDeleteCB,
		FileBrowserListDragEndCB,
		fb
	    );
	}

	/* List vertical scrollbar */
	adj = (GtkAdjustment *)gtk_adjustment_new(
	    0.0f, 0.0f, 1.0f,
	    0.0f, 0.0f, 1.0f
	);
	fb->list_vsb = w = gtk_vscrollbar_new(adj);
	gtk_table_attach(
	    GTK_TABLE(parent2), w,
	    1, 2, 0, 1,
	    0,
	    GTK_FILL | GTK_SHRINK | GTK_EXPAND,
	    0, 0
	);
	gtk_signal_connect(
	    GTK_OBJECT(adj), "value_changed",
	    GTK_SIGNAL_FUNC(FileBrowserListScrollCB), fb
	);

	/* List's horizontal GtkScrollBar */
	adj = (GtkAdjustment *)gtk_adjustment_new(
	    0.0f, 0.0f, 1.0f,
	    0.0f, 0.0f, 1.0f
	);
	fb->list_hsb = w = gtk_hscrollbar_new(adj);
	gtk_table_attach(
	    GTK_TABLE(parent2), w,
	    0, 1, 1, 2,
	    GTK_FILL | GTK_SHRINK | GTK_EXPAND,
	    0,
	    0, 0
	);
	gtk_signal_connect(
	    GTK_OBJECT(adj), "value_changed",
	    GTK_SIGNAL_FUNC(FileBrowserListScrollCB), fb
	);

	/* List Right Click GtkMenu */
	if(TRUE)
	{
	    guint8 **icon;
	    const gchar *label;
	    guint accel_key, accel_mods;
	    gpointer data = fb;
	    void (*func_cb)(GtkWidget *w, gpointer);
	    GtkWidget *menu = (GtkWidget *)GUIMenuCreate();

#define ADD_MENU_ITEM_LABEL	{		\
 w = GUIMenuItemCreate(				\
  menu,						\
  GUI_MENU_ITEM_TYPE_LABEL,			\
  accelgrp,					\
  icon, label,					\
  accel_key, accel_mods,			\
  NULL,						\
  data, func_cb					\
 );						\
}
#define ADD_MENU_ITEM_CHECK	{		\
 w = GUIMenuItemCreate(				\
  menu,						\
  GUI_MENU_ITEM_TYPE_CHECK,			\
  accelgrp,					\
  icon, label,					\
  accel_key, accel_mods,			\
  NULL,						\
  data, func_cb					\
 );						\
}
#define ADD_MENU_ITEM_SEPARATOR	{		\
 w = GUIMenuItemCreate(				\
  menu,						\
  GUI_MENU_ITEM_TYPE_SEPARATOR,			\
  NULL,						\
  NULL, NULL,					\
  0, 0,						\
  NULL,						\
  NULL, NULL					\
 );						\
}

	    icon = (guint8 **)icon_select_20x20_xpm;
	    label =
#if defined(PROG_LANGUAGE_SPANISH)
"Selecto"
#elif defined(PROG_LANGUAGE_FRENCH)
"Choisir"
#elif defined(PROG_LANGUAGE_GERMAN)
"Erlesen"
#elif defined(PROG_LANGUAGE_ITALIAN)
"Scegliere"
#elif defined(PROG_LANGUAGE_DUTCH)
"Uitgezocht"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"Selecione"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Utvalgt"
#else
"Select"
#endif
	    ;
	    accel_key = GDK_Return;
	    accel_mods = 0;
	    func_cb = FileBrowserEntryActivateCB;
	    ADD_MENU_ITEM_LABEL
/*	    fb->select_mi = w; */

	    ADD_MENU_ITEM_SEPARATOR

	    icon = (guint8 **)icon_folder_parent_20x20_xpm;
	    label =
#if defined(PROG_LANGUAGE_SPANISH)
"El Padre"
#elif defined(PROG_LANGUAGE_FRENCH)
"Le Parent"
#elif defined(PROG_LANGUAGE_GERMAN)
"Elternteil"
#elif defined(PROG_LANGUAGE_ITALIAN)
"Il Genitore"
#elif defined(PROG_LANGUAGE_DUTCH)
"Ouder"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"O Pai"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Mor Eller Far"
#else
"Go To Parent"
#endif
	    ;
	    accel_key = GDK_BackSpace;
	    accel_mods = 0;
	    func_cb = FileBrowserGoToParentCB;
	    ADD_MENU_ITEM_LABEL
/*	    fb->goto_parent_mi = w; */

	    ADD_MENU_ITEM_SEPARATOR

	    icon = (guint8 **)icon_folder_closed_20x20_xpm;
	    label =
#if defined(PROG_LANGUAGE_SPANISH)
"La Guía Nueva"
#elif defined(PROG_LANGUAGE_FRENCH)
"Nouvel Annuaire"
#elif defined(PROG_LANGUAGE_GERMAN)
"Neues Verzeichnis"
#elif defined(PROG_LANGUAGE_ITALIAN)
"L'Elenco Nuovo"
#elif defined(PROG_LANGUAGE_DUTCH)
"Nieuwe Gids"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"Novo Guia"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Ny Directory"
#else
"New Directory"
#endif
	    ;
	    accel_key = GDK_n;
	    accel_mods = GDK_CONTROL_MASK;
	    func_cb = FileBrowserNewDirectoryCB;
	    ADD_MENU_ITEM_LABEL
/*	    fb->new_directory_mi = w; */

	    icon = (guint8 **)icon_rename_20x20_xpm;
	    label =
#if defined(PROG_LANGUAGE_SPANISH)
"Reagrupe"
#elif defined(PROG_LANGUAGE_FRENCH)
"Renommer"
#elif defined(PROG_LANGUAGE_GERMAN)
"Benennen Sie Um"
#elif defined(PROG_LANGUAGE_ITALIAN)
"Rinominare"
#elif defined(PROG_LANGUAGE_DUTCH)
"Herdoop"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"Mude Nome"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Ombenevn"
#else
"Rename"
#endif
	    ;
	    accel_key = GDK_F2;
	    accel_mods = 0;
	    func_cb = FileBrowserRenameCB;
	    ADD_MENU_ITEM_LABEL
/*	    fb->rename_mi = w; */

	    icon = (guint8 **)icon_chmod_20x20_xpm;
	    label =
#if defined(PROG_LANGUAGE_SPANISH)
"Cambie Los Permisos"
#elif defined(PROG_LANGUAGE_FRENCH)
"Changer Des Permissions"
#elif defined(PROG_LANGUAGE_GERMAN)
"Ändern Sie Erlaubnis"
#elif defined(PROG_LANGUAGE_ITALIAN)
"Cambiare I Permessi"
#elif defined(PROG_LANGUAGE_DUTCH)
"Verandeer Toestemmingen"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"Mude Permissões"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Forandr Permissions"
#else
"Change Permissions"
#endif
	    ;
	    accel_key = GDK_F9;
	    accel_mods = 0;
	    func_cb = FileBrowserCHModCB;
	    ADD_MENU_ITEM_LABEL
/*	    fb->chmod_mi = w; */

	    icon = (guint8 **)icon_cancel_20x20_xpm;
	    label =
#if defined(PROG_LANGUAGE_SPANISH)
"Borre"
#elif defined(PROG_LANGUAGE_FRENCH)
"Effacer"
#elif defined(PROG_LANGUAGE_GERMAN)
"Löschen Sie"
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
	    accel_key = GDK_Delete;
	    accel_mods = 0;
	    func_cb = FileBrowserDeleteCB;
	    ADD_MENU_ITEM_LABEL
/*	    fb->delete_mi = w; */

	    ADD_MENU_ITEM_SEPARATOR

	    icon = NULL;
	    label =
#if defined(PROG_LANGUAGE_SPANISH)
"Escoja Todo"
#elif defined(PROG_LANGUAGE_FRENCH)
"Choisir Tout"
#elif defined(PROG_LANGUAGE_GERMAN)
"Wählen Sie Alle Aus"
#elif defined(PROG_LANGUAGE_ITALIAN)
"Scegliere Tutto"
#elif defined(PROG_LANGUAGE_DUTCH)
"Selecteer Alle"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"Selecione Todo"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Utvalgt All"
#else
"Select All"
#endif
	    ;
	    accel_key = GDK_a;
	    accel_mods = GDK_CONTROL_MASK;
	    func_cb = FileBrowserSelectAllCB;
	    ADD_MENU_ITEM_LABEL
/*	    fb->select_all_mi = w; */

	    icon = NULL;
	    label =
#if defined(PROG_LANGUAGE_SPANISH)
"Rechace Todo"
#elif defined(PROG_LANGUAGE_FRENCH)
"Déssélectionner Tout"
#elif defined(PROG_LANGUAGE_GERMAN)
"Entmarkieren Sie Alle"
#elif defined(PROG_LANGUAGE_ITALIAN)
"Deselezionare Tutto"
#elif defined(PROG_LANGUAGE_DUTCH)
"Heldere Keuze"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"O Deselect Todo"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Klar Selection"
#else
"Unselect All"
#endif
	    ;
	    accel_key = GDK_u;
	    accel_mods = GDK_CONTROL_MASK;
	    func_cb = FileBrowserUnselectAllCB;
	    ADD_MENU_ITEM_LABEL
/*	    fb->unselect_all_mi = w; */

	    icon = NULL;
	    label =
#if defined(PROG_LANGUAGE_SPANISH)
"Invierta La Selección"
#elif defined(PROG_LANGUAGE_FRENCH)
"Inverser La Sélection"
#elif defined(PROG_LANGUAGE_GERMAN)
"Invertieren Sie Auswahl"
#elif defined(PROG_LANGUAGE_ITALIAN)
"Invertire La Selezione"
#elif defined(PROG_LANGUAGE_DUTCH)
"Keer Keuze Om"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"Inverta Seleção"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Inverter Selection"
#else
"Invert Selection"
#endif
	    ;
	    accel_key = GDK_i;
	    accel_mods = GDK_CONTROL_MASK;
	    func_cb = FileBrowserInvertSelectionCB;
	    ADD_MENU_ITEM_LABEL
/*	    fb->invert_selection_mi = w; */

	    ADD_MENU_ITEM_SEPARATOR

	    icon = (guint8 **)icon_reload_20x20_xpm;
	    label =
#if defined(PROG_LANGUAGE_SPANISH)
"Refresque"
#elif defined(PROG_LANGUAGE_FRENCH)
"Rafraîchir"
#elif defined(PROG_LANGUAGE_GERMAN)
"Erfrischen Sie"
#elif defined(PROG_LANGUAGE_ITALIAN)
"Rinfrescare"
#elif defined(PROG_LANGUAGE_DUTCH)
"Verfris"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"Refresque-se"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Forfrisk"
#else
"Refresh"
#endif
	    ;
	    accel_key = GDK_F5;
	    accel_mods = 0;
	    func_cb = FileBrowserRefreshCB;
	    ADD_MENU_ITEM_LABEL
/*	    fb->refresh_mi = w; */

	    ADD_MENU_ITEM_SEPARATOR

	    icon = NULL;
	    label = "Show Hidden Objects";
	    accel_key = GDK_h;
	    accel_mods = GDK_CONTROL_MASK;
	    func_cb = FileBrowserShowHiddenObjectsToggleCB;
	    ADD_MENU_ITEM_CHECK
	    fb->show_hidden_objects_micheck = w;
	    if(fb->flags & FB_SHOW_HIDDEN_OBJECTS)
		GTK_CHECK_MENU_ITEM_SET_ACTIVE(w, TRUE);

	    ADD_MENU_ITEM_SEPARATOR

	    icon = NULL;
	    label = "Standard Listing";
	    accel_key = GDK_1;
	    accel_mods = GDK_CONTROL_MASK;
	    func_cb = FileBrowserListFormatToggleCB;
	    ADD_MENU_ITEM_CHECK
	    fb->list_format_standard_micheck = w;

	    icon = NULL;
	    label = "Vertical Listing";
	    accel_key = 0;
	    accel_mods = 0;
	    func_cb = FileBrowserListFormatToggleCB;
	    ADD_MENU_ITEM_CHECK
	    fb->list_format_vertical_micheck = w;
	    gtk_widget_hide(w);			/* Disable */

	    icon = NULL;
	    label = "Detailed Listing";
	    accel_key = GDK_2;
	    accel_mods = GDK_CONTROL_MASK;
	    func_cb = FileBrowserListFormatToggleCB;
	    ADD_MENU_ITEM_CHECK
	    fb->list_format_vertical_details_micheck = w;


	    fb->list_menu = menu;

#undef ADD_MENU_ITEM_LABEL
#undef ADD_MENU_ITEM_CHECK
#undef ADD_MENU_ITEM_SEPARATOR
	}


	w = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(parent), w, FALSE, FALSE, 0);
	gtk_widget_show(w);


	w = gtk_hbox_new(FALSE, border_major);
	gtk_box_pack_start(GTK_BOX(parent), w, FALSE, FALSE, 0);
	gtk_widget_show(w);
	parent2 = w;

	/* GtkVBox for the name and type */
	w = gtk_vbox_new(FALSE, border_major);
	gtk_box_pack_start(GTK_BOX(parent2), w, TRUE, TRUE, 0);
	gtk_widget_show(w);
	parent3 = w;

	/* GtkHBox for the name */
	w = gtk_hbox_new(FALSE, border_minor);
	gtk_box_pack_start(GTK_BOX(parent3), w, FALSE, FALSE, 0);
	gtk_widget_show(w);
	parent4 = w;

	w = gtk_label_new(
#if defined(PROG_LANGUAGE_SPANISH)
"Archive Nombre:"
#elif defined(PROG_LANGUAGE_FRENCH)
"Nom De Fichier:"
#elif defined(PROG_LANGUAGE_GERMAN)
"Dateiname:"
#elif defined(PROG_LANGUAGE_ITALIAN)
"Schedare Nome:"
#elif defined(PROG_LANGUAGE_DUTCH)
"Archiveer Naam:"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"Arquive Nome:"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Arkiver Name:"
#else
"File Name:"
#endif
	);
	gtk_label_set_justify(GTK_LABEL(w), GTK_JUSTIFY_RIGHT);
	gtk_box_pack_start(GTK_BOX(parent4), w, FALSE, FALSE, 0);
	gtk_widget_show(w);

	fb->entry = w = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(parent4), w, TRUE, TRUE, 0);
	gtk_signal_connect(
	    GTK_OBJECT(w), "key_press_event",
	    GTK_SIGNAL_FUNC(FileBrowserEntryCompletePathKeyCB), fb
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "key_release_event",
	    GTK_SIGNAL_FUNC(FileBrowserEntryCompletePathKeyCB), fb
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "activate",
	    GTK_SIGNAL_FUNC(FileBrowserEntryActivateCB), fb
	);
	GUIEditableEndowPopupMenu(w, 0);
	gtk_widget_show(w);
	GUISetWidgetTip(
	    w,
#if defined(PROG_LANGUAGE_SPANISH)
"Entre el nombre del objeto, usted puede especificar más\
 que un objeto (separa cada nombre con un ',' el carácter)"
#elif defined(PROG_LANGUAGE_FRENCH)
"Entrer le nom de l'objet, vous pouvez spécifier plus\
 qu'un objet (sépare chaque nom avec un ',' le caractère)"
#elif defined(PROG_LANGUAGE_GERMAN)
"Tragen Sie den Namen des Objekts, Sie können angeben\
 mehr als ein Objekt ein (trennen Sie jeden Namen mit einem ',' charakter)"
#elif defined(PROG_LANGUAGE_ITALIAN)
"Entrare il nome dell'oggetto, lei può specificare più\
 di un oggetto (separa ogni nome con un ',' il carattere)"
#elif defined(PROG_LANGUAGE_DUTCH)
"Ga de naam van het voorwerp, u zou kunnen specificeren\
 meer than een voorwerp binnen (scheid elk naam met een ',' teken)"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"Entre o nome do objeto, você pode especificar mais de\
 um objeto (separa cada nome com um ',' caráter)"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Gå inn i navnet av objektet, De spesifiserer mere enn\
 et objekt (separere hver navn med et ',' karakter)"
#else
"Enter the name of the object, you may specify more than\
 one object (separate each name with a ',' character)"
#endif
	);
	if(w != NULL)
	{
	    const GtkTargetEntry dnd_tar_types[] = {
{GUI_TARGET_NAME_TEXT_PLAIN,	0,		0},
{GUI_TARGET_NAME_TEXT_URI_LIST,	0,		1},
{GUI_TARGET_NAME_STRING,	0,		2}
	    };
	    GUIDNDSetTar(
		w,
		dnd_tar_types,
		sizeof(dnd_tar_types) / sizeof(GtkTargetEntry),
		GDK_ACTION_COPY,		/* Actions */
		GDK_ACTION_COPY,		/* Default action if same */
		GDK_ACTION_COPY,		/* Default action */
		FileBrowserDragDataReceivedCB,
		fb,
		TRUE				/* Highlight */
	    );
	}

	/* GtkHBox for the type */
	w = gtk_hbox_new(FALSE, border_minor);	
	gtk_box_pack_start(GTK_BOX(parent3), w, FALSE, FALSE, 0);
	gtk_widget_show(w);
	parent4 = w;

	w = gtk_label_new(
#if defined(PROG_LANGUAGE_SPANISH)
"Tipo Archivo:"
#elif defined(PROG_LANGUAGE_FRENCH)
"Type De Fichier:"
#elif defined(PROG_LANGUAGE_GERMAN)
"Legt Typ Ab:"
#elif defined(PROG_LANGUAGE_ITALIAN)
"Tipo Di File:"
#elif defined(PROG_LANGUAGE_DUTCH)
"Archiveer Type:"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"Arquivo Tipo:"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Typen Arkivet:"
#else
"File Type:"
#endif
	);
	gtk_box_pack_start(GTK_BOX(parent4), w, FALSE, FALSE, 0);
	gtk_widget_show(w);

	/* Types Popup List Box */
	fb->types_pulistbox = w = PUListBoxNew(
	    -1, -1
	);
	gtk_box_pack_start(GTK_BOX(parent4), w, TRUE, TRUE, 0);
	PUListBoxSetChangedCB(
	    w,
	    FileBrowserTypesPUListChangedCB, fb
	);
	gtk_widget_show(w);
	pulist = PUListBoxGetPUList(w);
	i = PUListAddItem(pulist, FB_DEFAULT_TYPE_STR);
	if(i > -1)
	{
	    fb_type_struct *t = FB_TYPE(g_malloc0(sizeof(fb_type_struct)));
	    if(t != NULL)
	    {
		t->ext = STRDUP("*.*");
		t->name = STRDUP("All Files");
		PUListSetItemDataFull(
		    pulist, i,
		    t, FileBrowserTypesPUListItemDestroyCB
		);
	    }
	    PUListBoxSetLinesVisible(w, 1);
	    PUListBoxSelect(w, i);
	    FileBrowserTypesPUListChangedCB(w, i, fb);
	}

	/* Vbox for ok and cancel buttons */
	w = gtk_vbox_new(TRUE, border_minor);
	gtk_box_pack_start(GTK_BOX(parent2), w, FALSE, FALSE, 0);
	gtk_widget_show(w);
	parent3 = w;


	/* OK GtkButton */
	fb->ok_btn = w = GUIButtonPixmapLabelH(
	    (guint8 **)icon_ok_20x20_xpm, "OK", &fb->ok_btn_label
	);
	GTK_WIDGET_SET_FLAGS(w, GTK_CAN_DEFAULT);
	gtk_widget_set_usize(
	    w,
	    GUI_BUTTON_HLABEL_WIDTH_DEF,
	    GUI_BUTTON_HLABEL_HEIGHT_DEF
	);
	gtk_box_pack_start(GTK_BOX(parent3), w, TRUE, FALSE, 0);
	gtk_signal_connect(
	    GTK_OBJECT(w), "clicked",
	    GTK_SIGNAL_FUNC(FileBrowserOKCB), fb
	);
#if 0
	gtk_accel_group_add(
	    accelgrp, GDK_Return, 0, GTK_ACCEL_VISIBLE,
	    GTK_OBJECT(w), "clicked"
	);
	gtk_accel_group_add(
	    accelgrp, GDK_3270_Enter, 0, GTK_ACCEL_VISIBLE,
	    GTK_OBJECT(w), "clicked"
	);
	gtk_accel_group_add(
	    accelgrp, GDK_KP_Enter, 0, GTK_ACCEL_VISIBLE,
	    GTK_OBJECT(w), "clicked"
	);
	gtk_accel_group_add(
	    accelgrp, GDK_ISO_Enter, 0, GTK_ACCEL_VISIBLE,
	    GTK_OBJECT(w), "clicked"
	);
#endif
	gtk_widget_show(w);

	/* Cancel GtkButton */
	fb->cancel_btn = w = GUIButtonPixmapLabelH(
	    (guint8 **)icon_cancel_20x20_xpm, "Cancel", &fb->cancel_btn_label
	);
	GTK_WIDGET_SET_FLAGS(w, GTK_CAN_DEFAULT);
	gtk_widget_set_usize(
	    w,
	    GUI_BUTTON_HLABEL_WIDTH_DEF,
	    GUI_BUTTON_HLABEL_HEIGHT_DEF
	);
	gtk_box_pack_start(GTK_BOX(parent3), w, TRUE, FALSE, 0);
	gtk_signal_connect(
	    GTK_OBJECT(w), "clicked",
	    GTK_SIGNAL_FUNC(FileBrowserCancelCB), fb
	);
	gtk_accel_group_add(
	    accelgrp, GDK_Escape, 0, GTK_ACCEL_VISIBLE,
	    GTK_OBJECT(w), "clicked"
	);
	gtk_widget_show(w);


	/* Load the icons, the order is important because the index
	 * must match the FB_ICON_* values as indices
	 */
	if(fb->toplevel != NULL)
	{
	    gint i = 0;
	    gpointer icon_list[] = FB_ICON_DATA_LIST;
	    while(icon_list[i] != NULL)
	    {
		FileBrowserIconAppend(
		    fb,
		    (guint8 **)icon_list[i + 1],	/* XPM data */
		    (const gchar *)icon_list[i + 0]	/* Description */
		);
		i += 6;
	    }
	}

	/* Force the setting of the default list format and get
	 * listing
	 */
	fb->list_format = -1;
	FileBrowserSetListFormat(fb, FB_LIST_FORMAT_STANDARD);

	fb->freeze_count--;

	return(0);
}

/*
 *	Sets the File Browser's style.
 */
void FileBrowserSetStyle(GtkRcStyle *rc_style)
{
	GtkWidget *w;
	FileBrowser *fb = &file_browser;

	w = fb->toplevel;
	if(w != NULL)
	{
	    if(rc_style != NULL)
	    {
		gtk_widget_modify_style(w, rc_style);
	    }
	    else
	    {
		rc_style = gtk_rc_style_new();
		gtk_widget_modify_style_recursive(w, rc_style);
		GTK_RC_STYLE_UNREF(rc_style)
	    }
	}
}

/*
 *	Sets the File Browser to be a transient for the given
 *	GtkWindow w.
 *
 *	If w is NULL then transient for will be unset.
 */
void FileBrowserSetTransientFor(GtkWidget *w)
{
	FileBrowser *fb = &file_browser;
	if(fb->toplevel != NULL)
	{
	    GtkWidget *toplevel = fb->toplevel;

	    if(w != NULL)
	    {
		if(!GTK_IS_WINDOW(w))
		    return;

/* Since the file browser itself has popup windows that may need to be
 * set modal, we cannot set the file browser itself modal.
		gtk_window_set_modal(
		    GTK_WINDOW(toplevel), TRUE
		);
 */
		gtk_window_set_transient_for(
		    GTK_WINDOW(toplevel), GTK_WINDOW(w)
		);
	    }
	    else
	    {
/*
		gtk_window_set_modal(
		    GTK_WINDOW(toplevel), FALSE
		);
 */
		gtk_window_set_transient_for(
		    GTK_WINDOW(toplevel), NULL
		);
	    }
	}
}

/*
 *	Sets the object created callback.
 */
void FileBrowserSetObjectCreatedCB(
	void (*cb)(
		const gchar *path,
		gpointer data
	),
	gpointer data
)
{
	FileBrowser *fb = &file_browser;
	fb->object_created_cb = cb;
	fb->object_created_data = data;
}

/*
 *	Sets the object modified callback.
 */
void FileBrowserSetObjectModifiedCB(
	void (*cb)(
		const gchar *old_path,
		const gchar *new_path,
		gpointer data
	),
	gpointer data
)
{
	FileBrowser *fb = &file_browser;
	fb->object_modified_cb = cb;
	fb->object_modified_data = data;
}

/*
 *	Sets the object deleted callback.
 */
void FileBrowserSetObjectDeletedCB(
	void (*cb)(
		const gchar *path,
		gpointer data
	),
	gpointer data
)
{
	FileBrowser *fb = &file_browser;
	fb->object_deleted_cb = cb;
	fb->object_deleted_data = data;
}

/*
 *	Returns TRUE if currently blocking for query.
 */
gboolean FileBrowserIsQuery(void) 
{
	FileBrowser *fb = &file_browser;

	if(fb->main_level > 0)
	    return(TRUE);
	else
	    return(FALSE);
}

/*
 *	Ends query if any and returns a not available response.
 */
void FileBrowserBreakQuery(void)
{
	FileBrowser *fb = &file_browser;

	/* Clear the response OK flag */
	fb->flags &= ~FB_RESPONSE_OK;

	/* Break out of an additional blocking loops */
	while(fb->main_level > 0)
	{
	    gtk_main_quit();
	    fb->main_level--;
	}
	fb->main_level = 0;
}

/*
 *	Returns the File Browser's toplevel GtkWidget.
 */
GtkWidget *FileBrowserGetToplevel(void)
{
	FileBrowser *fb = &file_browser;
	return(fb->toplevel);
}

/*
 *	Clears the lists and unset the location.
 */
void FileBrowserReset(void)
{
	FileBrowser *fb = &file_browser;
	GtkEntry *entry;

	fb->freeze_count++;

#if 0
/* Do not reset the current location */
	/* Unset the current location */
	g_free(fb->cur_location);
	fb->cur_location = NULL;
#endif

	/* Unset the file name entry */
	entry = (GtkEntry *)fb->entry;
	if(entry != NULL)
	    gtk_entry_set_text(entry, "");


	/* Clear the locations popup list */
	PUListClear(PUListBoxGetPUList(fb->location_pulistbox));

	/* Clear the types popup list */
	PUListClear(PUListBoxGetPUList(fb->types_pulistbox));


	/* Clear the objects listing */

	/* Unselect all */
	fb->focus_object = -1;
	g_list_free(fb->selection);
	fb->selection = fb->selection_end = NULL;

	/* Delete all the objects */
	if(fb->objects_list != NULL)
	{
	    g_list_foreach(
		fb->objects_list, (GFunc)FileBrowserObjectDelete, NULL
	    );
	    g_list_free(fb->objects_list);
	    fb->objects_list = NULL;
	}

	/* Reset the number of objects per row */
	fb->objects_per_row = 0;

	fb->freeze_count--;
}

/*
 *	Maps the file browser and sets up the inital values.
 *
 *	Returns TRUE if a path was selected or FALSE if user canceled.
 *
 *	For most values that are set NULL, the value is left unchanged.
 *	All given values are coppied.
 *
 *	If type is set to NULL however, then the type list on the file
 *	browser will be left empty.
 *
 *	All returned pointer values should be considered statically
 *	allocated. The returned pointer for type_rtn may point to
 *	a structure in the input type list.
 */
gboolean FileBrowserGetResponse(
	const gchar *title,
	const gchar *ok_label,
	const gchar *cancel_label,
	const gchar *path,
	fb_type_struct **type, gint total_types,
	gchar ***path_rtn, gint *path_total_rtns,
	fb_type_struct **type_rtn
)
{
	GtkWidget *w, *toplevel;
	FileBrowser *fb = &file_browser;

#define RESET_RETURNS	{	\
 if(path_rtn != NULL)		\
  *path_rtn = NULL;		\
 if(path_total_rtns != NULL)	\
  *path_total_rtns = 0;		\
				\
 if(type_rtn != NULL)		\
  *type_rtn = NULL;		\
}

	/* Do not handle response if already waiting for a response,
	 * return with a not available response code
	 */
	if(fb->main_level > 0)
	{
	    RESET_RETURNS
	    return(FALSE);
	}


	/* Reset the values */
	fb->flags &= ~FB_RESPONSE_OK;

	g_free(fb->cur_type.name);
	g_free(fb->cur_type.ext);
	memset(&fb->cur_type, 0x00, sizeof(fb_type_struct));


	/* Reset the returns */
	RESET_RETURNS


	toplevel = fb->toplevel;

	/* Reget the drive paths */
	if(fb->device_paths_list != NULL)
	{
	    g_list_foreach(fb->device_paths_list, (GFunc)g_free, NULL);
	    g_list_free(fb->device_paths_list);
	}
	fb->device_paths_list = FileBrowserGetDrivePaths();

	/* Set title */
	if(title != NULL)
	    gtk_window_set_title(GTK_WINDOW(toplevel), title);

	/* Set OK Button label */
	w = fb->ok_btn_label;
	if((ok_label != NULL) && (w != NULL))
	    gtk_label_set_text(GTK_LABEL(w), ok_label);

	/* Set Cancel Button label */
	w = fb->cancel_btn_label;
	if((cancel_label != NULL) && (w != NULL))
	    gtk_label_set_text(GTK_LABEL(w), cancel_label);

	/* Set the types list */
	if((type != NULL) && (total_types > 0))
	    FileBrowserTypesListSetTypes(
		fb,
		type, total_types
	    );


	FileBrowserSetBusy(fb, TRUE);

	/* Map the File Browser
	 *
	 * This needs to be done now in order to allow the proper
	 * realizing of sizes before other things can be updated
	 *
	 * A configure_event signal will be emitted at which the
	 * correct sizes will be known when adding the objects to
	 * the objects list
	 */
	FileBrowserMap();

	/* No initial path specified? */
	if(STRISEMPTY(path))
	{
	    /* If no initial path was specified then we need to
	     * check if the list was previous cleared (reset), in
	     * which case we need to reget the listing
	     */
	    if(fb->objects_list == NULL)
	    {
		FileBrowserListUpdate(fb, NULL);
		FileBrowserLocationsPUListUpdate(fb);
	    }
	}
	else
	{
	    /* Set the initial location */
	    FileBrowserSetLocation(fb, path);
	}

	/* If no location was set then set the initial location as
	 * the home directory
	 */
	if(STRISEMPTY(fb->cur_location))
	    FileBrowserSetLocation(fb, "~");

	/* If there is a first object in the list then set it in focus */
	if(fb->objects_list != NULL)
	{
	    if(fb->focus_object < 0)
	    {
		fb->focus_object = 0;
		gtk_widget_queue_draw(fb->list_da);
	    }
	}

	FileBrowserSetBusy(fb, FALSE);

	/* Block until user response */
	fb->main_level++;
	gtk_main();

	/* Unmap the File Browser just in case it was not unmapped
	 * from any of the callbacks
	 */
	FileBrowserUnmap();

	/* Break out of an additional blocking loops */
	while(fb->main_level > 0)
	{
	    gtk_main_quit();
	    fb->main_level--;
	}
	fb->main_level = 0;


	/* Begin setting returns */

	/* Response path returns */
	if(path_rtn != NULL)
	    *path_rtn = fb->selected_path;
	if(path_total_rtns != NULL)
	    *path_total_rtns = fb->total_selected_paths;

	/* File type */
	if(type_rtn != NULL)
	    *type_rtn = &fb->cur_type;

	return((fb->flags & FB_RESPONSE_OK) ? TRUE : FALSE);
#undef RESET_RETURNS
}


/*
 *	Maps the File Browser.
 */
void FileBrowserMap(void)
{
	GtkWidget *w;
	FileBrowser *fb = &file_browser;

	gtk_widget_show_raise(fb->toplevel);
	fb->flags |= FB_MAPPED;

	w = fb->list_da;
	gtk_widget_grab_focus(w);
}

/*
 *	Unmaps the File Browser.
 */
void FileBrowserUnmap(void)
{
	FileBrowser *fb = &file_browser;
	gtk_widget_hide(fb->toplevel);
	fb->flags &= ~FB_MAPPED;
}

/*
 *	Shuts down the File Browser.
 */
void FileBrowserShutdown(void)
{
	gint i;
	FileBrowser *fb = &file_browser;

	/* Delete the current file type */
	g_free(fb->cur_type.name);
	g_free(fb->cur_type.ext);
	memset(&fb->cur_type, 0x00, sizeof(fb_type_struct));

	/* Delete the sSelected paths */
	for(i = 0; i < fb->total_selected_paths; i++)
	    g_free(fb->selected_path[i]);
	g_free(fb->selected_path);
	fb->selected_path = NULL;
	fb->total_selected_paths = 0;

	/* Delete the selected objects list */
	fb->focus_object = -1;
	g_list_free(fb->selection);
	fb->selection = fb->selection_end = NULL;

	/* Delete all the objects */
	if(fb->objects_list != NULL)
	{
	    g_list_foreach(
		fb->objects_list, (GFunc)FileBrowserObjectDelete, NULL
	    );
	    g_list_free(fb->objects_list);
	    fb->objects_list = NULL;
	}
	fb->objects_per_row = 0;

	/* Delete the icons */
	if(fb->icons_list != NULL)
	{
	    g_list_foreach(
		fb->icons_list, (GFunc)FileBrowserIconDelete, NULL
	    );
	    g_list_free(fb->icons_list);
	    fb->icons_list = NULL;
	}

	/* Delete the list columns */
	FileBrowserListColumnsClear(fb);

	/* Delete the device paths list */
	if(fb->device_paths_list != NULL)
	{
	    g_list_foreach(fb->device_paths_list, (GFunc)g_free, NULL);
	    g_list_free(fb->device_paths_list);
	}


	/* Break out of any remaining main levels */
	while(fb->main_level > 0)
	{
	    gtk_main_quit();
	    fb->main_level--;
	}
	fb->main_level = 0;


	fb->freeze_count++;

	/* Delete the dialog */
	gtk_widget_destroy(fb->list_menu);
	gtk_widget_destroy(fb->toplevel);
	gtk_accel_group_unref(fb->accelgrp);

	GDK_PIXMAP_UNREF(fb->list_pm);

	GDK_CURSOR_DESTROY(fb->cur_busy);
	GDK_CURSOR_DESTROY(fb->cur_column_hresize);
	GDK_CURSOR_DESTROY(fb->cur_translate);

	GDK_GC_UNREF(fb->gc);
	GDK_BITMAP_UNREF(fb->transparent_stipple_bm);

	g_free(fb->home_path);
	g_free(fb->cur_location);

	fb->freeze_count--;

	memset(fb, 0x00, sizeof(FileBrowser));
}


/*
 *	Gets the last location.
 *
 *	If no previous location was set then NULL will be returned.
 */
const gchar *FileBrowserGetLastLocation(void)
{
	return(FileBrowserGetLocation(&file_browser));
}

/*
 *	Show hidden objects.
 */
void FileBrowserShowHiddenObjects(const gboolean show)
{
	FileBrowser *fb = &file_browser;
	FileBrowserSetShowHiddenObjects(fb, show);
}

/*
 *	Sets the list format to standard.
 */
void FileBrowserListStandard(void)
{
	FileBrowser *fb = &file_browser;
	FileBrowserSetListFormat(fb, FB_LIST_FORMAT_STANDARD);
}

/*
 *	Sets the list format to detailed.
 */
void FileBrowserListDetailed(void)
{
	FileBrowser *fb = &file_browser;
	FileBrowserSetListFormat(fb, FB_LIST_FORMAT_VERTICAL_DETAILS);
}


/*
 *	Convience function to allocate a new file browser file extension
 *	type structure and append it to the given list. The given list
 *	will be modified and the index number of the newly allocated
 *	structure will be returned.
 *
 *	Can return -1 on error.
 */
gint FileBrowserTypeListNew(
	fb_type_struct ***list, gint *total,
	const gchar *ext,	/* Space separated list of extensions */
	const gchar *name	/* Descriptive name */
)
{
	gint n;
	fb_type_struct *fb_type_ptr;


	if((list == NULL) || (total == NULL))
	    return(-1);

	n = MAX(*total, 0);
	*total = n + 1;

	/* Allocate more pointers */
	*list = (fb_type_struct **)g_realloc(
	    *list,
	    (*total) * sizeof(fb_type_struct *)
	);
	if(*list == NULL)
	{
	    *total = 0;
	    return(-1);
	}

	/* Allocate new structure */
	fb_type_ptr = (fb_type_struct *)g_malloc0(sizeof(fb_type_struct));
	(*list)[n] = fb_type_ptr;
	if(fb_type_ptr == NULL)
	{
	    *total = n;
	    return(-1);
	}

	/* Set values */
	fb_type_ptr->ext = STRDUP(ext);
	fb_type_ptr->name = STRDUP(name);

	return(n);
}

/*
 *	Deletes the type.
 */
static void FileBrowserTypeDelete(fb_type_struct *t)
{
	if(t == NULL)
	    return;

	g_free(t->ext);
	g_free(t->name);
	g_free(t);
}

/*
 *	Deletes the types list.
 */
void FileBrowserDeleteTypeList(
	fb_type_struct **list, const gint total 
)
{
	gint i;

	for(i = 0; i < total; i++)
	    FileBrowserTypeDelete(list[i]);

	g_free(list);
}
