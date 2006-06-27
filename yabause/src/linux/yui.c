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

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib/gi18n.h>
#include "yabause_logo.xpm"
#include "icon.xpm"

#include "../scu.h"
#include "../yui.h"
#include "../sndsdl.h"
#include "../vidogl.h"
#include "../vidsoft.h"
#include "../persdl.h"
#include "../cs0.h"
#include "../scsp.h"
#include "SDL.h"

#define FS_X_DEFAULT 640
#define FS_Y_DEFAULT 448
#define CARTTYPE_DEFAULT 0
#define N_KEPT_FILES 8

#include <glib.h>
#include <envz.h>
#include <stdio.h>
#include <stdlib.h>

#define DONT_PROFILE
#include "../profile.h"

static gboolean forceSoundDisabled = FALSE;
static gchar* forceBiosFileName = NULL;
static gchar* forceCdFileName = NULL;
static gint   forceCdType = 0;

static void yuiRun(void);
static void yuiPause(void);

typedef struct {

  GtkWidget *frame;
  GtkWidget *hbox;
  GtkWidget *combo;
  GtkWidget *button;
  GtkWidget *filew;

  gchar *name;
  gchar lastPathEntry[64];
  gint  idc;
} ComboFileSelect;

static struct {
  GtkWidget *window;
  GdkPixbuf *pixBufIcon;
  ComboFileSelect *cfBios, *cfCdRom;
  GtkWidget *checkIso;
  GtkWidget *checkCdRom;
  GtkWidget *buttonRun;
  GtkWidget *buttonPause;
  GtkWidget *buttonReset;
  GtkWidget *buttonFs;
  GtkWidget *buttonSettings;
  enum {GTKYUI_WAIT,GTKYUI_RUN,GTKYUI_PAUSE} running;
} yui;

/* --------------------------------------------------------------------------------- */
/* managing the configuration file $HOME/.yabause                                    */

static struct {
  char* fileName;
  char* confPtr;
  size_t confLen;
} yuiConf = { "", NULL, 0 };

static void yuiConfInit() {

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

static char* yuiGetString( const char* name ) {

  return envz_get( yuiConf.confPtr, yuiConf.confLen, name );
}

static void yuiSetString( const char* name, const char* value ) {

  envz_add( &yuiConf.confPtr, &yuiConf.confLen, name, value );
 }

static int yuiGetInt( const char* name, int deflt ) {

  char* res = yuiGetString( name );
  if ( !res ) return deflt;
  return atol( res );
}

static void yuiSetInt( const char* name, const int value ) {

  char tmp[32];
  sprintf( tmp, "%d", value );
  yuiSetString( name, tmp );
}

static void yuiStore() {

  FILE* out = fopen( yuiConf.fileName, "w" );
  if ( !out ) {

    fprintf( stderr, "Yabause:yuiConf : Cannot store configurations in file %s\n", yuiConf.fileName );
    return;
  }
  fwrite( yuiConf.confPtr, 1, yuiConf.confLen, out );
  fclose( out );
}

/* --------------------------------------------------------*/
/* Interface lists                                         */

SH2Interface_struct *SH2CoreList[] = {
&SH2Interpreter,
&SH2DebugInterpreter,
NULL
};
#define SH2CORE_YUI_DEFAULT 1

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
&VIDOGL,
&VIDSoft,
NULL
};
#define VIDCORE_YUI_DEFAULT 1

/* ------------------------------------------------------------ */
/* ComboFileSelect - Control for file selection                 */

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
    c = yuiGetString( tmp );
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
    yuiSetString( tmp, c );
  }
  yuiStore();
}

static void yuiFileSelect(GtkWidget * w, ComboFileSelect* cf ) {

  gint newid = ++(cf->idc);
  const gchar* fileName = gtk_file_selection_get_filename( GTK_FILE_SELECTION(cf->filew) );
  yuiSetString( cf->lastPathEntry, fileName ); /* will be stored by saveCombo */

  gtk_combo_box_insert_text( GTK_COMBO_BOX( cf->combo ), newid, fileName );
  saveCombo( GTK_COMBO_BOX(cf->combo), newid, cf->name );
  gtk_combo_box_set_active( GTK_COMBO_BOX(cf->combo), newid );
  gtk_widget_destroy(cf->filew);
}

static void yuiFileCancel(GtkWidget * w, ComboFileSelect* cf ) {

  const gchar* fileName = gtk_file_selection_get_filename( GTK_FILE_SELECTION(cf->filew) );
  yuiSetString( cf->lastPathEntry, fileName );
  gtk_widget_destroy(cf->filew);
}


static void yuiBrowse( GtkButton* button, ComboFileSelect* cf ) {

  gchar* lastpath = yuiGetString( cf->lastPathEntry );
  cf->filew = gtk_file_selection_new("File selection");
  gtk_window_set_modal( GTK_WINDOW(cf->filew), TRUE );

  if ( lastpath ) gtk_file_selection_set_filename(GTK_FILE_SELECTION( cf->filew ), lastpath );
  g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(cf->filew)->ok_button), "clicked", 
		   G_CALLBACK(yuiFileSelect),
		   (gpointer)cf);
  g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(cf->filew)->cancel_button), "clicked", 
		   G_CALLBACK(yuiFileCancel),
		   (gpointer)cf);

  gtk_widget_show(cf->filew);
}

