//
//  PadButton.m
//  uoyabause
//
//  Created by Shinya Miyamoto on 2018/05/01.
//  Copyright © 2018年 devMiyax. All rights reserved.
//

#import "PadButton.h"

@implementation PadButton

-(id) init
{
    if(self = [super init]){
        self.is_on_ = nil;
    }
    return self;
}

- (void)On :(UITouch *)index {
    self.is_on_ = index;
    
}

- (void)Off {
    self.is_on_ = (UITouch *)nil;
}

- (Boolean)isOn {
    if( self.is_on_ != (UITouch *)nil ){
        return TRUE;
    }
    return FALSE;
}

- (Boolean)isOn :(UITouch *)index {
    if( self.is_on_ != index ){
        return TRUE;
    }
    return FALSE;
}

- (UITouch *) getPointId {
    return self.is_on_;
}


@end
