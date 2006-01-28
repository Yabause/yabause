/*   SDLMain.m - main entry point for our Cocoa-ized SDL app
       Initial Version: Darrell Walisser <dwaliss1@purdue.edu>
       Non-NIB-Code & other changes: Max Horn <max@quendi.de>

    Feel free to customize this file to suit your needs
*/
/* Yabause SDLMain 
	0.6.0 Beta
	J. Vernet
	16/01/2006	*/

#import "SDL.h"
#import "SDLMain.h"
#import <sys/param.h> /* for MAXPATHLEN */
#import <unistd.h>
#import "PreferenceController.h"

/* Use this flag to determine whether we use SDLMain.nib or not */
#define		SDL_USE_NIB_FILE	1


static int    gArgc;
static char  **gArgv;
static BOOL   gFinderLaunch;

int SDLRunning;

//Some useful string lazy am I
static char b[1024]; // Bios
static char i[1024]; // Iso
static char cd[1024]; // CD device Name, usually /dev/disk1
static int s;		// Sound
static int icd;		// Iso or CD iso=0 CD=1


void YuiSetBiosFilename(const char * biosfilename);

#if SDL_USE_NIB_FILE
/* A helper category for NSString */
@interface NSString (ReplaceSubString)
- (NSString *)stringByReplacingRange:(NSRange)aRange with:(NSString *)aString;
@end
#else
/* An internal Apple class used to setup Apple menus */
@interface NSAppleMenuController:NSObject {}
- (void)controlMenu:(NSMenu *)aMenu;
@end
#endif

@interface SDLApplication : NSApplication
@end

@implementation SDLApplication
/* Invoked from the Quit menu item */
- (void)terminate:(id)sender
{
    /* Post an SDL_QUIT event */
	NSLog(@"Want to terminate...");
    if(SDLRunning)
	{
		SDL_Event event;
		event.type = SDL_QUIT;
		SDL_PushEvent(&event);
	}
	[defaults synchronize];
	NSLog(@"...OK");
	exit(0);
}
@end


/* The main class of the application, the application's delegate */
@implementation SDLMain

+(void) initialize
{
	
	
	defaults = [[NSUserDefaults standardUserDefaults] retain];

	if(![defaults objectForKey:YabBiosFile])
	{
		// No preference file
		NSLog(@"Pas de prefs par defaut");
		NSMutableDictionary	*defaultValues = [NSMutableDictionary dictionary];
		[defaultValues setObject:@"./saturn.rom" forKey:YabBiosFile];
		[defaultValues setObject:@"./game.iso" forKey:YabIsoFile];
		[defaultValues setObject:@"/dev/disk1" forKey:YabCDFile];
		[defaultValues setObject:[NSNumber numberWithInt:0] forKey:YabCDOrISO];
		[defaultValues setObject:[NSNumber numberWithBool:YES] forKey:YabSndEnable];
		[defaults registerDefaults: defaultValues];
		
		[defaults synchronize];
		
		
		NSRunAlertPanel (@"Check Preferences !",@"",@"OK",@"" , nil);

	}
	else
	{
		NSLog(@"Preferences par defaut existent");
		// Read Preferences
		// Read BIOS File
		strcpy(b, [[defaults objectForKey:YabBiosFile] UTF8String]);
		printf("Bios:%s\n",b);
		YuiSetBiosFilename(b);
		
		// Read if ISO or Real CD
		icd=[defaults integerForKey:YabCDOrISO];
		if(icd)
		{
			NSLog(@"Real CD selected");
			strcpy(cd, [[defaults objectForKey:YabIsoFile] UTF8String]);
			YuiSetCdromFilename(cd);
		} else
		{
			NSLog(@"ISO Selected");
			strcpy(i, [[defaults objectForKey:YabIsoFile] UTF8String]);
			
			// Set Iso File Name for Yabause
			YuiSetIsoFilename(i);
		}
		
		s=[defaults boolForKey:YabSndEnable];
		YuiSetSoundEnable(s);
	}
	
	SDLRunning=0;
}


-(int) openBios:(id)sender
{

}

-(void) quit:(id)sender
{
	/* Post an SDL_QUIT event */
	NSLog(@"Want to quit...");
    if(SDLRunning)
	{
		SDL_Event event;
		event.type = SDL_QUIT;
		SDL_PushEvent(&event);
	}
	[defaults synchronize];
	NSLog(@"...OK");
	exit(0);
}

- (IBAction)prefsMenu:(id)sender
{
	
    if (!preferenceController) {
		preferenceController=[[PreferenceController alloc] init];
		}
	if(!preferenceController)
	{
	 NSLog(@"Preferences didn't load");
	}
	
	[preferenceController showWindow:self];
			
}

