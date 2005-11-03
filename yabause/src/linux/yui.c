/*  Copyright 2004-2005 Guillaume Duhamel
    Copyright 2005 Joost Peters
    Copyright 2005 Israel Jacques
    Copyright 2005 Fabien Coulon

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

#include "../yui.h"
#include "../sndsdl.h"
#include "../vidsdlgl.h"
#include "../persdl.h"
#include "../cs0.h"
#include "yuiconf.h"

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>
#include "yabause_logo.xpm"
#include "icon.xpm"

#include "SDL.h"

#define N_KEPT_FILES 8
#include <glib.h>
#include <envz.h>
#include <stdio.h>
#include <stdlib.h>

static struct {
  char* fileName;
  char* confPtr;
  size_t confLen;
} yuiConf = { "", NULL, 0 };

void GtkYuiConfInit() {

  FILE* in;
  const char* homeDir = g_get_home_dir();
  char* confName = "/.yabause";
  yuiConf.fileName = (char*)malloc( strlen( homeDir )+strlen( confName )+3 );
  strcpy( yuiConf.fileName, homeDir );
  strcpy( yuiConf.fileName+strlen(yuiConf.fileName), confName );
  in = fopen( yuiConf.fileName, "r" );

  if ( !in ) {
    yuiConf.confPtr = (char*)malloc(1); yuiConf.confPtr[0] = 0;
    yuiConf.confLen = 0L;
  }
  else  {
    fseek( in, 0L, SEEK_END );
    yuiConf.confLen = ftell( in );
    rewind( in );

    yuiConf.confPtr = (char*)malloc( yuiConf.confLen );
    fread( yuiConf.confPtr, 1, yuiConf.confLen, in );
    fclose( in );
  }
}

char* GtkYuiGetString( const char* name ) {

  return envz_get( yuiConf.confPtr, yuiConf.confLen, name );
}

void GtkYuiSetString( const char* name, const char* value ) {

  envz_add( &yuiConf.confPtr, &yuiConf.confLen, name, value );
 }

int GtkYuiGetInt( const char* name, int deflt ) {

  char* res = GtkYuiGetString( name );
  if ( !res ) return deflt;
  return atoi( res );
}

void GtkYuiSetInt( const char* name, const int value ) {

  char tmp[32];
  sprintf( tmp, "%d", value );
  GtkYuiSetString( name, tmp );
}

void GtkYuiStore() {

  FILE* out = fopen( yuiConf.fileName, "w" );
  if ( !out ) {

    fprintf( stderr, "Yabause:YuiConf : Cannot store configurations in file %s\n", yuiConf.fileName );
    return;
  }
  fwrite( yuiConf.confPtr, 1, yuiConf.confLen, out );
  fclose( out );
}

SH2Interface_struct *SH2CoreList[] = {
&SH2Interpreter,
NULL
};

PerInterface_struct *PERCoreList[] = {
&PERSDL,
NULL
};

CDInterface *CDCoreList[] = {
&DummyCD,
&ISOCD,
&ArchCD,
NULL
};

SoundInterface_struct *SNDCoreList[] = {
&SNDDummy,
&SNDSDL,
NULL
};

VideoInterface_struct *VIDCoreList[] = {
&VIDDummy,
&VIDSDLGL,
NULL
};

int cdcore = CDCORE_DEFAULT;
const char * bios = "jap.rom";
const char * iso_or_cd = 0;

struct {
  GtkWidget *window;
  GdkPixbuf *pixBufIcon;
  GtkWidget *buttonBios;
  GtkWidget *buttonCdRom;
  GtkWidget *comboBios;
  gint idcBios;
  GtkWidget *comboCdRom;
  gint idcCdRom;
  GtkWidget *checkIso;
  GtkWidget *checkCdRom;
  GtkWidget *buttonRun;
  GtkWidget *buttonPause;
  enum {GTKYUI_WAIT,GTKYUI_RUN,GTKYUI_PAUSE} running;
} GtkYui;

void GtkYuiAbout(void);

void YuiSetSoundEnable(int enablesound) {}

void YuiVideoResize(unsigned int w, unsigned int h, int isfullscreen) {}

int GtkWorkaround(void) {
	return ~(PERCore->HandleEvents());
}

void GtkYuiErrorPopup( gchar* text ) {
  
  GtkWidget* dialog = gtk_message_dialog_new (GTK_WINDOW(GtkYui.window),
				   GTK_DIALOG_DESTROY_WITH_PARENT,
				   GTK_MESSAGE_ERROR,
				   GTK_BUTTONS_CLOSE,
				   text );
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

static void GtkYuiRun(void) {

  switch ( GtkYui.running ) {

  case GTKYUI_WAIT: {
    GtkTreeIter iter;    
    yabauseinit_struct yinit;

    if ( gtk_combo_box_get_active( GTK_COMBO_BOX(GtkYui.comboBios) ) <= 0 ) {
      GtkYuiErrorPopup("You need to select a BIOS file.");
      return;
    }
    
    gtk_widget_set_sensitive( GtkYui.comboBios, FALSE );
    gtk_widget_set_sensitive( GtkYui.comboCdRom, FALSE );
    gtk_widget_set_sensitive( GtkYui.checkIso, FALSE );
    gtk_widget_set_sensitive( GtkYui.checkCdRom, FALSE );
    gtk_widget_set_sensitive( GtkYui.buttonBios, FALSE );
    gtk_widget_set_sensitive( GtkYui.buttonCdRom, FALSE );
    
    if ( gtk_combo_box_get_active( GTK_COMBO_BOX(GtkYui.comboCdRom) ) <= 0 ) {
      yinit.cdpath = NULL;
      yinit.cdcoretype = CDCORE_DEFAULT; 
    }
    else {
      gtk_combo_box_get_active_iter( GTK_COMBO_BOX(GtkYui.comboCdRom), &iter );
      gtk_tree_model_get( gtk_combo_box_get_model(GTK_COMBO_BOX(GtkYui.comboCdRom)), &iter,
			  0, &yinit.cdpath, -1 );
      yinit.cdcoretype = ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( GtkYui.checkIso ) ) )
	? CDCORE_ISO : CDCORE_ARCH;
    }
    yinit.percoretype = PERCORE_SDL;
    yinit.sh2coretype = SH2CORE_DEFAULT;
    yinit.vidcoretype = VIDCORE_SDLGL;
    yinit.sndcoretype = SNDCORE_SDL;
    yinit.carttype = CART_NONE;
    yinit.regionid = REGION_AUTODETECT;
    gtk_combo_box_get_active_iter( GTK_COMBO_BOX(GtkYui.comboBios), &iter );
    gtk_tree_model_get( gtk_combo_box_get_model(GTK_COMBO_BOX(GtkYui.comboBios)), &iter,
			0, &yinit.biospath, -1 );
    
    yinit.buppath = "backup.ram";
    yinit.mpegpath = NULL;
    yinit.cartpath = NULL;
    
    YabauseInit(&yinit);
  } /* fall through GTKYUI_PAUSE */
  case GTKYUI_PAUSE:
    gtk_widget_show( GtkYui.buttonPause );
    gtk_widget_hide( GtkYui.buttonRun );
    GtkYui.running = GTKYUI_RUN;
    g_idle_add((gboolean (*)(void*)) GtkWorkaround, (gpointer)1 );
  }
}

