#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"


namespace {

class BfTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  BfTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~BfTest() {
    delete pctx_;    
  }

virtual void SetUp() {
  printf("BfTest::SetUp\n");
  
}

virtual void TearDown() {
  printf("BfTest::TearDown\n");

}

};

TEST_F(BfTest, normal) {

  // rtcl R[0]

  memSetWord(0x06002E4C,0x8BFE);



  pctx_->SET_PC( 0x06002E4C );
  pctx_->SET_SR( 0x000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x06002E4C, pctx_->GET_PC() );

}

TEST_F(BfTest, normal2) {

  // rtcl R[0]

  memSetWord(0x06002E4C,0x8BF0);



  pctx_->SET_PC( 0x06002E4C );
  pctx_->SET_SR( 0x000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x06002E30, pctx_->GET_PC() );

}

TEST_F(BfTest, carry) {

  // rtcl R[0]

  memSetWord(0x06002E4C,0x8BFE);



  pctx_->SET_PC( 0x06002E4C );
  pctx_->SET_SR( 0x000001);
  pctx_->Execute();

  EXPECT_EQ( 0x06002E4E, pctx_->GET_PC() );

}

TEST_F(BfTest, bfs) {

  // rtcl R[0]

  memSetWord(0x06002E4C,0x8F0B);
  memSetWord( 0x06002E4E, 0x0009 );  // nop 

  pctx_->SET_PC( 0x06002E4C );
  pctx_->SET_SR( 0x000000);
  pctx_->Execute();

  EXPECT_EQ( 0x06002E4C+4+(0xB<<1), pctx_->GET_PC() );

  memSetWord(0x06002E4C,0x8F0B);
  memSetWord( 0x06002E4E, 0x0009 );  // nop 

  pctx_->SET_PC( 0x06002E4C );
  pctx_->SET_SR( 0x000001);
  pctx_->Execute();

  EXPECT_EQ( 0x06002E4C+4, pctx_->GET_PC() );

  pctx_->GetGenRegPtr()[3]=0x2;

  memSetWord(0x06002E4C,0x8F8B);
  memSetWord( 0x06002E4E, 0x7304 );  // add #4, r3
  memSetWord( 0x06002E50, 0x0009 );  // nop 

  pctx_->SET_PC( 0x06002E4C );
  pctx_->SET_SR( 0x000000);
  pctx_->Execute();

  EXPECT_EQ( 0x6, pctx_->GetGenRegPtr()[3] );
  EXPECT_EQ( 0x06002E4C+4+(0xFFFFFF8B<<1), pctx_->GET_PC() );

  memSetWord(0x06002E4C,0x8F8B);
  memSetWord( 0x06002E4E, 0x0009 );  // nop 

  pctx_->SET_PC( 0x06002E4C );
  pctx_->SET_SR( 0x000001);
  pctx_->Execute();

  EXPECT_EQ( 0x06002E4C+4, pctx_->GET_PC() );

}

}  // namespace
