#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"


namespace {

class TestiTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  TestiTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~TestiTest() {
    delete pctx_;    
  }

virtual void SetUp() {
 
}

virtual void TearDown() {
}

};

TEST_F(TestiTest, normal) {

  pctx_->GetGenRegPtr()[0]=0x000000FA;

  // tst R[0]x, 0xFA
  memSetWord( 0x06000000, 0xC8FA );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->SET_SR( 0x000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x00, (pctx_->GET_SR()&0x01) );
  EXPECT_EQ( 0x000000FA, pctx_->GetGenRegPtr()[0]);
}



TEST_F(TestiTest, normal2) {

  pctx_->GetGenRegPtr()[0]=0x0000000F;

 // tst R[0]x, 0xF0
  memSetWord( 0x06000000, 0xC8F0 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop
  pctx_->SET_PC( 0x06000000 );
  pctx_->SET_SR( 0x000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x01, (pctx_->GET_SR()&0x01) );

}

}  // namespace
