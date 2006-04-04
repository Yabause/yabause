//
//  Controller.m
//  Yabausemac
//
//  Created by Michele Balistreri on 29/03/06.
//  Copyright 2006 Michele Balistreri. All rights reserved.
//

#import "Controller.h"
#import <SDL/SDL.h>
#import "yui.h"

NSString *BIOSPath = @"values.BIOSPath";
NSString *ISOPath = @"values.ISOPath";
NSString *CDOrISO = @"values.CDOrISO";
NSString *EnableSound = @"values.EnableSound";
NSString *DoubleSize = @"values.DoubleSize";

@implementation Controller

- (void)setupDirectories
{
	// Other directories will be created for cartidges and BIOS files, expansions roms and PAR rom
	NSString *mainDirectory = [[[NSHomeDirectory() stringByAppendingPathComponent:@"Library"] stringByAppendingPathComponent:@"Application Support"] stringByAppendingPathComponent:@"Yabause"];
	NSFileManager *fileManager = [NSFileManager defaultManager];
	[fileManager createDirectoryAtPath:mainDirectory attributes:nil];
	YuiSetBackupRamFilename([[mainDirectory stringByAppendingPathComponent:@"System Memory.ram"] UTF8String]);
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
	[self setupDirectories];
	
	drivesManager = [[CDDrivesManager alloc] init];
	[drivesManagerController setContent:drivesManager];
	[drivesManager addObserver:self forKeyPath:@"deviceCount" options:NSKeyValueObservingOptionNew context:NULL];	
	
	if(![drivesManager deviceCount])
		[sharedUserDefaults setValue:[NSNumber numberWithInt:1] forKeyPath:CDOrISO];
}

- (void)applicationWillTerminate:(NSNotification *)aNotification
{
	YuiQuit();
}

- (IBAction)startNewGame:(id)sender
{
	if(YuiIsRunning())
		return;
	
	YuiSetBiosFilename([[sharedUserDefaults valueForKeyPath:BIOSPath] UTF8String]);
	
	if([[sharedUserDefaults valueForKeyPath:CDOrISO] intValue] == 0)
		YuiSetCdromFilename([[drivesManager devicePathWithName:[driveSelection titleOfSelectedItem]] UTF8String]);
	else
		YuiSetIsoFilename([[sharedUserDefaults valueForKeyPath:ISOPath] UTF8String]);
	
	YuiSetSoundEnable([[sharedUserDefaults valueForKeyPath:EnableSound] intValue]);
	
	if([[sharedUserDefaults valueForKeyPath:DoubleSize] intValue])
		YuiSetDefaultWindowSize(640,448);
	else
		YuiSetDefaultWindowSize(320,224);

	YuiInit();
}

- (IBAction)selectBIOS:(id)sender
{
	NSOpenPanel *openPanel = [NSOpenPanel openPanel];
	
	if([openPanel runModal] == NSOKButton)
	{
		[sharedUserDefaults setValue:[openPanel filename] forKeyPath:BIOSPath];
	}
	
	else
		return;	
}

- (IBAction)selectISO:(id)sender
{
	NSOpenPanel *openPanel = [NSOpenPanel openPanel];
	
	if([openPanel runModal] == NSOKButton)
	{
		[sharedUserDefaults setValue:[openPanel filename] forKeyPath:ISOPath];
		[sharedUserDefaults setValue:[NSNumber numberWithInt:1] forKeyPath:CDOrISO];
	}
	
	else
		return;
}

- (IBAction)selectCDDrive:(id)sender
{
	[sharedUserDefaults setValue:[NSNumber numberWithInt:0] forKeyPath:CDOrISO];
}

// KVO
- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
	if([keyPath isEqual:@"deviceCount"])
	{
		if(![[change objectForKey:NSKeyValueChangeNewKey] intValue])
			[sharedUserDefaults setValue:[NSNumber numberWithInt:1] forKeyPath:CDOrISO];
	}
}

@end
