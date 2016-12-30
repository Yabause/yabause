//
//  GameViewController.m
//  uoyabause
//
//  Created by MiyamotoShinya on 2016/02/06.
//  Copyright © 2016年 devMiyax. All rights reserved.
//

#import <QuartzCore/QuartzCore.h>
#import "GameViewController.h"
#import <OpenGLES/ES2/glext.h>
@import GameController;

#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AudioToolbox/ExtendedAudioFile.h>
#import "GameRevealViewController.h"
#import "SaturnControllerKeys.h"
#import "KeyMapper.h"

/** @defgroup pad Pad
 *
 * @{
 */


#define CART_NONE            0
#define CART_PAR             1
#define CART_BACKUPRAM4MBIT  2
#define CART_BACKUPRAM8MBIT  3
#define CART_BACKUPRAM16MBIT 4
#define CART_BACKUPRAM32MBIT 5
#define CART_DRAM8MBIT       6
#define CART_DRAM32MBIT      7
#define CART_NETLINK         8
#define CART_ROM16MBIT       9

void PerKeyDown(unsigned int key);
void PerKeyUp(unsigned int key);
int start_emulation( int originx, int originy, int width, int height );
int emulation_step( int command );
int enterBackGround();
int MuteSound();
int UnMuteSound();

EAGLContext *g_context = nil;
EAGLContext *g_share_context = nil;

// Settings
BOOL _bios =YES;
int _cart = 0;
BOOL _fps = NO;
BOOL _frame_skip = NO;
BOOL _aspect_rate = NO;
int _filter = 0;
int _sound_engine = 0;
int _rendering_resolution = 0;

@interface GameViewController () {
   int command;
}

@property (strong, nonatomic) EAGLContext *context;
@property (strong, nonatomic) EAGLContext *share_context;
@property (nonatomic, strong) GCController *controller;
@property (nonatomic, assign) int command;
@property (nonatomic, assign) Boolean _return;

@property (nonatomic,strong) KeyMapper *keyMapper;
@property (nonatomic) BOOL isKeyRemappingMode;
@property (nonatomic,strong) UIAlertController *remapAlertController;
@property (nonatomic) SaturnKey currentlyMappingKey;
@property (nonatomic,strong) NSMutableArray *remapLabelViews;

- (void)setupGL;
- (void)tearDownGL;

@end

static GameViewController *sharedData_ = nil;
static NSDictionary *saturnKeyDescriptions;
static NSDictionary *saturnKeyToViewMappings;

int swapAglBuffer ()
{
    EAGLContext* context = [EAGLContext currentContext];
    [context presentRenderbuffer:GL_RENDERBUFFER];
    return 0;
}

void RevokeOGLOnThisThread(){
    [EAGLContext setCurrentContext:g_share_context];
}

void UseOGLOnThisThread(){
    [EAGLContext setCurrentContext:g_context];
}

const char * GetBiosPath(){
    if( _bios == YES ){
        return NULL;
    }
    
    NSFileManager *filemgr;
    filemgr = [NSFileManager defaultManager];
    NSString * fileName = @"bios.bin";
    
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    
    NSString *filePath = [documentsDirectory stringByAppendingPathComponent: fileName];
    NSLog(@"full path name: %@", filePath);
    
    // check if file exists
    if ([filemgr fileExistsAtPath: filePath] == YES){
        NSLog(@"File exists");
        
    }else {
        NSLog (@"File not found, file will be created");
        return NULL;
    }
    
    return [filePath fileSystemRepresentation];
}

const char * GetGamePath(){
    
    if( sharedData_ == nil ){
        return nil;
    }
    NSString *path = sharedData_.selected_file;
    return [path cStringUsingEncoding:1];
}

const char * GetStateSavePath(){
    BOOL isDir;
    NSFileManager *filemgr;
    filemgr = [NSFileManager defaultManager];
    NSString * fileName = @"state/";
    
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    
    NSString *filePath = [documentsDirectory stringByAppendingPathComponent: fileName];
    NSLog(@"full path name: %@", filePath);
    
    
    NSString *docDir = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0];
    NSString *dirName = [docDir stringByAppendingPathComponent:@"state"];
    
    
    NSFileManager *fm = [NSFileManager defaultManager];
    if(![fm fileExistsAtPath:dirName isDirectory:&isDir])
    {
        if([fm createDirectoryAtPath:dirName withIntermediateDirectories:YES attributes:nil error:nil])
            NSLog(@"Directory Created");
        else
            NSLog(@"Directory Creation Failed");
    }
    else
        NSLog(@"Directory Already Exist");

    return [filePath fileSystemRepresentation];
}

const char * GetMemoryPath(){
    BOOL isDir;
    NSFileManager *filemgr;
    filemgr = [NSFileManager defaultManager];
    NSString * fileName = @"backup/memory.bin";
    
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    
    NSString *filePath = [documentsDirectory stringByAppendingPathComponent: fileName];
    NSLog(@"full path name: %@", filePath);
    
    
    NSString *docDir = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0];
    NSString *dirName = [docDir stringByAppendingPathComponent:@"backup"];
    
    
    NSFileManager *fm = [NSFileManager defaultManager];
    if(![fm fileExistsAtPath:dirName isDirectory:&isDir])
    {
        if([fm createDirectoryAtPath:dirName withIntermediateDirectories:YES attributes:nil error:nil])
            NSLog(@"Directory Created");
        else
            NSLog(@"Directory Creation Failed");
    }
    else
        NSLog(@"Directory Already Exist");
    
    // check if file exists
    if ([filemgr fileExistsAtPath: filePath] == YES){
        NSLog(@"File exists");
        
    }else {
        NSLog (@"File not found, file will be created");
    }
    
    return [filePath fileSystemRepresentation];
}