static ComboFileSelect* cfNew( gchar *_name, gchar *title ) {

  ComboFileSelect* cf = (ComboFileSelect*)malloc( sizeof( ComboFileSelect ) );

  cf->name = (gchar*)malloc( strlen(_name)+1 );
  strcpy( cf->name, _name );
  strcpy( cf->lastPathEntry, "lastpath_" );
  strcpy( cf->lastPathEntry+strlen( cf->lastPathEntry ), _name );

  cf->frame = gtk_frame_new( title );
  gtk_frame_set_shadow_type( GTK_FRAME( cf->frame ), GTK_SHADOW_OUT );
  
  cf->hbox = gtk_hbox_new( FALSE, 0 );
  gtk_container_set_border_width( GTK_CONTAINER( cf->hbox ),1 );
  gtk_container_add (GTK_CONTAINER (cf->frame), cf->hbox);

  cf->combo = gtk_combo_box_new_text();
  gtk_box_pack_start( GTK_BOX( cf->hbox ), cf->combo, TRUE, TRUE, 0 );
  loadCombo( GTK_COMBO_BOX(cf->combo), &cf->idc, cf->name );

  cf->button = gtk_button_new();
  gtk_container_add( GTK_CONTAINER(cf->button), gtk_image_new_from_stock( GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU ));
  gtk_box_pack_start( GTK_BOX( cf->hbox ), cf->button, FALSE, FALSE, 0 );

  g_signal_connect(cf->button, "clicked",
		   G_CALLBACK(yuiBrowse), (gpointer)cf );
  
  return cf;
}

static void cfDelete( ComboFileSelect* cf ) {

  free( cf->name ); /* widgets are destroyed by GTK, just remove our data */
  free( cf );
}

static void cfSetSensitive( ComboFileSelect* cf, gboolean bSensitive ) {

  gtk_widget_set_sensitive( cf->combo, bSensitive );
  gtk_widget_set_sensitive( cf->button, bSensitive );
}

static gint cfGetActive( ComboFileSelect* cf ) {

  return gtk_combo_box_get_active( GTK_COMBO_BOX(cf->combo) );
}

static void cfSetActive( ComboFileSelect* cf, gint i ) {

  gtk_combo_box_set_active( GTK_COMBO_BOX(cf->combo), i );
}

static GtkWidget* cfGetWidget( ComboFileSelect* cf ) { return cf->frame; }

static void cfGetText( ComboFileSelect* cf, gchar** str ) {

  GtkTreeIter iter;    

  if ( cfGetActive( cf ) < 1 ) *str = NULL;
  else {
    gtk_combo_box_get_active_iter( GTK_COMBO_BOX(cf->combo), &iter );
    gtk_tree_model_get( gtk_combo_box_get_model( GTK_COMBO_BOX(cf->combo)), &iter, 0, str, -1 );
  }
}

static gboolean cfCatchValue( ComboFileSelect* cf, gchar* value ) {

  gint i;
  gchar* c = "";
  if ( !value ) {
    cfSetActive( cf, 0 );
    return TRUE;
  }
  for ( i=1 ; i <= cf->idc ; i++ ) {
    cfSetActive( cf, i );
    cfGetText( cf, &c );
    if ( !strcmp(value, c ) ) return TRUE;
  }
  return FALSE;
}

static cfCatchOrInsertValue( ComboFileSelect* cf, gchar* value ) {

  if ( !cfCatchValue( cf, value )) {

    gint newid = ++(cf->idc);
    gtk_combo_box_insert_text( GTK_COMBO_BOX( cf->combo ), newid, value );
    saveCombo( GTK_COMBO_BOX(cf->combo), newid, cf->name );
    gtk_combo_box_set_active( GTK_COMBO_BOX(cf->combo), newid );    
  }
}

/* ---------------------------------------------------- */
/* Settings dialog                                      */

static struct {
  ComboFileSelect *cfBup, *cfCart, *cfMpeg;
  GtkWidget *comboCartType, *comboRegion, *spinX, *spinY, *checkAspect, *checkSound, *cbSh2, *cbVid;
  gboolean soundenabled;
} yuiSettings;

