#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"


namespace {

class BsrTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  BsrTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~BsrTest() {
    delete pctx_;    
  }

virtual void SetUp() {
  printf("BsrTest::SetUp\n");
  
}

virtual void TearDown() {
  printf("BsrTest::TearDown\n");

}

};

TEST_F(BsrTest, normal) {

  // rtcl R[0]

memSetWord(0x06002E4C,0xB123);
memSetWord(0x06002E4E, 0x0009);

  pctx_->SET_PC( 0x06002E4C );
  pctx_->SET_SR( 0x000000 );
  pctx_->Execute();

EXPECT_EQ( 0x06003096, pctx_->GET_PC() );
  EXPECT_EQ( 0x06002E50, pctx_->GET_PR() );

}

TEST_F(BsrTest, negatif) {

  // rtcl R[0]

memSetWord(0x06002E4C,0xB823);
memSetWord(0x06002E4E, 0x0009);

  pctx_->SET_PC( 0x06002E4C );
  pctx_->SET_SR( 0x000000 );
  pctx_->Execute();

EXPECT_EQ( 0x06002E4C - 4022, pctx_->GET_PC() );
  EXPECT_EQ( 0x06002E50, pctx_->GET_PR() );

}


TEST_F(BsrTest, bsrf) {

  pctx_->GetGenRegPtr()[1]=0x00006520; //source

  memSetWord(0x06002E00,0x0103);
  memSetWord(0x06002E02, 0x0009);



  pctx_->SET_PC( 0x06002E00 );
  pctx_->SET_SR( 0x000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x06009324, pctx_->GET_PC() );
  EXPECT_EQ( 0x06002E04, pctx_->GET_PR() );

}


}  // namespace
