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

class MovaTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  MovaTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~MovaTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
 
  
}

virtual void TearDown() {


}

};

TEST_F(MovaTest, normal) {


  pctx_->GetGenRegPtr()[0]=0x00000000;


  // mova
  memSetWord( 0x0600024c, 0xc72f );
  memSetWord( 0x0600024e, 0x000b );  // rts
  memSetWord( 0x06000250, 0x0009 );  // nop

  pctx_->SET_PC( 0x0600024c );
  pctx_->Execute();

  EXPECT_EQ( 0x0600030c, pctx_->GetGenRegPtr()[0] );

}



}  // namespace
