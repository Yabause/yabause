#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"

namespace {

class CmpPzTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  CmpPzTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~CmpPzTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};


TEST_F(CmpPzTest, normal) {

  pctx_->GetGenRegPtr()[1]=0x0001; // m
  pctx_->SET_SR(0x00000E0);

  memSetWord( 0x06000000, 0x4111 );  // cmppl R[1]
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x000000E1, pctx_->GET_SR() );
}

TEST_F(CmpPzTest, Zero) {

  pctx_->GetGenRegPtr()[1]=0x0; // m
  pctx_->SET_SR(0x00000E0);

  memSetWord( 0x06000000, 0x4111 );  // cmppz R[1]
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x000000E1, pctx_->GET_SR() );
}

TEST_F(CmpPzTest, min) {

  pctx_->GetGenRegPtr()[1]=0xFFFFFFFE; // m
  pctx_->SET_SR(0x00000E0);

  memSetWord( 0x06000000, 0x4111 );  // cmppz R[1]
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x000000E0, pctx_->GET_SR() );
}


}  // namespacegPtr
