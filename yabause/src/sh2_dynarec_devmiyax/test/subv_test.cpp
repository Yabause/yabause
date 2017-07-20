#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"

namespace {

class SubvTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  SubvTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~SubvTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

/*
subv   r0,r1    ! Before execution: r0 = 0x00000002, r1 = 0x80000001
                ! After execution: r1 = 0x7FFFFFFF, T = 1

subv   r2,r3    ! Before execution: r2 = 0xFFFFFFFE, r3 = 0x7FFFFFFE
                ! After execution r3 = 0x80000000, T = 1
*/

TEST_F(SubvTest, normal) {

  pctx_->GetGenRegPtr()[0]=0x00000002;
  pctx_->GetGenRegPtr()[1]=0x80000001;
  pctx_->SET_SR(0xE0);

  // subv   r0,r1
  memSetWord( 0x06000000, 0x310B );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x00000002, pctx_->GetGenRegPtr()[0] );
  EXPECT_EQ( 0x7FFFFFFF, pctx_->GetGenRegPtr()[1] );
  EXPECT_EQ( 0xE1, pctx_->GET_SR() );

  pctx_->GetGenRegPtr()[2]=0xFFFFFFFE;
  pctx_->GetGenRegPtr()[3]=0x7FFFFFFE;
  pctx_->SET_SR(0xE0);

  // subv   r2,r3
  memSetWord( 0x06000000, 0x332B );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );

  pctx_->Execute();

  EXPECT_EQ( 0xFFFFFFFE, pctx_->GetGenRegPtr()[2] );
  EXPECT_EQ( 0x80000000, pctx_->GetGenRegPtr()[3] );
  EXPECT_EQ( 0xE1, pctx_->GET_SR() );
}

}  // namespacegPtr
