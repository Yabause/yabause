#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"


namespace {

class RotrTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  RotrTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~RotrTest() {
    delete pctx_;    
  }

virtual void SetUp() {
  printf("RotrTest::SetUp\n");
  
}

virtual void TearDown() {
  printf("RotrTest::TearDown\n");

}

};

TEST_F(RotrTest, normal) {

  pctx_->GetGenRegPtr()[4]=0x80000000;

  // rotr R[4]
  memSetWord( 0x06000000, 0x4405 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->SET_SR( 0x000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x00, (pctx_->GET_SR()&0x01) );
  EXPECT_EQ( 0x40000000, pctx_->GetGenRegPtr()[4] );

}



TEST_F(RotrTest, carry) {

  pctx_->GetGenRegPtr()[4]=0x00000001;

  // rotr R[4]
  memSetWord( 0x06000000, 0x4405 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->SET_SR( 0x000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x01, (pctx_->GET_SR()&0x01) );
  EXPECT_EQ( 0x80000000, pctx_->GetGenRegPtr()[4] );
}

TEST_F(RotrTest, from_carry) {

  pctx_->GetGenRegPtr()[4]=0x80000001;

  // rotr R[4]
  memSetWord( 0x06000000, 0x4405 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->SET_SR( 0x000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x01, (pctx_->GET_SR()&0x01) );
  EXPECT_EQ( 0xc0000000, pctx_->GetGenRegPtr()[4] );

}


}  // namespace
