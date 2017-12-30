#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"

namespace {

class Div1Test : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  Div1Test() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~Div1Test() {
    delete pctx_;    
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(Div1Test, normal) {

  pctx_->GetGenRegPtr()[0]=0x0000000E; // m
  pctx_->GetGenRegPtr()[3]=0xFFFFFFFF; // n
  pctx_->SET_SR(0x0000101);

  // subc r1,r2
  memSetWord( 0x06000000, 0x3304 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x0000000E, pctx_->GetGenRegPtr()[0] );
  EXPECT_EQ( 0x0000000D, pctx_->GetGenRegPtr()[3] );
  EXPECT_EQ( 0x00000001, pctx_->GET_SR() );
}

//060133C8: DIV1(i) 3304 R[0]:000000D9,R[3]:FFFFFFD1,SR:00000100
//060133C8: DIV1(O) m:000000D9,n:0000007B,SR:00000001

TEST_F(Div1Test, normal02) {

  pctx_->GetGenRegPtr()[0]=0x000000D9; // m
  pctx_->GetGenRegPtr()[3]=0xFFFFFFD1; // n
  pctx_->SET_SR(0x0000100);

  // subc r1,r2
  memSetWord( 0x06000000, 0x3304 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x000000D9, pctx_->GetGenRegPtr()[0] );
  EXPECT_EQ( 0x0000007B, pctx_->GetGenRegPtr()[3] );
  EXPECT_EQ( 0x00000001, pctx_->GET_SR() );
}

//06013340: DIV1i 3304 R[0]:00000008,R[3]:FFFFFFF8,SR:000001E0
//06013340: DIV1o m:00000008,n:FFFFFFF8,SR:000001E0

TEST_F(Div1Test, normal03) {

  pctx_->GetGenRegPtr()[0]=0x00000008; // m
  pctx_->GetGenRegPtr()[3]=0xFFFFFFF8; // n
  pctx_->SET_SR(0x000001E0);

  // subc r1,r2
  memSetWord( 0x06000000, 0x3304 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x00000008, pctx_->GetGenRegPtr()[0] );
  EXPECT_EQ( 0xFFFFFFF8, pctx_->GetGenRegPtr()[3] );
  EXPECT_EQ( 0x000001E0, pctx_->GET_SR() );
}

}  // namespacegPtr
