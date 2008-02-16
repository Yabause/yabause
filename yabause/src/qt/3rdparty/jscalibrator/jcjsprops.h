/*
                        Joystick Properties Dialog
 */

#ifndef JCJSPROPS_H
#define JCJSPROPS_H

#include <gtk/gtk.h>


/*
 *	Joystick properties dialog structure:
 */
typedef struct {

	gboolean initialized;
	gboolean map_state;

	gpointer core_ptr;	/* Pointer to a jc_struct. */

	GtkAccelGroup *accelgrp;

	GtkWidget	*toplevel,
			*main_vbox,
			*notebook,

			*general_page_vbox,
			*force_feedback_page_vbox,

			*ok_btn,
			*apply_btn,
			*cancel_btn;

	GtkWidget	*general_icon_fixed,
			*general_icon_pm,
			*name_entry,	/* Descriptive name. */
			*device_entry,
			*driver_version_entry,
			*calibration_file_entry,
			*total_axises_label,
			*total_buttons_label,
			*last_calibrated_label,
			*has_changes_label,

			*ff_icon_fixed,
			*ff_icon_pm,
			*product_id_entry;

} jc_jsprops_struct;


extern jc_jsprops_struct *JCJSPropsNew(
	gpointer core_ptr		/* jc_struct * */
);
extern void JCJSPropsGetValues(jc_jsprops_struct *jsp);
extern void JCJSPropsSetValues(jc_jsprops_struct *jsp);
extern void JCJSPropsMap(jc_jsprops_struct *jsp);
extern void JCJSPropsUnmap(jc_jsprops_struct *jsp);
extern void JCJSPropsDelete(jc_jsprops_struct *jsp);



#endif	/* JCJSPROPS_H */
