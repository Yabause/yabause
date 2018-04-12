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

class StcTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  StcTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~StcTest() {   
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(StcTest, stcsr) {

  MSH2->regs.R[1]=0xDEADCAFE;
  MSH2->regs.SR.all =(0x03216721);

  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x0102 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );
  SH2MappedMemoryWriteLong(MSH2, 0x06000250, 0x03216721 );

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x321, MSH2->regs.R[1] );
}

TEST_F(StcTest, stcmsr) {

  MSH2->regs.R[1]=0x06000250;
  MSH2->regs.SR.all =(0x3F3);

  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x4103 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );
  SH2MappedMemoryWriteLong(MSH2, 0x0600024C, 0x03216FFF );

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x0600024C, MSH2->regs.R[1] );
  EXPECT_EQ( 0x3F3, SH2MappedMemoryReadLong(MSH2, 0x0600024C ) );

}

TEST_F(StcTest, stcgbr) {

  MSH2->regs.R[1]=0x06000250;
  MSH2->regs.GBR =(0xDEADCAFE);

  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x0112 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );
  SH2MappedMemoryWriteLong(MSH2, 0x0600024C, 0x03216FFF );

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0xDEADCAFE, MSH2->regs.R[1] );

}

TEST_F(StcTest, stcmgbr) {

  MSH2->regs.R[1]=0x06000250;
  MSH2->regs.GBR =(0xDEADCAFE);

  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x4113 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );
  SH2MappedMemoryWriteLong(MSH2, 0x0600024C, 0x03216FFF );

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x0600024C, MSH2->regs.R[1] );
  EXPECT_EQ( 0xDEADCAFE, SH2MappedMemoryReadLong(MSH2, 0x0600024C ) );

}

TEST_F(StcTest, stcvbr) {

  MSH2->regs.R[1]=0x06000250;
  MSH2->regs.VBR =(0xDEADCAFE);

  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x0122 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );
  SH2MappedMemoryWriteLong(MSH2, 0x0600024C, 0x03216FFF );

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0xDEADCAFE, MSH2->regs.R[1] );

}

TEST_F(StcTest, stcmvbr) {

  MSH2->regs.R[1]=0x06000250;
  MSH2->regs.VBR =(0xDEADCAFE);

  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x4123 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );
  SH2MappedMemoryWriteLong(MSH2, 0x0600024C, 0x03216FFF );

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x0600024C, MSH2->regs.R[1] );
  EXPECT_EQ( 0xDEADCAFE, SH2MappedMemoryReadLong(MSH2, 0x0600024C ) );

}

}  // namespacegPtr
