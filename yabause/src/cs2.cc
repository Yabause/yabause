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

#include "cs2.hh"
#include "timer.hh"
#include "cd.hh"

#define CDB_HIRQ_CMOK      0x0001
#define CDB_HIRQ_DRDY      0x0002
#define CDB_HIRQ_CSCT      0x0004
#define CDB_HIRQ_BFUL      0x0008
#define CDB_HIRQ_PEND      0x0010
#define CDB_HIRQ_DCHG      0x0020
#define CDB_HIRQ_ESEL      0x0040
#define CDB_HIRQ_EHST      0x0080
#define CDB_HIRQ_ECPY      0x0100
#define CDB_HIRQ_EFLS      0x0200
#define CDB_HIRQ_SCDQ      0x0400
#define CDB_HIRQ_MPED      0x0800
#define CDB_HIRQ_MPCM      0x1000
#define CDB_HIRQ_MPST      0x2000

#define CDB_STAT_BUSY      0x00
#define CDB_STAT_PAUSE     0x01
#define CDB_STAT_STANDBY   0x02
#define CDB_STAT_PLAY      0x03
#define CDB_STAT_SEEK      0x04
#define CDB_STAT_SCAN      0x05
#define CDB_STAT_OPEN      0x06
#define CDB_STAT_NODISC    0x07
#define CDB_STAT_RETRY     0x08
#define CDB_STAT_ERROR     0x09
#define CDB_STAT_FATAL     0x0A
#define CDB_STAT_PERI      0x20
#define CDB_STAT_TRNS      0x40
#define CDB_STAT_WAIT      0x80
#define CDB_STAT_REJECT    0xFF

unsigned short Cs2::getHIRQ(void) {return Memory::getWord(0x90008);}
unsigned short Cs2::getHIRQMask(void) {return Memory::getWord(0x9000C);}
unsigned short Cs2::getCR1(void) {return Memory::getWord(0x90018);}
unsigned short Cs2::getCR2(void) {return Memory::getWord(0x9001C);}
unsigned short Cs2::getCR3(void) {return Memory::getWord(0x90020);}
unsigned short Cs2::getCR4(void) {return Memory::getWord(0x90024);}
void Cs2::setHIRQ(unsigned short val) {Memory::setWord(0x90008, val);}
void Cs2::setHIRQMask(unsigned short val) {Memory::setWord(0x9000C, val);}
void Cs2::setCR1(unsigned short val) {Memory::setWord(0x90018, val);}
void Cs2::setCR2(unsigned short val) {Memory::setWord(0x9001C, val);}
void Cs2::setCR3(unsigned short val) {Memory::setWord(0x90020, val);}
void Cs2::setCR4(unsigned short val) {Memory::setWord(0x90024, val);}

unsigned short Cs2::getWord(unsigned long addr) {
  unsigned short val=0;
  if (addr >= size) { 
#ifndef _arch_dreamcast
	throw BadMemoryAccess(addr);
#else
	printf("Bad Memory Access: %x\n", addr);
#endif
  }
  switch(addr) {
    case 0x90008:
                  val = Memory::getWord(addr);

                  if (isbufferfull)
                    val |= CDB_HIRQ_BFUL;
                  else
                    val &= ~CDB_HIRQ_BFUL;

                  if (isdiskchanged)
                    val |= CDB_HIRQ_DCHG;
                  else
                    val &= ~CDB_HIRQ_DCHG;

                  if (issubcodeqdecoded)
                    val |= CDB_HIRQ_SCDQ;
                  else
                    val &= ~CDB_HIRQ_SCDQ;

                  Memory::setWord(addr, val);
#if CDDEBUG
                  fprintf(stderr, "cs2\t: Hirq read - ret: %x\n", val);
#endif
	          break;
    case 0x9000C:
    case 0x90018:
    case 0x9001C:
    case 0x90020: 
    case 0x90024: val = Memory::getWord(addr);
		  break;
    case 0x98000:
                  // transfer info
                  switch (infotranstype) {
                     case 0:
                             // Get Toc Data
                             if (transfercount % 4 == 0)
                                val = (TOC[transfercount >> 2] & 0xFFFF0000) >> 16;
                             else
                                val = TOC[transfercount >> 2] & 0x0000FFFF;

                             transfercount += 2;
                             cdwnum += 2;

                             if (transfercount > (0xCC * 2))
                             {
                                transfercount = 0;
                                infotranstype = -1;
                             }
                             break;
                     case 1:
                             // Get File Info
                             val = (transfileinfo[transfercount] << 8) |
                                    transfileinfo[transfercount + 1];
                             transfercount += 2;
                             cdwnum += 2;

                             if (transfercount > (0x6 * 2))
                             {
                                transfercount = 0;
                                infotranstype = -1;
                             }
#if CDDEBUG
                             fprintf(stderr, "cs2\t: getfileinfo data: %x\n", val);
#endif

                             break;
                     default: break;
                  }
                  break;
    default: val = Memory::getWord(addr);
             break;
  }

  return val;
}

void Cs2::setWord(unsigned long addr, unsigned short val) {
  if (addr >= size) { 
#ifndef _arch_dreamcast
	throw BadMemoryAccess(addr);
#else
	printf("Bad Memory Access: %x\n", addr);
#endif
  }
  switch(addr) {
    case 0x90008:
#if CDDEBUG
                  fprintf(stderr, "cs2\t: WriteWord hirq = %x\n", val);
#endif
                  Memory::setWord(addr, Memory::getWord(addr) & val);
	          break;
    case 0x9000C:
    case 0x90018:
    case 0x9001C:
    case 0x90020: Memory::setWord(addr, val);
		  break;
    case 0x90024: Memory::setWord(addr, val);
                  _command = true;
                  execute();
                  _command = false;
		  break;
    default: Memory::setWord(addr, val);
  }
}

