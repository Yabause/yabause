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

class XtractTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  XtractTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~XtractTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
  printf("XtractTest::SetUp\n");
  
}

virtual void TearDown() {
  printf("XtractTest::TearDown\n");

}

};

TEST_F(XtractTest, normal) {

  pctx_->GetGenRegPtr()[0]=0x00000000;
  pctx_->GetGenRegPtr()[1]=0x00000001;

  // xtract r1,r0
  memSetWord( 0x06000000, 0x201D );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x00010000, pctx_->GetGenRegPtr()[0] );

}

TEST_F(XtractTest, normal2) {

  pctx_->GetGenRegPtr()[3]=0x00000003;
  pctx_->GetGenRegPtr()[0]=0x6631C000;

  // xtract r3,r0
  memSetWord( 0x06000000, 0x203D );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x00036631, pctx_->GetGenRegPtr()[0] );

}



}  // namespace