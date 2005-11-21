/*  Copyright 2005 Fabien Coulon

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

typedef struct _memDumpDlg {

  GtkWidget *dialog, *list, *entryAddress;
  GtkListStore *listStore;
  u32 offset; /* the position we are dumping */
  gint nLines, wLine; /* number of lines, width of a line */
  struct _memDumpDlg *pred, *succ; /* they are douly-linked chained */
} memDumpDlg;

typedef struct { 
  GtkWidget *dialog, *bpList, *regList, *uLabel;
  GtkListStore *bpListStore, *regListStore;
  u32 cbp[MAX_BREAKPOINTS]; /* the list of breakpoint positions, as they can be found in the list widget */
  u32 lastCode; /* offset of last unassembly. Try to reuse it to prevent sliding. */
  SH2_struct *debugsh;
} SH2procDlg;

static SH2procDlg SSH2procDlg;
static SH2procDlg MSH2procDlg;

static u32 memDumpLastOffset = 0; /* last offset set to a memDump dialog. To be reused in new dialogs. */
static memDumpDlg *dumpDlgs = NULL; /* chain of dump dialogs */

static gboolean openedSH2[2] = {FALSE,FALSE};
static gboolean openedVDP1 = FALSE;
static gboolean openedVDP2 = FALSE;
static gboolean openedM68K = FALSE;
static gboolean openedSCUDSP = FALSE;

static void debugUpdateViews();

static void debugPauseLoop() { /* secondary gtk event loop for the "breakpoint pause" state */

  while ( yui.running != GTKYUI_RUN )
    if ( gtk_main_iteration() ) return;
}

static void yuiUpcase( gchar* str ) {

  for ( ; *str ; str++ ) if (( *str >= 'a' )&&( *str <= 'z' )) *str += 'A'-'a';
}

static gint hexaDigitToInt( gchar c ) {

  if (( c >= '0' )&&( c <= '9' )) return c-'0';
  if (( c >= 'a' )&&( c <= 'f' )) return c-'a' + 0xA;
  if (( c >= 'A' )&&( c <= 'F' )) return c-'A' + 0xA;
  return -1;
}

/* ------------------------------------------------------------------------------- */
/* Memory Dump Dialogs ----------------------------------------------------------- */

static memDumpDlg* memDumpNew() {

  memDumpDlg *dumpDlg = (memDumpDlg*)malloc(sizeof(memDumpDlg));
  dumpDlg->succ = dumpDlgs;
  dumpDlg->pred = NULL;
  if ( dumpDlgs ) dumpDlgs->pred = dumpDlg;
  dumpDlgs = dumpDlg;
  return dumpDlg;
}

static void memDumpDelete( memDumpDlg* dumpDlg ) {

  if ( dumpDlg->pred ) dumpDlg->pred->succ = dumpDlg->succ;
  else dumpDlgs = dumpDlg->succ;
  if ( dumpDlg->succ ) dumpDlg->succ->pred = dumpDlg->pred;
  free( dumpDlg );
}

static void memDumpUpdate( memDumpDlg* memDump ) {

  gint i,j;
  GtkTreeIter iter;
  u32 memCurs = memDump->offset;
  gchar dumpstr[64*4+10];
  gchar addrstr[10];
  gchar *dumpcurs = dumpstr;

  for ( i = 0 ; i < memDump->nLines ; i++ ) {

    sprintf( addrstr, "%08X", memCurs );
    for ( j = 0 ; j < memDump->wLine ; j++ ) {

      sprintf( dumpcurs, "%02X ", MappedMemoryReadByte( memCurs++ ) );
      dumpcurs += strlen(dumpcurs);
    }
    if ( !i ) gtk_tree_model_get_iter_first( GTK_TREE_MODEL( memDump->listStore ), &iter );
    else gtk_tree_model_iter_next( GTK_TREE_MODEL( memDump->listStore ), &iter );
    gtk_list_store_set( GTK_LIST_STORE( memDump->listStore ), &iter, 0, addrstr, 1, dumpstr, -1 );    
    dumpcurs = dumpstr;
  }

  /* Update the address bar */

  sprintf( addrstr, "%08X", memDump->offset );
  gtk_entry_set_text( GTK_ENTRY(memDump->entryAddress), addrstr );
}

static void memDumpUpdateAll() {

  memDumpDlg *memDump;
  for ( memDump = dumpDlgs ; memDump ; memDump = memDump->succ ) memDumpUpdate( memDump );
}

