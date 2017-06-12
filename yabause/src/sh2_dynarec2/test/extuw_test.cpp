#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"

namespace {

class ExtuwTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  ExtuwTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~ExtuwTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(ExtuwTest, normal) {

  pctx_->GetGenRegPtr()[0]=0xFFFFFFFF; //source

  // subc r1,r2
  memSetWord( 0x06000000, 0x600d );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x0000FFFF, pctx_->GetGenRegPtr()[0] );
}

TEST_F(ExtuwTest, normal_T1) {

  pctx_->GetGenRegPtr()[0]=0x00000080; //source

  // subc r1,r2
  memSetWord( 0x06000000, 0x600d );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x00000080, pctx_->GetGenRegPtr()[0] );
}

}  // namespacegPtr
