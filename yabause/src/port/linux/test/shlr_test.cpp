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

class ShlrTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  ShlrTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~ShlrTest() {    
  }   

virtual void SetUp() {
   
}

virtual void TearDown() {
}

};

TEST_F(ShlrTest, normal) {

  MSH2->regs.R[2]=0x00000001;
  MSH2->regs.SR.all =(0);

  // shlr2
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x4201 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x00000000, MSH2->regs.R[2] );
  EXPECT_EQ( 0x00000001, MSH2->regs.SR.all );

}

TEST_F(ShlrTest, shift2) {

  MSH2->regs.R[2]=0x0000003D;
  MSH2->regs.SR.all =(0xE0);

  // shlr2
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x4209 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x0000000F, MSH2->regs.R[2] );
  EXPECT_EQ( 0xE0, MSH2->regs.SR.all );

}

TEST_F(ShlrTest, shift8) {

  MSH2->regs.R[2]=0xFF003D01;
  MSH2->regs.SR.all =(0x1);

  // shlr8
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x4219 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x00FF003D, MSH2->regs.R[2] );
  EXPECT_EQ( 0x00000001, MSH2->regs.SR.all );

}

TEST_F(ShlrTest, shift16) {

  MSH2->regs.R[2]=0xDEADCAFE;
  MSH2->regs.SR.all =(1);

  // shlr16
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x4229 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x0000DEAD, MSH2->regs.R[2] );
  EXPECT_EQ( 0x00000001, MSH2->regs.SR.all );

}



}  // namespace
