#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"

namespace {

class CmpgtTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  CmpgtTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~CmpgtTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};


TEST_F(CmpgtTest, normal) {

  pctx_->GetGenRegPtr()[1]=0x06020000; // m
  pctx_->GetGenRegPtr()[2]=0x06010000;
  pctx_->SET_SR(0x00000E0);

  memSetWord( 0x06000000, 0x3127 );  // cmphi 
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x000000E1, pctx_->GET_SR() );
}

TEST_F(CmpgtTest, Zero) {

  pctx_->GetGenRegPtr()[1]=0x06010000; // m
  pctx_->GetGenRegPtr()[2]=0x06020000;
  pctx_->SET_SR(0x00000E1);

  memSetWord( 0x06000000, 0x3127 );  // cmphi 
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x000000E0, pctx_->GET_SR() );
}

TEST_F(CmpgtTest, min) {
  pctx_->GetGenRegPtr()[2]=0xFFFFFFFF; // m
  pctx_->GetGenRegPtr()[1]=0x00000000;
  pctx_->SET_SR(0x00000E0);

  memSetWord( 0x06000000, 0x3127 );  // cmphi 
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x000000E1, pctx_->GET_SR() );

}

TEST_F(CmpgtTest, same) {

  pctx_->GetGenRegPtr()[1]=0x06010000; // m
  pctx_->GetGenRegPtr()[2]=0x06010000;
  pctx_->SET_SR(0x00000E1);

  memSetWord( 0x06000000, 0x3127 );  // cmphi 
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x000000E0, pctx_->GET_SR() );

}


}  // namespacegPtr
