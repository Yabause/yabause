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

class SubcTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  SubcTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~SubcTest() {  
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(SubcTest, normal) {

  MSH2->regs.R[1]=0x00000007;
  MSH2->regs.R[2]=0x00000000;
  MSH2->regs.SR.all =(0x00000E0);

  // subc r1,r2
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x312A );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x00000007, MSH2->regs.R[1] );
  EXPECT_EQ( 0x00000000, MSH2->regs.R[2] );
  EXPECT_EQ( 0x000000E0, MSH2->regs.SR.all );
}

TEST_F(SubcTest, normal_T1) {

   MSH2->regs.R[1]=0x00000001;
   MSH2->regs.R[2]=0x00000002;
   MSH2->regs.SR.all =(0x00000E0);

   // subc r1,r2
   SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x312A );
   SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
   SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

   MSH2->regs.PC =( 0x06000000 );
   SH2TestExec(MSH2, 1);

   EXPECT_EQ( 0xffffffff, MSH2->regs.R[1] );
   EXPECT_EQ( 0x00000002, MSH2->regs.R[2] );
   EXPECT_EQ( 0x000000E1, MSH2->regs.SR.all );
}

TEST_F(SubcTest, normal_T21) {

   MSH2->regs.R[1]=0x00000001;
   MSH2->regs.R[2]=0x00000001;
   MSH2->regs.SR.all =(0x00000E1);

   // subc r1,r2
   SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x312A );
   SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
   SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

   MSH2->regs.PC =( 0x06000000 );
   SH2TestExec(MSH2, 1);

   EXPECT_EQ( 0xffffffff, MSH2->regs.R[1] );
   EXPECT_EQ( 0x00000001, MSH2->regs.R[2] );
   EXPECT_EQ( 0x000000E1, MSH2->regs.SR.all );
}

TEST_F(SubcTest, normal_T31) {

   MSH2->regs.R[1]=0x00000000;
   MSH2->regs.R[3]=0x00000001;
   MSH2->regs.SR.all =(0x00000E0);

   // subc r1,r2
   SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x313A );
   SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
   SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

   MSH2->regs.PC =( 0x06000000 );
   SH2TestExec(MSH2, 1);

   EXPECT_EQ( 0xffffffff, MSH2->regs.R[1] );
   EXPECT_EQ( 0x00000001, MSH2->regs.R[3] );
   EXPECT_EQ( 0x000000E1, MSH2->regs.SR.all );
}

TEST_F(SubcTest, normal_T32) {

   MSH2->regs.R[0]=0x00000000;
   MSH2->regs.R[2]=0x00000000;
   MSH2->regs.SR.all =(0x00000E1);

   // subc r1,r2
   SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x302A );
   SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
   SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

   MSH2->regs.PC =( 0x06000000 );
   SH2TestExec(MSH2, 1);

   EXPECT_EQ( 0xffffffff, MSH2->regs.R[0] );
   EXPECT_EQ( 0x00000000, MSH2->regs.R[2] );
   EXPECT_EQ( 0x000000E1, MSH2->regs.SR.all );
}

}  // namespacegPtr
