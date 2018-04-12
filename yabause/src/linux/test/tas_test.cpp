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

class TasTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  TasTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~TasTest() { 
  }   

virtual void SetUp() {
 
  
}

virtual void TearDown() {


}

};

TEST_F(TasTest, normal) {

//0602E3C2: MACL R[6]=06001F00@FFFBF400,R[7]=06030060@FFFF1CC5,MACH=00000003,MACL=9792C400
//0602E3C4: MACL R[6]=06001F04@FFFB60F6,R[7]=06030064@00007421,MACH=00000001,MACL=7EE9BBB6

  MSH2->regs.R[3]=0x060fffb8;
  MSH2->regs.SR.all = (0x00000000);

  SH2MappedMemoryWriteByte(MSH2, 0x060fffb8, 0 );

  // tas.b R[3]
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x431b );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC = ( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x80, SH2MappedMemoryReadByte(MSH2, 0x060fffb8) );
  EXPECT_EQ( 0x00000001, MSH2->regs.SR.all );

}

}  // namespace
