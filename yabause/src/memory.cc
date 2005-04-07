/*  Copyright 2003,2004 Guillaume Duhamel
    Copyright 2004-2005 Theo Berkau

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

#include "saturn_memory.hh"
#include "superh.hh"
#include "yui.hh"
#include "cdbase.hh"

Memory::Memory(unsigned long m, unsigned long size) {
  mask = m;
  this->size = size;
  if (size == 0) return;
  base_mem = new unsigned char[size];
  memset(base_mem, 0, size);
#ifdef WORDS_BIGENDIAN
  memory = base_mem;
#else
  memory = base_mem + size;
#endif
}

Memory::~Memory(void) {
	if (size == 0) return;
	delete [] base_mem;
}

unsigned char Memory::getByte(unsigned long addr) {
	addr &= mask;
/*
	if (addr >= size) {
#ifndef _arch_dreamcast
		throw BadMemoryAccess(addr);
#else
		printf("Bad memory access: %8x", addr);
#endif
	}
*/
#ifdef WORDS_BIGENDIAN
	return (memory + addr)[0];
#else
	return (memory - addr - 1)[0];
#endif
}

void Memory::setByte(unsigned long addr, unsigned char val) {
	addr &= mask;
/*
	if (addr >= size) {
#ifndef _arch_dreamcast
		throw BadMemoryAccess(addr);
#else
		printf("Bad memory access: %8x", addr);
#endif
	}
*/
#ifdef WORDS_BIGENDIAN
	(memory + addr)[0] = val;
#else
	(memory - addr - 1)[0] = val;
#endif
}

unsigned short Memory::getWord(unsigned long addr) {
	addr &= mask;
/*
	if (addr >= size) {
#ifndef _arch_dreamcast
		throw BadMemoryAccess(addr);
#else
		printf("Bad memory access: %8x", addr);
#endif
	}
*/
#ifdef WORDS_BIGENDIAN
  return ((unsigned short *) (memory + addr))[0];
#else
  return ((unsigned short *) (memory - addr - 2))[0];
#endif
}

void Memory::setWord(unsigned long addr, unsigned short val) {
	addr &= mask;
/*
	if (addr >= size) {
#ifndef _arch_dreamcast
		throw BadMemoryAccess(addr);
#else
		printf("Bad memory access: %8x", addr);
#endif
	}
*/
#ifdef WORDS_BIGENDIAN
	((unsigned short *) (memory + addr))[0] = val;
#else
	((unsigned short *) (memory - addr - 2))[0] = val;
#endif
}

unsigned long Memory::getLong(unsigned long addr) {
	addr &= mask;
/*
	if (addr >= size) {
#ifndef _arch_dreamcast
		throw BadMemoryAccess(addr);
#else
		printf("Bad memory access: %8x", addr);
#endif
	}
*/
#ifdef WORDS_BIGENDIAN
	return ((unsigned long *) (memory + addr))[0];
#else
	return ((unsigned long *) (memory - addr - 4))[0];
#endif
}

void Memory::setLong(unsigned long addr, unsigned long val) {
	addr &= mask;
/*
	if (addr >= size) {
#ifndef _arch_dreamcast
		throw BadMemoryAccess(addr);
#else
		printf("Bad memory access: %8x", addr);
#endif
	}
*/
#ifdef WORDS_BIGENDIAN
  ((unsigned long *) (memory + addr))[0] = val;
#else
  ((unsigned long *) (memory - addr - 4))[0] = val;
#endif
}

unsigned long Memory::getSize(void) const {
  return size;
}

unsigned char *Memory::getBuffer(void) const {
  return base_mem;
}

void Memory::load(const char *filename, unsigned long addr) {
#ifndef _arch_dreamcast
	struct stat infos;
	unsigned long i;
	char buffer;

	if (stat(filename,&infos) == -1)
		throw FileNotFound(filename);

	if (((unsigned int) infos.st_size) > size) {
		cerr << filename << " : File too big" << endl;
		throw BadMemoryAccess(addr);
	}

	ifstream exe(filename, ios::in | ios::binary);
	for(i = addr;i < (infos.st_size + addr);i++) {
		exe.read(&buffer, 1);
		setByte(i, buffer);
	}
	exe.close();
#else
	file_t file = fs_open(filename, O_RDONLY);
	if(!file) {
		printf("File Not Found: %s\n", filename);
		return;
	}
	if(fs_total(file) > size)     {
		printf("File Too Large...\n");
		return;
	}
	
	unsigned long i;
	char buffer;
	for(i = addr;i < fs_total(file) + addr;i++) {
		fs_read(file, &buffer, 1);
		setByte(i, buffer);
	}
	fs_close(file);
#endif
}

