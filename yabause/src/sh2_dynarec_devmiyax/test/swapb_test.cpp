#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"

namespace {

class SwapbTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  SwapbTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~SwapbTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
 
  
}

virtual void TearDown() {


}

};

TEST_F(SwapbTest, normal) {

  // 0601157E: swapwin 6249, R[2]=06034CE8 R[4]=00120000
  // 0601157E: swapwout 6249, R[2]=00000012 R[4]=00120000

  pctx_->GetGenRegPtr()[2]=0x06034CE8;
  pctx_->GetGenRegPtr()[4]=0x00120000;

  // swap.w r2,r4
  memSetWord( 0x06000000, 0x6248 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x00120000, pctx_->GetGenRegPtr()[2] );
  EXPECT_EQ( 0x00120000, pctx_->GetGenRegPtr()[4] );

}
TEST_F(SwapbTest, normal2) {

  pctx_->GetGenRegPtr()[2]=0x00000002;
  pctx_->GetGenRegPtr()[3]=0xAABBCCDD;

  // swap.w r2,r3
  memSetWord( 0x06000000, 0x6238 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xAABBDDCC, pctx_->GetGenRegPtr()[2] );
  EXPECT_EQ( 0xAABBCCDD, pctx_->GetGenRegPtr()[3] );

}

TEST_F(SwapbTest, sonicr) {


  pctx_->GetGenRegPtr()[13]=0x00000000;
  pctx_->GetGenRegPtr()[0]=0x00000001;

  // swap.w r2,r3
  memSetWord( 0x06000000, 0x6d08 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x00000100, pctx_->GetGenRegPtr()[13] );
  EXPECT_EQ( 0x00000001, pctx_->GetGenRegPtr()[0] );

}

}  // namespace
