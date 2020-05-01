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

class ShalTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  ShalTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~ShalTest() {   
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};


TEST_F(ShalTest, normal) {

  MSH2->regs.R[3]=0xC; // m
  MSH2->regs.SR.all =(0x00000F1);

  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x4320 );  // shal
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x18, MSH2->regs.R[3] );
  EXPECT_EQ( 0x000000F0, MSH2->regs.SR.all );
}

TEST_F(ShalTest, normal01) {

  MSH2->regs.R[3]=0x8000000C; // m
  MSH2->regs.SR.all =(0x00000F0);

  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x4320 );  // shal
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x18, MSH2->regs.R[3] );
  EXPECT_EQ( 0x000000F1, MSH2->regs.SR.all );
}


}  // namespacegPtr
