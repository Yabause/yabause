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

class AddvTest : public ::testing::Test {
 protected:
    SH2Interface_struct* pctx_;

  AddvTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~AddvTest() { 
  }

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(AddvTest, normal) {

  MSH2->regs.R[2]=0x00000001; //source
  MSH2->regs.R[3]=0x00000001; //dest
  MSH2->regs.SR.all = (0x0000000);

  // addv r3,r2
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x332f );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC = ( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x00000001, MSH2->regs.R[2] );
  EXPECT_EQ( 0x00000002, MSH2->regs.R[3] );
  EXPECT_EQ( 0x00000000, MSH2->regs.SR.all );
}

TEST_F(AddvTest, normal_T1) {

   MSH2->regs.R[2]=0xFFFFFFFF;
   MSH2->regs.R[3]=0x00000001;
   MSH2->regs.SR.all = (0x0000000);

   // addv r1,r2
   SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x332f );
   SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
   SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

   MSH2->regs.PC = ( 0x06000000 );
   SH2TestExec(MSH2, 1);

   EXPECT_EQ( 0xffffffff, MSH2->regs.R[2] );
   EXPECT_EQ( 0x00000000, MSH2->regs.R[3] );
   EXPECT_EQ( 0x00000000, MSH2->regs.SR.all );
}

TEST_F(AddvTest, normal_T21) {

   MSH2->regs.R[2]=0x7FFFFFFF;
   MSH2->regs.R[3]=0x00000002;
   MSH2->regs.SR.all = (0x0000000);

   // subc r1,r2
   SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x332f );
   SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
   SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

   MSH2->regs.PC = ( 0x06000000 );
   SH2TestExec(MSH2, 1);

   EXPECT_EQ( 0x7fffffff, MSH2->regs.R[2] );
   EXPECT_EQ( 0x80000001, MSH2->regs.R[3] );
   EXPECT_EQ( 0x00000001, MSH2->regs.SR.all );
}

TEST_F(AddvTest, normal_T31) {

   MSH2->regs.R[2]=0x80000000;
   MSH2->regs.R[3]=0xFFFFFFFE;
   MSH2->regs.SR.all = (0x0000000);

   // subc r1,r2
   SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x332f );
   SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
   SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

   MSH2->regs.PC = ( 0x06000000 );
   SH2TestExec(MSH2, 1);

   EXPECT_EQ( 0x80000000, MSH2->regs.R[2] );
   EXPECT_EQ( 0x7ffffffe, MSH2->regs.R[3] );
   EXPECT_EQ( 0x00000001, MSH2->regs.SR.all );
}

}  // namespacegPtr