int GetCartridgeType(){
    return _cart;
}

int GetVideoInterface(){
    return 0;
}

int GetEnableFPS(){
    if( _fps == YES )
        return 1;
    
    return 0;
}

int GetEnableFrameSkip(){
    if( _frame_skip == YES ){
        return 1;
    }
    return 0;
}

int GetUseNewScsp(){
    return _sound_engine;
}

int GetVideFilterType(){
    return _filter;
}

int GetResolutionType(){
    NSLog (@"GetResolutionType %d",_rendering_resolution);
    return _rendering_resolution;
}

const char * GetCartridgePath(){
    BOOL isDir;
    NSFileManager *filemgr;
    filemgr = [NSFileManager defaultManager];
    NSString * fileName = @"cart/invalid.ram";
    
    switch(_cart) {
        case CART_NONE:
            fileName = @"cart/none.ram";
        case CART_PAR:
            fileName = @"cart/par.ram";
        case CART_BACKUPRAM4MBIT:
            fileName = @"cart/backup4.ram";
        case CART_BACKUPRAM8MBIT:
            fileName = @"cart/backup8.ram";
        case CART_BACKUPRAM16MBIT:
            fileName = @"cart/backup16.ram";
        case CART_BACKUPRAM32MBIT:
            fileName = @"cart/backup32.ram";
        case CART_DRAM8MBIT:
            fileName = @"cart/dram8.ram";
        case CART_DRAM32MBIT:
            fileName = @"cart/dram32.ram";
        case CART_NETLINK:
            fileName = @"cart/netlink.ram";
        case CART_ROM16MBIT:
            fileName = @"cart/om16.ram";
        default:
            fileName = @"cart/invalid.ram";
    }
    
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    
    NSString *filePath = [documentsDirectory stringByAppendingPathComponent: fileName];
    NSLog(@"full path name: %@", filePath);
    
    
    NSString *docDir = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0];
    NSString *dirName = [docDir stringByAppendingPathComponent:@"cart"];
    
    
    NSFileManager *fm = [NSFileManager defaultManager];
    if(![fm fileExistsAtPath:dirName isDirectory:&isDir])
    {
        if([fm createDirectoryAtPath:dirName withIntermediateDirectories:YES attributes:nil error:nil])
            NSLog(@"Directory Created");
        else
            NSLog(@"Directory Creation Failed");
    }
    else
        NSLog(@"Directory Already Exist");
    
    // check if file exists
    if ([filemgr fileExistsAtPath: filePath] == YES){
        NSLog(@"File exists");
        
    }else {
        NSLog (@"File not found, file will be created");
    }
    return [filePath fileSystemRepresentation];
}

int GetPlayer2Device(){
    return -1;
}


@implementation GameViewController
@synthesize iPodIsPlaying;
@synthesize selected_file;


- (void)saveState{
    self.command = 1;
}

- (void)loadState{
    self.command = 2;
}

/*-------------------------------------------------------------------------------------
 Settings
---------------------------------------------------------------------------------------*/

- (NSString *)GetSettingFilePath {
    
    NSFileManager *filemgr;
    filemgr = [NSFileManager defaultManager];
    NSString * fileName = @"settings.plist";
    
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES);
    NSString *libraryDirectory = [paths objectAtIndex:0];
    
    NSString *filePath = [libraryDirectory stringByAppendingPathComponent: fileName];
    NSLog(@"full path name: %@", filePath);
    
    // check if file exists
    if ([filemgr fileExistsAtPath: filePath] == YES){
        NSLog(@"File exists");
        
    }else {
        NSBundle *bundle = [NSBundle mainBundle];
        NSString *path = [bundle pathForResource:@"settings" ofType:@"plist"];
        [filemgr copyItemAtPath:path toPath:filePath error:nil];
    }
    
    return filePath;
}

- (void)loadSettings {

    NSBundle *bundle = [NSBundle mainBundle];
    //読み込むプロパティリストのファイルパスを指定
    NSString *path = [self GetSettingFilePath];
    //プロパティリストの中身データを取得
    NSDictionary *dic = [NSDictionary dictionaryWithContentsOfFile:path];
    _bios = [[dic objectForKey: @"builtin bios"] boolValue];
    _cart = [[dic objectForKey: @"cartridge"] intValue];
    _fps = [[dic objectForKey: @"show fps"]boolValue];
    _frame_skip = [[dic objectForKey: @"frame skip"]boolValue];
    _aspect_rate = [[dic objectForKey: @"keep aspect rate"]boolValue];
    _filter = 0; //[0; //userDefaults boolForKey: @"filter"];
    _sound_engine = [[dic objectForKey: @"sound engine"] intValue];
    _rendering_resolution = [[dic objectForKey: @"rendering resolution"] intValue];
    
    /*
    NSUserDefaults* userDefaults = [NSUserDefaults standardUserDefaults];
    _bios = [userDefaults boolForKey: @"bios"];
    _cart = (int)[userDefaults integerForKey: @"cart"];
    _fps = [userDefaults boolForKey: @"fps"];
    _frame_skip = [userDefaults boolForKey: @"frame_skip"];
    _aspect_rate = [userDefaults boolForKey: @"aspect_rate"];
    _filter = [userDefaults boolForKey: @"filter"];
    _sound_engine = [userDefaults boolForKey: @"sound_engine"];
     */
}

