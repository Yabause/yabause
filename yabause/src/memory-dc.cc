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

/*	For some reason or another, the memory code that was put in place around
	the time that 0.0.6 was released does not work at all on the Dreamcast port
	So, I am forced to keep this file around.
	
	By the way, this is a slightly modified version of the 0.0.5 memory.cc file
*/

#include "registres.hh"
#include "superh.hh"
#include "timer.hh"

Memory::Memory(unsigned long size) {
  this->size = size;
  if (size == 0) return;
  memory = new unsigned char[size];
}

Memory::~Memory(void) {
  if (size == 0) return;
  delete [] memory;
}

unsigned char Memory::getByte(unsigned long adr) {
  if (adr >= size) {
     printf("Bad memory access: %8x", adr);
  }
  return memory[adr];
}

void Memory::setByte(unsigned long adr, unsigned char valeur) {
  if (adr >= size) {
     printf("Bad memory access: %8x", adr);
  }
  memory[adr] = valeur;
}

unsigned short Memory::getWord(unsigned long adr) {
  if (adr >= size) {
     printf("Bad memory access: %8x", adr);
  }
  return (unsigned short) ((memory[adr] << 8) | memory[adr + 1]);
}

void Memory::setWord(unsigned long adr, unsigned short valeur) {
  if (adr >= size) {
     printf("Bad memory access: %8x", adr);
  }
  memory[adr] = (unsigned char) (valeur >> 8);
  memory[adr + 1] = (unsigned char) (valeur & 0x00FF);
}

unsigned long Memory::getLong(unsigned long adr) {
  if (adr >= size) {
     printf("Bad memory access: %8x", adr);
  }
  return (unsigned long) ((memory[adr] << 24) |
			  (memory[adr + 1] << 16) |
			  (memory[adr + 2] << 8) |
			  memory[adr + 3]);
}

void Memory::setLong(unsigned long adr, unsigned long valeur) {
  if (adr >= size) {
     printf("Bad memory access: %8x", adr);
  }
  memory[adr] = (unsigned char) (valeur >> 24);
  memory[adr + 1] = (unsigned char) ((valeur >> 16) & 0x00FF);
  memory[adr + 2] = (unsigned char) ((valeur >> 8) & 0x00FF);
  memory[adr + 3] = (unsigned char) (valeur & 0x00FF);
}

unsigned long Memory::getSize(void) const {
  return size;
}

void Memory::load(const char *filename, unsigned long adr) {
  file_t file = fs_open(filename, O_RDONLY);
  if(!file)	{
	  printf("File Not Found: %s\n", filename);
	  return;
  }
  if(fs_total(file) > size)	{
	  printf("File Too Large...\n");
	  return;
  }
  fs_read(file, ((char *) memory) + adr, fs_total(file));
  fs_close(file);
}

