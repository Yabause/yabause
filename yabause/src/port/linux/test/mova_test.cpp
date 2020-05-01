#include "gtest/gtest.h"
#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory.h"
extern "C" {
extern void initEmulation();
}
namespace {

class MovaTest : public ::testing::Test {
 protected:
   SH2Interface_struct* pctx_;

  MovaTest() {
    initEmulation();
    pctx_ = SH2Core;
  }

  virtual ~MovaTest() {  
  }   

virtual void SetUp() {
 
  
}

virtual void TearDown() {


}

};

TEST_F(MovaTest, mova) {


  MSH2->regs.R[0]=0x00000000;


  // mova
  SH2MappedMemoryWriteWord(MSH2, 0x0600024c, 0xc72f );
  SH2MappedMemoryWriteWord(MSH2, 0x0600024e, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000250, 0x0009 );  // nop

  MSH2->regs.PC =( 0x0600024c );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x0600030c, MSH2->regs.R[0] );


  MSH2->regs.R[0]=0xFFFFFFFF;
  // mova
  SH2MappedMemoryWriteWord(MSH2, 0x0600024a, 0xc78f );
  SH2MappedMemoryWriteWord(MSH2, 0x0600024c, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024e, 0x0009 );  // nop

  MSH2->regs.PC =( 0x0600024a );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x06000488, MSH2->regs.R[0] );

}

TEST_F(MovaTest, mov) {

  MSH2->regs.R[7]=0xDEADDEAD;
  MSH2->regs.R[4]=0xCAFECAFE;

  SH2MappedMemoryWriteWord(MSH2, 0x0600024c, 0x6743 );  //MOVLI
  SH2MappedMemoryWriteWord(MSH2, 0x0600024e, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000250, 0x0009 );  // nop

  SH2MappedMemoryWriteWord(MSH2, 0x06000270, 0xDEAD );
  SH2MappedMemoryWriteWord(MSH2, 0x06000272 ,0xCAFE);

  MSH2->regs.PC =( 0x0600024c );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xCAFECAFE, MSH2->regs.R[7] );
 EXPECT_EQ( 0xCAFECAFE, MSH2->regs.R[4] );
}

TEST_F(MovaTest, movi) {

  MSH2->regs.R[7]=0xDEADDEAD;

  SH2MappedMemoryWriteWord(MSH2, 0x0600024c, 0xE743 );  //MOVI
  SH2MappedMemoryWriteWord(MSH2, 0x0600024e, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000250, 0x0009 );  // nop

  SH2MappedMemoryWriteWord(MSH2, 0x06000270, 0xDEAD );
  SH2MappedMemoryWriteWord(MSH2, 0x06000272 ,0xCAFE);

  MSH2->regs.PC =( 0x0600024c );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0x00000043, MSH2->regs.R[7] );

  MSH2->regs.R[7]=0xDEADDEAD;

  SH2MappedMemoryWriteWord(MSH2, 0x0600024c, 0xE783 );  //MOVI
  SH2MappedMemoryWriteWord(MSH2, 0x0600024e, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000250, 0x0009 );  // nop

  SH2MappedMemoryWriteWord(MSH2, 0x06000270, 0xDEAD );
  SH2MappedMemoryWriteWord(MSH2, 0x06000272 ,0xCAFE);

  MSH2->regs.PC =( 0x0600024c );
  SH2TestExec(MSH2, 1);

  EXPECT_EQ( 0xFFFFFF83, MSH2->regs.R[7] );

}

TEST_F(MovaTest, movli) {

  MSH2->regs.R[7]=0xFFFFFFFF;

  SH2MappedMemoryWriteWord(MSH2, 0x0600024c, 0xD708 );  //MOVLI
  SH2MappedMemoryWriteWord(MSH2, 0x0600024e, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000250, 0x0009 );  // nop

  SH2MappedMemoryWriteWord(MSH2, 0x06000270, 0xDEAD );
  SH2MappedMemoryWriteWord(MSH2, 0x06000272 ,0xCAFE);

  MSH2->regs.PC =( 0x0600024c );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xDEADCAFE, MSH2->regs.R[7] );

