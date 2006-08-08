/*  Copyright 2005-2006 Fabien Coulon

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
#include <gdk/gdkkeysyms.h>

#include "yuish.h"
#include "../sh2core.h"
#include "../sh2d.h"
#include "../yabause.h"
#include "settings.h"

static void yui_sh_class_init	(YuiShClass * klass);
static void yui_sh_init		(YuiSh      * yfe);
static void yui_sh_clear(YuiSh * sh);
static void yui_sh_editedReg( GtkCellRendererText *cellrenderertext,
			      gchar *arg1,
			      gchar *arg2,
			      YuiSh *sh2);
static void yui_sh_editedBp( GtkCellRendererText *cellrenderertext,
			     gchar *arg1,
			     gchar *arg2,
			     YuiSh *sh2);
static void yui_sh_step( GtkWidget* widget, YuiSh * sh2 );
static void yui_sh_step_over( GtkWidget* widget, YuiSh * sh2 );
static void SH2BreakpointHandler (SH2_struct *context, u32 addr);

static YuiSh *yui_msh, *yui_ssh;
static YuiWindow * yui;

GType yui_sh_get_type (void) {
  static GType yfe_type = 0;

  if (!yfe_type)
    {
      static const GTypeInfo yfe_info =
      {
	sizeof (YuiShClass),
	NULL, /* base_init */
        NULL, /* base_finalize */
	(GClassInitFunc) yui_sh_class_init,
        NULL, /* class_finalize */
	NULL, /* class_data */
        sizeof (YuiSh),
	0,
	(GInstanceInitFunc) yui_sh_init,
      };

      yfe_type = g_type_register_static(GTK_TYPE_WINDOW, "YuiSh", &yfe_info, 0);
    }

  return yfe_type;
}

static void yui_sh_class_init (YuiShClass * klass) {
}

static void yui_sh_init (YuiSh * sh2) {
  gtk_window_set_title(GTK_WINDOW(sh2), "SH");
  gtk_window_set_resizable( GTK_WINDOW(sh2), FALSE );

  sh2->vbox = gtk_vbox_new(FALSE, 2);
  gtk_container_set_border_width( GTK_CONTAINER( sh2->vbox ),4 );
  gtk_container_add (GTK_CONTAINER (sh2), sh2->vbox);  

  sh2->hboxmain = gtk_hbox_new(FALSE, 2);
  gtk_container_set_border_width( GTK_CONTAINER( sh2->hboxmain ),4 );
  gtk_box_pack_start( GTK_BOX( sh2->vbox ), sh2->hboxmain, FALSE, FALSE, 4 );

  sh2->hbox = gtk_hbutton_box_new();
  gtk_container_set_border_width( GTK_CONTAINER( sh2->hbox ),4 );
  gtk_box_pack_start( GTK_BOX( sh2->vbox ), sh2->hbox, FALSE, FALSE, 4 ); 

  sh2->vboxmain = gtk_vbox_new(FALSE, 2);
  gtk_container_set_border_width( GTK_CONTAINER( sh2->vboxmain ),4 );
  gtk_box_pack_start( GTK_BOX( sh2->hboxmain ), sh2->vboxmain, FALSE, FALSE, 4 );

  /* unassembler frame */

  sh2->uFrame = gtk_frame_new("Disassembled code");
  gtk_box_pack_start( GTK_BOX( sh2->vboxmain ), sh2->uFrame, FALSE, FALSE, 4 );
  
  sh2->uLabel = gtk_label_new(NULL);
  gtk_container_add (GTK_CONTAINER (sh2->uFrame), sh2->uLabel );


  /* Register list */

  sh2->regListStore = gtk_list_store_new(2,G_TYPE_STRING,G_TYPE_STRING);
  sh2->regList = gtk_tree_view_new_with_model( GTK_TREE_MODEL(sh2->regListStore) );
  sh2->regListRenderer1 = gtk_cell_renderer_text_new();
  sh2->regListRenderer2 = gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(sh2->regListRenderer2), "editable", TRUE, "mode", GTK_CELL_RENDERER_MODE_EDITABLE, NULL );
  sh2->regListColumn1 = gtk_tree_view_column_new_with_attributes("Register", sh2->regListRenderer1, "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(sh2->regList), sh2->regListColumn1);
  sh2->regListColumn2 = gtk_tree_view_column_new_with_attributes("Value", sh2->regListRenderer2, "text", 1, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(sh2->regList), sh2->regListColumn2);
  gtk_box_pack_start( GTK_BOX( sh2->hboxmain ), sh2->regList, FALSE, FALSE, 4 );
  g_signal_connect(G_OBJECT(sh2->regListRenderer2), "edited", GTK_SIGNAL_FUNC(yui_sh_editedReg), sh2 );

  /* breakpoint list */

  sh2->bpListStore = gtk_list_store_new(1,G_TYPE_STRING);
  sh2->bpList = gtk_tree_view_new_with_model( GTK_TREE_MODEL(sh2->bpListStore) );
  sh2->bpListRenderer = gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(sh2->bpListRenderer), "editable", TRUE, "mode", GTK_CELL_RENDERER_MODE_EDITABLE, NULL );
  sh2->bpListColumn = gtk_tree_view_column_new_with_attributes("Breakpoints", sh2->bpListRenderer, "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(sh2->bpList), sh2->bpListColumn);
  gtk_box_pack_start( GTK_BOX( sh2->hboxmain ), sh2->bpList, FALSE, FALSE, 4 );
  g_signal_connect(G_OBJECT(sh2->bpListRenderer), "edited", GTK_SIGNAL_FUNC(yui_sh_editedBp), sh2 );

  g_signal_connect(G_OBJECT(sh2), "delete-event", GTK_SIGNAL_FUNC(yui_sh_destroy), NULL);
}


