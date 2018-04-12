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

class MuluTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  MuluTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~MuluTest() {    
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(MuluTest, normal) {

  MSH2->regs.R[3]=0x0000000f;
  MSH2->regs.R[2]=0x00000018;

  // mulu r9, r3
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x232e );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x0168, MSH2->regs.MACL );

}


TEST_F(MuluTest, middle) {

  MSH2->regs.R[3]=0x00008000;
  MSH2->regs.R[2]=0x00008000;

  // mulu r9, r3
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x232e );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x40000000, MSH2->regs.MACL );

}

TEST_F(MuluTest, max) {

  MSH2->regs.R[3]=0x0000FFFF;
  MSH2->regs.R[2]=0x0000FFFF;

  // mulu r9, r3
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x232e );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0xfffe0001, MSH2->regs.MACL );

}

TEST_F(MuluTest, ex) {

  MSH2->regs.R[3]=0xFFFFFFFF;
  MSH2->regs.R[2]=0xFFFFFFFF;

  // mulu r9, r3
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x232e );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0xfffe0001, MSH2->regs.MACL );

}


}  // namespace