//MOVLI

  MSH2->regs.R[0]=0xFFFFFFFF;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0xD003 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop
  SH2MappedMemoryWriteLong(MSH2, 0x06000256, 0xDEADDEAD ); 
  SH2MappedMemoryWriteLong(MSH2, 0x06000254, 0xCAFECAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xCAFECAFE, MSH2->regs.R[0] );

  MSH2->regs.R[0]=0xFFFFFFFF;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0xD083 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop
  SH2MappedMemoryWriteLong(MSH2, 0x06000456, 0xDEADDEAD ); 
  SH2MappedMemoryWriteLong(MSH2, 0x06000454, 0xCAFECAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xCAFECAFE, MSH2->regs.R[0] );
}

TEST_F(MovaTest, movll) {

//MOVLL

  MSH2->regs.R[7]=0x06000270;
  MSH2->regs.R[6]=0xFFFFFFFF;

  SH2MappedMemoryWriteWord(MSH2, 0x0600024c, 0x6672 );  //MOVL_MEM_REG
  SH2MappedMemoryWriteWord(MSH2, 0x0600024e, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x06000250, 0x0009 );  // nop

  SH2MappedMemoryWriteWord(MSH2, 0x06000270, 0xDEAD );
  SH2MappedMemoryWriteWord(MSH2, 0x06000272 ,0xCAFE);

  MSH2->regs.PC =( 0x0600024c );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xDEADCAFE, MSH2->regs.R[6] );
 EXPECT_EQ( 0x06000270, MSH2->regs.R[7] );
}

TEST_F(MovaTest, movbs) {
//MOVBS
  MSH2->regs.R[0]=0x0600024C;
  MSH2->regs.R[3]=0xDEADCAFE;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x2030);  
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop
  SH2MappedMemoryWriteLong(MSH2, 0x0600024C, 0xFFFFFFFF ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xFE, SH2MappedMemoryReadByte(MSH2,0x0600024C) );


  MSH2->regs.R[0]=0xFFFFFFFF;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0xC71C );  //MOVL_MEM_REG
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( (0x06000246 & 0xFFFFFFFC)  + (0x1C << 2) + 4, MSH2->regs.R[0] );
}

TEST_F(MovaTest, movls) {
//MOVLS

  MSH2->regs.R[0]=0x0600024C;
  MSH2->regs.R[3]=0xDEADCAFE;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x2032 );  //MOVLS
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop
  SH2MappedMemoryWriteLong(MSH2, 0x0600024C, 0xFFFFFFFF ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xDEADCAFE, SH2MappedMemoryReadLong(MSH2,0x0600024C) );
}

TEST_F(MovaTest, movbp) {
//MOVBP
  MSH2->regs.R[1]=0x0600024C;
  MSH2->regs.R[7]=0xDEADCAFE;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x6714 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop
  SH2MappedMemoryWriteLong(MSH2, 0x0600024C, 0xCAFEDEAD );  // nop

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xFFFFFFCA, MSH2->regs.R[7] );
 EXPECT_EQ( 0x0600024D, MSH2->regs.R[1] );

  MSH2->regs.R[1]=0x0600024C;
  MSH2->regs.R[7]=0xDEADCAFE;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x6714 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop
  SH2MappedMemoryWriteLong(MSH2, 0x0600024C, 0x7AFEDEAD );  // nop

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0x0000007A, MSH2->regs.R[7] );
 EXPECT_EQ( 0x0600024D, MSH2->regs.R[1] );
}

TEST_F(MovaTest, movwp) {
//MOVWP

  MSH2->regs.R[1]=0x0600024C;
  MSH2->regs.R[7]=0xDEADCAFE;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x6715 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop
  SH2MappedMemoryWriteLong(MSH2, 0x0600024C, 0xCAFEDEAD );  // nop

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xFFFFCAFE, MSH2->regs.R[7] );
 EXPECT_EQ( 0x0600024E, MSH2->regs.R[1] );


  MSH2->regs.R[1]=0x0600024C;
  MSH2->regs.R[7]=0xDEADCAFE;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x6715 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop
  SH2MappedMemoryWriteLong(MSH2, 0x0600024C, 0x7AFEDEAD );  // nop

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0x00007AFE, MSH2->regs.R[7] );
 EXPECT_EQ( 0x0600024E, MSH2->regs.R[1] );
}

