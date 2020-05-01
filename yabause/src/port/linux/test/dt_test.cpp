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

class DtTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  DtTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~DtTest() { 
  }   

virtual void SetUp() {
 
  
}

virtual void TearDown() {


}

};

TEST_F(DtTest, normal) {

  MSH2->regs.R[2]=0x2;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x4210 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x0600024C, 0xDEADCAFE ); 

  MSH2->regs.PC = ( 0x06000246 );
  MSH2->regs.SR.all = ( 0x0 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0x1, MSH2->regs.R[2] );
 EXPECT_EQ( 0x0, MSH2->regs.SR.all );

  MSH2->regs.R[2]=0x2;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x4210 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x0600024C, 0xDEADCAFE ); 

  MSH2->regs.PC = ( 0x06000246 );
  MSH2->regs.SR.all = ( 0x1 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0x1, MSH2->regs.R[2] );
 EXPECT_EQ( 0x0, MSH2->regs.SR.all );

  MSH2->regs.R[2]=0x1;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x4210 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x0600024C, 0xDEADCAFE ); 

  MSH2->regs.PC = ( 0x06000246 );
  MSH2->regs.SR.all = ( 0x0 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0x0, MSH2->regs.R[2] );
 EXPECT_EQ( 0x1, MSH2->regs.SR.all );

}

TEST_F(DtTest, negative) {
  MSH2->regs.R[2]=-256;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x4210 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x0600024C, 0xDEADCAFE ); 

  MSH2->regs.PC = ( 0x06000246 );
  MSH2->regs.SR.all = ( 0x0 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( -257, MSH2->regs.R[2] );
 EXPECT_EQ( 0x0, MSH2->regs.SR.all );
}

TEST_F(DtTest, overflow) {
  MSH2->regs.R[2]=0x80000000;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x4210 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x0600024C, 0xDEADCAFE ); 

  MSH2->regs.PC = ( 0x06000246 );
  MSH2->regs.SR.all = ( 0x0 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0x7fffffff, MSH2->regs.R[2] );
 EXPECT_EQ( 0x0, MSH2->regs.SR.all );
}

}  // namespace
