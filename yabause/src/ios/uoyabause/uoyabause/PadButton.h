//
//  PadButton.h
//  uoyabause
//
//  Created by Shinya Miyamoto on 2018/05/01.
//  Copyright © 2018年 devMiyax. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIkit/UIView.h>

typedef NS_ENUM (NSUInteger, kPadButtons) {
    BUTTON_UP = 0,
    BUTTON_RIGHT = 1,
    BUTTON_DOWN = 2,
    BUTTON_LEFT = 3,
    BUTTON_RIGHT_TRIGGER = 4,
    BUTTON_LEFT_TRIGGER = 5,
    BUTTON_START = 6,
    BUTTON_A = 7,
    BUTTON_B = 8,
    BUTTON_C = 9,
    BUTTON_X = 10,
    BUTTON_Y = 11,
    BUTTON_Z = 12,
    BUTTON_LAST = 13
};

@interface PadButton : NSObject {
}
@property UIView * target_;
@property UITouch * is_on_;
@property UITouch * pointid_;
- (void)On :(UITouch *)index;
- (void)Off;
- (Boolean)isOn;
- (Boolean)isOn :(UITouch *)index;
- (UITouch *)getPointId;
@end
