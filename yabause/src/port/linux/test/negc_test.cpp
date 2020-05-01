#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory.h"
extern "C" {
extern void initEmulation();
}
namespace {

class NegcTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  NegcTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~NegcTest() {    
  }   

virtual void SetUp() {
 
  
}

virtual void TearDown() {


}

};

TEST_F(NegcTest, normal) {
/*! Sign inversion of r0:r1 (64 bits)

clrt
negc   r1,r1    ! Before execution: r1 = 0x00000001, T = 0
                ! After execution: r1 = 0xFFFFFFFF, T = 1
negc   r0,r0    ! Before execution: r0 = 0x00000000, T = 1
                ! After execution: r0 = 0xFFFFFFFF, T = 1

- - - - - - - - - - - - - - - - 

! Store reversed T bit in r0

mov    #-1,r1
negc   r1,r0    ! r0 = 0 - (-1) - T
                ! r0 = 1 - T
                ! Notice that T bit will be modified by the negc operation.
                ! In this case, T will be always set to 1.
*/

  MSH2->regs.R[1]=0x00000001;
  MSH2->regs.SR.all =(0xE0);

  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x611A );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0xFFFFFFFF, MSH2->regs.R[1] );
  EXPECT_EQ( 0xE1, MSH2->regs.SR.all );

  MSH2->regs.R[0]=0x00000000;
  MSH2->regs.SR.all =(0xE1);

  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x600A );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );

  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0xFFFFFFFF, MSH2->regs.R[0] );
  EXPECT_EQ( 0xE1, MSH2->regs.SR.all );

  MSH2->regs.R[1]=0xFFFFFFFF;
  MSH2->regs.SR.all =(0xE0);

  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x601A );

  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );

  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0xFFFFFFFF, MSH2->regs.R[1] );
  EXPECT_EQ( 0x1, MSH2->regs.R[0] );
  EXPECT_EQ( 0xE1, MSH2->regs.SR.all );

  MSH2->regs.R[1]=0xFFFFFFFF;
  MSH2->regs.SR.all =(0xE1);

  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x601A );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);
  EXPECT_EQ( 0xFFFFFFFF, MSH2->regs.R[1] );
  EXPECT_EQ( 0x0, MSH2->regs.R[0] );
  EXPECT_EQ( 0xE1, MSH2->regs.SR.all );

  MSH2->regs.R[1]=0x1;

  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x621B );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);
  EXPECT_EQ( 0xFFFFFFFF, MSH2->regs.R[2] );

}

}  // namespace
