#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"

namespace {

class ExtswTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  ExtswTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~ExtswTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(ExtswTest, normal) {

  pctx_->GetGenRegPtr()[4]=0x0000e1fb; //source

  // subc r1,r2
  memSetWord( 0x06000000, 0x644f );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xffffe1fb, pctx_->GetGenRegPtr()[4] );
}

TEST_F(ExtswTest, normal_T1) {

  pctx_->GetGenRegPtr()[4]=0x00000080; //source

  // subc r1,r2
  memSetWord( 0x06000000, 0x644f );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x00000080, pctx_->GetGenRegPtr()[4] );
}

}  // namespacegPtr
