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

class SwapbTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  SwapbTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~SwapbTest() {    
  }   

virtual void SetUp() {
 
  
}

virtual void TearDown() {


}

};

TEST_F(SwapbTest, normal) {

  // 0601157E: swapwin 6249, R[2]=06034CE8 R[4]=00120000
  // 0601157E: swapwout 6249, R[2]=00000012 R[4]=00120000

  MSH2->regs.R[2]=0x06034CE8;
  MSH2->regs.R[4]=0x00120000;

  // swap.w r2,r4
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x6248 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x00120000, MSH2->regs.R[2] );
  EXPECT_EQ( 0x00120000, MSH2->regs.R[4] );

}
TEST_F(SwapbTest, normal2) {

  MSH2->regs.R[2]=0x00000002;
  MSH2->regs.R[3]=0xAABBCCDD;

  // swap.w r2,r3
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x6238 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0xAABBDDCC, MSH2->regs.R[2] );
  EXPECT_EQ( 0xAABBCCDD, MSH2->regs.R[3] );

}

TEST_F(SwapbTest, sonicr) {


  MSH2->regs.R[13]=0x00000000;
  MSH2->regs.R[0]=0x00000001;

  // swap.w r2,r3
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x6d08 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x00000100, MSH2->regs.R[13] );
  EXPECT_EQ( 0x00000001, MSH2->regs.R[0] );

}

}  // namespace
