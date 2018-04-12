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

class TrappaTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  TrappaTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~TrappaTest() {
  }   

virtual void SetUp() {
   
}

virtual void TearDown() {
}

};

TEST_F(TrappaTest, normal) {

  MSH2->regs.R[15]=0x06100000;
  MSH2->regs.SR.all =(0x000000f0);
  MSH2->regs.PC =(0x06014026);
  MSH2->regs.VBR =(0x06000000);
 
  SH2MappedMemoryWriteLong(MSH2, 0x060000C0, 0x060041d8 );

  // trapa
  SH2MappedMemoryWriteWord(MSH2, 0x06014026, 0xc330 );
  
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x060041d8, MSH2->regs.PC );
  EXPECT_EQ( MSH2->regs.SR.all, SH2MappedMemoryReadLong(MSH2,0x060ffffC) );
  EXPECT_EQ( 0x06014028, SH2MappedMemoryReadLong(MSH2,0x060ffff8) );
  EXPECT_EQ( 0x060ffff8, MSH2->regs.R[15] );

}





}  // namespace
