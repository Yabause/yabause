//
//  SidebarViewController.m
//  SidebarDemo
//
//  Created by Simon on 29/6/13.
//  Copyright (c) 2013 Appcoda. All rights reserved.
//

#import "SidebarViewController.h"
#import "SWRevealViewController.h"
#import "GameRevealViewController.h"
#import "GameViewController.h"

@interface SidebarViewController ()

@property (nonatomic, strong) NSArray *menuItems;
@end

@implementation SidebarViewController {
    NSArray *menuItems;
}

- (id)initWithStyle:(UITableViewStyle)style
{
    self = [super initWithStyle:style];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];

}

- (void)viewWillAppear:(BOOL)animated
{
    NSLog( @"viewWillAppear" );
    GameRevealViewController *revealViewController = (GameRevealViewController *)self.revealViewController;
    if ( revealViewController )
    {
        GameViewController * view = (GameViewController * )[revealViewController frontViewController];
        if( view ){
            [view setPaused:YES];
        }
    }
}

-(void)viewDidDisappear:(BOOL)animated
{
    GameRevealViewController *revealViewController = (GameRevealViewController *)self.revealViewController;
    if ( revealViewController )
    {
        GameViewController * view = (GameViewController * )[revealViewController frontViewController];
        if( view ){
            [view setPaused:NO];
        }
    }
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    
    [tableView deselectRowAtIndexPath:indexPath animated:YES]; // 選択状態の解除をします。
    if(indexPath.section == 0) { // 1個目のセクションです。
        
        switch(indexPath.row){
            case 0:
            {
            
            UIAlertController *alertController = [UIAlertController alertControllerWithTitle:@"uoYabause" message:@"Are you sure you want to exit?" preferredStyle:UIAlertControllerStyleAlert];
            
            // addActionした順に左から右にボタンが配置されます
            [alertController addAction:[UIAlertAction actionWithTitle:@"Yes" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
                   exit(0);
            }]];
            [alertController addAction:[UIAlertAction actionWithTitle:@"No" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
            }]];
            
            [self presentViewController:alertController animated:YES completion:nil];
            break;
            }
            case 1:
            {
                GameRevealViewController *revealViewController = (GameRevealViewController *)self.revealViewController;
                if ( revealViewController )
                {
                    [revealViewController revealToggleAnimated:YES];
                    GameViewController * view = (GameViewController * )[revealViewController frontViewController];
                    if( view ){
                        [view saveState];
                    }
                }
                
                break;
            }
            case 2:
            {
                GameRevealViewController *revealViewController = (GameRevealViewController *)self.revealViewController;
                if ( revealViewController )
                {
                    [revealViewController revealToggleAnimated:YES];
                    GameViewController * view = (GameViewController * )[revealViewController frontViewController];
                    if( view ){
                        [view loadState];
                    }
                }
                
                break;
                
            }
        }
        
    }
    
    
    
    
    
    
    //exit(0);
#if 0
    UIAlertView *messageAlert = [[UIAlertView alloc]
                                 initWithTitle:@"Row Selected" message:@"You've selected a row" delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil];
    
    // Display Alert Message
    [messageAlert show];
#endif
    
}




@end