- (id)init
{
   return [super init];
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{

    if( [self hasControllerConnected ] ) return;
    
    int i=0;
    NSSet *allTouches = [event allTouches];
    for (UITouch *touch in allTouches)
    {
        CGPoint point = [touch locationInView:[self view]];
        
        if( CGRectContainsPoint([ [self left_view ]frame ], point) ){
        
            point = [touch locationInView:[self left_view]];
            
            if( CGRectContainsPoint([ [self right_button ]frame ], point) ){
                [self right_button ].backgroundColor = [UIColor redColor];
                PerKeyDown(SaturnKeyRight);
            }
            if( CGRectContainsPoint([ [self left_button ]frame ], point) ){
                [self left_button ].backgroundColor = [UIColor redColor];
                PerKeyDown(SaturnKeyLeft);
            }
            if( CGRectContainsPoint([ [self up_button ]frame ], point) ){
                [self up_button ].backgroundColor = [UIColor redColor];
                PerKeyDown(SaturnKeyUp);
            }
            if( CGRectContainsPoint([ [self down_button ]frame ], point) ){
                [self down_button ].backgroundColor = [UIColor redColor];
                PerKeyDown(SaturnKeyDown);
            }
            if( CGRectContainsPoint([ [self left_trigger ]frame ], point) ){
                [self left_trigger ].backgroundColor = [UIColor redColor];
                PerKeyDown(SaturnKeyLeftTrigger);
            }

            if( CGRectContainsPoint([ [self start_button ]frame ], point) ){
                [self start_button ].backgroundColor = [UIColor redColor];
                PerKeyDown(SaturnKeyStart);
            }
        }
        
        if( CGRectContainsPoint([ [self right_view ]frame ], point) ){
            
            point = [touch locationInView:[self right_view]];
            
            if( CGRectContainsPoint([ [self a_button ]frame ], point) ){
                [self a_button ].backgroundColor = [UIColor redColor];
                PerKeyDown(SaturnKeyA);
            }
            if( CGRectContainsPoint([ [self b_button ]frame ], point) ){
                [self b_button ].backgroundColor = [UIColor redColor];
                PerKeyDown(SaturnKeyB);
            }
            if( CGRectContainsPoint([ [self c_button ]frame ], point) ){
                [self c_button ].backgroundColor = [UIColor redColor];
                PerKeyDown(SaturnKeyC);
            }
            if( CGRectContainsPoint([ [self x_button ]frame ], point) ){
                [self x_button ].backgroundColor = [UIColor redColor];
                PerKeyDown(SaturnKeyX);
            }
            if( CGRectContainsPoint([ [self y_button ]frame ], point) ){
                [self y_button ].backgroundColor = [UIColor redColor];
                PerKeyDown(SaturnKeyY);
            }
            if( CGRectContainsPoint([ [self z_button ]frame ], point) ){
                [self z_button ].backgroundColor = [UIColor redColor];
                PerKeyDown(SaturnKeyZ);
            }
            if( CGRectContainsPoint([ [self right_trigger ]frame ], point) ){
                [self right_trigger ].backgroundColor = [UIColor redColor];
                PerKeyDown(SaturnKeyRightTrigger);
            }
        }
        
        i++;
    }
    


}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    if( [self hasControllerConnected ] ) return;
    
    int i=0;
    NSSet *allTouches = [event allTouches];
    for (UITouch *touch in allTouches)
    {
        CGPoint point = [touch locationInView:[self view]];
        
        if( CGRectContainsPoint([ [self left_view ]frame ], point) ){
            
            point = [touch locationInView:[self left_view]];
        
            if( CGRectContainsPoint([ [self right_button ]frame ], point) ){
                [self right_button ].backgroundColor = [UIColor redColor];
                PerKeyDown(SaturnKeyRight);
            }else{
                [self right_button ].backgroundColor = [UIColor darkGrayColor];
                PerKeyUp(SaturnKeyRight);
            }
            if( CGRectContainsPoint([ [self left_button ]frame ], point) ){
                [self left_button ].backgroundColor = [UIColor redColor];
                PerKeyDown(SaturnKeyLeft);
            }else{
                [self left_button ].backgroundColor = [UIColor darkGrayColor];
                PerKeyUp(SaturnKeyLeft);
            }
            if( CGRectContainsPoint([ [self up_button ]frame ], point) ){
                [self up_button ].backgroundColor = [UIColor redColor];
                PerKeyDown(SaturnKeyUp);
            }else{
                [self up_button ].backgroundColor = [UIColor darkGrayColor];
                PerKeyUp(SaturnKeyUp);
            }
            if( CGRectContainsPoint([ [self down_button ]frame ], point) ){
                [self down_button ].backgroundColor = [UIColor redColor];
                PerKeyDown(SaturnKeyDown);
            }else{
                [self down_button ].backgroundColor = [UIColor darkGrayColor];
                PerKeyUp(SaturnKeyDown);
            }
        }

        i++;
    }

}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    if( [self hasControllerConnected ] && !self.isKeyRemappingMode ) return;
    
    int i=0;
    NSSet *allTouches = [event allTouches];
    for (UITouch *touch in allTouches)
    {
        CGPoint point = [touch locationInView:[self view]];
        
        if( CGRectContainsPoint([ [self left_view ]frame ], point) ){
            
            point = [touch locationInView:[self left_view]];
            
            if( CGRectContainsPoint([ [self right_button ]frame ], point) ){
                [self right_button ].backgroundColor = [UIColor darkGrayColor];
                PerKeyUp(SaturnKeyRight);
            }
            if( CGRectContainsPoint([ [self left_button ]frame ], point) ){
                [self left_button ].backgroundColor = [UIColor darkGrayColor];
                PerKeyUp(SaturnKeyLeft);
            }
            if( CGRectContainsPoint([ [self up_button ]frame ], point) ){
                [self up_button ].backgroundColor = [UIColor darkGrayColor];
                PerKeyUp(SaturnKeyUp);
            }
            if( CGRectContainsPoint([ [self down_button ]frame ], point) ){
                [self down_button ].backgroundColor = [UIColor darkGrayColor];
                PerKeyUp(SaturnKeyDown);
            }
            if( CGRectContainsPoint([ [self left_trigger ]frame ], point) ){
                [self left_trigger ].backgroundColor = [UIColor darkGrayColor];
                if ( self.isKeyRemappingMode ) {
                    [self showRemapControlAlertWithSaturnKey:SaturnKeyLeftTrigger];
                    return;
                } else {
                    PerKeyUp(SaturnKeyLeftTrigger);
                }
            }

            if( CGRectContainsPoint([ [self start_button ]frame ], point) ){
                [self start_button ].backgroundColor = [UIColor darkGrayColor];
                if ( self.isKeyRemappingMode ) {
                    [self showRemapControlAlertWithSaturnKey:SaturnKeyStart];
                    return;
                } else {
                    PerKeyUp(SaturnKeyStart);
                }
            }
        }
        
        if( CGRectContainsPoint([ [self right_view ]frame ], point) ){
            
            point = [touch locationInView:[self right_view]];
            
            if( CGRectContainsPoint([ [self a_button ]frame ], point) ){
                [self a_button ].backgroundColor = [UIColor darkGrayColor];
                if ( self.isKeyRemappingMode ) {
                    [self showRemapControlAlertWithSaturnKey:SaturnKeyA];
                    return;
                } else {
                    PerKeyUp(SaturnKeyA);
                }
            }
            if( CGRectContainsPoint([ [self b_button ]frame ], point) ){
                [self b_button ].backgroundColor = [UIColor darkGrayColor];
                if ( self.isKeyRemappingMode ) {
                    [self showRemapControlAlertWithSaturnKey:SaturnKeyB];
                    return;
                } else {
                    PerKeyUp(SaturnKeyB);
                }
            }
            if( CGRectContainsPoint([ [self c_button ]frame ], point) ){
                [self c_button ].backgroundColor = [UIColor darkGrayColor];
                if ( self.isKeyRemappingMode ) {
                    [self showRemapControlAlertWithSaturnKey:SaturnKeyC];
                    return;
                } else {
                    PerKeyUp(SaturnKeyC);
                }
            }
            if( CGRectContainsPoint([ [self x_button ]frame ], point) ){
                [self x_button ].backgroundColor = [UIColor darkGrayColor];
                if ( self.isKeyRemappingMode ) {
                    [self showRemapControlAlertWithSaturnKey:SaturnKeyX];
                    return;
                } else {
                    PerKeyUp(SaturnKeyX);
                }
            }
            if( CGRectContainsPoint([ [self y_button ]frame ], point) ){
                [self y_button ].backgroundColor = [UIColor darkGrayColor];
                if ( self.isKeyRemappingMode ) {
                    [self showRemapControlAlertWithSaturnKey:SaturnKeyY];
                    return;
                } else {
                    PerKeyUp(SaturnKeyY);
                }
            }
            if( CGRectContainsPoint([ [self z_button ]frame ], point) ){
                [self z_button ].backgroundColor = [UIColor darkGrayColor];
                if ( self.isKeyRemappingMode ) {
                    [self showRemapControlAlertWithSaturnKey:SaturnKeyZ];
                    return;
                } else {
                    PerKeyUp(SaturnKeyZ);
                }
            }
            if( CGRectContainsPoint([ [self right_trigger ]frame ], point) ){
                [self right_trigger ].backgroundColor = [UIColor darkGrayColor];
                if ( self.isKeyRemappingMode ) {
                    [self showRemapControlAlertWithSaturnKey:SaturnKeyRightTrigger];
                    return;
                } else {
                    PerKeyUp(SaturnKeyRightTrigger);
                }
            }
        }

        i++;
    }
}

