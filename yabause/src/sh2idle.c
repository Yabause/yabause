
#define MAX_CYCLE_CHECK 12

/* Register determinism markers */
/* 0 : register has not been changed. At the end of first pass, such registers are considered deterministic.
   1 : register content is "deterministic" : its content does not depend on the number of executed loops 
   -1 : register has been changed and is not proved to be deterministic
*/

static struct sbDet {
  u8 R[15];
  u8 GBR, VBR, MACL, MACH, PR;
  u8 SRT,SRS,SRI,SRQ,SRM;
  u8 Const;
} bDet;

#define delayCheck(PC) SH2idleCheckIterate( fetchlist[(PC >> 20) & 0x0FF](PC), PC )

#define implies(dest,src) dest = (src)?1:-1;
#define implies2(dest,dest2,src) dest = dest2 = (src)?1:-1;

#define destRB bDet.R[INSTRUCTION_B(instruction)]
#define destRC bDet.R[INSTRUCTION_C(instruction)]
#define destMACL bDet.MACL
#define destMACH bDet.MACH
#define destSR bDet.SRT=bDet.SRS=bDet.SRI=bDet.SRQ=bDet.SRM
#define destSRT bDet.SRT
#define destSRQ bDet.SRQ
#define destPR bDet.PR
#define destGBR bDet.GBR
#define destVBR bDet.VBR
#define destCONST bDet.Const
#define destR0 bDet.R[0]

#define srcSR (bDet.SRT==1 && bDet.SRS==1 && bDet.SRI==1 && bDet.SRQ==1 && bDet.SRM==1)
#define srcSRT (bDet.SRT==1)
#define srcSRS (bDet.SRS==1)
#define srcSRQ (bDet.SRQ==1)
#define srcSRM (bDet.SRM==1)
#define srcGBR (bDet.GBR == 1)
#define srcVBR (bDet.VBR == 1)
#define srcRC (bDet.R[INSTRUCTION_C(instruction)] == 1)
#define srcRB (bDet.R[INSTRUCTION_B(instruction)] == 1)
#define srcR0 (bDet.R[0] == 1)
#define srcMACL (bDet.MACL == 1)
#define srcMACH (bDet.MACH == 1)
#define srcMAC ( (bDet.MACL == 1)&&(bDet.MACH == 1) )
#define srcPR (bDet.PR==1)

#define isConst(src) if ( bDet.Const && !src ) return 0; 