static void yuiSettingsResponse(GtkWidget *widget, gint arg1, gpointer user_data ) {
  /* settings dialog has exited. Recover all information. */

  if ( arg1 == GTK_RESPONSE_ACCEPT ) {
   
    gchar *c;
    int r;
    cfGetText( yuiSettings.cfBup, &c );
    yuiSetString( "buppath", c );
    cfGetText( yuiSettings.cfCart, &c );
    yuiSetString( "cartpath", c );
    cfGetText( yuiSettings.cfMpeg, &c );
    yuiSetString( "mpegpath", c );
    yuiSetInt( "carttype", gtk_combo_box_get_active( GTK_COMBO_BOX(yuiSettings.comboCartType)) );
    switch ( gtk_combo_box_get_active( GTK_COMBO_BOX(yuiSettings.comboRegion)) ) {
    case 0: r = REGION_AUTODETECT; break;
    case 1: r = REGION_JAPAN ; break;
    case 2: r = REGION_ASIANTSC ; break;
    case 3: r = REGION_NORTHAMERICA ; break;
    case 4: r = REGION_CENTRALSOUTHAMERICANTSC ; break;
    case 5: r = REGION_KOREA ; break;
    case 6: r = REGION_ASIAPAL ; break;
    case 7: r = REGION_EUROPE ; break;
    case 8: r = REGION_CENTRALSOUTHAMERICAPAL ; break;
    }
    yuiSetInt( "sh2core", gtk_combo_box_get_active( GTK_COMBO_BOX(yuiSettings.cbSh2) ));
    yuiSetInt( "vidcore", gtk_combo_box_get_active( GTK_COMBO_BOX(yuiSettings.cbVid) ));
    yuiSetInt( "region", r );
    yuiSetInt( "fsX", gtk_spin_button_get_value_as_int( GTK_SPIN_BUTTON( yuiSettings.spinX ) ) );
    yuiSetInt( "fsY", gtk_spin_button_get_value_as_int( GTK_SPIN_BUTTON( yuiSettings.spinY ) ) );
    yuiSetInt( "keepRatio", gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( yuiSettings.checkAspect ) ) );
    yuiSettings.soundenabled = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( yuiSettings.checkSound ) );
    if ( !forceSoundDisabled ) yuiSetInt( "soundenabled", yuiSettings.soundenabled );
    else if ( yuiSettings.soundenabled ) yuiSetInt( "soundenabled", yuiSettings.soundenabled );
    yuiStore();
  }
}

static void yuiComboCart( GtkWidget *widget, gpointer user_data ) {
  /* comboCartType changed */
  
  switch ( gtk_combo_box_get_active( GTK_COMBO_BOX(yuiSettings.comboCartType) ) ) {

  case CART_PAR:
  case CART_BACKUPRAM4MBIT:
  case CART_BACKUPRAM8MBIT:
  case CART_BACKUPRAM16MBIT:
  case CART_BACKUPRAM32MBIT:
  case CART_ROM16MBIT:
    cfSetSensitive( yuiSettings.cfCart, TRUE );
    break;
  default:
    cfSetSensitive( yuiSettings.cfCart, FALSE );   
  }
}

static void spinFsChanged( GtkWidget *widget, gboolean bFsY ) {
  /* Keep the 10/7 ratio between spinX and spinY */

  if ( ! gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( yuiSettings.checkAspect ) ) ) return;
  if ( bFsY ) gtk_spin_button_set_value( GTK_SPIN_BUTTON(yuiSettings.spinX),
					 (gtk_spin_button_get_value_as_int( GTK_SPIN_BUTTON(yuiSettings.spinY))*10)/7 );
  else gtk_spin_button_set_value( GTK_SPIN_BUTTON(yuiSettings.spinY),
					 (gtk_spin_button_get_value_as_int( GTK_SPIN_BUTTON(yuiSettings.spinX))*7)/10 );
}