static void GtkYuiPause(void) {

  if ( GtkYui.running == GTKYUI_RUN ) {

    gtk_widget_show( GtkYui.buttonRun );
    gtk_widget_hide( GtkYui.buttonPause );
    g_idle_remove_by_data( (gpointer)1 );
    GtkYui.running = GTKYUI_PAUSE;
  }
}

static void loadCombo( GtkComboBox* combo, gint* idc, gchar* name ) {

  gchar tmp[64] = "remind_";
  gchar *label;
  gint i;
  strcpy( tmp+strlen(tmp), name );
  label = tmp+strlen(tmp);
  *(label+1)=0;

  gtk_combo_box_insert_text( combo, 0, "<none>" );
  *idc = 0;
  for ( i = 0 ; i < N_KEPT_FILES ; i++ ) {

    gchar *c;
    *label = 'A'+i;
    c = GtkYuiGetString( tmp );
    if ( c ) gtk_combo_box_insert_text( combo, ++(*idc), c );
  }
  gtk_combo_box_set_active( combo, *idc );
}

static void saveCombo( GtkComboBox* combo, gint idc, gchar* name ) {

  gchar tmp[64] = "remind_";
  gchar *label;
  gint i;
  gint from = idc - N_KEPT_FILES +1;
  if ( from < 1 ) from = 1;
  strcpy( tmp+strlen(tmp), name );
  label = tmp+strlen(tmp);
  *(label+1)=0;

  for ( i=from ; i <= idc ; i++ ) {

    gchar *c;
    GtkTreeIter iter;
    *label = 'A'+i-from;

    gtk_combo_box_set_active( combo, i );
    gtk_combo_box_get_active_iter( combo, &iter );
    gtk_tree_model_get( gtk_combo_box_get_model(combo), &iter, 0, &c, -1 );
    GtkYuiSetString( tmp, c );
  }
  GtkYuiStore();
}

