#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "../include/disk.h"

#include "guiutils.h"
#include "pulist.h"
#include "pdialog.h"
#include "config.h"

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
#include "images/icon_edit_32x32.xpm"
#include "images/icon_install_32x32.xpm"
#include "images/icon_uninstall_32x32.xpm"

#include "images/icon_file_32x32.xpm"
#include "images/icon_folder_closed_32x32.xpm"
#include "images/icon_folder_opened_32x32.xpm"
#include "images/icon_link2_32x32.xpm"
#include "images/icon_pipe_32x32.xpm"
#include "images/icon_device_misc_32x32.xpm"
#include "images/icon_device_block_32x32.xpm"
#include "images/icon_device_character_32x32.xpm"
#include "images/icon_socket_32x32.xpm"

/* TODO */
#include "images/icon_move_file_32x32.xpm"
#include "images/icon_copy_file_32x32.xpm"
#include "images/icon_properties2_32x32.xpm"
#include "images/icon_planet_32x32.xpm"
#include "images/icon_ftp_32x32.xpm"
#include "images/icon_chat_32x32.xpm"
#include "images/icon_file_www_32x32.xpm"


/* Button icons */
#include "images/icon_browse_20x20.xpm"
#include "images/icon_ok_20x20.xpm"
#include "images/icon_cancel_20x20.xpm"
#include "images/icon_help_20x20.xpm"


/*
 *	Prompt Types:
 */
typedef enum {
	PDIALOG_PROMPT_TYPE_LABEL,
	PDIALOG_PROMPT_TYPE_ENTRY,
	PDIALOG_PROMPT_TYPE_SPIN,
	PDIALOG_PROMPT_TYPE_SCALE,
	PDIALOG_PROMPT_TYPE_POPUP_LIST,
	PDIALOG_PROMPT_TYPE_COMBO,
	PDIALOG_PROMPT_TYPE_RADIO,
	PDIALOG_PROMPT_TYPE_TOGGLE,
	PDIALOG_PROMPT_TYPE_SEPARATOR
} pdialog_prompt_type;


typedef struct _pdialog_struct		pdialog_struct;
#define PDIALOG(p)			((pdialog_struct *)(p))
typedef struct _pdialog_prompt_struct	pdialog_prompt_struct;
#define PDIALOG_PROMPT(p)		((pdialog_prompt_struct *)(p))


/*
 *	Prompt Dialog Prompt:
 */
struct _pdialog_prompt_struct {

	pdialog_prompt_type	type;
	GtkWidget       *toplevel,
			*icon_pm,
			*label,
			*value,
			*browse_btn,
			*pulistbox,
			*radio_toplevel, **radio;
	gchar		**radio_label;
	gint		total_radios;

	gchar	*(*browse_cb)(
		gpointer,	/* PDialog */
		gpointer,	/* Data */
		gint		/* Prompt Number */
	);
	gpointer	browse_data;

};


/*
 *	Prompt Dialog:
 */
struct _pdialog_struct {

	gboolean	initialized;
	GtkAccelGroup   *accelgrp;
	GtkWidget       *toplevel,
			*main_vbox,	/* GtkBox for the prompts */
			*icon_pm,
			*message_label_hbox,
			*message_label;

	pdialog_prompt_struct	**prompt;
	gint			total_prompts;

	GtkWidget       *submit_btn,
			*submit_btn_label,
			*cancel_btn,
			*cancel_btn_label,
			*help_btn;

	GtkWidget	*last_transient_for;	/* Do not reference */
	pdialog_icon	last_icon_code;

	gint		block_loop_level;

	gchar		**response_val;
	gint		response_nvals;
};


/* Callbacks */
static gint PDialogCloseCB(
	GtkWidget *widget, GdkEvent *event, gpointer data
);
static void PDialogBrowseButtonCB(GtkWidget *widget, gpointer data);
static void PDialogEntryEnterCB(GtkWidget *widget, gpointer data);
static gint PDialogEntryCompletePathKeyCB(
	GtkWidget *widget, GdkEventKey *key, gpointer data
);
static void PDialogButtonCB(GtkWidget *widget, gpointer data);
static void PDialogPromptDragDataReceivedCB(
	GtkWidget *widget, GdkDragContext *dc, gint x, gint y,
	GtkSelectionData *selection_data, guint info, guint t,
	gpointer data
);

/* Utilities */
static guint8 **PDialogGetMessageIconDataFromCode(pdialog_icon icon_code);
static void PDialogSetIcon(pdialog_struct *d, guint8 **icon_data);
static pdialog_prompt_struct *PDialogPromptNewNexus(
	const pdialog_prompt_type type,
	const guint8 **icon_data,
	const gchar *label,
	const gchar *value,
	const gboolean hide_value,	/* For passwords */
	pdialog_struct *d,
	gchar *(*browse_cb)(gpointer, gpointer, gint),
	gpointer browse_data
);
static void PDialogPromptDelete(pdialog_prompt_struct *p);

/* Front Ends */
gint PDialogInit(void);
void PDialogSetStyle(GtkRcStyle *rc_style);
void PDialogSetTransientFor(GtkWidget *w);
gboolean PDialogIsQuery(void);
void PDialogBreakQuery(void);
GtkWidget *PDialogGetToplevel(void);

