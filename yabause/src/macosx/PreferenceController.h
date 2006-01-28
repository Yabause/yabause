//
//  PreferenceController.h
//  Yabausemac
//
//  Created by Jerome Vernet on 10/01/06.
//  
//

#import <Cocoa/Cocoa.h>

// preferences dictionary keys
extern NSString *YabBiosFile;
extern NSString *YabIsoFile;
extern NSString *YabSndEnable;
extern NSString *YabCDFile;
extern NSString	*YabCDOrISO; 

// 0=ISO, 1=CD


@interface PreferenceController : NSWindowController {
IBOutlet NSTextField	*biosFile;
IBOutlet NSTextField	*isoFile;
IBOutlet NSTextField	*CDRomPath;
IBOutlet NSMatrix		*CDISO;
// IBOutlet NSButtonCell	*ISO;
IBOutlet NSButton		*soundEnabled;


}
-(IBAction)changeBiosFile:(id)sender;
-(IBAction)changeIsoFile:(id)sender;
-(IBAction)chooseBiosFile:(id)sender;
-(IBAction)chooseIsoFile:(id)sender;

-(IBAction)changeSound:(id)sender;
-(IBAction)changeCDRom:(id)sender;
-(IBAction)changeISOCD:(id)sender;
-(IBAction)apply:(id)sender;


@end
