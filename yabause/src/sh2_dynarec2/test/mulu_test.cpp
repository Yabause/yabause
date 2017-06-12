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

class MuluTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  MuluTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~MuluTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(MuluTest, normal) {

  pctx_->GetGenRegPtr()[3]=0x0000000f;
  pctx_->GetGenRegPtr()[2]=0x00000018;

  // mulu r9, r3
  memSetWord( 0x06000000, 0x232e );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x0168, pctx_->GET_MACL() );

}


TEST_F(MuluTest, middle) {

  pctx_->GetGenRegPtr()[3]=0x00008000;
  pctx_->GetGenRegPtr()[2]=0x00008000;

  // mulu r9, r3
  memSetWord( 0x06000000, 0x232e );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x40000000, pctx_->GET_MACL() );

}

TEST_F(MuluTest, max) {

  pctx_->GetGenRegPtr()[3]=0x0000FFFF;
  pctx_->GetGenRegPtr()[2]=0x0000FFFF;

  // mulu r9, r3
  memSetWord( 0x06000000, 0x232e );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xfffe0001, pctx_->GET_MACL() );

}

TEST_F(MuluTest, ex) {

  pctx_->GetGenRegPtr()[3]=0xFFFFFFFF;
  pctx_->GetGenRegPtr()[2]=0xFFFFFFFF;

  // mulu r9, r3
  memSetWord( 0x06000000, 0x232e );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xfffe0001, pctx_->GET_MACL() );

}


}  // namespace
