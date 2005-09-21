#ifndef SMPC_H
#define SMPC_H

#include "memory.h"

#define REGION_AUTODETECT               0
#define REGION_JAPAN                    1
#define REGION_ASIANTSC                 2
#define REGION_NORTHAMERICA             4
#define REGION_CENTRALSOUTHAMERICANTSC  5
#define REGION_KOREA                    6
#define REGION_ASIAPAL                  10
#define REGION_EUROPE                   12
#define REGION_CENTRALSOUTHAMERICAPAL   13

typedef struct {
        u8 IREG[7];
        u8 padding[8];
	u8 COMREG;
        u8 OREG[32];
	u8 SR;
	u8 SF;
        u8 padding2[8];
        u8 PDR[2];
        u8 DDR[2];
        u8 IOSEL;
        u8 EXLE;
} Smpc;

extern Smpc * SmpcRegs;
extern u8 * SmpcRegsT;

typedef struct {
   u8 dotsel; // 0 -> 320 | 1 -> 352
   u8 mshnmi;
   u8 sndres;
   u8 cdres;
   u8 sysres;
   u8 resb;
   u8 ste;
   u8 resd;
   u8 intback;
   u8 intbackIreg0;
   u8 firstPeri;
   u8 regionid;
   u8 SMEM[4];
   s32 timing;
} SmpcInternal;

int SmpcInit(u8 regionid);
void SmpcDeInit(void);
void SmpcReset(void);
void SmpcExec(s32 t);
void SmpcINTBACKEnd();

u8 FASTCALL	SmpcReadByte(u32);
u16 FASTCALL	SmpcReadWord(u32);
u32 FASTCALL	SmpcReadLong(u32);
void FASTCALL	SmpcWriteByte(u32, u8);
void FASTCALL	SmpcWriteWord(u32, u16);
void FASTCALL	SmpcWriteLong(u32, u32);

#endif
