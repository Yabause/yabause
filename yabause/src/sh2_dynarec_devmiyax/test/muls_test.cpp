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

class MulsTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  MulsTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~MulsTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(MulsTest, normal) {

  pctx_->GetGenRegPtr()[9]=0x0000001;
  pctx_->GetGenRegPtr()[0x0b]=0x000001f4;

  // macl r1, r3
  memSetWord( 0x06000000, 0x29bf );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x01f4, pctx_->GET_MACL() );

}




}  // namespace
