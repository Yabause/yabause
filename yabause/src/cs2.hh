/*  Copyright 2003 Guillaume Duhamel
    Copyright 2004 Theo Berkau

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

#ifndef CS2_HH
#define CS2_HH

#include "memory.hh"

class CDInterface;

#define MAX_BLOCKS      200
#define MAX_SELECTORS   24
#define MAX_FILES       256

typedef struct
{
   long size;
   unsigned long FAD;
   unsigned char cn;
   unsigned char fn;
   unsigned char sm;
   unsigned char ci;
   unsigned char data[2352];
} block_struct;

typedef struct
{
   unsigned long FAD;
   unsigned long range;
   unsigned char mode;
   unsigned char chan;
   unsigned char smmask;
   unsigned char cimask;
   unsigned char fid;
   unsigned char smval;
   unsigned char cival;
   unsigned char condtrue;
   unsigned char condfalse;
} filter_struct;

typedef struct
{
   long size;
   block_struct *block[MAX_BLOCKS];
   unsigned char numblocks;
} partition_struct;

typedef struct
{
  unsigned short groupid;
  unsigned short userid;
  unsigned short attributes;
  unsigned short signature;
  unsigned char filenumber;
  unsigned char reserved[5];
} xarec_struct;

typedef struct
{
  unsigned char recordsize;
  unsigned char xarecordsize;
  unsigned long lba;
  unsigned long size;
  unsigned char dateyear;
  unsigned char datemonth;
  unsigned char dateday;
  unsigned char datehour;
  unsigned char dateminute;
  unsigned char datesecond;
  unsigned char gmtoffset;
  unsigned char flags;
  unsigned char fileunitsize;
  unsigned char interleavegapsize;
  unsigned short volumesequencenumber;
  unsigned char namelength;
  char name[32];
  xarec_struct xarecord;
} dirrec_struct;

typedef struct
{
   unsigned char audcon;
   unsigned char audlay;
   unsigned char audbufdivnum;
   unsigned char vidcon;
   unsigned char vidlay;
   unsigned char vidbufdivnum;
} mpegcon_struct;

typedef struct
{
   unsigned char audstm;
   unsigned char audstmid;
   unsigned char audchannum;
   unsigned char vidstm;
   unsigned char vidstmid;
   unsigned char vidchannum;
} mpegstm_struct;

class Cs2 : public Memory {
private:
  unsigned long FAD;
  unsigned char status;

  // cd specific stats
  unsigned char options;
  unsigned char repcnt;
  unsigned char ctrladdr;
  unsigned char track;
  unsigned char index;

  // mpeg specific stats
  unsigned char actionstatus;
  unsigned char pictureinfo;
  unsigned char mpegaudiostatus;
  unsigned short mpegvideostatus;
  unsigned short vcounter;

  // authentication variables
  unsigned short satauth;
  unsigned short mpgauth;

  unsigned long transfercount;
  unsigned long cdwnum;
  unsigned long TOC[102];
  unsigned long playFAD;
  unsigned long playendFAD;
  unsigned long getsectsize;
  unsigned long putsectsize;
  unsigned long calcsize;
  long infotranstype;
  long datatranstype;
  bool isonesectorstored;
  bool isdiskchanged;
  bool isbufferfull;
  bool speed1x;
  unsigned char transfileinfo[12];
  unsigned char lastbuffer;

  filter_struct filter[MAX_SELECTORS];
  filter_struct *outconcddev;
  filter_struct *outconmpegfb;
  filter_struct *outconmpegbuf;
  filter_struct *outconmpegrom;
  filter_struct *outconhost;

  partition_struct partition[MAX_SELECTORS];

  partition_struct *datatranspartition;
  long datatransoffset;
  unsigned long datanumsecttrans;
  unsigned short datatranssectpos;
  unsigned short datasectstotrans;

  unsigned long blockfreespace;
  block_struct block[MAX_BLOCKS];
  block_struct workblock;

  unsigned long curdirsect;
  dirrec_struct fileinfo[MAX_FILES];
  unsigned long numfiles;

  unsigned long mpegintmask;

  mpegcon_struct mpegcon[2];
  mpegstm_struct mpegstm[2];

  bool _command;
  unsigned long _periodiccycles;
  unsigned long _periodictiming;
  CDInterface *cd;
public:
  Cs2(void);
  ~Cs2(void);
  unsigned short getHIRQ(void);
  unsigned short getHIRQMask(void);
  unsigned short getCR1(void);
  unsigned short getCR2(void);
  unsigned short getCR3(void);
  unsigned short getCR4(void);
  void setHIRQ(unsigned short val);
  void setHIRQMask(unsigned short val);
  void setCR1(unsigned short val);
  void setCR2(unsigned short val);
  void setCR3(unsigned short val);
  void setCR4(unsigned short val);

  unsigned char getByte(unsigned long);
  void setByte(unsigned long, unsigned char);
  unsigned short getWord(unsigned long);
  void setWord(unsigned long, unsigned short);
  unsigned long getLong(unsigned long);
  void setLong(unsigned long, unsigned long);

  void run(unsigned long);
  void execute(void);
  void reset(void);
  void SetTiming(bool);
  void command(void);
  void periodicUpdate(void);
                                  
  //   command name                          command code
  void getStatus(void);                   // 0x00
  void getHardwareInfo(void);             // 0x01
  void getToc(void);                      // 0x02
  void getSessionInfo();                  // 0x03
  void initializeCDSystem(void);          // 0x04
  // Open Tray                            // 0x05
  void endDataTransfer(void);             // 0x06
  void playDisc(void);                    // 0x10
  void seekDisc(void);                    // 0x11
  // Scan Disc                            // 0x12
  void getSubcodeQRW(void);               // 0x20
  void setCDDeviceConnection(void);       // 0x30
  // get CD Device Connection             // 0x31
  void getLastBufferDestination(void);    // 0x32
  void setFilterRange(void);              // 0x40
  // get Filter Range                     // 0x41
  void setFilterSubheaderConditions(void);// 0x42
  void getFilterSubheaderConditions(void);// 0x43
  void setFilterMode(void);               // 0x44
  void getFilterMode(void);               // 0x45
  void setFilterConnection(void);         // 0x46
  // Get Filter Connection                // 0x47
  void resetSelector(void);               // 0x48
  void getBufferSize(void);               // 0x50
  void getSectorNumber(void);             // 0x51
  void calculateActualSize(void);         // 0x52
  void getActualSize(void);               // 0x53
  void getSectorInfo(void);               // 0x54
  void setSectorLength(void);             // 0x60
  void getSectorData(void);               // 0x61
  void deleteSectorData(void);            // 0x62
  void getThenDeleteSectorData(void);     // 0x63
  void putSectorData(void);               // 0x64
  // Copy Sector Data                     // 0x65
  // Move Sector Data                     // 0x66
  void getCopyError(void);                // 0x67
  void changeDirectory(void);             // 0x70
  void readDirectory(void);               // 0x71
  void getFileSystemScope(void);          // 0x72
  void getFileInfo(void);                 // 0x73
  void readFile(void);                    // 0x74
  void abortFile(void);                   // 0x75
  void mpegGetStatus(void);               // 0x90
  void mpegGetInterrupt(void);            // 0x91
  void mpegSetInterruptMask(void);        // 0x92
  void mpegInit(void);                    // 0x93
  void mpegSetMode(void);                 // 0x94
  void mpegPlay(void);                    // 0x95
  void mpegSetDecodingMethod(void);       // 0x96
  // MPEG Out Decoding Sync               // 0x97
  // MPEG Get Timecode                    // 0x98
  // MPEG Get Pts                         // 0x99
  void mpegSetConnection(void);           // 0x9A
  void mpegGetConnection(void);           // 0x9B
  // MPEG Change Connection               // 0x9C
  void mpegSetStream(void);               // 0x9D
  void mpegGetStream(void);               // 0x9E
  // MPEG Get Picture Size                // 0x9F
  void mpegDisplay(void);                 // 0xA0
  void mpegSetWindow(void);               // 0xA1
  void mpegSetBorderColor(void);          // 0xA2
  void mpegSetFade(void);                 // 0xA3
  void mpegSetVideoEffects(void);         // 0xA4
  // MPEG Get Image                       // 0xA5
  // MPEG Set Image                       // 0xA6
  // MPEG Read Image                      // 0xA7
  // MPEG Write Image                     // 0xA8
  // MPEG Read Sector                     // 0xA9
  // MPEG Write Sector                    // 0xAA
  // MPEG Get LSI                         // 0xAE
  void mpegSetLSI(void);                  // 0xAF
  void cmdE0(void);                       // 0xE0
  void cmdE1(void);                       // 0xE1
  void cmdE2(void);                       // 0xE2

  unsigned char FADToTrack(unsigned long fad);
  unsigned long TrackToFad(unsigned short trackandindex);
  void SetupDefaultPlayStats(unsigned char track_number);
  block_struct *AllocateBlock();
  void FreeBlock(block_struct *blk);
  void SortBlocks(partition_struct *part);
  partition_struct *GetPartition(filter_struct *curfilter);
  partition_struct *FilterData(filter_struct *curfilter, bool isaudio);
  int CopyDirRecord(unsigned char *buffer, dirrec_struct *dirrec);
  int ReadFileSystem(filter_struct *curfilter, unsigned long fid, bool isoffset);
  void SetupFileInfoTransfer(unsigned long fid);
  partition_struct *ReadUnFilteredSector(unsigned long rufsFAD);
  partition_struct *ReadFilteredSector(unsigned long rfsFAD);
  unsigned char GetRegionID();
};

#endif
