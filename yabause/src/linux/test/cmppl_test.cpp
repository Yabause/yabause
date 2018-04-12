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

class SmpplTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  SmpplTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~SmpplTest() {   
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};


TEST_F(SmpplTest, normal) {

  MSH2->regs.R[1]=0x0001; // m
  MSH2->regs.SR.all = (0x00000E0);

  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x4115 );  // cmppl R[1]
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC = ( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x000000E1, MSH2->regs.SR.all );
}

TEST_F(SmpplTest, Zero) {

  MSH2->regs.R[1]=0x0; // m
  MSH2->regs.SR.all = (0x00000E0);

  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x4115 );  // shar
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC = ( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x000000E0, MSH2->regs.SR.all );
}

TEST_F(SmpplTest, min) {

  MSH2->regs.R[1]=0xFFFFFFFE; // m
  MSH2->regs.SR.all = (0x00000E0);

  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x4115 );  // shar
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC = ( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x000000E0, MSH2->regs.SR.all );
}


}  // namespacegPtr
