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

class CmpgeTest : public ::testing::Test {
 protected:
  SH2Interface_struct* pctx_;

  CmpgeTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~CmpgeTest() {   
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};


TEST_F(CmpgeTest, normal) {

  MSH2->regs.R[1]=0x06020000; // m
  MSH2->regs.R[2]=0x06010000;
  MSH2->regs.SR.all = (0x00000E0);

  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x3123 );  // cmphi 
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC = ( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x000000E1, MSH2->regs.SR.all );
}

TEST_F(CmpgeTest, Zero) {

  MSH2->regs.R[1]=0x06010000; // m
  MSH2->regs.R[2]=0x06020000;
  MSH2->regs.SR.all = (0x00000E1);

  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x3123 );  // cmphi 
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC = ( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x000000E0, MSH2->regs.SR.all );
}

TEST_F(CmpgeTest, min) {
  MSH2->regs.R[2]=0xFFFFFFFF; // m
  MSH2->regs.R[1]=0x00000000;
  MSH2->regs.SR.all = (0x00000E0);

  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x3123 );  // cmphi 
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC = ( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x000000E1, MSH2->regs.SR.all );

}

TEST_F(CmpgeTest, same) {

  MSH2->regs.R[1]=0x06010000; // m
  MSH2->regs.R[2]=0x06010000;
  MSH2->regs.SR.all = (0x00000E0);

  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x3123 );  // cmphi 
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC = ( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x000000E1, MSH2->regs.SR.all );

}


}  // namespacegPtr