static void GtkYuiSetBios(GtkWidget * w, GtkFileSelection * fs) {

  gint newid = ++GtkYui.idcBios;
  const gchar* fileName = gtk_file_selection_get_filename( GTK_FILE_SELECTION(fs) );
  GtkYuiSetString( "lastbiospath", fileName ); /* will be stored by saveCombo */

  gtk_combo_box_insert_text( GTK_COMBO_BOX( GtkYui.comboBios ), newid, fileName );
  saveCombo( GTK_COMBO_BOX(GtkYui.comboBios), newid, "bios" );
  gtk_combo_box_set_active( GTK_COMBO_BOX(GtkYui.comboBios), newid );	
  gtk_widget_destroy(GTK_WIDGET(fs));
}

static void GtkYuiSetCdRom(GtkWidget * w, GtkFileSelection * fs) {

  gint newid = ++GtkYui.idcCdRom;
  const gchar* fileName = gtk_file_selection_get_filename( GTK_FILE_SELECTION(fs) );
  GtkYuiSetString( "lastcdrompath", fileName ); /* will be stored by saveCombo */

  gtk_combo_box_insert_text( GTK_COMBO_BOX( GtkYui.comboCdRom ), newid, fileName );
  saveCombo( GTK_COMBO_BOX(GtkYui.comboCdRom), newid, "cdrom" );
  gtk_combo_box_set_active( GTK_COMBO_BOX(GtkYui.comboCdRom), newid );	
  gtk_widget_destroy(GTK_WIDGET(fs));
}

void GtkYuiBrowse( GtkButton* button, gpointer bCdRom ) {

  GtkWidget *filew = gtk_file_selection_new("File selection");
  gchar* lastpath = GtkYuiGetString( bCdRom ? "lastcdrompath" : "lastbiospath" );
  if ( lastpath ) gtk_file_selection_set_filename(GTK_FILE_SELECTION( filew ), lastpath );
  g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(filew)->ok_button), "clicked", 
		   bCdRom ? G_CALLBACK(GtkYuiSetCdRom) : G_CALLBACK(GtkYuiSetBios), 
		   (gpointer)filew);
  g_signal_connect_swapped(G_OBJECT(GTK_FILE_SELECTION(filew)->cancel_button), "clicked", G_CALLBACK(gtk_widget_destroy), G_OBJECT(filew));
  gtk_widget_show(filew);
}

