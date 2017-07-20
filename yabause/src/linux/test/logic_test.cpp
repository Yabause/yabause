#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
extern "C" {
extern void initEmulation();
}
namespace {

class LogicTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  LogicTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~LogicTest() { 
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(LogicTest, and) {

  MSH2->regs.R[2]=0x00FF00FF; //source
  MSH2->regs.R[3]=0x007007e0; //dest

  MappedMemoryWriteWord( 0x06000000, 0x2329 );
  MappedMemoryWriteWord( 0x06000002, 0x000b );  // rts
  MappedMemoryWriteWord( 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2Exec(MSH2, 1);

  EXPECT_EQ( 0x00FF00FF, MSH2->regs.R[2] );
  EXPECT_EQ( 0x007000e0, MSH2->regs.R[3] );
}

TEST_F(LogicTest, andi) {

  MSH2->regs.R[0]=0x00FF00F8; //source

  MappedMemoryWriteWord( 0x06000000, 0xC929 );
  MappedMemoryWriteWord( 0x06000002, 0x000b );  // rts
  MappedMemoryWriteWord( 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2Exec(MSH2, 1);

  EXPECT_EQ( 0x28, MSH2->regs.R[0] );
}

TEST_F(LogicTest, andb) {

  MSH2->regs.R[0]=0x000000250; //source
  MSH2->regs.GBR =(0x06000000); //source

  MappedMemoryWriteWord( 0x06000000, 0xCD29 );
  MappedMemoryWriteWord( 0x06000002, 0x000b );  // rts
  MappedMemoryWriteWord( 0x06000004, 0x0009 );  // nop
  MappedMemoryWriteLong( 0x06000250, 0xDEADCAFE );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2Exec(MSH2, 1);

  EXPECT_EQ( 0x08ADCAFE, MappedMemoryReadLong(0x06000250) );
}

TEST_F(LogicTest, not) {

  MSH2->regs.R[0]=0xF0F0F0F0; //source

  MappedMemoryWriteWord( 0x06000000, 0x6107 );
  MappedMemoryWriteWord( 0x06000002, 0x000b );  // rts
  MappedMemoryWriteWord( 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2Exec(MSH2, 1);

  EXPECT_EQ( 0xF0F0F0F0, MSH2->regs.R[0] );
  EXPECT_EQ( 0x0F0F0F0F, MSH2->regs.R[1] );
}

TEST_F(LogicTest, or) {
  MSH2->regs.R[0]=0xF0F0F2F0; //source
  MSH2->regs.R[1]=0xF7F4F0F0; //source

  MappedMemoryWriteWord( 0x06000000, 0x210B );
  MappedMemoryWriteWord( 0x06000002, 0x000b );  // rts
  MappedMemoryWriteWord( 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2Exec(MSH2, 1);

  EXPECT_EQ( 0xF0F0F2F0, MSH2->regs.R[0] );
  EXPECT_EQ( 0xF7F4F2F0, MSH2->regs.R[1] );
}

TEST_F(LogicTest, ori) {
  MSH2->regs.R[0]=0xF0F0F270; //source

  MappedMemoryWriteWord( 0x06000000, 0xCB1B );
  MappedMemoryWriteWord( 0x06000002, 0x000b );  // rts
  MappedMemoryWriteWord( 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2Exec(MSH2, 1);

  EXPECT_EQ( 0xF0F0F27B, MSH2->regs.R[0] );
}

TEST_F(LogicTest, origbr) {
  MSH2->regs.R[0]=0x00000250; //source
  MSH2->regs.GBR =(0x06000000); //source

  MappedMemoryWriteWord( 0x06000000, 0xCF1B );
  MappedMemoryWriteWord( 0x06000002, 0x000b );  // rts
  MappedMemoryWriteWord( 0x06000004, 0x0009 );  // nop
  MappedMemoryWriteLong( 0x06000250, 0xDEADCAFE );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2Exec(MSH2, 1);

  EXPECT_EQ( 0xDFADCAFE, MappedMemoryReadLong(0x06000250) );
}

TEST_F(LogicTest, tasb) {
  MSH2->regs.R[1]=0x06000250; //source
  MSH2->regs.SR.all = (0xE0); //source

  MappedMemoryWriteWord( 0x06000000, 0x411B );
  MappedMemoryWriteWord( 0x06000002, 0x000b );  // rts
  MappedMemoryWriteWord( 0x06000004, 0x0009 );  // nop
  MappedMemoryWriteLong( 0x06000250, 0x00ADDEAD );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2Exec(MSH2, 1);

  EXPECT_EQ( 0x80ADDEAD, MappedMemoryReadLong(0x06000250) );
  EXPECT_EQ( 0xE1, MSH2->regs.SR.all );

  MSH2->regs.R[1]=0x06000250; //source
  MSH2->regs.SR.all = (0xE1); //source

  MappedMemoryWriteWord( 0x06000000, 0x411B );
  MappedMemoryWriteWord( 0x06000002, 0x000b );  // rts
  MappedMemoryWriteWord( 0x06000004, 0x0009 );  // nop
  MappedMemoryWriteLong( 0x06000250, 0x07ADDEAD );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2Exec(MSH2, 1);

  EXPECT_EQ( 0x87ADDEAD, MappedMemoryReadLong(0x06000250) );
  EXPECT_EQ( 0xE0, MSH2->regs.SR.all );
}

TEST_F(LogicTest, tst) {
  MSH2->regs.R[0]=0x00000250; //source
  MSH2->regs.R[1]=0x00FF0400; //source
  MSH2->regs.SR.all = (0xE0);

  MappedMemoryWriteWord( 0x06000000, 0x2108 );
  MappedMemoryWriteWord( 0x06000002, 0x000b );  // rts
  MappedMemoryWriteWord( 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2Exec(MSH2, 1);

  EXPECT_EQ( 0xE1, MSH2->regs.SR.all );

  MSH2->regs.R[0]=0x00000250; //source
  MSH2->regs.R[1]=0x00FF0600; //source
  MSH2->regs.SR.all = (0xE1);

  MappedMemoryWriteWord( 0x06000000, 0x2108 );
  MappedMemoryWriteWord( 0x06000002, 0x000b );  // rts
  MappedMemoryWriteWord( 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2Exec(MSH2, 1);

  EXPECT_EQ( 0xE0, MSH2->regs.SR.all );
}

TEST_F(LogicTest, tsti) {
  MSH2->regs.R[0]=0x00FF0442; //source
  MSH2->regs.SR.all = (0xE0);

  MappedMemoryWriteWord( 0x06000000, 0xC825 );
  MappedMemoryWriteWord( 0x06000002, 0x000b );  // rts
  MappedMemoryWriteWord( 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2Exec(MSH2, 1);

  EXPECT_EQ( 0xE1, MSH2->regs.SR.all );

  MSH2->regs.R[0]=0x00FF0442; //source
  MSH2->regs.SR.all = (0xE1);

  MappedMemoryWriteWord( 0x06000000, 0xC822 );
  MappedMemoryWriteWord( 0x06000002, 0x000b );  // rts
  MappedMemoryWriteWord( 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2Exec(MSH2, 1);

  EXPECT_EQ( 0xE0, MSH2->regs.SR.all );
}

TEST_F(LogicTest, tstb) {
  MSH2->regs.R[0]=0x00000250; //source
  MSH2->regs.GBR =(0x06000000); //source
  MSH2->regs.SR.all = (0xE0);

  MappedMemoryWriteWord( 0x06000000, 0xCC25 );
  MappedMemoryWriteWord( 0x06000002, 0x000b );  // rts
  MappedMemoryWriteWord( 0x06000004, 0x0009 );  // nop
  MappedMemoryWriteLong( 0x06000250, 0x42ADDEAD );

  MSH2->regs.PC =( 0x06000000 );
  SH2Exec(MSH2, 1);

  EXPECT_EQ( 0xE1, MSH2->regs.SR.all );
  EXPECT_EQ( 0x42ADDEAD, MappedMemoryReadLong(0x06000250) );

  MSH2->regs.R[0]=0x00000250; //source
  MSH2->regs.GBR =(0x06000000); //source
  MSH2->regs.SR.all = (0xE1);

  MappedMemoryWriteWord( 0x06000000, 0xCC25 );
  MappedMemoryWriteWord( 0x06000002, 0x000b );  // rts
  MappedMemoryWriteWord( 0x06000004, 0x0009 );  // nop
  MappedMemoryWriteLong( 0x06000250, 0x62ADDEAD );

  MSH2->regs.PC =( 0x06000000 );
  SH2Exec(MSH2, 1);

  EXPECT_EQ( 0xE0, MSH2->regs.SR.all );
  EXPECT_EQ( 0x62ADDEAD, MappedMemoryReadLong(0x06000250) );
}

TEST_F(LogicTest, xor) {
  MSH2->regs.R[0]=0xF0F0F2F0; //source
  MSH2->regs.R[1]=0xF7F4F0F0; //source

  MappedMemoryWriteWord( 0x06000000, 0x210A );
  MappedMemoryWriteWord( 0x06000002, 0x000b );  // rts
  MappedMemoryWriteWord( 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2Exec(MSH2, 1);

  EXPECT_EQ( 0xF0F0F2F0, MSH2->regs.R[0] );
  EXPECT_EQ( 0x07040200, MSH2->regs.R[1] );
}

TEST_F(LogicTest, xori) {
  MSH2->regs.R[0]=0xF0F0F2F0; //source

  MappedMemoryWriteWord( 0x06000000, 0xCA1B );
  MappedMemoryWriteWord( 0x06000002, 0x000b );  // rts
  MappedMemoryWriteWord( 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2Exec(MSH2, 1);

  EXPECT_EQ( 0xF0F0F2EB, MSH2->regs.R[0] );

  MSH2->regs.R[0]=0xF0F0F2F0; //source

  MappedMemoryWriteWord( 0x06000000, 0xCA9B );
  MappedMemoryWriteWord( 0x06000002, 0x000b );  // rts
  MappedMemoryWriteWord( 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2Exec(MSH2, 1);

  EXPECT_EQ( 0xF0F0F26B, MSH2->regs.R[0] );
}

TEST_F(LogicTest, xorigbr) {
  MSH2->regs.R[0]=0x00000250; //source
  MSH2->regs.GBR =(0x06000000); //source

  MappedMemoryWriteWord( 0x06000000, 0xCE1B );
  MappedMemoryWriteWord( 0x06000002, 0x000b );  // rts
  MappedMemoryWriteWord( 0x06000004, 0x0009 );  // nop
  MappedMemoryWriteLong( 0x06000250, 0xF0F0F2F0 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2Exec(MSH2, 1);

  EXPECT_EQ( 0xEBF0F2F0, MappedMemoryReadLong(0x06000250) );
}

}  // namespacegPtr
