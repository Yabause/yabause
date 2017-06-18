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

class TasTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  TasTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~TasTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
 
  
}

virtual void TearDown() {


}

};

TEST_F(TasTest, normal) {

//0602E3C2: MACL R[6]=06001F00@FFFBF400,R[7]=06030060@FFFF1CC5,MACH=00000003,MACL=9792C400
//0602E3C4: MACL R[6]=06001F04@FFFB60F6,R[7]=06030064@00007421,MACH=00000001,MACL=7EE9BBB6

  pctx_->GetGenRegPtr()[3]=0x060fffb8;
  pctx_->SET_SR(0x00000000);

  memSetByte( 0x060fffb8, 0 );

  // tas.b R[3]
  memSetWord( 0x06000000, 0x431b );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0x80, memGetByte( 0x060fffb8) );
  EXPECT_EQ( 0x00000001, pctx_->GET_SR() );

}

}  // namespace