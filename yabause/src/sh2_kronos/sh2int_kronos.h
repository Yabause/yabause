#ifndef SH2INT_KRONOS_H
#define SH2INT_KRONOS_H

#define SH2CORE_KRONOS_INTERPRETER 8

typedef void (FASTCALL *opcode_func)(SH2_struct *);
typedef u16 (FASTCALL *fetchfunc)(SH2_struct *context, u32);

extern SH2Interface_struct SH2KronosInterpreter;

extern fetchfunc krfetchlist[0x1000];
extern opcode_func opcodeTable[0x10000];


#endif
