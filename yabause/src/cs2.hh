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
#include "cpu.hh"

#define MAX_BLOCKS      200
#define MAX_SELECTORS   24
#define MAX_FILES       1024

typedef struct
{
   long size;
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

class Cs2 : public Cpu, public Memory {
private:
  SDL_Thread *cdThread;
  SDL_mutex *cdMutex;
  unsigned long FAD;
  unsigned char status;
  unsigned char options;
  unsigned char repcnt;
  unsigned char ctrladdr;
  unsigned char track;
  unsigned char index;

  unsigned long transfercount;
  unsigned long cdwnum;
  unsigned long TOC[102];
  unsigned long playFAD;
  unsigned long playendFAD;
  unsigned long getsectsize;
  long infotranstype;
  long datatranstype;
  bool isonesectorstored;
  bool isdiskchanged;
  bool issubcodeqdecoded;
  bool isbufferfull;
  unsigned char transfileinfo[12];

  filter_struct filter[MAX_SELECTORS];
  filter_struct *curfilter;

  partition_struct partition[MAX_SELECTORS];
  partition_struct *curpartition;

  unsigned long databytestotrans;

  unsigned long blockfreespace;
  block_struct block[MAX_BLOCKS];

  dirrec_struct fileinfo[MAX_FILES];
  unsigned long numfiles;

  bool _command;
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

  unsigned short getWord(unsigned long);
  void setWord(unsigned long, unsigned short);
  unsigned long getLong(unsigned long);

  static void run(Cs2 *);
  void execute(void);
  void command(void);
  void periodicUpdate(void);
                                  
  //   command name                     command code
  void getStatus(void);              // 0x00
  void getHardwareInfo(void);        // 0x01
  void getToc(void);                 // 0x02
  void getSessionInfo();             // 0x03
  void initializeCDSystem(void);     // 0x04
  void endDataTransfer(void);        // 0x06
  void playDisc(void);               // 0x10
  void setCDDeviceConnection(void);  // 0x30
  void resetSelector(void);          // 0x48
  void getSectorNumber(void);        // 0x51
  void setSectorLength(void);        // 0x60
  void getThenDeleteSectorData(void);// 0x63
  void getCopyError(void);           // 0x67
  void changeDirectory(void);        // 0x70
  void getFileSystemScope(void);     // 0x72
  void getFileInfo(void);            // 0x73
  void readFile(void);               // 0x74
  void abortFile(void);              // 0x75
  void mpegInit(void);               // 0x93
  void cmdE0(void);                  // 0xE0
  void cmdE1(void);                  // 0xE1

  unsigned char FADToTrack(unsigned long fad);
  void SetupDefaultPlayStats(unsigned char track_number);
  block_struct *AllocateBlock();
  void FreeBlock(block_struct *blk);
  void SortBlocks(partition_struct *part);
  partition_struct *GetPartition();
  int CopyDirRecord(unsigned char *buffer, dirrec_struct *dirrec);
  void ReadFileSystem();
  void SetupFileInfoTransfer(unsigned long fid);
};

#endif