#pragma mark AVAudioSession
- (void)handleInterruption:(NSNotification *)notification
{
    UInt8 theInterruptionType = [[notification.userInfo valueForKey:AVAudioSessionInterruptionTypeKey] intValue];
    
    NSLog(@"Session interrupted > --- %s ---\n", theInterruptionType == AVAudioSessionInterruptionTypeBegan ? "Begin Interruption" : "End Interruption");
    
    if (theInterruptionType == AVAudioSessionInterruptionTypeBegan) {
        //alcMakeContextCurrent(NULL);
        //if (self.isPlaying) {
        //    self.wasInterrupted = YES;
        //}
    } else if (theInterruptionType == AVAudioSessionInterruptionTypeEnded) {
        // make sure to activate the session
        NSError *error;
        bool success = [[AVAudioSession sharedInstance] setActive:YES error:&error];
        if (!success) NSLog(@"Error setting session active! %@\n", [error localizedDescription]);
        
        //alcMakeContextCurrent(self.context);
        
        //if (self.wasInterrupted)
        //{
         //   [self startSound];
         //   self.wasInterrupted = NO;
        //}
    }
}

#pragma mark -Audio Session Route Change Notification

- (void)handleRouteChange:(NSNotification *)notification
{
    UInt8 reasonValue = [[notification.userInfo valueForKey:AVAudioSessionRouteChangeReasonKey] intValue];
    AVAudioSessionRouteDescription *routeDescription = [notification.userInfo valueForKey:AVAudioSessionRouteChangePreviousRouteKey];
    
    NSLog(@"Route change:");
    switch (reasonValue) {
        case AVAudioSessionRouteChangeReasonNewDeviceAvailable:
            NSLog(@"     NewDeviceAvailable");
            break;
        case AVAudioSessionRouteChangeReasonOldDeviceUnavailable:
            NSLog(@"     OldDeviceUnavailable");
            break;
        case AVAudioSessionRouteChangeReasonCategoryChange:
            NSLog(@"     CategoryChange");
            NSLog(@" New Category: %@", [[AVAudioSession sharedInstance] category]);
            break;
        case AVAudioSessionRouteChangeReasonOverride:
            NSLog(@"     Override");
            break;
        case AVAudioSessionRouteChangeReasonWakeFromSleep:
            NSLog(@"     WakeFromSleep");
            break;
        case AVAudioSessionRouteChangeReasonNoSuitableRouteForCategory:
            NSLog(@"     NoSuitableRouteForCategory");
            break;
        default:
            NSLog(@"     ReasonUnknown");
    }
    
    NSLog(@"Previous route:\n");
    NSLog(@"%@", routeDescription);
}

