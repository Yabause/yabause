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

class ExtsbTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  ExtsbTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~ExtsbTest() {  
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(ExtsbTest, normal) {

  MSH2->regs.R[2]=0x00000020; //source
  MSH2->regs.R[1]=0x00FF0036; //source

  // subc r1,r2
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x621e );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC = ( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x00000036, MSH2->regs.R[2] );
  EXPECT_EQ( 0x00FF0036, MSH2->regs.R[1] );
}

TEST_F(ExtsbTest, normal_T1) {

  MSH2->regs.R[0]=0x00000080; //source

  // subc r1,r2
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x600e );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC = ( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0xffffff80, MSH2->regs.R[0] );
}

}  // namespacegPtr