- (IBAction)newGame:(id)sender
{
	
	int status;    
    NSRunAlertPanel (@"Get ready to blow up some... stuff!", 
        @"Click OK to begin total carnage. Click Cancel to prevent total carnage.", 	        		@"OK", @"Cancel", nil);
		 /* Hand off to main application code */
    SDLRunning=1;
	status = SDL_main (gArgc, gArgv);
	//NSLog("@Running...");
	//SDLRunning=1;
	
    /* We're done, thank you for playing */
    exit(status);
}

- (IBAction)openGame:(id)sender
{
    NSString *path = nil;
    NSOpenPanel *openPanel = [ NSOpenPanel openPanel ];
    
    if ( [ openPanel runModalForDirectory:nil
             file:@"SavedGame" types:nil ] ) {
             
        path = [ [ openPanel filenames ] objectAtIndex:0 ];
    }
    
    printf ("open game: %s\n", [ path cString ]);
}

- (IBAction)saveGame:(id)sender
{
    NSString *path = nil;
    NSSavePanel *savePanel = [ NSSavePanel savePanel ];
    
    if ( [ savePanel runModalForDirectory:nil
           file:@"SaveGameFile" ] ) {
            
        path = [ savePanel filename ];
    }
    
    printf ("save game: %s\n", [ path cString ]);
}

- (IBAction)saveGameAs:(id)sender
{
    printf ("save game as\n");
}

- (IBAction)help:(id)sender
{
    NSRunAlertPanel (@"Oh help, where have ye gone?", 
        @"Sorry, there is no help available.\n\nThis message brought to you by We Don't Document, Inc.\n\n", @"Rats", @"Good, I never read it anyway", nil);
}


/* Set the working directory to the .app's parent directory */
- (void) setupWorkingDirectory:(BOOL)shouldChdir
{
    char parentdir[MAXPATHLEN];
    char *c;
    
    strncpy ( parentdir, gArgv[0], sizeof(parentdir) );
    c = (char*) parentdir;

    while (*c != '\0')     /* go to end */
        c++;
    
    while (*c != '/')      /* back up to parent */
        c--;
    
    *c++ = '\0';             /* cut off last part (binary name) */
  
    if (shouldChdir)
    {
      assert ( chdir (parentdir) == 0 );   /* chdir to the binary app's parent */
      assert ( chdir ("../../../") == 0 ); /* chdir to the .app's parent */
    }
}

#if SDL_USE_NIB_FILE

/* Fix menu to contain the real app name instead of "SDL App" */
- (void)fixMenu:(NSMenu *)aMenu withAppName:(NSString *)appName
{
    NSRange aRange;
    NSEnumerator *enumerator;
    NSMenuItem *menuItem;

    aRange = [[aMenu title] rangeOfString:@"SDL App"];
    if (aRange.length != 0)
        [aMenu setTitle: [[aMenu title] stringByReplacingRange:aRange with:appName]];

    enumerator = [[aMenu itemArray] objectEnumerator];
    while ((menuItem = [enumerator nextObject]))
    {
        aRange = [[menuItem title] rangeOfString:@"SDL App"];
        if (aRange.length != 0)
            [menuItem setTitle: [[menuItem title] stringByReplacingRange:aRange with:appName]];
        if ([menuItem hasSubmenu])
            [self fixMenu:[menuItem submenu] withAppName:appName];
    }
    [ aMenu sizeToFit ];
}

#else

void setupAppleMenu(void)
{
    /* warning: this code is very odd */
    NSAppleMenuController *appleMenuController;
    NSMenu *appleMenu;
    NSMenuItem *appleMenuItem;

    appleMenuController = [[NSAppleMenuController alloc] init];
    appleMenu = [[NSMenu alloc] initWithTitle:@""];
    appleMenuItem = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    
    [appleMenuItem setSubmenu:appleMenu];

    /* yes, we do need to add it and then remove it --
       if you don't add it, it doesn't get displayed
       if you don't remove it, you have an extra, titleless item in the menubar
       when you remove it, it appears to stick around
       very, very odd */
    [[NSApp mainMenu] addItem:appleMenuItem];
    [appleMenuController controlMenu:appleMenu];
    [[NSApp mainMenu] removeItem:appleMenuItem];
    [appleMenu release];
    [appleMenuItem release];
}