- (void)controllerDidConnect:(NSNotification *)notification
{
    [self foundController];
}

- (void)controllerDidDisconnect
{
    [self setControllerOverlayHidden:NO];
    self.remapButton.hidden = YES;
}

-(void)completionWirelessControllerDiscovery
{
    
    void (^mfiButtonHandler)(KeyMapMappableButton,BOOL) = ^void(KeyMapMappableButton mfiButton, BOOL pressed) {
        NSLog(@"calling mfi button handler with button: %li , key remap mode = %@ , currently mapped key: %li",(long)mfiButton,self.isKeyRemappingMode ? @"YES" : @"NO", self.currentlyMappingKey);
        if ( self.isKeyRemappingMode && self.currentlyMappingKey != NSNotFound ) {
            [self.keyMapper mapKey:self.currentlyMappingKey ToControl:mfiButton];
            if ( self.remapAlertController != nil ) {
                [self.remapAlertController dismissViewControllerAnimated:YES completion:nil];
            }
            [self.keyMapper saveKeyMapping];
            self.currentlyMappingKey = NSNotFound;
            [self refreshViewsWithKeyRemaps];
        } else {
            SaturnKey saturnKey = [self.keyMapper getMappedKeyForControl:mfiButton];
            if ( saturnKey != NSNotFound ) {
                if(pressed){
                    PerKeyDown(saturnKey);
                }else{
                    PerKeyUp(saturnKey);
                }
            }
        }
    };
    
    if( [GCController controllers].count >= 1 ){
        self.controller = [GCController controllers][0];
        if (self.controller.gamepad) {
            
            [self.controller.gamepad.buttonA setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                mfiButtonHandler(MFI_BUTTON_A, pressed);
            }];
            
            [self.controller.extendedGamepad.rightShoulder setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                mfiButtonHandler(MFI_BUTTON_RS, pressed);
            }];
            
            [self.controller.extendedGamepad.leftShoulder setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                mfiButtonHandler(MFI_BUTTON_LS, pressed);
            }];
            
            [self.controller.extendedGamepad.leftTrigger setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                mfiButtonHandler(MFI_BUTTON_LT, pressed);
            }];
            
            [self.controller.extendedGamepad.rightTrigger setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                mfiButtonHandler(MFI_BUTTON_RT, pressed);
            }];
            
            [self.controller.gamepad.buttonX setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                mfiButtonHandler(MFI_BUTTON_X, pressed);
            }];
            [self.controller.gamepad.buttonY setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                mfiButtonHandler(MFI_BUTTON_Y, pressed);
            }];
            [self.controller.gamepad.buttonB setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                mfiButtonHandler(MFI_BUTTON_B, pressed);
            }];
            [self.controller.gamepad.dpad.up setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                if(pressed){
                    PerKeyDown(SaturnKeyUp);
                }else{
                    PerKeyUp(SaturnKeyUp);
                }
            }];
            [self.controller.gamepad.dpad.down setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                if(pressed){
                    PerKeyDown(SaturnKeyDown);
                }else{
                    PerKeyUp(SaturnKeyDown);
                }
            }];
            [self.controller.gamepad.dpad.left setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                if(pressed){
                    PerKeyDown(SaturnKeyLeft);
                }else{
                    PerKeyUp(SaturnKeyLeft);
                }
            }];
            [self.controller.gamepad.dpad.right setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                if(pressed){
                    PerKeyDown(SaturnKeyRight);
                }else{
                    PerKeyUp(SaturnKeyRight);
                }
            }];
            [self.controller.extendedGamepad.rightThumbstick setValueChangedHandler:^(GCControllerDirectionPad *dpad, float xValue, float yValue) {
                if(yValue >= 0.5 || yValue <= -0.5 ){
                    PerKeyDown(SaturnKeyStart);
                }else{
                    PerKeyUp(SaturnKeyStart);
                }
            }];
        }
    }
    
}