int GtkYuiInit(void) {
	int fake_argc = 0;
	char ** fake_argv = NULL;
	char * text;

	GtkWidget *vbox;
	GtkWidget *hboxHigh;
	GtkWidget *hboxLow;
	GtkWidget *vboxHigh;
	GtkWidget *hboxCdRom;
	GtkWidget *hboxBios;
	GtkWidget *frameBios;
	GtkWidget *frameCdRom;
	GtkWidget *hboxRadioCD;
	GtkWidget *buttonQuit;
	GtkWidget *buttonHelp;
	
	gtk_init (&fake_argc, &fake_argv);
	GtkYui.running = GTKYUI_WAIT;
	GtkYui.pixBufIcon = gdk_pixbuf_new_from_xpm_data(icon_xpm);
	GtkYuiConfInit();

	GtkYui.window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_resizable( GTK_WINDOW( GtkYui.window ), FALSE );
	g_signal_connect(G_OBJECT (GtkYui.window), "delete_event", GTK_SIGNAL_FUNC(YuiQuit), NULL);
	gtk_window_set_icon( GTK_WINDOW(GtkYui.window), GtkYui.pixBufIcon );

	vbox = gtk_vbox_new (FALSE, 2);
	gtk_container_set_border_width( GTK_CONTAINER( vbox ),4 );
	gtk_container_add (GTK_CONTAINER (GtkYui.window), vbox);

	hboxHigh = gtk_hbox_new( FALSE, 4 );
	gtk_box_pack_start( GTK_BOX( vbox ), hboxHigh, TRUE, TRUE, 2 );
	gtk_box_pack_start( GTK_BOX( vbox ), gtk_hseparator_new(), TRUE, TRUE, 2 );

	hboxLow = gtk_hbox_new( FALSE, 2 );
	gtk_box_pack_start( GTK_BOX( vbox ), hboxLow, TRUE, TRUE, 2 );

	vboxHigh = gtk_vbox_new( FALSE, 5 );
	gtk_box_pack_start( GTK_BOX( hboxHigh ), vboxHigh, TRUE, TRUE, 2 );

	frameBios = gtk_frame_new( "Bios" );
	gtk_frame_set_shadow_type( GTK_FRAME( frameBios ), GTK_SHADOW_OUT );
	gtk_box_pack_start( GTK_BOX( vboxHigh ), frameBios, FALSE, FALSE, 2 );
	frameCdRom = gtk_frame_new( "CD-ROM" );
	gtk_frame_set_shadow_type( GTK_FRAME( frameCdRom ), GTK_SHADOW_OUT );
	gtk_box_pack_start( GTK_BOX( vboxHigh ), frameCdRom, FALSE, FALSE, 2 );

	hboxRadioCD = gtk_hbox_new( FALSE, 2 );
	gtk_box_pack_start( GTK_BOX( vboxHigh ), hboxRadioCD, FALSE, TRUE, 4 );

	GtkYui.checkIso = gtk_radio_button_new_with_label( NULL, "ISO file" );
	GtkYui.checkCdRom = gtk_radio_button_new_with_label_from_widget( GTK_RADIO_BUTTON(GtkYui.checkIso), "Native CD" );
	gtk_box_pack_start( GTK_BOX( hboxRadioCD ), GtkYui.checkIso, FALSE, TRUE, 4 );
	gtk_box_pack_start( GTK_BOX( hboxRadioCD ), GtkYui.checkCdRom, FALSE, TRUE, 4 );

	hboxBios = gtk_hbox_new( FALSE, 0 );
	gtk_container_set_border_width( GTK_CONTAINER( hboxBios ),1 );
	gtk_container_add (GTK_CONTAINER (frameBios), hboxBios);

	hboxCdRom = gtk_hbox_new( FALSE, 0 );
	gtk_container_set_border_width( GTK_CONTAINER( hboxCdRom ),1 );
	gtk_container_add (GTK_CONTAINER (frameCdRom), hboxCdRom);

	GtkYui.comboBios = gtk_combo_box_new_text();
	gtk_box_pack_start( GTK_BOX( hboxBios ), GtkYui.comboBios, TRUE, TRUE, 0 );
	loadCombo( GTK_COMBO_BOX(GtkYui.comboBios), &GtkYui.idcBios, "bios" );

	GtkYui.comboCdRom = gtk_combo_box_new_text();
	gtk_box_pack_start( GTK_BOX( hboxCdRom ), GtkYui.comboCdRom, TRUE, TRUE, 0 );
	loadCombo( GTK_COMBO_BOX(GtkYui.comboCdRom), &GtkYui.idcCdRom, "cdrom" );

	GtkYui.buttonBios = gtk_button_new();
	gtk_container_add( GTK_CONTAINER(GtkYui.buttonBios), gtk_image_new_from_stock( GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU ));
	gtk_box_pack_start( GTK_BOX( hboxBios ), GtkYui.buttonBios, FALSE, FALSE, 0 );

	GtkYui.buttonCdRom = gtk_button_new();
	gtk_container_add( GTK_CONTAINER(GtkYui.buttonCdRom), gtk_image_new_from_stock( GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU ));
	gtk_box_pack_start( GTK_BOX( hboxCdRom ), GtkYui.buttonCdRom, FALSE, FALSE, 0 );

	GtkYui.buttonRun = gtk_button_new_from_stock( GTK_STOCK_EXECUTE );
	gtk_box_pack_start( GTK_BOX( hboxLow ), GtkYui.buttonRun, FALSE, FALSE, 2 );

	GtkYui.buttonPause = gtk_button_new_with_label( "Pause" );
	gtk_box_pack_start( GTK_BOX( hboxLow ), GtkYui.buttonPause, FALSE, FALSE, 2 );

	gtk_box_pack_start( GTK_BOX( hboxLow ), gtk_vseparator_new(), TRUE, FALSE, 2 );

	buttonQuit = gtk_button_new_from_stock( GTK_STOCK_QUIT );
	gtk_box_pack_start( GTK_BOX( hboxLow ), buttonQuit, FALSE, FALSE, 2 );

	//	buttonHelp = gtk_button_new_with_label("About");
	buttonHelp = gtk_button_new_from_stock( GTK_STOCK_HELP );
	gtk_box_pack_start( GTK_BOX( hboxLow ), buttonHelp, FALSE, FALSE, 2 );

	text = g_strdup_printf ("Yabause %s", VERSION);
	gtk_window_set_title(GTK_WINDOW (GtkYui.window), text);
	g_free(text);

	g_signal_connect(GtkYui.buttonRun, "clicked",
			 G_CALLBACK(GtkYuiRun), NULL);
	g_signal_connect(GtkYui.buttonPause, "clicked",
			 G_CALLBACK(GtkYuiPause), NULL);
	g_signal_connect_swapped(buttonQuit, "clicked",
				 G_CALLBACK(YuiQuit), NULL );
	g_signal_connect(GtkYui.buttonBios, "clicked",
			 G_CALLBACK(GtkYuiBrowse), (gpointer)FALSE );
	g_signal_connect(GtkYui.buttonCdRom, "clicked",
			 G_CALLBACK(GtkYuiBrowse), (gpointer)TRUE );
	g_signal_connect(buttonHelp, "clicked",
			 G_CALLBACK(GtkYuiAbout), NULL );

	gtk_widget_show_all (GtkYui.window);
	gtk_widget_hide( GtkYui.buttonPause );
}

