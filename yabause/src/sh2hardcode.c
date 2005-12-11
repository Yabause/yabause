
#include "sh2core.h"
#include "memory.h"
#include "sh2hardcode.h"

#define OPCODE_HC 0x00000001

void albertOdysseyWait1( SH2_struct *sh ) {

  /* At 0x060806DC - 0x060806E2 : wait until @R4 == 0 */

  if ( MappedMemoryReadLong(sh->regs.R[4]) ) sh->cycles = 0xffffffff;
  else {
    sh->regs.PC = 0x060806E2;
    sh->cycles += 3;
  }
}

void albertOdysseyWait2( SH2_struct *sh ) {

  /* At 0x061F190 - 0x0601F196 : wait until @R14 == 0 */

  if ( MappedMemoryReadLong(sh->regs.R[14]) ) sh->cycles = 0xffffffff;
  else {
    sh->regs.PC = 0x0601F196;
    sh->cycles += 3;
  }
}

void albertOdysseyWaitSSH1( SH2_struct *sh ) {

  /* At 0x060919F6 - 0x06091A00 : wait until @R14 & R13 = R13 */

  if ( ~MappedMemoryReadByte(sh->regs.R[14]) & (u8)(sh->regs.R[13]) ) sh->cycles = 0xffffffff;
  else {
    sh->regs.PC = 0x06091A00;
    sh->cycles += 6;
  }
}

void jpbiosArithmetic1( SH2_struct *sh ) {

  /* Beginning of emulation */
  /* At 0x060101B6 - 0x060101C6 : arithmetic  */
  /* R6 <-w sum of integers in [#R5,#R4-1] */
  /* R2,R3,R5 <-w #R4 */

  u32 R5 = (u16)(sh->regs.R[5]);
  u32 R4 = (u16)(sh->regs.R[4]);
  sh->regs.R[5] = sh->regs.R[3] = sh->regs.R[2] = R4;
  if ( R4-1 >= R5 ) {
    sh->regs.R[6] = (R4-R5)*(R4-1+R5)/2;
    sh->cycles += 14*(R4-R5);
  }
  sh->regs.PC = 0x060101C6;
}

void jpbiosLoop1( SH2_struct *sh ) {

  /* At 0x6010D1E - 0x6010D2A */
  /* Strange waiting loop doing calculations always resulting in R6 = 0x1B3 */

  if ( MappedMemoryReadWord( MappedMemoryReadLong(0x6010D68) ) ) sh->cycles = 0xffffffff;
  else {
    sh->regs.PC = 0x06010D2A;
    sh->cycles += 0x1E*7+11;
    sh->regs.R[6] = 0x1B3;
  }
}

void jpbiosCopy1( SH2_struct *sh ) {

  /* At 0x0602E37E - 0x0602E386 */
  /* Copy from @R6 to @R5 */

  u32 R6 = sh->regs.R[6];
  u32 R5 = sh->regs.R[5];
  u32 R4 = sh->regs.R[4];
  sh->cycles += R4*4;
  for ( ; R4 ; R4--, R5+=4, R6+=4 ) MappedMemoryWriteLong( R5, MappedMemoryReadLong( R6 ) );
  sh->regs.PC = 0x0602E386;
}

void jpbiosWait1( SH2_struct *sh ) {

  /* At 0x06010088 - 0x06010092 */
  /* Wait until R4w = @(@6010114)w */
  /* this code is copied, must be relative to PC */

  u32 PC = sh->regs.PC;
  if ( MappedMemoryReadWord( MappedMemoryReadLong(PC+0x8E) ) == (u16)(sh->regs.R[4]) ) sh->cycles = 0xffffffff;
  else {
    sh->regs.PC += 0xA;
    sh->cycles += 5;
  }
}
 
u16 hackAlbertOdysseyWait1[3] = {0x6042,0x2008,0x8BFC};
u16 hackAlbertOdysseyWait2[3] = {0x60E0,0x2008,0x8BFC};
u16 hackAlbertOdysseyWaitSSH1[5] = {0x63E0,0x633C,0x23D9,0x33D0,0x8BFA};
u16 hackArithmetic1[4] = {0x655F,0x635F,0x363C,0x7501};
u16 hackJpbiosLoop1[6] = {0xBA45,0xE41E,0xD011,0x6001,0x2008,0x8BF9};
u16 hackJpbiosCopy1[4] = {0x6166,0x4410,0x2512,0x8FFB};
u16 hackJpbiosWait1[5] = {0x624F,0xD322,0x6331,0x3230,0x89FA};

hcTag hcTags[N_HARD_BLOCS] = {{0x060806DC,3,hackAlbertOdysseyWait1,albertOdysseyWait1},
			      {0x060101B6,4,hackArithmetic1,jpbiosArithmetic1},
			      {0x06010D1E,6,hackJpbiosLoop1,jpbiosLoop1},
			      {0x0602E37E,4,hackJpbiosCopy1,jpbiosCopy1},
			      {0x06010088,5,hackJpbiosWait1,jpbiosWait1},
                              {0x060919F6,5,hackAlbertOdysseyWaitSSH1,albertOdysseyWaitSSH1},
			      {0x0601F190,3,hackAlbertOdysseyWait2,albertOdysseyWait2}};

int hackDelay = 0;

void doCheckHcHacks() {

  int i,j;
  for ( i = 0 ; i < N_HARD_BLOCS ; i++ ) {
    
    u16* hack = hcTags[i].hack;
    u16* addr = (u16*)hcTags[i].addr;
    if ( MappedMemoryReadWord( (u32)(addr++) ) == hack[0] ) {
      int hackSize = hcTags[i].hackSize;
      for ( j=1 ; (j<hackSize)&&(hack[j]==MappedMemoryReadWord((u32)(addr++))) ; j++ ) ;;
      if ( j>=hackSize ) MappedMemoryWriteWord( hcTags[i].addr, OPCODE_HC | (i<<8) );
    }
  }
}

