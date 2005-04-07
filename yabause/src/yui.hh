#include "saturn_memory.hh"

class CDInterface;

/* this function should be the "main" function of the program,
 * the parameter is a function pointer, this function wants a
 * SaturnMemory as parameter, the interface must create it
 * (ie: saturn = new SaturnMemory();) after the user has choose
 * the bios file and all others possible configurations.
 * The function should be called as often as possible */
void	yui_init(int (*)(void *));

/* quit yui_init */
void	yui_quit(void);

/* hide or show the interface */
void	yui_hide_show(void);

/* returns a non NULL BIOS filename */
const char *	yui_bios(void);

/* returns a cd device name or NULL if no cd device is used */
const char *	yui_cdrom(void);

/* returns an instance derived from CDInterface, NULL if nothing instantiated */
CDInterface *yui_cd(void);

/* returns the region of system or 0 if autodetection is used */
unsigned char yui_region(void);

/* returns a save ram filename. Can be set to NULL if not present */
const char * yui_saveram(void);

/* returns a mpeg rom filename. Can be set to NULL if not present */
const char * yui_mpegrom(void);

/* If Yabause encounters any fatal errors, it sends the error text to this function */
void yui_errormsg(Exception error, SuperH sh2opcodes);

/* Sets bios filename in the yui - used to specify bios from the commandline */
void yui_set_bios_filename(const char *);

/* Sets ISO filename in the yui - used to specify an ISO from the commandline */
void yui_set_iso_filename(const char *);

