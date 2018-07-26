#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"

namespace {

class SubTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  SubTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~SubTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(SubTest, normal) {

  pctx_->GetGenRegPtr()[1]=0x00FF00FF;
  pctx_->GetGenRegPtr()[2]=0x000000FF;

  // subc r1,r2
  memSetWord( 0x06000000, 0x3128 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x00FF0000, pctx_->GetGenRegPtr()[1] );
  EXPECT_EQ( 0x000000FF, pctx_->GetGenRegPtr()[2] );
}

TEST_F(SubTest, negative) {

  pctx_->GetGenRegPtr()[1]=0x10;
  pctx_->GetGenRegPtr()[2]=0x00000100;

  // subc r1,r2
  memSetWord( 0x06000000, 0x3128 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xFFFFFF10, pctx_->GetGenRegPtr()[1] );
  EXPECT_EQ( 0x00000100, pctx_->GetGenRegPtr()[2] );
}

}  // namespacegPtr
