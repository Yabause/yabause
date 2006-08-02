#ifndef SH2INT_H
#define SH2INT_H

#define SH2CORE_INTERPRETER             0
#define SH2CORE_DEBUGINTERPRETER        1

#define INSTRUCTION_A(x) ((x & 0xF000) >> 12)
#define INSTRUCTION_B(x) ((x & 0x0F00) >> 8)
#define INSTRUCTION_C(x) ((x & 0x00F0) >> 4)
#define INSTRUCTION_D(x) (x & 0x000F)
#define INSTRUCTION_CD(x) (x & 0x00FF)
#define INSTRUCTION_BCD(x) (x & 0x0FFF)

int SH2InterpreterInit();
int SH2DebugInterpreterInit();
void SH2InterpreterDeInit();
int SH2InterpreterReset();
u32 FASTCALL SH2InterpreterExec(SH2_struct *context, u32 cycles);
u32 FASTCALL SH2DebugInterpreterExec(SH2_struct *context, u32 cycles);

extern SH2Interface_struct SH2Interpreter;
extern SH2Interface_struct SH2DebugInterpreter;

typedef u32 (FASTCALL *fetchfunc)(u32);
extern fetchfunc fetchlist[0x100];

typedef void (FASTCALL *opcodefunc)(SH2_struct *);
extern opcodefunc opcodes[0x10000];

#endif
