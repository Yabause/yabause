#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"


namespace {

class RotcrTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  RotcrTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~RotcrTest() {
    delete pctx_;    
  }

virtual void SetUp() {
  printf("RotcrTest::SetUp\n");
  
}

virtual void TearDown() {
  printf("RotcrTest::TearDown\n");

}

};

TEST_F(RotcrTest, normal) {

  pctx_->GetGenRegPtr()[0]=0x80000000;

  // rtcr R[0]
  memSetWord( 0x06000000, 0x4025 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->SET_SR( 0x000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x00, (pctx_->GET_SR()&0x01) );
  EXPECT_EQ( 0x40000000, pctx_->GetGenRegPtr()[0] );

}



TEST_F(RotcrTest, carry) {

  pctx_->GetGenRegPtr()[0]=0x80000001;

  // rtcr R[0]
  memSetWord( 0x06000000, 0x4025 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop
  pctx_->SET_PC( 0x06000000 );
  pctx_->SET_SR( 0x000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x01, (pctx_->GET_SR()&0x01) );
  EXPECT_EQ( 0x40000000, pctx_->GetGenRegPtr()[0] );

}

TEST_F(RotcrTest, from_carry) {

  pctx_->GetGenRegPtr()[0]=0x00000000;

  // rtcr R[0]
  memSetWord( 0x06000000, 0x4025 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop
  pctx_->SET_PC( 0x06000000 );
  pctx_->SET_SR( 0x0000001 );
  pctx_->Execute();

  EXPECT_EQ( 0x00, (pctx_->GET_SR()&0x01) );
  EXPECT_EQ( 0x80000000, pctx_->GetGenRegPtr()[0] );

}


}  // namespace