- (BOOL)hasControllerConnected {
    return [[GCController controllers] count] > 0;
}


- (void)viewDidAppear:(BOOL)animated
{
    
    if ([self hasControllerConnected]) {
        NSLog(@"Discovery finished on first pass");
        [self foundController];
    } else {
        NSLog(@"Discovery happening patiently");
        [self patientlyDiscoverController];
    }
}

- (void)patientlyDiscoverController {
    
    [GCController startWirelessControllerDiscoveryWithCompletionHandler:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(foundController)
                                                 name:GCControllerDidConnectNotification
                                               object:nil];
}   

- (void)foundController {
    NSLog(@"Found Controller");
    

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(controllerDidDisconnect)
                                                 name:GCControllerDidDisconnectNotification
                                               object:nil];
    [self refreshViewsWithKeyRemaps];
    [self setControllerOverlayHidden:YES];
    self.remapButton.hidden = NO;
    [self completionWirelessControllerDiscovery];
}

-(void)setControllerOverlayHidden:(BOOL)hidden {
    [self left_panel ].hidden = hidden;
    [self right_panel ].hidden = hidden;
    [self left_button ].hidden = hidden;
    [self right_button ].hidden = hidden;
    [self up_button ].hidden = hidden;
    [self down_button ].hidden = hidden;
    [self a_button ].hidden = hidden;
    [self b_button ].hidden = hidden;
    [self c_button ].hidden = hidden;
    [self x_button ].hidden = hidden;
    [self y_button ].hidden = hidden;
    [self z_button ].hidden = hidden;
    [self left_trigger ].hidden = hidden;
    [self right_trigger ].hidden = hidden;
    [self start_button ].hidden = hidden;
    
    [self.remapLabelViews enumerateObjectsUsingBlock:^(UILabel *  _Nonnull obj, NSUInteger idx, BOOL * _Nonnull stop) {
        obj.hidden = [self hasControllerConnected] && hidden == NO ? NO : hidden;
    }];
}

-(void)setPaused:(BOOL)paused
{
    [super setPaused:paused];
    if( paused == YES ){
        MuteSound();
    }else{
        UnMuteSound();
    }
}

