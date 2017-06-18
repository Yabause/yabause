#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"

namespace {

class ClrtTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  ClrtTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~ClrtTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};


TEST_F(ClrtTest, normal) {

  pctx_->SET_SR(0x00000FF);

  memSetWord( 0x06000000, 0x0008 );  // clrt
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x000000FE, pctx_->GET_SR() );
}



}  // namespacegPtr
