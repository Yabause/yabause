//
//  CDDrivesManager.m
//  Yabausemac
//
//  Created by Michele Balistreri on 30/03/06.
//  Copyright 2006 Michele Balistreri. All rights reserved.
//

#import "CDDrivesManager.h"
#include <paths.h>

NSString *DevicePath = @_PATH_DEV;

@implementation CDDrivesManager

+ (BOOL)automaticallyNotifiesObserversForKey:(NSString *)theKey 
{
	return NO;
}

- (id)init
{
	self = [super init];
	
	if(self)
	{
		deviceDictionary = [[NSMutableDictionary alloc] init];
		[[DRNotificationCenter currentRunLoopCenter] addObserver:self selector:@selector(deviceDisappeared:) name:DRDeviceDisappearedNotification object:nil];
		[[DRNotificationCenter currentRunLoopCenter] addObserver:self selector:@selector(deviceAppeared:) name:DRDeviceAppearedNotification object:nil];
	}
	
	return self;
}

- (void)dealloc
{
	[deviceDictionary release];
	[super dealloc];
}

- (unsigned int)deviceCount
{
	return [deviceDictionary count];
}

- (NSArray *)deviceList
{
	return [deviceDictionary allKeys];
}

- (NSString *)devicePathWithName:(NSString *)name
{
	return [deviceDictionary objectForKey:name];
}

- (void)deviceAppeared:(NSNotification *)aNotification
{
	[[DRNotificationCenter currentRunLoopCenter] addObserver:self selector:@selector(deviceStateChanged:) name:DRDeviceStatusChangedNotification object:[aNotification object]];
}

- (void)deviceDisappeared:(NSNotification *)aNotification
{
	[[DRNotificationCenter currentRunLoopCenter] removeObserver:self name:DRDeviceStatusChangedNotification object:[aNotification object]];
}

- (void)deviceStateChanged:(NSNotification *) aNotification
{
	if([[aNotification object] mediaIsPresent])
		[self addDevice:[aNotification object]];
	else
		[self removeDevice:[aNotification object]];
}

- (void)addDevice:(DRDevice *)device
{
	[self willChangeValueForKey:@"deviceList"];
	[self willChangeValueForKey:@"deviceCount"];
	
	NSString *devPath = [DevicePath stringByAppendingPathComponent:[@"r" stringByAppendingString:[device bsdName]]];
	[deviceDictionary setObject:devPath forKey:[device displayName]];
	
	[self didChangeValueForKey:@"deviceList"];
	[self didChangeValueForKey:@"deviceCount"];
}

- (void)removeDevice:(DRDevice *)device
{
	[self willChangeValueForKey:@"deviceList"];
	[self willChangeValueForKey:@"deviceCount"];
	
	[deviceDictionary removeObjectForKey:[device displayName]];
	
	[self didChangeValueForKey:@"deviceList"];
	[self didChangeValueForKey:@"deviceCount"];
}
@end