unsigned long Cs2::getLong(unsigned long addr) {
  unsigned long val=0;
  if (addr >= size) { 
#ifndef _arch_dreamcast
	throw BadMemoryAccess(addr);
#else
	printf("Bad Memory Access: %x\n", addr);
#endif
  }
  switch(addr) {
    case 0x18000:
                  // transfer data
                  switch (datatranstype) {
                    case 0:
                            // get then delete sector
                            if (databytestotrans > 0)
                            {
                               // fix me
                               val = (curpartition->block[cdwnum / getsectsize]->data[(cdwnum % getsectsize)] << 24) +
                                     (curpartition->block[cdwnum / getsectsize]->data[(cdwnum % getsectsize) + 1] << 16) +
                                     (curpartition->block[cdwnum / getsectsize]->data[(cdwnum % getsectsize) + 2] << 8) +
                                      curpartition->block[cdwnum / getsectsize]->data[(cdwnum % getsectsize) + 3];
                               // get then delete sector
                               cdwnum += 4;
                               databytestotrans -= 4;
                            }

                            if (databytestotrans == 0)
                            {
                               datatranstype = -1;

                               // free blocks
                               for (unsigned long i = 0; i < (cdwnum / getsectsize); i++)
                               {
                                  FreeBlock(curpartition->block[i]);
                                  curpartition->block[i] = NULL;
                               }

                               // sort remaining blocks here
                               SortBlocks(curpartition);

                               curpartition->size -= cdwnum;
                               curpartition->numblocks -= (cdwnum / getsectsize); // fix me

#if CDDEBUG
                               fprintf(stderr, "cs2\t: curpartition->size = %x\n", val);
#endif
                            }
                            break;
                    default: break;
                  }

	          break;
    default: val = Memory::getLong(addr);
  }

  return val;
}


Cs2::Cs2(void) : Memory(0xFFFFF, 0x100000) {
  unsigned long i, i2;

  _stop = false;

  if (CDIsCDPresent())
  {
     status = CDB_STAT_PERI | CDB_STAT_PAUSE; // Still not 100% correct, but better

     FAD = 150;
     options = 0;
     repcnt = 0;
     ctrladdr = 0x41;
     track = 1;
     index = 1;
  }
  else
  {
     status = CDB_STAT_PERI | CDB_STAT_NODISC;
//     status = CDB_STAT_PERI | CDB_STAT_OPEN;

     FAD = 0xFFFFFFFF;
     options = 0xFF;
     repcnt = 0xFF;
     ctrladdr = 0xFF;
     track = 0xFF;
     index = 0xFF;
  }

  infotranstype = -1;
  datatranstype = -1;
  transfercount = 0;
  cdwnum = 0;
  getsectsize = 2048;
  isonesectorstored = false;
  isdiskchanged = true;
  issubcodeqdecoded = true;
  isbufferfull = false;

  setCR1(( 0 <<8) | 'C');
  setCR2(('D'<<8) | 'B');
  setCR3(('L'<<8) | 'O');
  setCR4(('C'<<8) | 'K');
  setHIRQ(0xFFFF);
  setHIRQMask(0xFFFF);

  playFAD = 0xFFFFFFFF;
  playendFAD = 0xFFFFFFFF;

  // set cd authentication variable to 0(not authenticated)
  authval = 0;

  // clear filter conditions
  for (i = 0; i < MAX_SELECTORS; i++)
  {
     filter[i].FAD = 0;
     filter[i].range = 0xFFFFFFFF;
     filter[i].mode = 0;
     filter[i].chan = 0;
     filter[i].smmask = 0;
     filter[i].cimask = 0;
     filter[i].fid = 0;
     filter[i].smval = 0;
     filter[i].cival = 0;
     filter[i].condtrue = 0;
     filter[i].condfalse = 0xFF;
  }

  // clear partitions
  for (i = 0; i < MAX_SELECTORS; i++)
  {
     partition[i].size = -1;
     partition[i].numblocks = 0;

     for (i2 = 0; i2 < MAX_BLOCKS; i2++)
     {
        partition[i].block[i2] = NULL;
     }
  }

  // clear blocks
  for (i = 0; i < MAX_BLOCKS; i++)
  {
     block[i].size = -1;
     memset(block[i].data, 0, 2352);
  }

  blockfreespace = 200;

  // initialize TOC
  memset(TOC, 0xFF, sizeof(TOC));

  // clear filesystem stuff
  memset(&fileinfo, 0, sizeof(fileinfo));
  numfiles = 0;

  _command = false;
  SDL_CreateThread((int (*)(void*)) &run, this);
}

Cs2::~Cs2(void) {
   _stop = true;
   SDL_WaitThread(cdThread, NULL);
}

