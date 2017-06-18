#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"

namespace {

class ClrmacTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  ClrmacTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~ClrmacTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};


TEST_F(ClrmacTest, normal) {

  pctx_->SET_MACH(0x00000FF);
  pctx_->SET_MACL(0xFF00000);

  memSetWord( 0x06000000, 0x0028 );  // clrmac
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x00000000, pctx_->GET_MACH() );
  EXPECT_EQ( 0x00000000, pctx_->GET_MACL() );
}



}  // namespacegPtr
