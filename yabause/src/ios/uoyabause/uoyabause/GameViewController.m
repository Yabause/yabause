//
//  GameViewController.m
//  uoyabause
//
//  Created by MiyamotoShinya on 2016/02/06.
//  Copyright © 2016年 devMiyax. All rights reserved.
//

#import "GameViewController.h"
#import <OpenGLES/ES2/glext.h>
@import GameController;

#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AudioToolbox/ExtendedAudioFile.h>

/** @defgroup pad Pad
 *
 * @{
 */
#define PERPAD_UP	0
#define PERPAD_RIGHT	1
#define PERPAD_DOWN	2
#define PERPAD_LEFT	3
#define PERPAD_RIGHT_TRIGGER 4
#define PERPAD_LEFT_TRIGGER 5
#define PERPAD_START	6
#define PERPAD_A	7
#define PERPAD_B	8
#define PERPAD_C	9
#define PERPAD_X	10
#define PERPAD_Y	11
#define PERPAD_Z	12
void PerKeyDown(unsigned int key);
void PerKeyUp(unsigned int key);


int start_emulation( int width, int height );
int emulation_step();

EAGLContext *g_context = nil;
EAGLContext *g_share_context = nil;


@interface GameViewController () {
    GLuint _program;
    
    GLKMatrix4 _modelViewProjectionMatrix;
    GLKMatrix3 _normalMatrix;
    float _rotation;
    
    GLuint _vertexArray;
    GLuint _vertexBuffer;

    
}
@property (strong, nonatomic) EAGLContext *context;
@property (strong, nonatomic) EAGLContext *share_context;
@property (nonatomic, strong) GCController *controller;

- (void)setupGL;
- (void)tearDownGL;

@end

@implementation GameViewController
@synthesize iPodIsPlaying;
static GameViewController *sharedData_ = nil;

// C "trampoline" function to invoke Objective-C method
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
    NSString * path = [[NSBundle mainBundle] pathForResource:  @"bios" ofType: @"bin"];
    return [path cStringUsingEncoding:1];
    //return NULL;
}

const char * GetGamePath(){
    
    if( sharedData_ == nil ){
        return nil;
    }
    //NSString * path = [[NSBundle mainBundle] pathForResource:  @"nights" ofType: @"iso"];
    //NSString * path = [[NSBundle mainBundle] pathForResource:  @"gd" ofType: @"cue"];
    NSString *path = sharedData_.selected_file; //[NSString stringWithFormat:@"%@/gd.cue" , [NSHomeDirectory() stringByAppendingPathComponent:@"Documents"]];
    
    return [path cStringUsingEncoding:1];

}

const char * GetMemoryPath(){
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *docs_dir = [paths objectAtIndex:0];
    NSString* aFile = [docs_dir stringByAppendingPathComponent: @"memory2.bin"];
    return [aFile fileSystemRepresentation];
}

int GetCartridgeType(){
    return 0;
}
int GetVideoInterface(){
    return 0;
}
const char * GetCartridgePath(){
    return NULL;
}

int GetPlayer2Device(){
    return -1;
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    //if ([touches count] &lt; 1) return;
    //CGPoint point = [[touches anyObject] locationInView:[self view]];
    PerKeyDown(PERPAD_START);
    //CubeWorld_touch_down(world, point.x, point.y);
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    //if ([touches count] &lt; 1) return;
    //CGPoint point = [[touches anyObject] locationInView:[self view]];
    //CubeWorld_touch_move(world, point.x, point.y);
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    //if ([touches count] &lt; 1) return;
    //CGPoint point = [[touches anyObject] locationInView:[self view]];
    //CubeWorld_touch_up(world, point.x, point.y);
    PerKeyUp(PERPAD_START);
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
    
}

- (void)controllerDidDisconnect
{
    
}