static void yuiSettingsDialog() {
  /* create and run settings dialog. */

  int r,i;
  GtkWidget *hboxcart, *hboxregion, *fSh2, *fVid;
  GtkWidget* dialog = gtk_dialog_new_with_buttons ( "Settings",
						    NULL,
						   (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
						    GTK_STOCK_OK,
						    GTK_RESPONSE_ACCEPT,
						    GTK_STOCK_CANCEL,
						    GTK_RESPONSE_REJECT, NULL);
  GtkWidget* vbox = GTK_DIALOG(dialog)->vbox;

  /* sh2core combo */

  fSh2 = gtk_frame_new("Execution mode");
  gtk_box_pack_start( GTK_BOX( vbox ), fSh2, FALSE, FALSE, 4 );

  yuiSettings.cbSh2 = gtk_combo_box_new_text();
  gtk_container_add (GTK_CONTAINER( fSh2 ), yuiSettings.cbSh2 );
  for ( i = 0 ; SH2CoreList[i] ; i++ ) 
    gtk_combo_box_append_text( GTK_COMBO_BOX(yuiSettings.cbSh2), SH2CoreList[i]->Name );
  gtk_combo_box_set_active( GTK_COMBO_BOX(yuiSettings.cbSh2), yuiGetInt( "sh2core", SH2CORE_YUI_DEFAULT ) );

  /* Video Core selection */

  fVid = gtk_frame_new("Video Core");
  gtk_box_pack_start( GTK_BOX( vbox ), fVid, FALSE, FALSE, 4 );

  yuiSettings.cbVid = gtk_combo_box_new_text();
  gtk_container_add (GTK_CONTAINER( fVid ), yuiSettings.cbVid );
  for ( i = 0 ; VIDCoreList[i] ; i++ ) 
    gtk_combo_box_append_text( GTK_COMBO_BOX(yuiSettings.cbVid), VIDCoreList[i]->Name );
  gtk_combo_box_set_active( GTK_COMBO_BOX(yuiSettings.cbVid), yuiGetInt( "vidcore", VIDCORE_YUI_DEFAULT ) );

  /* Cartbridge path selection CF */

  yuiSettings.cfCart = cfNew( "cart", "Cartbridge slot" );
  cfCatchValue( yuiSettings.cfCart, yuiGetString( "cartpath" ) );
  gtk_box_pack_start( GTK_BOX( vbox ), cfGetWidget(yuiSettings.cfCart), FALSE, FALSE, 4 );

  /* Cartbridge type selection Combo */

  hboxcart = gtk_hbox_new( FALSE, 4 );
  gtk_box_pack_start( GTK_BOX( vbox ), hboxcart, FALSE, FALSE, 4 );  

  gtk_box_pack_start( GTK_BOX( hboxcart ), gtk_label_new( "Cartbridge type >" ), FALSE, FALSE, 2 );
  yuiSettings.comboCartType = gtk_combo_box_new_text();

  gtk_combo_box_insert_text( GTK_COMBO_BOX(yuiSettings.comboCartType), 0, "None" );
  gtk_combo_box_insert_text( GTK_COMBO_BOX(yuiSettings.comboCartType), 1, "Pro Action Replay" );
  gtk_combo_box_insert_text( GTK_COMBO_BOX(yuiSettings.comboCartType), 2, "4 Mbit Backup Ram" );
  gtk_combo_box_insert_text( GTK_COMBO_BOX(yuiSettings.comboCartType), 3, "8 Mbit Backup Ram" );
  gtk_combo_box_insert_text( GTK_COMBO_BOX(yuiSettings.comboCartType), 4, "16 Mbit Backup Ram" );
  gtk_combo_box_insert_text( GTK_COMBO_BOX(yuiSettings.comboCartType), 5, "32 Mbit Backup Ram" );
  gtk_combo_box_insert_text( GTK_COMBO_BOX(yuiSettings.comboCartType), 6, "8 Mbit Dram" );
  gtk_combo_box_insert_text( GTK_COMBO_BOX(yuiSettings.comboCartType), 7, "32 Mbit Dram" );
  gtk_combo_box_insert_text( GTK_COMBO_BOX(yuiSettings.comboCartType), 8, "Netlink" );
  gtk_combo_box_insert_text( GTK_COMBO_BOX(yuiSettings.comboCartType), 9, "16 Mbit ROM" );
  
  g_signal_connect (GTK_OBJECT (yuiSettings.comboCartType), "changed", G_CALLBACK (yuiComboCart), NULL );
  
  gtk_combo_box_set_active( GTK_COMBO_BOX(yuiSettings.comboCartType), yuiGetInt( "carttype", CARTTYPE_DEFAULT ));
  gtk_box_pack_start( GTK_BOX( hboxcart ), yuiSettings.comboCartType, FALSE, FALSE, 2 );

  /* Backup Memory path selection CF */

  yuiSettings.cfBup = cfNew( "bup", "Backup memory" ); 
  cfCatchValue( yuiSettings.cfBup, yuiGetString( "buppath" ) );  
  gtk_box_pack_start( GTK_BOX( vbox ), cfGetWidget(yuiSettings.cfBup), FALSE, FALSE, 4 );  

  /* Region selection Combo */

  hboxregion = gtk_hbox_new( FALSE, 4 );
  gtk_box_pack_start( GTK_BOX( vbox ), hboxregion, FALSE, FALSE, 4 );  

  gtk_box_pack_start( GTK_BOX( hboxregion ), gtk_label_new( "Region >" ), FALSE, FALSE, 2 );
  yuiSettings.comboRegion = gtk_combo_box_new_text();

  gtk_combo_box_insert_text( GTK_COMBO_BOX(yuiSettings.comboRegion), 0, "Auto-detect" );
  gtk_combo_box_insert_text( GTK_COMBO_BOX(yuiSettings.comboRegion), 1, "Japan(NTSC)" );
  gtk_combo_box_insert_text( GTK_COMBO_BOX(yuiSettings.comboRegion), 2, "Asia(NTCS)" );
  gtk_combo_box_insert_text( GTK_COMBO_BOX(yuiSettings.comboRegion), 3, "North America(NTSC)" );
  gtk_combo_box_insert_text( GTK_COMBO_BOX(yuiSettings.comboRegion), 4, "Central/South America(NTSC)" );
  gtk_combo_box_insert_text( GTK_COMBO_BOX(yuiSettings.comboRegion), 5, "Korea(NTSC)" );
  gtk_combo_box_insert_text( GTK_COMBO_BOX(yuiSettings.comboRegion), 6, "Asia(PAL)" );
  gtk_combo_box_insert_text( GTK_COMBO_BOX(yuiSettings.comboRegion), 7, "Europe + others(PAL)" );
  gtk_combo_box_insert_text( GTK_COMBO_BOX(yuiSettings.comboRegion), 8, "Central/South America(PAL)" );

  switch ( yuiGetInt( "region", 0 ) ) {
    case REGION_AUTODETECT: r = 0; break;
    case REGION_JAPAN: r=1; break;
    case REGION_ASIANTSC: r=2; break;
    case REGION_NORTHAMERICA: r=3; break;
    case REGION_CENTRALSOUTHAMERICANTSC: r=4; break;
    case REGION_KOREA: r=5; break;
    case REGION_ASIAPAL: r=6; break;
    case REGION_EUROPE: r=7; break;
    case REGION_CENTRALSOUTHAMERICAPAL: r=8; break;
  }
  gtk_combo_box_set_active( GTK_COMBO_BOX(yuiSettings.comboRegion), r );
  gtk_box_pack_start( GTK_BOX( hboxregion ), yuiSettings.comboRegion, FALSE, FALSE, 2 );

  /* MPEG ROM File selection CF */

  yuiSettings.cfMpeg = cfNew( "mpeg", "MPEG ROM File" ); 
  cfCatchValue( yuiSettings.cfMpeg, yuiGetString( "mpegpath" ) );  
  gtk_box_pack_start( GTK_BOX( vbox ), cfGetWidget(yuiSettings.cfMpeg), FALSE, FALSE, 4 );  

  /* Fullscreen resolution */

  { GtkWidget* hboxFs = gtk_hbox_new( FALSE, 4 );
  yuiSettings.spinX = gtk_spin_button_new_with_range( 40, 2048, 40 );
  yuiSettings.spinY = gtk_spin_button_new_with_range( 28, 2048, 28 );
  gtk_spin_button_set_value( GTK_SPIN_BUTTON(yuiSettings.spinX), yuiGetInt( "fsX", FS_X_DEFAULT ) );
  gtk_spin_button_set_value( GTK_SPIN_BUTTON(yuiSettings.spinY), yuiGetInt( "fsY", FS_Y_DEFAULT ) );
  g_signal_connect(GTK_OBJECT( yuiSettings.spinX ), "value-changed", G_CALLBACK( spinFsChanged ), (gpointer)0 );
  g_signal_connect(GTK_OBJECT( yuiSettings.spinY ), "value-changed", G_CALLBACK( spinFsChanged ), (gpointer)1 );

  gtk_box_pack_start( GTK_BOX( hboxFs ), gtk_label_new( "Fullscreen resolution >  X :" ), FALSE, FALSE, 2 );
  gtk_box_pack_start( GTK_BOX( hboxFs ), yuiSettings.spinX, FALSE, FALSE, 0 );
  gtk_box_pack_start( GTK_BOX( hboxFs ), gtk_label_new( " Y :" ), FALSE, FALSE, 2 );
  gtk_box_pack_start( GTK_BOX( hboxFs ), yuiSettings.spinY, FALSE, FALSE, 0 );

  gtk_box_pack_start( GTK_BOX( vbox ), hboxFs, FALSE, FALSE, 4 );
  }


  { GtkWidget* hbox = gtk_hbox_new( FALSE, 4 );

  gtk_box_pack_start( GTK_BOX( hbox ), gtk_hseparator_new(), TRUE, FALSE, 2 );
  yuiSettings.checkAspect = gtk_check_button_new_with_label("Keep aspect ratio");
  gtk_box_pack_start( GTK_BOX( hbox ), yuiSettings.checkAspect, FALSE, FALSE, 0 );
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( yuiSettings.checkAspect ), yuiGetInt( "keepRatio", 1 ) );

  gtk_box_pack_start( GTK_BOX( vbox ), hbox, FALSE, FALSE, 4 );
  }

  /* enable sound ? */

  { yuiSettings.checkSound = gtk_check_button_new_with_label("Sound enabled");
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( yuiSettings.checkSound ), yuiSettings.soundenabled );

  gtk_box_pack_start( GTK_BOX( vbox ), yuiSettings.checkSound, FALSE, FALSE, 4 );
  }

  /* ---------------------- */

  if ( yui.running != GTKYUI_WAIT ) {

    cfSetSensitive( yuiSettings.cfCart, FALSE );
    cfSetSensitive( yuiSettings.cfBup, FALSE );
    cfSetSensitive( yuiSettings.cfMpeg, FALSE );
    gtk_widget_set_sensitive( yuiSettings.comboCartType, 0 );
    gtk_widget_set_sensitive( yuiSettings.comboRegion, 0 );
    gtk_widget_set_sensitive( yuiSettings.cbSh2, 0 );
    gtk_widget_set_sensitive( yuiSettings.checkSound, 0 );
  }

  gtk_widget_show_all(dialog);
  g_signal_connect (GTK_OBJECT (dialog),
		    "response",
		    G_CALLBACK (yuiSettingsResponse), NULL);
  gtk_dialog_run( GTK_DIALOG( dialog ) );

  cfDelete( yuiSettings.cfCart );
  cfDelete( yuiSettings.cfBup );
  cfDelete( yuiSettings.cfMpeg );
  gtk_widget_destroy(dialog);
}

