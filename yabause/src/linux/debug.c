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

#define MAX_VDP1_COMMAND 4000

typedef struct {

  gpointer memDump;
  gint num;
} _mdqTag;

typedef struct {
  ComboFileSelect* cfFile;
  GtkWidget *startEdit, *toEdit, *dialog, *checkExe;
} memTransferDlg;

typedef struct _memDumpDlg {

  GtkWidget *dialog, *list, *entryAddress;
  GtkListStore *listStore;
  u32 offset; /* the position we are dumping */
  gint nLines, wLine; /* number of lines, width of a line */
  _mdqTag tag[10];
  struct _memDumpDlg *pred, *succ; /* they are douly-linked chained */
} memDumpDlg;

typedef struct { 
  GtkWidget *dialog, *bpList, *regList, *uLabel;
  GtkListStore *bpListStore, *regListStore;
  u32 cbp[MAX_BREAKPOINTS]; /* the list of breakpoint positions, as they can be found in the list widget */
  u32 lastCode; /* offset of last unassembly. Try to reuse it to prevent sliding. */
  SH2_struct *debugsh;
} SH2procDlg;

typedef struct { 
  GtkWidget *dialog, *bpList, *regList, *uLabel;
  GtkListStore *bpListStore, *regListStore;
  u32 cbp[MAX_BREAKPOINTS]; /* the list of breakpoint positions, as they can be found in the list widget */
  u32 lastCode; /* offset of last unassembly. Try to reuse it to prevent sliding. */
} SCUDSPprocDlg;

typedef struct { 
  GtkWidget *dialog, *bpList, *regList, *uLabel;
  GtkListStore *bpListStore, *regListStore;
  u32 cbp[MAX_BREAKPOINTS]; /* the list of breakpoint positions, as they can be found in the list widget */
  u32 lastCode; /* offset of last unassembly. Try to reuse it to prevent sliding. */
} M68KprocDlg;

typedef struct {
  GtkWidget *dialog, *spin, *commName, *commDesc;
  gint curs;
} VDP1procDlg;

typedef struct {
  GtkWidget *dialog, *NBG0, *NBG1, *NBG2, *NBG3, *RBG0;
} VDP2procDlg;

static SH2procDlg SSH2procDlg;
static SH2procDlg MSH2procDlg;
static SCUDSPprocDlg SCUDSPproc;
static VDP1procDlg VDP1proc;
static VDP2procDlg VDP2proc;
static M68KprocDlg M68Kproc;
static memTransferDlg transferDlg;

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
  /* to be called whenever memory content may have changed */

  memDumpDlg *memDump;
  for ( memDump = dumpDlgs ; memDump ; memDump = memDump->succ ) memDumpUpdate( memDump );
}

static void editedAddressDump( GtkWidget *widget, memDumpDlg *memDump ) {
  /* address bar has been modified */

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
  /* dump line <arg1> has been modified - new content is <arg2> */

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
  debugUpdateViews();
}

