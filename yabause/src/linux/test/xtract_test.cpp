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

class XtractTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  XtractTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~XtractTest() {    
  }   

virtual void SetUp() {
  printf("XtractTest::SetUp\n");
  
}

virtual void TearDown() {
  printf("XtractTest::TearDown\n");

}

};

TEST_F(XtractTest, normal) {

  MSH2->regs.R[0]=0x00000000;
  MSH2->regs.R[1]=0x00000001;

  // xtract r1,r0
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x201D );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x00010000, MSH2->regs.R[0] );

}

TEST_F(XtractTest, normal2) {

  MSH2->regs.R[3]=0x00000003;
  MSH2->regs.R[0]=0x6631C000;

  // xtract r3,r0
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x203D );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x00036631, MSH2->regs.R[0] );

}



}  // namespace
