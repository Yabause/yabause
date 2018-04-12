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

class JmpTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  JmpTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~JmpTest() {
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(JmpTest, jmp) {

  MSH2->regs.R[1]=0x03216721;

  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x412B );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x0009 );

  MSH2->regs.PC = ( 0x06000000 );
  MSH2->regs.SR.all = ( 0x00000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x03216721, MSH2->regs.R[1] );
  EXPECT_EQ( 0x03216721, MSH2->regs.PC );
}

}  // namespacegPtr
