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

class SubvTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  SubvTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~SubvTest() {   
  }

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

/*
subv   r0,r1    ! Before execution: r0 = 0x00000002, r1 = 0x80000001
                ! After execution: r1 = 0x7FFFFFFF, T = 1

subv   r2,r3    ! Before execution: r2 = 0xFFFFFFFE, r3 = 0x7FFFFFFE
                ! After execution r3 = 0x80000000, T = 1
*/

TEST_F(SubvTest, normal) {

  MSH2->regs.R[0]=0x00000002;
  MSH2->regs.R[1]=0x80000001;
  MSH2->regs.SR.all =(0xE0);

  // subv   r0,r1
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x310B );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x00000002, MSH2->regs.R[0] );
  EXPECT_EQ( 0x7FFFFFFF, MSH2->regs.R[1] );
  EXPECT_EQ( 0xE1, MSH2->regs.SR.all );

  MSH2->regs.R[2]=0xFFFFFFFE;
  MSH2->regs.R[3]=0x7FFFFFFE;
  MSH2->regs.SR.all =(0xE0);

  // subv   r2,r3
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x332B );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );

  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0xFFFFFFFE, MSH2->regs.R[2] );
  EXPECT_EQ( 0x80000000, MSH2->regs.R[3] );
  EXPECT_EQ( 0xE1, MSH2->regs.SR.all );
}

}  // namespacegPtr
