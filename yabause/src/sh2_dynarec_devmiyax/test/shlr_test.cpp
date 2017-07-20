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

class ShlrTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  ShlrTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~ShlrTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
   
}

virtual void TearDown() {
}

};

TEST_F(ShlrTest, normal) {

  pctx_->GetGenRegPtr()[2]=0x00000001;
  pctx_->SET_SR(0);

  // shlr2
  memSetWord( 0x06000000, 0x4201 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x00000000, pctx_->GetGenRegPtr()[2] );
  EXPECT_EQ( 0x00000001, pctx_->GET_SR() );

}

TEST_F(ShlrTest, shift2) {

  pctx_->GetGenRegPtr()[2]=0x0000003D;
  pctx_->SET_SR(0xE0);

  // shlr2
  memSetWord( 0x06000000, 0x4209 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x0000000F, pctx_->GetGenRegPtr()[2] );
  EXPECT_EQ( 0xE0, pctx_->GET_SR() );

}

TEST_F(ShlrTest, shift8) {

  pctx_->GetGenRegPtr()[2]=0xFF003D01;
  pctx_->SET_SR(0x1);

  // shlr8
  memSetWord( 0x06000000, 0x4219 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x00FF003D, pctx_->GetGenRegPtr()[2] );
  EXPECT_EQ( 0x00000001, pctx_->GET_SR() );

}

TEST_F(ShlrTest, shift16) {

  pctx_->GetGenRegPtr()[2]=0xDEADCAFE;
  pctx_->SET_SR(1);

  // shlr16
  memSetWord( 0x06000000, 0x4229 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x0000DEAD, pctx_->GetGenRegPtr()[2] );
  EXPECT_EQ( 0x00000001, pctx_->GET_SR() );

}



}  // namespace