static void editedAddressDump( GtkWidget *widget, memDumpDlg *memDump ) {

  const gchar *stradr = gtk_entry_get_text(GTK_ENTRY(memDump->entryAddress));
  gchar *endptr;
  u32 addr;

  addr = strtol( stradr, &endptr, 16 );
  if (endptr - stradr < strlen(stradr)) return;
  if ( memDump->offset == addr ) return;
  memDump->offset = addr;
  memDumpUpdate( memDump );
  memDumpLastOffset = addr;
}

static void editedDump( GtkCellRendererText *cellrenderertext,
		      gchar *arg1,
		      gchar *arg2,
		      memDumpDlg *memDump) {

  GtkTreeIter iter;
  gint i = atoi(arg1);
  gint j,k;
  gchar *curs;
  u32 addr = memDump->offset + i*memDump->wLine;

  gtk_tree_model_get_iter_from_string( GTK_TREE_MODEL( memDump->listStore ), &iter, arg1 );
  
  /* check the format : wLine*2 hexa digits */
  for ( curs = arg2, j=0 ; *curs ; curs++ )
    if ( hexaDigitToInt( *curs ) != -1 ) j++;

  if ( j != memDump->wLine * 2 ) return;

  /* convert */
  for ( curs = arg2, k=-1 ; *curs ; curs++ ) { 

    if ( hexaDigitToInt( *curs )!=-1 ) {

      if ( k==-1 ) k = hexaDigitToInt( *curs );
      else { MappedMemoryWriteByte( addr++, 16*k + hexaDigitToInt( *curs ) ); k = -1;
      }
    }
  }
  memDumpUpdate( memDump );
}

static gboolean memDumpDeleteCallback(GtkWidget *widget,
				      GdkEvent *event,
				      memDumpDlg *memDump) {

  memDumpDelete( memDump );
  return FALSE;
}

static void memDumpPageUpCallback(GtkWidget *w, gpointer func, gpointer data, gpointer data2, memDumpDlg *memDump) {

  memDump->offset -= 2*memDump->wLine;
  memDumpLastOffset = memDump->offset;
  memDumpUpdate( memDump );
}

static void memDumpPageDownCallback(GtkWidget *w, gpointer func, gpointer data, gpointer data2, memDumpDlg *memDump) {

  memDump->offset += 2*memDump->wLine;
  memDumpLastOffset = memDump->offset;
  memDumpUpdate( memDump );
}

static memDumpDlg* openMemDump() {

  GtkWidget *vbox;
  GtkAccelGroup *accelGroup;
  GtkCellRenderer *listRenderer, *listRendererAddress;
  GtkTreeViewColumn *listColumn, *listColumnAddress;
  memDumpDlg *memDump =   memDumpNew();
  gint i;

  /* Initialization */

  memDump->offset = memDumpLastOffset;
  memDump->nLines = 6;
  memDump->wLine = 8;
  if ( yui.running == GTKYUI_WAIT ) yuiYabauseInit(); /* force yabause initialization */
  
  /* Dialog window */

  memDump->dialog = gtk_window_new( GTK_WINDOW_TOPLEVEL );
  gtk_window_set_title( GTK_WINDOW(memDump->dialog), "Memory Dump" );
  gtk_window_set_resizable( GTK_WINDOW(memDump->dialog), FALSE );

  vbox = gtk_vbox_new(FALSE, 2);
  gtk_container_set_border_width( GTK_CONTAINER( vbox ),4 );
  gtk_container_add (GTK_CONTAINER (memDump->dialog), vbox);  

  /* The address bar */

  memDump->entryAddress = gtk_entry_new_with_max_length( 10 );
  gtk_box_pack_start( GTK_BOX( vbox ), memDump->entryAddress, FALSE, FALSE, 4 );
  g_signal_connect(G_OBJECT(memDump->entryAddress), "activate", GTK_SIGNAL_FUNC(editedAddressDump), memDump );

  /* The dump area */

  memDump->listStore = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING );
  memDump->list = gtk_tree_view_new_with_model( GTK_TREE_MODEL(memDump->listStore) );
  listRenderer = gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(listRenderer), "editable", TRUE, "mode", GTK_CELL_RENDERER_MODE_EDITABLE, NULL );
  listRendererAddress = gtk_cell_renderer_text_new();

  listColumnAddress = gtk_tree_view_column_new_with_attributes("Offset", listRendererAddress, "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(memDump->list), listColumnAddress);
  listColumn = gtk_tree_view_column_new_with_attributes("Dump", listRenderer, "text", 1, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(memDump->list), listColumn);

  gtk_box_pack_start( GTK_BOX( vbox ), memDump->list, FALSE, FALSE, 4 );
  g_signal_connect(G_OBJECT(listRenderer), "edited", GTK_SIGNAL_FUNC(editedDump), memDump );

  for (i = 0; i < memDump->nLines ; i++) {

    GtkTreeIter iter;
    gtk_list_store_append( GTK_LIST_STORE( memDump->listStore ), &iter );
  }
  memDumpUpdate(memDump);

  accelGroup = gtk_accel_group_new ();
  gtk_accel_group_connect( accelGroup, GDK_Page_Up, 0, 0, g_cclosure_new (G_CALLBACK(memDumpPageUpCallback), memDump, NULL) );
  gtk_accel_group_connect( accelGroup, GDK_Page_Down, 0, 0, g_cclosure_new (G_CALLBACK(memDumpPageDownCallback), memDump, NULL) );
  gtk_window_add_accel_group( GTK_WINDOW( memDump->dialog ), accelGroup );

  g_signal_connect(G_OBJECT(memDump->dialog), "delete-event", GTK_SIGNAL_FUNC(memDumpDeleteCallback), memDump );
  gtk_widget_show_all( memDump->dialog );
  gtk_window_activate_focus( GTK_WINDOW(memDump->dialog) );
  return memDump;
}

