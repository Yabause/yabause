#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"

namespace {

class CmpeqTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  CmpeqTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~CmpeqTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(CmpeqTest, normal) {

 pctx_->GetGenRegPtr()[1]=0x7C; // m
 pctx_->GetGenRegPtr()[2]=0x7C; // m
  pctx_->SET_SR(0x00000E0);

  memSetWord( 0x06000000, 0x3210 );  // 
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x000000E1, pctx_->GET_SR() );

 pctx_->GetGenRegPtr()[1]=0x7C; // m
 pctx_->GetGenRegPtr()[2]=0xFC; // m
  pctx_->SET_SR(0x00000E1);

  memSetWord( 0x06000000, 0x3210 );  // 
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x000000E0, pctx_->GET_SR() );

}

TEST_F(CmpeqTest, immediate) {

  pctx_->GetGenRegPtr()[0]=0x7C; // m
  pctx_->SET_SR(0x00000E0);

  memSetWord( 0x06000000, 0x887C );  // cmphi 
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x000000E1, pctx_->GET_SR() );

  pctx_->GetGenRegPtr()[0]=0x7E; // m
  pctx_->SET_SR(0x00000E1);

  memSetWord( 0x06000000, 0x887C );  // cmphi 
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x000000E0, pctx_->GET_SR() );

  pctx_->GetGenRegPtr()[0]=0xFFFFFFCE; // m
  pctx_->SET_SR(0x00000E0);

  memSetWord( 0x06000000, 0x88CE );  // cmphi 
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x000000E1, pctx_->GET_SR() );

  pctx_->GetGenRegPtr()[0]=0xFFFFFFC7; // m
  pctx_->SET_SR(0x00000E1);

  memSetWord( 0x06000000, 0x88CE );  // cmphi 
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x000000E0, pctx_->GET_SR() );
}


}  // namespacegPtr
