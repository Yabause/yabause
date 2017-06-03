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

class DmulsTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  DmulsTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~DmulsTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(DmulsTest, normal) {

  pctx_->GetGenRegPtr()[1]=0x000ACA55;
  pctx_->GetGenRegPtr()[3]=0xFFFF90D9;

  // macl r1, r3
  memSetWord( 0x06000000, 0x313D );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x50A0520D, pctx_->GET_MACL() );
  EXPECT_EQ( 0xFFFFFFFB, pctx_->GET_MACH() );

}

TEST_F(DmulsTest, normal_) {

    pctx_->GetGenRegPtr()[2]=0x017FAF00;
    pctx_->GetGenRegPtr()[4]=0x008D2F00;

    // dmul r1, r3
    memSetWord( 0x06000000, 0x324D );
    memSetWord( 0x06000002, 0x000b );  // rts
    memSetWord( 0x06000004, 0x0009 );  // nop

    pctx_->SET_PC( 0x06000000 );
    pctx_->Execute();

    EXPECT_EQ( 0xD4210000, pctx_->GET_MACL() );
    EXPECT_EQ( 0x0000D399, pctx_->GET_MACH() );

}


TEST_F(DmulsTest, normal_s) {

    pctx_->GetGenRegPtr()[4]=0xFFF5D04F;
    pctx_->GetGenRegPtr()[5]=0x0000056D;

    // dmul r4, r5
    memSetWord( 0x06000000, 0x345D );
    memSetWord( 0x06000002, 0x000b );  // rts
    memSetWord( 0x06000004, 0x0009 );  // nop

    pctx_->SET_PC( 0x06000000 );
    pctx_->Execute();

    EXPECT_EQ( 0xFFFFFFFF, pctx_->GET_MACH() );
    EXPECT_EQ( 0xC8BB3CA3, pctx_->GET_MACL() );

}

}  // namespace