/* ------------------------------------------------------------------------------- */
/* SH2 Dialogs ------------------------------------------------------------------- */

static SH2procDlg* openSH2( GtkWidget* widget, gpointer bMaster );

static void SH2UpdateRegList( SH2procDlg *sh2, sh2regs_struct *regs) {
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

static void sh2setRegister( SH2procDlg *sh2, int nReg, u32 value ) {
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

static void SH2UpdateCodeList( SH2procDlg *sh2, u32 addr) {
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

  gtk_label_set_markup( GTK_LABEL(sh2->uLabel), buf );
}

static void sh2step( GtkWidget* widget, SH2procDlg* sh2 ) {

  SH2Step(sh2->debugsh);
  debugUpdateViews(); /* update all dialogs, including us */
}

static void sh2stepMaster() { sh2step( NULL, &MSH2procDlg ); }
static void sh2stepSlave() { sh2step( NULL, &SSH2procDlg ); }

static void editedReg( GtkCellRendererText *cellrenderertext,
		      gchar *arg1,
		      gchar *arg2,
		      SH2procDlg *sh2) {

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
}

static void editedBp( GtkCellRendererText *cellrenderertext,
		      gchar *arg1,
		      gchar *arg2,
		      SH2procDlg *sh2) {

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

static void SH2BreakpointHandler (SH2_struct *context, u32 addr) {

  yuiPause();
  {
    sh2regs_struct sh2regs;
    SH2procDlg* sh2 = openSH2( NULL, (gpointer)(context == MSH2) );
    
    SH2GetRegisters(sh2->debugsh, &sh2regs);
    SH2UpdateRegList(sh2, &sh2regs);
    SH2UpdateCodeList(sh2, sh2regs.PC);  
  }
  debugPauseLoop();
}

static void sh2update( gboolean bMaster ) {

  sh2regs_struct sh2regs;
  SH2procDlg *sh2 = bMaster ? &MSH2procDlg : &SSH2procDlg;
  SH2GetRegisters(sh2->debugsh, &sh2regs);
  SH2UpdateCodeList(sh2,sh2regs.PC);
  SH2UpdateRegList(sh2, &sh2regs);
}

static SH2procDlg* openSH2( GtkWidget* widget, gpointer bMaster ) {

  GtkWidget *hbox, *vbox, *uFrame, *buttonStep, *buttonRun, *hboxButtons;
  GClosure *closureF9, *closureF7;
  GtkAccelGroup *accelGroup;
  GtkCellRenderer *bpListRenderer, *regListRenderer1, *regListRenderer2;
  GtkTreeViewColumn *bpListColumn, *regListColumn1, *regListColumn2;
  codebreakpoint_struct *cbp;
  sh2regs_struct sh2regs;
  char tempstr[10];
  int i;

  /* Initialization */

  SH2procDlg *sh2 = bMaster ? &MSH2procDlg : &SSH2procDlg;
  sh2->lastCode = 0;
  if ( openedSH2[(int)bMaster] ) {

    gtk_widget_show_all( sh2->dialog );
    gtk_window_activate_focus( GTK_WINDOW(sh2->dialog) );
    return sh2;
  }
  openedSH2[(int)bMaster] = TRUE;
  if ( yui.running == GTKYUI_WAIT ) yuiYabauseInit(); /* force yabause initialization */
  sh2->debugsh = bMaster ? MSH2 : SSH2; /* so that proc structures exist */  
  SH2SetBreakpointCallBack(sh2->debugsh, (void (*)(void *, u32))SH2BreakpointHandler);
  SH2GetRegisters(sh2->debugsh, &sh2regs);

  /* Dialog window */

  sh2->dialog = gtk_window_new( GTK_WINDOW_TOPLEVEL );
  gtk_window_set_title( GTK_WINDOW(sh2->dialog), bMaster ? "Master SH2" : "Slave SH2" );
  gtk_window_set_resizable( GTK_WINDOW(sh2->dialog), FALSE );

  hbox = gtk_hbox_new(FALSE, 2);
  gtk_container_set_border_width( GTK_CONTAINER( hbox ),4 );
  gtk_container_add (GTK_CONTAINER (sh2->dialog), hbox);  

  vbox = gtk_vbox_new(FALSE, 2);
  gtk_container_set_border_width( GTK_CONTAINER( vbox ),4 );
  gtk_box_pack_start( GTK_BOX( hbox ), vbox, FALSE, FALSE, 4 );

  /* unassembler frame */

  uFrame = gtk_frame_new("Assembler code");
  gtk_box_pack_start( GTK_BOX( vbox ), uFrame, FALSE, FALSE, 4 );
  
  sh2->uLabel = gtk_label_new(NULL);
  gtk_container_add (GTK_CONTAINER (uFrame), sh2->uLabel );
  SH2UpdateCodeList(sh2,sh2regs.PC);

  /* Register list */

  sh2->regListStore = gtk_list_store_new(2,G_TYPE_STRING,G_TYPE_STRING);
  sh2->regList = gtk_tree_view_new_with_model( GTK_TREE_MODEL(sh2->regListStore) );
  regListRenderer1 = gtk_cell_renderer_text_new();
  regListRenderer2 = gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(regListRenderer2), "editable", TRUE, "mode", GTK_CELL_RENDERER_MODE_EDITABLE, NULL );
  regListColumn1 = gtk_tree_view_column_new_with_attributes("Register", regListRenderer1, "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(sh2->regList), regListColumn1);
  regListColumn2 = gtk_tree_view_column_new_with_attributes("Value", regListRenderer2, "text", 1, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(sh2->regList), regListColumn2);
  gtk_box_pack_start( GTK_BOX( hbox ), sh2->regList, FALSE, FALSE, 4 );
  g_signal_connect(G_OBJECT(regListRenderer2), "edited", GTK_SIGNAL_FUNC(editedReg), sh2 );

  for (i = 0; i < 23 ; i++) {

    GtkTreeIter iter;
    gtk_list_store_append( GTK_LIST_STORE( sh2->regListStore ), &iter );
  }
  SH2UpdateRegList(sh2, &sh2regs);

  /* breakpoint list */

  sh2->bpListStore = gtk_list_store_new(1,G_TYPE_STRING);
  sh2->bpList = gtk_tree_view_new_with_model( GTK_TREE_MODEL(sh2->bpListStore) );
  bpListRenderer = gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(bpListRenderer), "editable", TRUE, "mode", GTK_CELL_RENDERER_MODE_EDITABLE, NULL );
  bpListColumn = gtk_tree_view_column_new_with_attributes("Breakpoints", bpListRenderer, "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(sh2->bpList), bpListColumn);
  gtk_box_pack_start( GTK_BOX( hbox ), sh2->bpList, FALSE, FALSE, 4 );
  g_signal_connect(G_OBJECT(bpListRenderer), "edited", GTK_SIGNAL_FUNC(editedBp), sh2 );

  cbp = SH2GetBreakpointList(bMaster?MSH2:SSH2);
  
  for (i = 0; i < MAX_BREAKPOINTS; i++) {
    
    GtkTreeIter iter;
    sh2->cbp[i] = cbp[i].addr;
    gtk_list_store_append( GTK_LIST_STORE( sh2->bpListStore ), &iter );
    if (cbp[i].addr != 0xFFFFFFFF) {
      
      sprintf(tempstr, "%08X", (int)cbp[i].addr);
      gtk_list_store_set( GTK_LIST_STORE( sh2->bpListStore ), &iter, 0, tempstr, -1 );
    } else gtk_list_store_set( GTK_LIST_STORE( sh2->bpListStore ), &iter, 0, "<empty>", -1 );
  }

  /* Control buttons */
  
  hboxButtons = gtk_hbox_new(FALSE, 2);
  gtk_container_set_border_width( GTK_CONTAINER( hboxButtons ),4 );
  gtk_box_pack_start( GTK_BOX( vbox ), hboxButtons, FALSE, FALSE, 4 );

  buttonStep = gtk_button_new_with_label( "Step [F7]" );
  gtk_box_pack_start( GTK_BOX( hboxButtons ), buttonStep, FALSE, FALSE, 2 );
  g_signal_connect( buttonStep, "clicked", G_CALLBACK(sh2step), sh2 );

  buttonRun = gtk_button_new_with_label( "Run [F9]" );
  gtk_box_pack_start( GTK_BOX( hboxButtons ), buttonRun, FALSE, FALSE, 2 );
  g_signal_connect( buttonRun, "clicked", G_CALLBACK(yuiRun), NULL );

  accelGroup = gtk_accel_group_new ();
  closureF9 = g_cclosure_new (G_CALLBACK (yuiRun), NULL, NULL);
  closureF7 = g_cclosure_new (G_CALLBACK (bMaster ? sh2stepMaster : sh2stepSlave), NULL, NULL);
  gtk_accel_group_connect( accelGroup, GDK_F9, 0, 0, closureF9 );
  gtk_accel_group_connect( accelGroup, GDK_F7, 0, 0, closureF7 );
  gtk_window_add_accel_group( GTK_WINDOW( sh2->dialog ), accelGroup );

  g_signal_connect(G_OBJECT(sh2->dialog), "delete-event", GTK_SIGNAL_FUNC(gtk_widget_hide_on_delete), NULL );
  /* never kill this dialog - Just hide it */
  gtk_widget_show_all( sh2->dialog );
  gtk_window_activate_focus( GTK_WINDOW(sh2->dialog) );
  return sh2;
}

static void openVDP1() {

}

static void openVDP2() {

}

static void openM68K() {

}

static void openSCUDSP() {

}

static void debugUpdateViews() {

  if ( openedSH2[FALSE] ) sh2update( FALSE );
  if ( openedSH2[TRUE] ) sh2update( TRUE );
  memDumpUpdateAll();
}

static GtkWidget* widgetDebug() {

  GtkWidget *hbox = gtk_hbox_new( FALSE, 4 );
  GtkWidget *buttonMemDump = gtk_button_new_with_label( "Memory Dump" );
  GtkWidget *buttonMSH2 = gtk_button_new_with_label( "MSH2" );
  GtkWidget *buttonSSH2 = gtk_button_new_with_label( "SSH2" );
  GtkWidget *buttonVDP1 = gtk_button_new_with_label( "VDP1" );
  GtkWidget *buttonVDP2 = gtk_button_new_with_label( "VDP2" );
  GtkWidget *buttonM68K = gtk_button_new_with_label( "M68K" );
  GtkWidget *buttonSCUDSP = gtk_button_new_with_label( "ScuDsp" );
  gtk_box_pack_start( GTK_BOX( hbox ), buttonMemDump, FALSE, FALSE, 4 );
  gtk_box_pack_start( GTK_BOX( hbox ), buttonMSH2, FALSE, FALSE, 4 );
  gtk_box_pack_start( GTK_BOX( hbox ), buttonSSH2, FALSE, FALSE, 4 );
  gtk_box_pack_start( GTK_BOX( hbox ), buttonVDP1, FALSE, FALSE, 4 );
  gtk_box_pack_start( GTK_BOX( hbox ), buttonVDP2, FALSE, FALSE, 4 );
  gtk_box_pack_start( GTK_BOX( hbox ), buttonM68K, FALSE, FALSE, 4 );
  gtk_box_pack_start( GTK_BOX( hbox ), buttonSCUDSP, FALSE, FALSE, 4 );

  g_signal_connect(buttonMemDump, "clicked",
		   G_CALLBACK(openMemDump), NULL);
  g_signal_connect(buttonMSH2, "clicked",
		   G_CALLBACK(openSH2), (gpointer)TRUE );
  g_signal_connect(buttonSSH2, "clicked",
		   G_CALLBACK(openSH2), (gpointer)FALSE );
  g_signal_connect(buttonVDP1, "clicked",
		   G_CALLBACK(openVDP1), NULL);
  g_signal_connect(buttonVDP2, "clicked",
		   G_CALLBACK(openVDP2), NULL);
  g_signal_connect(buttonM68K, "clicked",
		   G_CALLBACK(openM68K), NULL);
  g_signal_connect(buttonSCUDSP, "clicked",
		   G_CALLBACK(openSCUDSP), NULL);

  return hbox;
}
