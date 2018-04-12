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

class Div1Test : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  Div1Test() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~Div1Test() {   
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(Div1Test, normal) {

  MSH2->regs.R[0]=0x0000000E; // m
  MSH2->regs.R[3]=0xFFFFFFFF; // n
  MSH2->regs.SR.all = (0x0000101);

  // subc r1,r2
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x3304 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC = ( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x0000000E, MSH2->regs.R[0] );
  EXPECT_EQ( 0x0000000D, MSH2->regs.R[3] );
  EXPECT_EQ( 0x00000001, MSH2->regs.SR.all );
}

//060133C8: DIV1(i) 3304 R[0]:000000D9,R[3]:FFFFFFD1,SR:00000100
//060133C8: DIV1(O) m:000000D9,n:0000007B,SR:00000001

TEST_F(Div1Test, normal02) {

  MSH2->regs.R[0]=0x000000D9; // m
  MSH2->regs.R[3]=0xFFFFFFD1; // n
  MSH2->regs.SR.all = (0x0000100);

  // subc r1,r2
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x3304 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC = ( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x000000D9, MSH2->regs.R[0] );
  EXPECT_EQ( 0x0000007B, MSH2->regs.R[3] );
  EXPECT_EQ( 0x00000001, MSH2->regs.SR.all );
}

//06013340: DIV1i 3304 R[0]:00000008,R[3]:FFFFFFF8,SR:000001E0
//06013340: DIV1o m:00000008,n:FFFFFFF8,SR:000001E0

TEST_F(Div1Test, normal03) {

  MSH2->regs.R[0]=0x00000008; // m
  MSH2->regs.R[3]=0xFFFFFFF8; // n
  MSH2->regs.SR.all = (0x000001E0);

  // subc r1,r2
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x3304 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC = ( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x00000008, MSH2->regs.R[0] );
  EXPECT_EQ( 0xFFFFFFF8, MSH2->regs.R[3] );
  EXPECT_EQ( 0x000001E0, MSH2->regs.SR.all );
}

}  // namespacegPtr
