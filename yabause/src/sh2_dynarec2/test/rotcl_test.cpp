#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"


namespace {

class RotclTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  RotclTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~RotclTest() {
    delete pctx_;    
  }

virtual void SetUp() {
  printf("RotclTest::SetUp\n");
  
}

virtual void TearDown() {
  printf("RotclTest::TearDown\n");

}

};

TEST_F(RotclTest, normal) {

  pctx_->GetGenRegPtr()[0]=0x00000001;

  // rtcl R[0]
  memSetWord( 0x000000, 0x4024 );
  memSetWord( 0x000002, 0x000b );  // rts
  memSetWord( 0x000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x000000 );
  pctx_->SET_SR( 0x000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x00, (pctx_->GET_SR()&0x01) );
  EXPECT_EQ( 0x02, pctx_->GetGenRegPtr()[0] );

}



TEST_F(RotclTest, carry) {

  pctx_->GetGenRegPtr()[0]=0x80000000;

  // rtcl R[0]
  memSetWord( 0x000000, 0x4024 );
  memSetWord( 0x000002, 0x000b );  // rts
  memSetWord( 0x000004, 0x0009 );  // nop
  pctx_->SET_PC( 0x000000 );
  pctx_->SET_SR( 0x000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x01, (pctx_->GET_SR()&0x01) );
  EXPECT_EQ( 0x00, pctx_->GetGenRegPtr()[0] );

}

TEST_F(RotclTest, from_carry) {

  pctx_->GetGenRegPtr()[0]=0x00000000;

  // rtcl R[0]
  memSetWord( 0x000000, 0x4024 );
  memSetWord( 0x000002, 0x000b );  // rts
  memSetWord( 0x000004, 0x0009 );  // nop
  pctx_->SET_PC( 0x000000 );
  pctx_->SET_SR( 0x000001 );
  pctx_->Execute();

  EXPECT_EQ( 0x00, (pctx_->GET_SR()&0x01) );
  EXPECT_EQ( 0x01, pctx_->GetGenRegPtr()[0] );

}


}  // namespace
