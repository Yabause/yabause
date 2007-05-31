/*  Copyright 2004-2006 Theo Berkau

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#define IDR_MENU             10000
#define IDI_ICON             10001
#define IDB_BITMAP           10002
#define IDR_MAIN_ACCEL       10003
#define IDM_CHOOSEBIOS       10004
#define IDM_CHOOSECDROM      10005
#define IDM_MEMTRANSFER      10006
#define IDM_RUN              10007
#define IDM_PAUSE            10008
#define IDM_RESET            10009
#define IDM_SETTINGS         10010
#define IDM_EXIT             10011
#define IDM_SEARCHCHEATS     10020
#define IDM_CHEATLIST        10021
#define IDM_MSH2DEBUG        10030
#define IDM_SSH2DEBUG        10031
#define IDM_VDP1DEBUG        10032
#define IDM_VDP2DEBUG        10033
#define IDM_M68KDEBUG        10034
#define IDM_SCUDSPDEBUG      10035
#define IDM_SCSPDEBUG        10036
#define IDM_MEMORYEDITOR     10037
#define IDM_TOGGLEFULLSCREEN 10040
#define IDM_TOGGLENBG0       10041
#define IDM_TOGGLENBG1       10042
#define IDM_TOGGLENBG2       10043
#define IDM_TOGGLENBG3       10044
#define IDM_TOGGLERBG0       10045
#define IDM_TOGGLEVDP1       10046
#define IDM_TOGGLEFPS        10047
#define IDM_WEBSITE          10050
#define IDM_FORUM            10051
#define IDM_SUBMITBUGREPORT  10052
#define IDM_DONATE           10053
#define IDM_COMPATLIST       10054
#define IDM_ABOUT            10055

#define IDC_EDITTEXT1                   1001
#define IDC_EDITTEXT2                   1002
#define IDC_EDITTEXT3                   1003

#define IDC_LISTBOX1                    1004
#define IDC_LISTBOX2                    1005
#define IDC_LISTBOX3                    1006
#define IDC_LISTBOX4                    1007

#define IDC_CHECKBOX1                   1008
#define IDC_CHECKBOX2                   1009
#define IDC_CHECKBOX3                   1010
#define IDC_CHECKBOX4                   1011
#define IDC_CHECKBOX5                   1012
                                            
#define IDC_STATUSBAR                   1020

#define IDC_STEP                        1025
#define IDC_STEPOVER                    1026
#define IDC_MEMTRANSFER                 1027

#define IDC_ADDBP1                      1028
#define IDC_DELBP1                      1029

#define IDC_ADDBP2                      1030
#define IDC_DELBP2                      1031

#define IDC_CHKREAD                     1032
#define IDC_CHKWRITE                    1033
#define IDC_CHKBYTE1                    1034
#define IDC_CHKWORD1                    1035
#define IDC_CHKLONG1                   1036
#define IDC_CHKBYTE2                    1037
#define IDC_CHKWORD2                    1038
#define IDC_CHKLONG2                   1039

#define IDC_BROWSE                      1040
#define IDC_DOWNLOADMEM                 1041
#define IDC_UPLOADMEM                   1042

//////////////////////////////////////////////////////////////////////////////
// Settings stuff

#define IDC_SETTINGSTAB                 1020

//////////////////////////////////////////////////////////////////////////////
// Basic Settings stuff

#define IDC_DISCTYPECB                  1001
#define IDC_DRIVELETTERCB               1002
#define IDC_IMAGEEDIT                   1003
#define IDC_IMAGEBROWSE                 1004

//#define IDC_BIOSLANGCB                  1005
#define IDC_SH2CORECB                   1005

#define IDC_REGIONCB                    1006

#define IDC_BIOSEDIT                    1007
#define IDC_BIOSBROWSE                  1008

#define IDC_BACKUPRAMEDIT               1009
#define IDC_BACKUPRAMBROWSE             1010

#define IDC_MPEGROMEDIT                 1011
#define IDC_MPEGROMBROWSE               1012

#define IDC_CARTTYPECB                  1013
#define IDC_CARTEDIT                    1014
#define IDC_CARTBROWSE                  1015

//////////////////////////////////////////////////////////////////////////////
// Video Settings stuff

#define IDC_VIDEOCORECB                 1001
#define IDC_FULLSCREENSTARTUPCB         1002
#define IDC_AUTOFRAMESKIPCB             1003
#define IDC_FSSIZECB                    1004
#define IDC_CUSTOMWINDOWCB              1005
#define IDC_WIDTHEDIT                   1006
#define IDC_HEIGHTEDIT                  1007

//////////////////////////////////////////////////////////////////////////////
// Sound Settings stuff

#define IDC_SOUNDCORECB                 1001
#define IDC_SLVOLUME                    1002

//////////////////////////////////////////////////////////////////////////////
// Netlink Settings stuff

#define IDC_LOCALREMOTEIP               1001
#define IDC_PORTET                      1002

//////////////////////////////////////////////////////////////////////////////
// Input Settings stuff

#define IDC_PAD1PB                      1001
#define IDC_PAD2PB                      1002

#define IDC_DXDEVICECB                  1010
#define IDC_UPPB                        1011
#define IDC_DOWNPB                      1012
#define IDC_LEFTPB                      1013
#define IDC_RIGHTPB                     1014
#define IDC_LPB                         1015
#define IDC_RPB                         1016
#define IDC_STARTPB                     1017
#define IDC_APB                         1018
#define IDC_BPB                         1019
#define IDC_CPB                         1020
#define IDC_XPB                         1021
#define IDC_YPB                         1022
#define IDC_ZPB                         1023
#define IDC_UPTEXT                      1024
#define IDC_DOWNTEXT                    1025
#define IDC_LEFTTEXT                    1026
#define IDC_RIGHTTEXT                   1027
#define IDC_LTEXT                       1028
#define IDC_RTEXT                       1029
#define IDC_STARTTEXT                   1030
#define IDC_ATEXT                       1031
#define IDC_BTEXT                       1032
#define IDC_CTEXT                       1033
#define IDC_XTEXT                       1034
#define IDC_YTEXT                       1035
#define IDC_ZTEXT                       1036

#define IDC_CUSTOMCANCEL                1037

//////////////////////////////////////////////////////////////////////////////
// Log Settings stuff

#define IDC_USELOGCB                    1001
#define IDC_LOGTYPECB                   1002
#define IDC_LOGFILENAMEET               1003
#define IDC_LOGBROWSEBT                 1004

//////////////////////////////////////////////////////////////////////////////
// Log stuff

#define IDC_LOGET                       1001
#define IDC_SAVELOGBT                   1002
#define IDC_CLEARBT                     1003

//////////////////////////////////////////////////////////////////////////////
// VDP1 debug stuff

#define IDC_VDP1CMDLB                   1001
#define IDC_VDP1CMDET                   1002
#define IDC_VDP1TEXTET                  1003

//////////////////////////////////////////////////////////////////////////////
// VDP2 debug stuff

#define IDC_NBG0ENABCB                  1001
#define IDC_NBG0ET                      1002

#define IDC_NBG1ENABCB                  1003
#define IDC_NBG1ET                      1004

#define IDC_NBG2ENABCB                  1005
#define IDC_NBG2ET                      1006

#define IDC_NBG3ENABCB                  1007
#define IDC_NBG3ET                      1008

#define IDC_RBG0ENABCB                  1009
#define IDC_RBG0ET                      1010

//////////////////////////////////////////////////////////////////////////////
// SCSP debug stuff

#define IDC_SCSPSLOTCB                  1001
#define IDC_SCSPSLOTET                  1002

//////////////////////////////////////////////////////////////////////////////
// Memory Editor stuff

#define IDC_HEXEDIT                     1001
#define IDC_GOTOADDRESS                 1002
#define IDC_SAVESEL                     1003
#define IDC_SEARCHMEM                   1004

//////////////////////////////////////////////////////////////////////////////
// Goto address dialog stuff

#define IDC_SPECIFYADDRRB               1001
#define IDC_PRESETADDRRB                1002
#define IDC_OFFSETET                    1003
#define IDC_PRESETLISTCB                1004

//////////////////////////////////////////////////////////////////////////////
// Error debug stuff

#define IDC_CHEATLIST                   1001
#define IDC_DELETECODE                  1002
#define IDC_CLEARCODES                  1003
#define IDC_ADDAR                       1004
#define IDC_ADDRAWMEMADDR               1005
#define IDC_ADDFROMFILE                 1006

#define IDC_CODE                        1010
#define IDC_CODEDESC                    1011

#define IDC_CODEADDR                    1012
#define IDC_CODEVAL                     1013

#define IDC_CTENABLE                    1015
#define IDC_CTBYTEWRITE                 1016
#define IDC_CTWORDWRITE                 1017
#define IDC_CTLONGWRITE                 1018

//////////////////////////////////////////////////////////////////////////////
// Error debug stuff

#define IDC_EDTEXT                      1001
#define IDC_EDCONTINUE                  1002
#define IDC_EDDEBUG                     1003

//////////////////////////////////////////////////////////////////////////////
// About box stuff

#define IDC_VERSIONTEXT                 1001

//////////////////////////////////////////////////////////////////////////////
