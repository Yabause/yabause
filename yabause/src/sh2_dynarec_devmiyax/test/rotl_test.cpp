#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"


namespace {

class RotlTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  RotlTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~RotlTest() {
    delete pctx_;    
  }

virtual void SetUp() {
  printf("RotlTest::SetUp\n");
  
}

virtual void TearDown() {
  printf("RotlTest::TearDown\n");

}

};

TEST_F(RotlTest, normal) {

  pctx_->GetGenRegPtr()[4]=0x3ff00000;

  // rotl R[4]
  memSetWord( 0x06000000, 0x4404 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->SET_SR( 0x000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x00, (pctx_->GET_SR()&0x01) );
  EXPECT_EQ( 0x7fe00000, pctx_->GetGenRegPtr()[4] );

}



TEST_F(RotlTest, carry) {

  pctx_->GetGenRegPtr()[4]=0x80000000;

  // rotl R[4]
  memSetWord( 0x06000000, 0x4404 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->SET_SR( 0x000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x01, (pctx_->GET_SR()&0x01) );
  EXPECT_EQ( 0x00000001, pctx_->GetGenRegPtr()[4] );
}

TEST_F(RotlTest, from_carry) {

  pctx_->GetGenRegPtr()[4]=0x00000000;

  // rotl R[4]
  memSetWord( 0x06000000, 0x4404 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->SET_SR( 0x000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x00, (pctx_->GET_SR()&0x01) );
  EXPECT_EQ( 0x00, pctx_->GetGenRegPtr()[4] );

}


}  // namespace
