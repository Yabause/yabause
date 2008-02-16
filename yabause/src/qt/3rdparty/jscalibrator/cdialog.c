#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "guiutils.h"
#include "cdialog.h"
#include "config.h"


/* Message Icons */
#include "images/icon_info_32x32.xpm"
#include "images/icon_warning_32x32.xpm"
#include "images/icon_error_32x32.xpm"
#include "images/icon_question_32x32.xpm"
#include "images/icon_help_32x32.xpm"
#include "images/icon_wand_32x32.xpm"
#include "images/icon_search_32x32.xpm"
#include "images/icon_security_32x32.xpm"
#include "images/icon_print2_32x32.xpm"
#include "images/icon_sound_32x32.xpm"
#include "images/icon_bulb_32x32.xpm"
#include "images/icon_power_32x32.xpm"
#if defined(__linux__)
# include "images/icon_linux_32x32.xpm"
#else
# include "images/icon_linux_32x32.xpm"
#endif
#include "images/icon_terminal2_32x32.xpm"
#include "images/icon_tuning_32x32.xpm"
#include "images/icon_tools_32x32.xpm"
#include "images/icon_monitor2_32x32.xpm"
#include "images/icon_clipboard_32x32.xpm"
#include "images/icon_clipboard_empty_32x32.xpm"
#include "images/icon_install_32x32.xpm"
#include "images/icon_uninstall_32x32.xpm"
#include "images/icon_cpu_32x32.xpm"

#include "images/icon_file_32x32.xpm"
#include "images/icon_folder_closed_32x32.xpm"
#include "images/icon_folder_opened_32x32.xpm"
#include "images/icon_link2_32x32.xpm"
#include "images/icon_pipe_32x32.xpm"
#include "images/icon_device_misc_32x32.xpm"
#include "images/icon_device_block_32x32.xpm"
#include "images/icon_device_character_32x32.xpm"
#include "images/icon_socket_32x32.xpm"

#if 0
/* TODO */
#include "images/icon_move_file_32x32.xpm"
#include "images/icon_copy_file_32x32.xpm"
#endif

#include "images/icon_planet_32x32.xpm"
#include "images/icon_ftp_32x32.xpm"
#include "images/icon_chat_32x32.xpm"
#include "images/icon_file_www_32x32.xpm"


/* Button icons */
#include "images/icon_ok_20x20.xpm"
#include "images/icon_cancel_20x20.xpm"
#include "images/icon_help_20x20.xpm"


typedef struct _CDlg			CDlg;
#define CDLG(p)		((CDlg *)(p))


/* Callbacks */
static gint CDialogCloseCB(
	GtkWidget *widget, GdkEvent *event, gpointer data
);
static gint CDialogKeyCB(
	GtkWidget *widget, GdkEventKey *key, gpointer data
);
static void CDialogButtonCB(GtkWidget *widget, gpointer data);

/* Utilities */
static guint8 **CDialogGetMessageIconDataFromCode(cdialog_icon icon_code);
static void CDialogLoadIcon(CDlg *d, guint8 **icon_data);

/* Front Ends */
gint CDialogInit(void);
void CDialogSetStyle(GtkRcStyle *rc_style);
void CDialogSetTransientFor(GtkWidget *w);
gboolean CDialogIsQuery(void);
void CDialogBreakQuery(void);
GtkWidget *CDialogGetToplevel(void);
static void CDialogRemapButtons(
	CDlg *d,
	cdialog_btn_flags show_buttons,		/* Any of CDIALOG_BTNFLAG_* */
	cdialog_btn_flags default_button	/* One of CDIALOG_BTNFLAG_* */
);
gint CDialogGetResponse(
	const gchar *title,
	const gchar *message,
	const gchar *explaination,
	cdialog_icon icon_code,
	cdialog_btn_flags show_buttons,		/* Any of CDIALOG_BTNFLAG_* */
	cdialog_btn_flags default_button	/* One of CDIALOG_BTNFLAG_* */
);
gint CDialogGetResponseIconData(
	const gchar *title,
	const gchar *message,
	const gchar *explaination,
	guint8 **icon_data,
	cdialog_btn_flags show_buttons,		/* Any of CDIALOG_BTNFLAG_* */
	cdialog_btn_flags default_button	/* One of CDIALOG_BTNFLAG_* */
);
gint CDialogGetResponseFile(
	const gchar *title,
	const gchar *filename,
	const gchar *explaination,
	cdialog_icon icon_code,
	cdialog_btn_flags show_buttons,		/* Any of CDIALOG_BTNFLAG_* */
	cdialog_btn_flags default_button	/* One of CDIALOG_BTNFLAG_* */
);

void CDialogMap(void);
void CDialogUnmap(void);
void CDialogShutdown(void);


/*
 *	Confirmation Dialog:
 */
struct _CDlg {

	GtkWidget	*toplevel;
	GtkAccelGroup	*accelgrp;
	gint		freeze_count,
			main_level,
			response_code;

	GtkWidget	*main_vbox,
			*label_hbox,
			*icon_pm,
			*label,
			*help_vbox,
			*help_icon_pm,
			*help_label,
			*button_hbox;

	GtkWidget	*ok_btn,
			*yes_btn,
			*yes_to_all_btn,
			*no_btn,
			*cancel_btn,
			*ignore_btn,
			*retry_btn,
			*abort_btn,
			*help_btn;

	GtkWidget	*last_transient_for;	/* Do not reference */
	cdialog_icon	last_icon_code;

};


static CDlg *cdlg;


#if defined(PROG_LANGUAGE_SPANISH)
# define CDIALOG_DEF_TITLE	"El Mensaje"
#elif defined(PROG_LANGUAGE_FRENCH)
# define CDIALOG_DEF_TITLE	"Message"
#elif defined(PROG_LANGUAGE_GERMAN)
# define CDIALOG_DEF_TITLE	"Nachricht"
#elif defined(PROG_LANGUAGE_ITALIAN)
# define CDIALOG_DEF_TITLE	"Il Messaggio"
#elif defined(PROG_LANGUAGE_DUTCH)
# define CDIALOG_DEF_TITLE	"Bericht"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
# define CDIALOG_DEF_TITLE	"A Mensagem"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
# define CDIALOG_DEF_TITLE	"Budskap"
#else
# define CDIALOG_DEF_TITLE	"Message"
#endif

#if defined(PROG_LANGUAGE_SPANISH)
# define CDIALOG_MESG_NO_HELP_AVAILABLE	"Ninguna ayuda disponible."
#elif defined(PROG_LANGUAGE_FRENCH)
# define CDIALOG_MESG_NO_HELP_AVAILABLE	"Aucune aide disponible."
#elif defined(PROG_LANGUAGE_GERMAN)
# define CDIALOG_MESG_NO_HELP_AVAILABLE	"Keine hilfe verfügbar."
#elif defined(PROG_LANGUAGE_ITALIAN)
# define CDIALOG_MESG_NO_HELP_AVAILABLE	"Nessuno aiuto disponibile."
#elif defined(PROG_LANGUAGE_DUTCH)
# define CDIALOG_MESG_NO_HELP_AVAILABLE	"Geen hulp verkrijgbaar."
#elif defined(PROG_LANGUAGE_PORTUGUESE)
# define CDIALOG_MESG_NO_HELP_AVAILABLE	"A nenhuma ajuda disponível."
#elif defined(PROG_LANGUAGE_NORWEGIAN)
# define CDIALOG_MESG_NO_HELP_AVAILABLE	"Ingen hjelp tilgjengelig."
#else
# define CDIALOG_MESG_NO_HELP_AVAILABLE	"No help available."
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