/* ---------------------------------------------------------- */
/* GUI Core                                                   */

static void yuiAbout(void);

static int GtkWorkaround(void) {
  if ( yui.running == GTKYUI_RUN ) return ~(PERCore->HandleEvents());
  return TRUE;
}

static void yuiPopup( gchar* text, GtkMessageType mType ) {
  
  GtkWidget* dialog = gtk_message_dialog_new (GTK_WINDOW(yui.window),
				   GTK_DIALOG_DESTROY_WITH_PARENT,
				   mType,
				   GTK_BUTTONS_CLOSE,
				   text );
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

static void yuiErrorPopup( gchar* text ) {
  
  yuiPopup( text, GTK_MESSAGE_ERROR );
}

static void yuiWarningPopup( gchar* text ) {
  
  yuiPopup( text, GTK_MESSAGE_WARNING );
}

static void yuiYabauseInit() {

    yabauseinit_struct yinit;
    gchar *c;

    if ( cfGetActive( yui.cfBios ) <= 0 ) {
      yuiErrorPopup("You need to select a BIOS file.");
      return;
    }
    
    if ( cfGetActive( yui.cfCdRom) <= 0 ) {
      yinit.cdpath = NULL;
      yinit.cdcoretype = CDCORE_DEFAULT; 
    }
    else {
      cfGetText( yui.cfCdRom, (gchar**)(&yinit.cdpath) );
      yinit.cdcoretype = ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( yui.checkIso ) ) )
	? CDCORE_ISO : CDCORE_ARCH;
    }
    yinit.percoretype = PERCORE_SDL;
    yinit.sh2coretype = SH2CoreList[yuiGetInt( "sh2core", SH2CORE_YUI_DEFAULT )]->id;
    printf( "%d   (sh2)\n", yinit.sh2coretype);
    yinit.vidcoretype = VIDCoreList[yuiGetInt( "vidcore", VIDCORE_YUI_DEFAULT )]->id;
    printf( "%d   (vidcore)\n",yinit.vidcoretype );
    yinit.sndcoretype = yuiSettings.soundenabled ? SNDCORE_SDL : SNDCORE_DUMMY;
    yinit.regionid = yuiGetInt( "region", REGION_AUTODETECT );
    cfGetText( yui.cfBios, (gchar**)(&yinit.biospath) );
    
    yinit.buppath = g_strdup(yuiGetString( "buppath" ));
    yinit.mpegpath = g_strdup(yuiGetString( "mpegpath" ));
    yinit.cartpath = g_strdup(yuiGetString( "cartpath" ));
    yinit.carttype = yuiGetInt( "carttype", CARTTYPE_DEFAULT );

    cfGetText( yui.cfBios, &c );
    yuiSetString( "lastrunbios", c );
    cfGetText( yui.cfCdRom, &c );
    yuiSetString( "lastruncdrom", c );
    yuiSetInt( "cdType", !gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( yui.checkIso ) ) );
    yuiStore();
     
    if (YabauseInit(&yinit) == -1) {
      yuiErrorPopup("Yabause initialization failed.");
      return;
    }
    
    cfSetSensitive( yui.cfBios, FALSE );
    cfSetSensitive( yui.cfCdRom, FALSE );
    gtk_widget_set_sensitive( yui.checkIso, FALSE );
    gtk_widget_set_sensitive( yui.checkCdRom, FALSE );
    gtk_widget_show( yui.buttonReset );

    yui.running = GTKYUI_PAUSE;
}

