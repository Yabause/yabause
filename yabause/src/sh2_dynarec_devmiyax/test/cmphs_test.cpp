#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"

namespace {

class CmphsTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  CmphsTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~CmphsTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};


TEST_F(CmphsTest, normal) {

  pctx_->GetGenRegPtr()[2]=0x06010000; // m
  pctx_->GetGenRegPtr()[0x0b]=0x0604FFFF;
  pctx_->SET_SR(0x00000E1);

  memSetWord( 0x06000000, 0x32b2 );  // cmphi 
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x000000E0, pctx_->GET_SR() );
}

TEST_F(CmphsTest, Zero) {

  pctx_->GetGenRegPtr()[2]=0x0604FFFF; // m
  pctx_->GetGenRegPtr()[0x0b]=0x06010000;
  pctx_->SET_SR(0x00000E0);

  memSetWord( 0x06000000, 0x32b2 );  // cmphi 
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x000000E1, pctx_->GET_SR() );
}

TEST_F(CmphsTest, min) {
  pctx_->GetGenRegPtr()[2]=0xFFFFFFFF; // m
  pctx_->GetGenRegPtr()[0x0b]=0x00000001;
  pctx_->SET_SR(0x00000E0);

  memSetWord( 0x06000000, 0x32b2 );  // cmphi 
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x000000E1, pctx_->GET_SR() );

}

TEST_F(CmphsTest, same) {
  pctx_->GetGenRegPtr()[2]=0xFFFFFFFF; // m
  pctx_->GetGenRegPtr()[0x0b]=0xFFFFFFFF;
  pctx_->SET_SR(0x00000E0);

  memSetWord( 0x06000000, 0x32b2 );  // cmphi 
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x000000E1, pctx_->GET_SR() );

}


}  // namespacegPtr