static gboolean memDumpDeleteCallback(GtkWidget *widget,
				      GdkEvent *event,
				      memDumpDlg *memDump) {

  memDumpDelete( memDump );
  return FALSE; /* propagate event */
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


gpointer mdqTag( memDumpDlg* _memDump, gint _num ) {

  _memDump->tag[_num].num = _num;
  _memDump->tag[_num].memDump = _memDump;
  return &(_memDump->tag[_num]);
}

static void memDumpQuick( GtkWidget* w, _mdqTag* tag ) {

  u32 addr;

  switch ( tag->num ) {

    case 0: addr = 0x25E00000 ; break; /* VDP2_VRAM_A0*/
    case 1: addr = 0x25E20000 ; break; /* VDP2_VRAM_A1*/
    case 2: addr = 0x25E40000 ; break; /* VDP2_VRAM_B0*/
    case 3: addr = 0x25E60000 ; break; /* VDP2_VRAM_B1*/
    case 4: addr = 0x25F00000 ; break; /* VDP2_CRAM*/
    case 5: addr = 0x20200000 ; break; /* LWRAM*/
    case 6: addr = 0x26000000 ; break; /* HWRAM*/
    case 7: addr = 0x25C00000 ; break; /* SpriteVRAM*/
  }
  ((memDumpDlg*)tag->memDump)->offset = addr;
  memDumpUpdate( (memDumpDlg*)tag->memDump );
}

static memDumpDlg* openMemDump() {

  static gchar* quickName[8] = {"VDP2_VRAM_A0", "VDP2_VRAM_A1", "VDP2_VRAM_B0", "VDP2_VRAM_B1", "VDP2_CRAM", "LWRAM", "HWRAM", "SpriteVRAM" };
  GtkWidget *vbox, *hbox, *vboxQuick;
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
  gtk_window_set_icon( GTK_WINDOW(memDump->dialog), yui.pixBufIcon );

  hbox = gtk_hbox_new(FALSE, 2);
  gtk_container_set_border_width( GTK_CONTAINER( hbox ),4 );
  gtk_container_add (GTK_CONTAINER (memDump->dialog), hbox);  
   
  vbox = gtk_vbox_new(FALSE, 2);
  gtk_box_pack_start( GTK_BOX( hbox ), vbox, FALSE, FALSE, 4 );  

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

  /* Quick buttons */

  vboxQuick = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start( GTK_BOX( hbox ), vboxQuick, FALSE, FALSE, 4 );
  for ( i = 0 ; i < 8 ; i++ ) {
    GtkWidget* button;
    g_signal_connect(G_OBJECT(button=gtk_button_new_with_label( quickName[i] )), "clicked", 
		     GTK_SIGNAL_FUNC(memDumpQuick), mdqTag(memDump,i) );
    gtk_box_pack_start( GTK_BOX( vboxQuick ), button, FALSE, FALSE, 0 );
  }

  memDumpUpdate(memDump);

  accelGroup = gtk_accel_group_new ();
  gtk_accel_group_connect( accelGroup, GDK_Page_Up, 0, 0, g_cclosure_new (G_CALLBACK(memDumpPageUpCallback), memDump, NULL) );
  gtk_accel_group_connect( accelGroup, GDK_Page_Down, 0, 0, g_cclosure_new (G_CALLBACK(memDumpPageDownCallback), memDump, NULL) );
  gtk_window_add_accel_group( GTK_WINDOW( memDump->dialog ), accelGroup );

  g_signal_connect(G_OBJECT(memDump->dialog), "delete-event", GTK_SIGNAL_FUNC(memDumpDeleteCallback), memDump );
  gtk_widget_show_all( memDump->dialog );
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
  *curs = 0;
  gtk_label_set_markup( GTK_LABEL(sh2->uLabel), buf );
}

static void sh2step( GtkWidget* widget, SH2procDlg* sh2 ) {

  if ( yui.running != GTKYUI_PAUSE) return;
  SH2Step(sh2->debugsh);
  debugUpdateViews(); /* update all dialogs, including us */
}

static void sh2stepMaster() { sh2step( NULL, &MSH2procDlg ); }
static void sh2stepSlave() { sh2step( NULL, &SSH2procDlg ); }

static void sh2stepOver( GtkWidget* widget, SH2procDlg* sh2 ) {

  sh2regs_struct sh2regs;
  if ( yui.running != GTKYUI_PAUSE) return;

  SH2GetRegisters(sh2->debugsh, &sh2regs);

  if ( sh2->cbp[MAX_BREAKPOINTS-1] != 0xFFFFFFFF) SH2DelCodeBreakpoint(sh2->debugsh, sh2->cbp[MAX_BREAKPOINTS-1]);
  if (SH2AddCodeBreakpoint(sh2->debugsh, sh2regs.PC+2) == 0) {

    sh2->cbp[MAX_BREAKPOINTS-1] = sh2regs.PC+2;
    yuiRun(); /* pray we catch the breakpoint */
  } else {
    sh2->cbp[MAX_BREAKPOINTS-1] = 0xFFFFFFFF;
    yuiErrorPopup("Cannot step over: sh2 refuses to register a new breakpoint.");
  }
}

static void sh2stepOverMaster() { sh2stepOver( NULL, &MSH2procDlg ); }
static void sh2stepOverSlave() { sh2stepOver( NULL, &SSH2procDlg ); }

static void editedReg( GtkCellRendererText *cellrenderertext,
		      gchar *arg1,
		      gchar *arg2,
		      SH2procDlg *sh2) {
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
}

static void editedBp( GtkCellRendererText *cellrenderertext,
		      gchar *arg1,
		      gchar *arg2,
		      SH2procDlg *sh2) {
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

static void SH2BreakpointHandler (SH2_struct *context, u32 addr) {

  yuiPause();
  {
    sh2regs_struct sh2regs;
    SH2procDlg* sh2 = openSH2( NULL, (gpointer)(context == MSH2) );
    
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

static void sh2update( gboolean bMaster ) {

  sh2regs_struct sh2regs;
  SH2procDlg *sh2 = bMaster ? &MSH2procDlg : &SSH2procDlg;
  SH2GetRegisters(sh2->debugsh, &sh2regs);
  SH2UpdateCodeList(sh2,sh2regs.PC);
  SH2UpdateRegList(sh2, &sh2regs);
}

static SH2procDlg* openSH2( GtkWidget* widget, gpointer bMaster ) {

  GtkWidget *hbox, *vbox, *uFrame, *buttonStep, *buttonRun, *buttonStepOver, *buttonPause, *hboxButtons1, *hboxButtons2;
  GClosure *closureF9, *closureF8, *closureF7, *closureF6;
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
  gtk_window_set_icon( GTK_WINDOW(sh2->dialog), yui.pixBufIcon );

  hbox = gtk_hbox_new(FALSE, 2);
  gtk_container_set_border_width( GTK_CONTAINER( hbox ),4 );
  gtk_container_add (GTK_CONTAINER (sh2->dialog), hbox);  

  vbox = gtk_vbox_new(FALSE, 2);
  gtk_container_set_border_width( GTK_CONTAINER( vbox ),4 );
  gtk_box_pack_start( GTK_BOX( hbox ), vbox, FALSE, FALSE, 4 );

  /* unassembler frame */

  uFrame = gtk_frame_new("Disassembled code");
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
  
  for (i = 0; i < MAX_BREAKPOINTS-1; i++) {
    
    GtkTreeIter iter;
    sh2->cbp[i] = cbp[i].addr;
    gtk_list_store_append( GTK_LIST_STORE( sh2->bpListStore ), &iter );
    if (cbp[i].addr != 0xFFFFFFFF) {
      
      sprintf(tempstr, "%08X", (int)cbp[i].addr);
      gtk_list_store_set( GTK_LIST_STORE( sh2->bpListStore ), &iter, 0, tempstr, -1 );
    } else gtk_list_store_set( GTK_LIST_STORE( sh2->bpListStore ), &iter, 0, "<empty>", -1 );
  }

  /* Control buttons */
  
  hboxButtons1 = gtk_hbox_new(FALSE, 2);
  gtk_container_set_border_width( GTK_CONTAINER( hboxButtons1 ),4 );
  gtk_box_pack_start( GTK_BOX( vbox ), hboxButtons1, FALSE, FALSE, 4 );

  buttonStep = gtk_button_new_with_label( "Step [F7]" );
  gtk_box_pack_start( GTK_BOX( hboxButtons1 ), buttonStep, FALSE, FALSE, 2 );
  g_signal_connect( buttonStep, "clicked", G_CALLBACK(sh2step), sh2 );

  buttonStepOver = gtk_button_new_with_label( "Step over [F8]" );
  gtk_box_pack_start( GTK_BOX( hboxButtons1 ), buttonStepOver, FALSE, FALSE, 2 );
  g_signal_connect( buttonStepOver, "clicked", G_CALLBACK(sh2stepOver), sh2 );

  hboxButtons2 = gtk_hbox_new(FALSE, 2);
  gtk_container_set_border_width( GTK_CONTAINER( hboxButtons2 ),4 );
  gtk_box_pack_start( GTK_BOX( vbox ), hboxButtons2, FALSE, FALSE, 4 );

  buttonRun = gtk_button_new_with_label( "Run [F9]" );
  gtk_box_pack_start( GTK_BOX( hboxButtons2 ), buttonRun, FALSE, FALSE, 2 );
  g_signal_connect( buttonRun, "clicked", G_CALLBACK(yuiRun), NULL );

  buttonPause = gtk_button_new_with_label( "Pause [F6]" );
  gtk_box_pack_start( GTK_BOX( hboxButtons2 ), buttonPause, FALSE, FALSE, 2 );
  g_signal_connect( buttonPause, "clicked", G_CALLBACK(yuiPause), NULL );

  accelGroup = gtk_accel_group_new ();
  closureF9 = g_cclosure_new (G_CALLBACK (yuiRun), NULL, NULL);
  closureF8 = g_cclosure_new (G_CALLBACK (bMaster ? sh2stepOverMaster : sh2stepOverSlave), NULL, NULL);
  closureF7 = g_cclosure_new (G_CALLBACK (bMaster ? sh2stepMaster : sh2stepSlave), NULL, NULL);
  closureF6 = g_cclosure_new (G_CALLBACK (yuiPause), NULL, NULL);
  gtk_accel_group_connect( accelGroup, GDK_F9, 0, 0, closureF9 );
  gtk_accel_group_connect( accelGroup, GDK_F8, 0, 0, closureF8 );
  gtk_accel_group_connect( accelGroup, GDK_F7, 0, 0, closureF7 );
  gtk_accel_group_connect( accelGroup, GDK_F6, 0, 0, closureF6 );
  gtk_window_add_accel_group( GTK_WINDOW( sh2->dialog ), accelGroup );

  g_signal_connect(G_OBJECT(sh2->dialog), "delete-event", GTK_SIGNAL_FUNC(gtk_widget_hide_on_delete), NULL );
  /* never kill this dialog - Just hide it */
  gtk_widget_show_all( sh2->dialog );
  return sh2;
}

/* ------------------------------------------------------------------------------- */
/* VDP1 Dialog ------------------------------------------------------------------- */

static void vdp1define() {
  /* fill in all dialog elements */
 
  gint i;
  gchar *string;
  gchar nameTemp[1024];
  gchar *nameCurs;
  static gchar tagPC[] = "<span foreground=\"red\">";
  static gchar tagEnd[] = "</span>";
  static gchar noName[] = "---";

  nameCurs = nameTemp;
  for ( i = VDP1proc.curs-2 ; i < VDP1proc.curs + 3 ; i++ ) {

    if ( i >= 0 ) string = Vdp1DebugGetCommandNumberName(i);
    else string = noName;
    if ( !string ) string = noName;

    if ( i == VDP1proc.curs ) { strcpy( nameCurs, tagPC ); nameCurs += strlen( tagPC ); }
    strcpy( nameCurs, string ); nameCurs += strlen( string );
    *(nameCurs++) = '\n';
    if ( i == VDP1proc.curs ) { strcpy( nameCurs, tagEnd ); nameCurs += strlen( tagEnd ); }
  }
  *nameCurs = 0;
  gtk_label_set_markup( GTK_LABEL(VDP1proc.commName), nameTemp );

  Vdp1DebugCommand( VDP1proc.curs, nameTemp );
  gtk_label_set_text( GTK_LABEL(VDP1proc.commDesc), nameTemp );
}

static void vdp1cursorChanged (GtkWidget *spin, gpointer user_data) {
  /* called when user change line number */

  VDP1proc.curs = gtk_spin_button_get_value_as_int( GTK_SPIN_BUTTON(spin) );
  vdp1define();
}

static void vdp1update() {
  /* to be called when command list has changed */

  gint i;
  for ( i = 0 ; i < MAX_VDP1_COMMAND ; i++ ) if ( !Vdp1DebugGetCommandNumberName(i) ) break;
  gtk_spin_button_set_range(GTK_SPIN_BUTTON(VDP1proc.spin),0,i-1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(VDP1proc.spin),0);
  VDP1proc.curs = 0;
  vdp1define();
}

static gboolean vdp1deleteSignal() {

  openedVDP1 = FALSE;
  return FALSE; /* propagate event */
}

static void openVDP1() {

  GtkWidget *vbox, *hbox;

  /* Initialization */

  if ( openedVDP1 ) {

    gtk_widget_show_all( VDP1proc.dialog );
    return;
  }
  if ( yui.running == GTKYUI_WAIT ) {
    yuiYabauseInit(); /* force yabause initialization */
    YabauseExec();
  }
  openedVDP1 = TRUE;
  
  /* Dialog window */

  VDP1proc.dialog = gtk_window_new( GTK_WINDOW_TOPLEVEL );
  gtk_window_set_title( GTK_WINDOW(VDP1proc.dialog), "VDP1" );
  gtk_window_set_resizable( GTK_WINDOW(VDP1proc.dialog), FALSE );
  gtk_window_set_icon( GTK_WINDOW(VDP1proc.dialog), yui.pixBufIcon );

  vbox = gtk_vbox_new(FALSE, 2);
  gtk_container_set_border_width( GTK_CONTAINER( vbox ),4 );
  gtk_container_add (GTK_CONTAINER (VDP1proc.dialog), vbox);  

  /* Command list */

  hbox = gtk_hbox_new(FALSE, 2);
  VDP1proc.spin = gtk_spin_button_new_with_range(0,MAX_VDP1_COMMAND,1);
  gtk_box_pack_start( GTK_BOX( hbox ), VDP1proc.spin, FALSE, FALSE, 4 );
  g_signal_connect(G_OBJECT(VDP1proc.spin), "value-changed", GTK_SIGNAL_FUNC(vdp1cursorChanged), NULL );
  VDP1proc.commName = gtk_label_new("");
  gtk_box_pack_start( GTK_BOX( hbox ), VDP1proc.commName, FALSE, FALSE, 4 );
  gtk_box_pack_start( GTK_BOX( vbox ), hbox, FALSE, FALSE, 4 );

   /* Description text */

  VDP1proc.commDesc = gtk_label_new("");
  gtk_box_pack_start( GTK_BOX( vbox ), VDP1proc.commDesc, FALSE, FALSE, 4 );
  
  vdp1update();
  g_signal_connect(G_OBJECT(VDP1proc.dialog), "delete-event", GTK_SIGNAL_FUNC(vdp1deleteSignal), NULL );
  gtk_widget_show_all( VDP1proc.dialog );
}

/* ------------------------------------------------------------------------------- */
/* VDP2 Dialog ------------------------------------------------------------------- */

static void vdp2update() {
  /* to be called if something changed in plane stats */

  gchar tempstr[1024];
  gboolean isscrenabled;

  Vdp2DebugStatsNBG0(tempstr, &isscrenabled);  
  if ( isscrenabled ) gtk_label_set_text( GTK_LABEL(VDP2proc.NBG0), tempstr );
  else gtk_label_set_text( GTK_LABEL(VDP2proc.NBG0), "<disabled>" );

  Vdp2DebugStatsNBG1(tempstr, &isscrenabled);  
  if ( isscrenabled ) gtk_label_set_text( GTK_LABEL(VDP2proc.NBG1), tempstr );
  else gtk_label_set_text( GTK_LABEL(VDP2proc.NBG1), "<disabled>" );

  Vdp2DebugStatsNBG2(tempstr, &isscrenabled);  
  if ( isscrenabled ) gtk_label_set_text( GTK_LABEL(VDP2proc.NBG2), tempstr );
  else gtk_label_set_text( GTK_LABEL(VDP2proc.NBG2), "<disabled>" );

  Vdp2DebugStatsNBG3(tempstr, &isscrenabled);  
  if ( isscrenabled ) gtk_label_set_text( GTK_LABEL(VDP2proc.NBG3), tempstr );
  else gtk_label_set_text( GTK_LABEL(VDP2proc.NBG3), "<disabled>" );

  Vdp2DebugStatsRBG0(tempstr, &isscrenabled);  
  if ( isscrenabled ) gtk_label_set_text( GTK_LABEL(VDP2proc.RBG0), tempstr );
  else gtk_label_set_text( GTK_LABEL(VDP2proc.RBG0), "<disabled>" );
}

static void openVDP2() {

  GtkWidget *vbox, *frame, *scrolled;

  /* Initialization */

  if ( openedVDP2 ) {

    gtk_widget_show_all( VDP2proc.dialog );
    return;
  }
  if ( yui.running == GTKYUI_WAIT ) {
    yuiYabauseInit(); /* force yabause initialization */
    YabauseExec();
  }
  openedVDP2 = TRUE;
  
  /* Dialog window */

  VDP2proc.dialog = gtk_window_new( GTK_WINDOW_TOPLEVEL );
  gtk_window_set_title( GTK_WINDOW(VDP2proc.dialog), "VDP2" );
  gtk_window_resize( GTK_WINDOW(VDP2proc.dialog), 300, 600 );
  gtk_window_set_icon( GTK_WINDOW(VDP2proc.dialog), yui.pixBufIcon );

  scrolled = gtk_scrolled_window_new( NULL, NULL );
  gtk_container_add (GTK_CONTAINER (VDP2proc.dialog), scrolled);  
  
  vbox = gtk_vbox_new(FALSE, 2);
  gtk_container_set_border_width( GTK_CONTAINER( vbox ),4 );
  gtk_scrolled_window_add_with_viewport( GTK_SCROLLED_WINDOW( scrolled ), vbox );

  /* Plane list */
  
  frame = gtk_frame_new("NBG0/RBG1");
  gtk_container_add( GTK_CONTAINER ( frame ), VDP2proc.NBG0 = gtk_label_new("") );
  gtk_box_pack_start( GTK_BOX( vbox ), frame, FALSE, FALSE, 4 );

  frame = gtk_frame_new("NBG1");
  gtk_container_add( GTK_CONTAINER ( frame ), VDP2proc.NBG1 = gtk_label_new("") );
  gtk_box_pack_start( GTK_BOX( vbox ), frame, FALSE, FALSE, 4 );

  frame = gtk_frame_new("NBG2");
  gtk_container_add( GTK_CONTAINER ( frame ), VDP2proc.NBG2 = gtk_label_new("") );
  gtk_box_pack_start( GTK_BOX( vbox ), frame, FALSE, FALSE, 4 );

  frame = gtk_frame_new("NBG3");
  gtk_container_add( GTK_CONTAINER ( frame ), VDP2proc.NBG3 = gtk_label_new("") );
  gtk_box_pack_start( GTK_BOX( vbox ), frame, FALSE, FALSE, 4 );

  frame = gtk_frame_new("RBG0");
  gtk_container_add( GTK_CONTAINER ( frame ), VDP2proc.RBG0 = gtk_label_new("") );
  gtk_box_pack_start( GTK_BOX( vbox ), frame, FALSE, FALSE, 4 );
  
  vdp2update();
  g_signal_connect(G_OBJECT(VDP2proc.dialog), "delete-event", GTK_SIGNAL_FUNC(gtk_widget_hide_on_delete), NULL );
  /* never kill this dialog - Just hide it */
  gtk_widget_show_all( VDP2proc.dialog );
}

/* ------------------------------------------------------------------------------- */
/* M680000 Dialog ---------------------------------------------------------------- */

static void openM68K(GtkWidget* widget);

static void M68KupdateRegList( m68kregs_struct *regs) {
  /* refresh the registery list */

  GtkTreeIter iter;
  char regstr[32];
  char valuestr[32];
  int i;
 
  for ( i = 0 ; i < 8 ; i++ ) {
    
    if ( i==0 ) gtk_tree_model_get_iter_first( GTK_TREE_MODEL( M68Kproc.regListStore ), &iter );
    else gtk_tree_model_iter_next( GTK_TREE_MODEL( M68Kproc.regListStore ), &iter );
    sprintf(regstr, "D%d", i );
    sprintf(valuestr, "%08x", regs->D[i]);
    gtk_list_store_set( GTK_LIST_STORE( M68Kproc.regListStore ), &iter, 0, regstr, 1, valuestr, -1 );
  }
  for ( i = 0 ; i < 8 ; i++ ) {
    
    gtk_tree_model_iter_next( GTK_TREE_MODEL( M68Kproc.regListStore ), &iter );
    sprintf(regstr, "A%d", i );
    sprintf(valuestr, "%08x", regs->A[i]);
    gtk_list_store_set( GTK_LIST_STORE( M68Kproc.regListStore ), &iter, 0, regstr, 1, valuestr, -1 );
  }

  gtk_tree_model_iter_next( GTK_TREE_MODEL( M68Kproc.regListStore ), &iter );
  sprintf(valuestr, "%08x", regs->SR);
  gtk_list_store_set( GTK_LIST_STORE( M68Kproc.regListStore ), &iter, 0, "SR", 1, valuestr, -1 );

  gtk_tree_model_iter_next( GTK_TREE_MODEL( M68Kproc.regListStore ), &iter );
  sprintf(valuestr, "%08x", regs->PC);
  gtk_list_store_set( GTK_LIST_STORE( M68Kproc.regListStore ), &iter, 0, "PC", 1, valuestr, -1 );
}

static void M68KupdateCodeList(u32 addr) {
  /* refresh the assembler view. <addr> points the line to be highlighted. */

}

static void M68KEditedBp( GtkCellRendererText *cellrenderertext,
			  gchar *arg1,
			  gchar *arg2) {
  /* breakpoint <arg1> has been set to address <arg2> */

  GtkTreeIter iter;
  char bptext[10];
  char *endptr;
  int i = atoi(arg1);
  u32 addr;
  gtk_tree_model_get_iter_from_string( GTK_TREE_MODEL( M68Kproc.bpListStore ), &iter, arg1 );
  addr = strtol(arg2, &endptr, 16 );
  if ((endptr - arg2 < strlen(arg2)) || (!addr)) addr = 0xFFFFFFFF;
  if ( M68Kproc.cbp[i] != 0xFFFFFFFF) M68KDelCodeBreakpoint(M68Kproc.cbp[i]);
  M68Kproc.cbp[i] = 0xFFFFFFFF;

  if ((addr!=0xFFFFFFFF)&&(M68KAddCodeBreakpoint(addr) == 0)) {
   
    sprintf(bptext, "%08X", (int)addr);
    M68Kproc.cbp[i] = addr;
  } else strcpy(bptext,"<empty>");
  gtk_list_store_set( GTK_LIST_STORE( M68Kproc.bpListStore ), &iter, 0, bptext, -1 );
}

static void M68KbreakpointHandler (u32 addr) {

  yuiPause();
  openM68K(NULL);
  debugPauseLoop(); /* execution is suspended inside a normal cycle - enter secondary gtk loop */
}

static void M68Kupdate() {

  m68kregs_struct regs;
  M68KGetRegisters(&regs);
  M68KupdateCodeList(regs.PC);
  M68KupdateRegList(&regs);
}

static void M68Kstep( GtkWidget* widget, M68KprocDlg* m68k ) {

  M68KStep();
  debugUpdateViews(); /* update all dialogs, including us */
}

static void openM68K(GtkWidget* widget) {

  GtkWidget *hbox, *vbox, *uFrame, *buttonStep, *buttonRun, *hboxButtons;
  GClosure *closureF9, *closureF7;
  GtkAccelGroup *accelGroup;
  GtkCellRenderer *bpListRenderer, *regListRenderer1, *regListRenderer2;
  GtkTreeViewColumn *bpListColumn, *regListColumn1, *regListColumn2;
  codebreakpoint_struct *cbp;
  char tempstr[10];
  int i;

  /* Initialization */

  M68Kproc.lastCode = 0;
  if ( openedM68K ) {

    gtk_widget_show_all( M68Kproc.dialog );
    return;
  }
  openedM68K = TRUE;
  if ( yui.running == GTKYUI_WAIT ) yuiYabauseInit(); /* force yabause initialization */
  M68KSetBreakpointCallBack(&M68KbreakpointHandler);
 
  /* Dialog window */

  M68Kproc.dialog = gtk_window_new( GTK_WINDOW_TOPLEVEL );
  gtk_window_set_title( GTK_WINDOW(M68Kproc.dialog), "M68000" );
  gtk_window_set_resizable( GTK_WINDOW(M68Kproc.dialog), FALSE );
  gtk_window_set_icon( GTK_WINDOW(M68Kproc.dialog), yui.pixBufIcon );

  hbox = gtk_hbox_new(FALSE, 2);
  gtk_container_set_border_width( GTK_CONTAINER( hbox ),4 );
  gtk_container_add (GTK_CONTAINER (M68Kproc.dialog), hbox);  

  vbox = gtk_vbox_new(FALSE, 2);
  gtk_container_set_border_width( GTK_CONTAINER( vbox ),4 );
  gtk_box_pack_start( GTK_BOX( hbox ), vbox, FALSE, FALSE, 4 );

  /* unassembler frame */

  uFrame = gtk_frame_new("Disassembled code");
  gtk_box_pack_start( GTK_BOX( vbox ), uFrame, FALSE, FALSE, 4 );
  
  M68Kproc.uLabel = gtk_label_new("\n\nDisassembler for M68K is missing.\n");
  gtk_container_add (GTK_CONTAINER (uFrame), M68Kproc.uLabel );

  /* Register list */

  M68Kproc.regListStore = gtk_list_store_new(2,G_TYPE_STRING,G_TYPE_STRING);
  M68Kproc.regList = gtk_tree_view_new_with_model( GTK_TREE_MODEL(M68Kproc.regListStore) );
  regListRenderer1 = gtk_cell_renderer_text_new();
  regListRenderer2 = gtk_cell_renderer_text_new();
  regListColumn1 = gtk_tree_view_column_new_with_attributes("Register", regListRenderer1, "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(M68Kproc.regList), regListColumn1);
  regListColumn2 = gtk_tree_view_column_new_with_attributes("Value", regListRenderer2, "text", 1, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(M68Kproc.regList), regListColumn2);
  gtk_box_pack_start( GTK_BOX( hbox ), M68Kproc.regList, FALSE, FALSE, 4 );

  for (i = 0; i < 23 ; i++) {

    GtkTreeIter iter;
    gtk_list_store_append( GTK_LIST_STORE( M68Kproc.regListStore ), &iter );
  }

  /* breakpoint list */

  M68Kproc.bpListStore = gtk_list_store_new(1,G_TYPE_STRING);
  M68Kproc.bpList = gtk_tree_view_new_with_model( GTK_TREE_MODEL(M68Kproc.bpListStore) );
  bpListRenderer = gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(bpListRenderer), "editable", TRUE, "mode", GTK_CELL_RENDERER_MODE_EDITABLE, NULL );
  bpListColumn = gtk_tree_view_column_new_with_attributes("Breakpoints", bpListRenderer, "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(M68Kproc.bpList), bpListColumn);
  gtk_box_pack_start( GTK_BOX( hbox ), M68Kproc.bpList, FALSE, FALSE, 4 );
  g_signal_connect(G_OBJECT(bpListRenderer), "edited", GTK_SIGNAL_FUNC(M68KEditedBp), NULL );

  for (i = 0; i < MAX_BREAKPOINTS; i++) {
    
    GtkTreeIter iter;
    M68Kproc.cbp[i] = 0xFFFFFFFF;
    gtk_list_store_append( GTK_LIST_STORE( M68Kproc.bpListStore ), &iter );
    gtk_list_store_set( GTK_LIST_STORE( M68Kproc.bpListStore ), &iter, 0, "<empty>", -1 );
  }

  /* Control buttons */
  
  hboxButtons = gtk_hbox_new(FALSE, 2);
  gtk_container_set_border_width( GTK_CONTAINER( hboxButtons ),4 );
  gtk_box_pack_start( GTK_BOX( vbox ), hboxButtons, FALSE, FALSE, 4 );

  buttonStep = gtk_button_new_with_label( "Step [F7]" );
  gtk_box_pack_start( GTK_BOX( hboxButtons ), buttonStep, FALSE, FALSE, 2 );
  g_signal_connect( buttonStep, "clicked", G_CALLBACK(M68Kstep), NULL );

  buttonRun = gtk_button_new_with_label( "Run [F9]" );
  gtk_box_pack_start( GTK_BOX( hboxButtons ), buttonRun, FALSE, FALSE, 2 );
  g_signal_connect( buttonRun, "clicked", G_CALLBACK(yuiRun), NULL );

  accelGroup = gtk_accel_group_new ();
  closureF9 = g_cclosure_new (G_CALLBACK (yuiRun), NULL, NULL);
  closureF7 = g_cclosure_new (G_CALLBACK (M68Kstep), NULL, NULL);
  gtk_accel_group_connect( accelGroup, GDK_F9, 0, 0, closureF9 );
  gtk_accel_group_connect( accelGroup, GDK_F7, 0, 0, closureF7 );
  gtk_window_add_accel_group( GTK_WINDOW( M68Kproc.dialog ), accelGroup );

  M68Kupdate();
  g_signal_connect(G_OBJECT(M68Kproc.dialog), "delete-event", GTK_SIGNAL_FUNC(gtk_widget_hide_on_delete), NULL );
  /* never kill this dialog - Just hide it */
  gtk_widget_show_all( M68Kproc.dialog );
}


/* ------------------------------------------------------------------------------- */
/* SCUDSP Dialog ----------------------------------------------------------------- */

static void openSCUDSP(GtkWidget* widget);

static void SCUDSPupdateRegList( scudspregs_struct *regs) {
  /* refresh the registery list */

  GtkTreeIter iter;
  char regstr[32];
  char valuestr[32];
  int i;
  
  gtk_tree_model_get_iter_first( GTK_TREE_MODEL( SCUDSPproc.regListStore ), &iter );
  sprintf(valuestr, "%d", regs->ProgControlPort.part.PR);
  yuiUpcase(valuestr);
  gtk_list_store_set( GTK_LIST_STORE( SCUDSPproc.regListStore ), &iter, 0, "PR", 1, valuestr, -1 );

  #define SCUDSPUPDATEREGLISTp(rreg,format) \
  gtk_tree_model_iter_next( GTK_TREE_MODEL( SCUDSPproc.regListStore ), &iter ); \
  sprintf(valuestr, #format, (int)regs->ProgControlPort.part.rreg); \
  gtk_list_store_set( GTK_LIST_STORE( SCUDSPproc.regListStore ), &iter, 0, #rreg, 1, valuestr, -1 );
  #define SCUDSPUPDATEREGLIST(rreg,format) \
  gtk_tree_model_iter_next( GTK_TREE_MODEL( SCUDSPproc.regListStore ), &iter ); \
  sprintf(valuestr, #format, (int)regs->rreg); \
  gtk_list_store_set( GTK_LIST_STORE( SCUDSPproc.regListStore ), &iter, 0, #rreg, 1, valuestr, -1 );
  #define SCUDSPUPDATEREGLISTx(rreg,vreg,format) \
  gtk_tree_model_iter_next( GTK_TREE_MODEL( SCUDSPproc.regListStore ), &iter ); \
  sprintf(valuestr, #format, (int)(vreg)); \
  gtk_list_store_set( GTK_LIST_STORE( SCUDSPproc.regListStore ), &iter, 0, #rreg, 1, valuestr, -1 );
  
  SCUDSPUPDATEREGLISTp(EP,%d);
  SCUDSPUPDATEREGLISTp(T0,%d);
  SCUDSPUPDATEREGLISTp(S,%d);
  SCUDSPUPDATEREGLISTp(Z,%d);
  SCUDSPUPDATEREGLISTp(C,%d);
  SCUDSPUPDATEREGLISTp(V,%d);
  SCUDSPUPDATEREGLISTp(E,%d);
  SCUDSPUPDATEREGLISTp(ES,%d);
  SCUDSPUPDATEREGLISTp(EX,%d);
  SCUDSPUPDATEREGLISTp(LE,%d);
  SCUDSPUPDATEREGLISTp(P,%02X);
  SCUDSPUPDATEREGLIST(TOP,%02X);
  SCUDSPUPDATEREGLIST(LOP,%02X);
  gtk_tree_model_iter_next( GTK_TREE_MODEL( SCUDSPproc.regListStore ), &iter );
  sprintf(valuestr, "%08X", ((u32)(regs->CT[0]))<<24 + ((u32)(regs->CT[1]))<<16 + ((u32)(regs->CT[2]))<<8 + ((u32)(regs->CT[3])) );
  gtk_list_store_set( GTK_LIST_STORE( SCUDSPproc.regListStore ), &iter, 0, "CT", 1, valuestr, -1 );
  SCUDSPUPDATEREGLISTx(RA,regs->RA0,%08X);
  SCUDSPUPDATEREGLISTx(WA,regs->WA0,%08X);
  SCUDSPUPDATEREGLIST(RX,%08X);
  SCUDSPUPDATEREGLIST(RY,%08X);
  SCUDSPUPDATEREGLISTx(PH,regs->P.part.H & 0xFFFF,%04X);
  SCUDSPUPDATEREGLISTx(PL,regs->P.part.L & 0xFFFFFFFF,%08X);
  SCUDSPUPDATEREGLISTx(ACH,regs->AC.part.H & 0xFFFF,%04X);
  SCUDSPUPDATEREGLISTx(ACL,regs->AC.part.L & 0xFFFFFFFF,%08X);
}

static void SCUDSPupdateCodeList(u32 addr) {
  /* refresh the assembler view. <addr> points the line to be highlighted. */

  int i;
  static char tagPC[] = "<span foreground=\"red\">";
  static char tagEnd[] = "</span>\n";
  char buf[100*24+40];
  char *curs = buf;
  char lineBuf[100];
  u32 offset;

  if ( addr - SCUDSPproc.lastCode >= 20 ) offset = addr - 8;
  else offset = SCUDSPproc.lastCode;
  SCUDSPproc.lastCode = offset;

  for (i=0; i < 24; i++) {

    if ( offset + i == addr ) { strcpy( curs, tagPC ); curs += strlen(tagPC); }
    ScuDspDisasm(offset+i, lineBuf);
    strcpy( curs, lineBuf );
    curs += strlen(lineBuf);
    if ( offset + i == addr ) { strcpy( curs, tagEnd ); curs += strlen(tagEnd); }
    else { strcpy( curs, "\n" ); curs += 1;}
  }
  *curs = 0;
  gtk_label_set_markup( GTK_LABEL(SCUDSPproc.uLabel), buf );
}

static void SCUDSPstep( GtkWidget* widget, SCUDSPprocDlg* scu ) {

  ScuDspStep();
  debugUpdateViews(); /* update all dialogs, including us */
}

static void scuEditedBp( GtkCellRendererText *cellrenderertext,
		      gchar *arg1,
		      gchar *arg2) {
  /* breakpoint <arg1> has been set to address <arg2> */

  GtkTreeIter iter;
  char bptext[10];
  char *endptr;
  int i = atoi(arg1);
  u32 addr;
  gtk_tree_model_get_iter_from_string( GTK_TREE_MODEL( SCUDSPproc.bpListStore ), &iter, arg1 );
  addr = strtol(arg2, &endptr, 16 );
  if ((endptr - arg2 < strlen(arg2)) || (!addr)) addr = 0xFFFFFFFF;
  if ( SCUDSPproc.cbp[i] != 0xFFFFFFFF) ScuDspDelCodeBreakpoint(SCUDSPproc.cbp[i]);
  SCUDSPproc.cbp[i] = 0xFFFFFFFF;

  if ((addr!=0xFFFFFFFF)&&(ScuDspAddCodeBreakpoint(addr) == 0)) {
   
    sprintf(bptext, "%08X", (int)addr);
    SCUDSPproc.cbp[i] = addr;
  } else strcpy(bptext,"<empty>");
  gtk_list_store_set( GTK_LIST_STORE( SCUDSPproc.bpListStore ), &iter, 0, bptext, -1 );
}

static void SCUDSPbreakpointHandler (u32 addr) {

  yuiPause();
  openSCUDSP(NULL);
  debugPauseLoop(); /* execution is suspended inside a normal cycle - enter secondary gtk loop */
}

static void SCUDSPupdate() {

  scudspregs_struct regs;
  ScuDspGetRegisters(&regs);
  SCUDSPupdateCodeList(regs.ProgControlPort.part.P);
  SCUDSPupdateRegList(&regs);
}

static void openSCUDSP(GtkWidget* widget) {

  GtkWidget *hbox, *vbox, *uFrame, *buttonStep, *buttonRun, *hboxButtons;
  GClosure *closureF9, *closureF7;
  GtkAccelGroup *accelGroup;
  GtkCellRenderer *bpListRenderer, *regListRenderer1, *regListRenderer2;
  GtkTreeViewColumn *bpListColumn, *regListColumn1, *regListColumn2;
  codebreakpoint_struct *cbp;
  char tempstr[10];
  int i;

  /* Initialization */

  SCUDSPproc.lastCode = 0;
  if ( openedSCUDSP ) {

    gtk_widget_show_all( SCUDSPproc.dialog );
    return;
  }
  openedSCUDSP = TRUE;
  if ( yui.running == GTKYUI_WAIT ) yuiYabauseInit(); /* force yabause initialization */
  ScuDspSetBreakpointCallBack(&SCUDSPbreakpointHandler);
 
  /* Dialog window */

  SCUDSPproc.dialog = gtk_window_new( GTK_WINDOW_TOPLEVEL );
  gtk_window_set_title( GTK_WINDOW(SCUDSPproc.dialog), "SCU/DSP" );
  gtk_window_set_resizable( GTK_WINDOW(SCUDSPproc.dialog), FALSE );
  gtk_window_set_icon( GTK_WINDOW(SCUDSPproc.dialog), yui.pixBufIcon );

  hbox = gtk_hbox_new(FALSE, 2);
  gtk_container_set_border_width( GTK_CONTAINER( hbox ),4 );
  gtk_container_add (GTK_CONTAINER (SCUDSPproc.dialog), hbox);  

  vbox = gtk_vbox_new(FALSE, 2);
  gtk_container_set_border_width( GTK_CONTAINER( vbox ),4 );
  gtk_box_pack_start( GTK_BOX( hbox ), vbox, FALSE, FALSE, 4 );

  /* unassembler frame */

  uFrame = gtk_frame_new("Disassembled code");
  gtk_box_pack_start( GTK_BOX( vbox ), uFrame, FALSE, FALSE, 4 );
  
  SCUDSPproc.uLabel = gtk_label_new(NULL);
  gtk_container_add (GTK_CONTAINER (uFrame), SCUDSPproc.uLabel );

  /* Register list */

  SCUDSPproc.regListStore = gtk_list_store_new(2,G_TYPE_STRING,G_TYPE_STRING);
  SCUDSPproc.regList = gtk_tree_view_new_with_model( GTK_TREE_MODEL(SCUDSPproc.regListStore) );
  regListRenderer1 = gtk_cell_renderer_text_new();
  regListRenderer2 = gtk_cell_renderer_text_new();
  regListColumn1 = gtk_tree_view_column_new_with_attributes("Register", regListRenderer1, "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(SCUDSPproc.regList), regListColumn1);
  regListColumn2 = gtk_tree_view_column_new_with_attributes("Value", regListRenderer2, "text", 1, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(SCUDSPproc.regList), regListColumn2);
  gtk_box_pack_start( GTK_BOX( hbox ), SCUDSPproc.regList, FALSE, FALSE, 4 );

  for (i = 0; i < 23 ; i++) {

    GtkTreeIter iter;
    gtk_list_store_append( GTK_LIST_STORE( SCUDSPproc.regListStore ), &iter );
  }

  /* breakpoint list */

  SCUDSPproc.bpListStore = gtk_list_store_new(1,G_TYPE_STRING);
  SCUDSPproc.bpList = gtk_tree_view_new_with_model( GTK_TREE_MODEL(SCUDSPproc.bpListStore) );
  bpListRenderer = gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(bpListRenderer), "editable", TRUE, "mode", GTK_CELL_RENDERER_MODE_EDITABLE, NULL );
  bpListColumn = gtk_tree_view_column_new_with_attributes("Breakpoints", bpListRenderer, "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(SCUDSPproc.bpList), bpListColumn);
  gtk_box_pack_start( GTK_BOX( hbox ), SCUDSPproc.bpList, FALSE, FALSE, 4 );
  g_signal_connect(G_OBJECT(bpListRenderer), "edited", GTK_SIGNAL_FUNC(scuEditedBp), NULL );

  for (i = 0; i < MAX_BREAKPOINTS; i++) {
    
    GtkTreeIter iter;
    SCUDSPproc.cbp[i] = 0xFFFFFFFF;
    gtk_list_store_append( GTK_LIST_STORE( SCUDSPproc.bpListStore ), &iter );
    gtk_list_store_set( GTK_LIST_STORE( SCUDSPproc.bpListStore ), &iter, 0, "<empty>", -1 );
  }

  /* Control buttons */
  
  hboxButtons = gtk_hbox_new(FALSE, 2);
  gtk_container_set_border_width( GTK_CONTAINER( hboxButtons ),4 );
  gtk_box_pack_start( GTK_BOX( vbox ), hboxButtons, FALSE, FALSE, 4 );

  buttonStep = gtk_button_new_with_label( "Step [F7]" );
  gtk_box_pack_start( GTK_BOX( hboxButtons ), buttonStep, FALSE, FALSE, 2 );
  g_signal_connect( buttonStep, "clicked", G_CALLBACK(SCUDSPstep), NULL );

  buttonRun = gtk_button_new_with_label( "Run [F9]" );
  gtk_box_pack_start( GTK_BOX( hboxButtons ), buttonRun, FALSE, FALSE, 2 );
  g_signal_connect( buttonRun, "clicked", G_CALLBACK(yuiRun), NULL );

  accelGroup = gtk_accel_group_new ();
  closureF9 = g_cclosure_new (G_CALLBACK (yuiRun), NULL, NULL);
  closureF7 = g_cclosure_new (G_CALLBACK (SCUDSPstep), NULL, NULL);
  gtk_accel_group_connect( accelGroup, GDK_F9, 0, 0, closureF9 );
  gtk_accel_group_connect( accelGroup, GDK_F7, 0, 0, closureF7 );
  gtk_window_add_accel_group( GTK_WINDOW( SCUDSPproc.dialog ), accelGroup );

  SCUDSPupdate();
  g_signal_connect(G_OBJECT(SCUDSPproc.dialog), "delete-event", GTK_SIGNAL_FUNC(gtk_widget_hide_on_delete), NULL );
  /* never kill this dialog - Just hide it */
  gtk_widget_show_all( SCUDSPproc.dialog );
}

/* ---------------------------------------------------------------------------- */
/* Memory transfert                                                             */

static openMemCallback(GtkWidget* widget, gpointer action) {

  gchar *fileName;
  const gchar *str;
  gchar *endptr;
  gboolean bExe = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(transferDlg.checkExe) );
  u32 from, to;

  cfGetText( transferDlg.cfFile, &fileName );
  if ( !fileName ) {
    yuiErrorPopup("Please give a file name.");
    return;
  }

  from = strtol( str = gtk_entry_get_text( GTK_ENTRY(transferDlg.startEdit) ), &endptr, 16 );
  if (endptr - str < strlen(str)) {
    yuiErrorPopup("Invalid <From> field, please enter a 8 digit hexadecimal integer.");
    return;
  }
  if ( (gint)action == 1 ) { /* field <to> just for store action */
    to = strtol( str = gtk_entry_get_text( GTK_ENTRY(transferDlg.toEdit) ), &endptr, 16 );
    if (endptr - str < strlen(str)) {
      yuiErrorPopup("Invalid <To> field, please enter a 8 digit hexadecimal integer.");
      return;
    }
    if ( to <= from ) {
      yuiErrorPopup("Make sure <To> is greater than <From>.");
      return;
    }
  }

  switch ((gint)action) {
  case 0: /* load */
    if ( bExe ) {
      MappedMemoryLoadExec(fileName, from);
    } else {
      if ( MappedMemoryLoad(fileName, from) ) {
	yuiErrorPopup("Something bad happend when trying to load file.");
	return;      
      }
    }
    break;
  case 1: /* save */
    if ( MappedMemorySave(fileName, from, to-from) ) {
      yuiErrorPopup("Something bad happend when trying to store in file.");
      return;      
    }
    break;
  }
  yuiSetString( "transferpath", fileName );
  yuiSetInt( "transferStart", from );
  if ( (gint)action == 1 ) yuiSetInt( "transferTo", to );
  yuiSetInt( "checkExe", bExe );
  yuiStore();
  gtk_dialog_response(GTK_DIALOG( transferDlg.dialog ), GTK_RESPONSE_DELETE_EVENT );
  debugUpdateViews();
}

static void openMemTrans(GtkWidget* widget) {

  GtkWidget *vbox, *hboxstart, *hboxbuttons, *buttonLoad, *buttonStore;
  gchar str[16];
  if ( yui.running == GTKYUI_WAIT ) yuiYabauseInit(); /* force yabause initialization */
  transferDlg.dialog = gtk_dialog_new_with_buttons ( "Memory Transfer",
						    NULL,
						   (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
						    GTK_STOCK_CLOSE,
						    GTK_RESPONSE_DELETE_EVENT, NULL);
  gtk_window_set_icon( GTK_WINDOW(transferDlg.dialog), yui.pixBufIcon );
  gtk_window_set_resizable( GTK_WINDOW(transferDlg.dialog), FALSE );
  vbox = GTK_DIALOG(transferDlg.dialog)->vbox;

  /* File name */

  transferDlg.cfFile = cfNew( "transfer", "Transfer file" ); 
  cfCatchValue( transferDlg.cfFile, yuiGetString( "transferpath" ) );  
  gtk_box_pack_start( GTK_BOX( vbox ), cfGetWidget(transferDlg.cfFile), FALSE, FALSE, 4 );  

  /* Start address */
 
  hboxstart = gtk_hbox_new( FALSE, 4 );
  gtk_box_pack_start( GTK_BOX( vbox ), hboxstart, FALSE, FALSE, 4 );  

  gtk_box_pack_start( GTK_BOX( hboxstart ), gtk_label_new( "From >" ), FALSE, FALSE, 2 );
  transferDlg.startEdit = gtk_entry_new_with_max_length(10);
  g_sprintf( str, "%08X", yuiGetInt( "transferStart", 0 ) );
  gtk_entry_set_text( GTK_ENTRY( transferDlg.startEdit ), str );
  gtk_box_pack_start( GTK_BOX( hboxstart ), transferDlg.startEdit, FALSE, FALSE, 2 );

  gtk_box_pack_start( GTK_BOX( hboxstart ), gtk_label_new( "    To >" ), FALSE, FALSE, 2 );
  transferDlg.toEdit = gtk_entry_new_with_max_length(10);
  g_sprintf( str, "%08X", yuiGetInt( "transferTo", 0 ) );
  gtk_entry_set_text( GTK_ENTRY( transferDlg.toEdit ), str );
  gtk_box_pack_start( GTK_BOX( hboxstart ), transferDlg.toEdit, FALSE, FALSE, 2 );

  /* Buttons */

  hboxbuttons = gtk_hbox_new( FALSE, 4 );
  gtk_box_pack_start( GTK_BOX( vbox ), hboxbuttons, FALSE, FALSE, 4 );  

  buttonLoad = gtk_button_new_with_label( "Load" );
  buttonStore = gtk_button_new_with_label( "Store" );
  gtk_box_pack_start( GTK_BOX( hboxbuttons ), buttonLoad, FALSE, FALSE, 2 );
  g_signal_connect( buttonLoad, "clicked", G_CALLBACK(openMemCallback), (gpointer)0 );
  gtk_box_pack_start( GTK_BOX( hboxbuttons ), buttonStore, FALSE, FALSE, 2 );
  g_signal_connect( buttonStore, "clicked", G_CALLBACK(openMemCallback), (gpointer)1 );
  
  transferDlg.checkExe = gtk_check_button_new_with_label( "Load as executable" );
  gtk_box_pack_start( GTK_BOX( hboxbuttons ), transferDlg.checkExe, FALSE, FALSE, 4 );  
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( transferDlg.checkExe ), yuiGetInt( "checkExe", 0 ));

  gtk_widget_show_all(transferDlg.dialog); 
  gtk_dialog_run( GTK_DIALOG( transferDlg.dialog ) );

  cfDelete( transferDlg.cfFile );
  gtk_widget_destroy(transferDlg.dialog);
}

/* ------------------------------------------------------------------------------ */

static void debugUpdateViews() {
  /* to be called whenever something may have changed (run, step, user action on memory) */

  if ( openedVDP1 ) vdp1update();
  if ( openedVDP2 ) vdp2update();
  if ( openedSH2[FALSE] ) sh2update( FALSE );
  if ( openedSH2[TRUE] ) sh2update( TRUE );
  if ( openedSCUDSP ) SCUDSPupdate();
  if ( openedM68K ) M68Kupdate();
  memDumpUpdateAll();
}

static GtkWidget* widgetDebug() {

  GtkWidget *hbox = gtk_hbox_new( FALSE, 4 );
  GtkWidget *buttonMemDump = gtk_button_new_with_label( "Memory" );
  GtkWidget *buttonMemTrans = gtk_button_new_with_label( "Transfer" );
  GtkWidget *buttonMSH2 = gtk_button_new_with_label( "MSH2" );
  GtkWidget *buttonSSH2 = gtk_button_new_with_label( "SSH2" );
  GtkWidget *buttonVDP1 = gtk_button_new_with_label( "VDP1" );
  GtkWidget *buttonVDP2 = gtk_button_new_with_label( "VDP2" );
  GtkWidget *buttonM68K = gtk_button_new_with_label( "M68K" );
  GtkWidget *buttonSCUDSP = gtk_button_new_with_label( "ScuDsp" );
  gtk_box_pack_start( GTK_BOX( hbox ), buttonMemDump, FALSE, FALSE, 4 );
  gtk_box_pack_start( GTK_BOX( hbox ), buttonMemTrans, FALSE, FALSE, 4 );
  gtk_box_pack_start( GTK_BOX( hbox ), buttonMSH2, FALSE, FALSE, 4 );
  gtk_box_pack_start( GTK_BOX( hbox ), buttonSSH2, FALSE, FALSE, 4 );
  gtk_box_pack_start( GTK_BOX( hbox ), buttonVDP1, FALSE, FALSE, 4 );
  gtk_box_pack_start( GTK_BOX( hbox ), buttonVDP2, FALSE, FALSE, 4 );
  gtk_box_pack_start( GTK_BOX( hbox ), buttonM68K, FALSE, FALSE, 4 );
  gtk_box_pack_start( GTK_BOX( hbox ), buttonSCUDSP, FALSE, FALSE, 4 );

  g_signal_connect(buttonMemDump, "clicked",
		   G_CALLBACK(openMemDump), NULL);
  g_signal_connect(buttonMemTrans, "clicked",
		   G_CALLBACK(openMemTrans), NULL);
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
