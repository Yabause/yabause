#include "../yui.h"
#include "../sndsdl.h"
#include "../vidsdlgl.h"
#include "../persdl.h"
#include "../cs0.h"

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>
#include "yabause_logo.xpm"

#include "SDL.h"

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

char SDL_windowhack[32];
int cdcore = CDCORE_DEFAULT;
const char * bios = "jap.rom";
const char * iso_or_cd = 0;

void GtkYuiRun(void);
void GtkYuiChooseBios(void);
void GtkYuiChooseCdrom(void);
void GtkYuiChooseIso(void);
void GtkYuiAbout(void);

static GtkItemFactoryEntry entries[] = {
	{ "/_Yabause",			NULL,		NULL,			0, "<Branch>" },
	{ "/Yabause/tear1",		NULL,		NULL,			0, "<Tearoff>" },
	{ "/Yabause/_Open BIOS...",	"<CTRL>B",	GtkYuiChooseBios,	1, "<Item>" },
	{ "/Yabause/_Open CDROM...",	"<CTRL>C",	GtkYuiChooseCdrom,	1, "<Item>" },
	{ "/Yabause/_Open ISO...",	"<CTRL>I",	GtkYuiChooseIso,	1, "<Item>" },
	{ "/Yabause/sep1",		NULL,		NULL,			0, "<Separator>" },
	{ "/Yabause/_Quit",		"<CTRL>Q",	YuiQuit,		0, "<StockItem>", GTK_STOCK_QUIT },
	{ "/Emulation",			NULL,		NULL,			0, "<Branch>" },
	{ "/Emulation/tear2",		NULL,		NULL,			0, "<Tearoff>" },
	{ "/Emulation/_Run",		"r",		GtkYuiRun,		1, "<Item>" },
	{"/_Help",			NULL,		NULL,			0, "<Branch>"},
	{"/Help/tear3",			NULL,		NULL,			0, "<Tearoff>"},
	{"/Help/About",			NULL,		GtkYuiAbout,			1, "<Item>"}
};

struct {
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *event_box;
	GtkWidget *menu_bar;
	gboolean hide;
} GtkYui;

int GtkWorkaround(void) {
	return PERCore->HandleEvents() + 1;
}

static gboolean GtkYuiKeyPress( GtkWidget *widget, GdkEvent *event, gpointer data) {
	SDL_Event eve;
	eve.type = eve.key.type = SDL_KEYDOWN;
	eve.key.state = SDL_PRESSED;
	switch(event->key.keyval) {
		case GDK_Up:
			eve.key.keysym.sym = SDLK_UP;
			break;
		case GDK_Right:
			eve.key.keysym.sym = SDLK_RIGHT;
			break;
		case GDK_Down:
			eve.key.keysym.sym = SDLK_DOWN;
			break;
		case GDK_Left:
			eve.key.keysym.sym = SDLK_LEFT;
			break;
		case GDK_F1:
			eve.key.keysym.sym = SDLK_F1;
			break;
		default:
			eve.key.keysym.sym = (SDLKey) event->key.keyval;
			break;
	}
	SDL_PushEvent(&eve);
}

static gboolean GtkYuiKeyRelease( GtkWidget *widget, GdkEvent *event, gpointer data) {
	SDL_Event eve;
	eve.type = eve.key.type = SDL_KEYUP;
	eve.key.state = SDL_RELEASED;
	switch(event->key.keyval) {
		case GDK_Up:
			eve.key.keysym.sym = SDLK_UP;
			break;
		case GDK_Right:
			eve.key.keysym.sym = SDLK_RIGHT;
			break;
		case GDK_Down:
			eve.key.keysym.sym = SDLK_DOWN;
			break;
		case GDK_Left:
			eve.key.keysym.sym = SDLK_LEFT;
			break;
		case GDK_F1:
			eve.key.keysym.sym = SDLK_F1;
			break;
		default:
			eve.key.keysym.sym = (SDLKey) event->key.keyval;
			break;
	}
	SDL_PushEvent(&eve);
}

static void GtkYuiSetCdrom(GtkWidget * w, GtkFileSelection * fs) {
	cdcore = CDCORE_ARCH;
        iso_or_cd = g_strdup(gtk_file_selection_get_filename (GTK_FILE_SELECTION (fs)));
        gtk_widget_destroy( GTK_WIDGET(fs) );
}

void GtkYuiChooseCdrom(void) {
        GtkWidget *filew = gtk_file_selection_new ("File selection");

        g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (filew)->ok_button), "clicked", G_CALLBACK (GtkYuiSetCdrom), (gpointer) filew);
        g_signal_connect_swapped (G_OBJECT (GTK_FILE_SELECTION (filew)->cancel_button), "clicked", G_CALLBACK (gtk_widget_destroy), G_OBJECT (filew));

        gtk_widget_show (filew);
}

