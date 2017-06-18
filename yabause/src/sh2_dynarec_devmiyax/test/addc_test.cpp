#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"

namespace {

class AddcTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  AddcTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~AddcTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(AddcTest, normal) {

  pctx_->GetGenRegPtr()[2]=0x00000000; //source
  pctx_->GetGenRegPtr()[3]=0x000000e0; //dest
  pctx_->SET_SR(0x00000E0);

  // subc r1,r2
  memSetWord( 0x06000000, 0x332e );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x00000000, pctx_->GetGenRegPtr()[2] );
  EXPECT_EQ( 0x000000E0, pctx_->GetGenRegPtr()[3] );
  EXPECT_EQ( 0x000000E0, pctx_->GET_SR() );
}

TEST_F(AddcTest, normal_T1) {

   pctx_->GetGenRegPtr()[2]=0xFFFFFFFF;
   pctx_->GetGenRegPtr()[3]=0x00000001;
   pctx_->SET_SR(0x00000E0);

   // subc r1,r2
   memSetWord( 0x06000000, 0x332e );
   memSetWord( 0x06000002, 0x000b );  // rts
   memSetWord( 0x06000004, 0x0009 );  // nop

   pctx_->SET_PC( 0x06000000 );
   pctx_->Execute();

   EXPECT_EQ( 0xffffffff, pctx_->GetGenRegPtr()[2] );
   EXPECT_EQ( 0x00000000, pctx_->GetGenRegPtr()[3] );
   EXPECT_EQ( 0x000000E1, pctx_->GET_SR() );
}

TEST_F(AddcTest, normal_T21) {

   pctx_->GetGenRegPtr()[2]=0xFFFFFFFF;
   pctx_->GetGenRegPtr()[3]=0x00000000;
   pctx_->SET_SR(0x00000E1);

   // subc r1,r2
   memSetWord( 0x06000000, 0x332e );
   memSetWord( 0x06000002, 0x000b );  // rts
   memSetWord( 0x06000004, 0x0009 );  // nop

   pctx_->SET_PC( 0x06000000 );
   pctx_->Execute();

   EXPECT_EQ( 0xffffffff, pctx_->GetGenRegPtr()[2] );
   EXPECT_EQ( 0x00000000, pctx_->GetGenRegPtr()[3] );
   EXPECT_EQ( 0x000000E1, pctx_->GET_SR() );
}
}  // namespacegPtr
