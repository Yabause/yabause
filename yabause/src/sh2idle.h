#ifndef SH2IDLE_H
#define SH2IDLE_H

void FASTCALL SH2idleCheck(SH2_struct *context, u32 cycles);
void FASTCALL SH2idleParse(SH2_struct *context, u32 cycles);

#endif