-(void)completionWirelessControllerDiscovery
{
    if( [GCController controllers].count >= 1 ){
        self.controller = [GCController controllers][0];
        if (self.controller.gamepad) {
            
            [self.controller.gamepad.buttonA setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                if(pressed){
                    PerKeyDown(PERPAD_A);
                }else{
                    PerKeyUp(PERPAD_A);
                }
            }];
            
            [self.controller.extendedGamepad.rightShoulder setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                if(pressed){
                    PerKeyDown(PERPAD_C);
                }else{
                    PerKeyUp(PERPAD_C);
                }
            }];
            
            [self.controller.extendedGamepad.leftShoulder setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                if(pressed){
                    PerKeyDown(PERPAD_Z);
                }else{
                    PerKeyUp(PERPAD_Z);
                }
            }];
            
            [self.controller.extendedGamepad.leftTrigger setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                if(pressed){
                    PerKeyDown(PERPAD_LEFT_TRIGGER);
                }else{
                    PerKeyUp(PERPAD_LEFT_TRIGGER);
                }
            }];
            
            [self.controller.extendedGamepad.rightTrigger setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                if(pressed){
                    PerKeyDown(PERPAD_RIGHT_TRIGGER);
                }else{
                    PerKeyUp(PERPAD_RIGHT_TRIGGER);
                }
            }];
            
            [self.controller.gamepad.buttonX setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                if(pressed){
                    PerKeyDown(PERPAD_X);
                }else{
                    PerKeyUp(PERPAD_X);
                }
            }];
            [self.controller.gamepad.buttonY setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                if(pressed){
                    PerKeyDown(PERPAD_Y);
                }else{
                    PerKeyUp(PERPAD_Y);
                }
            }];
            [self.controller.gamepad.buttonB setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                if(pressed){
                    PerKeyDown(PERPAD_B);
                }else{
                    PerKeyUp(PERPAD_B);
                }
            }];
            [self.controller.gamepad.dpad.up setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                if(pressed){
                    PerKeyDown(PERPAD_UP);
                }else{
                    PerKeyUp(PERPAD_UP);
                }
            }];
            [self.controller.gamepad.dpad.down setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                if(pressed){
                    PerKeyDown(PERPAD_DOWN);
                }else{
                    PerKeyUp(PERPAD_DOWN);
                }
            }];
            [self.controller.gamepad.dpad.left setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                if(pressed){
                    PerKeyDown(PERPAD_LEFT);
                }else{
                    PerKeyUp(PERPAD_LEFT);
                }
            }];
            [self.controller.gamepad.dpad.right setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                if(pressed){
                    PerKeyDown(PERPAD_RIGHT);
                }else{
                    PerKeyUp(PERPAD_RIGHT);
                }
            }];
            [self.controller.extendedGamepad.rightThumbstick setValueChangedHandler:^(GCControllerDirectionPad *dpad, float xValue, float yValue) {
                if(yValue >= 0.5 || yValue <= -0.5 ){
                    PerKeyDown(PERPAD_START);
                }else{
                    PerKeyUp(PERPAD_START);
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
   // [super viewDidAppear];
   // [self completionWirelessControllerDiscovery];
    
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
    
    //if (self.controllerCallbackSetup) {
    //    self.controllerCallbackSetup([[GCController controllers] firstObject]);
    //}
    //[self stop];  
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(controllerDidDisconnect)
                                                 name:GCControllerDidDisconnectNotification
                                               object:nil];
    
    [self completionWirelessControllerDiscovery];
}


- (void)viewDidLoad
{
    sharedData_ = self;
    [super viewDidLoad];
    
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
     [view bindDrawable ];
    
    start_emulation(1920,1080);
    
}

- (void)tearDownGL
{
    [EAGLContext setCurrentContext:self.context];
   
}

#pragma mark - GLKView and GLKViewController delegate methods

- (void)update
{
    emulation_step();
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    
}

- (void)didEnterBackground {
    
     GLKView *view = (GLKView *)self.view;
    
    //if (view.active)
        [view resignFirstResponder];
}

- (void)didBecomeActive {
    //if (self.view.active)
        [self.view becomeFirstResponder];
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

@end
