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

class OrbTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  OrbTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~OrbTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
 
  
}

virtual void TearDown() {


}

};

TEST_F(OrbTest, normal) {


  pctx_->GetGenRegPtr()[0]=0x00000001;
  pctx_->SET_GBR(0x060ffcbb);

  memSetByte( 0x060ffcbc, 0x08 );

  // or.b #6
  memSetWord( 0x06000000, 0xcf06 );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x0e, memGetByte( 0x060ffcbc) );

}


TEST_F(OrbTest, max) {


  pctx_->GetGenRegPtr()[0]=0x00000001;
  pctx_->SET_GBR(0x060ffcbb);

  memSetByte( 0x060ffcbc, 0x08 );

  // or.b #6
  memSetWord( 0x06000000, 0xcfff );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xff, memGetByte( 0x060ffcbc) );

}

}  // namespace