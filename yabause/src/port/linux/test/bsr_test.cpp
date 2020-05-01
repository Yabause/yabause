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

class BsrTest : public ::testing::Test {
 protected:
  SH2Interface_struct* pctx_;

  BsrTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~BsrTest() {   
  }

virtual void SetUp() {
  printf("BsrTest::SetUp\n");
  
}

virtual void TearDown() {
  printf("BsrTest::TearDown\n");

}

};

TEST_F(BsrTest, normal) {

  // rtcl R[0]

SH2MappedMemoryWriteWord(MSH2,0x06002E4C,0xB123);
SH2MappedMemoryWriteWord(MSH2,0x06002E4E, 0x0009);

  MSH2->regs.PC = ( 0x06002E4C );
  MSH2->regs.SR.all = ( 0x000000 );
  SH2TestExec(MSH2, 1);

EXPECT_EQ( 0x06003096, MSH2->regs.PC );
  EXPECT_EQ( 0x06002E50, MSH2->regs.PR );

}

TEST_F(BsrTest, negatif) {

  // rtcl R[0]

SH2MappedMemoryWriteWord(MSH2,0x06002E4C,0xB823);
SH2MappedMemoryWriteWord(MSH2,0x06002E4E, 0x0009);

  MSH2->regs.PC = ( 0x06002E4C );
  MSH2->regs.SR.all = ( 0x000000 );
  SH2TestExec(MSH2, 1);

EXPECT_EQ( 0x06002E4C - 4022, MSH2->regs.PC );
  EXPECT_EQ( 0x06002E50, MSH2->regs.PR );

}


TEST_F(BsrTest, bsrf) {

  MSH2->regs.R[1]=0x00006520; //source

  SH2MappedMemoryWriteWord(MSH2,0x06002E00,0x0103);
  SH2MappedMemoryWriteWord(MSH2,0x06002E02, 0x0009);



  MSH2->regs.PC = ( 0x06002E00 );
  MSH2->regs.SR.all = ( 0x000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x06009324, MSH2->regs.PC );
  EXPECT_EQ( 0x06002E04, MSH2->regs.PR );

}


}  // namespace