static void GtkYuiSetBios(GtkWidget * w, GtkFileSelection * fs) {
        bios = g_strdup(gtk_file_selection_get_filename (GTK_FILE_SELECTION (fs)));
        gtk_widget_destroy( GTK_WIDGET(fs) );
}

void GtkYuiChooseBios(void) {
        GtkWidget *filew;

        filew = gtk_file_selection_new ("File selection");
        g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (filew)->ok_button), "clicked", G_CALLBACK (GtkYuiSetBios), (gpointer) filew);
        g_signal_connect_swapped (G_OBJECT (GTK_FILE_SELECTION (filew)->cancel_button), "clicked", G_CALLBACK (gtk_widget_destroy), G_OBJECT (filew));

        gtk_widget_show (filew);
}

static void GtkYuisetIso(GtkWidget * w, GtkFileSelection * fs) {
	cdcore = CDCORE_ISO;
        iso_or_cd = g_strdup(gtk_file_selection_get_filename (GTK_FILE_SELECTION (fs)));
        gtk_widget_destroy(GTK_WIDGET(fs));
}

void GtkYuiChooseIso(void) {
        GtkWidget *filew = gtk_file_selection_new("File selection");
        g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(filew)->ok_button), "clicked", G_CALLBACK(GtkYuisetIso), (gpointer)filew);
        g_signal_connect_swapped(G_OBJECT(GTK_FILE_SELECTION(filew)->cancel_button), "clicked", G_CALLBACK(gtk_widget_destroy), G_OBJECT(filew));
        gtk_widget_show(filew);
}

void GtkYuiRun(void) {
   yabauseinit_struct yinit;

   yinit.percoretype = PERCORE_SDL;
   yinit.sh2coretype = SH2CORE_DEFAULT;
   yinit.vidcoretype = VIDCORE_SDLGL;
   yinit.sndcoretype = SNDCORE_SDL;
   yinit.cdcoretype = cdcore;
   yinit.carttype = CART_NONE;
   yinit.regionid = REGION_AUTODETECT;
   yinit.biospath = bios;
   yinit.cdpath = iso_or_cd;
   yinit.buppath = NULL;
   yinit.mpegpath = NULL;
   yinit.cartpath = NULL;

   YabauseInit(&yinit);

   g_idle_add((gboolean (*)(void*)) GtkWorkaround, NULL);
}

int GtkYuiInit(void) {
	int fake_argc = 0;
	char ** fake_argv = NULL;
	char * text;
	GtkItemFactory *gif;

	gtk_init (&fake_argc, &fake_argv);

	GtkYui.window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	g_signal_connect(G_OBJECT (GtkYui.window), "key_press_event", G_CALLBACK (GtkYuiKeyPress), NULL);
	g_signal_connect(G_OBJECT (GtkYui.window), "key_release_event", G_CALLBACK (GtkYuiKeyRelease), NULL);
	g_signal_connect(G_OBJECT (GtkYui.window), "delete_event", GTK_SIGNAL_FUNC(YuiQuit), NULL);

	GtkYui.vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (GtkYui.window), GtkYui.vbox);
	gtk_widget_show (GtkYui.vbox);

	text = g_strdup_printf ("Yabause %s", VERSION);
	gtk_window_set_title(GTK_WINDOW (GtkYui.window), text);
	g_free(text);

	gif = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<YabauseMain>", NULL);
	gtk_item_factory_create_items(gif, sizeof(entries)/sizeof(entries[0]), entries, NULL);

	GtkYui.menu_bar = gtk_item_factory_get_widget(gif, "<YabauseMain>");
	gtk_box_pack_start(GTK_BOX (GtkYui.vbox), GtkYui.menu_bar, FALSE, FALSE, 0);
	gtk_widget_show (GtkYui.menu_bar);

	GtkYui.event_box = gtk_event_box_new ();
	
	gtk_widget_set_usize(GtkYui.event_box, 320, 224);
	gtk_box_pack_start (GTK_BOX (GtkYui.vbox), GtkYui.event_box, FALSE, FALSE, 0);
	gtk_widget_show (GtkYui.event_box);
	gtk_widget_realize(GtkYui.event_box);

	gtk_window_set_resizable( GTK_WINDOW(GtkYui.window), FALSE);
	gtk_widget_show (GtkYui.window);

	GtkYui.hide = FALSE;
	sprintf(SDL_windowhack,"SDL_WINDOWID=%ld", GDK_WINDOW_XWINDOW(GTK_WIDGET(GtkYui.event_box)->window));
	putenv(SDL_windowhack);
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

void YuiErrorMsg(const char *string)
{
   fprintf(stderr, string);
}

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