void Cs2::run(Cs2 *cd) {
  Timer t;
  while(!cd->_stop) {
    if (cd->_command) {
      t.waitVBlankOUT();
      t.wait(1);
    }
    else {
      unsigned short val=0;

      switch (cd->status & 0xF) {
         case CDB_STAT_PAUSE:
            break;
         case CDB_STAT_PLAY:
         {
            if (cd->ctrladdr & 0x40)
            {
               // read data

               if ((cd->curpartition = cd->GetPartition()) != NULL && !cd->isbufferfull)
               {
                  cd->curpartition->block[cd->curpartition->numblocks] = cd->AllocateBlock();

                  if (cd->curpartition->block[cd->curpartition->numblocks] != NULL) {
                     CDReadSector(cd->FAD - 150, cd->getsectsize, cd->curpartition->block[cd->curpartition->numblocks]->data);
                     cd->curpartition->numblocks++;
                     cd->curpartition->size += cd->getsectsize;
                     cd->FAD++;

#if CDDEBUG
                     fprintf(stderr, "blocks = %d blockfreespace = %d fad = %x curpartition->size = %x isbufferfull = %x\n", cd->curpartition->numblocks, cd->blockfreespace, cd->FAD, cd->curpartition->size, cd->isbufferfull);
#endif
                     cd->isonesectorstored = true;
                     cd->setHIRQ(cd->getHIRQ() | CDB_HIRQ_CSCT);

                     if (cd->FAD >= cd->playendFAD) {
                        // we're done
                        cd->status = CDB_STAT_PERI | CDB_STAT_PAUSE;
//                        cd->setHIRQ(cd->getHIRQ() | HIRQ_DRDY | CDB_HIRQ_PEND);
                        cd->setHIRQ(cd->getHIRQ() | CDB_HIRQ_PEND);
                     }
                  }

//                  if (cd->isbufferfull)
//                     cd->status = CDB_STAT_PERI | CDB_STAT_PAUSE;
               }
            }

            break;
         }
         case CDB_STAT_SEEK:
            break;
         case CDB_STAT_SCAN:
            break;
         case CDB_STAT_RETRY:
            break;
         default: break;
      }

      cd->status |= CDB_STAT_PERI;

      // adjust command registers appropriately here(fix me)

      // somehow I doubt this is correct
      t.waitVBlankOUT();
      t.wait(1);
      //cd->periodicUpdate();
    }
  }
}

void Cs2::command(void) {
  _command = true;
}

void Cs2::periodicUpdate(void) {
  if ((getCR1() >> 8) != 0 ) return;
  status |= CDB_STAT_PERI;
}

void Cs2::execute(void) {
  setHIRQ(getHIRQ() & 0XFFFE);
  unsigned short instruction = getCR1() >> 8;

  switch (instruction) {
    case 0x00:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getStatus\n");
#endif
      getStatus();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", getHIRQ(), getCR1(), getCR2(), getCR3(), getCR4());
#endif
      break;
    case 0x01:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getHardwareInfo\n");
#endif
      getHardwareInfo();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", getHIRQ(), getCR1(), getCR2(), getCR3(), getCR4());
#endif
      break;
    case 0x02:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getToc\n");
#endif
      getToc();
      break;
    case 0x03:
    {
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getSessionInfo\n");
#endif
       getSessionInfo();
       break;
    }
    case 0x04:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: initializeCDSystem\n");
#endif
      initializeCDSystem();
      break;
    case 0x06:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: endDataTransfer %04x %04x %04x %04x %04x\n", getHIRQ(), getCR1(), getCR2(), getCR3(), getCR4());
#endif
      endDataTransfer();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", getHIRQ(), getCR1(), getCR2(), getCR3(), getCR4());
#endif
      break;
    case 0x10:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: playDisc %04x %04x %04x %04x %04x\n", getHIRQ(), getCR1(), getCR2(), getCR3(), getCR4());
#endif
      playDisc();
      break;
    case 0x11:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: seekDisc %04x %04x %04x %04x %04x\n", getHIRQ(), getCR1(), getCR2(), getCR3(), getCR4());
#endif
      seekDisc();
      break;
    case 0x20:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getSubcodeQRW %04x %04x %04x %04x %04x\n", getHIRQ(), getCR1(), getCR2(), getCR3(), getCR4());
#endif
      getSubcodeQRW();
      break;
    case 0x30:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: setCDDeviceConnection\n");
#endif
      setCDDeviceConnection();
      break;
    case 0x42:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: setFilterSubheaderConditions\n");
#endif
      setFilterSubheaderConditions();
      break;
    case 0x44:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: setFilterMode\n");
#endif
      setFilterMode();
      break;
    case 0x46:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: setFilterConnection\n");
#endif
      setFilterConnection();
      break;
    case 0x48:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: resetSelector\n");
#endif
      resetSelector();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", getHIRQ(), getCR1(), getCR2(), getCR3(), getCR4());
#endif
      break;
    case 0x51:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getSectorNumber\n");
#endif
      getSectorNumber();

#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", getHIRQ(), getCR1(), getCR2(), getCR3(), getCR4());
#endif
      break;
    case 0x60:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: setSectorLength\n");
#endif
      setSectorLength();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", getHIRQ(), getCR1(), getCR2(), getCR3(), getCR4());
#endif
      break;
    case 0x63:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getThenDeleteSectorData %04x %04x %04x %04x %04x\n", getHIRQ(), getCR1(), getCR2(), getCR3(), getCR4());
#endif
      getThenDeleteSectorData();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", getHIRQ(), getCR1(), getCR2(), getCR3(), getCR4());
#endif
      break;
    case 0x67:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getCopyError\n");
#endif
      getCopyError();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", getHIRQ(), getCR1(), getCR2(), getCR3(), getCR4());
#endif
      break;
    case 0x70:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: changeDirectory\n");
#endif
      changeDirectory();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", getHIRQ(), getCR1(), getCR2(), getCR3(), getCR4());
#endif
      break;
    case 0x72:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getFileSystemScope\n");
#endif
      getFileSystemScope();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", getHIRQ(), getCR1(), getCR2(), getCR3(), getCR4());
#endif
      break;
    case 0x73:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getFileInfo\n");
#endif
      getFileInfo();
      break;
    case 0x74:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: readFile\n");
#endif
      readFile();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", getHIRQ(), getCR1(), getCR2(), getCR3(), getCR4());
#endif
      break;
    case 0x75:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: abortFile\n");
#endif
      abortFile();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", getHIRQ(), getCR1(), getCR2(), getCR3(), getCR4());
#endif
      break;
    case 0x93: {
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegInit\n");
#endif
      mpegInit();
      break;
    }
    case 0xE0: {
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: cmdE0 %04x %04x %04x %04x %04x\n", getHIRQ(), getCR1(), getCR2(), getCR3(), getCR4());
#endif
      cmdE0();
      break;
    }
    case 0xE1: {
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: cmdE1 %04x %04x %04x %04x %04x\n", getHIRQ(), getCR1(), getCR2(), getCR3(), getCR4());
#endif 
      cmdE1();
      break;
    }
    case 0xE2: {
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: cmdE2 %04x %04x %04x %04x %04x\n", getHIRQ(), getCR1(), getCR2(), getCR3(), getCR4());
#endif 
      cmdE2();
      break;
    }
    default:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command %02x not implemented\n", instruction);
#endif
      break;
  }
}