void YuiSetBiosFilename(const char * biosfilename) {
        bios = biosfilename;
}

void YuiSetIsoFilename(const char * isofilename) {
	cdcore = CDCORE_ISO;
	iso_or_cd = isofilename;
}

void YuiSetCdromFilename(const char * cdromfilename) {
	cdcore = CDCORE_ARCH;
	iso_or_cd = cdromfilename;
}

void YuiHideShow(void) {
}

void YuiQuit(void) {
  
   gtk_main_quit();
}

int YuiInit(void) {

   GtkYuiInit();
   gtk_main();
   return 0;
}

void YuiErrorMsg(const char *string) {

   fprintf(stderr, string);
}

/* Code taken from beep media player:
   http://www.sosdg.org/~larne/w/BMP_Homepage

  Copyright (C) 1998-2003  Peter Alm, Mikael Alm, Olle Hallnas,
                           Thomas Nilsson and 4Front Technologies
  Copyright (C) 2000-2003  Haavard Kvaalen
*/

enum {
    COL_LEFT,
    COL_RIGHT,
    N_COLS
};

static const gchar *bmp_brief =
    N_("<big><b>Yabause %s</b></big>\n"
       "Yabause is a Sega Saturn emulator.\n"
       "\n"
       "Copyright (C) 2004-2005 Guillaume Duhamel\n");

static const gchar *credit_text[] = {
    N_("Developers:"),
    N_("Guillaume Duhamel"),
    N_("Theo Berkau"),
    N_("Adam Green"),
    N_("Lawrence Sebald"),
    N_("menace690"),
    N_("Joost Peters"),
    NULL,

    N_("With Additional Help:"),
    N_("Fabien Autrel"),
    N_("Runik"),
    N_("Israel Jacques"),
    NULL,

    N_("Website Design:"),
    N_("Romain Vallet"),
    NULL,

    NULL
};

