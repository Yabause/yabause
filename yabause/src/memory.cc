/*  Copyright 2003 Guillaume Duhamel

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

#include "registres.hh"
#include "superh.hh"
#include "timer.hh"
#include "yui.hh"
#include "cd.hh"

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
	if (addr >= size) {
#ifndef _arch_dreamcast
		throw BadMemoryAccess(addr);
#else
		printf("Bad memory access: %8x", addr);
#endif
	}
#ifdef WORDS_BIGENDIAN
	return (memory + addr)[0];
#else
	return (memory - addr - 1)[0];
#endif
}

void Memory::setByte(unsigned long addr, unsigned char val) {
	addr &= mask;
	if (addr >= size) {
#ifndef _arch_dreamcast
		throw BadMemoryAccess(addr);
#else
		printf("Bad memory access: %8x", addr);
#endif
	}
#ifdef WORDS_BIGENDIAN
	(memory + addr)[0] = val;
#else
	(memory - addr - 1)[0] = val;
#endif
}

unsigned short Memory::getWord(unsigned long addr) {
	addr &= mask;
	if (addr >= size) {
#ifndef _arch_dreamcast
		throw BadMemoryAccess(addr);
#else
		printf("Bad memory access: %8x", addr);
#endif
	}
#ifdef WORDS_BIGENDIAN
  return ((unsigned short *) (memory + addr))[0];
#else
  return ((unsigned short *) (memory - addr - 2))[0];
#endif
}

void Memory::setWord(unsigned long addr, unsigned short val) {
	addr &= mask;
	if (addr >= size) {
#ifndef _arch_dreamcast
		throw BadMemoryAccess(addr);
#else
		printf("Bad memory access: %8x", addr);
#endif
	}
#ifdef WORDS_BIGENDIAN
	((unsigned short *) (memory + addr))[0] = val;
#else
	((unsigned short *) (memory - addr - 2))[0] = val;
#endif
}

unsigned long Memory::getLong(unsigned long addr) {
	addr &= mask;
	if (addr >= size) {
#ifndef _arch_dreamcast
		throw BadMemoryAccess(addr);
#else
		printf("Bad memory access: %8x", addr);
#endif
	}
#ifdef WORDS_BIGENDIAN
	return ((unsigned long *) (memory + addr))[0];
#else
	return ((unsigned long *) (memory - addr - 4))[0];
#endif
}

void Memory::setLong(unsigned long addr, unsigned long val) {
	addr &= mask;
	if (addr >= size) {
#ifndef _arch_dreamcast
		throw BadMemoryAccess(addr);
#else
		printf("Bad memory access: %8x", addr);
#endif
	}
#ifdef WORDS_BIGENDIAN
  ((unsigned long *) (memory + addr))[0] = val;
#else
  ((unsigned long *) (memory - addr - 4))[0] = val;
#endif
}

unsigned long Memory::getSize(void) const {
  return size;
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
  mshThread = NULL;
  msh = new SuperH();

  Timer::initSuperH(msh);

  rom         = new Memory(0xFFFFF, 0x80000);
  onchip      = new Onchip(this);
  ram         = new Memory(0xFFFF, 0x10000);
  ramLow      = new Memory(0xFFFFF, 0x100000);
  cs0	      = new Dummy(0xFFFFFF);
  cs1         = new Cs1();
  cs2         = new Cs2();
  sound       = new ScspRam(); //Memory(0x7FFFF);
  soundr      = new Memory(0xFFF, 0xEE4);
  scu         = new Scu(this);
  vdp1_2      = new Vdp1(this);
  vdp1_1      = ((Vdp1*) vdp1_2)->getVRam();
  vdp2_3      = new Vdp2(this);
  vdp2_1      = ((Vdp2 *) vdp2_3)->getVRam();
  vdp2_2      = ((Vdp2 *) vdp2_3)->getCRam();
  smpc        = new Smpc(this);
  ramHigh     = new Memory(0xFFFFF, 0x100000);
  purgeArea   = new Dummy(0xFFFFFFFF);
  adressArray = new Memory(0xFFF, 0x3FF);
  dataArray   = new Memory(0xFFF, 0x1000);
  modeSdram   = new Memory(0xFFF, 0x4FFF);
  initMemoryMap();

	char *bios;

	bios = yui_bios();
	if (bios == NULL) exit(1);
	else {
		try {
			rom->load(bios, 0);
		}
		catch (Exception e) {
			exit(1);
		}
	}
/*
	if (exe != NULL) {
		ramHigh->load(exe, 0x4000);
		rom->setLong(0, 0x06004000);
		rom->setLong(4, 0x06002000);
	}
*/

	msh->setMemory(this);
	mshThread = SDL_CreateThread((int (*)(void*)) &SuperH::lancer, msh);
	msh->run();
}