void Memory::save(const char *filename, unsigned long addr, unsigned long size) {
#ifndef _arch_dreamcast
	unsigned long i;
	char buffer;

        ofstream exe(filename, ios::out | ios::binary);
        for(i = addr;i < (addr + size);i++) {
                buffer = getByte(i);
                exe.write(&buffer, 1);
        }
        exe.close();
#endif
}

#ifndef _arch_dreamcast
ostream& operator<<(ostream& os, const Memory& mem) {
  for(unsigned long i = 0;i < mem.size;i++) {
    os << hex << setw(6) << i << " -> " << setw(10) << mem.memory[i] << endl;
  }
  return os;
}
#endif

LoggedMemory::LoggedMemory(const char *n, Memory *mem, bool d) : Memory(0, 0) {
  strcpy(nom, n);
  this->mem = mem;
  destroy = d;
  size = mem->getSize();
}

LoggedMemory::~LoggedMemory(void) {
  if(destroy) delete mem;
}

Memory *LoggedMemory::getMemory(void) {
  return mem;
}

unsigned char LoggedMemory::getByte(unsigned long addr) {
  unsigned char tmp = mem->getByte(addr);
#if DEBUG
  cerr << hex << nom << "\t: " << setw(10) << addr
	      << " -> " << setw(10) << (int) tmp << '\n';
#endif
  return tmp;
}

void LoggedMemory::setByte(unsigned long addr, unsigned char val) {
  mem->setByte(addr, val);
#if DEBUG
  cerr << hex << nom << "\t: " << setw(10) << addr
	      << " <- " << setw(10) << (int) val << '\n';
#endif
}

unsigned short LoggedMemory::getWord(unsigned long addr) {
  unsigned short ret = mem->getWord(addr);
#if DEBUG
  cerr << hex << nom << "\t: " << setw(10) << addr
	      << " -> " << setw(10) << ret << '\n';
#endif
  return ret;
}

void LoggedMemory::setWord(unsigned long addr, unsigned short val) {
  mem->setWord(addr, val);
#if DEBUG
  cerr << hex << nom << "\t: " << setw(10) << addr
	      << " <- " << setw(10) << val << '\n';
#endif
}

unsigned long LoggedMemory::getLong(unsigned long addr) {
  unsigned long ret = mem->getLong(addr);
#if DEBUG
  cerr << hex << nom << "\t: " << setw(10) << addr
	      << " -> " << setw(10) << ret << '\n';
#endif
  return ret;
}

void LoggedMemory::setLong(unsigned long addr, unsigned long val) {
  mem->setLong(addr, val);
#if DEBUG
  cerr << hex << nom << "\t: " << setw(10) << addr
	      << " <- " << setw(10) << val << '\n';
#endif
}

/*********************/
/*                   */
/* Saturn Memory     */
/*                   */
/*********************/


SaturnMemory::SaturnMemory(void) : Memory(0, 0) {
        msh = new SuperH(false, this);
        ssh = new SuperH(true, this);

	rom         = new Memory(0xFFFFF, 0x80000);
	ram         = new Memory(0xFFFF, 0x10000);
	ramLow      = new Memory(0xFFFFF, 0x100000);
        minit = new InputCaptureSignal(ssh);
        sinit = new InputCaptureSignal(msh);
        cs0         = new Cs0(NULL, CART_NONE);
        cs1         = new Cs1(NULL, CART_NONE);
        cs2         = new Cs2(this, CART_NONE);
        soundr      = new Scsp(this);
        sound       = ((Scsp *)soundr)->getSRam();
	scu         = new Scu(this);
	vdp1_2      = new Vdp1(this);
	vdp1_1      = ((Vdp1*) vdp1_2)->getVRam();
	vdp2_3      = new Vdp2(this);
	vdp2_1      = ((Vdp2 *) vdp2_3)->getVRam();
	vdp2_2      = ((Vdp2 *) vdp2_3)->getCRam();
	smpc        = new Smpc(this);
	ramHigh     = new Memory(0xFFFFF, 0x100000);
        unhandled   = new UnHandled();

	initMemoryMap();

	const char *bios;

	bios = yui_bios();
	if (bios == NULL) {
		printf("No bios selected!\n");
		exit(1);
	} else {
		try {
			rom->load(bios, 0);
		}
		catch (Exception e) {
			printf("Couldn't load bios '%s'!\n", bios);
			exit(1);
		}
	}

        // Load Backup Ram file
        const char *backupram;

        backupram = yui_saveram();
        if (backupram != NULL)
        {
           // it doesn't really matter if the load succeeds
           try
           {
              ram->load(backupram, 0);
           }
           catch (Exception e)
           {
              // Format Ram then
              FormatBackupRam();
           }
        }
        else
        {
           FormatBackupRam();
        }

/*
	decilineCount = 0;
	lineCount = 0;
	decilineStop = 170;
	duf = 268;
	cycleCountII = 0;
*/
        // Set the sh2 timing to the default
        changeTiming(26846587, false);

	msh->setMemory(this);
	sshRunning = false;
}

