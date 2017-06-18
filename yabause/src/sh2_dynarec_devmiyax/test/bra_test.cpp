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

class BraTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  BraTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~BraTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
   
}

virtual void TearDown() {
}

};

TEST_F(BraTest, normal) {


  // BRA
  memSetWord( 0x06002E00, 0xA023 );
  memSetWord( 0x06002E02, 0x0009 );  // nop

  pctx_->SET_PC( 0x06002E00 );
  pctx_->Execute();

  EXPECT_EQ( 0x06002E4A, pctx_->GET_PC() );

}





}  // namespace
