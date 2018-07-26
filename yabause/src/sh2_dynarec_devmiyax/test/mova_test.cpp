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

TEST_F(MovaTest, mova) {


  pctx_->GetGenRegPtr()[0]=0x00000000;


  // mova
  memSetWord( 0x0600024c, 0xc72f );
  memSetWord( 0x0600024e, 0x000b );  // rts
  memSetWord( 0x06000250, 0x0009 );  // nop

  pctx_->SET_PC( 0x0600024c );
  pctx_->Execute();

  EXPECT_EQ( 0x0600030c, pctx_->GetGenRegPtr()[0] );


  pctx_->GetGenRegPtr()[0]=0xFFFFFFFF;
  // mova
  memSetWord( 0x0600024a, 0xc78f );
  memSetWord( 0x0600024c, 0x000b );  // rts
  memSetWord( 0x0600024e, 0x0009 );  // nop

  pctx_->SET_PC( 0x0600024a );
  pctx_->Execute();

  EXPECT_EQ( 0x06000488, pctx_->GetGenRegPtr()[0] );

}

TEST_F(MovaTest, mov) {

  pctx_->GetGenRegPtr()[7]=0xDEADDEAD;
  pctx_->GetGenRegPtr()[4]=0xCAFECAFE;

  memSetWord( 0x0600024c, 0x6743 );  //MOVLI
  memSetWord( 0x0600024e, 0x000b );  // rts
  memSetWord( 0x06000250, 0x0009 );  // nop

  memSetWord( 0x06000270, 0xDEAD );
  memSetWord( 0x06000272 ,0xCAFE);

  pctx_->SET_PC( 0x0600024c );
  pctx_->Execute();

 EXPECT_EQ( 0xCAFECAFE, pctx_->GetGenRegPtr()[7] );
 EXPECT_EQ( 0xCAFECAFE, pctx_->GetGenRegPtr()[4] );
}

TEST_F(MovaTest, movi) {

  pctx_->GetGenRegPtr()[7]=0xDEADDEAD;

  memSetWord( 0x0600024c, 0xE743 );  //MOVI
  memSetWord( 0x0600024e, 0x000b );  // rts
  memSetWord( 0x06000250, 0x0009 );  // nop

  memSetWord( 0x06000270, 0xDEAD );
  memSetWord( 0x06000272 ,0xCAFE);

  pctx_->SET_PC( 0x0600024c );
  pctx_->Execute();

  EXPECT_EQ( 0x00000043, pctx_->GetGenRegPtr()[7] );

  pctx_->GetGenRegPtr()[7]=0xDEADDEAD;

  memSetWord( 0x0600024c, 0xE783 );  //MOVI
  memSetWord( 0x0600024e, 0x000b );  // rts
  memSetWord( 0x06000250, 0x0009 );  // nop

  memSetWord( 0x06000270, 0xDEAD );
  memSetWord( 0x06000272 ,0xCAFE);

  pctx_->SET_PC( 0x0600024c );
  pctx_->Execute();

  EXPECT_EQ( 0xFFFFFF83, pctx_->GetGenRegPtr()[7] );

}