static void yuiRun(void) {

  switch ( yui.running ) {

  case GTKYUI_WAIT:
    yuiYabauseInit();
    if ( yui.running != GTKYUI_PAUSE ) return;
    /* fall through */
  case GTKYUI_PAUSE:
    gtk_widget_show( yui.buttonPause );
    gtk_widget_show( yui.buttonFs );
    gtk_widget_hide( yui.buttonRun );
    yui.running = GTKYUI_RUN;
    g_idle_add((gboolean (*)(void*)) GtkWorkaround, (gpointer)1 );
  }
}

static void debugUpdateViews();
static void yuiPause(void) {

  if ( yui.running == GTKYUI_RUN ) {

    yui.running = GTKYUI_PAUSE;
    gtk_widget_show( yui.buttonRun );
    gtk_widget_hide( yui.buttonPause );
    if (VIDCore->IsFullscreen())
      VIDCore->Resize( 320, 224, FALSE );
    g_idle_remove_by_data( (gpointer)1 );
    debugUpdateViews();
  }
}

static void yuiFs(void) {

  yuiRun();
  if ( yui.running == GTKYUI_RUN ) 
    VIDCore->Resize( yuiGetInt( "fsX", FS_X_DEFAULT  ), yuiGetInt( "fsY", FS_Y_DEFAULT ), TRUE );
}

#include "debug.c"