SaturnMemory::~SaturnMemory(void) {
  const char *backupram;
#if DEBUG
  cerr << "stopping master sh2\n";
#endif
#if DEBUG
  cerr << "master sh2 stopped\n";
#endif

  // Save Backup Ram file
  backupram = yui_saveram();
  if (backupram != NULL)
     ram->save(backupram, 0, 0x10000);

  delete rom;
  delete smpc;
  delete ram;
  delete ramLow;
  delete minit;
  delete sinit;
  delete cs0;
  delete cs1;
  delete cs2;
  delete soundr;
  delete vdp2_3;
  delete vdp1_2;
  delete scu;
  delete ramHigh;
  delete unhandled;

  delete msh;
  delete ssh;
}

unsigned char SaturnMemory::getByte(unsigned long addr) {
	mapping(addr);
	return mapMem->getByte(mapAddr);
}

void SaturnMemory::setByte(unsigned long addr, unsigned char val) {
	mapping(addr);
	mapMem->setByte(mapAddr, val);
}

unsigned short SaturnMemory::getWord(unsigned long addr) {
	mapping(addr);
	return mapMem->getWord(mapAddr);
}

void SaturnMemory::setWord(unsigned long addr, unsigned short val) {
	mapping(addr);
	mapMem->setWord(mapAddr, val);
}

unsigned long SaturnMemory::getLong(unsigned long addr) {
	mapping(addr);
	return mapMem->getLong(mapAddr);
}

void SaturnMemory::setLong(unsigned long addr, unsigned long val) {
	mapping(addr);
	mapMem->setLong(mapAddr, val);
}

void SaturnMemory::loadBios(const char *filename) {
	rom->load(filename, 0);
}

void SaturnMemory::loadExec(const char *filename, unsigned long PC) {
  sh2regs_struct sh2regs;
  load(filename, PC);
  msh->GetRegisters(&sh2regs);
  sh2regs.PC = PC;
  msh->SetRegisters(&sh2regs);
}

void SaturnMemory::FormatBackupRam() {
  int i, i2;
  unsigned char header[32] = {
  0xFF, 'B', 0xFF, 'a', 0xFF, 'c', 0xFF, 'k',
  0xFF, 'U', 0xFF, 'p', 0xFF, 'R', 0xFF, 'a',
  0xFF, 'm', 0xFF, ' ', 0xFF, 'F', 0xFF, 'o',
  0xFF, 'r', 0xFF, 'm', 0xFF, 'a', 0xFF, 't'
  };

  // Fill in header
  for(i2 = 0; i2 < 4; i2++) {
     for(i = 0; i < 32; i++)
        ram->setByte((i2 * 32) + i, header[i]);
  }

  // Clear the rest
  for(i = 0x80; i < 0x10000; i+=2) {
     ram->setByte(i, 0xFF);
     ram->setByte(i+1, 0x00);
  }
}

void SaturnMemory::load(const char *fichier, unsigned long adr) {
  mapping(adr);
  mapMem->load(fichier, mapAddr);
}

SuperH *SaturnMemory::getMasterSH(void) {
	return msh;
}

SuperH *SaturnMemory::getCurrentSH(void) {
        return cursh;
}

SuperH *SaturnMemory::getSlaveSH(void) {
        return ssh;
}

void SaturnMemory::initMemoryHandler(int begin, int end, Memory * m) {
	for(int i = begin;i < end;i++)
		memoryMap[i] = m;
}

void SaturnMemory::initMemoryMap() {
	for(int i = 0;i < 0x800;i++)
                memoryMap[i] = unhandled;

	initMemoryHandler(    0,   0x8, rom);
	initMemoryHandler( 0x10,  0x11, smpc);
	initMemoryHandler( 0x18,  0x19, ram);
	initMemoryHandler( 0x20,  0x30, ramLow);
        initMemoryHandler(0x100, 0x180, minit);
        initMemoryHandler(0x180, 0x200, sinit);
	initMemoryHandler(0x200, 0x400, cs0);
	initMemoryHandler(0x400, 0x500, cs1);
	initMemoryHandler(0x580, 0x590, cs2);
	initMemoryHandler(0x5A0, 0x5A8, sound);
	initMemoryHandler(0x5B0, 0x5B1, soundr);
        initMemoryHandler(0x5C0, 0x5CC, vdp1_1);
	initMemoryHandler(0x5D0, 0x5D1, vdp1_2);
	initMemoryHandler(0x5E0, 0x5E8, vdp2_1);
	initMemoryHandler(0x5F0, 0x5F1, vdp2_2);
	initMemoryHandler(0x5F8, 0x5F9, vdp2_3);
	initMemoryHandler(0x5FE, 0x5FF, scu);
	initMemoryHandler(0x600, 0x610, ramHigh);
}

