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

class RteTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  RteTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~RteTest() {    
  }   

virtual void SetUp() {
   
}

virtual void TearDown() {
}

};

TEST_F(RteTest, normal) {

  MSH2->regs.R[15]=0x06001F68;
  SH2MappedMemoryWriteLong(MSH2, 0x06001F68, 0x060107B6 );
  SH2MappedMemoryWriteLong(MSH2, 0x06001F6C, 0x0 );
  MSH2->regs.SR.all =(0);

  // rte
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x002b );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x060107B6, MSH2->regs.PC );
  EXPECT_EQ( 0x00000000, MSH2->regs.SR.all );
  EXPECT_EQ( 0x06001F70, MSH2->regs.R[15] );

}

TEST_F(RteTest, rts) {

  MSH2->regs.PR =(0x06001000);

  MSH2->regs.R[7]=0xDEADDEAD;

  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024c, 0x000b );
  SH2MappedMemoryWriteWord(MSH2, 0x0600024e, 0xE743 );  //MOVI

  MSH2->regs.PC =( 0x0600024c );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x00000043, MSH2->regs.R[7] );
  EXPECT_EQ( 0x06001000, MSH2->regs.PC );
}

}  // namespace
