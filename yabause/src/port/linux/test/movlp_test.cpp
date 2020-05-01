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

class MovlpTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  MovlpTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~MovlpTest() {
  }   

virtual void SetUp() {
 
  
}

virtual void TearDown() {


}

};

TEST_F(MovlpTest, normal) {


  MSH2->regs.R[0]=0x0600030c;
  MSH2->regs.R[6]=0xe0000000;

  SH2MappedMemoryWriteLong(MSH2,0x0600030c,0x04);

  // mova
  SH2MappedMemoryWriteWord(MSH2, 0x0600024c, 0x6606 );
  SH2MappedMemoryWriteWord(MSH2, 0x0600024e, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000250, 0x0009 );  // nop

  MSH2->regs.PC =( 0x0600024c );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x06000310, MSH2->regs.R[0] );
  EXPECT_EQ( 0x00000004, MSH2->regs.R[6] );
}

TEST_F(MovlpTest, samereg) {

    MSH2->regs.R[0]=0x0600030c;

    SH2MappedMemoryWriteLong(MSH2,0x0600030c,0x04);

    // mova
    SH2MappedMemoryWriteWord(MSH2, 0x0600024c, 0x6006 );
    SH2MappedMemoryWriteWord(MSH2, 0x0600024e, 0x000b );  // rts
    SH2MappedMemoryWriteWord(MSH2, 0x06000250, 0x0009 );  // nop

    MSH2->regs.PC =( 0x0600024c );
    SH2TestExec(MSH2, 1);

    EXPECT_EQ( 0x0000004, MSH2->regs.R[0] );
}




}  // namespace