void Cs2::getStatus(void) {
  setCR1((status << 8) | ((options & 0xF) << 4) | (repcnt & 0xF));
  setCR2((ctrladdr << 8) | track);
  setCR3((index << 8) | ((FAD >> 16) & 0xFF));
  setCR4((unsigned short) FAD);
  setHIRQ(getHIRQ() | CDB_HIRQ_CMOK);
}

void Cs2::getHardwareInfo(void) {
  if ((status & 0xF) != CDB_STAT_OPEN && (status & 0xF) != CDB_STAT_NODISC)
     isdiskchanged = false;

  setCR1(status << 8);
  // hardware flags/CD Version
  setCR2(0x0201); // mpeg card exists
  // mpeg version, it actually is required(at least by the bios)
  setCR3(0x1);
  // drive info/revision
  setCR4(0x0400); 
  setHIRQ(getHIRQ() | CDB_HIRQ_CMOK);
}

void Cs2::getToc(void) {
  CDReadToc(TOC);

  transfercount = 0;
  infotranstype = 0;

  setCR1(status << 8);
  setCR2(0xCC);
  setCR3(0x0);
  setCR4(0x0); 
  setHIRQ(getHIRQ() | CDB_HIRQ_CMOK | CDB_HIRQ_DRDY);
  status = CDB_STAT_PERI | CDB_STAT_PAUSE;
}

void Cs2::getSessionInfo(void) {

  switch (getCR1() & 0xFF) {
    case 0:
            setCR3(0x0100 | ((TOC[101] & 0xFF0000) >> 16));
            setCR4(TOC[101] & 0x00FFFF);
            break;
    case 1:
            setCR3(0x0100);
            setCR4(0);
            break;
    default:
            setCR3(0xFFFF);
            setCR4(0xFFFF);
            break;
  }

  status = CDB_STAT_PERI | CDB_STAT_PAUSE;
  setCR1(status << 8);
  setCR2(0);

  setHIRQ(getHIRQ() | CDB_HIRQ_CMOK);
}

void Cs2::initializeCDSystem(void) {
  unsigned short val=0;

  if (status & 0xF != CDB_STAT_OPEN && status & 0xF != CDB_STAT_NODISC)
  {
     status = CDB_STAT_PERI | CDB_STAT_PAUSE;
     isonesectorstored = false;
     issubcodeqdecoded = true;
     FAD = 150;
  }

  val = getHIRQ() & 0xFFE5;
  isbufferfull = false;

  if (isonesectorstored)
     val |= CDB_HIRQ_CSCT;
  else
     val &= ~CDB_HIRQ_CSCT;

  if (isdiskchanged)
     val |= CDB_HIRQ_DCHG;
  else
     val &= ~CDB_HIRQ_DCHG;

  if (issubcodeqdecoded)
     val |= CDB_HIRQ_SCDQ;
  else
     val &= ~CDB_HIRQ_SCDQ;

  setCR1((status << 8) | (repcnt & 0xF));
  setCR2((ctrladdr << 8) | (track & 0xFF));
  setCR3((index << 8) | ((FAD >> 16) &0xFF));
  setCR4((unsigned short) FAD);
  setHIRQ(val | CDB_HIRQ_CMOK | CDB_HIRQ_ESEL);
}

void Cs2::endDataTransfer(void) {
  if (cdwnum)
  {
     setCR1((status << 8) | ((cdwnum >> 17) & 0xFF));
     setCR2((unsigned short)(cdwnum >> 1));
     setCR3(0);
     setCR4(0);
  }
  else
  {
     setCR1((status << 8) | 0xFF); // FIXME
     setCR2(0xFFFF);
     setCR3(0);
     setCR4(0);
  }

  isonesectorstored = false;
  cdwnum = 0;

  setHIRQ(getHIRQ() | CDB_HIRQ_CMOK);
}

void Cs2::playDisc(void) {
  // This obviously is going to need alot of work

  if ((getCR1() & 0xFF) != 0 && getCR2() != 0xFFFF)
  {
     if ((getCR1() & 0x80) && (getCR3() >> 8) != 0xFF)
     {
        // fad mode
        unsigned long size;

        FAD = ((getCR1() & 0xF) << 16) + getCR2();
        size = getCR4();
        
        SetupDefaultPlayStats(FADToTrack(FAD));

        playFAD = FAD;
        playendFAD = FAD + size;

        if ((curpartition = GetPartition()) != NULL)
        {
          curpartition->size = 0;
        }

        isonesectorstored = true;
        setHIRQ(getHIRQ() | CDB_HIRQ_CSCT);

        status = CDB_STAT_PERI | CDB_STAT_PLAY;

        setHIRQ(getHIRQ() | CDB_HIRQ_CMOK);
        setCR1((status << 8) | ((options & 0xF) << 4) | (repcnt & 0xF));
        setCR2((ctrladdr << 8) | (track & 0xFF));
        setCR3((index << 8) | ((FAD >> 16) &0xFF));
        setCR4((unsigned short) FAD);
     }
  }
}