int FASTCALL SH2idleCheckIterate(u16 instruction, u32 PC) {
  // update bDet after execution of <instruction>
  // return 0 : cannot continue idle check, probably because of a memory write

  switch (INSTRUCTION_A(instruction))
    {
    case 0:
      switch (INSTRUCTION_D(instruction))
	{
	case 2:
	  switch (INSTRUCTION_C(instruction))
	    {
	    case 0: implies(destRB, srcSR); //stcsr
	      break;
	    case 1: implies(destRB, srcGBR); //stcgbr;
	      break;
	    case 2: implies(destRB, srcVBR); //stcvbr;
	      break;
	    }
	  break;
	case 3:
	  switch (INSTRUCTION_C(instruction))
	    {
	    case 0: //bsrf;
	    case 2: return delayCheck(PC+2); //braf;
	    }
	  break;
	case 4: //movbs0;
	case 5: //movws0;
	case 6: return 0;  //movls0;
	case 7: implies(destMACL, srcRC && srcRB );  // mull
	  break;
	case 8:
	  switch (INSTRUCTION_C(instruction))
	    {
	    case 0:  //clrt;
	    case 1: bDet.SRT = 1;  //sett;
	      break;
	    case 2: bDet.MACL = bDet.MACH = 1;  //clrmac;
	      break;
	    }     
	  break;
	case 9:
	  switch (INSTRUCTION_C(instruction))
	    {
	    case 0:  //nop;
	      break;
	    case 1: bDet.SRM = bDet.SRQ = bDet.SRT = 1;  //div0u;
	      break;
	    case 2: implies(destRB, srcSRT );   //movt;
	      break;
	    }     
	  break;
	case 10:
	  switch (INSTRUCTION_C(instruction))
	    {
	    case 0: implies(destRB, srcMACH);   //stsmach;
	      break;
	    case 1: implies(destRB, srcMACL);   //stsmacl;
	      break;
	    case 2: implies(destRB, srcPR);   //stspr;
	      break;
	    }     
	  break;
	case 11:
	  switch (INSTRUCTION_C(instruction))
	    {
	    case 1: //sleep;
	      break;
	    case 0: //rts;
	    case 2: return delayCheck(PC+2);  //rte;
	    }     
	  break;
	case 12: //movbl0;
	case 13:  //movwl0;
	case 14:  implies(destRB, srcRC && srcR0);  //movll0;
	  break;
	case 15:  implies2(destMACH, destMACL, srcRC && srcRB && srcSRS && srcMACL && srcMACH); //macl
	  break;
	}
      break;
    case 1: return 0; //movls4;
    case 2:
      switch (INSTRUCTION_D(instruction))
	{
	case 0:  //movbs;
	case 1:  //movws;
	case 2:  //movls;
	case 4:  //movbm;
	case 5:  //movwm;
	case 6:  //movlm;
	case 7: implies(destSR, srcRC && srcRB ); //div0s
	  break;
	case 8: implies(destSRT, srcRC && srcRB ); //tst
	  break;
	case 9:  //y_and;
	case 10:  //y_xor;
	case 11: implies( destRB, srcRB && srcRC ); //y_or
	  break;
	case 12: implies( destSRT, srcRC && srcRB ); //cmpstr;
	  break;
	case 13: implies( destRB, srcRB && srcRC ); //xtrct
	  break;
	case 14: implies2( destMACL, destMACH, srcRB && srcRC ); //mulu
	  break;
	case 15: implies( destMACL, srcRB && srcRC ); //muls
	  break;
	}
      break;
    case 3:
      switch(INSTRUCTION_D(instruction))
	{
	case 0:  //cmpeq;
	case 2:   //cmphs;
	case 6:   //cmphi;
	case 7:   //cmpgt;
	case 3: implies( destSRT, srcRB && srcRC );
	  break;
	case 4: implies2( destSRQ, destSRT, srcRB && srcRC && srcSRQ && srcSRM );  //div1; /* CHECK ME */
	  break;
	case 13:   //dmuls;
	case 5: implies2( destMACL, destMACH, srcRB && srcRC ); // dmulu
	  break;
	case 8: implies( destRB, srcRB && srcRC ); //sub
	  break;
	case 15:  //addv;
	case 11: implies( destSRT, srcRB && srcRC ); //subv
	  break;
	case 12: implies( destRB, srcRB && srcRC ); //add
	  break;
	case 10:   //subc;
	case 14: implies2( destRB, destSRT, srcRB && srcRC && srcSRT ); //addc
	  break;
	}
      break;
    case 4:
      switch(INSTRUCTION_D(instruction))
	{
	case 0:
	  switch(INSTRUCTION_C(instruction))
	    {
	    case 0:  //shll;
	    case 1:  //dt;
	    case 2: implies( destSRT, srcRB ); //shal
	      break;
	    }
	  break;
	case 1:
	  switch(INSTRUCTION_C(instruction))
	    {
	    case 0:  //shlr;
	    case 1:  //cmppz;
	    case 2: implies( destSRT, srcRB ); //shar
	      break;
	    }     
	  break;
	case 2:
	  switch(INSTRUCTION_C(instruction))
	    {
	    case 0:  //stsmmach;
	    case 1:  //stsmmacl;
	    case 2:  return 0;//stsmpr;
	    }
	  break;
	case 3:
	  switch(INSTRUCTION_C(instruction))
	    {
	    case 0:  //stcmsr;
	    case 1:  //stcmgbr;
	    case 2: return 0; //stcmvbr;
	    }
	  break;
	case 4:
	  switch(INSTRUCTION_C(instruction))
	    {
	    case 0:  //rotl;
	    case 2: implies2( destRB, destSRT, srcRB ); //rotcl
	    }     
	  break;
	case 5:
	  switch(INSTRUCTION_C(instruction))
	    {
	    case 0: implies2( destRB, destSRT, srcRB ); //rotr;
	      break;
	    case 1: implies( destSRT, srcRB ); //cmppl;
	      break;
	    case 2: implies2( destSRT, destRB, srcSRT && srcRB ); //rotcr
	      break;
	    }       
          break;
	case 6:
	  switch(INSTRUCTION_C(instruction))
	    {
	    case 0: implies( destMACH, srcRB ); //ldsmmach
	      break;
	    case 1: implies( destMACL, srcRB ); //lsdmmacl
	      break;
	    case 2: implies( destPR, srcRB ); //ldsmpr
	      break;
	    }     
	  break;
	case 7:
	  switch(INSTRUCTION_C(instruction))
	    {
	    case 0: implies( destSR, srcRB ); //ldcmsr
	      break;
	    case 1: implies( destGBR, srcRB ); //ldcmgbr
	      break;
	    case 2: implies( destVBR, srcRB ); //lscmvbr
	      break;
	    }     
	  break;
	case 8:
	  switch(INSTRUCTION_C(instruction))
	    {
	    case 0:  //shll2;
	    case 1:  //shll8;
	    case 2: implies( destRB, srcRB ); //shll16
	    }     
	  break;
	case 9:
	  switch(INSTRUCTION_C(instruction))
	    {
	    case 0:  //shlr2;
	    case 1:  //shlr8;
	    case 2: implies( destRB, srcRB ); //shlr16
	    }     
	  break;
	case 10:
	  switch(INSTRUCTION_C(instruction))
	    {
	    case 0: implies( destMACH, srcRB ); //ldsmach
	      break;
	    case 1: implies( destMACL, srcRB ); //ldsmacl
	      break;
	    case 2: implies( destPR, srcRB ); //ldspr
	      break;
	    }     
	  break;
	case 11:
	  switch(INSTRUCTION_C(instruction))
	    {
	    case 0: isConst( srcRB );
	      bDet.PR = 1;
	      break; //jsr
	    case 1:  return 0; //tas;
	    case 2: isConst( srcRB );
	      return delayCheck(PC+2); //jmp
	    }     
	  break;
	case 14:
	  switch(INSTRUCTION_C(instruction))
	    {
	    case 0:  implies( destSR, srcRB ); //ldcsr
	      break;
	    case 1: implies( destGBR, srcRB ); //ldcgbr
	      break;
	    case 2: implies( destVBR, srcRB ); //ldcvrb
	      break;
	    }
	  break;
	case 15: implies2( destMACL, destMACH, srcRB && srcRC && srcMACL ); //macw
	  break;
	}
    case 5: implies( destRB, srcRC ); //movll4
      break;
    case 6:
      switch (INSTRUCTION_D(instruction))
	{
	case 6:   //movlp;
	case 5:   //movwp;
	case 4: return 0;  //movbp;
	case 0:  //movbl;
	case 1:   //movwl;
	case 2:   //movll;
	case 3:   //mov;
	case 7:   //y_not;
	case 8:   //swapb;
	case 11:  //neg;
	case 12:  //extub;
	case 13:  //extuw;
	case 14:  //extsb;
	case 15:  //extsw;
	case 9:  implies( destRB, srcRC ); //swapw;
	  break;
	case 10:  implies2( destRB, destSRT, srcRB && srcSRT ); //negc
	  break;
	}
      break;
    case 7: implies( destRB, srcRB ); //addi;
      break;
    case 8:
      switch (INSTRUCTION_B(instruction))
	{
	case 0:   //movbs4;
	case 1:  return 0;  //movws4;
	case 4:   //movbl4;
	case 5: implies( destR0, srcRC );   //movwl4;
	  break;
	case 8: implies( destSRT, srcR0 ); //cmpim;
	  break;
	case 9:   //bt;
	case 11:  //bf;
	case 13:  //bts;
	case 15: return 0; //bfs; /* FIX ME */
	}   
      break;
    case 9: bDet.R[INSTRUCTION_B(instruction)] = 1;  //movwi;
      break;
    case 10: return delayCheck(PC+2); //bra;
    case 11: bDet.PR = 1; return delayCheck(PC+2); //bsr;
    case 12:
      switch(INSTRUCTION_B(instruction))
	{
	case 0:   //movbsg;
	case 1:   //movwsg;
	case 2:   //movlsg;
	case 3: return 0;  //trapa;
	case 4:   //movblg;
	case 5:   //movwlg;
	case 6: implies( destR0, srcGBR ); //movllg
	  break;
	case 7: bDet.R[0] = 1;   //mova;
	  break;
	case 8: implies( destSRT, srcR0 );  //tsti;
	  break;
	case 9:   //andi;
	case 10:  //xori;
	case 11: implies( destR0, srcR0 ); //ori;
	  break;
	case 12: implies( destSRT, srcGBR && srcR0 );   //tstm;
	  break;
	case 13:  //andm;
	case 14:  //xorm;
	case 15:  return 0;//orm;
	}
      break;
    case 13: //movli;
    case 14:  bDet.R[INSTRUCTION_B(instruction)] = 1;  //movi;
      break;
    }
  return 1;
}

