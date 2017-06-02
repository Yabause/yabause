#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"

namespace {

class Div1Test : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  Div1Test() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~Div1Test() {
    delete pctx_;    
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(Div1Test, normal) {

  pctx_->GetGenRegPtr()[0]=0x0000000E; // m
  pctx_->GetGenRegPtr()[3]=0xFFFFFFFF; // n
  pctx_->SET_SR(0x0000101);

  // subc r1,r2
  memSetWord( 0x06000000, 0x3304 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x0000000E, pctx_->GetGenRegPtr()[0] );
  EXPECT_EQ( 0x0000000D, pctx_->GetGenRegPtr()[3] );
  EXPECT_EQ( 0x00000001, pctx_->GET_SR() );
}


}  // namespacegPtr
