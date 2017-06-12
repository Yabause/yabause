#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"

namespace {

class CmpStrTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  CmpStrTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~CmpStrTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};


TEST_F(CmpStrTest, normal) {

  pctx_->GetGenRegPtr()[4]=0x2e00ffff; // m
  pctx_->GetGenRegPtr()[6]=0x00000000;
  pctx_->SET_SR(0x00000E0);

  memSetWord( 0x06000000, 0x264c );  // cmpstr R[4] R[6]
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x000000E1, pctx_->GET_SR() );
}

TEST_F(CmpStrTest, equal) {
  pctx_->GetGenRegPtr()[4]=0x2e00ffff; // m
  pctx_->GetGenRegPtr()[6]=0xffff1111; // n
  pctx_->SET_SR(0x00000E1);

  memSetWord( 0x06000000, 0x264c );  // cmpstr R[4] R[6]
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x000000E0, pctx_->GET_SR() );

}



}  // namespacegPtr