TEST_F(MovaTest, movli) {

  pctx_->GetGenRegPtr()[7]=0xFFFFFFFF;

  memSetWord( 0x0600024c, 0xD708 );  //MOVLI
  memSetWord( 0x0600024e, 0x000b );  // rts
  memSetWord( 0x06000250, 0x0009 );  // nop

  memSetWord( 0x06000270, 0xDEAD );
  memSetWord( 0x06000272 ,0xCAFE);

  pctx_->SET_PC( 0x0600024c );
  pctx_->Execute();

 EXPECT_EQ( 0xDEADCAFE, pctx_->GetGenRegPtr()[7] );

//MOVLI

  pctx_->GetGenRegPtr()[0]=0xFFFFFFFF;

  memSetWord( 0x06000246, 0xD003 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop
  memSetLong( 0x06000256, 0xDEADDEAD ); 
  memSetLong( 0x06000254, 0xCAFECAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0xCAFECAFE, pctx_->GetGenRegPtr()[0] );

  pctx_->GetGenRegPtr()[0]=0xFFFFFFFF;

  memSetWord( 0x06000246, 0xD083 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop
  memSetLong( 0x06000456, 0xDEADDEAD ); 
  memSetLong( 0x06000454, 0xCAFECAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0xCAFECAFE, pctx_->GetGenRegPtr()[0] );
}

TEST_F(MovaTest, movll) {

//MOVLL

  pctx_->GetGenRegPtr()[7]=0x06000270;
  pctx_->GetGenRegPtr()[6]=0xFFFFFFFF;

  memSetWord( 0x0600024c, 0x6672 );  //MOVL_MEM_REG
  memSetWord( 0x0600024e, 0x000b );  // rts
  memSetWord( 0x06000250, 0x0009 );  // nop

  memSetWord( 0x06000270, 0xDEAD );
  memSetWord( 0x06000272 ,0xCAFE);

  pctx_->SET_PC( 0x0600024c );
  pctx_->Execute();

 EXPECT_EQ( 0xDEADCAFE, pctx_->GetGenRegPtr()[6] );
 EXPECT_EQ( 0x06000270, pctx_->GetGenRegPtr()[7] );
}

TEST_F(MovaTest, movbs) {
//MOVBS
  pctx_->GetGenRegPtr()[0]=0x0600024C;
  pctx_->GetGenRegPtr()[3]=0xDEADCAFE;

  memSetWord( 0x06000246, 0x2030);  
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop
  memSetLong( 0x0600024C, 0xFFFFFFFF ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0xFE, memGetByte(0x0600024C) );
 EXPECT_EQ( 0xFEFFFFFF, memGetLong(0x0600024C) );


  pctx_->GetGenRegPtr()[0]=0xFFFFFFFF;

  memSetWord( 0x06000246, 0xC71C );  //MOVL_MEM_REG
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( (0x06000246 & 0xFFFFFFFC)  + (0x1C << 2) + 4, pctx_->GetGenRegPtr()[0] );
}

TEST_F(MovaTest, movls) {
//MOVLS

  pctx_->GetGenRegPtr()[0]=0x0600024C;
  pctx_->GetGenRegPtr()[3]=0xDEADCAFE;

  memSetWord( 0x06000246, 0x2032 );  //MOVLS
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop
  memSetLong( 0x0600024C, 0xFFFFFFFF ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0xDEADCAFE, memGetLong(0x0600024C) );
}

TEST_F(MovaTest, movbp) {
//MOVBP
  pctx_->GetGenRegPtr()[1]=0x0600024C;
  pctx_->GetGenRegPtr()[7]=0xDEADCAFE;

  memSetWord( 0x06000246, 0x6714 );
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop
  memSetLong( 0x0600024C, 0xCAFEDEAD );  // nop

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0xFFFFFFCA, pctx_->GetGenRegPtr()[7] );
 EXPECT_EQ( 0x0600024D, pctx_->GetGenRegPtr()[1] );

  pctx_->GetGenRegPtr()[1]=0x0600024C;
  pctx_->GetGenRegPtr()[7]=0xDEADCAFE;

  memSetWord( 0x06000246, 0x6714 );
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop
  memSetLong( 0x0600024C, 0x7AFEDEAD );  // nop

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0x0000007A, pctx_->GetGenRegPtr()[7] );
 EXPECT_EQ( 0x0600024D, pctx_->GetGenRegPtr()[1] );
}

