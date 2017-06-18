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

class XoriTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  XoriTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~XoriTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
 
  
}

virtual void TearDown() {


}

};

TEST_F(XoriTest, normal) {


  pctx_->GetGenRegPtr()[0]=0x00000006;

  // xori
  memSetWord( 0x06000000, 0xca04 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x02, pctx_->GetGenRegPtr()[0]  );

}

TEST_F(XoriTest, max) {


  pctx_->GetGenRegPtr()[0]=0xFFFFFFFF;

  // xori
  memSetWord( 0x06000000, 0xca01 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xfffffffe, pctx_->GetGenRegPtr()[0]  );

}

}  // namespace