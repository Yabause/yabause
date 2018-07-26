#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"

//SH2_struct *CurrentSH2;
//yabsys_struct yabsys;

namespace {

class StsTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  StsTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~StsTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
 
  
}

virtual void TearDown() {


}

};

TEST_F(StsTest, normal) {


  pctx_->GetGenRegPtr()[3]=0x06000264;
  pctx_->SET_PR(0xCAFECAFE);


  // sts.l
  memSetWord( 0x0600024c, 0x4322 );
  memSetWord( 0x0600024e, 0x000b );  // rts
  memSetWord( 0x06000250, 0x0009 );  // nop
  memSetLong( 0x06000260, 0xDEADDEAD );  // nop

  pctx_->SET_PC( 0x0600024c );
  pctx_->Execute();

  EXPECT_EQ( 0x06000260, pctx_->GetGenRegPtr()[3] );
  EXPECT_EQ( 0xCAFECAFE, memGetLong(0x06000260) );

}


TEST_F(StsTest, stsmach) {

  pctx_->GetGenRegPtr()[3]=0x06000264;
  pctx_->SET_MACH(0xCAFECAFE);

  memSetWord( 0x0600024c, 0x030A );
  memSetWord( 0x0600024e, 0x000b );  // rts
  memSetWord( 0x06000250, 0x0009 );  // nop
  memSetLong( 0x06000260, 0xDEADDEAD );  // nop

  pctx_->SET_PC( 0x0600024c );
  pctx_->Execute();

  EXPECT_EQ( 0xCAFECAFE, pctx_->GetGenRegPtr()[3] );

}

TEST_F(StsTest, stsmmach) {

  pctx_->GetGenRegPtr()[3]=0x06000264;
  pctx_->SET_MACH(0xCAFECAFE);

  memSetWord( 0x0600024c, 0x4302 );
  memSetWord( 0x0600024e, 0x000b );  // rts
  memSetWord( 0x06000250, 0x0009 );  // nop
  memSetLong( 0x06000260, 0xDEADDEAD );  // nop

  pctx_->SET_PC( 0x0600024c );
  pctx_->Execute();

  EXPECT_EQ( 0x06000260, pctx_->GetGenRegPtr()[3] );
  EXPECT_EQ( 0xCAFECAFE, memGetLong(0x06000260) );

}

TEST_F(StsTest, stsmacl) {

  pctx_->GetGenRegPtr()[3]=0x06000264;
  pctx_->SET_MACL(0xCAFECAFE);

  memSetWord( 0x0600024c, 0x031A );
  memSetWord( 0x0600024e, 0x000b );  // rts
  memSetWord( 0x06000250, 0x0009 );  // nop
  memSetLong( 0x06000260, 0xDEADDEAD );  // nop

  pctx_->SET_PC( 0x0600024c );
  pctx_->Execute();

  EXPECT_EQ( 0xCAFECAFE, pctx_->GetGenRegPtr()[3] );

}

TEST_F(StsTest, stsmmacl) {

  pctx_->GetGenRegPtr()[3]=0x06000264;
  pctx_->SET_MACL(0xCAFECAFE);

  memSetWord( 0x0600024c, 0x4312 );
  memSetWord( 0x0600024e, 0x000b );  // rts
  memSetWord( 0x06000250, 0x0009 );  // nop
  memSetLong( 0x06000260, 0xDEADDEAD );  // nop

  pctx_->SET_PC( 0x0600024c );
  pctx_->Execute();

  EXPECT_EQ( 0x06000260, pctx_->GetGenRegPtr()[3] );
  EXPECT_EQ( 0xCAFECAFE, memGetLong(0x06000260) );

}

TEST_F(StsTest, stspr) {

  pctx_->GetGenRegPtr()[3]=0x06000264;
  pctx_->SET_PR(0xCAFECAFE);

  memSetWord( 0x0600024c, 0x032A );
  memSetWord( 0x0600024e, 0x000b );  // rts
  memSetWord( 0x06000250, 0x0009 );  // nop
  memSetLong( 0x06000260, 0xDEADDEAD );  // nop

  pctx_->SET_PC( 0x0600024c );
  pctx_->Execute();

  EXPECT_EQ( 0xCAFECAFE, pctx_->GetGenRegPtr()[3] );

}

TEST_F(StsTest, stsmpr) {

  pctx_->GetGenRegPtr()[3]=0x06000264;
  pctx_->SET_PR(0xCAFECAFE);

  memSetWord( 0x0600024c, 0x4322 );
  memSetWord( 0x0600024e, 0x000b );  // rts
  memSetWord( 0x06000250, 0x0009 );  // nop
  memSetLong( 0x06000260, 0xDEADDEAD );  // nop

  pctx_->SET_PC( 0x0600024c );
  pctx_->Execute();

  EXPECT_EQ( 0x06000260, pctx_->GetGenRegPtr()[3] );
  EXPECT_EQ( 0xCAFECAFE, memGetLong(0x06000260) );

}

}  // namespace
