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

class AddcTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;


  AddcTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~AddcTest() { 
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(AddcTest, normal) {
  for (int i = 0; i<16; i++)
    MSH2->regs.R[i] = 0xFFFFFFFF;

  MSH2->regs.R[2]=0x00000000; //source
  MSH2->regs.R[3]=0x000000e0; //dest
  MSH2->regs.SR.all = 0x00000E0;

  // subc r1,r2
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x332e );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC = 0x06000000;
  SH2TestExec(MSH2, 1);

  for (int i = 0; i<2; i++)
    EXPECT_EQ( 0xFFFFFFFF, MSH2->regs.R[i] );
  EXPECT_EQ( 0x00000000, MSH2->regs.R[2] );
  EXPECT_EQ( 0x000000E0, MSH2->regs.R[3] );
  for (int i = 4; i<16; i++)
    EXPECT_EQ( 0xFFFFFFFF, MSH2->regs.R[i] );
  EXPECT_EQ( 0x000000E0, MSH2->regs.SR.all );
}

TEST_F(AddcTest, normal_T1) {

  for (int i = 0; i<16; i++)
    MSH2->regs.R[i] = 0x00000000;

   MSH2->regs.R[2]=0xFFFFFFFF;
   MSH2->regs.R[3]=0x00000001;
   MSH2->regs.SR.all = 0x00000E0;

   // subc r1,r2
   SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x332e );
   SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
   SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

   MSH2->regs.PC = 0x06000000;
   pctx_->Exec(MSH2, 1);

  for (int i = 0; i<2; i++)
    EXPECT_EQ( 0x00000000, MSH2->regs.R[i] );
   EXPECT_EQ( 0xffffffff, MSH2->regs.R[2] );
   EXPECT_EQ( 0x00000000, MSH2->regs.R[3] );
  for (int i = 4; i<16; i++)
    EXPECT_EQ( 0x00000000, MSH2->regs.R[i] );
   EXPECT_EQ( 0x000000E1, MSH2->regs.SR.all );
}

TEST_F(AddcTest, normal_T21) {

  for (int i = 0; i<16; i++)
    MSH2->regs.R[i] = 0x00000000;

   MSH2->regs.R[2]=0xFFFFFFFF;
   MSH2->regs.R[3]=0x00000000;
   MSH2->regs.SR.all =0x00000E1;

   // subc r1,r2
   SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x332e );
   SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
   SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

   MSH2->regs.PC = 0x06000000;
   pctx_->Exec(MSH2, 1);

  for (int i = 0; i<2; i++)
    EXPECT_EQ( 0x00000000, MSH2->regs.R[i] );
   EXPECT_EQ( 0xffffffff, MSH2->regs.R[2] );
   EXPECT_EQ( 0x00000000, MSH2->regs.R[3] );
  for (int i = 4; i<16; i++)
    EXPECT_EQ( 0x00000000, MSH2->regs.R[i] );
   EXPECT_EQ( 0x000000E1, MSH2->regs.SR.all );
}
}  // namespacegPtr
