#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"

namespace {

class MovtTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  MovtTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~MovtTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};


TEST_F(MovtTest, normal) {

  pctx_->GetGenRegPtr()[0]=0xFFFFFFFF; // m
  pctx_->SET_SR(0x00000E1);

  memSetWord( 0x06000000, 0x0029 );  // cmppl R[1]
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x00000001, pctx_->GetGenRegPtr()[0] );
}

TEST_F(MovtTest, Zero) {
  pctx_->GetGenRegPtr()[0]=0xFFFFFFFF; // m
  pctx_->SET_SR(0x00000E0);

  memSetWord( 0x06000000, 0x0029 );  // cmppl R[1]
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x00000000, pctx_->GetGenRegPtr()[0] );

}


}  // namespacegPtr