TEST_F(MovaTest, movwp) {
//MOVWP

  pctx_->GetGenRegPtr()[1]=0x0600024C;
  pctx_->GetGenRegPtr()[7]=0xDEADCAFE;

  memSetWord( 0x06000246, 0x6715 );
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop
  memSetLong( 0x0600024C, 0xCAFEDEAD );  // nop

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0xFFFFCAFE, pctx_->GetGenRegPtr()[7] );
 EXPECT_EQ( 0x0600024E, pctx_->GetGenRegPtr()[1] );


  pctx_->GetGenRegPtr()[1]=0x0600024C;
  pctx_->GetGenRegPtr()[7]=0xDEADCAFE;

  memSetWord( 0x06000246, 0x6715 );
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop
  memSetLong( 0x0600024C, 0x7AFEDEAD );  // nop

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0x00007AFE, pctx_->GetGenRegPtr()[7] );
 EXPECT_EQ( 0x0600024E, pctx_->GetGenRegPtr()[1] );
}

TEST_F(MovaTest, movlp) {
//MOVLP

  pctx_->GetGenRegPtr()[0]=0x0600024C;
  pctx_->GetGenRegPtr()[4]=0xFFFFFFFF;

  memSetWord( 0x06000246, 0x6406 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x0600024C, 0xDEADCAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0x0600024C + 0x4, pctx_->GetGenRegPtr()[0] );
 EXPECT_EQ( 0xDEADCAFE, pctx_->GetGenRegPtr()[4] );

  pctx_->GetGenRegPtr()[0]=0x0600024C;

  memSetWord( 0x06000246, 0x6006 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x0600024C, 0xDEADCAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0xDEADCAFE, pctx_->GetGenRegPtr()[0] );
}

TEST_F(MovaTest, movbm) {
//MOVBM

  pctx_->GetGenRegPtr()[1]=0x0600024D;
  pctx_->GetGenRegPtr()[7]=0xDEADEAD;

  memSetWord( 0x06000246, 0x2174 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x0600024C, 0xCAFECAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0x0600024C, pctx_->GetGenRegPtr()[1] );
 EXPECT_EQ( 0xADFECAFE, memGetLong(0x0600024C) );
 EXPECT_EQ( 0xAD, memGetByte(0x0600024C) );
}

TEST_F(MovaTest, movwm) {
//MOVWM

  pctx_->GetGenRegPtr()[1]=0x0600024E;
  pctx_->GetGenRegPtr()[7]=0xDEADEAD;

  memSetWord( 0x06000246, 0x2175 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x0600024C, 0xCAFECAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0x0600024C, pctx_->GetGenRegPtr()[1] );
 EXPECT_EQ( 0xDEADCAFE, memGetLong(0x0600024C) );
 EXPECT_EQ( 0xDEAD, memGetWord(0x0600024C) );
}

TEST_F(MovaTest, movlm) {
//MOVLM

  pctx_->GetGenRegPtr()[1]=0x06000250;
  pctx_->GetGenRegPtr()[2]=0xDEADCAFE;

  memSetWord( 0x06000246, 0x2126 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x0600024C, 0xFFFFFFFF ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0xDEADCAFE, memGetLong(0x0600024C) );
 EXPECT_EQ( 0x0600024C, pctx_->GetGenRegPtr()[1] );
}

TEST_F(MovaTest, movbl4) {
//MOVBL4

  pctx_->GetGenRegPtr()[1]=0x06000246;

  memSetWord( 0x06000246, 0x841A ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x06000250, 0xDEADCAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0xFFFFFFDE, pctx_->GetGenRegPtr()[0] );
 EXPECT_EQ( 0x06000246, pctx_->GetGenRegPtr()[1] );

  pctx_->GetGenRegPtr()[1]=0x06000246;

  memSetWord( 0x06000246, 0x841A ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x06000250, 0x7EADCAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0x0000007E, pctx_->GetGenRegPtr()[0] );
 EXPECT_EQ( 0x06000246, pctx_->GetGenRegPtr()[1] );
}

