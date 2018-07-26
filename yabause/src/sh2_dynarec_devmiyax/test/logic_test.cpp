#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"

namespace {

class LogicTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  LogicTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~LogicTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(LogicTest, and) {

  pctx_->GetGenRegPtr()[2]=0x00FF00FF; //source
  pctx_->GetGenRegPtr()[3]=0x007007e0; //dest

  memSetWord( 0x06000000, 0x2329 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x00FF00FF, pctx_->GetGenRegPtr()[2] );
  EXPECT_EQ( 0x007000e0, pctx_->GetGenRegPtr()[3] );
}

TEST_F(LogicTest, andi) {

  pctx_->GetGenRegPtr()[0]=0x00FF00F8; //source

  memSetWord( 0x06000000, 0xC929 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x28, pctx_->GetGenRegPtr()[0] );
}

TEST_F(LogicTest, andb) {

  pctx_->GetGenRegPtr()[0]=0x000000250; //source
  pctx_->SET_GBR(0x06000000); //source

  memSetWord( 0x06000000, 0xCD29 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop
  memSetLong( 0x06000250, 0xDEADCAFE );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x08ADCAFE, memGetLong(0x06000250) );
}

TEST_F(LogicTest, not) {

  pctx_->GetGenRegPtr()[0]=0xF0F0F0F0; //source

  memSetWord( 0x06000000, 0x6107 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xF0F0F0F0, pctx_->GetGenRegPtr()[0] );
  EXPECT_EQ( 0x0F0F0F0F, pctx_->GetGenRegPtr()[1] );
}

TEST_F(LogicTest, or) {
  pctx_->GetGenRegPtr()[0]=0xF0F0F2F0; //source
  pctx_->GetGenRegPtr()[1]=0xF7F4F0F0; //source

  memSetWord( 0x06000000, 0x210B );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xF0F0F2F0, pctx_->GetGenRegPtr()[0] );
  EXPECT_EQ( 0xF7F4F2F0, pctx_->GetGenRegPtr()[1] );
}

TEST_F(LogicTest, ori) {
  pctx_->GetGenRegPtr()[0]=0xF0F0F270; //source

  memSetWord( 0x06000000, 0xCB1B );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xF0F0F27B, pctx_->GetGenRegPtr()[0] );
}

TEST_F(LogicTest, origbr) {
  pctx_->GetGenRegPtr()[0]=0x00000250; //source
  pctx_->SET_GBR(0x06000000); //source

  memSetWord( 0x06000000, 0xCF1B );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop
  memSetLong( 0x06000250, 0xDEADCAFE );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xDFADCAFE, memGetLong(0x06000250) );
}

TEST_F(LogicTest, tasb) {
  pctx_->GetGenRegPtr()[1]=0x06000250; //source
  pctx_->SET_SR(0xE0); //source

  memSetWord( 0x06000000, 0x411B );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop
  memSetLong( 0x06000250, 0x00ADDEAD );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x80ADDEAD, memGetLong(0x06000250) );
  EXPECT_EQ( 0xE1, pctx_->GET_SR() );

  pctx_->GetGenRegPtr()[1]=0x06000250; //source
  pctx_->SET_SR(0xE1); //source

  memSetWord( 0x06000000, 0x411B );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop
  memSetLong( 0x06000250, 0x07ADDEAD );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x87ADDEAD, memGetLong(0x06000250) );
  EXPECT_EQ( 0xE0, pctx_->GET_SR() );
}

TEST_F(LogicTest, tst) {
  pctx_->GetGenRegPtr()[0]=0x00000250; //source
  pctx_->GetGenRegPtr()[1]=0x00FF0400; //source
  pctx_->SET_SR(0xE0);

  memSetWord( 0x06000000, 0x2108 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xE1, pctx_->GET_SR() );

  pctx_->GetGenRegPtr()[0]=0x00000250; //source
  pctx_->GetGenRegPtr()[1]=0x00FF0600; //source
  pctx_->SET_SR(0xE1);

  memSetWord( 0x06000000, 0x2108 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xE0, pctx_->GET_SR() );
}

TEST_F(LogicTest, tsti) {
  pctx_->GetGenRegPtr()[0]=0x00FF0442; //source
  pctx_->SET_SR(0xE0);

  memSetWord( 0x06000000, 0xC825 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xE1, pctx_->GET_SR() );

  pctx_->GetGenRegPtr()[0]=0x00FF0442; //source
  pctx_->SET_SR(0xE1);

  memSetWord( 0x06000000, 0xC822 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xE0, pctx_->GET_SR() );
}

TEST_F(LogicTest, tstb) {
  pctx_->GetGenRegPtr()[0]=0x00000250; //source
  pctx_->SET_GBR(0x06000000); //source
  pctx_->SET_SR(0xE0);

  memSetWord( 0x06000000, 0xCC25 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop
  memSetLong( 0x06000250, 0x42ADDEAD );

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xE1, pctx_->GET_SR() );
  EXPECT_EQ( 0x42ADDEAD, memGetLong(0x06000250) );

  pctx_->GetGenRegPtr()[0]=0x00000250; //source
  pctx_->SET_GBR(0x06000000); //source
  pctx_->SET_SR(0xE1);

  memSetWord( 0x06000000, 0xCC25 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop
  memSetLong( 0x06000250, 0x62ADDEAD );

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xE0, pctx_->GET_SR() );
  EXPECT_EQ( 0x62ADDEAD, memGetLong(0x06000250) );
}

TEST_F(LogicTest, xor) {
  pctx_->GetGenRegPtr()[0]=0xF0F0F2F0; //source
  pctx_->GetGenRegPtr()[1]=0xF7F4F0F0; //source

  memSetWord( 0x06000000, 0x210A );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xF0F0F2F0, pctx_->GetGenRegPtr()[0] );
  EXPECT_EQ( 0x07040200, pctx_->GetGenRegPtr()[1] );
}

TEST_F(LogicTest, xori) {
  pctx_->GetGenRegPtr()[0]=0xF0F0F2F0; //source

  memSetWord( 0x06000000, 0xCA1B );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xF0F0F2EB, pctx_->GetGenRegPtr()[0] );

  pctx_->GetGenRegPtr()[0]=0xF0F0F2F0; //source

  memSetWord( 0x06000000, 0xCA9B );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xF0F0F26B, pctx_->GetGenRegPtr()[0] );
}

TEST_F(LogicTest, xorigbr) {
  pctx_->GetGenRegPtr()[0]=0x00000250; //source
  pctx_->SET_GBR(0x06000000); //source

  memSetWord( 0x06000000, 0xCE1B );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop
  memSetLong( 0x06000250, 0xF0F0F2F0 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xEBF0F2F0, memGetLong(0x06000250) );
}

}  // namespacegPtr
