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

class CmpStrTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  CmpStrTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~CmpStrTest() {  
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};


TEST_F(CmpStrTest, normal) {

  MSH2->regs.R[4]=0x2e00ffff; // m
  MSH2->regs.R[6]=0x00000000;
  MSH2->regs.SR.all = (0x00000E0);

  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x264c );  // cmpstr R[4] R[6]
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC = ( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x000000E1, MSH2->regs.SR.all );
}

TEST_F(CmpStrTest, equal) {
  MSH2->regs.R[4]=0x2e00ffff; // m
  MSH2->regs.R[6]=0xffff1111; // n
  MSH2->regs.SR.all = (0x00000E1);

  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x264c );  // cmpstr R[4] R[6]
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC = ( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x000000E0, MSH2->regs.SR.all );

}



}  // namespacegPtr
