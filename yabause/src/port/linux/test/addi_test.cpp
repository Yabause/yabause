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

class AddiTest : public ::testing::Test {
 protected:
  SH2Interface_struct* pctx_;

  AddiTest() {
    initEmulation();
    pctx_ = SH2Core;
  }


  virtual ~AddiTest() {   
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(AddiTest, normal) {

  MSH2->regs.R[3]=0x60000000; //dest

  // subc r1,r2
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x7310 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC = ( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x60000010, MSH2->regs.R[3] );
}

TEST_F(AddiTest, normal_T1) {

   MSH2->regs.R[3]=0x60000000;
   MSH2->regs.SR.all = (0x00000E0);

   // subc r1,r2
   SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x73ff );
   SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
   SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

   MSH2->regs.PC = ( 0x06000000 );
   SH2TestExec(MSH2, 1);

   EXPECT_EQ( 0x5FFFFFFF, MSH2->regs.R[3] );
}

TEST_F(AddiTest, normal_T21) {

   MSH2->regs.R[3]=0xFFFFFFFF;

   // subc r1,r2
   SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x73FF );
   SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
   SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

   MSH2->regs.PC = ( 0x06000000 );
   SH2TestExec(MSH2, 1);

   EXPECT_EQ( 0xFFFFFFFE, MSH2->regs.R[3] );
}
}  // namespacegPtr
