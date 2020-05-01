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

class RotrTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  RotrTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~RotrTest() { 
  }

virtual void SetUp() {
  printf("RotrTest::SetUp\n");
  
}

virtual void TearDown() {
  printf("RotrTest::TearDown\n");

}

};

TEST_F(RotrTest, normal) {

  MSH2->regs.R[4]=0x80000000;

  // rotr R[4]
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x4405 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  MSH2->regs.SR.all =( 0x000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x00, (MSH2->regs.SR.all&0x01) );
  EXPECT_EQ( 0x40000000, MSH2->regs.R[4] );

}



TEST_F(RotrTest, carry) {

  MSH2->regs.R[4]=0x00000001;

  // rotr R[4]
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x4405 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  MSH2->regs.SR.all =( 0x000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x01, (MSH2->regs.SR.all&0x01) );
  EXPECT_EQ( 0x80000000, MSH2->regs.R[4] );
}

TEST_F(RotrTest, from_carry) {

  MSH2->regs.R[4]=0x80000001;

  // rotr R[4]
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x4405 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  MSH2->regs.SR.all =( 0x000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x01, (MSH2->regs.SR.all&0x01) );
  EXPECT_EQ( 0xc0000000, MSH2->regs.R[4] );

}


}  // namespace
