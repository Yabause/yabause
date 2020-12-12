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
    int _ad_showing;
}



- (id)initWithStyle:(UITableViewStyle)style
{
    self = [super initWithStyle:style];
    if (self) {
        // Custom initialization
    }
    _ad_showing = 0;
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    NSString* path = [[NSBundle mainBundle] pathForResource:@"apikey" ofType:@"plist"];
    NSDictionary *dic = [NSDictionary dictionaryWithContentsOfFile:path];
    NSString* keyval = [dic objectForKey:@"ADMOB_KEY"];
    NSArray* plist = [NSArray arrayWithContentsOfFile:path];
    self.banner.adUnitID = keyval;
    self.banner.rootViewController = self;
    GADRequest *request = [GADRequest request];
    //request.testDevices = @[@"2077ef9a63d2b398840261c8221a0c9b"];
    [self.banner loadRequest:request];
   
    NSString* keyval_in = [dic objectForKey:@"ADMOB_KEY_FULLSCREEN"];
    self.interstitial =
    [[GADInterstitial alloc] initWithAdUnitID:keyval_in];
    self.interstitial.delegate = self;
    GADRequest *request_in = [GADRequest request];
    // Request test ads on devices you specify. Your test device ID is printed to the console when
    // an ad request is made.
    //request.testDevices = @[ kGADSimulatorID, @"2077ef9a63d2b398840261c8221a0c9b" ];
    [self.interstitial loadRequest:request_in];

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
        if( view && !_ad_showing){
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
    
    if(indexPath.section == 0) { // 1個目のセクションです。
        
        switch(indexPath.row){
            case 0:
            {
 
                
            
            UIAlertController *alertController = [UIAlertController alertControllerWithTitle:@"uoYabause" message:@"Are you sure you want to exit?" preferredStyle:UIAlertControllerStyleAlert];
            
                // addActionした順に左から右にボタンが配置されます
                [alertController addAction:[UIAlertAction actionWithTitle:@"Yes" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
                    if (self.interstitial.isReady) {
                        _ad_showing = 1;
                        [self.interstitial presentFromRootViewController:self];
                    } else {
                        exit(0);
                    }
                }]];
                [alertController addAction:[UIAlertAction actionWithTitle:@"No" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
                    [tableView deselectRowAtIndexPath:indexPath animated:YES]; // 選択状態の解除をします。
                }]];
            
            [self presentViewController:alertController animated:YES completion:nil];

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
                        [view saveState];
                    }
                }
                [tableView deselectRowAtIndexPath:indexPath animated:YES]; // 選択状態の解除をします。
                
                break;
            }
            case 3:
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
                [tableView deselectRowAtIndexPath:indexPath animated:YES]; // 選択状態の解除をします。
                break;
                
            }
                
            case 4:
            {
                GameRevealViewController *revealViewController = (GameRevealViewController *)self.revealViewController;
                if ( revealViewController )
                {
                    [revealViewController revealToggleAnimated:YES];
                    GameViewController * view = (GameViewController * )[revealViewController frontViewController];
                    if( view ){
                        if ( [view isCurrentlyRemappingControls] ) {
                            self.showRemapControlsLabel.text = @"Show Remap Controls";
                        } else {
                            self.showRemapControlsLabel.text = @"Hide Remap Controls";
                        }
                        [view startKeyRemapping:nil];
                    }
                }
                [tableView deselectRowAtIndexPath:indexPath animated:YES]; // 選択状態の解除をします。
                break;
            }
            case 5:
            {
                GameRevealViewController *revealViewController = (GameRevealViewController *)self.revealViewController;
                if ( revealViewController )
                {
                    [revealViewController revealToggleAnimated:YES];
                    GameViewController * view = (GameViewController * )[revealViewController frontViewController];
                    if( view ){
                        [view toggleControllerEditMode];
                    }
                }
                [tableView deselectRowAtIndexPath:indexPath animated:YES]; // 選択状態の解除をします。
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

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
    // hide the remap controls overlay if there is no mfi controller present
    if ( indexPath.row == 4 ) {
        GameRevealViewController *revealViewController = (GameRevealViewController *)self.revealViewController;
        GameViewController * vc = (GameViewController * )[revealViewController frontViewController];
        if ( ![vc hasControllerConnected] ) {
            return 0.0;
        }
    }
    return 44.0;
}


- (void)interstitialDidDismissScreen:(GADInterstitial *)ad {
    exit(0);
}

-(void)refreshContents {
    [self.tableView reloadData];
}

@end
