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

class TestbTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  TestbTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~TestbTest() {   
  }

virtual void SetUp() {
 
}

virtual void TearDown() {
}

};

TEST_F(TestbTest, normal) {

  MSH2->regs.R[0]=0x000000F0;

  SH2MappedMemoryWriteWord(MSH2, 0x000030F0, 0x00FB );  // nop

  // tst R[0]x, 0xFA
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0xCC04 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  MSH2->regs.SR.all =( 0x000000 );
  MSH2->regs.GBR =( 0x00003000 );

  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x01, (MSH2->regs.SR.all&0x01) );
}

}  // namespace