static u32 oldLoopBegin[2] = {0,0};
static u32 idleCheckCount = 0;
static u32 sh2cycleCount = 0;
static u32 sh2oldCycleCount = 0;
static u32 oldCheckCount = 0;

#ifdef IDLE_DETECT_VERBOSE
#define DROP_IDLE {\
    idleCheckCount += cycles - context->cycles; \
    context->cycles = cycles;}
#define IDLE_VERBOSE_SH2_COUNT {\
   sh2cycleCount += cycles; \
    if ( sh2cycleCount-sh2oldCycleCount > 0x8fffff ) { \
      fprintf( stderr, "%lu idle instructions dropped / %lu sh2 instructions parsed : %g \%\n", \
	       idleCheckCount-oldCheckCount, sh2cycleCount-sh2oldCycleCount, \
	       (float)(idleCheckCount-oldCheckCount)/(sh2cycleCount-sh2oldCycleCount)*100 ); \
      oldCheckCount = idleCheckCount; \
      sh2oldCycleCount = sh2cycleCount; \
    }}
#else
#define DROP_IDLE context->cycles = cycles;
#define IDLE_VERBOSE_SH2_COUNT
#endif

void SH2idleCheck(SH2_struct *context, u32 cycles) {
  // try to find an idle loop while interpreting

  int i;
  u8 isDelayed = 0;
  u32 loopEnd;
  u32 loopBegin;
  s32 disp;
  u32 cyclesCheckEnd;

  IDLE_VERBOSE_SH2_COUNT;

  // run until conditional branching - delayed instruction excluded

  for (;;) {
      SH2HandleBreakpoints(context);
      // Fetch Instruction
      context->instruction = fetchlist[(context->regs.PC >> 20) & 0x0FF](context->regs.PC);

      if ( INSTRUCTION_A(context->instruction)==8 ) {

	switch( INSTRUCTION_B(context->instruction) ) {
	case 13: //SH2bts
	  isDelayed = 1; 
	case 9:  //SH2bt
	  if (context->regs.SR.part.T != 1) {
	    context->regs.PC += 2;
	    context->cycles++;
	    return;
	  }
	  loopEnd = context->regs.PC;
	  disp = (s32)(s8)context->instruction;
	  loopBegin = context->regs.PC = context->regs.PC+(disp<<1)+4;
	  if ( isDelayed ) loopBegin -= 2;
	  context->cycles += 3;
	  goto branching_reached;
	  break;
	case 15: //SH2bfs
	  isDelayed = 1; 
	case 11: //SH2bf
	  if (context->regs.SR.part.T == 1) {
	    context->regs.PC += 2;
	    context->cycles++;
	    return;
	  }
	  loopEnd = context->regs.PC;
	  disp = (s32)(s8)context->instruction;
	  loopBegin = context->regs.PC = context->regs.PC+(disp<<1)+4;
	  if ( isDelayed ) loopBegin -= 2;
	  context->cycles += 3;
	  goto branching_reached;
	  break;
	default: opcodes[context->instruction](context);
	}
	} else opcodes[context->instruction](context);
      if ( context->cycles >= cycles ) return;
    }
 branching_reached:

  // if branching, execute (delayed included) until getting back to the conditional instruction  

  memset( &bDet, 0, sizeof( struct sbDet ) ); // initialize determinism markers

  cyclesCheckEnd = context->cycles + MAX_CYCLE_CHECK;
  //  if ( cycleCheckEnd > cycles ) cycleCheckEnd = cycles; 

  if ( isDelayed ) {
    context->instruction = fetchlist[((loopEnd+2) >> 20) & 0x0FF](loopEnd+2);
    opcodes[context->instruction](context);
    context->regs.PC -= 2;
    if ( !SH2idleCheckIterate(context->instruction,0) ) return;
  }
  
  // First pass

  while ( context->regs.PC != loopEnd ) {

    u32 PC = context->regs.PC;
    context->instruction = fetchlist[(PC >> 20) & 0x0FF](PC);
    opcodes[context->instruction](context);
    if ( !SH2idleCheckIterate(context->instruction,PC) ) return;    
    if ( context->cycles >= cyclesCheckEnd ) return;
  }

  // conditional jump 

  u32 PC = context->regs.PC;
  context->instruction = fetchlist[(PC >> 20) & 0x0FF](PC);
  opcodes[context->instruction](context);
  if ( context->regs.PC != loopBegin ) return;

  // Mark unchanged registers as deterministic registers

  #define markunchanged( val ) val = val ? 0: 1;
  for ( i=0 ; i < 16 ; i++ ) markunchanged(bDet.R[i]);
  markunchanged(bDet.GBR);
  markunchanged(bDet.VBR);
  markunchanged(bDet.MACL);
  markunchanged(bDet.MACH);
  markunchanged(bDet.PR);
  markunchanged(bDet.SRT);
  markunchanged(bDet.SRS);
  markunchanged(bDet.SRI);
  markunchanged(bDet.SRQ);
  markunchanged(bDet.SRM);
  bDet.Const = 1; // changing a "constant value" by a non-deterministic value is now forbidden

  // Second pass

  if ( isDelayed ) {

    context->instruction = fetchlist[((loopEnd+2) >> 20) & 0x0FF](loopEnd+2);
    opcodes[context->instruction](context);
    if ( !SH2idleCheckIterate(context->instruction,0) ) return;
  }

  while ( context->regs.PC != loopEnd ) {
    
    u32 PC = context->regs.PC;
    context->instruction = fetchlist[(PC >> 20) & 0x0FF](PC);
    opcodes[context->instruction](context);
    if ( !SH2idleCheckIterate(context->instruction,PC) ) return;    
  }
  context->instruction = fetchlist[(PC >> 20) & 0x0FF](PC);
  opcodes[context->instruction](context);  

#ifdef IDLE_DETECT_VERBOSE
  if (( bDet.SRT == 1 )&&(loopBegin!=oldLoopBegin[context==MSH2])) {
    printf( "New %s idle loop at %X -- %X\n", (context==MSH2)?"master":"slave", loopBegin, loopEnd );
    oldLoopBegin[context==MSH2] = loopBegin;
    if ( context->regs.PC != loopBegin ) fprintf( stderr, "SH2idleCheck : branching is not leading to begin of loop !\n" );
  }
#endif
  if ( bDet.SRT == 1 ) {
    DROP_IDLE;
    context->isIdle = 1;
  }
}

