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

class OrbTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  OrbTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~OrbTest() { 
  }   

virtual void SetUp() {
 
  
}

virtual void TearDown() {


}

};

TEST_F(OrbTest, normal) {


  MSH2->regs.R[0]=0x00010001;
  MSH2->regs.GBR =(0x060efcbb);

  SH2MappedMemoryWriteByte(MSH2, 0x60FFCBC, 0x08 );

  // or.b #6
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0xcf06 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x0e, SH2MappedMemoryReadByte(MSH2, 0x60FFCBC) );

}


TEST_F(OrbTest, max) {


  MSH2->regs.R[0]=0x00010001;
  MSH2->regs.GBR =(0x060efcbb);

  SH2MappedMemoryWriteByte(MSH2, 0x060ffcbc, 0x08 );

  // or.b #6
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0xcfff );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0xff, SH2MappedMemoryReadByte(MSH2, 0x060ffcbc) );

}

}  // namespace
