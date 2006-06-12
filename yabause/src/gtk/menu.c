#include "settings.h"
#include "yuiwindow.h"

GtkWidget* create_menu(YuiWindow * window1) {
  GtkWidget *vbox1;
  GtkWidget *menubar1;
  GtkWidget *menuitem1;
  GtkWidget *menuitem1_menu;
  GtkWidget *new1;
  GtkWidget *open1;
  GtkWidget *save1;
  GtkWidget *separatormenuitem1;
  GtkWidget *quit1;
  GtkWidget *view1;
  GtkWidget *view1_menu;
  GtkWidget *fps1;
  GtkWidget *layer1;
  GtkWidget *layer1_menu;
  GtkWidget *vdp3;
  GtkWidget *nbg1;
  GtkWidget *nbg2;
  GtkWidget *nbg3;
  GtkWidget *nbg4;
  GtkWidget *rbg1;
  GtkWidget *fullscreen1;
  GtkWidget *menuitem3;
  GtkWidget *menuitem3_menu;
  GtkWidget *msh1;
  GtkWidget *ssh1;
  GtkWidget *vdp2;
  GtkWidget *vdp1;
  GtkWidget *menuitem4;
  GtkWidget *menuitem4_menu;
  GtkWidget *about1;
  GtkAccelGroup *accel_group;

  accel_group = gtk_accel_group_new ();

  menubar1 = gtk_menu_bar_new ();
  gtk_widget_show (menubar1);
  //gtk_box_pack_start (GTK_BOX (vbox1), menubar1, FALSE, FALSE, 0);

  menuitem1 = gtk_menu_item_new_with_mnemonic ("_Yabause");
  gtk_widget_show (menuitem1);
  gtk_container_add (GTK_CONTAINER (menubar1), menuitem1);

  menuitem1_menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem1), menuitem1_menu);

  new1 = gtk_image_menu_item_new_from_stock ("gtk-preferences", accel_group);
  gtk_widget_show (new1);
  gtk_container_add (GTK_CONTAINER (menuitem1_menu), new1);

  open1 = gtk_image_menu_item_new_from_stock ("gtk-media-play", accel_group);
  gtk_widget_show (open1);
  gtk_container_add (GTK_CONTAINER (menuitem1_menu), open1);

  save1 = gtk_image_menu_item_new_from_stock ("gtk-media-pause", accel_group);
  gtk_widget_show (save1);
  gtk_container_add (GTK_CONTAINER (menuitem1_menu), save1);

  separatormenuitem1 = gtk_separator_menu_item_new ();
  gtk_widget_show (separatormenuitem1);
  gtk_container_add (GTK_CONTAINER (menuitem1_menu), separatormenuitem1);
  gtk_widget_set_sensitive (separatormenuitem1, FALSE);

  quit1 = gtk_image_menu_item_new_from_stock ("gtk-quit", accel_group);
  gtk_widget_show (quit1);
  gtk_container_add (GTK_CONTAINER (menuitem1_menu), quit1);

  view1 = gtk_menu_item_new_with_mnemonic ("_View");
  gtk_widget_show (view1);
  gtk_container_add (GTK_CONTAINER (menubar1), view1);

  view1_menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (view1), view1_menu);

  fps1 = gtk_check_menu_item_new_with_mnemonic ("FPS");
  gtk_widget_show (fps1);
  gtk_container_add (GTK_CONTAINER (view1_menu), fps1);

  layer1 = gtk_menu_item_new_with_mnemonic ("Layer");
  gtk_widget_show (layer1);
  gtk_container_add (GTK_CONTAINER (view1_menu), layer1);

  layer1_menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (layer1), layer1_menu);

  vdp3 = gtk_check_menu_item_new_with_mnemonic ("Vdp1");
  gtk_widget_show (vdp3);
  gtk_container_add (GTK_CONTAINER (layer1_menu), vdp3);
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (vdp3), TRUE);

  nbg1 = gtk_check_menu_item_new_with_mnemonic ("NBG0");
  gtk_widget_show (nbg1);
  gtk_container_add (GTK_CONTAINER (layer1_menu), nbg1);
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (nbg1), TRUE);

  nbg2 = gtk_check_menu_item_new_with_mnemonic ("NBG1");
  gtk_widget_show (nbg2);
  gtk_container_add (GTK_CONTAINER (layer1_menu), nbg2);
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (nbg2), TRUE);

  nbg3 = gtk_check_menu_item_new_with_mnemonic ("NBG2");
  gtk_widget_show (nbg3);
  gtk_container_add (GTK_CONTAINER (layer1_menu), nbg3);
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (nbg3), TRUE);

  nbg4 = gtk_check_menu_item_new_with_mnemonic ("NBG3");
  gtk_widget_show (nbg4);
  gtk_container_add (GTK_CONTAINER (layer1_menu), nbg4);
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (nbg4), TRUE);

  rbg1 = gtk_check_menu_item_new_with_mnemonic ("RBG1");
  gtk_widget_show (rbg1);
  gtk_container_add (GTK_CONTAINER (layer1_menu), rbg1);
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (rbg1), TRUE);

#if ((GLIB_MAJOR_VERSION < 2) || ((GLIB_MAJOR_VERSION == 2) && (GLIB_MINOR_VERSION < 8)))
  fullscreen1 = gtk_menu_item_new_with_mnemonic ("Fullscreen");
#else
  fullscreen1 = gtk_image_menu_item_new_from_stock ("gtk-fullscreen", accel_group);
#endif
  g_signal_connect_swapped(fullscreen1, "activate", G_CALLBACK(yui_window_toggle_fullscreen), window1);
  gtk_container_add (GTK_CONTAINER (view1_menu), fullscreen1);

  menuitem3 = gtk_menu_item_new_with_mnemonic ("_Debug");
  gtk_widget_show (menuitem3);
  gtk_container_add (GTK_CONTAINER (menubar1), menuitem3);

  menuitem3_menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem3), menuitem3_menu);

  msh1 = gtk_menu_item_new_with_mnemonic ("MSH2");
  gtk_widget_show (msh1);
  gtk_container_add (GTK_CONTAINER (menuitem3_menu), msh1);

  ssh1 = gtk_menu_item_new_with_mnemonic ("SSH2");
  gtk_widget_show (ssh1);
  gtk_container_add (GTK_CONTAINER (menuitem3_menu), ssh1);

  vdp2 = gtk_menu_item_new_with_mnemonic ("Vdp1");
  gtk_widget_show (vdp2);
  gtk_container_add (GTK_CONTAINER (menuitem3_menu), vdp2);

  vdp1 = gtk_menu_item_new_with_mnemonic ("Vdp2");
  gtk_widget_show (vdp1);
  gtk_container_add (GTK_CONTAINER (menuitem3_menu), vdp1);

  menuitem4 = gtk_menu_item_new_with_mnemonic ("_Help");
  gtk_widget_show (menuitem4);
  gtk_container_add (GTK_CONTAINER (menubar1), menuitem4);

  menuitem4_menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem4), menuitem4_menu);

  about1 = gtk_image_menu_item_new_from_stock ("gtk-about", accel_group);
  gtk_widget_show (about1);
  gtk_container_add (GTK_CONTAINER (menuitem4_menu), about1);

  gtk_window_add_accel_group (GTK_WINDOW (window1), accel_group);

  g_signal_connect(new1, "activate", yui_conf, 0);
  g_signal_connect(open1, "activate", yui_run, 0);
  g_signal_connect(quit1, "activate", gtk_main_quit, 0);

  return menubar1;
}

