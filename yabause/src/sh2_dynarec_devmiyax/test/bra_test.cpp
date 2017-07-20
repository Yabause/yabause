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
  memSetWord( 0x06002E00, 0xA123 );
  memSetWord( 0x06002E02, 0x0009 );  // nop

  pctx_->SET_PC( 0x06002E00 );
  pctx_->Execute();

  EXPECT_EQ( 0x0600304A, pctx_->GET_PC() );

  // BRA
  memSetWord( 0x00000220, 0xA015 );
  memSetWord( 0x00000222, 0x277A );  // nop

  pctx_->SET_PC( 0x00000220 );
  pctx_->Execute();

  EXPECT_EQ( 0x0000024E, pctx_->GET_PC() );

}

TEST_F(BraTest, negative) {
  // BRA
  memSetWord( 0x06002E00, 0xAFF5 );
  memSetWord( 0x06002E02, 0x0009 );  // nop

  pctx_->SET_PC( 0x06002E00 );
  pctx_->Execute();

  EXPECT_EQ( 0x06002DEE, pctx_->GET_PC() );
}

TEST_F(BraTest, braf) {
  // BRAF
  pctx_->GetGenRegPtr()[1]=0x00106520; //source

  memSetWord( 0x06002E00, 0x0123 );
  memSetWord( 0x06002E02, 0x0009 );  // nop

  pctx_->SET_PC( 0x06002E00 );
  pctx_->Execute();

  EXPECT_EQ( 0x06109324, pctx_->GET_PC() );
}

TEST_F(BraTest, braf_negative) {
  // BRAF
  pctx_->GetGenRegPtr()[1]=0xFFFFFFFC; //source

  memSetWord( 0x06002E00, 0x0123 );
  memSetWord( 0x06002E02, 0x0009 );  // nop

  pctx_->SET_PC( 0x06002E00 );
  pctx_->Execute();

  EXPECT_EQ( 0x06002E00, pctx_->GET_PC() );
}

}  // namespace
