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

class SwapwTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  SwapwTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~SwapwTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
 
  
}

virtual void TearDown() {


}

};

TEST_F(SwapwTest, normal) {

  // 0601157E: swapwin 6249, R[2]=06034CE8 R[4]=00120000
  // 0601157E: swapwout 6249, R[2]=00000012 R[4]=00120000

  pctx_->GetGenRegPtr()[2]=0x06034CE8;
  pctx_->GetGenRegPtr()[4]=0x00120000;

  // swap.w r2,r4
  memSetWord( 0x06000000, 0x6249 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x00000012, pctx_->GetGenRegPtr()[2] );
  EXPECT_EQ( 0x00120000, pctx_->GetGenRegPtr()[4] );

}
TEST_F(SwapwTest, normal2) {

  // 0602EA70: swapwin 6239, R[2]=00000002 R[3]=AABBCCDD
  // 0602EA70: swapwout 6239, R[2]=CCDDAABB R[3]=AABBCCDD

  pctx_->GetGenRegPtr()[2]=0x00000002;
  pctx_->GetGenRegPtr()[3]=0xAABBCCDD;

  // swap.w r2,r3
  memSetWord( 0x06000000, 0x6239 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xCCDDAABB, pctx_->GetGenRegPtr()[2] );
  EXPECT_EQ( 0xAABBCCDD, pctx_->GetGenRegPtr()[3] );

}

}  // namespace