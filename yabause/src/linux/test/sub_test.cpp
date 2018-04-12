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

class SubTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  SubTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~SubTest() {    
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(SubTest, normal) {

  MSH2->regs.R[1]=0x00FF00FF;
  MSH2->regs.R[2]=0x000000FF;

  // subc r1,r2
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x3128 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x00FF0000, MSH2->regs.R[1] );
  EXPECT_EQ( 0x000000FF, MSH2->regs.R[2] );
}

TEST_F(SubTest, negative) {

  MSH2->regs.R[1]=0x10;
  MSH2->regs.R[2]=0x00000100;

  // subc r1,r2
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x3128 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0xFFFFFF10, MSH2->regs.R[1] );
  EXPECT_EQ( 0x00000100, MSH2->regs.R[2] );
}

}  // namespacegPtr
