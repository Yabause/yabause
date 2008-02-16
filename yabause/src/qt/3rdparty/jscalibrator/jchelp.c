#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <unistd.h>
#include "../include/string.h"
#include "../include/disk.h"
#include "../include/prochandle.h"
#include "cdialog.h"
#include "config.h"


void JCHelp(
	const gchar *topic, GtkWidget *toplevel
);


/*
 *	Help Program Argument Types:
 *
 *	For the HELP_PROG_LOCATIONS list.
 */
typedef enum {
	HELP_PROG_INPUT_TYPE_PATH,
	HELP_PROG_INPUT_TYPE_URL
} help_prog_arg_type;

/*
 *	Help Programs List:
 *
 *	The list item format:
 *
 *	<program_path>,
 *	<arguments>,
 *	<help_prog_input_type>,
 *	<reserved>
 *
 *	The last set of four pointers in the list should be all NULL's.
 */
#define HELP_PROG_LOCATIONS	{		\
	/* GNOME Help Browser */		\
{	"/usr/bin/gnome-help-browser",		\
	"",					\
	HELP_PROG_INPUT_TYPE_URL,		\
	0					\
},						\
{	"/usr/local/bin/gnome-help-browser",	\
	"",					\
	HELP_PROG_INPUT_TYPE_URL,		\
	0					\
},						\
{	"/bin/gnome-help-browser",		\
	"",					\
	HELP_PROG_INPUT_TYPE_URL,		\
	0					\
},						\
	/* Netscape */				\
{	"/usr/bin/netscape",			\
	"",					\
	HELP_PROG_INPUT_TYPE_URL,		\
	0					\
},						\
{	"/usr/local/bin/netscape",		\
	"",					\
	HELP_PROG_INPUT_TYPE_URL,		\
	0					\
},						\
{	"/bin/netscape",			\
	"",					\
	HELP_PROG_INPUT_TYPE_URL,		\
	0					\
},						\
	/* Mozilla */				\
{	"/usr/bin/mozilla",			\
	"",					\
	HELP_PROG_INPUT_TYPE_URL,		\
	0					\
},						\
{	"/usr/local/bin/mozilla",		\
	"",					\
	HELP_PROG_INPUT_TYPE_URL,		\
	0					\
},						\
{	"/bin/mozilla",				\
	"",					\
	HELP_PROG_INPUT_TYPE_URL,		\
	0					\
},						\
	/* Lynx */				\
{	"/usr/X11R6/bin/nxterm",		\
	"-e lynx",				\
	HELP_PROG_INPUT_TYPE_URL,		\
	0					\
},						\
{	"/usr/X11R6/bin/xterm",			\
	"-e lynx",				\
	HELP_PROG_INPUT_TYPE_URL,		\
	0					\
},						\
	/* End */				\
{	NULL, NULL, 0, 0			\
}						\
}
typedef struct {
	const gchar	*prog;
	const gchar	*arg;
	help_prog_arg_type	arg_type;
	gint		reserved;
} help_prog_struct;


/*
 *	Help Topics & Help File Locations List:
 */
#define HELP_FILE_LOCATIONS {					\
{ "Contents",		"help/index.html",		0 },	\
{ "Introduction",	"help/introduction.html",	0 },	\
{ "How To Calibrate",	"help/how_to_calibrate.html",	0 },	\
{ NULL,			NULL,				0 }	\
}
typedef struct {
	const gchar	*topic;
	const gchar	*file;
	gint		reserved;
} help_file_struct;


#define ATOI(s)         (((s) != NULL) ? atoi(s) : 0)
#define ATOL(s)         (((s) != NULL) ? atol(s) : 0)
#define ATOF(s)         (((s) != NULL) ? atof(s) : 0.0f)
#define STRDUP(s)       (((s) != NULL) ? g_strdup(s) : NULL)

#define MAX(a,b)        (((a) > (b)) ? (a) : (b))
#define MIN(a,b)        (((a) < (b)) ? (a) : (b))
#define CLIP(a,l,h)     (MIN(MAX((a),(l)),(h)))
#define STRLEN(s)       (((s) != NULL) ? strlen(s) : 0)
#define STRISEMPTY(s)   (((s) != NULL) ? (*(s) == '\0') : TRUE)

#define OBJEXISTS(p)		(((p) != NULL) ? (!access((p), F_OK)) : FALSE)
#define OBJEXECUTABLE(p)	(((p) != NULL) ? (!access((p), X_OK)) : FALSE)
#define OBJREADABLE(p)		(((p) != NULL) ? (!access((p), R_OK)) : FALSE)


/*
 *	Displays the help page specified by topic by calling the help
 *	browser.
 *
 *	If topic is NULL then the topic is automatically ste to
 *	"Contents".
 *
 *	Valid values for topic are defined in HELP_FILE_LOCATIONS.
 */
