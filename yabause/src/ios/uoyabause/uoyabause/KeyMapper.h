//
//  KeyMapper.h
//
//  Created by Yoshi Sugawara on 4/9/16.
//
//

#import <Foundation/Foundation.h>
#import "SaturnControllerKeys.h"

typedef NS_ENUM(NSInteger, KeyMapMappableButton) {
    MFI_BUTTON_X,
    MFI_BUTTON_A,
    MFI_BUTTON_B,
    MFI_BUTTON_Y,
    MFI_BUTTON_LT,
    MFI_BUTTON_RT,
    MFI_BUTTON_LS,
    MFI_BUTTON_RS,
    MFI_DPAD_UP,
    MFI_DPAD_DOWN,
    MFI_DPAD_LEFT,
    MFI_DPAD_RIGHT,
    MFI_BUTTON_HOME,
    MFI_BUTTON_MENU,
    MFI_BUTTON_OPTION
};

@interface KeyMapper : NSObject<NSCopying>

-(void)loadFromDefaults;
-(void) resetToDefaults;
-(void) saveKeyMapping;
-(void) mapKey:(SaturnKey)keyboardKey ToControl:(KeyMapMappableButton)button;
-(void) unmapKey:(SaturnKey)keyboardKey;
-(SaturnKey) getMappedKeyForControl:(KeyMapMappableButton)button;
+(NSString*) controlToDisplayName:(KeyMapMappableButton)button;
-(NSArray*) getControlsForMappedKey:(SaturnKey) keyboardKey;

@end
