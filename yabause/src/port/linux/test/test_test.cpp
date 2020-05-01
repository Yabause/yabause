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

class TestTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  TestTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~TestTest() {    
  }

virtual void SetUp() {
 
}

virtual void TearDown() {
}

};

TEST_F(TestTest, normal) {

  MSH2->regs.R[2]=0xF0F0F0F0;
  MSH2->regs.R[3]=0x0F0F0FFF;

  // tst R[0]x, 0xFA
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x2238 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  MSH2->regs.SR.all =( 0xE1 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x00, (MSH2->regs.SR.all&0x01) );
}

TEST_F(TestTest, zero) {

  MSH2->regs.R[2]=0xF0F0F0F0;
  MSH2->regs.R[3]=0x0F0F0F0F;

  // tst R[0]x, 0xFA
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x2238 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  MSH2->regs.SR.all =( 0xE0 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x01, (MSH2->regs.SR.all&0x01) );
}

TEST_F(TestTest, same) {

  MSH2->regs.R[2]=0xF0F0F0F0;

  // tst R[0]x, 0xFA
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x2228 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  MSH2->regs.SR.all =( 0x000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x00, (MSH2->regs.SR.all&0x01) );
}

TEST_F(TestTest, samezero) {

  MSH2->regs.R[2]=0x00000000;

  // tst R[0]x, 0xFA
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x2228 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  MSH2->regs.SR.all =( 0xE1 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x01, (MSH2->regs.SR.all&0x01) );
}




}  // namespace
