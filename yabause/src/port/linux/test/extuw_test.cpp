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

class ExtuwTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  ExtuwTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~ExtuwTest() {    
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(ExtuwTest, normal) {

  MSH2->regs.R[0]=0xFFFFFFFF; //source

  // subc r1,r2
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x600d );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC = ( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x0000FFFF, MSH2->regs.R[0] );
}

TEST_F(ExtuwTest, normal_T1) {

  MSH2->regs.R[0]=0x00000080; //source

  // subc r1,r2
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x600d );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC = ( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x00000080, MSH2->regs.R[0] );
}

}  // namespacegPtr