void SH2idleParse( SH2_struct *context, u32 cycles ) {
  // called when <context> is in idle state : check whether we are still idle

  IDLE_VERBOSE_SH2_COUNT;
  for(;;) {
    
    u32 PC = context->regs.PC;
    context->instruction = fetchlist[(PC >> 20) & 0x0FF](PC);
    if ( INSTRUCTION_A(context->instruction)==8 ) {
      switch( INSTRUCTION_B(context->instruction) ) {
      case 13: //SH2bts
      case 9:  //SH2bt
	if ( !context->regs.SR.part.T ) context->isIdle = 0;
	else DROP_IDLE;
	opcodes[context->instruction](context);
	return;
      case 15: //SH2bfs
      case 11: //SH2bf
	if ( context->regs.SR.part.T ) context->isIdle = 0;
	else DROP_IDLE;
	opcodes[context->instruction](context);
	return;
      }   
    }
    opcodes[context->instruction](context);
  }
}

/* ------------------------------------------------------ */
/* Code markers                                           */
/*
typedef struct {

  u32 begin, end;
  u16 instruction;
} idlemarker;

idlemarker idleMark[1024];
int topMark = 0;

void addMarker( u32 begin, u32 end ) {
 
  if ( topMark < 1024 ) {
    idleMark[topMark].instruction = MappedMemoryReadWord(begin);
    if ( (idleMark[topMark].instruction & 0xff0000ff) == OPCODE_HC ) return;
    idleMark[topMark].begin = begin;
    idleMark[topMark].end = end;
    idleMark[topMark].instruction = MappedMemoryReadWord(begin);
    MappedMemoryWriteWord( begin, OPCODE_HC | (topMark<<8) );  
    topMark++;
  } else printf( stderr, "Code Marker overflow !\n" );
}

void markerExec( SH2_struct *sh, u16 nMark ) {

  opcodes[sh->instruction = idleMark[instruction]](sh); // execute the hidden instruction

  for{;;} {
    
    u32 PC = sh->regs.PC;
    sh->instruction = fetchlist[(PC >> 20) & 0x0FF](PC);
    if ( INSTRUCTION_A(context->instruction)==8 ) {
      switch( INSTRUCTION_B(context->instruction) ) {
      case 13: //SH2bts
      case 9:  //SH2bt
	if ( sh->regs.SR.T )
	  cycles = 0xffffffff;
	return;
      case 15: //SH2bfs
      case 11: //SH2bf
	if ( ! sh->regs.SR.T )
	  cycles = 0xffffffff;
	return;
      }   
    opcodes[context->instruction](context);
  }
}
*/