void Cs2::seekDisc(void) {
  if (getCR1() & 0x80)
  {
     // Seek by FAD
  }
  else
  {
     // Seek by index
     SetupDefaultPlayStats((getCR2() >> 8) + 1);
     index = getCR2() & 0xFF;
  }

  setCR1((status << 8) | ((options & 0xF) << 4) | (repcnt & 0xF));
  setCR2((ctrladdr << 8) | (track & 0xFF));
  setCR3((index << 8) | ((FAD >> 16) &0xFF));
  setCR4((unsigned short) FAD);

  setHIRQ(getHIRQ() | CDB_HIRQ_CMOK);
}

void Cs2::getSubcodeQRW(void) {
  // According to Tyranid's doc, the subcode type is stored in the low byte
  // of CR2. However, Sega's CDC library writes the type to the low byte
  // of CR1. Somehow I'd sooner believe Sega is right.
  switch(getCR1() & 0xFF) {
     case 0:
             // Get Q Channel
             setCR1((status << 8) | 0);
             setCR2(5);
             setCR3(0);
             setCR4(0);

             // setup transfer here(fix me)
             break;
     case 1:
             // Get RW Channel
             setCR1((status << 8) | 0);
             setCR2(12);
             setCR3(0);
             setCR4(0);

             // setup transfer here(fix me)
             break;
     default: break;
  }

  setHIRQ(getHIRQ() | CDB_HIRQ_CMOK | CDB_HIRQ_DRDY);
}

void Cs2::setCDDeviceConnection(void) {
  if ((getCR3() >> 8) != 0xFF && (getCR3() >> 8) < 0x24)
  {
     curfilter = filter + (getCR3() >> 8);
  }

  setCR1((status << 8) | ((options & 0xF) << 4) | (repcnt & 0xF));
  setCR2((ctrladdr << 8) | (track & 0xFF));
  setCR3((index << 8) | ((FAD >> 16) &0xFF));
  setCR4((unsigned short) FAD);
  setHIRQ(getHIRQ() | CDB_HIRQ_CMOK | CDB_HIRQ_ESEL);
}

void Cs2::setFilterSubheaderConditions(void) {
  unsigned char sfscfilternum;

  sfscfilternum = getCR3() >> 8;

  filter[sfscfilternum].chan = getCR1() & 0xFF;
  filter[sfscfilternum].smmask = getCR2() >> 8;
  filter[sfscfilternum].cimask = getCR2() & 0xFF;
  filter[sfscfilternum].fid = getCR3() & 0xFF;;
  filter[sfscfilternum].smval = getCR4() >> 8;
  filter[sfscfilternum].cival = getCR4() & 0xFF;

  setCR1((status << 8) | ((options & 0xF) << 4) | (repcnt & 0xF));
  setCR2((ctrladdr << 8) | (track & 0xFF));
  setCR3((index << 8) | ((FAD >> 16) &0xFF));
  setCR4((unsigned short) FAD);
  setHIRQ(getHIRQ() | CDB_HIRQ_CMOK | CDB_HIRQ_ESEL);
}

void Cs2::setFilterMode(void) {
  unsigned char sfmfilternum;

  sfmfilternum = getCR3() >> 8;

  filter[sfmfilternum].mode = getCR1() & 0x7F;
  filter[sfmfilternum].FAD = 0;
  filter[sfmfilternum].range = 0xFFFFFFFF;

  setCR1((status << 8) | ((options & 0xF) << 4) | (repcnt & 0xF));
  setCR2((ctrladdr << 8) | (track & 0xFF));
  setCR3((index << 8) | ((FAD >> 16) &0xFF));
  setCR4((unsigned short) FAD);
  setHIRQ(getHIRQ() | CDB_HIRQ_CMOK | CDB_HIRQ_ESEL);
}

void Cs2::setFilterConnection(void) {
  unsigned char sfcfilternum;

  sfcfilternum = getCR3() >> 8;

  if (getCR1() & 0x1)
  {
     // Set connection for true condition
     filter[sfcfilternum].condtrue = getCR2() >> 8;
  }

  if (getCR1() & 0x2)
  {
     // Set connection for false condition
     filter[sfcfilternum].condfalse = getCR2() & 0xFF;
  }

  setCR1((status << 8) | ((options & 0xF) << 4) | (repcnt & 0xF));
  setCR2((ctrladdr << 8) | (track & 0xFF));
  setCR3((index << 8) | ((FAD >> 16) &0xFF));
  setCR4((unsigned short) FAD);
  setHIRQ(getHIRQ() | CDB_HIRQ_CMOK | CDB_HIRQ_ESEL);
}

void Cs2::resetSelector(void) {
  // still needs a bit of work
  unsigned long i;

  isbufferfull = false;

  // parse flags and reset the specified area(fix me)
  if (getCR1() & 0x80)
  {
     // reset false condition connections
     for (i = 0; i < MAX_SELECTORS; i++)
        filter[i].condfalse = 0xFF;
  }

  if (getCR1() & 0x40)
  {
     // reset true condition connections
     for (i = 0; i < MAX_SELECTORS; i++)
        filter[i].condtrue = 0;
  }

  if (getCR1() & 0x10)
  {
     // reset filter conditions
     for (i = 0; i < MAX_SELECTORS; i++)
     {
        filter[i].FAD = 0;
        filter[i].range = 0xFFFFFFFF;
        filter[i].mode = 0;
        filter[i].chan = 0;
        filter[i].smmask = 0;
        filter[i].cimask = 0;
        filter[i].fid = 0;
        filter[i].smval = 0;
        filter[i].cival = 0;
     }
  }

  if (getCR1() & 0x4)
  {
     // reset partitions
  }

  setCR1((status << 8) | ((options & 0xF) << 4) | (repcnt & 0xF));
  setCR2((ctrladdr << 8) | (track & 0xFF));
  setCR3((index << 8) | ((FAD >> 16) &0xFF));
  setCR4((unsigned short) FAD);
  setHIRQ(getHIRQ() | CDB_HIRQ_CMOK | CDB_HIRQ_ESEL);
}