/* Create a window menu */
void setupWindowMenu(void)
{
    NSMenu		*windowMenu;
    NSMenuItem	*windowMenuItem;
    NSMenuItem	*menuItem;


    windowMenu = [[NSMenu alloc] initWithTitle:@"Window"];
    
    /* "Minimize" item */
    menuItem = [[NSMenuItem alloc] initWithTitle:@"Minimize" action:@selector(performMiniaturize:) keyEquivalent:@"m"];
    [windowMenu addItem:menuItem];
    [menuItem release];
    
    /* Put menu into the menubar */
    windowMenuItem = [[NSMenuItem alloc] initWithTitle:@"Window" action:nil keyEquivalent:@""];
    [windowMenuItem setSubmenu:windowMenu];
    [[NSApp mainMenu] addItem:windowMenuItem];
    
    /* Tell the application object that this is now the window menu */
    [NSApp setWindowsMenu:windowMenu];

    /* Finally give up our references to the objects */
    [windowMenu release];
    [windowMenuItem release];
}

/* Replacement for NSApplicationMain */
void CustomApplicationMain (argc, argv)
{
    NSAutoreleasePool	*pool = [[NSAutoreleasePool alloc] init];
    SDLMain				*sdlMain;

    /* Ensure the application object is initialised */
    [SDLApplication sharedApplication];
    
    /* Set up the menubar */
    [NSApp setMainMenu:[[NSMenu alloc] init]];
    setupAppleMenu();
    setupWindowMenu();
    
    /* Create SDLMain and make it the app delegate */
    sdlMain = [[SDLMain alloc] init];
    [NSApp setDelegate:sdlMain];
    
    /* Start the main event loop */
    [NSApp run];
    
    [sdlMain release];
    [pool release];
}

#endif

/* Called when the internal event loop has just started running */
- (void) applicationDidFinishLaunching: (NSNotification *) note
{
    

    /* Set the working directory to the .app's parent directory */
    [self setupWorkingDirectory:gFinderLaunch];

#if SDL_USE_NIB_FILE
    /* Set the main menu to contain the real app name instead of "SDL App" */
    [self fixMenu:[NSApp mainMenu] withAppName:[[NSProcessInfo processInfo] processName]];
#endif

    /* Hand off to main application code */
 //   status = SDL_main (gArgc, gArgv);

    /* We're done, thank you for playing */
   // exit(status);
}

-(void)dealloc
{
	// Save prefs
	
	[defaults synchronize];
    [defaults release];
	
	[preferenceController release];
	[super dealloc];
}

@end // end SDLMain


@implementation NSString (ReplaceSubString)

- (NSString *)stringByReplacingRange:(NSRange)aRange with:(NSString *)aString
{
    unsigned int bufferSize;
    unsigned int selfLen = [self length];
    unsigned int aStringLen = [aString length];
    unichar *buffer;
    NSRange localRange;
    NSString *result;

    bufferSize = selfLen + aStringLen - aRange.length;
    buffer = NSAllocateMemoryPages(bufferSize*sizeof(unichar));
    
    /* Get first part into buffer */
    localRange.location = 0;
    localRange.length = aRange.location;
    [self getCharacters:buffer range:localRange];
    
    /* Get middle part into buffer */
    localRange.location = 0;
    localRange.length = aStringLen;
    [aString getCharacters:(buffer+aRange.location) range:localRange];
     
    /* Get last part into buffer */
    localRange.location = aRange.location + aRange.length;
    localRange.length = selfLen - localRange.location;
    [self getCharacters:(buffer+aRange.location+aStringLen) range:localRange];
    
    /* Build output string */
    result = [NSString stringWithCharacters:buffer length:bufferSize];
    
    NSDeallocateMemoryPages(buffer, bufferSize);
    
    return result;
}


@end



#ifdef main
#  undef main
#endif


/* Main entry point to executable - should *not* be SDL_main! */
int main (int argc, char **argv)
{

	NSLog(@"Yabause Start SDLMain.m main.c");
    /* Copy the arguments into a global variable */
    int i;
    
    /* This is passed if we are launched by double-clicking */
    if ( argc >= 2 && strncmp (argv[1], "-psn", 4) == 0 ) {
        gArgc = 1;
	gFinderLaunch = YES;
    } else {
        gArgc = argc;
	gFinderLaunch = NO;
    }
    gArgv = (char**) malloc (sizeof(*gArgv) * (gArgc+1));
    assert (gArgv != NULL);
    for (i = 0; i < gArgc; i++)
        gArgv[i] = argv[i];
    gArgv[i] = NULL;

#if SDL_USE_NIB_FILE
    [SDLApplication poseAsClass:[NSApplication class]];
    NSApplicationMain (argc, argv);
#else
    CustomApplicationMain (argc, argv);
#endif
    return 0;
}