static int yuiInit(void) {
	int fake_argc = 0;
	char ** fake_argv = NULL;
	char * text;

	GtkWidget *vbox;
	GtkWidget *hboxHigh;
	GtkWidget *hboxLow;
	GtkWidget *vboxHigh;
	GtkWidget *hboxRadioCD;
	GtkWidget *buttonQuit;
	GtkWidget *buttonHelp;

	gtk_init (&fake_argc, &fake_argv);
	yui.running = GTKYUI_WAIT;
	yui.pixBufIcon = gdk_pixbuf_new_from_xpm_data((const char **)icon_xpm);
	yuiConfInit();

	if ( forceSoundDisabled ) yuiSettings.soundenabled = FALSE;
	else yuiSettings.soundenabled = yuiGetInt( "soundenabled", TRUE );

	yui.window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_resizable( GTK_WINDOW( yui.window ), FALSE );
	g_signal_connect(G_OBJECT (yui.window), "delete_event", GTK_SIGNAL_FUNC(YuiQuit), NULL);
	gtk_window_set_icon( GTK_WINDOW(yui.window), yui.pixBufIcon );

	vbox = gtk_vbox_new (FALSE, 2);
	gtk_container_set_border_width( GTK_CONTAINER( vbox ),4 );
	gtk_container_add (GTK_CONTAINER (yui.window), vbox);

	hboxHigh = gtk_hbox_new( FALSE, 4 );
	gtk_box_pack_start( GTK_BOX( vbox ), hboxHigh, TRUE, TRUE, 2 );
	gtk_box_pack_start( GTK_BOX( vbox ), gtk_hseparator_new(), TRUE, TRUE, 2 );

	hboxLow = gtk_hbox_new( FALSE, 2 );
	gtk_box_pack_start( GTK_BOX( vbox ), hboxLow, TRUE, TRUE, 2 );
	gtk_box_pack_start( GTK_BOX( vbox ), widgetDebug(), TRUE, TRUE, 2 );

	vboxHigh = gtk_vbox_new( FALSE, 5 );
	gtk_box_pack_start( GTK_BOX( hboxHigh ), vboxHigh, TRUE, TRUE, 2 );

	yui.cfBios = cfNew( "bios", "Bios" );
	yui.cfCdRom = cfNew( "cdrom", "CD-ROM" );
	cfCatchValue( yui.cfBios, yuiGetString( "lastrunbios" ) );  
	cfCatchValue( yui.cfCdRom, yuiGetString( "lastruncdrom" ) );  
	gtk_box_pack_start( GTK_BOX( vboxHigh ), cfGetWidget( yui.cfBios ), FALSE, TRUE, 4 );
	gtk_box_pack_start( GTK_BOX( vboxHigh ), cfGetWidget( yui.cfCdRom ), FALSE, TRUE, 4 );	

	if ( forceBiosFileName ) cfCatchOrInsertValue( yui.cfBios, forceBiosFileName );
	if ( forceCdFileName ) cfCatchOrInsertValue( yui.cfCdRom, forceCdFileName );

	hboxRadioCD = gtk_hbox_new( FALSE, 2 );
	gtk_box_pack_start( GTK_BOX( vboxHigh ), hboxRadioCD, FALSE, TRUE, 4 );

	yui.checkIso = gtk_radio_button_new_with_label( NULL, "ISO or CUE file" );
	yui.checkCdRom = gtk_radio_button_new_with_label_from_widget( GTK_RADIO_BUTTON(yui.checkIso), "Native CD" );
	gtk_box_pack_start( GTK_BOX( hboxRadioCD ), yui.checkIso, FALSE, TRUE, 4 );
	gtk_box_pack_start( GTK_BOX( hboxRadioCD ), yui.checkCdRom, FALSE, TRUE, 4 );

	if (( yuiGetInt( "cdType", 0 )&& !forceCdType )||( forceCdType == 2 ))
	  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(yui.checkCdRom), TRUE ); 

	yui.buttonRun = gtk_button_new_from_stock( GTK_STOCK_EXECUTE );
	gtk_box_pack_start( GTK_BOX( hboxLow ), yui.buttonRun, FALSE, FALSE, 2 );

	yui.buttonPause = gtk_button_new_with_label( "Pause" );
	gtk_box_pack_start( GTK_BOX( hboxLow ), yui.buttonPause, FALSE, FALSE, 2 );

	yui.buttonReset = gtk_button_new_with_label( "Reset" );
	gtk_box_pack_start( GTK_BOX( hboxLow ), yui.buttonReset, FALSE, FALSE, 2 );

	yui.buttonFs = gtk_button_new_with_label( "Full Screen" );
	gtk_box_pack_start( GTK_BOX( hboxLow ), yui.buttonFs, FALSE, FALSE, 2 );
 
	yui.buttonSettings = gtk_button_new_from_stock( GTK_STOCK_PREFERENCES );
	gtk_box_pack_start( GTK_BOX( hboxLow ), yui.buttonSettings, FALSE, FALSE, 2 );

	gtk_box_pack_start( GTK_BOX( hboxLow ), gtk_vseparator_new(), TRUE, FALSE, 2 );

	buttonQuit = gtk_button_new_from_stock( GTK_STOCK_QUIT );
	gtk_box_pack_start( GTK_BOX( hboxLow ), buttonQuit, FALSE, FALSE, 2 );

	buttonHelp = gtk_button_new_from_stock( GTK_STOCK_HELP );
	gtk_box_pack_start( GTK_BOX( hboxLow ), buttonHelp, FALSE, FALSE, 2 );

	text = g_strdup_printf ("Yabause %s", VERSION);
	gtk_window_set_title(GTK_WINDOW (yui.window), text);
	g_free(text);

	g_signal_connect(yui.buttonRun, "clicked",
			 G_CALLBACK(yuiRun), NULL);
	g_signal_connect(yui.buttonPause, "clicked",
			 G_CALLBACK(yuiPause), NULL);
	g_signal_connect(yui.buttonReset, "clicked",
			 G_CALLBACK(YabauseReset), NULL);
	g_signal_connect(yui.buttonFs, "clicked",
			 G_CALLBACK(yuiFs), NULL);
	g_signal_connect(yui.buttonSettings, "clicked",
			 G_CALLBACK(yuiSettingsDialog), NULL);
	g_signal_connect_swapped(buttonQuit, "clicked",
				 G_CALLBACK(YuiQuit), NULL );
	g_signal_connect(buttonHelp, "clicked",
			 G_CALLBACK(yuiAbout), NULL );

	gtk_widget_show_all (yui.window);
	gtk_widget_hide( yui.buttonReset );
	gtk_widget_hide( yui.buttonPause );
	gtk_widget_hide( yui.buttonFs );
}

