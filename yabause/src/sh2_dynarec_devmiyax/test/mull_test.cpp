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

class MullTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  MullTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~MullTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
  
}

virtual void TearDown() {

}

};

TEST_F(MullTest, normal) {

  pctx_->GetGenRegPtr()[1]=0x000000A;
  pctx_->GetGenRegPtr()[2]=0x001AF000;
  pctx_->SET_MACL(0xFFFFFFFF);
  pctx_->SET_MACH(0xDEADCAFE);

  // macl r1, r3
  memSetWord( 0x06000000, 0x0127 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x10D6000, pctx_->GET_MACL() );
  EXPECT_EQ( 0xDEADCAFE, pctx_->GET_MACH() );

  pctx_->GetGenRegPtr()[1]=0xFFFFFFFE;
  pctx_->GetGenRegPtr()[2]=0x00005555;
  pctx_->SET_MACL(0xDEADCAFE);
  pctx_->SET_MACH(0xDEADCAFE);

  // macl r1, r3
  memSetWord( 0x06000000, 0x0127 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xFFFF5556, pctx_->GET_MACL() );
  EXPECT_EQ( 0xDEADCAFE, pctx_->GET_MACH() );
}




}  // namespace