TEST_F(MovaTest, movlp) {
//MOVLP

  MSH2->regs.R[0]=0x0600024C;
  MSH2->regs.R[4]=0xFFFFFFFF;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x6406 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x0600024C, 0xDEADCAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0x0600024C + 0x4, MSH2->regs.R[0] );
 EXPECT_EQ( 0xDEADCAFE, MSH2->regs.R[4] );

  MSH2->regs.R[0]=0x0600024C;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x6006 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x0600024C, 0xDEADCAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xDEADCAFE, MSH2->regs.R[0] );
}

TEST_F(MovaTest, movbm) {
//MOVBM

  MSH2->regs.R[1]=0x0600024D;
  MSH2->regs.R[7]=0xDEADEAD;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x2174 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x0600024C, 0xCAFECAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0x0600024C, MSH2->regs.R[1] );
 EXPECT_EQ( 0xADFECAFE, SH2MappedMemoryReadLong(MSH2,0x0600024C) );
 EXPECT_EQ( 0xAD, SH2MappedMemoryReadByte(MSH2,0x0600024C) );
}

TEST_F(MovaTest, movwm) {
//MOVWM

  MSH2->regs.R[1]=0x0600024E;
  MSH2->regs.R[7]=0xDEADEAD;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x2175 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x0600024C, 0xCAFECAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0x0600024C, MSH2->regs.R[1] );
 EXPECT_EQ( 0xDEADCAFE, SH2MappedMemoryReadLong(MSH2,0x0600024C) );
 EXPECT_EQ( 0xDEAD, SH2MappedMemoryReadWord(MSH2,0x0600024C) );
}

TEST_F(MovaTest, movlm) {
//MOVLM

  MSH2->regs.R[1]=0x06000250;
  MSH2->regs.R[2]=0xDEADCAFE;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x2126 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x0600024C, 0xFFFFFFFF ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xDEADCAFE, SH2MappedMemoryReadLong(MSH2,0x0600024C) );
 EXPECT_EQ( 0x0600024C, MSH2->regs.R[1] );
}

TEST_F(MovaTest, movbl4) {
//MOVBL4

  MSH2->regs.R[1]=0x06000246;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x841A ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x06000250, 0xDEADCAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xFFFFFFDE, MSH2->regs.R[0] );
 EXPECT_EQ( 0x06000246, MSH2->regs.R[1] );

  MSH2->regs.R[1]=0x06000246;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x841A ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x06000250, 0x7EADCAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0x0000007E, MSH2->regs.R[0] );
 EXPECT_EQ( 0x06000246, MSH2->regs.R[1] );
}

TEST_F(MovaTest, movwl4) {
//MOVWL4

  MSH2->regs.R[1]=0x06000246;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x851A ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x0600025A, 0xDEADCAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xFFFFDEAD, MSH2->regs.R[0] );
 EXPECT_EQ( 0x06000246, MSH2->regs.R[1] );

  MSH2->regs.R[1]=0x06000246;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x851A ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x0600025A, 0x7EADCAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0x00007EAD, MSH2->regs.R[0] );
 EXPECT_EQ( 0x06000246, MSH2->regs.R[1] );
}

TEST_F(MovaTest, movll4) {
//MOVLL4

  MSH2->regs.R[1]=0x06000246;
  MSH2->regs.R[2]=0xDEADDEAD;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x5215 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x0600025A, 0xCAFECAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xCAFECAFE, MSH2->regs.R[2] );
 EXPECT_EQ( 0x06000246, MSH2->regs.R[1] );

  MSH2->regs.R[1]=0x06000246;
  MSH2->regs.R[2]=0xDEADDEAD;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x5218 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x06000266, 0xCAFECAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xCAFECAFE, MSH2->regs.R[2] );
 EXPECT_EQ( 0x06000246, MSH2->regs.R[1] );
}

