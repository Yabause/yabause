#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"


namespace {

class TestTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  TestTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~TestTest() {
    delete pctx_;    
  }

virtual void SetUp() {
 
}

virtual void TearDown() {
}

};

TEST_F(TestTest, normal) {

  pctx_->GetGenRegPtr()[2]=0xF0F0F0F0;
  pctx_->GetGenRegPtr()[3]=0x0F0F0FFF;

  // tst R[0]x, 0xFA
  memSetWord( 0x06000000, 0x2238 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->SET_SR( 0xE1 );
  pctx_->Execute();

  EXPECT_EQ( 0x00, (pctx_->GET_SR()&0x01) );
}

TEST_F(TestTest, zero) {

  pctx_->GetGenRegPtr()[2]=0xF0F0F0F0;
  pctx_->GetGenRegPtr()[3]=0x0F0F0F0F;

  // tst R[0]x, 0xFA
  memSetWord( 0x06000000, 0x2238 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->SET_SR( 0xE0 );
  pctx_->Execute();

  EXPECT_EQ( 0x01, (pctx_->GET_SR()&0x01) );
}

TEST_F(TestTest, same) {

  pctx_->GetGenRegPtr()[2]=0xF0F0F0F0;

  // tst R[0]x, 0xFA
  memSetWord( 0x06000000, 0x2228 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->SET_SR( 0x000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x00, (pctx_->GET_SR()&0x01) );
}

TEST_F(TestTest, samezero) {

  pctx_->GetGenRegPtr()[2]=0x00000000;

  // tst R[0]x, 0xFA
  memSetWord( 0x06000000, 0x2228 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->SET_SR( 0xE1 );
  pctx_->Execute();

  EXPECT_EQ( 0x01, (pctx_->GET_SR()&0x01) );
}




}  // namespace