static GtkWidget *
GtkYuiGenerateCreditList(const gchar * text[], gboolean sec_space)
{
    GtkWidget *scrollwin;
    GtkWidget *treeview;
    GtkListStore *list_store;
    GtkTreeIter iter;
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;
    const gchar *const *item;

    list_store = gtk_list_store_new(N_COLS, G_TYPE_STRING, G_TYPE_STRING);

    item = text;

    while (*item) {
        gtk_list_store_append(list_store, &iter);
        gtk_list_store_set(list_store, &iter,
                           COL_LEFT, _(item[0]), COL_RIGHT, _(item[1]), -1);
        item += 2;

        while (*item) {
            gtk_list_store_append(list_store, &iter);
            gtk_list_store_set(list_store, &iter,
                               COL_LEFT, "", COL_RIGHT, _(*item++), -1);
        }

        ++item;

        if (*item && sec_space) {
            gtk_list_store_append(list_store, &iter);
            gtk_list_store_set(list_store, &iter,
                               COL_LEFT, "", COL_RIGHT, "", -1);
        }
    }

    treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list_store));
    gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(treeview), FALSE);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)),
        GTK_SELECTION_NONE);

    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "xalign", 1.0, NULL);
    column = gtk_tree_view_column_new_with_attributes("Left", renderer,
                                                      "text", COL_LEFT, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "xalign", 0.0, NULL);
    column = gtk_tree_view_column_new_with_attributes("Right", renderer,
                                                      "text", COL_RIGHT,
                                                      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    scrollwin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin),
                                   GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrollwin), GTK_SHADOW_IN);
    gtk_container_add(GTK_CONTAINER(scrollwin), treeview);
    gtk_container_set_border_width(GTK_CONTAINER(scrollwin), 10);

    gtk_widget_show_all(scrollwin);

    return scrollwin;
}

void GtkYuiAbout(void) {
    static GtkWidget *about_window = NULL;

    GdkPixmap *yabause_logo_pmap = NULL, *yabause_logo_mask = NULL;
    GtkWidget *about_vbox;
    GtkWidget *about_credits_logo_box, *about_credits_logo_frame;
    GtkWidget *about_credits_logo;
    GtkWidget *about_notebook;
    GtkWidget *list;
    GtkWidget *bbox, *close_btn;
    GtkWidget *label;
    gchar *text;

    if (about_window)
        return;

    about_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_icon( GTK_WINDOW(about_window), GtkYui.pixBufIcon );
    gtk_window_set_type_hint(GTK_WINDOW(about_window),
                             GDK_WINDOW_TYPE_HINT_DIALOG);

    gtk_window_set_default_size(GTK_WINDOW(about_window), -1, 480);
    gtk_window_set_title(GTK_WINDOW(about_window), _("About Yabause"));
    gtk_window_set_position(GTK_WINDOW(about_window), GTK_WIN_POS_CENTER);
    gtk_window_set_resizable(GTK_WINDOW(about_window), TRUE);
    gtk_container_set_border_width(GTK_CONTAINER(about_window), 10);

    g_signal_connect(about_window, "destroy",
                     G_CALLBACK(gtk_widget_destroyed), &about_window);

    gtk_widget_realize(about_window);

    about_vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(about_window), about_vbox);

    if (!yabause_logo_pmap)
        yabause_logo_pmap =
            gdk_pixmap_create_from_xpm_d(about_window->window,
                                         &yabause_logo_mask, NULL, yabause_logo);

    about_credits_logo_box = gtk_hbox_new(TRUE, 0);
    gtk_box_pack_start(GTK_BOX(about_vbox), about_credits_logo_box,
                       FALSE, FALSE, 0);


    about_credits_logo_frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(about_credits_logo_frame),
                              GTK_SHADOW_ETCHED_OUT);
    gtk_box_pack_start(GTK_BOX(about_credits_logo_box),
                       about_credits_logo_frame, FALSE, FALSE, 0);

    about_credits_logo = gtk_pixmap_new(yabause_logo_pmap, yabause_logo_mask);
    gtk_container_add(GTK_CONTAINER(about_credits_logo_frame),
                      about_credits_logo);

    label = gtk_label_new(NULL);
    text = g_strdup_printf(_(bmp_brief), VERSION);
    gtk_label_set_markup(GTK_LABEL(label), text);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
    g_free(text);

    gtk_box_pack_start(GTK_BOX(about_vbox), label, FALSE, FALSE, 0);

    about_notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(about_vbox), about_notebook, TRUE, TRUE, 0);

    list = GtkYuiGenerateCreditList(credit_text, TRUE);
    gtk_notebook_append_page(GTK_NOTEBOOK(about_notebook), list,
                             gtk_label_new(_("Credits")));

    bbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
    gtk_box_pack_start(GTK_BOX(about_vbox), bbox, FALSE, FALSE, 0);

    close_btn = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    g_signal_connect_swapped(close_btn, "clicked",
                             G_CALLBACK(gtk_widget_destroy), about_window);
    GTK_WIDGET_SET_FLAGS(close_btn, GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(bbox), close_btn, TRUE, TRUE, 0);
    gtk_widget_grab_default(close_btn);

    gtk_widget_show_all(about_window);
}