GtkWidget * yui_sh_new(YuiWindow * y, gboolean bMaster) {
  GtkWidget * dialog;
  GClosure *closureF7, *closureF8;
  GtkAccelGroup *accelGroup;
  codebreakpoint_struct *cbp;
  YuiSh * sh2;
  gint i;
  yui = y;

  if (!( yui->state & YUI_IS_INIT )) {
    yui_window_run(dialog, yui);
    yui_window_pause(dialog, yui);
  }

  if ( bMaster && yui_msh ) return GTK_WIDGET(yui_msh);
  if ( !bMaster && yui_ssh ) return GTK_WIDGET(yui_ssh);
  
  dialog = GTK_WIDGET(g_object_new(yui_sh_get_type(), NULL));
  sh2 = YUI_SH(dialog);

  sh2->breakpointEnabled = MSH2->breakpointEnabled; 
  if ( !sh2->breakpointEnabled )
    gtk_box_pack_start( GTK_BOX( sh2->vboxmain ), 
			gtk_label_new("Breakpoints are disabled (fast interpreter)"), FALSE, FALSE, 4 );

  sh2->bMaster = bMaster;
  sh2->debugsh = bMaster ? MSH2 : SSH2; 

  SH2SetBreakpointCallBack(sh2->debugsh, (void (*)(void *, u32))SH2BreakpointHandler);

  gtk_window_set_title(GTK_WINDOW(sh2), bMaster?"Master SH2":"Slave SH2");  

  for (i = 0; i < 23 ; i++) {
    
    GtkTreeIter iter;
    gtk_list_store_append( GTK_LIST_STORE( sh2->regListStore ), &iter );
  }	
  
  cbp = SH2GetBreakpointList(bMaster?MSH2:SSH2);
  
  for (i = 0; i < MAX_BREAKPOINTS-1; i++) {
    
    GtkTreeIter iter;
    sh2->cbp[i] = cbp[i].addr;
    gtk_list_store_append( GTK_LIST_STORE( sh2->bpListStore ), &iter );
    if (cbp[i].addr != 0xFFFFFFFF) {
      
      gchar tempstr[20];
      sprintf(tempstr, "%08X", (int)cbp[i].addr);
      gtk_list_store_set( GTK_LIST_STORE( sh2->bpListStore ), &iter, 0, tempstr, -1 );
    } else gtk_list_store_set( GTK_LIST_STORE( sh2->bpListStore ), &iter, 0, "<empty>", -1 );
  } 

  {
    GtkWidget * but2, * but3, * but4;
    
    sh2->buttonStep = gtk_button_new_with_label( "Step [F7]" );
    gtk_box_pack_start( GTK_BOX( sh2->hbox ), sh2->buttonStep, FALSE, FALSE, 2 );
    g_signal_connect( sh2->buttonStep, "clicked", G_CALLBACK(yui_sh_step), sh2 );
    
    sh2->buttonStepOver = gtk_button_new_with_label( "Step over [F8]" );
    gtk_box_pack_start( GTK_BOX( sh2->hbox ), sh2->buttonStepOver, FALSE, FALSE, 2 );
    g_signal_connect( sh2->buttonStepOver, "clicked", G_CALLBACK(yui_sh_step_over), sh2 );
    
    but2 = gtk_button_new();
    gtk_action_connect_proxy(gtk_action_group_get_action(yui->action_group, "run"), but2);
    gtk_box_pack_start(GTK_BOX(sh2->hbox), but2, FALSE, FALSE, 2);
    
    but3 = gtk_button_new();
    gtk_action_connect_proxy(gtk_action_group_get_action(yui->action_group, "pause"), but3);
    gtk_box_pack_start(GTK_BOX(sh2->hbox), but3, FALSE, FALSE, 2);
    
    but4 = gtk_button_new_from_stock("gtk-close");
    g_signal_connect_swapped(but4, "clicked", G_CALLBACK(yui_sh_destroy), dialog);
    gtk_box_pack_start(GTK_BOX(sh2->hbox), but4, FALSE, FALSE, 2);
  }
  sh2->paused_handler = g_signal_connect_swapped(yui, "paused", G_CALLBACK(yui_sh_update), sh2);
  sh2->running_handler = g_signal_connect_swapped(yui, "running", G_CALLBACK(yui_sh_clear), sh2);
  accelGroup = gtk_accel_group_new ();
  closureF7 = g_cclosure_new (G_CALLBACK (yui_sh_step), sh2, NULL);
  closureF8 = g_cclosure_new (G_CALLBACK (yui_sh_step_over), sh2, NULL);
  gtk_accel_group_connect( accelGroup, GDK_F7, 0, 0, closureF7 );
  gtk_accel_group_connect( accelGroup, GDK_F8, 0, 0, closureF8 );
  gtk_window_add_accel_group( GTK_WINDOW( dialog ), accelGroup );
  
  yui_sh_update(sh2);
  if ( yui->state & YUI_IS_RUNNING ) yui_sh_clear(sh2);
  
  gtk_widget_show_all(GTK_WIDGET(sh2));
  if ( !sh2->breakpointEnabled ) {
    gtk_widget_hide( sh2->bpList );
    gtk_widget_hide( sh2->buttonStepOver );
  }
  
  return dialog;
}

