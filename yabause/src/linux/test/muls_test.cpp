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

class MulsTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  MulsTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~MulsTest() {   
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(MulsTest, normal) {

  MSH2->regs.R[9]=0x0000001;
  MSH2->regs.R[0x0b]=0x000001f4;

  // macl r1, r3
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x29bf );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x01f4, MSH2->regs.MACL );

}




}  // namespace
