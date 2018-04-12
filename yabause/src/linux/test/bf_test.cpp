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

class BfTest : public ::testing::Test {
 protected:
  SH2Interface_struct* pctx_;

  BfTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~BfTest() { 
  }

virtual void SetUp() {
  printf("BfTest::SetUp\n");
  
}

virtual void TearDown() {
  printf("BfTest::TearDown\n");

}

};

TEST_F(BfTest, normal) {

  // rtcl R[0]

  SH2MappedMemoryWriteWord(MSH2,0x06002E4C,0x8BFE);



  MSH2->regs.PC = ( 0x06002E4C );
  MSH2->regs.SR.all = ( 0x000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x06002E4C, MSH2->regs.PC );

}

TEST_F(BfTest, normal2) {

  // rtcl R[0]

  SH2MappedMemoryWriteWord(MSH2,0x06002E4C,0x8BF0);
  SH2MappedMemoryWriteWord(MSH2,0x06002E4E, 0x0009 );
  SH2MappedMemoryWriteWord(MSH2,0x06002E30, 0x000b );



  MSH2->regs.PC = ( 0x06002E4C );
  MSH2->regs.SR.all = ( 0x000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x06002E30, MSH2->regs.PC );

}

TEST_F(BfTest, carry) {

  // rtcl R[0]

  SH2MappedMemoryWriteWord(MSH2,0x06002E4C,0x8BFE);



  MSH2->regs.PC = ( 0x06002E4C );
  MSH2->regs.SR.all = ( 0x000001);
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x06002E4E, MSH2->regs.PC );

}

TEST_F(BfTest, bfs) {

  // rtcl R[0]

  SH2MappedMemoryWriteWord(MSH2,0x06002E4C,0x8F0B);
  SH2MappedMemoryWriteWord(MSH2, 0x06002E4E, 0x0009 );  // nop 

  MSH2->regs.PC = ( 0x06002E4C );
  MSH2->regs.SR.all = ( 0x000000);
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x06002E4C+4+(0xB<<1), MSH2->regs.PC );

  SH2MappedMemoryWriteWord(MSH2,0x06002E4C,0x8F0B);
  SH2MappedMemoryWriteWord(MSH2, 0x06002E4E, 0x0009 );  // nop 

  MSH2->regs.PC = ( 0x06002E4C );
  MSH2->regs.SR.all = ( 0x000001);
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x06002E4C+2, MSH2->regs.PC );

  MSH2->regs.R[3]=0x2;

  SH2MappedMemoryWriteWord(MSH2,0x06002E4C,0x8F8B);
  SH2MappedMemoryWriteWord(MSH2, 0x06002E4E, 0x7304 );  // add #4, r3
  SH2MappedMemoryWriteWord(MSH2, 0x06002E50, 0x0009 );  // nop 

  MSH2->regs.PC = ( 0x06002E4C );
  MSH2->regs.SR.all = ( 0x000000);
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x6, MSH2->regs.R[3] );
  EXPECT_EQ( 0x06002E4C+4+(0xFFFFFF8B<<1), MSH2->regs.PC );

  SH2MappedMemoryWriteWord(MSH2,0x06002E4C,0x8F8B);
  SH2MappedMemoryWriteWord(MSH2, 0x06002E4E, 0x0009 );  // nop 

  MSH2->regs.PC = ( 0x06002E4C );
  MSH2->regs.SR.all = ( 0x000001);
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x06002E4C+2, MSH2->regs.PC );

}

}  // namespace
