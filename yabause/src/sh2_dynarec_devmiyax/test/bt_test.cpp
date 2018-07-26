#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"

//SH2_struct *CurrentSH2;
//yabsys_struct yabsys;

namespace {

class BtTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  BtTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~BtTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
 
  
}

virtual void TearDown() {


}

};

TEST_F(BtTest, normal) {

  memSetWord( 0x06000246, 0x8902 ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->SET_SR( 0x0 );
  pctx_->Execute();

 EXPECT_EQ( 0x06000248, pctx_->GET_PC() );
 EXPECT_EQ( 0x0, pctx_->GET_SR() );

  memSetWord( 0x06000246, 0x8902 ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->SET_SR( 0x1 );
  pctx_->Execute();

 EXPECT_EQ( 0x0600024E, pctx_->GET_PC() );
 EXPECT_EQ( 0x1, pctx_->GET_SR() );

  memSetWord( 0x06000246, 0x8982 ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->SET_SR( 0x0 );
  pctx_->Execute();

 EXPECT_EQ( 0x06000248, pctx_->GET_PC() );
 EXPECT_EQ( 0x0, pctx_->GET_SR() );

  memSetWord( 0x06000246, 0x8982 ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->SET_SR( 0x1 );
  pctx_->Execute();

 EXPECT_EQ( 0x0600014E, pctx_->GET_PC() );
 EXPECT_EQ( 0x1, pctx_->GET_SR() );

}

TEST_F(BtTest, bts) {

  memSetWord(0x06002E4C,0x8D0B);
  memSetWord( 0x06002E4E, 0x0009 );  // nop 

  pctx_->SET_PC( 0x06002E4C );
  pctx_->SET_SR( 0x000001);
  pctx_->Execute();

  EXPECT_EQ( 0x06002E4C+4+(0xB<<1), pctx_->GET_PC() );

  memSetWord(0x06002E4C,0x8D0B);
  memSetWord( 0x06002E4E, 0x0009 );  // nop 

  pctx_->SET_PC( 0x06002E4C );
  pctx_->SET_SR( 0x000000);
  pctx_->Execute();

  EXPECT_EQ( 0x06002E4C+4, pctx_->GET_PC() );

  pctx_->GetGenRegPtr()[3]=0x2;

  memSetWord(0x06002E4C,0x8D8B);
  memSetWord( 0x06002E4E, 0x7304 );  // add #4, r3
  memSetWord( 0x06002E50, 0x0009 );  // nop 

  pctx_->SET_PC( 0x06002E4C );
  pctx_->SET_SR( 0x000001);
  pctx_->Execute();

  EXPECT_EQ( 0x6, pctx_->GetGenRegPtr()[3] );
  EXPECT_EQ( 0x06002E4C+4+(0xFFFFFF8B<<1), pctx_->GET_PC() );

  memSetWord(0x06002E4C,0x8D8B);
  memSetWord( 0x06002E4E, 0x0009 );  // nop 

  pctx_->SET_PC( 0x06002E4C );
  pctx_->SET_SR( 0x000000);
  pctx_->Execute();

  EXPECT_EQ( 0x06002E4C+4, pctx_->GET_PC() );

}

}  // namespace
