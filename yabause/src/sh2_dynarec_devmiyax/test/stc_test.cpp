#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"

namespace {

class StcTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  StcTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~StcTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(StcTest, stcsr) {

  pctx_->GetGenRegPtr()[1]=0xDEADCAFE;
  pctx_->SET_SR(0x03216721);

  memSetWord( 0x06000000, 0x0102 );
  memSetWord( 0x06000002, 0x000b );
  memSetWord( 0x06000004, 0x0009 );
  memSetLong( 0x06000250, 0x03216721 );

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x321, pctx_->GetGenRegPtr()[1] );
}

TEST_F(StcTest, stcmsr) {

  pctx_->GetGenRegPtr()[1]=0x06000250;
  pctx_->SET_SR(0x3F3);

  memSetWord( 0x06000000, 0x4103 );
  memSetWord( 0x06000002, 0x000b );
  memSetWord( 0x06000004, 0x0009 );
  memSetLong( 0x0600024C, 0x03216FFF );

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x0600024C, pctx_->GetGenRegPtr()[1] );
  EXPECT_EQ( 0x3F3, memGetLong( 0x0600024C ) );

}

TEST_F(StcTest, stcgbr) {

  pctx_->GetGenRegPtr()[1]=0x06000250;
  pctx_->SET_GBR(0xDEADCAFE);

  memSetWord( 0x06000000, 0x0112 );
  memSetWord( 0x06000002, 0x000b );
  memSetWord( 0x06000004, 0x0009 );
  memSetLong( 0x0600024C, 0x03216FFF );

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xDEADCAFE, pctx_->GetGenRegPtr()[1] );

}

TEST_F(StcTest, stcmgbr) {

  pctx_->GetGenRegPtr()[1]=0x06000250;
  pctx_->SET_GBR(0xDEADCAFE);

  memSetWord( 0x06000000, 0x4113 );
  memSetWord( 0x06000002, 0x000b );
  memSetWord( 0x06000004, 0x0009 );
  memSetLong( 0x0600024C, 0x03216FFF );

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x0600024C, pctx_->GetGenRegPtr()[1] );
  EXPECT_EQ( 0xDEADCAFE, memGetLong( 0x0600024C ) );

}

TEST_F(StcTest, stcvbr) {

  pctx_->GetGenRegPtr()[1]=0x06000250;
  pctx_->SET_VBR(0xDEADCAFE);

  memSetWord( 0x06000000, 0x0122 );
  memSetWord( 0x06000002, 0x000b );
  memSetWord( 0x06000004, 0x0009 );
  memSetLong( 0x0600024C, 0x03216FFF );

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xDEADCAFE, pctx_->GetGenRegPtr()[1] );

}

TEST_F(StcTest, stcmvbr) {

  pctx_->GetGenRegPtr()[1]=0x06000250;
  pctx_->SET_VBR(0xDEADCAFE);

  memSetWord( 0x06000000, 0x4123 );
  memSetWord( 0x06000002, 0x000b );
  memSetWord( 0x06000004, 0x0009 );
  memSetLong( 0x0600024C, 0x03216FFF );

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x0600024C, pctx_->GetGenRegPtr()[1] );
  EXPECT_EQ( 0xDEADCAFE, memGetLong( 0x0600024C ) );

}

}  // namespacegPtr
