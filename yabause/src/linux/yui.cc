#include "../yui.hh"
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <gdk/gdkx.h>
#include <SDL/SDL.h>
#include "../memory.hh"

void yui_choose_bios(void);
void yui_choose_cdrom(void);
void yui_coin_coin(void);

static GtkItemFactoryEntry entries[] = {
	{ "/_Yabause", NULL,      NULL,         0, "<Branch>" },
	{ "/Yabause/tear1", NULL,      NULL,         0, "<Tearoff>" },
	{ "/Yabause/_Open BIOS...",     "<CTRL>B", yui_choose_bios,     1, "<Item>" },
	{ "/Yabause/_Open CDROM...", "<CTRL>C", yui_choose_cdrom,    1, "<Item>" },
	{ "/Yabause/_Run...", "<CTRL>R", yui_coin_coin,    1, "<Item>" },
	{ "/Yabause/sep1",     NULL,      NULL,         0, "<Separator>" },
	{ "/Yabause/_Quit",    "<CTRL>Q", yui_quit, 0, "<StockItem>", GTK_STOCK_QUIT } };

GtkWidget *status_bar;
gint context_id;
GtkWidget *window;
GtkWidget *event_box;
GtkWidget *menu_bar;
gboolean hide;

char *bios;
char *cdrom;
SaturnMemory *saturn;
int (*yab_main)(void *);

char SDL_windowhack[32];

static gboolean key_press( GtkWidget *widget, GdkEvent *event, gpointer data) {
	SDL_Event eve;
	eve.type = eve.key.type = SDL_KEYDOWN;
	eve.key.state = SDL_PRESSED;
	eve.key.keysym.sym = (SDLKey) event->key.keyval;
	SDL_PushEvent(&eve);
}

static gboolean key_release( GtkWidget *widget, GdkEvent *event, gpointer data) {
	SDL_Event eve;
	eve.type = eve.key.type = SDL_KEYUP;
	eve.key.state = SDL_RELEASED;
	eve.key.keysym.sym = (SDLKey) event->key.keyval;
	SDL_PushEvent(&eve);
}

void yui_init(int (*fonction)(void *)) {
	GtkWidget *vbox;
	GtkItemFactory *gif;
	char *text;

	gtk_init (NULL, NULL);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	g_signal_connect(G_OBJECT (window), "key_press_event", G_CALLBACK (key_press), NULL);
	g_signal_connect(G_OBJECT (window), "key_release_event", G_CALLBACK (key_release), NULL);
	
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (window), vbox);
	gtk_widget_show (vbox);

	text = g_strdup_printf ("Yabause %s", VERSION);
	gtk_window_set_title(GTK_WINDOW (window), text);
	g_free(text);

	gif = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<YabauseMain>", NULL);
	gtk_item_factory_create_items(gif, 7, entries, NULL);

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

	bios = NULL;
	saturn = NULL;

	/*
	saturn = new SaturnMemory();
	g_idle_add((gboolean (*)(void*)) fonction, saturn);
	*/
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

char * yui_bios(void) {
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

static void set_cdrom( GtkWidget        *w, GtkFileSelection *fs ) {
	cdrom = g_strdup(gtk_file_selection_get_filename (GTK_FILE_SELECTION (fs)));
	gtk_widget_destroy( GTK_WIDGET(fs) );
}

void yui_choose_cdrom(void) {
	GtkWidget *filew;
	    
	filew = gtk_file_selection_new ("File selection");
		    
	g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (filew)->ok_button), "clicked", G_CALLBACK (set_cdrom), (gpointer) filew);
			    
	g_signal_connect_swapped (G_OBJECT (GTK_FILE_SELECTION (filew)->cancel_button), "clicked", G_CALLBACK (gtk_widget_destroy), G_OBJECT (filew));
			        
	gtk_widget_show (filew);
}

char * yui_cdrom(void) {
	return cdrom;
}

void yui_coin_coin(void) {
	saturn = new SaturnMemory();
	//saturn->start();
	g_idle_add((gboolean (*)(void*)) yab_main, saturn);
}

char * yui_mpegrom(void) {
	return NULL;
}

unsigned char yui_region(void) {
	return 0;
}

char * yui_saveram(void) {
	return NULL;
}