TEST_F(MovaTest, movwl4) {
//MOVWL4

  pctx_->GetGenRegPtr()[1]=0x06000246;

  memSetWord( 0x06000246, 0x851A ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x0600025A, 0xDEADCAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0xFFFFDEAD, pctx_->GetGenRegPtr()[0] );
 EXPECT_EQ( 0x06000246, pctx_->GetGenRegPtr()[1] );

  pctx_->GetGenRegPtr()[1]=0x06000246;

  memSetWord( 0x06000246, 0x851A ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x0600025A, 0x7EADCAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0x00007EAD, pctx_->GetGenRegPtr()[0] );
 EXPECT_EQ( 0x06000246, pctx_->GetGenRegPtr()[1] );
}

TEST_F(MovaTest, movll4) {
//MOVLL4

  pctx_->GetGenRegPtr()[1]=0x06000246;
  pctx_->GetGenRegPtr()[2]=0xDEADDEAD;

  memSetWord( 0x06000246, 0x5215 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x0600025A, 0xCAFECAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0xCAFECAFE, pctx_->GetGenRegPtr()[2] );
 EXPECT_EQ( 0x06000246, pctx_->GetGenRegPtr()[1] );

  pctx_->GetGenRegPtr()[1]=0x06000246;
  pctx_->GetGenRegPtr()[2]=0xDEADDEAD;

  memSetWord( 0x06000246, 0x5218 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x06000266, 0xCAFECAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0xCAFECAFE, pctx_->GetGenRegPtr()[2] );
 EXPECT_EQ( 0x06000246, pctx_->GetGenRegPtr()[1] );
}

TEST_F(MovaTest, movbs4) {
//MOVBS4

  pctx_->GetGenRegPtr()[1]=0x06000246;
  pctx_->GetGenRegPtr()[0]=0xDEADDEAD;

  memSetWord( 0x06000246, 0x801A ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x06000250, 0xCAFECAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0xDEADDEAD, pctx_->GetGenRegPtr()[0] );
 EXPECT_EQ( 0x06000246, pctx_->GetGenRegPtr()[1] );
 EXPECT_EQ( 0xADFECAFE, memGetLong(0x06000250) );
 EXPECT_EQ( 0xAD, memGetByte(0x06000250) );
}

TEST_F(MovaTest, movws4) {
//MOVWS4

  pctx_->GetGenRegPtr()[1]=0x06000246;
  pctx_->GetGenRegPtr()[0]=0xDEADDEAD;

  memSetWord( 0x06000246, 0x8115 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x06000250, 0xCAFECAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0xDEADDEAD, pctx_->GetGenRegPtr()[0] );
 EXPECT_EQ( 0x06000246, pctx_->GetGenRegPtr()[1] );
 EXPECT_EQ( 0xDEADCAFE, memGetLong(0x06000250) );
 EXPECT_EQ( 0xDEAD, memGetWord(0x06000250) );
}

TEST_F(MovaTest, movls4) {
//MOVLS4

  pctx_->GetGenRegPtr()[1]=0x06000246;
  pctx_->GetGenRegPtr()[2]=0xDEADDEAD;

  memSetWord( 0x06000246, 0x1125 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x0600025A, 0xCAFECAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0xDEADDEAD, pctx_->GetGenRegPtr()[2] );
 EXPECT_EQ( 0x06000246, pctx_->GetGenRegPtr()[1] );
 EXPECT_EQ( 0xDEADDEAD, memGetLong(0x0600025A) );
}

TEST_F(MovaTest, movbl0) {
//MOVBL0

  pctx_->GetGenRegPtr()[0]=0x0000025A;
  pctx_->GetGenRegPtr()[1]=0x06000000;
  pctx_->GetGenRegPtr()[2]=0xDEADDEAD;

  memSetWord( 0x06000246, 0x021C ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x0600025A, 0xCAFECAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0xFFFFFFCA, pctx_->GetGenRegPtr()[2] );
 EXPECT_EQ( 0x06000000, pctx_->GetGenRegPtr()[1] );
 EXPECT_EQ( 0x0000025A, pctx_->GetGenRegPtr()[0] );

  pctx_->GetGenRegPtr()[0]=0x0000025A;
  pctx_->GetGenRegPtr()[1]=0x06000000;
  pctx_->GetGenRegPtr()[2]=0xDEADDEAD;

  memSetWord( 0x06000246, 0x021C ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x0600025A, 0x7AFECAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0x0000007A, pctx_->GetGenRegPtr()[2] );
 EXPECT_EQ( 0x06000000, pctx_->GetGenRegPtr()[1] );
 EXPECT_EQ( 0x0000025A, pctx_->GetGenRegPtr()[0] );
}

