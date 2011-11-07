/*  Copyright 2010 Lawrence Sebald

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include <sched.h>
#include <stdio.h>
#include <stdlib.h>

#include "YabauseController.h"
#include "YabauseGLView.h"
#include "YabausePrefsController.h"

#include "vdp1.h"
#include "vdp2.h"
#include "scsp.h"
#include "peripheral.h"
#include "cdbase.h"
#include "yabause.h"
#include "yui.h"
#include "PerCocoa.h"
#include "m68kc68k.h"
#include "cs0.h"

YabauseController *controller;

@interface YabauseController (InternalFunctions)
- (void)startEmulationWithCDCore:(int)cdcore;
- (void)emulationThread:(id)ignored;
- (void)terminateEmulation;
@end

/* Menu Item tags. */
enum {
    tagVDP1 = 1,
    tagNBG0 = 2,
    tagNBG1 = 3,
    tagNBG2 = 4,
    tagNBG3 = 5,
    tagRBG0 = 6,
    tagFPS  = 7
};

static void FlipToggle(NSMenuItem *item) {
    if([item state] == NSOffState) {
        [item setState:NSOnState];
    }
    else {
        [item setState:NSOffState];
    }
}

@implementation YabauseController

- (void)awakeFromNib
{
    controller = self;
    _running = NO;
    _paused = NO;
    _runLock = [[NSLock alloc] init];
    _emuThd = nil;
    _bramFile = NULL;
    _doneExecuting = NO;
}

- (void)dealloc
{
    [_runLock release];
    [super dealloc];
}

- (BOOL)windowShouldClose:(id)sender
{
    [self terminateEmulation];
    return YES;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)app
{
    [self terminateEmulation];
    return NSTerminateNow;
}

- (IBAction)showPreferences:(id)sender
{
    [prefsPane makeKeyAndOrderFront:self];
}

- (IBAction)runBIOS:(id)sender
{
    /* This will simply start up the system with the dummy CD core, so there's
       no way it'll actually read that there's a disc to be played. */
    [self startEmulationWithCDCore:CDCORE_DUMMY];
}

- (IBAction)runCD:(id)sender
{
    [self startEmulationWithCDCore:CDCORE_ARCH];
}

- (IBAction)toggleFullscreen:(id)sender
{
    /* The view handles any heavy lifting here... */
    [view toggleFullscreen];
}

- (IBAction)toggle:(id)sender
{
    /* None of these will work unless we're running... */
    if(!_running) {
        return;
    }

    /* Flip the checkmark on the button. */
    FlipToggle((NSMenuItem *)sender);

    /* Do whatever this toggle is asking us to do. */
    switch([sender tag]) {
        case tagVDP1:
            ToggleVDP1();
            break;

        case tagNBG0:
            ToggleNBG0();
            break;

        case tagNBG1:
            ToggleNBG1();
            break;

        case tagNBG2:
            ToggleNBG2();
            break;

        case tagNBG3:
            ToggleNBG3();
            break;

        case tagRBG0:
            ToggleRBG0();
            break;

        case tagFPS:
            ToggleFPS();
            break;
    }
}

- (IBAction)toggleFrameskip:(id)sender
{
    if([sender state] == NSOnState) {
        DisableAutoFrameSkip();
        [sender setState:NSOffState];
    }
    else {
        EnableAutoFrameSkip();
        [sender setState:NSOnState];
    }
}

- (IBAction)pause:(id)sender
{
    if(_running) {
        if(!_paused) {
            _paused = YES;

            /* Mute the audio before we actually pause otherwise the user might
               not like the result... */
            ScspMuteAudio();
            [_runLock lock];
            [sender setState:NSOnState];
        }
        else {
            _paused = NO;
            [_runLock unlock];
            ScspUnMuteAudio();
            [sender setState:NSOffState];
        }
    }
}

- (IBAction)reset:(id)sender
{
    if(_running) {
        /* Act as if the user pressed the reset button on the console. */
        YabauseResetButton();
    }
}

- (YabauseGLView *)view
{
    return view;
}

@end /* @implementation YabauseController */

@implementation YabauseController (InternalFunctions)

