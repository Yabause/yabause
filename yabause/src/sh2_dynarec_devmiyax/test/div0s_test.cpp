#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"

namespace {

class Div0sTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  Div0sTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~Div0sTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};


TEST_F(Div0sTest, normal) {

  pctx_->GetGenRegPtr()[2]=0x1; // m
  pctx_->GetGenRegPtr()[3]=0x2; // n
  pctx_->SET_SR(0x0000000);

  memSetWord( 0x06000000, 0x2327 );  // div0s
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x1, pctx_->GetGenRegPtr()[2] );
  EXPECT_EQ( 0x2, pctx_->GetGenRegPtr()[3] );
  EXPECT_EQ( 0x00000000, pctx_->GET_SR() );
}


TEST_F(Div0sTest, normal_101) {

  pctx_->GetGenRegPtr()[2]=0x00000000; // m
  pctx_->GetGenRegPtr()[3]=0xFFFFFFCD; // n
  pctx_->SET_SR(0x0000000);

  memSetWord( 0x06000000, 0x2327 );  // div0s
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x00000000, pctx_->GetGenRegPtr()[2] );
  EXPECT_EQ( 0xFFFFFFCD, pctx_->GetGenRegPtr()[3] );
  EXPECT_EQ( 0x00000101, pctx_->GET_SR() );
}

TEST_F(Div0sTest, normal_201 ) {

  pctx_->GetGenRegPtr()[2]=0xFFFFFFCD; // m
  pctx_->GetGenRegPtr()[3]=0x00000000; // n
  pctx_->SET_SR(0x0000000);

  memSetWord( 0x06000000, 0x2327 );  // div0s
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xFFFFFFCD, pctx_->GetGenRegPtr()[2] );
  EXPECT_EQ( 0x00000000, pctx_->GetGenRegPtr()[3] );
  EXPECT_EQ( 0x00000201, pctx_->GET_SR() );
}

TEST_F(Div0sTest, normal_300 ) {

  pctx_->GetGenRegPtr()[2]=0xFFFFFFCD; // m
  pctx_->GetGenRegPtr()[3]=0xFFFFFFCE; // n
  pctx_->SET_SR(0x0000000);

  memSetWord( 0x06000000, 0x2327 );  // div0s
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xFFFFFFCD, pctx_->GetGenRegPtr()[2] );
  EXPECT_EQ( 0xFFFFFFCE, pctx_->GetGenRegPtr()[3] );
  EXPECT_EQ( 0x00000300, pctx_->GET_SR() );
}

}  // namespacegPtr