TEST_F(MovaTest, movwl0) {
//MOVWL0

  pctx_->GetGenRegPtr()[0]=0x0000025A;
  pctx_->GetGenRegPtr()[1]=0x06000000;
  pctx_->GetGenRegPtr()[2]=0xDEADDEAD;

  memSetWord( 0x06000246, 0x021D ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x0600025A, 0xCAFECAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0xFFFFCAFE, pctx_->GetGenRegPtr()[2] );
 EXPECT_EQ( 0x06000000, pctx_->GetGenRegPtr()[1] );
 EXPECT_EQ( 0x0000025A, pctx_->GetGenRegPtr()[0] );

  pctx_->GetGenRegPtr()[0]=0x0000025A;
  pctx_->GetGenRegPtr()[1]=0x06000000;
  pctx_->GetGenRegPtr()[2]=0xDEADDEAD;

  memSetWord( 0x06000246, 0x021D ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x0600025A, 0x7AFECAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0x00007AFE, pctx_->GetGenRegPtr()[2] );
 EXPECT_EQ( 0x06000000, pctx_->GetGenRegPtr()[1] );
 EXPECT_EQ( 0x0000025A, pctx_->GetGenRegPtr()[0] );
}

TEST_F(MovaTest, movll0) {
//MOVLL0

  pctx_->GetGenRegPtr()[0]=0x0000025A;
  pctx_->GetGenRegPtr()[1]=0x06000000;
  pctx_->GetGenRegPtr()[2]=0xDEADDEAD;

  memSetWord( 0x06000246, 0x021E ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x0600025A, 0xCAFECAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0xCAFECAFE, pctx_->GetGenRegPtr()[2] );
 EXPECT_EQ( 0x06000000, pctx_->GetGenRegPtr()[1] );
 EXPECT_EQ( 0x0000025A, pctx_->GetGenRegPtr()[0] );
}

TEST_F(MovaTest, movbs0) {
//MOVBS0

  pctx_->GetGenRegPtr()[0]=0x0000025A;
  pctx_->GetGenRegPtr()[1]=0x06000000;
  pctx_->GetGenRegPtr()[2]=0xDEADDEAD;

  memSetWord( 0x06000246, 0x0124 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x0600025A, 0xCAFECAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0xADFECAFE, memGetLong(0x0600025A) );
 EXPECT_EQ( 0x06000000, pctx_->GetGenRegPtr()[1] );
 EXPECT_EQ( 0x0000025A, pctx_->GetGenRegPtr()[0] );
}

TEST_F(MovaTest, movws0) {
//MOVWS0

  pctx_->GetGenRegPtr()[0]=0x0000025A;
  pctx_->GetGenRegPtr()[1]=0x06000000;
  pctx_->GetGenRegPtr()[2]=0xDEADDEAD;

  memSetWord( 0x06000246, 0x0125 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x0600025A, 0xCAFECAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0xDEADCAFE, memGetLong(0x0600025A) );
 EXPECT_EQ( 0x06000000, pctx_->GetGenRegPtr()[1] );
 EXPECT_EQ( 0x0000025A, pctx_->GetGenRegPtr()[0] );
}