- (void)viewDidLoad
{
    sharedData_ = self;
    [super viewDidLoad];
    
    GameRevealViewController *revealViewController = (GameRevealViewController *)self.revealViewController;
    if ( revealViewController )
    {
        //[self.sidebarButton setTarget: self.revealViewController];
        //SWRevealViewControllerSetSegue        [self.sidebarButton setAction: @selector( revealToggle: )];
        [self.view addGestureRecognizer:self.revealViewController.panGestureRecognizer];
        self.selected_file = revealViewController.selected_file;
    }
    
    self.view.multipleTouchEnabled = YES;
    self.command = 0;
    
     [self right_view].backgroundColor = [UIColor colorWithRed:0.0f green:0.0f blue:0.0f alpha:0.0f];
    [self left_view].backgroundColor = [UIColor colorWithRed:0.0f green:0.0f blue:0.0f alpha:0.0f];

    [self left_button ].alpha = 0.0f;
    [self right_button ].alpha = 0.0f;
    [self up_button ].alpha = 0.0f;
    [self down_button ].alpha = 0.0f;
    [self a_button ].alpha = 0.0f;
    [self b_button ].alpha = 0.0f;
    [self c_button ].alpha = 0.0f;
    [self x_button ].alpha = 0.0f;
    [self y_button ].alpha = 0.0f;
    [self z_button ].alpha = 0.0f;
    [self left_trigger ].alpha = 0.0f;
    [self right_trigger ].alpha = 0.0f;
    [self start_button ].alpha = 0.0f;
    
    [self loadSettings];

    if( _aspect_rate ){
        CGRect newFrame = self.view.frame;
        int specw = self.view.frame.size.width;
        int spech = self.view.frame.size.height;
        float specratio = (float)specw / (float)spech;
        int saturnw = 320;
        int saturnh = 224;
        float saturnraito = (float)saturnw/ (float)saturnh;
        float revraito = (float) saturnh/ (float)saturnw;
        
        if( specratio > saturnraito ){
            
            newFrame.size.width = spech * saturnraito;
            newFrame.size.height = spech;
            newFrame.origin.x = (self.view.frame.size.width - newFrame.size.width)/2.0;
            newFrame.origin.y = (self.view.frame.size.height - newFrame.size.height)/2.0;
            self.view.frame = newFrame;
        }else{
            newFrame.size.width = specw * revraito;
            newFrame.size.height = specw;
            newFrame.origin.x = (self.view.frame.size.width - newFrame.size.width)/2.0;
            newFrame.origin.y = (self.view.frame.size.height - newFrame.size.height)/2.0;
            self.view.frame = newFrame;
        }
    }

    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(didEnterBackground) name:UIApplicationDidEnterBackgroundNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(didBecomeActive) name:UIApplicationDidBecomeActiveNotification object:nil];
    
    self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
    //self.context.multiThreaded = YES;
    self.share_context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3 sharegroup:[self.context sharegroup] ];

    if (!self.context) {
        NSLog(@"Failed to create ES context");
    }

    g_share_context = self.share_context;
    g_context = self.context;
    
    GLKView *view = (GLKView *)self.view;
    view.context = self.context;
    view.drawableDepthFormat = GLKViewDrawableDepthFormat24;
    view.drawableStencilFormat = GLKViewDrawableStencilFormat8;

    // Configure the audio session
    AVAudioSession *sessionInstance = [AVAudioSession sharedInstance];
    NSError *error;
    
    // set the session category
    iPodIsPlaying = [sessionInstance isOtherAudioPlaying];
    NSString *category = iPodIsPlaying ? AVAudioSessionCategoryAmbient : AVAudioSessionCategorySoloAmbient;
    bool success = [sessionInstance setCategory:category error:&error];
    if (!success) NSLog(@"Error setting AVAudioSession category! %@\n", [error localizedDescription]);
    
    double hwSampleRate = 44100.0;
    success = [sessionInstance setPreferredSampleRate:hwSampleRate error:&error];
    if (!success) NSLog(@"Error setting preferred sample rate! %@\n", [error localizedDescription]);
    
    // add interruption handler
    [[NSNotificationCenter defaultCenter]   addObserver:self
                                               selector:@selector(handleInterruption:)
                                                   name:AVAudioSessionInterruptionNotification
                                                 object:sessionInstance];
    
    // we don't do anything special in the route change notification
    [[NSNotificationCenter defaultCenter]   addObserver:self
                                               selector:@selector(handleRouteChange:)
                                                   name:AVAudioSessionRouteChangeNotification
                                                 object:sessionInstance];
    
    // activate the audio session
    success = [sessionInstance setActive:YES error:&error];
    if (!success) NSLog(@"Error setting session active! %@\n", [error localizedDescription]);

   
    
    self.preferredFramesPerSecond =120;
    
    
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        saturnKeyDescriptions =
        @{
          [NSNumber numberWithInteger:SaturnKeyA] : @"A",
          [NSNumber numberWithInteger:SaturnKeyB] : @"B",
          [NSNumber numberWithInteger:SaturnKeyC] : @"C",
          [NSNumber numberWithInteger:SaturnKeyX] : @"X",
          [NSNumber numberWithInteger:SaturnKeyY] : @"Y",
          [NSNumber numberWithInteger:SaturnKeyZ] : @"Z",
          [NSNumber numberWithInteger:SaturnKeyLeftTrigger] : @"LT",
          [NSNumber numberWithInteger:SaturnKeyRightTrigger] : @"RT",
          [NSNumber numberWithInteger:SaturnKeyStart] : @"Start",
          };
        
        saturnKeyToViewMappings =
        @{
          [NSNumber numberWithInteger:SaturnKeyA] : _a_button,
          [NSNumber numberWithInteger:SaturnKeyB] : _b_button,
          [NSNumber numberWithInteger:SaturnKeyC] : _c_button,
          [NSNumber numberWithInteger:SaturnKeyX] : _x_button,
          [NSNumber numberWithInteger:SaturnKeyY] : _y_button,
          [NSNumber numberWithInteger:SaturnKeyZ] : _z_button,
          [NSNumber numberWithInteger:SaturnKeyLeftTrigger] : _left_trigger,
          [NSNumber numberWithInteger:SaturnKeyRightTrigger] : _right_trigger,
          [NSNumber numberWithInteger:SaturnKeyStart] : _start_button,
          };
    });
    self.keyMapper = [[KeyMapper alloc] init];
    [self.keyMapper loadFromDefaults];
    self.remapLabelViews = [NSMutableArray array];
    self.remapButton.layer.borderWidth = 1.0;
    self.remapButton.layer.borderColor = [self.remapButton.tintColor CGColor];
    self.remapButton.hidden = YES;
  
    [self setupGL];
}

- (void)dealloc
{    
    [self tearDownGL];
    
    if ([EAGLContext currentContext] == self.context) {
        [EAGLContext setCurrentContext:nil];
    }
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];

    if ([self isViewLoaded] && ([[self view] window] == nil)) {
        self.view = nil;
        
        [self tearDownGL];
        
        if ([EAGLContext currentContext] == self.context) {
            [EAGLContext setCurrentContext:nil];
        }
        self.context = nil;
    }

    // Dispose of any resources that can be recreated.
}

- (BOOL)prefersStatusBarHidden {
    return YES;
}