void Cs2::getSectorNumber(void) {
  // somehow, I don't think this is right
  setCR4(partition[getCR3() >> 8].size / getsectsize);

  setCR1(status << 8);
  setCR2(0);
  setCR3(0);
  setHIRQ(getHIRQ() | CDB_HIRQ_CMOK | CDB_HIRQ_DRDY);
}

void Cs2::setSectorLength(void) {
  switch (getCR1() & 0xFF) {
    case 0:
            getsectsize = 2048;
            break;
    case 1:
            getsectsize = 2336;
            break;
    case 2:
            getsectsize = 2340;
            break;
    case 3:
            getsectsize = 2352;
            break;
    default: break;
  }

  setCR1((status << 8) | ((options & 0xF) << 4) | (repcnt & 0xF));
  setCR2((ctrladdr << 8) | (track & 0xFF));
  setCR3((index << 8) | ((FAD >> 16) &0xFF));
  setCR4((unsigned short) FAD);
  setHIRQ(getHIRQ() | CDB_HIRQ_CMOK | CDB_HIRQ_ESEL);
}

void Cs2::getThenDeleteSectorData(void) {
  cdwnum = 0;
  datatranstype = 0;
  databytestotrans = getsectsize * getCR4();

  setCR1((status << 8) | ((options & 0xF) << 4) | (repcnt & 0xF));
  setCR2((ctrladdr << 8) | (track & 0xFF));
  setCR3((index << 8) | ((FAD >> 16) &0xFF));
  setCR4((unsigned short) FAD);
  setHIRQ(getHIRQ() | CDB_HIRQ_CMOK | CDB_HIRQ_DRDY | CDB_HIRQ_EHST);
}

void Cs2::getCopyError(void) {
  setCR1(status << 8);
  setCR2(0);
  setCR3(0);
  setCR4(0);
  setHIRQ(getHIRQ() | CDB_HIRQ_CMOK);
}

void Cs2::changeDirectory(void) {
  // fix me
  setCR1((status << 8) | ((options & 0xF) << 4) | (repcnt & 0xF));
  setCR2((ctrladdr << 8) | (track & 0xFF));
  setCR3((index << 8) | ((FAD >> 16) &0xFF));
  setCR4((unsigned short) FAD);
  setHIRQ(getHIRQ() | CDB_HIRQ_CMOK | CDB_HIRQ_EFLS);
}

void Cs2::getFileSystemScope(void) {
  ReadFileSystem();

  setCR1(status << 8);
  setCR2(numfiles - 2);
  setCR3(0x0100);
  setCR4(0x0002);
  setHIRQ(getHIRQ() | CDB_HIRQ_CMOK | CDB_HIRQ_EFLS);
}

void Cs2::getFileInfo(void) {
  unsigned long gfifid;

  gfifid = ((getCR3() & 0xFF) << 16) | getCR4();

  if (gfifid == 0xFFFFFF)
  {
     // fix me
     setCR1(status << 8);
     setCR2(0x0600);
     setCR3(0);
     setCR4(0);
  }
  else
  {
     SetupFileInfoTransfer(gfifid);
     setCR1(status << 8);
     setCR2(0x06);
     setCR3(0);
     setCR4(0);
  }

  setHIRQ(getHIRQ() | CDB_HIRQ_CMOK | CDB_HIRQ_DRDY);

  transfercount = 0;
  infotranstype = 1;
}

void Cs2::readFile(void) {
  unsigned long rffid, rfoffset, rfsize;

  rffid = ((getCR3() & 0xFF) << 8) | getCR4();
  rfoffset = ((getCR1() & 0xFF) << 8) | getCR2();
  rfsize = ((fileinfo[rffid].size + getsectsize - 1) /
           getsectsize) - rfoffset;

  SetupDefaultPlayStats(FADToTrack(fileinfo[rffid].lba + rfoffset));

  playFAD = FAD = fileinfo[rffid].lba + rfoffset;
  playendFAD = playFAD + rfsize;

  if ((curpartition = GetPartition()) != NULL)
  {
     curpartition->size = 0;
  }

  options = 0x8;
  issubcodeqdecoded = true;

  status = CDB_STAT_PERI | CDB_STAT_PLAY;

  setCR1((status << 8) | ((options & 0xF) << 4) | (repcnt & 0xF));
  setCR2((ctrladdr << 8) | (track & 0xFF));
  setCR3((index << 8) | ((FAD >> 16) &0xFF));
  setCR4((unsigned short) FAD);
  setHIRQ(getHIRQ() | CDB_HIRQ_CMOK | CDB_HIRQ_EFLS);
}

void Cs2::abortFile(void) {
  if ((status & 0xF) != CDB_STAT_OPEN &&
      (status & 0xF) != CDB_STAT_NODISC)
     status = CDB_STAT_PERI | CDB_STAT_PAUSE;
  isonesectorstored = false;
  datatranstype = -1;
  cdwnum = 0;
  setCR1((status << 8) | ((options & 0xF) << 4) | (repcnt & 0xF));
  setCR2((ctrladdr << 8) | (track & 0xFF));
  setCR3((index << 8) | ((FAD >> 16) &0xFF));
  setCR4((unsigned short) FAD);
  setHIRQ(getHIRQ() | CDB_HIRQ_CMOK | CDB_HIRQ_EFLS);
}

