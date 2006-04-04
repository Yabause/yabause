//
//  CDDrivesManager.h
//  Yabausemac
//
//  Created by Michele Balistreri on 30/03/06.
//  Copyright 2006 Michele Balistreri. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <DiscRecording/DiscRecording.h>

@interface CDDrivesManager : NSObject 
{
	NSMutableDictionary *deviceDictionary;
}

- (NSArray *)deviceList;
- (unsigned int)deviceCount;
- (NSString *)devicePathWithName:(NSString *)name;

// Private

- (void)addDevice:(DRDevice *)device;
- (void)removeDevice:(DRDevice *)device;

@end
