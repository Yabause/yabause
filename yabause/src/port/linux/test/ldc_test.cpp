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

class LdcTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  LdcTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~LdcTest() {
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(LdcTest, normal) {

  MSH2->regs.R[0]=0xFFFFFFFF; //source

  // subc r1,r2
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x400E );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC = ( 0x06000000 );
  MSH2->regs.SR.all = ( 0x00000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0xFFFFFFFF, MSH2->regs.R[0] );
  EXPECT_EQ( 0x3F3, MSH2->regs.SR.all );
}

TEST_F(LdcTest, ldcmsr) {

  MSH2->regs.R[1]=0x06000250; //source

  // subc r1,r2
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x4107 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop
  SH2MappedMemoryWriteLong(MSH2, 0x06000250, 0x03210321 );

  MSH2->regs.PC = ( 0x06000000 );
  MSH2->regs.SR.all = ( 0x00000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x06000254, MSH2->regs.R[1] );
  EXPECT_EQ( 0x321, MSH2->regs.SR.all );
}

TEST_F(LdcTest, ldcgbr) {

  MSH2->regs.R[1]=0x06000250; //source

  // subc r1,r2
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x411E );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC = ( 0x06000000 );
  MSH2->regs.SR.all = ( 0x00000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x06000250, MSH2->regs.R[1] );
  EXPECT_EQ( 0x06000250, MSH2->regs.GBR );
}

TEST_F(LdcTest, ldcmgbr) {

  MSH2->regs.R[1]=0x06000250; //source

  // subc r1,r2
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x4117 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop
  SH2MappedMemoryWriteLong(MSH2, 0x06000250, 0x03210321 );

  MSH2->regs.PC = ( 0x06000000 );
  MSH2->regs.SR.all = ( 0x00000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x06000254, MSH2->regs.R[1] );
  EXPECT_EQ( 0x03210321, MSH2->regs.GBR );
}

TEST_F(LdcTest, ldcvbr) {

  MSH2->regs.R[1]=0x06000250; //source

  // subc r1,r2
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x412E );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop
  SH2MappedMemoryWriteLong(MSH2, 0x06000250, 0x03210321 );

  MSH2->regs.PC = ( 0x06000000 );
  MSH2->regs.SR.all = ( 0x00000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x06000250, MSH2->regs.R[1] );
  EXPECT_EQ( 0x06000250, MSH2->regs.VBR );
}

TEST_F(LdcTest, ldcmvbr) {

  MSH2->regs.R[1]=0x06000250; //source

  // subc r1,r2
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x4127 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop
  SH2MappedMemoryWriteLong(MSH2, 0x06000250, 0x03210321 );

  MSH2->regs.PC = ( 0x06000000 );
  MSH2->regs.SR.all = ( 0x00000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x06000254, MSH2->regs.R[1] );
  EXPECT_EQ( 0x03210321, MSH2->regs.VBR );
}

}  // namespacegPtr
