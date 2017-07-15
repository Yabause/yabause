#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"

namespace {

class CmpimTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  CmpimTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~CmpimTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};


TEST_F(CmpimTest, normal) {

  pctx_->GetGenRegPtr()[0]=0xFFFFFFFF; // m
  pctx_->SET_SR(0x00000E0);

  memSetWord( 0x06000000, 0x88ff );  // cmppl R[1]
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x000000E1, pctx_->GET_SR() );
}

TEST_F(CmpimTest, Zero) {

  pctx_->GetGenRegPtr()[0]=0xFFFFFFFF; // m
  pctx_->SET_SR(0x00000E1);

  memSetWord( 0x06000000, 0x8800 );  // shar
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x000000E0, pctx_->GET_SR() );
}



}  // namespacegPtr
