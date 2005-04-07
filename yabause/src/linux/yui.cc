/*  Copyright 2004 Guillaume Duhamel
    Copyright 2005 Joost Peters
    Copyright 2005 Israel Jacques

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

#include "../yui.hh"
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <gdk/gdkx.h>
#include <SDL/SDL.h>
#include "../saturn_memory.hh"
#include "cd.hh"
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>
#include "yabause_logo.xpm"


const char *bios = 0;
const char *iso = 0;
CDInterface *cd = 0;
const char *binary;
SaturnMemory *saturn;
int (*yab_main)(void *);


void yui_set_bios_filename(const char *biosfilename) {
	bios = biosfilename;
}

void yui_set_iso_filename(const char *isofilename) {
	iso = isofilename;
}
void yui_choose_bios(void);
void yui_choose_cdrom(void);
void yui_choose_binary(void);
void yui_choose_iso(void);
void yui_coin_coin(void);
void yui_about(void);
static GtkWidget *generate_credit_list(const gchar * text[], gboolean sec_space);

static GtkItemFactoryEntry entries[] = {
	{ "/_Yabause",			NULL,		NULL,			0, "<Branch>" },
	{ "/Yabause/tear1",		NULL,		NULL,			0, "<Tearoff>" },
	{ "/Yabause/_Open BIOS...",	"<CTRL>B",	yui_choose_bios,	1, "<Item>" },
	{ "/Yabause/_Open Binary...",	NULL,		yui_choose_binary,	1, "<Item>" },
	{ "/Yabause/_Open CDROM...",	"<CTRL>C",	yui_choose_cdrom,	1, "<Item>" },
	{ "/Yabause/_Open ISO...",	"<CTRL>I",	yui_choose_iso,		1, "<Item>" },
	{ "/Yabause/sep1",		NULL,		NULL,			0, "<Separator>" },
	{ "/Yabause/_Quit",		"<CTRL>Q",	yui_quit,		0, "<StockItem>", GTK_STOCK_QUIT },
	{ "/Emulation",			NULL,		NULL,			0, "<Branch>" },
	{ "/Emulation/tear2",		NULL,		NULL,			0, "<Tearoff>" },
	{ "/Emulation/_Run",		"r",		yui_coin_coin,		1, "<Item>" },
	{"/_Help",NULL,NULL,0,"<Branch>"},
	{"/Help/tear3",NULL,NULL,0,"<Tearoff>"},
	{"/Help/About",NULL,yui_about,1,"<Item>"}
};

GtkWidget *status_bar;
gint context_id;
GtkWidget *window;
GtkWidget *event_box;
GtkWidget *menu_bar;
gboolean hide;

char SDL_windowhack[32];

static gboolean key_press( GtkWidget *widget, GdkEvent *event, gpointer data) {
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

static gboolean key_release( GtkWidget *widget, GdkEvent *event, gpointer data) {
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

void yui_init(int (*fonction)(void *)) {
	GtkWidget *vbox;
	GtkItemFactory *gif;
	char *text;
        int fake_argc = 0;
        char ** fake_argv = NULL;

	gtk_init (&fake_argc, &fake_argv);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	g_signal_connect(G_OBJECT (window), "key_press_event", G_CALLBACK (key_press), NULL);
	g_signal_connect(G_OBJECT (window), "key_release_event", G_CALLBACK (key_release), NULL);
	g_signal_connect(G_OBJECT (window), "delete_event", GTK_SIGNAL_FUNC(yui_quit), NULL);
	
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (window), vbox);
	gtk_widget_show (vbox);

	text = g_strdup_printf ("Yabause %s", VERSION);
	gtk_window_set_title(GTK_WINDOW (window), text);
	g_free(text);

	gif = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<YabauseMain>", NULL);
	gtk_item_factory_create_items(gif, sizeof(entries)/sizeof(entries[0]), entries, NULL);

	menu_bar = gtk_item_factory_get_widget(gif, "<YabauseMain>");
	gtk_box_pack_start(GTK_BOX (vbox), menu_bar, FALSE, FALSE, 0);
	gtk_widget_show (menu_bar);

	event_box = gtk_event_box_new ();
	
	gtk_widget_set_usize(event_box, 320, 224);
	gtk_box_pack_start (GTK_BOX (vbox), event_box, FALSE, FALSE, 0);
	gtk_widget_show (event_box);
	gtk_widget_realize(event_box);

	status_bar = gtk_statusbar_new ();      
	gtk_box_pack_end (GTK_BOX (vbox), status_bar, FALSE, FALSE, 0);
	gtk_widget_show (status_bar);

	context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR (status_bar), "Statusbar example");
	gtk_statusbar_push (GTK_STATUSBAR (status_bar), context_id, "coin coin");
	
	gtk_window_set_resizable( GTK_WINDOW(window), FALSE);
	gtk_widget_show (window);

	hide = FALSE;
	sprintf(SDL_windowhack,"SDL_WINDOWID=%ld", GDK_WINDOW_XWINDOW(GTK_WIDGET(event_box)->window));
	putenv(SDL_windowhack);

	saturn = NULL;
	cd = new DummyCDDrive();
	
	yab_main = fonction;
	gtk_main ();
}

void yui_quit() {
	gtk_main_quit();
	if (saturn != NULL) delete(saturn);
}

void yui_fps(int fps) {
	char *text;
	gtk_statusbar_pop(GTK_STATUSBAR(status_bar), context_id);
	text = g_strdup_printf ("%d fps", fps);;
	gtk_statusbar_push( GTK_STATUSBAR(status_bar), context_id, text );
	g_free (text);
}

void yui_hack(void) {
	sprintf(SDL_windowhack,"SDL_WINDOWID=%ld", GDK_WINDOW_XWINDOW(GTK_WIDGET(event_box)->window));
	putenv(SDL_windowhack);
}

void yui_hide_show(void) {
	hide = !hide;
	if (hide) {
		gtk_widget_hide(menu_bar);
		gtk_widget_hide(status_bar);
	}
	else {
		gtk_widget_show(menu_bar);
		gtk_widget_show(status_bar);
	}
}

const char * yui_bios(void) {
	return bios;
}

static void set_bios( GtkWidget        *w, GtkFileSelection *fs ) {
	bios = g_strdup(gtk_file_selection_get_filename (GTK_FILE_SELECTION (fs)));
	gtk_widget_destroy( GTK_WIDGET(fs) );
}

void yui_choose_bios(void) {
	GtkWidget *filew;
	    
	filew = gtk_file_selection_new ("File selection");
		    
	g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (filew)->ok_button), "clicked", G_CALLBACK (set_bios), (gpointer) filew);
			    
	g_signal_connect_swapped (G_OBJECT (GTK_FILE_SELECTION (filew)->cancel_button), "clicked", G_CALLBACK (gtk_widget_destroy), G_OBJECT (filew));
			        
	gtk_widget_show (filew);
}

static void set_cdrom(GtkWidget *w, GtkFileSelection *fs ) {
	delete cd;
	cd = new LinuxCDDrive(g_strdup(gtk_file_selection_get_filename (GTK_FILE_SELECTION (fs))));
	gtk_widget_destroy( GTK_WIDGET(fs) );
}

static void set_iso(GtkWidget *w, GtkFileSelection *fs) {
	delete cd;
	cd = new ISOCDDrive(g_strdup(gtk_file_selection_get_filename (GTK_FILE_SELECTION (fs))));
	gtk_widget_destroy(GTK_WIDGET(fs));
}

void yui_choose_cdrom(void) {
	GtkWidget *filew = gtk_file_selection_new ("File selection");
		    
	g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (filew)->ok_button), "clicked", G_CALLBACK (set_cdrom), (gpointer) filew);
			    
	g_signal_connect_swapped (G_OBJECT (GTK_FILE_SELECTION (filew)->cancel_button), "clicked", G_CALLBACK (gtk_widget_destroy), G_OBJECT (filew));
			        
	gtk_widget_show (filew);
}

void yui_choose_iso(void) {
	GtkWidget *filew = gtk_file_selection_new("File selection");
	g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(filew)->ok_button), "clicked", G_CALLBACK(set_iso), (gpointer)filew);
	g_signal_connect_swapped(G_OBJECT(GTK_FILE_SELECTION(filew)->cancel_button), "clicked", G_CALLBACK(gtk_widget_destroy), G_OBJECT(filew));
	gtk_widget_show(filew);
}

static void set_binary( GtkWidget * w, GtkFileSelection * fs ) {
	binary = g_strdup(gtk_file_selection_get_filename (GTK_FILE_SELECTION (fs)));
	saturn->loadExec(binary, 0x6004000);
	gtk_widget_destroy( GTK_WIDGET(fs) );
}

void yui_choose_binary(void) {
	 GtkWidget *filew;

	filew = gtk_file_selection_new ("File selection");

	g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (filew)->ok_button), "clicked", G_CALLBACK (set_binary), (gpointer) filew);

	g_signal_connect_swapped (G_OBJECT (GTK_FILE_SELECTION (filew)->cancel_button), "clicked", G_CALLBACK (gtk_widget_destroy), G_OBJECT (filew));

	gtk_widget_show (filew);
}

CDInterface *yui_cd(void) {
	return cd;
}

void yui_coin_coin(void) {
	try {
		saturn = new SaturnMemory();
	}
	catch(Exception e) {
		cerr << e << endl;
		exit(1);
	}
	g_idle_add((gboolean (*)(void*)) yab_main, saturn);
}

const char * yui_mpegrom(void) {
	return NULL;
}

unsigned char yui_region(void) {
	return 0;
}

const char * yui_saveram(void) {
	return NULL;
}

void yui_errormsg(Exception error, SuperH sh2opcodes) {
   cerr << error << endl;
   cerr << sh2opcodes << endl;
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


void yui_about(void) {
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

    list = generate_credit_list(credit_text, TRUE);
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

static GtkWidget *
generate_credit_list(const gchar * text[], gboolean sec_space)
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