/*
 *	GtkWindow "delete_event" signal callback.
 */
static gint CDialogCloseCB(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	CDlg *d = CDLG(data);
	if((widget == NULL) || (d == NULL))
	    return(FALSE);

	if(d->freeze_count > 0)
	    return(FALSE);

	/* Set the response code to "cancel" */
	d->response_code = CDIALOG_RESPONSE_CANCEL;

	/* Break out of this dialog's GTK main loop */
	if(d->main_level > 0)
	{
	    gtk_main_quit();
	    d->main_level--;
	}

	return(TRUE);
}

/*
 *	Dialog toplevel GtkWindow "key_press_event" or
 *	"key_release_event" signal callback.
 */
static gint CDialogKeyCB(
	GtkWidget *widget, GdkEventKey *key, gpointer data
)
{
	gboolean press;
	gint status = FALSE;
	CDlg *d = CDLG(data);
	if((widget == NULL) || (key == NULL) || (d == NULL))
	    return(status);

	if(d->freeze_count > 0)
	    return(status);

	press = (key->type == GDK_KEY_PRESS) ? TRUE : FALSE;

	switch(key->keyval)
	{
	  case GDK_Escape:
	    if(press)
	    {
                /* Set the response code to "cancel" */
                d->response_code = CDIALOG_RESPONSE_CANCEL;

                /* Break out of this dialog's GTK main loop */
                if(d->main_level > 0)
                {
                    gtk_main_quit();
                    d->main_level--;
                }
	    }
	    status = TRUE;
	    break;
	}

	return(status);
}

/*
 *	Dialog GtkButton "clicked" signal callback.
 */
static void CDialogButtonCB(GtkWidget *widget, gpointer data)
{
	GtkWidget *w;
	CDlg *d = CDLG(data);
	if((widget == NULL) || (d == NULL))
	    return;

	if(d->freeze_count > 0)
	    return;

	/* If this button is not mapped (which implies this signal
	 * was received synthetically or from an keyboard accelerator)
	 * then it should be ignored
	 */
	if(!GTK_WIDGET_MAPPED(widget))
	    return;

	/* Check which button was pressed */
	if(widget == d->ok_btn)
	    d->response_code = CDIALOG_RESPONSE_OK;
	else if(widget == d->yes_btn)
	    d->response_code = CDIALOG_RESPONSE_YES;
	else if(widget == d->yes_to_all_btn)
	    d->response_code = CDIALOG_RESPONSE_YES_TO_ALL;
	else if(widget == d->no_btn)
	    d->response_code = CDIALOG_RESPONSE_NO;
	else if(widget == d->cancel_btn)
	    d->response_code = CDIALOG_RESPONSE_CANCEL;
	else if(widget == d->help_btn)
	{
	    d->response_code = CDIALOG_RESPONSE_HELP;
	    w = d->help_vbox;
	    if(w != NULL)
		gtk_widget_show(w);
	    w = d->help_btn;
	    if(w != NULL)
		gtk_widget_hide(w);

	    return;	/* Return, do not pop a main loop */
	}

	/* Break out of this dialog's GTK main loop */
	if(d->main_level > 0)
	{
	    gtk_main_quit();
	    d->main_level--;
	}
}


/*
 *	Returns the icon data that corresponds to the specified
 *	icon_code.
 *
 *	The returned pointer must not be modified or deleted.
 */
static guint8 **CDialogGetMessageIconDataFromCode(cdialog_icon icon_code)
{
	guint8 **d = (guint8 **)icon_info_32x32_xpm;

	switch(icon_code)
	{
	  case CDIALOG_ICON_INFO:
	    d = (guint8 **)icon_info_32x32_xpm;
	    break;
	  case CDIALOG_ICON_WARNING:
	    d = (guint8 **)icon_warning_32x32_xpm;
	    break;
	  case CDIALOG_ICON_ERROR:
	    d = (guint8 **)icon_error_32x32_xpm;
	    break;
	  case CDIALOG_ICON_QUESTION:
	    d = (guint8 **)icon_question_32x32_xpm;
	    break;
	  case CDIALOG_ICON_HELP:
	    d = (guint8 **)icon_help_32x32_xpm;
	    break;
	  case CDIALOG_ICON_WIZARD:
	    d = (guint8 **)icon_wand_32x32_xpm;
	    break;
	  case CDIALOG_ICON_SEARCH:
	    d = (guint8 **)icon_search_32x32_xpm;
	    break;
	  case CDIALOG_ICON_SECURITY:
	    d = (guint8 **)icon_security_32x32_xpm;
	    break;
	  case CDIALOG_ICON_PRINTER:
	    d = (guint8 **)icon_print2_32x32_xpm;
	    break;
	  case CDIALOG_ICON_SPEAKER:
	    d = (guint8 **)icon_sound_32x32_xpm;
	    break;
	  case CDIALOG_ICON_BULB:
	    d = (guint8 **)icon_bulb_32x32_xpm;
	    break;
	  case CDIALOG_ICON_POWER:
	    d = (guint8 **)icon_power_32x32_xpm;
	    break;
	  case CDIALOG_ICON_OS:
#if defined(__linux__)
	    d = (guint8 **)icon_linux_32x32_xpm;
#else
	    d = (guint8 **)icon_linux_32x32_xpm;
#endif
	    break;
	  case CDIALOG_ICON_TERMINAL:
	    d = (guint8 **)icon_terminal2_32x32_xpm;
	    break;
	  case CDIALOG_ICON_SETTINGS:
	    d = (guint8 **)icon_tuning_32x32_xpm;
	    break;
	  case CDIALOG_ICON_TOOLS:
	    d = (guint8 **)icon_tools_32x32_xpm;
	    break;
	  case CDIALOG_ICON_MONITOR:
	    d = (guint8 **)icon_monitor2_32x32_xpm;
	    break;
	  case CDIALOG_ICON_CLIPBOARD_EMPTY:
	    d = (guint8 **)icon_clipboard_empty_32x32_xpm;
	    break;
	  case CDIALOG_ICON_CLIPBOARD_FULL:
	    d = (guint8 **)icon_clipboard_32x32_xpm;
	    break;
	  case CDIALOG_ICON_INSTALL:
	    d = (guint8 **)icon_install_32x32_xpm;
	    break;
	  case CDIALOG_ICON_UNINSTALL:
	    d = (guint8 **)icon_uninstall_32x32_xpm;
	    break;
	  case CDIALOG_ICON_CPU:
	    d = (guint8 **)icon_cpu_32x32_xpm;
	    break;

	  case CDIALOG_ICON_FILE:
	    d = (guint8 **)icon_file_32x32_xpm;
	    break;
	  case CDIALOG_ICON_FOLDER_CLOSER:
	    d = (guint8 **)icon_folder_closed_32x32_xpm;
	    break;
	  case CDIALOG_ICON_FOLDER_OPENED:
	    d = (guint8 **)icon_folder_opened_32x32_xpm;
	    break;
	  case CDIALOG_ICON_LINK:
	    d = (guint8 **)icon_link2_32x32_xpm;
	    break;
	  case CDIALOG_ICON_PIPE:
	    d = (guint8 **)icon_pipe_32x32_xpm;
	    break;
	  case CDIALOG_ICON_DEVICE:
	    d = (guint8 **)icon_device_misc_32x32_xpm;
	    break;
	  case CDIALOG_ICON_DEVICE_BLOCK:
	    d = (guint8 **)icon_device_block_32x32_xpm;
	    break;
	  case CDIALOG_ICON_DEVICE_CHARACTER:
	    d = (guint8 **)icon_device_character_32x32_xpm;
	    break;
	  case CDIALOG_ICON_SOCKET:
	    d = (guint8 **)icon_socket_32x32_xpm;
	    break;

	  case CDIALOG_ICON_FILE_MOVE:
	    d = (guint8 **)icon_file_32x32_xpm;
	    break;
	  case CDIALOG_ICON_FILE_COPY:
	    d = (guint8 **)icon_file_32x32_xpm;
	    break;

	  case CDIALOG_ICON_PLANET:
	    d = (guint8 **)icon_planet_32x32_xpm;
	    break;
	  case CDIALOG_ICON_FTP:
	    d = (guint8 **)icon_ftp_32x32_xpm;
	    break;
	  case CDIALOG_ICON_CHAT:
	    d = (guint8 **)icon_chat_32x32_xpm;
	    break;
	  case CDIALOG_ICON_FILE_WWW:
	    d = (guint8 **)icon_file_www_32x32_xpm;
	    break;

	  case CDIALOG_ICON_USER_DEFINED:
	    break;
	}
	return(d);
}

