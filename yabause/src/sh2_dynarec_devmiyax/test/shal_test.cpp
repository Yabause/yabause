#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"

namespace {

class ShalTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  ShalTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~ShalTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};


TEST_F(ShalTest, normal) {

  pctx_->GetGenRegPtr()[3]=0xC; // m
  pctx_->SET_SR(0x00000F1);

  memSetWord( 0x06000000, 0x4320 );  // shal
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x18, pctx_->GetGenRegPtr()[3] );
  EXPECT_EQ( 0x000000F0, pctx_->GET_SR() );
}

TEST_F(ShalTest, normal01) {

  pctx_->GetGenRegPtr()[3]=0x8000000C; // m
  pctx_->SET_SR(0x00000F0);

  memSetWord( 0x06000000, 0x4320 );  // shal
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x18, pctx_->GetGenRegPtr()[3] );
  EXPECT_EQ( 0x000000F1, pctx_->GET_SR() );
}


}  // namespacegPtr
