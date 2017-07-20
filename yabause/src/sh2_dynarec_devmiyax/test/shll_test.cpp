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

class ShllTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  ShllTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~ShllTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
   
}

virtual void TearDown() {
}

};

TEST_F(ShllTest, normal) {

  pctx_->GetGenRegPtr()[2]=0x00000001;
  pctx_->SET_SR(0x000000E0);

  // shlr2
  memSetWord( 0x06000000, 0x4200 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x00000002, pctx_->GetGenRegPtr()[2] );
  EXPECT_EQ( 0x000000E0, pctx_->GET_SR() );

}

TEST_F(ShllTest, shift2) {

  pctx_->GetGenRegPtr()[2]=0xCAFEDEAD;

  // shll2
  memSetWord( 0x06000000, 0x4208 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xCAFEDEAD << 2, pctx_->GetGenRegPtr()[2] );
}

TEST_F(ShllTest, shift8) {

  pctx_->GetGenRegPtr()[2]=0xCAFEDEAD;
  pctx_->SET_SR(0x000000E0);

  // shll16
  memSetWord( 0x06000000, 0x4218 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xFEDEAD00, pctx_->GetGenRegPtr()[2] );
}

TEST_F(ShllTest, shift16) {

  pctx_->GetGenRegPtr()[2]=0xCAFEDEAD;
  pctx_->SET_SR(0x000000E0);

  // shll16
  memSetWord( 0x06000000, 0x4228 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xDEAD0000, pctx_->GetGenRegPtr()[2] );
}

TEST_F(ShllTest, tflg) {

  pctx_->GetGenRegPtr()[2]=0x80000001;
  pctx_->SET_SR(0x000000E0);

  // shlr2
  memSetWord( 0x06000000, 0x4200 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x00000002, pctx_->GetGenRegPtr()[2] );
  EXPECT_EQ( 0x000000E1, pctx_->GET_SR() );

}




}  // namespace
