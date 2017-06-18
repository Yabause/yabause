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

class MovlpTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  MovlpTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~MovlpTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
 
  
}

virtual void TearDown() {


}

};

TEST_F(MovlpTest, normal) {


  pctx_->GetGenRegPtr()[0]=0x0000030c;
  pctx_->GetGenRegPtr()[6]=0xe0000000;

  memSetLong(0x0000030c,0x04);

  // mova
  memSetWord( 0x0600024c, 0x6606 );
  memSetWord( 0x0600024e, 0x000b );  // rts
  memSetWord( 0x06000250, 0x0009 );  // nop

  pctx_->SET_PC( 0x0600024c );
  pctx_->Execute();

  EXPECT_EQ( 0x0000310, pctx_->GetGenRegPtr()[0] );
  EXPECT_EQ( 0x0000004, pctx_->GetGenRegPtr()[6] );
}

TEST_F(MovlpTest, samereg) {

    pctx_->GetGenRegPtr()[0]=0x0000030c;

    memSetLong(0x0000030c,0x04);

    // mova
    memSetWord( 0x0600024c, 0x6006 );
    memSetWord( 0x0600024e, 0x000b );  // rts
    memSetWord( 0x06000250, 0x0009 );  // nop

    pctx_->SET_PC( 0x0600024c );
    pctx_->Execute();

    EXPECT_EQ( 0x0000004, pctx_->GetGenRegPtr()[0] );
}




}  // namespace
