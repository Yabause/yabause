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

class DmuluTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  DmuluTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~DmuluTest() {   
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(DmuluTest, normal) {

  MSH2->regs.R[4]=0x00000002;
  MSH2->regs.R[7]=0xcccccccd;

  // dmulu r1, r3
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x3475 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC = ( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x9999999a, MSH2->regs.MACL  );
  EXPECT_EQ( 0x00000001, MSH2->regs.MACH );

}


}  // namespace