GtkWidget * yui_msh_new(YuiWindow * y) { 
  return GTK_WIDGET( yui_msh = YUI_SH(yui_sh_new( y, TRUE )) );
}

GtkWidget * yui_ssh_new(YuiWindow * y) { 
  return GTK_WIDGET( yui_ssh = YUI_SH(yui_sh_new( y, FALSE )) );
}



static void yuiUpcase( gchar* str ) {

  for ( ; *str ; str++ ) if (( *str >= 'a' )&&( *str <= 'z' )) *str += 'A'-'a';
}

static void SH2UpdateRegList( YuiSh *sh2, sh2regs_struct *regs) {
  /* refresh the registery list */

  GtkTreeIter iter;
  char regstr[32];
  char valuestr[32];
  int i;
  
  for (i = 0; i < 16; i++) {
    sprintf(regstr, "R%02d", i);
    sprintf(valuestr, "%08x", (int)regs->R[i] );
    yuiUpcase(valuestr);
    if ( !i ) gtk_tree_model_get_iter_first( GTK_TREE_MODEL( sh2->regListStore ), &iter );
    else gtk_tree_model_iter_next( GTK_TREE_MODEL( sh2->regListStore ), &iter );
    gtk_list_store_set( GTK_LIST_STORE( sh2->regListStore ), &iter, 0, regstr, 1, valuestr, -1 );
  }
  
  #define SH2UPDATEREGLIST(rreg) \
  gtk_tree_model_iter_next( GTK_TREE_MODEL( sh2->regListStore ), &iter ); \
  sprintf(valuestr, "%08x", (int)regs->rreg); \
  yuiUpcase(valuestr); \
  gtk_list_store_set( GTK_LIST_STORE( sh2->regListStore ), &iter, 0, #rreg, 1, valuestr, -1 );
  
  SH2UPDATEREGLIST(SR.all);
  SH2UPDATEREGLIST(GBR);
  SH2UPDATEREGLIST(VBR);
  SH2UPDATEREGLIST(MACH);
  SH2UPDATEREGLIST(MACL);
  SH2UPDATEREGLIST(PR);
  SH2UPDATEREGLIST(PC);
}

static void sh2setRegister( YuiSh *sh2, int nReg, u32 value ) {
  /* set register number <nReg> to value <value> in proc <sh2> */

  sh2regs_struct sh2regs;
  SH2GetRegisters(sh2->debugsh, &sh2regs);

  if ( nReg < 16 ) sh2regs.R[nReg] = value;
  switch ( nReg ) {
  case 16: sh2regs.SR.all = value; break;
  case 17: sh2regs.GBR = value; break;
  case 18: sh2regs.VBR = value; break;
  case 19: sh2regs.MACH = value; break;
  case 20: sh2regs.MACL = value; break;
  case 21: sh2regs.PR = value; break;
  case 22: sh2regs.PC = value; break;
  }

  SH2SetRegisters(sh2->debugsh, &sh2regs);
}

static void SH2UpdateCodeList( YuiSh *sh2, u32 addr) {
  /* refresh the assembler view. <addr> points the line to be highlighted. */

  int i;
  static char tagPC[] = "<span foreground=\"red\">";
  static char tagEnd[] = "</span>\n";
  char buf[64*24+40];
  char *curs = buf;
  char lineBuf[64];
  u32 offset;

  if ( addr - sh2->lastCode >= 20*2 ) offset = addr - (8*2);
  else offset = sh2->lastCode;
  sh2->lastCode = offset;

  for (i=0; i < 24; i++) {

    if ( offset + 2*i == addr ) { strcpy( curs, tagPC ); curs += strlen(tagPC); }
    SH2Disasm(offset+2*i, MappedMemoryReadWord(offset+2*i), 0, lineBuf);
    strcpy( curs, lineBuf );
    curs += strlen(lineBuf);
    if ( offset + 2*i == addr ) { strcpy( curs, tagEnd ); curs += strlen(tagEnd); }
    else { strcpy( curs, "\n" ); curs += 1;}
  }
  *curs = 0;
  gtk_label_set_markup( GTK_LABEL(sh2->uLabel), buf );
}

static void yui_sh_step( GtkWidget* widget, YuiSh * sh2 ) {

  SH2Step(sh2->debugsh);
  yui_window_invalidate( widget, yui ); /* update all dialogs, including us */
}

static void yui_sh_step_over( GtkWidget* widget, YuiSh* sh2 ) {

  sh2regs_struct sh2regs;

  SH2GetRegisters(sh2->debugsh, &sh2regs);

  if ( sh2->cbp[MAX_BREAKPOINTS-1] != 0xFFFFFFFF) SH2DelCodeBreakpoint(sh2->debugsh, sh2->cbp[MAX_BREAKPOINTS-1]);
  if (SH2AddCodeBreakpoint(sh2->debugsh, sh2regs.PC+2) == 0) {

    sh2->cbp[MAX_BREAKPOINTS-1] = sh2regs.PC+2;
    yui_window_run(widget, yui); /* pray we catch the breakpoint */
  } else {
    sh2->cbp[MAX_BREAKPOINTS-1] = 0xFFFFFFFF;
    yui_popup( yui, "Cannot step over: sh2 refuses to register a new breakpoint.", GTK_MESSAGE_ERROR );
  }
}

static void yui_sh_editedReg( GtkCellRendererText *cellrenderertext,
			      gchar *arg1,
			      gchar *arg2,
			      YuiSh *sh2) {
  /* registry number <arg1> value has been set to <arg2> */

  GtkTreeIter iter;
  char bptext[10];
  char *endptr;
  int i = atoi(arg1);
  u32 addr;

  gtk_tree_model_get_iter_from_string( GTK_TREE_MODEL( sh2->regListStore ), &iter, arg1 );
  addr = strtol(arg2, &endptr, 16 );
  if ( endptr - arg2 == strlen(arg2) ) {
   
    sprintf(bptext, "%08X", (int)addr);
    sh2setRegister( sh2, i, addr );
    gtk_list_store_set( GTK_LIST_STORE( sh2->regListStore ), &iter, 1, bptext, -1 );
  }
  yui_window_invalidate( NULL, yui );
}

static void yui_sh_editedBp( GtkCellRendererText *cellrenderertext,
			     gchar *arg1,
			     gchar *arg2,
			     YuiSh *sh2) {
  /* breakpoint <arg1> has been set to address <arg2> */

  GtkTreeIter iter;
  char bptext[10];
  char *endptr;
  int i = atoi(arg1);
  u32 addr;
  gtk_tree_model_get_iter_from_string( GTK_TREE_MODEL( sh2->bpListStore ), &iter, arg1 );
  addr = strtol(arg2, &endptr, 16 );
  if ((endptr - arg2 < strlen(arg2)) || (!addr)) addr = 0xFFFFFFFF;
  if ( sh2->cbp[i] != 0xFFFFFFFF) SH2DelCodeBreakpoint(sh2->debugsh, sh2->cbp[i]);
  sh2->cbp[i] = 0xFFFFFFFF;

  if ((addr!=0xFFFFFFFF)&&(SH2AddCodeBreakpoint(sh2->debugsh, addr) == 0)) {
   
    sprintf(bptext, "%08X", (int)addr);
    sh2->cbp[i] = addr;
  } else strcpy(bptext,"<empty>");
  gtk_list_store_set( GTK_LIST_STORE( sh2->bpListStore ), &iter, 0, bptext, -1 );
}

static void debugPauseLoop() { /* secondary gtk event loop for the "breakpoint pause" state */

  while ( !(yui->state & YUI_IS_RUNNING) )
    if ( gtk_main_iteration() ) return;
}

static void SH2BreakpointHandler (SH2_struct *context, u32 addr) {

  yui_window_pause(NULL, yui);
  {
    sh2regs_struct sh2regs;
    YuiSh* sh2 = YUI_SH(yui_sh_new( yui, context == MSH2 ));
    
    SH2GetRegisters(sh2->debugsh, &sh2regs);
    SH2UpdateRegList(sh2, &sh2regs);
    SH2UpdateCodeList(sh2, sh2regs.PC);  
    if ( sh2regs.PC == sh2->cbp[MAX_BREAKPOINTS-1] ) { /* special step over breakpoint */
      
      SH2DelCodeBreakpoint(sh2->debugsh, sh2->cbp[MAX_BREAKPOINTS-1]);
      sh2->cbp[MAX_BREAKPOINTS-1] = 0xFFFFFFFF;
    }
  }
  debugPauseLoop(); /* execution is suspended inside a normal cycle - enter secondary gtk loop */
}


void yui_sh_update(YuiSh * sh) {
  sh2regs_struct sh2regs;
  SH2GetRegisters(sh->debugsh, &sh2regs);
  SH2UpdateCodeList(sh,sh2regs.PC);
  SH2UpdateRegList(sh, &sh2regs);	
  gtk_widget_set_sensitive(sh->uLabel, TRUE);
  gtk_widget_set_sensitive(sh->bpList, TRUE);
  gtk_widget_set_sensitive(sh->regList, TRUE);
  gtk_widget_set_sensitive(sh->buttonStepOver, TRUE);
  gtk_widget_set_sensitive(sh->buttonStep, 
			   !sh->debugsh->isIdle && !(( sh->debugsh == SSH2 )&&( !yabsys.IsSSH2Running )));
}

void yui_sh_destroy(YuiSh * sh) {
  g_signal_handler_disconnect(yui, sh->running_handler);
  g_signal_handler_disconnect(yui, sh->paused_handler);
  
  if ( sh->bMaster ) yui_msh = NULL;
  else yui_ssh = NULL;

  gtk_widget_destroy(GTK_WIDGET(sh));
}

static void yui_sh_clear(YuiSh * sh) {
  
  gtk_widget_set_sensitive(sh->uLabel, FALSE);
  gtk_widget_set_sensitive(sh->bpList, FALSE);
  gtk_widget_set_sensitive(sh->regList, FALSE);
  gtk_widget_set_sensitive(sh->buttonStepOver, FALSE);
  gtk_widget_set_sensitive(sh->buttonStep, FALSE);
}