TEST_F(MovaTest, movbs4) {
//MOVBS4

  MSH2->regs.R[1]=0x06000246;
  MSH2->regs.R[0]=0xDEADDEAD;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x801A ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x06000250, 0xCAFECAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xDEADDEAD, MSH2->regs.R[0] );
 EXPECT_EQ( 0x06000246, MSH2->regs.R[1] );
 EXPECT_EQ( 0xADFECAFE, SH2MappedMemoryReadLong(MSH2,0x06000250) );
 EXPECT_EQ( 0xAD, SH2MappedMemoryReadByte(MSH2,0x06000250) );
}

TEST_F(MovaTest, movws4) {
//MOVWS4

  MSH2->regs.R[1]=0x06000246;
  MSH2->regs.R[0]=0xDEADDEAD;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x8115 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x06000250, 0xCAFECAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xDEADDEAD, MSH2->regs.R[0] );
 EXPECT_EQ( 0x06000246, MSH2->regs.R[1] );
 EXPECT_EQ( 0xDEADCAFE, SH2MappedMemoryReadLong(MSH2,0x06000250) );
 EXPECT_EQ( 0xDEAD, SH2MappedMemoryReadWord(MSH2,0x06000250) );
}

TEST_F(MovaTest, movls4) {
//MOVLS4

  MSH2->regs.R[1]=0x06000246;
  MSH2->regs.R[2]=0xDEADDEAD;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x1125 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x0600025A, 0xCAFECAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xDEADDEAD, MSH2->regs.R[2] );
 EXPECT_EQ( 0x06000246, MSH2->regs.R[1] );
 EXPECT_EQ( 0xDEADDEAD, SH2MappedMemoryReadLong(MSH2,0x0600025A) );
}

TEST_F(MovaTest, movbl0) {
//MOVBL0

  MSH2->regs.R[0]=0x0000025A;
  MSH2->regs.R[1]=0x06000000;
  MSH2->regs.R[2]=0xDEADDEAD;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x021C ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x0600025A, 0xCAFECAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xFFFFFFCA, MSH2->regs.R[2] );
 EXPECT_EQ( 0x06000000, MSH2->regs.R[1] );
 EXPECT_EQ( 0x0000025A, MSH2->regs.R[0] );

  MSH2->regs.R[0]=0x0000025A;
  MSH2->regs.R[1]=0x06000000;
  MSH2->regs.R[2]=0xDEADDEAD;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x021C ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x0600025A, 0x7AFECAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0x0000007A, MSH2->regs.R[2] );
 EXPECT_EQ( 0x06000000, MSH2->regs.R[1] );
 EXPECT_EQ( 0x0000025A, MSH2->regs.R[0] );
}

TEST_F(MovaTest, movwl0) {
//MOVWL0

  MSH2->regs.R[0]=0x0000025A;
  MSH2->regs.R[1]=0x06000000;
  MSH2->regs.R[2]=0xDEADDEAD;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x021D ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x0600025A, 0xCAFECAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xFFFFCAFE, MSH2->regs.R[2] );
 EXPECT_EQ( 0x06000000, MSH2->regs.R[1] );
 EXPECT_EQ( 0x0000025A, MSH2->regs.R[0] );

  MSH2->regs.R[0]=0x0000025A;
  MSH2->regs.R[1]=0x06000000;
  MSH2->regs.R[2]=0xDEADDEAD;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x021D ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x0600025A, 0x7AFECAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0x00007AFE, MSH2->regs.R[2] );
 EXPECT_EQ( 0x06000000, MSH2->regs.R[1] );
 EXPECT_EQ( 0x0000025A, MSH2->regs.R[0] );
}

TEST_F(MovaTest, movll0) {
//MOVLL0

  MSH2->regs.R[0]=0x0000025A;
  MSH2->regs.R[1]=0x06000000;
  MSH2->regs.R[2]=0xDEADDEAD;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x021E ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x0600025A, 0xCAFECAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xCAFECAFE, MSH2->regs.R[2] );
 EXPECT_EQ( 0x06000000, MSH2->regs.R[1] );
 EXPECT_EQ( 0x0000025A, MSH2->regs.R[0] );
}

