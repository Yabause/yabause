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


@interface GameViewController () {
    GLuint _program;
    
    GLKMatrix4 _modelViewProjectionMatrix;
    GLKMatrix3 _normalMatrix;
    float _rotation;
    
    GLuint _vertexArray;
    GLuint _vertexBuffer;
    
}
@property (strong, nonatomic) EAGLContext *context;
@property (nonatomic, strong) GCController *controller;

- (void)setupGL;
- (void)tearDownGL;

@end

@implementation GameViewController

// C "trampoline" function to invoke Objective-C method
int swapAglBuffer ()
{
//    EAGLContext* context = [EAGLContext currentContext];
//    [context presentRenderbuffer:GL_RENDERBUFFER];
    return 0;
}

const char * GetBiosPath(){
    NSString * path = [[NSBundle mainBundle] pathForResource:  @"bios" ofType: @"bin"];
    return [path cStringUsingEncoding:1];
}

const char * GetGamePath(){
    NSString * path = [[NSBundle mainBundle] pathForResource:  @"nights" ofType: @"iso"];
    return [path cStringUsingEncoding:1];
}

const char * GetMemoryPath(){
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *docs_dir = [paths objectAtIndex:0];
    NSString* aFile = [docs_dir stringByAppendingPathComponent: @"somedocthatdefinitelyexists.doc"];
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

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];

    if (!self.context) {
        NSLog(@"Failed to create ES context");
    }
    
    GLKView *view = (GLKView *)self.view;
    view.context = self.context;
    view.drawableDepthFormat = GLKViewDrawableDepthFormat24;
    view.drawableStencilFormat = GLKViewDrawableStencilFormat8;

    self.preferredFramesPerSecond =60;

    if( [GCController controllers].count == 1 ){
    self.controller = [GCController controllers][0];
    if (self.controller.gamepad) {
        
        [self.controller.gamepad.buttonA setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
            if(pressed){
                PerKeyDown(PERPAD_A);
            }else{
                PerKeyUp(PERPAD_A);
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
    }
  }
  
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
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    emulation_step();
}



@end
