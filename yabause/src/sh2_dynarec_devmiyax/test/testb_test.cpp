#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"


namespace {

class TestbTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  TestbTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~TestbTest() {
    delete pctx_;    
  }

virtual void SetUp() {
 
}

virtual void TearDown() {
}

};

TEST_F(TestbTest, normal) {

  pctx_->GetGenRegPtr()[0]=0x000000F0;

  memSetWord( 0x000030F0, 0x00FB );  // nop

  // tst R[0]x, 0xFA
  memSetWord( 0x06000000, 0xCC04 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->SET_SR( 0x000000 );
  pctx_->SET_GBR( 0x00003000 );

  pctx_->Execute();

  EXPECT_EQ( 0x01, (pctx_->GET_SR()&0x01) );
}

}  // namespace
