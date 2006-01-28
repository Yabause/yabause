//
//  PreferenceController.m
//  Yabausemac
//
//  Created by Jerome Vernet on 10/01/06.
//  Copyright 2006 __MyCompanyName__. All rights reserved.
//

#import "PreferenceController.h"

// preferences dictionary keys
NSString *YabBiosFile = @"YabauseBiosFile";
NSString *YabIsoFile = @"YabauseIsoFile";
NSString *YabSndEnable = @"YabauseSoundEnable";
NSString *YabCDFile = @"YabauseCDFile";
NSString *YabCDOrISO = @"YabauseCDOrISO"; // 0=ISO, 1=CD

//Some useful string lazy am I
char b[1024];
char i[1024];

@implementation PreferenceController
- (id) init {

if((self = [self initWithWindowNibName: @"Preferences"]) != nil) {
      [self setWindowFrameAutosaveName: @"PrefWindows"];
      [self setShouldCascadeWindows: NO];
      return self;
   }
  return nil;

}

- (void)windowDidLoad {
	NSUserDefaults *defaults;
	NSString *astring;
	int i;
	
// Fill field with values from Preferences
	NSLog(@"Fichier .nib chargé.");
	
	defaults = [NSUserDefaults standardUserDefaults];
	NSLog(@"defaults...");
	astring=[defaults stringForKey:YabBiosFile];
	if(astring)
	{
		[biosFile setStringValue:astring];
	} else
	{
			[biosFile setStringValue:@"./ROMs/saturn.rom"];
			[[NSUserDefaults standardUserDefaults] setObject:@"./ROMs/saturn.rom" forKey:YabBiosFile];
			NSLog(@"Pas de bios ??");
	}
	
	NSLog(@"BIOS:");NSLog(astring);
	astring=[defaults stringForKey:YabIsoFile];
	if(astring)
	{
		[isoFile setStringValue:astring];
	} else
	{
			[biosFile setStringValue:@"./ISOs/game.iso"];
			[[NSUserDefaults standardUserDefaults] setObject:@"./ISOs/game.iso" forKey:YabIsoFile];
			NSLog(@"Pas de ISO ??");
	}
	
	NSLog(@"ISO:");NSLog(astring);
	
	astring=[defaults stringForKey:YabCDFile];
	if(astring)
	{
		[CDRomPath setStringValue:astring];
	} else
	{
			[CDRomPath setStringValue:@"/dev/disk1"];
			[[NSUserDefaults standardUserDefaults] setObject:@"/dev/disk1" forKey:YabCDFile];
			NSLog(@"Pas de CDROM ?");
	}
	NSLog(@"CDPath:");NSLog(astring);
	
	
	i=[defaults integerForKey:YabCDOrISO];
	
	[CDISO selectCellWithTag:i];
	
	
	[soundEnabled setState:[defaults boolForKey:YabSndEnable]];
	
	NSLog(@"Préférences chargees");
	[[NSUserDefaults standardUserDefaults] synchronize];
	NSLog(@"Preferences sauvees peut etre");
}

//Actions

-(IBAction)changeBiosFile:(id)sender
{
	NSString *st=[sender stringValue];
	
	[[NSUserDefaults standardUserDefaults] setObject:st forKey:YabBiosFile];
	
	
	NSLog(@"BiosFile Modifie: %s",b);
}

-(IBAction)changeIsoFile:(id)sender
{
	NSString *st=[sender stringValue];
	[[NSUserDefaults standardUserDefaults] setObject:st forKey:YabIsoFile];
	
	NSLog(@"isoFile Modifié");
}

-(IBAction)changeSound:(id)sender
{
	[[NSUserDefaults standardUserDefaults] setBool:[sender state] forKey:YabSndEnable];
	
	NSLog(@"Sound Modifié");
}

-(IBAction)changeCDRom:(id)sender
{
	NSString *st=[sender stringValue];
	[[NSUserDefaults standardUserDefaults] setObject:st forKey:YabCDFile];
	NSLog(@"CROM Modifié");
}

-(IBAction)chooseBiosFile:(id)sender
{
 NSOpenPanel *oPanel = [NSOpenPanel openPanel];
	
    [oPanel setAllowsMultipleSelection:NO];
    [oPanel setTitle:@"Open Bios File..."];
    if ([oPanel runModalForDirectory:nil file:nil types:nil] == NSOKButton) 
    {
		
		strcpy(b, [[[oPanel filenames] objectAtIndex:0] UTF8String]);
		
		[biosFile setStringValue:[NSString stringWithFormat:@"%s",b]];
		
       // return 1;
    }
    // return 0;
}

-(IBAction)chooseIsoFile:(id)sender
{
	NSOpenPanel *oPanel = [NSOpenPanel openPanel];
	
    [oPanel setAllowsMultipleSelection:NO];
    [oPanel setTitle:@"Open Iso File..."];
    if ([oPanel runModalForDirectory:nil file:nil types:nil] == NSOKButton) 
    {
		
		strcpy(i,[[[oPanel filenames] objectAtIndex:0] UTF8String]);
		printf("Iso:%s\n",i);
		[isoFile setStringValue:[NSString stringWithFormat:@"%s",i]];
		
    }
}

-(IBAction)changeISOCD:(id)sender
{

	int i=[[sender selectedCell] tag]; // Get tag
	
	[[NSUserDefaults standardUserDefaults] setInteger:i forKey:YabCDOrISO];
	
	
	NSLog(@"changeIsoCD");
	
}

-(IBAction)apply:(id)sender


{
	int ci;
	
	// Save prefs
	[[NSUserDefaults standardUserDefaults] synchronize];
	
	//Apply sound Changes
	YuiSetSoundEnable([soundEnabled state]);
	
	// Apply BIOS changes
	strcpy(b, [[biosFile stringValue] UTF8String]);
	YuiSetBiosFilename(b);
	
	ci=[[CDISO selectedCell] tag]; // Get tag
	
	if(ci==0)
	{
		// Iso Selected
		strcpy(i, [[isoFile stringValue] UTF8String]);
		printf("ISO Selected:%s\n",i);
		YuiSetIsoFilename(i);
	} else
	{
		strcpy(i, [[CDRomPath stringValue] UTF8String]);
		printf("CDROM Selected:%s\n",i);
		YuiSetCdromFilename(i);
	}

	NSLog(@"Applied Successfully !");
}


@end