static pdialog_prompt_struct *PDialogAddPromptNexus(
	const pdialog_prompt_type type,
	const guint8 **icon_data,
	const gchar *label,
	const gchar *value,
	const gboolean hide_value,	/* For passwords */
	gchar *(*browse_cb)(gpointer, gpointer, gint),
	gpointer browse_data
);
void PDialogAddPromptLabel(const gchar *label);
void PDialogAddPrompt(
	const guint8 **icon_data,	/* Can be NULL */
	const gchar *label,		/* Can be NULL */
	const gchar *value		/* Can be NULL */
);
void PDialogAddPromptPassword(
	const guint8 **icon_data,
	const gchar *label,
	const gchar *value
);
void PDialogAddPromptWithBrowse(
	const guint8 **icon_data,	/* Can be NULL */
	const gchar *label,		/* Can be NULL */
	const gchar *value,		/* Can be NULL */
	gpointer browse_data,		/* Can be NULL */
	gchar *(*browse_cb)(gpointer, gpointer, gint)	/* Can be NULL */
);
void PDialogAddPromptSpin(
	const guint8 **icon_data,
	const gchar *label,
	gfloat value, gfloat lower, gfloat upper,
	gfloat step_increment, gfloat page_increment,
	gfloat climb_rate, guint digits
);
void PDialogAddPromptScale(
	const guint8 **icon_data,
	const gchar *label,
	gfloat value, gfloat lower, gfloat upper,
	gfloat step_increment, gfloat page_increment,
	gboolean show_value, guint digits
);
void PDialogAddPromptPopupList(
	const guint8 **icon_data,
	const gchar *label,
	GList *list,			/* List of values */
	const gint start_num,		/* Initial value */
	const gint nitems_visible	/* Number of items visible */
);
void PDialogAddPromptCombo(
	const guint8 **icon_data,
	const gchar *label,
	const gchar *value,		/* Initial value */
	GList *list,			/* List of values */
	const gboolean editable,
	const gboolean case_sensitive
);
void PDialogAddPromptRadio(
	const guint8 **icon_data,
	const gchar *label,
	GList *list,			/* List of values */
	const gint start_num		/* Initial value */
);
void PDialogAddPromptToggle(
	const guint8 **icon_data,
	const gchar *label, gboolean value
);
void PDialogAddPromptSeparator(void);
void PDialogSetPromptValue(
	gint prompt_num,
	const guint8 **icon_data,
	const gchar *label,
	const gchar *value
);
void PDialogSetPromptTip(gint prompt_num, const gchar *tip);
const gchar *PDialogGetPromptValue(gint prompt_num);
void PDialogSetPromptCompletePath(gint prompt_num);
void PDialogDeleteAllPrompts(void);
gchar **PDialogGetResponse(
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
gchar **PDialogGetResponseIconData(
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
void PDialogSetSize(gint width, gint height);
void PDialogMap(void);
void PDialogUnmap(void);
void PDialogShutdown(void);


#define ATOI(s)		(((s) != NULL) ? atoi(s) : 0)
#define ATOL(s)		(((s) != NULL) ? atol(s) : 0)
#define ATOF(s)		(((s) != NULL) ? atof(s) : 0.0f)
#define STRDUP(s)	(((s) != NULL) ? g_strdup(s) : NULL)

#define MAX(a,b)	(((a) > (b)) ? (a) : (b))
#define MIN(a,b)	(((a) < (b)) ? (a) : (b))
#define CLIP(a,l,h)	(MIN(MAX((a),(l)),(h)))
#define STRLEN(s)	(((s) != NULL) ? strlen(s) : 0)
#define STRISEMPTY(s)	(((s) != NULL) ? (*(s) == '\0') : TRUE)


static pdialog_struct	pdialog;


/*
 *	Dialog toplevel GtkWindow "delete_event" signal callback.
 */
static gint PDialogCloseCB(
	GtkWidget *widget, GdkEvent *event, gpointer data
)
{
	pdialog_struct *d = PDIALOG(data);
	if((widget == NULL) || (d == NULL))
	    return(TRUE);

	/* Break out of one main loop level */
	gtk_main_quit();
	d->block_loop_level--;

	return(TRUE);
}

/*
 *	Dialog prompt browse GtkButton "clicked" signal callback.
 */
static void PDialogBrowseButtonCB(GtkWidget *widget, gpointer data)
{
	gint i;
	const gchar *s;
	GtkWidget *w;
	pdialog_prompt_struct *p;
	pdialog_struct *d = PDIALOG(data);
	if((widget == NULL) || (d == NULL))
	    return;

	/* Check which browse button was pressed */
	for(i = 0; i < d->total_prompts; i++)
	{
	    p = d->prompt[i];
	    if(p == NULL)
		continue;

	    if(widget != p->browse_btn)
	        continue;

	    if(p->browse_cb != NULL)
	    {
		s = (const gchar *)p->browse_cb(
		    d,			/* Prompt dialog */
		    p->browse_data,	/* Data */
		    i			/* Prompt number */
		);
		w = p->value;
		if((w != NULL) && (s != NULL))
		{
		    if(GTK_IS_ENTRY(w))
			gtk_entry_set_text(GTK_ENTRY(w), s);
		}
	    }
	    break;
	}
}

/*
 *	Dialog prompt GtkEntry "activate" signal callback.
 */
static void PDialogEntryEnterCB(GtkWidget *widget, gpointer data)
{
	pdialog_struct *d = PDIALOG(data);
	if((widget == NULL) || (d == NULL))
	    return;

	/* Call button callback and pass the submit button as the
	 * widget
	 */
	PDialogButtonCB(d->submit_btn, d);
}

/*
 *	Dialog prompt GtkEntry complete path "key_press_event" or
 *	"key_release_event" signal callback.
 */
static gint PDialogEntryCompletePathKeyCB(
	GtkWidget *widget, GdkEventKey *key, gpointer data
)
{
	gint status = 0;
	gboolean press;
	GtkEntry *entry;
	pdialog_struct *d = PDIALOG(data);
	if((widget == NULL) || (key == NULL) || (d == NULL))
	    return(status);

	press = (key->type == GDK_KEY_PRESS) ? TRUE : FALSE;

	/* Given widget must have or be a GtkEntry */
	if(GTK_IS_COMBO(widget))
	{
	    GtkCombo *combo = GTK_COMBO(widget);
	    entry = GTK_ENTRY(combo->entry);
	}
	else if(GTK_IS_ENTRY(widget))
	{
	    entry = GTK_ENTRY(widget);
	}
	else
	{
	    return(status);
	}

#define SIGNAL_EMIT_STOP	{		\
 gtk_signal_emit_stop_by_name(			\
  GTK_OBJECT(widget),				\
  press ?					\
   "key_press_event" : "key_release_event"	\
 );						\
}

	switch(key->keyval)
	{
	  case GDK_Tab:
	    /* Skip this if the shift or ctrl keys are held */
	    if((key->state & GDK_CONTROL_MASK) ||
	       (key->state & GDK_SHIFT_MASK)
	    )
		return(status);

	    if(press)
	    {
		gchar *path = STRDUP(gtk_entry_get_text(entry));
		if(path != NULL)
		{
		    gchar *prev_path = STRDUP(path);
		    gint status;

		    /* Complete the path */
		    path = (gchar *)CompletePath((char *)path, &status);
		    gtk_entry_set_text(entry, (path != NULL) ? path : "");
		    gtk_entry_set_position(entry, -1);

		    /* Beep if there was no change in the path */
		    if((prev_path != NULL) && (path != NULL))
		    {
			if(!strcmp((const char *)prev_path, (const char *)path))
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
 *	Dialog GtkButton "clicked" signal callback.
 */
static void PDialogButtonCB(GtkWidget *widget, gpointer data)
{
	pdialog_struct *d = PDIALOG(data);
	if((widget == NULL) || (d == NULL))
	    return;

	/* Check which widget this signal is for */

	/* Submit */
	if(widget == d->submit_btn)
	{
	    gint i;

	    /* Clear the existing responses values */
	    for(i = 0; i < d->response_nvals; i++)
		g_free(d->response_val[i]);
	    g_free(d->response_val);
	    d->response_val = NULL;
	    d->response_nvals = 0;

	    /* Fetch values from each prompt and copy them to
	     * the response values
	     *
	     * The number of response values is equal to the number
	     * of prompts on the dialog
	     */
	    d->response_nvals = d->total_prompts;
	    d->response_val = (d->response_nvals > 0) ?
		(gchar **)g_malloc0(d->response_nvals * sizeof(gchar *)) : NULL;
	    if(d->response_val == NULL)
	    {
		d->response_nvals = 0;
	    }
	    else
	    {
		GtkWidget *w;
		pdialog_prompt_struct *p;

		/* Fetch values for each response value, thus going
		 * through each prompt
		 */
		for(i = 0; i < d->response_nvals; i++)
		{
		    p = d->prompt[i];
		    if(p == NULL)
		    {
			d->response_val[i] = STRDUP("");
			continue;
		    }

		    w = p->value;

		    switch(p->type)
		    {
		      case PDIALOG_PROMPT_TYPE_LABEL:
			d->response_val[i] = STRDUP("");
			break;

		      case PDIALOG_PROMPT_TYPE_ENTRY:
			if(GTK_IS_ENTRY(w))
			{
			    const gchar *s = gtk_entry_get_text(GTK_ENTRY(w));
			    d->response_val[i] = STRDUP((s != NULL) ? s : "");
			}
			break;

		      case PDIALOG_PROMPT_TYPE_SPIN:
			if(GTK_IS_SPIN_BUTTON(w))
			{
			    const gchar *s = gtk_entry_get_text(GTK_ENTRY(w));
			    d->response_val[i] = STRDUP((s != NULL) ? s : "");
			}
			break;

		      case PDIALOG_PROMPT_TYPE_SCALE:
			if(GTK_IS_SCALE(w))
			{
			    GtkRange *range = GTK_RANGE(w);
			    GtkAdjustment *adj = range->adjustment;
			    if(adj != NULL)
			    {
				gchar fmt_str[80];
				g_snprintf(
				    fmt_str, sizeof(fmt_str),
				    "%%.%if", 
				    range->digits
				);
				d->response_val[i] = g_strdup_printf(
				    fmt_str, adj->value
				);
			    }
			}
			break;

		      case PDIALOG_PROMPT_TYPE_POPUP_LIST:
			if(p->pulistbox != NULL)
			{
			    GtkWidget *pulist = PUListBoxGetPUList(p->pulistbox);
			    const gchar *v;
			    PUListGetItemText(
				pulist,
				PUListGetSelectedLast(pulist),
				&v
			    );
			    d->response_val[i] = STRDUP((v != NULL) ? v : "");
			}
			break;

		      case PDIALOG_PROMPT_TYPE_COMBO:
			if(GTK_IS_COMBO(w))
			{
			    GtkCombo *combo = GTK_COMBO(w);
			    const gchar *s = gtk_entry_get_text(
				GTK_ENTRY(combo->entry)
			    );
			    d->response_val[i] = STRDUP((s != NULL) ? s : "");
			}
			break;

		      case PDIALOG_PROMPT_TYPE_RADIO:
			if(p->total_radios > 0)
			{
			    gint n;
			    for(n = 0; n < p->total_radios; n++)
			    {
				if(GTK_TOGGLE_BUTTON_GET_ACTIVE(p->radio[n]))
				{
				    const gchar *s = p->radio_label[n];
				    d->response_val[i] = STRDUP(
					(s != NULL) ? s : ""
				    );
				    break;
				}
			    }
			}
			break;

		      case PDIALOG_PROMPT_TYPE_TOGGLE:
			if(GTK_IS_TOGGLE_BUTTON(w))
			{
			    GtkToggleButton *tb = GTK_TOGGLE_BUTTON(w);
			    d->response_val[i] = STRDUP(
				(tb->active) ? "1" : "0"
			    );
			}
			break;

		      case PDIALOG_PROMPT_TYPE_SEPARATOR:
			d->response_val[i] = STRDUP("");
			break;
		    }
		}
	    }

	    /* Break out of one main loop level */
	    gtk_main_quit();
	    d->block_loop_level--;
	}
	/* Cancel */
	else if(widget == d->cancel_btn)
	{
	    /* Break out of one main loop level */
	    gtk_main_quit();
	    d->block_loop_level--;
	}
	/* Help */
	else if(widget == d->help_btn)
	{

	}
}

/*
 *	Prompt GtkEntry "drag_data_received" signal callback.
 */
static void PDialogPromptDragDataReceivedCB(
	GtkWidget *widget, GdkDragContext *dc, gint x, gint y,
	GtkSelectionData *selection_data, guint info, guint t,
	gpointer data
)
{
	if((widget == NULL) || (dc == NULL) || (selection_data == NULL))
	    return;

	if((selection_data->data == NULL) ||
	   (selection_data->length <= 0)
	)
	    return;

	/* Handle by the target type
	 *
	 * String
	 */
	if((info == 0) ||	/* GUI_TARGET_NAME_TEXT_PLAIN */
	   (info == 1) ||	/* GUI_TARGET_NAME_TEXT_URI_LIST */
	   (info == 2)		/* GUI_TARGET_NAME_STRING */
	)
	{
	    gint len = selection_data->length;
	    const gchar *ss = (const gchar *)selection_data->data;
	    gchar *s;

	    if(ss[len - 1] == '\0')
		len--;

	    s = (gchar *)g_malloc(len + (1 * sizeof(gchar)));
	    if(s != NULL)
	    {
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

		/* Insert the string */
		if(GTK_IS_EDITABLE(widget))
		{
		    GtkEditable *editable = GTK_EDITABLE(widget);
		    gint position = gtk_editable_get_position(editable);
		    if(len > 0)
			gtk_editable_insert_text(
			    editable,
			    s, len,
			    &position
			);
		}

		g_free(s);
	    }
	}
}


/*
 *	Returns the icon data for the specified icon code.
 */
static guint8 **PDialogGetMessageIconDataFromCode(pdialog_icon icon_code)
{
	guint8 **d = (guint8 **)icon_question_32x32_xpm;

	switch(icon_code)
	{
	  case PDIALOG_ICON_INFO:
	    d = (guint8 **)icon_info_32x32_xpm;
	    break;
	  case PDIALOG_ICON_WARNING:
	    d = (guint8 **)icon_warning_32x32_xpm;
	    break;
	  case PDIALOG_ICON_ERROR:
	    d = (guint8 **)icon_error_32x32_xpm;
	    break;
	  case PDIALOG_ICON_QUESTION:
	    d = (guint8 **)icon_question_32x32_xpm;
	    break;
	  case PDIALOG_ICON_HELP:
	    d = (guint8 **)icon_help_32x32_xpm;
	    break;
	  case PDIALOG_ICON_WIZARD:
	    d = (guint8 **)icon_wand_32x32_xpm;
	    break;
	  case PDIALOG_ICON_SEARCH:
	    d = (guint8 **)icon_search_32x32_xpm;
	    break;
	  case PDIALOG_ICON_SECURITY:
	    d = (guint8 **)icon_security_32x32_xpm;
	    break;
	  case PDIALOG_ICON_PRINTER:
	    d = (guint8 **)icon_print2_32x32_xpm;
	    break;
	  case PDIALOG_ICON_SPEAKER:
	    d = (guint8 **)icon_sound_32x32_xpm;
	    break;
	  case PDIALOG_ICON_BULB:
	    d = (guint8 **)icon_bulb_32x32_xpm;
	    break;
	  case PDIALOG_ICON_POWER:
	    d = (guint8 **)icon_power_32x32_xpm;
	    break;
	  case PDIALOG_ICON_OS:
#if defined(__linux__)
	    d = (guint8 **)icon_linux_32x32_xpm;
#else
	    d = (guint8 **)icon_linux_32x32_xpm;
#endif
	    break;
	  case PDIALOG_ICON_TERMINAL:
	    d = (guint8 **)icon_terminal2_32x32_xpm;
	    break;
	  case PDIALOG_ICON_SETTINGS:
	    d = (guint8 **)icon_tuning_32x32_xpm;
	    break;
	  case PDIALOG_ICON_TOOLS:
	    d = (guint8 **)icon_tools_32x32_xpm;
	    break;
	  case PDIALOG_ICON_MONITOR:
	    d = (guint8 **)icon_monitor2_32x32_xpm;
	    break;
	  case PDIALOG_ICON_CLIPBOARD_EMPTY:
	    d = (guint8 **)icon_clipboard_empty_32x32_xpm;
	    break;
	  case PDIALOG_ICON_CLIPBOARD_FULL:
	    d = (guint8 **)icon_clipboard_32x32_xpm;
	    break;
	  case PDIALOG_ICON_EDIT:
	    d = (guint8 **)icon_edit_32x32_xpm;
	    break;
	  case PDIALOG_ICON_INSTALL:
	    d = (guint8 **)icon_install_32x32_xpm;
	    break;
	  case PDIALOG_ICON_UNINSTALL:
	    d = (guint8 **)icon_uninstall_32x32_xpm;
	    break;

	  case PDIALOG_ICON_FILE:
	    d = (guint8 **)icon_file_32x32_xpm;
	    break;
	  case PDIALOG_ICON_FOLDER_CLOSER:
	    d = (guint8 **)icon_folder_closed_32x32_xpm;
	    break;
	  case PDIALOG_ICON_FOLDER_OPENED:
	    d = (guint8 **)icon_folder_opened_32x32_xpm;
	    break;
	  case PDIALOG_ICON_LINK:
	    d = (guint8 **)icon_link2_32x32_xpm;
	    break;
	  case PDIALOG_ICON_PIPE:
	    d = (guint8 **)icon_pipe_32x32_xpm;
	    break;
	  case PDIALOG_ICON_DEVICE:
	    d = (guint8 **)icon_device_misc_32x32_xpm;
	    break;
	  case PDIALOG_ICON_DEVICE_BLOCK:
	    d = (guint8 **)icon_device_block_32x32_xpm;
	    break;
	  case PDIALOG_ICON_DEVICE_CHARACTER:
	    d = (guint8 **)icon_device_character_32x32_xpm;
	    break;
	  case PDIALOG_ICON_SOCKET:
	    d = (guint8 **)icon_socket_32x32_xpm;
	    break;

	  case PDIALOG_ICON_FILE_MOVE:
	    d = (guint8 **)icon_move_file_32x32_xpm;
	    break;
	  case PDIALOG_ICON_FILE_COPY:
	    d = (guint8 **)icon_copy_file_32x32_xpm;
	    break;
	  case PDIALOG_ICON_FILE_PROPERTIES:
	    d = (guint8 **)icon_properties2_32x32_xpm;
	    break;

	  case PDIALOG_ICON_PLANET:
	    d = (guint8 **)icon_planet_32x32_xpm;
	    break;
	  case PDIALOG_ICON_FTP:
	    d = (guint8 **)icon_ftp_32x32_xpm;
	    break;
	  case PDIALOG_ICON_CHAT:
	    d = (guint8 **)icon_chat_32x32_xpm;
	    break;
	  case PDIALOG_ICON_FILE_WWW:
	    d = (guint8 **)icon_file_www_32x32_xpm;
	    break;

	  case PDIALOG_ICON_USER_DEFINED:
	    break;
	}
	return(d);
}

/*
 *	Updates the icon for the dialog. Updates the member icon_pm
 *	as needed.
 */
static void PDialogSetIcon(pdialog_struct *d, guint8 **icon_data)
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
 *	Allocates and creates a new prompt structure, but does not 
 *	add it to the prompt dialog.
 */
static pdialog_prompt_struct *PDialogPromptNewNexus(
	const pdialog_prompt_type type,
	const guint8 **icon_data,
	const gchar *label,
	const gchar *value,
	const gboolean hide_value,	/* For passwords */
	pdialog_struct *d,
	gchar *(*browse_cb)(gpointer, gpointer, gint),
	gpointer browse_data
)
{
	const gint	border_major = 5,
			border_minor = 2;
	GdkWindow *window;
	GtkStyle *style;
	GtkWidget *w, *parent;
	pdialog_prompt_struct *p = PDIALOG_PROMPT(
	    g_malloc0(sizeof(pdialog_prompt_struct))
	);
	if(p == NULL)
	    return(p);

	w = d->toplevel;
	window = (w != NULL) ? w->window : NULL;
	style = gtk_widget_get_style(w); 

	p->type = type;
	p->toplevel = parent = w = gtk_hbox_new(FALSE, border_minor);

	/* Create icon? */
	if(icon_data != NULL)
	{
	    GdkBitmap *mask;
	    GdkPixmap *pixmap = GDK_PIXMAP_NEW_FROM_XPM_DATA(
		&mask, (guint8 **)icon_data
	    );
	    if(pixmap != NULL)
	    {
		gint width, height;
		gdk_window_get_size(pixmap, &width, &height);
		p->icon_pm = w = gtk_pixmap_new(pixmap, mask);
		gtk_widget_set_usize(w, width, height);
		gtk_box_pack_start(GTK_BOX(parent), w, FALSE, FALSE, 0);
		gtk_widget_show(w);

		GDK_PIXMAP_UNREF(pixmap);
		GDK_BITMAP_UNREF(mask);
	    }
	}

	/* Create label? */
	if((label != NULL) &&
	   (type != PDIALOG_PROMPT_TYPE_LABEL) &&
	   (type != PDIALOG_PROMPT_TYPE_TOGGLE) &&
	   (type != PDIALOG_PROMPT_TYPE_SEPARATOR)
	)
	{
	    p->label = w = gtk_label_new(label);
	    gtk_label_set_justify(GTK_LABEL(w), GTK_JUSTIFY_RIGHT);
	    gtk_box_pack_start(GTK_BOX(parent), w, FALSE, FALSE, 0);
	    gtk_widget_show(w);
	}
 
	/* Primary input widget */
	if(TRUE)
	{
	    const GtkTargetEntry dnd_tar_types[] = {
{GUI_TARGET_NAME_TEXT_PLAIN,	0,		0},
{GUI_TARGET_NAME_TEXT_URI_LIST,	0,		1},
{GUI_TARGET_NAME_STRING,	0,		2}
	    };
	    GtkWidget *w2;

	    /* Create the primary input widget by the prompt type */
	    switch(type)
	    {
	      case PDIALOG_PROMPT_TYPE_LABEL:
		p->value = w = gtk_label_new((value != NULL) ? value : "");
		gtk_label_set_justify(GTK_LABEL(w), GTK_JUSTIFY_LEFT);
		gtk_box_pack_start(GTK_BOX(parent), w, FALSE, FALSE, 0);
		gtk_widget_show(w);
		break;

	      case PDIALOG_PROMPT_TYPE_ENTRY:
		p->value = w = gtk_entry_new();
		gtk_entry_set_visibility(GTK_ENTRY(w), !hide_value);
		gtk_box_pack_start(GTK_BOX(parent), w, TRUE, TRUE, 0);
		GUIDNDSetTar(
		    w,
		    dnd_tar_types,
		    sizeof(dnd_tar_types) / sizeof(GtkTargetEntry),
		    GDK_ACTION_COPY,		/* Actions */
		    GDK_ACTION_COPY,		/* Default action if same */
		    GDK_ACTION_COPY,		/* Default action */
		    PDialogPromptDragDataReceivedCB,
		    w,
		    TRUE			/* Highlight */
		);
		if(value != NULL)
		{
		    gtk_entry_set_text(GTK_ENTRY(w), value);
		    gtk_entry_set_position(GTK_ENTRY(w), -1);
		}
		gtk_signal_connect(
		    GTK_OBJECT(w), "activate",
		    GTK_SIGNAL_FUNC(PDialogEntryEnterCB), d
		);
		GUIEditableEndowPopupMenu(w, 0);
		gtk_widget_show(w);
		break;

	      case PDIALOG_PROMPT_TYPE_SPIN:
		p->value = w = gtk_spin_button_new(NULL, 1.0f, 0);
		gtk_box_pack_start(GTK_BOX(parent), w, TRUE, TRUE, 0);
		gtk_signal_connect(
		    GTK_OBJECT(w), "activate",
		    GTK_SIGNAL_FUNC(PDialogEntryEnterCB), d
		);
		GUIEditableEndowPopupMenu(w, 0);
		gtk_widget_show(w);
		break;

	      case PDIALOG_PROMPT_TYPE_SCALE:
		p->value = w = gtk_hscale_new(NULL);
		gtk_box_pack_start(GTK_BOX(parent), w, TRUE, TRUE, 0);
		gtk_widget_show(w);
		break;

	      case PDIALOG_PROMPT_TYPE_POPUP_LIST:
		p->pulistbox = w = PUListBoxNew(
		    -1, -1
		);
		gtk_box_pack_start(GTK_BOX(parent), w, TRUE, TRUE, 0);
		gtk_widget_show(w);
		break;

	      case PDIALOG_PROMPT_TYPE_COMBO:
		p->value = w = gtk_combo_new();
		gtk_box_pack_start(GTK_BOX(parent), w, TRUE, TRUE, 0);
		gtk_combo_disable_activate(GTK_COMBO(w));
		w2 = GTK_COMBO(w)->entry;
		gtk_entry_set_visibility(GTK_ENTRY(w2), !hide_value);
		GUIDNDSetTar(
		    w2,
		    dnd_tar_types,
		    sizeof(dnd_tar_types) / sizeof(GtkTargetEntry),
		    GDK_ACTION_COPY,		/* Actions */
		    GDK_ACTION_COPY,		/* Default action if same */
		    GDK_ACTION_COPY,		/* Default action */
		    PDialogPromptDragDataReceivedCB,
		    w2,
		    TRUE			/* Highlight */
		);
		if(value != NULL)
		{
		    gtk_entry_set_text(GTK_ENTRY(w2), value);
		    gtk_entry_set_position(GTK_ENTRY(w2), -1);
		}
		gtk_signal_connect(
		    GTK_OBJECT(w2), "activate",
		    GTK_SIGNAL_FUNC(PDialogEntryEnterCB), d
		);
		GUIEditableEndowPopupMenu(w2, 0);
		gtk_widget_show(w);
		break;

	      case PDIALOG_PROMPT_TYPE_RADIO:
		p->radio_toplevel = w = gtk_hbox_new(FALSE, border_major);
		gtk_box_pack_start(GTK_BOX(parent), w, TRUE, TRUE, 0);
		gtk_widget_show(w);
		/* Do not create each radio button, they will be
		 * created by the calling function
		 */
		break;

	     case PDIALOG_PROMPT_TYPE_TOGGLE:
		if(label != NULL)
		    w = gtk_check_button_new_with_label(label);
		else
		    w = gtk_check_button_new();
		p->value = w;
		gtk_box_pack_start(GTK_BOX(parent), w, TRUE, TRUE, 0);
		gtk_widget_show(w);
		break;

	      case PDIALOG_PROMPT_TYPE_SEPARATOR:
		p->value = w = gtk_hseparator_new();
		gtk_box_pack_start(GTK_BOX(parent), w, TRUE, TRUE, 0);
		gtk_widget_show(w);
		break;
	    }
	}

	/* Create browse button? */
	if(browse_cb != NULL)
	{
	    /* Set browse callback function and client data */
	    p->browse_cb = browse_cb;
	    p->browse_data = browse_data;

	    /* Create browse button */
	    p->browse_btn = w = GUIButtonPixmap(
		(guint8 **)icon_browse_20x20_xpm
	    );
	    gtk_signal_connect(
		GTK_OBJECT(w), "clicked",
		GTK_SIGNAL_FUNC(PDialogBrowseButtonCB), d
	    );
	    gtk_box_pack_start(GTK_BOX(parent), w, FALSE, FALSE, 0);
	    GUISetWidgetTip(
		w,
#ifdef PROG_LANGUAGE_ENGLISH
		"Browse"
#endif
#ifdef PROG_LANGUAGE_SPANISH
		"Hojee"
#endif
#ifdef PROG_LANGUAGE_FRENCH
		"Naviguer"
#endif
#ifdef PROG_LANGUAGE_GERMAN
		"Brausen"
#endif
#ifdef PROG_LANGUAGE_ITALIAN
		"Curiosare"
#endif
#ifdef PROG_LANGUAGE_NORWEGIAN
		"Browse"
#endif
#ifdef PROG_LANGUAGE_PORTUGUESE
		"Olhe"
#endif
	    );
	    gtk_widget_show(w);
	}

	return(p); 
}



/*
 *	Deletes all widgets in the prompt structure and deallocates
 *	the prompt structure itself, regardless of the prompt's
 *	type.
 */
static void PDialogPromptDelete(pdialog_prompt_struct *p)
{
	gint i;

	if(p == NULL)
	    return;

	for(i = 0; i < p->total_radios; i++)
	{
	    GTK_WIDGET_DESTROY(p->radio[i])
	    g_free(p->radio_label[i]);
	}
	g_free(p->radio);
	p->radio = NULL;
	g_free(p->radio_label);
	p->radio_label = NULL;
	p->total_radios = 0;
	GTK_WIDGET_DESTROY(p->radio_toplevel)

	GTK_WIDGET_DESTROY(p->pulistbox);

	GTK_WIDGET_DESTROY(p->browse_btn);
	GTK_WIDGET_DESTROY(p->value);
	GTK_WIDGET_DESTROY(p->label);
	GTK_WIDGET_DESTROY(p->icon_pm);
	GTK_WIDGET_DESTROY(p->toplevel);

	g_free(p);
}


/*
 *	Initializes the prompt dialog.
 */
gint PDialogInit(void)
{
	const gint border_major = 5;
	GdkBitmap *mask;
	GdkPixmap *pixmap;
	GdkWindow *window;
	GtkAccelGroup *accelgrp;
	GtkStyle *style;
	GtkWidget	*w,
			*parent, *parent2, *parent3,
			*toplevel;
	pdialog_struct *d = &pdialog;

	/* Already initialized? */
	if(d->initialized)
	    return(-2);

	/* Reset values */
	memset(d, 0x00, sizeof(pdialog_struct));

	d->toplevel = toplevel = gtk_window_new(GTK_WINDOW_DIALOG);
	d->accelgrp = accelgrp = gtk_accel_group_new();
	d->initialized = TRUE;
	d->last_icon_code = PDIALOG_ICON_QUESTION;
	d->last_transient_for = NULL;
	d->block_loop_level = 0;
	d->response_val = NULL;
	d->response_nvals = 0;

	/* Toplevel GtkWindow */
	w = toplevel;
	gtk_window_set_policy(GTK_WINDOW(w), TRUE, TRUE, TRUE);
	gtk_window_set_title(GTK_WINDOW(w), "Enter Values");
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
	    GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
	);
	gtk_signal_connect(
	    GTK_OBJECT(w), "delete_event",
	    GTK_SIGNAL_FUNC(PDialogCloseCB), d
	);
	gtk_container_set_border_width(GTK_CONTAINER(w), 0);
	gtk_window_add_accel_group(GTK_WINDOW(w), accelgrp);
	style = gtk_widget_get_style(w);
	parent = w;


	/* Vbox */
	w = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(parent), w);
	gtk_widget_show(w);
	parent = w;


	/* Hbox for holding the vboxes of icon and prompts */
	w = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(parent), w, TRUE, TRUE, border_major);
	gtk_widget_show(w);
	parent2 = w;

	/* Vbox for icon */
	w = gtk_vbox_new(TRUE, 0);
	gtk_box_pack_start(GTK_BOX(parent2), w, FALSE, FALSE, border_major);
	gtk_widget_show(w);
	parent3 = w;
	/* Icon */
	pixmap = GDK_PIXMAP_NEW_FROM_XPM_DATA(
	    &mask, (guint8 **)icon_question_32x32_xpm
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

	/* Vbox for message label and prompt widgets */
	d->main_vbox = w = gtk_vbox_new(FALSE, border_major);
	gtk_box_pack_start(GTK_BOX(parent2), w, TRUE, TRUE, border_major);
	gtk_widget_show(w);
	parent2 = w;

	/* Message hbox and label */
	d->message_label_hbox = w = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(parent2), w, FALSE, FALSE, 0);
	gtk_widget_show(w);
	parent2 = w;
	d->message_label = w = gtk_label_new("");
	gtk_label_set_justify(GTK_LABEL(w), GTK_JUSTIFY_LEFT);
	gtk_box_pack_start(GTK_BOX(parent2), w, FALSE, FALSE, 0);
	gtk_widget_show(w);


	/* Separator */
	w = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(parent), w, FALSE, FALSE, 0);
	gtk_widget_show(w);


	/* Hbox for buttons */
	w = gtk_hbox_new(TRUE, border_major);
	gtk_box_pack_start(GTK_BOX(parent), w, FALSE, FALSE, border_major);
	gtk_widget_show(w);
	parent2 = w;

	/* Submit button */
	d->submit_btn = w = GUIButtonPixmapLabelH(
	    (guint8 **)icon_ok_20x20_xpm,
#ifdef PROG_LANGUAGE_ENGLISH
	    "Submit",
#endif
#ifdef PROG_LANGUAGE_SPANISH
	    "Sométase",
#endif
#ifdef PROG_LANGUAGE_FRENCH
	    "Soumettre",
#endif
#ifdef PROG_LANGUAGE_GERMAN
	    "Sie Ein",
#endif
#ifdef PROG_LANGUAGE_ITALIAN
	    "Sottomettere",
#endif
#ifdef PROG_LANGUAGE_NORWEGIAN
	    "Forelegg",
#endif
#ifdef PROG_LANGUAGE_PORTUGUESE
	    "Submeta",
#endif
	    &d->submit_btn_label
	);
	GTK_WIDGET_SET_FLAGS(w, GTK_CAN_DEFAULT);
	gtk_widget_set_usize(
	    w,
	    GUI_BUTTON_HLABEL_WIDTH_DEF, GUI_BUTTON_HLABEL_HEIGHT_DEF
	);
	gtk_box_pack_start(GTK_BOX(parent2), w, TRUE, FALSE, 0);
	gtk_signal_connect(
	    GTK_OBJECT(w), "clicked",
	    GTK_SIGNAL_FUNC(PDialogButtonCB), d
	);
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

	/* Cancel button */
	d->cancel_btn = w = GUIButtonPixmapLabelH(
	    (guint8 **)icon_cancel_20x20_xpm,
#ifdef PROG_LANGUAGE_ENGLISH
	    "Cancel",
#endif
#ifdef PROG_LANGUAGE_SPANISH
	    "Cancele",
#endif
#ifdef PROG_LANGUAGE_FRENCH
	    "Annuler",
#endif
#ifdef PROG_LANGUAGE_GERMAN
	    "Sie Auf",
#endif
#ifdef PROG_LANGUAGE_ITALIAN
	    "Annullare",
#endif
#ifdef PROG_LANGUAGE_NORWEGIAN
	    "Kanseller",
#endif
#ifdef PROG_LANGUAGE_PORTUGUESE
	    "Cancelamento",
#endif
	    &d->cancel_btn_label
	);
	GTK_WIDGET_SET_FLAGS(w, GTK_CAN_DEFAULT);
	gtk_widget_set_usize(
	    w,
	    GUI_BUTTON_HLABEL_WIDTH_DEF, GUI_BUTTON_HLABEL_HEIGHT_DEF
	);
	gtk_box_pack_start(GTK_BOX(parent2), w, TRUE, FALSE, 0);
	gtk_signal_connect(
	    GTK_OBJECT(w), "clicked",
	    GTK_SIGNAL_FUNC(PDialogButtonCB), d
	);
	gtk_accel_group_add(
	    accelgrp, GDK_Escape, 0, GTK_ACCEL_VISIBLE,
	    GTK_OBJECT(w), "clicked"
	);

	/* Help button */
	d->help_btn = w = GUIButtonPixmapLabelH(
	    (guint8 **)icon_help_20x20_xpm,
#ifdef PROG_LANGUAGE_ENGLISH
	    "Help",
#endif
#ifdef PROG_LANGUAGE_SPANISH
	    "Ayuda",
#endif
#ifdef PROG_LANGUAGE_FRENCH
	    "Aide",
#endif
#ifdef PROG_LANGUAGE_GERMAN
	    "Hilfe",
#endif
#ifdef PROG_LANGUAGE_ITALIAN
	    "L'Aiuto",
#endif
#ifdef PROG_LANGUAGE_NORWEGIAN
	    "Hjelp",
#endif
#ifdef PROG_LANGUAGE_PORTUGUESE
	    "Ajuda",
#endif
	    NULL
	);
	GTK_WIDGET_SET_FLAGS(w, GTK_CAN_DEFAULT);
	gtk_widget_set_usize(
	    w,
	    GUI_BUTTON_HLABEL_WIDTH_DEF, GUI_BUTTON_HLABEL_HEIGHT_DEF
	);
	gtk_box_pack_start(GTK_BOX(parent2), w, TRUE, FALSE, 0);
	gtk_signal_connect(
	    GTK_OBJECT(w), "clicked",
	    GTK_SIGNAL_FUNC(PDialogButtonCB), d
	);


	return(0);
}

/*
 *	Sets the Dialog's style.
 */
void PDialogSetStyle(GtkRcStyle *rc_style)
{
	GtkWidget *w;
	pdialog_struct *d = &pdialog;

	if(!d->initialized)
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
void PDialogSetTransientFor(GtkWidget *w)
{
	pdialog_struct *d = &pdialog;

	if(!d->initialized)
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
 *	Checks if the Dialog is currently waiting for user input.
 */
gboolean PDialogIsQuery(void)
{
	pdialog_struct *d = &pdialog;
	return((d->block_loop_level > 0) ? TRUE : FALSE);
}

/*
 *	Breaks the Dialog's user input query (if any) and causes the
 *	querying function to return CDIALOG_RESPONSE_NOT_AVAILABLE.
 */
void PDialogBreakQuery(void)
{
	pdialog_struct *d = &pdialog;

	/* Break out of all our main loops */
	while(d->block_loop_level > 0)
	{
	    gtk_main_quit();
	    d->block_loop_level--;
	}
	d->block_loop_level = 0;
}

/*
 *	Returns the Dialog's toplevel widget.
 */
GtkWidget *PDialogGetToplevel(void)
{
	pdialog_struct *d = &pdialog;
	return(d->toplevel);
}


/*
 *	Nexus for adding a prompt to the dialog.
 */
static pdialog_prompt_struct *PDialogAddPromptNexus(
	const pdialog_prompt_type type,
	const guint8 **icon_data,
	const gchar *label,
	const gchar *value,
	const gboolean hide_value,		/* For passwords */
	gchar *(*browse_cb)(gpointer, gpointer, gint),
	gpointer browse_data
)
{
	gint n;
	GtkWidget *w, *parent;
	pdialog_struct *d = &pdialog;
	pdialog_prompt_struct *p = PDialogPromptNewNexus(
	    type,
	    icon_data, label, value, hide_value,
	    d,
	    browse_cb, browse_data
	);
	if(p == NULL)
	    return(NULL);

	if(d->total_prompts < 0)
	    d->total_prompts = 0;

	for(n = 0; n < d->total_prompts; n++)
	{
	    if(d->prompt[n] == NULL) 
		break; 
	}
	if(n < d->total_prompts)
	{
	    d->prompt[n] = p;
	} 
	else
	{
	    n = d->total_prompts;
	    d->total_prompts++;
	    d->prompt = (pdialog_prompt_struct **)g_realloc(
		d->prompt,
		d->total_prompts * sizeof(pdialog_prompt_struct *)
	    );
	    if(d->prompt == NULL)
	    {
		PDialogPromptDelete(p);
		d->total_prompts = 0;
		return(NULL);
	    }

	    d->prompt[n] = p;
	}

	parent = d->main_vbox;
	w = p->toplevel;
	if((parent != NULL) && (w != NULL))
	{
	    gtk_box_pack_start(GTK_BOX(parent), w, TRUE, FALSE, 0);
	    gtk_widget_show(w);
	}

	return(p);
}

/*
 *	Adds a label to the dialog.
 */
void PDialogAddPromptLabel(const gchar *label)
{
	PDialogAddPromptNexus(
	    PDIALOG_PROMPT_TYPE_LABEL,
	    NULL, NULL, label, FALSE, NULL, NULL
	);
}

/*
 *	Adds a prompt to the dialog.
 */
void PDialogAddPrompt(
	const guint8 **icon_data,
	const gchar *label,
	const gchar *value
)
{
	PDialogAddPromptNexus(
	    PDIALOG_PROMPT_TYPE_ENTRY,
	    icon_data, label, value, FALSE, NULL, NULL
	);
}

/*
 *	Adds a prompt (with the value hidden for passwords) to the
 *	dialog.
 */
void PDialogAddPromptPassword(
	const guint8 **icon_data,
	const gchar *label,
	const gchar *value
)
{
	PDialogAddPromptNexus(
	    PDIALOG_PROMPT_TYPE_ENTRY,
	    icon_data, label, value, TRUE, NULL, NULL
	);
}

/*
 *	Adds a prompt with a browse button to the dialog.
 */
void PDialogAddPromptWithBrowse(
	const guint8 **icon_data,
	const gchar *label,
	const gchar *value,
	gpointer browse_data,
	gchar *(*browse_cb)(gpointer, gpointer, gint)
)
{
	PDialogAddPromptNexus(
	    PDIALOG_PROMPT_TYPE_ENTRY,
	    icon_data, label, value, FALSE, browse_cb, browse_data
	);
}

/*
 *	Adds a spin prompt to the dialog.
 */
void PDialogAddPromptSpin(
	const guint8 **icon_data,
	const gchar *label,
	gfloat value, gfloat lower, gfloat upper,
	gfloat step_increment, gfloat page_increment,
	gfloat climb_rate, guint digits
)
{
	pdialog_prompt_struct *p = PDialogAddPromptNexus(
	    PDIALOG_PROMPT_TYPE_SPIN,
	    icon_data, label, NULL, FALSE, NULL, NULL
	);
	if(p != NULL)
	{
	    GtkAdjustment *adj = (GtkAdjustment *)gtk_adjustment_new(
		value, lower, upper,
		step_increment, page_increment, 0.0f
	    );
	    GtkSpinButton *spin = GTK_SPIN_BUTTON(p->value);
	    if((spin != NULL) && (adj != NULL))
		gtk_spin_button_configure(spin, adj, climb_rate, digits);
	}
}

/*
 *	Adds a scale prompt to the dialog.
 */
void PDialogAddPromptScale(
	const guint8 **icon_data,
	const gchar *label,
	gfloat value, gfloat lower, gfloat upper,
	gfloat step_increment, gfloat page_increment,
	gboolean show_value, guint digits
)
{
	pdialog_prompt_struct *p = PDialogAddPromptNexus(
	    PDIALOG_PROMPT_TYPE_SCALE,
	    icon_data, label, NULL, FALSE, NULL, NULL
	);
	if(p != NULL)
	{
	    GtkScale *scale = GTK_SCALE(p->value);
	    GtkRange *range = GTK_RANGE(scale);
	    GtkAdjustment *adj = (range != NULL) ? range->adjustment : NULL;
	    if(adj != NULL)
	    {
		adj->value = value;
		adj->lower = lower;
		adj->upper = upper;
		adj->step_increment = step_increment;
		adj->page_increment = page_increment;
	    }
	    if(scale != NULL)
	    {
		gtk_scale_set_value_pos(scale, GTK_POS_RIGHT);
		gtk_scale_set_draw_value(scale, show_value);
		gtk_scale_set_digits(scale, (gint)digits);
	    }
	}
}

/*
 *	Adds a popup list prompt to the dialog.
 *
 *	The list specifies the list of values, each value is a
 *	gchar * to a string. The list will not be modified by this
 *	function. If list is NULL then no list of values will be set.
 *
 *	The start_num specifies the initial value or -1 for none.
 *
 *	The nitems_visible specifies the number of items visible or
 *	-1 for default.
 */
void PDialogAddPromptPopupList(
	const guint8 **icon_data,
	const gchar *label,
	GList *list,			/* List of values */
	const gint start_num,		/* Initial value */
	const gint nitems_visible
)
{
	pdialog_prompt_struct *p = PDialogAddPromptNexus(
	    PDIALOG_PROMPT_TYPE_POPUP_LIST,
	    icon_data, label, NULL, FALSE, NULL, NULL
	);
	if(p != NULL)
	{
	    GtkWidget *pulistbox = p->pulistbox;
	    if(pulistbox != NULL)
	    {
		GtkWidget *pulist = PUListBoxGetPUList(pulistbox);
		GList *glist = list;
		while(glist != NULL)
		{
		    PUListAddItem(pulist, (gchar *)glist->data);
		    glist = g_list_next(glist);
		}
		if(start_num > -1)
		    PUListBoxSelect(pulistbox, start_num);

		if(nitems_visible > -1)
		{
		    PUListBoxSetLinesVisible(
			pulistbox, nitems_visible
		    );
		}
		else
		{
		    PUListBoxSetLinesVisible(
			pulistbox,
			MIN(
			    PUListGetTotalItems(pulist),
			    10
			)
		    );
		}
	    }
	}
}

/*
 *	Adds a combo prompt to the dialog.
 *
 *	The value specifies the initial value. If value is NULL then
 *	no initial value will be set.
 *
 *	The list specifies the list of values, each value is a
 *	gchar * to a string. The list will not be modified by this
 *	function. If list is NULL then no list of values will be set.
 *
 *	If editable is TRUE then the entry will be set as editable.
 *
 *	If case_sensitive is TRUE then only case sensitive matches
 *	will be made with the list values and the entry value.
 */
void PDialogAddPromptCombo(
	const guint8 **icon_data,
	const gchar *label,
	const gchar *value,
	GList *list,
	const gboolean editable,
	const gboolean case_sensitive
)
{
	pdialog_prompt_struct *p = PDialogAddPromptNexus(
	    PDIALOG_PROMPT_TYPE_COMBO,
	    icon_data, label, value, FALSE, NULL, NULL
	);
	if(p != NULL)
	{
	    GtkCombo *combo = GTK_COMBO(p->value);
	    GtkEntry *entry = (combo != NULL) ? GTK_ENTRY(combo->entry) : NULL;
	    gtk_entry_set_editable(entry, editable);
	    gtk_combo_set_case_sensitive(combo, case_sensitive);
	    gtk_combo_set_use_arrows_always(combo, TRUE);
	    if(list != NULL)
		gtk_combo_set_popdown_strings(combo, list);
	    if(value != NULL)
		gtk_entry_set_text(entry, value);
	}
}

/*
 *	Adds a radio prompt to the dialog.
 *
 *	The list specifies the list of values, each value is a
 *	gchar * to a string. The list will not be modified by this
 *	function. If list is NULL then no list of values will be set.
 *
 *	The start_num specifies the initial value or -1 for none.
 */
void PDialogAddPromptRadio(
	const guint8 **icon_data,
	const gchar *label,
	GList *list,
	const gint start_num
)
{
	pdialog_prompt_struct *p = PDialogAddPromptNexus(
	    PDIALOG_PROMPT_TYPE_RADIO,
	    icon_data, label, NULL, FALSE, NULL, NULL
	);
	if(p != NULL)
	{
	    gint i = 0;
	    GSList *gslist = NULL;
	    GList *glist = list;
	    GtkWidget *w;

	    /* Iterate through given list of values, creating a radio
	     * button widget and recording the value as a string for
	     * each value
	     */
	    while(glist != NULL)
	    {
		p->total_radios = i + 1;	/* Increase total */

		/* Create a new radio button */
		p->radio = (GtkWidget **)g_realloc(
		    p->radio, p->total_radios * sizeof(GtkWidget *)
		);
		p->radio[i] = w = gtk_radio_button_new_with_label(
		    gslist, (const gchar *)glist->data
		);
		gtk_box_pack_start(GTK_BOX(p->radio_toplevel), w, FALSE, FALSE, 0);
		GTK_TOGGLE_BUTTON(w)->active = (i == start_num) ? TRUE : FALSE;
		gtk_widget_show(w);
		if(g_list_next(glist) != NULL)
		    gslist = gtk_radio_button_group(GTK_RADIO_BUTTON(w));

		/* Record value as a the radio button label string */
		p->radio_label = (gchar **)g_realloc(
		    p->radio_label,
		    p->total_radios * sizeof(gchar *)
		);
		p->radio_label[i] = STRDUP((glist->data != NULL) ?
		    (const gchar *)glist->data : ""
		);

		/* Go on to next value */
		glist = g_list_next(glist);
		i++;
	    }
	}
}

/*
 *	Adds a toggle prompt to the dialog.
 */
void PDialogAddPromptToggle(
	const guint8 **icon_data,
	const gchar *label, gboolean value
)
{
	pdialog_prompt_struct *p = PDialogAddPromptNexus(
	    PDIALOG_PROMPT_TYPE_TOGGLE,
	    icon_data, label, "", FALSE, NULL, NULL
	);
	if(p != NULL)
	    gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(p->value), value ? TRUE : FALSE
	    );
}

/*
 *	Adds a separator to the dialog.
 */
void PDialogAddPromptSeparator(void)
{
	PDialogAddPromptNexus(
	    PDIALOG_PROMPT_TYPE_SEPARATOR,
	    NULL, NULL, NULL, FALSE, NULL, NULL
	);
}


/*
 *	Changes the value in the prompt entry or spin widget, changes
 *	the prompt's icon, and changes the label.
 *
 *	If any input is NULL then that value will be left unchanged.
 *
 *	If prompt_num is -1 then the last prompt will be used.
 */
void PDialogSetPromptValue(
	gint prompt_num,
	const guint8 **icon_data,
	const gchar *label,
	const gchar *value 
)
{
	GtkWidget *w;
	pdialog_prompt_struct *p;
	pdialog_struct *d = &pdialog;

	/* Last prompt? */
	if(prompt_num < 0)
	    prompt_num = d->total_prompts - 1;

	/* Prompt valid? */
	p = ((prompt_num >= 0) && (prompt_num < d->total_prompts)) ?
	    d->prompt[prompt_num] : NULL;
	if(p == NULL)
	    return;

	/* Change icon? */
	w = p->icon_pm;
	if((icon_data != NULL) && (w != NULL))
	{
	    GdkBitmap *mask;
	    GdkPixmap *pixmap = GDK_PIXMAP_NEW_FROM_XPM_DATA(
		&mask, (guint8 **)icon_data
	    );
	    if(pixmap != NULL)
	    {
		gint width, height;
		gdk_window_get_size(pixmap, &width, &height);
		gtk_pixmap_set(GTK_PIXMAP(w), pixmap, mask);
		gtk_widget_set_usize(w, width, height);

		GDK_PIXMAP_UNREF(pixmap);
		GDK_BITMAP_UNREF(mask);
	    }
	}

	/* Change label? */
	w = p->label;
	if((label != NULL) && (w != NULL))
	{
	    gtk_label_set_text(GTK_LABEL(w), label);
	    gtk_label_set_justify(GTK_LABEL(w), GTK_JUSTIFY_RIGHT);
	}

	/* Change value? */
	w = p->value;
	if((value != NULL) && (w != NULL))
	{
	    switch(p->type)
	    {
	      case PDIALOG_PROMPT_TYPE_LABEL:
		if(GTK_IS_LABEL(w))
		{
		    GtkLabel *label = GTK_LABEL(w);
		    gtk_label_set_text(label, value);
		}
		break;

	      case PDIALOG_PROMPT_TYPE_ENTRY:
		if(GTK_IS_ENTRY(w))
		{
		    GtkEntry *entry = GTK_ENTRY(w);
		    gtk_entry_set_text(entry, value);
		    gtk_entry_set_position(entry, -1);
		}
		break;

	      case PDIALOG_PROMPT_TYPE_SPIN:
		if(GTK_IS_SPIN_BUTTON(w))
		{
		    GtkEntry *entry = GTK_ENTRY(w);
		    gtk_entry_set_text(entry, value);
		    gtk_entry_set_position(entry, -1);
		}
		break;

	      case PDIALOG_PROMPT_TYPE_SCALE:
		if(GTK_IS_SCALE(w))
		{
		    GTK_ADJUSTMENT_SET_VALUE(
			GTK_RANGE(w)->adjustment,
			(gfloat)ATOF(value)
		    );
		}
		break;

	      case PDIALOG_PROMPT_TYPE_POPUP_LIST:
		if(p->pulistbox != NULL)
		{
		    gint i;
		    const gchar *v;
		    GtkWidget *pulistbox = p->pulistbox;
		    GtkWidget *pulist = PUListBoxGetPUList(pulistbox);
		    const gint m = PUListGetTotalItems(pulist);

		    for(i = 0; i < m; i++)
		    {
			PUListGetItemText(pulist, i, &v);
			if(v == NULL)
			    continue;

			if(!g_strcasecmp(v, value))
			{
			    PUListBoxSelect(pulistbox, i);
			    break;
			}
		    }
		}
		break;

	      case PDIALOG_PROMPT_TYPE_COMBO:
		if(GTK_IS_COMBO(w))
		{
		    GtkCombo *combo = GTK_COMBO(w);
		    GtkEntry *entry = GTK_ENTRY(combo->entry);
		    gtk_entry_set_text(entry, value);
		    gtk_entry_set_position(entry, -1);
		}
		break;

	      case PDIALOG_PROMPT_TYPE_RADIO:
		if(p->total_radios > 0)
		{
		    gint i;
		    const gchar *s;

		    for(i = 0; i < p->total_radios; i++)
		    {
			w = p->radio[i];
			s = p->radio_label[i];
			if((w == NULL) || (s == NULL))
			    continue;

			GTK_TOGGLE_BUTTON(w)->active =
			    (!strcmp(s, value)) ? TRUE : FALSE;
		    }
		}
		break;

	      case PDIALOG_PROMPT_TYPE_TOGGLE:
		if(GTK_IS_TOGGLE_BUTTON(w))
		{
		    GtkToggleButton *tb = GTK_TOGGLE_BUTTON(w);
		    if(!strcmp(value, "0") || (*value == '\0'))
			gtk_toggle_button_set_active(tb, FALSE);
		    else
			gtk_toggle_button_set_active(tb, TRUE);
		}
		break;

	      case PDIALOG_PROMPT_TYPE_SEPARATOR:
		break;
	    }
	}
}

/*
 *	Sets the tip for the given prompt.
 *
 *	If any input is NULL then that value will be left unchanged.
 *
 *	If prompt_num is -1 then the last prompt will be used.
 */
void PDialogSetPromptTip(gint prompt_num, const gchar *tip)
{
	GtkWidget *w;
	pdialog_prompt_struct *p;
	pdialog_struct *d = &pdialog;

	if(prompt_num < 0)
	    prompt_num = d->total_prompts - 1;

	p = ((prompt_num >= 0) && (prompt_num < d->total_prompts)) ?
	    d->prompt[prompt_num] : NULL;

	if(p != NULL)
	{
	    switch(p->type)
	    {
	      case PDIALOG_PROMPT_TYPE_LABEL:
		break;

	      case PDIALOG_PROMPT_TYPE_ENTRY:
		w = p->value;
		GUISetWidgetTip(w, tip);
		break;

	      case PDIALOG_PROMPT_TYPE_SPIN:
		w = p->value;
		GUISetWidgetTip(w, tip);
		break;

	      case PDIALOG_PROMPT_TYPE_SCALE:
		w = p->value;
		GUISetWidgetTip(w, tip);
		break;

	      case PDIALOG_PROMPT_TYPE_POPUP_LIST:
		PUListBoxSetTip(p->pulistbox, tip);
		break;

	      case PDIALOG_PROMPT_TYPE_COMBO:
		w = p->value;
		if(w != NULL)
		{
		    GtkCombo *combo = GTK_COMBO(w);
		    GUISetWidgetTip(combo->entry, tip);
		}
		break;

	      case PDIALOG_PROMPT_TYPE_RADIO:
/* Not sure how to set the tip for (each) radio?
		w = p->radio_toplevel;
		GUISetWidgetTip(w, tip);
 */
		break;

	      case PDIALOG_PROMPT_TYPE_TOGGLE:
		w = p->value;
		GUISetWidgetTip(w, tip);
		break;

	      case PDIALOG_PROMPT_TYPE_SEPARATOR:
		break;
	    }
	}
}

/*
 *	Gets the value of the specified prompt as a string.
 *
 *	Can return NULL on error or if the prompt does not have a value
 *	to return.
 *
 *	If prompt_num is -1 then the last prompt will be used.
 */
const gchar *PDialogGetPromptValue(gint prompt_num)
{
	pdialog_prompt_struct *p;
	pdialog_struct *d = &pdialog;
	static gchar num_str[80];

	*num_str = '\0';

	if(prompt_num < 0)
	    prompt_num = d->total_prompts - 1;

	p = ((prompt_num >= 0) && (prompt_num < d->total_prompts)) ?
	    d->prompt[prompt_num] : NULL;

	if((p != NULL) ? (p->value != NULL) : FALSE)
	{
	    GtkWidget *w = p->value;

	    switch(p->type)
	    {
	      case PDIALOG_PROMPT_TYPE_LABEL:
		break;

	      case PDIALOG_PROMPT_TYPE_ENTRY:
		if(GTK_IS_ENTRY(w))
		    return(gtk_entry_get_text(GTK_ENTRY(w)));
		break;

	      case PDIALOG_PROMPT_TYPE_SPIN:
		if(GTK_IS_SPIN_BUTTON(w))
		    return(gtk_entry_get_text(GTK_ENTRY(w)));
		break;

	      case PDIALOG_PROMPT_TYPE_SCALE:
		if(GTK_IS_SCALE(w))
		{
		    GtkRange *range = GTK_RANGE(w);
		    GtkAdjustment *adj = range->adjustment;
		    if(adj != NULL)
		    {
			gchar fmt_str[80];
			g_snprintf(
			    fmt_str, sizeof(fmt_str),
			    "%%.%if",
			    range->digits
			);
			g_snprintf(
			    num_str, sizeof(num_str),
			    fmt_str,
			    adj->value
			);
			return(num_str);
		    }
		}
		break;

	      case PDIALOG_PROMPT_TYPE_POPUP_LIST:
		if(p->pulistbox != NULL)
		{
		    const gchar *v;
		    GtkWidget *pulist = PUListBoxGetPUList(p->pulistbox);
		    PUListGetItemText(
			pulist,
			PUListGetSelectedLast(pulist),
			&v
		    );
		    return(v);
		}
		break;

	      case PDIALOG_PROMPT_TYPE_COMBO:
		if(GTK_IS_COMBO(w))
		{
		    GtkCombo *combo = GTK_COMBO(w);
		    w = combo->entry;
		    return(gtk_entry_get_text(GTK_ENTRY(w)));
		}
		break;

	      case PDIALOG_PROMPT_TYPE_RADIO:
		if(p->total_radios > 0)
		{
		    gint i;
		    for(i = 0; i < p->total_radios; i++)
		    {
			if(GTK_TOGGLE_BUTTON_GET_ACTIVE(p->radio[i]))
			    return(p->radio_label[i]);
		    }
		}
		break;

	      case PDIALOG_PROMPT_TYPE_TOGGLE:
		if(GTK_IS_TOGGLE_BUTTON(w))
		{
		    GtkToggleButton *tb = GTK_TOGGLE_BUTTON(w);
		    return((tb->active) ? "1" : "0");
		}
		break;

	      case PDIALOG_PROMPT_TYPE_SEPARATOR:
		break;
	    }
	}

	return(NULL);
}

/*
 *	Sets the prompt to accept the tab key to complete paths.
 *
 *	If prompt_num is -1 then the last prompt will be used.
 */
void PDialogSetPromptCompletePath(gint prompt_num)
{
	pdialog_prompt_struct *p;
	pdialog_struct *d = &pdialog;

	if(prompt_num < 0)
	    prompt_num = d->total_prompts - 1;

	p = ((prompt_num >= 0) && (prompt_num < d->total_prompts)) ?
	    d->prompt[prompt_num] : NULL;

	if(p != NULL)
	{
	    GtkWidget *w = p->value;

	    switch(p->type)
	    {
	      case PDIALOG_PROMPT_TYPE_LABEL:
		break;

	      case PDIALOG_PROMPT_TYPE_ENTRY:
		if(GTK_IS_ENTRY(w))
		{
		    gtk_signal_connect(
			GTK_OBJECT(w), "key_press_event",
			GTK_SIGNAL_FUNC(PDialogEntryCompletePathKeyCB), d
		    );
		    gtk_signal_connect(
			GTK_OBJECT(w), "key_release_event",
			GTK_SIGNAL_FUNC(PDialogEntryCompletePathKeyCB), d
		    );
		}
		break;

	      case PDIALOG_PROMPT_TYPE_SPIN:
		break;

	      case PDIALOG_PROMPT_TYPE_SCALE:
		break;

	      case PDIALOG_PROMPT_TYPE_POPUP_LIST:
		break;

	      case PDIALOG_PROMPT_TYPE_COMBO:
		if(GTK_IS_COMBO(w))
		{
		    GtkCombo *combo = GTK_COMBO(w);
		    w = combo->entry;
		    if(GTK_IS_ENTRY(w))
		    {
			gtk_signal_connect(
			    GTK_OBJECT(w), "key_press_event",
			    GTK_SIGNAL_FUNC(PDialogEntryCompletePathKeyCB), d
			);
			gtk_signal_connect(
			    GTK_OBJECT(w), "key_release_event",
			    GTK_SIGNAL_FUNC(PDialogEntryCompletePathKeyCB), d
			);
		    }
		}
		break;

	      case PDIALOG_PROMPT_TYPE_RADIO:
		break;

	      case PDIALOG_PROMPT_TYPE_TOGGLE:
		break;

	      case PDIALOG_PROMPT_TYPE_SEPARATOR:
		break;
	    }
	}
}

/*
 *	Destroys all prompts on prompt dialog.
 */
void PDialogDeleteAllPrompts(void)
{
	gint i;
	pdialog_struct *d = &pdialog;

	for(i = 0; i < d->total_prompts; i++)
	    PDialogPromptDelete(d->prompt[i]);
	g_free(d->prompt);
	d->prompt = NULL;
	d->total_prompts = 0;
}


/*
 *	Block input and wait for response.
 *
 *	Returns an array of string values from the given prompts
 *	which must not be modified or deleted.
 *
 *	If NULL is returned then it means the user canceled.
 *
 *	If any values are set NULL then that value will not be modified
 *	from since the last usage.
 */
gchar **PDialogGetResponse(
	const gchar *title,
	const gchar *message,
	const gchar *explaination,
	pdialog_icon icon_code,
	const gchar *submit_label,
	const gchar *cancel_label,
	pdialog_btn_flags show_buttons,		/* Any of PDIALOG_BTNFLAG_* */
	pdialog_btn_flags default_button,	/* One of PDIALOG_BTNFLAG_* */
	gint *nvalues
)
{
	gboolean set_focus = FALSE;
	gint i;
	GtkWidget *w;
	pdialog_struct *d = &pdialog;

	/* Reset returns */
	if(nvalues != NULL)
	    *nvalues = 0;

	/* Already waiting for response? */
	if(d->block_loop_level > 0)
	    return(NULL);

	/* Reset responses */
	for(i = 0; i < d->response_nvals; i++)
	    g_free(d->response_val[i]);
	g_free(d->response_val);
	d->response_val = NULL;
	d->response_nvals = 0;

	/* Set title */
	if(title != NULL)
	{
	    w = d->toplevel;
	    if(w != NULL)
		gtk_window_set_title(GTK_WINDOW(w), title);
	}

	/* Set message label text */
	w = d->message_label;
	if(w != NULL)
	    gtk_label_set_text(
		GTK_LABEL(w),
		(message != NULL) ? message : ""
	    );
	w = d->message_label_hbox;
	if(w != NULL)
	{
	    if(message != NULL)
		gtk_widget_show(w);
	    else
		gtk_widget_hide(w);
	}

	/* Change icon? */
	if(icon_code != d->last_icon_code)
	{
	    guint8 **icon_data = PDialogGetMessageIconDataFromCode(icon_code);
	    d->last_icon_code = icon_code;
	    PDialogSetIcon(d, icon_data);
	}

	/* Need to change button labels? */
	if(submit_label != NULL)
	{
	    w = d->submit_btn_label;
	    if(w != NULL)
		gtk_label_set_text(GTK_LABEL(w), submit_label);
	}
	if(cancel_label != NULL)
	{
	    w = d->cancel_btn_label;
	    if(w != NULL)
		gtk_label_set_text(GTK_LABEL(w), cancel_label);
	}


	/* Show/hide buttons */
#define DO_MAP_BUTTON		\
{ if(w != NULL) gtk_widget_show(w); }
#define DO_UNMAP_BUTTON		\
{ if(w != NULL) gtk_widget_hide(w); }
#define DO_DEFAULT_BUTTON	\
{ if(w != NULL) {		\
/*gtk_widget_grab_focus(w);	\
  gtk_widget_grab_default(w); */\
} }
#define DO_UNDEFAULT_BUTTON	\
{ if(w != NULL) {		\
/*GTK_WIDGET_UNSET_FLAGS(w, GTK_HAS_DEFAULT); \
  GTK_WIDGET_UNSET_FLAGS(w, GTK_RECEIVES_DEFAULT); */ \
} }

	w = d->submit_btn;
	if(show_buttons & PDIALOG_BTNFLAG_SUBMIT)
	    DO_MAP_BUTTON
	else
	    DO_UNMAP_BUTTON
	if(default_button & PDIALOG_BTNFLAG_SUBMIT)
	    DO_DEFAULT_BUTTON
	else
	    DO_UNDEFAULT_BUTTON

	w = d->cancel_btn;
	if(show_buttons & PDIALOG_BTNFLAG_CANCEL)
	    DO_MAP_BUTTON
	else
	    DO_UNMAP_BUTTON
	if(default_button & PDIALOG_BTNFLAG_CANCEL)
	    DO_DEFAULT_BUTTON
	else
	    DO_UNDEFAULT_BUTTON

	w = d->help_btn;
	if(show_buttons & PDIALOG_BTNFLAG_HELP)
	    DO_MAP_BUTTON
	else
	    DO_UNMAP_BUTTON
	if(default_button & PDIALOG_BTNFLAG_HELP)
	    DO_DEFAULT_BUTTON
	else
	    DO_UNDEFAULT_BUTTON

	/* The first prompt should be in focus */
	if(d->total_prompts > 0)
	{
	    pdialog_prompt_struct *p = d->prompt[0];
	    w = (p != NULL) ? p->value : NULL;
	    if((w != NULL) ? GTK_WIDGET_CAN_FOCUS(w) : FALSE)
	    {
		gtk_widget_grab_focus(w);
		set_focus = TRUE;
	    }
	}

	if(!set_focus)
	{
	    if(default_button & PDIALOG_BTNFLAG_SUBMIT)
	    {
		w = d->submit_btn;
		if(w != NULL)
		{
		    gtk_widget_grab_focus(w);
		    set_focus = TRUE;
		}
	    }
	    else if(default_button & PDIALOG_BTNFLAG_CANCEL)
	    {
		w = d->cancel_btn;
		if(w != NULL)
		{
		    gtk_widget_grab_focus(w);
		    set_focus = TRUE;
		}
	    }
	}

#undef DO_MAP_BUTTON
#undef DO_UNMAP_BUTTON
#undef DO_DEFAULT_BUTTON
#undef DO_UNDEFAULT_BUTTON

	/* Widget sizes may have changed due to new values est, queue
	 * resize for the toplevel
	 */
	w = d->toplevel;
	if(w != NULL)
	    gtk_widget_queue_resize(w);

	/* Map dialog */
	PDialogMap();

	/* Block until user response */
	d->block_loop_level++;
	gtk_main();

	/* Unmap dialog */
	PDialogUnmap();

	/* Break out of any subsequent main loops */
	while(d->block_loop_level > 0)
	{
	    gtk_main_quit();
	    d->block_loop_level--;
	}
	d->block_loop_level = 0;

	/* Update returns */
	if(nvalues != NULL)
	    *nvalues = d->response_nvals;

	return(d->response_val);
}


/*
 *	Same as PDialogGetResponse() except that the icon is set by  
 *	the specified icon data.
 */
gchar **PDialogGetResponseIconData(
	const gchar *title,
	const gchar *message,
	const gchar *explaination,
	guint8 **icon_data,
	const gchar *submit_label,
	const gchar *cancel_label,
	pdialog_btn_flags show_buttons,		/* Any of PDIALOG_BTNFLAG_* */
	pdialog_btn_flags default_button,	/* One of PDIALOG_BTNFLAG_* */
	gint *nvalues
)
{
	gint i;
	GtkWidget *w;
	pdialog_struct *d = &pdialog;

	/* Reset returns */
	if(nvalues != NULL)
	    *nvalues = 0;

	/* Already waiting for response? */
	if(d->block_loop_level > 0)
	    return(NULL);
	 
	/* Reset responses */
	for(i = 0; i < d->response_nvals; i++)
	    g_free(d->response_val[i]);
	g_free(d->response_val);
	d->response_val = NULL;
	d->response_nvals = 0;

	/* Set title */
	if(title != NULL)
	{
	    w = d->toplevel;
	    if(w != NULL)
		gtk_window_set_title(GTK_WINDOW(w), title);
	}

	/* Set message label text */
	w = d->message_label;
	if(w != NULL)
	    gtk_label_set_text(
		GTK_LABEL(w),
		(message != NULL) ? message : ""
	    );
	w = d->message_label_hbox;
	if(w != NULL)
	{
	    if(message != NULL)
		gtk_widget_show(w);
	    else
		gtk_widget_hide(w);
	}
	 
	/* Change icon? */
	if(icon_data != NULL)
	{
	    d->last_icon_code = PDIALOG_ICON_USER_DEFINED;
	    PDialogSetIcon(d, icon_data);
	}

	/* Need to change button labels? */
	if(submit_label != NULL)
	{
	    w = d->submit_btn_label;
	    if(w != NULL)
		gtk_label_set_text(GTK_LABEL(w), submit_label);
	}
	if(cancel_label != NULL)
	{
	    w = d->cancel_btn_label;
	    if(w != NULL)
		gtk_label_set_text(GTK_LABEL(w), cancel_label);
	}


	/* Show/hide buttons */
#define DO_MAP_BUTTON           \
{ if(w != NULL) gtk_widget_show(w); }
#define DO_UNMAP_BUTTON         \
{ if(w != NULL) gtk_widget_hide(w); }
#define DO_DEFAULT_BUTTON       \
{ if(w != NULL) {               \
/*gtk_widget_grab_focus(w);	\
  gtk_widget_grab_default(w); */\
} }
#define DO_UNDEFAULT_BUTTON     \
{ if(w != NULL) {               \
/*GTK_WIDGET_UNSET_FLAGS(w, GTK_HAS_DEFAULT); \
  GTK_WIDGET_UNSET_FLAGS(w, GTK_RECEIVES_DEFAULT); */ \
} }

	w = d->submit_btn;
	if(show_buttons & PDIALOG_BTNFLAG_SUBMIT)
	    DO_MAP_BUTTON
	else
	    DO_UNMAP_BUTTON
	if(default_button & PDIALOG_BTNFLAG_SUBMIT)
	    DO_DEFAULT_BUTTON
	else
	    DO_UNDEFAULT_BUTTON

	w = d->cancel_btn;
	if(show_buttons & PDIALOG_BTNFLAG_CANCEL)
	    DO_MAP_BUTTON
	else
	    DO_UNMAP_BUTTON
	if(default_button & PDIALOG_BTNFLAG_CANCEL)
	    DO_DEFAULT_BUTTON
	else
	    DO_UNDEFAULT_BUTTON

	w = d->help_btn;
	if(show_buttons & PDIALOG_BTNFLAG_HELP)
	    DO_MAP_BUTTON
	else
	    DO_UNMAP_BUTTON
	if(default_button & PDIALOG_BTNFLAG_HELP)
	    DO_DEFAULT_BUTTON
	else
	    DO_UNDEFAULT_BUTTON

	/* Have first prompt to grab focus */
	if(d->total_prompts > 0)
	{
	    pdialog_prompt_struct *p = d->prompt[0];
	    w = (p != NULL) ? p->value : NULL;
	    if(w != NULL)
		gtk_widget_grab_focus(w);
	}    

#undef DO_MAP_BUTTON
#undef DO_UNMAP_BUTTON
#undef DO_DEFAULT_BUTTON
#undef DO_UNDEFAULT_BUTTON

	/* Widget sizes may have changed due to new values est, queue
	 * resize for the toplevel
	 */
	if(d->toplevel != NULL)
	    gtk_widget_queue_resize(d->toplevel);

	/* Map dialog */
	PDialogMap();

	/* Block until user response */
	d->block_loop_level++;
	gtk_main();

	/* Unmap dialog */
	PDialogUnmap();

	/* Break out of any subsequent main loops */
	while(d->block_loop_level > 0)
	{
	    gtk_main_quit();
	    d->block_loop_level--;
	}
	d->block_loop_level = 0;

	/* Update returns */
	if(nvalues != NULL)
	    *nvalues = d->response_nvals;
					 
	return(d->response_val);
}


/*
 *	Sets the size of the toplevel window of the prompt dialog.
 */
void PDialogSetSize(gint width, gint height)
{
	pdialog_struct *d = &pdialog;
	GtkWidget *w = (d != NULL) ? d->toplevel : NULL;
	if(w == NULL)
	    return;

	gtk_widget_set_usize(w, width, height);
}

/*
 *	Maps the PDialog.
 */
void PDialogMap(void)
{
	pdialog_struct *d = &pdialog;
	GtkWidget *w = (d != NULL) ? d->toplevel : NULL;
	if(w == NULL)
	    return;

	gtk_widget_show_raise(w);
}

/*
 *	Unmaps the PDialog.
 */
void PDialogUnmap(void)
{
	pdialog_struct *d = &pdialog;
	GtkWidget *w = (d != NULL) ? d->toplevel : NULL;
	if(w == NULL)
	    return;

	gtk_widget_hide(w);
}

/*
 *	Shuts down the PDialog.
 */
void PDialogShutdown(void)
{
	gint i;
	pdialog_struct *d = &pdialog;

	/* Reset responses */
	for(i = 0; i < d->response_nvals; i++)
	    g_free(d->response_val[i]);
	g_free(d->response_val);
	d->response_val = NULL;
	d->response_nvals = 0;

	/* Break out of all our main loops */
	while(d->block_loop_level > 0) 
	{
	    gtk_main_quit();
	    d->block_loop_level--;
	}
	d->block_loop_level = 0;

	/* Unmap the dialog */
	PDialogUnmap();

	/* Delete all the prompts on the dialog */
	PDialogDeleteAllPrompts();

	/* Delete the dialog */
	gtk_widget_destroy(d->toplevel);
	gtk_accel_group_unref(d->accelgrp);

	memset(d, 0x00, sizeof(pdialog_struct));
}