/*
 *	Updates the CDialog's icons.
 */
static void CDialogLoadIcon(CDlg *d, guint8 **icon_data)
{
	gint width, height;
	GdkBitmap *mask;
	GdkPixmap *pixmap;
	GtkWidget *w = (d != NULL) ? d->icon_pm : NULL;
	if((w == NULL) || (icon_data == NULL))
	    return;

	/* Load new pixmap for icon */
	pixmap = GDK_PIXMAP_NEW_FROM_XPM_DATA(&mask, icon_data);
	if(pixmap == NULL)
	    return;

	gdk_window_get_size(pixmap, &width, &height);

	/* Set icon GtkPixmap */
	gtk_pixmap_set(GTK_PIXMAP(w), pixmap, mask);
	gtk_widget_set_usize(w, width, height);

	GDK_PIXMAP_UNREF(pixmap);
	GDK_BITMAP_UNREF(mask);

	/* Toplevel needs to be resized since new icon size may be
	 * different from the previous one
	 */
	w = d->toplevel;
	gtk_widget_queue_resize(w);

	/* Set WM icon based on the icon data */
	GUISetWMIcon(w->window, icon_data);
}


/*
 *	Initializes the CDialog.
 */
gint CDialogInit(void)
{
	const gint border_major = 5;
	GtkStateType state;
	GdkBitmap *mask;
	GdkPixmap *pixmap;
	GdkWindow *window;
	GtkAccelGroup *accelgrp;
	GtkRcStyle *rcstyle;
	GtkStyle *style;
	GtkWidget	*w,
			*parent, *parent2, *parent3, *parent4,
			*parent5,
			*toplevel;
	CDlg *d;

	/* Already initialized? */
	if(cdlg != NULL)
	    return(0);

	/* Create the new Dialog */
	cdlg = d = CDLG(g_malloc0(sizeof(CDlg)));
	if(d == NULL)
	    return(-3);

	d->toplevel = toplevel = gtk_window_new(GTK_WINDOW_DIALOG);
	d->accelgrp = accelgrp = gtk_accel_group_new();
	d->freeze_count = 0;
	d->response_code = CDIALOG_RESPONSE_NOT_AVAILABLE;
	d->main_level = 0;
	d->last_transient_for = NULL;
	d->last_icon_code = CDIALOG_ICON_INFO;

	d->freeze_count++;

	/* Toplevel GtkWindow */
	w = toplevel;
        gtk_window_set_policy(GTK_WINDOW(w), TRUE, TRUE, TRUE);
	gtk_window_set_title(GTK_WINDOW(w), CDIALOG_DEF_TITLE);
#ifdef PROG_NAME
	gtk_window_set_wmclass(
	    GTK_WINDOW(w),
	    "dialog",
	    PROG_NAME
	);
#endif
	gtk_widget_realize(w);
	window = w->window;
	if(window != NULL)
	{
	    GdkGeometry geo;
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
	    gdk_window_set_decorations(
		window,
		GDK_DECOR_BORDER | GDK_DECOR_TITLE
	    );
	    gdk_window_set_functions(
		window,
		GDK_FUNC_MOVE | GDK_FUNC_CLOSE
	    );
	}
	gtk_widget_add_events(
	    w,
	    GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "delete_event",
	    GTK_SIGNAL_FUNC(CDialogCloseCB), d
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "key_press_event",
	    GTK_SIGNAL_FUNC(CDialogKeyCB), d
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "key_release_event",
	    GTK_SIGNAL_FUNC(CDialogKeyCB), d
	);
	gtk_window_add_accel_group(GTK_WINDOW(w), accelgrp);
	style = gtk_widget_get_style(w);
	parent = w;

	/* Main GtkVBox */
	d->main_vbox = w = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(parent), w);
	gtk_widget_show(w);
	parent = w;


	/* Icon and message GtkHBox */
	d->label_hbox = w = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(parent), w, TRUE, TRUE, border_major);
	gtk_widget_show(w);
	parent2 = w;

	/* Icon GtkVBox */
	w = gtk_vbox_new(TRUE, 0);
	gtk_box_pack_start(GTK_BOX(parent2), w, FALSE, FALSE, border_major);
	gtk_widget_show(w);
	parent3 = w;
	/* Icon */
	pixmap = gdk_pixmap_create_from_xpm_d(
	    window,
	    &mask,
	    NULL,
	    (gchar **)icon_info_32x32_xpm
	);
	if(pixmap != NULL)
	{
	    gint width, height;
	    gdk_window_get_size(pixmap, &width, &height);
	    d->icon_pm = w = gtk_pixmap_new(pixmap, mask);
	    gtk_widget_set_usize(w, width, height);
	    gtk_box_pack_start(GTK_BOX(parent3), w, TRUE, FALSE, 0);
	    gtk_widget_show(w);
	    GDK_PIXMAP_UNREF(pixmap);
	    GDK_BITMAP_UNREF(mask);
	}

	/* Message GtkHBox */
	w = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(parent2), w, TRUE, TRUE, border_major);
	gtk_widget_show(w);
	parent3 = w;
	/* Message GtkLabel */
	d->label = w = gtk_label_new("");
	gtk_label_set_justify(GTK_LABEL(w), GTK_JUSTIFY_LEFT);
	gtk_box_pack_start(GTK_BOX(parent3), w, FALSE, FALSE, 0);
	gtk_widget_show(w);


	/* Help widgets GtkRCStyle */
	rcstyle = gtk_rc_style_new();
	state = GTK_STATE_NORMAL;
	rcstyle->color_flags[state] = GTK_RC_FG | GTK_RC_BG |
	    GTK_RC_TEXT | GTK_RC_BASE;
	GDK_COLOR_SET_COEFF(
	    &rcstyle->fg[state],
	    0.0f, 0.0f, 0.0f
	);
	GDK_COLOR_SET_COEFF(
	    &rcstyle->bg[state],
	    1.0f, 1.0f, 0.8f
	);
	GDK_COLOR_SET_COEFF(
	    &rcstyle->text[state],
	    0.0f, 0.0f, 0.0f
	);
	GDK_COLOR_SET_COEFF(
	    &rcstyle->base[state],
	    1.0f, 1.0f, 0.8f
	);
	state = GTK_STATE_INSENSITIVE;
	rcstyle->color_flags[state] = GTK_RC_FG | GTK_RC_BG |
	    GTK_RC_TEXT | GTK_RC_BASE;
	GDK_COLOR_SET_COEFF(
	    &rcstyle->fg[state],
	    0.25f, 0.25f, 0.25f
	);
	GDK_COLOR_SET_COEFF(
	    &rcstyle->bg[state],
	    1.0f, 1.0f, 0.8f
	);
	GDK_COLOR_SET_COEFF(
	    &rcstyle->text[state],
	    0.25f, 0.25f, 0.25f
	);
	GDK_COLOR_SET_COEFF(
	    &rcstyle->base[state],
	    1.0f, 1.0f, 0.8f
	);

	/* GtkVBox for the help GtkWidgets */
	d->help_vbox = w = gtk_vbox_new(FALSE, 0);	
	gtk_box_pack_start(GTK_BOX(parent), w, FALSE, FALSE, border_major);
	parent2 = w;
	w = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(parent2), w, FALSE, FALSE, 0);
	gtk_widget_show(w);
	parent3 = w;

	/* Help GtkFrame */
	w = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(w), GTK_SHADOW_IN);
	gtk_box_pack_start(GTK_BOX(parent3), w, TRUE, TRUE, border_major);
 	gtk_widget_show(w);
	parent4 = w;
	/* Help background GtkEventBox */
	w = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(parent4), w);
	gtk_widget_modify_style(w, rcstyle);
	gtk_widget_show(w);
	parent4 = w;

	/* Help GtkHBox inside the GtkFrame */
	w = gtk_hbox_new(FALSE, border_major);
	gtk_container_border_width(GTK_CONTAINER(w), border_major);
	gtk_container_add(GTK_CONTAINER(parent4), w);
	gtk_widget_show(w);
	parent4 = w;

	/* Help icon GtkVBox */
	w = gtk_vbox_new(TRUE, 0);
	gtk_box_pack_start(GTK_BOX(parent4), w, FALSE, FALSE, 0);
	gtk_widget_show(w);
	parent5 = w;
	/* Help icon pixmap */
	pixmap = gdk_pixmap_create_from_xpm_d(
	    window,
	    &mask,
	    NULL,
	    (gchar **)icon_help_32x32_xpm
	);
	if(pixmap != NULL)
	{
	    gint width, height;
	    gdk_window_get_size(pixmap, &width, &height);
	    d->help_icon_pm = w = gtk_pixmap_new(pixmap, mask);
	    gtk_box_pack_start(GTK_BOX(parent5), w, TRUE, FALSE, 0);
	    gtk_widget_modify_style(w, rcstyle);         
	    gtk_widget_show(w);
	    GDK_PIXMAP_UNREF(pixmap);
	    GDK_BITMAP_UNREF(mask);
	}

	/* Help message GtkHBox */
	w = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(parent4), w, TRUE, TRUE, 0);
	gtk_widget_show(w);
	parent5 = w;
	/* Help message GtkLabel */
	d->help_label = w = gtk_label_new(
	    CDIALOG_MESG_NO_HELP_AVAILABLE
	);
	gtk_label_set_justify(GTK_LABEL(w), GTK_JUSTIFY_LEFT);
	gtk_box_pack_start(GTK_BOX(parent5), w, FALSE, FALSE, 0);
	gtk_widget_modify_style(w, rcstyle);
	gtk_widget_show(w);

	GTK_RC_STYLE_UNREF(rcstyle);

	/* Horizontal GtkSeparator */
	w = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(parent), w, FALSE, FALSE, 0);
	gtk_widget_show(w);


	/* Buttons GtkHBox */
	w = gtk_hbox_new(TRUE, 0);
	gtk_box_pack_start(GTK_BOX(parent), w, FALSE, FALSE, border_major);
	gtk_widget_show(w);
	parent2 = w;

	/* OK GtkButton */
	d->ok_btn = w = GUIButtonPixmapLabelH(
	    (guint8 **)icon_ok_20x20_xpm, "OK", NULL
	);
	GTK_WIDGET_SET_FLAGS(w, GTK_CAN_DEFAULT);
	gtk_widget_set_usize(
	    w,
	    GUI_BUTTON_HLABEL_WIDTH_DEF, GUI_BUTTON_HLABEL_HEIGHT_DEF
	);
	gtk_box_pack_start(GTK_BOX(parent2), w, TRUE, FALSE, 0);
	gtk_signal_connect(
	    GTK_OBJECT(w), "clicked",
	    GTK_SIGNAL_FUNC(CDialogButtonCB), d
	);
	gtk_accel_group_add(
	    accelgrp,
	    GDK_o, 0,
	    GTK_ACCEL_VISIBLE,
	    GTK_OBJECT(w),
	    "clicked"
	);
	/* Let the default/focus grabbed button receive the
	 * GDK_space and GDK_Return keys
	 */
	GUIButtonLabelUnderline(w, GDK_o);

	/* Yes GtkButton */
	d->yes_btn = w = GUIButtonPixmapLabelH(
	    (guint8 **)icon_ok_20x20_xpm,
#if defined(PROG_LANGUAGE_SPANISH)
"Si"
#elif defined(PROG_LANGUAGE_FRENCH)
"Oui"
#elif defined(PROG_LANGUAGE_GERMAN)
"Ja"
#elif defined(PROG_LANGUAGE_ITALIAN)
"Sì"
#elif defined(PROG_LANGUAGE_DUTCH)
"Ja"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"Sim"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Ja"
#else
"Yes"
#endif
	    , NULL
	);
	GTK_WIDGET_SET_FLAGS(w, GTK_CAN_DEFAULT);
	gtk_widget_set_usize(
	    w,
	    GUI_BUTTON_HLABEL_WIDTH_DEF, GUI_BUTTON_HLABEL_HEIGHT_DEF
	);
	gtk_box_pack_start(GTK_BOX(parent2), w, TRUE, FALSE, 0);
	gtk_signal_connect(
	    GTK_OBJECT(w), "clicked",
	    GTK_SIGNAL_FUNC(CDialogButtonCB), d
	);
	gtk_accel_group_add(
	    accelgrp,
	    GDK_y, 0,
	    GTK_ACCEL_VISIBLE,
	    GTK_OBJECT(w),
	    "clicked"
	);
	GUIButtonLabelUnderline(w, GDK_y);


	/* Yes To All GtkButton */
	d->yes_to_all_btn = w = GUIButtonPixmapLabelH(
	    (guint8 **)icon_ok_20x20_xpm,
#if defined(PROG_LANGUAGE_SPANISH)
"Si A Todo"
#elif defined(PROG_LANGUAGE_FRENCH)
"Oui à Tout"
#elif defined(PROG_LANGUAGE_GERMAN)
"Ja Allen"
#elif defined(PROG_LANGUAGE_ITALIAN)
"Sì A Tutto"
#elif defined(PROG_LANGUAGE_DUTCH)
"Ja Aan Alle"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"Sim A Todo"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Ja Til All"
#else
"Yes To All"
#endif
	     , NULL
	);
	GTK_WIDGET_SET_FLAGS(w, GTK_CAN_DEFAULT);
	gtk_widget_set_usize(
	    w,
	    GUI_BUTTON_HLABEL_WIDTH_DEF, GUI_BUTTON_HLABEL_HEIGHT_DEF
	);
	gtk_box_pack_start(GTK_BOX(parent2), w, TRUE, FALSE, 0);
	gtk_signal_connect(
	    GTK_OBJECT(w), "clicked",
	    GTK_SIGNAL_FUNC(CDialogButtonCB), d
	);
	gtk_accel_group_add(
	    accelgrp,
	    GDK_a, 0,
	    GTK_ACCEL_VISIBLE,
	    GTK_OBJECT(w),
	    "clicked"
	);
	GUIButtonLabelUnderline(w, GDK_a);

	/* No GtkButton */  
	d->no_btn = w = GUIButtonPixmapLabelH(
	    (guint8 **)icon_cancel_20x20_xpm,
#if defined(PROG_LANGUAGE_SPANISH)
"No"
#elif defined(PROG_LANGUAGE_FRENCH)
"Non"
#elif defined(PROG_LANGUAGE_GERMAN)
"Nein"
#elif defined(PROG_LANGUAGE_ITALIAN)
"No"
#elif defined(PROG_LANGUAGE_DUTCH)
"Geen"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"Não"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Ingen"
#else
"No"
#endif
	    , NULL
	);
	GTK_WIDGET_SET_FLAGS(w, GTK_CAN_DEFAULT);
	gtk_widget_set_usize(
	    w,
	    GUI_BUTTON_HLABEL_WIDTH_DEF, GUI_BUTTON_HLABEL_HEIGHT_DEF
	);
	gtk_box_pack_start(GTK_BOX(parent2), w, TRUE, FALSE, 0);
	gtk_signal_connect(
	    GTK_OBJECT(w), "clicked",
	    GTK_SIGNAL_FUNC(CDialogButtonCB), d
	);
	gtk_accel_group_add(
	    accelgrp,
	    GDK_n, 0,
	    GTK_ACCEL_VISIBLE,
	    GTK_OBJECT(w),
	    "clicked"
	);
	GUIButtonLabelUnderline(w, GDK_n);


	/* Cancel GtkButton */
	d->cancel_btn = w = GUIButtonPixmapLabelH(
	    (guint8 **)icon_cancel_20x20_xpm,
#if defined(PROG_LANGUAGE_SPANISH)
"Cancela"
#elif defined(PROG_LANGUAGE_FRENCH)
"Annuler"
#elif defined(PROG_LANGUAGE_GERMAN)
"Heben"
#elif defined(PROG_LANGUAGE_ITALIAN)
"Annulla"
#elif defined(PROG_LANGUAGE_DUTCH)
"Annuleer"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"Cancelamento"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Kanseller"
#else
"Cancel"
#endif
	    , NULL
	);
	GTK_WIDGET_SET_FLAGS(w, GTK_CAN_DEFAULT);
	gtk_widget_set_usize(
	    w,
	    GUI_BUTTON_HLABEL_WIDTH_DEF, GUI_BUTTON_HLABEL_HEIGHT_DEF
	);
	gtk_box_pack_start(GTK_BOX(parent2), w, TRUE, FALSE, 0);
	gtk_signal_connect(
	    GTK_OBJECT(w), "clicked",
	    GTK_SIGNAL_FUNC(CDialogButtonCB), d
	);
	gtk_accel_group_add(
	    accelgrp,
	    GDK_c, 0,
	    GTK_ACCEL_VISIBLE,
	    GTK_OBJECT(w),
	    "clicked"
	);
	GUIButtonLabelUnderline(w, GDK_c);

	/* Ignore GtkButton */
	d->ignore_btn = w = GUIButtonPixmapLabelH(
	    (guint8 **)icon_cancel_20x20_xpm,
#if defined(PROG_LANGUAGE_SPANISH)
"Ignore"
#elif defined(PROG_LANGUAGE_FRENCH)
"Négliger"
#elif defined(PROG_LANGUAGE_GERMAN)
"Ignorieren"
#elif defined(PROG_LANGUAGE_ITALIAN)
"Ignorare"
#elif defined(PROG_LANGUAGE_DUTCH)
"Negeer"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"Ignore"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Ignorer"
#else
"Ignore"
#endif
	    , NULL
	);
	GTK_WIDGET_SET_FLAGS(w, GTK_CAN_DEFAULT);
	gtk_widget_set_usize(
	    w,
	    GUI_BUTTON_HLABEL_WIDTH_DEF, GUI_BUTTON_HLABEL_HEIGHT_DEF
	);
	gtk_box_pack_start(GTK_BOX(parent2), w, TRUE, FALSE, 0);
	gtk_signal_connect(
	    GTK_OBJECT(w), "clicked",
	    GTK_SIGNAL_FUNC(CDialogButtonCB), d
	);
	gtk_accel_group_add(
	    accelgrp,
	    GDK_i, 0,
	    GTK_ACCEL_VISIBLE,
	    GTK_OBJECT(w),
	    "clicked"
	);
	GUIButtonLabelUnderline(w, GDK_i);

	/* Retry GtkButton */
	d->retry_btn = w = GUIButtonPixmapLabelH(
	    (guint8 **)icon_ok_20x20_xpm,
#if defined(PROG_LANGUAGE_SPANISH)
"Otra Vez"
#elif defined(PROG_LANGUAGE_FRENCH)
"Juger"
#elif defined(PROG_LANGUAGE_GERMAN)
"Wiederholen"
#elif defined(PROG_LANGUAGE_ITALIAN)
"Ritentare"
#elif defined(PROG_LANGUAGE_DUTCH)
"Probeer"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"Novamente"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Omprøving"
#else
"Retry"
#endif
	    , NULL
	);
	GTK_WIDGET_SET_FLAGS(w, GTK_CAN_DEFAULT);
	gtk_widget_set_usize(
	    w,
	    GUI_BUTTON_HLABEL_WIDTH_DEF, GUI_BUTTON_HLABEL_HEIGHT_DEF
	);
	gtk_box_pack_start(GTK_BOX(parent2), w, TRUE, FALSE, 0);
	gtk_signal_connect(
	    GTK_OBJECT(w), "clicked",
	    GTK_SIGNAL_FUNC(CDialogButtonCB), d
	);
	gtk_accel_group_add(
	    accelgrp,
	    GDK_r, 0,
	    GTK_ACCEL_VISIBLE,
	    GTK_OBJECT(w),
	    "clicked"
	);
	GUIButtonLabelUnderline(w, GDK_r);

	/* Abort GtkButton */
	d->abort_btn = w = GUIButtonPixmapLabelH(
	    (guint8 **)icon_cancel_20x20_xpm,
#if defined(PROG_LANGUAGE_SPANISH)
"Aborte"
#elif defined(PROG_LANGUAGE_FRENCH)
"Avorter"
#elif defined(PROG_LANGUAGE_GERMAN)
"Brechen"
#elif defined(PROG_LANGUAGE_ITALIAN)
"Abortire"
#elif defined(PROG_LANGUAGE_DUTCH)
"Breek"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"Aborte"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Aborter"
#else
"Abort"
#endif
	    , NULL
	);
	GTK_WIDGET_SET_FLAGS(w, GTK_CAN_DEFAULT);
	gtk_widget_set_usize(
	    w,
	    GUI_BUTTON_HLABEL_WIDTH_DEF, GUI_BUTTON_HLABEL_HEIGHT_DEF
	);
	gtk_box_pack_start(GTK_BOX(parent2), w, TRUE, FALSE, 0);
	gtk_signal_connect(
	    GTK_OBJECT(w), "clicked",
	    GTK_SIGNAL_FUNC(CDialogButtonCB), d
	);
	/* Use 'b' as the keyboard accel key for "Abort", since 'a' is
	 * taken by "Yes To All"
	 */
	gtk_accel_group_add(
	    accelgrp,
	    GDK_b, 0,
	    GTK_ACCEL_VISIBLE,
	    GTK_OBJECT(w),
	    "clicked"
	);
	GUIButtonLabelUnderline(w, GDK_b);

	/* Help GtkButton */
	d->help_btn = w = GUIButtonPixmapLabelH(
	    (guint8 **)icon_help_20x20_xpm,
#if defined(PROG_LANGUAGE_SPANISH)
"Ayuda"
#elif defined(PROG_LANGUAGE_FRENCH)
"Aide"
#elif defined(PROG_LANGUAGE_GERMAN)
"Hilfe"
#elif defined(PROG_LANGUAGE_ITALIAN)
"L'Aiuto"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Hulp"
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"Ajuda"
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Hjelp"
#else
"Help"
#endif
	    , NULL
	);
	GTK_WIDGET_SET_FLAGS(w, GTK_CAN_DEFAULT);
	gtk_widget_set_usize(
	    w,
	    GUI_BUTTON_HLABEL_WIDTH_DEF, GUI_BUTTON_HLABEL_HEIGHT_DEF
	);
	gtk_box_pack_start(GTK_BOX(parent2), w, TRUE, FALSE, 0);
	gtk_signal_connect(
	    GTK_OBJECT(w), "clicked",
	    GTK_SIGNAL_FUNC(CDialogButtonCB), d
	);
	gtk_accel_group_add(
	    accelgrp,
	    GDK_h, 0,
	    GTK_ACCEL_VISIBLE,
	    GTK_OBJECT(w),
	    "clicked"
	);
	gtk_accel_group_add(
	    accelgrp,
	    GDK_F1, 0,
	    GTK_ACCEL_VISIBLE,
	    GTK_OBJECT(w),
	    "clicked"
	);
	gtk_accel_group_add(
	    accelgrp,
	    GDK_question, 0,
	    GTK_ACCEL_VISIBLE,
	    GTK_OBJECT(w),
	    "clicked"
	);
	gtk_accel_group_add(
	    accelgrp,
	    GDK_questiondown, 0,
	    GTK_ACCEL_VISIBLE,
	    GTK_OBJECT(w),
	    "clicked"
	);
	gtk_accel_group_add(
	    accelgrp,
	    GDK_Arabic_question_mark, 0,
	    GTK_ACCEL_VISIBLE,
	    GTK_OBJECT(w),
	    "clicked"
	);
	GUIButtonLabelUnderline(w, GDK_h);

	d->freeze_count--;

	return(0);
}

