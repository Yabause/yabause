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

class NegcTest : public ::testing::Test {
 protected:
   DynarecSh2 * pctx_;

  NegcTest() {
    initMemory();
    pctx_ = new DynarecSh2();  
    pctx_->SetCurrentContext();
  }

  virtual ~NegcTest() {
    delete pctx_;    
  }   

virtual void SetUp() {
 
  
}

virtual void TearDown() {


}

};

TEST_F(NegcTest, normal) {
/*! Sign inversion of r0:r1 (64 bits)

clrt
negc   r1,r1    ! Before execution: r1 = 0x00000001, T = 0
                ! After execution: r1 = 0xFFFFFFFF, T = 1
negc   r0,r0    ! Before execution: r0 = 0x00000000, T = 1
                ! After execution: r0 = 0xFFFFFFFF, T = 1

- - - - - - - - - - - - - - - - 

! Store reversed T bit in r0

mov    #-1,r1
negc   r1,r0    ! r0 = 0 - (-1) - T
                ! r0 = 1 - T
                ! Notice that T bit will be modified by the negc operation.
                ! In this case, T will be always set to 1.
*/

  pctx_->GetGenRegPtr()[1]=0x00000001;
  pctx_->SET_SR(0xE0);

  memSetWord( 0x06000000, 0x611A );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();

  EXPECT_EQ( 0xFFFFFFFF, pctx_->GetGenRegPtr()[1] );
  EXPECT_EQ( 0xE1, pctx_->GET_SR() );

  pctx_->GetGenRegPtr()[0]=0x00000000;
  pctx_->SET_SR(0xE1);

  memSetWord( 0x06000000, 0x600A );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );

  pctx_->Execute();

  EXPECT_EQ( 0xFFFFFFFF, pctx_->GetGenRegPtr()[0] );
  EXPECT_EQ( 0xE1, pctx_->GET_SR() );

  pctx_->GetGenRegPtr()[1]=0xFFFFFFFF;
  pctx_->SET_SR(0xE0);

  memSetWord( 0x06000000, 0x601A );

  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );

  pctx_->Execute();

  EXPECT_EQ( 0xFFFFFFFF, pctx_->GetGenRegPtr()[1] );
  EXPECT_EQ( 0x1, pctx_->GetGenRegPtr()[0] );
  EXPECT_EQ( 0xE1, pctx_->GET_SR() );

  pctx_->GetGenRegPtr()[1]=0xFFFFFFFF;
  pctx_->SET_SR(0xE1);

  memSetWord( 0x06000000, 0x601A );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();
  EXPECT_EQ( 0xFFFFFFFF, pctx_->GetGenRegPtr()[1] );
  EXPECT_EQ( 0x0, pctx_->GetGenRegPtr()[0] );
  EXPECT_EQ( 0xE1, pctx_->GET_SR() );

  pctx_->GetGenRegPtr()[1]=0x1;

  memSetWord( 0x06000000, 0x621B );
  memSetWord( 0x06000002, 0x000b );  // rts
  memSetWord( 0x06000004, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000000 );
  pctx_->Execute();
  EXPECT_EQ( 0xFFFFFFFF, pctx_->GetGenRegPtr()[2] );

}

}  // namespace
