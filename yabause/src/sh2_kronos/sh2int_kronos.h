#ifndef SH2INT_KRONOS_H
#define SH2INT_KRONOS_H

#define SH2CORE_KRONOS_INTERPRETER 8

typedef void (*opcode_func)(SH2_struct * sh);
typedef u16 (FASTCALL *fetchfunc)(SH2_struct *context, u32);

extern SH2Interface_struct SH2KronosInterpreter;

extern fetchfunc fetchlist[0x100];
extern opcode_func opcodeTable[0x10000];

#endif
