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

class LDC_GBR_INCTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  LDC_GBR_INCTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~LDC_GBR_INCTest() {  
  }   

virtual void SetUp() {
   
}

virtual void TearDown() {
}

};

TEST_F(LDC_GBR_INCTest, normal) {

  for (int i = 0; i<15; i++)
    MSH2->regs.R[i]=0xFFFFFFFF;


  MSH2->regs.R[15]=0x06001F3C;
  SH2MappedMemoryWriteLong(MSH2, 0x06001F3C, 0x25D00000 );
  MSH2->regs.SR.all = (0);

  // ldc.l @r15+, gbr
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x4F17 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000B );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC = ( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x25D00000, MSH2->regs.GBR );
  EXPECT_EQ( 0x06001F40, MSH2->regs.R[15] );
  for (int i = 0; i<15; i++)
    EXPECT_EQ( 0xFFFFFFFF, MSH2->regs.R[i] );
}

TEST_F(LDC_GBR_INCTest, normal_zero) {

  for (int i = 0; i<15; i++)
    MSH2->regs.R[i]=0x00000000;


  MSH2->regs.R[15]=0x06001F3C;
  SH2MappedMemoryWriteLong(MSH2, 0x06001F3C, 0x25D00000 );
  MSH2->regs.SR.all = (0);

  // ldc.l @r15+, gbr
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x4F17 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000B );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC = ( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x25D00000, MSH2->regs.GBR );
  EXPECT_EQ( 0x06001F40, MSH2->regs.R[15] );
  for (int i = 0; i<15; i++)
    EXPECT_EQ( 0x00000000, MSH2->regs.R[i] );
}





}  // namespace