/* ------------------------------------------------- */
/* Interface functions -- non-static !               */

void YuiSetSoundEnable(int enablesound) {
  
  if ( !enablesound ) forceSoundDisabled = TRUE;
}

void YuiVideoResize(unsigned int w, unsigned int h, int isfullscreen) {}

void YuiSetBiosFilename(const char * biosfilename) {

  forceBiosFileName = g_strdup(biosfilename);
}

void YuiSetIsoFilename(const char * isofilename) {

  forceCdFileName = g_strdup(isofilename);
  forceCdType = 1;
}

void YuiSetCdromFilename(const char * cdromfilename) {

  forceCdFileName = g_strdup(cdromfilename);
  forceCdType = 2;
}

void YuiHideShow(void) {
}

void YuiQuit(void) { gtk_main_quit(); }

int YuiInit(void) {

   yuiInit();
   gtk_main();
   return 0;
}

void YuiErrorMsg(const char *string) {

   fprintf(stderr, string);
}

/* ------------------------------------------------------------- */
/* About box                                                     */

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
    N_("Fabien Coulon"),
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
yuiGenerateCreditList(const gchar * text[], gboolean sec_space)
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

static void yuiAbout(void) {
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
    gtk_window_set_icon( GTK_WINDOW(about_window), yui.pixBufIcon );
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

    list = yuiGenerateCreditList(credit_text, TRUE);
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

void YuiSwapBuffers(void) {
	SDL_GL_SwapBuffers();
}

void YuiSetVideoAttribute(int type, int val) {
	if (SDL_WasInit(SDL_INIT_VIDEO) == 0)
		SDL_InitSubSystem(SDL_INIT_VIDEO);
	switch(type) {
		case RED_SIZE:
			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, val);
			break;
		case GREEN_SIZE:
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, val);
			break;
		case BLUE_SIZE:
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, val);
			break;
		case DEPTH_SIZE:
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, val);
			break;
		case DOUBLEBUFFER:
			SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
			break;
	}
}

int YuiSetVideoMode(int width, int height, int bpp, int fullscreen) {
	if (SDL_WasInit(SDL_INIT_VIDEO) == 0)
		SDL_InitSubSystem(SDL_INIT_VIDEO);
	return (SDL_SetVideoMode(width, height, bpp, SDL_OPENGL | SDL_RESIZABLE) == NULL);
}

int main(int argc, char *argv[]) {
#ifndef NO_CLI
   int i;
#endif
   LogStart();

#ifndef NO_CLI
   //handle command line arguments
   for (i = 1; i < argc; ++i) {
      if (argv[i]) {
         //show usage
         if (0 == strcmp(argv[i], "-h") || 0 == strcmp(argv[i], "-?") || 0 == strcmp(argv[i], "--help")) {
            print_usage(argv[0]);
            return 0;
         }
			
         //set bios
         if (0 == strcmp(argv[i], "-b") && argv[i + 1])
            YuiSetBiosFilename(argv[i + 1]);
         else if (strstr(argv[i], "--bios="))
            YuiSetBiosFilename(argv[i] + strlen("--bios="));
	
         //set iso
         else if (0 == strcmp(argv[i], "-i") && argv[i + 1])
            YuiSetIsoFilename(argv[i + 1]);
         else if (strstr(argv[i], "--iso="))
            YuiSetIsoFilename(argv[i] + strlen("--iso="));

         //set cdrom
         else if (0 == strcmp(argv[i], "-c") && argv[i + 1])
            YuiSetCdromFilename(argv[i + 1]);
         else if (strstr(argv[i], "--cdrom="))
            YuiSetCdromFilename(argv[i] + strlen("--cdrom="));

         // Set sound
         else if (strcmp(argv[i], "-ns") == 0 || strcmp(argv[i], "--nosound") == 0)
            YuiSetSoundEnable(0);
      }
   }
#endif

   if (YuiInit() != 0)
      fprintf(stderr, "Error running Yabause\n");

   YabauseDeInit();
   PROFILE_PRINT();
   LogStop();
   return 0;
}
