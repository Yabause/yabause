//
//  GameRevealViewController.h
//  uoyabause
//
//  Created by MiyamotoShinya on 2016/09/11.
//  Copyright © 2016年 devMiyax. All rights reserved.
//

#ifndef GameRevealViewController_h
#define GameRevealViewController_h

#import <UIKit/UIKit.h>
#import "SWRevealViewController.h"

@interface GameRevealViewController : SWRevealViewController
{
 NSString *selected_file;
    
}
@property (nonatomic, copy) NSString *selected_file;

@end
#endif /* GameRevealViewController_h */
