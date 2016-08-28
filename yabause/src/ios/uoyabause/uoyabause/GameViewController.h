//
//  GameViewController.h
//  uoyabause
//
//  Created by MiyamotoShinya on 2016/02/06.
//  Copyright © 2016年 devMiyax. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>

@interface GameViewController : GLKViewController
{
    UInt32					iPodIsPlaying;
    NSString *selected_file;
}

@property (nonatomic, assign) UInt32		iPodIsPlaying;		// Whether the iPod is playing
@property (nonatomic, copy) NSString *selected_file;

@end
