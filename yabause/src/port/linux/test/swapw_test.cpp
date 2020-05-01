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

class SwapwTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  SwapwTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~SwapwTest() {   
  }   

virtual void SetUp() {
 
  
}

virtual void TearDown() {


}

};

TEST_F(SwapwTest, normal) {

  // 0601157E: swapwin 6249, R[2]=06034CE8 R[4]=00120000
  // 0601157E: swapwout 6249, R[2]=00000012 R[4]=00120000

  MSH2->regs.R[2]=0x06034CE8;
  MSH2->regs.R[4]=0x00120000;

  // swap.w r2,r4
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x6249 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x00000012, MSH2->regs.R[2] );
  EXPECT_EQ( 0x00120000, MSH2->regs.R[4] );

}
TEST_F(SwapwTest, normal2) {

  // 0602EA70: swapwin 6239, R[2]=00000002 R[3]=AABBCCDD
  // 0602EA70: swapwout 6239, R[2]=CCDDAABB R[3]=AABBCCDD

  MSH2->regs.R[2]=0x00000002;
  MSH2->regs.R[3]=0xAABBCCDD;

  // swap.w r2,r3
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x6239 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0xCCDDAABB, MSH2->regs.R[2] );
  EXPECT_EQ( 0xAABBCCDD, MSH2->regs.R[3] );

}

}  // namespace
