/*
				Prompt Dialog
 */

#ifndef PDIALOG_H
#define PDIALOG_H

#include <gtk/gtk.h>


/*
 *	Icon Codes:
 */
typedef enum {
	PDIALOG_ICON_INFO,
	PDIALOG_ICON_WARNING,
	PDIALOG_ICON_ERROR,
	PDIALOG_ICON_QUESTION,
	PDIALOG_ICON_HELP,
	PDIALOG_ICON_WIZARD,		/* Wand */
	PDIALOG_ICON_SEARCH,		/* Magnifying glass */
	PDIALOG_ICON_SECURITY,		/* Padlock with key */
	PDIALOG_ICON_PRINTER,
	PDIALOG_ICON_SPEAKER,
	PDIALOG_ICON_BULB,
	PDIALOG_ICON_POWER,		/* Battery and A/C plug */
	PDIALOG_ICON_OS,		/* Operating System's Icon */
	PDIALOG_ICON_TERMINAL,
	PDIALOG_ICON_SETTINGS,		/* Gears */
	PDIALOG_ICON_TOOLS,
	PDIALOG_ICON_MONITOR,
	PDIALOG_ICON_CLIPBOARD_EMPTY,
	PDIALOG_ICON_CLIPBOARD_FULL,
	PDIALOG_ICON_EDIT,
	PDIALOG_ICON_INSTALL,
	PDIALOG_ICON_UNINSTALL,

	PDIALOG_ICON_FILE,
	PDIALOG_ICON_FOLDER_CLOSER,
	PDIALOG_ICON_FOLDER_OPENED,
	PDIALOG_ICON_LINK,
	PDIALOG_ICON_PIPE,
	PDIALOG_ICON_DEVICE,
	PDIALOG_ICON_DEVICE_BLOCK,
	PDIALOG_ICON_DEVICE_CHARACTER,
	PDIALOG_ICON_SOCKET,

	PDIALOG_ICON_FILE_MOVE,
	PDIALOG_ICON_FILE_COPY,
	PDIALOG_ICON_FILE_PROPERTIES,

	PDIALOG_ICON_PLANET,
	PDIALOG_ICON_FTP,
	PDIALOG_ICON_CHAT,
	PDIALOG_ICON_FILE_WWW,

	PDIALOG_ICON_USER_DEFINED
} pdialog_icon;


/*
 *	Button Flags:
 */
typedef enum {
	PDIALOG_BTNFLAG_SUBMIT		= (1 << 1),
	PDIALOG_BTNFLAG_CANCEL		= (1 << 2),
	PDIALOG_BTNFLAG_HELP		= (1 << 3)
} pdialog_btn_flags;


extern gint PDialogInit(void);
extern void PDialogSetStyle(GtkRcStyle *rc_style);
extern void PDialogSetTransientFor(GtkWidget *w);
extern gboolean PDialogIsQuery(void);
extern void PDialogBreakQuery(void);
extern GtkWidget *PDialogGetToplevel(void);

extern void PDialogAddPromptLabel(const gchar *label);
extern void PDialogAddPrompt(
	const guint8 **icon_data,
	const gchar *label,
	const gchar *value
);
extern void PDialogAddPromptPassword(
	const guint8 **icon_data,
	const gchar *label,
	const gchar *value
);
extern void PDialogAddPromptWithBrowse(
	const guint8 **icon_data,
	const gchar *label,
	const gchar *value,
	gpointer client_data,
	gchar *(*browse_cb)(gpointer, gpointer, gint)
);
extern void PDialogAddPromptSpin(
	const guint8 **icon_data,
	const gchar *label,
	gfloat value, gfloat lower, gfloat upper,
	gfloat step_increment, gfloat page_increment,
	gfloat climb_rate, guint digits
);
extern void PDialogAddPromptScale(
	const guint8 **icon_data,
	const gchar *label,
	gfloat value, gfloat lower, gfloat upper,
	gfloat step_increment, gfloat page_increment,
	gboolean show_value, guint digits
);
extern void PDialogAddPromptPopupList(
	const guint8 **icon_data,
	const gchar *label,
	GList *list,			/* List of values */
	const gint start_num,		/* Initial value */
	const gint nitems_visible	/* Number of items visible */
);
extern void PDialogAddPromptCombo(
	const guint8 **icon_data,
	const gchar *label,
	const gchar *value,		/* Initial value */
	GList *list,			/* List of values */
	const gboolean editable,
	const gboolean case_sensitive
);
extern void PDialogAddPromptRadio(
	const guint8 **icon_data,
	const gchar *label,
	GList *list,			/* List of values */
	const gint start_num		/* Initial value */
);
extern void PDialogAddPromptToggle(
	const guint8 **icon_data,
	const gchar *label, gboolean value
);
extern void PDialogAddPromptSeparator(void);
extern void PDialogSetPromptValue(
	gint prompt_num,
	const guint8 **icon_data,
	const gchar *label,
	const gchar *value
);
extern void PDialogSetPromptTip(gint prompt_num, const gchar *tip);
extern const gchar *PDialogGetPromptValue(gint prompt_num);
extern void PDialogSetPromptCompletePath(gint prompt_num);
extern void PDialogDeleteAllPrompts(void);

extern gchar **PDialogGetResponse(
	const gchar *title,
	const gchar *message,
	const gchar *explaination,
	pdialog_icon icon_code,
	const gchar *submit_label,
	const gchar *cancel_label,
	pdialog_btn_flags show_buttons,		/* Any of PDIALOG_BTNFLAG_* */
	pdialog_btn_flags default_button,	/* One of PDIALOG_BTNFLAG_* */
	gint *nvalues
);
extern gchar **PDialogGetResponseIconData(
	const gchar *title,
	const gchar *message,
	const gchar *explaination,
	guint8 **icon_data,
	const gchar *submit_label,
	const gchar *cancel_label,
	pdialog_btn_flags show_buttons,		/* Any of PDIALOG_BTNFLAG_* */
	pdialog_btn_flags default_button,	/* One of PDIALOG_BTNFLAG_* */
	gint *nvalues
);

extern void PDialogSetSize(gint width, gint height);

extern void PDialogMap(void);
extern void PDialogUnmap(void);

extern void PDialogShutdown(void);


#endif	/* PDIALOG_H */
