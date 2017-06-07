#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"

namespace {

class AddiTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  AddiTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~AddiTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(AddiTest, normal) {

  pctx_->GetGenRegPtr()[3]=0x60000000; //dest

  // subc r1,r2
  memSetWord( 0x06000000, 0x7310 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x60000010, pctx_->GetGenRegPtr()[3] );
}

TEST_F(AddiTest, normal_T1) {

   pctx_->GetGenRegPtr()[3]=0x60000000;
   pctx_->SET_SR(0x00000E0);

   // subc r1,r2
   memSetWord( 0x06000000, 0x73ff );
   memSetWord( 0x06000002, 0x000b );  // rts
   memSetWord( 0x06000004, 0x0009 );  // nop

   pctx_->SET_PC( 0x06000000 );
   pctx_->Execute();

   EXPECT_EQ( 0x5FFFFFFF, pctx_->GetGenRegPtr()[3] );
}

TEST_F(AddiTest, normal_T21) {

   pctx_->GetGenRegPtr()[3]=0xFFFFFFFF;

   // subc r1,r2
   memSetWord( 0x06000000, 0x73FF );
   memSetWord( 0x06000002, 0x000b );  // rts
   memSetWord( 0x06000004, 0x0009 );  // nop

   pctx_->SET_PC( 0x06000000 );
   pctx_->Execute();

   EXPECT_EQ( 0xFFFFFFFE, pctx_->GetGenRegPtr()[3] );
}
}  // namespacegPtr
