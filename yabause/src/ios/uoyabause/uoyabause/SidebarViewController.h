//
//  SidebarViewController.h
//  SidebarDemo
//
//  Created by Simon on 29/6/13.
//  Copyright (c) 2013 Appcoda. All rights reserved.
//

#import <UIKit/UIKit.h>
@import GoogleMobileAds;

@interface SidebarViewController : UITableViewController  <UITableViewDelegate, UITableViewDataSource, GADInterstitialDelegate>
@property (weak, nonatomic) IBOutlet GADBannerView *banner;
@property(nonatomic, strong) GADInterstitial *interstitial;

@property (strong, nonatomic) IBOutlet UILabel *showRemapControlsLabel;

-(void)refreshContents;

@end