TEST_F(MovaTest, movls0) {
//MOVLS0

  pctx_->GetGenRegPtr()[0]=0x0000025A;
  pctx_->GetGenRegPtr()[1]=0x06000000;
  pctx_->GetGenRegPtr()[2]=0xDEADDEAD;

  memSetWord( 0x06000246, 0x0126 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x0600025A, 0xCAFECAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0xDEADDEAD, memGetLong(0x0600025A) );
 EXPECT_EQ( 0x06000000, pctx_->GetGenRegPtr()[1] );
 EXPECT_EQ( 0x0000025A, pctx_->GetGenRegPtr()[0] );
}

TEST_F(MovaTest, movblg) {
//MOVBLG

  pctx_->GetGenRegPtr()[0]=0xDEADDEAD;
  pctx_->SET_GBR(0x06000252);

  memSetWord( 0x06000246, 0xC408 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x0600025A, 0xCAFECAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0xFFFFFFCA, pctx_->GetGenRegPtr()[0] );

  pctx_->GetGenRegPtr()[0]=0xDEADDEAD;
  pctx_->SET_GBR(0x06000252);

  memSetWord( 0x06000246, 0xC488 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x060002DA, 0x7AFECAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0x0000007A, pctx_->GetGenRegPtr()[0] );
}

TEST_F(MovaTest, movwlg) {
//MOVWLG

  pctx_->GetGenRegPtr()[0]=0xDEADDEAD;
  pctx_->SET_GBR(0x06000252);

  memSetWord( 0x06000246, 0xC504 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x0600025A, 0xCAFECAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0xFFFFCAFE, pctx_->GetGenRegPtr()[0] );

  pctx_->GetGenRegPtr()[0]=0xDEADDEAD;
  pctx_->SET_GBR(0x06000252);

  memSetWord( 0x06000246, 0xC504 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x0600025A, 0x7AFECAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0x00007AFE, pctx_->GetGenRegPtr()[0] );
}

TEST_F(MovaTest, movllg) {
//MOVLLG

  pctx_->GetGenRegPtr()[0]=0xDEADDEAD;
  pctx_->SET_GBR(0x06000252);

  memSetWord( 0x06000246, 0xC602 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x0600025A, 0xCAFECAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0xCAFECAFE, pctx_->GetGenRegPtr()[0] );

  pctx_->GetGenRegPtr()[0]=0xDEADDEAD;
  pctx_->SET_GBR(0x06000252);

  memSetWord( 0x06000246, 0xC602 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x0600025A, 0x7AFECAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0x7AFECAFE, pctx_->GetGenRegPtr()[0] );
}

TEST_F(MovaTest, movbsg) {
//MOVBSG

  pctx_->GetGenRegPtr()[0]=0xDEADDEAD;
  pctx_->SET_GBR(0x06000252);

  memSetWord( 0x06000246, 0xC008 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x0600025A, 0xCAFECAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0xADFECAFE, memGetLong( 0x0600025A ) );
}

TEST_F(MovaTest, movwsg) {
//MOVWSG

  pctx_->GetGenRegPtr()[0]=0xDEADDEAD;
  pctx_->SET_GBR(0x06000252);

  memSetWord( 0x06000246, 0xC104 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x0600025A, 0xCAFECAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0xDEADCAFE, memGetLong( 0x0600025A ) );
}

TEST_F(MovaTest, movlsg) {
//MOVLSG

  pctx_->GetGenRegPtr()[0]=0xDEADDEAD;
  pctx_->SET_GBR(0x06000252);

  memSetWord( 0x06000246, 0xC202 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x0600025A, 0xCAFECAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0xDEADDEAD, memGetLong( 0x0600025A ) );
}

