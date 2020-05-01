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

class Div0sTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  Div0sTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~Div0sTest() { 
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};


TEST_F(Div0sTest, normal) {

  MSH2->regs.R[2]=0x1; // m
  MSH2->regs.R[3]=0x2; // n
  MSH2->regs.SR.all = (0x0000000);

  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x2327 );  // div0s
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC = ( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x1, MSH2->regs.R[2] );
  EXPECT_EQ( 0x2, MSH2->regs.R[3] );
  EXPECT_EQ( 0x00000000, MSH2->regs.SR.all );
}


TEST_F(Div0sTest, normal_101) {

  MSH2->regs.R[2]=0x00000000; // m
  MSH2->regs.R[3]=0xFFFFFFCD; // n
  MSH2->regs.SR.all = (0x0000000);

  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x2327 );  // div0s
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC = ( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x00000000, MSH2->regs.R[2] );
  EXPECT_EQ( 0xFFFFFFCD, MSH2->regs.R[3] );
  EXPECT_EQ( 0x00000101, MSH2->regs.SR.all );
}

TEST_F(Div0sTest, normal_201 ) {

  MSH2->regs.R[2]=0xFFFFFFCD; // m
  MSH2->regs.R[3]=0x00000000; // n
  MSH2->regs.SR.all = (0x0000000);

  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x2327 );  // div0s
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC = ( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0xFFFFFFCD, MSH2->regs.R[2] );
  EXPECT_EQ( 0x00000000, MSH2->regs.R[3] );
  EXPECT_EQ( 0x00000201, MSH2->regs.SR.all );
}

TEST_F(Div0sTest, normal_300 ) {

  MSH2->regs.R[2]=0xFFFFFFCD; // m
  MSH2->regs.R[3]=0xFFFFFFCE; // n
  MSH2->regs.SR.all = (0x0000000);

  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x2327 );  // div0s
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC = ( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0xFFFFFFCD, MSH2->regs.R[2] );
  EXPECT_EQ( 0xFFFFFFCE, MSH2->regs.R[3] );
  EXPECT_EQ( 0x00000300, MSH2->regs.SR.all );
}

}  // namespacegPtr
