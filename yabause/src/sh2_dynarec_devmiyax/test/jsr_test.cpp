#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"

namespace {

class JsrTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  JsrTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~JsrTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(JsrTest, jsr) {

  pctx_->GetGenRegPtr()[1]=0x03216721;

  memSetWord( 0x06000000, 0x410B );
  memSetWord( 0x06000002, 0x0009 );

  pctx_->SET_PC( 0x06000000 );
  pctx_->SET_SR( 0x00000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x03216721, pctx_->GetGenRegPtr()[1] );
  EXPECT_EQ( 0x03216721, pctx_->GET_PC() );
  EXPECT_EQ( 0x06000004, pctx_->GET_PR() );
}

}  // namespacegPtr