void SaturnMemory::mapping(unsigned long addr) {
	switch (addr >> 29) {
		case 0: // cache used
		case 1: // cache not used
		case 5:  { // cache not used
			unsigned long naddr = addr & 0x1FFFFFFF;
			mapMem = memoryMap[naddr >> 16];
			mapAddr = addr & mapMem->mask;
			return;
			}
#ifndef _arch_dreamcast
			throw BadMemoryAccess(addr);
#else
			printf("Bad memory access: %8x", addr);
#endif
		case 2: // purge area
			mapMem = cursh->purgeArea;
			mapAddr = addr & mapMem->mask;
			return;
		case 3: { // direct access to cache addresses
			unsigned long naddr = addr & 0x1FFFFFFF;
			if (naddr < 0x3FF) {
				mapMem = cursh->adressArray;
				mapAddr = naddr & mapMem->mask;
				return;
			}
#ifndef _arch_dreamcast
			throw BadMemoryAccess(addr);
#else
			printf("Bad memory access: %8x", addr);
#endif
			}
		case 4:
		case 6:
			mapMem = cursh->dataArray;
			mapAddr = addr & 0x00000FFF;
			return;
		case 7:
			if ((addr >= 0xFFFF8000) && (addr < 0xFFFFC000)) {
				mapMem = cursh->modeSdram;
				mapAddr = addr & 0x00000FFF;
				return;
			}
			if (addr >= 0xFFFFFE00) {
				mapMem = cursh->onchip;
				mapAddr = addr & 0x000001FF;
				return;
			}
#ifndef _arch_dreamcast
			throw BadMemoryAccess(addr);
#else
			printf("Bad memory access: %8x", addr);
#endif
		default:
#ifndef _arch_dreamcast
			throw BadMemoryAccess(addr);
#else
			printf("Bad memory access: %8x", addr);
#endif
	}
}

void SaturnMemory::changeTiming(unsigned long sh2freq, bool pal) {
   // Setup all the variables related to timing

   decilineCount = 0;
   lineCount = 0;

   if (pal)
      decilineStop = sh2freq / 312 / 10 / 50; // I'm guessing this is wrong
   else
      decilineStop = sh2freq / 263 / 10 / 60; // I'm guessing this is wrong

   duf = sh2freq / 100000;

   cycleCountII = 0;
}


void SaturnMemory::synchroStart(void) {
        cursh = msh;
        msh->runCycles(decilineStop);
        if (sshRunning) {
           cursh = ssh;
           ssh->runCycles(decilineStop);
        }

	decilineCount++;
	switch(decilineCount) {
		case 9:
			// HBlankIN
			((Vdp2 *) vdp2_3)->HBlankIN();
			break;
		case 10:
			// HBlankOUT
			((Vdp2 *) vdp2_3)->HBlankOUT();
                        ((Scsp *)soundr)->run();
			decilineCount = 0;
			lineCount++;
			switch(lineCount) {
				case 224:
					// VBlankIN
                                        ((Smpc *) smpc)->INTBACKEnd();
					((Vdp2 *) vdp2_3)->VBlankIN();
					break;
				case 263:
					// VBlankOUT
					((Vdp2 *) vdp2_3)->VBlankOUT();
					lineCount = 0;
					break;
			}
			break;
	}
	cycleCountII += msh->cycleCount;

	while (cycleCountII > duf) {
		((Smpc *) smpc)->execute2(10);
                ((Cs2 *)cs2)->run(10);
		msh->run(10);
                ssh->run(10);
                ((Scu *)scu)->run(10);
		cycleCountII %= duf;
	}

        ((Scsp *)soundr)->run68k(170);

	msh->cycleCount %= decilineStop;
        if (sshRunning) 
          ssh->cycleCount %= decilineStop;
}

void SaturnMemory::startSlave(void) {
	ssh->setMemory(this);
	sshRunning = true;
}

void SaturnMemory::stopSlave(void) {
        ssh->reset();
	sshRunning = false;
}

