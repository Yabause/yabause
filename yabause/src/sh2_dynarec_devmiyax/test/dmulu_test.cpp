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

class DmuluTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  DmuluTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~DmuluTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(DmuluTest, normal) {

  pctx_->GetGenRegPtr()[4]=0x00000002;
  pctx_->GetGenRegPtr()[7]=0xcccccccd;

  // dmulu r1, r3
  memSetWord( 0x06000000, 0x3475 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x9999999a, pctx_->GET_MACL() );
  EXPECT_EQ( 0x00000001, pctx_->GET_MACH() );

}


}  // namespace
