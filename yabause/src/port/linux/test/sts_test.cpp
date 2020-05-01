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

class StsTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  StsTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~StsTest() {   
  }   

virtual void SetUp() {
 
  
}

virtual void TearDown() {


}

};

TEST_F(StsTest, normal) {


  MSH2->regs.R[3]=0x06000264;
  MSH2->regs.PR =(0xCAFECAFE);


  // sts.l
  SH2MappedMemoryWriteWord(MSH2, 0x0600024c, 0x4322 );
  SH2MappedMemoryWriteWord(MSH2, 0x0600024e, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000250, 0x0009 );  // nop
  SH2MappedMemoryWriteLong(MSH2, 0x06000260, 0xDEADDEAD );  // nop

  MSH2->regs.PC =( 0x0600024c );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x06000260, MSH2->regs.R[3] );
  EXPECT_EQ( 0xCAFECAFE, SH2MappedMemoryReadLong(MSH2,0x06000260) );

}


TEST_F(StsTest, stsmach) {

  MSH2->regs.R[3]=0x06000264;
  MSH2->regs.MACH =(0xCAFECAFE);

  SH2MappedMemoryWriteWord(MSH2, 0x0600024c, 0x030A );
  SH2MappedMemoryWriteWord(MSH2, 0x0600024e, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000250, 0x0009 );  // nop
  SH2MappedMemoryWriteLong(MSH2, 0x06000260, 0xDEADDEAD );  // nop

  MSH2->regs.PC =( 0x0600024c );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0xCAFECAFE, MSH2->regs.R[3] );

}

TEST_F(StsTest, stsmmach) {

  MSH2->regs.R[3]=0x06000264;
  MSH2->regs.MACH =(0xCAFECAFE);

  SH2MappedMemoryWriteWord(MSH2, 0x0600024c, 0x4302 );
  SH2MappedMemoryWriteWord(MSH2, 0x0600024e, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000250, 0x0009 );  // nop
  SH2MappedMemoryWriteLong(MSH2, 0x06000260, 0xDEADDEAD );  // nop

  MSH2->regs.PC =( 0x0600024c );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x06000260, MSH2->regs.R[3] );
  EXPECT_EQ( 0xCAFECAFE, SH2MappedMemoryReadLong(MSH2,0x06000260) );

}

TEST_F(StsTest, stsmacl) {

  MSH2->regs.R[3]=0x06000264;
  MSH2->regs.MACL =(0xCAFECAFE);

  SH2MappedMemoryWriteWord(MSH2, 0x0600024c, 0x031A );
  SH2MappedMemoryWriteWord(MSH2, 0x0600024e, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000250, 0x0009 );  // nop
  SH2MappedMemoryWriteLong(MSH2, 0x06000260, 0xDEADDEAD );  // nop

  MSH2->regs.PC =( 0x0600024c );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0xCAFECAFE, MSH2->regs.R[3] );

}

TEST_F(StsTest, stsmmacl) {

  MSH2->regs.R[3]=0x06000264;
  MSH2->regs.MACL =(0xCAFECAFE);

  SH2MappedMemoryWriteWord(MSH2, 0x0600024c, 0x4312 );
  SH2MappedMemoryWriteWord(MSH2, 0x0600024e, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000250, 0x0009 );  // nop
  SH2MappedMemoryWriteLong(MSH2, 0x06000260, 0xDEADDEAD );  // nop

  MSH2->regs.PC =( 0x0600024c );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x06000260, MSH2->regs.R[3] );
  EXPECT_EQ( 0xCAFECAFE, SH2MappedMemoryReadLong(MSH2,0x06000260) );

}

TEST_F(StsTest, stspr) {

  MSH2->regs.R[3]=0x06000264;
  MSH2->regs.PR =(0xCAFECAFE);

  SH2MappedMemoryWriteWord(MSH2, 0x0600024c, 0x032A );
  SH2MappedMemoryWriteWord(MSH2, 0x0600024e, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000250, 0x0009 );  // nop
  SH2MappedMemoryWriteLong(MSH2, 0x06000260, 0xDEADDEAD );  // nop

  MSH2->regs.PC =( 0x0600024c );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0xCAFECAFE, MSH2->regs.R[3] );

}

TEST_F(StsTest, stsmpr) {

  MSH2->regs.R[3]=0x06000264;
  MSH2->regs.PR =(0xCAFECAFE);

  SH2MappedMemoryWriteWord(MSH2, 0x0600024c, 0x4322 );
  SH2MappedMemoryWriteWord(MSH2, 0x0600024e, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000250, 0x0009 );  // nop
  SH2MappedMemoryWriteLong(MSH2, 0x06000260, 0xDEADDEAD );  // nop

  MSH2->regs.PC =( 0x0600024c );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x06000260, MSH2->regs.R[3] );
  EXPECT_EQ( 0xCAFECAFE, SH2MappedMemoryReadLong(MSH2,0x06000260) );

}

}  // namespace
