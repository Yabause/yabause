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

class MaclTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  MaclTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~MaclTest() {   
  }   

virtual void SetUp() {
 
  
}

virtual void TearDown() {


}

};

TEST_F(MaclTest, normal) {

//0602E3C2: MACL R[6]=06001F00@FFFBF400,R[7]=06030060@FFFF1CC5,MACH=00000003,MACL=9792C400
//0602E3C4: MACL R[6]=06001F04@FFFB60F6,R[7]=06030064@00007421,MACH=00000001,MACL=7EE9BBB6

  MSH2->regs.R[6]=0x06001F04;
  MSH2->regs.R[7]=0x06030064;
  MSH2->regs.MACH =(0x00000003);
  MSH2->regs.MACL =(0x9792C400);

  SH2MappedMemoryWriteLong(MSH2, 0x06001F04, 0xFFFB60F6 );
  SH2MappedMemoryWriteLong(MSH2, 0x06030064, 0x00007421 );

  // mac.l @r7+, @r6+
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x067F );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x00000001, MSH2->regs.MACH );
  EXPECT_EQ( 0x7EE9BBB6, MSH2->regs.MACL );

}

TEST_F(MaclTest, normal2) {

// 0602e3c2: macli 0x067f R[6]=0x00036420@0x06001EC4 R[7]=0xffff1cc5@0x060300c0  MACL 0x00000000 MACH 0x00000000
// 0602e3c2: maclo 0x067f MACL 0xfd6f8ca0 MACH 0xfffffffc

  MSH2->regs.R[6]=0x06001F04;
  MSH2->regs.R[7]=0x060300c0;
  MSH2->regs.SR.all =( 0xFFFFFFFF );
  MSH2->regs.MACH =(0);
  MSH2->regs.MACL =(0);

  SH2MappedMemoryWriteLong(MSH2, 0x06001F04, 0x00036420 );
  SH2MappedMemoryWriteLong(MSH2, 0x060300c0, 0xffff1cc5 );

  // mac.l @r7+, @r6+
  SH2MappedMemoryWriteWord(MSH2, 0x06000000, 0x067F );
  SH2MappedMemoryWriteWord(MSH2, 0x06000002, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000004, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000000 );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0xfffffffc, MSH2->regs.MACH );
  EXPECT_EQ( 0xfd6f8ca0, MSH2->regs.MACL );

}

}  // namespace
