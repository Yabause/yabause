#include "memory.hh"

class ScspRam : public Memory {
public:
	ScspRam(void) : Memory(0xFFFF, 0x7FFFF) {}

	unsigned char getByte(unsigned long addr) {
		switch(addr) {
		case 0x700:
		case 0x710:
		case 0x720:
		case 0x730:
		case 0x740:
		case 0x750:
		case 0x760:
		case 0x770:
			return 0;
			break;
		case 0x790:
		case 0x792:
			return 0;
			break;
		default:
			return Memory::getByte(addr);
		}
	}
	unsigned short getWord(unsigned long addr) {
		switch(addr) {
		case 0x700:
		case 0x710:
		case 0x720:
		case 0x730:
		case 0x740:
		case 0x750:
		case 0x760:
		case 0x770:
			return 0;
			break;
		case 0x790:
		case 0x792:
			return 0;
			break;
		default:
			return Memory::getWord(addr);
		}
	}
};
