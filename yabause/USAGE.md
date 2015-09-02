        _   _          
       / \_/ \            ___  _      ____ 
       \     /___  ___   /   || | __ /    \ ____
        \   //   ||   \ /    || | \ \\  \_//    \
        /  //    ||   //  _  || |__\ \\  \    __/
        \_//  _  ||   \\_/ \_||______/ \  \\  \__
           \_/ \_||___/              \____/ \____\
     Yet Another Buggy And Uncomplete Saturn Emulator

           ____________________________________
           Copyright (c) 2002-2013 Yabause team

From where to run
=================

Depends of your system, yabause can run from a menu, from a terminal o simples way choosing from the main desktop.

Once u locate the downloaded and installed yabause port, and then run will be show a GUI or user interface.
The interface depending of your instalation can be two: a GTK or QT. The menus in both are very similar.

Menu
=====

Theres two king of menus depending of the gui interface, a qt menu and the gtk menu.

Menu qt
-------

* Ybause 
** Settings... :	 you can change lot of options (bios path, CD-ROM, video, sound, etc...).
** Open ISO...	 choose the directory of your game.
** Open CD Rom... :	 choose your CD-ROM drive (real saturn game) or a virtual CD-ROM (games mounted).
** Save State To File...	 allow to save where you want.
** Load State From File...	 allow to load a save state.
** Screenshot :	 make a screenshot of your game.
** Quit :	 quit Yabause.
* Emulation	
** Run :	 start a game.
** Pause	 pause the game.
** Reset	 reset a game.
** Frame Skip / Limiter	 limit the speed.
* Tools	 Backup Manager...	 Manage all your game saves
** Cheat List...	 Show the current list of enabled cheats
** Cheat Search...	 A search engine for cheats
** Transfer	 Transfer data between disk and memory.
** View	 FPS	 enable / disable the FPS counter.
** Layer	 enable / disable layers : Vdp1, NGB0, NGB1, NGB2, NGB3 and RGB0.
** Fullscreen	 switch between fullscreen mode and windowed mode.
** Log	 enable / disable the log.
* Debug
** Master SH2 :	 For programmers, allows you to debug SH2 code for Master SH2
** Slave SH2	 For programmers, allows you to debug SH2 code for Slave SH2
** VDP1	 For programmers, displays VDP1 sprites/polygons/etc. and debug information.
** VDP2	 For programmers, displays VDP2 screens and debug information.
** M68K	 For programmers, allows you to debug code for 68000
** SCU-DSP	 For programmers, allows you to debug code for SCU's DSP
** SCSP	 For programmers, displays information on SCSP
** Memory Editor	 For programmers, Hex Editor for editing Saturn memory.
* Help
** Emu-Compatibility :	 website about yabause compatibility list.
** About	 info about Yabause.

Menu gtk
--------

* Yabause 
** Settings... :	 you can change lot of options (bios path, CD-ROM, video, sound, etc...).
** Run :	 start a game if defined on the readers drivers.
** Pause	 pause the game.
** Reset	 reset a game.
** Transfer	 Transfer data between disk and memory.
** Screenshot :	 make a screenshot of your game.
** Frame Skip / Limiter	 limit the speed.
** Savestate:	 Saves/Restores the games state at current run.
** Quit :	 quit Yabause.
* View	 Backup Manager...	 Manage all your game saves
** FPS:		 enable / disable the FPS counter.
** Layer	 enable / disable layers : Vdp1, NGB0, NGB1, NGB2, NGB3 and RGB0.
** Fullscreen	 switch between fullscreen mode and windowed mode.
** Log	 enable / disable the log.
* Debug	 Master SH2 :	 For programmers, allows you to debug SH2 code for Master SH2
** Slave SH2	 For programmers, allows you to debug SH2 code for Slave SH2
** VDP1	 For programmers, displays VDP1 sprites/polygons/etc. and debug information.
** VDP2	 For programmers, displays VDP2 screens and debug information.
** M68K	 For programmers, allows you to debug code for 68000
** SCU-DSP	 For programmers, allows you to debug code for SCU's DSP
** SCSP	 For programmers, displays information on SCSP
** Memory dumper	 For programmers, dump Saturn memory.
* Help
** About	 info about Yabause.


Getting Started
================

To get started we need litle things, a disk game, the files where will stored and in rare cases a bios files.
1) First, go into "Yabause" menu, then into "Settings".
2) Set the Bios path if u have it. Please note Yabause can run some games without a BIOS, but most of them needs it. If you want to use the emulated BIOS, just leave the BIOS entry blank.
3) Next, set the emulated CD-ROM. It can be a CDROM device, or a support CD image file(See supported types).
4) Select the type from the drop list, then select the drive from the list if it's a CDROM device, or set the filename of the image file.
5) Then go to video tab and select software render. Note that OpenGL render need modern support.
6) Then push aAcept button to store the configured settings
7) Then go to Yabause Menu and select Run or go to Emulation menu and choose Run.

Later a thing you have to configure is the game pad configuration. Currently theres some issues with GTK gui.

configuration controllers
---------------

Click on the "Input" tab. Select the pad type for Port 1-1(most likely the one you want is Pad). 
Click on the wrench icon and a new window will open. 
Click on each of the buttons and press the key/button you want to use. 
When you're finished, press the close button.
If you want to adjust other settings, please refer to the Configuration section. 
Otherwise just press the "OK" button.

Once everything is set, you can start emulation with going into the "Emulation" menu, and then "Run".
