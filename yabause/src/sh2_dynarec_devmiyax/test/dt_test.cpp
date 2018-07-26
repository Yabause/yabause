#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"

//SH2_struct *CurrentSH2;
//yabsys_struct yabsys;

// arevoir

namespace {

class DtTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  DtTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~DtTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
 
  
}

virtual void TearDown() {


}

};

TEST_F(DtTest, normal) {

  pctx_->GetGenRegPtr()[2]=0x2;

  memSetWord( 0x06000246, 0x4210 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x0600024C, 0xDEADCAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->SET_SR( 0x0 );
  pctx_->Execute();

 EXPECT_EQ( 0x1, pctx_->GetGenRegPtr()[2] );
 EXPECT_EQ( 0x0, pctx_->GET_SR() );

  pctx_->GetGenRegPtr()[2]=0x2;

  memSetWord( 0x06000246, 0x4210 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x0600024C, 0xDEADCAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->SET_SR( 0x1 );
  pctx_->Execute();

 EXPECT_EQ( 0x1, pctx_->GetGenRegPtr()[2] );
 EXPECT_EQ( 0x0, pctx_->GET_SR() );

  pctx_->GetGenRegPtr()[2]=0x1;

  memSetWord( 0x06000246, 0x4210 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x0600024C, 0xDEADCAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->SET_SR( 0x0 );
  pctx_->Execute();

 EXPECT_EQ( 0x0, pctx_->GetGenRegPtr()[2] );
 EXPECT_EQ( 0x1, pctx_->GET_SR() );

  pctx_->GetGenRegPtr()[2]=0xF0001;

  memSetWord( 0x06000246, 0x4210 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x0600024C, 0xDEADCAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->SET_SR( 0x1 );
  pctx_->Execute();

 EXPECT_EQ( 0xF0000, pctx_->GetGenRegPtr()[2] );
 EXPECT_EQ( 0x0, pctx_->GET_SR() );

}

TEST_F(DtTest, negative) {
  pctx_->GetGenRegPtr()[2]=-256;

  memSetWord( 0x06000246, 0x4210 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x0600024C, 0xDEADCAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->SET_SR( 0x0 );
  pctx_->Execute();

 EXPECT_EQ( -257, pctx_->GetGenRegPtr()[2] );
 EXPECT_EQ( 0x0, pctx_->GET_SR() );
}

TEST_F(DtTest, overflow) {
  pctx_->GetGenRegPtr()[2]=0x80000000;

  memSetWord( 0x06000246, 0x4210 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x0600024C, 0xDEADCAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->SET_SR( 0x0 );
  pctx_->Execute();

 EXPECT_EQ( 0x7fffffff, pctx_->GetGenRegPtr()[2] );
 EXPECT_EQ( 0x0, pctx_->GET_SR() );
}

}  // namespace
