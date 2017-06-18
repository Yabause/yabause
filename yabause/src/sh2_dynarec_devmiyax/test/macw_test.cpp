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

class MacwTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  MacwTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~MacwTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
 
  
}

virtual void TearDown() {


}

};

TEST_F(MacwTest, normal) {

  pctx_->GetGenRegPtr()[15]=0x06000fc0;
  pctx_->SET_MACH(0xffffff1b);
  pctx_->SET_MACL(0xffff23fe);

  memSetWord( 0x06000fc0, 0xff3b );
  memSetWord( 0x06000fc2, 0x00ac );

  // mac.l @r7+, @r6+
  memSetWord( 0x06000000, 0x4fff );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xffffff1b, pctx_->GET_MACH() );
  EXPECT_EQ( 0xfffe9fa2, pctx_->GET_MACL() );

}

TEST_F(MacwTest, s) {

  pctx_->GetGenRegPtr()[15]=0x06000fc0;
  pctx_->SET_MACH(0xffffff1b);
  pctx_->SET_MACL(0xffff23fe);
  pctx_->SET_SR(0x00000002);

  memSetWord( 0x06000fc0, 0xff3b );
  memSetWord( 0x06000fc2, 0x00ac );

  // mac.l @r7+, @r6+
  memSetWord( 0x06000000, 0x4fff );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xffffff1b, pctx_->GET_MACH() );
  EXPECT_EQ( 0xfffe9fa2, pctx_->GET_MACL() );

}


}  // namespace