bool SaturnMemory::start(void) {
}

void SaturnMemory::stop(void) {
}

SaturnMemory::~SaturnMemory(void) {
#if DEBUG
  cerr << "stopping master sh2\n";
#endif
  msh->stop();
  SDL_WaitThread(mshThread, NULL);
#if DEBUG
  cerr << "master sh2 stopped\n";
#endif

  delete rom;
  delete smpc;
  delete ram;
  delete ramLow;
  delete cs0;
  delete cs1;
  delete cs2;
  delete sound;
  delete soundr;
  ((Vdp2 *) vdp2_3)->stop();
  delete vdp2_3;
/*
  delete vdp2_1;
  delete vdp2_2;
*/
  delete vdp1_2;
  //delete vdp1_1;
  delete scu;
  delete ramHigh;
  delete purgeArea;
  delete adressArray;
  delete modeSdram;
  delete onchip;

  delete msh;
}

unsigned char SaturnMemory::getByte(unsigned long adr) {
  mappage2(adr);
  return mapMem->getByte(mapAdr);
}

void SaturnMemory::setByte(unsigned long adr, unsigned char valeur) {
  mappage2(adr);
  mapMem->setByte(mapAdr, valeur);
}

unsigned short SaturnMemory::getWord(unsigned long addr) {
  mappage2(addr);
  return mapMem->getWord(mapAdr);
}

void SaturnMemory::setWord(unsigned long adr, unsigned short valeur) {
  mappage2(adr);
  mapMem->setWord(mapAdr, valeur);
}

unsigned long SaturnMemory::getLong(unsigned long adr) {
  mappage2(adr);
  return mapMem->getLong(mapAdr);
}

void SaturnMemory::setLong(unsigned long adr, unsigned long valeur) {
  mappage2(adr);
  mapMem->setLong(mapAdr, valeur);
}

void SaturnMemory::loadBios(const char *fichier) {
  rom->load(fichier, 0);
}

void SaturnMemory::load(const char *fichier, unsigned long adr) {
  mappage2(adr);
  mapMem->load(fichier, mapAdr);
}

SuperH *SaturnMemory::getMasterSH(void) {
	return msh;
}

Memory *SaturnMemory::getOnchip(void) {
	return onchip;
}

Memory *SaturnMemory::getVdp1Ram(void) {
	return vdp1_1;
}

Memory *SaturnMemory::getVdp1(void) {
	return vdp1_2;
}

Memory *SaturnMemory::getScu(void) {
	return scu;
}

Memory *SaturnMemory::getVdp2(void) {
	return vdp2_3;
}

void SaturnMemory::initMemoryHandler(int begin, int end, Memory * m) {
	for(int i = begin;i < end;i++)
		memoryMap[i] = m;
}