- (void)startEmulationWithCDCore:(int)cdcore
{
    if(!_running) {
        yabauseinit_struct yinit;
        NSString *bios = [prefs biosPath];
        NSString *mpeg = [prefs mpegPath];
        NSString *bram = [prefs bramPath];
        NSString *cart = [prefs cartPath];

        yinit.percoretype = PERCORE_COCOA;
        yinit.sh2coretype = SH2CORE_DEFAULT;
        yinit.vidcoretype = [prefs videoCore];
        yinit.sndcoretype = [prefs soundCore];
        yinit.m68kcoretype = M68KCORE_C68K;
        yinit.cdcoretype = cdcore;
        yinit.carttype = [prefs cartType];
        yinit.regionid = [prefs region];
        yinit.biospath = ([bios length] > 0 && ![prefs emulateBios]) ?
            [bios UTF8String] : NULL;
        yinit.cdpath = NULL;
        yinit.buppath = NULL;
        yinit.mpegpath = ([mpeg length] > 0) ? [mpeg UTF8String] : NULL;
        yinit.videoformattype = ([prefs region] < 10) ? VIDEOFORMATTYPE_NTSC :
            VIDEOFORMATTYPE_PAL;
        yinit.frameskip = [frameskip state] == NSOnState;
        yinit.clocksync = 0;
        yinit.basetime = 0;
        yinit.usethreads = 0;

        /* Set up the internal save ram if specified. */
        if([bram length] > 0) {
            const char *tmp = [bram UTF8String];

            _bramFile = (char *)malloc(strlen(tmp) + 1);
            strcpy(_bramFile, tmp);
            yinit.buppath = _bramFile;
        }

        /* Set up the cartridge stuff based on what was selected. */
        if(yinit.carttype == CART_NETLINK) {
            yinit.cartpath = NULL;
            yinit.netlinksetting = ([cart length] > 0) ?
                [cart UTF8String] : NULL;
        }
        else {
            yinit.cartpath = ([cart length] > 0) ? [cart UTF8String] : NULL;
            yinit.netlinksetting = NULL;
        }

        if(cdcore == CDCORE_DUMMY && !yinit.biospath) {
            NSRunAlertPanel(@"Yabause Error", @"You must specify a BIOS file "
                            "(and have BIOS emulation disabled) in order to "
                            "run the BIOS.", @"OK", NULL, NULL);
            return;
        }

        YabauseInit(&yinit);
        YabauseSetDecilineMode(0);

        _running = YES;
        _doneExecuting = NO;

        [view showWindow];

        /* The emulation itself takes place in a separate thread from the main
           GUI thread. */
        [NSThread detachNewThreadSelector:@selector(emulationThread:)
                                 toTarget:self
                               withObject:nil];
    }
}

- (void)emulationThread:(id)ignored
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    CGLContextObj cxt;

    _emuThd = [NSThread currentThread];

    /* Make the OpenGL context current for this thread, otherwise we will be
       drawing to nothingness. */
    [[view openGLContext] makeCurrentContext];

    ScspUnMuteAudio();

    while(_running) {
        /* If we get paused from the GUI, we'll end up waiting in this lock
           here... Maybe not the most clear way to do it, but it works. */
        [_runLock lock];

        /* Make sure the main thread doesn't attempt to flip the buffer before
           this thread is done rendering. */
        cxt = CGLGetCurrentContext();
        CGLLockContext(cxt);

        /* Shortcut a function call here... We should technically be doing a
           PERCore->HandleEvents(), but that function simply calls YabauseExec()
           anyway... so cut out the middleman. */
        YabauseExec();

        CGLUnlockContext(cxt);
        [_runLock unlock];
    }

    ScspMuteAudio();

    _doneExecuting = YES;
    [pool release];
}

- (void)terminateEmulation
{
    _running = NO;

    /* Wait for the thread to die, and then clean up after it. */
    if(_emuThd) {
        while(!_doneExecuting) {
            sched_yield();
        }

        YabauseDeInit();

        if(_bramFile) {
            free(_bramFile);
            _bramFile = NULL;
        }

        _emuThd = nil;
    }
}

@end /* @implementation YabauseController (InternalFunctions) */
