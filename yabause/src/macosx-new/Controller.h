//
//  Controller.h
//  Yabausemac
//
//  Created by Michele Balistreri on 29/03/06.
//  Copyright 2006 Michele Balistreri. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "CDDrivesManager.h"

@interface Controller : NSObject 
{
	IBOutlet NSUserDefaultsController *sharedUserDefaults;
	IBOutlet NSObjectController *drivesManagerController;
	IBOutlet NSPopUpButton *driveSelection;
	CDDrivesManager *drivesManager;
}

- (IBAction)startNewGame:(id)sender;
- (IBAction)selectBIOS:(id)sender;
- (IBAction)selectISO:(id)sender;
- (IBAction)selectCDDrive:(id)sender;

@end
