/*
			      Floating Prompt
 */

#ifndef FPROMPT_H
#define FPROMPT_H

#include <gtk/gtk.h>


/*
 *	Flags:
 */
typedef enum {
	FPROMPT_FLAG_LABEL		= (1 << 0),	/* Show Label */
	FPROMPT_FLAG_SELECT_VALUE	= (1 << 2),	/* Select Value */
	FPROMPT_FLAG_BROWSE_BTN		= (1 << 8),	/* Show Browse Button */
	FPROMPT_FLAG_OK_BTN		= (1 << 9),	/* Show OK Button */
	FPROMPT_FLAG_CANCEL_BTN		= (1 << 10)	/* Show Cancel Button */
} fprompt_flags;

/*
 *      Map Codes:
 */
typedef enum {
	FPROMPT_MAP_NO_MOVE,
	FPROMPT_MAP_TO_POINTER,		/* Centered around pointer */
	FPROMPT_MAP_TO_POINTER_WINDOW	/* Window under pointer */
} fprompt_map_code;

/*
 *	Shadow Styles:
 */
typedef enum {
	FPROMPT_SHADOW_NONE,
	FPROMPT_SHADOW_DITHERED,
	FPROMPT_SHADOW_BLACK,
	FPROMPT_SHADOW_DARKENED
} fprompt_shadow_style;


#define FPROMPT_DEF_WIDTH		250
#define FPROMPT_DEF_HEIGHT		30

#define FPROMPT_BTN_WIDTH		(20 + (2 * 2))
#define FPROMPT_BTN_HEIGHT		(20 + (2 * 2))


extern gint FPromptInit(void);
extern void FPromptSetTransientFor(GtkWidget *w);
extern gboolean FPromptIsQuery(void);
extern void FPromptBreakQuery(void);
extern void FPromptSetPosition(const gint x, const gint y);
extern void FPromptSetShadowStyle(const fprompt_shadow_style shadow);
extern gint FPromptMapQuery(
	const gchar *label,
	const gchar *value,
	const gchar *tooltip_message,
	const fprompt_map_code map_code,
	const gint width, const gint height,
	const fprompt_flags flags,
	gpointer data,
	gchar *(*browse_cb)(gpointer, const gchar *),
	void (*apply_cb)(gpointer, const gchar *),
	void (*cancel_cb)(gpointer)
);
extern void FPromptMap(void);
extern void FPromptUnmap(void);
extern void FPromptShutdown(void);


#endif	/* FPROMPT_H */
