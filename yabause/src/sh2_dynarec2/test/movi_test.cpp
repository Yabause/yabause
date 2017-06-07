#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"


namespace {

class MoviTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  MoviTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~MoviTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
 
  
}

virtual void TearDown() {


}

};

TEST_F(MoviTest, normal) {


  pctx_->GetGenRegPtr()[0]=0x0000006d;


  // mova
  memSetWord( 0x0600024c, 0xe06e );
  memSetWord( 0x0600024e, 0x000b );  // rts
  memSetWord( 0x06000250, 0x0009 );  // nop

  pctx_->SET_PC( 0x0600024c );
  pctx_->Execute();

  EXPECT_EQ( 0x0000006e, pctx_->GetGenRegPtr()[0] );

}


TEST_F(MoviTest, m) {


  pctx_->GetGenRegPtr()[0]=0x0000006d;


  // mova
  memSetWord( 0x0600024c, 0xe081 );
  memSetWord( 0x0600024e, 0x000b );  // rts
  memSetWord( 0x06000250, 0x0009 );  // nop

  pctx_->SET_PC( 0x0600024c );
  pctx_->Execute();

  EXPECT_EQ( 0xFFFFFF81, pctx_->GetGenRegPtr()[0] );

}

}  // namespace
