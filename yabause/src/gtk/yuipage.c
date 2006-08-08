/*  Copyright 2006 Guillaume Duhamel

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <gtk/gtk.h>

#include "yuifileentry.h"
#include "yuirange.h"
#include "yuiresolution.h"
#include "yuipage.h"

static void yui_page_class_init	(YuiPageClass * klass);
static void yui_page_init		(YuiPage      * yfe);

GType yui_page_get_type (void) {
  static GType yfe_type = 0;

  if (!yfe_type)
    {
      static const GTypeInfo yfe_info =
      {
	sizeof (YuiPageClass),
	NULL, /* base_init */
        NULL, /* base_finalize */
	(GClassInitFunc) yui_page_class_init,
        NULL, /* class_finalize */
	NULL, /* class_data */
        sizeof (YuiPage),
	0,
	(GInstanceInitFunc) yui_page_init,
      };

      yfe_type = g_type_register_static(GTK_TYPE_VBOX, "YuiPage", &yfe_info, 0);
    }

  return yfe_type;
}

static void yui_page_class_init (YuiPageClass * klass) {
}

static void yui_page_init (YuiPage * yp) {
}

GtkWidget * yui_page_new(GKeyFile * keyfile, const gchar * group, YuiPageDesc * desc) {
	GtkWidget * label;
	GtkWidget * frame;
	GtkWidget * box;
	GtkWidget * widget;
	YuiPage * yp;
	guint i, j;

	widget = GTK_WIDGET(g_object_new(yui_page_get_type(), 0));
	yp = YUI_PAGE(widget);

	j = 0;
	while(desc[j].name) {
		frame = gtk_frame_new(NULL);
  
		gtk_box_pack_start(GTK_BOX(yp), frame, FALSE, TRUE, 0);
		gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);

		box = gtk_vbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(frame), box);

		label = gtk_label_new(desc[j].name);
		gtk_frame_set_label_widget(GTK_FRAME(frame), label);
		gtk_label_set_use_markup(GTK_LABEL(label), TRUE);

		i = 0;
		while(desc[j].items[i].name) {
			switch(desc[j].items[i].type) {
				case YUI_FILE_SETTING:
					gtk_container_add(GTK_CONTAINER(box), yui_file_entry_new(keyfile, group, desc[j].items[i].name));
					break;
				case YUI_RANGE_SETTING:
					gtk_container_add(GTK_CONTAINER(box), yui_range_new(keyfile, group, desc[j].items[i].name, desc[j].items[i].data));
					break;
				case YUI_RESOLUTION_SETTING:
					gtk_container_add(GTK_CONTAINER(box), yui_resolution_new(keyfile, group));
					break;
			}
			i++;
		}
		j++;
	}

	return widget;
}