void Cs2::mpegInit(void) {
  setCR1(status << 8);

  if (getCR2() == 0x0001) // software timer?
     setHIRQ(getHIRQ() | CDB_HIRQ_CMOK | CDB_HIRQ_MPCM | CDB_HIRQ_MPED | CDB_HIRQ_MPST); 
  else
     setHIRQ(getHIRQ() | CDB_HIRQ_CMOK | CDB_HIRQ_MPED | CDB_HIRQ_MPST); 

  setCR2(0);
  setCR3(0);
  setCR4(0);

  // future mpeg-related variables should be initialized here
}

void Cs2::cmdE0(void) {
  bool mpegauth;

  mpegauth = getCR2() & 0xFF;

  // Set registers all to invalid values(aside from status)

  status = CDB_STAT_PERI | CDB_STAT_BUSY;

  setCR1((status << 8) | 0xFF);
  setCR2(0xFFFF);
  setCR3(0xFFFF);
  setCR4(0xFFFF);

  if (mpegauth == 1)
  {
     setHIRQ(getHIRQ() | CDB_HIRQ_MPED);
     authval = 2;
  }
  else
  {
     // if authentication passes(obviously it always does), CDB_HIRQ_CSCT is set
     isonesectorstored = true;
     setHIRQ(getHIRQ() | CDB_HIRQ_EFLS | CDB_HIRQ_CSCT);
     authval = 4;
  }

  // Set registers all back to normal values

  status = CDB_STAT_PERI | CDB_STAT_PAUSE;

  setCR1((status << 8) | ((options & 0xF) << 4) | (repcnt & 0xF));
  setCR2((ctrladdr << 8) | (track & 0xFF));
  setCR3((index << 8) | ((FAD >> 16) &0xFF));
  setCR4((unsigned short) FAD);
  setHIRQ(getHIRQ() | CDB_HIRQ_CMOK);
}

void Cs2::cmdE1(void) {
  setCR1((status << 8));
  setCR2(authval);
  setCR3(0);
  setCR4(0);
  setHIRQ(getHIRQ() | CDB_HIRQ_CMOK);
}

void Cs2::cmdE2(void) {
  // fix me
  authval |= 0x300;
  setCR1((status << 8) | ((options & 0xF) << 4) | (repcnt & 0xF));
  setCR2((ctrladdr << 8) | (track & 0xFF));
  setCR3((index << 8) | ((FAD >> 16) &0xFF));
  setCR4((unsigned short) FAD);
  setHIRQ(getHIRQ() | CDB_HIRQ_CMOK | CDB_HIRQ_MPED);
}

unsigned char Cs2::FADToTrack(unsigned long val) {
  for (int i = 0; i < 99; i++)
  {
     if (TOC[i] == 0xFFFFFFFF) return 0xFF;

     if (val >= (TOC[i] & 0xFFFFFF) && val < (TOC[i+1] & 0xFFFFFF))
        return (i+1);
  }

  return 0;
}

void Cs2::SetupDefaultPlayStats(unsigned char track_number) {
  if (track_number != 0xFF)
  {
     options = 0;
     repcnt = 0;
     ctrladdr = (TOC[track_number - 1] & 0xFF000000) >> 24;
     index = 1;
     track = track_number;
     FAD = TOC[track_number - 1] & 0x00FFFFFF;
  }
}

block_struct *Cs2::AllocateBlock() {
  // find a free block
  for(unsigned long i = 0; i < 200; i++)
  {
     if (block[i].size == -1)
     {
        blockfreespace--;

        if (blockfreespace <= 0) isbufferfull = true;

        block[i].size = getsectsize;

        return (block + i);
     }
  }

  isbufferfull = true;

  return NULL;
}

void Cs2::FreeBlock(block_struct *blk) {
  blk->size = -1;
  blockfreespace++;
  isbufferfull = false;
  setHIRQ(getHIRQ() & ~CDB_HIRQ_BFUL);
}

void Cs2::SortBlocks(partition_struct *part) {
  unsigned long i, i2;
  block_struct *blktmp;

  for (i = 0; i < (MAX_BLOCKS-1); i++)
  {
     for (i2 = i+1; i2 < MAX_BLOCKS; i2++)
     {
        if (part->block[i] == NULL && part->block[i2] != NULL)
        {
           blktmp = part->block[i];
           part->block[i] = part->block[i2];
           part->block[i2] = blktmp;
        }
     }
  }
}

partition_struct *Cs2::GetPartition()
{
  // go through various filter conditions here(fix me)

  return &partition[curfilter->condtrue];
}

