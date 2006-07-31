#include "settings.h"

#include "yuifileentry.h"
#include "yuirange.h"
#include "yuipage.h"
#include "yuiinputentry.h"
#include "yuiresolution.h"

YuiRangeItem regions[] = {
	{ "Auto" , "Auto-detect" },
	{ "J" , "Japan" },
	{ "T", "Asia" },
	{ "U", "North America" },
	{ "B", "Central/South America" },
	{ "K", "Korea" },
	{ "A", "Asia" },
	{ "E", "Europe + others" },
	{ "L", "Central/South America" },
	{ 0, 0 }
};

YuiRangeItem carttypes[] = {
	{ "0", "None" },
	{ "1", "Pro Action Replay" },
	{ "2", "4 Mbit Backup Ram" },
	{ "3", "8 Mbit Backup Ram" },
	{ "4", "16 Mbit Backup Ram" },
	{ "5", "32 Mbit Backup Ram" },
	{ "6", "8 Mbit Dram" },
	{ "7", "32 Mbit Dram" },
	{ "8", "Netlink" },
	{ "9", "16 Mbit ROM" },
	{ 0, 0 }
};

YuiRangeItem cdcores[] = {
	{ "0" , "Dummy CD" },
	{ "1" , "ISO or CUE file" },
	{ "2" , "Cdrom device" },
	{ 0, 0 }
};

YuiRangeItem vidcores[] = {
	{ "0", "Dummy Video Interface" },
	{ "1", "OpenGL Video Interface" },
	{ "2", "Software Video Interface" },
	{ 1, 0 }
};

YuiRangeItem sndcores[] = {
	{ "0", "Dummy Sound Interface" },
	{ "1", "SDL Sound Interface" },
	{ 1, 0 }
};

const gchar * keys1[] = { "Up", "Right", "Down", "Left", "Right trigger", "Left trigger", "Start", 0 };
const gchar * keys2[] = { "A", "B", "C", "X", "Y", "Z", 0 };

YuiPageItem biositems[] = {
	{ "BiosPath", YUI_FILE_SETTING, 0 },
	{ 0, 0 }
};

YuiPageItem cditems[] = {
	{ "CDROMDrive", YUI_FILE_SETTING, 0 },
	{ "CDROMCore", YUI_RANGE_SETTING, cdcores },
	{ 0, 0 }
};

YuiPageItem regionitems[] = {
	{ "Region", YUI_RANGE_SETTING, regions },
	{ 0, 0 }
};

YuiPageDesc general_desc[] = {
	{ "<b>Bios</b>", biositems },
	{ "<b>Cdrom</b>", cditems },
	{ "<b>Region</b>", regionitems },
	{ 0, 0}
};

YuiPageItem viditems[] = {
	{ "VideoCore", YUI_RANGE_SETTING, vidcores },
	{ 0, 0 }
};

YuiPageItem resitems[] = {
	{ "Resolution", YUI_RESOLUTION_SETTING, 0 },
	{ 0, 0 }
};

YuiPageItem snditems[] = {
	{ "SoundCore", YUI_RANGE_SETTING, sndcores },
	{ 0, 0 }
};

YuiPageDesc video_sound_desc[] = {
	{ "<b>Video core</b>", viditems },
	{ "<b>Resolution</b>", resitems },
	{ "<b>Sound core</b>", snditems },
	{ 0, 0 }
};

YuiPageItem cartitems[] = {
	{ "CartPath", YUI_FILE_SETTING, 0 },
	{ "CartType", YUI_RANGE_SETTING, carttypes },
	{ 0, 0 }
};

YuiPageItem bupitems[] = {
	{ "BackupRamPath", YUI_FILE_SETTING, 0 },
	{ 0, 0 }
};

YuiPageItem mpegitems[] = {
	{ "MpegRomPath", YUI_FILE_SETTING, 0 },
	{ 0, 0 }
};

YuiPageDesc memory_desc[] = {
	{ "<b>Cartridge</b>", cartitems },
	{ "<b>Memory</b>", bupitems },
	{ "<b>Mpeg ROM</b>", mpegitems },
	{ 0, 0 }
};

GtkWidget*
create_dialog1 (void)
{
  GtkWidget *dialog1;
  GtkWidget *notebook1;
  GtkWidget *vbox17;
  GtkWidget *hbox22;
  GtkWidget *table4;
  GtkWidget *table5;
  GtkWidget *button11;
  GtkWidget *button12;
  GtkWidget * general, * video_sound, * cart_memory;

  dialog1 = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dialog1), "Yabause configuration");
  gtk_window_set_icon_name (GTK_WINDOW (dialog1), "gtk-preferences");
  gtk_window_set_type_hint (GTK_WINDOW (dialog1), GDK_WINDOW_TYPE_HINT_DIALOG);

  notebook1 = gtk_notebook_new ();
  
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog1)->vbox), notebook1, TRUE, TRUE, 0);

  /*
   * General configuration
   */
  general = yui_page_new(keyfile, "General", general_desc);
  
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook1), general, gtk_label_new ("General"));

  /*
   * Video/Sound configuration
   */
  video_sound = yui_page_new(keyfile, "General", video_sound_desc);
  
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook1), video_sound, gtk_label_new ("Video/Sound"));

  /*
   * Cart/Memory configuration
   */
  cart_memory = yui_page_new(keyfile, "General", memory_desc);
  
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook1), cart_memory, gtk_label_new ("Cart/Memory"));

  /*
   * Input Configuration
   */
  vbox17 = gtk_vbox_new (FALSE, 0);
  
  hbox22 = gtk_hbox_new (FALSE, 0);
  
  gtk_box_pack_start (GTK_BOX (vbox17), hbox22, FALSE, TRUE, 0);

  table4 = yui_input_entry_new(keyfile, "Input", keys1);
  
  gtk_box_pack_start (GTK_BOX (hbox22), table4, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (hbox22), gtk_vseparator_new(), TRUE, TRUE, 0);

  table5 = yui_input_entry_new(keyfile, "Input", keys2);
  
  gtk_box_pack_start (GTK_BOX (hbox22), table5, TRUE, TRUE, 0);

  gtk_notebook_append_page(GTK_NOTEBOOK(notebook1), vbox17, gtk_label_new ("Input"));

  /*
   * Dialog buttons
   */
  button11 = gtk_button_new_from_stock ("gtk-cancel");
  
  gtk_dialog_add_action_widget (GTK_DIALOG (dialog1), button11, GTK_RESPONSE_CANCEL);
  GTK_WIDGET_SET_FLAGS (button11, GTK_CAN_DEFAULT);

  button12 = gtk_button_new_from_stock ("gtk-ok");
  
  gtk_dialog_add_action_widget (GTK_DIALOG (dialog1), button12, GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS (button12, GTK_CAN_DEFAULT);

  return dialog1;
}

