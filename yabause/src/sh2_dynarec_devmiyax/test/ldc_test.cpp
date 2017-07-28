#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"

namespace {

class LdcTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  LdcTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~LdcTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(LdcTest, normal) {

  pctx_->GetGenRegPtr()[0]=0xDEADCFFF; //source

  // subc r1,r2
  memSetWord( 0x06000000, 0x400E );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->SET_SR( 0x00000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xDEADCFFF, pctx_->GetGenRegPtr()[0] );
  EXPECT_EQ( 0x3F3, pctx_->GET_SR() );
}

TEST_F(LdcTest, ldcmsr) {

  pctx_->GetGenRegPtr()[1]=0x06000250; //source

  // subc r1,r2
  memSetWord( 0x06000000, 0x4107 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop
  memSetLong( 0x06000250, 0x03210321 );

  pctx_->SET_PC( 0x06000000 );
  pctx_->SET_SR( 0x00000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x06000254, pctx_->GetGenRegPtr()[1] );
  EXPECT_EQ( 0x321, pctx_->GET_SR() );
}

TEST_F(LdcTest, ldcgbr) {

  pctx_->GetGenRegPtr()[1]=0x06000250; //source

  // subc r1,r2
  memSetWord( 0x06000000, 0x411E );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->SET_SR( 0x00000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x06000250, pctx_->GetGenRegPtr()[1] );
  EXPECT_EQ( 0x06000250, pctx_->GET_GBR() );
}

TEST_F(LdcTest, ldcmgbr) {

  pctx_->GetGenRegPtr()[1]=0x06000250; //source

  // subc r1,r2
  memSetWord( 0x06000000, 0x4117 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop
  memSetLong( 0x06000250, 0x03210321 );

  pctx_->SET_PC( 0x06000000 );
  pctx_->SET_SR( 0x00000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x06000254, pctx_->GetGenRegPtr()[1] );
  EXPECT_EQ( 0x03210321, pctx_->GET_GBR() );
}

TEST_F(LdcTest, ldcvbr) {

  pctx_->GetGenRegPtr()[1]=0x06000250; //source

  // subc r1,r2
  memSetWord( 0x06000000, 0x412E );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop
  memSetLong( 0x06000250, 0x03210321 );

  pctx_->SET_PC( 0x06000000 );
  pctx_->SET_SR( 0x00000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x06000250, pctx_->GetGenRegPtr()[1] );
  EXPECT_EQ( 0x06000250, pctx_->GET_VBR() );
}

TEST_F(LdcTest, ldcmvbr) {

  pctx_->GetGenRegPtr()[1]=0x06000250; //source

  // subc r1,r2
  memSetWord( 0x06000000, 0x4127 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop
  memSetLong( 0x06000250, 0x03210321 );

  pctx_->SET_PC( 0x06000000 );
  pctx_->SET_SR( 0x00000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x06000254, pctx_->GetGenRegPtr()[1] );
  EXPECT_EQ( 0x03210321, pctx_->GET_VBR() );
}

}  // namespacegPtr
