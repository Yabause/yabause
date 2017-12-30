#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"

//SH2_struct *CurrentSH2;
//yabsys_struct yabsys;

namespace {

class MaclTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  MaclTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~MaclTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
 
  
}

virtual void TearDown() {


}

};

TEST_F(MaclTest, normal) {

//0602E3C2: MACL R[6]=06001F00@FFFBF400,R[7]=06030060@FFFF1CC5,MACH=00000003,MACL=9792C400
//0602E3C4: MACL R[6]=06001F04@FFFB60F6,R[7]=06030064@00007421,MACH=00000001,MACL=7EE9BBB6

  pctx_->GetGenRegPtr()[6]=0x06001F04;
  pctx_->GetGenRegPtr()[7]=0x06030064;
  pctx_->SET_MACH(0x00000003);
  pctx_->SET_MACL(0x9792C400);

  memSetLong( 0x06001F04, 0xFFFB60F6 );
  memSetLong( 0x06030064, 0x00007421 );

  // mac.l @r7+, @r6+
  memSetWord( 0x06000000, 0x067F );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x00000001, pctx_->GET_MACH() );
  EXPECT_EQ( 0x7EE9BBB6, pctx_->GET_MACL() );

}

TEST_F(MaclTest, normal2) {

// 0602e3c2: macli 0x067f R[6]=0x00036420@0x06001EC4 R[7]=0xffff1cc5@0x060300c0  MACL 0x00000000 MACH 0x00000000
// 0602e3c2: maclo 0x067f MACL 0xfd6f8ca0 MACH 0xfffffffc

  pctx_->GetGenRegPtr()[6]=0x06001F04;
  pctx_->GetGenRegPtr()[7]=0x060300c0;
  pctx_->SET_SR( 0xFFFFFFFF );
  pctx_->SET_MACH(0);
  pctx_->SET_MACL(0);

  memSetLong( 0x06001F04, 0x00036420 );
  memSetLong( 0x060300c0, 0xffff1cc5 );

  // mac.l @r7+, @r6+
  memSetWord( 0x06000000, 0x067F );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xfffffffc, pctx_->GET_MACH() );
  EXPECT_EQ( 0xfd6f8ca0, pctx_->GET_MACL() );

}

}  // namespace
