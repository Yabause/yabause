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

class DmulsTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  DmulsTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~DmulsTest() {   
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(DmulsTest, normal) {

  MSH2->regs.R[1]=0x000ACA55;
  MSH2->regs.R[3]=0xFFFF90D9;

  // macl r1, r3
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x313D );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC = ( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x50A0520D, MSH2->regs.MACL );
  EXPECT_EQ( 0xFFFFFFFB, MSH2->regs.MACH );

}

TEST_F(DmulsTest, normal_) {

    MSH2->regs.R[2]=0x017FAF00;
    MSH2->regs.R[4]=0x008D2F00;

    // dmul r1, r3
    SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x324D );
    SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
    SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

    MSH2->regs.PC = ( 0x06000000 );
    SH2TestExec(MSH2, 1);

    EXPECT_EQ( 0xD4210000, MSH2->regs.MACL );
    EXPECT_EQ( 0x0000D399, MSH2->regs.MACH );

}


TEST_F(DmulsTest, normal_s) {

    MSH2->regs.R[4]=0xFFF5D04F;
    MSH2->regs.R[5]=0x0000056D;

    // dmul r4, r5
    SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x345D );
    SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
    SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

    MSH2->regs.PC = ( 0x06000000 );
    SH2TestExec(MSH2, 1);

    EXPECT_EQ( 0xFFFFFFFF, MSH2->regs.MACH );
    EXPECT_EQ( 0xC8BB3CA3, MSH2->regs.MACL );

}

}  // namespace
