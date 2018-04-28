//
//  GLView.h
//  uoyabause
//
//  Created by Shinya Miyamoto on 2018/04/29.
//  Copyright © 2018年 devMiyax. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#import <QuartzCore/QuartzCore.h>

@class GameViewController;

@interface GLView : UIView
{
}
@property (nonatomic, retain) IBOutlet GameViewController *controller;
@property (readonly, nonatomic, getter=isAnimating) BOOL animating;
@property (nonatomic) NSInteger animationFrameInterval;
@property (nonatomic, retain) EAGLContext *context;
@property (nonatomic) GLuint _renderBuffer;
- (void)startAnimation;
- (void)stopAnimation;
- (void)bindDrawable;
@end