- (void)setupGL
{
    [EAGLContext setCurrentContext:self.context];
    GLKView *view = (GLKView *)self.view;
    view.context = self.context;
    view.contentScaleFactor = [UIScreen mainScreen].scale;
     [view bindDrawable ];
    
    CGFloat scale = [[UIScreen mainScreen] scale];
    printf("viewport(%f,%f)\n",[view frame].size.width,[view frame].size.height);
    start_emulation([view frame].origin.x*scale, [view frame].origin.y*scale, [view frame].size.width*scale,[view frame].size.height*scale);
    
    self._return = YES;
    
    //start_emulation(1920,1080);
    
}

- (void)tearDownGL
{
    [EAGLContext setCurrentContext:self.context];
   
}

#pragma mark - GLKView and GLKViewController delegate methods

- (void)update
{
    if( self._return == YES ){
        GLKView *view = (GLKView *)self.view;
        GLint defaultFBO;
        [view bindDrawable];
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &defaultFBO);
        printf("DEFAULT FBO: %d\n", defaultFBO);
        self._return = NO;
        
    }
    
    emulation_step(self.command);
    self.command = 0;
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
}

- (void)didEnterBackground {
    
    GLKView *view = (GLKView *)self.view;
    enterBackGround();
    [self setPaused:true];
    
    //if (view.active)
        [view resignFirstResponder];
}

- (void)didBecomeActive {
    //if (self.view.active)
    [self.view becomeFirstResponder];
    
    [self setPaused:false];
    

    
    self._return = YES;
}

- (BOOL)canBecomeFirstResponder {
    return YES;
}

#pragma mark -
#pragma mark UIKeyInput Protocol Methods

- (BOOL)hasText {
    return NO;
}

- (void)insertText:(NSString *)text {
    NSLog(@"Key Input %@\n", text);
}

- (void)deleteBackward {
    // This space intentionally left blank to complete protocol
}

#pragma mark -
#pragma mark Key Remapping

-(IBAction)startKeyRemapping:(id)sender {
    [self refreshViewsWithKeyRemaps];
    if ( self.isKeyRemappingMode ) {
        self.isKeyRemappingMode = NO;
        [self.remapButton setSelected:NO];
        [self setControllerOverlayHidden:YES];
    } else {
        self.isKeyRemappingMode = YES;
        [self.remapButton setSelected:YES];
        [self setControllerOverlayHidden:NO];
    }
}

-(void)refreshViewsWithKeyRemaps {
    if ( self.remapLabelViews.count > 0 ) {
        [self.remapLabelViews makeObjectsPerformSelector:@selector(removeFromSuperview)];
    }
    [self.remapLabelViews removeAllObjects];
    
    for (NSNumber *button in saturnKeyToViewMappings.allKeys) {
        NSMutableString *buttonNames = [NSMutableString string];
        NSArray *mfiButtons = [self.keyMapper getControlsForMappedKey:button.integerValue];
        NSUInteger mfiButtonIndex = 0;
        for (NSNumber *mfiButton in mfiButtons) {
            if ( mfiButtonIndex++ > 0 ) {
                [buttonNames appendString:@" / "];
            }
            [buttonNames appendString:[KeyMapper controlToDisplayName:mfiButton.integerValue]];
        }
        if ( mfiButtons.count > 0 ) {
            UILabel *mappedLabel = [[UILabel alloc] initWithFrame:CGRectZero];
            mappedLabel.text = buttonNames;
            mappedLabel.alpha = 0.6f;
            mappedLabel.font = [UIFont boldSystemFontOfSize:16.0];
            mappedLabel.textColor = [UIColor redColor];
            mappedLabel.translatesAutoresizingMaskIntoConstraints = NO;
            UIView *saturnButtonView = [saturnKeyToViewMappings objectForKey:button];
            [self.view addSubview:mappedLabel];
            [self.view addConstraint:[NSLayoutConstraint constraintWithItem:mappedLabel attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:saturnButtonView attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0.0]];
            [self.view addConstraint:[NSLayoutConstraint constraintWithItem:mappedLabel attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:saturnButtonView attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0.0]];
            [self.view bringSubviewToFront:mappedLabel];
            [self.remapLabelViews addObject:mappedLabel];
            [self.view setNeedsLayout];
        }
    }
}


-(void)showRemapControlAlertWithSaturnKey:(SaturnKey)saturnKey {
    self.remapAlertController = [UIAlertController alertControllerWithTitle:@"Remap Key"                                                                message:[NSString stringWithFormat:@"Press a button to map the [%@] key",[saturnKeyDescriptions objectForKey:[NSNumber numberWithInteger:saturnKey]]]
        preferredStyle:UIAlertControllerStyleAlert];
    UIAlertAction *cancel = [UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:^(UIAlertAction * _Nonnull action) {
        self.isKeyRemappingMode = NO;
        [self.remapButton setSelected:NO];
        [self setControllerOverlayHidden:YES];
        [self.remapAlertController dismissViewControllerAnimated:YES completion:nil];
        self.currentlyMappingKey = NSNotFound;
    }];
    self.isKeyRemappingMode = YES;
    [self.remapAlertController addAction:cancel];
    self.currentlyMappingKey = saturnKey;
    [self presentViewController:self.remapAlertController animated:YES completion:nil];
}

@end
