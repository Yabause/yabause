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

class MovtTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  MovtTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~MovtTest() {  
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};


TEST_F(MovtTest, normal) {

  MSH2->regs.R[0]=0xFFFFFFFF; // m
  MSH2->regs.SR.all =(0x00000E1);

  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x0029 );  // cmppl R[1]
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x00000001, MSH2->regs.R[0] );
}

TEST_F(MovtTest, Zero) {
  MSH2->regs.R[0]=0xFFFFFFFF; // m
  MSH2->regs.SR.all =(0x00000E0);

  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x0029 );  // cmppl R[1]
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x00000000, MSH2->regs.R[0] );

}


}  // namespacegPtr