int Cs2::CopyDirRecord(unsigned char *buffer, dirrec_struct *dirrec)
{
  unsigned char *temp_pointer;

  temp_pointer = buffer;

  memcpy(&dirrec->recordsize, buffer, sizeof(dirrec->recordsize));
  buffer += sizeof(dirrec->recordsize);

  memcpy(&dirrec->xarecordsize, buffer, sizeof(dirrec->xarecordsize));
  buffer += sizeof(dirrec->xarecordsize);

#ifdef WORDS_BIGENDIAN
  buffer += sizeof(dirrec->lba);
  memcpy(&dirrec->lba, buffer, sizeof(dirrec->lba));
  buffer += sizeof(dirrec->lba);
#else
  memcpy(&dirrec->lba, buffer, sizeof(dirrec->lba));
  buffer += (sizeof(dirrec->lba) * 2);
#endif

#ifdef WORDS_BIGENDIAN
  buffer += sizeof(dirrec->size);
  memcpy(&dirrec->size, buffer, sizeof(dirrec->size));
  buffer += sizeof(dirrec->size);
#else
  memcpy(&dirrec->size, buffer, sizeof(dirrec->size));
  buffer += (sizeof(dirrec->size) * 2);
#endif

  dirrec->dateyear = buffer[0];
  dirrec->datemonth = buffer[1];
  dirrec->dateday = buffer[2];
  dirrec->datehour = buffer[3];
  dirrec->dateminute = buffer[4];
  dirrec->datesecond = buffer[5];
  dirrec->gmtoffset = buffer[6];
  buffer += 7;

  dirrec->flags = buffer[0];
  buffer += sizeof(dirrec->flags);

  dirrec->fileunitsize = buffer[0];
  buffer += sizeof(dirrec->fileunitsize);

  dirrec->interleavegapsize = buffer[0];
  buffer += sizeof(dirrec->interleavegapsize);

#ifdef WORDS_BIGENDIAN
  buffer += sizeof(dirrec->volumesequencenumber);
  memcpy(&dirrec->volumesequencenumber, buffer, sizeof(dirrec->volumesequencenumber));
  buffer += sizeof(dirrec->volumesequencenumber);
#else
  memcpy(&dirrec->volumesequencenumber, buffer, sizeof(dirrec->volumesequencenumber));
  buffer += (sizeof(dirrec->volumesequencenumber) * 2);
#endif

  dirrec->namelength = buffer[0];
  buffer += sizeof(dirrec->namelength);

  memset(dirrec->name, 0, sizeof(dirrec->name));
  memcpy(dirrec->name, buffer, dirrec->namelength);
  buffer += dirrec->namelength;

  // handle padding
  buffer += (1 - dirrec->namelength % 2);

  memset(&dirrec->xarecord, 0, sizeof(dirrec->xarecord));

  // sadily, this is the best way I can think of for detecting XA records

  if ((dirrec->recordsize - (buffer - temp_pointer)) == 14)
  {
     memcpy(&dirrec->xarecord.groupid, buffer, sizeof(dirrec->xarecord.groupid));
     buffer += sizeof(dirrec->xarecord.groupid);

     memcpy(&dirrec->xarecord.userid, buffer, sizeof(dirrec->xarecord.userid));
     buffer += sizeof(dirrec->xarecord.userid);

     memcpy(&dirrec->xarecord.attributes, buffer, sizeof(dirrec->xarecord.attributes));
     buffer += sizeof(dirrec->xarecord.attributes);

#ifndef WORDS_BIGENDIAN
     // byte swap it
     dirrec->xarecord.attributes = ((dirrec->xarecord.attributes & 0xFF00) >> 8) +
                                   ((dirrec->xarecord.attributes & 0x00FF) << 8);
#endif

     memcpy(&dirrec->xarecord.signature, buffer, sizeof(dirrec->xarecord.signature));
     buffer += sizeof(dirrec->xarecord.signature);

     memcpy(&dirrec->xarecord.filenumber, buffer, sizeof(dirrec->xarecord.filenumber));
     buffer += sizeof(dirrec->xarecord.filenumber);

     memcpy(dirrec->xarecord.reserved, buffer, sizeof(dirrec->xarecord.reserved));
     buffer += sizeof(dirrec->xarecord.reserved);
  }

  return 0;
}


void Cs2::ReadFileSystem()
{
  unsigned char tempsect[2048];
  unsigned char *workbuffer;
  unsigned long i;
  dirrec_struct dirrec;
  unsigned char numsectorsleft=0;

  // fix me (this function should support more than just the root directory)

  // read sector 16
  if (CDReadSector(16, 2048, tempsect) != 2048)
     return;

  // retrieve directory record's lba
  CopyDirRecord(tempsect + 0x9C, &dirrec);

  // now read sector using lba grabbed from above and start parsing
  if (CDReadSector(dirrec.lba, 2048, tempsect) != 2048)
     return;

  numsectorsleft = (dirrec.size / 2048) - 1;
  dirrec.lba++;
  workbuffer = tempsect;

  for (i = 0; i < MAX_FILES; i++)
  {
     CopyDirRecord(workbuffer, fileinfo + i);
     fileinfo[i].lba += 150;
     workbuffer+=fileinfo[i].recordsize;

     if (workbuffer[0] == 0)
     {
        if (numsectorsleft > 0)
        {
           if (CDReadSector(dirrec.lba, 2048, tempsect) != 2048)
              return;
           dirrec.lba++;
           numsectorsleft--;
           workbuffer = tempsect;
        }
        else
        {
           numfiles = i;
           break;
        }
     }
  }
}

void Cs2::SetupFileInfoTransfer(unsigned long fid) {
  transfileinfo[0] = (fileinfo[fid].lba & 0xFF000000) >> 24;
  transfileinfo[1] = (fileinfo[fid].lba & 0x00FF0000) >> 16;
  transfileinfo[2] = (fileinfo[fid].lba & 0x0000FF00) >> 8;
  transfileinfo[3] =  fileinfo[fid].lba & 0x000000FF;

  transfileinfo[4] = (fileinfo[fid].size & 0xFF000000) >> 24;
  transfileinfo[5] = (fileinfo[fid].size & 0x00FF0000) >> 16;
  transfileinfo[6] = (fileinfo[fid].size & 0x0000FF00) >> 8;
  transfileinfo[7] =  fileinfo[fid].size & 0x000000FF;

  transfileinfo[8] = fileinfo[fid].interleavegapsize;
  transfileinfo[9] = fileinfo[fid].fileunitsize;
  transfileinfo[10] = (unsigned char)fid;
  transfileinfo[11] = fileinfo[fid].flags;
}
