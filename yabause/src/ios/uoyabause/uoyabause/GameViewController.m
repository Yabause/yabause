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
#import "SidebarViewController.h"
#import "GLView.h"

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
void resize_screen( int x, int y, int width, int height );
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
BOOL _rotate_screen = NO;
float _controller_scale = 1.0;

GLuint _renderBuffer = 0;
NSObject* _objectForLock;

@interface GameViewController () {
   int command;
    int controller_edit_mode;
    BOOL canRotateToAllOrientations;
    BOOL _landscape;
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

@property (nonatomic) BOOL _isFirst;

- (void)setup;
- (void)tearDownGL;

@end

static GameViewController *sharedData_ = nil;
static NSDictionary *saturnKeyDescriptions;
static NSDictionary *saturnKeyToViewMappings;

int swapAglBuffer ()
{
    @synchronized (_objectForLock){
        EAGLContext* context = [EAGLContext currentContext];
        glBindRenderbuffer(GL_RENDERBUFFER, _renderBuffer);
        [context presentRenderbuffer:GL_RENDERBUFFER];
    }
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

int GetIsRotateScreen() {
    if( _rotate_screen == YES )
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
    _rotate_screen = [[dic objectForKey: @"rotate screen"]boolValue];
    //_controller_scale = [[dic objectForKey: @"controller scale"] floatValue];
    
    NSUserDefaults *ud = [NSUserDefaults standardUserDefaults];
    NSMutableDictionary *defaults = [NSMutableDictionary dictionary];
    [defaults setObject:@"0.8" forKey:@"controller scale"];
    [defaults setObject:[NSNumber numberWithBool:YES] forKey:@"landscape"];
    [ud registerDefaults:defaults];
    
    _controller_scale = [ud floatForKey:@"controller scale"];
    _landscape = [ud boolForKey:@"landscape"];

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
    //NSSet *allTouches = [event allTouches];
    for (UITouch *touch in touches)
    {
        for (int btnindex = 0; btnindex < BUTTON_LAST; btnindex++) {
            UIView * target = pad_buttons_[btnindex].target_;
            CGPoint point = [touch locationInView:target];
            if( CGRectContainsPoint( target.bounds , point) ) {
                [pad_buttons_[btnindex] On:touch];
 //               printf("PAD:%d on\n",btnindex);
            }else{
                //printf("%d: [%d,%d,%d,%d]-[%d,%d]\n", btnindex, (int)target.frame.origin.x, (int)target.frame.origin.y, (int)target.bounds.size.width, (int)target.bounds.size.height, (int)point.x, (int)point.y);
            }
        }
    }
    
    if( [self hasControllerConnected ] ) return;
    for (int btnindex = 0; btnindex < BUTTON_LAST; btnindex++) {
        if ([pad_buttons_[btnindex] isOn] ) {
            PerKeyDown(btnindex);
        } else {
            PerKeyUp(btnindex);
        }
    }
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    //NSSet *allTouches = [event allTouches];
    for (UITouch *touch in touches)
    {
        if( [pad_buttons_[BUTTON_DOWN] isOn] && pad_buttons_[BUTTON_DOWN].getPointId == touch ){

            UIView * target = pad_buttons_[BUTTON_DOWN].target_;
            CGPoint point = [touch locationInView:target];
            if( point.y < 0 ){
                [pad_buttons_[BUTTON_DOWN] Off ];
                //printf("PAD:%d OFF\n",(int)BUTTON_DOWN );
            }
        }
        if( [pad_buttons_[BUTTON_UP] isOn] && pad_buttons_[BUTTON_UP].getPointId == touch ){
            
            UIView * target = pad_buttons_[BUTTON_UP].target_;
            CGPoint point = [touch locationInView:target];
            //printf("PAD:%d MOVE %d\n",(int)BUTTON_UP,(int)point.y);
            if( point.y > target.bounds.size.height){
                [pad_buttons_[BUTTON_UP] Off ];
                //printf("PAD:%d OFF\n",(int)BUTTON_UP );
            }
        }
        
        UIView * target = pad_buttons_[BUTTON_DOWN].target_;
        CGPoint point = [touch locationInView:target];
        if( CGRectContainsPoint( target.bounds , point) ) {
            [pad_buttons_[BUTTON_DOWN] On:touch];
            //printf("PAD:%d ON\n",(int)BUTTON_DOWN );
        }
        target = pad_buttons_[BUTTON_UP].target_;
        point = [touch locationInView:target];
        if( CGRectContainsPoint( target.bounds , point) ) {
            [pad_buttons_[BUTTON_UP] On:touch];
            //printf("PAD:%d ON\n",(int)BUTTON_UP );
        }

        if( [pad_buttons_[BUTTON_RIGHT] isOn] && pad_buttons_[BUTTON_RIGHT].getPointId == touch ){
            
            UIView * target = pad_buttons_[BUTTON_RIGHT].target_;
            CGPoint point = [touch locationInView:target];
            if( point.x < 0 ){
                [pad_buttons_[BUTTON_RIGHT] Off ];
                //printf("PAD:%d OFF\n",(int)BUTTON_RIGHT );
            }
        }
        if( [pad_buttons_[BUTTON_LEFT] isOn] && pad_buttons_[BUTTON_LEFT].getPointId == touch ){
            
            UIView * target = pad_buttons_[BUTTON_LEFT].target_;
            CGPoint point = [touch locationInView:target];
            if( point.x > target.bounds.size.width){
                [pad_buttons_[BUTTON_LEFT] Off ];
                //printf("PAD:%d OFF\n",(int)BUTTON_LEFT );
            }
        }
        
        target = pad_buttons_[BUTTON_LEFT].target_;
        point = [touch locationInView:target];
        if( CGRectContainsPoint( target.bounds , point) ) {
            [pad_buttons_[BUTTON_LEFT] On:touch];
            //printf("PAD:%d ON\n",(int)BUTTON_LEFT );
        }
        target = pad_buttons_[BUTTON_RIGHT].target_;
        point = [touch locationInView:target];
        if( CGRectContainsPoint( target.bounds , point) ) {
            [pad_buttons_[BUTTON_RIGHT] On:touch];
            //printf("PAD:%d ON\n",(int)BUTTON_RIGHT );
        }


        for (int btnindex = BUTTON_RIGHT_TRIGGER; btnindex < BUTTON_LAST; btnindex++) {

            UIView * target = pad_buttons_[btnindex].target_;
            CGPoint point = [touch locationInView:target];

            if( pad_buttons_[btnindex].getPointId == touch ) {
                if( !CGRectContainsPoint( target.bounds , point) ) {
                    [pad_buttons_[btnindex] Off];
                    //printf("touchesMoved PAD:%d OFF\n",btnindex);
                }
            }else{
                if( CGRectContainsPoint( target.bounds , point) ) {
                    [pad_buttons_[btnindex] On:touch];
                    //printf("touchesMoved PAD:%d MOVE\n",btnindex);
                }else{
                    //printf("%d: [%d,%d,%d,%d]-[%d,%d]\n", btnindex, (int)target.frame.origin.x, (int)target.frame.origin.y, (int)target.bounds.size.width, (int)target.bounds.size.height, (int)point.x, (int)point.y);
                }
            }
        }
    }
    
    if( [self hasControllerConnected ] ) return;
    for (int btnindex = 0; btnindex < BUTTON_LAST; btnindex++) {
        if ([pad_buttons_[btnindex] isOn] ) {
            PerKeyDown(btnindex);
        } else {
            PerKeyUp(btnindex);
        }
    }

}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{

    //NSSet *allTouches = [event allTouches];
    for (UITouch *touch in touches)
    {
        for (int btnindex = 0; btnindex < BUTTON_LAST; btnindex++) {
            if ( [pad_buttons_[btnindex] isOn] && [pad_buttons_[btnindex] getPointId] == touch)  {
                [pad_buttons_[btnindex] Off ];
                //printf("touchesEnded PAD:%d Up\n",btnindex);
                if( self.isKeyRemappingMode ){
                    [self showRemapControlAlertWithSaturnKey:btnindex];
                }
            }
        }
    }
    
    if( [self hasControllerConnected ] ) return;
    for (int btnindex = 0; btnindex < BUTTON_LAST; btnindex++) {
        if ([pad_buttons_[btnindex] isOn] ) {
            PerKeyDown(btnindex);
        } else {
            PerKeyUp(btnindex);
        }
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
    @synchronized (_objectForLock){
        [self foundController];
    }
}

- (void)controllerDidDisconnect
{
    @synchronized (_objectForLock){
        [self setControllerOverlayHidden:NO];
        [self updateSideMenu];
    }
}

-(void)completionWirelessControllerDiscovery
{
    
    void (^mfiButtonHandler)(KeyMapMappableButton,BOOL) = ^void(KeyMapMappableButton mfiButton, BOOL pressed) {
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
    [super viewDidAppear:animated];
    
    if ([self hasControllerConnected]) {
        NSLog(@"Discovery finished on first pass");
        [self foundController];
    } else {
        NSLog(@"Discovery happening patiently");
        [self patientlyDiscoverController];
    }
    canRotateToAllOrientations = YES;
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
    @synchronized (_objectForLock){
    [self refreshViewsWithKeyRemaps];
    [self setControllerOverlayHidden:YES];
    [self completionWirelessControllerDiscovery];
    [self updateSideMenu];
    }
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
    GLView * gv = (GLView*) self.view;
    if( paused == YES ){
        [gv stopAnimation];
    }else{
        if( self->controller_edit_mode != 1 ){
            [gv startAnimation];
        }
    }
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    [self loadSettings];
    
    if( _landscape == YES ){
        [[UIDevice currentDevice] setValue:@(UIInterfaceOrientationLandscapeLeft) forKey:@"orientation"];
        [UINavigationController attemptRotationToDeviceOrientation];
    }
    
    sharedData_ = self;
    _objectForLock = [[NSObject alloc] init];
    self._isFirst = YES;

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
 
    pad_buttons_[BUTTON_UP] = [[PadButton alloc] init];
    pad_buttons_[BUTTON_UP].target_ = self.up_button;
    pad_buttons_[BUTTON_DOWN] = [[PadButton alloc] init];
    pad_buttons_[BUTTON_DOWN].target_ = self.down_button;
    pad_buttons_[BUTTON_LEFT] = [[PadButton alloc] init];
    pad_buttons_[BUTTON_LEFT].target_ = self.left_button;
    pad_buttons_[BUTTON_RIGHT] = [[PadButton alloc] init];
    pad_buttons_[BUTTON_RIGHT].target_ = self.right_button;
    pad_buttons_[BUTTON_A] = [[PadButton alloc] init];
    pad_buttons_[BUTTON_A].target_ = self.a_button;
    pad_buttons_[BUTTON_B] = [[PadButton alloc] init];
    pad_buttons_[BUTTON_B].target_ = self.b_button;
    pad_buttons_[BUTTON_C] = [[PadButton alloc] init];
    pad_buttons_[BUTTON_C].target_ = self.c_button;
    pad_buttons_[BUTTON_X] = [[PadButton alloc] init];
    pad_buttons_[BUTTON_X].target_ = self.x_button;
    pad_buttons_[BUTTON_Y] = [[PadButton alloc] init];
    pad_buttons_[BUTTON_Y].target_ = self.y_button;
    pad_buttons_[BUTTON_Z] = [[PadButton alloc] init];
    pad_buttons_[BUTTON_Z].target_ = self.z_button;
    pad_buttons_[BUTTON_LEFT_TRIGGER] = [[PadButton alloc] init];
    pad_buttons_[BUTTON_LEFT_TRIGGER].target_ = self.left_trigger;
    pad_buttons_[BUTTON_RIGHT_TRIGGER] = [[PadButton alloc] init];
    pad_buttons_[BUTTON_RIGHT_TRIGGER].target_ = self.right_trigger;
    pad_buttons_[BUTTON_START] = [[PadButton alloc] init];
    pad_buttons_[BUTTON_START].target_ = self.start_button;
    
    
    
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
    
    GLView * gview = (GLView*)self.view;
    gview.context = g_context;
    gview.controller = self;
    
    
    //[self.view  addSubview:self.myGLKView];
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

   
    
    //self.preferredFramesPerSecond =120;
    
    
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
    
    self.scale_slider.hidden = YES;
    self.scale_slider.minimumValue = 0.5;
    self.scale_slider.maximumValue = 1.0;
    self.scale_slider.value = _controller_scale;
    [self.scale_slider addTarget:self action:@selector(didValueChanged:) forControlEvents:UIControlEventValueChanged];
    controller_edit_mode = 0;
    [self updateControllScale :_controller_scale];
    
}

- (void)updateControllScale:( float ) scale{
    
    UIView * lfv = self.left_view;
    CGAffineTransform tf = CGAffineTransformMakeScale(1.0,1.0);
    tf = CGAffineTransformTranslate(tf, -(258.0/2.0), (340.0/2.0));
    tf = CGAffineTransformScale(tf, scale, scale);
    tf = CGAffineTransformTranslate(tf, (258.0/2.0), -(340.0/2.0));
    lfv.transform = tf;
    
    UIView * rfv = self.right_view;
    tf = CGAffineTransformMakeScale(1.0,1.0);
    tf = CGAffineTransformTranslate(tf, (258.0/2.0), (340.0/2.0));
    tf = CGAffineTransformScale(tf, scale, scale);
    tf = CGAffineTransformTranslate(tf, -(258.0/2.0), -(340.0/2.0));
    rfv.transform = tf;
    
    NSUserDefaults *ud = [NSUserDefaults standardUserDefaults];
    [ud setFloat:scale forKey:@"controller scale"];
    [ud synchronize];

}


- (void)didValueChanged:( UISlider *)slider
{
    float scale_val = slider.value;
    [self updateControllScale :scale_val ];
}

- (void)toggleControllerEditMode {
    
    GLView * gview = (GLView*)self.view;
    if(controller_edit_mode==0){
        self.scale_slider.hidden = NO;
        controller_edit_mode = 1;
        [gview stopAnimation];
    }else{
        self.scale_slider.hidden = YES;
        controller_edit_mode = 0;
        [gview startAnimation];
    }
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

- (void)setup
{
    [EAGLContext setCurrentContext:self.context];
    GLView *view = (GLView *)self.view;
    view.context = self.context;
    view.contentScaleFactor = [UIScreen mainScreen].scale;
    [view bindDrawable ];
    
    _renderBuffer = view._renderBuffer;
    
    CGFloat scale = [[UIScreen mainScreen] scale];
    printf("viewport(%f,%f)\n",[view frame].size.width,[view frame].size.height);

    CGRect newFrame = self.view.frame;
    if( _aspect_rate ){
        int specw = self.view.frame.size.width;
        int spech = self.view.frame.size.height;
        float specratio = (float)specw / (float)spech;
        int saturnw = 4;
        int saturnh = 3;
        if( _rotate_screen == YES ){
            saturnw = 3;
            saturnh = 4;
        }
        float saturnraito = (float)saturnw/ (float)saturnh;
        float revraito = (float) saturnh/ (float)saturnw;
        
        if( specratio > saturnraito ){
            
            newFrame.size.width = spech * saturnraito;
            newFrame.size.height = spech;
            newFrame.origin.x = (self.view.frame.size.width - newFrame.size.width)/2.0;
            newFrame.origin.y = 0;
        }else{
            newFrame.size.width = specw ;
            newFrame.size.height = specw * revraito;
            newFrame.origin.x = 0;
            newFrame.origin.y = spech - newFrame.size.height;
        }
    }

    
    if( self._isFirst == YES){
        start_emulation(newFrame.origin.x*scale, newFrame.origin.y*scale, newFrame.size.width*scale,newFrame.size.height*scale);
        self._isFirst = NO;
        [view startAnimation];
    }else{
        resize_screen(newFrame.origin.x*scale, newFrame.origin.y*scale, newFrame.size.width*scale,newFrame.size.height*scale);
    }
    self._return = YES;
    
    //CADisplayLink *displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(update)];
    //[displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];

    //start_emulation(1920,1080);
    
}

- (void)tearDownGL
{
    [EAGLContext setCurrentContext:self.context];
   
}

#pragma mark - GLKView and GLKViewController delegate methods

- (void)emulate_one_frame
{
    
    if( self._return == YES ){
        GLView *view = (GLView *)self.view;
        GLint defaultFBO;
        [view bindDrawable];
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &defaultFBO);
        printf("DEFAULT FBO: %d\n", defaultFBO);
        self._return = NO;
        
    }
    
    emulation_step(self.command);
    self.command = 0;
    //[self.myGLKView setNeedsDisplay];
    

}


- (void)didEnterBackground {
    
    GLView *view = (GLView *)self.view;
    [view stopAnimation];
    enterBackGround();
    [self setPaused:true];
}

- (void)didBecomeActive {
    //if (self.view.active)
    [self.view becomeFirstResponder];
    
    [self setPaused:false];
    self._return = YES;
    
    GLView *view = (GLView *)self.view;
    [view startAnimation];
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
        [self setControllerOverlayHidden:YES];
    } else {
        self.isKeyRemappingMode = YES;
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
        [self setControllerOverlayHidden:YES];
        [self.remapAlertController dismissViewControllerAnimated:YES completion:nil];
        self.currentlyMappingKey = NSNotFound;
    }];
    UIAlertAction *unbind = [UIAlertAction actionWithTitle:@"Unbind" style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
        [self.keyMapper unmapKey:saturnKey];
        self.currentlyMappingKey = NSNotFound;
        [self refreshViewsWithKeyRemaps];
        [self.remapAlertController dismissViewControllerAnimated:YES completion:nil];
    }];
    self.isKeyRemappingMode = YES;
    [self.remapAlertController addAction:cancel];
    [self.remapAlertController addAction:unbind];
    self.currentlyMappingKey = saturnKey;
    [self presentViewController:self.remapAlertController animated:YES completion:nil];
}

-(void)updateSideMenu {
    GameRevealViewController *revealViewController = (GameRevealViewController *)self.revealViewController;
    if ( revealViewController )
    {
        SidebarViewController *svc = (SidebarViewController*) revealViewController.rearViewController;
        [svc refreshContents];
    }
}

-(BOOL)isCurrentlyRemappingControls {
    return self.isKeyRemappingMode;
}

@end
