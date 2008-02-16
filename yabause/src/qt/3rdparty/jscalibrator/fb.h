/*
			      File Browser
 */

#ifndef FB_H
#define FB_H

#include <gtk/gtk.h>


typedef struct _fb_type_struct		fb_type_struct;
#define FB_TYPE(p)			((fb_type_struct *)(p))


/*
 *	File Type:
 */
struct _fb_type_struct {

	gchar		*ext,		/* Space separated list of extensions */
			*name;		/* Description of this file type */

};


extern gint FileBrowserInit(void);
extern void FileBrowserSetStyle(GtkRcStyle *rc_style);
extern void FileBrowserSetTransientFor(GtkWidget *w);
extern void FileBrowserSetObjectCreatedCB(
	void (*cb)(
		const gchar *path,
		gpointer data
	),
	gpointer data
);
extern void FileBrowserSetObjectModifiedCB(
	void (*cb)(
		const gchar *old_path,
		const gchar *new_path,
		gpointer data
	),
	gpointer data
);
extern void FileBrowserSetObjectDeletedCB(
	void (*cb)(
		const gchar *path,
		gpointer data
	),
	gpointer data
);
extern gboolean FileBrowserIsQuery(void);
extern void FileBrowserBreakQuery(void);
extern GtkWidget *FileBrowserGetToplevel(void);
extern void FileBrowserReset(void);
extern gboolean FileBrowserGetResponse(
	const gchar *title,
	const gchar *ok_label,
	const gchar *cancel_label,
	const gchar *path,
	fb_type_struct **type, gint total_types,
	gchar ***path_rtn, gint *path_total_rtns,
	fb_type_struct **type_rtn
);
extern void FileBrowserMap(void);
extern void FileBrowserUnmap(void);
extern void FileBrowserShutdown(void);

extern const gchar *FileBrowserGetLastLocation(void);
extern void FileBrowserShowHiddenObjects(const gboolean show);
extern void FileBrowserListStandard(void);
extern void FileBrowserListDetailed(void);


/* File Types */
extern gint FileBrowserTypeListNew(
	fb_type_struct ***list, gint *total,
	const gchar *ext,
	const gchar *name
);
extern void FileBrowserDeleteTypeList(
	fb_type_struct **list, const gint total
);


#endif	/* FB_H */