void SaturnMemory::initMemoryMap() {
	/*
	for(int i = 0;i < 0x800;i++)
		memory_map[i] = &unhandled;
		*/
	initMemoryHandler(    0,   0x8, rom);
	initMemoryHandler( 0x10,  0x11, smpc);
	initMemoryHandler( 0x18,  0x19, ram);
	initMemoryHandler( 0x20,  0x30, ramLow);
	/*
	initMemoryHandler(0x100, 0x101, minit);
	initMemoryHandler(0x180, 0x181, sinit);
	*/
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

void SaturnMemory::mappage2(unsigned long adr) {
  switch (adr >> 29) {
  case 0: // cache used
  case 1: // cache not used
  case 5:  { // cache not used
    unsigned long nadr = adr & 0x1FFFFFFF;
    mapMem = memoryMap[nadr >> 16]; mapAdr = adr & mapMem->mask; // FIXME
    //if ((mapMem->mask & mapAdr2) != mapAdr) cerr << hex << "mapAdr2=" << mapAdr2 << " mapAdr=" << mapAdr << endl;
    return;
    }
#ifndef _arch_dreamcast
    throw BadMemoryAccess(adr);
#else
    printf("Bad memory access: %8x", adr);
#endif
  case 2: // purge area
    mapMem = purgeArea; mapAdr = adr & mapMem->mask; return; // FIXME
  case 3: { // direct access to cache addresses
    unsigned long nadr = adr & 0x1FFFFFFF;
    if (nadr < 0x3FF) { mapMem = adressArray; mapAdr = nadr & mapMem->mask; return;} // FIXME
#ifndef _arch_dreamcast
    throw BadMemoryAccess(adr);
#else
    printf("Bad memory access: %8x", adr);
#endif
  }
  case 4:
  case 6:
	  mapMem = dataArray; mapAdr = adr & 0x1000; return;
  case 7:
    if ((adr >= 0xFFFF8000) && (adr < 0xFFFFC000)) {mapMem = modeSdram; mapAdr = adr & 0x00000FFF; return;}
    if (adr >= 0xFFFFFE00) { mapMem = onchip; mapAdr = adr & 0x000001FF; return;}
#ifndef _arch_dreamcast
    throw BadMemoryAccess(adr);
#else
    printf("Bad memory access: %8x", adr);
#endif
  default:
#ifndef _arch_dreamcast
    throw BadMemoryAccess(adr);
#else
    printf("Bad memory access: %8x", adr);
#endif
  }
}

void SaturnMemory::mappage(unsigned long adr) {
  switch (adr >> 29) {
  case 0:
  case 1: {
    unsigned long nadr = adr & 0x1FFFFFFF;
    if (nadr < 0x80000) { mapMem = rom; mapAdr = nadr; return; }
    if ((nadr >=  0x100000) && (nadr <  0x100080)) { mapMem = smpc;    mapAdr = nadr & 0x000000FF; return;}
    if ((nadr >=  0x180000) && (nadr <  0x190000)) { mapMem = ram;     mapAdr = nadr & 0x0000FFFF; return;}
    if ((nadr >=  0x200000) && (nadr <  0x300000)) { mapMem = ramLow;  mapAdr = nadr & 0x000FFFFF; return;}
    if ((nadr >= 0x2000000) && (nadr < 0x4000000)) { mapMem = cs0;     mapAdr = nadr & 0x00FFFFFF; return;}
    if ((nadr >= 0x4000000) && (nadr < 0x5000000)) { mapMem = cs1;     mapAdr = nadr & 0x00FFFFFF; return;}
    if ((nadr >= 0x5800000) && (nadr < 0x5900000)) { mapMem = cs2;     mapAdr = nadr & 0x000FFFFF; return;}
    if ((nadr >= 0x5A00000) && (nadr < 0x5A80000)) { mapMem = sound;   mapAdr = nadr & 0x0000FFFF; return;}
    if ((nadr >= 0x5B00000) && (nadr < 0x5B00EE4)) { mapMem = soundr;  mapAdr = nadr & 0x00000FFF; return;}
    if ((nadr >= 0x5C00000) && (nadr < 0x5CC0000)) { mapMem = vdp1_1;  mapAdr = nadr & 0x000FFFFF; return;}
    if ((nadr >= 0x5D00000) && (nadr < 0x5D00018)) { mapMem = vdp1_2;  mapAdr = nadr & 0x000000FF; return;}
    if ((nadr >= 0x5E00000) && (nadr < 0x5E80000)) { mapMem = vdp2_1;  mapAdr = nadr & 0x000FFFFF; return;}
    if ((nadr >= 0x5F00000) && (nadr < 0x5F01000)) { mapMem = vdp2_2;  mapAdr = nadr & 0x0000FFFF; return;}
    if ((nadr >= 0x5F80000) && (nadr < 0x5F80120)) { mapMem = vdp2_3;  mapAdr = nadr & 0x00000FFF; return;}
    if ((nadr >= 0x5FE0000) && (nadr < 0x5FE00D0)) { mapMem = scu;     mapAdr = nadr & 0x000000FF; return;}
    if ((nadr >= 0x6000000) && (nadr < 0x6100000)) { mapMem = ramHigh; mapAdr = nadr & 0x000FFFFF; return;}
    }
#ifndef _arch_dreamcast
    throw BadMemoryAccess(adr);
#else
    printf("Bad memory access: %8x", adr);
#endif
  case 2:
    mapMem = purgeArea; mapAdr = adr; return;
  case 3: {
    unsigned long nadr = adr & 0x1FFFFFFF;
    if (nadr < 0x3FF) { mapMem = adressArray; mapAdr = nadr; return;}
#ifndef _arch_dreamcast
    throw BadMemoryAccess(adr);
#else
    printf("Bad memory access: %8x", adr);
#endif
  }
  case 6:
	  mapMem = dataArray; mapAdr = adr & 0x1000; return;
  case 7:
    if ((adr >= 0xFFFF8000) && (adr < 0xFFFFC000)) {mapMem = modeSdram; mapAdr = adr & 0x00000FFF; return;}
    if (adr >= 0xFFFFFE00) { mapMem = onchip; mapAdr = adr & 0x000001FF; return;}
#ifndef _arch_dreamcast
    throw BadMemoryAccess(adr);
#else
    printf("Bad memory access: %8x", adr);
#endif
  default:
#ifndef _arch_dreamcast
    throw BadMemoryAccess(adr);
#else
    printf("Bad memory access: %8x", adr);
#endif
  }
}

bool SaturnMemory::running(void) {
	return (mshThread != NULL);
}
