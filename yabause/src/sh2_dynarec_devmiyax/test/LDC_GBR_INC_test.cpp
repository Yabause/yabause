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

class LDC_GBR_INCTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  LDC_GBR_INCTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~LDC_GBR_INCTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
   
}

virtual void TearDown() {
}

};

TEST_F(LDC_GBR_INCTest, normal) {

  pctx_->GetGenRegPtr()[15]=0x06001F3C;
  memSetLong( 0x06001F3C, 0x25D00000 );
  pctx_->SET_SR(0);

  // ldc.l @r15+, gbr
  memSetWord( 0x06000000, 0x4F17 );
  memSetWord( 0x06000002, 0x000B );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x25D00000, pctx_->GET_GBR() );
  EXPECT_EQ( 0x06001F40, pctx_->GetGenRegPtr()[15] );

}





}  // namespace