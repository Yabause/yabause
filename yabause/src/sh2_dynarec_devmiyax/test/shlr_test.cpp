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

class ShlrTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  ShlrTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~ShlrTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
   
}

virtual void TearDown() {
}

};

TEST_F(ShlrTest, normal) {

  pctx_->GetGenRegPtr()[2]=0x00000001;
  pctx_->SET_SR(0);

  // shlr2
  memSetWord( 0x06000000, 0x4201 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x00000000, pctx_->GetGenRegPtr()[2] );
  EXPECT_EQ( 0x00000001, pctx_->GET_SR() );

}





}  // namespace