TEST_F(MovaTest, movt) {
//MOVT

  pctx_->GetGenRegPtr()[1]=0xDEADDEAD;
  pctx_->SET_SR(0xFFFFF3);

  memSetWord( 0x06000246, 0x0129 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0x1, pctx_->GetGenRegPtr()[1] );

  pctx_->GetGenRegPtr()[1]=0xDEADDEAD;
  pctx_->SET_SR(0xFFFFF2);

  memSetWord( 0x06000246, 0x0129 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0x0, pctx_->GetGenRegPtr()[1] );

//

  pctx_->GetGenRegPtr()[1]=0xFFFFFFFF;
  pctx_->GetGenRegPtr()[7]=0xDEADCAFE;

  memSetWord( 0x06000246, 0x6173 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0xDEADCAFE, pctx_->GetGenRegPtr()[1] );
 EXPECT_EQ( 0xDEADCAFE, pctx_->GetGenRegPtr()[7] );

  pctx_->GetGenRegPtr()[2]=0xFFFFFFFF;

  memSetWord( 0x06000246, 0xE216 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0x16, pctx_->GetGenRegPtr()[2] );

  pctx_->GetGenRegPtr()[2]=0x000000FF;

  memSetWord( 0x06000246, 0xE296 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0xFFFFFF96, pctx_->GetGenRegPtr()[2] );
}

TEST_F(MovaTest, movbl) {
//MOVBL
  pctx_->GetGenRegPtr()[0]=0x0600024C;
  pctx_->GetGenRegPtr()[1]=0xDEADCAFE;

  memSetWord( 0x06000246, 0x6100 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x0600024C, 0xFEADCADE);

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0x0600024C, pctx_->GetGenRegPtr()[0] );
 EXPECT_EQ( 0xFFFFFFFE, pctx_->GetGenRegPtr()[1] );

  pctx_->GetGenRegPtr()[0]=0x0600024C;
  pctx_->GetGenRegPtr()[1]=0xDEADCAFE;

  memSetWord( 0x06000246, 0x6100 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x0600024C, 0x7EADCADE);

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0x0600024C, pctx_->GetGenRegPtr()[0] );
 EXPECT_EQ( 0x7E, pctx_->GetGenRegPtr()[1] );
}

TEST_F(MovaTest, movwl) {
//MOVWL
  pctx_->GetGenRegPtr()[0]=0x06000250;
  pctx_->GetGenRegPtr()[1]=0xDEADCAFE;

  memSetWord( 0x06000246, 0x6101 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x06000250, 0xDEADCAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0xFFFFDEAD, pctx_->GetGenRegPtr()[1] );

  pctx_->GetGenRegPtr()[0]=0x06000250;
  pctx_->GetGenRegPtr()[1]=0xFFF00FFF;

  memSetWord( 0x06000246, 0x6101 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x06000250, 0x7EADCAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0x00007EAD, pctx_->GetGenRegPtr()[1] );
}

TEST_F(MovaTest, movwi) {
//MOVWI

  pctx_->GetGenRegPtr()[0]=0xDEADCAFE;

  memSetWord( 0x06000246, 0x9003 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x06000250, 0x7EADCAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0x00007EAD, pctx_->GetGenRegPtr()[0] );

  pctx_->GetGenRegPtr()[0]=0xDEADCAFE;

  memSetWord( 0x06000246, 0x9083 ); 
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop 
  memSetLong( 0x06000350, 0xDEADCAFE ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0xFFFFDEAD, pctx_->GetGenRegPtr()[0] );
}

TEST_F(MovaTest, movws) {
//MOVWS

  pctx_->GetGenRegPtr()[0]=0x06000256;
  pctx_->GetGenRegPtr()[2]=0xF0F0CAFE;

  memSetWord( 0x06000246, 0x2021 );
  memSetWord( 0x06000248, 0x000b );  // rts
  memSetWord( 0x0600024A, 0x0009 );  // nop
  memSetLong( 0x06000256, 0xDEADDEAD ); 

  pctx_->SET_PC( 0x06000246 );
  pctx_->Execute();

 EXPECT_EQ( 0xCAFE, memGetWord(0x06000256) );
 EXPECT_EQ( 0xDEAD,  memGetWord(0x06000258) );
 EXPECT_EQ( 0x06000256, pctx_->GetGenRegPtr()[0] );
 EXPECT_EQ( 0xF0F0CAFE, pctx_->GetGenRegPtr()[2] );

}

}  // namespace