TEST_F(MovaTest, movbs0) {
//MOVBS0

  MSH2->regs.R[0]=0x0000025A;
  MSH2->regs.R[1]=0x06000000;
  MSH2->regs.R[2]=0xDEADDEAD;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x0124 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x0600025A, 0xCAFECAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xADFECAFE, SH2MappedMemoryReadLong(MSH2,0x0600025A) );
 EXPECT_EQ( 0x06000000, MSH2->regs.R[1] );
 EXPECT_EQ( 0x0000025A, MSH2->regs.R[0] );
}

TEST_F(MovaTest, movws0) {
//MOVWS0

  MSH2->regs.R[0]=0x0000025A;
  MSH2->regs.R[1]=0x06000000;
  MSH2->regs.R[2]=0xDEADDEAD;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x0125 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x0600025A, 0xCAFECAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xDEADCAFE, SH2MappedMemoryReadLong(MSH2,0x0600025A) );
 EXPECT_EQ( 0x06000000, MSH2->regs.R[1] );
 EXPECT_EQ( 0x0000025A, MSH2->regs.R[0] );
}

TEST_F(MovaTest, movls0) {
//MOVLS0

  MSH2->regs.R[0]=0x0000025A;
  MSH2->regs.R[1]=0x06000000;
  MSH2->regs.R[2]=0xDEADDEAD;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x0126 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x0600025A, 0xCAFECAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xDEADDEAD, SH2MappedMemoryReadLong(MSH2,0x0600025A) );
 EXPECT_EQ( 0x06000000, MSH2->regs.R[1] );
 EXPECT_EQ( 0x0000025A, MSH2->regs.R[0] );
}

TEST_F(MovaTest, movblg) {
//MOVBLG

  MSH2->regs.R[0]=0xDEADDEAD;
  MSH2->regs.GBR =(0x06000252);

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0xC408 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x0600025A, 0xCAFECAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xFFFFFFCA, MSH2->regs.R[0] );

  MSH2->regs.R[0]=0xDEADDEAD;
  MSH2->regs.GBR =(0x06000252);

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0xC488 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x060002DA, 0x7AFECAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0x0000007A, MSH2->regs.R[0] );
}

TEST_F(MovaTest, movwlg) {
//MOVWLG

  MSH2->regs.R[0]=0xDEADDEAD;
  MSH2->regs.GBR =(0x06000252);

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0xC504 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x0600025A, 0xCAFECAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xFFFFCAFE, MSH2->regs.R[0] );

  MSH2->regs.R[0]=0xDEADDEAD;
  MSH2->regs.GBR =(0x06000252);

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0xC504 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x0600025A, 0x7AFECAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0x00007AFE, MSH2->regs.R[0] );
}

TEST_F(MovaTest, movllg) {
//MOVLLG

  MSH2->regs.R[0]=0xDEADDEAD;
  MSH2->regs.GBR =(0x06000252);

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0xC602 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x0600025A, 0xCAFECAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xCAFECAFE, MSH2->regs.R[0] );

  MSH2->regs.R[0]=0xDEADDEAD;
  MSH2->regs.GBR =(0x06000252);

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0xC602 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x0600025A, 0x7AFECAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0x7AFECAFE, MSH2->regs.R[0] );
}

TEST_F(MovaTest, movbsg) {
//MOVBSG

  MSH2->regs.R[0]=0xDEADDEAD;
  MSH2->regs.GBR =(0x06000252);

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0xC008 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x0600025A, 0xCAFECAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xADFECAFE, SH2MappedMemoryReadLong(MSH2, 0x0600025A ) );
}

TEST_F(MovaTest, movwsg) {
//MOVWSG

  MSH2->regs.R[0]=0xDEADDEAD;
  MSH2->regs.GBR =(0x06000252);

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0xC104 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x0600025A, 0xCAFECAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xDEADCAFE, SH2MappedMemoryReadLong(MSH2, 0x0600025A ) );
}

