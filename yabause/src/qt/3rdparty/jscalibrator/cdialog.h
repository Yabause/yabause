/*
			   Confirmation Dialog
 */

#ifndef CDIALOG_H
#define CDIALOG_H

#include <gtk/gtk.h>


/*
 *	Response codes:
 */
#define CDIALOG_RESPONSE_NOT_AVAILABLE	-1
#define CDIALOG_RESPONSE_NO		0
#define CDIALOG_RESPONSE_YES		1
#define CDIALOG_RESPONSE_YES_TO_ALL	2
#define CDIALOG_RESPONSE_CANCEL		3
#define CDIALOG_RESPONSE_OK		4
#define CDIALOG_RESPONSE_HELP		5


/*
 *	Message icon codes, specifies the icon displayed next to
 *	the message:
 */
typedef enum {
	CDIALOG_ICON_INFO,
	CDIALOG_ICON_WARNING,
	CDIALOG_ICON_ERROR,
	CDIALOG_ICON_QUESTION,
	CDIALOG_ICON_HELP,
	CDIALOG_ICON_WIZARD,		/* Wand With Star */
        CDIALOG_ICON_SEARCH,		/* Magnifying Glass */
        CDIALOG_ICON_SECURITY,		/* Padlock & Key */
        CDIALOG_ICON_PRINTER,
        CDIALOG_ICON_SPEAKER,
        CDIALOG_ICON_BULB,		/* Light Bulb */
        CDIALOG_ICON_POWER,		/* Battery & A/C Plug */
        CDIALOG_ICON_OS,		/* The Operating System's Icon */
 	CDIALOG_ICON_TERMINAL,
	CDIALOG_ICON_SETTINGS,		/* Gears */
	CDIALOG_ICON_TOOLS,
	CDIALOG_ICON_MONITOR,
        CDIALOG_ICON_CLIPBOARD_EMPTY,
        CDIALOG_ICON_CLIPBOARD_FULL,
        CDIALOG_ICON_INSTALL,
        CDIALOG_ICON_UNINSTALL,
        CDIALOG_ICON_CPU,		/* Processor Chip */

	CDIALOG_ICON_FILE,
        CDIALOG_ICON_FOLDER_CLOSER,
        CDIALOG_ICON_FOLDER_OPENED,
        CDIALOG_ICON_LINK,
        CDIALOG_ICON_PIPE,
        CDIALOG_ICON_DEVICE,
        CDIALOG_ICON_DEVICE_BLOCK,
        CDIALOG_ICON_DEVICE_CHARACTER,
        CDIALOG_ICON_SOCKET,

        CDIALOG_ICON_FILE_MOVE,
        CDIALOG_ICON_FILE_COPY,

        CDIALOG_ICON_PLANET,
        CDIALOG_ICON_FTP,
        CDIALOG_ICON_CHAT,
        CDIALOG_ICON_FILE_WWW,

	CDIALOG_ICON_USER_DEFINED
} cdialog_icon;


/*
 *	Button flags:
 */
typedef enum {
	CDIALOG_BTNFLAG_OK		= (1 << 0),
	CDIALOG_BTNFLAG_YES		= (1 << 1),
	CDIALOG_BTNFLAG_YES_TO_ALL	= (1 << 2),
	CDIALOG_BTNFLAG_NO		= (1 << 3),
	CDIALOG_BTNFLAG_CANCEL		= (1 << 4),
	CDIALOG_BTNFLAG_IGNORE		= (1 << 5),
	CDIALOG_BTNFLAG_RETRY		= (1 << 6),
	CDIALOG_BTNFLAG_ABORT		= (1 << 7),
	CDIALOG_BTNFLAG_HELP		= (1 << 8)
} cdialog_btn_flags;


extern gint CDialogInit(void);
extern void CDialogSetStyle(GtkRcStyle *rc_style);
extern void CDialogSetTransientFor(GtkWidget *w);
extern gboolean CDialogIsQuery(void);
extern void CDialogBreakQuery(void);
extern GtkWidget *CDialogGetToplevel(void);
extern gint CDialogGetResponse(
	const gchar *title,
	const gchar *message,
	const gchar *explaination,
	cdialog_icon icon_code,
	cdialog_btn_flags show_buttons,		/* Any of CDIALOG_BTNFLAG_* */
	cdialog_btn_flags default_button	/* One of CDIALOG_BTNFLAG_* */
);
extern gint CDialogGetResponseIconData(
	const gchar *title,
	const gchar *message,
	const gchar *explaination,
	guint8 **icon_data,
	cdialog_btn_flags show_buttons,		/* Any of CDIALOG_BTNFLAG_* */
	cdialog_btn_flags default_button	/* One of CDIALOG_BTNFLAG_* */
);
extern gint CDialogGetResponseFile(
	const gchar *title,
	const gchar *filename,
	const gchar *explaination,
	cdialog_icon icon_code,
	cdialog_btn_flags show_buttons,		/* Any of CDIALOG_BTNFLAG_* */
	cdialog_btn_flags default_button	/* One of CDIALOG_BTNFLAG_* */
);
extern void CDialogMap(void);
extern void CDialogUnmap(void);
extern void CDialogShutdown(void);


#endif	/* CDIALOG_H */
