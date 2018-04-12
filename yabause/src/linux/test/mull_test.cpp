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

class MullTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  MullTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~MullTest() {   
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(MullTest, normal) {

  MSH2->regs.R[1]=0x000000A;
  MSH2->regs.R[2]=0x001AF000;
  MSH2->regs.MACL =(0xFFFFFFFF);
  MSH2->regs.MACH =(0xFFFFFFFF);

  // macl r1, r3
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x0127 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x10D6000, MSH2->regs.MACL );
  EXPECT_EQ( 0xFFFFFFFF, MSH2->regs.MACH );
}




}  // namespace