LoggedMemory::LoggedMemory(const char *n, Memory *mem, bool d) : Memory(0) {
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

unsigned char LoggedMemory::getByte(unsigned long adr) {
  unsigned char tmp = mem->getByte(adr);
#if DEBUG
  cerr << hex << nom << "\t: " << setw(10) << adr
	      << " -> " << setw(10) << (int) tmp << '\n';
#endif
  return tmp;
}

void LoggedMemory::setByte(unsigned long adr, unsigned char val) {
  mem->setByte(adr, val);
#if DEBUG
  cerr << hex << nom << "\t: " << setw(10) << adr
	      << " <- " << setw(10) << (int) val << '\n';
#endif
}

unsigned short LoggedMemory::getWord(unsigned long adr) {
  unsigned short ret = mem->getWord(adr);
#if DEBUG
  cerr << hex << nom << "\t: " << setw(10) << adr
	      << " -> " << setw(10) << ret << '\n';
#endif
  return ret;
}

void LoggedMemory::setWord(unsigned long adr, unsigned short val) {
  mem->setWord(adr, val);
#if DEBUG
  cerr << hex << nom << "\t: " << setw(10) << adr
	      << " <- " << setw(10) << val << '\n';
#endif
}

unsigned long LoggedMemory::getLong(unsigned long adr) {
  unsigned long ret = mem->getLong(adr);
#if DEBUG
  cerr << hex << nom << "\t: " << setw(10) << adr
	      << " -> " << setw(10) << ret << '\n';
#endif
  return ret;
}

void LoggedMemory::setLong(unsigned long adr, unsigned long val) {
  mem->setLong(adr, val);
#if DEBUG
  cerr << hex << nom << "\t: " << setw(10) << adr
	      << " <- " << setw(10) << val << '\n';
#endif
}

/*********************/
/*                   */
/* Saturn Memory     */
/*                   */
/*********************/


SaturnMemory::SaturnMemory(const char *bios, const char *exe) : Memory(0) {
  msh = new SuperH();

  Timer::initSuperH(msh);

  rom         = new Memory(0x80000);
  onchip      = new OnchipRegisters(msh);

  Intc * intc = ((OnchipRegisters *) onchip)->getIntc();
  
  ram         = new Memory(0x10000);
  ramLow      = new Memory(0x100000);
  cs0	      = new LoggedMemory("cs0", new Dummy(), 1);
  cs1         = new LoggedMemory("cs1", new Cs1(), 1);
  cs2         = new Cs2();
  sound       = new ScspRam(); //Memory(0x7FFFF);
  soundr      = new LoggedMemory("soundr", new Memory(0xEE4), 1);
  scu         = new ScuRegisters(intc);

  Scu * scup = ((ScuRegisters *) scu)->getScu();
  
  vdp1_1      = new Memory(0xC0000);
  vdp1_2      = new Vdp1Registers(vdp1_1, scup);
  vdp2_1      = new Vdp2Ram();
  vdp2_2      = new Vdp2ColorRam();
  vdp2_3      = new LoggedMemory("vdp2", new Vdp2Registers((Vdp2Ram *) vdp2_1,
			  	    (Vdp2ColorRam *) vdp2_2, scup,
				    ((Vdp1Registers *)vdp1_2)->getVdp1()), 1);
  smpc        = new SmpcRegisters(scup, this);
  ramHigh     = new Memory(0x100000);
  purgeArea   = new Dummy();
  adressArray = new Memory(0x3FF);
  modeSdram   = new Memory(0x4FFF);

  if (bios != NULL) rom->load(bios, 0);
  if (exe != NULL) {
    ramHigh->load(exe, 0x4000);
    rom->setLong(0, 0x06004000);
    rom->setLong(4, 0x06002000);
  }

  msh->setMemory(this);

  mshThread = SDL_CreateThread((int (*)(void*)) &SuperH::lancer, msh);
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
  delete vdp2_3;
  delete vdp2_1;
  delete vdp2_2;
  delete vdp1_2;
  delete vdp1_1;
  delete scu;
  delete ramHigh;
  delete purgeArea;
  delete adressArray;
  delete modeSdram;
  delete onchip;

  delete msh;
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
    printf("Bad memory access: %8x", adr);
  case 2:
    mapMem = purgeArea; mapAdr = adr; return;
  case 3: {
    unsigned long nadr = adr & 0x1FFFFFFF;
    if (nadr < 0x3FF) { mapMem = adressArray; mapAdr = nadr; return;}
    printf("Bad memory access: %8x", adr);
  }
  case 7:
    if ((adr >= 0xFFFF8000) && (adr < 0xFFFFC000)) {mapMem = modeSdram; mapAdr = adr & 0x00000FFF; return;}
    if (adr >= 0xFFFFFE00) { mapMem = onchip; mapAdr = adr & 0x000001FF; return;}
    printf("Bad memory access: %8x", adr);
  default:
    printf("Bad memory access: %8x", adr);
  }
}

unsigned char SaturnMemory::getByte(unsigned long adr) {
  mappage(adr);
  return mapMem->getByte(mapAdr);
}

void SaturnMemory::setByte(unsigned long adr, unsigned char valeur) {
  mappage(adr);
  mapMem->setByte(mapAdr, valeur);
}

unsigned short SaturnMemory::getWord(unsigned long adr) {
  mappage(adr);
  return mapMem->getWord(mapAdr);
}

void SaturnMemory::setWord(unsigned long adr, unsigned short valeur) {
  mappage(adr);
  mapMem->setWord(mapAdr, valeur);
}

unsigned long SaturnMemory::getLong(unsigned long adr) {
  mappage(adr);
  return mapMem->getLong(mapAdr);
}

void SaturnMemory::setLong(unsigned long adr, unsigned long valeur) {
  mappage(adr);
  mapMem->setLong(mapAdr, valeur);
}

void SaturnMemory::loadBios(const char *fichier) {
  rom->load(fichier, 0);
}

void SaturnMemory::load(const char *fichier, unsigned long adr) {
  mappage(adr);
  mapMem->load(fichier, mapAdr);
}

SuperH *SaturnMemory::getMasterSH(void) {
  return msh;
}
