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

class XoriTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  XoriTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~XoriTest() {    
  }   

virtual void SetUp() {
 
  
}

virtual void TearDown() {


}

};

TEST_F(XoriTest, normal) {


  MSH2->regs.R[0]=0x00000006;

  // xori
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0xca04 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x02, MSH2->regs.R[0]  );

}

TEST_F(XoriTest, max) {


  MSH2->regs.R[0]=0xFFFFFFFF;

  // xori
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0xca01 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0xfffffffe, MSH2->regs.R[0]  );

}

}  // namespace
