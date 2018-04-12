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

class ShllTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  ShllTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~ShllTest() {   
  }   

virtual void SetUp() {
   
}

virtual void TearDown() {
}

};

TEST_F(ShllTest, normal) {

  MSH2->regs.R[2]=0x00000001;
  MSH2->regs.SR.all =(0x000000E0);

  // shlr2
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x4200 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x00000002, MSH2->regs.R[2] );
  EXPECT_EQ( 0x000000E0, MSH2->regs.SR.all );

}

TEST_F(ShllTest, shift2) {

  MSH2->regs.R[2]=0xCAFEDEAD;

  // shll2
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x4208 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0xCAFEDEAD << 2, MSH2->regs.R[2] );
}

TEST_F(ShllTest, shift8) {

  MSH2->regs.R[2]=0xCAFEDEAD;
  MSH2->regs.SR.all =(0x000000E0);

  // shll16
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x4218 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0xFEDEAD00, MSH2->regs.R[2] );
}

TEST_F(ShllTest, shift16) {

  MSH2->regs.R[2]=0xCAFEDEAD;
  MSH2->regs.SR.all =(0x000000E0);

  // shll16
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x4228 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0xDEAD0000, MSH2->regs.R[2] );
}

TEST_F(ShllTest, tflg) {

  MSH2->regs.R[2]=0x80000001;
  MSH2->regs.SR.all =(0x000000E0);

  // shlr2
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x4200 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x00000002, MSH2->regs.R[2] );
  EXPECT_EQ( 0x000000E1, MSH2->regs.SR.all );

}




}  // namespace
