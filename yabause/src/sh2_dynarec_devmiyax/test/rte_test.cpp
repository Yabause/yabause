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

class RteTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  RteTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~RteTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
   
}

virtual void TearDown() {
}

};

TEST_F(RteTest, normal) {

  pctx_->GetGenRegPtr()[15]=0x06001F68;
  memSetLong( 0x06001F68, 0x060107B6 );
  memSetLong( 0x06001F6C, 0x0 );
  pctx_->SET_SR(0);

  // rte
  memSetWord( 0x06000000, 0x002b );
  memSetWord( 0x06000002, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x060107B6, pctx_->GET_PC() );
  EXPECT_EQ( 0x00000000, pctx_->GET_SR() );
  EXPECT_EQ( 0x06001F70, pctx_->GetGenRegPtr()[15] );

}

TEST_F(RteTest, rts) {

  pctx_->SET_PR(0x06001000);

  pctx_->GetGenRegPtr()[7]=0xDEADDEAD;

  // rts
  memSetWord( 0x0600024c, 0x000b );
  memSetWord( 0x0600024e, 0xE743 );  //MOVI

  pctx_->SET_PC( 0x0600024c );
  pctx_->Execute();

  EXPECT_EQ( 0x00000043, pctx_->GetGenRegPtr()[7] );
  EXPECT_EQ( 0x06001000, pctx_->GET_PC() );
}

}  // namespace
