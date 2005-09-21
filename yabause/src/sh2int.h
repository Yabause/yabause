#ifndef SH2INT_H
#define SH2INT_H

#define SH2CORE_INTERPRETER     0

int SH2InterpreterInit();
void SH2InterpreterDeInit();
int SH2InterpreterReset();
u32 FASTCALL SH2InterpreterExec(SH2_struct *context, u32 cycles);

extern SH2Interface_struct SH2Interpreter;

#endif