/*
 *	Sets the Dialog's style.
 */
void CDialogSetStyle(GtkRcStyle *rc_style)
{
	GtkWidget *w;
	CDlg *d = cdlg;
	if(d == NULL)
	    return;

	w = d->toplevel;
	if(w != NULL)
	{
	    if(rc_style != NULL)
	    {
		gtk_widget_modify_style_recursive(w, rc_style);
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
 *	Sets the Dialog to be a transient for the given GtkWindow w.
 *
 *	If w is NULL then transient for will be unset.
 */
void CDialogSetTransientFor(GtkWidget *w)
{
	CDlg *d = cdlg;
	if(d == NULL)
	    return;

	if(d->toplevel != NULL)
	{
	    if(w != NULL)
	    {
		if(!GTK_IS_WINDOW(GTK_OBJECT(w)))
		    return;

		if(GTK_WINDOW(w)->modal)
		    gtk_window_set_modal(GTK_WINDOW(w), FALSE);

		gtk_window_set_modal(
		    GTK_WINDOW(d->toplevel), TRUE
		);
		gtk_window_set_transient_for(
		    GTK_WINDOW(d->toplevel), GTK_WINDOW(w)
		);
		d->last_transient_for = w;
	    }
	    else
	    {
		gtk_window_set_modal(
		    GTK_WINDOW(d->toplevel), FALSE
		);
		gtk_window_set_transient_for(
		    GTK_WINDOW(d->toplevel), NULL
		);
		d->last_transient_for = NULL;
	    }
	}
}

/*
 *	Checks if the CDialog is currently waiting for user input.
 */
gboolean CDialogIsQuery(void)
{
	CDlg *d = cdlg;
	if(d == NULL)
	    return(FALSE);

	return((d->main_level > 0) ? TRUE : FALSE);
}

/*
 *	Breaks user query (if any) and causes the querying function to
 *	return CDIALOG_RESPONSE_NOT_AVAILABLE.
 */
void CDialogBreakQuery(void)
{
	CDlg *d = cdlg;
	if(d == NULL)
	    return;

	d->response_code = CDIALOG_RESPONSE_NOT_AVAILABLE;

	/* Break out of all of this dialog's main loops */
	while(d->main_level > 0)
	{
	    gtk_main_quit();
	    d->main_level--;
	}
	d->main_level = 0;
}

/*
 *	Returns the CDialog's toplevel GtkWindow.
 */
GtkWidget *CDialogGetToplevel(void)
{
	CDlg *d = cdlg;
	return((d != NULL) ? d->toplevel : NULL);
}


/*
 *	Maps or unmaps the CDialog's buttons with respect to the
 *	specified button flags.
 *
 *	The button in default_button will be grabbed and set as default.
 */
static void CDialogRemapButtons(
	CDlg *d,
	cdialog_btn_flags show_buttons,		/* Any of CDIALOG_BTNFLAG_* */
	cdialog_btn_flags default_button	/* One of CDIALOG_BTNFLAG_* */
)
{
	GtkWidget *w;

	if(d == NULL)
	    return;

#define DO_MAP_BUTTON(w)		\
{ if((w) != NULL) { gtk_widget_show(w); } }
#define DO_UNMAP_BUTTON(w)		\
{ if((w) != NULL) { gtk_widget_hide(w); } }
#define DO_DEFAULT_BUTTON(w)		\
{ if((w) != NULL) {			\
  gtk_widget_grab_focus(w);		\
  gtk_widget_grab_default(w);		\
} }
#define DO_UNDEFAULT_BUTTON(w)		\
{ if((w) != NULL) {			\
/*  GTK_WIDGET_UNSET_FLAGS(w, GTK_HAS_DEFAULT); \
  GTK_WIDGET_UNSET_FLAGS(w, GTK_RECEIVES_DEFAULT);  */ \
} }

	/* Begin mapping or unmapping buttons */

	/* OK Button */
	w = d->ok_btn;
	if(show_buttons & CDIALOG_BTNFLAG_OK)
	    DO_MAP_BUTTON(w)
	else
	    DO_UNMAP_BUTTON(w)
	if(default_button & CDIALOG_BTNFLAG_OK)
	    DO_DEFAULT_BUTTON(w)
	else
	    DO_UNDEFAULT_BUTTON(w)

	/* Yes Button */
	w = d->yes_btn;
	if(show_buttons & CDIALOG_BTNFLAG_YES)
	    DO_MAP_BUTTON(w)
	else
	    DO_UNMAP_BUTTON(w)
	if(default_button & CDIALOG_BTNFLAG_YES)
	    DO_DEFAULT_BUTTON(w)
	else
	    DO_UNDEFAULT_BUTTON(w)

	/* Yes To All Button */
	w = d->yes_to_all_btn;
	if(show_buttons & CDIALOG_BTNFLAG_YES_TO_ALL)
	    DO_MAP_BUTTON(w)
	else
	    DO_UNMAP_BUTTON(w)
	if(default_button & CDIALOG_BTNFLAG_YES_TO_ALL)
	    DO_DEFAULT_BUTTON(w)
	else
	    DO_UNDEFAULT_BUTTON(w)

	/* No Button */
	w = d->no_btn;
	if(show_buttons & CDIALOG_BTNFLAG_NO)
	    DO_MAP_BUTTON(w)
	else
	    DO_UNMAP_BUTTON(w)
	if(default_button & CDIALOG_BTNFLAG_NO)
	    DO_DEFAULT_BUTTON(w)
	else
	    DO_UNDEFAULT_BUTTON(w)

	/* Cancel Button */
	w = d->cancel_btn;
	if(show_buttons & CDIALOG_BTNFLAG_CANCEL)
	    DO_MAP_BUTTON(w)
	else
	    DO_UNMAP_BUTTON(w)
	if(default_button & CDIALOG_BTNFLAG_CANCEL)
	    DO_DEFAULT_BUTTON(w)
	else
	    DO_UNDEFAULT_BUTTON(w)

	/* Ignore Button */
	w = d->ignore_btn;
	if(show_buttons & CDIALOG_BTNFLAG_IGNORE)
	    DO_MAP_BUTTON(w)
	else
	    DO_UNMAP_BUTTON(w)
	if(default_button & CDIALOG_BTNFLAG_IGNORE)
	    DO_DEFAULT_BUTTON(w)
	else
	    DO_UNDEFAULT_BUTTON(w)

	/* Retry Button */
	w = d->retry_btn;
	if(show_buttons & CDIALOG_BTNFLAG_RETRY)
	    DO_MAP_BUTTON(w)
	else
	    DO_UNMAP_BUTTON(w)
	if(default_button & CDIALOG_BTNFLAG_RETRY)
	    DO_DEFAULT_BUTTON(w)
	else
	    DO_UNDEFAULT_BUTTON(w)

	/* Abort Button */
	w = d->abort_btn;
	if(show_buttons & CDIALOG_BTNFLAG_ABORT)
	    DO_MAP_BUTTON(w)
	else
	    DO_UNMAP_BUTTON(w)
	if(default_button & CDIALOG_BTNFLAG_ABORT)
	    DO_DEFAULT_BUTTON(w)
	else
	    DO_UNDEFAULT_BUTTON(w)

	/* Help Button */
	w = d->help_btn;
	if(show_buttons & CDIALOG_BTNFLAG_HELP)
	    DO_MAP_BUTTON(w)
	else
	    DO_UNMAP_BUTTON(w)
	if(default_button & CDIALOG_BTNFLAG_HELP)
	    DO_DEFAULT_BUTTON(w)
	else
	    DO_UNDEFAULT_BUTTON(w)

#undef DO_MAP_BUTTON
#undef DO_UNMAP_BUTTON
#undef DO_DEFAULT_BUTTON
#undef DO_UNDEFAULT_BUTTON
}

/*
 *	Maps the CDialog and blocks input until a user response is
 *	received.
 *
 *	Returns one of CDIALOG_RESPONSE_*.
 */
gint CDialogGetResponse(
	const gchar *title,
	const gchar *message,
	const gchar *explaination,
	cdialog_icon icon_code,
	cdialog_btn_flags show_buttons,		/* Any of CDIALOG_BTNFLAG_* */
	cdialog_btn_flags default_button	/* One of CDIALOG_BTNFLAG_* */
)
{
	GtkWidget *w, *toplevel;
	CDlg *d = cdlg;
	if(d == NULL)
	    return(CDIALOG_RESPONSE_NOT_AVAILABLE);

	/* Already waiting for user response? */
	if(d->main_level > 0)
	    return(CDIALOG_RESPONSE_NOT_AVAILABLE);

	/* Reset values */
	d->response_code = CDIALOG_RESPONSE_NOT_AVAILABLE;
	d->main_level = 0;

	toplevel = d->toplevel;

	/* Set title */
	if(title != NULL)
	{
	    w = toplevel;
	    if(w != NULL)
		gtk_window_set_title(GTK_WINDOW(w), title);
	}

	/* Set message */
	if(message != NULL)
	{
	    w = d->label;
	    if(w != NULL)
		gtk_label_set_text(GTK_LABEL(w), message);
	}

	/* Set help message */
	w = d->help_label;
	if(w != NULL)
	    gtk_label_set_text(
		GTK_LABEL(w),
		(explaination != NULL) ?
		    explaination : CDIALOG_MESG_NO_HELP_AVAILABLE
	    );

	/* Set icon */
	if(icon_code != d->last_icon_code)
	{
	    guint8 **icon_data = CDialogGetMessageIconDataFromCode(icon_code);
	    d->last_icon_code = icon_code;
	    CDialogLoadIcon(d, icon_data);
	}

	/* Remap buttons */
	CDialogRemapButtons(d, show_buttons, default_button);

	/* Unmap help vbox initially */
	w = d->help_vbox;
	if(w != NULL)
	    gtk_widget_hide(w);

	/* Since new values have been set and widgets have been
	 * mapped/unmapped we need to resize the CDialog
	 */
	if(toplevel != NULL)
	    gtk_widget_queue_resize(toplevel);

#if 0
	/* Center toplevel if not transient for */
/* GTK_WIN_POS_NONE does not seem to work in "resetting" the position */
	w = d->toplevel;
	if((w != NULL) ? GTK_IS_WINDOW(w) : FALSE)
	    gtk_window_set_position(
		GTK_WINDOW(w),
		(d->last_transient_for != NULL) ?
		    GTK_WIN_POS_NONE : GTK_WIN_POS_CENTER
	    );
#endif

	/* Map the CDialog */
	CDialogMap();

	/* Block until user response */
	d->main_level++;
	gtk_main();

	/* Unmap the CDialog */
	CDialogUnmap();

	/* Break out of all of this dialog's main loops */
	while(d->main_level > 0)
	{
	    gtk_main_quit();
	    d->main_level--;
	}
	d->main_level = 0;

	return(d->response_code);
}

/*
 *      Same as CDialogGetResponse() except that the icon is set by
 *	the specified icon data.
 */
gint CDialogGetResponseIconData(
	const gchar *title,
	const gchar *message,
	const gchar *explaination,
	guint8 **icon_data,
	cdialog_btn_flags show_buttons,		/* Any of CDIALOG_BTNFLAG_* */
	cdialog_btn_flags default_button	/* One of CDIALOG_BTNFLAG_* */
)
{
	GtkWidget *w, *toplevel;
	CDlg *d = cdlg;
	if(d == NULL)
	    return(CDIALOG_RESPONSE_NOT_AVAILABLE);

	/* Already waiting for user response? */
	if(d->main_level > 0)
	    return(CDIALOG_RESPONSE_NOT_AVAILABLE);

	/* Reset the values */
	d->response_code = CDIALOG_RESPONSE_NOT_AVAILABLE;
	d->main_level = 0;

	toplevel = d->toplevel;

	/* Set title */
	if(title != NULL)
	{
	    w = toplevel;
	    if(w != NULL)
		gtk_window_set_title(GTK_WINDOW(w), title);
	}

	/* Set message */
	if(message != NULL)
	{
	    w = d->label;
	    if(w != NULL)
		gtk_label_set_text(GTK_LABEL(w), message);
	}

	/* Set help message */
	w = d->help_label;
	if(w != NULL)
	    gtk_label_set_text(
		GTK_LABEL(w),
		(explaination != NULL) ?
		    explaination : CDIALOG_MESG_NO_HELP_AVAILABLE
 	    );

	/* Set icon */
	if(icon_data != NULL)
	{
	    d->last_icon_code = CDIALOG_ICON_USER_DEFINED;
	    CDialogLoadIcon(d, icon_data);
	}

	/* Remap buttons */
	CDialogRemapButtons(d, show_buttons, default_button);

	/* Unmap help vbox initially */
	w = d->help_vbox;
	if(w != NULL)
	    gtk_widget_hide(w);

	/* Since new values have been set and widgets have been
	 * mapped/unmapped we need to resize the CDialog
	 */
	if(toplevel != NULL)
	    gtk_widget_queue_resize(toplevel);

#if 0
	/* Center toplevel if not transient for */
	w = d->toplevel;
	if((w != NULL) ? GTK_IS_WINDOW(w) : FALSE)
	    gtk_window_set_position(
		GTK_WINDOW(w),
		(d->last_transient_for != NULL) ?
		    GTK_WIN_POS_NONE : GTK_WIN_POS_CENTER
	    );
#endif

	/* Map the CDialog */
	CDialogMap();

	/* Block until user response */
	d->main_level++;
	gtk_main();

	/* Unmap the CDialog */
	CDialogUnmap();

	/* Break out of all of this dialog's main loops */
	while(d->main_level > 0)
	{
	    gtk_main_quit();
	    d->main_level--;
	}
	d->main_level = 0;

	return(d->response_code);
}

/*
 *	Similar to CDialogGetResponse() except that it displays a
 *	message from the file specified by message_path.
 */
gint CDialogGetResponseFile(
	const gchar *title,
	const gchar *filename,
	const gchar *explaination,
	cdialog_icon icon_code,
	cdialog_btn_flags show_buttons,		/* Any of CDIALOG_BTNFLAG_* */
	cdialog_btn_flags default_button	/* One of CDIALOG_BTNFLAG_* */
)
{
	gint response;
	gchar *buf;
	FILE *fp = (filename != NULL) ?
	    fopen((const char *)filename, "rb") : NULL;
	if(fp != NULL)
	{
	    gint	buf_len = 2048,
			bytes_read;

	    buf = (gchar *)g_malloc(buf_len * sizeof(gchar));
	    if(buf != NULL)
		bytes_read = (gint)fread(buf, sizeof(gchar), (size_t)buf_len, fp);
	    else
		bytes_read = 0;
	    if(bytes_read < 0)
		bytes_read = 0;
	    else if(bytes_read >= buf_len)
		bytes_read = buf_len - 1;
	    buf[bytes_read] = '\0';

	    fclose(fp);
	}
	else
	{
	    const gint error_code = errno;
	    if(filename != NULL)
	    {
		gchar *error_msg = STRDUP(g_strerror(error_code));
		if(error_msg != NULL)
		{
		    *error_msg = (gchar)toupper((int)(*error_msg));
		    buf = g_strdup_printf(
"%s:\n\
\n\
    %s",
			error_msg,
			filename
		    );
		    g_free(error_msg);
		}
		else
		{
		    buf = STRDUP("Unable to open the file for reading");
		}
	    }
	    else
	    {
		buf = STRDUP("No file specified");
	    }
	}

	/* Map the CDialog using the message loaded from the file
	 * and wait until user response
	 */
	response = CDialogGetResponse(
	    title,			/* Title */
	    buf,			/* Message */
	    explaination,		/* Help Message */
	    icon_code,			/* Icon Code */
	    show_buttons,		/* Buttons */
	    default_button		/* Default Button */
	);

	g_free(buf);

	return(response);
}

/*
 *	Maps the CDialog.
 */
void CDialogMap(void)
{
	CDlg *d = cdlg;
	GtkWidget *w = (d != NULL) ? d->toplevel : NULL;
	if(w == NULL)
	    return;

	gtk_widget_show_raise(w);
}

/*
 *	Unmaps the CDialog.
 */
void CDialogUnmap(void)
{
	CDlg *d = cdlg;
	GtkWidget *w = (d != NULL) ? d->toplevel : NULL;
	if(w == NULL)
	    return;

	gtk_widget_hide(w);
}

/*
 *	Shuts down the CDialog.
 */
void CDialogShutdown(void)
{
	CDlg *d = cdlg;
	if(d == NULL)
	    return;

	d->response_code = CDIALOG_RESPONSE_NOT_AVAILABLE;

	/* Break out of all of this dialog's main loops */
	while(d->main_level > 0)
	{
	    gtk_main_quit();
	    d->main_level--;
	}
	d->main_level = 0;

	/* Unmap the dialog */
	CDialogUnmap();

	/* Clear the dialog pointer before deleting the dialog */
	cdlg = NULL;

	/* Delete this dialog */
	d->freeze_count++;

	gtk_widget_destroy(d->toplevel);
	gtk_accel_group_unref(d->accelgrp);

	d->freeze_count--;

	g_free(d);
}
