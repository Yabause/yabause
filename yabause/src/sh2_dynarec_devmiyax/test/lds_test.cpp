#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"

namespace {

class LdsTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  LdsTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~LdsTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(LdsTest, ldsmach) {

  pctx_->GetGenRegPtr()[1]=0x03216721;

  memSetWord( 0x06000000, 0x410A );
  memSetWord( 0x06000002, 0x000b );
  memSetWord( 0x06000004, 0x0009 );
  memSetLong( 0x06000250, 0x03216721 );

  pctx_->SET_PC( 0x06000000 );
  pctx_->SET_SR( 0x00000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x03216721, pctx_->GetGenRegPtr()[1] );
  EXPECT_EQ( 0x03216721, pctx_->GET_MACH() );
}

TEST_F(LdsTest, ldsmmach) {

  pctx_->GetGenRegPtr()[1]=0x06000250;

  memSetWord( 0x06000000, 0x4106 );
  memSetWord( 0x06000002, 0x000b );
  memSetWord( 0x06000004, 0x0009 );
  memSetLong( 0x06000250, 0x03216721 );

  pctx_->SET_PC( 0x06000000 );
  pctx_->SET_SR( 0x00000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x06000254, pctx_->GetGenRegPtr()[1] );
  EXPECT_EQ( 0x03216721, pctx_->GET_MACH() );
}

TEST_F(LdsTest, ldsmacl) {

  pctx_->GetGenRegPtr()[1]=0x03216721;

  memSetWord( 0x06000000, 0x411A );
  memSetWord( 0x06000002, 0x000b );
  memSetWord( 0x06000004, 0x0009 );
  memSetLong( 0x06000250, 0x03216721 );

  pctx_->SET_PC( 0x06000000 );
  pctx_->SET_SR( 0x00000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x03216721, pctx_->GetGenRegPtr()[1] );
  EXPECT_EQ( 0x03216721, pctx_->GET_MACL() );
}

TEST_F(LdsTest, ldsmmacl) {

  pctx_->GetGenRegPtr()[1]=0x06000250;

  memSetWord( 0x06000000, 0x4116 );
  memSetWord( 0x06000002, 0x000b );
  memSetWord( 0x06000004, 0x0009 );
  memSetLong( 0x06000250, 0x03216721 );

  pctx_->SET_PC( 0x06000000 );
  pctx_->SET_SR( 0x00000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x06000254, pctx_->GetGenRegPtr()[1] );
  EXPECT_EQ( 0x03216721, pctx_->GET_MACL() );
}

TEST_F(LdsTest, ldspr) {

  pctx_->GetGenRegPtr()[1]=0x03216721;

  memSetWord( 0x06000000, 0x412A );
  memSetWord( 0x06000002, 0x000b );
  memSetWord( 0x06000004, 0x0009 );
  memSetLong( 0x06000250, 0x03216721 );

  pctx_->SET_PC( 0x06000000 );
  pctx_->SET_SR( 0x00000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x03216721, pctx_->GetGenRegPtr()[1] );
  EXPECT_EQ( 0x03216721, pctx_->GET_PR() );
}

TEST_F(LdsTest, ldsmpr) {

  pctx_->GetGenRegPtr()[1]=0x06000250;

  memSetWord( 0x06000000, 0x4126 );
  memSetWord( 0x06000002, 0x000b );
  memSetWord( 0x06000004, 0x0009 );
  memSetLong( 0x06000250, 0x03216721 );

  pctx_->SET_PC( 0x06000000 );
  pctx_->SET_SR( 0x00000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x06000254, pctx_->GetGenRegPtr()[1] );
  EXPECT_EQ( 0x03216721, pctx_->GET_PR() );
}

}  // namespacegPtr
