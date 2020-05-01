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

class AddTest : public ::testing::Test {
 protected:
  SH2Interface_struct* pctx_;

  AddTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~AddTest() { 
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(AddTest, normal) {

  MSH2->regs.R[2]=0x00000000; //source
  MSH2->regs.R[3]=0x000000e0; //dest

  // add r1,r2
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x332C );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC = ( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x00000000, MSH2->regs.R[2] );
  EXPECT_EQ( 0x000000E0, MSH2->regs.R[3] );


  MSH2->regs.R[2]=0xDEADDEAD; //source
  MSH2->regs.R[3]=0x000000e0; //dest

  // add r1,r2
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x332C );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC = ( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0xDEADDEAD, MSH2->regs.R[2] );
  EXPECT_EQ( 0xDEADDF8D, MSH2->regs.R[3] );


  MSH2->regs.R[2]=0xDEADDEAD; //source
  MSH2->regs.R[3]=0xF00000e0; //dest

  // add r1,r2
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x332C );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC = ( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0xDEADDEAD, MSH2->regs.R[2] );
  EXPECT_EQ( 0xCEADDF8D, MSH2->regs.R[3] );
}

}  // namespacegPtr