TEST_F(MovaTest, movlsg) {
//MOVLSG

  MSH2->regs.R[0]=0xDEADDEAD;
  MSH2->regs.GBR =(0x06000252);

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0xC202 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x0600025A, 0xCAFECAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xDEADDEAD, SH2MappedMemoryReadLong(MSH2, 0x0600025A ) );
}

TEST_F(MovaTest, movt) {
//MOVT

  MSH2->regs.R[1]=0xDEADDEAD;
  MSH2->regs.SR.all =(0xFFFFF3);

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x0129 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0x1, MSH2->regs.R[1] );

  MSH2->regs.R[1]=0xDEADDEAD;
  MSH2->regs.SR.all =(0xFFFFF2);

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x0129 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0x0, MSH2->regs.R[1] );

//

  MSH2->regs.R[1]=0xFFFFFFFF;
  MSH2->regs.R[7]=0xDEADCAFE;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x6173 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xDEADCAFE, MSH2->regs.R[1] );
 EXPECT_EQ( 0xDEADCAFE, MSH2->regs.R[7] );

  MSH2->regs.R[2]=0xFFFFFFFF;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0xE216 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0x16, MSH2->regs.R[2] );

  MSH2->regs.R[2]=0x000000FF;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0xE296 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xFFFFFF96, MSH2->regs.R[2] );
}

TEST_F(MovaTest, movbl) {
//MOVBL
  MSH2->regs.R[0]=0x0600024C;
  MSH2->regs.R[1]=0xFFFFFFFF;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x6100 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x0600024C, 0xFEADCADE);

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0x0600024C, MSH2->regs.R[0] );
 EXPECT_EQ( 0xFFFFFFFE, MSH2->regs.R[1] );

  MSH2->regs.R[0]=0x0600024C;
  MSH2->regs.R[1]=0xFFFFFFFF;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x6100 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x0600024C, 0x7EADCADE);

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0x0600024C, MSH2->regs.R[0] );
 EXPECT_EQ( 0x7E, MSH2->regs.R[1] );
}

TEST_F(MovaTest, movwl) {
//MOVWL
  MSH2->regs.R[0]=0x06000250;
  MSH2->regs.R[1]=0xFFFFFFFF;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x6101 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x06000250, 0xDEADCAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xFFFFDEAD, MSH2->regs.R[1] );

  MSH2->regs.R[0]=0x06000250;
  MSH2->regs.R[1]=0xFFF00FFF;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x6101 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x06000250, 0x7EADCAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0x00007EAD, MSH2->regs.R[1] );
}

TEST_F(MovaTest, movwi) {
//MOVWI

  MSH2->regs.R[0]=0xFFFFFFFF;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x9003 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x06000250, 0x7EADCAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0x00007EAD, MSH2->regs.R[0] );

  MSH2->regs.R[0]=0xFFFFFFFF;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x9083 ); 
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop 
  SH2MappedMemoryWriteLong(MSH2, 0x06000350, 0xDEADCAFE ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xFFFFDEAD, MSH2->regs.R[0] );
}

TEST_F(MovaTest, movws) {
//MOVWS

  MSH2->regs.R[0]=0x06000256;
  MSH2->regs.R[2]=0xF0F0CAFE;

  SH2MappedMemoryWriteWord(MSH2, 0x06000246, 0x2021 );
  SH2MappedMemoryWriteWord(MSH2, 0x06000248, 0x000b );  // rts
  SH2MappedMemoryWriteWord(MSH2, 0x0600024A, 0x0009 );  // nop
  SH2MappedMemoryWriteLong(MSH2, 0x06000256, 0xDEADDEAD ); 

  MSH2->regs.PC =( 0x06000246 );
  SH2TestExec(MSH2, 1);

 EXPECT_EQ( 0xCAFE, SH2MappedMemoryReadWord(MSH2,0x06000256) );
 EXPECT_EQ( 0xDEAD,  SH2MappedMemoryReadWord(MSH2,0x06000258) );
 EXPECT_EQ( 0x06000256, MSH2->regs.R[0] );
 EXPECT_EQ( 0xF0F0CAFE, MSH2->regs.R[2] );

}

}  // namespace
