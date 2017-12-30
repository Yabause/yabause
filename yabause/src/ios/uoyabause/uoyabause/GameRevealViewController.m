//
//  GameRevealViewController.m
//  uoyabause
//
//  Created by MiyamotoShinya on 2016/09/11.
//  Copyright © 2016年 devMiyax. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "GameRevealViewController.h"
#import "GameViewController.h"

@implementation  GameRevealViewController
@synthesize selected_file;

- (void)loadView
{
    [super loadView];
    
}

//パラメータのセット(option)
-(void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender{
    if ([segue.identifier isEqualToString:@"back_to_game_selection"]) {
        //ViewController *vcA=(ViewAController*)segue.destinationViewController;
    }
    //vcA.foo=bar;
    //[vcA baz];
}

//unwind
-(IBAction)unwindForSegue:(UIStoryboardSegue *)unwindSegue towardsViewController:(UIViewController *)subsequentVC{
}

@end
