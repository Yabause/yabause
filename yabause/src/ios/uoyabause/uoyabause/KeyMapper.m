//
//  KeyMapper.m
//
//  Created by Yoshi Sugawara on 4/9/16.
//
//

#import "KeyMapper.h"
#import "SaturnControllerKeys.h"

@interface  KeyMapper()
@property (nonatomic, strong) NSMutableDictionary *keyMapping;
@end

@implementation KeyMapper

-(void)loadFromDefaults {
    NSData *data = [[NSUserDefaults standardUserDefaults] objectForKey:@"keyMapping"];
    if ( data == nil || ![data isKindOfClass:[NSData class]] ) {
        self.keyMapping = [self defaultMapping];
    } else {
        NSDictionary *fetchedDict = [NSKeyedUnarchiver unarchiveObjectWithData:data];
        self.keyMapping = [fetchedDict mutableCopy];
    }
}

- (id)copyWithZone:(NSZone *)zone {
    KeyMapper *copy = [[[self class] alloc] init];
    copy.keyMapping = [self.keyMapping mutableCopy];
    return copy;
}

-(NSMutableDictionary*) defaultMapping {
    return [@{ [NSNumber numberWithInteger:MFI_BUTTON_X] : [NSNumber numberWithInteger:SaturnKeyA],
               [NSNumber numberWithInteger:MFI_BUTTON_A] : [NSNumber numberWithInteger:SaturnKeyB],
               [NSNumber numberWithInteger:MFI_BUTTON_B] : [NSNumber numberWithInteger:SaturnKeyC],
               [NSNumber numberWithInteger:MFI_BUTTON_Y] : [NSNumber numberWithInteger:SaturnKeyX],
               [NSNumber numberWithInteger:MFI_BUTTON_LT] : [NSNumber numberWithInteger:SaturnKeyY],
               [NSNumber numberWithInteger:MFI_BUTTON_RT] : [NSNumber numberWithInteger:SaturnKeyZ],
               [NSNumber numberWithInteger:MFI_BUTTON_LS] : [NSNumber numberWithInteger:SaturnKeyLeftTrigger],
               [NSNumber numberWithInteger:MFI_BUTTON_RS] : [NSNumber numberWithInteger:SaturnKeyRightTrigger],
               [NSNumber numberWithInteger:MFI_BUTTON_HOME] : [NSNumber numberWithInteger:SaturnKeyStart],
               
               } mutableCopy];
}

-(void) resetToDefaults {
    self.keyMapping = [self defaultMapping];
}

-(void) saveKeyMapping {
    [[NSUserDefaults standardUserDefaults] setObject:[NSKeyedArchiver archivedDataWithRootObject:self.keyMapping] forKey:@"keyMapping"];
    [[NSUserDefaults standardUserDefaults] synchronize];
}

-(void) mapKey:(SaturnKey)keyboardKey ToControl:(KeyMapMappableButton)button {
    NSNumber *buttonKey = [NSNumber numberWithInteger:button];
    [self.keyMapping setObject:[NSNumber numberWithInteger:keyboardKey] forKey:buttonKey];
}

-(void) unmapKey:(SaturnKey)saturnKey {
    NSArray *mappedButtons = [self getControlsForMappedKey:saturnKey];
    for (NSNumber *button in mappedButtons) {
        [self.keyMapping removeObjectForKey:button];
    }
}

-(SaturnKey) getMappedKeyForControl:(KeyMapMappableButton)button {
    NSNumber *buttonKey = [NSNumber numberWithInteger:button];
    NSNumber *mappedKey = [self.keyMapping objectForKey:buttonKey];
    if ( mappedKey != nil ) {
        return [mappedKey intValue];
    } else {
        return NSNotFound;
    }
}

-(NSArray*) getControlsForMappedKey:(SaturnKey) saturnKey {
    NSMutableArray *foundControls = [NSMutableArray array];
    for (NSNumber *buttonKey in self.keyMapping) {
        NSNumber *mappedKey = [self.keyMapping objectForKey:buttonKey];
        if ( mappedKey != nil && [mappedKey integerValue] == saturnKey ) {
            [foundControls addObject:buttonKey];
        }
    }
    return foundControls;
}

+(NSString*) controlToDisplayName:(KeyMapMappableButton)button {
    switch (button) {
        case MFI_BUTTON_HOME:
            return @"H";
            break;
        case MFI_BUTTON_OPTION:
            return @"H";
            break;
        case MFI_BUTTON_MENU:
            return @"M";
            break;
        case MFI_BUTTON_A:
            return @"A";
            break;
        case MFI_BUTTON_B:
            return @"B";
            break;
        case MFI_BUTTON_X:
            return @"X";
            break;
        case MFI_BUTTON_Y:
            return @"Y";
            break;
        case MFI_BUTTON_LS:
            return @"LS";
            break;
        case MFI_BUTTON_LT:
            return @"LT";
            break;
        case MFI_BUTTON_RS:
            return @"RS";
            break;
        case MFI_BUTTON_RT:
            return @"RT";
            break;
        case MFI_DPAD_UP:
            return @"⬆️";
            break;
        case MFI_DPAD_DOWN:
            return @"⬇️";
            break;
        case MFI_DPAD_LEFT:
            return @"⬅️";
            break;
        case MFI_DPAD_RIGHT:
            return @"➡️";
            break;
        default:
            return @"?";
            break;
    }
}


@end
