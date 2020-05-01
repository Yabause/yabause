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

class SharTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  SharTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~SharTest() {   
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};


TEST_F(SharTest, normal) {

  MSH2->regs.R[3]=0xC; // m
  MSH2->regs.SR.all =(0x00000F1);

  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x4321 );  // shar
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x6, MSH2->regs.R[3] );
  EXPECT_EQ( 0x000000F0, MSH2->regs.SR.all );
}

TEST_F(SharTest, normal01) {

  MSH2->regs.R[3]=0x6; // m
  MSH2->regs.SR.all =(0x00000F0);

  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x4321 );  // shar
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x3, MSH2->regs.R[3] );
  EXPECT_EQ( 0x000000F0, MSH2->regs.SR.all );
}

TEST_F(SharTest, normal02) {

  MSH2->regs.R[3]=0x3; // m
  MSH2->regs.SR.all =(0x00000F0);

  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x4321 );  // shar
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x1, MSH2->regs.R[3] );
  EXPECT_EQ( 0x000000F1, MSH2->regs.SR.all );
}


}  // namespacegPtr