void JCHelp(
        const gchar *topic, GtkWidget *toplevel
)
{
	gint i;
	const gchar *s, *data_dir = JC_DATA_DIR;
	gchar *help_file;
	help_prog_struct *hp_ptr, help_prog_list[] = HELP_PROG_LOCATIONS,
				help_prog;
	help_file_struct *hf_ptr, help_file_list[] = HELP_FILE_LOCATIONS;

	/* If no topic is specified then set default topic */
	if(STRISEMPTY(topic))
	    topic = help_file_list[0].topic;
	if(STRISEMPTY(topic))
	    return;

	/* Get help file based on the specified topic */
	for(i = 0, hf_ptr = NULL;
	    help_file_list[i].topic != NULL;
	    i++
	)
	{
	    if(!g_strcasecmp(help_file_list[i].topic, topic))
	    {
		hf_ptr = &help_file_list[i];
		break;
	    }
	}
	s = (hf_ptr != NULL) ? hf_ptr->file : NULL;
	if(s != NULL)
	    help_file = STRDUP(PrefixPaths(data_dir, s));
	else
	    help_file = NULL;

	/* No help file matched for the specified topic? */
	if(help_file == NULL)
	{
	    gchar *buf = g_strdup_printf(
#if defined(PROG_LANGUAGE_SPANISH)
"Incapaz de encontrar el tema de ayuda \"%s\"."
#elif defined(PROG_LANGUAGE_FRENCH)
"Incapable de trouver \"%s\" de sujet d'aide."
#elif defined(PROG_LANGUAGE_GERMAN)
"Unfähig, hilfe thema \"%s\" zu finden."
#elif defined(PROG_LANGUAGE_ITALIAN)
"Incapace per trovare \"%s\" di soggetto di aiuto."
#elif defined(PROG_LANGUAGE_DUTCH)
"Onbekwaam hulp onderwerp \"%s\" te vinden."
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"Incapaz de achar \"%s\" de tema de ajuda."
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Maktesløs finne hjelpeemne \"%s\"."
#else
"Unable to find help topic \"%s\"."
#endif
		, topic
	    );
	    CDialogSetTransientFor(toplevel);
	    CDialogGetResponse(
#if defined(PROG_LANGUAGE_SPANISH)
"Ninguna Ayuda Disponible",
buf,
"El tema especificado de la ayuda no se encontró, para una lista de\n\
temas de ayuda por favor mirada en el contenido yendo a\n\
Ayudar->Contenido.\n",
#elif defined(PROG_LANGUAGE_FRENCH)
"Aucune Aide Disponible",
buf,
"Le sujet spécifié d'aide n'a pas été trouvé, pour une liste de\n\
sujets d'aide s'il vous plaît regard dans les contenus en allant\n\
pour Aider->Contenus.\n",
#elif defined(PROG_LANGUAGE_GERMAN)
"Keine Hilfe Verfügbar",
buf,
"Das angegebene hilfe thema wurde, für eine liste von hilfe\n\
themen bitte aussehen im inhalt durch der gehen nicht\n\
Hilfe->Themen.\n",
#elif defined(PROG_LANGUAGE_ITALIAN)
"Nessuno Aiuto Disponibile",
buf,
"Il soggetto di aiuto specificato non è stato trovato, per un\n\
elenco di soggetti di aiuto per favore sguardo nel contenuto da\n\
andare di Aiutare->Contenuto.\n",
#elif defined(PROG_LANGUAGE_DUTCH)
"Geen Hulp Verkrijgbaar",
buf,
"Het gespecificeerd hulp onderwerp werd door gaan, voor een lijst\n\
van hulp onderwerpen alstublieft blik in de inhoud niet\n\
Hulp->Gevonden.\n",
#elif defined(PROG_LANGUAGE_PORTUGUESE)
"A Nenhuma Ajuda Disponível",
buf,
"O tema especificado de ajuda não foi achado, para uma lista de\n\
temas de ajuda por favor olha no conteúdo por ir Ajudar->Conteúdo.\n",
#elif defined(PROG_LANGUAGE_NORWEGIAN)
"Ingen Hjelp Tilgjengelig",
buf,
"Det spesifiserte hjelpeemne fant ikke, for en liste av hjelpeemner\n\
behager titt i innholdet ved å dra Hjelpe->Innhold.\n",
#else
"No Help Available",
buf,
"The specified help topic was not found, for a list of help topics\n\
please look in the contents by going to Help->Contents.\n",
#endif
		CDIALOG_ICON_ERROR,
		CDIALOG_BTNFLAG_OK,
		CDIALOG_BTNFLAG_OK
	    );  
	    CDialogSetTransientFor(NULL);
	    g_free(buf);
	    return;
	}

	/* Help file exists and is readable? */
	if(!OBJREADABLE(help_file))
	{
	    gchar *buf = g_strdup_printf(
"Unable to find help documentation:\n\
\n\
    %s\n",
		help_file
	    );
	    CDialogSetTransientFor(toplevel);
	    CDialogGetResponse(
		"Help Display Failed",
		buf,
		NULL,
		CDIALOG_ICON_WARNING,
		CDIALOG_BTNFLAG_OK,
		CDIALOG_BTNFLAG_OK
	    );
	    CDialogSetTransientFor(NULL);
	    g_free(buf);

	    g_free(help_file);
	    return;
	}


	/* Search for a help program to use
	 *
	 * First check if the help program is specified in an
	 * environment variable
	 *
	 * If no help program is specified then use the first
	 * available help program found to exist in the help programs
	 * list
	 */

	/* Check if the HELPBROWSER environment variable is set */
	s = g_getenv(ENV_VAR_NAME_HELPBROWSER);
	if(STRISEMPTY(s))
	{
	    /* HELPBROWSER not set, fall back to the BROWSER environment
	     * variable
	     */
	    s = g_getenv(ENV_VAR_NAME_BROWSER);
	    if(STRISEMPTY(s))
	    {
		CDialogSetTransientFor(toplevel);
		CDialogGetResponse(
"Help Display Warning",
"Help browser environment variables not set:\n\
\n\
    " ENV_VAR_NAME_HELPBROWSER "\n\
    " ENV_VAR_NAME_BROWSER "\n\
\n\
(Click on \"Help\" for more information)\n",
"The environment variable which specifies which help browser to use\n\
to display the help documentation is not set. To set this you need to\n\
edit your shell configuration file (this is usually the file named\n\
.bashrc in your home directory) and add the lines:\n\
\n\
" ENV_VAR_NAME_BROWSER "=/usr/bin/mozilla\n\
export " ENV_VAR_NAME_BROWSER "\n\
" ENV_VAR_NAME_HELPBROWSER "=/usr/bin/mozilla\n\
export " ENV_VAR_NAME_HELPBROWSER "\n\
\n\
Save the file and then restart this application.\n",
		    CDIALOG_ICON_WARNING,
		    CDIALOG_BTNFLAG_OK,
		    CDIALOG_BTNFLAG_OK
		);
		CDialogSetTransientFor(NULL);
	    }
	}
	/* Environment variable set? */
	if(!STRISEMPTY(s))
	{
	    /* Found help program set from environment variable */
	    help_prog.prog = s;
	    help_prog.arg = "";
	    help_prog.arg_type = HELP_PROG_INPUT_TYPE_URL;
	    help_prog.reserved = 0;
	    hp_ptr = &help_prog;
	}
	else
	{
	    /* No run time help program specified, so look for help
	     * program specified in the help programs list
	     */
	    hp_ptr = NULL;
	    for(i = 0; help_prog_list[i].prog != NULL; i++)
	    {
		if(OBJEXECUTABLE(help_prog_list[i].prog))
		{
		    hp_ptr = &help_prog_list[i];
		    break;
		}
	    }
	}

	/* Found a help program? */
	if(hp_ptr != NULL)
	{
	    pid_t p;
	    gchar *cmd = NULL;

	    /* Format command */
	    cmd = strcatalloc(cmd, hp_ptr->prog);
	    if(hp_ptr->arg != NULL)
	    {
		cmd = strcatalloc(cmd, " ");
		cmd = strcatalloc(cmd, hp_ptr->arg);
	    }
	    switch(hp_ptr->arg_type)
	    {
	      case HELP_PROG_INPUT_TYPE_URL:
		cmd = strcatalloc(cmd, " file://");
		cmd = strcatalloc(cmd, help_file);
		break;

	      default:	/* HELP_PROG_INPUT_TYPE_PATH */
		cmd = strcatalloc(cmd, " ");
		cmd = strcatalloc(cmd, help_file);
		break;
	    }

	    /* Execute help viewer */
	    p = Exec(cmd);
	    if(p == 0)
	    {
		/* Unable to execute help viewer */
		gchar *buf = g_strdup_printf(
"Unable to execute help viewer command:\n\
\n\
    %s\n",
		    cmd
		);
		CDialogSetTransientFor(toplevel);
		CDialogGetResponse(
"Help Display Failed",
		    buf,
"An error was encountered while attempting to run the help\n\
viewer to display the requested document. Please verify that\n\
the help viewer and the requested document are both\n\
installed properly.",
		    CDIALOG_ICON_ERROR,
		    CDIALOG_BTNFLAG_OK,
		    CDIALOG_BTNFLAG_OK
		);
		CDialogSetTransientFor(NULL);
		g_free(buf);
	    }

	    /* Delete command */
	    g_free(cmd);
	}
	else
	{
	    gchar *buf = g_strdup_printf(
"Unable to find a help viewer to display help documentation:\n\
\n\
    %s\n",
		help_file
	    );
	    CDialogSetTransientFor(toplevel);
	    CDialogGetResponse(
		"Help Display Failed",
		buf,
		NULL,
		CDIALOG_ICON_WARNING,
		CDIALOG_BTNFLAG_OK,
		CDIALOG_BTNFLAG_OK
	    );
	    CDialogSetTransientFor(NULL);
	    g_free(buf);
	}

	g_free(help_file);
}
