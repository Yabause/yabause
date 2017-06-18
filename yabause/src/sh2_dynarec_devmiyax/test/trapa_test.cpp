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

class TrappaTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  TrappaTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~TrappaTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
   
}

virtual void TearDown() {
}

};

TEST_F(TrappaTest, normal) {

  pctx_->GetGenRegPtr()[15]=0x06100000;
  pctx_->SET_SR(0x000000f0);
  pctx_->SET_PC(0x06014026);
  pctx_->SET_VBR(0x06000000);
 
  memSetLong( 0x060000C0, 0x060041d8 );

  // trapa
  memSetWord( 0x06014026, 0xc330 );
  
  pctx_->Execute();

  EXPECT_EQ( 0x060041d8, pctx_->GET_PC() );
  EXPECT_EQ( pctx_->GET_SR(), memGetLong(0x060ffffC) );
  EXPECT_EQ( 0x06014028, memGetLong(0x060ffff8) );
  EXPECT_EQ( 0x060ffff8, pctx_->GetGenRegPtr()[15] );

}





}  // namespace